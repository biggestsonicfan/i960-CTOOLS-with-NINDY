/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1993, 1994 Intel Corporation
 *
 * Intel hereby grants you permission to copy, modify, and distribute this
 * software and its documentation.  Intel grants this permission provided
 * that the above copyright notice appears in all copies and that both the
 * copyright notice and this permission notice appear in supporting
 * documentation.  In addition, Intel grants this permission provided that
 * you prominently mark as "not part of the original" any modifications
 * made to this software or documentation, and that the name of Intel
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software or the documentation without specific,
 * written prior permission.
 *
 * Intel Corporation provides this AS IS, WITHOUT ANY WARRANTY, EXPRESS OR
 * IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY
 * OR FITNESS FOR A PARTICULAR PURPOSE.  Intel makes no guarantee or
 * representations regarding the use of, or the results of the use of,
 * the software and documentation in terms of correctness, accuracy,
 * reliability, currentness, or otherwise; and you rely on the software,
 * documentation and results solely at your own risk.
 *
 * IN NO EVENT SHALL INTEL BE LIABLE FOR ANY LOSS OF USE, LOSS OF BUSINESS,
 * LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES
 * OF ANY KIND.  IN NO EVENT SHALL INTEL'S TOTAL LIABILITY EXCEED THE SUM
 * PAID TO INTEL FOR THE PRODUCT LICENSED HEREUNDER.
 *
 ******************************************************************************/
/*)ce*/

    .file    "asm_supp.s"

    .globl    _get_fp
_get_fp:
    andnot    0xf, pfp, g0
    ret

    .globl    _get_ret_ip
_get_ret_ip:
    flushreg
    andnot    0xf, pfp, g2
    ld        (g2), g2
    andnot    0xf, g2, g2
    ld        8(g2), g0
    ret


/*
 * Function:   FAULT_RECORD *i960_get_fault_rec(void)
 *
 * Action:  Called from within a fault handler this function
 *          calculates the address of the fault record, and returns
 *          that address.
 *          WARNING: This function MUST be called from the "base"
 *          level of the fault handler.  It can NOT be nested.
 *          This is because the function relies on the value of
 *          the frame pointer (fp).
 *
 * Returns: pointer to a fault record
 */

    .set FAULT_RECORD_SIZE, 16
    .globl _i960_get_fault_rec
_i960_get_fault_rec:
    andnot    0xf, pfp, g0
    lda       -FAULT_RECORD_SIZE(g0), g0
    ret


/*
 * Function:   INTERRUPT_RECORD *i960_get_int_rec(void)
 *
 * Action:     Gets the current interrupt record from the stack.
 *             Must be called from an interrupt handler.
 *
 *             WARNING: This function MUST be called from the "base"
 *             level of the interrupt handler.  It can NOT be nested.
 *             This is because the function relies on the value of
 *             the frame pointer (fp).
 *
 *  Returns: pointer to an INTERRUPT_RECORD structure
 */

    .globl _i960_get_int_rec
_i960_get_int_rec:
    andnot   0xf, pfp, g0
    lda      -16(g0), g0
    ret


/*
 * Function:   unsigned int i960_modpc(unsigned int mask,unsigned int data)
 *
 * Action:  Modify the arithmetic controls register
 *          This function uses the "modpc" instruction.
 *          if the value of the mask is 0 then the function can be
 *          called from user mode, it returns the current value of the
 *          pc register. If the value of the mask is NOT equal to 0
 *          then it CAN ONLY BE CALLED FROM SUPERVISOR MODE.
 *          Bits that are set in the mask will be modified in the
 *          pc register to match the corosponding bits in the data.
 *
 * Passed:  unsigned int mask - bits that are set in the mask will be modified
 *                       to match the data.
 *          unsigned int data
 *
 * Returns: the value of the pc register before it was modified
 */

    .globl _i960_modpc
_i960_modpc:
    modpc    g0, g0, g1
    mov      g1, g0
    ret

/********************************************************/
/* EAT TIME for UI interface serial download            */
/********************************************************/
    .globl    _eat_time
_eat_time:
    ldconst  0,r4
    mov      r4,r3
    b        L2
L1:
    addi     1,r4,r4
    addi     1,r3,r3
L2:
    muli     20,g0,r15
    cmpibl   r3,r15,L1

    ret    


/********************************************************/
/* Store byte for device memory register IO             */
/*   passed g0 = byte, g1= address                      */
/********************************************************/
    .globl _store_byte
_store_byte:
    stob     g0, (g1)
    ldconst  0, g0
    call     _eat_time
    ret

/********************************************************/
/* Store quick byte for device memory register IO       */
/*   passed g0 = byte, g1= address                      */
/********************************************************/
    .align 4
    .globl _store_quick_byte
_store_quick_byte:
    stob    g0, (g1)
    ret

/********************************************************/
/* Load byte for device memory register IO              */
/*   passed g0 = address ,  returns g0 = byte           */
/********************************************************/
    .globl _load_byte
_load_byte:
    ldob    (g0), g1
    ldconst 0, g0
    call    _eat_time
    mov     g1, g0
    ret


/********************************************************/
/* Load byte for device memory register IO              */
/*   passed g0 = new prioity , returns g0 = old priority*/
/********************************************************/
    .globl _change_priority
_change_priority:
    lda     0x001f0000, r4
    shlo    16, g0, r5
    modpc   r4, r4, r5
    mov     r3,r3 /* Wait for modpc to take affect */
    mov     r3,r3 /* Wait for modpc to take affect */
    mov     r3,r3 /* Wait for modpc to take affect */
    mov     r3,r3 /* Wait for modpc to take affect */
    mov     r5, g0
    ret
