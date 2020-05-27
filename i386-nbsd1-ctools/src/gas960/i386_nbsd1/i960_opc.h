/* i960_opc.h - 80960 Opcodes
   Copyright (C) 1987, 1990, 1991 Free Software Foundation, Inc.

This file is part of GAS, the GNU Assembler.

GAS is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GAS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GAS; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Basic 80960 instruction formats.
 *
 * The 'COJ' instructions are actually COBR instructions with the 'b' in
 * the mnemonic replaced by a 'j';  they are ALWAYS "de-optimized" if necessary:
 * if the displacement will not fit in 13 bits, the assembler will replace them
 * with the corresponding compare and branch instructions.
 *
 * All of the 'MEMn' instructions are the same format; the 'n' in the name
 * indicates the default index scale factor (the size of the datum operated on).
 *
 * The FBRA formats are not actually an instruction format.  They are the
 * "convenience directives" for branching on floating-point comparisons,
 * each of which generates 2 instructions (a 'bno' and one other branch).
 *
 * The CALLJ format is not actually an instruction format.  It indicates that
 * the instruction generated (a CTRL-format 'call') should have its relocation
 * specially flagged for link-time replacement with a 'bal' or 'calls' if
 * appropriate.
 */ 

/* $Id: i960_opc.h,v 1.27 1995/11/22 10:52:08 peters Exp $ */

#define CTRL	0
#define COBR	1
#define COJ	2
#define REG	3
#define MEM1	4
#define MEM2	5
#define MEM4	6
#define MEM8	7
#define MEM12	8
#define MEM16	9
#define FBRA	10
#define CALLJ	11
#define CALLJX  12
#define COPR    13

/* Masks for the mode bits in REG format instructions */
#define M1		0x0800
#define M2		0x1000
#define M3		0x2000

/* maximum number of operands any instruction may have. */
#define MAX_OPS		4

/* Generate the 12-bit opcode for a REG format instruction by placing the 
 * high 8 bits in instruction bits 24-31, the low 4 bits in instruction bits
 * 7-10.
 */

#define REG_OPC(opc)	((opc & 0xff0) << 20) | ((opc & 0xf) << 7)

/* Generate a template for a REG format instruction:  place the opcode bits
 * in the appropriate fields and OR in mode bits for the operands that will not
 * be used.  I.e.,
 *		set m1=1, if src1 will not be used
 *		set m2=1, if src2 will not be used
 *		set m3=1, if dst  will not be used
 *
 * Setting the "unused" mode bits to 1 speeds up instruction execution(!).
 * The information is also useful to us because some 1-operand REG instructions
 * use the src1 field, others the dst field; and some 2-operand REG instructions
 * use src1/src2, others src1/dst.  The set mode bits enable us to distinguish.
 */
#define R_0(opc)	( REG_OPC(opc) | M1 | M2 | M3 )	/* No operands      */
#define R_1(opc)	( REG_OPC(opc) | M2 | M3 )	/* 1 operand: src1  */
#define R_1D(opc)	( REG_OPC(opc) | M1 | M2 )	/* 1 operand: dst   */
#define R_2(opc)	( REG_OPC(opc) | M3 )		/* 2 ops: src1/src2 */
#define R_2D(opc)	( REG_OPC(opc) | M2 )		/* 2 ops: src1/dst  */
#define R_3(opc)	( REG_OPC(opc) )		/* 3 operands       */

/* DESCRIPTOR BYTES FOR REGISTER OPERANDS
 *
 * Interpret names as follows:
 *	R:   global or local register only
 *	RS:  global, local, or (if target allows) special-function register only
 *	RL:  global or local register, or integer literal
 *	RSL: global, local, or (if target allows) special-function register;
 *		or integer literal
 *	F:   global, local, or floating-point register
 *	FL:  global, local, or floating-point register; or literal (including
 *		floating point)
 *
 * A number appended to a name indicates that registers must be aligned,
 * as follows:
 *	2: register number must be multiple of 2
 *	4: register number must be multiple of 4
 */

#define OP_RG		0x04		/* Mask for reg-OK bit */
#define OP_LIT		0x08		/* Mask for the "literal-OK" bit */
#define OP_SFR		0x10		/* Mask for the "sfr-OK" bit */
#define OP_FP		0x20		/* Mask for "floating-point-OK" bit */
#define OP_LIT12	0x40		/* Mask for 8 bit literal OK bit */

/* This macro ors the bits together.  Note that 'align' is a mask
 * for the low 0, 1, or 2 bits of the register number, as appropriate.
 */
#define OP(reg,align,lit,fp,sfr)	( reg | align | lit | fp | sfr )

#define R	OP( OP_RG, 0, 0,        0,     0      )
#define RS	OP( OP_RG, 0, 0,        0,     OP_SFR )
#define RL	OP( OP_RG, 0, OP_LIT,   0,     0      )
#define RSL	OP( OP_RG, 0, OP_LIT,   0,     OP_SFR )
#define F	OP( OP_RG, 0, 0,        OP_FP, 0      )
#define FL	OP( OP_RG, 0, OP_LIT,   OP_FP, 0      )
#define R2	OP( OP_RG, 1, 0,        0,     0      )
#define RS2	OP( OP_RG, 1, 0,        0,     OP_SFR )
#define RL2	OP( OP_RG, 1, OP_LIT,   0,     0      )
#define RSL2	OP( OP_RG, 1, OP_LIT,   0,     OP_SFR )
#define F2	OP( OP_RG, 1, 0,        OP_FP, 0      )
#define FL2	OP( OP_RG, 1, OP_LIT,   OP_FP, 0      )
#define R4	OP( OP_RG, 3, 0,        0,     0      )
#define RS4	OP( OP_RG, 3, 0,        0,     OP_SFR )
#define RL4	OP( OP_RG, 3, OP_LIT,   0,     0      )
#define RSL4	OP( OP_RG, 3, OP_LIT,   0,     OP_SFR )
#define F4	OP( OP_RG, 3, 0,        OP_FP, 0      )
#define FL4	OP( OP_RG, 3, OP_LIT,   OP_FP, 0      )
#define L12	OP( 0,     0, OP_LIT12, 0,     0      )
#define L	OP( 0,     0, OP_LIT,   0,     0      )

#define M	0x7f	/* Memory operand (MEMA & MEMB format instructions) */

/* Macros to extract info from the register operand descriptor byte 'od'.
 */
#define REG_OK(od)      (od & OP_RG)      /* TRUE if REG operand allowed */
#define SFR_OK(od)	(od & OP_SFR)	/* TRUE if sfr operand allowed */
#define LIT_OK(od)	(od & OP_LIT)	/* TRUE if literal operand allowed */
#define FP_OK(od)	(od & OP_FP)	/* TRUE if floating-point op allowed */
#define LIT12_OK(od)	(od & OP_LIT12)	/* TRUE if 8 bit literal op allowed */
#define REG_ALIGN(od,n)	((od & 0x3 & n) == 0)
					/* TRUE if reg #n is properly aligned */
#define MEMOP(od)	(od == M)	/* TRUE if operand is a memory operand*/

/* Description of a single i80960 instruction */
struct i960_opcode {
	long opcode;	/* 32 bits, constant fields filled in, rest zeroed */
	char *name;	/* Assembler mnemonic				   */
	short iclass;	/* Class: see #defines in tc_i960.h		   */
	short arch;	/* Architectures that support this instruction	   */
	char format;	/* REG, COBR, CTRL, MEMn, COJ, FBRA, CALLJ, COPR   */
	char num_ops;	/* Number of operands				   */
        		/* Operand descriptors; same order as assembler instr */
	char operand[MAX_OPS];
};

/* iclass: instruction class; legal values are defined in tc_i960.h.
 * These define the hierarchy of architectures as much as possible in the
 * 80960 instruction set.  The purpose is to predict compatible linkage.
 *
 *      I_CORE1		Original 80960 base instruction set	
 *	I_CORE2		New core instructions added when JX and HX added
 *      I_CX		80960Cx instruction		
 *      I_DEC		Decimal instruction		
 *      I_FP		Floating point instruction	
 *      I_KX		80960Kx instruction		
 *      I_CASIM		CA simulator instruction	
 *      I_JX		80960Jx instruction
 *	I_HX		80960Hx instruction
 */

/******************************************************************************
 *
 *		TABLE OF i960 INSTRUCTION DESCRIPTIONS
 *
 ******************************************************************************/

struct i960_opcode i960_opcodes[] = {

    /* if a CTRL instruction has an operand, it's always a displacement */

{ 0x09000000, "callj",  I_CORE1, A_CORE1, CALLJ, 1 },
{ 0x86000000, "calljx",	I_CORE1, A_CORE1, CALLJX, 1, M },
	
{ 0x11000000, "bg",	I_CORE1, A_CORE1, CTRL,  1 },
{ 0x12000000, "be",	I_CORE1, A_CORE1, CTRL,  1 },
{ 0x13000000, "bge",	I_CORE1, A_CORE1, CTRL,  1 },
{ 0x14000000, "bl",	I_CORE1, A_CORE1, CTRL,  1 },
{ 0x15000000, "bne",	I_CORE1, A_CORE1, CTRL,  1 },
{ 0x16000000, "ble",	I_CORE1, A_CORE1, CTRL,  1 },
{ 0x17000000, "bo",	I_CORE1, A_CORE1, CTRL,  1 },
{ 0x10000000, "bno",	I_CORE1, A_CORE1, CTRL,  1 },
{ 0x08000000, "b",	I_CORE1, A_CORE1, CTRL,  1 },
{ 0x09000000, "call",	I_CORE1, A_CORE1, CTRL,  1 },
{ 0x0a000000, "ret",	I_CORE1, A_CORE1, CTRL,  0 },
{ 0x0b000000, "bal",	I_CORE1, A_CORE1, CTRL,  1 },
{ 0x10000000, "bf",	I_CORE1, A_CORE1, CTRL,  1 }, /* same as bno */
{ 0x10000000, "bru",	I_CORE1, A_CORE1, CTRL,  1 }, /* same as bno */
{ 0x11000000, "brg",	I_CORE1, A_CORE1, CTRL,  1 }, /* same as bg */
{ 0x12000000, "bre",	I_CORE1, A_CORE1, CTRL,  1 }, /* same as be */
{ 0x13000000, "brge",	I_CORE1, A_CORE1, CTRL,  1 }, /* same as bge */
{ 0x14000000, "brl",	I_CORE1, A_CORE1, CTRL,  1 }, /* same as bl */
{ 0x15000000, "brlg",	I_CORE1, A_CORE1, CTRL,  1 }, /* same as bne */
{ 0x16000000, "brle",	I_CORE1, A_CORE1, CTRL,  1 }, /* same as ble */
{ 0x17000000, "bt",	I_CORE1, A_CORE1, CTRL,  1 }, /* same as bo */
{ 0x17000000, "bro",	I_CORE1, A_CORE1, CTRL,  1 }, /* same as bo */
{ 0x18000000, "faultno",I_CORE1, A_CORE1, CTRL,	 0 },
{ 0x18000000, "faultf",	I_CORE1, A_CORE1, CTRL,	 0 }, /*same as faultno*/
{ 0x19000000, "faultg",	I_CORE1, A_CORE1, CTRL,  0 },
{ 0x1a000000, "faulte",	I_CORE1, A_CORE1, CTRL,  0 },
{ 0x1b000000, "faultge",I_CORE1, A_CORE1, CTRL,  0 },
{ 0x1c000000, "faultl",	I_CORE1, A_CORE1, CTRL,  0 },
{ 0x1d000000, "faultne",I_CORE1, A_CORE1, CTRL,  0 },
{ 0x1e000000, "faultle",I_CORE1, A_CORE1, CTRL,  0 },
{ 0x1f000000, "faulto",	I_CORE1, A_CORE1, CTRL,  0 },
{ 0x1f000000, "faultt",	I_CORE1, A_CORE1, CTRL,	 0 }, /* syn for faulto */
	
{ 0x01000000, "syscall",I_CASIM, A_CX, CTRL,  0 },
	
/* If a COBR (or COJ) has 3 operands, the last one is always a
 * displacement and does not appear explicitly in the table.
 */
	
{ 0x20000000, "testno",	I_CORE1, A_CORE1, COBR, 1, R },
{ 0x21000000, "testg",	I_CORE1, A_CORE1, COBR, 1, R },
{ 0x22000000, "teste",	I_CORE1, A_CORE1, COBR, 1, R },
{ 0x23000000, "testge",	I_CORE1, A_CORE1, COBR, 1, R },
{ 0x24000000, "testl",	I_CORE1, A_CORE1, COBR, 1, R },
{ 0x25000000, "testne",	I_CORE1, A_CORE1, COBR, 1, R },
{ 0x26000000, "testle",	I_CORE1, A_CORE1, COBR, 1, R },
{ 0x27000000, "testo",	I_CORE1, A_CORE1, COBR, 1, R },
{ 0x30000000, "bbc", 	I_CORE1, A_CORE1, COBR, 3, RL, RS },
{ 0x31000000, "cmpobg",	I_CORE1, A_CORE1, COBR, 3, RL, RS },
{ 0x32000000, "cmpobe",	I_CORE1, A_CORE1, COBR, 3, RL, RS },
{ 0x33000000, "cmpobge",I_CORE1, A_CORE1, COBR, 3, RL, RS },
{ 0x34000000, "cmpobl",	I_CORE1, A_CORE1, COBR, 3, RL, RS },
{ 0x35000000, "cmpobne",I_CORE1, A_CORE1, COBR, 3, RL, RS },
{ 0x36000000, "cmpoble",I_CORE1, A_CORE1, COBR, 3, RL, RS },
{ 0x37000000, "bbs", 	I_CORE1, A_CORE1, COBR, 3, RL, RS },
{ 0x38000000, "cmpibno",I_CORE1, A_CORE1, COBR, 3, RL, RS },
{ 0x39000000, "cmpibg",	I_CORE1, A_CORE1, COBR, 3, RL, RS },
{ 0x3a000000, "cmpibe",	I_CORE1, A_CORE1, COBR, 3, RL, RS },
{ 0x3b000000, "cmpibge",I_CORE1, A_CORE1, COBR, 3, RL, RS },
{ 0x3c000000, "cmpibl", I_CORE1, A_CORE1, COBR, 3, RL, RS },
{ 0x3d000000, "cmpibne",I_CORE1, A_CORE1, COBR, 3, RL, RS },
{ 0x3e000000, "cmpible",I_CORE1, A_CORE1, COBR, 3, RL, RS },
{ 0x3f000000, "cmpibo", I_CORE1, A_CORE1, COBR, 3, RL, RS },
{ 0x31000000, "cmpojg", I_CORE1, A_CORE1, COJ, 3, RL, RS },
{ 0x32000000, "cmpoje", I_CORE1, A_CORE1, COJ, 3, RL, RS },
{ 0x33000000, "cmpojge",I_CORE1, A_CORE1, COJ, 3, RL, RS },
{ 0x34000000, "cmpojl", I_CORE1, A_CORE1, COJ, 3, RL, RS },
{ 0x35000000, "cmpojne",I_CORE1, A_CORE1, COJ, 3, RL, RS },
{ 0x36000000, "cmpojle",I_CORE1, A_CORE1, COJ, 3, RL, RS },
{ 0x38000000, "cmpijno",I_CORE1, A_CORE1, COJ, 3, RL, RS },
{ 0x39000000, "cmpijg", I_CORE1, A_CORE1, COJ, 3, RL, RS },
{ 0x3a000000, "cmpije", I_CORE1, A_CORE1, COJ, 3, RL, RS },
{ 0x3b000000, "cmpijge",I_CORE1, A_CORE1, COJ, 3, RL, RS },
{ 0x3c000000, "cmpijl", I_CORE1, A_CORE1, COJ, 3, RL, RS },
{ 0x3d000000, "cmpijne",I_CORE1, A_CORE1, COJ, 3, RL, RS },
{ 0x3e000000, "cmpijle",I_CORE1, A_CORE1, COJ, 3, RL, RS },
{ 0x3f000000, "cmpijo", I_CORE1, A_CORE1, COJ, 3, RL, RS },
{ 0x80000000, "ldob",   I_CORE1, A_CORE1, MEM1, 2, M,  R },
{ 0x82000000, "stob",   I_CORE1, A_CORE1, MEM1, 2, R , M },
{ 0x84000000, "bx",	I_CORE1, A_CORE1, MEM1, 1, M },
{ 0x85000000, "balx",	I_CORE1, A_CORE1, MEM1, 2, M,  R },
{ 0x86000000, "callx",	I_CORE1, A_CORE1, MEM1, 1, M },
{ 0x88000000, "ldos",	I_CORE1, A_CORE1, MEM2, 2, M,  R },
{ 0x8a000000, "stos",	I_CORE1, A_CORE1, MEM2, 2, R , M },
{ 0x8c000000, "lda",	I_CORE1, A_CORE1, MEM1, 2, M,  R },
{ 0x90000000, "ld",	I_CORE1, A_CORE1, MEM4, 2, M,  R },
{ 0x92000000, "st",	I_CORE1, A_CORE1, MEM4, 2, R , M },
{ 0x98000000, "ldl",	I_CORE1, A_CORE1, MEM8, 2, M,  R2 },
{ 0x9a000000, "stl",	I_CORE1, A_CORE1, MEM8, 2, R2 ,M },
{ 0xa0000000, "ldt",	I_CORE1, A_CORE1, MEM12, 2, M,  R4 },
{ 0xa2000000, "stt",	I_CORE1, A_CORE1, MEM12, 2, R4 ,M },
{ 0xb0000000, "ldq",	I_CORE1, A_CORE1, MEM16, 2, M,  R4 },
{ 0xb2000000, "stq",	I_CORE1, A_CORE1, MEM16, 2, R4 ,M },
{ 0xc0000000, "ldib",	I_CORE1, A_CORE1, MEM1, 2, M,  R },
{ 0xc2000000, "stib",	I_CORE1, A_CORE1, MEM1, 2, R , M },
{ 0xc8000000, "ldis",	I_CORE1, A_CORE1, MEM2, 2, M,  R },
{ 0xca000000, "stis",	I_CORE1, A_CORE1, MEM2, 2, R , M },
{ R_3(0x5b0), "addc",	I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x5b2), "subc",	I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_2(0x5a0), "cmpo",	I_CORE1, A_CORE1,    REG,    2, RSL,RSL },
{ R_2(0x5a1), "cmpi",	I_CORE1, A_CORE1,    REG,    2, RSL,RSL },
{ R_2(0x5a2), "concmpo",I_CORE1, A_CORE1,    REG,    2, RSL,RSL },
{ R_2(0x5a3), "concmpi",I_CORE1, A_CORE1,    REG,    2, RSL,RSL },
{ R_3(0x5a4), "cmpinco",I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x5a5), "cmpinci",I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x5a6), "cmpdeco",I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x5a7), "cmpdeci",I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_2(0x5ac), "scanbyte",I_CORE1, A_CORE1,    REG,    2, RSL,RSL },
{ R_2(0x5ae), "chkbit", I_CORE1, A_CORE1,    REG,    2, RSL,RSL },
{ R_3(0x580), "notbit", I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x581), "and",	I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x582), "andnot", I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x583), "setbit", I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x584), "notand", I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x586), "xor",	I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x587), "or",	I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x588), "nor",	I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x589), "xnor",	I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_2D(0x58a), "not",	I_CORE1, A_CORE1,    REG,    2, RSL,RS },
{ R_3(0x58b), "ornot",	I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x58c), "clrbit", I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x58d), "notor",	I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x58e), "nand",	I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x58f), "alterbit",I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x590), "addo",	I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x591), "addi",	I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x592), "subo",	I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x593), "subi",	I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x598), "shro",	I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x59a), "shrdi",	I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x59b), "shri",	I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x59c), "shlo",	I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x59d), "rotate",	I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x59e), "shli",   I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_2D(0x5cc), "mov",   I_CORE1, A_CORE1,    REG,    2, RSL,RS },
{ R_2D(0x5dc), "movl",  I_CORE1, A_CORE1,    REG,    2, RSL2,RS2 },
{ R_2D(0x5ec), "movt",  I_CORE1, A_CORE1,    REG,    2, RSL4,RS4 },
{ R_2D(0x5fc), "movq",  I_CORE1, A_CORE1,    REG,    2, RSL4,RS4 },
{ R_3(0x610), "atmod",  I_CORE1, A_CORE1,    REG,    3, RS, RSL,RS },
{ R_3(0x612), "atadd",  I_CORE1, A_CORE1,    REG,    3, RS, RSL,RS },
{ R_2D(0x640), "spanbit",I_CORE1, A_CORE1,    REG,    2, RSL,RS },
{ R_2D(0x641), "scanbit",I_CORE1, A_CORE1,    REG,    2, RSL,RS },
{ R_3(0x645), "modac",  I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x650), "modify", I_CORE1, A_CORE1,    REG,    3, RSL,RSL,R },
{ R_3(0x651), "extract",I_CORE1, A_CORE1,    REG,    3, RSL,RSL,R },
{ R_3(0x654), "modtc",  I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x655), "modpc",  I_CORE1, A_CORE1,    REG,    3, RSL,RSL,R },
{ R_1(0x660), "calls",  I_CORE1, A_CORE1,    REG,    1, RSL },
{ R_0(0x66b), "mark",   I_CORE1, A_CORE1,    REG,    0, },
{ R_0(0x66c), "fmark",  I_CORE1, A_CORE1,    REG,    0, },
{ R_0(0x66d), "flushreg",I_CORE1, A_CORE1,    REG,    0, },
{ R_0(0x66f), "syncf",  I_CORE1, A_CORE1,    REG,    0, },
{ R_3(0x670), "emul",   I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS2 },
{ R_3(0x671), "ediv",   I_CORE1, A_CORE1,    REG,    3, RSL,RSL2,RS2 },
{ R_2D(0x672), "cvtadr",I_CASIM, A_CX,    REG,    2, RL, R2 },
{ R_3(0x701), "mulo",   I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x708), "remo",  I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x70b), "divo",  I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x741), "muli",  I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x748), "remi",  I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x749), "modi",  I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
{ R_3(0x74b), "divi",  I_CORE1, A_CORE1,    REG,    3, RSL,RSL,RS },
 
 /* Floating-point instructions */

{ R_2D(0x674), "cvtir",	I_FP, A_KB, REG, 2, RL, F },
{ R_2D(0x675), "cvtilr",I_FP, A_KB, REG, 2, RL, F },
{ R_3(0x676), "scalerl",I_FP, A_KB, REG, 3, RL, FL2,F2 },
{ R_3(0x677), "scaler", I_FP, A_KB, REG, 3, RL, FL, F },
{ R_3(0x680), "atanr",	I_FP, A_KB, REG, 3, FL, FL, F },
{ R_3(0x681), "logepr", I_FP, A_KB, REG, 3, FL, FL, F },
{ R_3(0x682), "logr",	I_FP, A_KB, REG, 3, FL, FL, F },
{ R_3(0x683), "remr",	I_FP, A_KB, REG, 3, FL, FL, F },
{ R_2(0x684), "cmpor",	I_FP, A_KB, REG, 2, FL, FL },
{ R_2(0x685), "cmpr",	I_FP, A_KB, REG, 2, FL, FL },
{ R_2D(0x688), "sqrtr", I_FP, A_KB, REG, 2, FL, F },
{ R_2D(0x689), "expr",  I_FP, A_KB, REG, 2, FL, F },
{ R_2D(0x68a), "logbnr",I_FP, A_KB, REG, 2, FL, F },
{ R_2D(0x68b), "roundr",I_FP, A_KB, REG, 2, FL, F },
{ R_2D(0x68c), "sinr",  I_FP, A_KB, REG, 2, FL, F },
{ R_2D(0x68d), "cosr",  I_FP, A_KB, REG, 2, FL, F },
{ R_2D(0x68e), "tanr",  I_FP, A_KB, REG, 2, FL, F },
{ R_1(0x68f), "classr", I_FP, A_KB, REG, 1, FL  },
{ R_3(0x690), "atanrl", I_FP, A_KB, REG, 3, FL2,FL2,F2 },
{ R_3(0x691), "logeprl",I_FP, A_KB, REG, 3, FL2,FL2,F2 },
{ R_3(0x692), "logrl",	I_FP, A_KB, REG, 3, FL2,FL2,F2 },
{ R_3(0x693), "remrl",	I_FP, A_KB, REG, 3, FL2,FL2,F2 },
{ R_2(0x694), "cmporl", I_FP, A_KB, REG, 2, FL2,FL2 },
{ R_2(0x695), "cmprl",	I_FP, A_KB, REG, 2, FL2,FL2 },
{ R_2D(0x698), "sqrtrl",I_FP, A_KB, REG, 2, FL2,F2 },
{ R_2D(0x699), "exprl", I_FP, A_KB, REG, 2, FL2,F2 },
{ R_2D(0x69a), "logbnrl",I_FP, A_KB, REG, 2, FL2,F2 },
{ R_2D(0x69b), "roundrl",I_FP, A_KB, REG, 2, FL2,F2 },
{ R_2D(0x69c), "sinrl", I_FP, A_KB, REG, 2, FL2,F2 },
{ R_2D(0x69d), "cosrl", I_FP, A_KB, REG, 2, FL2,F2 },
{ R_2D(0x69e), "tanrl", I_FP, A_KB, REG, 2, FL2,F2 },
{ R_1(0x69f), "classrl",I_FP, A_KB, REG, 1, FL2  },
{ R_2D(0x6c0), "cvtri", I_FP, A_KB, REG, 2, FL, R },
{ R_2D(0x6c1), "cvtril",I_FP, A_KB, REG, 2, FL, R2 },
{ R_2D(0x6c2), "cvtzri",I_FP, A_KB, REG, 2, FL, R },
{ R_2D(0x6c3), "cvtzril",I_FP, A_KB, REG, 2, FL, R2 },
{ R_2D(0x6c9), "movr",  I_FP, A_KB, REG, 2, FL, F },
{ R_2D(0x6d9), "movrl", I_FP, A_KB, REG, 2, FL2,F2 },
{ R_2D(0x6e1), "movre", I_FP, A_KB, REG, 2, FL4,F4 },
{ R_3(0x6e2), "cpysre", I_FP, A_KB, REG, 3, FL4,FL4,F4 },
{ R_3(0x6e3), "cpyrsre",I_FP, A_KB, REG, 3, FL4,FL4,F4 },
{ R_3(0x78b), "divr",	I_FP, A_KB, REG, 3, FL, FL, F },
{ R_3(0x78c), "mulr",	I_FP, A_KB, REG, 3, FL, FL, F },
{ R_3(0x78d), "subr",	I_FP, A_KB, REG, 3, FL, FL, F },
{ R_3(0x78f), "addr",	I_FP, A_KB, REG, 3, FL, FL, F },
{ R_3(0x79b), "divrl",	I_FP, A_KB, REG, 3, FL2,FL2,F2 },
{ R_3(0x79c), "mulrl",	I_FP, A_KB, REG, 3, FL2,FL2,F2 },
{ R_3(0x79d), "subrl",	I_FP, A_KB, REG, 3, FL2,FL2,F2 },
{ R_3(0x79f), "addrl",	I_FP, A_KB, REG, 3, FL2,FL2,F2 },

 /* These are the floating point branch instructions.  Each actually
  * generates 2 branch instructions:  the first a CTRL instruction with
  * the indicated opcode, and the second a 'bno'.
  */

{ 0x12000000, "brue",  I_FP, A_KB, FBRA,  1 },
{ 0x11000000, "brug",  I_FP, A_KB, FBRA,  1 },
{ 0x13000000, "bruge", I_FP, A_KB, FBRA,  1 },
{ 0x14000000, "brul",  I_FP, A_KB, FBRA,  1 },
{ 0x16000000, "brule", I_FP, A_KB, FBRA,  1 },
{ 0x15000000, "brulg", I_FP, A_KB, FBRA,  1 },


 /* Decimal instructions */

{ R_3(0x642), "daddc",	I_DEC, A_KA|A_KB, REG, 3, RSL,RSL,RS },
{ R_3(0x643), "dsubc",	I_DEC, A_KA|A_KB, REG, 3, RSL,RSL,RS },
{ R_2D(0x644), "dmovt", I_DEC, A_KA|A_KB, REG, 2, RSL,RS },


 /* KX extensions */

{ R_2(0x600), "synmov",	I_KX, A_KA|A_KB, REG, 2, R,  R },
{ R_2(0x601), "synmovl",I_KX, A_KA|A_KB, REG, 2, R,  R },
{ R_2(0x602), "synmovq",I_KX, A_KA|A_KB, REG, 2, R,  R },
{ R_2D(0x615), "synld", I_KX, A_KA|A_KB, REG, 2, R,  R },


 /* CX extensions */

{ R_3(0x630), "sdma",  I_CX, A_CX, REG, 3, RSL,RSL,RL },
{ R_3(0x631), "udma",  I_CX, A_CX, REG, 0  },

 /* CX extensions, since usurped by CORE2 */
 /* Although these are supported by all 3 of: C-series, J-series,
    and H-series processors, we'll label the instructions CX. 
    An object file labelled CX will link with JX or HX anyway. */

{ R_3(0x5d8), "eshro",	I_CX, A_CX|A_JX|A_JL|A_HX, REG, 3, RSL,RSL,RS },
{ R_3(0x659), "sysctl", I_CX, A_CX|A_JX|A_JL|A_HX, REG, 3, RSL,RSL,R },


 /* CORE2 extensions */

{ R_3(0x780), "addono",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x790), "addog",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7a0), "addoe",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7b0), "addoge", I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7c0), "addol",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7d0), "addone", I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7e0), "addole", I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7f0), "addoo",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x781), "addino", I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x791), "addig",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7a1), "addie",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7b1), "addige", I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7c1), "addil",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7d1), "addine", I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7e1), "addile", I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7f1), "addio",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x782), "subono", I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x792), "subog",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7a2), "suboe",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7b2), "suboge", I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7c2), "subol",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7d2), "subone", I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7e2), "subole", I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7f2), "suboo",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x783), "subino", I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x793), "subig",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7a3), "subie",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7b3), "subige", I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7c3), "subil",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7d3), "subine", I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7e3), "subile", I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7f3), "subio",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x784), "selno",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x794), "selg",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7a4), "sele",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7b4), "selge",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7c4), "sell",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7d4), "selne",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7e4), "selle",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_3(0x7f4), "selo",	I_CORE2, A_CORE2, REG, 3, RSL,RSL,RS },
{ R_2(0x594), "cmpob",	I_CORE2, A_CORE2, REG, 2, RSL,RSL },
{ R_2(0x595), "cmpib",	I_CORE2, A_CORE2, REG, 2, RSL,RSL },
{ R_2(0x596), "cmpos",	I_CORE2, A_CORE2, REG, 2, RSL,RSL },
{ R_2(0x597), "cmpis",	I_CORE2, A_CORE2, REG, 2, RSL,RSL },
{ R_2D(0x5ad), "bswap",	I_CORE2, A_CORE2, REG, 2, RSL,RS },

/* JX/HX Extensions      */
/* These instructions are implemented for BOTH the J-series 
   and H-series processors.  We'll label them JX, but an object
   file marked JX will link with an HX file anyway. */
{ R_0(0x5b4), "intdis",	I_JX, A_JX|A_JL|A_HX, REG, 0,  },
{ R_0(0x5b5), "inten",	I_JX, A_JX|A_JL|A_HX, REG, 0,  },
{ R_2D(0x658), "intctl",I_JX, A_JX|A_JL|A_HX, REG, 2, RSL,RS },
{ R_3(0x65b), "icctl",	I_JX, A_JX|A_JL|A_HX, REG, 3, RSL,RSL,RS },
{ R_3(0x65c), "dcctl",	I_JX, A_JX|A_JL|A_HX, REG, 3, RSL,RSL,RS },
{ R_1(0x65d), "halt",	I_JX, A_JX|A_JL|A_HX, REG, 1, RSL        },
 
/* HX Extensions */
/* NOTE: the 0xf8 in the src/dst field for the dc* instructions is to
   work around an anomaly in the HA hardware, per EAS, rev. 2.1 */

{ 0xadf80000, "dcinva",	    I_HX, A_HX,  MEM1, 1, M },
{ 0xbdf80000, "dcflusha",   I_HX, A_HX,  MEM1, 1, M },
{ 0x9df80000, "dchint",	    I_HX, A_HX,  MEM1, 1, M },
{ 0xd0000000, "icemark",    I_HX, A_HX,  MEM1, 2, M, R },

/* Coprocessor Instruction encodings */
{ R_3(0x703), "cpmulo", I_JX, A_JL,        REG,    3, RSL,RSL,RS },

/* cpd960, and cpdcp are funny in that opcode is actually specified
 * in instruction, and checked for validity there. */
{ 0x00000000, "cpd960", I_JX, A_JL,  COPR, 4, L12,RL,RL,R},
{ 0x00000000, "cpdcp",	I_JX, A_JL,  COPR, 4, L12,RL,RL,L},

/* END OF TABLE */

{ 0,  NULL,  0,  0, 0 }
};

/* ERRATA1: these are the CTRL and REG instructions we have to detect.
   These are hashed in md_begin, and referenced in md_assemble.
   See comment circa line 425 in tc_i960.c for more explanation. */
struct i960_errata1
{
    char *name; /* Assembler mnemonic       */
    char format;/* REG, COBR, CTRL, MEMn, COJ, FBRA, or CALLJ    */
};

const struct i960_errata1 errata1_opcodes[] =
{ 
    { "bg",   CTRL },
    { "be",   CTRL },
    { "bge",  CTRL },
    { "bl",   CTRL },
    { "bne",  CTRL },
    { "ble",  CTRL },
    { "bo",   CTRL },
    { "bno",  CTRL },
    { "addc", REG },
    { "subc", REG },
    { "cmpo", REG },
    { "cmpi", REG },
    { "concmpo", REG },
    { "concmpi", REG },
    { "cmpinco", REG },
    { "cmpinci", REG },
    { "cmpdeco", REG },
    { "cmpdeci", REG },
    { "scanbyte", REG },
    { "chkbit", REG },
    { NULL, 0   }
};
