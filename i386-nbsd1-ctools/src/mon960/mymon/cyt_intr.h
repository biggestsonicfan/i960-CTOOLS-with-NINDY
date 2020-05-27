/*
 * intr.h
 *
 *	Include file for c145_int.c
 *
 */

#include "std_defs.h"
/* Misc defines */
#define FALSE			0
#define TRUE			1


/*
 * Public routines in intr.c
 */
void install_local_handler ();
void init_intr ();
void enable_intr ();
void disable_intr ();

#if KXSX_CPU
/* On board sources directly connected to CPU's XINT pins */
#define IRQ_CIO			0	/* 85C36  CIO ............ XINT0 */
#define IRQ_UART		1	/* 16C550 UART ........... XINT1 */
#define IRQ_SQ_IRQ1		1	/* Squall II, irq pin 1 .. XINT1 */
#define IRQ_PARALLEL	2	/* Parallel Port ......... XINT2 */
#define IRQ_PCI			2	/* PLX 9060 PCI Interface  XINT2 */
#define IRQ_SQ_IRQ0		3	/* Squall II, irq pin 0 .. XINT3 */

#define IRQ_MAX_LOCAL_SRCS	4
#endif /* KXSX */

#if CXHXJX_CPU
/* On board sources directly connected to CPU's XINT pins */
#define IRQ_UART		0	/* 16C550 UART ........... XINT7 */
#define IRQ_CIO			1	/* 85C36  CIO ............ XINT6 */
#define IRQ_SQ_IRQ1		2	/* Squall II, irq pin 1 .. XINT5 */
#define IRQ_SQ_IRQ0		3	/* Squall II, irq pin 0 .. XINT4 */
#define IRQ_PARALLEL	4	/* Parallel Port ......... XINT1 */
#define IRQ_PCI			5	/* PLX 9060 PCI Interface  XINT0 */

#define IRQ_MAX_LOCAL_SRCS	6
#endif /* CXJX */
