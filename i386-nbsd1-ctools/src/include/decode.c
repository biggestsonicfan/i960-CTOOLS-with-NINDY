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
 *	decode.c - decompression used by CAVE run-time system.
 *
 */

#include "cc_info.h"
#include "decode.h"

/* Predefined symbol initialized by the linker to point to the decompression
   tables */

#ifndef HOST_DECODE
Decode_table* const _decompression_table = 0;
#endif

void
_decompress_buffer(out, in, sz, dtable)
Uchar* out; 
Uchar* in; 
int sz; 
Decode_table* dtable;
{
	unsigned* in_first = (unsigned*) in;
	Uchar* out_stop = out + sz;
	Ushort* short_alphabet = (Ushort*)((Uchar*) dtable + DECODE_TABLE_SIZE);
	Uchar* char_alphabet = (Uchar*) (short_alphabet + dtable->short_alpha);
	Ushort char_rows = dtable->char_rows, rem = 0;
	unsigned word;

	while (out < out_stop)
	{
		int num;
		unsigned prefix = 0;
		unsigned suffix = 0;
		int len = PREFIX_LENGTH;

		if (rem < len)
		{
			if (rem)
			{
				prefix = word & MASK(rem);
				len -= rem;
				prefix <<= len;
			}
			rem = 32;
#ifdef HOST_DECODE
			CI_U32_FM_BUF (((Uchar*)in_first), word); in_first++;
#else
			word = *in_first++;
#endif
		}
		rem -= len;
		prefix |= (word >> rem) & MASK(len);

		len = dtable->suffix_length[prefix];
		if (rem < len)
		{
			if (rem)
			{
				suffix = word & MASK(rem);
				len -= rem;
				suffix <<= len;
			}
			rem = 32;
#ifdef HOST_DECODE
			CI_U32_FM_BUF (((Uchar*)in_first), word); in_first++;
#else
			word = *in_first++;
#endif
		}
		rem -= len;
		suffix |= (word >> rem) & MASK(len);

		num = suffix + 1;
		if (prefix)
			num += dtable->suffix_number[prefix - 1];
		if (prefix < char_rows)
			*out++ = char_alphabet[num - 1];
		else
		{
#ifdef HOST_DECODE
			Ushort sym;
			if (char_rows)
				num -= dtable->suffix_number[char_rows - 1];
                        sym = short_alphabet[num - 1];
			CI_U16_TO_BUF (out, sym); out += 2;
#else 
			unsigned char* t;
			if (char_rows)
				num -= dtable->suffix_number[char_rows - 1];
			t = (unsigned char*) &short_alphabet[num - 1];
			*out++ = t[0];
			*out++ = t[1];
#endif
		}
	}
}
