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
/*****************************************************************************/
/*                                                                           */
/* MODULE NAME: init.c                                 */
/*                                                                           */
/* DESCRIPTION:                                  */
/*    init.c initializes the connection to the target board.             */
/*                                                                           */
/*****************************************************************************/
/* $Header: /ffs/p1/dev/src/hdil/common/RCS/init.c,v 1.9 1995/09/07 01:03:23 cmorgan Exp $$Locker:  $ */

#define ACK 06

#include <stdio.h>
#include "private.h"
#include "hdi_com.h"
#include "dbg_mon.h"
extern  const STOP_RECORD *handle_stop_msg();

static int check_app_stack_loadable HDI_PARAMS((void));

static const unsigned char reset_cmd[2] = { RESET_TRGT, (unsigned char)~OK };
static int opened = FALSE;
static int reset_time;
static int mon_priority;
static int tint;
static int mon_returns_user_stack = FALSE;
static REG fp_initial_user_stack, sp_initial_user_stack;

/*
 * initialize the hostcmds layer
 */
hdi_init(config, arch)
HDI_CONFIG *config;
int *arch;
{
    const unsigned char * request;
	int size;

    reset_time = config->reset_time;
    mon_priority = config->mon_priority;
    _hdi_break_causes_reset = config->break_causes_reset;

    /*
     * initialize host-target communications environment
     */
    while (!opened)
        {
        if (com_init() != OK)
            {
            hdi_cmd_stat = com_get_stat();
            return(ERR);
            }
        
        if (config->intr_trgt)
            {
            _hdi_running = TRUE;
            if (com_intr_trgt() != OK)
                {
                _hdi_running = FALSE;
                com_term();
                return(ERR);
                }
			if ((request = com_get_msg(&size, COM_WAIT)) == NULL)
                {
                com_term();
                return(ERR);
                }
			handle_stop_msg(request);
            if (do_reset(FALSE) != OK)
                {
                com_term();
                return(ERR);
				}
            }
        else if (do_reset(config->no_reset ? FALSE : 2) != OK)
            {
            if (com_error() == OK)
                continue;

            com_term();
            return(ERR);
            }

        opened = TRUE;
        }

    *arch = _hdi_arch;

    /* Set up data structures for init'ing application SP and FP values. */
    if (check_app_stack_loadable() != OK)
        return(ERR);

    /* reset target reset flag after connection */ 
    com_get_target_reset();

    return(OK);
}

/*
 * terminate the hostcmds layer
 */
int
hdi_term(term_flag)
int term_flag;
{
    int r = OK;

    if (opened &&
              ! term_flag && 
                  ! _hdi_running && 
                      ! (hdi_cmd_stat == E_COMM_ERR ||
                             hdi_cmd_stat == E_COMM_TIMO))
        {
        /* COMM appears to be healthy & it's appropriate to clean up */

        if (hdi_bp_rm_all() == ERR)
		    {
            hdi_cmd_stat = com_get_stat();
            r = ERR;
			}

        /* If communications is down, don't waste time trying to
         * reset the target. */
        if (hdi_cmd_stat != E_COMM_ERR && hdi_cmd_stat != E_COMM_TIMO)
            {
            if (com_put_msg(reset_cmd, sizeof(reset_cmd)) != OK)
                { 
                hdi_cmd_stat = com_get_stat();
                r = ERR;
                }
            }
        }

    if (com_term() != OK)
        {
        hdi_cmd_stat = com_get_stat();
        r = ERR;
        }

    opened = FALSE;

    return(r);
}


/*
 * cause the target to do a cold start reset.
 */
hdi_reset()
{
    int r;

    r = do_reset(TRUE);
    if (hdi_cmd_stat == E_TARGET_RESET)
        {
        com_get_target_reset();
        hdi_cmd_stat = OK;
        r = OK;
        }
    return r;
}

int
do_reset(restart)
int restart;
{
    int r;

    if (restart != FALSE)    /* only reset if requested */ 
        {
        /*
         * If the board is configured so break causes reset, go ahead and
         * send the break to reset the board.  This is a convenience for
         * the user, but serve() depends on this behavior as well.
         */
        if (_hdi_break_causes_reset)
            {
            if (com_intr_trgt() != OK)
                {
                hdi_cmd_stat = com_get_stat();
                return(ERR);
                }
            }

        /* If the user manually reset the board, or we reset it by sending a
          * break above, we won't have to send it a reset command.  Com_reset
          * returns ERR if it can't establish contact with the target, OK if
         * it set the target to the correct baud rate, and something else if
         * the target was already at the correct baud rate (meaning it had
         * not been reset). */

        /* try using baud rate if the board was never reset */
        if (restart == TRUE)
		    com_put_msg_once(reset_cmd, sizeof(reset_cmd));

        if ((r=com_reset(reset_time)) == ERR)
            {
            hdi_cmd_stat = com_get_stat();
            return(ERR);
            }

        if (r != OK) 
			{
		    if ((com_put_msg(reset_cmd, sizeof(reset_cmd)) != OK) ||
               ((r=com_reset(reset_time)) == ERR))
                {
                hdi_cmd_stat = com_get_stat();
                return(ERR);
				}
            }
        }

    (void)hdi_bp_rm_all();

    _hdi_running = FALSE;
    _hdi_fast_download.download_active = FALSE;
    _hdi_invalidate_registers();
    _hdi_flush_cache();

    if ((r=_hdi_set_mon(mon_priority, tint)) != OK) 
        {
        /* if board not reset we just did an autobaud so we need to set_mon */
        if ((restart != FALSE) || ((r=_hdi_set_mon(mon_priority, tint)) != OK))
            {
            hdi_cmd_stat = com_get_stat();
            return(ERR);
            }
        }

    if (_hdi_get_mon() != OK)
        {
        hdi_cmd_stat = com_get_stat();
        return(ERR);
        }

    _hdi_bp_init();
    _hdi_io_init();

    if (check_app_stack_loadable() != OK)
        {
        hdi_cmd_stat = com_get_stat();
        return(ERR);
        }

    return(OK);
}



int
hdi_restart()
{
    _hdi_io_init();
    return (OK);
}



/*
 * Check to see if this version of the monitor will return values pointing
 * to a known, valid USER stack location.  If yes, nothing to do.  If no,
 * snag the current values of SP and FP from the monitor and assume
 * they point to a valid USER stack.  In the latter case, cache these
 * values for later use by the host debugger.
 */
static int
check_app_stack_loadable()
{
    if (_hdi_send(GET_USER_STACK) == NULL) 
    {
        if (HDI_SERVICE_UNIMP(hdi_cmd_stat))
        {
            /* 
             * Old monitor -- fetch and cache current values of FP & SP
             * (which are presumed to point to a valid app USER stack).
             */

            if (hdi_reg_get(REG_FP, &fp_initial_user_stack) != OK)
                return (ERR);
            if (hdi_reg_get(REG_SP, &sp_initial_user_stack) != OK)
                return (ERR);
        }
        else
        {
            /* some kind of comm failure */

            return (ERR);
        }
    }
    else
    {
        /*
         * This monitor supports a command to give us the current values
         * of the user stack on demand.
         */

        mon_returns_user_stack = TRUE;
    }
    return (OK);
}



/*
 * Set up an application's USER stack based upon services available from
 * the current monitor.
 */
int
_hdi_set_user_fp_and_sp()
{
    const unsigned char *rp;

    if (mon_returns_user_stack)
    {
        if ((rp = _hdi_send(GET_USER_STACK)) == NULL)
            return (ERR);
        else
        {
            fp_initial_user_stack = get_long(rp);
            sp_initial_user_stack = get_long(rp);
        }
    }
    /* else, use cached values. */

    if (hdi_reg_put(REG_FP, fp_initial_user_stack) != OK)
        return (ERR);
    if (hdi_reg_put(REG_SP, sp_initial_user_stack) != OK)
        return (ERR);
    return (OK);
}
