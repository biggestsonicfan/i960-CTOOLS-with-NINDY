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

/****************************************************************************
 * This file defines the storage areas for NINDY's copies of the
 * user's register set.
 *
 * WARNING!!!:
 *	If the storage is re-organized, or the indices are renumbered,
 *	be sure to reflect the changes in the following places, which
 *	make assumptions about the organization:
 *
 *	o gdb.c:  the sendregs() and recvregs() routines
 *
 *	o faultasm.s: the ".set" directives defining the assembler offsets
 *		into these arrays.
 *
 *****************************************************************************/

/*******************************************************
 *                                                     *
 * COPY OF USER'S LOCAL, GLOBAL, AND CONTROL REGISTERS *
 *                                                     *
 *******************************************************/

extern 	unsigned int register_set[];

/* Number of registers and number of bytes in each register
 */
#define NUM_REGS	36
#define REG_SIZE	4

/* Index of each register in 'register_set[]'
 */
#define REG_R0	0
#define REG_PFP	REG_R0
#define REG_R1	1
#define REG_SP	REG_R1
#define REG_R2	2
#define REG_RIP	REG_R2
#define REG_R3	3
#define REG_R4	4
#define REG_R5	5
#define REG_R6	6
#define REG_R7	7
#define REG_R8	8
#define REG_R9	9
#define REG_R10	10
#define REG_R11	11
#define REG_R12	12
#define REG_R13	13
#define REG_R14	14
#define REG_R15	15
#define REG_G0	16
#define REG_G1	17
#define REG_G2	18
#define REG_G3	19
#define REG_G4	20
#define REG_G5	21
#define REG_G6	22
#define REG_G7	23
#define REG_G8	24
#define REG_G9	25
#define REG_G10	26
#define REG_G11	27
#define REG_G12	28
#define REG_G13	29
#define REG_G14	30
#define REG_G15	31
#define REG_FP	REG_G15
#define REG_PC	32
#define REG_AC	33
#define REG_IP	34
#define REG_TC	35


/*******************************************
 *                                         *
 * COPY OF USER'S FLOATING POINT REGISTERS *
 *                                         *
 *******************************************/

extern  double	fp_register_set[];	/* fp register set */

/* Number of registers and number of bytes in each register
 */
#define NUM_FP_REGS	4
#define FP_REG_SIZE	8

/* Index of each register in 'fp_register_set[]'
 */
#define REG_FP0	0
#define REG_FP1	1
#define REG_FP2	2
#define REG_FP3	3
