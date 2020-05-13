/******************************************************************/
/* 		Copyright (c) 1989, 1990 Intel Corporation

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

	/*********************************************************
	 *                                                       *
	 *  LOWEST-LEVEL SUPPORT FOR MEMORY ALLOCATION ROUTINES  *
	 *                                                       *
	 *********************************************************/


extern char *stack_start;  /* address label from linker */
extern int end;		/* linker reserved word - end of .bss */

/* Following is the current break address (the high address of the
 * heap).
 *
 * This variable should NOT be initialized:  that places it in the
 * .data, which does not necessarily get re-initialized before the
 * program is run.  If the program is run out of RAM several times
 * in succession without re-downloading, the break address will
 * creep steadily up until mallocs fail.
 *
 * However, crtnindy.o guarantees that .bss is always zeroed before
 * the user program is run.  So the brk() and sbrk() routines check
 * this value;  if it's 0, they assume it's the first call and
 * initialize it to '&end', the end of the .bss segment.
 */

static char *current_brk;

#define ERROR -1

int
brk(endds)
char *endds;
{
	if ( (endds < (char *)&end) || (endds >= stack_start) ){
		return ERROR;
	}
	current_brk = endds;
	return 0;
}


char *
sbrk(incr)
int incr;
{
	char *new_brk, *old_brk;
	char *junk;

	if ( current_brk == 0 ){
		/* First call: initialize break address */
		current_brk = (char *) &end;
	}

	new_brk = current_brk + incr;

	if ( (new_brk < (char *)&end) || (new_brk >= stack_start) ){
		return (char *) ERROR;
	}

	for ( junk = current_brk; junk < new_brk; junk++ ){
		*junk = 0;	/* zero memory */
	}
	old_brk = current_brk;
	current_brk = new_brk;
	return old_brk;
}
