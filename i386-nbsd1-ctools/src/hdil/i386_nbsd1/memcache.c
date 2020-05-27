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
/* $Header: /ffs/p1/dev/src/hdil/common/RCS/memcache.c,v 1.9 1995/02/17 19:27:04 cmorgan Exp $$Locker:  $ */

#include <stdio.h>
#include <string.h>
#include "private.h"
#include "hdi_com.h"
#include "dbg_mon.h"

/*
 * This module provides routines to read and write target memory..
 * To improve performance of the debugger, reads are cached.
 * All memory requests are aligned on a byte boundary corresponding
 * to the cache width, and an entire cache entry is filled.
 * Any breakpoints which have been set in the region of memory
 * read are replaced in the cache by the original instructions.
 * Then the appropriate bytes from the cache are copied into the
 * caller's buffer.
 * Memory writes do not go through the cache.  They are written
 * directly to memory.  Any cache entries which overlap
 * the area of memory written are invalidated.
 */


/*
 * In order to prevent communication time-outs, we currently limit
 * the amount of work the target is asked to do at one time.  A request
 * for a larger copy or fill will be broken up into requests of this size.
 */
#define MAX_BLK_SIZE 65536L


#ifndef NOCACHE
#define ALLOW_UNALIGNED
#define CACHE_SIZE 8
#define CACHE_WIDTH 256

struct cache_entry {
    unsigned char count;
    unsigned char mem_size;
    ADDR addr;
    unsigned char data[CACHE_WIDTH];
} cache[CACHE_SIZE];
#endif /* NOCACHE */

#ifdef __STDC__
static int read_mem2(ADDR addr, void *buf, unsigned sz, int mem_size);
static int put_data(const void *data, int sz, int mem_size);
static int fast_device_write(int write_or_fill, ADDR addr, const void *data, unsigned long sz, int mem_size, int verify_or_pattern_size);
#else
static int read_mem2();
static int put_data();
static int fast_device_write();
#endif /* __STDC__ */


/*
 * Read a buffer of data from the target to the host.
 * Returns OK or ERR
 */
hdi_mem_read(addr, buf, sz, bypass_cache, mem_size)
ADDR addr;
void *buf;
unsigned int sz;
int bypass_cache;
int mem_size;
{
	int r;
	
#ifndef NOCACHE
#ifdef ALLOW_UNALIGNED
	/* The cache does not currently support unaligned accesses;
	 * set bypass_cache and let the monitor worry about it. */
	switch (mem_size)
	{
	    case 0:
	    case 1:
		break;

	    case 2:
		if ((addr & 1) != 0 || (sz & 1) != 0)
		    bypass_cache = TRUE;
		break;

	    case 12:
		if ((addr & 15) != 0 || sz != 12)
		    bypass_cache = TRUE;
		break;

	    default:
		/* Access of a word or greater may be aligned as words. */
		if ((addr & 3) != 0 || (sz & 3) != 0)
		    bypass_cache = TRUE;
		break;
	}
#endif

	/*Set bypass cache to value or'ed with cache_by_regions */
	if (bypass_cache == FALSE)
		bypass_cache = (cache_by_regions[addr >> 28] == 0);

	if (sz <= CACHE_WIDTH && !bypass_cache)
	{
	    int i;
	    int entry = 0;
	    ADDR cache_addr = addr & ~(CACHE_WIDTH - 1);
	    int cache_mem_size;
	    int offset = 0;
	    unsigned int to_copy;

	    switch (mem_size)
	    {
		case 0:
		case 1:
		    cache_mem_size = 0;
		    break;
		case 2:
		    if ((addr & 1) != 0 || (sz & 1) != 0)
		    {
			hdi_cmd_stat = E_ALIGN;
			return ERR;
		    }
		    cache_mem_size = 2;
		    break;
		case 12:
		    if ((addr & 15) != 0 || sz != 12)
		    {
			hdi_cmd_stat = E_ALIGN;
			return ERR;
		    }
		    cache_mem_size = 16;
		    break;
		default:
		    if ((addr & (mem_size-1)) != 0 || (sz & (mem_size-1)) != 0)
		    {
			hdi_cmd_stat = E_ALIGN;
			return ERR;
		    }
		    cache_mem_size = 16;
		    break;
	    }

	    for (i = 0; i < CACHE_SIZE; i++)
	    {
		if (cache[i].count > 0
		    && cache[i].addr == cache_addr
		    && (int)cache[i].mem_size == cache_mem_size)
		{
		    entry = i;
	    found:
		    /* Mark as most recently used */
		    cache[entry].count = 255;

		    /* Figure how much we can get out of this cache entry */
		    offset = (int) (addr - cache_addr);
		    to_copy = sz;
		    if (to_copy > (unsigned int) (CACHE_WIDTH - offset))
			to_copy = CACHE_WIDTH - offset;
		    memcpy(buf, &cache[entry].data[offset], to_copy);
		    if (to_copy < sz)
		    {
			if (hdi_mem_read(addr+to_copy,
					 (unsigned char *)buf+to_copy,
					 sz-to_copy,
					 bypass_cache, mem_size) != OK)
			    return(ERR);
		    }
		    return(OK);
		}

		/* Age this entry */
		if (cache[i].count)
		    cache[i].count--;

		/* If this entry is older than the previous oldest
		 * entry, pick it for possible replacement. */
		if (cache[i].count < cache[entry].count)
		    entry = i;
	    }

	    if (read_mem2(cache_addr, cache[entry].data,
			  CACHE_WIDTH, cache_mem_size) == OK)
	    {
		cache[entry].addr = cache_addr;
		cache[entry].mem_size = cache_mem_size;
		goto found;
	    }
	    else
	    {
		switch (hdi_cmd_stat)
		{
		    case E_COMM_ERR:
		    case E_COMM_TIMO:
		    case E_INTR:
		    case E_RUNNING:
			return(ERR);
		    default:
			/* Try just reading the amount requested. */
			break;
		}
	    }
	}
#endif

	r = read_mem2(addr, buf, sz, mem_size);
	return(r);
}

static int
read_mem2(addr, buf_arg, sz, mem_size)
ADDR addr;
void *buf_arg;
unsigned int sz;
int mem_size;
{
	unsigned char *buf = (unsigned char *)buf_arg;
	unsigned int bytes_left;

	if (_hdi_running)
	{
	    hdi_cmd_stat = E_RUNNING;
	    return(ERR);
	}
	
	com_init_msg();
	if (com_put_byte(READ_MEM) != OK
	    || com_put_byte(~OK) != OK
	    || com_put_long(addr) != OK
	    || com_put_short(sz) != OK
	    || com_put_byte(mem_size) != OK
	    || com_put_msg(NULL, 0) != OK)
	{
	    hdi_cmd_stat = com_get_stat();
	    return(ERR);
	}

	_hdi_signalled = FALSE;
	bytes_left = sz;
	while (bytes_left)
	{
	    const unsigned char *rp;
	    int msg_sz;
	    unsigned short actual;

	    if (_hdi_signalled)
	    {
	        hdi_cmd_stat = E_INTR;
	        return(ERR);
	    }

	    rp = com_get_msg(&msg_sz, COM_WAIT);

	    if (rp == NULL)
	    {
	        hdi_cmd_stat = com_get_stat();
	        return(ERR);
	    }

	    if (msg_sz < 2)
	    {
	        hdi_cmd_stat = E_COMM_ERR;
	        return(ERR);
	    }

	    if (rp[1] != OK)
	    {
		hdi_cmd_stat = rp[1];
	        return(ERR);
	    }

	    rp += 2;			/* skip message header */
	    actual = get_short(rp);
	    
	    /* Convert little-endian data from target to host-endian to
	     * return to debugger. */
	    if (mem_size >= 4)
	    {
		register unsigned long *lp = (unsigned long *)buf;
		register unsigned int i;

		for (i = 0; i < actual/4; i++)
		    lp[i] = get_long(rp);
	    }
	    else if (mem_size == 2)
	    {
		register unsigned short *sp = (unsigned short *)buf;
		register unsigned int i;
		
		for (i = 0; i < actual/2; i++)
		    sp[i] = get_short(rp);
	    }
	    else
		memcpy(buf, rp, actual);

	    buf += actual;
	    bytes_left -= actual;
	}


	_hdi_replace_bps(buf_arg, addr, sz, mem_size);

	return OK;
}


int
_hdi_flush_cache()
{
#ifndef NOCACHE
	int i;
	for (i = 0; i < CACHE_SIZE; i++)
	    cache[i].count = 0;
#endif
	return OK;
}

void
_hdi_invalidate_cache(addr, sz)
ADDR addr;
unsigned int sz;
{
#ifndef NOCACHE
	int i;

	for (i = 0; i < CACHE_SIZE; i++)
	{
	    if (cache[i].count > 0
		&& cache[i].addr < addr + sz
		&& cache[i].addr + CACHE_WIDTH > addr)
	    {
		cache[i].count = 0;
	    }
	}
#endif
}

/*
 * Write a buffer of data from the host to the target memory.
 * Returns OK or ERR.
 * The argument bypass_cache is not currently used, because writes are
 * never cached.
 */
int
hdi_mem_write(addr, data_arg, sz, verify, bypass_cache, mem_size)
ADDR addr;
const void *data_arg;
unsigned int sz;
int verify;
int bypass_cache;
int mem_size;
{
	const unsigned char *data = (unsigned char *)data_arg;
	unsigned int actual;

	if (_hdi_running)
	{
	    hdi_cmd_stat = E_RUNNING;
	    return(ERR);
	}

	if (_hdi_check_bps(addr, sz))
	{
	    hdi_cmd_stat = E_BPSET;
	    return(ERR);
	}

	_hdi_invalidate_cache(addr, sz);

	_hdi_signalled = FALSE;

	if (_hdi_fast_download.download_active == TRUE)
		return(fast_device_write(WRITE_MEM, addr, data, (unsigned long)sz, mem_size, verify));

	while (sz) {
		if (_hdi_signalled)
		{
		    hdi_cmd_stat = E_INTR;
		    return(ERR);
		}

		if ((sz <= MAX_MSG_SIZE - 10))
		    actual = sz;
		else
		    actual = MAX_MSG_SIZE - 10;
			
		com_init_msg();
		if (com_put_byte(WRITE_MEM) != OK
		 || com_put_byte(~OK) != OK	    /* status */
		 || com_put_long(addr) != OK
		 || com_put_short(actual) != OK
	     || com_put_byte(mem_size) != OK
	     || com_put_byte(verify) != OK)
		    {
	        hdi_cmd_stat = com_get_stat();
            return(ERR);
            }
				
		if (put_data(data, actual, mem_size) != OK)
		    {
		    hdi_cmd_stat = com_get_stat();
		    return(ERR);
	        }
		
	    if (_hdi_send(0) == NULL)
	        return(ERR);

		data += actual;
		addr += actual;
		sz -= actual;
	}

	return(OK);
}

/*
 * Fill target memory with a pattern.
 */
int
hdi_mem_fill(addr, pattern, pattern_size, count, mem_size)
ADDR addr;
const void *pattern;
int pattern_size;
unsigned long count;
int mem_size;
{
	unsigned long part_count;
	unsigned char fill = FILL_MEM;

	if ((pattern_size < 1) || (pattern_size > 0xff))
	{
	    hdi_cmd_stat = E_ARG;
	    return(ERR);
	}

	_hdi_flush_cache();

	if (_hdi_fast_download.download_active == TRUE)
		return(fast_device_write(FILL_MEM, addr, pattern, count, mem_size, pattern_size));

	for (part_count = MAX_BLK_SIZE / pattern_size;		/*Init to maximum*/
	     count > 0;
	     count -= part_count)
	{
	    if (part_count > count)
		part_count = count;

	    com_init_msg();
	    if (com_put_byte(FILL_MEM) != OK
	        || com_put_byte(~OK) != OK
	        || com_put_long(addr) != OK
	        || com_put_long(part_count) != OK
	        || com_put_byte(mem_size) != OK
	        || com_put_byte(pattern_size) != OK
	        || put_data(pattern, pattern_size, mem_size) != OK)
	    {
	        hdi_cmd_stat = com_get_stat();
	        return(ERR);
	    }

	    if (_hdi_send(0) == NULL)
	        return(ERR);

	    addr += part_count * pattern_size;
	}

	return(OK);
}

static int
put_data(data, sz, mem_size)
const void *data;
int sz;
int mem_size;
{
    /* Send host-endian data from debugger to target as little-endian */
    if (mem_size >= 4)
    {
	register unsigned long *lp = (unsigned long *)data;
	register int i;
	
	for (i = 0; i < sz/4; i++)
	    if (com_put_long(lp[i]) != OK)
		return ERR;
    }
    else if (mem_size == 2)
    {
	register unsigned short *sp = (unsigned short *)data;
	register int i;
	
	for (i = 0; i < sz/2; i++)
	    if (com_put_short(sp[i]) != OK)
		return ERR;
    }
    else
    {
	if (com_put_data(data, sz) != OK)
	    return ERR;
    }

    return OK;
}

static int
fast_device_write(write_or_fill, addr, data, sz, mem_size, verify_or_pattern_size)
int write_or_fill;
ADDR addr;
const void *data;
unsigned long sz;
int mem_size, verify_or_pattern_size;
{
    unsigned short crc_value = 0;
    unsigned char  msg_str[1];
    unsigned char  msize = mem_size;
    unsigned char  vpsize = verify_or_pattern_size;

/* 
 * FIXME -- all of this swap stuff needs to be ripped out when we make
 * the monitor endian-neutral re: fast download.
 */
#ifndef MSDOS
	unsigned char crc_str[2];
    struct swap
    {
        unsigned char s[4];
        union 
        {
            unsigned long val;
            unsigned char x[4];
        } u;
    } aswap, sswap;

    aswap.u.val = addr;
    aswap.s[0]  = aswap.u.x[3];
    aswap.s[1]  = aswap.u.x[2];
    aswap.s[2]  = aswap.u.x[1];
    aswap.s[3]  = aswap.u.x[0];
    sswap.u.val = sz;
    sswap.s[0]  = sswap.u.x[3];
    sswap.s[1]  = sswap.u.x[2];
    sswap.s[2]  = sswap.u.x[1];
    sswap.s[3]  = sswap.u.x[0];
#endif
#ifdef MSDOS
    /* if direct put works we are done, if it fails try MB io */
    if ((msize < 2) && (_hdi_fast_download.download_type == FAST_PCI_DOWNLOAD))
		{
		if (com_pci_direct_put(addr, (unsigned char *)data, sz) == OK)
		    return(OK);
        if (hdi_cmd_stat != OK)
            return ERR;
		}
#endif

    msg_str[0] = (write_or_fill == WRITE_MEM) ? WRITE_MEM : FILL_MEM;

    if ((_hdi_fast_download.fast_device_put((void *)msg_str, 1, NULL) != OK) ||
/* 
 * FIXME -- all of this swap stuff needs to be ripped out when we make
 * the monitor endian-neutral re: fast download.
 */
#ifdef MSDOS
        (_hdi_fast_download.fast_device_put((void *)&addr, 4, &crc_value) != OK) ||
        (_hdi_fast_download.fast_device_put((void *)&sz, 4, &crc_value) != OK) ||
#else  /* must be a big endian UNIX host */
        (_hdi_fast_download.fast_device_put((void *)aswap.s, 4, &crc_value) != OK) ||
        (_hdi_fast_download.fast_device_put((void *)sswap.s, 4, &crc_value) != OK) ||
#endif
        (_hdi_fast_download.fast_device_put((void *)&msize, 1, &crc_value) != OK) ||
        (_hdi_fast_download.fast_device_put((void *)&vpsize, 1, &crc_value) != OK) ||
        (_hdi_fast_download.fast_device_put((void *)"\377", 1, NULL) != OK)) /* 0xff */
        {
        return(ERR);
        }

    if (write_or_fill == FILL_MEM)
        {
        if (_hdi_fast_download.fast_device_put((void *)data, 
                             (unsigned int)verify_or_pattern_size, 
                             &crc_value) != OK)
            {
            return(ERR);
            }
        }
    else  /* write_mem */
        {
        /* Send host-endian data from debugger to target as little-endian */
        if (_hdi_fast_download.fast_device_put((void *)data, 
                                               sz, 
                                               &crc_value) != OK)
            {
            return(ERR);
            }
        }


/* 
 * FIXME -- all of this swap stuff needs to be ripped out when we make
 * the monitor endian-neutral re: fast download.
 */
#ifdef MSDOS
    if ((_hdi_fast_download.fast_device_put((void *)&crc_value, 2, NULL) != OK) ||
#else
    crc_str[0] = ((char *) &crc_value)[1];
    crc_str[1] = ((char *) &crc_value)[0];
    if ((_hdi_fast_download.fast_device_put((void *)crc_str, 2, NULL) != OK) ||
#endif
        (_hdi_fast_download.fast_device_put((void *)"\377", 1, NULL) != OK)) /* 0xff */
        {
        return(ERR);
        }

    if (_hdi_signalled)
    	{
        hdi_cmd_stat = E_INTR;
        return(ERR);
        }

    return OK;
}


/*
 * Copy target memory from one area to another.
 */
hdi_mem_copy(dst, src, length, dst_mem_size, src_mem_size)
ADDR dst;
ADDR src;
unsigned long length;
int dst_mem_size, src_mem_size;
{
	unsigned long part_length;

	_hdi_flush_cache();

	for (part_length = MAX_BLK_SIZE;		/*Init to maximum*/
	     length > 0;
	     length -= part_length)
	{
	    if (part_length > length)
		part_length = length;

	    com_init_msg();
	    if (com_put_byte(COPY_MEM) != OK
	        || com_put_byte(~OK) != OK
	        || com_put_long(src) != OK
	        || com_put_byte(src_mem_size) != OK
	        || com_put_long(dst) != OK
	        || com_put_byte(dst_mem_size) != OK
	        || com_put_long(part_length) != OK)
	    {
	        hdi_cmd_stat = com_get_stat();
	        return(ERR);
	    }

	    if (_hdi_send(0) == NULL)
	        return(ERR);

	    src += part_length;
	    dst += part_length;
	}

	return(OK);
}
