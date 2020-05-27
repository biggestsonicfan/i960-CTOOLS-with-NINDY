
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

extern LINENO *_cofl_line_start;
extern LINENO *_cofl_line_end;

int
ldlinit(ldptr, fcnindx)
LDFILE *ldptr;
long fcnindx;
{
   SCNHDR secthead;
   LINENO line;
   unsigned short lcnt;
   SYMENT syment;
   AUXENT symaux;
   int secnum;

   if (_cofl_ldfile_ok(ldptr) == FAILURE)
      return FAILURE;

   if ((ldtbread(ldptr, fcnindx, &syment) == SUCCESS) &&
       (syment.n_sclass == C_EXT))
   {
      if (syment.n_numaux > 0)
      {
         if (fread(&symaux, 1, AUXESZ, IOPTR(ldptr)) == AUXESZ)
         {
            if (symaux.x_sym.x_fcnary.x_fcn.x_lnnoptr)
            {
               if (ldshread(ldptr, syment.n_scnum, &secthead) == SUCCESS)
               {
                  if (secthead.s_nlnno)
                  {
                     _cofl_line_start = (LINENO *)(OFFSET(ldptr) + symaux.x_sym.x_fcnary.x_fcn.x_lnnoptr);
                     _cofl_line_end   = (LINENO *)(OFFSET(ldptr) + secthead.s_lnnoptr + (LINESZ * (secthead.s_nlnno-1)));
                     return SUCCESS;
                  }
               }
            }
         }
      }
   }

   return FAILURE;
}

