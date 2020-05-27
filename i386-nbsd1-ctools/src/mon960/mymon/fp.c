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

#include "ui.h"
#include "i960.h"
#include "retarget.h"
#include "hdi_arch.h"
#include "hdi_errs.h"

extern int load_mem(ADDR, int mem_size, void *buf, int buf_size);
extern prtf(), perror(), strcmp(), htoad(), class_d(), class_f(), class_e();
extern re_to_rl();

void disp_fp_register(int);

static double pow10(int e);
static validate (int type);

/************************************************/
/* Display Floating Point Registers 		*/
/*						*/
/* this routine will display the last stored	*/
/* fp register values from the global register	*/
/* storage area					*/
/************************************************/
void
display_fp_regs()
{
	if (arch == ARCH_KA || arch == ARCH_SA) return;  /* no fp on chip */

	prtf("fp0: "); disp_fp_register(REG_FP0); prtf("\n");
	prtf("fp1: "); disp_fp_register(REG_FP1); prtf("\n");
	prtf("fp2: "); disp_fp_register(REG_FP2); prtf("\n");
	prtf("fp3: "); disp_fp_register(REG_FP3); prtf("\n");
}

/************************************************/
/* Output a Floating Point number               */
/*                           			*/
/************************************************/
void
out_fp(fp, precision)
double fp;
int precision;
{
	int exponent, whole, fract;
	unsigned char exp_str[64];
	unsigned char whole_str[64];
	unsigned char fract_str[64];

	if (fp < 0.0) {
		fp = -fp;
		prtf("-");
	}

	exponent = 0; 
	if (fp != 0.0)
	{
	    while (fp >= 10.0)
		exponent++, fp /= 10.0;
	    while (fp < 1.0)
		exponent--, fp *= 10.0;
	}

	htoad(exponent, MAXDIGITS, exp_str, TRUE);

	whole = (int)fp;
	htoad(whole, MAXDIGITS, whole_str, TRUE);

	fract = (int)((fp - (double)whole) * pow10(precision));
	htoad(fract, precision, fract_str, FALSE);

	prtf ("%s.%se%s", whole_str, fract_str, exp_str);
}

static double
pow10(int e)
{
    int p = 1.0;

    while (e > 0)
	p *= 10.0, e--;

    while (e < 0)
	p /= 10.0, e++;

    return p;
}



/************************************************/
/* Display Floating Point       		*/
/*                           			*/
/************************************************/
void
disp_float( size, nargs, addr, cnt )
int size;	/* WORD, LONG, or EXTENDED	*/
int nargs;	/* Number of the following arguments that are valid (1 or 2) */
ADDR addr;	/* Address of floating point number in memory (required) */
int cnt;	/* Number of numbers to display (optional, default 1)	*/
{
	const int fetch_size = (size == EXTENDED ? TRIPLE : size);
	const int add_size = (size == EXTENDED ? QUAD : size);
	unsigned int buf[3];
	float real;
	double long_real;

	if (arch == ARCH_KA || arch == ARCH_SA) return;  /* no fp on chip */

	if ( nargs == 1 ){
		cnt = 1;	/* Set default */
	}

	break_flag = FALSE;
	while (cnt-- && !break_flag)
	{
		if (load_mem(addr, fetch_size, &buf, fetch_size) != OK)
		{
		    perror((char *)NULL, 0);
		    break;
		}
		prtf("%X : ", addr);

		switch (size) {
		case WORD:
			real = *(float *)buf;
			if (validate(class_f(real)) != ERR) {
				out_fp((double)real, 4);
			}
			break;
		case LONG:
			long_real = *(double *)buf;
			if (validate(class_d(long_real)) != ERR) {
				out_fp(long_real, 8);
			}
			break;
		case EXTENDED:
			if (validate(class_e(buf)) != ERR) {
				re_to_rl(buf, &long_real);
				out_fp(long_real, 8);
			}
			break;
		}

		addr += add_size;
		prtf("\n");
	}
}


/************************************************/
/* Display FP Register                	 	*/
/*                           			*/
/************************************************/
void
disp_fp_register(num)
int num;	/* index into 'fp_register_set[]' */
{
	if (arch == ARCH_KA || arch == ARCH_SA) return;  /* no fp on chip */

	if (validate(class_e(&fp_register_set[num].fp80)) != ERR){
		double d;
		re_to_rl(&fp_register_set[num].fp80, &d);
		out_fp(d, 6);
	}
}

/************************************************/
/* Get Floating Point Register Number		*/
/*                           			*/
/*	Returns:				*/
/*						*/
/*	- NUM_REGS+(offset into fp_register_set)*/
/*	  if register is a floating pt reg.	*/
/*						*/
/*	- ERROR otherwise			*/
/************************************************/
int
get_fp_regnum(reg)
char *reg;	/* Name of register	*/
{

	struct tabentry { char *name; int num; };
	static struct tabentry reg_tab[] = {
		{"fp0",	NUM_REGS+REG_FP0},
		{"fp1",	NUM_REGS+REG_FP1},
		{"fp2",	NUM_REGS+REG_FP2},
		{"fp3",	NUM_REGS+REG_FP3},
		{0,	0}
	};
	struct tabentry *tp;

	if (arch == ARCH_KA || arch == ARCH_SA) return(ERR);  /* no fp on chip */

	for (tp=reg_tab; tp->name != 0; tp++) {
		if (!strcmp(reg,tp->name)) {
			return (tp->num);
		}
	}
	return(ERR);
}

/************************************************/
/* Validate a floating point number 	 	*/
/*                           			*/
/* If anything but a Normal Finite Number,      */
/* print out the type and return ERROR    	*/
/************************************************/
static int
validate (type)
int type;
{
	char *p;

	switch (type) {
		case 0: p ="0.000000e 0";		break;
		case 1: p ="Denormalized Number";	break;
		case 2: return (0);
		case 3: p ="Infinity";			break;
		case 4: p ="QNaN";			break;
		case 5: p ="SNaN";			break;
		case 6: p ="Reserved Operand";		break;
		default:p ="    * * *   ";		break;
	}
	prtf(p);
	return (ERR);
}
