
/*(c**************************************************************************** *
 * Copyright (c) 1990, 1991, 1992, 1993 Intel Corporation
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
 ***************************************************************************c)*/

/***************************************************************************
 * %Id%
 *
 * This program is used by GNU/960 Makefiles for creation of time/date
 * stamp strings under MS/DOS.  The file is necessary because Code Builder
 * make does not have the functionality used under Unix.
 *
 * The syntax for use is:
 *
 *		verstr <name> <version_file>
 *
 *  where
 *	<name>		is a name that should be included in the string.
 *	<version_file>	is the name of a file that contains the version number.
 *
 *  Output goes to stout.
 *
 *  For example, if the the file '_version' contains "1.2.1a", this invocation:
 *
 *	verstr gar960 _version > ver960.c
 *
 *  will output the following string to ver960.c:
 *
 * 	"gar960 1.2.1a Mon Apr 15 15:09:03 1991";
 *
 ***************************************************************************/

#include <time.h>
#include <stdio.h>

struct tm *newtime;
time_t aclock;

extern void exit(int);

main(argc, argv)
    int argc;
    char *argv[];
{
	FILE *fp;
	char version[100];
	char *date;
	int i;

	if ( argc != 3 ){
		fprintf(stderr,"Usage: %s <name> <version-file>\n",argv[0]);
		exit(1);
	}

	/**************************************************************
	 *  Open the version file and extract the version number.
	 **************************************************************/
	fp = fopen(argv[2], "r");
	if (fp == NULL){ 
		fprintf( stderr, "Can't open '%s'\n", argv[2] );
		exit(1);
	}
	fscanf(fp, "%s", version);
	fclose(fp);

	/*************************************************************
	 *  Get the current date and time.  Translate this into a
	 *  string for printing.  Make sure to remove newline at the
	 *  end of the date string.
	 ************************************************************/
	time(&aclock);
	newtime = localtime(&aclock);
	date = asctime(newtime);
	for (i = 0; date[i] != '\n'; i++) {
		;
	}
	date[i] = '\0';

	/*************************************************************
	 *  Print the appropriate information.
	 ************************************************************/
	printf( "\t\"%s %s, %s\";\n", argv[1], version, date );
	return (0);
}
