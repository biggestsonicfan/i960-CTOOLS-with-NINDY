#include "config.h"

#ifdef IMSTG
/* Global Procedure integration for GNU CC.
   Copyright (C) 1990 Free Software Foundation, Inc.
   Contributed by Kevin B. Smith. Intel Corp.

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

#include <stdio.h>

#include "assert.h"
#include "rtl.h"
#include "tree.h"
#include "flags.h"
#include "expr.h"
#include "output.h"
#include "cc_info.h"
#include "i_profile.h"
#include "hard-reg-set.h"

#include "obstack.h"
#define	obstack_chunk_alloc	xmalloc
#define	obstack_chunk_free	free

#include "c-tree.h"
#include "i_double.h"
#include "insn-flags.h"
#include "i_glob_db.h"

extern void free ();

extern struct obstack *rtl_obstack;
extern struct obstack permanent_obstack;

extern char * get_call_name ();

#define MAX_PARMS	30

/*
 * data structure that allows remapping incoming parameters into
 * pseudo registers.  This isn't strictly necessary, but generates
 * better code when it can be done.
 */
struct parm_remap_info
{
  rtx incoming_rtl;
  rtx remap_pseudo;
  rtx copy_insn;
};

static struct parm_remap_info parm_remaps[MAX_PARMS];
static struct parm_remap_info ret_remap;

/*
 * An rtx created for mapping the frame pointer register.
 */
static rtx pseudo_frame_reg;

/* array of rtx for renaming labels during inlining */
static rtx *label_map;
static int min_lab;
static int max_lab;

/* array of rtx for renaming the pseudo registers during inlining */
static rtx *pseudo_reg_map;
static int num_regs;

/*
 * array of rtx for insns, for rebuilding the links (format 'u') between
 * insns in a second pass during inlining.
 */
static rtx *uid_to_insn_map;
static int num_uid;

/*
 * rtx for label used for translating return rtx into jump to ret_label.
 * ret_label is then placed at the end of the inlined function.
 */
static rtx ret_label;

/*
 * translation mode during inlining.  Changes on an insn by insn basis.
 */
static int inline_translate_mode;

/*
 * used for noting when an rtx for a SYMBOL_REF has already been packed
 * in a particular function.
 */
static unsigned char *sym_pck_tab;

/*
 * used for noting when a rtx for a pseudo register has already been
 * packed in a particular function.
 */
static unsigned char *pseudo_reg_tab;

/*
 * rtx for assuring that SYMBOL_REF rtx remains shared int the rtl
 * This is used both in saving the rtl for inlining, and when inlining
 * the routine.
 */
static rtx *sym_ref_tab;
static int max_sym;

/*
 * Used to assure proper parts of ASM_OPERANDS are shared.
 */
static int in_asm_operands = 0;
static rtvec orig_asm_operands_vector;
static rtvec orig_asm_constraints_vector;

/*
 * Used to assure proper sharing of MODE_CC registers.
 */
static rtx last_cc_rtx;

/*
 * Name of function currently being inlined.
 */
static char *func_being_inlined;

#define PK_NULL           0
#define PK_NON_NULL       1
#define PK_PC_RTX         2
#define PK_CC0_RTX        3
#define PK_CNST0_RTX      4
#define PK_CNST1_RTX      5
#define PK_CNST_RTX       6
#define PK_HARD_REG_RTX   7
#define PK_PSEUDO_REG_DEF_RTX 8
#define PK_PSEUDO_REG_REF_RTX 9
#define PK_FP_REG_RTX     10
#define PK_LABEL_REF_RTX  11
#define PK_CODE_LABEL_RTX 12
#define PK_INSN_LEVEL_RTX 13
#define PK_RETURN_RTX     16
#define PK_SYM_REF_RTX    17
#define PK_SYM_DEF_RTX    18
#define PK_FCNST0_RTX     19
#define PK_DCNST0_RTX     20
#define PK_FCNST_RTX      21
#define PK_DCNST_RTX      22
#define PK_ASM_OPS_RTX    23
#define PK_ASM_CNSTS_RTX  24
#define PK_MEM_RTX        25
#define PK_CNST2_RTX      26
#define PK_TCNST0_RTX     27
#define PK_TCNST_RTX      28

#define PK_TYPE_NULL       0
#define PK_TYPE_SCHAR      1
#define PK_TYPE_UCHAR      2
#define PK_TYPE_SSHORT     3
#define PK_TYPE_USHORT     4
#define PK_TYPE_SLONG      5
#define PK_TYPE_ULONG      6
#define PK_TYPE_SLLONG     7
#define PK_TYPE_ULLONG     8
#define PK_TYPE_FLOAT      9
#define PK_TYPE_DOUBLE    10
#define PK_TYPE_LDOUBLE   11
#define PK_TYPE_FUNCTION  12
#define PK_TYPE_PTR       0x10     /* just gets anded into the other type */

/*
 * all these PK macros assume that the bits of the u32 for the field
 * are already zero.
 */
#define PK_RTX_CODE(x,u32) ((u32) |= ((((int)x) & 0xFFFF) << 16))
#define PK_RTX_CODE_CHEAT(x, u32) ((u32) |= ((((int)x) & 0xFFFF) << 16))
#define PK_RTX_MODE(x,u32) ((u32) |= ((((int)x) & 0xFF) << 8))
#define PK_RTX_REPORT_UNDEF(x,u32) ((u32) |= (((x) & 1) << 7))
#define PK_RTX_REPORT_UNUSED(x,u32) ((u32) |= (((x) & 1) << 6))
#define PK_RTX_UNCG(x,u32) ((u32) |= (((x) & 1) << 5))
#define PK_RTX_VOL(x,u32)  ((u32) |= (((x) & 1) << 4))
#define PK_RTX_INST(x,u32) ((u32) |= (((x) & 1) << 3))
#define PK_RTX_USED(x,u32) ((u32) |= (((x) & 1) << 2))
#define PK_RTX_INTG(x,u32) ((u32) |= (((x) & 1) << 1))

#define UNPK_RTX_CODE(u32) ((enum rtx_code)(((u32) >> 16) & 0xFFFF))
#define UNPK_RTX_CODE_CHEAT(u32) (((u32) >> 16) & 0xFFFF)
#define UNPK_RTX_MODE(u32) ((enum machine_mode)(((u32) >> 8) & 0xFF))
#define UNPK_RTX_REPORT_UNDEF(u32) (((u32) >> 7) & 1)
#define UNPK_RTX_REPORT_UNUSED(u32) (((u32) >> 6) & 1)
#define UNPK_RTX_UNCG(u32) (((u32) >> 5) & 1)
#define UNPK_RTX_VOL(u32)  (((u32) >> 4) & 1)
#define UNPK_RTX_INST(u32) (((u32) >> 3) & 1)
#define UNPK_RTX_USED(u32) (((u32) >> 2) & 1)
#define UNPK_RTX_INTG(u32) (((u32) >> 1) & 1)

/* Default max number of insns a function can have and still be inline. */
#ifndef GLOBAL_INTEGRATE_THRESHOLD
#define GLOBAL_INTEGRATE_THRESHOLD(DECL) 1000
#endif

/*
 * Forward declarations for static functions.
 */
static void expand_global_inline ();
static int pack_string ();
static int pack_rtvec ();
static int pack_rtx ();
static int pack_rtl ();
static int unpack_string ();
static int unpack_rtvec ();
static int unpack_rtx ();
static void unpack_remap_insns ();


int
count_real_insns(insn, max_insns)
rtx insn;
int max_insns;
{
  /* Return real insn count, starting at insn, not to exceed max_insns */

  int n = 0;

  while (insn && n < max_insns)
  {
    if (GET_CODE(insn)==INSN || GET_CODE(insn)==JUMP_INSN ||
        GET_CODE(insn)==CALL_INSN)
      n++;

    insn=NEXT_INSN(insn);
  }

  return n;
}

/*
 * int function_globally_inlinable_p (tree fndecl);
 *
 * Return 1 if the function declaration passed is ok for
 * global inlining, 0 if it is not.
 */
int
function_globally_inlinable_p(fndecl)
  register tree fndecl;
{
  register rtx insn;
  int max_insns = GLOBAL_INTEGRATE_THRESHOLD (fndecl);
  register int ninsns = 0;
  register tree parms;

  /*
   * function "main" is not inlinable.
   */
  if (strcmp(XSTR(XEXP(DECL_RTL(fndecl),0),0), "main") == 0)
    return 0;

  /* No inlines with varargs.  `grokdeclarator' gives a warning
     message about that if `inline' is specified.  This code
     it put in to catch the volunteers.  */
  if (imstg_func_is_varargs_p(fndecl))
    return 0;

  if (current_function_calls_alloca)
    return 0;

  /* If its not even close, don't even look.  */
  if (!DECL_INLINE (fndecl) && get_max_uid () > 3 * max_insns)
    return 0;

  /* We can't globally inline functions that return through memory. */
  if (DECL_RTL(DECL_RESULT(fndecl)) != 0 &&
      GET_CODE(DECL_RTL(DECL_RESULT(fndecl))) == MEM)
    return 0;

  /*
   * Don't inline functions which have any arguments that are passed in
   * memory. This is done this way for now because I don't have a good
   * way to rewrite without having register arguments.
   * Don't inline functions that take the address of a parameter and do
   * not specify a function prototype.
   */
  for (parms = DECL_ARGUMENTS (fndecl); parms; parms = TREE_CHAIN (parms))
  {
    /* Don't globally inline any functions which have parameters in the
     * arg_block. */
    if (GET_CODE(DECL_INCOMING_RTL(parms)) != REG)
      return 0;
  }

  if (!DECL_INLINE (fndecl) && get_max_uid () > max_insns)
    if (count_real_insns (get_first_nonparm_insn (), max_insns) >= max_insns)
      return 0;
  return 1;
}

/*
 * save_for_global_inline ()
 *
 */
int
save_for_global_inline (fndecl, insns, buf_p)
  tree fndecl;
  rtx insns;
  unsigned char **buf_p;
{
  rtx inln_head;
  rtx t_insn;
  unsigned char *buf;
  int size;
  int i;

  /*
   * First build the global inline header.
   */
  inln_head = rtx_alloc(GLOBAL_INLINE_HEADER);
  GLOB_INLN_FRAME_SIZE(inln_head)    = get_frame_size() + outgoing_args_size();
  GLOB_INLN_FIRST_LABELNO(inln_head) = get_first_label_num();
  GLOB_INLN_LAST_LABELNO(inln_head)  = max_label_num();
  GLOB_INLN_NUM_REGS(inln_head)      = max_reg_num();
  GLOB_INLN_NUM_UIDS(inln_head)      = get_max_uid()+1;
  GLOB_INLN_NUM_SYMS(inln_head)      = last_global_rtx_id[IDF_SYM]+1;
  GLOB_INLN_RET_RTX(inln_head)       = DECL_RTL(DECL_RESULT(fndecl));
  GLOB_INLN_PARMS(inln_head)         = 0;

  INSN_UID(inln_head) = GLOB_INLN_NUM_UIDS(inln_head)-1;
  PREV_INSN(inln_head) = 0;
  NEXT_INSN(inln_head) = insns;

  /* allocate space for the mapping table for symbol refs */
  sym_pck_tab = (unsigned char *)xmalloc(last_global_rtx_id[IDF_SYM]+1);
  bzero(sym_pck_tab, last_global_rtx_id[IDF_SYM]+1);

  /* allocate space for the table of pseudo register defs. */
  pseudo_reg_tab = (unsigned char *)xmalloc(GLOB_INLN_NUM_REGS(inln_head));
  bzero(pseudo_reg_tab, GLOB_INLN_NUM_REGS(inln_head));

  /* now pack everything into a buffer */
  size = pack_rtl((unsigned char *)0, inln_head);
  size += 4;  /* for size indicator in front */

  buf = (unsigned char *)xmalloc(size);

  /*
   * need to rezero sym_pck_tab, otherwise we'll pack it different the
   * second time.  Same for pseudo_reg_tab.
   */
  bzero(sym_pck_tab, last_global_rtx_id[IDF_SYM]+1);
  bzero(pseudo_reg_tab, GLOB_INLN_NUM_REGS(inln_head));
  pack_rtl(buf+4, inln_head);

  free (sym_pck_tab);

  /* put rtl size in front of buffer */
  CI_U32_TO_BUF(buf, size);

  *buf_p = buf;   /* return the pointer to the buffer */
  return size;  /* and the number of bytes in it. */
}

/*
 * Look from insn forward to a NOTE_INSN_FUNCTION_ENTRY instruction looking
 * for sets of psuedo-registers to hard registers.  These define the
 * parm remaps.
 */
static int
find_parm_remaps(body_insn, call_insn, caller_name, callee_name)
rtx body_insn;
rtx call_insn;
char *caller_name;
char *callee_name;
{
  int n_remaps = 0;
  rtx t_insn;
  int i;
  int need_warn = 0;
  HARD_REG_SET actual_parm_set_regs;

  bzero(parm_remaps, sizeof(parm_remaps));

  t_insn = body_insn;
  while (t_insn != 0 && n_remaps < MAX_PARMS &&
         GET_CODE(t_insn) == INSN &&
         GET_CODE(PATTERN(t_insn)) == SET &&
         GET_CODE(SET_SRC(PATTERN(t_insn))) == REG &&
         REGNO(SET_SRC(PATTERN(t_insn))) < FIRST_PSEUDO_REGISTER)
  {
    if (REGNO(SET_SRC(PATTERN(t_insn))) != ARG_POINTER_REGNUM)
    {
      if (GET_CODE(SET_DEST(PATTERN(t_insn))) == MEM)
      {
        /* make up a pseudo to use for remapping */
        rtx treg = gen_reg_rtx(GET_MODE(SET_DEST(PATTERN(t_insn))));
        emit_insn_after(gen_rtx (SET, VOIDmode,
                                 SET_DEST(PATTERN(t_insn)),
                                 treg),
                         t_insn);
        SET_DEST(PATTERN(t_insn)) = treg;

        parm_remaps[n_remaps].incoming_rtl = SET_SRC(PATTERN(t_insn));
        parm_remaps[n_remaps].remap_pseudo = SET_DEST(PATTERN(t_insn));
        parm_remaps[n_remaps].copy_insn    = t_insn;
        n_remaps += 1;

        /* this will get us past this new instruction we created. */
        t_insn = NEXT_INSN(t_insn);
      }
      else if (GET_CODE(SET_DEST(PATTERN(t_insn))) == REG &&
               REGNO(SET_DEST(PATTERN(t_insn))) >= FIRST_PSEUDO_REGISTER)
      {
        parm_remaps[n_remaps].incoming_rtl = SET_SRC(PATTERN(t_insn));
        parm_remaps[n_remaps].remap_pseudo = SET_DEST(PATTERN(t_insn));
        parm_remaps[n_remaps].copy_insn    = t_insn;
        n_remaps += 1;
      }
      else
        break;
    }

    t_insn = NEXT_INSN(t_insn);
  }

  if (n_remaps == 0)
    return 0;

  /*
   * Figure out the set of hard registers that are actually set
   * coming into this call.
   */
  CLEAR_HARD_REG_SET(actual_parm_set_regs);
  t_insn = PREV_INSN(call_insn);
  while (t_insn != 0)
  {
    if (GET_CODE(t_insn) == NOTE &&
        NOTE_LINE_NUMBER(t_insn) == NOTE_INSN_CALL_BEGIN)
      break;

    if (GET_CODE(t_insn) == INSN &&
        GET_CODE(PATTERN(t_insn)) == SET &&
        GET_CODE(SET_DEST(PATTERN(t_insn))) == REG &&
        REGNO(SET_DEST(PATTERN(t_insn))) < FIRST_PSEUDO_REGISTER)
    {
      rtx reg = SET_DEST(PATTERN(t_insn));
      int rgno = REGNO(reg);
      int rgend = rgno + HARD_REGNO_NREGS(rgno, GET_MODE(reg));
      for (; rgno < rgend; rgno++)
        SET_HARD_REG_BIT(actual_parm_set_regs, rgno);
    }

    t_insn = PREV_INSN(t_insn);
  }

  /*
   * Go through the remaps and find any remaps which are not wholly
   * set based on the register set we just computed.  Generate SETs
   * for any that aren't so there won't be any uninitialized params,
   * which although a user problem can cause live range problems for
   * the hard registers later in the compile. Note that we plan to add
   * any sets immediately prior to call_insn.
   */
  for (i = 0; i < n_remaps; i++)
  {
    rtx reg = parm_remaps[i].incoming_rtl;
    int rgno = REGNO(reg);
    int rgend = rgno + HARD_REGNO_NREGS(rgno, GET_MODE(reg));
    for (; rgno < rgend; rgno++)
    {
      if (!TEST_HARD_REG_BIT(actual_parm_set_regs, rgno))
      {
        need_warn = 1;
        emit_insn_before(gen_rtx(SET, VOIDmode,
                                 gen_rtx(REG, word_mode, rgno),
                                 GEN_INT(0)),
                         call_insn);
      }
    }
  }

  if (need_warn)
  {
    char buf[1000];
    sprintf (buf, "parameter mismatch in call to %s from %s",
                   callee_name, caller_name);
    warning (buf);
    return -1;
  }

  return n_remaps;
}

static void
remap_parms(n_remaps, call_insn)
int n_remaps;
rtx call_insn;
{
  rtx t_insn;
  rtx del_insn;
  int i;

  for (i = 0; i < n_remaps; i++)
  {
    t_insn = PREV_INSN(call_insn);
    while (t_insn != 0)
    {
      if (GET_CODE(t_insn) == NOTE &&
          NOTE_LINE_NUMBER(t_insn) == NOTE_INSN_CALL_BEGIN)
        break;

      if (GET_CODE(t_insn) == INSN &&
          GET_CODE(PATTERN(t_insn)) == SET &&
          GET_CODE(SET_DEST(PATTERN(t_insn))) == REG &&
          REGNO(SET_DEST(PATTERN(t_insn))) < FIRST_PSEUDO_REGISTER)
      {
        rtx parm_reg = SET_DEST(PATTERN(t_insn));
        rtx in_reg = parm_remaps[i].incoming_rtl;
        if (REGNO(parm_reg) == REGNO(in_reg) &&
            GET_MODE(in_reg) == GET_MODE(parm_reg))
        {
          SET_DEST(PATTERN(t_insn)) = parm_remaps[i].remap_pseudo;

          del_insn = parm_remaps[i].copy_insn;
          PUT_CODE(del_insn, NOTE);
          NOTE_LINE_NUMBER (del_insn) = NOTE_INSN_DELETED;
          NOTE_SOURCE_FILE (del_insn) = 0;
        }
      }
      t_insn = PREV_INSN(t_insn);
    }
  }
}

static int
find_return_remap(body_insn, call_insn, caller_name, callee_name)
rtx body_insn;
rtx call_insn;
char *caller_name;
char *callee_name;
{
  /*
   * The instruction immediately following call_insn should copy the
   * return value register into a pseudo register.  If it doesn't it means
   * that the return value isn't used.  Store this information in
   * ret_remap.
   */
  int need_warn = 0;
  rtx t_insn = NEXT_INSN(call_insn);
  HARD_REG_SET return_reg_set;

  if (t_insn != 0 &&
      GET_CODE(t_insn) == INSN &&
      GET_CODE(PATTERN(t_insn)) == SET &&
      GET_CODE(SET_DEST(PATTERN(t_insn))) == REG &&
      GET_CODE(SET_SRC(PATTERN(t_insn))) == REG &&
      REGNO(SET_DEST(PATTERN(t_insn))) >= FIRST_PSEUDO_REGISTER &&
      REGNO(SET_SRC(PATTERN(t_insn))) < FIRST_PSEUDO_REGISTER)
  {
    rtx reg = SET_SRC(PATTERN(t_insn));

    ret_remap.copy_insn = t_insn;
    ret_remap.incoming_rtl  = reg;
    ret_remap.remap_pseudo = SET_DEST(PATTERN(t_insn));
  }
  else
  {
    ret_remap.copy_insn = 0;
    ret_remap.incoming_rtl  = 0;
    ret_remap.remap_pseudo = 0;
  }

  /*
   * search through the function, finding all return sites,
   * and making sure that they match with what the caller expects.
   */
  if (ret_remap.copy_insn != 0)
  {
    for (t_insn = body_insn; t_insn != 0; t_insn = NEXT_INSN(t_insn))
    {
      /* find all the "return" jumps */
      if (GET_CODE(t_insn) == JUMP_INSN &&
          GET_CODE(PATTERN(t_insn)) == SET &&
          SET_DEST(PATTERN(t_insn)) == pc_rtx &&
          GET_CODE(SET_SRC(PATTERN(t_insn))) == LABEL_REF &&
          XEXP(SET_SRC(PATTERN(t_insn)), 0) == ret_label)
      {
        rtx prev_insn;
        int need_set = 1;

        CLEAR_HARD_REG_SET(return_reg_set);

        /* this insn was a return */
        prev_insn = PREV_INSN(t_insn);
        if (prev_insn != 0 &&
            GET_CODE(prev_insn) == INSN &&
            GET_CODE(PATTERN(prev_insn)) == SET &&
            GET_CODE(SET_DEST(PATTERN(prev_insn))) == REG &&
            REGNO(SET_DEST(PATTERN(prev_insn))) ==
              REGNO(ret_remap.incoming_rtl))
        {
          rtx reg = SET_DEST(PATTERN(prev_insn));
          int rgno = REGNO(reg);
          int rgend = rgno + HARD_REGNO_NREGS(rgno, GET_MODE(reg));
          for (; rgno < rgend; rgno++)
            SET_HARD_REG_BIT(return_reg_set, rgno);

          /*
           * this is the return register, make sure the mode of the
           * return is larger than the mode needed, otherwise we need
           * to generate extra code to possibly fix the thing up.
           */
          rgno = REGNO(reg);
          if (HARD_REGNO_NREGS(rgno, GET_MODE(reg)) >=
              HARD_REGNO_NREGS(rgno, GET_MODE(ret_remap.incoming_rtl)))
            need_set = 0;
        }

        /*
         * If the hard register being set by this return isn't as big
         * as that being used, then add some sets to hard registers to keep
         * problems from happening later in the compiler.
         *
         * Generate SETs
         * for any that aren't so there won't be any uninitialized return.
         * which although a user problem can cause live range problems for
         * the hard registers later in the compile. Note that we plan to add
         * any sets immediately prior to the return insn.
         */
        if (need_set)
        {
          rtx reg = ret_remap.incoming_rtl;
          int rgno = REGNO(reg);
          int rgend = rgno + HARD_REGNO_NREGS(rgno, GET_MODE(reg));
          for (; rgno < rgend; rgno++)
          {
            if (!TEST_HARD_REG_BIT(return_reg_set, rgno))
            {
              need_warn = 1;
              emit_insn_before(gen_rtx(SET, VOIDmode,
                                       gen_rtx(REG, word_mode, rgno),
                                       GEN_INT(0)),
                               t_insn);
            }
          }
        }
      }
    }
  }

  if (need_warn)
  {
    char buf[1000];
    sprintf (buf, "%s returns a different type than expected by call in %s",
                  callee_name, caller_name);
    warning (buf);
    return -1;
  }
  return 0;
}

static void
remap_retval(insn)
rtx insn;
{
  rtx t_insn;
  rtx prev_insn;
  int rgno;

  rgno = -1;
  if (ret_remap.incoming_rtl != 0)
    rgno = REGNO(ret_remap.incoming_rtl);

  for (t_insn = insn; t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    /* find all the "return" jumps */
    if (GET_CODE(t_insn) == JUMP_INSN &&
        GET_CODE(PATTERN(t_insn)) == SET &&
        SET_DEST(PATTERN(t_insn)) == pc_rtx &&
        GET_CODE(SET_SRC(PATTERN(t_insn))) == LABEL_REF &&
        XEXP(SET_SRC(PATTERN(t_insn)), 0) == ret_label)
    {
      /* this insn was a return */
      prev_insn = PREV_INSN(t_insn);

      /* rewrite the copy into the hard return register */
      if (prev_insn != 0 &&
          GET_CODE(prev_insn) == INSN &&
          GET_CODE(PATTERN(prev_insn)) == SET &&
          GET_CODE(SET_DEST(PATTERN(prev_insn))) == REG &&
          REGNO(SET_DEST(PATTERN(prev_insn))) < FIRST_PSEUDO_REGISTER)
      {
        if (rgno == -1)
        {
          /* just delete it, the return value isn't used. */
          PUT_CODE(prev_insn, NOTE);
          NOTE_LINE_NUMBER (prev_insn) = NOTE_INSN_DELETED;
          NOTE_SOURCE_FILE (prev_insn) = 0;
        }
        else if (REGNO(SET_DEST(PATTERN(prev_insn))) == rgno &&
          (HARD_REGNO_NREGS(rgno, GET_MODE(SET_DEST(PATTERN(prev_insn)))) >=
           HARD_REGNO_NREGS(rgno, GET_MODE(ret_remap.incoming_rtl))))
        {
          SET_DEST(PATTERN(prev_insn)) =
            pun_rtx(ret_remap.remap_pseudo, GET_MODE(SET_DEST(PATTERN(prev_insn))));
        }
      }
    }
  }

  if (ret_remap.copy_insn != 0)
  {
    t_insn = ret_remap.copy_insn;
    /* delete the copy from hard return register into the pseudo */
    PUT_CODE(t_insn, NOTE);
    NOTE_LINE_NUMBER (t_insn) = NOTE_INSN_DELETED;
    NOTE_SOURCE_FILE (t_insn) = 0;
  }
}

/*
 * void expand_global_inline (rtx call_insn,
 *                            char *inline_info,
 *                            rtx *calls_to_expand,
 *                            int *calls_libcall,
 *                            int *num_to_expand,
 *                            int tot_inlinable,
 *                            char *caller_name,
 *                            char *callee_name);
 *
 * Expand the call represented by the rtl insn pointed to by call_insn
 * using the saved rtl given.
 */
static
void
expand_global_inline (call_insn, inline_info,
                      calls_to_expand, calls_libcall,
                      num_to_expand, tot_inlinable,
                      caller_name, callee_name)
rtx call_insn;
char *inline_info;
rtx *calls_to_expand;
rtx *calls_libcall;
int *num_to_expand;
int tot_inlinable;
char *caller_name;
char *callee_name;
{
  int ret_val = 0;
  unsigned char *buf;
  int bsize;

  int size;
  rtx inln_head;

  int frame_size;

  rtx t_insn;
  rtx last_insn;
  rtx first_insn_call_seq;  /* first instruction in call sequence */
  rtx last_insn_call_seq;   /* last instruction in call sequence */
  int i;
  int call_num;
  unsigned char *inline_vect;
  rtx insn_libcall = 0;

  /*
   * Get the size of the rtl we are unpacking.
   */
  buf   = glob_inln_rtl_buf(inline_info);
  CI_U32_FM_BUF(buf, bsize);
  bsize -= 4;
  buf += 4;
  inline_vect = glob_inln_vect(inline_info);

  /*
   * read the first insn from the buffer to get various sizes,
   * and numbers that are needed to allocate data structures.
   */
  size = unpack_rtx(buf, &inln_head);
  buf += size;
  bsize -= size;

  num_regs = GLOB_INLN_NUM_REGS(inln_head)+1;
  min_lab  = GLOB_INLN_FIRST_LABELNO(inln_head);
  max_lab  = GLOB_INLN_LAST_LABELNO(inln_head);
  num_uid   = GLOB_INLN_NUM_UIDS(inln_head);
  max_sym = GLOB_INLN_NUM_SYMS(inln_head);
  frame_size = GLOB_INLN_FRAME_SIZE(inln_head);

#if 0
  if (frame_size != 0)  /* for now we won't do it */
    return ;
#endif

  /* allocate the pseudo-register mapping table */
  pseudo_reg_map = (rtx *)xmalloc(num_regs * sizeof(rtx));

  /* allocate the label mapping table */
  label_map = (rtx *)xmalloc((max_lab+1)*sizeof(rtx));

  /* allocate insn uid mapping table */
  uid_to_insn_map = (rtx *)xmalloc(num_uid * sizeof(rtx));

  /* allocate the symbol ref mapping table */
  sym_ref_tab = (rtx *)xmalloc(max_sym *sizeof(rtx));

  /*
   * initialize the new pseudo-frame-pointer register.
   * If there are any users of it it will be nonzero after expansion,
   * and then we will generate the necessary code to access it.
   */
  pseudo_frame_reg = 0;

  /* initialize the pseudo-register mapping table. */
  for (i = 0; i < num_regs; i++)
    pseudo_reg_map[i] = 0;  /* entries get generated as we see them */
    
  /* initialize the label mapping table */
  for (i = 0; i < max_lab+1; i++)
    label_map[i] = 0;

  /* initialize the insn uid mapping table */
  for (i = 0; i < num_uid; i++)
    uid_to_insn_map[i] = 0;

  /* initialize the symbol reference mapping table */
  for (i = 0; i < max_sym; i++)
    sym_ref_tab[i] = 0;

  /* make a label to be used to translate jumps into returns */
  /* It will be put into the code stream later. */
  ret_label = gen_label_rtx();

  PREV_INSN(inln_head) = 0;
  last_insn = inln_head;

  /* unpack the insns doing the all the translation that is necessary */
  call_num = 0;
  while (bsize > 0)
  {
    rtx cur_insn;

    orig_asm_operands_vector = 0;
    orig_asm_constraints_vector = 0;

    size = unpack_rtx(buf, &cur_insn);
    buf += size;
    bsize -= size;

    NEXT_INSN(last_insn) = cur_insn;
    PREV_INSN(cur_insn) = last_insn;

    if (GET_RTX_CLASS(GET_CODE(cur_insn)) == 'i')
    {
      INSN_CODE(cur_insn) = -1;

      if (GET_CODE(PATTERN(cur_insn)) != USE &&
          GET_CODE(PATTERN(cur_insn)) != CLOBBER &&
          GET_CODE(PATTERN(cur_insn)) != ADDR_VEC &&
          GET_CODE(PATTERN(cur_insn)) != ADDR_DIFF_VEC &&
          GET_CODE(PATTERN(cur_insn)) != ASM_INPUT &&
          recog_memoized(cur_insn) < 0 &&
          asm_noperands(PATTERN(cur_insn)) < 0)
        fatal("first pass compilation of function %s uses different options than this compilation.", func_being_inlined);

      if (find_reg_note(cur_insn, REG_LIBCALL, 0))
        insn_libcall = cur_insn;
      else if (find_reg_note(cur_insn, REG_RETVAL, 0))
        insn_libcall = 0;

      if (GET_CODE(cur_insn) == CALL_INSN)
      {
        /* check the bit vector to see if the call is supposed to be inlined. */
        if (CI_FDEF_IVECT_VAL(inline_vect, call_num) == CI_FDEF_IVECT_INLINE)
        {
          if (*num_to_expand >= tot_inlinable)
            fatal("compiler error: tot_inlinable < num_to_expand.");
          calls_to_expand[*num_to_expand] = cur_insn;
          calls_libcall[*num_to_expand] = insn_libcall;
          *num_to_expand += 1;
        }
        call_num ++;
      }
    }

    last_insn = cur_insn;
  }

  NEXT_INSN(last_insn) = 0;

  /*
   * make second pass through insns to update all the rtx's that point
   * at insns (beside NEXT_INSN and PREV_INSN).
   */
  last_cc_rtx = 0;
  for (t_insn = inln_head; t_insn != 0; t_insn = NEXT_INSN(t_insn))
    unpack_remap_insns(&t_insn);

  /*
   * now get the any profiling information that may be necessary for this
   * file from the profile junk.
   */
  {
    unsigned char *prof_info;
    unsigned char *fname;
    int prof_off;

    prof_info = glob_inln_prof_vect(inline_info);
    fname = prof_info;
    prof_info += strlen((char *)fname) + 1;
    CI_U32_FM_BUF(prof_info, prof_off);
    imstg_prof_annotate_inline (NEXT_INSN(inln_head), fname, prof_off,
                                INSN_EXPECT(call_insn),inline_info);
  }

  /* put return label into insn stream here */
  PUT_NEW_UID(ret_label);
  NEXT_INSN(last_insn) = ret_label;
  PREV_INSN(ret_label) = last_insn;
  NEXT_INSN(ret_label) = 0;
  last_insn = ret_label;

  /*
   * for any symbol refs that may have been unpacked,
   * set their SYM_ADDR_TAKEN fields correctly.  We must do this here, not
   * in the unpacking because otherwise we could cause the packed rtl buffer
   * to get lost by the glob db manager.
   */
  for (i = 0; i < max_sym; i++)
  {
    rtx sym_rtx;

    /* pick addr taken flag out of global data base. */
    sym_rtx = sym_ref_tab[i];
    if (sym_rtx != 0)
    {
      int sym_addr_taken;

      sym_addr_taken = glob_addr_taken_p(XSTR(sym_rtx,0));
      if (sym_addr_taken != -1)
        SYM_ADDR_TAKEN_P(sym_rtx) = sym_addr_taken;
    }
  }


  /*
   * Change the call_insn to a note insn deleted.
   */
  PUT_CODE(call_insn, NOTE);
  NOTE_LINE_NUMBER (call_insn) = NOTE_INSN_DELETED;
  NOTE_SOURCE_FILE (call_insn) = 0;

  /*
   * Now delete any use instructions that may exist immediately before
   * the call_insn.
   */
  for (t_insn = PREV_INSN(call_insn); t_insn != 0; t_insn = PREV_INSN(t_insn))
  {
    if (GET_CODE(t_insn) == INSN && GET_CODE(PATTERN(t_insn)) == USE)
    {
      /* delete it. */
      PUT_CODE(t_insn, NOTE);
      NOTE_LINE_NUMBER (t_insn) = NOTE_INSN_DELETED;
      NOTE_SOURCE_FILE (t_insn) = 0;
    }
    else
      break;
  }

  /*
   * Now delete any use instructions that may exist immediately after
   * the call_insn.
   */
  for (t_insn = NEXT_INSN(call_insn); t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    if (GET_CODE(t_insn) == INSN && GET_CODE(PATTERN(t_insn)) == USE)
    {
      /* delete it. */
      PUT_CODE(t_insn, NOTE);
      NOTE_LINE_NUMBER (t_insn) = NOTE_INSN_DELETED;
      NOTE_SOURCE_FILE (t_insn) = 0;
    }
    else
      break;
  }

  {
    int n_remaps;
    int ret_map_val;

    /* remap parameter transmission register assignments */
    n_remaps = find_parm_remaps(NEXT_INSN(inln_head), call_insn,
                                caller_name, callee_name);
    ret_map_val = find_return_remap(NEXT_INSN(inln_head), call_insn,
                                    caller_name, callee_name);

    if (n_remaps >= 0 && ret_map_val >= 0)
    {
      remap_parms(n_remaps, call_insn);
      remap_retval(NEXT_INSN(inln_head));
    }
  }

  /*
   * Now simply add the translated inline insns after the call_insn.
   * Remember that by now the call_insn is really just a NOTE_INSN_DELETED.
   */
  t_insn = NEXT_INSN(inln_head);
  NEXT_INSN(inln_head) = 0;
  PREV_INSN(t_insn) = 0;
  reorder_insns(t_insn, last_insn, call_insn);

  if (pseudo_frame_reg != 0)
  {
    SETUP_INLINE_PSEUDO_FRAME(pseudo_frame_reg, frame_size, t_insn);
    CLEANUPUP_INLINE_PSEUDO_FRAME(pseudo_frame_reg, frame_size, last_insn);
  }

  /* reset globals */
  free(sym_ref_tab);
  sym_ref_tab = 0;
  pseudo_frame_reg = 0;
  free(label_map);
  label_map = 0;
  min_lab = 0;
  max_lab = 0;
  free (pseudo_reg_map);
  pseudo_reg_map = 0;
  num_regs = 0;
  free(uid_to_insn_map);
  uid_to_insn_map = 0;
  num_uid = 0;
  ret_label = 0;
}

static
rtx find_call(x)
rtx x;
{
  enum rtx_code c;
  register char* fmt;
  register int i;
  rtx ret;

  if (x == 0)
    return 0;

  if ((c = GET_CODE(x)) == CALL)
    return x;

  fmt = GET_RTX_FORMAT (c);
  i   = GET_RTX_LENGTH (c);

  while (--i >= 0)
  {
    if (fmt[i] == 'e')
    {
      if ((ret = find_call(XEXP(x,i))) != 0)
        return ret;
    }
    else if (fmt[i] == 'E')
    {
      register int j = XVECLEN(x,i);
      while (--j >= 0)
      {
        if ((ret = find_call(XVECEXP(x,i,j))) != 0)
          return ret;
      }
    }
  }
  return 0;
}

char *
get_call_name(insn_pat)
rtx insn_pat;
{
  rtx call_rtx = find_call(insn_pat);
  rtx tx;

  if (call_rtx == 0)
    return 0;

  tx = XEXP(call_rtx, 0);

  if (GET_CODE(tx) != MEM)
    return 0;

  tx = XEXP(tx, 0);

  if (GET_CODE(tx) != SYMBOL_REF)
    return 0;

  return XSTR(tx,0);
}

static void
lose_libcall_notes (insn)
rtx insn;
{
  rtx prev_note;
  rtx rg_note;
  rtx retval_insn;

  for (prev_note = 0, rg_note = REG_NOTES(insn); rg_note != 0;
       prev_note = rg_note, rg_note = XEXP(rg_note, 1))
  {
    if (REG_NOTE_KIND(rg_note) == REG_LIBCALL)
    {
      if ((retval_insn = XEXP(rg_note, 0)) == 0)
        abort();

      /* delete it. */
      if (prev_note == 0)
        REG_NOTES(insn) = XEXP(rg_note, 1);
      else
        XEXP(prev_note, 1) = XEXP(rg_note, 1);

      break;
    }
  }

  for (prev_note = 0, rg_note = REG_NOTES(retval_insn); rg_note != 0;
       prev_note = rg_note, rg_note = XEXP(rg_note, 1))
  {
    if (REG_NOTE_KIND(rg_note) == REG_RETVAL)
    {
      if (XEXP(rg_note, 0) != insn)
        abort();

      /* delete it. */
      if (prev_note == 0)
        REG_NOTES(retval_insn) = XEXP(rg_note, 1);
      else
        XEXP(prev_note, 1) = XEXP(rg_note, 1);

      break;
    }
  }
}

int
do_glob_inlines(insns, cur_func_name)
rtx insns;
char *cur_func_name;
{
  rtx t_insn;
  char * inline_info;

  rtx *calls_to_expand;
  rtx *calls_libcall;
  int num_to_expand;
  int cur_expanding;
  int call_num;
  int tot_inlinable;
  unsigned char *inline_vect;
  rtx insn_libcall;

  inline_info = glob_inln_info(cur_func_name);

  if (inline_info == 0)
    return 0;

  if (glob_inln_deletable(inline_info))
    /* let caller know not to further compile this routine. */
    return -1;

  if ((tot_inlinable = glob_inln_tot_inline(inline_info)) == 0)
    return 0;

  inline_vect = glob_inln_vect(inline_info);

  /* allocate space for the inline decision table */
  calls_to_expand = (rtx *)xmalloc(tot_inlinable * sizeof(rtx));
  calls_libcall = (rtx *)xmalloc(tot_inlinable * sizeof(rtx));

  /* go through and record the insns of all calls to be expanded. */
  call_num = 0;
  num_to_expand = 0;
  insn_libcall = 0;
  for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    enum rtx_code code;
    code = GET_CODE(t_insn);

    if (GET_RTX_CLASS(code) == 'i')
    {
      if (find_reg_note(t_insn, REG_LIBCALL, 0))
        insn_libcall = t_insn;
      else if (find_reg_note(t_insn, REG_RETVAL, 0))
        insn_libcall = 0;

      if (GET_CODE(t_insn) == CALL_INSN)
      {
        /* check the bit vector to see if the call is supposed to be inlined. */
        if (CI_FDEF_IVECT_VAL(inline_vect, call_num) != CI_FDEF_IVECT_NOTHING)
        {
          if (num_to_expand >= tot_inlinable)
            fatal("compiler error: tot_inlinable < num_to_expand.");
          calls_to_expand[num_to_expand] = t_insn;
          calls_libcall[num_to_expand] = insn_libcall;
          num_to_expand++;
        }
        call_num ++;
      }
    }
  }

  for (cur_expanding = 0; cur_expanding < num_to_expand; cur_expanding++)
  {
    char *name;
    t_insn = calls_to_expand[cur_expanding];

    if (((name = get_call_name(PATTERN(t_insn))) != 0) &&
        ((inline_info = glob_inln_info(name)) != 0))
    {
      func_being_inlined = name;
      if (calls_libcall[cur_expanding] != 0)
        lose_libcall_notes(calls_libcall[cur_expanding]);

      expand_global_inline(t_insn, inline_info,
                           calls_to_expand, calls_libcall,
                           &num_to_expand, tot_inlinable,
                           cur_func_name, func_being_inlined);
      func_being_inlined = 0;
    }
  }

  if (num_to_expand != tot_inlinable)
    fatal("compiler error in glob_inl, tot_inlinable != num_inlined.\n");

  free (calls_to_expand);
  free (calls_libcall);
  return 0;
}

static
int
pack_string(buf, st)
  unsigned char *buf;
  char *st;
{
  unsigned int slen;

  if (st == 0)
  {
    if (buf != 0)
      *buf++ = PK_NULL;
    return 1;
  }

  slen = strlen(st);
  if (buf != 0)
  {
    *buf++ = PK_NON_NULL;
    bcopy(st,buf,slen+1);  /* remember to copy the null */
  }

  /* return number of bytes that were copied */
  return (slen+2);
}

static
int
pack_type(buf, tree_p)
  unsigned char *buf;
  tree tree_p;
{
  int type_val = 0;

  if (buf != 0)
  {
    again:

    if (tree_p == 0)
    {
      if ((type_val & PK_TYPE_PTR) != 0)
        type_val |= PK_TYPE_SCHAR;
      else
        type_val = PK_TYPE_NULL;
    }
    else
    {
      switch (TREE_CODE(tree_p))
      {
        case INTEGER_TYPE:
          if (TYPE_PRECISION(tree_p) ==
              TYPE_PRECISION(char_type_node))
          {
            if (TREE_UNSIGNED(tree_p))
              type_val |= PK_TYPE_UCHAR;
            else
              type_val |= PK_TYPE_SCHAR;
          }
          else if (TYPE_PRECISION(tree_p) ==
                   TYPE_PRECISION(short_integer_type_node))
          {
            if (TREE_UNSIGNED(tree_p))
              type_val |= PK_TYPE_USHORT;
            else
              type_val |= PK_TYPE_SSHORT;
          }
          else if (TYPE_PRECISION(tree_p) ==
                   TYPE_PRECISION(long_integer_type_node))
          {
            if (TREE_UNSIGNED(tree_p))
              type_val |= PK_TYPE_ULONG;
            else
              type_val |= PK_TYPE_SLONG;
          }
          else
          {
            if (TREE_UNSIGNED(tree_p))
              type_val |= PK_TYPE_ULLONG;
            else
              type_val |= PK_TYPE_SLLONG;
          }
          break;

        case FUNCTION_TYPE:
          type_val |= PK_TYPE_FUNCTION;
          break;

        case REAL_TYPE:
          if (TYPE_PRECISION(tree_p) == TYPE_PRECISION(float_type_node))
            type_val |= PK_TYPE_FLOAT;
          else if (TYPE_PRECISION(tree_p) == TYPE_PRECISION(double_type_node))
            type_val |= PK_TYPE_DOUBLE;
          else
            type_val |= PK_TYPE_LDOUBLE;
          break;

        case ARRAY_TYPE:
          {
            tree_p = TREE_TYPE(tree_p);
            /* look over the base type */
            goto again;
          }
          break;

        case POINTER_TYPE:
          if ((type_val & PK_TYPE_PTR) != 0)
            type_val |= PK_TYPE_SCHAR;
          else
          {
            tree_p = TREE_TYPE(tree_p);
            type_val = PK_TYPE_PTR;
            /* look over the base type */
            goto again;
          }
          break;

        case RECORD_TYPE:
        case UNION_TYPE:
          /*
           * Treat unions and structs as 'char', unless they contain
           * pointers.  For overlap purposes, this is safe, because
           * char overlaps everything else.  For points-to, this is
           * an improvement, because this expression value really
           * cannot point at anything.
           */
          if ((type_val & PK_TYPE_PTR) == 0)
            if (!type_could_point_to (tree_p, void_type_node))
            {
              type_val = PK_TYPE_SCHAR;
              break;
            }
          /* Fall thru if pointer or contains pointer */

        case VOID_TYPE:
        default:
          /*
           * If it was a pointer to a type we cannot do, then make it pointer
           * to char otherwise make it null.
           */
          if ((type_val & PK_TYPE_PTR) != 0)
            type_val |= PK_TYPE_SCHAR;
          else
            type_val = PK_TYPE_NULL;
          break;
      }
    }

    *buf = type_val;
  }

  return 1;
}

static
int
pack_rtvec (buf, in_rtvec)
  unsigned char *buf;
  rtvec in_rtvec;
{
  int tot_size;
  int j;

  if (in_rtvec == NULL)
  {
    if (buf != 0)
      *buf++ = PK_NULL;
    return 1;
  }

  if (in_asm_operands)
  {
    if (in_rtvec == orig_asm_operands_vector)
    {
      if (buf != 0)
        *buf++ = PK_ASM_OPS_RTX;
      return 1;
    }
    else if (in_rtvec == orig_asm_constraints_vector)
    {
      if (buf != 0)
        *buf++ = PK_ASM_CNSTS_RTX;
      return 1;
    }
  }

  /* put out the vector length */
  if (buf != 0)
  {
    *buf++ = PK_NON_NULL;
    /* put out the number of elements */
    CI_U32_TO_BUF(buf, in_rtvec->num_elem);
    buf += 4;
  }
  tot_size = 5;

  for (j = 0; j < in_rtvec->num_elem; j++)
  {
    int size;

    size = pack_rtx(buf, in_rtvec->elem[j].rtx);
    tot_size += size;
    if (buf != 0)
      buf += size;
  }

  return (tot_size);
}

static
int
pack_rtx (buf, in_rtx)
  unsigned char *buf;
  register rtx in_rtx;
{
  register int i;
  register char *format_ptr;
  register int  format_length;
  register int  format_start;
  register int tot_size;
  int size;

  if (in_rtx == 0)
  {
    if (buf != 0)
      *buf++ = PK_NULL;
    return 1;
  }

  if (inline_translate_mode == INLN_DEL_CALLEE)
    return 0;

  switch (GET_CODE(in_rtx))
  {
    case PC:
      if (buf != 0)
        *buf = PK_PC_RTX;
      return 1;

    case CC0:
      if (buf != 0)
        *buf = PK_CC0_RTX;
      return 1;

    case CONST_INT:
      if (INTVAL(in_rtx) == 0)
      {
        if (buf != 0)
          *buf = PK_CNST0_RTX;
        return 1;
      }
      if (INTVAL(in_rtx) == 1)
      {
        if (buf != 0)
          *buf = PK_CNST1_RTX;
        return 1;
      }
      if (buf != 0)
      {
        *buf++ = PK_CNST_RTX;
        CI_U32_TO_BUF(buf, INTVAL(in_rtx));
        buf += 4;
      }
      return 5;

    case CONST_DOUBLE:
      if (GET_MODE_CLASS(GET_MODE(in_rtx)) != MODE_FLOAT)
      {
        if (buf != 0)
        {
          *buf++ = PK_CNST2_RTX;
          *buf++ = (int)GET_MODE(in_rtx);
          CI_U32_TO_BUF(buf, CONST_DOUBLE_LOW(in_rtx));
          buf += 4;
          CI_U32_TO_BUF(buf, CONST_DOUBLE_HIGH(in_rtx));
          buf += 4;
        }
        return 10;
      }
      else if (GET_MODE(in_rtx)==SFmode && IS_CONST_ZERO_RTX(in_rtx))
      {
        if (buf != 0)
          *buf = PK_FCNST0_RTX;
        return 1;
      }
      if (GET_MODE(in_rtx)==DFmode && IS_CONST_ZERO_RTX(in_rtx))
      {
        if (buf != 0)
          *buf = PK_DCNST0_RTX;
        return 1;
      }
      if (GET_MODE(in_rtx)==TFmode && IS_CONST_ZERO_RTX(in_rtx))
      {
        if (buf != 0)
          *buf = PK_TCNST0_RTX;
        return 1;
      }
      else
      {
        REAL_VALUE_TYPE f;
        if (buf != 0)
        { int i;

          if (GET_MODE(in_rtx) == SFmode)
            *buf++ = PK_FCNST_RTX;
          else if (GET_MODE(in_rtx) == DFmode)
            *buf++ = PK_DCNST_RTX;
          else
            *buf++ = PK_TCNST_RTX;

          REAL_VALUE_FROM_CONST_DOUBLE (f, in_rtx);

          /* HOST_FLOAT_WORDS_BIG_ENDIAN should be defined 0 in tm.h */
          assert (HOST_FLOAT_WORDS_BIG_ENDIAN == 0);

          for (i=0; i < sizeof(f.r)/sizeof(f.r[0]); i++)
          { unsigned long val = f.r[i];
#ifdef HOST_WORDS_BIG_ENDIAN
            /* real.c works on array of unsigned shorts.  Make the format
               in the db be consistent between BE and LE hosts. */
            val = (val << 16) | (val >> 16);
#endif
            CI_U32_TO_BUF (buf, val);
            buf += 4;
          }
        }
        return sizeof (f.r)+1;
      }

    case SYMBOL_REF:
      {
        int sym_id = RTX_ID(in_rtx);
        int typ_bytes;

        if (sym_pck_tab[sym_id] == 0)
        {
          /* note that it has been packed */
          sym_pck_tab[sym_id] = 1;

          /* generate the symbol def record */
          if (buf != 0)
          {
            unsigned int ubit32;
  
            *buf++ = PK_SYM_DEF_RTX;
  
            /* pack the header part of the rtx structure into a 32 bit int */
            ubit32 = 0;
            /* code is used to note the symbol ref number assigned */
            PK_RTX_CODE_CHEAT(sym_id, ubit32);
            PK_RTX_MODE(in_rtx->mode, ubit32);
            PK_RTX_REPORT_UNDEF(in_rtx->report_undef, ubit32);
            PK_RTX_REPORT_UNUSED(INSN_REPORT_UNUSED(in_rtx), ubit32);
            PK_RTX_UNCG(in_rtx->unchanging, ubit32);
            PK_RTX_VOL(in_rtx->volatil, ubit32);
            PK_RTX_INST(in_rtx->in_struct, ubit32);
            PK_RTX_USED(0, ubit32);  /* always set the used bit to 0 */
            PK_RTX_INTG(in_rtx->integrated, ubit32);
  
            /* now put the 32 bit int in the buffer */
            CI_U32_TO_BUF(buf,ubit32);
            buf += 4;
          }

          /* pack the symbol's type into the buffer */
          typ_bytes = pack_type(buf, RTX_TYPE(in_rtx));
          if (buf != 0)
            buf += typ_bytes;

          /*
           * pack the symbol's size into the data base, and
           * SYMREF_ETC field into the data base.
           */
          if (buf != 0)
          {
            unsigned int ubit32 = SYMREF_SIZE(in_rtx);
            CI_U32_TO_BUF(buf, ubit32);
            buf += 4;

            ubit32 = (SYMREF_ETC(in_rtx) & ~SYMREF_DEFINED);
            CI_U32_TO_BUF(buf, ubit32);
            buf += 4;
          }

          /* pack the symbol string into the buffer */
          return (5 + 8 + typ_bytes + pack_string(buf, XSTR(in_rtx,0)));
        }

        if (buf != 0)
        {
          *buf++ = PK_SYM_REF_RTX;
          CI_U32_TO_BUF(buf, sym_id);
          buf += 4;
        }
      }
      return 5;

    case MEM:
      {
        int typ_bytes;
        unsigned long ubit32 = 0;

        if (buf != 0)
        {
          *buf++ = PK_MEM_RTX;

          PK_RTX_CODE((int)MEM, ubit32);
          PK_RTX_MODE(in_rtx->mode, ubit32);
          PK_RTX_REPORT_UNDEF(in_rtx->report_undef, ubit32);
          PK_RTX_REPORT_UNUSED(INSN_REPORT_UNUSED(in_rtx), ubit32);
          PK_RTX_UNCG(in_rtx->unchanging, ubit32);
          PK_RTX_VOL(in_rtx->volatil, ubit32);
          PK_RTX_INST(in_rtx->in_struct, ubit32);
          PK_RTX_USED(0, ubit32);  /* always set the used bit to 0 */
          PK_RTX_INTG(in_rtx->integrated, ubit32);
          CI_U32_TO_BUF(buf, ubit32);
          buf += 4;
        }

#ifdef TMC_DEBUG
        if (RTX_TYPE(in_rtx)==0)
          warning ("mem rtx %x has no type during global inlining", in_rtx);
#endif
        typ_bytes = pack_type(buf, RTX_TYPE(in_rtx));
        if (buf != 0)
          buf += typ_bytes;

        format_start = 0;
        tot_size = 5 + typ_bytes;
      }
      break;

    case RETURN:
       if (buf != 0)
         *buf++ = PK_RETURN_RTX;
       return 1;

    case REG:
      {
        unsigned int ubit32 = 0;
        unsigned char pck_reg_op;

        if (REGNO(in_rtx) == REGNO(frame_pointer_rtx))
        {
          if (buf != 0)
            *buf++ = PK_FP_REG_RTX;
          return 1;
        }
  
        if (REGNO(in_rtx) < FIRST_PSEUDO_REGISTER)
          pck_reg_op = PK_HARD_REG_RTX;
        else
        {
          if (pseudo_reg_tab[REGNO(in_rtx)] != 0)
          {
            if (buf != 0)
            {
              *buf++ = PK_PSEUDO_REG_REF_RTX;
              CI_U16_TO_BUF(buf, REGNO(in_rtx));
            }
            return 3;
          }
          else
          {
            pck_reg_op = PK_PSEUDO_REG_DEF_RTX;
            pseudo_reg_tab[REGNO(in_rtx)] = 1;
          }
        }
        
        if (buf != 0)
        {
          /*
           * cheat and use the code part of the rtx packing representation to
           * house the register number.
           */
          *buf ++ = pck_reg_op;
          PK_RTX_CODE_CHEAT(REGNO(in_rtx), ubit32);
          PK_RTX_MODE(in_rtx->mode, ubit32);
          PK_RTX_REPORT_UNDEF(in_rtx->report_undef, ubit32);
          PK_RTX_REPORT_UNUSED(INSN_REPORT_UNUSED(in_rtx), ubit32);
          PK_RTX_UNCG(in_rtx->unchanging, ubit32);
          PK_RTX_VOL(in_rtx->volatil, ubit32);
          PK_RTX_INST(in_rtx->in_struct, ubit32);
          PK_RTX_USED(0, ubit32);  /* always set the used bit to 0 */
          PK_RTX_INTG(in_rtx->integrated, ubit32);
          CI_U32_TO_BUF(buf, ubit32);
          buf += 4;
        }

        return 5 + pack_type(buf, RTX_TYPE(in_rtx));
      }

    case ASM_OPERANDS:
      in_asm_operands = 1;
      goto do_normal_way;

    case LABEL_REF:
      if (buf != 0)
      {
        *buf++ = PK_LABEL_REF_RTX;
        CI_U32_TO_BUF(buf, CODE_LABEL_NUMBER(XEXP(in_rtx, 0)));
        buf += 4;
      }
      return 5;

    case CODE_LABEL:
      if (buf != 0)
      {
        *buf++ = PK_CODE_LABEL_RTX;
        CI_U32_TO_BUF(buf, CODE_LABEL_NUMBER(in_rtx));
        buf += 4;
        CI_U32_TO_BUF(buf, INSN_UID(in_rtx));
        buf += 4;
        CI_U32_TO_BUF(buf, INSN_PROF_DATA_INDEX(in_rtx));
        buf += 4;
      }
      return 13;

    case NOTE:
      /* 
       * delete all line number notes, these are ones with line numbers > 0.
       * Also delete NOTE_INSN_DELETED, NOTE_INSN_FUNCTION_BEG, 
       * NOTE_INSN_BLOCK_BEG, NOTE_INSN_BLOCK_END, NOTE_INSN_FUNCTION_END,
       * NOTE_INSN_FUNCTION_ENTRY, and NOTE_INSN_PARMS_HOMED notes.
       */
      if (NOTE_LINE_NUMBER(in_rtx) > 0 ||
          NOTE_LINE_NUMBER(in_rtx) == NOTE_INSN_DELETED ||
          NOTE_LINE_NUMBER(in_rtx) == NOTE_INSN_BLOCK_BEG ||
          NOTE_LINE_NUMBER(in_rtx) == NOTE_INSN_BLOCK_END ||
#ifndef GCC20
          NOTE_LINE_NUMBER(in_rtx) == NOTE_INSN_PARMS_HOMED ||
#endif
          NOTE_LINE_NUMBER(in_rtx) == NOTE_INSN_FUNCTION_BEG ||
          NOTE_LINE_NUMBER(in_rtx) == NOTE_INSN_FUNCTION_END ||
#if !defined(DEV_465)
          NOTE_LINE_NUMBER(in_rtx) == NOTE_INSN_FUNCTION_ENTRY ||
#endif
          NOTE_LINE_NUMBER(in_rtx) == NOTE_INSN_PROLOGUE_END ||
          NOTE_LINE_NUMBER(in_rtx) == NOTE_INSN_EPILOGUE_BEG)
        return 0;
      /* Fall Through */

    case INSN:
    case JUMP_INSN:
    case CALL_INSN:
      /*
       * If this instruction is part of the profile instrumentation
       * don't save it for global inlining since this would cause
       * the inlined code in the second pass to have profile instrumentation.
       */
      if (INSN_PROFILE_INSTR_P(in_rtx))
        return 0;
      /* Fall-thru */
    case BARRIER:
    /* CODE_LABEL already taken care of */
    /* NOTE already taken care of */
    case INLINE_HEADER:
    case GLOBAL_INLINE_HEADER:
      /* cheat so we don't store the NEXT_INSN & PREV_INSN fields,
       * these get rebuilt by context */
      if (buf != 0)
      {
        unsigned int ubit32;
  
        *buf++ = PK_INSN_LEVEL_RTX;
  
        /* pack the header part of the rtx structure into a 32 bit int */
        ubit32 = 0;
        PK_RTX_CODE(in_rtx->code, ubit32);
        PK_RTX_MODE(in_rtx->mode, ubit32);
        PK_RTX_REPORT_UNDEF(in_rtx->report_undef, ubit32);
        PK_RTX_REPORT_UNUSED(INSN_REPORT_UNUSED(in_rtx), ubit32);
        PK_RTX_UNCG(in_rtx->unchanging, ubit32);
        PK_RTX_VOL(in_rtx->volatil, ubit32);
        PK_RTX_INST(in_rtx->in_struct, ubit32);
        PK_RTX_USED(0, ubit32);  /* always set the used bit to 0 */
        PK_RTX_INTG(in_rtx->integrated, ubit32);
  
        /* now put the 32 bit int in the buffer */
        CI_U32_TO_BUF(buf,ubit32);
        buf += 4;
        /* pack the INSN_UID into the buffer */
        CI_U32_TO_BUF(buf, INSN_UID(in_rtx));
        buf += 4;
      }
      tot_size = 9;
      /* skip over the first three fields in these rtxs */
      format_start = 3;
      break;

    default:
      do_normal_way:

      if (buf != 0)
      {
        unsigned int ubit32;
  
        *buf++ = PK_NON_NULL;
        /* pack the header part of the rtx structure into a 32 bit int */
        ubit32 = 0;
        PK_RTX_CODE(in_rtx->code, ubit32);
        PK_RTX_MODE(in_rtx->mode, ubit32);
        PK_RTX_REPORT_UNDEF(in_rtx->report_undef, ubit32);
        PK_RTX_REPORT_UNUSED(INSN_REPORT_UNUSED(in_rtx), ubit32);
        PK_RTX_UNCG(in_rtx->unchanging, ubit32);
        PK_RTX_VOL(in_rtx->volatil, ubit32);
        PK_RTX_INST(in_rtx->in_struct, ubit32);
        PK_RTX_USED(0, ubit32);  /* always set the used bit to 0 */
        PK_RTX_INTG(in_rtx->integrated, ubit32);
    
        /* now put the 32 bit int in the buffer */
        CI_U32_TO_BUF(buf,ubit32);
        buf += 4;
      }
      tot_size = 5;
      format_start = 0;
      break;
  }

  format_ptr = GET_RTX_FORMAT (GET_CODE (in_rtx));
  format_length = GET_RTX_LENGTH (GET_CODE (in_rtx));

  format_ptr += format_start;

  for (i = format_start; i < format_length; i++)
  {
    switch (*format_ptr++)
    {
      case 'S':
      case 's':
        size = pack_string(buf, XSTR(in_rtx, i));
        tot_size += size;
        if (buf != 0)
          buf += size;
	break;

	/* 0 indicates a field for internal use that does not
           need to be saved */
      case '0':
	break;

      case 'e':
        size = pack_rtx (buf, XEXP (in_rtx, i));
        tot_size += size;
        if (buf != 0)
          buf += size;
	break;

      case 'E':
        size = pack_rtvec(buf, XVEC (in_rtx, i));
        tot_size += size;
        if (buf != 0)
          buf += size;
	break;

      case 'n':
      case 'i':
        /* no extra storage needed for these fields */
        if (buf != 0)
        {
          unsigned int ubit32;
          /* just put the field out into the area pointed to by tbuf */
          ubit32 = XINT(in_rtx, i);
          CI_U32_TO_BUF(buf, ubit32);
          buf += 4;
        }
        tot_size += 4;
	break;

      case 'u':
        /* no extra storage is needed for these fields */
        if (buf != 0)
        {
          unsigned int ubit32;

	  if (XEXP (in_rtx, i) != NULL)
	    ubit32 = INSN_UID (XEXP (in_rtx, i));
	  else
            ubit32 = 0xFFFFFFFF;
          CI_U32_TO_BUF(buf,ubit32);
          buf += 4;
        }
        tot_size += 4;
	break;

      default:
	fprintf (stderr,
		 "switch format wrong in pack-rtl.pack_rtx(). format was: %c.\n",
		 format_ptr[-1]);
	abort ();
    }
  }

  if (GET_CODE(in_rtx) == ASM_OPERANDS && orig_asm_operands_vector == 0)
  {
    orig_asm_operands_vector = XVEC (in_rtx, 3);
    orig_asm_constraints_vector = XVEC (in_rtx, 4);
  }

  in_asm_operands = 0;

  return(tot_size);
}

/*
 * returns the number of bytes that were packed into the buffer.
 * or if buf is 0, the number of bytes that would have been packed.
 */
static
int
pack_rtl (buf, rtx_first)
  unsigned char *buf;
  rtx rtx_first;
{
  register rtx tmp_rtx;
  register int tot_size = 0;
  register int size;

  /* comparison order changed to circumvent Macintosh compiler bug */
  for (tmp_rtx = rtx_first;  tmp_rtx != NULL; tmp_rtx = NEXT_INSN (tmp_rtx))
  {
    int insn_code = -1;

    if (GET_RTX_CLASS(GET_CODE(tmp_rtx)) == 'i')
    {
      insn_code = INSN_CODE(tmp_rtx);
      INSN_CODE(tmp_rtx) = -1;
    }

    if (GET_CODE(tmp_rtx) == INSN)
    {
      inline_translate_mode = INSN_INLN_TRANSLATE(tmp_rtx);
      /*
       * change all the translation modes that are based are translations
       * if this is the callee routine to be normal, since we are making the
       * translations happen as we are packing the rtl.
       */
      if (inline_translate_mode == INLN_DEL_CALLEE ||
          inline_translate_mode == INLN_PARAM_CALLEE ||
          inline_translate_mode == INLN_RET_CALLEE)
        INSN_INLN_TRANSLATE(tmp_rtx) = INLN_NORMAL;
    }
    else
      inline_translate_mode = INLN_NORMAL;

    orig_asm_operands_vector = 0;
    orig_asm_constraints_vector = 0;

    size = pack_rtx(buf, tmp_rtx);
    tot_size += size;
    if (buf != 0)
      buf += size;

    if (GET_CODE(tmp_rtx) == INSN)
    {
      /* change the translation mode back to what it was. */
      INSN_INLN_TRANSLATE(tmp_rtx) = inline_translate_mode;
    }

    if (insn_code != -1)
      INSN_CODE(tmp_rtx) = insn_code;
  }

  return (tot_size);
}

static
int
unpack_string(buf, st)
  unsigned char *buf;
  char **st;
{
  unsigned int slen;

  if (*buf++ == PK_NULL)
  {
    *st = 0;
    return 1;
  }

  slen = strlen((char *)buf);
  *st = obstack_alloc(rtl_obstack, slen+1);
  bcopy(buf, *st, slen+1);  /* remember to copy the null */

  /* return number of bytes that were consumed */
  return (slen+2);
}

static
int
unpack_type (buf, out_type)
  unsigned char *buf;
  tree *out_type;
{
  int val = *buf;
  int is_ptr = 0;
  tree ret_tree;

  if ((val & PK_TYPE_PTR) != 0)
  {
    is_ptr = 1;
    val &= ~PK_TYPE_PTR;
  }
    
  switch (val)
  {
    case PK_TYPE_SCHAR:
      ret_tree = signed_char_type_node;
      break;
    case PK_TYPE_UCHAR:
      ret_tree = unsigned_char_type_node;
      break;
    case PK_TYPE_SSHORT:
      ret_tree = short_integer_type_node;
      break;
    case PK_TYPE_USHORT:
      ret_tree = short_unsigned_type_node;
      break;
    case PK_TYPE_SLONG:
      ret_tree = long_integer_type_node;
      break;
    case PK_TYPE_ULONG:
      ret_tree = long_unsigned_type_node;
      break;
    case PK_TYPE_SLLONG:
      ret_tree = long_long_integer_type_node;
      break;
    case PK_TYPE_ULLONG:
      ret_tree = long_long_unsigned_type_node;
      break;
    case PK_TYPE_FLOAT:
      ret_tree = float_type_node;
      break;
    case PK_TYPE_DOUBLE:
      ret_tree = double_type_node;
      break;
    case PK_TYPE_LDOUBLE:
      ret_tree = long_double_type_node;
      break;
    case PK_TYPE_FUNCTION:
      ret_tree = default_function_type;
      break;

    default:
      ret_tree = 0;
      break;
  }

  if (is_ptr && ret_tree != 0)
    ret_tree = build_pointer_type(ret_tree);

  *out_type = ret_tree;
  return 1;
}

static
int
unpack_rtvec (buf, out_rtvec)
  unsigned char *buf;
  rtvec * out_rtvec;
{
  int tot_size;
  int j;
  unsigned int ubit32;
  rtvec ret_rtvec;

  switch (*buf++)
  {
    case PK_NULL:
      *out_rtvec = 0;
      return 1;

    case PK_ASM_OPS_RTX:
      *out_rtvec = orig_asm_operands_vector;
      return 1;

    case PK_ASM_CNSTS_RTX:
      *out_rtvec = orig_asm_constraints_vector;
      return 1;

    default:
      break;
  }

  CI_U32_FM_BUF(buf, ubit32);
  buf += 4;
  tot_size = 5;

  ret_rtvec = rtvec_alloc(ubit32);

  for (j = 0; j < ret_rtvec->num_elem; j++)
  {
    int size;

    size = unpack_rtx(buf, &ret_rtvec->elem[j].rtx);
    tot_size += size;
    buf += size;
  }

  *out_rtvec = ret_rtvec;
  return (tot_size);
}

static
int
unpack_rtx (buf, out_rtx)
  unsigned char *buf;
  register rtx *out_rtx;
{
  register int i;
  register char *format_ptr;
  register int format_length;
  register int format_start;
  register int tot_size;
  register rtx ret_rtx;
  int size;
  unsigned int ubit32;
  unsigned char c;

  /* check for case where its one of the specials */
  switch (c = *buf++)
  {
    case PK_NULL:
      *out_rtx = 0;
      return 1;

    case PK_PC_RTX:
      *out_rtx = pc_rtx;
      return 1;

    case PK_CC0_RTX:
      *out_rtx = cc0_rtx;
      return 1;

    case PK_CNST0_RTX:
      *out_rtx = const0_rtx;
      return 1;

    case PK_CNST1_RTX:
      *out_rtx = const1_rtx;
      return 1;

    case PK_CNST_RTX:
      CI_U32_FM_BUF(buf, ubit32);
      buf += 4;
      *out_rtx = gen_rtx(CONST_INT, VOIDmode, ubit32);
      return 5;

    case PK_FCNST0_RTX:
      *out_rtx = GET_CONST_ZERO_RTX(SFmode);
      return 1;

    case PK_DCNST0_RTX:
      *out_rtx = GET_CONST_ZERO_RTX(DFmode);
      return 1;

    case PK_TCNST0_RTX:
      *out_rtx = GET_CONST_ZERO_RTX(TFmode);
      return 1;

    case PK_FCNST_RTX:
    case PK_DCNST_RTX:
    case PK_TCNST_RTX:
      {
        REAL_VALUE_TYPE f;
        enum machine_mode m;
        int i;

        if (c==PK_FCNST_RTX)
          m = SFmode;
        else if (c==PK_DCNST_RTX)
          m = DFmode;
        else
          m = TFmode;

        /* HOST_FLOAT_WORDS_BIG_ENDIAN should be defined 0 in tm.h */
        assert (HOST_FLOAT_WORDS_BIG_ENDIAN == 0);

        for (i=0; i < sizeof(f.r)/sizeof(f.r[0]); i++)
        { unsigned long val;
          CI_U32_FM_BUF(buf, val);
#ifdef HOST_WORDS_BIG_ENDIAN
            /* real.c actually works on array of unsigned shorts.  Make the
               format in the db be consistent between BE and LE hosts. */
          val = (val << 16) | (val >> 16);
#endif
          f.r[i] = val;
          buf += 4;
        }

        *out_rtx = CONST_DOUBLE_FROM_REAL_VALUE (f, m);
        return sizeof(f.r)+1;
      }

    case PK_CNST2_RTX:
      {
        enum machine_mode mode;
        int lo, hi;

        mode = (enum machine_mode) *buf++;
        CI_U32_FM_BUF(buf, lo);
        buf += 4;
        CI_U32_FM_BUF(buf, hi);
        buf += 4;
        *out_rtx = immed_double_const(lo, hi, mode);
      }
      return 10;

    case PK_SYM_DEF_RTX:
      {
        char *sym_name;
        struct obstack *sv;
        tree sym_type;
        int sym_addr_taken;
        int symref_size;
        int symref_etc;

        CI_U32_FM_BUF(buf,ubit32);
        buf += 4;
        
        /* want this allocated on permanent obstack */
        sv = rtl_obstack;
        rtl_obstack = &permanent_obstack;
        tot_size = 5;

        size = unpack_type(buf, &sym_type);
        buf += size;
        tot_size += size;

        CI_U32_FM_BUF(buf,symref_size);
        buf += 4;
        tot_size += 4;

        CI_U32_FM_BUF(buf,symref_etc);
        buf += 4;
        tot_size += 4;

        size = unpack_string(buf, &sym_name);
        buf += size;
        tot_size += size;

        rtl_obstack = sv;

        ret_rtx = gen_symref_rtx(UNPK_RTX_MODE(ubit32), sym_name);
        sym_ref_tab[UNPK_RTX_CODE_CHEAT(ubit32)] = ret_rtx;

        /*
         * This may lose some information, but it is safe since it
         * is possible that the RTX_TYPE may not still point to valid
         * memory.
         */
        RTX_TYPE(ret_rtx) = sym_type;
        if (SYMREF_SIZE(ret_rtx) < symref_size)
          SYMREF_SIZE(ret_rtx) = symref_size;

        SYMREF_ETC(ret_rtx) |= symref_etc;

        ret_rtx->report_undef = UNPK_RTX_REPORT_UNDEF(ubit32);
        INSN_REPORT_UNUSED(ret_rtx) = UNPK_RTX_REPORT_UNUSED(ubit32);
        ret_rtx->unchanging = UNPK_RTX_UNCG(ubit32);
        ret_rtx->volatil = UNPK_RTX_VOL(ubit32);
        ret_rtx->in_struct = UNPK_RTX_INST(ubit32);
        ret_rtx->used = UNPK_RTX_USED(ubit32);
        ret_rtx->integrated = UNPK_RTX_INTG(ubit32);

        *out_rtx = ret_rtx;
      }
      return tot_size;

    case PK_SYM_REF_RTX:
      CI_U32_FM_BUF(buf, ubit32);
      buf += 4;
      if (ubit32 >= max_sym || sym_ref_tab[ubit32] == 0)
        abort();
      *out_rtx = sym_ref_tab[ubit32];
      return 5;

    case PK_MEM_RTX:
      CI_U32_FM_BUF(buf, ubit32);
      buf += 4;
      ret_rtx = rtx_alloc(MEM);
      PUT_MODE(ret_rtx, UNPK_RTX_MODE(ubit32));
      ret_rtx->report_undef = UNPK_RTX_REPORT_UNDEF(ubit32);
      INSN_REPORT_UNUSED(ret_rtx) = UNPK_RTX_REPORT_UNUSED(ubit32);
      ret_rtx->unchanging = UNPK_RTX_UNCG(ubit32);
      ret_rtx->volatil = UNPK_RTX_VOL(ubit32);
      ret_rtx->in_struct = UNPK_RTX_INST(ubit32);
      ret_rtx->used = UNPK_RTX_USED(ubit32);
      ret_rtx->integrated = UNPK_RTX_INTG(ubit32);
      
      tot_size = 5;
      size = unpack_type(buf, &RTX_TYPE(ret_rtx));
      buf += size;
      tot_size += size;
      format_start = 0;
      break;

    case PK_FP_REG_RTX:
      if (pseudo_frame_reg == 0)
        pseudo_frame_reg = gen_reg_rtx(Pmode);

      *out_rtx = pseudo_frame_reg;
      return 1;

    case PK_PSEUDO_REG_REF_RTX:
      CI_U16_FM_BUF(buf, ubit32);
      if (pseudo_reg_map[ubit32] == 0)
        abort();
      *out_rtx = pseudo_reg_map[ubit32];
      return 3;

    case PK_PSEUDO_REG_DEF_RTX:
      CI_U32_FM_BUF(buf, ubit32);
      buf += 4;
      if (pseudo_reg_map[UNPK_RTX_CODE_CHEAT(ubit32)] != 0)
        abort();

      /* the mapping register isn't allocated yet, allocate it now
       * and fill in the fields */
      ret_rtx = gen_reg_rtx(UNPK_RTX_MODE(ubit32));
      ret_rtx->report_undef = UNPK_RTX_REPORT_UNDEF(ubit32);
      INSN_REPORT_UNUSED(ret_rtx) = UNPK_RTX_REPORT_UNUSED(ubit32);
      ret_rtx->unchanging = UNPK_RTX_UNCG(ubit32);
      ret_rtx->volatil = UNPK_RTX_VOL(ubit32);
      ret_rtx->in_struct = UNPK_RTX_INST(ubit32);
      ret_rtx->used = UNPK_RTX_USED(ubit32);
      ret_rtx->integrated = UNPK_RTX_INTG(ubit32);

      tot_size = 5;
      tot_size += unpack_type(buf, &RTX_TYPE(ret_rtx));

      pseudo_reg_map[UNPK_RTX_CODE_CHEAT(ubit32)] = ret_rtx;
      *out_rtx = ret_rtx;
      return tot_size;

    case PK_HARD_REG_RTX:
      CI_U32_FM_BUF(buf, ubit32);
      buf += 4;
      ret_rtx = rtx_alloc(REG);
      PUT_MODE(ret_rtx, UNPK_RTX_MODE(ubit32));
      ret_rtx->report_undef = UNPK_RTX_REPORT_UNDEF(ubit32);
      INSN_REPORT_UNUSED(ret_rtx) = UNPK_RTX_REPORT_UNUSED(ubit32);
      ret_rtx->unchanging = UNPK_RTX_UNCG(ubit32);
      ret_rtx->volatil = UNPK_RTX_VOL(ubit32);
      ret_rtx->in_struct = UNPK_RTX_INST(ubit32);
      ret_rtx->used = UNPK_RTX_USED(ubit32);
      ret_rtx->integrated = UNPK_RTX_INTG(ubit32);
      REGNO(ret_rtx) = UNPK_RTX_CODE_CHEAT(ubit32);

  
      tot_size = 5;
      tot_size += unpack_type(buf, &RTX_TYPE(ret_rtx));

      if (REGNO(ret_rtx) == STACK_POINTER_REGNUM)
      {
        *out_rtx = stack_pointer_rtx;
        /*
         * must create a pseudo frame, since this is actually referencing
         * within the frame possibly.
         */
        if (pseudo_frame_reg == 0)
          pseudo_frame_reg = gen_reg_rtx(Pmode);
      }
      else
        *out_rtx = ret_rtx;
      return tot_size;

    case PK_RETURN_RTX:
      /*
       * change any return references to jumps to a label at the end
       * of the function.
       */
      *out_rtx = gen_rtx(SET, VOIDmode,
                         pc_rtx, gen_rtx(LABEL_REF, VOIDmode, ret_label));
      return 1;

    case PK_LABEL_REF_RTX:
      CI_U32_FM_BUF(buf, ubit32);
      buf += 4;

      /* if we haven't seen this one yet, generate it */
      if (label_map[ubit32] == 0)
        label_map[ubit32] = gen_label_rtx();

      *out_rtx = gen_rtx(LABEL_REF, VOIDmode, label_map[ubit32]);
      return 5;

    case PK_CODE_LABEL_RTX:
      CI_U32_FM_BUF(buf, ubit32);
      buf += 4;
      ret_rtx = label_map[ubit32];

      /* if we haven't seen this one yet, generate it */
      if (ret_rtx == 0)
        ret_rtx = label_map[ubit32] = gen_label_rtx();

      /*
       * record the insn uid mapping, then get a uid that will be OK
       * in the context of the procedure this is being integrated into.
       */
      CI_U32_FM_BUF(buf, ubit32);
      buf += 4;
      uid_to_insn_map[ubit32] = ret_rtx;
      PUT_NEW_UID(ret_rtx);

      CI_U32_FM_BUF(buf, ubit32);
      buf += 4;
      INSN_PROF_DATA_INDEX(ret_rtx) = ubit32;

      *out_rtx = ret_rtx;
      return 13;

    case PK_INSN_LEVEL_RTX:
      /* same as other rtx's except the 2nd and 3rd fields were not saved. */
      CI_U32_FM_BUF(buf,ubit32);
      buf += 4;

      ret_rtx = rtx_alloc(UNPK_RTX_CODE(ubit32));
      PUT_MODE(ret_rtx, UNPK_RTX_MODE(ubit32));
      ret_rtx->report_undef = UNPK_RTX_REPORT_UNDEF(ubit32);
      INSN_REPORT_UNUSED(ret_rtx) = UNPK_RTX_REPORT_UNUSED(ubit32);
      ret_rtx->unchanging = UNPK_RTX_UNCG(ubit32);
      ret_rtx->volatil = UNPK_RTX_VOL(ubit32);
      ret_rtx->in_struct = UNPK_RTX_INST(ubit32);
      ret_rtx->used = UNPK_RTX_USED(ubit32);
      ret_rtx->integrated = UNPK_RTX_INTG(ubit32);

      /* get INSN_UID field */
      CI_U32_FM_BUF(buf, ubit32);
      buf += 4;
      tot_size = 9;

      /*
       * record the insn uid mapping, then get a uid that will be OK
       * in the context of the procedure this is being integrated into.
       */
      if (uid_to_insn_map != 0)  /* necessary to get first insn */
        uid_to_insn_map[ubit32] = ret_rtx;
      PUT_NEW_UID(ret_rtx);
  
      format_start = 3;
      break;

    default:
    case PK_NON_NULL:
      CI_U32_FM_BUF(buf,ubit32);
      buf += 4;
      tot_size = 5;

      ret_rtx = rtx_alloc(UNPK_RTX_CODE(ubit32));
      PUT_MODE(ret_rtx, UNPK_RTX_MODE(ubit32));
      ret_rtx->report_undef = UNPK_RTX_REPORT_UNDEF(ubit32);
      INSN_REPORT_UNUSED(ret_rtx) = UNPK_RTX_REPORT_UNUSED(ubit32);
      ret_rtx->unchanging = UNPK_RTX_UNCG(ubit32);
      ret_rtx->volatil = UNPK_RTX_VOL(ubit32);
      ret_rtx->in_struct = UNPK_RTX_INST(ubit32);
      ret_rtx->used = UNPK_RTX_USED(ubit32);
      ret_rtx->integrated = UNPK_RTX_INTG(ubit32);
  
      format_start = 0;
      break;
  }

  format_ptr = GET_RTX_FORMAT (GET_CODE (ret_rtx));
  format_length = GET_RTX_LENGTH (GET_CODE (ret_rtx));

  format_ptr += format_start;
  for (i = format_start; i < format_length; i++)
  {
    switch (*format_ptr++)
    {
      case 'S':
      case 's':
        size = unpack_string(buf, &XSTR(ret_rtx, i));
        tot_size += size;
        buf += size;
	break;

	/* 0 indicates a field for internal use that was not saved */
      case '0':
	break;

      case 'e':
        size = unpack_rtx (buf, &XEXP(ret_rtx, i));
        tot_size += size;
        buf += size;
	break;

      case 'E':
        size = unpack_rtvec(buf, &XVEC(ret_rtx, i));
        tot_size += size;
        buf += size;
	break;

      case 'n':
      case 'i':
        /* no extra storage needed for these fields */
        CI_U32_FM_BUF(buf, XINT(ret_rtx, i));
        buf += 4;
        tot_size += 4;
	break;

      case 'u':
        /* no extra storage is needed for these fields */
        CI_U32_FM_BUF(buf, XINT(ret_rtx, i));
        buf += 4;
        tot_size += 4;
	break;

      default:
	fprintf (stderr,
		 "switch format wrong in pack-rtl.unpack_rtx(). format was: %c.\n",
		 format_ptr[-1]);
	abort ();
    }
  }

  if (GET_CODE(ret_rtx) == ASM_OPERANDS && orig_asm_operands_vector == 0)
  {
    orig_asm_operands_vector = XVEC (ret_rtx, 3);
    orig_asm_constraints_vector = XVEC (ret_rtx, 4);
  }

  *out_rtx = ret_rtx;
  return(tot_size);
}

static
void
unpack_remap_insns (x_p)
rtx *x_p;
{
  char *format_ptr;
  int i;
  rtx in_rtx = *x_p;

  format_ptr = GET_RTX_FORMAT (GET_CODE (in_rtx));
  i = 0;

  /*
   * must skip over the PREV_INSN and NEXT_INSN fields, since they
   * have already been done.
   */
  switch (GET_CODE(in_rtx))
  {
    case INSN:
    case JUMP_INSN:
    case CALL_INSN:
    case BARRIER:
    case CODE_LABEL:
    case NOTE:
    case INLINE_HEADER:
    case GLOBAL_INLINE_HEADER:
      format_ptr += 3;
      i += 3;
      break;

#ifndef HAVE_cc0
    case SET:
    case CLOBBER:
      /*
       * this is a major hack to get sharing rules for cc registers that
       * don't use cc0 correct.
       * If this is a set of cc register we record the object, and then
       * we rewrite any and all uses of cc register untill the next SET
       * of a cc register.
       */
      if (GET_CODE(XEXP(in_rtx,0)) == REG &&
          GET_MODE_CLASS(GET_MODE(XEXP(in_rtx,0))) == MODE_CC)
        last_cc_rtx = XEXP(in_rtx,0);
      break;

    case REG:
      if (last_cc_rtx != 0 &&
          GET_MODE_CLASS(GET_MODE(in_rtx)) == MODE_CC)
        *x_p = last_cc_rtx;
      break;
#endif

    case LABEL_REF:
      return;
  }

  for (; i < GET_RTX_LENGTH (GET_CODE (in_rtx)); i++)
  {
    switch (*format_ptr++)
    {
      case 'e':
        if (XEXP(in_rtx,i) != 0)
          unpack_remap_insns(&XEXP(in_rtx,i));
        break;

      case 'E':
        if (XVEC(in_rtx,i) != 0)
        {
          int j;
          for (j = 0; j < XVECLEN(in_rtx,i); j++)
          {
            if (XVECEXP(in_rtx,i,j) != 0)
              unpack_remap_insns(&XVECEXP(in_rtx,i,j));
          }
        }
        break;

      case 'u':
        if (XINT(in_rtx,i) == 0xFFFFFFFF)
          XEXP(in_rtx,i) = 0;
        else
          XEXP(in_rtx,i) = uid_to_insn_map[XINT(in_rtx,i)];
        break;

      default:
        break;
    }
  }
}
#endif
