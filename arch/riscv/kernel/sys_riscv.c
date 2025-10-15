// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2012 Regents of the University of California
 * Copyright (C) 2014 Darius Rad <darius@bluespec.com>
 * Copyright (C) 2017 SiFive
 */

#include <linux/syscalls.h>
#include <linux/prctl.h>
#include <asm/unistd.h>
#include <asm/cacheflush.h>
#include <asm-generic/mman-common.h>
#include <asm/csr.h>

static long riscv_sys_mmap(unsigned long addr, unsigned long len,
			   unsigned long prot, unsigned long flags,
			   unsigned long fd, off_t offset,
			   unsigned long page_shift_offset)
{
	if (unlikely(offset & (~PAGE_MASK >> page_shift_offset)))
		return -EINVAL;

	return ksys_mmap_pgoff(addr, len, prot, flags, fd,
			       offset >> (PAGE_SHIFT - page_shift_offset));
}

#ifdef CONFIG_64BIT
SYSCALL_DEFINE6(mmap, unsigned long, addr, unsigned long, len,
	unsigned long, prot, unsigned long, flags,
	unsigned long, fd, off_t, offset)
{
	return riscv_sys_mmap(addr, len, prot, flags, fd, offset, 0);
}
#else
SYSCALL_DEFINE6(mmap2, unsigned long, addr, unsigned long, len,
	unsigned long, prot, unsigned long, flags,
	unsigned long, fd, off_t, offset)
{
	/*
	 * Note that the shift for mmap2 is constant (12),
	 * regardless of PAGE_SIZE
	 */
	return riscv_sys_mmap(addr, len, prot, flags, fd, offset, 12);
}
#endif /* !CONFIG_64BIT */

/*
 * Allows the instruction cache to be flushed from userspace.  Despite RISC-V
 * having a direct 'fence.i' instruction available to userspace (which we
 * can't trap!), that's not actually viable when running on Linux because the
 * kernel might schedule a process on another hart.  There is no way for
 * userspace to handle this without invoking the kernel (as it doesn't know the
 * thread->hart mappings), so we've defined a RISC-V specific system call to
 * flush the instruction cache.
 *
 * sys_riscv_flush_icache() is defined to flush the instruction cache over an
 * address range, with the flush applying to either all threads or just the
 * caller.  We don't currently do anything with the address range, that's just
 * in there for forwards compatibility.
 */
SYSCALL_DEFINE3(riscv_flush_icache, uintptr_t, start, uintptr_t, end,
	uintptr_t, flags)
{
	/* Check the reserved flags. */
	if (unlikely(flags & ~SYS_RISCV_FLUSH_ICACHE_ALL))
		return -EINVAL;

	flush_icache_mm(current->mm, flags & SYS_RISCV_FLUSH_ICACHE_LOCAL);

	return 0;
}

/**
 * Handle zicfilp feature operations for prctl
 * @param op: Operation type - RISCV_ZICFILP_GET: get status, RISCV_ZICFILP_SET: set status
 * @param val: When op=RISCV_ZICFILP_SET, this parameter specifies the value to set
 *            (RISCV_ZICFILP_DISABLE or RISCV_ZICFILP_ENABLE)
 *
 * @return On success: If get operation, returns current status (0 or 1); If set operation, returns 0
 *         On failure: Returns negative error code
 */
int riscv_handle_zicfilp(unsigned long op, unsigned long val)
{
    /* Validate operation type */
    if (op != RISCV_ZICFILP_GET && op != RISCV_ZICFILP_SET)
        return -EINVAL;
    
    /* For set operation, validate the value */
    if (op == RISCV_ZICFILP_SET && 
        val != RISCV_ZICFILP_DISABLE && val != RISCV_ZICFILP_ENABLE)
        return -EINVAL;
    
    /* Require admin privileges for set operation */
    if (op == RISCV_ZICFILP_SET && !capable(CAP_SYS_ADMIN))
        return -EPERM;
    
    /* Handle GET operation */
    if (op == RISCV_ZICFILP_GET) {
        /* Read the bit directly with optimized assembly */
        unsigned long reg = csr_read(CSR_SENVCFG);
        return !!(reg & ENVCFG_LPE);  // return 1 or 0
    } else {
        /* Handle SET operation */
        if (val == RISCV_ZICFILP_ENABLE) {
            csr_set(CSR_SENVCFG, ENVCFG_LPE);
        } else {
            csr_clear(CSR_SENVCFG, ENVCFG_LPE);
        }

        return 0;
    }
}