/* C program interface to i960(R) Jx and Hx on-chip timers */

/* Functions are available to start the timers, stop the timers,
   set initial values, set reload values, put in single-shot or auto-load mode,
   and specify the frequency at which the count-down occurs */

#ifndef __TIMER_H__
#define __TIMER_H__

#if defined(__i960JA) || defined(__i960JD) || defined(__i960JF) \
	|| defined(__i960JL) || defined(__i960RP) \
	|| defined(__i960HA) || defined(__i960HD) || defined(__i960HT)

#define __TIMER_TRR0 0xff000300
#define __TIMER_TCR0 0xff000304
#define __TIMER_TMR0 0xff000308
#define __TIMER_TRR1 0xff000310
#define __TIMER_TCR1 0xff000314
#define __TIMER_TMR1 0xff000318
#define __TIMER_MASK1 0x00302100
#define __TIMER_MASK2 0x00000030
#define __TIMER_MASK3 0xffffffcf

/* Load a value into timer 0 or 1 */

__asm void timer_set_count(unsigned int timer, unsigned int count)
{
% void return; const(0) timer; const count; tmpreg temp; spillall;
	lda	count,temp
	st	temp,__TIMER_TCR0
% void return; const(1) timer; const count; tmpreg temp; spillall;
	lda	count,temp
	st	temp,__TIMER_TCR1
% void return; tmpreg timer; tmpreg count; tmpreg addr; spillall;
	lda	__TIMER_TCR0,addr
	st	count,(addr)[timer*16]
}

/* Get the value of timer 0 or 1 */

__asm unsigned int timer_get_count(unsigned int timer)
{
% tmpreg return; const(0) timer; spillall;
	ld	__TIMER_TCR0,return
% tmpreg return; const(1) timer; spillall;
	ld	__TIMER_TCR1,return
% tmpreg return; tmpreg timer, addr; spillall;
	lda	__TIMER_TCR0,addr
	ld	(addr)[timer*16],return
}

/* Load a reload value into timer 0 or 1 */

__asm void timer_set_reload_count(unsigned int timer, unsigned int count)
{
% void return; const(0) timer; const count; tmpreg temp; spillall;
	lda	count,temp
	st	temp,__TIMER_TRR0
% void return; const(1) timer; const count; tmpreg temp; spillall;
	lda	count,temp
	st	temp,__TIMER_TRR1
% void return; tmpreg timer; tmpreg count; tmpreg addr; spillall;
	lda	__TIMER_TRR0,addr
	st	count,(addr)[timer*16]
}

/* Get the reload value of timer 0 or 1 */

__asm unsigned int timer_get_reload_count(unsigned int timer)
{
% tmpreg return; const(0) timer; spillall;
	ld	__TIMER_TRR0,return
% tmpreg return; const(1) timer; spillall;
	ld	__TIMER_TRR1,return
% tmpreg return; tmpreg timer, addr; spillall;
	lda	__TIMER_TRR0,addr
	ld	(addr)[timer*16],return
}

/* Test whether timer 0 or 1 has counted down to 0 */

__asm int timer_test_zero_count(unsigned int timer)
{
% tmpreg return; const(0) timer; tmpreg temp; spillall;
	ld	__TIMER_TMR0,temp
	chkbit	0,temp
	teste	return
% tmpreg return; const(1) timer; tmpreg temp; spillall;
	ld	__TIMER_TMR1,temp
	chkbit	0,temp
	teste	return
% tmpreg return; tmpreg timer, temp, addr; spillall;
	lda	__TIMER_TMR0,addr
	ld	(addr)[timer*16],temp
	chkbit	0,temp
	teste	return
}

/* Set timer 0 or 1 mode: reload on */

__asm void timer_set_mode_reload_on(unsigned int timer)
{
% void return; const(0) timer; tmpreg temp; spillall;
	ld	__TIMER_TMR0,temp
	setbit	2,temp,temp
	st	temp,__TIMER_TMR0
% void return; const(1) timer; tmpreg temp; spillall;
	ld	__TIMER_TMR1,temp
	setbit	2,temp,temp
	st	temp,__TIMER_TMR1
% void return; tmpreg timer, addr; spillall;
	lda	__TIMER_TMR0,addr
	lda	(addr)[timer*16],addr
	ld	(addr),timer
	setbit	2,timer,timer
	st	timer,(addr)
}

/* Set timer 0 or 1 mode: reload off */

__asm void timer_set_mode_reload_off(unsigned int timer)
{
% void return; const(0) timer; tmpreg temp; spillall;
	ld	__TIMER_TMR0,temp
	clrbit	2,temp,temp
	st	temp,__TIMER_TMR0
% void return; const(1) timer; tmpreg temp; spillall;
	ld	__TIMER_TMR1,temp
	clrbit	2,temp,temp
	st	temp,__TIMER_TMR1
% void return; tmpreg timer, addr; spillall;
	lda	__TIMER_TMR0,addr
	lda	(addr)[timer*16],addr
	ld	(addr),timer
	clrbit	2,timer,timer
	st	timer,(addr)
}

/* Start timer 0 or 1 */

__asm void timer_start(unsigned int timer)
{
% void return; const(0) timer; tmpreg temp; spillall;
	ld	__TIMER_TMR0,temp
	setbit	1,temp,temp
	st	temp,__TIMER_TMR0
% void return; const(1) timer; tmpreg temp; spillall;
	ld	__TIMER_TMR1,temp
	setbit	1,temp,temp
	st	temp,__TIMER_TMR1
% void return; tmpreg timer, addr; spillall;
	lda	__TIMER_TMR0,addr
	lda	(addr)[timer*16],addr
	ld	(addr),timer
	setbit	1,timer,timer
	st	timer,(addr)
}

/* Stop timer 0 or 1 */

__asm void timer_stop(unsigned int timer)
{
% void return; const(0) timer; tmpreg temp; spillall;
	ld	__TIMER_TMR0,temp
	clrbit	1,temp,temp
	st	temp,__TIMER_TMR0
% void return; const(1) timer; tmpreg temp; spillall;
	ld	__TIMER_TMR1,temp
	clrbit	1,temp,temp
	st	temp,__TIMER_TMR1
% void return; tmpreg timer, addr; spillall;
	lda	__TIMER_TMR0,addr
	lda	(addr)[timer*16],addr
	ld	(addr),timer
	clrbit	1,timer,timer
	st	timer,(addr)
}

/* Set timer 0 or 1 clock rate */

__asm void timer_set_clock_divisor(unsigned int timer, int divisor)
{
% void return; const(0) timer; const divisor; \
  tmpreg ttimer, tdivisor, mask; spillall;
	lda	divisor,tdivisor
	lda	__TIMER_MASK1,mask
	shlo	1,tdivisor,tdivisor
	shro	tdivisor,mask,tdivisor
	lda	__TIMER_MASK2,mask
	and	mask,tdivisor,tdivisor
	ld	__TIMER_TMR0,ttimer
	lda	__TIMER_MASK3,mask
	and	mask,ttimer,ttimer
	or	tdivisor,ttimer,ttimer
	st	ttimer,__TIMER_TMR0
% void return; const(1) timer; const divisor; \
  tmpreg ttimer, tdivisor, mask; spillall;
	lda	divisor,tdivisor
	lda	__TIMER_MASK1,mask
	shlo	1,tdivisor,tdivisor
	shro	tdivisor,mask,tdivisor
	lda	__TIMER_MASK2,mask
	and	mask,tdivisor,tdivisor
	ld	__TIMER_TMR1,ttimer
	lda	__TIMER_MASK3,mask
	and	mask,ttimer,ttimer
	or	tdivisor,ttimer,ttimer
	st	ttimer,__TIMER_TMR1
% void return; tmpreg timer, divisor, mask, addr; spillall;
	lda	__TIMER_MASK1,mask
	shlo	1,divisor,divisor
	shro	divisor,mask,divisor
	lda	__TIMER_MASK2,mask
	and	mask,divisor,divisor
	lda	__TIMER_TMR0,addr
	lda	(addr)[timer*16],addr
	ld	(addr),timer
	lda	__TIMER_MASK3,mask
	and	mask,timer,timer
	or	divisor,timer,timer
	st	timer,(addr)
}

/* Get timer 0 or 1 mode register */

__asm unsigned int timer_get_mode_register(unsigned int timer)
{
% tmpreg return; const(0) timer; spillall;
	ld	__TIMER_TMR0,return
% tmpreg return; const(1) timer; spillall;
	ld	__TIMER_TMR1,return
% tmpreg return; tmpreg timer, addr; spillall;
	lda	__TIMER_TMR0,addr
	ld	(addr)[timer*16],return
}

#endif

#endif
