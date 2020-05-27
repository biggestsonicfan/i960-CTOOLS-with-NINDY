
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

/**
***  This file contains the utility routines used for collecting
***  profile information from instrumented code.
***
***  __profile_clear()
***
***     This routine zeros all the profile counters.
***
***  __profile_output_end()
***
***     Writes the collected information to the file "default.pf"
***     if profile collection is in use.
***     This routine is always called from exit by registering it
***     using the atexit function.
***
***  __profile_init()
***
***     This routine clears the profile counters, and registers
***     __profile_output_end using atexit.
***
***  The following symbols are set up specially by the linker.
***
***  int __profile_data_start[]
***
***     A pointer to the profile data.  This area is
***     where all the profile counters are allocated by the linker.
***    
***  int  __profile_data_length
***
***     This is a special symbol set up by the linker.  It's address
***     gives the length in bytes of the profile data area.
***
**/

#ifndef NO_FILE_SYSTEM
#include <sys/file.h>
#endif

extern unsigned long __profile_data_start[];
extern int __profile_data_length;

static int
prof_data_length()
{
  int ret = (int) &__profile_data_length;
#if defined(__PID)
  asm ("subo	g12,%1,%0" : "=d"(ret) : "dI"(ret));
#endif
  return ret;
}

void
__profile_clear(void)
{
  unsigned long *p;
  int n;
  int data_length = prof_data_length();

  /* zero out the data since we just dumped it. */
  for (p = __profile_data_start, n = data_length; n > 0; n -= 4, p += 1)
    *p = 0;
}

#ifndef NO_FILE_SYSTEM

#define profile_error(fd) \
 do { if ((fd) >= 0) close(fd);\
      write (2, "profile collection error.\n", 26); \
      return; } while (0)

#define CI_U32_TO_BUF(buf,u32) \
( (buf)[0] = (u32), (buf)[1] = (u32) >> 8, \
  (buf)[2] = (u32) >> 16, (buf)[3] = (u32) >> 24 )

#endif

void
__profile_output_end()
{
  int n;
  int fd;
  unsigned long *p;
  unsigned char buf[4];
  int data_length = prof_data_length();

  if (data_length == 0) return;

#ifndef NO_FILE_SYSTEM
  /*
   * note that O_BINARY doesn't work on some UNIX hosts under NINDY
   */
  fd = open ("default.pf", (O_TRUNC|O_WRONLY|O_BINARY|O_CREAT), 0666);
  if (fd < 0) profile_error(fd);

  CI_U32_TO_BUF(buf, data_length);
  if (write(fd, buf, 4) != 4)
    profile_error(fd);

  /* convert the data to proper endianness. */
  for (p = __profile_data_start, n = data_length; n > 0; n -= 4, p += 1)
  {
    unsigned char *t = (unsigned char *)p;
    unsigned int t1 = *p;
    CI_U32_TO_BUF(t, t1);
  }

  /* write the data to the profile file. */
  if (write(fd, __profile_data_start, data_length) != data_length)
    profile_error(fd);

  close(fd);

  /* zero out the data since we just dumped it. */
  __profile_clear();
#endif
}

void
__profile_init(void)
{
  __profile_clear();

#ifndef NO_FILE_SYSTEM
  if (atexit(__profile_output_end))
  {
    write (2, "atexit failed to register __profile_output_end\n", 47);
    write (2, "  profile information will not be output\n", 41);
  }
#endif
}

void (*__profile_init_ptr)(void) = __profile_init;
