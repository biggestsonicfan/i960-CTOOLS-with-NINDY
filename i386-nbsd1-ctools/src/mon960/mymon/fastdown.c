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

#include <string.h>

#include "retarget.h"
#include "mon960.h"
#include "dbg_mon.h"
#include "hdi_errs.h"
#include "hdi_com.h"
#include "cyc9060.h"

/* fast_download driver definitions */
static void (*fast_download_err)();
static int  (*fast_download_init)();
static int  (*fast_download_read)(unsigned char *data, unsigned int size, unsigned short *crc, unsigned int partial_transfer, unsigned int timeout);

#define PUTBUF(msg) com_put_msg((const unsigned char *)&(msg), sizeof(msg))

/*****************************************************************************/
/* FUNCTION NAME: fast_download_download                                         */
/*****************************************************************************/
static void
fast_download_error(int led_val, int error, int fast_download_cmd)
{
    CMD_TMPL response;

        fast_download_err();
        blink(led_val); blink(led_val); blink(led_val);
        response.cmd = fast_download_cmd;
        if (error != 0)
            response.stat = error;
        else
            response.stat = E_FAST_DOWNLOAD_BAD_FORMAT;
        PUTBUF(response);
}
  

/*****************************************************************************/
/* FUNCTION NAME: PCI_VERIFY                                         */
/* Verifies a direct download with a simple 32 bit CRC check */
/*****************************************************************************/
static int
pci_verify(int fast_download_cmd)
{
    unsigned long long_crc = 0, long_msg_crc, *data_ptr;
    unsigned int data_size, i;
    unsigned char format_byte;
        
    if ((fast_download_read((unsigned char *)&data_ptr, 4, NULL, FALSE, 0) == ERR) || 
        (fast_download_read((unsigned char *)&data_size, 4, NULL, FALSE, 0) == ERR) ||
        (fast_download_read((unsigned char *)&long_msg_crc, 4, NULL, FALSE, 0) == ERR) ||
        (fast_download_read(&format_byte, 1, NULL, FALSE, 0) == ERR) ||
        (format_byte != PCI_VERIFY))
        {
        fast_download_error(10, cmd_stat, fast_download_cmd);
        return ERR;
        }

#if BIG_ENDIAN_CODE
    /* 960 BE versus PC LE requires a swap to get the same 4 word result. */
    swap_data_byte_order(4, &data_ptr, 4);
    swap_data_byte_order(4, &data_size, 4);
    /* 960 BE versus PC LE requires a swap to get the same CRC result. */
    swap_data_byte_order(4, &long_crc, 4);
#endif
    
    for (i = 0; i < data_size; i = i+4)
        long_crc ^= *data_ptr++;

    if (long_crc != long_msg_crc)
        {
        fast_download_error(2, E_FAST_DOWNLOAD_BAD_DATA_CHECKSUM, fast_download_cmd);
        return ERR;
        }

    return OK;
}


/*****************************************************************************/
/* FUNCTION NAME: FAST_DOWNLOAD                                         */
/*****************************************************************************/
void
fast_download(const void *p_cmd, int fast_download_cmd)
{
#define FAST_DOWNLOAD_DELIMITER 0xff
#define DATA_BUF_SIZE 400
    unsigned char data[DATA_BUF_SIZE];

    ADDR addr;
    unsigned char byte_1, byte_2, format_byte, *rp = (unsigned char *)p_cmd + 2;
    unsigned int addr_value, mem_size, pattern_size, eeprom_mem,
                 write_or_fill_cmd, data_size;
    unsigned short our_crc, msg_crc;
    CMD_TMPL response;

    response.cmd = fast_download_cmd;
    /* init fast_download port */
    if (fast_download_init() != OK)
        {
        if (fast_download_cmd == PARALLEL_DOWNLOAD)
            response.stat = E_PARALLEL_DOWNLOAD_NOT_SUPPORTED;
        else
            response.stat = E_PCI_COMM_NOT_SUPPORTED;
        PUTBUF(response);
        return;
        }

    response.stat = OK;
    PUTBUF(response);

    /* just to see if we support fast_download */
    format_byte = get_byte(rp);
    if (format_byte == 0)
      return;

  leds(8,8);
  do 
  {  /* read fast_port until next fast_download_cmd message */
    format_byte = our_crc = mem_size = pattern_size = 0;

    if (fast_download_read(&format_byte, 1, NULL, FALSE, 0) == ERR)
        {
        fast_download_error(10, cmd_stat, fast_download_cmd);
        return;
        }

    if (format_byte == fast_download_cmd)
        {
        /* end of fast download */
        PUTBUF(response);
        leds(0,8);
        return;
        }
    if (format_byte == PCI_VERIFY)
        {
        if (pci_verify(fast_download_cmd) == ERR)
            return;
        continue;
        }

    if ((format_byte != WRITE_MEM) && (format_byte != FILL_MEM))
        {
        fast_download_error(10, 0, fast_download_cmd);
        return;
        }

    write_or_fill_cmd = format_byte;

    if ((fast_download_read((unsigned char *)&addr, 4, &our_crc, FALSE, 0) == ERR) ||
        (fast_download_read((unsigned char *)&data_size, 4, &our_crc, FALSE, 0) == ERR) ||
        (fast_download_read((unsigned char *)&byte_1, 1, &our_crc, FALSE, 0) == ERR) ||
        (fast_download_read((unsigned char *)&byte_2, 1, &our_crc, FALSE, 0) == ERR) ||
        (fast_download_read(&format_byte, 1, NULL, FALSE, 0) == ERR))
        {
        fast_download_error(10, cmd_stat, fast_download_cmd);
        return;
        }

    mem_size = (int)byte_1;
    pattern_size = (int)byte_2;
    if (format_byte != FAST_DOWNLOAD_DELIMITER)
        {
        fast_download_error(7, 0, fast_download_cmd);
        return;
        }

#if BIG_ENDIAN_CODE
    /* 960 BE versus PC LE requires a swap to get the same 4 word result. */
    if (fast_download_cmd == PCI_DOWNLOAD)
        {
        swap_data_byte_order(4, &addr, 4);
        swap_data_byte_order(4, &data_size, 4);
        }
#endif

    /* now read the data  */
    if (write_or_fill_cmd == FILL_MEM)
        {
        if (fast_download_read(data, pattern_size, &our_crc, FALSE, 0) == ERR)
            {
            fast_download_error(6, cmd_stat, fast_download_cmd);
            return;
            }
        }
    else if (((eeprom_mem = is_eeprom(addr, (unsigned long)data_size)) == TRUE) || (mem_size > 1))
        { /* eeprom or memory blocks */
        unsigned int i, partial_transfer = TRUE, increment = DATA_BUF_SIZE;

        if ((eeprom_mem == TRUE) && (check_eeprom(addr, (unsigned long)data_size) != OK))
            {
            fast_download_error(5, cmd_stat, fast_download_cmd);
            return;
            }

        for (i = 0; i < data_size; i += DATA_BUF_SIZE)
            {

            /* last partial read */
            if (i + increment >= data_size)
                {
                increment = data_size - i;
                partial_transfer = FALSE;        
                }

            if (fast_download_read(data, increment, &our_crc, partial_transfer, 0) == ERR)
                {
                fast_download_error(5, cmd_stat, fast_download_cmd);
                return;
                }
            /* we verify memory with CRC check */
            if (store_mem((ADDR)(addr + (ADDR)i), mem_size, data, increment, FALSE) != OK)
                {
                fast_download_error(5, cmd_stat, fast_download_cmd);
                return;
                }
            }
        }
    else if (eeprom_mem == FALSE)
        {    /* normal memory */
        /* test if writable memory */
        *(unsigned int *)addr = 0;
        addr_value = *(unsigned int *)addr;
        *(unsigned int *)addr = 0xa5a5a5a5;
        if (*(unsigned int *)addr != 0xa5a5a5a5 || addr_value != 0)
            {
            fast_download_error(4, E_WRITE_ERR, fast_download_cmd);
            return;
            }
        if (fast_download_read((unsigned char *)addr, data_size, &our_crc, FALSE, 0) == ERR)
            {
            fast_download_error(4, cmd_stat, fast_download_cmd);
            return;
            }
        }
    else 
        { /* Partial eeprom - not allowd */
        fast_download_error(3, E_EEPROM_ADDR, fast_download_cmd);
        return;
        }

    if (fast_download_read((unsigned char *)&msg_crc, 2, NULL, FALSE, 0) == ERR)
        {
        fast_download_error(2, cmd_stat, fast_download_cmd);
        return;
        }
    if (our_crc != msg_crc)
        {
        fast_download_error(2, E_FAST_DOWNLOAD_BAD_DATA_CHECKSUM, fast_download_cmd);
        return;
        }
    if (fast_download_read(&format_byte, 1, NULL, FALSE, 0) == ERR)
        {
        fast_download_error(2, cmd_stat, fast_download_cmd);
        return;
        }
    if (format_byte != FAST_DOWNLOAD_DELIMITER)
        {
        fast_download_error(2, 0, fast_download_cmd);
        return;
        }

    if (write_or_fill_cmd == FILL_MEM)
        {
        if (fill_mem(addr, mem_size, data, pattern_size, data_size) != OK)
            {
            fast_download_error(1, cmd_stat, fast_download_cmd);
            return;
            }
        }

  } while (TRUE);
}


void
fast_download_cfg(FAST_INIT_FNPTR init, VOID_FNPTR err, FAST_RD_FNPTR read)
{
    fast_download_init = init;
    fast_download_err  = err;
    fast_download_read = read;
}
