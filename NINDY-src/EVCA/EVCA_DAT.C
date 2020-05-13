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

/************  EV80960CA BOARD-SPECIFIC CONSTANTS AND DATA  *****************/

#include "memmap.h"


/*********************************************************************
 * Number of iterations of loop in readbyte() routine (io.c) that 
 * will consume approximately one second of real time.
 *********************************************************************/
int readbyte_ticks_per_second = 96000;

/*********************************************************************
 * board identifier
 *********************************************************************/
char boardname[] = "EV80960CA";
char architecture[] = "CA";	/* Processor type */


/*********************************************************************
 * Memory map
 *********************************************************************/
struct memseg memmap[] = {
	{ SEG_RAM, 0x00000000, 0x000003ff },
	{ SEG_HW,  0x00000400, 0xafffffff },
	{ SEG_RAM, 0xb0000000, 0xb000ffff },
	{ SEG_HW,  0xc0000000, 0xdfffffff },
	{ SEG_RAM, 0xe0000000, 0xe000ffff },
	{ SEG_ROM, 0xffff0000, 0xffffffff },
	{ SEG_EOM, 0,          0 },
};
