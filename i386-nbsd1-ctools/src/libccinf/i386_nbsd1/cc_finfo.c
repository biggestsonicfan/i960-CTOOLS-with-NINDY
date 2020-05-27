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
#include "assert.h"
#include "cc_info.h"
#include <sys/stat.h>
#include <sys/types.h>

#if defined(DOS)

#define READ_BINARY  "rb"
#define WRITE_BINARY "wb"
#define READ_TEXT    "rt"
#define WRITE_TEXT   "wt"

#else

#define READ_BINARY  "r"
#define WRITE_BINARY "w"
#define READ_TEXT    "r"
#define WRITE_TEXT   "w"

#endif

void
dbf_open_read (in, name, error)
DB_FILE* in;
char* name;
void (*error)();
{
  named_fd* f = (named_fd*) in;

  f->name = name;
  f->is_ascii = 0;

  if ((f->fd = fopen (f->name, READ_BINARY)) == 0)
    if (error)
      error ("cannot open %s for read", f->name);
}

void
dbf_open_read_ascii (in, name, error)
DB_FILE* in;
char* name;
void (*error)();
{
  named_fd* f = (named_fd*) in;

  f->name = name;
  f->is_ascii = 1;

  if ((f->fd = fopen (f->name, "r")) == 0)
    if (error)
      error ("cannot open %s for read", f->name);
}

void
dbf_open_write (in, name, error)
DB_FILE* in;
char* name;
void (*error)();
{
  named_fd* f = (named_fd*) in;

  f->name = name;
  f->is_ascii = 0;

  if ((f->fd = fopen (f->name, WRITE_BINARY)) == 0)
    if (error)
      error ("cannot open %s for write", f->name);
}

void
dbf_open_write_ascii (in, name, error)
DB_FILE* in;
char* name;
void (*error)();
{
  named_fd* f = (named_fd*) in;

  f->name = name;
  f->is_ascii = 1;

  if ((f->fd = fopen (f->name, "w")) == 0)
    if (error)
      error ("cannot open %s for write", f->name);
}

void
dbf_close (in)
DB_FILE* in;
{
  named_fd* f = (named_fd*) in;

  if (fclose (f->fd) != 0)
    db_fatal ("cannot close %s", f->name);

  f->fd = 0;
}

void
dbf_read (in, p, siz)
DB_FILE* in;
unsigned char *p;
int siz;
{
  named_fd* f = (named_fd*) in;
  assert (f->fd && f->name && f->name[0]);

  if (fread(p,1,siz, f->fd) != siz)
    db_fatal ("read failed on '%s'", f->name);
}

time_t
dbf_get_mtime (in)
DB_FILE* in;
{
  time_t ret;
  struct stat buf;

  named_fd* f = (named_fd*) in;
  assert (f->name && f->name[0]);

  ret = stat (f->name, &buf);
  
  if (ret)	/* If stat fails, return time 0 */
    ret = 0;
  else
  { ret = buf.st_mtime;
    assert (ret > 0);
  }

  return ret;
}

void
dbf_seek_set (in, tell)
DB_FILE* in;
int tell;
{
  named_fd* f = (named_fd*) in;
  assert (f->fd && f->name && f->name[0]);

  if (fseek(f->fd,tell,0) != 0)
    db_fatal ("seek failed on '%s'", f->name);
}

void
dbf_seek_cur (in, tell)
DB_FILE* in;
int tell;
{
  named_fd* f = (named_fd*) in;
  assert (f->fd && f->name && f->name[0]);

  if (fseek(f->fd,tell,1) != 0)
    db_fatal ("seek failed on '%s'", f->name);
}

void
dbf_seek_end (in, tell)
DB_FILE* in;
int tell;
{
  named_fd* f = (named_fd*) in;
  assert (f->fd && f->name && f->name[0]);

  if (fseek(f->fd,tell,2) != 0)
    db_fatal ("seek failed on '%s'", f->name);
}

int
dbf_tell (in)
DB_FILE* in;
{
  named_fd* f = (named_fd*) in;
  assert (f->fd && f->name && f->name[0]);

  return ftell (f->fd);
}

void
dbf_write (in, p, len)
DB_FILE* in;
char *p;
int len;
{
  named_fd* f = (named_fd*) in;
  assert (f->fd && f->name && f->name[0]);

  if (fwrite (p,1,len, f->fd) != len)
    db_fatal ("write failed on '%s'", f->name);
}

char* dbf_name (in)
DB_FILE* in;
{
  named_fd* f = (named_fd*) in;
  assert (f->fd && f->name && f->name[0]);
  return f->name;
}
