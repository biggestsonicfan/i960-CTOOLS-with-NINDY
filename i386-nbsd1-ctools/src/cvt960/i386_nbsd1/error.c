
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

/*					*/
/* Common Error handler routine		*/
/*					*/

#include <stdio.h>

#ifdef MWC
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "err_msg.h"

#if defined(MSDOS) || defined(MWC)
#define TMPDIR  "\tmp"
#else
#define TMPDIR  "/tmp"
#endif

#if 0
/*
 * MSDOS stderr mapping is now done in toolib
 */
#ifdef  MSDOS
#define STDERR  stdout
#define ERRFID  1
#else
#define STDERR  stderr
#define ERRFID  2
#endif

#else

#define STDERR  stderr
#define ERRFID  2

#endif

/*
 * MSDOS environment tmp directories
 */
#define TMPDOS  "TMP"

/* 
 * FIXME: Workaround for broken vfprintf() on sol-sun4 
 */
#ifdef	SOLARIS
char 	sol_tmpbuf[1024];
#endif

/* Table of errors */
error_struct errors[] = {
/* COMMON ERRORS */
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
"COFF argument symbol at index %d is ignored; addressing path too complicated for IEEE-695.\n",
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
        HPER_BADHPTYPE, "internal error: inappropriate IEEE-695 type (%d)\n",

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
	REGOUTOFFUNCSCOPE,"Symbol index: %d is a C_REG storage class outside a .bf/.ef scope.\n",

	END_ERR_TBL,"END TABLE",
};

int error_count;

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
	error_count ++;
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
	/* Put out error message */
	fprintf(fp,"%s ERROR %04u.%02u -- ", first, num, sub_num);
#if defined(unix) && defined(vax) 
	_doprnt(errors[i].msg, args, fp);
#else
	vfprintf(fp, errors[i].msg, args);
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
	/* Put out error message */
	fprintf(fp,"%s WARNING %04u.%02u -- ", first, num, sub_num);
#if defined(unix) && defined(vax) 
	_doprnt(errors[i].msg, args, fp);
#else
	vfprintf(fp, errors[i].msg, args);
#endif
	fflush(fp);
	return(OK_RETURN);
} /* end routine warn_log */


/* Routine to output a usage message for tool */
usage()
{
        static char usage_message[]=
        "Usage: cvt960 [-v960] [-{V|h|a|s|w|z}] [-Aarch] [-i file] [-o file]";
        static char get_help_message[]=
        "Use -h option to get help";

        fprintf(STDERR,"\n%s\n",usage_message);
        fprintf(STDERR,"%s\n\n",get_help_message);
	error_count ++;
} /* end routine usage */


/* Routine to output help message */
put_cvt_help()
{
        int i;
        static char umessage[]=
        "Usage: cvt960 [-v960] [-{V|h|a|s|w|z}] [-Aarch] [-i file] [-o file]";
        static char *utext[] = {
                "",
                "Converts the specified COFF file to IEEE-695:",
                "",
                "   -a: convert files for use with MRI Xray user interface",
                "        [this should be used when converting files to be used with",
                "         XICE production release 5.00 and earlier and engineering",
                "         release x263 and earlier]",
                "   -A: specify an 80960 architecture tag for the output file",
                "        [valid architectures are: CORE,KA,SA,KB,SB,CA,CF,JA,JF,JD,",
		"        HA,HD,HT,RP]",
		"   -c: specify to emit column zero for line entries rather than one.",
                "   -h: display this help message",
                "   -i: specify name of input file to be converted",
                "   -o: name of output file",
                "        [by default the input file is given a .x extension]",
                "   -s: does not convert debug information",
                "   -V: print version information and continue",
                "-v960: print version information and exit",
                "   -w: supress all warning messages",
                "   -z: writes constant time stamp and command line to the output file",
                "",
                NULL
        };

        fprintf(stdout,"%s\n", umessage);
	paginator(utext); 

} /* end put_cvt_help */
