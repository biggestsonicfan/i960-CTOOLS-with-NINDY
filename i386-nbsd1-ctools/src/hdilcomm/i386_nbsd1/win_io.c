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
/*
 *    serial support for target communications
 */
/* $Header: /ffs/p1/dev/src/hdilcomm/gorman/RCS/win_io.c,v 1.11 1996/01/18 18:37:36 gorman Exp $$Locker:  $ */

#include <dos.h>
#include <stdio.h>
#include <windows.h>
#include "common.h"
#include "hdil.h"
#include "hdi_errs.h"
#include "com.h"
#include "dev.h"

#define QBUFSZ    (5*1024)	/* queue size */
#define DOS_PKT_TIMO 1000
#define DOS_ACK_TIMO  500

extern const unsigned short CRCtab[];

static unsigned int intr_flag;


/*
 * Open the serial device.
 * return
 *    0    indicating success
 *    -1    failure
 */
int
serial_open()
{
	COMMTIMEOUTS  CommTimeOuts;
	DCB dcb;
	char DirectPort[9] = "\\.\\";

	if (_com_config.dev_type == 0)
		_com_config.dev_type = RS232;
	if (strncmp(_com_config.device, "COM", 3) == 0)
	{
		(void) strcat(DirectPort, _com_config.device);
		(void) strcat(DirectPort, ":");
	}
	else
	{
		com_stat = E_BAD_CONFIG;
		return(ERR);
	}
	if (_com_config.host_pkt_timo == 0)
		_com_config.host_pkt_timo = DOS_PKT_TIMO;

	if (_com_config.ack_timo == 0)
		_com_config.ack_timo = DOS_ACK_TIMO;

	(HANDLE) _com_config.iobase = CreateFile(DirectPort,
		  GENERIC_READ | GENERIC_WRITE, 0,			// exclusive access
		  (LPSECURITY_ATTRIBUTES) NULL,		// no security attrs
		  OPEN_EXISTING, 0, /* WAS FILE_ATTRIBUTE_NORMAL, */
		  (HANDLE) NULL);
    
    if ((HANDLE)_com_config.iobase == INVALID_HANDLE_VALUE)
        {
        hdi_put_line("Serial port open failed.\n");
        com_stat = E_COMM_ERR;
        return(ERR);
        }

	if (GetCommState((HANDLE) _com_config.iobase, &dcb) != TRUE)
        {
        hdi_put_line("Serial open GetCommState failed.\n");
		com_stat = E_COMM_ERR;
        return(ERR);
        }
    
	dcb.BaudRate = _com_config.baud;
	dcb.ByteSize = 8;
	dcb.StopBits = ONESTOPBIT;
    dcb.fParity = FALSE;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fNull = FALSE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fAbortOnError = TRUE;

	if (SetCommState((HANDLE) _com_config.iobase, &dcb) != TRUE)
        {
        hdi_put_line("Serial open SetCommState failed.\n");
		com_stat = E_COMM_ERR;
        return(ERR);
        }
	if (SetupComm((HANDLE) _com_config.iobase, QBUFSZ, QBUFSZ) != TRUE)
	    {
        hdi_put_line("Serial open SetupComm failed.\n");
		com_stat = E_COMM_ERR;
		return(ERR);
	    }
	CommTimeOuts.ReadIntervalTimeout = 0;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 1;
	CommTimeOuts.ReadTotalTimeoutConstant = _com_config.host_pkt_timo;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 1;
	CommTimeOuts.WriteTotalTimeoutConstant = _com_config.ack_timo;
	if (SetCommTimeouts((HANDLE) _com_config.iobase, &CommTimeOuts) != TRUE)
        {
        hdi_put_line("Serial open set timeouts failed.\n");
        com_stat = E_COMM_ERR;
        return(ERR);
        }
	return _com_config.iobase;
}

/*
 * Close the serial device, terminating usage in current context.
 * return
 *    0    indicating success
 *    -1    failure
 */
int
serial_close(int port)
{
	return CloseHandle((HANDLE) port);
}

/*
 * Attempt to read N bytes from the serial device, waiting up to
 * timo (time-out) milliseconds for data to become available.
 * return
 *    0-N    Actual number of bytes read, 0 if none.
 *    -1    error condition
 */
static int current_timo = DOS_ACK_TIMO;

int
serial_read(int port, unsigned char *buf, int size, int timo)
{
	int actual;
    long error; 
    COMSTAT cst;
	COMMTIMEOUTS  CommTimeOuts;

	if (current_timo != timo)
	{
		current_timo = timo;
		CommTimeOuts.WriteTotalTimeoutMultiplier = 1;
		CommTimeOuts.WriteTotalTimeoutConstant = _com_config.ack_timo;
		switch (timo)
		{
		case COM_POLL:
			CommTimeOuts.ReadIntervalTimeout = MAXDWORD;
		    CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
			CommTimeOuts.ReadTotalTimeoutConstant = 0;
			break;
		case COM_WAIT_FOREVER:
			CommTimeOuts.ReadIntervalTimeout = 0;
		    CommTimeOuts.ReadTotalTimeoutMultiplier = 1;
			CommTimeOuts.ReadTotalTimeoutConstant = 1000;
			break;
		default:
			CommTimeOuts.ReadIntervalTimeout = 0;
		    CommTimeOuts.ReadTotalTimeoutMultiplier = 1;
			CommTimeOuts.ReadTotalTimeoutConstant = timo;
			break;
		}
		if (SetCommTimeouts((HANDLE) _com_config.iobase, &CommTimeOuts) != TRUE)
            hdi_put_line("Serial set timeouts failed.\n");
    }
    
    intr_flag = FALSE;
    com_stat=OK;
    if (timo == COM_WAIT_FOREVER) 
        {
        /* Wait 1 seconds between checks for ctrl-c */
        int this_actual = 0, this_size = size;

        actual = 0;
        while (intr_flag == FALSE)
            {
    	    if (ReadFile((HANDLE) port, buf, this_size, &this_actual, NULL) == TRUE)
                {
                /* WIN95 returns TRUE when TIMEOUT occurs with 0m bytes read */        
                /* So we must check for if the bytes are actuall read */
                if (this_actual == this_size)
                    {
                    actual = size;
                    break;
                    }
                }
            else
               ClearCommError((HANDLE) port, (LPDWORD)&error, &cst );

            actual      += this_actual;
            buf         += this_actual;
            this_size   -= this_actual;    
            }
        }
    else 
        if (ReadFile((HANDLE) port, buf, size, &actual, NULL) != TRUE)
               {
               ClearCommError((HANDLE) port, (LPDWORD)&error, &cst );
	    	   com_stat = E_COMM_TIMO;
               }

    if (intr_flag == TRUE)
        com_stat = E_INTR;

	return actual;
}


void
serial_signal(int port)
{
    intr_flag = TRUE;
	com_stat = E_INTR;
}

/*
 * Attempt to write N bytes from the serial device, stopping as soon 
 * as unable to send more without waiting.  That's a lie!
 * return
 *    0-N    Actual number of bytes written, 0 if none.
 *    -1    error condition
 */
int
serial_write(int port, const unsigned char *bp, int want)
{
	int actual;
    long error; 
    COMSTAT cst;

	if (WriteFile((HANDLE) port, bp, want, &actual, NULL) != TRUE)
        {
        ClearCommError((HANDLE) port, (LPDWORD)&error, &cst );
		com_stat = E_COMM_TIMO;
        }
	return actual;
}


/*
 * Interrupt the target:
 * Send a break (RS232 low for more than a character period).
 */

#define	LOOPERMS	33		/* rough loop count per ms */
#define	BREAKMS		1000		/* break length */

int
serial_intr_trgt(int port)
{
	int cnt;
    long error; 
    COMSTAT cst;

	if (SetCommBreak((HANDLE) port) != TRUE)
        {
        hdi_put_line("Serial break start failed.\n");
        com_stat = E_COMM_ERR;
        return(ERR);
        }
	for (cnt = (unsigned)BREAKMS*LOOPERMS; cnt; --cnt)
        	;
	if (ClearCommBreak((HANDLE) port) != TRUE)
        {
        ClearCommError((HANDLE) port, (LPDWORD)&error, &cst );
        hdi_put_line("Serial break end failed.\n");
        com_stat = E_COMM_ERR;
        return(ERR);
        }
	return(OK);
}


static const DEV serial_dev =
    { serial_open, serial_close, serial_read, serial_write,
      serial_signal, serial_intr_trgt };


const DEV *
com_serial_dev()
{
	return(&serial_dev);
}


/*
    PARALLEL io routines for dos 
*/

static unsigned int i_pp_data;

/* 
 * Assign parallel download timeout interval, which is computed as:
 *
 *    max buffer xferred / slowest observed DOS download rate + fudge
 * I believe we COULD set this to zero, and just let Win95's per character
 * timeout rule.
 */
#define PAR_TIMEOUT ((unsigned long) ((MAX_DNLOAD_BUF / (20 * 1024)) + 2))

int EXPORT
com_parallel_init(device)
char * device;
{
	int FAR *ptr_port;
	COMMTIMEOUTS  CommTimeOuts;
	char DirectPort[9] = "\\.\\";

	if ((strcmp(device, "lpt1") !=0) && (strcmp(device , "LPT1") != 0) &&
	    (strcmp(device, "lpt2") !=0) && (strcmp(device , "LPT2") != 0) &&
	    (strcmp(device, "lpt3") !=0) && (strcmp(device , "LPT3") != 0))
	{
		hdi_cmd_stat = E_ARG;
		return(ERR);
	}
	else
	{
		(void) strcat(DirectPort, device);
		(void) strcat(DirectPort, ":");
	}

	(HANDLE) i_pp_data = CreateFile(device,
		  GENERIC_WRITE, 0,			// exclusive access
		  (LPSECURITY_ATTRIBUTES) NULL,		// no security attrs
		  OPEN_EXISTING, 0, /* WAS FILE_ATTRIBUTE_NORMAL, */
		  (HANDLE) NULL);

    
	if ((HANDLE)i_pp_data == INVALID_HANDLE_VALUE)
        {
		com_stat = E_COMM_ERR;
		return(ERR);
        }

	CommTimeOuts.ReadIntervalTimeout = 0;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = 0;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 1;
	CommTimeOuts.WriteTotalTimeoutConstant = PAR_TIMEOUT;

	SetCommTimeouts((HANDLE) i_pp_data, &CommTimeOuts);
	return(OK);
}


int EXPORT
com_parallel_end()
{
	int rval;

	rval = CloseHandle((HANDLE) i_pp_data);
	i_pp_data = 0;
	return rval;
}


int EXPORT
com_parallel_put(buf, size, crc)
register unsigned char *buf;
register unsigned long size;
unsigned short *crc;
{
	int actual;
	register unsigned short sum;

	if (WriteFile((HANDLE) i_pp_data, buf, size, &actual, NULL) == FALSE)
	{
		hdi_cmd_stat = E_PARA_DNLOAD_TIMO;
		return(ERR);
	}

	if (crc != NULL) {
		sum = *crc;
		while (size-- > 0)
		{
			sum = CRCtab[(sum^(*buf++))&0xff] ^ sum >> 8;
		}
		*crc = sum;
	}

	return(OK);
}
