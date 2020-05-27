#include "config.h"

#ifdef IMSTG
/*

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


/*
   This file contains the support routines and SYMBOL_REF rtx
   manager.  The main entry point for SYMBOL_REF rtx creation
   is 'gen_symref_rtx'.  This routine is the only way a SYMBOL_REF
   rtx should be created.  Any other creation mechanism may cause
   problems.

   In addition there is a function that execute a function for
   every SYMBOL_REF rtx currently known. This routine is named
   'forall_symref'.
 */

#ifdef macintosh
@pragma segment GNRA
#endif

#include <stdio.h>
#include <ctype.h>
#include "flags.h"
#include "rtl.h"
#include "obstack.h"
#include "tree.h"
#include "basic-block.h"
#include "expr.h"
#include "i_jmp_buf_str.h"
#include "assert.h"
#include "hard-reg-set.h"
#include "i_graph.h"
#include "i_dataflow.h"
#include "i_profile.h"
#include "i_lutil.h"
#include "i_glob_db.h"
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "cc_info.h"
#include "i_toolib.h"

#if defined(DOS)
#include "gnudos.h"
#endif

#ifndef S_ISREG
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif

extern char* dump_base_name;

int flag_db;
int flag_prof_db;
int flag_prof;
int flag_volatile_global;
int flag_bbr;
int flag_build_db;
int flag_build_src_db;
int flag_coalesce;
int flag_coff;
int flag_elf;
int flag_constcomp;
int flag_constprop;
int flag_constreg;
int flag_copyprop;
int flag_default_trip_count = 5;
int flag_default_call_count = 1;
int flag_df_order_by_var;
int flag_glob_inline;
int flag_glob_alias;
int flag_glob_sram;
int flag_linenum_mods = 1;
int flag_prof_instrument_only;
int flag_prof_instrument;
int flag_prof_use;
int flag_sblock;
int flag_sched_sblock;
int flag_shadow_globals;
int flag_shadow_mem;
int flag_marry_mem;
int flag_split_mem;
int flag_dead_elim;
int flag_coerce, flag_coerce2, flag_coerce3, flag_coerce4, flag_coerce5;
int flag_find_bname;
int flag_cond_xform;
int flag_ic960;
int flag_space_opt;
int flag_place_functions;
int flag_unroll_small_loops;
int flag_use_lomem;
int flag_int_alias_ptr, flag_int_alias_real, flag_int_alias_short;

int cnst_prop_has_run = 0;
static int globalize_labels;

double flag_default_if_prob = .45;
char base_file_name[512] = "$";
int have_base_file_name;
char* bname_tmp_file = 0;
char* in_tmp_file = 0;
int func_const_labelno;
extern int const_labelno;

extern struct obstack permanent_obstack;
extern struct obstack *rtl_obstack;

extern char ** save_argv;
extern int     save_argc;
extern int optimize;
extern int quiet_flag;
extern int flag_keep_inline_functions;

unsigned short last_global_rtx_id [NUM_DFK+1];
int found_dead_flow;

int bytes_big_endian = 0;	/* IMSTG_BIG_ENDIAN, but must be always present */

/* HASH_SZ must be a power of 2 */
#define HASH_SZ 512

#define sym_hash(ret_hash, sym) \
{ \
  char *_cp = (sym); \
  int _hash = 0; \
  while (*_cp != 0) \
    _hash += *_cp++; \
  (ret_hash) = _hash & (HASH_SZ - 1); \
}

/*
 * symbol table for keeping track of the symbol ref nodes used in
 * a function.  Each entry points to an EXPR_LIST.
 */
static rtx symref_tab[HASH_SZ];
char* function_dump_name = 0;
int save_stderr_fd = -1;
int save_stdout_fd = -1;

static int
can_rewind (f)
FILE* f;
{
  struct stat sbuf;

  if (f == 0 || fstat(fileno(f),&sbuf) < 0)
    return 0;

  if (S_ISREG(sbuf.st_mode))
    return 1;

  return 0;
}

setup_input_file (name, desc)
char **name;
FILE **desc;
{
  extern char* in_tmp_file;
  extern int in_file_size;
  extern int in_file_offset;
  int need_sinfo;

  need_sinfo = sinfo_name != 0 && (sinfo_file=fopen(sinfo_name,"r")) == 0;
  set_have_real_sinfo (sinfo_name != 0 && need_sinfo == 0);

  /* If we have to scan the input more than once and the user did not give
     us a real input file, we need to copy stdin so that it can be rewound. */

  if (in_file_size == 0 && !can_rewind (*desc))
    if (in_tmp_file)
    { FILE* f = fopen (in_tmp_file, "w");
  
      if (f == 0)
        fatal ("cannot open %s for write", in_tmp_file);
      else
      { int c;
        while ((c = fgetc (*desc)) != EOF)
        { in_file_size++;
          fputc (c, f);
        }
      }

      fclose (f);
      fclose (*desc);

      *name = in_tmp_file;
      *desc = fopen (*name, "r");

      if (*desc == 0)
        fatal ("cannot open %s for read", in_tmp_file);
    }
    else
      fatal ("-input_tmp is required because %s cannot be rewound",*name);

  if (!in_file_size)
  { int c;

    if (fseek (*desc, in_file_offset, 0) != 0)
      fatal ("cannot seek to offset %d of input file %s", in_file_offset,*name);

    while ((c = fgetc (*desc)) != EOF)
      in_file_size++;
  }

  if (need_sinfo)
  { int c,i,l,n,need_note;
    char buf[LISTING_BUF_DECL_SIZE];

    if ((sinfo_file = fopen (sinfo_name, "w")) == 0)
      fatal ("cannot open sinfo file %s for read or for write", sinfo_name);

    l = 0;
    n = 0;
    need_note = -1;

    if (fseek (*desc, in_file_offset, 0) != 0)
      fatal ("cannot seek to offset %d of input file %s", in_file_offset,*name);

    while (get_sinfo_line (buf, sizeof(buf), *desc))
    { if (!(i=strlen(buf)) || buf[--i] != '\n')
        buf[i]='\n';

      ++l;
      ++n;

      if (buf[0]=='#' && buf[1]==' ' && (c=buf[2])>'0' && c<='9')
      { l = atoi(buf+2)-1;
        fprintf (sinfo_file, "n%d\n", n);
        need_note = 0;
      }
      else
      { 
        if (need_note < 0)
          fprintf (sinfo_file, "n%d\n", n++);
        fprintf (sinfo_file, "s   0 %7d   %s", l, buf);
        need_note = 1;
      }
    }

    if (need_note)
      fprintf (sinfo_file, "n%d\n", ++n);

    fclose (sinfo_file);

    if ((sinfo_file = fopen (sinfo_name, "r")) == 0)
      fatal ("cannot reopen sinfo file %s for read", sinfo_name);
  }

  if (fseek (*desc, in_file_offset, 0) != 0)
    fatal ("cannot seek to offset %d of input file %s", in_file_offset,*name);
}

reset_std_files ()
{
  fflush (stderr);
  fflush (stdout);

#ifndef SELFHOST
  if (save_stdout_fd >= 0)
  { /* Current stdout is being sent to /dev/null. Resurrect it */
    close (1);
    dup2 (save_stdout_fd, 1);
    close (save_stdout_fd);
    save_stdout_fd = -1;
  }

  if (save_stderr_fd >= 0)
  { /* Close off current stderr;  it is being written to a file. */
    close (2);
    dup2 (save_stderr_fd, 2);
    close (save_stderr_fd);
    save_stderr_fd = -1;
  }
#endif
}

void
trap_bname_signal (sig)
int sig;
{
  abort_find_bname ("caught signal %d", sig);
}

abort_find_bname (arg, v1)
char* arg;
{
  extern char* bname_tmp_file;

  /* So we don't come back here again from fatal... */
  flag_find_bname = 0;

  /* If anything else goes wrong, make sure we give a message. */
  set_abort_signals(0);

  /* Get stdout and stderr reconnected, close bname_tmp_file */
  reset_std_files ();

  /* If there is a bname tmp file, show the user his messages;  else,
     he has already seen them. */

  if (bname_tmp_file)
  {
    FILE* f = fopen (bname_tmp_file, "r");

    if (f == 0)
      error ("cannot recover stderr from %s", bname_tmp_file);
    else
    {
      int c;

      while ((c = fgetc (f)) != EOF)
        fputc (c, stderr);
    }
  }

  if (arg == 0)
    fatal (arg, v1);
  else
    internal_error (arg, v1);
}

typedef struct {
  named_fd f;
  int len;
  char text[CI_X960_NBUF_INT * sizeof (int)];
} db_arg_buf;

void
db_append_arg (buf, s)
db_arg_buf *buf;
char* s;
{
  int n;

  assert (s);

  n = 1 + strlen (s);

  assert ((buf->len + n) < sizeof (buf->text));

  /* db_buf_at_least (&(buf->text), buf->len + n); */
  strcpy (buf->text + buf->len, s);

  buf->len += n;
}

void
db_flush_args (buf)
db_arg_buf *buf;
{
  assert (buf->text != 0 && buf->f.fd != 0 && buf->len > 0);
  dbf_write (&(buf->f), (char *)&(buf->len), 4);
  dbf_write (&(buf->f), buf->text, buf->len);
  buf->len = 0;
}

void
found_bname (s)
char* s;
{
  int i;
  extern char **save_argv;
  extern int save_argc;
  extern int errorcount,sorrycount;
  char buf[512];
  static db_arg_buf dba_buf;
  char *fname;
  int dummy;

  if (s == 0)
    strcpy (buf, "$");
  else
  { strcpy (buf, s);
    strcat (buf, "$");
  }

  reset_std_files ();

  if (errorcount || sorrycount)
    abort_find_bname(0);

  memset (&dba_buf,  0, sizeof (dba_buf));

  fname = get_cmd_file_name();
  assert (fname != 0);

  dbf_open_write (&(dba_buf.f), fname, db_fatal);

  /* write empty file removal list */
  dummy = 0;
  dbf_write(&(dba_buf.f), &dummy, 4);

  /* write argument list */
  for (i = 0; i < save_argc; i++)
  {
    db_append_arg(&dba_buf, save_argv[i]);
  }
  db_append_arg(&dba_buf, "-bname");
  db_append_arg(&dba_buf, buf);
  db_flush_args(&dba_buf);

  /* terminate command list */
  dummy = 0;
  dbf_write(&(dba_buf.f), &dummy, 4);

  /* terminate file */
  dummy = -1;
  dbf_write(&(dba_buf.f), &dummy, 4);

  dbf_close (&(dba_buf.f));

  fflush (stderr);
  set_abort_signals(0);

  i = 20;
  while (--i > 2)
    close (i);

  /* Since having an unreadable sinfo file is a signal to the compiler
     that it should make up its own, we have to unlink the sinfo file
     if we made up our own before we do the exec, because otherwise
     the fact that the sinfo file is phony will be lost, and the sinfo
     file will will mistakenly be treated as a real one.  TMC Spring '94 */

  if (sinfo_name && sinfo_name[0] && !get_have_real_sinfo())
    unlink (sinfo_name);

  exit(0);
}

rtx
gen_symref_rtx (mode, sym_name)
enum machine_mode mode;
char *sym_name;
{
  int hash;
  rtx symref;
  struct obstack *sv_obstack;

  sym_hash(hash, sym_name);

  for (symref = symref_tab[hash]; symref != 0; symref = SYMREF_NEXT(symref))
  {
    if (strcmp(XSTR(symref,0), sym_name) == 0)
    {
      if (symref->mode != mode)
        fatal("SYMBOL_REF modes don't match for same name.\n");
      return symref;
    }
  }

  /*
   * We didn't find one by this name already, so make one up, and throw
   * it at the front of the list for its hash value.
   * It needs to be allocated on the permanent obstack, so swap the
   * current rtl_obstack to point to the permanent obstack, then restore
   * rtl_obstack after the allocation has taken place.
   */
  sv_obstack = rtl_obstack;
  rtl_obstack = &permanent_obstack;

  symref = rtx_alloc (SYMBOL_REF);	/* Allocate the storage space.  */
  sym_name = obstack_copy0 (rtl_obstack, sym_name, strlen (sym_name));
  rtl_obstack = sv_obstack;

  symref->mode          = mode;
  XSTR(symref,0)        = sym_name;
  RTX_ID(symref)        = ++last_global_rtx_id[IDF_SYM];
  RTX_VAR_ID(symref)    = 0;
  RTX_TYPE(symref)      = 0;
  SYMREF_USECNT(symref) = 0;
  SYMREF_ETC(symref)    = 0;
  SYMREF_SIZE(symref)   = 0;

  SET_SYMREF_USEDBY(symref,0);

  /*
   * put it at the front of the list.
   */
  SYMREF_NEXT(symref) = symref_tab[hash];
  symref_tab[hash] = symref;

  return symref;
}

void
forall_symref (f)
void (*f)();
{
  int i;
  rtx symref;

  /* for every symref in the table process it in some manner. */

  for (i = 0; i < HASH_SZ; i++)
    for (symref = symref_tab[i]; symref != 0; symref = SYMREF_NEXT(symref))
      f(symref);
}

void
clear_symref_var_ids()
{
  int i;
  rtx symref;

  /* for every symref in the table, clear RTX_VAR_ID. */

  for (i = 0; i < HASH_SZ; i++)
    for (symref = symref_tab[i]; symref != 0; symref = SYMREF_NEXT(symref))
      RTX_VAR_ID(symref) = 0;
}

rtx
gen_typed_reg_rtx (type, mode)
tree type;
enum machine_mode mode;
{
  rtx r = gen_reg_rtx (mode);
  RTX_TYPE(r) = type;
  return r;
}

rtx
gen_typed_mem_rtx (type, mode, addr)
tree type;
enum machine_mode mode;
rtx addr;
{
  rtx r = gen_rtx (MEM, mode, addr);
  RTX_TYPE(r) = type;
  return r;
}

rtx
change_address_type (memref, mode, addr, type)
rtx memref;
enum machine_mode mode;
rtx addr;
tree type;
{
  rtx ret;

  assert (GET_CODE(memref)==MEM);
  ret = change_address (memref, mode, addr);

  /* We rely on change_address to return a new mem */
  assert (ret != memref);

  RTX_TYPE(ret) = type;
  return ret;
}

rtx
copy_mem_rtx (memref)
rtx memref;
{
  rtx new = gen_rtx (MEM, GET_MODE(memref), copy_rtx (XEXP(memref,0)));

  MEM_VOLATILE_P (new) = MEM_VOLATILE_P (memref);
  RTX_UNCHANGING_P (new) = RTX_UNCHANGING_P (memref);
  MEM_IN_STRUCT_P (new) = MEM_IN_STRUCT_P (memref);
  COPY_MEM_MRD_FIELDS (new, memref);

  return new;
}

tree
get_rtx_type (r)
rtx r;
{
  if (GET_CODE(r) == SUBREG)
    return get_rtx_type (SUBREG_REG(r));
  else if (GET_CODE(r)==REG || GET_CODE(r)==SYMBOL_REF || GET_CODE(r)==MEM)
    return RTX_TYPE(r);
  else
    return 0;
}

tree
rtx_mode_type (m)
enum machine_mode m;
{
  tree t;
  extern tree float_type_node, double_type_node, long_double_type_node;

  /* Used for type approximation at library calls */

  if (m==Pmode)
    t = ptr_type_node;

  else if (GET_MODE_CLASS(m)==MODE_FLOAT)
  {
    if (GET_MODE_SIZE(m)==int_size_in_bytes(float_type_node))
      t = float_type_node;

    else if (GET_MODE_SIZE(m)==int_size_in_bytes(double_type_node))
      t = double_type_node;

    else if (GET_MODE_SIZE(m)==int_size_in_bytes(long_double_type_node))
      t = long_double_type_node;

    else
    { t = 0;
      assert (0);
    }
  }

  else if (m == SImode || GET_MODE_CLASS(m) == MODE_CC)
    t = integer_type_node;

  else
    t = 0;

  return t;
}

rtx set_rtx_type (r,t)
rtx r;
tree t;
{
  if (r != 0 && (RTX_IS_LVAL_PARENT(r) || RTX_IS_LVAL(r)))
    RTX_TYPE(RTX_LVAL(r)) = t;
  return r;
}

rtx
set_rtx_mode_type (r)
rtx r;
{
  return set_rtx_type (r, rtx_mode_type (GET_MODE(r)));
}

rtx
embed_type (r, type)
rtx r;
tree type;
{
  /* Embed type into r if possible.  If type is not the same as
     the current type, can't do it unless we can make up a new
     object. */

  if (r != 0 && (RTX_IS_LVAL_PARENT(r) || RTX_IS_LVAL(r)))
    if (GET_CODE(r)==SUBREG)
    {
      rtx s = SUBREG_REG(r);
      rtx t = embed_type (s, type);
  
      if (t != s)
        r = gen_rtx (SUBREG, GET_MODE(r), t, SUBREG_WORD(r));
    }
    else
      if (type != RTX_TYPE(r))
        if (GET_CODE(r)==MEM)
        { if (RTX_TYPE(r) != 0)
            r = copy_mem_rtx (r);
          RTX_TYPE(r) = type;
        }
        else
          if (GET_CODE(r) == REG)
          {
            if (REGNO(r) < FIRST_PSEUDO_REGISTER)
            { if (RTX_TYPE(r) != 0)
                r = gen_rtx (REG, GET_MODE(r), REGNO(r));
              RTX_TYPE(r) = type;
            }
          }
          else if (GET_CODE(r) == SYMBOL_REF)
          {
            if (RTX_TYPE(r) == 0)
              RTX_TYPE(r) = type;
          }
  
  return r;
}

rtx
gen_typed_symref_rtx (type, mode, string)
tree type;
enum machine_mode mode;
char* string;
{
  /* This routine should be RARELY called - basically, only by the guy
     who makes up the DECL_RTL which actually uses the symbol.  The type
     presented here is the should be one of array, function, or pointer.

     Pointer is the normal case. */

  extern rtx gen_symref_rtx ();
  rtx r = gen_symref_rtx (mode, string);
  RTX_TYPE(r) = type;
  return r;
}

rtx
pun_rtx_offset (insn, src,m, word)
rtx insn;
rtx src;
enum machine_mode m;
int word;
{
  /* If insn is non-zero, it means to issue any temporary
     calculations directly before insn. */

  if (GET_CODE(src)==SUBREG)
  { word += XINT(src,1);
    src = XEXP (src,0);
  }

  assert (GET_CODE(src)==REG || GET_CODE(src) == MEM);

  if (word != 0 || GET_MODE(src) != m)
    if (GET_CODE(src) == REG)
    {
      if (word != 0 || m != GET_MODE(src))
        if (REGNO(src)<FIRST_PSEUDO_REGISTER &&
            HARD_REGNO_MODE_OK(REGNO(src)+word,m))
        { tree t = RTX_TYPE (src);
          src = gen_rtx (REG, m, REGNO(src)+word);
          RTX_TYPE(src) = t;
        }
        else
          src = gen_rtx (SUBREG, m, src, word);
    }
    else
    { 
      extern rtx change_address();
      rtx sav;

      src = copy_rtx (src);

      if (insn)
        START_SEQUENCE(sav);

      src = change_address
        (src, m, plus_constant (XEXP(src,0), word * GET_MODE_SIZE(SImode)));

      if (insn)
      {
        rtx seq = gen_sequence();
        END_SEQUENCE(sav);
        emit_insn_before (seq,insn);
      }
    }

  return src;
}

rtx
pun_rtx (src, m)
rtx src;
enum machine_mode m;
{
  return pun_rtx_offset (0,src,m,0);
}

/* Given an expression EXP that may be a COMPONENT_REF, a BIT_FIELD_REF,
   or an ARRAY_REF, get the type of the outermost union type thru
   which the refernce to EXP is made.  We use this as the type of the
   reference for MRD purposes. */
   
tree
get_outer_union_type (exp)
tree exp;
{
  tree type = TREE_TYPE(exp);

  while (1)
  {
    if (TREE_CODE (exp)==COMPONENT_REF ||
        TREE_CODE(exp)==BIT_FIELD_REF ||
        TREE_CODE(exp)==ARRAY_REF)
      ;
    else
      if (TREE_CODE (exp) != NON_LVALUE_EXPR
          && ! ((TREE_CODE (exp) == NOP_EXPR||TREE_CODE(exp)==CONVERT_EXPR)
                && (TYPE_MODE (TREE_TYPE (exp))
                    == TYPE_MODE (TREE_TYPE (TREE_OPERAND (exp, 0))))))
        break;

    exp = TREE_OPERAND (exp, 0);

    if (TREE_CODE(TREE_TYPE(exp)) == UNION_TYPE)
      type = TREE_TYPE(exp);
  }

  return type;
}


void xunalloc ();

alloc_insn_flow(need)
int need;
{
  static num_allocated;

  if (need >= 0)
  { if (need==0)
    { xunalloc (&uid_volatile);
      xunalloc (&uid_block_number);
    }
    else
    { if (num_allocated < need)
      { int extra = need - num_allocated;
    
        uid_block_number = (int *)
          xrealloc (uid_block_number, need * sizeof (uid_block_number[0]));
    
        uid_volatile = (char *)
          xrealloc (uid_volatile, need * sizeof (uid_volatile[0]));
    
        bzero (uid_volatile+num_allocated, extra * sizeof(uid_volatile[0]));
  
        while (extra--)
          (uid_block_number+num_allocated)[extra] = -1;
      }
    }
    num_allocated = need;
  }
  return num_allocated;
}

void
prep_for_flow_analysis (f)
     rtx f;
{
  register rtx insn,last_note;
  register int i;
  extern int found_dead_flow;

  /* Count the basic blocks.  Also find maximum insn uid value used.  */

  rtx nonlocal_label_list = nonlocal_label_rtx_list();

  do
  {
    register RTX_CODE prev_code = JUMP_INSN;
    register RTX_CODE code;

    for (insn = f, i = 0; insn; insn = NEXT_INSN (insn))
      { rtx p = PREV_INSN(insn);
        rtx n = NEXT_INSN(insn);

	code = GET_CODE (insn);

        /* Get rid of unreachable insns */
        if (p) 
          while(n != 0 &&
                ((code==NOTE && NOTE_LINE_NUMBER(insn)==NOTE_INSN_DELETED)))
          {
            PREV_INSN(n) = p;
            NEXT_INSN(p) = n;
            insn = n;
            code = GET_CODE(insn);
            n = NEXT_INSN(n);
          }

	if (code == CODE_LABEL
	    || (GET_RTX_CLASS (code) == 'i'
		&& (prev_code == JUMP_INSN
		    || (prev_code == CALL_INSN
			&& nonlocal_label_list != 0)
		    || prev_code == BARRIER)))
	  i++;
	if (code != NOTE)
	  prev_code = code;
      }

    /* Allocate some tables that last till end of compiling this function
       and some needed only in find_basic_blocks and life_analysis.  */
  
    alloc_insn_flow(get_max_uid()+4);
  
    n_basic_blocks = i;
    basic_block_head = (rtx *) xrealloc(basic_block_head, (i+2) * sizeof (rtx));
    basic_block_end = (rtx *) xrealloc (basic_block_end, (i+2) * sizeof (rtx));
    basic_block_drops_in = (char *) xrealloc (basic_block_drops_in, (i+2));
    basic_block_loop_depth = (short *) xrealloc (basic_block_loop_depth, (i+2) * sizeof (short));
#ifdef HAVE_SCHEDULER
    if (flag_schedule_insns)
    { basic_block_volatile = (char *) xrealloc (basic_block_volatile, (i+2));
      bzero (basic_block_volatile, (i+2));
    }
#endif /* HAVE_SCHEDULER */
  }
  while
    (find_basic_blocks (f, nonlocal_label_list), found_dead_flow);

  last_note = get_insns();
  while (NEXT_INSN(last_note) && GET_CODE(NEXT_INSN(last_note))==NOTE)
    last_note = NEXT_INSN(last_note);

  assert (last_note);

  emit_label_after (gen_label_rtx(), last_note);

  ENTRY_LABEL = NEXT_INSN(last_note);
  LABEL_REFS (ENTRY_LABEL)    = ENTRY_LABEL;
  BLOCK_NUM(ENTRY_LABEL)      = ENTRY_BLOCK;
  BLOCK_HEAD(ENTRY_BLOCK)     = ENTRY_LABEL;
  BLOCK_DROPS_IN(ENTRY_BLOCK) = 0;
  BLOCK_END(ENTRY_BLOCK)      = ENTRY_LABEL;

  emit_insn_after (gen_rtx(SET, VOIDmode, 0, 0), ENTRY_LABEL);
  ENTRY_NOTE = NEXT_INSN(ENTRY_LABEL);
  BLOCK_END(ENTRY_BLOCK)      = ENTRY_NOTE;
  BLOCK_NUM(ENTRY_NOTE)       = ENTRY_BLOCK;

  EXIT_LABEL   = emit_label (gen_label_rtx());
  LABEL_REFS (EXIT_LABEL)    = EXIT_LABEL;
  BLOCK_NUM (EXIT_LABEL)     = EXIT_BLOCK;
  BLOCK_HEAD(EXIT_BLOCK)     = EXIT_LABEL;
  BLOCK_DROPS_IN(EXIT_BLOCK) = 0;

  EXIT_NOTE = emit_note (0, NOTE_INSN_FUNCTION_END);
  BLOCK_END(EXIT_BLOCK)      = EXIT_NOTE;
  BLOCK_NUM(EXIT_NOTE)       = EXIT_BLOCK;

  /* Some users will revive these again. */
  mark_rtx_deleted (ENTRY_NOTE,  &ENTRY_NOTE_ZOMBIE);
  mark_rtx_deleted (EXIT_NOTE,   &EXIT_NOTE_ZOMBIE);
  mark_rtx_deleted (ENTRY_LABEL, &ENTRY_LABEL_ZOMBIE);
  mark_rtx_deleted (EXIT_LABEL,  &EXIT_LABEL_ZOMBIE);
}

int pid_flag;

/* If handler in use, go bak there instead of reporting message. */
jmp_buf_struct* xmalloc_handler;

/* If xmalloc_handler==&xmalloc_ret_zero, just return 0 like malloc/realloc 
   normally would. */

jmp_buf_struct xmalloc_ret_zero;
unsigned hi_xmalloc;
unsigned lo_xmalloc;

char*
xmalloc (size)
     unsigned size;
{
  register unsigned value;

  if (size==0)
    size = 4;

  value = (unsigned) malloc (size);

  if (value == 0)
    if (xmalloc_handler)
      if (xmalloc_handler==&xmalloc_ret_zero)
        return 0;
      else
        longjmp (xmalloc_handler->jmp_buf, 1);
    else
      fatal ("Virtual memory exhausted.");

#ifdef WATCH_HEAP
  if (value+size > hi_xmalloc)
  {
    hi_xmalloc = value+size;
    if (lo_xmalloc == 0)
      lo_xmalloc = value;
  }
#endif

  return (char*) value;
}


void
xunalloc (ptr)
char **ptr;
{
  if (*ptr)
  { free (*ptr);
    *ptr = 0;
  }
}

/* Same as `realloc' but report error if no memory available.  */
char*
xrealloc (ptr, size)
     char *ptr;
     int size;
{
  unsigned result = 0;

  if (size == 0)
    size = 4;

  if (ptr)
    result = realloc (ptr, size);
  else
    result = malloc (size);

  if (!result)
    if (xmalloc_handler)
      if (xmalloc_handler==&xmalloc_ret_zero)
        return 0;
      else
        longjmp (xmalloc_handler->jmp_buf, 1);
    else
      fatal ("Virtual memory exhausted.");

#ifdef WATCH_HEAP
  if (result+size > hi_xmalloc)
  {
    hi_xmalloc = result+size;
    if (lo_xmalloc == 0)
      lo_xmalloc = result;
  }
#endif

  return (char*) result;
}

df_hard_reg_info df_hard_reg[FIRST_PSEUDO_REGISTER];
/* Space for entry_label, entry_note, exit_label, and exit_note. */
rtx df_rtx[4];

/* Zombie space for above.  See basicblk.h */
rtx_zombie df_zombie[4];

rtx pid_reg_rtx;

rtx
get_cc0_user(x)
register rtx x;
{ /* insn x sets cc0.  Return the insn which uses this cc0 value.  If
     we call or jump or run off the block without finding the user,
     return 0.  TMC Spring '90.  */

  register RTX_CODE c;

  x = NEXT_INSN (x);
  while (x!=0 && !
    (((c=GET_CODE(x)) ==INSN || c==CALL_INSN || c==JUMP_INSN) &&
     reg_mentioned_p(cc0_rtx, PATTERN(x))))

    if (c==INSN || c==NOTE)
      x=NEXT_INSN(x);
    else
      x = 0;

  return x;
}

int
imstg_func_is_varargs_p(fndecl)
tree fndecl;
{
  tree fnargs = DECL_ARGUMENTS (fndecl);
  tree t;

  if (fnargs != 0)
  {
    t = tree_last(fnargs);

    if (t && DECL_NAME(t) &&
        !strcmp (IDENTIFIER_POINTER (DECL_NAME (t)), "__builtin_va_alist"))
      return 1;

    t = tree_last(TYPE_ARG_TYPES (TREE_TYPE (fndecl)));

    if (t && TREE_VALUE (t) != void_type_node)
      return 1;
  }

  return 0;
}

void
format_file_and_line (file, line, buf)
char* file;
int line;
char* buf;
{
  extern char* progname;

  if (file)
    sprintf (buf, "%s:%d: ", file, line);
  else
    sprintf (buf, "%s: ", progname);
}


void
dump_tree_top (outf, root)
FILE *outf;
tree root;
{
  print_node_brief (outf, "", root, 0);
}


int bbr_time;
int constprop_time;
int copyprop_time;
int glob_inline_time;
int sblock_time;
int shadow_time;
int shadow_mem_time;
int imstg_pic_time;
int constreg_time;
int constcomp_time;
int dead_elim_time;
int coerce_time;
int dataflow_time;
int cond_xform_time;

FILE *bbr_dump_file;
FILE *coer_dump_file;
FILE *constprop_dump_file;
FILE *copyprop_dump_file;
FILE *dataflow_dump_file;
FILE *dwarf_dump_file;
FILE *glob_inline_dump_file;
FILE *sblock_dump_file;
FILE *shadow_dump_file;
FILE *shadow_mem_dump_file;
FILE *imstg_pic_dump_file;
FILE *split_mem_dump_file;
FILE *marry_mem_dump_file;
FILE *dead_elim_dump_file;
FILE *cond_xform_dump_file;

FILE*
fopen_dump_file (ext)
char* ext;
{
  char name[512];
  FILE* f;

  strcpy (name, dump_base_name);
  strcat (name, ".");
  strcat (name, ext);

  if ((f = fopen (name, "w")) == 0)
    pfatal_with_name (name);

  return f;
}

unsigned last_xmalloc_report;

#if defined(IMSTG) && defined(WATCH_HEAP)
#define TIMEVAR(VAR, BODY)    \
do { extern unsigned hi_xmalloc,lo_xmalloc; \
     extern unsigned last_xmalloc_report; \
     int otime = get_run_time (); \
     int hstart = hi_xmalloc; \
     char __mibuf[255]; \
     sprintf(__mibuf,"htop was%8x(%d bytes) at %s:%d; added ", hstart,hstart-lo_xmalloc,__FILE__,__LINE__);\
     BODY; VAR += get_run_time () - otime; \
     if(hi_xmalloc!=hstart||hstart!=last_xmalloc_report)fprintf(stderr,"%s%d,%d\n",__mibuf,hstart-last_xmalloc_report,hi_xmalloc-hstart); \
     last_xmalloc_report=hi_xmalloc; \
} while (0)
#else
#define TIMEVAR(VAR, BODY)    \
do { int otime = get_run_time (); BODY; VAR += get_run_time () - otime; } while (0)
#endif

extern int dump_time;

int did_split;

int
split_mem_opt(decl)
tree decl;
{
  int ret;

  ret = 0;

  if (flag_split_mem)
  {
    ret += split_mem_refs(++did_split);
 
    if (split_mem_dump_file)
      TIMEVAR (dump_time,
      {
        fprintf(split_mem_dump_file,"\n;; Function %s [after split_mem %d]\n\n",
                           IDENTIFIER_POINTER  (DECL_NAME (decl)), did_split);
        print_rtl (split_mem_dump_file, get_insns());
        fflush (split_mem_dump_file);
      });
   }

  return ret;
}

int
marry_mem_opt(decl)
tree decl;
{
  int ret;

  ret = 0;

  if (flag_marry_mem)
  {
    ret += marry_mem_refs();
 
    if (marry_mem_dump_file)
      TIMEVAR (dump_time,
      {
        fprintf(marry_mem_dump_file, "\n;; Function %s [after marry_mem]\n\n",
                           IDENTIFIER_POINTER  (DECL_NAME (decl)));
        print_rtl (marry_mem_dump_file, get_insns());
        fflush (marry_mem_dump_file);
      });
   }

  return ret;
}

int
shadow_mem_opt(decl)
tree decl;
{
  int ret;

  ret = 0;

  if (flag_shadow_mem)
  {
    /* Don't do coalescion this time if we will do it later. */
    int do_coalesce = (flag_coalesce != 0 /* && flag_shadow_mem < 2 */);

    /* Tell shadow to set up CAN_MOVE_MEM_P if cond xform will run */
    TIMEVAR (shadow_mem_time, ret += shadow_mem(do_coalesce, flag_cond_xform));
 
    if (shadow_mem_dump_file)
      TIMEVAR (dump_time,
      {
        fprintf(shadow_mem_dump_file,"\n;; Function %s [after shadow_mem1]\n\n",
                           IDENTIFIER_POINTER  (DECL_NAME (decl)));
        print_rtl (shadow_mem_dump_file, get_insns());
        fflush (shadow_mem_dump_file);
      });
   }

  return ret;
}

int
coerce_opt(decl, pass)
tree decl;
{
  int ret;

  ret = 0;

  if (flag_coerce)
  {
    if (coer_dump_file)
      TIMEVAR (dump_time,
      {
        fprintf(coer_dump_file,"\n;; Function %s [before coerce %d]\n\n",
                           IDENTIFIER_POINTER  (DECL_NAME (decl)), pass);
        print_rtl (coer_dump_file, get_insns());
        fflush (coer_dump_file);
      });

    TIMEVAR (coerce_time, ret += find_dead_coercions ());
 
    if (coer_dump_file)
      TIMEVAR (dump_time,
      {
        fprintf(coer_dump_file,"\n;; Function %s [after coerce %d]\n\n",
                           IDENTIFIER_POINTER  (DECL_NAME (decl)), pass);
        print_rtl (coer_dump_file, get_insns());
        fflush (coer_dump_file);
      });
   }

  return ret;
}

void
imstg_pic(decl, insns)
tree decl;
rtx insns;
{
	     /*	When -mpic insert pic bias computation in the first block of a
		function and rewrite all pic references to adjust for the code
		relocation. */

	if (TARGET_PIC)
		TIMEVAR (imstg_pic_time, imstg_pic_rewrite(insns));

	if (imstg_pic_dump_file)
      		TIMEVAR (dump_time,
      		{
        		fprintf(imstg_pic_dump_file,
				"\n;; Function %s [after imstg_pic]\n\n",
                           	IDENTIFIER_POINTER  (DECL_NAME (decl)));
			print_rtl (imstg_pic_dump_file, insns);
        		fflush (imstg_pic_dump_file);
		});
}

int
shadow2_mem_opt(decl)
tree decl;
{
  int ret;

  ret = 0;

  if (flag_shadow_mem > 1)
  {
    TIMEVAR (shadow_mem_time, ret += shadow_mem(flag_coalesce, 1));
 
    if (shadow_mem_dump_file)
      TIMEVAR (dump_time,
      {
        fprintf(shadow_mem_dump_file,"\n;; Function %s [after shadow_mem2]\n\n",
                           IDENTIFIER_POINTER  (DECL_NAME (decl)));
        print_rtl (shadow_mem_dump_file, get_insns());
        fflush (shadow_mem_dump_file);
      });
   }

  return ret;
}

int
basic_shadow_opt(decl)
tree decl;
{
  int ret = 0;
  rtx insns = get_insns();

  if (flag_shadow_globals)
  {
    TIMEVAR (shadow_time, ret +=shadow_optimize(insns, shadow_dump_file));

    if (shadow_dump_file)
      TIMEVAR (dump_time,
      {
        fprintf (shadow_dump_file, "\n;; Function %s [after shadowing]\n\n",
                      IDENTIFIER_POINTER  (DECL_NAME (decl)));
        print_rtl (shadow_dump_file, insns);
        fflush (shadow_dump_file);
      });
  }
  return ret;
}

int
dead_elim_opt(decl)
tree decl;
{
  int ret = 0;
  rtx insns = get_insns();

  if (flag_dead_elim)
  {
    extern int jump_time;
    TIMEVAR (jump_time, jump_optimize(get_insns(),0,0,0));
    TIMEVAR (dead_elim_time, ret += remove_dead_code());

    if (dead_elim_dump_file)
      TIMEVAR (dump_time,
      {
        fprintf (dead_elim_dump_file, "\n;; Function %s [after dead_code]\n\n",
                      IDENTIFIER_POINTER  (DECL_NAME (decl)));
        print_rtl (dead_elim_dump_file, insns);
        fflush (dead_elim_dump_file);
      });
  }
  return ret;
}

int
copyprop_opt(decl, insns)
tree decl;
rtx insns;
{
  if (optimize && flag_copyprop)
  {
    TIMEVAR(copyprop_time, do_copyprop(insns));

    if (copyprop_dump_file)
      TIMEVAR (dump_time,
      {
        fprintf (copyprop_dump_file,
                 "\n;; Function %s [after copy propagation]\n\n",
                 IDENTIFIER_POINTER  (DECL_NAME (decl)));
        print_rtl (copyprop_dump_file, insns);
        fflush (copyprop_dump_file);
      });
  }
  return 0;
}

void
lose_funny_symbols(x_p)
rtx *x_p;
{
  rtx x = *x_p;
  RTX_CODE code;
  int i,j;
  char *format_ptr;

  if (x == 0)
    return;

  switch (GET_CODE(x))
  {
    case SYMBOL_REF:
      format_ptr = XSTR(x,0);
      if (format_ptr[0] == '@' && format_ptr[1] == '@')
        *x_p = const0_rtx;
      break;

    default:
      format_ptr = GET_RTX_FORMAT (GET_CODE (x));
      for (i = 0; i < GET_RTX_LENGTH (GET_CODE (x)); i++)
      {
        if (format_ptr[i] == 'e')
          lose_funny_symbols (&XEXP(x,i));
        else if (format_ptr[i] == 'E')
        {
          for (j = 0; j < XVECLEN (x, i); j++)
            lose_funny_symbols (&XVECEXP(x, i, j));
        }
      }
  }
}

int
imstg_lose_funny_symbols(insns)
rtx insns;
{
  rtx t_insn;
  for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
    lose_funny_symbols(&t_insn);
}

int
imstg_cond_xform(decl, insns)
tree decl;
rtx insns;
{
  extern int jump_time;

  if (optimize && flag_cond_xform)
  {
    int t;
    TIMEVAR(cond_xform_time, t = do_cond_xform(insns));

    /*
     * Any changes could affect the basic-block structure, so
     * rerun jump optimization.
     */
    if (t)
      TIMEVAR (jump_time, jump_optimize(insns,0,0,0));

    if (cond_xform_dump_file)
      TIMEVAR (dump_time,
      {
        fprintf (cond_xform_dump_file,
                 "\n;; Function %s [after conditional transformation]\n\n",
                 IDENTIFIER_POINTER  (DECL_NAME (decl)));
        print_rtl (cond_xform_dump_file, insns);
        fflush (cond_xform_dump_file);
      });
  }
  return 0;
}

#ifndef DEV_465
int
imstg_bbr_opt(decl, insns)
tree decl;
rtx insns;
{
  if (optimize && flag_bbr && USE_DB)
  {
    extern void bbr_optimize();
    TIMEVAR(bbr_time, bbr_optimize(insns));

    if (bbr_dump_file)
      TIMEVAR(dump_time,
      {
        fprintf(bbr_dump_file,
		"\n;; Function %s [after BBR]\n\n",
		IDENTIFIER_POINTER(DECL_NAME (decl)));
        print_rtl(bbr_dump_file, insns);
        fflush(bbr_dump_file);
      });
  }
}
#endif
int
update_expect(decl, insns)
tree decl;
rtx insns;
{
  if (optimize)
  {
    if(GRAPH_BuildGraph(insns))
    {
      expect_calculate(insns, flag_prof_use);
      GRAPH_FreeGraph();
    }
  }
}

void
zero_extend_double(mode, l1, h1, lv, hv)
enum machine_mode mode;
unsigned l1, h1, *lv, *hv;
{
  unsigned lo;
  unsigned hi;
  unsigned m_size;

  if (mode == VOIDmode)
    mode = DImode;

  m_size = GET_MODE_BITSIZE(mode);

  lo = GET_MODE_MASK(mode);
  if (m_size > HOST_BITS_PER_INT)
    hi = -1;
  else
    hi = 0;

  *lv = lo & l1;
  *hv = hi & h1;
}

void
sign_extend_double(mode, l1, h1, lv, hv)
enum machine_mode mode;
unsigned l1, h1, *lv, *hv;
{
  unsigned lo;
  unsigned hi;
  unsigned m_size;

  if (mode == VOIDmode)
    mode = DImode;

  m_size = GET_MODE_BITSIZE(mode);

  if (m_size > HOST_BITS_PER_INT)
  {
    *lv = l1;
    *hv = h1;
    return;
  }

  zero_extend_double(mode, l1, h1, lv, hv);
  hi = -1;
  lo = ~GET_MODE_MASK(mode);
  if ((l1 & (1 << (m_size-1))) == 0)
    return;
  *lv |= lo;
  *hv |= hi;
}

/* Retrieve constant value from x.  If mode is not VOIDmode,
   extend the value from the highest bit indicated by mode by
   sign extension, implementor's choice, or zero extension, according
   to whether 'extend' is -1,0, or 1. */
   
void
get_cnst_val(x, mode, l_p, h_p, extend)
rtx x;
enum machine_mode mode;
unsigned int *l_p;
unsigned int *h_p;
int extend;	/* (-1,0,1) for (sign extend, best extend, zero extend) */
{
  unsigned int lo = 0;
  unsigned int hi = 0;

  switch (GET_CODE(x))
  {
    case CONST_INT:
      lo = INTVAL(x);
      if ((lo & 0x80000000) == 0)
        hi = 0;
      else
        hi = -1;
      break;

    case CONST_DOUBLE:
      lo = CONST_DOUBLE_LOW(x);
      hi = CONST_DOUBLE_HIGH(x);
      break;

    default:
      abort();
  }

  if (mode == VOIDmode ||
      GET_MODE_CLASS(mode)  != MODE_INT ||
      GET_MODE_BITSIZE(mode) > HOST_BITS_PER_INT)
  { *l_p = lo;
    *h_p = hi;
  }

  else
  {
    if (extend == 0)
      extend = 1;	/* For now, if no preference, we zero-extend */

    if (extend < 0)
      sign_extend_double(mode, lo, hi, l_p, h_p);
    else
      zero_extend_double(mode, lo, hi, l_p, h_p);
  }
}

int cmpl_double(src1_lo, src1_hi, src2_lo, src2_hi)
unsigned src1_lo, src2_lo;
int src1_hi, src2_hi;
{
  if (src1_hi != src2_hi)
    return (src1_hi < src2_hi);

  return (src1_lo < src2_lo);
}

int cmplu_double (src1_lo, src1_hi, src2_lo, src2_hi)
unsigned src1_lo, src1_hi, src2_lo, src2_hi;
{
  if (src1_hi != src2_hi)
    return (src1_hi < src2_hi);

  return (src1_lo < src2_lo);
}

int cmple_double (src1_lo, src1_hi, src2_lo, src2_hi)
unsigned src1_lo, src2_lo;
int src1_hi, src2_hi;
{
  if (src1_hi != src2_hi)
    return (src1_hi < src2_hi);

  return (src1_lo <= src2_lo);
}

int cmpleu_double (src1_lo, src1_hi, src2_lo, src2_hi)
unsigned src1_lo, src1_hi, src2_lo, src2_hi;
{
  if (src1_hi != src2_hi)
    return (src1_hi < src2_hi);

  return (src1_lo <= src2_lo);
}

void
divu (rlo, rhi, dlo, dhi, qlo, qhi)
unsigned rlo, rhi, dlo, dhi, *qlo, *qhi;
{
  unsigned mlo, mhi, wlo, whi;

  *qlo = 0;
  *qhi = 0;

  mhi = 0;
  mlo = 1;

  wlo = dlo;
  whi = dhi;

  /* wlo, whi are denom * (mlo, mhi). */
  while ((whi & 0x80000000) == 0)
  { whi <<= 1;
    whi |= (((int)wlo) < 0);
    wlo <<= 1;

    mhi <<= 1;
    mhi |= (((int)mlo) < 0);
    mlo <<= 1;
  }

  /* dlo,dhi are denom, rlo,rhi are num. */

  while (cmpleu_double (dlo, dhi, rlo, rhi))
  { unsigned tlo, thi;

    /* while (denom * mask) < remainder ... */
    while (cmplu_double (rlo, rhi, wlo, whi))
    { 
      /* (denom * mask) >>= 1 */
      wlo >>= 1;
      wlo |= (whi << 31);
      whi >>= 1;

      /* mask >>= 1 */
      mlo >>= 1;
      mlo |= (mhi << 31);
      mhi >>= 1;
    }

    /* quo |= (mask) */
    add_double (mlo, mhi, *qlo, *qhi,
                (HOST_WIDE_INT *)qlo, (HOST_WIDE_INT *)qhi);

    /* rem -= (denom * mask) */
    neg_double (wlo, whi,
                (HOST_WIDE_INT *)&tlo, (HOST_WIDE_INT *)&thi);
    add_double (rlo, rhi, tlo, thi,
                (HOST_WIDE_INT *)&rlo, (HOST_WIDE_INT *)&rhi);
  }
}

void
divs (rlo, rhi, dlo, dhi, qlo, qhi)
unsigned rlo, rhi, dlo, dhi, *qlo, *qhi;
{
  int s1, s2;

  if (s1 = (((int) rhi) < 0))
    neg_double (rlo, rhi,
                (HOST_WIDE_INT *)&rlo, (HOST_WIDE_INT *)&rhi);

  if (s2 = (((int) dhi) < 0))
    neg_double (dlo, dhi,
                (HOST_WIDE_INT *)&dlo, (HOST_WIDE_INT *)&dhi);

  divu (rlo, rhi, dlo, dhi, qlo, qhi);

  if (s1 != s2)
    neg_double (*qlo, *qhi,
                (HOST_WIDE_INT *)qlo, (HOST_WIDE_INT *)qhi);
}

void
rems (rlo, rhi, dlo, dhi, qlo, qhi)
unsigned rlo, rhi, dlo, dhi, *qlo, *qhi;
{ unsigned tlo, thi;

  divs (rlo, rhi, dlo, dhi, &tlo, &thi);
  mul_double (tlo, thi, dlo, dhi, (HOST_WIDE_INT *)&tlo, (HOST_WIDE_INT *)&thi);
  neg_double (tlo, thi, (HOST_WIDE_INT *)&tlo, (HOST_WIDE_INT *)&thi);
  add_double (rlo, rhi, tlo, thi, (HOST_WIDE_INT *)qlo, (HOST_WIDE_INT *)qhi);
}

void
remu (rlo, rhi, dlo, dhi, qlo, qhi)
unsigned rlo, rhi, dlo, dhi, *qlo, *qhi;
{ unsigned tlo, thi;

  divu (rlo, rhi, dlo, dhi, &tlo, &thi);
  mul_double (tlo, thi, dlo, dhi, (HOST_WIDE_INT *)&tlo, (HOST_WIDE_INT *)&thi);
  neg_double (tlo, thi, (HOST_WIDE_INT *)&tlo, (HOST_WIDE_INT *)&thi);
  add_double (rlo, rhi, tlo, thi, (HOST_WIDE_INT *)qlo, (HOST_WIDE_INT *)qhi);
}

void
bname_setup()
{
  extern int flag_inline_functions;
  char *tptr;

#ifndef SELFHOST
  if (!have_base_file_name)
    if (BUILD_DB || PROF_CODE || USE_DB)
    {
      extern int save_stderr_fd;
      extern char* bname_tmp_file;

      flag_find_bname = 1;

      fflush (stdout);
      save_stdout_fd = dup (1);
#if defined(DOS)
      freopen ("nul", "w", stdout);
#else
      freopen ("/dev/null", "w", stdout);
#endif

      if (bname_tmp_file != 0)
      {
        /* OK, the deal is, when bname_tmp_files is given, we divert stderr
           to bname_tmp_file.  We also catch all signals which interrupt the
           process, including abort, so we can spill the tmp file to the real
           stderr if an error happens.  If this happens, we will display the
           messages and we will not re-exec the compiler.  Otherwise,
           we will discard the messages and the 2nd pass will display them
           on stderr.

           We put stdout out to /dev/null in the first pass, because there
           really shouldn't be any messages coming out to stdout that the
           user should see.

           If bname_tmp_file is not specified, we just leave stderr alone in
           the first pass;  this is intended as "debug" mode.  Innoucous
           messages may be seen from both passes in this case.
        */

        fflush (stderr);
        save_stderr_fd = dup (2);
        freopen (bname_tmp_file, "w", stderr);

        set_abort_signals (trap_bname_signal);
      }
    }
#endif


  /*
   * change any non-identifier chars in base file name to '_' so that
   * it can be used in constructing identifiers.
   */
  tptr = base_file_name;
  while (*tptr != '\0')
  {
    if (*tptr != '$' && !isalnum(*tptr))
      *tptr = '_';
    tptr ++;
  }
}

void
comp_unit_start()
{
  if (have_base_file_name && !GLOBALIZE_STATICS)
  {
    /*
     * We have a source file that contains no external definitions.  We
     * have no reliable way of globalizing static names, and therefore
     * cannot put out, or use a global data base, or else the data base
     * symbol table may contain name collisions.
     */
    flag_build_db = 0;
    flag_build_src_db = 0;
    flag_prof_db = 0;
    flag_prof_instrument = 0;
    flag_prof_instrument_only = 0;
    flag_glob_inline = 0;
    flag_glob_sram = 0;
    flag_glob_alias = 0;
    flag_prof_use = 0;
  }

  if (BUILD_DB || PROF_CODE || USE_DB)
    flag_inline_functions = 0;
    
  if (!flag_find_bname)
  {
    init_glob_db();

    if (BUILD_DB || PROF_CODE)
      prof_instrument_begin(base_file_name);

    if (USE_DB)
      open_in_glob_db ();

    if (have_in_glob_db())
      build_in_glob_db();
  }
}

void
comp_unit_end(asm_out_file)
FILE* asm_out_file;
{
  turn_dumps_on();

  if (BUILD_DB || PROF_CODE)
  {
    extern void dump_symref_info();

    prof_instrument_end(asm_out_file);

    /* Now is the time to dump the variable information to the
     * data base file we have been building.  */

    forall_symref(dump_symref_info);
    dump_src_info();
    end_out_db_info();
  }

  if (bbr_dump_file)
    fclose (bbr_dump_file);

  if (coer_dump_file)
    fclose (coer_dump_file);

  if (constprop_dump_file)
    fclose (constprop_dump_file);

  if (copyprop_dump_file)
    fclose (copyprop_dump_file);

  if (dataflow_dump_file)
    fclose (dataflow_dump_file);

  if (dwarf_dump_file)
    fclose (dwarf_dump_file);

  if (glob_inline_dump_file)
    fclose (glob_inline_dump_file);

  if (sblock_dump_file)
    fclose (sblock_dump_file);

  if (shadow_mem_dump_file)
    fclose (shadow_mem_dump_file);

  if (imstg_pic_dump_file)
    fclose (imstg_pic_dump_file);

  if (shadow_dump_file)
    fclose (shadow_dump_file);

  if (split_mem_dump_file)
    fclose (split_mem_dump_file);

  if (marry_mem_dump_file)
    fclose (marry_mem_dump_file);

  if (dead_elim_dump_file)
    fclose (dead_elim_dump_file);

  if (cond_xform_dump_file)
    fclose (cond_xform_dump_file);

  bbr_dump_file         = 0;
  coer_dump_file        = 0;
  constprop_dump_file   = 0;
  copyprop_dump_file    = 0;
  dataflow_dump_file    = 0;
  dwarf_dump_file       = 0;
  glob_inline_dump_file = 0;
  sblock_dump_file      = 0;
  shadow_mem_dump_file  = 0;
  imstg_pic_dump_file   = 0;
  shadow_dump_file      = 0;
  split_mem_dump_file   = 0;
  marry_mem_dump_file   = 0;
  dead_elim_dump_file   = 0;
  cond_xform_dump_file  = 0;

#ifdef TMC_STATS
  if (flag_shadow_mem)
    print_shadow_stats();
#endif

  finish_listing_info();

  if (!quiet_flag)
  {
    fprintf(stderr, "\n");
#ifndef DEV_465
    if (bbr_time) print_time ("bbr", bbr_time);
#endif
    if (constprop_time) print_time ("constprop", constprop_time);
    if (copyprop_time) print_time ("copyprop", copyprop_time);
    if (glob_inline_time) print_time ("glob_inline", glob_inline_time);
    if (imstg_pic_time) print_time ("imstg_pic", imstg_pic_time);
    if (sblock_time) print_time ("sblock", sblock_time);
    if (shadow_time) print_time ("shadow", shadow_time);
    if (shadow_mem_time) print_time ("shadow_mem", shadow_mem_time);
    if (constreg_time)  print_time ("constreg", constreg_time);
    if (constcomp_time) print_time ("constcomp", constcomp_time);
    if (dead_elim_time) print_time ("dead_elim", dead_elim_time);
    if (coerce_time) print_time ("coerce", coerce_time);
    if (cond_xform_time) print_time ("cond_xform", cond_xform_time);
    if (dataflow_time)
    { print_time ("dataflow", dataflow_time);
      print_df_time();
    }
  }
}

#if !defined(DOS)
#define normalize_file_name(x)	(x)
#endif

int
check_switch (p, n)
char* p;
int n;
{
  /* Set db_argv[n] if we are supposed to remember this switch in the
     second pass.  We are supposed to know the answer for the
     given switch; return 0 if we don't, which should clue the
     caller to not recognize the switch, which should then clue us to
     come in here to decide what to do with it. */

  static char *fixed_switch[] = {
    /* Remember these for the recompilation. */
    "Fbout",
    "Fcoff",
    "Felf",
    "+e0",
    "+e1",
    "+e2",
    "ansi",
    "fPIC",
    "fall-virtual",
    "fallow-single-precision",
    "falt-external-templates",
    "fansi-overloading",
    "fasm",
    "fbuiltin",
    "fbytecode",
    "fcadillac",
    "fcaller-saves",
    "fcommon",
    "fcond-mismatch",
    "fconserve-space",
    "fdefault-inline",
    "fdefer-pop",
    "fdollars-in-identifiers",
    "fdossier",
    "felide-constructors",
    "fenum-int-equiv",
    "fexternal-templates",
    "ffast-math",
    "ffloat-store",
    "fforce-addr",
    "fforce-mem",
    "ffunction-cse",
    "fgc",
    "fgnu-linker",
    "fhandle-exceptions",
    "fhandle-signatures",
    "fhuge-objects",
    "fident",
    "fimplement-inlines",
    "fimplicit-templates",
    "finhibit-size-directive",
    "fint-alias-ptr",
    "fint-alias-real",
    "fint-alias-short",
    "finline",
    "finline-functions",
    "fkeep-inline-functions",
    "flabels-ok",
    "flinenum-mods",
    "fmemoize-lookups",
    "fnonnull-objects",
    "fomit-frame-pointer",
    "fpcc-struct-return",
    "fpic",
    "fpretend-float",
    "freg-struct-return",
    "fsave-memoized",
    "fshared-data",
    "fshort-double",
    "fshort-enums",
    "fshort-temps",
    "fsigned-bitfields",
    "fsigned-char",
    "fstats",
    "fstrict-prototype",
    "fsyntax-only",
    "fthis-is-variable",
    "ftraditional",
    "fnotraditional",
    "funsigned-bitfields",
    "funsigned-char",
    "fverbose-asm",
    "fvolatile",
    "fvolatile-global",
    "fvtable-thunks",
    "fwritable-strings",
    "fxref",
    "ic960",
    "notraditional",
    "pid",
    "traditional",
  };

  static char *fixed2_switch[] = {
    /* Remember these and their arguments for the recompilation. */
    "bname",
  };

  static char *non_fixed_switch[] = {
    /* Forget these for the recompilation */
    "bname_tmp",
    "bsize",
    "clist",
    "dcmd",
    "fbbr",
    "fbuild-db",
    "fcoalesce",
    "fcoerce",
    "fcondxform",
    "fconstcomp",
    "fconstprop",
    "fconstreg",
    "fcopyprop",
    "fcse-follow-jumps",
    "fcse-skip-blocks",
    "fdb",
    "fdead_elim",
    "fdelayed-branch",
    "fexpensive-optimizations",
    "ffancy-errors",
    "fglob-alias",
    "fglob-inline",
    "fglob-sram",
    "fmarry_mem",
    "fmix-asm",
    "fpeephole",
    "fplace-functions",
    "fprof",
    "fprof-instrument",
    "fprof-use",
    "frerun-cse-after-loop",
    "fsblock",
    "fsched_sblock",
    "fschedule-insns",
    "fschedule-insns2",
    "fshadow-globals",
    "fshadow-mem",
    "fspace-opt",
    "fsplit_mem",
    "fstrength-reduce",
    "fthread-jumps",
    "fuse-lomem",
    "function",
    "funroll-all-loops",
    "funroll-loops",
    "input_tmp",
    "outz",
    "sinfo",
    "tmpz",
    "Z",
  };

  int i;

  char buf[1024];

  assert (n > 0 && n < save_argc && db_argv[n] == 0);

  if (p == 0 || *p == '\0')
    return 0;

  if (!strncmp (p, "mno-", 4) || !strncmp (p, "fno-", 4))
  { strcpy (buf+1, p+4);
    buf[0]=p[0];
    p=buf;
  }

  if (*p == 'm' ||
      !strncmp (p, "ffixed-", 7) ||
      !strncmp (p, "fcall-used-", 11) ||
      !strncmp (p, "fcall-saved-", 12))
  { db_argv[n] = 1;
    return 1;
  }

  for (i=0; i < sizeof (fixed_switch)/sizeof(fixed_switch[0]); i++)
    if (!strcmp (p, fixed_switch[i]))
    { db_argv[n] = 1;
      return 1;
    }

  for (i=0; i < sizeof (fixed2_switch)/sizeof(fixed2_switch[0]); i++)
    if (!strcmp (p, fixed2_switch[i]))
    { db_argv[n] = 1;
      db_argv[n+1] = 1;
      return 1;
    }

  for (i=0; i < sizeof (non_fixed_switch)/sizeof(non_fixed_switch[0]); i++)
    if (!strcmp (p, non_fixed_switch[i]))
      return 1;

  /* Forget warnings, opt level, and debugging options.  Note that some
     switches with these start letters were caught earlier in the
     tables (such as dcmd). */

  if (*p == 'W' || *p=='w' || *p == 'O' || *p == 'd')
    return 1;

  return 0;
}

/* If argv[i] is an option (mstg-specific) which we care about,
   process it and return 1.  If we have already processed it, return 1
   and don't do anything else.  Otherwise return 0.

   Note - this routine is called several times for options that
   have complex ordering dependencies.  'scan_opt_pass'
   tells us (from 0..3) which of the call sites from toplev 
   we are coming from.  This flag is used to delay processing of some options
   until appropriate things (such as defaulting based on optimization
   level) have been done.
*/

int scan_opt_pass;
scan_option (i)
int i;
{
  static char* did_arg_already = 0;
  char buf[1024];
  char* p = buf;
  int t;

  /* Set up a vector so we can remember the options we've processed */

  if (did_arg_already == 0)
    bzero ((did_arg_already = (char *) xmalloc (save_argc)), save_argc);
  else
    if (did_arg_already[i])
      return 1;

  strcpy (p, save_argv[i]);;
    
#if defined(DOS)
  if (*p++ != '-' && *(p-1) != '/')
#else
  if (*p++ != '-')
#endif
    return 0;

  if (*p == 'O')
  {
    /* Handle On, O3_1, O4_1, O3_2, etc here because gcc doesn't understand.

       Note that gcc will apply "OPTIMIZATION_OPTIONS(optimize)" later,
       so all we have to do is set "optimize" and note
       any additional effects (database and profiling switches) */

    static int did_Ox_x;

    int opt  = 0;
    
    p++;
    while (*p >= '0' && *p <= '9')
      opt = opt * 10 + (*p++ - '0');

    /* Don't allow O5 or above from normal command line */
    if (opt > 4 && !flag_is_subst_compile)
      fatal ("optimization levels above 4 are allowed only in substitution recompilations");

    if (*p=='\0')
    { if (opt > 4)
        strcpy (p, "_2");

      if (opt >= 4)
        opt = 4;
    }
    
    /* accept -O, -On, -Ox_x */

    if (*p == '\0')
    { if (p[-1] == 'O')
      { assert (opt == 0);
        opt = 1;
      }

      if (opt > optimize && !did_Ox_x)
        optimize = opt;
    }

    else if (*p == '_')
    { if (opt < 3 || opt > 4)
        return 0;

      if (p[1]=='1' && p[2]=='\0')
      { /* generate profiling code and ccinfo but no cpp output */

        optimize = 1;

        flag_build_db = 1;
        flag_prof_instrument = 1;
      }
      else if (p[1]=='2' && p[2]=='\0')
      { 
        optimize = opt;

        flag_prof_use = 1;
        flag_glob_inline = 1;
        flag_glob_sram = 1;
        flag_glob_alias = 1;
        flag_sblock = 1;
        flag_sched_sblock = 1;
        flag_bbr = 1;
      }
      else
        return 0;

      did_Ox_x = 1;	/* We don't allow overriding opt level for Ox_x */
    }

    else
      return 0;

    /* We are going to accept it, so this should be the first pass */
    assert (scan_opt_pass == 0);

    return (did_arg_already[i]=1);
  }

  else if (scan_opt_pass != 0 && lang_decode_option (p-1))
    ;

  else
  {
    if (*p == 'Y')
      p++;

    if (i < save_argc-1 && set_list_option (save_argv[i], save_argv[i+1]))
      did_arg_already[i+1] = 1;
    else
    { switch (*p++)
      { extern int flag_signed_char;
  
        default:
          return 0;
    
        case 'f':
          if (!strcmp (p, "unction") && i < save_argc - 1)
          { function_dump_name = save_argv[i+1];
            did_arg_already[i+1] = 1;
          }
  
          else
          { int found = 0;
            int lim, j;

            if (scan_opt_pass == 0)
            { j = 0;
              lim = FIRST_REGULAR_FOPTION;
            }
            else
            { j = FIRST_REGULAR_FOPTION;
              lim = sizeof_f_options/sizeof(f_options[0]);
            }

            /* Search for it in the table of options.  */

            for (; !found && j < lim; j++)
            { found = 1;

              if (!strncmp(p,"no-",3) && !strcmp (p+3,f_options[j].string))
                *f_options[j].variable = ! f_options[j].on_value;

              else if (!strcmp (p, f_options[j].string))
                *f_options[j].variable = f_options[j].on_value;

              else if (!strncmp (p, "fixed-", 6))
                fix_register (&p[6], 1, 1);

              else if (!strncmp (p, "call-used-", 10))
                fix_register (&p[10], 0, 1);

              else if (!strncmp (p, "call-saved-", 11))
                fix_register (&p[11], 0, 0);

              else
                found = 0;
            }

            if (!found)
              return 0;
          }
          break;
  
        case 's':
          if (!strcmp(p,"info") && i<save_argc-1 &&
		set_sinfo_file(normalize_file_name(save_argv[i+1])))
            did_arg_already[i+1]=1;
          else
            return 0;
          break;
    
        case 'm':
          if (scan_opt_pass == 0)
            return 0;

          if (!strcmp (p, "ic2.0-compat") || !strcmp(p, "ic-compat"))
          {
            target_flags &= ~(TARGET_FLAG_IC_COMPAT3_0);
            target_flags |= TARGET_FLAG_IC_COMPAT2_0;
            target_flags |= TARGET_FLAG_CLEAN_LINKAGE;
            flag_signed_char = 1;
          }
          else if (!strcmp (p, "ic3.0-compat"))
          {
            target_flags &= ~(TARGET_FLAG_IC_COMPAT2_0);
            target_flags |= TARGET_FLAG_IC_COMPAT3_0;
            target_flags |= TARGET_FLAG_CLEAN_LINKAGE;
            flag_signed_char = 1;
          }
          else
            return 0;
          break;
  
        case 'F':
          if (!strcmp (p, "bout"))
          {
            flag_coff = 0;
            flag_elf = 0;
          }
          else if (!strcmp (p, "coff"))
          {
            flag_coff = 1;
            flag_elf = 0;
          }
#ifndef DEV_465
          else if (!strcmp (p, "elf"))
          {
            flag_elf = 1;
            flag_coff = 0;
          }
#endif
          else
            return 0;
          break;
  
        case 'b':
          if (!strcmp (p, "name"))
          { strcpy (base_file_name,save_argv[i+1]);
            did_arg_already[i+1] = 1;
            have_base_file_name = 1;
            globalize_labels = GLOBALIZE_STATICS;
          }
          else if (!strcmp (p, "name_tmp"))
          { bname_tmp_file = normalize_file_name(save_argv[i+1]);
            did_arg_already[i+1] = 1;
          }
          else if (!strcmp (p, "size") && i < save_argc-1 &&
                   isdigit(save_argv[i+1][0]) && atoi(save_argv[i+1])>=0)
          { extern int max_df_insn;
            max_df_insn = atoi (save_argv[i+1]);
            did_arg_already[i+1] = 1;
          }
          else
            return 0;
          break;
  
        case 'd':
	  if (!strcmp (p, "cmd"))
	  { /* Put the driver's command line in the assembly output. */
	    extern char* driver_command_line;
            did_arg_already[i+1] = 1;
	    driver_command_line = save_argv[i+1];
	  }
          else
          {
            /* If no dump base yet, we'll come back and get it later. */
            if (dump_base_name == 0)
              return 0;

            switch (*p)
            {
              default :return 0;
              case 'a':
                bbr_dump_file         = fopen_dump_file("bbr");
                coer_dump_file        = fopen_dump_file("coe");
                constprop_dump_file   = fopen_dump_file("cnp");
                copyprop_dump_file    = fopen_dump_file("cpr");
                dataflow_dump_file    = fopen_dump_file("dfl");
                dwarf_dump_file       = fopen_dump_file("dwf");
                glob_inline_dump_file = fopen_dump_file("gin");
                sblock_dump_file      = fopen_dump_file("sbl");
                shadow_mem_dump_file  = fopen_dump_file("sdm");
                imstg_pic_dump_file   = fopen_dump_file("pic");
                shadow_dump_file      = fopen_dump_file("sdg");
                split_mem_dump_file   = fopen_dump_file("msp");
                marry_mem_dump_file   = fopen_dump_file("mmy");
                dead_elim_dump_file   = fopen_dump_file("dce");
                cond_xform_dump_file  = fopen_dump_file("cxm");
                did_arg_already [i] = 1;
                return 0;
                break;
              case 'C':
                constprop_dump_file   = fopen_dump_file("cnp");
                break;
              case 'E':
                coer_dump_file        = fopen_dump_file("coe");
                break;
              case 'P':
                copyprop_dump_file    = fopen_dump_file("cpr");
                break;
              case 'F':
                dataflow_dump_file    = fopen_dump_file("dfl");
                break;
              case 'W':
                dwarf_dump_file       = fopen_dump_file("dwf");
                break;
              case 'G':
                glob_inline_dump_file = fopen_dump_file("gin");
                break;
              case 'B':
                sblock_dump_file      = fopen_dump_file("sbl");
                break;
              case 'H':
                shadow_mem_dump_file  = fopen_dump_file("sdm");
                break;
              case 'A':
                imstg_pic_dump_file  = fopen_dump_file("pic");
                break;
              case 'I':
                shadow_dump_file      = fopen_dump_file("sdg");
                break;
              case 'Z':
                split_mem_dump_file   = fopen_dump_file("msp");
                marry_mem_dump_file   = fopen_dump_file("mmy");
                break;
              case 'X':
                dead_elim_dump_file   = fopen_dump_file("dce");
                break;
              case 'Y':
                bbr_dump_file         = fopen_dump_file("bbr");
                break;
              case 'T':
                cond_xform_dump_file  = fopen_dump_file("cxm");
                break;
            }
          }
          break;
    
        case 'i':
          if (!strcmp (p, "nput_tmp"))
          { in_tmp_file = normalize_file_name(save_argv[i+1]);
            did_arg_already[i+1] = 1;
          }
          else if (!strcmp (p, "c960"))
            flag_ic960 = 1;
          else
            return 0;
          break;
  
        case 'p':
          if (!strcmp (p, "id"))
            pid_flag = 1;
          else
            return 0;
          break;
  
        case 'Z':
          if (*p == '\0')
          { /* Remember pdb, for later creation if needed. */
            set_pdb_dir (save_argv[i+1]);
            did_arg_already[i+1] = 1;
          }
      }
      p--;
    }
  }

  /* If we are still here, the switch was processed, and we
     don't want the caller looking at it. */

  /* Note it in db_argv. */
  t = check_switch (p, i);
  assert (t);

  return (did_arg_already[i]=1);
}

asm_function_local_label (l, opts)
char* l;
int opts;
{
  char lbuf[255];
  extern FILE* asm_out_file;
  FILE* f;
  int is_glob;
  int ret;

  f = asm_out_file;

  if (l == 0)
    l = lbuf;

  if (opts & GENERATE_LABEL)
  {
    if ((is_glob = globalize_labels) != 0)
      sprintf(l, "*LFC%d.%s", (ret=func_const_labelno++), base_file_name);
    else
      sprintf(l, "*LC%d", (ret=const_labelno++));
  }
  else
  {
    assert (l[0] == '*' && l[1] == 'L');

    if (l[2] == 'F')
    { assert (l[3]== 'C');
      is_glob = 1;
      ret = atoi (l+4);
    }
    else 
    { assert(l[2] == 'C');
      is_glob = 0;
      ret = atoi (l+3);
    }
  }

  if (is_glob && GLOBALIZE_STATICS && (opts & GLOBALIZE_LABEL))
    ASM_GLOBALIZE_LABEL (f, l);

  if (opts & OUTPUT_LABEL)
    ASM_OUTPUT_LABEL (f, l);

  return ret;
}

/* In first pass, insert profiling code and save inlining info.

   In second pass, annote rtl with profiling info and do global
   inlining.  Return 0 if we should evade the rest if the compilation
   because nobody cares about this function.  */
int
imstg_function_start(decl)
tree decl;
{
  extern FILE* glob_inline_dump_file;
  extern int did_split;
  rtx insns = get_insns();

  globalize_labels = 0;
  did_split = 0;

  /* Set up target size info */
  init_ts_info (XSTR(XEXP(DECL_RTL(decl),0),0));

  /* Insert profiling instrumentation code, or annotate the rtl with
   * profiling information at this time.  */

  if (BUILD_DB || PROF_CODE)
    imstg_prof_min (insns);
  else
    imstg_prof_annotate(insns);

  /* Record target for size after instrumentation */
  ts_info_target (TS_RTL);

  /* Do the function save information if we are doing first pass of two
   * pass compilation, or if second pass of two pass compilation, do the
   * global inlining at this point.  */

  if (BUILD_DB || PROF_CODE)
    start_func_info (decl, insns);

  else if (have_in_glob_db() && flag_glob_inline)
  {
    char *cur_func_name = XSTR(XEXP(DECL_RTL(decl),0),0);
    int  ret;

    if (do_glob_inlines(insns, cur_func_name) < 0 &&
        !flag_keep_inline_functions)

      /* This function was marked as deletable, so don't compile it. */
      return 1;
  }

  /* compute the jump probabilities, loop counts, and insn expects. */
  imstg_compute_prob_expect(insns);
  make_mems_valid(insns);

  if (glob_inline_dump_file)
    TIMEVAR (dump_time,
    {
      fprintf (glob_inline_dump_file, "\n;; Function %s\n\n",
               IDENTIFIER_POINTER (DECL_NAME (decl)));
      print_rtl (glob_inline_dump_file, insns);
      fflush (glob_inline_dump_file);
    });

  return 0;
}

/* Reset any state needed at end of function.  The function
   has already been output if its is going to be */
void
imstg_function_end(decl)
tree decl;
{
  extern int const_fixup_happened;

  globalize_labels = GLOBALIZE_STATICS;
  cnst_prop_has_run = 0;
  const_fixup_happened = 0;

#ifdef RESTORE_OPTIONS_AFTER_FUNC
  RESTORE_OPTIONS_AFTER_FUNC(decl);
#endif
}

#define ASM_FILE_END(F) comp_unit_end(F)

void
imstg_symref_usedby_current(r)
rtx r;
{
  db_nameref *p;
  rtx fname_sym;
  char *fname;

  /* The purpose of this routine is to note all possible names which
     could affect the generated code rtl for the current function,
     for purposes of analyzing whether or not 2nd pass compiles inlining
     the current function need to be redone.

     This routine is called from imstg_use_count and imstg_note_addr_taken.

     For now, we consider that an address taken during static initialization
     implies a reference from whatever function we happen to be in, which is
     inaccurate but safe.  We can't easily seperate these cases because the
     callers use the same code for processing static initializers and for
     generating code.  This means that when the global information for a
     variable (size and/or addr taken) changes, we will occasionally invalidate
     a function we needn't have, and the effects will be that any inliner of
     the function will be (perhaps needlessly) invalidated.

     Because the size and addr-taken status of a variable shouldn't change
     very often, I can't really see this as a problem.  tmc winter 94 */

  if (current_function_decl == 0)
    return;

  p = GET_SYMREF_USEDBY(r);
  fname_sym = XEXP (DECL_RTL (current_function_decl), 0);
  fname = XSTR (fname_sym, 0);

  /* Check and see if we already know that r was referenced in this
     function.  If not, note it. */

  if (p==0 || strcmp(XSTR(p->fname_sym,0), XSTR(fname_sym,0)))
  {
    db_nameref *q = (db_nameref *) xmalloc (sizeof (*p));

    q->fname_sym = fname_sym;
    q->prev = p;

    SET_SYMREF_USEDBY(r, q);
  }
}

void
imstg_note_addr_taken(exp)
tree exp;
{
  tree x = exp;
  rtx  decl_rtx;

  while (1)
  {
    switch (TREE_CODE (x))
      {
      case ADDR_EXPR:
      case COMPONENT_REF:
      case ARRAY_REF:
        x = TREE_OPERAND (x, 0);
        break;

      case VAR_DECL:
      case CONST_DECL:
      case PARM_DECL:
      case RESULT_DECL:
      case FUNCTION_DECL:
        decl_rtx = DECL_RTL(x);
        if (decl_rtx != 0 &&
            GET_CODE(decl_rtx) == MEM &&
            GET_CODE(XEXP(decl_rtx,0)) == SYMBOL_REF)
        {
          /* mark the symbol ref as having had its address taken. */
          SYM_ADDR_TAKEN_P(XEXP(decl_rtx,0)) = 1;
          SYMREF_ADDRTAKEN(XEXP(decl_rtx,0)) = 1;
          imstg_symref_usedby_current (XEXP(decl_rtx,0));

          /*
           * Also if this is a variable bump its use count by a slightly 
           * since we are taking its address.
           */
          if (TREE_CODE(x) == VAR_DECL)
            SYMREF_USECNT(XEXP(decl_rtx,0)) += 2;
        }
        return;

      default:
        return;
    }
  }
}

void
imstg_use_count(decl)
tree decl;
{
  rtx x = DECL_RTL(decl);

  if (TREE_CODE(decl) == VAR_DECL &&
      x != 0 && GET_CODE(x) == MEM &&
      GET_CODE(XEXP(x,0)) == SYMBOL_REF)
  { SYMREF_USECNT(XEXP(x,0)) += 1;
    imstg_symref_usedby_current (XEXP(x,0));
  }
}

int
imstg_const_propagation(decl, insns)
tree decl;
rtx insns;
{
  if (optimize && flag_constprop)
  {
    TIMEVAR(constprop_time, do_cnstprop(insns));

    if (constprop_dump_file)
      TIMEVAR (dump_time,
      {
        fprintf (constprop_dump_file,
                 "\n;; Function %s [after constant propagation]\n\n",
                 IDENTIFIER_POINTER  (DECL_NAME (decl)));
        print_rtl (constprop_dump_file, insns);
        fflush (constprop_dump_file);
      });
  }
  cnst_prop_has_run = 1;
}

void
imstg_fixup_const(decl, insns)
tree decl;
rtx insns;
{
  cleanup_consts(insns);
}

/* c0 is an associative operator which is to be the parent of the same
   operator (except perhaps for negation) applied to c1, c2.  Return the
   appropriate type for the folded result of c1,c2.  See split_tree and
   the places in fold where it is called. */

tree
pick_fold_type (c0,c1,c2)
tree c0;
tree c1;
tree c2;
{
  /* Pointers win vs anything except pointers;  otherwise,
     return c0 sized int, since c0 is the final size.  Note that
     we do expect ptr vs ptr to yield an integral type.  tmc
     summer '92 */

  c0 = TREE_TYPE(c0);
  c1 = TREE_TYPE(c1);
  c2 = TREE_TYPE(c2);

  if (TREE_CODE(c1)==POINTER_TYPE && TREE_CODE(c2)!=POINTER_TYPE)
    return c1;

  if (TREE_CODE(c2)==POINTER_TYPE && TREE_CODE(c1)!=POINTER_TYPE)
    return c2;

  /* Because of two's complement arithmetic, as long as the
     selected type has a size as large as the final result,
     we should get the right final answer, regardless of any
     overflow. */

  return type_for_size (int_size_in_bytes(c0)*BITS_PER_UNIT, 0);
}


/*
 * called from toplev to invoke super block formation
 */
void imstg_sblock (decl, insns)
tree decl;
rtx insns;
{
  extern int jump_time;

  if ( ! optimize || ! flag_sblock )
   return;

  /* Update all the INSN_EXPECT fields */
  update_expect(decl, insns);

  /*
   * form superblocks before CSE and scheduling
   */

  TIMEVAR (sblock_time, sblock_optimize (insns));

  /* Dump rtl code after sblocks, if we are doing that.  */

  if ( sblock_dump_file )
  {
    TIMEVAR (dump_time,
              {
                print_rtl (sblock_dump_file, insns);
                fflush (sblock_dump_file);
              });
  } /* end if */

  /*
   * need to run jump_optimized to get correct data structures
   * after modifying the jump instructions
   */
  TIMEVAR (jump_time, jump_optimize (insns, 0, 0, 0));

} /* end imstg_sblock */


/*
 * called from toplev to rerun flow after the first
 * scheduling pass
 */
int second_time_flow = 0;	/* checked by flow.c */

imstg_second_flow (insns, decl)
	rtx insns;
	tree decl;
{
	extern int flag_schedule_insns;
	extern int flow_time;
	extern int flow_dump;
	extern int obey_regdecls;
	extern int warn_uninitialized;
	extern FILE * flow_dump_file;
        rtx i;

        /*
         * the first thing we need to do is delete the old
         * LOG_LINKS information, so as not to cause problems
         */
        for ( i = insns; i; i = NEXT_INSN (i) ) {
                if (GET_RTX_CLASS (GET_CODE (i)) != 'i')
                        continue;
                LOG_LINKS(i) = 0;
        } /* end for */


  second_time_flow = 1;
  if (optimize > 0 && flag_schedule_insns) {

	  /* Print function header into flow dump now
	     because doing the flow analysis makes some of the dump.  */

	  if (flow_dump)
	    TIMEVAR (dump_time,
		     {
		       fprintf (flow_dump_file, "\n;; Function %s after sched 1\n\n",
				IDENTIFIER_POINTER (DECL_NAME (decl)));
		     });

	  if (obey_regdecls)
	    {
	      TIMEVAR (flow_time,
		       {
			 regclass (insns, max_reg_num ());
			 stupid_life_analysis (insns, max_reg_num (),
					       flow_dump_file);
		       });
	    }
	  else
	    {
	      /* Do control and data flow analysis,
		 and write some of the results to dump file.  */

	      TIMEVAR (flow_time, flow_analysis (insns, max_reg_num (),
						 flow_dump_file));
	      if (warn_uninitialized)
		{
		  uninitialized_vars_warning (DECL_INITIAL (decl));
		  setjmp_args_warning ();
		}
	    }

	  /* Dump rtl after flow analysis.  */

	  if (flow_dump)
	    TIMEVAR (dump_time,
		     {
		       print_rtl (flow_dump_file, insns);
		       fflush (flow_dump_file);
		     });
	}
  second_time_flow = 0;
} /* end imstg_second_flow */


imstg_round_field_align (field, pconst_size, pdesired_align, precord_align)
tree field;
int *pconst_size;
int *pdesired_align;
int* precord_align;
{
  int const_size    = *pconst_size;
  int desired_align = *pdesired_align;
  int record_align  = *precord_align;

  if (TREE_TYPE(field) != error_mark_node)
  {
    if (DECL_BIT_FIELD_TYPE(field))
    { /* Align bit fields as per section 8.5.2 of ic960 v2.0
         compiler manual. */
  
      int empty_f, byte_pos, bit_off, field_size;

      empty_f  = MIN (32, TREE_GET_ALIGN_LIMIT(field));
      byte_pos = (const_size / 8) %  (empty_f / 8);
      bit_off  = (const_size &  7l) + (8 * byte_pos);
        
      field_size = DECL_FIELD_SIZE(field);

      if ((bit_off + field_size) > 32)
        const_size = ROUND (const_size, empty_f);

      record_align = MAX(record_align, empty_f);
    }
    else
    {
#ifndef DEV_465
      if (TREE_GET_ALIGN_LIMIT(field) < desired_align)
#else
      if (TREE_GET_ALIGN_LIMIT(field) < desired_align ||
          TREE_LANG_FLAG_5(field))
#endif
      { /* Apply #pragma pack, or #pragma i960_align */

        DECL_PACKED(field) = 1;
        desired_align = TREE_GET_ALIGN_LIMIT(field);
        DECL_ALIGN(field) = desired_align;

        /* We rely on caller (layout_record) to update record_align,
           var_size, etc, based on what we just did to DECL_ALIGN 
           and desired_align */
      }
    }
  }

  *pconst_size    = const_size;
  *pdesired_align = desired_align;
  *precord_align  = record_align;
}

setup_standard_rtx_types()
{
  if (stack_pointer_rtx)         RTX_TYPE(stack_pointer_rtx)         = ptr_type_node;
  if (frame_pointer_rtx)         RTX_TYPE(frame_pointer_rtx)         = ptr_type_node;
  if (arg_pointer_rtx)           RTX_TYPE(arg_pointer_rtx)           = ptr_type_node;
  if (virtual_incoming_args_rtx) RTX_TYPE(virtual_incoming_args_rtx) = ptr_type_node;
  if (virtual_stack_vars_rtx)    RTX_TYPE(virtual_stack_vars_rtx)    = ptr_type_node;
  if (virtual_stack_dynamic_rtx) RTX_TYPE(virtual_stack_dynamic_rtx) = ptr_type_node;
  if (virtual_outgoing_args_rtx) RTX_TYPE(virtual_outgoing_args_rtx) = ptr_type_node;
  if (struct_value_rtx)          RTX_TYPE(struct_value_rtx)          = ptr_type_node;
  if (struct_value_incoming_rtx) RTX_TYPE(struct_value_incoming_rtx) = ptr_type_node;
  if (static_chain_rtx)          RTX_TYPE(static_chain_rtx)          = ptr_type_node;
  if (static_chain_incoming_rtx) RTX_TYPE(static_chain_incoming_rtx) = ptr_type_node;
  if (pic_offset_table_rtx)      RTX_TYPE(pic_offset_table_rtx)      = ptr_type_node;
  if (pid_reg_rtx)               RTX_TYPE(pid_reg_rtx)               = ptr_type_node;
}

#ifdef IMSTG_UNALIGNED_MACROS
static int
int_val (p, targ)
tree p;
int* targ;
{
  int ret = 0;
  *targ   = 0;

  if (TREE_CODE(p) == NOP_EXPR)
    ret = int_val (TREE_OPERAND (p,0), targ);

  else if (TREE_CODE(p) == INTEGER_CST)
    (ret=1, *targ = TREE_INT_CST_LOW(p));

  return ret;
}

/* This function expands the builtin functions
 *    __builtin_get_unaligned(address, size) and
 *    __builtin_set_unaligned(address, size, expr)
 * to one of the "standard" unaligned access functions, which
 * are named __builtin_get_unaligned_size_X_align_Y() and
 *           __builtin_set_unaligned_size_X_align_Y().
 *
 * The basic algorithm is:
 *    determine size of the type specified (size)
 *    determine largest provable alignment of the address (align)
 *    generate a call to __builtin_get_unaligned_size_X_align_Y,
 *       where X = size, Y = max(align, size).
 *
 * Complain if there is no function to expand to, or if size > 4.
 * SVW Spring '92 (with TMC)
 */

rtx
imstg_expand_unaligned_builtin (exp, target, ignore)
tree exp;
rtx  target;
int  ignore;
{
  char  nbuf[128], *name;
  tree  func, new_func, args, lookup_name ();
  int   align, size, try_align;
  enum  built_in_function tid;

  func = TREE_OPERAND (TREE_OPERAND(exp,0),0);
#ifndef DEV_465
  name = IDENTIFIER_POINTER (DECL_NAME (TREE_OPERAND (TREE_OPERAND(exp,0),0)));
#else
  name = DECL_PRINT_NAME (TREE_OPERAND (TREE_OPERAND(exp,0),0));
#endif
  tid       = DECL_FUNCTION_CODE (func);

  /* get the worst case pointer alignment and the alignment
     of the integer operand.  */

  args  = TREE_OPERAND (exp,1);
  align = BIGGEST_ALIGNMENT;

  { register tree value = TREE_VALUE (args);
    register tree type  = TREE_TYPE  (value);

    assert (TREE_CODE(type)==POINTER_TYPE);
    args = TREE_CHAIN (args);
    align = get_pointer_alignment (value, align) / BITS_PER_UNIT;

    value = TREE_VALUE (args);
    type  = TREE_TYPE  (value);
    assert (TREE_CODE(type)==INTEGER_TYPE);
    args = TREE_CHAIN (args);
    if (!int_val(value, &size))
        fatal("cannot determine size of type specified in %s", name);
  }

  /* size and align have been determined */

  if (size > 4) {
     fatal ("size of type in %s is > 4 bytes", name);
  }

  align = (align > size ? size : align);

  new_func = 0;

  /* test with align = size, size/2, size/4, ... */
  for (try_align = align; try_align; try_align /= 2) {
     sprintf (nbuf, "%s_size_%d_align_%d", name, size, try_align);
     if ((new_func=lookup_name(get_identifier(nbuf))) != 0)
       break;
  }

  if (new_func==0) {
     /* no matching function declaration - complain. */
     fatal ("%s does not expand to a known function", name);
  }

  TREE_OPERAND (TREE_OPERAND (exp, 0), 0) = new_func;
  return expand_expr (exp, target, VOIDmode, 0);
}
#endif /* IMSTG_UNALIGNED_MACROS */

#ifdef SELFHOST
void
__eprintf (const char* s1, const char* s2, unsigned i, const char* s3)
{
  fprintf (stderr, s1, s2, i, s3);
  abort();
}
#endif

int dumps_are_off;

int save_rtl_dump;
int save_jump_opt_dump;
int save_cse_dump;
int save_loop_dump;
int save_cse2_dump;
int save_flow_dump;
int save_combine_dump;
int save_sched_dump;
int save_local_reg_dump;
int save_global_reg_dump;
int save_sched2_dump;
int save_jump2_opt_dump;
int save_dbr_sched_dump;
int save_stack_reg_dump;
FILE *save_rtl_dump_file;
FILE *save_jump_opt_dump_file;
FILE *save_cse_dump_file;
FILE *save_loop_dump_file;
FILE *save_cse2_dump_file;
FILE *save_flow_dump_file;
FILE *save_combine_dump_file;
FILE *save_sched_dump_file;
FILE *save_local_reg_dump_file;
FILE *save_global_reg_dump_file;
FILE *save_sched2_dump_file;
FILE *save_jump2_opt_dump_file;
FILE *save_dbr_sched_dump_file;
FILE *save_stack_reg_dump_file;
FILE *save_coer_dump_file;
FILE *save_constprop_dump_file;
FILE *save_copyprop_dump_file;
FILE *save_dataflow_dump_file;
FILE *save_glob_inline_dump_file;
FILE *save_sblock_dump_file;
FILE *save_shadow_dump_file;
FILE *save_shadow_mem_dump_file;
FILE *save_imstg_pic_dump_file;
FILE *save_split_mem_dump_file;
FILE *save_marry_mem_dump_file;
FILE *save_dead_elim_dump_file;
FILE *save_cond_xform_dump_file;
FILE *save_bbr_dump_file;
FILE *save_dwarf_dump_file;

extern int rtl_dump;
extern int jump_opt_dump;
extern int cse_dump;
extern int loop_dump;
extern int cse2_dump;
extern int flow_dump;
extern int combine_dump;
extern int sched_dump;
extern int local_reg_dump;
extern int global_reg_dump;
extern int sched2_dump;
extern int jump2_opt_dump;
extern int dbr_sched_dump;
extern int stack_reg_dump;
extern FILE *rtl_dump_file;
extern FILE *jump_opt_dump_file;
extern FILE *cse_dump_file;
extern FILE *loop_dump_file;
extern FILE *cse2_dump_file;
extern FILE *flow_dump_file;
extern FILE *combine_dump_file;
extern FILE *sched_dump_file;
extern FILE *local_reg_dump_file;
extern FILE *global_reg_dump_file;
extern FILE *sched2_dump_file;
extern FILE *jump2_opt_dump_file;
extern FILE *dbr_sched_dump_file;
extern FILE *stack_reg_dump_file;
extern FILE *coer_dump_file;
extern FILE *constprop_dump_file;
extern FILE *copyprop_dump_file;
extern FILE *dataflow_dump_file;
extern FILE *glob_inline_dump_file;
extern FILE *sblock_dump_file;
extern FILE *shadow_dump_file;
extern FILE *shadow_mem_dump_file;
extern FILE *imstg_pic_dump_file;
extern FILE *split_mem_dump_file;
extern FILE *marry_mem_dump_file;
extern FILE *dead_elim_dump_file;
extern FILE *cond_xform_dump_file;
extern FILE *bbr_dump_file;
extern FILE *dwarf_dump_file;

turn_dumps_off()
{
  if (dumps_are_off == 0)
  {
    dumps_are_off = 1;

    save_rtl_dump = rtl_dump;
    save_jump_opt_dump = jump_opt_dump;
    save_cse_dump = cse_dump;
    save_loop_dump = loop_dump;
    save_cse2_dump = cse2_dump;
    save_flow_dump = flow_dump;
    save_combine_dump = combine_dump;
    save_sched_dump = sched_dump;
    save_local_reg_dump = local_reg_dump;
    save_global_reg_dump = global_reg_dump;
    save_sched2_dump = sched2_dump;
    save_jump2_opt_dump = jump2_opt_dump;
    save_dbr_sched_dump = dbr_sched_dump;
    save_stack_reg_dump = stack_reg_dump;
    save_rtl_dump_file = rtl_dump_file;
    save_jump_opt_dump_file = jump_opt_dump_file;
    save_cse_dump_file = cse_dump_file;
    save_loop_dump_file = loop_dump_file;
    save_cse2_dump_file = cse2_dump_file;
    save_flow_dump_file = flow_dump_file;
    save_combine_dump_file = combine_dump_file;
    save_sched_dump_file = sched_dump_file;
    save_local_reg_dump_file = local_reg_dump_file;
    save_global_reg_dump_file = global_reg_dump_file;
    save_sched2_dump_file = sched2_dump_file;
    save_jump2_opt_dump_file = jump2_opt_dump_file;
    save_dbr_sched_dump_file = dbr_sched_dump_file;
    save_stack_reg_dump_file = stack_reg_dump_file;
    save_coer_dump_file = coer_dump_file;
    save_constprop_dump_file = constprop_dump_file;
    save_copyprop_dump_file = copyprop_dump_file;
    save_dataflow_dump_file = dataflow_dump_file;
    save_glob_inline_dump_file = glob_inline_dump_file;
    save_sblock_dump_file = sblock_dump_file;
    save_shadow_dump_file = shadow_dump_file;
    save_shadow_mem_dump_file = shadow_mem_dump_file;
    save_imstg_pic_dump_file = imstg_pic_dump_file;
    save_split_mem_dump_file = split_mem_dump_file;
    save_marry_mem_dump_file = marry_mem_dump_file;
    save_dead_elim_dump_file = dead_elim_dump_file;
    save_cond_xform_dump_file = cond_xform_dump_file;
    save_bbr_dump_file = bbr_dump_file;
    save_dwarf_dump_file = dwarf_dump_file;

    rtl_dump = 0;
    jump_opt_dump = 0;
    cse_dump = 0;
    loop_dump = 0;
    cse2_dump = 0;
    flow_dump = 0;
    combine_dump = 0;
    sched_dump = 0;
    local_reg_dump = 0;
    global_reg_dump = 0;
    sched2_dump = 0;
    jump2_opt_dump = 0;
    dbr_sched_dump = 0;
    stack_reg_dump = 0;
    rtl_dump_file = 0;
    jump_opt_dump_file = 0;
    cse_dump_file = 0;
    loop_dump_file = 0;
    cse2_dump_file = 0;
    flow_dump_file = 0;
    combine_dump_file = 0;
    sched_dump_file = 0;
    local_reg_dump_file = 0;
    global_reg_dump_file = 0;
    sched2_dump_file = 0;
    jump2_opt_dump_file = 0;
    dbr_sched_dump_file = 0;
    stack_reg_dump_file = 0;
    coer_dump_file = 0;
    constprop_dump_file = 0;
    copyprop_dump_file = 0;
    dataflow_dump_file = 0;
    glob_inline_dump_file = 0;
    sblock_dump_file = 0;
    shadow_dump_file = 0;
    shadow_mem_dump_file = 0;
    imstg_pic_dump_file = 0;
    split_mem_dump_file = 0;
    marry_mem_dump_file = 0;
    dead_elim_dump_file = 0;
    cond_xform_dump_file = 0;
    bbr_dump_file = 0;
    dwarf_dump_file = 0;
  }
}

turn_dumps_on ()
{
  if (dumps_are_off == 1)
  {
    dumps_are_off = 0;

    rtl_dump = save_rtl_dump;
    jump_opt_dump = save_jump_opt_dump;
    cse_dump = save_cse_dump;
    loop_dump = save_loop_dump;
    cse2_dump = save_cse2_dump;
    flow_dump = save_flow_dump;
    combine_dump = save_combine_dump;
    sched_dump = save_sched_dump;
    local_reg_dump = save_local_reg_dump;
    global_reg_dump = save_global_reg_dump;
    sched2_dump = save_sched2_dump;
    jump2_opt_dump = save_jump2_opt_dump;
    dbr_sched_dump = save_dbr_sched_dump;
    stack_reg_dump = save_stack_reg_dump;
    rtl_dump_file = save_rtl_dump_file;
    jump_opt_dump_file = save_jump_opt_dump_file;
    cse_dump_file = save_cse_dump_file;
    loop_dump_file = save_loop_dump_file;
    cse2_dump_file = save_cse2_dump_file;
    flow_dump_file = save_flow_dump_file;
    combine_dump_file = save_combine_dump_file;
    sched_dump_file = save_sched_dump_file;
    local_reg_dump_file = save_local_reg_dump_file;
    global_reg_dump_file = save_global_reg_dump_file;
    sched2_dump_file = save_sched2_dump_file;
    jump2_opt_dump_file = save_jump2_opt_dump_file;
    dbr_sched_dump_file = save_dbr_sched_dump_file;
    stack_reg_dump_file = save_stack_reg_dump_file;
    coer_dump_file = save_coer_dump_file;
    constprop_dump_file = save_constprop_dump_file;
    copyprop_dump_file = save_copyprop_dump_file;
    dataflow_dump_file = save_dataflow_dump_file;
    glob_inline_dump_file = save_glob_inline_dump_file;
    sblock_dump_file = save_sblock_dump_file;
    shadow_dump_file = save_shadow_dump_file;
    shadow_mem_dump_file = save_shadow_mem_dump_file;
    imstg_pic_dump_file = save_imstg_pic_dump_file;
    split_mem_dump_file = save_split_mem_dump_file;
    marry_mem_dump_file = save_marry_mem_dump_file;
    dead_elim_dump_file = save_dead_elim_dump_file;
    cond_xform_dump_file = save_cond_xform_dump_file;
    bbr_dump_file = save_bbr_dump_file;
    dwarf_dump_file = save_dwarf_dump_file;

    save_rtl_dump = 0;
    save_jump_opt_dump = 0;
    save_cse_dump = 0;
    save_loop_dump = 0;
    save_cse2_dump = 0;
    save_flow_dump = 0;
    save_combine_dump = 0;
    save_sched_dump = 0;
    save_local_reg_dump = 0;
    save_global_reg_dump = 0;
    save_sched2_dump = 0;
    save_jump2_opt_dump = 0;
    save_dbr_sched_dump = 0;
    save_stack_reg_dump = 0;
    save_rtl_dump_file = 0;
    save_jump_opt_dump_file = 0;
    save_cse_dump_file = 0;
    save_loop_dump_file = 0;
    save_cse2_dump_file = 0;
    save_flow_dump_file = 0;
    save_combine_dump_file = 0;
    save_sched_dump_file = 0;
    save_local_reg_dump_file = 0;
    save_global_reg_dump_file = 0;
    save_sched2_dump_file = 0;
    save_jump2_opt_dump_file = 0;
    save_dbr_sched_dump_file = 0;
    save_stack_reg_dump_file = 0;
    save_coer_dump_file = 0;
    save_constprop_dump_file = 0;
    save_copyprop_dump_file = 0;
    save_dataflow_dump_file = 0;
    save_glob_inline_dump_file = 0;
    save_sblock_dump_file = 0;
    save_shadow_dump_file = 0;
    save_shadow_mem_dump_file = 0;
    save_imstg_pic_dump_file = 0;
    save_split_mem_dump_file = 0;
    save_marry_mem_dump_file = 0;
    save_dead_elim_dump_file = 0;
    save_cond_xform_dump_file = 0;
    save_bbr_dump_file = 0;
    save_dwarf_dump_file = 0;
  }
}


#endif
