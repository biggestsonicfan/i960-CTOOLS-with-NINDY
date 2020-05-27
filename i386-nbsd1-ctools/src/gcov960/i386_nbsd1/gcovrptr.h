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
/* $Id: gcovrptr.h,v 1.7 1995/02/13 23:25:11 timc Exp $ */

#define COVERAGE(nhits,nblocks) \
(nblocks ? (float)nhits / (float)nblocks * 100 : 0)

/* for use with cmd_line.truncate_report  */
#define FNC_NAME_WIDTH 30

#define PROGRAM_COV  0
#define SOURCE_COV   1
#define FUNCTION_COV 2

#ifdef __STDC__
void display_header();
void display_trailer();
void display_new_hits( int );
unsigned long *clone_src_count( src_list_type *);
void display_hits (int, int );
void display_program_coverage();
void display_function_coverage();
void display_module_coverage();
void display_source_coverage();
void display_frequency( FILE *, int, src_list_type * );
void center_header( FILE *, char * );
void display_a_coverage( FILE *, count_type *, int);
void compute_program_coverage( count_type * );
void compute_src_cnts( src_list_type *, count_type *);
char *truncate_str (char *, int );
void identify_new_src_hits( int, src_list_type *, src_list_type * );

#else
void display_header(); 
void display_trailer();
void display_new_hits();
unsigned long *clone_src_count();
void display_hits();
void display_program_coverage();
void display_function_coverage();
void display_module_coverage();
void display_source_coverage();
void display_frequency();
void center_header();  
void display_a_coverage();
void compute_program_coverage();
void compute_src_cnts();
char *truncate_str();
void identify_new_hits();
#endif


