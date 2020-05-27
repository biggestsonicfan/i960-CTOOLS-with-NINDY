#define TREE_GET_ALIGN_LIMIT(N) ((int)((1 << (N)->common.pragma_align) * 8))

#define TREE_SET_ALIGN_LIMIT(N,A) \
do { \
  static int prag_ali[] = { 0,1,0,2,0,0,0,3,0,0,0,0,0,0,0,4 }; \
  (N)->common.pragma_align = prag_ali[(((A)>>3)-1) & 0xf]; \
} while (0)

#define TREE_LINKAGE_INTERNAL(D) tree_linkage_internal(D)

#define IS_TREE_ASM_FUNCTION(N) \
  (( (N) != 0 && TREE_CODE(N)==FUNCTION_DECL && (N)->common.pragma_align ))
#define SET_TREE_ASM_FUNCTION(N) (( (N)->common.pragma_align = 1 ))
extern tree pick_fold_type ();
extern tree get_rtx_type();
