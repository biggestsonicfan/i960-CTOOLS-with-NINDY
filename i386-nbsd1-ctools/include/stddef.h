#ifndef __STDDEF_H__
#define __STDDEF_H__

/* NULL is the null pointer constant. */
#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef _wchar_t
#define _wchar_t
typedef char wchar_t;
#endif

#ifndef _size_t
#define _size_t
typedef unsigned size_t;  /* result of sizeof operator */
#endif

#define offsetof(s_type, memb) ((size_t)&(((s_type *)0)->memb))

/* ptrdiff_t is "the signed type of the result of subtracting two pointers." */
typedef int ptrdiff_t;

/* non-ANSI definitions */
#define _NUL '\0'  /* string terminator */

#endif
