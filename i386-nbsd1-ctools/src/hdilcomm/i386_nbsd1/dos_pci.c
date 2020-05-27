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

/*
 * $Header: dos_pci.c,v 1.57 96/01/10 11:46:41 junderx Exp $
 *
 *
 * MODULE
 *     dos_pci.c
 *
 * PURPOSE
 *     A collection of routines to support DOS-based PCI download.
 *     The intent of this module is to be able to support access to
 *     more than one particular card.  Currently, these routines
 *     are compatible with a Cyclone PCI baseboard.
 */

#include <stdio.h>
#include <string.h>

#include "dbg_mon.h"
#include "private.h"
#include "com.h"
#include "dev.h"
#include "dos_pci.h"
#include "pcivndr.h"

extern plx_driver_routines(struct FAST_INTERFACE *);

#ifdef WIN95
#define DOS_PKT_TIMO    20000
#define DOS_ACK_TIMO    10000
#else /* WIN95  (else DOS, or HIGHC) */
#define DOS_PKT_TIMO    1000
#define DOS_ACK_TIMO     500
#endif /* WIN95 (else DOS, or HIGHC) */

static struct FAST_INTERFACE pci_driver;

#ifdef _MSC_VER
#   define INT1A(X)     int1a(&X)
#else   /* __ highc__ */
#   include <dos.h>
#   define INT1A(X)     int86(0x1A, (union REGS *) &X, (union REGS *) &X)
#endif

#define IOSPACE()       (pci.cfg.comm_mode == COM_PCI_IOSPACE)
#define MEMSPACE()      (pci.cfg.comm_mode == COM_PCI_MMAP)

DOS_PCI_INFO pci;     /* Declare storage to track PCI connection. */

typedef struct vendor_anno_struct
{
    int  hit;          /* Boolean, T if vendor HW listed at least once. */
    int  vendor_id;    /* PCI vendor ID                                 */
    int  device_id;    /* PCI device ID                                 */
    char *anno_str;    /* vendor listing annotation string              */
    char *fullname;    /* vendor's fullname                             */
} VENDOR_ANNO;

static VENDOR_ANNO vendor_anno[] = 
{
    { 0, VENDOR_CYCLONE, PCI80960,          "[*]", "Cyclone PCI Evaluation Target" },
    { 0, VENDOR_CYCLONE, CYC_CONTROLLER_JX, "[+]", "Cyclone PCI I/O Controller (Jx)" },
    { 0, VENDOR_CYCLONE, CYC_CONTROLLER_CX, "[%]", "Cyclone PCI I/O Controller (Cx)" },
    { 0, VENDOR_CYCLONE, CYC_CONTROLLER_HX, "[@]", "Cyclone PCI I/O Controller (Hx)" },
};


#if defined(_MSC_VER) && !defined(WIN95)
extern void grab_dos_vectors(void);
extern int init_timer(void);
extern int term_timer(void);
#endif

static int
pci_close(int fd);

/* ----------------------- Local Static Routines -------------------- */

static int 
pci_no_driver()
{
    hdi_cmd_stat = com_stat = E_PCI_HOST_PORT;
    return (ERR);
}

    
/* Central routine to set default PCI search device. */
static void
assign_default_vendor_device(dev)
COM_PCI_CFG *dev;
{
    dev->vendor_id = PCI_DFLT_VNDR;
    dev->device_id = PCI_DFLT_DVC;
}



/* For each vendor previously listed, dump a more descriptive annotation. */
static void
vendor_anno_epilog()
{
    VENDOR_ANNO *ap;
    int         once, i, 
                num_vnd = sizeof(vendor_anno) / sizeof(vendor_anno[0]);

    for (once = i = 0, ap = vendor_anno; i < num_vnd; i++, ap++)
    {
        if (ap->hit)
        {
            char buf[128];

            if (! once)
            {
                /* upspace before displaying annotation(s). */

                once = 1;
                hdi_put_line("\n");
            }
            sprintf(buf, "%s  %s\n", ap->anno_str, ap->fullname);
            hdi_put_line(buf);
        }
    }
    if (once)        /* separate annotations from next command prompt */
        hdi_put_line("\n");
}



/* 
 * Register each _supported_ vendor being listed.   This info will be used
 * later in vendor_anno_epilog().
 */
char *
vendor_anno_lookup(dev)
COM_PCI_CFG *dev;
{
    VENDOR_ANNO *ap;
    int         i, vendor_id, device_id,
                num_vnd = sizeof(vendor_anno) / sizeof(vendor_anno[0]);

    vendor_id = dev->vendor_id;
    device_id = dev->device_id;
    for (i = 0, ap = vendor_anno; i < num_vnd; i++, ap++)
    {
        if (vendor_id == ap->vendor_id && device_id == ap->device_id)
        {
            ap->hit = 1;
            return (ap->anno_str);
        }
    }
    return ("");
}



/* Mark every vendor in the annotation data structure as "not listed". */
static void
vendor_anno_prolog()
{
    VENDOR_ANNO *ap;
    int         i, num_vnd = sizeof(vendor_anno) / sizeof(vendor_anno[0]);

    for (i = 0, ap = vendor_anno; i < num_vnd; i++, ap++)
        ap->hit = FALSE;
}



/*
 * FUNCTION
 *    validate_pci_addr(COM_PCI_CFG *addr)
 *
 *    addr - specifies PCI bus address to check
 *
 * PURPOSE
 *    Given a fully specified PCI bus address (bus#, dev#, fun#), validate
 *    its components.
 *
 * RETURN
 *    OK   -> success.
 *
 *    ERR  -> read failed, com_stat set appropriately.
 */

static int
validate_pci_addr(addr)
COM_PCI_CFG *addr;
{
#if defined(_MSC_VER) || defined(__HIGHC__)
    if (addr->bus < 0 || addr->bus > PCI_MAX_BUS)
    {
        hdi_cmd_stat = com_stat = E_PCI_ADDRESS;
        return (ERR);
    }
    if (addr->dev < 0 || addr->dev > PCI_MAX_DEVICES)
    {
        hdi_cmd_stat = com_stat = E_PCI_ADDRESS;
        return (ERR);
    }
    if (addr->func < 0 || addr->func > PCI_MAX_FUNCS)
    {
        hdi_cmd_stat = com_stat = E_PCI_ADDRESS;
        return (ERR);
    }
    if (addr->func != 0)
    {
        X86REG reg;

        /* 
         * User specified access to a function on a multi-function card.
         * Is the card really multi-function?  (We check here because some
         * buggy, single function cards will accept any function number).
         */
        reg.h.ah = PCI_FUNCTION_ID;
        reg.h.al = READ_CONFIG_BYTE;
        reg.h.bh = (U_BYTE) addr->bus;
        reg.h.bl = MAKE_DEV_FUNC(addr->dev, 0);
        reg.x.di = PCI_CFG_HDRTYPE;
        INT1A(reg);
        if (reg.e.cflag != 0)
        {
            hdi_cmd_stat = com_stat = E_PCI_CFGREAD;
            return (ERR);
        }
        else if ((reg.h.cl & PCI_HDR_MFUNC_MASK) == 0)
        {
            hdi_cmd_stat = com_stat = E_PCI_MULTIFUNC;
            return (ERR);
        }
    }
    return (OK);
#else
    return (ERR);
#endif
}



/* Function to display a title for list_device_config() */
static void
list_device_title()
{
    char buf[128];

    sprintf(buf,
"Bus#   Dev#   Fcn#   VendId   DevId   StsReg   CmdReg   ClsCde   Rev   Hdr\n");
    hdi_put_line(buf);
}


/*
 * FUNCTION
 *    read_pci_cfg_obj(COM_PCI_CFG *pci_addr, 
 *                     int         size,
 *                     int         cfg_reg_num, 
 *                     U_WORD      *cfg_value)
 *
 *    pci_addr     - specifies PCI bus address to access
 *
 *    size         - size of object to read.  Can be one of:
 *
 *                      READ_CONFIG_BYTE READ_CONFIG_WORD READ_CONFIG_DWORD
 *
 *    cfg_reg_num  - PCI configuration space address to read
 *
 *    cfg_value    - value returned from successful read
 *
 * PURPOSE
 *    Given a fully specified PCI bus address (bus#, dev#, fun#), this
 *    routine reads an x86 dword/word/byte register from configuration space.
 *
 * RETURN
 *    OK   -> success.  Returned by reference:  requested object.
 *
 *    ERR  -> read failed, com_stat set appropriately.
 */

int
read_pci_cfg_obj(pci_addr, size, cfg_reg_num, cfg_value)
COM_PCI_CFG *pci_addr;
int         size;
int         cfg_reg_num;
void        *cfg_value; 
{
#if defined(_MSC_VER) || defined(__HIGHC__)
    X86REG   reg;
    U_BYTE   sz;
#if defined __HIGHC__
    U_DWORD  tmp;

    /*
     * If reading a DWORD (long) using HIGHC & Pharlap, then there's a
     * problem here.  The extender chops the high 16 bits from all 32 bit
     * registers when returning from a DOS interrupt.  Consequently, we can
     * only read the DWORD by reading configuration space with
     * (essentially) 2 16-bit accesses and then gluing the results
     * together.
     */
    sz       = (U_BYTE) ((size == READ_CONFIG_DWORD) ? READ_CONFIG_WORD : size);
#else
    sz       = (U_BYTE) size;
#endif

    reg.h.ah = PCI_FUNCTION_ID;
    reg.h.al = sz;
    reg.h.bh = (U_BYTE) pci_addr->bus;
    reg.h.bl = MAKE_DEV_FUNC(pci_addr->dev, pci_addr->func);
    reg.x.di = (U_WORD) cfg_reg_num;
    INT1A(reg);
    if (reg.e.cflag == 0)
    {
#if defined _MSC_VER
        if (size == READ_CONFIG_WORD)
            *(U_WORD *) cfg_value  = reg.x.cx;
        else if (size == READ_CONFIG_DWORD)
            *(U_DWORD *) cfg_value = reg.e.ecx;
        else
            *(U_BYTE *) cfg_value  = reg.h.cl;
        return (OK);
#else
        if (size == READ_CONFIG_WORD)
        {
            *(U_WORD *) cfg_value  = reg.x.cx;
            return (OK);
        }
        else if (size == READ_CONFIG_BYTE)
        {
            *(U_BYTE *) cfg_value  = reg.h.cl;
            return (OK);
        }

        /* See comments above as to why DWORD reads are different. */

        tmp      = reg.x.cx & 0xffff;
        reg.h.ah = PCI_FUNCTION_ID;
        reg.h.al = sz;
        reg.h.bh = (U_BYTE) pci_addr->bus;
        reg.h.bl = MAKE_DEV_FUNC(pci_addr->dev, pci_addr->func);
        reg.x.di = (U_WORD) cfg_reg_num + 2;
        INT1A(reg);
        if (reg.e.cflag == 0)
        {
            *(U_DWORD *) cfg_value = ((U_DWORD) reg.x.cx << 16) | tmp;
            return (OK);
        }
        /* Fall through to error exit */
#endif
    }
#endif   /* HIGHC or MSC */

    hdi_cmd_stat = com_stat = E_PCI_CFGREAD;
    return (ERR);
}



/*
 * FUNCTION
 *    write_pci_cfg_obj(COM_PCI_CFG *pci_addr, 
 *                      int         size,
 *                      int         cfg_reg_num,
 *                      void        *value) 
 *
 *    pci_addr     - specifies PCI bus address to access
 *
 *    size         - size of object to read.  Can be one of:
 *
 *                   WRITE_CONFIG_BYTE WRITE_CONFIG_WORD WRITE_CONFIG_DWORD
 *
 *    cfg_reg_num  - PCI configuration space address to write
 *
 *    value        - pointer to data to write.  Must be of appropriate size.
 *
 * PURPOSE
 *    Given a fully specified PCI bus address (bus#, dev#, fun#), this
 *    routine writes an x86 dword/word/byte to a configuration space register.
 *
 * RETURN
 *    OK   -> success
 *
 *    ERR  -> read failed.
 */

int
write_pci_cfg_obj(pci_addr, size, cfg_reg_num, value)
COM_PCI_CFG *pci_addr;
int         size;
int         cfg_reg_num;
void        *value;
{

/* NB:  The MSC version of this code is UNTESTED! */

#if defined(_MSC_VER) || defined(__HIGHC__)
    X86REG   reg;

    reg.h.ah = PCI_FUNCTION_ID;
    reg.h.al = (U_BYTE) size;
    reg.h.bh = (U_BYTE) pci_addr->bus;
    reg.h.bl = MAKE_DEV_FUNC(pci_addr->dev, pci_addr->func);
    reg.x.di = (U_WORD) cfg_reg_num;
    if (size == WRITE_CONFIG_DWORD)
        reg.e.ecx = *((U_DWORD *) value);
    else if (size == WRITE_CONFIG_WORD)
        reg.e.ecx = *((U_WORD *) value);
    else
        reg.e.ecx = *((U_BYTE *) value);
    INT1A(reg);
    if (reg.e.cflag == 0)
    {
        return (OK);
    }
    /* Else fall through and take error exit. */

#endif  /* MSC or HIGHC */

    hdi_cmd_stat = com_stat = E_PCI_CFGWRITE;
    return (ERR);
}



/*
 * FUNCTION
 *    pci_bus_addr_lookup(COM_PCI_CFG *srch)
 *
 *    srch - PCI bus address to probe
 *
 * PURPOSE
 *    Given a fully specified PCI bus address (bus#, dev#, fun#), this
 *    routine returns the PCI vendor and device ID of the device at that
 *    address (assuming it exists).
 *
 * RETURN
 *    OK  - found what appears to be a legit vendor id.
 *
 *    ERR - either could not read the bus or else got an invalid Vendor ID
 *          back.
 */

static int
pci_bus_addr_lookup(srch)
COM_PCI_CFG *srch;
{
    U_WORD tmp;
    int    ec;

    if ((ec = read_pci_cfg_obj(srch,
                               READ_CONFIG_WORD,
                               PCI_CFG_VNDID,
                               &tmp)) == OK)
    {
        srch->vendor_id = tmp;
        if (tmp == PCI_INVALID_VENDOR)
        {
            hdi_cmd_stat = com_stat = E_PCI_NODVC;
            ec           = ERR;
        }
        else 
        {
            ec = read_pci_cfg_obj(srch,
                                  READ_CONFIG_WORD, 
                                  PCI_CFG_DEVID, 
                                  &tmp);
            srch->device_id = tmp;
        }
    }
    return (ec);
}



/*
 * FUNCTION
 *    list_device_config(COM_PCI_CFG *dev)
 *
 *    dev - specifies PCI bus address to list
 *
 * PURPOSE
 *    Given a fully specified PCI bus address (bus#, dev#, fun#) plus the
 *    device's vendor and device ID, this routine dumps all of the 
 *    "required" PCI configuration info.
 *
 * RETURN
 *    OK  -> config info dumped, no problems
 *
 *    ERR -> could not read config info, applicable error is in com_stat.
 */

static int
list_device_config(dev)
COM_PCI_CFG *dev;
{
    char    *anno, buf[256];
    U_DWORD cls;
    U_WORD  stsreg, cmdreg;
    int     ec;
    U_BYTE  clsu, clsm, clsl, rev, hdr;

    anno = vendor_anno_lookup(dev);
    if ((ec = read_pci_cfg_obj(dev, 
                               READ_CONFIG_WORD, 
                               PCI_CFG_CMDREG, 
                               &cmdreg)) == OK)
    {
        if ((ec = read_pci_cfg_obj(dev, 
                                   READ_CONFIG_WORD, 
                                   PCI_CFG_STSREG, 
                                   &stsreg)) == OK)
        {
            if ((ec = read_pci_cfg_obj(dev, 
                                       READ_CONFIG_BYTE, 
                                       PCI_CFG_HDRTYPE, 
                                       &hdr)) == OK)
            {
                if ((ec = read_pci_cfg_obj(dev, 
                                           READ_CONFIG_BYTE, 
                                           PCI_CFG_CLSU, 
                                           &clsu)) == OK)
                {
                    if ((ec = read_pci_cfg_obj(dev, 
                                               READ_CONFIG_BYTE, 
                                               PCI_CFG_CLSM, 
                                               &clsm)) == OK)
                    {
                        if ((ec = read_pci_cfg_obj(dev, 
                                                   READ_CONFIG_BYTE, 
                                                   PCI_CFG_CLSL, 
                                                   &clsl)) == OK)
                        {
                            if ((ec = read_pci_cfg_obj(dev, 
                                                       READ_CONFIG_BYTE, 
                                                       PCI_CFG_REVID, 
                                                       &rev)) == OK)
                            {
                                cls = (((U_DWORD) clsu) << 16)  | 
                                      (((U_DWORD) clsm) << 8)   |
                                                  clsl;
                                sprintf(buf,
                                  "%4x%7x%7x%9x%8x%9x%9x   %06lx%6x%6x %s\n",
                                       dev->bus,
                                       dev->dev,
                                       dev->func,
                                       dev->vendor_id,
                                       dev->device_id,
                                       stsreg,
                                       cmdreg,
                                       cls,
                                       rev,
                                       hdr,
                                       anno);
                                hdi_put_line(buf);
                            }
                        }
                    }
                }
            }
        }
    }
    return (ec);
}



/*
 * FUNCTION
 *    test_and_dump_device(COM_PCI_CFG dmp, int *dumped_once, int *ec)
 *
 *    dmp         - specifies PCI bus address to dump, must specify bus#,
 *                  dev#, and fun#.
 *
 *    dumped_once - flag that tracks whether or not to display a title
 *                  before dumping a device's config info.
 *
 *    ec          - error code returned by reference.
 *
 * PURPOSE
 *    Given a fully specified PCI bus address (bus#, dev#, fun#),
 *    this routine first looks up vendor and device ID info to check that
 *    a device exists and if it does, dumps all of the "required" PCI 
 *    configuration info.  If the device does not exist, that is not 
 *    considered to be an error.
 *
 * RETURN
 *    Boolean, T -> device found and successfully dumped.
 *             F -> either not a device, or else an unexpected error
 *                  occurred and *ec has been set accordingly.
 */

static int
test_and_dump_device(dmp, dumped_once, ec)
COM_PCI_CFG *dmp;
int         *dumped_once, *ec;
{
    int is_device = FALSE;

    if ((*ec = pci_bus_addr_lookup(dmp)) == OK)
    {
        if (! *dumped_once)
        {
            *dumped_once = TRUE;
            list_device_title();
        }
        is_device = ((*ec = list_device_config(dmp)) == OK);
    }
    else if (dmp->vendor_id == PCI_INVALID_VENDOR)
    {
        /* No device at bus addr.  Not an error. */

        *ec = OK;
    }
    return (is_device);
}



/*
 * FUNCTION
 *    pci_device_lookup(int srch_indx, COM_PCI_CFG *srch)
 *
 *    srch_indx - PCI BIOS search index, as required by the FIND PCI
 *                Device service.  A value of 0 indicates that the BIOS
 *                should search from "the beginning".
 *
 *    srch      - PCI bus information to probe
 *
 * PURPOSE
 *    Given a PCI vendor ID, device ID, this routine returns the absolute
 *    bus address (bus #, device#, func#) of the PCI device (by reference).
 *
 * RETURN
 *    OK  - found a device.  Returned by reference:  the address's
 *          absolute bus address.
 *
 *    ERR - nada.
 */

static int
pci_device_lookup(srch_indx, srch)
int         srch_indx;
COM_PCI_CFG *srch;
{
#if defined(_MSC_VER) || defined(__HIGHC__)
    X86REG   reg;

    reg.h.ah = PCI_FUNCTION_ID;
    reg.h.al = FIND_PCI_DEVICE;
    reg.x.cx = srch->device_id;
    reg.x.dx = srch->vendor_id;
    reg.x.si = (U_WORD) srch_indx;
    INT1A(reg);
    if (reg.e.cflag == 0)
    {
        srch->bus  = reg.h.bh;
        srch->dev  = EXTRACT_DEV(reg.h.bl);
        srch->func = EXTRACT_FUNC(reg.h.bl);
        return (OK);
    }
    /* else fall through and take the error exit */
#endif

    hdi_cmd_stat = com_stat = E_PCI_SRCH_FAIL;
    return (ERR);
}



/*
 * FUNCTION
 *    com_pcibios_present(void)
 *
 * PURPOSE
 *    Determine whether or not PCI BIOS is present.  Can't execute any PCI 
 *    ops if it's not.
 *
 * RETURN
 *    Boolean, T -> BIOS is present.
 *             F -> No BIOS, com_stat set to the applicable error code.
 */

int EXPORT
com_pcibios_present()
{
#if defined(_MSC_VER) || defined(__HIGHC__)
    X86REG reg;

    reg.h.ah = PCI_FUNCTION_ID;
    reg.h.al = PCI_BIOS_PRESENT;

    INT1A(reg);

    if ((reg.e.cflag == 0) && (reg.h.ah == 0) && 
#   ifdef _MSC_VER
                                                 (reg.e.edx == PCI_BIOS_TEST))
#   else

    /*
     * Refer to the description of PCI_BIOS_TEST in dos_pci.h for an
     * explanation of why the PCI_BIOS_TEST's differ.
     */
                                                 (reg.x.dx == PCI_BIOS_TEST))
#   endif
    {
        return (TRUE);
    }   
    /* else fall through and take the error condition. */

#endif
    hdi_cmd_stat = com_stat = E_NO_PCIBIOS;
    return (FALSE);
}



/*
 * FUNCTION
 *    com_list_pci_bus(int bus_number)
 *
 *    bus_number - PCI bus number for which devices should be listed.
 *                 If the magic value "-1" is specified as a bus number,
 *                 all devices on all possible PCI buses are listed.
 *
 * PURPOSE
 *    List all "required" PCI configuration information for all devices
 *    found on the specified bus.
 *
 * RETURN
 *    COM_PCI_LIST_OK   -> listed at least one device
 *    COM_PCI_LIST_NONE -> listed no devices
 *    COM_PCI_LIST_ERR  -> unexpected error aborted listing
 */

int EXPORT
com_list_pci_bus(bus_number)
int bus_number;
{
    COM_PCI_CFG dmp;
    int         begin_bus, end_bus, ec, hit;
    U_BYTE      hdr;

    hit = 0;
    if (! com_pcibios_present())
        return (COM_PCI_LIST_ERR);

    vendor_anno_prolog();

    if (bus_number != -1)
    {
        /* Validate user's bus number */

        dmp.bus = bus_number;
        dmp.dev = dmp.func = 0;
        if (validate_pci_addr(&dmp) != OK)
            return (COM_PCI_LIST_ERR);
        begin_bus = end_bus = bus_number;
    }
    else
    {
        begin_bus = 0;
        end_bus   = PCI_MAX_BUS;
    }

    for (dmp.bus = begin_bus; dmp.bus <= end_bus; dmp.bus++)
    {
        /* The following set of nested loops were "borrowed" from PCI SIG. */

        for(dmp.dev = 0; dmp.dev <= PCI_MAX_DEVICES; dmp.dev++)
        {
            dmp.func = 0;
            if (test_and_dump_device(&dmp, &hit, &ec))
            {
                /*
                 * Is a device.  Any more functions (other than 0) for 
                 * this device?  The header will tell us if so.
                 */

                if (read_pci_cfg_obj(&dmp, 
                                     READ_CONFIG_BYTE, 
                                     PCI_CFG_HDRTYPE, 
                                     &hdr) != OK)
                {
                    return (COM_PCI_LIST_ERR);
                }
                if (hdr & PCI_HDR_MFUNC_MASK)
                {
                    /* Search for other functions */

                    for (dmp.func = 1; dmp.func <= PCI_MAX_FUNCS; dmp.func++)
                    {
                        int dont_care_ec;

                        /*
                         * We could test for unexpected bus read errors for 
                         * every bus probe, here.  But that's overkill for a
                         * simple diagnostic dump routine.
                         */
                        (void) test_and_dump_device(&dmp, &hit, &dont_care_ec);
                    }
                }
            }
            else
            {
                if (ec != OK)
                {
                    /* unexpected error during listing, bag it */

                    return (COM_PCI_LIST_ERR);
                }
                /* Else, not a device, continue. */
            }
        }
    }
    vendor_anno_epilog();
    return ((hit) ? COM_PCI_LIST_OK : COM_PCI_LIST_NONE);
}


/*
 * FUNCTION
 *    com_find_pci_devices(int vendor_id, int device_id)
 *
 *    vendor_id - PCI vendor ID of desired device
 *
 *    device_id - PCI device ID of desired device
 *
 * PURPOSE
 *    Dump all "required" PCI configuration information for the
 *    selected device.
 *
 * RETURN
 *    COM_PCI_FIND_OK   -> found at least one device
 *    COM_PCI_FIND_NONE -> found no devices
 *    COM_PCI_FIND_ERR  -> unexpected error aborted listing, com_stat
 *                         set as a side effect.
 */

int EXPORT
com_find_pci_devices(vendor_id, device_id)
int vendor_id, device_id;
{
    COM_PCI_CFG lst;
    int         ec, hit, srch_indx;

    hit = srch_indx = 0;
    if (! com_pcibios_present())
        return (COM_PCI_FIND_ERR);
    vendor_anno_prolog();
    if (vendor_id == COM_PCI_DFLT_VENDOR)
        assign_default_vendor_device(&lst);
    else
    {
        lst.vendor_id = vendor_id;
        lst.device_id = device_id;
    }
    do
    {
        if ((ec = pci_device_lookup(srch_indx++, &lst)) == OK)
        {
            if (! hit)
            {
                hit = TRUE;
                list_device_title();
            }
            if ((ec = list_device_config(&lst)) != OK)
                return (COM_PCI_FIND_ERR);
        }
    }
    while (ec == OK);

    vendor_anno_epilog();

    return ((hit) ? COM_PCI_FIND_OK : COM_PCI_FIND_NONE);
}



/*
 * FUNCTION
 *    com_pci_init(COM_PCI_CFG *cfg)
 *
 *    cfg - relevant info necessary to initialize PCI download.
 *
 * PURPOSE
 *    Configure PCI to effect PCI download.
 *
 * RETURN
 *    Boolean, T -> configuration was successful.  When configuration is
 *    successful, the cfg structure is initialized with the bus address
 *    _and_ PCI vendor/device ID of the target download device.
 */

int EXPORT
com_pci_init(cfg)
COM_PCI_CFG *cfg;
{
    int ec, initialized, srch_indx = 0;
    
     /* if using pci comm don't init again for download*/
    if (_com_config.dev_type == HDI_PCI_COMM)
       {
       *cfg = pci.cfg;
       return OK;
       }

    if (! com_pcibios_present())
        return (ERR);

#if defined(__HIGHC__)
    pci.real_mode = FALSE;       /* Assume PHARLAP is active */
#elif defined(_MSC_VER)
    pci.real_mode = TRUE;        /* DOS only */
#else
    pci.real_mode = TRUE;        /* WIN95 uses real mode to simplify */
#endif

    if (cfg->vendor_id == COM_PCI_DFLT_VENDOR)
        assign_default_vendor_device(cfg);

    pci.cfg   = *cfg;
    pci.iolcl = pci.ioreg = 0x0;
    memset(&pci.mem, 0, sizeof(pci.mem));
    
    if (pci.real_mode && MEMSPACE())
    {
        /* No can do -- insufficient horsepower to map physical memory. */

        hdi_cmd_stat = com_stat = E_PHYS_MEM_MAP;
        return (ERR);
    }

    /*
     * Attempt to select a set of interface routines based upon user
     * specifications.  The first thing to do is fetch enough config
     * info from the PCI BIOS to specify both the bus address and
     * vendor/device ID of the target card (device).
     */
    if (pci.cfg.bus != COM_PCI_NO_BUS_ADDR)
    {
        /* 
         * User specified an absolute PCI bus address.  Go get the config 
         * info for device at this address following validation.
         */

        if (validate_pci_addr(&pci.cfg) != OK)
            return (ERR);
        pci.initial_bios_lookup = PCI_ABSOLUTE_ADDR;
        if ((ec = pci_bus_addr_lookup(&pci.cfg)) != OK)
            return (ec);
    }
    else
    {
        /*
         * All we have is a vendor and device ID.  We need to find the
         * bus address of the first card matching the user's request.
         */

        pci.initial_bios_lookup = PCI_FIND_NEXT;
        if ((ec = pci_device_lookup(srch_indx, &pci.cfg)) != OK)
            return (ec);
    }

    /* Set to pci_no_driver so pci_driver->init returns the 
     * appropiate error if no drivers are set.
     */
        pci_driver.init        = pci_no_driver;
        pci_driver.connect     = pci_no_driver;
        pci_driver.disconnect  = (FUNC_VOID_PTR)pci_no_driver;
        pci_driver.err         = (FUNC_VOID_PTR)pci_no_driver;
        pci_driver.get         = pci_no_driver;
        pci_driver.put         = pci_no_driver;
        pci_driver.direct_put  = pci_no_driver;
        pci_driver.intr_trgt   = pci_no_driver;

    /* Map devices to interface routines. */
    if (pci.cfg.vendor_id == VENDOR_CYCLONE   && 
        SUPPORTED_CYCLONE_DVCID(pci.cfg.device_id))
        {
        /* HERE is where we connect cyclone PLX code to pci download */
        plx_driver_routines(&pci_driver);
        }

    initialized  = FALSE;
    hdi_cmd_stat = com_stat = OK;
    do
    {
        if ((ec = pci_driver.init()) == OK)
        {
            /* 
             * Device initialized.  Init the cfg structure passed to us 
             * so that clients know what they're talking to.
             */

            *cfg        = pci.cfg;
            initialized = TRUE;
        }
        else if (pci.initial_bios_lookup == PCI_FIND_NEXT && 
                                            com_stat == E_CONTROLLING_PORT)
        {
            if ((ec = pci_device_lookup(++srch_indx, &pci.cfg)) != OK)
            {
                /* 
                 * Need to warp error code here to say...
                 * "bus search can't locate target attached to controlling
                 * communication port".  This is either an internal error
                 * or else the user is attempting to download to a target
                 * in a completely different environment (e.g., a target
                 * in a different PC card cage :-) ).
                 */

                hdi_cmd_stat = com_stat = E_PORT_SEARCH;
            }
        }
    }
    while (! initialized && ec == OK);

    return (ec);
}




/* ------------------------------------------------------------------ */
/* -------------- Routines to terminate PCI download ---------------- */
/* ------------------------------------------------------------------ */

/*
 * FUNCTION
 *    pci_generic_end(void)
 *
 * PURPOSE
 *    Terminate PCI download.  This is fairly trivial -- just dealloc
 *    allocated resources.  Will probably work for most PCI cards in a
 *    PC environment.
 *
 * RETURN
 *    Boolean, T -> all is well.
 */

int
pci_generic_end()
{
     /* if using pci comm don't init again for download*/
    if (_com_config.dev_type == HDI_PCI_COMM)
       return OK;

    if (MEMSPACE())
    {
        /* Must deallocate mapped physical pages. */

#if defined(__HIGHC__)
        union REGS   cmd, result;
        struct SREGS segs;

        segread(&segs);
        cmd.h.ah = 0x49;          /* Free segment. */
        if (pci.mem.reg.lp)
        {
            segs.es = pci.mem.reg_sel;
            intdosx(&cmd, &result, &segs);
            if (result.x.cflag != 0)
            {
                hdi_cmd_stat = com_stat = E_PHYS_MEM_FREE;
                return (ERR);
            }
        }
        if (pci.mem.lcl.lp)
        {
            segs.es = pci.mem.lcl_sel;
            intdosx(&cmd, &result, &segs);
            if (result.x.cflag != 0)
            {
                hdi_cmd_stat = com_stat = E_PHYS_MEM_FREE;
                return (ERR);
            }
        }
        return (OK);
#endif
    }
    /* Else, all transactions were done in I/O space...nothing to do. */

    return (OK);
}


/* ------------------------------------------------------------------ */
/* -------------- Routines for PCI Download---------------- */
/* ------------------------------------------------------------------ */
int EXPORT
com_pci_end()
{
    return pci_generic_end();
}


int EXPORT
com_pci_put(data, size, crc)
unsigned char  *data;
unsigned long  size;
unsigned short *crc;
{
    return pci_driver.put(data, size, crc);
}


int EXPORT
com_pci_direct_put(addr, data, size)
unsigned long  addr;
unsigned char  *data;
unsigned long  size;
{
    return pci_driver.direct_put(addr, data, size);
}


/* ------------------------------------------------------------------ */
/* -------------- Routines to start PCI comm ---------------- */
/* ------------------------------------------------------------------ */

/*
 * FUNCTION
 *    pci_open(void)
 *
 * PURPOSE
 *
 * Open the pci device passed in as a parameter and prepare it 
 * for IO. Returns OK or -1 on error (unable to initialize requested device).
 *
 *
 * RETURN
 *    Boolean, T -> all is well.
 */

static int
pci_open()
{
    unsigned int ec;

    _com_config.dev_type = NO_COMM;
    *(unsigned long *)_com_config.pci.control_port = 0xffffffffL;
    if ((ec=com_pci_init(&_com_config.pci)) != OK)
        return ec;

    _com_config.dev_type = HDI_PCI_COMM;

    if (_com_config.host_pkt_timo == 0)
        _com_config.host_pkt_timo = DOS_PKT_TIMO;

    if (_com_config.ack_timo == 0)
        _com_config.ack_timo = DOS_ACK_TIMO;

#if defined(_MSC_VER) && !defined(WIN95)
    /*
     * We must be ready to remove our interrupt routines on
     * a forced termination (i.e., Ctrl-Brk or critical error).
     * So we must patch these vectors BEFORE the serial & timer.
     */
    grab_dos_vectors();

    if (init_timer() != OK)
        return(ERR);
#endif
        
    if ( pci_driver.connect() != OK )
    {
	pci_close(0);
	return(ERR);
    }
    else
	return OK;
}


/* ------------------------------------------------------------------ */
/* -------------- Routines to terminate PCI COMM ---------------- */
/* ------------------------------------------------------------------ */

/*
 * FUNCTION
 *    pci_close(int fd)
 *
 * PURPOSE
 *
 * RETURN
 *    Boolean, T -> all is well.
 */

static int
pci_close(int fd)
{
    int ec;

    pci_driver.disconnect();

#if defined(_MSC_VER) && !defined(WIN95)
    term_timer();
#endif    
    
    if ((ec=pci_generic_end()) != OK)
        return ec;

    return OK;
}


/* ------------------------------------------------------------------ */
/* -------------- Routines to read a PCI message ---------------- */
/* ------------------------------------------------------------------ */

/*
 * FUNCTION
 *    pci_read(unsigned char * data, int length, int timeout)
 *
 * PURPOSE
 *
 * RETURN
 *    Boolean, T -> all is well.
 */

static int
pci_read(int fd, unsigned char * data, int length, int timeout)
{
    int ec;
    
    if ((ec=pci_driver.get(data, (long)length, NULL, timeout)) != OK)
        return ec;
    
    return length;
}


/* ------------------------------------------------------------------ */
/* -------------- Routines to write a PCI message ---------------- */
/* ------------------------------------------------------------------ */

/*
 * FUNCTION
 *    pci_write(const unsigned char * data, int length)
 *
 * PURPOSE
 *
 * RETURN
 *    Boolean, T -> all is well.
 */

static int
pci_write(int fd, const unsigned char * data, int length)
{
    int ec;
    
    if ((ec=pci_driver.put(data, (long)length, NULL)) != OK)
        return ec;
    
    return OK;
}


/* ------------------------------------------------------------------ */
/* -------------- Routines to interrupt a PCI target ---------------- */
/* ------------------------------------------------------------------ */

/*
 * FUNCTION
 *    pci_intr_trgt(int fd)
 *
 * PURPOSE
 *
 * RETURN
 *    Boolean, T -> all is well.
 */

static int
pci_intr_trgt(int fd)
{
    int ec;
    
    if ((ec=pci_driver.intr_trgt()) != OK)
        return ec;
    
    return OK;
}


/* ------------------------------------------------------------------ */
/* -------------- Routines to interrupt a PCI ttransfer ---------------- */
/* ------------------------------------------------------------------ */

/*
 * FUNCTION
 *    pci_signal(int fd)
 *
 * PURPOSE
 *
 * RETURN
 *    Boolean, T -> all is well.
 */
extern int pci_intr_flag;

static void
pci_signal(int fd)
{
   pci_intr_flag = TRUE; 
   com_stat = E_INTR;
}


/* ------------------------------------------------------------------ */
/* ---------- Global Common PCI Routines (PC Environment) ----------- */
/* ------------------------------------------------------------------ */

static const DEV pci_dev =
    { pci_open, pci_close, pci_read, pci_write,
      pci_signal, pci_intr_trgt };

const DEV *
com_pci_dev()
{
    return(&pci_dev);
}
