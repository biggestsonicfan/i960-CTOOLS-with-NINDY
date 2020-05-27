
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

#include <stdio.h>
#include <string.h>
#include "gcov960.h"
#include "gcovrptr.h"
#include "i_toolib.h"


#define MAX_CNT_COL    10
#define MAX_EXPRS_PER_LINE   60  /* how many basic blks can 1 line generate */

typedef struct {
  unsigned line_number;
  unsigned col_number;
  unsigned block_number;
  unsigned long count;
} line_block_count;

static int
cmp_lbc(left,right)
line_block_count *left,*right;
{
  if (left->line_number != right->line_number)
    return left->line_number - right->line_number;
  else {
    if (left->col_number != right->col_number)
      return left->col_number - right->col_number;
    else {
      if (left->block_number != right->block_number)
        return left->block_number - right->block_number;
      else
        return 0;
    }
  }
}

/*
 * find_duplicate_filenames()
 *
 *     This function is used to warn for duplicate sources, since this
 *     will affect how we're going to generate the .cov source
 *     annotated files. Fortunately, the source records are already
 *     sorted, which makes finding easier.
 */
static void find_duplicate_filenames()
{
  src_list_type *lst;
  
  for (lst = src_list_head; lst ; lst = lst->link)
  {
    if (lst->link)
    {
      if (lst->selected && lst->link->selected && 
        (strcmp(lst->srcfile, lst->link->srcfile) == 0))
      {
        fprintf(stderr, "WARNING: More than one source file named '%s'", lst->srcfile );
      }
    }
  }
  
}
/*
 * gcov_src_report()
 *
 *      This functions insert "####### " for misses, "+++++++ " for
 *      hits, and execution count in source files.
 */

#define MISSED   "######## "
#define JUST_HIT "++++++++ "

void gcov_src_report(compare, hits_only, both_hits_and_misses, display_top_n)
  int compare; /* true means we're doing profile file compare */
  int hits_only; /* report hits only if true */
    int both_hits_and_misses; /* report both hits and misses, currently
                                 both hits and misses are not allowed
                                 when -n (compare) is specified */
  int display_top_n;
{
  extern src_list_type *src_list_head;
  extern src_list_type *other_src_list_head;

  src_list_type *src = src_list_head;
  src_list_type *src2 = compare ? other_src_list_head: src_list_head;
  unsigned long *saved_count;

  int i;
  unsigned long *cnt;
  count_type cov;

  static char *src_file_buff;

  line_block_count *lbc_array;
  
  find_duplicate_filenames();

  for (; src ; src = src->link, src2 = src2->link) 
  {
    directory_list *p;
    int found_src_file = 0;
    
    /* user did not select this so skip */
    if (!src->selected) 
      continue;

    if (compare) 
    {
      saved_count = src->count;
      src->count = clone_src_count(src);
      /* Mark new hits or misses */
      identify_new_src_hits ( hits_only, src, src2 );
    }
    
    for (p=&dir_list_head; !found_src_file && p; p = p->next) 
    {
      FILE *src_file,*rpt_file;
      
      int l = strlen(p->name)+strlen(src->srcfile)+2;

      db_buf_at_least (&src_file_buff, l);
      
      path_concat(src_file_buff, p->name, src->srcfile);

      if (src_file=fopen(src_file_buff,"r")) 
      {
        int line_count = 1;
        char buff[1024],*temp;

        int lbc_csize = 0,
            search_index = 0,
            just_hits = 0,
            just_missed = 0;
        
        found_src_file = 1;
        sprintf(src_file_buff,"%s", src->srcfile);
        if (temp=(char*)strrchr(src_file_buff, '.'))
          *temp = 0;
        strcat(src_file_buff, ".cov");
        if (rpt_file=fopen(src_file_buff, "w")) 
        {
          lbc_array = (line_block_count *)
            db_malloc (src->no_lines * sizeof(line_block_count));

          memset (lbc_array, 0, src->no_lines * sizeof(line_block_count));

          for (i=0, cnt=src->count ; i < src->no_lines ; lbc_csize++, i++) 
          {
            lbc_array[lbc_csize].line_number = src->line_info[i].lineno;
            lbc_array[lbc_csize].col_number = src->line_info[i].column;
            lbc_array[lbc_csize].block_number = src->line_info[i].blockno;
            lbc_array[lbc_csize].count = cnt[src->line_info[i].blockno];
          }
          qsort(lbc_array,lbc_csize,sizeof(line_block_count),cmp_lbc);
          
          while (fgets(buff,sizeof(buff),src_file)) {
            int exprs_on_this_line = 0;
            int count_buff_len;
            char count_column[MAX_CNT_COL*2];
            char count_buff[MAX_CNT_COL*MAX_EXPRS_PER_LINE];
            count_buff[0] = '\0';

            while (search_index < lbc_csize &&
                 lbc_array[search_index].line_number == line_count) 
            {
              if (exprs_on_this_line > MAX_EXPRS_PER_LINE)
              {
                search_index++;
                continue;
              }

              /* if we're doing profile comparison (i.e. -n was specified)
                                     *  then we're only interested in displaying hits or misses */
              if (compare)
              {
                  if (lbc_array[search_index].count) 
                {
                  if (hits_only)
                  {
                    just_hits++;
                    sprintf(count_column, JUST_HIT);
                    strcat(count_buff, count_column);
                  }
                }
                else if (!hits_only)
                {
                  just_missed++;
                  sprintf(count_column, MISSED);
                  strcat(count_buff, count_column);
                }
              }
              else if (hits_only)
              {
                if (lbc_array[search_index].count)
                {
                    sprintf(count_column, "%u ", lbc_array[search_index].count);
                  strcat(count_buff, count_column);
                }
                
              }
              else if (both_hits_and_misses)
              {
                if (lbc_array[search_index].count)
                {
                  sprintf(count_column,"%u ",   lbc_array[search_index].count);
                  strcat(count_buff, count_column);
                }
                else
                {
                    sprintf(count_column, MISSED);
                  strcat(count_buff, count_column);
                }
              }
              else if (lbc_array[search_index].count == 0)
              {
                sprintf(count_column, MISSED);
                strcat(count_buff, count_column);
              }
              exprs_on_this_line++;
              search_index++;
            }
            line_count++;

            count_buff_len = strlen(count_buff);
            if (count_buff_len)
            {
              if (count_buff_len >= MAX_CNT_COL)
                fprintf(rpt_file,"%s-> %s", count_buff, buff);
              else
                fprintf(rpt_file,"%10s-> %s", count_buff, buff);
            }
            else fprintf(rpt_file,"%*c    %s", MAX_CNT_COL, ' ', buff);
          }
          fclose(src_file);
          fprintf(rpt_file,"\n\n");
          if (compare)
          {
            if (hits_only)
            {
              fprintf(rpt_file, "%-55s %11d\n",
                      "Number of lines hit only by comparison profile(s):   ",
                       just_hits);
            }
            else
            {
              fprintf(rpt_file, "%-55s %11d\n",
                      "Number of lines missed only by comparison profile(s):",
                      just_missed);
            }

            free(src->count);
            src->count = saved_count;
          }
          compute_src_cnts(src, &cov); 
          display_a_coverage(rpt_file, &cov, SOURCE_COV);
          display_trailer(rpt_file);
          if (display_top_n)
          {
            /* display top n executed list */
            fprintf(rpt_file,"\n");
            display_frequency(rpt_file, 1, src);
            fprintf(rpt_file,"\n");
          }
          fclose(rpt_file);
          free(lbc_array);
        }  /* if fopen( report file ) */
        else
          perror(src_file_buff);
      } /* if ( fopen ( source file ) */
    }  /* for ( directory search list ...) */


    if (!found_src_file) 
    {
      printf("Warning: can not find source file: %s\n",src->srcfile);
      printf("Suggest using the -I option.\n");
    }
  }   /* for ( source file list ... ) */
}






