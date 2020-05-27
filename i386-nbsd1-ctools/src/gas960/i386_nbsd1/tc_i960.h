/* tc-i960.h - Basic 80960 instruction formats.
   Copyright (C) 1989, 1990, 1991 Free Software Foundation, Inc.

This file is part of GAS, the GNU Assembler.

GAS is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 1,
or (at your option) any later version.

GAS is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public
License along with GAS; see the file COPYING.  If not, write
to the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. */

/* $Id: tc_i960.h,v 1.16 1995/07/20 23:27:22 kevins Exp paulr $ */

/*
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

 /* tailor gas */
#define LOCAL_LABELS_FB

 /* tailor the coff format */
#define OBJ_COFF_MAX_AUXENTRIES	(2)

/* Classes of 960 intructions:
 *	- each instruction falls into one class.
 *	- each target architecture supports one or more classes.
 * Used to predict compatible linkage with other modules.
 * EACH CONSTANT MUST CONTAIN 1 AND ONLY 1 SET BIT!:  see targ_has_iclass().
 */
#define I_CORE1	0x01	/* Original 80960 core instruction set	*/
#define I_CX	0x02	/* 80960Cx instruction		*/
#define I_DEC	0x04	/* Decimal instruction		*/
#define I_FP	0x08	/* Floating point instruction	*/
#define I_KX	0x10	/* 80960Kx instruction		*/
#define I_CASIM	0x40	/* CA simulator instruction	*/
#define I_CORE2 0x80	/* NEW 80960 core instructions, added for JX and HX */
#define I_JX	0x100	/* 80960Jx instruction */
#define I_HX	0x200	/* 80960Hx instruction */

/* Classes of 960 architectures:
 *	- each instruction is supported by one or more 960 architectures
 * Used to check valid assembly input only.
 * EACH CONSTANT MUST CONTAIN 1 AND ONLY 1 SET BIT!
 */
#define A_KA	0x01	/* Instruction can be executed on 80960KA */
#define A_KB	0x02	/* Instruction can be executed on 80960KB */
#define A_CX	0x04	/* Instruction can be executed on 80960Cx */
#define A_JX	0x08 	/* Instruction can be executed on 80960Jx */
#define A_JL	0x10	/* Instruction can be executed on JX-Lite */
#define A_HX	0x20	/* Instruction can be executed on 80960Hx */

#define A_CORE1 A_KA|A_KB|A_CX|A_JX|A_JL|A_HX
#define A_CORE2 A_JX|A_JL|A_HX

/* MEANING OF 'n_other' in the symbol record.
 *
 * If non-zero, the 'n_other' fields indicates either a leaf procedure or
 * a system procedure, as follows:
 *
 *	1 <= n_other <= 32 :
 *		The symbol is the entry point to a system procedure.
 *		'n_value' is the address of the entry, as for any other
 *		procedure.  The system procedure number (which can be used in
 *		a 'calls' instruction) is (n_other-1).  These entries come from
 *		'.sysproc' directives.
 *
 *	n_other == N_CALLNAME
 *		the symbol is the 'call' entry point to a leaf procedure.
 *		The *next* symbol in the symbol table must be the corresponding
 *		'bal' entry point to the procedure (see following).  These
 *		entries come from '.leafproc' directives in which two different
 *		symbols are specified (the first one is represented here).
 *	
 *
 *	n_other == N_BALNAME
 *		the symbol is the 'bal' entry point to a leaf procedure.
 *		These entries result from '.leafproc' directives in which only
 *		one symbol is specified, or in which the same symbol is
 *		specified twice.
 *
 * Note that an N_CALLNAME entry *must* have a corresponding N_BALNAME entry,
 * but not every N_BALNAME entry must have an N_CALLNAME entry.
 */
#if 0

We use the definitions of these from include/bout.h

#define	N_CALLNAME	(-1)
#define	N_BALNAME	(-2)
#endif

 /* i960 uses a custom relocation record. */

 /* let obj-aout.h know */
#define CUSTOM_RELOC_FORMAT 1
 /* let a.out.gnu.h know */
#define N_RELOCATION_INFO_DECLARED 1

#ifndef OBJ_BOUT
/* This structure is included as part of bout.h, if we are generating b.out */
struct relocation_info {
	int	 r_address;	/* File address of item to be relocated	*/
	unsigned
		r_symbolnum:24,/* Index of symbol on which relocation is based*/
		r_pcrel:1,	/* 1 => relocate PC-relative; else absolute
				 *	On i960, pc-relative implies 24-bit
				 *	address, absolute implies 32-bit.
				 */
		r_length:2,	/* Number of bytes to relocate:
				 *	0 => 1 byte
				 *	1 => 2 bytes
				 *	2 => 4 bytes -- only value used for i960
				 */
		r_extern:1,
		r_bsr:1,	/* Something for the GNU NS32K assembler */
		r_disp:1,	/* Something for the GNU NS32K assembler */
		r_callj:1,	/* 1 if relocation target is an i960 'callj' */
		r_calljx:1;	/* 1 if relocation target is an i960 'calljx'*/
};
#endif	/* OBJ_BOUT */

 /* hacks for tracking callj's */
#define TC_S_MAX_SYSPROC_INDEX  259

#define TC_S_IS_SYSPROC(s)	((s)->is_sysproc)
#define TC_S_IS_LEAFPROC(s)	((s)->is_leafproc)
#define TC_S_IS_BALNAME(s)	((s)->is_balname)

#define TC_S_GET_SYSINDEX(s)	((s)->i960_sys_index)
#define TC_S_GET_BALSYM(s)      ((s)->bal_entry_symbol)
#define TC_S_GET_BALNAME(s)	(TC_S_GET_BALSYM(s)->sy_name)

#define TC_S_SET_SYSINDEX(s, p)	((s)->i960_sys_index = (p))
#define TC_S_SET_BALSYM(s,sym)  (TC_S_GET_BALSYM(s) = (sym))
#define TC_S_SET_BALNAME(s, p)  (TC_S_GET_BALNAME(s) = (p))

#ifdef __STDC__

void brtab_emit(void);
void reloc_callj(); /* this is really reloc_callj(fixS *fixP) but I don't want to change header inclusion order. */
void tc_set_bal_of_call(); /* this is really tc_set_bal_of_call(symbolS *callP, symbolS *balP) */

#else /* __STDC__ */

void brtab_emit();
void reloc_callj();
void tc_set_bal_of_call();

#endif /* __STDC__ */
