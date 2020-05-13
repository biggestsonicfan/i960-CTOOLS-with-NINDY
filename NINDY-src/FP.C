/******************************************************************/
/* 		Copyright (c) 1989, Intel Corporation

   Intel hereby grants you permission to copy, modify, and 
   distribute this software and its documentation.  Intel grants
   this permission provided that the above copyright notice 
   appears in all copies and that both the copyright notice and
   this permission notice appear in supporting documentation.  In
   addition, Intel grants this permission provided that you
   prominently mark as not part of the original any modifications
   made to this software or documentation, and that the name of 
   Intel Corporation not be used in advertising or publicity 
   pertaining to distribution of the software or the documentation 
   without specific, written prior permission.  

   Intel Corporation does not warrant, guarantee or make any 
   representations regarding the use of, or the results of the use
   of, the software and documentation in terms of correctness, 
   accuracy, reliability, currentness, or otherwise; and you rely
   on the software, documentation and results solely at your own 
   risk.							  */
/******************************************************************/
#include "defines.h"
#include "globals.h"
#include "regs.h"

extern double log10();
extern double pow();

/************************************************/
/* Display Floating Point Registers 		*/
/*						*/
/* this routine will display the last stored	*/
/* fp register values from the global register	*/
/* storage area					*/
/************************************************/
display_fp_regs()
{
	prtf("\n fp0:"); disp_fp_register(REG_FP0);
	prtf("\n fp1:"); disp_fp_register(REG_FP1);
	prtf("\n fp2:"); disp_fp_register(REG_FP2);
	prtf("\n fp3:"); disp_fp_register(REG_FP3);
	prtf("\n");
}

/************************************************/
/* Output a Floating Point number               */
/*                           			*/
/************************************************/
out_fp(fp, precision)
double fp;
int precision;
{
int exponent, whole, fract;
double num;
unsigned char exp_str[LONG];
unsigned char whole_str[LONG];
unsigned char fract_str[LONG];


	if (fp < 0.0) {
		fp = -fp;
		prtf("-");
	}
	errno = FALSE;
	exponent = (int) log10(fp);
	if (errno != FALSE) 
		exponent = 0;
	htoad(exponent, MAXDIGITS, exp_str, TRUE);

	num = fp * (pow(10.0,(double)-exponent));
	whole = (int)num;
	htoad(whole, MAXDIGITS, whole_str, TRUE);

	fract = (int)((num-(double)whole)*pow(10.0,(double)precision));
	htoad(fract, precision, fract_str, FALSE);

	prtf ("%s.%se%s", whole_str, fract_str, exp_str);
}



/************************************************/
/* Display Floating Point       		*/
/*                           			*/
/************************************************/
disp_float( size, nargs, addr, cnt )
int size;	/* INT, LONG, or EXTENDED	*/
int nargs;	/* Number of the following arguments that are valid (1 or 2) */
unsigned *addr;	/* Address of floating point number in memory (required) */
int cnt;	/* Number of numbers to display (optional, default 1)	*/
{
float real;
double long_real;
long double ext_real;
int valid;

	if ( nargs == 1 ){
		cnt = 1;	/* Set default */
	}

	while ( cnt-- ){
		prtf("\n%X : ", addr);
		switch (size) {
		case INT:
			real = *(float *)addr;
			valid = class_f(real);
			addr++;
			if (validate(valid) != ERROR) {
				out_fp((double)real, 4);
			}
			break;
		case LONG:
			long_real = *(double *)addr;
			valid = class_d(long_real);
			addr += 2;
			if (validate(valid) != ERROR) {
				out_fp(long_real, 8);
			}
			break;
		case EXTENDED:
			ext_real = *(long double *)addr;
			valid = class_e(ext_real);
			addr += 3;
			if (validate(valid) != ERROR) {
				out_fp((double)ext_real, 8);
			}
			break;
		}
	}
	prtf( "\n" );
}


/************************************************/
/* Display FP Register                	 	*/
/*                           			*/
/************************************************/
disp_fp_register(num)
int num;	/* index into 'fp_register_set[]' */
{
	if (validate(class_d(fp_register_set[num])) != ERROR){
		out_fp(fp_register_set[num], 6);
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
get_fp_regnum(reg)
char *reg;	/* Name of register	*/
{

	struct tabentry { char *name; int num; };
	static struct tabentry reg_tab[] = {
		"fp0",	NUM_REGS+REG_FP0,
		"fp1",	NUM_REGS+REG_FP1,
		"fp2",	NUM_REGS+REG_FP2,
		"fp3",	NUM_REGS+REG_FP3,
		0,	0
	};
	struct tabentry *tp;


	for (tp=reg_tab; tp->name != 0; tp++) {
		if (!strcmp(reg,tp->name)) {
			return (tp->num);
		}
	}
	return(ERROR);
}

/************************************************/
/* Validate a floating point number 	 	*/
/*                           			*/
/* If anything but a Normal Finite Number,      */
/* print out the type and return ERROR    	*/
/************************************************/
static
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
	return (ERROR);
}
