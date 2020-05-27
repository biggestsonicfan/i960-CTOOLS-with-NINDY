
#ifdef PROCEDURE_PLACEMENT

#ifndef LDREGION_H
#define LDREGION_H 1

typedef struct region_struct {
   struct   region_struct*  next;
   unsigned int starting_addr;
   unsigned int ending_addr;
   unsigned int half_lines;
   asection*    section;
} region;

region *new_region ();

void insert_in_region_list ();

void add_regions_to_arcs ();

extern int cg_arc_cnt;

#endif
#endif
