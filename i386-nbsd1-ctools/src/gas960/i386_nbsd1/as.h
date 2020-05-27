/* as.h - global header file
   Copyright (C) 1987, 1990, 1991 Free Software Foundation, Inc.

This file is part of GAS, the GNU Assembler.

GAS is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GAS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GAS; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* static const char rcsid[] = "$Id: as.h,v 1.40 1995/12/06 22:48:21 paulr Exp $"; */

#define GAS 1

#include "host.h"

#ifndef __STDC__
#define	volatile	/**/
#ifndef const
#define	const		/**/
#endif /* const */
#endif /* __STDC__ */

#ifdef __GNUC__
#define alloca __builtin_alloca
#define register
#endif /* __GNUC__ */

#ifndef __LINE__
#define __LINE__ "unknown"
#endif /* __LINE__ */

#ifndef __FILE__
#define __FILE__ "unknown"
#endif /* __FILE__ */

/*
 * Naming conventions used (mostly) throughout the sources:
 *
 * CAPITALISED names are #defined.
 * "lowercaseH" is #defined if "lowercase.h" has been #include-d.
 * "lowercaseT" is a typedef of "lowercase" objects.
 * "lowercaseP" is type "pointer to object of type 'lowercase'".
 * "lowercaseS" is typedef struct ... lowercaseS.
 *
 * #define DEBUG to enable all the "know" assertion tests.
 * #define SUSPECT when debugging.
 * #define COMMON as "extern" for all modules except one, where you #define
 *	COMMON as "".
 * If TEST is #defined, then we are testing a module: #define COMMON as "".
 */

/* These #defines are for parameters of entire assembler. */

/* #define SUSPECT JF remove for speed testing */
/* These #includes are for type definitions etc. */

#include <stdio.h>
#include <assert.h>

#define obstack_chunk_alloc	xmalloc
#define obstack_chunk_free	xfree

#define BAD_CASE(val)							\
{									\
  as_fatal("Case value %d unexpected at line %d of file \"%s\"\n",	\
	   val, __LINE__, __FILE__);					\
}

 
/* These are assembler-wide concepts */


#ifndef COMMON
#ifdef TEST
#define COMMON			/* declare our COMMONs storage here. */
#else
#define COMMON extern		/* our commons live elswhere */
#endif
#endif
				/* COMMON now defined */
#ifdef DEBUG
#undef NDEBUG
#define know(p) assert(p)	/* Verify our assumptions!  Please note that
				 the argument can NOT contain newlines.  For example:
				 know(i == 0 ||
				      j == i);
				 Is bad.  Substitute with:
				 know(i == 0 ||\
				      j == i); 
				 However, this does not work on the above hosts for one
				 reason or another. */
                                 /* This is left here as a debug aid to
				    help find a problem in a named bss
				    section where its contents is
				    non-zero. You should never need to
				    enable this, because there should be
				    no way to make such an entry.
				    However, just in case. */
extern void make_sure_zero();    /* in read.c */
#define MAKE_SURE_ZERO(P,N)      make_sure_zero(P,N)

#else
#define know(p)			/* know() checks are no-op.ed */
#define MAKE_SURE_ZERO(P,N)     1
#endif

#define xfree free

/* input_scrub.c */

/*
 * Supplies sanitised buffers to read.c.
 * Also understands printing line-number part of error messages.
 */

/*
 * INPUT_BUFFER_SIZE is supposed to be a number chosen for speed.
 * The caller only asks once what INPUT_BUFFER_SIZE is, and asks before
 * the nature of the input files (if any) is known.
 *
 * LISTING_BUFFER_SIZE must be significantly larger than INPUT_BUFFER_SIZE,
 * because it will also contain comment chars, strings of spaces, etc.  
 */

#define INPUT_BUFFER_SIZE (32 * 1024)
#define LISTING_BUFFER_SIZE (INPUT_BUFFER_SIZE * 2)

#ifdef	DOS
#define	PATHNAME_DELIM		'\\'
#define	PATHNAME_DELIM_STR	"\\"
#define isslash(c)		((c == '/') || (c == '\\'))
#else
#define PATHNAME_DELIM		'/'
#define PATHNAME_DELIM_STR	"/"
#define isslash(c)		(c == '/')
#endif

/* 
 * The input handlers are parameterized so that we don't slow down 
 * the assembler when it's NOT supposed to make a listing file. 
 */
int (* scrub_from_file) ();
void (* scrub_to_file) ();

/*
 * This table describes the use of segments as EXPRESSION types.
 *
 *	X_seg	X_add_symbol  X_subtract_symbol	X_add_number
 * SEG_ABSENT						no (legal) expression
 * SEG_PASS1						no (defined) "
 * SEG_BIG					*	> 32 bits const.
 * SEG_ABSOLUTE				     	0
 * SEG_DATA		*		     	0
 * SEG_TEXT		*			0
 * SEG_BSS		*			0
 * SEG_UNKNOWN		*			0
 * SEG_DIFFERENCE	0		*	0
 * SEG_REGISTER					*
 *
 * The blank fields MUST be 0, and are nugatory.
 * The '0' fields MAY be 0. The '*' fields MAY NOT be 0.
 *
 * SEG_BIG: X_add_number is < 0 if the result is in
 *	generic_floating_point_number.  The value is -'c' where c is the
 *	character that introduced the constant.  e.g. "0f6.9" will have  -'f'
 *	as a X_add_number value.
 *	X_add_number > 0 is a count of how many littlenums it took to
 *	represent a bignum.
 * SEG_DIFFERENCE:
 * If segments of both symbols are known, they are the same segment.
 * X_add_symbol != X_sub_symbol (then we just cancel them, => SEG_ABSOLUTE).
 */

typedef enum {
	SEG_ABSOLUTE = 0,
	SEG_TEXT,
	SEG_DATA,
	SEG_BSS,
	SEG_UNKNOWN,
	SEG_ABSENT,		/* Mythical Segment (absent): NO expression seen. */
	SEG_PASS1,		/* Mythical Segment: Need another pass. */
	SEG_GOOF,		/* Only happens if AS has a logic error. */
				/* Invented so we don't crash printing */
				/* error message involving weird segment. */
	SEG_BIG,		/* Bigger than 32 bits constant. */
	SEG_DIFFERENCE,		/* Mythical Segment: absolute difference. */
	SEG_DEBUG,		/* Debug segment */
	SEG_REGISTER 		/* Mythical: a register-valued expression */
} segT;

#define SEG_MAXIMUM_ORDINAL (SEG_REGISTER)


extern char *const seg_name[];
extern int section_alignment[];


/* relax() */

typedef enum
{
	rs_fill,		/* Variable chars to be repeated fr_offset */
				/* times. Fr_symbol unused. */
				/* Used with fr_offset == 0 for a constant */
				/* length frag. */

	rs_align,		/* Align: Fr_offset: power of 2. */
				/* 1 variable char: fill character. */
	rs_org,			/* Org: Fr_offset, fr_symbol: address. */
				/* 1 variable char: fill character. */

	rs_machine_dependent
}
relax_stateT;

/* typedef unsigned char relax_substateT; */
/* JF this is more likely to leave the end of a struct frag on an align
   boundry.  Be very careful with this.  */
typedef unsigned long relax_substateT;

typedef unsigned long relax_addressT;/* Enough bits for address. */
				/* Still an integer type. */


/* frags.c */

/*
 * A code fragment (frag) is some known number of chars, followed by some
 * unknown number of chars. Typically the unknown number of chars is an
 * instruction address whose size is yet unknown. We always know the greatest
 * possible size the unknown number of chars may become, and reserve that
 * much room at the end of the frag.
 * Once created, frags do not change address during assembly.
 * We chain the frags in (a) forward-linked list(s). The object-file address
 * of the 1st char of a frag is generally not known until after relax().
 * Many things at assembly time describe an address by {object-file-address
 * of a particular frag}+offset.

 BUG: it may be smarter to have a single pointer off to various different
notes for different frag kinds. See how code pans 
 */
struct frag			/* a code fragment */
{
	unsigned long fr_address; /* Object file address. */
	struct frag *fr_next;	/* Chain forward; ascending address order. */
				/* Rooted in frch_root. */
	struct frag *fr_prev;	/* Chain backward; useful for var frags */
	long fr_fix;	/* (Fixed) number of chars we know we have. */
				/* May be 0. */
	long fr_var;	/* (Variable) number of chars after above. */
				/* May be 0. */
	struct symbol	*fr_symbol; /* For variable-length tail. */
	long 		fr_offset;	/* For variable-length tail. */
	char		*fr_opcode;	/*->opcode low addr byte,for relax()ation*/
	relax_stateT 	fr_type;	/* What state is my tail in? */
	relax_substateT	fr_subtype;
	char		fr_big_endian;
		/* This is needed only on the NS32K machines */
	char		fr_bsr;
	char		fr_literal [1];	/* Chars begin here. */
				/* One day we will compile fr_literal[0]. */
};
#define SIZEOF_STRUCT_FRAG \
 ((int)zero_address_frag.fr_literal-(int)&zero_address_frag)
				/* We want to say fr_literal[0] above. */

typedef struct frag fragS;

COMMON fragS *	curr_frag;	/* -> current frag we are building. */
				/* This frag is incomplete. */

extern fragS zero_address_frag;	/* For foreign-segment symbol fixups. */


/* listing.c */

/* types of listing info lines */
enum lst_status
{
	complete,
	varying,
	fill,
	info
};

/* types of listing info lines with "info" status */
enum lst_info_type
{
	segment_change,
	source_change, 
	include_start,
	bss,
	eject, 
	error
};

typedef struct
{
	enum lst_status	status;		/* does more work need to be done? */
	long		lineid;		/* line # in listing, not source */
	long 		srcline;	/* line # in current source file */
	long		size;		/* total number of machine bytes to list */
	int		byteswap;	/* 1 == byteswap (for printing instructions bigendian) */
	char		*frag;		/* ptr to frag structure */
	char 		*fragptr;	/* ptr to this line's code within frag */
}	listing_line_info;


/* 
 * The following tells the listing backend to reverse the endianness 
 * of machine words, unless the target is already big-endian, or unless
 * the user requested strict target-endian printing.
 */
#define LIST_BIGENDIAN ( ! SEG_IS_BIG_ENDIAN(curr_seg) && ! listflagseen['e'])

COMMON char *
listing_file_name;		/* name of assembler listing file */

COMMON char 
listing_error_tmpbuf[256];	/* For echoing error messages to the listing file */

COMMON int
listing_now;			/* TRUE if a .list has been seen or implied */

COMMON int
listing_pass2;			/* TRUE if input processing is completed */

COMMON int
listing_include;		/* TRUE if we are currently in an include file */

COMMON listing_line_info
listinfo;			/* Global structure for source line listing info */


/* main program "as.c" (command arguments etc) */

COMMON char
flagseen[128];			/* ['x'] TRUE if "-x" seen. */

COMMON char
listflagseen[128];		/* listing flags, patterned after flagseen[] */

COMMON char *
out_file_name;			/* name of emitted object file */

#define STDIN_FILENAME "{standard input}"
				/* for error messages when input is stdin */

COMMON int	need_pass_2;	/* TRUE if we need a second pass. */
COMMON int	need_pseudo_pass_2;	/* TRUE if we have any delayed exprs */

/* A struct to hold ".set" symbols whose values could not be
 * calculated during pass 1.
 */
typedef struct del_expr
{
	struct symbol 	*symP;	/* symbol to set in pseudo pass 2. */
	char		*exprP; /* pointer to original expression. */
	struct del_expr *next;  /* for chaining. */
} delayed_exprS;

COMMON delayed_exprS	*del_expr_rootP;
COMMON delayed_exprS	*del_expr_lastP;

typedef struct {
  char *	poc_name;	/* assembler mnemonic, lower case, no '.' */
  void		(*poc_handler)();	/* Do the work */
  int		poc_val;	/* Value to pass to handler */
} pseudo_typeS;

typedef struct
{
	char	*name;		/* Environment var name */
	char	*value;		/* and its setting (NULL if not set) */
} env_varS;



#if defined(__STDC__) && !defined(NO_STDARG)
    int had_errors(void);
    int had_warnings(void);
    void as_bad(const char *Format, ...);
    void as_fatal(const char *Format, ...);
    void as_tsktsk(const char *Format, ...);
    void as_warn(const char *Format, ...);
#else
    int had_errors();
    int had_warnings();
    void as_bad();
    void as_fatal();
    void as_tsktsk();
    void as_warn();
#endif /* __STDC__ & !NO_STDARG */

#ifdef __STDC__

char *app_push(void);
char *env_var(char *name);
char *input_scrub_include_file(char *filename, char *position);
char *input_scrub_new_file(char *filename);
char *input_scrub_next_buffer(char **bufp);
char *strstr(const char *s, const char *wanted);
char *xmalloc(int size);
char *xrealloc(char *ptr, long n);
FILE *create_temp_file(char **name, char *mode);
int do_scrub_next_char(int (*get)(), void (*unget)());
int had_err(void);
int had_errors(void);
int had_warnings(void);
int ignore_input(void);
int scrub_from_file_normal(void);
int scrub_from_file_listing(void);
int scrub_from_string(void);
int seen_at_least_1_file(void);
int seg_new(segT type, char *name, int flags);
long report_line_number(void);
void app_pop(char *arg);
void as_howmuch(FILE *stream);
void as_perror(char *gripe, char *filename);
void as_where(void);
void bump_line_counters(void);
void do_scrub_begin(void);
void input_scrub_begin(void);
void input_scrub_close(void);
void input_scrub_end(void);
void new_logical_line(char *fname, int line_number);
void scrub_to_file_normal(int ch);
void scrub_to_file_listing(int ch);
void scrub_to_string(int ch);
void seg_change(int seg);
void seg_finish(int seg);
void segs_begin(void);
void segs_push_down(int);
void frags_begin(void);
void cond_begin(void);

#else /* __STDC__ */

char *app_push();
char *env_var();
char *input_scrub_include_file();
char *input_scrub_new_file();
char *input_scrub_next_buffer();
char *strstr();
char *xmalloc();
char *xrealloc();
FILE *create_temp_file();
int do_scrub_next_char();
int had_err();
int had_errors();
int had_warnings();
int ignore_input();
int scrub_from_file_normal();
int scrub_from_file_listing();
int scrub_from_string();
int seen_at_least_1_file();
int seg_new();
long report_line_number();
void app_pop();
void as_howmuch();
void as_perror();
void as_where();
void bump_line_counters();
void do_scrub_begin();
void input_scrub_begin();
void input_scrub_close();
void input_scrub_end();
void new_logical_line();
void scrub_to_file_normal();
void scrub_to_file_listing();
void scrub_to_string();
void seg_change();
void seg_finish();
void segs_begin();
void segs_push_down();
void frags_begin();
void cond_begin();

#endif /* __STDC__ */

#ifdef GNU960
#	define OBJ_COFF_IGNORE_EXEC_FLAG
#	define OBJ_COFF_OMIT_OPTIONAL_HEADER
#	include "objfmt.h"
#else
	 /* this one starts the chain of target dependant headers */
#	include "targ-env.h"
#endif /* GNU960 */

/* segs.c */

#include "segs.h"

 /* these define types needed by the interfaces */
#ifdef GNU960
#	include "strucsym.h"
#else
#	include "struc-symbol.h"
#endif
#include "reloc.h"
#include "write.h"
#include "flonum.h"
#include "expr.h"
#include "frags.h"
#include "hash.h"
#include "read.h"
#include "symbols.h"

#include "tc.h"
#include "obj.h"

/*
 * Local Variables:
 * comment-column: 0
 * fill-column: 131
 * End:
 */

/* end: as.h */

extern char * _S_GET_PRTABLE_NAME();
#define S_GET_PRTABLE_NAME(SYM) _S_GET_PRTABLE_NAME( S_GET_NAME( SYM ) )
