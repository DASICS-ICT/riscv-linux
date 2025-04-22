// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <linux/cpu.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/sched/debug.h>
#include <linux/sched/signal.h>
#include <linux/signal.h>
#include <linux/kdebug.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/irq.h>
#include <linux/kexec.h>

#include <asm/processor.h>
#include <asm/ptrace.h>
#include <asm/csr.h>

#include <linux/hashtable.h>
#include <asm/kdasics.h>
#include <linux/random.h>

int show_unhandled_signals = 1;

extern asmlinkage void handle_exception(void);

static DEFINE_SPINLOCK(die_lock);

void die(struct pt_regs *regs, const char *str)
{
	static int die_counter;
	int ret;

	oops_enter();

	spin_lock_irq(&die_lock);
	console_verbose();
	bust_spinlocks(1);

	pr_emerg("%s [#%d]\n", str, ++die_counter);
	print_modules();
	show_regs(regs);

#ifdef CONFIG_DASICS
	show_ext_regs(regs);
#endif 

	ret = notify_die(DIE_OOPS, str, regs, 0, regs->cause, SIGSEGV);

	if (regs && kexec_should_crash(current))
		crash_kexec(regs);

	bust_spinlocks(0);
	add_taint(TAINT_DIE, LOCKDEP_NOW_UNRELIABLE);
	spin_unlock_irq(&die_lock);
	oops_exit();

	if (in_interrupt())
		panic("Fatal exception in interrupt");
	if (panic_on_oops)
		panic("Fatal exception");
	if (ret != NOTIFY_STOP)
		make_task_dead(SIGSEGV);
}

void do_trap(struct pt_regs *regs, int signo, int code, unsigned long addr)
{
	struct task_struct *tsk = current;

	if (show_unhandled_signals && unhandled_signal(tsk, signo)
	    && printk_ratelimit()) {
		pr_info("%s[%d]: unhandled signal %d code 0x%x at 0x" REG_FMT,
			tsk->comm, task_pid_nr(tsk), signo, code, addr);
		print_vma_addr(KERN_CONT " in ", instruction_pointer(regs));
		pr_cont("\n");
		show_regs(regs);
		
#ifdef CONFIG_DASICS
		show_ext_regs(regs);
#endif 

	}

	force_sig_fault(signo, code, (void __user *)addr);
}

static void do_trap_error(struct pt_regs *regs, int signo, int code,
	unsigned long addr, const char *str)
{
	if (user_mode(regs)) {
		do_trap(regs, signo, code, addr);
	} else {
		if (!fixup_exception(regs))
			die(regs, str);
	}
}

#define DO_ERROR_INFO(name, signo, code, str)				\
asmlinkage __visible void name(struct pt_regs *regs)			\
{									\
	do_trap_error(regs, signo, code, regs->epc, "Oops - " str);	\
}

DO_ERROR_INFO(do_trap_unknown,
	SIGILL, ILL_ILLTRP, "unknown exception");
DO_ERROR_INFO(do_trap_insn_misaligned,
	SIGBUS, BUS_ADRALN, "instruction address misaligned");
DO_ERROR_INFO(do_trap_insn_fault,
	SIGSEGV, SEGV_ACCERR, "instruction access fault");
DO_ERROR_INFO(do_trap_insn_illegal,
	SIGILL, ILL_ILLOPC, "illegal instruction");
DO_ERROR_INFO(do_trap_load_fault,
	SIGSEGV, SEGV_ACCERR, "load access fault");
#ifndef CONFIG_RISCV_M_MODE
DO_ERROR_INFO(do_trap_load_misaligned,
	SIGBUS, BUS_ADRALN, "Oops - load address misaligned");
DO_ERROR_INFO(do_trap_store_misaligned,
	SIGBUS, BUS_ADRALN, "Oops - store (or AMO) address misaligned");
#else
int handle_misaligned_load(struct pt_regs *regs);
int handle_misaligned_store(struct pt_regs *regs);

asmlinkage void do_trap_load_misaligned(struct pt_regs *regs)
{
	if (!handle_misaligned_load(regs))
		return;
	do_trap_error(regs, SIGBUS, BUS_ADRALN, regs->epc,
		      "Oops - load address misaligned");
}

asmlinkage void do_trap_store_misaligned(struct pt_regs *regs)
{
	if (!handle_misaligned_store(regs))
		return;
	do_trap_error(regs, SIGBUS, BUS_ADRALN, regs->epc,
		      "Oops - store (or AMO) address misaligned");
}
#endif
DO_ERROR_INFO(do_trap_store_fault,
	SIGSEGV, SEGV_ACCERR, "store (or AMO) access fault");
DO_ERROR_INFO(do_trap_ecall_u,
	SIGILL, ILL_ILLTRP, "environment call from U-mode");
DO_ERROR_INFO(do_trap_ecall_s,
	SIGILL, ILL_ILLTRP, "environment call from S-mode");
DO_ERROR_INFO(do_trap_ecall_m,
	SIGILL, ILL_ILLTRP, "environment call from M-mode");

static inline unsigned long get_break_insn_length(unsigned long pc)
{
	bug_insn_t insn;

	if (get_kernel_nofault(insn, (bug_insn_t *)pc))
		return 0;

	return GET_INSN_LENGTH(insn);
}

asmlinkage __visible void do_trap_break(struct pt_regs *regs)
{
	if (user_mode(regs))
		force_sig_fault(SIGTRAP, TRAP_BRKPT, (void __user *)regs->epc);
#ifdef CONFIG_KGDB
	else if (notify_die(DIE_TRAP, "EBREAK", regs, 0, regs->cause, SIGTRAP)
								== NOTIFY_STOP)
		return;
#endif
	else if (report_bug(regs->epc, regs) == BUG_TRAP_TYPE_WARN)
		regs->epc += get_break_insn_length(regs->epc);
	else
		die(regs, "Kernel BUG");
}

#ifdef CONFIG_GENERIC_BUG
int is_valid_bugaddr(unsigned long pc)
{
	bug_insn_t insn;

	if (pc < VMALLOC_START)
		return 0;
	if (get_kernel_nofault(insn, (bug_insn_t *)pc))
		return 0;
	if ((insn & __INSN_LENGTH_MASK) == __INSN_LENGTH_32)
		return (insn == __BUG_INSN_32);
	else
		return ((insn & __COMPRESSED_INSN_MASK) == __BUG_INSN_16);
}
#endif /* CONFIG_GENERIC_BUG */

static int dasics_ldst_checker(uint64_t stval, int is_read, struct pt_regs *regs) {
	int valid_perm = DASICS_LIBCFG_V | (is_read ? DASICS_LIBCFG_R : DASICS_LIBCFG_W);
    uint64_t libcfg = regs->dasicsLibCfg0;   // DasicsLibCfg
    int step = 4;

	struct dasics_bound *bound;
	int bkt;

	hash_for_each(current->dasics_hash_table, bkt, bound, node) {
		if (bound->lo <= stval && stval < bound->hi && \
			(bound->priv & valid_perm) == valid_perm) {

			int victim = get_random_u32() % DASICS_LIBCFG_WIDTH;
			regs->dasicsLibBounds[victim][0] = bound->lo;
			regs->dasicsLibBounds[victim][1] = bound->hi;

			libcfg &= ~(DASICS_LIBCFG_MASK << (victim * step));
			libcfg |= ((uint64_t)bound->priv) << (victim * step);
			regs->dasicsLibCfg0 = libcfg;

			current->dlibcfg_handle_map[victim] = bound->handle;

			pr_info("[DASICS EXCEPTION]Info: dasics load/store fault OK! new csr idx is %d, lo = 0x%lx, hi = 0x%lx\n", 
				victim, bound->lo, bound->hi);
			return victim;
		}
	}

	return -1;
}
/* This function may handle dasics exceptions in another way in future. */
asmlinkage void do_trap_dasics(struct pt_regs *regs) 
{
	char *trap_name = regs->cause == 0x18 ? "fetch" :
					  regs->cause == 0x19 ? "load"  : "store";

	// show_regs(regs);
	// show_ext_regs(regs);
	pr_info("[DASICS EXCEPTION]Info: dasics %s fault occurs, scause = 0x%lx spec = 0x%lx stval = 0x%lx\n",
		                                trap_name, regs->cause, regs->epc, regs->stval);
	
	if (regs->cause == 0x18) die(regs, "Jump Error!");

	// load/store
	int is_read = regs->cause == 0x19;
	int csr_idx = dasics_ldst_checker(regs->stval, is_read, regs);

	if (csr_idx == -1) die(regs, "No load/store bound!");	
}
/* stvec & scratch is already set from head.S */
void trap_init(void)
{
}
