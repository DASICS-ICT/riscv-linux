/*
 * RISC-V Memory Protection Keys management
 *
 * Note:
 *  1. MPK for RV64 is based on three customized CSRs: UPKRU(0x800), SPKRS(0x9d1) and SPKCTL(0x9d0).
 *  2. The wd/ad layout of UPKRU/SPKRS is the same as PKRU/IA32_PKRS of x86_64 MPK design,
 *  but the high 32 bits of UPKRU/SPKRS are available, thus we can support up to 32 different pkeys.
 *  3. SPKCTL.PKE(bit 0)/SPKCTL.PKS(bit 1) is used to enable MPK mechanism for user mode or supervisor mode,
 *  which is the same as CR4.PKE/CR4.PKS.
 *  4. Five reserved bits of PTE (PTE[58:54]) are used as pkey, while pkey0 is still the default one.
 */

#ifndef _ASM_RISCV_PKEYS_H
#define _ASM_RISCV_PKEYS_H

#include <asm/pkru.h>

#define ARCH_DEFAULT_PKEY 0
#define ARCH_VM_PKEY_FLAGS (VM_PKEY_BIT0 | VM_PKEY_BIT1 | VM_PKEY_BIT2 | VM_PKEY_BIT3 | VM_PKEY_BIT4)

#define arch_max_pkey() (PKRU_NUM_PKEYS)

#define mm_pkey_allocation_map(mm)	(mm->context.pkey_allocation_map)
#define mm_set_pkey_allocated(mm, pkey) do {		\
	mm_pkey_allocation_map(mm) |= (1U << pkey);	\
} while (0)
#define mm_set_pkey_free(mm, pkey) do {			\
	mm_pkey_allocation_map(mm) &= ~(1U << pkey);	\
} while (0)

extern int arch_set_user_pkey_access(struct task_struct *tsk, int pkey,
		unsigned long init_val);

static inline bool arch_pkeys_enabled(void)
{
	return is_pkru_enabled();
}

static inline int vma_pkey(struct vm_area_struct *vma)
{
	unsigned long vma_pkey_mask = VM_PKEY_BIT0 | VM_PKEY_BIT1 |
				      VM_PKEY_BIT2 | VM_PKEY_BIT3 | VM_PKEY_BIT4;
	return (vma->vm_flags & vma_pkey_mask) >> VM_PKEY_SHIFT;
}

/*
 * Try to dedicate one of the protection keys to be used as an
 * execute-only protection key.
 */
extern int __execute_only_pkey(struct mm_struct *mm);
static inline int execute_only_pkey(struct mm_struct *mm)
{
	if (!arch_pkeys_enabled())
		return ARCH_DEFAULT_PKEY;

	return __execute_only_pkey(mm);
}

extern int __arch_override_mprotect_pkey(struct vm_area_struct *vma, int prot, int pkey);
static inline int arch_override_mprotect_pkey(struct vm_area_struct *vma, int prot, int pkey)
{
	if (!arch_pkeys_enabled())
		return 0;

	return __arch_override_mprotect_pkey(vma, prot, pkey);
}

static inline bool mm_pkey_is_allocated(struct mm_struct *mm, int pkey)
{
	/*
	 * "Allocated" pkeys are those that have been returned
	 * from pkey_alloc() which is allocated implicitly
	 * when the mm is created.
	 */
	if (pkey <= 0)
		return false;
	if (pkey >= arch_max_pkey())
		return false;
	/*
	 * The exec-only pkey is set in the allocation map, but
	 * is not available to any of the user interfaces like
	 * mprotect_pkey().
	 */
	if (pkey == mm->context.execute_only_pkey)
		return false;

	return mm_pkey_allocation_map(mm) & (1U << pkey);
}

/*
 * Returns a positive, 5-bit key on success, or -1 on failure.
 */
static inline int mm_pkey_alloc(struct mm_struct *mm)
{
	/*
	 * Note: this is the one and only place we make sure
	 * that the pkey is valid as far as the hardware is
	 * concerned.  The rest of the kernel trusts that
	 * only good, valid pkeys come out of here.
	 */
	u32 all_pkeys_mask = (u32)((1UL << arch_max_pkey()) - 1);
	int ret;

	/*
	 * Are we out of pkeys?  We must handle this specially
	 * because ffz() behavior is undefined if there are no
	 * zeros.
	 */
	if (mm_pkey_allocation_map(mm) == all_pkeys_mask)
		return -1;

	ret = ffz(mm_pkey_allocation_map(mm));

	mm_set_pkey_allocated(mm, ret);

	return ret;
}

static inline int mm_pkey_free(struct mm_struct *mm, int pkey)
{
	if (!mm_pkey_is_allocated(mm, pkey))
		return -EINVAL;

	mm_set_pkey_free(mm, pkey);

	return 0;
}

#endif  /* _ASM_RISCV_PKEYS_H */