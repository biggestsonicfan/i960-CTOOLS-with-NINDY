#ifndef __STDIO_H__
#define __STDIO_H__

/*
 * stdio.h - standard I/O header file
 * Copyright (c) 1988, 1989, 1993 Intel Corporation, ALL RIGHTS RESERVED.
 */

#include <__macros.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef _size_t
#define _size_t
typedef unsigned size_t;                /* result of sizeof operator */
#endif

typedef long int fpos_t;

#define BUFSIZ         1024
#define FOPEN_MAX      15
#define TMP_MAX        25
#define FILENAME_MAX   128
#define L_tmpnam       9
typedef struct _iobuf  FILE;
#define EOF            (-1)

#define _IOFBF         0x00  		/* neither line nor char buffering */
#define _IOREAD        0x01
#define _IOWRT         0x02
#define _IONBF         0x04
#define _IOMYBUF       0x08
#define _IOEOF         0x10
#define _IOERR         0x20
#define _IOLBF         0x40
#define _IORW          0x80
#define _IODIRTY       0x200

/* seek */
#define SEEK_SET       0
#define SEEK_CUR       1
#define SEEK_END       2

#pragma i960_align _iobuf = 4
struct _iobuf {
  unsigned char *_ptr;
  int _cnt;
  unsigned char *_base;
  int _flag;
  int _fd;                              /* file descriptor number */
  int _size;                            /* file buffer size */
  char *_temp_name;                     /* temporary file name */
  void *_sem;                           /* semaphore */
  FILE *_next_stream;                   /* pointer to the next stream */
};

#include <reent.h>

#define stdin	(&_stdio_ptr()->_stdin)
#define stdout	(&_stdio_ptr()->_stdout)
#define stderr	(&_stdio_ptr()->_stderr)

/* 4.9.4 Operations on files */
__EXTERN int	(remove)(__CONST char*);
__EXTERN int	(rename)(__CONST char*, __CONST char*);
__EXTERN FILE*	(tmpfile)(__NO_PARMS);
__EXTERN char*	(tmpnam)(char*);

/* 4.9.5 File access functions */
__EXTERN int	(fclose)(FILE*);
__EXTERN int	(fflush)(FILE*);
__EXTERN FILE*	(fopen)(__CONST char*, __CONST char*);
__EXTERN FILE*	(freopen)(__CONST char*, __CONST char*, FILE*);
__EXTERN void	(setbuf)(FILE*, char*);
__EXTERN int	(setvbuf)(FILE*, char*, int, size_t);

/* 4.9.6 Formatted input/output functions */
__EXTERN int	(fprintf)(FILE*, __CONST char*, ...);
__EXTERN int	(fscanf)(FILE*, __CONST char*, ...);
__EXTERN int	(printf)(__CONST char*, ...);
__EXTERN int	(scanf)(__CONST char*, ...);
__EXTERN int	(sprintf)(char*, __CONST char*,...);
__EXTERN int	(sscanf)(__CONST char*, __CONST char*, ...);
__EXTERN int	(vfprintf)(FILE*, __CONST char*, _va_list);
__EXTERN int	(vprintf)(__CONST char*, _va_list);
__EXTERN int	(vsprintf)(char*, __CONST char*, _va_list);

/* 4.9.7 Character input/output functions */
__EXTERN int	(fgetc)(FILE*);
__EXTERN char*	(fgets)(char*, int, FILE*);
__EXTERN int	(fputc)(int, FILE*);
__EXTERN int	(fputs)(__CONST char*, FILE*);
__EXTERN int	(getc)(FILE*);
__EXTERN int	(getchar)(__NO_PARMS);
__EXTERN char*	(gets)(char*);
__EXTERN int	(putc)(int, FILE*);
__EXTERN int	(putchar)(int);
__EXTERN int	(puts)(__CONST char*);
__EXTERN int	(ungetc)(int, FILE*);

/* 4.9.8 Direct input/output functions */
__EXTERN size_t	(fread)(void*, size_t, size_t, FILE*);
__EXTERN size_t	(fwrite)(__CONST void*, size_t, size_t, FILE*);

/* 4.9.9 File positioning functions */
__EXTERN int	(fgetpos)(FILE*, fpos_t*);
__EXTERN int	(fseek)(FILE*, long, int);
__EXTERN int	(fsetpos)(FILE*, __CONST fpos_t*);
__EXTERN long	(ftell)(FILE*);
__EXTERN void	(rewind)(FILE*);

/* 4.9.10 Error handling functions */
__EXTERN void	(clearerr)(FILE*);
__EXTERN int	(feof)(FILE*);
__EXTERN int	(ferror)(FILE*);
__EXTERN void	(perror)(__CONST char*);

#if !defined(__STRICT_ANSI) && !defined(__STRICT_ANSI__)

__EXTERN int	(vscanf)(__CONST char*, _va_list);
__EXTERN int	(vfscanf)(FILE*, __CONST char*, _va_list);
__EXTERN int	(vsscanf)(__CONST char*, __CONST char*, _va_list);
__EXTERN FILE * (fdopen)(int,__CONST char*);

__EXTERN int    (fileno)(FILE *);
__EXTERN int    (fcloseall)(__NO_PARMS);
__EXTERN int    (fgetchar)(__NO_PARMS);
__EXTERN int    (flushall)(__NO_PARMS);
__EXTERN int    (getw)(FILE *);
__EXTERN int    (putw)(int, FILE *);
__EXTERN int    (rmtmp)(__NO_PARMS);
__EXTERN int    (fputchar)(int);
#endif

__EXTERN int	(_filbuf)(FILE*);
__EXTERN int	(_flsbuf)(unsigned char, FILE*);
__EXTERN int    (_putch)(int, FILE *);
__EXTERN int    (_getch)(FILE *);

#ifdef _STRIP_CRZ
/*
 * This version of getc for DOS skips a CR or cntl-Z. It will not
 * skip two in a row.
 */
static int _getc_value;
#define __getc(s) ((_getc_value = (((s)->_cnt--) > 0 ? \
				  ((unsigned char)(*(s)->_ptr++)) : _filbuf(s))\
		 ) == 0x0d || _getc_value == 0x1a ? \
		      (((s)->_cnt--) > 0 ? \
			((unsigned char)(*(s)->_ptr++)) : _filbuf(s)) : \
		 _getc_value \
		)
#else
#define __getc(s) ((((s)->_cnt--) > 0) ? \
                ((unsigned char)(*(s)->_ptr++)) : _filbuf(s))
#endif

#define __getchar()	__getc(stdin)

#define __putc(c,s) (((--(s)->_cnt) >= 0) \
                    ? (*((s)->_ptr++)=(c)) : _flsbuf((c),(s)))

#define __putchar(c) __putc((c),stdout)

#if !defined(__STRICT_ANSI) && !defined(__STRICT_ANSI__)
/* non-ANSI functions */

#define _ungetc_
#define fileno(f) ((f)->_fd)
#define fgetchar() fgetc(stdin)

#endif /* strict ANSI */

#define feof(f)   (((f)->_flag & _IOEOF) != 0)
#define ferror(f) (((f)->_flag & _IOERR) != 0)

#if !defined(__NO_INLINE)

__inline static int __inline_getc(FILE* __p)
{ return __getc(__p); }

__inline static int __inline_getchar(__NO_PARMS)
{ return __inline_getc(stdin); }

__inline static int __inline_putc(int __x, FILE* __p)
{ return __putc(__x, __p); }

__inline static int __inline_putchar(int __x)
{ return __inline_putc(__x, stdout); }

#define getc(p) __inline_getc(p)
#define getchar() __inline_getchar()
#define putc(x,p) __inline_putc(x,p)
#define putchar(x) __inline_putchar(x)

#else

#define getc(p) __getc(p)
#define getchar() __getchar()
#define putc(x,p) __putc(x,p)
#define putchar(x) __putchar(x)

#endif
#endif /* __STDIO_H__ */
