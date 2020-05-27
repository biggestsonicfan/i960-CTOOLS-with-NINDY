/* mv2dos.c: A program to DOS-ify file names, and to run sed on 
   the DOS files to make sure that #includes use the DOS-ified names.

   Usage: mv2dos srcdir [-s filelist]
*/

#include <string.h>
#include <stdio.h>

struct Rename {
	char * oldname;
	char * newname;
} files [] = {
	{"dosproc.c",		"dosproc.c"},
	{"alloca.c",		"alloca.c"},
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
	{"cexp.c",		"cexp.c"},
	{"combine.c",		"combine.c"},
        {"convert.c",		"convert.c"},
	{"cse.c",		"cse.c"},
	{"dbxout.c",		"dbxout.c"},
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
	{"i_c_misc1.c",		"i_c_misc.c"},
        {"i_cndxfm.c",		"i_cndxfm.c"},
	{"i_cnstcomp.c",	"i_cnstco.c"},
	{"i_cnstfix.c",		"i_cnstfi.c"},
	{"i_cnstprop.c",	"i_cnstpr.c"},
	{"i_cnstreg.c",		"i_cnstre.c"},
        {"i_coerce.c",		"i_coerce.c"},
	{"i_cp_prop.c",		"i_cp_pro.c"},
	{"i_dataflow.c",	"i_datafl.c"},
	{"i_deadelim.c",	"i_deadel.c"},
	{"i_df_align.c",	"i_df_ali.c"},
	{"i_df_set.c",		"i_df_set.c"},
	{"i_double.c",		"i_double.c"},
	{"i_forw_prop.c",	"i_forw_p.c"},
	{"i_expect.c",		"i_expect.c"},
	{"i_glob_db.c",		"i_glob_d.c"},
	{"i_glob_inl.c",	"i_glob_i.c"},
	{"i_graph.c",		"i_graph.c"},
	{"i_ic960.c",		"i_ic960.c"},
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
	{"sys-protos.h",	"sys-prot.h"},
	{"stack.h",		"stack.h"},
	{"rtl.h",		"rtl.h"},
	{"reload.h",		"reload.h"},
	{"regs.h",		"regs.h"},
	{"recog.h",		"recog.h"},
	{"real.h",		"real.h"},
	{"pcp.h",		"pcp.h"},
	{"output.h",		"output.h"},
	{"obstack.h",		"obstack.h"},
	{"machmode.h",		"machmode.h"},
	{"loop.h",		"loop.h"},
	{"longlong.h",		"longlong.h"},
	{"integrate.h",		"integrat.h"},
	{"input.h",		"input.h"},
	{"i_tree.h",		"i_tree.h"},
	{"i_sparc.h",		"i_sparc.h"},
	{"i_set.h",		"i_set.h"},
	{"i_rtl.h",		"i_rtl.h"},
	{"i_profile.h",		"i_profil.h"},
        {"i_prof_form.h",	"i_prof_f.h"},
	{"i_list.h",		"i_list.h"},
	{"i_lister.h",		"i_lister.h"},
	{"i_jmp_buf_str.h",	"i_jmp_bu.h"},
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
	{"assert.h",		"assert.h"},
	{"basic-block.h",	"basic-bl.h"},

	{"insn-attr.h",		"insn-att.h"},
	{"insn-codes.h",	"insn-cod.h"},
	{"insn-config.h",	"insn-con.h"},
	{"insn-flags.h",	"insn-fla.h"},

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

	{0, 0}
};

static char buffer [1000];
static char long_hostname[] = "\"Extended DOS\"";

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
  } /* end if */

  srcdir = argv[1];
  if (argc >= 3 && strcmp(argv[2],"-s") == 0)
  {
	  first_user_file = 3;
	  snapshot = 1;
  }

  /*
    * Version files need the source time and date stamp
    */
  printf ("creating ver960.c\n");

  /* mkver.sh depends on existence of i_minrev, so copy that file first. */
  /* DO NOT put a DOS CR/LF end-of-line in i_minrev. */
  sprintf(buffer, "cp %s/i_minrev i_minrev", srcdir);
  printf ("%s\n", buffer);
  system (buffer);

  if (snapshot)
	  sprintf(buffer,"sh -x %s/mkver.sh SNAPSHOT %s",srcdir,long_hostname);
  else
	  sprintf(buffer,"sh -x %s/mkver.sh %s", srcdir, long_hostname);
  system (buffer);
  /* Now we need to dosify ver960.c */
  sprintf (buffer, "sed -f %s/newline.sed < ver960.c > mv2dos.tmp", srcdir);
  printf ("%s\n", buffer);
  system (buffer);

  printf("rm -f ver960.c"); system("rm -f ver960.c");
  printf("mv mv2dos.tmp ver960.c"); system("mv mv2dos.tmp ver960.c");

  if (argc > 3 || (argc > 2 && !snapshot))
  {
    int i = first_user_file;

    while (i < argc)
    {
      for (f = 0; files[f].oldname; ++f)
      {
        if (strcmp(argv[i], files[f].oldname) == 0)
        {
          sprintf (buffer, "sed -f %s/mv2dos.sed < %s/%s > %s",
                   srcdir, srcdir,
                   files[f].oldname, files[f].newname);
          printf ("%s\n", buffer);
          system (buffer);
          break;
        }
      }
      i++;
    }
  }
  else
  {
    /*
      * translate the new files
      */
    for ( f = 0; files[f].oldname; ++f )
    {
      sprintf (buffer, "sed -f %s/mv2dos.sed < %s/%s > %s",
                srcdir, srcdir,
                files[f].oldname, files[f].newname);
      printf ("%s\n", buffer);
      system (buffer);
    } /* end for */
  }

  printf ("creating response and bat files \n");

  sprintf (buffer, "sed -f %s/newline.sed < %s/makefile.dos > makefile",
            srcdir, srcdir);
  printf ("%s\n", buffer);
  system (buffer);

  sprintf (buffer, "sed -f %s/newline.sed < %s/cc1link.dos > cc1link.dos",
            srcdir, srcdir);
  printf ("%s\n", buffer);
  system (buffer);

  sprintf (buffer, "sed -f %s/newline.sed < %s/cpplink.dos > cpplink.dos",
            srcdir, srcdir);
  printf ("%s\n", buffer);
  system (buffer);

  sprintf (buffer, "sed -f %s/newline.sed < %s/gcclink.dos > gcclink.dos",
            srcdir, srcdir);
  printf ("%s\n", buffer);
  system (buffer);

  sprintf (buffer, "sed -f %s/newline.sed < %s/cflag.dos > cflag.dos",
            srcdir, srcdir);
  printf ("%s\n", buffer);
  system (buffer);

  sprintf (buffer, "sed -f %s/newline.sed < %s/hc.pro > hc.pro",
            srcdir, srcdir);
  printf ("%s\n", buffer);
  system (buffer);

  printf ("done\n");
  exit (0);
} /* end main */
