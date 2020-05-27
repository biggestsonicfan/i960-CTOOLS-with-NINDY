
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


#define COFL_STR_MAX 1000
static char _cofl_retstr[COFL_STR_MAX]; /* for ldgetname() return value */
static char *_cofl_null_str = "";

/*
 * ldgetname()
 */

#include <stdio.h>
#include "ldfcn.h"

char *
ldgetname(ldptr, symbol)
LDFILE *ldptr;
SYMENT *symbol;
{
   long str_start;

   if (_cofl_ldfile_ok(ldptr) == FAILURE)
      return FAILURE;

   if (symbol->n_zeroes != 0) {
      /* the name is in SYMENT *symbol already; copy and return it */
      /* silly calling routine! */
      strncpy(_cofl_retstr, symbol->n_name, SYMNMLEN);
      _cofl_retstr[SYMNMLEN] = (char)NULL;
      return _cofl_retstr;
   }

   if (symbol->n_offset <= 3) /* no name at all */
   {
      _cofl_retstr[0] = NULL; /* this should really be return (char *)NULL, but AT&T */
      return _cofl_retstr;    /* doesn't do it that way. */
   }

   if (HEADER(ldptr).f_symptr)
   {
      str_start = OFFSET(ldptr) + HEADER(ldptr).f_symptr + (HEADER(ldptr).f_nsyms * SYMESZ);
      if (fseek(IOPTR(ldptr), str_start, BEGINNING) == 0)
      {
         char *cp = _cofl_retstr;
         int cnt = 0;
         int cr;
         long str_size;

         /* This should be buffered better! */

         if (fread(&str_size, 1, 4, IOPTR(ldptr)) != 4)
            return (char *) NULL;

         if ((symbol->n_offset >= str_size) ||
             (fseek(IOPTR(ldptr), symbol->n_offset-4, CURRENT) != 0))
            return (char *) NULL;

         while (((cr = fgetc(IOPTR(ldptr))) != NULL) &&
                (!feof(IOPTR(ldptr))) &&
                (cnt < COFL_STR_MAX))
         {
            *cp++ = cr;
            cnt++;
         }

         if (feof(IOPTR(ldptr)) ||
             (ftell(IOPTR(ldptr)) > (str_start + str_size)) ||
             (cp == _cofl_retstr)) /* no name read */
         {
            return (char *)NULL;
         }

         if (cnt >= COFL_STR_MAX)
            _cofl_retstr[COFL_STR_MAX-1] = (char) NULL;
         else
            *cp = (char) NULL;

         return _cofl_retstr;
      }
   } /* if */

   return (char *) NULL;

} /* ldgetname() */

