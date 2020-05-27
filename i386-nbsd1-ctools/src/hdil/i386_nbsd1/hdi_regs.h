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
/****************************************************************************
 * This file defines the layout of the application register set.
 *
 * If the indices of the registers are renumbered, or registers are added,
 * be sure to reflect the changes in the following places, which make
 * assumptions about the organization:
 *
 *	o entry.s: the ".set" directives defining the assembler offsets
 *		into these arrays.
 *
 *	o Any host software which receives these registers.
 *
 *****************************************************************************/

#ifndef HDI_REGS_H
#define HDI_REGS_H

#define NUM_REGS 40
#define NUM_FP_REGS	4

typedef unsigned long REG;
typedef REG UREG[NUM_REGS];

#ifdef HOST
typedef union {
    unsigned char fp80[10];
    struct {
	unsigned char value[8];
	unsigned char flags;
    } fp64;
} FPREG;
#endif

#ifdef TARGET
typedef struct {
    long double fp80;
    struct {
	double value;
	unsigned char flags;
    } fp64;
} FPREG;
#endif

#define FP_80BIT 1
#define FP_64BIT 2

#define	REG_R0		0
#define REG_PFP		REG_R0
#define REG_R1		1
#define REG_SP		REG_R1
#define REG_R2		2
#define	REG_RIP		REG_R2
#define	REG_IP		REG_R2
#define REG_R3		3
#define REG_R4		4
#define REG_R5		5
#define REG_R6		6
#define REG_R7		7
#define REG_R8		8
#define REG_R9		9
#define REG_R10		10
#define REG_R11		11
#define REG_R12		12
#define REG_R13 	13
#define REG_R14		14
#define REG_R15		15
#define	REG_G0		16
#define REG_G1		17
#define REG_G2		18
#define REG_G3		19
#define REG_G4		20
#define REG_G5		21
#define REG_G6		22
#define REG_G7		23
#define REG_G8		24
#define REG_G9		25
#define REG_G10		26
#define REG_G11		27
#define REG_G12		28
#define REG_G13 	29
#define REG_G14		30
#define REG_G15		31
#define REG_FP		REG_G15
#define REG_PC		32
#define REG_AC		33
#define REG_TC		34

#define REG_SF0		35	/* SFR - pending interrupts */
#define REG_SF1		36	/* SFR - interrupt mask register */
#define REG_IPND	REG_SF0
#define REG_IMSK	REG_SF1
#define REG_SF2		37	/* SFR - dma control register CX */
                        /*       of casche ctl regi HX */
#define REG_DMAC	REG_SF2
#define REG_CCON    REG_SF2
#define REG_SF3		38	/* SFR - interrupt control register */
#define REG_SF4		39	/* SFR - GMU control register */
#define REG_ICON    REG_SF3
#define REG_GCON    REG_SF4

#define REG_FP0	0
#define REG_FP1	1
#define REG_FP2	2
#define REG_FP3	3

#endif
