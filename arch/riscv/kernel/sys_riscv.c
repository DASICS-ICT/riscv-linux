// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2012 Regents of the University of California
 * Copyright (C) 2014 Darius Rad <darius@bluespec.com>
 * Copyright (C) 2017 SiFive
 */

#include <linux/syscalls.h>
#include <asm/unistd.h>
#include <asm/cacheflush.h>
#include <asm-generic/mman-common.h>

#include <asm/kdasics.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/hashtable.h>
#include <linux/random.h>

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

#ifdef CONFIG_DASICS
SYSCALL_DEFINE5(riscv_dasics_bound, int, op, int, handle, int, priv,
	unsigned long, lo, unsigned long, hi)
{
	struct dasics_bound *bound;
	u32 hash_key;
	struct pt_regs *regs;
	uint64_t libcfg;
	int32_t victim;
	uint64_t curr_cfg;
	int32_t step;
	int ret = 0;

	hash_key = hash_32(handle, 4);
	regs = current_pt_regs();
	libcfg =  regs->dasicsLibCfg0;
	step = 4;

	switch(op) {
	case 0:
		bound = kmalloc(sizeof(*bound), GFP_KERNEL);
		if (!bound) {
			ret = -ENOMEM;
			break;
		}

		bound->handle = handle;
		bound->priv = priv;
		bound->lo = lo;
		bound->hi = hi;

		hash_add((current->dasics_hash_table), &bound->node, hash_key);

		// Find an empty libbound slot
		for (victim = 0; victim < DASICS_LIBCFG_WIDTH; ++victim) {
			curr_cfg = (libcfg >> (victim * step)) & DASICS_LIBCFG_MASK;
			if ((curr_cfg & DASICS_LIBCFG_V) == 0) {
				break;
			} 
		}
		// Kick out a random victim if we cannot find one available slot
		if (victim == DASICS_LIBCFG_WIDTH) {
			victim = get_random_u32() % DASICS_LIBCFG_WIDTH;
		}
	
		// Write libbound
		regs->dasicsLibBounds[victim][0] = lo;
		regs->dasicsLibBounds[victim][1] = hi;
	
		// Write config
		libcfg &= ~(DASICS_LIBCFG_MASK << (victim * step));
		libcfg |= ((uint64_t)bound->priv) << (victim * step);
		regs->dasicsLibCfg0 = libcfg;   // DasicsLibCfg

		// update the csr_idx->handle map
		current->dlibcfg_handle_map[victim] = bound->handle;

		pr_info("DASICS LibBound Alloc: handle:%d pri:0x%lx lo:0x%lx hi:0x%lx\n", \
			handle, priv, lo, hi);
		break;
		
	case 1:
		hash_for_each_possible((current->dasics_hash_table), bound, node, hash_key) {
			if (bound->handle == handle) {
				hash_del(&bound->node);
				kfree(bound);

				for (victim = 0; victim < DASICS_LIBCFG_WIDTH; ++victim) {
					if (current->dlibcfg_handle_map[victim] == handle) {
						libcfg &= ~(DASICS_LIBCFG_V << (victim * step));
						regs->dasicsLibCfg0 = libcfg;   // DasicsLibCfg
						current->dlibcfg_handle_map[victim] = -1;
						break;
					}
				}
				pr_info("DASICS LibBound Free: handle:%d\n", handle);
				break;
			}
		}
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}
#endif