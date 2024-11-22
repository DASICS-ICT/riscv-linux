/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2015 Regents of the University of California
 */

#ifndef _ASM_RISCV_CSR_H
#define _ASM_RISCV_CSR_H

#include <asm/asm.h>
#include <linux/const.h>

/* Status register flags */
#define SR_SIE		_AC(0x00000002, UL) /* Supervisor Interrupt Enable */
#define SR_MIE		_AC(0x00000008, UL) /* Machine Interrupt Enable */
#define SR_SPIE		_AC(0x00000020, UL) /* Previous Supervisor IE */
#define SR_MPIE		_AC(0x00000080, UL) /* Previous Machine IE */
#define SR_SPP		_AC(0x00000100, UL) /* Previously Supervisor */
#define SR_MPP		_AC(0x00001800, UL) /* Previously Machine */
#define SR_SUM		_AC(0x00040000, UL) /* Supervisor User Memory Access */

/* Ustatus register flags */
#define UR_UIE	_AC(0x00000001, UL) /* User Interrupt Enable */
#define UR_UPIE	_AC(0x00000010, UL) /* Previous User IE */


#define SR_FS		_AC(0x00006000, UL) /* Floating-point Status */
#define SR_FS_OFF	_AC(0x00000000, UL)
#define SR_FS_INITIAL	_AC(0x00002000, UL)
#define SR_FS_CLEAN	_AC(0x00004000, UL)
#define SR_FS_DIRTY	_AC(0x00006000, UL)

#define SR_XS		_AC(0x00018000, UL) /* Extension Status */
#define SR_XS_OFF	_AC(0x00000000, UL)
#define SR_XS_INITIAL	_AC(0x00008000, UL)
#define SR_XS_CLEAN	_AC(0x00010000, UL)
#define SR_XS_DIRTY	_AC(0x00018000, UL)

#ifndef CONFIG_64BIT
#define SR_SD		_AC(0x80000000, UL) /* FS/XS dirty */
#else
#define SR_SD		_AC(0x8000000000000000, UL) /* FS/XS dirty */
#endif

/* SATP flags */
#ifndef CONFIG_64BIT
#define SATP_PPN	_AC(0x003FFFFF, UL)
#define SATP_MODE_32	_AC(0x80000000, UL)
#define SATP_MODE	SATP_MODE_32
#else
#define SATP_PPN	_AC(0x00000FFFFFFFFFFF, UL)
#define SATP_MODE_39	_AC(0x8000000000000000, UL)
#define SATP_MODE	SATP_MODE_39
#endif

/* Exception cause high bit - is an interrupt if set */
#define CAUSE_IRQ_FLAG		(_AC(1, UL) << (__riscv_xlen - 1))

/* Interrupt causes (minus the high bit) */
#define IRQ_S_SOFT		1
#define IRQ_M_SOFT		3
#define IRQ_S_TIMER		5
#define IRQ_M_TIMER		7
#define IRQ_S_EXT		9
#define IRQ_M_EXT		11

/* Exception causes */
#define EXC_INST_MISALIGNED	0
#define EXC_INST_ACCESS		1
#define EXC_BREAKPOINT		3
#define EXC_LOAD_ACCESS		5
#define EXC_STORE_ACCESS	7
#define EXC_SYSCALL		8
#define EXC_INST_PAGE_FAULT	12
#define EXC_LOAD_PAGE_FAULT	13
#define EXC_STORE_PAGE_FAULT	15

/* PMP configuration */
#define PMP_R			0x01
#define PMP_W			0x02
#define PMP_X			0x04
#define PMP_A			0x18
#define PMP_A_TOR		0x08
#define PMP_A_NA4		0x10
#define PMP_A_NAPOT		0x18
#define PMP_L			0x80

/* symbolic CSR names: */
#define CSR_CYCLE		0xc00
#define CSR_TIME		0xc01
#define CSR_INSTRET		0xc02
#define CSR_CYCLEH		0xc80
#define CSR_TIMEH		0xc81
#define CSR_INSTRETH		0xc82

#define CSR_SSTATUS		0x100
#define CSR_SIE			0x104
#define CSR_STVEC		0x105
#define CSR_SCOUNTEREN		0x106
#define CSR_SSCRATCH		0x140
#define CSR_SEPC		0x141
#define CSR_SCAUSE		0x142
#define CSR_STVAL		0x143
#define CSR_SIP			0x144
#define CSR_SATP		0x180

#define CSR_MSTATUS		0x300
#define CSR_MISA		0x301
#define CSR_MIE			0x304
#define CSR_MTVEC		0x305
#define CSR_MSCRATCH		0x340
#define CSR_MEPC		0x341
#define CSR_MCAUSE		0x342
#define CSR_MTVAL		0x343
#define CSR_MIP			0x344
#define CSR_PMPCFG0		0x3a0
#define CSR_PMPADDR0		0x3b0
#define CSR_MHARTID		0xf14

#ifdef CONFIG_RISCV_M_MODE
# define CSR_STATUS	CSR_MSTATUS
# define CSR_IE		CSR_MIE
# define CSR_TVEC	CSR_MTVEC
# define CSR_SCRATCH	CSR_MSCRATCH
# define CSR_EPC	CSR_MEPC
# define CSR_CAUSE	CSR_MCAUSE
# define CSR_TVAL	CSR_MTVAL
# define CSR_IP		CSR_MIP

# define SR_IE		SR_MIE
# define SR_PIE		SR_MPIE
# define SR_PP		SR_MPP

# define RV_IRQ_SOFT		IRQ_M_SOFT
# define RV_IRQ_TIMER	IRQ_M_TIMER
# define RV_IRQ_EXT		IRQ_M_EXT
#else /* CONFIG_RISCV_M_MODE */
# define CSR_STATUS	CSR_SSTATUS
# define CSR_IE		CSR_SIE
# define CSR_TVEC	CSR_STVEC
# define CSR_SCRATCH	CSR_SSCRATCH
# define CSR_EPC	CSR_SEPC
# define CSR_CAUSE	CSR_SCAUSE
# define CSR_TVAL	CSR_STVAL
# define CSR_IP		CSR_SIP

# define SR_IE		SR_SIE
# define SR_PIE		SR_SPIE
# define SR_PP		SR_SPP

# define RV_IRQ_SOFT		IRQ_S_SOFT
# define RV_IRQ_TIMER	IRQ_S_TIMER
# define RV_IRQ_EXT		IRQ_S_EXT
#endif /* CONFIG_RISCV_M_MODE */

/* IE/IP (Supervisor/Machine Interrupt Enable/Pending) flags */
#define IE_SIE		(_AC(0x1, UL) << RV_IRQ_SOFT)
#define IE_TIE		(_AC(0x1, UL) << RV_IRQ_TIMER)
#define IE_EIE		(_AC(0x1, UL) << RV_IRQ_EXT)

/* Add dasics exceptions */
#define EXC_DASICS_UCHECK_FAULT   24
#define EXC_DASICS_SCHECK_FAULT   25
#endif /* CONFIG_DASICS */

/* U state csrs */
#define CSR_USTATUS         0x000
#define CSR_UIE             0x004
#define CSR_UTVEC           0x005
#define CSR_USCRATCH        0x040
#define CSR_UEPC            0x041
#define CSR_UCAUSE          0x042
#define CSR_UTVAL           0x043
#define CSR_UIP             0x044
#define CSR_UTIMER          0x045

#ifdef CONFIG_DASICS
/* DASICS csrs */
#define CSR_DUMBOUND        0x9e1
#define CSR_DSMBOUND        0xbc1
/* DASICS Lib csrs */
#define CSR_DMBOUND0      0x890
#define CSR_DMBOUND1      0x891
#define CSR_DMBOUND2      0x892
#define CSR_DMBOUND3      0x893
#define CSR_DMBOUND4      0x894
#define CSR_DMBOUND5      0x895
#define CSR_DMBOUND6      0x896
#define CSR_DMBOUND7      0x897
#define CSR_DMBOUND8      0x898
#define CSR_DMBOUND9      0x899
#define CSR_DMBOUND10     0x89a
#define CSR_DMBOUND11     0x89b
#define CSR_DMBOUND12     0x89c
#define CSR_DMBOUND13     0x89d
#define CSR_DMBOUND14     0x89e
#define CSR_DMBOUND15     0x89f
#define CSR_DMBOUND16     0x8a0
#define CSR_DMBOUND17     0x8a1
#define CSR_DMBOUND18     0x8a2
#define CSR_DMBOUND19     0x8a3
#define CSR_DMBOUND20     0x8a4
#define CSR_DMBOUND21     0x8a5
#define CSR_DMBOUND22     0x8a6
#define CSR_DMBOUND23     0x8a7
#define CSR_DMBOUND24     0x8a8
#define CSR_DMBOUND25     0x8a9
#define CSR_DMBOUND26     0x8aa
#define CSR_DMBOUND27     0x8ab
#define CSR_DMBOUND28     0x8ac
#define CSR_DMBOUND29     0x8ad
#define CSR_DMBOUND30     0x8ae
#define CSR_DMBOUND31     0x8af

#define CSR_DMAINCALL       0x8b0
#define CSR_DRETURNPC       0x8b1
#define CSR_DFZRETURN       0x8b2
#define CSR_DFREASON        0x8b3

#define CSR_DJBOUND0      0x8c0
#define CSR_DJBOUND1      0x8c1
#define CSR_DJBOUND2      0x8c2
#define CSR_DJBOUND3      0x8c3
#define CSR_DJBOUND4      0x8c4
#define CSR_DJBOUND5      0x8c5
#define CSR_DJBOUND6      0x8c6
#define CSR_DJBOUND7      0x8c7

/* DASICS Main cfg */
#define DASICS_MAINCFG_MASK    0xfUL
#define DASICS_MAINCFG_ECALLF  0x8UL
#define DASICS_MAINCFG_JUMPF   0x4UL
#define DASICS_MAINCFG_LOADF   0x2UL
#define DASICS_MAINCFG_STOREF  0x1UL

/* DASICS Mem cfg */
#define DASICS_MEMCFG_WIDTH 32
#define DASICS_MEMCFG_MASK  0xfUL
#define DASICS_MEMCFG_V     0x8UL
#define DASICS_MEMCFG_U     0x4UL
#define DASICS_MEMCFG_R     0x2UL
#define DASICS_MEMCFG_W     0x1UL

/* DASICS Jmp cfg */
#define DASICS_JMPCFG_WIDTH 8
#define DASICS_JMPCFG_V    	0x8UL

#ifndef __ASSEMBLY__

#define csr_swap(csr, val)					\
({								\
	unsigned long __v = (unsigned long)(val);		\
	__asm__ __volatile__ ("csrrw %0, " __ASM_STR(csr) ", %1"\
			      : "=r" (__v) : "rK" (__v)		\
			      : "memory");			\
	__v;							\
})

#define csr_read(csr)						\
({								\
	register unsigned long __v;				\
	__asm__ __volatile__ ("csrr %0, " __ASM_STR(csr)	\
			      : "=r" (__v) :			\
			      : "memory");			\
	__v;							\
})

#define csr_write(csr, val)					\
({								\
	unsigned long __v = (unsigned long)(val);		\
	__asm__ __volatile__ ("csrw " __ASM_STR(csr) ", %0"	\
			      : : "rK" (__v)			\
			      : "memory");			\
})

#define csr_read_set(csr, val)					\
({								\
	unsigned long __v = (unsigned long)(val);		\
	__asm__ __volatile__ ("csrrs %0, " __ASM_STR(csr) ", %1"\
			      : "=r" (__v) : "rK" (__v)		\
			      : "memory");			\
	__v;							\
})

#define csr_set(csr, val)					\
({								\
	unsigned long __v = (unsigned long)(val);		\
	__asm__ __volatile__ ("csrs " __ASM_STR(csr) ", %0"	\
			      : : "rK" (__v)			\
			      : "memory");			\
})

#define csr_read_clear(csr, val)				\
({								\
	unsigned long __v = (unsigned long)(val);		\
	__asm__ __volatile__ ("csrrc %0, " __ASM_STR(csr) ", %1"\
			      : "=r" (__v) : "rK" (__v)		\
			      : "memory");			\
	__v;							\
})

#define csr_clear(csr, val)					\
({								\
	unsigned long __v = (unsigned long)(val);		\
	__asm__ __volatile__ ("csrc " __ASM_STR(csr) ", %0"	\
			      : : "rK" (__v)			\
			      : "memory");			\
})

#endif /* __ASSEMBLY__ */

#endif /* _ASM_RISCV_CSR_H */
