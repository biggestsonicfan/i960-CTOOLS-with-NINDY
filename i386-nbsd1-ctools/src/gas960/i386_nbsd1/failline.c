

/*
Please see flt_test.sh for information on this file.

Paul Reger

Wed Jan  6 10:28:34 PST 1993
*/

/* $Id: failline.c,v 1.2 1993/09/28 16:58:35 peters Exp $ */

#include <stdio.h>

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

/* Returns 0 if .extended,
           1 if host agrees with first encoding,
	   2 for second encoding and
	   3 for something else. */

unsigned long hostwords[2];

int
cmphost(enc1,enc2,ascii)
    char *enc1,*enc2,*ascii;
{
    unsigned long enc1words[2],enc2words[2],size;
    char *p;

    sscanf(enc1,"%x: %x %x",&size,enc1words,&enc1words[1]);
    sscanf(enc2,"%x: %x %x",&size,enc2words,&enc2words[1]);
    if (p=strstr(ascii,"0f")) {
	hostwords[1] = 0;
	sscanf(p+2,"%f",hostwords);
	size = 4;
    }
    else if (p=strstr(ascii,"0d")) {
	sscanf(p+2,"%lf",hostwords);
	size = 8;
    }
    else
	    return 0;
    SWAPLETOHOST(enc1words,size,0);
    SWAPLETOHOST(enc2words,size,0);
    if (!strncmp(enc1words,hostwords,size))
	    return 1;
    else if (!strncmp(enc2words,hostwords,size))
	    return 2;
    else
	    return 3;
}

fetchline(fname,line1,line2,output)
    char *fname,*line1,*line2;
    FILE *output[4];
{
    int addr,extcnt;
    FILE *f = fopen(fname,"r");

    sscanf(line1,"%x:",&addr);
    addr /= 16;
    for (extcnt=0;!feof(f);) {
	char buff[512];

	fgets(buff,512,f);
	if (strstr(buff,".float") || strstr(buff,".double") ||
	    strstr(buff,".extended"))
		if (extcnt++ == addr) {
		    int i = cmphost(line1,line2,buff);
		    fprintf(output[i],"# %s",line1);
		    fprintf(output[i],"# %s",line2);
		    if (i == 3)
			    fprintf(output[i],"# host encoding: %08x %08x\n",hostwords[0],hostwords[1]);
		    fprintf(output[i],"%s",buff);
		    break;
		}
    }
    fclose(f);    
}

main(argc,argv)
    int argc;
    char *argv[];
{
    FILE *dmp1,*dmp2,*assfile,*output[4];

    if (argc != 8) {
 hoser:
	fprintf(stderr,"Usage: %s gasdmp asmdmp assfile output0 output1 output2 output3\n",argv[0]);
	fprintf(stderr,"output0 is for .extended failures\n");
	fprintf(stderr,"output1 is for failures where gas != asm but gas == host\n");
	fprintf(stderr,"output2 is for failures where gas != asm but asm == host\n");
	fprintf(stderr,"output3 is for failures where gas != asm and neither gas nor asm == host\n");
	exit(1);
    }

#define OPENFAIL(file,name,mode) if (!(file=fopen(name,mode))) {\
				    perror(name);\
				    goto hoser; }
    OPENFAIL(dmp1,argv[1],"r");
    OPENFAIL(dmp2,argv[2],"r");
    OPENFAIL(assfile,argv[3],"r");
    fclose(assfile);
    OPENFAIL(output[0],argv[4],"a");
    OPENFAIL(output[1],argv[5],"a");
    OPENFAIL(output[2],argv[6],"a");
    OPENFAIL(output[3],argv[7],"a");

    while (!feof(dmp1) && !feof(dmp2)) {
	char buff1[128],buff2[128];

	fgets(buff1,128,dmp1);
	fgets(buff2,128,dmp2);
	if (strcmp(buff1,buff2))
		fetchline(argv[3],buff1,buff2,output);
    }
}
