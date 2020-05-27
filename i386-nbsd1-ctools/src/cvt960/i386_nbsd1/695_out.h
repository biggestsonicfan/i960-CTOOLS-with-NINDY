
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


/* Provides definitions for clients of the 695_out.c module.
 * Many of these definitions correspond to the HP/MRI 695 V4.0 spec. 
 */

#ifndef FILE
#include <stdio.h>
#endif

#define NOT_AN_ERROR	-1		/* return from hp_err when no error */

typedef struct {
			/* version information for a 695 producer */

   unsigned short version;
   unsigned short revision;
   char level;		/* 'a', 'b', etc.; '\0' means there is no level */
   
} HPTOOL_VERS;

typedef struct {
	/* A collection of all the input data needed by 
	 * the hp_create operation.
	 */
			
   char *client;	/* Name of client tool for error handling. */
   int  toolid;		/* 695-toolid from 695_out.h HPTOOL_* */
   HPTOOL_VERS version;	/* '9.9z' style version*/
   int  cpu;		/* F_ values from filehdr.h */
   char *lmodname;	/* load module name */
   char *outputname;	/* 695-format file to create */
   long timdat;		/* UNIX-format time & date stamp for COFF file */
   int  hostenv;	/* value in HPHOST_* */
   char *comline;	/* command line image; NULL if unused */
   char *comment;	/* 695 comment; NULL if unused */
} HP695_MODDESCR;

/*
 * For the purposes of the 960 tools, section ids are in range 1..0x7FFF.
 */
typedef short HPSECID;

typedef struct {
   HPSECID id;		/* section the MAUs are in */
   unsigned size;	/* number of MAUs */  
   FILE *stream;	/* stream to get them from */
} HPIMAGE_SEG;

/*
 * 695 tool id for CVT960 tool.
 */
#define	HPTOOL_CVT960	210
#define HPTOOL_UNK      0
#define HPTOOL_IC960    208
#define HPTOOL_ASM960   209

/* LOGADR960 is the type which holds a 960 logical address
 * (logical = physical for KA, KB, CA and JX).
 */
typedef unsigned long LOGADR960;


/*
 * HPHOST_* define host environment of object module translator.
 * Omitted environment codes which could also be supported are ms-dos and vms.
 */
#define HPHOST_695WRTR	-1	/* whatever 695 writer's env. is */
#define HPHOST_UNK	0	/* define environment to be unknown */
#define HPHOST_UNIX	3
#define HPHOST_HPUX	4

/*
 * HPE_* are input values for hp_set_exestat
 */
#define HPE_SUCCESS	0
#define HPE_WARNING	1
#define	HPE_ERROR	2
#define	HPE_FATAL	3

/*
 * HPABST_* are section-type inputs for hp_creabsect.
 */
#define	HPABST_CODE	((int)'P')
#define	HPABST_ROMDATA	((int)'R')
#define	HPABST_RWDATA	((int)'D')

/*
 * HPATN_* are ATN codes which, for a variable, represent the 
 * attributes, the blocks they can appear in.
 * These are the "n3" values of 695 Table 3-6, with omissions due
 * to unused features and features which are hidden by the 
 * 695_out.c interface.
 */

#define	HPATN_AUTO	1
#define	HPATN_REGVAR	2
#define HPATN_COMPSTAT	3
#define HPATN_EXTFUNC	4
#define HPATN_EXTVAR	5
#define	HPATN_SRCCOORD	7
#define	HPATN_COMPGLOBAL 8
#define	HPATN_VARLIFE	9
#define	HPATN_LOCKREG	10
#define HPATN_BASVAR	12
#define	HPATN_CONST	16
#define HPATN_ASMSTAT	19
#define HPATN_SFVNO	36
#define HPATN_OMFVNO	37
#define	HPATN_OMFTYP	38
#define	HPATN_OMFCASE	39
#define	HPATN_CREDTG	50
#define	HPATN_COMLINE	51
#define	HPATN_EXESTAT	52
#define	HPATN_HOSTENV	53
#define	HPATN_TOOLVNO	54
#define	HPATN_COMMENT	55
#define	HPATN_PMISC	62
#define HPATN_VMISC	63
#define HPATN_MMISC	64
#define	HPATN_MSTRING	65

/* HPMISC_* Are the miscellaneous-record codes */
#define HPMISC_COMPID  50
#define HPMISC_960CALL 63

/* HP960PROC_* are definitions from VMISC 63 80960 Call Opt in 695 Errata */
#define HP960PROC_LEAF 1
#define HP960PROC_SYS 2



typedef unsigned long HPTYPEHANDLE;	

/*
 * HPTYSTR_* define the contents of the HPHITYPE union, and are selected
 * by the HPHITY_* codes.
 */
typedef unsigned HPTYSTR_UNK;	/* size in MAUs */

/* MISCARGPTR is a type which carries information about an argument of
 * a misc record.  Such an argument may be either a string or a number.
 */
#define MSTRPTR pointer.to_string
#define MNUMPTR pointer.to_number
typedef struct {
   char is_string; /* != 0 selects the pointer.to_string member */
   union {
      char *to_string;
      unsigned long *to_number;
   } pointer;
} MISCARGPTR;

typedef struct {
				/* bitfield type */
   char is_signed;		/* != '\0' if signed */
   char has_basetype;		/* != '\0' if basetype exists */
   unsigned size;		/* in bits */
   HPTYPEHANDLE basetype;
   
} HPTYSTR_BITFLD;	

typedef struct tagmem_tag {
   /* Member of structure, union or enumeration.
    */
   char *name; /* of the member */
   unsigned long value; /* enum value, or struct/union mem offset */
   HPTYPEHANDLE type; /* of struct/union member or bitfield;
		       * not used for enum
		       */
} TAGMEM;

typedef  struct {
				/* Enum, struct or union tag */
   unsigned long size;		/* in MAUs */
   TAGMEM *member;		/* Array of members */
   unsigned long membcount;	/* Count of members */

} HPTYSTR_TAG;

typedef HPTYPEHANDLE HPTYSTR_PTR;

typedef struct {
				/* zero-based array (e.g. "C" array) */
   HPTYPEHANDLE membtype;
   int high_bound;		/* -1 => high bound unknown */
} HPTYSTR_CARR;


/* HPPAT_*; used in HPTYSTR_PROCWI, see Table A-1 note 3 */
#define HPPAT_UNKFRAME	(1<<0)	/* Set for dual-entry leaf procedure */
#define HPPAT_NEAR	(1<<1)	/* use is TBD */
#define HPPAT_FAR	(1<<2)	/* use is TBD */
#define HPPAT_REENT	(1<<3)
#define HPPAT_ROM	(1<<4)
/* bit 5 => PASCAL nested, unused */
#define HPPAT_NOPUSH	(1<<6)	/* probably always for 960; no 68000 style
				 * push mask.
				 */
#define HPPAT_INTERPT	(1<<7)	/* Use is TBD */
#define HPPAT_LEAFP	(1<<8)  /* leaf procedure (may be bal, or call&bal) */

/* We believe that the following attributes are always present in
 * functions produced by iC960.
 */
#define HPPAT_NORMCATTS	(HPPAT_ROM | HPPAT_REENT | HPPAT_NOPUSH)

#define HPUNKARGS -1 /* unknown arguments for PROCWI */

typedef HPTYPEHANDLE HPTYSTR_ARG;

typedef struct {
   /* Procedure with "compiler dependencies".
    * UPDATE: no support for frame types other than 0 (standard CALL frame);
    * LEAF procs called with BAL should designate unknown-frame bit in
    * the attribute field.
    */
   unsigned long attribute;	/* bitmask, see HPPAT_* for bit defs */
   HPTYPEHANDLE rtype;		/* Return type */
   int argcnt;			/* -1 if unknown, see 695 Table A-1 4of4 */
   HPTYSTR_ARG *arglist;

	/* Level and father fields are unsupported pending the need
	 * to support nested-procedure languages like PASCAL and Ada.
	 */
} HPTYSTR_PROCWI;

typedef  struct {
	/* attributes of static assembler variable */
   unsigned numelems;	/* number of elements of the type in HPSYM */
   char globflag;	/* nonzero => global */

} ASMSTAT_ATTRS;

#define BASVAR_STATMEM 	0
#define BASVAR_REG 	1
#define BASVAR_BANK 	2
#define BASVAR_PTR	3
#define BASVAR_INDIR 	4
/*
 * BASVAR_ATTRS  ATN parameters for based variables.
 */
typedef struct {
   LOGADR960 offset; /* based variable offset */
   short control; /* an enumeration; see 695 page 33 */
   char global; /* nonzero if global */
} BASVAR_ATTRS;

typedef struct {
		/* 695 table 3-6 */
   char atnid;		/* "n3" of table 3-6; in (1..65).  See HPATN_* */
   union {
	/* members are selected by the atnid field.
         * See 695 Table 3-6.
	 */

	/* atnid = HPATN_AUTO; automatic variable stack offset*/
      unsigned auto_offset;	

	/* atnid = HPATN_REGVAR => register variable;  
	 * regid is an index from 695 80960 architecture appendix
	 * (or possibly a special-section offset)
         */
	/* atnid = HPATN_LOCKREG; locked register variable;  */
      unsigned regid;		

	/* atnid = HPATN_COMPSTAT; compiler defined static */
   /* -- no attrs -- */		

	/* atnid = HPATN_EXTFUNC; external function; 
	 * this atnid value is not actually passed
	 * to 695_out.c; instead hp_openblock defines this symbol.
   	 */
   /* -- no attrs -- */	

	/* atnid = HPATN_EXTVAR; external variable definition */
   /* -- no attrs -- */

	/* atnid = 6; reserved */

	/* atnid = HPATN_SRCCOORD; line number */
      unsigned srccoord[4]; /* [0] is line, [1] is column, [2,3] reserved */

  	/* atnid = HPATN_COMPGLOBAL; compiler global */
   /* -- no attrs -- */

	/* atnid = HPATN_VARLIFE; variable lifetime; 
	 * presently unused by 960 tools 
	 */

	/* atnid = 11; reserved for FORTRAN */

	/* atnid = 12; HPATN_BASVAR; based variable */
      BASVAR_ATTRS base;

	/* atnid = 13..15 reserved */

	/* atnid = HPATN_CONST; constant; presently unused by 960 tools */

	/* atnid = 17..18 reserved */

	/* atnid = HPATN_ASMSTAT; static assembler variable */
      ASMSTAT_ATTRS asmstat;

	/* atnid = 20..35 reserved */

	/* atnid = HPATN_VNO; version number of source files; 
	 * unused by 960 tools 
	 */

	/* atnid = HPATN_OMFVNO, HPATN_OMFTYP, HPATN_OMFCASE;
	 * used to build AD extension part;
	 * attributes for these are not defined here but emitted by
	 * 695_out.c during the hp_create operation.
	 */

	/* atnid = 
	 * HPATN_CREDTG, HPATN_COMLINE, HPATN_EXESTAT, HPATN_HOSTENV, 
	 * HPATN_TOOLVNO, HPATN_COMMENT;
	 * used to build Environmental Part;
	 * attributes for these are not defined here but emitted by
	 * 695_out.c during the hp_create operation.
	 */

	/* atnid = HPATN_PBMISC, HPATN_VMISC, HPATN_MMISC,HPATN_MSTRING;
	 * unused at the 695_out.c interface.
	 */
   
   } att;	
} HPSYMATT;

/* HPHITY_* are 695 high level type classes which index into the 695 
 * Table A-1.  They are integer codes which happen to correspond to
 * character "mnemonics".   They determine the value of the HPHITYPE 
 * "data" field.
 * Note:
 *	* Codes for Pascal, Ada and FORTRAN have been omitted. 
 *	* We omitted the 'G' type because the 695 spec. suggests
 *        it is a fossil.  
 *	* We omitted the 'O' type because it does not map onto
 * 	  the 960 as specified in 695.  We might reconsider this if we need
 *	  to support "short pointers".
 *	* We supplied 'T' and 'X' because we might use them in the future,
 *	  although we don't support them now.
 */

#define	HPHITY_UNK	'!'
#define	HPHITY_CENUM	'N'
#define	HPHITY_PTR	'P'
#define HPHITY_STRUCT	'S'
#define	HPHITY_TYPENAM	'T'	/* not used; might be in future for typedefs*/
#define HPHITY_UNION	'U'
#define	HPHITY_PROCWO	'X'	/* not used; proc w/o compiler dependencies */
#define HPHITY_PROCWI	'x'	/* proc with compiler dependencies */
#define	HPHITY_CARR	'Z'
#define	HPHITY_BITFLD	'g'


typedef struct {
   char *name;    /* The thing this name applies to has various
                   * interpretations depending on
                   * the value of the selector field; see NN/{Id}
                   * entries of 695 table A-1.
                   */
   char selector; /* HPHITY_* selects member from descriptor field */
   union {
	/* See 695 Table A-1 for high level symbol type concepts */
      HPTYSTR_UNK unk;
      HPTYSTR_TAG tag; /* either struct, or union, or enum */
      HPTYSTR_PTR ptr;
      HPTYSTR_CARR carr;
      HPTYSTR_BITFLD bitfld;
      HPTYSTR_PROCWI procwi;
   } descriptor;
} HPHITYPE;

/*
 * HPBITY_* are the 695 built in type codes as defined in table A-2
 * of the 695 spec.
 */
#define HPBITY_UNK		0	/* unknown type	*/
#define HPBITY_V		1	/* void-return  */
#define HPBITY_B		2	/* 8-bit signed	*/
#define HPBITY_C		3	/* 8-bit unsigned */
#define HPBITY_H		4	/* 16-bit signed */
#define HPBITY_I		5	/* 16-bit unsigned */
#define HPBITY_L		6	/* 32-bit signed */
#define HPBITY_M		7	/* 32-bit unsigned */
#define HPBITY_F		10	/* 32-bit float	*/
#define HPBITY_D		11	/* 64-bit float	*/
#define HPBITY_K		12	/* extended float */
#define HPBITY_J		15	/* code address	*/
#define HPBITY_P_UNK		32	/* p. unknown type */	
#define HPBITY_P_V		33	/* p. void-return */
#define HPBITY_P_B		34	/* p. 8-bit signed */
#define HPBITY_P_C		35	/* p. 8-bit unsigned */
#define HPBITY_P_H		36	/* p. 16-bit signed */
#define HPBITY_P_I		37	/* p. 16-bit unsigned */
#define HPBITY_P_L		38	/* p. 32-bit signed */
#define HPBITY_P_M		39	/* p. 32-bit unsigned */
#define HPBITY_P_F		42	/* p. 32-bit float */
#define HPBITY_P_D		43	/* p. 64-bit float */
#define HPBITY_P_K		44	/* p. extended float */
#define HPBITY_P_J		47	/* p. code address */

typedef struct {

   char *name;		/* name of symbol */
   HPTYPEHANDLE type;	/* BITY or index of HPHITYPE */
   HPSYMATT attribute;	/* 695 table 3-6 */
   LOGADR960 asnval;	/* if used, corresponds to 695 ASN value field */
} HPSYM;

typedef struct {
   /* UPDATE: with a little work, users of this struct could instead
    * use HPSYM.
    */
   char *name;
   char bity;		/* built-in type; see restrictions in 695 3.4.2 */
   LOGADR960 value;	/* value of external symbol */
   unsigned numelems;
} HPXSYM;


/*
 * Entry points of 695_out.c
 *
 * Provides the interface for creating, defining and writing an HP/MRI 695
 * load module for the Intel 80960 KA, KB, CA or JX.
 * "695_out.h" provides definitions for the users of this module.
 *
 * This interface is currently intended to support Hewlett Packard 
 * Logic Systems Division "Emulator Functionality".  Extensions are
 * needed to provide "Debugger Functionality".
 *
 * The intent of the interface is to abstract the information which
 * a 695 load module requires without requiring this module to keep
 * a significant database of program symbols
 * (this module only keeps the accumulated 695 External Part in memory
 * between calls).  This design decision
 * requires that the user of this module adhere to an operation ordering
 * which is defined below (after the list of operations).
 *
 * Entry Points:
 *
 *
 *	hp_err()
 *		Returns the last error which this module printed, or
 *	NOT_AN_ERROR.
 * 
 *	hp_create()
 *		Begins the definition of a 695 load module.
 *
 *      hp_set_exestat()
 *		Set the cumulative error level of the 695 file.  
 *
 *	hp_creabsect()
 *		Create an absolute section.
 *
 *	hp_putabsdata()
 *	hp_putabs0()
 *		Fill a section created by hp_creabsect.
 *
 *	hp_puttype(), hp_patchtype()
 *		Define a type which may be referenced by parameters to
 *		hp_putcsym.
 *
 *	hp_openmod()
 *		Begins the definition of symbolic information for a source
 *		module.
 *
 *	hp_closemod()
 *		Terminates the definition of symbolic information for
 *		a source module.
 *
 *	hp_openblock()
 *		Begins the definition of symbolic information for
 *		a "C" function or block.
 *
 *	hp_closeblock()
 *		Terminates the definition of symbolic information for
 *		a "C" function or block.
 *
 *	hp_putxsym()
 *		Define a global symbol (bound external) symbol.
 *
 *	hp_putcsym(), hp_putasym()
 *		Define a program symbol.  
 *
 *	hp_openline()
 *		Begins line number definitions for a source module.
 *	hp_putline()
 *		Defines a line number/machine address mapping.
 *	hp_closeline()
 *		Terminates line number definitions for a source module.
 *
 *	hp_putmodsect()
 *		Defines the portion of a section which is "owned" by
 *		a source module.
 *
 *	hp_close()
 *		Concludes the definition of a load module and writes
 *		all information to disk.
 *
 *	hp_abort()
 *		Terminates definition of a load module and deletes
 *		the output file.
 *	
 *
 * Legal Call Ordering: (a regular expression read top-down, left-to-right,
 *   "[]" implies 0 or 1 occurrences, "+" implies alternation, 
 *   "()" imply grouping, "*" suffix implies 0 or more occurrences).  
 *    hp_put?sym is an abbreviation of the expression (hp_putcsym + hp_putasym).
 *
 *   Not shown: hp_set_exestat is legal anywhere before hp_close.
 *   hp_abort is legal anywhere.  hp_mark is legal after the entity
 *   it modifys, for example a module-mark after hp_openmod, a variable-mark
 *   after hp_putxsym or hp_put?sym, and proc-mark after hp_openblock.
 *------------------------------------------------------------------------------
 *	Operation					> 695 definition
 *------------------------------------------------------------------------------
 *|	hp_create 					>Header, ADX & Env Parts
 *|							> --------------
 *|	hp_creabsect* 					> Section Part
 *|							> --------------
 *|	(hp_putabsdata + hp_putabs0)*  			> Data Part
 *|							> --------------
 *|     hp_putxsym*					> External Part
 *|							> --------------
 *|	                         			> Debug Part
 *|	(						>
 *|       hp_openmod 					>
 *|       [						>
 *|	     (hp_puttype+hp_patchtype)*			>
 *|	     [hp_putcompid]
 *|          (hp_openblock hp_putcsym* hp_closeblock)*	>
 *|          hp_putcsym*				>
 *|	  ]						>
 *|       hp_openline
 *|       hp_putline* 					>
 *|       [hp_putasmid]					>
 *|	  hp_putmodsect*				>
 *|	  hp_putasym*					>
 *|       hp_closemod					>
 *|     )*						>
 *|							> -------------
 *|	hp_close 					> Header, Trailer 
 *|							> & Env Parts
 */

extern char hp_version[];

extern int hp_create();
extern int hp_close();
extern int hp_creabsect();
extern int hp_putabsdata();
extern int hp_putabs0();
extern int hp_err();	
extern void hp_set_exestat();
extern void hp_abort();

extern int hp_openmod();
extern int hp_closemod();
extern int hp_putxsym();
extern int hp_putasym();
extern int hp_putcsym();
extern int hp_openblock();
extern int hp_closeblock();
extern int hp_openline();
extern int hp_putline();
extern int hp_puttype();
extern int hp_patchtype();

extern int hp_mark();
extern int hp_putasmid();
extern int hp_putcompid();
