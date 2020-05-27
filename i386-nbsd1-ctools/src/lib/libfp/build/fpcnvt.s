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

/*  Routine to convert an integer to single-precision floating point.
    On entry:  g0 = integer A
    On exit:   g0 = float(A)
*/

	.file	"fpcnvt.s"
	.link_pix

	.globl  ___floatsisf
	.align	4

___floatsisf:
	cmpi	g0,0		#handle + and - separately for speed
	lda	158,r4		#   initial exponent into r4
	bl.f	Lflo10
	mov	g0,r5

	scanbit	r5,r12		#normalize the mantissa
	subo	r12,31,r12
	bno.f	Lflo99
	shlo	r12,r5,r5
	chkbit	7,r5
	shro	8,r5,r5
	clrbit	23,r5,r5
	subo	r12,r4,r4

	shlo	23,r4,g0	#build the result
	addc	r5,g0,g0

Lflo99:	ret

Lflo10:	subo	g0,0,r5		#negative number

	scanbit	r5,r12		#normalize the mantissa
	subo	r12,31,r12
	shlo	r12,r5,r5
	chkbit	7,r5
	shro	8,r5,r5
	clrbit	23,r5,r5
	subo	r12,r4,r4

	shlo	23,r4,g0	#build the result
	addc	r5,g0,g0
	setbit	31,g0,g0

	b	Lflo99

/*  Routine to convert a single-precision floating point number to an integer.
    **** This routine truncates down ABSOLUTE value ***
    On entry:  g0 = floating-point number
    On exit:   g0 = int(A)
*/

	.globl	___fixsfsi
___fixsfsi:
	shlo	1,g0,r4		#exponent -> r4
	lda	0xff000000,r12
	cmpo	r4,r12
	bg.f	Lfix26
	shro	24,r4,r4

	shlo	7,g0,r5		#mantissa into r5
	setbit	30,r5,r5
	clrbit	31,r5,r5

	cmpi	g0,0		#handle + and - separately for speed
	lda	157,r12
	bl.f	Lfix10

	cmpo	r4,r12		#just leave the integer
	subo	r4,r12,r4
	bg.f	Lfix3
	shri	r4,r5,g0

Lfix99:	ret

Lfix3:	subo	1,0,g0		#here exponent is too large, return INF
	clrbit	31,g0,g0
	b	Lfix99

Lfix10:	cmpo	r4,r12		# - case, shift in place
	subo	r4,r12,r4
	bg.f	Lfix15
	shri	r4,r5,r5

	subo	r5,0,g0		#complement after shift

	b	Lfix99

Lfix15:	shlo	31,1,g0		#exponent too large, return -INF
	b	Lfix99

Lfix26:	mov	0,g0		#NaN becomes 0
	b	Lfix99


/*  Routine to convert a single-precision floating point number to an
    unsigned integer.  Negative number becomes 0.
    **** This routine truncates DOWN ****
    On entry:  g0    = sinle floating-point A
    On exit:   g0    = unsigned int(A)
*/

	.globl	___fixunssfsi

___fixunssfsi:
	shlo	1,g0,r4		#exponent to r4
	lda	0xff000000,r12
	cmpo	r4,r12
	bg.f	Lfiu85
	shro	24,r4,r4

	shlo	7,g0,r5		#mantissa into r5
	setbit	30,r5,r5
	clrbit	31,r5,r5

	cmpi	g0,0		#negative becomes 0
	lda	157,r12
	bl.f	Lfiu85

	cmpo	r4,r12
	subo	r4,r12,r4
	bg.f	Lfiu3
	shro	r4,r5,g0

Lfiu99:	ret

Lfiu3:	subo	1,0,g0		#here exponent is too large, return INF
	ret

Lfiu85:	mov	0,g0		#here return 0
	ret

/*  Routine to negate a number.
    Routine to take absolute value.
*/

	.globl	___abssf2
	.globl	___negsf2

___negsf2:
	notbit	31,g0,r10	#toggle sign bit
	b	Labs5

___abssf2:
	clrbit	31,g0,r10	#clear sign bit

Labs5:	shlo	1,g0,r4		#leave NaN unchanged
	lda	0xff000000,r12
	cmpo	r4,r12
	bg.f	Labs99

	mov	r10,g0		#this is the result

Labs99:	ret
