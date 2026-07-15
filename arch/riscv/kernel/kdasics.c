#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/preempt.h>

#include <asm/csr.h>    
#include <asm/kdasics.h>

static_assert(DASICS_MAX_DATA_BOUNDS * DASICS_LIBCFG_BITS == BITS_PER_LONG);
static_assert(DASICS_MAX_JUMP_BOUNDS * DASICS_JUMPCFG_BITS == BITS_PER_LONG);
static_assert(ARRAY_SIZE(((struct dasics_hw_state *)0)->lib_lo) ==
	      DASICS_MAX_DATA_BOUNDS);
static_assert(ARRAY_SIZE(((struct dasics_hw_state *)0)->lib_hi) ==
	      DASICS_MAX_DATA_BOUNDS);
static_assert(ARRAY_SIZE(((struct dasics_hw_state *)0)->jump_lo) ==
	      DASICS_MAX_JUMP_BOUNDS);
static_assert(ARRAY_SIZE(((struct dasics_hw_state *)0)->jump_hi) ==
	      DASICS_MAX_JUMP_BOUNDS);

static int dasics_hw_read_data_bound(unsigned int idx, unsigned long *lo,
				     unsigned long *hi)
{
	switch (idx) {
	case 0:
		*lo = csr_read(CSR_DLBOUND0LO);
		*hi = csr_read(CSR_DLBOUND0HI);
		break;
	case 1:
		*lo = csr_read(CSR_DLBOUND1LO);
		*hi = csr_read(CSR_DLBOUND1HI);
		break;
	case 2:
		*lo = csr_read(CSR_DLBOUND2LO);
		*hi = csr_read(CSR_DLBOUND2HI);
		break;
	case 3:
		*lo = csr_read(CSR_DLBOUND3LO);
		*hi = csr_read(CSR_DLBOUND3HI);
		break;
	case 4:
		*lo = csr_read(CSR_DLBOUND4LO);
		*hi = csr_read(CSR_DLBOUND4HI);
		break;
	case 5:
		*lo = csr_read(CSR_DLBOUND5LO);
		*hi = csr_read(CSR_DLBOUND5HI);
		break;
	case 6:
		*lo = csr_read(CSR_DLBOUND6LO);
		*hi = csr_read(CSR_DLBOUND6HI);
		break;
	case 7:
		*lo = csr_read(CSR_DLBOUND7LO);
		*hi = csr_read(CSR_DLBOUND7HI);
		break;
	case 8:
		*lo = csr_read(CSR_DLBOUND8LO);
		*hi = csr_read(CSR_DLBOUND8HI);
		break;
	case 9:
		*lo = csr_read(CSR_DLBOUND9LO);
		*hi = csr_read(CSR_DLBOUND9HI);
		break;
	case 10:
		*lo = csr_read(CSR_DLBOUND10LO);
		*hi = csr_read(CSR_DLBOUND10HI);
		break;
	case 11:
		*lo = csr_read(CSR_DLBOUND11LO);
		*hi = csr_read(CSR_DLBOUND11HI);
		break;
	case 12:
		*lo = csr_read(CSR_DLBOUND12LO);
		*hi = csr_read(CSR_DLBOUND12HI);
		break;
	case 13:
		*lo = csr_read(CSR_DLBOUND13LO);
		*hi = csr_read(CSR_DLBOUND13HI);
		break;
	case 14:
		*lo = csr_read(CSR_DLBOUND14LO);
		*hi = csr_read(CSR_DLBOUND14HI);
		break;
	case 15:
		*lo = csr_read(CSR_DLBOUND15LO);
		*hi = csr_read(CSR_DLBOUND15HI);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int dasics_hw_read_jump_bound(unsigned int idx, unsigned long *lo,
				     unsigned long *hi)
{
	switch (idx) {
	case 0:
		*lo = csr_read(CSR_DJBOUND0LO);
		*hi = csr_read(CSR_DJBOUND0HI);
		break;
	case 1:
		*lo = csr_read(CSR_DJBOUND1LO);
		*hi = csr_read(CSR_DJBOUND1HI);
		break;
	case 2:
		*lo = csr_read(CSR_DJBOUND2LO);
		*hi = csr_read(CSR_DJBOUND2HI);
		break;
	case 3:
		*lo = csr_read(CSR_DJBOUND3LO);
		*hi = csr_read(CSR_DJBOUND3HI);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int dasics_hw_save(struct dasics_hw_state *state)
{
	unsigned int idx;
	int ret = 0;

	if (!state)
		return -EINVAL;

	preempt_disable();
	state->libcfg = csr_read(CSR_DLCFG0);
	for (idx = 0; idx < DASICS_MAX_DATA_BOUNDS; idx++) {
		ret = dasics_hw_read_data_bound(idx, &state->lib_lo[idx],
						&state->lib_hi[idx]);
		if (ret)
			goto out;
	}

	state->jumpcfg = csr_read(CSR_DJCFG);
	for (idx = 0; idx < DASICS_MAX_JUMP_BOUNDS; idx++) {
		ret = dasics_hw_read_jump_bound(idx, &state->jump_lo[idx],
						&state->jump_hi[idx]);
		if (ret)
			goto out;
	}

	state->dmaincall = csr_read(CSR_DMAINCALL);
	state->dretpc = csr_read(CSR_DRETPC);
	state->dretpcactz = csr_read(CSR_DRETPCACTZ);

out:
	preempt_enable();
	return ret;
}
EXPORT_SYMBOL_GPL(dasics_hw_save);

#ifdef CONFIG_64BIT
#define STEP 8
#else 
#define STEP 4
#endif 

void dasics_init_umain_bound(uint64_t cfg, uint64_t hi, uint64_t lo)
{
	csr_write(CSR_DUMCFG, cfg);
	csr_write(CSR_DUMBOUNDHI, hi);
	csr_write(CSR_DUMBOUNDLO, lo);
}

void dasics_init_smaincall(uint64_t entry)
{
	csr_write(CSR_DMAINCALL, entry);
}

uint64_t dasics_smaincall(SmaincallTypes type, uint64_t arg0, uint64_t arg1)
{
    //save_smaincall();
    uint64_t retval = 0;

    // TODO: change to linux version
    const char *fmt = (const char*)arg0;
    switch (type)
    {
        case Smaincall_PRINT:
            printk(fmt, arg1);
            break;
        default:
            pr_cont("Warning: Invalid smaincall number %u!\n", type);
            break;
    }

    // TODO: Use compiler to optimize such ugly code in the future ...
    // Compile Error: gcc doesn't recognize pulpret. Use machine code instead.
    /*
    asm volatile("mv        a0, a5\n"\
                 "ld        ra, 88(sp)\n"\
                 "ld        s0, 80(sp)\n"\
                 "addi      sp, sp, 96\n"\
                 "ret\n"\
                 //".word 0x0000f00b \n" /* dasicsret x0,  0, x1 in little endian */ 
    /*             "nop");
    */
    //restore_smaincall();
    return retval;
}
EXPORT_SYMBOL(dasics_smaincall);

int32_t dasics_libcfg_kalloc(uint64_t cfg, uint64_t hi, uint64_t lo)
{
	u64 libcfg0 = csr_read(CSR_DLCFG0);

    //printk("[dasics_kallock] libcfg0: 0x%lx libcfg1: 0x%lx\n",libcfg0,libcfg1);

	s32 max_cfgs = DASICS_LIBCFG_WIDTH;
	s32 step = DASICS_LIBCFG_BITS;
    int32_t idx;

    for (idx = 0; idx < max_cfgs; ++idx)
    {
        uint64_t curr_cfg = (libcfg0 >> (idx * step)) & DASICS_LIBCFG_MASK;

        if ((curr_cfg & DASICS_LIBCFG_V) == 0)  // Find avaliable cfg
        {
            // Write DASICS boundary csrs
            switch (idx)
            {
                case 0:
			csr_write(CSR_DLBOUND0LO, lo);
			csr_write(CSR_DLBOUND0HI, hi);
                    break;
                case 1:
			csr_write(CSR_DLBOUND1LO, lo);
			csr_write(CSR_DLBOUND1HI, hi);
                    break;
                case 2:
			csr_write(CSR_DLBOUND2LO, lo);
			csr_write(CSR_DLBOUND2HI, hi);
                    break;
                case 3:
			csr_write(CSR_DLBOUND3LO, lo);
			csr_write(CSR_DLBOUND3HI, hi);
                    break;
                case 4:
			csr_write(CSR_DLBOUND4LO, lo);
			csr_write(CSR_DLBOUND4HI, hi);
                    break;
                case 5:
			csr_write(CSR_DLBOUND5LO, lo);
			csr_write(CSR_DLBOUND5HI, hi);
                    break;
                case 6:
			csr_write(CSR_DLBOUND6LO, lo);
			csr_write(CSR_DLBOUND6HI, hi);
                    break;
                case 7:
			csr_write(CSR_DLBOUND7LO, lo);
			csr_write(CSR_DLBOUND7HI, hi);
                    break;
                case 8:
			csr_write(CSR_DLBOUND8LO, lo);
			csr_write(CSR_DLBOUND8HI, hi);
                    break;
                case 9:
			csr_write(CSR_DLBOUND9LO, lo);
			csr_write(CSR_DLBOUND9HI, hi);
                    break;
                case 10:
			csr_write(CSR_DLBOUND10LO, lo);
			csr_write(CSR_DLBOUND10HI, hi);
                    break;
                case 11:
			csr_write(CSR_DLBOUND11LO, lo);
			csr_write(CSR_DLBOUND11HI, hi);
                    break;
                case 12:
			csr_write(CSR_DLBOUND12LO, lo);
			csr_write(CSR_DLBOUND12HI, hi);
                    break;
                case 13:
			csr_write(CSR_DLBOUND13LO, lo);
			csr_write(CSR_DLBOUND13HI, hi);
                    break;
                case 14:
			csr_write(CSR_DLBOUND14LO, lo);
			csr_write(CSR_DLBOUND14HI, hi);
                    break;
                default:
			csr_write(CSR_DLBOUND15LO, lo);
			csr_write(CSR_DLBOUND15HI, hi);
                    break;
            }

            libcfg0 &= ~(DASICS_LIBCFG_MASK << (idx * step));
            libcfg0 |= ((cfg | DASICS_LIBCFG_V) & DASICS_LIBCFG_MASK) << (idx * step);
			csr_write(CSR_DLCFG0, libcfg0);
            return idx;
        }
    }

    return -1;
}
EXPORT_SYMBOL(dasics_libcfg_kalloc);

int32_t dasics_libcfg_kfree(int32_t idx)
{
	s32 step = DASICS_LIBCFG_BITS;
	u64 libcfg;

    if (idx < 0 || idx >= DASICS_LIBCFG_WIDTH)
    {
        return -1;
    }

	libcfg = csr_read(CSR_DLCFG0);
    libcfg &= ~(DASICS_LIBCFG_V << (idx * step));

	csr_write(CSR_DLCFG0, libcfg);

    return 0;
}
EXPORT_SYMBOL(dasics_libcfg_kfree);

uint32_t dasics_libcfg_kget(int32_t idx)
{
	s32 step = DASICS_LIBCFG_BITS;
	u64 libcfg;

    if (idx < 0 || idx >= DASICS_LIBCFG_WIDTH)
    {
        return -1;
    }

	libcfg = csr_read(CSR_DLCFG0);

    return (libcfg >> (idx * step)) & DASICS_LIBCFG_MASK;
}

int32_t ATTR_SMAIN_TEXT dasics_jumpcfg_kalloc(uint64_t lo, uint64_t hi)
{
	u64 jumpcfg = csr_read(CSR_DJCFG);
	s32 max_cfgs = DASICS_JUMPCFG_WIDTH;
	s32 step = DASICS_JUMPCFG_BITS;

    int32_t idx;
    for (idx = 0; idx < max_cfgs; ++idx) {
        uint64_t curr_cfg = (jumpcfg >> (idx * step)) & DASICS_JUMPCFG_MASK;
        if ((curr_cfg & DASICS_JUMPCFG_V) == 0) // found available cfg
        {
            // Write DASICS jump boundary CSRs
            switch (idx) {
                case 0:
			csr_write(CSR_DJBOUND0LO, lo);
			csr_write(CSR_DJBOUND0HI, hi);
                    break;
                case 1:
			csr_write(CSR_DJBOUND1LO, lo);
			csr_write(CSR_DJBOUND1HI, hi);
                    break;
                case 2:
			csr_write(CSR_DJBOUND2LO, lo);
			csr_write(CSR_DJBOUND2HI, hi);
                    break;
                case 3:
			csr_write(CSR_DJBOUND3LO, lo);
			csr_write(CSR_DJBOUND3HI, hi);
                    break;
                default:
                    break;
            }

            jumpcfg &= ~(DASICS_JUMPCFG_MASK << (idx * step));
            jumpcfg |= DASICS_JUMPCFG_V << (idx * step);
			csr_write(CSR_DJCFG, jumpcfg);

            return idx;
        }
    }

    return -1;
}
EXPORT_SYMBOL(dasics_jumpcfg_kalloc);

int32_t ATTR_SMAIN_TEXT dasics_jumpcfg_kfree(int32_t idx) {
	s32 step = DASICS_JUMPCFG_BITS;
	u64 jumpcfg;

    if (idx < 0 || idx >= DASICS_JUMPCFG_WIDTH) {
        return -1;
    }

	jumpcfg = csr_read(CSR_DJCFG);
    jumpcfg &= ~(DASICS_JUMPCFG_V << (idx * step));
	csr_write(CSR_DJCFG, jumpcfg);
    return 0;
}
EXPORT_SYMBOL(dasics_jumpcfg_kfree);

uint32_t dasics_jumpcfg_get(int32_t idx) {
	s32 step = DASICS_JUMPCFG_BITS;
	u64 jumpcfg;

    if (idx < 0 || idx >= DASICS_JUMPCFG_WIDTH) {
        return -1;
    }

	jumpcfg = csr_read(CSR_DJCFG);

    return (jumpcfg >> (idx * step)) & DASICS_JUMPCFG_MASK;
}

void klib_call(void* func_name, ...) {
    register long a0 asm("a0") = (long)func_name;

    __asm__ __volatile__ (
        "addi sp, sp, -8\n\t"
        "sd ra, 0(sp)\n\t"
        ".word 0x0005108b\n\t"  // dasicscall.jr ra, a0
        "ld ra, 0(sp)\n\t"
        "addi sp, sp, 8\n\t"
        : "+r" (a0)  // a0 as input / output
        : 
        : "memory", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "t0", "t1", "t2", "t3", "t4", "t5", "t6"
    );
}

int ret_klib_call(void* func_name, ...) {
    uint64_t ret;
    __asm__ __volatile__ (
        "addi sp, sp, -8\n\t"
        "sd ra, 0(sp)\n\t"
        "mv t3, a0\n\t"
        "mv a0, a1\n\t"          // 第二个参数从a1移动到a0
        ".word 0x000e108b\n\t"   // dasicscall.jr ra, t3
        "ld ra, 0(sp)\n\t"
        "addi sp, sp, 8\n\t"
        : "=r" (ret)             // 结果直接存入ret（使用a0寄存器）
        : 
        : "memory", "a1", "a2", "a3", "t3"
    );
    return ret;
}
EXPORT_SYMBOL(ret_klib_call);

void register_kdasics(uint64_t funcptr) {
    // Set smaincall & sfault handler
    uint64_t smaincall_helper = (funcptr != 0) ? funcptr : (uint64_t) dasics_smaincall;
	csr_write(CSR_DMAINCALL, (uint64_t)smaincall_helper);
    //csr_write(0x005, (uint64_t)dasics_ufault_entry);
}
EXPORT_SYMBOL(register_kdasics);


void unregister_kdasics(void) {
    ;
}
EXPORT_SYMBOL(unregister_kdasics);
/*
void save_smaincall(void) {
    asm volatile (
        "addi sp, sp, -OFFSET_SMAINCALL\n"           // 调整栈指针
        "sd t0, OFFSET_SMAINCALL_T0(sp)\n"           // 保存 t0
        "sd t1, OFFSET_SMAINCALL_T1(sp)\n"           // 保存 t1
        "sd t3, OFFSET_SMAINCALL_T3(sp)\n"           // 保存 t3
        "sd ra, OFFSET_SMAINCALL_RA(sp)\n"           // 保存 ra
        "sd a0, OFFSET_SMAINCALL_A0(sp)\n"           // 保存 a0
        "sd a1, OFFSET_SMAINCALL_A1(sp)\n"           // 保存 a1
        "sd a2, OFFSET_SMAINCALL_A2(sp)\n"           // 保存 a2
        "sd a3, OFFSET_SMAINCALL_A3(sp)\n"           // 保存 a3
        "sd a4, OFFSET_SMAINCALL_A4(sp)\n"           // 保存 a4
        "sd a5, OFFSET_SMAINCALL_A5(sp)\n"           // 保存 a5
        "sd a6, OFFSET_SMAINCALL_A6(sp)\n"           // 保存 a6
        "sd a7, OFFSET_SMAINCALL_A7(sp)\n"           // 保存 a7
        "addi t0, sp, OFFSET_SMAINCALL\n"            // 计算原始 sp 值
        "sd t0, OFFSET_SMAINCALL_SP(sp\n)"             // 保存原始 sp
        : "sp", "t0", "memory"                // 破坏列表：修改的寄存器和内存
    );
}


void restore_smaincall(void) {
    asm volatile (
        "ld t0, OFFSET_SMAINCALL_T0(sp)\n"
        "ld t1, OFFSET_SMAINCALL_T1(sp)\n"
        "ld t3, OFFSET_SMAINCALL_T3(sp)\n"
        "ld ra, OFFSET_SMAINCALL_RA(sp)\n"
        "ld a0, OFFSET_SMAINCALL_A0(sp)\n"
        "ld a1, OFFSET_SMAINCALL_A1(sp)\n"
        "ld a2, OFFSET_SMAINCALL_A2(sp)\n"
        "ld a3, OFFSET_SMAINCALL_A3(sp)\n"
        "ld a4, OFFSET_SMAINCALL_A4(sp)\n"
        "ld a5, OFFSET_SMAINCALL_A5(sp)\n"
        "ld a6, OFFSET_SMAINCALL_A6(sp)\n"
        "ld a7, OFFSET_SMAINCALL_A7(sp)\n"
        "ld sp, OFFSET_SMAINCALL_SP(sp)\n"
        : "sp", "t0", "t1", "t3", "ra", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "memory"
    );
}
    */
