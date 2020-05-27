
/*(c****************************************************************************** *
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
 *****************************************************************************c)*/

/*****************************************************************************
 *
 * This file replaces the normal <stat.h> file when building the GNU/960
 * tools on the Macintosh.
 *
 *****************************************************************************/


#ifndef mac_status 
#   define mac_status
#   include <time.h>

#define	S_IFMT	 0170000
#define S_IFREG	 0100000
#define S_IFDIR	 0040000
#define S_IFBLK	 0060000
#define S_IFCHR	 0020000
#define S_IFLNK	 0120000
#define S_IFSOCK 0140000
#define S_IFIFO	 0010000

struct stat {
	time_t st_ctime;  /* creation time */
	time_t st_mtime;  /* modification time */
	long st_size;
	short st_uid;     /* unused, should be 0 */
	short st_gid;	  /* unused, should be 0 */
	short st_mode;    /* File type should be regular file */
	};

int stat(char * path, struct stat *);
int fstat(int fd, struct stat *);

#endif

 
