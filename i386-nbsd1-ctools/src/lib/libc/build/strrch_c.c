/*******************************************************************************
 * 
 * Copyright (c) 1993 Intel Corporation
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
 ******************************************************************************/

/*
 * strrch : string searching routines.
 * (and non-ANSI variants strpos, strrpos)
 *
 * Notes:  A naive implementation.  Should use the 960
 *         "scanbyte" instruction and be inlined for benchmarking.
 *
 * Andy Wilson, 3-Oct-89.
 */

#include <string.h>

char *
(strrchr)(const char* s, int c)
{
  char *lastc=(char *)0;

  /*
   * are we searching for a null?  if so,
   * this routine is the same as strchr.
   */
  if (c=='\0')
    return strchr(s, c);

  /*
   * If not, walk the string until we find a null,
   * returning a pointer to the last char to match 'c'.
   */
  while (*s != '\0')
    {
      if (*s == c)
	lastc = (char *)s;
      s++;
    }
  return lastc;
}

int
(strpos)(const char* s, char c)
{
  char *p;

  p = strchr(s, c);
  if (p)
    return (p-s);
  else
    return -1;
}

int
(strrpos)(const char* s, char c)
{
  char *p;

  p = strrchr(s, c);
  if (p)
    return (p-s);
  else
    return -1;
}
