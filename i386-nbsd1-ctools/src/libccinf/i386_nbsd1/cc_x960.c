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

#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include "cc_info.h"
#include "i_toolib.h"

#if defined(DOS)

/* #include <io.h> */
#include <process.h>

#else

/* #include <sys/file.h> */
#ifndef __SELFHOST__
#include <sys/wait.h>
#endif

#endif

int flag_dryrun;
int flag_noisy;

static void
note_command (argv)
char* argv[];
{
  db_comment_no_buf ("%s", *argv);
  argv += 1;

  while (*argv != 0)
  {
    if (strchr(*argv, ' ') || strchr(*argv,'\t'))
      db_comment_no_buf (" \"%s\"", *argv);
    else
      db_comment_no_buf (" %s", *argv);
    argv += 1;
  }

  db_comment_no_buf ("\n");
}

/* Invoke the given command and return it's exit status.
 * A status > 0 indicates an error.
 * A status == 0 generally indicates success.
 * A status massively greater than 0 (eg, -1) generally indicates a
 * catastrophic error.
 */

enum syscall_id { sys_unlink };
static char* syscall[] = { "unlink" };

static
int syscall_id (command)
char *command;
{
  int sid;

  for (sid = 0; sid < sizeof(syscall)/sizeof(syscall[0]); sid++)
    if (!strcmp (command, syscall[sid]))
      return sid;
  return -1;
}

static unsigned
do_command (argv, level, cmd_temp)
char* argv[];
int level;
char *cmd_temp;
{
#ifdef I386_NBSD1
  extern const char *const sys_errlist[];
#else
  extern char* sys_errlist[];
#endif
  unsigned status = 0;

  int sid, noisy;

  /* Flush standard output streams */
  fflush(stdout);
  fflush(stderr);

  note_command (argv);

  fflush(stderr);

  if (flag_dryrun)
    return 0;

  sid = syscall_id(argv[0]);
  switch (sid)
  {
    case (int) sys_unlink:
      /* If we are noisy, then note_command has already announced the
         unlink, so turn noisy off */
      noisy = db_set_noisy (0);
      while (*(++argv))
        db_unlink (*argv,db_fatal);
      db_set_noisy(noisy);
      return 0;

    case -1:
      break;

    default:
      assert (0);
      break;
  }

#if defined(DOS)
  argv = check_dos_args(argv);
  status = spawnvp(P_WAIT, argv[0], argv);
  if (((int) status) < 0)
    db_error ("spawn %s failed; %s", argv[0], sys_errlist[errno]);
  delete_response_file();
#elif defined(__SELFHOST__)
  if (1) {
      char *buff = db_malloc(128);
      int buff_size = 128;
      int i;

      buff[0] = 0;

      for (i=0;argv[i];i++) {
	  if ((strlen(buff) + strlen(argv[i]) + 2) < buff_size)
		  buff = db_realloc(buff,buff_size *= 2);
	  if (buff[0])
		  strcat(buff," ");
	  strcat(buff,argv[i]);
      }
      status = system(buff);
  }
#else
  {
    int pid;
    pid = fork();
    status = 0;

    if (pid == -1)    /* fork failed */
    { status = -1;
      db_error ("fork %s failed; %s", argv[0], sys_errlist[errno]);
    }
    else if (pid == 0)  /* In child */
    { fflush (stderr);
      fflush (stdout);
      execvp (argv[0], &argv[0]);
      db_fatal ("exec %s failed; %s", argv[0], sys_errlist[errno]);
    }
    else      /* In parent */
    {
      int wait_status = 0;

      /* Wait for our child to terminate */
      while (wait(&wait_status) != pid)
        ;

      if (WIFEXITED(wait_status))

        /* Return the child's exit status. */
        status = WEXITSTATUS(wait_status);
      else
      {
        /* Child exited abnormally, not via its * own call to exit or _exit.  */
        db_error ("%s got fatal signal %d\n", argv[0], WTERMSIG(wait_status));
        status = -1;
      }
    }
  }
#endif

  fflush(stdout);
  fflush(stderr);

  if (status == 0)
    status = do_command_file (cmd_temp, level+1);
  db_unlink(cmd_temp, db_fatal);

  return status;
}

static int
do_command_file(fname, level)
char *fname;
int level;
{
  int sz;

  /* Buffers for the worst case command or removal we should see */
  char** arg = (char **)db_malloc(sizeof (char *) * CI_X960_NARG_PTR);
  char*  buf = (char *)db_malloc(CI_X960_NBUF_INT * sizeof(int));
  char*  rm = (char *)db_malloc(CI_X960_NBUF_INT * sizeof(int));

  /* Set up a slot for file removal by signal handler. */
  char** removal;
  char *cmd_file_temp;

  /* We send back the highest return of any of our children */
  int file_status = 0;
  named_fd f;

  sprintf (buf, "xdXXXXXX.%.3d", level);
  cmd_file_temp = get_960_tools_temp_file(buf, db_malloc);

  dbf_open_read (&f, fname, 0);
  if (f.fd == 0)
    return 0;

  removal = db_new_removal ("");

  do
  { /* Process each command group until we see -1 for removal list. */
    dbf_read (&f, &sz, sizeof (sz));

    if (sz != -1)
    {
      int group_status = 0;
  
      if (sz)
        /* Read the files to be removed if anybody in the group fails. */
        dbf_read (&f, rm,  sz);
  
      rm[sz]   = '\0';
      if (flag_dryrun)
        rm[0]='\0';
      *removal = rm;
  
      do
      { /* Process each command in the group.  */
        dbf_read (&f, &sz, sizeof(sz));
  
        if (sz != 0)
        {
          char  *bp = buf;
          char **av = arg;
          int   csz = sz;
          int   i;
    
          /* Read the command ... */
          dbf_read (&f, bp, sz);
    
          /* Set up argument vector */
          i = 0;
          do
          { 
            int   n = strlen(bp)+1;

            *(av++) = bp;
            bp     += n;
            csz    -= n;

            /*
             * make first arguments be -c960 stuff, except if this is
             * one of our system calls.
             */
            if (i == 0 && syscall_id(arg[0]) < 0)
            {
              *(av++) = "-c960";
              *(av++) = cmd_file_temp;
            }

            i += 1;
          } while (csz);
    
          *(av++) = 0;
    
          /* Execute the command, unless this group is already hosed. */
          if (group_status == 0)
            if (group_status = do_command (arg, level, cmd_file_temp))
            { if (group_status > file_status)
                file_status = group_status;
              db_remove_files (removal);
            }
        }
      }
      while
        (sz != 0);

      *removal = 0;
    }
  } while (sz != -1);

  *removal = 0;  /* this deallocates the removal table entry */
  dbf_close(&f);

  free(rm);
  free(buf);
  free(arg);
  free(cmd_file_temp);

  return file_status;
}

int
db_x960(argc, argv)
int argc;
char* argv[];
{
  char *fname = 0;
  int i;
  int ret_status;

  i = 0;

  argc = get_response_file(argc, &argv);
  check_v960(argc, argv);

  for (i = 1; i < argc; i++)
    if (IS_OPTION_CHAR(argv[i][0]))
    {
      if (!strcmp (argv[i]+1, "dryrun"))
        flag_dryrun = 1;
      else if (!strcmp (argv[i]+1, "v"))
        flag_noisy = 1;
      else
        db_fatal ("invalid switch %s", argv[i]);
    }
    else if (fname == 0)
    {
      fname = argv[i];
    }
    else
      db_fatal ("only one input file is allowed", argv[i]);

  if (flag_noisy | flag_dryrun)
    db_set_noisy (1);

  if (fname == 0)
    db_fatal ("no input file was specified");

  ret_status = do_command_file(fname, 1);

  return ret_status;
}
