
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
 * Note: ldlitem assumes ldlinit() or ldlread() has been called
 * to set _cofl_line_start.
 * Note: line number table can be in any order - must scan entire
 * table for exact match, and then return closest.
 */

#include <stdio.h>
#include "ldfcn.h"

extern LINENO *_cofl_line_start;
extern LINENO *_cofl_line_end;

int
#if	defined(__STDC__)
ldlitem(LDFILE *ldptr, unsigned short linenum, LINENO *linent)
#else
ldlitem(ldptr, linenum, linent)
LDFILE *ldptr;
unsigned short linenum;
LINENO *linent;
#endif
{
   LINENO templn, saveln;
   LINENO *pos = _cofl_line_start; /* the file position after the last fread() */
   int saveln_valid = 0;

   if (_cofl_ldfile_ok(ldptr) == FAILURE)
      return FAILURE;

   fseek(IOPTR(ldptr), _cofl_line_start, BEGINNING);
   if (fread(&templn, 1, LINESZ, IOPTR(ldptr)) != LINESZ)
      return FAILURE;
   pos++;

   if (linenum == 0) /* get the start of the function */
   {
      *linent = templn;
      return SUCCESS;
   }
 
   while ((pos++ <= _cofl_line_end) &&
          (fread(&templn, 1, LINESZ, IOPTR(ldptr)) == LINESZ) &&
          (templn.l_lnno != 0))
   {
      if (templn.l_lnno == linenum)
      {
         *linent = templn;
         return SUCCESS;
      }

      if ((templn.l_lnno > linenum) &&
          (saveln_valid ? (saveln.l_lnno > templn.l_lnno) : 1))
      {
         saveln = templn;
         saveln_valid = 1;
      }
   } /* while */

   if (saveln_valid)
   {
      *linent = saveln;
      return SUCCESS;
   }

   return FAILURE;
}

