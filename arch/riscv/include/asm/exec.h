/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ASM_EXEC_H
#define __ASM_EXEC_H

#include <linux/types.h>

extern unsigned long arch_align_stack(unsigned long sp);

#ifdef CONFIG_RISCV_DASICS
struct riscv_dasics_elf_sections {
	unsigned long text_lo;
	unsigned long text_hi;
	unsigned long ulib_text_lo;
	unsigned long ulib_text_hi;
	unsigned long freezone_lo;
	unsigned long freezone_hi;
	bool have_text;
	bool have_ulib_text;
	bool have_freezone;
};

struct riscv_dasics_exec_state {
	struct riscv_dasics_elf_sections sections;
	bool requested;
	bool validated;
};

struct linux_binprm;

int riscv_dasics_prepare_exec(struct linux_binprm *bprm,
			      const char __user *last_arg);
#define arch_bprm_prepare_exec riscv_dasics_prepare_exec
#endif

#endif	/* __ASM_EXEC_H */
