/******************************************************************/
/*       Copyright (c) 1990,1991,1992,1993 Intel Corporation

   Intel hereby grants you permission to copy, modify, and 
   distribute this software and its documentation.  Intel grants
   this permission provided that the above copyright notice 
   appears in all copies and that both the copyright notice and
   this permission notice appear in supporting documentation.  In
   addition, Intel grants this permission provided that you
   prominently mark as not part of the original any modifications
   made to this software or documentation, and that the name of 
   Intel Corporation not be used in advertising or publicity 
   pertaining to distribution of the software or the documentation 
   without specific, written prior permission.  

   Intel Corporation provides this AS IS, without any warranty,
   including the warranty of merchantability or fitness for a
   particular purpose, and makes no guarantee or representations
   regarding the use of, or the results of the use of, the software 
   and documentation in terms of correctness, accuracy, reliability, 
   currentness, or otherwise; and you rely on the software, 
   documentation and results solely at your own risk.
 */
/******************************************************************/

#include <stdio.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>

#include "cc_info.h"

#if defined(__STDC__)
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#if defined(__STDC__)
void
db_fatal(char *fmt, ...)
#else
void
db_fatal(va_alist)
va_dcl
#endif
{
  char buf[1024];
  va_list arg;
  int i;

#if defined(__STDC__)
  va_start(arg, fmt);
#else
  char* fmt;
  va_start(arg);
  fmt = va_arg(arg, char *);
#endif
  vsprintf (buf, fmt, arg);
  va_end (arg);

  db_remove_files(0);

  fatal ("%s", buf);
}

#if defined(__STDC__)
void
db_error(char *fmt, ...)
#else
void
db_error(va_alist)
va_dcl
#endif
{
  char buf[1024];
  va_list arg;
#if defined(__STDC__)
  va_start(arg, fmt);
#else
  char* fmt;
  va_start(arg);
  fmt = va_arg(arg, char *);
#endif
  vsprintf (buf, fmt, arg);
  va_end (arg);

  error ("%s", buf);
}

#if defined(__STDC__)
void
db_warning(char *fmt, ...)
#else
void
db_warning(va_alist)
va_dcl
#endif
{
  char buf[1024];
  va_list arg;
#if defined(__STDC__)
  va_start(arg, fmt);
#else
  char* fmt;
  va_start(arg);
  fmt = va_arg(arg, char *);
#endif
  vsprintf (buf, fmt, arg);
  va_end (arg);

  warning ("%s", buf);
}


/* Same as `malloc' but report error if no memory available.  */

char *
db_malloc (size)
unsigned size;
{
  extern char* xmalloc();
  return xmalloc(size);
}

char *
db_realloc (p, size)
char* p;
unsigned size;
{
  extern char* xrealloc();
  return xrealloc (p, size);
}
