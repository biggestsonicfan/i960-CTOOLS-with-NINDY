#ifndef __UNALIGN_H__
#define __UNALIGN_H__ 

/*
 * This include file defines special macros for accessing short (16-bit)
 * and word (32-bit) quantities on unaligned addresses.  With some 80960
 * processors, unaligned accesses are faster using
 * compiler-scheduled instructions than allowing the microcode and/or
 * bus controller to handle them. 
 *
 * The macros defined are:
 *
 *          type  GET_UNALIGNED                (address, type)
 *          long  GET_UNALIGNED_WORD           (address)
 *          long  GET_UNALIGNED2_WORD          (address)
 *
 *          short GET_UNALIGNED_SHORT          (address)
 * unsigned short GET_UNALIGNED_UNSIGNED_SHORT (address)
 *
 *          type  SET_UNALIGNED                (address, type, expr)
 *          long  SET_UNALIGNED_WORD           (address, expr)
 *          long  SET_UNALIGNED2_WORD          (address, expr)
 *
 *          short SET_UNALIGNED_SHORT          (address, expr)
 * unsigned short SET_UNALIGNED_UNSIGNED_SHORT (address, expr)
 *
 * GET_UNALIGNED, SET_UNALIGNED
 * These macros allow the compiler to choose the best sequence for accessing
 * the specified type at the specified address.
 * NOTE: GET_UNALIGNED and SET_UNALIGNED are not available yet.
 *
 * GET_UNALIGNED_WORD, SET_UNALIGNED_WORD
 * These are for word accesses which are unaligned more than 10% of the
 * time, and the alignment is not always 2-byte. 
 *
 * GET_UNALIGNED2_WORD, SET_UNALIGNED2_WORD
 * These are for word accesses which are unaligned more than 10% of the
 * time, and the alignment is ALWAYS 2-byte. 
 *
 * GET_UNALIGNED_SHORT, SET_UNALIGNED_SHORT
 * These are for signed short accesses which are unaligned more than 10% of
 * the time. 
 *
 * GET_UNALIGNED_UNSIGNED_SHORT, SET_UNALIGNED_UNSIGNED_SHORT
 * These are for unsigned short accesses which are unaligned more than 10%
 * of the time. 
 *
 * For naturally aligned data references (structure fields not under
 * #pragma pack or #pragma align, and pointer dereferences without a cast),
 * standard C syntax should be used.  The macros in this file provide a
 * method of abstracting non-natural data references so that the
 * application does not have to concern itself with how unaligned accesses
 * are performed. 
 *
 * By default, the macros are generated for unaligned accesses in
 * little-endian memory regions.  If the preprocessor symbol
 * __i960_BIG_ENDIAN__ is defined, the macros are generated for big-endian
 * memory accesses.  The compiler option that enables big-endian code
 * generation also causes the macro __i960_BIG_ENDIAN__ to be defined.
 *
 * Special note for big-endian memory users:
 *
 * If you are using an 80960CA D-step part (or later), the chip supports
 * unaligned accesses in big-endian memory regions.  Earlier (pre-D-step)
 * parts will fault on any unaligned accesses in big-endian memory regions.
 * Therefore, if you have a pre-D-step part and there is a possibility that
 * a memory access will be unaligned, you must use one of the "UNALIGNED"
 * or "UNALIGNED2" macros above or you will get a fault. 
 */

#include <__macros.h>

#if !defined(__SOFT_UNALIGNED_ACCESS)

/* These are the equivalents when not using a CA,CF,JA,JD,or JF processor. */
/* These are just for the convenience of the user             */
/* NOTE: These may generate unaligned accesses                */

/*#define GET_UNALIGNED(x,t)                (*(             t *)(x)) */
#define GET_UNALIGNED_WORD(x)             (*(unsigned  long *)(x))
#define GET_UNALIGNED2_WORD(x)            (*(unsigned  long *)(x))
#define GET_UNALIGNED_SHORT(x)            (*(         short *)(x))
#define GET_UNALIGNED_UNSIGNED_SHORT(x)   (*(unsigned short *)(x))
/*#define SET_UNALIGNED(x,t,v)              (*(             t *)(x) = (t)(v)) */
#define SET_UNALIGNED_WORD(x,v)           (*(unsigned  long *)(x) = (v))
#define SET_UNALIGNED_WORD2(x,v)          (*(unsigned  long *)(x) = (v))
#define SET_UNALIGNED_SHORT(x,v)          (*(         short *)(x) = (v))
#define SET_UNALIGNED_UNSIGNED_SHORT(x,v) (*(unsigned short *)(x) = (v))

#else /* __SOFT_UNALIGNED_ACCESS */

/* #define GET_UNALIGNED(x,t)   (t) __builtin_get_unaligned(x,sizeof(t)) */
/* #define SET_UNALIGNED(x,t,v) (t) __builtin_set_unaligned(x,sizeof(t),v) */

#define GET_UNALIGNED_WORD(x)					\
   __builtin_get_unaligned_size_4_align_1  ((unsigned char *)(x), 0)

#define GET_UNALIGNED2_WORD(x)					\
   __builtin_get_unaligned_size_4_align_2 ((unsigned char *)(x), 0)

#define GET_UNALIGNED_SHORT(x)					\
   __builtin_get_unaligned_size_2_align_1 ((unsigned char *)(x), 0)

#define GET_UNALIGNED_UNSIGNED_SHORT(x)				\
   __builtin_get_unaligned_unsigned_short((unsigned char *)(x), 0)

#define SET_UNALIGNED_WORD(x,v)					\
   __builtin_set_unaligned_size_4_align_1 ((unsigned char *)(x), 0, (long)(v))

#define SET_UNALIGNED2_WORD(x,v)				\
   __builtin_set_unaligned_size_4_align_2 ((unsigned char *)(x), 0, (long)(v))

#define SET_UNALIGNED_SHORT(x,v)				\
    __builtin_set_unaligned_size_2_align_1 ((unsigned char *)(x), 0, (unsigned long)(v))

#define SET_UNALIGNED_UNSIGNED_SHORT(x,v)			\
   ((unsigned long)SET_UNALIGNED_SHORT((x), (v)))


/*
 * NOTE: The functions below may change in future versions of the
 *       compiler or 80960 processor.
 */

/* GET functions */

static __inline__ long
__builtin_get_unaligned_size_4_align_1 (unsigned char *p, int ignored)
{

#pragma i960_align __tw = 8
   struct __tw {
     unsigned long lo;
     unsigned long hi;
   } two_words;

   unsigned long *base_addr;
   unsigned long shift;
            long ret;

   base_addr = (unsigned long *) (((unsigned long)p) & ~ 0x3);

#ifdef __i960_BIG_ENDIAN__
   shift = (4-(((unsigned long)p) & 0x3)) << 3;
   two_words.lo = base_addr[1];
   two_words.hi = base_addr[0];
#else
   shift = (  (((unsigned long)p) & 0x3)) << 3; 
   two_words.lo = base_addr[0];
   two_words.hi = base_addr[1];
#endif /* __i960_BIG_ENDIAN__ */

#if !defined(__i960RP) || !defined(__CORE0) || !defined(__i960RD)
   __asm ("eshro %2,%1,%0" : "=d" (ret) : "d" (two_words), "d" (shift));
#else
   shift &= 31;
   ret = two_words.lo >> shift;
   shift = 32 - shift; 
   ret |= two_words.hi << shift ;
#endif
   return ret;
}

static __inline__ long
__builtin_get_unaligned_size_4_align_2 (unsigned char *p, int ignored)
{
   unsigned short *q = (unsigned short *)p;

#ifdef __i960_BIG_ENDIAN__
   return ((*(q+0)) << 16 | *(q+1));
#else
   return ((*(q+1)) << 16 | *(q+0));
#endif /* __i960_BIG_ENDIAN__ */

}

static __inline__ long
__builtin_get_unaligned_size_2_align_1 (unsigned char *p, int ignored)
{
#ifdef __i960_BIG_ENDIAN__
   return ((*(signed char *)(p+0)) << 8 | *(unsigned char *)(p+1));
#else
   return ((*(signed char *)(p+1)) << 8 | *(unsigned char *)(p+0));
#endif /* __i960_BIG_ENDIAN__ */

}

static __inline__ unsigned long
__builtin_get_unaligned_unsigned_short (unsigned char *p, int ignored)
{
#ifdef __i960_BIG_ENDIAN__
   return ((*(p+0)) << 8 | *(p+1));
#else
   return ((*(p+1)) << 8 | *(p+0));
#endif /* __i960_BIG_ENDIAN__ */

}

/* SET functions */

static __inline__ long
__builtin_set_unaligned_size_4_align_1 (unsigned char *p, int ignored, long v)
{
   long t = v;

#ifdef __i960_BIG_ENDIAN__
   *(p+3) = t; t >>= 8;
   *(p+2) = t; t >>= 8;
   *(p+1) = t; t >>= 8;
   *(p+0) = t;
#else
   *(p+0) = t; t >>= 8;
   *(p+1) = t; t >>= 8;
   *(p+2) = t; t >>= 8;
   *(p+3) = t;
#endif /* __i960_BIG_ENDIAN__ */

   return(v);
}

static __inline__ long
__builtin_set_unaligned_size_4_align_2 (unsigned char *p, int ignored, long v)
{
   long t = v;
   unsigned short *q = (unsigned short *)p;

#ifdef __i960_BIG_ENDIAN__
   *(q+1) = t; t >>= 16;
   *(q+0) = t;
#else
   *(q+0) = t; t >>= 16;
   *(q+1) = t;
#endif /* __i960_BIG_ENDIAN__ */

   return(v);
}

static __inline__ long
__builtin_set_unaligned_size_2_align_1 (unsigned char *p, int ignored, unsigned long v)
{
   unsigned long t = v;

#ifdef __i960_BIG_ENDIAN__
   *(p+1) = t; t >>= 8;
   *(p+0) = t;
#else
   *(p+0) = t; t >>= 8;
   *(p+1) = t;
#endif /* __i960_BIG_ENDIAN__ */

   return(v);
}

/* natural accesses */
/* these are called by GET_UNALIGNED() and SET_UNALIGNED() if the access
   is provably aligned */

static __inline__ unsigned char
__builtin_get_unaligned_size_1_align_1 (unsigned char *p, int ignored)
{ return (*p); }

static __inline__ unsigned short
__builtin_get_unaligned_size_2_align_2 (unsigned short *p, int ignored)
{ return (*p); }

static __inline__ unsigned long
__builtin_get_unaligned_size_4_align_4 (unsigned long *p, int ignored)
{ return (*p); }

static __inline__ unsigned char
__builtin_set_unaligned_size_1_align_1 (unsigned char *p, int ignored, unsigned char v)
{ return (*p = v); }

static __inline__ unsigned short
__builtin_set_unaligned_size_2_align_2 (unsigned short *p, int ignored, unsigned short v)
{ return (*p = v); }

static __inline__ unsigned long
__builtin_set_unaligned_size_4_align_4 (unsigned long *p, int ignored, unsigned long v)
{ return (*p = v); }

#endif /* __SOFT_UNALIGNED_ACCESS */

#endif /* __UNALIGN_H__ */
