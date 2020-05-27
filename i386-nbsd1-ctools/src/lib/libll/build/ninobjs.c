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
 * This file contains entry points required for GNU/960 HLL calls,
 * but not for ic960. They are basically NINDY-compatibility entry
 * points.
 */

#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>

/***************************************/
/* tell                                */
/*                                     */
/* tell where the file pointer is      */
/***************************************/
tell(fd)
int fd;
{
        return lseek(fd, (long)0, SEEK_CUR);
}

/***************************************/
/* System                              */
/*                                     */
/***************************************/
systemd(string)
char *string;
{
        return system(string);
}

/***************************************/
/* Spawnd                              */
/*                                     */
/***************************************/
spawnd(path, argv)
char *path;
char *argv[];
{
        return ERROR;
}

/*
 * "posix" stubs for GCC960 libraries: getpid, link
 */

int getpid()
{
  return 1;
}

int
link(char *s1, char *s2)
{
  return -1;    /* it never works */
}

/***************************************/
/* Unmask                              */
/*                                     */
/***************************************/
unmask(mode)
int mode;
{
        return (-1); /* not used in libll */
}

