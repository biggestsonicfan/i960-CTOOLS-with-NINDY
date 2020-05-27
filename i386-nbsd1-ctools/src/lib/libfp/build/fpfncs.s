/*
; ; ; ; ;
;	80960 FPAC/DPAC		Floating Point Library
;	FPFNCS			Trig/Trans Functions (SP)
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

	.set	Lfone,  0x3F800000    #              1.0000 00000E+00
	.set	Lftwo,  0x40000000    #              2.0000 00000E+00
	.set	Lfln2,  0x3F317218    # LN 2 =       6.9314 71806E-01
	.set	Lftwopi,0x40c90fdb    #              6.283185307
	.set	Lfpip4, 0x3f490fdb    #              pi/4
	.set	Lfpip2, 0x3fc90fdb    #              pi/2
	.set	Lflogc,0x3ede5bd9     # 1/ln 10 = 0.43429449
	.set	Lfinln2,0x3FB8AA3B    # 1 / LN 2  =  1.4426 95040E+00

/*  Routine to raise e to the specified power.
    On entry:  g0 = argument x
    On exit:   g0 = e^x
*/

	.file	"fpfncs.s"
	.link_pix

	.globl	_fpexp

#  Algorithm:
#  The argument is multiplied by 1/LN 2.  The integer portion of the
#  product becomes the two's exponent.  The mantissa is calculated
#  from the old series 
#
#	a^x = SIGMA((x ln a)^n / n!)
#
#  To enhance convergence, we scale the "x" to be less than 1/2 in
#  absolute value.  

	.text
	.align	4
Lfexpcn:				#coefficients for 2^x
	.word	0x00003ff9		#ln(2)^7/7! = 0.0000153
	.word	0x00028612		#ln(2)^6/6! = 0.0001540
	.word	0x0015d880		#ln(2)^5/5! = 0.0013334
	.word	0x009d955b		#ln(2)^4/4! = 0.0096181
	.word	0x038d611b		#ln(2)^3/3! = 0.0555041
	.word	0x0f5fdf00		#ln(2)^2/2! = 0.2402265
	.word	0x2c5c85fe		#ln(2)^1/1! = 0.6931472
	.word	0x40000000		#ln(2)^0/0! = 1.0000000
	.set	Ltc,8
	.text


_fpexp:
	shlo	1,g0,r4		#take exponent -> r4
	cmpo	r4,0
	be.f	Lexp82
	shro	24,r4,r4
	lda	0x7e,r12
	subo	r12,r4,r4

	shlo	8,g0,r13	#mantissa left-justified -> r13
	setbit	31,r13,r13

	lda	0xb8aa3b29,r10	#multiply with 1/ln(2)
	emul	r13,r10,r12
	cmpi	r13,0
	bl.t	Lexp2
	shlo	1,r13,r13
	subo	1,r4,r4

Lexp2:	cmpi	r4,6		#exponent max 6 (number < 128)
	bg.f	Lexp80

	shri	31,g0,r12	#initialize sign of fraction -> r12

	subo	r4,31,r10	#integer -> r7
	shro	r10,r13,r7

	addo	1,r4,r10	#fraction -> r13
	cmpi	r10,0
	bl	Lexp12
	shlo	r10,r13,r13
	b	Lexp13
Lexp12:	subo	r10,0,r10
	shro	r10,r13,r13

Lexp13:	chkbit	31,r13		#if fraction > 1/2 we use fraction - 1
	bno.f	Lexp6
	subo	r13,0,r13
	addo	1,r7,r7
	not	r12,r12

Lexp6:	shri	31,g0,r10	#make integer part 2's complement
	xor	r7,r10,r7
	subo	r10,r7,r7

	lda	Lfexpcn-(.+8)(ip),r11	#initialize for sum = x*sum + k
	ld	(r11),g1
	addo	4,r11,r11
	mov	Ltc-1,r10

Lexp5:	shri	31,g1,r14	# s = s * k
	emul	r13,g1,g0	#   signed multiply
	and	r13,r14,r14
	subo	r14,g1,g1
	xor	g1,r12,g1
	subo	r12,g1,g1

	ld	(r11),g0	#add to that next constant
	addo	4,r11,r11	#   loop to next
	cmpdeco	1,r10,r10
	addo	g0,g1,g1	
	bl.t	Lexp5

	shro	30,g1,r14	#convert result back to floating
	shro	r14,g1,g1
	chkbit	5,g1
	shro	6,g1,g1
	clrbit	23,g1,g0
	lda	0x7e(r14),r14
	shlo	23,r14,r14
	addc	g0,r14,g0

Lexp29:	shlo	23,r7,r7	#last add the part to exponent
	addo	r7,g0,g0

Lexp99:	ret

Lexp31:	mov	0,r11		#here fraction was 0, so mantissa = 1
	b	Lexp29

Lexp80:	shlo	1,g0,r4		#exponent too large, see if ff
	lda	0xff000000,r12
	cmpo	r12,r4
	bl.f	Lexp84

	cmpi	g0,0		#no, return INF or 0
	bl.f	Lexp87

	lda	0x7f800000,g0	#here exponent is too large, return INF
	b	Lexp99

Lexp82:	lda	Lfone,g0	#here argument 0, return 1
	b	Lexp99

Lexp84:	subo	1,0,g0		#NaN
	b	Lexp99

Lexp87:	mov	0,g0		#zero
	b	Lexp99


/*  Natural (base e) logarithm.
    On entry:  g0 = argument x
    On exit:   g0 = ln x
*/

	.globl	_fpln
	.globl	_fplog

#  ln ( (2^E) * M ) = (E * ln 2) + ln M
#           M between 1 and 2
#
#  We approximate the difference between "ln M" and "ln C" with the
#  textbook formula
#
#      ln(( 1+x)/(1-x) ) = 2(x + x^3/3 + x^5/5 + x^7/7 + ...
#
#  Substituting x = (M-C)/(M+C) the left side of the formula becomes
#  ln (M/C) = ln M - LN C  or precisely the delta we need.
#
#  We use as C's 1, 1 1/8, 1 2/8 and so on.

	.text
	.align	4

Lflnlog:			#a table of known logarithms
        .word   0x03f81516      #ln(1 1/64) = 0.01550419
        .word   0x0bba2c7b      #ln(1 3/64) = 0.04580954
        .word   0x1341d796      #ln(1 5/64) = 0.07522342
        .word   0x1a926d3a      #ln(1 7/64) = 0.10379679
        .word   0x21aefcfa      #ln(1 9/64) = 0.13157636
        .word   0x289a56da      #ln(1 11/64) = 0.15860503
        .word   0x2f571204      #ln(1 13/64) = 0.18492234
        .word   0x35e7929d      #ln(1 15/64) = 0.21056477
        .word   0x3c4e0edc      #ln(1 17/64) = 0.23556607
        .word   0x428c938a      #ln(1 19/64) = 0.25995752
        .word   0x48a507ef      #ln(1 21/64) = 0.28376817
        .word   0x4e993156      #ln(1 23/64) = 0.30702504
        .word   0x546ab61d      #ln(1 25/64) = 0.32975329
        .word   0x5a1b207a      #ln(1 27/64) = 0.35197642
        .word   0x5fabe0ee      #ln(1 29/64) = 0.37371641
        .word   0x651e5071      #ln(1 31/64) = 0.39499381
        .word   0x6a73b26a      #ln(1 33/64) = 0.41582790
        .word   0x6fad3677      #ln(1 35/64) = 0.43623677
        .word   0x74cbf9f8      #ln(1 37/64) = 0.45623743
        .word   0x79d10987      #ln(1 39/64) = 0.47584590
        .word   0x7ebd623e      #ln(1 41/64) = 0.49507727
        .word   0x8391f2e1      #ln(1 43/64) = 0.51394575
        .word   0x884f9cf2      #ln(1 45/64) = 0.53246480
        .word   0x8cf735a4      #ln(1 47/64) = 0.55064712
        .word   0x918986be      #ln(1 49/64) = 0.56850474
        .word   0x96074f6b      #ln(1 51/64) = 0.58604905
        .word   0x9a7144ed      #ln(1 53/64) = 0.60329085
        .word   0x9ec81354      #ln(1 55/64) = 0.62024041
        .word   0xa30c5e11      #ln(1 57/64) = 0.63690746
        .word   0xa73ec08e      #ln(1 59/64) = 0.65330127
        .word   0xab5fceae      #ln(1 61/64) = 0.66943065
        .word   0xaf70154a      #ln(1 63/64) = 0.68530400
	.text

	.set	Lln2,0xb17217f8	#ln(2) in 32 bits

_fpln:
	cmpo	g0,0		#check for NaN 0 INF neg
	be.f	Llog83
	lda	0x7f800000,r12
	cmpo	g0,r12
	bge.f	Llog80

	shro	23,g0,r3	#exponent to r3
	shlo	9,g0,r8		#   interpolation index -> r6
	shro	27,r8,r6

	shro	18,g0,r10	#special handling for
	lda	0xfdf,r11	#   63/64 < x < 1
	cmpo	r10,r11		#   1 < x < 65/64
	be.f	Llog40
	lda	0xfe0,r11
	cmpo	r10,r11
	be.f	Llog30

Llog3:	shro	2,r8,r8		# x scaled to 2 -> r8
	setbit	30,r8,r8

	lda	0x41[r6*2],r7	#this is C -> r7
	shlo	24,r7,r7

	subo	r7,r8,r11	#calculate x - C -> r11
	shri	31,r11,r12	#   and sign mask -> r12
	xor	r11,r12,r11
	subo	r12,r11,r11

	addo	r8,r7,g1	#take x + C

	mov	0,r10		#calculate x = (M-C)/(M+C)
	shlo	3,r11,r11
	ediv	g1,r10,r8

	emul	r9,r9,r10	#calculate x^2 -> r13
	shro	6,r11,r13

	lda	0x15555555,g1	#pick up first constant
	emul	r13,g1,g0	# k * x^2

	shlo	30,1,g0		#add next which is 1
	addo	g0,g1,g1	

	emul	r9,g1,g0	#then multiply by x, scaled to 1 again
	xor	r12,g1,g1
	subo	r12,g1,g1

	lda	Lflnlog-(.+8)(ip),g0
	ld	(g0)[r6*4],g0#then add the base of the area
	addo	g0,g1,g1

	lda	0x7e,r10	#handle exponent -1 here
	cmpo	r10,r3
	be.f	Llog50

	scanbit	g1,r10		#convert to floating-point
	bno.f	Llog22
	subo	r10,31,r11
	shlo	r11,g1,g1
	chkbit	7,g1
	shro	8,g1,g1
	clrbit	23,g1,g1
	lda	0x7f-32(r10),r10
	shlo	23,r10,r10
	addc	g1,r10,r11

Llog24:	lda	0xffffff81(r3),g0 #exponent to floating-point -> g0
	call	___floatsisf

	lda	Lfln2,g1	#calculate E*ln2
	call	___mulsf3

	mov	r11,g1		#add the ln M term to answer
	call	___addsf3

Llog99:	ret

Llog22:	mov	0,r11		#log of mantissa is all zero here
	b	Llog24

Llog50:	lda	Lln2,g0		#exponent -1 but x < 63/64
	subo	g1,g0,g1
	scanbit	g1,r10
	subo	r10,31,r11
	shlo	r11,g1,g1
	chkbit	7,g1
	shro	8,g1,g1
	clrbit	23,g1,g1
	lda	0x100+0x7f-32(r10),r10
	shlo	23,r10,r10
	addc	g1,r10,g0
	b	Llog99

Llog40:	subo	r8,0,r8		#entry for x little under 1
	shro	1,r8,r8

Llog30:				#entry for x little over 1
	lda	Lfone+0x80000000,g1
	call	___addsf3	#save x-1 -> r9
	mov	g0,r9		#   sign mask -> r11
	shri	31,r9,r11
	not	r11,r11

	shlo	28,1,g1		#compute 1+x/2+x^2/3+x^3/4
	emul	r8,g1,g0
	xor	r11,g1,g1
	subo	r11,g1,g1
	lda	0x15555555,g0
	addo	g1,g0,g1
	emul	r8,g1,g0
	xor	r11,g1,g1
	subo	r11,g1,g1
	shlo	29,1,g0
	addo	g1,g0,g1
	emul	r8,g1,g0
	xor	r11,g1,g1
	subo	r11,g1,g1
	shlo	30,1,g0
	addo	g1,g0,g1

	scanbit	g1,r10		#convert to floating-point
	bno.f	Llog22
	subo	r10,31,r11
	shlo	r11,g1,g1
	chkbit	7,g1
	shro	8,g1,g1
	clrbit	23,g1,g1
	lda	0x7f+2-32(r10),r10
	shlo	23,r10,r10
	addc	g1,r10,g1

	mov	r9,g0		#multiply in fp with x
	call	___mulsf3
	b	Llog99

# here we had bad or unusual arguments

Llog80:	cmpi	g0,0		#negative returns NaN
	bl.f	Llog82

	b	Llog99		#it must be INF or NaN, return as such

Llog82:	subo	1,0,g0	#return NaN
	b	Llog99

Llog83:	lda	0xff800000,g0	#return -INF
	b	Llog99

_fplog:
	call	_fpln
	lda	Lflogc,g1
	call	___mulsf3
	ret


/*  SIN(x), COS(x)
    On entry:  g0 = argument x
    On exit:   g0 = sin(x) or cos(x)
*/

	.globl	_fpsin
	.globl	_fpcos
	.globl	_fptan

#  We use the polynomials
#
#       sin(x) = SIGMA( -1^n(x^(2n-1)/(2n-1)! )
#       cos(x) = SIGMA( -1^n(x^(2n)/(2n)! )
#
#  to approximate the true value.  To enhance convergence, we reduce x
#  to between 0 and pi/4 and get the answer for other values with the
#  following formulas:
#
#       trig(x + 2pi) = trig(x)
#       sin(x+pi/2) = cos(x)
#       cos(x+pi/2) = -sin(x)
#	sin(pi/2 - x) = cos(x)
#	cos(pi/2 - x) = sin(x)
#       sin(-x) = -sin(x)
#       cos(-x) = cos(x)

	.text
	.align	4
Lsinc:				#coefficients for sin(x)
        .word   0x00000b8f              #1/9! = 0.0000028
        .word   0xfffcbfcd              #-1/7! = -0.0001984
        .word   0x00888889              #1/5! = 0.0083333
        .word   0xf5555556              #-1/3! = -0.1666667
        .word   0x40000000              #1/1! = 1.0000000
	.set	Ltcs,5

Lcosc:				#coefficients for cos(x)
        .word   0xfffffed9              #-1/10! = -0.0000003
        .word   0x00006807              #1/8! = 0.0000248
        .word   0xffe93e95              #-1/6! = -0.0013889
        .word   0x02aaaaab              #1/4! = 0.0416667
        .word   0xe0000001              #-1/2! = -0.5000000
        .word   0x40000000              #1/0! = 1.0000000
	.set	Ltcc,6

Ltanc:				#coefficients for tan(x)
        .word   0xffff91da      # -0.000026262
        .word   0xfffc9837      # -0.000207850
        .word   0xffdd4da8      # -0.002117717
        .word   0xfe93ea0f      # -0.022222029
        .word   0xeaaaaaa3      # -0.333333341
        .word   0x40000000      # 1.000000000
	.set	Ltct,6

	.text

_fpsin:
	shro	31,g0,r3	#set sign bit in r3
	shlo	31,r3,r3

	bal	Lreduct		#argument to a pi/4 interval -> r9
	cmpi	g0,0
	bl.f	Lsin99
	mov	g0,r9

	shro	2,g2,r12	#sin(x+pi) = -sinx
	shlo	31,r12,r12	#   so if octant > 3 we flip the sign
	xor	r3,r12,r3	#   and subtract 4 from octant number
	clrbit	2,g2,g2

	subo	1,g2,r12	#if octant is 1 or 2 we take cosx
	cmpo	r12,2
	bl.f	Lcos3

Lsin3:	mov	Ltcs-1,r10	#initialize for loop to do
	lda	Lsinc-(.+8)(ip),r11	#   sum = x^2*sum + k

	emul	r13,r13,r12	#make this x^2

	ld	(r11),g1	#pick up first constant
	addo	4,r11,r11

Lsin5:	shri	31,g1,r14	# s = s * k
	emul	r13,g1,g0	#   signed multiply
	and	r13,r14,r14
	subo	r14,g1,g1

	ld	(r11),g0	#add to that next constant
	addo	4,r11,r11	#   go take next
	cmpdeco	1,r10,r10
	addo	g0,g1,g1	
	bl.t	Lsin5

	shro	30,g1,r14	#convert result back to floating
	shro	r14,g1,g1
	chkbit	5,g1
	shro	6,g1,g1
	clrbit	23,g1,g0
	lda	0x7e(r14),r14
	shlo	23,r14,r14
	addc	g0,r14,g0

	mov	r9,g1		#last multiply by x
	call	___mulsf3

	or	r3,g0,g0	#put in the sign

Lsin99:	mov	0,g14		#required by Intel C compiler
	ret

_fpcos:
	mov	0,r3		#clear sign bit in r3

	bal	Lreduct		#argument to a pi/4 interval -> r9
	cmpi	g0,0
	bl.f	Lcos99
	mov	g0,r9

	shro	2,g2,r12	#cos(x+pi) = -cosx
	shlo	31,r12,r12	#   so if octant > 3 we flip the sign
	xor	r3,r12,r3	#   and subtract 4 from octant number
	clrbit	2,g2,g2

	shro	1,g2,r12	#we flip the sign in octants 2 and 3
	shlo	31,r12,r12
	xor	r3,r12,r3

	subo	1,g2,r12	#if octant is 1 or 2 we use sinx
	cmpo	r12,2
	bl.f	Lsin3

Lcos3:	mov	Ltcc-1,r10	#initialize for loop to do
	lda	Lcosc-(.+8)(ip),r11	#   sum = x^2*sum + k

	emul	r13,r13,r12	#calculate x^2

	ld	(r11),g1	#pick up first constant
	addo	4,r11,r11

Lcos5:	shri	31,g1,r14	# s = s * k
	emul	r13,g1,g0	#   signed multiply
	and	r13,r14,r14
	subo	r14,g1,g1

	ld	(r11),g0	#add to that next constant
	addo	4,r11,r11	#   go take next
	cmpdeco	1,r10,r10
	addo	g0,g1,g1	
	bl.t	Lcos5

	shro	30,g1,r14	#convert result back to floating
	shro	r14,g1,g1
	chkbit	5,g1
	shro	6,g1,g1
	clrbit	23,g1,g0
	lda	0x7e(r14),r14
	shlo	23,r14,r14
	addc	g0,r14,g0

	or	r3,g0,g0	#put in the sign

Lcos99:	mov	0,g14		#required by Intel C compiler
	ret


_fptan:
	shro	31,g0,r3	#set sign bit in r3
	shlo	31,r3,r3

	bal	Lreduct		#argument to a pi/4 interval -> r9
	cmpi	g0,0
	bl.f	Ltan99
	mov	g0,r9

	clrbit	2,g2,g2		#tan(x+pi) = tan(x) 

	shro	1,g2,r12	#if octant is 2 or 3 flip sign
	shlo	31,r12,r12
	xor	r12,r3,r3

	mov	Ltct-1,r10	#initialize for loop to do
	lda	Ltanc-(.+8)(ip),r11	#   sum = x^2*sum + k

	emul	r13,r13,r12	#calculate x^2

	ld	(r11),g1	#pick up first constant
	addo	4,r11,r11

Ltan5:	shri	31,g1,r14	# s = s * k
	emul	r13,g1,g0	#   signed multiply
	and	r13,r14,r14
	subo	r14,g1,g1

	ld	(r11),g0	#add to that next constant
	addo	4,r11,r11	#   go take next
	cmpdeco	1,r10,r10
	addo	g0,g1,g1	
	bl.t	Ltan5

	shro	30,g1,r14	#convert result back to floating
	shro	r14,g1,g1
	chkbit	5,g1
	shro	6,g1,g1
	clrbit	23,g1,g0
	lda	0x7e(r14),r14
	shlo	23,r14,r14
	addc	g0,r14,g0

	mov	r9,g1		#tan(x) = x/SUM
	subo	1,g2,r12	#   if octant is 1 or 2 we take 1/tan(x)
	cmpo	r12,2
	bl.f	Ltan7
	mov	g0,g1
	mov	r9,g0
Ltan7:	call	___divsf3

	or	r3,g0,g0	#put in the sign

Ltan99:	mov	0,g14		#required by Intel C compiler
	ret


# routine to reduce the argument to between 0 and pi/4
# on entering:  g0 = argument
# on exit:      g0 = reduced argument
#               g2 = octant number (0-7)

	.set	Lpi,0xc90fdaa2	#value of pi shifted left 30 bits
	.set 	Lpilo,0x2168c235

Lreduct:
	shlo	1,g0,r4		#left-justify exponent -> r4
	shro	1,r4,g0		#   also clear the sign bit

	lda	Lfpip4<<1,r12	#do nothing if less than pi/4
	cmpo	r4,r12
	bge.f	Lred1

	shro	24,r4,r14	#scaled mantissa -> r13
	lda	0x7e,r13	
	subo	r14,r13,r14
	shlo	8,g0,r13
	setbit	31,r13,r13
	shro	r14,r13,r13

	mov	0,g2		#octant then 0

Lred99: 	bx	(g14)		#back to the caller

Lred1:	shro	24,r4,r4	#exponent -> r4

	lda	0xffffff81(r4),r4 #exponent unbiased -> r4
	cmpi	r4,24		#   reject large exponent
	bge.f	Lred82		#   because most precision would be gone

	shlo	8,g0,r6		#mantissa left-justified -> r6
	setbit	31,r6,r6

	lda	Lpi,r10		#integer pi left-justified -> r10
	lda	Lpilo,r11

	addo	1,r4,r12	#get shift count -> r12

	mov	0,r7		#divide by subtraction
	mov	0,r8
	mov	0,g2
Lred61:	cmpo	0,0
	subc	r11,r7,r7
	subc	r10,r6,r6
	subc	0,r8,r8
	be.t	Lred63
	addc	r11,r7,r7
	addc	r10,r6,r6
	cmpo	1,0
Lred63:	addc	g2,g2,g2
	cmpdeci	0,r12,r12
	bge.t	Lred65
	addc	r7,r7,r7
	addc	r6,r6,r6
	teste	r8
	b	Lred61

Lred65:	and	7,g2,g2

	chkbit	0,g2		#if octant = 1 or 3 give pi/4-x
	bno.t	Lred11
	subc	r7,r11,r7
	subc	r6,r10,r6

Lred11:	mov	r6,r13		#scaled mantissa -> r13

	scanbit	r6,r8		#normalize remainder
	subo	r8,31,r12
	shlo	r12,r6,r6
	shro	r8,r7,r7
	shro	1,r7,r7
	or	r6,r7,r6
	chkbit	7,r6
	shro	8,r6,r6
	clrbit	23,r6,r6

	lda	0x7e,r10	#put in exponent
	subo	r12,r10,r12
	shlo	23,r12,r12
	addc	r6,r12,g0

	b	Lred99

Lred82:	subo	1,0,g0		#return NaN
	b	Lred99


	.globl	_fpsqrt
#
# Calculate square root using the iterative formula
#
#    y    = (y  + x / y ) * 1/2
#     n+1     n        n
#

	.set	Lsqrt2,0xb504f334	#sqrt2 * 2^31

	.text
	.align	4
Livtab:	.word	0x80ff01fb,0x82f73478,0x84e7ee6c,0x86d1826d
	.word	0x88b43d45,0x8a90668a,0x8c66410f,0x8e360b59
	.word	0x90000000,0x91c45600,0x9383410c,0x953cf1d1
	.word	0x96f19633,0x98a15985,0x9a4c64bd,0x9bf2dea0
	.word	0x9d94ebeb,0x9f32af78,0xa0cc4a61,0xa261dc1f
	.word	0xa3f382a5,0xa5815a7c,0xa70b7ed6,0xa89209ab
	.word	0xaa1513c6,0xab94b4dc,0xad11039a,0xae8a15b7
	.word	0xb0000000,0xb172d668,0xb2e2ac14,0xb44f9363

	.text

_fpsqrt:
	lda	0x7f7fffff,r12	#test for INF, NaN, 0, neg.
	cmpo	r12,g0
	concmpo	g0,0
	ble.f	Lsqr80

	shlo	9,g0,r5		#mantissa justified to bit 28 -> r5
	shro	27,r5,r6	#   interpolation interval -> r6
	lda	Livtab-(.+8)(ip),r9
	ld	(r9)[r6*4],r9	#   initial estimate -> r9
	shro	4,r5,r5
	setbit	28,r5,r5
	mov	0,r4

	shro	1,r9,r10	#do 1 iteration
	ediv	r10,r4,r8	# y = (x/y + y)/2
	addo	r10,r9,r9

	shro	1,r9,r10	#do 1 iteration
	ediv	r10,r4,r8	# y = (x/y + y)/2
	addo	r10,r9,r9

	shro	23,g0,r4	#2 * exponent -> r4
	lda	0x7f(r4),r4

	chkbit	0,r4		#odd exponent: have to multipy with
	bno.t	Lsqr12		#   sqrt(2)
	lda	Lsqrt2,r10
	emul	r9,r10,r8
	shlo	1,r9,r9
	clrbit	0,r4,r4

Lsqr12:	shro	8,r9,r9		#compose the answer
	clrbit	23,r9,r9
	shlo	22,r4,r4
	or	r9,r4,g0

Lsqr99:	ret

Lsqr80:	cmpi	g0,0		#zero returned as such
	be.f	Lsqr99		#   less than as NaN
	bl.f	Lsqr81

	lda	0x7f800000,g0	#INF as such
	b	Lsqr99

Lsqr81:	subo	1,0,g0		#here return NaN
	b	Lsqr99


	.globl	_fpxtoi

#  This is x to an integer power.  The integer i is a sum of different
#  powers of 2, so we can apply the formula
#
#     x^(a+b+ .. ) = x^a * x^b * ...
#
#  Each set bit in i causes a multiplier  x^2^n  which is best calculated
#  by squaring x successively n times.
#

_fpxtoi:
	cmpo	g1,0		#zero exponent special case
	be.f	Ltoi80

Ltoi4:	shri	31,g1,r11	#sign of i -> r11
	xor	g1,r11,r10	#   and absolute value -> r10
	subo	r11,r10,r10

	mov	g0,r9		# (x^2)^n -> r9

	chkbit	0,r10		#initialize r5 to 1 or x depending on
	be.f	Ltoi7		#   low bit of exponent
	lda	Lfone,g0
	b	Ltoi7

Ltoi3:	mov	r9,g0		#generate (x^2)^n
	mov	g0,g1
	call	___mulsf3
	mov	g0,r9

	chkbit	0,r10		#if corresponding bit in i not set,
	bno.t	Ltoi8		#   skip this power

	mov	r5,g1		#included, multiply
	call	___mulsf3

Ltoi7:	mov	g0,r5		#sum to register r5

Ltoi8:	shro	1,r10,r10	#next round, unless no more bits
	cmpo	r10,0
	bne.t	Ltoi3

	cmpo	r11,0		#if i was negative, take one over
	be.f	Ltoi99
	mov	g0,g1
	lda	Lfone,g0
	call	___divsf3

Ltoi99:	ret

Ltoi80:	shlo	1,g0,r4		#any special ^ 0 = NaN
	lda	0xfeffffff,r12	#   other let code decide
	cmpo	r12,r4
	concmpo	r4,0
	bg	Ltoi4

	subo	1,0,g0		#here it's NaN
	b	Ltoi99

	.globl	_fpatn

#  The arctangent function is approximated with the polynomial
#
#     atan(x) = x - x^3/3 + x^5/5
#
#  This is only usable for small absolute values of x.  Therefore, we
#  break x into two components:
#
#     x = C + d 
#
#  where d is much smaller than 1.  To get atan(x) we use the following
#  formula:
#
#    atan(C + d) = atan(C) + atan(d/(1 + dC + C^2))
#
#  We use the following formulas to initially reduce the argument to
#  between 0 and 1:
#
#         atan(-x) = -atan(x)
#         atan(1/x) = pi/2 - atan(x)
#

	.text
	.align	4

Lattab:				#interpolation table for (2n+1)/64
        .word   0x03ffeaab      #atan(1/64) = 0.01562373
        .word   0x0bfdc0c2      #atan(3/64) = 0.04684071
        .word   0x13f59f0e      #atan(5/64) = 0.07796663
        .word   0x1be39ebe      #atan(7/64) = 0.10894196
        .word   0x23c3f5f6      #atan(9/64) = 0.13970887
        .word   0x2b93023c      #atan(11/64) = 0.17021193
        .word   0x334d51d3      #atan(13/64) = 0.20039855
        .word   0x3aefabbe      #atan(15/64) = 0.23021959
        .word   0x4277165f      #atan(17/64) = 0.25962963
        .word   0x49e0dc81      #atan(19/64) = 0.28858736
        .word   0x512a90db      #atan(21/64) = 0.31705575
        .word   0x5852100c      #atan(23/64) = 0.34500218
        .word   0x5f55812e      #atan(25/64) = 0.37239845
        .word   0x66335515      #atan(27/64) = 0.39922077
        .word   0x6cea4477      #atan(29/64) = 0.42544964
        .word   0x73794d0d      #atan(31/64) = 0.45106966
        .word   0x79dfadfc      #atan(33/64) = 0.47606933
        .word   0x801ce39f      #atan(35/64) = 0.50044081
        .word   0x8630a2db      #atan(37/64) = 0.52417963
        .word   0x8c1ad446      #atan(39/64) = 0.54728438
        .word   0x91db8f17      #atan(41/64) = 0.56975645
        .word   0x97731421      #atan(43/64) = 0.59159971
        .word   0x9ce1c8e7      #atan(45/64) = 0.61282020
        .word   0xa22832dc      #atan(47/64) = 0.63342588
        .word   0xa746f2de      #atan(49/64) = 0.65342634
        .word   0xac3ec0fc      #atan(51/64) = 0.67283255
        .word   0xb110688b      #atan(53/64) = 0.69165662
        .word   0xb5bcc491      #atan(55/64) = 0.70991162
        .word   0xba44bc7e      #atan(57/64) = 0.72761133
        .word   0xbea94145      #atan(59/64) = 0.74477013
        .word   0xc2eb4abc      #atan(61/64) = 0.76140277
        .word   0xc70bd54d      #atan(63/64) = 0.77752431
	.text

_fpatn:
	shro	31,g0,r3	#sign bit -> r3
	shlo	31,r3,r3
	clrbit	31,g0,g0

	mov	0,r4		#clear flag word -> r4

	cmpo	g0,0		#check for NaN 0
	be.f	Latn24
	lda	0x7f800000,r12
	cmpo	g0,r12
	bg.f	Latn26

	lda	Lfone,r12	#if x > 1 we invert it and set flag
	cmpo	g0,r12
	bl.f	Latn3
	be.f	Latn21
	mov	g0,g1
	lda	Lfone,g0
	call	___divsf3
	setbit	0,r4,r4

Latn3:	shro	23,g0,r10	#scaled x -> r8
	lda	0x7e,r11
	subo	r10,r11,r10
	shlo	8,g0,r8
	setbit	31,r8,r8
	shro	r10,r8,r8

	cmpo	r10,6		#treat first 1/64 differently 
	bge.f	Latn30		#   not to lose precision

	shro	27,r8,r6	#interpolation index -> r6

	lda	1[r6*2],r7	#this is C -> r7
	shlo	26,r7,r7

	subo	r7,r8,r11	#calculate x - C = d -> r11
	shri	31,r11,r12	#   and sign mask -> r12
	xor	r11,r12,r11
	subo	r12,r11,r11

	emul	r7,r8,g0	#take 1 + CX scaled to 2
	shro	1,g1,g1
	setbit	31,g1,g1

	mov	0,r10		#calculate d/(1 + dC + C2)
	ediv	g1,r10,r10	#   = d/(1 + C*x)
	shro	1,r11,r9

	emul	r9,r9,r10	#calculate x^2 -> r13
	mov	r11,r13

	lda	0xeaaaaaaa,g1	#pick up first constant

	emul	r13,g1,g0	# k * x^2
	subo	r13,g1,g1

	shlo	30,1,g0		#add next which is 1
	addo	g0,g1,g1	

	shlo	2,r9,r9		#then multiply by x, scaled to 1 again
	emul	r9,g1,g0
	xor	r12,g1,g1
	subo	r12,g1,g1

	lda	Lattab-(.+8)(ip),g0
	ld	(g0)[r6*4],g0	#then add the base of the area
	addo	g0,g1,g1

	scanbit	g1,r10		#convert to floating-point
	bno.f	Latn23
	subo	r10,31,r11
	shlo	r11,g1,g1
	chkbit	7,g1
	shro	8,g1,g1
	clrbit	23,g1,g1
	lda	0x7f-32(r10),r10
	shlo	23,r10,r10
	addc	g1,r10,g0

Latn16:	chkbit	0,r4		#if argument was inverted, we return
	bno.t	Latn12		#   pi/2 - y
	notbit	31,g0,g0
	lda	Lfpip2,g1
	call	___addsf3

Latn12:	or	r3,g0,g0	#put in the sign

Latn99:	mov	0,g14		#required by Intel C compiler
	ret

Latn30:	mov	g0,r9		#save fp argument

	emul	r8,r8,r10	#calculate x^2 -> r13
	mov	r11,r13

	lda	0xeaaaaaaa,g1	#pick up first constant

	emul	r13,g1,g0	# k * x^2
	subo	r13,g1,g1

	shlo	30,1,g0		#add next which is 1
	addo	g0,g1,g1	

	scanbit	g1,r10		#convert to floating-point
	bno.f	Latn23
	subo	r10,31,r11
	shlo	r11,g1,g1
	chkbit	7,g1
	shro	8,g1,g1
	clrbit	23,g1,g1
	lda	0x7f+2-32(r10),r10
	shlo	23,r10,r10
	addc	g1,r10,g0

	mov	r9,g1		#floating-point multiply with x
	call	___mulsf3
	b	Latn16

Latn21:	lda	Lfpip4,g0	#atn(1)
	b	Latn12

Latn23:	mov	0,g1		#differential is zero
	b	Latn16

Latn24:	mov	0,g0		#zero
	b	Latn12

Latn26:	subo	1,0,g0		#NaN
	b	Latn99
