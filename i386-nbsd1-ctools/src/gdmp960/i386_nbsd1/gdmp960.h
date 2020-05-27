
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

/* $Id: gdmp960.h,v 1.11 1995/12/21 23:24:01 kevins Exp $
 */

#include "bfd.h"
#include "libdwarf.h"

extern char host_is_big_endian;	/* TRUE or FALSE */
extern char file_is_big_endian;	/* TRUE or FALSE */
#ifdef BIG_ENDIAN_MODS
extern char big_endian_target_sections; /* TRUE or FALSE */
#endif /* BIG_ENDIAN_MODS */

extern char * xmalloc();

/* Perform byte swap in place on variable 'n' */
#define	BYTESWAP(n)	byteswap(&n,sizeof(n))

/* keep a list of sections to dump */
typedef struct special_arg
{
    char *name;
    int  id;
    struct special_arg *link;
} special_type;

/*
 *  PARAMS macro for function prototypes.  This allows for writing
 *  only one declaration per function.
 *
 *  NOTE: All known AIX compilers implement prototypes, but don't always
 *  define __STDC__
 */
#if ! defined(PARAMS)
#if defined(__STDC__) || defined(_AIX)
#define PARAMS(paramlist)       paramlist
#else
#define PARAMS(paramlist)       ()
#endif
#endif

#ifdef	__STDC__
#include <stdarg.h>
void 	error (const char *Format, ...);
void	warning (const char *Format, ...);
#else	/* not ANSI C */
#include <varargs.h>
void 	error ();
void	warning ();
#endif

void	xread PARAMS ((char *bufp, int n));
long	xseek PARAMS ((long offset));
long	xseekend PARAMS ((void));
char	*xmalloc PARAMS ((int len));
char	*xrealloc PARAMS ((char *oldbuf, int len));
int  	dmp_bout_stringtab PARAMS ((void));

int	dmp_debug_info PARAMS ((Dwarf_Debug));
int	dmp_debug_line PARAMS ((Dwarf_Debug));
int	dmp_debug_frame PARAMS ((Dwarf_Debug));
int	dmp_debug_pubnames PARAMS ((Dwarf_Debug));
int	dmp_debug_aranges PARAMS ((Dwarf_Debug));
int	dmp_debug_macinfo PARAMS ((Dwarf_Debug));
int	dmp_debug_loc PARAMS ((Dwarf_Debug));
int	dmp_debug_abbrev PARAMS ((Dwarf_Debug));

void	build_libdwarf_section_list PARAMS ((bfd *, asection *, PTR));
Dwarf_Signed	dwarf2_errhand PARAMS ((Dwarf_Error));

