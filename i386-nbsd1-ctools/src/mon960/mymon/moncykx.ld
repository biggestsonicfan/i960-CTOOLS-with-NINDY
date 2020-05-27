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
/* Mon960 linker directives for EPCX board */

MEMORY
{
    rom:	o=0x00000000,l=0x3f800	/*Based on 28F020 (2 Mbit flash)*/
    rom_data:   o=0x0003f800,l=0x0800   /*Enough space must be reserved in rom*/
                                        /*after the text section to hold the  */
                                        /*initial values of the data section. */
    data:	o=0xa0000000,l=0x0800
    bss:	o=0xa0000800,l=0x7800
}

SECTIONS
{
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
#*move $0 .text 0
#*move $0
#*mkimage $0 $0.ima
#*ihex $0.ima $0.hex mode16
#*split $0.ima 262144 32 262144 8 out
#*ihex out.00 $0_u4.hex mode16
#*ihex out.01 $0_u3.hex mode16
#*ihex out.02 $0_u2.hex mode16
#*ihex out.03 $0_u1.hex mode16
#*map $0
#*quit
*/
