/*
; ; ; ; ;
;	80960 FPAC/DPAC		Floating Point Library
;	FPOPNS			Basic Operations (SP)
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

/*  Routine for single-precision floating point addition.
    On entry:  g0 = argument A
               g1 = argument B
    On exit:   g0 = A + B
*/

	.file	"fpopns.s"
	.link_pix

	.globl	___addsf3
	.globl	___subsf3
	.text
	.align	4

___subsf3:
	notbit	31,g1,g1	#subtract negates argument B

___addsf3:
	shlo	1,g0,r4		#exponent and mantissa A -> r4
	shlo	1,g1,r6		#		       B -> r6

	shlo	24,1,r12	#check for INF NaN 0
	addo	r4,r12,r13
	cmpo	r13,r12
	addo	r6,r12,r13
	ble.f	Lfad80
	cmpo	r13,r12

	shlo	8,g0,r5		#isolate mantissa to bits 8-31
	ble.f	Lfad80
	shlo	8,g1,r7		#   MA -> r5
	setbit	31,r5,r5	#   MB -> r7
	setbit	31,r7,r7

	shro	31,g0,r10	#sign of A -> r10
	shlo	31,r10,r10

	shro	24,r4,r4	#exponent bits 0-7: AE -> r4	
	shro	24,r6,r6	#		    AB -> r6

	cmpo	r6,r4		#equalize the exponents by shifting
	subo	r6,r4,r12
	bg.t	Lfad2
	shro	r12,r7,r7
	b	Lfad3
Lfad2:	subo	r12,0,r12
	mov	r6,r4
	shro	r12,r5,r5

Lfad3:	xor	g0,g1,r12	#two cases: A and B same sign
	chkbit	31,r12		#                   different sign
	be.f	Lfad10

	addc	r7,r5,r5	#mantissa add, set the carry if no room

	bno.f	Lfad20		#renormalize if carry happens
	shro	1,r5,r5
	addo	1,r4,r4

	lda	0xff,r12	#that might bring us to infinity
	cmpo	r4,r12
	bl.t	Lfad20

	mov	r12,r4		#overflow, return INF
	mov	0,r5
	b	Lfad20

Lfad10:	subc	r7,r5,r5	#different signs, subtract B from A
	be.t	Lfad11

	subo	r5,0,r5		#here B > A, reverse sign
	notbit	31,r10,r10

Lfad11:	scanbit	r5,r12		#normalize the mantissa
	bno.f	Lfad31		#   return 0 if no bits
	subo	r12,31,r12	#   hidden bit at 31 again
	shlo	r12,r5,r5

	subo	r12,r4,r4	#adjust exponent for shift

	cmpi	r4,0		#if underflow, return 0
	bl.f	Lfad32

Lfad20:	chkbit	7,r5		#this is the rounding bit

	clrbit	31,r5,r5	#combine the elements to floating point
	shro	8,r5,r5
	shlo	23,r4,g0
	or	r5,g0,g0
	addc	r10,g0,g0	

Lfad99:	ret			#return to calling C program

Lfad31:	mov	0,g0		#return +0 if all bits cancelled
	b	Lfad99

Lfad32:	mov	r10,g0		#return signed 0 for underflow
	b	Lfad99

# This is coding for invalid or special arguments

Lfad80:	lda	0xff000000,r12	#check for INF or NaN
	cmpo	r12,r4		#   if either NaN return NaN
	be.f	Lfad81
	bl.f	Lfad82
	cmpo	r12,r6
	be.f	Lfad83
	bl.f	Lfad82

	cmpo	r4,0		#neither, so A or B or both = 0
	bne.f	Lfad99		#   B must be zero, return A

	cmpo	r6,0		#if A = B = 0 return +0 unless
	be.f	Lfad88		#   both are -0

Lfad83:	mov	g1,g0		# return B if  A = 0 or
	b	Lfad99		#              B = INF and A regular

Lfad81:  cmpo	r12,r6		# A was INF, what is B?
	bl.f	Lfad82		#   if NaN return NaN
	bg.f	Lfad99		#   regular, return A
	xor	g0,g1,r12	#   A and B infinite, different sign?
	cmpi	r12,0		#   if yes, NaN, else return argument A
	bge.f	Lfad99

Lfad82:	subo	1,0,g0		# return Not a Number
	b	Lfad99

Lfad88:	and	g0,g1,g0	#return +0 or -0
	b	Lfad99


/*  Routine for single-precision floating point divide.
    On entry:  g0 = argument A
               g1 = argument B
    On exit:   g0 = A / B
*/

	.globl	___divsf3
	.align	4

___divsf3:
	xor	g0,g1,r10	#sign bit to r10
	shro	31,r10,r10
	shlo	31,r10,r10

	shlo	1,g0,r4		#exponent and mantissa A -> r4
	shlo	1,g1,r6		#		       B -> r6

	shlo	24,1,r12	#check for INF NaN 0
	addo	r4,r12,r13
	cmpo	r13,r12
	addo	r6,r12,r13
	ble.f	Lfdv80
	cmpo	r13,r12

	shro	24,r4,r4	#exponent bits 0-7: AE -> r4	
	ble.f	Lfdv80
	shro	24,r6,r6	#		    AB -> r6

	shlo	8,g0,r9		#mantissa: AM -> r8-r9
	setbit	31,r9,r9	#          BM -> r7
	shlo	8,g1,r7
	setbit 	31,r7,r7

	cmpo	r7,r9		#adjust so that mantissa of A < of B
	testle	r3
	shro	r3,r9,r9

	lda	126(r4)[r3],r4	#exponent = EA - EB - 1 + 127
	subo	r6,r4,r4
	lda	0xfe,r12
	cmpo	r12,r4
	bl.f	Lfdv7

	addo	1,r7,r8		#add half of divider to dividend to round
	shro	1,r8,r8

	ediv	r7,r8,r12	#do the divide

Lfdv9:	chkbit	7,r13		#set carry for rounding bit

	shro	8,r13,r13	#move mantissa to proper place
	clrbit	23,r13,r13	#   clear default bit

	shlo	23,r4,r4	#build the composite
	or	r13,r4,g0
	addc	r10,g0,g0

Lfdv99:	ret

Lfdv7:	cmpi	r4,0		#special cases in the result:
	bl	Lfdv8		#
	addo	1,r12,r4	#
	mov	0,r13		#   here exponent > FE
	b	Lfdv9		#
Lfdv8:	mov	0,r4		#   here exponent < 0
	mov	0,r13
	b	Lfdv9

# handle here bad arguments or special cases

Lfdv80:	lda	0xff000000,r12	#check for INF or NaN
	cmpo	r12,r4		#   if either NaN return NaN
	be.f	Lfdv81
	bl.f	Lfdv82
	cmpo	r12,r6
	be.f	Lfdv83
	bl.f	Lfdv82

	cmpo	r4,0		#neither, so A or B or both = 0
	bne.f	Lfdv86		#   A /= 0 so B = 0, return INF
	cmpo	r6,0		#   B = 0?   
	be.f	Lfdv82		#   A and B = 0, return NaN

Lfdv83:	mov	r10,g0		# return 0 if A = 0 and B regular or
	b	Lfdv99		#	      B = INF and A regular

Lfdv81:  cmpo	r12,r6		# A was INF, what is B?
	bg.f	Lfdv86		#   regular, return INF, else NaN

Lfdv82:	subo	1,0,g0		# return Not a Number
	b	Lfdv99

Lfdv86:	lda	0x7f800000,g0	#return INF
	or	r10,g0,g0
	b	Lfdv99



/*  Routine for single-precision floating point multiply.
    On entry:  g0 = argument A
               g1 = argument B
    On exit:   g0 = A * B
*/

	.globl	___mulsf3
	.align	4

___mulsf3:
	xor	g0,g1,r10	#get the sign bit to r10
	shro	31,r10,r10
	shlo	31,r10,r10

	shlo	1,g0,r4		#exponent and mantissa A -> r4
	shlo	1,g1,r6		#		       B -> r6

	shlo	24,1,r12	#check for INF NaN 0
	addo	r4,r12,r13
	cmpo	r13,r12
	addo	r6,r12,r13
	ble.f	Lfmu80
	cmpo	r13,r12

	shro	24,r4,r4	#exponent bits 0-7: AE -> r4	
	ble.f	Lfmu80
	shro	24,r6,r6	#		    AB -> r6

	shlo	8,g0,r5		#mantissa: AM -> r8-r9
	setbit	31,r5,r5	#          BM -> r7
	shlo	8,g1,r7
	setbit 	31,r7,r7

	emul	r5,r7,r12	#do the multiply

	shro	31,r13,r12	#normalize, we may be off by 1 bit
	shro	r12,r13,r13
	addo	r12,r4,r4

	addo	r6,r4,r4	#exponent = EA + EB - 127
	lda	127,r12
	subo	r12,r4,r4
	lda	0xfe,r12
	cmpo	r12,r4
	bl.f	Lfmu7

Lfmu9:	chkbit	6,r13		#set carry for rounding bit

	shro	7,r13,r13	#move mantissa to proper place
	clrbit	23,r13,r13	#   clear default bit

	shlo	23,r4,r4	#build the composite
	or	r13,r4,g0
	addc	r10,g0,g0

Lfmu99:	ret

Lfmu7:	cmpi	r4,0		#special cases in the result:
	bl	Lfmu8		#
	addo	1,r12,r4	#
	mov	0,r13		#   here exponent > FE
	b	Lfmu9		#
Lfmu8:	mov	0,r4		#   here exponent < 0
	mov	0,r13
	b	Lfmu9

# handle here bad arguments or special cases

Lfmu80:	lda	0xff000000,r12	#check for INF or NaN
	cmpo	r12,r4		#   if either NaN return NaN
	be.f	Lfmu81		#   if neither, A or B = 0 so return 0
	bl.f	Lfmu82
	cmpo	r12,r6
	be.f	Lfmu84
	bl.f	Lfmu82

	mov	r10,g0		# return 0 if A or B = 0 
	b	Lfmu99

Lfmu81:  cmpo	r12,r6		# A was INF, what is B?
	bl.f	Lfmu82		#   NaN, return NaN
	cmpo	r6,0		#   0, return NaN
	bne.f	Lfmu83		#   else INF

Lfmu82:	subo	1,0,g0		# return Not a Number
	b	Lfmu99

Lfmu84:	cmpo	r4,0		# B = INF, return NaN if A = 0
	be.f	Lfmu82		#          else return INF

Lfmu83:	lda	0x7f800000,g0	#return INF
	or	r10,g0,g0
	b	Lfmu99


/*  Routine to compare two single-precision floating point numbers.
    If the difference in the numbers is at most "ffuzz" units, the
    numbers are called equal.  The units are mantissa units for the
	larger of A and B.

    On entry:  g0 = argument A
               g1 = argument B
    On exit:   g0 = -1 A < B
    		     0 A = B
		     1 A > B
		     3 no comparison
*/

	.set	Lffuzz,20	#maximum difference for =

	.globl	___cmpsf2
	.align	4

___cmpsf2:
	shlo	1,g0,r4		#exponent and mantissa A -> r4
	shlo	1,g1,r6		#		       B -> r6

	lda	0xfeffffff,r12	#check special cases
	cmpo	r12,r4		#   A == INF or NaN?  
	concmpo	r4,0		#   A == 0?
	ble.f	Lfcp80		#
Lfcp1:	cmpo	r12,r6		#   B == INF or NaN?
	concmpo	r6,0		#   B == 0?
	ble.f	Lfcp90

Lfcp2:	shri	31,g0,r12	#two's complement A
	clrbit	31,g0,r5
	xor	r5,r12,r5
	subo	r12,r5,r5

	shri	31,g1,r12	#two's complement B
	clrbit	31,g1,r7
	xor	r7,r12,r7
	subo	r12,r7,r7

#	subo	r7,r5,r12	#calculate abs(A-B) -> r12
#	shri	31,r12,r13
#	xor	r12,r13,r12
#	subo	r13,r12,r12

#	cmpo	r12,ffuzz	#if difference not larger than ffuzz,
#	ble.t	Lfcp7		#   eliminate it

	cmpi	r5,r7		#compare sign, exponent, mantissa and all
	subo	1,0,g0
	bl.f	Lfcp99
	mov	1,g0
	bne.f	Lfcp99

Lfcp7:	mov	0,g0

Lfcp99:	ret			#return to C program

# This is coding for invalid or special arguments

Lfcp80:	be.f	Lfcp81		#jump if A = 0
	lda	0xff000000,r12	#    else NaN or INF
	cmpo	r12,r4
	be.f	Lfcp82

Lfcp8n:	mov	3,g0		#return "no compare"
	b	Lfcp99

Lfcp8g:	mov	1,g0		#return "greater"
	b	Lfcp99

Lfcp8l:	subo	1,0,g0		#return "less"
	b	Lfcp99

Lfcp81:	mov	0,g0		#force A to +0 and resume 
	b	Lfcp1

Lfcp83:	cmpi	g0,0		#here A was INF and B regular
	bl	Lfcp8l
	b	Lfcp8g

Lfcp82:  cmpo	r12,r6		# A was INF, what is B?
	bl.f	Lfcp8n		#   if NaN return "no compare"
	bg.f	Lfcp83		#   B regular:  > for INF, < for -INF
	xor	g0,g1,r12	#   A and B infinite, same sign?
	cmpi	r12,0		#   if yes, EQUAL, else 
	bge.f	Lfcp7		#   according to sign of A
	chkbit	31,g0
	bno.f	Lfcp8g
	b	Lfcp8l

Lfcp90:	be.f	Lfcp91		# if EQ set B = 0
	lda	0xff000000,r12	#    else NaN or INF
	cmpo	r12,r6
	bl.f	Lfcp8n
	chkbit	31,g1
	be.f	Lfcp8g
	b	Lfcp8l

Lfcp91:	mov	0,g1		#force B to +0 and resume 
	b	Lfcp2
