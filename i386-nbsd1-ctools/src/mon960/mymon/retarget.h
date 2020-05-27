/*(CB*/
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

#ifndef RETARGET_H
#define RETARGET_H

#ifndef HDI_COMMON_H
#   include "common.h"
#endif

#ifndef I960_H
#   include "i960.h"
#endif

#define TIMER_API_UNAVAIL      0
#define TIMER_API_VIA_CPU      1
#define TIMER_API_VIA_TRGT_HW  2

/* External comm-based retargeting vars. */
extern unsigned long baud_rate;


/* External timer-based retarget vars. */
extern unsigned int ghist_count;

/* <BOARD>.hw definitions */
extern int arch;
extern char arch_name[];
extern char board_name[];
extern char target_common_name[];
extern ADDR unwritable_addr;

extern void init_hardware();
extern int  get_int_vector(int);
extern void init_imap_reg(PRCB *prcb);
extern int  clear_break_condition();
extern void board_reset();
extern void board_go_user();
extern void board_exit_user();

/* Parallel driver definitions */
extern int  para_comm_supported(void);
extern int  para_dnload_supported(void);
extern void parallel_err(void);
extern int  parallel_init(void);
extern int  parallel_read(unsigned char *, unsigned int, 
                          unsigned short *, unsigned int, unsigned int);

/* PCI driver definitions */
extern int  pci_supported(void);
extern void pci_err(void);
extern void pci_not_inuse(void);
extern int  pci_inuse(void);
extern int  pci_connect_request(void);
extern int  pci_init(void);
extern int  pci_intr(void);
extern void pci_write_data_mb(unsigned long);

extern int  pci_dev_open(void);
extern int  pci_dev_close(int);
extern int  pci_dev_read(int, unsigned char *, int, int);
extern int  pci_dev_write(int, const unsigned char *, int);

extern int  pci_read(unsigned char *, 
                     unsigned int, 
                     unsigned short *, 
                     unsigned int,
                     unsigned int);
extern int  pci_write(unsigned char *, 
                     unsigned int, 
                     unsigned short *, 
                     unsigned int);

/* serial driver definitions */
extern int  serial_supported(void);
extern void serial_init();
extern int  serial_baud(int, unsigned long);
extern void serial_set(unsigned long baud);
extern void serial_loopback(int flag);
extern int  serial_intr(void);
extern int  serial_open(void);
extern int  serial_getc();
extern void serial_putc(int c);
extern int  serial_read(int, unsigned char *, int, int);
extern int  serial_write(int, const unsigned char *, int);

/* flash  definitions */
extern unsigned long eeprom_size;
extern ADDR eeprom_prog_first, eeprom_prog_last;

extern int is_eeprom(ADDR addr, unsigned long length);
extern int check_eeprom(ADDR addr, unsigned long length);
extern int erase_eeprom(ADDR addr, unsigned long length);
extern int write_eeprom(ADDR addr, const void *data, int data_size);
extern int flash_supported(int, ADDR *, unsigned long *);

/* timer definitions */
extern void         ghist_reload_timer(unsigned int);
extern void         ghist_suspend_timer(void);
extern unsigned int timer_read(int timer);
extern unsigned int timer_init(int          timer, 
                               unsigned int init_time, 
                               void         *timer_isr,
                               unsigned int *prev_prio);
extern void timer_term(int timer);
extern unsigned long timer_extended_speed(void);
extern void timer_for_onboard_only(int onboard_only);
extern void timer_suspend(void);
extern void timer_resume(void);
extern int  timer_supported(void);  
/*
 * timer_supported() returns one of these values:
 *
 *    TIMER_API_UNAVAIL     - target does not support Mon960 timer API
 *    TIMER_API_VIA_CPU     - target ported to Mon960 timer API, hw 
 *                            provided by Hx/Jx processors.
 *    TIMER_API_VIA_TRGT_HW - target ported to Mon960 timer API, hw
 *                            provided by target.
 */


/* Future functionality (maybe :-)) */
extern int i2c_supported(void);
extern int jtag_supported(void);
extern int mon_relocatable(void);

#endif  /* ! defined RETARGET_H */
