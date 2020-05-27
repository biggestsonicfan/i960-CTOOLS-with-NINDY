
/*(c************************************************************************* *
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
 *****************************************************************************c)*/

/*********************************************************************
 *
 * gver960:
 *
 * This program executes each of the tools in $G960BASE/bin and
 * $G960BASE/lib interrogating its 960 version string:
 * If G960BASE is not set, and the base directory is not given on
 * the command line, then I960BASE is used.
 *
 *********************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(DOS) && (defined(WIN95) || defined(__HIGHC__))
#if defined(WIN95)
#define S_ISREG(m)  (((m) & _S_IFMT) == _S_IFREG)
#endif
#include <process.h>  /* needed for _spawnvp() */
#endif
#include "i_toolib.h"

extern char* getenv();

/*
 On dos, some tools are named differently than from UNIX.
*/

#ifdef DOS
#define CC1 "cc1"
#define CC1PLUS "cc1plus"
#define CPP "cpp"
#define GSTRIP "gstrip96"
static char extension[] = ".exe";
#else
#define CC1 "cc1.960"
#define CC1PLUS "cc1plus.960"
#define CPP "cpp.960"
#define GSTRIP "gstrip960"
static char extension[] = "";
#endif

struct tool_descr {
	char	*subdir;	/* Currently, either "bin" or "lib" */
	char	*name;
	char	*version_opt;	/* Generally -v960 */
} tool_table[] = {
	{ "bin",	"arc960",	"-v960" },
	{ "bin",	"asm960",	"-v960" },
	{ "bin",	"cof960",	"-v960" },
	{ "bin",	"comm960",	"-v960" },
	{ "bin",	"cvt960",	"-v960" },
	{ "bin",	"dmp960",	"-v960" },
	{ "bin",	"gar960",	"-v960" },
	{ "bin",	"gas960",	"-v960" },
	{ "bin",	"gas960c",	"-v960" },
	{ "bin",	"gas960e",	"-v960" },
	{ "bin",	"gcc960",	"-v960" },
	{ "bin",	"gcdm960",	"-v960" },
	{ "bin",	"gcov960",	"-v960" },
	{ "bin",	"gdb960",	"-v960" },
	{ "bin",	"gdmp960",	"-v960" },
	{ "bin",	"ghist960",	"-v960" },
	{ "bin",	"gld960",	"-v960" },
	{ "bin",	"gmpf960",	"-v960" },
	{ "bin",	"gmung960",	"-v960" },
	{ "bin",	"gnm960",	"-v960" },
	{ "bin",	"grom960",	"-v960" },
	{ "bin",	"gsize960",	"-v960" },
	{ "bin",	GSTRIP,		"-v960" },
	{ "bin",	"gver960",	"-v960" },
	{ "bin",	"ic960",	"-v960" },
	{ "bin",	"lnk960",	"-v960" },
	{ "bin",	"mpp960",	"-v960" },
	{ "bin",	"nam960",	"-v960" },
	{ "bin",	"objcopy",	"-v960" },
	{ "bin",	"rom960",	"-v960" },
	{ "bin",	"siz960",	"-v960" },
	{ "bin",	"str960",	"-v960" },
	{ "lib",	CC1,		"-v960" },
	{ "lib",	CC1PLUS,	"-v960" },
	{ "lib",	CPP,		"-v960" },
	{ NULL,		NULL,		NULL, }
};

static void
usage(progname)
char	*progname;
{
	(void) printf("\nUsage:\t%s [-v960] [-h] [-v] [basedir]\n", progname);
	(void) printf("\nDisplay Product Version Information\n\n");

	(void) printf("-v960     display version of %s and exit\n", progname);
	(void) printf("-h        display this usage information and exit\n");
	(void) printf("-v        display invocations used to extract version information\n");
	(void) printf("basedir   product base directory.  G960BASE/I960BASE are defaults\n");

	(void) printf("\n");
}
  
main(argc,argv)
    int argc;
    char *argv[];
{
	char			*base = NULL;	/* 'effective' G960BASE */
	int			verbose = 0;	/* Was -v specified? */
	int			argv_index;
	struct tool_descr	*tool;
	char			*buf;
	struct stat		stat_buf;
	int			status = 0;
	int			num_tools = 0;

	argc = get_response_file(argc, &argv);
	check_v960( argc, argv );

	for (argv_index = 1; argv_index < argc; argv_index++)
	{
		char *cur_opt;

		if (!IS_OPTION_CHAR(argv[argv_index][0]))
		{
			if (base != NULL)
			{
				(void) fprintf(stderr,
		"ERROR -- too many base directories specified\n");
				usage(argv[0]);
				exit(1);
			}
			base = argv[argv_index];
			continue;
		}

		if (argv[argv_index][1] == '\0')
		{
			(void) fprintf(stderr,
				"ERROR -- no option following %c\n",
				argv[argv_index][0]);
			usage(argv[0]);
			exit(1);
		}

		/* Parse all options following the option char */
		for (cur_opt = argv[argv_index] + 1; *cur_opt; cur_opt++)
		{
			switch (*cur_opt)
			{
			case 'h':
				usage(base_name_ptr(argv[0]));
				exit(0);
				break;
			case 'v':
				verbose = 1;
				break;
			default:
				(void) fprintf(stderr,
					"ERROR -- Unrecognized option `-%c'\n",
					*cur_opt);
				usage(argv[0]);
				exit(1);
			}
		}
	}

	if (base == NULL)
	{
		/* Try G960BASE first, to be backwards compatible with
		   versions that didn't need to know about I960BASE.
		 */

		if (((base = getenv("G960BASE")) == NULL || *base == '\0')
		      && ((base = getenv("I960BASE")) == NULL || *base == '\0'))
		{
			(void) fprintf(stderr, "ERROR -- base directory not specified and neither G960BASE nor I960BASE defined\n");
			usage(argv[0]);
			exit(1);
		}
	}

	/* Determine if the effective base is a real directory */

	if (stat(base,&stat_buf) != 0
	    || ((stat_buf.st_mode & S_IFMT) != S_IFDIR))
	{
		(void) fprintf(stderr,
			"ERROR -- invalid product base directory %s\n",base);
		usage(argv[0]);
		exit(1);
	}

	if ((buf = (char*)malloc(strlen(base) + 256)) == NULL)
	{
		(void) fprintf(stderr,"Internal Error:  Out of memory\n");
		exit(1);
	}

	for (tool = &tool_table[0]; tool->name; tool++)
	{
		int	j, Argc;
		char	*Argv[10];

		path_concat(buf, base, tool->subdir);
		path_concat(buf, buf, tool->name);
		if (*extension)
			(void) strcat(buf, extension);

		/* Since we don't know which product (CTOOLS960 or GNU/960)
		   we are interrogating, ignore tools that don't exist.
		 */

		if (stat(buf,&stat_buf) != 0 || !S_ISREG(stat_buf.st_mode))
			continue;

		num_tools++;

		Argc = 0;
		Argv[Argc++] = buf;
		Argv[Argc++] = tool->version_opt;
		Argv[Argc] = NULL;

		if (verbose)
		{
			(void) fprintf(stderr, "%s", Argv[0]);
			for (j = 1; j < Argc; j++)
				(void) fprintf(stderr, " %s", Argv[j]);
			(void) fprintf(stderr, ":\n");
		}

#if defined(DOS)
		if (_spawnvp(P_WAIT, Argv[0], Argv) != 0)
			status = 1;
#else
		for (j = 1; j < Argc; j++)
		{
			(void) strcat(buf, " ");
			(void) strcat(buf, Argv[j]);
		}

		if (system(buf) != 0)
			status = 1;
#endif
	}

	if (num_tools == 0)
	{
		(void) fprintf(stderr,
				"ERROR -- no tools found under %s\n", base);
		status = 1;
	}

	return status;
}
