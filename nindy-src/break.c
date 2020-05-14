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
#include "globals.h"
#include "break.h"

static const char no_dbpts[] = "\n No Data breakpoints in this architecture";

/************************************************/
/* Display Breakpoints              	 	*/
/*                           			*/
/************************************************/
static
display_bpt( type )
char type;	/* BRK_INST or BRK_DATA */
{
int count;
struct bpt *bp;

	count = 0;
	prtf("\n %s breakpoints set: ",type==BRK_INST ? "Instruction":"Data");
	for ( bp = &bptable[0]; bp->type != BRK_EOT; bp++ ){
		if (bp->active && bp->type == type){
			count++;
			prtf (" %X", bp->addr);
		}
	}
	if (count == 0) {
		prtf (" none");
	}
}

/************************************************/
/* Search Table for a Specific Breakpoint	*/
/*						*/
/* To match, a breakpoint in the table must have*/
/* The specified type and the specified 'active'*/
/* state.  If and only if the breakpoint is 	*/
/* active, the address must also match.	`	*/
/*						*/
/************************************************/
static
struct bpt *
find_bpt( type, active, addr )
char type;		/* BRK_DATA or BRK_INST		*/
char active;		/* TRUE or FALSE		*/
int *addr;		/* Only valid if active == TRUE	*/
{
struct bpt *bp;

	for ( bp = &bptable[0]; bp->type != BRK_EOT; bp++ ){
		if ( bp->type == type
		&&   bp->active == active 
		&&   (!active || bp->addr == addr) ){
			return bp;
		}
	}
	return NULL;
}

/************************************************/
/* Set a Breakpoint              	 	*/
/*                           			*/
/* 'breakpt' and 'databreak' set (or display)	*/
/* instruction and data breakpoints, respectivly*/
/*						*/
/* 'set_bpt' is a lower-level entry point that 	*/
/* does the actual work.  It is also invoked by */
/* the 'gdb mode' code to satisfy remote debug-	*/
/* ger requests.  In support of the latter, it	*/
/* returns TRUE if it succeeds, FALSE otherwise.*/
/************************************************/
breakpt( dummy, nargs, addr )
int dummy;	/* Ignored */
int nargs;	/* Number of following arguments that are valid (0 or 1) */
int addr;	/* Optional breakpoint address				*/

{
	if ( nargs == 0 ){
		display_bpt( BRK_INST );
	} else {
		(void) set_bpt( BRK_INST, addr );
	}
}


databreak( dummy, nargs, addr )
int dummy;	/* Ignored */
int nargs;	/* Number of following arguments that are valid (0 or 1) */
int addr;	/* Optional breakpoint address			*/
{
	if ( !have_data_bpts ){
		prtf( no_dbpts );
	} else if ( nargs == 0 ){
		display_bpt( BRK_DATA );
	} else {
		(void) set_bpt( BRK_DATA, addr );
	}
}


set_bpt( type, addr )
char type;
int addr;
{
struct bpt *bp;
char *typename;
int retval;	/* Return code, needed by gdb interface */

	retval = TRUE;	/* Assume success */

	typename = (type == BRK_INST) ? "Instruction" : "Data";

	if ( find_bpt(type,TRUE,addr) ){
		prtf("\n %s breakpoint already set at %X", typename, addr);

	} else if ( (bp = find_bpt(type,FALSE,0)) == NULL ){
		prtf("\n No %s breakpoints left", typename);
		retval = FALSE;

	} else {
		bp->addr = (int *)addr;
		bp->active = TRUE;
		pgm_bpt_hw(type);
	}

	return retval;
}

/************************************************/
/* Delete a Breakpoint              	 	*/
/*                           			*/
/* 'delete' and 'delete_data' remove instruc-	*/
/* tion and data breakpoints, respectivly.	*/
/*						*/
/* 'del_bpt' is a lower-level entry point that 	*/
/* does the actual work.  It is also invoked by */
/* the 'gdb mode' code to satisfy remote debug-	*/
/* ger requests.				*/
/************************************************/
delete( dummy, nargs, addr )
int dummy;	/* Ignored */
int nargs;	/* Number of following arguments that are valid (0 or 1) */
int addr;	/* Optional breakpoint address	*/
{
	del_bpt( BRK_INST, addr );
}


delete_data( dummy, nargs, addr )
int dummy;	/* Ignored */
int nargs;	/* Number of following arguments that are valid (0 or 1) */
int addr;	/* Optional breakpoint address	*/
{
	if ( !have_data_bpts ){
		prtf( no_dbpts );
	} else {
		del_bpt( BRK_DATA, addr );
	}
}


del_bpt( type, addr )
char type;
int *addr;
{
struct bpt *bp;
int retval;	/* Return code, needed by gdb interface */


	if ( (bp = find_bpt(type,TRUE,addr)) == NULL ){
		prtf("\n No %s breakpoint set at %X",
				type==BRK_INST ? "Instruction" : "Data", addr);
	} else {
		bp->active = FALSE;
		pgm_bpt_hw( type );
		display_bpt( type );
	}
}
