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

#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include "gcov960.h"

/*  COL_BITS denotes the number of bits we give (out of 32) to column position.
  MAX_LIST_COL is based on COL_BITS.  All column positions >= MAX_LIST_COL
  are represented as MAX_LIST_COL. */

#define COL_BITS 8
#define MAX_LIST_COL (((1 << COL_BITS)-1))

#define GET_LINE(P)  (( ((unsigned)(P)) >> COL_BITS ))
#define GET_COL(P)   (( (P) & MAX_LIST_COL ))

static src_list_type
*build_prof_info (db)
dbase* db;
{
  int stabi = 0;

  src_list_type *src_head = 0;
  src_list_type *src_list = 0;
    
  for (; stabi < CI_HASH_SZ; stabi++)
  { st_node* prof_sym = db->db_stab[stabi];

    for (; prof_sym; prof_sym=prof_sym->next)
      if (prof_sym->db_rec_size && prof_sym->rec_typ == CI_PROF_REC_TYP)
      {
        st_node* fdef = 0;

        int line_cnt = 0;
        int show_prof = 0;
        int lo = -1;
        int hi = -1;

        int n_blocks, n_lines, b;
        unsigned long *counts;

        unsigned char *p, *file_name;
        struct lineno_block_info *lines;
        src_list_type* src_node;

        CI_U32_FM_BUF(prof_sym->db_rec + CI_PROF_NBLK_OFF, n_blocks);
        CI_U32_FM_BUF(prof_sym->db_rec + CI_PROF_NLINES_OFF, n_lines);
      
        src_node = (src_list_type *) db_malloc (sizeof(*src_node));
        memset (src_node, 0, sizeof(*src_node));

        file_name = CI_LIST_TEXT (prof_sym, CI_PROF_FNAME_LIST);
        src_node->srcfile  = db_malloc(strlen(file_name) + 1);
        strcpy(src_node->srcfile, file_name);

        src_node->module = db_malloc(strlen(prof_sym->name) + 1);
        strcpy(src_node->module, prof_sym->name);

        src_node->no_blocks = n_blocks;
        counts = (unsigned long*) db_malloc(sizeof(*counts) * n_blocks);
        memset (counts, 0, sizeof(*counts) * n_blocks);
        src_node->count = counts;

        src_node->no_lines = n_lines;
        lines = (struct lineno_block_info *) db_malloc(sizeof(*lines)*n_lines);
        memset (lines, 0, sizeof(*lines) * n_lines);
        src_node->line_info = lines;

        if (src_head == 0) 
        { src_head = src_node;
          src_list = src_node;
        }
        else
        { src_list->link = src_node;
          src_list = src_node;
        }

        p = file_name + strlen (file_name) + 1;

        for (b = 0; b < n_blocks; b++)
        { unsigned lineno;

          if (fdef == 0 || b < lo || b > hi)
          { fdef = dbp_prof_block_fdef (db, prof_sym, b, &lo, &hi);
            if (show_prof = db_fdef_has_prof(fdef))
              show_prof = db_fdef_prof_quality(fdef) >= cmd_line.min_quality;
          }

          assert (lo != -1);
      
          if (show_prof)
            counts[b] = db_fdef_prof_counter (fdef, b-lo);

          while ((p = db_unpk_num(p, &lineno)), lineno)
          { /* record basic block to line number correlation */
      
            if (line_cnt >= n_lines)
              db_fatal("profile information has been corrupted");
      
            lines[line_cnt].lineno = GET_LINE(lineno);
            lines[line_cnt].column = GET_COL(lineno);
            lines[line_cnt].blockno = b;
            line_cnt += 1;
          }
      
          /* skip formulas and variable info */
          while ((p = db_unpk_num(p, &lineno)), lineno) ;
          while (*p != 0) p += strlen(p)+1;
          p++;
        }
      }
  }

  return src_head;
}

src_list_type *src_list_head = 0;
src_list_type *other_src_list_head = 0;


/*
 * This function checks the source file and module pairs whether they in 
 * fact exist in the profile data base.
 */

static void verify_src_mod_pair( file_name, module_name)
char *file_name;
char *module_name;         
{
  src_list_type *src_lst = src_list_head;
  int file_found = 0;
  int module_found = 0;

  for ( ; src_lst ; src_lst = src_lst->link)
    if (!file_found)
      if (!strcmp(src_lst->srcfile, file_name))
      {
        file_found = 1;
        if (module_name && !strcmp(src_lst->module, module_name))
          module_found = 1;
      }

  if (!file_found)
    db_fatal ("Can't find filename '%s' in profile database", file_name);

  if (!module_found && module_name)
    db_fatal ("Can't find module name '%s' in profile database",
        module_name);
}

/*
 * verify that the two dynamic profile file specified came from the
 * same load module 
 */

static void verify_profiles()
{
  src_list_type *src1 = src_list_head;
  src_list_type *src2 = other_src_list_head;

  for ( ; src1, src2 ; src1 = src1->link, src2 = src2->link)
    if (strcmp(src1->srcfile, src2->srcfile))
      db_fatal ("Cannot compare profile files from different load modules");
}

/*
 * Find the source information record in src_list_head
 * associated with a filename,module_name pair
 */
src_list_type  *find_src(filename, modname)
char *filename;
char *modname;
{
  src_list_type *src;

  if ( (filename == 0))
    return 0;
  
  for (  src = src_list_head; src ; src = src->link )
  {
    if (modname)
    {
      if (!strcmp(src->srcfile, filename) && 
        !strcmp(src->module, modname))
        return src;
    }
    else if (!strcmp(src->srcfile, filename))
      return src;
  }
  return 0;
}

/*
 * cnt_blocks_hit()
 *
 *   From the count array extracted from the profile file, examine
 *   the number of hits.
 */
int cnt_blocks_hit( src_lst )
src_list_type *src_lst;
{
  int n = src_lst->no_blocks;
  int k;

  for (k=0; k < src_lst->no_blocks; k++)
    if (src_lst->count[k] == 0)
      n--;

  return n;
}

/*
 * sort_sources()
 *
 *   sort the sources that was derived from the profile database
 *   in alphabetical order. Ensure that their links are updated
 *   as well.
 */

void sort_sources()
{
  register src_list_type *src = src_list_head;
  register src_list_type **src_ptrs;
  int no_of_sources = 0;
  int i;
  extern int src_cmp();

  /* Count the source records */
  for ( ; src ; src = src->link)
    no_of_sources += 1;

  src_ptrs = (src_list_type**) db_malloc((no_of_sources+1)*sizeof(src_list_type));
  memset (src_ptrs, 0, ((no_of_sources+1)*sizeof(src_list_type)));

  for (i = 0, src = src_list_head ; src ; src = src->link, i++)
    src_ptrs[i] = src;

  qsort(src_ptrs, no_of_sources, sizeof(src_list_type *), src_cmp);
  
  /* update their links */
  for (i = 0 ; i < (no_of_sources - 1) ; i++)
    src_ptrs[i]->link = src_ptrs[i+1];

  src_list_head = src_ptrs[0];  /* new head of list */
  src_ptrs[no_of_sources - 1]->link = 0; /* end of list */

  /* Now do the same for the other specified sources */
  if (other_src_list_head)
  {
    for (i = 0, src = other_src_list_head; src ; src = src->link, i++)
      src_ptrs[i] = src;

    qsort(src_ptrs, no_of_sources, sizeof(src_list_type *), src_cmp);
  
    /* update their links */
    for (i = 0 ; i < (no_of_sources - 1) ; i++)
      src_ptrs[i]->link = src_ptrs[i+1];

    other_src_list_head = src_ptrs[0];  /* new head of list */
    src_ptrs[no_of_sources - 1]->link = 0; /* end of list */
  }
  free(src_ptrs);

}

void extract_profile_info (build_cg, perform_prof_compare)
int build_cg;
int perform_prof_compare;
{
  extern coverage_info_type cmd_line;

  mod_info_type *M = &cmd_line.mod_info;
  src_list_type *src;
  static dbase main_db, other_db;

  char* pass1 = db_pdb_file (0, "pass1.db");
  char* pass2 = db_pdb_file (0, "pass2.db");

  /* Use the newer of pass1.db, pass2.db.  Remember, linker may have
     run since gcdm960 last ran, in which case pass2.db is out of date.  */

  char* db_name = (db_get_mtime(pass2)>=db_get_mtime(pass1) ? pass2 : pass1);

  db_comment (0, "opening %s", db_name);

  { named_fd in;
    dbf_open_read (&in, db_name, db_fatal);

    dbp_read_ccinfo (&main_db, &in);
    dbp_get_prof_info (&main_db, cmd_line.main_iargc, cmd_line.main_iargv,
                                 cmd_line.main_sargc, cmd_line.main_sargv);

    src_list_head = build_prof_info (&main_db);
    dbf_close (&in);
  }

  if (cmd_line.other_iargc || cmd_line.other_sargc)
  { named_fd in;
    dbf_open_read (&in, db_name, db_fatal);

    dbp_read_ccinfo (&other_db, &in);
    dbp_get_prof_info (&other_db, cmd_line.other_iargc, cmd_line.other_iargv,
                                  cmd_line.other_sargc, cmd_line.other_sargv);

    other_src_list_head = build_prof_info (&other_db);
    dbf_close (&in);
  }

  if (perform_prof_compare)
    verify_profiles();

  if (FILE_SPECIFIED)
    /* now verify file=mode spec in command line */
    for ( ; M != 0; M = M->link)
      verify_src_mod_pair(M->src_name, M->mod_name);
  else
  {
    /* verify that there are no NULL source and module names */
    for (src = src_list_head ; src != 0 ; src = src->link)
    {
      if ((src->srcfile == 0) || (src->module == 0) ||
        (src->srcfile[0] == '\0' ) || (src->module[0] == '\0'))
        db_fatal ("%s contains null source or module names", db_name);
    }
  }

  /*  go through the command line file=module spec
   *  and mark the source records associated with them as "selected" so only
   *  thosed marked will be displayed.  */

  if (FILE_SPECIFIED)
    for (M = &cmd_line.mod_info; M != 0; M = M->link)
    { src = find_src(M->src_name, M->mod_name);
      src->selected = 1;
    }
  else /* non selected form command line, so marke all as selected */
    for (src = src_list_head ; src != 0; src = src->link)
      src->selected = 1;

  /* sort the source records */
  sort_sources(); 

  if (build_cg)
  { cmd_line.cg = construct_call_graph (&main_db);
    prof_call_count (&main_db, cmd_line.cg);
  }
}
