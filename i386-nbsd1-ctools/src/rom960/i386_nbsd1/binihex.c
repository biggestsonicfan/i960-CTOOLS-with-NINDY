
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
 * binihex -- convert binary to intel hex record
 *	ibuf = data buffer pointer
 *	obuf = output buffer pointer
 *	addr = load address
 *	len = number of bytes
 *  type = record type
 */


#define ACASE 'A'	/* base for hex digits a-f */

binihex(ibuf, obuf, addr, len, type)
char *ibuf, *obuf;
unsigned addr, len, type;
{
	register c, chksum;

	chksum = (addr & 0377) + (addr >> 8);	/* checksum includes address */
	*obuf++ = ':';				/* start of record char */
	hcon(obuf, len, 2);			/* byte count */
	chksum += len;
	obuf += 2;
	hcon(obuf, addr, 4);		/* load address */
	obuf += 4;
	hcon(obuf, type, 2);		/* record type */
	chksum += type;
	obuf += 2;

	while (len--) {
	    c = (unsigned)*ibuf++;
	    chksum += c;
	    hcon(obuf, c, 2);		/* append data byte */
		obuf += 2;
	}
	hcon(obuf, -chksum, 2);	/* append checksum */
	*(obuf+2) = '\0';		/* terminate string with null */
}

/* hcon -- convert binary to ascii hex
 *	obuf = output buffer (char)
 *	n = number to be converted
 *	c = nibble count
 */
hcon(obuf, n, c)
char *obuf;
unsigned c, n;
{
	register d;

	if (!c)
		return;

	hcon(obuf, n/16, --c);
	d = n%16;
	if (d > 9)
		*(obuf+c) = d - 10 + ACASE;
	else
		*(obuf+c) = d + '0';
}
