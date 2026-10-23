// SPDX-License-Identifier: GPL-2.0-only
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/mm.h>
#include <linux/preempt.h>
#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/slab.h>
#include <linux/task_work.h>
#include <linux/uaccess.h>
#include <asm/cacheflush.h>
#include <asm/cpufeature.h>
#include <asm/csr.h>
#include <asm/dasics.h>
#include <asm/ptrace.h>
#include <asm/sbi.h>
#include <asm/switch_to.h>

static bool verify_matches(u64 cookie)
{
	const struct riscv_dasics_verify_state *state = &current->thread.dasics.verify;

	return cookie && state->cookie == cookie && state->mm == current->mm;
}

static int verify_menvcfg(unsigned long *value)
{
	struct sbiret ret = sbi_ecall(0xda51c5, 2, 0, 0, 0, 0, 0, 0);

	if (ret.error)
		return -EOPNOTSUPP;
	*value = ret.value;
	return 0;
}

/* Only the owned CBO bits are restored. This also operates on a copied child
 * before it can run; another task's live hardware CSR is never written. */
void riscv_dasics_verify_reset_task(struct task_struct *task)
{
	struct riscv_dasics_verify_state *state = &task->thread.dasics.verify;

	if (state->cookie && (state->flags & FDI_VERIFY_CBO)) {
		if (task == current)
			preempt_disable();
		envcfg_update_bits(task, FDI_VERIFY_ENVCFG_MASK, state->saved_envcfg);
		if (task == current) {
			if ((csr_read(CSR_ENVCFG) & FDI_VERIFY_ENVCFG_MASK) != state->saved_envcfg)
				pr_err("FDI_VERIFY_RESTORE result=FAIL pid=%d\n", task_pid_vnr(task));
			preempt_enable();
		}
	}
	memset(state, 0, sizeof(*state));
}

int riscv_dasics_verify_begin(u64 cookie, u64 flags)
{
	struct riscv_dasics_verify_state *state = &current->thread.dasics.verify;
	unsigned long machine;
	int error;

	if (!cookie || !current->mm || flags & ~FDI_VERIFY_CBO)
		return -EINVAL;
	if (state->cookie)
		return -EBUSY;
	if (!riscv_has_extension_unlikely(RISCV_ISA_EXT_XLINUXENVCFG))
		return -EOPNOTSUPP;
	if (flags & FDI_VERIFY_CBO) {
		if (!riscv_has_extension_unlikely(RISCV_ISA_EXT_ZICBOM) ||
		    !riscv_has_extension_unlikely(RISCV_ISA_EXT_ZICBOZ) ||
		    riscv_cbom_block_size != 64 || riscv_cboz_block_size != 64)
			return -EOPNOTSUPP;
		error = verify_menvcfg(&machine);
		if (error || (machine & FDI_VERIFY_ENVCFG_MASK) != FDI_VERIFY_ENVCFG_MASK)
			return error ?: -EOPNOTSUPP;
	}
	memset(state, 0, sizeof(*state));
	state->cookie = cookie;
	state->mm = current->mm;
	state->flags = flags;
	state->saved_envcfg = current->thread.envcfg & FDI_VERIFY_ENVCFG_MASK;
	if (flags & FDI_VERIFY_CBO) {
		preempt_disable();
		envcfg_update_bits(current, FDI_VERIFY_ENVCFG_MASK, FDI_VERIFY_ENVCFG_MASK);
		error = (csr_read(CSR_ENVCFG) & FDI_VERIFY_ENVCFG_MASK) == FDI_VERIFY_ENVCFG_MASK ? 0 : -EIO;
		preempt_enable();
		if (error) {
			riscv_dasics_verify_reset_task(current);
			return error;
		}
	}
	return 0;
}
EXPORT_SYMBOL_GPL(riscv_dasics_verify_begin);

int riscv_dasics_verify_arm(u64 cookie, u64 kind, unsigned long site)
{
	struct riscv_dasics_verify_state *state = &current->thread.dasics.verify;
	struct vm_area_struct *vma;
	u32 word;
	bool executable;

	if (!verify_matches(cookie))
		return -ESTALE;
	if ((site & 1) || site > TASK_SIZE - sizeof(word))
		return -EINVAL;
	mmap_read_lock(current->mm);
	vma = find_vma(current->mm, site);
	executable = vma && site >= vma->vm_start && site + sizeof(word) <= vma->vm_end &&
		(vma->vm_flags & VM_EXEC) && !(vma->vm_flags & VM_WRITE);
	mmap_read_unlock(current->mm);
	if (!executable || copy_from_user(&word, (const void __user *)site, sizeof(word)))
		return -EFAULT;
	if (!((kind == FDI_VERIFY_CSR_READ && word == 0x8b302573) ||
	      (kind == FDI_VERIFY_CSR_WRITE && word == 0x88051073) ||
	      (kind == FDI_VERIFY_FDICALL && ((word & 0x707f) == 0x7b ||
					     (word & 0x707f) == 0x107b))))
		return -EINVAL;
	memset(&state->record, 0, sizeof(state->record));
	state->record.kind = kind;
	state->record.site = site;
	state->record.instruction = word;
	return 0;
}
EXPORT_SYMBOL_GPL(riscv_dasics_verify_arm);

int riscv_dasics_verify_query(u64 cookie, struct fdi_verify_result *result)
{
	struct riscv_dasics_verify_state *state = &current->thread.dasics.verify;
	unsigned long machine = 0;
	int error;

	if (!verify_matches(cookie))
		return -ESTALE;
	if (state->flags & FDI_VERIFY_CBO) {
		error = verify_menvcfg(&machine);
		if (error)
			return error;
	}
	preempt_disable();
	*result = state->record;
	result->version = FDI_VERIFY_VERSION;
	result->flags = state->flags;
	result->cookie = state->cookie;
	result->active = 1;
	result->pid = task_pid_vnr(current);
	result->menvcfg = machine;
	result->senvcfg = csr_read(CSR_ENVCFG);
	result->shadow_envcfg = current->thread.envcfg;
	result->saved_envcfg = state->saved_envcfg;
	result->cbom_block_size = riscv_cbom_block_size;
	result->cboz_block_size = riscv_cboz_block_size;
	preempt_enable();
	return 0;
}
EXPORT_SYMBOL_GPL(riscv_dasics_verify_query);

int riscv_dasics_verify_end(u64 cookie, struct fdi_verify_result *result)
{
	int error = riscv_dasics_verify_query(cookie, result);

	/* A failed observation must not leave temporary permissions enabled. */
	if (!verify_matches(cookie))
		return -ESTALE;
	if (error) {
		riscv_dasics_verify_reset_task(current);
		return error;
	}
	riscv_dasics_verify_reset_task(current);
	preempt_disable();
	result->active = 0;
	result->senvcfg = csr_read(CSR_ENVCFG);
	result->shadow_envcfg = current->thread.envcfg;
	preempt_enable();
	if ((result->senvcfg & FDI_VERIFY_ENVCFG_MASK) != result->saved_envcfg ||
	    (result->shadow_envcfg & FDI_VERIFY_ENVCFG_MASK) != result->saved_envcfg)
		return -EIO;
	return 0;
}
EXPORT_SYMBOL_GPL(riscv_dasics_verify_end);

struct riscv_dasics_revoke_work {
	struct callback_head work;
	struct task_struct *owner;
	u64 cookie;
};

void riscv_dasics_verify_free_revoke(struct riscv_dasics_revoke_work *work)
{
	if (work) {
		put_task_struct(work->owner);
		kfree(work);
	}
}
EXPORT_SYMBOL_GPL(riscv_dasics_verify_free_revoke);

static void verify_revoke_work(struct callback_head *callback)
{
	struct riscv_dasics_revoke_work *work = container_of(callback, struct riscv_dasics_revoke_work, work);

	if (current->thread.dasics.verify.cookie == work->cookie)
		riscv_dasics_verify_reset_task(current);
	riscv_dasics_verify_free_revoke(work);
}

struct riscv_dasics_revoke_work *riscv_dasics_verify_alloc_revoke(struct task_struct *owner)
{
	struct riscv_dasics_revoke_work *work = kmalloc(sizeof(*work), GFP_KERNEL);

	if (!work)
		return NULL;
	work->owner = owner;
	get_task_struct(owner);
	init_task_work(&work->work, verify_revoke_work);
	return work;
}
EXPORT_SYMBOL_GPL(riscv_dasics_verify_alloc_revoke);

int riscv_dasics_verify_revoke(struct task_struct *task, u64 cookie,
			     struct riscv_dasics_revoke_work **reserved)
{
	struct riscv_dasics_revoke_work *work;
	int error;

	if (!cookie)
		return 0;
	if (task == current) {
		if (current->thread.dasics.verify.cookie == cookie)
			riscv_dasics_verify_reset_task(current);
		return 0;
	}
	/* A shared-fd close may run in another task. Revoke in the owner before
	 * its next user return, not by writing the closing task's hardware CSR.
	 * Storage was reserved before BEGIN, so close cannot fail to allocate. */
	if (WARN_ON(!reserved || !*reserved || (*reserved)->owner != task))
		return -EINVAL;
	work = *reserved;
	work->cookie = cookie;
	*reserved = NULL;
	error = task_work_add(task, &work->work, TWA_RESUME);
	if (error) {
		*reserved = work;
		/* An exited task cannot return to user mode; arch release clears it. */
		if (error == -ESRCH)
			return 0;
	}
	return error;
}
EXPORT_SYMBOL_GPL(riscv_dasics_verify_revoke);

void riscv_dasics_verify_illegal(const struct pt_regs *regs)
{
	struct riscv_dasics_verify_state *state = &current->thread.dasics.verify;
	struct fdi_verify_result *record = &state->record;

	if (!state->cookie || state->mm != current->mm || !user_mode(regs) || regs->cause != 2)
		return;
	preempt_disable();
	record->illegal_count++;
	if (record->illegal_count == 1) {
		record->cause = regs->cause;
		record->epc = regs->epc;
		record->tval = regs->badaddr;
		/* User-origin entry skips SAVE_DASICS. The extended pt_regs FDI
		 * fields are therefore invalid; read the task's live CSR bank. */
		record->freason = csr_read(0x8b3);
		record->return_pc = csr_read(0x8b1);
		record->gpr[0] = 0;
		BUILD_BUG_ON(offsetof(struct pt_regs, t6) - offsetof(struct pt_regs, ra) != 30 * sizeof(unsigned long));
		memcpy(&record->gpr[1], &regs->ra, 31 * sizeof(unsigned long));
	}
	preempt_enable();
}
