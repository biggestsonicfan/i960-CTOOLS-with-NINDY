
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
 * ldopen()
 */

#include <stdio.h>
#include "ldfcn.h"

static int _cofl_next_fnum_ = 1;

LDFILE *
ldopen(filename, ldptr)
char *filename;
LDFILE *ldptr;
{
   LDFILE *retptr;
   char arnam_ck[SARMAG];
   ARCHDR arch_hdr;
   AR_HDR a_hdr;
   char *cp;
   long cnt;

   if (ldptr != (LDFILE *)NULL)
   {
      if (TYPE(ldptr) == ARTYPE)
      {
         /* non-NULL, archive ldptr. OFFSET() should have been set up by
          * a previous call to ldclose()
          */
         fseek(IOPTR(ldptr), OFFSET(ldptr), BEGINNING);
         if (fread(&HEADER(ldptr), 1, sizeof(HEADER(ldptr)), IOPTR(ldptr)) != sizeof(HEADER(ldptr)))
         {
            /* couldn't read next member's header */
            return NULL;
         }
         return ldptr;
      } else {
         /* non-NULL ldptr, but not an archive */
         return NULL;
      }
   } else {
      if ((retptr = (LDFILE *)malloc(LDFSZ)) == NULL)
      {
         /* error - can't allocate space for structure */
         return(NULL);
      }
#if defined( DOS ) || defined( __GNU960C )
      if (!filename || ((IOPTR(retptr) = fopen(filename, "rb")) == NULL))
#else
      if (!filename || ((IOPTR(retptr) = fopen(filename, "r")) == NULL))
#endif
      {
         /* error - couldn't open file */
         return(NULL);
      }

      if (fread(arnam_ck, 1, SARMAG, IOPTR(retptr)) != SARMAG)
      {
         /* error - couldn't read header */
         return NULL;
      }

      if (strncmp(arnam_ck, ARMAG, SARMAG) == 0)
      {
         /* this is an archive file */
         if (fread(&a_hdr, 1, AR_HSZ, IOPTR(retptr)) != AR_HSZ)
         {
            return NULL;
         }

         if (strncmp(a_hdr.ar_name, "/ ", 2) == 0)
         {
            /* the first member is the symbol table - skip it */
            fseek(IOPTR(retptr), sgetl(a_hdr.ar_size), CURRENT);
            _cofl_make_addr_even(retptr);
            if (fread(&a_hdr, 1, AR_HSZ, IOPTR(retptr)) != AR_HSZ)
            {
               return NULL;
            }
         }

         if (strncmp(a_hdr.ar_name, I960ARNM, I960ARNMSZ) == 0)
         {
            /* this is a "spelling section", read it and go on */
            ARSTRSIZE(retptr) = sgetl(a_hdr.ar_size);
            SPELLINGS(retptr) = (char *)malloc(ARSTRSIZE(retptr));
            if (fread(SPELLINGS(retptr), 1, ARSTRSIZE(retptr), IOPTR(retptr)) != ARSTRSIZE(retptr))
            {
               return NULL;
            }
            /* the names in the SPELLING pool are terminated with "\n\0", which makes them
             * print out funny (and not match AT&T). We go through and turn the '\n's to '\0's. 
             */

            for (cp = SPELLINGS(retptr), cnt = 0; cnt < ARSTRSIZE(retptr); ++cp, ++cnt)
            {
               if (*cp == '\n')
                  *cp = NULL;
            }

            _cofl_make_addr_even(retptr);
            if (fread(&a_hdr, 1, AR_HSZ, IOPTR(retptr)) != AR_HSZ)
            {
               return NULL;
            }
         }

         /* OK - Now we're pointed at the first member of the archive */

         retptr->_fnum_    = _cofl_next_fnum_++;
         OFFSET(retptr)    = ftell(IOPTR(retptr));
         TYPE(retptr)      = ARTYPE;

      } else {
         /* this is a non-archive file */
         rewind(IOPTR(retptr));

         retptr->_fnum_    = _cofl_next_fnum_++;
         OFFSET(retptr)    = 0;
         TYPE(retptr)      = I960ROMAGIC; /* TVTYPE ??? */
         SPELLINGS(retptr) = NULL;
         ARSTRSIZE(retptr) = 0;
      }

      if (ldfhread(retptr, &HEADER(retptr)) == FAILURE)
      {
         fprintf(stderr, "error: couldn't read file %s header\n", filename);
         return (NULL);
      }

      if (!ISCOFF(HEADER(retptr).f_magic))
      {
         fprintf(stderr, "warning: file has unknown magic number 0x%04x\n", HEADER(retptr).f_magic);
      }

      return (retptr);
   }
   /*NOTREACHED*/
} /* ldopen() */
