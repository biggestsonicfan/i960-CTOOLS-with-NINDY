#include "config.h"

#ifdef IMSTG
/*  Copyright (C) 1987, 1988, 1992 Free Software Foundation, Inc.

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
#include <errno.h>

#include "tree.h"
#include "rtl.h"
#include "flags.h"
#include "regs.h"
#include "insn-config.h"
#include "c-tree.h"
#include "assert.h"
#include "i_dataflow.h"

int is_inner_redecl (decl)
tree decl;
{
  tree outer_decl;
    
  /* see if there is a more global declaration that this refers to.*/
  outer_decl = IDENTIFIER_GLOBAL_VALUE(DECL_NAME(decl));

  return (outer_decl != decl && outer_decl != 0);
}

/* Look at the symbol defined by DECL,
   which must be a ..._DECL node in the normal namespace.
   LOCAL is nonzero if the scope is less than the entire file.  */

void
symwalk_symbol (decl, local)
tree decl;
int local;
{
  tree type;
  rtx  r;

  type = TREE_TYPE (decl);
  r    = DECL_RTL  (decl);

  if (!DECL_IGNORED_P (decl) && type != error_mark_node && r != 0)
    switch (TREE_CODE (decl))
    {
      case PARM_DECL:
      { rtx entry_parm = DECL_INCOMING_RTL(decl);

        if (entry_parm)
        {
          set_rtx_type (entry_parm, type);

          if (GET_CODE(entry_parm)==REG && REGNO(entry_parm) < FIRST_PSEUDO_REGISTER)
            if (GET_MODE(entry_parm) == BLKmode ||
                !HARD_REGNO_MODE_OK(REGNO(entry_parm),GET_MODE(entry_parm)))
            { /* Dataflow cannot use BLKmode or unaligned registers to set up
                 its dataflow information for procedure entry.  These never
                 actually appear in the rtl, but we need to set up pseudos
                 for them, so we treat them as a group of single-word
                 parameters. */

              int regno = REGNO(entry_parm);
              int nregs = int_size_in_bytes(type) / UNITS_PER_WORD;

              while (nregs--)
              {
                assert(df_hard_reg[regno+nregs].reg);
                set_rtx_type(df_hard_reg[regno+nregs].reg, type);
                df_hard_reg[regno+nregs].is_parm = 1;
              }
            }
            else
            { df_hard_reg[REGNO(entry_parm)].reg     = entry_parm;
              df_hard_reg[REGNO(entry_parm)].is_parm = 1;
            }
        }
      }

      /* Fall thru */
      default:
        set_rtx_type (r, type);

        if (GET_CODE(r)==MEM && GET_CODE(XEXP(r,0))==SYMBOL_REF)
        {
          rtx sym_ref = XEXP(r,0);
          int t = glob_addr_taken_p(XSTR(sym_ref, 0));
          int n = 0;

          if (t == -1)
          {
            /* wasn't found in the data base */
            if (!local || TREE_ADDRESSABLE(decl))
              SYM_ADDR_TAKEN_P(sym_ref) = 1;
          }
          else
            SYM_ADDR_TAKEN_P(sym_ref) = t;

          /* Size of symbol space allocation has to be at least as big
             as the largest reference ... */
          if (GET_MODE(r) != VOIDmode)
            n = GET_MODE_SIZE(GET_MODE(r));

          /* If a bigger size is available from the type, use it. */
          if (type != 0 && (t=int_size_in_bytes(type)) > n)
            n = t;

          if (n > SYMREF_SIZE(sym_ref))
            SYMREF_SIZE(sym_ref) = n;

          /* Don't allow SYMREF_ADDRTAKEN to go from 1 to 0. */
          if (SYMREF_ADDRTAKEN(sym_ref) == 0)
            SYMREF_ADDRTAKEN(sym_ref) = TREE_ADDRESSABLE(decl);

          if (!is_inner_redecl(decl))
            set_rtx_type (XEXP(r,0), build_pointer_type(type));
        }
        else if (GET_CODE(r) == SYMBOL_REF)
        {
          /*
           * Only way for this to occur is if the symbols address
           * was taken and and assigned to a pointer variable.
           */
          SYM_ADDR_TAKEN_P(r) = 1;
          SYMREF_ADDRTAKEN(r) = 1;
        }
        break;
    }
}

static void
symwalk_parms (parms)
     tree parms;
{
  while (parms)
  {
    symwalk_symbol (parms, 1);
    parms = TREE_CHAIN (parms);
  }
}

/* Look at a symbol block (a BLOCK node that represents a scope level),
   including contained blocks.

   BLOCK is the BLOCK node.  ARGS is usually zero, but for the outermost
   block of the body of a function, it is a chain of PARM_DECLs for the
   function parameters.

   Actually, BLOCK may be several blocks chained together.
   We handle them all in sequence.  */

static void
symwalk_block (block, args)
register tree block;
tree args;
{
  int blocknum;

  while (block)
  {
    if (TREE_USED (block))
    {
      tree t = BLOCK_VARS(block);

      while (t)
      {
        symwalk_symbol (t,1);
        t = TREE_CHAIN (t);
      }

      if (args)
        symwalk_parms (args);

      /* Output the subblocks.  */
      symwalk_block (BLOCK_SUBBLOCKS (block), 0);
    }
    block = BLOCK_CHAIN (block);
  }
}

/* Walk all symbols for a function definition.
   This includes a definition of the function name itself (a symbol),
   definitions of the parameters, and then the block that makes up the
   function body (including all the auto variables of the function).  */

void
symwalk_function (decl)
tree decl;
{
  int i;
#ifdef SETUP_HARD_PARM_REG
  SETUP_HARD_PARM_REG (decl);
#else
  bzero (df_hard_reg, sizeof (df_hard_reg));
#endif

  symwalk_symbol (decl, 0);
  symwalk_parms (DECL_ARGUMENTS (decl));
  symwalk_symbol (DECL_RESULT (decl), 1);
  symwalk_block (DECL_INITIAL (decl), 0, DECL_ARGUMENTS (decl));
}

#endif
