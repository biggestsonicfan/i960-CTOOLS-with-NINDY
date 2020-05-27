
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
 * this file contains routines easing the task of
 * manipulating arrays of bits.
 * max g webb Tue May 19 18:06:07 PDT 1987
 *
 * ASSUMPTION: N%8 == 0 for both routines below!
 */

#include <stdio.h>
#include "paths.h"

bit_to_byte(pbit,pbyte,n)
char *pbit,*pbyte;
int n;
{
	int i;
	for (i=0; i<n; i++)
		pbyte[i] = (pbit[i/8] >> (i%8)) & 1;
}

byte_to_bit(pbyte,pbit,n)
char *pbyte,*pbit;
int n;
{
	int i;
	for (i=0; i<n/8; i++)
		pbit[i] = 0;
	for (i=0; i<n; i++)
		pbit[i/8] |= (pbyte[i] << (i%8));
}

#if 0

/* note -- here n is in BYTES */
bit_and(pbit1,pbit2,n)
char *pbit1,*pbit2;
int n;
{
	int i;
	for (i=0; i<n; i++)
		if (pbit1[i] & pbit2[i])
			return -1;
	return 0;
}
#endif


#ifdef TEST
main ()
{
	int wordin, wordout, i;
	char bits[32];
	while (1) {
		fprintf(STDERR,"input hex word to use:\n");
		scanf("%x",&wordin);
		bit_to_byte(&wordin, bits, 32);
		for (i=0; i<32; i++)
			fprintf(STDERR," %d",bits[i]);
		byte_to_bit(bits, &wordout, 32);
		fprintf(STDERR,"\nwordin = %x, wordout = %x\n", wordin,wordout);
	}
}
#endif
