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


#include "cc_info.h"
#include "i_toolib.h"

char *text[] = {
  "",
  "GNU profile file merge utility",
  "",
  "         -h: display this help message",
  "     -Z pdb: specify the program data base directory",
  "  -rprofile: display counts on stdout",
  "    -spf fn: output the merge to self-contained profile 'fn'",
  "      -v960: print version information and exit",
  "",
  "See your user's guide for a complete command-line description",
  "",
  NULL
};

/* Usage -- prints out help and usage info */
void 
usage()
{
  printf ("\nUsage:\t%s -[h | v960 | rprofile] -spf fn  file1 file2 ...\n",
          db_prog_base_name());
  paginator (text);
}

static int
del_fmt (p)
st_node* p;
{
  int ret = 0;

  if (!CI_ISFDEF(p->rec_typ) || !db_fdef_has_prof(p))
    ret = -1;
  else
    ret = ~((1<<CI_FDEF_PROF_LIST)
           |(1<<CI_FDEF_REGP_LIST)
           |(1<<CI_FDEF_CFG_LIST));

  return ret;
}

static dbase merge_db = { 0, 0, 0, del_fmt };

int flag_print_profile_counts = 0;

int
main (argc, argv)
int argc;
char **argv;
{
  static char **sprofv, **iprofv;
  int iprofc = 0;
  int sprofc = 0;
  int is_raw = 0;
  int help   = 0;

  int i;
  named_fd out;

  argc = get_response_file(argc, &argv);
  check_v960(argc, argv);

  db_set_prog (*argv);

db_set_noisy(1);

  memset (&out, 0, sizeof (out));

  if (argc == 1)
    usage();

  for (i = 1; i < argc; i++)
  { char c = argv[i][0];
    if (IS_OPTION_CHAR(c))
    {
      if (!strcmp (argv[i]+1, "h"))
      { help = 1;
        usage ();
      }

      else if (!strcmp (argv[i]+1, "t"))
        db_warning
        ("'%s' is obsolete; ascii profiles are detected automatically",argv[i]);

      else if (!strcmp (argv[i]+1, "o") || !strcmp (argv[i]+1, "spf"))
      { if (out.name)
          db_fatal ("only one output file is allowed");
        else if (i == argc-1)
          db_fatal ("%s requires an argument", argv[i]);
        else
        { is_raw = (argv[i][1] == 'o');
          out.name = argv[++i];
        }
      }

      else if (!strcmp (argv[i]+1, "Z"))
      { if (i == argc-1)
          db_fatal ("%s requires an argument", argv[i]);
        else
          db_note_pdb (argv[++i]);
      }

      else if (!strcmp (argv[i]+1, "rprofile"))
      { flag_print_profile_counts = 1;
      }

      else if (!strcmp (argv[i]+1, "v"))
        db_set_noisy (1);

      else
      { db_fatal ("unrecognized switch %s", argv[i]);
      }
    }

    else if (!db_access_rok (argv[i]))
      db_fatal ("cannot open input file '%s' for reading", argv[i]);

    else if (db_is_iprof (argv[i]))
      db_set_arg (&iprofc, &iprofv, argv[i]);

    else if (db_is_kind (argv[i], CI_SPF_DB))
      db_set_arg (&sprofc, &sprofv, argv[i]);

    else
      db_fatal ("input file '%s' is not a profile file", argv[i]);
  }

  if (help)
    exit (0);

  if (out.name == 0)
    db_fatal ("no output file was specified");

  if (iprofc == 0 && sprofc == 0)
    db_fatal ("no input files were specified");

  /* Get profile information, if any. */
  if (is_raw)
  { unsigned char* iprof_data = 0;
    int iprof_len;
    unsigned char buf[4];

    db_warning ("-o (raw profile output) is obsolete;  use -spf instead");
    if (sprofc)
      db_fatal ("self contained profile input files not allowed when -o is specified");;

    if (flag_print_profile_counts)
      db_fatal ("profile counts cannot be printed unless -spf is used");

    iprof_len = db_get_iprof_data (0, iprofc, iprofv, &iprof_data, 0);
    CI_U32_TO_BUF (buf, iprof_len);

    dbf_open_write (&out, out.name, db_fatal);
    dbf_write (&out, buf, 4);
    dbf_write (&out, iprof_data, iprof_len);
    free (iprof_data);
  }
  else
  { named_fd in;

    char* pass1 = db_pdb_file (0, "pass1.db");
    char* pass2 = db_pdb_file (0, "pass2.db");

    /* Use the newer of pass1.db, pass2.db.  Remember, linker may have
       run since gcdm960 last ran, in which case pass2.db is out of date.  */

    char* name = (db_get_mtime(pass2)>=db_get_mtime(pass1) ? pass2 : pass1);

    db_comment (0, "opening %s", name);
    dbf_open_read (&in, name, db_fatal);

    dbp_read_ccinfo (&merge_db, &in);
    dbp_get_prof_info (&merge_db, iprofc, iprofv, sprofc, sprofv);
    dbf_close (&in);

    if (flag_print_profile_counts)
    { named_fd file;
      file.name = "";
      file.fd = stdout;
      dbp_print_profile_counts (&merge_db, &file);
    }

    merge_db.kind = CI_SPF_DB;
    dbf_open_write (&out, out.name, db_fatal);
    dbp_write_ccinfo (&merge_db, &out);
  }

  dbf_close (&out);

  exit (0);
}
