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

#include <assert.h>
#include <string.h>
#if !defined(WIN95) && !defined(__HIGHC__) && !defined(M_AP400) /* Microsoft C 9.00 (Windows 95), Metaware C or Apollo 400. */
#include <unistd.h>
#endif
#include "gcov960.h"
#include "gcovrptr.h"

extern coverage_info_type cmd_line;
extern src_list_type *src_list_head;
extern src_list_type *other_src_list_head;

/*
 * center_header(), center_header_with_width();
 *
 *     Print a string centered within the output line.
 */
static void center_header_with_width(fd, str, width)
FILE *fd;
char *str;
int width;
{
  int     left_margin = (width - (int)strlen(str) - 4) / 2;
  char    centering_fmt[20];
  sprintf (centering_fmt, "\n%%%ds %%s %%s\n\n", left_margin > 0 ? left_margin : 1);
  fprintf (fd, centering_fmt, "   ", str, "   ");
}

void center_header(fd,  str )
FILE *fd;
char *str;
{
#define LINE_WIDTH 80
  center_header_with_width(fd, str, LINE_WIDTH);
}

static void emit_version()
{
  extern gnu960_get_version();
  char *version_cp, *tmp;

  char *version = (char *)gnu960_get_version();

  assert(version);

  if (cmd_line.suppress_signon) return;
  version_cp = db_malloc(strlen(version) + 1);
  strcpy(version_cp, version);
  if (tmp = strchr(version_cp, ','))
    *tmp = '\0'; /* Remove the text after the comma */
  fprintf(stdout,"\n%s\n", version_cp);
  fprintf(stdout,"Copyright (C) 1993 Intel Corporation.  All rights reserved.\n");
  free(version_cp);
}

void display_header()
{
  emit_version();
  center_header(stdout, "Coverage Analysis");
}

static void display_header_with_width( width )
int width;
{
  emit_version();
  center_header_with_width(stdout, "Coverage Analysis", width);
}

void display_trailer(stream)
FILE *stream;
{
  int i;

  if (cmd_line.suppress_signon) 
    return;

  fprintf (stream,"\n%-25s %s\n", "Program database: ", "");

  if (cmd_line.main_iargc + cmd_line.main_sargc > 1)
    fprintf(stream, "%-25s ", "Program profiles: ");
  else
    fprintf(stream, "%-25s ", "Program profile: ");

  for (i=0; i < cmd_line.main_iargc; i++)
    fprintf (stream,"%s \n", cmd_line.main_iargv[i]);

  for (i=0; i < cmd_line.main_sargc; i++)
    fprintf (stream,"%s \n", cmd_line.main_sargv[i]);

  if (cmd_line.other_iargc + cmd_line.other_sargc > 0)
  {
    if (cmd_line.other_iargc + cmd_line.other_sargc > 1)
      fprintf (stream,"%-25s ", "Comparison profiles: ");
    else
      fprintf (stream,"%-25s ", "Comparison profile: ");

    for (i=0; i < cmd_line.other_iargc; i++)
      fprintf (stream,"%s \n", cmd_line.other_iargv[i]);
  
    for (i=0; i < cmd_line.other_sargc; i++)
      fprintf (stream,"%s \n", cmd_line.other_sargv[i]);
  }

  if (cmd_line.print_prof_total_count)
  {
    extern double total_profile_exec_count;
    fprintf(stream, "%-25s %3.2f\n", "Total of all profile counters: ",
            total_profile_exec_count);
  }
  fprintf(stream,"\n");
}

/* 
 * compute_src_cnts()
 *
 *    Fill in the counts given a source record
 */
void compute_src_cnts( S, C )
src_list_type *S;
count_type    *C;
{
  C->no_blocks = S->no_blocks;
  C->no_hits = cnt_blocks_hit(S);
  C->no_miss = C->no_blocks - C->no_hits;
  C->c1 = COVERAGE(C->no_hits,C->no_blocks);
}  

/*
 * compute_program_coverage()
 *
 *    Get a complete coverage of the entire program
 */
void compute_program_coverage(C)
count_type *C;
{
  src_list_type *src;

  memset (C, 0, sizeof(count_type));
  
  for (src = src_list_head; src != 0 ; src = src->link)
  {
    count_type src_cnt;
    compute_src_cnts(src, &src_cnt);
    C->no_blocks += src_cnt.no_blocks;
    C->no_hits += src_cnt.no_hits;
  }
  C->no_miss = C->no_blocks - C->no_hits;
  C->c1 = COVERAGE(C->no_hits, C->no_blocks);
  
}

/*
 * compute_fnc_cnts()
 *
 *   A function which is represented in a call graph node has
 *   has a basic block range (first_bb .. last_bb) associate with it.  
 *   Calculate its counts based on it basic-block range.
 */ 
void compute_fnc_cnts(F, C, src)
call_graph_node *F;
count_type  *C;
src_list_type *src;
{
  int nblock;

  memset(C, 0, sizeof(count_type));
  if (F->first_bb != -1 )
  {
    for (nblock = F->first_bb; nblock <= F->last_bb; nblock++)
      if (src->count[nblock])
        C->no_hits++;

    C->no_blocks = F->last_bb - F->first_bb + 1;
    C->no_miss = C->no_blocks - C->no_hits;

  }
  C->c1 = COVERAGE(C->no_hits, C->no_blocks);
}
     
/* 
 * display_a_coverage()
 *    
 *   Given the hits and misses, print coverage.
 */
void display_a_coverage(fd, C, coverage_kind)
FILE *fd;
count_type *C;
int coverage_kind; /* == 0 ==> program coverage */
                   /* == 1 ==> source coverage */
                   /* == 2 ==> function coverage */
{
  fprintf(fd, "%-55s %11d\n", "Number of Blocks: ", C->no_blocks);
  fprintf(fd, "%-55s %11d\n", "Number of Blocks Executed: ", C->no_hits);
  fprintf(fd, "%-55s %11d\n", "Number of Blocks Never Executed: ", C->no_miss);

  if (coverage_kind == PROGRAM_COV)
    fprintf(fd, "%-55s %11.2f%%\n",
           "Percentage of Blocks in Program that were executed:",
            COVERAGE(C->no_hits, C->no_blocks));
  else if (coverage_kind == SOURCE_COV)
    fprintf(fd, "%-55s %11.2f%%\n",
            "Percentage of Blocks in Source File that were executed:",
            COVERAGE(C->no_hits, C->no_blocks));    
  else if (coverage_kind == FUNCTION_COV)
    fprintf(fd, "%-55s %11.2f%%\n",
            "Percentage of Blocks in Function that were executed:",
            COVERAGE(C->no_hits, C->no_blocks));    
  else
    assert(0);
}
    
/*
 *  display_program_coverage()
 *
 *     Displays a summary of the entire program's coverage
 */
void display_program_coverage()
{
  count_type cov;

  src_list_type *src = find_src(cmd_line.mod_info.src_name, 
             cmd_line.mod_info.mod_name);

#define PROG_COVERAGE_WIDTH 58
  display_header_with_width(PROG_COVERAGE_WIDTH);

  compute_program_coverage( &cov);
  center_header_with_width(stdout, "Program Summary", PROG_COVERAGE_WIDTH);
  fprintf(stdout, "No.         No. Blocks  No. Blocks\n");
  fprintf(stdout, "Blocks      Hit         Missed      Coverage\n");
  fprintf(stdout, "__________  __________  __________  _________\n");
  fprintf(stdout, "%10d  %10d  %10d     %3.2f%%\n\n", cov.no_blocks, 
    cov.no_hits, cov.no_miss, cov.c1);

  display_trailer(stdout);
}


/*
 * truncate_str()
 *
 *      chops of a string to the specified size.  It will put a '*'
 *      to the end of truncated string to indicate truncation.
 *      
 *      NOTE: this is used in conjunction with cmd_line.truncate_report
 *      to truncate names and keep symbol names within column widths.
 */
char * truncate_str(str, size)
char *str;
int size; 
{
#define MAX_SYMBOL_NAME 128
  static char buf[MAX_SYMBOL_NAME]; /* should be enough */

  if (cmd_line.truncate_report)
  {
    assert(size < MAX_SYMBOL_NAME );
    if ((int)strlen(str) >= size)
    {
      strncpy(buf, str, size - 1);
      buf[size - 1] = '*';
      buf[size] = 0;
      return (buf);
    }
  }
  return (str);
}

/*
 * display_module_coverage()
 *
 *   Displays a coverage summary per module, including the coverage
 *   of the entire program.
 */

void display_module_coverage()
{
  src_list_type *src_lst;
  char *curr_src;
  count_type cov;
  count_type total;
  char cov_buf[10];

  memset(&total, 0, sizeof(count_type));

  display_header();
  center_header(stdout, "Module Summary");

  fprintf(stdout, "                                No.         No. Blocks  No. Blocks\n");
  fprintf(stdout, "Filename/Module                 Blocks      Hit         Missed      Coverage\n");
  fprintf(stdout, "______________________________  __________  __________  __________  _________\n");


  curr_src = "";
  for (src_lst = src_list_head ; (src_lst != 0) ; src_lst = src_lst->link)
  {
    if (!src_lst->selected) continue;

    if (strcmp(curr_src, src_lst->srcfile) != 0)
    {
      curr_src = src_lst->srcfile;
      fprintf(stdout, "%s:\n", truncate_str(curr_src,
                          FNC_NAME_WIDTH));
    }

    compute_src_cnts(src_lst, &cov);
    total.no_blocks += cov.no_blocks;
    total.no_hits += cov.no_hits;
    sprintf(cov_buf,"%3.2f%%", cov.c1);
    fprintf(stdout, "  %-29s %10d  %10d  %10d    %7s\n\n", 
        truncate_str(src_lst->module, FNC_NAME_WIDTH - 2),
        cov.no_blocks, cov.no_hits, cov.no_miss, cov_buf);
  }
  total.no_miss = total.no_blocks - total.no_hits;
  display_a_coverage(stdout, &total, PROGRAM_COV);
  display_trailer(stdout);
}
/*
 *  func_display()
 *     
 *     Given a call graph node (which represents a function), 
 *     this displays its coverage.
 */
static void func_display( node_p, src)
call_graph_node *node_p;
src_list_type *src;
{
  count_type fnc;
  char   *function_name;
  char cov_buf[10];

  /* count number of hits in function */
  compute_fnc_cnts(node_p, &fnc, src);

  function_name = node_p->sym->name;
  sprintf(cov_buf,"%3.2f%%", fnc.c1);
  fprintf(stdout, "%-30s  %10d  %10d  %10d    %7s\n", truncate_str(function_name, FNC_NAME_WIDTH), fnc.no_blocks,
                                fnc.no_hits, fnc.no_miss, cov_buf);
}

/*
 *  display_function_coverage()
 *
 *     This is the main driver for displaying function coverage.
 */
void display_function_coverage()
{
  call_graph_node *node_p;
  call_graph_node *quit;
  call_graph      *cg = cmd_line.cg;
  src_list_type   *src;
  count_type cov;
  count_type total;
  char cov_buf[10];
  
  display_header();
  center_header(stdout, "Function Summary");

  fprintf(stdout, "Function                        No.         No. Blocks  ");
  fprintf(stdout, "No. Blocks\n");
        fprintf(stdout, "Name                            Blocks      Hit         ");
  fprintf(stdout, "Missed      Coverage\n");
  fprintf(stdout, "______________________________  __________  __________  ");
  fprintf(stdout, "__________  _________\n");

  quit = &cg->cg_nodes[cg->num_cg_nodes];

  /* extract func info from call graph and then display functions 
         * per module 
         */
  memset(&total, 0, sizeof(total));
  memset(&cov, 0, sizeof(cov));

  for (src = src_list_head ; src != 0 ; src = src->link)
  {
    if (!src->selected) continue;

    fprintf(stdout,"\n   Source file: %s\n", src->srcfile);
    fprintf(stdout,"   Module:      %s\n\n", src->module);

    for ( node_p = cg->cg_nodes ; node_p < quit ; node_p++)
    {
      if (!node_p->traversed && strcmp(db_rec_prof_info(node_p->sym), src->module) == 0)
      {
        func_display (node_p, src);
        node_p->traversed = 1;
      }
    }
    compute_src_cnts(src, &cov);
    total.no_blocks += cov.no_blocks;
    total.no_hits += cov.no_hits;
    total.no_miss += cov.no_miss;

    sprintf(cov_buf,"%3.2f%%", cov.c1);
    fprintf(stdout, "\n%-30s  %10d  %10d  %10d    %7s\n", "Total for Module", 
        cov.no_blocks, cov.no_hits, cov.no_miss, cov_buf);
  }
  total.c1 = COVERAGE(total.no_hits, total.no_blocks);
  sprintf(cov_buf,"%3.2f%%", total.c1);
  fprintf(stdout, "\n%-30s  %10d  %10d  %10d    %7s\n", 
    FILE_SPECIFIED ? "Total for Specified Files": "Total for Program", 
    total.no_blocks, total.no_hits, total.no_miss, cov_buf);

  display_trailer(stdout);
}

/* Needed for sorting */
typedef struct
{
  int line_no;
  int block_no;
  unsigned long count_no;
  char *module;
  char *filename;
} count_info;


/* 
 *  The following compare routines are used for qsort()
 *  and bsearch()
 */

/* cnt_cmp_ascend()
 *
 *  for sorting count info in ascending order
 */
static int cnt_cmp_ascend(b1, b2)
count_info *b1, *b2;
{
  if (b1->count_no < b2->count_no)
    return -1;

  if (b1->count_no > b2->count_no)
    return 1;

  /*
   * if the counts are equal, then I always want the sort to
   * be descending in line number and block number.
   */
  if (b1->line_no < b2->line_no)
    return -1;

  if (b1->line_no > b2->line_no)
    return 1;

  if (b1->block_no < b2->block_no)
    return -1;

  if (b1->block_no > b2->block_no)
    return 1;

  if (b1 < b2)
    return -1;

  if (b1 > b2)
    return 1;

  return 0;
}

/* cnt_cmp_descend()
 *
 *  for sorting count info in descending order
 */
static int cnt_cmp_descend(b1, b2)
count_info *b1, *b2;
{
  if (b1->count_no > b2->count_no)
    return -1;

  if (b1->count_no < b2->count_no)
    return 1;

  /*
   * if the counts are equal, then I always want the sort to
   * be descending in line number and block number.
   */
  if (b1->line_no < b2->line_no)
    return -1;

  if (b1->line_no > b2->line_no)
    return 1;

  if (b1->block_no < b2->block_no)
    return -1;

  if (b1->block_no > b2->block_no)
    return 1;

  if (b1 < b2)
    return -1;

  if (b1 > b2)
    return 1;

  return 0;
}

/* line_cmp()
 *
 *  for sorting line info pointers
 */
static int line_cmp(l1, l2)
struct lineno_block_info **l1, **l2;
{
    if ((*l1)->lineno != (*l2)->lineno)
        return ((*l1)->lineno - (*l2)->lineno);
    else
    {
        if ((*l1)->column != (*l2)->column)
            return ((*l1)->column - (*l2)->column);
        else {
            if ((*l1)->blockno != (*l2)->blockno)
                return ((*l1)->blockno - (*l2)->blockno);
            else
                return 0;
        }
    }
}
 
static int bsearch_block_cmp(b1, b2)
struct lineno_block_info *b1, *b2;
{
    int v1 = b1->blockno;
    int v2 = b2->blockno;

    return (v1<v2) ? -1 : (v1>v2) ? 1 : 0;
}

static int block_cmp(b1, b2)
struct lineno_block_info *b1, *b2;
{
  if (b1->blockno != b2->blockno)
    return (b1->blockno - b2->blockno);
  else 
  {
    if (b1->lineno != b2->lineno)
      return (b1->lineno - b2->lineno);
    else {
      if (b1->column != b2->column)
        return (b1->column - b2->column);
      else
        return 0;
    }
  }
}

int src_cmp(s1, s2)
src_list_type **s1;
src_list_type **s2;
{
    int cmp_value = strcmp((*s1)->srcfile, (*s2)->srcfile);
  
  return (cmp_value == 0 ? strcmp((*s1)->module, (*s2)->module) : cmp_value);
}

/* This ugly piece of work is needed to duplicate results from
   the previous method where qsort was used once on the entire
   array. */
static
int need_replacement (want_least_freq, lcl_extr, count, block, line)
int want_least_freq;
count_info *lcl_extr;
unsigned long count;
int block;
int line;
{
  int replace = 0;  
      
  if (want_least_freq)
  {
    if (count < lcl_extr->count_no)
    {
      replace = 1;
    }
    else if (count == lcl_extr->count_no)
    {
      if (line < lcl_extr->line_no)
        replace = 1;
      else if (line == lcl_extr->line_no)
      {
        if (block < lcl_extr->block_no)
          replace = 1;
      }
    }
  }
  else  
  {
    if (count > lcl_extr->count_no)
    {
      replace = 1;
    }
    else if (count == lcl_extr->count_no)
    {
      if (line > lcl_extr->line_no)
        replace = 1;
      else if (line == lcl_extr->line_no)
      {
        if (block > lcl_extr->block_no)
          replace = 1;
      }
    }

  }

  return replace;
}

static
count_info *find_smallest(list, lsize)
count_info *list;
int lsize;
{
  count_info *tmp;
  tmp = list;
  while (lsize--)
  {
    if (tmp->count_no > list->count_no)
      tmp = list;
    list++;
  }
  return tmp;
}

static
count_info *find_biggest(list, lsize)
count_info *list;
int lsize;
{
  count_info *tmp;
  tmp = list;
  while (lsize--)
  {
    if (tmp->count_no < list->count_no)
      tmp = list;
    list++;
  }
  return tmp;
}

/*
 *  display_frequency()
 *
 *     This summarizes the most and least freguently executed
 *  blocks as specified in cmd_line. The table displayed changes
 *  formats when attaching to the source-annotated listing.
 */
void display_frequency(fd, attach_to_listing, this_src_only)
FILE *fd;
int attach_to_listing;
src_list_type *this_src_only;
{
  int i;
  register src_list_type *src_lst = src_list_head;
  count_info *cnt_info, *cnt_indx;
  count_info *lcl_extr;
  int frequency = cmd_line.frequency;
  int fill_at_start;      /* Fill cnt_info to prime the algorithm */
  char want_least_freq = 0;
  char *curr_src;
  count_type cov;
  char buf[LINE_WIDTH];

  if (!attach_to_listing)
    display_header();

  if (frequency < 0) {
    want_least_freq = 1;
    frequency = -frequency;
  }
  
  cnt_info  = (count_info *) db_malloc (sizeof(count_info) * frequency);
  cnt_indx  = cnt_info;

  memset (cnt_info, 0, sizeof(count_info) * frequency);

  fill_at_start = frequency;
  for (src_lst = src_list_head ; src_lst ; src_lst = src_lst->link)
  {
    if ((this_src_only != 0 && src_lst == this_src_only) ||
        (this_src_only == 0 && src_lst->selected))
    {
      for (i = 0 ; i < src_lst->no_lines ; i++)
      {
        if (fill_at_start)
        {
          cnt_indx->block_no = src_lst->line_info[i].blockno;
          cnt_indx->line_no  = src_lst->line_info[i].lineno;
          cnt_indx->module   = src_lst->module;
          cnt_indx->filename = src_lst->srcfile;
          cnt_indx->count_no =src_lst->count[cnt_indx->block_no];
          cnt_indx++;
          fill_at_start--;
          if (!fill_at_start)
          {
            if (want_least_freq)
              lcl_extr = find_biggest(cnt_info, frequency);
            else
              lcl_extr = find_smallest(cnt_info, frequency);
          }
        }
        else
        {
          int block = src_lst->line_info[i].blockno;
          unsigned long count = src_lst->count[block];
          int line  = src_lst->line_info[i].lineno;
          
          if (need_replacement (want_least_freq, lcl_extr,
              count, block, line))
          {
            lcl_extr->block_no = block;
            lcl_extr->line_no  = src_lst->line_info[i].lineno;
            lcl_extr->module   = src_lst->module;
            lcl_extr->filename = src_lst->srcfile;
            lcl_extr->count_no = src_lst->count[block];
            if (want_least_freq)
              lcl_extr = find_biggest(cnt_info, frequency);
            else
              lcl_extr = find_smallest(cnt_info, frequency);
          }
        }
      }
    }
  }

  /* If the user requested 10 lines but we only have */
  /* info for 9, pretend that 9 lines were specified */
  frequency -= fill_at_start;

  if (!want_least_freq)
  {
    sprintf(buf, "%d Most Frequently Executed Lines", frequency);
    center_header(fd, buf);

    /* sort by execution count in descending order*/
    qsort(cnt_info, frequency, sizeof(count_info), cnt_cmp_descend);
  }
  else
  {
    sprintf(buf,"%d Least Frequently Executed Lines", frequency);
    center_header(fd, buf);

    /* sort by execution count in ascending order*/
    qsort(cnt_info, frequency, sizeof(count_info), cnt_cmp_ascend);    
  }
  
  if (attach_to_listing)
  {
    fprintf(fd, "Block        Line         Execution\n");
    fprintf(fd, "No.          No.          Count\n");
    fprintf(fd, "__________   __________   __________\n");
  }
  else
  {
    fprintf(fd, "Filename                         Block        Line         Execution\n");
    fprintf(fd, "                                 No.          No.          Count\n");
    fprintf(fd, "______________________________   __________   __________   __________\n");
  }
  
  curr_src = "";
  for (i = 0 ; i < frequency; i++)
  {
    if (strcmp(curr_src,cnt_info[i].filename) != 0)
    {
      if (attach_to_listing)
        fprintf(fd, "%10d   %10d   %10u\n",  
            cnt_info[i].block_no, cnt_info[i].line_no, cnt_info[i].count_no);
      else
        fprintf(fd, "%-30s   %10d   %10d   %10u\n", truncate_str(cnt_info[i].filename, FNC_NAME_WIDTH), 
            cnt_info[i].block_no, cnt_info[i].line_no, cnt_info[i].count_no);
      curr_src = cnt_info[i].filename;
    }
    else
    {
      if (attach_to_listing)
        fprintf(fd, "%10d   %10d   %10u\n", cnt_info[i].block_no,
            cnt_info[i].line_no, cnt_info[i].count_no);
      else
        fprintf(fd, "%-30s   %10d   %10d   %10u\n", "", cnt_info[i].block_no,
            cnt_info[i].line_no, cnt_info[i].count_no);
    }
  }
  if (!attach_to_listing)
  {
    compute_program_coverage(&cov);
    display_a_coverage(stdout, &cov, PROGRAM_COV);
    display_trailer(stdout);
  }

  free(cnt_info);
}

/*
 * indentify_new_src_hits()
 *
 * WARNING: This routine is destructive. It will modify src_list_head;
 *
 *    This compares two profile files. Two profile files are assumed intact 
 *   in src_list_head and other_src_list_head.  This function creates a new block 
 *    count array for display out of this two profile files.  The new block-count
 *    array will be stored in src_list_head.  Missed are identified with
 *    a zero count.
 */

void identify_new_src_hits(hits_only, src1, src2)
int hits_only;
src_list_type *src1; 
src_list_type *src2;
{
  int i;
  for (i = 0 ; i < src1->no_blocks ; i++)
  {
    if (hits_only)
    {
      if ( (src1->count[i] == 0) && (src2->count[i] != 0))
        src1->count[i] = src2->count[i];
      else src1->count[i] = 0;
    }
    else
    {
      if ( (src1->count[i] != 0) && (src2->count[i] == 0))
        src1->count[i] = 0;
      else if ( (src1->count[i] == 0) && (src2->count[i] == 0))
        /* kludge: 1 here so it won't be reported as a
         * miss.
         */
        src1->count[i] =  1 ; 
      else
        src1->count[i] = src2->count[i];
    }
  }

}

void identify_new_hits(hits_only)
int hits_only;
{
  src_list_type *src1 = src_list_head;
  src_list_type *src2 = other_src_list_head;

  for ( ; src1 && src2 ; src1 = src1->link, src2 = src2->link)
  {
    if (!src1->selected) continue;
    identify_new_src_hits(hits_only, src1, src2);
  }
}

static struct lineno_block_info **lines_hit = 0;

/*
 * There is a range of blocks and lines associated with a function that have
 * been hit/miss. This function will collect these.
 */
static struct lineno_block_info **collect_lines_hit(node_p, src, hits_only)
call_graph_node *node_p;
src_list_type *src;
int hits_only;
{
  int j = 0;
  int func_block_no;


  if (node_p->first_bb == -1)
    return 0;

  if (lines_hit == 0)
  { lines_hit = (struct lineno_block_info **) db_malloc( (src->no_lines + 1) * 
             sizeof(struct lineno_block_info *));
    memset (lines_hit, 0, (src->no_lines+1) *sizeof(struct lineno_block_info*));
  }
  
  for (func_block_no = node_p->first_bb ; func_block_no <= node_p->last_bb ; func_block_no++)
  {  
    struct lineno_block_info *el, *quit;
    struct lineno_block_info key;
    
    if (hits_only)
    {
      if (src->count[func_block_no] == 0)
        continue;
    }
    else if (src->count[func_block_no] != 0)
        continue;

    key.blockno = func_block_no;
    key.lineno = 0;  
    key.column = 0;

    /* find the first entry with this key.blockno 
     * via bsearch.
     */
    el = (struct lineno_block_info *) bsearch( (char *)&key,
                (char *)src->line_info, src->no_lines,
                sizeof(struct lineno_block_info), 
                bsearch_block_cmp);
    /* Cannot find a block no in the line info.  This
                 *  implies that a specific block no was not hit, therefore
                 *  just skip.
                 */
    if (el == 0) 
      continue;

    quit = &(src->line_info[src->no_lines]);
    while ((el >= src->line_info) && (el->blockno == key.blockno))
    {
      el--; /* bsearch might put us in the middle of list of
                   * items with same block #. Find the beginning.
           */
    }
    el++;
    while ((el < quit) && (el->blockno == key.blockno))
    {
      lines_hit[j++] = el;
      el++;
    }
  }
  if (j > 1)
    qsort(lines_hit, j, sizeof(struct lineno_block_info*), line_cmp); 
   lines_hit[j] = 0 ; /* end of list */
  return lines_hit;
}

static void
lines_hit_display( node_p, src, hits_only )
call_graph_node *node_p;
src_list_type *src;
int hits_only;
{
  char *func_name;
  count_type fnc;
  struct lineno_block_info **collected_lineno;

  func_name = node_p->sym->name;
  compute_fnc_cnts(node_p, &fnc, src);

  if (hits_only)
    fprintf(stdout, "%-30s  %10d  %10d  ", truncate_str(func_name, FNC_NAME_WIDTH), fnc.no_blocks, fnc.no_hits);
  else
    fprintf(stdout, "%-30s  %10d  %10d  ", truncate_str(func_name, FNC_NAME_WIDTH), fnc.no_blocks, fnc.no_miss);
  
  collected_lineno = collect_lines_hit( node_p, src, hits_only);

  if (collected_lineno)
  {
    int prev_lineno = -1;
    struct lineno_block_info **line_ptr = collected_lineno;
    int line_cnt = 0;

    while (*line_ptr)
    {
      if (prev_lineno != (*line_ptr)->lineno)
      {
        fprintf(stdout,"%5d ", (*line_ptr)->lineno);
        line_cnt++;
      }
      if (line_cnt == 3)
      {
        fprintf(stdout, "\n");
        fprintf(stdout,"%-56s", " ");
        line_cnt = 0;
      }
      prev_lineno = (*line_ptr)->lineno;
      line_ptr++;
    }
  }
  fprintf(stdout,"\n\n");
}


void static reset_nodes()
{
  register call_graph_node *quit;
  register call_graph_node *node_p;
  register call_graph *cg = cmd_line.cg;

  quit = &cg->cg_nodes[cg->num_cg_nodes];

  for (node_p = cg->cg_nodes ; node_p < quit ; node_p++)
  {
    node_p->traversed = 0;
  }
}

/*
 * display_source_coverage()
 *
 *   Print the coverage per source and its modules within the profile file
 */
void display_source_coverage()
{
  register src_list_type *src = src_list_head;
  char *curr_src;
  char buf[MAX_SYMBOL_NAME];
  char cov_buf[10];

  display_header();
  center_header(stdout, "File and Module Coverage");
  fprintf(stdout, "\n%-30s   %-30s   %-8s\n", "Filename", "Module Name", "Coverage");
  fprintf(stdout, "______________________________   ______________________________   ________\n\n");

  curr_src = "";
  for ( ; src ; src = src->link)
  {
    count_type c;
    if (!src->selected) continue;

    compute_src_cnts(src, &c);

    sprintf(cov_buf,"%3.2f%%", c.c1);
    if (strcmp(curr_src, src->srcfile) != 0)
    {
      curr_src = src->srcfile;
      strcpy(buf, truncate_str(src->module, FNC_NAME_WIDTH));
      fprintf(stdout, "%-30s   %-30s    %7s\n", truncate_str(curr_src, FNC_NAME_WIDTH), 
        buf, cov_buf);
    }
    else
    {
      strcpy(buf, truncate_str(src->module, FNC_NAME_WIDTH));
      fprintf(stdout, "%-30s   %-30s    %7s\n", " ", buf, cov_buf);      
    }
  }
  fprintf(stdout, "\n");
}


void display_fnc_hits (hits_only)
int hits_only;
{
  register call_graph_node *node_p;
  register call_graph_node *quit;
  register call_graph *cg = cmd_line.cg;
  register src_list_type *src;

  quit = &cg->cg_nodes[cg->num_cg_nodes];

  for (src = src_list_head ; src ; src = src->link)
  {
    if (!src->selected) continue;
    qsort(src->line_info, src->no_lines, LININFO_SZ, block_cmp);
    fprintf(stdout,"\n   Source file: %s\n",  src->srcfile);
    fprintf(stdout,"   Module:      %s\n\n", src->module);
    for ( node_p = cg->cg_nodes; node_p < quit ; node_p++)
    {
      if (!node_p->traversed && !strcmp(db_rec_prof_info(node_p->sym),
                        src->module))
      {
        lines_hit_display(node_p, src, hits_only);
        node_p->traversed = 1;
      }
    }
  }
  if (lines_hit) /* destroy */
  {
    free(lines_hit);
    lines_hit = 0;
  }

}

/* 
 * clone_src_count()
 *
 *     duplicates the count array of a source record. This
 *     is used for preserving counts since a few of the routines
 *     in this module (e.g. identify_new_hits) may modify source-count array.
 */
unsigned long *clone_src_count(src)
src_list_type *src;
{
  static unsigned long *ret;

  int count_len = sizeof(int) * src->no_blocks;
  /* calculate size to allocate */
  assert (count_len);
  ret = (unsigned long *) db_malloc(count_len);
  memcpy(ret, src->count, count_len);
  return (ret);
}
   
void display_new_hits(hits_only)
int hits_only;
{
  count_type cnt;

  display_header();
  compute_program_coverage(&cnt); /* Call this first before, it get modified
                                     * by identify_new_hits()
                                     */
  identify_new_hits(hits_only);

  if (hits_only)
    center_header(stdout, "Lines Just Hit");
  else
    center_header(stdout, "Lines Just Missed");

  fprintf(stdout, "Function                        No.         No. New Blk   New Line No.\n");
  if (hits_only)
    fprintf(stdout, "Name                            Blocks      Hits          Hits\n");
  else
    fprintf(stdout, "Name                            Blocks      Missed        Missed\n");
  fprintf(stdout, "______________________________  __________  __________    ____________________\n");
  
  display_fnc_hits(hits_only);

  display_a_coverage(stdout, &cnt, PROGRAM_COV);
  display_trailer(stdout);
}

void display_hits(hits_only, both_hits_and_miss)
int hits_only;
int both_hits_and_miss; 
{

  count_type cnt;

  static char *h_header[2][3] =
  {
    "Lines Hit",
      "Function                        No.         No. Blocks    Lines.\n",
    "Name                            Blocks      Hit           Hit\n",
    "Lines Missed",
        "Function                        No.         No. Blocks    Lines\n",
        "Name                            Blocks      Missed        Missed\n"
    };

  static char *h_border = 
      "______________________________  __________  __________    ____________________\n";
    
  display_header();

  if (hits_only || both_hits_and_miss)
  {
    /* display hits */
    center_header(stdout, h_header[0][0] );
    fprintf(stdout, h_header[0][1] );
    fprintf(stdout, h_header[0][2] );
    fprintf(stdout, h_border );
    display_fnc_hits(1);
  }
  if (!hits_only || both_hits_and_miss)
  {
    if (both_hits_and_miss)
      reset_nodes();

    /* display misses */
    center_header(stdout, h_header[1][0] );
    fprintf(stdout, h_header[1][1] );
    fprintf(stdout, h_header[1][2] );
    fprintf(stdout, h_border );
    display_fnc_hits(0);
  }

  compute_program_coverage(&cnt);
  display_a_coverage(stdout, &cnt, PROGRAM_COV);
  display_trailer(stdout);
}
