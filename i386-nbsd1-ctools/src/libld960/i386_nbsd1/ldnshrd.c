
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



#include <stdio.h>
#include "ldfcn.h"

int
ldnshread(ldptr, sectname, secthead)
LDFILE *ldptr;
char *sectname;
SCNHDR *secthead;
{
   int cnt = 0;

   if (_cofl_ldfile_ok(ldptr) == FAILURE)
      return FAILURE;

   if (sectname)	/* non-NULL sectname */
   {
      fseek(IOPTR(ldptr), OFFSET(ldptr) + FILHSZ + HEADER(ldptr).f_opthdr, BEGINNING); /* start of sections */

      do
      {
         if (fread(secthead, 1, SCNHSZ, IOPTR(ldptr)) != SCNHSZ) {
            /* error - couldn't read section header */
            return FAILURE;
         }

         if (strncmp(sectname, secthead->s_name, sizeof(secthead->s_name)) == 0)
         {
            return SUCCESS;
         } /* if */
      } while (cnt++ < HEADER(ldptr).f_nscns);
   } /* if */

   return FAILURE;
} /* ldnshread() */

