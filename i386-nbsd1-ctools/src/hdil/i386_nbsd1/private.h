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
/* $Header: /ffs/p1/dev/src/hdil/common/RCS/private.h,v 1.5 1995/08/22 00:55:09 cmorgan Exp $$Locker:  $ */

#ifndef HDI_PRIVATE_H
#define HDI_PRIVATE_H

#ifndef HDIL_H
#   include "hdil.h"
#endif

#define DBG_FLAG CFLAG
#define ADR_SELF_IAC 0xff000010

typedef struct 
{
    int download_active;
	enum DOWNLOAD_TYPE download_type;
	unsigned char download_cmd;
#ifdef __STDC__
	int (*fast_device_init)(void *); 
	int (*fast_device_put)(unsigned char *, unsigned long, unsigned short *);
	int (*fast_device_end)(void);
#else
	int (*fast_device_init)();
	int (*fast_device_put)();
	int (*fast_device_end)();
#endif
} fast_download;

extern REGION_CACHE cache_by_regions;
extern int _hdi_running;
extern int _hdi_arch;
extern int _hdi_mon_version;
extern int _hdi_signalled;
extern int _hdi_save_errno;
extern int _hdi_break_causes_reset;
extern fast_download _hdi_fast_download;

extern int do_reset                  HDI_PARAMS((int));
extern int _hdi_set_user_fp_and_sp   HDI_PARAMS((void));

#ifdef __STDC__
extern const unsigned char *_hdi_send(int cmd);
extern const STOP_RECORD *_hdi_serve(int interrupted);
extern void _hdi_serve_init(void);
extern void _hdi_io_init(void);
extern void _hdi_bp_init(void);
extern int _hdi_flush_cache(void);
extern void _hdi_invalidate_cache(ADDR addr, unsigned sz);
extern int _hdi_put_regs();
extern void _hdi_invalidate_registers();
extern void _hdi_set_ip_fp(REG ip, REG fp);
extern int _hdi_set_mon(int monpriority, int break_int);
extern int _hdi_get_mon();
extern int _hdi_bp_instr(ADDR, unsigned long *);
extern void _hdi_replace_bps(void *buf, ADDR addr,
			     unsigned int size, int mem_size);
extern int _hdi_check_bps(ADDR addr, unsigned int size);
#else /* __STDC__ */
extern const unsigned char *_hdi_send();
extern const STOP_RECORD *_hdi_serve();
extern void _hdi_serve_init();
extern void _hdi_io_init();
extern void _hdi_bp_init();
extern int _hdi_flush_cache();
extern void _hdi_invalidate_cache();
extern int _hdi_put_regs();
extern void _hdi_invalidate_registers();
extern void _hdi_set_ip_fp();
extern int _hdi_set_mon();
extern int _hdi_get_mon();
extern int _hdi_bp_instr();
extern void _hdi_replace_bps();
extern int _hdi_check_bps();
extern int _hdi_mem_write();
#endif /* __STDC__ */

#endif /* HDI_PRIVATE_H */
