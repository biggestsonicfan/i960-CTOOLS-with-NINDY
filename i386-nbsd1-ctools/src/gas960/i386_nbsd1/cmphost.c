

/*
Please see flt_test.sh for information on this source file.

Paul Reger
Thu Jan  7 09:32:12 PST 1993
*/

/* $Id: cmphost.c,v 1.2 1993/09/28 16:58:03 peters Exp $ */

#include <stdio.h>

mybzero(x,y)
    char *x;
    int y;
{
    int i;

    for (i=0;i < y;i++)
	    x[i] = 0;
}

SWAPLETOHOST(x,y,z) 
    char *x;
    int y,z;
{
    short p = 0x0100;

    if (z || *((char*)&p) == 0x1) {
	int i;

	for (i=0;i < y/2;i++) {
	    char t = x[i];
	    x[i] = x[y-i-1];
	    x[y-i-1] = t;
	}
    }
    else {
	/* le host.  Swap bytes in the one or two words. */
	SWAPLETOHOST(x,4,1);
	if (y == 8)
		SWAPLETOHOST(x+4,4,1);
    }
}

/*
#define ABS(x) (((x) > 0) ? (x) : -(x))
*/

#define ABS(x) (x)


print_words(gas,correct,size)
    unsigned int gas[2],correct[2];
{
    int diff = ABS((int) gas[0] - (int) correct[0]);

    printf("gas words: 0x%08x 0x%08x correct words: 0x%08x 0x%08x  difference: %d\n",gas[0],gas[1],
	   correct[0],correct[1],diff);
}

main(argc,argv)
    int argc;
    char *argv[];
{
    FILE *f,*fout;
    unsigned int gascnt = 0,asmcnt = 0,othercnt = 0,counts_cnt = 0,line;
    struct counts {
	char gas_count,asm_count,other_count;
    } counts[200];

    if (argc != 3 || !(f=fopen(argv[1],"r"))) {
 hoser:
	fprintf(stderr,"usage: %s file outputfile\n",argv[0]);
    }

    mybzero(counts,sizeof(counts[0])*200);

    if (fout=fopen(argv[2],"r")) {
	int i;

	for (i=0;!feof(fout);i++)
		fread(&counts[i],sizeof(counts[0]),1,fout);
	fclose(fout);
    }
    else
	    printf("Warning: %s does not exist.  Initializin counts to zero\n",
		   argv[2]);
    fout = fopen(argv[2],"w");
    printf(" Line | Gas | Asm | Other | Floating point\n");
    printf("Number|Count|Count| Count | Number\n");
    printf("------+-----+-----+-------+----------------------------\n");
    line = 3;
    while (!feof(f)) {
	char buff[512];
	unsigned int gaswords[2],asmwords[2],crap,floatwords[2],size;
	
	if (fgets(buff,512,f)) {
	    sscanf(buff,"# %x: %x %x",&crap,gaswords,&gaswords[1]);
	    fgets(buff,512,f);
	    sscanf(buff,"# %x: %x %x",&crap,asmwords,&asmwords[1]);
	    fgets(buff,512,f);
	    switch (buff[0]) {
	case 'd':
		sscanf(&buff[1],"%lf",floatwords);
		size = 8;
		break;
	case 'f':
		floatwords[1] = 0;
		sscanf(&buff[1],"%f",floatwords);
		size = 4;
		break;
	default:
		size = 0;
	    }
	    if (size) {
		SWAPLETOHOST(gaswords,size,0);
		SWAPLETOHOST(asmwords,size,0);
		if (!strncmp(gaswords,floatwords,size))
			gascnt += ++counts[counts_cnt].gas_count;
		else if (!strncmp(asmwords,floatwords,size))
			asmcnt += ++counts[counts_cnt].asm_count;
		else
			othercnt += ++counts[counts_cnt].other_count;
	    }
	    printf(" %4d | %3d | %3d | %5d | %s",line,(int)counts[counts_cnt].gas_count,
	       (int)counts[counts_cnt].asm_count,
	       (int)counts[counts_cnt].other_count,buff);
	    if (size)
		    print_words(gaswords,floatwords);
	    line += 3;
	    fwrite(&counts[counts_cnt++],sizeof(counts[0]),1,fout);
	}
    }
    printf("------+-----+-----+-------+----------------------------\n");
    printf("gascnt: %d asmcnt: %d othercnt: %d\n",gascnt,asmcnt,othercnt);
}
