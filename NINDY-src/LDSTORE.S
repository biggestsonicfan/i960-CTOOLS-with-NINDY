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
	.globl	_load_byte
	.globl	_load_long
	.globl	_load_triple
	.globl	_load_quad
	.globl	_store_byte
	.globl	_store_quick_byte
	.globl	_store_long
	.globl	_store_triple
	.globl	_store_quad

_load_byte:
	ldob	(g0), r4
	mov	r4, g0
	ret

_load_long:
	ldl	(g0), r4
	stl	r4, (_long_data)
	ret

_load_triple:
	ldt	(g0), r4
	stt	r4, (_long_data)
	ret

_load_quad:
	ldq	(g0), r4
	stq	r4, (_long_data)
	ret

_store_quick_byte:
	stob	g0, (g1)
	ret

_store_byte:
	stob	g0, (g1)
	ldconst 0, g0
	call	_eat_time
	ret

_store_long:
	mov	g0, r4
	mov	g0, r5
	stl	r4, (g1)
	ret

_store_triple:
	mov	g0, r4
	mov	g0, r5
	mov	g0, r6
	stt	r4, (g1)
	ret

_store_quad:
	mov	g0, r4
	mov	g0, r5
	mov	g0, r6
	mov	g0, r7
	stq	r4, (g1)
	ret
