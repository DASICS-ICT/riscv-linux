/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 * Copyright (C) 2017 SiFive
 */

#include <linux/init.h>
#include <linux/linkage.h>

#include <asm/asm.h>
#include <asm/csr.h>
#include <asm/unistd.h>
#include <asm/thread_info.h>
#include <asm/asm-offsets.h>
#include <asm/page.h>

// === macro defination zone ===

	.text
	.altmacro

/* 
 * Don't modify s0-s5 since they will be used in handle_exception, 
 * Or you should save csr s registers in final.
 * We don't save umcfg here since it is previously saved.
 */ 
	.macro SAVE_DASICS

	/* Save dasics main context */	
	csrrw t0, CSR_DUMCFG, zero
  	REG_S t0, PT_DUMCFG(sp)	 
	csrr t1, CSR_DUMBOUNDLO
	REG_S t1, PT_DUMBOUNDLO(sp)
	csrr t0, CSR_DUMBOUNDHI
	REG_S t0, PT_DUMBOUNDHI(sp)


	csrr t0, CSR_DLCFG0
	#csrr t1, CSR_DLCFG1
	REG_S t0, PT_DLCFG0(sp)
	#REG_S t1, PT_DLCFG1(sp)

	csrr t0, CSR_DLBOUND0LO
	csrr t1, CSR_DLBOUND0HI
	REG_S t0, PT_DLBOUND0LO(sp)
	REG_S t1, PT_DLBOUND0HI(sp)
	csrr t0, CSR_DLBOUND1LO
	csrr t1, CSR_DLBOUND1HI
	REG_S t0, PT_DLBOUND1LO(sp)
	REG_S t1, PT_DLBOUND1HI(sp)
	csrr t0, CSR_DLBOUND2LO
	csrr t1, CSR_DLBOUND2HI
	REG_S t0, PT_DLBOUND2LO(sp)
	REG_S t1, PT_DLBOUND2HI(sp)
	csrr t0, CSR_DLBOUND3LO
	csrr t1, CSR_DLBOUND3HI
	REG_S t0, PT_DLBOUND3LO(sp)
	REG_S t1, PT_DLBOUND3HI(sp)
	csrr t0, CSR_DLBOUND4LO
	csrr t1, CSR_DLBOUND4HI
	REG_S t0, PT_DLBOUND4LO(sp)
	REG_S t1, PT_DLBOUND4HI(sp)
	csrr t0, CSR_DLBOUND5LO
	csrr t1, CSR_DLBOUND5HI
	REG_S t0, PT_DLBOUND5LO(sp)
	REG_S t1, PT_DLBOUND5HI(sp)
	csrr t0, CSR_DLBOUND6LO
	csrr t1, CSR_DLBOUND6HI
	REG_S t0, PT_DLBOUND6LO(sp)
	REG_S t1, PT_DLBOUND6HI(sp)
	csrr t0, CSR_DLBOUND7LO
	csrr t1, CSR_DLBOUND7HI
	REG_S t0, PT_DLBOUND7LO(sp)
	REG_S t1, PT_DLBOUND7HI(sp)

	csrr t0, CSR_DLBOUND8LO
	csrr t1, CSR_DLBOUND8HI
	REG_S t0, PT_DLBOUND8LO(sp)
	REG_S t1, PT_DLBOUND8HI(sp)
	csrr t0, CSR_DLBOUND9LO
	csrr t1, CSR_DLBOUND9HI
	REG_S t0, PT_DLBOUND9LO(sp)
	REG_S t1, PT_DLBOUND9HI(sp)
	csrr t0, CSR_DLBOUND10LO
	csrr t1, CSR_DLBOUND10HI
	REG_S t0, PT_DLBOUND10LO(sp)
	REG_S t1, PT_DLBOUND10HI(sp)
	csrr t0, CSR_DLBOUND11LO
	csrr t1, CSR_DLBOUND11HI
	REG_S t0, PT_DLBOUND11LO(sp)
	REG_S t1, PT_DLBOUND11HI(sp)
	csrr t0, CSR_DLBOUND12LO
	csrr t1, CSR_DLBOUND12HI
	REG_S t0, PT_DLBOUND12LO(sp)
	REG_S t1, PT_DLBOUND12HI(sp)
	csrr t0, CSR_DLBOUND13LO
	csrr t1, CSR_DLBOUND13HI
	REG_S t0, PT_DLBOUND13LO(sp)
	REG_S t1, PT_DLBOUND13HI(sp)
	csrr t0, CSR_DLBOUND14LO
	csrr t1, CSR_DLBOUND14HI
	REG_S t0, PT_DLBOUND14LO(sp)
	REG_S t1, PT_DLBOUND14HI(sp)
	csrr t0, CSR_DLBOUND15LO
	csrr t1, CSR_DLBOUND15HI
	REG_S t0, PT_DLBOUND15LO(sp)
	REG_S t1, PT_DLBOUND15HI(sp)

	csrr t0, CSR_DMAINCALL
	REG_S t0, PT_DMAINCALL(sp)
	csrr t1, CSR_DRETURNPC
	REG_S t1, PT_DRETURNPC(sp)
	csrr t0, CSR_DFZRETURN
	REG_S t0, PT_DFZRETURN(sp)

	csrr t0, CSR_DJBOUND0LO
	csrr t1, CSR_DJBOUND0HI
	REG_S t0, PT_DJBOUND0LO(sp)
	REG_S t1, PT_DJBOUND0HI(sp)
	csrr t0, CSR_DJBOUND1LO
	csrr t1, CSR_DJBOUND1HI
	REG_S t0, PT_DJBOUND1LO(sp)
	REG_S t1, PT_DJBOUND1HI(sp)
	csrr t0, CSR_DJBOUND2LO
	csrr t1, CSR_DJBOUND2HI
	REG_S t0, PT_DJBOUND2LO(sp)
	REG_S t1, PT_DJBOUND2HI(sp)
	csrr t0, CSR_DJBOUND3LO
	csrr t1, CSR_DJBOUND3HI
	REG_S t0, PT_DJBOUND3LO(sp)
	REG_S t1, PT_DJBOUND3HI(sp)

	csrr t0, CSR_DJCFG
	REG_S t0, PT_DJCFG(sp)

	.endm

	.macro SAVE_UCSR

	/* save N Extension registers. */
  	csrr t0, CSR_USTATUS
  	REG_S t0, PT_USTATUS(sp)
 	csrr t1, CSR_UEPC
  	REG_S t1, PT_UEPC(sp) 
  	csrr t0, CSR_UTVAL
  	REG_S t0, PT_UBADADDR(sp) 
  	csrr t1, CSR_UCAUSE
  	REG_S t1, PT_UCAUSE(sp)
	csrr t0, CSR_UTVEC
  	REG_S t0, PT_UTVEC(sp)
  	csrr t1, CSR_UIE  	
  	REG_S t1, PT_UIE(sp)	  
	csrr t0, CSR_UIP
  	REG_S t0, PT_UIP(sp)
	csrr t1, CSR_USCRATCH
 	REG_S t1, PT_USCRATCH(sp)

	.endm 

/*
 * Prepares to enter a system call or exception by saving all registers to the
 * stack.
 */
	.macro SAVE_ALL
	LOCAL _restore_kernel_tpsp
	LOCAL _save_context
	LOCAL _check_kernel
	LOCAL _check_dasics
	LOCAL _save_dasics
	LOCAL _save_end

	/*
	 * If coming from userspace, preserve the user thread pointer and load
	 * the kernel thread pointer.  If we came from the kernel, sscratch
	 * will contain 0, and we should continue on the current TP.
	 */

	csrrw tp, CSR_SCRATCH, tp
	bnez tp, _save_context

_restore_kernel_tpsp:
	csrr tp, CSR_SCRATCH
	REG_S sp, TASK_TI_KERNEL_SP(tp)
_save_context:
	REG_S sp, TASK_TI_USER_SP(tp)
	REG_L sp, TASK_TI_KERNEL_SP(tp)
	addi sp, sp, -(PT_SIZE_ON_STACK)
	REG_S x1,  PT_RA(sp)
	REG_S x3,  PT_GP(sp)
	REG_S x5,  PT_T0(sp)
	REG_S x6,  PT_T1(sp)
	REG_S x7,  PT_T2(sp)
	REG_S x8,  PT_S0(sp)
	REG_S x9,  PT_S1(sp)
	REG_S x10, PT_A0(sp)
	REG_S x11, PT_A1(sp)
	REG_S x12, PT_A2(sp)
	REG_S x13, PT_A3(sp)
	REG_S x14, PT_A4(sp)
	REG_S x15, PT_A5(sp)
	REG_S x16, PT_A6(sp)
	REG_S x17, PT_A7(sp)
	REG_S x18, PT_S2(sp)
	REG_S x19, PT_S3(sp)
	REG_S x20, PT_S4(sp)
	REG_S x21, PT_S5(sp)
	REG_S x22, PT_S6(sp)
	REG_S x23, PT_S7(sp)
	REG_S x24, PT_S8(sp)
	REG_S x25, PT_S9(sp)
	REG_S x26, PT_S10(sp)
	REG_S x27, PT_S11(sp)
	REG_S x28, PT_T3(sp)
	REG_S x29, PT_T4(sp)
	REG_S x30, PT_T5(sp)
	REG_S x31, PT_T6(sp)

	/*
	 * Disable user-mode memory access as it should only be set in the
	 * actual user copy routines.
	 *
	 * Disable the FPU to detect illegal usage of floating point in kernel
	 * space.
	 */
	li t0, SR_SUM | SR_FS

	REG_L s0, TASK_TI_USER_SP(tp)
	csrrc s1, CSR_STATUS, t0
	csrr s2, CSR_EPC
	csrr s3, CSR_TVAL
	csrr s4, CSR_CAUSE
	csrr s5, CSR_SCRATCH
	REG_S s0, PT_SP(sp)
	REG_S s1, PT_STATUS(sp)
	REG_S s2, PT_EPC(sp)
	REG_S s3, PT_BADADDR(sp)
	REG_S s4, PT_CAUSE(sp)
	REG_S s5, PT_TP(sp)

_check_kernel:
	/* if exception comes from kernel, sscratch contains 0 */
	# bnez s5, _check_dasics
	# REG_S zero, PT_DUMCFG(sp)
	# j _save_end

_check_dasics:
	/* dasics uregs are thread specific.
	 * if this thread doesn't open dasics, 
	 * its umaincfg should be set to 0 
	 * before starting thread. */
	REG_L t0, PT_DUMCFG(sp)
	bnez t0, _save_dasics
	j _save_end
_save_dasics:
	SAVE_UCSR
	SAVE_DASICS
_save_end:	
	.endm

	.macro RESTORE_DASICS

	/* Restore dasics main context */
	/* dumaincfg is saved previously */
	// REG_L t0, PT_DUMCFG(sp)
	// csrw CSR_DUMCFG, t0
	REG_L t1, PT_DUMBOUNDLO(sp)
	csrw CSR_DUMBOUNDLO, t1
	REG_L t0, PT_DUMBOUNDHI(sp)
	csrw CSR_DUMBOUNDHI, t0


	REG_L t0, PT_DLCFG0(sp)
	# REG_L t1, PT_DLCFG1(sp)
	csrw CSR_DLCFG0, t0
	# csrw CSR_DLCFG1, t1

	REG_L t0, PT_DLBOUND0LO(sp)
	REG_L t1, PT_DLBOUND0HI(sp)
	csrw 	 CSR_DLBOUND0LO, t0
	csrw 	 CSR_DLBOUND0HI, t1
	REG_L t0, PT_DLBOUND1LO(sp)
	REG_L t1, PT_DLBOUND1HI(sp)
	csrw 	 CSR_DLBOUND1LO, t0
	csrw 	 CSR_DLBOUND1HI, t1
	REG_L t0, PT_DLBOUND2LO(sp)
	REG_L t1, PT_DLBOUND2HI(sp)
	csrw 	 CSR_DLBOUND2LO, t0
	csrw 	 CSR_DLBOUND2HI, t1
	REG_L t0, PT_DLBOUND3LO(sp)
	REG_L t1, PT_DLBOUND3HI(sp)
	csrw 	 CSR_DLBOUND3LO, t0
	csrw 	 CSR_DLBOUND3HI, t1
	REG_L t0, PT_DLBOUND4LO(sp)
	REG_L t1, PT_DLBOUND4HI(sp)
	csrw 	 CSR_DLBOUND4LO, t0
	csrw 	 CSR_DLBOUND4HI, t1
	REG_L t0, PT_DLBOUND5LO(sp)
	REG_L t1, PT_DLBOUND5HI(sp)
	csrw 	 CSR_DLBOUND5LO, t0
	csrw 	 CSR_DLBOUND5HI, t1
	REG_L t0, PT_DLBOUND6LO(sp)
	REG_L t1, PT_DLBOUND6HI(sp)
	csrw 	 CSR_DLBOUND6LO, t0
	csrw 	 CSR_DLBOUND6HI, t1
	REG_L t0, PT_DLBOUND7LO(sp)
	REG_L t1, PT_DLBOUND7HI(sp)
	csrw 	 CSR_DLBOUND7LO, t0
	csrw 	 CSR_DLBOUND7HI, t1

	REG_L t0, PT_DLBOUND8LO(sp)
	REG_L t1, PT_DLBOUND8HI(sp)
	csrw 	 CSR_DLBOUND8LO, t0
	csrw 	 CSR_DLBOUND8HI, t1
	REG_L t0, PT_DLBOUND9LO(sp)
	REG_L t1, PT_DLBOUND9HI(sp)
	csrw 	 CSR_DLBOUND9LO, t0
	csrw 	 CSR_DLBOUND9HI, t1
	REG_L t0, PT_DLBOUND10LO(sp)
	REG_L t1, PT_DLBOUND10HI(sp)
	csrw 	 CSR_DLBOUND10LO, t0
	csrw 	 CSR_DLBOUND10HI, t1
	REG_L t0, PT_DLBOUND11LO(sp)
	REG_L t1, PT_DLBOUND11HI(sp)
	csrw 	 CSR_DLBOUND11LO, t0
	csrw 	 CSR_DLBOUND11HI, t1
	REG_L t0, PT_DLBOUND12LO(sp)
	REG_L t1, PT_DLBOUND12HI(sp)
	csrw 	 CSR_DLBOUND12LO, t0
	csrw 	 CSR_DLBOUND12HI, t1
	REG_L t0, PT_DLBOUND13LO(sp)
	REG_L t1, PT_DLBOUND13HI(sp)
	csrw 	 CSR_DLBOUND13LO, t0
	csrw 	 CSR_DLBOUND13HI, t1
	REG_L t0, PT_DLBOUND14LO(sp)
	REG_L t1, PT_DLBOUND14HI(sp)
	csrw 	 CSR_DLBOUND14LO, t0
	csrw 	 CSR_DLBOUND14HI, t1
	REG_L t0, PT_DLBOUND15LO(sp)
	REG_L t1, PT_DLBOUND15HI(sp)
	csrw 	 CSR_DLBOUND15LO, t0
	csrw 	 CSR_DLBOUND15HI, t1

	REG_L t0, PT_DMAINCALL(sp)
	csrw CSR_DMAINCALL, t0
	REG_L t1, PT_DRETURNPC(sp)
	csrw CSR_DRETURNPC, t1
	REG_L t0, PT_DFZRETURN(sp)
	csrw CSR_DFZRETURN, t0

	REG_L t0, PT_DLBOUND0LO(sp)
	REG_L t1, PT_DLBOUND0HI(sp)
	csrw 	 CSR_DLBOUND0LO, t0
	csrw 	 CSR_DLBOUND0HI, t1
	REG_L t0, PT_DLBOUND1LO(sp)
	REG_L t1, PT_DLBOUND1HI(sp)
	csrw 	 CSR_DLBOUND1LO, t0
	csrw 	 CSR_DLBOUND1HI, t1
	REG_L t0, PT_DLBOUND2LO(sp)
	REG_L t1, PT_DLBOUND2HI(sp)
	csrw 	 CSR_DLBOUND2LO, t0
	csrw 	 CSR_DLBOUND2HI, t1
	REG_L t0, PT_DLBOUND3LO(sp)
	REG_L t1, PT_DLBOUND3HI(sp)
	csrw 	 CSR_DLBOUND3LO, t0
	csrw 	 CSR_DLBOUND3HI, t1

	REG_L t0, PT_DJCFG(sp)
	csrw 	 CSR_DJCFG, t0

	.endm

	.macro RESTORE_UCSR

	/* Restore N Extension registers. */
	REG_L t0, PT_USTATUS(sp)	
	csrw CSR_USTATUS, t0
	REG_L t1, PT_UEPC(sp)
	csrw CSR_UEPC, t1
	REG_L t0, PT_UBADADDR(sp)
	csrw CSR_UTVAL, t0
	REG_L t1, PT_UCAUSE(sp)
	csrw CSR_UCAUSE, t1	
	REG_L t0, PT_UTVEC(sp)	
	csrw CSR_UTVEC, t0
	REG_L t1, PT_UIE(sp)	
	csrw CSR_UIE, t1	
	REG_L t0, PT_UIP(sp)	
	csrw CSR_UIP, t0
	REG_L t1, PT_USCRATCH(sp)	
	csrw CSR_USCRATCH, t1
	.endm
/*
 * Prepares to return from a system call or exception by restoring all
 * registers from the stack.
 */
	.macro RESTORE_ALL
	LOCAL _restore_general
	LOCAL _check_kernel
	LOCAL _check_dasics
	LOCAL _restore_dasics

	REG_L a0, PT_STATUS(sp)
	/*
	 * The current load reservation is effectively part of the processor's
	 * state, in the sense that load reservations cannot be shared between
	 * different hart contexts.  We can't actually save and restore a load
	 * reservation, so instead here we clear any existing reservation --
	 * it's always legal for implementations to clear load reservations at
	 * any point (as long as the forward progress guarantee is kept, but
	 * we'll ignore that here).
	 *
	 * Dangling load reservations can be the result of taking a trap in the
	 * middle of an LR/SC sequence, but can also be the result of a taken
	 * forward branch around an SC -- which is how we implement CAS.  As a
	 * result we need to clear reservations between the last CAS and the
	 * jump back to the new context.  While it is unlikely the store
	 * completes, implementations are allowed to expand reservations to be
	 * arbitrarily large.
	 */
	REG_L  a2, PT_EPC(sp)
	REG_SC x0, a2, PT_EPC(sp)

	csrw CSR_STATUS, a0
	csrw CSR_EPC, a2

_check_kernel:
	/* How can I know I handled an exception from kernel thread? */
	# andi t0, a0, SR_SPP
	# beqz t0, _check_dasics
	# csrw CSR_DUMCFG, zero
	# j _restore_general
_check_dasics:
	REG_L t0, PT_DUMCFG(sp)
	csrw CSR_DUMCFG, t0
	beqz t0, _restore_general
_restore_dasics:
	RESTORE_DASICS
	RESTORE_UCSR
_restore_general:
	/* restore general regs */

	REG_L x1,  PT_RA(sp)
	REG_L x3,  PT_GP(sp)
	REG_L x4,  PT_TP(sp)
	REG_L x5,  PT_T0(sp)
	REG_L x6,  PT_T1(sp)
	REG_L x7,  PT_T2(sp)
	REG_L x8,  PT_S0(sp)
	REG_L x9,  PT_S1(sp)
	REG_L x10, PT_A0(sp)
	REG_L x11, PT_A1(sp)
	REG_L x12, PT_A2(sp)
	REG_L x13, PT_A3(sp)
	REG_L x14, PT_A4(sp)
	REG_L x15, PT_A5(sp)
	REG_L x16, PT_A6(sp)
	REG_L x17, PT_A7(sp)
	REG_L x18, PT_S2(sp)
	REG_L x19, PT_S3(sp)
	REG_L x20, PT_S4(sp)
	REG_L x21, PT_S5(sp)
	REG_L x22, PT_S6(sp)
	REG_L x23, PT_S7(sp)
	REG_L x24, PT_S8(sp)
	REG_L x25, PT_S9(sp)
	REG_L x26, PT_S10(sp)
	REG_L x27, PT_S11(sp)
	REG_L x28, PT_T3(sp)
	REG_L x29, PT_T4(sp)
	REG_L x30, PT_T5(sp)
	REG_L x31, PT_T6(sp)

	REG_L x2,  PT_SP(sp)
	.endm

// === macro defination zone end ===



#if !IS_ENABLED(CONFIG_PREEMPTION)
.set resume_kernel, restore_all
#endif

ENTRY(handle_exception)
	/*
	 * If coming from userspace, preserve the user thread pointer and load
	 * the kernel thread pointer.  If we came from the kernel, the scratch
	 * register will contain 0, and we should continue on the current TP.
	 */
	sfence.vma
	SAVE_ALL
	/*
	 * Set the scratch register to 0, so that if a recursive exception
	 * occurs, the exception vector knows it came from the kernel
	 */
	csrw CSR_SCRATCH, x0

load_gp:
	/* Load the global pointer */
.option push
.option norelax
	la gp, __global_pointer$
.option pop

#ifdef CONFIG_TRACE_IRQFLAGS
	call __trace_hardirqs_off
#endif

#ifdef CONFIG_CONTEXT_TRACKING
	/* If previous state is in user mode, call context_tracking_user_exit. */
	li   a0, SR_PP
	and a0, s1, a0
	bnez a0, skip_context_tracking
	call context_tracking_user_exit
skip_context_tracking:
#endif

	/*
	 * MSB of cause differentiates between
	 * interrupts and exceptions
	 */
	bge s4, zero, 1f

	la ra, ret_from_exception

	/* Handle interrupts */
	move a0, sp /* pt_regs */
	la a1, handle_arch_irq
	REG_L a1, (a1)
	jr a1
1:
	/*
	 * Exceptions run with interrupts enabled or disabled depending on the
	 * state of SR_PIE in m/sstatus.
	 */
	andi t0, s1, SR_PIE
	beqz t0, 1f
#ifdef CONFIG_TRACE_IRQFLAGS
	call __trace_hardirqs_on
#endif
	csrs CSR_STATUS, SR_IE

1:
	la ra, ret_from_exception
	/* Handle syscalls */
	li t0, EXC_SYSCALL
	beq s4, t0, handle_syscall

	/* Handle other exceptions */
	slli t0, s4, RISCV_LGPTR
	la t1, excp_vect_table
	la t2, excp_vect_table_end
	move a0, sp /* pt_regs */
	add t0, t1, t0
	/* Check if exception code lies within bounds */
	bgeu t0, t2, 1f
	REG_L t0, 0(t0)
	jr t0
1:
	tail do_trap_unknown

handle_syscall:
#ifdef CONFIG_RISCV_M_MODE
	/*
	 * When running is M-Mode (no MMU config), MPIE does not get set.
	 * As a result, we need to force enable interrupts here because
	 * handle_exception did not do set SR_IE as it always sees SR_PIE
	 * being cleared.
	 */
	csrs CSR_STATUS, SR_IE
#endif
#if defined(CONFIG_TRACE_IRQFLAGS) || defined(CONFIG_CONTEXT_TRACKING)
	/* Recover a0 - a7 for system calls */
	REG_L a0, PT_A0(sp)
	REG_L a1, PT_A1(sp)
	REG_L a2, PT_A2(sp)
	REG_L a3, PT_A3(sp)
	REG_L a4, PT_A4(sp)
	REG_L a5, PT_A5(sp)
	REG_L a6, PT_A6(sp)
	REG_L a7, PT_A7(sp)
#endif
	 /* save the initial A0 value (needed in signal handlers) */
	REG_S a0, PT_ORIG_A0(sp)
	/*
	 * Advance SEPC to avoid executing the original
	 * scall instruction on sret
	 */
	addi s2, s2, 0x4
	REG_S s2, PT_EPC(sp)
	/* Trace syscalls, but only if requested by the user. */
	REG_L t0, TASK_TI_FLAGS(tp)
	andi t0, t0, _TIF_SYSCALL_WORK
	bnez t0, handle_syscall_trace_enter
check_syscall_nr:
	/* Check to make sure we don't jump to a bogus syscall number. */
	li t0, __NR_syscalls
	la s0, sys_ni_syscall
	/*
	 * Syscall number held in a7.
	 * If syscall number is above allowed value, redirect to ni_syscall.
	 */
	bgeu a7, t0, 1f
	/* Call syscall */
	la s0, sys_call_table
	slli t0, a7, RISCV_LGPTR
	add s0, s0, t0
	REG_L s0, 0(s0)
1:
	jalr s0

ret_from_syscall:
	/* Set user a0 to kernel a0 */
	REG_S a0, PT_A0(sp)
	/*
	 * We didn't execute the actual syscall.
	 * Seccomp already set return value for the current task pt_regs.
	 * (If it was configured with SECCOMP_RET_ERRNO/TRACE)
	 */
ret_from_syscall_rejected:
	/* Trace syscalls, but only if requested by the user. */
	REG_L t0, TASK_TI_FLAGS(tp)
	andi t0, t0, _TIF_SYSCALL_WORK
	bnez t0, handle_syscall_trace_exit

ret_from_exception:
	REG_L s0, PT_STATUS(sp)
	csrc CSR_STATUS, SR_IE
#ifdef CONFIG_TRACE_IRQFLAGS
	call __trace_hardirqs_off
#endif
#ifdef CONFIG_RISCV_M_MODE
	/* the MPP value is too large to be used as an immediate arg for addi */
	li t0, SR_MPP
	and s0, s0, t0
#else
	andi s0, s0, SR_SPP
#endif
	bnez s0, resume_kernel

resume_userspace:
	/* Interrupts must be disabled here so flags are checked atomically */
	REG_L s0, TASK_TI_FLAGS(tp) /* current_thread_info->flags */
	andi s1, s0, _TIF_WORK_MASK
	bnez s1, work_pending

#ifdef CONFIG_CONTEXT_TRACKING
	call context_tracking_user_enter
#endif

	/* Save unwound kernel stack pointer in thread_info */
	addi s0, sp, PT_SIZE_ON_STACK
	REG_S s0, TASK_TI_KERNEL_SP(tp)

	/*
	 * Save TP into the scratch register , so we can find the kernel data
	 * structures again.
	 */
	csrw CSR_SCRATCH, tp

restore_all:
#ifdef CONFIG_TRACE_IRQFLAGS
	REG_L s1, PT_STATUS(sp)
	andi t0, s1, SR_PIE
	beqz t0, 1f
	call __trace_hardirqs_on
	j 2f
1:
	call __trace_hardirqs_off
2:
#endif

	RESTORE_ALL

#ifdef CONFIG_RISCV_M_MODE
	mret
#else
	sret
#endif

#if IS_ENABLED(CONFIG_PREEMPTION)
resume_kernel:
	REG_L s0, TASK_TI_PREEMPT_COUNT(tp)
	bnez s0, restore_all
	REG_L s0, TASK_TI_FLAGS(tp)
	andi s0, s0, _TIF_NEED_RESCHED
	beqz s0, restore_all
	call preempt_schedule_irq
	j restore_all
#endif

work_pending:
	/* Enter slow path for supplementary processing */
	la ra, ret_from_exception
	andi s1, s0, _TIF_NEED_RESCHED
	bnez s1, work_resched
work_notifysig:
	/* Handle pending signals and notify-resume requests */
	csrs CSR_STATUS, SR_IE /* Enable interrupts for do_notify_resume() */
	move a0, sp /* pt_regs */
	move a1, s0 /* current_thread_info->flags */
	tail do_notify_resume
work_resched:
	tail schedule

/* Slow paths for ptrace. */
handle_syscall_trace_enter:
	move a0, sp
	call do_syscall_trace_enter
	move t0, a0
	REG_L a0, PT_A0(sp)
	REG_L a1, PT_A1(sp)
	REG_L a2, PT_A2(sp)
	REG_L a3, PT_A3(sp)
	REG_L a4, PT_A4(sp)
	REG_L a5, PT_A5(sp)
	REG_L a6, PT_A6(sp)
	REG_L a7, PT_A7(sp)
	bnez t0, ret_from_syscall_rejected
	j check_syscall_nr
handle_syscall_trace_exit:
	move a0, sp
	call do_syscall_trace_exit
	j ret_from_exception

END(handle_exception)

ENTRY(ret_from_fork)
	la ra, ret_from_exception
	tail schedule_tail
ENDPROC(ret_from_fork)

ENTRY(ret_from_kernel_thread)
	call schedule_tail
	/* Call fn(arg) */
	la ra, ret_from_exception
	move a0, s1
	jr s0
ENDPROC(ret_from_kernel_thread)


/*
 * Integer register context switch
 * The callee-saved registers must be saved and restored.
 *
 *   a0: previous task_struct (must be preserved across the switch)
 *   a1: next task_struct
 *
 * The value of a0 and a1 must be preserved by this function, as that's how
 * arguments are passed to schedule_tail.
 */
ENTRY(__switch_to)
	/* Save context into prev->thread */
	li    a4,  TASK_THREAD_RA
	add   a3, a0, a4
	add   a4, a1, a4
	REG_S ra,  TASK_THREAD_RA_RA(a3)
	REG_S sp,  TASK_THREAD_SP_RA(a3)
	REG_S s0,  TASK_THREAD_S0_RA(a3)
	REG_S s1,  TASK_THREAD_S1_RA(a3)
	REG_S s2,  TASK_THREAD_S2_RA(a3)
	REG_S s3,  TASK_THREAD_S3_RA(a3)
	REG_S s4,  TASK_THREAD_S4_RA(a3)
	REG_S s5,  TASK_THREAD_S5_RA(a3)
	REG_S s6,  TASK_THREAD_S6_RA(a3)
	REG_S s7,  TASK_THREAD_S7_RA(a3)
	REG_S s8,  TASK_THREAD_S8_RA(a3)
	REG_S s9,  TASK_THREAD_S9_RA(a3)
	REG_S s10, TASK_THREAD_S10_RA(a3)
	REG_S s11, TASK_THREAD_S11_RA(a3)
	/* Restore context from next->thread */
	REG_L ra,  TASK_THREAD_RA_RA(a4)
	REG_L sp,  TASK_THREAD_SP_RA(a4)
	REG_L s0,  TASK_THREAD_S0_RA(a4)
	REG_L s1,  TASK_THREAD_S1_RA(a4)
	REG_L s2,  TASK_THREAD_S2_RA(a4)
	REG_L s3,  TASK_THREAD_S3_RA(a4)
	REG_L s4,  TASK_THREAD_S4_RA(a4)
	REG_L s5,  TASK_THREAD_S5_RA(a4)
	REG_L s6,  TASK_THREAD_S6_RA(a4)
	REG_L s7,  TASK_THREAD_S7_RA(a4)
	REG_L s8,  TASK_THREAD_S8_RA(a4)
	REG_L s9,  TASK_THREAD_S9_RA(a4)
	REG_L s10, TASK_THREAD_S10_RA(a4)
	REG_L s11, TASK_THREAD_S11_RA(a4)
	/* Swap the CPU entry around. */
	lw a3, TASK_TI_CPU(a0)
	lw a4, TASK_TI_CPU(a1)
	sw a3, TASK_TI_CPU(a1)
	sw a4, TASK_TI_CPU(a0)
	/* The offset of thread_info in task_struct is zero. */
	move tp, a1
	ret
ENDPROC(__switch_to)

#ifndef CONFIG_MMU
#define do_page_fault do_trap_unknown
#endif

	.section ".rodata"
	.align LGREG
	/* Exception vector table */
ENTRY(excp_vect_table)
	RISCV_PTR do_trap_insn_misaligned
	RISCV_PTR do_trap_insn_fault
	RISCV_PTR do_trap_insn_illegal
	RISCV_PTR do_trap_break
	RISCV_PTR do_trap_load_misaligned
	RISCV_PTR do_trap_load_fault
	RISCV_PTR do_trap_store_misaligned
	RISCV_PTR do_trap_store_fault
	RISCV_PTR do_trap_ecall_u /* system call, gets intercepted */
	RISCV_PTR do_trap_ecall_s
	RISCV_PTR do_trap_unknown
	RISCV_PTR do_trap_ecall_m
	RISCV_PTR do_page_fault   /* instruction page fault */
	RISCV_PTR do_page_fault   /* load page fault */
	RISCV_PTR do_trap_unknown
	RISCV_PTR do_page_fault   /* store page fault */
	RISCV_PTR do_trap_unknown
	RISCV_PTR do_trap_unknown
	RISCV_PTR do_trap_unknown
	RISCV_PTR do_trap_unknown
	RISCV_PTR do_trap_unknown
	RISCV_PTR do_trap_unknown
	RISCV_PTR do_trap_unknown
	RISCV_PTR do_trap_unknown	
	RISCV_PTR do_trap_dasics  /* dasics ufetch fault */
	RISCV_PTR do_trap_dasics  /* dasics sfetch fault */
	RISCV_PTR do_trap_dasics  /* dasics uload fault */
	RISCV_PTR do_trap_dasics  /* dasics sload fault */
	RISCV_PTR do_trap_dasics  /* dasics ustore fault */
	RISCV_PTR do_trap_dasics  /* dasics sstore fault */
	RISCV_PTR do_trap_dasics  /* dasics uecall fault */
	RISCV_PTR do_trap_dasics  /* dasics secall fault */
excp_vect_table_end:
END(excp_vect_table)

#ifndef CONFIG_MMU
ENTRY(__user_rt_sigreturn)
	li a7, __NR_rt_sigreturn
	scall
END(__user_rt_sigreturn)
#endif
