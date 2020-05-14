/******************************************************************/
/* 		Copyright (c) 1989, Intel Corporation

   Intel hereby grants you permission to copy, modify, and 
   distribute this software and its documentation.  Intel grants
   this permission provided that the above copyright notice 
   appears in all copies and that both the copyright notice and
   this permission notice appear in supporting documentation.  In
   addition, Intel grants this permission provided that you
   prominently mark as not part of the original any modifications
   made to this software or documentation, and that the name of 
   Intel Corporation not be used in advertising or publicity 
   pertaining to distribution of the software or the documentation 
   without specific, written prior permission.  

   Intel Corporation does not warrant, guarantee or make any 
   representations regarding the use of, or the results of the use
   of, the software and documentation in terms of correctness, 
   accuracy, reliability, currentness, or otherwise; and you rely
   on the software, documentation and results solely at your own 
   risk.							  */
/******************************************************************/


/* Offsets into external '_fp_register_set' array
 * of values of individual floating point registers.
 *
 * UPDATE 'regs.h' IF THESE CHANGE!
 */
	.set	REG_FP0,0
	.set	REG_FP1,8
	.set	REG_FP2,16
	.set	REG_FP3,24


	.globl	_class_f
_class_f:
	classr	g0
	modac	0, r5, r5
	shro	3, r5, r5
	and	r5, 0x07, g0
	ret

	.globl	_class_d
_class_d:
	classrl	g0
	modac	0, r5,	r5
	shro	3, r5,	r5
	and	r5, 0x07, g0
	ret

	.globl	_class_e
_class_e:
	movre	g0, fp0
	classrl	fp0
	modac	0, r5, r5
	shro	3, r5, r5
	and	r5, 0x07, g0
	ret

	.globl	_init_fp
_init_fp:
	# initialize floating point registers to 0 values
	cvtir	0, fp0
	movre	fp0, fp1
	movre 	fp1, fp2
	movre	fp2, fp3
	ret

	.globl	_save_fpr
_save_fpr:
	# Save user floating point registers

	ldconst	_fp_register_set, r5
	movrl 	fp0, r8
	stl	r8, REG_FP0(r5)
	movrl 	fp1, r8
	stl	r8, REG_FP1(r5)
	movrl 	fp2, r8
	stl	r8, REG_FP2(r5)
	movrl 	fp3, r8
	stl	r8, REG_FP3(r5)
	ret

	.globl	_restore_fpr
_restore_fpr:
	# Restore user floating point registers

	ldconst	_fp_register_set, r5
	ldl	REG_FP0(r5), r8
	movrl	r8, fp0
	ldl	REG_FP1(r5), r8
	movrl	r8, fp1
	ldl	REG_FP2(r5), r8
	movrl	r8, fp2
	ldl	REG_FP3(r5), r8
	movrl	r8, fp3
	ret
