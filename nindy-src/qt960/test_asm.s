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
	.globl 	_check_2
	.globl 	_check_3
	.globl 	_check_4
	.globl 	_sram_error
	.globl	_read_int_controller
	.globl	_change_int_controller
	.globl	_change_priority

_check_2:
	ldl	(g0), r8
	cmpibe	r8, g1, ok	/* if data match continue on */
	b	sram_error	/* else error */
ok:	cmpibe r9, g2, noerror	/* if 2nd data match return */
	mov	g2, g1		/* else error with data in g1 */
	b	sram_error

_check_3:
	ldt	(g0), r8
	cmpibe	r8, g1, ok1	/* if data match continue on */
	b	sram_error	/* else error */
ok1:	cmpibe r9, g2, ok2	/* if data match continue on */
	mov	g2, g1		/* else move data to g1 */
	b	sram_error	/* error */
ok2:	cmpibe r10, g3, noerror	/* if data match continue on */
	mov	g3, g1		/* else move data to g1 */
	b	sram_error	/* error */


_check_4:
	ldq	(g0), r8
	cmpibe	r8, g1, ok3	/* if data match continue on */
	b	sram_error	/* else error */
ok3:	cmpibe r9, g2, ok4	/* if data match continue on */
	mov	g2, g1		/* else move data to g1 */
	b	sram_error	/* error */
ok4:	cmpibe r10, g3, ok5	/* if data match continue on */
	mov	g3, g1		/* else move data to g1 */
	b	sram_error	/* error */
ok5:	cmpibe r11, g4, noerror /* if data match continue on */
	mov	g4, g1		/* else move data to g1 */
	b	sram_error	/* error */

noerror:
	ldconst 0, g0
	ret

_sram_error:
sram_error:		/* added for the ic960 assembler's sake */
	ldconst	0x28000000, r8	/* User LED 01 */
	ldconst	0x28000004, r9	/* User LED 23 */
	ldconst	0, r10		/* LED off value */

loop3: 	st	r10, (r8)	/* blink LED's */ 
	st	r10, (r9)	/* blink LED's */
	lda	0xfffff, r12	/* count value */
loop4:	cmpdeco	0, r12, r12	/* loop several times */
	st	g1, (g0)	/* store offending data value */
	bl	loop4		/* go back to loop */

	cmpibe	0, r10, on	/* if LED's off, turn on */
off:	mov	0, r10		/* LED off value */
	b 	loop3		/* go back and blink LED's */
on:	mov	0xf, r10	/* LED on value */
	b 	loop3		/* go back and blink LED's */

_read_int_controller:
	lda	0xff000004, r5
	synld 	r5, g0
	ret

_change_int_controller:
	lda	0xff000004, r5
	synmov 	r5, g0
	ret
	
_change_priority:
	ldconst	0x001f0000, r5
	shlo	16, g0, g0
	modpc	r5, r5, g0
	ret
