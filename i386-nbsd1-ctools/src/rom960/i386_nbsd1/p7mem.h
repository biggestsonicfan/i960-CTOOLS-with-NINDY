
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
 * header info for p7mem
 */


/* system includes */

#include <stdio.h>

#if defined(MSDOS) || defined(MWC)
#include <string.h>
#include <assert.h>
#if !defined(MWC)
#define	bzero(sp,size)	memset(sp,'\0',size)
#endif
#else
#if defined(VMS) || defined(SYSV)
#include <string.h>
#define  index(a,b)   strchr(a,b)
#else
#include <strings.h>
#endif
#endif

/* files from the assembler/linker */

#if defined(MWC)
#undef BUFSIZE	/* BUFSIZE is defined in MWC's stdio.h */
#endif
#define BUFSIZE 4096

#define max(x,y) (((x)>(y))?(x):(y))
#define min(x,y) (((x)<(y))?(x):(y))

#define round_up(x,y) ((((x)+(y)-1)/(y))*(y))


extern int debug;	/* turn debugging on/off */
extern char	buf[];
extern char	*myname;


extern unsigned long	full_size;
extern unsigned long	image_start;	/* start of first section */
extern unsigned long	image_current;	/* current address working on.*/

extern char *copy();

#ifdef	MSDOS
extern char *fntdos();
#else 
#define	fntdos(fn) fn
#endif

#define MAXSECS	0x100	/* maximum COFF sections we will allow */

#define MODE16	1	/* setting for mode if user asked to have ihex output
			in 8086 mode */
#define MODE32	2	/* setting for mode if user asked to have ihex output
			in 32-bit mode (the default) */
#define I960ROMAGIC     0x160   /* magic numbers for COFF files */
#define I960RWMAGIC     0x161   

