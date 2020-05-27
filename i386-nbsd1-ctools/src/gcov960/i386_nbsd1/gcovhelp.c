/*(c**************************************************************************** *
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
 ***************************************************************************c)*/
/* $Id: gcovhelp.c,v 1.18 1995/08/31 17:17:42 kevins Exp $ */

#include <stdio.h>
#include "cc_info.h"

void usage()
{
        fprintf (stdout, "\nUsage:  %s [options] [file[=module]]\n", db_prog_name());
}

static char *help_text[] = {
"",
"gcov960 Coverage analyzer",
"",
"options:",
"  -c[p | m | f | s]",
"     Produce coverage analysis on the profiled program.",
"       p   produce a program-level coverage report only",
"       m   produce a module- and program-level coverage report",
"       f   produce a function-, module-, and program-level coverage report",
"       s   produce a source- and module-level coverage report",
"  -r[l | h | m | lh | lm]",
"     Report information about coverage on the profiled program.",
"       l   produce a source listing annotated with execution counts.",
"           When 'l' is specified all output goes to <source>.cov",
"       h   report and/or annotate only the lines that were executed",
"       m   report and/or annotate only the lines that were not executed",
"       lh  Does both l and h above.",
"       lm  Does both l and m above.",
"  -f[<n>]",
"     Display the most or least frequently executed lines.",
"       If the <n> specified is negative, this shows the",
"       -<n> least frequently executed lines.  When -f is",
"       used with -rl, the report is attached to the annotated",
"       listing and only the most or least frequently executed",
"       lines in that particular source file are shown.",
"  -g[h]",
"     Report a call-graph listing of the profiled program.",
"       h   attach to the report an explanation of how to",
"           read the call graph listing.",  
"  -n <profile file>",
"     Compare two profile files. The <profile file> is",
"     compared against the file default.pf or the file",
"     specified in the -p or -iprof options. When used with",
"     the -rh, -rlh, -rm or -rlm options, the line numbers",
"     either hit only by, or missed only by <profile file> are",
"     reported.",
"  -Q<n>",
"     -Q<n> means to ignore hits except from functions whose profiles",
"     are at least <n> accurate.  The default is -Q9, which means to",
"     ignore profile information for all functions except those with",
"     profiles that have not been stretched due to source code changes",
"     since the profile was collected. -Q0 means to use profile",
"     even for functions whose profile information has been completely",
"     estimated.  <n> must be 0-9.  A profile's quality gradually drops",
"     as changes are made to the source code after the profile was",
"     collected.",
"  -Z <database directory>",
"     Use the specified directory as the program database directory.",
"  -I <search directory>",
"     Add the specified directory to the list of paths that gcov960",
"     should use in searching for source files.  By default gcov960",
"     will search only in the current directory for source files.",
"  -iprof <profile file>",
"     Use the profile information contained in <profile file>.",
"     If no <profile file> is specified gcov960 will attempt to",
"     get profile information from the file default.pf.",
"  -q",
"     Suppress display of version and copyright notices.",
"  -t",
"     Truncate displayed names to keep them within column widths.",
"  -C",
"     Include the sum of all the profile counters in the report.",
"  -V",
"     Print the version number.",
"  -v960",
"     Print the version number, then exit.",
"  -h",
"     Print this help message.",
"",
"file[=module]",            
"   Perform coverage only on the specified file or module.  Module",
"   specification is only useful if you have more than 1 file named",
"   <file>, and you wish to distinguish between them.  In such case",
"   the module name is the name of first defined global function or",
"   variable with '$' concatenated onto the end.",
"     For example:",
"       gcov960 -rh prog.c",
"       gcov960 -rl prog.c=main$",
"",
"Please see the user's guide for a complete command-line description",
0,
      };

static char *cg_help_text [] = {
	"",
	"                    Explanation of the Call Graph Listing:",
    "",
	"Function entries:",
    "",
    "     Index     the index of the function in the call graph listing,",
    "               as an aid in locating it.",
    "",
    "     %Cov      the percentage of basic blocks within this ",
    "               function that were executed.",
    "",
    "     Called    the number of times this function is called.",
    "",
    "     Name      the name and index number of this function.",
    "",
    "Parent Listings:",
    "",
    "     Called    the number of times this function is called by this",
    "               parent.",
	"",
    "     Total     the number of times this function was called by all of",
    "               of its parents.",
    "",
    "     Parents   the name and index number of this parent.",
    "               The names of all parent functions are indented, and",
    "               all parents are listed prior to the function entry.",
    "",
    "Children Listings:",
    "",
    "     Called    the number of times this child is called by this function.",
    "",
    "     Total     the number of times this child is called by all functions.",
    "",
    "     Children  the name and index number of this child.",
    "               The names of all chiled functions are indented, and",
    "               all children are listed after the function entry.",
	"",
	NULL
	};


void
gcov960_help()
{

extern void paginator();

	usage();
	paginator(help_text);
	exit (0);
}

/*
 *  Display the explanation to the call-graph output.
 */
void gcov960_help_cg()
{
	int i;
	for (i=0 ; cg_help_text[i] != 0; i++)
    {
		puts( cg_help_text[i] );
	}
}



