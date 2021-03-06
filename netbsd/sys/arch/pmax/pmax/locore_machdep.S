/*	$NetBSD: locore_machdep.S,v 1.9 1999/03/25 01:17:53 simonb Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Digital Equipment Corporation and Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Copyright (C) 1989 Digital Equipment Corporation.
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies.
 * Digital Equipment Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * from: Header: /sprite/src/kernel/mach/ds3100.md/RCS/loMem.s,
 *	v 1.1 89/07/11 17:55:04 nelson Exp  SPRITE (DECWRL)
 * from: Header: /sprite/src/kernel/mach/ds3100.md/RCS/machAsm.s,
 *	v 9.2 90/01/29 18:00:39 shirriff Exp  SPRITE (DECWRL)
 * from: Header: /sprite/src/kernel/vm/ds3100.md/vmPmaxAsm.s,
 *	v 1.1 89/07/10 14:27:41 nelson Exp  SPRITE (DECWRL)
 *
 *	@(#)locore.s	8.5 (Berkeley) 1/4/94
 */


/*
 * DEcstation-specific mips locore code.
 */

#include <mips/asm.h>
#include <mips/cpuregs.h>	/* XXX - misnomer? */

#include "opt_dec_3100.h"
#include "opt_dec_5100.h"
#include "opt_dec_5400.h"

	.set	noreorder
#if defined(DEC_3100) || defined(DEC_5100)

/*
 * Copy data to a DMA buffer padded with 16 bits of data, 16
 * bits of padding per 32bit word (e.g., for pmin/pmax  sii (and lance) DMA).
 *
 * The DMA bufffer can only be written one short at a time
 * (and takes ~14 cycles).
 *
 *	CopyToBuffer(src, dst, length)
 *		u_short *src;	NOTE: must be short aligned
 *		u_short *dst;
 *		int length;
 */
LEAF(CopyToBuffer)
	blez	a2, 2f
	nop
1:
	lhu	t0, 0(a0)		# read 2 bytes of data
	subu	a2, a2, 2
	addu	a0, a0, 2
	addu	a1, a1, 4
	bgtz	a2, 1b
	sh	t0, -4(a1)		# write 2 bytes of data to buffer
2:
	j	ra
	nop
END(CopyToBuffer)

/*
 * Copy data from the DMA buffer.
 * The DMA bufffer can only be read one short at a time
 * (and takes ~12 cycles).
 *
 *	CopyFromBuffer(src, dst, length)
 *		u_short *src;
 *		char *dst;
 *		int length;
 */
LEAF(CopyFromBuffer)
	and	t0, a1, 1		# test for aligned dst
	beq	t0, zero, 3f
	nop
	blt	a2, 2, 7f		# at least 2 bytes to copy?
	nop
1:
	lhu	t0, 0(a0)		# read 2 bytes of data from buffer
	addu	a0, a0, 4		# keep buffer pointer word aligned
	addu	a1, a1, 2
	subu	a2, a2, 2
	sb	t0, -2(a1)
	srl	t0, t0, 8
	bge	a2, 2, 1b
	sb	t0, -1(a1)
3:
	blt	a2, 2, 7f		# at least 2 bytes to copy?
	nop
6:
	lhu	t0, 0(a0)		# read 2 bytes of data from buffer
	addu	a0, a0, 4		# keep buffer pointer word aligned
	addu	a1, a1, 2
	subu	a2, a2, 2
	bge	a2, 2, 6b
	sh	t0, -2(a1)
7:
	ble	a2, zero, 9f		# done?
	nop
	lhu	t0, 0(a0)		# copy one more byte
	nop
	sb	t0, 0(a1)
9:
	j	ra
	nop
END(CopyFromBuffer)
#endif /* DEC_3100 || DEC_5100 */


/*
 * Write-buffer flush for writebuffer hardware on DEC mips r2000a
 */
#if defined(DEC_5100) || defined(DEC_5400)
LEAF(dec_mips1_wbflush)
        mfc0	v0, MIPS_COP_0_STATUS_REG	# save original SR in v0
	li	t0, 0x80000000			# set CU3 bit
	or	v1, v0 ,t0			# v1 = v0 | 0x80000000
	nop
	mtc0	v1, MIPS_COP_0_STATUS_REG
	nop				/* two  more cycles to complete */
 	nop

1:						# spin
	bc3f    1b				# until write buffer drained
        nop                     		# (branch delay slot)

        mtc0    v0, MIPS_COP_0_STATUS_REG	# restore SR on exit
        nop
        j       ra
        nop
END(dec_mips1_wbflush)
#endif	/* defined(DEC_5100) || defined(DEC_5400) */

/*
 * Set/clear software interrupt routines.
 */

LEAF(setsoftclock)
	mfc0	v1, MIPS_COP_0_STATUS_REG	# save status register
	mtc0	zero, MIPS_COP_0_STATUS_REG	# disable interrupts (2 cycles)
	nop
	mfc0	v0, MIPS_COP_0_CAUSE_REG	# read cause register
	nop
	or	v0, v0, MIPS_SOFT_INT_MASK_0	# set soft clock interrupt
	mtc0	v0, MIPS_COP_0_CAUSE_REG	# save it
	mtc0	v1, MIPS_COP_0_STATUS_REG
	j	ra
	nop
END(setsoftclock)

LEAF(clearsoftclock)
	mfc0	v1, MIPS_COP_0_STATUS_REG	# save status register
	mtc0	zero, MIPS_COP_0_STATUS_REG	# disable interrupts (2 cycles)
	nop
	nop
	mfc0	v0, MIPS_COP_0_CAUSE_REG	# read cause register
	nop
	and	v0, v0, ~MIPS_SOFT_INT_MASK_0	# clear soft clock interrupt
	mtc0	v0, MIPS_COP_0_CAUSE_REG	# save it
	mtc0	v1, MIPS_COP_0_STATUS_REG
	j	ra
	nop
END(clearsoftclock)

LEAF(setsoftnet)
	mfc0	v1, MIPS_COP_0_STATUS_REG	# save status register
	mtc0	zero, MIPS_COP_0_STATUS_REG	# disable interrupts (2 cycles)
	nop
	nop
	mfc0	v0, MIPS_COP_0_CAUSE_REG	# read cause register
	nop
	or	v0, v0, MIPS_SOFT_INT_MASK_1	# set soft net interrupt
	mtc0	v0, MIPS_COP_0_CAUSE_REG	# save it
	mtc0	v1, MIPS_COP_0_STATUS_REG
	j	ra
	nop
END(setsoftnet)

LEAF(clearsoftnet)
	mfc0	v1, MIPS_COP_0_STATUS_REG	# save status register
	mtc0	zero, MIPS_COP_0_STATUS_REG	# disable interrupts (2 cycles)
	nop
	nop
	mfc0	v0, MIPS_COP_0_CAUSE_REG	# read cause register
	nop
	and	v0, v0, ~MIPS_SOFT_INT_MASK_1	# clear soft net interrupt
	mtc0	v0, MIPS_COP_0_CAUSE_REG	# save it
	mtc0	v1, MIPS_COP_0_STATUS_REG
	j	ra
	nop
END(clearsoftnet)

/*
 * Set/change interrupt priority routines.
 */

LEAF(spl0)
ALEAF(spllow)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	nop
	or	t0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
	mtc0	t0, MIPS_COP_0_STATUS_REG	# enable all interrupts
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(spl0)

LEAF(splsoftclock)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~MIPS_SOFT_INT_MASK_0	# disable soft clock
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(splsoftclock)

LEAF(splsoftnet)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(splsoftnet)

/*
 * hardware-level spls for hardware where the device interrupt priorites
 * are ordered, and map onto mips interrupt pins in increasing priority.
 * This maps directly onto BSD spl levels.
 */

/*
 * Block out int2 (hardware interrupt 0) and lower mips levels.
 */
LEAF(cpu_spl0)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_SPL0)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(cpu_spl0)

/*
 * Block out Int3 (hardware interrupt 1) and lower mips levels.
 */
LEAF(cpu_spl1)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_SPL1)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(cpu_spl1)

LEAF(cpu_spl2)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_SPL2)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(cpu_spl2)

LEAF(cpu_spl3)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_SPL3)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(cpu_spl3)

LEAF(cpu_spl4)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_SPL4)

	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(cpu_spl4)

LEAF(cpu_spl5)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_SPL5)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(cpu_spl5)

/*
 * hardware-level spls for hardware where the interrupt priorites
 * DO NOT  map onto levels.
 *
 * For now, that means DECstations that use only two distinct CPU
 * levels, one for TOD clock interrupts, and a second for all other
 * external devices (via an external controller.
 * XXX  the spl handling really needs re-writing from scratch.
 */
LEAF(Mach_spl0)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_0|MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(Mach_spl0)

LEAF(Mach_spl1)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_1|MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(Mach_spl1)

LEAF(Mach_spl2)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_2|MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(Mach_spl2)

LEAF(Mach_spl3)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_3|MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(Mach_spl3)

LEAF(Mach_spl4)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_4|MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(Mach_spl4)

LEAF(Mach_spl5)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_5|MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(Mach_spl5)


/*
 * We define an alternate entry point after mcount is called so it
 * can be used in mcount without causeing a recursive loop.
 */
LEAF(splhigh)
ALEAF(_splhigh)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~MIPS_SR_INT_IE	# disable all interrupts
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(splhigh)

/*
 * Restore saved interrupt mask.
 */
LEAF(splx)
ALEAF(_splx)
	mfc0	v0, MIPS_COP_0_STATUS_REG
	li	t0, ~(MIPS_INT_MASK | MIPS_SR_INT_IE)
	and	t0, t0, v0
	or	t0, t0, a0
	mtc0	t0, MIPS_COP_0_STATUS_REG
	nop					# 3 ins to disable
	j	ra
	nop
	.globl	_C_LABEL(splx_end)
_C_LABEL(splx_end):
END(splx)
/*
 * Interrupt counters for vmstat.
 */
	.data
	.globl _C_LABEL(intrcnt)
	.globl _C_LABEL(eintrcnt)
	.globl _C_LABEL(intrnames)
	.globl _C_LABEL(eintrnames)
_C_LABEL(intrnames):
	.asciiz	"softclock"
	.asciiz	"softnet"
	.asciiz	"serial0"
	.asciiz	"serial1"
	.asciiz	"serial2"
	.asciiz	"ether"
	.asciiz	"scsi"
	.asciiz	"memory"
	.asciiz	"clock"
	.asciiz	"fpu"
	.asciiz	"tcslot0"
	.asciiz	"tcslot1"
	.asciiz	"tcslot2"
	.asciiz "dtop"
	.asciiz "isdn"
	.asciiz	"floppy"
	.asciiz	"stray"
	.asciiz	"nmi"
	.asciiz	"lostclock"
_C_LABEL(eintrnames):
	.align	2
_C_LABEL(intrcnt):
	.word	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0
_C_LABEL(eintrcnt):
	.word	0	# This shouldn't be needed but with 4.4bsd's as, the eintrcnt
			#  label ends end up in a different section otherwise.
