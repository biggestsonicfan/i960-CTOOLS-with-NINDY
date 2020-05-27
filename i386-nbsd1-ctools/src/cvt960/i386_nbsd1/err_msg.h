
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


/* Macros for error.c */
#define END_ERR_TBL	99999
#define	ERR_RETURN	-1
#define	OK_RETURN	0

/* Macros for usage() */
#define CVT_IDENT	12

/*  Structure for error table */
typedef struct {
	int	num;
	char	*msg;
}error_struct;

/* Macros for ERRORS */
#define	NO_SUB			0	/* For no sub number */

/* File errors and memory allocation errors */
#define	NOT_OPEN		50
#define	NO_FILES		51
#define	NO_FOUND		52
#define	NOT_COFF		53
#define	CRE_FILE		54
#define	TMP_FILE		55
#define	ER_FREAD		56
#define	EXTR_FIL		57
#define	PREM_EOF		58
#define	ALLO_MEM		60
#define	ALLO_ARR		61
#define	WRIT_ERR		62
#define	NOT_COF2		63
#define	NO_TMPNM		64
#define	NOATEXIT		65

/* Command line Option errors */
#define NSP_ARGS		70
#define OPT_ARG			71
#define ILL_OPT			72
#define	AOPT_ARG		73
#define	BD_AOPT			74
#define	EMPY_OPT		74
#define	AMBG_OPT		75

/* Macros for libld access and other coff format referencing */
#define	RD_ARCHD		301
#define	RD_ARSYM		302
#define	RD_FILHD		303
#define	NO_OFLHD		304
#define	RD_SECHD		305
#define	NO_SECHD		306
#define	RD_SECT			307
#define	SYMTB_HD		308
#define	RD_SYMTB		309
#define	NO_SYMTB		310
#define	RD_SYMNM		311
#define	SK_SYMTB		312
#define	NO_FUNC			313
#define	RD_STRTB		314
#define	RD_AUXEN		315
#define	RD_RELOC		316
#define	RD_LINEN		317
#define	RD_LINES		318
#define	NO_LINE			319
#define	NLINE_FN		320
#define	WR_FILHD		321
#define	RD_OFLHD		322
#define	WR_OFLHD		323
#define	NO_OHDSZ		324
#define	WR_SECHD		325
#define	WR_RELOC		326
#define	WR_LINES		327
#define	RD_SCHDF		328
#define	WR_SCHDF		329
#define	RD_RLOCF		330
#define	WR_RLOCF		331
#define	RD_LINEF		332
#define	WR_LINEF		333

/* CVT960 */

/* 695 Writer Errors (hp695_960.c) */

#define HPER_SECID		1600
#define HPER_DUPSEC		1601
#define HPER_ACODE		1602
#define HPER_BADCHAR		1603
#define HPER_UNKSEC		1604
#define HPER_UNK		1605
#define HPER_BADSTYPE		1606
#define HPER_BADHPTYPE		1607
#define HPER_UNKDBLK		1608
#define HPER_BADATT		1609

#define HPER_BADHITYPE		1610
#define HPER_RECOPMOD		1611
#define HPER_TYDEFUSE		1612
#define HPER_CLSDMOD		1613
#define HPER_FCLOSE		1614
#define HPER_NOTASMATT		1615
#define HPER_SYNCNTXT		1616

/* cvt960.c */
#define	UNEXPECTED_SECTYPE	1717
#define TOO_MANY_SCNS           1718
#define NO_FILE_SYMS		1719
#define SYMCONFLICT		1720
#define DATACONFLICT		1721
#define	INVTAGNDX		1722
#define	ARGBLKARG		1723
#define	ILLREGVAL		1724
#define PIX_NOTRANS		1725
#define FILESAME		1726
#define BADAUX			1727

/* and the rest of the cvt960 errors */
#define CVT_BAD_ASW		1728
#define CVT_MV_BACK		1729
#define CVT_NO_MEM		1730
#define CVT_E_PT_ERR		1731
#define CVT_BAD_LD_ITEM		1732
#define CVT_E_PT_IMBAL		1733
#define CVT_D_SEC_SEEK		1734
#define CVT_BAD_TRAILER		1735
#define CVT_AS_RD_ERR		1736
#define CVT_BAD_695_ARCH	1737
#define CVT_ADDR_MISSING	1738
#define CVT_E_PT_MISSING	1739
#define CVT_STRING_BAD		1740
#define CVT_OFFSET_MISSING	1741
#define CVT_NO_CMP_FILE		1742
#define CVT_NOT_COFF		1743
#define CVT_NOT_695		1744
#define CVT_UNSUP_695_SEC	1745
#define CVT_BAD_STRING		1746
#define CVT_NO_OPEN_695		1747
#define CVT_TRAILER_SEEK	1748
#define CVT_SECTION_SEEK	1749
#define CVT_DATA_SEEK		1750
#define CVT_CATASTROPHE		1751
#define CVT_ERRS_FOUND		1752
#define CVT_WARNS_FOUND		1753
#define CVT_WRITE_ERR		1754
#define CVT_HP_ABORT		1755
#define CVT_SYMENT_SEEK		1756
#define CVT_DATA_CLASH		1757
#define CVT_NO_SUCH_SEC		1758
#define REGOUTOFFUNCSCOPE       1759

/* ZZZ is the next error number available for use */
#define ZZZ		1760
