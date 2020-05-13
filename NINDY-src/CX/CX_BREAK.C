/******************************************************************/
/* 		Copyright (c) 1989, 1990, Intel Corporation

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

   Intel Corporation does not warrant, guarantee or make any 
   representations regarding the use of, or the results of the use
   of, the software and documentation in terms of correctness, 
   accuracy, reliability, currentness, or otherwise; and you rely
   on the software, documentation and results solely at your own 
   risk.							  */
/******************************************************************/
#include "defines.h"
#include "break.h"
#include "regs.h"


/* 960 CX architecture has 2 instruction breakpoints and 2 data breakpoints
 */
struct bpt bptable[] = {
	{ BRK_INST, FALSE, 0 },	
	{ BRK_INST, FALSE, 0 },
	{ BRK_DATA, FALSE, 0 },
	{ BRK_DATA, FALSE, 0 },
	{ BRK_EOT,  FALSE, 0 },
};

char have_data_bpts = TRUE;

extern unsigned int control_table[16];

/****************************************/
/* Program Breakpoint Hardware 		*/
/*					*/
/* This version, for the 960CX, uses	*/
/* the sysctl instruction to reload the	*/
/* appropriate registers in the control	*/
/* table.				*/
/*					*/
/* This routine reprograms *all* break-	*/
/* points of the specified type to the	*/
/* state currently found in 'bptable'.	*/
/****************************************/
void
pgm_bpt_hw( type )
char type;		/* BRK_INST or BRK_DATA */
{
int addr[2];
struct bpt *bp;
int count;
int mask;
int i;

	addr[0] = addr[1] = 2;	/* Will disable both instruction breakpoints */
	count = 0;

	if ( type == BRK_INST ){
		for ( bp = &bptable[0]; bp->type != BRK_EOT; bp++ ){
			if ( bp->active && bp->type == type ){
				addr[count++] = ((int) bp->addr) | 3;
				/* The '| 3' enables the instruction bpt */
			}
		}
		i = 0;	/* Entries 0 & 1 in control table will be set */

	} else {	/* DATA BREAKPOINT */

		control_table[25] &= 0xffccffff;
		mask = 0x000f0000;
		for ( bp = &bptable[0]; bp->type != BRK_EOT; bp++ ){
			if ( bp->active && bp->type == type ){
				addr[count++] = (int) bp->addr;
				control_table[25] |= mask;
				mask <<= 4;
			}
		}
		i = 2;	/* Entries 2 & 3 in control table will be set */
		send_sysctl(0x406, 0, 0);   /* Reload control register group 6*/
	}


	control_table[i++] = addr[0];
	control_table[i] = addr[1];
	send_sysctl(0x400, 0, 0);   /* Reload control register group 0 */

	register_set[REG_TC] |= 0x80;
}
