/* C program interface to i960(R) Jx and Hx instruction cache */

#ifndef __ICACHE_H__
#define __ICACHE_H__

#if defined(__i960JA) || defined(__i960JD) || defined(__i960JF) \
	|| defined(__i960JL) || defined(__i960RP) \
	|| defined(__i960HA) || defined(__i960HD) || defined(__i960HT)

/* Disable the instruction cache */

__asm void icache_disable(void)
{
% void return;
	icctl	0,0,g0	/* dst is not updated, no need to say use g0 */
}

/* Enable the instruction cache */

__asm void icache_enable(void)
{
% void return;
	icctl	1,0,g0	/* dst is not updated, no need to say use g0 */
}

/* Invalidate the instruction cache and unlock any code locked in the cache */

__asm void icache_invalidate(void)
{
% void return;
	icctl	2,0,g0	/* dst is not updated, no need to say use g0 */
}

/* Load instructions from function "func" into the instruction cache.
   "ways" indicates number of cache ways to load.
   The number of bytes loaded depends on the size of the cache.
   On the JA/JF/JD, "ways" is 1, which is 1K/2K/2K of a 2K/4K/4K cache.
   On the Hx, "ways" is 1/2/3, which is 4K/8K/12K of a 16K cache */

__asm void icache_load_and_lock(void (*func)(void), unsigned int ways)
{
% const func; const ways; tmpreg temp; void return;
	lda	ways,temp
	icctl	3,func,temp
% reglit func; const ways; tmpreg temp; void return;
	lda	ways,temp
	icctl	3,func,temp
% const func; tmpreg ways; void return;
	icctl	3,func,ways
% reglit func; tmpreg ways; void return;
	icctl	3,func,ways
}

/* Return size, ways, locked status, etc. about the instruction cache */

__asm unsigned int icache_get_status(void)
{
% tmpreg return;
	icctl	4,0,return
}

/* Return locking information about the instruction icache */

__asm unsigned int icache_get_locking_status(void)
{
% tmpreg return;
	icctl	5,0,return
}

/* Store contents of instruction cache into memory */

__asm void icache_store_cache_sets
	(unsigned short set1, unsigned short set2, char* mem)
{
% reglit set1; tmpreg set2; reglit mem; void return; spillall;
	shlo	16,set2,set2
	or	set1,set2,set2
	icctl	6,mem,set2
}

#endif

#endif
