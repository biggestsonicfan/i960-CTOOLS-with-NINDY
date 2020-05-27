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
/*******************************************************************
 *
 * MODULE NAME: dbg_mon.h
 *
 * DESCRIPTION:
 *    dbg_mon.h contains definitions of structures and constants
 *    used in sending commands from the host to the monitor.
 *******************************************************************/

#ifndef HDI_DBG_MON_H
#define HDI_DBG_MON_H

/* Command Codes */
/* The values of these codes must match the positions of the entries in
 * the command table in hi_msg.c.  There are gaps so that future commands
 * can be placed together with related commands without renumbering.  */
#define READ_MEM                    1
#define WRITE_MEM                   2
#define COPY_MEM                    3
#define FILL_MEM                    4
#define PARALLEL_DOWNLOAD           5
#define PCI_DOWNLOAD                6
#define GET_REGS                    7
#define PUT_REGS                    8
#define RESET_TRGT                  9
#define EXEC_USR                    10
#define TRGT_VER                    11
#define PCI_VERIFY                  12
#define SET_PROC_MCON               13    /* Not KX or SX */
#define SET_HW_BP                   14
#define CLR_HW_BP                   15
#define CHECK_EEPROM                16
#define ERASE_EEPROM                17
#define GET_CPU                     19
#define GET_MON                     20
#define SET_MON                     21
#define SET_PRCB                    22
#define SEND_IAC                    23    /* Kx only */
#define SYS_CNTL                    24    /* Not KX or SX */
#define UI_CMD                      25
#define GET_USER_STACK              26
#define GET_MONITOR_PRIORITY        27
#define GET_STOP_REASON             28
#define SET_MMR_REG                 29
#define OBSOLETE_MON_CALL_NO_1      30    /* Obsolete, do _not_ use.
                                           * This jump table entry formerly
                                           * returned incomplete and
                                           * undocumented monitor config 
                                           * info.  The HDI entry point to
                                           * this monitor service was 
                                           * undocumented as well.
                                           */
#define APLINK_WAIT                 31
#define APLINK_RESET                32
#define APLINK_SWITCH               33
#define APLINK_ENABLE               34
#define OBSOLETE_MON_CALL_NO_2      35    /* Obsolete, do _not_ use.
                                           * This jump table entry formerly
                                           * returned monitor config info
                                           * that was later expanded
                                           * internally by the 960 tools
                                           * group prior to customer
                                           * release.
                                           */
#define GET_MON_CONFIG              36

#define PCI_TEST                    49 
#define STOP                        50


/*
 * These macros are used to read data off the stream.
 */
#define get_byte(p) (*(p)++)
#define get_short(p) ((p)+=2,_get_short((p)-2))
#define get_long(p) ((p)+=4,_get_long((p)-4))

#ifdef TARGET
extern int _get_short(const unsigned char *p);
extern long _get_long(const unsigned char *p);
#endif /* TARGET */

#ifdef HOST
/* These definitions could be used for the target as well, but they make
 * the monitor larger.  Since speed is not important (after all, we just
 * read this data off a serial line!), there is no reason to use them.  */
#define _get_short(p) ((unsigned short)(p)[1] << 8 | (unsigned short)(p)[0])
#define _get_long(p) ((unsigned long)(p)[3] << 24 \
              | (unsigned long)(p)[2] << 16 \
              | (unsigned long)(p)[1] << 8 \
              | (unsigned long)(p)[0])
#endif /* HOST */


/* Command Templates */

/* CMD_TMPL is used to send commands that require no data  */
/* and to return command statuses that require no further information */

typedef struct {
        unsigned char cmd;
    unsigned char stat;
} CMD_TMPL;

typedef struct {
        unsigned char cmd;
    unsigned char stat;
    char string[1];
} STRING_TMPL;


#endif /* HDI_DBG_MON_H */
