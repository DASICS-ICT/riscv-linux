/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_AUXVEC_H
#define _LINUX_AUXVEC_H

#include <uapi/linux/auxvec.h>

#ifdef CONFIG_DASICS
#define AT_VECTOR_SIZE_BASE 31 /* NEW_AUX_ENT entries in auxiliary table */
#else
#define AT_VECTOR_SIZE_BASE 30 /* NEW_AUX_ENT entries in auxiliary table */
#endif
  /* number of "#define AT_.*" above, minus {AT_NULL, AT_IGNORE, AT_NOTELF} */
#endif /* _LINUX_AUXVEC_H */
