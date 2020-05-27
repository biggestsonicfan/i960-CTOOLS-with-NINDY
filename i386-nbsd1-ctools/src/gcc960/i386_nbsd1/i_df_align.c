#include "config.h"

#ifdef IMSTG
#include <stdio.h>	/* only necessary for dataflow.h */
#include <setjmp.h>
#include "tree.h"	/* only necessary for dataflow.h */
#include "rtl.h"
#include "regs.h"
#include "flags.h"

#include "basic-block.h"
#include "i_dataflow.h"
#include "i_df_set.h"

#define AL_NC 0  /* this is an otherwise impossible value for align */
#define AL_UNKNOWN 1  /* another impossible encoding for align */

#define MAX_BITS_KNOWN 8
#define BITS_KNOWN(x)  ((((x) >> 8) & 0xFF) - 1)
#define BIT_VAL(x)     ((x) | 0xFFFFFF00)
#define PACK_ALIGN(b_kwn,b_val) (((((b_kwn)+1) & 0xFF) << 8) | \
                                 (((b_val) | (0xFFFFFFFF << b_kwn)) & 0xFF))

#define UDN_FORW_ALIGN(df_univ,ud) ((df_univ)->maps.ud_map[(ud)].ud_forw_align)
#define UDN_FULL_ALIGN(df_univ,ud) ((df_univ)->maps.ud_map[(ud)].ud_align)

static unsigned int compute_def_insn_align();

static unsigned int compute_use_forw_align();

static unsigned int compute_use_align();

static unsigned int compute_def_forw_align();

static unsigned int compute_def_align();

static unsigned int compute_ptr_type_align();


static unsigned int
join_align(al0, al1)
unsigned int al0;
unsigned int al1;
{
  unsigned int ret_known;
  unsigned int b1_known;
  unsigned int b0_val;
  unsigned int b1_val;
  unsigned int mask;
  unsigned int i;

  if (al0 == AL_UNKNOWN)
    return al1;

  if (al1 == AL_UNKNOWN)
    return al0;

  ret_known = BITS_KNOWN(al0);
  b0_val = BIT_VAL(al0);
  b1_known = BITS_KNOWN(al1);
  b1_val = BIT_VAL(al1);

  if (ret_known > b1_known)
    ret_known = b1_known;

  mask = 1;
  i = 0;

  while (i < ret_known)
  {
    if ((b0_val & mask) != (b1_val & mask))
    {
      ret_known = i;
      break;
    }
    i += 1;
    mask <<= 1;
  }

  return PACK_ALIGN(ret_known, b0_val);
}

static unsigned int
compute_rtx_expr_align(insn_id, x_p, use_compute_func)
int insn_id;
rtx *x_p;
unsigned int (*use_compute_func)();
{
  rtx x = *x_p;
  ud_info *df_univ;
  int ud_num;
  unsigned int al0;
  unsigned int al1;
  unsigned int b0_known;
  unsigned int b1_known;
  unsigned int b0_val;
  unsigned int b1_val;

  switch (GET_CODE(x))
  {
    case REG:
      /*
       * Both the frame pointer and stack pointer are guaranteed to be
       * 16 byte aligned on the i964.
       */
      if (x == frame_pointer_rtx || x == stack_pointer_rtx)
        return PACK_ALIGN(4, 0);

#ifdef HAVE_PID
      if (pid_flag && REGNO(x) == REGNO(pid_reg_rtx))
        return PACK_ALIGN(4, 0);
#endif

      /* Fall-Thru */
    case SUBREG:
    case MEM:
      ud_num = lookup_ud(insn_id, x_p, &df_univ);
      if ((UDN_ATTRS(df_univ, ud_num) & S_DEF) != 0)
        return PACK_ALIGN(0,0);
      return use_compute_func(df_univ, ud_num);

    case SIGN_EXTEND:
    case ZERO_EXTEND:
      ud_num = lookup_ud(insn_id, x_p, &df_univ);
      if (ud_num != -1)
      {
        if ((UDN_ATTRS(df_univ, ud_num) & S_DEF) != 0)
          return PACK_ALIGN(0,0);
        return use_compute_func(df_univ, ud_num);
      }
      return (compute_rtx_expr_align(insn_id, &XEXP(x,0), use_compute_func));

    case CONST:
      /* just look down at its operand */
      return compute_rtx_expr_align(insn_id, &XEXP(x,0), use_compute_func);

    case SYMBOL_REF:
      /*
       * find the symbols size, and figure out what's its alignment
       * must have been from that.
       */
      al0 = SYMREF_SIZE(x);
      if (al0 >= 16)
        return PACK_ALIGN(4,0);
      else if (al0 >= 8)
        return PACK_ALIGN(3,0);
      else if (al0 >= 4)
        return PACK_ALIGN(2,0);
      else if (al0 >= 2)
        return PACK_ALIGN(1,0);
      else
        return PACK_ALIGN(0,0);

    case CONST_INT:
      return PACK_ALIGN(MAX_BITS_KNOWN, XINT(x,0));

    case PLUS:
      al0 = compute_rtx_expr_align(insn_id, &XEXP(x,0), use_compute_func);
      al1 = compute_rtx_expr_align(insn_id, &XEXP(x,1), use_compute_func);
      b0_known = BITS_KNOWN(al0);
      b0_val   = BIT_VAL(al0);
      b1_known = BITS_KNOWN(al1);
      b0_val  += BIT_VAL(al1);
      if (b0_known > b1_known)
        b0_known = b1_known;
      return PACK_ALIGN(b0_known, b0_val);
      
    case MINUS:
      al0 = compute_rtx_expr_align(insn_id, &XEXP(x,0), use_compute_func);
      al1 = compute_rtx_expr_align(insn_id, &XEXP(x,1), use_compute_func);
      b0_known = BITS_KNOWN(al0);
      b0_val   = BIT_VAL(al0);
      b1_known = BITS_KNOWN(al1);
      b0_val  -= BIT_VAL(al1);
      if (b0_known > b1_known)
        b0_known = b1_known;
      return PACK_ALIGN(b0_known, b0_val);

    case MULT:
#ifndef GCC20
    case UMULT:
#endif
      al0 = compute_rtx_expr_align(insn_id, &XEXP(x,0), use_compute_func);
      al1 = compute_rtx_expr_align(insn_id, &XEXP(x,1), use_compute_func);
      b0_known = BITS_KNOWN(al0);
      b1_known = BITS_KNOWN(al1);
      b0_val = BIT_VAL(al0);
      b1_val = BIT_VAL(al1);

      /*
       * the number of bits known is
       * max (min(b0_known,b1_known),
       *      number of low bit zeros in the product of the two bit vals)
       */
      {
        unsigned int i;
        unsigned int mask;

        if (b0_known > b1_known)
          b0_known = b1_known;

        b0_val *= b1_val;

        for (i = 0, mask = 1; ;i++, mask <<= 1)
        {
          if ((b0_val & mask) != 0)
            break;
        }

        if (b0_known < i)
          b0_known = i;
      }

      return PACK_ALIGN(b0_known, b0_val);

    case LSHIFT:
    case ASHIFT:
      al0 = compute_rtx_expr_align(insn_id, &XEXP(x,0), use_compute_func);
      al1 = compute_rtx_expr_align(insn_id, &XEXP(x,1), use_compute_func);
      b0_known = BITS_KNOWN(al0);
      b1_known = BITS_KNOWN(al1);
      b0_val = BIT_VAL(al0);
      b1_val = BIT_VAL(al1);
      b1_val &= ((unsigned long)(0xFFFFFFFF)) >> (HOST_BITS_PER_INT - b1_known);
      b1_val &= 0x1F;  /* bewteen 0 and 31 */

      b0_val <<= b1_val;
      b0_known += b1_val;
      return PACK_ALIGN(b0_known, b0_val);

    case LSHIFTRT:
    case ASHIFTRT:
      al0 = compute_rtx_expr_align(insn_id, &XEXP(x,0), use_compute_func);
      al1 = compute_rtx_expr_align(insn_id, &XEXP(x,1), use_compute_func);
      b0_known = BITS_KNOWN(al0);
      b1_known = BITS_KNOWN(al1);
      b0_val = BIT_VAL(al0);
      b1_val = BIT_VAL(al1);
      b1_val &= ((unsigned long)(0xFFFFFFFF)) >> (HOST_BITS_PER_INT - b1_known);
      b1_val &= 0x1F;  /* bewteen 0 and 31 */

      if (b1_val >= b0_known)
        b0_known = 0;
      else
      {
        b0_val >>= b1_val;
        b0_known -= b1_val;
      }
      return PACK_ALIGN(b0_known, b0_val);
      
    default:
      return PACK_ALIGN(0,0);
  }
}

static unsigned int
compute_def_insn_align (df_univ, def, use_compute_func)
ud_info * df_univ;
int def;
unsigned int (*use_compute_func)();
{
  rtx def_insn, def_urtx, def_vrtx;

  unsigned int def_align; /* computed def alignment */
  unsigned int type_align;  /* implicit alignment of type */
  int def_off;

  def_insn = UDN_INSN_RTX(df_univ, def);
  def_off  = UDN_VAR_OFFSET(df_univ, def);
  def_urtx = UDN_RTX(df_univ, def);
  def_vrtx = UDN_VRTX(df_univ, def);

  if (def_off != 0 && (def_urtx != def_vrtx || GET_CODE(def_urtx) != REG))
    return PACK_ALIGN(0,0);

  if (GET_CODE(def_insn) != INSN)
    return PACK_ALIGN(0,0);

  if (GET_CODE(PATTERN(def_insn)) != SET)
    return PACK_ALIGN(0,0);

  if (def_urtx != SET_DEST(PATTERN(def_insn)))
    return PACK_ALIGN(0,0);

  if (GET_CODE(def_vrtx) == REG)
    type_align = compute_ptr_type_align(RTX_TYPE(def_vrtx));
  else
    type_align = PACK_ALIGN(0,0);
  
  def_align = compute_rtx_expr_align(INSN_UID(def_insn),
                                     &SET_SRC(PATTERN(def_insn)),
                                     use_compute_func);

  if (BITS_KNOWN(type_align) > BITS_KNOWN(def_align))
    def_align = type_align;

  return def_align;
}

/*
 * compute the forward alignment of the use ud passed in.
 * This will be for least of the alignments of the defs that reach
 * this use without traversing a back edge.  This routine will also
 * fill the exact alignment in if there are no defs that reach this
 * use except ones reaching via forward edges.
 */
static unsigned int
compute_use_forw_align(df_univ, use)
ud_info * df_univ;
int use;
{
  int forw_align;
  int def;
  int use_var_offset;
  int use_bytes;
  rtx urtx, vrtx;

  forw_align = UDN_FORW_ALIGN(df_univ, use);
  if (forw_align != AL_NC)
    return (forw_align);

  use_var_offset = UDN_VAR_OFFSET(df_univ, use);
  urtx = UDN_RTX(df_univ, use);
  vrtx = UDN_VRTX(df_univ, use);

  if (use_var_offset==0 || (urtx==vrtx && GET_CODE(vrtx)==REG))
  {
    ud_set defs_reaching;

    /* start it out great, and let it work down. */
    forw_align = AL_UNKNOWN;

    use_bytes = UDN_BYTES(df_univ, use);
    defs_reaching = UDN_DEFS(df_univ, use);

    for (def = next_flex(defs_reaching, -1); def != -1;
         def = next_flex(defs_reaching, def))
    {
      if (UDN_VAR_OFFSET(df_univ, def) == use_var_offset &&
          UDN_BYTES(df_univ, def) == use_bytes)
      {
        if (is_forward_use(df_univ, def, use))
        {
          forw_align = join_align(forw_align,
                                  compute_def_forw_align(df_univ, def));

          if (BITS_KNOWN(forw_align) == 0)
            break;
        }
      }
      else
        forw_align = PACK_ALIGN(0,0);
    }
  }
  else
    forw_align = PACK_ALIGN(0,0);

  UDN_FORW_ALIGN(df_univ, use) = forw_align;
  if (BITS_KNOWN(forw_align) == 0)
    UDN_FULL_ALIGN(df_univ, use) = forw_align;

  return forw_align;
}

/*
 * compute the alignment of the use ud passed in.
 * This is the least of the forward alignment and all the backwards
 * aligments that reach the use.
 */
static unsigned int
compute_use_align(df_univ, use)
ud_info * df_univ;
int use;
{
  int align;
  int def;
  ud_set defs_reaching;

  align = UDN_FORW_ALIGN(df_univ, use);
  if (align != AL_NC)
  {
    align = UDN_FULL_ALIGN(df_univ, use);
    if (align != AL_NC)
      return (align);
  }
  else
    compute_use_forw_align(df_univ, use);

  align = UDN_FORW_ALIGN(df_univ, use);
  UDN_FULL_ALIGN(df_univ, use) = align;     /* stops infinite recursion. */

  if (BITS_KNOWN(align) == 0)
    return align;       /* can't improve, don't waste the time */

  defs_reaching = UDN_DEFS(df_univ, use);
  for (def = next_flex(defs_reaching, -1); def != -1;
       def = next_flex(defs_reaching, def))
  {
    align = join_align(align, compute_def_align(df_univ, def));
    if (BITS_KNOWN(align) == 0)
      break;
  }

  if (align == AL_UNKNOWN)
    align = PACK_ALIGN(MAX_BITS_KNOWN, 0);

  UDN_FULL_ALIGN(df_univ, use) = align;
  return align;
}

/*
 * compute the forward flowing alignment for the def passed in as a
 * parameter.  This entails examining the instruction that causes the
 * def, and if necessary computing the forward use information for any
 * uses in the instruction that would effect the alignment of the def.
 */
static unsigned int
compute_def_forw_align(df_univ, def)
ud_info * df_univ;
int def;
{
  int forw_align;

  forw_align = UDN_FORW_ALIGN(df_univ, def);

  if (forw_align != AL_NC)
    return (forw_align);

  forw_align = compute_def_insn_align (df_univ, def, compute_use_forw_align);

  UDN_FORW_ALIGN(df_univ, def) = forw_align;
  return forw_align;
}

static unsigned int
compute_def_align(df_univ, def)
ud_info * df_univ;
int def;
{
  int align;

  compute_def_forw_align(df_univ, def);

  align = UDN_FULL_ALIGN(df_univ, def);

  if (align != AL_NC)
    return (align);

  UDN_FULL_ALIGN(df_univ, def) = align = UDN_FORW_ALIGN(df_univ, def);

  align = compute_def_insn_align (df_univ, def, compute_use_align);

  UDN_FULL_ALIGN(df_univ, def) = align;
  return align;
}

static unsigned int
compute_ptr_type_align (tree_type)
tree tree_type;
{
  if (tree_type == 0)
    return PACK_ALIGN(0,0);

  if (TREE_CODE(tree_type) != POINTER_TYPE ||
      TREE_CODE(tree_type = TREE_TYPE(tree_type)) == VOID_TYPE)
    return PACK_ALIGN(0,0);

  switch (TYPE_ALIGN(tree_type))
  {
    default:
      return PACK_ALIGN(0,0);
    case 2 * BITS_PER_UNIT:
      return PACK_ALIGN(1,0);
    case 4 * BITS_PER_UNIT:
      return PACK_ALIGN(2,0);
    case 8 * BITS_PER_UNIT:
      return PACK_ALIGN(3,0);
    case 16 * BITS_PER_UNIT:
      return PACK_ALIGN(4,0);
  }
}

static unsigned int
compute_mode_align (mode)
enum machine_mode mode;
{
  switch (GET_MODE_BITSIZE(mode))
  {
    default:
      return PACK_ALIGN(0,0);
    case 2 * BITS_PER_UNIT:
      return PACK_ALIGN(1,0);
    case 4 * BITS_PER_UNIT:
      return PACK_ALIGN(2,0);
    case 8 * BITS_PER_UNIT:
      return PACK_ALIGN(3,0);
    case 16 * BITS_PER_UNIT:
      return PACK_ALIGN(4,0);
  }
}

/*
 * return an integer that tells what the maximal known alignment 
 */
unsigned int
compute_mem_align(insn_id, mem_rtx, const_off)
int insn_id;
rtx mem_rtx;
int const_off;
{
  unsigned int mem_align;
  unsigned int mode_align;
  unsigned int bits_known;
  unsigned int bit_val;

  mode_align = compute_mode_align(GET_MODE(mem_rtx));

  mem_align =
    compute_rtx_expr_align(insn_id, &XEXP(mem_rtx,0), compute_use_forw_align);
  mem_align = join_align(mem_align,
    compute_rtx_expr_align(insn_id, &XEXP(mem_rtx,0), compute_use_align));

  if (BITS_KNOWN(mode_align) > BITS_KNOWN(mem_align))
    mem_align = mode_align;

  bits_known = BITS_KNOWN(mem_align);
  bit_val = BIT_VAL(mem_align);
  bit_val += const_off;

  mem_align = PACK_ALIGN(bits_known, bit_val);
  
  return BIT_VAL(mem_align);
}
#endif
