/*
 * GNU m4 -- A simple macro processor
 * Copyright (C) 1989-1992 Free Software Foundation, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "m4.h"
#include "helptext.h"
#include "getopt.h"
#include <sys/types.h>

#ifndef __GCC960
#if defined(__HIGHC__) || defined(WIN95)
#include <signal.h>
#else
#include <sys/signal.h>
#endif
#endif

static void cmd_error ();
extern FILE *debug;  /* found in debug.c */


/* Operate interactively (-i).  */
static int interactive = 0;

/* Enable sync output for /lib/cpp (-s).  */
int sync_output = 0;

/* Debug (-d[flags]).  */
int debug_level = 0;

/* Hash table size (should be a prime) (-Hsize).  */
int hash_table_size = HASHMAX;

/* Number of diversions allocated.  */
int ndiversion = NDIVERSIONS;

/* Disable GNU extensions (-G).  */
int no_gnu_extensions = 0;

/* Max length of arguments in trace output (-lsize).  */
int max_debug_argument_length = 0;

/* Suppress warnings about missing arguments.  */
int suppress_warnings = 0;

/* Program name.  */
const char *progname = NULL;

static struct option const m4_options[] =
{
  { "interactive",		no_argument,		NULL, 'i' },
  { "version",			no_argument,		NULL, 'V' },
  { "help",			optional_argument,	NULL, 'h' },
  { "synclines",		no_argument,		NULL, 's' },
  { "debug",			optional_argument,	NULL, 'd' },
  { "arglength",		required_argument,	NULL, 'l' },
  { "output",			required_argument,	NULL, 'o' },
  { "erroroutput",		required_argument,	NULL, 'e' },
  { "hashsize",			required_argument,	NULL, 'H' },
  { "diversions",		required_argument,	NULL, 'N' },
  { "include",			required_argument,	NULL, 'I' },
  { "no-gnu-extensions",	no_argument,		NULL, 'G' },
  { "silent",			no_argument,		NULL, 'Q' },
  { "quiet",			no_argument,		NULL, 'Q' },

  /* These are somewhat troublesome.  */
  { "define",			required_argument,	NULL, 'D' },
  { "undefine",			required_argument,	NULL, 'U' },
  { "trace",			required_argument,	NULL, 't' },
  { "TEST",			no_argument,		NULL, '\01' },
  { NULL },
};

#define OPTSTRING "BSTD:GH:I:N:QU:Vd:e:h:il:o:st:\01"

struct macro_definition
{
  struct macro_definition *next;
  int code;			/* D, U or t */
  char *macro;
};
typedef struct macro_definition macro_definition;

/*
 * usage --- Print usage message on stderr.
 */
static void
usage (void)
{
  fprintf (stderr, "\
Usage: m4 [OPTION]... [FILE]...\n\
\n\
  -D, --define NAME[=VALUE]     enter NAME has having VALUE, or empty\n\
  -G, --no-gnu-extensions       suppress all GNU extensions\n\
  -H, --hashsize PRIME          set symbol lookup hash table size\n\
  -I, --include DIRECTORY       search this directory second for includes\n\
  -N, --diversions NUMBER       select number of simultaneous diversions\n\
  -Q, --quiet, --silent         suppress some warnings for builtins\n\
  -U, --undefine NAME           delete builtin NAME\n\
  -V, --version                 print version number\n\
  -d, --debug [FLAGS]           set debug level\n\
  -i, --interactive             unbuffer output, ignore interrupts\n\
  -l, --arglength NUM           restrict macro tracing size\n\
  -o, --output FILE             redirect standard output\n\
  -e, --erroroutput FILE        redirect error, debug and trace output\n\
  -s, --synclines               generate `#line NO \"FILE\"' lines\n\
  -t, --trace NAME              trace NAME when it will be defined\n\
  -help                         print LONG, more detailed help text\n\
  -v960                         print version and exit\n\
\n\
If no FILE or if FILE is `-', standard input is read.\n");
#ifdef IMPLEMENT_M4OPTS
  fprintf (stderr, "\
Options can be given through M4OPTS environment variable.\n");
#endif

  exit (1);
}

#ifdef IMPLEMENT_M4OPTS
/*
 * Split a string of arguments in ENV, taken from the environment.
 * Return generated argv, and the number through the pointer ARGCP.
 */
static char **
split_env_args (char *env, int *argcp)
{
  char **argv;
  int argc;

  if (env == NULL)
    {
      *argcp = 0;
      return NULL;
    }

  /* This should be enough.  */
  argv = (char **) xmalloc ((strlen (env) / 2 + 3) * sizeof (char *));

  argc = 1;
  for (;;)
    {
      while (*env && (*env == ' ' || *env == '\t' || *env == '\n'))
	env++;

      if (*env == '\0')
	break;

      argv[argc++] = env;

      while (*env && !(*env == ' ' || *env == '\t' || *env == '\n'))
	env++;

      if (*env == '\0')
	break;

      *env++ = '\0';
    }
  argv[argc] = NULL;

  *argcp = argc;
  return argv;
}
#endif /* IMPLEMENT_M4OPTS */


/*
 * Parse the arguments ARGV.  Arguments that cannot be handled until
 * later (i.e., -D, -U and -t) are accumulated on a list, which is
 * returned through DEFINES.  The function returns the rest of the
 * arguments.
 */
#ifndef IMPLEMENT_M4OPTS
static const char **
parse_args (int argc, const char **argv, macro_definition **defines)
#else /* IMPLEMENT_M4OPTS */
static const char **
parse_args (int cmdargc, const char **cmdargv, macro_definition **defines)
#endif /* IMPLEMENT_M4OPTS */
{
  int option;			/* option character */
  int dummy;			/* dummy option_index, unused */
#ifdef IMPLEMENT_M4OPTS
  const char **argv;		/* argv from environment M4OPTS */
  int argc;			/* argc fron environment M4OPTS */
#endif
  macro_definition *head;	/* head of deferred argument list */
  macro_definition *tail;
  macro_definition *new;

  head = tail = NULL;

#ifdef IMPLEMENT_M4OPTS
  /* Should find a way to avoid the cast.  */
  argv = (const char **) split_env_args (getenv ("M4OPTS"), &argc);
  if (argv == NULL)
    {
      argv = cmdargv;
      argc = cmdargc;
      cmdargv = NULL;
    }
#endif /* IMPLEMENT_M4OPTS */

  while (TRUE)
    {
      /* Should find a way to avoid the cast.  */
      option = getopt_long (argc, (char *const *) argv,
			    OPTSTRING, m4_options, &dummy);

#ifndef IMPLEMENT_M4OPTS
      if (option == EOF)
	break;
#else /* IMPLEMENT_M4OPTS */

      /* Setting M4OPTS will likely break shell scripts that use m4 and
	 don't expect options that they don't give it to be set.  That's
	 why in general, noninteractive utilities shouldn't take options
	 from environment variables.  For programs like less and vi, it's
	 ok because the environment variables usually just affect the
	 appearance of what the user sees.  David Mackenzie, 1992-03-10.  */

      if (option == EOF)
	{
	  if (cmdargv != NULL)
	    {
	      if (optind != argc)
		cmd_error ("warning: excess file arguments in M4OPTS ignored");

	      argv = cmdargv;
	      argc = cmdargc;
	      cmdargv = NULL;
	      optind = 0;
	      continue;
	    }
	  else
	    break;
	}
#endif /* IMPLEMENT_M4OPTS */      

      switch (option)
	{
	case '\01': /* special dirty-tricks switch for testing purposes. */
	  progname = "mpp960";
	  break;

	case 'h':  /* assume -help; this is a kluge! */
	  paginator(helptext);
	  exit(0);

	case 'i':
	  interactive = 1;
	  break;

	case 's':
	  sync_output = 1;
	  break;

	case 'G':
	  no_gnu_extensions = 1;
	  break;

	case 'Q':
	  suppress_warnings = 1;
	  break;

	case 'V':
	  /* -V means print version info */
	  gnu960_put_version();
	  break;

	case 'd':
	  debug_level = debug_decode (optarg);
	  if (debug_level < 0)
	    {
	      cmd_error ("bad debug flags: `%s'", optarg);
	      debug_level = 0;
	    }
	  break;

	case 'l':
	  max_debug_argument_length = atoi (optarg);
	  if (max_debug_argument_length <= 0)
	    max_debug_argument_length = 0;
	  break;

	case 'e':
	  if (!debug_set_output (optarg))
	    cmd_error ("cannot set error file %s: %s", optarg, syserr ());
	  break;

	case 'o':
	  if (!set_output (optarg))
	    cmd_error ("cannot write standard-out to file %s: %s", optarg, syserr ());
	  break;

	case 'H':
	  hash_table_size = atoi (optarg);
	  if (hash_table_size <= 0)
	    hash_table_size = HASHMAX;
	  break;

	case 'N':
	  ndiversion = atoi (optarg);
	  if (ndiversion <= 0)
	    ndiversion = NDIVERSIONS;
	  break;

	case 'I':
	  add_include_directory (optarg);
	  break;

	case 'B':		/* compatibility junk */
	case 'S':
	case 'T':
	  break;

	case 'D':
	case 'U':
	case 't':
	  new = (macro_definition *) xmalloc (sizeof (macro_definition));
	  new->code = option;
	  new->macro = optarg;
	  new->next = NULL;

	  if (head == NULL)
	    head = new;
	  else
	    tail->next = new;
	  tail = new;

	  break;

	default:
	  usage ();
	}
    }

  *defines = head;
  return argv + optind;
}


int
main (int argc, const char **argv)
{
  macro_definition *defines;
  FILE *fp;

  argc = get_response_file(argc,&argv);

  check_v960( argc, argv );

  progname = rindex (argv[0], '/');
  if (progname == NULL)
    progname = argv[0];
  else
    progname++;

  include_init ();
  debug_init ();
  output_init ();

  /*
   * First, we decode the arguments, to size up tables and stuff.
   */
  argv = parse_args (argc, argv, &defines);

  /*
   * Do the basic initialisations.
   */
  input_init ();
  divert_init ();  /* this _must_ follow output_init and parse_args calls */
		   /* so diversion 0 is set to the "base" output stream */
  symtab_init ();
  builtin_init ();
  include_env_init ();

  /*
   * Handle deferred command line macro definitions.  Must come after
   * initialisation of the symbol table.
   */
  while (defines != NULL)
    {
      macro_definition *next;
      char *macro_value;
      symbol *sym;

      switch (defines->code)
	{
	case 'D':
	  macro_value = index (defines->macro, '=');
	  if (macro_value == NULL)
	    macro_value = "";
	  else
	    *macro_value++ = '\0';
	  define_user_macro (defines->macro, macro_value, SYMBOL_INSERT);
	  break;

	case 'U':
	  lookup_symbol (defines->macro, SYMBOL_DELETE);
	  break;

	case 't':
	  sym = lookup_symbol (defines->macro, SYMBOL_INSERT);
	  SYMBOL_TRACED (sym) = TRUE;
	  break;

	default:
	  internal_error ("bad code in deferred arguments.");
	}

      next = defines->next;
      xfree (defines);
      defines = next;
    }

#ifndef __GCC960
  /*
   * Interactive mode means unbuffered output, and interrupts ignored.
   */
  if (interactive)
    {
      signal (SIGINT, SIG_IGN);
      setbuf (stdout, (char *) NULL);
    }
#endif

  /*
   * Handle the various input files.  Each file is pushed on the
   * input, and the input read.  Wrapup text is handled separately
   * later.
   */
  if (*argv == NULL)
    {
      push_file (stdin, "stdin");
      expand_input ();
    }
  else
    for (; *argv != NULL; argv++)
      {
	if (strcmp (*argv, "-") == 0)
	  {
	    push_file (stdin, "stdin");
	  }
	else
	  {
	    fp = path_search (*argv);
	    if (fp == NULL)
	      {
		cmd_error ("can't open %s: %s", *argv, syserr ());
		continue;
	      }
	    else
	      push_file (fp, *argv);
	  }
	expand_input ();
      }
#undef NEXTARG

  /* Now handle wrapup text.  */
  while (pop_wrapup ())
    expand_input ();


  undivert_all ();

  return 0;
}


/*
 * The rest of this file contains error handling functions, and memory
 * allocation.
 */

/* Non m4 specific error -- just complain.  */
/* VARARGS */
static void
#ifdef __HIGHC__
cmd_error (the_tool)
char *the_tool;
#else
cmd_error (va_alist)
    va_dcl
#endif
{
  va_list args;
  char *fmt;

  fprintf (debug, "%s: ", progname);

#ifdef __HIGHC__
  va_start (args, the_tool);
  fmt = the_tool;
#else
  va_start (args);
  fmt = va_arg (args, char *);
#endif
  vfprintf (debug, fmt, args);
  va_end (args);

  putc ('\n', debug);
}

/* Basic varargs macro for all error output.  */
#define vmesg(level, fmt, args) \
  fflush (stdout); \
  fprintf (debug, "%s:%s:%d: ", progname, current_file, current_line); \
  if (level != NULL) \
    fprintf (debug, "%s: ", level); \
  vfprintf (debug, fmt, args); \
  putc ('\n', debug);
/* end of macro */

/* Internal errors -- print and dump core.  */
/* VARARGS */
volatile void
#ifdef __HIGHC__
internal_error (the_tool)
char *the_tool;
#else
internal_error (va_alist)
    va_dcl
#endif
{
  va_list args;
  char *fmt;

#ifdef __HIGHC__
  va_start(args, the_tool);
  fmt = the_tool;
#else
  va_start (args);
  fmt = va_arg (args, char*);
#endif
  vmesg ("internal error", fmt, args);
  va_end (args);

  abort ();
}

/* Fatal error -- print and exit.  */
/* VARARGS */
volatile void
#ifdef __HIGHC__
fatal (the_tool)
char *the_tool;
#else
fatal (va_alist)
    va_dcl
#endif
{
  va_list args;
  char *fmt;

#ifdef __HIGHC__
  va_start(args, the_tool);
  fmt = the_tool;
#else
  va_start (args);
  fmt = va_arg (args, char*);
#endif
  vmesg ("fatal error", fmt, args);
  va_end (args);

  exit (1);
}

/* "Normal" error -- just complain.  */
/* VARARGS */
volatile void
#ifdef __HIGHC__
error (the_tool)
char *the_tool;
#else
error (va_alist)
    va_dcl
#endif
{
  va_list args;
  char *fmt;

#ifdef __HIGHC__
  va_start(args, the_tool);
  fmt = the_tool;
#else
  va_start (args);
  fmt = va_arg (args, char*);
#endif
  vmesg ((char *) NULL, fmt, args);
  va_end (args);

}

/* Warning --- for potential trouble.  */
/* VARARGS */
volatile void
#ifdef __HIGHC__
warning (va_list the_tool)
#else
warning (va_alist)
    va_dcl
#endif
{
  va_list args;
  char *fmt;

#ifdef __HIGHC__
  va_start(args, the_tool);
  fmt = the_tool;
#else
  va_start (args);
  fmt = va_arg (args, char*);
#endif
  vmesg ("warning", fmt, args);
  va_end (args);

}


/*
 * Memory allocation functions
 */

/* Out of memory error -- die.  */
static void
no_memory (void)
{
  fatal ("Out of memory");
}

/* Free previously allocated memory.  */
void
xfree (void *p)
{
  if (p != NULL)
    free (p);
}

/* Semi-safe malloc -- never returns NULL.  */
void *
xmalloc (unsigned int size)
{
  register void *cp = malloc (size);
  if (cp == NULL)
    no_memory ();
  return cp;
}

#if 0
/* Ditto realloc.  */
void *
xrealloc (void *p, unsigned int size)
{
  register void *cp = realloc (p, size);
  if (cp == NULL)
    no_memory ();
  return cp;
}
#endif

/* And strdup.  */
char *
xstrdup (const char *s)
{
  return strcpy (xmalloc ((unsigned int) strlen (s) + 1), s);
}
