
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

/*
 * parse.c -- routines parsing a given kind of argument from a passed down string.
 *
 */
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <setjmp.h>
#include "err_msg.h"
#include "rom960.h"

extern jmp_buf parse_err;

#define STRSIZ 256

skip_white(ps)
char **ps;
{
	while (isspace(**ps))
		++*ps;
}

static char _buf[STRSIZ];

char *
get_file_name(ps,prompt)
char **ps;
char *prompt;				/* If no param. && prompt == 0,
					     0 is returned.		*/
{
	int i=0;

	if (!**ps) {
		if (!prompt)
			return ((char *) 0);
		fwrite(prompt,strlen(prompt),1,stderr);
		scanf("%s",_buf); getchar();
		return _buf;
	}

	skip_white(ps);

	if (!**ps) {
		if (!prompt)
			return ((char *) 0);
		fwrite(prompt,strlen(prompt),1,stderr);
		scanf("%s",_buf); getchar();
		return _buf;
	}

	while (!isspace(**ps)) {
		if (!**ps)
			break;
		_buf[i++] = *((*ps)++);
	}
	_buf[i] = 0;
	return _buf;
}

FILE * 
#if defined(MWC)
get_bin_file_w(char **ps, char *prompt)
#else
get_bin_file_w(ps, prompt)
char **ps;
char *prompt;
#endif
{
	FILE * retval;

	get_file_name(ps,prompt);

	if (NULL == (retval = fopen(_buf, FOPEN_WRONLY_TRUNC)))
		/* the next line was commented out and the following one added
		during update to common error handling 9/28/89 -- robertat 
		p_error ("couldn't open %s\n",_buf);
		*/
		{
		error_out(ROMNAME,NOT_OPEN,0,_buf);
		longjmp(parse_err,1);
		}
	return retval;
}

FILE * 
#if defined(MWC)
get_asc_file_w(char **ps, char *prompt)
#else
get_asc_file_w(ps, prompt, ftype)
char **ps;
char *prompt;
#endif
{
	FILE * retval;

	get_file_name(ps,prompt);

	if (NULL == (retval = fopen(_buf, FOPEN_WRONLY_TRUNC_ASC)))
		/* the next line was commented out and the following one added
		during update to common error handling 9/28/89 -- robertat 
		p_error ("couldn't open %s\n",_buf);
		*/
		{
		error_out(ROMNAME,NOT_OPEN,0,_buf);
		longjmp(parse_err,1);
		}
	return retval;
}

FILE * 
#if defined(MWC)
get_bin_file_r(char **ps, char *prompt)
#else
get_bin_file_r(ps, prompt)
char **ps;
char *prompt;
#endif
{
	FILE * retval;
	char *mode;

	get_file_name(ps,prompt);

	if (NULL == (retval = fopen(_buf, FOPEN_RDONLY)))
		/* the next line was commented out and the following one added
		during update to common error handling 9/28/89 -- robertat 
		p_error ("couldn't open %s\n",_buf);
		*/
		{
		error_out(ROMNAME,NOT_OPEN,0,_buf);
		longjmp(parse_err,1);
		}
	return retval;
}

/*
 *	Open file for reading & return name.
 */
FILE * 
#if defined(MWC)
get_bin_file_rname(char **ps, char *prompt, char *namebuf)
#else
get_bin_file_rname(ps, prompt, namebuf)
char **ps;
char *prompt;
char *namebuf;
#endif
{
	FILE * retval;
	char *mode;

	get_file_name(ps,prompt);
	strcpy(namebuf, _buf);

	if (NULL == (retval = fopen(_buf, FOPEN_RDONLY)))
		/* the next line was commented out and the following one added
		during update to common error handling 9/28/89 -- robertat 
		p_error ("couldn't open %s\n",_buf);
		*/
		{
		error_out(ROMNAME,NOT_OPEN,0,_buf);
		longjmp(parse_err,1);
		}
	return retval;
}

FILE * 
#if defined(MWC)
get_bin_file_rw(char **ps, char *prompt)
#else
get_bin_file_rw(ps, prompt)
char **ps;
char *prompt;
#endif
{
	FILE * retval;
	char *mode;

	get_file_name(ps,prompt);

	if (NULL == (retval = fopen(_buf, FOPEN_RDWR)))
		/* the next line was commented out and the following one added
		during update to common error handling 9/28/89 -- robertat 
		p_error ("couldn't open %s\n",_buf);
		*/
		{
		error_out(ROMNAME,NOT_OPEN,0,_buf);
		longjmp(parse_err,1);
		}
	return retval;
}

long 
get_int(ps,prompt,asectname)
    char **ps;
    char *prompt,**asectname;
{
	int i=0;
	long retval = 0;

 tryagain:

	if (!**ps) {
		fwrite(prompt,strlen(prompt),1,stderr);
		scanf("%s",_buf); getchar();
	} else {
		skip_white(ps);
		if (!**ps) {
			fwrite(prompt,strlen(prompt),1,stderr);
			scanf("%s",_buf); getchar();
		} else {

		while (!isspace(**ps)) {
			if (!**ps)
				break;
			_buf[i++] = *((*ps)++);
		}
		_buf[i] = 0;
		}
	}
	if (!strcmp("after",_buf)) {
	    if (asectname)
		    (*asectname) = get_file_name(ps,"Section name:");
	    else {
		fprintf(stderr,"Use of after disallowed in this context.\n");
		goto tryagain;
	    }
	    return 0;
	}
#if !defined(MSDOS) && !defined(MWC)
	if (_buf[0] != '0')
		sscanf(_buf,"%d",&retval);
	else if (_buf[1] == 'x' || _buf[1] == 'X')
		sscanf(_buf+2,"%x",&retval);
	else
		sscanf(_buf,"%o",&retval);
#else
/* The following code may be ANSI standard, but there is a bug in the
current Metaware compiler that returns a 0 from sscanf using %li of the
string being read starts with a zero. Hence, we comment out this
section
   #if defined(MWC)
	/* This conditional compilation should not be necessary. * /
	/* If the compilers we use support the ANSI standard, we * /
	/* should be able to use the following code for each     * /
	/* environment. Note that the format "%i" handles octal, * /
	/* decimal and hex numbers.  (mlb) * /

	if (sscanf(_buf, "%li", &retval) != 1) {
		error_out(ROMNAME,ROM_GET_INT_FAILED,NO_SUB,prompt);
		longjmp(parse_err,1);
	}
  #else
*/
	if (_buf[0] != '0')
		sscanf(_buf,"%ld",&retval);
	else if (_buf[1] == 'x' || _buf[1] == 'X')
		sscanf(_buf+2,"%lx",&retval);
	else
		sscanf(_buf,"%lo",&retval);
#endif
/*
  #endif
*/

	return retval;
}
int get_mode(ps,prompt)
char **ps;
char *prompt;				/* If no param. && prompt == 0,
					     0 is returned.		*/
{
	int i=0;

	if (!**ps) {
		if (!prompt)
			return (0);
		fwrite(prompt,strlen(prompt),1,stderr);
		scanf("%s",_buf); getchar();
		if ((!strcmp(_buf,"MODE32")) || (!strcmp(_buf,"mode32")))
			return(MODE32);
		else if ((!strcmp(_buf,"MODE16")) || (!strcmp(_buf,"mode16")))
			return(MODE16);
		else return (0);
	}

	skip_white(ps);

	if (!**ps) {
		if (!prompt)
			return (0);
		fwrite(prompt,strlen(prompt),1,stderr);
		scanf("%s",_buf); getchar();
		if ((!strcmp(_buf,"MODE32")) || (!strcmp(_buf,"mode32")))
			return(MODE32);
		else if ((!strcmp(_buf,"MODE16")) || (!strcmp(_buf,"mode16")))
			return(MODE16);
		else return (0);
	}
	if ((!strncmp(*ps,"MODE32",6)) || (!strncmp(*ps,"mode32",6)))
		return(MODE32);
	else if ((!strncmp(*ps,"MODE16",6)) || (!strncmp(*ps,"mode16",6)))
		return(MODE16);
	else return (0);
}

