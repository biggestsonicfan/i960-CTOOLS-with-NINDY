#include "config.h"

#ifdef IMSTG
#ifdef GCC20
#include "i_list.h"
#else
#include "list.h"
#endif

list_p
alloc_list_node (list_blk_head_p)
list_block_p * list_blk_head_p;
{
  struct list_block *first_blk = *list_blk_head_p;

  if (first_blk == 0 || first_blk->nodes_left <= 0)
  {
    first_blk = (struct list_block *) xmalloc(sizeof(struct list_block));

    first_blk->nodes_left = NODES_IN_BLK;
    first_blk->next = *list_blk_head_p;
    *list_blk_head_p = first_blk;
  }

  first_blk->nodes_left -= 1;
  return (&first_blk->nodes[first_blk->nodes_left]);
}

void
free_lists (list_blk_head_p)
list_block_p * list_blk_head_p;
{
  struct list_block *t_blk;
  struct list_block *nxt_blk;

  t_blk = *list_blk_head_p;
  while (t_blk != 0)
  {
    nxt_blk = t_blk->next;
    free(t_blk);
    t_blk = nxt_blk;
  }

 *list_blk_head_p = 0;
}
#endif
