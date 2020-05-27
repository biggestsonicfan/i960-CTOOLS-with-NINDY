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
 * Interface to communication system
 * $Header: /tmp_mnt/ffs/p1/dev/src/hdil/common/RCS/hdi_com.h,v 1.27 1995/11/27 20:07:42 cmorgan Exp $$Locker:  $
 */

#ifndef HDI_COM_H
#define HDI_COM_H

#ifdef WINDOWS
#include <windows.h>
#define EXPORT WINAPI
#else /* WINDOWS */
#define EXPORT
#endif /* WINDOWS */

#define MAX_MSG_SIZE 1000

#ifndef HDI_PARAMS
#   ifdef __STDC__
#       define HDI_PARAMS(paramlist)   paramlist
#   else
#       define HDI_PARAMS(paramlist)   ()
#   endif
#endif

/*
 * The following constant is used allocate a dynamic buffer for program
 * download.  It is currently set at a value that makes sense for COFF
 * programs (no COFF section can exceed 64K).  This buffer constant is also
 * used to compute parallel download timeouts for really slow Unix
 * hosts (e.g., AIX 3.2).
 */
#define MAX_DNLOAD_BUF   ((unsigned long) 64 * 1024)  

#define COM_POLL         (-1)
#define COM_WAIT_FOREVER 0
#define COM_WAIT         1


#ifdef HOST
    /*
     * Define return codes for com_find_pci_devices() and
     * com_list_pci_bus() .
     */
#   define COM_PCI_FIND_OK     0   /* found at least one device.    */
#   define COM_PCI_FIND_NONE   1   /* found no devices.             */
#   define COM_PCI_FIND_ERR    2   /* unexpected error aborted find */

#   define COM_PCI_LIST_OK     0   /* listed at least one device.      */
#   define COM_PCI_LIST_NONE   1   /* listed no devices.               */
#   define COM_PCI_LIST_ERR    2   /* unexpected error aborted listing */

    /*
     * Define some data structures and constants to support PCI download and
     * PCI comm.
     */
#   define COM_PCI_MMAP         0  /* PCI mailboxes addressed via memory-mapped 
                                    * accesses.
                                    */
#   define COM_PCI_IOSPACE      1  /* PCI mailboxes addressed via I/O space. */
#   define COM_PCI_NO_BUS_ADDR  (-1)
                                   /* Magic value to indicate that user is
                                    * _not_ selecting a card based on bus
                                    * address, but instead wishes to select
                                    * based upon vendor and device ID.
                                    */
#   define COM_PCI_DFLT_FUNC    0
#   define COM_PCI_DFLT_VENDOR  (-1) 
                                   /* PCI Vendor ID to use to select a
                                    * Cyclone PCI baseboard.  We can use
                                    * a default like this for the time
                                    * being because Cyclone is the only
                                    * 960 PCI game in town.
                                    */
#   define COM_PCI_SRL_PORT_SZ  5

    typedef struct 
    {
        int  comm_mode;         /* Either COM_PCI_MMAP or COM_PCI_IOSPACE. */
        int  bus;               /* PCI bus# of the card the user wants to
                                 * select for PCI ops.  If this field is
                                 * initialized to COM_PCI_NO_BUS_ADDR, hdil 
                                 * will interrogate the bus and select the
                                 * next unused PCI card based upon the
                                 * specified vendor and device id.  IN ALL
                                 * CASES, this field must be initialized to
                                 * either COM_PCI_NO_BUS_ADDR or a user-
                                 * selected bus#.
                                 */
        int  dev;               /* PCI device# of the user-selected card.
                                 * Irrelevant if "bus" specifies 
                                 * COM_PCI_NO_BUS_ADDR.
                                 */
        int  func;              /* PCI function# of the user-selected card.
                                 * Irrelevant if "bus" specifies 
                                 * COM_PCI_NO_BUS_ADDR.
                                 */
        int  vendor_id;         /* PCI vendor ID of the target card.  Can
                                 * be specified as COM_PCI_DFLT_VENDOR if
                                 * the client wishes to communicate with a
                                 * Cylcone PCI baseboard.  Irrelevant if
                                 * "bus" is initialized to some value other
                                 * than COM_PCI_NO_BUS_ADDR.
                                 */
        int  device_id;         /* PCI device ID of the target card.  This 
                                 * field is irrelevant if vendor_id above
                                 * is specified as COM_PCI_DFLT_VENDOR or
                                 * if vendor_id is itself irrelevant.
                                 */
        char control_port[COM_PCI_SRL_PORT_SZ];
                                /* When the PCI is used only for download
                                 * operations, this field specifies the
                                 * last 4 bytes of the name of the 
                                 * controlling serial port (e.g., "COM1"
                                 * or "bpp0").
                                 */
        /*
         * Note:  This data structure can be confusing.  To properly
         * select a PCI card, clients may use one of two approaches:
         *
         * a) specify an absolute bus address using bus#, device#, and
         *    function#.  In this situation, vendor_id and device_id are
         *    irrelevant, or
         *
         * b) specify a vendor and device ID.  In this situation, bus# must
         *    be set to COM_PCI_NO_BUS_ADDR and device# and function# are
         *    irrelevant.
         *
         * Initialization scenario pseudo code:
         *
         *     init comm_mode as appropriate
         *     init control_port as appropriate
         *     init func to COM_PCI_DFLT_FUNC
         *     init bus to COM_PCI_NO_BUS_ADDR
         *     examine user-specified PCI address selections
         *     if user specifies a bus#, device#, and optional func#
         *         use these values to specify a PCI address
         *         init done
         *     else if user specifies a vendor & device id
         *         use these values to specify a PCI address
         *         init done
         *     else user specified no address info so
         *         init vendor_id to COM_PCI_DFLT_VENDOR
         *         init done
         *     fi
         */
    } COM_PCI_CFG;  
#endif    /* HOST */


#ifdef HOST
typedef struct {
    char *name;               /* name of debugger option */
    unsigned char *opt_set;   /* pointer to value for option (TRUE/FALSE) */
    unsigned char needs_arg;  /* true if this option requires an argument */
    char *argument;           /* pointer to argument for this option */
} COM_INVOPT;

#ifdef __STDC__
typedef int (*opt_handler_t)(const char *);
#else
typedef int (*opt_handler_t)();
#endif /* __STDC__ */
#endif /* HOST */

/* ----------- External comm-based function declarations ----------- */

#ifdef HOST
extern int  com_commopt_ack_timo    HDI_PARAMS((const char *, const char *));
extern int  com_commopt_host_timo   HDI_PARAMS((const char *, const char *));
extern int  com_commopt_max_pktlen  HDI_PARAMS((const char *, const char *));
extern int  com_commopt_max_retry   HDI_PARAMS((const char *, const char *));
extern int  com_commopt_target_timo HDI_PARAMS((const char *, const char *));
extern int  com_pci_comm            HDI_PARAMS((void));
#   ifdef MSDOS
extern void com_get_pci_cfg         HDI_PARAMS((COM_PCI_CFG *));
extern void com_pci_debug           HDI_PARAMS((int));
extern void com_pciopt_cfg          HDI_PARAMS((COM_PCI_CFG *));
extern void com_select_pci_comm     HDI_PARAMS((void));
#   endif
extern void com_select_serial_comm  HDI_PARAMS((void));
extern int  com_serial_comm         HDI_PARAMS((void));
extern int  com_seropt_baud         HDI_PARAMS((const char *, const char *));
#   ifdef MSDOS
extern int  com_seropt_freq         HDI_PARAMS((const char *, const char *));
#   endif
extern int  com_seropt_port         HDI_PARAMS((const char *, const char *));
#endif


#ifdef __STDC__
extern int EXPORT com_init(void);
extern int EXPORT com_reset(int reset_time);
extern void EXPORT com_signal(void);
extern int EXPORT com_intr_trgt(void);
extern int EXPORT com_term(void);
extern const unsigned char * EXPORT com_get_msg(int *szp, int wait);
extern int EXPORT com_put_msg(const unsigned char *msg, int sz);
extern const unsigned char *EXPORT com_send_cmd(const unsigned char *msg,
					 int *szp, int wait);
extern int EXPORT com_mode(int cmd, unsigned long arg);
extern void EXPORT com_init_msg(void);
extern int EXPORT com_put_byte(int);
extern int EXPORT com_put_short(int);
extern int EXPORT com_put_long(unsigned long);
extern int EXPORT com_put_data(const unsigned char *, int);
extern int EXPORT com_get_stat(void);
extern int EXPORT com_get_target_reset(void);
extern int EXPORT com_put_msg_once(const unsigned char *msg, int sz);
extern int EXPORT com_parallel_init(char * port);
extern int EXPORT com_parallel_end(void);
extern int EXPORT com_parallel_put(unsigned char *data,
								   unsigned int size, unsigned short * crc);
#ifdef HOST
extern int EXPORT com_pcibios_present(void);
extern int EXPORT com_find_pci_devices(int vendor_id, int device_id);
extern int EXPORT com_list_pci_bus(int bus_number);
extern int EXPORT com_pci_init(COM_PCI_CFG *);
extern int EXPORT com_pci_end(void);
extern int EXPORT com_pci_put(unsigned char *data,
                              unsigned long size, unsigned short *crc);
extern int EXPORT com_pci_direct_put(unsigned long addr, unsigned char *data,
									 unsigned long size);
extern void EXPORT com_get_pci_controlling_port(char *port_buffer);

#ifdef WINDOWS
extern void EXPORT com_setup(HANDLE instance, HWND parent, FARPROC *pglptrs,
                             VOID *top_window);
extern int EXPORT com_version(char *version, int length);
extern void EXPORT com_set_eval(void);
extern short EXPORT com_get_PrivOptList(const COM_INVOPT **optPP,
                                        const opt_handler_t **optHandlerPP);
extern int EXPORT com_get_Parallel(char __far * parallel_device);
extern void EXPORT  com_set_ParalCapable(const short status);
#endif /* WINDOWS */

extern short EXPORT com_get_opt_list(const COM_INVOPT **optPtr,
                                     const opt_handler_t **optHandler);
extern int EXPORT com_error(void);
extern int EXPORT com_config(void);

#endif /* HOST */

#else /* __STDC__ */

extern int EXPORT com_init();
extern int EXPORT com_reset();
extern void EXPORT com_signal();
extern int EXPORT com_intr_trgt();
extern int EXPORT com_term();
extern const unsigned char * EXPORT com_get_msg();
extern int EXPORT com_put_msg();
extern const unsigned char * EXPORT com_send_cmd();
extern int EXPORT com_mode();
extern void EXPORT com_init_msg();
extern int EXPORT com_put_byte();
extern int EXPORT com_put_short();
extern int EXPORT com_put_long();
extern int EXPORT com_put_data();
extern int EXPORT com_get_stat();
extern int EXPORT com_get_target_reset();
extern int EXPORT com_put_msg_once();
extern int EXPORT com_parallel_init();
extern int EXPORT com_parallel_end();
extern int EXPORT com_parallel_put();
#ifdef HOST
extern int EXPORT com_pcibios_present();
extern int EXPORT com_find_pci_devices();
extern int EXPORT com_list_pci_bus();
extern int EXPORT com_pci_direct_put_capable();
extern int EXPORT com_pci_direct_put();
extern int EXPORT com_pci_init();
extern int EXPORT com_pci_end();
extern int EXPORT com_pci_put();
extern void EXPORT com_get_pci_controlling_port();

#ifdef WINDOWS
extern void EXPORT com_setup();
extern int EXPORT com_version();
extern void EXPORT com_set_eval();
extern short EXPORT com_get_PrivOptList();
extern int EXPORT com_get_Parallel();
extern void EXPORT  com_set_ParalCapable();
#endif /* WINDOWS */

extern short EXPORT com_get_opt_list();
extern int EXPORT com_error();
extern int EXPORT com_config();
#endif /* HOST */

#endif /* __STDC__ */

#endif /* HDI_COM_H */
