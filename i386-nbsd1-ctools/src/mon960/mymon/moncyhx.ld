/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1993, 1994 Intel Corporation
 *
 * Intel hereby grants you permission to copy, modify, and distribute this
 * software and its documentation.  Intel grants this permission provided
 * that the above copyright notice appears in all copies and that both the
 * copyright notice and this permission notice appear in supporting
 * documentation.  In addition, Intel grants this permission provided that
 * you prominently mark as "not part of the original" any modifications
 * made to this software or documentation, and that the name of Intel
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software or the documentation without specific,
 * written prior permission.
 *
 * Intel Corporation provides this AS IS, WITHOUT ANY WARRANTY, EXPRESS OR
 * IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY
 * OR FITNESS FOR A PARTICULAR PURPOSE.  Intel makes no guarantee or
 * representations regarding the use of, or the results of the use of,
 * the software and documentation in terms of correctness, accuracy,
 * reliability, currentness, or otherwise; and you rely on the software,
 * documentation and results solely at your own risk.
 *
 * IN NO EVENT SHALL INTEL BE LIABLE FOR ANY LOSS OF USE, LOSS OF BUSINESS,
 * LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES
 * OF ANY KIND.  IN NO EVENT SHALL INTEL'S TOTAL LIABILITY EXCEED THE SUM
 * PAID TO INTEL FOR THE PRODUCT LICENSED HEREUNDER.
 *
 ******************************************************************************/
/*)ce*/
/* Mon960 linker directives for C145/146-HX board */

/* rom must start at 0xf0040000 to ensure that if the same image was
   blown into U27 (Expansion Flash #2), the system would boot if the
   "romswap" switch was thrown */

MEMORY
{
    rom:	o=0xf0040000,l=0x3f800	/*Based on 28F020 (2 Mbit flash)*/
    rom_data:   o=0xf007f800,l=0x0700   /*Enough space must be reserved in rom*/
                                        /*after the text section to hold the  */
                                        /*initial values of the data section. */
    ibr:	o=0xf007ff30,l=0x00cf
    data:	o=0xa0000000,l=0x0700
    bss:	o=0xa0000700,l=0x7900
}

SECTIONS
{
	.ibr :
	{
		cyhxhibr.o
	} > ibr

	.text :
	{
        } > rom

	.data :
	{
        } > data

	.bss :
	{
        } > bss
}

pre_init = init_mem;         /* size and clear memory before initialization */
post_init = fix_ctrl_table;
initial_stack = _monitor_stack;
rom_data = __Etext;
_checksum = -(_rom_prcb + _start_ip);

HLL()


/*
        Rommer script embedded here:
 
Ibr offset must be (rom_length - 0x100).  For 64K roms (27C512), use 0xff00.
For 128K roms (27C010), use 0x1ff00
For 256K roms (28F020), use 0x3ff00
For 512K roms (27C040), use 0x7ff00
 
#*move $0 .text 0
#*move $0
#*move $0 .ibr 0x3ff30
#*mkimage $0 $0.ima
#*ihex $0.ima $0.hex mode16
#*move $0.fls .text 0xe0040000
#*move $0.fls
#*move $0.fls .ibr 0xe007ff30
#*map $0
#*quit
*/
