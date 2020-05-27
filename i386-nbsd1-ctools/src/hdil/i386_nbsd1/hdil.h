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

#ifndef HDIL_H
#define HDIL_H

#ifndef HDI_COMMON_H
#   include "common.h"
#endif

#ifndef HDI_REGS_H
#   include "hdi_regs.h"
#endif

#ifndef HDI_ERRS_H
#   include "hdi_errs.h"
#endif

#ifndef HDI_BRK_H
#   include "hdi_brk.h"
#endif

#ifndef HDI_ARCH_H
#   include "hdi_arch.h"
#endif

#ifndef HDI_STOP_H
#   include "hdi_stop.h"
#endif

#ifndef HDI_COM_H
#   include "hdi_com.h"
#endif

#ifndef HDI_MCFG_H
#   include "hdi_mcfg.h"
#endif

#ifndef HDI_GMU_H
#   include "hdi_gmu.h"
#endif

#ifdef __STDC__
#   define HDI_PARAMS(paramlist)   paramlist
#else
#   define HDI_PARAMS(paramlist)   ()
#endif

/* A couple of defns used by hdi_convert_number(). */
#define HDI_CVT_SIGNED   0
#define HDI_CVT_UNSIGNED 1

/* A couple of defns used by hdi_aplink_sync(). */
#define HDI_APLINK_RESET 0
#define HDI_APLINK_WAIT  1

extern int hdi_cmd_stat;

typedef struct {
    int reset_time;        /* Time in ms it takes target to reset */
    int intr_trgt;        /* If true, interrupt target during init */
    int break_causes_reset;    /* If true, target resets when host
                 * sends break. */
    int mon_priority;
    int tint;
    int no_reset;        /* If true, host does not reset target during
                 * initial connection. */
} HDI_CONFIG;

typedef struct {
    ADDR cpu_prcb;        /* PRCB Address                     */
    ADDR cpu_sptb;        /* System Procedure Table Address   */
    ADDR cpu_ftb;        /* Fault Table Address              */
    ADDR cpu_itb;        /* Interrupt Table Address          */
    ADDR cpu_isp;        /* Base of Interrupt Stack          */
    ADDR cpu_sat;        /* (KX/MC) SAT/ST Address           */
    ADDR cpu_ctb;        /* (CA) Control Table Address       */
} CPU_STATUS;

enum DOWNLOAD_TYPE {STOP_FAST_DOWNLOAD, FAST_PARALLEL_DOWNLOAD, FAST_PCI_DOWNLOAD};

typedef struct {
	enum DOWNLOAD_TYPE download_selector;
	int                pci_quiet_connect;  /* Boolean, set to TRUE if no info
                                            * messages are desired when a PCI
                                            * download is initiated.
                                            */
	char *             fast_port;
	COM_PCI_CFG        init_pci;
} DOWNLOAD_CONFIG; 

#define NO_DOWNLOAD_CONFIG      (void *)NULL
#define FAST_DOWNLOAD_CAPABLE   (char *)NULL
#define PCI_UNUSED_FAST_PORT    "" 
extern DOWNLOAD_CONFIG *END_FAST_DOWNLOAD, *PARALLEL_CAPABLE, *PCI_CAPABLE;

/* define the type for caching by regions */
/* assumed 16 regions for now 9/10/93 */
#define hdi_max_regions 16
typedef unsigned char REGION_CACHE[hdi_max_regions]; /* 1 = cache on for this region */

/*
 * host command extension defs
 */
#define HDI_EINIT 0
#define HDI_EPOLL 1
#define HDI_EDATA 2
#define HDI_EEXIT 3

/* Data structures to handle simple command line option parsing. */
#ifdef HOST
#   ifdef __STDC__
        typedef int (*HDI_CMDLINE_OPT_HNDLR)(const char *, const char *);
#   else
        typedef int (*HDI_CMDLINE_OPT_HNDLR)();
#   endif /* __STDC__ */

        typedef struct 
        {
            char                  *name;     /* option name */
            HDI_CMDLINE_OPT_HNDLR hndlr;     /* function to process option */
            unsigned char         needs_arg; /* T -> opt requires argument */
        } HDI_CMDLINE_OPT;
#endif /* HOST */


/* ---------------------- External Declarations ------------------------- */

extern int  hdi_aplink_sync      HDI_PARAMS((int));
extern int  hdi_aplink_enable    HDI_PARAMS((unsigned long, unsigned long));
extern int  hdi_aplink_switch    HDI_PARAMS((unsigned long, unsigned long));
extern int  hdi_convert_number   HDI_PARAMS((const char *num, 
                                             long       *arg,
                                             int        arg_type, 
                                             int        base,
                                             const char *error_prefix));
extern int  hdi_invalid_arg      HDI_PARAMS((const char *));
extern int  hdi_opt_arg_required HDI_PARAMS((const char *, const char *));
extern int  hdi_set_lmadr        HDI_PARAMS((unsigned long, unsigned long));
extern int  hdi_set_lmmr         HDI_PARAMS((unsigned long, unsigned long));
extern int  hdi_set_mcon         HDI_PARAMS((unsigned long, unsigned long));
extern int  hdi_set_gmu_reg      HDI_PARAMS((int, int, HDI_GMU_REG *));
extern int  hdi_update_gmu_reg   HDI_PARAMS((int, int, int));
extern int  hdi_get_gmu_reg      HDI_PARAMS((int, int, HDI_GMU_REG *));
extern int  hdi_get_gmu_regs	 HDI_PARAMS((HDI_GMU_REGLIST *));

#ifdef __STDC__
extern int hdi_init(HDI_CONFIG *config, int *arch);
extern int hdi_term(int term_flag);
extern int hdi_mem_read(ADDR addr, void *buf, unsigned size,
                        int bypass_cache, int mem_size);
extern int hdi_mem_write(ADDR addr, const void *buf, unsigned size,
                         int verify, int bypass_cache, int mem_size);
extern int hdi_mem_copy(ADDR dst, ADDR src, unsigned long size,
                        int dst_mem_size, int src_mem_size);
extern int hdi_mem_fill(ADDR addr, const void *pattern,
                        int pattern_size, unsigned long count, int mem_size);
extern int hdi_regs_get(UREG regs);
extern int hdi_reg_get(int reg_number, REG *reg);
extern int hdi_regs_put(const UREG regs);
extern int hdi_reg_put(int reg_number, REG reg);
extern int hdi_regfp_get(int reg_number, int format, FPREG *reg);
extern int hdi_regfp_put(int reg_number, int format, const FPREG *reg);
extern int hdi_version(char *version, int length);
extern int hdi_set_prcb(ADDR prcb_address);
extern int hdi_iac(ADDR dest, const unsigned long iac[4]);
extern int hdi_sysctl(int cmd, int f1, unsigned int f2,
                      unsigned long f3, unsigned long f4);
extern const STOP_RECORD *hdi_targ_go(int mode);
extern const STOP_RECORD *hdi_targ_intr(void);
extern const STOP_RECORD *hdi_poll();
extern void hdi_signal(void);
extern int hdi_reset(void);
extern int hdi_restart(void);
extern int hdi_download(const char *fname, ADDR *start_ip,
                        unsigned long textoff, unsigned long dataoff,
						int zero_bss, DOWNLOAD_CONFIG *fast_config, int quiet);
extern int hdi_bp_set(ADDR addr, int type, int flags);
extern int hdi_bp_del(ADDR addr);
extern int hdi_bp_rm_all(void);
extern int hdi_bp_type(ADDR addr);
extern int hdi_cpu_stat(CPU_STATUS *);
extern int hdi_eeprom_check(ADDR addr, unsigned long length,
                unsigned long *eeprom_size, ADDR prog[2]);
extern int hdi_eeprom_erase(ADDR addr, unsigned long length);
extern int hdi_ui_cmd(const char *cmd);
extern const char *hdi_get_message();
extern void hdi_async_input();
extern void hdi_flush_user_input();
extern void hdi_inputline(char *buffer, int length);
extern int  hdi_init_app_stack(void);
extern int  hdi_get_monitor_priority(int *priority);
extern int  hdi_get_monitor_config(HDI_MON_CONFIG *);
extern int  hdi_arch(void);
extern int  hdi_get_region_cache(REGION_CACHE region_cache); 
extern void hdi_set_region_cache(REGION_CACHE region_cache); 
extern const STOP_RECORD *hdi_get_stop_reason();
extern int hdi_fast_download_set_port(DOWNLOAD_CONFIG *download_cfg);
extern int hdi_set_mmr_reg(ADDR mmr_offset,
                             REG new_value,
                             REG mask,
                             REG *old_value);

/* Imported functions */
extern void hdi_put_line(const char *);
extern void hdi_user_put_line(const char *, int);
extern int hdi_user_get_line(char *, int);
extern int hdi_cmdext(int, const unsigned char *, int);
extern void hdi_get_cmd_line(char *, int);

#else /* __STDC__ */

extern int hdi_init();
extern int hdi_term();
extern int hdi_mem_read();
extern int hdi_mem_write();
extern int hdi_mem_copy();
extern int hdi_mem_fill();
extern int hdi_regs_get();
extern int hdi_reg_get();
extern int hdi_regs_put();
extern int hdi_reg_put();
extern int hdi_regfp_get();
extern int hdi_regfp_put();
extern int hdi_version();
extern int hdi_set_prcb();
extern int hdi_iac();
extern int hdi_sysctl();
extern const STOP_RECORD *hdi_targ_go();
extern const STOP_RECORD *hdi_targ_intr();
extern const STOP_RECORD *hdi_poll();
extern void hdi_signal();
extern int hdi_reset();
extern int hdi_restart();
extern int hdi_download();
extern int hdi_bp_set();
extern int hdi_bp_del();
extern int hdi_bp_rm_all();
extern int hdi_bp_type();
extern int hdi_cpu_stat();
extern int hdi_eeprom_check();
extern int hdi_eeprom_erase();
extern int hdi_ui_cmd();
extern const char *hdi_get_message();
extern void hdi_async_input();
extern void hdi_flush_user_input();
extern void hdi_inputline();
extern int  hdi_init_app_stack();
extern int  hdi_get_monitor_priority();
extern int  hdi_get_monitor_config();
extern int  hdi_arch();
extern int  hdi_get_region_cache(); 
extern void  hdi_set_region_cache(); 
extern const STOP_RECORD *hdi_get_stop_reason();
extern int hdi_fast_download_set_port();
extern int hdi_set_mmr_reg();

/* Imported functions */
extern void hdi_put_line();
extern void hdi_user_put_line();
extern int hdi_user_get_line();
extern int hdi_cmdext();
extern void hdi_get_cmd_line();
#endif /* __STDC__ */

#endif /* HDIL_H */
