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
