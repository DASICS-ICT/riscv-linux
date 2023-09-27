#ifndef _ASM_RISCV_PKU_H
#define _ASM_RISCV_PKU_H

#include <asm/csr.h>

#define PKRU_AD_BIT 0x1
#define PKRU_WD_BIT 0x2
#define PKRU_BITS_PER_PKEY 2
#define PKRU_NUM_PKEYS 32

#ifdef CONFIG_RISCV_MEMORY_PROTECTION_KEYS
static inline bool is_pkru_enabled(void)
{
    u64 spkctl = csr_read(0x9c0);
    return spkctl & SPKCTL_PKE;
}

static inline void enable_pkru(void)
{
    u64 spkctl = csr_read(0x9c0);
    csr_write(0x9c0, spkctl | SPKCTL_PKE);
}

static inline void disable_pkru(void)
{
    u64 spkctl = csr_read(0x9c0);
    csr_write(0x9c0, spkctl & (~SPKCTL_PKE));
}

static inline u64 read_pkru(void)
{
    return is_pkru_enabled() ? csr_read(0x800) : 0;
}

static inline void write_pkru(u64 val)
{
    if (is_pkru_enabled())
        csr_write(0x800, val);
}

static inline bool pkru_allows_read(int pkey)
{
    u64 pkru = read_pkru();
    int pkru_pkey_bits = pkey * PKRU_BITS_PER_PKEY;
    return !(pkru & (PKRU_AD_BIT << pkru_pkey_bits));
}

static inline bool pkru_allows_write(int pkey)
{
    u64 pkru = read_pkru();
    int pkru_pkey_bits = pkey * PKRU_BITS_PER_PKEY;
    /*
	 * Access-disable disables writes too so we need to check
	 * both bits here.
	 */
	return !(pkru & ((PKRU_AD_BIT|PKRU_WD_BIT) << pkru_pkey_bits));
}

static inline u64 pkru_init_val(void)
{
    u64 init_val = 0;
    /*
    * Make the default PKRU value (at execve() time) as restrictive
    * as possible.  This ensures that any threads clone()'d early
    * in the process's lifetime will not accidentally get access
    * to data which is pkey-protected later on.
    */
    int i;
    for (i = 1; i < PKRU_NUM_PKEYS; ++i)
    {
        init_val |= (((u64)PKRU_AD_BIT) << (i * PKRU_BITS_PER_PKEY));
    }

    return init_val;
}
#else
static inline bool is_pkru_enabled(void)
{
    return false;
}

static inline void enable_pkru(void)
{
}

static inline void disable_pkru(void)
{
}

static inline u64 read_pkru(void)
{
    return 0;
}

static inline void write_pkru(u64 val)
{
}

static inline bool pkru_allows_read(int pkey)
{
    return true;
}

static inline bool pkru_allows_write(int pkey)
{
    return true;
}

static inline u64 pkru_init_val(void)
{
    return 0;
}
#endif  /* CONFIG_RISCV_MEMORY_PROTECTION_KEYS */

#endif  /* _ASM_RISCV_PKU_H */