
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
 * ldsseek()
 * ldnsseek()
 */

#include <stdio.h>
#include "ldfcn.h"

int
#if	defined(__STDC__)
ldsseek(LDFILE *ldptr, unsigned short sectindex)
#else
ldsseek(ldptr, sectindex)
LDFILE *ldptr;
unsigned short sectindex;
#endif
{
   SCNHDR sechdr;

   if (_cofl_ldfile_ok(ldptr) == FAILURE)
      return FAILURE;

   if (sectindex != 0) 
   {
      if (ldshread(ldptr, sectindex, &sechdr) == SUCCESS)
      {
	 if (sechdr.s_scnptr != 0 )
         {
            if (fseek(IOPTR(ldptr), sechdr.s_scnptr, BEGINNING) == 0 )
            { 
                return SUCCESS;
	    }
         } /* if */
      } /* if */
   } /* if */

   return FAILURE;
} /* ldsseek() */



