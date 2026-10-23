/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _ASM_RISCV_DASICS_VERIFY_H
#define _ASM_RISCV_DASICS_VERIFY_H

#include <linux/types.h>
#include <uapi/linux/fdi_verify.h>

struct mm_struct;
struct task_struct;
struct pt_regs;
struct riscv_dasics_revoke_work;

#ifdef CONFIG_RISCV_DASICS_VALIDATION
/* This state belongs to one task and one exec image. Fork/exec cannot inherit
 * a live observation window or temporary CBO permissions. */
struct riscv_dasics_verify_state {
	u64 cookie;
	struct mm_struct *mm;
	unsigned long saved_envcfg;
	u64 flags;
	struct fdi_verify_result record;
};

int riscv_dasics_verify_begin(u64 cookie, u64 flags);
int riscv_dasics_verify_arm(u64 cookie, u64 kind, unsigned long site);
int riscv_dasics_verify_query(u64 cookie, struct fdi_verify_result *result);
int riscv_dasics_verify_end(u64 cookie, struct fdi_verify_result *result);
struct riscv_dasics_revoke_work *riscv_dasics_verify_alloc_revoke(struct task_struct *owner);
void riscv_dasics_verify_free_revoke(struct riscv_dasics_revoke_work *work);
int riscv_dasics_verify_revoke(struct task_struct *task, u64 cookie,
			     struct riscv_dasics_revoke_work **reserved);
void riscv_dasics_verify_reset_task(struct task_struct *task);
void riscv_dasics_verify_illegal(const struct pt_regs *regs);
#else
static inline void riscv_dasics_verify_reset_task(struct task_struct *task) { }
static inline void riscv_dasics_verify_illegal(const struct pt_regs *regs) { }
#endif

#endif
