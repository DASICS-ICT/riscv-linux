#include <linux/io.h>
#include <linux/io-64-nonatomic-lo-hi.h>
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/timer.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>

#define DBCHECKER_BASE_ADDR 0x40000000
#define DBCHECKER_REG_SIZE 8 // 64bit
#define DBCHECKER_REG_NUM 10  // 10 registers

#define DBCHECKER_EN_OFFSET         0x00U
#define DBCHECKER_CMD_OFFSET        0x08U
#define DBCHECKER_MTDT_LO_OFFSET    0x10U
#define DBCHECKER_MTDT_HI_OFFSET    0x18U
#define DBCHECKER_RES_OFFSET        0x20U
#define DBCHECKER_KEYL_OFFSET       0x28U
#define DBCHECKER_KEYH_OFFSET       0x30U
#define DBCHECKER_ERR_CNT_OFFSET    0x38U
#define DBCHECKER_ERR_INFO_OFFSET   0x40U
#define DBCHECKER_ERR_MTDT_OFFSET   0x48U

#define DBCHECKER_DEBUG 0
#define DBCHECKER_DEBUG_LOG(fmt, args...) \
	do { \
		if (DBCHECKER_DEBUG) \
			pr_info(fmt, ##args); \
	} while (0)

#define MAX_DBTE_TABLE_SIZE 4096 // log2 = 12, offset = 52

enum dbchecker_cmd_op {
  DBCHECKER_OP_FREE,
  DBCHECKER_OP_ALLOC,
  DBCHECKER_OP_CLEAR,
  DBCHECKER_OP_SWITCH
};

enum dbchecker_cmd_status {
    DBCHECKER_CMD_INVALID,
    DBCHECKER_CMD_REQUEST,
    DBCHECKER_CMD_DONE,
    DBCHECKER_CMD_ERROR
};

enum dbchecker_rw_mode {
    DBCHECKER_RWMODE_INVALID,
    DBCHECKER_RWMODE_RO,
    DBCHECKER_RWMODE_WO,
    DBCHECKER_RWMODE_RW
};

struct dbchecker_mtdt {
  uint8_t  wr     : 2; // 0: INVALID, 1: RO, 2: WO, 3: RW
  uint8_t  dev    : 5; // device id
  uint32_t id     : 25;
  uint64_t up_bnd : 48;
  uint64_t lo_bnd : 48;
};

struct dbchecker_cmd {
  uint8_t  status : 2; // 00: inv, 01: req, 10: done, 11: err
  uint8_t  op     : 2; // 00: free, 01: alloc, 10: clear, 11: switch
  uint8_t  pad    : 8;
  uint64_t imm    : 52;
  struct dbchecker_mtdt mtdt; // only used for alloc cmd and switch cmd
};

struct dbchecker_en_ctrl {
  bool func_en;
  bool intr_en;
  bool intr_clr;
  bool stall_mode;
  bool err_byp;
  bool err_rpt;
};

static void __iomem *dbchecker_rf;
static struct timer_list dbchecker_timer;
static uint32_t dbte_table[MAX_DBTE_TABLE_SIZE];

int dbchecker_command(struct dbchecker_cmd *cmd){
    if (cmd == NULL)
        return -1;

    DBCHECKER_DEBUG_LOG("DBCHECKER: issue command, op: 0x%x, imm: 0x%llx\n",
        cmd->op, cmd->imm);
    DBCHECKER_DEBUG_LOG("DBCHECKER: mtdt wr: 0x%x, dev: 0x%x, id: 0x%lx, up_bnd: 0x%llx, lo_bnd: 0x%llx\n",
        cmd->mtdt.wr, cmd->mtdt.dev, cmd->mtdt.id, cmd->mtdt.up_bnd, cmd->mtdt.lo_bnd);
    uint64_t validated_cmd = (0x1UL << 62) | 
                             ((uint64_t)(cmd->op & 0x3) << 60) | 
                             (cmd->imm & 0x0FFFFFFFFFFFFFULL);
    DBCHECKER_DEBUG_LOG("DBCHECKER: validated cmd: 0x%llx\n", validated_cmd);
    uint32_t cmd_status;
    if (cmd->op == DBCHECKER_OP_ALLOC) { // alloc
        uint64_t mtdt_lo = (cmd->mtdt.lo_bnd & 0xFFFFFFFFFFFF) |
                           ((cmd->mtdt.up_bnd & 0xFFFFFFFFFFFF) << 48);

        uint64_t mtdt_hi = ((cmd->mtdt.up_bnd & 0xFFFFFFFFFFFF) >> 16) |
                           ((uint64_t)(cmd->mtdt.wr & 0x3) << 62);
        DBCHECKER_DEBUG_LOG("DBCHECKER: mtdt_lo: 0x%llx, mtdt_hi: 0x%llx\n", mtdt_lo, mtdt_hi);
        iowrite64_lo_hi(mtdt_lo, dbchecker_rf + DBCHECKER_MTDT_LO_OFFSET);
        iowrite64_lo_hi(mtdt_hi, dbchecker_rf + DBCHECKER_MTDT_HI_OFFSET);
        wmb();
        iowrite32((validated_cmd >> 32) & 0xFFFFFFFF, dbchecker_rf + DBCHECKER_CMD_OFFSET + 4);
    } else
        iowrite64_lo_hi(validated_cmd, dbchecker_rf + DBCHECKER_CMD_OFFSET);

    do {
        cmd_status = (ioread32(dbchecker_rf + DBCHECKER_CMD_OFFSET + 4) >> 30) & 0x3;
    } while (cmd_status == DBCHECKER_CMD_REQUEST);

    if (cmd_status == DBCHECKER_CMD_ERROR) {
        pr_err("DBCHECKER: command error, cmd: 0x%llx\n", validated_cmd);
        return -1;
    }
    DBCHECKER_DEBUG_LOG("DBCHECKER: command completed, op: 0x%x, imm: 0x%llx\n",
        cmd->op, cmd->imm);
    return 0;
}

void dbchecker_en_set(struct dbchecker_en_ctrl *ctrl){
    uint32_t en_val = 0;
    en_val |= (ctrl->func_en) |
              (ctrl->intr_en << 1) |
              (ctrl->intr_clr << 2) |
              (ctrl->stall_mode << 3) |
              (ctrl->err_byp << 4) |
              (ctrl->err_rpt << 5);

    iowrite32(en_val, dbchecker_rf + DBCHECKER_EN_OFFSET);
}
uint32_t dbchecker_en_get(void){
    return ioread32(dbchecker_rf + DBCHECKER_EN_OFFSET);
}

dma_addr_t dbchecker_alloc_mtdt(dma_addr_t addr, size_t size, enum dma_data_direction dir){
    if (!(dbchecker_en_get() & 0x1))
        return addr; // not enabled
    
    struct dbchecker_mtdt mtdt;
    mtdt.wr = (dir == DMA_BIDIRECTIONAL) ? DBCHECKER_RWMODE_RW :
              (dir == DMA_FROM_DEVICE) ? DBCHECKER_RWMODE_WO :
              (dir == DMA_TO_DEVICE) ? DBCHECKER_RWMODE_RO :
               DBCHECKER_RWMODE_INVALID;
    
    dma_addr_t alloc_addr;
    mtdt.lo_bnd = addr & 0xFFFFFFFFFFFFULL;
    mtdt.up_bnd = (addr + size - 1) & 0xFFFFFFFFFFFFULL;
    
    struct dbchecker_cmd alloc_cmd;
    alloc_cmd.op = DBCHECKER_OP_ALLOC;
    alloc_cmd.mtdt = mtdt;
    DBCHECKER_DEBUG_LOG("DBCHECKER: request alloc mtdt, lo: 0x%llx, up: 0x%llx\n",
        mtdt.lo_bnd, mtdt.up_bnd);
    if (!dbchecker_command(&alloc_cmd)) {
        uint32_t cmd_res = ioread32(dbchecker_rf + DBCHECKER_RES_OFFSET + 4);
        alloc_addr = (addr & 0xFFFFFFFFFFFF) | ((uint64_t)(cmd_res & 0xFFF00000) << 32); // new addr
        DBCHECKER_DEBUG_LOG("DBCHECKER: construct alloc_addr: 0x%llx | 0x%llx\n", 
        addr & 0xFFFFFFFFFFFF, (uint64_t)(cmd_res & 0xFFF00000) << 32);
        dbte_table[cmd_res >> 20] = &mtdt; // store for future free
        DBCHECKER_DEBUG_LOG("DBCHECKER: alloc addr: 0x%llx, save metadata 0x%lx, index 0x%lx\n", 
        alloc_addr, cmd_res, cmd_res >> 20);
        return alloc_addr;
    } else
        return -1;
}
EXPORT_SYMBOL(dbchecker_alloc_mtdt);

dma_addr_t dbchecker_free_mtdt(dma_addr_t addr){
    if (!(dbchecker_en_get() & 0x1)) 
        return addr; // dbchecker not enabled

    struct dbchecker_cmd free_cmd;
    free_cmd.op = DBCHECKER_OP_FREE;
    free_cmd.imm = (((addr >> 52) & 0xFFF) << 40)  | (addr & 0xFFFFFFFFULL);
    dbchecker_command(&free_cmd);
    DBCHECKER_DEBUG_LOG("DBCHECKER: free addr: 0x%llx\n", addr);
    dbte_table[addr >> 52] = NULL; // clear entry
    return addr & 0xFFFFFFFFFFFF; // orig addr
}
EXPORT_SYMBOL(dbchecker_free_mtdt);

int dbchecker_err_handler(void){
    uint64_t cnt = ioread64_lo_hi(dbchecker_rf + DBCHECKER_ERR_CNT_OFFSET);
    uint64_t info = ioread64_lo_hi(dbchecker_rf + DBCHECKER_ERR_INFO_OFFSET);
    uint64_t mtdt = ioread64_lo_hi(dbchecker_rf + DBCHECKER_ERR_MTDT_OFFSET);
    if (cnt & ~0xF){
        pr_err("DBCHECKER: error detected!\n");
        pr_err("DBCHECKER: error count: 0x%llx, info: 0x%llx, mtdt: 0x%llx\n", cnt, info, mtdt);
        struct dbchecker_cmd err_cmd;
        err_cmd.op = DBCHECKER_OP_CLEAR;
        dbchecker_command(&err_cmd);
    }
    return 0;
}

void dbchecker_timer_func(struct timer_list *t)
{
    dbchecker_err_handler();
    mod_timer(&dbchecker_timer, jiffies + msecs_to_jiffies(100)); // 1秒周期
}

static int __init dbchecker_module_init(void)
{
    dbchecker_rf = ioremap(DBCHECKER_BASE_ADDR, DBCHECKER_REG_NUM * DBCHECKER_REG_SIZE);
    timer_setup(&dbchecker_timer, dbchecker_timer_func, 0);
    mod_timer(&dbchecker_timer, jiffies + msecs_to_jiffies(100));
    struct dbchecker_en_ctrl ctrl = {
        .func_en = true,
        .intr_en = false,
        .intr_clr = false,
        .stall_mode = true,
        .err_byp = false,
        .err_rpt = true
    };
    dbchecker_en_set(&ctrl);
    printk("DBCHECKER: init\n");
    return 0;
}

static void __exit dbchecker_module_exit(void)
{
    struct dbchecker_en_ctrl ctrl;
    ctrl.func_en = false;
    dbchecker_en_set(&ctrl);
    del_timer_sync(&dbchecker_timer);
    iounmap(dbchecker_rf);
    printk("DBCHECKER: exit\n");
}


module_init(dbchecker_module_init);
module_exit(dbchecker_module_exit);

MODULE_AUTHOR("Gwins7");
MODULE_DESCRIPTION("DBChecker driver");
MODULE_LICENSE("GPL v2");