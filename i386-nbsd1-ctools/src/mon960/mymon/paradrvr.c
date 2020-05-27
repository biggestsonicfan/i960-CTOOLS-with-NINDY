/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1993, 1994, 1995 Intel Corporation
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
/*
 * This module contains target-specific parallel routines for the retargetable
 * DB960 monitor.  This implementation is for the EV960SX and EP960CX board.
 */

#include "retarget.h"
#include "hdi_errs.h"
#include "dbg_mon.h"
#include "mon960.h"
#include "this_hw.h"

#define pp_data   (volatile unsigned char *)PP_DATA_ADDR
#define pp_stat   (volatile unsigned char *)PP_STAT_ADDR
#define pp_cont   (volatile unsigned char *)PP_CTRL_ADDR
/* timeout at cpu speed * 1000000 / 5 = 5 seconds approximately */
#define PARA_TIMEOUT_FACTOR 200000

void
parallel_err(void)
{
	*pp_cont = 0;
}


int 
parallel_init(void)
{
	int val;
	
	*pp_cont = 0;
	*pp_cont = PP_INIT_BITS;
    val = *pp_data;
	val = *pp_stat;

	return OK;
}


int
parallel_read(register unsigned char *data,
              register unsigned int  size,
              unsigned short         *crc,
              unsigned int           partial_transfer,
              unsigned int           not_used)
{
	register unsigned short sum;
    register unsigned int timeout;
#if BIG_ENDIAN_CODE
    unsigned char * data_start = data;
    unsigned int data_size = size;
#endif   

    if (crc != NULL)
        sum = *crc;
	else
		sum = 0;

#if defined(PP_NO_ACK_STROBE)
    /*
     * Some parallel HW implementations do not automatically strobe the ACK
     * bit (for example: evsx and ApLink targets).  Set ACK high to begin
     * the read sequence.  This action is required for those Unix hosts
     * with parallel drivers that use an ACK handshake.
     */
    *pp_cont = PP_INIT_BITS | PP_ACK_BIT;
#endif

    while (size-- > 0)
        {
        timeout = __cpu_speed * PARA_TIMEOUT_FACTOR;
        while (((*pp_stat & PP_READ_STATUS) != PP_READ_STATUS) && !break_flag)
            if (timeout-- == 0)
                {
                cmd_stat = E_PARA_DNLOAD_TIMO;
                return ERR;
                } 
#if defined(SECOND_CHECK_PP_READ_STATUS) 
        while (((*pp_stat & PP_READ_STATUS) != PP_READ_STATUS) && !break_flag)
            if (timeout-- == 0)
                {
                cmd_stat = E_PARA_DNLOAD_TIMO;
                return ERR;
                } 
#endif

        *data = *pp_data;

#if defined(PP_NO_ACK_STROBE)
        /* Toggle the ACK bit -- necessary for some Unix parallel drivers. */
        *pp_cont = PP_INIT_BITS;
        *pp_cont = PP_INIT_BITS | PP_ACK_BIT;
#endif

        sum = CRCtab[(sum^(*data))&0xff] ^ sum >> 8;
        data++;
        }

	if (break_flag)
		return ERR; 

    if (crc != NULL)
        *crc = sum;

#if BIG_ENDIAN_CODE
    /* Swap data for fastdown.c */
    if (data_size > 4)
        swap_data_byte_order(1, data_start, data_size); 
    else if (data_size == 4) 
        swap_data_byte_order(4, data_start, 4); 
    else if (data_size == 2) 
        swap_data_byte_order(2, data_start, 2); 
#endif 

	return OK;
}



void
parallel_download(const void *p_cmd)
{
    fast_download_cfg(parallel_init, parallel_err, parallel_read);
    fast_download(p_cmd, PARALLEL_DOWNLOAD);
}
 


int
para_comm_supported(void)
{
    return (FALSE);
}


int para_dnload_supported(void)
{
    return (TRUE);
}
