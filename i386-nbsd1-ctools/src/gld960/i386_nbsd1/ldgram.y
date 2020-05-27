%{

/* 
 * This is a YACC grammer intended to parse a superset of the AT&T
 * linker scripting languaue.
 *
 * Written by Steve Chamberlain steve@cygnus.com
 * Modified by Intel Corporation.
 *
 * $Id: ldgram.y,v 1.63 1995/08/29 21:03:39 paulr Exp $
 */

#define DONTDECLARE_MALLOC
#include "sysdep.h"
#include "bfd.h"
#include "ld.h"    
#include "ldexp.h"
#include "ldlang.h"
#include "ldemul.h"
#include "ldmain.h"
#include "ldfile.h"
#include "ldmisc.h"
#include "ldsym.h"

#define YYDEBUG 1

lang_memory_region_type *region;
lang_memory_region_type *lang_memory_region_create();
lang_output_section_statement_type *lang_output_section_statement_lookup();

#ifdef __STDC__
	void lang_add_data(int type, union etree_union *exp);
	void lang_enter_output_section_statement(
		char *output_section_statement_name,
		etree_type *address_exp,
		int flags);
#else
	void lang_add_data();
	void lang_enter_output_section_statement();
#endif /* __STDC__ */

extern args_type command_line;
extern int lex_in_pathname;
extern char *output_filename;
extern boolean suppress_all_warnings;
extern boolean e_switch_seen;
extern boolean o_switch_seen;
extern boolean in_defsym;
char *current_file;
int in_target_scope;
boolean ldgram_getting_exp = false;
boolean we_are_on_dos =
#ifdef DOS
	        true
#else
		false
#endif
;

%}

%union {
	bfd_vma integer;
	char *name;
	int token;
	union etree_union *etree;
	asection *section;
	union  lang_statement_union **statement_ptr;
}

%type <etree> exp opt_exp  cksum_args
%type <integer> fill_opt opt_block opt_type
%type <name> memspec_opt pathname
%type <integer> length
%token <integer> INT  
%token <name> NAME PATHNAME
%right <token> PLUSEQ MINUSEQ MULTEQ DIVEQ  '=' LSHIFTEQ RSHIFTEQ   ANDEQ OREQ 
%right <token> '?' ':'
%left <token> CKSUM
%left <token> OROR
%left <token> ANDAND
%left <token> '|'
%left <token> '^'
%left <token> '&'
%left <token> EQ NE
%left <token> '<' '>' LE GE
%left <token> LSHIFT RSHIFT
%left <token> '+' '-'
%left <token> '*' '/' '%'
%right UNARY
%left <token> '('
%token <token> ALIGN_K BLOCK LONG SHORT BYTE
%token SECTIONS  
%token '{' '}'
%token SIZEOF_HEADERS FORCE_COMMON_ALLOCATION OUTPUT_ARCH
%token MEMORY NOLOAD DSECT COPY
%token DEFINED SEARCH_DIR ENTRY
%token <integer> SIZEOF ADDR 
%token STARTUP HLL SYSLIB FLOAT NOFLOAT CHECKSUM GROUP
%token ORIGIN FILL LENGTH CREATE_OBJECT_SYMBOLS INPUT TARGET INCLUDE OUTPUT CMD_LINE_OPT
%type <token> assign_op 

%%

parse: script
	;

script:	script directive
        |
	;

directive:
	  MEMORY '{' { ldgram_getting_exp = true; }
			memory_spec memory_spec_list '}'
                        { ldgram_getting_exp = false; }
	| SECTIONS { ldgram_getting_exp = true; }
                     	'{' sec_or_group '}'  
			{ ldgram_getting_exp = false; }
	| STARTUP '(' pathname ')'	{ lang_startup($3); ldgram_getting_exp = false; }
	| HLL '(' hll_NAME_list ')'     { ldgram_getting_exp = false; }
	| HLL '(' ')'			{ ldemul_hll((char *)NULL); ldgram_getting_exp = false; }
	| SYSLIB '(' syslib_NAME_list ')' { ldgram_getting_exp = false; }
	| FLOAT				{ lang_float(true); ldgram_getting_exp = false; }
	| NOFLOAT			{ lang_float(false); ldgram_getting_exp = false; }	
	| statement_anywhere            { ldgram_getting_exp = false; }
	| ';'                           { ldgram_getting_exp = false; }
	| SEARCH_DIR '(' pathname ')'	{ ldfile_add_library_path($3,1);
					  ldgram_getting_exp = false; }
	| OUTPUT '(' pathname ')'	{ if (!o_switch_seen) output_filename = $3; ldgram_getting_exp = false; }
	| OUTPUT_ARCH '(' NAME ')'	{ ldfile_add_arch($3); ldgram_getting_exp = false; }
	| FORCE_COMMON_ALLOCATION       { command_line.force_common_definition=true;
					  ldgram_getting_exp = false; }
	| INPUT '(' file_list ')'       { ldgram_getting_exp = false; }
        | pathname 			{ parse_three_args($1,"","",0,0);
					  ldgram_getting_exp = false; }
        | CMD_LINE_OPT                  { parse_command_line_option(); ldgram_getting_exp = false; }
        | INCLUDE '(' pathname ')'      { command_file_list t;
					  FILE *f;
					  char *realname;
					  t.named_with_T = in_target_scope;
					  t.filename = $3;
					  f = ldfile_find_command_file(&t,&realname);
					  parse_script_file(f,realname,0);
					  ldgram_getting_exp = false; }
        | TARGET '(' pathname ')'       { command_file_list t;
					  FILE *f;
					  char *realname;
					  t.named_with_T = true;
					  t.filename = $3;
					  f = ldfile_find_command_file(&t,&realname);
					  parse_script_file(f,realname,1);
					  ldgram_getting_exp = false; }
	;

file_list:
	pathname
	     {lang_add_input_file($1,lang_input_file_is_file_enum,(char*)NULL,(bfd *)0,-1,-1);}
	| file_list ',' pathname
	     {lang_add_input_file($3,lang_input_file_is_file_enum,(char*)NULL,(bfd *)0,-1,-1);}
	| file_list pathname
	     {lang_add_input_file($2,lang_input_file_is_file_enum,(char*)NULL,(bfd *)0,-1,-1);}
	;

pathname: NAME | PATHNAME ;

sec_or_group:
	  sec_or_group group
	| sec_or_group section
	|
	;

scns_in_group:
          scns_in_group section
        |
        ;

group:
        GROUP opt_exp { ldgram_getting_exp = false; } opt_block ':' '{'
                        { lang_enter_group_statement($2); }
                        scns_in_group '}' fill_opt memspec_opt
                        { ldgram_getting_exp = true;
                        lang_leave_group_statement($10,$11); }
        ;

statement_anywhere:
	  ENTRY '(' NAME ')'	{ if (!e_switch_seen) lang_add_entry($3); }
	| assignment
	;

section:
	NAME opt_exp { ldgram_getting_exp = false; }
			opt_type opt_block ':' opt_things '{' 
			{ lang_enter_output_section_statement ($1,$2,$4);}
	statement '}' fill_opt memspec_opt { ldgram_getting_exp = true;
			lang_leave_output_section_statement($12,$13); }
	;

opt_things: { }
        ;

statement:
	  statement assignment
	| statement CREATE_OBJECT_SYMBOLS
		{ lang_add_attribute(lang_object_symbols_statement_enum); }
	| statement input_section_spec
	| statement length lparenexp exp ')' { lang_add_data($2,$4); }
	| statement FILL lparenexp exp ')' { lang_add_fill
	   (exp_get_value_int($4,0,"fill value",lang_first_phase_enum)); }
	|
	;

opt_type:	
	  NOLOAD	{ $$ = LDLANG_NOLOAD; }
	| DSECT		{ $$ = LDLANG_DSECT; }
	| COPY		{ $$ = LDLANG_COPY; }
  	|		{ $$ = 0; }
	;

opt_exp:
	  { ldgram_getting_exp = true; } exp { $$ = $2; }
	| { $$= (etree_type *)NULL; }
	;

lparenexp:
	'(' { ldgram_getting_exp = true; }
        ;

opt_block:
	BLOCK lparenexp exp ')'
		{ $$ = 1; 
		if (!suppress_all_warnings) {
			fprintf(stderr,"WARNING: BLOCK linker directive no longer has any function\n");
		} }
	| { $$  = 1; }
	;
  
memspec_opt:
	'>' NAME { $$ = $2; }
	| '>' '(' NAME ')' {
                char *temp;
                temp = ldmalloc(strlen($3)+3);
		sprintf(temp,"(%s)",$3);
                $$ = temp;
	        }
	| { $$ = (char *)NULL;}
	;

section_list:
	  NAME { lang_add_wild($1, current_file); }
	| section_list opt_comma pathname { lang_add_wild($3, current_file); }
	;

input_section_spec:
	  pathname opt_comma { lang_add_wild((char *)NULL, $1); }
	| '['	   { current_file = (char *)NULL; }	section_list  ']' 
	| pathname { current_file = $1; } '(' section_list ')' opt_comma
	| '*'	   { current_file = (char *)NULL; }	'(' section_list ')'
	;

length	: LONG { $$ = $1; }
	| SHORT { $$ = $1; }
	| BYTE { $$ = $1; }
	;

fill_opt:
	assequals INT { $$ = $2; ldgram_getting_exp = false;}
	| { extern int default_fill_value;
	    $$ = default_fill_value | LDLANG_DFLT_FILL;
	  }
	;

assign_op:
	  PLUSEQ	{ $$ = '+';  ldgram_getting_exp = true; }
	| MINUSEQ	{ $$ = '-';  ldgram_getting_exp = true; }
	| MULTEQ	{ $$ = '*';  ldgram_getting_exp = true; }
	| DIVEQ		{ $$ = '/';  ldgram_getting_exp = true; }
	| LSHIFTEQ	{ $$ = LSHIFT;  ldgram_getting_exp = true; }
	| RSHIFTEQ	{ $$ = RSHIFT;  ldgram_getting_exp = true; }
	| ANDEQ		{ $$ = '&';  ldgram_getting_exp = true; }
	| OREQ		{ $$ = '|';  ldgram_getting_exp = true; }
	;

end:	';' | ',';

assequals:
	'=' {ldgram_getting_exp = true; }
        ;

assignment:
 	  /* What is happening in the following statement between curly
 	  braces is this: the name to which the CHECKSUM total is to be
	  assigned is checked to make sure it doesn't conflict with a name
	  defined with a -defsym on the command line; the result of the
	  cksum_args non-terminal is passed through add_cksum() in ldexp.c
	  one more time to pick up the carry bit; the result of that is
	  negated so that, when it is summed with the arguments to CHECKSUM,
	  zero will result; and that value is assigned to the variable to
	  the left of the equals sign in the original command */
 	  NAME assequals CHECKSUM '(' cksum_args ')' {
		ldsym_type *temp;
		temp = ldsym_get($1);
		if (temp->defsym_flag)
			info("%F Attempt to assign CHECKSUM to symbol %s defined on command line\n",$1);
		temp->assignment_flag = 1;
	        lang_add_assignment(exp_assop('=',$1,exp_unop('-',
                   exp_binop(CKSUM,exp_intop(0x0),$5)))); 
		ldgram_getting_exp=false;
	  }
          end
        | NAME assequals exp {
                if (in_defsym) {
                        ldsym_type *temp;
			/* Create a symbol that remembers it was defined
			with a defsym */
                        temp = ldsym_get($1);
                        temp->defsym_flag = true;
			/* This will overwrite any prior definition of this
			symbol */
                        lang_add_assignment(exp_assop('=',$1,$3));
                }
                else {
                        ldsym_type *temp;
                        temp = ldsym_get($1);
			/* We only generate an assignment statement if
			this symbol was not previously defined by a defsym */
                        if (!temp->defsym_flag) {
                                lang_add_assignment(exp_assop('=',$1,$3));
				temp->assignment_flag = true;
                        }
                }
		ldgram_getting_exp=false;
          }
          end
	| NAME assign_op exp  {
                if (in_defsym) {
                        ldsym_type *temp;
			/* Create a symbol that remembers it was defined
			with a defsym */
                        temp = ldsym_get($1);
                        temp->defsym_flag = true;
			/* This will overwrite any prior definition of this
			symbol */
                        lang_add_assignment(exp_assop('=',$1,
				exp_binop($2,exp_nameop(NAME,$1),$3)));
                }
                else {
                        ldsym_type *temp;
                        temp = ldsym_get($1);
			/* We only generate an assignment statement if
			this symbol was not previously defined by a defsym */
                        if (!temp->defsym_flag) {
                        	lang_add_assignment(exp_assop('=',$1,
					exp_binop($2,exp_nameop(NAME,$1),$3)));
				temp->assignment_flag = true;
                        }
                }
		ldgram_getting_exp=false;
	  }
          end
	;

	/* The CKSUM operation does a 33-bit add on its arguments, saving
	the 33rd bit in a static variable for addition the next time through */
cksum_args:
 	cksum_args ',' exp	{ $$ = exp_binop(CKSUM,$3,$1);}
 	| exp			{ $$ = $1; }
	;

opt_comma: ',' | ;

memory_spec_list:
	  memory_spec_list memory_spec 
	| memory_spec_list ',' memory_spec
	|
	;

memory_spec:
	NAME { region = lang_memory_region_create($1);}
		attributes_opt ':' origin_spec opt_comma length_spec { }
	;

origin_spec:
	ORIGIN assequals exp
		{ region->origin = exp_get_vma($3, 0L,"origin", lang_first_phase_enum); }
	;

length_spec:
	LENGTH assequals exp		
               { region->length_lower_32_bits = exp_get_vma($3, ~((bfd_vma)0), "length",
					       lang_first_phase_enum); }
	;
	
attributes_opt:
	'(' NAME ')' { lang_set_flags(&region->flags, $2); }
	|
	;

hll_NAME_list:
	  hll_NAME_list opt_comma pathname	{ ldemul_hll($3); }
	| pathname				{ ldemul_hll($1); }
	;

syslib_NAME_list:
	syslib_NAME_list opt_comma pathname { ldemul_syslib($3); }
	|
	;

exp:	  '-' exp %prec UNARY	{ $$ = exp_unop('-', $2); }
	| '(' exp ')'		{ $$ = $2; }
	| '!' exp  %prec UNARY	{ $$ = exp_unop('!', $2); }
	| '+' exp  %prec UNARY	{ $$ = $2; }
	| '~' exp  %prec UNARY	{ $$ = exp_unop('~', $2);}
	| exp '*' exp		{ $$ = exp_binop('*', $1, $3); }
	| exp '/' exp		{ $$ = exp_binop('/', $1, $3); }
	| exp '%' exp		{ $$ = exp_binop('%', $1, $3); }
	| exp '+' exp		{ $$ = exp_binop('+', $1, $3); }
	| exp '-' exp		{ $$ = exp_binop('-' , $1, $3); }
	| exp LSHIFT exp	{ $$ = exp_binop(LSHIFT , $1, $3); }
	| exp RSHIFT exp	{ $$ = exp_binop(RSHIFT , $1, $3); }
	| exp EQ exp		{ $$ = exp_binop(EQ , $1, $3); }
	| exp NE exp		{ $$ = exp_binop(NE , $1, $3); }
	| exp LE exp		{ $$ = exp_binop(LE , $1, $3); }
	| exp GE exp		{ $$ = exp_binop(GE , $1, $3); }
	| exp '<' exp		{ $$ = exp_binop('<' , $1, $3); }
	| exp '>' exp		{ $$ = exp_binop('>' , $1, $3); }
	| exp '&' exp		{ $$ = exp_binop('&' , $1, $3); }
	| exp '^' exp		{ $$ = exp_binop('^' , $1, $3); }
	| exp '|' exp		{ $$ = exp_binop('|' , $1, $3); }
	| exp '?' exp ':' exp	{ $$ = exp_trinop('?' , $1, $3, $5); }
	| exp ANDAND exp	{ $$ = exp_binop(ANDAND , $1, $3); }
	| exp OROR exp		{ $$ = exp_binop(OROR , $1, $3); }
	| DEFINED '(' NAME ')'	{ $$ = exp_nameop(DEFINED, $3); }
	| INT			{ $$ = exp_intop($1); }
	| SIZEOF_HEADERS	{ $$ = exp_nameop(SIZEOF_HEADERS,0); }
	| SIZEOF  '('  NAME ')'	{ $$ = exp_nameop(SIZEOF,$3); }
	| ADDR '(' NAME ')'	{ $$ = exp_nameop(ADDR,$3); }
	| ALIGN_K '(' exp ')'	{ $$ = exp_unop(ALIGN_K,$3); }
	| NAME			{ $$ = exp_nameop(NAME,$1); }
	;
