
/*(c**************************************************************************** *
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
 ***************************************************************************c)*/

#include <stdio.h>

#ifdef MWC
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "paths.h"
#include "err_msg.h"

/* 
 * FIXME: Workaround for broken vfprintf() on sol-sun4 
 */
#ifdef	SOLARIS
char 	sol_tmpbuf[1024];
#endif

/* Table of errors */
error_struct errors[] = {
/* COMMON ERRORS */
#if defined(LNK960)
/* The linker has to prepend filenames and line numbers - we'll revise this later for all tools */
/* files */
	NOT_OPEN,"%s Unable to open %s\n",			/* p1 = filename (ldaopen) */
	WRIT_ERR,"%s Error writing to %s\n",		/* p1 = filename */
	NO_FILES,"%s No files given.\n",			/* no p1 */
	NO_FOUND,"%s File not found: %s\n",		/* p1 = filename */
	NOT_COFF,"%s Not in COFF format: %s\n",		/* p1 = filename */
	NOT_COF2,"%s Not in COFF format: %s[%s]\n",	/* p1 = arname p2 = membername */
	CRE_FILE,"%s Unable to create file %s\n",		/* p1 = name */
	TMP_FILE,"%s Unable to create temporary file %s\n",/* p1 = filename */
	NO_TMPNM,"%s Unable to create name(s) for temporary file(s)\n",/*no p1*/
	NOATEXIT,"%s System error, atexit() failed\n",	/* no p1 */
    	EXTR_FIL,"%s Too many filenames\n", 		/* no p1 */
	ER_FREAD,"%s Unable to read %s\n",			/* p1 = filename */
	PREM_EOF,"%s EOF found before expected\n",			/* no p1 */
/* allocation */
	ALLO_MEM,"%s Unable to allocate memory\n",		/* no p1 */
	ALLO_ARR,"%s Unable to allocate space for array\n",/* no p1 */
	NSP_ARGS,"%s No space for args\n",			/* no p1 */
/* options */
	OPT_ARG,"Option requires an argument -- %s\n",	/* p1 = option */ /* MISSING %s due to getopt() kluge */
	ILL_OPT,"Illegal option -- %s\n",		/* p1 = option */ /* MISSING %s due to getopt() kluge */

	AOPT_ARG,"%s Architecture option needs an argument\n",	/* no p1 */
	BD_AOPT, "%s Illegal architecture: %s\n",          /* p1 = option */
#ifdef VMS
	EMPY_OPT,"%s empty option\n",			/* no p1 */
	AMBG_OPT,"%s ambiguous option %s\n",		/* p1 = keyword */
        SUBPROCQUOTA, "%s error in calling %s, check subprocess quota(PRCLM)\n", 
	WILDCARD, "%s Wildcard specification not permitted in this parameter (%s)\n",
#endif
/* coff */
	RD_ARCHD,"%s Unable to read archive header in %s\n",/* p1 = filename (ldahread) */
	RD_ARSYM,"%s Unable to read archive symbol table\n",/* no p1 */
	RD_FILHD,"%s Unable to read file header in %s\n",	/* p1 = filename (ldfhread)*/
	WR_FILHD,"%s Unable to write file header in %s\n",	/* p1 = filename */
	NO_OFLHD,"%s No optional file header in %s\n", 	/* p1 = filename (ldohseek) */
	NO_OHDSZ,"%s Unknown optional header size %s\n",	/* p1 = size */
	RD_OFLHD,"%s Unable to read optional file header in %s\n",/* p1 = filename  */
	WR_OFLHD,"%s Unable to write optional file header in %s\n",/* p1 = filename  */
	RD_SECHD,"%s Unable to read section header %s\n",	/* p1 = section number (ldshread) */
	WR_SECHD,"%s Unable to write section header %s\n",	/* p1 = section number*/
	NO_SECHD,"%s Unable to find section header %s\n", 	/* p1 = section name: filename */
	RD_SECT,"%s Unable to read section %s\n",		/* p1 = section number (ldsseek) */
	SYMTB_HD,"%s Unable to read symbol table header\n",/* no p1 */
	RD_SYMTB,"%s Unable to read symbol table entry %s\n", /* p1 = symindex (ldtbread) */
	NO_SYMTB,"%s No symbol table in %s\n",		/* p1 = filename (ldtbread or ldtbseek) */
	RD_SYMNM,"%s Unable to read name of symbol table entry %s\n", /* p1 = symindex (ldgetname) */
	SK_SYMTB,"%s Unable to seek to end of symbol table\n", /* no p1 */
	NO_FUNC,"%s Symbol table has no such function: %s\n", /* p1 = function name or funcname:filename */
	RD_STRTB,"%s Unable to read string table %s\n",	/* p1 = filename */
	RD_AUXEN,"%s Unable to read aux entry %s\n",	/* p1 = symindx */
	RD_RELOC,"%s Unable to read relocation information in section %s\n", /* p1 = section number (ldrseek) */
	WR_RELOC,"%s Unable to write relocation information in section %s\n",/* p1 = section number */
	RD_LINEN,"%s Unable to read line numbers\n",	/* no p1 */
	RD_LINES,"%s Unable to read line number entries in section %s\n",	/* p1 = section number (ldlseek) */
	WR_LINES,"%s Unable to write line number entries in section %s\n",	/* p1 = section number */
	NO_LINE,"%s No line number >= %s\n",	        /* p1 = linenum (ldlitem or ldlread) */
	NLINE_FN,"%s No line numbers for function",	/* no p1 (ldlinit or ldlread) */
#else
/* files */
	NOT_OPEN,"Unable to open %s\n",			/* p1 = filename (ldaopen) */
	WRIT_ERR,"Error writing to %s\n",		/* p1 = filename */
	NO_FILES,"No files given.\n",			/* no p1 */
	NO_FOUND,"File not found: %s\n",		/* p1 = filename */
	NOT_COFF,"Not in COFF format: %s\n",		/* p1 = filename */
	NOT_COF2,"Not in COFF format: %s[%s]\n",	/* p1 = arname p2 = membername */
	CRE_FILE,"Unable to create file %s\n",		/* p1 = name */
	TMP_FILE,"Unable to create temporary file %s\n",/* p1 = filename */
	NO_TMPNM,"Unable to create name(s) for temporary file(s)\n", /* no p1 */
	NOATEXIT,"System error, atexit() failed\n",	/* no p1 */
    	EXTR_FIL,"Too many filenames\n", 		/* no p1 */
	ER_FREAD,"Unable to read %s\n",			/* p1 = filename */
	PREM_EOF,"EOF found before expected\n",	/* no p1 */
/* allocation */
	ALLO_MEM,"Unable to allocate memory\n",		/* no p1 */
	ALLO_ARR,"Unable to allocate space for array\n",/* no p1 */
	NSP_ARGS,"No space for args\n",			/* no p1 */
/* options */
	OPT_ARG,"Option requires an argument -- %s\n",	/* p1 = option */
	ILL_OPT,"Illegal option -- %s\n",		/* p1 = option */

	AOPT_ARG,"Architecture option needs an argument\n",	/* no p1 */
	BD_AOPT, "Illegal architecture: %s\n",          /* p1 = option */
#ifdef VMS
	EMPY_OPT,"Empty option\n",			/* no p1 */
	AMBG_OPT,"Ambiguous option %s\n",		/* p1 = keyword */
        SUBPROCQUOTA, "Error in calling %s, check subprocess quota(PRCLM)\n", 
	WILDCARD, "Wildcard specification not permitted in this parameter (%s)\n",
	OLD_OPTN, "Illegal option, use /mode={hexadecimal|decimal|octal}\n",
#endif
/* coff */
	RD_ARCHD,"Unable to read archive header in %s\n",/* p1 = filename (ldahread) */
	RD_ARSYM,"Unable to read archive symbol table\n",/* no p1 */
	RD_FILHD,"Unable to read file header in %s\n",	/* p1 = filename (ldfhread)*/
	WR_FILHD,"Unable to write file header in %s\n",	/* p1 = filename */
	NO_OFLHD,"No optional file header in %s\n", 	/* p1 = filename (ldohseek) */
	NO_OHDSZ,"Unknown optional header size %s\n",	/* p1 = size */
	RD_OFLHD,"Unable to read optional file header in %s\n",/* p1 = filename  */
	WR_OFLHD,"Unable to write optional file header in %s\n",/* p1 = filename  */
	RD_SECHD,"Unable to read section header %s\n",	/* p1 = section number (ldshread) */
	RD_SCHDF,"Unable to read section header %d in %s\n",/* p1=sec,p2=fname*/
	WR_SECHD,"Unable to write section header %s\n",	/* p1 = section number*/
	WR_SCHDF,"Unable to write section header %d in %s\n",/*p1=sec,p2=fnam*/
	NO_SECHD,"Unable to find section header %s\n", 	/* p1 = section name: filename */
	RD_SECT,"Unable to read section %s\n",		/* p1 = section number (ldsseek) */
	SYMTB_HD,"Unable to read symbol table header\n",/* no p1 */
	RD_SYMTB,"Unable to read symbol table entry %s\n", /* p1 = symindex (ldtbread) */
	NO_SYMTB,"No symbol table in %s\n",		/* p1 = filename (ldtbread or ldtbseek) */
	RD_SYMNM,"Unable to read name of symbol table entry %s\n", /* p1 = symindex (ldgetname) */
	SK_SYMTB,"Unable to seek to end of symbol table\n", /* no p1 */
	NO_FUNC,"Symbol table has no such function: %s\n", /* p1 = function name or funcname:filename */
	RD_STRTB,"Unable to read string table in %s\n",	/* p1 = filename */
	RD_AUXEN,"Unable to read aux entry %s\n",	/* p1 = symindx */
	RD_RELOC,"Unable to read relocation information in section %s\n", /* p1 = section number (ldrseek) */
	RD_RLOCF,"Unable to read relocation information in section %d of %s\n",
					 /* p1 = section number p2=filename*/
	WR_RELOC,"Unable to write relocation information in section %s\n",/* p1 = section number */
	WR_RLOCF,"Unable to write relocation information in section %d of %s\n",
	RD_LINEN,"Unable to read line numbers\n",	/* no p1 */
	RD_LINES,"Unable to read line number entries in section %s\n",	/* p1 = section number (ldlseek) */
	RD_LINEF,"Unable to read line number entries in section %d of %s\n",
	WR_LINES,"Unable to write line number entries in section %s\n",	/* p1 = section number */
	WR_LINEF,"Unable to write line number entries in section %d of %s\n",
	NO_LINE,"No line number >= %s\n",	        /* p1 = linenum (ldlitem or ldlread) */
	NLINE_FN,"No line numbers for function",	/* no p1 (ldlinit or ldlread) */
#endif /* LNK960  COMMONS */
#ifdef DIS960
	N_LD_SEC,"No such loaded section %s\n", 	/* p1 = section name: filename */
	RD_FUNNM,"Unable to read function name\n",	/* no p1 */
	STR_BADD,"Possible strings in text or bad physical address before location %s\n",  /* p1 = address */
	N_SYMDIS,"No symbol table, unable to perform symbolic disassembly on: %s\n",  /* p1 = filename */
	F_D_OPTS,"Incompatible combination of flags: F and D or d\n",	/* no p1 */
	NO_TEXT,"> 9999 .words, %s section in %s may not be a text section\n",	/* p1=section p2=filename */
#ifdef VMS
	EX_FOPT,"too many functions specified\n",		/* no p1 */
#else
	EX_FOPT, "too many -F options\n",			/* no p1 */
#endif
#endif /* DIS960 */
#ifdef DMP960
	NO_OPT,"At least one option is required\n",	/* no p1 */
#ifndef VMS
	ONE_NOPT,"only one -n allowed, first one given used\n",	/* no p1 */ 
#endif
	RD_LIBSC,"failed to read .lib section in file: %s\n", /* p1 = filename */
#endif /* DMP960 */
#ifdef COF960
	AR_N_BIN,"Archive %s member %s not 80960 binary\n",
	BAD_ARCH,"Archive %s contains no COFF members\n",
	BAD_WHNC,"Bad whence (%d)\n",
	BAD_MAGC,"Bad magic number in %s (%x should be %x)\n",
	FIL_CNVT,"Already converted: %s\n",
	AFIL_CVT,"Already converted: %s[%s]\n",
	FIL_SHRT,"File %s is too short\n",
	UN_OHDSZ,"Unknown optional header size %d in %s\n",
	NAM_POOL,"Could not find member name in %s\n",
#endif /* COF960 */
#ifdef LNK960
	ADDR_EXCLUDES_OWNER,"%s can not specify an owner for section within a group\n",
	ADD_XXX_TO_MLT_OUT,"%s adding %s(%.8s) to multiple output sections\n",
	ADV_BAD_FILL,"%s bad fill value\n",
	ADV_BAD_SECT_FLAG,"%s bad flag value in SECTIONS directive: -%s\n",
 	ADV_EMBEDDED_TSWITCH,"%s Use TARGET for target files within ifiles\n",
	ADV_FLAG_NEEDS_NUM,"%s -%c flag does not specify a number: %s\n",
	ADV_INCLUDE,"%s Use INCLUDE for nesting ifile %s\n",
	ADV_I_REGIONS,"%s i flag ignored: %d regions specified\n",
	ADV_I_TEXT,"%s i flag ignored: text memory specified\n",
	ADV_L_NODIR_PATH,"%s -L path '%s' not found\n",
	ADV_MEM_BAD_ARG,"%s bad attribute value in MEMORY directive: %c\n",
	ADV_NEW_SECTION_LINES,"%s splitting section due to >64K line numbers.\nOld section renamed to %s, new section gets name %s\n",
	ADV_NEW_SECTION_RELOC,"%s relocation entries exceed 64K.\nSection split into old section %s and new section %s\n",
	ADV_NFLG,"%s operating system support of the -N option may be dropped in a future release\n",
	ADV_NO_TARGNAME,"%s Missing target name\n",
	ADV_O_BADNAME,"%s -o flag does not specify a valid file name\n",
	ADV_P_B_FLGS,"%s pflag(%d) less than Bflag, set to value of Bflag(%d)\n\n",
	ADV_RES_SECT_NAME,"%s %s is a reserved section name\n",
	ALIGN_BAD_IN_CONTEXT,"%s ALIGN illegal in this context\n",
	ALLOC_SLOTVEC,"%s Unable to allocate %ld bytes for slotvec table\n",
	ARC_STRTAB_SHORT,"%s too few symbol names in string table for archive %s\n",
	ARCH_MISMATCH,"%s architecture level mismatch%s\n",
	AUDIT_ADDR_MATCH,"%s audit_groups, address mismatch\n",
	AUDIT_NO_NODES,"%s audit_groups, findsanode failure\n",
	AUX_OFLO,"%s aux table overflow\n",
	AUX_OUT_OF_SEQ,"%s Making aux entry %d for symbol %s out of sequence\n",
	AUX_TAB_ID,"%s invalid aux table id\n",
	AUX_TAB_NEG,"%s negative aux table id\n",
	BAD_ALIGN,"%s %.1lx is not a power of 2\n",
	BAD_ALIGN_SECT_IN_GRP,"%s can not align a section within a group\n",
	BAD_ALLOC,"%s memory allocation failure on %d-byte 'calloc' call\n",
	BAD_ARCSIZ,"%s invalid archive size for file %s\n",
	BAD_BOND_ESC,"%s can not bond a section within a group\n",
	BAD_B_D_DFLT_ALLOC,"%s default allocation did not put %.8s and %.8s into same region\n",
	BAD_CONFIG_CMD,"%s unrecognized command in configuration file: %s\n\n",
	BAD_MAGIC,"%s file %s is of unknown type: magic number = %06.1x\n",
	BAD_PHY_ARG,"%s operand of PHY must be a name\n",
	BAD_SYSCALL_VALUE,"%s Invalid syscall value %d\n",
	BAD_RELOC_TYPE,"%s Illegal relocation type %d found in section %s in file %s\n",
	BOND_ADDR_EXCLUDES_OWNER,"%s cannot bond '%s' to an address AND specify an owner\n",
	BOND_NO_ALIGN,"%s bonding excludes alignment\n",
	BOND_NO_CONFIG,"%s %.8s enters unconfigured memory at %.2lx\n",
	BOND_OUT_OF_BOUNDS,"%s bond address %.2lx for %.8s is not in configured memory\n",
	BOND_OVRLAP,"%s bond address %.2lx for %.8s overlays previously allocated section %.8s at %.2lx\n",
	BOND_TOO_BIG,"%s %.8s, bonded at %.2lx, will not fit into configured memory\n",
	CP_RELOC_FIL,"%s Unable to copy relocation file %s into %s\n",
	CP_SECT,"%s Unable to copy section %s of file %s\n",
	CP_SECT_REM,"%s Unable to copy the rest of section %s of file %s\n",
	DECR_DOT,"%s attempt to decrement DOT\n",
	EXPR_TERM,"%s semicolon required after expression\n",
	FIL_SECT_TOO_BIG,"%s section %.8s in file %s is too big\n",
	GROUP_TOO_BIG,"%s GROUP containing section %.8s is too big\n",
	IFILE_NEST,"%s ifile nesting limit exceeded with file %s\n",
	ILL_ABS_IN_PHY,"%s phy of absolute symbol %s is illegal\n",
	ILL_ADDR_DOT,"%s illegal assignment of physical address to DOT\n",
	ILL_EXPR_OP,"%s illegal operator in expression\n",
	ILL_USED_DOT,"%s misuse of DOT symbol in assignment instruction\n",
 	INTRNL_ERROR,"%s has caused internal error in linker\n",
	IO_ERR_OUT,"%s I/O error on output file %s\n",
 	LD_SYNTAX,"%s syntax error\n",
	LIB_SYMDIR_TOO_BIG,"%s archive symbol directory in archive %s is too large\n",
	LINE_NUM_NON_RELOC,"%s line nbr entry (%ld %d) found for non-relocatable symbol: section %.8s, file %s\n",
	LIST_JUMBLE,"%s in allocate lists, list confusion (%d %d)\n",
	MEM_IGNORE,"%s MEMORY specification ignored\n",
	MEM_OVRLAP,"%s memory types %.8s and %.8s overlap\n",
	MEMORY_WRAP,"%s section %.8s (0x%lx bytes) causes memory wraparound\n", 
	MERG_STRING_FIL,"%s Unable to merge string file %s into %s\n",
	MISS_SECT,"%s Sections .text .data or .bss not found. Optional header may be useless\n",
	MLT_SYM,"%s Symbol %s in %s is multiply defined.\n",
	MLT_SYM_SZ,"%s Multiply defined symbol %s, in %s, has more than one size\n",
	MULT_DEF,"%s Symbol %s in %s is multiply defined. First defined in %s\n",
	MULT_FLOATS,"%s Multiple FLOAT/NOFLOATs found; %s will be used\n",
	MULT_LEAFPROC_DEFS,"%s Multiple definitions of leafproc %s\n",
	MULT_STARTUPS,"%s Multiple STARTUPs found; %s will be used\n",
	MULT_SYSCALL_INDEX,"%s Multiple indexes %d and %d given for sysproc %s\n",
	NOTFOUND_CRT0,"%s Cannot find startup file %s\n",
	NOTFOUND_LIB_FIL,"%s Cannot find library %s\n",
	NOTFOUND_LIB_NAME,"%s Cannot find library lib%s.a\n",
	NOTFOUND_TARGFILE,"%s Cannot find target file %s\n",
	NOT_FINISH_WRITE,"%s Cannot complete output file %s. Write error.\n",
	NOT_OPEN_IN,"%s Cannot open file %s for input\n",
	NOT_OPEN_TARG,"%s Cannot open target file %s\n\n",
	NO_ALLOC_ATTR,"%s Cannot allocate %.8s with attr %x\n",
	NO_ALLOC_OWN,"%s Cannot allocate section %.8s into owner %.8s\n",
	NO_ALLOC_SECT,"%s Cannot allocate output section %.8s, of size 0x%lx\n",
	NO_DFLT_ALLOC,"%s default allocation failed for %.8s\n",
	NO_DFLT_ALLOC_SIZ,"%s default allocation failed: %.8s is too large\n",
	NO_DOT_ENTRY,"%s internal error: no symtab entry for DOT\n",
	NO_ENTRY,"%s Entry point symbol (%s) not found\n",
	NO_EXEC,"%s Output file %s not executable\n",
	NO_HLL_NAME,"%s Missing HLL library name\n",
	NO_OUTPUT,"%s Error(s). No output written to %s\n",
	NO_RELOC_ENTRY,"%s No reloc entry found for symbol: index %ld, section %s, file %s, virtual addr. 0x%lx\n",
	NO_REL_FIL,"%s file %s has no relocation information\n",
	NO_REL_LIB,"%s library %s, member has no relocation information\n",
	NO_SEEK_RELOC_SECT,"%s Seek to relocation entries for section %.8s in file %s failed\n",
	NO_SEEK_SECT,"%s Seek to %s section %.8s %sfailed\n",
	NO_STARTUP_NAME,"%s Missing STARTUP file name\n",
	NO_STRTAB,"%s no string table in file %s\n",
	NO_SYSLIB_NAME,"%s Missing SYSLIB library name\n",
	NO_TARG,"%s no target file specified; I960_TARGET not defined\n",
	NO_TV_FILL,"%s tv fill symbol %s does not exist\n",
	NO_TV_SYM,"%s No .tv in symbol table\n",
	NO_XXX_IN_FILE,"%s %s(%.8s) not found\n",
 	NUM_AUXS_DIFF,"%s Multiple irreconcilable definitions of symbol %s\n",
	ODD_SECT,"%s Section %.8s starts on an odd byte boundary!\n",
	ORPH_DSECT,"%s DSECT %.8s cannot be given an owner\n",
	OUTFILE_OVERWRITES,"%s Output file will overwrite %s\n",
	OVR_AUX,"%s Overwriting aux entry %d of symbol %s\n",
	OVR_SECT,"%s GROUP\n",
	PIX_MISMATCH,"%s File %s is %s, expecting %s\n",
	RD_1ST_WORD,"%s Cannot read 1st word of file %s\n",
	RD_AUXEN_FIL,"%s Unable to read aux entries of file %s\n",
	RD_LIB,"%s error reading library %s\n",
	RD_I960LIB,"%s Unable to read library directory '%s'\n",
	RD_LINENO_FIL,"%s Unable to read lnno of section %.8s of file %s\n",
	RD_OPEN_TV,"%s Cannot open %s to read tv\n",
	RD_RELOC_FIL,"%s Unable to read reloc entries of file %s\n",
	RD_RELOC_F_TO_F,"%s Unable to read relocation file %s into %s\n",
	RD_REL_SECT_FIL,"%s Unable to read the reloc entries of section %s of %s\n",
	RD_SECHD_ENT_FIL,"%s Unable to read section header %d of %s\n",
	RD_SECHD_FIL,"%s Unable to read section headers of file %s\n",
	RD_SECT_FIL,"%s Unable to read section %s of file %s\n",
	RD_STRSIZ_FIL,"%s Unable to read string size of file %s\n",
	RD_STR_FIL,"%s Unable to read string file %s\n",
	RD_SYMDIR,"%s Cannot read archive symbol directory of archive %s\n",
	RD_SYMDIR_SYMNUM,"%s Cannot read archive symbol directory number of symbols from archive %s\n",
	RD_SYMTAB_ENT_FIL,"%s Unable to read symtb entry %ld of file %s\n",
	RD_SYMTAB_FIL,"%s Unable to read symbol table of file %s\n",
	RD_SYMTEMP_FIL,"%s Unable to read symbol table file %s\n",
	REDEF_ABS,"%s absolute symbol %s being redefined\n",
	REDEF_SYM,"%s symbol %s from file %s being redefined\n",
	REGION_NC_MEM,"%s region %.8s is outside of configured memory\n",
	RELOC_OFLO,"%s relocation overflow at offset 0x%lx of section %s of file %s\n",
	RES_IDENT,"%s file %s has a section name which is a reserved ld identifier: %.8s\n",
	SECT_NOT_BUILT,"%s section %s not built\n",
	SECT_TOO_BIG,"%s %.8s section will not fit in %s region\n",
	SEEK_MEM_FIL,"%s Unable to seek to the member of %s\n",
	SKIP_MEMSTR_FIL,"%s Unable to skip the mem of struct of %s\n",
	SKP_AUXEN,"%s Unable to skip the aux entry(s) of %s\n",
	SK_MEM,"%s Unable to seek to the member of %s\n",
	SK_REWIND,"%s Cannot seek to the beginning of file %s\n",
	SK_SYMTAB_FIL,"%s Unable to seek to symbol table of file %s\n",
	SPLIT_SCNS,"%s split_scns, size of %.8s exceeds its new displacement\n",
	STMT_IGNORE,"%s statement ignored\n",
	SYM_REF_ERRS,"%s Symbol referencing errors. No output written to %s\n",
	SYM_TAB_ID,"%s invalid symbol table id\n",
	SYM_TAB_NEG,"%s negative symbol table id\n",
	SYM_TAB_OFLO,"%s symbol table overflow\n",
	TINY_HDR,"%s optional header size (%d bytes) is too small to contain the COFF a.out header (%d bytes)\n",
	TOO_HARD,"%s %s run is too large and complex\n",
	TOO_MANY_LINENUMS,"%s >64K line numbers in %s; cannot complete link\n",
	TOO_MANY_RELOCS,"%s >64K relocation entries in %s; cannot complete link\n",
	TV_EXCESS_ENTRIES,"%s tv needs %ld entries; only %d allowed\n",
	TV_ILL_SYM,"%s attempt to assign tv slot to illegal symbol\n",
	TV_MULT_SLOT_FUNC,"%s Two tv slot assignments for function %.8s: %d and %d\n",
	TV_NOT_BUILT,"%s %.8s not built\n",
	TV_NO_ALIGN,"%s %.8s not aligned\n",
	TV_RANGE_TOO_NARROW,"%s tv range allows %d entries; %d needed\n",
	TV_SLOTS,"%s out of tv slots\n",
	TV_SYM_OUT_RANGE,"%s Defined symbol assigned a tv slot outside tv range\n",
	TV_UNDEF_SYM,"%s Undefined symbol assigned a tv slot within tv range\n",
	UNATTR_DSECT,"%s DSECT %.8s cannot be linked to an attribute\n",
	UNDEF_SYM,"%s symbol %s is undefined\n",
	UNDEF_SYSCALL,"%s No index given for sysproc %s\n",
	UNIMP_VAL_DEF,"%s inimplemented feature: value definition for symbol %.8s\n",
	WR_OPEN_TV,"%s Cannot open %s to write the tv\n",
	WR_SECT,"%s Unable to output section %s of file %s\n",
	WR_STRTB,"%s Unable to write size of string table for file %s\n",
	WR_SYMTEMP_FIL,"%s Unable to write symbol table file %s into %s\n",
	WR_SYM_STRTB,"%s Unable to write symbol name %s in string table for file %s\n",
	AS_ILLINCHAR, 	"%s Illegal input character, (decimal) %d\n",
#ifdef VMS
	ADV_B_ILL_SECT_NAME,"%s /bss flag does not specify a legal section name\n",
	ADV_E_ILL_SYM_NAME,"%s /entry qualifier does not specify a legal symbol name\n",
	ADV_F_NUMFORMAT,"%s /fill qualifier does not specify a two-byte number: %s\n",
	ADV_L2_UNSUPP,"%s the /library flag (specifying a default library) is not supported\n",
	ADV_L_2MANY,"%s too many /SEARCHLIB options, %d allowed\n",
	ADV_L_LONGPATH,"%s /searchlib path too long(%s)\n",
	ADV_L_NOPATH,"%s no directory given with /searchlib\n",
	ADV_O_BADARG,"%s illegal flag: /optimize=%s\n",
	ADV_P_BADARG,"%s illegal flag: /pix=%s\n",
	ADV_R_S_FLGS,"%s both /relocation and /strip flags are set. /strip flag turned off\n",
	ADV_U_ILL_SYM_NAME,"%s /undefine qualifier does not specify a legal symbol name\n",
	LIB_NO_SYMDIR,"%s archive symbol directory is missing in archive %s\nexecute `%s /list/symbol %s' to restore archive symbol directory\n",
	LIB_NO_SYMTAB,"%s archive symbol table is empty in archive %s\nexecute `%s /list/symbol %s' to restore archive symbol table\n",
	ONAME_TOO_BIG,"%s /output file name (%s) too large (> 128 char)\n",
#else
	ADV_B_ILL_SECT_NAME,"%s -b flag does not specify a legal section name\n",
	ADV_E_ILL_SYM_NAME,"%s -e flag does not specify a legal symbol name\n",
	ADV_F_NUMFORMAT,"%s -f flag does not specify a two-byte number: %s\n",
	ADV_L2_UNSUPP,"%s the -l flag (specifying a default library) is not supported\n",
	ADV_L_2MANY,"%s too many -L options, %d allowed\n",
	ADV_L_LONGPATH,"%s -L path too long(%s)\n",
	ADV_L_NOPATH,"%s no directory given with -L\n",
	ADV_O_BADARG,"%s illegal flag: -O%s\n",
	ADV_P_BADARG,"%s illegal flag: -p%s\n",
	ADV_R_S_FLGS,"%s both -r and -s flags are set. -s flag turned off\n",
	ADV_U_ILL_SYM_NAME,"%s -u flag does not specify a legal symbol name\n",
	LIB_NO_SYMDIR,"%s archive symbol directory is missing in archive %s\nexecute `%s ts %s' to restore archive symbol directory\n",
	LIB_NO_SYMTAB,"%s archive symbol table is empty in archive %s\nexecute `%s ts %s' to restore archive symbol table\n",
	ONAME_TOO_BIG,"%s -o file name (%s) too large (> 128 char)\n",
#endif /* VMS */
#endif /* LNK960 */

#ifdef SIZ960
    MULT_MODES, "Redefining output radix: Using %s\n",
    NOT_RD_ARHD,"Unable to read archive header in %s\n", /* p1 = filename */
    NOT_RD_SECHD,"Unable to read section header in %s\n", /* p1 = filename */
#endif /* SIZ960 */

#ifdef STR960
    NOT_RD_SECHD, "Unable to read section header for section %s\n", /* p1 = section */
    NOT_SEEK_LINE_NO, "Unable to seek to line numbers\n",    /* no p1 */
    NOT_RD_LINE_NO, "Unable to read line numbers\n",         /* no p1 */ 
    NOLOC_NSYMTB_IND, "Unable to locate new symbol table index\n",  /* no p1 */ 
    NOT_WR_LINE_NO, "Unable to write line numbers\n",         /* no p1 */
    NOT_RD_ARHD, "Unable to read archive header\n",           /* no p1 */
    NOT_RD_STR_TBL, "Unable to read string table\n",          /* no p1 */
    NOT_RD_REL_SEC, "Unable to read reloc info for section %d\n",   /* p1 = section no. */
    NOT_SEEK_REL_SEC, "Unable to seek to reloc entries in sect: %d\n", /* p1 = section no. */
    NOT_RD_REL, "Unable to read relocation\n",  /* no p1 */
    NOT_FWR, "Unable to fwrite\n",              /* no p1 */
    NO_INDEX_FND, "No index found for relocation entry: %lo\n",   /* p1 is a relocation index */
    MALLOC_FAIL, "Malloc failed, unable to reset relocation\n",  /* no p1 */
    NOT_CPY_OPTHD, "Unable to copy optional header: %s\n",   /* p1 = filename */
    NOT_CPY_SECHD, "Unable to copy section headers: %s\n",   /* p1 = filename */
    NOT_CPY_SECS, "Unable to copy sections: %s\n",           /* p1 = filename */
    NOT_CPY_REL_INF, "Unable to copy relocation information: %s\n", /* p1 = filename */
    NOT_CPY_SYM_TBL, "Unable to copy symbol table: %s\n",    /* p1 = filename */
    NOT_CPY_EXT_SYM, "Unable to copy external symbols: %s\n",/* p1 = filename */
    NOT_CPY_REL_ENT, "Unable to copy relocation entries: %s\n", /* p1 = filename */
    NOT_CPY_LINE_NO, "Unable to copy line numbers: %s\n",    /* p1 = filename */
    NO_SYM_TBL, "No symbol table: %s\n",    /* p1 = filename */
    NO_LOC_SYMS, "No local symbols: %s\n",  /* p1 = filename */
    REL_ENT_NOT_STR, "Relocation entries; cannot strip: %s\n", /* p1 = filename */
    NOT_REC_FILEHD, "Unable to recreate fileheader: %s\n",   /* p1 = filename */
    NOT_REC_STR_FILE, "Unable to recreate stripped file: %s\n", /* p1 = filename */
    NOT_RD_TMP_FILE, "Unable to read temp file\n",        /* no p1 */ 
    ERR_SEEK_FILE, "Error seeking file: %s\n",        /* p1 = filename */
    ERR_ARHD_RD, "Error in archive hdr read: %s\n",   /* p1 = filename */
    NOT_OPEN_TMP_RD, "Unable to open temporary file %s for reading\n", /* p1 = tmp filename */
    NOT_OPEN_AR_WR, "Unable to open archive file %s for writing\n",   /* no p1 */ 

#ifdef VMS
    V_TO_RESTORE, "Symbol directory deleted from archive %s\n    execute  `%s /list/symbol %s' to restore symbol directory.\n",    /* p1 is filename; p2 is archive name; p3 is filename */
#else
    TO_RESTORE, "Symbol directory deleted from archive %s\n    execute  `%s us %s' to restore symbol directory.\n",   /* p1 is filename; p2 is archive name; p3 is filename */
#endif

    NOT_CPY_AR_NAM, "Unable to copy archive names: %s\n",    /* p1 = filename */
    NOT_REC_FILE, "Unable to recreate file: %s\n",           /* p1 = filename */
    NOT_RD_FILE_HD, "Unable to read file header: %s\n",      /* p1 = filename */
    NOT_OPEN_TMP, "Unable to open temporary file: %s\n",     /* p1 = filename */
    NOT_WR_STR, "Can neither write nor strip %s\n",          /* p1 = filename */
    NOT_CPY_ARHD, "Unable to copy archive header back\n",        /* no p1 */ 
    NOT_WR_TMP, "Unable to write to tmp file: %s\n",         /* p1 = filename */
    NOT_REC_ARHD, "Unable to recreate archive header: %s\n", /* p1 = filename */
    BAD_COMBO_LS, "-l and -s are incompatible options\n",  /* no p1 */ 
    BAD_COMBO_XR, "-x and -r are incompatible options\n",  /* no p1 */ 
#endif /* STR960 */

#ifdef NAM960
	LST_SECN,"Unable to build list of section names for %s\n",
	ALST_SEC,"Unable to build list of section names for %s[%s]\n",
	NO_PROSY,"Unable to process symbol table (bad format) %s\n",
	NO_REOPN,"Unable to open %s for additional processing\n",
	NO_SYMBS,"No symbols in file %s\n",
	ANO_SYMB,"No symbols in file %s[%s]\n",
	OX_EXCLU,"-o and -x are mutually exclusive\n",
	VN_EXCLU,"-v and -n are mutually exclusive, -n assumed\n",
#endif /* NAM960 */
#ifdef ARC960
	ACT_OONE,"Only one control allowed\n", /* no p1 */
	ACT_QUAL,"A control must be specified\n", /*no p1 */
	ARC_MALF,"Malformed archive at %ld\n", 		/* p1 = position */
	ARC_ORDR,"Internal error, archive %s is out of order\n", /* p1=arname */
	ARC_SEEK,"Unable to reposition in archive %s\n",/* p1 = archive name */
	BAD_ADDN,"File has same name as archive: %s\n",
	BAD_HEAD,"Bad header layout for %s: %s\n",	/* p1=arname p2=fname */
	BAD_SOFF,"Bad string table offset in file %s\n",/* p1 = filename */
	BAD_STRT,"Bad string table in file %s\n", 	/* p1 = filename */
	CPY_HEAD,"Error copying archive header\n", 	/* no p1 */
	CPY_POOL,"Error copying name pool to archive\n",/* no p1 */
	ER_NAMPO,"Error reading name pool\n", 		/* no p1 */
	GRW_STRT,"Cannot grow string table in archive %s", /* p1 = arname */
	IGN_POSN,"Position %s is ignored\n",		/* p1 = posname */
	LNG_NAME,"Internal error, long names not read\n",	/* no p1 */
	MAK_SYMB,"Unable to make symbol directory\n",		/* no p1 */
	MNY_SYMB,"Too many external symbols\n",		/* no p1 */
	NO_STRTB,"File %s missing string table\n",	/* p1 = filename*/
	NOT_ARCH,"File %s not in archive format\n",	/* p1 = archive name */
	NOT_FOND,"Archive member %s not found\n",	/* p1 = filename */
	NOT_OBJT,"File %.16s pretends to be an object file\n",/* p1=filename */
	NOT_POSN,"Position %s not found \n",	/* p1 = posname */
	BAD_FILE_FMT,"File %s not in supported file format \n",	/* p1 = filename */
	PHAS_ERR,"Archive header size conflict: %s[%s]\n",
	PRE_FIVE,"File %.16s in pre 5.0 format \n",	/* p1 = filename */
	SKP_STRN,"Cannot skip string table in %s: %s\n",/* p1=arname,p2=mname */
#endif /* ARC960 */
#ifdef PIX960
	ENDLIST,"Premature end of listing file (possible bad header)\n",	/* no p1 */
	ENDCOMM,"Missing '*/' end of comment\n",	/* no p1 */
	G12USED,"Register g12 in use, unable to proceed\n",	/* no p1 */
	LABL_BD,"Labels do not match in switch\n",	/* no p1 */
	BD_SWIT,"EOF before switch replace done\n",	/* no p1 */
	BD_CALLX,"Indexing not allowed in Callx PIC replace\n",	/* no p1 */
	BD_BX,"Indexing not allowed in BX PIC replace\n",	/* no p1 */
	BD_ASM,"Instruction not converted to PID/PIC format: %s\n",	/* no p1 */
#endif /* PIX960 */
#ifdef ASM960
	/*************** addr1.c ***************/
	AS_UNDEFTAG, 	"%s Undefined tag name referenced: %s\n",
	AS_DIMDEB, 	"%s Too many array dimensions for symbolic debug\n",
	AS_UNBSYM, 	"%s Unbalanced Symbol Table Entries - Too Many Scope Beginnings\n",
        AS_ILLTAG,      "%s Illegal structure, union, or enum tag\n",
	AS_AUXARRDIM,   "%s Too many array dimensions for symbolic debug\n",
	/*************** addr2.c ***************/
	AS_RELSYM, 	"%s Attempt to relocate improper symbol\n",
	AS_RELSYMSZ, 	"%s Attempt to relocate improperly-sized symbol\n",
	AS_DISPOVF, 	"%s CTRL displacement overflow, use extended instruction\n",
	AS_ILLEXP, 	"%s Illegal expression, symbols are external or not in same section\n",
	AS_COBRLAB, 	"%s Illegal use of COBR format with external label\n",
	AS_COBRISBR, 	"%s COBR cannot do inter-segment branch, use BRANCH_EXTENDED\n",
	AS_COBRDISPOVF, "%s COBR displacement overflow, use BRANCH_EXTENDED\n",
	/*************** code.c ***************/                                              
	AS_INIBSS, 	"%s Attempt to initialize bss\n",
	AS_NOSECTCONT,  "%s Section definition must define the section contents\n",
	AS_DUPSECT,     "%s Section name already defined\n",
	AS_MAXSECT,     "%s Too many sections defined\n",
	AS_SECTNAM,     "%s Section name too long\n",
        AS_SECTATTR,    "%s Sections with same name must have matching attributes\n",
	/*************** codeout.c ***************/
	AS_ILLCODEGEN, 	"%s Illegal code generation\n",	/* not a user error */
	AS_EROFIL, 	"%s Error writing output file\n",
	AS_TMPFIL, 	"%s Unable to open temporary file %s\n",
	AS_INVACT, 	"%s Invalid action routine\n",
	AS_NOSCOPEENDS, "%s Unbalanced Symbol Table Entries - Missing Scope Endings\n",
	/*************** expand1.c ***************/
	AS_TABOVF, 	"%s Table overflow: some span-dependent optimizations lost\n",
	AS_LABTABMALLOC,"%s Cannot calloc labels table for span-dependent optimization\n",
	AS_LABTABRALLOC,"%s Cannot realloc more labels table for span-dependent optimization\n",
	AS_SDITABMALLOC,"%s Cannot calloc sdi table for span-dependent optimization\n",
	AS_SDITABRALLOC,"%s Cannot realloc more sdi table for span-dependent optimization\n",
	/*************** gencode.c ***************/
	AS_ARCHINSTR, 	"%s Instruction (%s) not in target architecture\n",
	AS_BRTNOOK, 	"%s 'branch-taken' not allowed in instruction (%s)\n",
	AS_INSFORUNK, 	"%s Internal error:  instruction format unknown.\n",
	AS_BADMEMTYP, 	"%s Improper MEM-type operator: %s\n", 
	AS_REGREQ, 	"%s %s must be a register.\n", 
	AS_NOSFRREG, 	"%s %s must not be a special function register.\n", 
	AS_BADALGNDST,	"%s Unaligned register destination\n",
	AS_REGASMEMOP, 	"%s Register used as memory operand.\n",
	AS_ILLUFLTREG, 	"%s Illegal use of floating-point register.\n",
	AS_MEMISABS, 	"%s Memory is absolute address\n",
	AS_ABASEINVIR, 	"%s Base register not allowed in virtual instruction.\n",
	AS_SYNTAX, 	"%s Syntax error\n",
#ifdef CA_ASTEP
	AS_CALLXOVF, 	"%s callx overflow condition may crash 80960CA A-step processor\n",
#endif
	AS_NOIDXBYT, 	"%s No indexing for %d-byte operands\n",
	AS_VFMTNODISP, 	"%s Internal error, V-format instruction w/out disp. should be MEMA\n",
	AS_NOLITERAL, 	"%s %s cannot be literal\n", 
	AS_ARCHNOSFR, 	"%s Special fun. reg. not allowed in this architecture\n",
	AS_UNALGNREG, 	"%s Unaligned register\n",
	AS_ILL_LIT, 	"%s Illegal literal in floating-point operation.\n",
	AS_ILLFLIT, 	"%s Illegal floating-point literal\n",
	AS_LITNOINT, 	"%s Illegal literal, integer expected.\n",
	AS_ILLLITVAL, 	"%s Illegal literal value, must be 0..31\n",
	AS_REGORLITREQ, "%s %s must be register or literal\n", 
	AS_BADPROPREG, 	"%s Internal error, bad property for REG formal %s\n",
	AS_BADCTRLTARG,	"%s Target of CTRL instruction must be a label\n",
	AS_CTRLDISPOVF, "%s CTRL displacement overflow, use extended instruction\n",
	AS_ILLSFRUSE, 	"%s Illegal use of special function register as src1\n",
	AS_INTERNALCOBR,"%s Internal error for COBR instructions!\n",
	AS_BADCOBRTARG,	"%s Target of COBR-format instruction must be a label\n",
	AS_COBRDISPOVF, "%s Displacement too large for COBR format\n",
	/*************** obj.c ***************/
	AS_OPENTEMP, 	"%s Unable to Open Temporary File\n",
	AS_RELSYMBAD, 	"%s Reference to symbol not in symbol table\n",
	AS_UNKSYM, 	"%s Unknown symbol in symbol table\n",
	AS_SYMSTKOVF, 	"%s Symbol Table Stack Overflow\n",
	AS_SCOPEENDS, 	"%s Unbalanced Symbol Table Entries - Too Many Scope Ends\n",
	AS_UNKLEAFPROC,	"Static leafproc %s not defined in this module\n",
	/*************** pass0.c ***************/      
	AS_IGNARGS,  	"Arguments after %s ignored\n", 
	AS_NOI960INC, 	"No include directory given.\n",
	AS_MAXI960INC,	"%d Include directories allowed.\n",
	AS_NOOUTFILE,	"No output filename given.\n",
	AS_LISTSRC,  	"Cannot list to a source file\n",
	AS_LSTOVWSRC,	"List file (%s) would overwrite source.\n",
	AS_OUTOVWSRC,	"Output (%s) would overwrite source.\n",
	AS_NOMPP,    	"Option for calling macro preprocessor not supported\n",
	AS_MPPNOTFND,	"Unable to find %s\n", 
	AS_CMDTOOLNG,	"%s command invocation line too long\n", 
	AS_BADCLI,   	"CLI must be DCL to use %s\n",
	AS_MPPFAIL,  	"%s failed\n",
    	AS_ILLPIXARG,   "%s Illegal PIC or PID argument\n",
	AS_OUTOVWPROT,  "Output (%s) would overwrite protected file.\n",
	AS_LSTOVWPROT,  "Listing (%s) would overwrite protected file.\n",
	AS_NOTIMPLEM,   "Listing feature has not yet been implemented.\n",
	/*************** pass1.c ***************/
	AS_INPUTFILE, 	"%s Unable to open input file: %s\n",
	AS_TXTTEMPFILE, "%s Unable to open temporary (text) file\n",
	AS_DATATEMPFILE,"%s Unable to open temporary (data) file\n",
	AS_TEMPLISTFILE,"%s Unable to open temporary (listing) file\n",
	AS_WRITEERR, 	"%s Trouble writing; probably out of temp-file space\n",
	/*************** pass2.c ***************/
	AS_OUTPUTFILE, 	"%s Unable to Open Output File\n",
	AS_TMPSYMFILE, 	"%s Unable to Open Temporary (symbol) File\n",
	AS_TMPLNNOFILE,	"%s Unable to Open Temporary (line no) File\n",
	AS_TMPRELFILE, 	"%s Unable to Open Temporary (rel) File\n",
	AS_TMPGSYMFILE,	"%s Unable to Open Temporary (gbl sym) File\n",
	AS_LISTINGFILE, "Unable to open listing file %s\n",
	AS_IGNOREI, 	"Interactive option ignored, input file is %s\n",
	/*************** symbols.c ***************/
	AS_SYMTABOVF, 	"%s Symbol table overflow\n",
	AS_HASHOVF, 	"%s Hash table overflow\n",
	AS_DUPINSTRTAB, "%s Duplicate instruction table name\n",
	AS_STRTABRALLOC,"%s Cannot realloc memory for string table\n",
	AS_STRTABMALLOC,"%s Cannot malloc memory for string table\n",
	AS_SYMTABRALLOC,"%s Cannot realloc memory for symbol table\n",
	AS_SYMTABMALLOC,"%s Cannot malloc memory for symbol table\n",
	AS_SYMHASHRALLOC,"%s Cannot realloc memory for symbol hash table\n",
	AS_SYMHASHMALLOC,"%s Cannot malloc memory for symbol hash table\n",
	/*************** symlist.c ***************/
	AS_ALLOC, 	"%s Cannot allocate storage\n",
	/*************** parse.y ***************/
	AS_MULTDEFLAB, 	"%s Multiply defined label\n",
	AS_ILLNUMLAB, 	"%s Illegal numeric label\n",
	AS_ASMABORTED, 	"%s Assembly aborted\n",
	AS_ILLSPCSZEXP, "%s Illegal .space size expression\n",
	AS_ILLREPCNT, 	"%s Illegal replication count\n",
	AS_ILLSIZE, 	"%s Illegal size\n",
	AS_ILLFILLVAL, 	"%s Illegal fill value\n",
	AS_BSSNOTABS, 	"%s .bss size not absolute\n",
	AS_BSSALGNNOABS,"%s .bss alignment not absolute\n",
	AS_ILLBSSSZ, 	"%s Illegal .bss size\n",
	AS_ILLBSSALGN, 	"%s Illegal .bss alignment\n",
	AS_MULDEFBSSLAB,"%s Multiply defined .bss label\n",
	AS_ILLEXPINSET, "%s Illegal expression in .set\n",
	AS_SYMDEF, 	"%s Unable to define symbol\n",
	AS_MULDEFLAB, 	"%s Multiply defined label\n",
	AS_UNDEFLAB, 	"%s Undefined symbol\n",
	AS_ILLCOMMSZ, 	"%s Illegal .comm size expression\n",
	AS_ILLREDEFSYM, "%s Illegal attempt to redefine symbol\n",
	AS_ILLALGNEXP, 	"%s Illegal .align expression\n",
	AS_ILLORGEXP, 	"%s Illegal .org expression\n",
	AS_ILLORGEXPTYP,"%s Incompatible .org expression type\n",
	AS_ABSORIGIN, 	"%s Usage of absolute origins\n",
	AS_DECVALDOT, 	"%s Cannot decrement value of '.'\n",
	AS_ORGGTLIMIT, 	"%s Origin value greater than limit\n",
	AS_ONEFILE, 	"%s Only one .file directive allowed\n",
	AS_LNARGBAD, 	"%s Nonsensical .ln argument -- using value of 1\n",
	AS_UNALGNVADDR, "%s Unaligned virtual address\n",
	AS_BADSCALEF, 	"%s Scale factor must be 1, 2, 4, 8, or 16\n",
	AS_ILLFLTEXP, 	"%s Illegal floating point expression\n",
	AS_ILLFLTSZ, 	"%s Illegal floating point size\n",
	AS_ILLFIXPTEXP, "%s Illegal fixed point expression\n",
	AS_ILLBITFLDEXP,"%s Illegal bit field expression\n",
	AS_RELBITFLDEXP,"%s Cannot relocate bit field expression\n",	
	AS_BITFLDXBNDRY,"%s Bit field crosses data boundary\n",
	AS_ILLEXPOP, 	"%s Illegal expression operand\n",
	AS_ILLEXP, 	"%s Illegal expression\n",
	AS_DIVBYZERO, 	"%s Division by zero\n",
	AS_ILLEXPOPS, 	"%s Illegal expression operands\n",      
	AS_BADINSTMNEU, "%s Invalid instruction mnemonic: %s\n",
	AS_BADNUMLAB, 	"%s Numeric label not in range 0..9\n",
	AS_LABUNDEF, 	"%s Label %db undefined\n",
	AS_DUPDECPT, 	"%s More than one decimal point\n",
	AS_ILLCHARCONST,"%s Illegal character constant\n",
	AS_FLABUNDEF, 	"%s Label %df undefined\n",
	AS_BADSTRING, 	"%s String not terminated\n",
	AS_ILLINCHAR, 	"%s Illegal input character, (decimal) %d\n",
	AS_BADFILLSZ, 	"%s The .fill size argument must be 1..4 bytes\n",
	AS_LOADDPCONST, "%s Cannot load a double precision constant\n",
	AS_TOOMANYERRS,	"Too many errors - Assembly aborted\n",
	AS_LOADXPCONST, "%s Cannot load an extended precision constant\n",
	AS_NOI_NOINFILE,"%s Must specify either -i switch or input file name\n",
	AS_LONGFILE,  "%s .file directive value truncated to 134 characters\n",
	AS_SYSPROC_IDX, "%s System Procedure Table index out of range: %d\n",
	AS_TOOBIGSIZE,	"%s \n	.size argument > 0x10000, symbol size info shown will be incorrect\n",
	AS_TOOBIGDIM,	"%s \n	.dim argument > 0x10000, symbol size info shown will be incorrect\n",
	AS_BADSECTTYP,	"%s Section type must be t, d, or b\n",
	AS_BADSECTATTR, "%s Section attribute must be m\n",
	AS_BADSECTNAME, "%s %s is not a section\n",
	/*************** unused.c ****************/
        AS_ERRUNUSED,   "%s Internal error processing unused symbols\n",
        AS_UNUSEDSYMS,  "%s Can not reduce symbol table - unused symbols remain\n",
	/*************** statistics **************/
        AS_STAT_OPEN,   "%s Can not open statistics file: %s\n",
#endif /* ASM960 */
/* ROM960 */
#ifdef ROM960
	ROM_NO_CMDS,"No ROM960 commands in configuration file %s\n", /* p1 =
						config filename */
	ROM_SILLY_ARG,"Invalid substitution argument number %s\n", /* p1 =
						invalid value */
	ROM_UNSUP_EVA,"Unsupported function on EVA host\n",
	ROM_SILLY_CMD,"Unrecognized command %s\n", /* p1 = unrecognized
						string */
	ROM_WARN_SPLIT,"Large file %s will take a long time to split\n", /* p1 =
						name of large file */
	ROM_WARN_PERMUTE,"Large file %s will take a long time to permute\n",
						/* p1 = name of large file */
	ROM_ADDR_BOUNDS,"Address permutation gives out-of-bounds address\n",
	ROM_PATCH_WRITE,"Unable to write patch to image file\n",
	ROM_READ_IMAGE,"Unable to read image into memory\n", 
	ROM_TOO_MANY_SECTS,"%d sections found, only %d allowed\n", /* p1 =
						max sects allowed, p2 =
						number sects found */
	ROM_READ_TEXT,"Unable to read header for %s section\n", /* p1 == section name. */
	ROM_NO_CLOSE,"Unable to close file %s\n",	/* p1 = filename */
	ROM_NO_SECTS,"File %s contains no sections\n", /* p1 = filename */
	ROM_PAD_FSEEK,"Unable to place file pointer in %s to write section %s\n",
						/* p1 = image filename,
						   p2 = section name */
	ROM_NO_IMAGE_WRITE,"Unable to write to image file\n",
	ROM_UNABLE_2_SIZE,"Unable to prepare destination file %s on DOS\n",
						/* p1 = filename */
	ROM_UNEXPECTED_SECTYPE,"Expected %s section to be %s, found %s\n",
						/* p1 = order word (first,
						   second, third), p2 =
						   expected section type,
						   p3 = found section type */
	ROM_MISSING_SECTION,"Unable to find section #%d\n", /* p1 = section
						count */
	ROM_SECTION_ORDER,"Sections out of order in %s\n", /* p1 = filename */
	ROM_CKSUM_ADDRS,"Start address for checksum is greater than end address\n",
	ROM_CKSUM_TARGET,"Target for checksum is within checksummed range\n",
	ROM_ROM_WIDTH,"ROM width must be even multiple of byte size\n",
	ROM_IMAGE_SIZE,"Image size is more than twice as big as ROM space\n",
	ROM_SHORT_READ,"%#lx bytes asked for, only %#lx bytes read\n", /* p1 = number of bytes asked for, p2 = number actually read */
	ROM_MORE_TO_GO,"File is larger than specified image length %#lx\n", /* p1 = number of bytes read */
	ROM_BAD_ROMLEN,"Romlength %#x is invalid\n",	/* p1 = input romlength */
	ROM_BAD_ROMWIDTH,"Romwidth %#x is invalid\n",	/* p1 = input romwidth */
	ROM_BAD_ROMCOUNT,"Romcount %#x is invalid\n",	/* p1 = input romcount */
	ROM_BAD_MEMLEN,"Memlen %#x is invalid\n",	/* p1 = input memlength */
	ROM_BAD_MEMWIDTH,"Memwidth %#x is invalid\n",	/* p1 = input memwidth */
	ROM_MEMW_LT_ROMW,"Memwidth %#x < romwidth %#x\n",	/* p1 = memwidth, p2 = romwidth */
	ROM_BAD_WIDTH_RATIO,"Memwidth must be even multiple of romwidth \n",
	ROM_GET_INT_FAILED,"Integer value expected for prompt '%s'\n",  /* p1 = the prompt passed to get_int() */
	ROM_BAD_IHEX_MODE,"Only MODE16 and MODE32 are acceptable values for ihex mode\n",
	ROM_CKSUM_STRETCH,"%s 0x%lx is beyond current end of file\n", /* p1 = describes address, p2 = actual address */
	ROM_WARN_OVERLAP,"Section %s overlaps section %s\n",
	ROM_WARN_WRAP_ADDR,"Image size has exceeded 0xffffffff\n",
	ROM_NOT_OBJECT,"File %s is not an object file \n", /* p1 is file */
	ROM_NO_HEX_RECORDS,"No hex records found in %s \n", /* p1 is file */
	FILE_SEEK_ERROR,"Unable to seek to section %s\n",
	SECT_HAS_BAD_PADDR_FILE_OFFSET,"Section has bad paddr file offset %s\n",
	FILE_WRITE_ERROR,"File write error for section %s\n",
#endif
#ifdef MPP960
	/* m4.c */
	MAXTOKSIZ,  "more than %d chars in word\n",        
	ASTKOVF,    "more than %d items on argument stack\n",
	QUOTEDEOF,  "EOF in quote\n", 
	COMMENTEOF, "EOF in comment\n",
	ARGLSTEOF,  "EOF in argument list\n",
	MAXI960INC,  "only %d include directories allowed\n",
	/* m4macs.c */
	MAXCOMMENT, "comment marker longer than %d chars\n",
	MAXQUOTE,   "quote marker longer than %d chars\n", 
	BADMACNAM,  "bad macro name\n",
	MACDEFSELF, "macro defined as itself\n",
	INVEXPR,    "invalid expression\n",
	INCNEST,    "input file nesting too deep (9)\n",
	/* m4.h */
	PSHBKOVF,   "pushed back more than %d chars\n",
	ARGTXTOVF,  "more than %d chars of argument text\n",
#endif /* MPP960 */


#ifdef CVT960

	/* messages originating in cvt960.c */
	UNEXPECTED_SECTYPE, 
	   "COFF section id %d has invalid section type %s\n",
	DATACONFLICT, 
"COFF section id %d is type COPY; symbol/data conflicts possible in output\n",
	SYMCONFLICT,
       "COFF section id %d is type %s; symbol conflicts possible in output\n",
        TOO_MANY_SCNS, "Only (%d) sections are allowed\n",
	NO_FILE_SYMS, 
"No public/debug info produced; no .file symbols in COFF symbol table\n",
	INVTAGNDX,
"One or more COFF symbols (index %d) have invalid tag index %d.\n",
	ARGBLKARG,
"COFF argument symbol at index %d is ignored; addressing path too complicated for HP/MRI 695.\n",
	ILLREGVAL,
"Illegal register value (%d) at symbol index %d.\n",
	 PIX_NOTRANS,
"COFF file contains PIX symbols; PIX flags disappear in 695 translation.\n",
	FILESAME,
"Input file '%s' is the same as output file '%s'.\n",
	BADAUX,
"Corrupt COFF symbol table; symbol table entry %s has %d aux entries.\n",

	/* messages originating in hp695_960.c */
	HPER_SECID, "internal error: section id out of range (%d)\n",
        HPER_DUPSEC, "internal error: duplicate section definition (%d)\n",
        HPER_BADSTYPE, "internal error: bad section type (%d)\n",
        HPER_UNKSEC, "internal error: unknown section reference (%d)\n",
        HPER_ACODE, "internal error: erroneous architecture code (%d)\n",
        HPER_BADCHAR, "internal error: erroneous character written (%d)\n",
        HPER_BADHPTYPE, "internal error: inappropriate HP/MRI 695 type (%d)\n",

	HPER_UNKDBLK, "internal error: attempted to open or close invalid debug block type (%d)\n",
	HPER_BADATT, "internal error: tried to write a symbol with erroneous attribute (%d)\n",
	HPER_BADHITYPE, "internal error: tried to define an invalid or unsupported high level type\n",
	HPER_RECOPMOD, "internal error: tried to define a module within a module\n",
	HPER_TYDEFUSE, "internal error: tried to define a type when a reference was expected\n",
	HPER_CLSDMOD, "internal error: tried to put debug information in an unopened module\n",
	HPER_FCLOSE, "internal error: tried to close a function in the context of block type (%d)\n",
	HPER_NOTASMATT, "internal error: attempt to put a non-assembler attributed symbol into asm block\n",
        HPER_SYNCNTXT, "internal error: attempt to put high level symbol in wrong block type (%d)\n",
        HPER_UNK, "unidentified internal error",

	/* and even more cvt960 errors */
	CVT_BAD_ASW,"%s: Bad ASW in 695 file '%s' at offset %#x.\n",
	CVT_MV_BACK,"%s: Skip/Retrograde movement of PC in 695 file '%s' at offset %#x.\n",
	CVT_NO_MEM,"%s: Cannot allocate memory.\n",
	CVT_E_PT_ERR,"%s: Entry points unequal: COFF file '%s' = %#x; 695 file '%s' = %#x.\n",
	CVT_BAD_LD_ITEM,"%s: Bad 695 load item in '%s' at offset %#x.\n",
	CVT_E_PT_IMBAL,"%s: 695 file '%s' has an entry point, but COFF file '%s' does not.\n",
	CVT_D_SEC_SEEK,"%s: Could not seek to COFF data for section %d at offset %#x in '%s'.\n",
	CVT_BAD_TRAILER,"%s: Bad 695 trailer part in '%s' at offset %#x.\n",
	CVT_AS_RD_ERR,"%s: Read error in AS record of '%s' at offset %#x.\n",
	CVT_BAD_695_ARCH,"%s: 695 file is for unsupported architecture '%s'.\n",
	CVT_ADDR_MISSING,"%s: '%s' omits data which '%s' includes at 80960 address %#x.\n",
	CVT_E_PT_MISSING,"%s: 695 file '%s' did not have an entry point",
	CVT_STRING_BAD,"%s: Non-printing string char %#x at offset %#x in 695 file %s.\n",
	CVT_OFFSET_MISSING,"%s: Required number in 695 file '%s' at offset %#x is missing.\n",
	CVT_NO_CMP_FILE,"%s: No file to compare with '%s'\n",
	CVT_NOT_COFF,"%s: Neither '%s' nor '%s' are COFF files.\n",
	CVT_NOT_695,"%s: '%s' is not a 695 file.\n",
	CVT_UNSUP_695_SEC,"%s: Unsupported 695 section type in section %d of '%s': (%#x)(%#x)(%#x).\n",
	CVT_BAD_STRING,"%s: 695 file '%s' has bad string at offset %#x.\n",
	CVT_NO_OPEN_695,"%s: Could not open 695 file '%s'.\n",
	CVT_TRAILER_SEEK,"%s: Could not seek to 695 trailer part of '%s' at offset %#x.\n",
	CVT_SECTION_SEEK,"%s: Could not seek to 695 section part of '%s' at offset %#x.\n",
	CVT_DATA_SEEK,"%s: Could not seek to 695 data part at offset %d\n",
	CVT_CATASTROPHE,"%s: Internal error: undefined error error = %d./n",
	CVT_ERRS_FOUND,"One or more pairs of files had errors.\n",
	CVT_WARNS_FOUND,"One or more pairs of files had warnings.\n",
	CVT_WRITE_ERR,"cvt960: ferror on output stream.\n",
	CVT_HP_ABORT,"\"hp695_960.c\".hp_abort: Aborting.  output file '%s' left for debugging.\n",
	CVT_SYMENT_SEEK,"Cannot seek to symentry %l\n",
	CVT_DATA_CLASH,"%s: Data in 695 file '%s' and COFF file '%s' clash in 80960 address range [%#x..%#x].\n",
	CVT_NO_SUCH_SEC,"%s: Section index %d in data part of '%s' corresponds to no declared section.\n",

#endif /* CVT960 */
	END_ERR_TBL,"END TABLE",
};

/*
* error_out() - log error message to STDERR
*	tool	= the tool name the error is in
*	num 	= the Error number	       
*	sub_num	= the Sub Error number (used for debugging)
*	p1 - p5 = Any additional items such as the file name
*/
#ifdef MWC
error_out(the_tool)
char *the_tool;
#else
error_out( va_alist)
va_dcl
#endif
{
	va_list args;
	char *tool;
	unsigned int num, sub_num;
 	int i = 0;

#ifdef MWC
	va_start(args, the_tool);
	tool = the_tool;
#else
	va_start(args);
	tool =   va_arg(args, char *);
#endif
	num =    va_arg(args, unsigned int);
	sub_num = va_arg(args, unsigned int);
        
	fflush(stdout);
	/* Find the error in the table or end of table */
	while (errors[i].num != num && errors[i].num != END_ERR_TBL) i++;

	/* if it was end of table return output message and return error */
	if (errors[i].num == END_ERR_TBL) {
		fprintf(STDERR,"%s: ERROR %04u.%02u -- is an Unknown Error\n", 
				tool, num, sub_num);
		fflush(STDERR);
		return(ERR_RETURN);
	}

#if defined(SOLARIS)
	/* FIXME: Workaround for broken vfprintf() on sol-sun4 */
	vsprintf(sol_tmpbuf, errors[i].msg, args);
#endif

	/* Put out error message */
	fprintf(STDERR,"%s: ERROR %04u.%02u -- ", tool, num, sub_num);
#if defined(unix) && defined(vax) 
	_doprnt(errors[i].msg, args, STDERR);
#else
#if defined(SOLARIS)
	/* FIXME: Workaround for broken vfprintf() on sol-sun4 */
	fprintf(STDERR, sol_tmpbuf);
#else
	vfprintf(STDERR, errors[i].msg, args);
#endif
#endif
	fflush(STDERR);
	return(OK_RETURN);
} /* end routine error_out */

/*
* error_log() - log error to specified file (for asm960) 
*	first 	= the true line number
*	num 	= the Error number 			   
*	sub_num	= the Sub Error number (used for debugging)
*	p1 - p5 = Any additional items such as the file name
*/
#ifdef MWC
error_log(the_file)
FILE *the_file;
#else
error_log( va_alist)
va_dcl
#endif
{
	va_list args;
	FILE *fp;
	char *first;
	unsigned int num, sub_num;
 	int i = 0;

#ifdef MWC
	va_start(args, the_file);
	fp = the_file;
#else
	va_start(args);
	fp   =   va_arg(args, FILE *);
#endif
	first =  va_arg(args, char *);
	num =    va_arg(args, unsigned int);
	sub_num = va_arg(args, unsigned int);
        
	fflush(stdout);
	/* Find the error in the table or end of table */
	while (errors[i].num != num && errors[i].num != END_ERR_TBL) i++;

	/* if it was end of table return output message and return error */
	if (errors[i].num == END_ERR_TBL) {
		fprintf(fp,"%s ERROR %04u.%02u -- is an Unknown Error\n",
				first, num, sub_num);
		fflush(fp);
		return(ERR_RETURN);
	}

#if defined(SOLARIS)
	/* FIXME: Workaround for broken vfprintf() on sol-sun4 */
	vsprintf(sol_tmpbuf, errors[i].msg, args);
#endif

	/* Put out error message */
	fprintf(fp,"%s ERROR %04u.%02u -- ", first, num, sub_num);
#if defined(unix) && defined(vax) 
	_doprnt(errors[i].msg, args, fp);
#else
#if defined(SOLARIS)
	/* FIXME: Workaround for broken vfprintf() on sol-sun4 */
	fprintf(STDERR, sol_tmpbuf);
#else
	vfprintf(fp, errors[i].msg, args);
#endif
#endif
	fflush(fp);
	return(OK_RETURN);
} /* end routine error_log */

/*
* warn_out() - log warning message to STDERR
*	tool	= the tool name the error is in
*	num 	= the Error number	       
*	sub_num	= the Sub Error number (used for debugging)
*	p1 - p5 = Any additional items such as the file name
*/
#ifdef MWC
warn_out(the_tool)
char *the_tool;
#else
warn_out( va_alist)
va_dcl
#endif
{
	va_list args;
	char *tool;
	unsigned int num, sub_num;
 	int i = 0;

#ifdef MWC
	va_start(args, the_tool);
	tool = the_tool;
#else
	va_start(args);
	tool =   va_arg(args, char *);
#endif
	num =    va_arg(args, unsigned int);
	sub_num = va_arg(args, unsigned int);
        
	fflush(stdout);
	/* Find the error in the table or end of table */
	while (errors[i].num != num && errors[i].num != END_ERR_TBL) i++;

	/* if it was end of table return output message and return error */
	if (errors[i].num == END_ERR_TBL) {
		fprintf(STDERR,"%s: ERROR %04u.%02u -- is an Unknown Error\n", 
					tool, num, sub_num);
		fflush(STDERR);
		return(ERR_RETURN);
	}

#if defined(SOLARIS)
	/* FIXME: Workaround for broken vfprintf() on sol-sun4 */
	vsprintf(sol_tmpbuf, errors[i].msg, args);
#endif

	/* Put out error message */
	fprintf(STDERR,"%s: WARNING %04u.%02u -- ", tool, num, sub_num);
#if defined(unix) && defined(vax) 
	_doprnt(errors[i].msg, args, STDERR);
#else
#if defined(SOLARIS)
	/* FIXME: Workaround for broken vfprintf() on sol-sun4 */
	fprintf(STDERR, sol_tmpbuf);
#else
	vfprintf(STDERR, errors[i].msg, args);
#endif
#endif
	fflush(STDERR);
	return(OK_RETURN);
} /* end routine warn_out */

/*
* warn_log() - log warning to specified file 
*	first 	= the true line number
*	num 	= the Error number 			   
*	sub_num	= the Sub Error number (used for debugging)
*	p1 - p5 = Any additional items such as the file name
*/
#ifdef MWC
warn_log(the_file)
FILE *the_file;
#else
warn_log( va_alist)
va_dcl
#endif
{
	va_list args;
	FILE *fp;
	char *first;
	unsigned int num, sub_num;
 	int i = 0;

#ifdef MWC
	va_start(args, the_file);
	fp = the_file;
#else
	va_start(args);
	fp   =   va_arg(args, FILE *);
#endif
	first =  va_arg(args, char *);
	num =    va_arg(args, unsigned int);
	sub_num = va_arg(args, unsigned int);
        
	fflush(stdout);    

	/* Find the error in the table or end of table */
	while (errors[i].num != num && errors[i].num != END_ERR_TBL) i++;

	/* if it was end of table return output message and return error */
	if (errors[i].num == END_ERR_TBL) {
		fprintf(fp,"%s ERROR %04u.%02u -- is an Unknown Error\n",
					first, num, sub_num);
		fflush(fp);
		return(ERR_RETURN);
	}

#if defined(SOLARIS)
	/* FIXME: Workaround for broken vfprintf() on sol-sun4 */
	vsprintf(sol_tmpbuf, errors[i].msg, args);
#endif

	/* Put out error message */
	fprintf(fp,"%s WARNING %04u.%02u -- ", first, num, sub_num);
#if defined(unix) && defined(vax) 
	_doprnt(errors[i].msg, args, fp);
#else
#if defined(SOLARIS)
	/* FIXME: Workaround for broken vfprintf() on sol-sun4 */
	fprintf(STDERR, sol_tmpbuf);
#else
	vfprintf(fp, errors[i].msg, args);
#endif
#endif
	fflush(fp);
	return(OK_RETURN);
} /* end routine warn_log */

/* Routine to output a usage message for each tool			*/
usage()
{
	static char usage_message[]=
#ifdef ARC960
		"Usage: arc960 -{d|m|p|r|t|u|x}[-clsuvV] [-abi posname] archive [name ...]";
#endif
#ifdef ASM960
		"Usage: asm960 [-nrRVWmD] [-p{c|d|b}] [-o file] [-Lfile] [-Aarch] [-Iinclude] {-i|file ...}";
#endif
#ifdef COF960
		"Usage: cof960 [-rvV] file ...";
#endif
#ifdef DIS960
		"Usage: dis960 [-aosLV] [-Aarch] [-d sec] [-D sec] [-F func] [-t sec] file ...";
#endif
#ifdef DMP960
		"Usage: dmp960 [-acfghloprstxV] [-d num [-D stop]] [-n name] [-Tstart,stop]\n\t\t [-z name[,num] [-Zstop]] file ...";
#endif
#ifdef GEN960
		"Usage: gen960 [ [-b file] [-c file] [-d dir] [-f maketarg] [-m file] [-o file]\n\t\t[-T env] [-V] [ [-p process file ...] [-s nnn] [-i nnn] ] ]";
#endif
#ifdef LNK960
		"Usage: lnk960	[config_file ...] [-CFmM{r|s}txV] [-p{c|d|b}] [-Ttarg] [-Aarch] [-u sym]\n\t\t[-e ss] [-f bb] [-O{b|s}] [-Ldir ...] [file ...] [-lx ...]";
#endif
#ifdef MPP960
		"Usage: mpp960 [-esV] [-o file] [-Bint] [-D nam[=val]] [-Hint] [-llib]\n\t\t[-Sint] [-Tint] [-Unam]";
#endif
#ifdef NAM960
		"Usage: nam960 [-o|x|d] [-hvnpefurVT] file ...";
#endif
#ifdef PIX960
		"Usage: pix960 [-LVcd] [-ofilename] file";
#endif
#ifdef ROM960
		"Usage: rom960 [-Vh | v960] [config_file [script_arg[, script_arg ...]]]";
#endif
#ifdef SIZ960
		"Usage: siz960 [-{d|f|o|x|V}] file ...";
#endif
#ifdef STR960
		"Usage: str960 [-{b|l|r|x|V}] file ...";
#endif
#ifdef CVT960
		"Usage: cvt960 [-{V|s|w}] [-i file] [-o file]";
#endif

	fprintf(STDERR,"\n%s\n",usage_message);
	fprintf(STDERR,"Use -h option to get help\n\n");
} /* end routine usage */
