/******************************************************************/
/*       Copyright (c) 1994 Intel Corporation

   Intel hereby grants you permission to copy, modify, and
   distribute this software and its documentation.  Intel grants
   this permission provided that the above copyright notice
   appears in all copies and that both the copyright notice and
   this permission notice appear in supporting documentation.  In
   addition, Intel grants this permission provided that you
   prominently mark as not part of the original any modifications
   made to this software or documentation, and that the name of
   Intel Corporation not be used in advertising or publicity
   pertaining to distribution of the software or the documentation
   without specific, written prior permission.

   Intel Corporation provides this AS IS, without any warranty,
   including the warranty of merchantability or fitness for a
   particular purpose, and makes no guarantee or representations
   regarding the use of, or the results of the use
   of, the software and documentation in terms of correctness,
   accuracy, reliability, currentness, or otherwise; and you rely
   on the software, documentation and results solely at your own
   risk.                                                          */
/******************************************************************/

/* This module checks for the -c960 option followed by a file name.
 * Such a name is used by some tools to pass commands back to whomever
 * called them to be executed.
 */

#include "i_toolib.h"

static char *cmd_file_name;

/* Start of get_cmd_file() code.
 * The usage model is:
 *
 *	main(argc,argv)
 *	int argc; char *argv[];
 *	{
 *		argc = get_response_file(argc, &argv);
 *              argc = get_cmd_file(argc, argv);
 *	...
 *	}
 */

int
get_cmd_file(argc, argv)
int argc;
char **argv;
{
  int src_pos;
  int dst_pos;
  int new_argc = argc;

  src_pos = 1;
  dst_pos = 1;
  while (src_pos < argc)
  {
    if (IS_OPTION_CHAR(argv[src_pos][0]) &&
        strcmp(argv[src_pos]+1, "c960") == 0)
    {
      cmd_file_name = argv[src_pos+1];
      src_pos += 2;
      new_argc -= 2;
    }
    else 
    {
      argv[dst_pos] = argv[src_pos];
      dst_pos += 1;
      src_pos += 1;
    }
  }

  while (dst_pos < argc)
  {
    argv[dst_pos] = 0;
    dst_pos += 1;
  }

  return new_argc;
}

char *
get_cmd_file_name()
{
  return cmd_file_name;
}
