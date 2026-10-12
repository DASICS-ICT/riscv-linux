/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _ASM_RISCV_KDASICS_H
#define _ASM_RISCV_KDASICS_H

#include <asm/csr.h>

#define CSR_DUMCFG 0x9e1
#define CSR_DUMBOUNDLO 0x9e2
#define CSR_DUMBOUNDHI 0x9e3

#define CSR_DLCFG0          0x880

#define CSR_DLBOUND0LO      0x890
#define CSR_DLBOUND0HI      0x891
#define CSR_DLBOUND1LO      0x892
#define CSR_DLBOUND1HI      0x893
#define CSR_DLBOUND2LO      0x894
#define CSR_DLBOUND2HI      0x895
#define CSR_DLBOUND3LO      0x896
#define CSR_DLBOUND3HI      0x897
#define CSR_DLBOUND4LO      0x898
#define CSR_DLBOUND4HI      0x899
#define CSR_DLBOUND5LO      0x89a
#define CSR_DLBOUND5HI      0x89b
#define CSR_DLBOUND6LO      0x89c
#define CSR_DLBOUND6HI      0x89d
#define CSR_DLBOUND7LO      0x89e
#define CSR_DLBOUND7HI      0x89f
#define CSR_DLBOUND8LO      0x8a0
#define CSR_DLBOUND8HI      0x8a1
#define CSR_DLBOUND9LO      0x8a2
#define CSR_DLBOUND9HI      0x8a3
#define CSR_DLBOUND10LO     0x8a4
#define CSR_DLBOUND10HI     0x8a5
#define CSR_DLBOUND11LO     0x8a6
#define CSR_DLBOUND11HI     0x8a7
#define CSR_DLBOUND12LO     0x8a8
#define CSR_DLBOUND12HI     0x8a9
#define CSR_DLBOUND13LO     0x8aa
#define CSR_DLBOUND13HI     0x8ab
#define CSR_DLBOUND14LO     0x8ac
#define CSR_DLBOUND14HI     0x8ad
#define CSR_DLBOUND15LO     0x8ae
#define CSR_DLBOUND15HI     0x8af

#define CSR_DMAINCALL       0x8b0
#define CSR_DRETPC          0x8b1
#define CSR_DRETPCACTZ      0x8b2
#define CSR_DFREASON        0x8b3

/* Compatibility names used by the existing exception entry code. */
#define CSR_DRETURNPC       CSR_DRETPC
#define CSR_DFZRETURN       CSR_DRETPCACTZ

#define CSR_DJBOUND0LO      0x8c0
#define CSR_DJBOUND0HI      0x8c1
#define CSR_DJBOUND1LO      0x8c2
#define CSR_DJBOUND1HI      0x8c3
#define CSR_DJBOUND2LO      0x8c4
#define CSR_DJBOUND2HI      0x8c5
#define CSR_DJBOUND3LO      0x8c6
#define CSR_DJBOUND3HI      0x8c7
#define CSR_DJCFG           0x8c8

/* DASICS bound counts and packed configuration fields. */
#define DASICS_MAX_DATA_BOUNDS 16
#define DASICS_LIBCFG_BITS  4
#define DASICS_LIBCFG_WIDTH DASICS_MAX_DATA_BOUNDS
#define DASICS_LIBCFG_MASK  0xfUL
#define DASICS_LIBCFG_V     0x8UL
#define DASICS_LIBCFG_R     0x2UL
#define DASICS_LIBCFG_W     0x1UL

#define DASICS_MAX_JUMP_BOUNDS 4
#define DASICS_JUMPCFG_BITS 16
#define DASICS_JUMPCFG_WIDTH DASICS_MAX_JUMP_BOUNDS
#define DASICS_JUMPCFG_MASK 0xffffUL
#define DASICS_JUMPCFG_V    0x1UL


#ifndef __ASSEMBLY__
#include <linux/types.h>
#include <asm/dasics.h>

struct pt_regs;

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

#define DASICS_MAINCALL_ABI_VERSION 1
#define DASICS_MAINCALL_MAX_ARGS 7
#define DASICS_MAINCALL_SERVICE_DEBUG_NESTED 0x44424701
#define DASICS_MAINCALL_SERVICE_DEBUG_SLEEP 0x44424702
#define DASICS_MAINCALL_SLEEP_TEST_VALUE 0x534c454550UL

enum dasics_maincall_service_id {
	DASICS_MAINCALL_SERVICE_ABI_INFO = 1,
};

enum dasics_maincall_abi_query {
	DASICS_MAINCALL_QUERY_RUNTIME = 1,
	DASICS_MAINCALL_QUERY_REGISTERS,
};

#define DASICS_MAINCALL_REGISTER_TEST_VALUE 0x12345678UL

/* Register request captured by the trusted maincall assembly entry. */
struct dasics_maincall_request {
	unsigned long service_id;
	unsigned long args[DASICS_MAINCALL_MAX_ARGS];
	unsigned long return_pc;
	unsigned long untrusted_sp;
	unsigned long irq_status;
	long status;
	unsigned long value;
};

#define DASICS_RECOVERY_CONTEXT_MAGIC 0x44524358UL

/* Trusted continuation state used to escape a faulting untrusted call. */
struct dasics_recovery_context {
	unsigned long magic;
	unsigned long regs;
	long error;
	unsigned long scratch;
	unsigned long gp;
	unsigned long tp;
	unsigned long sp;
	unsigned long ra;
	unsigned long s[12];
};

int dasics_hw_save(struct dasics_hw_state *state);
int dasics_hw_restore(const struct dasics_hw_state *state);
void dasics_hw_clear_call_authority(void);
long dasics_hw_call(struct dasics_call_regs *regs,
		    struct dasics_recovery_context *recovery);
int dasics_hw_redirect_recovery(struct pt_regs *regs,
				struct dasics_recovery_context *recovery,
				const struct dasics_hw_state *parent);
int dasics_hw_install_call_authority(const struct dasics_hw_state *state,
				     unsigned int fail_bound);
int dasics_hw_replace_data_bound(unsigned int slot, unsigned long lo,
				 unsigned long hi, unsigned long cfg);
void dasics_maincall_gate(void);

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

#endif /* !__ASSEMBLY__ */


#endif
