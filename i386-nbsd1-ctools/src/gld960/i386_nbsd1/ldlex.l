%{

/* Copyright (C) 1991 Free Software Foundation, Inc.
 *
 * This file is part of GLD, the Gnu Linker.
 *
 * GLD is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * GLD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GLD; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* $Id: ldlex.l,v 1.43 1995/08/29 21:03:39 paulr Exp $ */

#include "sysdep.h"
#include "bfd.h"
#include <ctype.h>
#include "ldgramtb.h"
#include "ld.h"
#include "ldexp.h"
#include "ldgram.h"
#include "ldmisc.h"
#include "ldfile.h"

#undef input
#undef unput
#define input lex_input
#define unput lex_unput
#define RTOKEN(x)  { yylval.token = x; return x; }

#ifdef __STDC__
	#ifndef fgetc
		extern int fgetc(FILE *);
	#endif
#else
	extern int fgetc();
#endif

static struct keyword_type { char *name; int value; } keywords[] = {
	"ADDR",		ADDR,
	"ALIGN",	ALIGN_K,
	"align",	ALIGN_K,
	"BLOCK",	BLOCK,
	"BYTE",		BYTE,
	"CHECKSUM",	CHECKSUM,
	"checksum",	CHECKSUM,
	"COPY",		COPY,
	"CREATE_OBJECT_SYMBOLS", CREATE_OBJECT_SYMBOLS,
	"DEFINED",	DEFINED,
	"DSECT",	DSECT,
	"ENTRY",	ENTRY,
	"FILL",		FILL,
	"FLOAT",	FLOAT,
	"FORCE_COMMON_ALLOCATION", FORCE_COMMON_ALLOCATION,
	"GROUP",	GROUP,
	"group",	GROUP,
	"HLL",		HLL,
	"INPUT",	INPUT,
	"INCLUDE",	INCLUDE,
	"TARGET",	TARGET,
	"LENGTH",	LENGTH,
	"l",		LENGTH,
	"len",		LENGTH,
	"length",	LENGTH,
	"LONG",		LONG,
	"MEMORY",	MEMORY,

/* NEXT lexemes are transformed into ALIGNS */
	"NEXT",		ALIGN_K,

	"NOFLOAT",	NOFLOAT,
	"NOLOAD",	NOLOAD,
	"ORIGIN",	ORIGIN,
	"o",		ORIGIN,
	"org",		ORIGIN,
	"origin",	ORIGIN,
	"OUTPUT",	OUTPUT,
	"OUTPUT_ARCH",	OUTPUT_ARCH,
	"SEARCH_DIR",	SEARCH_DIR,
	"SECTIONS",	SECTIONS,
	"SHORT",	SHORT,
	"SIZEOF",	SIZEOF,
	"STARTUP",	STARTUP,
	"SYSLIB",	SYSLIB,
	0,		0
};

extern int yyparse();
extern boolean hex_mode;
extern char *ldfile_input_filename;
/* The following variable is used to know whether INCLUDE directives
   are treated as TARGET directives.  See ldgram.y and ldlex.l */
extern int in_target_scope;

#define NPUSHBACK 128
boolean in_defsym;
static int pushback[NPUSHBACK];
static FILE *script_file = (FILE*)NULL;
static unsigned int have_pushback;
static boolean in_comment;
static char *line;
static char *defsym_line;
static unsigned int lineno;

/*************************************************************************

We implement a synthesized stack to support TARGET and INCLUDE in
script files.  The overall idea is because yyparse() can not be called
recursively, we emulate this using iteration and sleight of hand with
the following stack of the parser's and scanner's environment.

See lex_input, parse_script_file and parse_defsym for the sleight of hand.
The following are the primitives used by those routines.

*************************************************************************/

static struct env_stack_node {
       int pushback[NPUSHBACK];
       FILE *script_file;
       unsigned int have_pushback;
       boolean in_defsym;
       char *line;
       char *defsym_line;
       unsigned int lineno;
       char *ldfile_input_filename;
       int in_target_scope;
       struct env_stack_node *next;
} *Env_stack;


/*
The following routine initializes the parser and scanner environment
as if it had never been called before:
*/
static Env_init()
{
     script_file           = (FILE *) 0;
     have_pushback         = 0;
     in_defsym             = 0;
     line                  = NULL;
     defsym_line           = NULL;
     lineno                = 0;
     ldfile_input_filename = NULL;
     in_target_scope       = 0;
}

static Env_nesting_depth = 0;

/*
The following restores the topmost node from the scanner/lexer stack.
if the stack is empty, it initializes the environment.
*/
static Env_pop(destructive)
     int destructive;     /* If this is non zero, Env_pop() will
                              free the top node's environment from memory. */
{
    if (Env_stack) {
       struct env_stack_node *p = Env_stack;

       Env_nesting_depth--;
       memcpy((char *)pushback,(char *)Env_stack->pushback,sizeof(pushback));
       script_file           = Env_stack->script_file;
       have_pushback         = Env_stack->have_pushback;
       in_defsym             = Env_stack->in_defsym;
       line                  = Env_stack->line;
       defsym_line           = Env_stack->defsym_line;
       lineno                = Env_stack->lineno;
       ldfile_input_filename = Env_stack->ldfile_input_filename;
       in_target_scope       = Env_stack->in_target_scope;
       Env_stack             = Env_stack->next;
       if (destructive)
          free(p);
    }
    else
       Env_init();
}

#define MAX_ENV_DEPTH 32

/*
The following pushes the current environment on the stack, then
initializes the environment.
*/
static Env_push()
{
     if (++Env_nesting_depth <= MAX_ENV_DEPTH) {
     struct env_stack_node *p = (struct env_stack_node *)
                                 ldmalloc(sizeof(struct env_stack_node));

     p->next                          = Env_stack;
     Env_stack                        = p;
     memcpy((char *)Env_stack->pushback,(char *)pushback,sizeof(pushback));
     Env_stack->script_file           = script_file;
     Env_stack->have_pushback         = have_pushback;
     Env_stack->in_defsym             = in_defsym;
     Env_stack->line                  = line;
     Env_stack->defsym_line           = defsym_line;
     Env_stack->lineno                = lineno;
     Env_stack->ldfile_input_filename = ldfile_input_filename;
     Env_stack->in_target_scope       = in_target_scope;
     Env_init();
     }
     else {
         lex_err();
         info("%P%F: maximum TARGET/INCLUDE depth exceeded\n");
     }
}

int
lex_input()
{
	int c;

	if (have_pushback > 0) {
		have_pushback--;
		c = pushback[have_pushback];
	} else if (line)  {
                if (*line)
		   c = *(line++);
                else {
                   if (in_defsym)
                           free(defsym_line);
                   /* We pop the environment here because there might be a
                     -defsym in a script file. (It is overkill to support this granted,
                     but it is elegant to be able to do so.  */
                   Env_pop(1);
		   /* Recursive call here could easily have been a goto top of lex_input(),
		      but the intention is clearer given the call. */
                   c = lex_input();
                }
	} else if (script_file) {
	          if ((c=fgetc(script_file)) == EOF) {
			if (in_comment)
			   info("%S%F unclosed comment in directives file\n");
			fclose(script_file);
			/* Here, we pop the environment because we got to EOF of a
			   script file.  If this script was nested, popping here
			   will return to the previous script's or defsym's stream. */               
                        Env_pop(1);
			c = lex_input();
		   }
		   else if (c == 0)
		     c = -1;   /* We just tried to read a binary file such
				  as a bout file while in coff mode, or vice
				  versa, and unfortunately the first char is
				  0 (which would signal the parser to exit w/o
		                  error if passed on to the scanner.) Signal
				  the scanner to issue an error in this case. */
	} else
         /*
          * The only way to reach this code is if the environment stack is exhausted and
	  * the first scope's input stream is also exhausted.
	  * We return zero here to signal the parser to return.
	  *
	  */
		c = 0;

	if (c == '\t') {
		c = ' ';
	} else if (c == '\n') { 
		c = ' '; 
		lineno++; 
	}
	return c;
}

static void
stack_unputter(c)
    int c;
{
    if (Env_stack && Env_nesting_depth > 0) {
       Env_stack->pushback[Env_stack->have_pushback] = c;
       Env_stack->have_pushback++;
    }
    else
       info("%F%P: internal error in linker.  Stack_unputter, stack underflow\n");
}

void
lex_unput(c)
    int c;
{
	if (have_pushback > NPUSHBACK) {
		info("%F%P Too many pushbacks\n");
	}
	pushback[have_pushback] = c;
	have_pushback ++;
}

void parse_command_line_option()
{
    char options[128],value1[128],value2[128];
    int c,i,j,back_from_process_switch,was_dash_T_or_defsym;
    void (*unputter)();

    strcpy(options,yytext);  /* options[0] = '-' || '/'
				options[1] = First option.
				options[2-(yyleng-)] = option's value / options ?
				(The above may not be there.) */
    /* We are now at the point indicated:
       -xo file
          ^
       Skip over the optional white space before the first argument 
    */
    while (isspace(c = input()))
	    ;
    /* We are now at the point indicated:

       -xo file
           ^
    */
    value1[0] = c;
    for (i=1;!isspace((c = input())) && c;value1[i++] = c)
	    ;
    value1[i] = 0;
    /* Skip over the white space before the (possible) second argument */
    while (isspace(c = input()))
	    ;
    value2[0] = c;
    for (j=1;!isspace((c = input())) && c;value2[j++] = c)
	    ;
    value2[j] = 0;
    unput(' ');   /* Unput the white space we just read. */
    back_from_process_switch = process_switch(buystring(options),
					      buystring(value1),
					      buystring(value2),
					      0,
					      &was_dash_T_or_defsym,
					      0); 
    unputter = was_dash_T_or_defsym ? stack_unputter : unput;
    switch (back_from_process_switch) {
 case 0:
	for (;j;j--)
		unputter(value2[j-1]);
	unputter(' ');
	for (;i;i--)
		unputter(value1[i-1]);
	unputter(' ');
	break;
 case 1:
	for (;j;j--)
		unputter(value2[j-1]);
	unputter(' ');
	break;
 case 2:
	break;
 default:
	ASSERT(0);
    }
}

int
yywrap()
{ 
	return 1; 
}

/*VARARGS*/

static int yyparsecallcnt = 0;

void
parse_line(arg)
    char *arg;
{
    Env_init();
    line = arg;
    lineno = 1;
    yyparse();
}

void
parse_script_file(f,name,target)
    FILE *f;
    char *name;
    int target;
{
     /* The global variable in_target_scope means that the current file being parsed
       was in a -Tfile or TARGET(file).  If this is the case, then it is sought for
       using a differnt scheme.  See ldfile_find_command_file for details.  Note that
       this changes the sense of INCLUDE directives. */
    int new_target_scope = in_target_scope || target;
    extern boolean write_map;

    if (write_map)
        add_file_to_filelist(ldfile,name,NULL,NULL);
    if (yyparsecallcnt)    /* if we previously called yyparse() ... */
        Env_push();       /* we push the current environment ... */
    else
        Env_init();
    ldfile_input_filename = name;  /* we switch the current environment to this one */
    script_file = f;
    lineno = 1;
    in_target_scope = new_target_scope;
    if (yyparsecallcnt == 0) {
          yyparsecallcnt++;
          yyparse();
          yyparsecallcnt--;
    }
}

/*
See above analogous comments regarding the sleight of hand here wrt
pushing and initializing the environment.
*/
void
parse_defsym(s)
    char *s;
{
        char buffer[1024];

        if (yyparsecallcnt)
	   Env_push();
        else
           Env_init();
	in_defsym = 1;
        sprintf(buffer,"%s;",s);
	line = defsym_line = buystring(buffer);
        lineno = 1;
        if (yyparsecallcnt == 0) {
           yyparsecallcnt++;
           yyparse();
           yyparsecallcnt--;
        }
}

/* Print source script file and line number for current environment only. */
static lex_err_real()
{
	int c,n;

	if (in_defsym) {
		fprintf(stderr,"'-defsym %s': ", defsym_line );
	} else if (script_file) {
		fprintf(stderr,"%s:%u", ldfile_input_filename, lineno );
	} else {
		fputs("command (just before \"",stderr);
		c = lex_input();
		for ( n=0; c != 0 && n < 10; n++) {
			fprintf(stderr, "%c", c);
			c = lex_input();
		}
		fputs("\")",stderr);
	}
}

/*
The following issues an error for the current environment and traverses all subsequent
environments allowing the user to tell how the file was actually reached. */
lex_err()
{
    struct env_stack_node *local = Env_stack;

    lex_err_real();
    if (Env_stack) {
       fprintf(stderr,"\nINCLUDE/TARGET trace: ");
       do {
          Env_pop(0);
	  lex_err_real();
	  fprintf(stderr,"\n");
       } while (Env_stack);
    }
    Env_stack = local;
}

static long
DEFUN_VOID(number)
{
	long base;
	unsigned long l,oldl;
	int ch;

	oldl = l = 0;
	ch = yytext[0];

	if (ch == '0') {
		base = in_defsym ? 16 : 8;
		ch = input();
		if ( ch == 'x' || ch == 'X' ){
		        ch = input();
			base = 16;
		}

	} else
		base = in_defsym ? 16 : 10;

	while (1) {
	        oldl = l;
		switch (ch) {
		case '8': case '9':
		        if (base == 8)
			        goto ejectit;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': 
			l = l * base + ch - '0';
			break;

		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		        if (base < 16) goto ejectit;
			l = (l*base) + ch - 'a' + 10;
			break;

		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		        if (base < 16) goto ejectit;
			l = (l*base) + ch - 'A' + 10;
			break;

		case 'k': case 'K':
			yylval.integer = l = l * 1024;
			if (oldl || l)
				if (l <= oldl) {
				    lex_err();
				    info("%P%F: overflow detected in integer constant\n");
				}
			return INT;

		case 'm': case 'M':
			yylval.integer = l = l * 1024 * 1024;
			if (oldl || l)
				if (l <= oldl) {
				    lex_err();
				    info("%P%F: overflow detected in integer constant\n");
				}
			return INT;

		default:
ejectit:
			unput(ch);
			yylval.integer = l;
			return INT;
		}
		if (oldl || l)
			if (l <= oldl) {
			    lex_err();
			    info("%P%F: overflow detected in integer constant\n");
			}
		ch = input();
	}
}
%}

/* Some defintions for identifier/number/keyword/filename lexer. */

/* None of the following are operators.  All characters in */
/* here can be found in an identifier:  */
ALPHA           [a-zA-Z0-9\$\.\_]

/* These are operators that can be found in a filename. */
/* But not in an identifier: */
SPECIAL         [\/\:\-\\]
%%
" "		{ }
"<<="		{ RTOKEN(LSHIFTEQ); }
">>="		{ RTOKEN(RSHIFTEQ); }
"||"		{ RTOKEN(OROR); }
"=="		{ RTOKEN(EQ); }
"!="		{ RTOKEN(NE); }
">="		{ RTOKEN(GE); }
"<="		{ RTOKEN(LE); }
"<<"		{ RTOKEN(LSHIFT); }
">>"		{ RTOKEN(RSHIFT); }
"+="		{ RTOKEN(PLUSEQ); }
"-="		{ RTOKEN(MINUSEQ); }
"*="		{ RTOKEN(MULTEQ); }
"/="		{ RTOKEN(DIVEQ); }
"&="		{ RTOKEN(ANDEQ); }
"|="		{ RTOKEN(OREQ); }
"&&"		{ RTOKEN(ANDAND); }
">"		{ RTOKEN('>'); }
"^"		{ RTOKEN('^'); }
","		{ RTOKEN(','); }
"&"		{ RTOKEN('&'); }
"|"		{ RTOKEN('|'); }
"~"		{ RTOKEN('~'); }
"!"		{ RTOKEN('!'); }
"?"		{ RTOKEN('?'); }
"*"		{ RTOKEN('*'); }
"%"		{ RTOKEN('%'); }
"<"		{ RTOKEN('<'); }
">"		{ RTOKEN('>'); }
"}"		{ RTOKEN('}') ; }
"{"		{ RTOKEN('{'); }
")"		{ RTOKEN(')'); }
"("		{ /* This mess is to support things like: '(COPY)' */
                  static char *buff;
                  static int bufflength = 0;
                  int curbufflength,c,alphastart=-1;
                  enum states { zero, one, two, bugout, checkandaccept} state = zero;
                  enum toktype { white, alpha, endofinput, rparen, other } curtoken;
                  static enum states nextstate[3][5] = {
                       {zero, one,    bugout, bugout,         bugout},
                       {two,  one,    bugout, checkandaccept, bugout},
                       {two,  bugout, bugout, checkandaccept, bugout} };

                  curbufflength = 0;
                  while (1) {
                        if (curbufflength+1 > bufflength) {
                           if (bufflength == 0)
                              buff = ldmalloc(bufflength=20);
                           else
                              buff = ldrealloc(buff,bufflength *= 2);
                        }
                        if (isspace((buff[curbufflength++]=c=input())))
                           curtoken = white;
                        else if (isalpha(c))
                           curtoken = alpha;
                        else if (c == 0)
                           curtoken = endofinput;
                        else if (c == ')')
                           curtoken = rparen;
                        else
                           curtoken = other;
                        switch (state=nextstate[(int)state][(int)curtoken]) {
                           case zero:
                                break;
                           case one:
                                if (alphastart == -1)
                                   alphastart = curbufflength-1;
                           case two:
                                break;
                           case bugout:
                             leave:
                                for (curbufflength--;curbufflength >= 0;curbufflength--)
                                   unput(buff[curbufflength]);
                                RTOKEN('(');
                                break;
                           case checkandaccept: {
                                static struct keyword_type parenkeys[] = {
                                                   {"COPY", COPY},
                                                   {"DSECT",DSECT},
                                                   {"NOLOAD",NOLOAD},
                                                   {0,0} };
                                int i;

                                for (i=0;parenkeys[i].name;i++)
                                    if (strncmp(parenkeys[i].name,&buff[alphastart],
                                          strlen(parenkeys[i].name)) == 0)
                                        RTOKEN(parenkeys[i].value);
                                goto leave;
                                break;
                         }
                        }
                  }
                }
"+"		{ RTOKEN('+'); }
"="		{ RTOKEN('='); }
"]"		{ RTOKEN(']'); }
"["		{ RTOKEN('['); }
";"		{ RTOKEN(';'); }
"/*"		{
			int c;
			in_comment = true;
			do {
				while ((c=input()) != '*') {
                                      lineno += c == '\n';
				}
				while ( (c=input()) == '*') {
				      ;
				}
                                lineno += c == '\n';
			} while (c != '/');
			in_comment = false;
		}
"\""[^\"]*"\""	{
			yylval.name = buystring(yytext+1);
			yylval.name[yyleng-2] = 0; /* Fry final quote */
			return NAME;
		}
({ALPHA}|{SPECIAL})*{ALPHA} {

/*
   The following mess parses the NAMECHARS into numbers, Reserved words,
   CLI, files, and symbols.  It bases this decision on:
      1. The scope where the token is found (whether the parser is expecting an
         expression or not), and also:
      2. if the linker is running on dos.
   Notice that the lexeme can NOT end in an operator by virtue of the above
   definition.
*/

	struct keyword_type *k;
        extern boolean ldgram_getting_exp,we_are_on_dos;

        if (ldgram_getting_exp) {
             int i,backoffto = -1;

             if (isdigit(yytext[0])) {

                /* Put the token back out on the input stream and call number.
                   note that the leading digit is retained
                   (per number() convention).: */
                for (i=yyleng;i-1;i--)
                      unput(yytext[i-1]);
                return number();          /* Numbers can only be returned in
                                             expression scope. */
             }

             for (i=0;yytext[i];i++)
                   if (yytext[i] == '/' ||
                       yytext[i] == ':' ||
                       yytext[i] == '\\' ||
                       yytext[i] == '-') {
                         backoffto = i;
                         break;
                   }
             /* We found an operator or the beginnings of a filename in yytext,
                we backoff to the point where it was found putting the chars
                back on the input stream. */
             if (backoffto != -1) {
                for (;yyleng != backoffto;)
                      unput(yytext[--yyleng]);
                if (backoffto == 0) {       /* If we only saw the operator,
                                               we must return it. */
                   yytext[0] = input();
                   yyleng = 1;
                }
                yytext[yyleng] = 0;
             }
             else  /* We did not find an operator it must be a symbol or a reserved word. */
            	 for (k = keywords; k->name; k++)
		     if ( !strcmp(k->name,yytext) ) {
			yylval.token = k->value;
			return k->value;
		     }

             /* If the first char is an operator, we return it.  Else we buy a string
                and return NAME */

             return (yytext[0] == '/' || yytext[0] == '-' || yytext[0] == ':') ?
                        yytext[0] : (yylval.name = buystring(yytext),NAME);
        }

        /* We are not in an expression scope, yytext can contain a filename, cli
           or a reserved word. */

        /* We have seen some cli if ... */

        else if ((yytext[0] == '/' && we_are_on_dos) || yytext[0] == '-') {

	    return CMD_LINE_OPT;
        }
        else {
             int pathname = 0,i;
             /* YYtext is either a symbol (e.g. abc = ...), a filename or a reserved word. */

            /* Let's see if it is a reserved word: */

            for (i=0;i < yyleng;i++)
                if (yytext[i] == '/' || yytext[i] == ':' || yytext[i] == '-' ||
                    yytext[i] == '\\') {
                   pathname = 1;
                   break;
                }

	    if ( !pathname ){
		for (k = keywords; k->name; k++) {
			if ( !strcmp(k->name,yytext) ) {
				yylval.token = k->value;
				return k->value;
			}
		}
            }
            /* It must be a pathname or a symbol. */
            yylval.name = buystring(yytext);
            return pathname ? PATHNAME : NAME;
        }
}
":"		{ RTOKEN(':'); }
"-"		{ RTOKEN('-'); }
"/"		{ RTOKEN('/'); }
.               { info("%P%F: Unexpected character %d (decimal) found at lineno %d of file %s\n",
                        (int) yytext[0],lineno,
                        ldfile_input_filename ? ldfile_input_filename : "(null)");
                }
%%
