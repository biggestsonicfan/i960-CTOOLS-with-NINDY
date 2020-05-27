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
 * Host side of logical communication layer.  These commands are used
 * by sdm to implement user commands.
 */
/* "$Header: /ffs/p1/dev/src/hdil/gorman/RCS/hostcmds.c,v 1.45 1996/01/05 18:40:06 gorman Exp $$Locker:  $ */

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "private.h"
#include "hdi_com.h"
#include "dbg_mon.h"
#include "hdi_mcfg.h"

extern const STOP_RECORD * handle_stop_msg();

const char _hdi_version_string[] = "HDIL 3.0.2";

/* Data exported by this module */
int  hdi_cmd_stat;

int _hdi_signalled;
int _hdi_arch;
int _hdi_mon_version;
int _hdi_break_causes_reset;
int _hdi_save_errno;
int _hdi_mon_priority;
fast_download _hdi_fast_download;

REGION_CACHE cache_by_regions={'\01','\01','\01','\01','\01','\01','\01','\01',
                               '\01','\01','\01','\01','\01','\01','\01','\01'};

static DOWNLOAD_CONFIG end_fast_download = 
    { STOP_FAST_DOWNLOAD, FALSE, (char *)NULL };
static DOWNLOAD_CONFIG parallel_capable = 
    { FAST_PARALLEL_DOWNLOAD, FALSE, FAST_DOWNLOAD_CAPABLE };
static DOWNLOAD_CONFIG pci_capable = 
    { FAST_PCI_DOWNLOAD, FALSE, FAST_DOWNLOAD_CAPABLE };
DOWNLOAD_CONFIG *END_FAST_DOWNLOAD = &end_fast_download;
DOWNLOAD_CONFIG *PARALLEL_CAPABLE  = &parallel_capable;
DOWNLOAD_CONFIG *PCI_CAPABLE       = &pci_capable;


/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: hdi_set_prcb                                               */
/*                                                                           */
/* ACTION:                                                                   */
/*      hdi_set_prcb causes the monitor to reinitialize the processor with   */
/*      a new PRCB.  The address of the prcb to be used is passed in to the  */
/*      function.  This may be the address of a new prcb or the current one  */
/*      with values in the prcb changed.                                     */
/*                                                                           */
/*****************************************************************************/
int
hdi_set_prcb(prcb_address)
ADDR prcb_address;
{
    if (_hdi_put_regs() != OK || _hdi_flush_cache() != OK)
    return(ERR);

    com_init_msg();
    if (com_put_byte(SET_PRCB) != OK
    || com_put_byte(~OK) != OK
    || com_put_long(prcb_address) != OK)
    {
    hdi_cmd_stat = com_get_stat();
    return(ERR);
    }

    if (_hdi_send(0) == NULL)
    return(ERR);

    _hdi_invalidate_registers();

    return(OK);
}

/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: hdi_iac                                                    */
/*                                                                           */
/* ACTION:                                                                   */
/*      Issue a processor iac command.                              */
/*                                                                           */
/*****************************************************************************/
int
hdi_iac(addr, iac)
ADDR addr;
const unsigned long iac[4];
{
    unsigned char request = (unsigned char) (iac[0] >> 24);
    int i;
    
    if (_hdi_arch != ARCH_KA && _hdi_arch != ARCH_KB &&
        _hdi_arch != ARCH_SA && _hdi_arch != ARCH_SB)
    {
        hdi_cmd_stat = E_ARCH;
        return(ERR);
    }

    if (addr == 0)
        addr = ADR_SELF_IAC;

    if (addr == ADR_SELF_IAC && request==0x92) {
        hdi_put_line("Use the reset command to reinitialize the processor\n");
        hdi_cmd_stat = E_ARG;
        return(ERR);
    }

    if (addr == ADR_SELF_IAC
        && (request == 0x81 || request == 0x93 || request == 0x8e))
    {
        hdi_put_line("Use the restart command to change the processor context\n");
        hdi_cmd_stat = E_ARG;
        return(ERR);
    }

    if (_hdi_put_regs() != OK || _hdi_flush_cache() != OK)
        return(ERR);

        com_init_msg();
        if (com_put_byte(SEND_IAC) != OK
            || com_put_byte(~OK) != OK
            || com_put_long(addr) != OK)
        {
            hdi_cmd_stat = com_get_stat();
            return(ERR);
        }

    for (i = 0; i < 4; i++)
            if (com_put_long(iac[i]) != OK)
        {
        hdi_cmd_stat = com_get_stat();
        return(ERR);
        }

        if (_hdi_send(0) == NULL)
            return(ERR);

    return(OK);
}

int
hdi_sysctl(cmd, f1, f2, f3, f4)
int cmd;
int f1;
unsigned int f2;
unsigned long f3, f4;
{
    if ((_hdi_arch != ARCH_CA) && (_hdi_arch != ARCH_JX) && (_hdi_arch != ARCH_HX))
    {
        hdi_cmd_stat = E_ARCH;
        return(ERR);
    }

    if (_hdi_put_regs() != OK || _hdi_flush_cache() != OK)
    return(ERR);

    com_init_msg();
    if (com_put_byte(SYS_CNTL) != OK
    || com_put_byte(~OK) != OK
    || com_put_byte(f1) != OK
    || com_put_byte(cmd) != OK
    || com_put_short(f2) != OK
    || com_put_long(f3) != OK
    || com_put_long(f4) != OK)
    {
    hdi_cmd_stat = com_get_stat();
    return(ERR);
    }

    if (_hdi_send(0) == NULL)
    return(ERR);

    return(OK);
}



/*
 * Get extended CPU data.
 */
hdi_cpu_stat(cpu_status)
CPU_STATUS *cpu_status;
{
     const unsigned char *rp;

      if ((rp = _hdi_send(GET_CPU)) == NULL)
        return(ERR);

    cpu_status->cpu_prcb = get_long(rp);
    cpu_status->cpu_sptb = get_long(rp);
    cpu_status->cpu_ftb = get_long(rp);
    cpu_status->cpu_itb = get_long(rp);
    cpu_status->cpu_isp = get_long(rp);
    cpu_status->cpu_sat = get_long(rp);
    cpu_status->cpu_ctb = get_long(rp);

    return(OK);
}


/*
 * Get mon config info .
 */
hdi_get_monitor_config(mon_config)
HDI_MON_CONFIG *mon_config;
{
    int                 i;
    const unsigned char *rp;

    if ((rp = _hdi_send(GET_MON_CONFIG)) == NULL)
        return(ERR);

    for (i = 0; i < HDI_MAX_FLSH_BNKS; i++)
        mon_config->flash_addr[i] = get_long(rp);
    for (i = 0; i < HDI_MAX_FLSH_BNKS; i++)
        mon_config->flash_size[i] = get_long(rp);

    mon_config->ram_start_addr      = get_long(rp);
    mon_config->ram_size            = get_long(rp);
    mon_config->unwritable_addr     = get_long(rp);
    mon_config->step_info           = get_long(rp);
    mon_config->arch                = get_short(rp);
    mon_config->bus_speed           = get_short(rp);
    mon_config->cpu_speed           = get_short(rp);
    mon_config->monitor             = get_short(rp);
    mon_config->inst_brk_points     = get_short(rp);
    mon_config->data_brk_points     = get_short(rp);
    mon_config->fp_regs             = get_short(rp);
    mon_config->sf_regs             = get_short(rp);
    mon_config->comm_cfg            = get_short(rp);
    for (i = 0; i < HDI_MAX_TRGT_NAME + 1; i++)
        mon_config->trgt_common_name[i] = get_byte(rp);
    mon_config->config_version      = get_short(rp); 
    for (i = 0; i < HDI_MAX_CFG_UNUSED; i++)
        mon_config->unused[i] = get_long(rp);

    return(OK);
}


int
hdi_get_region_cache(region_cache)
REGION_CACHE region_cache;
{
    int region_size = 16;
    int i;

    for (i=0; i<region_size; i++)
        region_cache[i] = cache_by_regions[i]; 

    return(region_size);
}


void
hdi_set_region_cache(region_cache)
REGION_CACHE region_cache;
{
    int region_size = 16;
    int i;

    for (i=0; i<region_size; i++)
        cache_by_regions[i] = region_cache[i]; 
}


int
_hdi_set_mon(mon_priority, break_int)
int mon_priority;
int break_int;
{
    com_init_msg();
    if (com_put_byte(SET_MON) != OK
        || com_put_byte(~OK) != OK
        || com_put_byte(mon_priority) != OK
        || com_put_byte(break_int) != OK)
    {
        hdi_cmd_stat = com_get_stat();
        return(ERR);
    }

    if (_hdi_send(0) == NULL)
        return(ERR);

    return(OK);
}


/*
 * Get monitor priority.
 */
int
hdi_get_monitor_priority(priority)
int *priority;
{
    const unsigned char *rp;

    if ((rp = _hdi_send(GET_MONITOR_PRIORITY)) == NULL)
        {
        hdi_cmd_stat = E_OLD_MON;
        return(ERR);
        }

    *priority = get_byte(rp);

    return(OK);
}


/*
 * Get stop reason.
 */
const STOP_RECORD *
hdi_get_stop_reason()
{
    const STOP_RECORD *stop_reason;
    const unsigned char *rp;

    if ((rp = _hdi_send(GET_STOP_REASON)) == NULL)
        {
        hdi_cmd_stat = E_OLD_MON;
        return((STOP_RECORD *)NULL);
        }

    stop_reason = handle_stop_msg(rp);

    return(stop_reason);
}


/*
 * Get monitor data.
 */
_hdi_get_mon()
{
    const unsigned char *rp;

    if ((rp = _hdi_send(GET_MON)) == NULL)
        return(ERR);

    _hdi_arch = get_byte(rp);
    _hdi_mon_version = get_byte(rp);

    return(OK);
}

/*
 *    Return current Arch 
 */
int
hdi_get_arch()
    {
    return(_hdi_arch);
    }


/*
 * Get target version string.
 */
hdi_version(version, length)
register char *version;
register int length;
{
    register const char *p;

    if (length <= 16)
    {
        hdi_cmd_stat = E_BUFOVFLH;
        return(ERR);
    }

    for (p = _hdi_version_string; *p && length>1; p++, version++, length--)
        *version = *p;

    if (length > 0)    /* paranoia; should always be true */
        *version = '\0';

    if ((p = (const char *)_hdi_send(TRGT_VER)) == NULL)
        return(ERR);

    *version++ = ';';
    *version++ = ' ';
    length -= 2;

    for ( ; *p && length>1; p++, version++, length--)
        *version = *p;
    *version = '\0';

    if (*p != '\0')
    {
        hdi_cmd_stat = E_BUFOVFLH;
        return(ERR);
    }

    return(OK);
}

/*
 * Send user interface command to target
 */
int
hdi_ui_cmd(cmd)
const char *cmd;
{
    const unsigned char *rp;
    int sz;
    int interrupted = FALSE;

    if (_hdi_running)
    {
        hdi_cmd_stat = E_RUNNING;
        return(ERR);
    }

    if (_hdi_put_regs() != OK || _hdi_flush_cache() != OK)
        return(ERR);

    _hdi_invalidate_registers();

    com_init_msg();
    if (com_put_byte(UI_CMD) != OK
        || com_put_byte(~OK) != OK
        || com_put_data((unsigned char *)cmd, strlen(cmd)+1) != OK
        || com_put_msg(NULL, 0) != OK)
    {
        hdi_cmd_stat = com_get_stat();
        return(ERR);
    }

    _hdi_signalled = FALSE;
    do {
        rp = com_get_msg(&sz, COM_WAIT);

        if (_hdi_signalled)
        {
            if (_hdi_break_causes_reset)
            {
            /* Hdi_reset will send the break.  If it does not
             * return OK, hdi_cmd_stat is already set.  */
            if (hdi_reset() == OK)
                hdi_cmd_stat = E_RESET;
                return(ERR);
            }

            if (interrupted)
            {
            /* The user has interrupted more than once; give up. */
                hdi_cmd_stat = E_INTR;
                return(ERR);
            }

            /* Send an interrupt to the target. */
            if (com_intr_trgt() != OK)
            {
                hdi_cmd_stat = com_get_stat();
                return(ERR);
            }

            interrupted = TRUE;
            _hdi_signalled = FALSE;
        }

        if (rp == NULL)
        {
            if (com_get_stat() == E_INTR)
                continue;

            hdi_cmd_stat = com_get_stat();
            return(ERR);
        }

        if (sz < 2)
        {
            hdi_cmd_stat = E_COMM_ERR;
            return(ERR);
        }

        if (rp[1] != OK && rp[1] != 0xfe)
        {
            hdi_cmd_stat = rp[1];
            return(ERR);
        }

        hdi_put_line((char *)(rp+2));
    } while (rp[1] == 0xfe);

    return(OK);
}

void
hdi_signal()
{
    _hdi_signalled = TRUE;
    com_signal();
}



/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: _hdi_send                                     */
/*                                                                           */
/* ACTION: run a completely valided command transaction with the target.     */
/*                                                                           */
/* return: pointer to the response buffer, NULL if failure.                  */
/*                                                                           */
/*****************************************************************************/
const unsigned char *
_hdi_send(cmd)
int cmd;
{
    const unsigned char *rp;
    int sz;

    if (_hdi_running)
    {
        hdi_cmd_stat = E_RUNNING;
        return(NULL);
    }

    if (cmd != 0)
    {
        com_init_msg();
        if (com_put_byte(cmd) != OK
        || com_put_byte(~OK) != OK)
        {
        hdi_cmd_stat = com_get_stat();
        return(NULL);
        }
    }

    /* issue command */
    if ((rp = com_send_cmd(NULL, &sz, COM_WAIT)) == NULL)
    {
        hdi_cmd_stat = com_get_stat();
        return(NULL); 
    }

    if (sz < 2 || (cmd != 0 && rp[0] != (unsigned char)cmd))
    {
        hdi_cmd_stat = E_COMM_ERR;
        return(NULL); 
    }

    if ((hdi_cmd_stat = rp[1]) != OK)
    {
        return(NULL);
    }

    return(rp + 2);
}



/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: hdi_init_app_stack                                         */
/*                                                                           */
/* ACTION:                                                                   */
/*      Interface to a monitor routine to init an application stack          */
/*      immediately after a program has been loaded.  The monitor sets       */
/*      FP and SP to point to its dedicated user stack.  Clients are         */
/*      responsible for setting PFP to a sensible value, whatever that       */
/*      might be (in the case of DB960, PFP is set to 0).                    */
/*                                                                           */
/*****************************************************************************/
int
hdi_init_app_stack()
{
    /*
     * newmon -- monitor sets fp and sp directly to appropriate user stack
     *           values.  Note that we cannot use cached versions of fp &
     *           sp because that would not be compatible with ApLink (a
     *           relocatable monitor).
     * oldmon -- monitor interface does not exist, yet.  Use a kludge (i.e.,
     *           set the stack to FP/SP values that were saved during HDI 
     *           init).
     */
    return (_hdi_set_user_fp_and_sp());
}



/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: hdi_fast_download_set_port                             */
/*                                                                           */
/* ACTION:                                                                   */
/*     Interface to call monitor and set the port for mem_write. */
/*     returns: TRUE if target set the port for mem_write.  */
/*                  or if it is capable                                      */
/*****************************************************************************/
int
hdi_fast_download_set_port(download_cfg)
DOWNLOAD_CONFIG *download_cfg;
{
    const unsigned char *rp;
    int sz;
    unsigned char error, fast_download_set = 0;
    static unsigned char download_cmd[1];
    int    e_device_error;

    
    if (download_cfg->download_selector == FAST_PARALLEL_DOWNLOAD)
		{
	    if (download_cfg->fast_port != FAST_DOWNLOAD_CAPABLE)
	    	{
            if (com_parallel_init(download_cfg->fast_port) != OK)
                return(ERR);
            _hdi_fast_download.fast_device_init = com_parallel_init;
            _hdi_fast_download.fast_device_put = com_parallel_put;
            _hdi_fast_download.fast_device_end = com_parallel_end;
            _hdi_fast_download.download_type = FAST_PARALLEL_DOWNLOAD;
		    }
		download_cmd[0] = PARALLEL_DOWNLOAD;	
		e_device_error = E_PARALLEL_DOWNLOAD_NOT_SUPPORTED;
        }
#ifdef MSDOS
	else if (download_cfg->download_selector == FAST_PCI_DOWNLOAD)
		{
	    if (download_cfg->fast_port != FAST_DOWNLOAD_CAPABLE)
		    {
	        if (com_pci_init(&download_cfg->init_pci) != OK)
                return(ERR);
            _hdi_fast_download.fast_device_init = com_pci_init;
            _hdi_fast_download.fast_device_put = com_pci_put;
            _hdi_fast_download.fast_device_end = com_pci_end;
            _hdi_fast_download.download_type = FAST_PCI_DOWNLOAD;
            if (! download_cfg->pci_quiet_connect)
                {
                char buf[200];

                /* Tell user what target is being accessed. */
                sprintf(buf,
         "PCI->vndid %#x, dvcid %#x, bus# %#x, dev# %#x, func# %#x, comm %s\n",
                        download_cfg->init_pci.vendor_id,
                        download_cfg->init_pci.device_id,
                        download_cfg->init_pci.bus,
                        download_cfg->init_pci.dev,
                        download_cfg->init_pci.func,
                        download_cfg->init_pci.comm_mode == COM_PCI_IOSPACE ?
                            "io" : "mmap"
                      );
                hdi_put_line(buf);
                }
	    	}
		download_cmd[0] = PCI_DOWNLOAD;	
		e_device_error = E_PCI_COMM_NOT_SUPPORTED;
        }
#endif /* MSDOS */

    if (download_cfg->download_selector == STOP_FAST_DOWNLOAD)
        {
        if (_hdi_fast_download.download_active == TRUE)
            {
            download_cmd[0] = _hdi_fast_download.download_cmd;
            _hdi_fast_download.download_active = FALSE;

            /* close the download channel */
            if (hdi_cmd_stat == OK)
                {
                (void) _hdi_fast_download.fast_device_put(
                                                    (void *)download_cmd, 
                                                    1,
                                                    NULL);
                }

            if ((rp = com_get_msg(&sz, COM_WAIT)) == NULL)
                {
                hdi_targ_intr();
                return ERR;
                }
            else if (get_byte(rp) != download_cmd[0])
                {
                hdi_cmd_stat = E_COMM_ERR;
                return ERR;
                }
            else if ((error=get_byte(rp)) != 0)
                {
                _hdi_fast_download.fast_device_end();
                hdi_cmd_stat = error;
                return ERR;
                }
            }

        return(_hdi_fast_download.fast_device_end());
        }

    if (download_cfg->fast_port == FAST_DOWNLOAD_CAPABLE)
        {
        if (_hdi_fast_download.download_active == TRUE)
            hdi_fast_download_set_port(END_FAST_DOWNLOAD);
        fast_download_set = 0;
        }
    else
        {
        /* start a fast download */
		_hdi_fast_download.download_cmd = download_cmd[0];	
        _hdi_fast_download.download_active = TRUE;
        /* fast download capable send message to target */ 
        fast_download_set = 0xff;
        }

    /* 5/ff/0 = just return if capable, 5/ff/>0 = start download (PARALLEL)*/
    /* 6/ff/0 = just return if capable, 6/ff/>0 = start download  (PCI)*/
    com_init_msg();
    if (com_put_byte(download_cmd[0]) != OK
        || com_put_byte(~OK) != OK
        || com_put_byte(fast_download_set) != OK)
        {
        _hdi_fast_download.download_active = FALSE;
        hdi_cmd_stat = com_get_stat();
        return(ERR);
        }

    if ((rp = _hdi_send(0)) == NULL)
        {
        _hdi_fast_download.download_active = FALSE;
        hdi_cmd_stat = e_device_error; 
        return(ERR);
        }
    return(OK);
}


/****************************************************************************
 *
 * FUNCTION NAME: hdi_set_mmr_reg
 *
 * PARAMETERS:
 *     mmr_offset   -- offset into the MMR (i.e., address) that is to be
 *                     modified.
 *     new_value    -- new value to place in MMR.  This value is irrelevant
 *                     if "mask" is 0.
 *     mask         -- see JX HX sysctl instruction.
 *     old_value    -- value before MMR[mmr_offset] was modified.
 *
 * ACTION:
 *     There are occasions when the client wishes to modify the MMR via
 *     a sysctl instruction.  Note that if a mask value of 0 is used, this
 *     routine may be used to only read an MMR location.
 *
 * RETURNS:
 *     OK if the modification succeeded, ERR otherwise.
 *
 *     Note also that the previous MMR value is returned by reference.
 *
 * CAVEATS:
 *     Only works on JX or HX processors (to date).
 ****************************************************************************/

int
hdi_set_mmr_reg(mmr_offset, new_value, mask, old_value)
ADDR mmr_offset;
REG new_value, mask, *old_value;
{
    const unsigned char *rp;

    com_init_msg();
    if (com_put_byte(SET_MMR_REG) != OK
        || com_put_byte(~OK) != OK
        || com_put_long(mmr_offset) != OK
        || com_put_long(new_value) != OK
        || com_put_long(mask) != OK)
    {
        hdi_cmd_stat = com_get_stat();
        return(ERR);
    }

	if ((rp = _hdi_send(0)) == NULL)
	    return(ERR);

    *old_value = get_long(rp);

    return(OK);
}
