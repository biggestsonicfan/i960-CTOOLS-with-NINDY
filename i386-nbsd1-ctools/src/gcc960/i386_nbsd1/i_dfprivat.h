
/* Private macros for dataflow.  These should be referenced
   from dataflow.h or dataflow.c, and nowhere else. */

/* #define REGNO_VID(R)     (R)  */
/* #define REGNO_VOFFSET(R) (0)  */

#define REGNO_VID(R)     (( (R)<32 ? ((R) & ~3) : (R) ))
#define REGNO_VOFFSET(R) (( ((R)-REGNO_VID(R)) * SI_BYTES ))

#ifndef MIN
#define MIN(x,y) ((x < y) ? x : y)
#endif

#ifndef MAX
#define MAX(x,y) ((x > y) ? x : y)
#endif

#define XEXP_OFFSET(X,I) (( ((char*)&XEXP((X),(I))) - ((char*)(X)) ))
#define PAT_OFFSET(X) (( ((char*)&PATTERN((X))) - ((char*)(X)) ))
#ifdef CALL_INSN_FUNCTION_USAGE
#define CUSE_OFFSET(X) (( ((char*)&CALL_INSN_FUNCTION_USAGE((X))) - ((char*)(X)) ))
#endif
#define XVECEXP_OFFSET(X,I,J) (( ((char*)&XVECEXP((X),(I),(J))) - ((char*)(X))))
#define NDX_RTX(R,I) (( *((rtx *) (((char *)(R)) + (I))) ))


#define DF_KIND(C) \
(( (C)==REG ?DF_REG :((C)==SYMBOL_REF ?DF_SYM :((C)==MEM ?DF_MEM : NUM_DFK)) ))

#define RTX_DF_KIND(X) \
(( RTX_IS_LVAL_PARENT(X) ?DF_KIND(GET_CODE(XEXP((X),0))):DF_KIND(GET_CODE(X)) ))

static long xx_fc_xx;
#define FITS_IRANGE(I,L,H) (( (xx_fc_xx=(long)((I))) >= ((long)(L)) && xx_fc_xx <= ((long)(H)) ))

/* Given lt #, df_kind, produce ptr to row of LT's. */
#define LT_SIZE(T)       (( (T).n_lt[(int)NUM_DFK] ))
#define LT_VECT(T,N,K)   (( (T).linear_terms + ((N) * (T).n_lt[(int)NUM_DFK] + (T).n_lt[K]) ))

#define LAST_RTX_ID(I) (( last_global_rtx_id [(I) - &df_info[0]] ))

#define SPARSE_REACH_ELTS 6
#define FOR_ALL_DF(X) for ((X)=df_info; (X)!=(df_info+(int)NUM_DFK); (X)++)

/*  A map is a variable length array which knows it's own size
    and is extendable.  It's space is always allocated on the heap.
*/

#ifdef SELFHOST
typedef signed char signed_byte;
#else
typedef char signed_byte;
#endif

#if 0
#define FITS_CHAR(I) (( FITS_IRANGE((I),-128,127) ))
#else
#define FITS_CHAR(I) (( (I) == (signed_byte)(I) ))
#endif


typedef unsigned short uid;
typedef unsigned short ud;
typedef unsigned short id;
typedef unsigned short vid;
typedef unsigned short tinfo;
typedef char   lt_count;

/* Sets always have fixed external size, but they grow as needed. */

typedef flex_set ud_set;
typedef flex_set id_set;
typedef flex_set vid_set;
typedef flex_set lt_set;
typedef flex_set bb_set;
typedef flex_set uid_set;

typedef struct
{
  int  uds_used_by;         /* uds using the variable. */
  tree vtype;
} id_rec;

typedef struct
{
  rtx            user;      /* address of rtx containing the ud  rtx_def.*/
  flex_set       pts_at;    /* The set of syms this guy could point at   */
  unsigned short ud_bytes;  /* mask of bytes referred to by this ud.     */
  unsigned short varno;     /* this guy's variable #.                    */
  unsigned short insn;      /* insn containing this ud.                  */
  unsigned short ud_forw_align; /* this ud's forward flowing alignment   */
  unsigned short ud_align;  /* this ud's actual alignment                */
  unsigned char  attrs;     /* attributes of this ud.                    */
  unsigned char  off;       /* Offset in bytes from containing rtx       */
  unsigned char  ud_var_offset;  /* Offset of this ud from base of its var */
} ud_rec;

typedef struct
{
  int uds_in_insn;
  int defs_in_insn;
} uid_rec;

typedef struct
{
  rtx      rtx_ptr;
  int      iid;
  int      retval_uid;
  char     insn_has_uds;
  char     insn_volatile;
} uid_info_rec;

typedef enum { R_NC, R_UNKNOWN, R_UNSIGNED , R_SIGNED } rng_kind;

typedef unsigned short req;

typedef struct {
  unsigned tag:2;
  unsigned bits:6;
  unsigned zeroes:6;
} rng;

typedef struct {int size; ud_rec   text[1]; } ud_rec_map;
typedef struct {int size; id_rec   text[1]; } id_rec_map;
typedef struct {int size; uid_rec  text[1]; } uid_rec_map;
typedef struct {int size; uid_info_rec  text[1]; } uid_info_rec_map;
typedef struct {int size; flex_set   text[1]; } flex_set_map;
typedef struct {int size; sized_flex_set   text[1]; } sized_flex_set_map;
typedef struct {int size; rtx text[1]; } rtx_map;
typedef struct {int size; ud text[1]; } ud_map;
typedef struct {int size; char text[1]; } char_map;
typedef struct {int size; unsigned char text[1]; } uchar_map;
typedef struct {int size; short text[1]; } short_map;
typedef struct {int size; unsigned short text[1]; } ushort_map;
typedef struct {int size; unsigned long text[1]; } ulong_map;
typedef struct {int size; rng text[1]; } rng_map;
typedef struct {int size; req text[1]; } req_map;

typedef struct
{
  int         df_state;
  int         first_flow_ud;                /* where the flow ud's start.   */
  int         attr_sets_cnt[NUM_UD_SETS];
  int         attr_sets_first[NUM_UD_SETS];
  int         attr_sets_last[NUM_UD_SETS];
  int         num_ud;
  int         num_id;

  struct
  { ud_rec*     ud_map;                       /* info about each ud#          */
    uid_rec*    uid_map;                      /* info about each uid#         */
    id_rec*     id_map;                       /* info about each variable     */
    ud_set*     defs_reaching;                /* defs which reach each ud.    */
    ud_set*     uses_reached;                 /* uses which each def reaches  */
    ud*         equiv;
    req*	ud_req_rng;
    rng*	ud_full_rng;
#ifdef TMC_FLOD
    unsigned char* nkills;
    unsigned char* nuses;
#endif
  } maps;
  
  struct
  { ud_set     attr_sets[NUM_UD_SETS];       /* sets which characterize uds  */
    ud_set     dense_reach;
    ud_set     sparse_reach;
  } sets;


  struct
  { flex_pool *attr_sets_pool;
    flex_pool *ud_pool;
    flex_pool *mondo_ud_pool;
  } pools;

  int * uds_used_by_pool;
  int * uds_in_insn_pool;
  int * defs_in_insn_pool;

  int (*ud_range) ();
  char* (*ud_name) ();
  int (*setup_dataflow) ();
  int (*alloc_df) ();
  int (*update_insn_in) ();
  int (*note_insn_gen_kill) ();
  int (*get_insn_reaches) ();
  int (*next_df) ();
  int (*is_flod) ();

  int bb_bytes;
  int vars_per_def;
  unsigned short* gen;
  unsigned short* kill;
  unsigned short* in;
  unsigned short* out;
} ud_info;

typedef struct
{
  int vsize;                    /* size of this variable in bytes.  */
  int terms;                    /* index of additive terms entry */
  int lt_offset;                /* offset from terms */
  id_set slot_pt;           /* sym id's this slot could refer to. */
  int unum;
} mem_slot_info;

typedef struct {int size; mem_slot_info   text[1]; } mem_slot_info_map;

typedef struct
{
  /* These data are a sparse-to-dense mapping for the uds in the linear terms
     vectors.  Instead of recording ud numbers, we record the next available
     map index.  We still allocate the full amount (sum of all uds) per
     linear terms row;  however, only the first lt_len entries will be
     non-zero.  This means that when adding, comparing, etc, linear terms,
     we can skip the unused parts (which are most of the space). */

  unsigned short *map_lt[NUM_DFK];
  unsigned short *unmap_lt[NUM_DFK];
  int   lt_len [NUM_DFK];

  int   n_lt [(int)NUM_DFK+1];
  int   next_lt;
  int   alloc_lt;
  signed_byte* linear_terms;
} lt_rec;


#ifdef TSTUFF
typedef struct
{
  flex_set overlap;		/* Known to overlap */
  flex_set no_overlap;		/* Known not to overlap */
  flex_set points_to;		/* Known to point to */
  flex_set no_points_to;	/* Known not to point to */
  tree tree_ptr;		/* This types tree ptr */
} type_alias_info;

typedef struct {int size; type_alias_info   text[1]; } type_alias_info_map;
#endif

typedef struct
{
  unsigned short insn;
  rtx user;
  int  off;
  unsigned short attrs;
  flex_set pts_to;
  rtx new_expr;
} df_subst_rec;

typedef struct {int size; df_subst_rec   text[1]; } df_subst_rec_map;

typedef struct
{
  int num_subst;
  df_subst_rec* subst;
} df_subst_queue;

typedef struct
{
  short inner_loop;	/* The most deeply nested natural loop of which this
                           block is a member. */
  
  short parent;		/* The closest natural loop which contains inner_loop */
  
  flex_set members;	/* All blocks in this natural loop */
  flex_set member_of;	/* All natural loops this block belongs to */
} loop_rec;

typedef struct {int size; loop_rec   text[1]; } loop_rec_map;

extern ud_info df_info[(int)NUM_DFK];

extern mem_slot_info empty_slot;
extern mem_slot_info* mem_slots;

typedef struct
{
  int have_line_info;
  int iid_invalid;
  int code_at_exit;
  int slot_base;
#ifdef TSTUFF
  int computing_types;
#endif
  int exact_path_check;
  int function_never_exits;
  int frame_is_call_safe;
  int shared_syms;
  int pcache_entries;
  
  lt_rec lt_info;
  
  rtx universal_sym;
  rtx slot_base_reg;
  rtx fp_sym;
  rtx ap_sym;
  tree last_mem_df;
  tree last_function_decl;
  tree last_restrict_pts_to;
  int mem_df_level;
  int num_refine;
  FILE* dump_stream;

  flex_set* dom_relation[2];
  
  struct
  {
#ifdef TSTUFF
    flex_set new_types;
    flex_set known_types;
#endif
    flex_set natural_loops;
    flex_set new_loop;
    flex_set printing_mem_id;
    flex_set addr_syms;
    flex_set all_symbols;
    flex_set frame_symbols;
    flex_set arg_symbols;
    flex_set local_symbols;
    flex_set no_symbols;
    flex_set all_addr_syms_except_frame;
    flex_set all_addr_syms_except_arg;
    flex_set all_addr_syms_except_local;
    flex_set all_syms_except_frame;
    flex_set all_syms_except_arg;
    flex_set all_syms_except_local;
    flex_set insn_rng_change;
    flex_set dom_tmp;
    flex_set dom_ones;
  } sets;

  struct
  { loop_rec* loop_info;
    flex_set* succs;
    flex_set* dominators;
    flex_set* out_dominators;
    flex_set* forw_succs;
#if 0
    flex_set* mem_ud_dom[2];
#endif
    uid_info_rec* insn_info;
    sized_flex_set* preds;
    int *preds_alt;
    id_set* symbol_set;
    rtx* symbol_rtx;
#ifdef TSTUFF
    type_alias_info* type_info;
#endif
    flex_set* old_pt[(int)NUM_DFK];
    char* value_stack;
    short* lstk;
    short* ud_equiv;
    flex_set* ctrl_dep;
    flex_set* insn_reach;
    flex_set* insn_rng_dep;
    unsigned long *path_cache;
#if 0
    unsigned short* region;
#endif
  } maps;
  
#if 0
#define REGION_MAP(h,t) df_data.maps.region[(h) * N_BLOCKS + (t)]
#endif

  struct
  {
#ifdef TSTUFF
    flex_pool* type_info_pool;
#endif
    flex_pool* pred_pool;
    flex_pool* loop_pool;
    flex_pool* dom_pool;
    flex_pool* out_dom_pool;
    flex_pool* symbol_pool;
    flex_pool* shared_sym_pool;
    flex_pool* ud_pt_pool;
    flex_pool* ud_pt_tmp_pool;
    flex_pool* pm_pool;
    flex_pool* rd_pool;
    flex_pool* old_pool;
    flex_pool* slot_pt_pool;
    flex_pool* value_pool;
#ifdef TSTUFF
    flex_pool* overlap_pool;
#endif
    flex_pool* mrd_set_pool;
    flex_pool* var_set_pool;
#if 0
    flex_pool* mem_ud_dom_pool;
#endif
    flex_pool* insn_reach_pool;
    flex_pool* ctrl_dep_pool;
    flex_pool* insn_rng_dep_pool;
  } pools;

} df_global_info;

extern df_global_info df_data;

