#include "config.h"

#ifdef IMSTG

#include <stdio.h>	/* only necessary for dataflow.h */
#include <setjmp.h>
#include "assert.h"
#include "tree.h"	/* only necessary for dataflow.h */
#include "rtl.h"
#include "regs.h"
#include "flags.h"
#include "basic-block.h"
#include "machmode.h"
#include "i_dataflow.h"
#include "i_df_set.h"

rtx find_jump_table_use(r)
rtx r;
{
  RTX_CODE c = GET_CODE(r);

  rtx ret = 0;

  if (c==USE)
  { if (XEXP(r,0)!=0 && GET_CODE(XEXP(r,0))==LABEL_REF)
      ret = XEXP(r,0);
  }

  else
  {
    int   len = GET_RTX_LENGTH(c);
    char* fmt = GET_RTX_FORMAT(c);
    int i;

    for (i = 0; i < len && ret==0; i++)
      if (fmt[i] == 'e')
        ret = find_jump_table_use (XEXP (r, i));

      else if (fmt[i] == 'E')
      {
        register int j;
        for (j = XVECLEN (r, i) - 1; j >= 0 && ret==0; j--)
          ret = find_jump_table_use (XVECEXP (r, i, j));
      }
  }
  return ret;
}

int
remove_dead_code ()
{
  int eliminated = 0;

  flex_set pile, marked, marked_blocks;
  rtx insn;
  int change;
  ud_info* reg;
  uid_set* reach;
  bb_set* branch_scope;
  bb_set* succs;
  int otime;

  insn_subst_si_for_qihi(0);

  otime = get_run_time ();

  update_dataflow(MK_SET(IDF_REG),0);
  restart_dataflow (df_info+DF_SYM);
  restart_dataflow (df_info+DF_MEM);

  dataflow_time += get_run_time() - otime;

  if ((GET_DF_STATE(df_info+IDF_REG) & DF_ERROR) ||
      !(GET_DF_STATE(df_info+IDF_REG) & DF_REACHING_DEFS))
    return 0;

  find_dead_coercions ();

  reg = df_info+IDF_REG;
  succs = df_data.maps.succs;

  ALLOC_MAP (&df_data.maps.ctrl_dep, N_BLOCKS, flex_set_map);
  branch_scope = df_data.maps.ctrl_dep;
  {
    int i;
    for (i = 0; i < N_BLOCKS; i++)
      branch_scope[i] = -1;
  }

  new_flex_pool (&df_data.pools.ctrl_dep_pool, 1, N_BLOCKS, F_DENSE);
  marked_blocks = alloc_flex (df_data.pools.ctrl_dep_pool);

  ALLOC_MAP (&df_data.maps.insn_reach,  NUM_UID,  flex_set_map);
  reach = df_data.maps.insn_reach;

  new_flex_pool (&df_data.pools.insn_reach_pool, NUM_UID, NUM_UID/8, F_DENSE);
  pile   = alloc_flex (df_data.pools.insn_reach_pool);
  marked = alloc_flex (df_data.pools.insn_reach_pool);

  /* Build the insn_reach map from reg->defs_reaching;  mark
     initial critical instructions. */

  for (insn=get_insns(); insn; insn=NEXT_INSN(insn))
  { int i = INSN_UID(insn);
    enum rtx_code c  = GET_CODE(insn);
    enum rtx_code cp = (REAL_INSN(insn)) ? (GET_CODE(PATTERN(insn))) : 0;

    if (cp != 0 || df_data.maps.insn_info[i].insn_has_uds)
    { int crit      = 0;
      int elt       = reg->maps.uid_map[i].uds_in_insn;

      while (elt != -1)
      { if (UDN_ATTRS(reg,elt) & S_USE)
        { int d = -1;
          while ((d=next_flex(reg->maps.defs_reaching[elt],d)) != -1)
          { if (reach[i] == 0)
              reach[i] = alloc_flex(df_data.pools.insn_reach_pool);

            set_flex_elt (reach[i], UDN_INSN_UID(reg, d));
          }
        }
        elt=reg->uds_in_insn_pool[elt];
      }

      crit = UID_VOLATILE(i);

      if (c==CALL_INSN)
        ;  /* Critical if not between LIBCALL/RETVAL */

      else if (c==INSN)
      { if (cp==USE)
        { /* Say that USE insns reach the following insn; if we need the
             following insn, we will need the USE */

          int ni = INSN_UID(NEXT_INSN(insn));
  
          if (reach[ni] == 0)
            reach[ni] = alloc_flex(df_data.pools.insn_reach_pool);
  
          set_flex_elt (reach[ni], i);
        }
        else if (cp==SET)
        { rtx r = SET_DEST(PATTERN(insn));
  
          if (GET_CODE(r) == SUBREG)
            r = SUBREG_REG(r);
          crit |= (GET_CODE(r)!=REG);
        }
        else
          crit = 1;
      }

      else if (c==JUMP_INSN)
      { int b = BLOCK_NUM(insn);
        rtx r = find_jump_table_use(PATTERN(insn));
        int n = next_flex(succs[b],-1);

        /* Say that all backward branches are critical, and the jump
           (usually return) out of the last basic block is critical. */

        if (/* n <= b || */ b==N_BLOCKS-3)
          crit = 1;

        /* Say that an indirect jump reaches its table, so that
           the jump gets pulled in when the table does. */

        if (r)
        { int ri;

          r = XEXP(r,0);
          assert (GET_CODE(r)==CODE_LABEL);
          r = NEXT_INSN(r);
          assert (GET_CODE(r)==JUMP_INSN);

          ri = INSN_UID(r);
          if (reach[ri] == 0)
            reach[ri] = alloc_flex(df_data.pools.insn_reach_pool);

          set_flex_elt (reach[ri], i);
        }
        else
          /* Record the most forward successor for forward branches or
             the earliest successor for backward branches. */

          if (!crit)
          { int t, s;

            s = n;

            while ((t = next_flex(succs[b],n)) != -1)
              n = t;

            /* Say a branch that is not strictly backward or strictly forward
               is critical */

            if (s <= b)
              if (n > b+1)
                crit = 1;
              else
                branch_scope[b] = s;
            else
              branch_scope[b] = n;
          }
      }
      else
        crit = 1;

      /* mark it as critical */
      if (crit)
        set_flex_elt(pile, INSN_UID(insn));
    }
  }

  do
  { int i,j,k;

    change = 0;

    /* Look at jumps which don't appear to be needed.  If any
       needed blocks now appear under the jump, put the jump on the pile */

    for (i = 0; i < N_BLOCKS; i++)
      if ((j = branch_scope[i]) != -1)
      { if (j <= i)
          k = i+1;  /* Backwards; look at earliest block thru current block */
        else
        {
          k = j;    /* Forward.  Look at next block up to last target. */
          j = i+1;
        }

        while (j < k)
        { if (in_flex (marked_blocks, j))
          { set_flex_elt (pile, INSN_UID(BLOCK_END(i)));
            branch_scope[i] = -1;
            break;
          }
          j++;
        }
      }

    /* Mark needed instructions, and the instructions reaching them. */
    while ((i = next_flex(pile, -1)) != -1)
    {
      change = 1;

      clr_flex_elt(pile, i);
      set_flex_elt(marked, i);
      set_flex_elt(marked_blocks, BLOCK_NUM(UID_RTX(i)));
  
      if (reach[i])
      { int d = -1;
  
        while ((d=next_flex(reach[i], d)) != -1)
          if (!in_flex(marked, d))
            set_flex_elt(pile, d);
      }
    }
  }
  while (change);

  /* now delete any insns whose ids are not in the marked set. */

  insn = get_insns();
  while (insn)
  { int i = INSN_UID(insn);
    if (df_data.maps.insn_info[i].insn_has_uds && !in_flex(marked,i))
    {
      insn = delete_insn(insn);
      eliminated++;
    }
    else
      insn = NEXT_INSN(insn);
  }

  return (eliminated);
}

#endif
