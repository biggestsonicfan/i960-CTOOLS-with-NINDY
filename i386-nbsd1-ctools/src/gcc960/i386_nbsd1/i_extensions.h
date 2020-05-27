/*
 * This file should make defs that are applicable to compilers
 * for all machines of interest to imstg.
 */
    
#ifndef ROUND
#define ROUND(X,MULTIPLE_OF) (( (((X) + (MULTIPLE_OF)-1) / (MULTIPLE_OF)) * MULTIPLE_OF ))
#endif

#ifndef min
static int min (i, j)
int i;
int j;
{ return ((i <= j) ? i : j);
}
#endif

#ifndef max
static int max (i, j)
int i;
int j;
{ return ((i >= j) ? i : j);
}
#endif

#define MACH_CHEAP_MULTIPLY 1

#define LOOP_REG_THRESHOLD 36
#define HAVE_PID 1

extern int pid_flag;
#define PROB_BASE 100

/* 1 if insn was generated as part of profiling instrumentation. */
#define INSN_PROFILE_INSTR_P(INSN)   ((INSN)->bit8_flag)

extern int flag_bbr;
extern int flag_build_db;
extern int flag_build_src_db;
extern int flag_coalesce;
extern int flag_coff;
extern int flag_elf;
/* flag_bout is not a real flag */
#define flag_bout (!(flag_coff || flag_elf))
extern int flag_constcomp;
extern int flag_constprop;
extern int flag_constreg;
extern int flag_copyprop;
extern int flag_cond_xform;
extern int flag_default_trip_count;
extern int flag_default_call_count;
extern int flag_df_order_by_var;
extern int flag_glob_inline;
extern int flag_glob_alias;
extern int flag_glob_sram;
extern int flag_linenum_mods;	/* Perform fine-grained line number note
				   tracking for DWARF.  Always disabled
				   if flag_elf is off.
				 */
extern int flag_prof_use;
extern int flag_prof_db;
extern int flag_prof_instrument;
extern int flag_prof_instrument_only;
extern int flag_prof_use;
extern int flag_sblock;
extern int flag_sched_sblock;
extern int flag_shadow_globals;
extern int flag_dead_elim;
extern int flag_shadow_mem;
extern int flag_split_mem;
extern int flag_marry_mem;
extern int flag_volatile_global;
extern int flag_coerce, flag_coerce2, flag_coerce3, flag_coerce4, flag_coerce5;
extern int flag_find_bname;
extern int flag_ic960;
extern int flag_space_opt;
extern int flag_mix_asm;
extern int flag_fancy_errors;
extern int flag_place_functions;
extern int flag_unroll_small_loops;
extern int flag_use_lomem;
extern int flag_int_alias_ptr, flag_int_alias_real, flag_int_alias_short;

extern double flag_default_if_prob;
extern int  in_prof_default;
extern char *in_prof_name;
extern char  base_file_name[];
extern int   have_base_file_name;

extern int bbr_time;
extern int constprop_time;
extern int copyprop_time;
extern int dataflow_time;
extern int dataflow_time_vect[];
extern int glob_inline_time;
extern int sblock_time;
extern int shadow_time;
extern int shadow_mem_time;
extern int coerce_time;

extern int cur_insn_uid;

#define PUT_NEW_UID(I) \
do { \
  INSN_UID(I) = cur_insn_uid++; \
} while (0)

#define ASM_FILE_END(F) comp_unit_end(F)

#define ASM_FUNCTION_LOCAL_LABEL(L,G) asm_function_local_label(L,G)

#define OUTPUT_LABEL 1
#define GENERATE_LABEL 2
#define GLOBALIZE_LABEL 4

#define ROUND_FIELD_ALIGN(F,CONST_SIZE,DESIRED_ALIGN,RECORD_ALIGN) \
  imstg_round_field_align ((F),(CONST_SIZE),(DESIRED_ALIGN),(RECORD_ALIGN))

typedef
struct { char *string; int *variable; int on_value;} f_options_rec;
extern f_options_rec f_options[];

extern int sizeof_f_options;
extern int scan_opt_pass;

/* The index of the first option which should NOT be scanned in the
   initial scan_options pass.  For now, it is 9, which is "bbr".  Don't
   forget to update this if options are added in front of bbr. */

#define FIRST_REGULAR_FOPTION 9

/* The options which are to be scanned in the initial scan_options
   pass must come first. */
#define IMSTG_FOPTIONS \
  {"db", &flag_build_src_db, 1}, \
  {"prof-db", &flag_prof_db, 1}, \
  {"prof", &flag_prof_instrument_only, 1}, \
  {"prof-instrument", &flag_prof_instrument, 1}, \
  {"prof-use", &flag_prof_use, 1}, \
  {"build-db", &flag_build_db, 1}, \
  {"glob-inline", &flag_glob_inline, 1}, \
  {"glob-alias", &flag_glob_alias, 1}, \
  {"glob-sram", &flag_glob_sram, 1}, \
  {"bbr", &flag_bbr, 1}, \
  {"linenum-mods", &flag_linenum_mods, 0}, \
  {"shadow-globals", &flag_shadow_globals, 1}, \
  {"coalesce", &flag_coalesce, 1}, \
  {"shadow-mem", &flag_shadow_mem, 1}, \
  {"constcomp", &flag_constcomp, 1}, \
  {"constprop", &flag_constprop, 1}, \
  {"constreg", &flag_constreg, 1}, \
  {"copyprop", &flag_copyprop, 1}, \
  {"condxform", &flag_cond_xform, 1}, \
  {"sblock", &flag_sblock, 1}, \
  {"split_mem", &flag_split_mem, 1}, \
  {"marry_mem", &flag_marry_mem, 1}, \
  {"dead_elim", &flag_dead_elim, 1}, \
  {"coerce", &flag_coerce, 1}, \
  {"sched_sblock", &flag_sched_sblock, 1}, \
  {"space-opt", &flag_space_opt, 1}, \
  {"fancy-errors", &flag_fancy_errors, 1}, \
  {"mix-asm", &flag_mix_asm, 1}, \
  {"place-functions", &flag_place_functions, 1}, \
  {"use-lomem", &flag_use_lomem, 1}, \
  {"int-alias-ptr", &flag_int_alias_ptr, 1}, \
  {"int-alias-real", &flag_int_alias_real, 1}, \
  {"int-alias-short", &flag_int_alias_short, 1}, \

/* Are we generating counting code ? */
#define PROF_CODE  (( flag_prof_instrument | flag_prof_instrument_only | flag_prof_db ))

/* Do we need to read pass2.db ? */
#define USE_DB     (( flag_prof_use |flag_glob_inline|flag_glob_sram ))

/* Do we want to output a database with cpp info in it ? */
extern int flag_is_subst_compile;
#define BUILD_SRC_DB (( flag_build_src_db | (flag_prof_instrument_only && !flag_is_subst_compile)))

/* Are we generating cc_info ? */
#define BUILD_DB   (( flag_build_db |BUILD_SRC_DB |flag_prof_instrument | flag_prof_db ))


