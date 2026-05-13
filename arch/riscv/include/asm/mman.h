/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_RISCV_MMAN_H
#define __ASM_RISCV_MMAN_H

#include <uapi/asm/mman.h>

#ifndef BUILD_VDSO
#include <linux/mm.h>
#include <linux/types.h>
#include <asm/hwcap.h>

#ifdef CONFIG_RISCV_ISA_ZIMT
static inline vm_flags_t arch_calc_vm_prot_bits(unsigned long prot,
						unsigned long pkey __always_unused)
{
	if (riscv_has_extension_unlikely(RISCV_ISA_EXT_ZIMT) && (prot & PROT_ZIMT))
		return VM_ARCH_1;
	return 0;
}
#define arch_calc_vm_prot_bits(prot, pkey) arch_calc_vm_prot_bits(prot, pkey)

static inline bool arch_validate_prot(unsigned long prot,
				      unsigned long addr __always_unused)
{
	unsigned long supported = PROT_READ | PROT_WRITE | PROT_EXEC | PROT_SEM;

	if (riscv_has_extension_unlikely(RISCV_ISA_EXT_ZIMT))
		supported |= PROT_ZIMT;

	return (prot & ~supported) == 0;
}
#define arch_validate_prot(prot, addr) arch_validate_prot(prot, addr)
#endif

#endif /* !BUILD_VDSO */

#endif /* __ASM_RISCV_MMAN_H */
