
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

/*
 * routines for dealing with reading the COFF file
 */

#include "rom960.h"
#if !defined(MSDOS) || defined(MWC) || defined(CBLD)
#define	myalloc(a)	malloc(a)
#define	MAXDRW		0x7FFFFFFF
#else
#include <malloc.h>
#define	free(a)		hfree(a)
#define	myalloc(a)	halloc(a,1)
#define	MAXDRW		32767
#endif

#if defined(MWC) || defined(CBLD)
#include <stdlib.h>
#endif

#ifdef VMS
#include "vmsutils.h"
#else
#define EXIT_OK 0
#define EXIT_FAIL 1
#endif

#include "portable.h"
/* the following declarations and includes were added to support common
error handling 9/29/89 -- robertat */
#include <setjmp.h>
#include "err_msg.h"
#if defined(MWC) || defined(CBLD)
static char *err_msg;
#else
char *err_msg;
#endif
extern jmp_buf parse_err;

read_section_info(infile,nsects,sects,filename,nl)
bfd *infile; 	/*  COFF file ptr */
int nsects;
sec_ptr sects[];
char *filename;
struct name_list *nl;
{
    int i;
    sec_ptr p;

    /*
     * Read in all section headers.
     */

    if (!nsects) { 
	error_out(ROMNAME,ROM_NO_SECTS,0,filename);
	longjmp(parse_err,1);
    }

    if (!nl) {
	for (p=infile->sections,i = 0; i < nsects; i++,p = p->next) {
	    if (!p) {
		error_out(ROMNAME,ROM_MISSING_SECTION,i);
		longjmp(parse_err,1);
	    }
	    sects[i] = p;
	}
    }
    else {
	for (i=0;nl;i++,nl = nl->next_name) {
	    if (!(sects[i]=bfd_get_section_by_name(infile,nl->name))) {
		error_out(ROMNAME,ROM_MISSING_SECTION,i);
		longjmp(parse_err,1);
	    }
	}
    }
}

/*
 * quick and dirty sort of sections into order by start address
 */
sort_sections(nsects,sects)
int nsects;
sec_ptr sects[];
{
	int i,j;
	sec_ptr tmp;

	for (i = nsects - 1; i > 0; i--) {
		for (j = 0; j < i; j++) {
			if ((unsigned long) sects[j]->pma >
			    (unsigned long) sects[j+1]->pma ) {
				tmp = sects[j];
				sects[j] = sects[j+1];
				sects[j+1] = tmp;
				
			}
		}
	}
}

