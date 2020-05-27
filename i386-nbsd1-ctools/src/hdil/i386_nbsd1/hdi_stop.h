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

#ifndef HDI_STOP_H
#define HDI_STOP_H

/* Possible values for mode passed to hdi_trgt_go and go_user in the monitor */
#define GO_RUN	0
#define GO_STEP 1
#define GO_NEXT 2
#define GO_SHADOW 3
#define GO_BACKGROUND 0x80
#define GO_MASK 0x7f		/* Mask for mode values passed on to target */


/* Possible values for stop reason */
#define STOP_RUNNING	0x0000
#define STOP_EXIT	0x0001
#define STOP_BP_SW	0x0002
#define STOP_BP_HW	0x0004
#define STOP_BP_DATA0	0x0008
#define STOP_BP_DATA1	0x0010
#define STOP_TRACE	0x0020
#define STOP_FAULT	0x0040
#define STOP_INTR	0x0080
#define STOP_CTRLC	0x0100
#define STOP_UNK_SYS	0x0200
#define STOP_UNK_BP	0x0400
#define STOP_MON_ENTRY	0x0800

typedef struct {
	unsigned long reason;
	struct {
		unsigned long exit_code;	/* STOP_EXIT */
		ADDR sw_bp_addr;		/* STOP_BP_SW */
		ADDR hw_bp_addr;		/* STOP_BP_HW */
		ADDR da0_bp_addr;		/* STOP_BP_DATA0 */
		ADDR da1_bp_addr;		/* STOP_BP_DATA1 */
		struct {			/* STOP_TRACE */
			unsigned char type;
			ADDR ip;
		} trace;
		struct {			/* STOP_FAULT */
			unsigned char type;
			unsigned char subtype;
			ADDR ip;
			ADDR record;
		} fault;
		unsigned char intr_vector;	/* STOP_INTR */
	} info;
} STOP_RECORD;

#endif /* HDI_STOP_H */
