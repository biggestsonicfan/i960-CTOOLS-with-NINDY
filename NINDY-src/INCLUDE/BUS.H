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
/***********************************************************************
****                                                                ****
****    bus.h   header file for 80960CA bus controller              ****
****                                                                ****
***********************************************************************/

#define BURST_ENABLE    0x1
#define READY_ENABLE    0x2
#define PIPELINE_ENABLE 0x4

#define BUS_WIDTH_8     0x0
#define BUS_WIDTH_16    (0x1 << 19)
#define BUS_WIDTH_32    (0x2 << 19)

#define BIG_ENDIAN      (0x1 << 22)

#define NRAD(WS)    (WS << 3)   /* WS can be 0-31   */
#define NRDD(WS)    (WS << 8)   /* WS can be 0-3    */
#define NXDA(WS)    (WS << 10)  /* WS can be 0-3    */
#define NWAD(WS)    (WS << 12)  /* WS can be 0-31   */
#define NWDD(WS)    (WS << 17)  /* WS can be 0-3    */

/* 
Perform a bit-wise OR of the desired parameters to specify a region.

 
    bus_region_1_config =
        BURST_ENABLE | BUS_WIDTH_32 | NRAD(3) | NRDD(1) |
        NXDA(1) | NWAD(2) | NWDD(2);

*/
