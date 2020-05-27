/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1995 Intel Corporation
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

/* $Header: /ffs/p1/dev/src/mon960/common/RCS/commcfg.c,v 1.7 1995/11/03 19:58:33 cmorgan Exp $ */

/***************************************************************************
 *
 * NAME:         commcfg.c
 *
 * DESCRIPTION:  A wrapper to manage connecting with 1 of N comm ports.
 *
 ***************************************************************************/

#include "retarget.h"
#include "mon960.h"
#include "hdi_com.h"
#include "dev.h"

unsigned int auto_timeout_select = FALSE;

static int pci_active, serial_active;
static DEV comm_dev;

/* 
 * Function to poll all possible comm channels for data from the host. 
 * As soon as data is received at one of the channels, we assume
 * that it is the channel the host wishes to use for communication.
 */
static void
poll_comm_channels(void)
{
    comm_dev.dev_open  = serial_open;
    comm_dev.dev_read  = serial_read;
    comm_dev.dev_write = serial_write;
    comm_dev.dev_baud  = serial_baud;
    serial_active = TRUE;
    pci_active = FALSE;
    com_init();
    auto_timeout_select = TRUE;
    pci_not_inuse();

    while(TRUE)
        {
        if (pci_inuse() == OK)
            {
            pci_active = TRUE;
            serial_active = FALSE;
            comm_dev.dev_open  = pci_dev_open;
            comm_dev.dev_close = pci_dev_close;
            comm_dev.dev_read  = pci_dev_read;
            comm_dev.dev_write = pci_dev_write;
            com_init();
            
            if (pci_connect_request() == OK) 
                {
                host_connection = TRUE;
                return;
                }
                
            comm_dev.dev_open  = serial_open;
            comm_dev.dev_read  = serial_read;
            comm_dev.dev_write = serial_write;
            comm_dev.dev_baud  = serial_baud;
            pci_active = FALSE;
            serial_active = TRUE;
            com_init();
            }
            
        if (com_reset(0) == OK)
            return;
        }
    /* never reached */
}



/*
 * Function called by the monitor very early in its boot phase to 
 * determine whether or not a connection should be made with a serial 
 * or PCI comm channel.
 */
void
comm_config(void)
{
    int brk_vector, serial_avail, pci_avail;

    leds(0,15);
    serial_active = pci_active = FALSE;
    serial_avail  = serial_supported();
    pci_avail     = pci_supported();

    /*
     * Determine if the serial/pci channels must be polled or if this
     * monitor's HW only supports one possible comm channel.
     */
    if ((serial_avail == FALSE && pci_avail == FALSE))
    {
        /* 
         * Hey, no comm channel avail at all!  Go into infinite blink 
         * loop.  Nothing more we can do here.
         */
        while (1)
            blink (0xf);
        /* NOT REACHED */
    }

    if (serial_avail == TRUE && pci_avail == TRUE)
    {
        /* Must poll the comm channels. */
        poll_comm_channels();
    }
    else if (serial_avail == TRUE)
    {
        /* HW only supports serial comm channel. */
        serial_active = TRUE;
        comm_dev.dev_open  = serial_open;
        comm_dev.dev_read  = serial_read;
        comm_dev.dev_write = serial_write;
        comm_dev.dev_baud  = serial_baud;
        com_init();
        auto_timeout_select = FALSE;
        while (com_reset(0) != OK) blink(15);
    }
    else
    {
        /* HW only supports PCI comm channel. */

        pci_active = TRUE;
        comm_dev.dev_open  = pci_dev_open;
        comm_dev.dev_close = pci_dev_close;
        comm_dev.dev_read  = pci_dev_read;
        comm_dev.dev_write = pci_dev_write;
        com_init();

        pci_not_inuse();
        while (pci_connect_request() == ERR) ;
        host_connection = TRUE;
    }

    /* Set up HDIL comm structures accordingly. */
    /* Setup serial/pci break vector. */
    brk_vector = get_int_vector(-1);
    set_break_vector(brk_vector, NULL);
    if (brk_vector != 0xf8)
    {
        /* set priority to one less than serial/pci intr*/
        mon_priority = (brk_vector >> 3) - 1;
    }
}


const DEV *
com_dev()
{
    return(&comm_dev);
}


/* Simple function to return state of current comm connection. */
int 
pci_comm(void)
{
    return (pci_active);
}


/* Simple function to return state of current comm connection. */
int 
serial_comm(void)
{
    return (serial_active);
}
