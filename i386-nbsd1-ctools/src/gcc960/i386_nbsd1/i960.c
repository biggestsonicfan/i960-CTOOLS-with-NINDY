/* Subroutines used for code generation on intel 80960.
   Copyright (C) 1992 Free Software Foundation, Inc.
   Contributed by Steven McGeady, Intel Corp.
   Additional Work by Glenn Colon-Bonet, Jonathan Shapiro, Andy Wilson
   Converted to GCC 2.0 by Jim Wilson and Michael Tiemann, Cygnus Support.

This file is part of GNU CC.

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
#include <string.h>
#include <ctype.h>

#include "config.h"
#include "obstack.h"
#include "rtl.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "real.h"
#include "insn-config.h"
#include "conditions.h"
#include "insn-flags.h"
#include "output.h"
#include "insn-attr.h"
#include "flags.h"
#include "tree.h"
#include "insn-codes.h"
#include "assert.h"
#include "expr.h"
#include "function.h"
#include "recog.h"
#include "coff.h"
#include "stab.h"
#include "i_double.h"
#include "i_dataflow.h"
#include "i_dwarf2.h"
#include <math.h>
#include "input.h"

extern tree long_double_type_node;
extern tree signed_char_type_node;
extern tree short_integer_type_node;
extern tree unsigned_char_type_node;
extern tree short_unsigned_type_node;

#if defined(IMSTG_REAL) && (HOST_FLOAT_WORDS_BIG_ENDIAN != 0)
oops - HOST_FLOAT_WORDS_BIG_ENDIAN is supposed to be 0 in tm.h
#endif

#if defined(IMSTG_REAL) && (HOST_FLOAT_WORDS_BIG_ENDIAN == 0)
#define TW0 0
#define TW1 1
#define TW2 2
#define DW0 0
#define DW1 1
#else
#define TW0 2
#define TW1 1
#define TW2 0
#define DW0 1
#define DW1 0
#endif

extern struct obstack permanent_obstack;

/* align functions on 16 byte boundaries by default. */
unsigned i960_function_boundary = 132;

char i960_br_start_char;

static rtx cave_end_label;
static int in_cave_function_body = 0;

static int i960_integrate_const = 8;

/* Save the operands last given to a compare for use when we
   generate a scc or bcc insn.  */

rtx i960_compare_op0, i960_compare_op1;

/*
 * Set this flag if we generate any code that could change g14 from 0,
 * within this function.
 */
static int i960_func_changes_g14 = 0;

/*
 * This flag tells whether its OK to adjust the legal address.  Set to false
 * when either nocompress or compress is on.
 */
static int i960_ok_to_adjust;

/*
 * Used to implement #pragma align/noalign.  Initialized by
 * OVERRIDE_OPTIONS macro in i960.h.
 */
static int i960_maxbitalignment;
static int i960_last_maxbitalignment;

/* This variable is referenced via the macro IMSTG_PRAGMA_SECTION_SEEN. */
int i960_pragma_section_seen = 0;

/*
 * These two variables are used to implement pragma pack.
 */
static int i960_last_max_member_bit_align;
static int i960_max_member_bit_align;

/*
 * This structure and the following variables are used to
 * implement pragma i960_align.  Also used to implement i960_align
 * is a symbol table that comes later.
 */
struct i960_align_info {
  unsigned short used_i960_align;
  unsigned short align_value_used;
};

char * i960_align_option;
static int i960_n_struct_align_info;
static struct i960_align_info *i960_struct_align_info;
static int i960_align_top;

/* Used to implement switching between MEM and ALU insn types, for better
   C series performance.  */

enum insn_types i960_last_insn_type;

int i960_wait_states = 2;  /* default for now, may change later with switch */
int i960_burst_mem = 1;    /* assume burstable memory */

/* Where to save/restore register 14 to/from before/after a procedure call
   when it holds an argument block pointer.  */

static rtx g14_save_reg;

/* The leaf-procedure return register.  Set only if this is a leaf routine.  */

static int i960_leaf_ret_reg    = -1;
static int i960_func_has_calls  = -1;
int i960_func_passes_on_stack;

/* i960_declared_fname is the name parameter passed into
   i960_function_name_declare.
   This name is used to emit the .elf_size directive.
   If CAVE is enabled, we emit the function's .elf_size in the cave stub.
   Otherwise, we emit it in i960_function_epilogue().
 */
static char *i960_declared_fname;
static int  elf_end_func_label_ctr = 0;

#define i960_emit_elf_function_size(FILE) \
  do { \
    if (flag_elf && i960_declared_fname) { \
      elf_end_func_label_ctr++; \
      fprintf ((FILE), "LE%d:\n", elf_end_func_label_ctr); \
      fprintf ((FILE), "\t.elf_size\t_%s, LE%d - _%s\n", \
	i960_declared_fname, elf_end_func_label_ctr, i960_declared_fname); \
    } \
  } while (0)

/* True if replacing tail calls with jumps is OK.  */

static int tail_call_ok;

/* A string containing a list of insns to emit in the epilogue so as to
   restore all registers saved by the prologue.  Created by the prologue
   code as it saves registers away.
   The abstract_epilogue_string is potentially parameterized with %L, so that
   each epilogue instance can contain unique labels which are referenced
   from DWARF's .debug_frame section.
 */

char abstract_epilogue_string[1000];
char epilogue_string[1000];
char prologue_string[1000];

static char*
i960_epilogue_string()
{
#if defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2
  if (write_symbols == DWARF_DEBUG && debug_info_level >= DINFO_LEVEL_NORMAL
      && !TARGET_CAVE)
  {
    /* Copy abstract_epilogue_string to epilogue_string, replacing
       instances of %L with a label.
     */
    char *src, *dst;
    int  lnum;
    char code_label[MAX_ARTIFICIAL_LABEL_BYTES];

    lnum = dwarf_fde.prologue_label_num + 1 + 3*dwarf_fde.num_epilogues_emitted;
    src = abstract_epilogue_string;
    dst = epilogue_string;

    while (*src)
    {
      if (*src == '%' && *(src+1) == 'L')
      {
	ASM_GENERATE_INTERNAL_LABEL(code_label,DWARF_FDE_CODE_LABEL_CLASS,lnum);
	lnum++;
        /* HACK: GENERATE_INTERNAL_LABEL puts a * at the start of the label */
        if (code_label[0] == '*')
          sprintf (dst, "%s:\n", code_label+1);
        else
          sprintf (dst, "%s:\n", code_label);

        dwarf_fde_code_label_counter++;
	src += 2;
	dst += strlen(dst);
      }
      else
        *dst++ = *src++;
    }

    *dst = '\0';

    dwarf_fde.num_epilogues_emitted++;
    return epilogue_string;
  }
  else
#endif
    /* In this case, abstract_epilogue_string can be used as is. */
    return abstract_epilogue_string;
}

/* A unique number (per function) for return labels.  */

static int i960_leaf_ret_label = 0;

/* flag letting us know if we are processing and interrupt or isr routine. */
static int i960_in_isr;

extern char * i960_disect_address();

#define UNSET_SYSCALL		-3
#define NOT_SYSCALL		-2
#define NO_SYSCALL_INDEX	-1

#define PRAG_ID_SYSTEM		1
#define PRAG_ID_ISR		2
#define PRAG_ID_INLINE		3
#define PRAG_ID_INTERRUPT	4
#define PRAG_ID_PURE		5
#define PRAG_ID_COMPRESS	6
#define PRAG_ID_OPTIMIZE	7
#define PRAG_ID_CAVE		8

/* flags to say whether we have global compress pragma in effect. */
static int i960_compress_pragma_glob = PRAGMA_UNSET;

/* flags to say whether we have global inline pragma in effect. */
static int i960_inline_pragma_glob = PRAGMA_UNSET;

/* flags to say whether we have global pure pragma in effect. */
static int i960_pure_pragma_glob = PRAGMA_UNSET;

/* flags to say whether we have global interrupt pragma in effect. */
static int i960_interrupt_pragma_glob = PRAGMA_UNSET;

/* flags to say whether we have a global pragma cave in effect. */
static int i960_cave_pragma_glob = PRAGMA_UNSET;

/* flags to say whether we have global system pragma in effect. */
static int i960_system_pragma_glob = UNSET_SYSCALL;

/* string to indicate whether we have a global #pragma optimize */
static char * i960_opt_string;

/* string to use to concatenate to section names. */
static char * i960_section_string;

static void interpret_opt_string();

struct sym_pragma_info
{
  /* This stuff is for managing, building the the symbol table */
  struct sym_pragma_info *next;
  char *sym_name;

  /* This stuff is the pragma info for the symbol */
  short         syscall_num;
  unsigned char sysproc_out;
  unsigned char i960_align;
  unsigned char i960_align_set;
  unsigned char isr_flag;
  unsigned char inline_flag;
  unsigned char interrupt_flag;
  unsigned char cave_flag;
  unsigned char pure_flag;
  unsigned char compress_flag;
  char *        optimize_string;
};

struct sym_pragma_info *prag_tab[0x1000];

struct sym_pragma_info * look_sym_pragma(name, add)
unsigned char *name;
int add;
{
  /* hash is just xor of first two chars */
  int hash = 0;
  int i;
  int t;
  struct sym_pragma_info *p;

  for (i = 0; (t = name[i]) != 0; i++)
    hash = ((hash + t) ^ (~t << 1)) & 0xFFF;

  p = prag_tab[hash];
  while (p != 0)
  {
    if (strcmp(p->sym_name, (char *)name) == 0)
      return p;
    p = p->next;
  }

  /*
   * didn't find it, add it or return 0.
   */
  if (add)
  {
    p = (struct sym_pragma_info *)
        obstack_alloc(&permanent_obstack,
                      sizeof(struct sym_pragma_info)+strlen((char *)name)+1);

    p->next = prag_tab[hash];
    prag_tab[hash] = p;

    p->sym_name = (char *)(p + 1);  /* first byte following the struct */
    strcpy (p->sym_name, (char *)name);

    p->syscall_num = UNSET_SYSCALL;
    p->sysproc_out = 0;
    p->i960_align = 0;
    p->i960_align_set = 0;
    p->isr_flag = PRAGMA_UNSET;
    p->inline_flag = PRAGMA_UNSET;
    p->interrupt_flag = PRAGMA_UNSET;
    p->cave_flag = PRAGMA_UNSET;
    p->pure_flag = PRAGMA_UNSET;
    p->compress_flag = PRAGMA_UNSET;
    p->optimize_string = 0;
  }

  return p;
}

static void
process_pragma_i960_align(name, n, prag_name)
char *name;
int n;
char *prag_name;
{
  struct sym_pragma_info * p;

  if (name == 0)
  {
    i960_struct_align_info[0].used_i960_align = 1;
    i960_struct_align_info[0].align_value_used = n*8; 
    return;
  }

  p = look_sym_pragma(name, 1);

  if (p->i960_align_set && p->i960_align != n*8)
  {
    warning ("pragma %s tries to redefine alignment for struct tag \"%s\" - redefinition ignored.",
             prag_name, name);
    return;
  }

  p->i960_align_set = 1;
  p->i960_align = n*8;
}

static void
process_pragma_isr(name, flag, prag_name)
char *name;
{
  struct sym_pragma_info * p;
  p = look_sym_pragma(name, 1);

  if (p->isr_flag != PRAGMA_UNSET && p->isr_flag != flag)
  {
    warning ("#pragma %s attempts redefinition of %s - pragma ignored.",
              prag_name, name);
    return;
  }

  if (flag == PRAGMA_DO && p->syscall_num > NOT_SYSCALL)
    warning ("function %s is used in both #pragma isr and #pragma system.",
             name);

  p->isr_flag = flag;
}

static void
process_pragma_system(name, sys_index, prag_name)
char *name;
int sys_index;
char *prag_name;
{
  struct sym_pragma_info * p;

  if (name == 0)
  {
    i960_system_pragma_glob = sys_index;
    return;
  }

  p = look_sym_pragma(name, 1);

  if (p->syscall_num != UNSET_SYSCALL && p->syscall_num != sys_index)
  {
    warning ("#pragma %s tries redefinition for function %s - redefinition ignored.",
              prag_name, name);
    return;
  }

  if (p->isr_flag == PRAGMA_DO && sys_index > NOT_SYSCALL)
    warning ("function %s is used in both #pragma isr and #pragma system.",
             name);

  p->syscall_num = sys_index;
}

static void
process_pragma_inline(name, flag, prag_name)
char *name;
int flag;
char *prag_name;
{
  struct sym_pragma_info * p;

  if (name == 0)
  {
    i960_inline_pragma_glob = flag;
    if (flag == PRAGMA_NO)
    {
      flag_no_inline = 1;
      flag_inline_functions = 0;
    }
    else
    {
      flag_no_inline = 0;
      flag_inline_functions = 1;
    }
    return;
  }

  p = look_sym_pragma(name, 1);

  if (p->inline_flag != PRAGMA_UNSET && p->inline_flag != flag)
  {
    warning ("#pragma %s attempts redefinition of %s - pragma ignored.",
              prag_name, name);
    return;
  }

  p->inline_flag = flag;
}

static void
process_pragma_cave(name, flag, prag_name)
char *name;
int flag;
char *prag_name;
{
  struct sym_pragma_info * p;

  if (name == 0)
  {
    i960_cave_pragma_glob = flag;
    return;
  }

  p = look_sym_pragma(name, 1);

  if (p->cave_flag != PRAGMA_UNSET && p->cave_flag != flag)
  {
    warning ("#pragma %s attempts redefinition of %s - pragma ignored.",
              prag_name, name);
    return;
  }

  p->cave_flag = flag;
}

static void
process_pragma_interrupt(name, flag, prag_name)
char *name;
int flag;
char *prag_name;
{
  struct sym_pragma_info * p;

  if (name == 0)
  {
    i960_interrupt_pragma_glob = flag;
    return;
  }

  p = look_sym_pragma(name, 1);

  if (p->interrupt_flag != PRAGMA_UNSET && p->interrupt_flag != flag)
  {
    warning ("#pragma %s attempts redefinition of %s - pragma ignored.",
              prag_name, name);
    return;
  }

  p->interrupt_flag = flag;
}

static void
process_pragma_pure(name, flag, prag_name)
char *name;
int flag;
char *prag_name;
{
  struct sym_pragma_info * p;

  if (name == 0)
  {
    i960_pure_pragma_glob = flag;
    return;
  }

  p = look_sym_pragma(name, 1);

  if (p->pure_flag != PRAGMA_UNSET && p->pure_flag != flag)
  {
    warning ("#pragma %s attempts redefinition of %s - pragma ignored.",
              prag_name, name);
    return;
  }

  p->pure_flag = flag;
}

static void
process_pragma_compress(name, flag, prag_name)
char *name;
int flag;
char *prag_name;
{
  struct sym_pragma_info * p;

  if (name == 0)
  {
    /* set a global flag that changes the sense of compress. */
    i960_compress_pragma_glob = flag;
    return;
  }

  p = look_sym_pragma(name, 1);

  if (p->compress_flag != PRAGMA_UNSET && p->compress_flag != flag)
  {
    warning ("#pragma %s attempts redefinition of %s - pragma ignored.",
              prag_name, name);
    return;
  }

  p->compress_flag = flag;
}

struct pragma_optimize_option {
	char	*name;
	unsigned mask;
} pragma_optimize_options[] = {
	{ "lp",		TARGET_FLAG_LEAFPROC },
	{ "tce",	TARGET_FLAG_TAILCALL },
	{ NULL,		0 }
};

static void
process_pragma_optimize(name, opt_string, prag_name)
char *name;
char *opt_string;
char *prag_name;
{
  struct sym_pragma_info * p;

  /* Give a warning for unrecognized options */
  if (opt_string && *opt_string)
    interpret_opt_string(opt_string, 1);

  if (name == 0)
  {
    /* set a global flag that changes the sense of optimize. */
    i960_opt_string = xrealloc(i960_opt_string, strlen(opt_string)+1);
    strcpy(i960_opt_string, opt_string);
    return ;
  }

  p = look_sym_pragma(name, 1);

  if (p->optimize_string != 0 && strcmp(p->optimize_string, opt_string) != 0)
  {
    warning ("#pragma %s attempts redefinition of %s - pragma ignored.",
              prag_name, name);
    return;
  }

  p->optimize_string = xrealloc(p->optimize_string, strlen(opt_string)+1);
  strcpy(p->optimize_string, opt_string);
}

int
i960_pragma_inline(decl)
tree decl;
{
  struct sym_pragma_info * p;
  if (DECL_NAME(decl) != 0 &&
      IDENTIFIER_POINTER(DECL_NAME(decl)) != 0 &&
      (p = look_sym_pragma(IDENTIFIER_POINTER(DECL_NAME(decl)), 0)) != 0 &&
      p->inline_flag != PRAGMA_UNSET)
    return p->inline_flag;

  return i960_inline_pragma_glob;
}

int
i960_pragma_pure(decl)
tree decl;
{
  struct sym_pragma_info * p;
  if (DECL_NAME(decl) != 0 &&
      IDENTIFIER_POINTER(DECL_NAME(decl)) != 0 &&
      (p = look_sym_pragma(IDENTIFIER_POINTER(DECL_NAME(decl)), 0)) != 0 &&
      p->pure_flag != PRAGMA_UNSET)
    return p->pure_flag;

  return i960_pure_pragma_glob;
}

int
i960_pragma_isr(decl)
tree decl;
{
  struct sym_pragma_info * p;
  if (DECL_NAME(decl) != 0 &&
      IDENTIFIER_POINTER(DECL_NAME(decl)) != 0 &&
      (p = look_sym_pragma(IDENTIFIER_POINTER(DECL_NAME(decl)), 0)) != 0 &&
      p->isr_flag != PRAGMA_UNSET)
    return p->isr_flag;

  return PRAGMA_UNSET;
}

int
i960_pragma_interrupt(decl)
tree decl;
{
  struct sym_pragma_info * p;
  if (DECL_NAME(decl) != 0 &&
      IDENTIFIER_POINTER(DECL_NAME(decl)) != 0 &&
      (p = look_sym_pragma(IDENTIFIER_POINTER(DECL_NAME(decl)), 0)) != 0 &&
      p->interrupt_flag != PRAGMA_UNSET)
    return p->interrupt_flag;

  return i960_interrupt_pragma_glob;
}

int
i960_pragma_cave(decl)
tree decl;
{
  struct sym_pragma_info * p;
  if (DECL_NAME(decl) != 0 &&
      IDENTIFIER_POINTER(DECL_NAME(decl)) != 0 &&
      (p = look_sym_pragma(IDENTIFIER_POINTER(DECL_NAME(decl)), 0)) != 0 &&
      p->cave_flag != PRAGMA_UNSET)
    return p->cave_flag;

  return i960_cave_pragma_glob;
}

int
i960_pragma_compress(decl)
tree decl;
{
  struct sym_pragma_info * p;
  if (DECL_NAME(decl) != 0 &&
      IDENTIFIER_POINTER(DECL_NAME(decl)) != 0 &&
      (p = look_sym_pragma(IDENTIFIER_POINTER(DECL_NAME(decl)), 0)) != 0 &&
      p->compress_flag != PRAGMA_UNSET)
    return p->compress_flag;

  return i960_compress_pragma_glob;
}

char *
i960_pragma_optimize(decl)
tree decl;
{
  struct sym_pragma_info * p;
  if (DECL_NAME(decl) != 0 &&
      IDENTIFIER_POINTER(DECL_NAME(decl)) != 0 &&
      (p = look_sym_pragma(IDENTIFIER_POINTER(DECL_NAME(decl)), 0)) != 0 &&
      p->optimize_string != 0)
    return p->optimize_string;

  return i960_opt_string;
}

int
i960_pragma_system(name, sysproc_out_p)
char *name;
unsigned char **sysproc_out_p;
{
  struct sym_pragma_info * p;
  static unsigned char dummy;

  if ((p = look_sym_pragma(name, 0)) != 0 &&
      p->syscall_num != UNSET_SYSCALL)
  {
    *sysproc_out_p = &p->sysproc_out;
    return p->syscall_num;
  }

  dummy = 0;
  *sysproc_out_p = &dummy;
  return i960_system_pragma_glob;
}

int
i960_interrupt_handler()
{
  return (i960_pragma_isr(current_function_decl) == PRAGMA_DO ||
          i960_pragma_interrupt(current_function_decl) == PRAGMA_DO);
}

int
i960_system_function(decl)
tree decl;
{
  if (decl != 0 && DECL_NAME(decl) != 0 &&
      IDENTIFIER_POINTER(DECL_NAME(decl)) != 0)
  {
    int syscall_num;
    unsigned char *sysproc_out_p;
    syscall_num = i960_pragma_system(IDENTIFIER_POINTER(DECL_NAME(decl)),
                                     &sysproc_out_p);

    return syscall_num > NOT_SYSCALL;
  }

  return 0;
}

static void
i960_out_sysproc(file, name, syscall_num, sysproc_out_p)
FILE *file;
char *name;
int syscall_num;
unsigned char *sysproc_out_p;
{
  if (syscall_num > NOT_SYSCALL && !*sysproc_out_p)
  {
    if (syscall_num != NO_SYSCALL_INDEX)
    {
      if (syscall_num <= 253 || !flag_bout)
        fprintf(file, "\t.sysproc\t_%s,%d\n", name, syscall_num);
    }
    else
      fprintf(file, "\t.sysproc\t_%s\n", name);
    *sysproc_out_p = 1;
  }
}

/* Initialize variables before compiling any files.  */

void
i960_initialize ()
{
  i960_n_struct_align_info = 100;
  i960_struct_align_info =
      (struct i960_align_info *)xmalloc(i960_n_struct_align_info *
                                        sizeof(struct i960_align_info));
  i960_align_top = 0;
  i960_struct_align_info[0].used_i960_align = 0;
  i960_struct_align_info[0].align_value_used = 128;

  if (i960_align_option != 0)
  {
    int n_align = atoi(i960_align_option);

    switch (n_align)
    {
      case 1:
      case 2:
      case 4:
      case 8:
      case 16:
        i960_struct_align_info[0].used_i960_align = 1;
        i960_struct_align_info[0].align_value_used = n_align * 8;
        break;
        
      default:
        warning ("invalid alignment \"%s\" in -mi960_align - ignored.",
                 i960_align_option);
        break;
    }
  }

  if (TARGET_IC_COMPAT2_0)
    {
      i960_maxbitalignment = 8;
      i960_last_maxbitalignment = 128;
    }
  else if (TARGET_ABI)
    {
      i960_maxbitalignment = 32;
      i960_last_maxbitalignment = 32;
    }
  else
    {
      i960_maxbitalignment = 128;
      i960_last_maxbitalignment = 8;
    }
}

char* driver_command_line = NULL;

i960_asm_file_start(file)
FILE *file;
{
  int i;
  int n_chars;
  extern int save_argc;
  extern char **save_argv;
  fputs("\n\n", file);

  if (driver_command_line)
    fprintf(file,"\t# Command line %s: %s\n",
		flag_ic960 ? " (ic960)" : "(gcc960)", driver_command_line);

  fprintf(file,"\t# Command line    (cc1):");
  n_chars = 84;
  for (i=0; i<save_argc; i++)
  {
    int has_space = ((strchr(save_argv[i],' ') != NULL) ||
                     (strchr(save_argv[i],'\t') != NULL));
    int argv_length;

    argv_length = strlen(save_argv[i]) + 1 + (4 * has_space);

    if ((n_chars + argv_length) > 80)
    {
      fprintf (file, "\n\t# ");
      n_chars = 10;
    }

    if (has_space)
      fprintf(file," \"%s\"", save_argv[i]);
    else
      fprintf(file," %s", save_argv[i]);

    n_chars += argv_length;
  }
  fprintf(file,"\n");

  output_file_directive(file, main_input_filename);

  if (write_symbols == SDB_DEBUG)
    fprintf (file, "ic_name_rules.:\n");

  if (TARGET_PIC)
    fprintf (file, "\t.pic\n");
  if (TARGET_PID)
    fprintf (file, "\t.pid\n");
  if (TARGET_PID_SAFE)
    fprintf (file, "\t.link_pix\n");

  if (TARGET_ASM_COMPAT)
    i960_br_start_char = 'j';
  else
    i960_br_start_char = 'b';

  if (!flag_bout && flag_use_lomem)
    output_lomem(file);
}

/* return true if op can be used as either the source or the destination
 * of a move.  This is essentially general_operand, except I want to
 * allow volatile memory operands always, which general operand does
 * not.  Using general operand keeps combine from doing a combine that
 * affects the addressing mode of a volatile operand for no good reason.
 */
int
move_operand(op, mode)
rtx op;
enum machine_mode mode;
{
  if (general_operand(op, mode))
    return 1;

  if (GET_CODE(op) == MEM && MEM_VOLATILE_P(op))
    return memory_address_p(GET_MODE(op), XEXP(op,0));

  return 0;
}

/* Return true if OP can be used as the source of an fp move insn.  */
int
fpmove_src_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  return (GET_CODE (op) == CONST_DOUBLE || general_operand (op, mode));
}

#if 0
/* Return true if OP is a register or zero.  */

int
reg_or_zero_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  return register_operand (op, mode) || op == const0_rtx;
}
#endif

/* Return truth value of whether OP can be used as an operands in a three
   address arithmetic insn (such as add %o1,7,%l2) of mode MODE.  */

int
arith_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  return (register_operand (op, mode) || literal (op, mode));
}

/* Return true if OP is a register or a valid floating point literal.  */

int
fp_arith_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  return (register_operand (op, mode) || fp_literal (op, mode));
}

/* Return true is OP is a register or a valid signed integer literal.  */

int
signed_arith_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  return (register_operand (op, mode) || signed_literal (op, mode));
}

/* Return truth value of whether OP is a integer which fits the
   range constraining immediate operands in three-address insns.  */

int
literal (op, mode)
     rtx op;
     enum machine_mode mode;
{
  return ((GET_CODE (op) == CONST_INT) && INTVAL(op) >= 0 && INTVAL(op) < 32);
}

/* Return true if OP is a float constant of 1.  */

int
fp_literal_one (op, mode)
     rtx op;
     enum machine_mode mode;
{
  if (mode==VOIDmode)
    mode=GET_MODE(op);

  if (!(TARGET_NUMERICS) || (mode!=GET_MODE(op)) ||
      (GET_MODE_CLASS(mode) != MODE_FLOAT))
    return 0;

  return (op==CONST1_RTX(mode));
}

/* Return true if OP is a float constant of 0.  */

int
fp_literal_zero (op, mode)
     rtx op;
     enum machine_mode mode;
{
  if (mode==VOIDmode)
    mode=GET_MODE(op);

  if (!(TARGET_NUMERICS) || (mode!=GET_MODE(op)) ||
      (GET_MODE_CLASS(mode) != MODE_FLOAT))
    return 0;

  return (op==CONST0_RTX(mode));
}

/* Return true if OP is a valid floating point literal.  */

int
fp_literal(op, mode)
     rtx op;
     enum machine_mode mode;
{
  return fp_literal_zero (op, mode) || fp_literal_one (op, mode);
}

/* Return true if OP is a valid signed immediate constant.  */

int
signed_literal(op, mode)
     rtx op;
     enum machine_mode mode;
{
  return ((GET_CODE (op) == CONST_INT) && INTVAL(op) > -32 && INTVAL(op) < 32);
}

/* Return truth value of statement that OP is a symbolic memory
   operand of mode MODE.  */

int
symbolic_memory_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  if (GET_CODE (op) == SUBREG)
    op = SUBREG_REG (op);
  if (GET_CODE (op) != MEM)
    return 0;
  op = XEXP (op, 0);
  return (GET_CODE (op) == SYMBOL_REF || GET_CODE (op) == CONST
	  || GET_CODE (op) == HIGH || GET_CODE (op) == LABEL_REF);
}

/* Return truth value of whether OP is EQ or NE.  */

int
eq_or_neq (op, mode)
     rtx op;
     enum machine_mode mode;
{
  return (GET_CODE (op) == EQ || GET_CODE (op) == NE);
}

/* OP is an integer register or a constant.  */

int
arith32_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  if (register_operand (op, mode))
    return 1;
  return (CONSTANT_P (op));
}

/* Return true if OP is an integer constant which is a power of 2.  */

int
power2_operand (op,mode)
     rtx op;
     enum machine_mode mode;
{
  if (GET_CODE(op) != CONST_INT)
    return 0;

  return exact_log2 (INTVAL (op)) >= 0;
}

/*
 * Return true if OP is an integer constant which is
 * the ones complement of a power of 2. 
 */
int
inv_power2_operand (op,mode)
     rtx op;
     enum machine_mode mode;
{
  int t;
  if (GET_CODE(op) != CONST_INT)
    return 0;

  return exact_log2 (~INTVAL (op)) >= 0;
}

/* If VAL has only one bit set, return the index of that bit.  Otherwise
   return -1.  */

int
bitpos (val)
     unsigned int val;
{
  register int i;

  for (i = 0; val != 0; i++, val >>= 1)
    {
      if (val & 1)
	{
	  if (val != 1)
	    return -1;
	  return i;
	}
    }
  return -1;
}

/* Return non-zero if OP is a mask, i.e. all one bits are consecutive.
   The return value indicates how many consecutive non-zero bits exist
   if this is a mask.  This is the same as the next function, except that
   it does not indicate what the start and stop bit positions are.  */

int
is_mask (val)
     unsigned int val;
{
  register int start, end, i;

  start = -1;
  for (i = 0; val != 0; val >>= 1, i++)
    {
      if (val & 1)
	{
	  if (start < 0)
	    start = i;

	  end = i;
	  continue;
	}
      /* Still looking for the first bit.  */
      if (start < 0)
	continue;

      /* We've seen the start of a bit sequence, and now a zero.  There
	 must be more one bits, otherwise we would have exited the loop.
	 Therefore, it is not a mask.  */
      if (val)
	return 0;
    }

  /* The bit string has ones from START to END bit positions only.  */
  return end - start + 1;
}

/* If VAL is a mask, then return nonzero, with S set to the starting bit
   position and E set to the ending bit position of the mask.  The return
   value indicates how many consecutive bits exist in the mask.  This is
   the same as the previous function, except that it also indicates the
   start and end bit positions of the mask.  */

int
bitstr (val, s, e)
     unsigned int val;
     int *s, *e;
{
  register int start, end, i;

  start = -1;
  end = -1;
  for (i = 0; val != 0; val >>= 1, i++)
    {
      if (val & 1)
	{
	  if (start < 0)
	    start = i;

	  end = i;
	  continue;
	}

      /* Still looking for the first bit.  */
      if (start < 0)
	continue;

      /* We've seen the start of a bit sequence, and now a zero.  There
	 must be more one bits, otherwise we would have exited the loop.
	 Therefor, it is not a mask.  */
      if (val)
	{
	  start = -1;
	  end = -1;
	  break;
	}
    }

  /* The bit string has ones from START to END bit positions only.  */
  *s = start;
  *e = end;
  return ((start < 0) ? 0 : end - start + 1);
}

/* Return the machine mode to use for a comparison.  */

enum machine_mode
select_cc_mode (op, x, y)
     RTX_CODE op;
     rtx x;
     rtx y;
{
  if (op == GTU || op == LTU || op == GEU || op == LEU)
    return CC_UNSmode;
  return CCmode;
}

/* X and Y are two things to compare using CODE.  Emit the compare insn and
   return the rtx for register 36 in the proper mode.  */

rtx
gen_compare_reg (code, x, y)
     enum rtx_code code;
     rtx x, y;
{
  rtx cc_reg;
  enum machine_mode mode, ccmode;
  int unsignedp;

  ccmode = SELECT_CC_MODE (code, x, y);
  mode = GET_MODE (x) == VOIDmode ? GET_MODE (y) : GET_MODE (x);

  unsignedp = (code == EQ || code == NE ||
               code == LTU || code == LEU ||
               code == GTU || code == GEU);

  if (mode == SImode || mode == HImode || mode == QImode)
    {
      int lo, hi;

      x = embed_type (x, type_for_size (GET_MODE_BITSIZE(mode), unsignedp));
      y = embed_type (y, type_for_size (GET_MODE_BITSIZE(mode), unsignedp));

      if (mode != SImode && memory_operand(x,VOIDmode) &&
                            memory_operand(y,VOIDmode))
      {
        x = convert_to_mode(SImode, x, unsignedp);
        y = convert_to_mode(SImode, y, unsignedp);
        mode = SImode;
      }

      if (mode!=SImode && memory_operand(x,VOIDmode) && GET_CODE(y)==CONST_INT)
      {
        x = convert_to_mode(SImode, x, unsignedp);

        get_cnst_val(y, mode, &lo, &hi, (unsignedp ? 1 : -1));
        y = immed_double_const(lo, hi, SImode);
        mode = SImode;
      }

      if (mode!=SImode && GET_CODE(x)==CONST_INT && memory_operand(y,VOIDmode))
      {
        y = convert_to_mode(SImode, y, unsignedp);

        get_cnst_val(x, mode, &lo, &hi, (unsignedp ? 1 : -1));
        x = immed_double_const(lo, hi, SImode);
        mode = SImode;
      }

      if (! register_operand (x, mode))
	x = force_reg (mode, x);
      if (! nonmemory_operand (y, mode))
	y = force_reg (mode, y);
    }

  cc_reg = gen_rtx (REG, ccmode, 36);
  set_rtx_type (cc_reg, integer_type_node);
  emit_insn (gen_rtx (SET, VOIDmode, cc_reg,
		      gen_rtx (COMPARE, ccmode, x, y)));

  return cc_reg;
}

rtx
i960_find_cc_user(s_insn)
register rtx s_insn;
{
  /*
   * insn s_insn sets the condition code register - reg:cc 36.
   * Return the insn which uses this register.  If we call or jump
   * or run off the basic block without finding the user, return 0.
   */

  register enum rtx_code c;
  register rtx  t_insn;
  register rtx  cc_rtx = gen_rtx(REG, VOIDmode, 36);

  t_insn = NEXT_INSN (s_insn);
  while (1)
  {
    if (t_insn == 0)
      return (0);

    if (GET_RTX_CLASS(c = GET_CODE(t_insn)) == 'i' &&
        reg_mentioned_p(cc_rtx, PATTERN(t_insn)))
      return (t_insn);

    if (c != INSN && c != NOTE)
      return (0);

    t_insn = NEXT_INSN (t_insn);
  }
  /* Never reached */
}


/*
 * Return true if the only user's of the condition resulting from
 * the comparison instruction "s_insn" is an equal or not equal.  This
 * is simplified by assuming that either all the uses are in the same
 * basic-block, or if they aren't this routine simply returns false.
 */
int
i960_cc_user_eq_neq_p(s_insn)
rtx s_insn;
{
  /*
   * We look at all user's in the basic-blk until we find a dead note,
   * run-off the end of the basic-block, or find a user that is not
   * an eq or neq.
   */
  while (1)
  {
    rtx cc_user;
    rtx cond;

    cc_user = i960_find_cc_user(s_insn);
    /* did we find a user?  if not return 0 */
    if (cc_user == 0)
      return 0;

    /* try to find condition */
    if (GET_RTX_CLASS(GET_CODE(cc_user)) != 'i')
      return 0;

    if (GET_CODE(PATTERN(cc_user)) != SET)
      return 0;

    cond = SET_SRC(PATTERN(cc_user));

    if (GET_CODE(cond) == IF_THEN_ELSE)
      cond = XEXP(cond,0);

    if (GET_CODE(cond) != EQ && GET_CODE(cond) != NE)
      return 0;

    /* Is the condition code register dead after the user?
     * If it isn't keep looking for user's otherwise, return 1. */
    if (find_regno_note(cc_user, REG_DEAD, 36) != 0)
      return 1;

    s_insn = cc_user;
  }
}

int
i960_pic_sym_p(x)
rtx x;
{
  /* for now, this will change if we add pic. */
  return 0;
}

/*
 * For the i960 the address cost varies, but here's how they break down
 * on the CA for ld/st and lda.
 *
 * lda   - (reg)		0
 *         disp/offset		0
 *         disp/offset(reg)	0
 *         disp[reg*scale]	0
 *         (reg)[reg*scale]	1
 *         disp(reg)[reg*scale]	1
 *         disp+8(IP)		3
 *
 * ld/st - (reg)		0
 *         disp/offset		0
 *         disp/offset(reg)	1
 *         disp[reg*scale]	1
 *         (reg)[reg*scale]	2
 *         disp(reg)[reg*scale]	2
 *         disp+8(IP)		4
 *
 * Here's how they break out for the S and K series.
 *
 * all   - offset		0
 *         disp			1
 *         (reg)		1
 *         disp/offset(reg)	2
 *         disp[reg*scale]	2
 *         (reg)[reg*scale]	3
 *         disp(reg)[reg*scale]	3
 *         disp+8(IP)		4
 *
 */
int
i960_address_cost (x, is_lda)
rtx x;
int is_lda;
{
  rtx disp;
  int scale;
  int ireg;
  int breg;
  enum mem_mode {OFF, DISP, REG, REG_OFF, REG_DISP,
                 REG_IND, REG_IND_DISP, IND_DISP, IP_REL};
  enum mem_mode mem_mode = OFF;

  if (i960_disect_address(x, &breg, &ireg, &scale, &disp) != 0)
    return 10;

  switch ((breg != -1) << 2 | (ireg != -1) << 1 | (disp != 0) << 0)
  {
    case 7:
      mem_mode = REG_IND_DISP;
      break;

    case 6:
      mem_mode = REG_IND;
      break;

    case 5:
      /* register with offset, or displacement addressing */
      if (GET_CODE(disp) == CONST_INT &&
          INTVAL(disp) >= 0 && INTVAL(disp) <= 4095)
        mem_mode = REG_OFF;
      else
        mem_mode = REG_DISP;
      break;

    case 4:
      mem_mode = REG;
      break;

    case 3:
    case 2:
      mem_mode = IND_DISP;
      break;

    case 1:
      /* offset, displacement, or ip relative address */
      if (GET_CODE(disp) == CONST_INT &&
          INTVAL(disp) >= 0 && INTVAL(disp) <= 4095)
        mem_mode = OFF;
      else
        if (i960_pic_sym_p(x))
          mem_mode = IP_REL;
        else
          mem_mode = DISP;
      break;

    case 0:
      mem_mode = OFF;
      break;
  }

  if (TARGET_C_SERIES || TARGET_H_SERIES)
  {
    switch (mem_mode)
    {
      case OFF:
      case DISP:
      case REG:
        return 0;
      case REG_OFF:
      case REG_DISP:
      case IND_DISP:
        return is_lda ? 0 : 1;
      case REG_IND:
      case REG_IND_DISP:
        return is_lda ? 1 : 2;
      case IP_REL:
        return is_lda ? 3 : 4;
    }
  }

  if (TARGET_K_SERIES)
  {
    switch (mem_mode)
    {
      case OFF:
        return 0;
      case DISP:
      case REG:
        return 1;
      case REG_OFF:
      case REG_DISP:
      case IND_DISP:
        return 2;
      case REG_IND:
      case REG_IND_DISP:
        return 3;
      case IP_REL:
        return 4;
    }
  }

  if (TARGET_J_SERIES)
  {
    switch (mem_mode)
    {
      case OFF:
      case REG:
      case REG_OFF:
        return 1;
      case DISP:
      case REG_DISP:
      case IND_DISP:
        return 2;
      case REG_IND:
      case REG_IND_DISP:
      case IP_REL:
        return 6;
    }
  }
}

/*
 * return the cost, roughly in number of register-to-register moves
 * that it will take to do this mem, either load or store. This routine
 * doesn't know whether its a load or store, and is therefore somewhat
 * inaccurate.
 * For a load there are two clocks of overhead + n_words * wait states.
 * However if we have burst memory, then its 2 + n_words + wait state.
 * finally if loads go back to back one of the overhead slots gets
 * overlapped.
 * So the formula actually used is either 1 + n_words 
 */
int
i960_mem_cost(x)
rtx x;
{
  int cost;
  int n_words = GET_MODE_SIZE(GET_MODE(x)) / GET_MODE_SIZE(word_mode);

  cost = i960_address_cost(XEXP(x,0), 0);

  if (i960_burst_mem)
    cost += 1 + n_words + i960_wait_states;
  else
    cost += 1 + n_words * i960_wait_states;

  return cost;
}

/* Emit insns to move operands[1] into operands[0].

   Return 1 if we have written out everything that needs to be done to
   do the move.  Otherwise, return 0 and the caller will emit the move
   normally.  */

int
emit_move_sequence (operands, mode)
     rtx *operands;
     enum machine_mode mode;
{
  register rtx operand0 = operands[0];
  register rtx operand1 = operands[1];

  /* We can only store registers to memory.  */
  if (GET_CODE (operand0) == MEM && GET_CODE (operand1) != REG)
    operands[1] = force_reg (mode, operand1);

  return 0;
}

#ifndef IMSTG_REAL
void
dbl_to_i960_edbl (d, f)
double d;
unsigned f[];
{
  unsigned t,s;

  /* Get IEEE double from host double ... */
  dbl_to_ieee_dbl (d, f);

  t = f[0];
  s = (t & 0x80000000) >> 16;	/* Sign bit */
  t &= 0x7fffffff;

  if (t | f[1])
  { /* Non-zero */
    if (t == 0)
    { /* Denormalized */
      f[0] = (t << 12) | (f[1] >> 22);
      f[1] <<= 12;
      t = -1022;

      /* Normalize it */
      while ((f[0] & 0x80000000) == 0)
      { f[0] = (f[0] << 1) | (f[1] & 0x80000000 != 0);
        f[1] <<= 1;
        t--;
      }
    }
    else
    { /* Normal/Infinity/Nan */

      f[0] = ((t << 11) | 0x80000000) | (f[1] >> 21);
      f[1] <<= 11;
      t = (t >> 20) - 1023;
    }

    t += 16383;	/* Final bias */
  }
  else
    f[0] = 0;	/* Because of -0.0 */

  t |= s;

  f[2] = f[1];
  f[1] = f[0];
  f[0] = t;
}
#endif

/* Emit insns to load a constant.  Uses several strategies to try to use
   as few insns as possible.  */

char *
i960_output_ldconst (dst, src)
     register rtx dst, src;
{
  register int rsrc1;
  register unsigned rsrc2;
  enum machine_mode mode = GET_MODE (dst);
  rtx operands[4];
  unsigned int tg_lo;
  unsigned int tg_hi;
#ifndef IMSTG_REAL
  union { long l[2]; double d; } x;
#endif

  operands[0] = operands[2] = dst;
  operands[1] = operands[3] = src;

  /* Anything that isn't a compile time constant, such as a SYMBOL_REF,
     must be a ldconst insn.  */

  if (GET_CODE (src) != CONST_INT && GET_CODE (src) != CONST_DOUBLE)
  {
      output_asm_insn ("lda	%1,%0", operands);
      return "";
  }

  switch (mode)
  {
    case TFmode:
    {
      long f[4];
      REAL_VALUE_TYPE d;

      assert (GET_CODE(src)==CONST_DOUBLE);
#ifdef IMSTG_REAL
      REAL_VALUE_FROM_CONST_DOUBLE (d, src);
      REAL_VALUE_TO_TARGET_LONG_DOUBLE(d,f);
#else
      dbl_to_i960_edbl (get_double(src),f);
#endif

      if (!FP_REG_P(dst) && f[0]==0 && f[1]==0 && f[2]==0)
      {
        i960_out_movq (operands[0], const0_rtx);
        return "";
      }

      if (fp_literal(src,VOIDmode))
        return "movre	%1,%0";

      assert (!FP_REG_P(dst));

      output_asm_insn ("# ld %1,%0",operands);

      operands[0] = gen_rtx (REG, SImode, REGNO (dst));
      operands[1] = gen_rtx (CONST_INT, VOIDmode, f[TW0]);
      output_asm_insn (i960_output_ldconst (operands[0], operands[1]),operands);

      operands[0] = gen_rtx (REG, SImode, REGNO (dst) + 1);
      operands[1] = gen_rtx (CONST_INT, VOIDmode, f[TW1]);
      output_asm_insn (i960_output_ldconst (operands[0],operands[1]), operands);

      operands[0] = gen_rtx (REG, SImode, REGNO (dst) + 2);
      operands[1] = gen_rtx (CONST_INT, VOIDmode, f[TW2]);
      output_asm_insn (i960_output_ldconst (operands[0], operands[1]),operands);

      return "";
    }

    case DFmode:
    {
      rtx first, second;
      long f[2];
      REAL_VALUE_TYPE d;

      assert (GET_CODE(src)==CONST_DOUBLE);
#ifdef IMSTG_REAL
      REAL_VALUE_FROM_CONST_DOUBLE (d, src);
      REAL_VALUE_TO_TARGET_DOUBLE(d,f);
#else
      dbl_to_ieee_dbl (get_double(src), f);
#endif

      if (!FP_REG_P(dst)&& f[0]==0 && f[1]==0)
        return i960_out_movl(operands[0], const0_rtx);

      if (fp_literal(src,VOIDmode))
        return "movrl	%1,%0";

      assert (!FP_REG_P(dst));

      first  = gen_rtx (CONST_INT, VOIDmode, f[DW0]);
      second = gen_rtx (CONST_INT, VOIDmode, f[DW1]);

      output_asm_insn ("# lda	%1,%0",operands);

      operands[0] = gen_rtx (REG, SImode, REGNO (dst));
      operands[1] = first;
      output_asm_insn (i960_output_ldconst (operands[0], operands[1]),
		      operands);
      operands[0] = gen_rtx (REG, SImode, REGNO (dst) + 1);
      operands[1] = second;
      output_asm_insn (i960_output_ldconst (operands[0], operands[1]),
		      operands);
      return "";
    }

    case SFmode:
    {
      unsigned long l[1];
      REAL_VALUE_TYPE d;

      assert (GET_CODE(src)==CONST_DOUBLE);
#ifdef IMSTG_REAL
      REAL_VALUE_FROM_CONST_DOUBLE (d, src);
      REAL_VALUE_TO_TARGET_SINGLE(d, l[0]);
#else
      dbl_to_ieee_sgl (get_double(src), l);
#endif

      output_asm_insn ("# ldconst	%1,%0",operands);
      operands[0] = gen_rtx (REG, SImode, REGNO (dst));
      operands[1] = gen_rtx (CONST_INT, VOIDmode, l[0]);
      output_asm_insn (i960_output_ldconst (operands[0], operands[1]),
		      operands);
      return "";
    }

    case TImode:
      abort ();

    case DImode:
    {
      char *string;
      if (GET_CODE (src) != CONST_DOUBLE && GET_CODE (src) != CONST_INT)
        abort();

      get_cnst_val(src, DImode, &tg_lo, &tg_hi, 0);

      if (tg_hi == 0 && tg_lo <= 31)
        return i960_out_movl(dst, GEN_INT(tg_lo));

      string = i960_output_ldconst(gen_rtx (REG, SImode, REGNO (dst) + 1),
                                   GEN_INT(tg_hi));
      output_asm_insn (string, 0);

      string = i960_output_ldconst(gen_rtx (REG, SImode, REGNO (dst)),
                                   GEN_INT(tg_lo));
      output_asm_insn (string, 0);

      return "";
    }

    case SImode:
    case HImode:
    case QImode:
    {
      get_cnst_val(src, VOIDmode, &tg_lo, &tg_hi, 0);
      rsrc1 = tg_lo;

      if (rsrc1 >= 0)
      {
        /* ldconst 0..31,X  ->  mov 0..31,X  */
        if (rsrc1 < 32)
        {
          if (i960_last_insn_type == I_TYPE_REG &&
              (TARGET_C_SERIES || TARGET_H_SERIES))
            return "lda\t%1,%0";
          return "mov\t%1,%0";
        }

        /* ldconst 32..63,X -> add 31,nn,X  */
        if (rsrc1 < 63)
        {
          if (i960_last_insn_type == I_TYPE_REG &&
              (TARGET_C_SERIES || TARGET_H_SERIES))
            return "lda\t%1,%0";
          operands[1] = gen_rtx (CONST_INT, VOIDmode, rsrc1 - 31);
          output_asm_insn ("addo\t31,%1,%0\t# ldconst %3,%0", operands);
          return "";
        }
      }
      else if (rsrc1 < 0)
      {
        /* ldconst -1..-31  -> sub 0,0..31,X  */
        if (rsrc1 >= -31)
        {
          /* return 'sub -(%1),0,%0' */
          operands[1] = gen_rtx (CONST_INT, VOIDmode, - rsrc1);
          output_asm_insn ("subo\t%1,0,%0\t# ldconst %3,%0", operands);
          return "";
        }
      
        /* ldconst -32  -> not 31,X  */
        if (rsrc1 == -32)
        {
          operands[1] = gen_rtx (CONST_INT, VOIDmode, ~rsrc1);
          output_asm_insn ("not\t%1,%0\t# ldconst %3,%0", operands);
          return "";
        }
      }

      /* If const is a single bit.  */
      if (bitpos (rsrc1) >= 0)
      {
        operands[1] = gen_rtx (CONST_INT, VOIDmode, bitpos (rsrc1));
        output_asm_insn ("setbit\t%1,0,%0\t# ldconst %3,%0", operands);
        return "";
      }

      /* If const is a bit string of less than 6 bits (1..31 shifted).  */
      if (is_mask (rsrc1))
      {
        int s, e;

        if (bitstr (rsrc1, &s, &e) < 6)
        {
          rsrc2 = ((unsigned int) rsrc1) >> s;
          operands[1] = gen_rtx (CONST_INT, VOIDmode, rsrc2);
          operands[2] = gen_rtx (CONST_INT, VOIDmode, s);
          output_asm_insn ("shlo\t%2,%1,%0\t# ldconst %3,%0", operands);
          return "";
        }
      }

      /*
       * Unimplemented cases:
       * const is in range 0..31 but rotated around end of word:
       * ror 31,3,g0 -> ldconst 0xe0000003,g0
       */

      output_asm_insn ("lda\t%1,%0", operands);
      return "";
    }

    default:
      abort();
  }
}

char *
i960_out_movl(dst, src)
rtx dst;
rtx src;
{
  rtx operands[2];

  operands[0] = dst;
  operands[1] = src;

  if (TARGET_J_SERIES && !flag_space_opt)
  {
    output_asm_insn ("mov\t%1,%0", operands);
    output_asm_insn ("mov\t%D1,%D0", operands);
    return "";
  }

  output_asm_insn ("movl\t%1,%0", operands);
  return "";
}

char *
i960_out_movt(dst, src)
rtx dst;
rtx src;
{
  rtx operands[2];

  operands[0] = dst;
  operands[1] = src;

  if (TARGET_J_SERIES && !flag_space_opt)
  {
    output_asm_insn ("mov\t%1,%0", operands);
    output_asm_insn ("mov\t%D1,%D0", operands);
    output_asm_insn ("mov\t%T1,%T0", operands);
    return "";
  }

  output_asm_insn ("movt\t%1,%0", operands);
  return "";
}

int
i960_reg_move_cost(x,y)
enum reg_class x;
enum reg_class y;
{
  /* When we can think of something intelligent to do,
     put some code in here.  For now, this is the
     normal gcc default. */
  return 2;
}

char *
i960_out_cop_move (operands)
rtx operands[];
{
#ifdef IMSTG_COPREGS
  int src_regno = -1;
  int dst_regno = -1;
  int src_clas  = -1;
  int dst_clas  = -1;

  static char *to_960[] = COP_TO_960;
  static char* fm_960[] = COP_FM_960;
  static char* fm_cop[] = COP_FM_COP;

  rtx dst = operands[0];
  rtx src = operands[1];

  if (GET_CODE (src) == REG || GET_CODE (src) == SUBREG)
    src_regno = true_regnum (src);

  if (GET_CODE (dst) == REG || GET_CODE (dst) == SUBREG)
    dst_regno = true_regnum (dst);

  assert (src_regno >= 0 && dst_regno >= 0);

  src_clas = i960_reg_class (src_regno);
  dst_clas = i960_reg_class (dst_regno);

  assert (IS_COP_CLASS(src_clas) || IS_COP_CLASS(dst_clas));

  if (IS_COP_CLASS(src_clas) && IS_COP_CLASS(dst_clas))
  { assert (src_clas == dst_clas);
    output_asm_insn (fm_cop[src_clas - COP0_REGS], operands);
  }

  else if (IS_COP_CLASS(src_clas))
    output_asm_insn (to_960[src_clas - COP0_REGS], operands);

  else
    output_asm_insn (fm_960[dst_clas - COP0_REGS], operands);
#else
  assert (0);
#endif
  return "";
}

char *
i960_out_movq(dst, src)
rtx dst;
rtx src;
{
  rtx operands[2];

  operands[0] = dst;
  operands[1] = src;

  if (TARGET_J_SERIES && !flag_space_opt)
  {
    output_asm_insn ("mov\t%1,%0", operands);
    output_asm_insn ("mov\t%D1,%D0", operands);
    output_asm_insn ("mov\t%T1,%T0", operands);
    output_asm_insn ("mov\t%X1,%X0", operands);
    return "";
  }

  output_asm_insn ("movq\t%1,%0", operands);
  return "";
}

char *
i960_out_ldl(dst, src)
rtx dst;
rtx src;
{
  rtx operands[2];

  operands[0] = dst;
  operands[1] = src;

  if (TARGET_J_SERIES && !flag_space_opt)
  {
    rtx disp;
    int breg;
    int ireg;
    int scale;

    if (i960_disect_address(XEXP(src, 0), &breg, &ireg, &scale, &disp) == 0 &&
        breg != -1 &&
        ireg == -1 &&
        (disp == 0 ||
         (GET_CODE(disp) == CONST_INT &&
          (((unsigned)INTVAL(disp)) + 4) < 4095)))
    {
      int disp_val = 0;
      char buf[40];
      if (disp != 0)
        disp_val = INTVAL(disp);
      
      if (REGNO(operands[0]) == breg)
      {
        /*
         * breg overlaps the first register in the pair, so we have to
         * reverse the order of the load so as not to clobber breg before
         * we are done with it.
         */
        sprintf (buf, "ld\t%d(%s),%%D0", disp_val+4, reg_names[breg]);
        output_asm_insn (buf, operands);
        sprintf (buf, "ld\t%d(%s),%%0", disp_val, reg_names[breg]);
        output_asm_insn (buf, operands);
      }
      else
      {
        sprintf (buf, "ld\t%d(%s),%%0", disp_val, reg_names[breg]);
        output_asm_insn (buf, operands);
        sprintf (buf, "ld\t%d(%s),%%D0", disp_val+4, reg_names[breg]);
        output_asm_insn (buf, operands);
      }
      return "";
    }
  }

  output_asm_insn ("ldl\t%1,%0", operands);
  return "";
}

char *
i960_out_ldt(dst, src)
rtx dst;
rtx src;
{
  rtx operands[2];

  operands[0] = dst;
  operands[1] = src;

  if (TARGET_J_SERIES && !flag_space_opt)
  {
    rtx disp;
    int breg;
    int ireg;
    int scale;

    if (i960_disect_address(XEXP(src, 0), &breg, &ireg, &scale, &disp) == 0 &&
        breg != -1 &&
        ireg == -1 &&
        (disp == 0 ||
         (GET_CODE(disp) == CONST_INT &&
          (((unsigned)INTVAL(disp)) + 8) < 4095)))
    {
      int disp_val = 0;
      char buf[40];
      if (disp != 0)
        disp_val = INTVAL(disp);

      if (REGNO(operands[0]) == breg)
      {
        /*
         * breg overlaps the first register, so we have to
         * clobber it last.
         */
        sprintf (buf, "ld\t%d(%s),%%D0", disp_val+4, reg_names[breg]);
        output_asm_insn (buf, operands);
        sprintf (buf, "ld\t%d(%s),%%T0", disp_val+8, reg_names[breg]);
        output_asm_insn (buf, operands);
        sprintf (buf, "ld\t%d(%s),%%0", disp_val, reg_names[breg]);
        output_asm_insn (buf, operands);
      }
      else if (REGNO(operands[0])+1 == breg)
      {
        /*
         * breg overlaps the second register, so we have to
         * clobber it last.
         */
        sprintf (buf, "ld\t%d(%s),%%0", disp_val, reg_names[breg]);
        output_asm_insn (buf, operands);
        sprintf (buf, "ld\t%d(%s),%%T0", disp_val+8, reg_names[breg]);
        output_asm_insn (buf, operands);
        sprintf (buf, "ld\t%d(%s),%%D0", disp_val+4, reg_names[breg]);
        output_asm_insn (buf, operands);
      }
      else
      {
        sprintf (buf, "ld\t%d(%s),%%0", disp_val, reg_names[breg]);
        output_asm_insn (buf, operands);
        sprintf (buf, "ld\t%d(%s),%%D0", disp_val+4, reg_names[breg]);
        output_asm_insn (buf, operands);
        sprintf (buf, "ld\t%d(%s),%%T0", disp_val+8, reg_names[breg]);
        output_asm_insn (buf, operands);
      }
      return "";
    }
  }

  output_asm_insn ("ldt\t%1,%0", operands);
  return "";
}

char *
i960_out_ldq(dst, src)
rtx dst;
rtx src;
{
  rtx operands[2];

  operands[0] = dst;
  operands[1] = src;

  if (TARGET_J_SERIES && !flag_space_opt)
  {
    rtx disp;
    int breg;
    int ireg;
    int scale;

    if (i960_disect_address(XEXP(src, 0), &breg, &ireg, &scale, &disp) == 0 &&
        breg != -1 &&
        ireg == -1 &&
        (disp == 0 ||
         (GET_CODE(disp) == CONST_INT &&
          (((unsigned)INTVAL(disp)) + 12) < 4095)))
    {
      int disp_val = 0;
      char buf[40];
      if (disp != 0)
        disp_val = INTVAL(disp);

      if (REGNO(operands[0]) == breg)
      {
        /*
         * breg overlaps the first register, so we have to
         * clobber it last.
         */
        sprintf (buf, "ld\t%d(%s),%%D0", disp_val+4, reg_names[breg]);
        output_asm_insn (buf, operands);
        sprintf (buf, "ld\t%d(%s),%%T0", disp_val+8, reg_names[breg]);
        output_asm_insn (buf, operands);
        sprintf (buf, "ld\t%d(%s),%%X0", disp_val+12, reg_names[breg]);
        output_asm_insn (buf, operands);
        sprintf (buf, "ld\t%d(%s),%%0", disp_val, reg_names[breg]);
        output_asm_insn (buf, operands);
      }
      else if (REGNO(operands[0])+1 == breg)
      {
        /*
         * breg overlaps the second register, so we have to
         * clobber it last.
         */
        sprintf (buf, "ld\t%d(%s),%%0", disp_val, reg_names[breg]);
        output_asm_insn (buf, operands);
        sprintf (buf, "ld\t%d(%s),%%T0", disp_val+8, reg_names[breg]);
        output_asm_insn (buf, operands);
        sprintf (buf, "ld\t%d(%s),%%X0", disp_val+12, reg_names[breg]);
        output_asm_insn (buf, operands);
        sprintf (buf, "ld\t%d(%s),%%D0", disp_val+4, reg_names[breg]);
        output_asm_insn (buf, operands);
      }
      else if (REGNO(operands[0])+2 == breg)
      {
        /*
         * breg overlaps the third register, so we have to
         * clobber it last.
         */
        sprintf (buf, "ld\t%d(%s),%%0", disp_val, reg_names[breg]);
        output_asm_insn (buf, operands);
        sprintf (buf, "ld\t%d(%s),%%D0", disp_val+4, reg_names[breg]);
        output_asm_insn (buf, operands);
        sprintf (buf, "ld\t%d(%s),%%X0", disp_val+12, reg_names[breg]);
        output_asm_insn (buf, operands);
        sprintf (buf, "ld\t%d(%s),%%T0", disp_val+8, reg_names[breg]);
        output_asm_insn (buf, operands);
      }
      else
      {
        sprintf (buf, "ld\t%d(%s),%%0", disp_val, reg_names[breg]);
        output_asm_insn (buf, operands);
        sprintf (buf, "ld\t%d(%s),%%D0", disp_val+4, reg_names[breg]);
        output_asm_insn (buf, operands);
        sprintf (buf, "ld\t%d(%s),%%T0", disp_val+8, reg_names[breg]);
        output_asm_insn (buf, operands);
        sprintf (buf, "ld\t%d(%s),%%X0", disp_val+12, reg_names[breg]);
        output_asm_insn (buf, operands);
      }
      return "";
    }
  }
  output_asm_insn ("ldq\t%1,%0", operands);
  return "";
}

char *
i960_out_stl(dst, src)
rtx dst;
rtx src;
{
  rtx operands[2];

  operands[0] = dst;
  operands[1] = src;

#if 0
  if (TARGET_J_SERIES && !flag_space_opt)
  {
    rtx disp;
    int breg;
    int ireg;
    int scale;

    if (i960_disect_address(XEXP(dst, 0), &breg, &ireg, &scale, &disp) == 0 &&
        breg != -1 &&
        ireg == -1 &&
        (disp == 0 ||
         (GET_CODE(disp) == CONST_INT &&
          (((unsigned)INTVAL(disp)) + 4) < 4095)))
    {
      int disp_val = 0;
      char buf[40];
      if (disp != 0)
        disp_val = INTVAL(disp);

      sprintf (buf, "st\t%%1,%d(%s)", disp_val, reg_names[breg]);
      output_asm_insn (buf, operands);
      sprintf (buf, "st\t%%D1,%d(%s)", disp_val+4, reg_names[breg]);
      output_asm_insn (buf, operands);
      return "";
    }
  }
#endif

  output_asm_insn ("stl\t%1,%0", operands);
  return "";
}

char *
i960_out_stt(dst, src)
rtx dst;
rtx src;
{
  rtx operands[2];

  operands[0] = dst;
  operands[1] = src;

#if 0
  if (TARGET_J_SERIES && !flag_space_opt)
  {
    rtx disp;
    int breg;
    int ireg;
    int scale;

    if (i960_disect_address(XEXP(dst, 0), &breg, &ireg, &scale, &disp) == 0 &&
        breg != -1 &&
        ireg == -1 &&
        (disp == 0 ||
         (GET_CODE(disp) == CONST_INT &&
          (((unsigned)INTVAL(disp)) + 8) < 4095)))
    {
      int disp_val = 0;
      char buf[40];
      if (disp != 0)
        disp_val = INTVAL(disp);

      sprintf (buf, "st\t%%1,%d(%s)", disp_val, reg_names[breg]);
      output_asm_insn (buf, operands);
      sprintf (buf, "st\t%%D1,%d(%s)", disp_val+4, reg_names[breg]);
      output_asm_insn (buf, operands);
      sprintf (buf, "st\t%%T1,%d(%s)", disp_val+8, reg_names[breg]);
      output_asm_insn (buf, operands);
      return "";
    }
  }
#endif

  output_asm_insn ("stt\t%1,%0", operands);
  return "";
}

char *
i960_out_stq(dst, src)
rtx dst;
rtx src;
{
  rtx operands[2];

  operands[0] = dst;
  operands[1] = src;

#if 0
  if (TARGET_J_SERIES && !flag_space_opt)
  {
    rtx disp;
    int breg;
    int ireg;
    int scale;

    if (i960_disect_address(XEXP(dst, 0), &breg, &ireg, &scale, &disp) == 0 &&
        breg != -1 &&
        ireg == -1 &&
        (disp == 0 ||
         (GET_CODE(disp) == CONST_INT &&
          (((unsigned)INTVAL(disp)) + 12) < 4095)))
    {
      int disp_val = 0;
      char buf[40];
      if (disp != 0)
        disp_val = INTVAL(disp);

      sprintf (buf, "st\t%%1,%d(%s)", disp_val, reg_names[breg]);
      output_asm_insn (buf, operands);
      sprintf (buf, "st\t%%D1,%d(%s)", disp_val+4, reg_names[breg]);
      output_asm_insn (buf, operands);
      sprintf (buf, "st\t%%T1,%d(%s)", disp_val+8, reg_names[breg]);
      output_asm_insn (buf, operands);
      sprintf (buf, "st\t%%X1,%d(%s)", disp_val+12, reg_names[breg]);
      output_asm_insn (buf, operands);
      return "";
    }
  }
#endif

  output_asm_insn ("stq\t%1,%0", operands);
  return "";
}

/* Determine if there is an opportunity for a bypass optimization.
   Bypass suceeds on the 960K* if the destination of the previous
   instruction is the second operand of the current instruction.
   Bypass always succeeds on the C*.
 
   Return 1 if the pattern should interchange the operands.

   CMPBR_FLAG is true if this is for a compare-and-branch insn.
   OP1 and OP2 are the two source operands of a 3 operand insn.  */

int
i960_bypass (insn, op1, op2, cmpbr_flag)
     register rtx insn, op1, op2;
     int cmpbr_flag;
{
  register rtx prev_insn, prev_dest;

  if (!TARGET_K_SERIES)
    return 0;

  /* Can't do this if op1 isn't a register.  */
  if (! REG_P (op1))
    return 0;

  /* Can't do this for a compare-and-branch if both ops aren't regs.  */
  if (cmpbr_flag && ! REG_P (op2))
    return 0;

  prev_insn = prev_real_insn (insn);

  if (prev_insn && GET_CODE (prev_insn) == INSN
      && GET_CODE (PATTERN (prev_insn)) == SET)
    {
      prev_dest = SET_DEST (PATTERN (prev_insn));
      if ((GET_CODE (prev_dest) == REG && REGNO (prev_dest) == REGNO (op1))
	  || (GET_CODE (prev_dest) == SUBREG
	      && GET_CODE (SUBREG_REG (prev_dest)) == REG
	      && REGNO (SUBREG_REG (prev_dest)) == REGNO (op1)))
	return 1;
    }
  return 0;
}

tree last_varargs_func;
current_func_has_argblk()
{
  int yup;

  yup = (current_function_args_size > 48) ||
        (last_varargs_func == current_function_decl);

  return yup;
}

static int compute_regs_used ();

/*
 * This routine is a very specialized search to see if any address in the
 * frame is used other than in the address expression for a MEM.
 */
int frame_address_used_p(x)
rtx x;
{
  char *fmt;
  int i;
  enum rtx_code code;

  if (x == 0)
    return 0;

  switch (code = GET_CODE(x))
  {
    case MEM:
      return 0;

    case REG:
      if (REGNO(x) == STACK_POINTER_REGNUM ||
          REGNO(x) == FRAME_POINTER_REGNUM)
        return 1;
      return 0;

    default:
      fmt = GET_RTX_FORMAT (code);
      for (i = GET_RTX_LENGTH (code) - 1; i >= 0; i--)
      {
        if (fmt[i] == 'E')
        {
          register int j;
          for (j = XVECLEN (x, i) - 1; j >= 0; j--)
            if (frame_address_used_p (XVECEXP (x, i, j)))
              return 1;
        }
        else if (fmt[i] == 'e' && frame_address_used_p (XEXP (x, i)))
          return 1;
      }
      return 0;
  }
}

/* Output the code which declares the function name.  This also handles
   leaf routines, which have special requirements, and initializes some
   global variables.  */

void
i960_function_name_declare (file, name, fndecl)
FILE *file;
char *name;
tree fndecl;
{
  int i, j, icol, leaf_proc_ok;
  int regs[FIRST_PSEUDO_REGISTER];
  rtx first_insn = get_insns();
  rtx insn;
  int in_interrupt = i960_pragma_interrupt(fndecl) == PRAGMA_DO;

  /* Check that an interrupt routine that has no parameters. */
  if (in_interrupt && DECL_ARGUMENTS(fndecl) != 0)
    warning ("interrupt function %s cannot have parameters.", name);
  
  /* Check that interrupt routine has void return type. */
  if (in_interrupt &&
      (DECL_RESULT(fndecl) != 0 &&
       TREE_TYPE(DECL_RESULT(fndecl)) != 0 &&
       TREE_CODE(TREE_TYPE(DECL_RESULT(fndecl))) != VOID_TYPE))
    warning ("interrupt function %s should have void return type.", name);

  /* Increment global return label.  */
  i960_leaf_ret_label++;

  tail_call_ok = (TARGET_TAILCALL) != 0;
  leaf_proc_ok = (TARGET_LEAFPROC) != 0 &&
                 !i960_in_isr && !i960_system_function(fndecl);

  i960_leaf_ret_reg = -1;

  /* Even if nobody uses extra parms, can't have leafroc or tail calls if
     argblock, because argblock uses g14 implicitly.  */

  if (current_func_has_argblk() ||
      aggregate_value_p (DECL_RESULT(fndecl)) ||
      current_function_calls_alloca)
  {
    tail_call_ok = 0;
    leaf_proc_ok = 0;
  }

  if (i960_func_has_calls)
  {
    leaf_proc_ok = 0;

    if (i960_in_isr)
    {
      /* must make g0-g7,g13 be used registers. */
      regs_ever_live[0] = 1;
      regs_ever_live[1] = 1;
      regs_ever_live[2] = 1;
      regs_ever_live[3] = 1;
      regs_ever_live[4] = 1;
      regs_ever_live[5] = 1;
      regs_ever_live[6] = 1;
      regs_ever_live[7] = 1;
      regs_ever_live[13] = 1;

      if (TARGET_NUMERICS)
      {
        regs_ever_live[32] = 1;
        regs_ever_live[33] = 1;
        regs_ever_live[34] = 1;
        regs_ever_live[35] = 1;
      }
    }
  }
      
  /*
   * This loop is kind of complex.
   * It is meant to check to make sure that leaf_proc or tail calls are
   * ok.  A leafproc is OK is there are no references to the frame pointer
   * anywhere within this function.
   * A tail call is OK only if no address of anything in the current frame
   * can be passed into the function to be called via a tail-call.
   * We are actually much more picky than that.  If the address of anything
   * within the frame is taken then we disallow all tail-calls.
   *
   * We check for the address of a frame item by checking to see if any
   * rtl expressions that are not memory references use either the frame
   * pointer or the stack pointer.  We also allow a little extra lattitude
   * for stack pointer references, in that we allow tail-calls to be used
   * if a stack pointer expression is used just to reassign the stack pointer.
   *
   */
  for (insn = first_insn; (insn != 0 && (leaf_proc_ok | tail_call_ok) != 0);
       insn = NEXT_INSN(insn))

  {
    if (GET_RTX_CLASS(GET_CODE(insn)) == 'i')
    {
      rtx pat = PATTERN(insn);
      
      if (tail_call_ok && frame_address_used_p(pat))
      {
        if (GET_CODE(pat) != SET ||
            GET_CODE(SET_DEST(pat)) != REG ||
            REGNO(SET_DEST(pat)) != STACK_POINTER_REGNUM)
          tail_call_ok = 0;
      }

      if (leaf_proc_ok && reg_mentioned_p(frame_pointer_rtx, pat))
        leaf_proc_ok = 0;
    }
  }

  compute_regs_used (regs);

  if (leaf_proc_ok)
  { 
    /* Can not be a leaf routine if any local registers are used */
    for (i = 16; i < 32 && leaf_proc_ok; i++)
      if (regs[i])
        leaf_proc_ok = 0;

    /* Find a leaf return register which does not change prologue/epilogue */
    if (leaf_proc_ok)
    {
      for (i = 0; i < 8 && i960_leaf_ret_reg == -1; i++)
        if (!regs[i])
          i960_leaf_ret_reg = i;

      if (i960_leaf_ret_reg == -1)
        leaf_proc_ok = 0;
    }
  }

  /* Do this after choosing the leaf return register, so it will be listed
     if one was chosen.  */
  if (file != 0)
  {
    i960_declared_fname = name;

    fprintf (file, "\t#  Function '%s'\n", name);
    fprintf (file, "\t#  Registers used: ");

    if (i960_leaf_ret_reg >= 0)
    {
      regs_ever_live[i960_leaf_ret_reg] = 1;
      regs[i960_leaf_ret_reg] = 1;
    }

    for ((i=j=0); i < FIRST_PSEUDO_REGISTER; i++)
      if (regs[i])
      {
        fprintf (file, "%s%s ", reg_names[i], call_used_regs[i] ? "" : "*");

        if ((i < 16) != (j < 16))
        {
          fprintf (file,"\n\t#\t\t   ");
          j = i;
        }
      }

    fprintf (file, "\n");

#if 0
    if (i960_in_isr && !in_interrupt)
    {
      /*
       * put out sizteen byte ID that vx960 uses to identify that this
       * was compiled with pragma isr.
       */
      fprintf (file, "\t.ascii \"i960_pragma_isr\\0\"\n");
    }
#endif

    {
      unsigned char *sysproc_out_p;
      int syscall_num = i960_pragma_system(name, &sysproc_out_p);
      i960_out_sysproc(file, name, syscall_num, sysproc_out_p);
    }

#if defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2
    /* Create a .debug_frame CIE entry for this function. */
    if (write_symbols == DWARF_DEBUG && debug_info_level >= DINFO_LEVEL_NORMAL)
    {
      Dwarf_cie	cie;
      unsigned long	offset;

      /* First pre-compute all of the CIE field values for this function.
         Note that for leaf functions we will create two CIEs, one describing
         the non-leaf entry point, and one describing the leaf entry point.
         Create the non-leaf CIE first.
       */

      cie.next_cie = NULL;
      cie.label_num = 0;

      cie.augmentation = DWARF_CIE_AUGMENTER;
      cie.code_alignment_factor = DWARF_MIN_INSTRUCTION_BYTE_LENGTH;
      cie.data_alignment_factor = 1;

      if (dwarf_reg_info[FRAME_POINTER_REGNUM].cie_column < 0)
      {
	cie.cfa_rule = DW_CFA_undefined;
	cie.cfa_register = 0;
	cie.cfa_offset = 0;
      }
      else
      {
	cie.cfa_rule = DW_CFA_def_cfa;
	cie.cfa_register = dwarf_reg_info[FRAME_POINTER_REGNUM].cie_column;
	cie.cfa_offset = 0;
      }

      /* Return address is in r2, the rip register. */
      cie.return_address_register = dwarf_reg_info[18].cie_column;

      /* Set the initial rules for each register. */

      if (i960_in_isr)
      {
      	for (i = 0; i < DWARF_CIE_MAX_REG_COLUMNS; i++)
      	{
	  cie.rules[i][0] = DW_CFA_same_value;
	  cie.rules[i][1] = 0;
      	}

      	/* For the local registers, use DW_CFA_i960_pfp_offset */

      	for (i = 16, offset = 0; i <= 31; i++, offset += 4)
      	{
      	  icol = dwarf_reg_info[i].cie_column;
      	  cie.rules[icol][0] = DW_CFA_i960_pfp_offset;
      	  cie.rules[icol][1] = offset;
      	}
      }
      else
      {
      	/* Initialize every rule and operand to some dummy value. */
      	for (i = 0; i < DWARF_CIE_MAX_REG_COLUMNS; i++)
      	{
      	  cie.rules[i][0] = DW_CFA_undefined;
      	  cie.rules[i][1] = 0;
      	}

      	/* Now handle 960-specific register usage.  fp0 thru fp3, g0 thru g7,
	   and g13 are call-scratch registers, leave them as DW_CFA_undefined.
      	 */

      	/* g8 thru g11 are sort of funny.  They are supposed to be
	   call-preserved, *unless* used as parameters or global variables in
	   which case they're call-scratch.  But it seems our implementation
	   is conservative and always treats them as call-preserved.
      	 */
      	for (i = 8; i <= 11; i++)
      	{
      	  if (!global_regs[i])
      	  {
      	    icol = dwarf_reg_info[i].cie_column;
      	    cie.rules[icol][0] = DW_CFA_same_value;
      	    cie.rules[icol][1] = 0;
      	  }
      	}

      	/* g12 is call-preserved, unless used as a global variable */
      	if (!global_regs[12])
      	{
      	  icol = dwarf_reg_info[12].cie_column;
      	  cie.rules[icol][0] = DW_CFA_same_value;
      	  cie.rules[icol][1] = 0;
      	}

	/* We needn't go out of our way to tell the debugger how to restore
	   g14, since its value in the previous frame won't be used in
      	   any location expression or FDE rule at the point of call.
	   However, in the FDE's emitted for the call entry point of a
	   leaf function, g14 is coincidentally unaltered.  In this case,
	   we treat it as DW_CFA_same_value.
	 */
        if (i960_leaf_ret_reg >= 0)
	{
      	  icol = dwarf_reg_info[14].cie_column;
      	  cie.rules[icol][0] = DW_CFA_same_value;
      	  cie.rules[icol][1] = 0;
	}

      	/* g15 is DW_CFA_register(pfp). */
      	icol = dwarf_reg_info[15].cie_column;
      	cie.rules[icol][0] = DW_CFA_register;
      	cie.rules[icol][1] = dwarf_reg_info[16].cie_column;

      	/* Registers r0 thru r15 are preserved in the previous frame. */

      	for (i = 16, offset = 0; i <= 31; i++, offset += 4)
      	{
      	  icol = dwarf_reg_info[i].cie_column;
      	  cie.rules[icol][0] = DW_CFA_i960_pfp_offset;
      	  cie.rules[icol][1] = offset;
      	}
      }

      /* Initialize the .debug_frame FDE for this function. */

      bzero (&dwarf_fde, sizeof(dwarf_fde));
      dwarf_fde.cie = dwarfout_cie(&cie);
      dwarf_fde.name = (char*) xmalloc(strlen(name) + 1);
      (void) strcpy(dwarf_fde.name, name);
      dwarf_fde.leaf_return_reg = -1;
      dwarf_fde.leaf_return_label_num = -1;
    }
#endif /* DWARF_DEBUGGING_INFO */

    if (flag_elf)
      fprintf (file, "\t.elf_type\t_%s,function\n", name);

    if (i960_leaf_ret_reg >= 0)
    {
      char *fname = IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(fndecl));
      /* Make it a leaf procedure.  */

#if defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2

      /* Create another .debug_frame CIE entry for this function. */
      if (write_symbols==DWARF_DEBUG && debug_info_level >= DINFO_LEVEL_NORMAL)
      {
        Dwarf_cie	cie;
        unsigned long	offset;

        cie.next_cie = NULL;
        cie.label_num = 0;

        cie.augmentation = DWARF_CIE_AUGMENTER;
        cie.code_alignment_factor = DWARF_MIN_INSTRUCTION_BYTE_LENGTH;
        cie.data_alignment_factor = 1;

      	if (dwarf_reg_info[STACK_POINTER_REGNUM].cie_column < 0)
	{
      	  cie.cfa_rule = DW_CFA_undefined;
      	  cie.cfa_register = 0;
      	  cie.cfa_offset = 0;
      	}
	else
	{
      	  cie.cfa_rule = DW_CFA_def_cfa;
      	  cie.cfa_register = dwarf_reg_info[STACK_POINTER_REGNUM].cie_column;
      	  cie.cfa_offset = 0;
      	}

      	/* Return address is in g14. */
      	cie.return_address_register = dwarf_reg_info[14].cie_column;

      	/* Initialize every rule and operand to some dummy value. */
      	for (i = 0; i < DWARF_CIE_MAX_REG_COLUMNS; i++)
      	{
      		cie.rules[i][0] = DW_CFA_undefined;
      		cie.rules[i][1] = 0;
      	}

      	/* Leave g0 thru g7, and g13 as DW_CFA_undefined.
      	   g8 thru g12 are call-preserved in our implementation
      	   Actually, we suppress leafprocs when g8 thru g11 are used.
      	 */

      	for (i = 8; i <= 12; i++)
      	{
      	  if (!global_regs[i])
      	  {
      	    icol = dwarf_reg_info[i].cie_column;
      	    cie.rules[icol][0] = DW_CFA_same_value;
      	    cie.rules[icol][1] = 0;
      	  }
      	}

      	/* g14 is unchanged initially.  It contains the return address. */
      	icol = dwarf_reg_info[14].cie_column;
      	cie.rules[icol][0] = DW_CFA_same_value;
      	cie.rules[icol][1] = 0;

      	/* g15 is unchanged initially. */
      	icol = dwarf_reg_info[15].cie_column;
      	cie.rules[icol][0] = DW_CFA_same_value;
      	cie.rules[icol][1] = 0;

      	/* The local registers r0, r1 and r3 thru r15 are call-preserved.
	   The old value of r2, the rip register, is undefined.
	 */
      	for (i = 16; i <= 31; i++)
      	{
      		icol = dwarf_reg_info[i].cie_column;
      		cie.rules[icol][0] = DW_CFA_same_value;
      		cie.rules[icol][1] = 0;
      	}

	/* r2 (18) is undefined. */
	icol = dwarf_reg_info[18].cie_column;
	cie.rules[icol][0] = DW_CFA_undefined;
	cie.rules[icol][1] = 0;

        dwarf_fde.call_cie = dwarf_fde.cie;
        dwarf_fde.cie = dwarfout_cie(&cie);
      }
#endif /* DWARF_DEBUGGING_INFO */

      if (TREE_PUBLIC (fndecl) || GLOBALIZE_STATICS)
        fprintf (file,"\t.globl    %s.lf\n", name);

      fprintf (file, "\t.leafproc\t_%s,%s.lf\n", name, name);
      if (flag_elf)
        fprintf (file, "\t.elf_type\t%s.lf, function\n", name);

      /*
       * This next bit is a stupid workaround because of brain-damaged
       * COFF debug information.
       */
      if ((flag_coff) && strcmp(name, fname) != 0)
        fprintf (file, "\t.leafproc\t_%s,%s.lf\n", fname, name);
      fprintf (file, "_%s:\n", name);
      if (TARGET_PIC)
        fprintf (file, "\tlda    LR%d-(.+8)(ip),g14\n", i960_leaf_ret_label);
      else
        fprintf (file, "\tlda    LR%d,g14\n", i960_leaf_ret_label);
      fprintf (file, "%s.lf:\n", name);
      fprintf (file, "\tmov\tg14,g%d\n", i960_leaf_ret_reg);

#if defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2
      if (write_symbols == DWARF_DEBUG
	  && debug_info_level >= DINFO_LEVEL_NORMAL
	  && (icol = dwarf_reg_info[14].cie_column) >= 0)
      {
	/* Put a label before the insn that clears g14.
	   At this point, the .debug_frame FDE will change the g14 rule
	   to indicate its original value is now in i960_leaf_ret_reg.
	 */
        char	lab[MAX_ARTIFICIAL_LABEL_BYTES];

	/* Note that we reserve the label number now.  Some other label
	   numbers are reserved in i960_compute_prologue_epilogue().
	   Note, however, that i960_compute_prologue_epilogue() is called
	   several times, so that the labels "reserved" there might change
	   before final prologue/epilogue emission.
	   Later, final_start_function() calls i960_function_prologue(),
	   which will call i960_compute_prologue_epilogue() one last time
	   before emission.
	 */
        dwarf_fde.clear_g14_label_num = ++dwarf_fde_code_label_counter;

        (void) ASM_GENERATE_INTERNAL_LABEL(lab, DWARF_FDE_CODE_LABEL_CLASS,
			dwarf_fde.clear_g14_label_num);
	ASM_OUTPUT_LABEL(file, lab);
      }
#endif /* DWARF_DEBUGGING_INFO */

      if (TARGET_C_SERIES || TARGET_H_SERIES)
      {
        fprintf (file, "\tlda\t0,g14\n");
        i960_last_insn_type = I_TYPE_MEM;
      }
      else
      {
        fprintf (file, "\tmov\t0,g14\n");
        i960_last_insn_type = I_TYPE_REG;
      }
    }
    else
    {
      ASM_OUTPUT_LABEL (file, name);
      if (TARGET_CAVE)
      {
	rtx first_lnote = NULL_RTX, last_lnote = NULL_RTX;
	rtx cave_begin_label = gen_label_rtx();
	cave_end_label = gen_label_rtx();

	/* For CAVE functions, emit source position information only
	   for the first and last instructions in the stub function.
	   Don't emit any debug info for the secondary function, since
	   its position independent nature makes setting break points
	   within it not only impossible, but possibly harmful.
	 */
        if (write_symbols != NO_DEBUG)
        {
	  /* Find the lowest and highest numbered line number notes. */
	  rtx lnote;
	  for (lnote = get_insns(); lnote; lnote = NEXT_INSN(lnote))
	  {
	    if (IS_LINE_NUMBER_NOTE_P(lnote))
	    {
	      first_lnote = last_lnote = lnote;
	      lnote = NEXT_INSN(lnote);
	      break;
	    }
	  }

	  for (; lnote; lnote = NEXT_INSN(lnote))
	  {
	    if (IS_LINE_NUMBER_NOTE_P(lnote))
	    {
	      if (NOTE_LINE_NUMBER(lnote) < NOTE_LINE_NUMBER(first_lnote))
		first_lnote = lnote;
	      if (NOTE_LINE_NUMBER(lnote) > NOTE_LINE_NUMBER(last_lnote))
		last_lnote = lnote;
	    }
	  }

#if defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2
	  if (write_symbols == DWARF_DEBUG && first_lnote)
	  {
	    NOTE_SET_LEFTMOST_STMT(first_lnote);
	    NOTE_SET_LEFTMOST_STMT(last_lnote);

	    NOTE_SET_STMT_BEGIN(first_lnote);
	    if (NOTE_LINE_NUMBER(first_lnote) ==
		NOTE_LINE_NUMBER(last_lnote))
	      NOTE_CLEAR_STMT_BEGIN(last_lnote);
	    else
	      NOTE_SET_STMT_BEGIN(last_lnote);
	  }
#endif
        }

	imstg_cave_final_start_function(first_lnote, file);
	fprintf (file, "\tlda	L%d,r4\n", CODE_LABEL_NUMBER(cave_begin_label));
	if (TARGET_USE_CALLX)
	{
		if (TARGET_PIC)
		{
			fprintf (file, 
				"\tlda 0(ip),r3; lda .,r5; subo r5,r3,r3\n");
			fprintf (file, "\tcallx	__cave_dispatch(r3)\n");
		}
		else
			fprintf (file, "\tcallx	__cave_dispatch\n");
	}
	else
		fprintf (file, "\tcall	__cave_dispatch\n");
#if defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2
	if (write_symbols == DWARF_DEBUG)
	  dwarfout_le_boundary(func_last_le_boundary_number);
#endif
	imstg_cave_final_pre_end_function(last_lnote, file);
	fprintf (file, "\tret\n");
	i960_emit_elf_function_size(file);
	imstg_cave_final_end_function(file);
	in_cave_function_body = 1;
	set_no_section();
	text_section();
	fprintf (file, "\t.align	2\n");		/* Word align */
	fprintf (file, "\t.word L%d-L%d\n", 
			CODE_LABEL_NUMBER(cave_end_label),
			CODE_LABEL_NUMBER(cave_begin_label));
	ASM_OUTPUT_INTERNAL_LABEL(file, "L",
			CODE_LABEL_NUMBER(cave_begin_label));
      }	
      i960_last_insn_type = I_TYPE_CTRL; 
    }
  }
}


int
outgoing_args_size()
{
  int asize;

  if (i960_func_passes_on_stack == 0 &&
      !current_function_calls_alloca &&
      current_function_outgoing_args_size <= 48)
    asize = 0;
  else
    asize = current_function_outgoing_args_size;

  asize += current_function_pretend_args_size;

  asize = ROUND (asize, 16);

  return asize;
}

/* Compute prologue_string, epilogue_string;  return frame size */

static int
i960_compute_prologue_epilogue (size, leaf_ret, regs)
unsigned int size;
int leaf_ret;
int regs[];
{
  int i,j,nr,n_iregs,rsize,fsize,asize,ssize,sret,saves[FIRST_PSEUDO_REGISTER];
  char *p,*q, sizes[FIRST_PSEUDO_REGISTER];
  int in_interrupt = i960_pragma_interrupt(current_function_decl) == PRAGMA_DO;
  int offset;
#if defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2
  int	icol, jcol, n, atleast_one_reg_saved;
#endif

  bzero (saves, sizeof(saves));
  bzero (sizes, sizeof(sizes));

  (p=prologue_string)[0] = '\0';
  (q=abstract_epilogue_string)[0] = '\0';

#if defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2
  if (write_symbols == DWARF_DEBUG && debug_info_level >= DINFO_LEVEL_NORMAL
      && !TARGET_CAVE)
  {
    dwarf_fde.prologue_label_num = 0;
    bzero (dwarf_fde.prologue_rules, sizeof(dwarf_fde.prologue_rules));
    atleast_one_reg_saved = 0;

    /* Record in dwarf_fde the register that g14 will be copied into.  */

    if (leaf_ret != -1)
    {
      icol = dwarf_reg_info[14].cie_column;
      jcol = dwarf_reg_info[i960_leaf_ret_reg].cie_column;

      dwarf_fde.leaf_return_reg = jcol;
      dwarf_fde.leaf_return_label_num = i960_leaf_ret_label;

      dwarf_fde.prologue_rules[icol][0] = DW_CFA_register;
      dwarf_fde.prologue_rules[icol][1] = jcol;
    }
    else
    {
      dwarf_fde.leaf_return_reg = -1;
      dwarf_fde.leaf_return_label_num = -1;
    }
  }
#endif

  if (TARGET_NUMERICS && i960_in_isr)
  {
    if (regs_ever_live[32] || regs_ever_live[33] ||
        regs_ever_live[34] || regs_ever_live[35])
    {
      /*
       * make registers r4-r7 alive so that we can use them for saving
       * the floating point registers.
       */
      regs_ever_live[20] = 1;
      regs_ever_live[21] = 1;
      regs_ever_live[22] = 1;
      regs_ever_live[23] = 1;
    }
  }

  for (i = 0, n_iregs = 0; i < FIRST_PSEUDO_REGISTER; i++)
  {
    regs[i] = 0;

    if (regs_ever_live[i])
    {
      regs[i] = 1;
      if (((i < 14 || i > 31) && i960_in_isr) ||
          (i >= 8 && i <= 12 && !global_regs[i]))
      {
        regs[i] = -1;
	n_iregs++;
      }
    }
  }

  if (i960_in_isr && i960_func_has_calls)
  {
    /*
     * If this is the case then we need to save g14 and zero it in the
     * prologue, and restore it in the epilogue.
     */
    regs[14] = -1;
    n_iregs++;
  }

  sprintf (q, "#EPILOGUE:\n");
  q += strlen (q);

#if defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2
  if (write_symbols == DWARF_DEBUG && debug_info_level >= DINFO_LEVEL_NORMAL
      && !TARGET_CAVE)
  {
    /* Put a tentative label (%L) at the start of the epilogue string.
       This will be removed later if not needed.
     */
    strcpy(q, "%L");
    q += strlen(q);
  }
#endif

  /*
   * Look for local registers to save globals in.  We don't do this for
   * #pragma interrupt functions because the globals for these have to
   * be saved in a certain place.
   */
  if (!in_interrupt)
  {
    for (i = 0; i < 15; i++)
      if (regs[i] == -1)
        for (j = 20; j < 32; j++)
          if (regs[j] == 0)
          {
            regs[i] = 1;
            regs[j] = 1;
            nr = 1;
    
            if (nr==1 && j<=30 && !(i%2) && !(j%2) &&
                regs[i+1]== -1 && !regs[j+1] &&
                (!TARGET_J_SERIES || flag_space_opt))
            { nr = 2;
              regs[i+1] = 1;
              regs[j+1] = 1;
             }
    
            if (nr==2 && j<=28 && !(i%4) && !(j%4) &&
                regs[i+2]== -1 && !regs[j+2])
            { nr = 3;
              regs[i+2] = 1;
              regs[j+2] = 1;
            }
    
            if (nr==3 && regs[i+3]==-1 && !regs[j+3])
            { nr = 4;
              regs[i+3] = 1;
              regs[j+3] = 1;
            }
    
            if ((TARGET_C_SERIES || TARGET_H_SERIES) &&
                nr == 1 && i960_last_insn_type == I_TYPE_REG)
            {
              sprintf(p, "\tlda\t(%s),%s\n", reg_names[i], reg_names[j]);
              sprintf(q, "\tlda\t(%s),%s\n", reg_names[j], reg_names[i]);
              i960_last_insn_type = I_TYPE_MEM;
            }
            else
            {
              char *t = ((nr==4)?"q":(nr==3)?"t":(nr==2)?"l":"");
              sprintf(p, "\tmov%s\t%s,%s\n", t, reg_names[i], reg_names[j]);
              sprintf(q, "\tmov%s\t%s,%s\n", t, reg_names[j], reg_names[i]);
              i960_last_insn_type = I_TYPE_REG;
            }

#if defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2
	    if (write_symbols == DWARF_DEBUG
		&& debug_info_level >= DINFO_LEVEL_NORMAL
		&& !TARGET_CAVE)
	    {
	      atleast_one_reg_saved = 1;
	      for (n = nr-1; n >= 0; n--)
	      {
	        icol = dwarf_reg_info[i+n].cie_column;
	        jcol = dwarf_reg_info[j+n].cie_column;
	        dwarf_fde.prologue_rules[icol][0] = DW_CFA_register;
	        dwarf_fde.prologue_rules[icol][1] = jcol;
	      }
	    }
#endif

            p += strlen(p);
            q += strlen(q);
            tail_call_ok = 0;
            n_iregs -= nr;
            i += nr-1;
            break;
          }
  }

  rsize = size;

  /*
   * Record sizes and save offsets for saving registers into stack.
   * Again special stuff happens for #pragma interrupt routines to
   * get everything in the right places.
   */
  if (in_interrupt)
  {

    for (i = 0; i < 12; i += 4)
    {
      if (regs[i] == -1 || regs[i+1] == -1 ||
          regs[i+2] == -1 || regs[i+3] == -1)
      {
        sizes[i] = 'q';
        saves[i] = i * 4;
        tail_call_ok = 0;
      }
    }
    if (regs[12] == -1 || regs[13] == -1 || regs[14] == -1)
    {
      sizes[12] = 't';
      saves[12] = 12*4;
      tail_call_ok = 0;
    }

    for (i = 32; i < 36; i++)
    {
      if (regs[i] == -1)
      {
        sizes[i] = 't';
        saves[i] = 64 + ((i-32) * 16);
        tail_call_ok = 0;
      }
    }

    size += TARGET_NUMERICS ? 128 : 64;
  }
  else
  {
    for (i=0; i < 16; i++)
    {
      if (regs[i] == -1)
      {
        nr = 1;
 
        if (nr==1 && !(i%2) && regs[i+1]== -1)
        { size = ROUND(size,8);
          nr = 2;
        }
 
        if (nr==2 && !(i%4) && regs[i+2]== -1)
        { size = ROUND(size, 16);
          nr = 3;
        }
 
        if (nr==3 && regs[i+3] == -1)
          nr = 4;
 
        saves[i] = size;
        sizes[i] = ((nr==4) ? 'q':(nr==3) ? 't':(nr==2) ? 'l':' ');
        tail_call_ok = 0;
 
        size += (nr*4);
        i += nr-1;
      }
    }

    for (i = 32; i < 36; i++)
    {
      if (regs[i] == -1)
      {
        saves[i] = size;
        sizes[i] = 't';
        tail_call_ok = 0;
        size += 16;
      }
    }
  }

  rsize = size - rsize;

  /* Have to provide space for fake struct return for small structs
     not in register.  YUK ! */

  sret = -1;

  if (current_function_returns_struct)
  { ssize = int_size_in_bytes(TREE_TYPE(DECL_RESULT(current_function_decl)));

    if (ssize <= 16)
    { int tssize = i960_object_bytes_bitalign(ssize)/BITS_PER_UNIT;
      size  = ROUND (size, tssize);
      sret  = size;
      size += tssize;
    }
  }

  asize = outgoing_args_size();
  size  = ROUND (size, 16);
  fsize = size + asize;

  /* Allocate space for saves, locals, fake struct ret, outgoing parms */
  if (fsize)
  { if (fsize < 32)
    {
      sprintf (p, "\taddo\t%d,sp,sp\n", fsize);
      i960_last_insn_type = I_TYPE_REG;
    }
    else
    {
      sprintf (p, "\tlda\t%d(sp),sp\n", fsize);
      i960_last_insn_type = I_TYPE_MEM;
    }
    p += strlen(p);
  }

  /* Kludge small struct ret */
  if (sret >= 0)
  {
    char *t;
    switch (ssize)
    {
      case 1: t = "ob"; break;
      case 2: t = "os"; break;

      case 3:
      case 4: t = ""; break;

      case 5:
      case 6:
      case 7:
      case 8: t = "l"; break;

      case 9:
      case 10:
      case 11:
      case 12: t = "t"; break;

      default: t = "q"; break;
    }

    if (leaf_ret != -1)
      sprintf(p,"\tlda\t-%d(sp),g13\n", fsize-sret);
    else
      sprintf(p,"\tlda\t%d(fp),g13\n", 64+sret);
    p += strlen(p);

    i960_last_insn_type = I_TYPE_MEM;
    tail_call_ok = 0;

    if (leaf_ret != -1)
      sprintf(q,"\tld%s\t-%d(sp),g0\n", t, fsize-sret);
    else
      sprintf(q,"\tld%s\t%d(fp),g0\n", t, 64+sret);
    q += strlen(q);

    if (ssize == 3)
    {
      sprintf(q, "\tshro\t8,g0,g0\n");
      q += strlen(q);
    }
  }

  for (i=0; i < 16; i++)
  {
    if (sizes[i] == 0)
      continue;

    /*
     * We only want to load sp relative if this is a leaf function.  This
     * is because the negative offsets guarantee a 32 bit displacement
     * addressing mode, while the small positive numbers could probably use
     * the 10 bit offset addressing mode.  This results in smaller code,
     * and on some machines, faster code.
     */
    if (leaf_ret != -1)
    {
      offset = fsize-saves[i];
      sprintf(p,"\tst%c\t%s,-%d(sp)\n", sizes[i], reg_names[i], offset);
    }
    else
    {
      offset = 64+saves[i];
      sprintf(p,"\tst%c\t%s,%d(fp)\n", sizes[i], reg_names[i], offset);
    }
    p += strlen(p);
    i960_last_insn_type = I_TYPE_MEM;

#if defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2
    if (write_symbols == DWARF_DEBUG && debug_info_level >= DINFO_LEVEL_NORMAL
	&& !TARGET_CAVE)
    {
      if ((icol = dwarf_reg_info[i].cie_column) >= 0)
      {
	atleast_one_reg_saved = 1;
        dwarf_fde.prologue_rules[icol][0] = DW_CFA_offset;
        dwarf_fde.prologue_rules[icol][1] = offset;
      }

      if (sizes[i] == 'l' || sizes[i] == 't' || sizes[i] == 'q')
      {
	if ((icol = dwarf_reg_info[i+1].cie_column) >= 0)
	{
	  atleast_one_reg_saved = 1;
	  dwarf_fde.prologue_rules[icol][0] = DW_CFA_offset;
	  dwarf_fde.prologue_rules[icol][1] = offset + 4;
	}
	if (sizes[i] == 't' || sizes[i] == 'q')
	{
	  if ((icol = dwarf_reg_info[i+2].cie_column) >= 0)
	  {
	    atleast_one_reg_saved = 1;
	    dwarf_fde.prologue_rules[icol][0] = DW_CFA_offset;
	    dwarf_fde.prologue_rules[icol][1] = offset + 8;
	  }
	  if (sizes[i] == 'q')
	  {
	    if ((icol = dwarf_reg_info[i+3].cie_column) >= 0)
	    {
	      atleast_one_reg_saved = 1;
	      dwarf_fde.prologue_rules[icol][0] = DW_CFA_offset;
	      dwarf_fde.prologue_rules[icol][1] = offset + 12;
	    }
	  }
        }
      }
    }
#endif

    /*
     * Use SP if the function is a leaf function, otherwise use fp.
     */
    if (leaf_ret != -1)
      sprintf(q,"\tld%c\t-%d(sp),%s\n", sizes[i], offset, reg_names[i]);
    else
      sprintf(q,"\tld%c\t%d(fp),%s\n", sizes[i], offset, reg_names[i]);
    q += strlen(q);
  }

  for (i = 32; i < 36; i++)
  {
    if (sizes[i] == 0)
      continue;

    sprintf(p,"\tmovre\t%s,r4\n", reg_names[i]);
    p += strlen(p);
    if (leaf_ret != -1)
    {
      offset = fsize-saves[i];
      sprintf(p,"\tst%c\tr4,-%d(sp)\n", sizes[i], offset);
    }
    else
    {
      offset = 64+saves[i];
      sprintf(p,"\tst%c\tr4,%d(fp)\n", sizes[i], offset);
    }
    p += strlen(p);

#if defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2
    if (write_symbols == DWARF_DEBUG && debug_info_level >= DINFO_LEVEL_NORMAL
	&& !TARGET_CAVE)
    {
      if ((icol = dwarf_reg_info[i].cie_column) >= 0)
      {
	atleast_one_reg_saved = 1;
        dwarf_fde.prologue_rules[icol][0] = DW_CFA_offset;
        dwarf_fde.prologue_rules[icol][1] = offset;
      }
    }
#endif

    if (leaf_ret != -1)
      sprintf(q,"\tld%c\t-%d(sp),r4\n", sizes[i], fsize-saves[i]);
    else
      sprintf(q,"\tld%c\t%d(fp),r4\n", sizes[i], 64+saves[i]);
    q += strlen(q); 
    sprintf(q,"\tmovre\tr4,%s\n", reg_names[i]);
    q += strlen(q); 
  }

  if (i960_in_isr && i960_func_has_calls)
  {
    /* generate code to set g14 to zero. */

#if defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2
    if (write_symbols == DWARF_DEBUG && debug_info_level >= DINFO_LEVEL_NORMAL
	&& !TARGET_CAVE
	&& (icol = dwarf_reg_info[14].cie_column) >= 0
	&& dwarf_fde.prologue_rules[icol][0] != DW_CFA_nop)
    {
      char	lab[MAX_ARTIFICIAL_LABEL_BYTES];

      dwarf_fde.clear_g14_label_num = ++dwarf_fde_code_label_counter;

      (void) ASM_GENERATE_INTERNAL_LABEL(lab, DWARF_FDE_CODE_LABEL_CLASS,
			dwarf_fde.clear_g14_label_num);
      /* HACK: GENERATE_INTERNAL_LABEL puts a * at the start of the label */
      if (lab[0] == '*')
        sprintf (p, "%s:\n", lab+1);
      else
        sprintf (p, "%s:\n", lab);
      p += strlen(p);
    }
#endif

    sprintf(p,"\tmov\t0,g14\n");
    i960_last_insn_type = I_TYPE_REG;
    p += strlen(p);
  }

#if defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2
  if (write_symbols == DWARF_DEBUG && debug_info_level >= DINFO_LEVEL_NORMAL
      && !TARGET_CAVE
      && atleast_one_reg_saved)
  {
    /* Put a label in the epilogue here.  The function's original register
       save rules (from the CIE) are now in effect.
     */
    strcpy(q, "%L");
    q += strlen(q);
  }
#endif

  /* De-allocate space for saves, locals, fake struct ret, outgoing parms */
  if (fsize && leaf_ret != -1)
  { if (fsize < 32)
      sprintf (q, "\tsubo\t%d,sp,sp\n", fsize);
    else
      sprintf (q, "\tlda\t-%d(sp),sp\n", fsize);
    q += strlen(q);
  }

  /* Issue return */
  if (leaf_ret != -1)
    sprintf (q,"\tbx\t(%s)\n", reg_names[leaf_ret]);
  else
    sprintf (q, "\tret\n");
  q += strlen(q);

#if defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2
  if (write_symbols == DWARF_DEBUG && debug_info_level >= DINFO_LEVEL_NORMAL
      && !TARGET_CAVE
      && atleast_one_reg_saved)
  {
    strcpy(q, "%L");
    q += strlen(q);
  }
#endif

  sprintf(p,"\t#Prologue stats:\n\t#  Total Frame Size: %d bytes\n",fsize);
  p += strlen(p);

  sprintf (p, "\t#  Local Variable Size: %d bytes\n", size-rsize);
  p += strlen(p);

  sprintf (p, "\t#  Register Save Size: %d regs, %d bytes\n",n_iregs,rsize);
  p += strlen(p);
  sprintf (p, "\t#End Prologue#\n");
  p += strlen(p);

#if defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2
  if (write_symbols == DWARF_DEBUG && debug_info_level >= DINFO_LEVEL_NORMAL
      && !TARGET_CAVE)
  {
    if (!atleast_one_reg_saved)
    {
      /* Remove the %L parameterizations in the epilogue string.
	 Otherwise we'll end up emitting bogus labels.
       */
      char	*dst, *src;

      dst = src = abstract_epilogue_string;
      while (*src)
      {
	if (*src == '%' && *(src+1) == 'L')
	  src += 2;
	else
	  *dst++ = *src++;
      }

      *dst = '\0';
    }
    else
    {
      /* Put a label at the end of the prologue for use in the FDE. */
      char  lab[MAX_ARTIFICIAL_LABEL_BYTES];
      dwarf_fde.prologue_label_num = ++dwarf_fde_code_label_counter;

      (void) ASM_GENERATE_INTERNAL_LABEL(lab, DWARF_FDE_CODE_LABEL_CLASS,
			dwarf_fde.prologue_label_num);
      /* HACK: GENERATE_INTERNAL_LABEL puts a * at the start of the label */
      if (lab[0] == '*')
        sprintf (p, "%s:\n", lab+1);
      else
        sprintf (p, "%s:\n", lab);
      p += strlen(p);
    }
  }
#endif

  return fsize;
}

int
compute_frame_size (size)
{
  int regs[FIRST_PSEUDO_REGISTER];
  return i960_compute_prologue_epilogue (size,-1,regs);
}

static int
compute_regs_used (regs)
int regs[FIRST_PSEUDO_REGISTER];
{
  return i960_compute_prologue_epilogue (get_frame_size(),-1,regs);
}

i960_find_unused_register (m, could_cross_call)
enum machine_mode m;
int could_cross_call;
{
  int used[FIRST_PSEUDO_REGISTER];
  int  r,i,j;
  int old_size;

  /* try to find an unallocated register that will not change the
     size of the stack frame. */

  old_size = compute_regs_used (used);

  if (!i960_func_has_calls)
    could_cross_call = 0;

  for ((i = 0),(r = -1); i < FIRST_PSEUDO_REGISTER && r == -1; i++)
  {
    r = reg_alloc_order[i];

    if (HARD_REGNO_MODE_OK(r,m))
    {
      for (j = HARD_REGNO_NREGS(r,m)-1; j >= 0 && r != -1; j--)
        if (used[r+j] || fixed_regs[r+j] ||
            (could_cross_call && TEST_HARD_REG_BIT(call_used_reg_set, (r+j))))
          r = -1;
    }
    else
      r = -1;
  }

  if (r != -1)
  {
    int old_regs_ever_live = regs_ever_live[r];
    int new_size;
    regs_ever_live[r] = 1;

    new_size = compute_regs_used(used);
    regs_ever_live[r] = old_regs_ever_live;
    if (old_size != new_size)
      r = -1;
  }
  return r;
}

void
i960_function_prologue (file, size)
FILE* file;
unsigned int size;
{
  int regs[FIRST_PSEUDO_REGISTER];
  i960_compute_prologue_epilogue (size, i960_leaf_ret_reg, regs);
  fprintf (file, prologue_string);
}

/* Output code for the function epilogue.  */

void
i960_function_epilogue (file, size)
FILE *file;
unsigned int size;
{

  /* Emit epilogue only if control can fall through to end of function */

  rtx tmp = get_last_insn ();
  int need_epilogue = 1;

  while (tmp && need_epilogue)

    if (!INSN_DELETED_P(tmp) &&
        (GET_CODE (tmp) == BARRIER ||
        (GET_CODE (tmp) == JUMP_INSN && GET_CODE(PATTERN (tmp)) == RETURN)))
      need_epilogue = 0;

    else
      if (GET_CODE(tmp)==CODE_LABEL || GET_CODE(tmp)==JUMP_INSN)
        break;
      else
        tmp = PREV_INSN (tmp);

  if (need_epilogue)
    fprintf (file, "\t%s", i960_epilogue_string());

  if (i960_leaf_ret_reg != -1)
    /* Always have to put out leaf wrapper */
    fprintf (file, "LR%d:\tret\n", i960_leaf_ret_label);

  if (TARGET_CAVE)
  {
	ASM_OUTPUT_INTERNAL_LABEL(file, "L", CODE_LABEL_NUMBER(cave_end_label));
	in_cave_function_body = 0;
	set_no_section();
  }
  else
    i960_emit_elf_function_size(file);

  i960_leaf_ret_reg = -1;
  i960_func_has_calls = -1;
  i960_func_passes_on_stack = 0;
  i960_func_changes_g14 = 0;
}

char*
i960_text_section_asm_op()
{
  static char buf[80];

  if (in_cave_function_body)
    return ".section\tcave, text";

  if (i960_section_string != 0)
  {
    sprintf(buf, ".section\t.text%s,text", i960_section_string);
    return buf;
  }

  return ".text";
}

char *
i960_data_section_asm_op()
{
  static char buf[80];

  if (i960_section_string != 0)
  {
    sprintf (buf, ".section\t.data%s,data", i960_section_string);
    return buf;
  }
  return ".data";
}

char *
i960_init_section_asm_op()
{
  static char buf[80];
  if (i960_section_string != 0)
  {
    sprintf (buf, ".section\t.init%s,text", i960_section_string);
    return buf;
  }
  return ".section\t.init,text";
}

char *
i960_ctors_section_asm_op()
{
  static char buf[80];
  if (i960_section_string != 0)
  {
    sprintf (buf, ".section\tctors%s,text", i960_section_string);
    return buf;
  }
  return ".section\tctors,text";
}

char *
i960_dtors_section_asm_op()
{
  static char buf[80];
  if (i960_section_string != 0)
  {
    sprintf (buf, ".section\tdtors%s,text", i960_section_string);
    return buf;
  }
  return ".section\tdtors,text";
}

/* Output code for a call insn.  */

char *
i960_output_call_insn (target, argsize_rtx, insn)
     register rtx target, argsize_rtx, insn;
{
  int non_indirect;
  int argsize = INTVAL (argsize_rtx);
  rtx nexti = next_real_insn (insn);
  rtx operands[1];
  int is_syscall;
  unsigned char *sysproc_out_p;
  int syscall_num;
  char *syscall_name;

  operands[0] = target;

  while (nexti != 0 && GET_CODE(PATTERN(nexti))==USE)
    nexti = next_real_insn(nexti);

  non_indirect = 0;
  is_syscall = 0;

  if ((GET_CODE (target) == MEM) &&
      (GET_CODE (XEXP (target, 0)) == SYMBOL_REF))
  {
    non_indirect = 1;
    syscall_name = XSTR(XEXP(target, 0),0);
    syscall_num = i960_pragma_system(syscall_name, &sysproc_out_p);
    if (syscall_num > NOT_SYSCALL)
      is_syscall = 1;
  }

  /*
   * Don't emit a tail-call if we are calling a routine that will need an
   * argblock, otherwise we won't come back to execute the necessary cleanup
   * code for g14.
   * Also nexti could be zero if the called routine is volatile, so don't
   * just blindly dereference it.
   */
  if (!is_syscall && argsize < 48 && tail_call_ok 
      && (nexti == 0 || GET_CODE (PATTERN (nexti)) == RETURN))
  {
    if (nexti && no_labels_between_p (insn, nexti))
    {
      /* Just mark the following return insn as deleted. */
      PUT_CODE(nexti, NOTE);
      NOTE_LINE_NUMBER (nexti) = NOTE_INSN_DELETED;
      NOTE_SOURCE_FILE (nexti) = 0;
      NOTE_CALL_COUNT (nexti) = 0;
    }
    output_asm_insn ((!(TARGET_USE_CALLX || TARGET_CAVE) && non_indirect ?
                        "b\t%P0" :
                        "bx\t%0"), operands);
    return "";
  }

  if (!is_syscall && tail_call_ok &&
      (nexti = next_nonnote_insn(insn)) != 0 &&
      GET_CODE(nexti) == BARRIER)
  {
    /* call to function that is declared volatile.  this is never supposed
     * to return, so we can tail-call it provided no stack data is used,
     * which is true if tail_call_ok is true.
     */
    output_asm_insn ((!(TARGET_USE_CALLX || TARGET_CAVE) && non_indirect ?
                        "b\t%P0" :
                        "bx\t%0"), operands);
    return "";
  }

  if (is_syscall)
  {
    i960_out_sysproc(asm_out_file, syscall_name, syscall_num, sysproc_out_p);

    if (syscall_num != NO_SYSCALL_INDEX && flag_bout)
    {
      char buffer[30];
      operands[0] = GEN_INT(syscall_num);
      if (syscall_num > 31)
      {
        output_asm_insn("lda\t%0,g13", operands);
        operands[0] = gen_rtx(REG, Pmode, 13);
      }
      output_asm_insn("calls\t%0", operands);
    }
    else
      output_asm_insn("calljx\t%0", operands);
  }
  else
    output_asm_insn (!(TARGET_USE_CALLX || TARGET_CAVE) && non_indirect ?
                        "callj\t%P0" : "calljx\t%0", operands);
  return "";
}

/* Output code for a return insn.  */

char *
i960_output_ret_insn (insn)
rtx insn;
{
  return i960_epilogue_string();
}

char *
i960_br_predict_opcode(insn, inverted)
rtx insn;
int inverted;
{
  /*
   * Return a character string representing the branch prediction
   * opcode to be tacked on an instruction.  This must at least
   * return a null string.
   */
  if (TARGET_BRANCH_PREDICT)
  {
    int jump_then_likely;      /* if the probability is > 55 % */
    int jump_then_unlikely;    /* if the probability is < 45 % */

    jump_then_likely   = JUMP_THEN_PROB(insn) > ((55 * PROB_BASE) / 100);
    jump_then_unlikely = JUMP_THEN_PROB(insn) < ((45 * PROB_BASE) / 100);
    if ((jump_then_likely && !inverted) || (jump_then_unlikely && inverted))
      return (".t");
    return (".f");
  }

  return ("");
}

static char branch_buf[128];
int i960_need_compare;
rtx i960_cmp_values[2];
char *i960_cmp_op;
enum machine_mode i960_cmp_mode;
int i960_cmp_ops_swapped;

char *
i960_output_compare (insn, cc_mode, operands)
rtx insn;
enum machine_mode cc_mode;
rtx *operands;
{
  int can_delay_cmp = !TARGET_NOCMPBRANCH;
  rtx cc_user;

  i960_cmp_ops_swapped = 0;
  i960_cmp_mode = cc_mode;
  switch (cc_mode)
  {
    case DFmode:
      can_delay_cmp = 0;
      i960_cmp_op = "cmprl";
      break;
    case TFmode:
    case SFmode:
      can_delay_cmp = 0;
      i960_cmp_op = "cmpr";
      break;
    case CCmode:
      i960_cmp_op = "cmpi";
      if (GET_MODE(operands[0]) == HImode || GET_MODE(operands[1]) == HImode)
      {
        i960_cmp_op = "cmpis";
        can_delay_cmp = 0;
      }
      if (GET_MODE(operands[0]) == QImode || GET_MODE(operands[1]) == QImode)
      {
        i960_cmp_op = "cmpib";
        can_delay_cmp = 0;
      }
      break;
    case CC_UNSmode:
      i960_cmp_op = "cmpo";
      if (GET_MODE(operands[0]) == HImode || GET_MODE(operands[1]) == HImode)
      {
        i960_cmp_op = "cmpos";
        can_delay_cmp = 0;
      }
      if (GET_MODE(operands[0]) == QImode || GET_MODE(operands[1]) == QImode)
      {
        i960_cmp_op = "cmpob";
        can_delay_cmp = 0;
      }
      break;
    case CC_CHKmode:
      i960_cmp_op = "chkbit";
      break;

    default: abort(); break;
  }

  if (can_delay_cmp &&
      ((cc_user = i960_find_cc_user(insn)) != 0) &&
      cc_user == next_nonnote_insn(insn) &&
      dead_or_set_regno_p(cc_user, 36) &&
      GET_CODE(cc_user) == JUMP_INSN)
  {
    i960_cmp_values[0] = operands[0];
    i960_cmp_values[1] = operands[1];
    i960_need_compare = 1;
    return "";
  }

  if (cc_mode != CC_CHKmode &&
      i960_bypass(insn, operands[0], operands[1], 0) &&
      (cc_user = i960_find_cc_user(insn)) != 0 &&
      dead_or_set_regno_p(cc_user, 36))
  {
    /* There is only one user of the cc, so swap the operands, and generate
     * the compare.
     */
    sprintf (branch_buf, "%s\t%%1,%%0", i960_cmp_op);
    i960_cmp_ops_swapped = 1;
  }
  else
    sprintf (branch_buf, "%s\t%%0,%%1", i960_cmp_op);

  i960_need_compare = 0;
  return branch_buf;
}

char *
i960_output_cmp_branch(insn, operands, inverted)
rtx insn;
rtx *operands;
int inverted;
{
  char *cond;
  enum rtx_code code;

  operands[2] = i960_cmp_values[0];
  operands[3] = i960_cmp_values[1];

  code = GET_CODE (operands[0]);
  if (i960_cmp_mode == CC_CHKmode && code != EQ && code != NE)
    abort();

  if (i960_cmp_ops_swapped)
  {
    operands[2] = i960_cmp_values[1];
    operands[3] = i960_cmp_values[0];
    code = swap_condition(code);
  }

  if (i960_need_compare && i960_cmp_mode != CC_CHKmode)
  {
    /*
     * We are going to generate a compare and branch, but we have to make
     * sure the second operand is not the literal if we have one.
     */
    if (GET_CODE(operands[3]) == CONST_INT)
    {
      /* swap the operands, and swap the condition to test. */
      operands[2] = i960_cmp_values[1];
      operands[3] = i960_cmp_values[0];
      code = swap_condition(code);
    }
  }

  if (inverted)
    code = reverse_condition (code);

  switch (code)
  {
    case EQ:
      if (i960_cmp_mode == CC_CHKmode)
        cond = "no";
      else
        cond = "e";
      break;

    case NE:
      if (i960_cmp_mode == CC_CHKmode)
        cond = "e";
      else
        cond = "ne";
      break;

    case GT:
    case GTU:
      cond = "g"; break;

    case LT:
    case LTU:
      cond = "l"; break;

    case GE:
    case GEU:
      cond = "ge"; break;

    case LE:
    case LEU:
      cond = "le"; break;

    default: abort();
  }

  if (!i960_need_compare)
  {
    /* output jump opcode only */
    sprintf (branch_buf, "%c%s%s\t%%l1", i960_br_start_char, cond,
             i960_br_predict_opcode(insn, inverted));
  }
  else if (i960_cmp_mode == CC_CHKmode)
  {
    /* output a check bit and jump opcode */
    sprintf (branch_buf, "bb%c%s\t%%2,%%3,%%l1",
        (code == EQ) ? 'c' : 's',
        i960_br_predict_opcode(insn, inverted));
  }
  else
  {
    /* output a compare and jump opcode */
    sprintf (branch_buf, "%s%c%s%s\t%%2,%%3,%%l1", i960_cmp_op, i960_br_start_char,
             cond, i960_br_predict_opcode(insn, inverted));
  }

  i960_need_compare = 0;
  i960_cmp_ops_swapped = 0;
  return branch_buf;
}

char *
i960_output_test(operands)
rtx *operands;
{
  char *cond;
  enum rtx_code code;

  code = GET_CODE (operands[1]);
  if (i960_cmp_mode == CC_CHKmode && code != EQ && code != NE)
    abort();

  if (i960_cmp_ops_swapped)
    code = swap_condition(code);

  switch (code)
  {
    case EQ:
      if (i960_cmp_mode == CC_CHKmode)
        cond = "no";
      else
        cond = "alterbit\t0,0,";
      break;

    case NE:
      if (i960_cmp_mode == CC_CHKmode)
        cond = "e";
      else
        cond = "ne";
      break;

    case GT:
    case GTU:
      cond = "g"; break;

    case LT:
    case LTU:
      cond = "l"; break;

    case GE:
    case GEU:
      cond = "ge"; break;

    case LE:
    case LEU:
      cond = "le"; break;

    default: abort();
  }

  assert (i960_need_compare == 0);

  /* output a test instruction only */
  if (*cond == 'a')
    sprintf (branch_buf, "%s%%0", cond);
  else
    sprintf (branch_buf, "test%s\t%%0", cond);

  i960_need_compare = 0;
  i960_cmp_ops_swapped = 0;
  return branch_buf;
}

char *
i960_output_pred_insn(opc, inverted, operands)
char *opc;
int inverted;
rtx *operands;
{
  char *cond;
  enum rtx_code code;

  code = GET_CODE (operands[1]);
  if (i960_cmp_mode == CC_CHKmode && code != EQ && code != NE)
    abort();

  if (i960_cmp_ops_swapped)
    code = swap_condition(code);

  if (inverted)
    code = reverse_condition(code);

  switch (code)
  {
    case EQ:
      if (i960_cmp_mode == CC_CHKmode)
        cond = "no";
      else
        cond = "e";
      break;

    case NE:
      if (i960_cmp_mode == CC_CHKmode)
        cond = "e";
      else
        cond = "ne";
      break;

    case GT:
    case GTU:
      cond = "g"; break;

    case LT:
    case LTU:
      cond = "l"; break;

    case GE:
    case GEU:
      cond = "ge"; break;

    case LE:
    case LEU:
      cond = "le"; break;

    default: abort();
  }

  assert (i960_need_compare == 0);

  sprintf (branch_buf, "%s%s\t%%3,%%2,%%0", opc, cond);

  i960_need_compare = 0;
  i960_cmp_ops_swapped = 0;
  return branch_buf;
}

/* Print the operand represented by rtx X formatted by code CODE.  */

void
i960_print_operand (file, x, code)
     FILE *file;
     rtx x;
     char code;
{
  enum rtx_code rtxcode = GET_CODE (x);

  if (rtxcode == REG)
    {
      switch (code)
	{
	case 'D':
	  /* Second reg of a double.  */
	  fprintf (file, "%s", reg_names[REGNO (x)+1]);
	  break;

	case 'T':
	  /* Third reg of a quad. */
	  fprintf (file, "%s", reg_names[REGNO (x)+2]);
	  break;

	case 'X':
	  /* Fourth reg of a quad. */
	  fprintf (file, "%s", reg_names[REGNO (x)+3]);
	  break;

	case 0:
	  fprintf (file, "%s", reg_names[REGNO (x)]);
	  break;

	default:
	  abort ();
	}
      return;
    }
  else if (rtxcode == MEM)
    {
      if (!legitimate_address_p(SImode,XEXP(x,0),0))
        fatal("illegal memory address.");
      output_address (XEXP (x, 0));
      return;
    }
  else if (rtxcode == CONST_INT)
    {
      if (INTVAL (x) > 9999 || INTVAL (x) < -999)
	fprintf (file, "0x%x", INTVAL (x));
      else
	fprintf (file, "%d", INTVAL (x));
      return;
    }
  else if (rtxcode == CONST_DOUBLE)
    {
      REAL_VALUE_TYPE d;

      if (x==CONST0_RTX(TFmode)||x==CONST0_RTX(DFmode) || x==CONST0_RTX(SFmode))
	{
	  fprintf (file, "0f0.0");
	  return;
	}
      else if (x==CONST1_RTX(TFmode)||x==CONST1_RTX(DFmode)||x==CONST1_RTX(SFmode))
	{
	  fprintf (file, "0f1.0");
	  return;
	}

      /* This better be a comment.  */
      REAL_VALUE_FROM_CONST_DOUBLE (d, x);
#ifdef IMSTG_REAL
      { char buf[128];
        ereal_to_decimal(d,buf);
        fprintf (file, "%s", buf);
      }
#else
      fprintf (file, "%#g", d);
#endif

      return;
    }

  switch(code)
    {
    case 'B':
      /* Branch or jump, depending on assembler.  */
      i960_last_insn_type = I_TYPE_CTRL;
      if (TARGET_ASM_COMPAT)
	fputs ("j", file);
      else
	fputs ("b", file);
      break;

    case 'S':
      /* Sign of condition.  */
      if ((rtxcode == EQ) || (rtxcode == NE) || (rtxcode == GTU)
	  || (rtxcode == LTU) || (rtxcode == GEU) || (rtxcode == LEU))
	fputs ("o", file);
      else if ((rtxcode == GT) || (rtxcode == LT)
	  || (rtxcode == GE) || (rtxcode == LE))
	fputs ("i", file);
      else
	abort();
      break;

    case 'I':
      /* Inverted condition.  */
      rtxcode = reverse_condition (rtxcode);
      goto normal;

    case 'X':
      /* Inverted condition w/ reversed operands.  */
      rtxcode = reverse_condition (rtxcode);
      /* Fallthrough.  */

    case 'R':
      /* Reversed operand condition.  */
      rtxcode = swap_condition (rtxcode);
      /* Fallthrough.  */

    case 'C':
      /* Normal condition.  */
      i960_last_insn_type = I_TYPE_CTRL;
    normal:
      if (rtxcode == EQ)       { if (GET_MODE(x) == CC_CHKmode)
                                   fputs ("c", file);
                                 else
                                   fputs ("e", file); 
                                 return; }
      else if (rtxcode == NE)  { if (GET_MODE(x) == CC_CHKmode)
                                   fputs ("s", file);
                                 else
                                   fputs ("ne", file);
                                 return; }
      else if (rtxcode == GT)  { fputs ("g", file); return; }
      else if (rtxcode == GTU) { fputs ("g", file); return; }
      else if (rtxcode == LT)  { fputs ("l", file); return; }
      else if (rtxcode == LTU) { fputs ("l", file); return; }
      else if (rtxcode == GE)  { fputs ("ge", file); return; }
      else if (rtxcode == GEU) { fputs ("ge", file); return; }
      else if (rtxcode == LE)  { fputs ("le", file); return; }
      else if (rtxcode == LEU) { fputs ("le", file); return; }
      else abort ();
      break;

    case 'T':
      /* branch taken setting, pc_rtx means not-taken,
         anything else is means taken. */
      i960_last_insn_type = I_TYPE_CTRL;
      if (!TARGET_BRANCH_PREDICT)
        return;
      if (rtxcode == PC) { fputs(".f", file); return; }
      else               { fputs(".t", file); return; }
      break;

    case 0:
      output_addr_const (file, x);
      break;

    default:
      abort ();
    }

  return;
}

char *
i960_disect_address(addr, breg_p, ireg_p, scale_p, offset_p)
rtx addr;
int *breg_p;
int *ireg_p;
int *scale_p;
rtx *offset_p;
{
  register rtx op0, op1;
  rtx breg, ireg;
  rtx scale, offset;
  int nested_plus = 0;	/* true if is plus (...plus(...)) rtx */
  int regnum;

  ireg = 0;
  breg = 0;
  offset = 0;
  scale = 0;
  nested_plus = 0;

retry:
  switch (GET_CODE (addr))
  {
    case SUBREG:
    case REG:
      breg = addr;
      break;

    case MULT:
      /* fake it as an add of zero. */
      op0 = addr;
      op1 = const0_rtx;
      goto do_addr;

    case PLUS:
      op0 = XEXP(addr, 0);
      op1 = XEXP(addr, 1);

      do_addr: ;
      if (CONSTANT_ADDRESS_P (op0))
      {
        if (op0 == const0_rtx)
          offset = 0;
        else
          offset = op0;
        op0 = 0;
      }
      else if (CONSTANT_ADDRESS_P (op1))
      {
        if (op1 == const0_rtx)
          offset = 0;
        else
          offset = op1;
        op1 = 0;
      }

      if (op0 && (GET_CODE (op0) == REG || GET_CODE(op0) == SUBREG))
      {
        if (!breg)
          breg = op0;
        else if (!ireg)
          ireg = op0;
        else
          return "compiler error - Too many registers in i960_disect_address()";
        op0 = 0;
      }

      if (op1 && (GET_CODE (op1) == REG || GET_CODE(op1) == SUBREG))
      {
        if (!breg)
          breg = op1;
        else if (!ireg)
          ireg = op1;
        else
          return "compiler error - Too many registers in i960_disect_address()";
        op1 = 0;
      }

      if (op0 && GET_CODE (op0) == MULT)
      {
        if ((GET_CODE (XEXP (op0, 0)) == REG ||
             GET_CODE (XEXP (op0, 0)) == SUBREG) && 
            (GET_CODE (XEXP (op0, 1)) == CONST_INT))
        {
          ireg = XEXP (op0, 0);
          scale = XEXP (op0, 1);
          op0 = 0;
        }
        else if ((GET_CODE (XEXP (op0, 1)) == REG ||
                  GET_CODE (XEXP (op0, 1)) == SUBREG) && 
                  (GET_CODE (XEXP (op0, 0)) == CONST_INT))
        {
          ireg = XEXP (op0, 1);
          scale = XEXP (op0, 0);
          op0 = 0;
        }
        else
          return "compiler error - bad MULT rtx in i960_disect_address()";
      }
      else if (op1 && GET_CODE (op1) == MULT)
      {
        if ((GET_CODE (XEXP (op1, 0)) == REG ||
             GET_CODE (XEXP (op1, 0)) == SUBREG) && 
            (GET_CODE (XEXP (op1, 1)) == CONST_INT))
        {
          ireg = XEXP (op1, 0);
          scale = XEXP (op1, 1);
          op1 = 0;
        }
        else if ((GET_CODE (XEXP (op1, 1)) == REG ||
                  GET_CODE (XEXP (op1, 1)) == SUBREG) && 
                    (GET_CODE (XEXP (op1, 0)) == CONST_INT))
        {
          ireg = XEXP (op1, 1);
          scale = XEXP (op1, 0);
          op1 = 0;
        }
        else
          return "compiler error - bad MULT rtx in i960_disect_address()";
      }

      if (op0 && GET_CODE (op0) == PLUS)
      {
        if (op1 || nested_plus) /* should have already matched op1 */
          return "compiler error - illegal address in i960_disect_address()";
        addr = op0;
        nested_plus = 1;
        goto retry;
      }
      else if (op1 && GET_CODE (op1) == PLUS)
      {
        if (op0) /* should have already matched op0 */
          return "compiler error - illegal address in i960_disect_address()";
        addr = op1;
        nested_plus = 1;
        goto retry;
      }

      if (op0 || op1) /* something didn't match */
        return "compiler error - illegal address in i960_disect_address()";

      break;

    case SYMBOL_REF:
    case LABEL_REF:
    case CONST_INT:
    case CONST_DOUBLE:
    case CONST:
      offset = addr;
      break;

    default:
      return "compiler error - illegal address in i960_disect_address()";
      break;
  }

  if (breg != 0)
  {
    if (GET_CODE(breg) == SUBREG)
    {
      regnum = REGNO(XEXP(breg, 0));
      regnum += XINT(breg, 1);
    }
    else
      regnum = REGNO(breg);
    *breg_p = regnum;
  }
  else
    *breg_p = -1;

  if (ireg != 0)
  {
    if (GET_CODE(ireg) == SUBREG)
    {
      regnum = REGNO(XEXP(ireg, 0));
      regnum += XINT(ireg, 1);
    }
    else
      regnum = REGNO(ireg);
    *ireg_p = regnum;
  }
  else
    *ireg_p = -1;

  if (scale != 0)
  {
    if (!SCALE_TERM_P(scale))
      return "compiler error - illegal scale factor in i960_disect_address";
    *scale_p = INTVAL(scale);
  }
  else
    *scale_p = 1;

  *offset_p = offset;
  return 0;
}

/* Print a memory address as an operand to reference that memory location.  */
i960_print_operand_addr (file, addr)
FILE *file;
register rtx addr;
{
  int breg, ireg, scale;
  rtx offset;
  int regnum;
  char *mess;

  if (GET_CODE (addr) == MEM)
    addr = XEXP (addr, 0);

  mess = i960_disect_address(addr, &breg, &ireg, &scale, &offset);
  if (mess != 0)
    fatal(mess);

  if (offset != 0)
    output_addr_const(file, offset);

  if (breg != -1)
    fprintf (file, "(%s)", reg_names[breg]);

  if (ireg != -1)
    fprintf (file, "[%s*%d]", reg_names[ireg], scale);
}

char *
i960_best_lda_sequence(operands)
rtx *operands;
{
  char *ret_val = "lda\t%a1,%0";
  int target_reg;
  static char buf[100];

  if (GET_CODE(operands[0]) == SUBREG)
  {
    target_reg = REGNO(XEXP(operands[0], 0));
    target_reg += XINT(operands[0], 1);
  }
  else
    target_reg = REGNO(operands[0]);

  /* operands[0] is the dest, operands[1] is the address operand. */
  if (TARGET_K_SERIES)
  {
    /* break the address operand into its associated parts */
    int breg;
    int ireg;
    int scale;
    rtx disp;

    i960_disect_address(operands[1], &breg, &ireg, &scale, &disp);

    if (breg != -1 && ireg == -1 &&
        GET_CODE(disp) == CONST_INT && 
        disp != 0 && INTVAL(disp) >= 0 && INTVAL(disp) <= 4095 &&
        breg != target_reg &&
        !flag_space_opt)
    {
      char *targ_name = reg_names[target_reg];
      char *breg_name = reg_names[breg];
      sprintf(buf, "lda\t%d,%s\n\taddo\t%s,%s,%s",
              INTVAL(disp), targ_name,
              breg_name, targ_name, targ_name);
      return buf;
    }

    if (breg != -1 && ireg != -1 && disp == 0)
    {
      char *targ_name = reg_names[target_reg];
      char *breg_name = reg_names[breg];
      char *ireg_name = reg_names[ireg];

      if (scale == 1)
      {
        sprintf (buf, "addo\t%s,%s,%s", breg_name, ireg_name, targ_name);
        return buf;
      }

      if (!flag_space_opt && breg != target_reg)
      {
        sprintf (buf, "shlo\t%d,%s,%s\n\taddo\t%s,%s,%s",
                      bitpos(scale), ireg_name, targ_name,
                      breg_name, targ_name, targ_name);
        return buf;
      }
    }

    if (breg != -1 && ireg != -1 && disp != 0 &&
        GET_CODE(disp) == CONST_INT &&
        INTVAL(disp) >= 0 && INTVAL(disp) <= 31)
    {
      char *targ_name = reg_names[target_reg];
      char *breg_name = reg_names[breg];
      char *ireg_name = reg_names[ireg];

      if (scale == 1)
      {
        if (breg != target_reg)
          sprintf (buf, "addo\t%d,%s,%s\n\taddo\t%s,%s,%s",
                        INTVAL(disp), ireg_name, targ_name,
                        breg_name, targ_name, targ_name);
        else if (ireg != target_reg)
          sprintf (buf, "addo\t%d,%s,%s\n\taddo\t%s,%s,%s",
                        INTVAL(disp), breg_name, targ_name,
                        ireg_name, targ_name, targ_name);
        else
          /* both are equal to target_reg and equal to each other. */
          sprintf (buf, "shlo\t1,%s,%s\n\taddo\t%d,%s,%s",
                        targ_name, targ_name,
                        INTVAL(disp), targ_name, targ_name);

        return buf;
      }

      if (!flag_space_opt && target_reg != breg)
      {
        sprintf (buf, "shlo\t%d,%s,%s\n\taddo\t%s,%s,%s\n\taddo\t%d,%s,%s",
                      bitpos(scale), ireg_name, targ_name,
                      breg_name, targ_name, targ_name,
                      INTVAL(disp), targ_name, targ_name);
        return buf;
      }
    }

    return ret_val;
  }

  return ret_val;
}

/* GO_IF_LEGITIMATE_ADDRESS recognizes an RTL expression
   that is a valid memory address for an instruction.
   The MODE argument is the machine mode for the MEM expression
   that wants to use this address.

	On 80960, legitimate addresses are:
		disp	(12 or 32 bit)		ld	foo,r0
		base				ld	(g0),r0
		base + displ			ld	0xf00(g0),r0
		index*scale + displ		ld	0xf00[g1*4],r0
		base + index*scale		ld	(g0)[g1*4],r0
		base + index*scale + displ	ld	0xf00(g0)[g1*4],r0

	in each case, scale can be 1,2,4,8, or 16

   We can also treat a SYMBOL_REF as legitimate if it is part of this
   function's constant-pool, because such addresses can actually
   be output as REG+SMALLINT.  */

/*
 * returns 1 if X is a valid indexing term.  This could either be a 
 * register only, or a the product of a register and a scale term
 * of size 1, 2, 4, 8, or 16.
 */

int
index_term_p(x, mode, strict)
register rtx x;
enum machine_mode mode;
int strict;
{
  /*
   * The following two cases deal with an index which has an implicit
   * scale of 1.
   */
  if (GET_CODE (x) == REG &&
      (strict ? REG_OK_FOR_INDEX_P_STRICT(x) : REG_OK_FOR_INDEX_P(x)))
    return 1;

  if (GET_CODE (x) == SUBREG &&
      GET_CODE (XEXP(x,0)) == REG &&
      (strict ? REG_OK_FOR_INDEX_P_STRICT(XEXP(x,0)) :
                REG_OK_FOR_INDEX_P(XEXP(x,0))))
    return 1;

  /*
   * The following deals with cases where the scale is not 1.
   */
  if (GET_CODE (x) == MULT)
  {
    /* handle constant in either first or second spot */
    if (GET_CODE (XEXP (x,0)) == CONST_INT && SCALE_TERM_P (XEXP (x,0)))
    {
      if (GET_CODE (XEXP(x,1)) == REG &&
          (strict ? REG_OK_FOR_INDEX_P_STRICT(XEXP(x,1)) :
                    REG_OK_FOR_INDEX_P (XEXP (x,1))))
        return 1;
      if (GET_CODE (XEXP(x,1)) == SUBREG &&
          GET_CODE (XEXP(XEXP(x,1),0)) == REG &&
          (strict ? REG_OK_FOR_INDEX_P_STRICT(XEXP(XEXP(x,1),0)) :
                    REG_OK_FOR_INDEX_P(XEXP(XEXP(x,1),0))))
        return 1;
    }
    if (GET_CODE (XEXP (x,1)) == CONST_INT && SCALE_TERM_P (XEXP (x,1)))
    {
      if (GET_CODE (XEXP (x,0)) == REG &&
          (strict ? REG_OK_FOR_INDEX_P_STRICT(XEXP(x,0)) :
                    REG_OK_FOR_INDEX_P (XEXP (x,0))))
        return 1;
      if (GET_CODE (XEXP(x,0)) == SUBREG &&
          GET_CODE (XEXP(XEXP(x,0),0)) == REG &&
          (strict ? REG_OK_FOR_INDEX_P_STRICT(XEXP(XEXP(x,0),0)) :
                    REG_OK_FOR_INDEX_P(XEXP(XEXP(x,0),0))))
        return 1;
    }
  }
  return 0;
}

int
nonindexed_address_p(x, strict)
register rtx x;
int strict;
{ 
  /* e.g.	ld	(g0),g1 */
  if (GET_CODE(x) == REG &&
      (strict ? REG_OK_FOR_BASE_P_STRICT(x) : REG_OK_FOR_BASE_P(x)))
    return 1;

  if (GET_CODE (x) == SUBREG && GET_CODE(XEXP(x,0)) == REG &&
      (strict ? REG_OK_FOR_BASE_P_STRICT(XEXP(x,0)) :
                REG_OK_FOR_BASE_P(XEXP(x,0))))
    return 1;

  /* e.g. ld	_foo,g0 */
  if (CONSTANT_ADDRESS_P(x))
    return 1;

  if (TARGET_ADDR_LEGAL != TARGET_FLAG_REG_OR_DIRECT_ONLY)
  {
    /* e.g. ld	_foo(g0),g1 */
    if (GET_CODE(x) == PLUS && CONSTANT_ADDRESS_P(XEXP (x, 1)))
    {
      if (GET_CODE(XEXP(x, 0)) == REG &&
          (strict ? REG_OK_FOR_BASE_P_STRICT(XEXP(x,0)) :
                    REG_OK_FOR_BASE_P(XEXP (x, 0))))
        return 1;

      if (GET_CODE(XEXP(x, 0)) == SUBREG &&
          GET_CODE(XEXP(XEXP(x,0),0)) == REG &&
          (strict ?  REG_OK_FOR_BASE_P_STRICT(XEXP(XEXP(x,0),0)) :
                      REG_OK_FOR_BASE_P(XEXP(XEXP(x,0),0))))
        return 1;
    }
  }

  return 0;
}

int
indexing_p(x, mode, strict)
register rtx x;
enum machine_mode mode;
int strict;
{
  if (TARGET_ADDR_LEGAL == TARGET_FLAG_REG_OR_DIRECT_ONLY ||
      TARGET_ADDR_LEGAL == TARGET_FLAG_REG_OFF_ADDR)
    return 0;

  if (GET_CODE(x) == PLUS)
  {
    if (index_term_p(XEXP(x, 0), mode, strict))
    {
      if (CONSTANT_ADDRESS_P(XEXP (x, 1)))
        return 1;

      if (TARGET_ADDR_LEGAL == TARGET_FLAG_ALL_ADDR &&
          nonindexed_address_p(XEXP(x, 1), strict))
        return 1;
    }

    if (index_term_p(XEXP(x, 1), mode, strict))
    {
      if (CONSTANT_ADDRESS_P(XEXP (x, 0)))
        return 1;

      if (TARGET_ADDR_LEGAL == TARGET_FLAG_ALL_ADDR &&
          nonindexed_address_p(XEXP(x, 0), strict))
        return 1;
    }
  }

  if (index_term_p(x, mode, strict))
    return 1;

  return 0;
}

const_plus_index_p(x, mode, strict)
register rtx x;
enum machine_mode mode;
int strict;
{ 
  if (GET_CODE(x) == PLUS)
  { 
    if (CONSTANT_ADDRESS_P (XEXP (x, 0)) &&
        index_term_p (XEXP (x, 1), mode, strict))
      return 1;
    if (CONSTANT_ADDRESS_P (XEXP (x, 1)) &&
        index_term_p (XEXP (x, 0), mode, strict))
      return 1;
  } 
  return 0;
}


/* Go to ADDR if X is the sum of a register
   and a valid index term for mode MODE.  */
int
reg_plus_index_p(x, mode, strict)
register rtx x;
enum machine_mode mode;
int strict;
{
  if (GET_CODE (x) == PLUS)
  { 
    /* ld	(g0)[g1*4],g2 */
    if (GET_CODE (XEXP(x, 0)) == REG &&
        (strict ? REG_OK_FOR_BASE_P_STRICT(XEXP(x,0)) :
                  REG_OK_FOR_BASE_P(XEXP(x,0))) &&
        index_term_p(XEXP(x,1), mode, strict))
      return 1;

    if (GET_CODE (XEXP(x,0)) == SUBREG &&
        GET_CODE (XEXP(XEXP(x,0),0)) == REG &&
        (strict ? REG_OK_FOR_BASE_P_STRICT(XEXP(XEXP(x,0),0)) :
                  REG_OK_FOR_BASE_P(XEXP(XEXP(x,0),0))) &&
        index_term_p(XEXP(x,1), mode, strict))
      return 1;

    /* other order */
    if (GET_CODE (XEXP(x,1)) == REG &&
        (strict ? REG_OK_FOR_BASE_P_STRICT(XEXP(x,1)) :
                  REG_OK_FOR_BASE_P(XEXP(x,1))) &&
        index_term_p (XEXP(x,0), mode, strict))
      return 1;

    if (GET_CODE (XEXP(x,1)) == SUBREG &&
        GET_CODE (XEXP(XEXP(x,1),0)) == REG &&
        (strict ? REG_OK_FOR_BASE_P_STRICT(XEXP(XEXP(x,1),0)) :
                  REG_OK_FOR_BASE_P (XEXP(XEXP(x,1),0))) &&
        index_term_p (XEXP(x,0), mode, strict))
      return 1;
  } 
  return 0;
}

int
indexed_address_p(x,mode,strict)
register rtx x;
enum machine_mode mode;
int strict;
{
  if (TARGET_ADDR_LEGAL != TARGET_FLAG_ALL_ADDR)
    return 0;

  if (GET_CODE (x) == PLUS)
  { 
    if (GET_CODE (XEXP (x, 0)) == PLUS)
    {
      if (GET_CODE (XEXP (x, 1)) == REG &&
          (strict ? REG_OK_FOR_BASE_P_STRICT(XEXP(x,1)) :
                    REG_OK_FOR_BASE_P(XEXP(x,1))) &&
          const_plus_index_p(XEXP (x, 0), mode, strict))
        return 1;

      if (GET_CODE (XEXP(x,1)) == SUBREG &&
          GET_CODE (XEXP(XEXP(x,1),0)) == REG &&
          (strict ? REG_OK_FOR_BASE_P_STRICT(XEXP(XEXP(x,1),0)) :
                    REG_OK_FOR_BASE_P (XEXP(XEXP(x,1),0))) &&
          const_plus_index_p(XEXP (x, 0), mode, strict))
        return 1;

      if (CONSTANT_ADDRESS_P(XEXP (x, 1)) &&
          reg_plus_index_p(XEXP (x, 0), mode, strict))
        return 1;
    }

    if (GET_CODE(XEXP (x, 1)) == PLUS)
    {
      if (GET_CODE (XEXP(x, 0)) == REG &&
          (strict ? REG_OK_FOR_BASE_P_STRICT(XEXP(x,0)) :
                    REG_OK_FOR_BASE_P(XEXP(x,0))) &&
          const_plus_index_p(XEXP (x, 1), mode, strict))
        return 1;

      if (GET_CODE (XEXP(x,0)) == SUBREG &&
          GET_CODE (XEXP(XEXP(x,0),0)) == REG &&
          (strict ? REG_OK_FOR_BASE_P_STRICT(XEXP(XEXP(x,0),0)) :
                    REG_OK_FOR_BASE_P(XEXP(XEXP(x,0),0))) &&
          const_plus_index_p(XEXP (x, 1), mode, strict))
        return 1;

      if (CONSTANT_ADDRESS_P(XEXP (x, 0)) &&
          reg_plus_index_p(XEXP (x, 1), mode, strict))
        return 1;
    }
  }
  return 0;
}

int
legitimate_address_p(mode,x,strict)
enum machine_mode mode;
register rtx x;
int strict;
{
  if (GET_CODE(x)==SCRATCH || CONSTANT_P(x))
    return 1;

  if (nonindexed_address_p(x,strict)) 
    return 1;

  if (indexing_p(x,mode,strict))
    return 1;

  if (indexed_address_p(x,mode,strict)) 
    return 1;

  return 0;
}

void
i960_adjust_legal_addr(adjust)
int adjust;
{
  static int orig_legal_addr;

  if (adjust)
  {
    orig_legal_addr = TARGET_ADDR_LEGAL;

    if (i960_ok_to_adjust)
    {
      if (TARGET_C_SERIES)
      {
        target_flags &= ~TARGET_FLAG_ADDR_MASK;
        target_flags |= TARGET_FLAG_ALL_ADDR;
      }

      if (TARGET_K_SERIES)
      {
        target_flags &= ~TARGET_FLAG_ADDR_MASK;
        target_flags |= TARGET_FLAG_ALL_ADDR;
      }

      if (TARGET_J_SERIES &&
          (orig_legal_addr & TARGET_FLAG_INDEX_ADDR) == 0)
      {
        target_flags &= ~TARGET_FLAG_ADDR_MASK;
        target_flags |= TARGET_FLAG_INDEX_ADDR;
      }
    }
  }
  else
  {
    target_flags &= ~TARGET_FLAG_ADDR_MASK;
    target_flags |= orig_legal_addr;
  }
}

int
i960_const_pic_pid_p(x, want_pic, want_pid)
rtx x;
int want_pic;
int want_pid;
{
  int retval = 0;

  switch (GET_CODE(x))
  {
    case LABEL_REF:
      if (want_pic)
        return 1;
      return 0;

    case CONST:
      if (GET_CODE(XEXP(x,0)) == PLUS || GET_CODE(XEXP(x,0)) == MINUS)
        return i960_const_pic_pid_p(XEXP(XEXP(x,0),0), want_pic, want_pid) ||
               i960_const_pic_pid_p(XEXP(XEXP(x,0),1), want_pic, want_pid);

    case SYMBOL_REF:
      if (want_pic)
        retval |= ((SYMREF_ETC(x) & SYMREF_PICBIT) != 0);

      if (want_pid)
        retval |= ((SYMREF_ETC(x) & SYMREF_PIDBIT) != 0);

      return retval;

    default:
      return 0;
  }
}

/* Try machine-dependent ways of modifying an illegitimate address
   to be legitimate.  If we find one, return the new, valid address.
   This macro is used in only one place: `memory_address' in explow.c.

   This converts some non-canonical addresses to canonical form so they
   can be recognized.  */

rtx
legitimize_address (x, oldx, mode)
     register rtx x;
     register rtx oldx;
     enum machine_mode mode;
{ 
  if (TARGET_ADDR_LEGAL == TARGET_FLAG_REG_OR_DIRECT_ONLY ||
      TARGET_ADDR_LEGAL == TARGET_FLAG_REG_OFF_ADDR)
    return x;

  /* Canonicalize (plus (mult (reg) (const)) (plus (reg) (const)))
     into (plus (plus (mult (reg) (const)) (reg)) (const)).  This can be
     created by virtual register instantiation, register elimination, and
     similar optimizations.  */
  if (GET_CODE (x) == PLUS && GET_CODE (XEXP (x, 0)) == MULT
      && GET_CODE (XEXP (x, 1)) == PLUS)
    x = gen_rtx (PLUS, Pmode,
		 gen_rtx (PLUS, Pmode, XEXP (x, 0), XEXP (XEXP (x, 1), 0)),
		 XEXP (XEXP (x, 1), 1));

  /* Canonicalize (plus (plus (mult (reg) (const)) (plus (reg) (const))) const)
     into (plus (plus (mult (reg) (const)) (reg)) (const)).  */
  else if (GET_CODE (x) == PLUS && GET_CODE (XEXP (x, 0)) == PLUS
	   && GET_CODE (XEXP (XEXP (x, 0), 0)) == MULT
	   && GET_CODE (XEXP (XEXP (x, 0), 1)) == PLUS
	   && CONSTANT_P (XEXP (x, 1)))
    {
      rtx constant, other;

      if (GET_CODE (XEXP (x, 1)) == CONST_INT)
	{
	  constant = XEXP (x, 1);
	  other = XEXP (XEXP (XEXP (x, 0), 1), 1);
	}
      else if (GET_CODE (XEXP (XEXP (XEXP (x, 0), 1), 1)) == CONST_INT)
	{
	  constant = XEXP (XEXP (XEXP (x, 0), 1), 1);
	  other = XEXP (x, 1);
	}
      else
	constant = 0;

      if (constant)
	x = gen_rtx (PLUS, Pmode,
		     gen_rtx (PLUS, Pmode, XEXP (XEXP (x, 0), 0),
			      XEXP (XEXP (XEXP (x, 0), 1), 0)),
		     plus_constant (other, INTVAL (constant)));
    }

  return x;
}

#if 0
/* Return the most stringent alignment that we are willing to consider
   objects of size SIZE and known alignment ALIGN as having. */
   
int
i960_alignment (size, align)
     int size;
     int align;
{
  int i;

  if (! TARGET_STRICT_ALIGN)
    if (TARGET_IC_COMPAT2_0 || align >= 4)
      {
	i = i960_object_bytes_bitalign (size) / BITS_PER_UNIT;
	if (i > align)
	  align = i;
      }

  return align;
}
#endif

/* Return the minimum alignment of an expression rtx X in bytes.  This takes
   advantage of machine specific facts, such as knowing that the frame pointer
   is always 16 byte aligned.  */

int
i960_expr_alignment (x, size)
     rtx x;
     int size;
{
  int align = 1;

  if (x == 0)
    return 1;

  switch (GET_CODE(x))
    {
    case CONST_INT:
      align = INTVAL(x);

      if ((align & 0xf) == 0)
	align = 16;
      else if ((align & 0x7) == 0)
	align = 8;
      else if ((align & 0x3) == 0)
	align = 4;
      else if ((align & 0x1) == 0)
	align = 2;
      else
	align = 1;
      break;

    case PLUS:
      align = MIN (i960_expr_alignment (XEXP (x, 0), size),
		   i960_expr_alignment (XEXP (x, 1), size));
      break;

    case SYMBOL_REF:
      if (SYMREF_SIZE(x) > size)
        /* If we have an allocation size bigger than the reference, use it */
        align = i960_object_bytes_bitalign (SYMREF_SIZE(x)) / BITS_PER_UNIT;
      else
        /* If this is a valid program, objects are guaranteed to be
	   correctly aligned for whatever size the reference actually is. */
        align = i960_object_bytes_bitalign (size) / BITS_PER_UNIT;

      break;

    case REG:
      if (REGNO (x) == FRAME_POINTER_REGNUM ||
          REGNO (x) == ARG_POINTER_REGNUM)
	align = 16;
      break;

    case ASHIFT:
    case LSHIFT:
      align = i960_expr_alignment (XEXP (x, 0), size);

      if (GET_CODE (XEXP (x, 1)) == CONST_INT)
	{
	  align = align << INTVAL (XEXP (x, 1));
	  align = MIN (align, 16);
	}
      break;

    case MULT:
      align = (i960_expr_alignment (XEXP (x, 0), size) *
	       i960_expr_alignment (XEXP (x, 1), size));

      align = MIN (align, 16);
      break;
    }

  return align;
}

/* Return true if it is possible to reference both BASE and OFFSET, which
   have alignment at least as great as 4 byte, as if they had alignment valid
   for an object of size SIZE.  */

int
i960_improve_align (base, offset, size)
     rtx base;
     rtx offset;
     int size;
{
  int i, j;

  /* We have at least a word reference to the object, so we know it has to
     be aligned at least to 4 bytes.  */

  i = MIN (i960_expr_alignment (base, 4),
	   i960_expr_alignment (offset, 4));

  i = MAX (i, 4);

  /* We know the size of the request.  If strict align is not enabled, we
     can guess that the alignment is OK for the requested size.  */

  if (! TARGET_STRICT_ALIGN)
    if ((j = (i960_object_bytes_bitalign (size) / BITS_PER_UNIT)) > i)
      i = j;

  return (i >= size);
}

/* Return true if it is possible to access BASE and OFFSET, which have 4 byte
   (SImode) alignment as if they had 16 byte (TImode) alignment.  */

int
i960_si_ti (base, offset)
     rtx base;
     rtx offset;
{
  return i960_improve_align (base, offset, 16);
}

/* Return true if it is possible to access BASE and OFFSET, which have 4 byte
   (SImode) alignment as if they had 8 byte (DImode) alignment.  */

int
i960_si_di (base, offset)
     rtx base;
     rtx offset;
{
  return i960_improve_align (base, offset, 8);
}

void 
i960_arg_size_and_align (mode, type, size_out, align_out)
enum machine_mode mode;
tree type;
int* size_out;
int* align_out;
{
  int size, align;

  /*  Use formal alignment req of type being passed, except
      make it at least a word.  If we don't have a type, this
      is a library call, and the parm has to be of scalar type.
      In this case, consider its formal alignment requirement
      to be its size in words.  TMC Summer 90 */

  if ((mode) == BLKmode)
    size = (int_size_in_bytes(type)+3)/4;

  else if (mode==VOIDmode)
  { /* End of parm list. */
    assert (type != 0 && TYPE_MODE(type)==VOIDmode);
    size = 1;
  }
  else
    size = (GET_MODE_SIZE(mode)+3)/4;

  if (type == 0)
    align = size;
  else
    if (TYPE_ALIGN(type) >= BITS_PER_WORD)
      align = TYPE_ALIGN(type)/BITS_PER_WORD;
    else
      align = 1;

  *size_out  = size;
  *align_out = align;
}

/* On 80960 the first 12 args are in registers and the rest are pushed.
   Any arg that is bigger than 4 words is placed on the stack and all
   subsequent arguments are placed on the stack.

   Additionally, parameters with an alignment requirement stronger than
   a word must be be aligned appropriately. 

   It used to be that 'named' meant someting here;  it doesn't anymore.
   The last parm of a varargs routine is treated like anybody else for
   purposes of figuring his offset.

   This stuff rewritten to support both kinds of alignment rules,
   ANSI stdarg, and to fix various parameter passing bugs by
   TMC Summer 1990.
*/
  
i960_function_arg_advance(cum, mode, type, named)
CUMULATIVE_ARGS *cum;
enum machine_mode mode;
tree type;
int named;
{
  int size,align;

  i960_arg_size_and_align (mode, type, &size, &align);

  if (size>4 ||cum->ca_nstackparms!=0 ||(size+ROUND(cum->ca_nregparms,align)) >NPARM_REGS)
    cum->ca_nstackparms = ROUND (cum->ca_nstackparms, align) + size;
  else
    cum->ca_nregparms   = ROUND (cum->ca_nregparms, align) + size;
}

rtx
i960_function_arg(cum,mode,type,named)
CUMULATIVE_ARGS *cum;
enum machine_mode mode;
tree type;
int named;
{
  rtx ret;
  int size,align;

  i960_arg_size_and_align (mode, type, &size, &align);

  if (size>4 ||cum->ca_nstackparms!=0 ||(size+ROUND(cum->ca_nregparms,align)) >NPARM_REGS)
  { cum->ca_nstackparms = ROUND (cum->ca_nstackparms, align);
    ret = 0;
  }
  else
  { cum->ca_nregparms  = ROUND (cum->ca_nregparms, align);
    ret = gen_rtx (REG, mode, cum->ca_nregparms);

    if (type == 0)
      /* Library call - pick a safe type for DFA from mode */
      set_rtx_mode_type (ret);
    else
      set_rtx_type (ret, type);
  }

  return ret;
}

/*  Return size of named arguments. */
rtx
i960_argsize_rtx()
{
  rtx ret;

  extern CUMULATIVE_ARGS current_function_args_info;
         CUMULATIVE_ARGS *cum;

  cum = &current_function_args_info;

  if (cum->ca_nstackparms)
    ret = gen_rtx (CONST_INT, VOIDmode, 48 + cum->ca_nstackparms*4);
  else
    ret = gen_rtx (CONST_INT, VOIDmode, cum->ca_nregparms*4);

  return ret;
}

/* Return the rtx for the register representing the return value, or 0
   if the return value must be passed through the stack.  */

rtx
i960_function_value (type)
     tree type;
{
  enum machine_mode mode = TYPE_MODE (type);

  if (mode == BLKmode)
    {
      unsigned int size = int_size_in_bytes (type);

      if (size <= 16)
	mode = mode_for_size (i960_object_bytes_bitalign (size), MODE_INT, 0);
    }

  if (mode == BLKmode || mode == VOIDmode)
    /* Tell stmt.c and expr.c to pass in address */
    return 0;
  else
    return set_rtx_type (gen_rtx (REG, mode,
                                  (TARGET_NUMERICS && mode == TFmode &&
                                   TARGET_IC_COMPAT2_0) ? 32 : 0),
                         type);
}

/* Floating-point support.  */

void
i960_output_long_double (file, value)
FILE *file;
REAL_VALUE_TYPE value;
{
  long f[4];
#ifdef IMSTG_REAL
  REAL_VALUE_TO_TARGET_LONG_DOUBLE(value,f);
#else
  dbl_to_i960_edbl(value, f);
#endif

  fprintf (file, "\t.word 0x%x\n", f[TW0]);
  fprintf (file, "\t.word 0x%x\n", f[TW1]);
  fprintf (file, "\t.word 0x%x\n", f[TW2]);
  fprintf (file, "\t.word 0x%x\n", 0);
}

void
i960_output_double (file, value)
FILE *file;
REAL_VALUE_TYPE value;
{
  long f[4];
#ifdef IMSTG_REAL
  REAL_VALUE_TO_TARGET_DOUBLE(value,f);
#else
  dbl_to_ieee_dbl (value, f);
#endif

  fprintf (file, "\t.word 0x%x\n", f[DW0]);
  fprintf (file, "\t.word 0x%x\n", f[DW1]);
}

void
i960_output_float (file, value)
FILE *file;
REAL_VALUE_TYPE value;
{
  unsigned f[4];
#ifdef IMSTG_REAL
  REAL_VALUE_TO_TARGET_SINGLE(value,f[0]);
#else
  dbl_to_ieee_sgl (value, f);
#endif

  fprintf (file, "\t.word 0x%x\n", f[0]);
}

/* Return the number of bits that an object of size N bytes is aligned to.  */

int
i960_object_bytes_bitalign (n)
     int n;
{
  if (n > 8)      n = 128;
  else if (n > 4) n = 64;
  else if (n > 2) n = 32;
  else if (n > 1) n = 16;
  else            n = 8;

  return n;
}

/* Do any needed setup for a varargs function.  For the i960, we must
   create a register paramter block if one doesn't exist, and then copy
   all register parameters to memory.  */

void
i960_setup_incoming_varargs (cum, mode, type, pretend_size, no_rtl)
CUMULATIVE_ARGS *cum;
enum machine_mode mode;
tree type;
int *pretend_size;
int no_rtl;
{
  
  int first_reg = cum->ca_nregparms;

  *pretend_size = 0;
  last_varargs_func = current_function_decl;

  /* Copy only unnamed register arguments to memory.  If there are
     any stack parms, there are no unnamed arguments in register, and
     an argument block was already allocated by the caller.

     If there are no stack arguments but there are exactly NPARM_REGS
     registers, either there were no extra arguments or the caller
     allocated an argument block. */

  if (cum->ca_nstackparms==0 && first_reg < NPARM_REGS && !no_rtl)
  {
    rtx label = gen_label_rtx ();
    rtx ap    = current_function_internal_arg_pointer;
    rtx addr  = XEXP(assign_stack_local (BLKmode, 48, BIGGEST_ALIGNMENT), 0);

    emit_insn (gen_cmpsi (ap, const0_rtx));
    emit_jump_insn (gen_bne (label));
    emit_insn (gen_rtx (SET,VOIDmode,ap, memory_address(BLKmode,addr)));
    emit_label (label);

    addr = memory_address (BLKmode, plus_constant (ap,first_reg*4));

    move_block_from_reg
      (first_reg, gen_typed_mem_rtx(void_type_node,BLKmode,addr),
       NPARM_REGS-first_reg, (NPARM_REGS-first_reg) * UNITS_PER_WORD);
  }
}

/* Calculate the final size of the reg parm stack space for the current
   function, based on how many bytes would be allocated on the stack.  */

int
i960_final_reg_parm_stack_space (const_size, var_size)
     int const_size;
     tree var_size;
{
  if (var_size || const_size > 48)
    return 48;
  else
    return 0;
}

/* Calculate the size of the reg parm stack space.  This is a bit complicated
   on the i960.  */

int
i960_reg_parm_stack_space (fndecl)
     tree fndecl;
{
  /* In this case, we are called from emit_library_call, and we don't need
     to pretend we have more space for parameters than what's apparent.  */
  if (fndecl == 0)
    return 0;

  return 48;
}

/* Return the register class of a scratch register needed to copy IN into
   or out of a register in CLASS in MODE.  If it can be done directly,
   NO_REGS is returned.  */

#define LOCAL_OR_GLOBAL_CLASS(c) (\
  (c)==GLOBAL_REGS||\
  (c)==LOCAL_REGS||\
  (c)==LOCAL_OR_GLOBAL_QREGS||\
  (c)==LOCAL_OR_GLOBAL_DREGS||\
  (c)==LOCAL_OR_GLOBAL_REGS\
)

enum reg_class
secondary_reload_class (class, mode, in)
     enum reg_class class;
     enum machine_mode mode;
     rtx in;
{
  int regno = -1;

  assert (class != NO_REGS && class != LIM_REG_CLASSES);

  if (GET_CODE (in) == REG || GET_CODE (in) == SUBREG)
    regno = true_regnum (in);

#ifdef IMSTG_COPREGS
  /* For co-processor registers, can copy within same class or to/from 960. */
  if (IS_COP_CLASS(class))
    if ((regno>=0 && regno<32) ||i960_reg_class(regno)==class)
      return NO_REGS;
    else
      return LOCAL_OR_GLOBAL_REGS;
  else
    if (IS_COP_REG(regno))
      if (LOCAL_OR_GLOBAL_CLASS(class) || i960_reg_class(regno) == class)
        return NO_REGS;
      else
        return LOCAL_OR_GLOBAL_REGS;
#endif

  if (regno >= 0 && regno < 32)
    return NO_REGS;

  if (class == FP_REGS ||
      class == FP_OR_QREGS ||
      class == FP_OR_DREGS ||
      class == ALL_REGS)
  {
    /* We can place any hard register, 0.0, and 1.0 into FP_REGS.  */
    if ((regno >= 0 && regno <= FIRST_PSEUDO_REGISTER) ||
        in == CONST0_RTX (mode) || in == CONST1_RTX (mode))
      return NO_REGS;

    if (GET_MODE_SIZE(mode) <= 4)
      return LOCAL_OR_GLOBAL_REGS;
    else if (GET_MODE_SIZE(mode) <= 8)
      return LOCAL_OR_GLOBAL_DREGS;
    else
      return LOCAL_OR_GLOBAL_QREGS;
  }

  return NO_REGS;
}

/* Emit the code necessary for a procedure call.  Return value is needed
   after the call if target is non-zero.  */

void
i960_expand_return ()
{
  emit_jump_insn (gen_rtx (RETURN, VOIDmode));
}

rtx current_function_argptr_insn;	/* insn which did argptr copy */

/* Attach call_fusage to the last CALL_INSN */

static void
attach_call_fusage(call_fusage)
rtx call_fusage;
{
  rtx call_insn;

  /* Find the CALL insn we just emitted.  */
  for (call_insn = get_last_insn ();
       call_insn && GET_CODE (call_insn) != CALL_INSN;
       call_insn = PREV_INSN (call_insn))
    ;

  if (! call_insn)
    abort ();

  /* Put the register usage information on the CALL.  If there is already
     some usage information, put ours at the end.  */
  if (CALL_INSN_FUNCTION_USAGE (call_insn))
    {
      rtx link;

      for (link = CALL_INSN_FUNCTION_USAGE (call_insn); XEXP (link, 1) != 0;
	   link = XEXP (link, 1))
	;

      XEXP (link, 1) = call_fusage;
    }
  else
    CALL_INSN_FUNCTION_USAGE (call_insn) = call_fusage;
}

void
i960_expand_call (op1, op2, op3, target)
rtx op1, op2, op3, target;
{
  /* op1 is the function to call.
   * op2 is the amount of junk that has gone in the argblock so far.
   * op3 is the next argument register available for a parameter.
   * If op3 is 0 then its means we went into the argblock with the
   * parameters.
   */

  /* Used to ensure that g14 is set to 0 once and only once
     for each function if it is needed.  */

  static char *this_function_name = 0;
  int need_argblk;
  rtx clobber_reg = gen_rtx (REG, TImode, 4);
  rtx call_rtx;
  rtx call_fusage = 0; /* Points to USE expressions for function's arguments */

  if (this_function_name != current_function_name)
  {
    this_function_name = current_function_name;

    /* If the current function has an argument block, then set g14 to
       zero at function entry. */

    if (current_func_has_argblk())
    {
      rtx seq;

      start_sequence ();
      emit_insn (gen_rtx (SET, VOIDmode, arg_pointer_rtx, const0_rtx));
      emit_insn (gen_rtx (USE, VOIDmode, arg_pointer_rtx));
      seq = gen_sequence ();
      end_sequence ();
      emit_insn_after (seq, current_function_argptr_insn);
      i960_func_changes_g14 = 1;
    }
  }

  need_argblk = (GET_CODE(op2)!= CONST_INT || INTVAL(op2)>48);

  if (need_argblk)
  {
    emit_insn (gen_rtx(SET,VOIDmode,arg_pointer_rtx,
                           virtual_outgoing_args_rtx));
    use_reg(&call_fusage, arg_pointer_rtx);
    i960_func_changes_g14 = 1;
  }

  /*
   * If we needed an argblock, or we used any parameter register
   * g8 or above, assume that g8,g9,g10,g11 are clobbered.
   */
  if (need_argblk || op3 == 0 ||
      GET_CODE(op3) != REG || REGNO(op3) > 8)
  {
    REGNO(clobber_reg) = 8;
  }

  
  if (target)
    call_rtx = gen_rtx (SET, VOIDmode, target,
                        gen_rtx (CALL,VOIDmode,op1,op2));
  else
    call_rtx = gen_rtx (CALL, VOIDmode, op1, op2);

  emit_call_insn (gen_rtx (PARALLEL, VOIDmode, gen_rtvec(2,
                           call_rtx,
                           gen_rtx (CLOBBER, VOIDmode, clobber_reg))));

  /* Attach call_fusage if not empty to the last CALL_INSN */

  if (call_fusage)
    attach_call_fusage(call_fusage);

  if (need_argblk)
  {
    emit_insn (gen_rtx (SET, VOIDmode, arg_pointer_rtx, const0_rtx));
    emit_insn (gen_rtx (USE, VOIDmode, arg_pointer_rtx));
    i960_func_changes_g14 = 1;
  }
}

/* Look at the opcode P, and set i96_last_insn_type to indicate which
   function unit it executed on.  */

/* ??? This would make more sense as an attribute.  */

void
i960_scan_opcode (p)
     char *p;
{
  switch (*p)
    {
    case 'a':
    case 'd':
    case 'e':
    case 'm':
    case 'n':
    case 'o':
    case 'r':
      /* Ret is not actually of type REG, but it won't matter, because no
	 insn will ever follow it.  */
    case 'u':
    case 'x':
      i960_last_insn_type = I_TYPE_REG;
      break;

    case 'b':
      if (p[1] == 'x' || p[3] == 'x')
        i960_last_insn_type = I_TYPE_MEM;
      i960_last_insn_type = I_TYPE_CTRL;
      break;

    case 'f':
    case 't':
      i960_last_insn_type = I_TYPE_CTRL;
      break;

    case 'c':
      if (p[1] == 'a')
	{
	  if (p[4] == 'x')
	    i960_last_insn_type = I_TYPE_MEM;
	  else
	    i960_last_insn_type = I_TYPE_CTRL;
	}
      else if (p[1] == 'm')
	{
	  if (p[3] == 'd')
	    i960_last_insn_type = I_TYPE_REG;
	  else if (p[4] == 'b' || p[4] == 'j')
	    i960_last_insn_type = I_TYPE_CTRL;
	  else
	    i960_last_insn_type = I_TYPE_REG;
	}
      else
        i960_last_insn_type = I_TYPE_REG;
      break;

    case 'l':
      i960_last_insn_type = I_TYPE_MEM;
      break;

    case 's':
      if (p[1] == 't')
        i960_last_insn_type = I_TYPE_MEM;
      else
        i960_last_insn_type = I_TYPE_REG;
      break;
    }
}


int
i960_round_type_align (t, align)
tree t;
int align;
{
  assert (t != 0);

  if (TREE_CODE(t) == RECORD_TYPE || TREE_CODE(t) == UNION_TYPE)
  { tree s;
    if ((s=TYPE_SIZE(t)) != 0 && TREE_CODE(s) == INTEGER_CST)
    { int i;

      i = i960_object_bytes_bitalign (TREE_INT_CST_LOW(s)/ BITS_PER_UNIT);

      /* restrict with #pragma align.  */
      if (i > TREE_GET_ALIGN_LIMIT (t) || TYPE_IMSTG_I960ALIGN(t))
        i = TREE_GET_ALIGN_LIMIT (t);

      if (align < i)
        align = i;
    }
  }

  return align;
}

tree
i960_round_type_size (type, size, align)
tree type;
tree size;
int  align;
{
  return round_up (size, i960_round_type_align (type, align));
}

rtx
i960_setup_hard_parm_reg(func_decl)
tree func_decl;
{
  int i;

  bzero (df_hard_reg, sizeof(df_hard_reg));

  for (i = 0; i < 32; i++)
    set_rtx_type((df_hard_reg[i].reg=gen_rtx(REG,SImode,i)),void_type_node);

  if (TARGET_NUMERICS)
    for (i = 32; i < 36; i++)
      set_rtx_type((df_hard_reg[i].reg=gen_rtx(REG,TFmode,i)),long_double_type_node);

  if (last_varargs_func == current_function_decl)
    for (i = 0; i < NPARM_REGS; i++)
      df_hard_reg[i].is_parm = 1;
}

i960_setup_pseudo_frame(pseudo_frame_reg, frame_size, insn)
rtx pseudo_frame_reg;
int frame_size;
rtx insn;
{
  frame_size += 64;
  frame_size = ROUND(frame_size, 16);

  emit_insn_before(gen_rtx(SET, Pmode,
                           pseudo_frame_reg,
                           stack_pointer_rtx),
                   insn);

  emit_insn_before(gen_rtx(SET, Pmode,
                           stack_pointer_rtx,
                           gen_rtx(PLUS, Pmode,
                                   stack_pointer_rtx,
                                   gen_rtx(CONST_INT, VOIDmode,
                                           frame_size))),
                   insn);

  emit_insn_before(gen_rtx(USE, Pmode, stack_pointer_rtx),
                   insn);
}

i960_cleanup_pseudo_frame(pseudo_frame_reg, frame_size, insn)
rtx pseudo_frame_reg;
int frame_size;
rtx insn;
{
  emit_insn_after (gen_rtx(USE, Pmode, stack_pointer_rtx),
                   insn);

  emit_insn_after (gen_rtx(SET, Pmode,
                           stack_pointer_rtx,
                           pseudo_frame_reg),
                   insn);

}

i960_pre_final_opt (decl, insns)
tree decl;
rtx insns;
{
  /*
   * dump_func_info must happen before dump_call_graph_info otherwise, well
   * be screwed up.
   */
  if (BUILD_DB || PROF_CODE)
    dump_func_info(decl);

#if 0
  if (flag_place_functions)
  {
    /* get the counts approximately right for the calls */
    update_expect(decl, insns);
    /* put out the call graph that the linker will use for
       function placement. */
    dump_call_graph_info(XSTR(XEXP(DECL_RTL(decl),0), 0), insns);
  }
#endif
}

#if 0
char *noalias_libcall[] = {
	"__absdf2",
	"__abssf2",
	"__adddf3",
	"__addsf3",
	"__cmpdf2",
	"__cmpsf2",
	"__divdf3",
	"__divsf3",
	"__extendsfdf2",
	"__fixdfsi",
	"__fixunsdfsi",
	"__floatsidf",
	"__muldf3",
	"__mulsf3",
	"__negdf2",
	"__negsf2",
	"__subdf3",
	"__subsf3",
	"__truncdfsf2",
	0,
};
#endif

enum optype { BINOP,CMPOP,TSTOP,UNOP } type;
struct libcalls {
	enum insn_code	icode;
	char *lib;
	enum rtx_code	rcode;
	enum optype	type;
	int		noalias_p;
} libs[] = {
#ifdef HAVE_cmptf
	{CODE_FOR_cmptf,	"__cmptf2",	COMPARE,	CMPOP, 1},
#endif
#ifdef HAVE_cmpdf
	{CODE_FOR_cmpdf,	"__cmpdf2",	COMPARE,	CMPOP, 1},
#endif
#ifdef HAVE_cmpsf
	{CODE_FOR_cmpsf,	"__cmpsf2",	COMPARE,	CMPOP, 1},
#endif

#ifdef HAVE_tstdf
	{CODE_FOR_tstdf,	"__cmpdf2",	COMPARE,	TSTOP, 1},
#endif
#ifdef HAVE_tstdf
	{CODE_FOR_tstdf,	"__cmpdf2",	COMPARE,	TSTOP, 1},
#endif
#ifdef HAVE_tstsf
	{CODE_FOR_tstsf,	"__cmpsf2",	COMPARE,	TSTOP, 1},
#endif

#ifdef HAVE_extendsftf2
	{CODE_FOR_extendsftf2, 	"__extendsftf2",FLOAT_EXTEND,	UNOP, 1},
#endif
#ifdef HAVE_extenddftf2
	{CODE_FOR_extenddftf2, 	"__extenddftf2",FLOAT_EXTEND,	UNOP, 1},
#endif
#ifdef HAVE_extendsfdf2
	{CODE_FOR_extendsfdf2, 	"__extendsfdf2",FLOAT_EXTEND,	UNOP, 1},
#endif

#ifdef HAVE_trunctfsf2
	{CODE_FOR_trunctfsf2, 	"__trunctfsf2",	FLOAT_TRUNCATE,	UNOP, 1},
#endif
#ifdef HAVE_trunctfdf2
	{CODE_FOR_trunctfdf2, 	"__trunctfdf2",	FLOAT_TRUNCATE,	UNOP, 1},
#endif
#ifdef HAVE_truncdfsf2
	{CODE_FOR_truncdfsf2, 	"__truncdfsf2",	FLOAT_TRUNCATE,	UNOP, 1},
#endif

#ifdef HAVE_fixuns_trunctfsi2
	{CODE_FOR_fixuns_trunctfsi2,"__fixunstfsi",TRUNCATE,	UNOP, 1},
#endif
#ifdef HAVE_fixuns_trunctfdi2
	{CODE_FOR_fixuns_trunctfdi2,"__fixunstfdi",TRUNCATE,	UNOP, 1},
#endif
#ifdef HAVE_fixuns_truncdfdi2
	{CODE_FOR_fixuns_truncdfdi2,"__fixunsdfdi",TRUNCATE,	UNOP, 1},
#endif
#ifdef HAVE_fixuns_truncsfdi2
	{CODE_FOR_fixuns_truncsfdi2,"__fixunssfdi",TRUNCATE,	UNOP, 1},
#endif
#ifdef HAVE_fixuns_truncdfsi2
	{CODE_FOR_fixuns_truncdfsi2,"__fixunsdfsi",TRUNCATE,	UNOP, 1},
#endif
#ifdef HAVE_fixuns_truncsfsi2
	{CODE_FOR_fixuns_truncsfsi2,"__fixunssfsi",TRUNCATE,	UNOP, 1},
#endif

#ifdef HAVE_fix_trunctfdi2
	{CODE_FOR_fix_trunctfdi2,"__fixtfdi",	FIX,		UNOP, 1},
#endif
#ifdef HAVE_fix_trunctfsi2
	{CODE_FOR_fix_trunctfsi2,"__fixtfsi",	FIX,		UNOP, 1},
#endif
#ifdef HAVE_fix_truncdfdi2
	{CODE_FOR_fix_truncdfdi2,"__fixdfdi",	FIX,		UNOP, 1},
#endif
#ifdef HAVE_fix_truncsfdi2
	{CODE_FOR_fix_truncsfdi2,"__fixsfdi",	FIX,		UNOP, 1},
#endif
#ifdef HAVE_fix_truncdfsi2
	{CODE_FOR_fix_truncdfsi2,"__fixdfsi",	FIX,		UNOP, 1},
#endif
#ifdef HAVE_fix_truncsfsi2
	{CODE_FOR_fix_truncsfsi2,"__fixsfsi",	FIX,		UNOP, 1},
#endif

#ifdef HAVE_floatunssitf
	{CODE_FOR_floatunssitf,	"__floatunssitf",UNSIGNED_FLOAT,UNOP, 1},
#endif
#ifdef HAVE_floatunssidf
	{CODE_FOR_floatunssidf,	"__floatunssidf",UNSIGNED_FLOAT,UNOP, 1},
#endif
#ifdef HAVE_floatunssisf
	{CODE_FOR_floatunssisf,	"__floatunssisf",UNSIGNED_FLOAT,UNOP, 1},
#endif
#ifdef HAVE_floatditf2
	{CODE_FOR_floatditf2,	"__floatditf",	FLOAT,		UNOP, 1},
#endif
#ifdef HAVE_floatdidf2
	{CODE_FOR_floatdidf2,	"__floatdidf",	FLOAT,		UNOP, 1},
#endif
#ifdef HAVE_floatdisf2
	{CODE_FOR_floatdisf2,	"__floatdisf",	FLOAT,		UNOP, 1},
#endif
#ifdef HAVE_floatsitf2
	{CODE_FOR_floatsitf2,	"__floatsitf",	FLOAT,		UNOP, 1},
#endif
#ifdef HAVE_floatsidf2
	{CODE_FOR_floatsidf2,	"__floatsidf",	FLOAT,		UNOP, 1},
#endif
#ifdef HAVE_floatsisf2
	{CODE_FOR_floatsisf2,	"__floatsisf",	FLOAT,		UNOP, 1},
#endif

#ifdef HAVE_addtf3
	{CODE_FOR_addtf3,	"__addtf3",	PLUS,		BINOP, 1},
#endif
#ifdef HAVE_adddf3
	{CODE_FOR_adddf3,	"__adddf3",	PLUS,		BINOP, 1},
#endif
#ifdef HAVE_addsf3
	{CODE_FOR_addsf3,	"__addsf3",	PLUS,		BINOP, 1},
#endif

#ifdef HAVE_subtf3
	{CODE_FOR_subtf3,	"__subtf3",	MINUS,		BINOP, 1},
#endif
#ifdef HAVE_subdf3
	{CODE_FOR_subdf3,	"__subdf3",	MINUS,		BINOP, 1},
#endif
#ifdef HAVE_subsf3
	{CODE_FOR_subsf3,	"__subsf3",	MINUS,		BINOP, 1},
#endif

#ifdef HAVE_multf3
	{CODE_FOR_multf3,	"__multf3",	MULT,		BINOP, 1},
#endif
#ifdef HAVE_muldf3
	{CODE_FOR_muldf3,	"__muldf3",	MULT,		BINOP, 1},
#endif
#ifdef HAVE_mulsf3
	{CODE_FOR_mulsf3,	"__mulsf3",	MULT,		BINOP, 1},
#endif

#ifdef HAVE_divtf3
	{CODE_FOR_divtf3,	"__divtf3",	DIV,		BINOP, 1},
#endif
#ifdef HAVE_divdf3
	{CODE_FOR_divdf3,	"__divdf3",	DIV,		BINOP, 1},
#endif
#ifdef HAVE_divsf3
	{CODE_FOR_divsf3,	"__divsf3",	DIV,		BINOP, 1},
#endif

#ifdef HAVE_remsf3
	{CODE_FOR_remsf3	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_remdf3
	{CODE_FOR_remdf3,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_remtf3
	{CODE_FOR_remtf3,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_rmdsf3
	{CODE_FOR_rmdsf3,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_rmddf3
	{CODE_FOR_rmddf3,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_rmdtf3
	{CODE_FOR_rmdtf3,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_ceildf2
	{CODE_FOR_ceildf2,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_ceilsf2
	{CODE_FOR_ceilsf2,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_ceiltf2
	{CODE_FOR_ceiltf2,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_clsdfsi
	{CODE_FOR_clsdfsi,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_clssfsi
	{CODE_FOR_clssfsi,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_clstfsi
	{CODE_FOR_clstfsi,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_floordf2
	{CODE_FOR_floordf2,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_floorsf2
	{CODE_FOR_floorsf2,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_floortf2
	{CODE_FOR_floortf2,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_logbdf2
	{CODE_FOR_logbdf2,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_logbsf2
	{CODE_FOR_logbsf2,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_logbtf2
	{CODE_FOR_logbtf2,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_rintdf2
	{CODE_FOR_rintdf2,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_rintsf2
	{CODE_FOR_rintsf2,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_rinttf2
	{CODE_FOR_rinttf2,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_rounddf2
	{CODE_FOR_rounddf2,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_rounddfsi
	{CODE_FOR_rounddfsi,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_roundsf2
	{CODE_FOR_roundsf2,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_roundsfsi
	{CODE_FOR_roundsfsi,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_roundtf2
	{CODE_FOR_roundtf2,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_roundtfsi
	{CODE_FOR_roundtfsi,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_roundunsdfsi
	{CODE_FOR_roundunsdfsi,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_roundunssfsi
	{CODE_FOR_roundunssfsi,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_roundunstfsi
	{CODE_FOR_roundunstfsi,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_scaledfsidf
	{CODE_FOR_scaledfsidf,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_scalesfsisf
	{CODE_FOR_scalesfsisf,	"NOTUSED",	0,	0,	0},
#endif
#ifdef HAVE_scaletfsitf
	{CODE_FOR_scaletfsitf,	"NOTUSED",	0,	0,	0},
#endif
	{CODE_FOR_nothing,"",UNKNOWN,UNOP}	/* sentinal entry */
};

#if 0
i960_noalias_libcall_p(insn)
rtx insn;
{
	rtx pat;
	char *fnname;
	struct libcalls *p = libs;

	/* Get the function's name, as described by its RTL */

	pat = XEXP(insn,1);
	if (GET_CODE(pat) != CALL)
		return 0;
	pat = XEXP(pat, 0);
	if (GET_CODE(pat) != MEM)
		return 0;
	pat = XEXP(pat,0);
	if (GET_CODE(pat) != SYMBOL_REF)
		return 0;
	fnname = XSTR(pat, 0);

	while(p->icode != CODE_FOR_nothing) {
		if (strcmp(p->lib,fnname) == 0) {
			return p->noalias_p;
		}
		p++;
	}
	return 0;
}
#endif

i960_emit_libcall(rcode,icode,ops)
enum rtx_code rcode;
enum insn_code icode;
rtx ops[];
{
	rtx insn_first, insn_last;
	rtx notes, tmp;
        rtx seq;
	enum machine_mode mode = GET_MODE(ops[0]);
	struct libcalls *p = libs;

	while(p->icode != CODE_FOR_nothing) {
		if (p->icode == icode) break;
		p++;
	}
	if (p->icode == CODE_FOR_nothing) {
		fatal("bad library call");
		return;
	}

        start_sequence();
	if (p->type == BINOP) {
		/* Pass 1 for NO_QUEUE so we don't lose any increments
		   if the libcall is cse'd or moved.  */
		emit_library_call(gen_symref_rtx(Pmode, p->lib),
					1,mode,
					2,ops[1],GET_MODE(ops[1]),
					  ops[2],GET_MODE(ops[2]));
		emit_move_insn(ops[0], hard_libcall_value(mode));

		insn_first = get_insns();
		if (insn_first == 0) abort();
		insn_last = get_last_insn();

		notes = gen_rtx(EXPR_LIST, REG_EQUAL,
				gen_rtx (p->rcode, mode, ops[1], ops[2]),
				gen_rtx (INSN_LIST, REG_RETVAL, insn_first,
					REG_NOTES(insn_last)));
	} else if (p->type == CMPOP) {
                rtx treg = gen_reg_rtx(SImode);

		emit_library_call(gen_symref_rtx(Pmode, p->lib),
					0,SImode,
					2,ops[0],GET_MODE(ops[0]),
					  ops[1],GET_MODE(ops[1]));

                
                insn_last = emit_move_insn(treg, hard_libcall_value(SImode));
		emit_cmp_insn(treg,const0_rtx,0,0,SImode,0,0);

		insn_first = get_insns();
		if (insn_first == 0) abort();

		notes = gen_rtx(EXPR_LIST, REG_EQUAL,
				gen_rtx(p->rcode, CCmode, ops[0], ops[1]),
				gen_rtx (INSN_LIST, REG_RETVAL, insn_first,
					REG_NOTES(insn_last)));
	} else if (p->type == TSTOP) {
		rtx zero;
                rtx treg = gen_reg_rtx(SImode);

                zero = GET_CONST_ZERO_RTX (mode);
		emit_library_call(gen_symref_rtx(Pmode, p->lib),
					0,SImode,2,ops[0],mode,zero,mode);

                insn_last = emit_move_insn(treg, hard_libcall_value(SImode));
		emit_cmp_insn(treg,const0_rtx,0,0,SImode,0,0);

		insn_first = get_insns();
		if (insn_first == 0) abort();

		notes = gen_rtx(EXPR_LIST, REG_EQUAL,
				gen_rtx (p->rcode, CCmode, ops[0], zero),
				gen_rtx (INSN_LIST, REG_RETVAL, insn_first,
					REG_NOTES(insn_last)));
	} else if (p->type == UNOP) {
		emit_library_call(gen_symref_rtx(Pmode, p->lib),
				0, mode, 1, ops[1], GET_MODE(ops[1]));
		emit_move_insn(ops[0], hard_libcall_value(GET_MODE(ops[0])));

		insn_first = get_insns();
		if (insn_first == 0) abort();
		insn_last = get_last_insn();

		notes = gen_rtx (EXPR_LIST, REG_EQUAL,
				gen_rtx(EXPR_LIST, VOIDmode,
					gen_symref_rtx(Pmode, p->lib),
					gen_rtx (EXPR_LIST,VOIDmode,ops[1],0)),
				gen_rtx (INSN_LIST, REG_RETVAL, insn_first,
                                	REG_NOTES (insn_last)));

	} else {
		abort();
	}

	REG_NOTES(insn_last) = notes;
	REG_NOTES(insn_first) = gen_rtx(INSN_LIST, REG_LIBCALL, insn_last,
					REG_NOTES(insn_first));
        seq = gen_sequence();
        end_sequence();
        emit_insn(seq);
}

i960_is_leaf_proc(decl)
tree decl;
{
  char * name = "";
  if (DECL_ASSEMBLER_NAME(decl) != 0 &&
      IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(decl)) != 0)
    name = IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(decl));

  i960_function_name_declare((FILE *)0, name, decl);
  return (i960_leaf_ret_reg >= 0);
}

static enum machine_mode
pick_move_mode (ali, regno, off, size)
int ali;
int regno;
int off;
int size;
{
  enum machine_mode m = VOIDmode;
  register int i = ali | off;

  assert (ali > 0 && size >0 && off >= 0);
  assert ((regno + ((off+size)%UNITS_PER_WORD))  <= FIRST_PSEUDO_REGISTER);

  /* align is alignment of base word.  Make it be alignment
     in bytes of word base+off. */

  ali = 1;
  while ((ali & i)==0 && ali< (BIGGEST_ALIGNMENT / BITS_PER_UNIT))
    ali <<= 1;

  regno += off / UNITS_PER_WORD;

  if (!( (HAVE_movti && ali >= GET_MODE_SIZE(m=TImode)) &&
         (size >= GET_MODE_SIZE(m)) && (HARD_REGNO_MODE_OK (regno, m)) ))

  if (!( (HAVE_movdi && ali >= GET_MODE_SIZE(m=DImode)) &&
         (size >= GET_MODE_SIZE(m)) && (HARD_REGNO_MODE_OK (regno, m)) ))

  if (!( (HAVE_movsi && ali >= GET_MODE_SIZE(m=SImode)) &&
         (size >= GET_MODE_SIZE(m)) && (HARD_REGNO_MODE_OK (regno, m)) ))

  if (!( (HAVE_movhi && ali >= GET_MODE_SIZE(m=HImode)) &&
         (size >= GET_MODE_SIZE(m)) && (HARD_REGNO_MODE_OK (regno, m)) ))

  if (!( (HAVE_movqi && ali >= GET_MODE_SIZE(m=QImode)) &&
         (size >= GET_MODE_SIZE(m)) && (HARD_REGNO_MODE_OK (regno, m)) ))

    assert (0);

  return m;
}

move_to_reg (regno, x, size, align)
int regno;
rtx x;
int size;
int align;
{
  int i = 0;

  assert (regno < FIRST_PSEUDO_REGISTER);

#ifdef RTX_ALIGNMENT
  /* RTX_ALIGNMENT will decide if relaxed align is ok */
  align = min (RTX_ALIGNMENT (gen_rtx (REG, SImode, regno), size, align),
               RTX_ALIGNMENT (x, size, align));
#endif

  if (align <= UNITS_PER_WORD)
    size = ROUND (size, align);
  else
    size = ROUND (size, UNITS_PER_WORD);

  /* New code added to handle the requirements of
     -mstrict-align.  If we don't have at least word
     alignment, we make a memory temp, move the source in
     memory to the temp, and load from the temp using a mondo load. */

  if (align < GET_MODE_SIZE(SImode) && align < size)
  {
    int old_align = align;
    int old_size  = size;
    rtx old_x     = x;

    /* Strict align handling for processors which have regs
       which are not at least word aligned is not implemented, so
       assert that such is not the case. */

    assert (GET_CODE(x) == MEM);

    align = i960_object_bytes_bitalign (size)/BITS_PER_UNIT;

    if (align <= UNITS_PER_WORD)
      size = ROUND (size, align);
    else
      size = ROUND (size, UNITS_PER_WORD);

    x = assign_stack_local (BLKmode, size, align * BITS_PER_UNIT);

    emit_block_move (x, old_x, gen_rtx(CONST_INT,VOIDmode,old_size),old_align);
  }

  /* emit_move insn can handle const_double */
  if (GET_CODE (x) == CONST_DOUBLE)
  { enum machine_mode m;

    if (size == GET_MODE_SIZE(SFmode))
      m = SFmode;
    else if (size == GET_MODE_SIZE(DFmode))
      m = DFmode;
    else if (size == GET_MODE_SIZE(TFmode))
      m = TFmode;
    else
      assert (0);

    emit_move_insn (gen_rtx (REG, m, regno), x);
  }
  else
    while (size)
    { enum machine_mode m;
      rtx src, dst;

      m = pick_move_mode (align, regno, i, size);

      if (GET_CODE(x) == MEM)
        src = gen_typed_mem_rtx (get_rtx_type(x),m, memory_address (m, plus_constant (XEXP(x,0), i)));
      else
        if (m!=GET_MODE(x) || i!=0)
          src = gen_rtx (SUBREG,m,x,i/UNITS_PER_WORD);
        else
          src = x;

      dst = gen_rtx (REG,m,regno+i/UNITS_PER_WORD);

      emit_move_insn (dst, src);

      i += GET_MODE_SIZE(m);
      size -= GET_MODE_SIZE(m);
    }
}

move_from_reg (regno, x, size, align)
int regno;
rtx x;
int size;
int align;
{
  int i = 0;
  int big_end_adjust = 0;

#ifdef RTX_ALIGNMENT
  align = min (RTX_ALIGNMENT (gen_rtx (REG, SImode, regno), size, align),
               RTX_ALIGNMENT (x, size, align));
#endif

  if (GET_CODE(x) != MEM)
  { size  = ROUND (size,  UNITS_PER_WORD);
    align = ROUND (align, UNITS_PER_WORD);
  }

  if (size < UNITS_PER_WORD && bytes_big_endian)
    big_end_adjust = (UNITS_PER_WORD - size) * BITS_PER_UNIT;

  while (size > 0)
  {
    enum machine_mode m;
    rtx dst, src;

    m = pick_move_mode (align, regno, i, size);

    if (GET_CODE(x) == MEM)
      dst = gen_typed_mem_rtx (get_rtx_type(x), m,
              memory_address (m, plus_constant (XEXP(x,0), i)));
    else
      if (m!=GET_MODE(x) || i!=0)
        dst = gen_rtx (SUBREG,m,x,i/UNITS_PER_WORD);
      else
        dst = x;


    if (GET_MODE_SIZE(m) < UNITS_PER_WORD)
    {
      int bitsize = GET_MODE_SIZE(m) * BITS_PER_UNIT;
      int bitpos  = (i % UNITS_PER_WORD) * BITS_PER_UNIT;
      int xbitpos = bitpos + big_end_adjust;

      dst = gen_typed_mem_rtx (get_rtx_type(x), word_mode,
              memory_address (word_mode,
                plus_constant (XEXP(x,0),
                               ((i / UNITS_PER_WORD) * UNITS_PER_WORD))));
      MEM_IN_STRUCT_P(dst) = 1;
      src = gen_rtx (REG, word_mode, regno+(i/UNITS_PER_WORD));
      store_bit_field (dst, bitsize, bitpos, word_mode,
                       extract_bit_field (src,
                                          bitsize,
                                          xbitpos,
                                          1,
                                          NULL_RTX,
                                          word_mode,
                                          word_mode,
                                          bitsize / BITS_PER_UNIT,
                                          BITS_PER_WORD),
                       bitsize / BITS_PER_UNIT, BITS_PER_WORD);
    }
    else
    {
      dst = gen_typed_mem_rtx (get_rtx_type(x), m,
              memory_address (m, plus_constant (XEXP(x,0), i)));
      src = gen_rtx (REG, m, regno+i/UNITS_PER_WORD);
      emit_move_insn (dst, src);
    }

    i += GET_MODE_SIZE(m);
    size -= GET_MODE_SIZE(m);
  }
}

/* Return the most stringent alignment that we are willing
   to consider objects of size 'size and known alignment 'align'
   as having. */

i960_mem_alignment (x, size, align)
rtx x;
int size;
int align;
{
  int i;

  assert (align>0 && align<=(BIGGEST_ALIGNMENT/BITS_PER_UNIT));
  assert ((align & (align-1)) == 0);
  assert (x == 0 || GET_CODE(x) == MEM);

  if (x != 0)
  { enum machine_mode m = GET_MODE(x);

    if (m != BLKmode && m != VOIDmode)
      align = MAX (align, GET_MODE_SIZE (m));

    align = max (align, i960_expr_alignment (XEXP(x,0), size));
  }

  /*  After discovering the best known alignment for x, if strict-align
      is not enabled, we may improve upon the known alignment by guessing
      that x will actually be better aligned at runtime, or that if it is
      poorly aligned, the cost will be low enough that it is still better
      to avoid the extra code we would get from an aligned dereference. */

  if (!(TARGET_STRICT_ALIGN))
    if ((TARGET_IC_COMPAT2_0) || align >= 4)
      if ((i = i960_object_bytes_bitalign(size)/BITS_PER_UNIT) > align)
        align = i;

  return align;
}

i960_reg_alignment (x, size, align)
rtx x;
int size;
int align;
{
  /*  If x is non-zero, and if we can tell that x is better aligned than
      indicated by 'align' by examining its rtx, we can use that
      information regardless of the setting of TARGET_STRICT_ALIGN.  */

  assert (align>0 && align<=(BIGGEST_ALIGNMENT/BITS_PER_UNIT));
  assert ((align & (align-1)) == 0);
  assert (x == 0 || GET_CODE(x) == REG);

  /* ANY register on the 960 is ok for at least word alignment. */
  align = MAX (align, UNITS_PER_WORD);

  if (x != 0)
  { enum machine_mode m = GET_MODE(x);
    int r = REGNO (x);

    if (m != BLKmode && m != VOIDmode)
      align = MAX (align, GET_MODE_SIZE (m));

    if (r < FIRST_PSEUDO_REGISTER)
      if (FP_REG_P(x) || (r & 0x3) == 0)
        align = 16;
      else
        if ((r & 0x1) == 0)
          align = MAX (align, 8);
  }
  return align;
}

unsigned int hard_regno_mode_ok[FIRST_PSEUDO_REGISTER] = REGNO_MODE_OK_INIT;

i960_rtx_alignment (x, size, align)
rtx x;
int size;
int align;
{
  /*  If we can tell that x is better aligned than
      indicated by 'align' by examining its rtx, we can use that
      information regardless of the setting of TARGET_STRICT_ALIGN.

      If x is memory and TARGET_STRICT_ALIGN is disabled, we can
      guess at x's true alignment at runtime based on its size.  */

  enum machine_mode m;
  enum rtx_code c;
  
  assert (align>0 && align<=(BIGGEST_ALIGNMENT/BITS_PER_UNIT));
  assert ((align & (align-1)) == 0);
  assert (x != 0);

  m = GET_MODE(x);
  c = GET_CODE(x);

  if (m != BLKmode && m != VOIDmode)
    align = MAX (align, GET_MODE_SIZE (m));

  if (c == REG)
    align = i960_reg_alignment (x, size, align);

  else if (c == MEM)
    align = i960_mem_alignment (x, size, align);

  else if (c == SUBREG)
  {
    rtx y = XEXP(x,0);
    int i = GET_MODE_SIZE (GET_MODE(y));

    i = i960_rtx_alignment (y, i, i);
    i = MIN (i, 4 * SUBREG_WORD(x));
    align = MAX (align, i);
  }

  return align;
}

/* Handle #pragma align and #pragma pack.  These pragmas
   have nothing whatsover to do with IC960 pragmas.  */

pragma_align(align)
int align;
{
  switch (align)
  {
    case 0:                             /* return to i960_last alignment */
      align = i960_last_maxbitalignment/8;

    case 16:                            /* Byte alignments */
    case 8:
    case 4:
    case 2:
    case 1:
      i960_last_maxbitalignment = i960_maxbitalignment;
      i960_maxbitalignment      = align*8;
      break;

    default:
      warning ("argument to #pragma align must be one of 0,1,2,4,8, or 16.");
      break;
  }
}

pragma_pack(align)
int align;
{
  switch (align)
  {
    case 0:                             /* return to i960_last alignment */
      align = i960_last_max_member_bit_align/8;

    case 16:                            /* Byte alignments */
    case 8:
    case 4:
    case 2:
    case 1:
      i960_last_max_member_bit_align = i960_max_member_bit_align;
      i960_max_member_bit_align      = align*8;
      break;

    default:
      warning ("argument to #pragma pack must be one of 0,1,2,4,8, or 16.");
      break;
  }
}

static char *
eat_white_space(cp)
char *cp;
{
  while (*cp == ' ' || *cp == '\t')
    cp ++;

  return cp;
}

static char *
get_num(cp, np, flag_p)
char *cp;
int *np;
int *flag_p;
{
  int radix;
  int ok = 1;
  int legal_digit;
  int sum = 0;
  int n_legal_digits = 0;

  if (*cp == '0')
  {
    if (*(cp+1) == 'x' || *(cp+1) == 'X')
    {
      radix = 16;
      cp = cp+2;
    }
    else
      radix = 8;
  }
  else
    radix = 10;

  do
  {
    switch (*cp)
    {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
        legal_digit = 1;
        sum = sum * radix + (*cp - '0');
        cp++;
        break;

      case '8':
      case '9':
        if (radix == 8)
        {
          legal_digit = 0;
          ok = 0;
          break;
        }
        legal_digit = 1;
        sum = sum * radix + (*cp - '0');
        cp++;
        break;

      case 'A':
      case 'B':
      case 'C':
      case 'D':
      case 'E':
      case 'F':
        if (radix != 16)
        {
          legal_digit = 0;
          break;
        }
        legal_digit = 1;
        sum = sum * radix + 10 + (*cp - 'A');
        cp++;
        break;

      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f':
        if (radix != 16)
        {
          legal_digit = 0;
          break;
        }
        legal_digit = 1;
        sum = sum * radix + 10 + (*cp - 'a');
        cp++;
        break;

      default:
        legal_digit = 0;
        break;
    }

    if (legal_digit)
      n_legal_digits += 1;
  } while (ok && legal_digit);

  *np = sum;
  *flag_p = ok && n_legal_digits != 0;

  return cp;
}

static char *
get_id(cp, id_p, flag_p)
char *cp;
char **id_p;
int *flag_p;
{
  *id_p = cp;

  if (*cp == '_' || isalpha(*cp))
  {
    *flag_p = 1;
    cp ++;
    while (*cp == '_' || isalnum(*cp)) cp ++;
  }
  else
    *flag_p = 0;

  return cp;
}

static char *
get_string(cp, str_p, flag_p)
char *cp;
char **str_p;
int *flag_p;
{
  *str_p = cp;

  if (*cp != '"')
  {
    *flag_p = 0;
    return cp;
  }

  cp++;
  while (*cp != '"' && *cp != '\n')
  {
    if (*cp == '\\' && *(cp+1) == '"')
      cp++;
    cp++;
  }

  if (*cp == '"')
  {
    *flag_p = 1;
    return cp+1;
  }
  else
  {
    *flag_p = 0;
    return cp;
  }
}

static int
i960_function_pragma(bp, prag_id, prag_val, prag_name, proc_f)
char *bp;
int prag_id;
int prag_val;
char *prag_name;
int (*proc_f)();
{
  char *start;
  char *id_start;
  char *id_end;
  char old;
  int success;
  int n;
  char *str;
  char *str_end;

  bp = eat_white_space(bp);
  start = bp;
  if (*bp == '(')
  {
    *bp = ' ';
    bp = eat_white_space(bp+1);
  }

  if (prag_id == PRAG_ID_OPTIMIZE && *bp == '"')
  {
    bp = get_string(bp, &str, &success);
    str_end = bp-1;

    if (!success)
    {
      warning ("invalid syntax for #pragma %s - pragma ignored.", prag_name);
      return 0;
    }
    bp = eat_white_space(bp);

    if (*bp == ',' || *bp == ')')
      *bp = ' ';
    bp = eat_white_space(bp);

    if (*bp != '\n')
    {
      warning ("invalid syntax for #pragma %s - pragma ignored.", prag_name);
      return 0;
    }

    *str_end = '\0';
    proc_f((char *)0, str+1, prag_name);
    *str_end = '"';
    return 1;
  }

  while (*bp != '\n')
  {
    bp = get_id(bp, &id_start, &success);
    if (!success)
    {
      warning ("invalid syntax for #pragma %s - pragma ignored.", prag_name);
      return 0;
    }

    bp = eat_white_space(bp);

    if (prag_id == PRAG_ID_SYSTEM && prag_val != NOT_SYSCALL && *bp == '=')
    {
      bp = eat_white_space(bp + 1);
      bp = get_num(bp, &n, &success);

      if (!success)
      {
        warning ("invalid syntax for #pragma %s - pragma ignored.", prag_name);
        return 0;
      }

      if (n < 0 || n > 259)
      {
        warning ("invalid value in #pragma %s - pragma ignored.", prag_name);
        return 0;
      }

      if (flag_bout && n > 253)
        warning("value in #pragma %s too large for object format.", prag_name);
    }

    if (prag_id == PRAG_ID_OPTIMIZE)
    {
      if (*bp != '=')
      {
        warning ("invalid syntax for #pragma %s - pragma ignored.", prag_name);
        return 0;
      }
      *bp = ' ';
      bp = eat_white_space(bp + 1);
      bp = get_string(bp, &str, &success);
      if (!success)
      {
        warning ("invalid syntax for #pragma %s - pragma ignored.", prag_name);
        return 0;
      }
    }

    bp = eat_white_space(bp);
    if (*bp == ',' || *bp == ')')
      *bp = ' ';

    bp = eat_white_space(bp);
  }


  /*
   * now that we have checked the syntax, 
   * actually record the info.
   */
  bp = eat_white_space(start);

  if (*bp == '\n' && prag_id != PRAG_ID_OPTIMIZE)
  {
    proc_f((char *)0, prag_val, prag_name);
    return 1;
  }

  while (*bp != '\n')
  {
    bp = get_id(bp, &id_start, &success);
    id_end = bp;
    bp = eat_white_space(bp);

    if (prag_id == PRAG_ID_SYSTEM && prag_val != NOT_SYSCALL)
    {
      if (*bp == '=')
      {
        bp = eat_white_space(bp + 1);
        bp = get_num(bp, &n, &success);
      }
      else
        n = NO_SYSCALL_INDEX;
      prag_val = n;
    }

    old = *id_end;
    *id_end = '\0';

    if (prag_id == PRAG_ID_OPTIMIZE)
    {
      bp = eat_white_space(bp);
      bp = get_string(bp, &str, &success);
      str_end  = bp - 1;
      *str_end = '\0';
      proc_f(id_start, str+1, prag_name);
      *str_end = '"';
    }
    else
      proc_f(id_start, prag_val, prag_name);

    *id_end = old;

    bp = eat_white_space(bp);
  }

  return 1;
}

static int
i960_align_pragma(bp, prag_name, value_allowed, align_default)
char *bp;
char *prag_name;
int value_allowed;
int align_default;
{
  char *start;
  char *id_start;
  char *id_end;
  char old;
  int n;
  int success;

  start = bp;
  bp = eat_white_space(bp);

  if (*bp == '\n')
  {
    process_pragma_i960_align((char *)0, align_default, prag_name);
    return 1;
  }

  if (*bp == '(')
  {
    *bp = ' ';
    bp = eat_white_space(bp+1);
  }

  if (value_allowed)
  {
    bp = get_num(bp, &n, &success);
    if (success)
    {
      if (n != 1 && n != 2 && n != 4 && n != 8 && n != 16)
      {
        warning ("value for #pragma %s must be 1,2,4,8, or 16.", prag_name);
        return 0;
      }

      bp = eat_white_space(bp);

      if (*bp == ')')
      { 
        *bp = ' ';
        bp = eat_white_space(bp+1);
      }

      if (*bp != '\n')
      {
        warning ("invalid syntax for #pragma %s - pragma ignored.", prag_name);
        return 0;
      }

      process_pragma_i960_align((char *)0, n, prag_name); 
      return 1;
    }
  }

  /* must be identifier or error */
  while (*bp != '\n' && *bp != ')')
  {
    bp = get_id(bp, &id_start, &success);
    if (!success)
    {
      warning ("invalid syntax for #pragma %s - pragma ignored.", prag_name);
      return 0;
    }

    bp = eat_white_space(bp);

    if (value_allowed && *bp == '=')
    {
      bp = eat_white_space(bp + 1);
      bp = get_num(bp, &n, &success);

      if (!success)
      {
        warning ("invalid syntax for #pragma %s - pragma ignored.", prag_name);
        return 0;
      }

      if (n != 1 && n != 2 && n != 4 && n != 8 && n != 16)
      {
        warning ("value for #pragma %s must be 1,2,4,8, or 16.", prag_name);
        return 0;
      }
    }

    if (*bp == ',')
      *bp = ' ';

    bp = eat_white_space(bp);
  }

  if (*bp == ')')
  { 
    *bp = ' ';
    bp = eat_white_space(bp+1);
  }
  
  if (*bp != '\n')
  {
    warning ("invalid syntax for #pragma %s - pragma ignored.", prag_name);
    return 0;
  }


  /*
   * OK, we checked the syntax, now lets process it.
   */
  bp = eat_white_space(start);

  while (*bp != '\n' && *bp != ')')
  {
    bp = get_id(bp, &id_start, &success);
    id_end = bp;

    bp = eat_white_space(bp);

    if (value_allowed && *bp == '=')
    {
      bp = eat_white_space(bp + 1);
      bp = get_num(bp, &n, &success);
    }
    else
      n = align_default;

    old = *id_end;
    *id_end = '\0';
    process_pragma_i960_align(id_start, n, prag_name);
    *id_end = old;

    bp = eat_white_space(bp);
  }
}

static int
i960_section_pragma(bp, prag_name)
char *bp;
char *prag_name;
{
  char *str = 0;
  char *str_end = 0;
  int success;

  if (flag_bout)
  {
    warning ("#pragma %s not supported for B.OUT object - pragma ignored",
             prag_name);
    return 0;
  }

  bp = eat_white_space(bp);

  if (*bp == '"')
  {
    char *strp;
    bp = get_string(bp, &str, &success);

    if (!success)
    {
      warning ("invalid syntax for #pragma %s - pragma ignored.", prag_name);
      return 0;
    }
    str += 1;
    str_end  = bp-1;

    if (flag_coff && ((str_end - str) > 3))
    {
      warning ("#pragma %s must be 3 or less chars for COFF - pragma ignored",
               prag_name);
      return 0;
    }

    for (strp=str; strp < str_end; strp++)
    {
      if (!isalnum(*strp))
      {
        warning ("string for #pragma %s must be alphanumeric- pragma ignored",
                 prag_name);
        return 0;
      }
    }

    *str_end = '\0';
    bp = eat_white_space(bp);
  }

  if (*bp != '\n')
  {
    warning ("invalid syntax for #pragma %s - pragma ignored.", prag_name);
    return 0;
  }

  if (i960_section_string != 0)
  {
    free (i960_section_string);
    i960_section_string = 0;
  }

  if (str != 0 && *str != '\0')
  {
    i960_section_string = xmalloc(strlen(str)+1);
    strcpy(i960_section_string, str);
#if defined(IMSTG_PRAGMA_SECTION_SEEN)
    IMSTG_PRAGMA_SECTION_SEEN = 1;
#endif
  }

  /*
   * since this changes the section name, we want to essentially
   * clear any notion the compiler has about what section it is in.
   */
  set_no_section();
  return 1;
}

static int
non_ident_p(c)
int c;
{
  if (isalnum(c) || c == '_')
    return 0;
  return 1;
}

void
i960_process_pragma(get_func, unget_func, f)
int (*get_func)();
int (*unget_func)();
FILE *f;
{
  int n,c;
  int success;
  char buf[1024];
  char *bp = buf;
  char *prag_name;

  /* fgets(buf, 1024, f); */
  n = 0;
  do
  { buf[n++] = (c=get_func(f));
  }
  while (n < 1023 && c!='\n' && c != EOF);
  buf[n] = '\0';

  if (strchr(bp, '\n') == 0)
  {
    warning ("#pragma line too long - 1023 char max.");
    return;
  }

  bp = eat_white_space(bp);
  if (*bp == '\n')
    return;

  if (strncmp(bp, "pack", 4) == 0 && non_ident_p(bp[4]))
  {
    bp += 4;
    bp = eat_white_space(bp);
    bp = get_num(bp, &n, &success);
    if (!success)
    {
      warning ("invalid number in #pragma pack.");
      goto fail;
    }
    bp = eat_white_space(bp);
    if (*bp != '\n')
    {
      warning ("invalid syntax for #pragma %s - pragma ignored.", "pack");
      goto fail;
    }
    pragma_pack (n);
  }
  else if (strncmp(bp, "align", 5) == 0 && non_ident_p(bp[5]) && !flag_ic960)
  {
    bp += 5;
    bp = eat_white_space(bp);
    bp = get_num(bp, &n, &success);
    if (!success)
    {
      warning ("invalid number in #pragma align.");
      goto fail;
    }
    bp = eat_white_space(bp);
    if (*bp != '\n')
    {
      warning ("invalid syntax for #pragma %s - pragma ignored.", "align");
      goto fail;
    }
    pragma_align (n);
  }
  else if (strncmp(bp, "align", 5) == 0 && non_ident_p(bp[5]) && flag_ic960)
  {
    if (!i960_align_pragma(bp+5, "align", 1, 0))
      goto fail;
  }
  else if (strncmp(bp, "i960_align", 10) == 0 && non_ident_p(bp[10]))
  {
    if (!i960_align_pragma(bp+10, "i960_align", 1, 0))
      goto fail;
  }
  else if (strncmp(bp, "noalign", 7) == 0 && non_ident_p(bp[7]))
  {
    if (!i960_align_pragma(bp+7, "noalign", 0, 1))
      goto fail;
  }
  else if (strncmp(bp, "inline", 6) == 0 && non_ident_p(bp[6]))
  {
    if (!i960_function_pragma(bp+6, PRAG_ID_INLINE, PRAGMA_DO,
                              "inline", process_pragma_inline))
      goto fail;
  }
  else if (strncmp(bp, "noinline", 8) == 0 && non_ident_p(bp[8]))
  {
    if (!i960_function_pragma(bp+8, PRAG_ID_INLINE, PRAGMA_NO,
                              "noinline", process_pragma_inline))
      goto fail;
  }
  else if (strncmp(bp, "isr", 3) == 0 && non_ident_p(bp[3]))
  {
    if (!i960_function_pragma(bp+3, PRAG_ID_ISR, PRAGMA_DO,
                              "isr", process_pragma_isr))
      goto fail;
  }
  else if (strncmp(bp, "system", 6) == 0 && non_ident_p(bp[6]))
  {
    if (!i960_function_pragma(bp+6, PRAG_ID_SYSTEM, NO_SYSCALL_INDEX,
                              "system", process_pragma_system))
      goto fail;
  }
  else if (strncmp(bp, "nosystem", 8) == 0 && non_ident_p(bp[8]))
  {
    if (!i960_function_pragma(bp+8, PRAG_ID_SYSTEM, NOT_SYSCALL,
                              "nosystem", process_pragma_system))
      goto fail;
  }
  else if (strncmp(bp, "pure", 4) == 0 && non_ident_p(bp[4]))
  {
    if (!i960_function_pragma(bp+4, PRAG_ID_PURE, PRAGMA_DO,
                              "pure", process_pragma_pure))
      goto fail;
  }
  else if (strncmp(bp, "nopure", 6) == 0 && non_ident_p(bp[6]))
  {
    if (!i960_function_pragma(bp+6, PRAG_ID_PURE, PRAGMA_NO,
                              "nopure", process_pragma_pure))
      goto fail;
  }
  else if (strncmp(bp, "compress", 8) == 0 && non_ident_p(bp[8]))
  {
    if (!i960_function_pragma(bp+8, PRAG_ID_COMPRESS, PRAGMA_DO,
                              "compress", process_pragma_compress))
      goto fail;
  }
  else if (strncmp(bp, "cave", 4) == 0 && non_ident_p(bp[4]))
  {
    if (flag_bout)
    {
      warning ("#pragma cave not supported for B.OUT object - pragma ignored");
      goto fail;
    }
    if (!i960_function_pragma(bp+4, PRAG_ID_CAVE, PRAGMA_DO,
                              "cave", process_pragma_cave))
      goto fail;
  }
  else if (strncmp(bp, "nocompress", 10) == 0 && non_ident_p(bp[10]))
  {
    if (!i960_function_pragma(bp+10, PRAG_ID_COMPRESS, PRAGMA_NO,
                              "nocompress", process_pragma_compress))
      goto fail;
  }
  else if (strncmp(bp, "interrupt", 9) == 0 && non_ident_p(bp[9]))
  {
    if (!i960_function_pragma(bp+9, PRAG_ID_INTERRUPT, PRAGMA_DO,
                              "interrupt", process_pragma_interrupt))
      goto fail;
  }
  else if (strncmp(bp, "nointerrupt", 11) == 0 && non_ident_p(bp[11]))
  {
    if (!i960_function_pragma(bp+11, PRAG_ID_INTERRUPT, PRAGMA_NO,
                              "nointerrupt", process_pragma_interrupt))
      goto fail;
  }
  else if (strncmp(bp, "optimize", 8) == 0 && non_ident_p(bp[8]))
  {
    if (!i960_function_pragma(bp+8, PRAG_ID_OPTIMIZE, PRAGMA_DO,
                              "optimize", process_pragma_optimize))
      goto fail;
  }
  else if (strncmp(bp, "section", 7) == 0 && non_ident_p(bp[8]))
  {
    if (!i960_section_pragma(bp+7, "section"))
      goto fail;
  }

  fail: ;
  unget_func('\n', f);
}

i960_set_pragma_pack(t)
tree t;
{
  /*
   * If the align info for this aggregate is using pragma align then
   * iuse that alignment that was specified, otherwise use the alignment
   * specified by pragma pack.  Record this alignment in the member;
   * after the field's type is known, this will be used to
   * restrict DECL_ALIGN if this alignment is more restrictive.
   */

  int align = i960_max_member_bit_align;

  if (i960_struct_align_info[i960_align_top].used_i960_align)
    align = i960_struct_align_info[i960_align_top].align_value_used;

  TREE_SET_ALIGN_LIMIT(t, align);
}

i960_set_pragma_align(t)
tree t;
{
  /*
   * Determine the alignment to be used for this structure or union.
   * Here are the rules:
   *
   *   1.  If this is a struct and the struct tag name matches a name
   *       specified in a #pragma i960_align, then use the alignment
   *       specified there.
   *   2.  If this is a struct with an unnamed tag and it is within
   *       a struct whose alignment was determined by #pragma i960_align,
   *       then use the alignment specified for that enclosing struct.
   *   3.  If this is a struct and there is a pragma i960_align in effect,
   *       then use that alignment.
   *   4.  If this is a union or none of the above rules apply, then
   *       use the current setting of i960_maxbitalignment.
   */
  int use_i960_align = 0;
  int align_value = i960_maxbitalignment;

  if (TREE_CODE(t) == RECORD_TYPE || TREE_CODE(t) == UNION_TYPE)
  {
    /*
     * First check for a named tag that matches one on a pragma i960_align.
     */
    tree tname;

    if (t != 0 &&
        (tname = TYPE_NAME(t)) != 0)
    {
      char *name = 0;
      struct sym_pragma_info * prag_sym;

      if (TREE_CODE(tname) == IDENTIFIER_NODE)
        name = IDENTIFIER_POINTER(TYPE_NAME(t));
      else if (TREE_CODE(tname) == TYPE_DECL && DECL_NAME(tname) != 0)
        name = IDENTIFIER_POINTER(DECL_NAME(tname));

      if (name != 0 &&
          (prag_sym = look_sym_pragma(name, 0)) != 0 &&
          prag_sym->i960_align_set)
      {
        use_i960_align = 1;
        align_value = prag_sym->i960_align;
      }
    }
        
    if (!use_i960_align &&
        t != 0 &&
        TYPE_NAME(t) == 0 &&
        i960_struct_align_info[i960_align_top].used_i960_align)
    {
      use_i960_align = 1;
      align_value = i960_struct_align_info[i960_align_top].align_value_used;
    }

    if (!use_i960_align &&
        i960_struct_align_info[0].used_i960_align)
    {
      use_i960_align = 1;
      align_value = i960_struct_align_info[0].align_value_used;
    }
  }

  i960_align_top += 1;
  if (i960_align_top >= i960_n_struct_align_info)
  {
    i960_n_struct_align_info += 100;
    i960_struct_align_info =
        (struct i960_align_info *)xrealloc(i960_struct_align_info,
                                           i960_n_struct_align_info *
                                           sizeof(struct i960_align_info));
  }
  i960_struct_align_info[i960_align_top].used_i960_align = use_i960_align;
  i960_struct_align_info[i960_align_top].align_value_used = align_value;
 
  /*
   * This handles the special case where I specified an i960_align pragma
   * with natural alignment.  Only now do we treat it as natural alignment.
   */
  if (align_value == 0)
  {
    align_value = 128;
    use_i960_align = 0;
  }

  /*
   * Record the alignment in the aggregate.
   * This will be used to limit the amount of optional padding
   * done in i960_round_type_align.
   */
  TREE_SET_ALIGN_LIMIT(t, align_value);
  if (use_i960_align)
    TYPE_IMSTG_I960ALIGN(t) = 1;
}

i960_done_pragma_align(t)
tree t;
{
  i960_align_top -= 1;
}

enum reg_class i960_reg_class_from_c(c)
char c;
{
  switch (c)
  {
    case 'l': return LOCAL_REGS;
    case 'b': return GLOBAL_REGS;
    case 'd': return LOCAL_OR_GLOBAL_REGS;
    case 'q': return LOCAL_OR_GLOBAL_QREGS;
    case 't': return LOCAL_OR_GLOBAL_DREGS;

    case 'r':
      return GENERAL_REGS;
  
    case 'f':
      if (TARGET_NUMERICS)
        return FP_REGS;
      else
        return NO_REGS;

#ifdef IMSTG_COPREGS
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
      if ((c)-'u' < COP_NUM_CLASSES)
        return COP0_REGS + ((c)-'u');
      else
        return NO_REGS;
#endif

    default:
      return NO_REGS;
  }
}

enum reg_class i960_reg_class(r)
register int r;
{
  /* Return the first class which r could belong to. */

  int class;

  if (r < 0)
    return NO_REGS;

  assert (r < FIRST_PSEUDO_REGISTER);

  if (r >= 32 && r < 36)
  { assert ((TARGET_NUMERICS));
    return FP_REGS;
  }

  for (class = 0; class < N_REG_CLASSES; class++)
    if (TEST_HARD_REG_BIT (reg_class_contents[class], r))
      break;

  assert (class <= (int)ALL_REGS);

  return (enum reg_class)class;
}

enum reg_class i960_pref_reload_class (x, class)
rtx x;
enum reg_class class;
{
  /* Restrict 'class' based on mode/size as needed. */

  if (!IS_COP_CLASS(class))
    switch ((int) class)
    {
      default:
        /* Implementor, decide what to do with your new class. */
        assert (0);
        break;

      case NO_REGS:
      case FP_REGS:
      case LIM_REG_CLASSES:
      case GLOBAL_REGS:
      case LOCAL_REGS:
        break;
  
      case ALL_REGS:
      case LOCAL_OR_GLOBAL_QREGS:
      case LOCAL_OR_GLOBAL_DREGS:
      case LOCAL_OR_GLOBAL_REGS:
      case FP_OR_QREGS:
      case FP_OR_DREGS:
        if (x != 0 && RTX_BYTES(x) == 16)
          class = LOCAL_OR_GLOBAL_QREGS;
        else if (x != 0 && RTX_BYTES(x) == 8)
          class = LOCAL_OR_GLOBAL_DREGS;
        else
          class = LOCAL_OR_GLOBAL_REGS;
        break;
    }

  return class;
}

static
void
i960_set_spaceopt_options()
{
  /* turn on Compare and branch generation. */
  target_flags &= ~TARGET_FLAG_CMPBR_MASK;
  target_flags |= TARGET_FLAG_CMPBRANCH;

  /* allow all legal addressing modes */
  target_flags &= ~TARGET_FLAG_ADDR_MASK;
  target_flags |= TARGET_FLAG_ALL_ADDR;

  /* disallow loop unrolling */
  flag_unroll_loops = 0;
  flag_unroll_small_loops = 0;

  /*
   * disallow automatic function inlining, but not explicit function
   * inlining.
   */
  flag_inline_functions = 0;

  /*
   * turn off leaf-procedures.
   */
  target_flags &= ~TARGET_FLAG_LEAFPROC;

  /*
   * On the off chance that this is a 2-pass compilation, disallow
   * superblock formation.
   */
  flag_sblock = 0;

  /*
   * Turn off code-alignment, and cause functions to be aligned on
   * word boundaries.
   */
  target_flags &= ~TARGET_FLAG_CODE_ALIGN;
  i960_function_boundary = 32;
}

int
i960_override_options()
{
  /*
   * In case no arch was specified default to K series.  This keeps some
   * code that checks for specific series from having undefined results.
   */
  if (!TARGET_K_SERIES && !TARGET_C_SERIES && !TARGET_J_SERIES &&
      !TARGET_H_SERIES)
    target_flags |= TARGET_FLAG_K_SERIES;

  if (TARGET_C_SERIES &&
      (TARGET_K_SERIES || TARGET_J_SERIES || TARGET_H_SERIES))
  {
    warning ("conflicting architectures defined - using C series", 0);
    target_flags &= ~(TARGET_FLAG_K_SERIES |
                      TARGET_FLAG_J_SERIES | TARGET_FLAG_H_SERIES);
  }

  if (TARGET_K_SERIES && (TARGET_J_SERIES || TARGET_H_SERIES))
  {
    warning ("conflicting architectures defined - using K series", 0);
    target_flags &= ~(TARGET_FLAG_J_SERIES | TARGET_FLAG_H_SERIES);
  }

  if (TARGET_J_SERIES && TARGET_H_SERIES)
  {
    warning ("conflicting architectures defined - using J series", 0);
    target_flags &= ~(TARGET_FLAG_H_SERIES);
  }

  if (!TARGET_J_SERIES && !TARGET_H_SERIES && flag_cond_xform)
    flag_cond_xform = 0;

  if (TARGET_J_SERIES && flag_unroll_loops)
    flag_unroll_small_loops = 1;

  if (flag_shadow_mem==0)
    flag_coalesce = 0;

#ifdef IMSTG_BIG_ENDIAN
  if (bytes_big_endian)
  {
    if (flag_bout)
      fatal("Big-endian code generation requires the COFF or Elf object format");
      /* no return */
    flag_coalesce = 0;
  }
#endif /* IMSTG_BIG_ENDIAN */

  if (TARGET_CMPBRANCH && TARGET_NOCMPBRANCH)
  {
    warning ("both -mno-cmpbr and -mcmpbr specified, both ignored.");
    target_flags &= ~TARGET_FLAG_CMPBR_MASK;
  }

  /*
   * This is fatal because we probably got the preprocessor macro wrong
   * also.  This needs to be caught in the driver.
   */
  if (TARGET_ABI && !flag_signed_char)
    fatal ("Illegal option combination: -mabi and -funsigned-char");

  if (TARGET_ABI && TARGET_IC_COMPAT2_0)
    fatal ("Illegal option combination: -mabi and -mic2.0-compat");

  if (TARGET_ABI && i960_align_option != 0)
    fatal ("Illegal option combination: -mabi and -mi960_align");

  if (TARGET_ABI && flag_short_enums)
    fatal ("Illegal option combination: -mabi and -fshort-enums");

  if (TARGET_ABI)
    target_flags |= TARGET_FLAG_IC_COMPAT3_0;

  if (TARGET_CAVE)
  {
    if (flag_bout)
      fatal("CAVE code generation is not available with the B.OUT object format");
    target_flags |= TARGET_FLAG_CAVE;
    target_flags &= ~TARGET_FLAG_LEAFPROC;
#if 0
    target_flags |= TARGET_FLAG_USE_CALLX;
    target_flags &= ~TARGET_FLAG_TAILCALL;
#endif
    flag_inline_functions = 0;
    flag_space_opt = 1;
  }

  if (!(TARGET_NUMERICS))
  {
    int i;
    for (i = 32; i <= 35; i++)
      hard_regno_mode_ok[i] = 0;
  }

  if (flag_space_opt)
    i960_set_spaceopt_options();

#if 0
  if (flag_bout && flag_place_functions)
  {
    warning ("-fplace-functions doesn't work with B.OUT object format, -fplace-functions turned off.");
    flag_place_functions = 0;
  }
#endif

  /* For COFF and BOUT, treat all non-zero -g levels like -g2.
     For ELF/DWARF, treat -g1 like -g2.
   */
#ifdef SDB_DEBUGGING_INFO
  if (write_symbols == SDB_DEBUG && debug_info_level > 0)
    debug_info_level = DINFO_LEVEL_NORMAL;
#endif
#ifdef DBX_DEBUGGING_INFO
  if (write_symbols == DBX_DEBUG && debug_info_level > 0)
    debug_info_level = DINFO_LEVEL_NORMAL;
#endif
#ifdef DWARF_DEBUGGING_INFO
  if (write_symbols == DWARF_DEBUG && debug_info_level == DINFO_LEVEL_TERSE)
    debug_info_level = DINFO_LEVEL_NORMAL;
#endif


  if (USE_DB && BUILD_DB)
    fatal ("You cannot generate cc_info in a program-wide optimized recompilation");
  else
    if (USE_DB && PROF_CODE)
      fatal ("You cannot generate profiling code in a program-wide optimized recompilation");

  /* Disable fine-grained line number note tracking if not compiling
     for Elf/DWARF.
   */
  if (!flag_elf)
    flag_linenum_mods = 0;

  i960_initialize ();
}

int
i960_g14_contains_zero()
{
  if (current_func_has_argblk())
    return 0;
  if (i960_func_changes_g14)
    return 0;
  if (i960_in_isr && !i960_func_has_calls)
    return 0;

  return 1;
}

i960_output_arg_ptr_def()
{
  rtx ap_rtx = current_function_internal_arg_pointer;

  if (ap_rtx == 0)
    return;

  if (GET_CODE(ap_rtx) == REG)
  {
    int ap_regno = REGNO(current_function_internal_arg_pointer);

    if (ap_regno > FIRST_PSEUDO_REGISTER)
    {
      if (reg_renumber[ap_regno] < 0)
        ap_regno = 14;
      else
        ap_regno = reg_renumber[ap_regno];
    }
  
    if (write_symbols == SDB_DEBUG)
    {
      fprintf (asm_out_file,
               "\t.def\t.argptr; .val %d; .scl %d; .type 0x21; .endef\n",
               DBX_REGISTER_NUMBER (ap_regno), C_REG);
    }
    else if (write_symbols == DBX_DEBUG)
    {
      fprintf (asm_out_file, ".stabs \".argptr:r");
      dbxout_type(ptr_type_node);
      fprintf (asm_out_file, "\",%d,0,0,%d\n", N_RSYM,
               DBX_REGISTER_NUMBER (ap_regno));
    }
  }
  else if (GET_CODE(ap_rtx) == MEM)
  {
    rtx x = XEXP(ap_rtx,0);
    if (GET_CODE(x) == PLUS &&
        GET_CODE(XEXP(x,0)) == REG &&
        REGNO(XEXP(x,0)) == FRAME_POINTER_REGNUM &&
        GET_CODE(XEXP(x,1)) == CONST_INT)
    {
      int offset = INTVAL(XEXP(x,1));
      if (write_symbols == SDB_DEBUG)
      {
        fprintf (asm_out_file,
                 "\t.def\t.argptr; .val %d; .scl %d; .type 0x21; .endef\n",
                 offset, C_AUTO);
      }
      else if (write_symbols == DBX_DEBUG)
      {
        fprintf (asm_out_file, ".stabs \".argptr:");
        dbxout_type(ptr_type_node);
        fprintf (asm_out_file, "\",%d,0,0,%d\n", N_LSYM, offset);
      }
    }
  }
}

get_regno (x)
rtx x;
{
  if (GET_CODE(x) == REG)
    return REGNO(x);
  else if (GET_CODE(x) == SUBREG)
    return REGNO(SUBREG_REG(x)) + SUBREG_WORD(x);
  else
    abort();
}

/*
 * returns non-zero for any register that has a save cost.
 */
i960_reg_save_cost(regno)
int regno;
{
  if (regno <= 7 || regno == 13)
  {
    return i960_in_isr ? 1 : 0;
  }

  if (regno >= 19 && regno <= 31)
  {
    /*
     * if any other local registers have been used then
     * there is bno save cost for the others.
     */
    int i;
    for (i = 19; i <= 31; i++)
      if (regs_ever_live[i])
        return 0;
    return 1;
  }

  /* any others are either fixed or have a save cost associated with them. */
  return 1;
}

i960_encode_section_info(decl)
tree decl;
{
  rtx xx_rtx;
  switch (TREE_CODE(decl))
  {
    case VAR_DECL:
      xx_rtx = DECL_RTL(decl);
      if (GET_CODE(xx_rtx) == MEM && GET_CODE(XEXP(xx_rtx,0)) == SYMBOL_REF)
      {
        char *name;
        int n;
        xx_rtx = XEXP(xx_rtx,0);

        SYMREF_ETC(xx_rtx) |= SYMREF_VARBIT;

        if (TREE_READONLY(decl) && TARGET_PIC)
          SYMREF_ETC(xx_rtx) |= SYMREF_PICBIT;

        if (TREE_LINKAGE_INTERNAL(decl))
          SYMREF_ETC(xx_rtx) |= SYMREF_STATICBIT;

        name = XSTR(xx_rtx, 0);
        n = strlen(name)+2;
        if (i960_section_string != 0)
        {
          int ss_len = strlen(i960_section_string);
          n += ss_len + 1;
          XSTR(xx_rtx, 0) = xmalloc(n);
          sprintf(XSTR(xx_rtx, 0), "+%s+%s", i960_section_string, name);
          XSTR(xx_rtx, 0) += ss_len+2;
        }
        else
        {
          XSTR(xx_rtx,0) = xmalloc(n);
          sprintf (XSTR(xx_rtx,0), " %s", name);
          XSTR(xx_rtx,0) += 1;
        }
      }
      break;

    case THUNK_DECL:
    case FUNCTION_DECL:
      xx_rtx = DECL_RTL(decl);
      if (GET_CODE(xx_rtx) == MEM && GET_CODE(XEXP(xx_rtx,0)) == SYMBOL_REF)
      {
        xx_rtx = XEXP(xx_rtx,0);

        SYMREF_ETC(xx_rtx) |= SYMREF_FUNCBIT;
        if (TARGET_PIC)
          SYMREF_ETC(xx_rtx) |= SYMREF_PICBIT;

        if (TREE_LINKAGE_INTERNAL(decl))
          SYMREF_ETC(xx_rtx) |= SYMREF_STATICBIT;
      }
      break;

    case STRING_CST: 
      xx_rtx = TREE_CST_RTL(decl);
      if (xx_rtx != 0 &&
          GET_CODE(xx_rtx) == MEM && GET_CODE(XEXP(xx_rtx,0)) == SYMBOL_REF)
      {
        xx_rtx = XEXP(xx_rtx,0);

        if (TARGET_PIC && !flag_writable_strings)
          SYMREF_ETC(xx_rtx) |= SYMREF_PICBIT;

        SYMREF_ETC(xx_rtx) |= SYMREF_STATICBIT;
      }
      break;

    case CONSTRUCTOR:
    case INTEGER_CST:
    case REAL_CST:
    case COMPLEX_CST:
      xx_rtx = TREE_CST_RTL(decl);
      if (xx_rtx != 0 &&
          GET_CODE(xx_rtx) == MEM && GET_CODE(XEXP(xx_rtx,0)) == SYMBOL_REF)
      {
        xx_rtx = XEXP(xx_rtx,0);

        if (TARGET_PIC)
          SYMREF_ETC(xx_rtx) |= SYMREF_PICBIT;

        SYMREF_ETC(xx_rtx) |= SYMREF_STATICBIT;
      }
      break;

    case TYPE_DECL:
      /*
       * This is completely weird, but in some cases TYPE_DECLs
       * get passed into make_decl_rtl, and actually get to here
       * due to some gcc extensions.  Don't do anything for them.
       */
      break;

    default:
      warning("unexpected decl %s in i960_encode_section_info",
              tree_code_name[TREE_CODE(decl)]);
      break;
  }
}

int
i960_integrate_threshold(decl)
tree decl;
{
  if (i960_pragma_inline(decl) == PRAGMA_NO ||
      i960_pragma_isr(decl) == PRAGMA_DO ||
      i960_pragma_interrupt(decl) == PRAGMA_DO ||
      i960_system_function(decl))
    return 0;

  if (i960_pragma_inline(decl) == PRAGMA_DO)
    return 0x10000000;  /* HUGE NUMBER */

  return (i960_integrate_const * (8 + list_length (DECL_ARGUMENTS(decl))));
}

int
i960_global_integrate_threshold(decl)
tree decl;
{
  if (i960_pragma_inline(decl) == PRAGMA_NO ||
      i960_pragma_isr(decl) == PRAGMA_DO ||
      i960_pragma_interrupt(decl) == PRAGMA_DO ||
      i960_system_function(decl))
    return 0;

  return 1000;
}

/*
 * This routine is written so that all intermediate double calculations
 * get rounded to double before having further calculation done on them.
 * This helps assure that any code using floating calculations is portable.
 */
#ifndef IMSTG_REAL
double
d_to_d(d)
double d;
{
  return d;
}
#endif

static void
find_mem_mk_valid(insn, x, x_p)
rtx insn;
rtx x;
rtx *x_p;
{
  register char* fmt;
  register int i;
  int code;

  if (x == 0)
    return;

  code = GET_CODE(x);

  if (code == MEM)
  {
    rtx sav;
    rtx addr_seq;

    if (memory_address_p(GET_MODE(x), XEXP(x,0)))
      return;

    START_SEQUENCE (sav);
    *x_p = change_address(x, VOIDmode, NULL_RTX);
    addr_seq = gen_sequence();
    END_SEQUENCE (sav);

    emit_insn_before(addr_seq, insn);
    return;
  }

  fmt = GET_RTX_FORMAT (code);
  i   = GET_RTX_LENGTH (code);
  while (--i >= 0)
  {
    if (fmt[i] == 'e')
      find_mem_mk_valid(insn, XEXP(x,i), &XEXP(x,i));
    else if (fmt[i] == 'E')
    {
      register int j = XVECLEN(x,i);
      while (--j >= 0)
        find_mem_mk_valid(insn, XVECEXP(x,i,j), &XVECEXP(x,i,j));
    }
  }
}

void
make_mems_valid(insns)
rtx insns;
{
  rtx t_insn;

  for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    if (GET_RTX_CLASS(GET_CODE(t_insn)) == 'i')
      find_mem_mk_valid(t_insn, PATTERN(t_insn), &PATTERN(t_insn));
  }
}

static int saved_target_flags = 0;
static int saved_space_opt = 0;
static int flags_were_saved = 0;
static int saved_flag_unroll_loops;
static int saved_flag_unroll_small_loops;
static int saved_flag_sblock;
static int saved_flag_inline_functions;
static int saved_flag_no_inline;
static int saved_i960_function_boundary;

static void
interpret_opt_string(opt_s, warn_only)
char *opt_s;
int  warn_only;	/* Don't update target_flags, just issue warnings
		   for unrecognized options.
		 */
{
  /* parsing for #pragma optimize string */

  while (opt_s != 0)
  {
    char *opt_start;
    char *opt_end;
    char *opt_base;
    struct pragma_optimize_option *poo;
    int enable;
    unsigned int mask;
  
    opt_start = opt_s;

    opt_end = strchr(opt_s, ',');
    if (opt_end != 0)
    {
      *opt_end = '\0';
      opt_s = opt_end+1;
    }
    else
      opt_s = opt_end;

    if (strncmp(opt_start, "no", 2) == 0)
    {
      enable = 0;
      opt_base = opt_start + 2;
    }
    else
    {
      enable = 1;
      opt_base = opt_start;
    }

    mask = 0;
    for (poo = pragma_optimize_options; poo->name; poo++)
    {
      if (strcmp(opt_base, poo->name) == 0)
      {
        mask = poo->mask;
        break;
      }
    }

    if (warn_only)
    {
      if (!mask)
        warning ("unrecognized or unsupported control `%s' in #pragma optimize - control ignored.", opt_start);
    }
    else if (mask)
    {
      if (enable)
        target_flags |= mask;
      else
        target_flags &= ~mask;
    }

    if (opt_end != 0)
      *opt_end = ',';
  }
}

i960_change_flags_for_function(decl)
tree decl;
{
  int t;
  char *opt_string;

  flags_were_saved = 1;
  saved_target_flags = target_flags;
  saved_space_opt = flag_space_opt;
  saved_flag_unroll_loops = flag_unroll_loops;
  saved_flag_unroll_small_loops = flag_unroll_small_loops;
  saved_flag_sblock = flag_sblock;
  saved_flag_inline_functions = flag_inline_functions;
  saved_flag_no_inline = flag_no_inline;
  saved_i960_function_boundary = i960_function_boundary;
  i960_ok_to_adjust = 1;

  if (i960_pragma_cave(decl) == PRAGMA_DO)
  {
    target_flags |= TARGET_FLAG_CAVE;
#if 0
    target_flags |= TARGET_FLAG_USE_CALLX;
    target_flags &= ~TARGET_FLAG_TAILCALL;
#endif
    target_flags &= ~TARGET_FLAG_LEAFPROC;
    i960_set_spaceopt_options();
    i960_ok_to_adjust = 0;
  }

  t = i960_pragma_compress(decl);
  if (t == PRAGMA_DO)
  {
    i960_set_spaceopt_options();
    i960_ok_to_adjust = 0;
  }

  if (t == PRAGMA_NO)
  {
    target_flags &= ~TARGET_FLAG_ADDR_MASK;
    target_flags |= TARGET_FLAG_REG_OR_DIRECT_ONLY;

    target_flags &= ~TARGET_FLAG_CMPBR_MASK;
    target_flags |= TARGET_FLAG_NOCMPBRANCH;

    flag_space_opt = 0;
    i960_ok_to_adjust = 0;
  }

  opt_string = i960_pragma_optimize(decl);
  if (opt_string != 0)
    interpret_opt_string(opt_string, 0);
}

i960_restore_flags_after_function(decl)
tree decl;
{
  if (flags_were_saved)
  {
    target_flags = saved_target_flags;
    flag_space_opt = saved_space_opt;
    flag_unroll_loops = saved_flag_unroll_loops;
    flag_unroll_small_loops = saved_flag_unroll_small_loops;
    flag_sblock = saved_flag_sblock;
    flag_inline_functions = saved_flag_inline_functions;
    flag_no_inline = saved_flag_no_inline;
    i960_function_boundary = saved_i960_function_boundary;
  }

  flags_were_saved = 0;
  i960_ok_to_adjust = 1;
}

/*
 * For common objects, output rounded size.  Also, if size
 * is 0, treat this as a declaration, not a definition - i.e.,
 * do nothing at all.
 * Check to see if this is supposed to be allocated to SRAM and
 * do the appropriate thing.
 */
i960_asm_output_common(f, name, size)
FILE *f;
char *name;
int size;
{
  if (size != 0)
  {
    int ob_align = OBJECT_BYTES_BITALIGN(size);
    int rndsiz = ROUND(size,ob_align/8);
    int sram_addr = glob_sram_addr(name);
    char buf[30];

    if (sram_addr != 0)
    {
      sprintf (buf, "__sram.%u", sram_addr);
      fprintf (f, "\t.globl\t_%s\n", name);
      fprintf (f, "\t.set\t_%s,%u\n", name, sram_addr);

      if (flag_elf)
      {
        fprintf (f, "\t.elf_type\t_%s,object\n", name);
        fprintf (f, "\t.elf_size\t_%s,%d\n", name, size);
      }

      fprintf (f, "\t.globl\t___sram_length.%u\n", sram_addr);
      fprintf (f, "\t.set\t___sram_length.%u,%u\n", sram_addr, size);
      name = buf;
    }

    fputs ("\t.globl\t", f);
    assemble_name (f, name);

    if (name[-1] == '+' || i960_section_string != 0)
    {
      char *sect = i960_section_string;
      char *nul = 0;

      if (name[-1] == '+')
      {
        sect = &name[-1];
        nul = &name[-1];
        name[-1] = '\0';
        while (sect[-1] != '+')
          sect -= 1;
      }
      else
        assert (name[-1] == ' ');

      fprintf (f, "\n.section\t.bss%s,bss\n", sect);
      assemble_name (f, name);
      fputs (":\n", f);
      fprintf (f, "\t.align\t%d\n",
                  ((ob_align) <= 8 ? 0 : ((ob_align) <= 16 ? 1
                    : ((ob_align) <= 32 ? 2
                       : ((ob_align <= 64 ? 3 : 4))))));
      fprintf (f, "\t.space\t%d\n", rndsiz);
      if (nul != 0)
        *nul = '+';
      set_no_section();
    }
    else if (TARGET_STRICT_REF_DEF)
    {
      fputs ("\n\t.bss\t", f);
      assemble_name (f, name);
      fprintf (f, ",%d,%d\n", rndsiz,
               ((ob_align) <= 8 ? 0 : ((ob_align) <= 16 ? 1
               : ((ob_align) <= 32 ? 2 
                  : ((ob_align <= 64 ? 3 : 4))))));
    }
    else
    {
      fputs ("\n\t.comm\t", f);
      assemble_name (f, name);
      fprintf (f, ",%d\n", rndsiz);
    }

    if (flag_elf)
    {
      fputs ("\t.elf_type\t", f);
      assemble_name (f, name);
      fputs (",object\n", f);

      fputs ("\t.elf_size\t", f);
      assemble_name (f, name);
      fprintf (f, ",%d\n", rndsiz);
    }
  }
}

/*
 * This says how to output an assembler line to define a local common symbol.
 * Output padded size, with request to linker to align as requested.
 * 0 size should not be possible here.
 * Check to see if this is supposed to be allocated to SRAM and
 * do the appropriate thing.
 */
i960_asm_output_local(f, name, size)
FILE *f;
char *name;
int size;
{
  int ob_align = OBJECT_BYTES_BITALIGN(size);
  int rndsiz = ROUND(size,ob_align/8);
  int sram_addr = glob_sram_addr(name);
  char buf[30];

  if (sram_addr != 0)
  {
    sprintf (buf, "__sram.%u", sram_addr);
    fprintf (f, "\t.globl\t_%s\n", name);
    fprintf (f, "\t.set\t_%s,%u\n", name, sram_addr);

    if (flag_elf)
    {
      fprintf (f, "\t.elf_type\t_%s,object\n", name);
      fprintf (f, "\t.elf_size\t_%s,%d\n", name, size);
    }

    fprintf (f, "\t.globl\t___sram_length.%u\n", sram_addr);
    fprintf (f, "\t.set\t___sram_length.%u,%u\n", sram_addr, size);
    name = buf;
  }

  if (GLOBALIZE_STATICS)
  {
    fputs ("\t.globl\t", f);
    assemble_name (f, name);
  }

  if (name[-1] == '+' || i960_section_string != 0)
  {
    char *sect = i960_section_string;
    char *nul = 0;

    if (name[-1] == '+')
    {
      sect = &name[-1];
      nul = &name[-1];
      name[-1] = '\0';
      while (sect[-1] != '+')
        sect -= 1;
    }
    else
      assert (name[-1] == ' ');

    fprintf (f, "\n.section\t.bss%s,bss\n", sect);
    assemble_name (f, name);
    fputs (":\n", f);
    fprintf (f, "\t.align\t%d\n",
                ((ob_align) <= 8 ? 0 : ((ob_align) <= 16 ? 1
                  : ((ob_align) <= 32 ? 2
                     : ((ob_align <= 64 ? 3 : 4))))));
    fprintf (f, "\t.space\t%d\n", rndsiz);
    if (nul != 0)
      *nul = '+';
    set_no_section();
  }
  else
  {
    fputs ("\n\t.bss\t", f);
    assemble_name (f, name);
    fprintf (f, ",%d,%d\n", (rndsiz),
             ((ob_align) <= 8 ? 0
              : ((ob_align) <= 16 ? 1
                 : ((ob_align) <= 32 ? 2
                    : ((ob_align <= 64 ? 3 : 4))))));
  }

  if (flag_elf)
  {
    fputs ("\t.elf_type\t", f);
    assemble_name (f, name);
    fputs (",object\n", f);

    fputs ("\t.elf_size\t", f);
    assemble_name (f, name);
    fprintf (f, ",%d\n", rndsiz);
  }
}

static tree* ic2_enum_order[] = {
  &signed_char_type_node,
  &short_integer_type_node,
  &integer_type_node,
  0
};

static tree* ic3_enum_order[] = {
  &signed_char_type_node,
  &unsigned_char_type_node,
  &short_integer_type_node,
  &short_unsigned_type_node,
  &integer_type_node,
  &unsigned_type_node,
  0
};

static tree* abi_enum_order[] = {
  &integer_type_node,
  0
};

static tree* gcc_short_enum_order[] = {
  &unsigned_char_type_node,
  &signed_char_type_node,
  &short_unsigned_type_node,
  &short_integer_type_node,
  &unsigned_type_node,
  &integer_type_node,
  0
};

static tree* gcc_normal_enum_order[] = {
  &unsigned_type_node,
  &integer_type_node,
  0
};

/* Pick high and low bounds and precision for enums, according to 960 rules. */
i960_setup_enum (penumtype, pminnode, pmaxnode)
tree *penumtype;
tree *pminnode;
tree *pmaxnode;
{
  tree enumtype = *penumtype;
  tree minnode  = *pminnode;
  tree maxnode  = *pmaxnode;
  tree type,**t;

  if (flag_short_enums)
    t = gcc_short_enum_order;
  else if (TARGET_IC_COMPAT3_0)
  {
    if (TARGET_ABI)
      t = abi_enum_order;
    else
      t = ic3_enum_order;
  }
  else if (TARGET_IC_COMPAT2_0)
    t = ic2_enum_order;
  else
    t = gcc_normal_enum_order;

  while (*t && (INT_CST_LT (minnode,TYPE_MIN_VALUE(**t)) ||
                INT_CST_LT (TYPE_MAX_VALUE(**t), maxnode)))
    t++;

  if (*t == 0)
  { error ("illegal enumeration range");
    type = integer_type_node;
  }
  else
    type = **t;

  TYPE_PRECISION (enumtype) = TYPE_PRECISION(type);
  TYPE_MIN_VALUE (enumtype) = *pminnode = TYPE_MIN_VALUE(type);
  TYPE_MAX_VALUE (enumtype) = *pmaxnode = TYPE_MAX_VALUE(type);
}

void
i960_check_pic_pid(x)
rtx x;
{
  if (i960_const_pic_pid_p(x, 1, 0) && TARGET_PIC)
    warning ("initialization uses address of position independent code object");

  if (i960_const_pic_pid_p(x, 0, 1) && TARGET_PID)
    warning ("initialization uses address of position independent data object");
}

/*
 * this function returns 1 if this function contains calls to other functions.
 * we are not going to consider calls to volatile functions to be calls,
 * since these "calls" are really just non-local jumps of a sort.
 * However, we need to consider such calls calls if in an ISR.
 */
int
i960_leaf_function_p(insns, in_isr)
rtx insns;
int in_isr;
{
  rtx t_insn;
  for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    if (GET_CODE(t_insn) == CALL_INSN &&
        (in_isr ||
         (NEXT_INSN(t_insn) == 0 ||
          GET_CODE(NEXT_INSN(t_insn)) != BARRIER)))
      return 0;
  }
  return 1;
}

static int i960_reg_alloc_order[2][FIRST_PSEUDO_REGISTER] =
  { REG_ALLOC_ORDER, ISR_REG_ALLOC_ORDER };

/*
 * This routine has the funny side effect that it also
 * sets the variables i960_func_has_calls, and i960_in_isr.
 */
i960_pick_reg_alloc_order()
{
  /*
   * use ISR reg allocation order only if the ISR routine doesn't
   * contain any calls.
   */
  i960_in_isr = i960_interrupt_handler();
  i960_func_has_calls = !i960_leaf_function_p(get_insns(), i960_in_isr);

  bcopy (i960_reg_alloc_order[i960_in_isr && !i960_func_has_calls],
         reg_alloc_order, FIRST_PSEUDO_REGISTER * sizeof(int));

  /*
   * make r3 be a fixed register if we are compiling an interrupt routine.
   * This is done because the i960 somtimes saves the IMSK register into
   * r3 upon servicing an interrupt.  r3 must then be used at the end of
   * the interrupt routine to restore the IMSK register.
   */
  if (i960_in_isr)
    hard_regno_mode_ok[19] = 0;  /* this keeps any one from allocating it */
  else
    hard_regno_mode_ok[19] = S_MODES;
}

/* Return 1 if the given register is reserved for a specific purpose for
 * the current function, else return 0.  This check, and setting
 * hard_regno_mode_ok to 0 in i960_pick_reg_alloc_order, are used to
 * treat registers as fixed on a per function basis.
 */
int
i960_hard_regno_function_reserved_p(regno)
int regno;
{
	/* r3 has a dedicated purpose in an interrupt function */
	if (regno == 19 && i960_interrupt_handler())
		return 1;
	return 0;
}

char *
i960_strip_name_encoding (targ, src)
char *targ;
char *src;
{
  /* Get just the user-specified name out of the string in a SYMBOL_REF.
   * On most machines, we discard the leading * if any and that's all.
   * For the 960, we may have trailing junk (after '.') to discard.
   *
   * This routine always allocates space if there is trailing crap that
   * needs to be deleted and targ is NULL.
   */

  char *t;

  if (*src == '*')
    src++;

  if ((t = strchr (src, '.')) != 0)
  {
    int n = t - src;
    if (targ == 0)
      targ = xmalloc(n + 1);
    strncpy(targ, src, n);
    targ[n] = '\0';
  }
  else
  {
    if (targ != 0)
      strcpy(targ, src);
    else
      targ = src;
  }

  return targ;
}
