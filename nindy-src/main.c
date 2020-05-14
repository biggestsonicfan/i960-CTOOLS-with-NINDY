/******************************************************************/
/* 		Copyright (c) 1989, Intel Corporation

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
#include "regs.h"

extern int monitor();

/****************************************/
/* Storage for NINDY's copies of the	*/
/* user's register set.			*/
/****************************************/
unsigned int register_set[NUM_REGS];   /* local, global, special regs */
double	fp_register_set[NUM_FP_REGS];  /* fp register set */


/****************************************/
/* Main		  			*/ 
/* 					*/ 
/****************************************/
main()
{
int i;
int baudrate;

	if ( !strcmp(end.r_magic,RESTART_ARGS_VALID) ){
		baudrate = baudtable[end.r_baud];
		gdb = end.r_gdb;
	} else {
		baudrate = 9600;
		end.r_baud = lookup_baud(baudrate);
		gdb = FALSE;
		strcpy(end.r_magic,RESTART_ARGS_VALID);
	}
	end.r_gdb = FALSE;	/* Next reset may be from keyboard.  */

	init_console(baudrate); 
	init_flash();
	init_regs();

	user_is_running = downld = FALSE;
	fault_cnt = 0;

	if ( gdb ){
		co('+');	/* Let GDB know we're done resetting. */
	}

	out_message();
	monitor();
}

/****************************************/
/* Initialize Register Set    		*/
/* 					*/ 
/* initialize the stored "register set" */
/* to correct values, so that if called */
/* from a start up routine, the regs    */
/* will be set appropriately 		*/
/****************************************/
init_regs()
{
unsigned int *reg_ptr;

	/* set pointer to user stack we will initialize 
		the "registers" to point at this area */
	reg_ptr  = (unsigned int *) &nindy_stack;	 

	/* set frame, stack pointer to point at current frame */
	register_set[REG_FP]  = (unsigned)reg_ptr + 64;
	register_set[REG_PFP] = (unsigned)reg_ptr;
	register_set[REG_SP]  = (unsigned)reg_ptr + 128;
	
	/* set ip to point to monitor, in case ip isn't set  */   
	register_set[REG_IP] = (unsigned)monitor;
	register_set[REG_TC] = (unsigned) 0x0;

	/* initial Process controls */
	register_set[REG_PC] = (unsigned)0x1f0003;

	/* initialize floating point register set */
	fp_register_set[REG_FP0] = 0;
	fp_register_set[REG_FP1] = 0;
	fp_register_set[REG_FP2] = 0;
	fp_register_set[REG_FP3] = 0;
}
