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

/*  Routine to convert a an ASCII number to a single-precision floating-point 
    number.
    On entry:  g0 = pointer to ASCII number
               g1 = pointer to byte count
    On exit:   returns double-precision floating-point
	       byte count word contains number of processed characters
*/

	.file	"dascbin.s"
	.globl	_dascbin
	.link_pix

	.text

	.align  8
Lplow:	.word   0xffffffff,0xffffffff,0x00000000,0xA0000000
        .word   0x00000000,0xC8000000,0x00000000,0xFA000000
        .word   0x00000000,0x9C400000,0x00000000,0xC3500000
        .word   0x00000000,0xF4240000,0x00000000,0x98968000
        .word   0x00000000,0xBEBC2000,0x00000000,0xEE6B2800
        .word   0x00000000,0x9502F900,0x00000000,0xBA43B740
        .word   0x00000000,0xE8D4A510,0x00000000,0x9184E72A
        .word   0x80000000,0xB5E620F4,0xA0000000,0xE35FA931
Lpelow:	.word   0,4,7,10,14,17,20,24,27,30,34,37,40,44,47,50
Lphi:	.word   0xffffffff,0xffffffff,0x04000000,0x8E1BC9BF
        .word   0x2B70B59D,0x9DC5ADA8,0x0E4395D6,0xAF298D05
        .word   0xFFCFA6D5,0xC2781F49,0x87DAF7FB,0xD7E77A8F
        .word   0xC59B14A2,0xEFB3AB16,0x9923329E,0x850FADC0
        .word   0x80E98CDF,0x93BA47C9,0xA8D3A6E7,0xA402B9C5
        .word   0x7FE617AA,0xB616A12B,0x859BBF93,0xCA28A291
        .word   0x3927556A,0xE070F78D,0x37826145,0xF92E0C35
        .word   0xE33CC92F,0x8A5296FF,0xD6BF1765,0x9991A6F3
        .word   0x9DF9DE8D,0xAA7EEBFB,0xA79DBC82,0xBD49D14A
        .word   0x5C6A2F8C,0xD226FC19,0x247C83FD,0xE950DF20
Lpehi:	.word   0,54,107,160,213,266,319,373,426,479,532,585,638
        .word   691,745,798,851,904,957,1010
	.align	8
Lnlow:	.word   0xffffffff,0xffffffff,0xCCCCCCCC,0xCCCCCCCC
        .word   0x70A3D70A,0xA3D70A3D,0x8D4FDF3B,0x83126E97
        .word   0xE219652B,0xD1B71758,0x1B478423,0xA7C5AC47
        .word   0xAF6C69B5,0x8637BD05,0xE57A42BC,0xD6BF94D5
        .word   0x8461CEFC,0xABCC7711,0x36B4A597,0x89705F41
        .word   0xBDEDD5BE,0xDBE6FECE,0xCB24AAFE,0xAFEBFF0B
        .word   0x6F5088CB,0x8CBCCC09,0x4BB40E13,0xE12E1342
        .word   0x095CD80F,0xB424DC35,0x3AB0ACD9,0x901D7CF7
Lnelow:	.word   0,-3,-6,-9,-13,-16,-19,-23,-26,-29,-33,-36,-39
        .word   -43,-46,-49
Lnhi:	.word   0xffffffff,0xffffffff,0xC44DE15B,0xE69594BE
        .word   0x453994BA,0xCFB11EAD,0xB17EC159,0xBB127C53
        .word   0xA539E9A5,0xA87FEA27,0x6B0919A5,0x97C560BA
        .word   0xFD75539B,0x88B402F7,0xF065D37D,0xF64335BC
        .word   0x64BCE4A0,0xDDD0467C,0x7C5382C8,0xC7CABA6E
        .word   0xDB73A093,0xB3F4E093,0x38CB002F,0xA21727DB
        .word   0x5423CC06,0x91FF8377,0x3DA4BC60,0x8380DEA9
        .word   0x4A314EBD,0xECE53CEC,0xCF32E1D6,0xD5605FCD
        .word   0x637A1939,0xC0314325,0x5EE43B66,0xAD1C8EAB
        .word   0x836AC577,0x9BECCE62,0xBA0B4925,0x8C71DCD9
Lnehi:	.word   0,-53,-106,-159,-212,-265,-318,-372,-425,-478,-531
        .word   -584,-637,-690,-744,-797,-850,-903,-956,-1009
		.text

_dascbin:
	mov	g0,r3		#initialize: original pointer
	movl	0,r6		#            work space
	movl	0,r8
	movl	0,r10

	lda	'0'-1,g2	#load some constants
	lda	'9',g3

Lfab1:	clrbit	3,r7,r7		#clear bit "E previous"

Lfab1a:	ldob	(g0),r4		#pick up a byte
	addo	1,g0,g0

	cmpo	g3,r4		#pick out numbers
	concmpo	r4,g2
	ble.f	Lfab3

	and	0xf,r4,r4	#mask digit

	chkbit	4,r7		#is this in exponent or mantissa?
	be.f	Lfab2a

	movl	r8,r14		#mantissa = (m * 10) + digit -> r8-r9
	shlo	2,r9,r9
	shro	30,r8,r13
	shlo	2,r8,r8
	or	r9,r13,r9
	cmpo	0,1
	addc	r14,r8,r8
	addc	r15,r9,r9
	addc	r8,r8,r8
	addc	r9,r9,r9
	addc	r4,r8,r8
	addc	0,r9,r9

	andnot	r9,r15,r15	#check for overflow
	cmpi	r15,0
	bl.f	Lfab51

	setbit	6,r7,r7		#set bit "digit present"

	chkbit	2,r7		#if . encountered bump decimal count
	addc	0,r6,r6	
	b	Lfab1

Lfab2a:	mov	r10,r11		#exponent = (e * 10) + digit -> r10
	shlo	2,r10,r10
	addo	r11,r10,r10
	shlo	1,r10,r10
	addo	r4,r10,r10

	andnot	r10,r11,r11	#check for overflow
	cmpi	r11,0
	bl.f	Lfab52

	setbit	7,r7,r7		#set bit "digit present in exponent"
	b	Lfab1

Lfab3:	lda	'.',r11		#check for decimal-point
	cmpo	r4,r11
	be.f	Lfab4

	lda	'-',r11		#check for minus-sign
	cmpo	r4,r11
	be.f	Lfab5

	lda	'+',r11		#check for plus
	cmpo	r4,r11
	be.f	Lfab7

	lda	'E',r11		#check for exponent sign 'E'
	clrbit	5,r4,r12
	cmpo	r12,r11
	be.f	Lfab9
	lda	'D',r11
	cmpo	r12,r11
	be.f	Lfab9

	subo	r3,g0,r3	#store character count
	subo	1,r3,r3
	st	r3,(g1)

# number is in, perform some checks and adjustments

	chkbit	6,r7		#check flags for invalid number
	bno.f	Lfab50		#   - no digits
	lda	0x90,r12	#   - E but no exponent digits
	and	r7,r12,r12
	cmpo	0x10,r12
	be.f	Lfab50

	chkbit	5,r7		#negate exponent for - sign
	bno.t	Lfab16		#   then adjust for number of decimals
	subo	r10,0,r10
Lfab16:	subo	r6,r10,r10

	lda	310,r12		#reject some obvious frauds
	subo	r12,0,r13
	subo	1,r13,r13
	cmpi	r12,r10
	concmpi	r10,r13
	ble.f	Lfab51

# perform the conversion to a real floating-point number
# r8-9 = 10-mantissa, r10 = 10-exponent, bit 1 of r7 = sign

	lda	0x3ff,r4	#initialize 2's exponent

	scanbit	r9,r14		#normalize the number, left-justified
	be.t	Lfab32
	scanbit	r8,r14
	bno.f	Lfab53
	subo	r14,31,r14
	shlo	r14,r8,r9
	mov	0,r8
	lda	32(r14),r14
	b	Lfab37
Lfab32: addo	1,r14,r15
	subo	r14,31,r14
	shro	r15,r8,r15
	shlo	r14,r8,r8
	shlo	r14,r9,r9
	or	r15,r9,r9
Lfab37:	subo	r14,r4,r4
	
	lda	Lplow-(.+8)(ip),r5 #use different table for negative exponent
	cmpi	r10,0
	bge.f	Lfab18
	lda	Lnlow-(.+8)(ip),r5
	subo	r10,0,r10

Lfab18:	and	0xf,r10,r11	#convert from 10's exponent to 2's exponent
	ldl	(r5)[r11*8],r12
	emul	r13,r9,r14
	emul	r12,r9,g2
	cmpo	0,1
	addc	r14,g3,r14
	addc	r15,0,r15
	emul	r13,r8,g2
	addc	r14,g3,r14
	addc	r15,0,r15
	ld	32*4(r5)[r11*4],r11
	addi	r4,r11,r4
	shro	4,r10,r11
	ldl	48*4(r5)[r11*8],r8
	emul	r15,r9,r12
	emul	r14,r9,g2
	cmpo	0,1
	addc	r12,g3,r12
	addc	r13,0,r13
	emul	r15,r8,g2
	addc	r12,g3,r12
	addc	r13,0,r13
	ld	88*4(r5)[r11*4],r11
	addi	r4,r11,r4

# now we are in base 2, normalize and do the other massage

	lda	63(r4),r4	#adjust 2's exponent

Lfab38:	chkbit	31,r13		#normalize the number, left-justified
	be.t	Lfab39		#   it's at most 2 bits out
	addc	r12,r12,r12
	addc	r13,r13,r13
	subo	1,r4,r4
	b	Lfab38

Lfab39:	cmpi	r4,0		#may have underflowed
	bl.f	Lfab53

	mov	r12,r11		#save rounding bit ->  bit 10 in r11

	clrbit	31,r13,r13	#clear hidden bit in mantissa

	shro	11,r12,g0	#right-justify mantissa to g0-g1
	shlo	21,r13,r14
	shro	11,r13,g1
	or	r14,g0,g0

	shlo	20,r4,r4	#combine mantissa and exponent
	or	g1,r4,g1

	chkbit	10,r11		#round
	addc	0,g0,g0
	addc	0,g1,g1

Lfab98:	chkbit	1,r7		#put in the sign finally
	alterbit 31,g1,g1

Lfab99:	ret

Lfab50:	subo	1,0,g0		#invalid
	subo	1,0,g1
	b	Lfab99

Lfab51:	be	Lfab53		#exponent boundary error
	b	Lfab57		#   result 0 or INF

Lfab52:	chkbit	5,r7		#exponent overflow, that's zero if
	bno.f	Lfab57		#   exponent < 0, else INF

Lfab53:	movl	0,g0		#zero
	b	Lfab98

Lfab56:	cmpi	r10,0		#overflow or underflow
	bl	Lfab53

Lfab57:	lda	0x7ff00000,g1	#number too big
	mov	0,g0
	b	Lfab98

# handle decimal-point

Lfab4:	chkbit	2,r7		#two decimal-points is an error
	be.f	Lfab50		#   otherwise set bit
	setbit	2,r7,r7
	b	Lfab1

# handle minus-sign

Lfab5:	subo	1,g0,r12	#if first real character, number 
	cmpo	r12,r3		#   is negative
	bne.f	Lfab5a
	setbit	1,r7,r7
	b	Lfab1

Lfab5a:	chkbit	3,r7		#see if previous character was E
	bno.f	Lfab50		#   else this is an invalid '-'
	setbit	5,r7,r7
	b	Lfab1

# handle + sign

Lfab7:	subo	1,g0,r12	#if first real character, number 
	cmpo	r12,r3		#   positive (so what?)
	be.f	Lfab1		

	chkbit	3,r7		#if next to E good but meaningless
	be.f	Lfab1
	b	Lfab50

# handle exponent-sign E

Lfab9:	chkbit	4,r7		#if second E it's invalid
	be.f	Lfab50

	setbit	4,r7,r7		#set flags
	setbit	3,r7,r7
	b	Lfab1a



/*  Routine to convert a double-precision number to ASCII for display.
    On entry:  g0-1 = floating-point number
               g2   = pointer to output buffer
               g3   = field size
               g4   = decimal count
    On exit:   result in caller's buffer
*/

	.globl	_dbinasc

	.text
	.align	8
Ldptab: .word   0x00000001,0x00000000,0x0000000A,0x00000000
        .word   0x00000064,0x00000000,0x000003E8,0x00000000
        .word   0x00002710,0x00000000,0x000186A0,0x00000000
        .word   0x000F4240,0x00000000,0x00989680,0x00000000
        .word   0x05F5E100,0x00000000,0x3B9ACA00,0x00000000
        .word   0x540BE400,0x00000002,0x4876E800,0x00000017
        .word   0xD4A51000,0x000000E8,0x4E72A000,0x00000918
        .word   0x107A4000,0x00005AF3,0xA4C68000,0x00038D7E
        .word   0x6FC10000,0x002386F2,0x5D8A0000,0x01634578
        .word   0xA7640000,0x0DE0B6B3,0x89E80000,0x8AC72304
	.set	Ldpmax,20

Ltptab:	.word	1,10,100,1000,10000,100000,1000000,10000000
	.word	100000000,1000000000
	.set	Ltpmax,10

	.text
	.align	4

_dbinasc:
	lda	0xffe00000,r12	#exponent  -> r4 
	shlo	1,g1,r4		#  doing this we check for exponent FFE
	cmpo	r12,r4
	ble.f	Lbia80
	shro	21,r4,r4

	lda	' ',r12		#store sign
	cmpi	g1,0
	bge.t	Lbia3
	lda	'-',r12
Lbia3:	stob	r12,(g2)
	addo	1,g2,g2

	lda	0x3ff+52,r12	# 2's exponent for integer mantissa -> r6
	subo	r12,r4,r6

	mov	g0,r4		#mantissa -> r4 right-justified
	shlo	11,g1,r5
	shro	11,r5,r5
	setbit	20,r5,r5

	shlo	1,g1,r12	#if argument = 0 clear mantissa and
	or	g0,r12,r12	#   exponent
	cmpo	r12,0
	bne.t	Lbia4
	movl	0,r4
	mov	0,r6

Lbia4:	lda	30103,r12	# 10's exponent for integer mantissa -> r3
	muli	r12,r6,r7	#   (remember your logarithms?)
	lda	100000,r12
	shro	1,r12,r13
	shri	31,r7,r14
	xor	r14,r13,r13
	addo	r7,r13,r7
	divi	r12,r7,r7
	mov	r7,r3

	cmpi	r6,0		#branch if exponent not negative
	bge.f	Lbia10		#   (if number very large)

Lbia5:	mov	Ltpmax-1,r12	#multiply mantissa by a power of 10	
	addi	r12,r7,r13	#   into r11-r5-r4
	cmpi	r13,0
	ble	Lbia6
	subi	r7,0,r12
Lbia6:	addi	r12,r7,r7
	ld	Ltptab[r12*4],r13
	emul	r5,r13,r10
	emul	r4,r13,r4
	cmpo	0,1
	addc	r10,r5,r5
	addc	0,r11,r11

	scanbit	r11,r12		#then shift right into r5-r4
	bno.f	Lbia7
	addo	r12,r6,r6
	addo	1,r6,r6
	shro	r12,r4,r4
	chkbit	0,r4
	shro	1,r4,r4
	shro	r12,r5,r10
	shro	1,r10,r10
	subo	r12,31,r12
	shlo	r12,r5,r5
	addc	r4,r5,r4
	shlo	r12,r11,r11
	addc	r10,r11,r5

Lbia7:	cmpi	r7,0		#as long as powers of 10 remain, go back
	bl.f	Lbia5
	
	cmpo	r6,0		#if there are 2's powers, 
	be.f	Lbia8		#   do the remainder of the shifting
	lda	32(r6),r12
	subo	r6,0,r6
	subo	1,r6,r15
	chkbit	r15,r4
	shro	r6,r4,r4
	shlo	r12,r5,r12
	shro	r6,r5,r5
	addc	r12,r4,r4

Lbia8:	mov	Ldpmax-1,r15	#number of digits -> r15
Lbib2:	ldl	Ldptab[r15*8],r12
	subo	1,r15,r15
	cmpo	r5,r13
	bl.t	Lbib2
	bg.f	Lbib3
	cmpo	r12,1
	be.f	Lbib3
	cmpo	r4,r12
	bl.t	Lbib2
Lbib3:	addo	2,r15,r15

	addo	r15,r3,r14	#needed integer positions -> r14
	cmpi	r14,0		#   number of digits + exponent
	bg	Lbib6		#   but at least 1
	mov	1,r14

Lbib6:	addo	1,r14,r13	#total positions needed -> r13
	addo	g4,r13,r13	#   add sign + number of decimals
	cmpo	g4,0
	testg	r12
	addo	r12,r13,r13

	cmpo	r13,g3		#if not enough space, we try an
	bg.f	Lbic3		#   alternate representation

	cmpo	g3,18		#adjust requested field size and decimals
	ble.t	Lbib31		#   so we don't display non-significant
	mov	18,g3		#   digits
Lbib31:	subo	r14,16,r12
	cmpo	g4,r12
	ble.t	Lbib32
	mov	r12,g4

Lbib32:	mov	0,r8		#we have to add trailing zeroes if
	cmpo	r14,r15		#   integers exceed total digits		
	ble	Lbib7
	subo	r15,r14,r8

Lbib7:	addo	r15,r3,r12	#if number is < 1 we need to display
	cmpi	r12,0		#   all leading zeroes
	bg	Lbib9
	subo	r3,1,r15

Lbib9:	shri	31,r3,r12	#number of decimals present -> r13
	xor	r3,r12,r13	#   0 if exponent not < 0
	subo	r12,r13,r13	#   -exponent otherwise
	and	r12,r13,r13

	cmpo	r13,g4		#if we display all the decimals, skip
	ble	Lbib15		#   truncate
	subo	g4,r13,r13	#   if there are over Ldpmax-1 excess
	cmpo	r13,Ldpmax-1	#   decimals, we have just a zero
	ble.t	Lbib17
	movl	0,r4
	mov	1,r15
	b	Lbib15

Lbib17:	ldl	Ldptab[r13*8],r10 #here we round by adding 10^n/2
	shro	1,r10,r10
	shlo	31,r11,g0
	shro	1,r11,r11
	or	g0,r10,r10
	cmpo	0,1
	addc	r10,r4,r4
	addc	r11,r5,r5

	cmpo	r13,10		#then truncate by dividing with 10^n
	bl.t	Lbib18
	lda	1000000000,r10
	ediv	r10,r4,r4
	mov	r5,r4
	mov	0,r5
	subo	9,r13,r13
	subo	9,r15,r15
Lbib18:	ld	Ltptab[r13*4],r10
	remo	r10,r5,g1
	divo	r10,r5,r5
	mov	r4,g0
	ediv	r10,g0,g0
	mov	g1,r4
	subo	r13,r15,r15

Lbib15:	movl	r4,g0		#convert to decimal digits
	bal	Lintas

Lbib21:	cmpi	r8,0		#add trailing zeroes if needed
	ble	Lbia22
	lda	'0',r12
	stob	r12,(g2)
	addo	1,g2,g2
	subo	1,r8,r8
	b	Lbib21

Lbia22:	mov	0,r13		#store trailing zero
	stob	r13,(g2)
	addo	1,g2,g2

Lbia99:	mov	0,g14		#seems to be required by C
	ret

Lbia10:	lda	Lnehi-(.+8)(ip),r12	#pick a power of two to multiply
	mov	0,r13
Lbif11:	ld	(r12),g0	
	subo	g0,0,g0
	cmpo	g0,r6
	bg.t	Lbif12
	addo	4,r12,r12
	addo	1,r13,r13
	cmpo	r13,16
	bl.t	Lbif11

Lbif12:	subo	4,r12,r12	#back up to previous element
	subo	1,r13,r13
	ld	(r12),g0
	cmpo	g0,0
	be.f	Lbif13
	
	addo	g0,r6,r6	#adjust 2's power
	shlo	4,r13,r12
	subo	r12,r7,r7	#   and 10's power

	lda	Lnhi-(.+8)(ip),r12
	ldl	(r12)[r13*8],r12	#pick up coefficient and multiply
	emul	r13,r5,r10
	emul	r12,r5,g0
	cmpo	0,1
	addc	r10,g1,r10
	addc	r11,0,r11
	emul	r13,r4,g0
	addc	r10,g1,r4
	addc	r11,0,r5

	lda	53,r12		#may have to repeat
	cmpo	r6,r12
	bge.f	Lbia10

Lbif13:	lda	Lnelow-(.+8)(ip),r12	#pick a power of two to multiply
	mov	0,r13
Lbia11:	ld	(r12),g0	
	subo	g0,0,g0
	cmpo	g0,r6
	bg.t	Lbia12
	addo	4,r12,r12
	addo	1,r13,r13
	cmpo	r13,16
	bl.t	Lbia11

Lbia12:	subo	4,r12,r12	#back up to previous element
	subo	1,r13,r13
	ld	(r12),g0
	cmpo	g0,0
	be.f	Lbia13
	
	addo	g0,r6,r6	#adjust 2's power
	subo	r13,r7,r7	#   and 10's power

	lda	Lnlow-(.+8)(ip),r12
	ldl	(r12)[r13*8],r12	#pick up coefficient and multiply
	emul	r13,r5,r10
	emul	r12,r5,g0
	cmpo	0,1
	addc	r10,g1,r10
	addc	r11,0,r11
	emul	r13,r4,g0
	addc	r10,g1,r4
	addc	r11,0,r5

Lbia13:	shlo	r6,r5,r5
	lda	32,r12
	subo	r6,r12,r12
	shro	r12,r4,r12
	shlo	r6,r4,r4
	or	r12,r5,r5

	subo	r7,r3,r3	#cut remaining power of 10 from exponent

	b	Lbia8		#go convert to decimal

# here we don't have space for normal representation, and try
# to use the exponential notation

Lbic3:	cmpo	g3,6		#if space < 6 we show asterisks
	bl.f	Lbid3
	
	addo	r15,r3,r3	#adjust exponent 
	subo	1,r3,r3

	mov	0,r13		#if 6 or 7, no decimals
	cmpo	g3,7
	ble.f	Lbic5

	subo	7,g3,r13	#number of decimals -> r13
	cmpi	r13,15		#   that's as much as space allows,
	ble	Lbic5		#   up to 15
	mov	15,r13

Lbic5:	addo	1,r13,r13	#this is total digits to display

	cmpo	r15,r13		#if we have more numbers than we 
	ble	Lbic15		#   display, we round and truncate
	subo	r13,r15,r14

	ldl	Ldptab[r14*8],r10 #here we round by adding 10^n/2
	shro	1,r10,r10
	shlo	31,r11,g0
	shro	1,r11,r11
	or	g0,r10,r10
	cmpo	0,1
	addc	r10,r4,r4
	addc	r11,r5,r5

	cmpo	r14,10		#then truncate by dividing with 10^n
	bl.t	Lbic18
	lda	1000000000,r10
	ediv	r10,r4,r4
	mov	r5,r4
	mov	0,r5
	subo	9,r14,r14
	subo	9,r15,r15
Lbic18:	ld	Ltptab[r14*4],r10
	remo	r10,r5,g1
	divo	r10,r5,r5
	mov	r4,g0
	ediv	r10,g0,g0
	mov	g1,r4
	subo	r14,r15,r15

	ldl	Ldptab[r15*8],r12 #adjust number of digits if that
	cmpo	r5,r13		#   changed
	bl.t	Lbic15
	bg.f	Lbic16
	cmpo	r4,r12
	bl.t	Lbic15
Lbic16:	addo	1,r15,r15
	addo	1,r3,r3

Lbic15:	movl	r4,g0		#store digits and .
	mov	1,r14
	bal	Lintas

	lda	'E',r12		#store exponent
	stob	r12,(g2)
	addo	1,g2,g2
	lda	'+',r12
	cmpi	r3,0
	bge	Lbia20
	subo	r3,0,r3
	lda	'-',r12
Lbia20:	stob	r12,(g2)
	addo	1,g2,g2
	mov	r3,g0
	mov	0,g1
	mov	3,r15
	mov	r15,r14
	mov	r15,r13
	bal	Lintas

	b	Lbia22		#go store terminator

Lbid3:	lda	'*',r12		#store asterisks
	stob	r12,(g2)
	addo	1,g2,g2
	cmpdeco	2,g3,g3
	bl.t	Lbid3
	b	Lbia22

Lbia80:	lda	0x4e614e,r12	#return string "NaN"
	bne	Lbia82
	lda	'+',r12		#store sign for INF
	cmpi	g1,0
	bge.t	Lbia85
	lda	'-',r12
Lbia85:	stob	r12,(g2)
	addo	1,g2,g2
	lda	0x464e49,r12
Lbia82:	stob	r12,(g2)
	addo	1,g2,g2
	cmpo	r12,0
	shro	8,r12,r12
	bne.t	Lbia82
	b	Lbia99

# Subroutine to convert a number to ASCII
# Call with bal
# g0	integer
# g2 	pointer to buffer
# r13	number of digits to display
# r14	digits before .
# r15	total digits
#       destroys r10-r15

Lintas:
	lda	Ldptab-8[r15*8],r12
Lias6:	cmpo	r14,0
	bne.t	Lias8
	lda	'.',r15
	stob	r15,(g2)
	addo	1,g2,g2
Lias8:	ldl	(r12),r10
	subo	8,r12,r12
	lda	'0'-1,r15
Lias21:	addo	1,r15,r15
	cmpo	0,0
	subc	r10,g0,g0
	subc	r11,g1,g1
	be.t	Lias21
	addc	r10,g0,g0
	addc	r11,g1,g1
	stob	r15,(g2)
	addo	1,g2,g2
	subo	1,r14,r14
	or	g0,g1,r15
	cmpo	r15,0
	bne.t	Lias13
	cmpi	r14,0
	bl.f	Lias11
Lias13:	cmpo	r10,1
	bg.t	Lias6
Lias11:	bx	(g14)
