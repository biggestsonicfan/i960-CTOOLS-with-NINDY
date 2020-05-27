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

#include <string.h>

typedef struct
{
  long a;
  long b;
  long c;
  long d;
} QWORD;

typedef struct
{
  long a;
  long b;
} DWORD;

void *
(memmove)(void* dest, const void *src, size_t len)
{
  const char* lasts;
  char* lastd;
  void *retval;
  
  const char* char_src = src;
  char* char_dest = dest;
  /*
   * check for overlap; if not, use the
   * (presumably faster) memcpy routine.
   */
  lasts = char_src + (len-1);
  lastd = char_dest + (len-1);
  if (((char_dest < char_src) || (char_dest > lasts)) &&
      ((lastd < char_src) || (lastd > lasts)))
    return memcpy(char_dest, char_src, len);
  
  /*
   * no joy; copy the strings byte-by-byte
   * in the appropriate order (increasing byte
   * addresses if char_dest<char_src, decreasing if char_dest>char_src).
   */
  
  retval = char_dest;
  if (char_dest < char_src)
    while (len--)
      *char_dest++ = *char_src++;
  else
    while (len--)
      *lastd-- = *lasts--;
  
  return retval;
}

void *
(memcpy)(void* dest, const void* src, size_t len)
{
	void *retval=dest;
	register int i;

	register QWORD *qs;
	register QWORD *qd;

	register DWORD *ds;
	register DWORD *dd;

	register long *ls;
	register long *ld;

	register short *ss;
	register short *sd;

	register char *cs;
	register char *cd;

/* quad word moves */

	if ((((long)dest | (long)src) & 0xF)==0)
	{
		qs = (QWORD *) src;
		qd = (QWORD *) dest;

		while ( len >= sizeof(QWORD))
		{
			*qd++ = *qs++;
			len -= sizeof(QWORD);
		}
		if (len==0) return retval;
		dest = (void *) qd;
		src  = (void *) qs;
	}
  
/* double word moves */

	if ((((long)dest | (long)src) & 0x7)==0)
	{
		ds = (DWORD *) src;
		dd = (DWORD *) dest;

		while ( len >= sizeof(DWORD))
		{
			*dd++ = *ds++;
			len -= sizeof(DWORD);
		}
		if (len==0) return retval;
		dest = (void *) dd;
		src  = (void *) ds;
	}

/* word moves */

	if ((((long)dest | (long)src) & 0x3)==0)
	{
		ls = (long *) src;
		ld = (long *) dest;

		while ( len >= sizeof(long))
		{
			*ld++ = *ls++;
			len -= sizeof(long);
		}
		if (len==0) return retval;
		dest = (void *) ld;
		src  = (void *) ls;
	}

/* short moves */

	if ((((long)dest | (long)src) & 0x1)==0)
	{
		ss = (short *) src;
		sd = (short *) dest;

		while ( len >= sizeof(short))
		{
			*sd++ = *ss++;
			len -= sizeof(short);
		}
		if (len==0) return retval;
		dest = (void *) sd;
		src  = (void *) ss;
	}

/* byte moves */

	cs = (char *) src;
	cd = (char *) dest;

	while (len--)
		*cd++ = *cs++;
  return retval;
}

/*
 * bcopy : Berkleyism.
 * like memcpy with operands reversed.
 */
void
(bcopy)(char* src, char* dest, int len)
{
  (void)memmove(dest, src, len);
}
