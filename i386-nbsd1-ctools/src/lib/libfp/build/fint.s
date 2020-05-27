/*
; ; ; ; ;
;	80960 FPAC/DPAC		Floating Point Library
;	FPCNVT			Conversions (SP)
;
;	Copyright (C) 1990 By
;	United States Software Corporation
;	14215 N.W. Science Park Drive
;	Portland, Oregon 97229
;
;	This software is furnished under a license and may be used
;	and copied only in accordance with the terms of such license
;	and with the inclusion of the above copyright notice.
;	This software or any other copies thereof may not be provided
;	or otherwise made available to any other person.  No title to
;	and ownership of the software is hereby transferred.
;
;	The information in this software is subject to change without 
;	notice and should not be construed as a commitment by United
;	States Software Corporation.
;
;	Version:	See VSNLOG.TXT
;	Released:	31 January 1990
; ; ; ; ;
*/

	.file	"fint.s"
	.link_pix

	.globl	_ceilf
_ceilf:
	/* faint misbehaves for -0.0, so check for either 0.0 or -0.0 before
	   calling faint. */
	setbit	31,0,g2		/* g2 <- 0x80000000 */
	andnot	g2,g0,g2
	cmpo	0,g2
	bne	L100
	ret			/* just return 0.0 */

L100:
	/* negate the number */
	notbit	31,g0,g0
	callj	_faint
	/* negate the result */
	notbit	31,g0,g0
	ret
	

	.globl	_floorf
_floorf:
	/* faint misbehaves for -0.0, so check for either 0.0 or -0.0 before
	   calling faint. */
	setbit	31,0,g2		/* g2 <- 0x80000000 */
	andnot	g2,g0,g2
	cmpo	0,g2
	bne	_faint
	ret			/* just return 0.0 */

/*  Routine to convert a single-precision floating point number to whole
    floating-point number.
    **** This routine truncates DOWN ***
    On entry:  g0 = floating-point number
    On exit:   g0 = truncated floating-point number
*/

	.globl	_faint

_faint:
	shlo	1,g0,r4		#exponent -> r4
	shro	24,r4,r4

	lda	150,r12		#bit number of the "one" bit -> r12
	subo	r4,r12,r12

	cmpi	r12,0		#if no decimals, return unchanged
	ble.f	Lfai99

	cmpi	g0,0		#branch for negative number
	bl.f	Lfai10

	cmpi	r12,23		#if < 1 return 0
	bg.f	Lfai5

	shro	r12,g0,g0	#shift right and left to clear decimals
	shlo	r12,g0,g0

Lfai99:	ret

Lfai5:	mov	0,g0		#result dropped to zero
	b	Lfai99

Lfai10:	cmpi	r12,23		#if number (-1 to 0) return -1
	bg.f	Lfai16

	subo	g0,0,g0		#change to 2's complement
	shro	r12,g0,g0	#    shift right and left to clear decimals
	shlo	r12,g0,g0
	subo	g0,0,g0

	b	Lfai99

Lfai16:	lda	0xbf800000,g0	#return -1
	b	Lfai99


/*  Routine to convert a single-precision floating point number to an integer.
    **** This routine truncates DOWN ***
    On entry:  g0 = floating-point number
    On exit:   g0 = int(A)
*/

	.globl	_fpint

_fpint:
	shlo	1,g0,r4		#exponent -> r4
	lda	0xff000000,r12
	cmpo	r4,r12
	bg.f	Lfin26
	shro	24,r4,r4

	shlo	7,g0,r5		#mantissa into r5
	setbit	30,r5,r5
	clrbit	31,r5,r5

	cmpi	g0,0		#handle + and - separately for speed
	lda	157,r12
	bl.f	Lfin10

	cmpo	r4,r12		#just leave the integer
	subo	r4,r12,r4
	bg.f	Lfin3
	shri	r4,r5,g0

Lfin99:	ret

Lfin3:	subo	1,0,g0		#here exponent is too large, return INF
	clrbit	31,g0,g0
	b	Lfin99

Lfin10:	cmpo	r4,r12		# - case, complement number, shift in place
	subo	r4,r12,r4
	bg.f	Lfin16
	subo	r5,0,r5
	shri	r4,r5,g0

	b	Lfin99

Lfin16:	shlo	31,1,g0		#exponent too large, return -INF
	b	Lfin99

Lfin26:	mov	0,g0		#NaN becomes 0
	b	Lfin99
