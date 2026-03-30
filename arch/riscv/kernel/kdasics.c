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

uint64_t dasics_smaincall(SmaincallTypes type, uint64_t arg0, uint64_t arg1, uint64_t arg2)
{
    uint64_t dasics_return_pc = csr_read(0x8b1);
    //uint64_t dasics_free_zone_return_pc = csr_read(0x8b2);

    uint64_t retval = 0;

    // TODO: change to linux version
    switch (type)
    {
        default:
            pr_cont("Warning: Invalid smaincall number %u!\n", type);
            break;
    }

    csr_write(0x8b1, dasics_return_pc);
    //csr_write(0x8a5, dasics_free_zone_return_pc);

    // TODO: Use compiler to optimize such ugly code in the future ...
    // Compile Error: gcc doesn't recognize pulpret. Use machine code instead.
    asm volatile("mv        a0, a5\n"\
                 "ld        ra, 88(sp)\n"\
                 "ld        s0, 80(sp)\n"\
                 "addi      sp, sp, 96\n"\
                 "ret\n"\
                 //".word 0x0000f00b \n" /* dasicsret x0,  0, x1 in little endian */ 
                 "nop");

    return retval;
}

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
            libcfg0 |= (cfg & DASICS_LIBCFG_MASK) << (idx * step);
            csr_write(0x880, libcfg0);  // DasicsLibCfg0
            return idx;
        }
    }

    return -1;
}

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

uint32_t dasics_jumpcfg_get(int32_t idx) {
    if (idx < 0 || idx >= DASICS_JUMPCFG_WIDTH) {
        return -1;
    }

    int32_t step = 16;
    uint64_t jumpcfg = csr_read(0x8c8);    // DasicsJumpCfg

    return (jumpcfg >> (idx * step)) & DASICS_JUMPCFG_MASK;
}