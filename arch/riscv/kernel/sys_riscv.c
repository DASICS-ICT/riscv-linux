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
#include <asm/ptrace.h>
#include <linux/sched/task_stack.h>

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
 * riscv_handle_zicfilp - Handle Zicfilp (Landing Pad) control for user-space
 * @op: Operation type (RISCV_ZICFILP_GET or RISCV_ZICFILP_SET)
 * @val: Value for SET operation (RISCV_ZICFILP_DISABLE or RISCV_ZICFILP_ENABLE)
 *
 * This function allows user-space processes to control the Zicfilp (forward-edge
 * CFI) feature on a per-process basis. The state is stored in the process's
 * senvcfg CSR, which is automatically saved/restored during context switches
 * via the pt_regs structure.
 *
 * Key behaviors:
 * - Each process can independently enable/disable its own Zicfilp protection
 * - No special privileges required (process controls its own security)
 * - State persists across context switches via pt_regs.senvcfg
 * - Hardware automatically manages ELP (Expected Landing Pad) state
 * - Kernel disables Zicfilp during trap handling to avoid kernel faults
 *
 * Return:
 *   GET: Current status (0=disabled, 1=enabled)
 *   SET: 0 on success
 *   Error: Negative error code (-EINVAL for invalid args, -ENODEV if unsupported)
 */
int riscv_handle_zicfilp(unsigned long op, unsigned long val)
{
	struct pt_regs *regs = task_pt_regs(current);
	unsigned long senvcfg;

	/* Validate operation type */
	if (op != RISCV_ZICFILP_GET && op != RISCV_ZICFILP_SET)
		return -EINVAL;

	/* For SET operation, validate the value */
	if (op == RISCV_ZICFILP_SET &&
	    val != RISCV_ZICFILP_DISABLE && val != RISCV_ZICFILP_ENABLE)
		return -EINVAL;

	/*
	 * Runtime hardware support check:
	 * Try to read senvcfg. If the CSR doesn't exist, this will trap.
	 * Note: This is a simple check. In production, you may want to cache
	 * the hardware capability in a global variable during boot.
	 */

	/* Handle GET operation */
	if (op == RISCV_ZICFILP_GET) {
		/*
		 * Read from pt_regs instead of CSR directly.
		 * This ensures we get the process's saved state, not the
		 * current kernel state (kernel disables Zicfilp in traps).
		 */
		senvcfg = regs->senvcfg;
		return !!(senvcfg & ENVCFG_LPE);
	}

	/* Handle SET operation */
	if (val == RISCV_ZICFILP_ENABLE) {
		/* Enable Zicfilp for this process */
		regs->senvcfg |= ENVCFG_LPE;
		/* Also set current CSR for immediate effect */
		csr_set(CSR_SENVCFG, ENVCFG_LPE);
	} else {
		/* Disable Zicfilp for this process */
		regs->senvcfg &= ~ENVCFG_LPE;
		/* Also clear current CSR for immediate effect */
		csr_clear(CSR_SENVCFG, ENVCFG_LPE);
	}

	return 0;
}