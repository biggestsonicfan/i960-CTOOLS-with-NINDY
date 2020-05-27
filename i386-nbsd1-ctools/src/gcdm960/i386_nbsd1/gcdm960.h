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
#ifndef GCDM960_H
#define GCDM960_H

extern named_fd decision_file;

extern int flag_print_summary;
extern int flag_print_decisions;
extern int flag_print_variables;
extern int flag_print_call_graph;
extern int flag_print_reverse_call_graph;
extern int flag_print_closure;
extern int flag_print_profile_counts;
extern int flag_dryrun;
extern int flag_gcdm;
extern int flag_noisy;
extern int flag_no_inline_libs;
extern int flag_ic960;
extern int lines_per_page;
extern long text_size;

#endif
