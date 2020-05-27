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

#define _AUTOBAUD_DEFINES
#include "common.h"
#include "hdi_com.h"
#include "hdi_errs.h"
#include "com.h"
#include "dev.h"


#ifdef TARGET
/* Although the syntax and semantics of pragma align in ic960 and gcc960
 * are not identical, the following pragma has the desired effect in both:
 * i.e., to force the size of struct auto_baud_map to 12 rather than 16,
 * while retaining the natural alignment of its members.  This saves a
 * lot of space. */
/* Furthermore, the leading space is necessary for certain *HOST* compilers
 * which accidentally examine preprocessor directives within ifdefs that
 * aren't taken.  Note that this does not harm the *TARGET* compiler's
 * recognition of the pragma for either ic960 or gcc960. */
 #pragma align 4

extern int pci_inuse();
extern int break_flag, host_connection, auto_timeout_select;
extern unsigned long baud_rate;
#endif /* TARGET */


/*
 * The autobaud mechanism works as follows:
 *    The target listens at a pre-defined baud rate: 9600 for RS232.
 *    The host looks up the user-specified baud rate in the HOST table.
 *        It transmits the send characters to the target. (Note: if the "host"
 *        is a dumb terminal, the send character is always ^M (0x0d).)
 *    The target receives a character which is dependent on the baud rate
 *        the host is using and the character that was sent.  It looks this
 *        character up in the recv field of the target table. 
 *        If the character is not found, the target ignores it.
 *    The target changes to the baud rate in the entry corresponding to the
 *        received character.
 *    If the state field of the table entry corresponding to the received
 *        character is UI or HI, autobaud is complete.  The target responds to
 *        the host with ACK if the State field is HI.
 *    If the state field of the table entry is NEXT, the target clears
 *        the channel and waits for another byte from the host.  The host
 *        sends the next byte, and the process is repeated until it completes.
 *        This will take at most 3 bytes transmitted by the host.
 */

#ifdef HOST
struct  send_table {
   unsigned long  baud;
   unsigned char  size_to_send;
   unsigned char  send_chars[3];
};

static struct send_table host_send[8] = {
   {115200, 3, 0x0e, 0x1c, 0x5a},
   {57600,  2, 0xe0, 0x00, 0x00},
   {38400,  2, 0xf8, 0x5a, 0x00},
   {19200,  2, 0x98, 0x5a, 0x00},
   {9600,   1, 0x5a, 0x00, 0x00},
   {4800,   1, 0xfa, 0x00, 0x00},
   {2400,   2, 0xfe, 0x5a, 0x00},
   {1200,   2, 0xff, 0xfa, 0x00}};

#else /*TARGET*/
enum recv_types {TABLE_START, UI, HI, NEXT};

struct  recv_table {
    enum recv_types  recv_state;
    unsigned long    baud;
    unsigned char    recv_char;
};

static struct recv_table target_recv[] = {
  {TABLE_START,     0,    0},       /* 0 default starting baud 9600 */
    {HI,            9600, 0x5a}, {HI,            4800, 0x98},
    {NEXT,         38400, 0xff}, {NEXT,         38400, 0xfb},
    {NEXT,         38400, 0xfc}, {NEXT,         38400, 0xfd},
    {NEXT,         19200, 0xfa},
    {NEXT,         19200, 0xf9}, {NEXT,         19200, 0xf8},
    {NEXT,         19200, 0xf3}, {NEXT,         19200, 0xf2},
    {NEXT,         19200, 0xf1}, {NEXT,         19200, 0xf0},
    {NEXT,          9600, 0x0d}, {NEXT,          4800, 0xe6},
    {NEXT,          2400, 0x80}, {NEXT,          2400, 0x78},
  {TABLE_START,    38400, 0},
    {HI,           38400, 0x5a}, {HI,           57600, 0xe0},
    {NEXT,        115200, 0xff}, {NEXT,         19200, 0xe6},
    {NEXT,        115200, 0xfc}, {NEXT,        115200, 0xfd},
    {NEXT,        115200, 0xfe}, 
    {UI,           38400, 0x0d}, {UI,           57600, 0xe2},
    {UI,           57600, 0xe3}, {UI,           57600, 0xe7},
    {UI,           57600, 0xf1}, {UI,           57600, 0xf8},
  {TABLE_START,   115200, 0},
    {HI,          115200, 0x5a},
    {UI,          115200, 0x0d}, {UI,           57600, 0xe6},
  {TABLE_START,    19200, 0},
    {HI,           19200, 0x5a}, {UI,           19200, 0x0d},
    {UI,           57600, 0xfc}, {UI,           57600, 0xfd},
    {UI,           57600, 0xfe}, {UI,           57600, 0xff},
  {TABLE_START,     2400, 0},
    {HI,            2400, 0x5a}, {HI,            1200, 0x98},
    {UI,            2400, 0x0d}, {UI,            1200, 0xe6},
  {TABLE_START,     9600, 0},      /* make user give 2 CR to connect to UI */
    {UI,            9600, 0x0d},
  {TABLE_START,     4800, 0},
    {UI,            4800, 0x0d},
  {TABLE_START,     0,    0}};     /* This last entry ensures the search loop
                                      termination. DO NOT REMOVE */
#endif

#define TARGET_CLEAR_TIMEOUT  50
#define HOST_CLEAR_TIMEOUT  3000

#define HOST_READ_TIMEOUT   2000
#define TARGET_READ_TIMEOUT 2500    /* Arbitarrily large to be in read for
                                     * all host writes */
/* The host has to delay before sending a subsequent byte so it doesn't get
 * swallowed by the target clearing the channel. */
#define HOST_DELAY_TIMEOUT  (TARGET_CLEAR_TIMEOUT*2)


static void
clear_channel(fd, dev, timo)
int fd;
const DEV *dev;
int timo;
{
    unsigned char junk[100];

    /* Keep reading until we time out. */
    while ((*dev->dev_read)(fd, junk, sizeof junk, timo) > 0)
        ;
}

#define ACK 0x06
#define NAK 0x15

#ifdef TARGET
static const unsigned char ack = ACK;
static const unsigned char nak = NAK;
#define send_ack() (*dev->dev_write)(fd, &ack, 1)
#define send_nak() (*dev->dev_write)(fd, &nak, 1)


/* FIND ENTRY - params baud_rate to find and received char      
 *              This routines searches the target table for the sub-table
 *              corresponding to the baud_rate and then searches for the
 *              received char.  If found this routine returns the table
 *              index for the found entry.  If not found the index returned
 *              is set equal to ERR.  Baud rate of 0 is the standard default
 *              table all searches start at (9600 Baud). A second 9600 baud 
 *              table allows us to rewuire 2 0x0d chars for a 9600 baud UI
 *              connection which stops misconnects when garbage occurs on the
 *              serial line during autobaud.  This target table is  assumed to 
 *              end with a TABLE_START entry to ensure termination. */
static int
find_entry(unsigned int baud_rate, unsigned char recv)
{
    unsigned int i;

    for (i=0; i<sizeof(target_recv); i++)
        {
        if ((target_recv[i].recv_state == TABLE_START) && 
            (target_recv[i].baud == baud_rate)) 
            /* found table start */
            {
            while (1)  {
                if (target_recv[++i].recv_state == TABLE_START)  
                    break;    
                if (target_recv[i].recv_char == recv)
                    return i;
                }
            }
        }
    return ERR;
}

int
autobaud(int fd, const DEV *dev)
{
    unsigned int entry;
    unsigned char recv;
    int timeout, read_stat;

    entry = ERR;
    do {
        if (auto_timeout_select == TRUE)
            timeout = 100;
        else
            timeout = COM_WAIT_FOREVER;
        
        baud_rate = 0;

        if ((*dev->dev_baud)(fd, RS232_BAUD) != OK)
            return(ERR);

        do {
            /* Wait for the autobaud character from the host or CR from the
             * user. Now check the status of read.  If no char was read,
             * break to outer loop. */
            if (auto_timeout_select == TRUE)
                {
                if (pci_inuse() == OK)
                    return ERR;
                while (((read_stat = (*dev->dev_read)(fd, &recv, 1, timeout)) != 1))
                   {
                    if (pci_inuse() == OK)
                        return ERR;
                   }
                }
            else 
                read_stat = (*dev->dev_read)(fd, &recv, 1, timeout);

            /* At some baud rates, CR arrives as multiple
             * characters.  We recognize the baud rate based on
             * the first character received; subsequent ones need
             * to be discarded. */
            clear_channel(fd, dev, TARGET_CLEAR_TIMEOUT);
            break_flag = FALSE;

            if (read_stat != 1)
                {
                entry = 0;
                break;
                }

            /* Look up the received character in the table.  If it is not
             * found, start the outermost loop over again. */
            entry = find_entry(baud_rate, recv);
            if (entry == ERR)
                {
                entry = 0;
                send_nak();
                break;
                }

            if (target_recv[entry].recv_state == NEXT)
                {
                baud_rate = target_recv[entry].baud;
                /* Change to the new baud rate. */
                (*dev->dev_baud)(fd, baud_rate);

                /* If the table entry has a next field, listen again
                 * at the new baud rate.  Otherwise, we have found
                 * the proper baud rate. */
                timeout = TARGET_READ_TIMEOUT;
                }
        } while (target_recv[entry].recv_state == NEXT);

    } while (!((target_recv[entry].recv_state == HI) ||
               (target_recv[entry].recv_state == UI)));

    /* set the correct baud rate before sending out the ACK */ 
    baud_rate = target_recv[entry].baud;
    /* Change to the new baud rate. */
    (*dev->dev_baud)(fd, baud_rate);
    
    /* If the autobaud was done by a host, respond with an ACK. */
    if (target_recv[entry].recv_state == HI)
        {
        send_ack();
        host_connection = TRUE;
        }
    else
        host_connection = FALSE;

    return(OK);
}
#endif /* TARGET */

#ifdef HOST
int
autobaud(baud_rate, dev_type, fd, dev)
unsigned long baud_rate;
int dev_type;
int fd;
const DEV *dev;
{
    int i, j, nread, entry = ERR;
    unsigned char resp[10];

    for (i = 0; i < sizeof(host_send); i++)
        if (host_send[i].baud==baud_rate)
           {
           entry = i;
           break;
           }

    if ((dev_type != RS232) || (entry == ERR))
        {
        com_stat = E_ARG;
        return(ERR);
        }

    com_stat = E_COMM_TIMO;            /* Assume no response */
       
    for (j=0; j<3; j++)
        {
        for (i=0; i<(int)host_send[entry].size_to_send; i++)
            {
            /* clear  channel before sending first autobaud char */
            clear_channel(fd, dev, HOST_DELAY_TIMEOUT);
            (*dev->dev_write)(fd, &host_send[entry].send_chars[i], 1);
            }
        
        /* wait for the target to ACK. */
        /* Workaround for strange Windows interaction.
           Observation is that the FIRST serial interrupt reads
           one garbage character before it reads the desired ACK
           character.  So we'll read more than one charater. */
#if defined(MSDOS)
        i = 10;
#else
        i = 1;
#endif
        nread = (*dev->dev_read)(fd, resp, i, HOST_READ_TIMEOUT);
        if ( nread > 0 )
            {
            if (resp[nread - 1] == ACK)
                {
                /* Delay longer than the target before sending the
                 * next byte. If receiving chars restart sequence. */
                clear_channel(fd, dev, HOST_DELAY_TIMEOUT);
                return(OK);
                }

            /* not ACK, timeout or error  we are getting something*/
            clear_channel(fd, dev, HOST_CLEAR_TIMEOUT);
            com_stat = E_COMM_ERR;    /* We're getting something */
            }
        else if (j==2)
            {
            /* ACK, timeout or error delay before last try */
            clear_channel(fd, dev, HOST_CLEAR_TIMEOUT *2);
            com_stat = E_COMM_ERR;    
            }
        }

    return ERR;
}

#endif /* HOST */
