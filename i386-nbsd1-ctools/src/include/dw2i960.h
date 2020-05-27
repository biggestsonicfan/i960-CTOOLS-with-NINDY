/******************************************************************************
 * 
 * Copyright (c) 1995 Intel Corporation
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
 *****************************************************************************/

#ifndef _DW2I960_H
#define _DW2I960_H

/*
 *  This file defines mappings between abstract Dwarf2 objects and
 *  their concrete 80960 counterparts.
 */

/* These map Dwarf 2 location registers to 80960 registers. */
#define DW_OP_i960_pfp	    DW_OP_reg0
#define DW_OP_i960_sp	    DW_OP_reg1
#define DW_OP_i960_rip	    DW_OP_reg2
#define DW_OP_i960_r3	    DW_OP_reg3
#define DW_OP_i960_r4	    DW_OP_reg4
#define DW_OP_i960_r5	    DW_OP_reg5
#define DW_OP_i960_r6	    DW_OP_reg6
#define DW_OP_i960_r7	    DW_OP_reg7
#define DW_OP_i960_r8	    DW_OP_reg8
#define DW_OP_i960_r9	    DW_OP_reg9
#define DW_OP_i960_r10	    DW_OP_reg10
#define DW_OP_i960_r11	    DW_OP_reg11
#define DW_OP_i960_r12	    DW_OP_reg12
#define DW_OP_i960_r13	    DW_OP_reg13
#define DW_OP_i960_r14	    DW_OP_reg14
#define DW_OP_i960_r15	    DW_OP_reg15
#define DW_OP_i960_g0	    DW_OP_reg16
#define DW_OP_i960_g1	    DW_OP_reg17
#define DW_OP_i960_g2	    DW_OP_reg18
#define DW_OP_i960_g3	    DW_OP_reg19
#define DW_OP_i960_g4	    DW_OP_reg20
#define DW_OP_i960_g5	    DW_OP_reg21
#define DW_OP_i960_g6	    DW_OP_reg22
#define DW_OP_i960_g7	    DW_OP_reg23
#define DW_OP_i960_g8	    DW_OP_reg24
#define DW_OP_i960_g9	    DW_OP_reg25
#define DW_OP_i960_g10	    DW_OP_reg26
#define DW_OP_i960_g11	    DW_OP_reg27
#define DW_OP_i960_g12	    DW_OP_reg28
#define DW_OP_i960_g13	    DW_OP_reg29
#define DW_OP_i960_g14	    DW_OP_reg30
#define DW_OP_i960_fp	    DW_OP_reg31

/* These map the first operand of a DW_OP_regx or DW_OP_bregx location
   expression opcode to an 80960 register.
 */
#define DW_OP_regx_i960_fp0	0x0
#define DW_OP_regx_i960_fp1	0x1
#define DW_OP_regx_i960_fp2	0x2
#define DW_OP_regx_i960_fp3	0x3
#define DW_OP_regx_i960_sf0	0x4
#define DW_OP_regx_i960_sf1	0x5
#define DW_OP_regx_i960_sf2	0x6
#define DW_OP_regx_i960_sf3	0x7
#define DW_OP_regx_i960_sf4	0x8

/* DW_OP_regx_i960_pic_bias is a "virtual" register, that identifies
   the Position Independent Code (PIC) bias.  Its value, which isn't known
   until load-time, is the difference between the link-time and load-time
   address of the code section.  A typical use of this register, to
   identify the location of a PIC object such as "const int xyz",
   would be as follows:

	DW_OP_addr _xyz  DW_OP_bregx 0x10 0  DW_OP_plus
 */
#define DW_OP_regx_i960_pic_bias	0x10

/* These map Dwarf 2 call frame registers to 80960 registers. */
#define DW_CFA_i960_pfp	    DW_CFA_R0
#define DW_CFA_i960_sp	    DW_CFA_R1
#define DW_CFA_i960_rip	    DW_CFA_R2
#define DW_CFA_i960_r3	    DW_CFA_R3
#define DW_CFA_i960_r4	    DW_CFA_R4
#define DW_CFA_i960_r5	    DW_CFA_R5
#define DW_CFA_i960_r6	    DW_CFA_R6
#define DW_CFA_i960_r7	    DW_CFA_R7
#define DW_CFA_i960_r8	    DW_CFA_R8
#define DW_CFA_i960_r9	    DW_CFA_R9
#define DW_CFA_i960_r10	    DW_CFA_R10
#define DW_CFA_i960_r11	    DW_CFA_R11
#define DW_CFA_i960_r12	    DW_CFA_R12
#define DW_CFA_i960_r13	    DW_CFA_R13
#define DW_CFA_i960_r14	    DW_CFA_R14
#define DW_CFA_i960_r15	    DW_CFA_R15
#define DW_CFA_i960_g0	    DW_CFA_R16
#define DW_CFA_i960_g1	    DW_CFA_R17
#define DW_CFA_i960_g2	    DW_CFA_R18
#define DW_CFA_i960_g3	    DW_CFA_R19
#define DW_CFA_i960_g4	    DW_CFA_R20
#define DW_CFA_i960_g5	    DW_CFA_R21
#define DW_CFA_i960_g6	    DW_CFA_R22
#define DW_CFA_i960_g7	    DW_CFA_R23
#define DW_CFA_i960_g8	    DW_CFA_R24
#define DW_CFA_i960_g9	    DW_CFA_R25
#define DW_CFA_i960_g10	    DW_CFA_R26
#define DW_CFA_i960_g11	    DW_CFA_R27
#define DW_CFA_i960_g12	    DW_CFA_R28
#define DW_CFA_i960_g13	    DW_CFA_R29
#define DW_CFA_i960_g14	    DW_CFA_R30
#define DW_CFA_i960_fp	    DW_CFA_R31
#define DW_CFA_i960_fp0	    DW_CFA_R32
#define DW_CFA_i960_fp1	    DW_CFA_R33
#define DW_CFA_i960_fp2	    DW_CFA_R34
#define DW_CFA_i960_fp3	    DW_CFA_R35

/* Define the CIE augmentation string employed for the 80960 ABI. */
#define DW_CFA_i960_ABI_augmentation	"80960 ABI"

/* The DW_CFA_i960_pfp_offset rule is an 80960 ABI extension.  It is used
   to specify that a register is preserved at some offset from the previous
   frame pointer.  It takes two unsigned LEB128 arguments.  The first indicates
   the register that is being saved.  The second is a factored offset.
   This rule indicates that the given register is saved at
   offset (data_alignment_factor * factored offset) from the current pfp.
 */
#define DW_CFA_i960_pfp_offset	DW_CFA_lo_user

#endif /* _DW2I960 */



