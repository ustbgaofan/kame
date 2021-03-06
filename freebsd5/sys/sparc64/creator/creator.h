/*-
 * Copyright (c) 2003 Jake Burkholder.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/sparc64/creator/creator.h,v 1.1 2003/08/24 01:15:40 jake Exp $
 */

#ifndef _DEV_FB_CREATOR_H_
#define	_DEV_FB_CREATOR_H_

#define	FFB_NREG		24

#define	FFB_DAC			1
#define	FFB_DAC_TYPE		0x0
#define	FFB_DAC_VALUE		0x4
#define	FFB_DAC_TYPE2		0x8
#define	FFB_DAC_VALUE2		0xc

#define	FFB_FBC			2
#define	FFB_FBC_BY		0x60
#define	FFB_FBC_BX		0x64
#define	FFB_FBC_DY		0x68
#define	FFB_FBC_DX		0x6c
#define	FFB_FBC_BH		0x70
#define	FFB_FBC_BW		0x74
#define	FFB_FBC_PPC		0x200		/* Pixel Processor Control */
#define	FFB_FBC_FG		0x208		/* Foreground */
#define	FFB_FBC_BG		0x20c		/* Background */
#define	FFB_FBC_FBC		0x254		/* Frame Buffer Control */
#define	FFB_FBC_ROP		0x258		/* Raster Operation */
#define	FFB_FBC_PMASK		0x290		/* Pixel Mask */
#define	FFB_FBC_DRAWOP		0x300		/* Draw Operation */
#define	FFB_FBC_FONTXY		0x314		/* Font X/Y */
#define	FFB_FBC_FONTW		0x318		/* Font Width */
#define	FFB_FBC_FONTINC		0x31c		/* Font Increment */
#define	FFB_FBC_FONT		0x320		/* Font Data */
#define	FFB_FBC_UCSR		0x900		/* User Control & Status */

#define	FBC_PPC_VCE_DIS		0x00001000
#define	FBC_PPC_APE_DIS		0x00000800
#define	FBC_PPC_TBE_OPAQUE	0x00000200
#define	FBC_PPC_CS_CONST	0x00000003

#define	FFB_FBC_WB_A		0x20000000
#define	FFB_FBC_RB_A		0x00004000
#define	FFB_FBC_SB_BOTH		0x00003000
#define	FFB_FBC_XE_OFF		0x00000040
#define	FFB_FBC_RGBE_MASK	0x0000003f

#define	FBC_ROP_NEW		0x83

#define	FBC_DRAWOP_RECTANGLE	0x08

#define	FBC_UCSR_FIFO_OVFL	0x80000000
#define	FBC_UCSR_READ_ERR	0x40000000
#define	FBC_UCSR_RP_BUSY	0x02000000
#define	FBC_UCSR_FB_BUSY	0x01000000
#define	FBC_UCSR_FIFO_MASK	0x00000fff

#define	FFB_VIRT_SFB8R		0x00000000
#define	FFB_VIRT_SFB8G		0x00400000
#define	FFB_VIRT_SFB8B		0x00800000
#define	FFB_VIRT_SFB8X		0x00c00000
#define	FFB_VIRT_SFB32		0x01000000
#define	FFB_VIRT_SFB64		0x02000000
#define	FFB_VIRT_FBC		0x04000000
#define	FFB_VIRT_FBC_BM		0x04002000
#define	FFB_VIRT_DFB8R		0x04004000
#define	FFB_VIRT_DFB8G		0x04404000
#define	FFB_VIRT_DFB8B		0x04804000
#define	FFB_VIRT_DFB8X		0x04c04000
#define	FFB_VIRT_DFB24		0x05004000
#define	FFB_VIRT_DFB32		0x06004000
#define	FFB_VIRT_DFB422A	0x07004000
#define	FFB_VIRT_DFB422AD	0x07804000
#define	FFB_VIRT_DFB24B		0x08004000
#define	FFB_VIRT_DFB422B	0x09004000
#define	FFB_VIRT_DFB422BD	0x09804000
#define	FFB_VIRT_SFB16Z		0x0a004000
#define	FFB_VIRT_SFB8Z		0x0a404000
#define	FFB_VIRT_SFB422		0x0ac04000
#define	FFB_VIRT_SFB422D	0x0b404000
#define	FFB_VIRT_FBC_KREG	0x0bc04000
#define	FFB_VIRT_DAC		0x0bc06000
#define	FFB_VIRT_PROM		0x0bc08000
#define	FFB_VIRT_EXP		0x0bc18000

#define	FFB_PHYS_SFB8R		0x04000000
#define	FFB_PHYS_SFB8G		0x04400000
#define	FFB_PHYS_SFB8B		0x04800000
#define	FFB_PHYS_SFB8X		0x04c00000
#define	FFB_PHYS_SFB32		0x05000000
#define	FFB_PHYS_SFB64		0x06000000
#define	FFB_PHYS_FBC		0x00600000
#define	FFB_PHYS_FBC_BM		0x00600000
#define	FFB_PHYS_DFB8R		0x01000000
#define	FFB_PHYS_DFB8G		0x01400000
#define	FFB_PHYS_DFB8B		0x01800000
#define	FFB_PHYS_DFB8X		0x01c00000
#define	FFB_PHYS_DFB24		0x02000000
#define	FFB_PHYS_DFB32		0x03000000
#define	FFB_PHYS_DFB422A	0x09000000
#define	FFB_PHYS_DFB422AD	0x09800000
#define	FFB_PHYS_DFB24B		0x0a000000
#define	FFB_PHYS_DFB422B	0x0b000000
#define	FFB_PHYS_DFB422BD	0x0b800000
#define	FFB_PHYS_SFB16Z		0x0c800000
#define	FFB_PHYS_SFB8Z		0x0c000000
#define	FFB_PHYS_SFB422		0x0d000000
#define	FFB_PHYS_SFB422D	0x0d800000
#define	FFB_PHYS_FBC_KREG	0x00610000
#define	FFB_PHYS_DAC		0x00400000
#define	FFB_PHYS_PROM		0x00000000
#define	FFB_PHYS_EXP		0x00200000

#define	FFB_READ(sc, reg, off) \
	bus_space_read_4((sc)->sc_bt[(reg)], (sc)->sc_bh[(reg)], (off))
#define	FFB_WRITE(sc, reg, off, val) \
	bus_space_write_4((sc)->sc_bt[(reg)], (sc)->sc_bh[(reg)], (off), (val))

struct creator_softc {
	video_adapter_t		sc_va;			/* XXX must be first */

	dev_t			sc_si;

	struct resource		*sc_reg[FFB_NREG];
	bus_space_tag_t		sc_bt[FFB_NREG];
	bus_space_handle_t	sc_bh[FFB_NREG];
	char			sc_model[32];
	int			sc_console;
	int			sc_dac;

	int			sc_height;
	int			sc_width;
	int			sc_ncol;
	int			sc_nrow;

	int			sc_xmargin;
	int			sc_ymargin;

	u_char			*sc_font;
	int			*sc_rowp;
	int			*sc_colp;

	int			sc_bg_cache;
	int			sc_fg_cache;
	int			sc_fifo_cache;
};

#endif
