#ifndef __ASSERT_H__
#define __ASSERT_H__

/*
 * <assert.h> : assertion facility
 *
 * (note 1 -- depends on ANSI string-ize operator (#))
 * (note 2 -- is an expression, not a statement)
 */

#include <__macros.h>

__EXTERN void __CONST_FUNC (_assert) (__CONST char*, __CONST char*, int);

#ifdef __STDC__
#ifdef NDEBUG
#define assert(ignore) ((void)0)
#else
#define assert(exp) ((exp) ? ((void)0) : _assert(#exp, __FILE__, __LINE__))
#endif /* ! NDEBUG */
#else
#ifdef NDEBUG
#define assert(ignore)
#else
#define assert(exp)  if (!(exp)) _assert("exp", __FILE__, __LINE__)
#endif /* ! NDEBUG */
#endif /* __STDC__ */
#endif /* __ASSERT_H__ */
