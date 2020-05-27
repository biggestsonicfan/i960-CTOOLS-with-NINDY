/*****************************************************************************
 * Copyright (c) 1990, 1991, 1992, 1993 Intel Corporation
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
 ***************************************************************************/

/* A table to help decipher location expressions.  Here is a copy of the
   declaration of struct loc_op, declared in ddmp.h:
struct loc_op
{
    char numops;    /* 0, 1, or 2 
    char sign;      /* 1 == expects signed operand, 0 == unsigned
    char opsize;    /* For constant types (only), size in bytes of operand:
		       0, 1, 2, 4, or 8; 0 == LEB128
};

Note that we're using the DW_OP_* constant as the INDEX into this array;
hence there's no "operator" or "opcode" field.
*/

struct loc_op optable[] =
{
/* numops, sign, opsize */
    		/* index  dwarf2.h name */
{ 0, 0, 0 },	/*   0  undefined */
{ 0, 0, 0 },	/*   1  undefined */
{ 0, 0, 0 },	/*   2  undefined */
{ 1, 0, 4 },	/*   3  DW_OP_addr */
{ 0, 0, 0 },	/*   4  undefined */
{ 0, 0, 0 },	/*   5  undefined */
{ 0, 0, 0 },	/*   6  DW_OP_deref */
{ 0, 0, 0 },	/*   7  undefined */
{ 1, 0, 1 },	/*   8  DW_OP_const1u */
{ 1, 1, 1 },	/*   9  DW_OP_const1s */
{ 1, 0, 2 },	/*  10  DW_OP_const2u */
{ 1, 1, 2 },	/*  11  DW_OP_const2s */
{ 1, 0, 4 },	/*  12  DW_OP_const4u */
{ 1, 1, 4 },	/*  13  DW_OP_const4s */
{ 1, 0, 8 },	/*  14  DW_OP_const8u */
{ 1, 1, 8 },	/*  15  DW_OP_const8s */
{ 1, 0, 0 }, 	/*  16  DW_OP_constu  */
{ 1, 1, 0 },	/*  17  DW_OP_consts  */
{ 0, 0, 0 },	/*  18  DW_OP_dup */
{ 0, 0, 0 },	/*  19  DW_OP_drop */
{ 0, 0, 0 },	/*  20  DW_OP_over */
{ 1, 0, 1 },	/*  21  DW_OP_pick */
{ 0, 0, 0 },	/*  22  DW_OP_swap */
{ 0, 0, 0 },	/*  23  DW_OP_rot */
{ 0, 0, 0 },	/*  24  DW_OP_xderef */
{ 0, 0, 0 },	/*  25  DW_OP_abs */
{ 0, 0, 0 },	/*  26  DW_OP_and */
{ 0, 0, 0 },	/*  27  DW_OP_div */
{ 0, 0, 0 },	/*  28  DW_OP_minus */
{ 0, 0, 0 },	/*  29  DW_OP_mod */
{ 0, 0, 0 },	/*  30  DW_OP_mul */
{ 0, 0, 0 },	/*  31  DW_OP_neg */
{ 0, 0, 0 },	/*  32  DW_OP_not */
{ 0, 0, 0 },	/*  33  DW_OP_or */
{ 0, 0, 0 },	/*  34  DW_OP_plus */
{ 1, 0, 0 },	/*  35  DW_OP_plus_uconst */
{ 0, 0, 0 },	/*  36  DW_OP_shl */
{ 0, 0, 0 },	/*  37  DW_OP_shr */
{ 0, 0, 0 },	/*  38  DW_OP_shra */
{ 0, 0, 0 },	/*  39  DW_OP_xor */
{ 1, 1, 2 },	/*  40  DW_OP_bra */
{ 0, 0, 0 },	/*  41  DW_OP_eq */
{ 0, 0, 0 },	/*  42  DW_OP_ge */
{ 0, 0, 0 },	/*  43  DW_OP_gt */
{ 0, 0, 0 },	/*  44  DW_OP_le */
{ 0, 0, 0 },	/*  45  DW_OP_lt */
{ 0, 0, 0 },	/*  46  DW_OP_ne */
{ 1, 1, 2 },	/*  47  DW_OP_skip */
{ 0, 0, 0 },	/*  48  DW_OP_lit0 */
{ 0, 0, 0 },	/*  49  DW_OP_lit1 */
{ 0, 0, 0 },	/*  50  DW_OP_lit2 */
{ 0, 0, 0 },	/*  51  DW_OP_lit3 */
{ 0, 0, 0 },	/*  52  DW_OP_lit4 */
{ 0, 0, 0 },	/*  53  DW_OP_lit5 */
{ 0, 0, 0 },	/*  54  DW_OP_lit6 */
{ 0, 0, 0 },	/*  55  DW_OP_lit7 */
{ 0, 0, 0 },	/*  56  DW_OP_lit8 */
{ 0, 0, 0 },	/*  57  DW_OP_lit9 */
{ 0, 0, 0 },	/*  58  DW_OP_lit10 */
{ 0, 0, 0 },	/*  59  DW_OP_lit10 */
{ 0, 0, 0 },	/*  60  DW_OP_lit10 */
{ 0, 0, 0 },	/*  61  DW_OP_lit10 */
{ 0, 0, 0 },	/*  62  DW_OP_lit10 */
{ 0, 0, 0 },	/*  63  DW_OP_lit10 */
{ 0, 0, 0 },	/*  64  DW_OP_lit10 */
{ 0, 0, 0 },	/*  65  DW_OP_lit10 */
{ 0, 0, 0 },	/*  66  DW_OP_lit10 */
{ 0, 0, 0 },	/*  67  DW_OP_lit10 */
{ 0, 0, 0 },	/*  68  DW_OP_lit20 */
{ 0, 0, 0 },	/*  69  DW_OP_lit21 */
{ 0, 0, 0 },	/*  70  DW_OP_lit22 */
{ 0, 0, 0 },	/*  71  DW_OP_lit23 */
{ 0, 0, 0 },	/*  72  DW_OP_lit24 */
{ 0, 0, 0 },	/*  73  DW_OP_lit25 */
{ 0, 0, 0 },	/*  74  DW_OP_lit26 */
{ 0, 0, 0 },	/*  75  DW_OP_lit27 */
{ 0, 0, 0 },	/*  76  DW_OP_lit28 */
{ 0, 0, 0 },	/*  77  DW_OP_lit29 */
{ 0, 0, 0 },	/*  78  DW_OP_lit30 */
{ 0, 0, 0 },	/*  79  DW_OP_lit31 */

{ 0, 0, 0 },	/*  80  DW_OP_reg0 */
{ 0, 0, 0 },	/*  81  DW_OP_reg1 */
{ 0, 0, 0 },	/*  82  DW_OP_reg2 */
{ 0, 0, 0 },	/*  83  DW_OP_reg3 */
{ 0, 0, 0 },	/*  84  DW_OP_reg4 */
{ 0, 0, 0 },	/*  85  DW_OP_reg5 */
{ 0, 0, 0 },	/*  86  DW_OP_reg6 */
{ 0, 0, 0 },	/*  87  DW_OP_reg7 */
{ 0, 0, 0 },	/*  88  DW_OP_reg8 */
{ 0, 0, 0 },	/*  89  DW_OP_reg9 */
{ 0, 0, 0 },	/*  90  DW_OP_reg10 */
{ 0, 0, 0 },	/*  91  DW_OP_reg10 */
{ 0, 0, 0 },	/*  92  DW_OP_reg10 */
{ 0, 0, 0 },	/*  93  DW_OP_reg10 */
{ 0, 0, 0 },	/*  94  DW_OP_reg10 */
{ 0, 0, 0 },	/*  95  DW_OP_reg10 */
{ 0, 0, 0 },	/*  96  DW_OP_reg10 */
{ 0, 0, 0 },	/*  97  DW_OP_reg10 */
{ 0, 0, 0 },	/*  98  DW_OP_reg10 */
{ 0, 0, 0 },	/*  99  DW_OP_reg10 */
{ 0, 0, 0 },	/* 100  DW_OP_reg20 */
{ 0, 0, 0 },	/* 101  DW_OP_reg21 */
{ 0, 0, 0 },	/* 102  DW_OP_reg22 */
{ 0, 0, 0 },	/* 103  DW_OP_reg23 */
{ 0, 0, 0 },	/* 104  DW_OP_reg24 */
{ 0, 0, 0 },	/* 105  DW_OP_reg25 */
{ 0, 0, 0 },	/* 106  DW_OP_reg26 */
{ 0, 0, 0 },	/* 107  DW_OP_reg27 */
{ 0, 0, 0 },	/* 108  DW_OP_reg28 */
{ 0, 0, 0 },	/* 109  DW_OP_reg29 */
{ 0, 0, 0 },	/* 110  DW_OP_reg30 */
{ 0, 0, 0 },	/* 111  DW_OP_reg31 */

{ 1, 1, 0 },	/* 112  DW_OP_breg0 */
{ 1, 1, 0 },	/* 113  DW_OP_breg1 */
{ 1, 1, 0 },	/* 114  DW_OP_breg2 */
{ 1, 1, 0 },	/* 115  DW_OP_breg3 */
{ 1, 1, 0 },	/* 116  DW_OP_breg4 */
{ 1, 1, 0 },	/* 117  DW_OP_breg5 */
{ 1, 1, 0 },	/* 118  DW_OP_breg6 */
{ 1, 1, 0 },	/* 119  DW_OP_breg7 */
{ 1, 1, 0 },	/* 120  DW_OP_breg8 */
{ 1, 1, 0 },	/* 121  DW_OP_breg9 */
{ 1, 1, 0 },	/* 122  DW_OP_breg10 */
{ 1, 1, 0 },	/* 123  DW_OP_breg11 */
{ 1, 1, 0 },	/* 124  DW_OP_breg12 */
{ 1, 1, 0 },	/* 125  DW_OP_breg13 */
{ 1, 1, 0 },	/* 126  DW_OP_breg14 */
{ 1, 1, 0 },	/* 127  DW_OP_breg15 */
{ 1, 1, 0 },	/* 128  DW_OP_breg16 */
{ 1, 1, 0 },	/* 129  DW_OP_breg17 */
{ 1, 1, 0 },	/* 130  DW_OP_breg18 */
{ 1, 1, 0 },	/* 131  DW_OP_breg19 */
{ 1, 1, 0 },	/* 132  DW_OP_breg20 */
{ 1, 1, 0 },	/* 133  DW_OP_breg21 */
{ 1, 1, 0 },	/* 134  DW_OP_breg22 */
{ 1, 1, 0 },	/* 135  DW_OP_breg23 */
{ 1, 1, 0 },	/* 136  DW_OP_breg24 */
{ 1, 1, 0 },	/* 137  DW_OP_breg25 */
{ 1, 1, 0 },	/* 138  DW_OP_breg26 */
{ 1, 1, 0 },	/* 139  DW_OP_breg27 */
{ 1, 1, 0 },	/* 140  DW_OP_breg28 */
{ 1, 1, 0 },	/* 141  DW_OP_breg29 */
{ 1, 1, 0 },	/* 142  DW_OP_breg30 */
{ 1, 1, 0 },	/* 143  DW_OP_breg31 */

{ 1, 0, 0 },	/* 144  DW_OP_regx */
{ 1, 1, 0 },	/* 145  DW_OP_fbreg */
{ 2, 0, 0 },	/* 146  DW_OP_bregx */
{ 1, 0, 0 },	/* 147  DW_OP_piece */
{ 1, 0, 1 },	/* 148  DW_OP_deref_size */
{ 1, 0, 1 }, 	/* 149  DW_OP_xderef_size */
{ 0, 0, 0 },	/* 150  DW_OP_nop */

{ 0, 0, 0 },	/* 151  undefined */
{ 0, 0, 0 },	/* 152  undefined */
{ 0, 0, 0 },	/* 153  undefined */
{ 0, 0, 0 },	/* 154  undefined */
{ 0, 0, 0 },	/* 155  undefined */
{ 0, 0, 0 },	/* 156  undefined */
{ 0, 0, 0 },	/* 157  undefined */
{ 0, 0, 0 },	/* 158  undefined */
{ 0, 0, 0 },	/* 159  undefined */
{ 0, 0, 0 },	/* 160  undefined */
{ 0, 0, 0 },	/* 161  undefined */
{ 0, 0, 0 },	/* 162  undefined */
{ 0, 0, 0 },	/* 163  undefined */
{ 0, 0, 0 },	/* 164  undefined */
{ 0, 0, 0 },	/* 165  undefined */
{ 0, 0, 0 },	/* 166  undefined */
{ 0, 0, 0 },	/* 167  undefined */
{ 0, 0, 0 },	/* 168  undefined */
{ 0, 0, 0 },	/* 169  undefined */
{ 0, 0, 0 },	/* 170  undefined */
{ 0, 0, 0 },	/* 171  undefined */
{ 0, 0, 0 },	/* 172  undefined */
{ 0, 0, 0 },	/* 173  undefined */
{ 0, 0, 0 },	/* 174  undefined */
{ 0, 0, 0 },	/* 175  undefined */
{ 0, 0, 0 },	/* 176  undefined */
{ 0, 0, 0 },	/* 177  undefined */
{ 0, 0, 0 },	/* 178  undefined */
{ 0, 0, 0 },	/* 179  undefined */
{ 0, 0, 0 },	/* 180  undefined */
{ 0, 0, 0 },	/* 181  undefined */
{ 0, 0, 0 },	/* 182  undefined */
{ 0, 0, 0 },	/* 183  undefined */
{ 0, 0, 0 },	/* 184  undefined */
{ 0, 0, 0 },	/* 185  undefined */
{ 0, 0, 0 },	/* 186  undefined */
{ 0, 0, 0 },	/* 187  undefined */
{ 0, 0, 0 },	/* 188  undefined */
{ 0, 0, 0 },	/* 189  undefined */
{ 0, 0, 0 },	/* 190  undefined */
{ 0, 0, 0 },	/* 191  undefined */
{ 0, 0, 0 },	/* 192  undefined */
{ 0, 0, 0 },	/* 193  undefined */
{ 0, 0, 0 },	/* 194  undefined */
{ 0, 0, 0 },	/* 195  undefined */
{ 0, 0, 0 },	/* 196  undefined */
{ 0, 0, 0 },	/* 197  undefined */
{ 0, 0, 0 },	/* 198  undefined */
{ 0, 0, 0 },	/* 199  undefined */
{ 0, 0, 0 },	/* 200  undefined */
{ 0, 0, 0 },	/* 201  undefined */
{ 0, 0, 0 },	/* 202  undefined */
{ 0, 0, 0 },	/* 203  undefined */
{ 0, 0, 0 },	/* 204  undefined */
{ 0, 0, 0 },	/* 205  undefined */
{ 0, 0, 0 },	/* 206  undefined */
{ 0, 0, 0 },	/* 207  undefined */
{ 0, 0, 0 },	/* 208  undefined */
{ 0, 0, 0 },	/* 209  undefined */
{ 0, 0, 0 },	/* 210  undefined */
{ 0, 0, 0 },	/* 211  undefined */
{ 0, 0, 0 },	/* 212  undefined */
{ 0, 0, 0 },	/* 213  undefined */
{ 0, 0, 0 },	/* 214  undefined */
{ 0, 0, 0 },	/* 215  undefined */
{ 0, 0, 0 },	/* 216  undefined */
{ 0, 0, 0 },	/* 217  undefined */
{ 0, 0, 0 },	/* 218  undefined */
{ 0, 0, 0 },	/* 219  undefined */
{ 0, 0, 0 },	/* 220  undefined */
{ 0, 0, 0 },	/* 221  undefined */
{ 0, 0, 0 },	/* 222  undefined */
{ 0, 0, 0 },	/* 223  undefined */
{ 0, 0, 0 },	/* 224  undefined */
{ 0, 0, 0 },	/* 225  undefined */
{ 0, 0, 0 },	/* 226  undefined */
{ 0, 0, 0 },	/* 227  undefined */
{ 0, 0, 0 },	/* 228  undefined */
{ 0, 0, 0 },	/* 229  undefined */
{ 0, 0, 0 },	/* 230  undefined */
{ 0, 0, 0 },	/* 231  undefined */
{ 0, 0, 0 },	/* 232  undefined */
{ 0, 0, 0 },	/* 233  undefined */
{ 0, 0, 0 },	/* 234  undefined */
{ 0, 0, 0 },	/* 235  undefined */
{ 0, 0, 0 },	/* 236  undefined */
{ 0, 0, 0 },	/* 237  undefined */
 
{ 0, 0, 0 },	/* 238  User-Defined */
{ 0, 0, 0 },	/* 239  User-Defined */
{ 0, 0, 0 },	/* 240  User-Defined */
{ 0, 0, 0 },	/* 241  User-Defined */
{ 0, 0, 0 },	/* 242  User-Defined */
{ 0, 0, 0 },	/* 243  User-Defined */
{ 0, 0, 0 },	/* 244  User-Defined */
{ 0, 0, 0 },	/* 245  User-Defined */
{ 0, 0, 0 },	/* 246  User-Defined */
{ 0, 0, 0 },	/* 247  User-Defined */
{ 0, 0, 0 },	/* 248  User-Defined */
{ 0, 0, 0 },	/* 249  User-Defined */
{ 0, 0, 0 },	/* 250  User-Defined */
{ 0, 0, 0 },	/* 251  User-Defined */
{ 0, 0, 0 },	/* 252  User-Defined */
{ 0, 0, 0 },	/* 253  User-Defined */
{ 0, 0, 0 },	/* 254  User-Defined */
{ 0, 0, 0 }     /* 255  User-Defined */
};
