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
 * Implements a packet-based error correcting protocol on top of a byte-stream.
 */
/* $Header: /ffs/p1/dev/src/hdilcomm/common/RCS/packet.c,v 1.9 1995/11/02 22:38:56 gorman Exp $$Locker:  $ */

#include "common.h"
#include "hdi_errs.h"
#include "hdi_com.h"
#include "com.h"
#include "dev.h"

#ifdef TARGET
            extern  void blink();
            extern  int  pci_comm(), serial_comm();
#endif				

#ifdef HOST
int target_reset;
#endif

#define TIMEOUT -1
#define BAD_PKT -2
#define BAD_SEQ -3
#define INIT_SEQ -4
#define BAD_MON  -5

#define    SOH        01
#define    STX        02
#define    ETX        03
#define    ACK        06
#define    NAK        21


#define    ADDCRC(crc,ch)    (crc=((crc >> 8) & 0xFF) ^ CRCtab[(crc ^ (ch)) & 0xFF])

#define    ENCODELEN(header, len)    (((header)[P_HLEN] = (((len)&0xFC0)>>6)+0x60), \
                   ((header)[P_LLEN] = ((len) & 0x3F) + 0x60))
#define    PACKET_SIZE(header)    (((((header)[P_HLEN] - 0x60) << 6) & 0xfc0) \
                   + (((header)[P_LLEN] - 0x60) & 0x3f))

/*
 * packet "templates"
 */
#define HEADER_SIZE    5
#define P_SOH        0
#define P_LLEN        1
#define P_HLEN        2
#define P_SEQ        3
#define P_STX        4

#define TRAILER_SIZE    3
#define P_ETX        0
#define P_LCRC        1
#define P_HCRC        2

#define DFLT_RESET_TIME 3000

#define DFLT_INTERTIMO 5000
#define DFLT_INTRATIMO 2000
#define DFLT_ACKTIMO   5000
#define DFLT_MAXTRY    3

#define MAXPKTSZ 4095

static int      do_read            HDI_PARAMS((unsigned char *, int, int, int));
static int      pkt_put_once       HDI_PARAMS(
                                       (const unsigned char *, int, int));
static void     pkt_clear_channel  HDI_PARAMS((int));
static int      pkt_get            HDI_PARAMS((unsigned char *, int, int));
static int      pkt_get1           HDI_PARAMS((unsigned char *, int, int, int));
static int      pkt_init           HDI_PARAMS((void));
static int      pkt_intr_trgt      HDI_PARAMS((void));
static int      pkt_put            HDI_PARAMS((const unsigned char *, int));
static int      pkt_reset          HDI_PARAMS((int));
static void     pkt_signal         HDI_PARAMS((void));
static int      pkt_term           HDI_PARAMS((void));

#ifdef HOST
static int      send_init_pkt      HDI_PARAMS((void));
#else
static void     handle_init_pkt    HDI_PARAMS((unsigned char *));
#endif

extern const unsigned short CRCtab[];

/* --------------------------- EXPORTED DATA ----------------------------- */
PKTDEV byte_pkt;

/* ---------------------------- LOCAL DATA ------------------------------- */
typedef struct {
    unsigned short    inter_timo;
    unsigned short    intra_timo; /* Not used; for backward compatibility */
    unsigned short    ack_timo;
    unsigned short    max_len;
    unsigned char    max_try;
} PKT_MODE;

static const PKT_MODE dflt_mode = { DFLT_INTERTIMO, DFLT_INTRATIMO,
                    DFLT_ACKTIMO, MAXPKTSZ, DFLT_MAXTRY };

static PKT_MODE cur_mode;
static const DEV *dev = NULL;
static int fd = -1;
static const unsigned char ack = ACK;
static const unsigned char nak = NAK;
#ifdef HOST
static int interrupted;
#endif

#define send_ack() (*dev->dev_write)(fd, &ack, 1)
#define send_nak() (*dev->dev_write)(fd, &nak, 1)

#define INTRA_TIMEOUT (cur_mode.ack_timo / 5)
#define CLEAR_TIMEOUT ((cur_mode.ack_timo*2)/(unsigned)5)

#define CLEAR_POLL -1
#define CLEAR_WAIT 0


/*
 * Init packet function pointers at runtime to permit Mon960 communication
 * protocol SW to be compiled PIC/PID and then subsequently dynamically
 * relocated on an ApLink target.
 */
void
pkt_protocol_setup()
{
    byte_pkt.pkt_init      = pkt_init;
    byte_pkt.pkt_term      = pkt_term;
    byte_pkt.pkt_get       = pkt_get;
    byte_pkt.pkt_put       = pkt_put;
    byte_pkt.pkt_put_once  = pkt_put_once;
    byte_pkt.pkt_reset     = pkt_reset;
    byte_pkt.pkt_signal    = pkt_signal;
    byte_pkt.pkt_intr_trgt = pkt_intr_trgt;

    com_pkt_ptr_init();  /* PIC/PID assist. */
    dev = com_dev();     /* PIC/PID assist */
}



static int
pkt_init()
{
    if ((fd = (*dev->dev_open)()) < 0)
        return(ERR);

    cur_mode = dflt_mode;
    return(OK);
}

static int
pkt_term()
{
    int r = OK;

    if (fd >= 0 && dev && dev->dev_close)
    {
        if ((*dev->dev_close)(fd) < 0) {
        r = ERR;
        }
    }
    fd = -1;

    return(r);
}


/*
 * get a packet
 * Return value is the size of the data if successful, else 0.
 * If there is an error receiving a packet (either a timeout or
 * a bad crc), wait for the channel to clear (nothing received
 * for 2 seconds), send a NAK, and retry.
 * Wait may be COM_POLL, COM_WAIT, or COM_WAIT_FOREVER.  COM_POLL
 * returns immediately if there is no packet available; COM_WAIT
 * waits for the usual length of time, sending NAK if nothing is
 * received, and retrying.
 */
static int
pkt_get(buf, size, wait)
unsigned char    *buf;
int    size;
int    wait;
{
    int r, inter_timo = 0, retries = 0;

#ifdef HOST
    interrupted = FALSE;
#endif

    switch (wait)
    {
        case COM_POLL:
        case COM_WAIT_FOREVER:
            inter_timo = wait;
            break;
        default:
            inter_timo = cur_mode.inter_timo;
            break;
    }

    while ((r = pkt_get1(buf, size, inter_timo, INTRA_TIMEOUT)) < 0)
        {
        if (com_stat == E_INTR)
            return(ERR);

        com_stat = E_COMM_TIMO;

        if (inter_timo == COM_POLL && r == TIMEOUT)
            return(0);

        switch (r)
            {
#ifdef TARGET
            case INIT_SEQ:
                send_ack();
                handle_init_pkt(buf);
                continue;
#endif
            case BAD_SEQ:
                send_ack();
                break;

            case BAD_MON:
                pkt_clear_channel(CLEAR_TIMEOUT);
                send_ack();
                com_stat = E_COMM_ERR;
                return(ERR);

            default:
                   /* fall through */
            case BAD_PKT:
                pkt_clear_channel(CLEAR_TIMEOUT);
                com_stat = E_COMM_ERR;

#if defined(HOST) && defined(MSDOS)
                if (_com_config.dev_type == RS232)
                {
                    /*
                     * It has been empirically determined that when an app 
                     * reads/writes large amounts of data at high baud
                     * rates, the PC UART shuts down, causing generation
                     * of BAD_PKTs.  We workaround this problem by
                     * reinitializing the UART.  Subsequently sending a
                     * NAK to the target will restart comm.
                     */

                    (void) (*dev->dev_close)(fd);
                    fd = (*dev->dev_open)();
                }
#endif
                send_nak();

                /* fall through */
            case TIMEOUT:
				/* no ack on timeout because no data was received */
                if (++retries == (int)cur_mode.max_try)
                    {    
                    /* if HOST we check for reset by autobaud sequence */
#ifdef HOST
                    if (pkt_reset(0) == OK)
                        {
                        target_reset = TRUE;
                        com_stat = E_TARGET_RESET;
                        }
#endif
                    return(ERR);
                    }

                break;
            }

        inter_timo = cur_mode.inter_timo;
        }

    /* If subsequent data has been received, it must be a duplicate
     * of the current packet.  Clear it out to avoid synchronization
     * problems. */
    pkt_clear_channel(CLEAR_WAIT);
    send_ack();

    return(r);
}

static int
pkt_get1(buf, size, inter_timeout, intra_timeout)
unsigned char    *buf;
int size;
int inter_timeout, intra_timeout;
{
    static int recv_seq = 0;
    unsigned char header[HEADER_SIZE], trailer[TRAILER_SIZE];
    unsigned short    crc, rcrc;
    int    to_recv, read_err;
    unsigned char *p;

    if ((read_err = do_read(header, HEADER_SIZE, inter_timeout, intra_timeout)) != OK)
    {
        return(read_err);        /* return BAD_PKT or TIMEOUT */
    }

    if ((to_recv = PACKET_SIZE(header)) == 0)
		{
		/* packet with no data means monitor overwritten */
        return(BAD_MON);
		}

    if (header[P_SOH] != SOH || header[P_STX] != STX || to_recv > size)
    {
        return(BAD_PKT);
    }

    if (to_recv > 0)
        if (do_read(buf, to_recv, intra_timeout, intra_timeout) != OK)
        {
        return(BAD_PKT);
        }

    if (do_read(trailer, TRAILER_SIZE, intra_timeout, intra_timeout) != OK)
    {
        return(BAD_PKT);
    }

    if (trailer[P_ETX] != ETX)
    {
        return(BAD_PKT);
    }


    crc = 0;
    ADDCRC(crc, header[P_LLEN]); 
    ADDCRC(crc, header[P_HLEN]); 
    ADDCRC(crc, header[P_SEQ]); 
    for (p = buf; p < &buf[to_recv]; p++)
        ADDCRC(crc,*p);

    rcrc = (trailer[P_HCRC] << 8) | trailer[P_LCRC];
    if (rcrc != crc)
    {
        return(BAD_PKT);
    }
    
#if defined(__GNUC__)
    if ((signed char)header[P_SEQ] == INIT_SEQ)
#else
    if ((char)header[P_SEQ] == INIT_SEQ)
#endif /* __GNUC__ */
       {
        return(INIT_SEQ);
       }

    /* Multi-packet messages have sequence numbers starting with 1.
     * Single packet messages have sequence number 0.  Sequence number
     * 0 is accepted at all times, even though it should never be
     * received after sequence number 1; this allows the communications
     * to automatically resynchronize after a failure on the other end.
     * Sequence number 1 is accepted at all times except after sequence
     * number 1.
     */
    if (!(header[P_SEQ] == 0
          || (int)header[P_SEQ] == recv_seq + 1
          || (recv_seq != 1 && header[P_SEQ] == 1)))
    {
        return(BAD_SEQ);
    }

    recv_seq = header[P_SEQ];

    return(to_recv);
}

static int
do_read(buf, size, inter_timeout, intra_timeout)
unsigned char    *buf;
int    size;
int inter_timeout, intra_timeout;
{
    int to_read = size;
    int timo;
    int actual;

    timo = inter_timeout;
    while (to_read)
    {
        if ((actual = (*dev->dev_read)(fd, buf, to_read, timo)) <= 0)
        {
#ifdef HOST
            /* If this is the first interrupt, we want to finish
             * reading what we are in the middle of.  If this is
             * not the first interrupt, give up immediately.  If
             * timo is 0, we are not in the middle of anything,
             * and we should return immediately.
             */
            if (com_stat != E_INTR || interrupted || timo <= 0)
                if (to_read < size)        /* error return */
                    return(BAD_PKT);    /* got some not all */
                else
                    return(TIMEOUT);    /* no data read */

            interrupted = TRUE;
            actual = 0;
#else
            return(ERR);
#endif
        }
        to_read -= actual;
        buf += actual;
        timo = intra_timeout;
    }
    return(OK);
}

/*
 * put a packet
 * Return value is the size of the data if successful, else 0.  The 
 * attempt is aborted if the data being sent is too large for the protocol, 
 * or any character output times out.  The data is encapsulated in a 
 * packet.  A running 16 bit crc starts with the low length and runs up 
 * to the crc itself.
 */
static int
pkt_put(data, size)
const unsigned char *data;
int    size;
{
    int to_put;
    int retries;
    int response;
    int send_seq = 0;

    if (size > (int)cur_mode.max_len)
        send_seq = 1;

#ifdef HOST
    interrupted = FALSE;
#endif

    while (size > 0)
    {
        to_put = size;
        if (to_put > (int)cur_mode.max_len)
            to_put = cur_mode.max_len;

        retries = 0;

        while ((response = pkt_put_once(data, to_put, send_seq)) != ACK)
            {
            if (com_stat == E_INTR)
				{
#ifdef TARGET
			    blink(7);
#endif				
                return(ERR);
				}

            /* If we get SOH from the other end, it is probably trying
             * to send to us, too.  If we are retrying, it is probably
             * because we lost its last ACK; otherwise it is probably[
             * because it lost our last ACK.  In the first case, act as
             * if we got an ACK.  In the second case, keep trying.
             */
            if (response == SOH)
                {
                if (retries > 0 && to_put == size)
                    break;

                pkt_clear_channel(CLEAR_TIMEOUT);
                }

            else if (++retries == (int)cur_mode.max_try)
                {    /* response = -1 (timeout) */
#ifdef HOST
                /* if HOST we check for reset by autobaud sequence */
                pkt_clear_channel(CLEAR_TIMEOUT);
                if (pkt_reset(0) == OK)
                    {
                    target_reset = TRUE;
                    com_stat = E_TARGET_RESET;
                    return(ERR);
                    }
#endif
                com_stat = E_COMM_ERR;
                return(ERR);
                }

            /* If we received a NAK, probably we don't need to
             * clear the channel, so let's not waste the time.
             * If we need to clear the channel and don't,
             * the next retry will be a waste, so don't take the
             * chance if the next retry is our last. */
            else if (response != -1 && (response != NAK ||
                     retries == (int)cur_mode.max_try - 1))
                pkt_clear_channel(CLEAR_TIMEOUT);

            } /* while */

        data += to_put;
        size -= to_put;
        send_seq++;
    }

    return(OK);
}

static int
pkt_put_once(data, size, send_seq)
const unsigned char *data;
int size;
int send_seq;
{
    unsigned char header[HEADER_SIZE], trailer[TRAILER_SIZE];
    unsigned short    crc;
    const unsigned char *p;
    unsigned char response;

    header[P_SOH] = SOH;
    ENCODELEN(header, size);
    header[P_SEQ] = send_seq;
    header[P_STX] = STX;

    if ((*dev->dev_write)(fd, header, HEADER_SIZE) == ERR)
        return ERR;

    crc = 0;
    ADDCRC(crc, header[P_LLEN]); 
    ADDCRC(crc, header[P_HLEN]); 
    ADDCRC(crc, header[P_SEQ]); 

    if ((*dev->dev_write)(fd, data, size) == ERR)
        return ERR;

    for (p = data; p < &data[size]; p++)
        ADDCRC(crc,*p);

    trailer[P_ETX] = ETX;
    trailer[P_LCRC] = crc & 0xff;
    trailer[P_HCRC] = (crc >> 8) & 0xff;

    if ((*dev->dev_write)(fd, trailer, TRAILER_SIZE) == ERR)
        return ERR;

    if ((*dev->dev_read)(fd, &response, 1, (int)cur_mode.ack_timo) <= 0)
    {
#ifdef HOST
        if (com_stat != E_INTR || interrupted)
        {
        return(ERR);
        }

        interrupted = TRUE;

        /* Continue on interrupt */
        if ((*dev->dev_read)(fd, &response,1,(int)cur_mode.ack_timo) <= 0)
#endif
        {
        return(ERR);
        }
    }

    /* return(response); */
    return(response);
}

static void
pkt_clear_channel(arg)
int arg;
{
    int timo = 0;
    unsigned char junk[100];
    int max_reads = MAXPKTSZ+1, r;

#ifdef HOST
    if (_com_config.dev_type == HDI_PCI_COMM)
       return;
#endif      
#ifdef TARGET
    if (pci_comm() == TRUE)
        return;
#endif

    /* If timeout is 0, use the default. */
    switch (arg)
    {
    case CLEAR_POLL: timo = COM_POLL; break;
    case CLEAR_WAIT: timo = COM_POLL; break;
    default:         timo = arg;      break;
    }
    
    /* Keep reading until we time out. */
    while (max_reads > 0)
    {
    if ((r = (*dev->dev_read)(fd, junk, sizeof junk, timo)) <= 0)
    {
#ifdef HOST
        /* Continue on interrupt */
        if (com_stat != E_INTR || interrupted)
            break;
        interrupted = TRUE;
        r = 0;
#else
        break;
#endif
    }

    /* If timeout is CLEAR_WAIT, after we get something use
     * CLEAR_TIMEOUT for further reads. */
    if (arg == CLEAR_WAIT)
        timo = CLEAR_TIMEOUT;

    max_reads -= r;
    }
}



/* S message offsets */
#define SP_LINTRA       0
#define SP_HINTRA       1
#define SP_LINTER       2
#define SP_HINTER       3
#define SP_L_ACK        4
#define SP_H_ACK        5
#define SP_LMAXLEN      6
#define SP_HMAXLEN      7
#define SP_MAXTRY       8
#define SPKTSZ          9

static int
pkt_reset(reset_time)
int reset_time;
{
    cur_mode = dflt_mode;

    /* Clear the channel, and wait for the reset period to pass.
     * (On the target, reset_time is 0.) */
    if (reset_time < 0)
        reset_time = DFLT_RESET_TIME;
    else if (reset_time == 0)
        reset_time = CLEAR_POLL;
    pkt_clear_channel(reset_time);

#ifdef HOST
    if (_com_config.dev_type == RS232)
        if (autobaud(_com_config.baud, _com_config.dev_type, fd, dev) == ERR)
            return(ERR);

    /* Send communications parameters to the target.  This must be done
     * using the default values for local parameters, since the target
     * is still using default values. */
    if (send_init_pkt() != OK)
        return(ERR);

    /* Now that the correct parameters have been sent to the target,
     * we can use them locally. */
    if (_com_config.host_pkt_timo)
        cur_mode.inter_timo = _com_config.host_pkt_timo;
    if (_com_config.ack_timo)
        cur_mode.ack_timo = _com_config.ack_timo;
    
    if (_com_config.max_len)
    {
        cur_mode.max_len = _com_config.max_len;
        if (cur_mode.max_len > MAXPKTSZ)
        cur_mode.max_len = MAXPKTSZ;
    }
    if (_com_config.max_try)
        cur_mode.max_try = _com_config.max_try;
#endif /* HOST */

#ifdef TARGET
    if (serial_comm() == TRUE)
        if (autobaud(fd, dev) == ERR)
            return(ERR);
#endif /* TARGET */

    return(OK);
}

#ifdef HOST
/* Send communications parameters to the target. */
static int
send_init_pkt()
{
    PKT_MODE remote;
    unsigned char buf[SPKTSZ];
    int retries;
    int response;

    remote = dflt_mode;
    if (_com_config.target_pkt_timo)
        remote.inter_timo = _com_config.target_pkt_timo;
    if (_com_config.ack_timo)
    {
        remote.ack_timo = _com_config.ack_timo;
        /* This field is only used by old monitors. */
        remote.intra_timo = (remote.ack_timo / 5);
    }

    if (_com_config.max_len)
    {
        remote.max_len = _com_config.max_len;
        if (remote.max_len > MAXPKTSZ)
        remote.max_len = MAXPKTSZ;
    }
    if (_com_config.max_try)
        remote.max_try = _com_config.max_try;

    buf[SP_HINTER] = (unsigned char)(remote.inter_timo >> 8);
    buf[SP_LINTER] = (unsigned char)remote.inter_timo;
    buf[SP_HINTRA] = (unsigned char)(remote.intra_timo >> 8);
    buf[SP_LINTRA] = (unsigned char)remote.intra_timo;
    buf[SP_H_ACK] = (unsigned char)(remote.ack_timo >> 8);
    buf[SP_L_ACK] = (unsigned char)remote.ack_timo;
    buf[SP_HMAXLEN] = (unsigned char)(remote.max_len >> 8);
    buf[SP_LMAXLEN] = (unsigned char)remote.max_len;
    buf[SP_MAXTRY] = (unsigned char)remote.max_try;

    retries = 0;
    while ((response = pkt_put_once(buf, sizeof(buf), INIT_SEQ)) != ACK)
    {
        if (com_stat == E_INTR)
            return(ERR);

        if (++retries == (int)cur_mode.max_try)
        {
            com_stat = E_COMM_ERR;
            return(ERR);
        }

        pkt_clear_channel(CLEAR_TIMEOUT);
    }

    return(OK);
}
#endif /* HOST */


#ifdef TARGET
static void
handle_init_pkt(unsigned char *buf)
{
    /* Handle communications parameters sent from the host. */
    cur_mode.inter_timo = (buf[SP_HINTER] << 8) | buf[SP_LINTER];
    cur_mode.intra_timo = (buf[SP_HINTRA] << 8) | buf[SP_LINTRA];
    cur_mode.ack_timo = (buf[SP_H_ACK] << 8) | buf[SP_L_ACK];
    cur_mode.max_len = (buf[SP_HMAXLEN] << 8) | buf[SP_LMAXLEN];
        if (cur_mode.max_len > MAXPKTSZ)
        cur_mode.max_len = MAXPKTSZ;
    cur_mode.max_try = buf[SP_MAXTRY];
}
#endif /* TARGET */


static void
pkt_signal()
{
#ifdef HOST
    (*dev->dev_signal)(fd);
#endif
}

static int
pkt_intr_trgt()
{
#ifdef HOST
    return (*dev->dev_intr_trgt)(fd);
#else
    return(0);
#endif /* HOST */
}

