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
#include <ctype.h>
#include <signal.h>
#include "cc_info.h"
#include <string.h>
#include <stdlib.h>
#include "assert.h"
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "i_toolib.h"

#if (defined(__STDC__) | defined(I386_NBSD1))
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <stdlib.h>

#if defined(DOS)
#include <io.h>
#include <process.h>
#if !defined(WIN95)
#include <stat.h>
#endif
#else
#include <sys/file.h>
extern char* mktemp();
#endif
#include <fcntl.h>

#ifndef R_OK
#define R_OK 4
#endif
#ifndef W_OK
#define W_OK 2
#endif
#ifndef X_OK
#define X_OK 1
#endif

#if defined(DOS)
#define GNUTMP_DFLT     "./"
#else
#define GNUTMP_DFLT     "/tmp"
#endif

int db_rw_buf_siz;
unsigned char* db_rw_buf;

int
db_get32 (p)
unsigned char* p;
{
  int n;
  CI_U32_FM_BUF (p, n);
  return n;
}
char *
db_genv2(s)
char *s;
{
  int i;
  char buf[255], *p;

  p = 0;
  strcpy (buf+1, s);

  for (i=0; i<2 && p==0; i++)
  { buf[0] = "GI"[i];
    if (p = getenv(buf))
      if (*p == '\0')	/* So behaviour is same as shell compare with "" */
        p = 0;
  }

  return p;
}

void
db_sig_handler(signum)
int	signum;
{
  signal(signum, SIG_DFL);	/* This should happen automatically */
  
  db_fatal ("caught signal %d\n", signum);
}

void
db_set_signal_handlers()
{
  if (signal(SIGINT, SIG_IGN) != SIG_IGN)
    signal(SIGINT, db_sig_handler);

  if (signal(SIGABRT, SIG_IGN) != SIG_IGN)
    signal(SIGABRT, db_sig_handler);

#if defined(SIGHUP)
  if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
    signal(SIGHUP, db_sig_handler);
#endif

  if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
    signal(SIGTERM, db_sig_handler);

#if defined(SIGSEGV)
  if (signal(SIGSEGV, SIG_IGN) != SIG_IGN)
    signal(SIGSEGV, db_sig_handler);
#endif

#if defined(SIGIOT)
  if (signal(SIGIOT, SIG_IGN) != SIG_IGN)
    signal(SIGIOT, db_sig_handler);
#endif

#if defined(SIGILL)
  if (signal(SIGILL, SIG_IGN) != SIG_IGN)
    signal(SIGILL, db_sig_handler);
#endif

#if defined(SIGBUS)
  if (signal(SIGBUS, SIG_IGN) != SIG_IGN)
    signal(SIGBUS, db_sig_handler);
#endif
}

#if 0
#ifdef __i960__
char* mktemp(p)
char* p;
{
  return 0;
}
#endif
char*
db_choose_temp_base ()
{
  char *foo = "ccXXXXXX";
  char *tmp, *q, ch, *name;

  if (q = db_genv2("960TMP"))
  { int n = strlen(q); 
    tmp = db_malloc(n+1);
    memcpy (tmp, q, strlen(q));
    tmp[n] = 0;
  }
  else
  { tmp = (char *) db_malloc (strlen(GNUTMP_DFLT) + 1);
    strcpy (tmp, GNUTMP_DFLT);
  }

  name = (char *) db_malloc (strlen(tmp) + 1 + strlen(foo) + 1);
  strcpy (name, tmp);

  /* Append a trailing slash if needed */
  ch = name[strlen(name) - 1];

#if defined(DOS)
  if (ch != '/' && ch != '\\' &&
      !(ch == ':' && strlen(name) == 2))
    (void)strcat(name, "\\");
#else
  if (ch != '/')
    (void)strcat(name, "/");
#endif

  strcat (name, foo);

#if defined(DOS)
  if (name[0] == '/')
    name[0] = '\\';
#endif

  if ((q = mktemp (name)) == 0 || *q == '\0')
    db_fatal("unable to obtain temporary file base name");

  free(tmp);
  return name;
}
#endif


time_t
db_access_fok (name)
char* name;
{ /* Return 1 if name exists, 0 otherwise. */
  struct stat buf;
  return !stat (name, &buf);
}

int
db_access_rok (name)
char* name;
{
  int ret = (access (name, R_OK) == 0);
  return ret;
}

time_t
db_get_mtime (path)
char* path;
{
  named_fd f;
  time_t ret;

  memset (&f, 0, sizeof (f));
  f.name = path;

  ret = dbf_get_mtime (&f);
  return ret;
}

void
db_unlink (name, error)
char* name;
void (*error)();
{
#ifdef DEBUG
  static char* comment;

  db_comment (&comment, "unlink %s\n", name);
#endif
#ifdef DOS
    /*
     * for DOS we must change the permission, so that db_unlink can
     * remove this file.  In particular this is a problem at least for
     * pdb.lck.
     */
    chmod(name, (_S_IREAD | _S_IWRITE));
#endif

  if (unlink (name) != 0)
    if (errno != ENOENT)
      if (error)
        error ("could not unlink %s", name);
}

void
dbp_for_all_sym(this,f)
dbase* this;
void (*f)();
{
  st_node *list;
  int i;
  for (i = 0; i < CI_HASH_SZ; i++)
  {
    for (list = this->db_stab[i]; list != 0; list = list->next)
      f(list, i);
  }
}

st_node *
dbp_lookup_sym(this,name)
dbase* this;
char *name;
{
  int hash;
  st_node *list;

  CI_SYM_HASH(hash, name);

  list = this->db_stab[hash];

  while (list != 0)
  {
    if (strcmp(list->name, name) == 0)
    {
      /*
       * We found the entry in the symbol table, now make sure
       * that its data is here.
       */
      return list;
    }
    list = list->next;
  }

  return 0;
}

st_node *
dbp_add_sym(this,name, extra_size)
dbase *this;
char *name;
int extra_size;
{
  int hash;
  st_node *list;

  CI_SYM_HASH(hash, name);

  list = this->db_stab[hash];
  while (list != 0)
  {
    if (strcmp(list->name, name) == 0)
      return 0;
    list = list->next;
  }

  /*
   * we didn't find one, so add it to the symbol table.
   */
  list = (st_node *)db_malloc(sizeof(st_node)+strlen(name)+1+extra_size);
  memset (list, 0, sizeof (*list));

  list->next = this->db_stab[hash];
  list->name = ((char *)list)+sizeof(st_node)+extra_size;
  strcpy(list->name, name);
  list->db_rec = ((unsigned char *)list)+sizeof(st_node);

  this->db_stab[hash] = list;
  return list;
}

st_node*
dbp_bname_sym (this, name)
dbase* this;
char* name;
{
  char buf[1024];

  buf[0] = ci_lead_char[CI_SRC_REC_TYP];
  strcpy (buf+1, name);
  return dbp_lookup_sym (this, buf);
}

char*
db_member(p)
st_node*p;
{
  char* ret;
  assert (p->rec_typ == CI_SRC_REC_TYP);
  ret = (char*) CI_LIST_TEXT (p, CI_SRC_MEM_LIST);
  return ret;
}

int
db_fstat(p)
st_node*p;
{
  int ret;
  assert (p->rec_typ == CI_SRC_REC_TYP);
  CI_U32_FM_BUF(p->db_rec+CI_SRC_REC_FSTAT_OFF, ret);
  return ret;
}

int
db_set_fstat(p, mt)
st_node*p;
unsigned mt;
{
  assert (p->rec_typ == CI_SRC_REC_TYP);
  CI_U32_TO_BUF(p->db_rec+CI_SRC_REC_FSTAT_OFF,mt);
  return mt;
}

void
db_unpack_sizes (t, into)
unsigned char* t;
unsigned long* into;
{ /* Unpack target sizes or observed sizes. */
  int n = NUM_TS;

  while (n--)
  { unsigned long u;
 
    CI_U16_FM_BUF (t, u);
    *into++ = (u << 2);
    t += 2;
  }
}

void
db_pack_sizes (t, from)
unsigned char* t;
unsigned long* from;
{ /* Pack target sizes or observed sizes. */
  int n = NUM_TS;

  while (n--)
  { unsigned long u;

    u = *from++ >> 2;
    CI_U16_TO_BUF (t, u);
    t += 2;
  }
}

long
db_size_ratio (a, b, c)
long a;
long b;
long c;
{
  long ret;

  if (b == 0)
  { assert (a == 0);
    ret = 0;
  }
  else
    ret = dbl_to_long (clean_dbl (((double) a) / ((double) b)) * (double) c);

  return ret;
}

unsigned char*
db_tsinfo (p)
st_node* p;
{ unsigned char* ret;

  assert (CI_ISFDEF (p->rec_typ));
  ret = CI_LIST_TEXT (p, CI_FDEF_TSINFO_LIST);
  return ret;
}

int
db_dindex(p)
st_node*p;
{
  int ret;
  assert (p->rec_typ == CI_SRC_REC_TYP);
  CI_U16_FM_BUF(p->db_rec+CI_SRC_REC_DINDEX_OFF, ret);
  return ret;
}

int
db_set_dindex(p,n)
st_node* p;
{
  n &= 0xffff;
  assert (p->rec_typ == CI_SRC_REC_TYP);
  CI_U16_TO_BUF(p->db_rec+CI_SRC_REC_DINDEX_OFF,n);
  return n;
}

int
db_sindex(p)
st_node*p;
{
  int ret;
  assert (p->rec_typ == CI_SRC_REC_TYP);
  CI_U16_FM_BUF(p->db_rec+CI_SRC_REC_SINDEX_OFF,ret);
  return (short) ret;
}

int
db_set_sindex(p,n)
st_node* p;
{
  assert (p->rec_typ == CI_SRC_REC_TYP);
  CI_U16_TO_BUF(p->db_rec+CI_SRC_REC_SINDEX_OFF,n);
  return n;
}

int
db_rec_addr_taken(p)
st_node *p;
{
  int t;

  CI_U8_FM_BUF(p->db_rec + CI_ADDR_TAKEN_OFF, t);
  return t;
}

void
db_rec_set_addr_taken(p)
st_node *p;
{
  CI_U8_TO_BUF(p->db_rec + CI_ADDR_TAKEN_OFF, 1);
}

unsigned
db_gld_value(p)
st_node *p;
{
  unsigned long t;

  assert (p->rec_typ == CI_GLD_REC_TYP);
  CI_U32_FM_BUF(p->db_rec + CI_GLD_VALUE_OFF, t);
  return t;
}

void
db_set_gld_value(p, t)
st_node *p;
unsigned long t;
{
  assert (p->rec_typ == CI_GLD_REC_TYP);
  CI_U32_TO_BUF(p->db_rec + CI_GLD_VALUE_OFF, t);
}

unsigned long
dbp_lookup_gld_value (this, name)
dbase* this;
char* name;
{
  char buf[1024];
  st_node* db_p;
  unsigned long ret = 0;

  buf[0] = ci_lead_char[CI_GLD_REC_TYP];
  strcpy (buf+1, name);

  if (db_p = dbp_lookup_sym (this, buf))
    ret = db_gld_value (db_p);
  else
    db_fatal ("cannot find required symbol '%s' in '%s'", name, dbf_name(this->in));
  return ret;
}

int
db_fdef_can_delete(p)
st_node *p;
{
  int t;

  CI_U8_FM_BUF(p->db_rec + CI_FDEF_CAN_DELETE_OFF, t);
  return t & 1;
}

void
db_fdef_set_can_delete(p)
st_node *p;
{
  unsigned t;

  CI_U8_FM_BUF(p->db_rec + CI_FDEF_CAN_DELETE_OFF, t);
  t |= 1;
  CI_U8_TO_BUF(p->db_rec + CI_FDEF_CAN_DELETE_OFF, t);
}

int
db_rec_n_insns(p)
st_node *p;
{
  int ret_val;

  if (!CI_ISFDEF(p->rec_typ))
    return (0);

  CI_U16_FM_BUF(p->db_rec + CI_FDEF_NINSN_OFF, ret_val);
  return ret_val;
}

int
db_rec_n_parms(p)
st_node *p;
{
  int ret_val;

  if (!CI_ISFDEF(p->rec_typ))
    return (0);

  CI_U16_FM_BUF(p->db_rec + CI_FDEF_NPARM_OFF, ret_val);
  return ret_val;
}

int
db_rec_n_calls(p)
st_node *p;
{
  int ret_val;

  if (!CI_ISFDEF(p->rec_typ))
    return (0);

  CI_U16_FM_BUF(p->db_rec + CI_FDEF_NCALL_OFF, ret_val);
  return (ret_val);
}

int
db_rec_n_regs_used(p)
st_node *p;
{
  int ret_val;

  if (!CI_ISFDEF(p->rec_typ))
    return (0);

  CI_U8_FM_BUF(p->db_rec + CI_FDEF_REG_USE_OFF, ret_val);
  return (ret_val);
}

unsigned char *
db_inln_dec_vec(p)
st_node* p;
{
  unsigned char* ret = 0;

  if (CI_ISFDEF(p->rec_typ))
    ret = CI_LIST_TEXT(p, CI_FDEF_IVEC_LIST);

  return ret;
}


unsigned char *
db_rec_reg_pressure_info(p)
st_node *p;
{
  unsigned char* ret = 0;

  if (CI_ISFDEF(p->rec_typ))
    ret = CI_LIST_TEXT (p, CI_FDEF_REGP_LIST);

  return ret;
}

st_node*
dbp_prof_block_fdef (db, p, b, lo, hi)
dbase* db;
st_node* p;
int b;
unsigned *lo;
unsigned *hi;
{
  st_node* fdef;
  int stabi;

  *lo = *hi = -1;
  fdef = 0;

  for (stabi=0; stabi < CI_HASH_SZ; stabi++)
    for (fdef = db->db_stab[stabi]; fdef; fdef = fdef->next)
      if (fdef->db_rec_size && CI_ISFDEF(fdef->rec_typ))
      { unsigned char* file = CI_LIST_TEXT (fdef, CI_FDEF_FILE_LIST);

        if (!strcmp ((char *)file, p->name))
        {
          db_func_block_range (fdef, lo, hi);
          if (b >= *lo && b <= *hi)
          { assert (*lo != -1 && *hi != -1 && b >= *lo && b <= *hi);
            return fdef;
          }
        }
      }

  assert (0);
}

unsigned char *
db_rec_rtl(p)
st_node* p;
{
  unsigned char* ret = 0;

  if (CI_ISFDEF(p->rec_typ))
    ret = CI_LIST_TEXT (p, CI_FDEF_RTL_LIST);

  return ret;
}

unsigned char*
db_fdef_cfg(p)
st_node* p;
{
  assert (CI_ISFDEF(p->rec_typ));
  return db_get_list (p, CI_FDEF_CFG_LIST);
}

int
db_parm_len(parm_info)
unsigned char *parm_info;
{
  int length;

  if (parm_info == 0)
    return 0;

  CI_U16_FM_BUF(parm_info, length);
  return ((length & 0x7FFF) * 2) + 4;  /* constant 4 bytes for a function plus
                                        * 2 bytes per parameter
                                        */ 
}

int
db_parm_is_varargs(parm_info)
unsigned char *parm_info;
{
  int length;

  if (parm_info == 0)
    return 1;

  CI_U16_FM_BUF(parm_info, length);

  return (length & 0x8000) != 0;
}

unsigned char *
db_fdef_call_ptype(p)
st_node *p;
{
  if (!CI_ISFDEF(p->rec_typ))
    return 0;

  return CI_LIST_TEXT(p, CI_FDEF_CALL_TYPE_LIST);
}

unsigned char *
db_fdef_ftype(p)
st_node * p;
{
  if (!CI_ISFDEF(p->rec_typ))
    return 0;

  return CI_LIST_TEXT(p, CI_FDEF_FTYPE_LIST);
}

unsigned char *
db_fdef_next_ptype_info(beg, prev)
unsigned char *beg;
unsigned char *prev;
{
  int tlen;
  int plen;

  if (prev == 0)
    return beg;

  beg -= 4;
  CI_U32_FM_BUF(beg, tlen);

  plen = db_parm_len(prev);

  if (((prev - beg) + plen) >= tlen)
    return 0;

  return prev + plen;
}

int
db_rec_rtl_size(p)
st_node* p;
{
  int i = 0;

  if (CI_ISFDEF(p->rec_typ))
  { unsigned char* t = db_rec_rtl (p);
    CI_U32_FM_BUF(t-4,i);		/* Length of saved rtl */
    i -= 4;
    assert (i >= 0);
  }
  return i;
}

int
db_rec_inlinable(p)
st_node *p;
{
  return (db_rec_rtl_size(p) > 0);
}


char *
db_rec_prof_info(p)
st_node *p;
{
  if (!CI_ISFDEF(p->rec_typ))
    return (0);

  return (char*) db_rec_reg_pressure_info (p) + db_rec_n_calls (p);
}

char *
db_rec_call_vec(p)
st_node *p;
{
  int n_calls;
  char * prof_start;
  
  if (!CI_ISFDEF(p->rec_typ))
    return (0);

  n_calls = db_rec_n_calls(p);

  /*
   * The call vector is after all the other stuff that is dependent on
   * the number of calls.  This is the inline decision vector, register
   * pressure vector, and profile info.
   */

  prof_start = db_rec_prof_info(p);
  return (prof_start + strlen(prof_start) + 1 + 8 + n_calls * 4);
}

void
db_func_block_range (p, lo, hi)
st_node* p;
unsigned *lo;
unsigned *hi;
{
  char* t;

  assert (CI_ISFDEF(p->rec_typ));

  t = db_rec_prof_info(p);
  t += strlen(t) + 1;

  CI_U32_FM_BUF(t+0, *lo);

  assert (*lo != -1);

  if (*lo == (unsigned)-1)
    *hi = *lo;
  else
    CI_U32_FM_BUF(t+4, *hi);

  assert (*lo <= *hi);
}

int
db_fdef_nblocks(p)
st_node* p;
{
  unsigned lo, hi;
  db_func_block_range (p, &lo, &hi);
  return (hi-lo)+1;
}

void
db_rec_make_tot_inline_calls (p, tot_inln_calls)
st_node *p;
int tot_inln_calls;
{
  if (!CI_ISFDEF(p->rec_typ))
    return;

  CI_U16_TO_BUF(p->db_rec + CI_FDEF_TOT_INLN_OFF, tot_inln_calls);
}

void
db_rec_make_arc_inlinable(p, arc_num, recursive)
st_node *p;
int arc_num;
int recursive;
{
  int val;
  unsigned char* t;

  if (!CI_ISFDEF(p->rec_typ))
    return;

  /* set the proper bit in the byte */
  if (recursive)
    val = CI_FDEF_IVECT_RECURSIVE;
  else
    val = CI_FDEF_IVECT_INLINE;

  t = db_inln_dec_vec(p);
  CI_FDEF_IVECT_SET(t, arc_num, val);
}

int
db_rec_var_size(p)
st_node *p;
{
  int t;
  if (CI_ISVDEF(p->rec_typ))
    CI_U32_FM_BUF(p->db_rec + CI_VDEF_SIZE_OFF, t);
  else
    t = 0;
  return t;
}

unsigned
db_rec_var_sram(p)
st_node *p;
{
  int t;
  if (CI_ISVDEF(p->rec_typ))
    CI_U32_FM_BUF(p->db_rec + CI_VDEF_SRAM_ADDR_OFF, t);
  else
    t = 0;
  return t;
}

int
db_rec_var_usage(p)
st_node *p;
{
  int t;
  CI_U32_FM_BUF(p->db_rec + CI_VDEF_USAGE_OFF, t);
  return t;
}

void
set_db_rec_var_usage(p, val)
st_node *p;
unsigned long val;
{
  CI_U32_TO_BUF(p->db_rec + CI_VDEF_USAGE_OFF, val);
}

void
db_rec_var_make_fmem(p, fmem_addr)
st_node *p;
unsigned long fmem_addr;
{
  CI_U32_TO_BUF(p->db_rec + CI_VDEF_SRAM_ADDR_OFF, fmem_addr);
}

int
CI_REC_LIST_LO(typ)
int typ;
{
  int ret = 0;

  switch (typ)
  {
    case CI_SRC_REC_TYP:
      ret = CI_SRC_LIST_LO;
      break;

    case CI_FDEF_REC_TYP:
    case CI_LIB_FDEF_REC_TYP:
      ret = CI_FDEF_LIST_LO;
      break;

    case CI_VDEF_REC_TYP:
    case CI_LIB_VDEF_REC_TYP:
      ret = CI_VDEF_LIST_LO;
      break;

    case CI_FREF_REC_TYP:
      ret = CI_FREF_LIST_LO;
      break;

    case CI_VREF_REC_TYP:
      ret = CI_VREF_LIST_LO;
      break;

    case CI_PROF_REC_TYP:
      ret = CI_PROF_LIST_LO;
      break;
  }
  return ret;
}

int
CI_REC_LIST_HI(typ)
int typ;
{
  int ret = 0;

  switch (typ)
  {
    case CI_SRC_REC_TYP:
      ret = CI_SRC_LIST_HI;
      break;

    case CI_FDEF_REC_TYP:
    case CI_LIB_FDEF_REC_TYP:
      ret = CI_FDEF_LIST_HI;
      break;

    case CI_VDEF_REC_TYP:
    case CI_LIB_VDEF_REC_TYP:
      ret = CI_VDEF_LIST_HI;
      break;

    case CI_FREF_REC_TYP:
      ret = CI_FREF_LIST_HI;
      break;

    case CI_VREF_REC_TYP:
      ret = CI_VREF_LIST_HI;
      break;

    case CI_PROF_REC_TYP:
      ret = CI_PROF_LIST_HI;
      break;
  }
  return ret;
}

int
CI_REC_FIXED_SIZE(typ)
int typ;
{
  int ret = 0;

  switch (typ)
  {
    case CI_FDEF_REC_TYP:
    case CI_LIB_FDEF_REC_TYP:
      ret = CI_FDEF_REC_FIXED_SIZE;
      break;

    case CI_VDEF_REC_TYP:
    case CI_LIB_VDEF_REC_TYP:
      ret = CI_VDEF_REC_FIXED_SIZE;
      break;

    case CI_FREF_REC_TYP:
      ret = CI_FREF_REC_FIXED_SIZE;
      break;

    case CI_VREF_REC_TYP:
      ret = CI_VREF_REC_FIXED_SIZE;
      break;

    case CI_PROF_REC_TYP:
      ret = CI_PROF_REC_FIXED_SIZE;
      break;

    case CI_CG_REC_TYP:
      ret = CI_CG_REC_FIXED_SIZE;
      break;

    case CI_SRC_REC_TYP:
      ret = CI_SRC_REC_FIXED_SIZE;
      break;

    case CI_GLD_REC_TYP:
      ret = CI_GLD_REC_FIXED_SIZE;
      break;

    default:
      assert (0);
      break;
  }
  return ret;
}


#if 0
int dbli_find_name (info, s, lnum, value, voff)
db_list_info* info;
st_node* s;
int lnum;
unsigned char* value;
{
  db_list_node **cur;
  int index, len, n;
  unsigned char *p;
  int found = 0;

  cur = &(s->db_list[lnum]);
  index = len = n = 0;
  p = 0;

  while ((*cur) && !found)
  { p = (*cur)->text;
  
    assert ((*cur)->tell == 0 && p != 0);

    CI_U32_FM_BUF(p, len);
    len -= 4;
    p   += 4;

    assert (len >= 0);

    while (len > 0 && !found)
    { n = strlen(p)+1;

      if (!strcmp (p+voff, value+voff))
        found=1;
      else
      { len -= n;
        p   += n;
        index++;
      }
    }
    if (!found)
    { assert (len==0);
      cur = &((*cur)->next);
    }
  }

  info->s     = s;
  info->lnum  = lnum;
  info->cur   = cur;
  info->index = index;
  info->len   = len;
  info->n     = n;
  info->p     = p;

  assert (!found == !(*cur));
  return (found);
}

int dbli_find_index (info, s, lnum, value, n)
db_list_info* info;
st_node* s;
int lnum;
int value;
int n;
{
  db_list_node **cur;
  int index, len;
  unsigned char *p;
  int found = 0;

  cur = &(s->db_list[lnum]);
  index = len = 0;
  p = 0;

  while ((*cur) && !found)
  { p = (*cur)->text;
  
    assert ((*cur)->tell == 0 && p != 0);

    CI_U32_FM_BUF(p, len);
    len -= 4;
    p   += 4;

    assert (len >= 0);

    while (len > 0 && !found)
    {
      if (index == value)
        found = 1;
      else
      {
        if (n>=0)
        { len -= n;
          p   += n;
        }
        else
        { int m = strlen (p)+1;
          len -= m;
          p   += m;
        }
        index++;
      }
    }

    if (!found)
    { assert (len==0);
      cur = &((*cur)->next);
    }
  }

  info->s     = s;
  info->lnum  = lnum;
  info->cur   = cur;
  info->index = index;
  info->len   = len;
  info->n     = n;
  info->p     = p;

  assert (!found == !(*cur));
  return (found);
}

void
dbli_delete (info)
db_list_info *info;
{
  int i;

  db_list_node **cur = info->cur;
  int             n = info->n;
  int           len = info->len;
  st_node        *s = info->s;
  unsigned char* p  = info->p;

  if (n == 0)
    n = strlen(p)+1;

  assert (*cur);

  /* Subtract n from all of the sizes that care... */
  (*cur)->size -= n;
  CI_U32_FM_BUF((*cur)->text, i);
  CI_U32_TO_BUF((*cur)->text, i-n);
  s->db_rec_size -= n;
  s->db_list_size -= n;

  len -= n;

  /* Delete the text by moving the remainder of this chunk over it. */
  for (i = 0; i < len; i++)
    p[i] = p[i+n];
}
#endif

db_list_node*
db_new_list (value, len)
unsigned char* value;
int len;
{
  db_list_node *p;

  p = (db_list_node *) db_malloc (sizeof (db_list_node));
  memset (p, 0, sizeof (db_list_node));

  p->size = len;

  p->text = (unsigned char*) db_malloc (len+4);
  CI_U32_TO_BUF (p->text, len+4);

  if (value)
    memcpy (p->text+4, value, len);

  p->last = p;
  return p;
}

void
db_replace_list (into, num, l)
st_node* into;
int num;
db_list_node* l;
{
  assert (into->db_list[num]);

  into->db_list_size -= into->db_list[num]->size;
  into->db_rec_size  -= into->db_list[num]->size;

  into->db_list[num] = l;

  into->db_list_size += l->size;
  into->db_rec_size  += l->size;
}

#if 0
void
db_append_list (into, num, l)
st_node* into;
int num;
db_list_node* l;
{
  assert (into->db_list[num]);

  if (l->size == 0)
    return;

  if (into->db_list[num]->size == 0)
  { db_replace_list (into, num, l);
    return;
  }

  into->db_list_size += l->size;
  into->db_rec_size  += l->size;

  into->db_list[num]->size      += l->size;
  into->db_list[num]->last->next = l;
  into->db_list[num]->last       = l->last;
}

void
db_prepend_list (into, num, l)
st_node* into;
int num;
db_list_node* l;
{
  assert (into->db_list[num]);

  if (l->size == 0)
    return;

  if (into->db_list[num]->size == 0)
  { db_replace_list (into, num, l);
    return;
  }

  into->db_list_size += l->size;
  into->db_rec_size  += l->size;

  l->size += into->db_list[num]->size;
  l->last->next = into->db_list[num];
  l->last = into->db_list[num]->last;

  into->db_list[num] = l;
}
#endif

void
db_weave_lists (into, from)
st_node* into;
st_node* from;
{
  int i;

  /* If both into and from have lists, merge them together.  If into has
     a list but from doesn't, no harm.  If from has a list but into
     doesn't, that's a problem.  Symbols must be able to represent all
     data being merged into them. */

  for (i = 0; i < CI_NUM_LISTS; i++)
    if (into->db_list[i])
    {
      if (from->db_list[i])
      {
        into->db_list_size += from->db_list[i]->size;
        into->db_rec_size  += from->db_list[i]->size;

        into->db_list[i]->size      += from->db_list[i]->size;
        into->db_list[i]->last->next = from->db_list[i];
        into->db_list[i]->last       = from->db_list[i]->last;
      }
    }
    else
      assert (from->db_list[i] == 0);
}

void
db_swap_recs (p, q)
st_node *p;
st_node *q;
{
  st_node t;
  int i;

  t = *p;

  p->db_rec_offset = q->db_rec_offset;
  p->db_rec_size   = q->db_rec_size;
  p->db_rec    = q->db_rec;
  p->rec_typ       = q->rec_typ;
  p->db_list_size  = q->db_list_size;

  for (i = 0; i < CI_NUM_LISTS; i++)
    p->db_list[i] = q->db_list[i];

  q->db_rec_offset = t.db_rec_offset;
  q->db_rec_size   = t.db_rec_size;
  q->db_rec    = t.db_rec;
  q->rec_typ       = t.rec_typ;
  q->db_list_size  = t.db_list_size;

  for (i = 0; i < CI_NUM_LISTS; i++)
    q->db_list[i] = t.db_list[i];
}

int
db_qsort_st_node (p, q)
st_node *p;
st_node *q;
{
  int ret = p->db_rec_offset - q->db_rec_offset;

  assert (p==q || ret!=0);

  return ret;
}

int
db_get_magic (buf, head)
unsigned char* buf;
ci_head_rec* head;
{
  int ok;

  memset (head, 0, sizeof (*head));

  /* Get the magic number.  */
  CI_U16_FM_BUF(buf + CI_HEAD_MAJOR_OFF, head->major_ver);
  CI_U8_FM_BUF(buf + CI_HEAD_MINOR_OFF, head->minor_ver);
  CI_U8_FM_BUF(buf + CI_HEAD_KIND_OFF, head->kind_ver);

  if (!(head->major_ver & CI_MAJOR_MASK) || !(head->major_ver & ~CI_MAJOR_MASK))
  { /* No magic number;  we have old or missing ccinfo. */
    head->major_ver  = 0;
    head->minor_ver  = 0;
    head->kind_ver   = 0;
    ok = 0;
  }
  else
  { head->major_ver &= ~CI_MAJOR_MASK;
    ok = 1;
  }
  return ok;
}

char* db_file_str (k)
int k;
{
  char* r = " unknown database ";

  switch (k)
  { case 0:              r = " database ";                     break;
    case CI_CC1_DB:      r = " gas960 output database ";       break;
    case CI_PARTIAL_DB:  r = " linker relocatable database ";  break;
    case CI_PASS1_DB:    r = " linker output database ";       break;
    case CI_PASS2_DB:    r = " gcdm database ";                break;
    case CI_SPF_DB:      r = " self-contained profile ";       break;
  }
  return r;
}

void
db_file_cleanup (name, kind)
char* name;
int kind;
{
  /* If the file 'name' exists and it isn't the right sort of database
     file, either remove it and warn the user or give a fatal
     error explaining that the user must remove it.
  */

  int rm = 0;

  if (db_access_fok (name))
  { char buf[CI_HEAD_REC_SIZE];
    ci_head_rec h;
    named_fd f;

    dbf_open_read (&f, name, db_fatal);
    dbf_read (&f, buf, CI_HEAD_REC_SIZE);
    dbf_close (&f);

    if (rm = !db_get_magic (buf, &h))
      db_warning ("%s is removing corrupted%s'%s'",
                  db_prog_base_name(), db_file_str (kind), name);

    else if (rm=(h.major_ver<CI_MAJOR))
      db_warning ("%s is removing '%s' because it was built by tools older than the current ones",
                  db_prog_base_name(), name);

    else if (h.major_ver > CI_MAJOR)
      db_fatal ("'%s' was built by tools newer than the current ones; you must either remove the file or use the newer toolset",
                name);

    else if (rm = (kind != h.kind_ver))
      db_warning ("%s is removing%s'%s' because it is not a%s",
                  db_prog_base_name(), db_file_str (h.kind_ver), name, db_file_str(kind));
  }

  if (rm)
    db_unlink (name, db_fatal);
}

void
db_exam_head (buf, name, head)
unsigned char* buf;
char* name;
ci_head_rec* head;
{
  int size;

  void (*error)() = 0;

  if (!db_get_magic (buf, head))
    error = db_fatal;

  else
  { CI_U32_FM_BUF(buf+CI_HEAD_TOT_SIZE_OFF, size);
    CI_U32_FM_BUF(buf+CI_HEAD_DBSIZE_OFF,   head->db_size);
    CI_U32_FM_BUF(buf+CI_HEAD_SYMSIZE_OFF,  head->sym_size);
    CI_U32_FM_BUF(buf+CI_HEAD_STRSIZE_OFF,  head->str_size);
    CI_U32_FM_BUF(buf+CI_HEAD_STAMP_OFF,    head->time_stamp);

    size -= CI_HEAD_REC_SIZE;

    if (head->db_size <0 || head->sym_size <0 || head->str_size <0
    ||  size <0 || size != (head->db_size +head->sym_size +head->str_size))
      db_fatal ("corrupted cc_info in %s", name);

    if (head->major_ver != CI_MAJOR)
      error = db_fatal;

    else if (head->minor_ver != CI_MINOR)
      error = db_warning;
  }

  if (error)
    error ("%s has version %d.%d cc_info, but %s was built to expect %d.%d",
           name, head->major_ver, head->minor_ver, db_prog_base_name(),
           CI_MAJOR, CI_MINOR);
}

void
db_merge_recs (into_p, from_p)
st_node *into_p;
st_node *from_p;
{
  int t1;
  int t2;

  switch (into_p->rec_typ * CI_MAX_REC_TYP + from_p->rec_typ)
  {
    case CI_LIB_VDEF_REC_TYP * CI_MAX_REC_TYP + CI_VDEF_REC_TYP:
      /* Swap into and fm then fall through, this is OK because we are
	 supporting the common model and can bind the var def in one
	 of the files we will be compiling. */
      db_swap_recs (into_p, from_p);
      /* FALL THRU */
      
    case CI_LIB_VDEF_REC_TYP * CI_MAX_REC_TYP + CI_LIB_VDEF_REC_TYP:
    case CI_VDEF_REC_TYP * CI_MAX_REC_TYP + CI_LIB_VDEF_REC_TYP:
    case CI_VDEF_REC_TYP * CI_MAX_REC_TYP + CI_VDEF_REC_TYP:
      /* two var recs - merge them */
      /* addr_taken is the logical or of both fields */
      CI_U8_FM_BUF(into_p->db_rec + CI_ADDR_TAKEN_OFF, t1);
      CI_U8_FM_BUF(from_p->db_rec + CI_ADDR_TAKEN_OFF, t2);
      t1 |= t2;
      CI_U8_TO_BUF(into_p->db_rec + CI_ADDR_TAKEN_OFF, t1);

      /* var size is the max of either of the fields */
      CI_U32_FM_BUF(into_p->db_rec + CI_VDEF_SIZE_OFF, t1);
      CI_U32_FM_BUF(from_p->db_rec + CI_VDEF_SIZE_OFF, t2);
      if (t1 < t2)
	t1 = t2;
      CI_U32_TO_BUF(into_p->db_rec + CI_VDEF_SIZE_OFF, t1);

      /* usage count is the sum of both of the fields */
      CI_U32_FM_BUF(into_p->db_rec + CI_VDEF_USAGE_OFF, t1);
      CI_U32_FM_BUF(from_p->db_rec + CI_VDEF_USAGE_OFF, t2);
      t1 += t2;
      CI_U32_TO_BUF(into_p->db_rec + CI_VDEF_USAGE_OFF, t1);
      break;

    case CI_VREF_REC_TYP * CI_MAX_REC_TYP + CI_LIB_VDEF_REC_TYP:
    case CI_VREF_REC_TYP * CI_MAX_REC_TYP + CI_VDEF_REC_TYP:
      /* swap em then fall through */
      db_swap_recs (into_p, from_p);
      /* FALL THRU */

    case CI_LIB_VDEF_REC_TYP * CI_MAX_REC_TYP + CI_VREF_REC_TYP:
    case CI_VDEF_REC_TYP * CI_MAX_REC_TYP + CI_VREF_REC_TYP:
      /* addr_taken is the logical or of both fields */
      CI_U8_FM_BUF(into_p->db_rec + CI_ADDR_TAKEN_OFF, t1);
      CI_U8_FM_BUF(from_p->db_rec + CI_ADDR_TAKEN_OFF, t2);
      t1 |= t2;
      CI_U8_TO_BUF(into_p->db_rec + CI_ADDR_TAKEN_OFF, t1);

      /* usage count is additive. */
      CI_U32_FM_BUF(into_p->db_rec + CI_VDEF_USAGE_OFF, t1);
      CI_U32_FM_BUF(from_p->db_rec + CI_VREF_USAGE_OFF, t2);
      t1 += t2;
      CI_U32_TO_BUF(into_p->db_rec + CI_VDEF_USAGE_OFF, t1);
      break;

    case CI_VREF_REC_TYP * CI_MAX_REC_TYP + CI_VREF_REC_TYP:
      /* addr_taken is the logical or of both fields */
      CI_U8_FM_BUF(into_p->db_rec + CI_ADDR_TAKEN_OFF, t1);
      CI_U8_FM_BUF(from_p->db_rec + CI_ADDR_TAKEN_OFF, t2);
      t1 |= t2;
      CI_U8_TO_BUF(into_p->db_rec + CI_ADDR_TAKEN_OFF, t1);

      /* usage count is additive. */
      CI_U32_FM_BUF(into_p->db_rec + CI_VREF_USAGE_OFF, t1);
      CI_U32_FM_BUF(from_p->db_rec + CI_VREF_USAGE_OFF, t2);
      t1 += t2;
      CI_U32_TO_BUF(into_p->db_rec + CI_VREF_USAGE_OFF, t1);
      break;

    case CI_LIB_FDEF_REC_TYP * CI_MAX_REC_TYP + CI_LIB_FDEF_REC_TYP:
      /*
       * keep the first one of these seen, since they are in the list
       * in reverse just swap them, essentially causing the one at the front
       * of the list to be deleted.
       */
      db_swap_recs (into_p, from_p);
      break;

    case CI_LIB_FDEF_REC_TYP * CI_MAX_REC_TYP + CI_FDEF_REC_TYP:
      /*
       * keep the function in the program, this will override the
       * function in the library.
       */
      db_swap_recs (into_p, from_p);
      break;

    case CI_FDEF_REC_TYP * CI_MAX_REC_TYP + CI_LIB_FDEF_REC_TYP:
      /* keep the function in the program, this will override the
       * function in the library.
       */
      break;
    
    case CI_FREF_REC_TYP * CI_MAX_REC_TYP + CI_FREF_REC_TYP:
      /* addr_taken is the logical or of both fields */
      CI_U8_FM_BUF(into_p->db_rec + CI_ADDR_TAKEN_OFF, t1);
      CI_U8_FM_BUF(from_p->db_rec + CI_ADDR_TAKEN_OFF, t2);
      t1 |= t2;
      CI_U8_TO_BUF(into_p->db_rec + CI_ADDR_TAKEN_OFF, t1);
      break;

    case CI_FREF_REC_TYP * CI_MAX_REC_TYP + CI_LIB_FDEF_REC_TYP:
    case CI_FREF_REC_TYP * CI_MAX_REC_TYP + CI_FDEF_REC_TYP:
      db_swap_recs (into_p, from_p);
      /* FALL THRU */

    case CI_LIB_FDEF_REC_TYP * CI_MAX_REC_TYP + CI_FREF_REC_TYP:
    case CI_FDEF_REC_TYP * CI_MAX_REC_TYP + CI_FREF_REC_TYP:
      /* addr_taken is the logical or of both fields */
      CI_U8_FM_BUF(into_p->db_rec + CI_ADDR_TAKEN_OFF, t1);
      CI_U8_FM_BUF(from_p->db_rec + CI_ADDR_TAKEN_OFF, t2);
      t1 |= t2;
      CI_U8_TO_BUF(into_p->db_rec + CI_ADDR_TAKEN_OFF, t1);
      break;

    default:
      db_warning ("conflicting multiple definitions of %s\n", into_p->name);
      return;

    case CI_LIB_VDEF_REC_TYP * CI_MAX_REC_TYP + CI_LIB_FDEF_REC_TYP:
    case CI_LIB_VDEF_REC_TYP * CI_MAX_REC_TYP + CI_FDEF_REC_TYP:
    case CI_LIB_VDEF_REC_TYP * CI_MAX_REC_TYP + CI_FREF_REC_TYP:
    case CI_VDEF_REC_TYP * CI_MAX_REC_TYP + CI_LIB_FDEF_REC_TYP:
    case CI_VDEF_REC_TYP * CI_MAX_REC_TYP + CI_FDEF_REC_TYP:
    case CI_VDEF_REC_TYP * CI_MAX_REC_TYP + CI_FREF_REC_TYP:

    case CI_FREF_REC_TYP * CI_MAX_REC_TYP + CI_LIB_VDEF_REC_TYP:
    case CI_FREF_REC_TYP * CI_MAX_REC_TYP + CI_VDEF_REC_TYP:
    case CI_FREF_REC_TYP * CI_MAX_REC_TYP + CI_VREF_REC_TYP:

    case CI_VREF_REC_TYP * CI_MAX_REC_TYP + CI_LIB_FDEF_REC_TYP:
    case CI_VREF_REC_TYP * CI_MAX_REC_TYP + CI_FDEF_REC_TYP:
    case CI_VREF_REC_TYP * CI_MAX_REC_TYP + CI_FREF_REC_TYP:

    case CI_LIB_FDEF_REC_TYP * CI_MAX_REC_TYP + CI_LIB_VDEF_REC_TYP:
    case CI_LIB_FDEF_REC_TYP * CI_MAX_REC_TYP + CI_VDEF_REC_TYP:
    case CI_LIB_FDEF_REC_TYP * CI_MAX_REC_TYP + CI_VREF_REC_TYP:
    case CI_FDEF_REC_TYP * CI_MAX_REC_TYP + CI_LIB_VDEF_REC_TYP:
    case CI_FDEF_REC_TYP * CI_MAX_REC_TYP + CI_VDEF_REC_TYP:
    case CI_FDEF_REC_TYP * CI_MAX_REC_TYP + CI_VREF_REC_TYP:
      db_warning ("conflicting definition of %s as both function and variable\n",
               into_p->name);
      return;

    case CI_FDEF_REC_TYP * CI_MAX_REC_TYP + CI_FDEF_REC_TYP:
      db_warning ("multiple definitions of function %s in ccinfo\n", into_p->name);
      return;
  } 
  /* Weave together the lists of functions using the object */
  db_weave_lists (into_p, from_p);
}

int
db_check_static (st1_p, st2_p)
st_node *st1_p;
st_node *st2_p;
{
  if (!st1_p->is_static && !st2_p->is_static)
    return 0;

  if (st1_p->is_static && st1_p->is_static)
  {
    /* Its OK to have a reference and a def because these can both be from
     * the same file, anything else is not good.
     */
    switch (st1_p->rec_typ * CI_MAX_REC_TYP + st2_p->rec_typ)
    {
      case CI_LIB_VDEF_REC_TYP * CI_MAX_REC_TYP + CI_VREF_REC_TYP:
      case CI_VDEF_REC_TYP * CI_MAX_REC_TYP + CI_VREF_REC_TYP:
      case CI_VREF_REC_TYP * CI_MAX_REC_TYP + CI_LIB_VDEF_REC_TYP:
      case CI_VREF_REC_TYP * CI_MAX_REC_TYP + CI_VDEF_REC_TYP:

      case CI_LIB_FDEF_REC_TYP * CI_MAX_REC_TYP + CI_FREF_REC_TYP:
      case CI_FDEF_REC_TYP * CI_MAX_REC_TYP + CI_FREF_REC_TYP:
      case CI_FREF_REC_TYP * CI_MAX_REC_TYP + CI_LIB_FDEF_REC_TYP:
      case CI_FREF_REC_TYP * CI_MAX_REC_TYP + CI_FDEF_REC_TYP:
	return 0;

      default:
	/* Fall through to error return */
	break;
    } 
  }

  /* compiler seems to have produced a symbol clash in its renaming somehow. */
  db_warning ("compiler produced static variable name %s clashes with list name\n",
	    st1_p->name);
  return 1;
}

void
dbp_merge_syms(this)
dbase *this;
{
  /*
   * look for duplicate symbols in the table and merge their information
   * where they exist. This is made somewhat easier by knowing that
   * duplicates can only exist within a single hash table bucket.
   */
  int i;

  for (i = 0; i < CI_HASH_SZ; i++)
  {
    st_node *targ_p;

    targ_p = this->db_stab[i];

    while (targ_p != 0)
    {
      char *targ_name;
      st_node *prev_p;
      st_node *cur_p;

      targ_name = targ_p->name;
      prev_p = targ_p;
      cur_p  = targ_p->next;

      while (cur_p != 0)
      {
	if (strcmp(targ_name, cur_p->name) == 0)
	{
	  if (db_check_static(targ_p, cur_p) == 0)
	    db_merge_recs(targ_p, cur_p);

	  /* delete cur_p from the list */
	  cur_p = cur_p->next;
	  prev_p->next = cur_p;
	}
	else
	{
	  prev_p = cur_p;
	  cur_p = cur_p->next;
	}
      }

      /* move up the list for another target */
      targ_p = targ_p->next;
    }
  }
}

void
dbp_write_ccinfo(this,output)
dbase *this;
DB_FILE* output;
{
  /*
   * write the ccinfo header, then write the records,
   * then the symbol table, and finally the string table.
   */

  int i;
  long str_off;
  unsigned char header_buf[CI_HEAD_REC_SIZE];

  dbp_pre_write (this);

  CI_U16_TO_BUF(header_buf + CI_HEAD_MAJOR_OFF, CI_MAJOR | CI_MAJOR_MASK);
  CI_U8_TO_BUF (header_buf + CI_HEAD_MINOR_OFF, CI_MINOR);
  CI_U8_TO_BUF (header_buf + CI_HEAD_KIND_OFF,  this->kind);
  CI_U32_TO_BUF(header_buf + CI_HEAD_STAMP_OFF, this->time_stamp);

  CI_U32_TO_BUF(header_buf + CI_HEAD_DBSIZE_OFF, this->db_sz);
  CI_U32_TO_BUF(header_buf + CI_HEAD_SYMSIZE_OFF, this->sym_sz);
  CI_U32_TO_BUF(header_buf + CI_HEAD_STRSIZE_OFF, this->str_sz);
  CI_U32_TO_BUF(header_buf + CI_HEAD_TOT_SIZE_OFF, this->tot_sz);

  dbf_write (output, header_buf,CI_HEAD_REC_SIZE);

  for (i = 0; i < CI_HASH_SZ; i++)
  { st_node * list_p;
    for (list_p = this->db_stab[i]; list_p != 0; list_p = list_p->next)
    {
      if (list_p->db_rec_size != 0)
      {
        int j;
        int total_size = 0;

        /* Write out everything up to the first list ... */
        j = list_p->db_rec_size - list_p->db_list_size;
        assert (j >= 0);
  
        if (j > 0)
          dbf_write (output,list_p->db_rec,j);
  
        for (j = 0; j < CI_NUM_LISTS; j++)
        { db_list_node   *p = list_p->db_list[j];
  
          if (p)
          { unsigned char buf[4];

            int n = p->size + 4;
            total_size += n;

            /* Write out the final size of this list ... */
            assert (n >= 4);
            CI_U32_TO_BUF(buf, n);

            /* Remember where this record is in the output */
            p->out_tell = dbf_tell (output);

            dbf_write (output, buf, 4);

            if (n > 4)
            { /* There is a list of type 'j' to be written out.  The
                 length of each chunk is stored at the begining of each chunk,
                 and we get from chunk to chunk by following the 'next' fields
                 in the list descriptors. */
  
              int m = 0;
              int t1;

              while (p)
              { unsigned char *t;
    
                if (p->tell)
                { /* If this is a skip chunk, seek to len field in the file */ 
                  dbf_seek_set ((DB_FILE*)p->text, p->tell);
                  t = db_rw_buf;
                  assert (t != 0 && db_rw_buf_siz >=4);
                  dbf_read((DB_FILE*)p->text, t,4);
                }
                else
                  t = p->text;
    
                CI_U32_FM_BUF(t, t1);
                assert (t1 >= 4);
    
                m += (t1-4);
    
                /* Write out each chunk */
                if (t1 > 4)
                { if (p->tell)
                  { assert (db_rw_buf_siz >= (t1-4));
                    dbf_read((DB_FILE*)p->text,t+4, t1-4);
                  }
    
                  dbf_write (output,t+4, t1-4);
                }
    
                p = p->next;
              }
              assert (m == n-4);
            }
          }
        }
        assert (total_size == list_p->db_list_size);
      }
    }
  }

  /*
   * cruise though the list assigning the string table offsets,
   * and putting the symbol table to the file.
   */
  str_off = 0;
  for (i = 0; i < CI_HASH_SZ; i++)
  { st_node * list_p;

    for (list_p = this->db_stab[i]; list_p != 0; list_p = list_p->next)
    { if (list_p->db_rec_size != 0)
      { unsigned char sym_buf[CI_STAB_REC_SIZE];
    
        /* assign the string table offset */
        CI_U32_TO_BUF(sym_buf + CI_STAB_STROFF_OFF, str_off);
        CI_U32_TO_BUF(sym_buf + CI_STAB_DBOFF_OFF, list_p->db_rec_offset);
        CI_U32_TO_BUF(sym_buf + CI_STAB_DBRSZ_OFF, list_p->db_rec_size);
        CI_U16_TO_BUF(sym_buf + CI_STAB_HASH_OFF, i);
        CI_U8_TO_BUF(sym_buf + CI_STAB_STATIC_OFF, list_p->is_static);
        CI_U8_TO_BUF(sym_buf + CI_STAB_RECTYP_OFF, list_p->rec_typ);
  
        dbf_write (output,sym_buf,CI_STAB_REC_SIZE);
  
        str_off += list_p->name_length+1;
      }
    }
  }

  /* now put the string table out */
  for (i = 0; i < CI_HASH_SZ; i++)
  { st_node * list_p;
    for (list_p = this->db_stab[i]; list_p != 0; list_p = list_p->next)
    { if (list_p->db_rec_size != 0)
        dbf_write (output,list_p->name, list_p->name_length+1);
    }
  }
}

char*
db_oname(into, db_p)
char** into;
st_node* db_p;
{
  /* Return the base file name for db_p's output to into.  This
     is always a function strictly of the member name. */

  char *p, *s, *e, *buf;
  int n,c;

  assert (db_p->rec_typ==CI_SRC_REC_TYP);
  p = (char*) CI_LIST_TEXT (db_p, CI_SRC_MEM_LIST);
  n = CI_LIST_TEXT_LEN (db_p, CI_SRC_MEM_LIST);
  assert (n == strlen(p)+1);

  buf = db_buf_at_least (into, n + CI_OBJ_EXT + 2);

  s = p + (n-1);

  while (s>p && isalnum(s[-1]) || s[-1]=='_' || s[-1]=='.')
    s--;

  strcpy (buf, s);

  if (strlen(buf) > 8)
    buf[8] = '\0';

  if (e = strchr (buf, '.'))
    *e = '\0';

  if (*buf == '\0')
    strcpy (buf, "0");

  p = buf;
  while (c = *p)
  {
    if (c>='A' && c<='Z')
      *p = ('a' + (c-'A'));

    p++;
  }
  return buf;
}

int
db_is_subst (p)
st_node* p;
{
  return (db_sindex(p) != 0);
}

char*
db_buf_at_least (ptr,size)
char** ptr;
int    size;
{
  char* p = ptr ? *ptr : 0;

  assert (size >= 0);

  if (size & (sizeof(int)-1))
    size = (size &~ (sizeof(int)-1)) + sizeof(int);

  size += sizeof (int);

  if (p != 0)
  { int n = ((int*)p)[-1];

    assert (n>sizeof (int) && (n & (sizeof(int)-1))==0);

    if (size==sizeof (int))
    { free (p - sizeof(int));
      p = 0;
    }
    else
      if (n < size)
      { p = sizeof (int) + db_realloc (p - sizeof(int), size);
        ((int*)p)[-1] = size;
      }
  }

  if (p == 0 && size != sizeof(int))
  { p = sizeof(int) + db_malloc (size);
    ((int*)p)[-1] = size;
  }

  if (ptr)
    *ptr = p;

  return p;
}

void
db_set_arg (argc, argv, s)
int  *argc;
char ***argv;
char* s;
{
  int have_size, need_size;

  assert (argc && argv && *argc >= 0);

  (*argc)++;

  have_size = (*argv) ? (((int*)(*argv))[-1] - sizeof(int)) : 0;
  assert (have_size >= 0 && (have_size & (sizeof(int)-1)) == 0);
  need_size = ((*argc)+1) * sizeof (char*);

  if (have_size < need_size)
    db_buf_at_least (argv, need_size);

  (*argv)[(*argc)-1] = s;
  (*argv)[*argc] = 0;
}

char*
db_subst_name (into, p, c)
char** into;
st_node* p;
char c;
{
  int i = db_sindex (p);
  int d = db_dindex (p);
  static char *name;
  char *ret;

  assert (p->rec_typ == CI_SRC_REC_TYP);
  assert (i >= '0' && i<('0'+CI_MAX_SUBST));
  assert (d > 0 && c > 0);

  d--;

  if (d < 10)
    d = '0' + d;
  else if (d < 36)
    d = 'a' + d;
  else
    assert (0);

  db_oname (&name, p);
  sprintf (name+strlen(name), ".%c%c%c", d, i, c);

  ret = db_pdb_file (into, name);

  return ret;
}

static int noisy = 0;

int
db_set_noisy (v)
int v;
{
  int ret = noisy;
  noisy = v;
  return ret;
}

#if (defined(__STDC__) | defined(I386_NBSD1))
void
db_comment(char** buf, char* fmt, ...)
#else
void
db_comment(va_alist)
va_dcl
#endif
{
  static char* local_buf;

  va_list arg;
#if (defined(__STDC__) | defined (I386_NBSD1))
  va_start(arg, fmt);
#else
  char** buf;
  char* fmt;
  va_start(arg);
  buf = va_arg(arg, char**);
  fmt = va_arg(arg, char *);
#endif

  if (buf == 0)
    buf = &local_buf;

  { int n = (*buf) ? strlen(*buf) : 0;

    db_buf_at_least (buf, n+255);
    vsprintf ((*buf)+n, fmt, arg);
    n = strlen(*buf);

    if (n && (*buf)[n-1] == '\n')
    { 
      if (noisy)
        fprintf (stderr, "%s", *buf);
      (*buf)[0] = '\0';
    }
  }
  va_end (arg);
}

#if (defined(__STDC__) | defined(I386_NBSD1))
void
db_comment_no_buf(char* fmt, ...)
#else
void
db_comment_no_buf(va_alist)
va_dcl
#endif
{
  va_list arg;
#if (defined(__STDC__) | defined(I386_NBSD1))
  va_start(arg, fmt);
#else
  char* fmt;
  va_start(arg);
  fmt = va_arg(arg, char *);
#endif

  if (noisy)
    vfprintf (stderr, fmt, arg);

  va_end (arg);
}

static
char* removals[CI_MAX_RM];

char**
db_new_removal (s)
char* s;
{
  char** ret = 0;
  int i;

  assert (s);
  for (i = 0; i < CI_MAX_RM; i++)
    if (removals[i] == 0)
    { removals[i] = s;
      ret = &removals[i];
      break;
    }
  assert (ret);
  return ret;
}

void
db_remove_files (rm_buf)
char** rm_buf;
{ /* Remove the files in *rm_buf, but do not deallocate the space */
  char* t;
  int i;

  if (rm_buf == 0)
  { rm_buf = removals;
    i = CI_MAX_RM;
  }
  else
    i = 1;

  while (i--)
    if ((t = rm_buf[i]) && *t)
    { char* save = t;
    
      rm_buf[i] = 0;	/*  So we don't try again in case we get a signal 
    			    and come here again while unlinking ... */
      while (*t)
      { db_unlink (t, 0);
        t += strlen(t) + 1;
      }
    
      save[0] = '\0';
      rm_buf[i] = save;
    }
}

static char *progname;
static char *progbasename;

void
db_set_prog (name)
char* name;
{
  char* cp;

  progbasename = progname = name;

#ifdef DOS
  if (isalpha(progbasename[0]) && progbasename[1] == ':')
    progbasename += 2;
  cp = progbasename;
  for (; *cp; cp++)
    if (*cp == '/' || *cp == '\\')
      progbasename = cp+1;
#else
  cp = progbasename;
  for (; *cp; cp++)
    if (*cp == '/')
      progbasename = cp+1;
#endif
}

char* db_prog_name ()
{
  return progname;
}

char* db_prog_base_name ()
{
  return progbasename;
}

static char* pdb_name;

static char*
db_dir_name (to, from, x1, x2)
char** to;
char* from;
char* x1;
char* x2;
{
  assert (from);
  { int n = strlen(from);
    char* ret = strcpy (db_buf_at_least (to, n+strlen(x1)+strlen(x2)+2), from);

#ifdef DOS
    if (n == 0 || (from[n-1] != '\\' && from[n-1] != '/'))
      strcat (*to, "\\");
#else
    if (n==0 || from[n-1] != '/')
      strcat (*to, "/");
#endif
    strcat (ret, x1);
    strcat (ret, x2);
    return ret;
  }
}

void
db_note_pdb(p)
char* p;
{
  char *old_name = pdb_name;

  pdb_name = 0;
  db_dir_name (&pdb_name, p, "", "");

  if (old_name && strcmp (old_name, pdb_name))
    db_warning ("%s overrides previous pdb specification %s",pdb_name,old_name);

  /* Make the pdb directory if it doesn't already exist. */
  if (old_name == 0 || strcmp (old_name, pdb_name))
  { int  n = strlen(pdb_name);
    char c = pdb_name[n-1];
    int t;

#if defined(DOS) && defined(__HIGHC__)
    int dir_exists = EACCES;

    if (c=='\\' || c=='/')
      pdb_name[n-1] = '\0';
    else
      c = '\0';
    t = mkdir (pdb_name);
#else
    int dir_exists = EEXIST;

#if defined(__i960)
    /* For self-host, if the directory doesn't exist, just error out */
    db_fatal ("could not create PDB directory '%s'", pdb_name);
#else
    if (c=='/')
      pdb_name[n-1] = '\0';
    else
      c = '\0';
    t = mkdir (pdb_name, 0777);
#endif /* __i960 */
#endif /* DOS && __HIGHC__ */
    if (t == 0)
      db_warning ("created PDB directory '%s'", pdb_name);
    else
      if (errno != dir_exists)
        db_fatal ("could not access or create PDB directory '%s'", pdb_name);

    if (c)
      pdb_name[n-1] = c;
  }
}

char*
db_get_pdb()
{
  char* t;

  if ((t = pdb_name) == 0)
  { if ((t = db_genv2("960PDB")) == 0)
    { db_warning ("no PDB directory was specified;  defaulting to 'pdb'");
      t = "pdb";
    }

    db_note_pdb (t);
  }

  return pdb_name;
}

static char*
db_tool_name(buf, x1, x2)
char** buf;
char* x1;
char* x2;
{
  char *t = db_genv2("960BASE");

  if (t)
    t = db_dir_name (buf, t, x1, x2);
  else
    db_fatal ("you must set and export G960BASE or I960BASE");

  return t;
}

char*
db_asm_name(base)
char* base;
{
  /* Note that asm name varies, depending on object format. */
  static char* asm_name;

  char *t = db_genv2("960AS");
  
  if (t != 0)
    strcpy (db_buf_at_least (&asm_name, 1+strlen(t)), t);
  else
    db_tool_name (&asm_name, "bin/", base);

  return asm_name;
}

static char*
db_cc1_name()
{
  static char* cc1_name;

  if (cc1_name == 0)
  { char* t = db_genv2("960CC1");
  
    if (t != 0)
      strcpy (db_buf_at_least (&cc1_name, 1+strlen(t)), t);
    else
#ifdef DOS
      db_tool_name (&cc1_name, "lib/", "cc1.exe");
#else
      db_tool_name (&cc1_name, "lib/", "cc1.960");
#endif
  }

  return cc1_name;
}

static char*
db_cc1plus_name()
{
  static char* cc1plus_name;

  if (cc1plus_name == 0)
  { char* t = db_genv2("960CC1PLUS");
  
    if (t != 0)
      strcpy (db_buf_at_least (&cc1plus_name, 1+strlen(t)), t);
    else
#ifdef DOS
      db_tool_name (&cc1plus_name, "lib/", "cc1plus.exe");
#else
      db_tool_name (&cc1plus_name, "lib/", "cc1plus.960");
#endif
  }

  return cc1plus_name;
}

char*
db_cc_name(p)
st_node* p;
{
  char* ret;

  int k;

  assert (p->rec_typ == CI_SRC_REC_TYP);
  CI_U8_FM_BUF(p->db_rec+CI_SRC_REC_LANG_OFF, k);

  switch (k)
  { default:
      ret = 0;
      assert (0);
      break;

    case CI_LANG_C:
      ret = db_cc1_name();
      break;

    case CI_LANG_CPLUS:
      ret = db_cc1plus_name();
      break;
  }

  return ret;
}

char*
db_gcdm_name()
{
  static char* gcdm_name;

  if (gcdm_name == 0)
  { char* t = db_genv2("960DM");
  
    if (t != 0)
      strcpy (db_buf_at_least (&gcdm_name, 1+strlen(t)), t);
    else
#ifdef DOS
      db_tool_name (&gcdm_name, "bin/", "gcdm960.exe");
#else
      db_tool_name (&gcdm_name, "bin/", "gcdm960");
#endif
  }

  return gcdm_name;
}

char*
db_x_name()
{
  static char* x_name;

  if (x_name == 0)
  { char* t = db_genv2("960X");
  
    if (t != 0)
      strcpy (db_buf_at_least (&x_name, 1+strlen(t)), t);
    else
#ifdef DOS
      db_tool_name (&x_name, "lib/", "x960.exe");
#else
      db_tool_name (&x_name, "lib/", "x960");
#endif
  }

  return x_name;
}

char*
db_pdb_file (buf, file)
char** buf;
char* file;
{
  char* pdb = db_get_pdb();
  char* ret = db_buf_at_least (buf, 1 + strlen(pdb) + strlen(file));

  sprintf (ret, "%s%s", pdb, file);

  return ret;
}

#ifdef DOS
#define open_lock(N) open ((N), (_O_CREAT | _O_EXCL | _O_BINARY), _S_IREAD)
#define fdopen_lock(FD) fdopen ((FD), "wb")
#else
#define open_lock(N) open ((N), (O_CREAT | O_EXCL | O_RDWR), 0444)
#define fdopen_lock(FD) fdopen ((FD), "w")
#endif

static named_fd lock;

char*
db_lock ()
{
  if (lock.name == 0)
  { char *pdb = db_get_pdb();
    int t;
  
    lock.name = db_pdb_file (0, "pdb.lck");
  
    if ((t = open_lock (lock.name)) != -1)
      lock.fd = fdopen_lock (t);
  
    if (lock.fd == 0)
      if (errno == EEXIST)
        db_fatal ("PDB directory '%s' is locked by another user;  if the user is dead, please remove '%s', else wait for the user to finish", pdb, lock.name);
      else
        db_fatal ("cannot open '%s' for write", lock.name);
  
    /* If the current process dies, remove the lock file ... */
    db_new_removal (lock.name);

    dbf_close (&lock);
  }

  assert (lock.name && !lock.fd);
  return lock.name;
}

void
db_unlock ()
{
  if (lock.name)
  {
    db_unlink (lock.name, db_fatal);
    lock.name = 0;
  }
}

/* Dump routines below this line - should seperate into another module */

#include <stdio.h>
#include "assert.h"
#include <string.h>
#if (defined(__STDC__) | defined(I386_NBSD1))
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "cc_info.h"

FILE* db_dump_file;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

/*  COL_BITS denotes the number of bits we give (out of 32) to column position.
    MAX_LIST_COL is based on COL_BITS.  All column positions >= MAX_LIST_COL
    are represented as MAX_LIST_COL. */

#define COL_BITS 8
#define MAX_LIST_COL (((1 << COL_BITS)-1))

#define GET_LINE(P)  (( ((unsigned)(P)) >> COL_BITS ))
#define GET_COL(P)   (( (P) & MAX_LIST_COL ))

static u8 *
unpk_num(p, num_p)
u8 *p;
int *num_p;
{
  int t;
  switch (((t = *p++) >> 6) & 0x3)
  {
    case 0:
      if ((t & 0x20) != 0)
        t |= (-1 << 6);
      break;

    case 1:
      t &= 0x3F;
      t |= *p++ << 6;
      if ((t & 0x2000) != 0)
        t |= (-1 << 14);
      break;

    case 2:
      t &= 0x3F;
      t |= *p++ << 6;
      t |= *p++ << 14;

      if ((t & 0x200000) != 0)
        t |= (-1 << 22);
      break;

    case 3:
      t &= 0x3F;
      t |= *p++ << 6;
      t |= *p++ << 14;
      t |= *p++ << 22;

      if ((t & 0x20000000) != 0)
        t |= (-1 << 30);
      break;
  }
  *num_p = t;
  return p;
}

static char *
rectype_string(rectype)
int rectype;
{
  char *ret = "Unknown record type";

  switch (rectype)
  {
    case CI_FDEF_REC_TYP:
      ret = "Function Definition Record"; break;
    case CI_VDEF_REC_TYP:
      ret = "Variable Definition Record"; break;
    case CI_FREF_REC_TYP:
      ret = "Function Reference Record"; break;
    case CI_VREF_REC_TYP:
      ret = "Variable Reference Record"; break;
    case CI_LIB_FDEF_REC_TYP:
      ret = "Library Function Definition Record"; break;
    case CI_LIB_VDEF_REC_TYP:
      ret = "Library Variable Definition Record"; break;
    case CI_PROF_REC_TYP:
      ret = "Profile Information Record"; break;
    case CI_CG_REC_TYP:
      ret = "Call Graph Record"; break;
    case CI_SRC_REC_TYP:
      ret = "Source Code Record"; break;
    default:
      break;
  }
  return ret;
}

static void
dump_inln(p, fmt, n_calls)
u8 *p;
char* fmt;
int n_calls;
{
  int i,v,len,vsize,nvect,nb;
  char buf[1024];

  CI_U32_FM_BUF(p, len);
  p += 4;
  len -= 4;

  vsize = CI_FDEF_IVECT_SIZE(n_calls);

  if (vsize == 0)
    nvect = 0;
  else
    nvect = len/vsize;

  /* Make sure we have an integral number of vectors */
  assert (nvect * vsize == len);
  sprintf (buf, fmt, nvect, n_calls);
  fprintf (db_dump_file, "%s\n", buf);


  nb = strlen (buf) -19;
  for (i = 0; i < nb; i++)
    buf[i]=' ';
  buf[i] = '\0';

  for (v = 0; v < nvect; v++)
  { i = 0;
    while (i < n_calls)
    { int k;
      if ((k=i+9) >= n_calls)
        k=n_calls-1;
      fprintf (db_dump_file, "%s vector %d[%2d..%2d]: ", buf, v,i,k);
      for (; i <= k; i++)
      { int c = CI_FDEF_IVECT_VAL(p,i);

        if (c==CI_FDEF_IVECT_NOTHING)
          c='.';
        else if  (c==CI_FDEF_IVECT_INLINE)
          c='I';
        else if (c==CI_FDEF_IVECT_RECURSIVE)
          c='R';
        else
          c='?';

        fprintf (db_dump_file, "%c", c);
      }
      fprintf (db_dump_file, "\n");
    }
    p += vsize;
  }
}

static int
get_encoded_plength(p)
u8 *p;
{
  int length;

  CI_U16_FM_BUF(p, length);
  return length & 0x7FFF;
}

static char *
dump_encoded_length(p)
u8 *p;
{
  static char buf[100];
  int length;

  CI_U16_FM_BUF(p, length);

  sprintf (buf, "nparms(%d), is_varargs(%d)",
           length & 0x7FFF,
           (length & 0x8000) >> 15);

  return buf;
}

static char *
dump_encode_type(p)
u8 *p;
{
  static char buf[100];
  static char *t_name[4] = {"t_int", "t_ptr", "t_float", "t_aggr"};

  int ty;
  int align;
  int size;
  int dummy;

  CI_U16_FM_BUF(p, dummy);
  ty = ((dummy >> 14) & 0x3);
  align = 1 << ((dummy >> 11) & 0x7);
  size  = align * (dummy & 0x7FF);

  sprintf (buf, "ty(%s), align(%d), size(%d)", t_name[ty], align, size);
  return buf;
}

static int
dump_encoded_ftype(p, indent)
u8 *p;
char *indent;
{
  int n_parms;
  int i;
  int length = 0;

  n_parms = get_encoded_plength(p);

  fprintf (db_dump_file, "%s%s ", indent, dump_encoded_length(p+length));
  length += 2;
  fprintf (db_dump_file, "return = %s\n", dump_encode_type(p+length));
  length += 2;

  for (i = 0; i < n_parms; i++)
  {
    fprintf (db_dump_file, "%sparm[%d] = %s\n",
             indent, i, dump_encode_type(p+length));
    length += 2;
  }

  return length;
}

static void
dump_ftype_list(p, fmt)
u8 *p;
char* fmt;
{
  int len;
  CI_U32_FM_BUF(p, len);

  p += 4;
  
  fprintf (db_dump_file, "%s\n", fmt);

  if (len != 4)
  { int tlen = dump_encoded_ftype(p, "        ");
    assert ((tlen + 4) == len);
  }
}

static void
dump_call_ptype_list(p, fmt)
u8 *p;
char* fmt;
{
  int len;
  int tlen;
  int i = 0;

  CI_U32_FM_BUF(p, len);
  p += 4;
  len -= 4;

  fprintf (db_dump_file, "%s\n", fmt);
  while (len > 0)
  {
    fprintf (db_dump_file, "        call_ptype for call %d:\n", i);
    tlen = dump_encoded_ftype(p, "          ");
    i += 1;
    len -= tlen;
    p += tlen;
  }
}

static void
dump_cfg(p, fmt)
u8 *p;
char* fmt;
{
  int a,i,len,narc,nb;
  char buf[1024];

  CI_U32_FM_BUF(p, len);
  p += 4;
  len -= 4;

  assert (len >= 0 && (len % 6) == 0);

  narc = len / 6;

  sprintf (buf, fmt, narc);
  fprintf (db_dump_file, "%s", buf);

  nb = strlen (buf);
  for (i = 0; i < nb; i++)
    buf[i]=' ';
  buf[i] = '\0';

  for (a = 0; a < narc; a++)
  { unsigned fm,to,id;

    CI_U16_FM_BUF(p,fm);  p += 2;
    CI_U16_FM_BUF(p,to);  p += 2;
    CI_U16_FM_BUF(p,id);  p += 2;

    if (a == 0)
      fprintf (db_dump_file, "%3d.%-3d(%d)", fm, to, id);
    else if ((a % 8) == 0)
    { fprintf (db_dump_file, "\n%s", buf);
      fprintf (db_dump_file, "%3d.%-3d(%d)", fm, to, id);
    }
    else
      fprintf (db_dump_file, " %3d.%-3d(%d)", fm, to, id);
  }

  fprintf (db_dump_file, "\n");
}

static void
dump_targets (p, fmt)
u8 *p;
char* fmt;
{
  int a,i,len,ntrg,nb;
  char buf[1024];
  unsigned long sz [NUM_TS];

  CI_U32_FM_BUF(p, len);
  p += 4;
  len -= 4;

  assert (len >= 0 && (len % 2) == 0);

  ntrg = len / 2;
  sprintf (buf, fmt, ntrg);
  fprintf (db_dump_file, "%s", buf);

  if (ntrg)
  {
    assert (NUM_TS == ntrg);
    db_unpack_sizes (p, sz);
  
    nb = strlen (buf);
    for (i = 0; i < nb; i++)
      buf[i]=' ';
    buf[i] = '\0';
  
    for (a = 0; a < ntrg; a++)
    { unsigned long targ;
  
      targ = sz[a];
  
      if (a == 0)
        fprintf (db_dump_file, "%d", targ);
      else if ((a % 8) == 0)
      { fprintf (db_dump_file, "\n%s", buf);
        fprintf (db_dump_file, "%d", targ);
      }
      else
        fprintf (db_dump_file, " %d", targ);
    }
  }

  fprintf (db_dump_file, "\n");
}


static void
dump_list_text (p, n, fmt)
u8 *p;
char* fmt;
{
  int s;
  u8 *end, endch;

  char buf[255];

  if (fmt == 0)
    fmt = "";

  assert (n >= 0);

  strcpy (buf, fmt);
  strcat (buf, "(");
  s = strlen (buf);

  if (n==0)
  { fprintf (db_dump_file, "%s", buf);
    end = 0;
    endch = 0;
  }
  else
  { /* Make sure that the last character is a 0. */

    end = p + (n-1);
    endch = *end;
    *end = 0;
  }

  while (n > 0)
  { int i,j;

    while ((i=strlen ((char *)p)) < 71 &&
           (s+i+(j=strlen((char *)(p+i+1)))) < 71 &&
           (i+j) < (n-1))
      p[i]=',';

    fprintf (db_dump_file, "%s%s", buf, p);

    n -= (i+1);
    p += (i+1);

    if (fmt)
    { i = s;
      buf[i] = '\0';
      while (i)
        buf[--i] = ' ';
      fmt = 0;
    }

    if (n > 0)
      fprintf (db_dump_file, "\n");
  }

  assert (n==0);

  if (endch)
  { fprintf (db_dump_file, "%c<no '\\0'>)", endch);
    *end = endch;
  }
  else
    fprintf (db_dump_file, ")\n");
}

static void
dump_call (p, fmt, n_calls)
u8 *p;
int n_calls;
char* fmt;
{
  int n;
  char buf[1024];
  char* name = "";
  unsigned lo = -1;
  unsigned hi = -1;
  
  CI_U32_FM_BUF(p, n);

  n -= 4;
  p += 4;

  if (n)
  {
    n -= n_calls;
    p += n_calls;

    name = (char*) p;
    n -= strlen((char *)p) + 1;
    p += strlen((char *)p) + 1;
    
    CI_U32_FM_BUF(p, lo);
    n -= 4;
    p += 4;
    CI_U32_FM_BUF(p, hi);
    n -= 4;
    p += 4;

    n -= 4 * n_calls;
    p += 4 * n_calls;
    sprintf (buf, fmt, name, lo, hi);
    assert (n >= 0);
  }
  else
    sprintf (buf, "no call info");

  dump_list_text (p, n, buf);
}

static void
dump_saved_ir(buf)
u8 *buf;
{
}

static void
dump_list (p, fmt)
u8 *p;
char* fmt;
{
  int n;

  CI_U32_FM_BUF(p, n);
  assert (n >= 4);

  dump_list_text (p+4, n-4, fmt);
}

static void
dump_src (p, fmt)
u8 *p;
char* fmt;
{
  int n,i,l;
  u8 *tmp, *q;
  u8 **s;
  char buf[255];

  CI_U32_FM_BUF(p, n);
  assert (n >= 4);
  tmp = (u8*) db_malloc (n+1);
  memcpy (tmp, p, n);
  p = tmp;
  p[n] = '\0';

  l = 0;

  /* Count lines */
  if (n > 4)
  {
    q = (u8*) strchr ((char *)(p+8), '\n');
    while (q && q<(p+n))
    { l++;
      q = (u8 *) strchr ((char *)(q+1),'\n');
    }
  }

  sprintf (buf, fmt, n, l);

  if (l == 0)
  { unsigned off;
    CI_U32_FM_BUF(p+4, off);
    fprintf (db_dump_file, "%sseek offset %d of %s\n", buf, off, p+8);
  }
  else
  {
    fprintf (db_dump_file, "%s\n", buf);
  
    i = strlen (buf);
    if (i > 8)
      buf[i-=8]='\0';
  
    while (i-- > 0)
      buf[i] = ' ';

    /* Get pointers to individual lines */
    s = (u8**) db_malloc ((l+2) * sizeof (char *));

    l = 0;
  
    if (n > 4)
    {
      s[0] = p+4;
      q = (u8*) strchr ((char *)(p+4), '\n');
      while (q && q<(p+n))
      { *q = '\0';
        s[++l] = ++q;
        q = (u8*) strchr ((char *)q,'\n');
      }
    }
  
    if (l < 19)
    {
      for (i=0; i < l; i++)
        fprintf (db_dump_file, "%s%6d: %s\n", buf, i+1, s[i]);
    }
    else
    {
      for (i=0; i < 9; i++)
        fprintf (db_dump_file, "%s%6d: %s\n", buf, i+1, s[i]);
      fprintf (db_dump_file,"%s      : <... skipping ...>\n", buf);
      for (i=l-9; i < l; i++)
        fprintf (db_dump_file, "%s%6d: %s\n", buf, i+1, s[i]);
    }
  }
}

static void
dump_prof (p, fmt)
u8 *p;
char* fmt;
{
  int n,i,l,v;
  u8 *ret;
  char buf[255];

  CI_U32_FM_BUF(p, n);
  assert (n >= 4);
  ret = p + n;
  n -= 4;
  assert ((n & 3) == 0);
  n /= 4;

  sprintf (buf, fmt, n);
  fprintf (db_dump_file, "%s", buf);
  i = strlen (buf);
  while (i-- > 0)
    buf[i] = ' ';

  p += 4;

  for (i=l=0; i < n; i++)
  {
    if (l == 8)
    { fprintf (db_dump_file, "\n%s", buf);
      l = 0;
    }
    else
      l++;

    CI_U32_FM_BUF(p, v);
    if (v == -1)
      fprintf (db_dump_file, " maxuint ", v);
    else
      fprintf (db_dump_file, "%8u ", v);
    p += 4;
  }
  fprintf (db_dump_file, "\n");
}

static unsigned char*
get_dmp_list (sym_p, lnum)
st_node* sym_p;
int lnum;
{
  unsigned char* ret;
  db_list_node* l = sym_p->db_list[lnum];

  if (l->tell == 0)
    ret = l->text;
  else
  { unsigned len;
    static unsigned char buf[64];

    CI_U32_FM_BUF (l->text, len);
    CI_U32_TO_BUF (buf, 36);
    sprintf ((char *)(buf+4), "tell %10u size %10u", l->tell, l->size);
    ret = buf;
  }
  return ret;
}

static void
dump_func_def_record(sym_p)
st_node *sym_p;
{
  u8 *buf = sym_p->db_rec;
  u8  addr_taken, can_delete;
  u32 tot_inline;
  u16 n_insns, n_parms, n_calls, p_qual, n_voters;
  u8  n_regs;

  int l;

  addr_taken = db_rec_addr_taken (sym_p);

  CI_U16_FM_BUF(buf + CI_FDEF_TOT_INLN_OFF, tot_inline);
  CI_U16_FM_BUF(buf + CI_FDEF_NINSN_OFF, n_insns);
  CI_U16_FM_BUF(buf + CI_FDEF_NPARM_OFF, n_parms);
  CI_U16_FM_BUF(buf + CI_FDEF_NCALL_OFF, n_calls);
  CI_U8_FM_BUF(buf + CI_FDEF_REG_USE_OFF, n_regs);
  CI_U16_FM_BUF(buf + CI_FDEF_PQUAL_OFF, p_qual);
  CI_U16_FM_BUF(buf + CI_FDEF_VOTER_OFF, n_voters);
  CI_U8_FM_BUF(buf + CI_FDEF_CAN_DELETE_OFF, can_delete);

  fprintf (db_dump_file, "    address taken? = %d\n", addr_taken);
  fprintf (db_dump_file, "    can_delete?    = %d\n", can_delete);
  fprintf (db_dump_file, "    total inline   = %u\n", tot_inline);
  fprintf (db_dump_file, "    n_insns        = %u\n", n_insns);
  fprintf (db_dump_file, "    n_parms        = %u\n", n_parms);
  fprintf (db_dump_file, "    n_calls        = %u\n", n_calls);
  fprintf (db_dump_file, "    n_regs_used    = %u\n", n_regs);
  fprintf (db_dump_file, "    prof quality   = %u\n", p_qual);
  fprintf (db_dump_file, "    prof voters    = %u\n", n_voters);

  for (l = CI_FDEF_LIST_LO; l < CI_FDEF_LIST_HI; l++)
  { u8* tmp = get_dmp_list (sym_p, l);

    switch (l)
    {
      default:
        dump_list (tmp, "    ??? list ???: ");
        break;

      case CI_FDEF_IVEC_LIST:
        dump_inln (tmp, "    %d inline decision vectors of %2d calls each: ",
                        n_calls);
        break;

      case CI_FDEF_REGP_LIST:
        dump_call (tmp, "    name %s f=%d, l=%d, calls: ", n_calls);
        break;

      case CI_FDEF_RTL_LIST:
        dump_saved_ir(tmp);
        break;

      case CI_FDEF_FILE_LIST:
        dump_list (tmp, "    defined in files: ");
        break;

      case CI_FDEF_FUNC_LIST:
        dump_list (tmp, "    fref in functions: ");
        break;

      case CI_FDEF_PROF_LIST:
        dump_prof (tmp, "    %d profile counters: ");
        break;

      case CI_FDEF_HIST_LIST:
        dump_list (tmp, "    have inline vectors for: ");
        break;

      case CI_FDEF_CFG_LIST:
        dump_cfg  (tmp, "    %d cfg arcs: ");
        break;

      case CI_FDEF_FTYPE_LIST:
        dump_ftype_list(tmp, "    function type: ");
        break;

      case CI_FDEF_CALL_TYPE_LIST:
        dump_call_ptype_list(tmp, "    call types: ");
        break;

      case CI_FDEF_TSINFO_LIST:
        dump_targets (tmp, "    %d tsinfo targets: ");
        break;
    }
  }
}

static void
dump_prof_record(sym_p)
st_node *sym_p;
{
  u8 *buf = sym_p->db_rec;
  u32 n_blks;
  u32 n_cnts;
  u32 n_lines;
  u32 prof_size;
  int i;
  int tmp;

  CI_U32_FM_BUF(buf+CI_PROF_NBLK_OFF, n_blks);
  CI_U32_FM_BUF(buf+CI_PROF_NCNT_OFF, n_cnts);
  CI_U32_FM_BUF(buf+CI_PROF_NLINES_OFF, n_lines);
  CI_U32_FM_BUF(buf+CI_PROF_SIZE_OFF, prof_size);
  buf = CI_LIST_TEXT (sym_p, CI_PROF_FNAME_LIST);

  fprintf (db_dump_file, "    n_blks         = %d\n", n_blks);
  fprintf (db_dump_file, "    n_cnts         = %u\n", n_cnts);
  fprintf (db_dump_file, "    n_lines        = %u\n", n_lines);
  fprintf (db_dump_file, "    prof_size      = %u\n", prof_size);
  fprintf (db_dump_file, "    file_name      = %s\n", buf);

  buf += strlen((char *)buf) + 1;
  for (i= 0; i < n_blks; i++)
  {
    fprintf (db_dump_file, "    block_info[%d] =\n", i);
    fprintf (db_dump_file, "      lines     =");
    buf = unpk_num(buf, &tmp);
    while (tmp != 0)
    {
      fprintf (db_dump_file, " (%d,%d)", GET_LINE(tmp), GET_COL(tmp));
      buf = unpk_num(buf, &tmp);
    }

    fprintf (db_dump_file, "\n      formula   =");
    buf = unpk_num(buf, &tmp);
    while (tmp != 0)
    {
      fprintf (db_dump_file, " %d", tmp);
      buf = unpk_num(buf, &tmp);
    }

    fprintf (db_dump_file, "\n      var_usage =");
    while (*buf != '\0')
    {
      fprintf (db_dump_file, " %s", buf);
      buf += strlen((char *)buf) + 1;
    }
    fprintf (db_dump_file, "\n");
    buf += 1;  /* to get past terminating '\0' */
  }
}

void
db_dump_symbol(sym_p, hash)
st_node *sym_p;
int hash;
{
  fprintf (db_dump_file, "Symbol = %s\n", sym_p->name);
  fprintf (db_dump_file, "  data_offset = %u\n", sym_p->db_rec_offset);
  fprintf (db_dump_file, "  data_size   = %u\n", sym_p->db_rec_size);
  fprintf (db_dump_file, "  hash_code   = %d\n", hash);
  fprintf (db_dump_file, "  is_static   = %d\n", sym_p->is_static);
  fprintf (db_dump_file, "  rectype     = %s\n", rectype_string(sym_p->rec_typ));
  fprintf (db_dump_file, "  record =\n");

  switch (sym_p->rec_typ)
  { int addr_taken;

    case CI_SRC_REC_TYP:
      dump_list(get_dmp_list(sym_p,CI_SRC_LIB_LIST),  "    archive name: ");
      dump_list(get_dmp_list(sym_p,CI_SRC_MEM_LIST),  "    member name: ");
      dump_list(get_dmp_list(sym_p,CI_SRC_CC1_LIST),  "    cc1 command: ");
      dump_src(get_dmp_list(sym_p,CI_SRC_SRC_LIST),  "    %d bytes,%d lines: ");
      dump_list(get_dmp_list(sym_p,CI_SRC_ASM_LIST),  "    asm command: ");
      dump_list(get_dmp_list(sym_p,CI_SRC_SUBST_LIST),"    subst history: ");
      break;

    case CI_FDEF_REC_TYP:
    case CI_LIB_FDEF_REC_TYP:
      dump_func_def_record(sym_p);
      break;

    case CI_LIB_VDEF_REC_TYP:
    case CI_VDEF_REC_TYP:
      addr_taken = db_rec_addr_taken (sym_p);
      fprintf (db_dump_file, "    address taken? = %d\n", addr_taken);

      dump_list(get_dmp_list(sym_p,CI_VDEF_FILE_LIST),"    defined in files: ");
      dump_list(get_dmp_list(sym_p,CI_VDEF_FUNC_LIST),"    used in functions: ");
      break;

    case CI_FREF_REC_TYP:
      addr_taken = db_rec_addr_taken (sym_p);
      fprintf (db_dump_file, "    address taken? = %d\n", addr_taken);

      dump_list(get_dmp_list(sym_p,CI_FREF_FUNC_LIST),"    fref in functions: ");
      break;

    case CI_VREF_REC_TYP:
      addr_taken = db_rec_addr_taken (sym_p);
      fprintf (db_dump_file, "    address taken? = %d\n", addr_taken);

      dump_list(get_dmp_list(sym_p,CI_VREF_FUNC_LIST),"    used in functions: ");
      break;

    case CI_PROF_REC_TYP:
      dump_prof_record(sym_p);
      break;

    case CI_GLD_REC_TYP:
    { unsigned long v;
      v = db_gld_value (sym_p);
      fprintf (db_dump_file, "    value = 0x%x\n", v);
      break;
    }

    case CI_CG_REC_TYP:
      break;

    default:
      break;
  }
  fprintf (db_dump_file, "\n");
}

int
db_debug_list(l)
db_list_node* l;
{
  int n;
  FILE* save = db_dump_file;

  db_dump_file = stderr;
  CI_U32_FM_BUF (l->text, n);

  fprintf (stderr, "text %x, size %d, text->size %d, text+text->size %x\n",
                   l->text, l->size, n, l->text+n);

  dump_list (l->text, "");
  db_dump_file = save;

  return 1;
}

int
db_debug_sym(p)
st_node* p;
{
  FILE* save = db_dump_file;

  db_dump_file = stderr;
  db_dump_symbol(p, 0);
  db_dump_file = save;


  return 1;
}

int
db_get_list_text_len (p,l)
st_node* p;
int l;
{
  unsigned char* t = db_get_list (p,l);
  int len;

  assert (t);
  CI_U32_FM_BUF (t, len);
  assert (len >= 4);

  return len-4;
}

int db_is_iprof(s)
char* s;
{ /* Return 2 if s is an iascii profile file,
            1 is s is a binary profile file.

     We say that we have an ascii profile file
     when byte 3 is a printable character.  This
     means that binary profile files can be at most
     (9 << 24) bytes long.
  */

  int ret = 0;

  named_fd f;
  dbf_open_read (&f, s, 0);

  if (f.fd)
  { unsigned char buf[4];  int len;

    dbf_read (&f, buf, sizeof (int));

    if (buf[3] >= 9 && (buf[3] & 0x80) == 0)
      ret = 2;
    else
    {
      CI_U32_FM_BUF(buf, len);
      len += 4;

      dbf_seek_end (&f, 0);

      if (dbf_tell (&f) == len)
        ret = 1;
    }

    dbf_close (&f);
  }

  return ret;
}

int
db_is_kind (s, kind)
char* s;
int kind;
{
  int ret = 0;

  named_fd f;
  dbf_open_read (&f, s, 0);

  if (f.fd)
  { char buf[4];
    ci_head_rec head;

    dbf_read (&f, buf, 4);

    if (db_get_magic (buf, &head) && head.kind_ver == kind)
    { unsigned len;
      /* We seem to have the right kind of db ... */

      dbf_read (&f, buf, 4);
      CI_U32_FM_BUF (buf, len);

      dbf_seek_end (&f, 0);

      if (dbf_tell (&f) == len)
        ret = 1;
    }

    dbf_close (&f);
  }

  return ret;
}

int
db_page_size()
{
  return 55;
}

static
char* comma (t)
char* t;
{
  /* Find the next argument seperator.  This is currently used for
     all cases except m= and sram=. */

  while (*t && *t != ' ' && !(*t == ',' && t[1] != ','))
    if (*t++ == ',')
    { assert (*t == ',');
      t++;
    }
  return t;
}

static
char* sram_comma (t)
char* t;
{
  /* Find the next argument seperator for m= and sram=.  We want to
     allow embedded commas when followed by digits, because of our
     old a-b,c-d, ... syntax.  */

  while (*t && *t != ' ' && !(*t == ',' && (t[1] != ',') && !isdigit(t[1])))
    if (*t++ == ',')
    { assert (*t == ',' || isdigit(*t));
      t++;
    }
  return t;
}

int
db_find_arg (s, x, y)
char** s;
char** x;
char** y;
{
  typedef struct arg_info_ {
    char* base;
    int arg_kind;
    char *(*get_seperator)();
  } arg_info;
  
  static arg_info gcdm_sw[]
  ={
    /* 1 optional argument */
    "iprof=", 0, comma,
  
    /* 1 required argument */
    "inline=", 1, comma,
    "sram=", 1, sram_comma,
    "m=", 1, sram_comma,
    "text_size=", 1, comma,
    "code_size", 1, comma,
    "subst=", 1, comma,
    "nosubst=", 1, comma,
    "ref=", 1, comma,
    "noref=", 1, comma,
    "dec=", 1, comma,
  
    /* No arguments */
    "dryrun", -1, comma,
    "v", -1, comma,
    "rcall-graph", -1, comma,
    "rclosure", -1, comma,
    "rdecisions", -1, comma,
    "rprofile", -1, comma,
    "rreverse", -1, comma,
    "rsummary", -1, comma,
    "rvariables", -1, comma,
    "ic960", -1, comma,
    "gcc960", -1, comma,
    0, 0, 0,
   };

  char *r = *s;
  arg_info *sw;

  *x = 0;
  *y = 0;

  /* Allow leading separators */
  while (*r && (*r==','||*r==' '))
    r++;

  if (*r)
  { /* '-' or '/' is optional. */
    if (IS_OPTION_CHAR(*r))
      r++;

    for (sw = gcdm_sw; sw->base; sw++)
    { int n = strlen (sw->base);

      if (!strncmp (sw->base, r, n))
      { char *t = sw->get_seperator (r+n);

        if ((sw->arg_kind  < 0 && t == r+n)	/* No arg allowed */
          ||(sw->arg_kind  > 0 && t >  r+n)	/* Required arg   */
          ||(sw->arg_kind == 0 && t >= r+n))	/* Optional arg   */

        { int m = (t-r)+2;	/* space for '-', arg, '\0' */

          *x = db_malloc (m);
          (*x)[0] = '-';
          memcpy ((*x)+1, r, m-2);
          (*x)[m-1] = '\0';

          if (sw->arg_kind >= 0)
          { *y = *x +n +1;
            (*x)[n] = '\0';
          }
          *s = t;	/* Advance s to point at seperator */
        }
        break;
      }
    }

    if (*x == 0)
      db_fatal ("'%s' is not a legal argument to the 'gcdm' switch",  *s);
  }

  return (*x != 0) + (*y != 0);
}

void  (*db_assert_fail)() = db_fatal;

