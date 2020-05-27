
/*
Please see flt_test.sh for information on this source file.

Paul Reger
Thu Jan  7 09:33:24 PST 1993
*/


/* $Id: flt_test.c,v 1.5 1993/09/28 16:58:43 peters Exp $ */

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>

char File[20];
int  Sequence_Number;
enum Ftype { poss_bad_floats, floats, poss_bad_doubles, doubles,
		     poss_bad_extendeds, extendeds } File_Type = floats;

extern double drand48();

char *
get_random(t)
    enum Ftype t;
{
    static int exptable[] = {40,38,318,308,4981,4931};
    static char buff[100];
    double mantissa = 1.0+(drand48()*8.0);  /* Will be in the range:
					       [1.0 .. 9.0) */
    char *sign = (drand48() < 0.5) ? "-" : "";
    int exponent = (((drand48() < 0.5) ? -1.0 : 1.0) * drand48())*exptable[t];

    sprintf(buff,"%s%20.19fe%d",sign,mantissa,exponent);
    return buff;
}

makeasmfile(fname,n,ft)
    char *fname;
    int *n;
    enum Ftype *ft;
{
    FILE *f;
#if 1
    static enum Ftype next_state[] = { floats, doubles, doubles,extendeds,extendeds,floats};

#else
    static enum Ftype next_state[] = { floats, poss_bad_doubles, doubles,
					       poss_bad_extendeds, extendeds, poss_bad_floats };
#endif

    sprintf(fname,"%08x.s",*n);
    (*n)++;

    if (f=fopen(fname,"w")) {
	int i;
	static char *directives[] = { ".float",".float",".double",".double",".extended",".extended"};
	static char *prefixes[]   = { "0f",    "0f",    "0d",     "0d",     "0e",       "0e"};
	static char *space_cmds[] = { ".space 12",".space 12",".space 8",".space 8",".space 4",".space 4" };
	static char *images[]     = { "poss_bad_floats", "floats", "poss_bad_doubles", "doubles",
		     "poss_bad_extendeds", "extendeds" };
	time_t t;

	time(&t);
	fprintf(f,"#\n# Written by flt_test\n# At: %s# File_type: %s\n#\n",ctime(&t),images[(int)*ft]);
	fprintf(f,".data\n");
	for (i=0;i < 100;i++) {
	    char *d = get_random(*ft);

	    fprintf(f,".globl x%d\n",i);
	    fprintf(f,"x%d: %s %s%s\n%s\n",i,directives[(int)*ft],prefixes[(int)*ft],d,space_cmds[(int)*ft]);
	}
	fclose(f);
    }
    *ft = next_state[(int)*ft];
}

#include <varargs.h>

int nfailures;

copy_fail(fname,fmt,va_alist)
    char *fname;
    char *fmt;
    va_dcl
{
    char buff[128];
    FILE *f;
    int s;

    if (f=fopen(fname,"a")) {
	fprintf(f,fmt,va_alist);
	fprintf(f,"\n");
	fclose(f);
    }
    else {
	fprintf(stderr,"fopen failed.\n");
	exit(1);
    }
    sprintf(buff,"cp %s failures",fname);
    if (s=system(buff)) {
	fprintf(stderr,"Copy failed returned: %d executed: %s.\n",s,buff);
	exit(1);
    }
    if (nfailures++ > 1000) {
	fprintf(stderr,"Max errors encountered\n");
	exit(1);
    }
}

main()
{

#ifdef DEBUG
#define DEBUG2(a,b) printf(a,b)
#else
#define DEBUG2(a,b) 
#endif

    int i;

    srand48(time(0));
    for(i=0;;i++) {
	int s40,s35;
	char buff[128];

	makeasmfile(File,&Sequence_Number,&File_Type);
	DEBUG2("made file: %s\n",File);
	sprintf(buff,"$GAS960 %s -o gas.o",File);
	s40 = system(buff);
	DEBUG2("gas returns: %d\n",s40);
	sprintf(buff,"$ASM960 %s -o asm.o",File);
	s35 = system(buff);
	DEBUG2("asm returns: %d\n",s35);
	if ((s40 == 0) && (s35 == 0)) {  /* If both assemblers assembled the file w/o incident. */
	    sprintf(buff,"gdmp960 gas.o > gas.dmp");
	    s40 = system(buff);
	    DEBUG2("dmp gas returns: %d\n",s40);
	    sprintf(buff,"gdmp960 asm.o > asm.dmp");
	    s35 = system(buff);
	    DEBUG2("dmp asm returns: %d\n",s35);
	    if ((s40 == 0) && (s35 == 0)) {
		if (s40=system("cmp gas.dmp asm.dmp")) {
		    char buff[128];
		    copy_fail(File,"# cmp returns: %d",s40);
		    sprintf(buff,"./faillines gas.dmp asm.dmp %s failures/f0.s failures/f1.s failures/f2.s failures/f3.s",File);
		    system(buff);
		}
	    }
	    else
		    copy_fail(File,"# dumper exit status 35: %d 40: %d",s35,s40);
	}
	else
		copy_fail(File,"# assembler exit status 35: %d 40: %d",s35,s40);
	unlink(File);
	DEBUG2("done with: %s\n",File);
    }
}
