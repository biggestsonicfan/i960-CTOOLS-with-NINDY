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

#include <string.h>

#include "common.h"
#include "i960.h"
#include "retarget.h"
#include "hdi_errs.h"

#if CXHXJX_CPU
extern void enable_dcache(), disable_dcache();
#endif /*CXHXJX*/
extern void * _mon_memcpy(void *, const void *, size_t);
extern int allow_unaligned();
#if MRI_CODE
extern char * _start_of_vars;
#else /*intel ic960 or gnu960*/
extern char _Bdata[];
#endif

static int check_align(ADDR addr, int data_size, int mem_size);
static int fill_eeprom(ADDR addr, const void *data, int data_size, int count);

typedef struct { unsigned long w0, w1;         } longword;
typedef struct { unsigned long w0, w1, w2;     } tripleword;
typedef struct { unsigned long w0, w1, w2, w3; } quadword;

typedef union {
   quadword		quad;
   tripleword		triple;
   longword		longword;
   unsigned long	word;
   unsigned short	half;
   unsigned char	byte;
} Large;

#if MRI_CODE
static unsigned char
ld_byte(ADDR addr)
{
    asm(unsigned char,"ldob (`addr`),g0");
}

static unsigned short
ld_short(ADDR addr)
{
    asm(unsigned short,"ldos (`addr`),g0");
}

static unsigned long
ld_word(ADDR addr)
{
    asm(unsigned long,"ld (`addr`),g0");
}

static longword
ld_long(ADDR addr)
{
    asm(longword,"ldl (`addr`),g0");
}

static tripleword
ld_triple(ADDR addr)
{
    asm(tripleword,"ldt (`addr`),g0");
}

static quadword
ld_quad(ADDR addr)
{
    asm(quadword,"ldq (`addr`),g0");
}

static unsigned char
st_byte(unsigned char Q, ADDR addr)
{
    asm(unsigned char,"stob `Q`,(`addr`)");
}

static unsigned short
st_short(unsigned short Q, ADDR addr)
{
    asm(unsigned short,"stos `Q`,(`addr`)");
}

static unsigned long
st_word(unsigned long Q, ADDR addr)
{
    asm(unsigned long,"st `Q`,(`addr`)");
}

static longword
st_long(longword Q, ADDR addr)
{
    asm(longword,"stl g0,(`addr`)");
}

static tripleword
st_triple(tripleword Q, ADDR addr)
{
    asm(tripleword,"stt g0,(`addr`)");
}

static quadword
st_quad(quadword Q, ADDR addr)
{
    asm(quadword,"stq g0,(`addr`)");
}

#else /*intel ic960 or gcc960*/
static __inline unsigned char
ld_byte(ADDR addr)
{
  unsigned char tmp;
  asm("ldob (%1), %0" : "=d"(tmp) : "d"(addr),
	"m"(*(unsigned char *)addr)); /* "m" needed per TMC */
  return tmp;
}

static __inline unsigned short
ld_short(ADDR addr)
{
  unsigned short tmp;
  asm("ldos (%1), %0" : "=d"(tmp) : "d"(addr),
	"m"(*(unsigned short *)addr)); /* "m" needed per TMC */
  return tmp;
}

static __inline unsigned long
ld_word(ADDR addr)
{
  unsigned long tmp;
  asm("ld (%1), %0" : "=d"(tmp) : "d"(addr),
	"m"(*(unsigned long *)addr)); /* "m" needed per TMC */
  return tmp;
}

static __inline longword
ld_long(ADDR addr)
{
  longword tmp;
  asm("ldl (%1), %0" : "=d"(tmp) : "d"(addr),
	"m"(*(longword *)addr)); /* "m" needed per TMC */
  return tmp;
}

static __inline tripleword
ld_triple(ADDR addr)
{
  tripleword tmp;
  asm("ldt (%1), %0" : "=t"(tmp) : "d"(addr),
	"m"(*(tripleword *)addr)); /* "m" needed per TMC */
  return tmp;
}

static __inline quadword
ld_quad(ADDR addr)
{
  quadword tmp;
  asm("ldq (%1), %0" : "=q"(tmp) : "d"(addr),
	"m"(*(quadword *)addr)); /* "m" needed per TMC */
  return tmp;
}

static __inline void
st_byte(unsigned char Q, ADDR addr)
{
  asm("stob %2, (%1)" : "=m"(*(unsigned char *)addr) /* "m" needed per TMC */
	: "d"(addr), "d"(Q));
}

static __inline void
st_short(unsigned short Q, ADDR addr)
{
  asm("stos %2, (%1)" : "=m"(*(unsigned short *)addr) /* "m" needed per TMC */
	: "d"(addr), "d"(Q));
}

static __inline void
st_word(unsigned long Q, ADDR addr)
{
  asm("st %2, (%1)" : "=m"(*(unsigned long *)addr) /* "m" needed per TMC */
	: "d"(addr), "d"(Q));
}

static __inline void
st_long(longword Q, ADDR addr)
{
  asm("stl %2, (%1)" : "=m"(*(longword *)addr) /* "m" needed per TMC */
	: "d"(addr), "d"(Q));
}

static __inline void
st_triple(tripleword Q, ADDR addr)
{
  asm("stt %2, (%1)" : "=m"(*(tripleword *)addr) /* "m" needed per TMC */
 	 : "d"(addr), "t"(Q));
}

static __inline void
st_quad(quadword Q, ADDR addr)
{
  asm("stq %2, (%1)" : "=m"(*(quadword *)addr) /* "m" needed per TMC */
	: "d"(addr), "q"(Q));
}

#endif /* __GNUC__ */


/*
 * These functions are here because they need to properly handle
 * copying/examining application memory for which we do not know the
 * byte order.  Execution speed is not a big concern in the monitor.
 * The versions in libc are optimized for speed, but make assumptions
 * about the byte order.
 * Note: According to ANSI, redefining standard functions causes undefined
 * behavior; therefore this code may be compiler dependent.
 */
#if MRI_CODE
#pragma inline _mon_memcpy
void *
(_mon_memcpy)(void *dst_arg, const void *src_arg, size_t sz)
#else /*intel or gnu960*/
__inline void *
(_mon_memcpy)(void *dst_arg, const void *src_arg, size_t sz)
#endif
{
    char *dst = dst_arg;
    const char *src = src_arg;
    while (sz-- != 0)
	*dst++ = *src++;
    return dst_arg;
}

size_t
(_mon_strlen)(const char *s)
{
    size_t len = 0;
    while (*s++)
	len++;
    return len;
}

/*
 * load_1_mem and store_1_mem must use a temporary to avoid making
 * unaligned accesses into the buffer.  The buffer is accessed only
 * via byte-access instructions (using memcpy).  The user address
 * is guaranteed to be aligned as required.
 */
#if MRI_CODE
#pragma inline load_1_mem
static int
load_1_mem(ADDR addr, void *buf, int mem_size)
#else /*intel ic960 or gnu960*/
static __inline int
load_1_mem(ADDR addr, void *buf, int mem_size)
#endif
{
   Large Q;

   switch (mem_size)
   {
   case BYTE:   Q.byte     = ld_byte  (addr); break;
   case SHORT:  Q.half     = ld_short (addr); break;
   case WORD:   Q.word     = ld_word  (addr); break;
   case LONG:   Q.longword = ld_long  (addr); break;
   case TRIPLE: Q.triple   = ld_triple(addr); break;
   case QUAD:   Q.quad     = ld_quad  (addr); break;
   } /* switch */

   _mon_memcpy(buf, (unsigned char *)&Q, mem_size);

   return OK;
}

#if MRI_CODE
#pragma inline store_1_mem
static int
store_1_mem(ADDR addr, const void *data, int mem_size)
#else /*intel ic960 or gnu960*/
static __inline int
store_1_mem(ADDR addr, const void *data, int mem_size)
#endif
{
   Large Q;

   _mon_memcpy((unsigned char *)&Q, data, mem_size);

   switch (mem_size)
   {
   case BYTE:   st_byte  (Q.byte,     addr); break;
   case SHORT:  st_short (Q.half,     addr); break;
   case WORD:   st_word  (Q.word,     addr); break;
   case LONG:   st_long  (Q.longword, addr); break;
   case TRIPLE: st_triple(Q.triple,   addr); break; 
   case QUAD:   st_quad  (Q.quad,     addr); break;
   }

   return OK;
}

static int verify_mem(ADDR, int mem_size, const void *data, int data_size);

/*
 * For load_mem() and store_mem(), the data coming in via buf_arg and
 * data_arg, respectively, is in TARGET byte order. All calling
 * routines are responsible to ensure the correct byte order is
 * provided.
 *
 * A consequence of this is that load_1_mem() and store_1_mem()
 * also expect TARGET byte order.
 */

int
load_mem(ADDR addr, int mem_size, void *buf_arg, int buf_size)
{
    unsigned char *buf = buf_arg;

    if (mem_size == 0)
        mem_size = 1;
    else if (check_align(addr, buf_size, mem_size) != OK)
	return ERR;

#if CXHXJX_CPU
	disable_dcache();
#endif /*CXHXJX*/
    while (buf_size > 0)
    {
	if (load_1_mem(addr, buf, mem_size) != OK)
	    return ERR;
	addr += mem_size;
	buf += mem_size;
	buf_size -= mem_size;
    }
#if CXHXJX_CPU
	enable_dcache();
#endif /*CXHXJX*/

    return OK;
}

int
store_mem(ADDR addr_arg, int mem_size, const void *data_arg, int data_size,
	  int verify)
{
    ADDR addr = addr_arg;
    const unsigned char *data = data_arg;
    int bytes_left = data_size;

    /*
     * Provide a basic level of protection for the monitor's data space.
     * This prevents writes that start at the beginning of monitor data.
     * To protect the entire range of the monitor's data would be bad
     * because it would stop both the UI and the HI from writing to items
     * that must be writable.  E.g., the debugger wouldn't be able to write
     * fields in tables, such as the PRCB, that still reside in monitor data.
     */
#if MRI_CODE
    if (addr == (ADDR)_start_of_vars)
#else /* intel ic960 or gnu960*/
    if (addr == (ADDR)_Bdata)
#endif
    {
	cmd_stat = E_WRITE_ERR;  /* This error# should probably be changed */
	return ERR;
    }

    /*
     * Check_eeprom returns OK if the memory region is EEPROM and is erased,
     * in which case we call write_eeprom to write to the memory region.
     * Check_eeprom returns cmd_stat = E_EEPROM_ADDR if the memory region
     * is not EEPROM, in which case we do a normal memory write. In any
     * other case, the memory region is EEPROM, but check_eeprom detected
     * an error, so we propogate that error.  (The error is probably
     * E_EEPROM_PROG, meaning the EEPROM is already programmed,
     * but it could be any other error.)
     */
    if (check_eeprom(addr, data_size) == OK)
	return write_eeprom(addr, data, data_size);
    else if (cmd_stat != E_EEPROM_ADDR && cmd_stat != E_VERSION &&
	 		 cmd_stat != E_NO_FLASH)
	return ERR;

    if (mem_size == 0)
        mem_size = 1;
    else if (check_align(addr, data_size, mem_size) != OK)
	return ERR;

#if CXHXJX_CPU
	disable_dcache();
#endif /*CXHXJX*/
    while (bytes_left > 0)
    {
	if (store_1_mem(addr, data, mem_size) != OK)
	    return ERR;
	addr += mem_size;
	data += mem_size;
	bytes_left -= mem_size;
    }
#if CXHXJX_CPU
	enable_dcache();
#endif /*CXHXJX*/

    if (verify && verify_mem(addr_arg, mem_size, data_arg, data_size) != OK)
	return ERR;

    return OK;
}

int
fill_mem(ADDR addr_arg, int mem_size, const void *data_arg, int data_size, int count)
{
    ADDR addr;
    const unsigned char *data = data_arg;
    int i;

    /*
     * If the memory region is EEPROM, call fill_eeprom to do the operation.
     * See comment about check_eeprom in store_mem.
     */
    if (check_eeprom(addr_arg, count*data_size) == OK)
	return fill_eeprom(addr_arg, data_arg, data_size, count);
    else if (cmd_stat != E_EEPROM_ADDR && cmd_stat != E_VERSION &&
			 cmd_stat != E_NO_FLASH)
	return ERR;

    if (mem_size == 0)
        mem_size = 1;
    else if (check_align(addr_arg, data_size, mem_size) != OK)
	return ERR;

    addr = addr_arg;
    for (i = 0; i < count; i++)
    {
	int j;

	for (j = 0; j < data_size; j += mem_size)
	{
	    if (store_1_mem(addr+j, data+j, mem_size) != OK)
		return ERR;
	    
	}

	for (j = 0; j < data_size; j += mem_size)
	{
	    unsigned char tmp[QUAD];
	    int k;

	    if (load_1_mem(addr+j, tmp, mem_size) != OK)
		return ERR;

	    for (k = 0; k < mem_size; k++)
	    {
		if ((data+j)[k] != tmp[k])
		{
		    cmd_stat = E_VERIFY_ERR;
		    return ERR;
		}
	    }

	}

	addr += data_size;
    }


    return OK;
}

static int
fill_eeprom(ADDR addr, const void *data, int data_size, int count)
{
    int i;

    for (i = 0; i < count; i++)
    {
	if (write_eeprom(addr, data, data_size) != OK)
	    return ERR;
	addr += data_size;
    }

    return OK;
}


int
copy_mem(ADDR dest, int dest_mem_size, ADDR src, int src_mem_size, int length)
{
    int store_length;
    int is_eeprom = 0;
    unsigned char tmp[QUAD];
    unsigned char tmp2[QUAD];

    /*
     * See comment about check_eeprom in store_mem.
     */
    if (check_eeprom(dest, length) == OK)
	is_eeprom = TRUE;
    else if (cmd_stat == E_EEPROM_ADDR || cmd_stat == E_VERSION ||
			 cmd_stat == E_NO_FLASH)
	is_eeprom = FALSE;
    else
	return ERR;

    if (dest_mem_size == 0)
        dest_mem_size = 1;
    else if (check_align(dest, length, dest_mem_size) != OK)
	return ERR;

    if (src_mem_size == 0)
        src_mem_size = 1;
    else if (check_align(src, length, src_mem_size) != OK)
	return ERR;

    /* This algorithm requires that src_mem_size be an integral multiple
     * of dest_mem_size or vice versa.  If one of these is TRIPLE, and
     * the other is not, that cannot be the case.  All other memory sizes
     * are powers of two, so this holds.  */

    if ((src_mem_size == TRIPLE || dest_mem_size == TRIPLE)
        && src_mem_size != dest_mem_size)
    {
	src_mem_size = WORD;
	dest_mem_size = WORD;
    }

    store_length = dest_mem_size;
    if (src_mem_size > store_length)
	store_length = src_mem_size;

    while (length > 0)
    {
	int i;

	/* Read */
	for (i = 0; i < store_length; i += src_mem_size)
	{
	    if (load_1_mem(src, &tmp[i], src_mem_size) != OK)
	        return ERR;
	    src += src_mem_size;
	}

	/* Write */
	if (is_eeprom)
	{
	    if (write_eeprom(dest, tmp, store_length) != OK)
		return ERR;
	    dest += store_length;
	}
	else
	{
	    for (i = 0; i < store_length; i += dest_mem_size)
	    {
		if (store_1_mem(dest, &tmp[i], dest_mem_size) != OK)
		    return ERR;
		dest += dest_mem_size;
	    }

	    /* Verify */
	    dest -= store_length;
	    for (i = 0; i < store_length; i += dest_mem_size)
	    {
		if (load_1_mem(dest, &tmp2[i], dest_mem_size) != OK)
		    return ERR;
		dest += dest_mem_size;
	    }
	    for (i = 0; i < store_length; i++)
	    {
		if (tmp2[i] != tmp[i])
		{
		    cmd_stat = E_VERIFY_ERR;
		    return ERR;
		}
	    }
	}

	length -= store_length;
    }

    return OK;
}


int
verify_mem(ADDR addr, int mem_size, const void *data_arg, int data_size)
{
    const unsigned char *data = data_arg;
    unsigned char tmp[QUAD];
    int i;

    if (mem_size == 0)
        mem_size = 1;
    if (check_align(addr, data_size, mem_size) != OK)
	return ERR;

    while (data_size > 0)
    {
	if (load_1_mem(addr, tmp, mem_size) != OK)
	    return ERR;

	for (i = 0; i < mem_size; i++)
	{
	    if (data[i] != tmp[i])
	    {
		cmd_stat = E_VERIFY_ERR;
		return ERR;
	    }
	}

	addr += mem_size;
	data += mem_size;
	data_size -= mem_size;
    }

    return OK;
}

static int
check_align(ADDR addr, int data_size, int mem_size)
{
#ifdef ALLOW_UNALIGNED
    if (allow_unaligned())
	return OK;
#endif

    if ((mem_size & (mem_size-1)) != 0)
    {
        if ((addr & (QUAD-1)) != 0 || data_size != mem_size)
	{
	    cmd_stat = E_ALIGN;
	    return ERR;
	}
    }
    else if ((addr & (mem_size-1)) != 0 || (data_size & (mem_size-1)) != 0)
    {
	cmd_stat = E_ALIGN;
	return ERR;
    }

    return OK;
}
