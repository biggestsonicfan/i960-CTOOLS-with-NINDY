/******************************************************************/
/* 		Copyright (c) 1990, Intel Corporation

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



/***************************************************************************
 * NINDY memory map
 *	For each board to which NINDY has been ported, the NINDY memory map
 *	is hardwired into the appropriate "*data.c" file.  If modifications
 *	are made to the board, or if the board has options that can result
 *	in a different memory map, it may be necessary to modify the memory
 *	map and rebuild the NINDY ROM for the board in question.
 *
 *	At present, the memory map is only used by the gdb interface;  a
 *	debugger running on a remote host can request the memory map so
 *	that it can decide where to use hard breakpoints instead of soft,
 *	where to disallow memory accesses, etc.
 ***************************************************************************/


/******** A single entry in the memory map *******/

struct memseg {
	char type;		/* Type of this segment of memory	*/
	unsigned long begin;	/* Beginning of this segment of memory	*/
	unsigned long end;	/* End of this segment of memory	*/
};

/******** Legal values of 'type' field *******/

#define SEG_EOM		'\0'
#define SEG_ROM		'0'
#define SEG_RAM		'1'
#define SEG_FLASH	'2'
#define SEG_HW		'3'


/******** Return TRUE iff memseg 'm' is the end-of-map delimiter *******/

#define END_OF_MAP(m)	(m.type == SEG_EOM)


/********
 *
 * The map itself is in the '*data.c' file for the board in question.
 *
 * The map should appear sorted by in order of increasing beginning
 * address.  Discontinuities in the memory map indicate address ranges
 * that are not decoded into anything meaningful.
 *
 * The map must be terminated by an entry of type SEG_EOM.
 *
 ******/

extern struct memseg memmap[];
