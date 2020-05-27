
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
 * ldclose()
 */

#include <stdio.h>
#include "ldfcn.h"

int
ldclose(ldptr)
LDFILE *ldptr;
{
   ARCHDR arch_hdr;

   /* can't use _cofl_ldfile_ok(ldptr) because ldclose() always returns SUCCESS
    * except for archives with more members.
    */ 

   if (ldptr &&
       (TYPE(ldptr) == ARTYPE))
   {
      long file_size;

      fseek(IOPTR(ldptr), 0, END);
      file_size = ftell(IOPTR(ldptr));

      /* return FAILURE if more members in the archive */
      if (ldahread(ldptr, &arch_hdr) == FAILURE)
      {
         /* couldn't read current archive header for some reason */
         return NULL;
      }

      /* go to the beginning of the next member */
      fseek(IOPTR(ldptr), OFFSET(ldptr) + arch_hdr.ar_size + AR_HSZ, BEGINNING);
      _cofl_make_addr_even(ldptr);
      if ((OFFSET(ldptr) = ftell(IOPTR(ldptr))) < file_size)
      {
         /* there is another member - return FAILURE */
         return FAILURE;
      }
   }

   return (ldaclose(ldptr)); /* frees memory; always returns SUCCESS */
} /* ldclose() */

