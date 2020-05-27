/******************************************************************/
/*       Copyright (c) 1990,1991,1992,1993 Intel Corporation

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
   regarding the use of, or the results of the use of, the software 
   and documentation in terms of correctness, accuracy, reliability, 
   currentness, or otherwise; and you rely on the software, 
   documentation and results solely at your own risk.
 */
/******************************************************************/

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include "assert.h"
#include "cc_info.h"
#include "i_toolib.h"

#include "gcdm960.h"
#include "callgrph.h"
#include "fastmem.h"
#include "parmchk.h"
#include "inline.h"
#include "subst.h"

#define ARCH_K_SERIES 0x10
#define ARCH_C_SERIES 0x20
#define ARCH_S_SERIES 0x40
#define ARCH_J_SERIES 0x80
#define ARCH_H_SERIES 0x100

#define ARCH_NUMERICS  0x1
#define ARCH_PROTECTED 0x2

#define ARCH_KA (ARCH_K_SERIES)
#define ARCH_KB (ARCH_K_SERIES | ARCH_NUMERICS)
#define ARCH_CA (ARCH_C_SERIES)
#define ARCH_SA (ARCH_S_SERIES)
#define ARCH_SB (ARCH_S_SERIES | ARCH_NUMERICS)
#define ARCH_JA (ARCH_J_SERIES)
#define ARCH_JD (ARCH_J_SERIES)
#define ARCH_JF (ARCH_J_SERIES)
#define ARCH_JL (ARCH_J_SERIES)
#define ARCH_RP (ARCH_J_SERIES)
#define ARCH_HA (ARCH_H_SERIES)
#define ARCH_HD (ARCH_H_SERIES)
#define ARCH_HT (ARCH_H_SERIES)

#define DEFAULT_PROF_NAME "default.pf"
#define DEFAULT_ARCH      ARCH_KB
#define DEFAULT_INLINE    3

named_fd decision_file;
int flag_print_summary;
int flag_print_decisions;
int flag_print_variables;
int flag_print_call_graph;
int flag_print_reverse_call_graph;
int flag_print_closure;
int flag_print_profile_counts;
int flag_dryrun;
int flag_noisy;
int flag_gcdm;
int flag_ic960;
int flag_no_inline_libs;
int lines_per_page = 55;
long text_size = 0;

static int
set_arch(arch_str)
char *arch_str;
{
  int i;
  static struct {
    char *arch_name;
    int  arch_val;
  }
  archs[] = {
    "KA", ARCH_KA,
    "KB", ARCH_KB,
    "CA", ARCH_CA,
    "CA_DMA", ARCH_CA,
    "CF", ARCH_CA,
    "CF_DMA", ARCH_CA,
    "SA", ARCH_SA,
    "SB", ARCH_SB,
    "JA", ARCH_JA,
    "JD", ARCH_JD,
    "JF", ARCH_JF,
    "JL", ARCH_JL,
    "RP", ARCH_RP,
    "HA", ARCH_HA,
    "HD", ARCH_HD,
    "HT", ARCH_HT,
    0, 0
  };

  for (i = 0; archs[i].arch_name != 0; i++)
  {
    if (strcmp(archs[i].arch_name, arch_str) == 0)
      return archs[i].arch_val;
  }

  db_fatal ("unrecognized architecture: %s", arch_str);
  return ARCH_KB;
}

int
get_num(cp, np)
char *cp;
unsigned long *np;
{
  int radix;
  int ok = 1;
  int legal_digit;
  int ret = 0;
  unsigned long sum = 0;
  int n_legal_digits = 0;

  if (*cp == '0')
  {
    if (*(cp+1) == 'x' || *(cp+1) == 'X')
    {
      radix = 16;
      cp = cp+2;
    }
    else
      radix = 8;
  }
  else
    radix = 10;

  do
  {
    switch (*cp)
    {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
        legal_digit = 1;
        sum = sum * radix + (*cp - '0');
        cp++;
        break;

      case '8':
      case '9':
        if (radix == 8)
        {
          legal_digit = 0;
          ok = 0;
          break;
        }
        legal_digit = 1;
        sum = sum * radix + (*cp - '0');
        cp++;
        break;

      case 'A':
      case 'B':
      case 'C':
      case 'D':
      case 'E':
      case 'F':
        if (radix != 16)
        {
          legal_digit = 0;
          break;
        }
        legal_digit = 1;
        sum = sum * radix + 10 + (*cp - 'A');
        cp++;
        break;

      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f':
        if (radix != 16)
        {
          legal_digit = 0;
          break;
        }
        legal_digit = 1;
        sum = sum * radix + 10 + (*cp - 'a');
        cp++;
        break;

      default:
        legal_digit = 0;
        break;
    }

    if (legal_digit)
      n_legal_digits += 1;
  } while (ok && legal_digit);

  *np = sum;
  ret = ok && n_legal_digits != 0;

  return ret;
}

static
char *utext[] =
{
  "",
  "GNU/960 compiler decision maker",
  "",
  "          -h: display this help message",
  "       -v960: print the version number and exit",
  "      -Z pdb: specify the program data base directory",
  "     -dryrun: make decisons but do not update database or issue compiles",
  "          -A: specify an 80960 architecture", 
  "              [valid architectures are: KA,SA,KB,SB,CA,CF,JA,JD,JF,",
  "              HA,HD,HT,RP]",
  "   -inline=N: control how aggressively gcdm960 makes decisions", 
  "              about global inlining, the higher the number 'N', the more",
  "              aggressive gcdm960's inlining decisions.",
  "              [default is inline=3, N must be 0-4]",
  "      -iprof: specify the profile information filename",
  "              [if this is not specified and no 'subst' commands are given",
  "              'default.pf' is used]",
  "      -sram=: specify the starting and ending addresses of the", 
  "              on-chip SRAM areas that are available for gcdm960 to",
  "              allocate to global variables",
  "              [default is to not allocate variables into SRAM]",
  "         -m=: specify the starting addresses and lengths of fast memory",
  "              areas that are available for gcdm960 to allocate to",
  "              global variables", 
  "              [default is to not allocate variables into fast memory]",
  "        -dec: specify output for the print options [defaults to stdout]",
  "   -rsummary: print a short summary of the decisions made",
  " -rdecisions: print a report of the decisions made",
  "-rcall-graph: print the program's call graph, with call-counts",
  "   -rreverse: print the reverse of the call graph, with call-counts",
  "   -rclosure: print a list of all functions that can be called",
  "              from a function, either directly or indirectly",
  " -rvariables: print the variables allocated to fast memory",
  "   -rprofile: print the profile counts for the whole program", 
  "",
  "       -gcdm: accept a list of gcdm options in the same form as that",
  "              accepted by gcc960, ic960, and the linker",
  "",
  "        -ref: specify that the set of modules described by a regular",
  "              expression are needed even though they appear not to be",
  "",
  "      -noref: make the specified modules be unaffected by a previous -ref",
  "",
  "      -subst: specify that the set of modules described by a regular",
  "              expression are to be automatically recompiled (by gcdm960)",
  "              using a specified list of switches, and that the modules",
  "              are to be automatically replaced by the linker at link time",
  "              by the recompiled modules",
  "",
  "    -nosubst: make the specified modules be unaffected by a previous -subst",
  "",
  "See your user's guide for a complete command-line description",
  NULL
};

void
put_gcdm_help()
{
  printf ("\nUsage:  %s [options ...]\n", db_prog_name());
  paginator (utext);        
}

static
int inline_switch = DEFAULT_INLINE;
int
inline_level ()
{
  return inline_switch;
}

#define REQUIRE_ARG(NULL_OK) \
do { \
  memcpy ((p=db_malloc(n+2)), s, n+2); \
  p[n+1] = '\0'; \
  if (s[n+1] == '\0') \
  { if (argn >= (argc - 1)) \
      db_fatal ("option '%s' requires an argument", s); \
    s = argv[++argn]; \
  } \
  else { \
    if (s[n+1] != '=') \
      db_fatal ("option '%s' requires an argument", p); \
    s += n+2; \
  } \
  if (s[0] == '\0' && !NULL_OK) \
    db_fatal ("option '%s' requires a non-null argument", p); \
} while(0)

int
main (argc, argv)
int argc;
char **argv;
{
  static char **sprofv, **iprofv;
  int iprofc = 0;
  int sprofc = 0;
  int null_iprof = 0;
  int argn;

  named_fd in, out, prev;

  int  arch_switch = DEFAULT_ARCH;

  call_graph * cg;   /* pointer to call graph data structure */

  db_set_signal_handlers ();

  db_assert_fail = db_fatal;
#ifdef DOS
#   ifdef _INTELC32_ /* Intel CodeBuilder */
  /***********************************************************
  **  Initialize the internal Code Builder run time libraries
  **  to accept the maximum number of files open at a time
  **  that it can.  With this set high, the number of files
  **  that can be open at a time is completely dependent on
  **  DOS config.sys FILES= command.
  ************************************************************/
  _init_handle_count(MAX_CB_FILES);
#   endif  /* Intel CodeBuilder */
#endif

  argc = get_response_file (argc, &argv);
  argc = get_cmd_file (argc, argv);
  check_v960 (argc, argv);

  decision_file.fd = stdout;
  decision_file.name = "";

  db_set_prog (*argv);

  /* Expand all /gcdm commands in into their component gcdm960 switches. */
  expand_gcdm_switches (&argc, &argv);

  if (argc == 1)
  { put_gcdm_help();
    exit(0);
  } 

  for (argn = 1; argn < argc; argn++)
  { mname_kind mkind = mn_nokind;
    char* s = argv[argn];
    char* p;
    int   n = 0;

    if (!IS_OPTION_CHAR(*s))
      db_fatal ("unrecognized option '%s'", s);

    /* Add /ref or /subst commands to the appropriate module name
       list, for later expansion after we have the names of the
       modules with ccinfo */

    if (!strncmp (s+1, "subst", n=5) || !strncmp (s+1, "nosubst", n=7))
      mkind = mn_subst;
    else
      if (!strncmp (s+1, "ref", n=3) || !strncmp (s+1, "noref", n=5))
        mkind = mn_ref;
      else
        if (!strncmp (s+1, "code_size", n=9))
          mkind = mn_size;

    if (mkind != mn_nokind)
    { 
      module_name **mhead = &mname_head[mkind];
      module_name *mnode;

      mnode = (module_name*) db_malloc (sizeof (*mnode));
      memset (mnode, 0, sizeof(*mnode));

      mnode->id = num_mname[mkind]++;

      /* If subst or ts, a + seperated modifier follows module_name.
         Otherwise, it is not allowed.  So, for all except subst, we
         fill in the modifier field with the switch so we know not
         to accept a modifier later if we find one. */

      if (strncmp (s+1, "subst", n) && strncmp (s+1, "code_size",n))
      { mnode->modifier = db_malloc (n+2);
        memcpy (mnode->modifier, s+1, n);
        mnode->modifier[n] = '\0';
      }

      REQUIRE_ARG(0);

      n = strlen (s) + 1;

      /* Make a copy of the module_name so we can later substitute
         '\0' characters for ':' and '+' */

      memcpy (mnode->text=db_malloc(n), s, n);
      mnode->next = *mhead;
      *mhead = mnode;
    }

    else if (!strncmp (s+1, "dec", n=3))
    {
      REQUIRE_ARG(0);

      if (decision_file.name[0])
      { db_warning("decision file '%s' overridden by '%s'",
                   decision_file.name, s);
        dbf_close (&decision_file);
      }
      dbf_open_write_ascii (&decision_file, s, db_fatal);
    }

    else if (!strcmp (s+1, "v"))
      flag_noisy = 1;

    else if (!strcmp (s+1, "dryrun"))
      flag_dryrun = 1;

    else if (!strcmp (s+1, "ic960"))
      flag_ic960 = 1;

    else if (!strcmp (s+1, "gcc960"))
      flag_ic960 = 0;

    else if (!strncmp (s+1, "iprof", n=5))
    {
      REQUIRE_ARG(1);

      if (*s == '\0')
        null_iprof = 1;
      else if (!db_access_rok (s))
        db_fatal ("cannot open '%s' argument '%s' for reading", p, s);
      else if (db_is_iprof (s))
        db_set_arg (&iprofc, &iprofv, s);
      else if (db_is_kind (s, CI_SPF_DB))
        db_set_arg (&sprofc, &sprofv, s);
      else
        db_fatal ("'%s' argument '%s' is not a profile file", p, s);
    }

    else if (!strcmp (s+1, "rsummary"))
      flag_print_summary = 1;

    else if (!strcmp (s+1, "rdecisions"))
      flag_print_decisions = 1;

    else if (!strcmp (s+1, "rcall-graph"))
    { flag_print_call_graph = 1;
      flag_print_reverse_call_graph = 0;
    }

    else if (!strcmp (s+1, "rreverse"))
    { flag_print_reverse_call_graph = 1;
      flag_print_call_graph = 0;
    }

    else if (!strcmp (s+1, "rclosure"))
      flag_print_closure = 1;

    else if (!strcmp (s+1, "rvariables"))
      flag_print_variables = 1;

    else if (!strcmp (s+1, "rprofile"))
      flag_print_profile_counts = 1;

    else if (!strcmp (s+1, "h"))
    { put_gcdm_help();
      exit(0);
    }

    else if (!strncmp (s+1, "A", 1))
      arch_switch = set_arch(s+2);

    else if (!strncmp (s+1, "Z", n=1))
    {
      REQUIRE_ARG(0);
      db_note_pdb (s);
    }

    else if (!strncmp (s+1, "text_size", n=9))
    {
      /* Handle global ts switch here.  This is an overall target
         that is used for modules not explicitly named in a -code_size

         Only 1 global ts value is allowed. */

      if (text_size)
        db_fatal ("'%s' can be specified only once", s);

      REQUIRE_ARG(0);

      if (!get_num(s, &text_size) || text_size == 0)
        db_fatal ("'%s' argument '%s' is illegal", p, s);
    }

    else if (!strncmp (s+1, "m", n=1))
    {
      char *e;
      unsigned long length;
      unsigned long start;

      REQUIRE_ARG(0);
      free_fmem();

      while (1)
      {
        e = strchr(s, ',');
        if (e == 0)
          goto m_error;

        *e = '\0';
        if (!get_num(s, &start))
          goto m_error;

        s = e + 1;
        e = strchr(s, ',');
        if (e != 0)
          *e = '\0';

        if (!get_num(s, &length))
          goto m_error;

        add_fmem(start, length);

        if (e != 0)
          s = e+1;
        else
          break;
      }
      
      if (0)
      {
        m_error: ;

        free_fmem();
        db_fatal ("'%s' argument '%s' is not of the form hexstart,hexlength[,hexstart-hexlength]", p, s);
      }
    }

    else if (!strncmp (s+1, "sram", n=4))
    {
      char *e;
      unsigned long start;
      unsigned long end;

      REQUIRE_ARG(0);
      free_fmem();

      while (1)
      {
        e = strchr(s, '-');
        if (e == 0)
          goto sram_error;

        *e = '\0';
        if (!get_num(s, &start))
          goto sram_error;

        s = e + 1;
        e = strchr(s, ',');
        if (e != 0)
          *e = '\0';

        if (!get_num(s, &end))
          goto sram_error;

        add_fmem (start, end - start + 1);

        if (e != 0)
          s = e+1;
        else
          break;
      }
      
      if (0)
      {
        sram_error: ;

        free_fmem();
        db_fatal ("'%s' argument '%s' is not of the form hexstart-hexend[,hexstart-hexend]", p, s);
      }
    }

    else if (!strncmp (s+1, "inline", n=6))
    {
      REQUIRE_ARG(0);

      if (s[1] != '\0' || s[0] < '0' || s[0] > '4')
        db_fatal ("'%s' argument must be one of {0,1,2,3,4}");

      inline_switch = s[0] - '0';
    }

    else if (!strcmp(s+1, "no-inline-libs"))
      flag_no_inline_libs = 1;

    else
      db_fatal ("unrecognized option '%s'", s);
  }

  if (flag_noisy)
    db_set_noisy (1);

  if (flag_no_inline_libs && mname_head[mn_subst])
  {
    /* user simply doesn't specify libraries in his subst command if he doesn't
       want to inline out of them.  */

    db_warning ("no-inline-libs is ignored when 'subst' commands are used");
    flag_no_inline_libs = 0;
  }

  if (mname_head[mn_subst] == 0 && !null_iprof)
    if (iprofc == 0 && sprofc == 0 && db_access_rok (DEFAULT_PROF_NAME))
      if (db_is_iprof (DEFAULT_PROF_NAME))
        db_set_arg (&iprofc, &iprofv, DEFAULT_PROF_NAME);
      else if (db_is_kind (DEFAULT_PROF_NAME, CI_SPF_DB))
        db_set_arg (&sprofc, &sprofv, DEFAULT_PROF_NAME);
      else
        db_fatal ("'%s' would by used by default, but it is not a profile file",
                  DEFAULT_PROF_NAME);

  db_lock ();
  dbf_open_read (&in, db_pdb_file (0, "pass1.db"), db_fatal);
  if (!db_is_kind (in.name, CI_PASS1_DB))
    db_fatal ("'%s' is corrupted or obsolete, or is not a pass1 database",
              in.name);

  dbp_read_ccinfo (&prog_db, &in);
  
  /* Count some stuff for fast_mem and call graph stuff */
  dbp_for_all_sym (&prog_db, count_var_and_func);

  /* Get profile information, if any. */
  if (iprofc || sprofc)
    dbp_get_prof_info (&prog_db, iprofc, iprofv, sprofc, sprofv);

  dbp_assign_var_counts (&prog_db);

  if (flag_print_profile_counts)
    dbp_print_profile_counts (&prog_db, &decision_file);
  
  /*
   * make decisions about which variables get allocated to fast memory.
   */
  make_fmem_decisions();

  /*
   * Now we need to make global optimization decisions for which we need
   * the call-graph. So construct it.
   */
  cg = construct_call_graph();

  /*
   * mark all nodes which have no connection with the rest of
   * the call graph as being deletable.
   */
  if (db_genv2("OLDINLINE"))
  { prune_deletable_nodes(cg);

    /* check the calls for paramater type mismatch and cut back inling
       for any that do not match.  */
    check_all_calls(cg);

    /* Incorporate profiling info into the call graph.  */
    prof_call_count (cg);

    /* generate any default profiles that may be necessary. */
    gen_default_profile(cg);

    count_dynamic_calls(cg);
    old_make_glob_inline_decisions (cg, inline_switch);
  }
  else
  {
    prune_inline_arcs (cg);
    delete_dead_cg_nodes (cg);

    /* check the calls for paramater type mismatch and cut back inling
       for any that do not match.  */
    check_all_calls(cg);

    /* Incorporate profiling info into the call graph.  */
    prof_call_count (cg);

    /* generate any default profiles that may be necessary. */
    gen_default_profile(cg);

    count_dynamic_calls(cg);
    make_glob_inline_decisions(cg, inline_switch);
  }

  /* Read the previous 2pass database, if it is available.  If we don't
     read it in, any lookups into prev_db will simply fail, which
     is generally what we want. */
  
  dbf_open_read (&prev, db_pdb_file (0, "pass2.db"), 0);
  if (prev.fd)
    if (!db_is_kind (prev.name, CI_PASS2_DB))
      db_fatal ("'%s' is corrupted or obsolete, or is not a pass2 database",
                prev.name);
    else
    { dbp_read_ccinfo (&prev_db, &prev);
      dbf_close (&prev);
    }

  /* merge prev database into the current one, set up final compile list */
  finish_subst (out.name);

  /* print the call graph with counts and such. (if requested) */
  print_cg_counts (cg, 1);

  /* print the call-graph closure (if requested) */
  print_cg_closure (cg);

  destroy_call_graph(cg);

  if (!flag_dryrun)
  { dbf_open_write (&out, db_pdb_file (0, "pass2.db"), db_fatal);

    /* Now we are committed.  If we don't finish without an error,
       remove pass2.db, because it will be corrupted. */

    db_new_removal (out.name);
    prog_db.kind = CI_PASS2_DB;
    dbp_write_ccinfo (&prog_db, &out);
    dbf_close (&out);
  }

  dbf_close (&in);

  if (flag_gcdm)
    /* If we were linker-invoked, remove the first pass database file */
    db_unlink (in.name);

  issue_compiles ();

  return 0;
}
