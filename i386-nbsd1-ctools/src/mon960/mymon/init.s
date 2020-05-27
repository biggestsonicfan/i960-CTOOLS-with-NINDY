/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1993, 1994 Intel Corporation
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
/*********************************************************************
 * This module contains the Initial Memory Image, including a PRCB,
 * Control Table (for the CA only), System Address Table (for the
 * Kx only), System Procedure Table, Fault Table, Interrupt Table.
 * These data structures are in ROM during the initial boot.
 * This module also contains the cold start address (start_ip) and
 * the system initialization code.  The initialization code does
 * the following:
 *     * calls pre_init to perform a self-test and enable RAM, if
 *      required
 *    * copies the processor data structures to RAM
 *    * initializes the monitor's data in RAM
 *    * reinitializes the processor, using a sysctl (CA) or IAC (Kx)
 *      to cause the processor to begin using the new PRCB and other
 *      data structures in RAM
 *    * turns off the interrupted state and changes to the monitor's
 *      stack
 *    * branches to main.
 *********************************************************************/

    .file    "init.s"
#include "std_defs.h"

#if KXSX_CPU
/* core initialization block (8 words located at address 0) */
/* overlaid by system address table (first 8 words of       */
/* system address table are not otherwise used)             */
    .text
    .globl _system_address_table
_system_address_table:
    .word  _system_address_table  /*  0 - SAT pointer    */
    .word  _rom_prcb              /*  4 - PRCB pointer   */
    .word  0
    .word  _start_ip              /* 12 - Initial IP */
    .word  -1
    .word  0
    .word  0
    .word  _checksum              /* calculated by linker */
                                  /* _checksum= -(SAT + PRCB + startIP) */
    .space 88

    .word sys_proc_table          /* 120 */
    .word 0x304000fb              /* 124 */

    .space 8

    .word _system_address_table   /* 136 */
    .word 0x00fc00fb              /* 140 */

    .space 8

    .word sys_proc_table          /* 152 */
    .word 0x304000fb              /* 156 */

    .space 8

    .word trace_proc_table        /* 168 */
    .word 0x304000fb              /* 172 */


/* initial PRCB  */

/* This is our startup PRCB.  After initialization, */
/* this will be copied to RAM                 */
/* Note: This is global so it can be accessed by    */
/* board-dependent code if necessary.               */
    .align 6
    .globl   _rom_prcb
_rom_prcb:
    .word    0x0               #   0 - reserved
    .word    0xc               #   4 - initialize to 0x0c 
    .word    0x0               #   8 - reserved
    .word    0x0               #  12 - reserved 
    .word    0x0               #  16 - reserved 
    .word    boot_intr_table   #  20 - interrupt table address
    .word    _intr_stack       #  24 - interrupt stack pointer
    .word    0x0               #  28 - reserved
    .word    0x000001ff        #  32 - pointer to offset zero
    .word    0x0000027f        #  36 - system procedure table pointer
    .word    boot_flt_table    #  40 - fault table
    .space   176-44            #  44 - 172 - reserved
#endif /* KXSX */

#if CXHXJX_CPU
/* initial PRCB  */
/* This is our startup PRCB.  After initialization, */
/* this will be copied to RAM                 */
/* Note: This is global so it can be accessed by    */
/* board-dependent code if necessary.               */
    .align 4
    .globl  _rom_prcb
_rom_prcb:
    .word   boot_flt_table          #  0 - Fault Table
    .word   _boot_control_table     #  4 - Control Register Table
    .word   0x00001000              #  8 - AC reg mask overflow fault
#ifdef ALLOW_UNALIGNED
    .word   0x40000001              # 12 - Flt - Mask Unaligned fault
#else
    .word   0x00000001              # 12 - Flt - Allow Unaligned fault
#endif
    .word   boot_intr_table         # 16 - Interrupt Table
    .word   rom_sys_proc_table      # 20 - System Procedure Table
    .word   0                       # 24 - Reserved
    .word   _intr_stack             # 28 - Interrupt Stack Pointer
    .word   0x00000000              # 32 - Inst. Cache - enable cache
#if CXHX_CPU
    .word   5                       # 36 - Reg. Cache - 5 sets cached
#else /* JX */
    .word   0                       # 36 - Reg. Cache - use all cache sets
#endif /* CXHX */
#endif /* CXHXJX */

/* Rom system procedure table */
/* This table defines the fault entry point.
 * It is used for the trace fault table on the Kx. 
 * Sys_proc_table is initialized by set_prcb the first time it is called,
 * and that procedure table is used for all other entry points to the monitor.
 * The CA requires a valid system procedure table during initialization, so
 * this table is referenced by _rom_prcb.  Ram_prcb refers to sys_proc_table.
 * The Kx SAT refers to sys_proc_table.  The Kx does not require a valid
 * system procedure table during initialization.
 * The supervisor trace bit is off by default.
 */
    .text
    .align 6
rom_sys_proc_table:
trace_proc_table:
    .space   12                 # Reserved
    .word    _trap_stack        # Supervisor stack pointer      
    .space   32                 # Preserved
    .word    _fault_entry+2     # trace handler

/* Entries marked with a * must not be moved to retain backward compatibility */


/*************************************************
 * Boot Fault Table
 * This is only used for a fault during
 * initialization, which is always a fatal error.
 *************************************************/

    .text
    .align    4
boot_flt_table:
    .word    _fatal_fault, 0        # parallel/Override Fault
    .word    _fatal_fault, 0        # Trace Fault
    .word    _fatal_fault, 0        # Operation Fault
    .word    _fatal_fault, 0        # Arithmetic Fault
    .word    _fatal_fault, 0        # Floating Point Fault
    .word    _fatal_fault, 0        # Constraint Fault
    .word    _fatal_fault, 0        # Virtual Memory Fault (MC)
    .word    _fatal_fault, 0        # Protection Fault
    .word    _fatal_fault, 0        # Machine Fault
    .word    _fatal_fault, 0        # Structural Fault (MC) 
    .word    _fatal_fault, 0        # Type Fault
    .word    _fatal_fault, 0        # Type 11 Reserved Fault Handler
    .word    _fatal_fault, 0        # Process Fault (MC)
    .word    _fatal_fault, 0        # Descriptor Fault (MC)
    .word    _fatal_fault, 0        # Event Fault (MC)
    .word    _fatal_fault, 0        # Type 15 Reserved Fault Handler
    .space    16*8            # reserved



/*************************************************
 * Boot Interrupt Table
 * This is only used for a spurious interrupt during
 * initialization, which is always a fatal error.
 **************************************************/

    .text
boot_intr_table:
    .word    0
    .word    0, 0, 0, 0, 0, 0, 0, 0
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr, _fatal_intr
    .word    _fatal_intr, _fatal_intr, _fatal_intr

/* START */
/* Processor starts execution here after reset. */
    .text
    .globl    _start_ip
    .globl    _reinit
_start_ip:
/* Save initial value of g0; in the CA it contains the stepping number. */
    mov       g0, g11

/* Call a board-specific pre-initialization routine.  This routine may do
 * a board self-test, enable/initialize RAM, disable any interrupts that
 * are not disabled by reset, or any other board-specific tasks that must
 * be done first thing.  This routine must be written in assembly language,
 * since the C environment is not set up yet.  Most board-specific
 * initialization should be done by init_hardware, which as called from
 * main, and can be written in C.
 * If the symbol pre_init is defined as 0 (by the linker directives file),
 * the call is unnecessary.
 * Don't use a call, since RAM may not be available yet.
 * May clobber any/all registers and memory, except g14 (return address)
 * and g11 (boot value of g0, contains stepping info on CA).
 */
    lda     pre_init, g0
    cmpobe  0, g0, 1f
    balx    (g0), g14
    st      g0, _dram_size
    b       _reinit

1:  /* pre_init() not called, _dram_size has garbage.  Init to zero. */
    
    mov     0, g1
    st      g1, _dram_size
    mov     g1, g1
    

_reinit:        # re-entry point for the "rs" command
	ld        _dram_size, r13  /* save dram_size accross zero vars */

/* Copy the .data area into RAM.  It has been packed in the EPROM
 * after the code area.  If the copy is not needed (RAM-based monitor), 
 * the symbol rom_data can be defined as 0 in the linker directives file.
 */
#if MRI_CODE
    callj     __initcopy         # MRI copy of initialized VARS to RAM
#else
    lda       rom_data, g1       # load source of copy
    cmpobe    0, g1, 1f
    lda       __Bdata, g2        # load destination
    lda       __Edata, g0
    subo      g2, g0, g0         # calculate length of data area
    bal       move_data          # call copy routine
#endif
1:

/* Initialize the BSS area of RAM. */
#if MRI_CODE
    callj    __init_zerovars  # MRI copy xerovars section to 0
#else
    lda      __Bbss, g2       # start of bss
    lda      __Ebss, g0       # end of bss
    subo     g2, g0, g0       # calculate length of bss
    ldconst  0, g1            # data to fill
    bal      fill_data        # call fill routine
#endif
	st       r13, _dram_size  /* restore dram_size */

/* initialize the interrupt table */

    /* First zero the pending bits */
    ldconst  9*4, g0
    ldconst  0, g1
    lda      intr_table, g2
    bal      fill_data

    /* Then fill in the interrupt table with vectors */
    ldconst 248*4, g0
    lda      _intr_entry, g1
    lda      intr_table+36, g2
    bal      fill_data

/* initialize the fault table */
/* This initializes all entries to the fault entry point.  Set_prcb will
 * initialize the trace entry properly.
 * The KX uses entry 1 of the trace procedure table; the CA uses entry
 * 256 of the regular procedure table.
 */
    ldconst 16, g0            /* Number of fault table entries */
    lda     fault_table, g1
#if KXSX_CPU
    ldconst 2, g2            /* System procedure 0 */
    ldconst 0x2bf, g3        /* Magic number for trace proc table */
#endif /* KXSX */
#if CXHXJX_CPU
    ldconst (256<<2)|2, g2        /* System procedure 256 */
    ldconst 0x27f, g3        /* Magic number for regular proc table */
#endif /* CXHXJX */

    ldconst 0, r3
1:
    stl     g2, (g1)[r3*8]
    addo    1, r3, r3
    cmpobg  g0, r3, 1b

/* initialize the system procedure table */
    lda     sys_proc_table, g0
    call    _init_sys_proc_tbl

#if CX_CPU
/* copy the control table to RAM */
    ldconst 112, g0                 # load length of control table
    lda     _boot_control_table, g1 # load source
    lda     control_table, g2       # load address of new table
    bal     move_data               # branch to move routine
#endif /* CX */


/* copy PRCB to RAM */
#if KXSX_CPU
    ldconst    176, g0         # load length of PRCB
#endif /* KXSX */
#if CXHXJX_CPU
    ldconst    40, g0          # load length of PRCB
#endif /* CX */
    lda        _rom_prcb, g1   # load source
    lda        ram_prcb, g2    # load destination
    bal        move_data       # branch to move routine

    lda        ram_prcb, g0

#if KXSX_CPU
/* fix up the PRCB to point to a new interrupt table and fault table */
    lda   intr_table, g8
    st    g8, 20(g0)
    lda   fault_table, g8
    st    g8, 40(g0)
#endif /* KXSX */

#if CX_CPU
/* fix up the PRCB to point to a new interrupt table, fault table, 
 * system procedure table, and control table */
    lda   control_table, g8
    st    g8, 4(g0)
#endif /* CX */
#if CXHXJX_CPU
    lda   intr_table, g8
    st    g8, 16(g0)
    lda   fault_table, g8
    st    g8, 0(g0)
    lda   sys_proc_table, g8
    st    g8, 20(g0)
#endif /* CXHXJX */

    call    _set_prcb    /* Pass ram_prcb in g0 */

/* At this point, the PRCB and interrupt table have been moved to RAM, */
/* and we can reinitialize with the RAM-based PRCB. */

#if KXSX_CPU
0:  mov     0, g0
    lda     reinitialize_iac, g1
    call    _send_iac

    /* If it doesn't work, try it again; this is to workaround a KB bug */
    b    0b

    .align    4
reinitialize_iac:
    .word   0x93000000, _system_address_table, ram_prcb, 1f

1:
    .globl  _arch
    .globl  _arch_name
#include "hdi_arch.h"
/* test for fp on chip, _arch = 1 for no fp and _arch = 2 for fp */
    ld      _arch, g0 
    cmpobe  ARCH_KA, g0, skip_fp /* don't decrement if we already set no fp */
    ld      _arch, g0 
    cmpobe  ARCH_SA, g0, skip_fp /* don't decrement if we already set no fp */

    lda     sys_proc_table + 48 + 10 * 4, g0 /* sys proc table entry for fault*/
    ld      (g0), g1
    st      g1, save_spt
    lda     fp_op_fault, g1
    st      g1, (g0)
    lda     fault_table + 2 * 8, g0    /* entry 2 */
    ldconst (10 << 2) | 2, g2        /* use spt entry 10 for fault */        
    ldconst 0x27f, g3
    stl     g2, (g0)
     
/* called from above to test for fp on kx or sx chips */
/* Check if floating point exists on chip */
/*    dmovt    g0,g0         test if FP exists */
    .word   0x64801210    /*dmovt g0,g0 */ 

op_fault:
    lda     fault_table + 2 * 8, g0    /* entry 2 */
    ldconst 2, g2
    ldconst 0x2bf, g3
    stl     g2, (g0)
    ld      save_spt,    g1
    lda     sys_proc_table + 48 + 10 * 4, g0 /* sys proc table entry for fault*/
    st      g1, (g0)
skip_fp:
#endif /* KXSX */

#if CXHXJX_CPU
    ldconst  0x300, r4    # reinitialize sys control
    lda      1f, r5
    lda      ram_prcb, r6
    sysctl   r4, r5, r6
1:
    mov      0, g0
    call     _set_mask
    mov      0, g0
    call     _set_pending
#endif /* CXHXJX */

/* Before call to main, we need to take the processor out              */
/* of the "interrupted" state.  We also want to switch to the proper   */
/* stack.  Fix_stack will change the pfp and "fix up" the stack frame  */
/* to cause an interrupt return to be executed which will return to    */
/* a different stack.  Initial_stack is normally defined as            */
/* _monitor_stack.  It is defined as _user_stack if the monitor is     */
/* linked into an application.                                         */
/* NO LOCAL REGISTERS ARE PRESERVED ACROSS THIS CALL!                  */

    lda   initial_stack, g0
    call  fix_stack     # routine to turn off int state and change stack

/* Compiler expects g14 = 0 */
    mov   0, g14
    st    g11, _boot_g0
    b     _main


/* This routine is used to copy data during initialization  */
move_data:    
    ldconst 0, r3
1:
    ld      (g1)[r3*1], r4      # load word into r4
    st      r4, (g2)[r3*1]       # store to destination
    addo    4, r3, r3      # increment index    
    cmpobg  g0, r3, 1b      # loop until done

    bx    (g14)


/* This routine is used to fill data during initialization  */
fill_data:
    ldconst  0, r3
1:
    st       g1, (g2)[r3*1]       # store data to destination
    addo     4, r3, r3      # increment index    
    cmpobg   g0, r3, 1b      # loop until done

    bx    (g14)

/*
 * Routine to turn off interrupt state and change stacks
 * Build a phony interrupt record on the stack before our stack frame
 * which the processor will pick up on return.  This trashes the
 * previous stack frame, but that's okay because we aren't going to
 * return there; we change the pfp to the new stack specified by g0.
 * Also, we will take advantage of the fact that the processor will
 * restore the pc and ac to its registers.
 */
    .text
    .globl    fix_stack
fix_stack:
    flushreg

    ldconst    0, r4           /* set initial pfp, */
    lda        0x40(g0), r5    /* initial sp, */
    ld         8(pfp), r6      /* and return address */
    stt        r4, (g0)        /* in new stack frame */

#if CXHXJX_CPU
    ldconst    0xd81f0002, r4    /* new value for process controls */
                /* the "d8" is required by the CA microcode */
                /* for an interrupt return */ 
#else /* KXSX */
    ldconst 0x001f0002, r4    /* new value for process controls */
#endif /* CXHXJX */

    ldconst    0x3b001000, r5 /* new value for arithmetic controls */
    stl        r4, -16(fp)    /* store pc and ac into interrupt record */
/* i960 core EAS 7.3.3 use flush regs */
    flushreg
    or         g0, 7, pfp     /* change pfp to stack we want to return to */
    flushreg                  /* and put interrupt return code into status */

    ret

#if KXSX_CPU
/* fp operation fault entry point */
fp_op_fault:
    ld      _arch, g0
    subo    1, g0, g0            /* set arch to -1 for no fp  */
    st      g0, _arch
    ldob    _arch_name + 1, g0   /* set arch name from B to A */
    subo    1, g0, g0            /* set arch_name (B) to A(-1) for no fp */
    stob    g0, _arch_name + 1

    flushreg        /* this is required because we change r2 on stack */

    lda       (pfp), g0
    ldconst   0xfffffffc, g1  /* and off low 4 bits */
    and       g1, g0, g0
    lda       op_fault, g1    /* set fake stack to retur from interrupt */
    st        g1, 8(g0)       /* set return pointer to fault on fp */  
    ret
#endif /* KXSX */

/* System data structures */
/* All the processor-defined data structures are copied to these RAM areas.  */
/* NOTE: The system procedure table must not span a 4096-byte boundary.      */
/* This means that it must not be located at an address xxxxxyyy where       */
/* yyy > 0xbc0.  Since this module is the first module linked, and           */
/* sys_proc_table is first bss section in this module, it will be located at */
/* the base of the bss section as defined in the linker-directives file.     */

    .bss    sys_proc_table, 1088, 6
    .bss    intr_table, 1028, 6
    .bss    fault_table, 32*8, 4
#if CX_CPU
    .bss    control_table, 112, 4
#endif /* CX */
    .bss    ram_prcb, 176, 6

/* Stacks */
/* The _user_stack is fairly small.  The monitor starts execution of    */
/* the application on _user_stack, however an application will normally */
/* define its own stack to replace _user_stack, and change to that      */
/* during the application start up code.  (The crt960 file provided     */
/* does this.)                                                          */
/* The _trap_stack will be used only if the application changes the     */
/* processor to user mode.  In this case, subsequent supervisor calls   */
/* and faults will use the trap stack.  The _trap_stack is large enough */
/* to support supervisor calls to the monitor routines.  If the         */
/* application contains supervisor procedures, the size of _trap_stack  */
/* may need to be increased.  If the application never changes the      */
/* processor to user mode, the size of _trap_stack may be reduced to    */
/* reduce memory usage.                                                 */
/* The _intr_stack is used by both the application and the monitor      */
/* for interrupts.  If the application uses interrupts with non-trivial */
/* handlers, the size of _intr_stack may need to be increased.          */

    .globl    _intr_stack
    .globl    _user_stack
    .globl    _trap_stack
    .bss      _user_stack, 0x0200, 6        # default application stack 
    .bss      _intr_stack, 0x0600, 6        # interrupt stack
    .bss      _trap_stack, 0x0800, 6        # fault (supervisor) stack

    .globl    _dram_size
    .data
_dram_size:   .word 0
save_spt:     .word 0
