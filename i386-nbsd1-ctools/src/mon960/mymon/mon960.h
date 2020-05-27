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

/* $Header: /ffs/p1/dev/src/mon960/common/RCS/mon960.h,v 1.16 1995/11/12 07:08:52 cmorgan Exp $ */

/***************************************************************************
 *
 * NAME:         mon960.h
 *
 * DESCRIPTION:  A file to declare mon960 externs that aren't listed in
 *               retarget.h .
 *
 ***************************************************************************/

#ifndef MON960_H
#define MON960_H

#ifndef HDI_COMMON_H
#   include "common.h"
#endif

#ifndef I960_H
#   include "i960.h"
#endif

/* -------------------------------- Defns -------------------------------- */

typedef int  (*FAST_INIT_FNPTR)(void);
typedef int  (*FAST_RD_FNPTR)(unsigned char *,
                              unsigned int,
                              unsigned short *,
                              unsigned int,
                              unsigned int);
typedef void (*GHIST_FNPTR)(int);
typedef void (*VOID_FNPTR)(void);

/* --------------------------------- Data -------------------------------- */

extern unsigned int _bentime_counter;
extern unsigned int __cpu_bus_speed;     /* BUS speed in MHZ. Clock doubling,
                                          * tripling, etc. not factored in.
                                          * Useful info for interfacing
                                          * Hx/Jx CPU timers.
                                          */
extern unsigned int __cpu_speed;         /* CPU speed in MHz, accounts for
                                          * clock doubling, tripling, etc.
                                          */
extern const        char base_version[];
extern const        char build_date[];
extern char         *step_string;
extern int          instr_brkpts_avail; 
extern int          data_brkpts_avail;
extern int          break_vector;
extern int          mon_priority;
extern PRCB         *prcb_ptr;
extern int          lost_stop_reason;
extern char         mon_version_byte;
extern int          boot_g0;
extern const unsigned short CRCtab[];

/* ------------------------------- Functions ----------------------------- */

extern void           _bentime_isr(void);
extern void           blink(int);
extern int            change_priority(unsigned int);
extern int            clr_bp(unsigned long);
extern void           clear_pending(int);
extern void           comm_config(void);
extern int            copy_mem(ADDR, int , ADDR, int, int);
extern void           disable_dcache(void);
extern void           eat_time(int);
extern void           enable_dcache(void);
extern void           enable_interrupt(unsigned int);
extern void           exit_mon(int);
extern void           fast_download(const void *, int);
extern void           fast_download_cfg(FAST_INIT_FNPTR, 
                                        VOID_FNPTR, 
                                        FAST_RD_FNPTR);
extern void           fatal_error(char, int, int, int, int);
extern int            fill_mem(ADDR, int, const void *, int, int );
extern int            get_bentime_timer(void);
extern unsigned int   get_brkreq(void);
extern void           *get_default_bentime_isr(void);
extern GHIST_FNPTR    get_ghist_callback(void);
extern unsigned int   get_ghist_timer(void);
extern unsigned int   get_imap(unsigned int);
extern unsigned int   get_mask(void);
extern unsigned int   get_mon_priority(void);
extern unsigned int   get_onboard_only(void);
extern unsigned int   get_pending(void);
extern PRCB           *get_prcbptr(void);
extern int            get_tcr(unsigned int);
extern void           *get_timer_default_isr(void);
extern unsigned int   get_timer_inuse(int);
extern unsigned int   get_timer_intr(int);
extern unsigned int   get_timer_irq(int);
extern unsigned int   get_timer_offset(int);
extern unsigned int   get_timer_vector(int);
extern void           _init_p_timer(int);
extern unsigned int   interrupt_register_read(void);
extern void           interrupt_register_write(int *);
extern void           intr_entry(void);
extern void           led(int);
extern void           leds(int, int);
extern unsigned int   load_byte(ADDR);
extern int            load_mem(ADDR, int, void *, int);
extern int            mmr_sysctl(int, int, int);
extern void           monitor(const void *);
extern void           parallel_download(const void *);
extern void           pause(void);
extern int            pci_comm(void);
extern void           pci_download(const void *);
extern void           pgm_bp_hw(void);
extern void           send_sysctl(unsigned int, unsigned int, unsigned int);
extern void           send_iac(int, void *);
extern int            serial_comm(void);
extern void           set_break_vector(int, PRCB *prcb);
extern int            set_bp(int, int, unsigned long);
extern void           set_imap(unsigned int, unsigned int);
extern void           set_mask(unsigned int);
extern void           set_mon_priority(unsigned int);
extern void           set_pending(unsigned int);
extern int            set_proc_mcon(unsigned int, unsigned long);
extern void           set_tcr(unsigned int, unsigned int);
extern unsigned int   set_timer_interrupt(int, void *);
extern unsigned int   set_timer_inuse(int);
extern unsigned int   set_timer_not_inuse(int);
extern void           set_tmr(unsigned int, unsigned int);
extern void           set_trr(unsigned int, unsigned int);
extern int            sprtf(unsigned char buffer[], int max_size, char *fmt, int arg0, int arg1, int arg2, int arg3); 
extern void           store_byte(int, ADDR);
extern int            store_mem(ADDR, int, const void *, int, int);
extern void           store_quick_byte(int, ADDR);
#if BIG_ENDIAN_CODE
   extern void swap_data_byte_order(int, void *, int);
#endif /* __i960_BIG_ENDIAN__ */
extern void           _term_p_timer(void);
extern void           update_fpr(int, int, unsigned char *);

#endif
