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

#define VISITED  0x1
#define COMPUTED 0x2

static void
compute_forw_succs(blk, succs, visited, forw_succs)
int blk;
flex_set *succs;
unsigned char *visited;
flex_set *forw_succs;
{
  int succ_blk;

  visited[blk] |= VISITED;

  copy_flex(forw_succs[blk], succs[blk]);
  for (succ_blk = next_flex(succs[blk], -1); succ_blk != -1;
       succ_blk = next_flex(succs[blk], succ_blk))
  {
    if ((visited[succ_blk] & VISITED) != 0)
    {
      new_natural_loop (succ_blk, blk);
      clr_flex_elt(forw_succs[blk], succ_blk);
    }
    else
    {
      if ((visited[succ_blk] & COMPUTED) == 0)
        compute_forw_succs(succ_blk, succs, visited, forw_succs);
      union_flex_into(forw_succs[blk], forw_succs[succ_blk]);
    }
  }

  visited[blk] &= ~VISITED;
  visited[blk] |= COMPUTED;
}

void
compute_all_forw_succs()
{
  unsigned char * visited;

  visited = (unsigned char *)xmalloc(N_BLOCKS);
  bzero(visited, N_BLOCKS);

  compute_forw_succs(ENTRY_BLOCK, df_data.maps.succs, visited, df_data.maps.forw_succs);

  free (visited);
}

/*
 * return true if def reaches use via some forward flow path.
 * return false otherwise.
 */
int
is_forward_use(df_univ, def, use)
ud_info * df_univ;
int def;
int use;
{
  int def_block;
  int use_block;
  rtx def_insn;
  rtx use_insn;
  
  def_insn = UDN_INSN_RTX(df_univ, def);
  use_insn = UDN_INSN_RTX(df_univ, use);
  def_block = BLOCK_NUM(def_insn);
  use_block = BLOCK_NUM(use_insn);
  /*
   * if def and use are in the same block, then def must
   * appear before use in the block.
   */
  if (def_block == use_block)
  {
    rtx t_insn = NEXT_INSN(def_insn);

    while (t_insn != 0 && BLOCK_NUM(t_insn) == def_block)
    {
      if (t_insn == use_insn)
        return 1;

      t_insn = NEXT_INSN(t_insn);
    }
    return 0;
  }

  /*
   * if def and use are in different blocks, then the block def
   * is in must be a block that reaches uses's block via forward
   * flow.
   */
  if (in_flex(df_data.maps.forw_succs[def_block], use_block))
    return 1;
  return 0;
}

#endif
