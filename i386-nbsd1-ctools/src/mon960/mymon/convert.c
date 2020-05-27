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

extern strchr();

int hexval(char c);

/************************************************/
/* Integer power function			*/
/*                           			*/
/************************************************/
unsigned int int_pow(number,  power)
int number, power;
{
int i;
unsigned int result;


	if (power == 0)
		return (1);

	result = number;
	for (i=1; i<power; i++) {
		result *= number;
	}
	return (result);
}
/************************************************/
/* Ascii to Decimal Converter    		*/
/*                           			*/
/************************************************/
int
atod(s,n)
char *s;
int *n;
{
int num;

	if ( *s == '\0' ){
		return FALSE;
	}
	for ( num=0; *s; s++ ){
		if ( !strchr("0123456789",*s) ){
			return FALSE;
		}
		num = (num * 10) + (*s-'0');
	}
	*n = num;
	return TRUE;
}

/************************************************/
/* Ascii to Hex Converter    			*/
/*                           			*/
/************************************************/
int
atoh(s,n)
char *s;
int *n;
{
int val;
int h;

	if ( s[0] == '0' && (s[1] == 'x' || s[1] == 'X') ){
		s += 2;
	}

	if ( *s == '\0' ){
		return FALSE;
	}

	for ( val = 0; *s; s++ ){
		if ( (h=hexval(*s)) == ERR) {
			return FALSE;
		}
		val = (val << 4) + h;
	}
	*n = val;
	return TRUE;
}

/************************************************/
/* Hex value of character passed		*/
/*                           			*/
/* This is a lazy way to get the value,but it   */
/* works!                                       */
/************************************************/
int
hexval(char c)
{
	switch (c) {
		case '0': return(0);
		case '1': return(1);
		case '2': return(2);
		case '3': return(3);
		case '4': return(4);
		case '5': return(5);
		case '6': return(6);
		case '7': return(7);
		case '8': return(8);
		case '9': return(9);
		case 'a': 
		case 'A': return(10);
		case 'b': 
		case 'B': return(11);
		case 'c': 
		case 'C': return(12);
		case 'd': 
		case 'D': return(13);
		case 'e': 
		case 'E': return(14);
		case 'f': 
		case 'F': return(15);
		default : return(ERR);
	}
}

/************************************************/
/* Hex to ASCII Decimal Converter      		*/
/*                           			*/
/************************************************/
void
htoad(number, size, string, neg)
int number;			/* number to be converted */
int size;			/* size or precision of number */
unsigned char string[];		/* return string */
int neg;			/* are negatives important? */
				/* if so, no leading 0's */
{
int first,large;
int i, val;
unsigned char *strptr; 

	large = int_pow(10, (size-1));
	first = TRUE;
	strptr = string;

	if (neg) {	/* check for negative numbers if appropriate */
		if (number < 0) {
			number = -number;
			*strptr++ = '-';
		}
	}

	for (i=0; i<size; i++) {
		val = ((number / large)) % 10;  /* get digit */
		large = large / 10;

		/* strip leading 0's if appropriate */
		if (val == 0) {	
			if (first && (i != size-1) && neg) {
				continue;
			}
		}

		else 
			first = FALSE;
		*strptr++ = val + '0';
	}
	*strptr = NUL;
}
