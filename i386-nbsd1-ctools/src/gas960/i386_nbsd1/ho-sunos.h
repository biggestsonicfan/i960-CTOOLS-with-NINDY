/* $Id: ho-sunos.h,v 1.1 1991/03/22 00:33:03 chrisb Exp $ */

#ifndef __STDC__
#define NO_STDARG
#include <memory.h>
#endif /* not __STDC__ */

#include <ctype.h>
#include <string.h>

/* externs for system libraries. */

extern char *index();
extern char *malloc();
extern char *realloc();
extern char *rindex();
extern int _filbuf();
extern int _flsbuf();
extern int abort();
extern int bcopy();
extern int bzero();
extern int bzero();
extern int exit();
extern int fclose();
extern int fprintf();
extern int fread();
extern int free();
extern int perror();
extern int printf();
extern int setvbuf();
extern int strcmp();
extern int strlen();
extern int strncmp();
extern int time();
extern int ungetc();
extern int vfprintf();
extern int vprintf();

#ifndef tolower
extern int tolower();
#endif

/*
 * Local Variables:
 * fill-column: 80
 * comment-column: 0
 * End:
 */

/* end of ho-sun4.h */
