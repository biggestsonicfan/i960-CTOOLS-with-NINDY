/*******************************************************************************
 * 
 * Copyright (c) 1993-1995 Intel Corporation
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

/* strerror - construct an error message
 * Copyright (c) 1984,85,87,88 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <string.h>
#include <stddef.h>
#include <errno.h>

char *
strerror(int errnum)
{
  int t = errnum;
  char *ret = " ";

  if (t)
    t = errno;

  switch (t)
  {
    case 0 : ret = " "; break;
    case 1 : ret = " "; break;
    case 2 : ret = "file or path not found "; break;
    case 3 : ret = " "; break;
    case 4 : ret = " "; break;
    case 5 : ret = " "; break;
    case 6 : ret = " "; break;
    case 7 : ret = "size of argument too large "; break;
    case 8 : ret = "file is not executable "; break;
    case 9 : ret = "invalid file descriptor "; break;
    case 10: ret = " "; break;
    case 11: ret = " "; break;
    case 12: ret = "out of memory "; break;
    case 13: ret = "file access denied "; break;
    case 14: ret = " "; break;
    case 15: ret = " "; break;
    case 16: ret = " "; break;
    case 17: ret = "file already exists "; break;
    case 18: ret = "attempt to move a file to a different device "; break;
    case 19: ret = " "; break;
    case 20: ret = " "; break;
    case 21: ret = " "; break;
    case 22: ret = "invalid argument or operation "; break;
    case 23: ret = " "; break;
    case 24: ret = "too many open files "; break;
    case 25: ret = " "; break;
    case 26: ret = " "; break;
    case 27: ret = " "; break;
    case 28: ret = "device is full "; break;
    case 29: ret = " "; break;
    case 30: ret = " "; break;
    case 31: ret = " "; break;
    case 32: ret = " "; break;
    case 33: ret = "argument out of range "; break;
    case 34: ret = "result out of range "; break;
    case 35: ret = " "; break;
    case 36: ret = "locking violation "; break;
    case 37: ret = "Bad signal vector "; break;
    case 38: ret = "Bad free pointer "; break;

    default:
    case 39: ret = "Illegal error number "; break;
  }
  return ret;
}
