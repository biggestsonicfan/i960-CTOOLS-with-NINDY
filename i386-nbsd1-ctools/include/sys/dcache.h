/* C program interface to i960(R) Jx and Hx data cache */

#ifndef __DCACHE_H__
#define __DCACHE_H__

#if defined(__i960JA) || defined(__i960JD) || defined(__i960JF) \
	|| defined(__i960JL) || defined(__i960RP) \
	|| defined(__i960HA) || defined(__i960HD) || defined(__i960HT)

/* Disable the data cache */

__asm void dcache_disable(void)
{
% void return;
	dcctl	0,0,g0	/* dst is not updated, no need to say use g0 */
}

/* Enable the data cache */

__asm void dcache_enable(void)
{
% void return;
	dcctl	1,0,g0	/* dst is not updated, no need to say use g0 */
}

/* Invalidate the data cache and unlock any code locked in the cache */

__asm void dcache_invalidate(void)
{
% void return;
	dcctl	2,0,g0	/* dst is not updated, no need to say use g0 */
}

/* Ensure coherency of data cache with memory */

__asm void dcache_ensure_coherency(void)
{
% void return;
	dcctl	3,0,g0	/* dst is not updated, no need to say use g0 */
}

/* Return size, ways, locked status, etc. about the data cache */

__asm unsigned int dcache_get_status(void)
{
% tmpreg return;
	dcctl	4,0,return
}

/* Store contents of data cache into memory */

__asm void dcache_store_cache_sets
	(unsigned short set1, unsigned short set2, char* mem)
{
% reglit set1; tmpreg set2; reglit mem; void return; spillall;
	shlo	16,set2,set2
	or	set1,set2,set2
	dcctl	6,mem,set2
}

#endif

#if defined(__i960HA) || defined(__i960HD) || defined(__i960HT)

/* Flush data cache contents by address */

__asm void dcache_flush_contents_by_address(char* mem)
{
% void return; tmpreg mem;
	dcflusha	mem
}

/* Give address to data cache as hint */

__asm void dcache_hint(char* mem)
{
% void return; tmpreg mem;
	dchint	mem
}

/* Invalidate data cache by address */

__asm void dcache_invalidate_by_address(char* mem)
{
% void return; tmpreg mem;
	dcinva	mem
}

#endif

#endif
