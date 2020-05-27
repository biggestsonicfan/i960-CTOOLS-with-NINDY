/*
    Link-time substitution and object module dependency analysis data
    structures.

    Copyright (C) 1995 Intel Corporation.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 2.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef SUBSTH
#define SUBSTH

/* Data structure for representing subst and ref commands */

typedef struct module_name_ {
  struct module_name_ *next;
  char* text;		/* Raw text of switch */
  char* modifier;	/* Modifer that follows name, or "no" */
  char* archive;
  char* member;
  unsigned long grp_ts;
  unsigned long grp_sz;
  int id;
} module_name;

extern dbase prog_db;
extern dbase prev_db;

typedef struct dbase_extra_ {
  int count[CI_MAX_REC_TYP+1];
  int first[CI_MAX_REC_TYP+1];
  int lim[CI_MAX_REC_TYP+1];
  st_node *st_nodes[1];
} dbase_extra;

typedef enum { mn_ref, mn_subst, mn_size, mn_nokind } mname_kind;
#define NUM_MKIND (( (int) mn_nokind ))
extern int num_mname[NUM_MKIND];
extern module_name* mname_head[NUM_MKIND];
extern module_name* ts_grp ();

typedef struct subst_info_ {
  char* key;
  int kind;
  int is_sinfo;
  int is_clist;
  char* asm_pp;
  char* original_modifier;
} subst_info;

typedef struct src_extra_ {
  int dope_index;
  module_name* raw_info[NUM_MKIND];	/* Pointers to raw subst info */
  subst_info parsed_info[NUM_MKIND];
  unsigned long mod_ts;
  unsigned long mod_sz;
} src_extra;

typedef struct fdef_extra_ {
  int dope_index;
  char* cgn;		/* Call graph node */
  int bname_index;	/* index in dope vector of bname for this fdef */
  int num_inlv;
  st_node** inlinev;
  unsigned* tclose;
  unsigned long fdef_size;
} fdef_extra;

typedef struct fref_extra_ {
  int dope_index;
  char* cgn;		/* Call graph node */
} fref_extra;

#define SRCX(p) ((*((src_extra *)  ((p)->extra))))
#define FDX(p)  ((*((fdef_extra *) ((p)->extra))))

#define IVK_OFF 1
#define IVK_LEN 2

extern char* find_cc1_opt();

#define GET_CHANGE(F)   (( (F)->sxtra+0 ))

#define SET_IS_COMPILE(S)   (( (S)->sxtra = 1 ))
#define CLR_IS_COMPILE(S)   (( (S)->sxtra = 0 ))
#define IS_COMPILE(S)       (( (S)->sxtra ))

#define FSET_INTS  (( (prog_db.extra->count[CI_SRC_REC_TYP]+31) / 32 ))

#define SET_FSET(SET,BIT)  (( (SET)[(BIT) / 32] |= (1 << ((BIT) % 32)) ))
#define TEST_FSET(SET,BIT) (( (SET)[(BIT) / 32]  & (1 << ((BIT) % 32)) ))

#define SUBST_KEY(p)  ((src_extra*) ((p)->extra))->parsed_info[mn_subst].key
#define SUBST_KIND(p) ((src_extra*) ((p)->extra))->parsed_info[mn_subst].kind
#define SUBST_IS_CLIST(p) ((src_extra*) ((p)->extra))->parsed_info[mn_subst].is_clist
#define SUBST_IS_SINFO(p) ((src_extra*) ((p)->extra))->parsed_info[mn_subst].is_sinfo
#define SUBST_ASM_PP(p) ((src_extra*) ((p)->extra))->parsed_info[mn_subst].asm_pp
#define SET_CHANGE(F,V,C) set_change(F,V,C)

#endif

