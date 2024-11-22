// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2012 Regents of the University of California
 * Copyright (C) 2017 SiFive
 */

#define GENERATING_ASM_OFFSETS

#include <linux/kbuild.h>
#include <linux/sched.h>
#include <asm/thread_info.h>
#include <asm/ptrace.h>

void asm_offsets(void)
{
	OFFSET(TASK_THREAD_RA, task_struct, thread.ra);
	OFFSET(TASK_THREAD_SP, task_struct, thread.sp);
	OFFSET(TASK_THREAD_S0, task_struct, thread.s[0]);
	OFFSET(TASK_THREAD_S1, task_struct, thread.s[1]);
	OFFSET(TASK_THREAD_S2, task_struct, thread.s[2]);
	OFFSET(TASK_THREAD_S3, task_struct, thread.s[3]);
	OFFSET(TASK_THREAD_S4, task_struct, thread.s[4]);
	OFFSET(TASK_THREAD_S5, task_struct, thread.s[5]);
	OFFSET(TASK_THREAD_S6, task_struct, thread.s[6]);
	OFFSET(TASK_THREAD_S7, task_struct, thread.s[7]);
	OFFSET(TASK_THREAD_S8, task_struct, thread.s[8]);
	OFFSET(TASK_THREAD_S9, task_struct, thread.s[9]);
	OFFSET(TASK_THREAD_S10, task_struct, thread.s[10]);
	OFFSET(TASK_THREAD_S11, task_struct, thread.s[11]);
	OFFSET(TASK_TI_FLAGS, task_struct, thread_info.flags);
	OFFSET(TASK_TI_PREEMPT_COUNT, task_struct, thread_info.preempt_count);
	OFFSET(TASK_TI_KERNEL_SP, task_struct, thread_info.kernel_sp);
	OFFSET(TASK_TI_USER_SP, task_struct, thread_info.user_sp);
	OFFSET(TASK_TI_CPU, task_struct, thread_info.cpu);

	OFFSET(TASK_THREAD_F0,  task_struct, thread.fstate.f[0]);
	OFFSET(TASK_THREAD_F1,  task_struct, thread.fstate.f[1]);
	OFFSET(TASK_THREAD_F2,  task_struct, thread.fstate.f[2]);
	OFFSET(TASK_THREAD_F3,  task_struct, thread.fstate.f[3]);
	OFFSET(TASK_THREAD_F4,  task_struct, thread.fstate.f[4]);
	OFFSET(TASK_THREAD_F5,  task_struct, thread.fstate.f[5]);
	OFFSET(TASK_THREAD_F6,  task_struct, thread.fstate.f[6]);
	OFFSET(TASK_THREAD_F7,  task_struct, thread.fstate.f[7]);
	OFFSET(TASK_THREAD_F8,  task_struct, thread.fstate.f[8]);
	OFFSET(TASK_THREAD_F9,  task_struct, thread.fstate.f[9]);
	OFFSET(TASK_THREAD_F10, task_struct, thread.fstate.f[10]);
	OFFSET(TASK_THREAD_F11, task_struct, thread.fstate.f[11]);
	OFFSET(TASK_THREAD_F12, task_struct, thread.fstate.f[12]);
	OFFSET(TASK_THREAD_F13, task_struct, thread.fstate.f[13]);
	OFFSET(TASK_THREAD_F14, task_struct, thread.fstate.f[14]);
	OFFSET(TASK_THREAD_F15, task_struct, thread.fstate.f[15]);
	OFFSET(TASK_THREAD_F16, task_struct, thread.fstate.f[16]);
	OFFSET(TASK_THREAD_F17, task_struct, thread.fstate.f[17]);
	OFFSET(TASK_THREAD_F18, task_struct, thread.fstate.f[18]);
	OFFSET(TASK_THREAD_F19, task_struct, thread.fstate.f[19]);
	OFFSET(TASK_THREAD_F20, task_struct, thread.fstate.f[20]);
	OFFSET(TASK_THREAD_F21, task_struct, thread.fstate.f[21]);
	OFFSET(TASK_THREAD_F22, task_struct, thread.fstate.f[22]);
	OFFSET(TASK_THREAD_F23, task_struct, thread.fstate.f[23]);
	OFFSET(TASK_THREAD_F24, task_struct, thread.fstate.f[24]);
	OFFSET(TASK_THREAD_F25, task_struct, thread.fstate.f[25]);
	OFFSET(TASK_THREAD_F26, task_struct, thread.fstate.f[26]);
	OFFSET(TASK_THREAD_F27, task_struct, thread.fstate.f[27]);
	OFFSET(TASK_THREAD_F28, task_struct, thread.fstate.f[28]);
	OFFSET(TASK_THREAD_F29, task_struct, thread.fstate.f[29]);
	OFFSET(TASK_THREAD_F30, task_struct, thread.fstate.f[30]);
	OFFSET(TASK_THREAD_F31, task_struct, thread.fstate.f[31]);
	OFFSET(TASK_THREAD_FCSR, task_struct, thread.fstate.fcsr);

	DEFINE(PT_SIZE, sizeof(struct pt_regs));
	OFFSET(PT_EPC, pt_regs, epc);
	OFFSET(PT_RA, pt_regs, ra);
	OFFSET(PT_FP, pt_regs, s0);
	OFFSET(PT_S0, pt_regs, s0);
	OFFSET(PT_S1, pt_regs, s1);
	OFFSET(PT_S2, pt_regs, s2);
	OFFSET(PT_S3, pt_regs, s3);
	OFFSET(PT_S4, pt_regs, s4);
	OFFSET(PT_S5, pt_regs, s5);
	OFFSET(PT_S6, pt_regs, s6);
	OFFSET(PT_S7, pt_regs, s7);
	OFFSET(PT_S8, pt_regs, s8);
	OFFSET(PT_S9, pt_regs, s9);
	OFFSET(PT_S10, pt_regs, s10);
	OFFSET(PT_S11, pt_regs, s11);
	OFFSET(PT_SP, pt_regs, sp);
	OFFSET(PT_TP, pt_regs, tp);
	OFFSET(PT_A0, pt_regs, a0);
	OFFSET(PT_A1, pt_regs, a1);
	OFFSET(PT_A2, pt_regs, a2);
	OFFSET(PT_A3, pt_regs, a3);
	OFFSET(PT_A4, pt_regs, a4);
	OFFSET(PT_A5, pt_regs, a5);
	OFFSET(PT_A6, pt_regs, a6);
	OFFSET(PT_A7, pt_regs, a7);
	OFFSET(PT_T0, pt_regs, t0);
	OFFSET(PT_T1, pt_regs, t1);
	OFFSET(PT_T2, pt_regs, t2);
	OFFSET(PT_T3, pt_regs, t3);
	OFFSET(PT_T4, pt_regs, t4);
	OFFSET(PT_T5, pt_regs, t5);
	OFFSET(PT_T6, pt_regs, t6);
	OFFSET(PT_GP, pt_regs, gp);
	OFFSET(PT_ORIG_A0, pt_regs, orig_a0);
	OFFSET(PT_STATUS, pt_regs, status);
	OFFSET(PT_BADADDR, pt_regs, badaddr);
	OFFSET(PT_CAUSE, pt_regs, cause);

	/* N extension user registers */
	OFFSET(PT_USTATUS, pt_regs, ustatus);
	OFFSET(PT_UEPC, pt_regs, uepc);
	OFFSET(PT_UBADADDR, pt_regs, ubadaddr);
	OFFSET(PT_UCAUSE, pt_regs, ucause);
	OFFSET(PT_UTVEC, pt_regs, utvec);
	OFFSET(PT_UIE, pt_regs, uie);
	OFFSET(PT_UIP, pt_regs, uip);
	OFFSET(PT_USCRATCH, pt_regs, uscratch);
	OFFSET(PT_UTIMER, pt_regs, utimer);

#ifdef CONFIG_DASICS
	/* dasics supervisor registers */
	OFFSET(PT_DUMBOUND, pt_regs, dasicsUMainBound);

	OFFSET(PT_DMBOUND0, pt_regs, dasicsMemBounds[0]);
	OFFSET(PT_DMBOUND1, pt_regs, dasicsMemBounds[1]);
	OFFSET(PT_DMBOUND2, pt_regs, dasicsMemBounds[2]);
	OFFSET(PT_DMBOUND3, pt_regs, dasicsMemBounds[3]);
	OFFSET(PT_DMBOUND4, pt_regs, dasicsMemBounds[4]);
	OFFSET(PT_DMBOUND5, pt_regs, dasicsMemBounds[5]);
	OFFSET(PT_DMBOUND6, pt_regs, dasicsMemBounds[6]);
	OFFSET(PT_DMBOUND7, pt_regs, dasicsMemBounds[7]);
	OFFSET(PT_DMBOUND8, pt_regs, dasicsMemBounds[8]);
	OFFSET(PT_DMBOUND9, pt_regs, dasicsMemBounds[9]);
	OFFSET(PT_DMBOUND10, pt_regs, dasicsMemBounds[10]);
	OFFSET(PT_DMBOUND11, pt_regs, dasicsMemBounds[11]);
	OFFSET(PT_DMBOUND12, pt_regs, dasicsMemBounds[12]);
	OFFSET(PT_DMBOUND13, pt_regs, dasicsMemBounds[13]);
	OFFSET(PT_DMBOUND14, pt_regs, dasicsMemBounds[14]);
	OFFSET(PT_DMBOUND15, pt_regs, dasicsMemBounds[15]);
	OFFSET(PT_DMBOUND16, pt_regs, dasicsMemBounds[16]);
	OFFSET(PT_DMBOUND17, pt_regs, dasicsMemBounds[17]);
	OFFSET(PT_DMBOUND18, pt_regs, dasicsMemBounds[18]);
	OFFSET(PT_DMBOUND19, pt_regs, dasicsMemBounds[19]);
	OFFSET(PT_DMBOUND20, pt_regs, dasicsMemBounds[20]);
	OFFSET(PT_DMBOUND21, pt_regs, dasicsMemBounds[21]);
	OFFSET(PT_DMBOUND22, pt_regs, dasicsMemBounds[22]);
	OFFSET(PT_DMBOUND23, pt_regs, dasicsMemBounds[23]);
	OFFSET(PT_DMBOUND24, pt_regs, dasicsMemBounds[24]);
	OFFSET(PT_DMBOUND25, pt_regs, dasicsMemBounds[25]);
	OFFSET(PT_DMBOUND26, pt_regs, dasicsMemBounds[26]);
	OFFSET(PT_DMBOUND27, pt_regs, dasicsMemBounds[27]);
	OFFSET(PT_DMBOUND28, pt_regs, dasicsMemBounds[28]);
	OFFSET(PT_DMBOUND29, pt_regs, dasicsMemBounds[29]);
	OFFSET(PT_DMBOUND30, pt_regs, dasicsMemBounds[30]);
	OFFSET(PT_DMBOUND31, pt_regs, dasicsMemBounds[31]);

	OFFSET(PT_DMAINCALL, pt_regs, dasicsMaincall);
	OFFSET(PT_DRETURNPC, pt_regs, dasicsReturnPC);
	OFFSET(PT_DFZRETURN, pt_regs, dasicsFreezoneRet);
	OFFSET(PT_DFREASON, pt_regs, dasicsFaultReason);

	OFFSET(PT_DJBOUND0, pt_regs, dasicsJmpBounds[0]);
	OFFSET(PT_DJBOUND1, pt_regs, dasicsJmpBounds[1]);
	OFFSET(PT_DJBOUND2, pt_regs, dasicsJmpBounds[2]);
	OFFSET(PT_DJBOUND3, pt_regs, dasicsJmpBounds[3]);
	OFFSET(PT_DJBOUND4, pt_regs, dasicsJmpBounds[4]);
	OFFSET(PT_DJBOUND5, pt_regs, dasicsJmpBounds[5]);
	OFFSET(PT_DJBOUND6, pt_regs, dasicsJmpBounds[6]);
	OFFSET(PT_DJBOUND7, pt_regs, dasicsJmpBounds[7]);

#endif 

	/*
	 * THREAD_{F,X}* might be larger than a S-type offset can handle, but
	 * these are used in performance-sensitive assembly so we can't resort
	 * to loading the long immediate every time.
	 */
	DEFINE(TASK_THREAD_RA_RA,
		  offsetof(struct task_struct, thread.ra)
		- offsetof(struct task_struct, thread.ra)
	);
	DEFINE(TASK_THREAD_SP_RA,
		  offsetof(struct task_struct, thread.sp)
		- offsetof(struct task_struct, thread.ra)
	);
	DEFINE(TASK_THREAD_S0_RA,
		  offsetof(struct task_struct, thread.s[0])
		- offsetof(struct task_struct, thread.ra)
	);
	DEFINE(TASK_THREAD_S1_RA,
		  offsetof(struct task_struct, thread.s[1])
		- offsetof(struct task_struct, thread.ra)
	);
	DEFINE(TASK_THREAD_S2_RA,
		  offsetof(struct task_struct, thread.s[2])
		- offsetof(struct task_struct, thread.ra)
	);
	DEFINE(TASK_THREAD_S3_RA,
		  offsetof(struct task_struct, thread.s[3])
		- offsetof(struct task_struct, thread.ra)
	);
	DEFINE(TASK_THREAD_S4_RA,
		  offsetof(struct task_struct, thread.s[4])
		- offsetof(struct task_struct, thread.ra)
	);
	DEFINE(TASK_THREAD_S5_RA,
		  offsetof(struct task_struct, thread.s[5])
		- offsetof(struct task_struct, thread.ra)
	);
	DEFINE(TASK_THREAD_S6_RA,
		  offsetof(struct task_struct, thread.s[6])
		- offsetof(struct task_struct, thread.ra)
	);
	DEFINE(TASK_THREAD_S7_RA,
		  offsetof(struct task_struct, thread.s[7])
		- offsetof(struct task_struct, thread.ra)
	);
	DEFINE(TASK_THREAD_S8_RA,
		  offsetof(struct task_struct, thread.s[8])
		- offsetof(struct task_struct, thread.ra)
	);
	DEFINE(TASK_THREAD_S9_RA,
		  offsetof(struct task_struct, thread.s[9])
		- offsetof(struct task_struct, thread.ra)
	);
	DEFINE(TASK_THREAD_S10_RA,
		  offsetof(struct task_struct, thread.s[10])
		- offsetof(struct task_struct, thread.ra)
	);
	DEFINE(TASK_THREAD_S11_RA,
		  offsetof(struct task_struct, thread.s[11])
		- offsetof(struct task_struct, thread.ra)
	);

	DEFINE(TASK_THREAD_F0_F0,
		  offsetof(struct task_struct, thread.fstate.f[0])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F1_F0,
		  offsetof(struct task_struct, thread.fstate.f[1])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F2_F0,
		  offsetof(struct task_struct, thread.fstate.f[2])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F3_F0,
		  offsetof(struct task_struct, thread.fstate.f[3])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F4_F0,
		  offsetof(struct task_struct, thread.fstate.f[4])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F5_F0,
		  offsetof(struct task_struct, thread.fstate.f[5])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F6_F0,
		  offsetof(struct task_struct, thread.fstate.f[6])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F7_F0,
		  offsetof(struct task_struct, thread.fstate.f[7])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F8_F0,
		  offsetof(struct task_struct, thread.fstate.f[8])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F9_F0,
		  offsetof(struct task_struct, thread.fstate.f[9])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F10_F0,
		  offsetof(struct task_struct, thread.fstate.f[10])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F11_F0,
		  offsetof(struct task_struct, thread.fstate.f[11])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F12_F0,
		  offsetof(struct task_struct, thread.fstate.f[12])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F13_F0,
		  offsetof(struct task_struct, thread.fstate.f[13])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F14_F0,
		  offsetof(struct task_struct, thread.fstate.f[14])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F15_F0,
		  offsetof(struct task_struct, thread.fstate.f[15])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F16_F0,
		  offsetof(struct task_struct, thread.fstate.f[16])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F17_F0,
		  offsetof(struct task_struct, thread.fstate.f[17])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F18_F0,
		  offsetof(struct task_struct, thread.fstate.f[18])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F19_F0,
		  offsetof(struct task_struct, thread.fstate.f[19])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F20_F0,
		  offsetof(struct task_struct, thread.fstate.f[20])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F21_F0,
		  offsetof(struct task_struct, thread.fstate.f[21])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F22_F0,
		  offsetof(struct task_struct, thread.fstate.f[22])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F23_F0,
		  offsetof(struct task_struct, thread.fstate.f[23])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F24_F0,
		  offsetof(struct task_struct, thread.fstate.f[24])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F25_F0,
		  offsetof(struct task_struct, thread.fstate.f[25])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F26_F0,
		  offsetof(struct task_struct, thread.fstate.f[26])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F27_F0,
		  offsetof(struct task_struct, thread.fstate.f[27])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F28_F0,
		  offsetof(struct task_struct, thread.fstate.f[28])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F29_F0,
		  offsetof(struct task_struct, thread.fstate.f[29])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F30_F0,
		  offsetof(struct task_struct, thread.fstate.f[30])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_F31_F0,
		  offsetof(struct task_struct, thread.fstate.f[31])
		- offsetof(struct task_struct, thread.fstate.f[0])
	);
	DEFINE(TASK_THREAD_FCSR_F0,
		  offsetof(struct task_struct, thread.fstate.fcsr)
		- offsetof(struct task_struct, thread.fstate.f[0])
	);

	/*
	 * We allocate a pt_regs on the stack when entering the kernel.  This
	 * ensures the alignment is sane.
	 */
	DEFINE(PT_SIZE_ON_STACK, ALIGN(sizeof(struct pt_regs), STACK_ALIGN));
}
