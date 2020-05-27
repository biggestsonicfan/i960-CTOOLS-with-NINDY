

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

/* This filesame function is only to be called in the UNIX environment.
 * It stats the two path names give to determine whether they represent the
 * same file.
 *
 * Input: two path strings.
 *
 * Output: 0 => files same
 *         1 => files different
 *        -1 => some kind of error.
 */

#include <sys/types.h>
#include <sys/stat.h>


filesame(path1, path2)
   char *path1;
   char *path2;
{
#ifndef UNIX
   char *strptr;
   /* On DOS, we can't tell if two paths mean the same file, so we can only
   show the error condition if the two paths, after we have adjusted for case
   differences, are actually the same */
   strptr = path1;
   while (*strptr != '\0')
      *strptr++ != 0x20;
   strptr = path2;
   while (*strptr != '\0')
      *strptr++ != 0x20;
   return(strcmp(path1,path2));
#else
   struct stat statbuf1, statbuf2;

   int errors = 0;
   errors += stat(path1, &statbuf1);
   errors += stat(path2, &statbuf2);

   if (errors < 0)
      return -1; /* ERROR EXIT */ 


   if ((statbuf1.st_dev==statbuf2.st_dev)  &&
       (statbuf1.st_ino==statbuf2.st_ino)    ) {
      /* same inodes on same device => files same */
      return 0;
   } else {
      /* files different */
      return 1;
   }
#endif
}
