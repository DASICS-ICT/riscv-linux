/*
 * Copyright (C) 2012 Regents of the University of California
 * Copyright (C) 2017 SiFive
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation, version 2.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/linkage.h>

#include <asm/asm.h>
#include <asm/csr.h>
#include <asm/unistd.h>
#include <asm/thread_info.h>
#include <asm/asm-offsets.h>
#include <asm/page.h>

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
	csrrw tp, sscratch, tp
	bnez tp, _save_context

_restore_kernel_tpsp:
	csrr tp, sscratch
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
	csrrc s1, sstatus, t0
	csrr s2, sepc
	csrr s3, sbadaddr
	csrr s4, scause
	csrr s5, sscratch
	REG_S s0, PT_SP(sp)
	REG_S s1, PT_SSTATUS(sp)
	REG_S s2, PT_SEPC(sp)
	REG_S s3, PT_SBADADDR(sp)
	REG_S s4, PT_SCAUSE(sp)
	REG_S s5, PT_TP(sp)

_check_kernel:
	/* if exception comes from kernel, sscratch contains 0 */
	// BUG(wh): the sscratch has been changed to tp, if tp be zero, will lost dasics csrs
	andi t0, s1, SR_SPP
	beqz t0, _check_dasics
	// clear CSR_DUMCFG again
	csrw CSR_DUMCFG, zero
	REG_S zero, PT_DUMCFG(sp)
	j _save_end

_check_dasics:
	/* dasics uregs are thread specific.
	 * if this thread doesn't open dasics, 
	 * its umaincfg should be set to 0 
	 * before starting thread. */
	// REG_L t0, PT_DUMCFG(sp)
	// BUG(wh), why read CSR_DUMCFG from kernel stack? the data on the kernel stack is unsure.	
	csrr t0, CSR_DUMCFG
	bnez t0, _save_dasics

	j _save_end
_save_dasics:
	SAVE_UCSR
	SAVE_DASICS
	// now, we enter kernel, clear CSR_DUMCFG again
	csrw CSR_DUMCFG, zero
_save_end:	
	.endm

	.macro RESTORE_DASICS

	/* Restore dasics main context */
	/* dumaincfg is saved previously */
	/* wh change, we save it to avoid metaphysics */
	REG_L t0, PT_DUMCFG(sp)
	csrw CSR_DUMCFG, t0
	REG_L t1, PT_DUMBOUNDLO(sp)
	csrw CSR_DUMBOUNDLO, t1
	REG_L t0, PT_DUMBOUNDHI(sp)
	csrw CSR_DUMBOUNDHI, t0


	REG_L t0, PT_DLCFG0(sp)
	csrw CSR_DLCFG0, t0

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

	REG_L t0, PT_DJBOUND0LO(sp)
	REG_L t1, PT_DJBOUND0HI(sp)
	csrw 	 CSR_DJBOUND0LO, t0
	csrw 	 CSR_DJBOUND0HI, t1
	REG_L t0, PT_DJBOUND1LO(sp)
	REG_L t1, PT_DJBOUND1HI(sp)
	csrw 	 CSR_DJBOUND1LO, t0
	csrw 	 CSR_DJBOUND1HI, t1
	REG_L t0, PT_DJBOUND2LO(sp)
	REG_L t1, PT_DJBOUND2HI(sp)
	csrw 	 CSR_DJBOUND2LO, t0
	csrw 	 CSR_DJBOUND2HI, t1
	REG_L t0, PT_DJBOUND3LO(sp)
	REG_L t1, PT_DJBOUND3HI(sp)
	csrw 	 CSR_DJBOUND3LO, t0
	csrw 	 CSR_DJBOUND3HI, t1

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

	REG_L a0, PT_SSTATUS(sp)
	REG_L a2, PT_SEPC(sp)
	csrw sstatus, a0
	csrw sepc, a2

_check_kernel:
	/* How can I know I handled an exception from kernel thread? */
	andi t0, a0, SR_SPP
	beqz t0, _check_dasics
	csrw CSR_DUMCFG, zero
	j _restore_general
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

ENTRY(handle_exception)
	sfence.vma
	SAVE_ALL

	/*
	 * Set sscratch register to 0, so that if a recursive exception
	 * occurs, the exception vector knows it came from the kernel
	 */
	csrw sscratch, x0

load_gp:
	/* Load the global pointer */
.option push
.option norelax
	la gp, __global_pointer$
.option pop

	la ra, ret_from_exception
	/*
	 * MSB of cause differentiates between
	 * interrupts and exceptions
	 */
	bge s4, zero, 1f

	/* Handle interrupts */
	move a0, sp /* pt_regs */
	move a1, s4 /* scause */
	tail do_IRQ
1:
	/* Exceptions run with interrupts enabled */
	csrs sstatus, SR_SIE

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
	 /* save the initial A0 value (needed in signal handlers) */
	REG_S a0, PT_ORIG_A0(sp)
	/*
	 * Advance SEPC to avoid executing the original
	 * scall instruction on sret
	 */
	addi s2, s2, 0x4
	REG_S s2, PT_SEPC(sp)
	/* Trace syscalls, but only if requested by the user. */
	REG_L t0, TASK_TI_FLAGS(tp)
	andi t0, t0, _TIF_SYSCALL_TRACE
	bnez t0, handle_syscall_trace_enter
check_syscall_nr:
	/* Check to make sure we don't jump to a bogus syscall number. */
	li t0, __NR_syscalls
	la s0, sys_ni_syscall
	/* Syscall number held in a7 */
	bgeu a7, t0, 1f
	la s0, sys_call_table
	slli t0, a7, RISCV_LGPTR
	add s0, s0, t0
	REG_L s0, 0(s0)
1:
	jalr s0

ret_from_syscall:
	/* Set user a0 to kernel a0 */
	REG_S a0, PT_A0(sp)
	/* Trace syscalls, but only if requested by the user. */
	REG_L t0, TASK_TI_FLAGS(tp)
	andi t0, t0, _TIF_SYSCALL_TRACE
	bnez t0, handle_syscall_trace_exit

ret_from_exception:
	REG_L s0, PT_SSTATUS(sp)
	csrc sstatus, SR_SIE
	andi s0, s0, SR_SPP
	bnez s0, restore_all 	/* previous is kernel state */

resume_userspace:
	/* Interrupts must be disabled here so flags are checked atomically */
	REG_L s0, TASK_TI_FLAGS(tp) /* current_thread_info->flags */
	andi s1, s0, _TIF_WORK_MASK
	bnez s1, work_pending

	/* Save unwound kernel stack pointer in thread_info */
	addi s0, sp, PT_SIZE_ON_STACK
	REG_S s0, TASK_TI_KERNEL_SP(tp)

	/*
	 * Save TP into sscratch, so we can find the kernel data structures
	 * again.
	 */
	csrw sscratch, tp

restore_all:
	RESTORE_ALL
	sret

work_pending:
	/* Enter slow path for supplementary processing */
	la ra, ret_from_exception
	andi s1, s0, _TIF_NEED_RESCHED
	bnez s1, work_resched
work_notifysig:
	/* Handle pending signals and notify-resume requests */
	csrs sstatus, SR_SIE /* Enable interrupts for do_notify_resume() */
	move a0, sp /* pt_regs */
	move a1, s0 /* current_thread_info->flags */
	tail do_notify_resume
work_resched:
	tail schedule

/* Slow paths for ptrace. */
handle_syscall_trace_enter:
	move a0, sp
	call do_syscall_trace_enter
	REG_L a0, PT_A0(sp)
	REG_L a1, PT_A1(sp)
	REG_L a2, PT_A2(sp)
	REG_L a3, PT_A3(sp)
	REG_L a4, PT_A4(sp)
	REG_L a5, PT_A5(sp)
	REG_L a6, PT_A6(sp)
	REG_L a7, PT_A7(sp)
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
#if TASK_TI != 0
#error "TASK_TI != 0: tp will contain a 'struct thread_info', not a 'struct task_struct' so get_current() won't work."
	addi tp, a1, TASK_TI
#else
	move tp, a1
#endif
	ret
ENDPROC(__switch_to)

ENTRY(__fstate_save)
	li  a2,  TASK_THREAD_F0
	add a0, a0, a2
	li t1, SR_FS
	csrs sstatus, t1
	frcsr t0
	fsd f0,  TASK_THREAD_F0_F0(a0)
	fsd f1,  TASK_THREAD_F1_F0(a0)
	fsd f2,  TASK_THREAD_F2_F0(a0)
	fsd f3,  TASK_THREAD_F3_F0(a0)
	fsd f4,  TASK_THREAD_F4_F0(a0)
	fsd f5,  TASK_THREAD_F5_F0(a0)
	fsd f6,  TASK_THREAD_F6_F0(a0)
	fsd f7,  TASK_THREAD_F7_F0(a0)
	fsd f8,  TASK_THREAD_F8_F0(a0)
	fsd f9,  TASK_THREAD_F9_F0(a0)
	fsd f10, TASK_THREAD_F10_F0(a0)
	fsd f11, TASK_THREAD_F11_F0(a0)
	fsd f12, TASK_THREAD_F12_F0(a0)
	fsd f13, TASK_THREAD_F13_F0(a0)
	fsd f14, TASK_THREAD_F14_F0(a0)
	fsd f15, TASK_THREAD_F15_F0(a0)
	fsd f16, TASK_THREAD_F16_F0(a0)
	fsd f17, TASK_THREAD_F17_F0(a0)
	fsd f18, TASK_THREAD_F18_F0(a0)
	fsd f19, TASK_THREAD_F19_F0(a0)
	fsd f20, TASK_THREAD_F20_F0(a0)
	fsd f21, TASK_THREAD_F21_F0(a0)
	fsd f22, TASK_THREAD_F22_F0(a0)
	fsd f23, TASK_THREAD_F23_F0(a0)
	fsd f24, TASK_THREAD_F24_F0(a0)
	fsd f25, TASK_THREAD_F25_F0(a0)
	fsd f26, TASK_THREAD_F26_F0(a0)
	fsd f27, TASK_THREAD_F27_F0(a0)
	fsd f28, TASK_THREAD_F28_F0(a0)
	fsd f29, TASK_THREAD_F29_F0(a0)
	fsd f30, TASK_THREAD_F30_F0(a0)
	fsd f31, TASK_THREAD_F31_F0(a0)
	sw t0, TASK_THREAD_FCSR_F0(a0)
	csrc sstatus, t1
	ret
ENDPROC(__fstate_save)

ENTRY(__fstate_restore)
	li  a2,  TASK_THREAD_F0
	add a0, a0, a2
	li t1, SR_FS
	lw t0, TASK_THREAD_FCSR_F0(a0)
	csrs sstatus, t1
	fld f0,  TASK_THREAD_F0_F0(a0)
	fld f1,  TASK_THREAD_F1_F0(a0)
	fld f2,  TASK_THREAD_F2_F0(a0)
	fld f3,  TASK_THREAD_F3_F0(a0)
	fld f4,  TASK_THREAD_F4_F0(a0)
	fld f5,  TASK_THREAD_F5_F0(a0)
	fld f6,  TASK_THREAD_F6_F0(a0)
	fld f7,  TASK_THREAD_F7_F0(a0)
	fld f8,  TASK_THREAD_F8_F0(a0)
	fld f9,  TASK_THREAD_F9_F0(a0)
	fld f10, TASK_THREAD_F10_F0(a0)
	fld f11, TASK_THREAD_F11_F0(a0)
	fld f12, TASK_THREAD_F12_F0(a0)
	fld f13, TASK_THREAD_F13_F0(a0)
	fld f14, TASK_THREAD_F14_F0(a0)
	fld f15, TASK_THREAD_F15_F0(a0)
	fld f16, TASK_THREAD_F16_F0(a0)
	fld f17, TASK_THREAD_F17_F0(a0)
	fld f18, TASK_THREAD_F18_F0(a0)
	fld f19, TASK_THREAD_F19_F0(a0)
	fld f20, TASK_THREAD_F20_F0(a0)
	fld f21, TASK_THREAD_F21_F0(a0)
	fld f22, TASK_THREAD_F22_F0(a0)
	fld f23, TASK_THREAD_F23_F0(a0)
	fld f24, TASK_THREAD_F24_F0(a0)
	fld f25, TASK_THREAD_F25_F0(a0)
	fld f26, TASK_THREAD_F26_F0(a0)
	fld f27, TASK_THREAD_F27_F0(a0)
	fld f28, TASK_THREAD_F28_F0(a0)
	fld f29, TASK_THREAD_F29_F0(a0)
	fld f30, TASK_THREAD_F30_F0(a0)
	fld f31, TASK_THREAD_F31_F0(a0)
	fscsr t0
	csrc sstatus, t1
	ret
ENDPROC(__fstate_restore)

	.section ".rodata"
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
