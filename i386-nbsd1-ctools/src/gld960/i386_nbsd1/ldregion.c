
#ifdef PROCEDURE_PLACEMENT
#include "bfd.h"
#include "ldregion.h"
#include "ldplace.h"
#include "ldcache.h"















































































































































































































































































static int
ceil (dnum)
double dnum;
{
   int inum = dnum;

   if (dnum - inum >= 0.001)
      inum++;
   return inum;
}

void
print_region (region_p)
region *region_p;
{
   printf ("Start Addr/End Addr/Sec Name/Half Lines: %u\t%u\t%s\t%u\n", 
            region_p->starting_addr,
            region_p->ending_addr,
            region_p->section->name,
            region_p->half_lines);
}


region *
new_region (start_addr, end_addr, sec)
unsigned int start_addr;
unsigned int end_addr;
asection *sec;
{
   int tmp_half_lines;
   region *tmp_ptr = (region *) ldmalloc (sizeof (region));
   tmp_ptr->starting_addr = start_addr;
   tmp_ptr->ending_addr   = end_addr;
   tmp_ptr->section       = sec;
   tmp_ptr->next          = (region *) 0;
   tmp_half_lines = ceil(((double)end_addr - (double) start_addr)
					/ ((double)cache_line_size()/2.0));
   tmp_ptr->half_lines = tmp_half_lines;
   return (tmp_ptr);
}


/*
 * insert_in_region_list - every node in the call graph has a list of regions
 *                         that it contains. These regions are a linked list
 *                         hanging off of the call graph node.
 */

void
insert_in_region_list(node_p, region_p)
call_graph_node *node_p;
region *region_p;
{
   region *tmp_region = node_p->region_list;

   /* Go through the trouble to insert regions in order: this might */
   /* come in handy later. */

   while (tmp_region && tmp_region->next)
      tmp_region = tmp_region->next;

   if (tmp_region)
      tmp_region->next = region_p;
   else
      node_p->region_list = region_p;
}



/*
 * add_regions_to_arcs - for every arc in call graph, find the region
 *                       that contains this call.
 */

void
add_regions_to_arcs (arc_array)
call_graph_arc *arc_array[];
{
   int i;
   call_graph_arc *arc_p;
   region         *region_p;
   unsigned int limit;

 
   /* limit is used to tell us when to stop looking for bigger regions: */
   /* cache_line_size returns sizes in words, */
   /* so if a region is "limit" sized, it will take up a fourth of the cache */
   limit = cache_line_size() * number_of_cache_lines() / 2;

   for (i = 0; i < cg_arc_cnt; i++)
   {
      arc_p    = arc_array[i];
      region_p = arc_p->call_fm->region_list;
      while (region_p)
      {
         if (arc_p->offset < region_p->ending_addr &&
             arc_p->offset >= region_p->starting_addr)
         {
            if (arc_p->call_region == (region *) NULL) 
            {
               arc_p->call_region = region_p;
            }
            else 
            {
               /* We have region already, look for a bigger one */
               if (region_p->ending_addr - region_p->starting_addr <= limit)
                  arc_p->call_region = region_p;
               else
                  break;
            }
         }
         region_p = region_p->next;
      }
   }
}
#endif
