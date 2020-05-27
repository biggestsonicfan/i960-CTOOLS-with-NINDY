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
 * This module contains target-specific PCI routines.
 */

#include "retarget.h"
#include "dev.h"
#include "com.h"
#include "mon960.h"
#include "cyc9060.h"
#include "this_hw.h"
#include "hdi_com.h"
#include "hdi_errs.h"
#include "dbg_mon.h"

/* SX cpu can only read 16 bits use read_long and write_long from c145_hw.c */
#if SX_CPU
#define AND_WORD(pci_addr,val)   write_long(pci_addr, read_long(pci_addr) & val)
#define OR_WORD(pci_addr,val)    write_long(pci_addr, read_long(pci_addr) | val)
#define WRITE_WORD(pci_addr,val) write_long(pci_addr,val)
#define READ_WORD(pci_addr)      read_long(pci_addr)
#else /* KX, CX, JX, HX */
#define AND_WORD(pci_addr,val)   *pci_addr = *pci_addr & val
#define OR_WORD(pci_addr,val)    *pci_addr = *pci_addr | val
#define WRITE_WORD(pci_addr,val) *pci_addr = val
#define READ_WORD(pci_addr)      *pci_addr
#endif

#if BIG_ENDIAN_CODE
#define PCI_ERROR_BIT         BE_CYCLONE_ERROR_BIT
#define PCI_DATA_READY_BIT    BE_CYCLONE_DATA_READY_BIT
#define PCI_EOD_BIT           BE_CYCLONE_EOD_BIT
#define PCI_INUSE_BIT         BE_CYCLONE_INUSE_BIT
#define PCI_INTERRUPT_BIT     BE_CYCLONE_INTERRUPT_BIT
#define PCI_HOST_TRANSFER     BE_CYCLONE_HOST_TRANSFER
#define PCI_TARGET_TRANSFER   BE_CYCLONE_TARGET_TRANSFER
#else
#define PCI_ERROR_BIT         CYCLONE_ERROR_BIT
#define PCI_DATA_READY_BIT    CYCLONE_DATA_READY_BIT
#define PCI_EOD_BIT           CYCLONE_EOD_BIT
#define PCI_INUSE_BIT         CYCLONE_INUSE_BIT
#define PCI_INTERRUPT_BIT     CYCLONE_INTERRUPT_BIT
#define PCI_HOST_TRANSFER     CYCLONE_HOST_TRANSFER
#define PCI_TARGET_TRANSFER   CYCLONE_TARGET_TRANSFER
#endif

/* _cpu_speed uses the processor clock frequency, in MHz. It is defined in the */
/* board-specific include file.  (If the processor is running at 25 MHz, */
/* we use __cpu_speed of 25, not 25000000.) */

/* Return the delay constant required to delay t msec. */
static unsigned long
pci_wait_loops(int t)
{
#if CX_CPU /* *1000 after debug FIXME FIXME */
    return((t * __cpu_speed) << 11);
#elif HX_CPU
    return((t * __cpu_speed) << 12);
#elif JX_CPU
    return((t * __cpu_speed) << 11);
#elif KXSX_CPU
    return((t * __cpu_speed) << 10);
#endif
}

/*
 * FUNCTION
 *    PCI_CONNECT_REQUEST()
 *
 *
 * PURPOSE
 *    This routine verifies that pci_inuse was set by the host
 *     Resets the error bit to tell host that were are connected 
 *     Reset the PCI interrupt just in case it was not reset
 *
 *
 * RETURNS
 *    OK   -> success.
 *    ERR  -> error,  .
 */
int
pci_connect_request(void)
{
    if (pci_supported() == FALSE)
        return ERR;
        
    if ((READ_WORD(CYCLONE_TARGET_STATUS_MB) & PCI_INUSE_BIT) != PCI_INUSE_BIT)
        return ERR;
    
    AND_WORD(CYCLONE_TARGET_STATUS_MB, ~(PCI_ERROR_BIT));
    AND_WORD(CYCLONE_TARGET_PLDB_REG, PCI_INTERRUPT_BIT);
    return OK;
}


/*
 * FUNCTION
 *    PCI_SUPPORTED()
 *
 *
 * PURPOSE
 *    This routine normally would simply return TRUE. 
 * However, there exist standalone Cyclone baseboards that do not
 * contain PLX PCI bus chips and, thus, do not support PCI!
 *
 *
 * RETURNS
 *    TRUE  -> PCI is available.
 *    FALSE ->  NO PCI on target  .
 */
int
pci_supported(void)
{
    return (PCI_INSTALLED());
}


/*
 * FUNCTION
 *    PCI_INTR()
 *
 *
 * PURPOSE
 *    This routine reset the PCI interrupt sent by the host.
 *
 *
 * RETURNS
 *    TRUE  -> success.
 *    FALSE -> No interrupt.
 */
int
pci_intr(void)
{
    if ((READ_WORD(CYCLONE_TARGET_PLDB_REG) & PCI_INTERRUPT_BIT) != PCI_INTERRUPT_BIT)
        return FALSE;

    AND_WORD(CYCLONE_TARGET_PLDB_REG, PCI_INTERRUPT_BIT);
    return TRUE;
}


/*
 * FUNCTION
 *    PCI_ERR()
 *
 *
 * PURPOSE
 *    This routine sets the PCI error bit.
 *    And resets the other data transfer bits.
 *
 * RETURNS
 */
void
pci_err(void)
{
    OR_WORD(CYCLONE_TARGET_STATUS_MB,  PCI_ERROR_BIT);
    AND_WORD(CYCLONE_TARGET_STATUS_MB,
        ~(PCI_HOST_TRANSFER | PCI_TARGET_TRANSFER | PCI_DATA_READY_BIT | PCI_EOD_BIT));
    AND_WORD(CYCLONE_TARGET_PLDB_REG, PCI_INTERRUPT_BIT);
}


/*
 * FUNCTION
 *    PCI_NOT_INUSE()
 *
 *
 * PURPOSE
 *    This routine ets the inuse bit
 *
 *
 * RETURNS
 */
void 
pci_not_inuse(void)
{
    if (pci_supported() == TRUE)
        AND_WORD(CYCLONE_TARGET_STATUS_MB, ~PCI_INUSE_BIT);
}

/*
 * FUNCTION
 *    PCI_INUSE()
 *
 *
 * PURPOSE
 *    This routine ets the PCI inuse bit
 *
 *
 * RETURNS
 *    OK   -> success.
 *    ERR  -> error,  .
 */
int 
pci_inuse(void)
{
    if ((pci_supported() == FALSE) ||
        ((READ_WORD(CYCLONE_TARGET_STATUS_MB) & PCI_INUSE_BIT) != PCI_INUSE_BIT))
        return ERR;

    return OK;
}


/*
 * FUNCTION
 *    pci_write_data_mb()
 *         mb_value  -   The value to write into the PLX MBOX 6
 *
 *
 * PURPOSE
 *    This routine puts a value int the data mb PLX MBOX 6. 
 *    Used by pci download/connect code to prove the corerect connection
 *
 *
 * RETURNS
 */
void
pci_write_data_mb(unsigned long mb_value)
{
#if BIG_ENDIAN_CODE
    swap_data_byte_order(4, &mb_value, 4);
#endif
    WRITE_WORD(CYCLONE_TARGET_DATA_MB, mb_value);
}


/*
 * FUNCTION
 *    PCI_READ()
 *        data     -  Pointer to the data to read
 *        size     -  size of data in bytes
 *        crc      -  optional CRC, use NULL for no CRC value calaculation
 *        partial_transfer - host is sending more than this request don't check EOD
 *        timeout_msec     - timeout values to use 
 *
 * PURPOSE
 *    This routine read data from the host using PCI .
 *
 *
 * RETURNS
 *    OK   -> success.
 *    ERR  -> error,  .
 */
int
pci_read(unsigned char  *data, 
         unsigned int   size, 
         unsigned short *crc,
         unsigned int   partial_transfer,
         unsigned int   timeout_msec)
{
    unsigned int status, sum = 0;
    union 
        {
        unsigned int   word;
        unsigned char  bytes[4];
        } temp;
    volatile unsigned long timeout;
    unsigned int * word_data = (unsigned int *) data;

    timeout = pci_wait_loops(timeout_msec);
    while ((((status=READ_WORD(CYCLONE_TARGET_STATUS_MB)) & PCI_HOST_TRANSFER) != PCI_HOST_TRANSFER) ||
        ((status & PCI_DATA_READY_BIT) != PCI_DATA_READY_BIT)) 
        {
        if (timeout != 0)
            if (--timeout == 0)
                {
	            com_stat = E_COMM_TIMO;
                pci_err();
                return ERR;
                }
        }

    if (size <= 4)
        {
        timeout = pci_wait_loops(timeout_msec);
        while ((((status = READ_WORD(CYCLONE_TARGET_STATUS_MB)) & PCI_DATA_READY_BIT) !=
                   PCI_DATA_READY_BIT) && !break_flag) 
            {
            if ((status & PCI_ERROR_BIT) == PCI_ERROR_BIT)
                {
                return ERR;
                }
            if (break_flag)
                {
                pci_err();
	    	    com_stat = E_INTR;
                return ERR;
                }
            if (timeout != 0)
                if (--timeout == 0)
                    {
                    pci_err();
	    	        com_stat = E_COMM_TIMO;
                    return ERR;
                    }
            }

        temp.word = READ_WORD(CYCLONE_TARGET_DATA_MB);
        *data++ = temp.bytes[0];
        if (size > 1)
            *data++ = temp.bytes[1];
        if (size > 2)
            *data++ = temp.bytes[2];
        if (size > 3)
            *data = temp.bytes[3];

        if ((status & PCI_EOD_BIT) != PCI_EOD_BIT ) 
            {
            pci_err();
            return ERR; 
            }

        AND_WORD(CYCLONE_TARGET_STATUS_MB, ~PCI_DATA_READY_BIT);
        if (size > 2)
            sum = temp.word;
        }
        
    else /* size > 4 */
        {
        size= (size+3)/4;
        while (size-- > 0)
            {
            timeout = pci_wait_loops(timeout_msec);
            while ((((status = READ_WORD(CYCLONE_TARGET_STATUS_MB)) & PCI_DATA_READY_BIT) != 
                    PCI_DATA_READY_BIT) && !break_flag) 
                {
                if ((status & PCI_ERROR_BIT) == PCI_ERROR_BIT)
                    return ERR;
                if (break_flag)
                    {
                    pci_err();
	    	        com_stat = E_INTR;
                    return ERR;
                    }
                if (timeout != 0)
                    if (--timeout == 0)
                       {
                       pci_err();
	    	           com_stat = E_COMM_TIMO;
                       return ERR;
                       }
                }
    
            *word_data = READ_WORD(CYCLONE_TARGET_DATA_MB);
            sum ^= *word_data++;

            if (size == 0)
                {
                if ((partial_transfer != TRUE) &&
                    ((status & PCI_EOD_BIT) != PCI_EOD_BIT) )
                    {
                    pci_err();
                    return ERR; 
                    }
                }
            else
                {
                if ((status & PCI_EOD_BIT) == PCI_EOD_BIT)
                    {
                    pci_err();
                    /*board_reset();*/
                    return ERR; 
                    }
                }

            AND_WORD(CYCLONE_TARGET_STATUS_MB, ~PCI_DATA_READY_BIT);
            }
        }

    if (crc != NULL)
        *crc ^= (unsigned short)((sum >> 16) ^ sum);

    return OK;
}

/*
 * FUNCTION
 *    PCI_WRITE()
 *        data     -  Pointer to the data to write
 *        size     -  size of data in bytes
 *        crc      -  optional CRC, use NULL for no CRC value calaculation
 *        timeout_msec     - timeout values to use 
 *
 * PURPOSE
 *    This routine read data from the host using PCI .
 *
 * RETURNS
 *    OK   -> success.
 *    ERR  -> error,  .
 */
int
pci_write(unsigned char *data, unsigned int size, unsigned short *crc, unsigned int timeout_msec)
{
    unsigned int status, sum = 0;
    unsigned int * word_data = (unsigned int *) data;
    volatile unsigned long timeout;
    union 
        {
        unsigned int   word;
        unsigned char  bytes[4];
        } temp;

    if (((status = READ_WORD(CYCLONE_TARGET_STATUS_MB)) & PCI_TARGET_TRANSFER) ==
          PCI_TARGET_TRANSFER)
        {
        pci_err();
        return ERR;
        }

    timeout = pci_wait_loops(timeout_msec);
    while ((((status = READ_WORD(CYCLONE_TARGET_STATUS_MB)) & PCI_HOST_TRANSFER) == 
            PCI_HOST_TRANSFER) && !break_flag) 
        {
        if ((status & PCI_ERROR_BIT) == PCI_ERROR_BIT)
            return ERR;
        if (break_flag)
            {
            pci_err();
            com_stat = E_INTR;
            return ERR;
            }
        if (timeout != 0)
            if (--timeout == 0)
               {
               pci_err();
               com_stat = E_COMM_TIMO;
               return ERR;
               }
        }

    AND_WORD(CYCLONE_TARGET_STATUS_MB, ~(PCI_ERROR_BIT | PCI_DATA_READY_BIT | PCI_EOD_BIT));
    OR_WORD(CYCLONE_TARGET_STATUS_MB, PCI_TARGET_TRANSFER);

    if (size <= 4)
        {
        temp.bytes[0] = *data++;
        if (size > 1)
            temp.bytes[1] = *data++;
        if (size > 2)
            temp.bytes[2] = *data++;
        if (size > 3)
            temp.bytes[3] = *data;
        WRITE_WORD(CYCLONE_TARGET_DATA_MB, temp.word);

        OR_WORD(CYCLONE_TARGET_STATUS_MB, PCI_DATA_READY_BIT | PCI_EOD_BIT);
        }
        
    else /* size > 4 */
        {

        size= (size+3)/4;
        while (size-- > 0)
            {
            timeout = pci_wait_loops(timeout_msec);
            while ((((status = READ_WORD(CYCLONE_TARGET_STATUS_MB)) & PCI_DATA_READY_BIT) == 
                    PCI_DATA_READY_BIT) && !break_flag) 
                {
                if ((status & PCI_ERROR_BIT) == PCI_ERROR_BIT)
                    return ERR;
                if (break_flag)
                    {
                    pci_err();
	    	        com_stat = E_INTR;
                    return ERR;
                    }
                if (timeout != 0)
                    if (--timeout == 0)
                       {
                       pci_err();
	                   com_stat = E_COMM_TIMO;
                       return ERR;
                       }
                }
        
            WRITE_WORD(CYCLONE_TARGET_DATA_MB, *word_data);
            sum ^= *word_data++;

            if (size == 0)
                OR_WORD(CYCLONE_TARGET_STATUS_MB, PCI_EOD_BIT);

            OR_WORD(CYCLONE_TARGET_STATUS_MB, PCI_DATA_READY_BIT);
            }

        }

    timeout = pci_wait_loops(timeout_msec);
    while ((((status = READ_WORD(CYCLONE_TARGET_STATUS_MB)) & PCI_DATA_READY_BIT) == 
            PCI_DATA_READY_BIT))
        {
        if ((status & PCI_ERROR_BIT) == PCI_ERROR_BIT)
            return ERR;
        if (timeout != 0)
            if (--timeout == 0)
               {
               pci_err();
	           com_stat = E_COMM_TIMO;
               return ERR;
               }
        }
        
    AND_WORD(CYCLONE_TARGET_STATUS_MB, ~PCI_TARGET_TRANSFER);

    if ((status & PCI_ERROR_BIT) == PCI_ERROR_BIT)
        return ERR;

    if (crc != NULL)
        *crc ^= (unsigned short)((sum >> 16) ^ sum);

    return OK;
}


/*
 * FUNCTION
 *    pci_init()
 *
 *
 * PURPOSE
 *    This routine resets al transfer  bits and the error bit .
 *
 *
 * RETURNS
 *    OK   -> success.
 *    ERR  -> error,  NO PCI available .
 */
int 
pci_init()
{
    if (pci_supported() == FALSE)
        return ERR;

    AND_WORD(CYCLONE_TARGET_STATUS_MB, ~(PCI_ERROR_BIT | PCI_DATA_READY_BIT | PCI_EOD_BIT |
                                         PCI_HOST_TRANSFER | PCI_TARGET_TRANSFER));
    return OK;
}


/*
 * FUNCTION
 *    PCI_OPEN()
 *
 *
 * PURPOSE
 *    This routine calls pci_init.
 *
 *
 * RETURNS
 *    OK   -> success.
 *    ERR  -> error, NO PCI Available .
 */
int 
pci_dev_open()
{
    return (pci_init());
}

/*
 * FUNCTION
 *    PCI_CLOSE()
 *         fd  - not used for pci
 *
 * PURPOSE
 *    This routine  resets the inuse bit and reset all other bits..
 *
 *
 * RETURNS
 *    OK   -> success.
 *    ERR  -> error, inot connected .
 */
int
pci_dev_close(int fd)
{
    if ((READ_WORD(CYCLONE_TARGET_STATUS_MB) & PCI_INUSE_BIT) != PCI_INUSE_BIT) 
        return ERR;

    AND_WORD(CYCLONE_TARGET_STATUS_MB, ~(PCI_ERROR_BIT | PCI_DATA_READY_BIT | PCI_EOD_BIT |
                                         PCI_HOST_TRANSFER | PCI_TARGET_TRANSFER));

    AND_WORD(CYCLONE_TARGET_STATUS_MB, ~PCI_INUSE_BIT);
    return OK;
}

/*
 * FUNCTION
 *    PCI_DEV_READ()
 *        fd         - not used
 *        buf        - the buffer to read data into
 *        cnt        - the number of bytes to read
 *        timeout    - POLL, WAIT_FOREVER or timeout in msec
 *
 * PURPOSE
 *    This routine reads data drom the host using PCI .
 *
 *
 * RETURNS
 *    the number of bytes read
 */
int
pci_dev_read(int fd, unsigned char * buf, int cnt, int timeout)
{
    switch(timeout)
        {
        case COM_POLL:
            return(pci_read(buf, cnt, (unsigned short *)NULL, FALSE, 3000)==OK ? cnt : 0);

        case COM_WAIT_FOREVER:
            return(pci_read(buf, cnt, (unsigned short *)NULL, FALSE, 0)==OK ? cnt : 0);
        
	    default:
            return(pci_read(buf, cnt, (unsigned short *)NULL, FALSE, timeout)==OK ? cnt : 0); /* timeout*/
        }
}


/*
 * FUNCTION
 *    PCI_DEV_WRITE()
 *        fd         - not used
 *        buf        - the buffer to write data from
 *        cnt        - the number of bytes to write
 *
 * PURPOSE
 *    This routine sends data to the host using PCI .
 *
 *
 * RETURNS
 *    OK   -> success.
 *    ERR  -> error,  .
 */
int
pci_dev_write(int fd, const unsigned char *data, int cnt)
{
    return (pci_write(data, cnt, NULL, 3000));  /*1000*/
}



/*
 * FUNCTION
 *    pci_download()
 *
 *
 * PURPOSE
 *    This routine set the PCI routines used by fast_downlaod .
 *
 *
 * RETURNS
 */
void
pci_download(const void *p_cmd)
{
    fast_download_cfg(pci_init, pci_err, pci_read);
    fast_download(p_cmd, PCI_DOWNLOAD);
}
