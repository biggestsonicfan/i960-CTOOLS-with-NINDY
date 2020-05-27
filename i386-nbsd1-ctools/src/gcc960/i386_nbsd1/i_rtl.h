typedef struct {
  unsigned rtx_id:16;
  unsigned var_id:16;
} dfa_info1;

/* These fields are used by new DFA. Each MEM, SYMBOL_REF, and REG
   has a '0' format field for fields 1 and 2. */
#define RTX_ID(X)      (( ((dfa_info1 *) &((X)->fld[1].rtint))[0].rtx_id  ))
#define RTX_VAR_ID(X)  (( ((dfa_info1 *) &((X)->fld[1].rtint))[0].var_id  ))
#define RTX_TYPE(X)    (( *((tree *)     &((X)->fld[2].rtx))              ))
#define RTX_MRD_SET(X) (( *((unsigned *) &((X)->fld[3].rtint))            ))

#define COPY_MEM_MRD_FIELDS(N,O) \
do {\
  RTX_VAR_ID(N)=RTX_VAR_ID(O); \
  (N)->fld[2] = (O)->fld[2]; \
  (N)->fld[3] = (O)->fld[3]; \
  (N)->report_undef = (O)->report_undef; \
  (N)->bit8_flag    = (O)->bit8_flag; \
} while (0)

/* Each mem carries a flex set of all the alias classes it belongs in */ 
#define SET_CAN_MOVE_MEM(X) (RTX_MRD_SET(X) |= 0x80000000)
#define CLR_CAN_MOVE_MEM(X) (RTX_MRD_SET(X) &= 0x7FFFFFFF)
#define CAN_MOVE_MEM_P(X) (((int) RTX_MRD_SET(X)) < 0)
#define PUT_MEM_CONFLICT(X,V) \
  (( RTX_MRD_SET(X) = (RTX_MRD_SET(X) & 0x80000000) | ((V) & 0x7FFFFFFF) ))
#define GET_MEM_CONFLICT(X)   (( RTX_MRD_SET(X) & 0x7FFFFFFF          ))

/* Change an rtx from one flavor to another */
#define PUT_CODE(X,C) \
do {\
  enum rtx_code __c__ = (enum rtx_code) (C);\
  rtx __r__ = (X);\
  if (__c__==REG || __c__==MEM || __c__==SYMBOL_REF)\
  { \
    RTX_ID(__r__) = 0; \
    RTX_VAR_ID(__r__) = 0; \
  };\
  __r__->code = __c__; \
} while (0)

/* Initialize a brand new rtx */
#define INIT_RTX(X,C,L)\
do {\
  rtx __r__; unsigned *__p__;  int __i__;\
  __r__ = (X);\
  __p__ = (unsigned *) __r__;\
  __i__ = (L)/sizeof(unsigned);\
  do \
    __p__[--__i__] = 0; \
  while\
    (__i__);\
  __r__->code = (C); \
} while (0)

#define GET_CODE(r) (((enum rtx_code) (r)->code))

/* This enumeration is for the various kinds of data flow universes */
typedef enum { DF_SYM, DF_REG, DF_MEM, NUM_DFK } df_kind;
#define IDF_SYM ((int)DF_SYM)
#define IDF_REG ((int)DF_REG)
#define IDF_MEM ((int)DF_MEM)
#define INUM_DFK ((int)NUM_DFK)

/* This array indicates the last global id for each universe. */
extern unsigned short last_global_rtx_id [NUM_DFK+1];


#define IS_SEXTEND(C) (( (C)==SIGN_EXTRACT||(C)==SIGN_EXTEND ))
#define IS_ZEXTEND(C) (( (C)==ZERO_EXTRACT||(C)==ZERO_EXTEND ))

#define RTX_BITS(X)           (( GET_MODE_BITSIZE(GET_MODE(X)) ))
#define RTX_BYTES(X)          (( GET_MODE_SIZE(GET_MODE(X))    ))

#define IS_LVAL_CODE(C) (( (C)==MEM || (C)==SYMBOL_REF || (C)==REG ))
#define IS_LVAL_PARENT_CODE(C) (( (C)==SUBREG||(C)==ZERO_EXTRACT||(C)==SIGN_EXTRACT||(C)==ZERO_EXTEND||(C)==SIGN_EXTEND ))

#define RTX_IS_LVAL(X)        (( IS_LVAL_CODE(GET_CODE(X)) ))
#define RTX_IS_LVAL_PARENT(X) (( IS_LVAL_PARENT_CODE(GET_CODE(X)) && RTX_IS_LVAL(XEXP(X,0)) ))
#define RTX_LVAL(X)      (( *(RTX_IS_LVAL_PARENT(X) ? &(XEXP((X),0)) : &(X)) ))

#define RTX_IS_SEXTEND(X) IS_SEXTEND(GET_CODE(X))
#define RTX_IS_ZEXTEND(X) IS_ZEXTEND(GET_CODE(X))

#define TF_BITS      (( GET_MODE_BITSIZE(TFmode) ))
#define DF_BITS      (( GET_MODE_BITSIZE(DFmode) ))
#define SF_BITS      (( GET_MODE_BITSIZE(SFmode) ))
#define TI_BITS      (( GET_MODE_BITSIZE(TImode) ))
#define DI_BITS      (( GET_MODE_BITSIZE(DImode) ))
#define SI_BITS      (( GET_MODE_BITSIZE(SImode) ))
#define HI_BITS      (( GET_MODE_BITSIZE(HImode) ))
#define QI_BITS      (( GET_MODE_BITSIZE(QImode) ))

#define TF_BYTES      (( GET_MODE_SIZE(TFmode) ))
#define DF_BYTES      (( GET_MODE_SIZE(DFmode) ))
#define SF_BYTES      (( GET_MODE_SIZE(SFmode) ))
#define TI_BYTES      (( GET_MODE_SIZE(TImode) ))
#define DI_BYTES      (( GET_MODE_SIZE(DImode) ))
#define SI_BYTES      (( GET_MODE_SIZE(SImode) ))
#define HI_BYTES      (( GET_MODE_SIZE(HImode) ))
#define QI_BYTES      (( GET_MODE_SIZE(QImode) ))

extern rtx pid_reg_rtx;

extern rtx gen_typed_mem_rtx ();
extern rtx gen_symref_rtx ();
extern rtx gen_typed_symref_rtx ();
extern rtx gen_typed_reg_rtx ();
extern rtx pun_rtx();
extern rtx pun_rtx_offset();
extern rtx embed_type();
extern rtx change_address_type();
extern rtx set_rtx_type ();
extern rtx set_rtx_mode_type ();

/* If the register is assocaited with a user variable, this is its name. */
#define REG_USERNAME(RTX)  ((RTX)->fld[3].rtstr)

#define SYM_ADDR_TAKEN_P(SYM) ((SYM)->volatil)
#define INSN_REPORT_UNUSED(I)    ((I)->call)
#define INSN_REPORT_UNDEF(I) ((I)->report_undef)

/*
 * SYMBOL_REF nodes have a field that is used to construct lists of them
 * for management purposes.  They also have a field that is used to store
 * usage counts, and a field for some other information.  These are all
 * considered internal fields.
 */
#define SYMREF_ADDRTAKEN(X) ((X)->bit8_flag)
#define SYMREF_USECNT(X)    ((X)->fld[3].rtint)
#define SYMREF_ETC(X)       ((X)->fld[4].rtint)
#define SYMREF_NEXT(X)      ((X)->fld[5].rtx)
#define SYMREF_SIZE(X)      ((X)->fld[6].rtint)

/* For making a list of all functions in the compilation unit which
   reference each symbol, to be listed when the VDEF/VREF/FREF/FDEF
   is put out in i_glob_db.c */

#define SET_SYMREF_USEDBY(X,Y)    (( (X)->fld[7].rtstr=(char *)(Y) ))
#define GET_SYMREF_USEDBY(X)      (( (struct _db_nameref *) (X)->fld[7].rtstr ))

typedef
  struct _db_nameref {
    struct _db_nameref *prev;
    rtx fname_sym;
  } db_nameref;
/*
 * MASKS for use with SYMREF_ETC field.
 */
#define SYMREF_VARBIT      0x1    /* Is the symbol a variable? */
#define SYMREF_FUNCBIT     0x2    /* Is the symbol a function? */
#define SYMREF_DEFINED     0x4    /* Was the symbol bound in this module? */
#define SYMREF_STATICBIT   0x8    /* Is the symbol a static name? */
#define SYMREF_PIDBIT     0x10    /* Is the symbol a pid symbol? */
#define SYMREF_PICBIT     0x20    /* Is the symbol a pic symbol? */

extern rtx gen_symref_rtx ();

/* The probability the then side of the if_the_else branch will be taken. */
#define JUMP_THEN_PROB(INSN) ((INSN)->fld[9].rtint)

/* The trip count for loop begin notes. */
#define NOTE_TRIP_COUNT(INSN) ((INSN)->fld[5].rtint)

/* The call count for function begin notes. */
#define NOTE_CALL_COUNT(INSN) ((INSN)->fld[5].rtint)

/* 1 if insn was generated as part of profiling instrumentation. */
#define INSN_PROFILE_INSTR_P(INSN)   ((INSN)->bit8_flag)

/* In line number notes, we record where we are in the listing file,
   so we can insert assembly at the right point in the listing. */
#define NOTE_LISTING_START(N) ((N)->fld[6].rtint)
#define NOTE_LISTING_END(N)   ((N)->fld[7].rtint)

/* Extract the source column number from a line number note.
   This is the sinfo column number, not the actual source file column number.
   Within a given source line, sinfo column numbers have the same relative
   ordering as real column numbers, and so are used throughout compilation
   to distinguish between different statements on the same line.
 */

#if defined(NOTE_LISTING_START) && defined(GET_COL)
#define NOTE_COLUMN_NUMBER(n)	(GET_COL(NOTE_LISTING_START(n)))
#else
#define NOTE_COLUMN_NUMBER(n)	(0)
#define COLUMN_INFO_UNAVAILABLE
#endif

#define NOTE_COPY_COLUMN_NUMBER(dst, src) \
		SET_COL(NOTE_LISTING_START(dst), NOTE_COLUMN_NUMBER(src))

#define IS_LINE_NUMBER_NOTE_P(n) \
		(GET_CODE(n) == NOTE && (NOTE_LINE_NUMBER(n) >= 0))

#if defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2

/* In line number notes, maintain an extra field of information for Elf/DWARF.
   This integer field, NOTE_AUX_SRC_INFO, is divided into several bit fields,
   accessed via the macros below.
   At the time of this writing, use of these macros is all guarded with

     #if defined(IMSTG) && defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2
	if (flag_linenum_mods)
		... use NOTE_AUX_SRC_INFO and NOTE_COLUMN_NUMBER
		    to improve granularity of debug information ...
	else
		... do not use NOTE_AUX_SRC_INFO nor NOTE_COLUMN_NUMBER ...
     #endif

   flag_linenum_mods is only enabled under -Felf, and is not dependent
   on -g.  Thus, line number note tracking for other OMFs (bout, coff)
   does not use these fields.
 */

#define NOTE_AUX_SRC_INFO(N) ((N)->fld[5].rtint)

#define IMSTG_NOTE_INSTANCE_MASK	0x0000ffff
#define IMSTG_NOTE_SRC_COL_MASK		0x00ff0000
#define IMSTG_NOTE_IS_STMT_MASK		0x01000000
#define IMSTG_NOTE_LEFTMOST_MASK	0x02000000

/* Optimizations that duplicate code should also duplicate the corresponding
   line number notes.  To distinguish between an original line number note,
   and its duplicate(s), assign a unique "instance number" to each duplicate.
   A counter is incremented to obtain new instance numbers.
   The current instance number is automatically assigned to line number
   note rtx's when they are created in emit-rtl.c.
 */

#define NOTE_GET_INSTANCE_NUMBER(N) \
		(NOTE_AUX_SRC_INFO(N) & IMSTG_NOTE_INSTANCE_MASK)

#define NOTE_SET_INSTANCE_NUMBER(N,n)  (NOTE_AUX_SRC_INFO(N) = \
		((NOTE_AUX_SRC_INFO(N) & (~IMSTG_NOTE_INSTANCE_MASK)) \
		  | ((n) & IMSTG_NOTE_INSTANCE_MASK)))

/* Indicate whether a line number note marks the beginning of a statement.
   This is true by default (ie, at initial RTL generation).  Optimizations
   that break the contiguity of a statement's instructions, must put a line
   number note before each separate chunk of contiguous instructions and
   mark exactly one such chunk as the beginning of the statement.  This is
   where the debugger will set a breakpoint for the statement.
   Optimizations that duplicate code sequences, must duplicate the line
   number notes and the is_stmt field as well so that multiple semantic
   breakpoints will be set for a given syntactic breakpoint.
 */

	/* Let the default value be a zero-bit rather than a one-bit,
	   to help reduce clutter in RTL dumps.
	 */
#define NOTE_STMT_BEGIN_P(N) \
		((NOTE_AUX_SRC_INFO(N) & IMSTG_NOTE_IS_STMT_MASK) == 0)
#define NOTE_CLEAR_STMT_BEGIN(N) \
		(NOTE_AUX_SRC_INFO(N) |= IMSTG_NOTE_IS_STMT_MASK)
#define NOTE_SET_STMT_BEGIN(N) \
		(NOTE_AUX_SRC_INFO(N) &= (~IMSTG_NOTE_IS_STMT_MASK))

#define NOTE_COPY_STMT_BEGIN(dst, src) \
		(NOTE_AUX_SRC_INFO(dst) |= \
			(NOTE_AUX_SRC_INFO(src) & IMSTG_NOTE_IS_STMT_MASK))

/* Indicate whether a line number note marks the left-most statement on
   a source line.  If so, we say the column number is 0 in the DWARF
   .debug_line information.  This allows .debug_line to be more compact,
   since we don't have to specify the exact column information for most
   left-most statements (they'll usually inherit the 0 from the previous
   statement's column information).
   This flag is set in final(), when making a prepass over the insns.
 */

#define NOTE_LEFTMOST_STMT_P(N) \
			(NOTE_AUX_SRC_INFO(N) & IMSTG_NOTE_LEFTMOST_MASK)
#define NOTE_SET_LEFTMOST_STMT(N) \
			(NOTE_AUX_SRC_INFO(N) |= IMSTG_NOTE_LEFTMOST_MASK)

/* Record the actual source column number.  This is initialized in
   dwarfout_simplify_notes(), before start_func_listing_info() has run.
   The order here is important, because start_func_listing_info() destroys
   information in NOTE_LISTING_START which is needed to map sinfo line and
   column information into the real source column.
   (For line number notes describing cave stub functions, this info
   is initialized in imstg_cave_final_{start,pre_end}_function().)
 */

#define NOTE_GET_REAL_SRC_COL(N) \
		((NOTE_AUX_SRC_INFO(N) & IMSTG_NOTE_SRC_COL_MASK) >> 16)

#define NOTE_SET_REAL_SRC_COL(N,c)  (NOTE_AUX_SRC_INFO(N) = \
		((NOTE_AUX_SRC_INFO(N) & (~IMSTG_NOTE_SRC_COL_MASK)) \
		  | (((c) << 16) & IMSTG_NOTE_SRC_COL_MASK)))

/* Determine if two line number notes represent the same source object.
   We use column information to distinguish notes only for Elf/DWARF.
 */

#define NOTES_REP_SAME_SOURCE_P(x,y) \
    (NOTE_SOURCE_FILE(x) == NOTE_SOURCE_FILE(y) \
     && NOTE_LINE_NUMBER(x) == NOTE_LINE_NUMBER(y) \
     && (NOTE_COLUMN_NUMBER(x) == NOTE_COLUMN_NUMBER(y)))

/* From a NOTE_INSN_DWARF_LE_BOUNDARY note, extract the integer which will be
   used in the name of the label generated to mark the boundary.
 */
#define NOTE_LE_BOUNDARY_NUMBER(N) ((N)->fld[5].rtint)

#define DWARF_SET_ALID(X) ((X)->fld[2].rtint)

/* Return the nearest line number note associated with the given insn,
   or NULL if one can't be found.
 */
extern rtx dwarfout_effective_note	PROTO((rtx /*insn*/));

/* Determine if 'insn' is the last active insn corresponding to the
   given line number note.  Assume 'note' is the insn's effective note.
 */
extern int dwarfout_last_insn_p		PROTO((rtx /*insn*/, rtx /*note*/));

/* Disable the line number note(s) corresponding to an insn that is being
   deleted.  This is done only if the insn is the last real insn
   associated with the line number notes.
   The notes are disabled by changing them to NOTE_INSN_DELETED notes.
   They are not detached from the insn stream.
 */
extern void dwarfout_disable_notes	PROTO((rtx /*deleted_insn*/));

/* From sinfo line and column, obtain a real source column number.
   0 is returned if list_pos is bogus.
 */
extern int imstg_map_sinfo_col_to_real_col( /* list_pos */ );

#endif /* DWARF */

/* Set up in expect.c; 0 before then.
   Gives the number of times you 'expect' to execute this particular
   insn.
   This field is defined for INSN, JUMP_INSN, CALL_INSN, and CODE_LABEL.
   Shares space with INSN_PROF_OFFSET.  */

#define INSN_EXPECT(INSN) ((INSN)->fld[7].rtint)

/* Set in profile.c; when instrumenting the code to collect profile information.
   Gives the offset into the profile_data array for information about
   the insn.

   Defined for CALL_INSN and JUMP_INSN.
   We share space with the INSN_EXPECT so once EXPECT_Calculate has
   been called this information has been overwritten.  */

#define INSN_PROF_DATA_INDEX(INSN) ((INSN)->fld[7].rtint)

/* Set in start_func_info in glob_db.c: 0 before then.
   Associates a number with each CALL_INSN, which can be used to work
   with the global function info. Defined only for CALL_INSN. */

#define INSN_CALL_NUM(INSN)  ((INSN)->fld[8].rtint)

/*
 * Set in calls.c to be the paramater and return type encoding if we
 * are building a data base.  Note this is the same field as INSN_CALL_NUM,
 * so start_func_info needs to be careful about not setting INSN_CALL_NUM,
 * until after it has gathered the INSN_CALL_PTYPE information.
 */
#define INSN_CALL_PTYPE(INSN) ((INSN)->fld[8].rtstr)

/*
 * defines for codes that are put on insns telling how to translate
 * various things in the INSN when a function is being inlined.
 */
#define INLN_NORMAL       0     /* nothing special */
#define INLN_DEL_CALLEE   1     /* delete if in inlined routine */
#define INLN_DEL_CALLER   2     /* delete if in calling routine */
#define INLN_PARAM_CALLEE 3     /* substitute param if inlined */
#define INLN_PARAM_CALLER 4     /* substitute param if in calling routine */
#define INLN_RET_CALLEE   5     /* substitute ret_val if in inlined routine */
#define INLN_RET_CALLER   6     /* substitute ret_val if in calling routine */

#define INSN_INLN_TRANSLATE(INSN) ((INSN)->fld[8].rtint)

/*  All insns which have the same non-zero value for this field
    represent a torn-apart TI or DI move;  we use this field
    to put them back together, if possible. see split_mem_refs
    and join_mem_refs */

#define INSN_MULTI_MOVE(INSN) ((INSN)->fld[8].rtint)

#define COMPARE_MODE(X) ((X)->fld[2].rttype)

/* For a GLOBAL_INLINE_HEADER rtx */
#define GLOB_INLN_FRAME_SIZE(RTX)     ((RTX)->fld[3].rtint)
#define GLOB_INLN_FIRST_LABELNO(RTX)  ((RTX)->fld[4].rtint)
#define GLOB_INLN_LAST_LABELNO(RTX)   ((RTX)->fld[5].rtint)
#define GLOB_INLN_NUM_REGS(RTX)       ((RTX)->fld[6].rtint)
#define GLOB_INLN_NUM_UIDS(RTX)       ((RTX)->fld[7].rtint)
#define GLOB_INLN_NUM_SYMS(RTX)       ((RTX)->fld[8].rtint)
#define GLOB_INLN_RET_RTX(RTX)        ((RTX)->fld[9].rtx)
#define GLOB_INLN_PARMS(RTX)          ((RTX)->fld[10].rtvec)
#define GLOB_INLN_NUM_PARMS(RTX)      ((RTX)->fld[10].rtvec->num_elem)
#define GLOB_INLN_PARM_RTX(RTX,N)     ((RTX)->fld[10].rtvec->elem[(N)].rtx)

#ifdef IMSTG_REAL
#define IS_CONST_ZERO_RTX(X) \
 (((X) != 0) && \
  ((GET_CODE(X)==CONST_INT && INTVAL(X)==0) || \
   (GET_CODE(X)==CONST_DOUBLE && GET_MODE_CLASS(GET_MODE(X))==MODE_INT && \
    CONST_DOUBLE_LOW(X)==0 && CONST_DOUBLE_HIGH(X)==0) || \
   (GET_CODE(X)==CONST_DOUBLE && GET_MODE_CLASS(GET_MODE(X))==MODE_FLOAT && \
    !bcmp (&CONST_DOUBLE_LOW(X), &CONST_DOUBLE_LOW(CONST0_RTX(GET_MODE(X))), \
		  sizeof (REAL_VALUE_TYPE)))))
#else
#define IS_CONST_ZERO_RTX(X) \
 (((X) != 0) && \
  ((GET_CODE(X)==CONST_INT && INTVAL(X)==0) || \
   (GET_CODE(X)==CONST_DOUBLE && \
    CONST_DOUBLE_LOW(X)==0 && CONST_DOUBLE_HIGH(X)==0)))
#endif

#define GET_CONST_ZERO_RTX(M)  CONST0_RTX(M)

#define START_SEQUENCE(SAV) start_sequence ()
#define END_SEQUENCE(SAV) end_sequence ()
extern rtx cvt_expr();

extern rtx current_function_argptr_insn;

extern char * db_cp_encode_buf();
