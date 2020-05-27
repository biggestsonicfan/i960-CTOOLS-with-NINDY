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
/********************************************************************
  Module Name: EVCAP_M1.S    MEMORY TESTS

  Functions:   _quad_word_test  
               _trip_word_test
               _long_word_test
    
  Functions called with: g0 contains the starting address
                         g1 contains the memory size in bytes

  Functions return:      g0 contains the address of the failing location
                         or zero if the test passes.

    This module contains the procedures necessary to test the
    memory array using STL/LDL, STT/LDT, and STQ/LDQ instructions.
    
    The tests write a walking '1' pattern through each memory
    location according the the size of the data being accessed.
    For instance with the STOL/LDOL instructions, the walking '1'
    is sequenced through every 64 bytes.  The entire array is
    written and then read and verified, creating a delay between
    the two events.
    
    Next, a checkerboard and checkerboard bar pattern is written
    to the array.  For each data type, the checkerboard value is
    written, verified, then checkerboard bar is immediately written
    and verified.
********************************************************************/
        

/*--------------------------------------------------------------------
  Function:  _quad_word_test(u_long *start, u_long size)
  Action:    Verify the memory array using STQ/LDQ instructions.
  Passed:    Starting address of the array and its size.
  Returns:   Address of failing location, or zero if test passes.
--------------------------------------------------------------------*/
        .text
        .align  4
        .globl  _quad_word_test
_quad_word_test:
        shro            4, g1, r7           # divide memory size by 16
        ldconst         0, r5               # r5 will be the index
/*
 * determine which word to write the walking '1' to by taking
 * [index modulus 128], the index beind r5.  Dividing this value by 32
 * yields the word (0, 1, 2, or 3) to write the '1' to.
 */
Q1:     ldconst         0x7F, r6
        and             r5, r6, r3          # index % 128
        ldconst         32, r6
        divo            r6, r3, r4          # (index % 128) / 32
        and             31, r3, r3          # (index % 32) -> bit position for '1'
        cmpobl          0, r4, Q2
        shlo            r3, 1, r8           # word 0
        ldconst         0, r9
        ldconst         0, r10
        ldconst         0, r11
        b               Q5
Q2:     cmpobl          1, r4, Q3
        shlo            r3, 1, r9           # word 1
        ldconst         0, r8
        ldconst         0, r10
        ldconst         0, r11
        b               Q5
Q3:     cmpobl          2, r4, Q4
        shlo            r3, 1, r10          # word 2
        ldconst         0, r8
        ldconst         0, r9
        ldconst         0, r11
        b               Q5
Q4:     shlo            r3, 1, r11          # word 3
        ldconst         0, r8
        ldconst         0, r9
        ldconst         0, r10
Q5:     stq             r8, (g0)[r5 * 16]   # write data
        addo            1, r5, r5           
        cmpobl.t        r5, r7, Q1          # loop until done

        ldconst         0, r5               # use the identical algorithm
Q6:     ldconst         0x7F, r6            # as above to verify the array
        and             r5, r6, r3
        ldconst         32, r6
        divo            r6, r3, r4
        and             31, r3, r3
        cmpobl          0, r4, Q7
        shlo            r3, 1, r8           # word 0
        ldconst         0, r9
        ldconst         0, r10
        ldconst         0, r11
        b               Q10
Q7:     cmpobl          1, r4, Q8
        shlo            r3, 1, r9           # word 1
        ldconst         0, r8
        ldconst         0, r10
        ldconst         0, r11
        b               Q10
Q8:     cmpobl          2, r4, Q9
        shlo            r3, 1, r10          # word 2
        ldconst         0, r8
        ldconst         0, r9
        ldconst         0, r11
        b               Q10
Q9:     shlo            r3, 1, r11          # word 3
        ldconst         0, r8
        ldconst         0, r9
        ldconst         0, r10
Q10:    ldq             (g0)[r5 * 16], r12  # read data previously written
        cmpobne.f       r8, r12, QUAD_ERR   # check all four words
        cmpobne.f       r9, r13, QUAD_ERR
        cmpobne.f       r10, r14, QUAD_ERR
        cmpobne.f       r11, r15, QUAD_ERR
        addo            1, r5, r5
        cmpobl.t        r5, r7, Q6          # loop until done

        ldconst         0, r5               # now do checkerboard pattern
Q11:    ldconst         0x55555555, r8
        mov             r8, r9
        mov             r8, r10
        mov             r8, r11
        stq             r8, (g0)[r5 * 16]   # write data
        ldq             (g0)[r5 * 16], r12  # read data
        cmpobne.f       r8, r12, QUAD_ERR   # verify results
        cmpobne.f       r9, r13, QUAD_ERR
        cmpobne.f       r10, r14, QUAD_ERR
        cmpobne.f       r11, r15, QUAD_ERR
        ldconst         0xAAAAAAAA, r8      # now do checkerboard bar
        mov             r8, r9
        mov             r8, r10
        mov             r8, r11
        stq             r8, (g0)[r5 * 16]   # write data
        ldq             (g0)[r5 * 16], r12  # read data
        cmpobne.f       r8, r12, QUAD_ERR   # verify results
        cmpobne.f       r9, r13, QUAD_ERR
        cmpobne.f       r10, r14, QUAD_ERR
        cmpobne.f       r11, r15, QUAD_ERR
        addo            1, r5, r5
        cmpobl.t        r5, r7, Q11         # loop until done
        ldconst         0, g0               # test successful - return 0
        b               QUAD_DONE
QUAD_ERR:
        shlo            4, r5, r5           # error - return failing address
        addo            r5, g0, g0
QUAD_DONE:
        ret

        
/*--------------------------------------------------------------------
  Function:  _trip_word_test(u_long *start, u_long size)
  Action:    Verify the memory array using STT/LDT instructions.
  Passed:    Starting address of the array and its size.
  Returns:   Address of failing location, or zero if test passes.
--------------------------------------------------------------------*/
        .text
        .align  4
        .globl  _trip_word_test
_trip_word_test:
        shro            4, g1, r7           # divide memory size by 16
        ldconst         0, r5               # r5 will be the index
/*
 * determine which word to write the walking '1' to by taking
 * [index modulus 128], the index beind r5.  Dividing this value by 32
 * yields the word (0, 1, or 2) to write the '1' to.  After the third
 * word is completely written the '1' is walked back to word 0.
 */
T1:     ldconst         0x7F, r6
        and             r5, r6, r3          # index % 128
        ldconst         32, r6
        divo            r6, r3, r4          # (index % 128) / 32
        and             31, r3, r3          # (index % 32) -> bit position for '1'
        cmpobl          0, r4, T2
        shlo            r3, 1, r8           # word 0
        ldconst         0, r9
        ldconst         0, r10
        b               T5
T2:     cmpobl          1, r4, T3
        shlo            r3, 1, r9           # word 1
        ldconst         0, r8
        ldconst         0, r10
        b               T5
T3:     cmpobl          2, r4, T4
        shlo            r3, 1, r10          # word 2
        ldconst         0, r8
        ldconst         0, r9
        b               T5
T4:     shlo            r3, 1, r8           # word 3
        ldconst         0, r9
        ldconst         0, r10
T5:     stt             r8, (g0)[r5 * 16]   # write data
        addo            1, r5, r5
        cmpobl.t        r5, r7, T1          # loop until done

        ldconst         0, r5               # use identical algorithm 
T6:     ldconst         0x7F, r6            # as above to verify the array
        and             r5, r6, r3
        ldconst         32, r6
        divo            r6, r3, r4
        and             31, r3, r3
        cmpobl          0, r4, T7           
        shlo            r3, 1, r8           # word 0
        ldconst         0, r9
        ldconst         0, r10
        b               T10
T7:     cmpobl          1, r4, T8
        shlo            r3, 1, r9           # word 1
        ldconst         0, r8
        ldconst         0, r10
        b               T10
T8:     cmpobl          2, r4, T9
        shlo            r3, 1, r10          # word 2
        ldconst         0, r8
        ldconst         0, r9
        b               T10
T9:     shlo            r3, 1, r8           # word 3
        ldconst         0, r9
        ldconst         0, r10
T10:    ldt             (g0)[r5 * 16], r12  # read data previously written
        cmpobne.f       r8, r12, TRIP_ERR   # check all three words
        cmpobne.f       r9, r13, TRIP_ERR
        cmpobne.f       r10, r14, TRIP_ERR
        addo            1, r5, r5
        cmpobl.t        r5, r7, T6          # loop until done

        ldconst         0, r5               # now do checkerboard pattern
T11:    ldconst         0x55555555, r8
        mov             r8, r9
        mov             r8, r10
        stt             r8, (g0)[r5 * 16]   # write data
        ldt             (g0)[r5 * 16], r12  # read data
        cmpobne.f       r8, r12, TRIP_ERR   # verify results
        cmpobne.f       r9, r13, TRIP_ERR
        cmpobne.f       r10, r14, TRIP_ERR
        ldconst         0xAAAAAAAA, r8      # now do checkerboard bar
        mov             r8, r9
        mov             r8, r10
        stt             r8, (g0)[r5 * 16]   # write data
        ldt             (g0)[r5 * 16], r12  # read data
        cmpobne.f       r8, r12, TRIP_ERR   # verify results
        cmpobne.f       r9, r13, TRIP_ERR
        cmpobne.f       r10, r14, TRIP_ERR
        addo            1, r5, r5
        cmpobl.t        r5, r7, T11         # loop until done
        ldconst         0, g0               # test successful - return 0
        b               TRIP_DONE
TRIP_ERR:
        shlo            4, r5, r5           # error - return failing address
        addo            r5, g0, g0
TRIP_DONE:
        ret

        
/*--------------------------------------------------------------------
  Function:  _long_word_test(u_long *start, u_long size)
  Action:    Verify the memory array using STL/LDL instructions.
  Passed:    Starting address of the array and its size.
  Returns:   Address of failing location, or zero if test passes.
--------------------------------------------------------------------*/
        .text
        .align  4
        .globl  _long_word_test
_long_word_test:
        shro            3, g1, r7           # divide memory size by 8
        ldconst         0, r5               # r5 wil be the index
/*
 * determine which word to write the walking '1' to by taking
 * [index modulus 64], the index beind r5.  Dividing this value by 32
 * yields the word (0 or 1) to write the '1' to.
 */
L1:     ldconst         0x3F, r6
        and             r5, r6, r3          # index % 64
        ldconst         32, r6
        divo            r6, r3, r4          # (index % 64) / 32
        and             31, r3, r3          # (index % 32) -> bit position for '1'
        cmpobl          0, r4, L2
        shlo            r3, 1, r8           # word 0
        ldconst         0, r9
        b               L5
L2:     shlo            r3, 1, r9           # word 1
        ldconst         0, r8
L5:     stl             r8, (g0)[r5 * 8]    # write data
        addo            1, r5, r5
        cmpobl.t        r5, r7, L1          # loop until done

        ldconst         0, r5               # use identical algorithm
L6:     ldconst         0x3F, r6            # as above to verify the array
        and             r5, r6, r3
        ldconst         32, r6
        divo            r6, r3, r4
        and             31, r3, r3
        cmpobl          0, r4, L7
        shlo            r3, 1, r8           # word 0
        ldconst         0, r9
        b               L10
L7:     shlo            r3, 1, r9           # word 1
        ldconst         0, r8
L10:    ldl             (g0)[r5 * 8], r12   # read data previously written
        cmpobne.f       r8, r12, LONG_ERR   # check both words
        cmpobne.f       r9, r13, LONG_ERR
        addo            1, r5, r5
        cmpobl.t        r5, r7, L6          # loop until done

        ldconst         0, r5               # now do checkerboard pattern
L11:    ldconst         0x55555555, r8
        mov             r8, r9
        stl             r8, (g0)[r5 * 8]    # write data
        ldl             (g0)[r5 * 8], r12   # read data
        cmpobne.f       r8, r12, LONG_ERR   # verify results
        cmpobne.f       r9, r13, LONG_ERR
        ldconst         0xAAAAAAAA, r8      # now do checkerboard bar
        mov             r8, r9
        stl             r8, (g0)[r5 * 8]    # write data
        ldl             (g0)[r5 * 8], r12   # read data
        cmpobne.f       r8, r12, LONG_ERR   # verify results
        cmpobne.f       r9, r13, LONG_ERR
        addo            1, r5, r5
        cmpobl.t        r5, r7, L11         # loop until done
        ldconst         0, g0               # test successful - return 0
        b               LONG_DONE
LONG_ERR:
        shlo            3, r5, r5           # error - return failing address
        addo            r5, g0, g0
LONG_DONE:
        ret

