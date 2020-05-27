#include <string.h>
#include <stdio.h>

/* List all the files that must be copied into the DOS working directory.
 */

struct Rename {
	char * oldname;
	char * newname;	/* Name conforming to DOS 8.3 convention. */
} files [] = {
	{"alloca.c",		"alloca.c"},
	{"bc-emit.c",		"bc-emit.c"},
	{"bc-optab.c",		"bc-optab.c"},
	{"c-aux-info.c",	"c-aux-in.c"},
	{"c-common.c",		"c-common.c"},
	{"c-convert.c",		"c-conver.c"},
	{"c-decl.c",		"c-decl.c"},
	{"c-iterate.c",		"c-iter.c"},
	{"c-lang.c",		"c-lang.c"},
	{"c-lex.c",		"c-lex.c"},
	{"c-parse.c",		"c-parse.c"},
	{"c-typeck.c",		"c-typeck.c"},
	{"caller-save.c",	"caller-s.c"},
	{"calls.c",		"calls.c"},
	{"cccp.c",		"cccp.c"},
	{"cc_dinfo.c",		"cc_dinfo.c"},
	{"cc_linfo.c",		"cc_linfo.c"},
	{"cexp.c",		"cexp.c"},
	{"combine.c",		"combine.c"},
        {"convert.c",		"convert.c"},
	{"cse.c",		"cse.c"},
	{"dbxout.c",		"dbxout.c"},
	{"dwarfout.c",		"dwarfout.c"},
	{"getpwd.c",		"getpwd.c"},
	{"emit-rtl.c",		"emit-rtl.c"},
	{"explow.c",		"explow.c"},
	{"expmed.c",		"expmed.c"},
	{"expr.c",		"expr.c"},
	{"final.c",		"final.c"},
	{"flow.c",		"flow.c"},
	{"fold-const.c",	"fold-con.c"},
	{"function.c",		"function.c"},
	{"gcc960.c",		"gcc960.c"},
	{"global.c",		"global.c"},
	{"i_asm_func.c",	"i_asmfnc.c"},
	{"i_bbr.c",		"i_bbr.c"},
	{"i_c_misc1.c",		"i_c_misc.c"},
        {"i_cndxfm.c",		"i_cndxfm.c"},
	{"i_cnstfix.c",		"i_cnstfi.c"},
	{"i_cnstprop.c",	"i_cnstpr.c"},
        {"i_coerce.c",		"i_coerce.c"},
	{"i_cp_prop.c",		"i_cp_pro.c"},
	{"i_dataflow.c",	"i_datafl.c"},
	{"i_deadelim.c",	"i_deadel.c"},
	{"i_df_align.c",	"i_df_ali.c"},
	{"i_df_set.c",		"i_df_set.c"},
	{"i_double.c",		"i_double.c"},
	{"i_dwarf2.c",		"i_dwarf2.c"},
	{"i_dwarf2.def",	"i_dwarf2.def"},
	{"i_dwarf2.h",		"i_dwarf2.h"},
	{"i_forw_prop.c",	"i_forw_p.c"},
	{"i_expect.c",		"i_expect.c"},
	{"i_glob_db.c",		"i_glob_d.c"},
	{"i_glob_inl.c",	"i_glob_i.c"},
	{"i_graph.c",		"i_graph.c"},
	{"i_ic960.c",		"i_ic960.c"},
	{"i_input.c",		"i_input.c"},
	{"i_list.c",		"i_list.c"},
	{"i_lister.c",		"i_lister.c"},
	{"i_lutil.h",		"i_lutil.h"},
	{"i_lutil.def",		"i_lutil.def"},
	{"i_misc1.c",		"i_misc1.c"},
	{"i_mrd_info.c",	"i_mrd_in.c"},
        {"i_pic.c",		"i_pic.c"},
	{"i_prof_form.c",	"i_prof_f.c"},
	{"i_profile.c",		"i_profil.c"},
	{"i_sblock.c",		"i_sblock.c"},
	{"i_sched.c",		"i_sched.c"},
	{"i_shadow.c",		"i_shadow.c"},
	{"i_shadow2.c",		"i_shad2.c"},
	{"i_size.c",		"i_size.c"},
	{"i_symwalk.c",		"i_symwal.c"},
	{"integrate.c",		"integrat.c"},
	{"jump.c",		"jump.c"},
	{"local-alloc.c",	"local-al.c"},
	{"loop.c",		"loop.c"},
	{"obstack.c",		"obstack.c"},
	{"optabs.c",		"optabs.c"},
	{"print-rtl.c",		"print-rt.c"},
	{"print-tree.c",	"print-tr.c"},
        {"real.c",		"real.c"},
	{"recog.c",		"recog.c"},
	{"regclass.c",		"regclass.c"},
	{"reload.c",		"reload.c"},
	{"reload1.c",		"reload1.c"},
	{"rtl.c",		"rtl.c"},
	{"rtlanal.c",		"rtlanal.c"},
	{"sched.c",		"sched.c"},
	{"sdbout.c",		"sdbout.c"},
	{"stmt.c",		"stmt.c"},
	{"stor-layout.c",	"stor-lay.c"},
	{"stupid.c",		"stupid.c"},
	{"toplev.c",		"toplev.c"},
	{"tree.c",		"tree.c"},
	{"unroll.c",		"unroll.c"},
	{"varasm.c",		"varasm.c"},
	{"version.c",		"version.c"},

	{"xcoffout.h",		"xcoffout.h"},
	{"va-i960.h",		"va-i960.h"},
	{"typeclass.h",		"typeclas.h"},
	{"tree.h",		"tree.h"},
	{"sys-types.h",		"sys-type.h"},
	{"stack.h",		"stack.h"},
	{"rtl.h",		"rtl.h"},
	{"reload.h",		"reload.h"},
	{"regs.h",		"regs.h"},
	{"recog.h",		"recog.h"},
	{"real.h",		"real.h"},
	{"pcp.h",		"pcp.h"},
	{"output.h",		"output.h"},
	{"obstack.h",		"obstack.h"},
	{"modemap.def",		"modemap.def"},
	{"machmode.h",		"machmode.h"},
	{"loop.h",		"loop.h"},
	{"longlong.h",		"longlong.h"},
	{"integrate.h",		"integrat.h"},
	{"input.h",		"input.h"},
	{"i_yyltype.h",		"i_yyltyp.h"},
	{"i_tree.h",		"i_tree.h"},
	{"i_sparc.h",		"i_sparc.h"},
	{"i_set.h",		"i_set.h"},
	{"i_rtl.h",		"i_rtl.h"},
	{"i_profile.h",		"i_profil.h"},
        {"i_prof_form.h",	"i_prof_f.h"},
	{"i_list.h",		"i_list.h"},
	{"i_lister.h",		"i_lister.h"},
	{"i_jmp_buf_str.h",	"i_jmp_bu.h"},
	{"i_icopts.def",	"i_icopts.def"},
	{"i_graph.h",		"i_graph.h"},
	{"i_glob_db.h",		"i_glob_d.h"},
	{"i_extensions.h",	"i_extens.h"},
	{"i_double.h",		"i_double.h"},
	{"i_dfprivat.h",	"i_dfpriv.h"},
	{"i_df_set.h",		"i_df_set.h"},
	{"i_dataflow.h",	"i_datafl.h"},
	{"i_basic-block.h",	"i_basic-.h"},
        {"i_asm_func.h",	"i_asmfnc.h"},
	{"hard-reg-set.h",	"hard-reg.h"},
	{"halfpic.h",		"halfpic.h"},
	{"va-metware.h",	"gvarargs.h"},
	{"gsyms.h",		"gsyms.h"},
	{"gstddef.h",		"gstddef.h"},
	{"gstdarg.h",		"gstdarg.h"},
	{"gstab.h",		"gstab.h"},
	{"function.h",		"function.h"},
	{"flags.h",		"flags.h"},
	{"expr.h",		"expr.h"},
	{"dwarf.h",		"dwarf.h"},
	{"defaults.h",		"defaults.h"},
	{"dbxstclass.h",	"dbxstcla.h"},
	{"convert.h",		"convert.h"},
	{"conditions.h",	"conditio.h"},
	{"c-tree.h",		"c-tree.h"},
	{"c-parse.h",		"c-parse.h"},
	{"c-lex.h",		"c-lex.h"},
	{"c-gperf.h",		"c-gperf.h"},
	{"bytecode.h",		"bytecode.h"},
	{"bi-run.h",		"bi-run.h"},
	{"bc-typecd.h",		"bc-typec.h"},
	{"bc-typecd.def",	"bc-typec.def"},
	{"bc-optab.h",		"bc-optab.h"},
	{"bc-opcode.h",		"bc-opcod.h"},
	{"bc-emit.h",		"bc-emit.h"},
	{"bc-arity.h",		"bc-arity.h"},
	{"basic-block.h",	"basic-bl.h"},
	{"assert.h",		"assert.h"},

	{"insn-attr.h",		"insn-att.h"},
	{"insn-codes.h",	"insn-cod.h"},
	{"insn-config.h",	"insn-con.h"},
	{"insn-flags.h",	"insn-fla.h"},

        {"i_copreg.h",		"i_copreg.h"},

	{"insn-attrtab.c",	"insn-att.c"},
	{"insn-emit.c",		"insn-emi.c"},
	{"insn-extract.c",	"insn-ext.c"},
	{"insn-output.c",	"insn-out.c"},
	{"insn-peep.c",		"insn-pee.c"},
	{"insn-recog.c",	"insn-rec.c"},
        {"insn-opinit.c",	"insn-opi.c"},

	{"machmode.def",	"machmode.def"},
	{"rtl.def",		"rtl.def"},
	{"tree.def",		"tree.def"},
	{"gstab.def",		"gstab.def"},

	{"xm-i386.h",		"xm-i386.h"},
	{"xm-i386dos.h",	"config.h"},
	{"i960.md",		"md"},
	{"i960.h",		"tm.h"},
	{"i960.c",	        "aux-outp.c"},

	{"_version",	        "_version"},

	{"makefile.dos",        "makefile"},
	{"makefile.win",        "makefile.win"},

	{0, 0}
};

static char buffer [1000];

char sedfile[] = "mv2dos.sed";

void
do_command(cmd)
char *cmd;
{
  printf ("%s\n", cmd);
  if (system(cmd) != 0)
  {
    fprintf(stderr, "ERROR -- Failed executing command `%s'\n", cmd);
    exit(1);
  }
}

main (argc, argv)
	int argc;
	char * * argv;
{
  int f;			/* current file being processed */
  char *srcdir;
  int	snapshot = 0;
  int	first_user_file = 2;

  if (argc < 2)
  {
    printf ("usage: mv2dos srcdir [-s] [files]\n");
    exit (1);
  }

  srcdir = argv[1];
  if (argc >= 3 && strcmp(argv[2],"-s") == 0)
  {
	  first_user_file = 3;
	  snapshot = 1;
  }

  /* Create a sed script to be run over each file.  The script will
     change #include "oldfilename" to #include "newfilename", and
     will add a carriage return before the end of line.
   */

  {
    FILE *fp;
    if ((fp = fopen(sedfile, "w")) == NULL)
    {
      fprintf(stderr, "ERROR -- Unable to create sed script `%s'\n", sedfile);
      exit(1);
    }

    for (f = 0; files[f].oldname; f++)
    {
      if (strcmp(files[f].oldname, files[f].newname) != 0)
      {
	char	*cp;

	/* Create a sed script line of the form
		s/include[ \t]*"imtoolong\.h"/include "imtoolon.h"/
	   For completeness, put a backslash before each '.' in oldname.
	 */

        fprintf(fp, "s/include[ \t]*\"");
	for (cp = files[f].oldname; *cp; cp++)
	{
	  if (*cp == '.')
	    fprintf(fp, "\\.");
	  else
	    fprintf(fp, "%c", *cp);
	}
	fprintf(fp, "\"/include \"%s\"/\n", files[f].newname);
      }

    }

    /* Now add a sed script line that puts CR before end-of-line. */

    fprintf(fp, "s/$/%c/\n", 13);
    fclose(fp);
  }


  /*
    * Version files need the source time and date stamp
    */
  printf ("creating ver960.c\n");

  /* mkver.sh depends on existence of i_minrev, so copy that file first. */
  /* DO NOT put a DOS CR/LF end-of-line in i_minrev. */
  sprintf(buffer, "cp %s/i_minrev i_minrev", srcdir);
  do_command(buffer);

  if (snapshot)
	  sprintf(buffer,"sh -x %s/mkver.sh SNAPSHOT",srcdir);
  else
	  sprintf(buffer,"sh -x %s/mkver.sh", srcdir);
  do_command(buffer);

  /* Now we need to dosify ver960.c */
  sprintf (buffer, "sed -f %s < ver960.c > mv2dos.tmp", sedfile);
  do_command(buffer);

  do_command("rm -f ver960.c");
  do_command("mv mv2dos.tmp ver960.c");

  if (argc > 3 || (argc > 2 && !snapshot))
  {
    /*
     * translate just the specified files
     */
    int i = first_user_file;

    while (i < argc)
    {
      for (f = 0; files[f].oldname; ++f)
      {
        if (strcmp(argv[i], files[f].oldname) == 0)
        {
          sprintf (buffer, "sed -f %s < %s/%s > %s",
                   sedfile, srcdir, files[f].oldname, files[f].newname);
          do_command (buffer);
          break;
        }
      }
      i++;
    }
  }
  else
  {
    /*
     * translate all files
     */
    for ( f = 0; files[f].oldname; ++f )
    {
      sprintf (buffer, "sed -f %s < %s/%s > %s",
                sedfile, srcdir, files[f].oldname, files[f].newname);
      do_command (buffer);
    }
  }

  sprintf(buffer, "rm -f %s", sedfile);
  do_command(buffer);

  printf ("done\n");
  return 0;
}
