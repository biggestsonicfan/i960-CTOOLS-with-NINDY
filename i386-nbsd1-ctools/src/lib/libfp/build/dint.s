/*
; ; ; ; ;
;	80960 FPAC/DPAC		Floating Point Library
;	DPCNVT			Conversions (DP)
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
	.file	"dint.s"
	.link_pix
	.globl	_ceil
_ceil:
	/* daint misbehaves for -0.0, so check for either 0.0 or -0.0 before
	   calling daint. */
	cmpo	0,g0
	setbit	31,0,g2		/* g2 <- 0x80000000 */
	andnot	g2,g1,g2
	bne	L100
	cmpo	0,g2
	bne	L100
	ret			/* just return -0.0 */

L100:
	/* negate the number */
	notbit	31,g1,g1
	callj	_daint
	/* negate the result */
	notbit	31,g1,g1
	ret
	

	.globl	_floor
_floor:
	/* daint misbehaves for -0.0, so check for either 0.0 or -0.0 before
	   calling daint. */
	cmpo	0,g0
	setbit	31,0,g2		/* g2 <- 0x80000000 */
	andnot	g2,g1,g2
	bne	_daint
	cmpo	0,g2
	bne	_daint
	ret			/* just return -0.0 */


/*  Routine to convert a double-precision floating point number to whole
    floating-point number.
    **** This routine truncates DOWN ***
    On entry:  g0-g1 = floating-point number
    On exit:   g0-g1 = truncated floating-point number
*/

	.globl	_daint

_daint:
	shlo	1,g1,r4		#exponent -> r4
	shro	21,r4,r4

	lda	1043,r13	#bit number of "one" -> r12
	subo	r4,r13,r13	#   that minus 32 -> r13
	lda	32(r13),r12

	cmpi	r12,0		#if no decimals, return unchanged
	ble.f	Lfai99

	cmpi	g1,0		#branch for negative number
	bl.f	Lfai10

	cmpi	r13,20		#number (0 - 1) returns 0
	bg.f	Lfai5

	shro	r12,g0,g0	#clear decimals in low word
	shlo	r12,g0,g0

	cmpi	r13,0		#also in high if there are any
	ble.f	Lfai99
	shro	r13,g1,g1
	shlo	r13,g1,g1

Lfai99:	ret

Lfai5:	movl	0,g0		#result dropped to zero
	b	Lfai99

Lfai10:	cmpi	r13,20		#number (-1 - 0) returns -1
	bg.f	Lfai15

	cmpo	0,0		#take 2's complement
	subc	g0,0,g0
	subc	g1,0,g1

	shro	r12,g0,g0	#clear decimals in low word
	shlo	r12,g0,g0

	cmpi	r13,0		#also in high if there are any
	ble.f	Lfai11
	shro	r13,g1,g1
	shlo	r13,g1,g1

Lfai11:	cmpo	0,0		#take 2's complement back
	subc	g0,0,g0
	subc	g1,0,g1

	b	Lfai99

Lfai15:	lda	0xbff00000,g1	#return -1
	mov	0,g0
	b	Lfai99



/*  Routine to convert a double-precision floating point number to an integer.
    **** This routine truncates DOWN ****
    On entry:  g0-g1 = double floating-point A
    On exit:   g0    = int(A)
*/

	.globl	_dpint

_dpint:
	shlo	1,g1,r4		#exponent to r4
	lda	0xffe00000,r12
	cmpo	r4,r12
	bg.f	Lfin85
	shro	21,r4,r4

	shlo	10,g1,r5	#mantissa into r5
	setbit	30,r5,r5
	clrbit	31,r5,r5
	shro	22,g0,r12
	or	r5,r12,r5

	cmpi	g1,0		#handle + and - separately for speed
	lda	1053,r12
	bl.f	Lfin10

	cmpo	r4,r12		#just leave the integer
	subo	r4,r12,r4
	bg.f	Lfin3
	shro	r4,r5,g0

Lfin99:	ret

Lfin3:	subo	1,0,g0		#here exponent is too large, return INF
	clrbit	31,g0,g0
	b	Lfin99

Lfin10:	cmpo	r4,r12		#just leave the integer
	subo	r4,r12,r4
	bg.f	Lfin5

	subo	r5,0,r5		#negate result before shifting
	shri	r4,r5,g0

	b	Lfin99

Lfin5:	shlo	31,1,g0		#here exponent is too large, return -INF
	b	Lfin99

Lfin85:	movl	0,g0		#NaN converted to 0
	b	Lfin99
