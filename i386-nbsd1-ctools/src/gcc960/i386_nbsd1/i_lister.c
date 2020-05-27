#include "config.h"

#ifdef IMSTG
#include <stdio.h>
#include "tree.h"
#include "rtl.h"
#include "insn-config.h"
#include "assert.h"
#include "expr.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
extern char* rindex();
extern char* index();
#define IN_ILISTER 1
#include "i_lister.h"
#include "i_lutil.h"
#include "input.h"
#include "i_lutil.def"
#include "i_yyltype.h"
#include "gvarargs.h"

int disable_lpos_update;
int sinfo_top;
sinfo_map_rec* sinfo_map;

static int have_real_sinfo;

void
set_have_real_sinfo(v)
int v;
{
  have_real_sinfo = v;
}

int
get_have_real_sinfo()
{
  return have_real_sinfo;
}

#define SET_LINE(P,I) do {\
  (P) = GET_COL(P) | ((I) << COL_BITS);\
} while (0)

#if 0
#define SET_COL(P,I) do {\
  (P) = ((P) & ~MAX_LIST_COL) | MIN((I),MAX_LIST_COL); \
} while (0)
#endif

extern FILE* asm_out_file;

static FILE* save_asm_out_file;
static int high_list_line;
static int real_list_line;
static int sinfo_alloc;
static int listing_out_of_line;

void
set_listing_out_of_line()
{
  listing_out_of_line = 1;
}

int
set_sinfo_file(s)
char* s;
{
  sinfo_name = s;
  assert (s);
  return 1;
}

#define GET_START_LINE(N)   GET_LINE(NOTE_LISTING_START(N))
#define GET_END_LINE(N)     GET_LINE(NOTE_LISTING_END(N))

#define SET_START_LINE(N,L)   SET_LINE(NOTE_LISTING_START(N),L)
#define SET_END_LINE(N,L)     SET_LINE(NOTE_LISTING_END(N),L)

#define IS_LIST_NOTE(p) \
(( (p) && (GET_CODE(p)==NOTE) && (NOTE_LINE_NUMBER(p)>0) \
           && (GET_START_LINE(p)!=0) ))

static rtx get_list_note(p)
rtx p;
{
  p = NEXT_INSN(p);

  while (p && !IS_LIST_NOTE(p))
    p = NEXT_INSN(p);

  return p;
}

static char sinfo_lbuf[LISTING_BUF_DECL_SIZE];
static char sinfo_pbuf[LISTING_BUF_DECL_SIZE];
static int sinfo_line;
static int sinfo_seek_index;

#define is_extra(t) (( (t)=='e' || (t)=='m' ))
#define is_line(t)  (( (t)=='i' || (t)=='s' || (t)=='c' ))
#define is_xline(t) (( (t)=='x' ))
#define is_note(t)  (( (t)=='n' ))
#define is_error(t) (( (t)=='e' ))
#define is_macro(t) (( (t)=='m' ))

char*
get_sinfo_line (buf, bufsiz, f)
char* buf;
int bufsiz;
FILE* f;
{
  int n = 0;
  int c;

  while ((c = fgetc (f)) != EOF)
  {
    if (n < (bufsiz - 3))
      buf[n++] = c;

    if (c == '\n')
      break;
  }

  if (n && buf[n-1] != '\n')
    buf[n++] = '\n';

  buf[n] = '\0';

  if (n == 0)
    return 0;
  else
    return buf;
}

static char*
peek_sinfo()
{
  if (sinfo_pbuf[0]==0)
  {
    char* p = get_sinfo_line (sinfo_pbuf, sizeof (sinfo_pbuf), sinfo_file);
  
    if (p)
    { if (*p)
        p += (strlen (p)-1);
  
      if (*p != '\n')
      { *p++ = '\n';
        *p++ = '\0';
      }
    }
    else
      sinfo_pbuf[0] = '\0';
  }
  return sinfo_pbuf;
}

static char*
read_sinfo_extra()
{
  static char dummy;
  char* ret = &dummy;

  assert (sinfo_pbuf[0]==0);

  if (sinfo_lbuf[0]!=0)
  { char *p = peek_sinfo();
    if (is_extra(p[0]))
    { strcpy (sinfo_lbuf, p);
      p[0] = '\0';
      ret = sinfo_lbuf;
    }
  }
  return ret;
}

static char*
read_sinfo_line()
{
  if (sinfo_lbuf[0]!=0)
  { char* p = peek_sinfo();
    strcpy (sinfo_lbuf, p);
    if (!is_extra(p[0]))
      sinfo_seek_index++;
    p[0] = '\0';
  }

  return sinfo_lbuf;
}

static void
reset_sinfo()
{
  fseek (sinfo_file, 0L, 0);
  sinfo_seek_index = 0;
  sinfo_lbuf[0] = '\n';
  sinfo_lbuf[1] = 0;
  sinfo_pbuf[0] = 0;
}

static int
find_map_entry(l)
{
  int m = 0;

  if (sinfo_name && l >= MAP_LINE(1) && l <= MAP_LINE(sinfo_top))
  {
    /*  Find the right entry for l, starting from the one we
        found last time we looked. */

    static int lastm;

    if (lastm==0)
      m = sinfo_top-1;
    else
      m = lastm;
    
    while (MAP_LINE(m) < l && m < sinfo_top-1)
      m++;

    while (MAP_LINE(m) > l)
      m--;

    lastm = m;
  }
  return m;
}

debug_map_entries(l,h)
{
  int i,j;
  int m;

  if (h > MAP_LINE(sinfo_top))
    h = MAP_LINE(sinfo_top);

  if (l < MAP_LINE(1))
    l = MAP_LINE(1);

  m = find_map_entry(l);
  j = l;

  fprintf (stderr,"%d(%d):%d..",m,MAP_LPOS(m),j);
  
  for (i = l; i <= h; i++)
  { int n = find_map_entry (i);
    if (n != m)
    {
      fprintf (stderr,"%d\n",j);
      m = n;
      j = i;
      fprintf (stderr,"%d(%d):%d..",m,MAP_LPOS(m),j);
    }
  }
  fprintf (stderr,"\n");
}

init_sinfo_map()
{
  sinfo_alloc = 100;
  sinfo_top   = 1;
  sinfo_map   = (sinfo_map_rec*) xmalloc (sinfo_alloc * sizeof(sinfo_map_rec));
  bzero (sinfo_map, sinfo_alloc * sizeof(sinfo_map[0]));
}

void
update_sinfo_map(new_line_lpos)
int new_line_lpos;
{
  int cur_list_line, elapsed_input, elapsed_sinfo;

  cur_list_line = MAP_LINE(sinfo_top);

  if (disable_lpos_update)
    return;

  if (!have_real_sinfo)
    new_line_lpos = cur_list_line;

  elapsed_input = cur_list_line - MAP_LINE(sinfo_top-1);
  elapsed_sinfo = new_line_lpos - MAP_LPOS(sinfo_top-1);

  assert (elapsed_sinfo >= 1 && elapsed_input >= 1);

  /* Bump MAP_LINE(sinfo_top) by an amount sufficient to cover all the lines
     between the marks in the sinfo file. */

  if (elapsed_sinfo > elapsed_input)
    MAP_LINE(sinfo_top) = (cur_list_line += (elapsed_sinfo - elapsed_input));

  MAP_LPOS(sinfo_top) = new_line_lpos;

  /* Now raise the sinfo stack, and set the new current line
     counter (MAP_LINE(sinfo_top)) to be the old one.  It will
     be incremented as soon as the end of this line directive is seen. */

  if (++sinfo_top >= sinfo_alloc)
  { sinfo_alloc = sinfo_top * 2 + 1;
    sinfo_map = (sinfo_map_rec *)
      xrealloc (sinfo_map,sinfo_alloc*sizeof(sinfo_map[0]));
    bzero (sinfo_map+sinfo_top,
             (sinfo_alloc-sinfo_top) * sizeof(sinfo_map[0]));
  }
  MAP_LINE(sinfo_top) = cur_list_line;
}

static char*
get_list_line (l)
{
  /* Find the information for the source line denoted by 'l'. */

  static char dummy;
  int m;

  assert (l > 0);

  if (sinfo_pbuf[0])
  { read_sinfo_line();
    assert (!is_extra(sinfo_lbuf[0]));
    sinfo_line++;
  }

  if (m=find_map_entry(l))
  {
    assert (m>0 && l >= MAP_LINE(m) && (l==MAP_LINE(sinfo_top)|| l<MAP_LINE(m+1)));

    if (sinfo_line>l || sinfo_line==0 || sinfo_lbuf[0]==0)
    { 
      reset_sinfo();
      while (read_sinfo_extra()[0])
        ;
      read_sinfo_line();
      sinfo_line = 1;
      assert (is_note(sinfo_lbuf[0]));
    }

    assert (sinfo_line <= l);

    if (sinfo_seek_index <= MAP_LPOS(m))
    {
      while (sinfo_seek_index < MAP_LPOS(m))
        if (read_sinfo_line()[0] == 0)
          internal_error ("cannot find list line %d in %s", l, sinfo_name);

      assert (is_note(sinfo_lbuf[0]));
      sinfo_line = MAP_LINE(m);

      if (sinfo_line < l)
      { read_sinfo_line();
        if (!is_extra(sinfo_lbuf[0]))
          sinfo_line++;
      }
    }

    assert (sinfo_line <= l);

    if (l == sinfo_line)
      assert (sinfo_seek_index >= MAP_LPOS(m));
    else
    {
      while (sinfo_lbuf[0] && !is_note(sinfo_lbuf[0]) && sinfo_line < l)
      { read_sinfo_line();
        if (!is_extra(sinfo_lbuf[0]))
          sinfo_line++;
      }

      if (sinfo_lbuf[0]==0)
      { assert (sinfo_line >= MAP_LINE(sinfo_top));
        sinfo_line = 0;
      }
    }
    return sinfo_lbuf;
  }
  else
    return &dummy;
}

struct lmsg
{ int line;
  int col;
  int seq;
  int lookup;
  int phase;
  char buf[LISTING_MESSAGE_LEN+1];
} *lmess;

static int nlmess,almess,slmess;

static void enque_message();

static void
cpp_message(m, line)
char* m;
int line;
{
  assert (is_error(m[0]) && (m[1]=='0'||m[1]=='1'));
  enque_message (m+2, 0, line, MAX_LIST_COL, ((m[1]-'0') && line > 0));
}

static void
output_listing_info (f, l, h)
FILE* f;
int l,h;
{
  static int did_first_line;

  if (!sinfo_name || (!flag_mix_asm && !flag_list) || l > h)
    return;

  /* fprintf (stderr, "listing %d..%d:\n", l, h); */
  if (!did_first_line)
  { char *t;
    reset_sinfo();
    while ((t=read_sinfo_extra())[0])
      cpp_message (t,0);
    did_first_line = 1;
  }

  if (l <= MAP_LINE(1))
    l = MAP_LINE(1);

  if (h > MAP_LINE(sinfo_top))
    h = MAP_LINE(sinfo_top);

  for (; l <= h; l++)
  { char* t = get_list_line (l);

    if (t[0])
    { int listed = 0; 
      char lbuf[LISTING_BUF_DECL_SIZE];

      if (high_list_line < l)
        high_list_line = l;

      if (is_line(t[0]))
      { real_list_line = l;
        split_list_line (t, 0, lbuf, 0, 0);
        if (flag_mix_asm && t[0]=='s')
          fprintf (f, "# (%4d) %s", atoi(t+6), lbuf);
      }

      if (flag_list && is_line(t[0]) && (flag_list_include || atoi(t+2)==0))
      { listed = 1;
#ifndef TMC_DEBUG
        fprintf (f, "+%d%s", l, t);
#else
        fprintf (f, "+%d n%d%s", l,MAP_LPOS(find_map_entry(l)), t);
#endif
      }

      while ((t = read_sinfo_extra())[0])
        if (is_error(t[0]) && !listing_out_of_line)
          cpp_message (t,real_list_line);
        else
          if (listed)
            if (is_macro(t[0]))
              fprintf (f, "+%s%s", list_pluses,t+LISTING_HEADER_SIZE);
            else
              fprintf (f, "+%s", t);
    }
  }
}

void
output_list_note_info (f, i)
FILE* f;
rtx i;
{
  if (IS_LIST_NOTE(i))
    output_listing_info (f, GET_START_LINE(i), GET_END_LINE(i));
}

static void
update_listing (f,l)
FILE* f;
{
  int i;

  if (!sinfo_name || (!flag_mix_asm && !flag_list))
    return;

  /* Any messages we have which we don't know what to do with, we
     bind at this spot. */

  assert (l >= 0);

  i = nlmess;
  while (i-- > 0)
    if (((int)lmess[i].line) < 0)
      lmess[i].line = l;

  if (l > high_list_line)
    output_listing_info (f, high_list_line+1, l);
}


static int last_func_line;

void
start_func_listing_info (file, first)
FILE* file;
rtx first;
{
  rtx t,p,i;

  if (!sinfo_name || (!flag_mix_asm && !flag_list))
    return;

  while (first && !IS_LIST_NOTE(first))
    first = NEXT_INSN(first);

  if (first==0)
    return;

  last_func_line = GET_START_LINE(first);

  if (high_list_line < GET_START_LINE(first))

    /* Do not allow gaps to appear in the source listing.  If the last line
       already printed is less than the first line of the function, go back
       and print the lines in between.  Note that we deliberately allow
       restarting BEFORE the highest line already printed; this typically
       happens when we get a late binding for an inline function.  */

    SET_START_LINE(first, high_list_line + 1);
  else
    if (listing_out_of_line)
      fprintf (file,"#\n# function body for previously listed inline function %s\n",
                  IDENTIFIER_POINTER(DECL_NAME(current_function_decl)));

  SET_END_LINE(first,GET_START_LINE(first));

  p = first;
  t = get_list_note (p);

  while (t)
  {
    assert (GET_END_LINE(p) >= GET_START_LINE(p));
    if (last_func_line < GET_START_LINE(t))
      last_func_line = GET_START_LINE(t);

    /* See if there is any code between the notes p and t. */
    for (i = NEXT_INSN(p); i != t; i = NEXT_INSN(i))
    { enum rtx_code c = GET_CODE(i);
      if (c!=BARRIER && (c!=NOTE||NOTE_LINE_NUMBER(i)==NOTE_INSN_FUNCTION_BEG))
        break;
    }
    
    if (t == i || GET_START_LINE(t)==GET_END_LINE(p))
      /* No code between p and t.  No point in bumping the listing
         cursor yet, cause we do not allow gaps anyhow. */
    { SET_START_LINE(t,0);
      assert (GET_END_LINE(t)==0);
      t = get_list_note (t);
    }

    else if (GET_START_LINE(t) < GET_END_LINE(p))
    { /* At this point, if a note would send us backwards, within the same
         function, we will not do it.  We do terminate the current chunk,
         however. */
      SET_START_LINE(t,GET_END_LINE(p)+1);
      SET_END_LINE(t,GET_START_LINE(t));
      t = get_list_note (p=t);
    }
    else
    { SET_END_LINE(p,GET_START_LINE(t)-1);
      SET_END_LINE(t,GET_START_LINE(t));
      t = get_list_note (p=t);
    }
  }

  if (last_func_line > GET_END_LINE(p))
    SET_END_LINE(p,last_func_line);

  output_list_note_info (file,first);
  SET_START_LINE(first,0);
  SET_END_LINE(first,0);
}

end_func_listing_info ()
{
  update_listing (asm_out_file, last_func_line);
}

end_decl_listing_info ()
{
  update_listing (asm_out_file, MAP_LINE(sinfo_top));
}


int
make_sinfo_file()
{
  int ret = 0;

  if (sinfo_name && !sinfo_file)
    if ((sinfo_file = fopen (sinfo_name, "r")) == 0)
      ret = 1;

  return ret;
}

void start_listing_info()
{
  open_listing_files();

  if (ltmp_name)
  { assert (asm_out_file && ltmp_file);
    save_asm_out_file = asm_out_file;
    asm_out_file = ltmp_file;
  }
}

static int lmess_compare ();
void
finish_listing_info()
{
  extern char *progname, *main_input_filename;
  extern char gnu960_ver[], ic960_ver[];

  if (progname == 0)
    return;

  update_listing (asm_out_file, 0x7fffffff);

  if (ltmp_file)
  { 
    /* Both the true final assembly and the listing are on ltmp_file.
       Produce seperate assembly and listing files from ltmp_file, and reset
       asm_out_file to be the real assembly file. */

    char buf[LISTING_BUF_DECL_SIZE];
    char lbuf[LISTING_BUF_DECL_SIZE];
    char cbuf[LISTING_BUF_DECL_SIZE];

    char *prog, *vers;
    int mess,need_nl;

    assert (list_file && save_asm_out_file && asm_out_file == ltmp_file);

    fclose (ltmp_file);
    if ((ltmp_file = fopen (ltmp_name, "r")) == 0)
      pfatal_with_name (ltmp_name);

    asm_out_file = save_asm_out_file;
    save_asm_out_file = 0;

    mess = 0;

    if (nlmess)
      qsort (lmess, nlmess, sizeof (struct lmsg), lmess_compare);

    need_nl = 1;

    prog = "";
    vers = "";

#ifndef SELFHOST
    if (flag_ic960)
      vers = ic960_ver;
    else
      vers = gnu960_ver;
#endif

    fprintf (list_file, "%s%s \"%s\"\n\n", prog, vers, main_input_filename);

    fprintf (list_file, "Include  Line\n");
    fprintf (list_file, " Level  Number  Source Lines\n");
    fprintf (list_file, "======= ======  ============\n");

    /* Walk thru the combined assembly/listing file ... */
    while (get_sinfo_line (buf, sizeof(buf), ltmp_file))

      /* + means the text goes to the listing file ... */
      if (buf[0]=='+')
      { char c, *p;
        int n;

        p = buf;
        n = 0;
        while ((c=p[1]) >= '0' && c <= '9')
        { n = n * 10 + (c - '0');
          p++;
        }

        if (n)
        { /* Put out all messages which are to go before this line. */
          while (mess < nlmess && lmess[mess].line < n)
          { char *t;

            if (need_nl)
              fprintf (list_file, "\n");

            /* Look up the source line if we are supposed to do so;
               otherwise, we just put out the message at this spot. */

            if (lmess[mess].lookup && (t=get_list_line(lmess[mess].line))[0] &&
                !is_xline(t[0]) && !is_note(t[0]))
            { int col = lmess[mess].col;
              split_list_line (t, col, lbuf, cbuf, 0);
              fprintf (list_file,"%s%s",list_arrows,lbuf);
              if (cbuf[0])
                fprintf (list_file,"%s%s",list_arrows,cbuf);
            }

            fprintf (list_file,"%s%s\n\n",list_arrows,lmess[mess].buf);
            need_nl = 0;
            mess++;
          }
        }

        p++;
        p[0]=' ';

        if (p[1]=='m')
        { /* Don't print the macro expansion table ... */
          p[1] = ' ';
          p[LISTING_HEADER_SIZE] = '\0';
          fputs (p, list_file);
          p = index(p+LISTING_HEADER_SIZE+1,':')+1;
        }
        fputs (p, list_file);
        need_nl = 1;
      }
      else
      { extern int errorcount,sorrycount;
        fputs (buf, asm_out_file);
        if (flag_list_asm && !errorcount && !sorrycount && buf[0] != '#')
        { fprintf (list_file, "%s", list_blanks);
          fputs(buf,list_file);
          need_nl = 1;
	}
      }

      while (mess < nlmess)
      { char* t;

        if (need_nl)
          fprintf (list_file, "\n");

        /* Look up the source line if we are supposed to do so;
           otherwise, we just put out the message at this spot. */

        if (lmess[mess].lookup && (t=get_list_line(lmess[mess].line))[0] &&
            !is_xline(t[0]) && !is_note(t[0]))
        { int col = lmess[mess].col;
          split_list_line (t, col, lbuf, cbuf, 0);
          fprintf (list_file,"%s%s",list_arrows,lbuf);
          if (cbuf[0])
            fprintf (list_file,"%s%s",list_arrows,cbuf);
        }
        fprintf (list_file,"%s%s\n\n",list_arrows,lmess[mess].buf);
        need_nl = 0;
        mess++;
      }

    nlmess = 0;
    fclose (ltmp_file);
    ltmp_file = 0;

    fclose (list_file);
    list_file = 0;
  }
  else
    assert (list_file == 0);

  if (sinfo_file)
  { fclose (sinfo_file);
    sinfo_file = 0;
  }
}

int
imstg_map_sinfo_col_to_real_col(list_pos)
unsigned long list_pos;
{
  int	list_lineno = GET_LINE(list_pos);
  int	list_col = GET_COL(list_pos);
  char	*list_line, *map;
  int	base, exp, real_col;

  if (list_lineno <= 0 || list_col < 0)
    return 0;

  /* If there is no sinfo_file, or we can't find the appropriate source line,
     or there are no macro expansions on the source line, then return the
     column number as is.  Add 1 because sinfo columns are 0-based,
     while real columns are 1-based.
   */
  if (!sinfo_file
      || (list_col >= MAX_LIST_COL)
      || ((list_line = get_list_line(list_lineno)) == NULL)
      || !is_line(list_line[0])
      || !is_macro(list_line[1]))
    return list_col + 1;

  /* Now we must parse the mapping information at the beginning of list_line,
     to translate list_col into a real column number.
   */

  map = list_line + LISTING_HEADER_SIZE;

  exp = *map++;
  while ((base = atoi(map)) > list_col)
  {
    map = index(map, ',') + 1;
    exp = *map++;
  }

  real_col = atoi(index(map, ' ') + 1);
  if (exp != 'm')
    real_col += (list_col - base);

  return real_col + 1;
}

static int
lmess_compare (p, q)
struct lmsg *p;
struct lmsg *q;
{
  int ret;

  if ((ret = p->line - q->line) == 0)
    if ((ret = p->phase - q->phase) == 0)
      if ((ret = q->lookup - p->lookup) == 0)
        if ((ret = p->col - q->col) == 0)
          if ((ret = p->seq - q->seq) == 0)
            ret = strcmp (p->buf, q->buf);

  return ret;
}

static void
enque_message (p, phase, line, col, lookup)
char* p;
int line;
int col;
{
  struct lmsg *t;
  int l;
  assert (nlmess >= 0 && almess >= 0);

  if (++nlmess > almess)
    if (almess == 0)
    { assert (lmess == 0);
      almess = 1;
      lmess = (struct lmsg *) xmalloc (almess * sizeof(lmess[0]));
    }
    else
    { assert (lmess != 0);
      almess = nlmess * 2 + 1;
      lmess = (struct lmsg *) xrealloc (lmess, almess * sizeof(lmess[0]));
    }

  assert (lmess != 0 && almess > 0 && nlmess <= almess);
  t = &lmess[nlmess-1];
  t->phase  = phase;
  t->line   = line;
  t->col    = col;
  t->lookup = lookup;

  t->seq  = ++slmess;
  strncpy (t->buf, p, LISTING_MESSAGE_LEN);
  l = strlen (t->buf);

  while (--l >= 0 && t->buf[l] == '\n')
    t->buf[l] = '\0';
}

void
message_with_file_and_line (severity, file, line, list_pos, s, v, v2, v3, v4)
er_severity severity;
char *file;
int line;
int list_pos;
char *s;
HOST_WIDE_INT v, v2, v3, v4;
{
  char buf[LISTING_BUF_DECL_SIZE];
  char lbuf[LISTING_BUF_DECL_SIZE];
  char cbuf[LISTING_BUF_DECL_SIZE];
  char mbuf[LISTING_BUF_DECL_SIZE];
  char *m, *p;
  extern char* progname;

  char* prog;
  int lookup,list_line,list_col,do_list;
  static int in_message;

  list_line = GET_LINE(list_pos);
  list_col  = GET_COL(list_pos);

  assert (severity>=0 && severity <= ER_INTERNAL);

  m=p=buf;
  lbuf[0] = 0;
  cbuf[0] = 0;
  mbuf[0] = 0;

  prog = progname;

  /* Decide if we want to show the source line.  If in_message is not
     0, don't try to show the text or write it to the listing file,
     because we are in the process of putting out a message and probably
     got a seg fault or assertion failure. */

  lookup = (in_message==0 && file != 0 && list_line > 0);
  do_list = (in_message==0 && flag_list);

  in_message++;

  /* If we have line and file, but we cannot find the source line,
     display the message at the current line. */

  if (file != 0 && list_line == 0)
    list_line = GET_LINE(LOOK_LPOS);

  if (flag_fancy_errors)
  { char *t;

    if (file)
    { prog = flag_ic960 ? "ic960" : "gcc960";

      if (lookup && (t=get_list_line(list_line))[0] && !is_xline(t[0]) &&!is_note(t[0]))
      { 
        split_list_line (t,list_col,lbuf,cbuf,0);

        if (cbuf[0] == 0)
          sprintf(p,"\n> %s\n",lbuf);
        else
          sprintf(p,"\n  %s  %s",lbuf, cbuf);

        p += strlen(p);
      }
    }

    m = p;

    if (prog)
    { strcpy (p, prog);
      p += strlen (p);
      *p++ = ' ';
    }

    if (file && !lbuf[0])
      m = p;

    strcpy (p, fancy_smess[severity]);
    strcpy (mbuf, m);
    p += strlen (p);

    if (file)
    { sprintf (p, "\"%s\", line %d -- ", file, line);
      p += strlen(p);
    }
    sprintf (p, s, v, v2, v3, v4);
    if (lbuf[0] && (flag_list_include || atoi(t+2)==0))
    { strcat (mbuf, p);
      m = mbuf;
    }
  }
  else
  { 
    if (file)
    { sprintf (p, "%s:%d: ", file, line);
      p += strlen (p);
    }
    else if (prog)
    { sprintf (p, "%s: ", prog);
      p += strlen (p);
    }

    strcpy (p, plain_smess[severity]);
    p += strlen (p);
    sprintf (p, s, v, v2, v3, v4);
  }

  fprintf (stderr, "%s\n", buf);

  if (do_list)
    enque_message (m, 1, list_line, list_col, lookup);

  in_message--;
}

void
v_message_with_file_and_line (severity, file, line, list_pos, s, ap)
er_severity severity;
char *file;
int line;
int list_pos;
char *s;
va_list ap;
{
  int v1, v2, v3, v4;

  /* HACK JOB - Fix this asap.  This will probably work for most
     hosts for now.  There could be any number of args actually present */

  v1 = va_arg (ap, int);
  v2 = va_arg (ap, int);
  v3 = va_arg (ap, int);
  v4 = va_arg (ap, int);

  message_with_file_and_line (severity, file, line, list_pos, s, v1,v2,v3,v4);
}

void
v_message_with_decl (severity, decl, s, ap)
er_severity severity;
tree decl;
char* s;
va_list ap;
{
  int v1, v2, v3, v4;
  char* junk;
  char ebuf[1024];

  extern char *(*decl_printable_name) ();

  /* HACK JOB - Fix this asap.  This will probably work for most
     hosts for now.  There could be any number of args actually present */

  v1 = va_arg (ap, int);
  v2 = va_arg (ap, int);
  v3 = va_arg (ap, int);
  v4 = va_arg (ap, int);

  if (DECL_NAME (decl))
    sprintf (ebuf, s, (*decl_printable_name) (decl, &junk), v1,v2,v3,v4);
  else
    sprintf (ebuf, s, "((anonymous))", v1,v2,v3,v4);

  message_with_file_and_line (severity, DECL_SOURCE_FILE(decl), DECL_SOURCE_LINE(decl),DECL_SOURCE_POS(decl),"%s", ebuf,0,0,0);
}

#endif /* IMSTG */
