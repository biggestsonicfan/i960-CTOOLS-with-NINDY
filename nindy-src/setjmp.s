##############################################################################
# setjmp/longjmp for GNU cc 
#
# $Id: setjmp.s,v 1.3 90/08/07 09:11:31 chrisb Exp $
#
# Andy Wilson, 28-Sep-89.
##############################################################################

	.text
	.align 4
	.globl _setjmp
_setjmp:
	# On entry, g0 -> buffer in which environment will be saved

	# save global registers
	stq	g0,0(g0)	# save g0..g3
	stq	g4,16(g0)	# save g4..g7
	stq	g8,32(g0)	# save g8..g11
	stt	g12,48(g0)	# save g12..g14
	st	pfp,60(g0)	# Caller's fp is our pfp -- save it in
				#	g15 slot of environment buffer

	# now save caller's local registers
	flushreg
	andnot	0xf,pfp,r3	# r3 = pfp & ~0xf (addr of callers frame)
	ldq	0(r3),r4	# save r0..r3
	stq	r4,64(g0)
	ldq	16(r3),r4	# save r4..r7
	stq	r4,64+16(g0)
	ldq	32(r3),r4	# save r8..r11
	stq	r4,64+32(g0)
	ldq	48(r3),r4	# save r12..r15
	stq	r4,64+48(g0)

	ldconst	0,g0		# return 0
	ret

# longjmp:
#   g0 - address of saved environment
#   g1 - code to return

# [atw] The call to _lj2 is necessary in the following (obscure)
#       case: where longjmp is called from the function where setjmp
#       was called, and the compiler has optimized the call into a
#       branch.  The flushreg instruction does not flush the current
#       frame, and we absolutely must be sure the frame to which we
#       return has been flushed.  Thus, the extra call to be sure.

	.align	4
	.globl	_longjmp
_longjmp:
	call	_lj2		# never returns...

_lj2:
	mov	g1,r15		# r15 = return code
	ld	60(g0),pfp	# pfp = saved frame pointer (from g15 slot)
	andnot	0xf,pfp,g3	# g3 = pfp & ~0xf
				#	(addr. of frame to which we return)
	flushreg
	ldq	64(g0),g4	# restore r0..r3
	stq	g4,0(g3)	#

	mov	g6,rip		# make sure rip is same as saved rip, to cover
				#	960CA A-step bug

	ldq	64+16(g0),g4	# restore r4..r7
	stq	g4,16(g3)
	ldq	64+32(g0),g4	# restore r8..r11
	stq	g4,32(g3)
	ldq	64+48(g0),g4	# restore r12..r15
	stq	g4,48(g3)
	ldq	48(g0),g12	# restore g12..g15
	ldq	32(g0),g8	# restore g8..g11
	ldq	16(g0),g4	# restore g4..g7
	ldq	0(g0),g0	# restore g0..g3
	mov	r15,g0		# set return code
	cmpibne	0,g0,ahead	# (not allowed to be 0 this time)
	mov	1,g0
ahead:
	ret
