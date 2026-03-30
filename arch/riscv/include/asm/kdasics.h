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
    Smaincall_UNKNOWN
} SmaincallTypes;

void     dasics_init_umain_bound(uint64_t cfg, uint64_t hi, uint64_t lo);
void     dasics_init_smaincall(uint64_t entry);
uint64_t dasics_smaincall(SmaincallTypes type, uint64_t arg0, uint64_t arg1, uint64_t arg2);
int32_t  dasics_libcfg_kalloc(uint64_t cfg, uint64_t hi, uint64_t lo);
int32_t  dasics_libcfg_kfree(int32_t idx);
uint32_t dasics_libcfg_kget(int32_t idx);
int32_t  dasics_jumpcfg_kalloc(uint64_t lo, uint64_t hi);
int32_t  dasics_jumpcfg_kfree(int32_t idx);
uint32_t dasics_jumpcfg_kget(int32_t idx);

#endif