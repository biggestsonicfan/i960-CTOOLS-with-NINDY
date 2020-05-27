#ifndef __DOUBLEH__
#define __DOUBLEH__ 1

extern REAL_VALUE_TYPE to_double ();
extern void fm_double ();
#ifndef IMSTG_REAL
extern double get_double ();

/* If these aren`t exact, get a real computer. */
#define DTWO31 (( (double)(32768.0) * (double)(65536.0) ))
#define DTWO32 (( (double)(65536.0) * (double)(65536.0) ))

#define UTWO31 (( (unsigned) 0x80000000 ))

/* We need this because many compilers mishandle ((double) (unsigned I))
   when bit 31 of I is set. */
 
#define FLOATUD(I) (( ((I) & 0x80000000) \
                    ? (((double) ((I) & 0x7fffffff)) + DTWO31) \
                    :  ((double) (I)) ))

/* Some compilers botch ((unsigned) (double D)) when
   D is on [2^31, 2^32). The user of this macro must ensure that
   D is on [0,2^32).

   BTW, Codebuilder "C" even botches conversions from float
   to integer by rounding instead of truncating, unless /zfloatsync
   is used.  Make sure the compiler is built with /zfloatsync when
   codebuilder is the host compiler !
*/

#define TRUNCDU(D) (( ((D)>=DTWO31) \
                      ? (((unsigned)((D)-DTWO31))+UTWO31) \
                      : ((unsigned)(D)) ))
#endif
#endif
