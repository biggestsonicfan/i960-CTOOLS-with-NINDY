/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1993, 1994, 1995 Intel Corporation
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
/*)ce*/
/* $Header: /ffs/p1/dev/src/hdil/common/RCS/cop.h,v 1.2 1995/08/21 22:53:32 cmorgan Exp $$Locker:  $ */

#ifndef HDI_COP_H
#define HDI_COP_H

/*
 * C library support, logical request codes.
 */
#define	DOPEN	0x40
#define	DREAD	0x41
#define	DWRITE	0x42
#define	DSEEK	0x43
#define	DCLOSE	0x44

#define	DTIME	0x48
#define	DSYSTEM	0x49
#define	DSTDARG	0x4A

#define	DRENAME	0x50
#define	DUNLINK	0x51

#define	DSTAT	0x58
#define	DFSTAT	0x59
#define DISATTY 0x5A


#endif /* HDI_COP_H */
