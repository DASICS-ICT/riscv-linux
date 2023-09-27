/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2015 Regents of the University of California
 */

#ifndef _ASM_RISCV_LINKAGE_H
#define _ASM_RISCV_LINKAGE_H

#ifdef CONFIG_64BIT
#define __ALIGN     .balign 8
#define __ALIGN_STR ".balign 8"
#else
#define __ALIGN		.balign 4
#define __ALIGN_STR	".balign 4"
#endif /* CONFIG_64BIT */

#endif /* _ASM_RISCV_LINKAGE_H */
