/* SPDX-License-Identifier: GPL-2.0 */
/* Written 2003 by Andi Kleen, based on a kernel by Evandro Menezes */

/*
 * Some CPUs run faster using the string copy instructions (sane microcode).
 * It is also a lot simpler. Use this when possible. But, don't use streaming
 * copy unless the CPU indicates X86_FEATURE_REP_GOOD. Could vary the
 * prefetch distance based on SMP/UP.
 */
.align 4,0x90
.section .text
.global copy_page
.type copy_page, @function

copy_page:
	movl	$4096/8, %ecx
	rep	movsq
	ret
