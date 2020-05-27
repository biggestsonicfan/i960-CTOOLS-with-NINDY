
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

/* 
** ldfcn.h - coff lib interface macros header
** $Version: 1.14 $
*/
#include <stdio.h>
#include "coff.h"
#include "ar.h"

#ifndef LDFILE
struct	ldfile {
	int	_fnum_;		/* so each instance of an LDFILE is unique */
	FILE	*ioptr;		/* system I/O pointer value */
	long	offset;		/* absolute offset to the start of the file */
	FILHDR	header;		/* the file header of the opened file */
	unsigned short	type;	/* indicator of the type of the file */
	char	*spellings;	/* [atw] names of long archive files */
	int	arstrsize;	/* [atw] length of spelling pool */
};

/*
** Provide a structure "type" definition, and the associated "attributes".
*/
#define	LDFILE		struct ldfile
#define IOPTR(x)	(x)->ioptr
#define OFFSET(x)	(x)->offset
#define TYPE(x)		(x)->type
#define	HEADER(x)	(x)->header
#define SPELLINGS(x)	(x)->spellings
#define ARSTRSIZE(x)	(x)->arstrsize
#define LDFSZ		sizeof(LDFILE)

/*
** Define various values of TYPE(ldptr)
*/
#define TVTYPE	TVMAGIC	    /* defined in terms of the filehdr.h include file */ 
#define USH_ARTYPE	(unsigned short) ARTYPE

#define ARTYPE 	0177545

/*
** Define symbolic positioning information for FSEEK (and fseek)
*/

#define BEGINNING	0
#define CURRENT		1
#define END		2

/*
** Define a structure "type" for an archive header
*/
typedef struct {
	union {
		char ar_name[16];	/* file name - `/' terminated */
		struct {
			long zeroes;	/* 0 IF name > 14 chars	      */
			char *nam_ptr;	/* pointer to long name	      */
		} a_n;
	} n;
	long ar_date;
	int ar_uid;
	int ar_gid;
	long ar_mode;
	long ar_size;
} archdr;

#define AR_NAME_PTR(x) ((x->n.a_n.zeroes) ? x->n.ar_name : x->n.a_n.nam_ptr)
#define ARNAM(x)	((x.n.a_n.zeroes) ? x.n.ar_name : x.n.a_n.nam_ptr)
#define	ARCHDR	archdr
#define ARCHSZ	sizeof(ARCHDR)

/*
** Define some useful symbolic constants
*/
#define SYMTBL	0 /* section number and/or section name of the Symbol Table */

#define	SUCCESS	 1
#define	CLOSED	 1
#define	FAILURE	 0
#define	NOCLOSE	 0
#define	BADINDEX	-1L

#define	OKFSEEK	0

/*
** Define macros to permit the direct use of LDFILE pointers with the
** standard I/O library procedures
*/
#define GETC(ldptr)         getc(IOPTR(ldptr))
#define GETW(ldptr)	    getw(IOPTR(ldptr))
#define FEOF(ldptr)	    feof(IOPTR(ldptr))
#define FERROR(ldptr)	    ferror(IOPTR(ldptr))
#define FGETC(ldptr)	    fgetc(IOPTR(ldptr))
#define FGETS(s,n,ldptr)    fgets(s,n,IOPTR(ldptr))
#define FILENO(ldptr)	    fileno(IOPTR(ldptr))
#define FREAD(p,s,n,ldptr)  fread(p,s,n,IOPTR(ldptr))
#define FSEEK(ldptr,o,p)    fseek(IOPTR(ldptr),(p==BEGINNING)?(OFFSET(ldptr)+o):o,p)
#define FTELL(ldptr)	    ftell(IOPTR(ldptr))
#define FWRITE(p,s,n,ldptr) fwrite(p,s,n,IOPTR(ldptr))
#define REWIND(ldptr)	    rewind(IOPTR(ldptr))
#define SETBUF(ldptr,b)	    setbuf(IOPTR(ldptr),b)
#define UNGETC(c,ldptr)	    ungetc(c,IOPTR(ldptr))
#ifdef SYMESZ
#define STROFFSET(ldptr)    (HEADER(ldptr).f_symptr + HEADER(ldptr).f_nsyms * SYMESZ)
#endif
#endif  /* end LDFILE */

/*
** Ansi Standard C compilers support prototypes,  others will not.
*/
#if defined(__STDC__)

struct ldfile *ldopen(char *,struct ldfile *);
struct ldfile *ldaopen(char *,struct ldfile *);
int ldaclose(struct ldfile *);
int ldclose(struct ldfile *);
int ldlseek(struct ldfile *,unsigned short );
int ldnlseek(struct ldfile *,char *);
int ldnrseek(struct ldfile *,char *);
int ldnsseek(struct ldfile *,char *);
int ldnshread(struct ldfile *,char *,struct scnhdr *);
int ldrseek(struct ldfile *,unsigned short );
int ldtbread(struct ldfile *,long ,struct syment *);
int ldsseek(struct ldfile *,unsigned short );
int ldshread(struct ldfile *,unsigned short ,struct scnhdr *);
int ldfhread(struct ldfile *,struct filehdr *);
int ldtbseek(struct ldfile *);
int ldohseek(struct ldfile *);
long ldtbindex(struct ldfile *);
int ldlread(struct ldfile *,long ,unsigned short ,struct lineno *);
int ldlinit(struct ldfile *,long );
int ldlitem(struct ldfile *,unsigned short ,struct lineno *);
long sgetl(char *);
char *ldgetname(struct ldfile *,struct syment *);
int ldahread(struct ldfile *, archdr *);

#else  /* No prototyping available */

struct ldfile  *ldopen();
struct ldfile  *ldaopen();
int ldaclose();
int ldclose();
int ldlseek();
int ldnlseek();
int ldnrseek();
int ldnsseek();
int ldnshread();
int ldrseek();
int ldtbread();
int ldsseek();
int ldshread();
int ldfhread();
int ldtbseek();
int ldohseek();
long ldtbindex();
int ldlread();
int ldlinit();
int ldlitem();
char *ldgetname();
int ldahread();	

#endif
