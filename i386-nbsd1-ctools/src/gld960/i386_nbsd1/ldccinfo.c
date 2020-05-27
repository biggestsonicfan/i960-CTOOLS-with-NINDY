/* Copyright (C) 1991 Free Software Foundation, Inc.
 *
 * This file is part of GLD, the Gnu Linker.
 *
 * GLD is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * GLD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GLD; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/******************************************************************************
 * This file provides support for GNU/960 2-pass compiler optimization by
 * reading the ccinfo section of all input files that have one, merging and
 * otherwise massaging it, and arranging to have it written to the output file.
 *
 * If none of the input files contain ccinfo sections, nothing is done.
 *
 * All of the routines here are static with the exception of
 * gnu960_ccinfo_build(), which should be called after linking is complete
 * but before the output file has been written.
 *****************************************************************************/

#include "sysdep.h"
#include "bfd.h"
#include "ldsym.h"
#include "ldlang.h"
#include "ldmisc.h"
#include "cc_info.h"
#ifdef PROCEDURE_PLACEMENT
#include "callsite.h"
#endif
#include <assert.h>
#include "dio.h"
#include "i_toolib.h"
#include "ldmain.h"


extern HOW_INVOKED invocation;
extern boolean option_v;
extern ld_config_type config;
extern boolean force_profiling;
extern char* CCINFO_dir;
extern char* output_filename;

static boolean is_dryrun;
static boolean is_verbose;

static int
post_rec_skip_fmt(p)
st_node *p;
{
  int typ = p->rec_typ;
  int ret = CI_REC_LIST_FMT(typ);

  /* Return a mask with 1's for all of the lists we should seek past in the
     -postlink database.  */

  switch (typ)
  {
     /* Right now, the only thing in the postlink database we care about
        is the member name list and the dindex and sindex fields from
        CI_SRC_REC records.  We can skip all other lists. */

    case CI_SRC_REC_TYP:
      ret &= ~((1<<CI_SRC_MEM_LIST));
      break;
  }
  return ret;
}

static int
prog_rec_skip_fmt(p)
st_node *p;
{
  int typ = p->rec_typ;
  int ret = CI_REC_LIST_FMT(typ);

  /* Return a mask with 1's for all of the lists we should seek past in the
     pass1 database. */

  switch (typ)
  {
     /* Right now, the only thing gld960 has to care about are the
        archive,member fields in CI_SRC records.  We can read thru
        all other lists. */

    case CI_SRC_REC_TYP:
      ret &= ~((1<<CI_SRC_LIB_LIST) | (1<<CI_SRC_MEM_LIST));
      break;
  }
  return ret;
}

static int gld_haveccinfo = 0;
static int gld_haveonly_cg_cc_info = 1;
static void gld_insert_into_hash_table();

static char* invoke_gcdm;

static void
prog_post_read (db, input, st_nodes, num_stabs)
dbase* db;
named_fd* input;
st_node* st_nodes;
{
  int i;

  bfd* abfd = (bfd *) (input->fd);

  /* This is the gld version of a routine which is called immediately
     after each file containing a database has been read, before records
     are merged. */

  for (i = 0; i < num_stabs; i++)
  {
    st_node *db_p = &st_nodes[i];

    if (db_p->rec_typ == CI_CG_REC_TYP)
      gld_insert_into_hash_table (db_p);
    else
      gld_haveonly_cg_cc_info = 0;

    /* If this entry is in a library, change the entries to indicate this.  */

    if (abfd->my_archive)
      switch (db_p->rec_typ)
      { case CI_FDEF_REC_TYP:
          db_p->rec_typ = CI_LIB_FDEF_REC_TYP;
          break;
  
        case CI_VDEF_REC_TYP:
          db_p->rec_typ = CI_LIB_VDEF_REC_TYP;
          break;
  
        case CI_LIB_VDEF_REC_TYP:
        case CI_LIB_FDEF_REC_TYP:
        case CI_VREF_REC_TYP:
        case CI_FREF_REC_TYP:
        case CI_PROF_REC_TYP:
        case CI_CG_REC_TYP:
          break;
  
        case CI_SRC_REC_TYP:
          break;

        default:
          db_fatal ("bad ccinfo record type in '%s'", abfd->filename);
      }

    if (db_p->rec_typ == CI_SRC_REC_TYP)
    { db_list_node *mem = db_p->db_list[CI_SRC_MEM_LIST];
      db_list_node *lib = db_p->db_list[CI_SRC_LIB_LIST];
      db_list_node *src = db_p->db_list[CI_SRC_SRC_LIST];

      assert (mem != 0 && lib != 0 && mem->last != 0 && lib->last != 0);

      if (mem->size == 0 && lib->size == 0)
      { char *nam = (char *)abfd->filename;
        int m;
        time_t modt;

        assert (nam && mem->next==0 && lib->next==0);

        m = strlen (nam)+1;
        mem->size = m;
        db_p->db_list_size += m;
        db_p->db_rec_size  += m;

        mem->text = (unsigned char *) db_malloc (m+4);
        CI_U32_TO_BUF (mem->text, m+4);
        memcpy (mem->text+4, nam, m);

        if (db->time_stamp)
          modt = db->time_stamp;
        else
          modt = bfd_get_mtime (abfd);

        db_set_fstat (db_p, modt);

        if (abfd->my_archive)
          nam = (char *)abfd->my_archive->filename;
        else
          nam = "";

        assert (nam);
        m = strlen (nam)+1;
        lib->size = m;
        db_p->db_list_size += m;
        db_p->db_rec_size  += m;

        lib->text = (unsigned char *) db_malloc (m+4);
        CI_U32_TO_BUF (lib->text, m+4);
        memcpy (lib->text+4, nam, m);
      }

      if (src->size)
      { /* We have a source list.  Replace it with the name of the
           object file the source came from, and the offset of the
           source from the start of the file. */

        int tell = src->tell;

        char *buf, *name;

        assert (tell);

        if (abfd->my_archive)
        { assert (abfd->origin);
          tell += abfd->origin;
          name = (char*)abfd->my_archive->filename;
        }
        else
          name = (char *)abfd->filename;

        src = db_new_list (0, strlen(name)+5);
        buf = (char*)(src->text+4);

        CI_U32_TO_BUF (buf, tell);
        strcpy (buf+4, name);

        db_replace_list (db_p, CI_SRC_SRC_LIST, src);
      }

      if (config.relocateable_output && !invoke_gcdm && !force_profiling)
        if (src->size)
        { /* Strip out the source because substitutions of pieces of
             link outputs are unimplemented.  By stripping the source here,
             we guarantee that we won't try to substitute, plus we save some
             space.  */

          static int smsg = 0;

          if (smsg == 0)
          { db_warning ("stripping '-fdb' info from -r output file '%s'", output_filename);
            smsg = 1;
          }

          db_replace_list (db_p, CI_SRC_SRC_LIST, db_new_list (0, 0));
        }
    }
  }
}

/* gld_lookup_call - finds the file where these symbol, name was defined.  */
static lang_input_section_type*
gld_lookup_call(file,name)
lang_input_statement_type *file;
char *name;
{
#ifdef PROCEDURE_PLACEMENT

    int i;
    ldsym_type *sym;

    /* Let's first look in the symbol table of the file.  Give preference here
     for local symbols. */

    for (i=0;i < file->symbol_count;i++) {
	asymbol *as = file->asymbols[i];

	if (
	    (0 == (as->flags & BSF_UNDEFINED)) &&
	    ( (as->flags & BSF_GLOBAL) || (as->flags & BSF_LOCAL) )  &&
	    ( 0 == (as->flags & BSF_DEBUGGING ) )
	    ) {
	    if (as->name && !strcmp(as->name,name)) {
		/* Found it. */
		lang_input_section_type *tmp = (lang_input_section_type *)
			ldmalloc(sizeof(lang_input_section_type));
		tmp->section = as->section;
		tmp->ifile = (lang_input_statement_type *) tmp->section->file;
		return tmp;
	    }
	}
    }

    /* Well, it is not in the scope of the file, let's look in the global symbol table: */
    sym = ldsym_get_soft(name);
    if (sym && sym->sdefs_chain) {
	lang_input_section_type *tmp = (lang_input_section_type *)
		ldmalloc(sizeof(lang_input_section_type));
	tmp->section = (*sym->sdefs_chain)->section;
	tmp->ifile = (lang_input_statement_type*) tmp->section->file;
	return tmp;
    }
	
    /* It is neither in the file nor in the global symbol table.
       It must be undefined. */
    
    return (lang_input_section_type *) 0;
#else
    return 0;
#endif
}


static void
gld_add_hash_node (in_file, in_sec, out_file, out_sec, weight, profiled)
lang_input_statement_type *in_file;
asection *in_sec;
lang_input_statement_type *out_file;
asection *out_sec;
unsigned int weight;
unsigned char profiled;
{
#ifdef PROCEDURE_PLACEMENT
   call_list *list_p;
   call_site *last_call;
   call_site *tmp_call;
   call_site *tmp_site;

   list_p = insert_or_lookup_call_site (in_file, in_sec, out_file, out_sec);

   tmp_site = (call_site *) ldmalloc (sizeof (call_site));
   tmp_site->weight   = weight;
   tmp_site->profiled = profiled;
   tmp_site->visited  = 0;
   tmp_site->next     = (call_site *) 0;

   /* Put the calls on the call list in order that we see them in. */
   if (list_p->call_site_list == (call_site *) 0) {
      tmp_site->number = 1;
      list_p->call_site_list = tmp_site;
   }
   else {
      tmp_call  = list_p->call_site_list;
      while (tmp_call)
      {
         last_call  = tmp_call;
         tmp_call   = tmp_call->next;
      }
      tmp_site->number = last_call->number + 1;
      last_call->next  = tmp_site;
   }
#endif
}

static lang_input_statement_type *gld_in_file;

static void
gld_insert_into_hash_table (db_p)
st_node *db_p;
{
  unsigned char is_profiled;
  unsigned int  my_call_count;
  unsigned int  call_count;
  char *callee_name;
  char *symbol_name;
  char *percent_pos;                 
  lang_input_section_type *caller;
  lang_input_section_type *callee;
  unsigned char *data = db_p->db_rec;;

  /* Note : symbol names for functions in call graph records are suffixed */
  /* with %cg at the end. We need to strip this off before getting sections. */
  /* Also, add the '_' prefix to the beginning of the symbol name. */

  symbol_name = (char *) ldmalloc (strlen(db_p->name) + 2);
  symbol_name[0] = '_';
  strcpy (&(symbol_name[1]), db_p->name);
  percent_pos = strrchr(symbol_name, '%');
  if (percent_pos == 0)
    return;
  (*percent_pos) = '\0';


  caller = gld_lookup_call (gld_in_file, symbol_name);
  if (!caller)
    return;

  is_profiled  = *data++;
  CI_U32_FM_BUF(data, my_call_count);
  data += 4;

  /* Now run through the functions that this guy calls. */

  while (*data != '\0') {
    callee_name = (char *) (data - 1);
    callee_name[0] = '_';
    data += strlen (callee_name);
    CI_U32_FM_BUF (data, call_count);
    data += 4;

    callee  = gld_lookup_call (gld_in_file, callee_name);
    if (callee != (lang_input_section_type *) 0)
    {
       gld_add_hash_node (caller->ifile, caller->section, 
                      callee->ifile, callee->section,
                      call_count, is_profiled);
    }
  }
}

/* gld_fill_in_section_ptr_to_parents - each section has a ptr to the
   file which contains this section. Fill in that ptr here.  */

static void
gld_fill_in_section_ptr_to_parents(f)
lang_input_statement_type *f;
{
#ifdef PROCEDURE_PLACEMENT

  asection *section_p;
  section_p = f->the_bfd->sections;
  while (section_p)
  {
    section_p->file = f;
    section_p       = section_p->next;
  }
#endif
}


static dbase prog_db = { prog_post_read, prog_rec_skip_fmt };

static void
gld_update_non_ccinfo()
{
  FOR_EACH_LDSYM(sp){
    if (sp->non_ccinfo_ref && *sp->name  == '_'){
      st_node *t = dbp_lookup_sym(&prog_db, sp->name + 1);
      if (t != 0){
	CI_U8_TO_BUF(t->db_rec + CI_ADDR_TAKEN_OFF, 1);
      }
    }
  }
}

static void
gld_read_ccinfo(f)
lang_input_statement_type *f;
{
  if (bfd_get_file_flags(f->the_bfd) & HAS_CCINFO)
  { named_fd *nf = (named_fd*) db_malloc (sizeof (named_fd));

    gld_haveccinfo = 1;
    gld_in_file = f;

    /*  Seek to start of cc info, read in the header.  */

    nf->fd = (FILE*) (f->the_bfd);
    nf->name = 0;

    if (bfd960_seek_ccinfo(f->the_bfd) <= 0 )
      db_fatal ("seek for cc_info failed in '%s'", dbf_name (nf));

    dbp_read_ccinfo (&prog_db, nf);
  }
}

static void set_gld_symbol();
extern bfd *output_bfd;
static named_fd output;

static void
gld_write_database()
{
  asection *s;

  s = bfd_get_section_by_name (output_bfd, ".text");

  set_gld_symbol ("__Stext", s->size);

  if (output.fd)
  { /* Write the database into the linker output file. */
    prog_db.kind = CI_PARTIAL_DB;
    dbp_write_ccinfo (&prog_db, &output);
  }
  else
  { /* Write the database into the pass1 database in the pdb */

    if (CCINFO_dir)
      db_note_pdb (CCINFO_dir);

    db_file_cleanup (db_pdb_file (0,"pass2.db"), CI_PASS2_DB);

    /* Open a lock file in the pdb, or die trying */
    db_lock ();

    dbf_open_write (&output, db_pdb_file(0,"pass1.db"), db_fatal);
    db_new_removal (output.name);

    prog_db.kind = CI_PASS1_DB;
    dbp_write_ccinfo (&prog_db, &output);
    dbf_close (&output);

    db_unlock();
  }
}

static dbase post_db = { 0, post_rec_skip_fmt };
static named_fd post_fd;

static int is_postlink;
static char** save_argv;
static int save_argc;

void
gnu960_set_postlink()
{
  is_postlink = -1;
}

void
gnu960_set_argc_argv (argc, argv)
int argc;
char* argv[];
{
  save_argc = argc;
  save_argv = argv;
}

void
gnu960_set_gcdm(p)
char* p;
{
  p += 4;
  if (*p != ',')
    db_fatal ("illegal gcdm specification '%s'", p);
  else
  { char *s, *arg1, *arg2;

    /* Parse the gcdm arguments, notice if noisy or dryrun */

    s = p;
    while (db_find_arg (&s, &arg1, &arg2))
      if (!strcmp (arg1, "-dryrun"))
        is_dryrun = 1;
      else if (!strcmp (arg1, "-v"))
        is_verbose = 1;

    if (invoke_gcdm == 0)
    { db_buf_at_least (&invoke_gcdm, 1 + strlen (p));
      strcpy (invoke_gcdm, p);
    }
    else
    { db_buf_at_least (&invoke_gcdm, 1 + strlen (p) + strlen (invoke_gcdm));
      strcat (invoke_gcdm, p);
    }
  }
}

typedef struct {
  named_fd f;
  int len;
  char text[CI_X960_NBUF_INT * sizeof (int)];
} db_arg_buf;

void
db_append_arg (buf, s)
db_arg_buf *buf;
char* s;
{
  int n;

  assert (s);

  n = 1 + strlen (s);

  assert ((buf->len + n) < sizeof (buf->text));

  /* db_buf_at_least (&(buf->text), buf->len + n); */
  strcpy (buf->text + buf->len, s);

  buf->len += n;
}

void
db_flush_args (buf)
db_arg_buf *buf;
{
  assert (buf->text != 0 && buf->f.fd != 0 && buf->len > 0);
  dbf_write (&(buf->f), (char *)&(buf->len), 4);
  dbf_write (&(buf->f), buf->text, buf->len);
  buf->len = 0;
}

void
gnu960_do_gcdm ()
{
  /* If /gcdm was requested, set up the command to invoke it, followed
     by an invocation of the linker with /postlink.  The commands are
     preceeded by a list of files to remove upon failure of of any
     command in the group. */

  if (invoke_gcdm && !is_postlink)
  {
    char* argv[5];
    int argc = 0;
    int i;
    char *fname;
    int  do_x960;
    static db_arg_buf buf;

    extern ld_config_type config;

    if (CCINFO_dir)
      db_note_pdb (CCINFO_dir);

    memset (argv, 0, sizeof (argv));
    memset (&buf,  0, sizeof (buf));

    fname = get_cmd_file_name();
    if (fname == 0)
    {
      fname = db_pdb_file(0,"xgld960.com");
      do_x960 = 1;
    }
    else
      do_x960 = 0;

    dbf_open_write (&(buf.f), fname, db_fatal);

    argv[argc++] = "cc_x960";
    argv[argc++] = buf.f.name;

    if (option_v || is_verbose)
      argv[argc++] = "-v";
  
    argv[argc] = 0;

    /* Calculate and write the removal list. */
    db_append_arg (&buf, output_filename);
    db_flush_args (&buf);
  
    /* Get "gcdm -Z pdb -gcdm invoke_gcdm" into the buffer ...*/
    db_append_arg (&buf, db_gcdm_name());
    db_append_arg (&buf, "-Z");
    db_append_arg (&buf, db_get_pdb());

    db_append_arg (&buf,(config.relocateable_output ?"-gcdm,ref=*:*" :"-gcdm"));
    buf.len--;	/* Want remainder of invocation in same argument */

    db_append_arg (&buf,(invocation==as_gld960 ? ",gcc960" : ",ic960"));
    buf.len--;

    db_append_arg (&buf, invoke_gcdm);
    db_flush_args (&buf);
  
    if (!is_dryrun)
    { /* Calculate and write the postlink command. */
      for (i = 0; i < save_argc; i++)
      { assert (save_argv[i]);
        db_append_arg (&buf, save_argv[i]);
      }
  
      db_append_arg (&buf, "-postlink");
      db_flush_args (&buf);
    }
  
    i = 0;
    dbf_write (&(buf.f), &i, sizeof(i));
  
    i = -1;
    dbf_write (&(buf.f), &i, sizeof (i));
    dbf_close (&(buf.f));

    fflush (stderr);
    fflush (stdout);

    /* Release the lock in the pdb so that gcdm960 can make it's own.  */
    db_unlock();

    if (do_x960)
      exit(db_x960(argc, argv));
  }
}

static unsigned long get_value();

/*  Called after linking is complete before the output file has been written. */
void
gnu960_ccinfo_build()
{

  int want_cc_info = !is_postlink &&
                     (config.relocateable_output || config.make_executable);

  if (want_cc_info)
  {
    /* load all the files into the ccinfo symbol table */
    lang_for_each_file (gld_fill_in_section_ptr_to_parents);
    lang_for_each_file (gld_read_ccinfo);

    if (!invoke_gcdm)

      /* The following test is a terrible idea.  We should put out a null
         database in the event that CCINFO_dir is known but there is no cc_info
         in the application, instead of just quietly leaving the old database
         around.  We're just asking for trouble here.  The problem is, the
         presence or absence of even 1 module with cc_info in the application
         (which could be buried in a library somewhere) will cause pass1.db to
         be written/not written.

         If pass1.db is not updated but gcdm is subsequently run, decisions
         will be made based on out of date information about the application.

         But, the behaviour of leaving the old pass1.db undisturbed (for non
         /gcdm invocations) is in the name of backward compatability.

                                                            tmc Spring 95 */

      if (gld_haveonly_cg_cc_info || !gld_haveccinfo)
        return;

    /* Make up a special database symbol to make available the text
       size to gcdm960 */

    set_gld_symbol ("__Stext", 0);

    /* check for duplicate entries and merge their data.  */
    dbp_merge_syms(&prog_db);

    /* Check for files that didn't have any information. Update
       the cc info based on any symbols that are defined or used
       in these modules.  */

    gld_update_non_ccinfo();

    output_bfd->flags |= HAS_CCINFO;

    if (BFD_ELF_FILE_P(output_bfd) && config.relocateable_output)
    {
      lang_output_section_statement_type *ccinfo_output_statement;

      dbp_pre_write (&prog_db);

      lang_enter_output_section_statement (".960.intel.ccinfo",0,
        (LDLANG_CCINFO | LDLANG_HAS_INPUT_FILES | LDLANG_HAS_CONTENTS));

      ccinfo_output_statement =
        lang_output_section_statement_lookup(".960.intel.ccinfo",1);

      init_os(ccinfo_output_statement);

      ccinfo_output_statement->bfd_section->size = prog_db.tot_sz;
    }

    if (!config.relocateable_output || invoke_gcdm || force_profiling)
    {
      /* Set up callback to output ccinfo to pass1.db. */
      assert (output.fd == 0);
    }

    else
      /* Set up callback to output ccinfo to the linker output file */
      output.fd = (FILE*) output_bfd;

    bfd960_set_ccinfo (output_bfd, gld_write_database);
  }
}

typedef struct db_symbol_rec_ {
  struct db_symbol_rec_ *next;
  char *name;
  int origin,length;
} db_symbol_rec;

static db_symbol_rec *profile_table;

void
gnu960_add_profile_symbol (name, length)
char *name;
int length;
{
  if (!config.relocateable_output || invoke_gcdm || force_profiling)
  {
    db_symbol_rec* new;

    new = (db_symbol_rec *) db_malloc (sizeof(*new));
    new->name = name;
    new->length = length;
    new->next = profile_table;

    profile_table = new;
  }
}

static unsigned long
get_value (s)
char* s;
{ ldsym_type* sym_p = ldsym_get_soft(s);

  unsigned long ret = 0;

  if (sym_p)
  { asymbol *sy = *sym_p->sdefs_chain;

    ret = sy->value;

    if (sy->section)
      ret += sy->section->output_offset + sy->section->output_section->vma;
  }
  else
    db_warning ("symbol '%s' not defined", s);

  return ret;
}

static void
set_gld_symbol (name, val)
char* name;
unsigned long val;
{
  char buf[1024];
  st_node* p;

  strcpy (buf+1, name);
  buf[0] = ci_lead_char [CI_GLD_REC_TYP];

  if ((p = dbp_lookup_sym (&prog_db, buf)) == 0)
  {
    p = dbp_add_sym (&prog_db, buf, CI_GLD_REC_FIXED_SIZE);
    assert (p != 0);
  
    p->rec_typ = CI_GLD_REC_TYP;
    p->db_rec_size = CI_GLD_REC_FIXED_SIZE;
    p->name_length = strlen (p->name);
  }

  db_set_gld_value (p, val);
}

/* Since all of the profile table is constant, we generate this into prof.map
   in the PDB.  The profile map contains an entry for each module that had a
   profile section.  Each entry contains a pointer to the module name, and an
   offset from the beginning of the profile data of the first data counter for
   that module.  These entries are terminated by an entry that contains
   2 words of 0.  Following this is the string table, which is just a buffer
   full of the module names, and is pointed into by the previously
   discussed entries.  */

void
gnu960_gen_prof_map()
{
  if (profile_table || invoke_gcdm || is_postlink)
    /* We should also remove the map anytime CCINFO_dir is set so that if there
       is no profile table, there is no map.  But, we are catering to
       backward compatability.  See the lecture in gnu960_ccinfo_build */

  {
    if (CCINFO_dir)
      db_note_pdb (CCINFO_dir);

    db_unlink (db_pdb_file (0, "prof.map"), db_fatal);
  }

  if (profile_table)
  {
    int prefix_len = strlen (CI_PROF_CTR_PREFIX);

    unsigned str_table_size = 0;
    unsigned offset_table_size  = 8;

    unsigned long prof_data_start_addr = get_value ("___profile_data_start");

    db_symbol_rec* p;
    unsigned char* buf, *buf_p;
    named_fd pass1, pmap;
    unsigned mtime;

    /* Count number of entries that will appear in table.  Start
       with 1 to take into account null entry that terminates table.  */

    for (p = profile_table; p; p = p->next)
      offset_table_size += 8;

    /* Figure out how big the string table will be.  */

    for (p = profile_table; p; p = p->next)
      str_table_size += strlen (p->name + prefix_len) + 1;

    buf   = (unsigned char*)db_malloc (8+str_table_size+offset_table_size+4);
    buf_p = buf;

    pmap.name = db_pdb_file(0,"prof.map");
    pmap.fd = 0;

    pass1.name = db_pdb_file(0,"pass1.db");
    pass1.fd = 0;

    mtime = 0;

#if 0
    /* We don't write out pass1.db now until the end, so we can't say
       anything about expected mod time. */

    if (!db_access_rok (pass1.name))
      db_warning ("writing '%s', but '%s' does not exist",pmap.name,pass1.name);

    else
      /* expected mod time of pass1.db */
      mtime = dbf_get_mtime (&pass1);
#endif

    CI_U32_TO_BUF (buf_p, mtime);
    buf_p += 4;

    { /* expected length of raw profile */
      unsigned len = get_value ("___profile_data_length");
      CI_U32_TO_BUF (buf_p, len);
      buf_p += 4;
    }

    /* Length of the rest */
    CI_U32_TO_BUF (buf_p, str_table_size+offset_table_size);
    buf_p += 4;
    
    /* Output the table entries.  Bump offset_table_size to indicate
       offset of next string in file name string table.  */

    for (p = profile_table; p; p = p->next)
    { unsigned long offset = get_value (p->name) - prof_data_start_addr;

      CI_U32_TO_BUF(buf_p, offset_table_size); 
      buf_p += 4;

      CI_U32_TO_BUF(buf_p, offset);
      buf_p += 4;

      offset_table_size += strlen (p->name + prefix_len) + 1;
    }

    /* Put the termination for the table */
    CI_U32_TO_BUF(buf_p, 0);
    buf_p += 4;
    CI_U32_TO_BUF(buf_p, 0);
    buf_p += 4;

    /* append the name string table.  */
    for ( p = profile_table; p; p = p->next)
    { strcpy((char *)buf_p, (char *)(p->name + prefix_len));
      buf_p += strlen ((char *)(p->name + prefix_len)) + 1;
    }

    dbf_open_write (&pmap, pmap.name, db_fatal);
    db_new_removal (pmap.name);

    assert (buf_p-buf == 8+offset_table_size+4);

    dbf_write (&pmap, buf, buf_p-buf);
    dbf_close(&pmap);

    free(buf);
  }
}

char*
gnu960_replace_object_file (input)
bfd* input;
{
  char* ret = 0;

  if (is_postlink && (input->flags & HAS_CCINFO))
  { st_node* s;
    unsigned char hbuf[CI_HEAD_REC_SIZE], *str, *str_base;
    named_fd f;
    unsigned tell;
    ci_head_rec head;

    if (CCINFO_dir)
      db_note_pdb (CCINFO_dir);

    if (is_postlink < 0)
    {
      /* Read in database output from gcdm960.  We delayed this so the
         pdb could be located. If we don't have it now, it's fatal. */

      if (option_v || is_verbose)
        db_set_noisy (option_v);

      /* open the lock file in the pdb, and register the lock for removal */
      db_lock();

      dbf_open_read (&post_fd, db_pdb_file(0,"pass2.db"), db_fatal);
      dbp_read_ccinfo (&post_db, &post_fd);
      dbf_close (&post_fd);
      is_postlink = 1;
    }

    f.fd = (FILE*) input;
    f.name = 0;

    tell = dbf_tell (&f);

    if (bfd960_seek_ccinfo((bfd*)f.fd) <= 0)
      db_fatal ("seek for cc_info failed in '%s'", dbf_name (&f));

    dbf_read (&f, hbuf, CI_HEAD_REC_SIZE);
    db_exam_head (hbuf, dbf_name(input), &head);

    /* seek past the data and symtab sections. */
    dbf_seek_cur (&f, head.db_size+head.sym_size);

    /* read in the string section ... */
    str = str_base = (unsigned char *) db_malloc (head.str_size);
    dbf_read (&f, str, head.str_size);

    /* reset the file pointer. */
    dbf_seek_set (&f, tell);
  
    while (head.str_size > 0 && *str != ci_lead_char[CI_SRC_REC_TYP])
    { int n = strlen((char *)str) + 1;
  
      str += n;
      head.str_size -= n;
    }
  
    if (head.str_size <= 0 || *str != ci_lead_char[CI_SRC_REC_TYP])
      db_fatal ("corrupted cc_info record in '%s'", dbf_name(&f));
  
    s = dbp_lookup_sym (&post_db, str);
    if (s == 0)
      /* We found a bname in the application that wasn't in pass2.db.
         It seems like it is most likely a user error. */
      db_warning ("'%s' has ccinfo but there is no ccinfo for it in '%s'", dbf_name(&f), post_fd.name);
    else
      if (db_is_subst (s))
      { ret = db_subst_name (0, s, 'o');
        db_comment (0, "substituted '%s' for '%s'\n", ret, dbf_name(&f));
      }
    free (str_base);
    db_unlock ();
  }
  return ret;
}

char*
dbf_name (f)
named_fd* f;
{
  if (f->name)
    return f->name;
  else
    return (char *)((bfd*)(f->fd))->filename;
}

void
dbf_seek_set (f, tell)
named_fd* f;
int tell;
{
  if (f->name == 0)
  { if (bfd_seek((bfd *)(f->fd),tell,SEEK_SET) != 0)
      db_fatal ("seek failed in '%s'", dbf_name (f));
  }
  else
    if (fseek(f->fd,tell,0) != 0)
      db_fatal ("seek failed in '%s'", f->name);
}

void
dbf_seek_cur (f, tell)
named_fd* f;
int tell;
{
  if (f->name == 0)
  { if (bfd_seek((bfd *)(f->fd),tell,SEEK_CUR) != 0)
      db_fatal ("seek failed in '%s'", dbf_name (f));
  }
  else
    if (fseek(f->fd,tell,1) != 0)
      db_fatal ("seek failed in '%s'", f->name);
}

void
dbf_seek_end (f, tell)
named_fd* f;
int tell;
{
  if (f->name == 0)
  { /* Cannot seek to end in bfd */
    assert (0);
  }
  else
    if (fseek(f->fd,tell,2) != 0)
      db_fatal ("seek failed in '%s'", f->name);
}

int
dbf_tell (f)
named_fd *f;
{
  if (f->name == 0)
    return bfd_tell ((bfd*)(f->fd));
  else
    return ftell ((FILE*)(f->fd));
}

void
dbf_read (f, p, siz)
named_fd* f;
unsigned char *p;
int siz;
{
  if (f->name == 0)
  { if (bfd_read(p,1,siz,(bfd *) (f->fd)) != siz)
      db_fatal ("read failed in '%s'", dbf_name (f));
  }
  else
  { if (fread (p,1,siz, (FILE*)(f->fd)) != siz)
      db_fatal ("read failed in '%s'", dbf_name (f));
  }
}

time_t
dbf_get_mtime (f)
named_fd* f;
{
  time_t ret;

  if (f->name == 0)
    ret = bfd_get_mtime ((bfd *)(f->fd));

  else
  { struct stat buf;
    ret = stat (f->name, &buf);
    if (ret)
      db_fatal ("cannot stat '%s'", f->name);
    ret = buf.st_mtime;
  }
  assert (ret);
  return ret;
}

void
dbf_write (f, p, len)
named_fd* f;
char *p;
int len;
{
  extern bfd* output_bfd;

  if (f->name == 0)
  { if (bfd_write(p,1,len, (bfd *)(f->fd)) != len)
      db_fatal ("write failed in '%s'", dbf_name (f));
  }
  else
  { if (fwrite (p,1,len, (FILE*)(f->fd)) != len)
      db_fatal ("write failed in '%s'", dbf_name (f));
  }
}

void
dbf_open_read (f, name, error)
named_fd* f;
char* name;
void (*error)();
{
  f->name = name;
  assert (f->name);

  if ((f->fd = fopen (f->name, FRDBIN)) == 0)
    if (error)
      error ("cannot open '%s' for read", f->name);
}

void
dbf_open_write (f, name, error)
named_fd* f;
char* name;
void (*error)();
{
  f->name = name;
  assert (f->name);

  if ((f->fd = fopen (f->name, FWRTBIN)) == 0)
    if (error)
      error ("cannot open '%s' for write", f->name);
}

void
dbf_close (f)
named_fd* f;
{
  assert (f->name);

  if (fclose ((FILE*)(f->fd)) != 0)
    db_fatal ("cannot close '%s'", f->name);

  f->fd = 0;
}

#include <stdio.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>

#include "cc_info.h"

#if defined(__STDC__)
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#if defined(__STDC__)
void
db_fatal(char *fmt, ...)
#else
void
db_fatal(va_alist)
va_dcl
#endif
{
  char buf[1024];
  va_list arg;

#if defined(__STDC__)
  va_start(arg, fmt);
#else
  char* fmt;
  va_start(arg);
  fmt = va_arg(arg, char *);
#endif
  vsprintf (buf, fmt, arg);
  va_end (arg);

  db_remove_files(0);

  info ("%P: %s\n%F", buf);
}

#if defined(__STDC__)
void
db_error(char *fmt, ...)
#else
void
db_error(va_alist)
va_dcl
#endif
{
  char buf[1024];
  va_list arg;
#if defined(__STDC__)
  va_start(arg, fmt);
#else
  char* fmt;
  va_start(arg);
  fmt = va_arg(arg, char *);
#endif
  vsprintf (buf, fmt, arg);
  va_end (arg);

  info ("%P: %s\n", buf);
}

#if defined(__STDC__)
void
db_warning(char *fmt, ...)
#else
void
db_warning(va_alist)
va_dcl
#endif
{
  char buf[1024];
  va_list arg;
#if defined(__STDC__)
  va_start(arg, fmt);
#else
  char* fmt;
  va_start(arg);
  fmt = va_arg(arg, char *);
#endif
  vsprintf (buf, fmt, arg);
  va_end (arg);

  info ("%P: %s\n", buf);
}

/* Same as `malloc' but report error if no memory available.  */

char *
db_malloc (size)
unsigned size;
{
  return ldmalloc (size);
}

char *
db_realloc (p, size)
char* p;
unsigned size;
{
  return ldrealloc (p, size);
}

