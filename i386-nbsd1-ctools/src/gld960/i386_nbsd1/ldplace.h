
#ifdef PROCEDURE_PLACEMENT
#ifndef LDPLACE_H
#define LDPLACE_H


#include "sysdep.h"
#include "bfd.h"
#include "ldlang.h"
#include "ldregion.h"


#define NAMED_TEXT_SECTION_PREFIX	".$"
#define IS_NAMED_TEXT_SECTION( SECT )   (!strncmp(NAMED_TEXT_SECTION_PREFIX,(SECT),strlen(NAMED_TEXT_SECTION_PREFIX)))

#define HASH_TABLE_SIZE 256



typedef struct placement_section {
   asection *section;
   lang_input_statement_type *file;
   struct placement_section *next;
} placement_section_node;


typedef struct reloc_node_struct {
   arelent *reloc;
   struct reloc_node_struct *next;
} reloc_node;


typedef struct call_graph_node_struct {
   int address;                            /* cache line number */
   unsigned int heaviest_arc_wt;           /* wt of heaviest arc in/out */
   asection *section;                      /* section containing this func */
   region *region_list;                    /* list of regions within func */
   char named_section;                     /* is this a named section ? */
   char placed;                            /* This function has been placed */
   char attached;                          /* Is there an arc attached here */
   lang_input_statement_type *file;        /* file containing this func */
   struct call_graph_node_struct *next;    /* used to build hash table */
   struct call_graph_node_struct *next_fluff;    /* used for lib fluff list */
   struct call_graph_node_struct *next_cline;    /* used for cache line list */
   struct call_graph_arc_struct  *call_out_list; /* list of called funcs */
   struct call_graph_arc_struct  *call_in_list;  /* funcs that call me */
} call_graph_node;


typedef struct call_graph_arc_struct {
   double weight;                              /* how many times was site hit */
   unsigned int arc_num;                       /* For qsort determinism */
   unsigned char profiled;                     /* is this weight from profile*/
   unsigned int offset;                        /* offset from start of caller */
   unsigned char constrained;                  /* have both ends been placed? */
   region   *call_region;                      /* addr range containing call */
   struct call_graph_arc_struct *in_arc_next;  /* ptr to next incoming arc */
   struct call_graph_arc_struct *out_arc_next; /* ptr to outgoing arc */
   struct call_graph_arc_struct *next;         /* used for hash table */
   call_graph_node *call_to;
   call_graph_node *call_fm;
} call_graph_arc;

extern boolean form_call_graph;  /* defined in ldmain.c */

void ldplace_form_call_graph();

placement_section_node *ldplace_arrange_input_sections();

void ldplace_print_call_graph();

void ldplace_print_placement_section_node();

/* int next_function (unsigned long address, lang_input_section_type *sec_p); */
int next_function ();

unsigned int hash_strings ();

#endif
#endif
