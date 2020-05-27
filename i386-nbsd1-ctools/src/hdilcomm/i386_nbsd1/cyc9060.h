/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1994, 1995 Intel Corporation
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

/* Cyclone PCI-80960 Device Data */
#define VENDOR_CYCLONE                  0x113c    /* Vendor Id */
#define PCI80960                        0x0001    /* Eval Target Device Id */
#define CYC_CONTROLLER_JX               0x911     /* Jx PCI I/O cntrlr Dvc ID */
#define CYC_CONTROLLER_CX               0x912     /* Cx PCI I/O cntrlr Dvc ID */
#define CYC_CONTROLLER_HX               0x913     /* Hx PCI I/O cntrlr Dvc ID */
#define BASE_CLASS                      0xff      /* Base Class = other */
#define SUB_CLASS                       0x00      /* Sub Class = ? */

/* 
 * Create a macro that can be used to select Cyclone targets for which
 * Intel has host and target PCI device drivers.
 */
#define SUPPORTED_CYCLONE_DVCID(X)      ((X) == PCI80960 || \
                                             ((X) >= 0x900 && (X) <= 0x9ff))

#define LOCAL_TO_PCI_OFFSET             0x40000000

/* Some common, shared configuration register addresses */
#define CYC_PCI_MB_MEM_ADDR             0x10
#define CYC_PCI_MB_IO_ADDR              0x14
#define CYC_PCI_LOCAL_MEM_ADDR          0x18

#define PCI_TIMEOUT                     4000000L
#define CYCLONE_DATA_READY_BIT         (1L << 0)
#define CYCLONE_EOD_BIT                (1L << 1)
#define CYCLONE_HOST_TRANSFER          (1L << 8)
#define CYCLONE_TARGET_TRANSFER        (1L << 9)
#define CYCLONE_ERROR_BIT              (1L << 16)
#define CYCLONE_INUSE_BIT              (1L << 31)
#define CYCLONE_INTERRUPT_BIT          (1L << 31)

#define BE_CYCLONE_DATA_READY_BIT      (1L << 24)
#define BE_CYCLONE_EOD_BIT             (1L << 25)
#define BE_CYCLONE_HOST_TRANSFER       (1L << 16)
#define BE_CYCLONE_TARGET_TRANSFER     (1L << 17)
#define BE_CYCLONE_ERROR_BIT           (1L << 8)
#define BE_CYCLONE_ACTIVE_BIT          (1L << 6)
#define BE_CYCLONE_INUSE_BIT           (1L << 7)
#define BE_CYCLONE_INTERRUPT_BIT       (1L << 7)

#if defined(HOST)
/* Shared Run-Time Registers host*/
#define CYCLONE_HOST_RANGE_PTOL_0       0x0
#define CYCLONE_HOST_PTOL_0             0x4
#define CYCLONE_HOST_DATA_MB            0x58
#define CYCLONE_HOST_STATUS_MB          0x5c
#define CYCLONE_HOST_PLDB_REG           0x60

#else /*TARGET*/
#define OK     0
#define ERROR -1 

extern unsigned long read_long();
extern void write_long();
extern int PCI_INSTALLED();

/***** PLX PCI-9060 Definitions *****/
 
/* PCI-9060 decodes its own address and is mapped 0x80000000 - 0x9fffffff */
#define PCI9060_BASE_ADDR               0x80000000
 
/*
 * Macros to get pointers to word, short, or byte registers in PCI9060.
 * Registers are directly memory mapped, starting at address
 * PCI9060_BASE_ADDR.  Some registers are 32 bits, some are 16, and some are 8.
 */
 
#define PCI_WORD_REG(n)   ((volatile unsigned long *)(PCI9060_BASE_ADDR + (n)))
#define PCI_SHORT_REG(n)  ((volatile unsigned short *)(PCI9060_BASE_ADDR + (n)))
#define PCI_CHAR_REG(n)   ((volatile unsigned char *)(PCI9060_BASE_ADDR + (n)))
 
/* PCI Configuration Registers */
#define PCI_VENDOR_ID       PCI_SHORT_REG(0x00)
#define PCI_DEVICE_ID       PCI_SHORT_REG(0x02)
#define PCI_COMMAND         PCI_SHORT_REG(0x04)
#define PCI_STATUS          PCI_SHORT_REG(0x06)
#define PCI_REV_ID          PCI_CHAR_REG(0x08)
/* Note: Class Code Register occupies the upper 24 bits from the word at 0x08 */
#define PCI_CLASS_CODE      PCI_WORD_REG(0x08)
#define PCI_CACHE_LINE_SZ   PCI_CHAR_REG(0x0c)
#define PCI_LATENCY_TIMER   PCI_CHAR_REG(0x0d)
#define PCI_HEADER_TYPE     PCI_CHAR_REG(0x0e)
#define PCI_BIST            PCI_CHAR_REG(0x0f)
#define PCI_BASE_MMAP_REG   PCI_WORD_REG(0x10)
#define PCI_BASE_IOMAP_REG  PCI_WORD_REG(0x14)
#define PCI_BASE_LOCAL_0    PCI_WORD_REG(0x18)
#define PCI_BASE_LOCAL_1    PCI_WORD_REG(0x1c)
#define PCI_BASE_EXP_ROM    PCI_WORD_REG(0x30)
#define PCI_INT_LINE        PCI_CHAR_REG(0x3c)
#define PCI_INT_PIN         PCI_CHAR_REG(0x3d)
#define PCI_MIN_GNT         PCI_CHAR_REG(0x3e)
#define PCI_MAX_LAT         PCI_CHAR_REG(0x3f)


/* Local Configuration Registers - PTOL:  PCI to Local
               LTOP:  Local to PCI
               MTOP:  Direct Master to PCI
               MTOPM: Direct Master to PCI Memory
               MTOPI: Direct Master to PCI I/O */
#define RANGE_PTOL_0           PCI_WORD_REG(0x80)
#define LOCAL_BASE_PTOL_0      PCI_WORD_REG(0x84)
#define RANGE_PTOL_1           PCI_WORD_REG(0x88)
#define LOCAL_BASE_PTOL_1      PCI_WORD_REG(0x8c)
#define RANGE_PTOL_ROM         PCI_WORD_REG(0x90)
#define BREQO_CONTROL          PCI_WORD_REG(0x94)
#define BUS_REGION_DESC_PTOL   PCI_WORD_REG(0x98)
#define RANGE_MTOP             PCI_WORD_REG(0x9c)
#define LOCAL_BASE_MTOPM       PCI_WORD_REG(0xa0)
#define LOCAL_BASE_MTOPI       PCI_WORD_REG(0xa4)
#define PCI_BASE_MTOP          PCI_WORD_REG(0xa8)
#define PCI_CONFIG_ADDR_REG    PCI_WORD_REG(0xac)


/* Shared Run-Time Registers */

#define PTOL_MBOX0      PCI_WORD_REG(0xc0)
#define PTOL_MBOX1      PCI_WORD_REG(0xc4)
#define PTOL_MBOX2      PCI_WORD_REG(0xc8)
#define PTOL_MBOX3      PCI_WORD_REG(0xcc)
#define LTOP_MBOX4      PCI_WORD_REG(0xd0)
#define LTOP_MBOX5      PCI_WORD_REG(0xd4)
#define LTOP_MBOX6      PCI_WORD_REG(0xd8)
#define LTOP_MBOX7      PCI_WORD_REG(0xdc)
#define PTOL_DOORBELL   PCI_WORD_REG(0xe0)
#define LTOP_DOORBELL   PCI_WORD_REG(0xe4)
#define PCI_INT_CSTAT   PCI_WORD_REG(0xe8)
#define PCI_EEPROM_CTL  PCI_WORD_REG(0xec)

/* Shared MON960 Run-Time Registers TARGET*/
#define CYCLONE_TARGET_DATA_MB   LTOP_MBOX6
#define CYCLONE_TARGET_STATUS_MB LTOP_MBOX7
#define CYCLONE_TARGET_PLDB_REG  PTOL_DOORBELL

/* Local DMA Registers */

#define DMA_CH0_MODE      PCI_WORD_REG(0x100)
#define DMA_CH0_PADDR     PCI_WORD_REG(0x104)
#define DMA_CH0_LADDR     PCI_WORD_REG(0x108)
#define DMA_CH0_BCOUNT    PCI_WORD_REG(0x10c)
#define DMA_CH0_DPTR      PCI_WORD_REG(0x110)
#define DMA_CH1_MODE      PCI_WORD_REG(0x114)
#define DMA_CH1_PADDR     PCI_WORD_REG(0x118)
#define DMA_CH1_LADDR     PCI_WORD_REG(0x11c)
#define DMA_CH1_BCOUNT    PCI_WORD_REG(0x120)
#define DMA_CH1_DPTR      PCI_WORD_REG(0x124)
#define DMA_CMD_STAT      PCI_WORD_REG(0x128)
#define DMA_ARB_REG0      PCI_WORD_REG(0x12c)
#define DMA_ARB_REG1      PCI_WORD_REG(0x130)


/* PCI Command Register Bit Definitions */
#define PCI_CMD_IOSPACE       (1 << 0)
#define PCI_CMD_MEMSPACE      (1 << 1)
#define PCI_CMD_MASTER_ENAB   (1 << 2)
#define PCI_CMD_PARITY_RESP   (1 << 6)
#define PCI_CMD_SERR_ENAB     (1 << 8)
#define PCI_CMD_FASTBB_ENAB   (1 << 9)

/* PCI Status Register Bit Definitions */
#define PCI_STAT_FASTBB_CAP    (1 << 7)
#define PCI_DATA_PARITY_ERR    (1 << 8)
#define PCI_TARGET_ABORT       (1 << 11)
#define PCI_RCV_TARGET_ABORT   (1 << 12)
#define PCI_RCV_MASTER_ABORT   (1 << 13)
#define PCI_SIGNALLED_SERR     (1 << 14)
#define PCI_BUS_PARITY_ERR     (1 << 15)
/* retrieve the DEVSEL assertion timing */
/*
#define PCI_DEVSEL_TIMING()   ((*PCI_STATUS & 0x0600) >> 9)
*/
#define PCI_DEVSEL_TIMING()   ((read_long(PCI_STATUS) & 0x0600) >> 9)

/* PCI Class Code Register Bit Definitions - Read Only */
/*
#define PCI_REG_LEVEL_PROG_IF()   ((*PCI_CLASS_CODE & 0x0000ff00) >> 8)
#define PCI_SUB_CLASS_ENCODING()  ((*PCI_CLASS_CODE & 0x00ff0000) >> 16)
#define PCI_BASE_CLASS_ENCODING() ((*PCI_CLASS_CODE & 0xff000000) >> 24)
*/
#define PCI_REG_LEVEL_PROG_IF() \
      ((read_long(PCI_CLASS_CODE) & 0x0000ff00) >> 8)
#define PCI_SUB_CLASS_ENCODING() \
      ((read_long(PCI_CLASS_CODE) & 0x00ff0000) >> 16)
#define PCI_BASE_CLASS_ENCODING() \
      ((read_long(PCI_CLASS_CODE) & 0xff000000) >> 24)

/* PCI Built-in Self Test Register Bit Definitions */
#define PCI_DEVICE_SUPPORTS_BIST   (1 << 7)
/*
#define PCI_POST_BIST_RESULTS(x)   (*PCI_BIST |= (x & 0x0f))
*/
#define PCI_POST_BIST_RESULTS(x) \
      (write_long(PCI_BIST,read_long(PCI_BIST) | (x & 0x0f)))
#define PCI_BIST_INTERRUPT         (1 << 6)

/* Various Interrupt bits and misc. */
#define SIGNALLED_SYSTEM_ERROR     (1 << 14)
#define RCVD_MASTER_ABORT          (1 << 13)
#define RCVD_TARGET_ABORT          (1 << 12)
#define SIGNALLED_TARGET_ABORT     (1 << 11)
#define LOCAL_INT_ACTIVE           (1 << 15)
#define LSERR_INT_ENABLE           (1 << 0)
#define LOCAL_INT_ENABLE           (1 << 16)
#define PCI_INT_ENABLE             (1 << 8)
#define LOCAL_DOORBELL_ENABLE      (1 << 17)
#define PCI_DOORBELL_ENABLE        (1 << 9)
#define DMA_CH0_INT_ENABLE         (1 << 18)
#define DMA_CH1_INT_ENABLE         (1 << 19)
#define LOCAL_DOORBELL_INT         (1 << 20)
#define DMA_CH0_INT                (1 << 21)
#define DMA_CH1_INT                (1 << 22)
#define BIST_INT                   (1 << 23)
#define DMA0_DONE                  (1 << 4)
#define DMA1_DONE                  (1 << 12)
#define CLEAR_CH0_INTS             (1 << 3)
#define CLEAR_CH1_INTS             (1 << 11)
#define MAX_RETRIES_256            (1 << 12)
#define DMA_CH0_ENABLE             (1 << 0)
#define DMA_CH1_ENABLE             (1 << 8)
#define DMA_CH0_START              (1 << 1)
#define DMA_CH1_START              (1 << 9)
#define DMA_CH0_ABORT              (1 << 2)
#define DMA_CH1_ABORT              (1 << 10)
#define DMA_WRITE                  (1 << 3)
#define BIST_NOT_SUPPORTED         (0 << 0)
#define PCI_BURST_TIME             (0xff)          /* 255 clocks, first cut */
 

/* some macros */

#define CLEAR_SIG_SERR() \
write_long(PCI_STATUS, read_long(PCI_STATUS) | SIGNALLED_SYSTEM_ERROR)

#define CLEAR_SIG_ABORT() \
write_long(PCI_STATUS, read_long(PCI_STATUS) | SIGNALLED_TARGET_ABORT)

#define CLEAR_MASTER_ABORT() \
write_long(PCI_STATUS, read_long(PCI_STATUS) | RCVD_MASTER_ABORT)

#define CLEAR_TARGET_ABORT() \
write_long(PCI_STATUS, read_long(PCI_STATUS) | RCVD_TARGET_ABORT)

#define CLEAR_DATA_PARITY_ERROR()  \
write_long(PCI_STATUS, read_long(PCI_STATUS) | PCI_DATA_PARITY_ERR)

#define CLEAR_BUS_PARITY_ERROR() \
write_long(PCI_STATUS, read_long(PCI_STATUS) | PCI_BUS_PARITY_ERR)

#define CLR_DMA_CH0() \
write_long(DMA_CMD_STAT, read_long(DMA_CMD_STAT) | CLEAR_CH0_INTS)

#define CLR_DMA_CH1() \
write_long(DMA_CMD_STAT, read_long(DMA_CMD_STAT) | CLEAR_CH1_INTS)

#define DISABLE_DMA_CH0() \
write_long(DMA_CMD_STAT, read_long(DMA_CMD_STAT) & ~(DMA_CH0_ENABLE))

#define DISABLE_DMA_CH1() \
write_long(DMA_CMD_STAT, read_long(DMA_CMD_STAT) & ~(DMA_CH1_ENABLE))

#define ENABLE_DMA_CH0() \
write_long(DMA_CMD_STAT, read_long(DMA_CMD_STAT) | DMA_CH0_ENABLE)

#define ENABLE_DMA_CH1() \
write_long(DMA_CMD_STAT, read_long(DMA_CMD_STAT) | DMA_CH1_ENABLE)

#define START_DMA_CH0()    \
write_long(DMA_CMD_STAT, read_long(DMA_CMD_STAT) | DMA_CH0_START)

#define START_DMA_CH1()    \
write_long(DMA_CMD_STAT, read_long(DMA_CMD_STAT) | DMA_CH1_START)

#define ABORT_DMA_CH0()    \
write_long(DMA_CMD_STAT, read_long(DMA_CMD_STAT) | DMA_CH0_ABORT)

#define ABORT_DMA_CH1()    \
write_long(DMA_CMD_STAT, read_long(DMA_CMD_STAT) | DMA_CH1_ABORT)

#define DMA_CH0_WRITE()    \
write_long(DMA_CH0_DPTR, read_long(DMA_CH0_DPTR) | DMA_WRITE)

#define DMA_CH0_READ()    \
write_long(DMA_CH0_DPTR, read_long(DMA_CH0_DPTR) & ~(DMA_WRITE))

#define DMA_CH1_WRITE()    \
write_long(DMA_CH1_DPTR, read_long(DMA_CH1_DPTR) | DMA_WRITE)

#define DMA_CH1_READ()    \
write_long(DMA_CH1_DPTR, read_long(DMA_CH1_DPTR) & ~(DMA_WRITE))

#define PCI_INIT_DONE() \
write_long(PCI_EEPROM_CTL, read_long(PCI_EEPROM_CTL) | (1 << 31))

#define IS_DONE_SET()   \
(read_long(PCI_EEPROM_CTL) & (1 << 31))

/* PCI Bus command encodings - really only 4 bits*/
#define PCI_IACK         (unsigned char)0x0
#define PCI_SPECIAL      (unsigned char)0x1
#define PCI_IO_RD        (unsigned char)0x2
#define PCI_IO_WR        (unsigned char)0x3
#define PCI_MEM_RD       (unsigned char)0x6
#define PCI_MEM_WR       (unsigned char)0x7
#define PCI_CFG_RD       (unsigned char)0xa
#define PCI_CFG_WR       (unsigned char)0xb
#define PCI_MEM_RD_MULT  (unsigned char)0xc
#define PCI_DUAL_ADDR    (unsigned char)0xd
#define PCI_MEM_RD_LINE  (unsigned char)0xe
#define PCI_MEM_WR_INV   (unsigned char)0xf

#define DMA_READ_CODE      (PCI_MEM_RD << 0)
#define DMA_WRITE_CODE     (PCI_MEM_WR << 4)
#define MASTER_READ_CODE   (PCI_MEM_RD << 8)
#define MASTER_WRITE_CODE  (PCI_MEM_WR << 12)

#define RANGE_DRAM_32     0xfe000000    /* 32 bit mem. space, no prefetch */
#define RANGE_DRAM_8      0xff800000    /* 32 bit mem. space, no prefetch */
#define RANGE_DRAM_2      0xffe00000    /* 32 bit mem. space, no prefetch */

#define BASE_DRAM         0xa0000001    /* local base and enable */
#define NO_IO_SPACE       0x00000000    /* no I/O CFG space in addr. map  */
#define PCI_MEMORY_SPACE  0x00000000

/* BREQo Control */
#define BREQO_ENABLE      (1 << 4)
#if 0   /* @ 32 clocks sometimes see RDY and BOFF at same time */
#define DEADLOCK_TIMEOUT   0x4      /* deadlock after 32 clocks */
#endif
#define DEADLOCK_TIMEOUT   0x8      /* deadlock after 64 clocks */
#define ROM_REMAP_ADDR      (0xe << 28)   /* Region E is Flash ROM     */

/* Local Bus Region Descriptor */
#define MEM_BUS_32BIT          0x3
#define MEM_USE_RDY_INPUT      (1 << 6)
#define ROM_BUS_8BIT           (0x0 << 16)
#define ROM_USE_RDY_INPUT      (1 << 22)   /* not really necessary */
#define MEM_BURST_ENABLE       (1 << 24)
#define ROM_BURST_DISABLE      (0 << 26)
#define NO_TRDY_WHEN_TXFULL    (1 << 27)   /* errata workaround */
#define RETRY_TIMEOUT          (15 << 28)   /* 120 CPU clocks */

/* PCI Region Descriptor */
#define PCI_MEM_MASTER_ENAB    (1 << 0)
#define PCI_IO_MASTER_DISAB    (0 << 1)
#define PCI_LOCK_ENAB          (1 << 2)
#define PCI_PREFETCH_DISAB     (0 << 3)
#define PCI_RELEASE_FIFO_FULL  (0 << 4)
#define PCI_REMAP_ADDR         (0x0000 < 16)

#define PCI_CONFIG_DISAB       (0 << 31)

#define ROM_DECODE_ENABLE      (1 << 0)
#define ROM_DECODE_DISABLE     (0 << 0)

#define TEST_VAL               (unsigned long) 0x43564d45 /* "CVME" */
#define DRAM_2MEG              0xa0200000
#define DRAM_8MEG              0xa0800000
#define DRAM_32MEG             0xa2000000

/* DMA Local Bus Region Descriptor */
#define LOCAL_BUS_32BIT        0x3
#define USE_RDY_INPUT          (1 << 6)
#define DMA_BURSTING_ENABLE    (1 << 8)
#define DMA_CHAINING_DISABLED  (0 << 9)
#define DMA_INT_ENDOFTRANSFER  (1 << 10)

/* DMA operational stuff */
#define DMA_ARB0_VALUE         0x0       /* Latency and Pause Timers
                     disabled, BREQ input diabled,
                     rotational DMA priority */
#define DMA_ARB1_VALUE         0x0      /* DMAs can request both the
                     PCI and Local busses without
                     regard for # of FIFO entries
                     */ 

#define DMA_MODE_VALUE        (LOCAL_BUS_32BIT | USE_RDY_INPUT |\
             DMA_BURSTING_ENABLE)

/* Synchronization "Messages" passed between the PC and the EP80960 */
#define PCI_TEST_CONTINUE     0xaaaaaaaa
#define PC_CAN_CONTINUE       0xa5a5a5a5
#define PCI_HAS_ABORTED       ~PC_CAN_CONTINUE
#define PC_HAS_ABORTED        ~PCI_CAN_CONTINUE
#define CX_JX_HX_PROCESSOR    0xcccccccc
#define CX_JX_HX_NOTAS        0xeeeeeeee
#define SX_KX_PROCESSOR       0xdddddddd

/* a good place to run memory tests... */
#define ABOVE_EVERYTHING      0xa0100000

/* Test an Set test parameters */
#define MAX_COUNT             1000000    /* number of test iterations */
#define COUNT_STUCK_LIMIT     10000      /* counter stalled indicator */
#define SEM_LOCKED_LIMIT      100000     /* semaphore stuck indicator */
#define TAS_DELAY             50         /* the amount of time between
                                            successive TAS calls */

#define CLR_BIST_INT()       (PCI_CFG->bist &= ~PCI_BIST_INTERRUPT)

/*******************************************************************************
*
* PCI Configuration Space Header
*
*/

typedef struct 
   {
   unsigned short  vendor_id;
   unsigned short  device_id;
   unsigned short  command;
   unsigned short  status;
   unsigned char   revision_id;
   unsigned char   prog_if;
   unsigned char   sub_class;
   unsigned char   base_class;
   unsigned char   cache_line_size;
   unsigned char   latency_timer;
   unsigned char   header_type;
   unsigned char   bist;
   unsigned long   pcibase_mm_regs;   
   unsigned long   pcibase_im_regs;   
   unsigned long   pcibase_local;
   unsigned long   reserved1[5];
   unsigned long   pcibase_exp_rom;
   unsigned long   reserved2[2];
   unsigned char   int_line;
   unsigned char   int_pin;
   unsigned char   min_gnt;
   unsigned char   max_lat;
   } PCI_CONFIG_SPACE;

#endif
