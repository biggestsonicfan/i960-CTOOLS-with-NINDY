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

/*  Routine to convert an integer to double-precision floating point.
    On entry:  g0 = integer A
    On exit:   g0 = double float (A)
*/

	.file	"dpcnvt.s"
	.link_pix
	.globl	___floatsidf
	.align	4

___floatsidf:
	cmpi	g0,0		#handle + and - separately for speed
	lda	0x3ff+20,r4	#   initial exponent into r4
	bl.f	Lflo10
	mov	g0,r5

	scanbit	r5,r12		#normalize the mantissa
	bno.f	Lflo22
	subo	r12,20,r12
	cmpi	r12,0
	bl.f	Lflo5
	shlo	r12,r5,g1
	mov	0,g0
	b	Lflo6
Lflo5:	subo	r12,0,r13
	shro	r13,r5,g1
	lda	32(r12),r13
	shlo	r13,r5,g0

Lflo6:	clrbit	20,g1,g1	#clear hidden bit

	subo	r12,r4,r4	#adjust the exponent

	shlo	20,r4,r4	#build the result
	or	r4,g1,g1

Lflo99:	ret

Lflo22:	movl	0,g0		#return 0
	b	Lflo99

Lflo10:	subo	g0,0,r5		#negative number

	scanbit	r5,r12		#normalize the mantissa
	subo	r12,20,r12
	cmpi	r12,0
	bl.f	Lflo15
	shlo	r12,r5,g1
	mov	0,g0
	b	Lflo16
Lflo15:	subo	r12,0,r13
	shro	r13,r5,g1
	lda	32(r12),r13
	shlo	r13,r5,g0

Lflo16:	clrbit	20,g1,g1	#clear hidden bit

	subo	r12,r4,r4	#adjust the exponent

	shlo	20,r4,r4	#build the result
	or	r4,g1,g1
	setbit	31,g1,g1

	b	Lflo99

/*  Routine to convert a double-precision floating point number to an integer.
    **** This routine truncates down the ABSOLUTE value ****
    On entry:  g0-g1 = double floating-point A
    On exit:   g0    = int(A)
*/

	.globl	___fixdfsi

___fixdfsi:
	shlo	1,g1,r4		#exponent to r4
	lda	0xffe00000,r12
	cmpo	r4,r12
	bg.f	Lfix85
	shro	21,r4,r4

	shlo	10,g1,r5	#mantissa into r5
	setbit	30,r5,r5
	clrbit	31,r5,r5
	shro	22,g0,r12
	or	r5,r12,r5

	cmpi	g1,0		#handle + and - separately for speed
	lda	1053,r12
	bl.f	Lfix10

	cmpo	r4,r12		#just leave the integer
	subo	r4,r12,r4
	bg.f	Lfix3
	shro	r4,r5,g0

Lfix99:	ret

Lfix3:	subo	1,0,g0		#here exponent is too large, return INF
	clrbit	31,g0,g0
	b	Lfix99

Lfix10:	cmpo	r4,r12		#just leave the integer
	subo	r4,r12,r4
	bg.f	Lfix5
	shro	r4,r5,r5

	subo	r5,0,g0		#negate result

	b	Lfix99

Lfix5:	shlo	31,1,g0		#here exponent is too large, return -INF
	b	Lfix99

Lfix85:	mov	0,g0		#NaN converted to 0
	b	Lfix99



/*  Routine to convert a double-precision floating point number to an
    unsigned integer.  Negative number becomes 0.
    **** This routine truncates DOWN ****
    On entry:  g0-g1 = double floating-point A
    On exit:   g0    = unsigned int(A)
*/

	.globl	___fixunsdfsi

___fixunsdfsi:
	shlo	1,g1,r4		#exponent to r4
	lda	0xffe00000,r12
	cmpo	r4,r12
	bg.f	Lfiu85
	shro	21,r4,r4

	shlo	11,g1,r5	#mantissa into r5
	setbit	31,r5,r5
	shro	21,g0,r12
	or	r5,r12,r5

	cmpi	g1,0		#negative becomes 0
	bl.f	Lfiu85

	lda	1054,r12	#just leave the integer
	cmpo	r4,r12
	bg.f	Lfiu3
	subo	r4,r12,r4
	shro	r4,r5,g0

Lfiu99:	ret

Lfiu3:	subo	1,0,g0		#here exponent is too large, return INF
	b	Lfiu99

Lfiu85:	mov	0,g0		#here return 0
	b	Lfiu99


/*  Routine to convert a double-precision floating point number to a single-
    precision floating-point number.
    On entry:  g0-g1 = double floating-point A
    On exit:   g0    = floating-point answer
*/

	.globl	___truncdfsf2
	.globl	___truncdfsf2_g960
/*  The entry point ___truncdfsf2_g960 must be guaranteed to only use the
    global registers g0,g1, plus the local registers.
*/

___truncdfsf2_g960:
___truncdfsf2:
	shlo	1,g1,r4		#exponent to r4
	lda	0xffdfffff,r12
	cmpo	r12,r4
	concmpo	r4,0
	ble.f	Ltsp80
	shro	21,r4,r4

	lda	0x3ff-0x7f,r12	#calculate new exponent
	subo	r12,r4,r4
	lda	0xff,r12
	cmpo	r4,r12
	bge.f	Ltsp20

	shlo	23,r4,r4	#build the result
	shlo	12,g1,r5
	shro	9,r5,r5
	or	r4,r5,r4
	shro	29,g0,r5
	chkbit	28,g0
	addc	r5,r4,g0

Ltsp9:	chkbit	31,g1		#sign bit in place
	alterbit 31,g0,g0

Ltsp99:	ret

Ltsp20:	cmpi	r4,0		#underflow if negative exponent
	bl	Ltsp22		#   overflow else
	b	Ltsp26

Ltsp80:	be.f	Ltsp22		#if eq set number was 0	
	lda	0xffe00000,r12	#   then separate NaN from INF
	cmpo	r4,r12
	bg.f	Ltsp21

Ltsp26:	lda	0x7f800000,g0	#this is INF
	b	Ltsp9
	
Ltsp21:	subo	1,0,g0		#this is NaN
	b	Ltsp99

Ltsp22:	mov	0,g0		#return signed 0
	b	Ltsp9



/*  Routine to convert a single-precision floating point number to a double-
    precision floating-point number.
    On entry:  g0    = single-precision floating-point A
    On exit:   g0-g1 = double-precision floating-point answer
*/

	.globl	___extendsfdf2

___extendsfdf2:
	shro	31,g0,r10	#sign bit -> r10
	shlo	31,r10,r10

	shlo	1,g0,r4		#exponent to r4
	lda	0xfeffffff,r12
	cmpo	r12,r4
	concmpo	r4,0
	ble.f	Ltdp80
	shro	24,r4,r4

	lda	0x7f-0x3ff,r12	#calculate new exponent
	subo	r12,r4,r4

	shlo	20,r4,r4	#build the result
	shlo	9,g0,r5
	shro	12,r5,r5
	or	r4,r5,g1
	shlo	29,g0,g0

Ltdp9:	or	g1,r10,g1	#sign bit in place

Ltdp99:	ret

Ltdp22:	movl	0,g0		#return double zero
	b	Ltdp9

Ltdp80:	be.f	Ltdp22		#if eq set it was zero
	lda	0xff000000,r12	#   then separate NaN from INF
	cmpo	r12,r4
	bl.f	Ltdp21

	lda	0x7ff00000,g1	#here return INF
	mov	0,g0
	b	Ltdp9
	
Ltdp21:	subo	1,0,g0		#this is NaN
	subo	1,0,g1
	b	Ltdp99



/*  Routine to convert an extented floating point number to a double-
    precision floating-point number.
    On entry:  g0-g3 = extended floating-point A
    On exit:   g0-g1 = double-precision floating-point answer
*/

	.globl	_eptodp

_eptodp:
	shro	15,g2,r10	#the sign -> r10
	shlo	31,r10,r10

	shlo	17,g2,r4	#exponent -> r4
	shro	17,r4,r4

	chkbit	31,g1		#check for unnormalized or zero
	bno.f	Ledp22

	lda	0x3fff-0x3ff,r12 
	subo	r12,r4,r4	#calculate new exponent

	lda	0x7ff,r12	#check for INF or invalid
	cmpo	r4,r12
	bge.f	Ledp20

	clrbit	31,g1,g1	#build the result
	shro	11,g1,r7
	shlo	21,g1,r6
	shlo	20,r4,r4
	or	r7,r4,g1
	chkbit	10,g0
	shro	11,g0,g0
	addc	r6,g0,g0

Ledp9:	or	r10,g1,g1	#put in the sign

Ledp99:	ret

Ledp20:	bg.f	Ledp21		#separate INF from NaN

	lda	0x7ff00000,g1	#this is INF
	mov	0,g0
	b	Ledp9

Ledp22:	or	g0,g1,g1	#this is 0 or unnormalized
	cmpo	g1,0
	be	Ledp9

Ledp21:	subo	1,0,g0		#this is NaN
	subo	1,0,g1
	b	Ledp99


/*  Negate double-precision number. 
    Take absolute value of double-precision number.
*/

	.globl	___negdf2
	.globl	___absdf2

___negdf2:
	notbit	31,g1,r10	#toggle sign bit
	b	Labs5

___absdf2:
	clrbit	31,g1,r10	#clear sign bit

Labs5:	shlo	1,g1,r4		#return NaN as such
	lda	0xffe00000,r12
	cmpo	r4,r12
	bg.f	Labs99

	mov	r10,g1		#build the result

Labs99:	ret
