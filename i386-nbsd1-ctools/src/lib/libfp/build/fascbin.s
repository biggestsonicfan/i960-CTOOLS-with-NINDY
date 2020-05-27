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

/*  Routine to convert a an ASCII number to a single-precision floating-point 
    number.
    On entry:  g0 = pointer to ASCII number
               g1 = pointer to byte count
    On exit:   returns single-precision floating-point
		       byte count word contains number of processed characters
*/

	.file	"fascbin.s"
	.link_pix

	.globl	_fascbin

	.text

Lplow:	.word	-1,2684354560,3355443200,4194304000,2621440000,3276800000
	.word	4096000000,2560000000
Lpelow:	.word	0,4,7,10,14,17,20,24
Lphi:	.word	-1,3200000000,2384185791,3552713679,2646977960
Lpehi:	.word	0,27,54,80,107
Lnlow:	.word	-1,3435973837,2748779069,2199023256,3518437209,2814749767
	.word	2251799814,3602879702
Lnelow:	.word	0,-3,-6,-9,-13,-16,-19,-23
Lnhi:	.word	-1,2882303762,3868562623,2596148429,3484491437
Lnehi:	.word	0,-26,-53,-79,-106
	.text

_fascbin:
	mov	g0,r10		#initialize: original pointer
	movl	0,r6		#            work space
	movl	0,r8

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

	mov	r8,r11		#mantissa = (m * 10) + digit
	shlo	2,r8,r8
	addo	r11,r8,r8
	shlo	1,r8,r8
	addo	r4,r8,r8

	andnot	r8,r11,r11	#check for overflow
	cmpi	r11,0
	bl.f	Lfab51

	setbit	6,r7,r7		#set bit "digit present"

	chkbit	2,r7		#if . encountered bump decimal count
	addc	0,r6,r6	
	b	Lfab1

Lfab2a:	mov	r9,r11		#exponent = (e * 10) + digit
	shlo	2,r9,r9
	addo	r11,r9,r9
	shlo	1,r9,r9
	addo	r4,r9,r9

	andnot	r9,r11,r11	#check for overflow
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

	subo	r10,g0,r10	#store character count
	subo	1,r10,r10
	st	r10,(g1)

# number is in, perform some checks and adjustments

	chkbit	6,r7		#check flags for invalid number
	bno.f	Lfab50		#   - no digits
	lda	0x90,r12	#   - E but no exponent digits
	and	r7,r12,r12
	cmpo	0x10,r12
	be.f	Lfab50

	chkbit	5,r7		#negate exponent for - sign
	bno.t	Lfab16		#   then adjust for number of decimals
	subo	r9,0,r9
Lfab16:	subo	r6,r9,r9

	lda	40,r12		#reject some obvious frauds
	subo	r12,0,r13
	subo	1,r13,r13
	cmpi	r12,r9
	concmpi	r9,r13
	ble.f	Lfab51

# perform the conversion to a real floating-point number
# r8 = 10-mantissa, r9 = 10-exponent, bit 1 of r7 = sign

	lda	127,r4		#initialize 2's exponent

	scanbit	r8,r12		#justify mantissa left
	bno.f	Lfab53
	subo	r12,31,r12
	subo	r12,r4,r4
	shlo	r12,r8,r8

	lda	Lplow-(.+8)(ip),g3	#use different table for negative exponent
	cmpi	r9,0
	bge.f	Lfab18
	lda	Lnlow-(.+8)(ip),g3
	subo	r9,0,r9

Lfab18:	and	7,r9,r11	#convert from 10's exponent to 2's exponent
	ld	(g3)[r11*4],r12
	emul	r12,r8,r12
	ld	8*4(g3)[r11*4],r11
	addi	r4,r11,r4
	shro	3,r9,r11
	ld	16*4(g3)[r11*4],r12
	emul	r12,r13,r12
	ld	21*4(g3)[r11*4],r11
	addi	r4,r11,r4

# now we are in base 2, normalize and do the other massage

	scanbit	r13,r12		#normalize the number
	bno.f	Lfab53
	addi	r4,r12,r4
	lda	0xfe,r11
	cmpo	r4,r11
	bg.f	Lfab56
	subo	r12,31,r12
	shlo	r12,r13,r13
	chkbit	7,r13
	shro	8,r13,r13

	clrbit	23,r13,r13	#clear hidden bit

	shlo	23,r4,r4	#or in the exponent
	or	r13,r4,g0
	addc	0,g0,g0

Lfab98:	chkbit	1,r7		#put in the sign finally
	alterbit 31,g0,g0

Lfab99:	ret

Lfab50:	lda	-1,g0		#invalid
	b	Lfab99

Lfab51:	be	Lfab53		#exponent boundary error
	b	Lfab57		#   result 0 or INF

Lfab52:	chkbit	5,r7		#exponent overflow, that's zero if
	bno.f	Lfab57		#   exponent < 0, else INF

Lfab53:	mov	0,g0		#zero
	b	Lfab98

Lfab56:	cmpi	r4,0		#overflow or underflow
	bl	Lfab53

Lfab57:	lda	0x7f800000,g0	#number too big
	b	Lfab98

# handle decimal-point

Lfab4:	chkbit	2,r7		#two decimal-points is an error
	be.f	Lfab50		#   otherwise set bit
	setbit	2,r7,r7
	b	Lfab1

# handle minus-sign

Lfab5:	subo	1,g0,r12	#if first real character, number 
	cmpo	r12,r10		#   is negative
	bne.f	Lfab5a
	setbit	1,r7,r7
	b	Lfab1

Lfab5a:	chkbit	3,r7		#see if previous character was E
	bno.f	Lfab50		#   else this is an invalid '-'
	setbit	5,r7,r7
	b	Lfab1

# handle + sign

Lfab7:	subo	1,g0,r12	#if first real character, number 
	cmpo	r12,r10		#   positive (so what?)
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



/*  Routine to convert a floating-point number to ASCII for display.
    On entry:  g0 = floating-point number
               g1 = pointer to output buffer
               g2 = field size
               g3 = decimal count
    On exit:   result in caller's buffer
*/

	.globl	_fbinasc

	.text
	.align	4
Ltptab:	.word	1,10,100,1000,10000,100000,1000000,10000000
	.word	100000000,1000000000
Ltptabe:
	.set	Ltpmax,10

	.text
	.align	4

_fbinasc:
	lda	0xff000000,r13	#exponent  -> r4 
	shlo	1,g0,r4		#  and jump if number NaN
	cmpo	r13,r4
	ble.f	Lbia80
	shro	24,r4,r4

	lda	' ',r12		#store sign
	cmpi	g0,0
	bge.t	Lbia3
	lda	'-',r12
Lbia3:	stob	r12,(g1)
	addo	1,g1,g1

	lda	150,r12		# 2's exponent for integer mantissa -> r6
	subo	r12,r4,r6

	shlo	8,g0,r4		#mantissa -> r4 right-justified
	shro	8,r4,r4
	setbit	23,r4,r4

	shlo	1,g0,r12	#if argument = 0 clear mantissa
	cmpo	r12,0		#   and exponent
	bne.t	Lbia32
	mov	0,r4
	mov	0,r6

Lbia32:	lda	30103,r12	# 10's exponent for integer mantissa
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
	addi	r12,r7,r13
	cmpi	r13,0
	ble	Lbia6
	subi	r7,0,r12
Lbia6:	addi	r12,r7,r7
	lda	ltptab-(.+8)(ip),r13
	ld	(r13)[r12*4],r13
	emul	r4,r13,r4

	scanbit	r5,r12		#then shift right so fits in one word
	bno.f	Lbia7
	addo	r12,r6,r6
	addo	1,r6,r6
	shro	r12,r4,r4
	chkbit	0,r4
	shro	1,r4,r4
	subo	r12,31,r12
	shlo	r12,r5,r5
	addc	r4,r5,r4

Lbia7:	cmpi	r7,0		#as long as powers of 10 remain, go back
	bl.f	Lbia5
	
	subo	r6,0,r6		#do the remainder of the shifting
	shro	r6,r4,r4

Lbia8:	mov	Ltpmax-1,r15	#number of digits -> r15
Lbib2:	
	lda	Ltptab-(.+8)(ip),r13
	ld	(r13)[r15*4],r13
	subo	1,r15,r15
	cmpo	r13,1
	be.f	Lbib5
	cmpo	r4,r13
	bl.t	Lbib2
Lbib5:	addo	2,r15,r15

	addo	r15,r3,r14	#needed integer positions -> r14
	cmpi	r14,0		#   number of digits + exponent
	bg	Lbib6		#   but at least 1
	mov	1,r14

Lbib6:	addo	1,r14,r13	#total positions needed -> r13
	addo	g3,r13,r13	#   add sign + number of decimals
	cmpo	g3,0
	testg	r12
	addo	r12,r13,r13

	cmpo	r13,g2		#if not enough space, we try an
	bg.f	Lbic3		#   alternate representation

	mov	0,r8		#we have to add trailing zeroes if
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

	cmpo	r13,g3		#if we display all the decimals, skip
	ble	Lbib15		#   truncate
	subo	g3,r13,r13	#   if there are over Ltpmax-1 excess
	cmpo	r13,Ltpmax-1	#   decimals, we have just a zero
	ble.t	Lbib17
	mov	0,r4
	mov	1,r15
	b	Lbib15

Lbib17:
	lda	Ltptab-(.+8)(ip),r12
	ld	(r12)[r13*4],r12 #here we round by adding 10^n/2
	shro	1,r12,r11	#   and truncate by dividing by 10^n
	addo	r11,r4,r4
	divo	r12,r4,r4
	subo	r13,r15,r15

Lbib15:	mov	r4,g0		#convert to decimal digits
	bal	Lintas

Lbib21:	cmpi	r8,0		#add trailing zeroes if needed
	ble	Lbia22
	lda	'0',r12
	stob	r12,(g1)
	addo	1,g1,g1
	subo	1,r8,r8
	b	Lbib21

Lbia22:	mov	0,r13		#store trailing zero
	stob	r13,(g1)
	addo	1,g1,g1

Lbia99:	mov	0,g14		#seems to be required by C
	ret

Lbia10:	addo	1,31,r12	#here shift left mantissa, by 32 bits
	subi	r12,r6,r13	#   if number is large enough, less 
	cmpi	r13,0		#   otherwise
	bge.f	Lbia16
	mov	r6,r12
Lbia16:	subo	r12,r6,r6
	rotate	r12,r4,r11
	shlo	r12,r4,r4
	xor	r11,r4,r5

	mov	9,r12		#then divide by power of 10, 9 maximum
	subo	r12,r7,r13
	cmpi	r13,0
	bge.f	Lbia17
	mov	r7,r12
Lbia17:	subo	r12,r7,r7
	lda	Ltptab-(.+8)(ip),r13
	ld	(r13)[r12*4],r13
	ediv	r13,r4,r4
	mov	r5,r4

	cmpi	r6,0		#continue if more powers of 2
	bg.f	Lbia10

	subo	r7,r3,r3	#cut remaining power of 10 from exponent

	b	Lbia8		#go convert to decimal

# here we don't have space for normal representation, and try
# to use the exponential notation

Lbic3:	cmpo	g2,6		#if space < 6 we show asterisks
	bl.f	Lbid3
	
	addo	r15,r3,r3	#adjust exponent 
	subo	1,r3,r3

	mov	0,r13		#if 6 or 7, no decimals
	cmpo	g2,7
	ble.f	Lbic5

	subo	7,g2,r13	#number of decimals -> r13
	cmpi	r13,7		#   that's as much as space allows,
	ble	Lbic5		#   up to 7
	mov	7,r13

Lbic5:	addo	1,r13,r13	#this is total digits to display

	cmpo	r15,r13		#if we have more numbers than we 
	ble	Lbic15		#   display, we round and truncate
	subo	r13,r15,r14
	lda	Ltptab-(.+8)(ip),r12
	ld	(r12)[r14*4],r12
	shro	1,r12,r11
	addo	r11,r4,r4
	divo	r12,r4,r4
	mov	r13,r15

Lbic15:	mov	r4,g0		#store digits and .
	mov	1,r14
	bal	Lintas

	lda	'E',r12		#store exponent
	stob	r12,(g1)
	addo	1,g1,g1
	lda	'+',r12
	cmpi	r3,0
	bge	Lbia20
	subo	r3,0,r3
	lda	'-',r12
Lbia20:	stob	r12,(g1)
	addo	1,g1,g1
	mov	r3,g0
	mov	2,r15
	mov	r15,r14
	bal	Lintas

	b	Lbia22		#go store terminator

Lbid3:	lda	'*',r12		#store asterisks
	stob	r12,(g1)
	addo	1,g1,g1
	cmpdeco	2,g2,g2
	bl.t	Lbid3
	b	Lbia22

Lbia80:	lda	0x4e614e,r12	#return string "NaN"
	bne	Lbia82
	lda	'+',r12		#store sign for INF
	cmpi	g0,0
	bge.t	Lbia85
	lda	'-',r12
Lbia85:	stob	r12,(g1)
	addo	1,g1,g1
	lda	0x464e49,r12
Lbia82:	stob	r12,(g1)
	addo	1,g1,g1
	cmpo	r12,0
	shro	8,r12,r12
	bne.t	Lbia82
	b	Lbia99

# Subroutine to convert a number to ASCII
# Call with bal
# g0	integer
# g1 	pointer to buffer
# r14	digits before .
# r15	total digits
#       destroys r10-r15

Lintas:
	lda	Ltptab-4-(.+8)(ip),r12
	lda	(r12)[r15*4],r12
	mov	g0,r10
Lias6:	mov	0,r11
	cmpo	r14,0
	bne.t	Lias8
	lda	'.',r13
	stob	r13,(g1)
	addo	1,g1,g1
Lias8:	ld	(r12),r13
	subo	4,r12,r12
	ediv	r13,r10,r10
	lda	'0'(r11),r11
	stob	r11,(g1)
	addo	1,g1,g1
	subo	1,r14,r14
	cmpo	r10,0
	bne.t	Lias13
	cmpi	r14,0
	bl.f	Lias11
Lias13:	cmpo	r13,1
	bg.t	Lias6
Lias11:	bx	(g14)
