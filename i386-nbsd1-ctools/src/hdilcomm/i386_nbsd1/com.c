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
/* $Header: /ffs/p1/dev/src/hdilcomm/common/RCS/com.c,v 1.14 1995/11/05 07:33:54 cmorgan Exp $$Locker:  $ */

#include <stdio.h>
#include <string.h>

#ifdef HOST

extern int target_reset;

#ifdef __STDC__
#include <stdlib.h>
#else /* __STDC__ */
extern char *malloc();
extern void free();
#endif /* __STDC__ */

#endif /* HOST */

#include "common.h"
#include "hdi_errs.h"
#include "dev.h"
#include "hdi_com.h"
#include "com.h"

/* message offsets */
#define    PP_LLEN        0
#define    PP_HLEN        1
#define    PROHDRSZ    2
#define    PP_DATA        PROHDRSZ

#ifdef HOST
COM_CONFIG _com_config = { RS232 };  /* Assume host will use serial comm. */
COM_CONFIG_SET _com_config_set;
#endif

int com_stat;

static const PKTDEV *pktp = NULL;

static unsigned char local_buf[MAX_MSG_SIZE + PROHDRSZ];
static unsigned char *put_ptr = NULL;

/* This function exists to assist creation of a PIC/PID monitor. */
void EXPORT
com_pkt_ptr_init()
{
    pktp = &byte_pkt;
}


int EXPORT
com_init()
{
#ifdef HOST
    /*
     * A bit of defensive programming here.  If the client has specified
     * neither serial nor PCI comm, the client has incorrectly configured
     * HDI's packet and communication layers.
     */
    if (! (_com_config.dev_type == RS232 || 
                                       _com_config.dev_type == HDI_PCI_COMM))
    {
        com_stat = E_COMM_PROTOCOL;
        return (ERR);
    }
#endif

    pkt_protocol_setup();  /* Inits global comm ptrs common to HOST & TARGET */

#ifdef HOST
    if (_com_check_config() != OK)
        return(ERR);

    while ((*pktp->pkt_init)() != OK)
    {
        if (com_error() != OK)
        {
        pktp = NULL;
        return(ERR);
        }
    }
#endif /* HOST */

#ifdef TARGET
    if ((*pktp->pkt_init)() != OK)
    {
        pktp = NULL;
        return(ERR);
    }
#endif

    return(OK);
}

int EXPORT
com_reset(reset_time)
int reset_time;
{
    if (pktp == NULL)
    {
        com_stat = E_COMM_ERR;
        return ERR;
    }

    return (*pktp->pkt_reset)(reset_time);
}

#ifdef HOST
void EXPORT
com_signal()
{
    if (pktp) {
        (*pktp->pkt_signal)();
    }
}

int EXPORT
com_intr_trgt()
{
    int status;
    if (pktp == NULL)
    {
        com_stat = E_COMM_ERR;
        return ERR;
    }
    status = (*pktp->pkt_intr_trgt)();
    return(status);
}

int EXPORT
com_term()
{
    int r;

    if (pktp == NULL)
    {
        com_stat = E_COMM_ERR;
        return(ERR);
    }
    
    r = (*pktp->pkt_term)();

    _com_config.dev_type = NO_COMM;
    
    pktp = NULL;

    return(r);
}
#endif

const unsigned char * EXPORT
com_get_msg(szp, wait)
int *szp;
int wait;
{
    unsigned char *bufp;
    int msg_sz, to_read;
    int r;

    if (pktp == NULL)
    {
        com_stat = E_COMM_ERR;
        return(NULL);
    }

    put_ptr = NULL;

    r = (*pktp->pkt_get)(local_buf, sizeof(local_buf), wait);

    if (r < PROHDRSZ) {
        return(NULL);
    }

    r -= PROHDRSZ;
    msg_sz = (local_buf[PP_HLEN] << 8) + local_buf[PP_LLEN];
    if (msg_sz > sizeof(local_buf) - PROHDRSZ)
    {
        com_stat = E_BUFOVFL;
        return (NULL);
    }

    bufp = local_buf + PROHDRSZ + r;
    to_read = msg_sz - r;

    while (to_read > 0)
    {
        r = (*pktp->pkt_get)(bufp, to_read, COM_WAIT);
        if (r <= 0)
            return(NULL);
        bufp += r;
        to_read -= r;
    }

    *szp = msg_sz;
    return &local_buf[PP_DATA];
}


int 
com_put_msgx(msg, sz, retry)
const unsigned char *msg;
int sz;
int retry;
{
    unsigned char *bufp = local_buf;
    int r;

    if (pktp == NULL)
    {
        com_stat = E_COMM_ERR;
        return(ERR);
    }

    if (msg == NULL)
    {
        if (put_ptr == NULL)
        {
            com_stat = E_ARG;
            return(ERR);
        }

        msg = &local_buf[PP_DATA];
        sz = put_ptr - msg;
    }

    put_ptr = NULL;

    if (sz+PROHDRSZ > sizeof(local_buf))
#ifdef HOST
        bufp = (unsigned char *)malloc((unsigned)sz+PROHDRSZ);
#else
    {
        com_stat = E_BUFOVFL;
        return ERR;
    }
#endif

    bufp[PP_HLEN] = sz >> 8;
    bufp[PP_LLEN] = sz;
    if (msg != &bufp[PP_DATA])
        memcpy((char *)&bufp[PP_DATA], (const char *)msg, sz);

    if (retry)
        r = (*pktp->pkt_put)(bufp,sz+PROHDRSZ);
    else
        r = (*pktp->pkt_put_once)(bufp, sz+PROHDRSZ, 1);
        
#ifdef HOST
    if (bufp != local_buf)
        free((void *)bufp);
#endif
    return(r);
}


int EXPORT
com_put_msg(msg, sz)
const unsigned char *msg;
int sz;
{
    return(com_put_msgx(msg, sz, TRUE));;
}


int EXPORT
com_put_msg_once(msg, sz)
const unsigned char *msg;
int sz;
{
    return(com_put_msgx(msg, sz, FALSE));;
}


const unsigned char * EXPORT
com_send_cmd(msg, szp, wait)
const unsigned char *msg;
int *szp;
int wait;
{
    const unsigned char *r = NULL;

    if (com_put_msg(msg, *szp) == OK)
        r = com_get_msg(szp, wait);

    return(r);
}


int EXPORT
com_get_stat()
{
    return(com_stat);
}

#ifdef HOST
int EXPORT
com_get_target_reset()
{
    int reset = target_reset;

    /* reset when called */
    target_reset = FALSE;
    return(reset);
}

void EXPORT
com_get_pci_controlling_port(port_buffer)
char *port_buffer;    /* Must be at least COM_PCI_SRL_PORT_SZ bytes long. */
{
    int len = strlen(_com_config.device);

    if (len < COM_PCI_SRL_PORT_SZ)
        strcpy(port_buffer, _com_config.device);
    else
    {
        strcpy(port_buffer,
               _com_config.device + len - (COM_PCI_SRL_PORT_SZ - 1));
    }
}
#endif


#define put_byte(b) (*put_ptr++ = (unsigned char)(b))

void EXPORT
com_init_msg()
{
    put_ptr = &local_buf[PP_DATA];
}

int EXPORT
com_put_byte(b)
int b;
{
    if (put_ptr - local_buf > sizeof(local_buf) - 1)
    {
    com_stat = E_BUFOVFL;
    return ERR;
    }
    put_byte(b);
    return OK;
}

int EXPORT
com_put_short(s)
int s;
{
    if (put_ptr - local_buf > sizeof(local_buf) - 2)
    {
    com_stat = E_BUFOVFL;
    return ERR;
    }
    put_byte(s);
    put_byte(s>>8);
    return OK;
}

int EXPORT
com_put_long(l)
unsigned long l;
{
    if (put_ptr - local_buf > sizeof(local_buf) - 4)
    {
    com_stat = E_BUFOVFL;
    return ERR;
    }
    put_byte(l);
    put_byte(l>>8);
    put_byte(l>>16);
    put_byte(l>>24);
    return OK;
}

int EXPORT
com_put_data(buf, size)
const unsigned char *buf;
int size;
{
    if (size > sizeof(local_buf)
        || (unsigned int)(put_ptr - local_buf) > sizeof(local_buf) - size)
    {
    com_stat = E_BUFOVFL;
    return ERR;
    }
    memcpy((char *)put_ptr, (const char *)buf, size);
    put_ptr += size;
    return OK;
}

#if !defined(WINDOWS) && defined(HOST)
/***********************************************************************
*
*  NAME: com_check_config
*
*  returnS: ERR if the configuration is bad
*           OK if user selected a valid configuration
*
*  DESCRIPTION:
*    Checks the validity of the global _com_config structure.
*********************************************************************p*/

int _com_check_config()
{
    return(OK);
}
#endif


#if !defined(WINDOWS) && defined(HOST)
/*p*********************************************************************
*
*  NAME: com_error
*
*  DESCRIPTION:
*    Called when an attempt to connect the target was unsuccessful.
*********************************************************************p*/
int EXPORT com_error()
{
    return (ERR);
}
#endif


#ifdef HOST

/* Provide debuggers with an interface to obtain the PCI configuration info
 * after a pci connection is made. */
void
com_pci_get_cfg(pci_cfg)
COM_PCI_CFG *pci_cfg;
{
    *pci_cfg = _com_config.pci;
}

/* 
 * Here are a couple of boolean query functions that permit HDI to track
 * the comm channel selected by the host.
 */

int
com_pci_comm()
{
    return (_com_config.dev_type == HDI_PCI_COMM);
}



int
com_serial_comm()
{
    return (_com_config.dev_type == RS232);
}

#endif /* defined(HOST) */


#ifdef HOST

/*
 * Initialization of the global comm DEV data structure is more complex on
 * the host than it is on the target because we must deal with the fact
 * that, currently, only DOS & Winblows supports multiple comm channels.
 */
const DEV *
com_dev()
{
#ifdef MSDOS
    if (com_serial_comm())
        return (com_serial_dev());
    else
    {
        /* Assume PCI comm requested. */

        return (com_pci_dev());
    }
#else

    /* Unix currently only supports serial comm.  Pity. */
    return (com_serial_dev());
#endif
}

#endif  /* defined(HOST) */
