/* atof_ieee.c - turn a Flonum into an IEEE floating point number
   Copyright (C) 1987 Free Software Foundation, Inc.

This file is part of GAS, the GNU Assembler.

GAS is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GAS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GAS; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* $Id: atofieee.c,v 1.13 1995/10/12 20:57:22 paulr Exp $ */

#include "as.h"

extern FLONUM_TYPE generic_floating_point_number; /* Flonums returned here. */

#ifndef NULL
#define NULL (0)
#endif

extern char EXP_CHARS[];
				/* Precision in LittleNums. */
#define MAX_PRECISION (6)
#define F_PRECISION (2)
#define D_PRECISION (4)
#define X_PRECISION (6)
#define P_PRECISION (6)

				/* Length in LittleNums of guard bits. */
#define GUARD (12)

static unsigned long mask [] = {
  0x00000000,
  0x00000001,
  0x00000003,
  0x00000007,
  0x0000000f,
  0x0000001f,
  0x0000003f,
  0x0000007f,
  0x000000ff,
  0x000001ff,
  0x000003ff,
  0x000007ff,
  0x00000fff,
  0x00001fff,
  0x00003fff,
  0x00007fff,
  0x0000ffff,
  0x0001ffff,
  0x0003ffff,
  0x0007ffff,
  0x000fffff,
  0x001fffff,
  0x003fffff,
  0x007fffff,
  0x00ffffff,
  0x01ffffff,
  0x03ffffff,
  0x07ffffff,
  0x0fffffff,
  0x1fffffff,
  0x3fffffff,
  0x7fffffff,
  0xffffffff
  };


#ifdef __STDC__
static int gen_to_words(LITTLENUM_TYPE *words, int precision, long exponent_bits);
#else /* __STDC__ */
static int gen_to_words();
#endif /* __STDC__ */

static int bits_left_in_littlenum;
static int littlenums_left;
static LITTLENUM_TYPE *littlenum_pointer;

static int
next_bits (number_of_bits)
     int		number_of_bits;
{
  int			return_value;

  if(!littlenums_left)
  	return 0;
  if (number_of_bits >= bits_left_in_littlenum)
    {
      return_value  = mask [bits_left_in_littlenum] & *littlenum_pointer;
      number_of_bits -= bits_left_in_littlenum;
      return_value <<= number_of_bits;
      if(--littlenums_left) {
	      bits_left_in_littlenum = LITTLENUM_NUMBER_OF_BITS - number_of_bits;
	      littlenum_pointer --;
	      return_value |= (*littlenum_pointer>>bits_left_in_littlenum) & mask[number_of_bits];
      }
    }
  else
    {
      bits_left_in_littlenum -= number_of_bits;
      return_value = mask [number_of_bits] & (*littlenum_pointer>>bits_left_in_littlenum);
    }
  return (return_value);
}


/* Num had better be less than LITTLENUM_NUMBER_OF_BITS */
static void
unget_bits(num)
int num;
{
	if(!littlenums_left) {
		++littlenum_pointer;
		++littlenums_left;
		bits_left_in_littlenum=num;
	} else if(bits_left_in_littlenum+num>LITTLENUM_NUMBER_OF_BITS) {
		bits_left_in_littlenum= num-(LITTLENUM_NUMBER_OF_BITS-bits_left_in_littlenum);
		++littlenum_pointer;
		++littlenums_left;
	} else
		bits_left_in_littlenum+=num;
}

static void
make_invalid_floating_point_number (words)
     LITTLENUM_TYPE *	words;
{
	as_bad("cannot create floating-point number");
	words[0]= ((unsigned)-1)>>1;	/* Zero the leftmost bit */
	words[1]= -1;
	words[2]= -1;
	words[3]= -1;
	words[4]= -1;
	words[5]= -1;
}


/* Round the number per ieee rules. */
static void
ieeeround(n,mb)
    FLONUM_TYPE *n;
    int mb;   /* Mantissa bits. */
{
    int number_of_bits_used = (n->leader - n->low) * 16;
    int i;
    int sticky_bit = 0,least_significant_bit = 0,guard_bit = 0;
    unsigned short *least_significant_word;
    unsigned short least_significant_bit_mask;

    /* Count the bits in the mantissa: */
    for (i=0;(mask[i] & (*n->leader)) != *n->leader;i++)
	    ;
    number_of_bits_used += i;

    /* If the number of bits in the mantissa is less than that of the
       format we are computing (single double or extended), then the
       bits are 0 and rounding does not change the result. */
    if (number_of_bits_used >= (mb+1)) {
	/* Fetch the least significant bit: */
	int least_significant_ordinal = number_of_bits_used - (mb+1);
	least_significant_word = n->low + (least_significant_ordinal / 16);
	least_significant_bit_mask = (1 << (least_significant_ordinal % 16));
	least_significant_bit = (least_significant_bit_mask & (*least_significant_word)) ?
		least_significant_bit_mask : 0;

	/* Again, if the mantissa does not have a guard bit in it, it is
	   zero. */
	if (number_of_bits_used >= (mb+2)) {
	    /* Fetch the guard bit: */
	    int guard_bit_ordinal = number_of_bits_used - (mb+2);
	    unsigned short * guard_bit_word = n->low + (guard_bit_ordinal / 16);
	    unsigned short guard_bit_mask = (1 << (guard_bit_ordinal % 16));
	    guard_bit = (guard_bit_mask & (*guard_bit_word)) ? 1 : 0;

	    /* The sticky_bit is set if any of the calculations to
	       obtain the mantissa were inexact or if any of the
	       remaining bits in the mantissa (past the guard bit) are
	       set. */
	    if (number_of_bits_used >= (mb+3)) {
		unsigned short *usp;

		/* Calculate the sticky_bit. */
		for(usp=n->low;!sticky_bit && usp < guard_bit_word;usp++)
			if (*usp)
				sticky_bit = 1;
		if (!sticky_bit)
			sticky_bit = ((guard_bit_mask-1) & (*guard_bit_word)) ? 1 : 0;
	    }
	}
	else
		return;
    }
    else
	    return;

    /* We round under the following two cases: */
    if ((guard_bit && sticky_bit) ||
	(least_significant_bit && guard_bit && !sticky_bit)) {
	unsigned short *usp,*temp = n->low,carry = least_significant_bit_mask;

	/* Add one to the least significant bit. */
	for (usp=least_significant_word;carry && usp <= n->high;usp++) {
	    unsigned int psum = carry + *usp;
	    *usp = psum % 65536;
	    carry = psum / 65536;
	    if (*usp && usp > n->leader)
		    temp = usp;
	}

	/* If carry then we have overflowed the mantissa. */
	if (carry) {
	    unsigned short *usp1;

	    n->exponent++;
	    /* Shift the bits down one digit. */
	    for (usp1=n->low;usp1 < n->high;usp1++)
		    *usp1 = *(usp1+1);
	    n->leader = n->high;
	    *n->high = carry;
	}
	else if (temp > n->leader)
		n->leader = temp;
    }
    else
	    return;
}

/***********************************************************************\
*	Warning: this returns 16-bit LITTLENUMs. It is up to the caller	*
*	to figure out any alignment problems and to conspire for the	*
*	bytes/word to be emitted in the right order. Bigendians beware!	*
*									*
\***********************************************************************/

/* Note that atof-ieee always has X and P precisions enabled.  it is up
   to md_atof to filter them out if the target machine does not support
   them.  */

char *				/* Return pointer past text consumed. */
atof_ieee (str, what_kind, words)
     char *		str;	/* Text to convert to binary. */
     char		what_kind; /* 'd', 'f', 'g', 'h' */
     LITTLENUM_TYPE *	words;	/* Build the binary here. */
{
	static LITTLENUM_TYPE	bits [MAX_PRECISION + MAX_PRECISION + GUARD];
				/* Extra bits for zeroed low-order bits. */
				/* The 1st MAX_PRECISION are zeroed, */
				/* the last contain flonum bits. */
	char *		return_value;
	int		precision; /* Number of 16-bit words in the format. */
	long	exponent_bits;
	long    mantissabits;

	return_value = str;
	generic_floating_point_number.low	= bits;
	generic_floating_point_number.high	= NULL;
	generic_floating_point_number.leader	= NULL;
	generic_floating_point_number.exponent	= 0;
	generic_floating_point_number.sign	= '\0';

				/* Use more LittleNums than seems */
				/* necessary: the highest flonum may have */
				/* 15 leading 0 bits, so could be useless. */

	bzero (bits, sizeof(LITTLENUM_TYPE) * MAX_PRECISION);

	switch(what_kind) {
	case 'f':
	case 'F':
	case 's':
	case 'S':
                mantissabits = 23;
		precision = F_PRECISION;
		exponent_bits = 8;
		break;

	case 'd':
	case 'D':
	case 'r':
	case 'R':
	        mantissabits = 52;
		precision = D_PRECISION;
		exponent_bits = 11;
		break;

	case 'x':
	case 'X':
	case 'e':
	case 'E':
 	        mantissabits = 63;   /* This might be buggy because there is an
 				    implied 1 in extended precision and I do not
 				    know whether the code below will
 				    handle this assumption or not.
				    PS: I tried 63 and 62 on some simple examples
				    and did not notice any change. */
		precision = X_PRECISION;
		exponent_bits = 15;
		break;

	case 'p':
	case 'P':
		
		precision = P_PRECISION;
		exponent_bits= -1;
		break;

	default:
		make_invalid_floating_point_number (words);
		return NULL;
	}

	generic_floating_point_number.high = generic_floating_point_number.low + precision - 1 + GUARD;

	if (atof_generic (& return_value, ".", EXP_CHARS, & generic_floating_point_number)) {
		/* as_bad("Error converting floating point number (Exponent overflow?)"); */
		make_invalid_floating_point_number (words);
		return NULL;
	}
	ieeeround(& generic_floating_point_number, mantissabits);
	gen_to_words(words, precision, exponent_bits);
	return return_value;
}

/* Turn generic_floating_point_number into a real float/double/extended */
static int gen_to_words(words, precision, exponent_bits)
LITTLENUM_TYPE *words;
int precision;
long exponent_bits;
{
	int return_value=0;

	long	exponent_1;
	long	exponent_2;
	long	exponent_3;
	long	exponent_4;
	int		exponent_skippage;
	LITTLENUM_TYPE	word1;
	LITTLENUM_TYPE *	lp;

	if (generic_floating_point_number.low > generic_floating_point_number.leader) {
		/* 0.0e0 seen. */
		if(generic_floating_point_number.sign=='+')
			words[0]=0x0000;
		else
			words[0]=0x8000;
		bzero (&words[1], sizeof(LITTLENUM_TYPE) * (precision-1));
		return return_value;
	}

	/* QNaN:  Do the right thing */
	if(generic_floating_point_number.sign=='Q') {
	    if(precision==F_PRECISION) {
		words[0]=0x7fff;
		words[1]=0xffff;
	    } else if (precision == D_PRECISION) {
		words[0]=0x7fff;
		words[1]=0xffff;
		words[2]=0xffff;
		words[3]=0xffff;
	    }
	    else {
		words[0]=0x7fff;
		words[1]=0xffff;
		words[2]=0xffff;
		words[3]=0xffff;
		words[4]=0xffff;
	    }
	    return return_value;
	}
	/* SNaN:  Do the right thing */
	if(generic_floating_point_number.sign=='S') {
	    if(precision==F_PRECISION) {
		words[0]=0x7f80;
		words[1]=0xffff;
	    }
	    else if (precision == D_PRECISION) {
		words[0]=0x7ff0;
		words[1]=0xffff;
		words[2]=0xffff;
		words[3]=0xffff;
	    }
	    else {
		words[0]=0x7fff;
		words[1]=0x8000;
		words[2]=0xffff;
		words[3]=0xffff;
		words[4]=0xffff;
	    }
	    return return_value;
	}
	else if(generic_floating_point_number.sign=='P') {
	    /* +INF:  Do the right thing */
	    if(precision==F_PRECISION) {
		words[0]=0x7f80;
		words[1]=0;
	    }
	    else if (precision == D_PRECISION) {
		words[0]=0x7ff0;
		words[1]=0;
		words[2]=0;
		words[3]=0;
	    }
	    else {
		words[0]=0x7fff;
		words[1]=0x8000;
		words[2]=0x0;
		words[3]=0x0;
		words[4]=0x0;
	    }
	    return return_value;
	}
	else if(generic_floating_point_number.sign=='N') {
	    /* Negative INF */
	    if(precision==F_PRECISION) {
		words[0]=0xff80;
		words[1]=0x0;
	    }
	    else if (precision == D_PRECISION) {
		words[0]=0xfff0;
		words[1]=0x0;
		words[2]=0x0;
		words[3]=0x0;
	    }
	    else {
		words[0]=0xffff;
		words[1]=0x8000;
		words[2]=0x0;
		words[3]=0x0;
		words[4]=0x0;
	    }
	    return return_value;
	}
	/*
	 * The floating point formats we support have:
	 * Bit 15 is sign bit.
	 * Bits 14:n are excess-whatever exponent.
	 * Bits n-1:0 (if any) are most significant bits of fraction.
	 * Bits 15:0 of the next word(s) are the next most significant bits.
	 *
	 * So we need: number of bits of exponent, number of bits of
	 * mantissa.
	 */
	bits_left_in_littlenum = LITTLENUM_NUMBER_OF_BITS;
	littlenum_pointer = generic_floating_point_number.leader;
	littlenums_left = 1+generic_floating_point_number.leader - generic_floating_point_number.low;
	/* Seek (and forget) 1st significant bit */
	for (exponent_skippage = 0;! next_bits(1); exponent_skippage ++)
		;
	
	exponent_1 = generic_floating_point_number.exponent + generic_floating_point_number.leader + 1 -
 generic_floating_point_number.low;
	/* Radix LITTLENUM_RADIX, point just higher than generic_floating_point_number.leader. */
	exponent_2 = exponent_1 * LITTLENUM_NUMBER_OF_BITS;
	/* Radix 2. */
	exponent_3 = exponent_2 - exponent_skippage;
	/* Forget leading zeros, forget 1st bit. */
	exponent_4 = exponent_3 + ((1 << (exponent_bits - 1)) - 2);
	/* Offset exponent. */

	lp = words;

	/* Word 1. Sign, exponent and perhaps high bits. */
	word1 =   (generic_floating_point_number.sign == '+') ? 0 : (1<<(LITTLENUM_NUMBER_OF_BITS-1));

	/* Assume 2's complement integers. */
	if(exponent_4<1 && exponent_4>=-63) {   /* These are the de-normalized numbers. */
		int prec_bits;
		int num_bits;

		unget_bits(1);
		num_bits= -exponent_4;
		prec_bits=LITTLENUM_NUMBER_OF_BITS*precision-(exponent_bits+1+num_bits);
		if(precision==X_PRECISION && exponent_bits==15)
			prec_bits-=LITTLENUM_NUMBER_OF_BITS+1;
		if(precision == X_PRECISION ||
		   num_bits>=LITTLENUM_NUMBER_OF_BITS-exponent_bits) {
			/*
			  This code is executed if the first short word does not have
			  any mantissa bits in it.
			  */
			num_bits-=(LITTLENUM_NUMBER_OF_BITS-1)-exponent_bits;
			*lp++=word1;					 
			if(num_bits+exponent_bits+1>=precision*LITTLENUM_NUMBER_OF_BITS) {
				/* Exponent overflow */
				make_invalid_floating_point_number(words);
				return return_value;
			}
			if(precision==X_PRECISION && exponent_bits==15) {
			    num_bits += (num_bits != 63);  /* This goodie is here because X_PRECISIONS
							      are off-by-one iffi num_bits != 63
							      (the most denormal number). */
			}
			while(num_bits>=LITTLENUM_NUMBER_OF_BITS) {
			    num_bits-=LITTLENUM_NUMBER_OF_BITS;
			    *lp++=0;
			}
			if(num_bits)
				*lp++=next_bits(LITTLENUM_NUMBER_OF_BITS-(num_bits));
		} else {
		    word1|= next_bits ((LITTLENUM_NUMBER_OF_BITS-1) - (exponent_bits+num_bits));
		    *lp++=word1;
		}
		while(lp<words+precision)
			*lp++=next_bits(LITTLENUM_NUMBER_OF_BITS);
		return return_value;
	} else 	if (exponent_4 & ~ mask [exponent_bits]) {
			/*
			 * Exponent overflow. Lose immediately.
			 */

			/*
			 * We leave return_value alone: admit we read the
			 * number, but return a floating exception
			 * because we can't encode the number.
			 */
		make_invalid_floating_point_number (words);
		return return_value;
	} else {
		word1 |=  (exponent_4 << ((LITTLENUM_NUMBER_OF_BITS-1) - exponent_bits))
			| next_bits ((LITTLENUM_NUMBER_OF_BITS-1) - exponent_bits);
	}

	* lp ++ = word1;

	if(exponent_bits==15 && precision==X_PRECISION)
	{
		*lp++= 1<<(LITTLENUM_NUMBER_OF_BITS-1)|next_bits(LITTLENUM_NUMBER_OF_BITS-1);
	}

	/* The rest of the words are just mantissa bits. */
	while(lp < words + precision)
		*lp++ = next_bits (LITTLENUM_NUMBER_OF_BITS);

	return (return_value);
}


#ifdef  DEBUG
/* A routine to print a generic fp number */
dump_gen()
{
	int i;
	printf ("Low:\n");
	for (i = 0; i < 6; ++i)
	{
		printf ("%d\t0x%x\n", i, generic_floating_point_number.low[i]);
	}
	printf ("High:\n");
	for (i = 0; i < 6; ++i)
	{
		printf ("%d\t0x%x\n", i, generic_floating_point_number.high[i]);
	}
	printf ("Leader:\n");
	for (i = 0; i < 6; ++i)
	{
		printf ("%d\t0x%x\n", i, generic_floating_point_number.leader[i]);
	}
}
#endif  /* DEBUG */

/* This routine is a real kludge.  Someone really should do it better, but
   I'm too lazy, and I don't understand this stuff all too well anyway
   (JF)
 */
void
int_to_gen(x)
long x;
{
	char buf[20];
	char *bufp;

	sprintf(buf,"%ld",x);
	bufp= &buf[0];
	if(atof_generic(&bufp,".", EXP_CHARS, &generic_floating_point_number))
		as_bad("Error converting number to floating point (Exponent overflow?)");
}

#ifdef TEST
char *
print_gen(gen)
FLONUM_TYPE *gen;
{
	FLONUM_TYPE f;
	LITTLENUM_TYPE arr[10];
	double dv;
	float fv;
	static char sbuf[40];

	if(gen) {
		f=generic_floating_point_number;
		generic_floating_point_number= *gen;
	}
	gen_to_words(&arr[0],4,11);
	bcopy(&arr[0],&dv,sizeof(double));
	sprintf(sbuf,"%x %x %x %x %.14G   ",arr[0],arr[1],arr[2],arr[3],dv);
	gen_to_words(&arr[0],2,8);
	bcopy(&arr[0],&fv,sizeof(float));
	sprintf(sbuf+strlen(sbuf),"%x %x %.12g\n",arr[0],arr[1],fv);
	if(gen)
		generic_floating_point_number=f;
	return sbuf;
}
#endif
