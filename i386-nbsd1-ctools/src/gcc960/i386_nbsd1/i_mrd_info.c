
#include "config.h"

#ifdef IMSTG

/*
  This file will be part of GNU CC.  Until then, it is the
  property of Intel Corporation, Copyright (C) 1991.
  
  GNU CC is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 1, or (at your option)
  any later version.
  
  GNU CC is distributed in the hope that it will be useful,
  But WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with GNU CC; see the file COPYING.  If not, write to
  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "rtl.h"
#include "expr.h"
#include "regs.h"
#include "flags.h"
#include "assert.h"
#include <stdio.h>
#include "tree.h"
#include "gvarargs.h"

#include "insn-config.h"
#include "hard-reg-set.h"
#include "insn-flags.h"
#include "basic-block.h"
#include "i_jmp_buf_str.h"
#include "i_dataflow.h"

#include "recog.h"


static int max_key; /* valid keys are > 0 and <= max_key */

/*
 * initialize the memory disambiguation support for the
 * machine instruction scheduler
 *
 * the memory conflict information is encoded by putting
 * an integer key and 15 bit vector into each MEM rtx's MEM_CONFLICT
 * field.  You can check if two different MEM rtx's conflict
 * by examining the keys and bit vectors as :
 *
 * if (key1 == 0 || key2 == 0 || abs (key1 - key2) > 15) then the mems conflict
 * else if ( key1 > key2 &&
 *           (bitvector1 & (1<<(key1-key2-1))) == 0 ) then the mems don't conflict
 * else if ( key2 > key1 &&
 *	     (bitvector2 & (1<<(key2-key1-1))) == 0 ) then the mems don't conflict
 * else the mems conflict
 *
 * the high order bit is reserved for the CAN_MOVE_MEM bit
 */

/*
 * This routine is only supposed to be called when disambiguation
 * information is up to date enough to get the job done correctly -
 * currently, that is right after memory dataflow and/or right
 * after coalescion.
*/

static tree last_sched_setup;

void imstg_sched_setup (insns)
	rtx insns;
{
	int block;		/* walks thru each basic block */
	rtx insn;		/* walks thru each instruction */
	uid_rec * uid_r;
        register lelt;
	rtx mem;
	int ud_window [15];
#define NULL_UD (-1)
	int prev_in_window;
	int w;			/* walks thru ud_window */
	unsigned bits;
	int position;
	ud_rec * ud1;
	ud_rec * ud2;

	/*
	 * zero is the reserved key
	 */
	max_key = 0;

	/*
	 * set up the window
	 */
	for ( w = 0; w < 15; ++w )
		ud_window [w] = NULL_UD;
	prev_in_window = 0;	/* index of prev mem in window */

	/*
	 * visit all the insn's using a 15 entry window
	 * to develop the MEM_CONFLICT information
	 */
	for ( insn = insns; insn; insn = NEXT_INSN (insn) ) {

		uid_r = &(df_info [IDF_MEM].maps.uid_map [INSN_UID (insn)]);

                for (lelt = uid_r->uds_in_insn; lelt != -1;
                     lelt = df_info[IDF_MEM].uds_in_insn_pool[lelt]) {
			mem = UDN_VRTX (df_info + IDF_MEM, lelt);
			assert (mem);
			ud1 = df_info [IDF_MEM].maps.ud_map + lelt;

			bits = 0x7fff;	/* inititalize to all 15 bits conflict */

			/*
			 * examine if mem conflicts with everything in ud_window
			 */
			position = 0;
			w = prev_in_window;
			do {
				if ( ud_window [w] == NULL_UD )
					break;
				ud2 = df_info [IDF_MEM].maps.ud_map + ud_window [w];

				/*
				 * check for no conflict
				 */
				if ( ! mem_ud_range (ud1, ud2, df_overlap) )
					bits &= ~(1 << position);

				if ( w == 0 )
					w = 14;
				else
					--w;
				++position;
			} while ( w != prev_in_window );

			/*
			 * store the information in the mem insn
			 */
			PUT_MEM_CONFLICT (mem, (bits << 16)|(++max_key));

			/*
			 * put this mem in the window
			 */
			if ( prev_in_window == 14 )
				prev_in_window = 0;
			else
				++prev_in_window;
			ud_window [prev_in_window] = lelt;
		} /* end for uds_in_insn */
	} /* end for insns */

  last_sched_setup = current_function_decl;
} /* end imstg_sched_setup */

/*
 *
 * examine the information in two mem's to see if they conflict
 *
 * if (key1 == 0 || key2 == 0 || abs (key1 - key2) > 15) then the mems conflict
 * else if ( key1 > key2 &&
 *           (bitvector1 & (1<<(key1-key2-1))) == 0 ) then the mems don't conflict
 * else if ( key2 > key1 &&
 *	     (bitvector2 & (1<<(key2-key1-1))) == 0 ) then the mems don't conflict
 * else the mems conflict
 *
 */
int imstg_mems_overlap (mem1, mem2)
	rtx mem1;
	rtx mem2;
{
	int key1;
	int key2;
	unsigned int bits;
	int diff;
	extern tree current_function_decl;

	/* If dataflow scan ran, all keys should at the least be
	   conservatively correct;  they are cleared in scan_rtx_for_uds
	   for the first trip thru a function.

	   Once they keys are established (by imstg_sched_setup), they
	   should remain correct, even if the mems are copied.  When
	   imstg_sched_setup runs, it should leave every key non-zero.
	*/


	if (max_key == 0  || last_sched_setup != current_function_decl)
		return 1;

	key1 = GET_MEM_CONFLICT (mem1) & 0xffff;
	if ( key1 == 0 )
		return 1;

	key2 = GET_MEM_CONFLICT (mem2) & 0xffff;
	if ( key2 == 0 )
		return 1;

	if ( key1 == key2 )
		return 1;

	if ( key1 > key2 ) {
		bits = (GET_MEM_CONFLICT (mem1) >> 16) & 0x7fff;
		diff = key1 - key2;
	} else {
		bits = (GET_MEM_CONFLICT (mem2) >> 16) & 0x7fff;
		diff = key2 - key1;
	} /* end if */

	if ( diff > 15 )
		return 1;

	--diff;
	return (bits & (1 << diff));
} /* end imstg_mems_overlap */

#endif
