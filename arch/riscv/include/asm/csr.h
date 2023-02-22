/*
 * Copyright (C) 2015 Regents of the University of California
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

#ifndef _ASM_RISCV_CSR_H
#define _ASM_RISCV_CSR_H

#include <linux/const.h>

/* Status register flags */
#define SR_SIE	_AC(0x00000002, UL) /* Supervisor Interrupt Enable */
#define SR_SPIE	_AC(0x00000020, UL) /* Previous Supervisor IE */
#define SR_SPP	_AC(0x00000100, UL) /* Previously Supervisor */
#define SR_SUM	_AC(0x00040000, UL) /* Supervisor may access User Memory */

#define SR_FS           _AC(0x00006000, UL) /* Floating-point Status */
#define SR_FS_OFF       _AC(0x00000000, UL)
#define SR_FS_INITIAL   _AC(0x00002000, UL)
#define SR_FS_CLEAN     _AC(0x00004000, UL)
#define SR_FS_DIRTY     _AC(0x00006000, UL)

#define SR_XS           _AC(0x00018000, UL) /* Extension Status */
#define SR_XS_OFF       _AC(0x00000000, UL)
#define SR_XS_INITIAL   _AC(0x00008000, UL)
#define SR_XS_CLEAN     _AC(0x00010000, UL)
#define SR_XS_DIRTY     _AC(0x00018000, UL)

#ifndef CONFIG_64BIT
#define SR_SD   _AC(0x80000000, UL) /* FS/XS dirty */
#else
#define SR_SD   _AC(0x8000000000000000, UL) /* FS/XS dirty */
#endif

/* SATP flags */
#if __riscv_xlen == 32
#define SATP_PPN     _AC(0x003FFFFF, UL)
#define SATP_MODE_32 _AC(0x80000000, UL)
#define SATP_MODE    SATP_MODE_32
#else
#define SATP_PPN     _AC(0x00000FFFFFFFFFFF, UL)
#define SATP_MODE_39 _AC(0x8000000000000000, UL)
#define SATP_MODE    SATP_MODE_39
#endif

/* Interrupt Enable and Interrupt Pending flags */
#define SIE_SSIE _AC(0x00000002, UL) /* Software Interrupt Enable */
#define SIE_STIE _AC(0x00000020, UL) /* Timer Interrupt Enable */
#define SIE_SEIE _AC(0x00000200, UL) /* External Interrupt Enable */

/* Ustatus register flags */
#define UR_UIE	_AC(0x00000001, UL) /* User Interrupt Enable */
#define UR_UPIE	_AC(0x00000010, UL) /* Previous User IE */

#define EXC_INST_MISALIGNED     0
#define EXC_INST_ACCESS         1
#define EXC_BREAKPOINT          3
#define EXC_LOAD_ACCESS         5
#define EXC_STORE_ACCESS        7
#define EXC_SYSCALL             8
#define EXC_INST_PAGE_FAULT     12
#define EXC_LOAD_PAGE_FAULT     13
#define EXC_STORE_PAGE_FAULT    15

#ifdef CONFIG_DASICS 
/* Add dasics exceptions */
#define EXC_DASICS_UFETCH_FAULT   24
#define EXC_DASICS_SFETCH_FAULT   25
#define EXC_DASICS_ULOAD_FAULT   26
#define EXC_DASICS_SLOAD_FAULT   27
#define EXC_DASICS_USTORE_FAULT  28
#define EXC_DASICS_SSTORE_FAULT  29
#define EXC_DASICS_UECALL_FAULT  30
#define EXC_DASICS_SECALL_FAULT  31
#endif /* CONFIG_DASICS */

#define CSR_CYCLE           0xc00
#define CSR_TIME            0xc01
#define CSR_INSTRET         0xc02
#define CSR_SSTATUS         0x100
#define CSR_SIE             0x104
#define CSR_STVEC           0x105
#define CSR_SCOUNTEREN      0x106
#define CSR_SSCRATCH        0x140
#define CSR_SEPC            0x141
#define CSR_SCAUSE          0x142
#define CSR_STVAL           0x143
#define CSR_SIP             0x144
#define CSR_SATP            0x180
#define CSR_CYCLEH          0xc80
#define CSR_TIMEH           0xc81
#define CSR_INSTRETH        0xc82

/* U state csrs */
#define CSR_USTATUS         0x000
#define CSR_UIE             0x004
#define CSR_UTVEC           0x005
#define CSR_USCRATCH        0x040
#define CSR_UEPC            0x041
#define CSR_UCAUSE          0x042
#define CSR_UTVAL           0x043
#define CSR_UIP             0x044

#ifdef CONFIG_DASICS
/* DASICS Main csrs */
#define CSR_DUMCFG          0x5c0
#define CSR_DUMBOUNDHI      0x5c1
#define CSR_DUMBOUNDLO      0x5c2

/* DASICS Main cfg */
#define DASICS_MAINCFG_MASK 0xfUL
#define DASICS_UCFG_CLS     0x8UL
#define DASICS_SCFG_CLS     0x4UL
#define DASICS_UCFG_ENA     0x2UL
#define DASICS_SCFG_ENA     0x1UL

/* DASICS Lib csrs */
#define CSR_DLCFG0          0x881
#define CSR_DLCFG1          0x882

#define CSR_DLBOUND0        0x883
#define CSR_DLBOUND1        0x884
#define CSR_DLBOUND2        0x885
#define CSR_DLBOUND3        0x886
#define CSR_DLBOUND4        0x887
#define CSR_DLBOUND5        0x888
#define CSR_DLBOUND6        0x889
#define CSR_DLBOUND7        0x88a
#define CSR_DLBOUND8        0x88b
#define CSR_DLBOUND9        0x88c
#define CSR_DLBOUND10       0x88d
#define CSR_DLBOUND11       0x88e
#define CSR_DLBOUND12       0x88f
#define CSR_DLBOUND13       0x890
#define CSR_DLBOUND14       0x891
#define CSR_DLBOUND15       0x892
#define CSR_DLBOUND16       0x893
#define CSR_DLBOUND17       0x894
#define CSR_DLBOUND18       0x895
#define CSR_DLBOUND19       0x896
#define CSR_DLBOUND20       0x897
#define CSR_DLBOUND21       0x898
#define CSR_DLBOUND22       0x899
#define CSR_DLBOUND23       0x89a
#define CSR_DLBOUND24       0x89b
#define CSR_DLBOUND25       0x89c
#define CSR_DLBOUND26       0x89d
#define CSR_DLBOUND27       0x89e
#define CSR_DLBOUND28       0x89f
#define CSR_DLBOUND29       0x8a0
#define CSR_DLBOUND30       0x8a1
#define CSR_DLBOUND31       0x8a2

#define CSR_DMAINCALL       0x8a3
#define CSR_DRETURNPC       0x8a4
#define CSR_DFZRETURN       0x8a5

/* DASICS Lib cfg */
#define DASICS_LIBCFG_WIDTH 8
#define DASICS_LIBCFG_MASK  0xfUL
#define DASICS_LIBCFG_V     0x8UL
#define DASICS_LIBCFG_X     0x4UL
#define DASICS_LIBCFG_R     0x2UL
#define DASICS_LIBCFG_W     0x1UL

#endif /* CONFIG_DASICS */

#ifndef __ASSEMBLY__

#define csr_swap(csr, val)					\
({								\
	unsigned long __v = (unsigned long)(val);		\
	__asm__ __volatile__ ("csrrw %0, " #csr ", %1"		\
			      : "=r" (__v) : "rK" (__v)		\
			      : "memory");			\
	__v;							\
})

#define csr_read(csr)						\
({								\
	register unsigned long __v;				\
	__asm__ __volatile__ ("csrr %0, " #csr			\
			      : "=r" (__v) :			\
			      : "memory");			\
	__v;							\
})

#define csr_write(csr, val)					\
({								\
	unsigned long __v = (unsigned long)(val);		\
	__asm__ __volatile__ ("csrw " #csr ", %0"		\
			      : : "rK" (__v)			\
			      : "memory");			\
})

#define csr_read_set(csr, val)					\
({								\
	unsigned long __v = (unsigned long)(val);		\
	__asm__ __volatile__ ("csrrs %0, " #csr ", %1"		\
			      : "=r" (__v) : "rK" (__v)		\
			      : "memory");			\
	__v;							\
})

#define csr_set(csr, val)					\
({								\
	unsigned long __v = (unsigned long)(val);		\
	__asm__ __volatile__ ("csrs " #csr ", %0"		\
			      : : "rK" (__v)			\
			      : "memory");			\
})

#define csr_read_clear(csr, val)				\
({								\
	unsigned long __v = (unsigned long)(val);		\
	__asm__ __volatile__ ("csrrc %0, " #csr ", %1"		\
			      : "=r" (__v) : "rK" (__v)		\
			      : "memory");			\
	__v;							\
})

#define csr_clear(csr, val)					\
({								\
	unsigned long __v = (unsigned long)(val);		\
	__asm__ __volatile__ ("csrc " #csr ", %0"		\
			      : : "rK" (__v)			\
			      : "memory");			\
})

#endif /* __ASSEMBLY__ */

#endif /* _ASM_RISCV_CSR_H */
