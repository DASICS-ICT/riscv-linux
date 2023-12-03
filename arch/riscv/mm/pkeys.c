#include <linux/mm_types.h>
#include <linux/pkeys.h>
#include <linux/mman.h>
#include <linux/printk.h>
#include <asm/pkru.h>

int pkey_initialize(void)
{
    enable_pkru();
    pr_notice("Protection Key for Userspace enabled\n");
    return 0;
}
arch_initcall(pkey_initialize);

static inline bool vma_is_pkey_exec_only(struct vm_area_struct *vma)
{
    /* Do this check first since the vm_flags should be hot */
    if ((vma->vm_flags & (VM_READ | VM_WRITE | VM_EXEC)) != VM_EXEC)
        return false;
    if (vma_pkey(vma) != vma->vm_mm->context.execute_only_pkey)
        return false;

    return true;
}

int __execute_only_pkey(struct mm_struct *mm)
{
    bool need_to_set_mm_pkey = false;
    int execute_only_pkey = mm->context.execute_only_pkey;
    int ret;

    /* Do we need to assign a pkey for mm's execute-only maps? */
    if (execute_only_pkey == -1) {
        /* Go allocate one to use, which might fail */
        execute_only_pkey = mm_pkey_alloc(mm);
        if (execute_only_pkey < 0)
            return -1;
        need_to_set_mm_pkey = true;
    }

    /*
     * We do not want to go through the relatively costly
     * dance to set PKRU if we do not need to.  Check it
     * first and assume that if the execute-only pkey is
     * write-disabled that we do not have to set it
     * ourselves.
     */
    if (!need_to_set_mm_pkey && !pkru_allows_read(execute_only_pkey)) {
        return execute_only_pkey;
    }

    /*
     * Set up PKRU so that it denies access for everything
     * other than execution.
     */
    ret = arch_set_user_pkey_access(current, execute_only_pkey,
            PKEY_DISABLE_ACCESS);

    /*
     * If the PKRU-set operation failed somehow, just return
     * 0 and effectively disable execute-only support.
     */
    if (ret) {
        mm_set_pkey_free(mm, execute_only_pkey);
        return -1;
    }

    /* We got one, store it and use it from here on out */
    if (need_to_set_mm_pkey)
        mm->context.execute_only_pkey = execute_only_pkey;
    return execute_only_pkey;
}

/*
 * This will go out and modify CSR_UPKRU register to set the access
 * rights for @pkey to @init_val.
 */
int arch_set_user_pkey_access(struct task_struct *tsk, int pkey,
        unsigned long init_val)
{
    u64 old_pkru, new_pkru;
    int pkey_shift = (pkey * PKRU_BITS_PER_PKEY);
    u64 new_pkru_bits = 0;

    /* Checks whether PKU feature is enabled */
    if (!arch_pkeys_enabled())
        return -EINVAL;

    /* Set the bits we need in PKRU:  */
    if (init_val & PKEY_DISABLE_ACCESS)
        new_pkru_bits |= PKRU_AD_BIT;
    if (init_val & PKEY_DISABLE_WRITE)
        new_pkru_bits |= PKRU_WD_BIT;

    /* Shift the bits in to the correct place in PKRU for pkey: */
    new_pkru_bits <<= pkey_shift;

    /* Get old PKRU and mask off any old bits in place: */
    old_pkru = read_pkru();
    old_pkru &= ~((PKRU_AD_BIT|PKRU_WD_BIT) << pkey_shift);

    /* Write old part along with new part: */
    new_pkru = old_pkru | new_pkru_bits;
    write_pkru(new_pkru);
    task_pt_regs(tsk)->upkru = new_pkru;

    return 0;
}

/*
 * This is only called for *plain* mprotect calls.
 */
int __arch_override_mprotect_pkey(struct vm_area_struct *vma, int prot, int pkey)
{
    /*
     * Is this an mprotect_pkey() call?  If so, never
     * override the value that came from the user.
     */
    if (pkey != -1)
        return pkey;

    /*
     * The mapping is execute-only.  Go try to get the
     * execute-only protection key.  If we fail to do that,
     * fall through as if we do not have execute-only
     * support in this mm.
     */
    if (prot == PROT_EXEC) {
        pkey = execute_only_pkey(vma->vm_mm);
        if (pkey > 0)
            return pkey;
    } else if (vma_is_pkey_exec_only(vma)) {
        /*
         * Protections are *not* PROT_EXEC, but the mapping
         * is using the exec-only pkey.  This mapping was
         * PROT_EXEC and will no longer be.  Move back to
         * the default pkey.
         */
        return ARCH_DEFAULT_PKEY;
    }

    /*
     * This is a vanilla, non-pkey mprotect (or we failed to
     * setup execute-only), inherit the pkey from the VMA we
     * are working on.
     */
    return vma_pkey(vma);
}
