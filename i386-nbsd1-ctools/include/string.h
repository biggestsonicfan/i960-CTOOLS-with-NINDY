#ifndef __STRING_H__
#define __STRING_H__

/*
 * <string.h> : string handling routines for gcc960 and ic960
 */

#include <__macros.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef _size_t
#define _size_t
typedef unsigned size_t;  /* result of sizeof operator */
#endif

#if !defined(__NO_BUILTIN)
#define memcpy(a,b,c) __builtin_memcpy(a,b,c)
#define memcmp(a,b,c) __builtin_memcmp(a,b,c)
#define strcmp(a,b)   __builtin_strcmp(a,b)
#define strcpy(a,b)   __builtin_strcpy(a,b)
#define strlen(a)     __builtin_strlen(a)
#endif /* ! __NO_BUILTIN */

/* 4.11.2 Copying functions */
__EXTERN void*	(memcpy)(void*,__CONST void*,size_t);
__EXTERN void*	(memmove)(void*,__CONST void*,size_t);
__EXTERN char*	(strcpy)(char*,__CONST char*);
__EXTERN char*	(strncpy)(char*,__CONST char*,size_t);

/* 4.11.3 Concatenation functions */
__EXTERN char*	(strcat)(char*,__CONST char*);
__EXTERN char*	(strncat)(char*,__CONST char*,size_t);

/* 4.11.4 Comparison functions */
__EXTERN int	(memcmp)(__CONST void*,__CONST void*,size_t);
__EXTERN int	(strcmp)(__CONST char*,__CONST char*);
__EXTERN int	(strcoll)(__CONST char*,__CONST char*);
__EXTERN int	(strncmp)(__CONST char*,__CONST char*,size_t);
__EXTERN size_t	(strxfrm)(char*,__CONST char*,size_t);

/* 4.11.5 Search functions */
__EXTERN void*	(memchr)(__CONST void*,int,size_t);
__EXTERN char*	(strchr)(__CONST char*,int);
__EXTERN size_t	(strcspn)(__CONST char*,__CONST char*);
__EXTERN char*	(strpbrk)(__CONST char*,__CONST char*);
__EXTERN char*	(strrchr)(__CONST char*,int);
__EXTERN size_t	(strspn)(__CONST char*,__CONST char*);
__EXTERN char*	(strstr)(__CONST char*,__CONST char*);
__EXTERN char*	(strtok)(char*,__CONST char*);

/* 4.11.6 Miscellaneous functions */
__EXTERN void*	(memset)(void*,int,size_t);
__EXTERN char*	(strerror)(int);
__EXTERN size_t	(strlen)(__CONST char*);

/* Miscellaneous NON-ANSI functions */
__EXTERN void*	(memccpy)(void*,__CONST void*,int,int);
__EXTERN int	(strpos)(__CONST char*,char);
__EXTERN char*	(strrpbrk)(__CONST char*,__CONST char*);
__EXTERN int	(strrpos)(__CONST char*,char);

__EXTERN char*  (strset)(char *, int);
__EXTERN char*  (strdup)(__CONST char *);
__EXTERN char*  (strlwr)(char *);
__EXTERN char*  (strnset)(char *, int, size_t);
__EXTERN char*  (strrev)(char *);
__EXTERN char*  (strupr)(char *);
__EXTERN int    (memicmp)(__CONST void *, __CONST void *, unsigned);
__EXTERN int    (stricmp)(__CONST char *, __CONST char *);
__EXTERN int    (strnicmp)(__CONST char *, __CONST char *, size_t);

#endif
