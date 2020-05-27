/*
; ; ; ; ;
;	80960 FPAC/DPAC		Floating Point Library
;	DPOPNS			Basic Operations (DP)
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

/*  Routine for double-precision floating point addition.
    On entry:  g0-g1 = argument A
               g2-g3 = argument B
    On exit:   g0-g1 = A + B
*/

/*  Subtract uses the same routine, just negates the second argument.
*/

	.file	"dpopns.s"
	.link_pix

	.globl	___adddf3
	.globl	___subdf3
	.text
	.align	4


___subdf3:
	notbit	31,g3,g3		#subtract negates argument B

___adddf3:
	shlo	1,g1,r4		#exponent and mantissa A -> r4
	shlo	1,g3,r5		#		       B -> r5

	shlo	21,1,r12	#check for INF NaN 0
	addo	r4,r12,r13
	cmpo	r13,r12
	ble.f	Lfad80
	addo	r5,r12,r13
	cmpo	r13,r12
	ble.f	Lfad80

Lfad1:	shlo	11,g1,r7	#mantissa of A left-justified r6-r7
	shro	21,g0,r12
	shlo	11,g0,r6
	or	r12,r7,r7
	setbit	31,r7,r7

	shlo	11,g3,r9	#mantissa of B left-justified r8-r9
	shro	21,g2,r12
	shlo	11,g2,r8
	or	r12,r9,r9
	setbit	31,r9,r9

	shro	21,r4,r4	#exponent bits 0-10: AE -> r4	
	shro	21,r5,r5	#		     AB -> r5

	shro	31,g1,r10	#sign bit of A to r10
	shlo	31,r10,r10

	cmpo	r4,r5		#equalize the exponents by shifting
	lda	32,r13
	bl.t	Lfad2
	subo	r5,r4,r12
	cmpo	r12,r13
	bl.t	Lfad1a
	mov	r9,r8
	mov	0,r9
	subo	r13,r12,r12
Lfad1a:	shro	r12,r8,r8
	subo	r12,r13,r13
	shlo	r13,r9,r13
	shro	r12,r9,r9
	or	r13,r8,r8
	b	Lfad3
Lfad2:	subo	r4,r5,r12
	cmpo	r12,r13
	bl.t	Lfad2a
	mov	r7,r6
	mov	0,r7
	subo	r13,r12,r12
Lfad2a:	shro	r12,r6,r6
	subo	r12,r13,r13
	shlo	r13,r7,r13
	shro	r12,r7,r7
	or	r13,r6,r6
	mov	r5,r4

Lfad3:	xor	g1,g3,r12	#branch if A and B have different signs
	chkbit	31,r12
	be.f	Lfad10
	
	addc	r8,r6,r6	#add mantissas of A and B
	addc	r9,r7,r7

	bno.f	Lfad20		#at overflow we shift mantissa right
	shro	1,r6,r6		#   and bump the exponent
	shlo	31,r7,r12
	shro	1,r7,r7
	or	r12,r6,r6
	addo	1,r4,r4

	lda	0x7ff,r12	#this may cause overflow of mantissa
	cmpo	r4,r12
	bl.t	Lfad20

	mov	r12,r4		#overflow, return INF
	movl	0,r6
	b	Lfad20

Lfad10:	subc	r8,r6,r6	#subtract B from A
	subc	r9,r7,r7
	be.f	Lfad11

	cmpo	0,0		#here B > A, negate and reverse sign
	subc	r6,0,r6
	subc	r7,0,r7
	notbit	31,r10,r10

Lfad11:	scanbit	r7,r12		#normalize the mantissa, left-justified
	be.t	Lfad12
	scanbit	r6,r12
	bno.f	Lfad31
	subo	r12,31,r12
	shlo	r12,r6,r7
	mov	0,r6
	lda	32(r12),r12
	b	Lfad17
Lfad12:	shro	r12,r6,r13
	shro	1,r13,r13
	subo	r12,31,r12
	shlo	r12,r7,r7
	shlo	r12,r6,r6
	or	r13,r7,r7

Lfad17:	subo	r12,r4,r4	#adjust exponent
	cmpi	r4,0		#    it may have underflowed
	bl.f	Lfad32

Lfad20:	chkbit	10,r6		#set flag for rounding bit

	clrbit	31,r7,r7	#clear hidden bit in mantissa

	shro	11,r6,g0	#right-justify mantissa to g0-g1
	shlo	21,r7,r13
	shro	11,r7,g1

	shlo	20,r4,r4	#combine mantissa, exponent, sign,
	or	g1,r4,g1	#   rounding bit
	addc	r13,g0,g0
	addc	r10,g1,g1

Lfad99:	ret			#return to calling C program

Lfad31:	movl	0,g0		#no bits left, return +0
	b	Lfad99

Lfad32:	mov	0,g0		#underflow: return signed 0
	mov	r10,g1
	b	Lfad99

# This is coding for invalid or special arguments

Lfad80:	lda	0xffe00000,r12	#check for INF or NaN
	cmpo	r12,r4		#   if either NaN return NaN
	be.f	Lfad81
	bl.f	Lfad82
	cmpo	r12,r5
	be.f	Lfad83
	bl.f	Lfad82

	or	r4,g0,r13	#neither, see if A or B or both = 0
	cmpo	r13,0		#   if neither this was false alarm
	be.f	Lfad86		#   we go back to mainstream
	or	r5,g2,r13	#   if A = 0 return B
	cmpo	r13,0		#   if B = 0 return A
	bne.f	Lfad1
	b	Lfad99

Lfad86:	or	r5,g2,r13	#if A and B 0 return +0 unless both -0
	cmpo	r13,0
	be.f	Lfad88

Lfad83:	movl	g2,g0		# return B if  A = 0 or
	b	Lfad99		#              B = INF and A regular

Lfad81:  cmpo	r12,r5		# A was INF, what is B?
	bl.f	Lfad82		#   if NaN return NaN
	bg.f	Lfad99		#   regular, return A
	xor	g1,g3,r12	#   A and B infinite, different sign?
	cmpi	r12,0		#   if yes, NaN, else return argument A
	bge.f	Lfad99

Lfad82:	subo	1,0,g0		# return Not a Number
	subo	1,0,g1
	b	Lfad99

Lfad88:	and	g1,g3,g1	#return +0 unless both -0
	mov	0,g0
	b	Lfad99


/*  Routine for double-precision floating point divide.
    On entry:  g0 = argument A
               g1 = argument B
    On exit:   g0 = A / B
*/

	.globl	___divdf3
	.align	4

___divdf3:
	xor	g1,g3,r3	#get the sign bit to r3
	shro	31,r3,r3
	shlo	31,r3,r3

	shlo	1,g1,r4		#exponent and mantissa A -> r4
	shlo	1,g3,r5		#		       B -> r5

	shlo	21,1,r12	#check for INF NaN 0
	addo	r4,r12,r13
	cmpo	r13,r12
	ble.f	Lfdv80
	addo	r5,r12,r13
	cmpo	r13,r12
	ble.f	Lfdv80

Lfdv1:	shlo	11,g1,r7	#isolate mantissa left-justified
	shro	1,r7,r7		#
	shlo	10,g0,r6	#   AM -> r6-r7 to bit 30
	shro	22,g0,r13	#
	or	r13,r7,r7	#
	setbit	30,r7,r7	#
	shlo	11,g3,r9	#   BM -> r8-r9 to bit 31
	shlo	11,g2,r8	#
	shro	21,g2,r13	#
	or	r13,r9,r9	#
	setbit	31,r9,r9

	shro	21,r4,r4	#exponent bits 0-10: AE -> r4	
	shro	21,r5,r5	#		     AB -> r5

/*  We can't divide 64 bits by 64 bits but we use the approximation
    A/(B+C) = (A/B)(1 - (C/B))  where A = r6-r7, B = r9 and C = r8 
*/

	ediv	r9,r6,r10	# r10/11 = A / Bh
	mov	r10,r13		#   to get 64 bits we divide the remainder
	mov	0,r12
	ediv	r9,r12,r12
	mov	r13,r10

	shro	1,r8,r13	# r13 = Bl / Bh
	mov	0,r12		#   shift dividend right to prevent overflow
	ediv	r9,r12,r12

	emul	r11,r13,r12	# r10/11 = A/B - A/B * r13
	cmpo	0,0
	subc	r13,r10,r10
	subc	0,r11,r11
	subc	r13,r10,r10
	subc	0,r11,r11

	chkbit	31,r11		#left-justify
	be.t	Lfdv13		#   at most 1 bit out
	addc	r10,r10,r10
	addc	r11,r11,r11
	subo	1,r4,r4

Lfdv13:	lda	0x3ff(r4),r4	#exponent = EA - EB + 0x3ff
	subo	r5,r4,r4
	lda	0x7fe,r12
	cmpo	r12,r4
	bl.f	Lfdv7

Lfdv9:	chkbit	10,r10		#set carry on rounding bit

	shro	11,r10,g0	#shift into proper place in g0-g1
	shlo	21,r11,r13
	shro	11,r11,g1
	clrbit	20,g1,g1

	shlo	20,r4,r4	#build the composite
	or	g1,r4,g1
	addc	r13,g0,g0
	addc	r3,g1,g1

Lfdv99:	ret

Lfdv7:	cmpi	r4,0		#special cases in the result:
	bl	Lfdv8		#
	addo	1,r12,r4	#
	movl	0,r10		#   here exponent > 3FE
	b	Lfdv9		#
Lfdv8:	mov	0,r4		#   here exponent < 0
	movl	0,r10
	b	Lfdv9

# handle here bad arguments or special cases

Lfdv80:	lda	0xffe00000,r12	#check for INF or NaN
	cmpo	r12,r4		#   if either NaN return NaN
	be.f	Lfdv81
	bl.f	Lfdv82
	cmpo	r12,r5
	be.f	Lfdv83
	bl.f	Lfdv82

	or	r4,g0,r13	#neither, see if A or B or both = 0
	cmpo	r13,0		#   if neither this was false alarm
	be.f	Lfdv84		#   we go back to mainstream
	or	r5,g2,r13
	cmpo	r13,0
	bne.f	Lfdv1

Lfdv86:	lda	0x7ff00000,g1	# INF:  B = 0 and A regular, or
	mov	0,g0		#       A = INF and B regular
	or	r3,g1,g1
	b	Lfdv99

Lfdv84:	or	r5,g2,r13	# A was zero, if B also return NaN
	cmpo	r13,0		#   B = 0?   
	be.f	Lfdv82		#   A and B = 0, return NaN

Lfdv83:	movl	0,g0		# return 0 if A = 0 and B regular or
	or	r3,g1,g1	#	      B = INF and A regular
	b	Lfdv99

Lfdv81:  cmpo	r12,r5		# A was INF, what is B?
	bg.f	Lfdv86		#   regular, return INF, else NaN

Lfdv82:	subo	1,0,g0		# return Not a Number
	subo	1,0,g1
	b	Lfdv99




/*  Routine for double-precision floating point multiply.
    On entry:  g0-g1 = argument A
               g2-g3 = argument B
    On exit:   g0-g1 = A * B
*/

	.globl	___muldf3
	.align	4

___muldf3:
	xor	g1,g3,r3	#get the sign bit to r10
	shro	31,r3,r3
	shlo	31,r3,r3

	shlo	1,g1,r4		#exponent and mantissa A -> r4
	shlo	1,g3,r5		#		       B -> r5

	shlo	21,1,r12	#check for INF NaN 0
	addo	r4,r12,r13
	cmpo	r13,r12
	ble.f	Lfmu80
	addo	r5,r12,r13
	cmpo	r13,r12
	ble.f	Lfmu80

Lfmu1:	shlo	11,g1,r7	#isolate mantissa left-justified
	shlo	11,g0,r6	#   AM -> r6-r7
	shro	21,g0,r13	#
	or	r13,r7,r7	#
	shlo	11,g3,r9	#   BM -> r8-r9
	shlo	11,g2,r8	#
	shro	21,g2,r13	#
	or	r13,r9,r9	#
	setbit	31,r7,r7	#   and set the hidden bit
	setbit	31,r9,r9

	shro	21,r4,r4	#exponent bits 0-10: AE -> r4	
	shro	21,r5,r5	#		     AB -> r5

	emul	r7,r9,r12	#we can't multiply 64 bits x 64 bits
	emul	r6,r9,r10	#   but we can do it easily in 32 bit
	cmpo	0,1		#   chunks, and add these up 
	addc	r12,r11,r12
	addc	r13,0,r13
	emul	r7,r8,r14
	addc	r10,r14,r14
	addc	r12,r15,r12
	addc	r13,0,r13

	chkbit	31,r13		#normalize, we may be off by 1 bit
	be.t	Lfmu3
	addc	r12,r12,r12
	addc	r13,r13,r13
	subo	1,r4,r4

Lfmu3:	lda	0xfffffc02(r5)[r4],r4	#exponent = EA + EB - 0x3fe
	lda	0x7fe,r10
	cmpo	r10,r4
	bl.f	Lfmu7

Lfmu9:	chkbit	10,r12		#set carry on rounding bit

	shro	11,r12,g0	#shift mantissa into place in g0-g1
	shlo	21,r13,r10
	shro	11,r13,g1
	clrbit	20,g1,g1

	shlo	20,r4,r4	#build the composite
	or	g1,r4,g1
	addc	r10,g0,g0
	addc	r3,g1,g1

Lfmu99:	ret

Lfmu7:	cmpi	r4,0		#special cases in the result:
	bl	Lfmu8		#
	addo	1,r10,r4	#
	movl	0,r12		#   here exponent > 7FE
	b	Lfmu9		#
Lfmu8:	mov	0,r4		#   here exponent < 0
	movl	0,r12
	b	Lfmu9

# handle here bad arguments or special cases

Lfmu80:	shro	31,r3,r3	#to simplify code, isolate the sign bit
	shlo	31,r3,r3

	lda	0xffe00000,r12	#check for INF or NaN
	cmpo	r12,r4		#   if either NaN return NaN
	be.f	Lfmu81		#   if neither, A or B = 0 so return 0
	bl.f	Lfmu82
	cmpo	r12,r5
	bl.f	Lfmu82
	be.f	Lfmu87

	or	r4,g0,r13	#neither, see if A or B or both = 0
	cmpo	r13,0		#   if neither this was false alarm
	be.f	Lfmu84		#   we go back to mainstream
	or	r5,g2,r13
	cmpo	r13,0
	bne.f	Lfmu1

Lfmu84:	movl	0,g0		#return 0 if A or B = 0 and 
	or	r3,g1,g1	#   B is not NaN or INF
	b	Lfmu99

Lfmu81:  cmpo	r12,r5		# A was INF, what is B?
	bl.f	Lfmu82		#   NaN, return NaN
	cmpo	r5,0		#   0, return NaN
	bne.f	Lfmu83		#   else INF

Lfmu82:	subo	1,0,g0		# return Not a Number
	subo	1,0,g1
	b	Lfmu99

Lfmu87:	or	r4,g0,r13	# B = INF, return NaN if A = 0, else INF
	cmpo	r13,0	
	be.f	Lfmu82

Lfmu83:	lda	0x7ff00000,g1	#return INF
	or	r3,g1,g1
	mov	0,g0
	b	Lfmu99



/*  Routine to compare two double-precision floating point numbers.
    If the difference is no more than "Lffuzz" units, we call them equal.
    "Units" are mantissa bits in the larger of the two numbers.

    On entry:  g0-g1 = argument A
               g2-g3 = argument B
    On exit:   g0 = -1 A < B
    		     0 A = B
		     1 A > B
		     3 no comparison
*/

	.set	Lffuzz,48	#maximum difference for =

	.globl	___cmpdf2
	.align	4

___cmpdf2:
	shlo	1,g1,r4		#exponent and mantissa A -> r4
	shlo	1,g3,r6		#		       B -> r6

	lda	0xffe00000,r12	#check special cases
	cmpo	r12,r4		#   A == INF or NaN?  
	ble.f	Lfcp80		#
	cmpo	r12,r6		#   B == INF or NaN?
	ble.f	Lfcp90

	or	r4,r6,r6	#if both zero, equal
	cmpo	r6,0
	be.f	Lfcp8

	shri	31,g1,r12	#two's complement A to r6-r7
	clrbit	31,g1,r7
	xor	g0,r12,r6
	xor	r7,r12,r7
	cmpo	0,0
	subc	r12,r6,r6
	subc	0,r7,r7

	shri	31,g3,r12	#two's complement B to r8-r9
	clrbit	31,g3,r9
	xor	g2,r12,r8
	xor	r9,r12,r9
	cmpo	0,0
	subc	r12,r8,r8
	subc	0,r9,r9

#	cmpo	0,0		#calculate abs(A-B) -> r12-r13
#	subc	r8,r6,r12
#	subc	r9,r7,r13
#	shri	31,r7,r10
#	xor	r10,r12,r12
#	xor	r10,r13,r13
#	cmpo	0,0
#	subc	r10,r12,r12
#	subc	0,r13,r13

#	cmpo	r13,0		#if difference not larger than Lffuzz,
#	bne.f	Lfcp6		#   we eliminate it
#	lda	Lffuzz,r10
#	cmpo	r12,r10
#	ble.f	Lfcp8

Lfcp6:	cmpi	r7,r9		#compare sign, exponent, mantissa and all
	subo	1,0,g0
	bl.f	Lfcp99
	mov	1,g0
	bg.f	Lfcp99
	cmpo	r6,r8
	bg.f	Lfcp99
	subo	1,0,g0
	bl.f	Lfcp99

Lfcp8:	mov	0,g0

Lfcp99:	ret			#return to C program

# This is coding for invalid or special arguments

Lfcp80:	cmpo	r12,r4		#here A is INF or NaN
	be.f	Lfcp82

Lfcp8n:	mov	3,g0		#return "no compare"
	b	Lfcp99

Lfcp8g:	mov	1,g0		#return "greater"
	b	Lfcp99

Lfcp8l:	subo	1,0,g0		#return "less"
	b	Lfcp99

Lfcp82:  cmpo	r12,r6		# A was INF, what is B?
	bl.f	Lfcp8n		#   if NaN return "no compare"
	bg.f	Lfcp86		#   B regular, return sign of A
	xor	g1,g3,r12	#   A and B infinite, same sign?
	cmpi	r12,0		#   if yes, EQUAL, else 
	bge.f	Lfcp8		#   according to sign of A
	chkbit	31,g1
	bno.f	Lfcp8g
	b	Lfcp8l

Lfcp86:	cmpi	g1,0		#return > if A=INF, < if A=-INF
	bl	Lfcp8l
	b	Lfcp8g

Lfcp90:	cmpo	r12,r6		#here B is INF or NaN
	bl.f	Lfcp8n
	chkbit	31,g3
	be.f	Lfcp8g
	b	Lfcp8l
