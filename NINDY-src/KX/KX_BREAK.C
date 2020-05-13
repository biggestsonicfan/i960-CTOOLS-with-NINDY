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
#include "iac.h"
#include "defines.h"
#include "break.h"
#include "regs.h"

/* Instruction breakpoints only in the 960KX architecture.
 */

struct bpt bptable[] = {
	{ BRK_INST, FALSE, 0 },
	{ BRK_INST, FALSE, 0 },
	{ BRK_EOT,  FALSE, 0 },
};

char have_data_bpts = FALSE;

/****************************************/
/* Program Breakpoint Hardware 		*/
/*					*/
/* This version, for the 960CX, uses	*/
/* an IAC to reload the	appropriate 	*/
/* registers.				*/
/* table.				*/
/*					*/
/* This routine reprograms *all* break-	*/
/* points of the specified type to the	*/
/* state currently found in 'bptable'.	*/
/****************************************/
void
pgm_bpt_hw( type )
char type;	/* Only BRK_INST will have any effect on this processor*/
{
int addr[2];
struct bpt *bp;
int count;
iac_struct iac;

	addr[0] = addr[1] = 2;	/* Disable breakpoints */
	count = 0;
	for ( bp = &bptable[0]; bp->type != BRK_EOT; bp++ ){
		if ( bp->active && bp->type == type ){
			addr[count++] = ((int) bp->addr) & ~3;
		}
	}
	iac.message_type = 0x8f;
	iac.field3 = addr[0];
	iac.field4 = addr[1];
	send_iac( (int)&iac );
	register_set[REG_TC] |= 0x80;
}
