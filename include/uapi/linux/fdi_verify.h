/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_LINUX_FDI_VERIFY_H
#define _UAPI_LINUX_FDI_VERIFY_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define FDI_VERIFY_VERSION 1ULL
#define FDI_VERIFY_CBO 1ULL
#define FDI_VERIFY_ENVCFG_MASK 0xf0ULL

#define FDI_VERIFY_CSR_READ 1ULL
#define FDI_VERIFY_CSR_WRITE 2ULL
#define FDI_VERIFY_FDICALL 3ULL

/* BEGIN consumes version/flags; ARM consumes version/kind/site. All other
 * fields are kernel observations. QUERY and END consume only version.
 * Closing any duplicated or inherited descriptor ends its shared test window;
 * a new BEGIN is needed before using a remaining descriptor for another test. */
struct fdi_verify_result {
	__u64 version, flags, kind, site, instruction;
	__u64 cookie, active, pid;
	__u64 menvcfg, senvcfg, shadow_envcfg, saved_envcfg;
	__u64 cbom_block_size, cboz_block_size;
	__u64 illegal_count, cause, epc, tval, freason, return_pc;
	__u64 gpr[32];
};

#define FDI_VERIFY_BEGIN _IOWR('F', 0x80, struct fdi_verify_result)
#define FDI_VERIFY_ARM   _IOWR('F', 0x81, struct fdi_verify_result)
#define FDI_VERIFY_QUERY _IOWR('F', 0x82, struct fdi_verify_result)
#define FDI_VERIFY_END   _IOWR('F', 0x83, struct fdi_verify_result)

#endif
