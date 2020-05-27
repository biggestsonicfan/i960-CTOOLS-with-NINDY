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

#ifndef	DECODE_H
#define DECODE_H
#define ALPHA		0x10000
#define MIN_CHAR	256
#define MIN_SHORT	256
#define PREFIX_LENGTH  	4
#define ROWS     	(1 << PREFIX_LENGTH)
#define MASK(x)		((1 << x) - 1)
#define HEADER_SLOTS	2

typedef unsigned short	Ushort;
typedef unsigned char	Uchar;

/* The structure is written by the linker into the cave section, 
   followed by short and char alphabets. The short alphabet's offset 
   from the beginning of the structure is computed as

	short_offset = DECODE_TABLE_SIZE;

   The char alphabet follows the short alphabet and its offset is

	char_offset = DECODE_TABLE_SIZE + short_alpha * 2;
*/

typedef struct
{
	Ushort	char_rows;
	Ushort	short_alpha;
	Ushort	suffix_number[ROWS];
	Uchar	suffix_length[ROWS];
} Decode_table;

#define DECODE_TABLE_SIZE	(4 + ROWS * 3)
#endif
