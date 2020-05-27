
/*(c****************************************************************************** *
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
 *****************************************************************************c)*/

/******************************************************************************
 *
 * This file describes an Intel 960 COFF object file.
 *
 ******************************************************************************/


/******************************* FILE HEADER *******************************/

struct filehdr {
	unsigned short	f_magic;	/* magic number			*/
	unsigned short	f_nscns;	/* number of sections		*/
	long		f_timdat;	/* time & date stamp		*/
	long		f_symptr;	/* file pointer to symtab	*/
	long		f_nsyms;	/* number of symtab entries	*/
	unsigned short	f_opthdr;	/* sizeof(optional hdr)		*/
	unsigned short	f_flags;	/* flags			*/
};


/* Bits for f_flags:
 *	F_RELFLG	relocation info stripped from file
 *	F_EXEC		file is executable (no unresolved external references)
 *	F_LNNO		line numbers stripped from file
 *	F_LSYMS		local symbols stripped from file
 *	F_AR32WR	file has byte ordering of an AR32WR machine (e.g. vax)
 *	F_PIC		file contains position-independent code
 *	F_PID		file contains position-independent data
 *	F_LINKPID	file is suitable for linking w/pos-indep code or data
 *	F_BIG_ENDIAN_TARGET	target part is big-endian (ie, for an i960 using big-endian memory)
 */
#define F_RELFLG	0x0001
#define F_EXEC		0x0002
#define F_LNNO		0x0004
#define F_LSYMS		0x0008
#define F_AR32WR	0x0010
#define F_COMP_SYMTAB   0x0020  /*  The symbol table has been compressed.
				    All but one orphaned tags are removed.
				    Duplicates are merged into one, and
				    the string table is compressed. */
#define F_PIC		0x0040
#define F_PID		0x0080
#define F_LINKPID	0x0100
#define F_SECT_SYM	0x0200	/* this bit pattern is set in the n_flags
				field of symbol structures that
				represent the names of sections
				output by the assembler; otherwise,
				that field is a copy of the file's
				f_flags field */

#define F_BIG_ENDIAN_TARGET	0x0400 /* target part is big-endian */

#define F_CCINFO	0x0800
	/* FOR GNU/960 VERSION ONLY!
	 *
	 * Set only if cc_info data (for 2-pass compiler optimization) is
	 * appended to the end of the object file.
	 *
	 * Since cc_info data is removed when a file is stripped, we can assume
	 * that its presence implies the presence of a symbol table in the file,
	 * with the cc_info block immediately following.
	 *
	 * A COFF symbol table does not necessarily require a string table.
	 * When cc_info is present we separate it from the symbol info with a
	 * delimiter word of 0xfffffff, which simplifies testing for presence
	 * of a string table:  if the word following the symbol table is not
	 * 0xffffffff, it is the string table length; the string table must be
	 * skipped over before the delimiter (and cc_info) can be read.
	 *
	 * The format/meaning of the cc_data block are known only to the
	 * compiler (and, to a lesser extent, the linker) except for the first
	 * 4 bytes, which contain the length of the block (including those 4
	 * bytes).  This length is stored in a machine-independent format, and
	 * can be retrieved with the CI_U32_FM_BUF macro in cc_info.h .
	 */


/*	Intel 80960 (I960) processor flags.
 *	F_I960TYPE == mask for processor type field.
 */
#define	F_I960TYPE	0xf000
#define	F_I960CORE	0x1000
#define	F_I960CORE1	0x1000	/* Original 80960 core: mov, lda, etc */
#define	F_I960KB	0x2000
#define	F_I960SB	0x2000
#define	F_I960CA	0x5000
#define F_I960CX	0x5000	/* Synonym, for consistency with JX, HX */
#define	F_I960KA	0x6000
#define	F_I960SA	0x6000
#define	F_I960JX	0x7000
#define	F_I960HX	0x8000
#define F_I960CORE2	0x9000	/* Core extensions added to J- and H-series */
#define F_I960JL	0xa000
#define F_I960P80	0xb000
#define F_I960CORE0	0xc000  /* P80/JX compatible code */
#define F_I960CORE3	0xd000	/* Cx, Hx, Jx compatible  */
#define F_PTRIZED       0xf000  /* this bit pattern is NEVER set in the n_flags
				   field of syment structures by either the
				   linker or the assembler.  It is literally
				   guaranteed to be zero for all time.  This
				   bit patternis used by cvt960 to indicate that
				   a syment's name has been pointerized.  */

/*
 * i80960 Magic Numbers
 */
#define I960ROMAGIC	0x160	/* read-only text segments	*/
#define I960RWMAGIC	0x161	/* read-write text segments	*/

#define ISCOFF(mag)   (((mag) == I960ROMAGIC) || ((mag) == I960RWMAGIC))
#define I960BADMAG(x) (!ISCOFF((x).f_magic))

#define	FILHDR	struct filehdr
#define	FILHSZ	sizeof(FILHDR)


/************************* AOUT "OPTIONAL HEADER" *************************/

#define OMAGIC	0407
#define NMAGIC	0410

typedef	struct aouthdr {
	short		magic;	/* type of file				*/
	short		vstamp;	/* version stamp			*/
	unsigned long	tsize;	/* text size in bytes, padded to FW bdry*/
	unsigned long	dsize;	/* initialized data "  "		*/
	unsigned long	bsize;	/* uninitialized data "   "		*/
	unsigned long	entry;	/* entry pt.				*/
	unsigned long	text_start;	/* base of text used for this file */
	unsigned long	data_start;	/* base of data used for this file */
	unsigned long	tagentries;	/* number of tag entries to follow
					 *	ALWAYS 0 FOR 960
					 */
} AOUTHDR;

#define AOUTSZ (sizeof(AOUTHDR))


/****************************** SECTION HEADER ******************************/

struct scnhdr {
	char		s_name[8];	/* section name			*/
	long		s_paddr;	/* physical address     */
	long		s_vaddr;	/* virtual address		*/
	long		s_size;		/* section size			*/
	long		s_scnptr;	/* file ptr to raw data for section */
	long		s_relptr;	/* file ptr to relocation	*/
	long		s_lnnoptr;	/* file ptr to line numbers	*/
	unsigned short	s_nreloc;	/* number of relocation entries	*/
	unsigned short	s_nlnno;	/* number of line number entries*/
	long		s_flags;	/* flags			*/
	unsigned long	s_align;	/* section alignment		*/
};

/*
 * names of "special" sections
 */
#define _TEXT	".text"
#define _DATA	".data"
#define _BSS	".bss"

/*
 * s_flags "type"
 */
				/* TYPE    ALLOCATED? RELOCATED? LOADED? */
				/* ----    ---------- ---------- ------- */
#define STYP_REG	0x0000	/* regular    yes        yes       yes   */
#define STYP_DSECT	0x0001	/* dummy      no         yes       no    */
#define STYP_NOLOAD	0x0002	/* noload     yes        yes       no    */
#define STYP_GROUP	0x0004	/* grouped  <formed from input sections> */
#define STYP_PAD	0x0008	/* padding    no         no        yes   */
#define STYP_COPY	0x0010	/* copy       no         no        yes   */
#define STYP_INFO	0x0200	/* comment    no         no        no    */

#define STYP_TEXT	0x0020	/* section contains text */
#define STYP_DATA	0x0040	/* section contains data */
#define STYP_BSS	0x0080	/* section contains bss  */


#define	SCNHDR	struct scnhdr
#define	SCNHSZ	sizeof(SCNHDR)


/******************************* LINE NUMBERS *******************************/

/* 1 line number entry for every "breakpointable" source line in a section.
 * Line numbers are grouped on a per function basis; first entry in a function
 * grouping will have l_lnno = 0 and in place of physical address will be the
 * symbol table index of the function name.
 */
struct lineno{
	union {
		long l_symndx;	/* function name symbol index, iff l_lnno == 0*/
		long l_paddr;	/* (physical) address of line number	*/
	} l_addr;
	unsigned short	l_lnno;	/* line number		*/
	char padding[2];	/* force alignment	*/
};

#define	LINENO	struct lineno
#define	LINESZ	sizeof(LINENO)


/********************************** SYMBOLS **********************************/

#define SYMNMLEN	8	/* # characters in a symbol name	*/
#define FILNMLEN	14	/* # characters in a file name		*/
#define DIMNUM		4	/* # array dimensions in auxiliary entry */


struct syment {
	union {
		char	_n_name[SYMNMLEN];	/* old COFF version	*/
		struct {
			long	_n_zeroes;	/* new == 0		*/
			long	_n_offset;	/* offset into string table */
		} _n_n;
		char	*_n_nptr[2];	/* allows for overlaying	*/
	} _n;
	long		n_value;	/* value of symbol		*/
	short		n_scnum;	/* section number		*/
	unsigned short	n_flags;	/* copy of flags from filhdr	*/
	unsigned long	n_type;		/* type and derived type	*/
	char		n_sclass;	/* storage class		*/
	char		n_numaux;	/* number of aux. entries	*/
	char		pad2[2];	/* force alignment		*/
};

#define n_name		_n._n_name
#define n_ptr		_n._n_nptr[1]
#define n_zeroes	_n._n_n._n_zeroes
#define n_offset	_n._n_n._n_offset

#define	SYMENT	struct syment
#define	SYMESZ	sizeof(SYMENT)


/*
 * Relocatable symbols have number of the section in which they are defined,
 * or one of the following:
 */
#define N_UNDEF ((short)0)  /* undefined symbol */
#define N_ABS   ((short)-1) /* value of symbol is absolute */
#define N_DEBUG ((short)-2) /* debugging symbol -- value is meaningless */
#define N_TV    ((short)-3) /* indicates symbol needs preload transfer vector */
#define P_TV    ((short)-4) /* indicates symbol needs postload transfer vector*/


/*
 * Symbol storage classes
 */
#define C_EFCN		-1	/* physical end of function	*/
#define C_NULL		0
#define C_AUTO		1	/* automatic variable		*/
#define C_EXT		2	/* external symbol		*/
#define C_STAT		3	/* static			*/
#define C_REG		4	/* register variable		*/
#define C_EXTDEF	5	/* external definition		*/
#define C_LABEL		6	/* label			*/
#define C_ULABEL	7	/* undefined label		*/
#define C_MOS		8	/* member of structure		*/
#define C_ARG		9	/* function argument		*/
#define C_STRTAG	10	/* structure tag		*/
#define C_MOU		11	/* member of union		*/
#define C_UNTAG		12	/* union tag			*/
#define C_TPDEF		13	/* type definition		*/
#define C_USTATIC	14	/* undefined static		*/
#define C_ENTAG		15	/* enumeration tag		*/
#define C_MOE		16	/* member of enumeration	*/
#define C_REGPARM	17	/* register parameter		*/
#define C_FIELD		18	/* bit field			*/
#define C_AUTOARG	19	/* auto argument		*/
#define C_BLOCK		100	/* ".bb" or ".eb"		*/
#define C_FCN		101	/* ".bf" or ".ef"		*/
#define C_EOS		102	/* end of structure		*/
#define C_FILE		103	/* file name			*/
#define C_LINE		104	/* line # reformatted as symbol table entry */
#define C_ALIAS	 	105	/* duplicate tag		*/
#define C_HIDDEN	106	/* ext symbol in dmert public lib */
#define C_SCALL		107	/* Procedure reachable via system call	*/
#define C_LEAFEXT	108	/* Global leaf procedure, "call" via BAL */
#define C_LEAFSTAT	113	/* Static leaf procedure, "call" via BAL */


/*
 * Type of a symbol, in low 5 bits of n_type
 */
#define T_NULL		0
#define T_VOID		1	/* function argument (only used by compiler) */
#define T_CHAR		2	/* character		*/
#define T_SHORT		3	/* short integer	*/
#define T_INT		4	/* integer		*/
#define T_LONG		5	/* long integer		*/
#define T_FLOAT		6	/* floating point	*/
#define T_DOUBLE	7	/* double word		*/
#define T_STRUCT	8	/* structure 		*/
#define T_UNION		9	/* union 		*/
#define T_ENUM		10	/* enumeration 		*/
#define T_MOE		11	/* member of enumeration*/
#define T_UCHAR		12	/* unsigned character	*/
#define T_USHORT	13	/* unsigned short	*/
#define T_UINT		14	/* unsigned integer	*/
#define T_ULONG		15	/* unsigned long	*/
#define T_LNGDBL	16	/* long double		*/

/*
 * derived types, in n_type
 */
#define DT_NON		0	/* no derived type	*/
#define DT_PTR		1	/* pointer		*/
#define DT_FCN		2	/* function		*/
#define DT_ARY		3	/* array		*/

#define N_BTMASK	0x1f
#define N_TMASK		0x60
#define N_BTSHFT	5
#define N_TSHIFT	2

#define BTYPE(x)	((x) & N_BTMASK)

#define ISPTR(x)	(((x) & N_TMASK) == (DT_PTR << N_BTSHFT))
#define ISFCN(x)	(((x) & N_TMASK) == (DT_FCN << N_BTSHFT))
#define ISARY(x)	(((x) & N_TMASK) == (DT_ARY << N_BTSHFT))
#define ISTAG(x)	((x)==C_STRTAG||(x)==C_UNTAG||(x)==C_ENTAG)


#define DECREF(x) ((((x)>>N_TSHIFT)&~N_BTMASK)|((x)&N_BTMASK))

union auxent {
	struct {
		long x_tagndx;	/* str, un, or enum tag indx */
		union {
			struct {
			    unsigned short x_lnno; /* declaration line number */
			    unsigned short x_size; /* str/union/array size */
			} x_lnsz;
			long x_fsize;	/* size of function */
		} x_misc;
		union {
			struct {		/* if ISFCN, tag, or .bb */
			    long x_lnnoptr;	/* ptr to fcn line # */
			    long x_endndx;	/* entry ndx past block end */
			} x_fcn;
			struct {		/* if ISARY, up to 4 dimen. */
			    unsigned short x_dimen[DIMNUM];
			} x_ary;
		} x_fcnary;
		unsigned short x_tvndx;		/* tv index */
	} x_sym;

	union {
		char x_fname[FILNMLEN];
		struct {
			long x_zeroes;
			long x_offset;
		} x_n;
	} x_file;

	struct {
		long x_scnlen;			/* section length */
		unsigned short x_nreloc;	/* # relocation entries */
		unsigned short x_nlinno;	/* # line numbers */
	} x_scn;

	struct {
		long		x_tvfill;	/* tv fill value */
		unsigned short	x_tvlen;	/* length of .tv */
		unsigned short	x_tvran[2];	/* tv range */
	} x_tv;		/* info about .tv section (in auxent of symbol .tv)) */

	/******************************************
	 *  I960-specific *2nd* aux. entry formats
	 ******************************************/
	struct {
		long		x_stindx;	/* sys. table entry */
	} x_sc;					/* system call entry */

	struct {
		unsigned long	x_balntry;	/* BAL entry point */
	} x_bal;	                        /* BAL-callable function */

	struct {
		unsigned long	x_timestamp;	/* time stamp */
		char 	x_idstring[20];	        /* producer identity string */
	} x_ident;				/* Producer ident info */

	char a[sizeof(struct syment)];	/* force auxent/syment sizes to match */
};

#define	AUXENT	union auxent
#define	AUXESZ	sizeof(AUXENT)

#define _ETEXT	"_etext"

/********************** RELOCATION DIRECTIVES **********************/

struct reloc {
	long r_vaddr;		/* Virtual address of reference */
	long r_symndx;		/* Index into symbol table	*/
	unsigned short r_type;	/* Relocation type		*/
	char pad[2];		/* Unused			*/
};

/* Only values of r_type GNU/960 cares about */
#define R_RELLONG	17	/* Direct 32-bit relocation		       */

#define R_RELLONG_SUB	18	/* Direct 32 bit relocation using subtraction.
				   If this is defined, it is enabled in the linker
				   and dumper.  If not, then the linker and dumper
				   do not recognize the relocation type. */

#define R_RELSHORT	22	/* Direct 12-bit relocation (for MEMA's)       */
#define R_IPRMED	25	/* 24-bit ip-relative relocation	       */
#define R_OPTCALL	27	/* 32-bit optimizable call (leafproc/sysproc)  */
#define R_OPTCALLX	28	/* 64-bit optimizable call (leafproc/sysproc)  */
#define R_OPTCALLXA     32      /* 3 word optimizable call for 80960A  */

#define RELOC struct reloc
#define RELSZ sizeof(RELOC)
