/* SPDX-License-Identifier: GPL-2.0-only */

#include <linux/linkage.h>
#include <asm/asm.h>
#include <asm/asm-offsets.h>

	.text
	.align 2
ENTRY(__dasics_hw_call)
	addi	sp, sp, -16
	REG_S	s1, 0(sp)
	REG_S	ra, 8(sp)
	mv	s1, a0

	REG_L	t3, DASICS_CALL_TARGET(s1)
	REG_L	a0, DASICS_CALL_A0(s1)
	REG_L	a1, DASICS_CALL_A1(s1)
	REG_L	a2, DASICS_CALL_A2(s1)
	REG_L	a3, DASICS_CALL_A3(s1)
	REG_L	a4, DASICS_CALL_A4(s1)
	REG_L	a5, DASICS_CALL_A5(s1)
	REG_L	a6, DASICS_CALL_A6(s1)
	REG_L	a7, DASICS_CALL_A7(s1)

	.word	0x000e108b /* dasicscall.jr ra, t3 */

	REG_S	a0, DASICS_CALL_RET_A0(s1)
	REG_S	a1, DASICS_CALL_RET_A1(s1)
	li	a0, 0
	REG_L	ra, 8(sp)
	REG_L	s1, 0(sp)
	addi	sp, sp, 16
	ret
ENDPROC(__dasics_hw_call)
