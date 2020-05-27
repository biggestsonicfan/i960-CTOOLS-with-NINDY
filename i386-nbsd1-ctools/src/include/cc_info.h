#ifndef CCINFOH
#define CCINFOH
#define PROTO2 1

/*(c****************************************************************************** *
 * Copyright (c) 1990, 1991, 1992, 1993 Intel Corporation
 * 
 * Intel hereby grants you permission to copy, modify, and distribute this
 * software and its documentation.  Intel grants this permission provided
 * that the above copyright notice appears in all copies and that both the
 * copyright notice and this permission notice appear in supporting
 * documentation.  In addition, Intel grants this permission provided that
 * you prominently mark as "not part of the original" any modifications
 * made to this software or documentation, and that the name of Intel
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software or the documentation without specific,
 * written prior permission.
 * 
 * Intel Corporation provides this AS IS, WITHOUT ANY WARRANTY, EXPRESS OR
 * IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY
 * OR FITNESS FOR A PARTICULAR PURPOSE.  Intel makes no guarantee or
 * representations regarding the use of, or the results of the use of,
 * the software and documentation in terms of correctness, accuracy,
 * reliability, currentness, or otherwise; and you rely on the software,
 * documentation and results solely at your own risk.
 * 
 * IN NO EVENT SHALL INTEL BE LIABLE FOR ANY LOSS OF USE, LOSS OF BUSINESS,
 * LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES
 * OF ANY KIND.  IN NO EVENT SHALL INTEL'S TOTAL LIABILITY EXCEED THE SUM
 * PAID TO INTEL FOR THE PRODUCT LICENSED HEREUNDER.
 * 
 *****************************************************************************c)*/

/*
 * Various definitions and macros for working with CC_INFO information
 * embedded in object modules.
 * Written by Kevin B. Smith, Intel Corp, 1991.
 */
 
typedef struct ci_head_rec_ {
  unsigned short major_ver;
  unsigned char minor_ver;
  unsigned char kind_ver;
  int db_size;
  int sym_size;
  int str_size;
  unsigned long time_stamp;
} ci_head_rec;

#define CI_CC1_DB     1
#define CI_PARTIAL_DB 2
#define CI_PASS1_DB   3
#define CI_PASS2_DB   4
#define CI_SPF_DB     5
#define CI_NUM_DB     6

/* Roll CI_MAJOR_VER when changes are made which are incompatible
   with the current cc_info.  The full major version number is computed as
   CI_MAJOR_VER+DEV_TREE_NUM.  The tools will issue fatal errors upon
   major version number mismatches.

   You must roll CI_MAJOR_VER by an amount sufficient to cover all
   of the versions currently represented by different development trees.

   Currently, there are 2 versions; /dev, and /dev_465.  Hence, you should
   bump CI_MAJOR_VER by 2 when there are changes sufficient to cause the
   major version number to roll. */

#define CI_MAJOR_VER  21

/* Roll this if you just want warnings when old stuff is encountered. */
#define CI_MINOR_VER  1

/* We use "DEV_TREE_NUM" to distinguish between development trees.
   DEV_TREE_NUM is be defined in dversion.h, which each
   tree has it's own copy of. */

#include <dversion.h>

#define CI_MAJOR  (( CI_MAJOR_VER+DEV_TREE_NUM ))
#define CI_MINOR  (( CI_MINOR_VER ))

#define CI_MAJOR_MASK 0x8000

#define CI_HEAD_MINOR_OFF         0
#define CI_HEAD_KIND_OFF          1
#define CI_HEAD_MAJOR_OFF         2
#define CI_HEAD_TOT_SIZE_OFF      4
#define CI_HEAD_DBSIZE_OFF        8
#define CI_HEAD_SYMSIZE_OFF      12
#define CI_HEAD_STRSIZE_OFF      16
#define CI_HEAD_STAMP_OFF        20
#define CI_HEAD_REC_SIZE         24

#define CI_STAB_STROFF_OFF        0
#define CI_STAB_DBOFF_OFF         4
#define CI_STAB_DBRSZ_OFF         8
#define CI_STAB_HASH_OFF         12
#define CI_STAB_STATIC_OFF       14
#define CI_STAB_RECTYP_OFF       15
#define CI_STAB_REC_SIZE         16

#define CI_VDEF_REC_TYP           1	/* Variable definition record */
#define CI_FDEF_REC_TYP           2	/* Function definition record */
#define CI_LIB_VDEF_REC_TYP       3     /* Library Variable definition record */
#define CI_LIB_FDEF_REC_TYP       4	/* Library Function definition record */
#define CI_VREF_REC_TYP           5	/* Variable reference record */
#define CI_FREF_REC_TYP           6	/* Function reference record */
#define CI_PROF_REC_TYP           7	/* Profile information record */
#define CI_CG_REC_TYP             8	/* Call-graph record */
#define CI_SRC_REC_TYP            9     /* Source (preprocessed) record */
#define CI_GLD_REC_TYP           10	/* info from gld */
#define CI_SUB_REC_TYP           11
#define CI_MAX_REC_TYP           11

#define CI_ADDR_TAKEN_OFF         0	/* for fref/fdef/vref/vdef records */

#define CI_VDEF_SIZE_OFF          1	/* Variable size */
#define CI_VDEF_USAGE_OFF         5     /* Variable static usage count */
#define CI_VDEF_SRAM_ADDR_OFF     9     /* Variable's fast memory address */
#define CI_VDEF_REC_FIXED_SIZE   13

#define CI_FDEF_CAN_DELETE_OFF    1	/* Function deletable flag */
#define CI_FDEF_TOT_INLN_OFF      5
#define CI_FDEF_NINSN_OFF         7	/* Number of insns in function */
#define CI_FDEF_NPARM_OFF         9	/* Number of parameters to function */
#define CI_FDEF_NCALL_OFF        11	/* Number of calls in function */
#define CI_FDEF_REG_USE_OFF      13	/* Number registers used by function */
#define CI_FDEF_VOTER_OFF        14
#define CI_FDEF_PQUAL_OFF        16
#define CI_FDEF_REC_FIXED_SIZE   18

#define CI_VREF_USAGE_OFF         1	/* Variable static usage count */
#define CI_VREF_REC_FIXED_SIZE    5

#define CI_FREF_REC_FIXED_SIZE    1

#define CI_PROF_NBLK_OFF          0
#define CI_PROF_NCNT_OFF          4
#define CI_PROF_NLINES_OFF        8
#define CI_PROF_SIZE_OFF         12
#define CI_PROF_REC_FIXED_SIZE   16

#define CI_CG_REC_FIXED_SIZE      0

#define CI_SRC_REC_FSTAT_OFF      0
#define CI_SRC_REC_DINDEX_OFF     4
#define CI_SRC_REC_SINDEX_OFF     6
#define CI_SRC_REC_LANG_OFF       8
#define CI_SRC_REC_FIXED_SIZE     9

#define CI_GLD_VALUE_OFF          0
#define CI_GLD_REC_FIXED_SIZE     4

#define CI_SUB_REC_FIXED_SIZE     4

#define CI_MAX_REC_FIXED_SIZE    18

#define CI_SRC_LIST_LO            0
#define CI_SRC_LIB_LIST           0
#define CI_SRC_MEM_LIST           1
#define CI_SRC_CC1_LIST           2
#define CI_SRC_SRC_LIST           3
#define CI_SRC_ASM_LIST           4
#define CI_SRC_SUBST_LIST         5
#define CI_SRC_LIST_HI            6

#define CI_FDEF_LIST_LO           0
#define CI_FDEF_IVEC_LIST         0
#define CI_FDEF_REGP_LIST         1
#define CI_FDEF_RTL_LIST          2
#define CI_FDEF_FILE_LIST         3
#define CI_FDEF_FUNC_LIST         4
#define CI_FDEF_PROF_LIST         5
#define CI_FDEF_HIST_LIST         6
#define CI_FDEF_TSINFO_LIST       7
#define CI_FDEF_CFG_LIST          8
#define CI_FDEF_FTYPE_LIST        9	/* encoded function and formals type */
#define CI_FDEF_CALL_TYPE_LIST   10     /* encoded actual parms and ret type */
#define CI_FDEF_LIST_HI          11

#define CI_VDEF_LIST_LO           0
#define CI_VDEF_FILE_LIST         0
#define CI_VDEF_FUNC_LIST         1
#define CI_VDEF_LIST_HI           2

#define CI_FREF_FUNC_LIST         CI_FDEF_FUNC_LIST
#define CI_FREF_LIST_LO           CI_FREF_FUNC_LIST
#define CI_FREF_LIST_HI           CI_FREF_FUNC_LIST+1

#define CI_VREF_FUNC_LIST         CI_VDEF_FUNC_LIST
#define CI_VREF_LIST_LO           CI_VREF_FUNC_LIST
#define CI_VREF_LIST_HI           CI_VREF_FUNC_LIST+1

#define CI_PROF_LIST_LO            0
#define CI_PROF_FNAME_LIST         0
#define CI_PROF_LIST_HI            1
#define CI_NUM_LISTS              12	/* Max number of lists in any record */

#define CI_LIST_FMT(LO,HI) (( ((1 << (HI)) - 1) & ((-1) << (LO)) ))
#define CI_REC_LIST_FMT(T) (( CI_LIST_FMT(CI_REC_LIST_LO(T),CI_REC_LIST_HI(T))))

#define CI_LIST_TEXT(p,l) (db_get_list(p,l)+4)
#define CI_LIST_TEXT_LEN(p,l) db_get_list_text_len(p,l)

/*
 * MACROS used for packing and unpacking information in a machine independent
 * order.
 */
#define CI_U32_TO_BUF(buf,u32) \
( (buf)[0] = (u32), (buf)[1] = (u32) >> 8, \
  (buf)[2] = (u32) >> 16, (buf)[3] = (u32) >> 24 )

#define CI_U16_TO_BUF(buf,u16) \
( (buf)[0] = (u16), (buf)[1] = (u16) >> 8 )

#define CI_U8_TO_BUF(buf,u8) \
( (buf)[0] = (u8) )

#define CI_U32_FM_BUF(buf,u32) \
( (u32) = ((buf)[0] & 0xFF) | (((buf)[1] & 0xFF) << 8) | \
          (((buf)[2] & 0xFF) << 16) | (((buf)[3] & 0xFF) << 24) )

#define CI_U16_FM_BUF(buf,u16) \
( (u16) = ((buf)[0] & 0xFF) | (((buf)[1] & 0xFF) << 8) )

#define CI_U8_FM_BUF(buf,u8) \
( (u8) = ((buf)[0] & 0xFF) )

/*
 * Macros for computing hash for CC_INFO symbol table.  Hash values are
 * stored in the file to make it easier to read in the symbol table and
 * use it without having to rehash everything.  Everyone must use the
 * same hash algorithm, or things will screw up.
 */

#define CI_HASH_SZ 512

#define CI_SYM_HASH(ret_hash, sym) \
{ \
  char *_cp = (sym); \
  int _hash = 0; \
  while (*_cp != 0) \
    _hash += *_cp++; \
  (ret_hash) = _hash & (CI_HASH_SZ - 1); \
}

static char CI_PROF_CTR_PREFIX[] = "___profile_data_start.";

#define CI_X960_NARG_PTR 4096
#define CI_X960_NBUF_INT 4096

#define CI_MAX_PROF_QUALITY   0xffff

#define CI_FDEF_IVECT_SIZE(n_bits) ((n_bits))
#define CI_FDEF_IVECT_SET(bp,bit,val) ((bp)[(bit)] = val)
#define CI_FDEF_IVECT_VAL(bp,bit)  ((bp)[(bit)])

#define CI_FDEF_IVECT_NOTHING 0
#define CI_FDEF_IVECT_INLINE  1
#define CI_FDEF_IVECT_RECURSIVE 2

#define CI_MAX_RM   20
#define CI_MAX_SUBST 2
#define CI_OBJ_BASE  8
#define CI_OBJ_EXT   3
#define CI_OBJ_LEN  12
#define CI_MAX_OBJ_CONFLICT 36	/* Max number of conflicts per 8.3 name */

#define CI_LANG_C    1
#define CI_LANG_CPLUS 2

#define CI_ISFDEF(t)  (( (t)==CI_FDEF_REC_TYP||(t)==CI_LIB_FDEF_REC_TYP ))
#define CI_ISVDEF(t)  (( (t)==CI_VDEF_REC_TYP||(t)==CI_LIB_VDEF_REC_TYP ))
#define CI_ISFUNC(t)  (( (t)==CI_FREF_REC_TYP || CI_ISFDEF(t) ))
#define CI_ISVAR(t)   (( (t)==CI_VREF_REC_TYP || CI_ISFDEF(t) ))
#define CI_ISDEF(t)   (( CI_ISFDEF(t) || CI_ISVDEF(t) ))

static char ci_lead_char[CI_MAX_REC_TYP+1] =
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, '1', '2' };

typedef char DB_FILE;

#include <stdio.h>
#include <time.h>

typedef struct named_fd_ {
  FILE* fd;
  char* name;
  int is_ascii;
} named_fd;

#if defined(__STDC__)
#define DB_PROTO(ARGS)	()
#else
#define DB_PROTO(ARGS)	()
#endif

/* We use db_list_node for weaving together lists at the ends of records.
   As the lists are written when we put the database out, we jam them back
   together.  */

typedef struct db_list_node_ {
  int size;
  int tell;		/* Seek offset in input file of this list node */
  int out_tell;		/* Seek offset in output file of this list node */
  unsigned char *text;
  struct db_list_node_ *next;
  struct db_list_node_ *last;
} db_list_node;

typedef struct db_stab_struct {
  struct db_stab_struct *next;  /* used for linking them together */
  char *name;                /* name of entry */
  int  name_length;          /* number of chars in name */
  long db_rec_offset;        /* location of db record */
  long db_rec_size;          /* size of the data base record */
  long db_list_size;         /* size of the list part at the end */
  unsigned char *db_rec;     /* actual data associated with this thing */
  db_list_node *db_list[CI_NUM_LISTS];
  unsigned char is_static;   /* is the name private */
  unsigned char rec_typ;     /* the record type */
  unsigned short sxtra;      /* application specific extra info */
  char* file_name;
  char* extra;               /* application specific extra info */
} st_node;

typedef struct dbase_ {

  void  (*post_read)();
  int   (*rec_skip_fmt)();
  void  (*pre_write)();
  int   (*pre_write_del)();

  st_node *db_stab[CI_HASH_SZ];
  struct dbase_extra_ *extra;
  named_fd* in;
  unsigned char kind;
  unsigned long time_stamp;

  long sym_sz;
  long db_sz;
  long str_sz;
  long tot_sz;

} dbase;

typedef enum {
  TS_RTL,
  TS_LOOP,
  TS_SBLK,
  TS_END,
  TS_PROF,
} ts_info_point;

#define TS_INL TS_END
#define NUM_TS (( 10 ))
#define TS_BUFSIZ (( NUM_TS * 2 ))	/* Space in db for ts info */

/* Bound in cc_finfo.c */
extern time_t dbf_get_mtime ();
extern time_t db_get_mtime ();
extern void  dbf_close ();
extern char* dbf_name ();
extern void  dbf_open_read ();
extern void  dbf_open_write ();
extern void  dbf_read ();
extern void  dbf_seek_set ();
extern void  dbf_seek_cur ();
extern void  dbf_seek_end ();
extern int   dbf_tell ();
extern void  dbf_write ();

/* Bound in cc_dinfo.c */
extern void  db_fatal (char *fmt, ...);
extern void  db_error (char *fmt, ...);
extern void  db_warning (char *fmt, ...);
extern void  db_sig_handler ();
extern void  db_set_signal_handlers ();
extern char* db_malloc  ();
extern char* db_realloc  ();
extern unsigned char* db_get_list ();

/* Bound in cc_info.c */
extern void db_func_block_range ();
extern st_node* dbp_prof_block_fdef ();
extern void db_note_pdb ();
extern char* db_get_pdb ();
extern char* db_pdb_file ();
extern void  db_set_prog  ();
extern char* db_prog_name  ();
extern char* db_prog_base_name  ();
extern char* db_genv2 ();
extern char* db_asm_name ();
extern char* db_cc1_name ();
extern char* db_x_name ();
extern int db_access_rok ();
extern void db_unlink  ();
extern void dbp_for_all_sym ();
extern st_node * dbp_lookup_sym ();
extern st_node * dbp_add_sym ();
extern char* db_member ();
extern st_node* dbp_bname_sym ();
extern int db_fstat ();
extern int db_set_fstat ();
extern int db_dindex ();
extern int db_set_dindex ();
extern int db_sindex ();
extern int db_set_sindex ();
extern int db_rec_addr_taken ();
extern void db_force_addr_taken ();
extern int db_rec_n_insns ();
extern int db_rec_n_parms ();
extern int db_rec_n_calls ();
extern int db_rec_n_regs_used ();
extern unsigned char * db_inln_dec_vec ();
extern unsigned char * db_rec_reg_pressure_info ();
extern double db_fdef_prof_counter ();
extern int db_fdef_has_prof ();
extern int db_fdef_prof_change ();
extern unsigned char * db_rec_rtl ();
extern unsigned char * db_fdef_cfg ();
extern int db_parm_len ();
extern int db_parm_is_varargs ();
extern unsigned char * db_fdef_call_ptype ();
extern unsigned char * db_fdef_ftype ();
extern unsigned char * db_fdef_next_ptype_info ();
extern int db_rec_rtl_size ();
extern int db_rec_inlinable ();
extern char * db_rec_prof_info ();
extern char * db_rec_call_vec ();
extern void db_rec_make_deletable  ();
extern void db_rec_make_tot_inline_calls  ();
extern void db_rec_make_arc_inlinable ();
extern int db_rec_var_size ();
extern unsigned db_rec_var_sram ();
extern int db_rec_var_usage ();
extern void db_rec_var_make_fmem ();
extern db_list_node* db_new_list  ();
extern void db_replace_list  ();
extern void db_init_lists  ();
extern void db_weave_lists  ();
extern void db_swap_recs  ();
extern int db_qsort_st_node  ();
extern void dbp_read_ccinfo ();
extern void db_merge_recs  ();
extern int db_check_static  ();
extern void dbp_merge_syms ();
extern void dbp_write_ccinfo ();
extern char* db_oname ();
extern int db_is_subst  ();
extern char* db_buf_at_least  ();
extern char* db_subst_name  ();
extern void db_comment(char** buf, char* fmt, ...);
extern char** db_new_removal  ();
extern void db_remove_files  ();
extern void db_dump_symbol ();
extern int db_get_list_text_len ();
extern char* db_lock ();

#endif
