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
    csr_write(0x5c0, cfg);  // DasicsUMainCfg
    csr_write(0x5c1, hi);   // DasicsUMainBoundHi
    csr_write(0x5c2, lo);   // DasicsUMainBoundLo
}

void dasics_init_smaincall(uint64_t entry)
{
    csr_write(0x8a3, entry);  // DasicsMaincallEntry
}

uint64_t dasics_smaincall(SmaincallTypes type, uint64_t arg0, uint64_t arg1, uint64_t arg2)
{
    uint64_t dasics_return_pc = csr_read(0x8a4);
    uint64_t dasics_free_zone_return_pc = csr_read(0x8a5);

    uint64_t retval = 0;

    // TODO: change to linux version
    switch (type)
    {
        default:
            pr_cont("Warning: Invalid smaincall number %u!\n", type);
            break;
    }

    csr_write(0x8a4, dasics_return_pc);
    csr_write(0x8a5, dasics_free_zone_return_pc);

    // TODO: Use compiler to optimize such ugly code in the future ...
    // Compile Error: gcc doesn't recognize pulpret. Use machine code instead.
    asm volatile("mv        a0, a5\n"\
                 "ld        ra, 88(sp)\n"\
                 "ld        s0, 80(sp)\n"\
                 "addi      sp, sp, 96\n"\
                 ".word 0x0000f00b\n"    /* in little endian, inst pulpret */
		 ); 

    return retval;
}

int32_t dasics_libcfg_kalloc(uint64_t cfg, uint64_t hi, uint64_t lo)
{
    uint64_t libcfg0 = csr_read(0x881);  // DasicsLibCfg0
    uint64_t libcfg1 = csr_read(0x882);  // DasicsLibCfg1
    uint64_t curr_cfg;

    int32_t max_cfgs = DASICS_LIBCFG_WIDTH << 1;

    int32_t idx, _idx;
    int choose_libcfg0;

    for (idx = 0; idx < max_cfgs; ++idx)
    {
        choose_libcfg0 = (idx < DASICS_LIBCFG_WIDTH);
        _idx = choose_libcfg0 ? idx : idx - DASICS_LIBCFG_WIDTH;
        curr_cfg = ((choose_libcfg0 ? libcfg0 : libcfg1) >> (_idx * STEP)) & DASICS_LIBCFG_MASK;

        if ((curr_cfg & DASICS_LIBCFG_V) == 0)  // Find avaliable cfg
        {
            if (choose_libcfg0)
            {
                libcfg0 &= ~(DASICS_LIBCFG_MASK << (_idx * STEP));
                libcfg0 |= (cfg & DASICS_LIBCFG_MASK) << (_idx * STEP);
                csr_write(0x881, libcfg0);  // DasicsLibCfg0
            }
            else
            {
                libcfg1 &= ~(DASICS_LIBCFG_MASK << (_idx * STEP));
                libcfg1 |= (cfg & DASICS_LIBCFG_MASK) << (_idx * STEP);
                csr_write(0x882, libcfg1);  // DasicsLibCfg1
            }

            // Write DASICS boundary csrs            
            switch (idx)
            {
                case 0:
                    csr_write(0x883, hi);  // DasicsLibBound0
                    csr_write(0x884, lo);  // DasicsLibBound1
                    break;
                case 1:
                    csr_write(0x885, hi);  // DasicsLibBound2
                    csr_write(0x886, lo);  // DasicsLibBound3
                    break;
                case 2:
                    csr_write(0x887, hi);  // DasicsLibBound4
                    csr_write(0x888, lo);  // DasicsLibBound5
                    break;
                case 3:
                    csr_write(0x889, hi);  // DasicsLibBound6
                    csr_write(0x88a, lo);  // DasicsLibBound7
                    break;
                case 4:
                    csr_write(0x88b, hi);  // DasicsLibBound8
                    csr_write(0x88c, lo);  // DasicsLibBound9
                    break;
                case 5:
                    csr_write(0x88d, hi);  // DasicsLibBound10
                    csr_write(0x88e, lo);  // DasicsLibBound11
                    break;
                case 6:
                    csr_write(0x88f, hi);  // DasicsLibBound12
                    csr_write(0x890, lo);  // DasicsLibBound13
                    break;
                case 7:
                    csr_write(0x891, hi);  // DasicsLibBound14
                    csr_write(0x892, lo);  // DasicsLibBound15
                    break;
                case 8:
                    csr_write(0x893, hi);  // DasicsLibBound16
                    csr_write(0x894, lo);  // DasicsLibBound17
                    break;
                case 9:
                    csr_write(0x895, hi);  // DasicsLibBound18
                    csr_write(0x896, lo);  // DasicsLibBound19
                    break;
                case 10:
                    csr_write(0x897, hi);  // DasicsLibBound20
                    csr_write(0x898, lo);  // DasicsLibBound21
                    break;
                case 11:
                    csr_write(0x899, hi);  // DasicsLibBound22
                    csr_write(0x89a, lo);  // DasicsLibBound23
                    break;
                case 12:
                    csr_write(0x89b, hi);  // DasicsLibBound24
                    csr_write(0x89c, lo);  // DasicsLibBound25
                    break;
                case 13:
                    csr_write(0x89d, hi);  // DasicsLibBound26
                    csr_write(0x89e, lo);  // DasicsLibBound27
                    break;
                case 14:
                    csr_write(0x89f, hi);  // DasicsLibBound28
                    csr_write(0x8a0, lo);  // DasicsLibBound29
                    break;
                default:
                    csr_write(0x8a1, hi);  // DasicsLibBound30
                    csr_write(0x8a2, lo);  // DasicsLibBound31
                    break;
            }

            return idx;
        }
    }

    return -1;
}

int32_t dasics_libcfg_kfree(int32_t idx)
{
    int choose_libcfg0;
    int32_t _idx;
    uint64_t libcfg;

    if (idx < 0 || idx >= (DASICS_LIBCFG_WIDTH << 1))
    {
        return -1;
    }

    choose_libcfg0 = (idx < DASICS_LIBCFG_WIDTH);
    _idx = choose_libcfg0 ? idx : idx - DASICS_LIBCFG_WIDTH;

    libcfg = choose_libcfg0 ? csr_read(0x881):  // DasicsLibCfg0
                                       csr_read(0x882);  // DasicsLibCfg1
    libcfg &= ~(DASICS_LIBCFG_V << (_idx * STEP));

    if (choose_libcfg0)
    {
        csr_write(0x881, libcfg);  // DasicsLibCfg0
    }
    else
    {
        csr_write(0x882, libcfg);  // DasicsLibCfg1
    }

    return 0;
}

uint32_t dasics_libcfg_kget(int32_t idx)
{
    int choose_libcfg0;
    int32_t _idx;
    uint64_t libcfg;

    if (idx < 0 || idx >= (DASICS_LIBCFG_WIDTH << 1))
    {
        return -1;
    }

    choose_libcfg0 = (idx < DASICS_LIBCFG_WIDTH);
    _idx = choose_libcfg0 ? idx : idx - DASICS_LIBCFG_WIDTH;

    libcfg = choose_libcfg0 ? csr_read(0x881):  // DasicsLibCfg0
                                       csr_read(0x882);  // DasicsLibCfg1

    return (libcfg >> (_idx * STEP)) & DASICS_LIBCFG_MASK;
}
