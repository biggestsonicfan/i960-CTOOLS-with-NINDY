/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1993, 1994 Intel Corporation
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
/*
 * Sets up an Initialization Boot Record for the 80960CA
 *
 * THE INITIALIZATION BOOT RECORD MUST BE LOCATED AT ADDRESS
 * 0xFFFFFF00 BY THE LINKER!
 */

#include "common.h"
#include "i960.h"
#include "this_hw.h"

extern void start_ip();
extern PRCB rom_prcb;
extern char checksum[];		/* symbol calculated at link time */

const IBR init_boot_record = {
#if BIG_ENDIAN_CODE
	BYTE_N(0, REGION_E_CONFIG) << 24,
	BYTE_N(1, REGION_E_CONFIG) << 24,
	BYTE_N(2, REGION_E_CONFIG) << 24,
	BYTE_N(3, REGION_E_CONFIG) << 24,
#else
	BYTE_N(0, REGION_E_CONFIG),
	BYTE_N(1, REGION_E_CONFIG),
	BYTE_N(2, REGION_E_CONFIG),
	BYTE_N(3, REGION_E_CONFIG),
#endif /* __i960_BIG_ENDIAN__ */

	start_ip,
	&rom_prcb,
	{-2,		/* Checksum word #1 */ /* WARNING *********************/
				/* DO NOT change -2 as GDB depends on this value to determine */
				/* endieness of the target */
	0,			/* Checksum word #2 */
	0,			/* Checksum word #3 */
	0,			/* Checksum word #4 */
	0,			/* Checksum word #5 */
	(int)checksum}		/* Checksum word #6:
				 * -(start_ip+rom_prcb)
				 * must be calculated by the linker
				 */
};
