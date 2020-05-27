#ifdef ASM_SET_CCINFO_TIME

#include <stdio.h>
#include <assert.h>

#if defined(__STDC__)
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "as.h"
#include "cc_info.h"

static named_fd output_save;
static named_fd output;
long ccinfo_offset = -1;

remember_old_output (name)
char* name;
{
  int noisy = db_set_noisy (0);

  /* We are about to overwrite 'name'.  If 'name' exists and
     is readable, remember it in a temporary file in the hope that
     the object will be unchanged, so that we can just pluck it's
     cc_info timestamp out and put it into the new output file.
  */

  dbf_open_read (&output, name, 0);

  /* Save to temp file */
  if (output.fd != 0)
  /* Supposedly, the fopen - unlink - fopen sequence wont work on DOS. */
  { int c;
    extern FILE* create_temp_file ();

#ifdef DOS
    output_save.fd = create_temp_file (&(output_save.name), "wb+");
#else
    output_save.fd = create_temp_file (&(output_save.name), "w+");
#endif

    while ((c = getc (output.fd)) != EOF)
      putc (c, output_save.fd);

    dbf_close (&output);
    dbf_close (&output_save);
  }

  db_set_noisy (noisy);
}

/* Return true iff index should be skipped for purposes of comparing objects. */
/* This is used to keep the standard coff timestamp from causing the objects
   to appear to be different. */

#ifdef OBJ_COFF
#define TS_INDEX(I) (( (I) >= 4 && (I) < 8 ))
#else
#define TS_INDEX(I) (( 0 ))
#endif

set_output_time ()
{
  long offset = ccinfo_offset;
  int noisy = db_set_noisy (0);

  assert (output.name != 0 && output.fd == 0);

  if (offset != -1)
  { time_t stamp;
    char buf[4];

    offset += CI_HEAD_STAMP_OFF;

    /* Get the modtime for the file we just wrote, then open it for r/w */
    stamp  = dbf_get_mtime (&output);
#ifdef DOS
    output.fd = fopen (output.name, "rb+");
#else
    output.fd = fopen (output.name, "r+");
#endif

    if (output.fd == 0)
      db_fatal ("cannot reopen '%s' for read/write to set cc_info timestamp");

    /* Have we got a previous version ? */
    if (output_save.name)
    { int size;

      if (output_save.fd == 0)
        dbf_open_read (&output_save, output_save.name, db_fatal);

      dbf_seek_end (&output, 0);
      dbf_seek_end (&output_save, 0);

      /* Are the files the same length ? */
      if ((size=dbf_tell(&output)) == dbf_tell(&output_save))
      { long i = 0;

        /* Yes.  Compare all bytes except the timestamps. */

        dbf_seek_set (&output, 0);
        dbf_seek_set (&output_save, 0);

        for (; i < offset;  i++)
          if (getc (output.fd) != getc (output_save.fd) && (!TS_INDEX(i)))
            break;

        if (i == offset)
        { /* Skip past stamp in new file ... */
          dbf_read (&output, buf, 4);

          /* Remember the stamp from old file ... */
          dbf_read (&output_save, buf, 4);

          for (i += 4; i < size; i++)
            if (getc (output.fd) != getc (output_save.fd) && (!TS_INDEX(i)))
              break;

          if (i == size)
            /* OK, the output of this assembly hasn't changed.  We can reuse
               the stamp from the old file. */

            CI_U32_FM_BUF (buf, stamp);
        }
      }
    }

    dbf_seek_set (&output, offset);
    db_comment (0, "writing timestamp %u\n", stamp);
    CI_U32_TO_BUF (buf, stamp);
    dbf_write (&output, buf, 4);
  }

  if (output.fd)
    dbf_close (&output);

  if (output_save.fd)
    dbf_close (&output_save);

  /* If we made a temp file, unlink it */
  if (output_save.name && strcmp (output.name, output_save.name))
    db_unlink (output_save.name);

  db_set_noisy (noisy);
}

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

  as_fatal ("%s\n", buf);
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

  as_warn ("%s\n", buf);
}

/* Same as `malloc' but report error if no memory available.  */

char *
db_malloc (size)
unsigned size;
{
  return (char*) xmalloc (size);
}

char *
db_realloc (p, size)
char* p;
unsigned size;
{
  return (char*) xrealloc (p, size);
}

#endif
