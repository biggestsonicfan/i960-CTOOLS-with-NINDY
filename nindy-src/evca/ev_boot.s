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
/***********************************************************************
****                                                                ****
****    THE INITIALIZATION BOOT RECORD                              ****
****    MUST BE LOACATED AT ADDRESS                                 ****
****    0xFFFFFF00 BY THE LINKER!                                   ****
****                                                                ****
****    Sets up an Initialization Boot Record for the 80960CA       ****
****            ----->  USE THE C PRE-PROCESSOR                     ****
****                                                                ****
***********************************************************************/

#include "bus.h"
#define BYTE_0(data)  (data & 0x000000FF)
#define BYTE_1(data)  ((data & 0x0000FF00) >> 8)
#define BYTE_2(data)  ((data & 0x00FF0000) >> 16)
#define BYTE_3(data)  ((data & 0xFF000000) >> 24)

#define EPROM	(BUS_WIDTH_8 | NRAD(20) | NRDD(0) | NXDA(1) | \
                 NWAD(20) | NWDD(0))

#define ROM960 	( BURST_ENABLE | BUS_WIDTH_32 | NRAD(2) | \
		  NRDD(0) | NXDA(0) | NWAD(1) | NWDD(1))

#define PSRAM 	(PIPELINE_ENABLE | BURST_ENABLE | BUS_WIDTH_32 | NRAD(0) | \
		  NRDD(0) | NXDA(0) | NWAD(1) | NWDD(1))

#define BDRAM 	(READY_ENABLE | BURST_ENABLE | BUS_WIDTH_32 )

#define I_O 	( BUS_WIDTH_8| NRAD(20) | NXDA(2) | NWAD(20))

#define SBX_0 	( BUS_WIDTH_8| NRAD(30) | NXDA(3) | NWAD(30))

#define BUS_CONFIG EPROM /* initial memory configuration */

#define  REGION_0_CONFIG   EPROM
#define  REGION_1_CONFIG   BUS_CONFIG
#define  REGION_2_CONFIG   BUS_CONFIG
#define  REGION_3_CONFIG   ROM960
#define  REGION_4_CONFIG   BUS_CONFIG
#define  REGION_5_CONFIG   ROM960
#define  REGION_6_CONFIG   BUS_CONFIG
#define  REGION_7_CONFIG   BUS_CONFIG
#define  REGION_8_CONFIG   BUS_CONFIG
#define  REGION_9_CONFIG   ROM960 
#define  REGION_A_CONFIG   BUS_CONFIG
#define  REGION_B_CONFIG   PSRAM
#define  REGION_C_CONFIG   SBX_0
#define  REGION_D_CONFIG   I_O
#define  REGION_E_CONFIG   BDRAM
#define  REGION_F_CONFIG   EPROM

/*--------------------------------------------------------------------*/

	.globl	_rom_control_table
	.globl	rom_prcb
	.globl	cs6
	.globl	_cs6

        .text

_init_boot_record:
	.word	BYTE_0(BUS_CONFIG)
	.word	BYTE_1(BUS_CONFIG)
	.word	BYTE_2(BUS_CONFIG)
	.word	BYTE_3(BUS_CONFIG)

	.word	start_ip
	.word	rom_prcb
	.word	-2			/* Checksum word #1 */
	.word	0			/* Checksum word #2 */
	.word	0			/* Checksum word #3 */
	.word	0			/* Checksum word #4 */
	.word	0			/* Checksum word #5 */
_cs6:	.word	cs6			/* Checksum word #6:
					 *	-(start_ip+rom_prcb)
					 *	must be calculated at link time
					 */

	.align 4
rom_prcb:
	.word	fault_table		/* adr of fault table (ram)	*/
	.word	_rom_control_table	/* adr of control_table (rom)	*/
	.word	0x00001000		/* AC reg mask overflow fault	*/
	.word	0x40000001		/* Flt - Mask Unaligned fault	*/
	.word	_intr_table		/* Interrupt Table Address	*/
	.word	sys_proc_table		/* System Procedure Table	*/
	.word	0			/* Reserved			*/
	.word	_intr_stack		/* Interrupt Stack Pointer	*/
	.word	0x00000000		/* Inst. Cache - enable cache	*/
	.word	15			/* Reg. Cache - 15 sets cached	*/

	.align 4
_rom_control_table:
	/* -- Group 0 -- Breakpoint Registers */
	.word	0			/* IPB0 IP Breakpoint Reg 0 */
	.word	0			/* IPB1 IP Breakpoint Reg 1 */
	.word	0			/* DAB0 Data Adr Bkpt Reg 0 */
	.word	0			/* DAB1 Data Adr Bkpt Reg 1 */

	/* -- Group 1 -- Interrupt Map Registers */
	.word	0			/* IMAP0 Interrupt Map Reg 0 */
	.word	0			/* IMAP1 Interrupt Map Reg 1 */
	.word	0			/* IMAP2 Interrupt Map Reg 2 */
	.word	0x400			/* ICON Reg sets int mode */
					/*  initially disabled */

	/* -- Group 2-- Bus Configuration Registers */
	.word	REGION_0_CONFIG
	.word	REGION_1_CONFIG
	.word	REGION_2_CONFIG
	.word	REGION_3_CONFIG
 	/* -- Group 3 -- */
	.word	REGION_4_CONFIG
	.word	REGION_5_CONFIG
	.word	REGION_6_CONFIG
	.word	REGION_7_CONFIG
 	/* -- Group 4 -- */
	.word	REGION_8_CONFIG
	.word	REGION_9_CONFIG
	.word	REGION_A_CONFIG
	.word	REGION_B_CONFIG
	/* -- Group 5 -- */
	.word	REGION_C_CONFIG	
	.word	REGION_D_CONFIG
	.word	REGION_E_CONFIG
	.word	REGION_F_CONFIG

	/* -- Group 6 -- Breakpoint, Trace and Bus Control Registers */ 
	.word	0			/* Reserved */
	.word	0			/* BPCON Register */
	.word	0			/* Trace Controls  */
	.word	0x00000001		/* BCON Register  */

