
/*
Please see flt_test.sh for information on this source file.

Paul Reger
Thu Jan  7 09:32:50 PST 1993
*/

/* $Id: encoding.c,v 1.2 1993/09/28 16:58:18 peters Exp $ */

main(argc,argv)
    int argc;
    char *argv[];
{
    char *s;
    extern double strtod();
    double d = strtod(argv[1],&s);
    int i;

    for (i=0;i < 8;i++)
	    printf("%02x ",(unsigned int) ((unsigned char*)&d)[i]);
    printf("\n");
}
