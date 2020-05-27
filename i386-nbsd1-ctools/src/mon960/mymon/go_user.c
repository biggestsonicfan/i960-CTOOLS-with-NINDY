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

#include "mon960.h"
#include "retarget.h"
#include "hdi_com.h"
#include "hdi_stop.h"
#include "hdi_errs.h"

#define FMARK 0x66003e00
#define OP_CALL     0x09    /* CTRL memory(efa) */
#define OP_BAL      0x0B    /* CTRL memory(efa) */
#define OP_BALX     0x85    /* MEM memory(efa) src/dest(dst:o) */
#define OP_CALLX    0x86    /* MEM memory(efa) */
#define OP_CALLS    0x660    /* REG source_1(val:o) */

static int bp_step(unsigned long bp_instr);

static const unsigned long fmark = FMARK;
static int go_mode;
static int bp_flag;
static ADDR bp_ip;
static unsigned long next_instr;
static ADDR prev_ip;
static REG next_fp;
static ADDR next_ip;

int
prepare_go_user(int mode, int flag, unsigned long bp_instr)
{
    unsigned long instr;
    unsigned char opcode;
    int instr_len;

    go_mode = mode;
    register_set[REG_TC] &= ~INSTRUCTION_TRACE;

    if (flag)
        if (bp_step(bp_instr) != OK)
        return ERR;

    switch (go_mode)
    {
        case GO_RUN:
        case GO_SHADOW:
        break;
        case GO_NEXT:
        prev_ip = (ADDR)register_set[REG_RIP];
        next_fp = register_set[REG_FP];
        if (load_mem(prev_ip, WORD, &instr, WORD) != OK)
            return ERR;
        opcode = instr >> 24;

        switch (opcode)
        {
            case OP_CALLS>>4:
                /* Check the full opcode, not just the top 8 bits */
                if (((opcode << 4) | ((instr >> 7) & 0xf)) != OP_CALLS)
                break;
            /* fall through */
            
            case OP_CALL:
            case OP_CALLX:
            case OP_BAL:
            case OP_BALX:
            instr_len =
                ((opcode == OP_BALX || opcode == OP_CALLX)
/* MEMB */            && (instr & (1L<<12)) != 0L
/* IP+displacement */        && ((instr & (0xfL<<10)) == (5<<10)
/* base+displacement */            || (instr & (1L<<13)) != 0L))
                ? 2*WORD : 1*WORD;

            next_ip = prev_ip + instr_len;

            if (load_mem(next_ip, WORD, &next_instr, WORD) != OK
                || store_mem(next_ip,WORD,&fmark,WORD,TRUE) != OK)
            {
                if (cmd_stat == E_VERIFY_ERR)
                cmd_stat = E_SWBP_ERR;
                return ERR;
            }

            /* If there was already a breakpoint at the next
             * instruction, this is the same as GO_RUN. */
            if (next_instr == FMARK)
                go_mode = GO_RUN;
            break;

            default:
            go_mode = GO_STEP;
            register_set[REG_TC] |= INSTRUCTION_TRACE;
            break;
        }
        break;
        case GO_STEP:
        register_set[REG_TC] |= INSTRUCTION_TRACE;
        break;
    }

    register_set[REG_TC] |= BREAKPOINT_TRACE;

    pgm_bp_hw();
    board_go_user();

    return OK;
}

static int
bp_step(unsigned long bp_instr)
{
    bp_ip = (ADDR)register_set[REG_RIP];
    if (store_mem(bp_ip, WORD, &bp_instr, WORD, TRUE) != OK)
        return ERR;
    /* single step restored instruction */
    register_set[REG_TC] |= INSTRUCTION_TRACE;
    bp_flag = TRUE;
    return OK;
}


void
exit_user(STOP_RECORD *stop_reason)
{
    if (bp_flag)
    {
        store_mem(bp_ip, WORD, &fmark, WORD, FALSE);
        bp_flag = FALSE;

        if (go_mode == GO_RUN || go_mode == GO_NEXT)
        {
        /* If we just stopped to put the breakpoint back,
         * continue execution. */
        if (stop_reason->reason == STOP_TRACE
            && stop_reason->info.trace.type == INSTRUCTION_TRACE
            && stop_reason->info.trace.ip == bp_ip)
        {
            register_set[REG_TC] &= ~INSTRUCTION_TRACE;
            exit_mon(FALSE);        /* return to user */
            /* NOTREACHED */
        }

        /* If there was some other stop cause, report it; but
         * don't report the instruction trace; the user doesn't
         * need to see that. */
        if (stop_reason->reason & STOP_TRACE)
            if (stop_reason->info.trace.type == INSTRUCTION_TRACE)
            stop_reason->reason &= ~STOP_TRACE;
            else
            stop_reason->info.trace.type &= ~INSTRUCTION_TRACE;
        }
    }

    if (go_mode == GO_NEXT)
    {
        if ((stop_reason->reason & STOP_BP_SW) != 0
        && stop_reason->info.sw_bp_addr == next_ip)
        {
        register_set[REG_RIP] = (REG)next_ip;

        /* Check for recursive call */
        if (register_set[REG_FP] != next_fp)
        {
            bp_step(next_instr);
            exit_mon(FALSE);        /* return to user */
        }

        /* Fake single-step */
        stop_reason->reason &= ~STOP_BP_SW;
        stop_reason->reason |= STOP_TRACE;
        stop_reason->info.trace.type = INSTRUCTION_TRACE;
        stop_reason->info.trace.ip = prev_ip;
        }

        store_mem(next_ip, WORD, &next_instr, WORD, FALSE);
    }

    board_exit_user(stop_reason);
    /* Turn off led 3 ater user program exits */
    leds(0,4);

    monitor(stop_reason);
}


/*
 * App_go_user and app_exit_user are called via the system procedure table
 * each time the application is started or stopped.  The application may
 * replace the entries in the system procedure table to cause its own
 * handlers to be called.  These stubs are called when the application has
 * not provided its own handlers.  These routines should never contain any
 * code.
 */
void
app_go_user()
{
}

void
app_exit_user()
{
}
