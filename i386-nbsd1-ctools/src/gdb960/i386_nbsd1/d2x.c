/* d2x.c  This file provides the interface between gdb960 and d2xt.c.
   Contributed by Paul Reger, Intel Corporation (paulr@ibeam.intel.com)
   Copyright 1987, 1988, 1989, 1990, 1991, 1992, 1993
   Free Software Foundation, Inc.

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "defs.h"

/* The routines here have the effect of translating gdb960's
   ieee extended values into double format and vice versa.

   d2xt.c assumes the input word array is in host endian byte and word
   order.  This code will translate to / from this byte ordering for
   gdb960's and d2xt.c's benefit.

   Modification history:

   When:                        Who:  Why:
   Fri Jun 18 09:45:32 PDT 1993 paulr Initial file.

   Paul Reger - Intel

   See d2xt.c.  This is where most of the work is done. */


/* *d is HOST ENDIAN.  Emits x[] in TARGET ordering.*/

void double_to_extended(d,x)
    double *d;
    unsigned x[];
{
    bcopy(d,x,sizeof(double));
    dbl_to_edbl(x);

    /* x is now in HOST ENDIAN byte and word order. */

    /* Swap to little endian, if needed: */
    if (HOST_BIG_ENDIAN) {
	unsigned t = x[0];

	x[0] = x[2];
	x[2] = t;
    }
    SWAP_TARGET_AND_HOST(&(x[0]),sizeof(x[0]));
    SWAP_TARGET_AND_HOST(&(x[1]),sizeof(x[1]));
    SWAP_TARGET_AND_HOST(&(x[2]),sizeof(x[2]));
}

/* x[] is in 960 IEEE Extended format.
   This will have words in little endian format always, and when the
   target is in big endian mode, will have the words in big endian
   format, else the words will be in little endian format as well.
*/ 

void extended_to_double(x,d)
    unsigned x[];
    double *d;
{
    unsigned three_words[3];

    three_words[1] = x[1];
    /* Determine if we have to swap words: */
    if (HOST_BIG_ENDIAN) {
	three_words[0] = x[2];
	three_words[2] = x[0];
    }
    else {
	three_words[0] = x[0];
	three_words[2] = x[2];
    }
    SWAP_TARGET_AND_HOST(&(three_words[0]),sizeof(three_words[0]));
    SWAP_TARGET_AND_HOST(&(three_words[1]),sizeof(three_words[1]));
    SWAP_TARGET_AND_HOST(&(three_words[2]),sizeof(three_words[2]));
    edbl_to_dbl(three_words);
    bcopy(three_words,d,sizeof(double));
}
