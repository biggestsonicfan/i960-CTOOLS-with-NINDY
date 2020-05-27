#ifndef CC_PINFO_H
#define CC_PINFO_H

/*  COL_BITS denotes the number of bits we give (out of 32) to column position.
    MAX_LIST_COL is based on COL_BITS.  All column positions >= MAX_LIST_COL
    are represented as MAX_LIST_COL. */

#define COL_BITS 8
#define MAX_LIST_COL (((1 << COL_BITS)-1))

#define GET_LINE(P)  (( ((unsigned)(P)) >> COL_BITS ))
#define GET_COL(P)   (( (P) & MAX_LIST_COL ))

struct lineno_block_info
{
  int id;
  unsigned lineno;
  int blockno;
};

#define GET_FDEF_PROF(f) (f->db_list[CI_FDEF_PROF_LIST]+0)
#define SET_FDEF_PROF(f,p) db_replace_list(f,CI_FDEF_PROF_LIST,p)

#define MAX_WEIGHT              1E+36
#define START_WEIGHT            100.0
#define LOOP_WEIGHT             10.0
#define MIN_WEIGHT              0.0
#define UNSET_WEIGHT            -1.0

#endif
