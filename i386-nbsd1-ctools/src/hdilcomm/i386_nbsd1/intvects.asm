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
; $Header: /ffs/p1/dev/src/hdilcomm/common/RCS/intvects.asm,v 1.2 1994/12/30 21:54:38 cmorgan Exp $

NAME	intvects

ARG1	EQU	[bp+6]
ARG2	EQU	[bp+8]
DOS	EQU	21H

;*******************************************   
;	low level support for AT/P10 timer stuff
;*******************************************   

PUBLIC  _SET_IV, _GET_IV, _CHANGE_IF
PUBLIC  _serial_intr_wrap
PUBLIC  _timer_intr_wrap

EXTERN	_serial_intr:PROTO
EXTERN	_timer_intr:PROTO


;Calc for how big stack for SERIAL:
;	54 bytes actually used (by serial_intr(), enqueue()).
;	26 bytes needed to store another interrupt's registers.
;	Total is 80.  Be generous to allow for future changes to code w/o worry.
SERIAL_STACK_SIZE	EQU	512

;Calc for how big stack for TIMER:
;	44 bytes actually used (by timer_intr(), inc. PUSHF,CS,IP for chain_intr).
;	26 bytes needed to store another interrupt's registers.
;	Total is 70.  Be generous to allow for future changes to code w/o worry.
TIMER_STACK_SIZE	EQU	512

;***********************************************
;	Data Segment (for Interrupt Handlers)
;***********************************************
intvects_data	SEGMENT	PUBLIC	'DATA'
	serial_ss	WORD	?
	serial_sp	WORD	?
	serial_stack	BYTE	SERIAL_STACK_SIZE DUP (?)
	serial_stack_end	BYTE	?
	timer_ss	WORD	?
	timer_sp	WORD	?
	timer_stack	BYTE	TIMER_STACK_SIZE DUP (?)
	timer_stack_end	BYTE	?
intvects_data	ENDS



;*************************
;	Code Segment
;*************************
_TEXT	SEGMENT	PUBLIC	'CODE'
assume cs:_TEXT

;*********************************
;
; SET_IV(num, addr)
;	set an interrupt vector using DOS request
;	RET	old value (0 if invalid request)
;
;*********************************
_SET_IV	PROC	FAR
	push	bp
	mov	bp,sp
	push	si
	push	di
	push	ds

	mov	ah,25h			; set iv
	mov	al,ARG1
	lds 	dx,DWORD PTR ARG2
	int	DOS

	pop	ds
	pop	di
	pop	si
	mov	sp,bp
	pop	bp
	ret
_SET_IV	ENDP


;*********************************
; GET_IV(num)
;	get an interrupt vector using DOS request
; RET	old value (0 if invalid request)
;
;*********************************
_GET_IV	PROC	FAR
	push	bp
	mov	bp,sp
	push	si
	push	di

	mov	ah,35h			; get iv
	mov	al,ARG1
	int	DOS
	mov	dx,ES			; return in dx:ax
	mov	ax,bx

	pop	di
	pop	si
	mov	sp,bp
	pop	bp
	ret	
_GET_IV	ENDP

;       int     change_if()
;
_CHANGE_IF PROC FAR
        push    bp
        mov     bp,sp

	pushf			; return previous value of if
	pop	ax
	mov	cl,9
	shr	ax,cl
	and	ax,1

	cli
	cmp	WORD PTR ARG1,0
	jz	c1
	sti
c1:

	pop	bp
	ret
_CHANGE_IF	ENDP


; We need a wrapper around the C routine that actually handles the
; interrupt.  Reason: we must switch stacks or else run the risk
; of overflowing the stack of whoever happened to get interrupted
; (say, for instance, DOS).  We go ahead and push the original
; registers on the incoming (old) stack simply because, under DOS,
; everyone's stack is supposed to be able to handle one set of
; registers from an interrupt -- which is exactly what this is.
_serial_intr_wrap PROC FAR

;Switch stacks
	push	ds	;Has to be on old stack
	push	ax	;Has to be on old stack
	push	cx
	push	dx
	push	bx
	;push	sp
	push	bp
	push	si
	push	di
	push	es

	mov	ax, SEG intvects_data
	mov	ds, ax
assume ds:intvects_data
	mov	ax, ss
	mov	WORD PTR serial_ss, ax
	mov	WORD PTR serial_sp, sp
	mov	ax, SEG serial_stack		;set up new stack registers
	mov	ss, ax
	mov	sp, OFFSET serial_stack_end	;stacks grow downwards

;Call C-code handler
	; We pretend it's still an interrupt routine, cuz:
	;	1. We need to get the right DS, and only MSC knows it.
	;	2. Other builds (besides MSC) may not want this wrapper.
	pushf
	call	FAR PTR _serial_intr

;Switch stacks back
	mov	ax, WORD PTR serial_ss
	mov	ss, ax
	mov	sp, WORD PTR serial_sp

;Restore all regs
	pop	es
	pop	di
	pop	si
	pop	bp
	;pop	sp
	pop	bx
	pop	dx
	pop	cx
	pop	ax
	pop	ds
	iret
_serial_intr_wrap ENDP


; We need a wrapper around the C routine that actually handles the
; interrupt.  Reason: we must switch stacks or else run the risk
; of overflowing the stack of whoever happened to get interrupted
; (say, for instance, DOS).  We go ahead and push the original
; registers on the incoming (old) stack simply because, under DOS,
; everyone's stack is supposed to be able to handle one set of
; registers from an interrupt -- which is exactly what this is.
_timer_intr_wrap PROC FAR

;Switch stacks
	push	ds	;Has to be on old stack
	push	ax	;Has to be on old stack
	push	cx
	push	dx
	push	bx
	;push	sp
	push	bp
	push	si
	push	di
	push	es

	mov	ax, SEG intvects_data
	mov	ds, ax
assume ds:intvects_data
	mov	ax, ss
	mov	WORD PTR timer_ss, ax
	mov	WORD PTR timer_sp, sp
	mov	ax, SEG timer_stack		;set up new stack registers
	mov	ss, ax
	mov	sp, OFFSET timer_stack_end	;stacks grow downwards

;Call C-code handler
	; We pretend it's still an interrupt routine, cuz:
	;	1. We need to get the right DS, and only MSC knows it.
	;	2. Other builds (besides MSC) may not want this wrapper.
	pushf
	call	FAR PTR _timer_intr

;Switch stacks back
	mov	ax, WORD PTR timer_ss
	mov	ss, ax
	mov	sp, WORD PTR timer_sp

;Restore all regs
	pop	es
	pop	di
	pop	si
	pop	bp
	;pop	sp
	pop	bx
	pop	dx
	pop	cx
	pop	ax
	pop	ds
	iret
_timer_intr_wrap ENDP


_TEXT	ENDS
	END
