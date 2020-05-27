#ifndef __DATAFLOWH__
#define __DATAFLOWH__ 1

#define DF_ERROR         1
#define DF_SORT          2
#define DF_RESTART       4
#define DF_REACHING_DEFS 8
#define DF_EQUIV         16

#define GET_DF_STATE(D) (( (D)->df_state ))
#define SET_DF_STATE(D,S) (( (D)->df_state = (S) ? ((S) | (((D)->df_state) &((S)-1))) : 0 ))

#define I_KILL      0
#define I_DEF       1
#define I_USE       2
#define I_ADDR      3
#define I_CALL      4
#define I_FLOD      5
#define I_INACTIVE  6
#define NUM_UD_SETS 7

#define S_KILL     ((1 << I_KILL     ))
#define S_DEF      ((1 << I_DEF      ))
#define S_USE      ((1 << I_USE      ))
#define S_ADDR     ((1 << I_ADDR     ))
#define S_CALL     ((1 << I_CALL     ))
#define S_FLOD     ((1 << I_FLOD     ))
#define S_INACTIVE ((1 << I_INACTIVE ))

#ifdef GCC20
#include "i_df_set.h"
#include "i_dfprivat.h"
#else
#include "df_set.h"
#include "dfprivat.h"
#endif

/* The stuff in dfprivat.h is needed to make the macros and
   stuff in this file (dataflow.h) work, but you shouldn't
   use any of it. */


#define REAL_INSN(I) (GET_CODE(I)==INSN || GET_CODE(I)==JUMP_INSN \
                      || GET_CODE(I)==CALL_INSN)

#define SWAP_FLEX(A,B) \
do { flex_set __flx_t0_; __flx_t0_=(A); (A)=(B); (B)=(__flx_t0_); } while (0)

#define MK_SET(X) (( (1) << ((int)(X)) ))

#define MEM_DF df_info[(int)DF_MEM]
#define SYM_DF df_info[(int)DF_SYM]
#define REG_DF df_info[(int)DF_REG]
#define IID(INSN) (df_data.maps.insn_info[INSN_UID(INSN)].iid)
#define INSN_RETVAL_UID(INSN) (df_data.maps.insn_info[INSN_UID(INSN)].retval_uid)
#define UID_RETVAL_UID(U) (df_data.maps.insn_info[U].retval_uid)
#define UID_VOLATILE(U)   (df_data.maps.insn_info[U].insn_volatile)
#define UID_RTX(U) (df_data.maps.insn_info[U].rtx_ptr)

#define TYPE_IS_PTR_TO_FUNC(t) (( \
  (t) && TREE_CODE(t)==POINTER_TYPE && TREE_TYPE(t) && \
  TREE_CODE(TREE_TYPE(t))==FUNCTION_TYPE \
))

#define RTX_IS_UD(X) (( \
  (GET_CODE(X)!=SYMBOL_REF || (!TYPE_IS_PTR_TO_FUNC(RTX_TYPE(X)))) && \
  (RTX_IS_LVAL(X) || RTX_IS_LVAL_PARENT(X)) ))

#define DF_INFO(X)    (( RTX_IS_UD(X) ? &df_info[(int)RTX_DF_KIND(X)] : 0 ))

#define UD_RTX(UD)         ((  (NDX_RTX((((UD).user)),((UD).off))) ))
#define UD_CONTEXT(UD)     ((  (UD).off ? ((UD).user) : 0 ))
#define UD_VRTX(UD)        ((  (RTX_LVAL(UD_RTX(UD))) ))
#define UD_TYPE(UD)        ((  (RTX_TYPE(RTX_LVAL(UD_RTX(UD)))) ))
#define UD_INSN_UID(UD)    ((  (UD).insn ))
#define UD_INSN_RTX(UD)    ((  UID_RTX(UD_INSN_UID(UD)) ))
#define UD_ATTRS(UD)       ((  (UD).attrs ))
#define UD_BLOCK(UD)       ((  BLOCK_NUM(UD_INSN_RTX(UD)) ))
#define UD_VAR_OFFSET(UD) ((  (UD).ud_var_offset ))
#define UD_BYTES(UD)       (( (UD).ud_bytes ))
#define UD_ALIGN(UD)       (( (UD).ud_align ))
#define UD_VARNO(UD)       (( (UD).varno ))
#define UD_PTS_AT(UD)      (( (UD).pts_at ))

#define UDN_RTX(I,UD)      UD_RTX((I)->maps.ud_map[UD])
#define UDN_CONTEXT(I,UD)  UD_CONTEXT((I)->maps.ud_map[UD])
#define UDN_VRTX(I,UD)     UD_VRTX((I)->maps.ud_map[UD])
#define UDN_TYPE(I,UD)     UD_TYPE((I)->maps.ud_map[UD])
#define UDN_INSN_UID(I,UD) UD_INSN_UID((I)->maps.ud_map[UD])
#define UDN_INSN_RTX(I,UD) UD_INSN_RTX((I)->maps.ud_map[UD])
#define UDN_ATTRS(I,UD)    UD_ATTRS((I)->maps.ud_map[UD])
#define UDN_DEFS(I,UD)     (( ((I)->maps.defs_reaching[UD]) ))
#define UDN_USES(I,UD)     (( ((I)->maps.uses_reached[UD])  ))
#define UDN_BLOCK(I,UD)    UD_BLOCK((I)->maps.ud_map[UD])
#define UDN_VAR_OFFSET(I,UD)   UD_VAR_OFFSET((I)->maps.ud_map[UD])
#define UDN_BYTES(I,UD) UD_BYTES((I)->maps.ud_map[UD])
#define UDN_ALIGN(I,UD) UD_ALIGN((I)->maps.ud_map[UD])
#define UDN_VARNO(I,UD) UD_VARNO((I)->maps.ud_map[UD])
#define UDN_PTS_AT(I,UD)UD_PTS_AT((I)->maps.ud_map[UD])

#define UDN_REQ_RNG(df,ud) ((df)->maps.ud_req_rng[ud])
#define UDN_FULL_RNG(df,ud) ((df)->maps.ud_full_rng[ud])

#define UDN_IRTX(I,U) UDN_INSN_RTX(I,U)

extern char* df_names[];
extern char* attr_names[];

#define NUM_UD(X)     (( (X)->num_ud ))
#define NUM_ID(X)     (( (X)->num_id ))

#define NUM_UID       (( (get_max_uid()+1) ))

#define UDS_IN_UID(DF,I) (( (DF)->maps.uid_map[I].uds_in_insn ))

#ifndef offsetof
#define offsetof(T,M) (( ((char*)&(((T *)1024)->M))-((char*)((T *) 1024)) ))
#endif

#define ALLOC_MAP(TEXT,N,TYPE) \
(( alloc_map((TEXT), ((N)*sizeof((*(TEXT))[0])), (offsetof(TYPE,text[0]))) ))

#define REALLOC_MAP(TEXT,N,TYPE) \
(( realloc_map((TEXT), ((N)*sizeof((*(TEXT))[0])), (offsetof(TYPE,text[0]))) ))

#define MAP_SIZE(TEXT,TYPE) \
(( (TEXT) ? (*((int*)(((char*)(TEXT))-offsetof(TYPE,text[0]))) / sizeof((TEXT)[0])) : 0 ))

typedef enum { df_overlap, df_contains, df_equivalent } df_range_kind;
typedef enum
{
  df_assertion,
  df_capacity,
  df_too_much_memory,
  df_too_many_insns,
  df_too_many_blocks
}
df_error_kind;

#define DF_MAX_NBLOCKS 1024

#define EQUAL_UD_TYPE(U,W) (( TREE_UID(UD_TYPE(U))==TREE_UID(UD_TYPE(W)) ))

#define DF_DOMI(D) (( ((D)>=0) ? UDN_IRTX((df_info+(int)DF_MEM),(D)) : UID_RTX(-(D)) ))

#ifndef DF_CC0_INSN
#define DF_CC0_INSN(I) (( \
  (I!=0) && \
  (GET_CODE(I)==INSN||GET_CODE(I)==JUMP_INSN|| \
  GET_CODE(I)==CALL_INSN) && \
  (reg_mentioned_p (cc0_rtx, PATTERN(I))) ))
#endif

#define LIBCALL_MRD_SET 1
#define LAST_SPECIAL_MRD_SET 1
extern int num_mrd_vars, mrd_nsets, num_mrd_spills;
extern df_global_info df_data;
extern char* attr_names[];
extern char* df_names[];
extern int max_df_insn, iid_gap;
extern tree last_mrd_function;

typedef enum {
  illegal_overlap,
  illegal_address,
  refine_info_entry,
  refine_info_slot_change,
  refine_info_pt_change,
  refine_info_no_change,
  superflow_entry,
  restrict_pt_change,
  restrict_pt_no_change,
  defs_reaching_entry,
  defs_reaching_exit,
  update_sets_exit,
  get_preds_exit,
  no_reaching_def,
  no_reached_use,
  no_type_info,
  equiv_use,
  new_linear_terms,
  bad_addr_term,
  add_addr_term_entry,
  addr_flow_equiv,
  add_addr_replace,
  addr_multi_reach,
  addr_isdef,
  addr_expand_ok,
  addr_expand_failed,
  addr_unexpanable,
  addr_added,
  addr_not_flow,
  addr_exit,
  replace_ud_entry,
  replace_ud_exit,
  init_trace
} dbt_kind;

typedef struct
{ dbt_kind kind;
  int cnt;
}
db_rec;

extern db_rec db_trace[(int)(init_trace+1)];

int b_hides_a_from_c();
int path_from_a_to_c_not_b();
int path_from_a_to_c_around_b();
void scan_rtx_for_uds();

typedef struct
{ rtx reg;
  int is_parm;
} df_hard_reg_info;

extern df_hard_reg_info df_hard_reg[FIRST_PSEUDO_REGISTER];

#ifndef GCC20
#define TYPE_UID(T) TREE_UID(T)
#endif

#ifdef TMC_NOISY
#define assign_run_time(i) df_assign_run_time(i)
#else
#define assign_run_time(i) 0
#endif

#endif /* __DATAFLOWH__ */

