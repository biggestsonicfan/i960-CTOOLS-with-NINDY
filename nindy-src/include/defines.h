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
/************************************************/
/* For an explanation of "GDB mode", see the 	*/
/* description of the 'gdb' flag in globals.h. 	*/
/************************************************/

#define DLE	'\020'
			/* GDB commands start with this character.
			 * And NINDY informs GDB that the user program
			 * has stopped running by sending this character to GDB.
			 */

#define	FALSE		0	/* standard */
#define	TRUE		1
#define	ERROR		-1

#define	FLASH		0	/* Flash download */
#define	RAM		1	/* RAM download */

#define	NOFLASH		1	/* Flash not present */
#define	ZEROFLASH	2	/* Flash zero error */
#define	ERASEFLASH	3	/* Flash erase error */

#define	LINELEN		40	/* length of input line */

#define	MAXDIGITS 	10	/* max num of digits in int */

/* ASCII characters
 */
#define	NUL		000	/* null */
#define	SOH  		001 	/* start of header */
#define	EOT		004	/* end of transmission */
#define	ACK  		006	/* acnowledge */
#define	BEL  		007	/* bell */
#define	TAB		011	/* tab */
#define	NAK  		025	/* no acnowledge */
#define	CAN		030	/* cancel */
#define	ESC		033	/* Escape */
#define	DEL		177	/* delete */
#define	CTRL_P		020	/* Control P */
#define	XON		021	/* flow control on */
#define	XOFF		023	/* flow control off */


#define	BYTE		8	/* 8 bits */
#define	SHORT		16	/* 16 bits */
#define	INT		32	/* 32 bits */
#define	LONG		64	/* 64 bits */
#define	TRIPLE		96	/* 96 bits */
#define	EXTENDED	80	/* 80 bits */
#define	QUAD		128	/* 128 bits */

#define TIMEOUT  	-1
#define MAXSECTS	12  	/* max sections allowed in coff file */

/* define for break from fmark to monitor */
#define	MAGIC_BREAK	0xfeedface

#define	NULL	0
#define	MAXARGS	5

/* Flags that can get passed to parse_arg() routine.
 * Must be separate bits (OR-able together).
 */
#define ARG_CNT  1	/* If set, count portion of argument is required */
#define ARG_ADDR 2	/* If set, address portion of argument is required */
