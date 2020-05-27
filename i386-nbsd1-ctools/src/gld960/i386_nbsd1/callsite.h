
#ifdef PROCEDURE_PLACEMENT

#ifndef CALLSITE_H
#define CALLSITE_H  1


#include "ldplace.h"

typedef struct call_site_struct {
   unsigned int number;
   unsigned int weight;
   unsigned char profiled;
   unsigned char visited;
   struct call_site_struct *next;
} call_site;


typedef struct call_list_struct {
   call_site                  *call_site_list;
   lang_input_statement_type  *caller_file;
   asection                   *caller_sec;
   lang_input_statement_type  *callee_file;
   asection                   *callee_sec;
   struct call_list_struct *next;
} call_list;


call_list *insert_or_lookup_call_site ();

call_site *get_next_call_site ();

void free_call_list_hash_table ();

void print_hash_table ();

#endif
#endif
