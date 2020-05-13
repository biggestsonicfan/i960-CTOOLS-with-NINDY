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
/* assignment statements for the 960 tests */

#define SRAM_ADDRESS  0x08000000       /* beginning SRAM space */
#define END_ADDRESS    0x081fffff      /* ending usable SRAM space */

#define FIRST_CYCLE    0x2800000c      /* first cycle register */
#define BURST_CYCLE    0x28000008      /* burst cycle register */
#define WRITE_STRETCH  0x28000010      /* write-stretch register */

#define RESET_CSR      0x28000014      /* reset register */
#define TEST_POINT     0x28000018      /* test point for factory test */

#define SERIAL_START   0x20000000      /* serial port address */

#define OOBA_ADDR      0x38000000      /* illegal address for OOBA */

#define FILLER         0xffffffff      /* arbitrary # to fill SRAM */
#define ALTERNATING_1  0x5a5a5a5a      /* alternating pattern for */
#define ALTERNATING_2  0xa5a5a5a5      /* checking SRAM */

#define USER_LED01     0x28000000      /* user LED 0-1 addresses */
#define USER_LED23     0x28000004      /* user LED 2-3 addresses */

#define BOARD_CONFIG   0x0000002c      /* ROM configuration info */
#define OFF_WAIT_STATE 0x18010071      /* changes the wait states for */
				       /* outside memory activities */

#define EPROM_START    0x00000000      /* beginning EPROM address */
#define EPROM_SIZE     0x00000020      /* EPROM size value address */
#define CHECKSUM       0x00000009      /* location of EPROM checksum */

#define FLASH_ADDRESS  0x10000000      /* start of flash memory */

/* global variable for number of SRAM bytes installed */
int sram_bytes;
