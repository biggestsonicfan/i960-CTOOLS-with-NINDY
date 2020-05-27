/*
 * Type for generic list data structure.  Managed by code in list.c.
 */

typedef struct list_node
{
  struct list_node * next;
  char *             elt;
} * list_p;

#define GET_LIST_ELT(P,TYPE) ((TYPE)((P)->elt))

#define SET_LIST_ELT(P,ELT)  ((P)->elt = (char *)(ELT))

#define NODES_IN_BLK 500

typedef struct list_block {
  struct list_block * next;
  int                 nodes_left;
  struct list_node    nodes[NODES_IN_BLK];
} * list_block_p;

extern list_p alloc_list_node();
extern void   free_list_nodes();
