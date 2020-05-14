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
#include "stop.h"

/* The following two bytes indicate the reason why the user program
 * last stopped.
 */
char stop_exit;	/* TRUE => user program exited;
		 * FALSE => user program fault, trace, or srq
		 */
char stop_code;	/* Meaning depends on value of stop_exit.
		 * If stop_exit == TRUE, this is the user exit code.
		 * If stop_exit == FALSE,
		 *	Faults:
		 *		0x00: parallel
		 *		0x01: unknown (trace faults have separate
		 *			numbers, see below)
		 *		0x02: operation
		 *		0x03: arithmetic
		 *		0x04: floating point
		 *		0x05: constraint
		 *		0x06: virtual memory
		 *		0x07: protection
		 *		0x08: machine
		 *		0x09: structural
		 *		0x0a: type
		 *		0x0b: [ reserved ]
		 *		0x0c: process
		 *		0x0d: descriptor
		 *		0x0e: event
		 *		0x0f: [ reserved ]
		 *	Traces:
		 *		0x10: Single-step
		 *		0x11: Branch
		 *		0x12: Call
		 *		0x13: Return
		 *		0x14: Pre-return
		 *		0x15: Supervisor Call
		 *		0x16: Breakpoint
		 *	Other:
		 *		0xfe: application service request (srq)
		 *		0xff: Software breakpoint (set by gdb)
		 */

struct fault_data {
	unsigned reserved;
	unsigned override[3];
	unsigned fdata[3];
	unsigned override_data;
	unsigned pc;
	unsigned ac;
	unsigned int	fsubtype:8,
			freserved:8,
			ftype:8,
			fflags:8;
	unsigned int *faddress;
};

void fault_handler();
static void tell_fault_type();
static void trace_fault_handler();

/*****************************************/
/* Fault Handler Routine	 	 */
/* 					 */
/* This is the C-level fault handler.	 */
/* It determines whether or not the	 */
/* fault is a trace fault and dispatches */
/* it appropriately			 */
/*****************************************/
void
fault_handler(fault_ptr)
struct fault_data *fault_ptr;	/* pointer to fault record on stack */
{
	if  ( fault_ptr->ftype == 1 ){
		trace_fault_handler( fault_ptr );
	} else {
		tell_fault_type(fault_ptr);
		monitor();
	}
}

/*****************************************/
/* Tell fault type			 */
/* 					 */
/*****************************************/
static void
tell_fault_type( fault_ptr )
struct fault_data *fault_ptr;	/* pointer to fault record on stack */
{
	unsigned int type;
	static char *fault_names[] = {
		"Parallel",		/* Type 0 */
		"Trace",		/* Type 1 */
		"Operation",		/* Type 2 */
		"Arithmetic",		/* Type 3 */
		"Floating Point",	/* Type 4 */
		"Constraint",		/* Type 5 */
		"Virtual Memory",	/* Type 6 */
		"Protection",		/* Type 7 */
		"Machine",		/* Type 8 */
		"Structural",		/* Type 9 */
		"Type",			/* Type a */
		"Reserved (0xb)",	/* Type b */
		"Process",		/* Type c */
		"Descriptor",		/* Type d */
		"Event",		/* Type e */
		"Reserved (0xf)"	/* Type f */
	};

	type = fault_ptr->ftype;

	/* Determine (and store for later query by GDB) the
	 * reason why program stopped.
	 */
	stop_exit = FALSE;
	if ( (0 <= type) && (type <= LAST_FAULT) ){
		stop_code = type;
	} else {
		stop_code = FAULT_UNKNOWN;
	}

	if (gdb) {
		co (DLE);    /* Let GDB know user program is stopped */
		return;
	}

	if ( (0 <= type) && (type <= LAST_FAULT) ){
		prtf( " %s fault at: ", fault_names[type] );
	} else {
		prtf(" Unknown fault of type %B at: ", type );
	}

	prtf( "%X\n", fault_ptr->faddress );
	prtf(" Fault record is on stack at %X: \n", fault_ptr );
	prtf(" fdata : %X %X %X\n",
		fault_ptr->fdata[0], fault_ptr->fdata[1], fault_ptr->fdata[2] );
	prtf(" fsubtype %B:     fflags : %B\n",
			fault_ptr->fsubtype, (unsigned int)fault_ptr->fflags );
}

/****************************************/
/* Trace Fault Handler Routine 		*/
/* 					*/
/* THIS CODE ASSUMES THAT:		*/
/*  hardware breakpoints are set by	*/
/*	nindy only in response to the	*/
/*	'br' command.			*/
/*  an 'fmark' instruction followed by	*/
/*	a magic word is the way the user*/
/*	program exits.			*/
/*  any other 'fmark' instruction was	*/
/*	written into memory by GDB,	*/
/*	and is a GDB breakpoint.	*/
/****************************************/
static void
trace_fault_handler(fault_ptr)
struct fault_data *fault_ptr;
{
	unsigned int *inst_ptr;
	int mask;	/* Masks single bit in fault subtype byte */
	int bitnum;	/* Number of bit currently masked by 'mask' */
	unsigned int subtype;



	if (gdb) {
		co (DLE);  /* Let GDB know user program is stopped */
	}

	inst_ptr = fault_ptr->faddress;
	subtype = fault_ptr->fsubtype;

	prtf("\n");

	if ((inst_ptr[0] == 0x66003e00)
	&&  (inst_ptr[1] == 0x66003f80)
	&&  (inst_ptr[2] == MAGIC_BREAK))
	{
		/* USER EXIT:
		 *	Store exit code for later query by GDB.
		 *	Set user_is_running flag;  on return to the
		 *	assembler level code, this should cause the
		 *	application to be terminated and the stack
		 *	fixed up.
		 */
		stop_code = register_set[REG_G0];
		stop_exit = TRUE;
		user_is_running = FALSE;
		if ( stop_code != 0 ){
			prtf("\n Program Exit: %x\n", stop_code );
		}
		return;

	 } else {		/* REAL BREAKPOINT */

		prtf(" *** Breakpoint at :%X\n", fault_ptr->faddress );
		prtf("  Type(s) : " );
		if (subtype & 0x2 ) prtf(" Single-step");
		if (subtype & 0x4 ) prtf(" Branch");
		if (subtype & 0x8 ) prtf(" Call");
		if (subtype & 0x10) prtf(" Return");
		if (subtype & 0x20) prtf(" Pre-return");
		if (subtype & 0x40) prtf(" Supervisor Call");
		if (subtype & 0x80) prtf(" Breakpoint");
		prtf("\n");
		dasm( 0, 0, 0, 0 );

		stop_exit = FALSE;
		if (*inst_ptr == 0x66003e00) {

			/* FMARK INSTRUCTION:
			 *	Store reason why program stopped (GDB
			 *	breakpoint), for later query by GDB.
			 *	GDB will replace the fmark with the
			 *	actual instruction at the breakpoint.
			 */
			stop_code = STOP_GDB_BPT;

		} else {

			/* Determine why program stopped */
			mask = 0x02;
			for (bitnum=1; bitnum<=7; bitnum++, mask <<=1){
				if (subtype & mask) {
					break;
				}
			}

			/* Store reason for later query by GDB */
			stop_code = LAST_FAULT + bitnum;

			/* Leave ip pointing at next instruction that will
			 * be executed:  trace fault leaves ip pointing
			 * at the instruction that just completed, but rip
			 * points to the next instruction, so copy it to ip.
			 */
			register_set[REG_IP] = register_set[REG_RIP];
		}
		monitor();
	}
}
