#include "config.h"

#ifdef IMSTG
/* Contributed by Kevin B. Smith. Intel Corp.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef macintosh
@pragma segment GNUH
#endif

#include <sys/types.h>
#ifndef DOS
#include <sys/file.h>
#endif
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include "obstack.h"
#include "rtl.h"
#include "tree.h"
#include "flags.h"
#include "expr.h"
#include "output.h"
#include "regs.h"
#include "cc_info.h"
#include "i_profile.h"
#include "i_prof_form.h"
#include "assert.h"

#ifdef GCC20
#include "insn-flags.h"
#include "i_glob_db.h"
#include "hard-reg-set.h"
#else
#include "insn_flg.h"
#include "glob_db.h"
#include "hreg_set.h"
#endif

extern void free ();
extern char *(rindex)();

extern struct obstack *rtl_obstack;

extern FILE *asm_out_file;
extern char *get_call_name();

#define get_cache_slot(p) (((struct cache_rec*) p->extra))
#define set_cache_slot(p,s) (p->extra=(char *) s)

#define BLK_PROF_BUF_SZ 2000

struct blk_prof_info_st
{
  struct blk_prof_info_st * next;
  int n_used;
  unsigned char buf[BLK_PROF_BUF_SZ];
};

extern int func_number;
static char *in_db_name;

static struct blk_prof_info_st b_prof_info = {0, 0 };
static struct blk_prof_info_st *cur_b_prof_info = &b_prof_info;

static
struct blk_prof_info_st *
new_prof_blk()
{
  struct blk_prof_info_st * new_inf;

  new_inf = (struct blk_prof_info_st *)xmalloc(sizeof(struct blk_prof_info_st));
  new_inf->next = 0;
  new_inf->n_used = 0;
  
  cur_b_prof_info->next = new_inf;
  cur_b_prof_info = new_inf;
  return new_inf;
}

#define PUTBYTE(b) \
  do { \
    struct blk_prof_info_st *__x = cur_b_prof_info; \
    if (__x->n_used >= BLK_PROF_BUF_SZ) \
      __x = new_prof_blk(); \
    __x->buf[__x->n_used++] = (b); \
  } while (0)

static
void pk_num(num)
int num;
{
  if (num >= -0x20 && num <= 0x1f)
  {
    PUTBYTE((num & 0x3F));
  }
  else if (num >= -0x2000 && num <= 0x1fff)
  {
    PUTBYTE((num & 0x3F) | (1 << 6));
    PUTBYTE((num >> 6) & 0xFF);
  }
  else if (num >= -0x200000 && num <= 0x1fffff)
  {
    PUTBYTE((num & 0x3F) | (2 << 6));
    PUTBYTE((num >> 6) & 0xFF);
    PUTBYTE((num >> 14) & 0xFF);
  }
  else
  {
    PUTBYTE((num & 0x3F) | (3 << 6));
    PUTBYTE((num >> 6) & 0xFF);
    PUTBYTE((num >> 14) & 0xFF);
    PUTBYTE((num >> 22) & 0xFF);
  }
}

static
unsigned char * get_pk_num(p, num_p)
unsigned char *p;
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

/*
 * Used for managing a cache of data for a symbol table node.
 */
#define ST_CACHE_SZ 30

struct cache_rec {
  st_node *st_p;
  struct cache_rec *next_cache_slot;
  struct cache_rec *prev_cache_slot;
};

/*
 * The hash table for the data base symbol table.
 */
static st_node *db_stab[CI_HASH_SZ];

/*
 * This array is used to represent a cache of db records that are kept
 * in memory.
 */
struct cache_rec st_cache[ST_CACHE_SZ];
/*
 * these global variables define the head and tail of the cache queue.
 * This queue is managed such that the head of the queue is always the
 * last used item in the cache, and the tail is always the most recently
 * used item, and those in between maintain this relationship.
 */
struct cache_rec * st_cache_head;
struct cache_rec * st_cache_tail;

/*
 * This variable is used to simply maintain a list of symbol table nodes
 * to be output at the end of the data base file.
 */
static st_node *stab_head; 

/*
 * Global variable used to keep track of the size of the data base record
 * output so far.
 */
long db_sect_size = 0;

/*
 * Global variable for file descriptor for input global data base file.
 */
static int glob_db_desc = -1;
static char *glob_db_name = 0;

/*
 * Buffer containing the function information for the current function
 * being compiled.
 */
unsigned char * f_info_buf;
unsigned f_info_buf_size;
unsigned char * f_info_inline_dec_pos;
unsigned char * f_info_reg_pressure_pos;
unsigned char * f_info_profile_index_pos;
unsigned char * f_info_call_vec_pos;
unsigned char * f_info_rtl_buf;
unsigned f_info_rtl_buf_size;
unsigned char * f_info_ftype;
unsigned char * f_info_call_ptype;

/*
 * Global variable file pointer for output data base file.
 */
#define DB_BUF_SIZE 60
static char db_buf[DB_BUF_SIZE];
static int num_in_buf = 0;
static char hex_buf[] = {'0','1','2','3','4','5','6','7',
                         '8','9','A','B','C','D','E','F'};

static
void
db_flush(is_head)
int is_head;
{
  if (num_in_buf == 0)
    return;

  if (BUILD_DB)
  { /* We don't write if we are only outputting profiling code */
    if (is_head)
      fwrite ("\t.cchead\t", 1, 9, asm_out_file);
    else
      fwrite ("\t.ccinfo\t", 1, 9, asm_out_file);
    fwrite (db_buf, 1, num_in_buf, asm_out_file);
    putc ('\n', asm_out_file);
  }
  num_in_buf = 0;
}

void
cc1_db_write(buf, n)
unsigned char *buf;
int n;
{
  register int l_num_in_buf = num_in_buf;
  register char *b_ptr = db_buf;

  while (n != 0)
  {
    unsigned char c;

    if (l_num_in_buf >= DB_BUF_SIZE-1)
    {
      num_in_buf = l_num_in_buf;
      db_flush (0);
      l_num_in_buf = 0;
    }

    c = *buf;
    switch (c)
    {
      case 0: case 1: case 2: case 3: case 4:
      case 5: case 6: case 7: case 8: case 9:
      /* translated into 'G' - 'P' */
      b_ptr [l_num_in_buf++] = c + 'G';
      break;

      case 0xF6: case 0xF7: case 0xF8: case 0xF9: case 0xFA:
      case 0xFB: case 0xFC: case 0xFD: case 0xFE: case 0xFF:
      /* translated into 'Q' - 'Z' */
      b_ptr [l_num_in_buf++] = (c - 0xF6) + 'Q';
      break;

      case 'a': case 'b': case 'c': case 'd': case 'e':
      case 'f': case 'g': case 'h': case 'i': case 'j':
      case 'k': case 'l': case 'm': case 'n': case 'o':
      case 'p': case 'q': case 'r': case 's': case 't':
      case 'u': case 'v': case 'w': case 'x': case 'y':
      case 'z': case '_': case '.':
      /* These don't have any translation */
      b_ptr [l_num_in_buf++] = c;
      break;

      default:
      /* any others get turned into two byte HEX using upper A-F */
      b_ptr [l_num_in_buf++] = hex_buf[(c >> 4) & 0xF];
      b_ptr [l_num_in_buf++] = hex_buf[(c & 0xF)];
      break;
    }

    n -= 1;
    buf ++;
  }

  num_in_buf = l_num_in_buf;
}

static
void
cc1_db_write_head(buf, n)
unsigned char *buf;
int n;
{
  register int l_num_in_buf;
  register char *b_ptr;

  db_flush(0);  /* flush all the cc info */

  l_num_in_buf = 0;
  b_ptr = db_buf;

  while (n != 0)
  {
    b_ptr [l_num_in_buf++] = hex_buf[(*buf >> 4) & 0xF];
    b_ptr [l_num_in_buf++] = hex_buf[(*buf & 0xF)];

    n -= 1;
    buf ++;
  }

  num_in_buf = l_num_in_buf;
  db_flush(1);
}

static
int
number_call_insns (insn, set_it)
rtx insn;
int set_it;
{
  int num_calls = 0;

  for (; insn != 0; insn = NEXT_INSN(insn))
  {
    if (GET_CODE(insn) == CALL_INSN)
    {
      int val;

      if (INSN_PROFILE_INSTR_P(insn))
        val = -1;
      else
        val = num_calls ++;

      if (set_it)
        INSN_CALL_NUM(insn) = val;
    }
  }

  return (num_calls);
}

static void
do_call_stats (insn, n_calls, vec_buf, prof_index_buf, vec_size_p, prof_size_p)
rtx insn;
int n_calls;
unsigned char *vec_buf;
unsigned char *prof_index_buf;
int *vec_size_p;
int *prof_size_p;
{
  /* this only returns the size of the call vector needed */

  extern int prof_func_block;
  char *fname;
  int l;
  int call_vec_tot_size = 0;
  int prof_tot_size = 0;
  int n_blocks = prof_num_nodes();

  fname = base_file_name;

  /* put out the base file name */
  l = strlen(fname) + 1;

  prof_tot_size += l + 8;

  if (prof_index_buf != 0)
  {
    memcpy(prof_index_buf, fname, l);
    prof_index_buf += l;

    /* put out the function's low basic block */
    CI_U32_TO_BUF(prof_index_buf, prof_func_block);
    prof_index_buf += 4;

    /* put out the function's high basic block */
    CI_U32_TO_BUF(prof_index_buf, (prof_func_block+n_blocks)-1);
    prof_index_buf += 4;
  }

  for (; insn != 0; insn = NEXT_INSN(insn))
  {
    if (GET_RTX_CLASS(GET_CODE(insn)) == 'i' && !INSN_PROFILE_INSTR_P(insn))
    {
      int bb_num = INSN_PROF_DATA_INDEX(insn);
      assert (bb_num < n_blocks);
  
      if (GET_CODE(insn) == CALL_INSN)
      {
        char *name;
        int  size;

        name = get_call_name(PATTERN(insn));

        if (name == 0)
          name = "?";

        size = strlen(name)+1;

        if (vec_buf != 0)
        {
          memcpy(vec_buf, name, size);
          vec_buf += size;
        }

        if (prof_index_buf != 0)
        {
          CI_U32_TO_BUF(prof_index_buf, prof_func_block+bb_num);
          prof_index_buf += 4;
        }

        call_vec_tot_size += size;
        prof_tot_size += 4;
      }
    }
  }

  *vec_size_p = call_vec_tot_size;
  *prof_size_p = prof_tot_size;
}

unsigned char *
db_func_encode_buf(n_parms)
int n_parms;
{
  unsigned char * ret = 0;
  int size;

  if ((BUILD_DB || PROF_CODE) && !flag_find_bname)
  {
    size = 4 /* space for size */ +
           2 /* length of param list encoding */ +
           (n_parms + 1) * 2 /* type encoding */;

    ret = (unsigned char *)obstack_alloc(rtl_obstack, size);
    CI_U32_TO_BUF(ret, size);
  }
  return ret;
}

char *
db_cp_encode_buf(t_old)
char *t_old;
{
  int t;
  unsigned char *old = (unsigned char *)t_old;
  unsigned char *new;

  if (old == 0)
    return 0;

  CI_U32_FM_BUF(old, t);
  new = (unsigned char *)obstack_alloc(rtl_obstack, t);
  memcpy(new, old, t);

  return (char *)new;
}

int
db_encode_type(type)
tree type;
{
  int ret = 0;

  if (type != 0)
  {
    enum ep_type {t_int, t_ptr, t_float, t_aggr};
    enum ep_type ty;
    int size;
    int align;
    int p_align;

    if (TREE_CODE_CLASS(TREE_CODE(type)) != 't')
      abort();

    align = TYPE_ALIGN(type) / BITS_PER_UNIT;
    if (align == 0) align = 1;
    size = int_size_in_bytes(type) / align;

    switch(TREE_CODE(type))
    {
      case INTEGER_TYPE:
      case ENUMERAL_TYPE:
      case VOID_TYPE: 
      case BOOLEAN_TYPE:
      case CHAR_TYPE:
        ty = t_int;

        /* 
         * for integral types round them up to fit into a
         * 'register'
         */
        if (align < UNITS_PER_WORD)
          align = UNITS_PER_WORD;
        if (size < UNITS_PER_WORD)
          size = UNITS_PER_WORD;
        break;

      case POINTER_TYPE:
      case REFERENCE_TYPE:
        ty = t_ptr;
        break;

      case REAL_TYPE:
        ty = t_float;
        break;

      default:
        ty = t_aggr;
    }


    for (p_align = 0; p_align < 8; p_align += 1)
      if ((align & (1 << p_align)) != 0)
        break;

    ret = (ty << 14) |
          ((p_align & 0x7) << 11) |
          (size & 0x7FF);
  }

  return ret;
}

void
db_pack_encode_length(nparms, is_varargs, buf)
int nparms;
int is_varargs;
unsigned char *buf;
{
  if (buf != 0)
  {
    int length;

    length = (nparms & 0x7FFF) | (is_varargs << 15);

    CI_U16_TO_BUF(buf+4, length);
  }
}

void
db_pack_encode_ret(decl, buf)
tree decl;
unsigned char *buf;
{
  if (buf != 0)
  {
    int ptype;

    buf += 4 + 2;  /* where return type sits */

    ptype = db_encode_type(decl);
    CI_U16_TO_BUF(buf, ptype);
  }
}

void
db_pack_encode_parm(decl, pnum, buf)
tree decl;
int pnum;  /* parameter number */
unsigned char * buf;
{
  if (buf != 0)
  {
    int ptype;

    buf += 4 + 2 + 2 + (2 * pnum);  /* where this param sits */
    
    ptype = db_encode_type(decl);
    CI_U16_TO_BUF(buf, ptype);
  }
}

unsigned char *
db_encode_func_type(fdecl, nparms)
tree fdecl;
int nparms;
{
  unsigned char *buf = db_func_encode_buf(nparms);
  tree parms;
  int length;
  int p_num;
  int is_varargs = imstg_func_is_varargs_p(fdecl);

  db_pack_encode_length(nparms, is_varargs, buf);
  db_pack_encode_ret(TREE_TYPE(DECL_RESULT(fdecl)), buf);

  p_num = 0;
  for (parms = DECL_ARGUMENTS (fdecl); parms; parms = TREE_CHAIN (parms))
  {
    db_pack_encode_parm(DECL_ARG_TYPE(parms), p_num, buf);
    p_num += 1;
  }

  return buf;
}

unsigned char *
db_encode_call_ptype(ret_type, parm_val)
tree ret_type;
tree parm_val;
{
  unsigned char *buf;
  int length;
  int nparms;
  tree p;

  /* figure out how many params we have */
  for (p = parm_val, nparms = 0; p; p = TREE_CHAIN (p))
    nparms += 1;

  buf = db_func_encode_buf(nparms);

  db_pack_encode_length(nparms, ret_type == 0, buf);

  if (ret_type == 0)
    ret_type = void_type_node;

  db_pack_encode_ret(ret_type, buf);

  for (p = parm_val, nparms = 0; p; p = TREE_CHAIN (p))
  {
    db_pack_encode_parm(TREE_TYPE (TREE_VALUE (p)), nparms, buf);
    nparms += 1;
  }

  return buf;
}

unsigned char *
db_gather_call_ptype(insn)
rtx insn;
{
  rtx t_insn;
  unsigned char *buf;
  unsigned char *buf_p;
  int size;
  int i;

  /*
   * first iteration of loop figures out size, and allocates buffer.
   * second iteration fills buffer.
   */
  for (i = 0; i < 2; i++)
  {
    size = 4;

    for (t_insn = insn; t_insn != 0; t_insn = NEXT_INSN(t_insn))
    {
      int n;
      if (GET_CODE(t_insn) == CALL_INSN && !INSN_PROFILE_INSTR_P(insn))
      {
        unsigned char *tbuf;

        tbuf = (unsigned char *)INSN_CALL_PTYPE(t_insn);
        assert (tbuf != 0);

        CI_U32_FM_BUF(tbuf, n);
        n -= 4;
        tbuf += 4;

        assert (n != 0);

        size += n;

        if (i == 1)
        {
          memcpy(buf_p, tbuf, n);
          buf_p += n;

          INSN_CALL_PTYPE(t_insn) = 0;
        }
      }
    }

    if (i == 0)
    {
      buf = (unsigned char *)xmalloc(size);
      CI_U32_TO_BUF(buf, size);
      buf_p = buf + 4;
    }
  }

  return buf;
}

void
start_func_info(decl, insns)
tree decl;
rtx  insns;
{
  int rec_size  = 0;
  unsigned char inlinable;
  int num_calls_from;
  int n_insns;
  int n_parms;
  int call_vec_size;
  int prof_vec_size;
  int idec_list_size;
  int regp_list_size;
  int i;

  if (((! TREE_PUBLIC (decl) &&
       ! (TREE_ADDRESSABLE (decl) || TREE_LANG_FLAG_6 (decl))) ||
       DECL_EXTERNAL (decl)) &&
      DECL_INLINE (decl))
    return;

  inlinable = function_globally_inlinable_p(decl) != 0;
  num_calls_from = number_call_insns(insns, 0);

  {
    rtx t_insn;

    /* only count real instructions */
    n_insns = 0;
    for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
      if ((GET_CODE(t_insn) == INSN ||
           GET_CODE(t_insn) == CALL_INSN ||
           GET_CODE(t_insn) == JUMP_INSN) &&
          !INSN_PROFILE_INSTR_P(t_insn))
        n_insns++;
  }

  {
    tree parms;
    n_parms = 0;
    for (parms = DECL_ARGUMENTS (decl); parms; parms = TREE_CHAIN (parms))
      n_parms ++;
  }

  do_call_stats(insns, num_calls_from, (char *)0, (char *)0,
                &call_vec_size, &prof_vec_size);

  /* allocate the buffer */
  /* rec_size if the size of the record less the size of the inlinable rtl */

  /* size of list for inline decision vector */
  idec_list_size = 4 + CI_FDEF_IVECT_SIZE(num_calls_from);

  /* size for list for reg pressure,profile info,call vector */
  regp_list_size = 4 + num_calls_from + prof_vec_size + call_vec_size;

  rec_size = CI_FDEF_REC_FIXED_SIZE + idec_list_size + regp_list_size;

  f_info_buf_size = rec_size;

  f_info_buf = (unsigned char *)xmalloc(rec_size);
  bzero(f_info_buf, rec_size);

  f_info_inline_dec_pos    = f_info_buf + CI_FDEF_REC_FIXED_SIZE + 4;
  CI_U32_TO_BUF(f_info_inline_dec_pos-4, idec_list_size);

  f_info_reg_pressure_pos  = f_info_inline_dec_pos + idec_list_size;
  f_info_profile_index_pos = f_info_reg_pressure_pos + num_calls_from;
  f_info_call_vec_pos      = f_info_profile_index_pos + prof_vec_size;
  CI_U32_TO_BUF(f_info_reg_pressure_pos-4, regp_list_size);

  /* start packing stuff into the buffer */
  CI_U8_TO_BUF(f_info_buf + CI_ADDR_TAKEN_OFF, 0);

  CI_U8_TO_BUF(f_info_buf + CI_FDEF_CAN_DELETE_OFF, 0);

  /* save space for total number of inlinable calls */
  CI_U16_TO_BUF(f_info_buf + CI_FDEF_TOT_INLN_OFF, 0);

  /* number of insns in this function. */
  CI_U16_TO_BUF(f_info_buf + CI_FDEF_NINSN_OFF, n_insns);

  /* number of parms to this function. */
  CI_U16_TO_BUF(f_info_buf + CI_FDEF_NPARM_OFF, n_parms);

  /* number of calls from this function */
  CI_U16_TO_BUF(f_info_buf + CI_FDEF_NCALL_OFF, num_calls_from);

  /* number of opinions about this functions profile */
  CI_U16_TO_BUF(f_info_buf + CI_FDEF_VOTER_OFF, 1);

  /* what we think of this profile info's truthfulness (poor, for now) */
  CI_U16_TO_BUF(f_info_buf + CI_FDEF_PQUAL_OFF, 1);

  do_call_stats (insns, num_calls_from,
                 f_info_call_vec_pos, f_info_profile_index_pos,
                 &call_vec_size, &prof_vec_size);

  if (inlinable)
    f_info_rtl_buf_size = save_for_global_inline (decl, insns, &f_info_rtl_buf);
  else
  {
    f_info_rtl_buf = (unsigned char *) xmalloc (4);
    CI_U32_TO_BUF (f_info_rtl_buf, 4);
    f_info_rtl_buf_size = 4;
  }

  f_info_ftype = db_encode_func_type(decl, n_parms);

  f_info_call_ptype = db_gather_call_ptype(insns);

  /* now actually number the call insns */
  number_call_insns(insns, 1);
}

void
db_func_reg_use()
{
  int i;
  int n_used = 0;

  for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
  {
    if (!fixed_regs[i] && regs_ever_live[i])
      n_used ++;
  }

  CI_U8_TO_BUF(f_info_buf + CI_FDEF_REG_USE_OFF, n_used);
}


static int
dump_usedby_list (symref)
rtx symref;
{
  int ret = 0;
  db_nameref *p;
  char buf[4];
  int n;

  /* Write out the list of functions where this symbol is referenced... */

  /* First, calculate the length ... */
  p = GET_SYMREF_USEDBY(symref);
  n = 4;
  while (p)
  { n += strlen (XSTR(p->fname_sym,0))+1;
    p=p->prev;
  }

  /* Write the length */
  CI_U32_TO_BUF(buf, n);
  cc1_db_write (buf, 4);
  ret += n;

  /* Write the names of the functions */
  p = GET_SYMREF_USEDBY(symref);
  while (p)
  { char *t = XSTR(p->fname_sym,0);
    cc1_db_write (t, strlen(t)+1);
    p=p->prev;
  }
  return ret;
}

static int
dump_bname_list ()
{
  char buf[4];

  int n = strlen (base_file_name) + 1;

  CI_U32_TO_BUF(buf,n+4);
  cc1_db_write (buf,4);
  cc1_db_write (base_file_name, n);

  return n+4;
}

static int sizev [NUM_TS];

static int
dump_ts_info ()
{
  unsigned char buf[4+TS_BUFSIZ];

  memset (buf, 0, sizeof (buf));

  CI_U32_TO_BUF (buf, sizeof (buf));
  db_pack_sizes (buf+4, sizev);
  
  cc1_db_write (buf, sizeof (buf));

  return sizeof (buf);
}

static int
dump_empty_list()
{
  char buf[4];

  CI_U32_TO_BUF (buf, 4);
  cc1_db_write (buf, 4);

  return 4;
}

void
dump_func_info(decl)
tree decl;
{
  if (f_info_buf != 0)
  {
    st_node *st_p;
    int rec_size;
    rtx symref;
    int l;

    /* allocate a stab record for this thing */
    st_p = (st_node *) xmalloc(sizeof(st_node));

    symref = XEXP(DECL_RTL(decl),0);
    st_p->name = XSTR(symref, 0);
    st_p->name_length = strlen(st_p->name);
    st_p->db_rec_offset = db_sect_size;
    st_p->rec_typ = CI_FDEF_REC_TYP;
    st_p->is_static = ((SYMREF_ETC(symref) & SYMREF_STATICBIT) != 0);

    /* put the stab in the list. */
    st_p->next = stab_head;
    stab_head = st_p;

    db_func_reg_use();

    assert (f_info_buf);
    rec_size = f_info_buf_size;
    cc1_db_write (f_info_buf, f_info_buf_size);
    free (f_info_buf);
    f_info_buf = 0;
  
    for (l=CI_FDEF_REGP_LIST+1; l<CI_FDEF_LIST_HI; l++)
      switch (l)
      { int n;
        char buf[4];

        case CI_FDEF_RTL_LIST:			/* RTL for inlining */
          assert (f_info_rtl_buf);
          rec_size += f_info_rtl_buf_size;
          cc1_db_write (f_info_rtl_buf, f_info_rtl_buf_size);
          free (f_info_rtl_buf);
          f_info_rtl_buf = 0;
          break;

        case CI_FDEF_FILE_LIST:			/* bname */
          rec_size += dump_bname_list();
          break;

        case CI_FDEF_TSINFO_LIST:		/* target sz info */
          rec_size += dump_ts_info();
          break;

        case CI_FDEF_FTYPE_LIST:
          /*
           * dump the function type encoding that we created during
           * start_func_info.
           */
          assert (f_info_ftype);
          CI_U32_FM_BUF(f_info_ftype, n);
          rec_size += n;
          cc1_db_write(f_info_ftype, n);
          f_info_ftype = 0;
          break;
 
        case CI_FDEF_CALL_TYPE_LIST:
          /*
           * dump the encodings of the return value and parameter types
           * for all calls made within this function.
           */
          assert (f_info_call_ptype);
          CI_U32_FM_BUF(f_info_call_ptype, n);
          rec_size += n;
          cc1_db_write(f_info_call_ptype, n);
          f_info_call_ptype = 0;
          break;

        default:
          assert (0);
          /* If we come back from assertion, fall thru and make dummy list */

        case CI_FDEF_FUNC_LIST:			/* dummy for FREF list */
        case CI_FDEF_HIST_LIST:			/* dummy for hist info */
          rec_size += dump_empty_list();
          break;

        case CI_FDEF_PROF_LIST:			/* default prof info */
        { char len[4]; int i;

          int n = prof_num_nodes();
          int s = 4 + (n * sizeof (unsigned));

          CI_U32_TO_BUF (len, s);
          cc1_db_write (len, 4);
          rec_size += 4;

          for (i = 0; i < n; i++)
          { char wbuf[4];
            unsigned w = prof_node(i);
            CI_U32_TO_BUF (wbuf, w);
            cc1_db_write (wbuf, 4);
            rec_size += 4;
          }
          prof_free_nodes();
          break;
        }

        case CI_FDEF_CFG_LIST:
        { char len[4]; int i;

          int n = prof_num_arcs();
          int s = 4 + (n * 3 * sizeof (unsigned short));

          CI_U32_TO_BUF (len, s);
          cc1_db_write (len, 4);
          rec_size += 4;

          for (i = 0; i < n; i++)
          {
            char buf [2+2+2]; int to, fm, id;

            prof_arc_info (i, &fm, &to, &id);
            CI_U16_TO_BUF (buf+0, fm);
            CI_U16_TO_BUF (buf+2, to);
            CI_U16_TO_BUF (buf+4, id);
            cc1_db_write (buf, 6);
            rec_size += 6;
          }
          prof_free_arcs();
          break;
        }
      }

    st_p->db_rec_size = rec_size;
    db_sect_size     += rec_size;
  }
}

st_node*
new_st_node (name)
char* name;
{
  st_node *st_p;

  /* allocate a stab record for this thing */
  st_p = (st_node *) xmalloc(sizeof(st_node));
  bzero (st_p, sizeof (st_node));

  st_p->name = name;
  st_p->name_length = strlen(st_p->name);
  st_p->db_rec_offset = db_sect_size;

  /* put the stab in the list. */
  st_p->next = stab_head;
  stab_head = st_p;

  return st_p;
}

/* For recording the switches which need to be put in the database for
   the assembler, for recompiling subst files.  */

enum as_switches { as_asm, as_arch, as_be, as_pc, as_pd, as_lim };
static char *asm_argv[((int)as_lim)] = { "gas960" };

char* db_argv;
extern int save_argc;
extern char **save_argv;

void
note_target_switch (argc)
int argc;
{ /* Process -m switch, and remember those things which affect the 2nd pass
     compiler and assembler invocation.

     We assume that all -m switches should be kept for 2nd pass compiles.
     For the assembler, we process onlyt the architecture switch here,
     because the rest of the switches are easily gleaned from flags,
     e.g, flag_coff. */

  assert (argc>0 && argc<save_argc && db_argv && save_argv && save_argv[argc]);

  { char* s = save_argv[argc];

    assert (s && (s[0]=='-' || s[0]=='/') && s[1]=='m');

    db_argv[argc] = 1;	/* For now, we hang on to all -m switches in db */

    s += 2;
    if (strlen(s)==2 && index("chkjs", s[0]) && index("abdft", s[1]))
    { static char p[5];
  
      p[0] = '-';
      p[1] = 'A';
      p[2] = (s[0] - 'a') + 'A';
      p[3] = (s[1] - 'a') + 'A';
  
      asm_argv[(int)as_arch] = p;
    }
  }
}

void
dump_src_info ()
{
  /* Dump various information about the input file and the
     compilation switches into the database records. */

  int lnum = CI_SRC_LIST_LO-1;	/* Current list, starting with fixed part */
  int siz  = 0;			/* Total size written */

  extern int flag_elf, flag_coff;

  if (flag_coff)
    asm_argv[0] = "gas960c";
  else if (flag_elf)
    asm_argv[0] = "gas960e";
  else
    asm_argv[0] = "gas960";

  if (TARGET_PIC)
    asm_argv[(int)as_pc] = "-pc";

  if (TARGET_PID)
    asm_argv[(int)as_pd] = "-pd";

  if (bytes_big_endian)
    asm_argv[(int)as_be] = "-G";

  while (lnum < CI_SRC_LIST_HI)
  { static unsigned char empty[4] = { 4 };

    unsigned char* buf;
    int len;

    /* On each iteration, collect list to be written into buf,
       write it, update total written, and free buf. */

    switch (lnum)
    {
      default:
        assert (0);

      case CI_SRC_LIB_LIST:		/* Empty list for archive name */
      case CI_SRC_MEM_LIST:		/* Empty list for member name */
      case CI_SRC_SUBST_LIST:		/* Empty list for subst history */
        len = 4;
        buf = empty;
        break;


      case CI_SRC_LIST_LO-1:		/* Fixed part of record */
      { char* lang = lang_identify();

        len = CI_SRC_REC_FIXED_SIZE;
        buf = (unsigned char *) xmalloc (len);
        memset (buf, 0, len);

        if (!strcmp (lang, "c"))
          CI_U8_TO_BUF (buf + CI_SRC_REC_LANG_OFF, CI_LANG_C);
        else if (!strcmp (lang, "cplusplus"))
          CI_U8_TO_BUF (buf + CI_SRC_REC_LANG_OFF, CI_LANG_CPLUS);
        else
          assert (0);

        break;
      }

      case CI_SRC_SRC_LIST:		/* src code */
      { extern int in_file_size;
        extern char *input_filename;
        extern FILE* finput;

        if (in_file_size && BUILD_SRC_DB)
        {
          int tell = ftell (finput);
          int ok = 0;

          buf = (unsigned char*) xmalloc (len = 4 + in_file_size);
          CI_U32_TO_BUF (buf, len);
  
          if (fseek (finput,0L,0) == 0)
          {
            /* Need to use an fgetc loop here, rather than one big fread,
               because MetaWare's fread fails on DOS text files exceeding 1K. */

            int c;
            unsigned char *t = buf + 4;
  
            while ((c = fgetc(finput)) != EOF)
              *t++ = c;
  
            if (ok = ((t-buf) == len && fseek(finput, tell, 0) == 0))
              len = src_compress (&buf);
          }
      
          if (!ok)
            internal_error ("could not reread %s during ccinfo generation",
                          input_filename);
        }
        break;
      }

      case CI_SRC_CC1_LIST:		/* Set up list for cmd line */
        if (BUILD_SRC_DB)
        { static char* cc1_extra[] = { "-quiet", "-w" };
          int i; unsigned char *t, *argc;

          len = 5;

          for (i = 1; i < save_argc; i++)
            if (db_argv[i])
              len += strlen (save_argv[i]) + 1;

          for (i = 0; i < sizeof(cc1_extra)/sizeof(cc1_extra[0]); i++)
            len += strlen (cc1_extra[i]) + 1;

          t = (buf = (unsigned char *) xmalloc (len)) + 4;
          CI_U32_TO_BUF (buf, len);
          *(argc = t++) = 0;
  
          /* Fill in cc1 command */
          for (i = 1; i < save_argc; i++)
            if (db_argv[i])
            { int n = strlen (save_argv[i]) + 1;
              int c = *(save_argv[i]);

#ifdef DOS
              if (c=='/')
                c = '-';
#endif
              *t = c;

              if (n > 1)
                memcpy (t+1, save_argv[i]+1, n-1);

              t += n;
              (*argc)++;
            }
  
          for (i = 0; i < sizeof(cc1_extra)/sizeof(cc1_extra[0]); i++)
          { int n = strlen (cc1_extra[i]) + 1;
            memcpy (t, cc1_extra[i], n);
            t += n;
            (*argc)++;
          }
        }
        break;

      case CI_SRC_ASM_LIST:		/* Set up cmd line for asm */
        if (BUILD_SRC_DB)
        { int i; unsigned char *t, *argc;

          len = 5;

          for (i = 0; i < (int)as_lim; i++)
            if (asm_argv[i])
              len += strlen (asm_argv[i]) + 1;

          t = (buf = (unsigned char *) xmalloc (len)) + 4;
          CI_U32_TO_BUF (buf, len);
          *(argc = t++) = 0;
  
          /* Fill in asm command */
          for (i = 0; i < (int)as_lim; i++)
            if (asm_argv[i])
            { int n = strlen (asm_argv[i]) + 1;
              memcpy (t, asm_argv[i], n);
              t += n;
              (*argc)++;
            }
        }
        break;
    }

    cc1_db_write (buf, len);
    siz += len;

    if (buf != empty)
      free (buf);

    lnum++;
  }

  { int n = strlen (base_file_name) + 2;
    char* nam = (char *) xmalloc (n);
    st_node* st_p;

    nam[0]='1';
    memcpy(nam+1, base_file_name, n-1);

    st_p = new_st_node (nam);

    st_p->is_static = 0;
    st_p->rec_typ = CI_SRC_REC_TYP;
    st_p->db_rec_size = siz;
  }

  db_sect_size += siz;
}


static int
dump_vdef_info (symref)
rtx symref;
{
  char buf[CI_VDEF_REC_FIXED_SIZE];
  int  off,l;

  for (off=0; off<sizeof(buf); )
    switch (off)
    { default:
        assert (0);
        CI_U8_TO_BUF(buf+off, 0);
        off += 1;
        break;

      case CI_ADDR_TAKEN_OFF:
        CI_U8_TO_BUF(buf+off, SYMREF_ADDRTAKEN(symref));
        off += 1;

      case CI_VDEF_SIZE_OFF:
        /* put out the variables size */
        CI_U32_TO_BUF(buf+off, SYMREF_SIZE(symref));
        off += 4;

      case CI_VDEF_USAGE_OFF:
        CI_U32_TO_BUF(buf+off, SYMREF_USECNT(symref));
        off += 4;
        break;

      case CI_VDEF_SRAM_ADDR_OFF:
        /* reserve space for SRAM allocation decision, initialize it to 0 */
        CI_U32_TO_BUF(buf+off, 0);
        off += 4;
        break;
    }

  assert (off == sizeof(buf));
  cc1_db_write (buf, off);

  for (l=CI_VDEF_LIST_LO; l < CI_VDEF_LIST_HI; l++)
    switch (l)
    { default:
        assert(0);
        off += dump_empty_list ();
        break;

      case CI_VDEF_FILE_LIST:
        off += dump_bname_list ();
        break;

      case CI_VDEF_FUNC_LIST:
        off += dump_usedby_list (symref);
        break;
    }

  return off;
}

static int
dump_vref_info (symref)
rtx symref;
{
  char buf[CI_VREF_REC_FIXED_SIZE];
  int  off, l;

  for (off=0; off < sizeof(buf); )
    switch (off)
    { default:
        assert (0);
        CI_U8_TO_BUF(buf+off, 0);
        off += 1;
        break;

      case CI_ADDR_TAKEN_OFF:
        CI_U8_TO_BUF(buf+off, SYMREF_ADDRTAKEN(symref));
        off += 1;

      case CI_VREF_USAGE_OFF:
        CI_U32_TO_BUF(buf+off, SYMREF_USECNT(symref));
        off += 4;
        break;
    }

  assert (off == sizeof(buf));
  cc1_db_write (buf, off);

  for (l=CI_VREF_LIST_LO; l < CI_VREF_LIST_HI; l++)
    switch (l)
    { default:
        assert(0);
        off += dump_empty_list ();
        break;

      case CI_VREF_FUNC_LIST:
        off += dump_usedby_list (symref);
        break;
    }

  return off;
}

static int
dump_fref_info (symref)
rtx symref;
{
  char buf[CI_FREF_REC_FIXED_SIZE];
  int  off, l;

  for (off=0; off < sizeof(buf); )
    switch (off)
    { default:
        assert (0);
        CI_U8_TO_BUF(buf+off, 0);
        off += 1;
        break;

      case CI_ADDR_TAKEN_OFF:
        CI_U8_TO_BUF(buf+off, SYMREF_ADDRTAKEN(symref));
        off += 1;
        break;
    }

  assert (off == sizeof (buf));
  cc1_db_write (buf, off);

  for (l=CI_FREF_LIST_LO; l < CI_FREF_LIST_HI; l++)
    switch (l)
    { default:
        assert(0);
        off += dump_empty_list ();
        break;

      case CI_FREF_FUNC_LIST:
        off += dump_usedby_list (symref);
        break;
    }

  return off;
}

void
dump_symref_info (symref)
rtx symref;
{
  char* name = XSTR(symref,0);
  int   typ  = 0;

  /* Keep __FUNCTION__ and __PRETTY_FUNCTION__ from polluting the data base.  */
  if (name == 0 ||
      strncmp(name, "__FUNCTION__", 12) == 0 ||
      strncmp(name, "__PRETTY_FUNCTION__", 19) == 0)
    return;

  if ((SYMREF_ETC(symref) & SYMREF_VARBIT) != 0)
    if ((SYMREF_ETC(symref) & SYMREF_DEFINED) != 0)
      typ = CI_VDEF_REC_TYP;
    else
      typ = CI_VREF_REC_TYP;
  else
    if ((SYMREF_ETC(symref) & SYMREF_FUNCBIT) != 0)
      /* only put out CI_FREF_REC if function address was taken. */
      if (SYMREF_ADDRTAKEN(symref))
        typ = CI_FREF_REC_TYP;

  if (typ)
  { st_node* p = new_st_node (name);

    p->rec_typ = typ;
    p->is_static = ((SYMREF_ETC(symref) & SYMREF_STATICBIT) != 0);

    switch (typ)
    { case CI_VDEF_REC_TYP: p->db_rec_size = dump_vdef_info (symref); break;
      case CI_VREF_REC_TYP: p->db_rec_size = dump_vref_info (symref); break;
      case CI_FREF_REC_TYP: p->db_rec_size = dump_fref_info (symref); break;
    }

    db_sect_size += p->db_rec_size;
  }
}

static int n_prof_lines;

/*
 * This saves the next block prof info for output later.
 */
void
save_next_blk_prof_info(bs_rtx, be_rtx, form, n_counters, func_cnt_base)
rtx bs_rtx; /* block start rtx - first insn in this basic block */
rtx be_rtx; /* block end rtx   - last insn is this basic block */
prof_formula_type form;
int n_counters;
int func_cnt_base;
{
  int i;
  int last_lineno = -1;
  rtx t_insn;

  /* pack all the line numbers into the basic block */
  for (t_insn = bs_rtx; t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    enum rtx_code c = GET_CODE(t_insn);
   
    if (c == CODE_LABEL && t_insn != be_rtx)
    { /* If there is code in the block, forget the label. */

      rtx t = NEXT_INSN(t_insn);

      while (t != 0 && t != be_rtx && GET_CODE(t) == NOTE)
        t = NEXT_INSN(t);

      if (t!=0 && ((c=GET_CODE(t)) == CALL_INSN || c==INSN || c==JUMP_INSN))
        t_insn = t;
      else
        c = CODE_LABEL;
    }

    if (c == CALL_INSN || c == INSN || c == JUMP_INSN || c == CODE_LABEL)
    {
      /*
       * line numbers are stored in the INSN_PROF_DATA_INDEX field of
       * the INSN at this point.  Its a hack, but best I could think of.
       */
      int cur_lineno = INSN_PROF_DATA_INDEX(t_insn);
      if (cur_lineno != -1)
      {
        if (cur_lineno != last_lineno)
        {
          pk_num(cur_lineno);
          n_prof_lines += 1;
          last_lineno = cur_lineno;
        }
      }
    }

    if (t_insn == be_rtx)
      break;
  }

  pk_num(0);  /* line number list terminator */
  
  /* now pack the formula into the record. */
  for (i = 0; i < n_counters; i++)
  {
    switch (form[i])
    {
      case COEF_POS:
        pk_num(func_cnt_base+i+1);
        break;
      case COEF_NEG:
        pk_num(-(func_cnt_base+i+1));
        break;
      case COEF_ZERO:
        break;
      default:
        abort();
    }
  }

  /* pack terminator for formula */
  pk_num(0);
}

static void
save_symbol_refs_usage_info(x)
rtx x;
{
  register char *name;
  register char *fmt;
  register int i;
  enum rtx_code code;

  if (x == 0)
    return;

  switch (code = GET_CODE(x))
  {
    case SYMBOL_REF:
      if ((SYMREF_ETC(x) & SYMREF_VARBIT) != 0)
      {
        name = XSTR(x,0);
        while (*name != 0)
        {
          PUTBYTE(*name);
          name += 1;
        }
        PUTBYTE(0);
      }
      break;

    default:
      fmt = GET_RTX_FORMAT (code);
      i   = GET_RTX_LENGTH (code);

      while (--i >= 0)
      {
        if (fmt[i] == 'e')
          save_symbol_refs_usage_info(XEXP(x,i));
        else if (fmt[i] == 'E')
        {
          register int j = XVECLEN(x,i);
          while (--j >= 0)
            save_symbol_refs_usage_info(XVECEXP(x,i,j));
        }
      }
      break;
  }
}

void
save_var_usage_info(bs_rtx, be_rtx)
rtx bs_rtx; /* block start rtx - first insn in this basic block */
rtx be_rtx; /* block end rtx   - last insn is this basic block */
{
  rtx t_insn;

  /* pack symbol references referenced by the basic block */
  for (t_insn = bs_rtx; t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    if (GET_RTX_CLASS(GET_CODE(t_insn)) == 'i')
      save_symbol_refs_usage_info(PATTERN(t_insn));

    if (t_insn == be_rtx)
      break;
  }
  PUTBYTE(0);  /* terminator */
}

void
dump_prof_info(module_name, file_name)
char *module_name;
char *file_name;
{
  int rsize;
  struct blk_prof_info_st *tptr;

  rsize = b_prof_info.n_used;
  for (tptr = b_prof_info.next; tptr != 0; tptr = tptr->next)
    rsize += tptr->n_used;

  if (rsize != 0)
  {
    int fname_len;
    int var_size;

    { /* Write out the fixed part of the buffer */
      unsigned char buf[CI_PROF_REC_FIXED_SIZE];

      memset (buf, 0, sizeof (buf));

      CI_U32_TO_BUF(buf+CI_PROF_NBLK_OFF, prof_n_bblocks);
      CI_U32_TO_BUF(buf+CI_PROF_NCNT_OFF, prof_n_counters);
      CI_U32_TO_BUF(buf+CI_PROF_NLINES_OFF, n_prof_lines);
      CI_U32_TO_BUF(buf+CI_PROF_SIZE_OFF, rsize);

      cc1_db_write (buf, sizeof(buf));
    }


    /*
     * we need to get the base file name, so if there is any path info
     * strip it off.
     */
    {
      char *nfname;
#if defined(DOS)
      if (*file_name && file_name[1] == ':')	/* Skip over drive name */
	file_name += 2;
      for (nfname = file_name; *nfname; nfname++)
	if (*nfname == '/' || *nfname == '\\')
          file_name = nfname + 1;
#else
      nfname = rindex(file_name, '/');
      if (nfname != 0)
        file_name = nfname + 1;
#endif
    }

    fname_len = strlen(file_name) + 1;
    var_size  = 4 + rsize + fname_len;

    { /* Allocate space for the variable part of the profile record */

      unsigned char *buf = (unsigned char*) xmalloc (var_size);
      unsigned char* p = buf;
  
      /* Set the size into place */
      CI_U32_TO_BUF (p, var_size);
      p += 4;
  
      /* Set the file name into place */
      memcpy (p, file_name, fname_len);
      p += fname_len;
  
      /* Set the profile record into place */
      memcpy (p, b_prof_info.buf, b_prof_info.n_used);
      p += b_prof_info.n_used;
  
      for (tptr = b_prof_info.next; tptr != 0; tptr = tptr->next)
      { memcpy (p, tptr->buf, tptr->n_used);
        p += tptr->n_used;
      }
  
      /* Now compress the record and write it out */
      { extern int db_compress_prof_rec;

        if (db_compress_prof_rec)
          var_size = src_compress (&buf);
      }

      cc1_db_write(buf, var_size);
      free (buf);
    }

    {
      st_node *st_p = (st_node *) xmalloc(sizeof(st_node));
      st_p->db_rec_size = CI_PROF_REC_FIXED_SIZE + var_size;
      st_p->name = module_name;
      st_p->name_length = strlen(module_name);
      st_p->db_rec_offset = db_sect_size;
      st_p->rec_typ = CI_PROF_REC_TYP;
      st_p->is_static = 0;

      /* put the stab in the list. */
      st_p->next = stab_head;
      stab_head = st_p;

      db_sect_size += st_p->db_rec_size;
    }
  }
}

dump_call_graph_info(func_name, insns)
char *func_name;
rtx insns;
{
  rtx t_insn;
  st_node *st_p;
  char *p;
  unsigned char buf[4];
  int rec_size = 0;
  unsigned long t;

  st_p = (st_node *) xmalloc(sizeof(st_node)+strlen(func_name)+4);
  p = ((char *)st_p) + sizeof(st_node);
  sprintf (p, "%s%%cg", func_name);
  st_p->name = p;
  st_p->name_length = strlen(p);
  st_p->db_rec_offset = db_sect_size;
  st_p->rec_typ = CI_CG_REC_TYP;
  st_p->is_static = 0;

  /* put the stab in the list. */
  st_p->next = stab_head;
  stab_head = st_p;

  buf[0] = prof_func_used_prof_info;
  cc1_db_write(buf, 1);
  rec_size += 1;

  for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    if (GET_CODE(t_insn) == NOTE &&
#if !defined(DEV_465)
        NOTE_LINE_NUMBER(t_insn) == NOTE_INSN_FUNCTION_ENTRY)
#else
        NOTE_LINE_NUMBER(t_insn) == NOTE_INSN_FUNCTION_BEG)
#endif
    {
      t = NOTE_CALL_COUNT(t_insn);
      CI_U32_TO_BUF(buf, t);
      cc1_db_write(buf, 4);
      rec_size += 4;

      break;
    }
  }

  if (rec_size != 5)
#if !defined(DEV_465)
    fatal("compiler error - NOTE_INSN_FUNCTION_ENTRY missing or duplicated.");
#else
    fatal("compiler error - NOTE_INSN_FUNCTION_BEG missing or duplicated.");
#endif

  for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    if (GET_CODE(t_insn) == CALL_INSN)
    {
      char *name = get_call_name(PATTERN(t_insn));
      int nm_sz;
     
      if (name == 0)
        name = "?";

      t = INSN_EXPECT(t_insn) / PROB_BASE;

      nm_sz = strlen(name)+1;
      cc1_db_write(name, nm_sz);

      CI_U32_TO_BUF(buf, t);
      cc1_db_write(buf, 4);

      rec_size += nm_sz + 4;
    }
  }

  buf[0] = '\0';
  cc1_db_write(buf, 1);
  rec_size += 1;

  st_p->db_rec_size = rec_size;
  db_sect_size += st_p->db_rec_size;
}

void
end_out_db_info()
{
  /*
   * all the data base records have been written out.
   * now write out the symbol table and string table, and then
   * backpatch the header.
   */
  st_node * list_p = stab_head;
  long str_tab_size = 0;
  long sym_tab_size = 0;

  if (list_p == 0)
  {
    /* no cc_info for the whole file, just return. */
    return;
  }

  /*
   * cruise though the list assigning the string table offsets,
   * and putting the symbol table to the file.
   */
  for (list_p = stab_head; list_p != 0; list_p = list_p->next)
  {
    int hash_val;
    unsigned char sym_buf[CI_STAB_REC_SIZE];

    /* assign the string table offset */
    CI_U32_TO_BUF(sym_buf + CI_STAB_STROFF_OFF, str_tab_size);
    CI_U32_TO_BUF(sym_buf + CI_STAB_DBOFF_OFF, list_p->db_rec_offset);
    CI_U32_TO_BUF(sym_buf + CI_STAB_DBRSZ_OFF, list_p->db_rec_size);
    CI_SYM_HASH(hash_val, list_p->name);
    CI_U16_TO_BUF(sym_buf + CI_STAB_HASH_OFF, hash_val);
    CI_U8_TO_BUF(sym_buf + CI_STAB_STATIC_OFF, list_p->is_static);
    CI_U8_TO_BUF(sym_buf + CI_STAB_RECTYP_OFF, list_p->rec_typ);

    /* write the record out */
    cc1_db_write(sym_buf, CI_STAB_REC_SIZE);

    sym_tab_size += CI_STAB_REC_SIZE;
    str_tab_size += list_p->name_length+1;
  }

  /* now put the string table out */
  for (list_p = stab_head; list_p != 0; list_p = list_p->next)
    cc1_db_write(list_p->name, list_p->name_length+1);

  /* now backpatch the header */
  {
    unsigned char header_buf[CI_HEAD_REC_SIZE];
    unsigned long tot_size;

    bzero(header_buf, sizeof(header_buf));

    tot_size = db_sect_size + sym_tab_size + str_tab_size + CI_HEAD_REC_SIZE;

    CI_U16_TO_BUF(header_buf + CI_HEAD_MAJOR_OFF, CI_MAJOR | CI_MAJOR_MASK);
    CI_U8_TO_BUF(header_buf + CI_HEAD_MINOR_OFF, CI_MINOR);
    CI_U8_TO_BUF(header_buf + CI_HEAD_KIND_OFF, CI_CC1_DB);
    CI_U32_TO_BUF(header_buf + CI_HEAD_TOT_SIZE_OFF, tot_size);
    CI_U32_TO_BUF(header_buf + CI_HEAD_DBSIZE_OFF, db_sect_size);
    CI_U32_TO_BUF(header_buf + CI_HEAD_SYMSIZE_OFF, sym_tab_size);
    CI_U32_TO_BUF(header_buf + CI_HEAD_STRSIZE_OFF, str_tab_size);

    cc1_db_write_head (header_buf, CI_HEAD_REC_SIZE);
  }
}

static
void
read_db_rec(db_p)
st_node *db_p;
{
  db_p->db_rec = (unsigned char *)xmalloc(db_p->db_rec_size);

  /*
   * seek to the correct place in the file for reading the record.
   */
  if (lseek(glob_db_desc, db_p->db_rec_offset, 0) < 0)
    fatal("IO error during lseek on 2nd pass data base file %s.", in_db_name);

  /*
   * read the record into the space.
   */
  if (read(glob_db_desc, db_p->db_rec, db_p->db_rec_size) != db_p->db_rec_size)
    fatal("IO error during read on 2nd pass data base file %s.", in_db_name);
}

void
cache_db_rec(db_p)
st_node *db_p;
{
  struct cache_rec *t;

  if (db_p->db_rec != 0)
  {
    /*
     * take this item out of its spot in the list and
     * put it back at the tail.
     */
    struct cache_rec *n;

    t = get_cache_slot(db_p);
    n = t->next_cache_slot;

    /*
     * take it out of the list, and put it back at the end,
     * note that if its next field is zero then it is already at the
     * tail and we don't need to bother with this. */
    if (n != 0)
    {
      struct cache_rec *p = t->prev_cache_slot;

      n->prev_cache_slot = p;
      if (p == 0)
        st_cache_head = n;
      else
        p->next_cache_slot = n;

      /* put it back in at the tail */
      t->next_cache_slot = 0;
      t->prev_cache_slot = st_cache_tail;
      st_cache_tail->next_cache_slot = t;
      st_cache_tail = t;
    }
    return;
  }

  /*
   * its not already in the cache, so we have to read it in.
   * throw the entry in the queue at head out, make it be the new
   * record read in and put it back in the list at tail.
   */
  t = st_cache_head;
  st_cache_head = t->next_cache_slot;
  st_cache_head->prev_cache_slot = 0;

  if (t->st_p != 0)
  {
    free(t->st_p->db_rec);
    t->st_p->db_rec = 0;
    set_cache_slot (t->st_p, 0);
  }

  /* put the new entry at the tail */
  t->next_cache_slot = 0;
  t->prev_cache_slot = st_cache_tail;
  st_cache_tail->next_cache_slot = t;
  st_cache_tail = t;

  /*
   * read the new cache entry in.
   */
  t->st_p = db_p;
  set_cache_slot (db_p, t);
  read_db_rec(db_p);
}

static
st_node *
find_db_rec(name)
char *name;
{
  int hash;
  st_node *list;

  CI_SYM_HASH(hash, name);

  list = db_stab[hash];

  while (list != 0)
  {
    if (strcmp(list->name, name) == 0)
      return (list);
    list = list->next;
  }

  return 0;
}

void
init_glob_db()
{
  int i;

  for (i = 0; i < CI_HASH_SZ; i++)
    db_stab[i] = 0;

  for (i = 0; i < ST_CACHE_SZ; i++)
  {
    st_cache[i].st_p = 0;
    st_cache[i].next_cache_slot = &st_cache[i+1];
    st_cache[i].prev_cache_slot = &st_cache[i-1];
  }
  st_cache_head = &st_cache[0];
  st_cache_head->prev_cache_slot = 0;

  st_cache_tail = &st_cache[ST_CACHE_SZ-1];
  st_cache_tail->next_cache_slot = 0;
}

static char* pdb_dir;

set_pdb_dir (p)
char* p;
{
  pdb_dir = xmalloc (strlen(p) + 1);
  strcpy (pdb_dir, p);
}

void
open_in_glob_db()
{
  assert (in_db_name == 0);

  if (pdb_dir)
    db_note_pdb (pdb_dir);

  /* We don't come back if the pdb wasn't specified. */
  db_pdb_file (&in_db_name, "pass2.db");

#ifndef DOS
  glob_db_desc = open(in_db_name, O_RDONLY, 0);
#else
  (void) normalize_file_name(in_db_name);
  glob_db_desc = open(in_db_name, (O_RDONLY|O_BINARY), 0);
#endif

  if (glob_db_desc < 0)
    fatal("cannot open 2nd pass data base file %s.", in_db_name);
}

int have_in_glob_db()
{
  return (glob_db_desc >= 0);
}

static st_node *st_nodes;
static int num_stabs;

void
output_lomem(outfile)
FILE *outfile;
{
  int i;
  st_node *db_p;

  for (i = 0; i < num_stabs; i++)
  {
    st_node *db_p = &st_nodes[i];
    if (db_p->rec_typ == CI_VDEF_REC_TYP)
    {
      unsigned long sram_addr;

      cache_db_rec(db_p);

      CI_U32_FM_BUF(db_p->db_rec + CI_VDEF_SRAM_ADDR_OFF, sram_addr);

      if (sram_addr > 0 && sram_addr < 4096)
        fprintf (outfile, "\t.lomem\t_%s\n", db_p->name);
    }
  }
}

void
build_in_glob_db()
{
  unsigned char hbuf[CI_HEAD_REC_SIZE];
  unsigned char *strtab;
  unsigned char *symtab;
  unsigned char *stab_ptr;
  int i;
  ci_head_rec head;

  /* read in the header */
  if (read(glob_db_desc, hbuf, CI_HEAD_REC_SIZE) != CI_HEAD_REC_SIZE)
    fatal("IO error during read on 2nd pass data base file %s.", in_db_name);

  db_exam_head (hbuf, in_db_name, &head);

  num_stabs = head.sym_size/CI_STAB_REC_SIZE;
  /*
   * allocate the string table, and the appropriate number
   * of symbol table nodes.
   */
  strtab = (unsigned char *)xmalloc(head.str_size);
  st_nodes = (st_node *)xmalloc(num_stabs * sizeof(st_node));
  symtab = (unsigned char *)xmalloc(num_stabs * CI_STAB_REC_SIZE);
  stab_ptr = symtab;

  /* seek to the symbol table and read it in */
  if (lseek(glob_db_desc, head.db_size + CI_HEAD_REC_SIZE, 0) < 0)
    fatal("IO error during lseek on 2nd pass data base file %s.", in_db_name);

  if (read(glob_db_desc, symtab, head.sym_size) != head.sym_size)
    fatal("IO error during read on 2nd pass data base file %s.", in_db_name);

  if (read(glob_db_desc, strtab, head.str_size) != head.str_size)
    fatal("IO error during read on 2nd pass data base file %s.", in_db_name);

  for (i = 0; i < num_stabs; i++)
  {
    int val;
    st_node *db_p = &st_nodes[i];

    db_p->db_rec        = 0;

    CI_U32_FM_BUF(stab_ptr + CI_STAB_STROFF_OFF, val);
    db_p->name = (char *)strtab + val;

    CI_U32_FM_BUF(stab_ptr + CI_STAB_DBOFF_OFF, db_p->db_rec_offset);
    db_p->db_rec_offset += CI_HEAD_REC_SIZE;

    CI_U32_FM_BUF(stab_ptr + CI_STAB_DBRSZ_OFF, db_p->db_rec_size);

    CI_U16_FM_BUF(stab_ptr + CI_STAB_HASH_OFF, val);

    CI_U8_FM_BUF(stab_ptr + CI_STAB_STATIC_OFF, db_p->is_static);

    CI_U8_FM_BUF(stab_ptr + CI_STAB_RECTYP_OFF, db_p->rec_typ);

    /* add it into the data base */
    db_p->next = db_stab[val];
    db_stab[val] = db_p;

    stab_ptr += CI_STAB_REC_SIZE;
  }

  /* don't need the symtab space any more, get rid of it */
  free (symtab);
}

/*
 * returns 0 for false, 1 for true, -1 for unknown.
 */
int
glob_addr_taken_p(name)
char *name;
{
  st_node *db_p;
  int ret_val;

  if (!flag_glob_alias)
    return -1;

  db_p = find_db_rec(name);

  if (db_p == 0 ||
      (db_p->rec_typ != CI_VDEF_REC_TYP &&
       db_p->rec_typ != CI_LIB_VDEF_REC_TYP &&
       db_p->rec_typ != CI_VREF_REC_TYP))
    return -1;

  cache_db_rec(db_p);

  ret_val = db_rec_addr_taken (db_p);

  return (ret_val);
}

int
glob_sram_addr(name)
char *name;
{
  st_node *db_p;
  int ret_val;

  if (!flag_glob_sram)
    return 0;

  db_p = find_db_rec(name);

  if (db_p == 0 || db_p->rec_typ != CI_VDEF_REC_TYP)
    return 0;

  cache_db_rec(db_p);

  CI_U32_FM_BUF(db_p->db_rec + CI_VDEF_SRAM_ADDR_OFF, ret_val);

  return (ret_val);
}

char *
glob_inln_info(name)
char *name;
{
  st_node *db_p;
  int ret_val;

  if (!flag_glob_inline)
    return 0;

  db_p = find_db_rec(name);

  if (db_p == 0 ||
      (db_p->rec_typ != CI_FDEF_REC_TYP &&
       db_p->rec_typ != CI_LIB_FDEF_REC_TYP))
    return 0;

  cache_db_rec(db_p);

  return ((char *)db_p);
}

int
glob_inln_deletable(t)
char *t;
{
  st_node *db_p = (st_node *)t;
  int ret_val;

  if (db_p->db_rec == 0)
    cache_db_rec(db_p);

  ret_val = db_fdef_can_delete (db_p);

  return ret_val;
}

int
glob_inln_tot_inline(t)
char *t;
{
  st_node *db_p = (st_node *)t;
  int ret_val;

  if (db_p->db_rec == 0)
    cache_db_rec(db_p);

  CI_U16_FM_BUF(db_p->db_rec + CI_FDEF_TOT_INLN_OFF, ret_val);

  return ret_val;
}

unsigned char *
glob_inln_vect(t)
char *t;
{
  st_node *db_p = (st_node *)t;

  if (db_p->db_rec == 0)
    cache_db_rec(db_p);

  return CI_LIST_TEXT (db_p, CI_FDEF_IVEC_LIST);
}

int
glob_have_prof_counts(t)
char* t;
{
  st_node *db_p = (st_node *)t;

  if (db_p->db_rec == 0)
    cache_db_rec(db_p);

  return db_fdef_has_prof (db_p);
}

unsigned
glob_prof_counter(t, counter)
char *t;
int counter;
{
  st_node *db_p = (st_node *)t;
  unsigned hi, lo, ret;

  if (db_p->db_rec == 0)
    cache_db_rec(db_p);

  ret = db_fdef_prof_counter (db_p, counter);
  return ret;
}

unsigned char *
glob_reg_pressure_vect(t)
unsigned char* t;
{
  st_node *db_p = (st_node *)t;

  if (db_p->db_rec == 0)
    cache_db_rec(db_p);

  return CI_LIST_TEXT (db_p, CI_FDEF_REGP_LIST);
}

unsigned char *
glob_inln_rtl_buf(t)
unsigned char* t;
{
  st_node *db_p = (st_node *)t;

  if (db_p->db_rec == 0)
    cache_db_rec(db_p);

  return CI_LIST_TEXT (db_p, CI_FDEF_RTL_LIST) - 4;
}

unsigned char *
glob_inln_prof_vect(t)
char *t;
{
  st_node *db_p = (st_node *)t;
  int n_calls;

  if (db_p->db_rec == 0)
    cache_db_rec(db_p);

  CI_U16_FM_BUF(db_p->db_rec + CI_FDEF_NCALL_OFF, n_calls);

  return glob_reg_pressure_vect(t) + n_calls;
}

int
glob_inln_rtl_size(t)
char *t;
{
  st_node *db_p = (st_node *)t;
  if (db_p->db_rec == 0)
    cache_db_rec(db_p);

  return db_rec_rtl_size (db_p);
}

int in_file_size;
int in_file_offset;

#ifdef DOS
#define RB "rb"
#define WB "wb"
#else
#define RB "r"
#define WB "w"
#endif

int flag_is_subst_compile;

void
check_db_file (pargc, pargv, pname)
int  *pargc;
char **pargv[];
char **pname;
{
  /* If we see "seek pdb offset tfile" at the end of the arguments, that
     means that some additional arguments and a pointer to the
     first pass object file (which contains the preprocessed source to compile)
     are found at the given offset in pdb/pass2.db. Read the file and set everything
     up so the rest of the compiler mostly doesn't know what hit it, using tfile
     as a temp file to uncompress the source into and to compile from.

     This routine sets in_file_size, in_file_offset, finput,
     and the filename (*pname) directly; the rest of the commands are
     handled by normal command line scanning on the new argv produced
     here (see the main option loop in toplev).

     IMSTG_GETC and IMSTG_UNGETC have been modified to keep track of the
     number of chars read and to return EOF exactly when chars_read ==
     in_file_size.  Note also that we open finput as a binary file.

     Other than that, we should like like a "normal" (hah!!)
     2nd pass compile from here on out.
  */

  char** argv = *pargv;
  int    argc = *pargc;

  if (argc >= 5 &&
#ifdef DOS
      (argv[argc-4][0]=='-' || argv[argc-4][0]=='/') &&
#else
      argv[argc-4][0]=='-' &&
#endif
      !strcmp (argv[argc-4]+1, "seek"))
  {
    char  buf[4];
    extern FILE* finput;

    char* tmp_file = argv[argc-1];
    unsigned tell = atoi(argv[argc-2]);
    char*    name;

    char**  nargv = 0;
    int     nargc = 0;

    flag_is_subst_compile = 1;

    set_pdb_dir (argv[argc-3]);

    /* Create the pdb if it isn't there already */
    assert (pdb_dir);
    db_note_pdb (pdb_dir);

    name = db_pdb_file (0, "pass2.db");
    finput = fopen (name, RB);

    if (finput && fseek (finput,tell,0) == 0 && fread (buf,1,4,finput) == 4)
    { int i,len;

      CI_U32_FM_BUF (buf,len);

      if ((len -= 4) > 0)
      { char* cmd = xmalloc (len+8);

        /* read <cc1 command><len><in_file_offset> */

        if (fread (cmd, 1, len+8, finput) == len+8)
        { int   nlen;
          char* nbuf;

          /* Read total #bytes for <len><in_file_offset><name> */
          CI_U32_FM_BUF (cmd+len, nlen);

          /* Length of <name> */
          nlen -= 8;

          CI_U32_FM_BUF (cmd+len+4, in_file_offset);

          /* name of first pass input */
          nbuf = xmalloc (nlen+1);
          nbuf[nlen] = '\0';

          if (fread (nbuf, 1, nlen, finput) == nlen)
          { fclose (finput);

            /* Open input and seek to start */
            name = nbuf;
            finput = fopen (name, RB);

            if (finput == 0)
              fatal ("can't open '%s' for read", name);

            if (fseek(finput,in_file_offset,0)==0 && fread(buf,1,4,finput)==4)
            {
              unsigned char *t, *tbuf;
              int c;
              FILE* f;
  
              /* Length of compressed input */
              CI_U32_FM_BUF (buf, in_file_size);

              if (in_file_size <= 4)
                goto oops;

              tbuf = (unsigned char*) xmalloc (in_file_size);
              CI_U32_TO_BUF (tbuf, in_file_size);

              t = tbuf + 4;
              in_file_size -= 4;

              /* Read compressed input from 1st pass object */
              while (in_file_size-- > 0)
              { c = fgetc (finput);
                if (c == EOF)
                  goto oops;
                *t++ = c;
              }

              fclose (finput);

              /* Uncompress input into memory */
              in_file_size   = src_uncompress (&tbuf) - 4;

              /* Write uncompressed input into temp file */
              if (in_file_size <= 0 || tmp_file == 0)
                goto oops;

              f = fopen (tmp_file, WB);
              if (f == 0)
                goto oops;

              fwrite (tbuf+4, 1, in_file_size, f);
              in_file_offset = 0;
              free (tbuf);
              fclose (f);

              /* Open the temp file as our input */
              name   = tmp_file;
              finput = fopen (tmp_file, RB);
              if (finput == 0)
                goto oops;

              /* Number of args in cc1 command */
              CI_U8_FM_BUF  (cmd, len);
              cmd++;
    
              /* We discard the "seek, name, offset tmp" from argv. */
              argc -= 4;
    
              nargc = argc + len;
              nargv = (char**) xmalloc ((nargc+1) * sizeof (char*));
    
              /* Get the old args ... */
              for (i = 0; i < argc; i++)
                nargv[i] = argv[i];
    
              /* Tack on the args from the database ... */
              for (i = argc; i < nargc; i++)
              { nargv[i] = cmd;
                cmd += (strlen (cmd))+1;
              }
    
              /* Preserve the old argv[argc]. */
              nargv[nargc] = argv[argc+4];
            }
          }
        }
      }
    }

    if (nargc == 0)
    { oops:
      fatal ("can't compile from corrupted source code in %s", name);
    }
    else
    { *pargc = nargc;
      *pargv = nargv;
      *pname = name;
    }
  }
}

init_ts_info (name)
char* name;
{
  /* If we are reading a database, recover the target size info for
     this function;  otherwise, clear it (the info has already been
     written out with the previous function def, if we are writing a
     database). */

  if (have_in_glob_db())
  {
    st_node* db_p = find_db_rec(name);
  
    if (db_p == 0 ||
        (db_p->rec_typ != CI_FDEF_REC_TYP &&
         db_p->rec_typ != CI_LIB_FDEF_REC_TYP))
      return 0;
  
    cache_db_rec(db_p);
  
    db_unpack_sizes (CI_LIST_TEXT (db_p, CI_FDEF_TSINFO_LIST), sizev);
  }
  else
    memset (sizev, 0, sizeof (sizev));
}

#ifndef REAL_INSN
#define REAL_INSN(I) \
  (GET_CODE(I)==INSN || GET_CODE(I)==JUMP_INSN || GET_CODE(I)==CALL_INSN)
#endif

static int
estimate_code_size (pnbytes, pibytes)
int *pnbytes;
int *pibytes;
{
  rtx insn = get_insns();

  int nwords = 0;
  int iwords = 0;
  int ninsns = 0;

  for (; insn; insn = NEXT_INSN(insn))
    if (REAL_INSN(insn))
    { if (INSN_PROFILE_INSTR_P(insn))
      { assert (pibytes != 0);
        iwords += insn_size (insn);
      }
      else
        nwords += insn_size (insn);

      ninsns++;
    }

  if (nwords == 0)
    nwords = 1;

  *pnbytes = (nwords << 2);

  if (pibytes)
    *pibytes = (iwords << 2);
  else
    assert (iwords == 0);

  return ninsns;
}

int
ts_info_target (p)
ts_info_point p;
{
  /* Retrieve/record target for this point.  If we have generated too
     much code, return the number of insns we are over by.  If we have
     generated too little, return -(the number we are under by). */

  int ret = 0;


  if (have_in_glob_db())
  { int ninsns, nbytes;

    ninsns = estimate_code_size (&nbytes, 0);

#if 0
    if (sizev[p])
      /* Use the current ratio of insns/bytes to calculate how many
         insns we are off by. */
      ret = db_size_ratio (ninsns, nbytes, nbytes - sizev[p]);
#endif
  }
  else
  { assert (p <= TS_END && sizev[p] == 0);
    estimate_code_size (&sizev[p], &sizev[TS_PROF]);
  }

  return ret;
}

#endif
