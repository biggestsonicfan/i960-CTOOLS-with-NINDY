/* d2xt.c  Conversion routines back-and-forth between double-precision
   and IEEE extended-precision numbers.
   Contributed by Tim Carver, Intel Corporation (timc@ibeam.intel.com)
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

static int dbig_endian ()
{
  /* Return 0 if host doubles are little-endian, 1 if big endian.
     Should work for most formats of radix 2/8/16. */

  static double d = 16.0;	/* lsw of this has gotta be zero ... */
  return ((unsigned *) &d)[0] != 0;
}

void
dbl_to_edbl (f)
unsigned f[];
{
  unsigned t,s;
  int lsw,msw;

  /* f points to a 3 word area, the first two words of which are
     an IEEE double in host byte and word order.

     Convert it to an i960 double-extended real in host byte and
     word order. */

  if (dbig_endian())
  { msw = 0;
    lsw = 1;
  }
  else
  { msw = 1;
    lsw = 0;
  }

  t = f[msw];
  s = (t & 0x80000000) >> 16;	/* Sign bit */
  t &= 0x7fffffff;

  if (t | f[lsw])
  { /* Non-zero */
    if (t == 0)
    { /* Denormalized */
      f[msw] = (t << 12) | (f[lsw] >> 22);
      f[lsw] <<= 12;
      t = -1022;

      /* Normalize it */
      while ((f[msw] & 0x80000000) == 0)
      { f[msw] = (f[msw] << 1) | ((f[lsw] & 0x80000000) != 0);
        f[lsw] <<= 1;
        t--;
      }
    }
    else
    { /* Normal/Infinity/Nan */

      f[msw] = ((t << 11) | 0x80000000) | (f[lsw] >> 21);
      f[lsw] <<= 11;
      t = (t >> 20) - 1023;
      if (t == 1024) /* Long real infinity */
	      t = 16384; /* Extended real infinity. */
    }
    t += 16383;	/* Final bias */
  }
  else
    f[msw] = 0;

  t |= s;

  if (msw==0)
  { f[2] = f[1];	/* Need big-endian result */
    f[1] = f[0];
    f[0] = t;
  }
  else
    f[2] = t;		/* Little-endian result  */
}

void
edbl_to_dbl (p)
unsigned p[];
{
  unsigned s;
  int lsw,msw,exp,e;

  /* p points to a 3 word area, which is an i960 double-extended
     real in host byte and word order.

     Convert it to an i960 double-extended real in host byte and
     word order at p.  Zero out p[2].  */

  if (dbig_endian())
  { exp = 0;
    msw = 1;
    lsw = 2;
  }
  else
  { exp = 2;
    msw = 1;
    lsw = 0;
  }

  s = (p[exp] & 0x8000) << 16;	/* Sign */
  e = p[exp]  & 0x7fff;

  if (e==0)
    p[lsw]=p[msw]=0;			/* 0.0 or denorm */

  else if ((p[msw] & 0x80000000) == 0)
    p[lsw]=p[msw]=0;			/* Reserved-encoding */

  else
  {
    int rnd,isnan;

    isnan = (e==0x7fff) && (p[msw] != 0x80000000 || p[lsw] != 0);

    /* Get true expt ... */
    e -= 16383;

    /* Get rid of explict MSBit */
    p[msw] &= 0x7fffffff;

    /* Round to nearest even; target precision is 53 bits.  Note that
       we chop Nans here, too.
    */
    rnd = !isnan && (p[lsw] & 0x400) && ((p[lsw] & 0x800) || (p[lsw] & 0x3ff));

    p[lsw] = (p[lsw] >> 11) | (p[msw] << 21);
    p[msw] = (p[msw] >> 11);

    /* If we round up, if we get wrap, increment characteristic
       and clear the fraction. */
    if (rnd && ++p[lsw] == 0 && (++p[msw] & 0x00100000))
    { p[msw] = p[lsw] = 0;
      e++;
    }

    if (e <= -1023)
      p[msw]=p[lsw]=e=0;	/* Flush all denorms to 0.0 for now.        */

    else if (isnan)
      e=0x7ff;			/* Nan.  Set e to max, fraction was chopped */

    else if (e >= 0x7ff)	/* Infinity.  Set fraction to 0, e to max   */
    { p[lsw]=p[msw]=0;
      e = 0x7ff;
    }

    else
      e += 1023;		/* Normal case.  Fraction was rounded       */

    p[msw] |= e << 20;
  }

  p[msw] |= s;

  if (lsw==2)
  { p[0] = p[1];		/* Need big-endian double result */
    p[1] = p[2]; 
  }
  p[2] = 0;
}

#ifdef TEST
double onep1 = 1.1;
long double tiny = -.1e-300;
long double nan;

main ()
{
  double d;
  unsigned x[32];
  unsigned *p = &nan;

#if 1
  *((double *) x) = onep1;
  printf ("%x,%x,%x,%g\n", x[0],x[1],x[2],*((double *) x));
  dbl_to_edbl (x);
  printf ("%x,%x,%x,%Lg\n", x[0],x[1],x[2],*((long double *) x));
  edbl_to_dbl (x);
  printf ("%x,%x,%x,%g\n", x[0],x[1],x[2],*((double *) x));

  *((long double *) x) = tiny;
  printf ("%x,%x,%x,%Lg\n", x[0],x[1],x[2],*((long double *) x));
  edbl_to_dbl (x);
  printf ("%x,%x,%x,%g\n", x[0],x[1],x[2],*((double *) x));
  dbl_to_edbl(x);
  printf ("%x,%x,%x,%Lg\n", x[0],x[1],x[2],*((long double *) x));
#endif

#if 0
  p[0] = 0x1;
  p[1] = 0x80000000;
  p[2] = 0x7fff;

  *((long double *) x) = nan;
  printf ("%x,%x,%x,%Lg\n", x[0],x[1],x[2],*((long double *) x));

  printf ("mine...\n");
  edbl_to_dbl (x);
  printf ("%x,%x,%x,%g\n", x[0],x[1],x[2],*((double *) x));
  dbl_to_edbl(x);
  printf ("%x,%x,%x,%Lg\n", x[0],x[1],x[2],*((long double *) x));

  printf ("machine...\n");
  *((double *) x) = nan;
  printf ("%x,%x,%x,%g\n", x[0],x[1],x[2],*((double *) x));
  *((long double *) x) = *((double *) x);
  printf ("%x,%x,%x,%Lg\n", x[0],x[1],x[2],*((long double *) x));
#endif
}
#endif
