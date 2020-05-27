
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

/*	@(#)paths.h	2.1	*/
/*
 * Pathnames for  VAX 11/780 and 11/750 assembler and linkage editor
 */
#ifdef VMS
#define I960LIB	""
#define I960LLIB ""
#else
#if defined(MSDOS) || defined(MWC)
#define I960LIB	"\lib"
#define I960LLIB "\usr\lib"
#else
#define I960LIB	"/lib"
#define I960LLIB "/usr/lib"
#endif
#endif

#define NDELDIRS 2

/*
 * Directory containing executable ("bin") files
 */
#if defined(MSDOS) || defined(MWC)
#define BINDIR	"\usr\bin"
#else
#define BINDIR	"/usr/bin"
#endif

/*
 * Directory for "temp"  files
 */
#ifdef VMS
#define TMPDIR	"sys$scratch:"
#else
#if defined(MSDOS) || defined(MWC)
#define TMPDIR	"\tmp"
#else
#define TMPDIR	"/tmp"
#endif
#endif

/*
 * Name of default output object file
 */
#define A_OUT	"a.out"

/*
 * MSDOS stderr mapping
 */
#ifdef	MSDOS
#define STDERR	stdout
#define ERRFID	1
#else
#define STDERR	stderr
#define ERRFID	2
#endif

/*
 * MSDOS environment tmp directories
 */
#define TMPDOS	"TMP"

/*
 * $Log: paths.h,v $
 * Revision 1.3  1993/06/30  17:45:35  karen
 * Intel Copyright update
 *
 * Revision 1.2  1992/11/04  18:13:28  dramos
 * Change copyright.
 *
 * Revision 1.1  1992/06/08  07:29:33  dougs
 * Initial revision
 *
 * Version 1.11  90/08/31  16:17:29  robertat
 * Substitute new environment variable names for old.
 * 
 * Version 1.11  90/08/31  16:17:29  robertat
 * Substitute new environment variable names for old.
 * 
 * Version 1.10  90/05/16  10:31:02  bodart
 * Changed forward slashes to backward slashes for DOS hosts.
 * 
 * Version 1.9  89/01/04  13:38:53  neighorn
 * new comments
 * 
 * Version 1.8  88/11/11  11:16:15  jeff
 * Added ERRFID
 * 
 * Version 1.7  88/03/22  15:49:59  jeff
 * VMS changes.
 * 
 * Revision 1.5  87/07/31  15:13:57  max
 * added proprietary notice.
 * 
 * Revision 1.4  86/11/14  14:45:14  wilson
 * Fix typo in id declaration.
 * 
 * Revision 1.3  86/11/13  20:57:34  pre
 * PC stderr, environment variables
 * 
 * PRE10/30/86
 * MSDOS path and stderr defines
 */
#ifndef	MSDOS
#ifndef	lint
static char	pathsh_id[] ="$Header: /ffs/a/paulr/RCS/paths.h,v 1.3 1993/06/30 17:45:35 karen Exp $$Locker:  $";
#endif
#endif
