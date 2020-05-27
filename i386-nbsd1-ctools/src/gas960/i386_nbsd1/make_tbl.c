
/*

This file is used to automatically create the file fp_const.c  To do so:

compile with an ansic compiler and run.

*/

/* $Id: make_tbl.c,v 1.5 1993/09/28 17:00:56 peters Exp $ */

#include "flonum.h"

#define MAX 13
#define BIG 8000
#define BASE 65536

void fix_flonum(FLONUM_TYPE *n)
{
    unsigned short *p;

    for (p=n->low;*p == 0 && p <= n->leader;p++,n->exponent++)
	    ;
    for (p=n->leader;*p == 0 && p > n->low;p--,n->leader--)
	    ;
    if (*n->leader == 0) {
	n->exponent = 0;
	n->leader = n->low;
    }
    else {
	unsigned short *p2;

	for (p2=n->low;*p2 == 0;p2++)
		;
	for (p=n->low;p2 <= n->leader;p2++,p++)
		*p = *p2;
	n->leader = p - 1;
    }
}

int flonum_zero(FLONUM_TYPE *);
void flonum_copy(FLONUM_TYPE *in,FLONUM_TYPE *out);
void flonum_shift_and_add(FLONUM_TYPE *a,unsigned short b);
void print_flonum(FLONUM_TYPE *n,int i);
int flonum_add(FLONUM_TYPE *a,FLONUM_TYPE *b,FLONUM_TYPE *sum);
int flonum_subtract(FLONUM_TYPE *a,FLONUM_TYPE *b,FLONUM_TYPE *difference);
void int_to_flonum(FLONUM_TYPE *n,int i);
void flonum_to_int(FLONUM_TYPE *n,int *ip);
void flonum_divide(FLONUM_TYPE *n,FLONUM_TYPE *d,FLONUM_TYPE *q,FLONUM_TYPE *r);
void flonum_reciprocal(FLONUM_TYPE *a,FLONUM_TYPE *r);

flonum_mult(FLONUM_TYPE *a,FLONUM_TYPE *b,FLONUM_TYPE *product)
{
    int i,j,carry = 0;
    int length_of_a = a->leader - a->low + 1;
    int length_of_b = b->leader - b->low + 1;
    int length_of_product = product->high - product->low + 1;
    unsigned short *usp;
    int length_of_real_product = 0;

    memset(product->low,0,2*(product->high - product->low + 1));
    if (flonum_zero(a) || flonum_zero(b)) {
	product->exponent = 0;
	product->sign = '+';
	product->leader = product->low;
	return;
    }

    if (!(usp=(unsigned short *)malloc(2*(length_of_a+length_of_b)))) {
	printf("malloc failure\n");
	exit(1);
    }
    product->exponent = a->exponent + b->exponent;
    product->sign = (a->sign == b->sign) ? '+' : '-';
    memset(usp,0,2*(length_of_a+length_of_b));
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
    for (i=0;i < length_of_product;i++) {
 again:
	if (usp[i] == 0) {
	    product->exponent++;
	    for (j=i;j < length_of_real_product-1;j++)
		    usp[j] = usp[j+1];
	    length_of_real_product--;
	    goto again;
	}
	else
		break;
    }
    if (length_of_real_product <= length_of_product) {
	for (i=0;i < length_of_real_product;i++)
		product->low[i] = usp[i];
	product->leader = &product->low[length_of_real_product-1];
    }
    else {
	for (i=length_of_real_product-1,j=product->high-product->low;
	     j >= 0;j--,i--)
		product->low[j] = usp[i];
    }
    free(usp);
    return;
}

void flonum_copy(FLONUM_TYPE *in,FLONUM_TYPE *out)
{
    unsigned short *inp,*outp;
    int length_of_in = in->leader - in->low + 1;
    int length_of_out = out->high - out->low + 1;

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

int
flonum_zero(FLONUM_TYPE *n)
{
    return (n->leader == n->low) && (*n->leader == 0);
}

void flonum_shift_and_add(FLONUM_TYPE *a,unsigned short b)
{
    FLONUM_TYPE t,t2;
    unsigned short ts[BIG];
    unsigned short ts2[BIG];

    t.low = t.leader = ts;
    t.high = &ts[(BIG-1)];
    t2.low = t2.leader = ts2;
    t2.high = &ts2[(BIG-1)];
    int_to_flonum(&t,b);
    if (!flonum_zero(a))
	    a->exponent++;
    flonum_add(a,&t,&t2);
    fix_flonum(&t2);
    flonum_copy(&t2,a);
}

static int sizesplus[MAX+1],sizesminus[MAX+1],exponentsplus[MAX+1],
               exponentsminus[MAX+1];

pfn(n)
    FLONUM_TYPE *n;
{
    unsigned short *usp;

    printf("%c ",n->sign);
    for (usp=n->leader;usp >= n->low;usp--)
	    printf("%d ",*usp);
    printf("e %d\n",n->exponent);
}

void print_flonum(FLONUM_TYPE *n,int i)
{
    unsigned short *p;
    int j;

#define ABS(x) (((x) < 0) ? -(x) : (x))

    if (i < 0) {
	sizesminus[-i] = n->leader - n->low + 1;
	exponentsminus[-i] = n->exponent;
    }
    else {
	sizesplus[i] = n->leader - n->low + 1;
	exponentsplus[i] = n->exponent;
    }
	
    printf("static const LITTLENUM_TYPE %s_%d [] = {\n",
	   (i > 0) ? "plus" : "minus",
	   (i > 0) ? i : -i);
    for (p=n->low;p <= n->leader;) {
	for (j=0;j < 10 && p <= n->leader;j++,p++)
		printf(" %5d,",*p);
	printf("\n");
    }
    printf("};\n");
}

wrap_up()
{
    int i;

    printf("const FLONUM_TYPE flonum_positive_powers_of_ten [] = {\n");
    printf("{X zero,X zero,X zero,0,'+'},\n");
    for (i=1;i <= MAX;i++)
	    printf("{X plus_%d,X plus_%d + %d,X plus_%d+%d, %d, '+'},\n",i,
		   i,sizesplus[i]-1,i,sizesplus[i]-1,exponentsplus[i]);
    printf("};\n");
    printf("const FLONUM_TYPE flonum_negative_powers_of_ten [] = {\n");
    printf("{X zero,X zero,X zero,0,'+'},\n");
    for (i=1;i <= MAX;i++)
	    printf("{X minus_%d,X minus_%d + %d,X minus_%d+%d, %d, '+'},\n",i,
		   i,sizesminus[i]-1,i,sizesminus[i]-1,exponentsminus[i]);
    printf("};\n");
}

#define GETWORD(x,y) (((x) >= (y)->low && (x) <= (y)->leader) ? *(x) : 0)
#define PUTWORD(x,y,z) (((x) >= (y)->low && (x) <= (y)->high) ? \
			(*(x)=(z)) : 0)

int
flonum_add(FLONUM_TYPE *a,FLONUM_TYPE *b,FLONUM_TYPE *sum)
{
    unsigned short *pa,*pb,*ps;
    int carry = 0;
    int lengthofa = a->leader - a->low + 1;
    int lengthofb = b->leader - b->low + 1;
    int lengthofsum = sum->high - sum->low + 1;
    int msda = lengthofa + a->exponent;
    int msdb = lengthofb + b->exponent;

    if (a->sign != b->sign)
	    return 0;
    memset(sum->low,0,2*lengthofsum);
    ps = sum->low;

    if (a->exponent <= b->exponent) {
	pa = a->low;
	pb = b->low - (b->exponent - a->exponent);
    }
    else {
	pa = a->low - (a->exponent - b->exponent);
	pb = b->low;
    }

    for (;pb <= b->leader || pa <= a->leader;
	 pa++,pb++,ps++) {
	int psum = GETWORD(pa,a) + GETWORD(pb,b) + carry;
	if (psum >= BASE) {
	    carry = 1;
	    psum -= BASE;
	}
	else
		carry = 0;
	PUTWORD(ps,sum,psum);
    }
    sum->exponent = (a->exponent < b->exponent) ? a->exponent :
	    b->exponent;
    sum->sign = a->sign;
    sum->leader = (ps <= sum->high) ? ps :
	    (printf("precisionlost\n"),sum->high);
    fix_flonum(sum);
    return 1;
}

int
flonum_subtract(FLONUM_TYPE *a,FLONUM_TYPE *b,FLONUM_TYPE *difference)
{
    unsigned short *pa,*pb,*pd;
    int borrow = 0;
    int lengthofa = a->leader - a->low + 1;
    int lengthofb = b->leader - b->low + 1;
    int lengthofdiff = difference->high - difference->low + 1;
    int msda = lengthofa + a->exponent;
    int msdb = lengthofb + b->exponent;

    if (msdb > msda || a->sign != b->sign)
	    return 0;
    if (msda == msdb)
	    for (pb=b->leader,pa=a->leader;
		 pa >= a->low || pb >= b->low;pa--,pb--)
		    if (GETWORD(pb,b) > GETWORD(pa,a))
			    return 0;
		    else if (GETWORD(pa,a) > GETWORD(pb,b))
			    break;

    /* We know a >= b.  We can do the subtraction. */
                 
    memset(difference->low,0,2*lengthofdiff);
    pd = difference->low;

    if (a->exponent <= b->exponent) {
	pa = a->low;
	pb = b->low - (b->exponent - a->exponent);
    }
    else {
	pa = a->low - (a->exponent - b->exponent);
	pb = b->low;
    }

    for (;pb <= b->leader || pa <= a->leader;
	 pa++,pb++,pd++) {
	int diff = GETWORD(pa,a) - (GETWORD(pb,b)+borrow);
	if (diff < 0) {
	    borrow = 1;
	    diff += BASE;
	}
	else
		borrow = 0;
	PUTWORD(pd,difference,diff);
    }

    difference->exponent = (a->exponent < b->exponent) ? a->exponent :
	    b->exponent;
    difference->sign = a->sign;
    difference->leader = (pd <= difference->high) ? pd :
	    (printf("precisionlost\n"),difference->high);
    fix_flonum(difference);
    return 1;
}

int scales[] = { 1,BASE,BASE*BASE };

void
int_to_flonum(FLONUM_TYPE *n,int i)
{
    unsigned short *p;
    n->exponent = 0;
    n->sign = '+';
    for (p=n->low,*p=0;i;p++) {
	*p = i % BASE;
	i /= BASE;
    }
    n->leader = (p > n->low) ? p - 1 : n->low;
    fix_flonum(n);
}

void flonum_to_int(FLONUM_TYPE *n,int *ip)
{
    unsigned short *p;
    *ip = 0;
    for (p=n->leader;p >= n->low;p--)
	    *ip = (*ip * BASE) + *p;
    *ip = *ip * scales[n->exponent];
}

void flonum_divide(FLONUM_TYPE *n,FLONUM_TYPE *d,FLONUM_TYPE *q,FLONUM_TYPE *r)
{
    FLONUM_TYPE t,highd;
    unsigned short ts[BIG],th[BIG];

    int_to_flonum(q,0);
    flonum_copy(n,r);
    t.low = t.leader = ts;
    t.high = &(ts[(BIG-1)]);
    if (!flonum_subtract(n,d,&t))
	    return;
    highd.low = highd.leader = th;
    highd.high = &(th[(BIG-1)]);
    flonum_copy(d,&highd);
    for (highd.exponent++;flonum_subtract(n,&highd,&t);highd.exponent++)
	    ;
    highd.exponent--;
    while (highd.exponent >= d->exponent) {
	FLONUM_TYPE t2;
	unsigned short high = BASE-1,low = 0,mid,t2s[BIG];

	t2.low = t2.leader = t2s;
	t2.high = &(t2s[(BIG-1)]);
	while (high-low > 1) {
	    mid = (high+low)/2;
	    int_to_flonum(&t,mid);
	    flonum_mult(&highd,&t,&t2);
	    if (flonum_subtract(r,&t2,&t))
		    low = mid;
	    else
		    high = mid;
	}
	int_to_flonum(&t,high);
	flonum_mult(&highd,&t,&t2);
	if (flonum_subtract(r,&t2,&t))
		low = high;
	else {
	    int_to_flonum(&t,low);
	    flonum_mult(&highd,&t,&t2);
	}
	if (!flonum_subtract(r,&t2,&t))
		printf("Huh?");
	else
		flonum_copy(&t,r);
	flonum_shift_and_add(q,low);
	highd.exponent--;
    }
}

void flonum_reciprocal(FLONUM_TYPE *a,FLONUM_TYPE *r)
{
    unsigned short onec = 1,oned = 0,remdigs[BIG],*pr,tdigs[BIG];
    FLONUM_TYPE one,q,rem,t;
    int leadingzeros = 0;

    one.low = one.leader = one.high = &onec;
    one.exponent = 0;
    one.sign = '+';
    q.low = q.leader = q.high = &oned;
    q.exponent = 0;
    q.sign = '+';
    rem.low = rem.leader = remdigs;
    rem.high = &remdigs[(BIG-1)];

    flonum_divide(&one,a,&q,&rem);
    while (flonum_zero(&q)) {
	one.exponent++;
	leadingzeros++;
	flonum_divide(&one,a,&q,&rem);
    }
    if (!flonum_zero(&rem)) {
	*r->high = oned;
	pr = r->high-1;
	t.low = t.leader = tdigs;
	t.high = &tdigs[(BIG-1)];
	while (!flonum_zero(&rem) && pr >= r->low) {
	    rem.exponent++;
	    flonum_divide(&rem,a,&q,&t);
	    *pr-- = oned;
	    flonum_copy(&t,&rem);
	}
	if (!flonum_zero(&rem))
		r->leader = r->high;
	else {
	    unsigned short *t;

	    for(pr++,t=r->low;pr <= r->high;pr++,t++)
		    *t = *pr;
	    r->leader = t-1;
	}
    }
    else
	    int_to_flonum(r,oned);
    r->exponent = -(r->leader - r->low + leadingzeros);
}

main()
{
    unsigned short fdigs[BIG];
    unsigned short gdigs[30];
    unsigned short tdigs[BIG];
    FLONUM_TYPE f,g,t;
    int i;

    f.low = fdigs;
    f.high = fdigs+(BIG-1);
    g.low = gdigs;
    g.high = gdigs+29;
    t.low = tdigs;
    t.high = tdigs+(BIG-1);
#if 0
    int_to_flonum(&f,123);
    int_to_flonum(&g,456);
    flonum_mult(&f,&g,&t);
    flonum_to_int(&t,&i);
    printf("%d\n",i);
#else
    int_to_flonum(&f,10);
    for(i=1;i <= MAX;i++) {
	printf("\n/* entry %d */\n",i);
	flonum_copy(&f,&g);
	print_flonum(&g,i);
	flonum_reciprocal(&f,&g);
	print_flonum(&g,-i);
	flonum_mult(&f,&f,&t);
	flonum_copy(&t,&f);
    }
    wrap_up();
#endif
}
