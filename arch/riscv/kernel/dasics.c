// SPDX-License-Identifier: GPL-2.0-only

#include <linux/compiler.h>
#include <linux/binfmts.h>
#include <linux/elf.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/smp.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

#include <asm/csr.h>
#include <asm/dasics.h>

static bool riscv_dasics_ready;

#define DASICS_SSTATEEN0_CUSTOM_CSR	0x1UL
#define DASICS_UMAIN_CFG_MASK		0x3eUL
#define DASICS_SUPERVISOR_MASK_TOTAL	9UL

#define DASICS_FREASON_ECALL		1UL
#define DASICS_TEST_SYSCALL		306L
#define DASICS_LINUX_CONTROL_MAGIC	0x4441534943534c58UL
#define DASICS_LINUX_QUERY_UMAINCFG	0x515259UL
#define DASICS_LINUX_ENABLE_CUET		0x43554554UL

#define DASICS_EXEC_OPTION		"-dasics"

#define DASICS_MAINCFG_MAGIC		0x4644494d43464755UL
#define DASICS_MAINCFG_ARM		0x41524dUL
#define DASICS_MAINCFG_REPORT		0x525054UL
#define DASICS_MAINCFG_OBSERVATIONS	16UL

#define DASICS_MAINCFG_ECALL_SENTINEL	0x4543414c4c534954UL
#define DASICS_MAINCFG_ECALL_GUARD	0x47554152444d4346UL
#define DASICS_MAINCFG_LOAD_SENTINEL	0x5a5a5a5a5a5a5a5aUL
#define DASICS_MAINCFG_LOAD_VALUE	0x554c4f4144444154UL
#define DASICS_MAINCFG_STORE_INITIAL	0x55494e495453544fUL
#define DASICS_MAINCFG_STORE_ATTEMPT	0x5553544f52454421UL
#define DASICS_MAINCFG_JUMP_SENTINEL	0x1122334455667788UL
#define DASICS_MAINCFG_JUMP_INITIAL	0x554a554d50494e49UL
#define DASICS_MAINCFG_JUMP_TAKEN	0x554a554d5054414bUL

enum riscv_dasics_maincfg_operation {
	DASICS_MAINCFG_ECALL,
	DASICS_MAINCFG_LOAD,
	DASICS_MAINCFG_STORE,
	DASICS_MAINCFG_JUMP,
	DASICS_MAINCFG_OPERATION_COUNT,
};

enum riscv_dasics_maincfg_stage {
	DASICS_MAINCFG_DISABLED_INITIAL,
	DASICS_MAINCFG_OPEN,
	DASICS_MAINCFG_CLOSED,
	DASICS_MAINCFG_DISABLED_FINAL,
};

enum riscv_dasics_maincfg_event {
	DASICS_MAINCFG_EVENT_NONE,
	DASICS_MAINCFG_EVENT_ORDINARY,
	DASICS_MAINCFG_EVENT_FDI,
};

static const char *const riscv_dasics_maincfg_operation_names[] = {
	"ecall", "load", "store", "jump",
};

static const char *const riscv_dasics_maincfg_stage_names[] = {
	"disabled-initial", "open", "closed", "disabled-final",
};

static const unsigned long riscv_dasics_maincfg_open_cfg[] = {
	0x3aUL, 0x2eUL, 0x36UL, 0x1eUL,
};

static bool riscv_dasics_maincfg_fault(struct pt_regs *regs);

#define DASICS_SAVE_BOUND(index, lo_csr, hi_csr) do {		\
		state->lib_bound_lo[index] = csr_read(lo_csr);	\
		state->lib_bound_hi[index] = csr_read(hi_csr);	\
	} while (0)

#define DASICS_RESTORE_BOUND(index, lo_csr, hi_csr) do {	\
		csr_write(lo_csr, state->lib_bound_lo[index]);	\
		csr_write(hi_csr, state->lib_bound_hi[index]);	\
	} while (0)

#define DASICS_SAVE_JUMP_BOUND(index, lo_csr, hi_csr) do {	\
		state->jump_bound_lo[index] = csr_read(lo_csr);	\
		state->jump_bound_hi[index] = csr_read(hi_csr);	\
	} while (0)

#define DASICS_RESTORE_JUMP_BOUND(index, lo_csr, hi_csr) do {	\
		csr_write(lo_csr, state->jump_bound_lo[index]);	\
		csr_write(hi_csr, state->jump_bound_hi[index]);	\
	} while (0)

static void riscv_dasics_save_hw(struct riscv_dasics_hw_state *state)
{
	/*
	 * Disable checking before taking the snapshot so an interrupt cannot
	 * observe a partially saved state as an active DASICS context.
	 */
	state->umain_cfg = csr_swap(0x9e1, 0);
	state->umain_bound_lo = csr_read(0x9e2);
	state->umain_bound_hi = csr_read(0x9e3);
	state->lib_cfg = csr_read(0x880);
	DASICS_SAVE_BOUND(0, 0x890, 0x891);
	DASICS_SAVE_BOUND(1, 0x892, 0x893);
	DASICS_SAVE_BOUND(2, 0x894, 0x895);
	DASICS_SAVE_BOUND(3, 0x896, 0x897);
	DASICS_SAVE_BOUND(4, 0x898, 0x899);
	DASICS_SAVE_BOUND(5, 0x89a, 0x89b);
	DASICS_SAVE_BOUND(6, 0x89c, 0x89d);
	DASICS_SAVE_BOUND(7, 0x89e, 0x89f);
	DASICS_SAVE_BOUND(8, 0x8a0, 0x8a1);
	DASICS_SAVE_BOUND(9, 0x8a2, 0x8a3);
	DASICS_SAVE_BOUND(10, 0x8a4, 0x8a5);
	DASICS_SAVE_BOUND(11, 0x8a6, 0x8a7);
	DASICS_SAVE_BOUND(12, 0x8a8, 0x8a9);
	DASICS_SAVE_BOUND(13, 0x8aa, 0x8ab);
	DASICS_SAVE_BOUND(14, 0x8ac, 0x8ad);
	DASICS_SAVE_BOUND(15, 0x8ae, 0x8af);
	state->maincall = csr_read(0x8b0);
	state->return_pc = csr_read(0x8b1);
	state->active_return = csr_read(0x8b2);
	state->freason = csr_read(0x8b3);
	DASICS_SAVE_JUMP_BOUND(0, 0x8c0, 0x8c1);
	DASICS_SAVE_JUMP_BOUND(1, 0x8c2, 0x8c3);
	DASICS_SAVE_JUMP_BOUND(2, 0x8c4, 0x8c5);
	DASICS_SAVE_JUMP_BOUND(3, 0x8c6, 0x8c7);
	state->jump_cfg = csr_read(0x8c8);
}

static void riscv_dasics_restore_hw(const struct riscv_dasics_hw_state *state)
{
	/* Activate checking only after every dependent CSR is restored. */
	csr_write(0x9e1, 0);
	csr_write(0x9e2, state->umain_bound_lo);
	csr_write(0x9e3, state->umain_bound_hi);
	csr_write(0x880, state->lib_cfg);
	DASICS_RESTORE_BOUND(0, 0x890, 0x891);
	DASICS_RESTORE_BOUND(1, 0x892, 0x893);
	DASICS_RESTORE_BOUND(2, 0x894, 0x895);
	DASICS_RESTORE_BOUND(3, 0x896, 0x897);
	DASICS_RESTORE_BOUND(4, 0x898, 0x899);
	DASICS_RESTORE_BOUND(5, 0x89a, 0x89b);
	DASICS_RESTORE_BOUND(6, 0x89c, 0x89d);
	DASICS_RESTORE_BOUND(7, 0x89e, 0x89f);
	DASICS_RESTORE_BOUND(8, 0x8a0, 0x8a1);
	DASICS_RESTORE_BOUND(9, 0x8a2, 0x8a3);
	DASICS_RESTORE_BOUND(10, 0x8a4, 0x8a5);
	DASICS_RESTORE_BOUND(11, 0x8a6, 0x8a7);
	DASICS_RESTORE_BOUND(12, 0x8a8, 0x8a9);
	DASICS_RESTORE_BOUND(13, 0x8aa, 0x8ab);
	DASICS_RESTORE_BOUND(14, 0x8ac, 0x8ad);
	DASICS_RESTORE_BOUND(15, 0x8ae, 0x8af);
	csr_write(0x8b0, state->maincall);
	csr_write(0x8b1, state->return_pc);
	csr_write(0x8b2, state->active_return);
	csr_write(0x8b3, state->freason);
	DASICS_RESTORE_JUMP_BOUND(0, 0x8c0, 0x8c1);
	DASICS_RESTORE_JUMP_BOUND(1, 0x8c2, 0x8c3);
	DASICS_RESTORE_JUMP_BOUND(2, 0x8c4, 0x8c5);
	DASICS_RESTORE_JUMP_BOUND(3, 0x8c6, 0x8c7);
	csr_write(0x8c8, state->jump_cfg);
	csr_write(0x9e1, state->umain_cfg);
}

static void riscv_dasics_clear_hw(void)
{
	static const struct riscv_dasics_hw_state empty;

	riscv_dasics_restore_hw(&empty);
}

static bool riscv_dasics_section_range(const struct elf_shdr *section,
				       unsigned long *lo,
				       unsigned long *hi)
{
	*lo = section->sh_addr;
	if (section->sh_size > ULONG_MAX - *lo)
		return false;
	*hi = *lo + section->sh_size;
	return *hi <= ULONG_MAX - 7UL;
}

static int riscv_dasics_read_elf_sections(
	struct linux_binprm *bprm, const struct elfhdr *elf_ex,
	struct riscv_dasics_elf_sections *result)
{
	const struct elf_shdr *string_section;
	struct elf_shdr *sections;
	char *names;
	loff_t pos;
	ssize_t read_size;
	size_t section_bytes;
	unsigned int i;
	int ret = 0;

	if (!elf_ex->e_shnum || elf_ex->e_shnum > 1024 ||
	    elf_ex->e_shentsize != sizeof(*sections) ||
	    elf_ex->e_shstrndx == SHN_UNDEF ||
	    elf_ex->e_shstrndx >= elf_ex->e_shnum)
		return -ENOEXEC;

	sections = kmalloc_array(elf_ex->e_shnum, sizeof(*sections),
				 GFP_KERNEL);
	if (!sections)
		return -ENOMEM;

	section_bytes = elf_ex->e_shnum * sizeof(*sections);
	pos = elf_ex->e_shoff;
	read_size = kernel_read(bprm->file, sections, section_bytes, &pos);
	if (read_size != section_bytes) {
		ret = read_size < 0 ? read_size : -EIO;
		goto out_sections;
	}

	string_section = &sections[elf_ex->e_shstrndx];
	if (!string_section->sh_size || string_section->sh_size > (1UL << 20)) {
		ret = -ENOEXEC;
		goto out_sections;
	}

	names = kmalloc(string_section->sh_size, GFP_KERNEL);
	if (!names) {
		ret = -ENOMEM;
		goto out_sections;
	}

	pos = string_section->sh_offset;
	read_size = kernel_read(bprm->file, names, string_section->sh_size,
				&pos);
	if (read_size != string_section->sh_size) {
		ret = read_size < 0 ? read_size : -EIO;
		goto out_names;
	}

	for (i = 0; i < elf_ex->e_shnum; i++) {
		const struct elf_shdr *section = &sections[i];
		const char *name;
		unsigned long lo;
		unsigned long hi;
		size_t remaining;

		if (section->sh_name >= string_section->sh_size)
			continue;
		name = names + section->sh_name;
		remaining = string_section->sh_size - section->sh_name;
		if (!memchr(name, '\0', remaining))
			continue;
		if (strcmp(name, ".text") &&
		    strcmp(name, ".ulibtext") &&
		    strcmp(name, ".ufreezonetext"))
			continue;
		if (!(section->sh_flags & SHF_EXECINSTR) ||
		    (!strcmp(name, ".text") ? section->sh_size < 4 :
					     !section->sh_size)) {
			ret = -ENOEXEC;
			goto out_names;
		}
		if (!riscv_dasics_section_range(section, &lo, &hi)) {
			ret = -ENOEXEC;
			goto out_names;
		}

		if (!strcmp(name, ".text")) {
			result->have_text = true;
			result->text_lo = lo;
			result->text_hi = hi;
		} else if (!strcmp(name, ".ulibtext")) {
			result->have_ulib_text = true;
			result->ulib_text_lo = lo;
			result->ulib_text_hi = hi;
		} else {
			result->have_freezone = true;
			result->freezone_lo = lo;
			result->freezone_hi = hi;
		}
	}

	if (!result->have_text)
		ret = -ENOEXEC;

out_names:
	kfree(names);
out_sections:
	kfree(sections);
	return ret;
}

static unsigned long riscv_dasics_align_down(unsigned long value)
{
	return value & ~7UL;
}

static unsigned long riscv_dasics_align_up(unsigned long value)
{
	return (value + 7UL) & ~7UL;
}

int riscv_dasics_prepare_exec(struct linux_binprm *bprm,
			      const char __user *last_arg)
{
	char option[sizeof(DASICS_EXEC_OPTION)];
	long length;

	if (bprm->argc < 2)
		return 0;
	if (IS_ERR(last_arg))
		return PTR_ERR(last_arg);
	if (!last_arg)
		return -EFAULT;

	length = strncpy_from_user(option, last_arg, sizeof(option));
	if (length < 0)
		return length;
	if (length != sizeof(option) - 1 ||
	    memcmp(option, DASICS_EXEC_OPTION, sizeof(option)))
		return 0;

	bprm->argc--;
	bprm->dasics.requested = true;
	return 0;
}

int riscv_dasics_validate_elf(struct linux_binprm *bprm,
			      const void *elf_header, bool has_interpreter)
{
	const struct elfhdr *elf_ex = elf_header;
	int ret;

	if (!bprm->dasics.requested)
		return 0;
	if (elf_ex->e_ident[EI_CLASS] != ELFCLASS64)
		return -ENOEXEC;
	if (bprm->interp != bprm->filename)
		return -ENOEXEC;
	/* The opt-in contract currently supports fixed-address static ELFs. */
	if (has_interpreter || elf_ex->e_type != ET_EXEC)
		return -ENOEXEC;

	ret = riscv_dasics_read_elf_sections(bprm, elf_ex,
					     &bprm->dasics.sections);
	if (ret)
		return ret;

	bprm->dasics.validated = true;
	return 0;
}

void riscv_dasics_setup_elf(struct linux_binprm *bprm,
			    unsigned long load_bias, unsigned long start_data)
{
	struct riscv_dasics_elf_sections sections = bprm->dasics.sections;
	struct riscv_dasics_state *state = &current->thread.dasics;
	struct riscv_dasics_hw_state *hw = &state->hw;

	memset(state, 0, sizeof(*state));
	if (!bprm->dasics.validated)
		return;

	sections.text_lo += load_bias;
	sections.text_hi += load_bias;
	if (sections.have_ulib_text) {
		sections.ulib_text_lo += load_bias;
		sections.ulib_text_hi += load_bias;
	}
	if (sections.have_freezone) {
		sections.freezone_lo += load_bias;
		sections.freezone_hi += load_bias;
	}

	state->metadata.enabled = true;
	state->metadata.text_lo = sections.text_lo;
	state->metadata.text_hi = sections.text_hi;
	state->metadata.ulib_text_lo = sections.ulib_text_lo;
	state->metadata.ulib_text_hi = sections.ulib_text_hi;
	state->metadata.start_data = start_data;

	hw->umain_cfg = RISCV_DASICS_UMAIN_UENA;
	hw->umain_bound_lo = riscv_dasics_align_down(sections.text_lo);
	hw->umain_bound_hi = riscv_dasics_align_up(sections.text_hi);

	if (sections.have_freezone) {
		hw->jump_bound_lo[0] =
			riscv_dasics_align_down(sections.freezone_lo);
		hw->jump_bound_hi[0] =
			riscv_dasics_align_up(sections.freezone_hi);
		hw->jump_cfg = 1UL;
	}

	if (sections.have_ulib_text) {
		hw->lib_bound_lo[0] =
			riscv_dasics_align_down(current->mm->start_brk);
		hw->lib_bound_hi[0] =
			riscv_dasics_align_up(current->mm->start_stack);
		hw->lib_cfg = RISCV_DASICS_LIBCFG_V |
			      RISCV_DASICS_LIBCFG_R |
			      RISCV_DASICS_LIBCFG_W;

		if (sections.ulib_text_hi < start_data) {
			hw->lib_bound_lo[1] =
				riscv_dasics_align_down(sections.ulib_text_hi);
			hw->lib_bound_hi[1] =
				riscv_dasics_align_up(start_data);
			hw->lib_cfg |=
				(RISCV_DASICS_LIBCFG_V |
				 RISCV_DASICS_LIBCFG_R) << 4;
		}
	}

	pr_info("DASICS_ELF_SETUP text=[0x%lx,0x%lx) "
		"ulib=[0x%lx,0x%lx) freezone=[0x%lx,0x%lx)\n",
		sections.text_lo, sections.text_hi,
		sections.ulib_text_lo, sections.ulib_text_hi,
		sections.freezone_lo, sections.freezone_hi);
}

static unsigned long riscv_dasics_read_lib_bound_lo(unsigned int index)
{
	switch (index) {
	case 0: return csr_read(0x890);
	case 1: return csr_read(0x892);
	case 2: return csr_read(0x894);
	case 3: return csr_read(0x896);
	case 4: return csr_read(0x898);
	case 5: return csr_read(0x89a);
	case 6: return csr_read(0x89c);
	case 7: return csr_read(0x89e);
	case 8: return csr_read(0x8a0);
	case 9: return csr_read(0x8a2);
	case 10: return csr_read(0x8a4);
	case 11: return csr_read(0x8a6);
	case 12: return csr_read(0x8a8);
	case 13: return csr_read(0x8aa);
	case 14: return csr_read(0x8ac);
	default: return csr_read(0x8ae);
	}
}

static unsigned long riscv_dasics_read_lib_bound_hi(unsigned int index)
{
	switch (index) {
	case 0: return csr_read(0x891);
	case 1: return csr_read(0x893);
	case 2: return csr_read(0x895);
	case 3: return csr_read(0x897);
	case 4: return csr_read(0x899);
	case 5: return csr_read(0x89b);
	case 6: return csr_read(0x89d);
	case 7: return csr_read(0x89f);
	case 8: return csr_read(0x8a1);
	case 9: return csr_read(0x8a3);
	case 10: return csr_read(0x8a5);
	case 11: return csr_read(0x8a7);
	case 12: return csr_read(0x8a9);
	case 13: return csr_read(0x8ab);
	case 14: return csr_read(0x8ad);
	default: return csr_read(0x8af);
	}
}

static bool riscv_dasics_buffer_permitted(unsigned long base,
					  unsigned long length,
					  unsigned long permission)
{
	unsigned long lib_cfg = csr_read(0x880);
	unsigned long end;
	unsigned long cursor;

	if (!length)
		return true;
	if (length > LONG_MAX || base > ULONG_MAX - length)
		return false;
	end = base + length;
	cursor = base;

	while (cursor < end) {
		unsigned long best = cursor;
		unsigned int i;

		for (i = 0; i < RISCV_DASICS_LIB_BOUND_COUNT; i++) {
			unsigned long cfg = (lib_cfg >> (i * 4)) & 0xf;
			unsigned long lo;
			unsigned long hi;

			if ((cfg & (permission | RISCV_DASICS_LIBCFG_V)) !=
			    (permission | RISCV_DASICS_LIBCFG_V))
				continue;
			lo = riscv_dasics_read_lib_bound_lo(i);
			hi = riscv_dasics_read_lib_bound_hi(i);
			if (lo <= cursor && cursor < hi && hi > best)
				best = hi;
		}

		if (best == cursor)
			return false;
		if (best >= end)
			return true;
		cursor = best;
	}

	return true;
}

static bool riscv_dasics_proxy_syscall(struct pt_regs *regs)
{
	unsigned long buffer = regs->a1;
	unsigned long length = regs->a2;
	unsigned long permission;
	long result;

	switch (regs->a7) {
	case __NR_read:
	case __NR_pread64:
		permission = RISCV_DASICS_LIBCFG_W;
		break;
	case __NR_write:
	case __NR_pwrite64:
		permission = RISCV_DASICS_LIBCFG_R;
		break;
	default:
		return false;
	}

	if (!riscv_dasics_buffer_permitted(buffer, length, permission))
		return false;

	switch (regs->a7) {
	case __NR_read:
		result = ksys_read(regs->a0, (char __user *)buffer, length);
		break;
	case __NR_write:
		result = ksys_write(regs->a0, (const char __user *)buffer,
				    length);
		break;
	case __NR_pread64:
		result = ksys_pread64(regs->a0, (char __user *)buffer, length,
				      regs->a3);
		break;
	default:
		result = ksys_pwrite64(regs->a0,
				       (const char __user *)buffer, length,
				       regs->a3);
		break;
	}
	regs->a0 = result;
	return true;
}

bool riscv_dasics_handle_illegal(struct pt_regs *regs)
{
	const struct riscv_dasics_metadata *metadata =
		&current->thread.dasics.metadata;

	if (!metadata->enabled ||
	    regs->epc < metadata->ulib_text_lo ||
	    regs->epc >= metadata->ulib_text_hi)
		return false;

	regs->epc += 4;
	return true;
}

bool riscv_dasics_handle_fault(struct pt_regs *regs)
{
	if (!current->thread.dasics.metadata.enabled)
		return false;

	if (riscv_dasics_maincfg_fault(regs))
		return true;
	if (csr_read(0x8b3) == DASICS_FREASON_ECALL)
		riscv_dasics_proxy_syscall(regs);
	regs->epc += 4;
	return true;
}

static bool riscv_dasics_pc_is_trusted(const struct pt_regs *regs)
{
	const struct riscv_dasics_metadata *metadata =
		&current->thread.dasics.metadata;
	unsigned long pc;

	if (regs->epc < 4)
		return false;
	pc = regs->epc - 4;
	return metadata->text_lo <= pc && pc <= metadata->text_hi - 4;
}

static enum riscv_dasics_maincfg_operation
riscv_dasics_maincfg_operation(unsigned long step)
{
	return step / 4;
}

static enum riscv_dasics_maincfg_stage
riscv_dasics_maincfg_stage(unsigned long step)
{
	return step % 4;
}

static unsigned long riscv_dasics_maincfg_expected_cfg(
	enum riscv_dasics_maincfg_operation operation,
	enum riscv_dasics_maincfg_stage stage)
{
	if (stage == DASICS_MAINCFG_OPEN)
		return riscv_dasics_maincfg_open_cfg[operation];
	if (stage == DASICS_MAINCFG_CLOSED)
		return DASICS_UMAIN_CFG_MASK;
	return 0;
}

static enum riscv_dasics_maincfg_event
riscv_dasics_maincfg_expected_event(
	enum riscv_dasics_maincfg_operation operation,
	enum riscv_dasics_maincfg_stage stage)
{
	if (stage == DASICS_MAINCFG_OPEN)
		return DASICS_MAINCFG_EVENT_FDI;
	if (operation == DASICS_MAINCFG_ECALL)
		return DASICS_MAINCFG_EVENT_ORDINARY;
	return DASICS_MAINCFG_EVENT_NONE;
}

static const char *riscv_dasics_maincfg_event_name(unsigned long event)
{
	switch (event) {
	case DASICS_MAINCFG_EVENT_ORDINARY:
		return "ordinary";
	case DASICS_MAINCFG_EVENT_FDI:
		return "fdi";
	default:
		return "none";
	}
}

static bool riscv_dasics_range_contains(unsigned long lo, unsigned long hi,
					unsigned long address,
					unsigned long size)
{
	return lo <= address && address < hi && size <= hi - address;
}

static bool riscv_dasics_maincfg_process_matches(unsigned long pid)
{
	return current->thread.dasics.metadata.enabled &&
	       task_pid_vnr(current) == pid;
}

static void riscv_dasics_maincfg_restore(void)
{
	struct riscv_dasics_state *state = &current->thread.dasics;
	struct riscv_dasics_maincfg_control *control =
		&state->maincfg_control;

	if (control->active) {
		state->hw = control->saved_hw;
		riscv_dasics_restore_hw(&state->hw);
	}
	memset(control, 0, sizeof(*control));
}

static bool riscv_dasics_maincfg_protocol_failure(struct pt_regs *regs,
						   const char *detail)
{
	pr_err("DASICS_MAINCFG_TOGGLE_PROTOCOL version=1 pid=%d step=%lu "
	       "detail=%s result=FAIL\n",
	       task_pid_vnr(current), regs->a3, detail);
	riscv_dasics_maincfg_restore();
	regs->a0 = -1UL;
	return true;
}

static void riscv_dasics_maincfg_capture(
	struct pt_regs *regs, unsigned long epc,
	enum riscv_dasics_maincfg_event event)
{
	struct riscv_dasics_maincfg_actual *actual =
		&current->thread.dasics.maincfg_control.actual;

	actual->cfg = csr_read(0x9e1) & DASICS_UMAIN_CFG_MASK;
	actual->cause = regs->cause;
	actual->epc = epc;
	actual->tval = regs->badaddr;
	actual->freason = csr_read(0x8b3);
	actual->event = event;
	actual->seen = true;
}

static void riscv_dasics_maincfg_set_csr_state(unsigned long cfg)
{
	struct riscv_dasics_state *state = &current->thread.dasics;
	struct riscv_dasics_hw_state *hw = &state->hw;

	memset(hw, 0, sizeof(*hw));
	hw->umain_cfg = cfg & DASICS_UMAIN_CFG_MASK;
	hw->umain_bound_lo =
		riscv_dasics_align_down(state->metadata.text_lo);
	hw->umain_bound_hi =
		riscv_dasics_align_up(state->metadata.text_hi);
	hw->freason = 5;
	riscv_dasics_restore_hw(hw);
}

static bool riscv_dasics_maincfg_validate_site(
	struct pt_regs *regs, enum riscv_dasics_maincfg_operation operation)
{
	const struct riscv_dasics_metadata *metadata =
		&current->thread.dasics.metadata;
	unsigned long source = regs->a4;
	unsigned long operand = regs->a5;
	unsigned long recovery = regs->a6;

	if ((source & 3) || (operand & 7) || (recovery & 3))
		return false;
	if (!riscv_dasics_range_contains(metadata->ulib_text_lo,
					 metadata->ulib_text_hi,
					 source, 4))
		return false;
	if (!riscv_dasics_range_contains(metadata->text_lo,
					 metadata->text_hi,
					 recovery, 4))
		return false;
	if (operation == DASICS_MAINCFG_JUMP)
		return riscv_dasics_range_contains(metadata->text_lo,
						   metadata->text_hi,
						   operand, 4);
	return current->mm &&
	       riscv_dasics_range_contains(metadata->start_data,
					   current->mm->start_brk,
					   operand,
					   sizeof(unsigned long));
}

static bool riscv_dasics_maincfg_site_identity_matches(
	struct riscv_dasics_maincfg_control *control,
	enum riscv_dasics_maincfg_operation operation,
	unsigned long source, unsigned long operand, unsigned long recovery)
{
	unsigned int prior;

	if (!control->source[operation]) {
		for (prior = 0; prior < operation; prior++) {
			if (control->source[prior] == source)
				return false;
		}
		control->source[operation] = source;
		control->operand[operation] = operand;
		control->recovery[operation] = recovery;
		return true;
	}

	return control->source[operation] == source &&
	       control->operand[operation] == operand &&
	       control->recovery[operation] == recovery;
}

static int riscv_dasics_maincfg_initialize_operand(
	enum riscv_dasics_maincfg_operation operation, unsigned long operand)
{
	unsigned long value;

	switch (operation) {
	case DASICS_MAINCFG_ECALL:
		value = DASICS_MAINCFG_ECALL_GUARD;
		break;
	case DASICS_MAINCFG_LOAD:
		value = DASICS_MAINCFG_LOAD_VALUE;
		break;
	case DASICS_MAINCFG_STORE:
		value = DASICS_MAINCFG_STORE_INITIAL;
		break;
	default:
		return 0;
	}

	return put_user(value, (unsigned long __user *)operand);
}

static bool riscv_dasics_maincfg_arm(struct pt_regs *regs)
{
	struct riscv_dasics_state *state = &current->thread.dasics;
	struct riscv_dasics_maincfg_control *control =
		&state->maincfg_control;
	unsigned long step = regs->a3;
	enum riscv_dasics_maincfg_operation operation;
	enum riscv_dasics_maincfg_stage stage;
	unsigned long cfg;

	if (step >= DASICS_MAINCFG_OBSERVATIONS)
		return riscv_dasics_maincfg_protocol_failure(
			regs, "arm-step-range");
	if (!step) {
		if (control->active || control->armed)
			return riscv_dasics_maincfg_protocol_failure(
				regs, "arm-active-state");
		memset(control, 0, sizeof(*control));
		riscv_dasics_save_hw(&control->saved_hw);
		riscv_dasics_restore_hw(&control->saved_hw);
		state->hw = control->saved_hw;
		control->active = true;
		control->pid = regs->a2;
	}
	if (!control->active || control->armed ||
	    control->pid != regs->a2 || control->next_step != step)
		return riscv_dasics_maincfg_protocol_failure(
			regs, "arm-sequence");

	operation = riscv_dasics_maincfg_operation(step);
	stage = riscv_dasics_maincfg_stage(step);
	if (!riscv_dasics_maincfg_validate_site(regs, operation))
		return riscv_dasics_maincfg_protocol_failure(
			regs, "arm-site-range");
	if (!riscv_dasics_maincfg_site_identity_matches(
		    control, operation, regs->a4, regs->a5, regs->a6))
		return riscv_dasics_maincfg_protocol_failure(
			regs, "arm-site-identity");
	if (riscv_dasics_maincfg_initialize_operand(operation, regs->a5))
		return riscv_dasics_maincfg_protocol_failure(
			regs, "arm-operand");

	cfg = riscv_dasics_maincfg_expected_cfg(operation, stage);
	memset(&control->actual, 0, sizeof(control->actual));
	control->armed_step = step;
	control->armed = true;
	riscv_dasics_maincfg_set_csr_state(cfg);
	regs->a0 = control->pid;
	return true;
}

static unsigned long riscv_dasics_maincfg_expected_cause(
	enum riscv_dasics_maincfg_operation operation,
	enum riscv_dasics_maincfg_stage stage)
{
	if (stage == DASICS_MAINCFG_OPEN)
		return EXC_DASICS_UCHECK_FAULT;
	return operation == DASICS_MAINCFG_ECALL ? EXC_SYSCALL : 0;
}

static unsigned long riscv_dasics_maincfg_expected_tval(
	struct riscv_dasics_maincfg_control *control,
	enum riscv_dasics_maincfg_operation operation,
	enum riscv_dasics_maincfg_stage stage)
{
	if (stage == DASICS_MAINCFG_OPEN &&
	    operation != DASICS_MAINCFG_ECALL)
		return control->operand[operation];
	return 0;
}

static unsigned long riscv_dasics_maincfg_expected_side0(
	struct riscv_dasics_maincfg_control *control,
	enum riscv_dasics_maincfg_operation operation,
	enum riscv_dasics_maincfg_stage stage)
{
	bool denied = stage == DASICS_MAINCFG_OPEN;

	switch (operation) {
	case DASICS_MAINCFG_ECALL:
		return denied ? DASICS_MAINCFG_ECALL_SENTINEL : control->pid;
	case DASICS_MAINCFG_LOAD:
		return denied ? DASICS_MAINCFG_LOAD_SENTINEL :
				DASICS_MAINCFG_LOAD_VALUE;
	case DASICS_MAINCFG_STORE:
		return denied ? DASICS_MAINCFG_STORE_INITIAL :
				DASICS_MAINCFG_STORE_ATTEMPT;
	default:
		return denied ? DASICS_MAINCFG_JUMP_SENTINEL :
				control->source[operation] + 4;
	}
}

static unsigned long riscv_dasics_maincfg_expected_side1(
	enum riscv_dasics_maincfg_operation operation,
	enum riscv_dasics_maincfg_stage stage)
{
	switch (operation) {
	case DASICS_MAINCFG_ECALL:
		return DASICS_MAINCFG_ECALL_GUARD;
	case DASICS_MAINCFG_LOAD:
		return DASICS_MAINCFG_LOAD_VALUE;
	case DASICS_MAINCFG_STORE:
		return DASICS_MAINCFG_STORE_ATTEMPT;
	default:
		return stage == DASICS_MAINCFG_OPEN ?
				DASICS_MAINCFG_JUMP_INITIAL :
				DASICS_MAINCFG_JUMP_TAKEN;
	}
}

static void riscv_dasics_maincfg_print_observation(
	struct riscv_dasics_maincfg_control *control, unsigned long step,
	enum riscv_dasics_maincfg_operation operation,
	enum riscv_dasics_maincfg_stage stage, unsigned long cfg_expected,
	unsigned long cause_expected, unsigned long tval_expected,
	unsigned long freason_expected, unsigned long side0_expected,
	unsigned long side0_actual, unsigned long side1_expected,
	unsigned long side1_actual, bool passed)
{
	const char *policy =
		operation == DASICS_MAINCFG_ECALL &&
		stage != DASICS_MAINCFG_OPEN ? "record" : "strict";

	pr_info("DASICS_MAINCFG_TOGGLE_OBSERVATION version=1 index=%lu "
		"op=%s stage=%s pid=%lu cfg_expected=0x%lx cfg_actual=0x%lx "
		"event=%s source=0x%lx operand=0x%lx recovery=0x%lx "
		"cause_expected=0x%lx cause_actual=0x%lx "
		"epc_expected=0x%lx epc_actual=0x%lx "
		"tval_expected=0x%lx tval_actual=0x%lx "
		"freason_policy=%s freason_expected=0x%lx "
		"freason_actual=0x%lx side0_expected=0x%lx "
		"side0_actual=0x%lx side1_expected=0x%lx "
		"side1_actual=0x%lx result=%s\n",
		step, riscv_dasics_maincfg_operation_names[operation],
		riscv_dasics_maincfg_stage_names[stage], control->pid,
		cfg_expected, control->actual.cfg,
		riscv_dasics_maincfg_event_name(control->actual.event),
		control->source[operation], control->operand[operation],
		control->recovery[operation], cause_expected,
		control->actual.cause, control->source[operation],
		control->actual.epc, tval_expected, control->actual.tval,
		policy, freason_expected, control->actual.freason,
		side0_expected, side0_actual, side1_expected, side1_actual,
		passed ? "PASS" : "FAIL");
}

static bool riscv_dasics_maincfg_report(struct pt_regs *regs)
{
	struct riscv_dasics_maincfg_control *control =
		&current->thread.dasics.maincfg_control;
	unsigned long pid = control->pid;
	unsigned long step = regs->a3;
	enum riscv_dasics_maincfg_operation operation;
	enum riscv_dasics_maincfg_stage stage;
	enum riscv_dasics_maincfg_event event_expected;
	unsigned long cfg_expected;
	unsigned long cause_expected;
	unsigned long tval_expected;
	unsigned long freason_expected;
	unsigned long side0_expected;
	unsigned long side1_expected;
	bool freason_strict;
	bool passed;

	if (!control->active || !control->armed ||
	    control->pid != regs->a2 || control->armed_step != step ||
	    control->next_step != step || regs->a6)
		return riscv_dasics_maincfg_protocol_failure(
			regs, "report-sequence");

	operation = riscv_dasics_maincfg_operation(step);
	stage = riscv_dasics_maincfg_stage(step);
	if (!control->actual.seen) {
		control->actual.cfg =
			csr_read(0x9e1) & DASICS_UMAIN_CFG_MASK;
		control->actual.cause = 0;
		control->actual.epc = control->source[operation];
		control->actual.tval = 0;
		control->actual.freason = csr_read(0x8b3);
		control->actual.event = DASICS_MAINCFG_EVENT_NONE;
		control->actual.seen = true;
	}

	cfg_expected =
		riscv_dasics_maincfg_expected_cfg(operation, stage);
	event_expected =
		riscv_dasics_maincfg_expected_event(operation, stage);
	cause_expected =
		riscv_dasics_maincfg_expected_cause(operation, stage);
	tval_expected =
		riscv_dasics_maincfg_expected_tval(control, operation, stage);
	freason_expected =
		stage == DASICS_MAINCFG_OPEN ? operation + 1 : 5;
	side0_expected =
		riscv_dasics_maincfg_expected_side0(control, operation, stage);
	side1_expected =
		riscv_dasics_maincfg_expected_side1(operation, stage);
	freason_strict =
		operation != DASICS_MAINCFG_ECALL ||
		stage == DASICS_MAINCFG_OPEN;

	passed = control->actual.cfg == cfg_expected &&
		 control->actual.event == event_expected &&
		 control->actual.cause == cause_expected &&
		 control->actual.epc == control->source[operation] &&
		 control->actual.tval == tval_expected &&
		 (!freason_strict ||
		  control->actual.freason == freason_expected) &&
		 regs->a4 == side0_expected && regs->a5 == side1_expected;

	riscv_dasics_maincfg_print_observation(
		control, step, operation, stage, cfg_expected, cause_expected,
		tval_expected, freason_expected, side0_expected, regs->a4,
		side1_expected, regs->a5, passed);
	if (!passed)
		control->failures++;
	control->armed = false;
	control->next_step++;

	if (control->next_step == DASICS_MAINCFG_OBSERVATIONS) {
		pr_info("DASICS_MAINCFG_TOGGLE_SUMMARY version=1 total=16 "
			"failed=%lu result=%s\n",
			control->failures,
			control->failures ? "FAIL" : "PASS");
		riscv_dasics_maincfg_restore();
	}

	regs->a0 = passed ? pid : -1UL;
	return true;
}

static bool riscv_dasics_maincfg_syscall(struct pt_regs *regs)
{
	struct riscv_dasics_maincfg_control *control =
		&current->thread.dasics.maincfg_control;
	unsigned long source_pc = regs->epc - 4;

	if (control->active && control->armed &&
	    riscv_dasics_maincfg_process_matches(control->pid) &&
	    riscv_dasics_maincfg_operation(control->armed_step) ==
			DASICS_MAINCFG_ECALL &&
	    source_pc == control->source[DASICS_MAINCFG_ECALL] &&
	    regs->orig_a0 == DASICS_MAINCFG_ECALL_SENTINEL) {
		if (control->actual.seen)
			return riscv_dasics_maincfg_protocol_failure(
				regs, "ecall-duplicate-event");
		riscv_dasics_maincfg_capture(
			regs, source_pc, DASICS_MAINCFG_EVENT_ORDINARY);
	}

	if (regs->orig_a0 != DASICS_MAINCFG_MAGIC)
		return false;
	if (!riscv_dasics_maincfg_process_matches(regs->a2))
		return riscv_dasics_maincfg_protocol_failure(
			regs, "control-identity");
	if (!riscv_dasics_pc_is_trusted(regs))
		return riscv_dasics_maincfg_protocol_failure(
			regs, "control-untrusted-pc");
	if (regs->a1 == DASICS_MAINCFG_ARM)
		return riscv_dasics_maincfg_arm(regs);
	if (regs->a1 == DASICS_MAINCFG_REPORT)
		return riscv_dasics_maincfg_report(regs);
	return riscv_dasics_maincfg_protocol_failure(
		regs, "control-command");
}

static bool riscv_dasics_linux_control_syscall(struct pt_regs *regs)
{
	struct riscv_dasics_state *state = &current->thread.dasics;
	unsigned long cfg;

	if (regs->orig_a0 != DASICS_LINUX_CONTROL_MAGIC)
		return false;
	if (regs->a2 || regs->a3 || regs->a4 || regs->a5 || regs->a6) {
		regs->a0 = -EINVAL;
		return true;
	}

	if (regs->a1 == DASICS_LINUX_QUERY_UMAINCFG) {
		regs->a0 = csr_read(0x9e1) & DASICS_UMAIN_CFG_MASK;
		return true;
	}
	if (regs->a1 != DASICS_LINUX_ENABLE_CUET) {
		regs->a0 = -EINVAL;
		return true;
	}

	cfg = csr_read(0x9e1) & DASICS_UMAIN_CFG_MASK;
	/* Trusted main may add CUET, but this interface never changes UENA. */
	if (!state->metadata.enabled ||
	    !riscv_dasics_pc_is_trusted(regs) ||
	    state->maincfg_control.active ||
	    cfg != RISCV_DASICS_UMAIN_UENA) {
		regs->a0 = -EPERM;
		return true;
	}

	cfg |= RISCV_DASICS_UMAIN_CUET;
	state->hw.umain_cfg = cfg;
	csr_write(0x9e1, cfg);
	regs->a0 = cfg;
	return true;
}

static bool riscv_dasics_maincfg_fault(struct pt_regs *regs)
{
	struct riscv_dasics_maincfg_control *control =
		&current->thread.dasics.maincfg_control;
	enum riscv_dasics_maincfg_operation operation;
	unsigned long recovery;

	if (!control->active)
		return false;
	if (!control->armed ||
	    !riscv_dasics_maincfg_process_matches(control->pid)) {
		riscv_dasics_maincfg_protocol_failure(regs, "fdi-unarmed");
		regs->epc += 4;
		return true;
	}

	operation = riscv_dasics_maincfg_operation(control->armed_step);
	recovery = control->recovery[operation];
	if (regs->epc != control->source[operation]) {
		riscv_dasics_maincfg_protocol_failure(regs, "fdi-source");
		regs->epc = recovery;
		return true;
	}
	if (control->actual.seen) {
		riscv_dasics_maincfg_protocol_failure(
			regs, "fdi-duplicate-event");
		regs->epc = recovery;
		return true;
	}

	riscv_dasics_maincfg_capture(regs, regs->epc,
				     DASICS_MAINCFG_EVENT_FDI);
	if (operation == DASICS_MAINCFG_JUMP)
		regs->epc = recovery;
	else
		regs->epc += 4;
	return true;
}

bool riscv_dasics_handle_syscall(struct pt_regs *regs, long syscall)
{
	if (syscall != DASICS_TEST_SYSCALL)
		return false;
	if (riscv_dasics_linux_control_syscall(regs))
		return true;
	if (!current->thread.dasics.metadata.enabled)
		return false;

	if (riscv_dasics_maincfg_syscall(regs))
		return true;
	regs->a0 = task_pid_vnr(current);
	return true;
}

void riscv_dasics_clear_task(struct task_struct *task)
{
	memset(&task->thread.dasics, 0, sizeof(task->thread.dasics));
	if (task == current && READ_ONCE(riscv_dasics_ready))
		riscv_dasics_clear_hw();
}

void riscv_dasics_prepare_copy(struct task_struct *task)
{
	if (task == current && READ_ONCE(riscv_dasics_ready)) {
		riscv_dasics_save_hw(&task->thread.dasics.hw);
		/* Saving disables checking; restore it before fork continues. */
		riscv_dasics_restore_hw(&task->thread.dasics.hw);
	}
}

void riscv_dasics_finish_copy(struct task_struct *task)
{
	struct riscv_dasics_state *state = &task->thread.dasics;
	struct riscv_dasics_maincfg_control *control =
		&state->maincfg_control;

	if (!control->active)
		return;

	/* A child must not resume the parent's in-flight test transaction. */
	state->hw = control->saved_hw;
	memset(control, 0, sizeof(*control));
}

void riscv_dasics_start_thread(struct task_struct *task)
{
	if (READ_ONCE(riscv_dasics_ready))
		riscv_dasics_restore_hw(&task->thread.dasics.hw);
}

void riscv_dasics_switch(struct task_struct *prev, struct task_struct *next)
{
	if (!READ_ONCE(riscv_dasics_ready))
		return;

	riscv_dasics_save_hw(&prev->thread.dasics.hw);
	riscv_dasics_restore_hw(&next->thread.dasics.hw);
}

static unsigned long dasics_supervisor_mask_record(
	const char *operation, const char *name, unsigned long addr,
	unsigned long mask, unsigned long operand, unsigned long old,
	unsigned long expected_old, unsigned long read,
	unsigned long expected_read, unsigned long *total)
{
	bool pass = old == expected_old && read == expected_read;

	(*total)++;
	if (!pass)
		pr_err("FDI_CSR_MASK_OPS case=%s privilege=supervisor csr=%s "
		       "addr=0x%lx mask=0x%lx operand=0x%lx old=0x%lx "
		       "expected_old=0x%lx read=0x%lx expected_read=0x%lx "
		       "result=FAIL\n", operation, name, addr, mask, operand,
		       old, expected_old, read, expected_read);

	return pass ? 0 : 1;
}

static unsigned long dasics_supervisor_mask_ops(void)
{
	unsigned long total = 0;
	unsigned long failures = 0;

	pr_info("FDI_CSR_MASK_OPS_BEGIN version=1 privilege=supervisor "
		"expected_total=%lu\n", DASICS_SUPERVISOR_MASK_TOTAL);

#define DASICS_RUN_SUPERVISOR_OPS(addr, name, mask, seed) do {		\
		unsigned long writable_bit = (mask) & (0UL - (mask));	\
		unsigned long write_operand =				\
			(0xa500000000000007UL ^			\
			 ((unsigned long)(seed) << 8)) & ~writable_bit;	\
		unsigned long expected = write_operand & (mask);		\
		unsigned long set_operand =				\
			(~expected & (mask)) | ~(mask);			\
		unsigned long clear_operand = writable_bit | ~(mask);	\
		unsigned long old = csr_swap(addr, write_operand);	\
		failures += dasics_supervisor_mask_record(		\
				"CSRRW", #name, addr, mask, write_operand,\
				old, 0, csr_read(addr), expected, &total);\
		old = csr_read_set(addr, set_operand);			\
		{							\
			unsigned long expected_old = expected;		\
			expected |= set_operand & (mask);		\
			failures += dasics_supervisor_mask_record(	\
					"CSRRS", #name, addr, mask,	\
					set_operand, old, expected_old,	\
					csr_read(addr), expected, &total);\
		}							\
		old = csr_read_clear(addr, clear_operand);		\
		{							\
			unsigned long expected_old = expected;		\
			expected &= ~(clear_operand & (mask));		\
			failures += dasics_supervisor_mask_record(	\
					"CSRRC", #name, addr, mask,	\
					clear_operand, old, expected_old,	\
					csr_read(addr), expected, &total);\
		}							\
	} while (0)

	DASICS_RUN_SUPERVISOR_OPS(0x9e1, FDIUMainCfg, 0x3eUL, 48);
	DASICS_RUN_SUPERVISOR_OPS(0x9e2, FDIUMainBoundLo, ~7UL, 49);
	DASICS_RUN_SUPERVISOR_OPS(0x9e3, FDIUMainBoundHi, ~7UL, 50);
#undef DASICS_RUN_SUPERVISOR_OPS

	csr_write(0x9e1, 0);
	csr_write(0x9e2, 0);
	csr_write(0x9e3, 0);
	if (total != DASICS_SUPERVISOR_MASK_TOTAL)
		failures++;

	pr_info("FDI_CSR_MASK_OPS_SIGNATURE version=1 privilege=supervisor "
		"total=%lu failed=%lu result=%s\n", total, failures,
		failures ? "FAIL" : "PASS");

	return failures;
}

static unsigned long dasics_supervisor_check_reset(const char *name,
						   unsigned long addr,
						   unsigned long *total)
{
	unsigned long read;
	bool pass;

	switch (addr) {
	case 0x9e1:
		read = csr_read(0x9e1);
		break;
	case 0x9e2:
		read = csr_read(0x9e2);
		break;
	default:
		read = csr_read(0x9e3);
		break;
	}
	pass = read == 0;
	(*total)++;
	if (!pass)
		pr_err("[DASICS-CSR] case=CSR-SMOKE-001 csr=%s addr=0x%lx "
		       "write=0x0 read=0x%lx expect=0x0 result=FAIL\n",
		       name, addr, read);

	return pass ? 0 : 1;
}

static unsigned long dasics_supervisor_check_value(
	const char *id, const char *name, unsigned long addr,
	unsigned long write, unsigned long expect, unsigned long *total)
{
	unsigned long read;
	bool pass;

	switch (addr) {
	case 0x9e1:
		csr_write(0x9e1, write);
		read = csr_read(0x9e1);
		break;
	case 0x9e2:
		csr_write(0x9e2, write);
		read = csr_read(0x9e2);
		break;
	default:
		csr_write(0x9e3, write);
		read = csr_read(0x9e3);
		break;
	}
	pass = read == expect;
	(*total)++;
	if (!pass)
		pr_err("[DASICS-CSR] case=%s csr=%s addr=0x%lx write=0x%lx "
		       "read=0x%lx expect=0x%lx result=FAIL\n", id, name,
		       addr, write, read, expect);

	return pass ? 0 : 1;
}

static unsigned long dasics_supervisor_smoke(void)
{
	unsigned long total = 0;
	unsigned long failures = 0;
	unsigned long stateen;
	unsigned long before;
	unsigned long after;

	pr_info("[DASICS-CSR] supervisor smoke begin\n");
	stateen = csr_read(0x10c);
	total++;
	if (!(stateen & DASICS_SSTATEEN0_CUSTOM_CSR))
		failures++;

	failures += dasics_supervisor_check_reset("DasicsUMainCfg", 0x9e1,
						  &total);
	failures += dasics_supervisor_check_reset("DasicsUMainBoundLo",
						  0x9e2, &total);
	failures += dasics_supervisor_check_reset("DasicsUMainBoundHi",
						  0x9e3, &total);
	failures += dasics_supervisor_check_value(
			"CSR-SMOKE-004", "DasicsUMainCfg", 0x9e1, ~0UL,
			DASICS_UMAIN_CFG_MASK, &total);
	failures += dasics_supervisor_check_value(
			"CSR-SMOKE-014", "DasicsUMainBoundLo", 0x9e2,
			0x1111111122222227UL, 0x1111111122222220UL, &total);
	failures += dasics_supervisor_check_value(
			"CSR-SMOKE-014", "DasicsUMainBoundHi", 0x9e3,
			0x3333333344444447UL, 0x3333333344444440UL, &total);

	csr_write(0x9e1, DASICS_UMAIN_CFG_MASK);
	before = csr_read(0x9e1);
	csr_write(0x9e0, ~0UL);
	after = csr_read(0x9e1);
	total++;
	if (before != DASICS_UMAIN_CFG_MASK ||
	    after != DASICS_UMAIN_CFG_MASK)
		failures++;

	riscv_dasics_clear_hw();
	pr_info("[DASICS-CSR] supervisor summary total=%lu failed=%lu "
		"result=%s\n", total, failures,
		failures ? "FAIL" : "PASS");

	return failures;
}

static void riscv_dasics_enable_cpu(void *unused)
{
	csr_set(0x10c, DASICS_SSTATEEN0_CUSTOM_CSR);
	riscv_dasics_clear_hw();
}

static int __init riscv_dasics_init(void)
{
	unsigned long failures;

	on_each_cpu(riscv_dasics_enable_cpu, NULL, 1);
	failures = dasics_supervisor_mask_ops();
	failures += dasics_supervisor_smoke();
	WRITE_ONCE(riscv_dasics_ready, true);

	if (failures)
		pr_err("DASICS supervisor initialization failed: %lu checks\n",
		       failures);

	return failures ? -EINVAL : 0;
}
arch_initcall(riscv_dasics_init);
