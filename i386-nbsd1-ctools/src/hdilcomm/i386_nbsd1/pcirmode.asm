;/*(cb*/
;/******************************************************************************
;*
;* Copyright (c) 1993, 1994, 1995 Intel Corporation
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
; $Header: /ffs/p1/dev/src/hdilcomm/common/RCS/pcirmode.asm,v 1.1 1995/01/09 21:51:33 cmorgan Exp $

; MODULE
;     pcirmode.asm
;
; PURPOSE
;     Provides 32-bit code for access to PCI functions from a 16-bit,
;     real mode world (e.g., MSC7).

    .MODEL    medium, c
    .386p
PCIRMODE SEGMENT PUBLIC USE16 'CODE'
    assume ds:nothing,cs:PCIRMODE

include pcirmode.inc

inpd    PROC FAR PUBLIC, ioaddr:WORD
    mov    dx,ioaddr
    in     eax,dx
    push   ax
    shr    eax,16
    mov    dx,ax
    pop    ax
    ret

inpd    ENDP

outpd    PROC FAR PUBLIC, ioaddr:WORD, val:DWORD
    mov    dx,ioaddr
    mov    eax,val
    out    dx,eax
    ret

outpd    ENDP

int1a    PROC FAR PUBLIC USES bp, struct1:PTR REGS

    mov    dx, struct1
    mov    bp,dx
    mov    eax,(REGS PTR [bp]).p_eax
    mov    ebx,(REGS PTR [bp]).p_ebx
    mov    ecx,(REGS PTR [bp]).p_ecx
    mov    edx,(REGS PTR [bp]).p_edx
    mov    edi,(REGS PTR [bp]).p_edi
    mov    esi,(REGS PTR [bp]).p_esi
    mov    (REGS PTR [bp]).p_cf,00h

    int    1ah

    jnc    worked
    mov    (REGS PTR [bp]).p_cf,0ffh

worked:
    mov    (REGS PTR [bp]).p_eax,eax
    mov    (REGS PTR [bp]).p_ebx,ebx
    mov    (REGS PTR [bp]).p_ecx,ecx
    mov    (REGS PTR [bp]).p_edx,edx
    mov    (REGS PTR [bp]).p_edi,edi
    mov    (REGS PTR [bp]).p_esi,esi

    ret
    
int1a    ENDP
    END
