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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gcov960.h"
#include "gcovrptr.h"

double total_profile_exec_count;

/* Globals */
static char *opt_coverage = "p";  /* program-level is the default */
static char *opt_report = "";    /* default is to  report both hits and miss in
                   * source annotation report 
                   */
static char *opt_call_graph = "";  /* default is no call graph help */

static char src_list_opt = 0;   /* 0 means don't generate source listing */

coverage_info_type cmd_line;
/* control options */
enum { none, coverage, frequency, call_graf, report } control = none;

/* The following "get_" routines will return a pointer to the
 * the last char parsed.  */

static char *get_coverage_opt (arg)
char *arg;
{
  opt_coverage = arg;

  if (*arg && (*arg !='p') && (*arg != 'm') && (*arg != 'f') && (*arg != 's'))
    db_fatal ("invalid coverage option:", arg);

  return (arg);
}

static char *get_report_opt (arg)
char *arg;
{
  /* Here it could be 'r[l][m|h]' */
  if (!*arg)
    db_fatal ("use -rl, -rh, or -rm");

  while (*arg)
  {
    if (*arg == 'l') 
      src_list_opt = 1;

    else if ( (*arg == 'm') || (*arg == 'h') )
      if (*opt_report) /* 'mh' or 'hm' was already parsed */
           opt_report = "b"; /* Kludge: signify both hits and miss */
      else
        opt_report = arg;

    else
      db_fatal ("invalid report option:", arg);

    arg++;
  }
  return (arg-1);
}

static char *get_cg_opt( arg )
char *arg;
{
  opt_call_graph = arg;

  if (*arg != 'h')
    db_fatal ("invalid call graph option:", arg);
  return arg;
}

static char *get_frequency_opt(arg)
char *arg;
{
  int top;

  if ((top = atoi(arg)) != 0)
  {
    cmd_line.frequency = top;
    return (arg + strlen(arg) - 1);
  }
  db_fatal ("invalid frequency option:", arg);
}

static char *get_quality_opt(arg)
char *arg;
{
  int top;

  if ((top = atoi(arg)) >= 0 && top <= 9)
  {
    cmd_line.min_quality = top;
    return (arg + strlen(arg) - 1);
  }
  db_fatal ("invalid quality option:", arg);
}

/*
 * get_file_list_opt()
 * 
 *  From the command line, this parses the file=module specs,
 *  and builds a list of which is head is cmd_line.
 */

static void get_file_list_opt (arg)
char *arg;
{
  char *tok_ptr = arg;
  char *work_arg;
  mod_info_type *N,*M;

  if (!*arg)
    return;

  /* look for the first empty slot in the module list */
  for (N = &cmd_line.mod_info; N->link != 0 ; N = N->link)
    ;

  /* Allocate a space for the next item to be inserteCd. */

  if ( (N->src_name != 0) )
  { N->link = (mod_info_type *) db_malloc(MODINFO_SZ);
    memset (N->link, 0, MODINFO_SZ);
    N = N->link;
  }

  /* This strange-looking for-loop is for controlling input
   * to strtok(). First the string to parse gets passed to
   * strtok(); NULLs subsequently.  */

  for (work_arg = arg; tok_ptr != NULL; work_arg = NULL)
  {
    if (tok_ptr = strtok(work_arg, " ="))
    {
      N->src_name = db_malloc(strlen(tok_ptr) + 1);
      strcpy(N->src_name, tok_ptr);

      if (tok_ptr = strtok(NULL, ", "))
      {
        if (N->mod_name) 
        {
          M = (mod_info_type *) db_malloc(MODINFO_SZ);
          memset (M, 0, MODINFO_SZ);

          N->mod_name = db_malloc(strlen(tok_ptr) + 1);
          strcpy(M->mod_name, tok_ptr);

          M->src_name = N->src_name;
          N->link = M;
          N = N->link;
        }
        else
          N->mod_name = tok_ptr;
      }
      /* now get rest of  module names */
      while (tok_ptr = strtok(NULL, ", "))
      {
        M = (mod_info_type *) db_malloc(MODINFO_SZ);
        memset (M, 0, MODINFO_SZ);

        N->mod_name = db_malloc (strlen(tok_ptr) + 1);
        strcpy(N->mod_name, tok_ptr);

        M->src_name = N->src_name;
        N->link = M;
        N = N->link;
      }
    }
  }

#if DEBUG
  for (M = &cmd_line.mod_info ; M ; M = M->link )
    printf("Source = %s,  Module = %s\n",M->src_name, M->mod_name);
#endif
}

directory_list dir_list_head = { ".",(directory_list *) 0};
static directory_list **bottom_of_dir_list = &dir_list_head.next;

static char *get_src_dir_opt (arg)
char *arg;
{
  if (!arg || !*arg)
    db_fatal ("-I option requires a directory name");

  (*bottom_of_dir_list) = (directory_list *) db_malloc(sizeof(directory_list));
  memset ((*bottom_of_dir_list), 0, sizeof(directory_list));

  (*bottom_of_dir_list)->name = arg;
  (*bottom_of_dir_list)->next = (directory_list *) 0;
  bottom_of_dir_list = &(*bottom_of_dir_list)->next;
  return (arg + strlen(arg) - 1);
}

static char *get_database_dir (arg)
char *arg;
{
  if (!arg || !*arg)
    db_fatal ("-Z option requires a directory name");

  db_note_pdb (arg);

  return (arg + strlen(arg) - 1);
}

static char *
get_prof_file_opt(opt, arg)
char* opt;
char *arg;
{
  if (!arg || !*arg)
    db_fatal ("'%s' option requires a profile file", opt);

  if (!db_access_rok (arg))
    if (opt)
      db_fatal ("cannot open '%s' argument '%s' for reading", opt, arg);
    else
      db_fatal ("cannot open '%s' for reading", arg);

  if (db_is_iprof (arg))
    db_set_arg (&(cmd_line.main_iargc), &(cmd_line.main_iargv), arg);

  else if (db_is_kind (arg, CI_SPF_DB))
    db_set_arg (&(cmd_line.main_sargc), &(cmd_line.main_sargv), arg);

  else
    if (opt)
      db_fatal ("'%s' argument '%s' is not a profile file", opt, arg);
    else
      db_fatal ("'%s' is not a profile file", arg);

  return (arg + strlen(arg) - 1);
}

static char *get_compare_opt ( arg )
char *arg;
{
  if (!arg || !*arg)
    db_fatal ("-n option requires a profile file");

  if ((control != none) && (control != report))
    db_fatal ("-n can only be used with -r option");

  if (db_is_iprof (arg))
    db_set_arg (&(cmd_line.other_iargc), &(cmd_line.other_iargv), arg);

  else if (db_is_kind (arg, CI_SPF_DB))
    db_set_arg (&(cmd_line.other_sargc), &(cmd_line.other_sargv), arg);

  else
    db_fatal ("'%s' is not a profile file", arg);

  return (arg + strlen(arg) - 1);
}

#define ARGV_OK ( (i+1) < argc)
#ifdef DOS
#define IS_OPTION_CHAR(c) ((c)=='-' || (c)=='/')
#else
#define IS_OPTION_CHAR(c) ((c)=='-')
#endif

int main (argc, argv)
int argc;
char **argv;
{
  char option_c;
  int i;
  char *next_arg;

  int  build_cg = 0,   /* need to build cg */  
  seen_n_option = 0,   /* seen -n */
  seen_f_option = 0;   /* seen -f */

  db_set_prog (argv[0]);

  /* Check the command line for a response file, and handle it if found.*/
  argc = get_response_file(argc,&argv);

  check_v960( argc, argv );
  
  if (argc == 1) 
  { gcov960_help();
    exit(0);
  } 

  memset (&cmd_line, 0, sizeof (cmd_line));
  cmd_line.min_quality = -1;
  cmd_line.dir_name = ".";
  cmd_line.frequency = 10;

  for (next_arg=argv[i=1]; i < argc ; next_arg=argv[++i])
    if (!IS_OPTION_CHAR(*next_arg) )
      get_file_list_opt(next_arg);

    else
      while (option_c = *(++next_arg)) 
        switch (option_c)
        {
          case 'c':
          case 'g':
          case 'r':
            if ((control != none) && (control != frequency))

              /* We're making an exception for -f; it can be used with -rl.  */
              db_error("multiple operations/controls specified");

            switch (option_c)
            {
              case 'c':
                if (*(next_arg+1))
                  next_arg = get_coverage_opt(next_arg+1); 
                control = coverage;
                break;
          
              case 'g':
                control = call_graf;
                if (*(next_arg + 1))
                  next_arg = get_cg_opt(next_arg+1);
                build_cg = 1;
                break;

              case 'r':
                next_arg = get_report_opt(next_arg+1);
                control = report;
                break;
            }
            break;

            /* Note that the options below are not controls, but
             *  are "other" options that can used along with the 
             *  controls */

            case 'f':
              if (*(next_arg+1))
                next_arg = get_frequency_opt(next_arg+1); 
              else if (ARGV_OK && !(IS_OPTION_CHAR(argv[i+1][0])))
                get_frequency_opt(argv[++i]);
              if (control != report)
                control = frequency;
              seen_f_option = 1;
              break;
        
            case 'Q': 
              if (*(next_arg+1))
                next_arg = get_quality_opt(next_arg+1);
              else if (ARGV_OK && !(IS_OPTION_CHAR(argv[i+1][0])))
                get_quality_opt(argv[++i]);
              else
                db_fatal ("'Q' option requires an argument");
              break;

            case 'n': 
              if (*(next_arg+1))
                next_arg = get_compare_opt(next_arg+1);
              else if (ARGV_OK && !(IS_OPTION_CHAR(argv[i+1][0])))
                get_compare_opt(argv[++i]);
              else
                db_fatal ("'n' option requires an argument");
              seen_n_option = 1;
              break;

            case 'h':
              gcov960_help();
              break;
                  
            case 'I':
              if (*(next_arg+1))
                next_arg = get_src_dir_opt(next_arg+1);
              else if (ARGV_OK && !(IS_OPTION_CHAR(argv[i+1][0])))
                get_src_dir_opt(argv[++i]);
              else
                db_fatal ("'I' option requires an argument");
              break;                    
              
            case 'Z': 
              if (*(next_arg+1))
                next_arg = get_database_dir(next_arg+1);
              else if ( ARGV_OK && !(IS_OPTION_CHAR(argv[i+1][0])))
                get_database_dir(argv[++i]);
              else
                db_fatal ("'Z' option requires an argument");
              break;

            case 'i': /* possible -iprof, similar to gcdm960's -iprof */
              if ((strncmp(next_arg, "iprof", 5) == 0))
              { next_arg += 4;
                if (*(next_arg+1))
                  next_arg = get_prof_file_opt("iprof", next_arg+1);
                else if (ARGV_OK && !(IS_OPTION_CHAR(argv[i+1][0])))
                  get_prof_file_opt("iprof", argv[++i]);
                else
                  db_fatal ("'iprof' option requires an argument");
                break;
              }

            case 'p':
              if (*(next_arg+1))
                next_arg = get_prof_file_opt("p", next_arg+1);
              else if (ARGV_OK && !(IS_OPTION_CHAR(argv[i+1][0])))
                get_prof_file_opt("p", argv[++i]);
              else
                db_fatal ("'p' option requires an argument");
              break;

            case 't':
              cmd_line.truncate_report = 1;
              break;
              
            case 'q':
              cmd_line.suppress_signon = 1;
              break;

            case 'C':
              cmd_line.print_prof_total_count = 1;
              break;

            case 'V':
              /* -V means print version info but don't exit */
              gnu960_put_version();
              break;

          default:
            db_fatal ("Illegal control option: %c", *next_arg);
        }

  if (cmd_line.min_quality < 0)
    cmd_line.min_quality = 9;

  if (cmd_line.main_iargc == 0 && cmd_line.main_sargc == 0)
    if (db_access_rok ("default.pf"))
      get_prof_file_opt (0, "default.pf");

  if ((control==report) &&(*opt_report=='b'||*opt_report=='\0') &&seen_n_option)
    if (src_list_opt)
      db_fatal ("-n requires either '-rlh' or '-rlm'");

    else if ((*opt_report == '\0') || (*opt_report == 'b'))
      db_fatal ("-n requires either '-rh' or '-rm'");

  if (control == none) 
  { /* set default behavior */
    control = coverage;
    opt_coverage = "p";
  } 

  if (control == call_graf)
    build_cg = 1;

  if ((control == coverage) && (*opt_coverage == 'f'))
    build_cg = 1;

  if (control == report
  && (*opt_report=='h'||*opt_report=='m'||*opt_report=='\0'||*opt_report=='b'))
    build_cg = 1;

  extract_profile_info (build_cg, seen_n_option);

  /* now perform command-line requests */

  switch (control)
  {
    case coverage:
      if (*opt_coverage == 'p' || (*opt_coverage == '\0'))
          /* -c without other option defaults to program-level coverage */
        display_program_coverage();

      else if (*opt_coverage == 'f')
        display_function_coverage();

      else if (*opt_coverage == 'm')
        display_module_coverage();

      else if (*opt_coverage = 's')
        display_source_coverage();

      break;

    case report:
      if (src_list_opt)
        gcov_src_report (seen_n_option, *opt_report == 'h',
                         (*opt_report == '\0') || (*opt_report == 'b'),
                          seen_f_option);
      else if (*opt_report == 'h')
        seen_n_option ? display_new_hits(1) : display_hits(1, 0);

      else if (*opt_report == 'm')
        seen_n_option ? display_new_hits(0) : display_hits(0, 0);

      else if (*opt_report == '\0' || *opt_report == 'b')
        display_hits(0, 1);  /* display both hits and misses */

      break;

    case frequency:
      display_frequency(stdout, 0, 0);
      break;

    case call_graf:
      display_call_graph(*opt_call_graph == 'h');
      break;
  } 

  exit(0);
}
