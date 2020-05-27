#include "config.h"

#ifdef IMSTG
#include "tree.h"
#include "cp-tree.h"

tree decl;
int  toplevel;

int tree_linkage_internal(decl)
tree decl;
{
  tree outer = IDENTIFIER_GLOBAL_VALUE(DECL_NAME(decl));

  if (outer)
    return !TREE_PUBLIC(outer);
  else
    return !TREE_PUBLIC(decl);
}
#endif
