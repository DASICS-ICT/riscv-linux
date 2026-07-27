/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _ASM_RISCV_DASICS_H
#define _ASM_RISCV_DASICS_H

#include <linux/types.h>

#define RISCV_DASICS_LIB_BOUND_COUNT	16
#define RISCV_DASICS_JUMP_BOUND_COUNT	4

#define RISCV_DASICS_UMAIN_UENA		0x2UL
#define RISCV_DASICS_UMAIN_CUET		0x4UL

#define RISCV_DASICS_LIBCFG_W		0x1UL
#define RISCV_DASICS_LIBCFG_R		0x2UL
#define RISCV_DASICS_LIBCFG_V		0x8UL

struct riscv_dasics_hw_state {
	unsigned long lib_cfg;
	unsigned long lib_bound_lo[RISCV_DASICS_LIB_BOUND_COUNT];
	unsigned long lib_bound_hi[RISCV_DASICS_LIB_BOUND_COUNT];
	unsigned long maincall;
	unsigned long return_pc;
	unsigned long active_return;
	unsigned long freason;
	unsigned long jump_bound_lo[RISCV_DASICS_JUMP_BOUND_COUNT];
	unsigned long jump_bound_hi[RISCV_DASICS_JUMP_BOUND_COUNT];
	unsigned long jump_cfg;
	unsigned long umain_cfg;
	unsigned long umain_bound_lo;
	unsigned long umain_bound_hi;
};

struct riscv_dasics_metadata {
	bool enabled;
	bool ecall_close;
	bool complete_app;
	bool maincfg_toggle;
	unsigned long text_lo;
	unsigned long text_hi;
	unsigned long ulib_text_lo;
	unsigned long ulib_text_hi;
	unsigned long freezone_lo;
	unsigned long freezone_hi;
	unsigned long start_data;
};

struct riscv_dasics_complete_control {
	unsigned long pid;
	unsigned long saved_cfg;
	unsigned long next_stage;
	bool active;
};

struct riscv_dasics_maincfg_actual {
	unsigned long cfg;
	unsigned long cause;
	unsigned long epc;
	unsigned long tval;
	unsigned long freason;
	unsigned long event;
	bool seen;
};

struct riscv_dasics_maincfg_control {
	unsigned long pid;
	unsigned long next_step;
	unsigned long source[4];
	unsigned long operand[4];
	unsigned long recovery[4];
	unsigned long armed_step;
	struct riscv_dasics_maincfg_actual actual;
	unsigned long failures;
	bool active;
	bool armed;
};

struct riscv_dasics_state {
	struct riscv_dasics_hw_state hw;
	struct riscv_dasics_metadata metadata;
	struct riscv_dasics_complete_control complete_control;
	struct riscv_dasics_maincfg_control maincfg_control;
};

struct linux_binprm;
struct pt_regs;
struct task_struct;

#ifdef CONFIG_RISCV_DASICS
void riscv_dasics_clear_task(struct task_struct *task);
int riscv_dasics_setup_elf(struct linux_binprm *bprm,
			   const void *elf_header, unsigned long load_bias,
			   unsigned long start_data);
bool riscv_dasics_handle_fault(struct pt_regs *regs);
bool riscv_dasics_handle_illegal(struct pt_regs *regs);
bool riscv_dasics_handle_syscall(struct pt_regs *regs, long syscall);
void riscv_dasics_prepare_copy(struct task_struct *task);
void riscv_dasics_start_thread(struct task_struct *task);
void riscv_dasics_switch(struct task_struct *prev, struct task_struct *next);
#else
static inline void riscv_dasics_clear_task(struct task_struct *task) { }
static inline bool riscv_dasics_handle_fault(struct pt_regs *regs)
{
	return false;
}
static inline bool riscv_dasics_handle_illegal(struct pt_regs *regs)
{
	return false;
}
static inline bool riscv_dasics_handle_syscall(struct pt_regs *regs,
					       long syscall)
{
	return false;
}
static inline void riscv_dasics_prepare_copy(struct task_struct *task) { }
static inline void riscv_dasics_start_thread(struct task_struct *task) { }
static inline void riscv_dasics_switch(struct task_struct *prev,
				       struct task_struct *next) { }
#endif

#endif /* _ASM_RISCV_DASICS_H */
