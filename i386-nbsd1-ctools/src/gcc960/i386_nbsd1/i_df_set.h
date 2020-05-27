#ifndef __DFSETH__
#define __DFSETH__ 1

#ifndef HINT_BITS
#define HINT_BITS (( 8 * sizeof (int) ))
#endif

typedef enum { FS_DENSE, FS_SPARSE, FS_SIZED } fl_attrs;
#define F_DENSE  ((1 << (int) FS_DENSE))
#define F_SPARSE ((1 << (int) FS_SPARSE))
#define F_SIZED  ((1 << (int) FS_SIZED))

#define FLEX_POOL_NUM(S)      ((S) >> 16)
#define FLEX_SET_NUM(S)       ((S) & 0xFFFF)
#define PACK_FLEX(PN, SN)     (((PN) << 16) | ((SN) & 0xFFFFF))

#define FLEX_POOL(S)      (( &(flex_data.flexp[FLEX_POOL_NUM(S)])   ))
#define FULL_SPARSE(S)    (full_sparse_flex(S))

typedef struct
{
  unsigned num_sets_used;
  unsigned longs_allocated;
  unsigned long* text;
  unsigned short lps;
  unsigned short attrs;
} flex_pool;

typedef flex_pool* flex_pool_ptr;

typedef unsigned long flex_set;

typedef struct
{ short first;
  short last;
  unsigned short cnt;
  flex_set body;
} sized_flex_set;

typedef struct
{
  short index;
  sized_flex_set set;
} flex_iterator;

typedef struct
{
  flex_pool flexp[256];

  int next_pid;
  int ftno;

  struct
  { flex_set ft_buf[8];
  } sets;

  struct
  { flex_pool* ft_pool[8];
  } pools;
} flex_global_info;

extern flex_global_info flex_data;


extern flex_pool* new_flex_pool();
extern flex_set reuse_flex();
extern void free_flex_pool();
extern flex_set alloc_flex();
extern sized_flex_set alloc_sized_flex();
extern flex_set get_empty_flex ();
extern flex_set get_flex();
extern int is_empty_flex();
extern int equal_flex();
extern int in_flex();
extern int next_flex();
extern int next_sized_flex();
extern void and_flex_into();
extern flex_set and_flex ();
extern void clr_flex ();
extern void and_compl_flex_into ();
extern flex_set and_compl_flex ();
extern void union_flex_into ();
extern flex_set union_flex ();
extern void set_flex_elt ();
extern void clr_flex_elt ();
extern void set_sized_flex_elt ();
extern int last_flex ();
extern flex_set copy_flex ();
extern int full_sparse_flex();
extern flex_set pick_flex_temp();
extern void dump_flex ();
extern void dump_sized_flex ();

#endif /* __DFSETH__ */
