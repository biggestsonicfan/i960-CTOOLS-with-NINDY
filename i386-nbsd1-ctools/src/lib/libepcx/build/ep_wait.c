
/*(cb*/
/**************************************************************************
 *
 *     Copyright (c) 1992 Intel Corporation.  All rights reserved.
 *
 *
 * Intel hereby grants you permission to copy, modify, and distribute this
 * software and its documentation.  Intel grants this permission provided
 * that the above copyright notice appears in all copies and that both the
 * copyright notice and this permission notice appear in supporting
 * documentation.  In addition, Intel grants this permission provided that
 * you prominently mark as not part of the original any modifications made
 * to this software or documentation, and that the name of Intel
 * Corporation not be used in advertising or publicity pertaining to the
 * software or the documentation without specific, written prior
 * permission.
 *
 * Intel provides this AS IS, WITHOUT ANY WARRANTY, INCLUDING THE WARRANTY
 * OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, and makes no
 * guarantee or representations regarding the use of, or the results of the
 * use of, the software and documentation in terms of correctness,
 * accuracy, reliability, currentness, or otherwise, and you rely on the
 * software, documentation, and results solely at your own risk.
 *
 **************************************************************************/
/*)ce*/


/* structure describing the region table entries in the
control table of the 960CA */

typedef struct region{
			unsigned int burst:1;
			unsigned int ready:1;
			unsigned int pipeline:1;
			unsigned int nrad:5;
			unsigned int nrdd:2;
			unsigned int nxda:2;
			unsigned int nwad:5;
			unsigned int nwdd:2;
			unsigned int reserved_2:1;
			unsigned int bus_width:2;
			unsigned int byte_order:1;
			unsigned int reserved:9;	
			} region_control;

extern unsigned int *get_prcb();

/********************************************************/
/* GET WAITSTATE					*/
/* 							*/
/* This routine retrieves the waitstate profile for a   */
/* specified region of memory from the current 80960CA  */
/* table.  It then loads the data from the control into */
/* a word and returns the word to the calling 		*/
/* subroutine. 						*/
/* This routine is designed to interface into the MON960*/
/* library to obtain the PRCB, which is provided through*/
/* a MON960 library call. 				*/
/********************************************************/
get_waitstate(region, reg_control)
unsigned int region;
region_control *reg_control;
{
unsigned int *prcb;
region_control *mem;

	prcb = get_prcb();
	mem = (region_control *)prcb[1];

	*reg_control =  mem[8+region];
}
	
/********************************************************/
/* SET WAITSTATE					*/
/* 							*/
/* This routine sets the waitstate profile for a 	*/
/* specified region of memory from the current 80960CA  */
/* table.  						*/
/* This routine is designed to interface into the NINDY */
/* library to obtain the PRCB, which is provided through*/
/* a NINDY library call.				*/
/********************************************************/
set_waitstate(region, reg_control)
unsigned int region;
region_control reg_control;
{
unsigned int *prcb;
region_control *mem;
unsigned int control_word;

	prcb = get_prcb();
	mem = (region_control *)prcb[1];

	mem[8+region] = reg_control;

	/* now time to request that the processor load the region table
		First, calculate the area to be loaded */

	control_word = 2 + region/4;

	/* next or in the command */

	control_word |= 0x400;

	/* now send the command */
	send_sysctl(control_word, 0,0);
}
