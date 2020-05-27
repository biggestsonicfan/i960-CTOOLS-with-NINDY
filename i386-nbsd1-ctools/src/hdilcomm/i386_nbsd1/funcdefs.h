/*(cb*/
/**************************************************************************
 *
 *     Copyright (c) 1992 Intel Corporation.  All rights reserved.
 *
 *
 * Intel hereby grants you permission to copy, modify, and distribute this
 * software and its documentation.  Intel grants this permission provided
 * that the above copyright notice appears in all copies and that both the
 * copyright notice and this permission notice appear in supporting
 * documentation.  In addition, Intel grants this permission provided that
 * you prominently mark as not part of the original any modifications made
 * to this software or documentation, and that the name of Intel
 * Corporation not be used in advertising or publicity pertaining to the
 * software or the documentation without specific, written prior
 * permission.
 *
 * Intel provides this AS IS, WITHOUT ANY WARRANTY, INCLUDING THE WARRANTY
 * OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, and makes no
 * guarantee or representations regarding the use of, or the results of the
 * use of, the software and documentation in terms of correctness,
 * accuracy, reliability, currentness, or otherwise, and you rely on the
 * software, documentation, and results solely at your own risk.
 *
 **************************************************************************/
/*)ce*/

#include "hdi_com.h"

extern int serial_open();
extern int serial_close(int);
extern int serial_read(int, unsigned char *, int, int);
extern int serial_write(int, const unsigned char *, int);
extern void serial_signal(int);
extern int serial_intr_trgt(int);

extern int init_timer(void);
extern int term_timer(void);
extern int settimeout(unsigned, int *);


#ifdef	__HIGHC__	/* Metaware */

#define	FAR _Far
#define INTR_HANDLER _Far _INTERRPT
#define INP(port) _inb(port)
#define OUTP(port, value) _outb(port, value)

#else			/* Other DOS compiler (Microsoft and clones) */

#define FAR __far
#define INTR_HANDLER __cdecl __interrupt __far

typedef void (INTR_HANDLER *intr_handler)();

extern intr_handler FAR GET_IV(int);
extern void FAR SET_IV(int, intr_handler);
extern int FAR CHANGE_IF(int);
extern int FAR INP(int);
extern void FAR OUTP(int, int);

/* Microsoft C nicely pushes the flags register before calling an
 * interrupt handler. */
#define _chain_intr(prev_handler) (*prev_handler)()

#endif
