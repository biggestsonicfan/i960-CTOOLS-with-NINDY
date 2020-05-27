/*******************************************************************************
 * 
 * Copyright (c) 1993 Intel Corporation
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

#ifndef __SDM_H__
#define __SDM_H__

#define INIT_BENTIME         220
#define BENTIME              221
#define TERM_BENTIME         222
#define INIT_BENTIME_NOINT   223
#define BENTIME_NOINT        224
#define INIT_EEPROM          225
#define IS_EEPROM            226
#define CHECK_EEPROM         227
#define ERASE_EEPROM         228
#define WRITE_EEPROM         229
#define SDM_OPEN             230
#define SDM_READ             231
#define SDM_WRITE            232
#define SDM_LSEEK            233
#define SDM_CLOSE            234
#define SDM_ISATTY           235
#define SDM_STAT             236
#define SDM_FSTAT            237
#define SDM_RENAME           238
#define SDM_UNLINK           239
#define SDM_TIME             240
#define SDM_SYSTEM           241
#define SDM_ARG_INIT         242
#define INIT_P_TIMER     	 243
#define SET_PRCB             244
#define GET_PRCBPTR          245
#define TERM_P_TIMER 	     246
#define SET_MON_PRIORITY     247
#define GET_MON_PRIORITY     248
#define SET_TIMER            249
#define CAVE_DISPATCH	     250
#define BENTIME_ONBOARD_ONLY 252
#define POST_TEST            253
#define MON_ENTRY            254
#define SDM_EXIT             257

#ifdef MON_LINK
/*
 * The following should use defines from above, but the
 * ANSI C standard does not have preprocessing occur in #pragmas.
 */
#pragma system mon_init_bentime     = 220	/*INIT_BENTIME*/
#pragma system mon_bentime          = 221	/*BENTIME*/
#pragma system mon_term_bentime     = 222	/*TERM_BENTIME*/
#pragma system mon_init_bentime_noint   = 223	/*INIT_BENTIME_NOINT*/
#pragma system mon_bentime_noint    = 224	/*BENTIME_NOINT*/
#pragma system mon_init_eeprom      = 225	/*INIT_EEPROM*/
#pragma system mon_is_eeprom        = 226	/*IS_EEPROM*/
#pragma system mon_check_eeprom     = 227	/*CHECK_EEPROM*/
#pragma system mon_erase_eeprom     = 228	/*ERASE_EEPROM*/
#pragma system mon_write_eeprom     = 229	/*WRITE_EEPROM*/
#pragma system sdm_open             = 230	/*SDM_OPEN*/
#pragma system sdm_read             = 231	/*SDM_READ*/
#pragma system sdm_write            = 232	/*SDM_WRITE*/
#pragma system sdm_lseek            = 233	/*SDM_LSEEK*/
#pragma system sdm_close            = 234	/*SDM_CLOSE*/
#pragma system sdm_isatty           = 235	/*SDM_ISATTY*/
#pragma system sdm_stat             = 236	/*SDM_STAT*/
#pragma system sdm_fstat            = 237	/*SDM_FSTAT*/
#pragma system sdm_rename           = 238	/*SDM_RENAME*/
#pragma system sdm_unlink           = 239	/*SDM_UNLINK*/
#pragma system sdm_time             = 240	/*SDM_TIME*/
#pragma system sdm_system           = 241	/*SDM_SYSTEM*/
#pragma system sdm_arg_init         = 242	/*SDM_ARG_INIT*/
#pragma system mon_init_p_timer     = 243	/*init_p_timer*/
#pragma system set_prcb             = 244	/*SET_PRCB*/
#pragma system get_prcbptr          = 245	/*GET_PRCBPTR*/
#pragma system mon_term_p_timer     = 246	/*term_p_timer*/
#pragma system set_mon_priority     = 247	/*SET_MON_PRIORITY*/
#pragma system get_mon_priority     = 248	/*GET_MON_PRIORITY*/
#pragma system mon_set_timer        = 249	/*MON_SET_TIMER*/
#pragma system cave_dispatch	    = 250	/*CAVE_DISPATCH*/
#pragma system mon_bentime_onboard_only = 252	/*bentime select*/
#pragma system post_test            = 253	/*post_test*/
#pragma system mon_entry            = 254	/*MON_ENTRY*/
#pragma system sdm_exit             = 257	/*SDM_EXIT*/

#endif

extern unsigned int mon_init_bentime();
extern unsigned int mon_bentime();
extern void mon_term_bentime();
extern unsigned long mon_init_bentime_noint(int Mhz);
extern unsigned long mon_bentime_noint();
extern void mon_init_eeprom();
extern int mon_is_eeprom(ADDR addr, unsigned long length);
extern int mon_check_eeprom(ADDR addr, unsigned long length);
extern int mon_erase_eeprom(ADDR addr, unsigned long length);
extern int mon_write_eeprom(ADDR start_addr, const void *data_arg, int data_size);
extern int sdm_open(const char * filename, int mode, int cmode, int * fd);
extern int sdm_read(int fd, char * buf, int sz, int * nread);
extern int sdm_write(int fd, const char * buf, int sz, int * nwritten);
extern int sdm_lseek(int fd, long offset, int whence, long * new_offset);
extern int sdm_close(int fd);
extern int sdm_isatty(int fd, int * result);
extern int sdm_stat(const char * path, void * bp);
extern int sdm_fstat(int fd, void * bp); /* bp is actually a (struct stat *). */
extern int sdm_rename(const char * old, const char * new);
extern int sdm_unlink(const char * path);
extern int sdm_time(long * time);
extern int sdm_system(const char * cp, int * result);
extern int sdm_arg_init(char * buf, int len);
extern void set_prcb(void * prcb);
extern void * get_prcbptr();
extern void set_mon_priority(unsigned int new_priority);
extern unsigned int get_mon_priority();
extern void mon_bentime_onboard_only(int onboard_only);
extern void post_test();
extern void mon_entry();
extern void mon_init_p_timer();
extern void mon_term_p_timer();
extern void sdm_exit(int code);

/************************************************/
/* mon_set_timer()  Tells the monitor who wants to use which timer.
   Each target has a collection of physical timer devices, and each
   physical timer device is associacted with a logical timer.  The
   mapping is done in the monitor itself, so the user can tell ghist or
   bentime to use a certain logical timer, and this information will
   percolate down to this call.  The monitor will then map the logical
   timer to a physical timer and the timer is set.

   client :: identifies who is calling mon_set_timer.  Currently, ghist
   and bentimer are the only ones that make sense, however, provision
   exists for extension of an 'other' client.

   timer :: identifies the logical timer ghist or bentime wishes to be
   used.  If there is no such mapping, an error is returned.  If
   DEFAULT_TIMER is passed, or if this function is never called, the 4.5
   backwards compatible timer is set.

   timer_isr :: identifies the interrupt subroutine to be used at each
   timer interrupt.  If NULL, then the timer is in no interrupt mode
   (such as init_bentime_noint())

   */
typedef enum {DEFAULT_TIMER, t0, t1, t2, t3, t4, t5, NO_TIMER} logical_timer;
typedef enum {ghist_timer_client, bentime_timer_client, other_timer_client} timer_client;
int
mon_set_timer(timer_client client, logical_timer timer, void (*timer_isr)());

#endif /* ifndef __SDM_H__ */
