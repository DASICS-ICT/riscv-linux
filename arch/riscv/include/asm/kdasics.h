#ifndef _ASM_DASICS_H_
#define _ASM_DASICS_H_

#include <linux/types.h>
#include <asm/kattr.h>
#include <asm/csr.h>

// TODO: Add Smaincall types

#define CONCAT(TYPE,OP) TYPE##_BOUND_REG_##OP

#define MEM_BOUND_REG_RD(bound,idx)   \
        case idx:  \
            bound = csr_read(0x890 + idx);  \
            break;

#define MEM_BOUND_REG_WR(bound,idx)   \
        case idx:  \
            csr_write(0x890 + idx, bound);  \
            break;

#define MEM_BOUND_LOOKUP(BOUND,IDX,OP) \
        switch (IDX) \
        {               \
            CONCAT(MEM,OP)(BOUND,0);  \
            CONCAT(MEM,OP)(BOUND,1);  \
            CONCAT(MEM,OP)(BOUND,2);  \
            CONCAT(MEM,OP)(BOUND,3);  \
            CONCAT(MEM,OP)(BOUND,4);  \
            CONCAT(MEM,OP)(BOUND,5);  \
            CONCAT(MEM,OP)(BOUND,6);  \
            CONCAT(MEM,OP)(BOUND,7);  \
            CONCAT(MEM,OP)(BOUND,8);  \
            CONCAT(MEM,OP)(BOUND,9);  \
            CONCAT(MEM,OP)(BOUND,10); \
            CONCAT(MEM,OP)(BOUND,11); \
            CONCAT(MEM,OP)(BOUND,12); \
            CONCAT(MEM,OP)(BOUND,13); \
            CONCAT(MEM,OP)(BOUND,14); \
            CONCAT(MEM,OP)(BOUND,15); \
            CONCAT(MEM,OP)(BOUND,16); \
            CONCAT(MEM,OP)(BOUND,17); \
            CONCAT(MEM,OP)(BOUND,18); \
            CONCAT(MEM,OP)(BOUND,19); \
            CONCAT(MEM,OP)(BOUND,20); \
            CONCAT(MEM,OP)(BOUND,21); \
            CONCAT(MEM,OP)(BOUND,22); \
            CONCAT(MEM,OP)(BOUND,23); \
            CONCAT(MEM,OP)(BOUND,24); \
            CONCAT(MEM,OP)(BOUND,25); \
            CONCAT(MEM,OP)(BOUND,26); \
            CONCAT(MEM,OP)(BOUND,27); \
            CONCAT(MEM,OP)(BOUND,28); \
            CONCAT(MEM,OP)(BOUND,29); \
            CONCAT(MEM,OP)(BOUND,30); \
            CONCAT(MEM,OP)(BOUND,31); \
            default: \
                printk("\x1b[31m%s\x1b[0m","[DASICS]Error: out of membound register range\n"); \
        }

#define JMP_BOUND_REG_RD(bound,idx)   \
        case idx:  \
            bound = csr_read(0x8c0 + idx);  \
            break;

#define JMP_BOUND_REG_WR(bound,idx)   \
        case idx:  \
            csr_write(0x8c0 + idx, bound);  \
            break;

#define JMP_BOUND_LOOKUP(BOUND,IDX,OP) \
        switch (IDX) \
        {               \
            CONCAT(JMP,OP)(BOUND,0);  \
            CONCAT(JMP,OP)(BOUND,1);  \
            CONCAT(JMP,OP)(BOUND,2);  \
            CONCAT(JMP,OP)(BOUND,3);  \
            CONCAT(JMP,OP)(BOUND,4);  \
            CONCAT(JMP,OP)(BOUND,5);  \
            CONCAT(JMP,OP)(BOUND,6);  \
            CONCAT(JMP,OP)(BOUND,7);  \
            default: \
                printk("\x1b[31m%s\x1b[0m","[DASICS]Error: out of jmpbound register range\n"); \
        }


#define cal_dasics_bound_val(lo, hi, cfg)           \
     (((uint64_t)    lo  & ((1UL<<39)-1)) |         \
     (((uint64_t)(hi-lo) & ((1UL<<21)-1)) <<  39) | \
     (((uint64_t)   cfg  & ((1UL<< 4)-1)) <<  60))

#define get_dasics_bound_lo(bound)           \
     ((uint64_t)bound & ((1UL<<39)-1))

#define get_dasics_bound_hi(bound)           \
     (get_dasics_bound_lo(bound) + (((uint64_t)bound >> 39) & ((1UL<<21)-1)))

#define get_dasics_bound_cfg(bound)          \
     ((uint64_t)bound >> 60)


typedef enum {
    Smaincall_UNKNOWN
} SmaincallTypes;

void     dasics_init_umain_bound(uint64_t cfg, uint64_t hi, uint64_t lo);
void     dasics_init_smaincall(uint64_t entry);
uint64_t dasics_smaincall(SmaincallTypes type, uint64_t arg0, uint64_t arg1, uint64_t arg2);

int32_t  dasics_membound_kalloc(uint64_t cfg, uint64_t lo, uint64_t hi);
uint64_t dasics_membound_kget(int32_t idx);
int32_t  dasics_membound_kset(int32_t idx, uint64_t val);

int32_t  dasics_jmpbound_kalloc(uint64_t lo, uint64_t hi);
uint64_t dasics_jmpbound_kget(int32_t idx);
int32_t  dasics_jmpbound_kset(int32_t idx, uint64_t val);

#define dasics_membound_kfree(idx) dasics_membound_kset(idx,0)
#define dasics_jmpbound_kfree(idx) dasics_jmpbound_kset(idx,0)

#endif