#ifndef _ASM_DASICS_H_
#define _ASM_DASICS_H_

#include <linux/types.h>
#include <asm/kattr.h>
#include <asm/csr.h>

// dasics elf type
#define NO_DASICS 0
#define DASICS_STATIC 1
#define DASICS_DYNAMIC 2

// judge the dasics option
#define DASICS_COMMAND "-dasics"
#define DASICS_LENGTH 8

// dasics dynamic elf base
#define DASICS_LINKER_BASE 0x1000 
#define COPY_LINKER_BASE 0x30000
#define DASICS_VDSO_BASE 0x60000
#define TRUST_LIB_BASE 0x800000
// TODO: Add Smaincall types
typedef enum {
    Smaincall_PRINT = 1
} SmaincallTypes;

struct dasics_hw_state {
	unsigned long libcfg;
	unsigned long lib_lo[DASICS_MAX_DATA_BOUNDS];
	unsigned long lib_hi[DASICS_MAX_DATA_BOUNDS];
	unsigned long jumpcfg;
	unsigned long jump_lo[DASICS_MAX_JUMP_BOUNDS];
	unsigned long jump_hi[DASICS_MAX_JUMP_BOUNDS];
	unsigned long dmaincall;
	unsigned long dretpc;
	unsigned long dretpcactz;
};

#define DASICS_CALL_MAX_ARGS 8

/*
 * Fixed register frame for DASICS calls. Arguments and results are raw XLEN
 * values. Stack arguments, floating-point/vector values, and aggregates must
 * be lowered or rejected by typed wrappers.
 */
struct dasics_call_regs {
	unsigned long target;
	unsigned long a0;
	unsigned long a1;
	unsigned long a2;
	unsigned long a3;
	unsigned long a4;
	unsigned long a5;
	unsigned long a6;
	unsigned long a7;
	unsigned long ret_a0;
	unsigned long ret_a1;
};

int dasics_hw_save(struct dasics_hw_state *state);
int dasics_hw_restore(const struct dasics_hw_state *state);
void dasics_hw_clear_call_authority(void);
long dasics_hw_call(struct dasics_call_regs *regs);

/* smaincall used */
#define OFFSET_SMAINCALL_T0     (8*0)
#define OFFSET_SMAINCALL_T1     (8*1)
#define OFFSET_SMAINCALL_T3     (8*2)
#define OFFSET_SMAINCALL_RA     (8*3)
#define OFFSET_SMAINCALL_A0     (8*4)
#define OFFSET_SMAINCALL_A1     (8*5)
#define OFFSET_SMAINCALL_A2     (8*6)
#define OFFSET_SMAINCALL_A3     (8*7)
#define OFFSET_SMAINCALL_A4     (8*8)
#define OFFSET_SMAINCALL_A5     (8*9)
#define OFFSET_SMAINCALL_A6     (8*10)
#define OFFSET_SMAINCALL_A7     (8*11)
#define OFFSET_SMAINCALL_SP     (8*12)

#define OFFSET_SMAINCALL        (8*13)

void     dasics_init_umain_bound(uint64_t cfg, uint64_t hi, uint64_t lo);
void     dasics_init_smaincall(uint64_t entry);
uint64_t dasics_smaincall(SmaincallTypes type, uint64_t arg0, uint64_t arg1);
int32_t  dasics_libcfg_kalloc(uint64_t cfg, uint64_t hi, uint64_t lo);
int32_t  dasics_libcfg_kfree(int32_t idx);
uint32_t dasics_libcfg_kget(int32_t idx);
int32_t  dasics_jumpcfg_kalloc(uint64_t lo, uint64_t hi);
int32_t  dasics_jumpcfg_kfree(int32_t idx);
uint32_t dasics_jumpcfg_kget(int32_t idx);
void klib_call(void* func_name, ...);
int ret_klib_call(void* func_name, ...);
void register_kdasics(uint64_t funcptr);
void unregister_kdasics(void);
//void save_smaincall(void);
//void restore_smaincall(void);

#endif
