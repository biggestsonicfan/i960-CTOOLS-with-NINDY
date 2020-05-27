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
 * $Header: dos_pci.h,v 1.14 96/01/09 13:10:06 junderx Exp $
 *
 * MODULE
 *   dos_pci.h
 *
 * PURPOSE
 *   General include file for DOS PCI operations.
 * 
 */

#ifndef DOS_PCI_H
#define DOS_PCI_H

#include <hdil.h>

typedef int (*FUNC_INT_PTR)();
typedef void (*FUNC_VOID_PTR)();
struct FAST_INTERFACE
       {
       FUNC_INT_PTR  init;
       FUNC_VOID_PTR err;
       FUNC_INT_PTR  connect;
       FUNC_VOID_PTR disconnect;
       FUNC_INT_PTR  intr_trgt;
       FUNC_INT_PTR  get;
       FUNC_INT_PTR  put;
       FUNC_INT_PTR  direct_put;
       } ;

#if defined(__HIGHC__)
    /*
     * In the pharlap/highc build environment, the extender restores the
     * the high-order 16 bits of all 32 bit registers following an x86
     * INT instruction.  This wipes out any 32-bit return value (duh)
     * and necessitates that we slightly modify the test signature used
     * to check for the presence of a PCI BIOS.
     *
     * Yes, we could workaround this problem by configuring our pharlap-based
     * exes with the -NOSAVEREGS switch.  However, if we do that we run into
     * unstable behavior when the exes are run under a NOS (i.e., Novell).
     */
#   define PCI_BIOS_TEST                  0x4350      /* "PC" */
#else
#   define PCI_BIOS_TEST                  0x20494350  /* "PCI " */
#endif


#define PCI_ABSOLUTE_ADDR                 0
#define PCI_FIND_NEXT                     1
#define PCI_HDR_MFUNC_MASK                0x80      /* Is device multi func? */
#define PCI_INVALID_VENDOR                0xffff
#define PCI_MAX_BUS                       0xff
#define PCI_MAX_DEVICES                   0x1f
#define PCI_MAX_FUNCS                     0x7

#define PCI_DVC_IN()                      0x1f
#define PCI_FUNC_IN()                     0x7

/* PCI BIOS Commands */
#define PCI_FUNCTION_ID                   0xb1
#define PCI_BIOS_PRESENT                  0x1
#define FIND_PCI_DEVICE                   0x2
#define READ_CONFIG_BYTE                  0x8
#define READ_CONFIG_WORD                  0x9
#define READ_CONFIG_DWORD                 0xA
#define WRITE_CONFIG_BYTE                 0xB
#define WRITE_CONFIG_WORD                 0xC
#define WRITE_CONFIG_DWORD                0xD

/* PCI BIOS Status */
#define SUCCESSFUL                        0x0

/* Macros to manipulate device & function returned by (passed to) PCI BIOS */
#define MAKE_DEV_FUNC(dev, func)          (U_BYTE) ((dev << 3) | (func & 0x7))
#define EXTRACT_DEV(dev_func)             ((dev_func >> 3) & 0x1f)
#define EXTRACT_FUNC(dev_func)            (dev_func & 0x7)

/* Universal PCI configuration register addresses */
#define PCI_CFG_CLSL                      0x9    /* Class lower byte  */
#define PCI_CFG_CLSM                      0xA    /* Class middle byte */
#define PCI_CFG_CLSU                      0xB    /* Class upper byte  */
#define PCI_CFG_CMDREG                    0x4
#define PCI_CFG_STSREG                    0x6
#define PCI_CFG_DEVID                     0x2
#define PCI_CFG_HDRTYPE                   0xE
#define PCI_CFG_REVID                     0x8
#define PCI_CFG_VNDID                     0x0


typedef unsigned char      U_BYTE ;
typedef unsigned short     U_WORD ;
typedef unsigned long      U_DWORD ;

/* 
 * Define data structures to support 32-bit registers accesses in either
 * a protected, 32-bit environment, or in a 16-bit, real mode.  The defns 
 * made below will work in both a MSC7 & IGHC environment, but may need to 
 * be tweeked for other compilers.
 */

struct DWordRegs
{
    U_DWORD   eax   ;
    U_DWORD   ebx   ;
    U_DWORD   ecx   ;
    U_DWORD   edx   ;
    U_DWORD   esi   ;
    U_DWORD   edi   ;
    U_DWORD   cflag ;
} ;

struct WordRegs
{
    U_WORD    ax     ;
    U_WORD    empty2 ;
    U_WORD    bx     ;
    U_WORD    empty3 ;
    U_WORD    cx     ;
    U_WORD    empty4 ;
    U_WORD    dx     ; 
    U_WORD    empty5 ;
    U_WORD    si     ;
    U_WORD    empty6 ;
    U_WORD    di     ;
    U_WORD    empty7 ;
    U_WORD    cflag  ;
    U_WORD    empty8 ;
} ;

struct ByteRegs
{
    U_BYTE    al, ah ;
    U_WORD    empty1 ;
    U_BYTE    bl, bh ;
    U_WORD    empty2 ;
    U_BYTE    cl, ch ;
    U_WORD    empty3 ;
    U_BYTE    dl, dh ;
    U_WORD    empty4 ;
    U_WORD    si     ;
    U_WORD    empty5 ;
    U_WORD    di     ;
    U_WORD    empty6 ;
    U_WORD    cflag  ;
    U_WORD    empty7 ;
} ;

union x86Regs
{
    struct DWordRegs  e ;
    struct WordRegs   x ;
    struct ByteRegs   h ;
} ;
typedef union    x86Regs   X86REG ;


typedef unsigned short DOS_SELECTOR;
typedef unsigned short DOS_IOADDR;

#ifdef	__HIGHC__	/* Metaware */

#define	FAR _Far
#elif	defined(WIN95)

#define FAR /* empty */
#else			/* Other DOS compiler (Microsoft and clones) */

#define FAR __far
#endif

/*
#if defined(__HIGHC__)
*/
    typedef struct
    {
        U_DWORD      off;
        DOS_SELECTOR sel;
    } X86_FAR_PHYS;
    typedef union
    {
        X86_FAR_PHYS x;
        U_DWORD FAR *lp;
        U_BYTE  FAR *cp;
    } FARX86_PTR;
/*
#else
    typedef unsigned long *FARX86_PTR;
#endif
*/

typedef struct
{
    FARX86_PTR    reg;     /* Pointer to memory-mapped registers shared
                            * by host and target.  May not be supported by
                            * all hardware.  If not supported, will be NULL.
                            */
    FARX86_PTR    lcl;     /* Pointer to target's memory-mapped address 
                            * space (i.e., LoCaL target memory).  If not
                            * supported, will be NULL.
                            */
    U_DWORD       lcl_size;/* Size of local target memory (in bytes).  N/A
                            * if lcl is NULL.
                            */
    DOS_SELECTOR  reg_sel; /* x86 selector used to create "reg".  N/A if
                            * reg is NULL.
                            */
    DOS_SELECTOR  lcl_sel; /* x86 selector used to create "lcl".  N/A if
                            * reg is NULL.
                            */
} PCI_MMAP_STRUCT;

/*
 * Define a structure that describes the PCI device currently being
 * addressed.
 */
typedef struct
{
    int             real_mode;
                              /* Boolean, T -> processor in real mode. */
    int             initial_bios_lookup;
                              /* How was this device initially accessed
                               * by the PCI BIOS.  Valid values are:
                               *
                               * PCI_ABSOLUTE_ADDR - user specified an
                               *                     absolute bus address.
                               *
                               * PCI_FIND_NEXT     - user specified only
                               *                     a vendor and device ID.
                               */
    PCI_MMAP_STRUCT mem;      /* Address PCI data via memory-mapped
                               * accesses, or
                               */
    DOS_IOADDR      iolcl;    /* Address target local memory via this I/O
                               * address.  May not be supported by all
                               * targets.
                               */
    DOS_IOADDR      ioreg;    /* Address PCI shared runtime registers via
                               * this I/O address.  May not be supported by
                               * all targets.
                               */
    COM_PCI_CFG     cfg;      /* What PCI card to access and how. */
} DOS_PCI_INFO;

/* ------------------------------- Externs ------------------------- */

#if defined(MSDOS) && defined(_MSC_VER)

    extern U_DWORD inpd(unsigned int port);
        /* input an unsigned long from "port" using a 32-bit I/O instruction */

    extern void int1a(X86REG *regs);
        /* execute int 1A and receive 32-bit register results. */

    extern void          outpd(unsigned int port, unsigned long data);
        /* output a 32-bit value to "port" using a 32-bit I/O instruction */

#endif

#endif /* DOS_PCI_H */
