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
/* $Id: gcov960.h,v 1.9 1995/02/13 23:25:11 timc Exp $ */

/*
 * gcov960.h
 *
 *     Structs, typedefs, and prototypes for extracting data base info.
 */
#include "cc_info.h"
#include "gcovgrph.h"

struct prof_data {
  int executed;
  int dont_care;
};

struct prof_file_offset {
  int file_name_index;
  int file_offset;
};

typedef struct prof_data *PROF_DATA, PROF_DATA_STRUCT;
typedef struct prof_file_offset *PROF_FILE_OFFSET, PROF_FILE_OFFSET_STRUCT;

typedef struct src_annot
{
  char *srcfile;
  char *module;
  int no_lines;
  struct lineno_block_info *line_info;
  int line_info_sorted; /* == 1 if line_info is sorted */
  unsigned long *count;
  int no_blocks;
  char selected; /* selected for display, per command line */
  struct src_annot *link;
} src_list_type;

#define SRCINFO_SZ sizeof(src_list_type)

extern src_list_type *src_list_head;
extern src_list_type *other_src_list_head;


/* This structure used to record coverage info. The idea
 * here is gather info for source file and modules
 */

typedef struct m_info_type {
  char *src_name;
  char *mod_name;
  struct m_info_type *link;
} mod_info_type;

typedef struct {

  int  main_iargc;
  int  main_sargc;
  char **main_iargv;
  char **main_sargv;

  int  other_iargc;
  int  other_sargc;
  char **other_iargv;
  char **other_sargv;

  char *dir_name;    /* source directory */
  int  frequency;    /* how much to display, e.g. top 10 or bottom 10? */
  char truncate_report; /* truncate names to keep symbol names within column */
  char suppress_signon; /* suppress copyright and version display on reports */
  char print_prof_total_count; /* print profile total exec count */
  call_graph *cg;    /* call graph for the profile data */
  mod_info_type mod_info; 
  int min_quality;	/* Show all profiles >= this quality */
} coverage_info_type;

extern coverage_info_type cmd_line;
#define FILE_SPECIFIED (cmd_line.mod_info.src_name != NULL)

#define COVINFO_SZ sizeof (coverage_info_type)
#define MODINFO_SZ sizeof (mod_info_type)

/* info for doing source-annotation */

/* line info */
struct lineno_block_info
{
  int lineno;
  int column;
  int blockno;
};

#define LININFO_SZ sizeof(struct lineno_block_info)

/* This will hold the complete list of directories specified with -I
   on the command line.  If no directories are specified, note that
   the list will have exactly one element in it: "." */

typedef struct directorylist {
  char *name;
  struct directorylist *next;
} directory_list;

extern directory_list dir_list_head;

typedef struct computed_counts
{
  int no_blocks;
  int no_hits;
  int no_miss;
  float c1;
} count_type;

extern src_list_type *find_src();
extern int cnt_blocks_hit();

