#ifndef __CTYPE_H__
#define __CTYPE_H__

#include <__macros.h>

extern	__CONST	unsigned char	_ctype[];

#define	_U	0x01	/* Uppercase */
#define	_L	0x02	/* Lowercase */
#define	_N	0x04	/* Digit */
#define	_S	0x08	/* Whitespace */
#define _P	0x10	/* Punctuation */
#define _C	0x20	/* Control */
#define _SP	0x40	/* Space */
#define _X	0x80	/* Hexadecimal */

__EXTERN int __CONST_FUNC (isalnum)(int);
__EXTERN int __CONST_FUNC (isalpha)(int);
__EXTERN int __CONST_FUNC (iscntrl)(int);
__EXTERN int __CONST_FUNC (isdigit)(int);
__EXTERN int __CONST_FUNC (isgraph)(int);
__EXTERN int __CONST_FUNC (islower)(int);
__EXTERN int __CONST_FUNC (isprint)(int);
__EXTERN int __CONST_FUNC (ispunct)(int);
__EXTERN int __CONST_FUNC (isspace)(int);
__EXTERN int __CONST_FUNC (isupper)(int);
__EXTERN int __CONST_FUNC (isxdigit)(int);
__EXTERN int __CONST_FUNC (tolower)(int);
__EXTERN int __CONST_FUNC (toupper)(int);

/* Put these last, so a pre-ansi preprocessor will
   not be confused and try to substitute in the
   above declarations. */

#define isalnum(c)	((_ctype+1)[c]&(_U|_L|_N))
#define	isalpha(c)	((_ctype+1)[c]&(_U|_L))
#define iscntrl(c)	((_ctype+1)[c]&_C)
#define	isdigit(c)	((_ctype+1)[c]&_N)
#define isgraph(c)	((_ctype+1)[c]&(_P|_U|_L|_N))
#define	islower(c)	((_ctype+1)[c]&_L)
#define isprint(c)	((_ctype+1)[c]&(_SP|_P|_U|_L|_N))
#define ispunct(c)	((_ctype+1)[c]&_P)
#define	isspace(c)	((_ctype+1)[c]&_S)
#define	isupper(c)	((_ctype+1)[c]&_U)
#define	isxdigit(c)	((_ctype+1)[c]&(_N|_X))

#if !defined(__NO_INLINE)
#define tolower(x) __inline_tolower(x)
__inline static int __inline_tolower(int _c)
{
  return(isupper(_c) ? ((_c)-'A'+'a') : _c);
}

#define toupper(x) __inline_toupper(x)
__inline static int __inline_toupper(int _c)
{
  return(islower(_c) ? ((_c)-'a'+'A') : _c);
}
#endif

#if !defined(__STRICT_ANSI) && !defined(__STRICT_ANSI__)
__EXTERN int __CONST_FUNC (isascii)(int);
__EXTERN int __CONST_FUNC (isodigit)(int);
__EXTERN int __CONST_FUNC (_tolower)(int);
__EXTERN int __CONST_FUNC (_toupper)(int);

#define isascii(c) ((unsigned)(c) <= 0x7f)

#if !defined(__NO_INLINE)
#define isodigit(x) __inline_isodigit(x)
__inline static int __inline_isodigit(int _c)
{
  return (_c >= '0' && _c <= '7');
}

#define _tolower(c) ((c)+'a'-'A')
#define _toupper(c) ((c)+'A'-'a')
#endif

#endif /* ! __STRICT_ANSI */
#endif /* __CTYPE_H__ */
