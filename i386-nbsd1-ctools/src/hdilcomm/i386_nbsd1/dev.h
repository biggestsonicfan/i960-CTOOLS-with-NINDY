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
/*
 * $Header: /ffs/p1/dev/src/hdilcomm/common/RCS/dev.h,v 1.5 1995/08/31 20:47:17 cmorgan Exp $$Locker:  $
 */

#ifndef HDI_DEV_H
#define HDI_DEV_H

#ifdef __STDC__
typedef struct {
	int	(*pkt_init)();
	int	(*pkt_term)();
	int	(*pkt_get)(unsigned char *buf, int size, int wait);
	int	(*pkt_put)(const unsigned char *data, int size);
	int	(*pkt_put_once)(const unsigned char *data, int size, int send_seq);
	int	(*pkt_reset)(int reset_time);
	void	(*pkt_signal)();
	int	(*pkt_intr_trgt)();
} PKTDEV;

typedef struct {
	int	(*dev_open)();
	int	(*dev_close)(int fd);
	int	(*dev_read)(int fd, unsigned char *buf, int cnt, int timeout);
	int	(*dev_write)(int fd, const unsigned char *data, int cnt);
#ifdef HOST
	void	(*dev_signal)(int fd);
	int	(*dev_intr_trgt)(int fd);
#endif /* HOST */
#ifdef TARGET
	int	(*dev_baud)(int fd, unsigned long baud);
#endif /* TARGET */
} DEV;


extern void com_pkt_ptr_init(void);
extern const DEV *com_dev(void);
extern void  pkt_protocol_setup(void);

#else /* __STDC__ */

typedef struct {
	int	(*pkt_init)();
	int	(*pkt_term)();
	int	(*pkt_get)();
	int	(*pkt_put)();
	int	(*pkt_put_once)();
	int	(*pkt_reset)();
	void	(*pkt_signal)();
	int	(*pkt_intr_trgt)();
} PKTDEV;

typedef struct {
	int	(*dev_open)();
	int	(*dev_close)();
	int	(*dev_read)();
	int	(*dev_write)();
#ifdef HOST
	void	(*dev_signal)();
	int	(*dev_intr_trgt)();
#endif /* HOST */
#ifdef TARGET
	int	(*dev_baud)();
#endif /* TARGET */
} DEV;

extern void com_pkt_ptr_init();
extern const DEV *com_dev();
extern void  pkt_protocol_setup();
#endif /* __STDC__ */

#ifdef HOST
#ifdef __STDC__
extern int        autobaud(unsigned long baud_rate, int dev_type,
		                   int fd, const DEV *dev);
extern int        _com_check_config(void);
extern const DEV *com_serial_dev(void);
extern const DEV *com_pci_dev(void);
#else
#ifndef _AUTOBAUD_DEFINES
extern int autobaud();
#endif
extern int _com_check_config();
extern const DEV *com_serial_dev();
extern const DEV *com_pci_dev();
#endif
#endif /* HOST */

#ifdef TARGET
#ifndef _AUTOBAUD_DEFINES
extern int autobaud(int fd, const DEV *dev);
#endif
#endif /* TARGET */

/* xltbaud for serial io */
struct baudmap {
	unsigned long baud;
	int code;
};

extern PKTDEV byte_pkt;


#endif /* ! defined HDI_DEV_H */
