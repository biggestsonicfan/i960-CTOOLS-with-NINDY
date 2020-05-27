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
/*
 *  This file contains definitions for 960-specific types and constants
 *  used by mon960.
 */

#ifndef I960_H
#define I960_H

#ifndef HDI_REGS_H
#   include "hdi_regs.h"
#endif

#ifndef STD_DEFS_H
#   include "std_defs.h"
#endif

#define    BYTE        1
#define    SHORT       2
#define    WORD        4
#define    LONG        8
#define    TRIPLE     12
#define    EXTENDED   10
#define    QUAD       16

/*=====================================================================*/
/*== 80960 Processor Fault Definitions ================================*/
/*=====================================================================*/

/* Fault Record */

#define FAULT_MAGIC_NUMBER 0x27F   /* for system procedures     */
#define TRACE_MAGIC_NUMBER 0x2BF   /* for trace procedure KX SX */
#define FAULT_SYS_CALL(x) ((void (*)())((x<<2)|2))
#define RESERVED           0

/* fault record */

typedef struct
   {
   unsigned int  pc;         /* process controls                   */
   unsigned int  ac;         /* arithmetic controls                */
   unsigned int  type;      /* type, subtype, and flags */
   unsigned int  *addr;      /* address of faulting instr          */
   }FAULT_RECORD;

/* fault table entry */

typedef struct
   {
   void (*flt_hndlr)();  /* pointer to fault handler */
   int magic_number;
   }FAULT_TABLE_ENTRY;

/* fault table */

typedef struct
   {
   FAULT_TABLE_ENTRY ft_entry[32];
   }FAULT_TABLE;
/*--------------------------------------------------------------------*/
/* Parallel Faults */
#define PARALLEL_FAULT  0

/* subtype indicates the number of parallel faults */
/*--------------------------------------------------------------------*/
#define TRACE_FAULT  1
                                 /* subtypes */
#define INSTRUCTION_TRACE  (1<<1)
#define BRANCH_TRACE       (1<<2)
#define CALL_TRACE         (1<<3)
#define RETURN_TRACE       (1<<4)
#define PRERETURN_TRACE    (1<<5)
#define SUPERVISOR_TRACE   (1<<6)
#define BREAKPOINT_TRACE   (1<<7)
/*--------------------------------------------------------------------*/
#define OPERATION_FAULT 2
                                 /* subtypes */
#define INVALID_OPCODE     0x01
#define UNIMPLEMENTED      0x02
#define UNALIGNED          0x03  /* CA specific */
#define INVALID_OPERAND    0x04
/*--------------------------------------------------------------------*/
#define ARITHMETIC_FAULT   3
                              /* subtypes */
#define INTEGER_OVERFLOW         0x01
#define ARITHMETIC_ZERO_DIVIDE   0x02
/*--------------------------------------------------------------------*/
#define FLOATING_POINT_FAULT  4
                              /* subtypes */
#define FLOATING_OVERFLOW           (1<<0)
#define FLOATING_UNDERFLOW          (1<<1)
#define FLOATING_INVALID_OPERATION  (1<<2)
#define FLOATING_ZERO_DIVIDE        (1<<3)
#define FLOATING_INEXACT            (1<<4)
#define FLOATING_RESERVED_ENCODING  (1<<5)
/*--------------------------------------------------------------------*/
#define CONSTRAINT_FAULT   5
                              /* subtypes */
#define CONSTRAINT_RANGE   1
#define PRIVILEGED         2
/*--------------------------------------------------------------------*/
#define VIRTUAL_MEMORY_FAULT  6
                              /* subtypes */
#define INVALID_SEGMENT_TABLE_ENTRY    1
#define INVALID_PAGE_TABLE_DIR_ENTRY   2
#define INVALID_PAGE_TABLE_ENTRY       3
/*--------------------------------------------------------------------*/
#define PROTECTION_FAULT   7
                              /* subtypes */
#define LENGTH       (1<<1)
#define PAGE_RIGHTS  (1<<2)
/*--------------------------------------------------------------------*/
#define MACHINE_FAULT   8
                              /* subtypes */
#define BAD_ACCESS      1
/*--------------------------------------------------------------------*/
#define STRUCTURAL_FAULT   9
                                 /* subtypes */
#define CONTROL   1
#define DISPATCH  2
#define IAC       3
/*--------------------------------------------------------------------*/
#define TYPE_FAULT   0xA
                                 /* subtypes */
#define TYPE_MISMATCH    1
#define CONTENTS         2
/*--------------------------------------------------------------------*/
#define PROCESS   0xC
                                 /* subtypes */
#define TIME_SLICE 1
/*--------------------------------------------------------------------*/
#define DESCRIPTOR   0xD
                                 /* subtypes */
#define INVALID_DESCRIPTOR 1
/*--------------------------------------------------------------------*/
#define EVENT        0xE
                                /* subtypes */
#define EVENT_NOTICE 1
/*--------------------------------------------------------------------*/

extern FAULT_RECORD *i960_get_fault_rec(void);


/*=====================================================================*/
/*===== Interrupt definitions and functions ===========================*/
/*=====================================================================*/

/*
 *  Interrupt Table
 *
 *  The following structures defines the interrupt table.  The interrupt 
 *  table is made up of a header (for posting interrupts)
 *  and the interrupt procedure entries.
 */

typedef struct
    {
    unsigned long  pending_priorities;
    unsigned long  pending_interrupts[8];
    void    (*interrupt_proc[248])();
    }INTERRUPT_TABLE;

/*
 *  Interrupt Record
 *
 *  Structure defines the interrupt record which is created on the interrupt
 *  stack when an interrupt procedure is invoked.
 */
 
typedef struct
    {
    unsigned long  pc;
    unsigned long  ac;
    unsigned char  vector_number;
    }INTERRUPT_RECORD;

#if CXHXJX_CPU
/*
 *  960CA Locked Cache Option
 *
 *  Macro to specify an interrupt procedure which is locked in the internal
 *  instruction cache.  Pointer to procedure is cast to integer, 2 is added
 *  to specify the locked cache option, and the resulting integer value is 
 *  cast back to pointer to procedure --- 960CA Only
 */

#define CACHED_INT_PROC(int_proc)  ((void (*)()) (((int) (int_proc))|2))
#endif

extern INTERRUPT_RECORD *i960_get_int_rec(void);

/*=====================================================================*/
/*===== 80960 System Procedure Table ==================================*/
/*=====================================================================*/
    
    /* OR with supervisor stack pointer for supervisor mode trace */
#define SUPER_TRACE(on) ((on)?1:0)

    /* system supervisor call entry */
#define SUPER_CALL(fun_name) ((void (*)())(((int)fun_name)|2))

typedef struct 
   {
   unsigned sysp_res1[3];         /* reserved                         */
   void     *super_stack;         /* supervisor mode stack pointer    */
   unsigned sysp_res2[8];         /* preserved                        */
   void     (*sysp_entry[260])(); /* system procedure entries         */
   }SYS_PROC_TABLE;

#if CXHXJX_CPU
/*====================================================================*/
/*==== i960CA/JX/HX Control Registers and Control Register Table ===========*/
/*====================================================================*/

typedef struct
   {
   unsigned control_reg[28];
   }CONTROL_TABLE;

/* Control Register Table indexes */
#define    IPB0    0
#define    IPB1    1
#define    DAB0    2
#define    DAB1    3
#define    IMAP0   4
#define    IMAP1   5
#define    IMAP2   6
#define    ICON    7
#define    MCON0   8
#define    MCON1   9
#define    MCON2  10
#define    MCON3  11
#define    MCON4  12
#define    MCON5  13
#define    MCON6  14
#define    MCON7  15
#define    MCON8  16
#define    MCON9  17
#define    MCON10 18
#define    MCON11 19
#define    MCON12 20
#define    MCON13 21
#define    MCON14 22
#define    MCON15 23
/* RESERVED       24 */
#define    BPCON  25
#define    PTC    26
#define    BCON   27

#if JX_CPU
#define    LMAR0_ADDR     0xff008108
#define    LMMR0_ADDR     0xff00810c
#define    LMAR1_ADDR     0xff008110
#define    LMMR1_ADDR     0xff008114

#define    IPB0_ADDR      0xff008400
#define    IPB1_ADDR      0xff008404
#define    DAB0_ADDR      0xff008420
#define    DAB1_ADDR      0xff008424
#define    BPCON_ADDR     0xff008440

#define    IPND_ADDR      0xff008500
#define    IMSK_ADDR      0xff008504
#define    ICON_ADDR      0xff008510
#define    IMAP0_ADDR     0xff008520
#define    IMAP1_ADDR     0xff008524
#define    IMAP2_ADDR     0xff008528

#define    PMCON0_ADDR    0xff008600
#define    PMCON2_ADDR    0xff008608
#define    PMCON4_ADDR    0xff008610
#define    PMCON6_ADDR    0xff008618
#define    PMCON8_ADDR    0xff008620
#define    PMCON10_ADDR   0xff008628
#define    PMCON12_ADDR   0xff008630
#define    PMCON14_ADDR   0xff008638
#define    BCON_ADDR      0xff0086fc

#define    PRCB_ADDR      0xff008700
#define    ISP_ADDR       0xff008704
#define    SSP_ADDR       0xff008708
#define    DEVID_ADDR     0xff008700

#define    TRR0_ADDR      0xff000300
#define    TCR0_ADDR      0xff000304
#define    TMR0_ADDR      0xff000308
#define    TRR1_ADDR      0xff000310
#define    TCR1_ADDR      0xff000314
#define    TMR1_ADDR      0xff000318
#endif

#if HX_CPU
#define    GCON_ADDR      0xff008000
#define    DLMCON         0xff008100
#define    LMAR0_ADDR     0xff008108
#define    LMMR0_ADDR     0xff00810c
#define    LMAR1_ADDR     0xff008110
#define    LMMR1_ADDR     0xff008114
#define    LMAR2_ADDR     0xff008118
#define    LMMR2_ADDR     0xff00811c
#define    LMAR3_ADDR     0xff008120
#define    LMMR3_ADDR     0xff008124
#define    LMAR4_ADDR     0xff008128
#define    LMMR4_ADDR     0xff00812c
#define    LMAR5_ADDR     0xff008130
#define    LMMR5_ADDR     0xff008134
#define    LMAR6_ADDR     0xff008138
#define    LMMR6_ADDR     0xff00813c
#define    LMAR7_ADDR     0xff008140
#define    LMMR7_ADDR     0xff008144
#define    LMAR8_ADDR     0xff008148
#define    LMMR8_ADDR     0xff00814c
#define    LMAR9_ADDR     0xff008150
#define    LMMR9_ADDR     0xff008154
#define    LMAR10_ADDR    0xff008158
#define    LMMR10_ADDR    0xff00815c
#define    LMAR11_ADDR    0xff008160
#define    LMMR11_ADDR    0xff008164
#define    LMAR12_ADDR    0xff008168
#define    LMMR12_ADDR    0xff00816c
#define    LMAR13_ADDR    0xff008170
#define    LMMR13_ADDR    0xff008174
#define    LMAR14_ADDR    0xff008178
#define    LMMR14_ADDR    0xff00817c
#define    LMAR15_ADDR    0xff008180
#define    LMMR15_ADDR    0xff008184

#define    IPB0_ADDR      0xff008400
#define    IPB1_ADDR      0xff008404
#define    IPB2_ADDR      0xff008408
#define    IPB3_ADDR      0xff00840c
#define    IPB4_ADDR      0xff008410
#define    IPB5_ADDR      0xff008414
#define    DAB0_ADDR      0xff008420
#define    DAB1_ADDR      0xff008424
#define    DAB2_ADDR      0xff008428
#define    DAB3_ADDR      0xff00842c
#define    DAB4_ADDR      0xff008430
#define    DAB5_ADDR      0xff008434
#define    BPCON_ADDR     0xff008440
#define    XBPCON_ADDR    0xff008444

#define    IPND_ADDR      0xff008500
#define    IMSK_ADDR      0xff008504
#define    ICON_ADDR      0xff008510
#define    IMAP0_ADDR     0xff008520
#define    IMAP1_ADDR     0xff008524
#define    IMAP2_ADDR     0xff008528

#define    PMCON0_ADDR    0xff008600
#define    PMCON1_ADDR    0xff008604
#define    PMCON2_ADDR    0xff008608
#define    PMCON3_ADDR    0xff00860c
#define    PMCON4_ADDR    0xff008610
#define    PMCON5_ADDR    0xff008614
#define    PMCON6_ADDR    0xff008618
#define    PMCON7_ADDR    0xff00861c
#define    PMCON8_ADDR    0xff008620
#define    PMCON9_ADDR    0xff008624
#define    PMCON10_ADDR   0xff008628
#define    PMCON11_ADDR   0xff00862c
#define    PMCON12_ADDR   0xff008630
#define    PMCON13_ADDR   0xff008634
#define    PMCON14_ADDR   0xff008638
#define    PMCON15_ADDR   0xff00863c
#define    BCON_ADDR      0xff0086fc

#define    PRCB_ADDR      0xff008700
#define    ISP_ADDR       0xff008704
#define    SSP_ADDR       0xff008708
#define    DEVID_ADDR     0xff008700

#define    TRR0_ADDR      0xff000300
#define    TCR0_ADDR      0xff000304
#define    TMR0_ADDR      0xff000308
#define    TRR1_ADDR      0xff000310
#define    TCR1_ADDR      0xff000314
#define    TMR1_ADDR      0xff000318

#define    MPAR0_ADDR     0xff008010
#define    MPAMR0_ADDR    0xff008014
#define    MPAR1_ADDR     0xff008018
#define    MPAMR1_ADDR    0xff00801c

#define    MDUB0_ADDR     0xff008080
#define    MDLB0_ADDR     0xff008084
#define    MDUB1_ADDR     0xff008088
#define    MDLB1_ADDR     0xff00808c
#define    MDUB2_ADDR     0xff008090
#define    MDLB2_ADDR     0xff008094
#define    MDUB3_ADDR     0xff008098
#define    MDLB3_ADDR     0xff00809c
#define    MDUB4_ADDR     0xff0080a0
#define    MDLB5_ADDR     0xff0080a4
#define    MDUB5_ADDR     0xff0080a8
#define    MDLB4_ADDR     0xff0080ac
#endif



/*--------------------------------------------------------------------*/
/* IBP0-IBP1 Instruction Address Breakpoint register */

#define  IBP_ENABLE(on)    ((on)?3:0)

/*--------------------------------------------------------------------*/
/* IMAP0-IMAP2 Interrupt Map registers */

/* REMEMBER: IMAP vectors are of the form 0xV2 where V is the     
 *           4 most significant bits of the vector, and the 4     
 *           least significant bits are hardwired to 2            
 */

/*
 * These macros are in the form IMAP_XINT(intr_f,vector)
 * intr_f selects the interrupt field, 0-3 for IMAP0,
 * 4-7 for IMAP1.
 *
 * IMAP_DMAINT(intr_f,vector) is used for programming IMAP2,
 * intr_f selects the interrupt field 0-3.
 *
 *
 * These macros get passed the desired vector -- but must end in 0x2 
 * EXAMPLE:
 * External Dedicated interrupts can be programmed for vectors:
 *
 *          HEX      DECIMAL | HEX      DECIMAL
 *          0x12     18      | 0x92     146
 *          0x22     34      | 0xA2     162
 *          0x32     50      | 0xB2     178
 *          0x42     66      | 0xC2     194
 *          0x52     82      | 0xD2     210
 *          0x62     98      | 0xE2     226
 *          0x72     114     | 0xF2     242
 *          0x82     130     | 
 *
 * 
 *                         
 * IMAP_XINT(0,34) - would poisition the bits so that external 
 * interrupt pin 0 would execute interrupt vector 34
 *
 */

                           /* intr_f == 0-3 for use in IMAP0 */
                           /* intr_f == 4-7 for use in IMAP1 */
#define  IMAP_XINT(intr_f,vector) (((vector)>>4) << ((intr_f & 3)*4))

                                                /* for use in IMAP2 */
#if CX_CPU
#define  IMAP_DMAINT(intr_f,vector) IMAP_XINT(intr_f,vector)
#endif

/*--------------------------------------------------------------------*/
/* ICON Interrupt Control Register  */


   /* possible interrupt modes */
#define  DEDICATED 0
#define  EXPANDED  1
#define  MIXED     2

   /* used to select interrupt mode */
#define  ICON_MODE(mode)   ((mode>-1 && mode<3)?mode:0)

   /* specify which inputs (0-7) are edge or level activated */
#define  EDGE_DETECT(num,on)  ((on)?(1<<(num+2)):0)

   /* 1 disables interrupts, 0 enables interrupts  */
#define  GLOBAL_INTERRUPT_DISABLE(on)   ((on)?(1<<10):0)

   /* Interrupt Mask Operation modes */
#define  UNCHANGED         0
#define  TO_R3_DEDICATED   1
#define  TO_R3_EXPANDED    2
#define  TO_R3_DED_EXP     (TO_R3_DEDICATED | TO_R3_EXPANDED)

#define IMASK_OPERATION(mode)    ((mode>-1 && mode<4)?(mode<<11):0)

#define  VECTOR_CACHE_ENABLE(on) ((on)?(1<<13):0)

      /* select fast sample or debouonce  */
#define  FAST_SAMPLE(on)         ((on)?(1<<14):0)

#if CX_CPU
/* suspend DMA during interrupt microcode */
#define  SUSPEND_DMA(on)         ((on)?(1<<15):0)
/*--------------------------------------------------------------------*/
/* MCON 0-15 Memeory Region Configurartion Registers */
/* 1 for ENABLE, 0 for DISABLE   */
#define BURST(on)      ((on)?0x1:0)
#define READY(on)      ((on)?0x2:0)
#define PIPELINE(on)   ((on)?0x4:0)
#define BIG_ENDIAN(on) ((on)?(0x1<<22):0)
#define DATA_CACHE(on) ((on)?(0x1<<23):0)

/* Bus Width can be 8,16 or 32, default to 8 */
#define BUS_WIDTH(bw)  ((bw==16)?(1<<19):(0)) | ((bw==32)?(2<<19):(0))

/* Wait States */
#define NRAD(ws)    ((ws>-1 && ws<32)?(ws<<3 ):0) /* ws can be 0-31   */
#define NRDD(ws)    ((ws>-1 && ws<4 )?(ws<<8 ):0) /* ws can be 0-3    */
#define NXDA(ws)    ((ws>-1 && ws<4 )?(ws<<10):0) /* ws can be 0-3    */
#define NWAD(ws)    ((ws>-1 && ws<32)?(ws<<12):0) /* ws can be 0-31   */
#define NWDD(ws)    ((ws>-1 && ws<4 )?(ws<<17):0) /* ws can be 0-3    */
#endif

#if HXJX_CPU
#define BUS_WIDTH(bw)  ((bw==16)?(1<<22):(0)) | ((bw==32)?(2<<22):(0))
#define BIG_ENDIAN(on) ((on)?(0x1<<31):0)
#endif
   
#if HX_CPU
#define BURST(on)      ((on)?0x1<<28:0)
#define READY(on)      ((on)?0x1<<29:0)
#define PIPELINE(on)   ((on)?0x1<<24:0)
#define PEN(en)    ((en)?(1<<20):(0))
#define PODD(odd)  ((en)?(1<<21):(0))
/* Wait States */
#define NRAD(ws)    ((ws>-1 && ws<32)?(ws<<0 ):0) /* ws can be 0-31   */
#define NRDD(ws)    ((ws>-1 && ws<4 )?(ws<<6 ):0) /* ws can be 0-3    */
#define NXDA(ws)    ((ws>-1 && ws<16 )?(ws<<16):0) /* ws can be 0-15  */
#define NWAD(ws)    ((ws>-1 && ws<32)?(ws<<8 ):0) /* ws can be 0-31   */
#define NWDD(ws)    ((ws>-1 && ws<4 )?(ws<<14):0) /* ws can be 0-3    */
#endif

/*--------------------------------------------------------------------*/
/* BPCON Breakpoint control register    controls data break points    */

   /* selects break point (bp= 0,1) to enable(en = 1) or disable (0) */
#define BPCON_ENABLE(bp,en)     (((!bp && en)?(3<<16):0) | \
                                  (( bp && en)?(3<<20):0))
#define XBPCON_ENABLE(bp,en)    (en?(0x3<<((bp-2)*4)):0) 
   /* select break point mode for break point bp */
#define BPCON_MODE(bp,mode)     ((bp)?(mode<<22):(mode<<18))
#define XBPCON_MODE(bp,mode)    (mode<<((bp-2)*4+2))

/* data break point modes */
#define  STORE_ONLY           0
#define  DATA_ONLY            1
#define  DATA_OR_INSTRUCTION  2
#define  ANY_ACCESS           3

/*--------------------------------------------------------------------*/
/* BCON Bus Configurartion Control */

#define REGION_TABLE_VALID(on)   ((on)?1:0)
#define RAM_PROTECT_ENABLE(on)   ((on)?2:0)
#if HXJX_CPU
#define SUPER_PROTECT_ENABLE(on) ((on)?4:0)
#endif

/*====================================================================*/
/*====== 80960CA PRCB Processor Control Block ========================*/
/*====================================================================*/

#define ENABLE_UNALIGNED_FAULTS(on)    ((on)?1:((1<<30)|1))
#define ENABLE_I_CACHE(on)             ((on)?0:(1<<16))

typedef struct prcb
   {
   FAULT_TABLE       *fault_table_adr;       /* Fault table address     */
   CONTROL_TABLE     *cntrl_table_adr;       /* Control table address   */
   unsigned          init_ac_reg;            /* Initial ac register     */
   unsigned          fault_config;           /* Non-aligend fault mask  */
   INTERRUPT_TABLE   *interrupt_table_adr;   /* Interrupt table address */
   SYS_PROC_TABLE    *sys_proc_table_adr;    /* System procedure table  */
   unsigned          reserved;               /* Reserved                */
   void              *interrupt_stack;       /* Interrupt stack         */
   unsigned          inst_cache_config;      /* Inst. cache enable      */
   unsigned          reg_cache_config;       /* Number of register sets */
   } PRCB;

/*====================================================================*/
/*======= i960CA Initialization Boot Record ==========================*/
/*====================================================================*/

/*
 * NOTE: The 80960CA ibr must be located at 0xFFFFFF00
 * NOTE: The 80960JX/HX ibr must be located at 0xFEFFFF30
 */
    /* n selects byte 0-3 */
#define BYTE_N(n,data)  (((unsigned)(data) >> (n*8)) & 0xFF)

typedef struct
   {
   unsigned    bus_byte_0;
   unsigned    bus_byte_1;
   unsigned    bus_byte_2;
   unsigned    bus_byte_3;
   void        (*first_inst)();
   PRCB        *prcb_ptr;
   int         check_sum[6];
   }IBR;

#endif   /* __i960CA */

/*====================================================================*/
/*====== 80960KA/KB/SA/SB PRCB Processor Control Block ===============*/
/*====================================================================*/

#if KXSX_CPU
typedef struct prcb
   {
   unsigned          reserved_0;             /* Must be set to 0        */
   unsigned          magic_number_0;         /* Must be set to 0xC      */
   unsigned          reserved_1[3];          /* Must be set to 0        */    
   INTERRUPT_TABLE   *interrupt_table_adr;   /* Interrupt table address */
   void              *interrupt_stack;       /* Interrupt stack pointer */
   unsigned          reserved_2;             /* Must be set to 0        */
   unsigned          magic_number_1;         /* Must be set to 0x1ff    */
   unsigned          magic_number_2;         /* Must be set to 0x27f    */
   FAULT_TABLE       *fault_table_adr;       /* Fault table adr         */
   unsigned          reserved_3[32];         /* Must be set to 0        */
   } PRCB;

/*====================================================================*/
/*====== KA/KB/SA/SB System Address Table ============================*/
/*====================================================================*/
/*
 * NOTE: The 80960KX/SX initial memory image must be located a 0.
 *
 * The checksum words are overlaid on the "preserved" area 
 * of the system address table.
 *
 * The system address table must be located at a 4Kbyte boundry
 *
 */

typedef struct __sat
   {
   struct __sat      *sat_ptr;               /* checksum - SAT address  */
   PRCB              *prcb_ptr;              /* checksum - PRCB address */
   unsigned int             check_word;             /* checksum - check word   */
   void              (*first_inst)();        /* checksum - first ip     */
   unsigned int             check_words[4];         /* checksum - 4 check words*/
   unsigned int             preserved_0[22];        /* preserved  not used     */
   SYS_PROC_TABLE    *sys_proc_table_adr_0;  /* system proceedure table */
   unsigned int             magic_number_0;         /* set to 0x304000fb       */
   unsigned int             preserved_1[2];         /* preserved  not used     */
   struct __sat      *offset_0;              /* set to offset 0 of SAT  */
   unsigned int             magic_number_1;         /* set to 0x00fc00fb       */
   unsigned int             preserved_2[2];         /* preserved  not used     */
   SYS_PROC_TABLE    *sys_proc_table_adr_1;  /* system proceedure table */
   unsigned int             magic_number_2;         /* set to 0x304000fb       */
   unsigned int             preserved_3[2];         /* preserved  not used     */
   SYS_PROC_TABLE    *trace_table_adr;       /* trace table address     */
   unsigned int             magic_number_3;         /* set to 0x304000fb       */
   }SAT;

#endif   /*  (_i960KA || _i960KB || ...)  */


/*====================================================================*/
/*==== Process Controls Register =====================================*/
/*====================================================================*/

typedef struct
   {
   int   trace_enable      :1;
   int   execution_mode    :1;
   int                     :7;   /* reserved */
   int   resume            :1;   /* reserved on i960CA */
   int   trace_fault_pend  :1;
   int   reserved_1        :2;
   int   state             :1;
   int                     :2;   /* reserved */
   int   priority          :5;
   int   internal_state    :11;  /* reserved on i960CA */
   }PC_REG_IMAGE;

/*
 * PC_REG_IMAGE may be used to examine or set bits in the PC 
 * use with i960_modpc()
 */

#define  PC_TRACE_ENABLE(on)        ((on)?1:0)
#define  PC_SUPERVISOR_MODE(on)     ((on)?(1<<1):0)
#define  PC_TRACE_FAULT_PENDING(on) ((on)?(1<<8):0)
#define  PC_INTERRUPTED_STATE(on)   ((on)?(1<<14):0)
#define  PC_PRIORITY(pri)           (((-1<(pri)) && (32>(pri)))?((pri)<<16):0)

/*--------------------------------------------------------------------*/
/*
 * Macro:   MODIFY_PC_PRIORITY(pri)
 *
 * Action:  if pri is valid (-1 < pri < 32), modify the pc and
 *          return the value of the previous pc.  If pri is out
 *          of range return -1
 */
/*--------------------------------------------------------------------*/

#define  MODIFY_PC_PRIORITY(pri) \
            (((pri)>-1 && (pri)<32)?i960_modpc(0x001F0000,((pri)<<16)):-1)

extern unsigned int i960_modpc(unsigned int, unsigned int);


/*====================================================================*/
/*======== Trace Controls Register ===================================*/
/*====================================================================*/

/*
 * TC_REG_IMAGE may be used to examine or set bits in the TC 
 */
typedef struct
   {
   int                     :1;   /* reserved */
   int   instruction_mode  :1;
   int   branch_mode       :1;
   int   call_mode         :1;
   int   return_mode       :1;
   int   prereturn_mode    :1;
   int   supervisor_mode   :1;
   int   breakpoint_mode   :1;

#if HX_CPU
   int   hw_bp_inst_2      :1;   
   int   hw_bp_inst_3      :1;  
   int   hw_bp_inst_4      :1; 
   int   hw_bp_inst_5      :1; 
   int   hw_bp_data_2      :1; 
   int   hw_bp_data_3      :1; 
   int   hw_bp_data_4      :1;
   int   hw_bp_data_5      :1;
   int                     :8; 
#else
   int                     :9;   /* reserved */
   int   instruction_event :1;
   int   branch_event      :1;
   int   call_event        :1;
   int   return_event      :1;
   int   prereturn_event   :1;
   int   supervisor_event  :1;
   int   breakpoint_event  :1;
#endif

   int   hw_bp_inst_1      :1;   /* i960CA only */
   int   hw_bp_inst_0      :1;   /* i960CA only */
   int   hw_bp_data_1      :1;   /* i960CA only */
   int   hw_bp_data_0      :1;   /* i960CA only */
   int                     :4;   /* reserved */
   }TC_REG_IMAGE;

#if CXJX_CPU
/* Trace Event Flags */
#define  TC_INSTRUCTION_EVENT (1<<17)
#define  TC_BRANCH_EVENT      (1<<18)
#define  TC_CALL_EVENT        (1<<19)
#define  TC_RETURN_EVENT      (1<<20)
#define  TC_PRE_RETURN_EVENT  (1<<21)
#define  TC_SUPERVISOR_EVENT  (1<<22)
#define  TC_BREAKPOINT_EVENT  (1<<23)
#endif



#if HX_CPU
/* Hardware Breakpoint Event Flags - 960HX ONLY */
#define  TC_INSTRUCTION_EVENT_2   (1<<8)
#define  TC_INSTRUCTION_EVENT_3   (1<<9)
#define  TC_INSTRUCTION_EVENT_4   (1<<10)
#define  TC_INSTRUCTION_EVENT_5   (1<<11)
#define  TC_DATA_EVENT_2          (1<<12)
#define  TC_DATA_EVENT_3          (1<<13)
#define  TC_DATA_EVENT_4          (1<<14)
#define  TC_DATA_EVENT_5          (1<<15)
#define  TC_INSTRUCTION_EVENT_0   (1<<24)
#define  TC_INSTRUCTION_EVENT_1   (1<<25)
#define  TC_DATA_EVENT_0          (1<<26)
#define  TC_DATA_EVENT_1          (1<<27)
#endif /* __i960HX */

#if CXJX_CPU
/* Trace Mode Bits */
#define  TC_INSTRUCTION_MODE(on) ((on)?(1<<1):0)
#define  TC_BRANCH_MODE(on)      ((on)?(1<<2):0)
#define  TC_CALL_MODE(on)        ((on)?(1<<3):0)
#define  TC_RETURN_MODE(on)      ((on)?(1<<4):0)
#define  TC_PRE_RETURN_MODE(on)  ((on)?(1<<5):0)
#define  TC_SUPERVISOR_MODE(on)  ((on)?(1<<6):0)
#define  TC_BREAKPOINT_MODE(on)  ((on)?(1<<7):0)
/* Hardware Breakpoint Event Flags - 960CA/JX/HX ONLY */
#define  TC_INSTRUCTION_EVENT_0   (1<<24)
#define  TC_INSTRUCTION_EVENT_1   (1<<25)
#define  TC_DATA_EVENT_0          (1<<26)
#define  TC_DATA_EVENT_1          (1<<27)
#endif

#ifndef _MONITOR_DEFINES
extern int cmd_stat;
extern int break_flag;
extern UREG register_set;
extern FPREG fp_register_set[];
extern int host_connection;
#endif

#if KXSX_CPU
typedef struct {
    unsigned long pad[2];
    ADDR address;
    unsigned long flags;
} SAT_ENT;

struct system_base { SAT_ENT *sat_ptr; PRCB *prcb_ptr; };
extern struct system_base get_system_base();
#endif /* KX */

/* Description of a single hardware breakpoint
 *
 * The number of and types of entries in the table are determined by the
 * capabilities of the target processor.  The 'type' field should be hard-coded
 * into a target-specific file and should never be modified at runtime.
 * The 'active', 'mode', and 'addr' fields should be modified to reflect
 * current breakpoint usage.
 */
struct bpt {
    char type;    /* BRK_HW, BRK_DATA, BRK_NONE */
    char active;    /* True if this breakpoint is in use */
    char mode;    /* For data breakpoints, indicates type of access */
    ADDR addr;    /* If active, address at which bpt is placed */
};

#endif /* ! defined I960_H */
