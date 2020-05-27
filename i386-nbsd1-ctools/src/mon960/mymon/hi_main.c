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

#include <string.h>

#include "retarget.h"
#include "mon960.h"
#include "dbg_mon.h"
#include "hdi_arch.h"
#include "hdi_stop.h"
#include "hdi_errs.h"
#include "hdi_mcfg.h"
#include "hdi_com.h"

extern int    prepare_go_user(int, int, unsigned long);
extern int    ui_cmd(char *, int);
extern void * _mon_memcpy(void *, const void *, size_t);
extern size_t _mon_strlen(const char *);

extern int          dram_size;
extern unsigned int user_stack;
extern int          _Bdata;

/*===========================================================================*/
/* LOCAL FUNCTIONS                                                           */
/*===========================================================================*/

static void nop(const void *);
static void read_mem(const void *cmd_buf);
static void write_mem(const void *cmd_buf);
static void copy_mem_cmd(const void *cmd_buf);
static void fill_mem_cmd(const void *cmd_buf);
static void get_regs(const void *cmd_buf);
static void put_regs(const void *cmd_buf);
static void exec_usr(const void *cmd_buf);
static void target_ver(const void *cmd_buf);
static void set_hw_bp(const void *cmd_buf);
static void clr_hw_bp(const void *cmd_buf);
static void erase_eeprom_cmd(const void *cmd_buf);
static void check_eeprom_cmd(const void *cmd_buf);
static void get_mon(const void *cmd_buf);
static void pci_verify(const void *cmd_buf);
static void set_mon(const void *cmd_buf);
static void do_ui_cmd(const void *cmd_buf);
static void get_user_stack(const void *cmd_buf);
static void get_monitor_priority(const void *cmd_buf);
static void set_mmr_reg(const void *cmd_buf);
static void get_mon_config(const void *cmd_buf);
static void set_mcon_cmd(const void *);
static void obsolete(const void *);
static void unimplemented(const void *);

static void stop_message(const STOP_RECORD *stop_reason);
static void get_stop_reason();

static const STOP_RECORD *last_stop_reason;

void aplink_switch_cmd(const void *);
void aplink_reset_cmd(const void *);
void aplink_enable_cmd(const void *);
void aplink_wait_cmd(const void *);
void set_prcb_cmd(const void *cmd_buf);
void get_cpu(const void *cmd_buf);
void send_iac_cmd(const void *cmd_buf);
void sys_ctl_cmd(const void *cmd_buf);

#define PUTBUF(msg) com_put_msg((const unsigned char *)&(msg), sizeof(msg))

/*===========================================================================*/
/* LOCAL VARIABLES                                                           */
/*===========================================================================*/

typedef void (*cmdptr)(const void *cmd_buf);

/* The positions of these entries must match the defines in dbg_mon.h */
static const cmdptr cmd_table[] = { nop,
    read_mem, write_mem, copy_mem_cmd, fill_mem_cmd, parallel_download,
	pci_download, get_regs, put_regs, board_reset, exec_usr, target_ver,
    pci_verify, set_mcon_cmd, set_hw_bp, clr_hw_bp, check_eeprom_cmd, 
    erase_eeprom_cmd, NULL, get_cpu, get_mon, set_mon, set_prcb_cmd, 
    send_iac_cmd, sys_ctl_cmd, do_ui_cmd, get_user_stack, 
    get_monitor_priority, get_stop_reason, set_mmr_reg,
    obsolete,            /* OBSOLETE_MON_CALL_NO_1 (30)  */
    aplink_wait_cmd,     /* APLINK_WAIT            (31)  */
    aplink_reset_cmd,    /* APLINK_RESET           (32)  */
    aplink_switch_cmd,   /* APLINK_SWITCH          (33)  */
    aplink_enable_cmd,   /* APLINK_ENABLE          (34)  */
    obsolete,            /* OBSOLETE_MON_CALL_NO_2 (35)  */
    get_mon_config,      /* GET_MON_CONFIG         (36)  */
    unimplemented,       /*   available            (37)  */
    unimplemented,       /*   available            (38)  */
    unimplemented,       /*   available            (39)  */
    unimplemented,       /*   available            (40)  */
    unimplemented,       /*   available            (41)  */
    unimplemented,       /*   available            (42)  */
    unimplemented,       /*   available            (43)  */
    unimplemented,       /*   available            (44)  */
    unimplemented,       /*   available            (45)  */
    unimplemented,       /*   available            (46)  */
    unimplemented,       /*   available            (47)  */
    unimplemented,       /*   available            (48)  */
    unimplemented,       /*   available            (49)  */
    };

/*
 * There are several buffers in this file which are created on the stack.
 * They may be as large as MAX_MSG_SIZE, but to save space on the stack,
 * they are no larger than 256 bytes.  Enlarging this value may improve
 * performance on read_mem when reading large amounts of data, which is rare.
 */
#if MAX_MSG_SIZE-4 > 256
#define BUFSIZE 256
#else
#define BUFSIZE MAX_MSG_SIZE-4
#endif


/*****************************************************************************
 *
 * FUNCTION NAME: host_cmds
 *
 * ACTION:
 *      main target command loop.  Get command from host and invoke
 *      proper execution routine.
 *
 *****************************************************************************/
void
hi_main(const STOP_RECORD *stop_reason)
{
    const unsigned char *cmd_buf;
    int cbuf_sz;
    int cmd;

    last_stop_reason = stop_reason;

	/* if break intr then break is done when we send a stop message */
	if (break_flag)
		break_flag = FALSE;

    if (!lost_stop_reason && stop_reason != NULL)
        stop_message(stop_reason);

    while (1)
        {
        if ((cmd_buf = com_get_msg(&cbuf_sz, COM_WAIT_FOREVER)) == NULL)
           {
           break_flag = FALSE;
           continue;
           }
        cmd = cmd_buf[0];

        if (cmd < sizeof(cmd_table)/sizeof(cmd_table[0])
            && cmd_buf[1] == 0xff && cmd_table[cmd] != NULL)
            {
            (*cmd_table[cmd])(cmd_buf);
            }
        else
            {
            CMD_TMPL response;
            response.cmd = cmd;
            response.stat = E_VERSION;
            PUTBUF(response);
            }
        }
}

static void
stop_message(const STOP_RECORD *stop_reason)
{
    unsigned long reason = stop_reason->reason;

    /* Note: this routine does not check for errors for the simple
     * reason that there is no action to perform if there is an error. */

    com_init_msg();
    com_put_byte(STOP);
    com_put_byte(~0);
    com_put_long(reason);
    com_put_long(register_set[REG_IP]);
    com_put_long(register_set[REG_FP]);

    if (reason & STOP_EXIT)
        com_put_long(stop_reason->info.exit_code);

    if (reason & STOP_BP_SW)
        com_put_long(stop_reason->info.sw_bp_addr);

    if (reason & STOP_BP_HW)
        com_put_long(stop_reason->info.hw_bp_addr);

    if (reason & STOP_BP_DATA0)
        com_put_long(stop_reason->info.da0_bp_addr);

    if (reason & STOP_BP_DATA1)
        com_put_long(stop_reason->info.da1_bp_addr);

    if (reason & STOP_TRACE)
    {
        com_put_byte(stop_reason->info.trace.type);
        com_put_long(stop_reason->info.trace.ip);
    }

    if (reason & STOP_FAULT)
    {
        com_put_byte(stop_reason->info.fault.type);
        com_put_byte(stop_reason->info.fault.subtype);
        com_put_long(stop_reason->info.fault.ip);
        com_put_long(stop_reason->info.fault.record);
    }

    if (reason & STOP_INTR)
        com_put_byte(stop_reason->info.intr_vector);

    com_put_msg((const unsigned char *)NULL, 0);
}


static void
get_stop_reason(const void *cmd_buf)
{
stop_message(last_stop_reason);
}


static void
nop(const void *cmd_buf)
{
    CMD_TMPL response;
    response.cmd = ((unsigned char *)cmd_buf)[0];
    response.stat = OK;
    PUTBUF(response);
}



static void
obsolete(const void *cmd_buf)
{
    CMD_TMPL response;

    response.cmd = ((unsigned char *)cmd_buf)[0];
    response.stat = E_VERSION;
    PUTBUF(response);
}



/* Stub function for HI jump table. */
static void
unimplemented(const void *cmd_buf)
{
    /*
     * Call a common routine to put out a "Not implemented in this version
     * of the monitor" message.
     */
    obsolete(cmd_buf);
}


/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: read_mem                                                   */
/*                                                                           */
/* ACTION:                                                                   */
/*      read_mem implements the target memory read command.                  */
/*                                                                           */
/*****************************************************************************/
static void
read_mem(const void *p_cmd)
{
    register const unsigned char *rp = (unsigned char *)p_cmd + 2;
    ADDR addr;
    int data_size, mem_size;

    addr = get_long(rp);
    data_size = get_short(rp);
    mem_size = get_byte(rp);

    while (data_size)
    {
        int actual;
        unsigned char data[BUFSIZE];

        if (data_size < sizeof(data))
            actual = data_size;
        else
            actual = sizeof(data);

        com_init_msg();
        com_put_byte(READ_MEM);

        if (load_mem(addr, mem_size, data, actual) != OK)
        {
        com_put_byte(cmd_stat);
        com_put_msg((const unsigned char *)NULL, 0);
        break;
        }

#if BIG_ENDIAN_CODE
        /* load_mem() returns data in target byte order, but we must
         * send it to the host in little-endian byte order, so swap.
         */
        swap_data_byte_order(mem_size, data, actual);
#endif /* __i960_BIG_ENDIAN__ */

        com_put_byte(OK);
        com_put_short(actual);
        com_put_data(data, actual);
        com_put_msg(NULL, 0);

        addr += actual;
        data_size -= actual;
    }
}


/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: write_mem                                                  */
/*                                                                           */
/*****************************************************************************/
static void
write_mem(const void *p_cmd)
{
    register unsigned char *rp = (unsigned char *)p_cmd + 2;
    ADDR addr;
    int data_size, mem_size, read_back;
    unsigned char *data;
        CMD_TMPL response;

    addr = get_long(rp);
    data_size = get_short(rp);
    mem_size = get_byte(rp);
    read_back = get_byte(rp);
    data = rp;

        response.cmd = WRITE_MEM;
        response.stat = OK;

#if BIG_ENDIAN_CODE
    /* store_mem() expects target byte order, but we got it from
     * the host in little-endian byte order, so swap.
     */
    swap_data_byte_order(mem_size, data, data_size);
#endif /* __i960_BIG_ENDIAN__ */

    if (store_mem(addr, mem_size, data, data_size, read_back) != OK)
        response.stat = cmd_stat;
#if HX_CPU
    else
    {
        /* 
         * A user may have modified one of the four SFR's shadowed in the
         * MMR.  If so, update the SFR to match.  This is an important
         * step, ensuring that when the monitor is exited, the old SFR
         * contents do not clobber the new MMR contents (which is a real
         * problem when programming the GMU via HDI's mem write API).
         */

        if (data_size == sizeof(REG) && mem_size == sizeof(REG))
        {
            if (addr == GCON_ADDR)
                register_set[REG_GCON] = *(REG *) data;
            else if (addr == IPND_ADDR)
                register_set[REG_IPND] = *(REG *) data;
            else if (addr == IMSK_ADDR)
                register_set[REG_IMSK] = *(REG *) data;
            else if (addr == ICON_ADDR)
                register_set[REG_ICON] = *(REG *) data;
        }
    }
#endif

    PUTBUF(response);
}


/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: fill_mem_cmd                                               */
/*                                                                           */
/* ACTION:                                                                   */
/*      fills an area of memory with requested value                         */
/*                                                                           */
/*****************************************************************************/
static void
fill_mem_cmd(const void *p_cmd)
{
    register const unsigned char *rp = (unsigned char *)p_cmd + 2;
    ADDR addr;
    unsigned long count;
    int pattern_size, mem_size;
    const unsigned char *pattern;
        CMD_TMPL response;

    addr = get_long(rp);
    count = get_long(rp);
    mem_size = get_byte(rp);
    pattern_size = get_byte(rp);
    pattern = rp;

    response.cmd = FILL_MEM;
    response.stat = OK;

        if (fill_mem(addr, mem_size, pattern, pattern_size, count) != OK)
        response.stat = cmd_stat;

    PUTBUF(response);
}


/*
 * Copy target memory from one area to another.
 */
static void
copy_mem_cmd(const void *p_cmd)
{
    register const unsigned char *rp = (unsigned char *)p_cmd + 2;
    ADDR src, dest;
    unsigned long length;
    int src_mem_size, dest_mem_size;
        CMD_TMPL response;

    src = get_long(rp);
    src_mem_size = get_byte(rp);
    dest = get_long(rp);
    dest_mem_size = get_byte(rp);
    length = get_long(rp);

    response.cmd = COPY_MEM;
    response.stat = OK;

    if (copy_mem(dest, dest_mem_size, src, src_mem_size, length) != OK)
        response.stat = cmd_stat;

    PUTBUF(response);
}


/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: get_regs                                                   */
/*                                                                           */
/* ACTION:                                                                   */
/*      get_regs returns the application register values                     */
/*                                                                           */
/*****************************************************************************/
static void
get_regs(const void *p_cmd)
{
    register const unsigned char *rp = (unsigned char *)p_cmd + 2;
    unsigned long mask0, mask1;
    int i;

    mask0 = get_long(rp);
    mask1 = get_long(rp);

    com_init_msg();
    com_put_byte(GET_REGS);
    com_put_byte(OK);

        for (i = 0; i < NUM_REGS; i++)
    {
        if (i < 32 ? (mask0 & (1L << i)) : (mask1 & (1L << (i-32))))
        com_put_long(register_set[i]);
    }

        for (i = 0; i < NUM_FP_REGS; i++)
    {
        if (arch == ARCH_KB || arch == ARCH_SB)
        {
        if (NUM_REGS+i < 32
            ? mask0 & (1L << (NUM_REGS+i))
            : mask1 & (1L << (NUM_REGS+i-32)))
        {
        com_put_data((unsigned char *)&fp_register_set[i].fp80, 10);
        com_put_data((unsigned char *)&fp_register_set[i].fp64.value, 8);
        com_put_byte(fp_register_set[i].fp64.flags);
        }
        }        
    }

    com_put_msg(NULL, 0);
}


#if HXJX_CPU
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: set_mmr_reg                                                   */
/*                                                                           */
/* ACTION:                                                                   */
/*      set_mmr_reg returns the *peicied mmr regiter after setting it.       */
/*                                                                           */
/*****************************************************************************/
static void
set_mmr_reg(const void *p_cmd)
{
    register const unsigned char *rp = (unsigned char *)p_cmd + 2;
    unsigned long mask, value, mmr_offset, mmr_value;

    mmr_offset = (get_long(rp) << 16); 
    value      = get_long(rp);
    mask       = get_long(rp);
    mmr_value  = mmr_sysctl((mmr_offset | 0x0500), value, mask);

#if HX_CPU
    if (mask != 0)
    {
        /* 
         * A user may have modified one of the four SFR's shadowed in the
         * MMR.  If so, update the SFR to match.  This is an important
         * step, ensuring that when the monitor is exited, the old SFR
         * contents do not clobber the new MMR contents (which is a real
         * problem when programming the GMU via the HDI).
         */

        ADDR *mmr_addr;

        mmr_addr = (ADDR *) ((0xff00 << 16) | (mmr_offset >> 16));
        if (mmr_addr == (ADDR *) GCON_ADDR)
            register_set[REG_GCON] = *mmr_addr;
        else if (mmr_addr == (ADDR *) IPND_ADDR)
            register_set[REG_IPND] = *mmr_addr;
        else if (mmr_addr == (ADDR *) IMSK_ADDR)
            register_set[REG_IMSK] = *mmr_addr;
        else if (mmr_addr == (ADDR *) ICON_ADDR)
            register_set[REG_ICON] = *mmr_addr;
    }
#endif

    com_init_msg();
    com_put_byte(SET_MMR_REG);
    com_put_byte(OK);
    com_put_long(mmr_value);

    com_put_msg(NULL, 0);
}
#else
static void
set_mmr_reg(const void *p_cmd)
{
    CMD_TMPL response;

    response.cmd = SET_MMR_REG;
    response.stat = E_ARCH;
    PUTBUF(response);
}
#endif /* HXJX */


/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: put_regs                                                   */
/*                                                                           */
/* ACTION:                                                                   */
/*      put_regs updates the `ureg' array with the values passed from the    */
/*      host.                                                                */
/*                                                                           */
/*****************************************************************************/
static void
put_regs(const void *p_cmd) 
{
    register const unsigned char *rp = (unsigned char *)p_cmd + 2;
    unsigned long mask0, mask1;
    int i;
        CMD_TMPL       response;

    /* The first two words are masks indicating which registers follow. */
    mask0 = get_long(rp);
    mask1 = get_long(rp);

        for (i = 0; i < NUM_REGS; i++)
    {
        if (i < 32 ? (mask0 & (1L << i)) : (mask1 & (1L << (i-32))))
        register_set[i] = get_long(rp);
    }

        for (i = 0; i < NUM_FP_REGS; i++)
    {
        if (arch == ARCH_KB || arch == ARCH_SB)
        {
        if (NUM_REGS+i < 32
            ? mask0 & (1L << (NUM_REGS+i))
            : mask1 & (1L << (NUM_REGS+i-32)))
        {
        /* The first word of an FP register is the format:
         * FP_80BIT or FP_64BIT.  The register value follows.
         * The value passed to update_fpr should be aligned on
         * a 16-byte boundary, so rp isn't passed directly.
         * Instead, the value is copied into a struct variable,
         * which the compiler will align on a 16-byte boundary.
         */
        struct { unsigned char value[10]; } align_struct;
        int format = get_byte(rp);
        int size = format == FP_80BIT ? 10 : 8;
        _mon_memcpy(align_struct.value, rp, size), rp += size;
        update_fpr(i, format, align_struct.value);
        }
        }
    }

    response.cmd = PUT_REGS;
    response.stat = OK;

    PUTBUF(response);
}



/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: get_monitor_priority                                       */
/*                                                                           */
/* ACTION:                                                                   */
/*      get_monitor_priority returns the current actual monitor priority.    */
/*      Note: this may be different than mon_priority (see set_mon_priority  */
/*      in monitor.c and switch_stack in entry.s).                           */
/*                                                                           */
/*****************************************************************************/
static void
get_monitor_priority(const void *p_cmd)
{
    com_init_msg();
    com_put_byte(GET_MONITOR_PRIORITY);
    com_put_byte(OK);

    com_put_byte(mon_priority);
    com_put_msg(NULL, 0);
}


/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: pci_verify                                                 */
/*                                                                           */
/* ACTION:                                                                   */
/*      pci_verify  puts the passed in value in the data MB PCI for verifing */
/*      this port connects to th specifed PCI asddress                       */
/*                                                                           */
/*****************************************************************************/
static void
pci_verify(const void *p_cmd)
{
    register const unsigned char *rp = (unsigned char *)p_cmd + 2;
    CMD_TMPL    response;

    pci_write_data_mb(get_long(rp));

    response.cmd = PCI_VERIFY;
    response.stat = OK;
    PUTBUF(response);
}


/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: get_mon_config                                             */
/*                                                                           */
/* ACTION:                                                                   */
/*      get_mon_config returns selected monitor and target info.             */
/*                                                                           */
/*****************************************************************************/
static void
get_mon_config(const void *p_cmd)
{
    int           i, timer_info; 
    short         mask, sf_regs;
    char          name[HDI_MAX_TRGT_NAME + 1];
    ADDR          flash_addr[HDI_MAX_FLSH_BNKS];
    unsigned long flash_size[HDI_MAX_FLSH_BNKS];

    com_init_msg();
    com_put_byte(GET_MON_CONFIG);
    com_put_byte(OK);

    (void) flash_supported(HDI_MAX_FLSH_BNKS, flash_addr, flash_size);

    /* Flash bank starting address(es) */
    for (i = 0; i < HDI_MAX_FLSH_BNKS; i ++)
        com_put_long(flash_addr[i]);

    /* Flash bank sizes(es) */
    for (i = 0; i < HDI_MAX_FLSH_BNKS; i ++)
        com_put_long(flash_size[i]);

    com_put_long((ADDR) &_Bdata);      /* DRAM start address */
    com_put_long(dram_size);

    com_put_long(unwritable_addr);
#if KX_CPU || SX_CPU
    com_put_long(-1);                  /* CPU step info            */
#else
    com_put_long(boot_g0);             /* CPU step info            */
#endif
    com_put_short(arch);               /* processor architecture   */
    com_put_short(__cpu_bus_speed);    /* bus speed, in MHz.       */
    com_put_short(__cpu_speed);        /* Processor speed, in MHz. */

    /* monitor info */
    mask  = (mon_relocatable() ? HDI_MONITOR_RELOC : 0);
#if BIG_ENDIAN_CODE
    mask |= HDI_MONITOR_BENDIAN;
#endif
    timer_info = timer_supported();
    if (timer_info & TIMER_API_VIA_CPU)
        mask |= HDI_MONITOR_CPU_HW_TMR;
    else if (timer_info & TIMER_API_VIA_TRGT_HW)
        mask |= HDI_MONITOR_TRGT_HW_TMR;

    com_put_short(mask);

    com_put_short(instr_brkpts_avail);
    com_put_short(data_brkpts_avail);

    /* # floating point regs */
    com_put_short((arch == ARCH_KB || arch == ARCH_SB) ? 4 : 0);

    if (arch == ARCH_HX)
        sf_regs = 5;
    else if (arch == ARCH_CA)
        sf_regs = 3;
    else
        sf_regs = 0;

    com_put_short(sf_regs);      /* # special function regs */

    /* comm capabilities */
    mask = 0;
    if (serial_supported())
        mask |= HDI_CFG_SERIAL_CAPABLE;
    if (pci_supported())
        mask |= HDI_CFG_PCI_CAPABLE;
    if (jtag_supported())
        mask |= HDI_CFG_JTAG_CAPABLE;
    if (i2c_supported())
        mask |= HDI_CFG_I2C_CAPABLE;
    if (para_dnload_supported())
        mask |= HDI_CFG_PARA_DNLD;
    if (para_comm_supported())
        mask |= HDI_CFG_PARA_COMM;
    com_put_short(mask);

    /* Target's common name, as opposed to its actual product (board) name. */
    strncpy(name, target_common_name, HDI_MAX_TRGT_NAME);
    name[HDI_MAX_TRGT_NAME] = '\0';
    com_put_data(name, sizeof(name));

    /* config version */
    com_put_short(-1);
    for (i = 0; i < HDI_MAX_CFG_UNUSED; i ++)
        com_put_long(0);

    com_put_msg(NULL, 0);
}


/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: get_user_stack                                             */
/*                                                                           */
/* ACTION:                                                                   */
/*      get_stack returns the current user stack information.                */
/*                                                                           */
/*****************************************************************************/
static void
get_user_stack(const void *p_cmd)
{
    com_init_msg();
    com_put_byte(GET_USER_STACK);
    com_put_byte(OK);

    com_put_long((unsigned int) &user_stack);            /* frame pointer */
    com_put_long((unsigned int) &user_stack + 0x40);    /* sp */
    com_put_msg(NULL, 0);
}


/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: get_mon                                                    */
/*                                                                           */
/* ACTION:                                                                   */
/*      get_mon returns the current monitor information.                     */
/*                                                                           */
/*****************************************************************************/
static void
get_mon(const void *p_cmd)
{
    com_init_msg();
    com_put_byte(GET_MON);
    com_put_byte(OK);

    com_put_byte(arch);
    com_put_byte(mon_version_byte);
    com_put_msg(NULL, 0);
}


/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: set_mon                                                    */
/*                                                                           */
/* ACTION:                                                                   */
/*      set_mon returns the current monitor information.                     */
/*                                                                           */
/*****************************************************************************/
static void
set_mon(const void *p_cmd)
{
    register const unsigned char *rp = (unsigned char *)p_cmd + 2;
    int mon_priority, break_int;
    CMD_TMPL    response;

    /* These values are signed bytes, and need to be sign-extended;
     * hence the cast to signed char.  (Actually, the only possible
     * negative value for each of them is -1.)
     */
    mon_priority = (int)(signed char)get_byte(rp);
    break_int = (int)(signed char)get_byte(rp);

    /* Don't raise the priority; only lower it. */
    if (mon_priority != -1 && get_mon_priority() > mon_priority)
        set_mon_priority(mon_priority);

    if (break_int != -1)
        {
        int break_vector = get_int_vector(break_int);

        set_break_vector(break_vector, NULL);
        }

    response.cmd = SET_MON;
    response.stat = OK;
    PUTBUF(response);
}


/*****************************************************************************
 *
 * FUNCTION NAME: set_hw_bp
 *
 *****************************************************************************/
static void
set_hw_bp(const void *p_cmd)
{
    register const unsigned char *rp = (unsigned char *)p_cmd + 2;
    int type, mode;
    ADDR addr;
        CMD_TMPL response;

    type = get_byte(rp);
    mode = get_byte(rp);
    addr = get_long(rp);

        response.cmd = SET_HW_BP;
    response.stat = OK;

    if (set_bp(type, mode, addr) != OK)
        response.stat = cmd_stat;

    PUTBUF(response);
}

/*****************************************************************************
 *
 * FUNCTION NAME: clr_hw_bp
 *
 *****************************************************************************/
static void
clr_hw_bp(const void *p_cmd)
{
    register const unsigned char *rp = (unsigned char *)p_cmd + 2;
    ADDR addr;
        CMD_TMPL response;

    addr = get_long(rp);

        response.cmd = CLR_HW_BP;
    response.stat = OK;

    if (clr_bp(addr) != OK)
        response.stat = cmd_stat;

    PUTBUF(response);
}


/*****************************************************************************
 *
 * FUNCTION NAME: exec_usr
 *
 * ACTION:
 *      exec_usr sets up the user trace controls and then transfers control
 *      to the user space.
 *
 *****************************************************************************/
static void
exec_usr(const void *p_cmd)
{
    register const unsigned char *rp = (unsigned char *)p_cmd + 2;
    int mode, bp_flag;
    unsigned long bp_instr;
        CMD_TMPL response;

    mode = get_byte(rp);
    bp_flag = get_byte(rp);
    bp_instr = get_long(rp);

        response.cmd = EXEC_USR;

    if (prepare_go_user(mode, bp_flag, bp_instr) != OK)
    {
        response.stat = cmd_stat;
        PUTBUF(response);
        return;
    }

    response.stat = OK;
    PUTBUF(response);

    leds(4, 4);         /* We are running! */
    exit_mon(mode == GO_SHADOW);
    /* exit_mon does not return */
}

/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: target_ver                                                 */
/*                                                                           */
/* ACTION:                                                                   */
/*      target_ver returns the version of the target software.               */
/*                                                                           */
/*****************************************************************************/
static void
target_ver(const void *p_cmd)
{
    unsigned char response[BUFSIZE];
    STRING_TMPL *rp = (STRING_TMPL *)response;

    rp->cmd = TRGT_VER;
    rp->stat = OK;
    strcpy(rp->string, arch_name);
    strcat(rp->string, "; ");
    strcat(rp->string, board_name);
    strcat(rp->string, "\n");
    strcat(rp->string, base_version);
    strcat(rp->string, " ");
    strcat(rp->string, build_date);

    if (step_string != NULL)
    {
        strcat(rp->string, "; ");
        strcat(rp->string, step_string);
    }

#if BIG_ENDIAN_CODE
    strcat(rp->string, " (Big-Endian)");
#endif /* __i960_BIG_ENDIAN__ */

    com_put_msg((const unsigned char *)response, sizeof(STRING_TMPL)+_mon_strlen(rp->string));
}


/*****************************************************************************
 *
 * FUNCTION NAME: check_eeprom_cmd
 *
 *****************************************************************************/
static void
check_eeprom_cmd(const void *p_cmd)
{
    register const unsigned char *rp = (unsigned char *)p_cmd + 2;
    ADDR addr;
    unsigned long length;
    int r;

    addr = get_long(rp);
    length = get_long(rp);

    r = check_eeprom(addr, length);

    com_init_msg();
    com_put_byte(CHECK_EEPROM);

    if (r == OK)
    {
        com_put_byte(OK);
        com_put_long(eeprom_size);
    }
    else if (cmd_stat == E_EEPROM_PROG)
    {
        com_put_byte(cmd_stat);
        com_put_long(eeprom_size);
        com_put_long(eeprom_prog_first);
        com_put_long(eeprom_prog_last);
    }
    else
        com_put_byte(cmd_stat);

    com_put_msg(NULL, 0);
}


/*****************************************************************************
 *
 * FUNCTION NAME: erase_eeprom_cmd
 *
 *****************************************************************************/
static void
erase_eeprom_cmd(const void *p_cmd)
{
    register const unsigned char *rp = (unsigned char *)p_cmd + 2;
    ADDR addr;
    unsigned long length;
        CMD_TMPL response;

    addr = get_long(rp);
    length = get_long(rp);

        response.cmd = ERASE_EEPROM;
    response.stat = OK;

    if (erase_eeprom(addr, length) != OK)
        response.stat = cmd_stat;

    PUTBUF(response);
}


/*****************************************************************************
 *
 * FUNCTION NAME: do_ui_cmd
 *
 *****************************************************************************/
static STRING_TMPL *ui_response = NULL;
static char *ui_ptr = NULL;
extern int hi_ui_cmd;

static void
do_ui_cmd(const void *p_cmd)
{
    char response[BUFSIZE];
    STRING_TMPL *p = (STRING_TMPL *)p_cmd;

    ui_response = (STRING_TMPL *)response;
    ui_response->cmd = UI_CMD;
    ui_ptr = ui_response->string;

	hi_ui_cmd = TRUE;
    if (ui_cmd(p->string, FALSE) == OK)
        ui_response->stat = OK;
    else
        ui_response->stat = cmd_stat;
	hi_ui_cmd = FALSE;
    
    *ui_ptr++ = '\0';
    com_put_msg((const unsigned char *)response, ui_ptr - response);
    ui_response = NULL;
    ui_ptr = NULL;
}



#if CXHXJX_CPU
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: set_mcon_cmd                                               */
/*                                                                           */
/*****************************************************************************/
static void
set_mcon_cmd(const void *p_cmd)
{
    int                          rc;
    unsigned int                 region;
    register const unsigned char *rp = (unsigned char *)p_cmd + 2;
    unsigned long                value;

    region = get_long(rp);
    value  = get_long(rp);
    rc     = set_proc_mcon(region, value);

    com_init_msg();
    com_put_byte(SET_PROC_MCON);
    com_put_byte(rc == OK ? OK : cmd_stat);

    com_put_msg(NULL, 0);
}
#else
static void
set_mcon_cmd(const void *p_cmd)
{
    CMD_TMPL response;

    response.cmd = SET_PROC_MCON;
    response.stat = E_ARCH;
    PUTBUF(response);
}
#endif /* CXHXJX */



/*****************************************************************************
 *
 * FUNCTION NAME: hi_co
 *
 *****************************************************************************/
int
hi_co(int c)
{ 
    char *response = (char *)ui_response;

    if (response == NULL)
        fatal_error(3,(int)"HI_CO",0,0,0);

    if (ui_ptr - response >= BUFSIZE-2)
    {
        ui_response->stat = 0xfe;
        *ui_ptr++ = '\0';
        com_put_msg((const unsigned char *)response, ui_ptr - response);
        ui_ptr = ui_response->string;
    }

    *ui_ptr++ = c;
}

int
_get_short(const unsigned char *p)
{
    return ((p)[1] << 8 | (p)[0]);
}

long
_get_long(const unsigned char *p)
{
    return ((p)[3] << 24 | (p)[2] << 16 | (p)[1] << 8 | (p)[0]);
}

#if BIG_ENDIAN_CODE
/* this function is only used (and hence, compiled) when running on
 * a big-endian target.
 */

/* swap_data_byte_order() swaps the order of data read from target memory
 * (via load_mem) or about to be stored into target memory (via store_mem)
 * because the data stream between mon960 and HDIL (on the host) is defined
 * to be always little-endian byte order.
 */

void
swap_data_byte_order(
    int mem_size,    /* the mem_size to use */
    void *buf_arg,    /* the data which was read from/will be stored to */
    int len)        /* how many bytes of data */
{
   unsigned char *bp = buf_arg;
   unsigned char tmp;

   /* assumption: mem_size must be 0, 1, 2, 4, 8, 12, or 16 */
   switch (mem_size)
   {
   case 0:
   case 1:
      break;
   case 2:
      /* swap the order of the shorts */
      while (len)
      {
         tmp = bp[0];
         bp[0] = bp[1];
         bp[1] = tmp;
         len -= 2;
         bp += 2;
      }
      break;
   case 4:
   case 8:
   case 12:
   case 16:
      /* swap the order of the words */
      while (len)
      {
         tmp = bp[0];
         bp[0] = bp[3];
         bp[3] = tmp;
         tmp = bp[1];
         bp[1] = bp[2];
         bp[2] = tmp;
         len -= 4;
         bp += 4;
      }
      break;
   } /* switch */

   return;
}
#endif /* __i960_BIG_ENDIAN__ */
