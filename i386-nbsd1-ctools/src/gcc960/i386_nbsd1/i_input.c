#ifdef IMSTG

/* This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <stdio.h>
#include "i_lister.h"
#include "config.h"
#include "assert.h"
#include "i_yyltype.h"

extern FILE* finput;
extern char* input_filename;
extern int lineno;

static unsigned prev_col, cur_col;

static int chars_read;
extern int in_file_size;

#define BGETC(f) (( (f)==finput ? (chars_read == in_file_size ?EOF :(++chars_read,getc(f))) : getc(f) ))
#define BUNGETC(c,f)(( (f)==finput ? (chars_read--, ungetc(c,f)) : ungetc(c,f)))

static int
bgetc(f)
FILE* f;
{ /* Only for debugging */
  int ret,iseof;
  assert (f == finput);
  assert (chars_read >= 0 && chars_read <= in_file_size);
  iseof = (chars_read == in_file_size);
  ret = BGETC(f);
  assert (iseof == (ret==EOF));
  return ret;
}

static int
bungetc(c,f)
FILE* f;
int c;
{ /* Only for debugging */
  int ret;
  assert (f == finput);
  assert (chars_read > 0 && chars_read <= in_file_size);
  ret = BUNGETC(c,f);
  return ret;
}

#ifdef DEBUG_GETC
#define GETC(f) bgetc(f)
#define UNGETC(c,f) bungetc(c,f)
#else
#define GETC(f) BGETC(f)
#define UNGETC(c,f) BUNGETC(c,f)
#endif

#define MIMSTG_GETC(f)\
(( ((cur_c=GETC(f))=='\n' ?((prev_col=cur_col),cur_col=0) :cur_col++), cur_c ))

#define MIMSTG_UNGETC(c,f)\
(( (((c)=='\n') ? ((cur_col=prev_col),(prev_col=(unsigned)-1)) : cur_col--), UNGETC(c,f) ))

imstg_getc(f)
FILE* f;
{ int cur_c;
  return MIMSTG_GETC(f);
}

imstg_ungetc(c,f)
FILE* f;
int c;
{ return MIMSTG_UNGETC(c,f);
}

#ifdef DEBUG_LISTING
#define IMSTG_GETC(f) imstg_getc(f)
#define IMSTG_UNGETC(c,f) imstg_ungetc(c,f)
#else
#define IMSTG_GETC(f) MIMSTG_GETC(f)
#define IMSTG_UNGETC(c,f) MIMSTG_UNGETC(c,f)
#endif

#define CUR_POS (( (MAP_LINE(sinfo_top)<<COL_BITS) | \
                    (((cur_col-1) >MAX_LIST_COL) ?MAX_LIST_COL :(cur_col-1)) ))

void advance_tok_info()
{
  yylloc.file = input_filename;
  yylloc.line = lineno;
  yylloc.lpos = CUR_POS;
}

error_at_token(buf, token_buffer)
char* buf;
char* token_buffer;
{
  /* Have to be careful here.  We want to place the caret at the start of the
     token.  file and line should not have advanced, but use the ones which
     were in effect at the start of the last token fed to the parser,
     just in case.  TMC Fall '93 */

  error_with_file_and_line (yylloc.file,yylloc.line,yylloc.lpos,buf,token_buffer,0);
}

#endif
