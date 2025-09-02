#include <linux/io.h>
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/timer.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>

#define DBCHECKER_BASE_ADDR 0x40000000
#define DBCHECKER_REG_SIZE 8 // 64bit
#define DBCHECKER_REG_NUM 8  // 8 registers

#define DBCHECKER_EN_OFFSET         0x00
#define DBCHECKER_CMD_OFFSET        0x08
#define DBCHECKER_RES_OFFSET        0x10
#define DBCHECKER_KEYL_OFFSET       0x18
#define DBCHECKER_KEYH_OFFSET       0x20
#define DBCHECKER_ERR_CNT_OFFSET    0x28
#define DBCHECKER_ERR_INFO_OFFSET   0x30
#define DBCHECKER_ERR_MTDT_OFFSET   0x38

#define DBCHECKER_BNDS_MAX_SIZE  0x10000 // 64KB
#define DBCHECKER_BNDL_MAX_SIZE  0x100000000 // 4GB

#define DBCHECKER_DEBUG 1

static void __iomem *dbchecker_rf;
static struct timer_list dbchecker_timer;

int dbchecker_command(uint64_t type, uint64_t imm){
    uint64_t validated_cmd = (0x1UL << 62) | ((type & 0x3) << 60) | (imm & 0x0FFFFFFFFFFFFFFF);
    iowrite64(validated_cmd, dbchecker_rf + DBCHECKER_CMD_OFFSET);
    while (validated_cmd >> 63 != 0x1){
        validated_cmd = ioread64(dbchecker_rf + DBCHECKER_CMD_OFFSET); // wait for complete
    }
    if (validated_cmd & (0x1UL << 62)) {
        if (DBCHECKER_DEBUG) printk("DBCHECKER: command complete, type: %llu, imm: %llu\n", (validated_cmd >> 60) & 0x3, validated_cmd & 0x0FFFFFFFFFFFFFFF);
        return 0;
    }
    if (DBCHECKER_DEBUG) printk("DBCHECKER: command error, cmd: %llu\n", validated_cmd);
    return -1;
}

void dbchecker_en_set(int a){
    iowrite64(a, dbchecker_rf + DBCHECKER_EN_OFFSET);
}

dma_addr_t dbchecker_alloc_mtdt(dma_addr_t addr, size_t size, enum dma_data_direction dir){
    uint64_t metadata = 0;
    uint64_t rw = (dir == DMA_BIDIRECTIONAL)? 0x3 : // RW
                  (dir == DMA_FROM_DEVICE)? 0x2 : // WO
                  (dir == DMA_TO_DEVICE)? 0x1 : // RO
                  0x0; //INVALID
                  
    if (size > DBCHECKER_BNDL_MAX_SIZE){
        if (DBCHECKER_DEBUG) printk("DBCHECKER: allocation size exceeds limit, addr: %llu, size: %zu\n", addr, size);
        return addr;
    }
    else if (size > DBCHECKER_BNDS_MAX_SIZE){
        if (DBCHECKER_DEBUG) printk("DBCHECKER: alloc large bound, addr: %llu, size: %zu\n", addr, size);
        if (addr & 0xFFFF) {
            if (DBCHECKER_DEBUG) printk("DBCHECKER: unaligned large bound, align to 64KB\n");
        }
        metadata = (addr >> 16 & 0xFFFFF) | (size << 20) | (rw << 52) | (0x1UL << 54);
    }
    else {
        if (DBCHECKER_DEBUG) printk("DBCHECKER: alloc small bound, addr: %llu, size: %zu\n", addr, size);
        metadata = (addr & 0xFFFFFFFFF) | (size << 36) | (rw << 52) | (0x1UL << 54);
    }
    if (!dbchecker_command(0x1UL, metadata)) 
        return ioread64(dbchecker_rf + DBCHECKER_RES_OFFSET);
    else return -1;
}
EXPORT_SYMBOL(dbchecker_alloc_mtdt);

dma_addr_t dbchecker_free_mtdt(dma_addr_t addr){
    dbchecker_command(0x1UL, addr >> 36);
    return addr & 0xFFFFFFFFF; // orig addr
}
EXPORT_SYMBOL(dbchecker_free_mtdt);

int dbchecker_err_handler(void){
    uint64_t cnt = ioread64(dbchecker_rf + DBCHECKER_ERR_CNT_OFFSET);
    uint64_t info = ioread64(dbchecker_rf + DBCHECKER_ERR_INFO_OFFSET);
    uint64_t mtdt = ioread64(dbchecker_rf + DBCHECKER_ERR_MTDT_OFFSET);
    if (cnt & ~0xF){
        printk("DBCHECKER: error detected!\n");
        printk("DBCHECKER: error count: %llu, info: %llu, mtdt: %llu\n", cnt, info, mtdt);
        dbchecker_command(0x2, 0);
    }
    return 0;
}

void dbchecker_timer_func(struct timer_list *t)
{
    dbchecker_err_handler();
    mod_timer(&dbchecker_timer, jiffies + msecs_to_jiffies(1000)); // 1秒周期
}

static int __init dbchecker_module_init(void)
{
    dbchecker_rf = ioremap(DBCHECKER_BASE_ADDR, DBCHECKER_REG_NUM * DBCHECKER_REG_SIZE);
    timer_setup(&dbchecker_timer, dbchecker_timer_func, 0);
    mod_timer(&dbchecker_timer, jiffies + msecs_to_jiffies(1000));
    dbchecker_en_set(1);
    printk("DBCHECKER: init\n");
    return 0;
}

static void __exit dbchecker_module_exit(void)
{
    dbchecker_en_set(0);
    del_timer_sync(&dbchecker_timer);
    iounmap(dbchecker_rf);
    printk("DBCHECKER: exit\n");
}


module_init(dbchecker_module_init);
module_exit(dbchecker_module_exit);

MODULE_AUTHOR("Gwins7");
MODULE_DESCRIPTION("DBChecker driver");
MODULE_LICENSE("GPL v2");