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

/* Description of a single hardware breakpoint
 *
 * The number of and types of entries in the table are determined by the
 * capabilities of the target processor.  The 'type' field should be hard-coded
 * into a target-specific file and should never be modified at runtime.
 * The 'active' and 'addr' fields should be modified to reflect current
 * breakpoint usage.
 */
struct bpt{
	char type;	/* BRK_DATA, BRK_INST, BRK_EOT			*/
	char active;	/* True if this breakpoint is in use		*/
	int *addr;	/* If active==TRUE, address at which bpt is placed */
};

#define BRK_DATA	0	/* Data breakpoint			*/
#define BRK_INST	1	/* Instruction breakpoint		*/
#define BRK_EOT		2	/* Dummy used to delimit end of bpt table */

extern struct bpt bptable[];

extern char have_data_bpts;	/* TRUE if there are one or more data breakpoint
				 * entries in the table,
				 */
