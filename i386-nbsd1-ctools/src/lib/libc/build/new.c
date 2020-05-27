/**************************************************************************
 *
 *     Copyright (c) 1993 Intel Corporation.  All rights reserved.
 *
 *
 * Intel hereby grants you permission to copy, modify, and distribute this
 * software and its documentation.  Intel grants this permission provided
 * that the above copyright notice appears in all copies and that both the
 * copyright notice and this permission notice appear in supporting
 * documentation.  In addition, Intel grants this permission provided that
 * you prominently mark as not part of the original any modifications made
 * to this software or documentation, and that the name of Intel
 * Corporation not be used in advertising or publicity pertaining to the
 * software or the documentation without specific, written prior
 * permission.
 *
 * Intel provides this AS IS, WITHOUT ANY WARRANTY, INCLUDING THE WARRANTY
 * OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, and makes no
 * guarantee or representations regarding the use of, or the results of the
 * use of, the software and documentation in terms of correctness,
 * accuracy, reliability, currentness, or otherwise, and you rely on the
 * software, documentation, and results solely at your own risk.
 *
 **************************************************************************/

#include <stddef.h>

/* operator new (size_t). This function is used by C++ programs to allocate 
   a block of memory to hold a single object. */

typedef void (*vfp)(void);

void __default_new_handler (void);

static vfp __new_handler;

#define __NEW_HANDLER() ((__new_handler == 0) ? \
                         (__new_handler = __default_new_handler) : \
                         (__new_handler))

void *
__builtin_new (size_t sz)
{
  void *p;

  /* malloc (0) is unpredictable; avoid it.  */
  if (sz == 0)
    sz = 1;
  p = (void *) malloc (sz);
  while (p == 0)
    {
      (*__NEW_HANDLER())();
      p = (void *) malloc (sz);
    }
  
  return p;
}

/* void * operator new [] (size_t).  This function is used by C++ programs 
   to allocate a block of memory for an array.  */

extern void * __builtin_new (size_t);

void *
__builtin_vec_new (size_t sz)
{
  return __builtin_new (sz);
}

/* operator delete (void *).  This function is used by C++ programs to return 
   to the free store a block of memory allocated as a single object. */

void
__builtin_delete (void *ptr)
{
  if (ptr)
    free (ptr);
}

/* operator delete [] (void *). This function is used by C++ programs to 
   return to the free store a block of memory allocated as an array. */

extern void __builtin_delete (void *);

void
__builtin_vec_delete (void *ptr)
{
  __builtin_delete (ptr);
}

/* set_new_handler and the default new handler, described in
   17.3.3.2 and 17.3.3.5.  These functions define the result of a failure
   to allocate the amount of memory requested from operator new or new []. */

#include <stdio.h>

vfp
set_new_handler (vfp handler)
{
  vfp prev_handler;

  prev_handler = __NEW_HANDLER();
  if (handler == 0) handler = __default_new_handler;
  __new_handler = handler;
  return prev_handler;
}

#define MESSAGE "Out of memory in `new'\n"

void
__default_new_handler ()
{
  /* don't use fprintf (stderr, ...) because it may need to call malloc.  */
  /* This should really print the name of the program, but that is hard to
     do.  We need a standard, clean way to get at the name.  */
  write (2, MESSAGE, sizeof (MESSAGE));
  /* don't call exit () because that may call global destructors which
     may cause a loop.  */
  _exit (-1);
}
