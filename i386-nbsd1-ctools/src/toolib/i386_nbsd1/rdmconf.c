
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

/*****************************************************************************
 *
 *
 * This file provides support for reading a memory configuration table from a
 * file.  This is necessary for the runtime support of some 80960CX versions
 * of NINDY (e.g., the CADIC board) which must be told at runtime the bus
 * configuration of memory regions 0-E of the host board on which they are
 * resident. (NINDY is always assumed to be running out of ROM in region F;
 * therefore the bus configuration of this region is known in advance and
 * should never be altered.)
 *
 * The format of the memory configuration table file is:
 *	15 hex numbers representing the values to be loaded into the 80960
 *	processor's MCON0-MCON14 (bus configuration) registers, respectively.
 *	The register values may be separated by any amount of whitespace,
 *	and may have optional leading "0x".
 *
 * Note that a memory configuration table is meaningless in the 80960 KX
 * architecture.
 *****************************************************************************/

#include <stdio.h>
#include <ctype.h>


/*****************************************************************************
 * MODULE GLOBALS
 *****************************************************************************/

#define MAXLINE 250

static FILE *f;			/* File containing table	*/
static char buffer[MAXLINE];	/* Next line from in put file	*/
static char *bp;		/* Pointer into line buffer	*/


/*****************************************************************************
 * next_token:
 *	Find next token (string of characters separated by whitespace) in input
 *	file.  Read new lines from the input file if necessary.
 *
 * RETURNS:
 *	Pointer to a '\0'-terminated token.  NULL if there are no more.
 *
 * ASSUMES:
 *	All module globals have been initialized.
 *
 *****************************************************************************/
static
char *
next_token()
{
	char *tokstart;

	/* Skip leading whitespace.
	 * Read another line from the input file, if necessary.
	 */
	while ( isspace(*bp) || (*bp == '\0') ){
		if ( *bp == '\0' ){
			if ( fgets(buffer,MAXLINE,f) == NULL ){
				return NULL;
			}
			bp = buffer;
		} else {
			bp++;
		}
	}

	/* Remember where the token starts.
	 * Then find it's end and make sure it is '\0'-terminated.
	 */
	tokstart = bp;
	while ( !isspace(*bp) && (*bp != '\0') ){
		bp++;
	}
	*bp++ = '\0';

	return tokstart;
}


/*****************************************************************************
 * bad_token:
 *	Output error message identifying a bad token in the input file
 *	(i.e., one not representing a hex number).
 *
 *****************************************************************************/
static
void
bad_token( tp, fn )
    char *tp;	/* Pointer to token	*/
    char *fn;	/* Name of file		*/
{
	fprintf(stderr,"Bad entry in in file %s: %s\n", fn, tp);
}


/*****************************************************************************
 * read_mem_conf_tab:
 *	Read the memory configuration table from a file, convert to binary,
 *	and store result in indicated table.
 *
 * RETURN:
 *	1 (TRUE) on success, 0 (FALSE) on failure.
 *
 *****************************************************************************/
read_mem_conf_tab( fn, table )
    char *fn;		/* Name of configuration table file	*/
    unsigned long table[]; /* Where to return converted values (15 entries) */
{
	char *tokstart;	/* Pointer to start of token	*/
	char *tp;	/* Pointer into token		*/
	int n;		/* Number of table entries read	*/
	int value;	/* Binary value of token	*/

	/* Open input file and initialize module globals.
	 */
	f = fopen(fn,"r");
	if ( f == NULL ){
		fprintf( stderr, "Can't open file %s\n", fn );
		return 0;
	}
	buffer[0] = 0;
	bp = buffer;

	/* Repeat loop until all tokens have been read from file.
	 */
	n = 0;
	while ( (tp=next_token()) != NULL ){
		if ( n >= 15 ){
			fprintf(stderr,"More than 15 entries in file %s\n", fn);
			return 0;
		}

		tokstart = tp; /* Save start, for error messages */

		/* Skip over optional leading "0x"
		 */
		if ( (tp[0] == '0') && (tp[1] == 'x') ){
			tp += 2;
			if ( *tp == '\0' ){
				bad_token(tokstart,fn);
				return 0;
			}
		}

		if ( strlen(tp) > 8 ){	/* 32-bits max */
			bad_token(tokstart,fn);
			return 0;
		}

		/* Convert ASCII to binary and store in return table
		 */
		for ( value = 0; *tp; tp++ ){
			value <<= 4;
			if ( ('0' <= *tp) && (*tp <= '9') ){
				value += *tp - '0';
			} else if ( ('A' <= *tp) && (*tp <= 'F') ){
				value += *tp - 'A' + 10;
			} else if ( ('a' <= *tp) && (*tp <= 'f') ){
				value += *tp - 'a' + 10;
			} else {
				bad_token(tokstart,fn);
				return 0;
			}
		}
		table[n++] = value;
	}

	if ( n < 15 ){
		fprintf(stderr,"Too few entries in file %s\n", fn);
		return 0;
	}
	return 1;
}
