#include <linux/kernel.h>
#include <linux/init.h>

#include <asm/csr.h>    
#include <asm/kdasics.h>

#ifdef CONFIG_64BIT
#define STEP 8
#else 
#define STEP 4
#endif 

void dasics_init_umain_bound(uint64_t cfg, uint64_t hi, uint64_t lo)
{
    csr_write(0x9e0, cfg);  // DasicsUMainCfg
    csr_write(0x9e3, hi);   // DasicsUMainBoundHi
    csr_write(0x9e2, lo);   // DasicsUMainBoundLo
}

void dasics_init_smaincall(uint64_t entry)
{
    csr_write(0x8b0, entry);  // DasicsMaincallEntry
}

uint64_t dasics_smaincall(SmaincallTypes type, uint64_t arg0, uint64_t arg1)
{
    //save_smaincall();
    uint64_t retval = 0;

    // TODO: change to linux version
    const char *fmt = (const char*)arg0;
    switch (type)
    {
        case Smaincall_PRINT:
            printk(fmt, arg1);
            break;
        default:
            pr_cont("Warning: Invalid smaincall number %u!\n", type);
            break;
    }

    // TODO: Use compiler to optimize such ugly code in the future ...
    // Compile Error: gcc doesn't recognize pulpret. Use machine code instead.
    /*
    asm volatile("mv        a0, a5\n"\
                 "ld        ra, 88(sp)\n"\
                 "ld        s0, 80(sp)\n"\
                 "addi      sp, sp, 96\n"\
                 "ret\n"\
                 //".word 0x0000f00b \n" /* dasicsret x0,  0, x1 in little endian */ 
    /*             "nop");
    */
    //restore_smaincall();
    return retval;
}
EXPORT_SYMBOL(dasics_smaincall);

int32_t dasics_libcfg_kalloc(uint64_t cfg, uint64_t hi, uint64_t lo)
{
    uint64_t libcfg0 = csr_read(0x880);  // DasicsLibCfg0

    //printk("[dasics_kallock] libcfg0: 0x%lx libcfg1: 0x%lx\n",libcfg0,libcfg1);

    int32_t max_cfgs = DASICS_LIBCFG_WIDTH;
    int32_t step = 4;
    int32_t idx;

    for (idx = 0; idx < max_cfgs; ++idx)
    {
        uint64_t curr_cfg = (libcfg0 >> (idx * step)) & DASICS_LIBCFG_MASK;

        if ((curr_cfg & DASICS_LIBCFG_V) == 0)  // Find avaliable cfg
        {
            // Write DASICS boundary csrs
            switch (idx)
            {
                case 0:
                    csr_write(0x890, lo);  // DasicsLibBound0Lo
                    csr_write(0x891, hi);  // DasicsLibBound0Hi
                    break;
                case 1:
                    csr_write(0x892, lo);  // DasicsLibBound1Lo
                    csr_write(0x893, hi);  // DasicsLibBound1Hi
                    break;
                case 2:
                    csr_write(0x894, lo);  // DasicsLibBound2Lo
                    csr_write(0x895, hi);  // DasicsLibBound2Hi
                    break;
                case 3:
                    csr_write(0x896, lo);  // DasicsLibBound3Lo
                    csr_write(0x897, hi);  // DasicsLibBound3Hi
                    break;
                case 4:
                    csr_write(0x898, lo);  // DasicsLibBound4Lo
                    csr_write(0x899, hi);  // DasicsLibBound4Hi
                    break;
                case 5:
                    csr_write(0x89a, lo);  // DasicsLibBound5Lo
                    csr_write(0x89b, hi);  // DasicsLibBound5Hi
                    break;
                case 6:
                    csr_write(0x89c, lo);  // DasicsLibBound6Lo
                    csr_write(0x89d, hi);  // DasicsLibBound6Hi
                    break;
                case 7:
                    csr_write(0x89e, lo);  // DasicsLibBound7Lo
                    csr_write(0x89f, hi);  // DasicsLibBound7Hi
                    break;
                case 8:
                    csr_write(0x8a0, lo);  // DasicsLibBound8Lo
                    csr_write(0x8a1, hi);  // DasicsLibBound8Hi
                    break;
                case 9:
                    csr_write(0x8a2, lo);  // DasicsLibBound9Lo
                    csr_write(0x8a3, hi);  // DasicsLibBound9Hi
                    break;
                case 10:
                    csr_write(0x8a4, lo);  // DasicsLibBound10Lo
                    csr_write(0x8a5, hi);  // DasicsLibBound10Hi
                    break;
                case 11:
                    csr_write(0x8a6, lo);  // DasicsLibBound11Lo
                    csr_write(0x8a7, hi);  // DasicsLibBound11Hi
                    break;
                case 12:
                    csr_write(0x8a8, lo);  // DasicsLibBound12Lo
                    csr_write(0x8a9, hi);  // DasicsLibBound12Hi
                    break;
                case 13:
                    csr_write(0x8aa, lo);  // DasicsLibBound13Lo
                    csr_write(0x8ab, hi);  // DasicsLibBound13Hi
                    break;
                case 14:
                    csr_write(0x8ac, lo);  // DasicsLibBound14Lo
                    csr_write(0x8ad, hi);  // DasicsLibBound14Hi
                    break;
                default:
                    csr_write(0x8ae, lo);  // DasicsLibBound15Lo
                    csr_write(0x8af, hi);  // DasicsLibBound15Hi
                    break;
            }

            libcfg0 &= ~(DASICS_LIBCFG_MASK << (idx * step));
            libcfg0 |= ((cfg | DASICS_LIBCFG_V) & DASICS_LIBCFG_MASK) << (idx * step);
            csr_write(0x880, libcfg0);  // DasicsLibCfg0
            return idx;
        }
    }

    return -1;
}
EXPORT_SYMBOL(dasics_libcfg_kalloc);

int32_t dasics_libcfg_kfree(int32_t idx)
{
    if (idx < 0 || idx >= DASICS_LIBCFG_WIDTH)
    {
        return -1;
    }

    int32_t step = 4;

    uint64_t libcfg = csr_read(0x880);  // DasicsLibCfg0
    libcfg &= ~(DASICS_LIBCFG_V << (idx * step));

    csr_write(0x880, libcfg);  // DasicsLibCfg0

    return 0;
}
EXPORT_SYMBOL(dasics_libcfg_kfree);

uint32_t dasics_libcfg_kget(int32_t idx)
{
    if (idx < 0 || idx >= DASICS_LIBCFG_WIDTH)
    {
        return -1;
    }

    int32_t step = 4;

    uint64_t libcfg = csr_read(0x880);  // DasicsLibCfg0

    return (libcfg >> (idx * step)) & DASICS_LIBCFG_MASK;
}

int32_t ATTR_SMAIN_TEXT dasics_jumpcfg_kalloc(uint64_t lo, uint64_t hi)
{
    uint64_t jumpcfg = csr_read(0x8c8);    // DasicsJumpCfg
    int32_t max_cfgs = DASICS_JUMPCFG_WIDTH;
    int32_t step = 16;

    int32_t idx;
    for (idx = 0; idx < max_cfgs; ++idx) {
        uint64_t curr_cfg = (jumpcfg >> (idx * step)) & DASICS_JUMPCFG_MASK;
        if ((curr_cfg & DASICS_JUMPCFG_V) == 0) // found available cfg
        {
            // Write DASICS jump boundary CSRs
            switch (idx) {
                case 0:
                    csr_write(0x8c0, lo);  // DasicsJumpBound0Lo
                    csr_write(0x8c1, hi);  // DasicsJumpBound0Hi
                    break;
                case 1:
                    csr_write(0x8c2, lo);  // DasicsJumpBound1Lo
                    csr_write(0x8c3, hi);  // DasicsJumpBound1Hi
                    break;
                case 2:
                    csr_write(0x8c4, lo);  // DasicsJumpBound2Lo
                    csr_write(0x8c5, hi);  // DasicsJumpBound2Hi
                    break;
                case 3:
                    csr_write(0x8c6, lo);  // DasicsJumpBound3Lo
                    csr_write(0x8c7, hi);  // DasicsJumpBound3Hi
                    break;
                default:
                    break;
            }

            jumpcfg &= ~(DASICS_JUMPCFG_MASK << (idx * step));
            jumpcfg |= DASICS_JUMPCFG_V << (idx * step);
            csr_write(0x8c8, jumpcfg); // DasicsJumpCfg

            return idx;
        }
    }

    return -1;
}
EXPORT_SYMBOL(dasics_jumpcfg_kalloc);

int32_t ATTR_SMAIN_TEXT dasics_jumpcfg_kfree(int32_t idx) {
    if (idx < 0 || idx >= DASICS_JUMPCFG_WIDTH) {
        return -1;
    }

    int32_t step = 16;
    uint64_t jumpcfg = csr_read(0x8c8);    // DasicsJumpCfg
    jumpcfg &= ~(DASICS_JUMPCFG_V << (idx * step));
    csr_write(0x8c8, jumpcfg); // DasicsJumpCfg
    return 0;
}
EXPORT_SYMBOL(dasics_jumpcfg_kfree);

uint32_t dasics_jumpcfg_get(int32_t idx) {
    if (idx < 0 || idx >= DASICS_JUMPCFG_WIDTH) {
        return -1;
    }

    int32_t step = 16;
    uint64_t jumpcfg = csr_read(0x8c8);    // DasicsJumpCfg

    return (jumpcfg >> (idx * step)) & DASICS_JUMPCFG_MASK;
}

void klib_call(void* func_name, ...) {
    register long a0 asm("a0") = (long)func_name;

    __asm__ __volatile__ (
        "addi sp, sp, -8\n\t"
        "sd ra, 0(sp)\n\t"
        ".word 0x0005108b\n\t"  // dasicscall.jr ra, a0
        "ld ra, 0(sp)\n\t"
        "addi sp, sp, 8\n\t"
        : "+r" (a0)  // a0 as input / output
        : 
        : "memory", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "t0", "t1", "t2", "t3", "t4", "t5", "t6"
    );
}

int ret_klib_call(void* func_name, ...) {
    uint64_t ret;
    __asm__ __volatile__ (
        "addi sp, sp, -8\n\t"
        "sd ra, 0(sp)\n\t"
        "mv t3, a0\n\t"
        "mv a0, a1\n\t"          // 第二个参数从a1移动到a0
        ".word 0x000e108b\n\t"   // dasicscall.jr ra, t3
        "ld ra, 0(sp)\n\t"
        "addi sp, sp, 8\n\t"
        : "=r" (ret)             // 结果直接存入ret（使用a0寄存器）
        : 
        : "memory", "a1", "a2", "a3", "t3"
    );
    return ret;
}
EXPORT_SYMBOL(ret_klib_call);

void register_kdasics(uint64_t funcptr) {
    // Set smaincall & sfault handler
    uint64_t smaincall_helper = (funcptr != 0) ? funcptr : (uint64_t) dasics_smaincall;
    csr_write(0x8b0, (uint64_t)smaincall_helper);
    //csr_write(0x005, (uint64_t)dasics_ufault_entry);
}
EXPORT_SYMBOL(register_kdasics);


void unregister_kdasics(void) {
    ;
}
EXPORT_SYMBOL(unregister_kdasics);
/*
void save_smaincall(void) {
    asm volatile (
        "addi sp, sp, -OFFSET_SMAINCALL\n"           // 调整栈指针
        "sd t0, OFFSET_SMAINCALL_T0(sp)\n"           // 保存 t0
        "sd t1, OFFSET_SMAINCALL_T1(sp)\n"           // 保存 t1
        "sd t3, OFFSET_SMAINCALL_T3(sp)\n"           // 保存 t3
        "sd ra, OFFSET_SMAINCALL_RA(sp)\n"           // 保存 ra
        "sd a0, OFFSET_SMAINCALL_A0(sp)\n"           // 保存 a0
        "sd a1, OFFSET_SMAINCALL_A1(sp)\n"           // 保存 a1
        "sd a2, OFFSET_SMAINCALL_A2(sp)\n"           // 保存 a2
        "sd a3, OFFSET_SMAINCALL_A3(sp)\n"           // 保存 a3
        "sd a4, OFFSET_SMAINCALL_A4(sp)\n"           // 保存 a4
        "sd a5, OFFSET_SMAINCALL_A5(sp)\n"           // 保存 a5
        "sd a6, OFFSET_SMAINCALL_A6(sp)\n"           // 保存 a6
        "sd a7, OFFSET_SMAINCALL_A7(sp)\n"           // 保存 a7
        "addi t0, sp, OFFSET_SMAINCALL\n"            // 计算原始 sp 值
        "sd t0, OFFSET_SMAINCALL_SP(sp\n)"             // 保存原始 sp
        : "sp", "t0", "memory"                // 破坏列表：修改的寄存器和内存
    );
}


void restore_smaincall(void) {
    asm volatile (
        "ld t0, OFFSET_SMAINCALL_T0(sp)\n"
        "ld t1, OFFSET_SMAINCALL_T1(sp)\n"
        "ld t3, OFFSET_SMAINCALL_T3(sp)\n"
        "ld ra, OFFSET_SMAINCALL_RA(sp)\n"
        "ld a0, OFFSET_SMAINCALL_A0(sp)\n"
        "ld a1, OFFSET_SMAINCALL_A1(sp)\n"
        "ld a2, OFFSET_SMAINCALL_A2(sp)\n"
        "ld a3, OFFSET_SMAINCALL_A3(sp)\n"
        "ld a4, OFFSET_SMAINCALL_A4(sp)\n"
        "ld a5, OFFSET_SMAINCALL_A5(sp)\n"
        "ld a6, OFFSET_SMAINCALL_A6(sp)\n"
        "ld a7, OFFSET_SMAINCALL_A7(sp)\n"
        "ld sp, OFFSET_SMAINCALL_SP(sp)\n"
        : "sp", "t0", "t1", "t3", "ra", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "memory"
    );
}
    */