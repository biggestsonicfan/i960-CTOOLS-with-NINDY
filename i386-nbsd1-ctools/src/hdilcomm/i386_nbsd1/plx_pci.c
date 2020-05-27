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

/*
 * $Header: /ffs/p1/dev/src/hdilcomm/gorman/RCS/plx_pci.c,v 1.15 1996/01/17 16:52:40 gorman Exp $
 *
 *
 * MODULE
 *     plx_pci.c
 *
 * PURPOSE
 *     A collection of routines to support DOS-based PCI download.
 *     The intent of this module is to be able to support access to
 *     more than one particular card.  Currently, these routines
 *     are compatible with a Cyclone PCI baseboard.
 */

#include <stdio.h>
#include <conio.h>
#include <dos.h>

#include "dbg_mon.h"
#include "hdi_com.h"
#include "com.h"
#include "dev.h"
#include "dos_pci.h"
#include "private.h"
#include "cyc9060.h"

#ifndef WIN95
#include "funcdefs.h"
#endif /* WIN95 */
 
extern int read_pci_cfg_obj  HDI_PARAMS((COM_PCI_CFG *, int, int, void *));
extern int write_pci_cfg_obj HDI_PARAMS((COM_PCI_CFG *, int, int, void *));

extern short CRCtab[];
extern DOS_PCI_INFO pci;

#if defined(WIN95)
#   define IND(x)     in_portd(x)
#   define OUTD(x,y)  out_portd(x,y)
#elif defined(__HIGHC__)
#   define IND(x)     _ind(x)
#   define OUTD(x,y)  _outd(x,y)
#else /* MSDOS */
#   define IND(x)     inpd(x)
#   define OUTD(x,y)  outpd(x,y)
#endif

#define DATA_MB    *data_mb.lp
#define STATUS_MB  *status_mb.lp
#define DOORBELL_REG *doorbell_reg.lp

#define IN_DATA_MB    IND(data_addr)
#define IN_STATUS_MB  IND(status_addr)
#define IN_DOORBELL_REG  IND(doorbell_addr)
#define OUT_DATA_MB(x)    OUTD(data_addr,x)
#define OUT_STATUS_MB(x)  OUTD(status_addr,x)
#define OUT_DOORBELL_REG(x)  OUTD(doorbell_addr,x)

#ifdef WIN95
/* Target time for timeouts. */
static long tgttime;
#define settimeout(a,b) (tgttime = time(NULL) + (a+500)/1000)
#define timo_flag (tgttime <= time(NULL))

static unsigned long
in_portd (unsigned short port);

static void
out_portd (unsigned short port, unsigned long value);
#endif /* WIN95 */

/* Interrupt flag for interrupting target reads from the HOST */
int pci_intr_flag;

/* Status and data addresses set in pci_cyclone_init(). */
static unsigned short data_addr, status_addr, doorbell_addr;

/* Status and data memory mapped addresses set in pci_cyclone_init(). */
static volatile FARX86_PTR status_mb, data_mb, doorbell_reg;

static void
pci_disp_regs();

#define IOSPACE()      (pci.cfg.comm_mode == COM_PCI_IOSPACE)
#define MEMSPACE()     (pci.cfg.comm_mode == COM_PCI_MMAP)

/* Win96 and phar lap need delays to sync up after errors in PCI */
/* DOS with delays creashede DOS so do not use*/
#if defined(__HIGHC__) || defined(WIN95)
#    define PCI_DEBUG_ERR(str) pci_debug_error(str);
/*
 * FUNCTION
 *    pci_debug_error()
 *
 *
 * PURPOSE
 *    This routine display pci registers on error if the debug switch is on.
 *    Else the routine delays to allow the target to recover.
 *
 * RETURNS
 */
static void
pci_debug_error(debug_msg)
char * debug_msg;
{
    unsigned long timeout;

    if (PCI_DEBUG()) 
        {
        pci_disp_regs(); 
        printf(debug_msg);
        }
    else
        {
        timeout = PCI_TIMEOUT;
        while (timeout-- != 0)
            if ((IN_STATUS_MB & CYCLONE_ERROR_BIT) != CYCLONE_ERROR_BIT)
                break;
        }
}
#else /* MSDOS */
#    define PCI_DEBUG_ERR(str) if (PCI_DEBUG()) { pci_disp_regs(); printf(str); }
#endif

/* ------------------------------------------------------------------ */
/* ----------------- HW-specific PCI Init Routines ------------------ */
/* ------------------------------------------------------------------ */

/*
 * FUNCTION
 *    pci_cyclone_init(void)
 *
 * PURPOSE
 *    Initialize a Cyclone baseboard with PLX PCI bridge installed to
 *    provide PCI download.
 *
 * RETURN
 *    OK                   - initialization ok.  As a side effect, 
 *                           pci.cfg.comm_mode is initialized to the
 *                           actual method (I/O or memory-mapped) used
 *                           to access the device.
 *
 *    ERR                  - initialization failed, com_stat is set to
 *                           the applicable error condition.
 */
static int 
pci_cyclone_init()
{
    int ec, hit = FALSE, comm_mode = COM_PCI_IOSPACE;

    /*
     * The Cyclone card has runtime registers that are mapped into both
     * I/O and memory space on the PC.  Get the physical memory and/or I/O
     * addresses of these registers as necessary from the PCI BIOS.
     */
    if (IOSPACE())
    {
        U_DWORD mb_io_addr;

        if ((ec = read_pci_cfg_obj(&pci.cfg,
                                   READ_CONFIG_DWORD,
                                   CYC_PCI_MB_IO_ADDR, 
                                   &mb_io_addr)) != OK)
        {
            return (ec);
        }
        if (mb_io_addr != 0x0)
        {
            mb_io_addr &= (~0x1);      /* Strip PCI I/O space bit */
            pci.ioreg   = (DOS_IOADDR) mb_io_addr;
        /* 
         * I/0 base address to Cyclone PCI runtime registers is stored in 
         * pci.ioreg .
         */

        data_addr   = pci.ioreg + CYCLONE_HOST_DATA_MB;
        status_addr = pci.ioreg + CYCLONE_HOST_STATUS_MB;
        doorbell_addr = pci.ioreg + CYCLONE_HOST_PLDB_REG;

            OUT_STATUS_MB(IN_STATUS_MB & 
                ~(CYCLONE_DATA_READY_BIT | CYCLONE_HOST_TRANSFER | CYCLONE_EOD_BIT |
                  CYCLONE_ERROR_BIT | CYCLONE_TARGET_TRANSFER)); 
            hit         = TRUE;
            comm_mode   = COM_PCI_IOSPACE;

        }
        else
        {
            /* PCI BIOS could not map device's registers into I/O space. */

            if (pci.real_mode)
            {
                /* We're toast -- no other way to access the device. */

                hdi_cmd_stat = com_stat = E_PHYS_MEM_MAP;
                return (ERR);
            }
            /* Else, fall through and try again using memory-mapped access. */
        }
    }

    if (! hit)
    {
        /*
         * We're either implicitly or explicitly using memory-mapped
         * device access.  Set it up.
         */

#ifdef __HIGHC__

        union REGS    cmd, result;
        struct SREGS  seg;
        unsigned long i, cyc_offset, pages;
        short         selector;
        U_DWORD       lcl_mem_addr, mb_mem_addr;

        if ((ec = read_pci_cfg_obj(&pci.cfg, 
                                   READ_CONFIG_DWORD,
                                   CYC_PCI_MB_MEM_ADDR, 
                                   &mb_mem_addr)) != OK)
        {
            return (ec);
        }

        cmd.h.ah  = 0x48;          /* Allocate selector, no mapped memory. */
        cmd.w.ebx = 0;
        int86(0x21, &cmd, &result);
        if (result.x.cflag != 0)
        {
            hdi_cmd_stat = com_stat = E_PHYS_MEM_ALLOC;
            return (ERR);
        }
        selector = result.x.ax;
        
        cyc_offset   = mb_mem_addr & 0xfff;
        mb_mem_addr &= (~0xfff);          /* Force addr alignment to 4K page */

        /* 
         * map 2, 4K pages of physical memory into empty selector.  The 
         * shared runtime registers will easily fit into one 4K page, but
         * 2 pages are used to ensure that the registers will be accessible
         * for the case where their physical location is very "close" to a
         * 4k boundary.
         */
        cmd.x.ax  = 0x250a;          /* Allocate PHYSICAL memory */
        cmd.w.ebx = mb_mem_addr;
        cmd.w.ecx = 2;
        segread(&seg);
        seg.es = selector;
        int86x(0x21, &cmd, &result, &seg);
        if (result.x.cflag != 0)
        {
            hdi_cmd_stat = com_stat = E_PHYS_MEM_ALLOC;
            return (ERR);
        }
        pci.mem.reg_sel   = pci.mem.reg.x.sel = selector;
        pci.mem.reg.x.off = cyc_offset;

    /* 
     * Ptr to Cyclone PCI runtime register base address is stored in 
     * pci.mem.reg and ptr to Cyclone PCI local memory is stored in
     * pci.mem.lcl .
     */
    status_mb        = pci.mem.reg;
    status_mb.x.off += CYCLONE_HOST_STATUS_MB;
    data_mb          = pci.mem.reg;
    data_mb.x.off   += CYCLONE_HOST_DATA_MB;
    doorbell_reg        = pci.mem.reg;
    doorbell_reg.x.off += CYCLONE_HOST_PLDB_REG;
        STATUS_MB    = (STATUS_MB & 
                         ~(CYCLONE_DATA_READY_BIT | CYCLONE_HOST_TRANSFER | CYCLONE_EOD_BIT |
                          CYCLONE_ERROR_BIT | CYCLONE_TARGET_TRANSFER)); 

        if ((ec = read_pci_cfg_obj(&pci.cfg, 
                                   READ_CONFIG_DWORD,
                                   CYC_PCI_LOCAL_MEM_ADDR,
                                   &lcl_mem_addr)) != OK)
        {
            (void) com_pci_end();  /* Free alloc'd selectors & mapped mem. */
            return (ec);
        }

        cmd.h.ah  = 0x48;          /* Allocate selector, no mapped memory. */
        cmd.w.ebx = 0;
        int86(0x21, &cmd, &result);
        if (result.x.cflag != 0)
        {
            (void) com_pci_end();  /* Free alloc'd selectors & mapped mem. */
            hdi_cmd_stat = com_stat = E_PHYS_MEM_ALLOC;
            return (ERR);
        }
        selector = result.x.ax;

        /* 
         * Determine size of local memory space and compute number of 4K
         * pages required to map same.
         */
        i = (unsigned long) -1;
        if ((ec = write_pci_cfg_obj(&pci.cfg, 
                                    WRITE_CONFIG_DWORD,
                                    CYC_PCI_LOCAL_MEM_ADDR,
                                    &i)) != OK)
        {
            (void) com_pci_end();  /* Free alloc'd selectors & mapped mem. */
            return (ec);
        }
        if ((ec = read_pci_cfg_obj(&pci.cfg, 
                                   READ_CONFIG_DWORD,
                                   CYC_PCI_LOCAL_MEM_ADDR,
                                   &i)) != OK)
        {
            (void) com_pci_end();  /* Free alloc'd selectors & mapped mem. */
            return (ec);
        }

        /* Put previous local memory address back */
        if ((ec = write_pci_cfg_obj(&pci.cfg, 
                                    WRITE_CONFIG_DWORD,
                                    CYC_PCI_LOCAL_MEM_ADDR,
                                    &lcl_mem_addr)) != OK)
        {
            (void) com_pci_end();  /* Free alloc'd selectors & mapped mem. */
            return (ec);
        }

        /*
         * Compute size of local address space based on method described
         * in PCI spec.
         */
        i >>= 4;   /* bottom four bits of returned value don't count */
        pci.mem.lcl_size = 0x10;
        if (i)
        {
            while ((i & 0x1) == 0)
            {
                pci.mem.lcl_size <<= 1;
                i >>= 1;
            }
        }

        /* 
         * 4k page calculation contains an additional page (+2) to cover
         * the degenerate case where < 4K of local memory must be mapped
         * across 2 physical pages.
         */
        pages         = pci.mem.lcl_size / 4096 + 2;
        cyc_offset    = lcl_mem_addr & 0xfff;
        lcl_mem_addr &= (~0xfff);    /* Force addr alignment to 4K page */
        cmd.x.ax  = 0x250a;          /* Allocate PHYSICAL memory */
        cmd.w.ebx = lcl_mem_addr;
        cmd.w.ecx = pages;
        segread(&seg);
        seg.es = selector;
        int86x(0x21, &cmd, &result, &seg);
        if (result.x.cflag != 0)
        {
            (void) com_pci_end();  /* Free alloc'd selectors & mapped mem. */
            hdi_cmd_stat = com_stat = E_PHYS_MEM_ALLOC;
            return (ERR);
        }
        pci.mem.lcl_sel   = pci.mem.lcl.x.sel = selector;
        pci.mem.lcl.x.off = cyc_offset;
        comm_mode         = COM_PCI_MMAP;
#else
        hdi_cmd_stat = com_stat = E_PHYS_MEM_MAP;
        return (ERR);
#endif
    }

    ec = OK;          /* An assumption */
    /* Test for pci comm - no control port*/
    if (*(unsigned long *)pci.cfg.control_port != -1L)
        {
        /* 
         * Check that controlling serial port is attached to specified
         * connected pci device.  This check prevents us from jamming
         * data down the throat of a PCI card that's not 'connected' to
         * the controlling serial port :-) .  This event could occur in
         * the case where more than one Cyclone/PLX card was inserted in
         * a PCI card cage and the user did not specify an absolute PCI
         * address for the target.  Algorithm:
         * 
         *    Send controlling port string to monitor via serial link.
         *    Next read control port string back from mailbox to see if 
         *        talking to correct monitor.
         *    If yes, then
         *       connected
         *       pci.cfg.comm_mode <- actual mode used to access target
         *    Else 
         *       return error saying Not Controlling Port
         * fi
         */
        com_init_msg();
        if ((com_put_byte(PCI_VERIFY) != OK) ||
            (com_put_byte(~OK) != OK) ||
            (com_put_long(*(unsigned long *)pci.cfg.control_port) != OK))
        {
            hdi_cmd_stat = com_stat = com_get_stat();
            return (ERR);
        }

        if (_hdi_send(0) == NULL)
            return (ERR);
         
        if (MEMSPACE())
        {
            if (*(unsigned long *)pci.cfg.control_port != DATA_MB)
            {
                hdi_cmd_stat = com_stat = E_CONTROLLING_PORT;
                ec           = ERR;
            }
        }
        else  /* IO space */
        {
            if (*(unsigned long *)pci.cfg.control_port != IN_DATA_MB)
            {
                hdi_cmd_stat = com_stat = E_CONTROLLING_PORT;
                ec           = ERR;
            }
        }
    }
    
    if (ec == OK)
        pci.cfg.comm_mode = comm_mode;
    return (ec);
}


/*
 * FUNCTION
 *    pci_cyclone_intr_trgt()
 *
 *
 * PURPOSE
 *    This routine set the door bell interrupt bit #31 to sxtop the target.
 *
 *
 * RETURNS
 *    OK   -> success.
 *    ERR  -> error,  .
 */
static int
pci_cyclone_intr_trgt()
{
    if (MEMSPACE())
        DOORBELL_REG |=  CYCLONE_INTERRUPT_BIT;
    else
        OUT_DOORBELL_REG(IN_DOORBELL_REG | CYCLONE_INTERRUPT_BIT);

    return OK;
}


/*
 * FUNCTION
 *    pci_cyclone_err()
 *
 *
 * PURPOSE
 *    This routine set the error  bit and resets all other transfer bit to off.
 *
 *
 *
 * RETURNS
 */
static void
pci_cyclone_err()
{
    if (MEMSPACE())
        {
        STATUS_MB |=  CYCLONE_ERROR_BIT;
        STATUS_MB &= ~(CYCLONE_HOST_TRANSFER | CYCLONE_TARGET_TRANSFER | CYCLONE_EOD_BIT | CYCLONE_DATA_READY_BIT );
        }
    else
        {
        OUT_STATUS_MB(IN_STATUS_MB | CYCLONE_ERROR_BIT);
        OUT_STATUS_MB(IN_STATUS_MB & 
            ~(CYCLONE_HOST_TRANSFER | CYCLONE_TARGET_TRANSFER | CYCLONE_EOD_BIT | CYCLONE_DATA_READY_BIT ));
        }
}

/*
 * FUNCTION
 *    pci_cyclone_connnect()
 *
 *
 * PURPOSE
 *    This routine set the PLX pci bit for inuse and error,
 *    then it waits for the target to reset the  error bit.
 *
 *
 * RETURNS
 *    OK   -> success.
 *    ERR  -> error,  timeout waiting for the error bit to be reset.
 */
static int
pci_cyclone_connect()
{
    unsigned long timeout, i;
    
    if (MEMSPACE())
        {
        for (i=0; i<3; i++)
          {
          STATUS_MB &= ~(CYCLONE_HOST_TRANSFER | CYCLONE_TARGET_TRANSFER |
                             CYCLONE_EOD_BIT | CYCLONE_DATA_READY_BIT | CYCLONE_INUSE_BIT);
          STATUS_MB |=  CYCLONE_INUSE_BIT | CYCLONE_ERROR_BIT;

          timeout = PCI_TIMEOUT;
          while ((STATUS_MB & CYCLONE_ERROR_BIT) == CYCLONE_ERROR_BIT)
              {
              if ((STATUS_MB & CYCLONE_INUSE_BIT) != CYCLONE_INUSE_BIT)
                  break;
              if (timeout-- == 0)
                  {
                  PCI_DEBUG_ERR("(02)Timeout err at connect loop\n");
                  pci_cyclone_err();
                  com_stat = E_PCI_COMM_TIMO;
                  return(ERR);
                  }
              }

          if ((STATUS_MB & CYCLONE_INUSE_BIT) == CYCLONE_INUSE_BIT)
              return OK;
          }
        }
    else
        {
        for (i=0; i<3; i++)
          {
          OUT_STATUS_MB(IN_STATUS_MB & 
              ~(CYCLONE_HOST_TRANSFER | CYCLONE_TARGET_TRANSFER |
                CYCLONE_EOD_BIT | CYCLONE_DATA_READY_BIT | CYCLONE_INUSE_BIT));
          OUT_STATUS_MB(IN_STATUS_MB | CYCLONE_INUSE_BIT | CYCLONE_ERROR_BIT);

          timeout = PCI_TIMEOUT;
          while ((IN_STATUS_MB & CYCLONE_ERROR_BIT) == CYCLONE_ERROR_BIT)
              {
              if ((IN_STATUS_MB & CYCLONE_INUSE_BIT) != CYCLONE_INUSE_BIT)
                  break;
              if (timeout-- == 0)
                  {
                  PCI_DEBUG_ERR("(03)Timeout err at connect loop\n");
                  pci_cyclone_err();
                  com_stat = E_PCI_COMM_TIMO;
                  return(ERR);
                  }
              }
        
          if ((IN_STATUS_MB & CYCLONE_INUSE_BIT) == CYCLONE_INUSE_BIT)
              return OK;
          }

        }

    PCI_DEBUG_ERR("(04)Inuse off err at connect loop\n");
    pci_cyclone_err();
    com_stat = E_PCI_COMM_TIMO;
    return(ERR);
}


/*
 * FUNCTION
 *    pci_cyclone_disconnect()
 *
 * PURPOSE
 *    This routine reset the plx PCI inuse bit.
 *
 *
 * RETURNS
 */
static void
pci_cyclone_disconnect()
{
    if (MEMSPACE())
        {
        STATUS_MB &=  ~CYCLONE_INUSE_BIT;
        }
    else
        {
        OUT_STATUS_MB(IN_STATUS_MB & ~CYCLONE_INUSE_BIT);
        }
}


/*
 * FUNCTION
 *    pci_cyclone_put()
 *
 *    -   buf,   address of the buffer containing data to write
 *    -   size,  the number of bytes to write
 *    -   crc,   the pointer to the crc variable to update or NULL
 *
 * PURPOSE
 *    This routine writes data to a i960 cyclone card.
 *
 * RETURNS
 *    OK   -> success.
 *    ERR  -> error,  .
 */
static int 
pci_cyclone_put(buf, size, crc)
unsigned char *buf;
unsigned long size;
unsigned short *crc;
{
    unsigned long * word_buf = (unsigned long *)buf;
    unsigned long   status, 
                    timeout,
                    sz = size,
                    sum = 0;

    if (MEMSPACE())
        {
        if ((STATUS_MB & CYCLONE_HOST_TRANSFER) ==  CYCLONE_HOST_TRANSFER)
            {
            PCI_DEBUG_ERR("(05)Targ err at put start\n");
            hdi_cmd_stat = com_stat = E_COMM_ERR;
            return(ERR);
            }

        timeout = PCI_TIMEOUT;
        while (((status=STATUS_MB) & CYCLONE_TARGET_TRANSFER) == CYCLONE_TARGET_TRANSFER)
            {
            if ((status & CYCLONE_ERROR_BIT) == CYCLONE_ERROR_BIT)
                {
                PCI_DEBUG_ERR("(06)Targ err at inner loop\n");
                hdi_cmd_stat = com_stat = E_COMM_ERR;
                return(ERR);
                }
            if (timeout-- == 0)
                {
                PCI_DEBUG_ERR("(07)Timeout err at start write\n");
                hdi_cmd_stat = com_stat = E_PCI_COMM_TIMO;
                pci_cyclone_err();
                return(ERR);
                }
            }

        STATUS_MB &= ~(CYCLONE_ERROR_BIT | CYCLONE_DATA_READY_BIT | CYCLONE_EOD_BIT);
        STATUS_MB |= CYCLONE_HOST_TRANSFER;

        /* timeout so we don't bloack forever */
        size = (size+3)/4;
        while (size-- > 0)
            {
            timeout = PCI_TIMEOUT;
            while (((status=STATUS_MB) & CYCLONE_DATA_READY_BIT) == CYCLONE_DATA_READY_BIT)
                {
                if ((status & CYCLONE_ERROR_BIT) == CYCLONE_ERROR_BIT)
                    {
                    PCI_DEBUG_ERR("(08)Targ err at inner loop\n");
                    hdi_cmd_stat = com_stat = E_COMM_ERR;
                    return(ERR);
                    }
                if (timeout-- == 0)
                    {
                    PCI_DEBUG_ERR("(09)Timeout err at inner loop\n");
                    hdi_cmd_stat = com_stat = E_PCI_COMM_TIMO;
                    pci_cyclone_err();
                    return(ERR);
                    }
                }

            DATA_MB = *word_buf;

            if (size == 0)
                STATUS_MB |= CYCLONE_EOD_BIT;
            STATUS_MB |= CYCLONE_DATA_READY_BIT;

            if (sz > 2)
                sum ^= *word_buf++;
            }

        timeout = PCI_TIMEOUT;
        while (((status=STATUS_MB) & CYCLONE_DATA_READY_BIT) == CYCLONE_DATA_READY_BIT)
                {
                if ((status & CYCLONE_ERROR_BIT) == CYCLONE_ERROR_BIT)
                    {
                    PCI_DEBUG_ERR("(10)Targ err at end of loop\n");
                    hdi_cmd_stat = com_stat = E_COMM_ERR;
                    return(ERR);
                    }
                if (timeout-- == 0)
                    {
                    PCI_DEBUG_ERR("(11)Timeout err at end of loop\n");
                    hdi_cmd_stat = com_stat = E_PCI_COMM_TIMO;
                    pci_cyclone_err();
                    return(ERR);
                    }
                }

        STATUS_MB &= ~CYCLONE_HOST_TRANSFER;

        if ((status & CYCLONE_ERROR_BIT) == CYCLONE_ERROR_BIT)
                {
                PCI_DEBUG_ERR("(12)Targ err at end of loop\n");
                hdi_cmd_stat = com_stat = E_COMM_ERR;
                return(ERR);
                }

        if (crc != NULL)
            *crc ^= (unsigned short)((sum >> 16) ^ sum);

        return(OK);
        }
    else
        {
        if ((IN_STATUS_MB & CYCLONE_HOST_TRANSFER) == CYCLONE_HOST_TRANSFER)
               {
               PCI_DEBUG_ERR("(13)Targ err at put start\n");
               hdi_cmd_stat = com_stat = E_COMM_ERR;
               return(ERR);
               }

        timeout = PCI_TIMEOUT;
        while (((status=IN_STATUS_MB) & CYCLONE_TARGET_TRANSFER) ==
                 CYCLONE_TARGET_TRANSFER)
            {
            if ((status & CYCLONE_ERROR_BIT) == CYCLONE_ERROR_BIT)
                {
                PCI_DEBUG_ERR("(14)Targ err at inner loop\n");
                hdi_cmd_stat = com_stat = E_COMM_ERR;
                return(ERR);
                }
            if (timeout-- == 0)
                {
                PCI_DEBUG_ERR("(15)Timeout err at start write\n");
                hdi_cmd_stat = com_stat = E_PCI_COMM_TIMO;
                pci_cyclone_err();
                return(ERR);
                }
            }
            
        OUT_STATUS_MB(IN_STATUS_MB &
                ~(CYCLONE_ERROR_BIT | CYCLONE_DATA_READY_BIT | CYCLONE_EOD_BIT));
        OUT_STATUS_MB(IN_STATUS_MB | CYCLONE_HOST_TRANSFER);

        size = (size+3)/4;
        while (size-- > 0)
            {
            timeout = PCI_TIMEOUT;
            while (((status=IN_STATUS_MB) & CYCLONE_DATA_READY_BIT) ==
                     CYCLONE_DATA_READY_BIT)
                {
                if ((status & CYCLONE_ERROR_BIT) == CYCLONE_ERROR_BIT)
                    {
                    PCI_DEBUG_ERR("(16)Targ err at inner loop\n");
                    hdi_cmd_stat = com_stat = E_COMM_ERR;
                    return(ERR);
                    }
                if (timeout-- == 0)
                    {
                    PCI_DEBUG_ERR("(17)Timeout err at inner loop\n");
                    hdi_cmd_stat = com_stat = E_PCI_COMM_TIMO;
                    return(ERR);
                    }
                }

            OUT_DATA_MB(*word_buf);

            if (size == 0)
                status |= CYCLONE_EOD_BIT;

            OUT_STATUS_MB(status | CYCLONE_DATA_READY_BIT);

            if (sz > 2)
                sum ^= *word_buf++;
            }

        timeout = PCI_TIMEOUT;
        while (((status=IN_STATUS_MB) & CYCLONE_DATA_READY_BIT) == CYCLONE_DATA_READY_BIT) 
           {
           if ((status & CYCLONE_ERROR_BIT) == CYCLONE_ERROR_BIT)
               {
               PCI_DEBUG_ERR("(18)Targ err at end of loop\n");
               hdi_cmd_stat = com_stat = E_COMM_ERR;
               return(ERR);
               }
           if (timeout-- == 0)
               {
               PCI_DEBUG_ERR("(19)Timeout err at end of loop\n");
               hdi_cmd_stat = com_stat = E_PCI_COMM_TIMO;
               pci_cyclone_err();
               return(ERR);
               }
           }

        OUT_STATUS_MB(status & ~CYCLONE_HOST_TRANSFER);

        if ((status & CYCLONE_ERROR_BIT) == CYCLONE_ERROR_BIT)
           {
           PCI_DEBUG_ERR("(20)Targ err at end of loop\n");
           hdi_cmd_stat = com_stat = E_COMM_ERR;
           return(ERR);
           }

        if (crc != NULL)
            *crc ^= (unsigned short)((sum >> 16) ^ sum);
        }

    return(OK);
}


/*
 * FUNCTION
 *    pci_cyclone_get()
 *
 *    -   buf,   address of the buffer to read data into
 *    -   size,  the number of bytes to read
 *    -   crc,   the pointer to the crc variable to update or NULL
 *    -   wait_flag, one of COM_WAIT, COM_POLL, COM_WAIT_FOREVER
 *
 * PURPOSE
 *    This reoutine reads data from a i960 cyclone card.
 *
 *
 * RETURNS
 *    OK   -> success.
 *    ERR  -> error,  .
 */
/* TESTING
static int 
*/
pci_cyclone_get(buf, size, crc, wait_flag)
unsigned char *buf;
unsigned long size;
unsigned short *crc;
int wait_flag;
{
#ifndef WIN95
    int             timo_flag = 0;
#endif      
    unsigned long * word_buf = (unsigned long *)buf;
    unsigned long   status, 
                    timeout_val, timeout,
                    sz = size,
                    sum = 0;

    pci_intr_flag = FALSE;

    switch (wait_flag)
        {
        case COM_POLL:
            timeout_val = 100;
            break;
        case COM_WAIT:
            timeout_val = 0;
            settimeout(wait_flag, &timo_flag);
            break;
        case COM_WAIT_FOREVER:
            timeout_val = 0;
            break;
        default:
            timeout_val = 0;
            settimeout(wait_flag, &timo_flag);
            break;
        }

    if (MEMSPACE())
        {
        timeout = timeout_val;
        while ((((status=STATUS_MB) & CYCLONE_ERROR_BIT) == CYCLONE_ERROR_BIT) || 
               ((status & CYCLONE_TARGET_TRANSFER) != CYCLONE_TARGET_TRANSFER))
            {
            if (timeout != 0)
                {
                if (--timeout == 0)
                    {
                    PCI_DEBUG_ERR("(21)Timeout err at end get start\n");
                    com_stat = E_PCI_COMM_TIMO;
                    pci_cyclone_err();
                    settimeout(0, &timo_flag);
                    return(ERR);
                    }
                }
            else if (timo_flag) /* timeout = 0 */
                {
                PCI_DEBUG_ERR("(22)Timeout err at get start\n");
                com_stat = E_PCI_COMM_TIMO;
                pci_cyclone_err();
                settimeout(0, &timo_flag);
                return(ERR);
                }
            _kbhit();
            if (pci_intr_flag == TRUE)
                {
                settimeout(0, &timo_flag);
                com_stat = E_INTR;
                return ERR;
                }
            if ((status & CYCLONE_ERROR_BIT) == CYCLONE_ERROR_BIT)
                {
                settimeout(0, &timo_flag);
                timeout = PCI_TIMEOUT;
                }
            }
            
        /* timeout so we don't block forever */
        size = (size+3)/4;
        while (size-- > 0)
            {
            timeout = timeout_val;
            while (((status=STATUS_MB) & CYCLONE_DATA_READY_BIT) != CYCLONE_DATA_READY_BIT)
                {
                if ((status & CYCLONE_ERROR_BIT) == CYCLONE_ERROR_BIT)
                    {
                    PCI_DEBUG_ERR("(23)Timeout err at target set error bit wait\n");
                    settimeout(0, &timo_flag);
                    com_stat = E_COMM_ERR;
                    return(ERR);
                    }
                if (timeout != 0)
                    {
                    if (--timeout == 0)
                        {
                        PCI_DEBUG_ERR("(24)Timeout err at ready bit timeout\n");
                        com_stat = E_PCI_COMM_TIMO;
                        pci_cyclone_err();
                        settimeout(0, &timo_flag);
                        return(ERR);
                        }
                    }
               else  if (timo_flag) /* timeout = 0 */
                    {
                    /* Normal tinmeout waiting for input */
                    /* PCI_DEBUG_ERR("(98)Timeout err at ready bit timeout\n");*/
                    com_stat = E_PCI_COMM_TIMO;
                    pci_cyclone_err();
                    settimeout(0, &timo_flag);
                    return(ERR);
                    }
                _kbhit();
                if (pci_intr_flag == TRUE)
                    {
                    pci_cyclone_err();
                    settimeout(0, &timo_flag);
                    com_stat = E_INTR;
                    return ERR;
                    }
                }

            *word_buf = DATA_MB;

            if (size == 0)
                {
                if ((status & CYCLONE_EOD_BIT) != CYCLONE_EOD_BIT)
                    {
                    PCI_DEBUG_ERR("(26)Error at last EOD not set\n");
                    pci_cyclone_err();
                    settimeout(0, &timo_flag);
                    return ERR;
                    }
                }
            else
                {
                if ((status & CYCLONE_EOD_BIT) == CYCLONE_EOD_BIT)
                    {
                    PCI_DEBUG_ERR("(27)Error at not last EOD set\n");
                    pci_cyclone_err();
                    settimeout(0, &timo_flag);
                    return ERR;
                    }
                }
            
            STATUS_MB &= ~CYCLONE_DATA_READY_BIT;

            if (sz > 2)
                sum ^= *word_buf++;
            }

        if (crc != NULL)
            *crc ^= (unsigned short)((sum >> 16) ^ sum);

        settimeout(0, &timo_flag);
        return(OK);
        }
    else
        {
        timeout = timeout_val;
        while ((((status=IN_STATUS_MB) & CYCLONE_ERROR_BIT) == CYCLONE_ERROR_BIT) ||
               ((status & CYCLONE_TARGET_TRANSFER) != CYCLONE_TARGET_TRANSFER))
            {
            if (timeout != 0)
                {
                if (--timeout == 0)
                    {
                    PCI_DEBUG_ERR("(28)Timeout err at error bit wait\n");
                    com_stat = E_PCI_COMM_TIMO;
                    pci_cyclone_err();
                    settimeout(0, &timo_flag);
                    return(ERR);
                    }
                else if (timo_flag)   /* timeout = 0 */
                    {
                    PCI_DEBUG_ERR("(29)Timeout err at error bit wait\n");
                    com_stat = E_PCI_COMM_TIMO;
                    pci_cyclone_err();
                    settimeout(0, &timo_flag);
                    return(ERR);
                    }
                }
            _kbhit();
            if (pci_intr_flag == TRUE)
                {
                settimeout(0, &timo_flag);
                com_stat = E_INTR;
                return ERR;
                }
            if ((status & CYCLONE_ERROR_BIT) == CYCLONE_ERROR_BIT)
                {
                settimeout(0, &timo_flag);
                timeout = PCI_TIMEOUT;
                }
            }
            
        size = (size+3)/4;
        while (size-- > 0)
           {
           timeout = timeout_val;
           while (((status=IN_STATUS_MB) & CYCLONE_DATA_READY_BIT) !=
                    CYCLONE_DATA_READY_BIT)
               {
               if ((status & CYCLONE_ERROR_BIT) == CYCLONE_ERROR_BIT)
                   {
                   PCI_DEBUG_ERR("(30)Timeout err at target set error bit wait\n");
                   com_stat = E_COMM_ERR;
                   settimeout(0, &timo_flag);
                   return(ERR);
                   }
               if (timeout != 0)
                   {
                   if (--timeout == 0)
                      {
                      PCI_DEBUG_ERR("(31)Timeout err at ready bit timeout\n");
                      com_stat = E_PCI_COMM_TIMO;
                      pci_cyclone_err();
                      settimeout(0, &timo_flag);
                      return(ERR);
                      }
                   }
               else if (timo_flag)   /* timeout = 0 */
                  {
                  /* Normal timeout waiting for input */      
                  /*PCI_DEBUG_ERR("(99)Timeout err at ready bit timeout\n");*/
                  com_stat = E_PCI_COMM_TIMO;
                  pci_cyclone_err();
                  settimeout(0, &timo_flag);
                  return(ERR);
                  }
               _kbhit();
               if (pci_intr_flag == TRUE)
                   {
                   pci_cyclone_err();
                   settimeout(0, &timo_flag);
                   com_stat = E_INTR;
                   return ERR;
                   }
               }

            *word_buf = IN_DATA_MB;

            if (size == 0)
                {
                if ((status & CYCLONE_EOD_BIT) != CYCLONE_EOD_BIT)
                    {
                    PCI_DEBUG_ERR("(32)Error at last EOD not set\n");
                    pci_cyclone_err();
                    settimeout(0, &timo_flag);
                    return ERR;
                    }
                }
            else
                {
                if ((status & CYCLONE_EOD_BIT) == CYCLONE_EOD_BIT)
                    {
                    PCI_DEBUG_ERR("(33)Error at not last EOD set\n");
                    pci_cyclone_err();
                    settimeout(0, &timo_flag);
                    return ERR;
                    }
                }

            OUT_STATUS_MB(IN_STATUS_MB & ~CYCLONE_DATA_READY_BIT);

            if (sz > 2)
                sum ^= *word_buf++;
            }

        if (crc != NULL)
            *crc ^= (unsigned short)((sum >> 16) ^ sum);
        }

    settimeout(0, &timo_flag);
    return(OK);
}


/*
 * FUNCTION
 *    pci_cyclone_direct_put(long, *buf, int)
 *
 *    - addr,  The cyclone i960 addr to write the buffer to
 *    - data,  The buffer containg the data to write
 *    - size,  The actual length to write
 *
 * PURPOSE
 *    This routine writes data to the i960 cards memory using direct writes
 *    avioding the overhead of passing data through mailboxes
 *
 *
 * RETURNS
 *    OK   -> success.
 *    ERR  -> error,  .
 */
static int 
pci_cyclone_direct_put(addr, data, size)
unsigned long addr;
unsigned char *data;
unsigned long size;
{
    volatile FARX86_PTR    ram_addr_reg,ram_size_reg,direct_addr;
    unsigned long          *word_data;
    unsigned long          i,j,crc = 0,
                           curr_size = size;
    unsigned char          pci_verify = PCI_VERIFY; 

    /* 
     * Ptr to Cyclone PCI runtime register base address is stored in 
     * pci.mem.reg and ptr to Cyclone PCI local memory is stored in
     * pci.mem.lcl .
     */
    hdi_cmd_stat = com_stat = OK;
    /* temporary until mmap download works  on win95 we need a VXD first */
    /*NOTE*/
    return ERR;

    if (IOSPACE())
        return ERR;

    ram_size_reg = ram_addr_reg = pci.mem.reg;
    ram_size_reg.x.off += CYCLONE_HOST_RANGE_PTOL_0;
    ram_addr_reg.x.off += CYCLONE_HOST_PTOL_0;
    /* test if address is the same region of the addr, and within memory bounds */
    if (((*ram_addr_reg.lp & 0xf0000000) != (addr & 0xf0000000)) ||
        ((addr+size - (*ram_addr_reg.lp & 0xfffffff0)) > 
        ((~(*ram_size_reg.lp&0xfffffff0))+1)))
        return ERR;

    /* 
     * Ptr to Cyclone PCI runtime register base address is stored in 
     * pci.mem.reg and ptr to Cyclone PCI local memory is stored in
     * pci.mem.lcl .
     */
    word_data = (unsigned long *)data;
    ram_addr_reg = pci.mem.reg;
    ram_addr_reg.x.off += CYCLONE_HOST_PTOL_0;
    direct_addr = pci.mem.lcl;
    direct_addr.x.off +=  addr - (*ram_addr_reg.lp & 0xfffffff0);

    /* Cyclone direct write work for any Big or Little endian region */
#if 0
    printf("Direct put start size=%x, addr=%x, sel=%x, da=%x\n",size,addr,direct_addr.x.sel,direct_addr.x.off);
#endif
    curr_size = (size+3)/4;
    while (curr_size-- > 0)
        {
        crc ^= *word_data;
        *direct_addr.lp = *word_data++;
        direct_addr.x.off += 4;
                for (i=0; i< 4; i++) j=i;
        }

#if 0
    printf("Direct put verify\n");
#endif
    if ((pci_cyclone_put((unsigned char *)&pci_verify,1L,NULL) != OK) ||
        (pci_cyclone_put((unsigned char *)&addr,4L,NULL) != OK) ||
        (pci_cyclone_put((unsigned char *)&size,4L,NULL) != OK) ||
        (pci_cyclone_put((unsigned char *)&crc,4L,NULL) != OK))
       return(ERR);

    if (pci_cyclone_put((unsigned char *)&pci_verify,1L,NULL) != OK) 
       return(ERR);

#if 0
    printf("Direct put done\n");
#endif
    return OK;
}

/*
 * FUNCTION
 *    pci_get_reg()
 *
 *    - addr, the offset of a PLX register
 *
 * PURPOSE
 *    This routine return the value of a specified PLX register.
 *
 *
 * RETURNS
 *    the current value of a PLX register
 */
unsigned long 
pci_get_reg(addr)
unsigned short addr;
{
    if (MEMSPACE())
        {
        volatile FARX86_PTR    data = pci.mem.reg;
        /* 
         * Ptr to Cyclone PCI runtime register base address is stored in 
         * pci.mem.reg and ptr to Cyclone PCI local memory is stored in
         * pci.mem.lcl .
         */
        data.x.off += addr;
        return *data.lp;
        }
    else
        {
        unsigned short io_data_addr;
        /* 
         * I/0 base address to Cyclone PCI runtime registers is stored in 
         * pci.ioreg .
         */

        io_data_addr  = pci.ioreg + addr;
        return IND(io_data_addr);
        }
}

/*
 * FUNCTION
 *    pci_put_reg()
 *
 *    -  addr, the PLX pci register offset
 *    -  value,  the value to set this register to
 *
 * PURPOSE
 *    This routine allow a caller to set a PLX register to a value.
 *
 * RETURNS
 */
void 
pci_put_reg(addr, value)
unsigned short addr;
unsigned long value;
{
    if (MEMSPACE())
        {
        volatile FARX86_PTR    data = pci.mem.reg;
        /* 
         * Ptr to Cyclone PCI runtime register base address is stored in 
         * pci.mem.reg and ptr to Cyclone PCI local memory is stored in
         * pci.mem.lcl .
         */
        data.x.off += addr;
        *data.lp = value;
        }
    else
        {
        unsigned short io_data_addr;
        /* 
         * I/0 base address to Cyclone PCI runtime registers is stored in 
         * pci.ioreg .
         */

        io_data_addr   = pci.ioreg + addr;
        OUTD(io_data_addr, value);
        }
}

/*
 * FUNCTION
 *    pci_disp_regs()
 *
 * PURPOSE
 *    This is a debug routine t print out the current PLX pci regs.
 *
 *
 * RETURNS
 */
void 
pci_disp_regs()
{
    short pci_addr;

    fprintf(stdout, "PCI/960 ADDR - PCI Register Values\n");
    for (pci_addr=0; pci_addr < 0x80; pci_addr +=16)
        {
        fprintf(stdout, "  0x%02x/0x%02x:  %08lx  %08lx  %08lx  %08lx\n",
            pci_addr, pci_addr+0x80,
            pci_get_reg(pci_addr),
            pci_get_reg(pci_addr + 4),
            pci_get_reg(pci_addr + 8),
            pci_get_reg(pci_addr + 12));
        }
}


/*
 * FUNCTION
 *    pci_driver_routines(&pci_driver)
 *
 *    -  pci_driver, the pointer to the list of fast interface routines to fill in
 *
 * PURPOSE
 *    This routine fill in the list pci drivers for a plx interface
 *
 * RETURNS
 */
void
plx_driver_routines(struct FAST_INTERFACE *pci_driver)
{
        pci_driver->init        = pci_cyclone_init;
        pci_driver->connect     = pci_cyclone_connect;
        pci_driver->disconnect  = pci_cyclone_disconnect;
        pci_driver->err         = pci_cyclone_err;
        pci_driver->get         = pci_cyclone_get;
        pci_driver->put         = pci_cyclone_put;
        pci_driver->direct_put  = pci_cyclone_direct_put;
        pci_driver->intr_trgt   = pci_cyclone_intr_trgt;
}


#ifdef WIN95
static volatile int j, timeval;
static unsigned short last_port;
/*
 * FUNCTION
 *    in_portd( port )
 *
 *        port - I/O port to read from.
 *
 * PURPOSE
 *    Uses inline assembly to do port I/O to the hardware
 *
 * RETURN
 *    unsigned long, Value read from the I/O Port
 */

static unsigned long
in_portd (unsigned short port)
{
    if (port == last_port)
    {
        timeval = 25;
        while(timeval--)
        {
            j = timeval;
        }
    }
    last_port = 0;

    _asm
    {
        Mov    DX, port
        In    EAX, DX
    }
}


/*
 * FUNCTION
 *    out_portd( port, value )
 *
 *         port - I/O Port to write to.
 *      value - 32 bit value to write to the port.
 *
 * PURPOSE
 *    Uses inline assembly to do port I/O to the hardware
 *
 * RETURN
 *    nothing
 */

static void
out_portd (unsigned short port, unsigned long value)
{
    _asm
    {
        Mov    DX, port
        Mov    EAX, value
        Out    DX, EAX
    }
        last_port = port;
}


void int1a(X86REG *regsptr)
{
    X86REG regs;

    regs = *regsptr;        /* Copy to local to simplify _asm{}. */
    _asm {
    mov    eax,regs.e.eax
    mov    ebx,regs.e.ebx
    mov    ecx,regs.e.ecx
    mov    edx,regs.e.edx
    mov    edi,regs.e.edi
    mov    esi,regs.e.esi
    mov    regs.e.cflag,00h

    int    1ah

    jnc    worked
    mov    regs.e.cflag,0ffh

    worked:
    mov    regs.e.eax,eax
    mov    regs.e.ebx,ebx
    mov    regs.e.ecx,ecx
    mov    regs.e.edx,edx
    mov    regs.e.edi,edi
    mov    regs.e.esi,esi
    }
    *regsptr = regs;        /* Copy back to calling routine. */
}
#endif /* WIN95 */
