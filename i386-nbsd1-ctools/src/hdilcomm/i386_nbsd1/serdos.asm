;/*(cb*/
;/******************************************************************************
;*
;* Copyright (c) 1993, 1994 Intel Corporation
;*
;* Intel hereby grants you permission to copy, modify, and distribute this
;* software and its documentation.  Intel grants this permission provided
;* that the above copyright notice appears in all copies and that both the
;* copyright notice and this permission notice appear in supporting
;* documentation.  In addition, Intel grants this permission provided that
;* you prominently mark as "not part of the original" any modifications
;* made to this software or documentation, and that the name of Intel
;* Corporation not be used in advertising or publicity pertaining to
;* distribution of the software or the documentation without specific,
;* written prior permission.
;*
;* Intel Corporation provides this AS IS, WITHOUT ANY WARRANTY, EXPRESS OR
;* IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY
;* OR FITNESS FOR A PARTICULAR PURPOSE.  Intel makes no guarantee or
;* representations regarding the use of, or the results of the use of,
;* the software and documentation in terms of correctness, accuracy,
;* reliability, currentness, or otherwise; and you rely on the software,
;* documentation and results solely at your own risk.
;*
;* IN NO EVENT SHALL INTEL BE LIABLE FOR ANY LOSS OF USE, LOSS OF BUSINESS,
;* LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES
;* OF ANY KIND.  IN NO EVENT SHALL INTEL'S TOTAL LIABILITY EXCEED THE SUM
;* PAID TO INTEL FOR THE PRODUCT LICENSED HEREUNDER.
;*
;******************************************************************************/
;/*)ce*/
; $Header: /ffs/p1/dev/src/hdilcomm/common/RCS/serdos.asm,v 1.1 1994/04/28 18:17:53 gorman Exp $

NAME	seriala

ARG1	EQU	[bp+6]
ARG2	EQU	[bp+8]

PUBLIC  _INP, _OUTP


;*************************
;       Code Segment
;*************************
seriala_code	SEGMENT	PUBLIC	'CODE'
assume cs:seriala_code

;*********************************
;	INP - input a byte
;	int INP(int port);
;*********************************
_INP	PROC	FAR
	push	bp
	mov	bp,sp

	mov	dx,ARG1
	in	al,dx
	mov	ah,0

	pop	bp
	ret
_INP	ENDP

;*********************************
;	OUTP - output a byte
;	void OUTP(int port_address, int byte)
;
;*********************************
_OUTP	PROC	FAR
	push	bp
	mov	bp,sp

	mov	dx,ARG1
	mov	al,ARG2
	out	dx,al

	pop	bp
	ret
_OUTP	ENDP

seriala_code	ENDS
	END
