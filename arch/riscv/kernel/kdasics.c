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
    csr_write(0x9e1, cal_dasics_bound_val(lo,hi,cfg));  // DasicsUMainBound
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

int32_t dasics_membound_kalloc(uint64_t cfg, uint64_t lo, uint64_t hi) {
    int32_t max_cfgs = DASICS_MEMCFG_WIDTH;
    int32_t idx;
    for (idx = 0; idx < max_cfgs; ++idx) {
        uint64_t tmp_bound;
        MEM_BOUND_LOOKUP(tmp_bound,idx,RD);
        uint64_t curr_cfg = get_dasics_bound_cfg(tmp_bound);

        if ((curr_cfg & DASICS_MEMCFG_V) == 0)  // Found available config
        {
            // Write DASICS bounds csr
            tmp_bound = cal_dasics_bound_val(lo,hi,((cfg & DASICS_MEMCFG_MASK) | DASICS_MEMCFG_V));
            MEM_BOUND_LOOKUP(tmp_bound,idx,WR);
            return idx;
        }
    }

    return -1;
}

uint64_t dasics_membound_kget(int32_t idx) {
    uint64_t val;
    if (idx < 0 || idx >= DASICS_MEMCFG_WIDTH) return -1;
    MEM_BOUND_LOOKUP(val,idx,RD);
    return val;
}

int32_t dasics_membound_kset(int32_t idx, uint64_t val) {
    if (idx < 0 || idx >= DASICS_MEMCFG_WIDTH) return -1;
    MEM_BOUND_LOOKUP(val,idx,WR);
    return 0;
}

int32_t dasics_jmpbound_kalloc(uint64_t lo, uint64_t hi) {
    int32_t max_cfgs = DASICS_JMPCFG_WIDTH;
    int32_t idx;
    for (idx = 0; idx < max_cfgs; ++idx) {
        uint64_t tmp_bound;
        JMP_BOUND_LOOKUP(tmp_bound,idx,RD);
        uint64_t curr_cfg = get_dasics_bound_cfg(tmp_bound);
        if ((curr_cfg & DASICS_JMPCFG_V) == 0) // found available cfg
        {
            // Write DASICS bounds csr
            tmp_bound = cal_dasics_bound_val(lo,hi,DASICS_JMPCFG_V);
            MEM_BOUND_LOOKUP(tmp_bound,idx,WR);
            return idx;
        }
    }

    return -1;
}

uint64_t dasics_jmpbound_kget(int32_t idx) {
    uint64_t val;
    if (idx < 0 || idx >= DASICS_JMPCFG_WIDTH) return -1;
    JMP_BOUND_LOOKUP(val,idx,RD);
    return val;
}

int32_t dasics_jmpbound_kset(int32_t idx, uint64_t val) {
    if (idx < 0 || idx >= DASICS_JMPCFG_WIDTH) return -1;
    JMP_BOUND_LOOKUP(val,idx,WR);
    return 0;
}
