#include "as.h"

/* $Id: fp_util.c,v 1.5 1995/03/08 10:43:32 peters Exp $ */

/*

This code replaces the ugmo fp_copy.c and fp_mult.c  It is entirely new as of Wed Jan  6 10:29:18 PST 1993

Paul Reger

*/

/* This is here for testing.  You can switch the base to something
   easier to deal with like 10 and debug. */

#define BASE 65536

static
int
flonum_equal(n,m)
    FLONUM_TYPE *n;
    unsigned short m;
{
    return (n->leader == n->low) && (n->exponent == 0) &&
	    (*n->leader == m);
}

/* This goodie 'normalizes' the flonum. */
static void fix_flonum(n)
    FLONUM_TYPE *n;
{
    unsigned short *p;
    int trailing_zero_cnt;

    /* Remove trailing zeros...  100 e 0 really is 1 e 2.
       (Please see below code also.) */
    for (trailing_zero_cnt=0,p=n->low;*p == 0 && p <= n->leader;
	 trailing_zero_cnt++,p++,n->exponent++)
	    ;

    /* Remove leading zeros...  00100 e 0 really is 100 e 0 */
    for (p=n->leader;*p == 0 && p > n->low;p--,n->leader--)
	    ;
    /* Did it goto zero?  If so make it zero proper retaining previous
       sign for religious reasons. */
    if (*n->leader == 0) {
	n->exponent = 0;
	n->leader = n->low;
    }
    else if (trailing_zero_cnt) {
	/* If we changed 100 e 0 into 1 e 2, then we should shift all
	   the digits down as many... */
	unsigned short *p2;

	for (p2=n->low;*p2 == 0;p2++)
		;
	/* p2 points at the first non zero digit. */
	for (p=n->low;p2 <= n->leader;p2++,p++)
		*p = *p2;
	n->leader = p - 1;
    }
}

/* Rounds n up unconditionally:
   1 0 0 0 e 0 becomes 1 0 0 1 e 0 or:
   1 BASE-1 BASE-1 BASE-1 e 0 becomes 2 0 0 0 e 0 (or 2 e 3)
*/
static void
flonum_addone(n)
    FLONUM_TYPE *n;
{
    unsigned short *usp,carry = 1;

    for (usp=n->low;carry && usp <= n->high;usp++) {
	unsigned long int psum = carry + *usp;
	*usp = psum % BASE;
	carry = psum / BASE;
    }
    if (usp > n->leader) {
	if (usp <= n->high)
		n->leader = usp;
	else if (carry)  /* This better not ever happen or I made some
			    gross logic error. */
		abort();
	else
		n->leader = n->high;
    }
    fix_flonum(n);
}

#ifdef DEBUG

/* This is here for debugging.  In gdb, do:

p pfn(n)

and this will print it out in logical format. */

void
pfn(n)
    FLONUM_TYPE *n;
{
    unsigned short *usp;

    printf("%c ",n->sign);
    for (usp=n->leader;usp >= n->low;usp--)
	    printf("%d ",*usp);
    printf("e %d\n",n->exponent);
}
#endif

/*
Computes:

product = a * b

Rounds product on inexact cases.
*/
void
flonum_multip(a,b,product)
    FLONUM_TYPE *a,*b,*product;
{
    int i,j,carry                     = 0;
    int length_of_a                   = a->leader - a->low + 1;
    int length_of_b                   = b->leader - b->low + 1;
    int length_of_product             = product->high - product->low + 1;
    static unsigned short *usp        = (unsigned short *) 0;
    static int length_of_usp          = 0;
    int length_of_real_product        = 0;

    /* Zero the result: */
    memset(product->low,0,2*length_of_product);
    if (flonum_equal(a,0) || flonum_equal(b,0)) { /* 0*b == a*0 == 0 */
	product->exponent = 0;
	product->sign = '+';
	product->leader = product->low;
	return;
    }
    else if (flonum_equal(a,1)) {
	flonum_copy(b,product);
	return;
    }
    else if (flonum_equal(b,1)) {
	flonum_copy(a,product);
	return;
    }
    product->exponent = a->exponent + b->exponent;
    product->sign = (a->sign == b->sign) ? '+' : '-';
    /* Set up real product bits.  Product will be calculated first here
       then stuffed into product after it's done. */
    if (length_of_usp < length_of_a+length_of_b) {
	if (length_of_usp == 0)
		usp = (unsigned short *) xmalloc(2*(length_of_usp =
					 (length_of_a+length_of_b)));
	else
		usp = (unsigned short *) xrealloc((char *)usp,2*(length_of_usp =
					 (length_of_a+length_of_b)));
    }
    memset(usp,0,2*length_of_usp);
    for (i=0;&a->low[i] <= a->leader;i++) {
	for (j=0;&b->low[j] <= b->leader;j++) {
	    unsigned long int pproduct = a->low[i]*b->low[j] +
		    usp[i+j] + carry;

	    carry = pproduct / BASE;
	    usp[i+j] = pproduct % BASE;
	}
	if (usp[i+j] += carry)
		length_of_real_product = i+j+1;
	else if (usp[i+j-1] && length_of_real_product < i+j)
		length_of_real_product = i+j;
	carry = 0;
    }
    /* There are no leading zeros in the product by virtue of the above algorithm... */

    /* We now remove trailing zeros from real product ... */
 again:
    if (usp[0] == 0) {
	product->exponent++;
	for (j=0;j < length_of_real_product-1;j++)
		usp[j] = usp[j+1];
	length_of_real_product--;
	goto again;
    }
    /* Does the result have enough space to place real product in?
       if so, plop it in. */
    if (length_of_real_product <= length_of_product) {
	for (i=0;i < length_of_real_product;i++)
		product->low[i] = usp[i];
	product->leader = &product->low[length_of_real_product-1];
    }
    else {
	/* Else we have to futz real product into the result. */
	for (i=length_of_real_product-1,j=product->high-product->low;
	     j >= 0;j--,i--)
		product->low[j] = usp[i];
	/* We have an inexact result. */
	product->exponent += length_of_real_product - length_of_product;
	product->leader = product->high;
	/* We need to round if (guard and sticky) or (lsb and guard but not sticky). */
	if (((usp[i] & 0x8000) && ((usp[i] & 0x7fff) || (i > 0))) ||
	    ((usp[i] == 0x8000) && (*product->low & 1) && (i == 0)))
		flonum_addone(product);
    }
    fix_flonum(product);
}

/*
Assign out the value of in:

out = in
*/
void
flonum_copy(in,out)
    FLONUM_TYPE *in,*out;
{
    unsigned short *inp,*outp;
    int length_of_in  = in->leader - in->low  + 1;
    int length_of_out = out->high  - out->low + 1;

    fix_flonum(in);
    out->sign = in->sign;
    memset(out->low,0,2*length_of_out);
    if (length_of_in > length_of_out) {
	out->exponent = in->exponent + (length_of_in - length_of_out);
	out->leader = out->high;
	for (inp=in->leader,outp=out->high;inp >= in->low && outp >= out->low;
	     inp--,outp--)
		*outp = *inp;
    }
    else {
	out->exponent = in->exponent;
	for (inp=in->low,outp=out->low;inp <= in->leader;inp++,outp++)
		*outp = *inp;
	out->leader = outp-1;
    }
}
