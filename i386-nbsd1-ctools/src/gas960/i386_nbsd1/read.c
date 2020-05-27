/* read.c - read a source file -
   Copyright (C) 1986, 1987, 1990, 1991 Free Software Foundation, Inc.

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
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. */

/* static const char rcsid[] = "$Id: read.c,v 1.68 1995/12/15 22:28:42 paulr Exp $"; */

#define MASK_CHAR (0xFF)	/* If your chars aren't 8 bits, you will
				   change this a bit.  But then, GNU isn't
				   spozed to run on your machine anyway.
				   (RMS is so shortsighted sometimes.)
				   */

#define MAXIMUM_NUMBER_OF_CHARS_FOR_FLOAT (16)
				/* This is the largest known floating point */
				/* format (for now). It will grow when we */
				/* do 4361 style flonums. */

#define MAX_ALIGN_POWER_OF_2 31 /* An absurdly large number, for backwards 
				 * compatibility with prior assemblers, but
				 * we'll print a warning much lower than this
				 */

/* Routines that read assembler source text to build spagetti in memory. */
/* Another group of these functions is in the as-expr.c module */

#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#ifdef DOS
#	include <malloc.h>
#endif

#include "as.h"

#include "obstack.h"

char *input_line_pointer;	/*->next char of source file to parse. */


#if BITS_PER_CHAR != 8
The following table is indexed by [ (char) ] and will break if
a char does not have exactly 256 states (hopefully 0:255!) !
#endif

const char /* used by is_... macros. our ctype[] */
lex_type [256] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,       /* @ABCDEFGHIJKLMNO */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,       /* PQRSTUVWXYZ[\]^_ */
  0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0,       /* _!"#$%&'()*+,-./ */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,       /* 0123456789:;<=>? */
  0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,       /* @ABCDEFGHIJKLMNO */
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 3,       /* PQRSTUVWXYZ[\]^_ */
  0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,       /* `abcdefghijklmno */
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0,       /* pqrstuvwxyz{|}~. */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


/*
 * In: a character.
 * Out: 1 if this character ends a line.
 */
#define _ (0)
char is_end_of_line [256] = {
#ifdef CR_EOL
 _, _, _, _, _, _, _, _, _, _,99, _, _, 99, _, _,/* @abcdefghijklmno */
#else
 _, _, _, _, _, _, _, _, _, _,99, _, _, _, _, _, /* @abcdefghijklmno */
#endif
 _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, /* */
 _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, /* */
 _, _, _, _, _, _, _, _, _, _, _,99, _, _, _, _, /* 0123456789:;<=>? */
 _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, /* */
 _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, /* */
 _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, /* */
 _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, /* */
 _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, /* */
 _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, /* */
 _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, /* */
 _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, /* */
 _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _  /* */
};
#undef _

/* Global vars related to general segments */
extern segS	*segs;		/* Internal segment array */
extern int  	curr_seg;	/* Active segment (index into segs[]) */
extern int	DEFAULT_TEXT_SEGMENT; 	/* .text */
extern int	DEFAULT_DATA_SEGMENT; 	/* .data */
extern int	DEFAULT_BSS_SEGMENT; 	/* .bss  */

extern const char line_comment_chars[];
extern char *physical_input_file;
char line_separator_chars[1];

static char *buffer;		/* 1st char of each buffer of lines is here. */
static char *buffer_limit;	/*->1 + last char in buffer. */

static char *bignum_low;	/* Lowest char of bignum. */
static char *bignum_limit;	/* 1st illegal address of bignum. */
static char *bignum_high;	/* Highest char of bignum. */
				/* May point to (bignum_start-1). */
				/* Never >= bignum_limit. */
static char *old_buffer = 0;	/* JF a hack */
static char *old_input;
static char *old_limit;

/* Variables for handling include file directory list. */

char **include_dirs;		/* List of pointers to directories to
				   search for .include's */
int include_dir_count;		/* How many are in the list */

#ifdef __STDC__

static char *demand_processed_string(int *lenP);
static int is_it_end_of_statement(void);
static segT get_known_segmented_expression(expressionS *expP);
static void grow_bignum(void);
static void pobegin(void);
static void stringer(int append_zero);

#else /* __STDC__ */

static char *demand_processed_string();
static int is_it_end_of_statement();
static segT get_known_segmented_expression();
static void grow_bignum();
static void pobegin();
static void stringer();

#endif /* __STDC__ */

 
void
read_begin()
{
	char 	*p;
	char	*inc_str;	/* For I960INC */

	pobegin();
	obj_read_begin_hook();
	
	obstack_begin(&notes, 5000);
	
	/* Start off the include list with "." (current directory) */
	include_dirs = (char **) xmalloc (sizeof (char *));
	include_dirs[0] = ".";
	include_dir_count = 1;
	
	/* Build up the include list with the I960INC string.
	 * Note that the routine below destroys the I960INC string,
	 * so you can only call read_begin once.
	 */
	if ( (inc_str = env_var("I960INC")) )
	{
		char	*end = inc_str + strlen(inc_str);
		char 	*c;
		for ( ; inc_str < end; inc_str += (strlen(inc_str) + 1) )
		{
			if ( (c = strchr(inc_str, ':')) != NULL )
				*c = 0;
			add_include_dir(inc_str);
		}
	}

	/* Set up the default input scrubbers.  These will be changed
	 * to ones with listing extensions in listing_begin() if a 
	 * listing was requested.
	 */
	scrub_from_file = scrub_from_file_normal;
	scrub_to_file = scrub_to_file_normal;

#define BIGNUM_BEGIN_SIZE (16)
	bignum_low = xmalloc((long)BIGNUM_BEGIN_SIZE);
	bignum_limit = bignum_low + BIGNUM_BEGIN_SIZE;

	/* FIXME-SOMEDAY: Use machine dependent syntax */
	for (p = line_separator_chars; *p; p++)
		is_end_of_line[*p] = 1;
}
 
/* set up pseudo-op tables */

struct hash_control *
po_hash = NULL;			/* use before set up: NULL->address error */

static const pseudo_typeS
potable[] =
{
  { "align",	s_align_ptwo,	0	},
  { "alignfill",s_alignfill,	0	},
  { "ascii",	stringer,	0	},
  { "asciz",	stringer,	1	},
  { "byte",	cons,		1	},
  { "comm",	s_comm,		0	},
  { "data",	s_data,		0	},
  { "double",	float_cons,	'd'	},
  { "eject",	s_eject,	0	},	/* Formfeed listing */
  { "else",	s_else,		0	},
  { "end",	s_end,		0	},
  { "endif",	s_endif,	0	},
  { "equ",	s_set,		0	},
  { "extended",	float_cons,	't'	},
  { "extern",	s_ignore,	0	},	/* We treat all undef as ext */
  { "file",	s_file,		0	},
  { "fill",	s_fill,		0	},
  { "float",	float_cons,	'f'	},
  { "global",	s_globl,	0	},
  { "globl",	s_globl,	0	},
  { "hword",	cons,		2	},
  { "if",	s_if,		0	},
  { "ifdef",	s_ifdef,	0	},
  { "ifndef",	s_ifdef,	1	},
  { "ifnotdef",	s_ifdef,	1	},
  { "include",	s_include,	0	},
  { "int",	cons,		4	},
  { "lcomm",	s_lcomm,	0	},
  { "lflags",	s_ignore,	0	},	/* Listing flags */
  { "list",	s_list,		1	},	/* Turn listing on */
  { "lomem",	s_lomem,	0       },
  { "long",	cons,		4	},
  { "lsym",	s_set,		0	},
  { "nolist",	s_list,		0	},	/* Turn listing off */
#ifdef ALLOW_BIG_CONS
  { "octa",	big_cons,	16	},
  { "quad",	big_cons,	8	},
#endif
  { "org",	s_org,		0	},
  { "sbttl",	s_ignore,	0	},	/* Subtitle of listing */
  { "set",	s_set,		0	},
  { "short",	cons,		2	},
  { "single",	float_cons,	'f'	},
  { "space",	s_space,	0	},
  { "text",	s_text,		0	},
  { "title",	s_title,	0	},	/* Listing title */
  { "word",	cons,		2	},
  { NULL }
};


static void pobegin() {
	char *	errtxt;		/* error text */
	const pseudo_typeS * pop;

	po_hash = hash_new();

	/* Do the target-specific pseudo ops. */
	for (pop=md_pseudo_table; pop->poc_name; pop++) {
		errtxt = hash_insert (po_hash, pop->poc_name, (char *)pop);
		if (errtxt && *errtxt) {
			as_fatal("error constructing md pseudo-op table");
		} /* on error */
	} /* for each op */

	/* Now object specific.  Skip any that were in the target table. */
	for (pop=obj_pseudo_table; pop->poc_name; pop++) {
		errtxt = hash_insert (po_hash, pop->poc_name, (char *)pop);
		if (errtxt && *errtxt) {
			if (!strcmp (errtxt, "exists")) {
				continue;	/* OK if target table overrides. */
			} else {
				as_fatal("error constructing obj pseudo-op table");
			} /* if overridden */
		} /* on error */
	} /* for each op */

	/* Now portable ones.  Skip any that we've seen already. */
	for (pop=potable; pop->poc_name; pop++) {
		errtxt = hash_insert (po_hash, pop->poc_name, (char *)pop);
		if (errtxt && *errtxt) {
			if (!strcmp (errtxt, "exists")) {
				continue;	/* OK if target table overrides. */
			} else {
				as_fatal("error constructing obj pseudo-op table");
			} /* if overridden */
		} /* on error */
	} /* for each op */

	return;
} /* pobegin() */


#define HANDLE_CONDITIONAL_ASSEMBLY()	\
  if (ignore_input ())					\
    {							\
      while (! is_end_of_line[*input_line_pointer++])	\
	if (input_line_pointer == buffer_limit)		\
	  break;					\
      continue;						\
    }

/*	read_a_source_file()
 *
 * We read the file, putting things into a web that
 * represents what we have been reading.
 *
 */
void read_a_source_file(name)
char *name;
{
	register char	c;
	register char 	*s;	/* string of symbol, '\0' appended */
	register int 	temp;
	pseudo_typeS	*pop;

	buffer = input_scrub_new_file(name);

	/* Now we can set the output file name based on the input name */
	output_file_setname();
	
	while ( buffer_limit = input_scrub_next_buffer(&input_line_pointer) ) 
	{ 	/* We have another line to parse. */
		know(buffer_limit[-1] == '\n'); /* Must have a sentinel. */
		while (input_line_pointer < buffer_limit) 
		{	
			/* We have more of this buffer to parse.
			 * We now have input_line_pointer->1st char of next line.
			 * If input_line_pointer [-1] == '\n' then we just
			 * scanned another line: so bump line counters.
			 */
			if (input_line_pointer[-1] == '\n') 
			{
				bump_line_counters();
				if ( flagseen['L'] )
					if ( listing_now )
					{
						write_listing_error_buf();
						write_listing_pass1_source_line(NULL);
					}
					else 
						clear_listing_pass1_source_line();
			}

			/*
			 * We are at the begining of a line, or similar place.
			 * We expect a well-formed assembler statement.
			 * A "symbol-name:" is a statement.
			 *
			 * Depending on what compiler is used, the order of these tests
			 * may vary to catch most common case 1st.
			 * Each test is independent of all other tests at the (top) level.
			 * PLEASE make a compiler that doesn't use this assembler.
			 * It is crufty to waste a compiler's time encoding things for this
			 * assembler, which then wastes more time decoding it.
			 * (And communicating via (linear) files is silly!
			 * If you must pass stuff, please pass a tree!)
			 */
			if ((c = *input_line_pointer++) == '\t' || c == ' ' || c=='\f') 
			{
				c = *input_line_pointer++;
			}

			if (is_name_beginner(c)) 
			{	/* want user-defined label or pseudo/opcode */
				HANDLE_CONDITIONAL_ASSEMBLY();
				
				s = --input_line_pointer;
				c = get_symbol_end(); /* name's delimiter */
				/*
				 * C is character after symbol.
				 * That character's place in the input line is now '\0'.
				 * S points to the beginning of the symbol.
				 *   [In case of pseudo-op, s->'.'.]
				 * Input_line_pointer->'\0' where c was.
				 */
				if (c == ':') 
				{
					colon(s);	/* user-defined label */
					* input_line_pointer ++ = ':'; /* Put ':' back for error messages' sake. */
					/* Input_line_pointer->after ':'. */
					SKIP_WHITESPACE();
				} 
				else if (c == '=' || input_line_pointer[1] == '=') 
				{ 	/* JF deal with FOO=BAR */
					equals(s);
					demand_empty_rest_of_line();
				} 
				else 
				{	/* expect pseudo-op or machine instruction */
					if (*s == '.') 
					{
						/*
						 * PSEUDO - OP.
						 *
						 * WARNING: c has next char, which may be end-of-line.
						 * We lookup the pseudo-op table with s+1 because we
						 * already know that the pseudo-op begins with a '.'.
						 */
						
						pop = (pseudo_typeS *) hash_find(po_hash, s+1);
						
						if ( pop ) 
						{
							/* The following skip of whitespace is compulsory.
							 * A well shaped space is sometimes all that 
							 * separates keyword from operands. 
							 */
							*input_line_pointer = c;
							if (c == ' ' || c == '\t') 
								input_line_pointer++;

							/* Call the handler for this pseudo-op. */
							(*pop->poc_handler)(pop->poc_val);
						}
						else
						{
							as_bad("Unknown pseudo-op: %s",s);
							*input_line_pointer = c;
							s_ignore(0);
						}
					} /* if (pseudo-op) */
					else 
					{	/* machine instruction
						 * WARNING: c has char, which may be end-of-line. 
						 * Also: input_line_pointer->`\0` where c was.
						 */
						* input_line_pointer = c;
						while (!is_end_of_line[*input_line_pointer]) 
							input_line_pointer++;

						c = *input_line_pointer;
						*input_line_pointer = '\0';
						if (SEG_IS_BSS(curr_seg))
							as_bad("attempt to assemble an instruction into bss-type section.");
						else
							md_assemble(s); /* Assemble 1 instruction. */
						*input_line_pointer++ = c;
					}
				} /* if c==':' */
				continue;
			} /* if (is_name_beginner(c)) */
			
			
			if (is_end_of_line [c]) 
				continue;
			
			if (isdigit(c)) 
			{ 	/* numeric label  ("4:") */
				HANDLE_CONDITIONAL_ASSEMBLY ();
				
				temp = c - '0';
#ifdef LOCAL_LABELS_DOLLAR
				if (*input_line_pointer=='$')
					input_line_pointer++;
#endif
				if (* input_line_pointer ++ == ':')
				{
					local_colon (temp);
				}
				else
				{
					as_bad("Spurious digit %d.", temp);
					input_line_pointer -- ;
					ignore_rest_of_line();
				}
				continue;
			} /* local label  ("4:") */

			HANDLE_CONDITIONAL_ASSEMBLY();
			
			input_line_pointer--;
			ignore_rest_of_line();
		}	/* while (more input in buffer) */
	} /* while (more buffers to scan) */

	/* For the listing: flush the last error message, if any */
	if ( listing_now )
		write_listing_error_buf();

	input_scrub_close();	/* Close the input file */

} /* read_a_source_file() */


void s_abort() {
	as_fatal("Assembly aborted.");
} /* s_abort() */


#if 0

Not reachable on Intel 80960 assemblers.  (PR Mon Jan  9 10:26:35 PST 1995)

/* For machines where ".align 4" means align to a 4 byte boundary. */
void s_align_bytes(arg)
int arg;
{
    register unsigned int temp;
    register long temp_fill;
    unsigned int i = 0;
    unsigned long max_alignment = 1 << (MAX_ALIGN_POWER_OF_2 - 1);

    if (is_end_of_line[*input_line_pointer])
	temp = arg;		/* Default value from pseudo-op table */
    else
	temp = get_absolute_expression ();

    if (temp > max_alignment) 
    {
	as_bad("Alignment too large: %d is the maximum.", temp = max_alignment);
    }

    /*
     * For the sparc, `.align (1<<n)' actually means `.align n'
     * so we have to convert it.
     */
    if (temp != 0) {
	for (i = 0; (temp & 1) == 0; temp >>= 1, ++i)
	    ;
    }
    if (temp != 1)
	as_bad("Alignment not a power of 2");

    temp = i;
    if (*input_line_pointer == ',') {
	input_line_pointer ++;
	temp_fill = get_absolute_expression ();
    } else {
	temp_fill = 0;
    }
    /* Only make a frag if we HAVE to. . . */
    if (temp && ! need_pass_2)
	frag_align(temp, (int)temp_fill);

    demand_empty_rest_of_line();
} /* s_align_bytes() */
#endif

/* For machines where ".align 4" means align to 2**4 boundary. */
void s_align_ptwo() 
{
	register int 	temp;
	register long 	temp_fill;
	int		max_alignment = MAX_ALIGN_POWER_OF_2;

	temp = get_absolute_expression ();
	if (temp > max_alignment)
		as_bad("Alignment too large: %d is the maximum.", temp = max_alignment);
	else if (temp < 0) 
	{
		as_bad("Negative alignment not allowed.");
		temp = 0;
	}
	
	if (*input_line_pointer == ',') 
	{
		input_line_pointer ++;
		temp_fill = get_absolute_expression ();
	} else
		temp_fill = 0;

	/* Any .align higher than about 20 is getting pretty ridiculous */
	if ( temp >= 20 && temp <= max_alignment )
	{
		as_warn ( "Alignment of %d is VERY large.\n\
Your output file will contain %u %d's.", temp, 1 << temp, temp_fill );
	}

	/* Only make a frag if we HAVE to. . . */
	if (temp && ! need_pass_2)
		frag_align (temp, (int)temp_fill);

	record_alignment(curr_seg, temp);

	demand_empty_rest_of_line();
} /* s_align_ptwo() */


/* Recognize the .lomem directive.
 * This directive attaches a flag to a symbol, even if the symbol
 * is undefined.  Same for both coff and elf, so it is here.  For bout,
 * there is a stub called obj_bout_lomem() that prints a warning message.
 */
void s_lomem()
{
    symbolS *symP;
    char    *sym_name;
    char    ch;
    
    do 
    {
	sym_name = input_line_pointer;
	ch = get_symbol_end();
	
	if ( *sym_name )
	{
	    symP = symbol_find_or_make (sym_name);
	    SF_SET_LOMEM(symP);
	}
	else
	{	
	    as_bad("Syntax error: expected symbol name after .lomem");
	}
	
	*input_line_pointer = ch;
	if ( ch == ',' )
	{
	    ++input_line_pointer;
	    if ( *input_line_pointer == '\n' )
		break;
	}
    } while ( ch == ',' );
    demand_empty_rest_of_line();
}

	
/* Align and fill in one directive.  Differs from .align in that 
 * you specify a pattern with which to fill the gap that is created
 * when the alignment is done.
 * Assumes that .align n on your machine means align to 2**n boundary.
 */
void s_alignfill() 
{
	register int 	alignment;
	char		*fill_pattern;
	int		pattern_len;
	int		max_alignment = MAX_ALIGN_POWER_OF_2;

	alignment = get_absolute_expression ();
	if (alignment > max_alignment)
		as_bad("Alignment too large: %d is the maximum.", alignment = max_alignment);
	else if (alignment < 0) 
	{
		as_bad("Negative alignment not allowed.");
		alignment = 0;
	}
	
	if (*input_line_pointer != ',') 
	{
		as_bad("Expected a fill pattern argument after alignment argument");
		ignore_rest_of_line();
		return;
	}

	input_line_pointer ++;
	fill_pattern = demand_processed_string ( &pattern_len );

	/* Any .align higher than about 20 is getting pretty ridiculous */
	if ( alignment >= 20 && alignment <= max_alignment )
	{
		as_warn ( "Alignment of %d is VERY large.\n\
Your output file will contain %u fill bytes.", alignment, 1 << alignment );
	}

	/* Only make a frag if we HAVE to. . . */
	if (alignment && ! need_pass_2)
		frag_alignfill ( alignment, fill_pattern, pattern_len );

	record_alignment(curr_seg, alignment);

	demand_empty_rest_of_line();
} /* s_alignfill() */

static struct comm_sym_chain_list_node {
    symbolS *comm_sym;
    struct comm_sym_chain_list_node *next_comm_sym;
} *root_of_comm_sym_chain,**bottom_of_comm_sym_chain = &root_of_comm_sym_chain;

symbolS *top_of_comm_sym_chain()
{
    struct comm_sym_chain_list_node *p = root_of_comm_sym_chain;

    if (!p)
	    return (symbolS *) 0;
    root_of_comm_sym_chain = p->next_comm_sym;
    return p->comm_sym;
}

void add_to_comm_sym_chain(sp)
    symbolS *sp;
{
    *bottom_of_comm_sym_chain = (struct comm_sym_chain_list_node *)
	    xmalloc(sizeof(struct comm_sym_chain_list_node));
    (*bottom_of_comm_sym_chain)->comm_sym = sp;
    (*bottom_of_comm_sym_chain)->next_comm_sym = NULL;
    bottom_of_comm_sym_chain = &((*bottom_of_comm_sym_chain)->next_comm_sym);
}

void s_comm() {
	register char *name;
	register char c;
	register char *p;
	register int arg1;
	register int arg2 = -1;	/* Used for Elf (only) */
	register symbolS *	symbolP;

	name = input_line_pointer;
	c = get_symbol_end();
	/* just after name is now '\0' */
	p = input_line_pointer;
	*p = c;
	SKIP_WHITESPACE();
	if (*input_line_pointer != ',') {
		as_bad("Expected comma after symbol name");
		ignore_rest_of_line();
		return;
	}
	input_line_pointer ++; /* skip ',' */
	if ((arg1 = get_absolute_expression()) < 0) {
		as_warn(".comm length < 0, ignored", arg1);
		ignore_rest_of_line();
		return;
	}
	*p = 0;
	symbolP = symbol_find_or_make(name);
	*p = c;
	if (S_IS_DEFINED(symbolP)) {
		as_bad("Symbol is already defined: %s", S_GET_PRTABLE_NAME(symbolP));
		ignore_rest_of_line();
		return;
	}
	if (S_GET_VALUE(symbolP)) {
		if (S_GET_VALUE(symbolP) != arg1)
			as_bad("Length of .comm \"%s\" is already %d", 
				S_GET_PRTABLE_NAME(symbolP),
				S_GET_VALUE(symbolP));
	} 
	else 
#if defined(OBJ_ELF)
	{
	    /* In elf, .comm has an OPTIONAL alignment argument:

	       .comm <symname>,<size> [ ,<alignment> ]
	     */
	    /* is there a second argument ? */
	    if (*input_line_pointer == ',') {
		unsigned long max_alignment = 1 << (MAX_ALIGN_POWER_OF_2 - 1);

		input_line_pointer++;  /* skip past the ',' */
		if ((arg2 = get_absolute_expression()) < 0) {
		    as_warn(".comm alignment < 0, ignored", arg2);
		    arg2 = -1;
		}
		else if (arg2 > max_alignment) {
		    as_warn("Alignment too large for .comm directive: %d is the maximum.",max_alignment);
		    arg2 = -1;
		}
	    }
	    if (arg2 >= 0)
		    arg2 = 1 << arg2;
	    else
		    arg2 = ELF_DFLT_COMMON_ALIGNMENT;
	    S_SET_VALUE(symbolP, arg2 );
	    S_SET_SIZE(symbolP, arg1);
	    S_SET_EXTERNAL(symbolP);
	}
#else
	{
		S_SET_VALUE(symbolP, arg1);
		S_SET_EXTERNAL(symbolP);
	}
#endif
	add_to_comm_sym_chain(symbolP);
	know(symbolP->sy_frag == &zero_address_frag);
	demand_empty_rest_of_line();
} /* s_comm() */

/* The ignore_absolute_expression call is a vestige from sub-segment days.
 * We don't want to force users to change every ".data 2" in their source file
 * to ".data".  But we should warn them that the "2" doesn't do anything.
 */
void
s_data()
{
	if ( ignore_absolute_expression () )
		as_warn ("Argument to .data ignored.  Try .section instead.");

	frag_wane (curr_frag);
	frag_finish (0);
	seg_change (DEFAULT_DATA_SEGMENT);
	if ( listing_now ) listing_info ( info, 0, NULL, curr_frag, segment_change );
	demand_empty_rest_of_line();
}

void s_file() 
{
	register char 	*s;
	int 		length;
	static char	dot_file_seen = 0;

	/* Some assemblers tolerate immediately following '"' */
	if ( s = demand_string(&length) ) 
	{
		/* As of CTOOLS 4.0 we no longer initialize a logical file name.
		 * Formerly:
		 * new_logical_line(s, -1); 
		 */
		demand_empty_rest_of_line();

		/* Only one .file symbol is allowed per object module.
		 * Silently ignore further .file directives.
		 */
		if ( dot_file_seen )
			return;
		else
			dot_file_seen = 1;
		if ( length == 0 )
			/* Treat this just as if no .file had been found. */
			return;
#ifdef OBJ_COFF
		/* For coff, call the handler that actually does the work. */
		c_dot_file_symbol(s);
#endif
#ifdef OBJ_ELF
		/* For elf, call the handler that actually does the work. */
		elf_dot_file_symbol(s);
#endif
		/* For bout, just return without doing anything. */
	}

} /* s_file() */

void s_fill() 
{
	long temp_repeat;
	long temp_size;
	register long temp_fill;
	char *p;

	if (get_absolute_expression_and_terminator(& temp_repeat) != ',') {
		input_line_pointer --; /* Backup over what was not a ','. */
		as_bad("Expect comma after rep-size in .fill:");
		ignore_rest_of_line();
		return;
	}
	if (get_absolute_expression_and_terminator(& temp_size) != ',') {
		  input_line_pointer --; /* Backup over what was not a ','. */
		  as_bad("Expected comma after size in .fill");
		  ignore_rest_of_line();
		  return;
	}
	/*
	 * This is to be compatible with BSD 4.2 AS, not for any rational reason.
	 */
#define BSD_FILL_SIZE_CROCK_8 (8)
	if (temp_size > BSD_FILL_SIZE_CROCK_8) 
	{
		as_bad(".fill size must be in the range 1 - %d.", BSD_FILL_SIZE_CROCK_8);
		/* Set it to 8 just to keep going */
		temp_size = BSD_FILL_SIZE_CROCK_8 ;
	} 
	else if (temp_size < 0) 
	{
		as_warn(".fill size negative. .fill ignored.");
		temp_size = 0;
	} 
	if (temp_repeat < 0) 
	{
		as_warn(".fill repeat factor negative. .fill ignored");
		temp_size = 0;
	}
	temp_fill = get_absolute_expression ();
	if (temp_size && !need_pass_2) 
	{
		p = frag_var(rs_fill, (int)temp_size, (int)temp_size, (relax_substateT)0, (symbolS *)0, temp_repeat, (char *)0);
		bzero (p, (int)temp_size);
/*
 * The magic number BSD_FILL_SIZE_CROCK_4 is from BSD 4.2 VAX flavoured AS.
 * The following bizzare behaviour is to be compatible with above.
 * I guess they tried to take up to 8 bytes from a 4-byte expression
 * and they forgot to sign extend. Un*x Sux.
 */
#define BSD_FILL_SIZE_CROCK_4 (4)
		if (SEG_IS_BSS(curr_seg))
			if (temp_fill)
				as_bad(".fill must have 0 for fill_expr for bss-type sections.");
		md_number_to_chars (p, temp_fill, temp_size > BSD_FILL_SIZE_CROCK_4 ?
				    BSD_FILL_SIZE_CROCK_4 : temp_size, SEG_IS_BIG_ENDIAN(curr_seg));
		if ( listing_now ) listing_info ( fill, temp_size, p, curr_frag->fr_prev, 0 );
	}
	demand_empty_rest_of_line();
}

void s_globl() {
	register char *name;
	register int c;
	register symbolS *	symbolP;

	do {
		name = input_line_pointer;
		c = get_symbol_end();
		if ( strlen(name) )
		{
		    int sclass;
		    symbolP = symbol_find_or_make(name);
#ifdef	OBJ_COFF
		    switch ( sclass = S_GET_STORAGE_CLASS(symbolP) )
		    {
		    case C_NULL:
			S_SET_EXTERNAL(symbolP);
			break;
		    case C_EXT:
		    case C_SCALL:
		    case C_LEAFEXT:
			/* Do nothing; these already have external linkage. */
			break;
		    case C_LEAFSTAT:
			/* Change to external linkage; this gets a warning. */
			as_warn ("changing storage class %d (C_LEAFSTAT) to %d (C_LEAFEXT) for symbol: %s", C_LEAFSTAT, C_LEAFEXT, name);
			S_SET_STORAGE_CLASS(symbolP, C_LEAFEXT);
			break;
		    default:
			/* All others get a generic warning */
			as_warn ("changing storage class %d to %d (C_EXT) for symbol: %s", sclass, C_EXT, name);
			S_SET_EXTERNAL(symbolP);
			break;
		    }
#else
		    /* b.out macro OR's in external-ness */
		    S_SET_EXTERNAL(symbolP);
#endif
		}
		else
		{
		    as_bad("Syntax error: expected symbol name after .globl");
		}
		* input_line_pointer = c;
		SKIP_WHITESPACE();
		if (c==',') {
			input_line_pointer++;
			SKIP_WHITESPACE();
			if (*input_line_pointer=='\n')
				c='\n';
		}
	} while(c==',');
	demand_empty_rest_of_line();
} /* s_globl() */

void s_lcomm(needs_align)
int needs_align;	/* 1 if this was a ".bss" directive, which may require
			 *	a 3rd argument (alignment).
			 * 0 if it was an ".lcomm" (2 args only)
			 */
{
	register char *name;
	register char c;
	register char *p;
	register int temp;
	register symbolS *	symbolP;
	const int max_alignment = MAX_ALIGN_POWER_OF_2;
	int align;

	name = input_line_pointer;
	c = get_symbol_end();
	p = input_line_pointer;
	*p = c;
	SKIP_WHITESPACE();
	if (* input_line_pointer != ',') {
		as_bad("Expected comma after name");
		ignore_rest_of_line();
		return;
	}
	input_line_pointer ++;

	if (*input_line_pointer == '\n') {
		as_bad("Missing size expression");
		return;
	}

	if ((temp = get_absolute_expression ()) < 0) {
		as_warn(".bss length < 0, ignored.", temp);
		ignore_rest_of_line();
		return;
	}

	if (needs_align) {
		align = 0;
		SKIP_WHITESPACE();
		if (*input_line_pointer != ',') {
			as_bad("Expected comma after size");
			ignore_rest_of_line();
			return;
		}
		input_line_pointer++;
		SKIP_WHITESPACE();
		if (*input_line_pointer == '\n') {
			as_bad("Missing alignment");
			return;
		}
		align = get_absolute_expression ();
		if (align > max_alignment)
		{
			align = max_alignment;
			as_bad("Alignment too large: %d is the maximum.", align);
		} else if (align < 0) 
		{
			align = 0;
			as_bad("Negative alignment not allowed.");
		}
		/* Any alignment higher than about 20 is getting pretty ridiculous */
		if ( align >= 20 && align <= max_alignment )
		{
			as_warn ( "Alignment of %d is VERY large.\n\
Your load module will contain %u 0's.", align, 1 << align );
		}

		record_alignment(DEFAULT_BSS_SEGMENT, align);
	} /* if needs align */

	*p = 0;
	symbolP = symbol_find_or_make(name);
	*p = c;
	if (S_IS_LOCAL(symbolP))
		SF_SET_LOCAL(symbolP);
	if (
#if defined(OBJ_BOUT)
	    S_GET_OTHER(symbolP) == 0 &&
	    S_GET_DESC(symbolP)  == 0 &&
#endif
	    (((S_GET_SEGTYPE(symbolP) == SEG_BSS) &&
	      (S_GET_VALUE(symbolP) == SEG_GET_SIZE(DEFAULT_BSS_SEGMENT))) ||
	     (!S_IS_DEFINED(symbolP) && S_GET_VALUE(symbolP) == 0)))
	{
		if (needs_align)
		{
		    unsigned long local_bss_counter = SEG_GET_SIZE(DEFAULT_BSS_SEGMENT);
		    /* Align */
		    align = ~ ((~0) << align);	/* Convert to a mask */
		    local_bss_counter =
			    (local_bss_counter + align) & (~align);
		    SEG_SET_SIZE(DEFAULT_BSS_SEGMENT,local_bss_counter);
		}

		S_SET_VALUE(symbolP,SEG_GET_SIZE(DEFAULT_BSS_SEGMENT));
		S_SET_SEGMENT(symbolP, DEFAULT_BSS_SEGMENT);
#ifdef OBJ_COFF
		/* The symbol may already have been created with a preceding
		 * ".globl" directive -- be careful not to step on storage
		 * class in that case.  Otherwise, set it to static.
		 */
		if (S_GET_STORAGE_CLASS(symbolP) != C_EXT){
			S_SET_STORAGE_CLASS(symbolP, C_STAT);
		}
#endif /* OBJ_COFF */
		symbolP->sy_frag  = SEG_GET_FRAG_ROOT(DEFAULT_BSS_SEGMENT);
		SEG_SET_SIZE(DEFAULT_BSS_SEGMENT,SEG_GET_SIZE(DEFAULT_BSS_SEGMENT) + temp);
	}
	else 
	{
		as_bad("Symbol is already defined: %s", S_GET_PRTABLE_NAME(symbolP));
	}

	/* For listing: We want to print this symbol's address.  Store the 
	 * symbol pointer into the frag pointer field of this info struct.
	 * We'll get the symbol's value later when we know what it is.
	 */
	if ( listing_now ) listing_info ( info, 0, NULL, (fragS *) symbolP, bss );

	demand_empty_rest_of_line();

	return;
} /* s_lcomm() */

#if 0

This code is dead in the Intel 80960 assemblers.  (PR)

void
s_long()
{
	cons(4);
}

void
s_int()
{
	cons(4);
}
#endif

void s_org() 
{
	register segT segtype;
	expressionS exp;
	register long temp_fill;
	register char *p;
/*
 * Don't believe the documentation of BSD 4.2 AS.
 * There is no such thing as a sub-segment-relative origin.
 * Any absolute origin is given a warning, then assumed to be segment-relative.
 * Any segmented origin expression ("foo+42") had better be in the right
 * segment or the .org is ignored.
 *
 * BSD 4.2 AS warns if you try to .org backwards. We cannot because we
 * never know sub-segment sizes when we are reading code.
 * BSD will crash trying to emit -ve numbers of filler bytes in certain
 * .orgs. We don't crash, but see as-write for that code.
 */
/*
 * Don't make frag if need_pass_2==1.
 */
	segtype = get_known_segmented_expression(&exp);
	if (*input_line_pointer == ',') {
		input_line_pointer ++;
		temp_fill = get_absolute_expression ();
	} else
		temp_fill = 0;
	if (! need_pass_2) {
		if ( exp.X_segment != curr_seg && segtype != SEG_ABSOLUTE )
		    as_bad("Invalid segment \"%s\"", segment_name(exp.X_segment));
		p = frag_var (rs_org, 1, 1, (relax_substateT)0, exp . X_add_symbol,
			      exp . X_add_number, (char *)0);
		* p = temp_fill;
		/* For the listing: this will be a fill-type frag eventually.
		 * We can get the details we need from the frag later.
		 */
		if ( listing_now ) listing_info ( fill, 1, p, curr_frag->fr_prev, 0 );
	} /* if (ok to make frag) */
	demand_empty_rest_of_line();
} /* s_org() */

void s_set() 
{
	segT segtype;
	expressionS exp;
	register char *name;
	register char delim;
	register char *end_name;
	register symbolS *symbolP;

	name = input_line_pointer;
	delim = get_symbol_end();
	end_name = input_line_pointer;
	*end_name = delim;
	SKIP_WHITESPACE();

	if (*input_line_pointer != ',') 
	{
		*end_name = 0;
		as_bad("Expected comma after name \"%s\"", name);
		*end_name = delim;
		ignore_rest_of_line();
		return;
	}

	if ( ! is_name_beginner(*name) )
		as_bad("Expected an identifier for .set");

	input_line_pointer ++;
	*end_name = 0;

	if (name[0]=='.' && name[1]=='\0') 
	{
		/* Turn '. = mumble' into a .org mumble */
		register char *ptr;

		segtype = get_known_segmented_expression(& exp);

		if (!need_pass_2) {
			if (exp.X_segment != curr_seg && segtype != SEG_ABSOLUTE )
			    as_bad("Invalid segment \"%s\"", segment_name(exp.X_segment));
			ptr = frag_var(rs_org, 1, 1, (relax_substateT)0, exp.X_add_symbol,
				       exp.X_add_number, (char *)0);
			*ptr= 0;

			/* For the listing: this will be a fill-type frag eventually.
			 * We can get the details we need from the frag later.
			 */
			if ( listing_now ) listing_info ( fill, 1, ptr, curr_frag->fr_prev, 0 );
		} /* if (ok to make frag) */
		
		*end_name = delim;
		return;
	}

	if ((symbolP = symbol_find(name)) == NULL
	    && (symbolP = md_undefined_symbol(name)) == NULL) 
	    {
		symbolP = symbol_new(name,
				     MYTHICAL_UNKNOWN_SEGMENT,
				     0,
				     &zero_address_frag);
#ifdef OBJ_COFF
		/* "set" symbols are local unless otherwise specified. */
		SF_SET_LOCAL(symbolP);
#endif /* OBJ_COFF */
		if (S_IS_LOCAL(symbolP))
			SF_SET_LOCAL(symbolP);
	}

	symbol_table_insert(symbolP);

	*end_name = delim;
	pseudo_set(symbolP);
	demand_empty_rest_of_line();
} /* s_set() */

void s_space() {
	long temp_repeat;
	register long temp_fill;
	register char *p;

	/* Just like .fill, but temp_size = 1 */
	if (get_absolute_expression_and_terminator(& temp_repeat) == ',') {
		temp_fill = get_absolute_expression ();
	} else {
		input_line_pointer --; /* Backup over what was not a ','. */
		temp_fill = 0;
	}
	if (temp_fill && SEG_IS_BSS(curr_seg))
		as_bad("attempt to specify non-zero fill char for .space directive while in a .bss section.");

	if (temp_repeat <= 0) {
		as_warn("Repeat < 0, .space ignored");
		ignore_rest_of_line();
		return;
	}
	if (! need_pass_2) {
		p = frag_var (rs_fill, 1, 1, (relax_substateT)0, (symbolS *)0,
 temp_repeat, (char *)0);
		* p = temp_fill;
		if ( listing_now ) listing_info ( fill, 1, p, curr_frag->fr_prev, 0 );
	}
	demand_empty_rest_of_line();
} /* s_space() */

/* The ignore_absolute_expression call is a vestige from sub-segment days.
 * We don't want to force users to change every ".text 2" in their source file
 * to ".text".  But we should warn them that the "2" doesn't do anything.
 */
void
s_text()
{
	if ( ignore_absolute_expression () )
		as_warn ("Argument to .text ignored.  Try .section instead.");

	frag_wane (curr_frag);
	frag_finish (0);
	seg_change (DEFAULT_TEXT_SEGMENT);
	if ( listing_now ) listing_info ( info, 0, NULL, curr_frag, segment_change );
	demand_empty_rest_of_line();
} /* s_text() */

 
void demand_empty_rest_of_line() {
	SKIP_WHITESPACE();
	if (is_end_of_line [*input_line_pointer]) {
		input_line_pointer++;
	} else {
		ignore_rest_of_line();
	}
	/* Return having already swallowed end-of-line. */
} /* Return pointing just after end-of-line. */

/* Check that input line is at end of line. */
void
ignore_rest_of_line()	
{
	if ( ! is_end_of_line [* input_line_pointer] )
	{
		if ( isprint(*input_line_pointer) )
			as_bad("Syntax error. Unexpected character '%c'.",
			       *input_line_pointer);
		else
			as_bad("Syntax error. Unexpected character 0x%x.",
			       *input_line_pointer);
		while (input_line_pointer < buffer_limit
		       && ! is_end_of_line [* input_line_pointer])
		{
			input_line_pointer ++;
		}
	}
	input_line_pointer ++;	/* Return pointing just after end-of-line. */
	know(is_end_of_line [input_line_pointer [-1]]);
}

/*
 *			pseudo_set()
 *
 * In:	Pointer to a symbol.
 *	Input_line_pointer->expression.
 *
 * Out:	Input_line_pointer->just after any whitespace after expression.
 *	Tried to set symbol to value of expression.
 *	Will change symbols type, value, and frag;
 *	May set need_pass_2 == 1.
 */
void
pseudo_set (symbolP)
     symbolS *	symbolP;
{
	expressionS	exp;
	register int    segment;
	register segT	segtype;
	char 		*sav; 	/* for delayed expression evaluation. */

#if defined(OBJ_BOUT)
	int ext;
#endif
	
	know(symbolP);		/* NULL pointer is logic error. */
#if defined(OBJ_BOUT)
	ext=S_IS_EXTERNAL(symbolP);
#endif
	
	sav = input_line_pointer;
	segtype = expression(& exp);
	segment = exp.X_segment;
	
	switch (segtype)
	{
	case SEG_BIG:
		as_bad("%s number is invalid for .set",
		       exp . X_add_number > 0 ? "Bignum" : "Floating-Point");
		S_SET_SEGMENT(symbolP, MYTHICAL_ABSOLUTE_SEGMENT);
		
#if defined(OBJ_BOUT)
		ext ? S_SET_EXTERNAL(symbolP) :
			S_CLEAR_EXTERNAL(symbolP);
#endif
		
		S_SET_VALUE(symbolP, 0);
		symbolP->sy_frag = & zero_address_frag;
		break;
		
	case SEG_ABSENT:
		as_bad(".set: Missing expression");
		S_SET_SEGMENT(symbolP, MYTHICAL_ABSOLUTE_SEGMENT);
#if defined(OBJ_BOUT)
		ext ? S_SET_EXTERNAL(symbolP) :
			S_CLEAR_EXTERNAL(symbolP);
#endif
		S_SET_VALUE(symbolP, 0);
		symbolP->sy_frag = & zero_address_frag;
		break;
		
	case SEG_DATA:
	case SEG_TEXT:
	case SEG_BSS:
		/* These are, strictly, not allowed.  But they are
		 * historically used by b.out .stabs directive. 
		 */
		S_SET_SEGMENT(symbolP, segment);
		
#if defined(OBJ_BOUT)
		if (ext) {
			S_SET_EXTERNAL(symbolP);
		} else {
			S_CLEAR_EXTERNAL(symbolP);
		}	/* if external */
#endif
		
		S_SET_VALUE(symbolP, exp.X_add_number + S_GET_VALUE(exp.X_add_symbol));
		symbolP->sy_frag = exp . X_add_symbol->sy_frag;
		break;
		
	case SEG_PASS1:
		need_pass_2 = 0;
		/* Intentional fallthrough */
	case SEG_DIFFERENCE:
	case SEG_UNKNOWN:
		/* FIXME-SOMEDAY:  These really call for a second pass.
		 * In lieu of that, set up a "delayed evaluation" for 
		 * this symbol.
		 */
		S_SET_SEGMENT(symbolP, MYTHICAL_UNKNOWN_SEGMENT);
		del_expr_append(symbolP, sav);
		break;
	       
	case SEG_ABSOLUTE:
		S_SET_SEGMENT(symbolP, MYTHICAL_ABSOLUTE_SEGMENT);
		
#if defined(OBJ_BOUT)
		ext ? S_SET_EXTERNAL(symbolP) :
			S_CLEAR_EXTERNAL(symbolP);
#endif
		
		S_SET_VALUE(symbolP, exp.X_add_number);
		symbolP->sy_frag = & zero_address_frag;
		break;
		
	default:
		as_bad ("Illegal .set expression");
		break;
	}
}

/*
 *			cons()
 *
 * CONStruct more frag of .bytes, or .words etc.
 * Should need_pass_2 be 1 then emit no frag(s).
 * This understands EXPRESSIONS, as opposed to big_cons().
 *
 * This has a split personality. We use expression() to read the
 * value. We can detect if the value won't fit in a byte or word.
 * But we can't detect if expression() discarded significant digits
 * in the case of a long. Not worth the crocks required to fix it.
 *
 */

void cons(nbytes)
register unsigned int nbytes;	/* 1=.byte, 2=.short, 4=.word */
{
	register char 	c;
	register long 	mask;   /* The bits we will actually output. */
	register segT	segtype;
	expressionS	exp;
	register long	total_bits = BITS_PER_CHAR * nbytes;
	char		*p;

	mask = ~0;
	if ( nbytes < sizeof(long) )
		mask = ~ (mask << total_bits);
	/*
	 * The following awkward logic is to parse ZERO or more expressions,
	 * comma seperated. Recall an expression includes its leading &
	 * trailing blanks. We fake a leading ',' if there is (supposed to
	 * be) a 1st expression, and keep demanding 1 expression for each ','.
	 */
	c = ',';
	while (c == ',') 
	{
		unsigned int 	bits_available = total_bits;
		char 		*hold = input_line_pointer;

		/* At least scan over the expression. */
		segtype = expression(&exp);
		
		if (*input_line_pointer == ':') 
		{
			long container = 0;

			/* Looks like a bitfield; e.g. after ".byte" as w:x,y:z, 
			 * where w and y are bitfields and x and y are values.
			 *
			 * The rules are:
			 *   1. pack bits into container in target byte order
			 *   2. a bitfield larger than the container is an error
			 *   3. if a field doesn't fit into what's left of the
			 *      container, put it in the next container.
			 *   4. Values that overflow the bitfield are silently 
			 *      truncated to fit.
			 *   5. Values must be absolute, not symbolic (a previous
			 *      .set or .equ is ok)
			 */
			
			for ( ; ; ) 
			{
				unsigned long 	width;
				long 		this_bitfield;
				
				if (*input_line_pointer != ':') 
				{
					input_line_pointer = hold - 1;
					break;
				} /* next piece is not a bitfield */
				
				if (segtype == SEG_ABSENT) 
				{
					as_warn("Missing expression. Using 0.");
					exp.X_add_number = 0;
					segtype = SEG_ABSOLUTE;
				} /* implied zero width bitfield */
				
				if (segtype != SEG_ABSOLUTE) 
				{
					*input_line_pointer = '\0';
					as_bad("Field width \"%s\" too complex for a bitfield.\n", hold);
					*input_line_pointer = ':';
					demand_empty_rest_of_line();
					return;
				} /* too complex */
				
				if ( (width = exp.X_add_number) > total_bits )
				{
					as_bad("Field width %d too big to fit into %d bits.\n",
					       width, total_bits);
					/* truncate just to keep going thru pass 1 */
					width = BITS_PER_CHAR * nbytes;
				} /* too big */
				
				if ( width > bits_available )
				{
					/* Output current container; then back up input 
					 * line pointer and reparse with a new container
					 */
					input_line_pointer = hold - 1;
					*input_line_pointer = ',';
					bits_available = total_bits;
					break;
				} /* won't fit */
				
				hold = ++input_line_pointer; /* skip ':' */
				
				if ( (segtype = expression(&exp)) != SEG_ABSOLUTE )
				{
					char cache = *input_line_pointer;
					
					*input_line_pointer = '\0';
					as_bad("Field value \"%s\" too complex for a bitfield.\n", hold);
					*input_line_pointer = cache;
					demand_empty_rest_of_line();
					return;
				} /* too complex */
				
				/* Values that are too wide get truncated here */
				this_bitfield = (width == (BITS_PER_CHAR * 4) ? 0 : (-1 << width));
				this_bitfield = ~ this_bitfield;
				this_bitfield &= exp.X_add_number;

				if ( SEG_IS_BIG_ENDIAN(curr_seg) )
					/* Pack container from left-to-right */
					container |= (this_bitfield << (bits_available - width));
				else
					/* Pack container from right-to-left */
					container |= (this_bitfield << (total_bits - bits_available));

				bits_available -= width;

				if ( is_it_end_of_statement() || *input_line_pointer != ',' ) 
				{
					break;
				} /* all the bitfields we're gonna get */
				
				hold = ++input_line_pointer;
				segtype = expression(&exp);
			} /* forever loop */
			
			/* Put bitfield result into expression format for what follows. */
			exp.X_add_number = container;
			segtype = SEG_ABSOLUTE;

		} /* if looks like a bitfield */

		/* Write the bytes out to a frag. */

		if (segtype == SEG_DIFFERENCE && exp.X_add_symbol == NULL) 
		{
			as_bad("Expression is too complex: - %s",
			       S_GET_PRTABLE_NAME(exp.X_subtract_symbol));
			segtype = SEG_ABSOLUTE;
			/* Leave exp . X_add_number alone. */
		}
		p = frag_more(nbytes);
		switch (segtype) 
		{
		case SEG_BIG:
			as_bad("%s number invalid",
			       exp . X_add_number > 0 ? "Bignum" : "Floating-Point");
			md_number_to_chars (p, (long)0, nbytes, SEG_IS_BIG_ENDIAN(curr_seg));
			if ( listing_now ) listing_info ( complete, nbytes, p, curr_frag, 0 );
			break;
		case SEG_ABSENT:
			as_warn("Missing expression. Using 0.");
			exp . X_add_number = 0;
			know(exp . X_add_symbol == NULL);
			/* fall into SEG_ABSOLUTE */
		case SEG_ABSOLUTE:
			/* Warn user if there will be a truncation */
			if ( (exp.X_add_number & mask) != exp.X_add_number )
			{
				/* Could be that the number is merely negative */
				if ( (exp.X_add_number & ~mask) != ~mask )
				{
					as_warn("Value 0x%x truncated to 0x%x.", 
						exp.X_add_number, exp.X_add_number & mask);
				}
			}
			if (exp.X_add_number && SEG_IS_BSS(curr_seg) && nbytes)
			    as_bad("Can not write non-zero constant data to bss-type sections.");
			/* Put bytes in target order. */
			md_number_to_chars (p, exp.X_add_number & mask, nbytes, SEG_IS_BIG_ENDIAN(curr_seg)); 
			if ( listing_now ) listing_info ( complete, nbytes, p, curr_frag, 0 );
			break;
		case SEG_DIFFERENCE:
		case SEG_BSS:
		case SEG_UNKNOWN:
		case SEG_TEXT:
		case SEG_DATA:
			if (SEG_IS_BSS(curr_seg))
				as_bad("attempt to generate bss-type section with relocation directive.");
			fix_new (curr_frag, p - curr_frag->fr_literal, nbytes,
				 exp . X_add_symbol, exp . X_subtract_symbol,
				 exp . X_add_number, 0, RELOC_32);
			if ( listing_now ) listing_info ( complete, nbytes, p, curr_frag, 0 );
			break;
		default:
			as_bad ("Illegal expression");
			break;
		} /* switch(segtype) */

		c = *input_line_pointer++;
		
	} /* while(c==',') */
	
	input_line_pointer--;	/* Put terminator back into stream. */
	demand_empty_rest_of_line();
	
} /* cons() */


#ifdef ALLOW_BIG_CONS
/*
 *			big_cons()
 *
 * CONStruct more frag(s) of .quads, or .octa etc.
 * Makes 0 or more new frags.
 * If need_pass_2 == 1, generate no frag.
 * This understands only bignums, not expressions. Cons() understands
 * expressions.
 *
 * Constants recognised are '0...'(octal) '0x...'(hex) '...'(decimal).
 *
 * This creates objects with struct obstack_control objs, destroying
 * any context objs held about a partially completed object. Beware!
 *
 * I think it sucks to have 2 different types of integers, with 2
 * routines to read them, store them etc.
 * It would be nicer to permit bignums in expressions and only
 * complain if the result overflowed. However, due to "efficiency"...
 */

void big_cons(nbytes)
     register int nbytes;
{
  register char c;	/* input_line_pointer->c. */
  register int radix;
  register long length;	/* Number of chars in an object. */
  register int digit;	/* Value of 1 digit. */
  register int carry;	/* For multi-precision arithmetic. */
  register int work;	/* For multi-precision arithmetic. */
  register char *	p;	/* For multi-precision arithmetic. */

  extern char hex_value[];	/* In hex_value.c. */

  /*
   * The following awkward logic is to parse ZERO or more strings,
   * comma seperated. Recall an expression includes its leading &
   * trailing blanks. We fake a leading ',' if there is (supposed to
   * be) a 1st expression, and keep demanding 1 expression for each ','.
   */
  if (is_it_end_of_statement())
    {
      c = 0;			/* Skip loop. */
    }
  else
    {
      c = ',';			/* Do loop. */
      -- input_line_pointer;
    }
  while (c == ',')
    {
      ++ input_line_pointer;
      SKIP_WHITESPACE();
      c = * input_line_pointer;
      /* C contains 1st non-blank character of what we hope is a number. */
      if (c == '0')
	{
	  c = * ++ input_line_pointer;
	  if (c == 'x' || c=='X')
	    {
	      c = * ++ input_line_pointer;
	      radix = 16;
	    }
	  else
	    {
	      radix = 8;
	    }
	}
      else
	{
	  radix = 10;
	}
      /*
       * This feature (?) is here to stop people worrying about
       * mysterious zero constants: which is what they get when
       * they completely omit digits.
       */
      if (hex_value[c] >= radix) {
	      as_bad("Missing digits. 0 assumed.");
      }
      bignum_high = bignum_low - 1; /* Start constant with 0 chars. */
      for(;   (digit = hex_value [c]) < radix;   c = * ++ input_line_pointer)
	{
	  /* Multiply existing number by radix, then add digit. */
	  carry = digit;
	  for (p=bignum_low;   p <= bignum_high;   p++)
	    {
	      work = (*p & MASK_CHAR) * radix + carry;
	      *p = work & MASK_CHAR;
	      carry = work >> BITS_PER_CHAR;
	    }
	  if (carry)
	    {
	      grow_bignum();
	      * bignum_high = carry & MASK_CHAR;
	      know((carry & ~ MASK_CHAR) == 0);
	    }
	}
      length = bignum_high - bignum_low + 1;
      if (length > nbytes)
	{
	  as_warn("Most significant bits truncated in integer constant.");
	}
      else
	{
	  register long leading_zeroes;

	  for(leading_zeroes = nbytes - length;
	      leading_zeroes;
	      leading_zeroes --)
	    {
	      grow_bignum();
	      * bignum_high = 0;
	    }
	}
      if (! need_pass_2)
	{
	  if (SEG_IS_BSS(curr_seg)) {
	      int i;

	      for (i=0;i < nbytes;i++)
		      if (bignum_low[i]) {
			  as_bad("Can not assemble non-zero big_num data into bss-type sections.");
			  break;
		      }
	  }
	  p = frag_more (nbytes);
	  bcopy (bignum_low, p, (int)nbytes);
	  if ( listing_now )
		  listing_info ( complete, nbytes, p, curr_frag, 0 );
	}
      /* C contains character after number. */
      SKIP_WHITESPACE();
      c = * input_line_pointer;
      /* C contains 1st non-blank character after number. */
    }
  demand_empty_rest_of_line();
} /* big_cons() */

#endif /* ALLOW_BIG_CONS */

 /* Extend bignum by 1 char. */
static void grow_bignum() {
  register long length;

  bignum_high ++;
  if (bignum_high >= bignum_limit)
    {
      length = bignum_limit - bignum_low;
      bignum_low = xrealloc(bignum_low, length + length);
      bignum_high = bignum_low + length;
      bignum_limit = bignum_low + length + length;
    }
} /* grow_bignum(); */
 
/*
 *			float_cons()
 *
 * CONStruct some more frag chars of .floats .ffloats etc.
 * Makes 0 or more new frags.
 * If need_pass_2 == 1, no frags are emitted.
 * This understands only floating literals, not expressions. Sorry.
 *
 * A floating constant is defined by atof_generic(), except it is preceded
 * by 0d 0f 0g or 0h. After observing the STRANGE way my BSD AS does its
 * reading, I decided to be incompatible. This always tries to give you
 * rounded bits to the precision of the pseudo-op. Former AS did premature
 * truncatation, restored noisy bits instead of trailing 0s AND gave you
 * a choice of 2 flavours of noise according to which of 2 floating-point
 * scanners you directed AS to use.
 *
 * In:	input_line_pointer->whitespace before, or '0' of flonum.
 *
 */

void	/* JF was static, but can't be if VAX.C is goning to use it */
float_cons(float_type)		/* Worker to do .float etc statements. */
				/* Clobbers input_line-pointer, checks end-of-line. */
     register int float_type;	/* 'f':.ffloat ... 'F':.float ... */
{
  register char *	p;
  register char c;
  int length;	/* Number of chars in an object. */
  register char *	err;	/* Error from scanning floating literal. */
  char temp [MAXIMUM_NUMBER_OF_CHARS_FOR_FLOAT];

  /*
   * The following awkward logic is to parse ZERO or more strings,
   * comma seperated. Recall an expression includes its leading &
   * trailing blanks. We fake a leading ',' if there is (supposed to
   * be) a 1st expression, and keep demanding 1 expression for each ','.
   */
  if (is_it_end_of_statement())
    {
      c = 0;			/* Skip loop. */
      ++ input_line_pointer;	/*->past termintor. */
    }
  else
    {
      c = ',';			/* Do loop. */
    }
  while (c == ',')
    {
      /* input_line_pointer->1st char of a flonum (we hope!). */
      SKIP_WHITESPACE();
      /* Skip any 0{letter} that may be present. Don't even check if the
       * letter is legal. Someone may invent a "z" format and this routine
       * has no use for such information. Lusers beware: you get
       * diagnostics if your input is ill-conditioned.
       */

      if (input_line_pointer[0]=='0' && isalpha(input_line_pointer[1]))
	  input_line_pointer+=2;

      err = md_atof (float_type, temp, &length);
      if (SEG_IS_BSS(curr_seg)) {
	  int i;

	  for (i=0;i < length;i++)
		  if (temp[i]) {
		      as_bad("Can not assemble non-zero floating point data into bss-type sections.");
		      break;
		  }
      }
      know(length <=  MAXIMUM_NUMBER_OF_CHARS_FOR_FLOAT);
      know(length > 0);
      if (* err)
	{
	  as_bad("Bad floating literal: %s", err);
	  ignore_rest_of_line();
	  /* Input_line_pointer->just after end-of-line. */
	  c = 0;		/* Break out of loop. */
	}
      else
	{
	  if (! need_pass_2)
	    {
	      p = frag_more (length);
	      bcopy (temp, p, length);
	      if ( listing_now ) listing_info ( complete, length, p, curr_frag, 0 );
	    }
	  SKIP_WHITESPACE();
	  c = * input_line_pointer ++;
	  /* C contains 1st non-white character after number. */
	  /* input_line_pointer->just after terminator (c). */
	}
    }
  -- input_line_pointer;		/*->terminator (is not ','). */
  demand_empty_rest_of_line();
}				/* float_cons() */
 
/*
 *			stringer()
 *
 * We read 0 or more ',' seperated, double-quoted strings.
 *
 * Caller should have checked need_pass_2 is FALSE because we don't check it.
 */
static void stringer(append_zero)		/* Worker to do .ascii etc statements. */
				/* Checks end-of-line. */
     register int append_zero;	/* 0: don't append '\0', else 1 */
{
	register unsigned int c,err_cnt = 0;
	char *string_start;
	fragS *frag_start;
	
	/*
	 * The following awkward logic is to parse ZERO or more strings,
	 * comma seperated. Recall a string expression includes spaces
	 * before the opening '\"' and spaces after the closing '\"'.
	 * We fake a leading ',' if there is (supposed to be)
	 * a 1st, expression. We keep demanding expressions for each
	 * ','.
	 */
	if (is_it_end_of_statement())
	    {
		    c = 0;			/* Skip loop. */
		    ++ input_line_pointer;	/* Compensate for end of loop. */
	    }
	else
	    {
		    c = ',';			/* Do loop. */
	    }
	if ( listing_now )
	{
		frag_start = curr_frag;
		string_start = obstack_next_free(&frags);
	}
	for (; c == ','; c = *input_line_pointer++) 
	{
		SKIP_WHITESPACE();
		if (*input_line_pointer == '\"') 
		{
			int foo = 0;
			++input_line_pointer; /*->1st char of string. */
			while (is_a_char(c = next_char_of_string())) 
			{
			        if (SEG_IS_BSS(curr_seg) && c && !err_cnt) {
				    as_bad("can not assemble .ascii data into bss-type sections.");
				    err_cnt++;
				}
				FRAG_APPEND_1_CHAR(c);
				++foo;
			}
			if (append_zero) 
				FRAG_APPEND_1_CHAR(0);
		}
		else 
			as_warn("Expected quoted string");
		SKIP_WHITESPACE();
	}
	if ( listing_now )
	{
		if ( curr_frag != frag_start )
		{
			/* Ai-yi-yi; the string spans the break between 2 frags.
			 * Do the listing_info in 2 chunks.
			 */
			
			listing_info ( complete, frag_start->fr_literal + frag_start->fr_fix - string_start, string_start, curr_frag->fr_prev, 0 );
			listing_info ( complete, obstack_next_free(&frags) - curr_frag->fr_literal, curr_frag->fr_literal, curr_frag, 0 );
		}
		else
			/* Normal case; it fits into one frag. */
			listing_info ( complete, obstack_next_free(&frags) - string_start, string_start, curr_frag, 0 );
	}
	--input_line_pointer;
	demand_empty_rest_of_line();
} /* stringer() */
 
 /* FIXME-SOMEDAY: I had trouble here on characters with the
    high bits set.  We'll probably also have trouble with
    multibyte chars, wide chars, etc.  Also be careful about
    returning values bigger than 1 byte.  xoxorich. */

unsigned int next_char_of_string() {
	register unsigned int c;
	
	c = *input_line_pointer++ & CHAR_MASK;
	switch (c) {
	case '\"':
		c = NOT_A_CHAR;
		break;
		
	case '\\':
		switch (c = *input_line_pointer++) {
		case 'b':
			c = '\b';
			break;
			
		case 'f':
			c = '\f';
			break;
			
		case 'n':
			c = '\n';
			break;
			
		case 'r':
			c = '\r';
			break;
			
		case 't':
			c = '\t';
			break;
			
		case 'v':
			c = '\013';
			break;
			
		case '\\':
		case '"':
			break;		/* As itself. */
			
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9': 
		{
			/* Working on an octal char constant, e.g. \012 == \12 */
			long 	number;
			int	count;	/* Octal char constant can be 1-3 digits */
			
			for (number = 0, count = 0; 
			     isdigit(c) && count < 3; 
			     c = *input_line_pointer++, count++)
			{
				number = number * 8 + c - '0';
			}
			c = number & 0xff;
		}
			--input_line_pointer;
			break;
			
		case '\n':
			/* To be compatible with BSD 4.2 as: give the luser a linefeed!! */
			as_warn("Unterminated string: Newline inserted.");
			c = '\n';
			break;
			
		default:
			
#ifdef ONLY_STANDARD_ESCAPES
			as_bad("Invalid character escape in string");
			c = '?';
#endif /* ONLY_STANDARD_ESCAPES */
			
			break;
		} /* switch on escaped char */
		break;
		
	default:
		break;
	} /* switch on char */
	return(c);
} /* next_char_of_string() */
 
static segT
get_segmented_expression (expP)
	register expressionS *	expP;
{
  register segT		retval;

  if ((retval = expression(expP)) == SEG_PASS1 || retval == SEG_ABSENT || retval == SEG_BIG)
    {
      as_bad("Expected address expression");
      retval = expP->X_seg = SEG_ABSOLUTE;
      expP->X_segment = MYTHICAL_ABSOLUTE_SEGMENT;
      expP->X_add_number   = 0;
      expP->X_add_symbol   = expP->X_subtract_symbol = 0;
    }
  return (retval);		/* SEG_ ABSOLUTE,UNKNOWN,DATA,TEXT,BSS */
}

static segT 
get_known_segmented_expression(expP)
	register expressionS *expP;
{
  register segT		retval;
  register char *	name1;
  register char *	name2;

  if ((retval = get_segmented_expression (expP)) == SEG_UNKNOWN)
    {
      name1 = expP->X_add_symbol ? S_GET_NAME(expP->X_add_symbol) : "";
      name2 = expP->X_subtract_symbol ?
	      S_GET_NAME(expP->X_subtract_symbol) :
		  "";
      if (name1 && name2)
	{
	  as_warn("Symbols \"%s\" and \"%s\" are undefined",
		  name1, name2);
	}
      else
	{
	  as_warn("Symbol \"%s\" is undefined",
		  name1 ? name1 : name2);
	}
      retval = expP->X_seg = SEG_ABSOLUTE;
      expP->X_segment = MYTHICAL_ABSOLUTE_SEGMENT;
      expP->X_add_number   = 0;
      expP->X_add_symbol   = expP->X_subtract_symbol = NULL;
    }
 know(retval == SEG_ABSOLUTE || retval == SEG_DATA || retval == SEG_TEXT || retval == SEG_BSS || retval == SEG_DIFFERENCE);
  return (retval);
}				/* get_known_segmented_expression() */


long
get_absolute_expression ()
{
  expressionS	exp;
  register segT s;

  if ((s = expression(& exp)) != SEG_ABSOLUTE)
    {
      if (s != SEG_ABSENT)
	{
	  as_bad("Absolute expression required.");
	}
      exp . X_add_number = 0;
    }
  return (exp . X_add_number);
}

/* Similar to get_absolute_expression, only the value of the expression
 * is immaterial; we just want to use up the input characters.  Return 1 if
 * there was an expression present, otherwise return 0.
 */
int
ignore_absolute_expression ()
{
  expressionS	exp;

  return ( expression(& exp) != SEG_ABSENT );
}


char /* return terminator */
get_absolute_expression_and_terminator(val_pointer)
     long *		val_pointer; /* return value of expression */
{
  * val_pointer = get_absolute_expression ();
  return (* input_line_pointer ++);
}
 
/*
 *			demand_string()
 *
 * Demand string, but return a safe (=private) null-terminated copy 
 * of the string.   Return NULL if we can't read a string here.
 * Don't do any processing on the string; just copy exactly what was 
 * in the input.
 */
char *demand_string(lenP)
int *lenP;
{
	register int len;
	char *retval;

	SKIP_WHITESPACE();
	if (*input_line_pointer == '\"') 
	{
		for ( ++input_line_pointer, len = 0; *input_line_pointer != '\"'; ++input_line_pointer, ++len )
		{
			obstack_1grow(&notes, *input_line_pointer);
		}
	
		/* Null-terminate the string. */
		obstack_1grow(&notes,'\0');
		retval=obstack_finish(&notes);
		++input_line_pointer;
	} 
	else 
	{
		as_bad("Missing string");
		retval = NULL;
		ignore_rest_of_line();
	}
	*lenP = len;
	return(retval);
} /* demand_string() */
 

/*
 *			demand_processed_string()
 *
 * Demand string, but return a safe (=private) null-terminated copy 
 * of the string.   Return NULL if we can't read a string here.
 * Process any C-style escapes found in the string.
 */
static char *demand_processed_string(lenP)
int *lenP;
{
	register unsigned int c;
	register int len;
	char *retval;
	
	len = 0;
	SKIP_WHITESPACE();
	if (*input_line_pointer == '\"') 
	{
		input_line_pointer++;	/* Skip opening quote. */
		
		while (is_a_char(c = next_char_of_string())) 
		{
			obstack_1grow(&notes, c);
			len ++;
		}
		/* Null-terminate the string. */
		obstack_1grow(&notes,'\0');
		retval=obstack_finish(&notes);
	} else {
		as_bad("Missing string");
		retval = NULL;
		ignore_rest_of_line();
	}
	*lenP = len;
	return(retval);
} /* demand_processed_string() */


/*
 * parse_defsym_option
 * We have a -D foo=12 type option.  "arg" points to "foo=12".
 * "myname" points to the name of the assembler for error messages.
 * Extract the symbol name and the equals sign, then pass the
 * expression to the expression evaluator and let it do the rest.  
 */
parse_defsym_option( arg, myname )
char *arg;
char *myname;
{
	char *inbuf_sav = input_line_pointer;
	char *sym;
	char ch;

	input_line_pointer = sym = (char *) xmalloc (strlen(arg) + 4);
	strcpy (input_line_pointer, arg);
	strcat (input_line_pointer, "\n");

	ch = get_symbol_end(); /* name's delimiter */
	/*
	 * ch is character after symbol.
	 * That character's place in the input line is now '\0'.
	 * sym points to start of symbol name.
	 * input_line_pointer points to the '\0' where c was.
	 */
	if ( ch != '=' )
	{
#if defined(DOS)
		if ( ch == '-' || ch == '/' )
#else
		if ( ch == '-' )
#endif
		{
			/* Option, not arg, follows -D */
			as_bad("%s: Expected defsym expression after -D", myname);
			return;
		}
		for ( ++input_line_pointer; *input_line_pointer && *input_line_pointer != '='; ++input_line_pointer )
			;
		if ( *input_line_pointer == '\0' )
		{
			/* There was just a symbol name, no expression.
			 * Emulate cpp and set the symbol to 1.
			 */
			strcpy ( ++input_line_pointer, "1" );
			--input_line_pointer;
		}
	} 
		       
	equals(sym);
	free (sym);
	input_line_pointer = inbuf_sav;
}


static int is_it_end_of_statement() {
  SKIP_WHITESPACE();
  return (is_end_of_line [* input_line_pointer]);
} /* is_it_end_of_statement() */

void equals(sym_name)
char *sym_name;
{
  register symbolS *symbolP; /* symbol we are working with */

  input_line_pointer++;
  if (*input_line_pointer=='=')
    input_line_pointer++;

  while(*input_line_pointer==' ' || *input_line_pointer=='\t')
    input_line_pointer++;

  if (sym_name[0]=='.' && sym_name[1]=='\0') {
    /* Turn '. = mumble' into a .org mumble */
    register segT segtype;
    expressionS exp;
    register char *p;

    segtype = get_known_segmented_expression(& exp);
    if (! need_pass_2) {
      if ( exp.X_segment != curr_seg && segtype != SEG_ABSOLUTE )
        as_bad("Illegal segment \"%s\"", segment_name(exp.X_segment));
      p = frag_var(rs_org, 1, 1, (relax_substateT)0, exp.X_add_symbol,
                    exp.X_add_number, (char *)0);
      * p = 0;

      /* For the listing: this will be a fill-type frag eventually.
       * We can get the details we need from the frag later.
       */
      if ( listing_now ) listing_info ( fill, 1, p, curr_frag->fr_prev, 0 );
    } /* if (ok to make frag) */
  } else {
    symbolP=symbol_find_or_make(sym_name);
    pseudo_set(symbolP);
  }
} /* equals() */

/* .include -- include a file at this point. */

/* 
 * s_include()
 * Search for an include file first in the current directory, 
 * then in the user's include path list (if there is one)
 *
 * NOTE that the path "." will always be present, and that it will
 * always be the first item on the list.
 */
void s_include(arg)
	int arg;
{
	int 	i, len;
	FILE 	*try;
	char 	*path = NULL;
	char 	*filename = demand_string(&len);
	
	if ( filename == NULL )
		as_fatal("Can't Continue.");
	demand_empty_rest_of_line();

	for ( i = 0; i < include_dir_count; i++ ) 
	{
		path = xmalloc( strlen(include_dirs[i]) + len + 4);
		strcpy(path, include_dirs[i]);
		strcat(path, "/");
		strcat(path, filename);
		if ( try = fopen(path, "r") )
		{
			fclose (try);
			break;
		}
		free(path);
		path = NULL;
	}

	if ( path == NULL )
		as_fatal ("Can't find include file: %s", filename);

	if ( listing_now  && listflagseen['n'] )
	{
		/* Listing of include files is TURNED OFF.  So there won't 
		 * be any more listing lines until this include returns.
		 * So force a sync-up in the back end using THIS frag, not 
		 * the one when the include returns.
		 */
		listing_info ( info, 0, obstack_next_free(&frags), curr_frag, include_start );
	}
	input_scrub_include_file (path, input_line_pointer);
	buffer_limit = input_scrub_next_buffer (&input_line_pointer);

} /* s_include() */

void add_include_dir(path)
char *path;
{
	int i;
	
	include_dir_count++;
	include_dirs = (char **) xrealloc((char *)include_dirs, include_dir_count*sizeof (char *));
    
	include_dirs[include_dir_count-1] = path;	/* New one */
} /* add_include_dir() */

/*
 * s_list()
 * 
 * Controls the listing.  If arg == 1, start (or continue) the listing. 
 * If arg == 0, stop (suspend) the listing.  All of the tests below
 * are required.  For example, if -L was NOT seen, then silently 
 * ignore the .list.  If listing_now is already true, then do nothing.
 */
void s_list(arg)
int arg;
{
	if ( arg && flagseen['L'] && ! listing_now && ! (listing_include && listflagseen['n']) )
	{
		/* We're turning listing back on after being off;
		 * it's easiest just to force a .list into the 
		 * pass1 listing file, so that the resulting
		 * listing will make sense.  Put out an info
		 * line to sync up the address and source line.
		 */
		write_listing_pass1_source_line( ".list\n" );
		listing_info ( info, 0, obstack_next_free(&frags), curr_frag, segment_change );
		listing_now = 1;
		demand_empty_rest_of_line();
		return;
	}
	if ( ! arg && ! listflagseen['a'] )
	{
		/* Turn off listing, unless -La was seen on the cmd line */
		if ( ! listflagseen['a'] )
			listing_now = 0;
	}
	demand_empty_rest_of_line();
}

/*
 * s_title
 *
 * The .title's argument is supposed to become the listing title.
 *
 * Don't do that if either:
 *    If -Lt was given on the command line
 *    A previous .title has been seen
 */
void s_title()
{
	char	*str;
	int	len;

	if ( str = demand_string(&len) ) 
	{
		set_listing_title ( str );
	}
}

/*
 * s_eject
 *
 * Insert a formfeed character into the listing.
 */
void s_eject()
{
	demand_empty_rest_of_line();
	if ( listing_now )
		listing_info ( info, 0, NULL, NULL, eject );
}

static unsigned long map_section_attribute(a)
    char *a;
{
    static struct attrib_map {
	char *attribute_name;
	unsigned long attribute_value;
    } am[] = {
    { "lomem",       SECT_ATTR_LOMEM },
    { "text",        SECT_ATTR_TEXT },
    { "data",        SECT_ATTR_DATA },
    { "info",        SECT_ATTR_INFO },
    { "alloc",       SECT_ATTR_ALLOC },
    { "read",        SECT_ATTR_READ },
    { "write",       SECT_ATTR_WRITE },
    { "exec",        SECT_ATTR_EXEC },
    { "super_read",  SECT_ATTR_SUPER_READ },
    { "super_write", SECT_ATTR_SUPER_WRITE },
    { "super_exec",  SECT_ATTR_SUPER_EXEC },
    { "msb",         SECT_ATTR_MSB },
    { "pi",          SECT_ATTR_PI },
    { "bss",         SECT_ATTR_BSS },
    { NULL,          0 } };
    struct attrib_map *t = am;

    for (;t->attribute_name;t++)
	    if (!strcmp(t->attribute_name,a))
		    return t->attribute_value;
    as_bad("unknown .section attribute: %s",a);
    return 0;
}

static char *buystring(s)
    char *s;
{
    char *r = (char *) xmalloc(strlen(s)+1);
    strcpy(r,s);
    return r;
}

/* Recognize the .section directive for COFF or ELF objects.
 * During gas processing, output sections are treated as segments.
 * Hence the mismatch between variable names and error messages.
 */
void s_section()
{
    char 	  *seg_name;
    segT          seg_type   = SEG_DATA;  /* Default seg type is going to be SEG_DATA */
    unsigned long attributes = 0;         /* Default attributes are none. */
    symbolS       *symP;
    char	  ch;
    extern int pic_flag,pid_flag;
    
    seg_name = input_line_pointer;
    ch = get_symbol_end();

#ifdef OBJ_COFF
    if ( strlen (seg_name) > (unsigned) 8 )
    {
	as_warn ("Truncating section name to 8 characters");
	seg_name [8] = 0;
    }
#endif
    
    /* Are there any attributes following .section sect_name ? */
    if (ch == ',')
	    /* Pick off the OPTIONAL attributes one at a time and OR them into the attributes
	       word. */
	    while (ch == ',') {
		char *attribute_str;

		++input_line_pointer;
		attribute_str = input_line_pointer;
		ch = get_symbol_end();

		attributes |= map_section_attribute(attribute_str);
	    }
    else
	    /* There were no attributes... */
	    seg_name = buystring(seg_name);

    if (flagseen['G'])
	    attributes |= SECT_ATTR_MSB;

    if ((attributes & SECT_ATTR_MSB) &&
	 (attributes & SECT_ATTR_ALLOC))
	md_check_arch();

    if ( (attributes & SECT_ATTR_TEXT) == SECT_ATTR_TEXT ) {
	seg_type = SEG_TEXT;
	if (pic_flag)
		attributes |= SECT_ATTR_PI;
    }
    else if ((attributes & SECT_ATTR_DATA) == SECT_ATTR_DATA) {
	seg_type = SEG_DATA;
	if (pid_flag)
		attributes |= SECT_ATTR_PI;
    }
    else if ((attributes & SECT_ATTR_BSS) == SECT_ATTR_BSS) {
	seg_type = SEG_BSS;
    }

    if (((attributes & SECT_ATTR_BSS) == SECT_ATTR_BSS) &&
	(attributes & (SECT_ATTR_EXEC)))
	    as_bad("bss-type sections can not contain code.");

    if ((attributes & SECT_ATTR_WRITE) && pid_flag)
	    attributes |= SECT_ATTR_PI;

    *input_line_pointer = ch;
    demand_empty_rest_of_line();
    
    /* Input processing is completed.  Check user's segment name to see if 
     * it is already a segment name.
     */
    if ( (symP = symbol_find (seg_name)) && S_IS_DEFINED(symP) )
    {
	if ( S_IS_SEGNAME(symP) )
	{
	    int seg = seg_find (seg_name);

	    if ( SEG_GET_TYPE(seg) != seg_type || SEG_GET_FLAGS(seg) != attributes )
	    {
		as_bad ("Section name %s already defined", seg_name);
		return;
	    }
	    else
	    {
		/* OK, we have a RESUME.  Same name, same type, same attr. */

		if (seg == DEFAULT_BSS_SEGMENT)
		    /* Sigh. The .bss segment does not contain any frags,
		       so we can't "resume" by appending frags to it. */
		    as_bad("Illegal section name: .bss");

		frag_finish (0);
		seg_change (seg);
		if ( listing_now ) listing_info ( info, 0, NULL, curr_frag, segment_change );
		return;
	    }
	}
	else
	{
	    /* Symbol with this name already defined. */
	    as_bad ("Identifier already defined: %s", seg_name);
	    return;
	}
    }
    else
    {
	/* OK, we have a virgin identifier.  Create a new segment.
	 * Set global curr_seg to the segs[] index of the new segment.
	 */
	frag_finish(0);
	curr_seg = seg_new (seg_type, seg_name, attributes);
	seg_change (curr_seg);
	frag_new (0);
	if ( listing_now ) listing_info ( info, 0, NULL, curr_frag, segment_change );
    }
}

void s_ignore(arg)
int arg;
{
	extern char is_end_of_line[];

	while (!is_end_of_line[*input_line_pointer]) {
		++input_line_pointer;
	}
	++input_line_pointer;

	return;
} /* s_ignore() */

/* s_stab_stub
   When we are COFF or ELF, we want to be able to pass over .stab* directives
   without dying, but we can't just call s_ignore because stabs can contain
   semicolons.  So this stub just reads the expected fields and throws the
   contents away.
*/
void s_stab_stub(what)
int what;
{
    char *string;
    expressionS e;
    int goof = 0;	/* TRUE if we have aborted. */
    int length;
    int saved_type = 0;
    long longint;
    symbolS *symbolP = 0;
    
    if (what == 's') 
    {
	string = demand_string(&length);
	SKIP_WHITESPACE();
	
	if (*input_line_pointer == ',')
	    input_line_pointer++;
	else 
	{
	    as_bad("Expected comma after symbol's name");
	    goof = 1;
	}
    }
    
    /*
     * Input_line_pointer->after ','.  String->symbol name.
     */
    if (!goof) 
    {
	if (get_absolute_expression_and_terminator(&longint) != ',') 
	{
	    as_bad("Expected comma after the n_type expression");
	    goof = 1;
	    input_line_pointer--; /* Backup over a non-',' char. */
	}
    }
    
    if (!goof) 
    {
	if (get_absolute_expression_and_terminator(&longint) != ',') 
	{
	    as_bad("Expected comma after the n_other expression");
	    goof = 1;
	    input_line_pointer--; /* Backup over a non-',' char. */
	}
    }
    
    if (!goof) 
    {
	get_absolute_expression();
	
	if (what == 's' || what == 'n') 
	{
	    if (*input_line_pointer != ',')
	    {
		as_bad("Expected comma after the n_desc expression");
		goof = 1;
	    } 
	    else
		input_line_pointer++;
	}
    }
    
    expression(&e);
    
    if (goof) 
	ignore_rest_of_line();
    else
	demand_empty_rest_of_line();
}

#ifdef DEBUG
/* This is for assertion purposes, to make sure some memory is zero.  It is useful
 to debug named bss sections. */
void
make_sure_zero(p,n)
    char *p;
    int n;
{
    int i;

    for (i=0;i < n;i++)
	    know (!p[i]);
}
#endif
