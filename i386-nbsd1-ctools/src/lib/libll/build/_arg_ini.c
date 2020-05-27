/*******************************************************************************
 * 
 * Copyright (c) 1993 Intel Corporation
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
 ******************************************************************************/

#include "sdm.h"

#ifndef NULL
#define NULL 0
#endif

char	**environ;

struct main_args {
	int  argc;
	char **argv;
	char **environ;
};

#define MAXARGC 20
#define MAXENVC 30
static char argbuf[1024];
static char *argv[MAXARGC];
static char *envv[MAXENVC];

static void parse_args(struct main_args *args, char *buf);
static unsigned strlen(const char *);    /* defined here to avoid dependencies
					  * on other libraries */

struct main_args
_arg_init()
{
	struct main_args args;

	if (sdm_arg_init(argbuf, sizeof(argbuf)) < 0)
	{
	    static char *argvv[1] = { NULL };
            args.argc = 0;
            args.argv = argvv;
            args.environ = NULL;
	}
	else
	    parse_args(&args, argbuf);

	environ = args.environ;
	return args;
}


#define isspace(c) ((c)==' '||(c)=='\t'||(c)=='\r'||(c)=='\n')

static void
parse_args(struct main_args *args, char *argbuf)
{
	char *cp;
	char *envp;
	int envc;

	envp = argbuf + strlen(argbuf) + 1;

	/* make argc, argv */
	args->argc = 0;
	args->argv = argv;
	cp = argbuf;
	while (*cp && args->argc < MAXARGC-1)
	{
	    while (*cp && isspace(*cp))
		cp++;

	    if (*cp)
	    {
		argv[args->argc++] = cp;
		while (*cp && !isspace(*cp))
		    cp++;

		if (*cp)
		    *cp++ = '\0';
	    }
	}
	argv[args->argc] = NULL;

	/* make environment vars
	 * comes in: string, [..string,] 0
	 */
	envc = 0;
	while (*envp && envc < MAXENVC-1)
	{
		envv[envc++] = envp;
		envp += strlen(envp) + 1;
	}
	envv[envc] = NULL;
	args->environ = envv;
}

static unsigned
strlen(const char *s)
{
    int i = 0;
    while (*s++)
        i++;
    return i;
}
