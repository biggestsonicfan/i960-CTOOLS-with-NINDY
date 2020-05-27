
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
 * suportroutines for the GNU/960 COFL library (libld960.a)
 */

#include <stdio.h>
#include "ldfcn.h"

/* _cofl_line_start is the ftell address of function's line numbers, as set
 * by ldlinit().  It should point to line number 0, the start of the
 * function, so that ldlitem() called with line number 0 will return the
 * function's base address.  _cofl_line_end is the ftell address of the last
 * line number entry in the section. 
 */

LINENO *_cofl_line_start = (LINENO *)0;
LINENO *_cofl_line_end   = (LINENO *)0; 

int
_cofl_make_addr_even(x)
LDFILE *x;
{
   if (ftell(IOPTR(x)) % 2 == 1)
      fseek(IOPTR(x), 1, CURRENT);
}

int
_cofl_ldfile_ok(ldptr)
LDFILE *ldptr;
{
   if (!ldptr || !IOPTR(ldptr))
      return FAILURE;

   return SUCCESS;
}

int
_cofl_arch_hdr_cvt(ldptr, dest, src)
LDFILE *ldptr;
ARCHDR *dest;
AR_HDR *src;
{
   char *slash_loc;

   /* convert AR_HDR format to ARCHDR format */
   if (src->ar_name[0] == ' ')
   {
      /* this is a long name (points into SPELLINGS) */
      dest->n.a_n.zeroes = 0;
      dest->n.a_n.nam_ptr = (char *)(SPELLINGS(ldptr) + sgetl(&src->ar_name[1]));
   } else {
      strncpy(dest->n.ar_name, src->ar_name, sizeof(src->ar_name));
      if ((slash_loc = (char *)strchr(dest->n.ar_name, '/')) != 0)
      {
         *slash_loc = NULL;
      } else {
         dest->n.ar_name[sizeof(src->ar_name)-1] = NULL;
      }
   }

   dest->ar_date = sgetl(src->ar_date);
   dest->ar_uid  = sgetl(src->ar_uid);
   dest->ar_gid  = sgetl(src->ar_gid);
   dest->ar_mode = sgetl(src->ar_mode);
   dest->ar_size = sgetl(src->ar_size);
}
