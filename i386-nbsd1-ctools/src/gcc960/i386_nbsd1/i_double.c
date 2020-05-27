#include "config.h"

#ifdef IMSTG
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "real.h"
#include "rtl.h"
#include "tree.h"

#include "i_double.h"

/*
   This file contains conversion routines to pack/unpack host doubles
   from/to an extended format.  This code is used by glob_inl.c to
   get double constants mashed into a host-independent form which
   is capable of representing all (non-Nan) numbers
   on a given host.  Precision is never lost converting INTO the
   extended format;  however, if a double is built from the extended
   form on a host which has different floating point characteristics
   than the one which wrote it, precision will be lost (gracefully).

   NaNs are converted to zero.  Infinities and denormals cause
   lossage only if extracted on a non-IEEE host;  they degrade to
   the maximum representable number and 0, respectively.
*/

#ifdef IMSTG_REAL

double
d_to_d (d)
double d;
{ return d;
}

/* Convert v1 to an int at *pn.  Return 1 if v1 was integral and there
   was no loss converting to signed integer; 0 otherwise. */

static int
to_int (v1, pn)
REAL_VALUE_TYPE v1;
int *pn;
{
  int n, h,ret;
  REAL_VALUE_TYPE v2;

  REAL_VALUE_TO_INT (&n, &h, v1);
  REAL_VALUE_FROM_INT (v2, n, h);

  *pn = n;
  ret = (((h==0 && n >= 0) || (h==-1 && n<0)) && REAL_VALUES_EQUAL (v1, v2));
  return ret;
}

/* Convert v to an int at *pn.  Return 1 if v1 was integral and there
   was no loss converting to signed integer; 0 otherwise. */

int
const_double_to_int (v,pn)
rtx v;
int* pn;
{
  int d;
  enum machine_mode m = GET_MODE(v);

  if (d=(GET_CODE(v)==CONST_DOUBLE && GET_MODE_CLASS(m)==MODE_FLOAT))
    /* Only accept floating consts which are really integers */
  { REAL_VALUE_TYPE r;
    REAL_VALUE_FROM_CONST_DOUBLE (r,v);

    d = to_int (r, pn);
  }

  return d;
}

/* Convert v to an int at *pn.  Return 1 if v1 was integral and there
   was no loss converting to signed integer; 0 otherwise. */
int
tree_real_cst_to_int(v,pn)
tree v;
int* pn;
{
  int d;

  if (d=(TREE_CODE(v)==REAL_CST))
    /* Only accept floating consts which are really integers */
    d = to_int (TREE_REAL_CST(v), pn);

  return d;
}

#else
#if HOST_FLOAT_FORMAT == VAX_FLOAT_FORMAT
#define DSGN(D) (( ((unsigned char *)&(D))[1] ))
#else
int dbig_endian ()
{
  /* Return 0 if host doubles are little-endian, 1 if big endian.
     Should work for most formats of radix 2/8/16. */

  static double d = 16.0;		/* lsw of this has gotta be zero ... */
  return ((unsigned *) &d)[0] != 0;
}

#define DLSW(D) (( ((unsigned *)&(D))[dbig_endian()] ))
#define DMSW(D) (( ((unsigned *)&(D))[!dbig_endian()] ))
#define DSGN(D) (( ((unsigned char *) &DMSW(D))[dbig_endian() ? 0 : 3] ))

#endif

int empty (p)
char *p;
{
}

int (*force_dstore) () = empty;

/* Convert from host double to special format.  Special format
   has 64 fraction bits, 16 exponent bits.  This code should be
   portable to most unix hosts;  furthermore, the format should
   be able to exactly represent all normal numbers representable with
   host doubles, assuming host doubles are 8 bytes. */
   
void
fm_double (d, s, e, f)
double d;
unsigned* s;
unsigned* e;
unsigned  f[];
{
  int expt;

  force_dstore (&d);	/* If there is a write pending to d, force it */
  
  /* Remember sign bit and clear it. */
  *s = DSGN(d) & 0x80;
  DSGN(d) &= 0x7f;
 
  /* Get d as [2^31, 2^32) * 2^(expt-32) */

#if HOST_FLOAT_FORMAT == IEEE_FLOAT_FORMAT

  /* Nans and Infinities break frexp. */
  if ((DMSW(d) & 0x7ff00000) == 0x7ff00000)
    if (DLSW(d) != 0 || DMSW(d) != 0x7ff00000)
    { d = 0;	/* NaN */
      expt = 0;
    }
    else
    { d = .5;	/* Infinity; have to avoid frexp cause it breaks */
      expt = 1025;
    }

  else
    d = frexp (d, &expt);

#else
  d = frexp (d, &expt);
#endif

  /* This is to work around broken frexps for early sunOS's which
     return 1.0 sometimes. */

  if (d == 1.0)
  { d = 0.5;
    expt++;
  }

  if (d == 0)
    f[0] = f[1] = expt = 0;
  else
  {
    d *= DTWO32;
  
    /* Truncate to get 32 most significant bits */
    f[0] = TRUNCDU(d);
  
    /* To get the least significant 32 bits, get difference between d and
       its integral part, multiply by 2^32 to put it on [0,2^32), and
       truncate to integer.  The difference is guaranteed to be exact,
       because the exponents are the same; hence, there is no
       pre-normalization lossage. */
  
    f[1] = TRUNCDU ((d-FLOATUD(f[0])) * DTWO32);
    expt -= 64;
  }
  /* Value of original number is now (f[0]*2^32 + f[1]) * 2^expt */
  *e = expt;
}

double
to_double (s, e, f)
unsigned s;
unsigned e;
unsigned *f;
{
  double d = ldexp (FLOATUD(f[0]) * DTWO32 + FLOATUD(f[1]), (short)e);
  DSGN(d) |= (unsigned char)s;

  return d;
}

float ftrunc;
float* pftrunc = &ftrunc;

double dbl_to_flt (d)
double d;
{
  ftrunc = d;
  return *pftrunc;
}

double get_double (op)
rtx op;
{
  union
  { double d;
    int i[2];
  } cheat1, cheat2;

  /* op's value is arranged properly for the host, but I
     am not sure it is aligned properly for a dereference,
     so we don't just say *((double *) &CONST_DOUBLE_LOW (op)). */

  cheat1.i[0] = CONST_DOUBLE_LOW (op);
  cheat1.i[1] = CONST_DOUBLE_HIGH (op);
  if (GET_MODE(op) == SFmode)
  {
    cheat2.d = dbl_to_flt(cheat1.d);
    assert (cheat2.i[0] == cheat1.i[0] && cheat2.i[1] == cheat1.i[1]);
    cheat1.d = cheat2.d;
  }
  return cheat1.d;
}

int
const_double_to_int (v,pn)
rtx v;
int* pn;
{
  int d,n;
  enum machine_mode m = GET_MODE(v);
  extern double get_double();
  double dv = get_double(v);

  n = *pn;

  if (d=(GET_CODE(v)==CONST_DOUBLE && GET_MODE_CLASS(m)==MODE_FLOAT))
    if (IS_CONST_ZERO_RTX(v))
      n = 0;
    else	/* Only accept floating consts which are really integers */
      if ((n = (int)dv)==0 || ((double) n) != dv)
        d = 0;

  *pn = n;

  return d;
}

int
tree_real_cst_to_int (v,pn)
tree v;
int* pn;
{
  int d,n;

  n = *pn;

  if (d=(TREE_CODE(v)==REAL_CST))
  { double dv = TREE_REAL_CST(v);
    if (dv == 0)
      n = 0;
    else	/* Only accept floating consts which are really integers */
      if ((n = (int)dv)==0 || ((double) n) != dv)
        d = 0;
  }

  *pn = n;

  return d;
}

#if HOST_FLOAT_FORMAT==IEEE_FLOAT_FORMAT

double
dnegate (d)
double d;
{
  DSGN(d) ^= 0x80;
  return d;
}

dbl_to_ieee_sgl (d, p)
double d;
unsigned p[];
{
  ((float *)p)[0] = d;
}

dbl_to_ieee_dbl (d, p)
double d;
unsigned p[];
{
  force_dstore (&d);	/* If there is a write pending to d, force it */

  p[0] = DMSW(d);
  p[1] = DLSW(d);
}

#endif

#if HOST_FLOAT_FORMAT==VAX_FLOAT_FORMAT

double
dnegate (d)
double d;
{
  return -d;
}

float force_float_cast;
float* pforce_float_cast = &force_float_cast;

dbl_to_ieee_sgl (d, p)
double d;
unsigned p[];
{
  unsigned s, e, t[2];

  /* Make sure we really only have 24 bits */
  force_float_cast = d;
  fm_double ((double) *pforce_float_cast, &s, &e, t);
  assert (t[1] == 0 && (t[0] & 0xFF) == 0);
  p[0] = t[0];

  if (p[0])
  {
    assert (p[0] & 0x80000000);
    p[0] &= 0x7fffffff;

    /* No rounding needed.  Already have 24 bit precision */
    p[0] = (p[0] >> 8) | ((e+64+126) << 23);

    if (s)
      p[0] |= 0x80000000;
  }
  else
    assert (s == 0 && e == 0);
}

dbl_to_ieee_dbl (d, p)
double d;
unsigned p[];
{
  unsigned s, e;
  fm_double (d, &s, &e, p);

  /* VAX doubles have 3 extra fraction bits, so the
     conversion to IEEE may have to round. */

  if (p[0] || p[1])
  {
    int rnd;

    assert (p[0] & 0x80000000);
    p[0] &= 0x7fffffff;

    /* Round to nearest even; target precision is 53 bits */
    rnd = (p[1] & 0x400) && ((p[1] & 0x800) || (p[1] & 0x3ff));

    p[1] = (p[1] >> 11) | (p[0] << 21);
    p[0] = (p[0] >> 11);

    /* If we round up, if we get wrap, increment characteristic
       and clear the fraction. */
    if (rnd && ++p[1] == 0 && (++p[0] & 0x00100000))
    { p[0] = p[1] = 0;
      e++;
    }

    p[0] |= (e+64+1022) << 20;

    if (s)
      p[0] |= 0x80000000;
  }
  else
    assert (s == 0 && e == 0);
}
#endif
#endif

#ifdef DEBUG_DOUBLE

main (argc, argv)
int argc;
char* argv[];
{
  extern double atof ();

  unsigned buf[3];
  double d,d_in;
  int s,e,c,s_in,e_in;
  unsigned buf_in[3];
  char cbuf[255];

  extern char* index();

  while ((gets (cbuf)) != 0)
  {
    if (sscanf (cbuf, "to %x %d %08x %08x", &s_in, &e_in, &buf_in[0], &buf_in[1]))
    { d = to_double (s_in,e_in,buf_in);
      printf ("%.20e\n", d);
      fm_double (d, &s, &e, buf);

      if (s_in != s || e_in != e || buf_in[0] != buf[0] || buf_in[1] != buf[1])
      {
        printf ("in:  %x,%d,%08x,%08x\n",s_in,e_in,buf_in[0],buf_in[1]);
        printf ("out: %x,%d,%08x,%08x\n",s,e,buf[0],buf[1]);
      }
    }
    else if (sscanf (cbuf, "fm %le", &d_in))
    {
      fm_double (d_in, &s, &e, buf);
      printf ("%x,%d,%08x,%08x\n", s, e, buf[0], buf[1]);
      d = to_double (s,e,buf);
  
      assert (DSGN(d) == DSGN(d_in));
  
      if (d_in != d)
      {
        printf ("in: %08x,%08x\n", ((int *) &d_in)[0], ((int *) &d_in)[1]);
        printf ("out:%08x,%08x\n", ((int *) &d)[0], ((int *) &d)[1]);
      }
    }
    else printf ("??\n");
  }
}

#endif

#endif
