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

#include "ui.h"
#include "i960.h"
#include "hdi_errs.h"

extern int store_mem(ADDR, int mem_size, const void *data, int data_size, int verify);
extern prtf(), readbyte(), perror(), co(), leds();
extern int _Bbss, _Ebss, _Bdata, _Edata;

void cancel_download();

static int parser_state;        /* reentry state for COFF parser */

static struct {                /* FILE HEADER STRUCTURE    */
    unsigned short    file_type;    /*    file type        */
    unsigned short    num_secs;    /*    number of sections    */
    long        time_date;    /*    time and date stamp    */
    long        symtbl_ptr;     /*    symbol table ptr    */
    long        num_syms;     /*    num entries in symtab    */
    unsigned short    opt_hdr;    /*    size of optional header    */
    unsigned short    flags;        /*    flags            */
} filebuf;

/* The size of the above structures NOT INCLUDING any padding
 * added by the compiler for alignment of entries within arrays.
 */
#define FHDR_UNPADDED_SIZE (sizeof(filebuf.file_type) \
                + sizeof(filebuf.num_secs) \
                + sizeof(filebuf.time_date) \
                + sizeof(filebuf.symtbl_ptr) \
                + sizeof(filebuf.num_syms) \
                + sizeof(filebuf.opt_hdr) \
                + sizeof(filebuf.flags))

/* COFF parser states
 */
#define    P_INIT         0    /* parser initialize */
#define    P_SECT         1    /* parse sections */
#define    P_RAWDATA     2    /* parse raw data */
#define    P_DONE         3    /* parser finished */

/* XMODEM Constants */
#define ERRORMAX    15    /* max errors tolerated */
#define    XMODSIZE    128    /* size of xmodem buffer */

extern ADDR start_addr;        /* set after successful download */

static receive_xmodem(void);
static readpkt(unsigned char *buff);
extern void eat_time(int arg);

#define MAXSECTS	12  	/* max sections allowed in coff file */

struct sect { 		/* section header structure */
	char		sec_name[8];   	/* section name */
	long    	p_addr;		/* physical address */
	long    	v_addr;		/* virtual address */
	long    	sec_size;	/* size of sections */
	long    	data_ptr;	/* pointer to data */
	long    	reloc_ptr;      /* relocation pointer */
	long    	line_num_ptr;	/* line number pointer */
	unsigned short	num_reloc;	/* number of reloc entries */
	unsigned short	num_line; 	/* number line num entries */
	long    	flags;		/* flags */
	unsigned long	sec_align; 	/* alignment for sect bndry */
};

struct aout {		/* a.out header structure */
	unsigned short 	magic_nmbr;	/* magic number */
	unsigned short 	version;	/* version */
	long		text_size;	/* size of .text section */
	long		data_size;	/* size of .data section */
	long		bss_size;	/* size of .bss section */
	long		start_addr; 	/* starting address */
	long 		text_begin;	/* start of text section */
	long		data_begin; 	/* start of data section */
};


static struct aout aoutbuf;
static struct sect sectbuf[MAXSECTS];
static int memoffset;

/* The following are sizes of the above structures NOT INCLUDING any padding
 * added by the compiler for alignment of entries within arrays.
 */
#define SHDR_UNPADDED_SIZE (sizeof(sectbuf[0].sec_name) \
				+ sizeof(sectbuf[0].p_addr) \
				+ sizeof(sectbuf[0].v_addr) \
				+ sizeof(sectbuf[0].sec_size) \
				+ sizeof(sectbuf[0].data_ptr) \
				+ sizeof(sectbuf[0].reloc_ptr) \
				+ sizeof(sectbuf[0].line_num_ptr) \
				+ sizeof(sectbuf[0].num_reloc) \
				+ sizeof(sectbuf[0].num_line) \
				+ sizeof(sectbuf[0].flags) \
				+ sizeof(sectbuf[0].sec_align))

#define AOUT_UNPADDED_SIZE (sizeof(aoutbuf.magic_nmbr) \
				+ sizeof(aoutbuf.version) \
				+ sizeof(aoutbuf.text_size) \
				+ sizeof(aoutbuf.data_size) \
				+ sizeof(aoutbuf.bss_size) \
				+ sizeof(aoutbuf.start_addr) \
				+ sizeof(aoutbuf.text_begin) \
				+ sizeof(aoutbuf.data_begin))



static int
check_for_monitor_overwrite(ADDR start_addr, int mem_size)
{
#if MRI_CODE
#else /*intel ic960 or gcc960*/
    if ( ((start_addr <= (ADDR)&_Ebss) && 
          (start_addr + (ADDR)mem_size >= (ADDR)&_Bbss))  ||
         ((start_addr <= (ADDR)&_Edata) && 
          (start_addr + (ADDR)mem_size >= (ADDR)&_Bdata)) ||
          (start_addr < 0x400) )
        {
        return(ERR);
        }
#endif

    return (OK);
}


/************************************************/
/* Download                              */
/*                                       */
/************************************************/
int
download(int dummy, int nargs, int offset)
{
    extern int user_stack[];

    memoffset = (nargs == 1) ? offset : 0;

    prtf ("Downloading\n");

    parser_state = P_INIT;
    downld = TRUE;
    if (receive_xmodem() == ERR){
        while (readbyte(2) != TIMEOUT){
            ;    /* wait for line to settle */
        }
        prtf("\n");
        perror("Download failed");
    } else {
        start_addr = aoutbuf.start_addr + memoffset;
        register_set[REG_IP] = start_addr;

        while(readbyte(1) != TIMEOUT)
            ; /* wait for silence on the line before printing */
        prtf("\n -- Download complete --\n");
        prtf(  "    Start address is : %X\n", start_addr);
    }

    /*
     * The stack must always be set up after a program is loaded.
     * So we may as well do it automatically instead of requiring
     * the user to do it by hand.  (This is redundant for the very
     * first load after a reset, but necessary for subsequent loads.)
     *
         * Set frame, stack pointer to point at application stack
     */
    register_set[REG_FP]  = (REG)user_stack;
    register_set[REG_PFP] = 0;
    register_set[REG_SP]  = (REG)user_stack + 0x40;

    /* Clear flag */
    downld = FALSE;
}

/****************************************/
/* COFF Parser                 */
/*                     */
/* The coff file is parsed as each      */
/* packet is received.  The state of    */
/* parsing is saved between each packet */
/* so that the parser can be reentered  */
/****************************************/
static int parse_coff( buff )
unsigned char *buff;
{
int i, j;
int size, a, b;
unsigned char *p;
static ADDR secbuffptr;            /* pointer to section info */
static int buffindex;            /* place holder for xmodem buffer */
static int current_section;        /* section number        */
static int current_byte;        /* byte within section        */


switch (parser_state) {

case P_INIT: /* transfer buffer to file header info */

    for ( p=(unsigned char*)&filebuf, i=0; i<FHDR_UNPADDED_SIZE; p++, i++ ){
        *p = buff[i];
    }

    buffindex = FHDR_UNPADDED_SIZE;

    /* transfer buffer to optional info 
       only if AOUT is present as OHDR */

    if (filebuf.opt_hdr >= AOUT_UNPADDED_SIZE){
        p = (unsigned char*)&aoutbuf;
        for (  i=0; i<AOUT_UNPADDED_SIZE; p++, i++ ){
            *p = buff[buffindex + i];
        }
    }
    
    buffindex += filebuf.opt_hdr;

    /* set up start section and start word */
    current_section = 0;
    current_byte = 0;

    if (buffindex > XMODSIZE) {
        /* sorry, this condition shouldn't happen */
        cmd_stat = E_FILE_ERR;
        return(ERR);
    }
    else if (buffindex == XMODSIZE)    {
        parser_state = P_SECT;
        return(0);
    }
    
    /* most likely case is some section stuff in first buffer */
    
case P_SECT:
    
    /* if code is from above, then don't reset buffer */
    if (parser_state == P_SECT) buffindex = 0;
    /* and away we go......... */

    for (i = current_section; i < filebuf.num_secs; i++) {
        p = (unsigned char*) &sectbuf[i];
        for (j=current_byte; j < SHDR_UNPADDED_SIZE; j++) {
            if (buffindex == XMODSIZE) {
                /* done with packet, save state */
                current_byte = j;
                current_section = i;
                parser_state = P_SECT;
                return(0);
            } else {
                /* need to process byte */
                p[j]= buff[buffindex];
                buffindex++;
            }
        }
        current_byte = 0;
    }
    /* done with headers, so continue on raw data */
    current_section = 0;
    current_byte = 0;

case P_RAWDATA:
    
    /* if code is from above, then don't reset buffer */
    if (parser_state == P_RAWDATA) buffindex = 0;

    /* transfer raw data to respective sections */
    for (i=current_section; i<filebuf.num_secs; i++) {
        if (current_byte == 0){
            secbuffptr = sectbuf[i].p_addr;
        }

        if (sectbuf[i].data_ptr == 0) {
            continue;    /* skip NOLOAD sections */
        }

        /* finish the section */
        for (j=current_byte; j<sectbuf[i].sec_size; j++) {

            /* done with packet, save state */
            if (buffindex == XMODSIZE) {
                current_byte = j;
                current_section = i;
                parser_state = P_RAWDATA;
                return(0);
            }

            /* process packet */

            /* set size to smaller of bytes left in packet
             * or bytes left in section
             */
            a = XMODSIZE - buffindex;
            b = sectbuf[i].sec_size - current_byte;
            size = a < b ? a : b;

            if (check_for_monitor_overwrite(secbuffptr+memoffset, size) != OK)
                {
                co(NAK);
                cancel_download();
                cmd_stat=E_WRITE_ERR;
                perror("Attempt to overwrite Mon960 data during download\n");
                perror("ERROR: Section address is : %X\n", secbuffptr);
                return(ERR);
                }

            if (store_mem(secbuffptr + memoffset, BYTE,
                      buff + buffindex, size, TRUE) != OK)
            {
                cancel_download();
                return(ERR);
            }

            secbuffptr += size;
            buffindex += size;
            j += (size - 1);
        }
        current_byte = 0;
    }
    parser_state = P_DONE;


case P_DONE:
    /* YEA!  we're done! */
        return(0);
}
}


/****************************************/
/* Receive Xmodem Transmission        */
/*                     */
/* this function is in charge of the    */
/* handshaking protocol between the     */
/* monitor and Xmodem.            */
/****************************************/
static int receive_xmodem()
{
int c=0;            /* 1st byte of packet */
int errors;        /* Running count of errors */
int errorflag;        /* True when error found while reading a packet    */
int fatalerror;        /* True when fatal error found while reading packet */
unsigned char pktnum;    /* 2nd byte of packet: packet # (mod 128) */
unsigned char pktchk;    /* 3rd byte of packet: should be complement of pktnum */
int pktcnt;        /* total # of packets received so far */
unsigned char buff[XMODSIZE+10];/* buffer for data        */
int initial_tries;
int first_time;

    fatalerror = FALSE;
    errors = pktcnt = 0;

    /* start communications by providing NAK to the sending
     * end.  The number and time over which these are provided
     * is 10 seconds at once per second, followed by about
     * one minute at 4 second intervals.  the maximum tolerable
     * inter-character time is 3 seconds, and if that occurs,
     * we will wait for 1 second of silence from the host and
     * then NAK the packet.
     */
    /* the Sweet Spot timer */
    /* about 36ms on a 33Mhz TomCAt with CA */
    /* about 36ms on a 33Mhz HK with CA */
    /* about 48ms on a 25Mhz EVCA with CA */
    /* about 263ms on a 16Mhz EVSX with SB */
    /* about 210ms on a 20Mhz QTKB with KB */
    eat_time(10000);
    initial_tries = 10;
    do{
        co(NAK);
        initial_tries--;
    } while(initial_tries && ((c = readbyte(1)) == TIMEOUT));

    first_time = TRUE;

    /* MAIN Do-While loop to read packets */
    do {
        /* read first byte in packet */
        /* If this is the first time through the loop, it was
         * read above. */
        if (!first_time)
            c = readbyte(3);
        else
            first_time = FALSE;

        errorflag = FALSE;

        switch (c){
        default:
            continue;

        case ACK:
            if (pktcnt > 0){
                continue;
            }
            break;

        case EOT:
            /* check for REAL EOT */
            if ((c = readbyte(1)) == TIMEOUT) {
                c = EOT;
            } else if (c != EOT) {
                c = TIMEOUT;
                errorflag = TRUE;
            }
            break;

        case TIMEOUT:
            errorflag = TRUE;
            break;

        case CAN:     /* bailing out? */
            if ((readbyte(3) & 0x7f) != CAN){
                errorflag = TRUE;
            } else {
                fatalerror = TRUE;
                cmd_stat = 0;
            }
            break;

        case SOH:
            /* start reading packet */
            pktnum = readbyte(3);
            pktchk = readbyte(3);

            if (pktnum != (unsigned char)~pktchk) {
                /* MISREAD PACKET # */
                errorflag = TRUE;
                break;
            }

            if (pktnum == (pktcnt & 0xff)){
                /* DUPLICATE PACKET -- DISCARD */
                while (readbyte(3) != TIMEOUT){ ; }
                co(ACK);
                break;
            }

            if ( pktnum != ((pktcnt+1) & 0xff) ){
                /* PHASE ERROR */
                fatalerror = TRUE;
                cmd_stat = E_COMM_ERR;
                break;
            }


            /* Read and calculate checksum for a packet of data */

            if ( !readpkt(buff) ){
                errorflag = TRUE;
                break;
            }

            errors = 0;
            pktcnt++;
            if (parse_coff(buff)== ERR) {
                fatalerror = TRUE;
                break;
            }

            /* ACK the packet */
            co(ACK);
            break;

        } /* end of switch */

        /* check on errors or batch transfers */
        if (errorflag || pktcnt == 0) {
            if (errorflag){
                errors++;
            }

            if (!fatalerror && !break_flag)
            {
                /* wait for line to settle */
                while (readbyte(2) != TIMEOUT){
                    ;
                }
                co(NAK);
            }
        }

    } while (c != EOT && errors < ERRORMAX && !fatalerror && !break_flag);

    /* end of MAIN Do-While */

    if (c == EOT && errors < ERRORMAX) {
        /* normal exit */
        co(ACK);
        return(0);
    } else {
        /* error exit */
        if (pktcnt != 0 && !break_flag)
            cancel_download();

        if (break_flag)
            cmd_stat = E_INTR;
        else if (!fatalerror)
            cmd_stat = E_COMM_ERR;

        return(ERR);
    }
}

/************************************************/
/* read buffer                                  */
/*                                              */
/* Get a buffer (XMODSIZE bytes) from uart.    */
/* Timeout if 3 "seconds" elapse.         */
/*                                              */
/* Return 1 on success, 0 on TIMEOUT or bad     */
/*    checksum.                */
/************************************************/
static int readpkt(buff)
unsigned char *buff;
{
int count;         /* Number of characters read */
int c;            /* Next character read */
unsigned char chksum;    /* accumulate checksum here */

    chksum = 0;

    for (count=0; count<XMODSIZE; count++) {
        buff[count] = c = readbyte(3);
        if ( c == TIMEOUT ){
            return(0);    /* abort operation */
        }
        chksum += c;
    }

    /* Read and confirm checksum */
    c = readbyte(3);
    if (c == TIMEOUT)
    {
        return(0);
    }
    if  ((c & 0xff) != chksum)
    {
        return(0);
    }

    return(1);
}

void
cancel_download()
{
    if (downld) {
        co (CAN);
        co (CAN);
        eat_time(10);
    }
    return;
}
