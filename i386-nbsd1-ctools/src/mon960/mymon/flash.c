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
/* This driver can be used for any board which has a single bank of
 * Flash EPROM.  The memory may be 1, 2, or 4 bytes wide.  The driver
 * will erase and program all devices in parallel.
 * The include file 'this_hw.h' must define the following symbols:
 *    FLASH_ADDR - the base address of the Flash memory
 *    FLASH_WIDTH - the number of devices which are accessed in parallel
 *    FLASH_BANKS - the number of devices which are accessed in serial
 *
 *    PROC_FREQ - the processor clock frequency in MHz
 * or
 *
 * If PROC_FREQ is not defined, the bentime timer is used to calibrate
 * the timing delays.  The timer is used only during initialization or
 * available flash; it is to the application after that.
 * It the timer is active during erase_flash the timer is used without
 * accecting the times used.
 */

#include "retarget.h"
#include "mon960.h"
#include "hdi_errs.h"
#include "this_hw.h"

#if FLASH_WIDTH == 4
typedef unsigned long FLASH_TYPE;
#define MASK 0xffffffff
#elif FLASH_WIDTH == 2
typedef unsigned short FLASH_TYPE;
#define MASK 0xffff
#elif FLASH_WIDTH == 1
typedef unsigned char FLASH_TYPE;
#define MASK 0xff
#endif /* FLASH_WIDTH */

#define READ_CMD    (0x00000000 & MASK)
#define WRITE_CMD   (0x40404040 & MASK)
#define STOP_CMD    (0xc0c0c0c0 & MASK)
#define ERASE_CMD   (0x20202020 & MASK)
#define VERIFY_CMD  (0xa0a0a0a0 & MASK)
#define RESET_CMD   (0xffffffff & MASK)
#define READ_ID_CMD (0x90909090 & MASK)
#define ERASED_VALUE (0xffffffff & MASK)


/* ASSEMBLY INLINE */

#if MRI_CODE
static void
delay(loops)
{
    asm("loop: cmpdeco 0,`loops`,`loops`");
    asm("      bl      loop");
}

#else /*gcc960 || ic960 */
__inline static void
delay(int loops )
{
    int cnt;

    asm volatile ( "mov %1,%0" : "=d"(cnt) : "d"(loops) );
    asm volatile ( "0: cmpdeco 0,%1,%0; bl 0b" : "=d"(cnt) : "0"(cnt) );
}
#endif /* __GNUC__ */

#ifdef NUM_FLASH_BANKS
static unsigned long flash_size[NUM_FLASH_BANKS];
static int flash_device[NUM_FLASH_BANKS];
static int flash_banks = NUM_FLASH_BANKS;
static ADDR flash_addr_incr = FLASH_ADDR_INCR;
#else
static unsigned long flash_size[1];
static int flash_device[1];
static int flash_banks = 1;
static ADDR flash_addr_incr = 0;
#endif

unsigned long eeprom_size;
ADDR flash_addr=FLASH_ADDR, eeprom_prog_first, eeprom_prog_last;

long max_errors = 0;
static long timeout_6;     /* number of loops needed for 6uS timeout */
static long timeout_10;    /* number of loops needed for 10uS timeout */

static void init_delay(), delay_time(), term_delay();
static int init_loopcnt();
static long loopcnt(int t);
static int program_zero(ADDR addr, unsigned long length);
static int program_word(ADDR addr, FLASH_TYPE data, FLASH_TYPE mask);

/********************************************************/
/* INIT FLASH                          */
/*                                     */
/* This routine initializes the variables for timing    */
/* with any board configuration.  This is used to get   */
/* exact timing every time.                */
/********************************************************/
void
init_eeprom()
{
    volatile FLASH_TYPE *check;
    ADDR flash_bank_addr = FLASH_ADDR;
    int banks = 0;

    /* Set defaults */
    eeprom_size = 0;

    /* set up timing loop numbers */
    /* If init_loopcnt returns -1, the processor architecture
     * is unknown and timing cannot be done. */
    if (init_loopcnt() == ERR)
        return;

    timeout_6 = loopcnt(6);
    timeout_10 = loopcnt(10);

    /* find out if devices are really Flash */
    while (banks++ < flash_banks)
        {
        check = (FLASH_TYPE *)flash_bank_addr;
        *check = RESET_CMD;    /* Reset device, in case it was being   */
        *check = RESET_CMD;    /* programmed when the board was reset. */
        *check = READ_ID_CMD;    /* Read-Intelligent-Identifier command  */

        if (check[0] == (0x89898989 & MASK)) 
            {
            /* find out which type of device is present */
            switch ( check[1] )
                {
                case 0xB8B8B8B8 & MASK:
                    flash_device[banks-1] = 512;     /* 512 Kbit Flash */
                    eeprom_size += 0x10000 * FLASH_WIDTH;
                    flash_size[banks-1] = 0x10000;
                    break;
                case 0xB4B4B4B4 & MASK:
                    flash_device[banks-1] = 1024;     /* 1 Mbit Flash */
                    eeprom_size += 0x20000 * FLASH_WIDTH;
                    flash_size[banks-1] = 0x20000;
                    break;
                case 0xBDBDBDBD & MASK:
                    flash_device[banks-1] = 2048;     /* 2 Mbit Flash */
                    eeprom_size += 0x40000 * FLASH_WIDTH;
                    flash_size[banks-1] = 0x40000;
                    break;
                default:
                    /* fltype was set to 0 above */
                    flash_size[banks-1] = 0;
                    flash_addr += flash_addr_incr;
                    break;
                }
            }
		else
			{
			if (eeprom_size == 0)
                flash_addr += flash_addr_incr;
            flash_size[banks-1] = 0;
            }
        *check = READ_CMD;
        flash_bank_addr += flash_addr_incr;
        }
}


/********************************************************/
/* IS EEPROM                         */
/* Check if memory is Flash */
/*                            */
/* returns TRUE if it is; FALSE if not eeprom ;  */
/* returns ERROR bad addr or partial eeprom    */
/*                                           */
/********************************************************/
int
is_eeprom(ADDR addr, unsigned long length)
{
    ADDR eeprom_end = flash_addr + eeprom_size - 1;
    ADDR block_end = addr + length - 1;

    /* Check for wrap: if the address and length given wrap past
     * the end of memory, it is an error. */
    if (block_end < addr)
        return ERR;
    if (addr >= flash_addr && block_end <= eeprom_end)
        return TRUE;
    if (addr > eeprom_end || block_end < flash_addr)
        return FALSE;
    /* If the block was partly within the Flash, it is an error. */
    return ERR;
}


/********************************************************/
/* CHECK EEPROM                         */
/* Check if Flash is Blank                */
/*                            */
/* returns OK if it is; returns ERROR and sets cmd_stat    */
/* to an error code if memory region is not Flash or if    */
/* it is not blank.                    */
/*                                           */
/********************************************************/
int
check_eeprom(ADDR addr, unsigned long length)
{
    unsigned char *p, *end;

    if (eeprom_size == 0)
        {
        cmd_stat = E_NO_FLASH;
        return ERR;
        }

    if (addr == NO_ADDR)
        {
        addr = flash_addr;
        length = eeprom_size;
        }
    else if (length == 0)
        length = 1;

    if (is_eeprom(addr, length) != 1)
        {
        cmd_stat = E_EEPROM_ADDR;
        return ERR;
        }

    p = (unsigned char *)addr;
    end = p + length; 

    /* find first non_blank address */
    while (p != end)
        {
        if (*p != 0xff)
            {
            cmd_stat = E_EEPROM_PROG;
            eeprom_prog_first = (ADDR)p;

            /* find last non_blank address */
            for (p = end; *--p == 0xff; )
            ;
            eeprom_prog_last = (ADDR)p;

            return ERR;
            }
        p++;
        }

    return OK;
}


/********************************************************/
/* ERASE EEPROM                         */
/*                                   */
/* returns OK if successful; otherwise returns ERROR    */
/* and sets cmd_stat to an error code            */
/*                                     */
/* Flash Programming Algorithm                 */
/* Copyright (c) 1988, Intel Corporation        */
/* All Rights Reserved                             */
/********************************************************/
int
erase_eeprom(ADDR addr, unsigned long length)
{
    FLASH_TYPE *end_addr;
    FLASH_TYPE value = 0;
    volatile FLASH_TYPE *erase_data;
    FLASH_TYPE mask;
    int error, ERRNUM;
    ADDR flash_bank_addr = flash_addr;
    int banks = 0;

    if (eeprom_size == 0)
        {
        cmd_stat = E_NO_FLASH;
        return ERR;
        }

    if (addr != NO_ADDR
        && (addr != flash_addr || (length != 0 && length != eeprom_size)))
        {
        cmd_stat = E_EEPROM_ADDR;
        return ERR;
        }

    /* If it's already erased, don't bother. */
    if (check_eeprom(flash_addr, eeprom_size) == OK)
        return OK;

    init_delay();

    /* erase n flash devices */
    while (banks++ < flash_banks)
        {
		if (flash_size[banks-1] == 0)
		   continue;

        ERRNUM = 1000;
        error = 0;            /* no cumulative errors */

        leds(12,15);
        /* program device to all zeros */
        if (program_zero(flash_bank_addr, flash_size[banks-1]) != OK)
            {
            cmd_stat = E_EEPROM_FAIL;
            return (ERR);
            }

        leds(14,15);
        erase_data = (FLASH_TYPE *)flash_bank_addr;
        end_addr = (FLASH_TYPE *)(flash_bank_addr + flash_size[banks-1]);
        mask = MASK;

        while (1)
            {
            *erase_data = ERASE_CMD & mask;        /* set up erase */
            *erase_data = ERASE_CMD & mask;        /* begin erase */

            delay_time(10000);            /* wait 10 mS */

            /* check for devices being erased */
            while (erase_data != end_addr) 
                {
                *erase_data = VERIFY_CMD;    /* end and verify erase */
                delay(timeout_6);        /* wait 6 uS */
                value = *erase_data;
    
                if (value != ERASED_VALUE)
                    break;
    
                erase_data++;
                }

            /* if completely erased */
            if (erase_data == end_addr)
                break;

            /* check for max errors */
            if (++error == ERRNUM) 
                {
                *erase_data = READ_CMD;
                cmd_stat = E_EEPROM_FAIL;
                term_delay();
                return (ERR);
                }

            /* erase only the invalid banks */
            mask = 0;

            if ((value & 0x000000ff) != 0x000000ff)
                mask |= 0x000000ff;
#if FLASH_WIDTH > 1
            if ((value & 0x0000ff00) != 0x0000ff00)
                mask |= 0x0000ff00;
#if FLASH_WIDTH > 2
            if ((value & 0x00ff0000) != 0x00ff0000)
                mask |= 0x00ff0000;
            if ((value & 0xff000000) != 0xff000000)
                mask |= 0xff000000;
#endif /* FLASH_WIDTH > 2 */
#endif /* FLASH_WIDTH > 1 */

            }

        erase_data = (FLASH_TYPE *)flash_bank_addr;
        *erase_data = READ_CMD;
        flash_bank_addr += flash_addr_incr;
                 if (error > max_errors)
                     max_errors = error;
        }

    term_delay();
    leds(0,15);
    return OK;
}


/********************************************************/
/* PROGRAM ZEROS                            */
/*                                     */
/* Returns OK if successful; otherwise returns ERROR.    */
/*                                     */
/* Flash Programming Algorithm                 */
/* Copyright (c) 1988, Intel Corporation        */
/* All Rights Reserved                             */
/********************************************************/
static int
program_zero(ADDR addr, unsigned long length)
{
    unsigned long i;

    /* begin writing */
    for (i = 0; i < length; i += FLASH_WIDTH, addr += FLASH_WIDTH)
        if (program_word(addr, 0, MASK) != OK)
            return ERR;

    return OK;
}


/********************************************************/
/* WRITE EEPROM                          */
/*                                     */
/* returns OK if successful; otherwise returns ERROR    */
/* and sets cmd_stat to an error code            */
/*                                     */
/* Flash Programming Algorithm                 */
/* Copyright (c) 1988, Intel Corporation        */
/* All Rights Reserved                             */
/********************************************************/
int
write_eeprom(ADDR start_addr, const void *data_arg, int data_size)
{
    const FLASH_TYPE *dataptr = data_arg;
	int i;
#if FLASH_WIDTH > 1
    FLASH_TYPE data, mask;
    int shift, leftover;
#endif

    if (eeprom_size == 0)
        {
        cmd_stat = E_NO_FLASH;
        return ERR;
        }

    /* need to make sure the address begins on a word boundary */
    /* program bytes until it is there */

#if FLASH_WIDTH > 1
    /* find number of bytes done */
    shift = start_addr % FLASH_WIDTH;
    if (shift != 0) 
        {
        /* get starting address to write extra bytes */
        start_addr -= shift;
        data = *dataptr << 8*shift;
        mask = MASK << 8*shift;
        if (data_size < FLASH_WIDTH-shift)
            mask &= MASK >> 8*(FLASH_WIDTH-(shift+data_size));

        if (program_word(start_addr, data, mask) != OK)
            return ERR;

        /* fix start address for rest of bytes */
        start_addr += FLASH_WIDTH;
        dataptr = (const FLASH_TYPE *)
                ((const char *)dataptr + FLASH_WIDTH-shift);
        data_size -= FLASH_WIDTH - shift;

        if (data_size <= 0)
            return OK;
        }

    /* write all full width words possible */
    /* find out how many leftover bytes must be written at end */
    leftover = data_size % FLASH_WIDTH;
    data_size -= leftover;
#endif /* FLASH_WIDTH > 1 */

    for (i=0; i<data_size; i+=FLASH_WIDTH,start_addr+=FLASH_WIDTH,dataptr++)
        {
        if (program_word(start_addr, *dataptr, MASK) != OK)
            return ERR;
        }

#if FLASH_WIDTH > 1
    /* write out leftover bytes */
    if (leftover)
        {
        mask = MASK >> 8*(FLASH_WIDTH-leftover);
        data = *dataptr & mask;

        if (program_word(start_addr, data, mask) != OK)
            return ERR;
        }
#endif /* FLASH_WIDTH > 1 */

    return OK;
}


static int
program_word(ADDR addr, FLASH_TYPE data, FLASH_TYPE mask)
{
    volatile FLASH_TYPE *write_addr = (FLASH_TYPE *)addr;
    FLASH_TYPE verify_data;
    int error = 0;

    while (1)
        {
        *write_addr = WRITE_CMD & mask; /* send write cmd */
        *write_addr = data & mask;    /* write data */

        delay(timeout_10);        /* standby for programming */

        *write_addr = STOP_CMD & mask; /* send stop cmd */

        delay(timeout_6);        /* wait 6 uS */

        verify_data = *write_addr & mask;

        if (verify_data == (data & mask))   /* verify data */
            break;

        if (++error == 25) 
            {
            *write_addr = READ_CMD;
            cmd_stat = E_EEPROM_FAIL;
            return (ERR);
            }

        /* mask off valid data */
        if ((verify_data & 0x000000ff) == (data & 0x000000ff))
            mask &= 0xffffff00;
#if FLASH_WIDTH > 1
        if ((verify_data & 0x0000ff00) == (data & 0x0000ff00))
            mask &= 0xffff00ff;
#if FLASH_WIDTH > 2
        if ((verify_data & 0x00ff0000) == (data & 0x00ff0000))
            mask &= 0xff00ffff;
        if ((verify_data & 0xff000000) == (data & 0xff000000))
            mask &= 0x00ffffff;
#endif /* FLASH_WIDTH > 2 */
#endif /* FLASH_WIDTH > 1 */
        }    

    *write_addr = READ_CMD;
    return OK;
}


#ifdef PROC_FREQ
/* These definitions give the number of clocks per loop in the delay routine. */
#define CX_CLOCKS 2
#define HX_CLOCKS CX_CLOCKS
#define JX_CLOCKS CX_CLOCKS
#define KX_CLOCKS 4

/* PROC_FREQ uses the processor clock frequency, in MHz. It is defined in the */
/* board-specific include file.  (If the processor is running at 25 MHz, */
/* we use __cpu_speed 25, not 25000000.) */

/* dummy routine */
static int
init_loopcnt()
{
}

/* Return the delay constant required to delay t us. */
static long
loopcnt(int t)
{
#if CX_CPU
    return (t * __cpu_speed) / CX_CLOCKS;
#elseif HX_CPU
    return (t * __cpu_speed) / HX_CLOCKS;
#elseif JX_CPU
    return (t * __cpu_speed) / JX_CLOCKS;
#elseif KXSX_CPU
    return (t * __cpu_speed) / KX_CLOCKS;
#endif
}

#else /* NOT PROC_FREQ */

static long time(int loops);
static long loops_ratio;

/* Initialize the loop ratio, which is used by loopcnt to calculate the
 * delay constant for any length of time. */
static int
init_loopcnt()
{
    /* Since this is all integral arithmetic, the units must be small
     * enough to keep rounding error within reason.  That is why all
     * calculations are done it ns.  */

    /* time() returns the time in us for the given loop count */
    long time1000 = time(1000);

    /* time run againi, should be in cache so the time speed should be the
     * max we could see during normal operation */
    /* loops_ration is us per loop */
    loops_ratio = time1000 * 1000;

    if (loops_ratio == 0)
        return ERR;

    return OK;
}

/* Return the delay constant required to delay t us. */
static long
loopcnt(int t)
{
    long t_ns = (long)t * 1000000;
    return 1 + (t_ns / loops_ratio);
}


/* use the bentime timer to delay 10 msec.  if the timer is active */
/* use it.  If the active timer in not the interrupt timer them terminate */
/* it and start it as the interuupt timer. */
static void
init_delay()
{
    int timer;

	timer = get_bentime_timer();	
    
    if (get_timer_intr(timer) == TRUE)
        return;

    /* timer init terms an active time bentime_noint */
    timer_init(timer, 50000 << 16, NULL, NULL);
}

static void
term_delay()
{
    int timer;

	timer = get_bentime_timer();	
    /* leave bentime(interrupt) timer on if it was inuse */
    if (get_timer_intr(timer) == FALSE)
        timer_term(timer);
}

static void
delay_time(unsigned int time_to_delay)
{
    int timer;
    unsigned int ticks;
    
	timer = get_bentime_timer();	

    /* wait for 10 msec to pass timer_read returns ticks in usec.*/
    ticks = timer_read(timer);
    while ((timer_read(timer) - ticks) < time_to_delay) ;
}


/* This routine uses the bentime timer to determine how
 * fast the timing loop is.  It is called only during initialization. After
 * that all timing is done with delay loops except for 10 msc delays.  */
static long
time(int loops)
{
#define TIMING_RUNS 4
    int i, timer, tot_ticks=0, ticks, overhead=0;

	timer = get_bentime_timer();	

    overhead = timer_init(timer, 50000 << 16, NULL, NULL);

    for (i=0; i<TIMING_RUNS; i++)
        {
        ticks = timer_read(timer);
        delay(loops);
        tot_ticks += timer_read(timer) - ticks - overhead;
        }

    timer_term(timer);
    /* figure out total number of ticks */
    /* Return the time in ns */
    return (long)((tot_ticks/TIMING_RUNS) * FLASH_TIME_ADJUST);
}
#endif /* PROC_FREQ */



int
flash_supported(int max_banks, ADDR bank_addr[], unsigned long bank_size[])
{
    ADDR flash_locn = FLASH_ADDR;
    int  i, banks_avail;

#ifdef NUM_FLASH_BANKS
    banks_avail = NUM_FLASH_BANKS;
#else
    banks_avail = 1;
#endif

    for (i = 0; i < max_banks; i++)
    {
        if (i >= banks_avail)
            bank_addr[i] = bank_size[i] = 0;
        else
        {
            bank_addr[i]  = flash_locn;
            flash_locn   += flash_addr_incr;
            bank_size[i]  = flash_size[i];
        }

    }
    return (TRUE);
}
