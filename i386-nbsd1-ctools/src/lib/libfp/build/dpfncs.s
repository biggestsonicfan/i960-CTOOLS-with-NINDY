/*
; ; ; ; ;
;	80960 FPAC/DPAC		Floating Point Library
;	DPFNCS			Trig/Trans Functions (DP)
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

	.link_pix

	.set	Lfhalfh,0x3fe00000  # 1/2
	.set	Lfoneh,0x3ff00000   # 1
	.set	Lfln2h,0x3FE62E42   # LN 2 = 6.9314 71805 59945 30942 D-01
	.set	Lfln2l,0xFEFA39EF
	.set	Lfpip4h,0x3FE921FB  # pi/4
	.set	Lfpip4l,0x54442D18
	.set	Lfpip2h,0x3FF921FB  # pi/2
	.set	Lfpip2l,0x54442D18
	.set	Lflogch,0x3FDBCB7B  # 1/LN 10 = 0.43429 44819 03251 82765
	.set	Lflogcl,0x1526E50E
	.set 	Lfinln2h,0x3FF71547 # 1/LN 2 = 1.4426 95040 88896 34074 D+00
	.set	Lfinln2l,0x652B82FE
	.set	Lfsqrt2h,0x3ff6a09e # sqrt(2)
	.set	Lfsqrt2l,0x667f3bcd

/*  Routine to raise e to the specified power.
    On entry:  g0-1 = argument x
    On exit:   g0-1 = e^x
*/

	.file	"dpfncs.s"
	.globl	_dpexp

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
	.align	8
Lfexpcn:				#coefficients for 2^x
        .word   0x070ef594,0x00000000   #ln(2)^12/12! = 0.000000000025678
        .word   0x7a32b1cd,0x00000000   #ln(2)^11/11! = 0.000000000444554
        .word   0x933d4562,0x00000007   #ln(2)^10/10! = 0.000000007054912
        .word   0x494f4e57,0x0000006d   #ln(2)^9/9! = 0.000000101780860
        .word   0x0088e972,0x0000058b   #ln(2)^8/8! = 0.000001321548679
        .word   0x7f8b1161,0x00003ff9   #ln(2)^7/7! = 0.000015252733804
        .word   0x25f0d8f0,0x00028612   #ln(2)^6/6! = 0.000154035303934
        .word   0xe78a6730,0x0015d87f   #ln(2)^5/5! = 0.001333355814643
        .word   0x7dd273b8,0x009d955b   #ln(2)^4/4! = 0.009618129107628
        .word   0xe09417e0,0x038d611a   #ln(2)^3/3! = 0.055504108664822
        .word   0xc162c700,0x0f5fdeff   #ln(2)^2/2! = 0.240226506959101
        .word   0xf473decb,0x2c5c85fd   #ln(2)^1/1! = 0.693147180559945
        .word   0x00000000,0x40000000   #ln(2)^0/0! = 1.000000000000000
	.set	Ltc,13
	.text


_dpexp:
	shlo	1,g1,r3		#check for NaN
	lda	0xffe00000,r12
	cmpo	r3,r12
	bg.f	Lexp84

	movl	g0,r4		#save original number
	clrbit	31,g1,g1	#   force positive

	shro	21,r3,r3	#exponent bits 0-10: AE -> r3	

	shlo	11,g1,r7	#isolate mantissa left-justified
	shlo	11,g0,r6	#   AM -> r6-r7
	shro	21,g0,r13	#
	or	r13,r7,r7	#
	setbit	31,r7,r7	#
	lda	0xb8aa3b29,r9	#   BM -> r8-r9
	lda	0x5c17f0bc,r8

	emul	r7,r9,r12	#multiply x * 1/ln(2) -> r6-7
	emul	r6,r9,r10
	cmpo	0,1
	addc	r12,r11,r12
	addc	r13,0,r13
	emul	r7,r8,r14
	addc	r10,r14,r14
	addc	r12,r15,r6
	addc	r13,0,r7
	addo	1,r3,r3

	chkbit	31,r7		#normalize, we may be off by 1 bit
	be.t	Lexq3
	addc	r6,r6,r6
	addc	r7,r7,r7
	subo	1,r3,r3

Lexq3:	lda	0x3fe,r8	#shift count to remove integers -> r8
	subo	r8,r3,r8

	cmpi	r8,12		#if too many integer bits, return INF
	bge.f	Lexp88

	cmpi	r8,0		#number (0 - 1) no change
	mov	0,r14
	ble.f	Lexq11

	subo	r8,r3,r3	#remove integer part
	lda	32,r9		#   and put integer -> r14
	subo	r8,r9,r9
	shro	r9,r7,r14
	shlo	r8,r7,r7
	shro	r9,r6,r10
	or	r7,r10,r7
	shlo	r8,r6,r6

	scanbit	r7,r12		#normalize the mantissa, left-justified
	be.t	Lexq12
	scanbit	r6,r12
	bno.f	Lexp87
	subo	r12,31,r12
	shlo	r12,r6,r7
	mov	0,r6
	lda	32(r12),r12
	b	Lexq17
Lexq12:	shro	r12,r6,r13
	shro	1,r13,r13
	subo	r12,31,r12
	shlo	r12,r7,r7
	shlo	r12,r6,r6
	or	r13,r7,r7

Lexq17:	subo	r12,r3,r3	#adjust exponent

Lexq11:	chkbit	10,r6		#set flag for rounding bit

	clrbit	31,r7,r7	#clear hidden bit in mantissa

	shro	11,r6,g0	#right-justify mantissa to g0-g1
	shlo	21,r7,r13
	shro	11,r7,g1

	shlo	20,r3,r3	#combine mantissa, exponent, sign,
	or	g1,r3,g1	#   rounding bit
	addc	r13,g0,g0
	addc	0,g1,g1

	mov	r14,r7		#save integer -> r7

	lda	Lfhalfh,r12	#if fraction > 1/2 we use fraction - 1
	cmpo	g1,r12
	bl	Lexp6
	lda	Lfoneh,g3
	mov	0,g2
	call	___subdf3
	addo	1,r7,r7

Lexp6:	shro	31,r5,r12	#put the original sign, save
	shlo	31,r12,r12	#   fraction -> r8-9
	xor	r12,g1,g1
	movl	g0,r8

	shri	31,r5,r12	#make integer 2's complement
	xor	r7,r12,r7
	subo	r12,r7,r7

	mov	Ltc-1,r10	#initialize for polynomial loop
	lda	Lfexpcn-(.+8)(ip),r11	#   x = r8  sum = g0
	bal	Lsigmax

Lexp29:	shlo	20,r7,r7	#last add the part to exponent
	addo	r7,g1,g1
	lda	0x7ff00000,r12
	cmpo	g1,r12
	bge.f	Lexp89

Lexp99:	mov	0,g14
	ret

Lexp84:	subo	1,0,g0		#NaN
	subo	1,0,g1
	b	Lexp99

Lexp87:	movl	0,g0		#here the decimal part = 0
	b	Lexp6

Lexp89:	mov	r7,r5		#entry for overflow

Lexp88:	cmpi	r5,0		#exponent oversize, return INF or 0
	bge	Lexp86
	movl	0,g0
	b	Lexp99

Lexp86:	lda	0x7ff00000,g1	#INF
	mov	0,g0
	b	Lexp99



/*  Natural (base e) logarithm.
    On entry:  g0 = argument x
    On exit:   g0 = ln x
*/

	.globl	_dpln
	.globl	_dplog

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
 	.align	8
Lflncns:				#coefficients for ln ((1+x)/(1-x))
        .word   0xec4ec4ec,0x04ec4ec4   #1/13
        .word   0x1745d174,0x05d1745d   #1/11 = 0.090909090909091
        .word   0x1c71c71c,0x071c71c7   #1/9 = 0.111111111111111
        .word   0x24924925,0x09249249   #1/7 = 0.142857142857143
        .word   0xcccccccd,0x0ccccccc   #1/5 = 0.200000000000000
        .word   0x55555555,0x15555555   #1/3 = 0.333333333333333
        .word   0x00000000,0x40000000   #1/1 = 1.000000000000000
	.set	Ltc,7

Lflnlog:				#a table of known logarithms
	.word	0,0x3ff00000		# 1
	.word	0x00000000,0x00000000   # ln 1      = 0
	.word	0,0x3FF20000 		# 1.125 
	.word	0x6E2AF2E5,0x3FBE2707   # ln 1.125 = 0.117783035656383
	.word	0,0x3FF40000 		# 1.25
	.word	0xC79A9A21,0x3FCC8FF7   # ln 1.25  = 0.22314355131421 
	.word	0,0x3FF60000 		# 1.375
	.word	0xC21C5EC3,0x3FD4618B   # ln 1.375 = 0.318453731118535
	.word	0,0x3FF80000 		# 1.5  
	.word	0xECBF984C,0x3FD9F323   # ln 1.5   = 0.405465108108164
	.word	0,0x3FFA0000 		# 1.625
	.word	0x5FAF06ED,0x3FDF128F   # ln 1.625 = 0.485507815781701
	.word	0,0x3FFC0000 		# 1.75 
	.word	0x5E7040D0,0x3FE1E85F   # ln 1.75  = 0.559615787935423
	.word	0,0x3FFE0000 		# 1.875
	.word	0xE84672AE,0x3FE41D8F   # ln 1.875 = 0.628608659422374
	.text


_dpln:
	lda	0x7fefffff,r12	#reject illegal arguments
	cmpo	r12,g1		#   negative, INF, NaN, 0
	concmpo	g1,0
	ble.f	Llog80

	shlo	1,g1,r4		#original exponent -> r4
	shro	21,r4,r4

	movl	g0,r10		#argument with exponent = 0 -> r10
	shlo	12,r11,r3	#   and area number -> r3
	shro	12,r3,r11
	lda	0x3ff00000(r11),r11
	shro	29,r3,r3

	cmpo	r3,7		#if area = 7 use area "-1" not to lose
	bl.t	Llog3		#   precision
	mov	0,r3
	addo	1,r4,r4
	clrbit	20,r11,r11

Llog3:	
	lda	Lflnlog-(.+8)(ip),r6
	ldl	(r6)[r3*16],r6 #get M to -> r6

	movl	r6,g2		#get M+C -> r8
	movl	r10,g0
	call	___adddf3
	movl	g0,r8

	movl	r10,g0		#get M-C
	movl	r6,g2
	call	___subdf3

	movl	r8,g2		#calculate x = (M-C)/(M+C) -> r6
	call	___divdf3
	movl	g0,r8

	mov	Ltc-1,r10	#initialize for polynomial loop
	lda	Lflncns-(.+8)(ip),r11	#   x^2 = r8  sum = g0
	bal	Lsigmax

	shlo	20,1,r12	#multiply by 2
	addo	g1,r12,g1

	movl	r8,g2		#then multiply by x
	call	___muldf3

Llog12:
	lda	Lflnlog+8-(.+8)(ip),g2
	ldl	(g2)[r3*16],g2 #then add the base of the area
	call	___adddf3
	movl	g0,r10

	lda	0xfffffc01(r4),g0 #exponent to floating point
	call	___floatsidf

	lda	Lfln2h,g3	#calculate E*ln2
	lda	Lfln2l,g2
	call	___muldf3

	movl	r10,g2		#add the ln M term to for answer
	call	___adddf3

Llog99:	mov	0,g14
	ret

Llog21:	movl	0,g0		#here x=0 so we use log C
	b	Llog12

# here we had bad or unusual arguments

Llog80:	cmpi	g1,0		#negative returns NaN
	bl.f	Llog82		#   all 0 returns -INF
	or	g0,g1,r12
	cmpo	r12,0
	be.f	Llog83

	b	Llog99		#it must be INF or NaN, return as such

Llog82:	subo	1,0,g0	#return NaN
	subo	1,0,g1
	b	Llog99

Llog83:	lda	0xfff00000,g1	#return -INF
	mov	0,g0
	b	Llog99

_dplog:
	call	_dpln
	lda	Lflogch,g3
	lda	Lflogcl,g2
	call	___muldf3
	ret



/*  SIN(x), COS(x)
    On entry:  g0 = argument x
    On exit:   g0 = sin(x) or cos(x)
*/

	.globl	_dpsin
	.globl	_dpcos
	.globl	_dptan

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
 	.align	8
Lsinc:				#coefficients for sin(x)
        .word   0xffca3019,0xffffffff   #-1/15! = -0.000000000000765
        .word   0x2c248c27,0x00000000   #1/13! = 0.000000000160590
        .word   0x19ba980b,0xffffffe5   #-1/11! = -0.000000025052108
        .word   0xf1d2ab63,0x00000b8e   #1/9! = 0.000002755731922
        .word   0xfcbfcbfd,0xfffcbfcb   #-1/7! = -0.000198412698413
        .word   0x88888888,0x00888888   #1/5! = 0.008333333333333
        .word   0x55555555,0xf5555555   #-1/3! = -0.166666666666667
        .word   0x00000000,0x40000000   #1/1! = 1.000000000000000
	.set	Ltcs,8

Lcosc:				#coefficients for cos(x)
        .word   0x00035cfe,0x00000000   #1/16! = 0.000000000000048
        .word   0xfcd8d16b,0xffffffff   #-1/14! = -0.000000000011471
        .word   0x3ddb1dff,0x00000002   #1/12! = 0.000000002087676
        .word   0x1b048877,0xfffffed8   #-1/10! = -0.000000275573192
        .word   0x80680680,0x00006806   #1/8! = 0.000024801587302
        .word   0xe93e93e9,0xffe93e93   #-1/6! = -0.001388888888889
        .word   0xaaaaaaab,0x02aaaaaa   #1/4! = 0.041666666666667
        .word   0x00000000,0xe0000000   #-1/2! = -0.500000000000000
        .word   0x00000000,0x40000000   #1/0! = 1.000000000000000
	.set	Ltcc,9

Ltanc:				#coefficients for tan(x)
        .word   0xffef4ece,0xffffffff   # -0.000000000000237
        .word   0xff5b4138,0xffffffff   # -0.000000000002341
        .word   0xf9a60784,0xffffffff   # -0.000000000023106
        .word   0xc1504d2f,0xffffffff   # -0.000000000228052
        .word   0x954f1c54,0xfffffffd   # -0.000000002250785
        .word   0x25b1e087,0xffffffe8   # -0.000000022214609
        .word   0x9269d8fc,0xffffff14   # -0.000000219259479
        .word   0xfd14dded,0xfffff6eb   # -0.000002164404281
        .word   0xc35023c6,0xffffa655   # -0.000021377799156
        .word   0x0d99621f,0xfffc8851   # -0.000211640211640
        .word   0x87fdd532,0xffdd532a   # -0.002116402116402
        .word   0x93e93e94,0xfe93e93e   # -0.022222222222222
        .word   0xaaaaaaab,0xeaaaaaaa   # -0.333333333333333
        .word   0x00000000,0x40000000   # 1.000000000000000
	.set	Ltct,14
	.text

_dpsin:
	shro	31,g1,r3	#set sign bit in r3
	shlo	31,r3,r3

	bal	Lreduct		#argument to a pi/4 interval -> r8
	cmpi	g1,0
	bl.f	Lsin99
	movl	g0,r8

	shro	2,g2,r12	#sin(x+pi) = -sinx
	shlo	31,r12,r12	#   so if octant > 3 we flip the sign
	xor	r3,r12,r3	#   and subtract 4 from octant number
	clrbit	2,g2,g2

	subo	1,g2,r12	#if octant is 1 or 2 we take cosx
	cmpo	r12,2
	bl.f	Lcos3

Lsin3:	mov	Ltcs-1,r10	#initialize for polynomial loop
	lda	Lsinc-(.+8)(ip),r11	#   x^2 = r8  sum = g0
	bal	Lsigmax

	movl	r8,g2		#then multiply by x
	call	___muldf3

	or	r3,g1,g1	#put in the sign

Lsin99:	mov	0,g14		#required by Intel C compiler
	ret

_dpcos:
	mov	0,r3		#clear sign bit in r3

	bal	Lreduct		#argument to a pi/4 interval -> r8
	cmpi	g1,0
	bl.f	Lcos99
	movl	g0,r8

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

Lcos3:	mov	Ltcc-1,r10	#initialize for polynomial loop
	lda	Lcosc-(.+8)(ip),r11	#   x^2 = r8  sum = g0
	bal	Lsigmax

	or	r3,g1,g1	#put in the sign

Lcos99:	mov	0,g14		#required by Intel C compiler
	ret


_dptan:
	shro	31,g1,r3	#set sign bit in r3
	shlo	31,r3,r3

	bal	Lreduct		#argument to a pi/4 interval -> r8
	cmpi	g1,0
	bl.f	Ltan99
	movl	g0,r8

	clrbit	2,g2,g2		#tan(x+pi) = tan(x) 

	shro	1,g2,r12	#if octant is 2 or 3 flip sign
	shlo	31,r12,r12
	xor	r12,r3,r3

	mov	g2,r4		#save octant -> r4

	mov	Ltct-1,r10	#initialize for loop to do
	lda	Ltanc-(.+8)(ip),r11	#   sum = x^2*sum + k
	bal 	Lsigmax

	movl	r8,g2		#tan(x) = x/SUM
	subo	1,r4,r12	#   if octant is 1 or 2 we take 1/tan(x)
	cmpo	r12,2
	bl.f	Ltan7
	movl	g0,g2
	movl	r8,g0
Ltan7:	call	___divdf3

	or	r3,g1,g1	#put in the sign

Ltan99:	mov	0,g14		#required by Intel C compiler
	ret


# routine to reduce the argument to between 0 and pi/4
# on entering:  g0 = argument
# on exit:      g0 = reduced argument
#               g2 = octant number (0-7)

	.set	Lpih,0xc90fdaa2	#value of pi shifted left 62 bits
	.set	Lpil,0x2168c234
	.set	Lpic,0xc0000000

Lreduct:
	clrbit	31,g1,g1	#drop the sign bit

	lda	Lfpip4h,r12	#do nothing if less than pi/4
	cmpo	g1,r12
	bg.f	Lred1
	bl.f	Lred19
	lda	Lfpip4l,r12
	cmpo	g0,r12
	bge.f	Lred1

Lred19:	mov	0,g2		#octant then 0

Lred99: 	bx	(g14)		#back to the caller

Lred1:	shro	20,g1,r4	#exponent -> r4

	lda	0xfffffc01(r4),r4 #exponent unbiased -> r4
	cmpi	r4,24		#   reject large exponents
	bge.f	Lred82		#   because most precision would be gone

	shlo	11,g1,r7	#mantissa left-justified -> r7:r6
	shro	21,g0,r13
	shlo	11,g0,r6
	or	r13,r7,r7
	setbit	31,r7,r7

	lda	Lpih,r9		#integer pi left-justified -> r9:r8:r11
	lda	Lpil,r8
	lda	Lpic,r11

	addo	1,r4,r12	#get shift count -> r12

	mov	0,r13		#divide by subtraction
	mov	0,g2		#   result left-justified r7:r6:r13
	mov	0,r14
Lred61:	cmpo	0,0
	subc	r11,r13,r13
	subc	r8,r6,r6
	subc	r9,r7,r7
	subc	0,r14,r14
	be.t	Lred63
	addc	r11,r13,r13
	addc	r8,r6,r6
	addc	r9,r7,r7
	cmpo	1,0
Lred63:	addc	g2,g2,g2
	cmpdeci	0,r12,r12
	bge.t	Lred65
	addc	r13,r13,r13
	addc	r6,r6,r6
	addc	r7,r7,r7
	teste	r14
	b	Lred61

Lred65:	and	7,g2,g2		#octant number

	chkbit	0,g2		#if octant = 1 or 3 give pi/4-x
	bno.t	Lred41
	subc	r13,r11,r13
	subc	r6,r8,r6
	subc	r7,r9,r7

Lred41:	mov	r13,r8		#argument now r7:r6:r8

	scanbit	r7,r12		#normalize the mantissa, left-justified
	be.t	Lred12		#   argument in r7:r6:r8
	scanbit	r6,r12
	bno.f	Lred32
	addo	1,r12,r13
	subo	r12,31,r12
	shro	r13,r8,r10
	shlo	r12,r6,r7
	or	r10,r7,r7
	shlo	r12,r8,r6
	lda	32(r12),r12
	b	Lred17
Lred12:	addo	1,r12,r13
	subo	r12,31,r12
	shro	r13,r8,r10
	shro	r13,r6,r11
	shlo	r12,r6,r6
	or	r10,r6,r6
	shlo	r12,r7,r7
	or	r11,r7,r7

Lred17:	lda	0x3fe,r4	#put the bias back into exponent
	subo	r12,r4,r4	#   and adjust by shift count

	mov	r6,r11		#save rounding bit ->  bit 10 in r11

	clrbit	31,r7,r7	#clear hidden bit in mantissa

	shro	11,r6,g0	#right-justify mantissa to g0-g1
	shlo	21,r7,r13
	shro	11,r7,g1
	or	r13,g0,g0

	shlo	20,r4,r4	#combine mantissa and exponent
	or	g1,r4,g1

	chkbit	10,r11		#round
	addc	0,g0,g0
	addc	0,g1,g1

	b	Lred99

Lred32:	movl	0,g0		#underflow, return 0
	b	Lred99

Lred82:	subo	1,0,g0		#return NaN
	subo	1,0,g1
	b	Lred99


	.globl	_dpsqrt
#
# Calculate square root using the iterative formula
#
#    y    = (y  + x / y ) * 1/2
#     n+1     n        n
#

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

_dpsqrt:
	lda	0x7fefffff,r12	#test for INF, NaN, 0, neg.
	cmpo	r12,g1
	concmpo	g1,0
	ble.f	Lsqr80

Lsqr1:	shlo	12,g1,r7	#mantissa justified to bit 28 -> r6
	shro	27,r7,r3	#   interpolation interval -> r3
	shro	4,r7,r7
	shro	24,g0,r13
	shlo	8,g0,r6
	or	r13,r7,r7
	setbit	28,r7,r7

	lda	Livtab-(.+8)(ip),r9
	ld	(r9)[r3*4],r9	#initial estimate -> r9

	shro	1,r9,r10	#do 1 iteration in 32 bits
	ediv	r10,r6,r8	# y = (x/y + y)/2
	addo	r10,r9,r9

	shro	1,r9,r10	#do 1 iteration in 32 bits
	ediv	r10,r6,r8	# y = (x/y + y)/2
	addo	r10,r9,r9

	shro	1,r9,r10	#iteration in 64 bits
	ediv	r10,r6,r8	#   r6/9 = x / y -> r10-11
	mov	r8,r13		#   to get 64 bits we divide the remainder
	mov	0,r12
	ediv	r10,r12,r12
	mov	r13,r8

	addo	r10,r9,r9	#add previous y (high 32 bits)

	shro	20,g1,r4	# 2 * exponent -> r4
	lda	0x3ff(r4),r4

	shro	11,r8,g0	#compose the answer
	shlo	21,r9,r12
	shro	11,r9,g1
	or	r12,g0,g0
	clrbit	20,g1,g1
	clrbit	0,r4,r5
	shlo	19,r5,r5
	or	g1,r5,g1

	chkbit	0,r4		#if odd exponent we multiply with
	bno.f	Lsqr99		#   sqrt(2)
	lda	Lfsqrt2h,g3
	lda	Lfsqrt2l,g2
	call	___muldf3

Lsqr99:	ret

Lsqr80:	be.f	Lsqr83		#if "eq" x-hi = 0
	cmpi	g1,0		#   if sign bit set return NaN
	bg	Lsqr99		#   INF and NaN as such

	subo	1,0,g0		#here return NaN
	subo	1,0,g1
	b	Lsqr99

Lsqr83:	or	g0,g1,r12	#if all zero return 0
	be	Lsqr99		#   else return to routine
	b	Lsqr1


	.globl	_dpxtoi

#  This is x to an integer power.  The integer i is a sum of different
#  powers of 2, so we can apply the formula
#
#     x^(a+b+ .. ) = x^a * x^b * ...
#
#  Each set bit in i causes a multiplier  x^2^n  which is best calculated
#  by squaring x successively n times.
#

_dpxtoi:
	cmpo	g2,0		#zero exponent special case
	be.f	Ltoi80

Ltoi4:	shri	31,g2,r11	#sign of i -> r11
	xor	g2,r11,r10	#   and absolute value -> r10
	subo	r11,r10,r10

	movl	g0,r8		# (x^2)^n -> r8

	chkbit	0,r10		#initialize r4 to 1 or x depending on
	be.f	Ltoi7		#   low bit of exponent
	lda	Lfoneh,g1
	mov	0,g0
	b	Ltoi7

Ltoi3:	movl	r8,g0		#generate (x^2)^n
	movl	g0,g2
	call	___muldf3
	movl	g0,r8

	chkbit	0,r10		#if corresponding bit in i not set,
	bno.t	Ltoi8		#   skip this power

	movl	r4,g2		#included, multiply
	call	___muldf3

Ltoi7:	movl	g0,r4		#sum to register r4

Ltoi8:	shro	1,r10,r10	#next round, unless no more bits
	cmpo	r10,0
	bne.t	Ltoi3

	cmpo	r11,0		#if i was negative, take one over
	be.f	Ltoi99
	movl	g0,g2
	lda	Lfoneh,g1
	mov	0,g0
	call	___divdf3

Ltoi99:	ret

Ltoi80:	shlo	1,g1,r4		#any special ^ 0 = NaN
	lda	0xffe00000,r12	#   other let code decide
	cmpo	r12,r4
	ble.f	Ltoi82
	or	r4,g0,r12
	cmpo	r12,0
	bne	Ltoi4

Ltoi82:	subo	1,0,g0		#here it's NaN
	subo	1,0,g1
	b	Ltoi99


	.globl	_dpatn

#  The arctangent function is approximated with the polynomial
#
#     atan(x) = x - x^3/3 + x^5/5 - ...
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
 	.align	8
Latanc:				#coefficients for polynomial
        .word   0xdb6db700,0xf6db6db6   #-1/7 = -0.142857142857143
        .word   0xcccccccc,0x0ccccccc   #1/5 = 0.200000000000000
        .word   0xaaaaaaab,0xeaaaaaaa   #-1/3 = -0.333333333333333
        .word   0x00000000,0x40000000   #1/1 = 1.000000000000000
	.set	Ltc,4

Lattab:				#interpolation table for (2n+1)/64
	.word	0,0,0,0
      	.word   0x00000000,0x3f900000           #0.015625
        .word   0x5bbb729b,0x3f8fff55           #0.01562372862047683
        .word   0x00000000,0x3fa80000           #0.046875
        .word   0x8430da2a,0x3fa7fb81           #0.04684071291596965
        .word   0x00000000,0x3fb40000           #0.078125
        .word   0x0e7c559d,0x3fb3f59f           #0.07796663383154230
        .word   0x00000000,0x3fbc0000           #0.109375
        .word   0xbe6f07c3,0x3fbbe39e           #0.10894195698986579
        .word   0x00000000,0x3fc20000           #0.140625
        .word   0xfb043727,0x3fc1e1fa           #0.13970887428916365
        .word   0x00000000,0x3fc60000           #0.171875
        .word   0x1e3ec26a,0x3fc5c981           #0.17021192528547441
        .word   0x00000000,0x3fca0000           #0.203125
        .word   0xe96c8626,0x3fc9a6a8           #0.20039855382587851
        .word   0x00000000,0x3fce0000           #0.234375
        .word   0xdf205736,0x3fcd77d5           #0.23021958727684372
        .word   0x00000000,0x3fd10000           #0.265625
        .word   0x97d86362,0x3fd09dc5           #0.25962962940825751
        .word   0x00000000,0x3fd30000           #0.296875
        .word   0x2057ef46,0x3fd27837           #0.28858736189407741
        .word   0x00000000,0x3fd50000           #0.328125
        .word   0x36c2af09,0x3fd44aa4           #0.31705575320914697
        .word   0x00000000,0x3fd70000           #0.359375
        .word   0x0309cfe1,0x3fd61484           #0.34500217720710508
        .word   0x00000000,0x3fd90000           #0.390625
        .word   0x4b63b3f7,0x3fd7d560           #0.37239844667675420
        .word   0x00000000,0x3fdb0000           #0.421875
        .word   0x454d6b18,0x3fd98cd5           #0.39922076957525254
        .word   0x00000000,0x3fdd0000           #0.453125
        .word   0x1da65c6c,0x3fdb3a91           #0.42544963737004227
        .word   0x00000000,0x3fdf0000           #0.484375
        .word   0x432c1350,0x3fdcde53           #0.45106965598852344
        .word   0x00000000,0x3fe08000           #0.515625
        .word   0x7f175a34,0x3fde77eb           #0.47606933032276122
        .word   0x00000000,0x3fe18000           #0.546875
        .word   0x73c1a40b,0x3fe0039c           #0.50044081314729405
        .word   0x00000000,0x3fe28000           #0.578125
        .word   0x5b5b43da,0x3fe0c614           #0.52417962878291324
        .word   0x00000000,0x3fe38000           #0.609375
        .word   0x88be7c13,0x3fe1835a           #0.54728438098743692
        .word   0x00000000,0x3fe48000           #0.640625
        .word   0xe2cc9e6a,0x3fe23b71           #0.56975645348297843
        .word   0x00000000,0x3fe58000           #0.671875
        .word   0x8406cbca,0x3fe2ee62           #0.59159971033511138
        .word   0x00000000,0x3fe68000           #0.703125
        .word   0x1cd4171a,0x3fe39c39           #0.61282020216524136
        .word   0x00000000,0x3fe78000           #0.734375
        .word   0x5b795b56,0x3fe44506           #0.63342588296914459
        .word   0x00000000,0x3fe88000           #0.765625
        .word   0x5bb6ec04,0x3fe4e8de           #0.65342634118076193
        .word   0x00000000,0x3fe98000           #0.796875
        .word   0x1f732fbb,0x3fe587d8           #0.67283254759376321
        .word   0x00000000,0x3fea8000           #0.828125
        .word   0x115d7b8e,0x3fe6220d           #0.69165662185319987
        .word   0x00000000,0x3feb8000           #0.859375
        .word   0x920b3d98,0x3fe6b798           #0.70991161846352480
        .word   0x00000000,0x3fec8000           #0.890625
        .word   0x8fba8e0f,0x3fe74897           #0.72761133262651068
        .word   0x00000000,0x3fed8000           #0.921875
        .word   0x289fa093,0x3fe7d528           #0.74477012571607515
        .word   0x00000000,0x3fee8000           #0.953125
        .word   0x576cc2c5,0x3fe85d69           #0.76140276980557842
        .word   0x00000000,0x3fef8000           #0.984375
        .word   0xa99cc05d,0x3fe8e17a           #0.77752431037334768
        .word   0x00000000,0x3fef8000           #0.984375
        .word   0xa99cc05d,0x3fe8e17a           #0.77752431037334768
	.text

_dpatn:
	shlo	1,g1,r4		#check for NaN 0
	lda	0xffe00000,r12
	cmpo	r4,r12
	bg.f	Latn84

	shro	31,g1,r3	#sign bit -> r3
	shlo	31,r3,r3
	clrbit	31,g1,g1

	lda	Lfoneh,r12	#if x > 1 we invert it and set flag
	cmpo	g1,r12
	bl.f	Latn3
	movl	g0,g2
	mov	r12,g1
	mov	0,g0
	call	___divdf3
	setbit	0,r3,r3

Latn3:	movl	g0,r6		#save argument -> r6

	shlo	11,g1,r12	#normalize to get interpolation interval
	setbit	31,r12,r12
	shro	20,g1,r13
	subo	r13,0,r13
	lda	26+0x3fe(r13),r13
	shro	r13,r12,r4
	cmpo	r4,0
	shro	1,r4,r4

	be.f	Latn4		#handle first 1/64 specially so as not
	addo	1,r4,r4		#   to lose precision

Latn4:
	lda	Lattab-(.+8)(ip),r10
	ldl	(r10)[r4*16],r10 #this is C

	movl	r10,g2		#calculate x - C = d
	call	___subdf3
	movl	g0,r8

	movl	r10,g2		#calculate d/(1 + dC + C2)
	movl	r6,g0		#   = d/(1 + C*x)
	call	___muldf3
	lda	Lfoneh,g3
	mov	0,g2
	call	___adddf3
	movl	g0,g2
	movl	r8,g0
	call	___divdf3
	movl	g0,r8

	mov	Ltc-1,r10	#initialize for polynomial loop
	lda	Latanc-(.+8)(ip),r11	#   x^2 = r8  sum = g0
	bal	Lsigmax

	movl	r8,g2		#multiply with x
	call	___muldf3

	lda	Lattab+8-(.+8)(ip),g2
	ldl	(g2)[r4*16],g2 #add atan(C) and atan(d)
	call	___adddf3

	chkbit	0,r3		#if argument was inverted, we return
	bno.t	Latn12		#   pi/2 - y
	movl	g0,g2
	lda	Lfpip2h,g1
	lda	Lfpip2l,g0
	call	___subdf3

Latn12:	clrbit	0,r3,r3		#put in the sign
	or	r3,g1,g1

Latn99:	mov	0,g14
	ret

Latn84:	subo	1,0,g0		#return NaN
	subo	1,0,g1
	b	Latn99


/*  This routine evaluates the polynomial.  To speed up the functions,
    the actual calculations are done in scaled fixed-point format.

    on entry: r8  = x (or x^2 if this is used)
	      r10 = number of terms  
    	      r11 = points to table of constants
    on exit:  g0  = value
*/

Lsigmax:
	shlo	1,r9,r15	#shift value to r15
	shro	21,r15,r15
	lda	0x3fe,r12
	subo	r15,r12,r15

	shlo	11,r9,r13	#mantissa left-justified r12-r13
	shro	21,r8,r12
	or	r12,r13,r13
	shlo	11,r8,r12
	setbit	31,r13,r13

	lda	32,r14		#shift mantissa right by r15
	cmpo	r15,r14
	bl.t	Lsig3
	mov	r13,r12
	mov	0,r13
	subo	r14,r15,r15
Lsig3:	shro	r15,r12,r12
	subo	r15,r14,r14
	shlo	r14,r13,r14
	shro	r15,r13,r13
	or	r14,r12,r12

	shri	31,r9,r6	#sign mask -> r6

	lda	Lfexpcn-(.+8)(ip),r14	#unless exp() take square
	cmpo	r11,r14
	be.f	Lsig9
	emul	r12,r13,r14
	emul	r13,r13,r12
	cmpo	0,1
	addc	r15,r12,r12
	addc	0,r13,r13
	addc	r15,r12,r12
	addc	0,r13,r13
	mov	0,r6

Lsig9:	ldl	(r11),g0	#pick up first constant
	addo	8,r11,r11

Lsig5:	shri	31,g1,r5	#uncomplement sum
	xor	g0,r5,g0
	xor	g1,r5,g1
	cmpo	0,0
	subc	r5,g0,g0
	subc	r5,g1,g1

	emul	g0,r13,r14	#multiply s = x * s
	mov	r15,g0
	emul	g1,r12,r14
	cmpo	0,1
	addc	r15,g0,r15
	emul	g1,r13,g0
	addc	r15,g0,g0
	addc	0,g1,g1

	xor	r6,r5,r15	#recomplement sum
	xor	g0,r15,g0
	xor	g1,r15,g1
	cmpo	0,0
	subc	r15,g0,g0
	subc	r15,g1,g1

	ldl	(r11),r14	#pick up next constant
	addo	8,r11,r11

	cmpo	0,1		#add that to sum
	addc	g0,r14,g0
	addc	g1,r15,g1

	cmpdeco	1,r10,r10	#loop control
	bl.t	Lsig5

	chkbit	30,g1		#normalize mantissa
	teste	r15
	be.t	Lsig11
	addc	g0,g0,g0
	addc	g1,g1,g1
Lsig11:	chkbit	9,g0
	shro	10,g0,g0
	shlo	22,g1,r14
	shro	10,g1,g1
	clrbit	20,g1,g1

	lda	0x3fe(r15),r15	#put exponent in place
	shlo	20,r15,r15
	addc	r14,g0,g0
	addc	r15,g1,g1

	bx	(g14)
