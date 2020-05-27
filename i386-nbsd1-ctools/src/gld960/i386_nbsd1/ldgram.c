#if !defined(WIN95) && !defined(GNU960)
extern char *malloc(), *realloc();
#endif

# line 2 "ldgram.y"

/* 
 * This is a YACC grammer intended to parse a superset of the AT&T
 * linker scripting languaue.
 *
 * Written by Steve Chamberlain steve@cygnus.com
 * Modified by Intel Corporation.
 *
 * $Id: ldgram.c,v 1.52 1995/08/29 21:07:40 paulr Exp $
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


# line 62 "ldgram.y"
typedef union  {
	bfd_vma integer;
	char *name;
	int token;
	union etree_union *etree;
	asection *section;
	union  lang_statement_union **statement_ptr;
} YYSTYPE;
# define INT 257
# define NAME 258
# define PATHNAME 259
# define PLUSEQ 260
# define MINUSEQ 261
# define MULTEQ 262
# define DIVEQ 263
# define LSHIFTEQ 264
# define RSHIFTEQ 265
# define ANDEQ 266
# define OREQ 267
# define CKSUM 268
# define OROR 269
# define ANDAND 270
# define EQ 271
# define NE 272
# define LE 273
# define GE 274
# define LSHIFT 275
# define RSHIFT 276
# define UNARY 277
# define ALIGN_K 278
# define BLOCK 279
# define LONG 280
# define SHORT 281
# define BYTE 282
# define SECTIONS 283
# define SIZEOF_HEADERS 284
# define FORCE_COMMON_ALLOCATION 285
# define OUTPUT_ARCH 286
# define MEMORY 287
# define NOLOAD 288
# define DSECT 289
# define COPY 290
# define DEFINED 291
# define SEARCH_DIR 292
# define ENTRY 293
# define SIZEOF 294
# define ADDR 295
# define STARTUP 296
# define HLL 297
# define SYSLIB 298
# define FLOAT 299
# define NOFLOAT 300
# define CHECKSUM 301
# define GROUP 302
# define ORIGIN 303
# define FILL 304
# define LENGTH 305
# define CREATE_OBJECT_SYMBOLS 306
# define INPUT 307
# define TARGET 308
# define INCLUDE 309
# define OUTPUT 310
# define CMD_LINE_OPT 311
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern int yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
YYSTYPE yylval, yyval;
# define YYERRCODE 256
int yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 134,
	58, 56,
	279, 56,
	-2, 54,
-1, 135,
	58, 56,
	279, 56,
	288, 56,
	289, 56,
	290, 56,
	-2, 54,
-1, 237,
	40, 68,
	-2, 97,
	};
# define YYNPROD 142
# define YYLAST 750
int yyact[]={

    12,   210,   131,   194,   239,   199,   200,   201,   197,    40,
   257,    69,    52,    25,   244,   258,   252,   186,    68,    18,
   223,    70,   102,    67,    81,   166,   165,   100,   103,    69,
   104,   164,   101,    64,    59,   230,    68,    23,   174,    70,
    92,    67,   228,    93,   225,   206,    54,   196,    51,    55,
   221,    57,    58,   238,    61,    62,    63,   171,    80,   212,
   102,   113,    50,    26,   262,   100,   103,    40,   104,    86,
   101,   102,   113,   229,   214,   261,   100,   103,   205,   104,
    94,   101,   185,   111,   143,   112,   116,   231,   102,   264,
   226,    86,   133,   100,   111,   260,   112,   116,   101,   142,
   269,   195,   182,    86,    71,   136,   102,   113,   137,   181,
   217,   100,   103,   138,   104,    38,   101,   114,   265,   180,
   267,   177,    71,    86,   178,    97,    96,    87,   114,   111,
    86,   112,   116,    84,    95,   135,    86,    91,   169,    90,
    89,    83,   259,    85,   102,   113,    88,   115,   183,   100,
   103,   102,   104,   135,   101,   256,   100,   103,   115,   104,
   207,   101,   173,   114,   127,   126,   125,   111,   124,   112,
   116,   141,   102,   113,   176,   209,   163,   100,   103,   134,
   104,    98,   101,    37,    36,    35,    34,    33,   170,    32,
    31,    30,    29,   115,    28,   111,   193,   112,   116,    24,
    25,   114,   172,   130,   128,    99,   190,   251,    41,    42,
    43,    44,    45,    46,    47,    48,   249,   250,   175,   234,
    24,    25,   227,   224,     5,   218,    16,    15,     4,   114,
   184,   115,   243,    13,    22,    73,    78,     6,     7,     8,
     9,    10,   240,   241,   242,   189,   208,   237,    17,    21,
    20,    14,    19,    73,    78,   216,    77,    52,    25,   115,
   105,   106,    74,    52,    25,   232,   236,   263,   233,    72,
   266,    81,    75,    76,    77,   187,   245,   220,   132,    65,
    74,   246,   247,   203,   268,   204,    60,    72,   253,    11,
    75,    76,   118,   117,   107,   108,   109,   110,   105,   106,
   168,   215,    56,   118,   117,   107,   108,   109,   110,   105,
   106,   102,   113,   222,    53,    82,   100,   103,    27,   104,
   129,   101,    49,     3,     2,   102,   113,   248,     1,    39,
   100,   103,   179,   104,   111,   101,   112,   116,   118,   117,
   107,   108,   109,   110,   105,   106,   102,   113,   111,   235,
   112,   100,   103,   198,   104,   139,   101,   270,     0,     0,
     0,     0,   202,     0,     0,     0,     0,     0,   114,   111,
     0,   112,   116,     0,     0,     0,   118,   117,   107,   108,
   109,   110,   105,   106,     0,     0,     0,     0,     0,   102,
   113,     0,     0,     0,   100,   103,     0,   104,   115,   101,
     0,     0,     0,   114,   118,   117,   107,   108,   109,   110,
   105,   106,   111,     0,   112,   102,   113,     0,     0,     0,
   100,   103,     0,   104,     0,   101,   102,   113,     0,     0,
     0,   100,   103,   115,   104,     0,   101,   102,   111,     0,
   112,     0,   100,   103,     0,   104,   114,   101,   102,   111,
     0,   112,     0,   100,   103,     0,   104,     0,   101,     0,
   111,     0,   112,     0,     0,     0,     0,     0,     0,     0,
     0,   111,   114,   112,     0,     0,   115,     0,     0,     0,
     0,     0,     0,   114,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,    66,   115,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,    79,     0,   118,   117,   107,   108,   109,   110,   105,
   106,     0,     0,     0,     0,     0,     0,     0,     0,   107,
   108,   109,   110,   105,   106,     0,     0,     0,     0,   119,
   120,   121,   122,   123,     0,     0,     0,     0,   118,   117,
   107,   108,   109,   110,   105,   106,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
   140,     0,   144,   145,   146,   147,   148,   149,   150,   151,
   152,   153,   154,   155,   156,   157,   158,   159,   160,   161,
   162,     0,   117,   107,   108,   109,   110,   105,   106,   167,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,   107,
   108,   109,   110,   105,   106,     0,     0,     0,     0,     0,
   107,   108,   109,   110,   105,   106,     0,     0,     0,     0,
     0,   107,   108,   109,   110,   105,   106,   188,     0,     0,
   191,   192,     0,     0,   109,   110,   105,   106,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,   211,     0,   213,     0,
     0,     0,     0,     0,     0,     0,     0,   219,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,   254,   255 };
int yypact[]={

 -1000, -1000,   -59, -1000,   -60, -1000,   154,   152,   151, -1000,
 -1000, -1000, -1000,   150,   149,   147, -1000,   146, -1000, -1000,
   145,   144,   143, -1000,   -52, -1000, -1000,   -61,  -246,     5,
 -1000,  -246,  -246,  -224,  -246,  -246,  -246,  -225,   -22,    -4,
 -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000,  -234,
 -1000,   100, -1000,    92, -1000, -1000,    86,    99,    98,    96,
    -1, -1000,    93,    85,    84,   141,   309,    -4,    -4,    -4,
    -4,    -4,   128, -1000, -1000,   126,   125,   124, -1000,   309,
 -1000, -1000,  -123, -1000, -1000,  -246, -1000, -1000,  -246, -1000,
 -1000, -1000, -1000,  -246, -1000, -1000, -1000, -1000,    -4,    40,
    -4,    -4,    -4,    -4,    -4,    -4,    -4,    -4,    -4,    -4,
    -4,    -4,    -4,    -4,    -4,    -4,    -4,    -4,    -4, -1000,
   135, -1000, -1000, -1000,  -227,  -232,  -233,    -4,    40,    13,
   122, -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000,    80,
   309, -1000, -1000, -1000, -1000, -1000, -1000,    51,    51,   114,
   114,   411,   411,   -15,   -15,   -15,   -15,   400,   288,   389,
   274,   378,   352, -1000,    78,    68,    61,   107, -1000, -1000,
 -1000,  -234,    24,  -241, -1000,    -4, -1000, -1000,    -4,    -4,
 -1000, -1000, -1000, -1000, -1000,  -300,    60,  -271,   309,  -283,
    40,   309,   309,    47,     6, -1000,    20,   120,  -271, -1000,
 -1000, -1000, -1000,  -304,    -4,   -64,    -4, -1000,    16, -1000,
     6,   309, -1000,    69, -1000,    -4, -1000, -1000,   -73,   309,
  -105, -1000, -1000,     6, -1000,    11,  -222,   -38, -1000,   -26,
 -1000,     6, -1000, -1000, -1000,   120,   120,    47, -1000, -1000,
 -1000, -1000, -1000, -1000,  -242,    11,    -4,    -4, -1000,   115,
  -243,   102,    54, -1000,    34,    23,  -243,    25, -1000,  -243,
 -1000, -1000, -1000,    79,  -246, -1000,    59,    47, -1000, -1000,
 -1000 };
int yypgo[]={

     0,   501,    38,   355,    44,    47,   353,    42,    19,   349,
   329,   328,   324,   323,   322,    58,   320,   318,   315,   314,
   302,   289,   286,   278,    92,   277,   275,   255,    37,   245,
   225,   223,   222,   219,    45,   218,    10,    89,   217,   216,
   207,    90,   171,   206,   205,   204,   203,   202,   196,   175 };
int yyr1[]={

     0,    11,    12,    12,    14,    13,    17,    13,    13,    13,
    13,    13,    13,    13,    13,    13,    13,    13,    13,    13,
    13,    13,    13,    13,    13,    22,    22,    22,     8,     8,
    18,    18,    18,    25,    25,    26,    27,    23,    21,    21,
    29,    31,    24,    30,    32,    32,    32,    32,    32,    32,
     6,     6,     6,     6,    35,     2,     2,    34,     5,     5,
     7,     7,     7,    36,    36,    33,    38,    33,    39,    33,
    40,    33,     9,     9,     9,     4,     4,    10,    10,    10,
    10,    10,    10,    10,    10,    42,    42,    41,    43,    28,
    44,    28,    45,    28,     3,     3,    37,    37,    16,    16,
    16,    46,    15,    48,    49,    47,    47,    19,    19,    20,
    20,     1,     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1 };
int yyr2[]={

     0,     2,     4,     0,     1,    13,     1,    11,     9,     9,
     7,     9,     3,     3,     3,     3,     9,     9,     9,     3,
     9,     3,     3,     9,     9,     3,     7,     5,     2,     2,
     4,     4,     0,     4,     0,     1,     1,    23,     9,     2,
     1,     1,    27,     1,     4,     5,     4,    11,    11,     0,
     3,     3,     3,     1,     1,     5,     1,     3,     9,     1,
     5,     9,     1,     3,     7,     5,     1,     8,     1,    12,
     1,    10,     3,     3,     3,     5,     1,     3,     3,     3,
     3,     3,     3,     3,     3,     2,     2,     3,     1,    16,
     1,    10,     1,    10,     7,     3,     2,     0,     4,     6,
     0,     1,    15,     7,     7,     7,     0,     7,     3,     7,
     0,     5,     7,     5,     5,     5,     7,     7,     7,     7,
     7,     7,     7,     7,     7,     7,     7,     7,     7,     7,
     7,     7,    11,     7,     7,     9,     3,     3,     9,     9,
     9,     3 };
int yychk[]={

 -1000,   -11,   -12,   -13,   287,   283,   296,   297,   298,   299,
   300,   -21,    59,   292,   310,   286,   285,   307,    -8,   311,
   309,   308,   293,   -28,   258,   259,   123,   -17,    40,    40,
    40,    40,    40,    40,    40,    40,    40,    40,   -41,   -10,
    61,   260,   261,   262,   263,   264,   265,   266,   267,   -14,
   123,    -8,   258,   -19,    41,    -8,   -20,    -8,    -8,   258,
   -22,    -8,    -8,    -8,   258,   301,    -1,    45,    40,    33,
    43,   126,   291,   257,   284,   294,   295,   278,   258,    -1,
   -15,   258,   -18,    41,    41,   -37,    44,    41,   -37,    41,
    41,    41,    41,    44,    -8,    41,    41,    41,    40,   -44,
    42,    47,    37,    43,    45,   275,   276,   271,   272,   273,
   274,    60,    62,    38,    94,   124,    63,   270,   269,    -1,
    -1,    -1,    -1,    -1,    40,    40,    40,    40,   -45,   -16,
   -46,   125,   -23,   -24,   302,   258,    -8,    -8,    -8,    -3,
    -1,   -42,    59,    44,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    41,   258,   258,   258,    -1,   -42,   125,
   -15,    44,   -47,    40,    -2,   -35,    -2,    41,    44,    58,
    41,    41,    41,    41,   -15,    58,   258,   -26,    -1,   -29,
   -43,    -1,    -1,   -48,   303,    41,    -5,   279,    -6,   288,
   289,   290,   -42,   -37,   -41,    58,   -34,    40,    -5,   -49,
   305,    -1,   123,    -1,    58,   -41,   -27,    41,   -30,    -1,
   -25,   123,   -24,   125,   -31,    -4,   -41,   -32,    -7,    62,
   257,   125,   -28,   306,   -33,    -9,   304,    -8,    91,    42,
   280,   281,   282,   258,    40,    -4,   -34,   -34,   -37,   -39,
   -38,   -40,   258,    -7,    -1,    -1,    40,   -36,   258,    40,
    41,    41,    41,   -36,   -37,    93,   -36,    41,    -8,    41,
   -37 };
int yydef[]={

     3,    -2,     1,     2,     0,     6,     0,     0,     0,    12,
    13,    14,    15,     0,     0,     0,    19,     0,    21,    22,
     0,     0,     0,    39,    28,    29,     4,     0,     0,     0,
   110,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    87,    77,    78,    79,    80,    81,    82,    83,    84,     0,
    32,     0,    28,    97,    10,   108,    97,     0,     0,     0,
     0,    25,     0,     0,     0,     0,    90,     0,     0,     0,
     0,     0,     0,   136,   137,     0,     0,     0,   141,    92,
   100,   101,     0,     8,     9,     0,    96,    11,     0,    16,
    17,    18,    20,     0,    27,    23,    24,    38,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,   111,
     0,   113,   114,   115,     0,     0,     0,     0,     0,     0,
   106,     7,    30,    31,    -2,    -2,   107,   109,    26,     0,
    95,    91,    85,    86,   116,   117,   118,   119,   120,   121,
   122,   123,   124,   125,   126,   127,   128,   129,   130,   131,
     0,   133,   134,   112,     0,     0,     0,     0,    93,     5,
    98,     0,     0,     0,    35,     0,    40,    88,     0,     0,
   135,   138,   139,   140,    99,     0,     0,    59,    55,    53,
     0,    94,   132,    97,     0,   105,     0,     0,    59,    50,
    51,    52,    89,     0,     0,     0,     0,    57,     0,   102,
     0,   103,    36,     0,    43,     0,    34,    58,     0,   104,
     0,    41,    33,    76,    49,    62,     0,     0,    37,     0,
    75,    76,    44,    45,    46,     0,     0,    -2,    66,    70,
    72,    73,    74,    60,     0,    62,     0,     0,    65,     0,
     0,     0,     0,    42,     0,     0,     0,    97,    63,     0,
    61,    47,    48,    97,     0,    67,    97,    97,    64,    71,
    69 };
typedef struct { char *t_name; int t_val; } yytoktype;
#ifndef YYDEBUG
#	define YYDEBUG	0	/* don't allow debugging */
#endif

#if YYDEBUG

yytoktype yytoks[] =
{
	"INT",	257,
	"NAME",	258,
	"PATHNAME",	259,
	"PLUSEQ",	260,
	"MINUSEQ",	261,
	"MULTEQ",	262,
	"DIVEQ",	263,
	"=",	61,
	"LSHIFTEQ",	264,
	"RSHIFTEQ",	265,
	"ANDEQ",	266,
	"OREQ",	267,
	"?",	63,
	":",	58,
	"CKSUM",	268,
	"OROR",	269,
	"ANDAND",	270,
	"|",	124,
	"^",	94,
	"&",	38,
	"EQ",	271,
	"NE",	272,
	"<",	60,
	">",	62,
	"LE",	273,
	"GE",	274,
	"LSHIFT",	275,
	"RSHIFT",	276,
	"+",	43,
	"-",	45,
	"*",	42,
	"/",	47,
	"%",	37,
	"UNARY",	277,
	"(",	40,
	"ALIGN_K",	278,
	"BLOCK",	279,
	"LONG",	280,
	"SHORT",	281,
	"BYTE",	282,
	"SECTIONS",	283,
	"{",	123,
	"}",	125,
	"SIZEOF_HEADERS",	284,
	"FORCE_COMMON_ALLOCATION",	285,
	"OUTPUT_ARCH",	286,
	"MEMORY",	287,
	"NOLOAD",	288,
	"DSECT",	289,
	"COPY",	290,
	"DEFINED",	291,
	"SEARCH_DIR",	292,
	"ENTRY",	293,
	"SIZEOF",	294,
	"ADDR",	295,
	"STARTUP",	296,
	"HLL",	297,
	"SYSLIB",	298,
	"FLOAT",	299,
	"NOFLOAT",	300,
	"CHECKSUM",	301,
	"GROUP",	302,
	"ORIGIN",	303,
	"FILL",	304,
	"LENGTH",	305,
	"CREATE_OBJECT_SYMBOLS",	306,
	"INPUT",	307,
	"TARGET",	308,
	"INCLUDE",	309,
	"OUTPUT",	310,
	"CMD_LINE_OPT",	311,
	"-unknown-",	-1	/* ends search */
};

char * yyreds[] =
{
	"-no such reduction-",
	"parse : script",
	"script : script directive",
	"script : /* empty */",
	"directive : MEMORY '{'",
	"directive : MEMORY '{' memory_spec memory_spec_list '}'",
	"directive : SECTIONS",
	"directive : SECTIONS '{' sec_or_group '}'",
	"directive : STARTUP '(' pathname ')'",
	"directive : HLL '(' hll_NAME_list ')'",
	"directive : HLL '(' ')'",
	"directive : SYSLIB '(' syslib_NAME_list ')'",
	"directive : FLOAT",
	"directive : NOFLOAT",
	"directive : statement_anywhere",
	"directive : ';'",
	"directive : SEARCH_DIR '(' pathname ')'",
	"directive : OUTPUT '(' pathname ')'",
	"directive : OUTPUT_ARCH '(' NAME ')'",
	"directive : FORCE_COMMON_ALLOCATION",
	"directive : INPUT '(' file_list ')'",
	"directive : pathname",
	"directive : CMD_LINE_OPT",
	"directive : INCLUDE '(' pathname ')'",
	"directive : TARGET '(' pathname ')'",
	"file_list : pathname",
	"file_list : file_list ',' pathname",
	"file_list : file_list pathname",
	"pathname : NAME",
	"pathname : PATHNAME",
	"sec_or_group : sec_or_group group",
	"sec_or_group : sec_or_group section",
	"sec_or_group : /* empty */",
	"scns_in_group : scns_in_group section",
	"scns_in_group : /* empty */",
	"group : GROUP opt_exp",
	"group : GROUP opt_exp opt_block ':' '{'",
	"group : GROUP opt_exp opt_block ':' '{' scns_in_group '}' fill_opt memspec_opt",
	"statement_anywhere : ENTRY '(' NAME ')'",
	"statement_anywhere : assignment",
	"section : NAME opt_exp",
	"section : NAME opt_exp opt_type opt_block ':' opt_things '{'",
	"section : NAME opt_exp opt_type opt_block ':' opt_things '{' statement '}' fill_opt memspec_opt",
	"opt_things : /* empty */",
	"statement : statement assignment",
	"statement : statement CREATE_OBJECT_SYMBOLS",
	"statement : statement input_section_spec",
	"statement : statement length lparenexp exp ')'",
	"statement : statement FILL lparenexp exp ')'",
	"statement : /* empty */",
	"opt_type : NOLOAD",
	"opt_type : DSECT",
	"opt_type : COPY",
	"opt_type : /* empty */",
	"opt_exp : /* empty */",
	"opt_exp : exp",
	"opt_exp : /* empty */",
	"lparenexp : '('",
	"opt_block : BLOCK lparenexp exp ')'",
	"opt_block : /* empty */",
	"memspec_opt : '>' NAME",
	"memspec_opt : '>' '(' NAME ')'",
	"memspec_opt : /* empty */",
	"section_list : NAME",
	"section_list : section_list opt_comma pathname",
	"input_section_spec : pathname opt_comma",
	"input_section_spec : '['",
	"input_section_spec : '[' section_list ']'",
	"input_section_spec : pathname",
	"input_section_spec : pathname '(' section_list ')' opt_comma",
	"input_section_spec : '*'",
	"input_section_spec : '*' '(' section_list ')'",
	"length : LONG",
	"length : SHORT",
	"length : BYTE",
	"fill_opt : assequals INT",
	"fill_opt : /* empty */",
	"assign_op : PLUSEQ",
	"assign_op : MINUSEQ",
	"assign_op : MULTEQ",
	"assign_op : DIVEQ",
	"assign_op : LSHIFTEQ",
	"assign_op : RSHIFTEQ",
	"assign_op : ANDEQ",
	"assign_op : OREQ",
	"end : ';'",
	"end : ','",
	"assequals : '='",
	"assignment : NAME assequals CHECKSUM '(' cksum_args ')'",
	"assignment : NAME assequals CHECKSUM '(' cksum_args ')' end",
	"assignment : NAME assequals exp",
	"assignment : NAME assequals exp end",
	"assignment : NAME assign_op exp",
	"assignment : NAME assign_op exp end",
	"cksum_args : cksum_args ',' exp",
	"cksum_args : exp",
	"opt_comma : ','",
	"opt_comma : /* empty */",
	"memory_spec_list : memory_spec_list memory_spec",
	"memory_spec_list : memory_spec_list ',' memory_spec",
	"memory_spec_list : /* empty */",
	"memory_spec : NAME",
	"memory_spec : NAME attributes_opt ':' origin_spec opt_comma length_spec",
	"origin_spec : ORIGIN assequals exp",
	"length_spec : LENGTH assequals exp",
	"attributes_opt : '(' NAME ')'",
	"attributes_opt : /* empty */",
	"hll_NAME_list : hll_NAME_list opt_comma pathname",
	"hll_NAME_list : pathname",
	"syslib_NAME_list : syslib_NAME_list opt_comma pathname",
	"syslib_NAME_list : /* empty */",
	"exp : '-' exp",
	"exp : '(' exp ')'",
	"exp : '!' exp",
	"exp : '+' exp",
	"exp : '~' exp",
	"exp : exp '*' exp",
	"exp : exp '/' exp",
	"exp : exp '%' exp",
	"exp : exp '+' exp",
	"exp : exp '-' exp",
	"exp : exp LSHIFT exp",
	"exp : exp RSHIFT exp",
	"exp : exp EQ exp",
	"exp : exp NE exp",
	"exp : exp LE exp",
	"exp : exp GE exp",
	"exp : exp '<' exp",
	"exp : exp '>' exp",
	"exp : exp '&' exp",
	"exp : exp '^' exp",
	"exp : exp '|' exp",
	"exp : exp '?' exp ':' exp",
	"exp : exp ANDAND exp",
	"exp : exp OROR exp",
	"exp : DEFINED '(' NAME ')'",
	"exp : INT",
	"exp : SIZEOF_HEADERS",
	"exp : SIZEOF '(' NAME ')'",
	"exp : ADDR '(' NAME ')'",
	"exp : ALIGN_K '(' exp ')'",
	"exp : NAME",
};
#endif /* YYDEBUG */
#line 1 "/usr/lib/yaccpar"
/*	@(#)yaccpar 1.10 89/04/04 SMI; from S5R3 1.10	*/

/*
** Skeleton parser driver for yacc output
*/

/*
** yacc user known macros and defines
*/
#define YYERROR		goto yyerrlab
#define YYACCEPT	{ free(yys); free(yyv); return(0); }
#define YYABORT		{ free(yys); free(yyv); return(1); }
#define YYBACKUP( newtoken, newvalue )\
{\
	if ( yychar >= 0 || ( yyr2[ yytmp ] >> 1 ) != 1 )\
	{\
		yyerror( "syntax error - cannot backup" );\
		goto yyerrlab;\
	}\
	yychar = newtoken;\
	yystate = *yyps;\
	yylval = newvalue;\
	goto yynewstate;\
}
#define YYRECOVERING()	(!!yyerrflag)
#ifndef YYDEBUG
#	define YYDEBUG	1	/* make debugging available */
#endif

/*
** user known globals
*/
int yydebug;			/* set to 1 to get debugging */

/*
** driver internal defines
*/
#define YYFLAG		(-1000)

/*
** static variables used by the parser
*/
static YYSTYPE *yyv;			/* value stack */
static int *yys;			/* state stack */

static YYSTYPE *yypv;			/* top of value stack */
static int *yyps;			/* top of state stack */

static int yystate;			/* current state */
static int yytmp;			/* extra var (lasts between blocks) */

int yynerrs;			/* number of errors */

int yyerrflag;			/* error recovery flag */
int yychar;			/* current input token number */


/*
** yyparse - return 0 if worked, 1 if syntax error not recovered from
*/
int
yyparse()
{
	register YYSTYPE *yypvt;	/* top of value stack for $vars */
	unsigned yymaxdepth = YYMAXDEPTH;

	/*
	** Initialize externals - yyparse may be called more than once
	*/
	yyv = (YYSTYPE*)malloc(yymaxdepth*sizeof(YYSTYPE));
	yys = (int*)malloc(yymaxdepth*sizeof(int));
	if (!yyv || !yys)
	{
		yyerror( "out of memory" );
		return(1);
	}
	yypv = &yyv[-1];
	yyps = &yys[-1];
	yystate = 0;
	yytmp = 0;
	yynerrs = 0;
	yyerrflag = 0;
	yychar = -1;

	goto yystack;
	{
		register YYSTYPE *yy_pv;	/* top of value stack */
		register int *yy_ps;		/* top of state stack */
		register int yy_state;		/* current state */
		register int  yy_n;		/* internal state number info */

		/*
		** get globals into registers.
		** branch to here only if YYBACKUP was called.
		*/
	yynewstate:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;
		goto yy_newstate;

		/*
		** get globals into registers.
		** either we just started, or we just finished a reduction
		*/
	yystack:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;

		/*
		** top of for (;;) loop while no reductions done
		*/
	yy_stack:
		/*
		** put a state and value onto the stacks
		*/
#if YYDEBUG
		/*
		** if debugging, look up token value in list of value vs.
		** name pairs.  0 and negative (-1) are special values.
		** Note: linear search is used since time is not a real
		** consideration while debugging.
		*/
		if ( yydebug )
		{
			register int yy_i;

			(void)printf( "State %d, token ", yy_state );
			if ( yychar == 0 )
				(void)printf( "end-of-file\n" );
			else if ( yychar < 0 )
				(void)printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				(void)printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ++yy_ps >= &yys[ yymaxdepth ] )	/* room on stack? */
		{
			/*
			** reallocate and recover.  Note that pointers
			** have to be reset, or bad things will happen
			*/
			int yyps_index = (yy_ps - yys);
			int yypv_index = (yy_pv - yyv);
			int yypvt_index = (yypvt - yyv);
			yymaxdepth += YYMAXDEPTH;
			yyv = (YYSTYPE*)realloc((char*)yyv,
				yymaxdepth * sizeof(YYSTYPE));
			yys = (int*)realloc((char*)yys,
				yymaxdepth * sizeof(int));
			if (!yyv || !yys)
			{
				yyerror( "yacc stack overflow" );
				return(1);
			}
			yy_ps = yys + yyps_index;
			yy_pv = yyv + yypv_index;
			yypvt = yyv + yypvt_index;
		}
		*yy_ps = yy_state;
		*++yy_pv = yyval;

		/*
		** we have a new state - find out what to do
		*/
	yy_newstate:
		if ( ( yy_n = yypact[ yy_state ] ) <= YYFLAG )
			goto yydefault;		/* simple state */
#if YYDEBUG
		/*
		** if debugging, need to mark whether new token grabbed
		*/
		yytmp = yychar < 0;
#endif
		if ( ( yychar < 0 ) && ( ( yychar = yylex() ) < 0 ) )
			yychar = 0;		/* reached EOF */
#if YYDEBUG
		if ( yydebug && yytmp )
		{
			register int yy_i;

			(void)printf( "Received token " );
			if ( yychar == 0 )
				(void)printf( "end-of-file\n" );
			else if ( yychar < 0 )
				(void)printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				(void)printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ( ( yy_n += yychar ) < 0 ) || ( yy_n >= YYLAST ) )
			goto yydefault;
		if ( yychk[ yy_n = yyact[ yy_n ] ] == yychar )	/*valid shift*/
		{
			yychar = -1;
			yyval = yylval;
			yy_state = yy_n;
			if ( yyerrflag > 0 )
				yyerrflag--;
			goto yy_stack;
		}

	yydefault:
		if ( ( yy_n = yydef[ yy_state ] ) == -2 )
		{
#if YYDEBUG
			yytmp = yychar < 0;
#endif
			if ( ( yychar < 0 ) && ( ( yychar = yylex() ) < 0 ) )
				yychar = 0;		/* reached EOF */
#if YYDEBUG
			if ( yydebug && yytmp )
			{
				register int yy_i;

				(void)printf( "Received token " );
				if ( yychar == 0 )
					(void)printf( "end-of-file\n" );
				else if ( yychar < 0 )
					(void)printf( "-none-\n" );
				else
				{
					for ( yy_i = 0;
						yytoks[yy_i].t_val >= 0;
						yy_i++ )
					{
						if ( yytoks[yy_i].t_val
							== yychar )
						{
							break;
						}
					}
					(void)printf( "%s\n", yytoks[yy_i].t_name );
				}
			}
#endif /* YYDEBUG */
			/*
			** look through exception table
			*/
			{
				register int *yyxi = yyexca;

				while ( ( *yyxi != -1 ) ||
					( yyxi[1] != yy_state ) )
				{
					yyxi += 2;
				}
				while ( ( *(yyxi += 2) >= 0 ) &&
					( *yyxi != yychar ) )
					;
				if ( ( yy_n = yyxi[1] ) < 0 )
					YYACCEPT;
			}
		}

		/*
		** check for syntax error
		*/
		if ( yy_n == 0 )	/* have an error */
		{
			/* no worry about speed here! */
			switch ( yyerrflag )
			{
			case 0:		/* new error */
				yyerror( "syntax error" );
				goto skip_init;
			yyerrlab:
				/*
				** get globals into registers.
				** we have a user generated syntax type error
				*/
				yy_pv = yypv;
				yy_ps = yyps;
				yy_state = yystate;
				yynerrs++;
			skip_init:
			case 1:
			case 2:		/* incompletely recovered error */
					/* try again... */
				yyerrflag = 3;
				/*
				** find state where "error" is a legal
				** shift action
				*/
				while ( yy_ps >= yys )
				{
					yy_n = yypact[ *yy_ps ] + YYERRCODE;
					if ( yy_n >= 0 && yy_n < YYLAST &&
						yychk[yyact[yy_n]] == YYERRCODE)					{
						/*
						** simulate shift of "error"
						*/
						yy_state = yyact[ yy_n ];
						goto yy_stack;
					}
					/*
					** current state has no shift on
					** "error", pop stack
					*/
#if YYDEBUG
#	define _POP_ "Error recovery pops state %d, uncovers state %d\n"
					if ( yydebug )
						(void)printf( _POP_, *yy_ps,
							yy_ps[-1] );
#	undef _POP_
#endif
					yy_ps--;
					yy_pv--;
				}
				/*
				** there is no state on stack with "error" as
				** a valid shift.  give up.
				*/
				YYABORT;
			case 3:		/* no shift yet; eat a token */
#if YYDEBUG
				/*
				** if debugging, look up token in list of
				** pairs.  0 and negative shouldn't occur,
				** but since timing doesn't matter when
				** debugging, it doesn't hurt to leave the
				** tests here.
				*/
				if ( yydebug )
				{
					register int yy_i;

					(void)printf( "Error recovery discards " );
					if ( yychar == 0 )
						(void)printf( "token end-of-file\n" );
					else if ( yychar < 0 )
						(void)printf( "token -none-\n" );
					else
					{
						for ( yy_i = 0;
							yytoks[yy_i].t_val >= 0;
							yy_i++ )
						{
							if ( yytoks[yy_i].t_val
								== yychar )
							{
								break;
							}
						}
						(void)printf( "token %s\n",
							yytoks[yy_i].t_name );
					}
				}
#endif /* YYDEBUG */
				if ( yychar == 0 )	/* reached EOF. quit */
					YYABORT;
				yychar = -1;
				goto yy_newstate;
			}
		}/* end if ( yy_n == 0 ) */
		/*
		** reduction by production yy_n
		** put stack tops, etc. so things right after switch
		*/
#if YYDEBUG
		/*
		** if debugging, print the string that is the user's
		** specification of the reduction which is just about
		** to be done.
		*/
		if ( yydebug )
			(void)printf( "Reduce by (%d) \"%s\"\n",
				yy_n, yyreds[ yy_n ] );
#endif
		yytmp = yy_n;			/* value to switch over */
		yypvt = yy_pv;			/* $vars top of value stack */
		/*
		** Look in goto table for next state
		** Sorry about using yy_state here as temporary
		** register variable, but why not, if it works...
		** If yyr2[ yy_n ] doesn't have the low order bit
		** set, then there is no action to be done for
		** this reduction.  So, no saving & unsaving of
		** registers done.  The only difference between the
		** code just after the if and the body of the if is
		** the goto yy_stack in the body.  This way the test
		** can be made before the choice of what to do is needed.
		*/
		{
			/* length of production doubled with extra bit */
			register int yy_len = yyr2[ yy_n ];

			if ( !( yy_len & 01 ) )
			{
				yy_len >>= 1;
				yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
				yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
					*( yy_ps -= yy_len ) + 1;
				if ( yy_state >= YYLAST ||
					yychk[ yy_state =
					yyact[ yy_state ] ] != -yy_n )
				{
					yy_state = yyact[ yypgo[ yy_n ] ];
				}
				goto yy_stack;
			}
			yy_len >>= 1;
			yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
			yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
				*( yy_ps -= yy_len ) + 1;
			if ( yy_state >= YYLAST ||
				yychk[ yy_state = yyact[ yy_state ] ] != -yy_n )
			{
				yy_state = yyact[ yypgo[ yy_n ] ];
			}
		}
					/* save until reenter driver code */
		yystate = yy_state;
		yyps = yy_ps;
		yypv = yy_pv;
	}
	/*
	** code supplied by user is placed in this switch
	*/
	switch( yytmp )
	{
		
case 4:
# line 113 "ldgram.y"
{ ldgram_getting_exp = true; } break;
case 5:
# line 115 "ldgram.y"
{ ldgram_getting_exp = false; } break;
case 6:
# line 116 "ldgram.y"
{ ldgram_getting_exp = true; } break;
case 7:
# line 118 "ldgram.y"
{ ldgram_getting_exp = false; } break;
case 8:
# line 119 "ldgram.y"
{ lang_startup(yypvt[-1].name); ldgram_getting_exp = false; } break;
case 9:
# line 120 "ldgram.y"
{ ldgram_getting_exp = false; } break;
case 10:
# line 121 "ldgram.y"
{ ldemul_hll((char *)NULL); ldgram_getting_exp = false; } break;
case 11:
# line 122 "ldgram.y"
{ ldgram_getting_exp = false; } break;
case 12:
# line 123 "ldgram.y"
{ lang_float(true); ldgram_getting_exp = false; } break;
case 13:
# line 124 "ldgram.y"
{ lang_float(false); ldgram_getting_exp = false; } break;
case 14:
# line 125 "ldgram.y"
{ ldgram_getting_exp = false; } break;
case 15:
# line 126 "ldgram.y"
{ ldgram_getting_exp = false; } break;
case 16:
# line 127 "ldgram.y"
{ ldfile_add_library_path(yypvt[-1].name,1);
					  ldgram_getting_exp = false; } break;
case 17:
# line 129 "ldgram.y"
{ if (!o_switch_seen) output_filename = yypvt[-1].name; ldgram_getting_exp = false; } break;
case 18:
# line 130 "ldgram.y"
{ ldfile_add_arch(yypvt[-1].name); ldgram_getting_exp = false; } break;
case 19:
# line 131 "ldgram.y"
{ command_line.force_common_definition=true;
					  ldgram_getting_exp = false; } break;
case 20:
# line 133 "ldgram.y"
{ ldgram_getting_exp = false; } break;
case 21:
# line 134 "ldgram.y"
{ parse_three_args(yypvt[-0].name,"","",0,0);
					  ldgram_getting_exp = false; } break;
case 22:
# line 136 "ldgram.y"
{ parse_command_line_option(); ldgram_getting_exp = false; } break;
case 23:
# line 137 "ldgram.y"
{ command_file_list t;
					  FILE *f;
					  char *realname;
					  t.named_with_T = in_target_scope;
					  t.filename = yypvt[-1].name;
					  f = ldfile_find_command_file(&t,&realname);
					  parse_script_file(f,realname,0);
					  ldgram_getting_exp = false; } break;
case 24:
# line 145 "ldgram.y"
{ command_file_list t;
					  FILE *f;
					  char *realname;
					  t.named_with_T = true;
					  t.filename = yypvt[-1].name;
					  f = ldfile_find_command_file(&t,&realname);
					  parse_script_file(f,realname,1);
					  ldgram_getting_exp = false; } break;
case 25:
# line 157 "ldgram.y"
{lang_add_input_file(yypvt[-0].name,lang_input_file_is_file_enum,(char*)NULL,(bfd *)0,-1,-1);} break;
case 26:
# line 159 "ldgram.y"
{lang_add_input_file(yypvt[-0].name,lang_input_file_is_file_enum,(char*)NULL,(bfd *)0,-1,-1);} break;
case 27:
# line 161 "ldgram.y"
{lang_add_input_file(yypvt[-0].name,lang_input_file_is_file_enum,(char*)NULL,(bfd *)0,-1,-1);} break;
case 35:
# line 178 "ldgram.y"
{ ldgram_getting_exp = false; } break;
case 36:
# line 179 "ldgram.y"
{ lang_enter_group_statement(yypvt[-4].etree); } break;
case 37:
# line 181 "ldgram.y"
{ ldgram_getting_exp = true;
                        lang_leave_group_statement(yypvt[-1].integer,yypvt[-0].name); } break;
case 38:
# line 186 "ldgram.y"
{ if (!e_switch_seen) lang_add_entry(yypvt[-1].name); } break;
case 40:
# line 191 "ldgram.y"
{ ldgram_getting_exp = false; } break;
case 41:
# line 193 "ldgram.y"
{ lang_enter_output_section_statement (yypvt[-7].name,yypvt[-6].etree,yypvt[-4].integer);} break;
case 42:
# line 194 "ldgram.y"
{ ldgram_getting_exp = true;
			lang_leave_output_section_statement(yypvt[-1].integer,yypvt[-0].name); } break;
case 43:
# line 198 "ldgram.y"
{ } break;
case 45:
# line 204 "ldgram.y"
{ lang_add_attribute(lang_object_symbols_statement_enum); } break;
case 47:
# line 206 "ldgram.y"
{ lang_add_data(yypvt[-3].integer,yypvt[-1].etree); } break;
case 48:
# line 207 "ldgram.y"
{ lang_add_fill
	   (exp_get_value_int(yypvt[-1].etree,0,"fill value",lang_first_phase_enum)); } break;
case 50:
# line 213 "ldgram.y"
{ yyval.integer = LDLANG_NOLOAD; } break;
case 51:
# line 214 "ldgram.y"
{ yyval.integer = LDLANG_DSECT; } break;
case 52:
# line 215 "ldgram.y"
{ yyval.integer = LDLANG_COPY; } break;
case 53:
# line 216 "ldgram.y"
{ yyval.integer = 0; } break;
case 54:
# line 220 "ldgram.y"
{ ldgram_getting_exp = true; } break;
case 55:
# line 220 "ldgram.y"
{ yyval.etree = yypvt[-0].etree; } break;
case 56:
# line 221 "ldgram.y"
{ yyval.etree= (etree_type *)NULL; } break;
case 57:
# line 225 "ldgram.y"
{ ldgram_getting_exp = true; } break;
case 58:
# line 230 "ldgram.y"
{ yyval.integer = 1; 
		if (!suppress_all_warnings) {
			fprintf(stderr,"WARNING: BLOCK linker directive no longer has any function\n");
		} } break;
case 59:
# line 234 "ldgram.y"
{ yyval.integer  = 1; } break;
case 60:
# line 238 "ldgram.y"
{ yyval.name = yypvt[-0].name; } break;
case 61:
# line 239 "ldgram.y"
{
                char *temp;
                temp = ldmalloc(strlen(yypvt[-1].name)+3);
		sprintf(temp,"(%s)",yypvt[-1].name);
                yyval.name = temp;
	        } break;
case 62:
# line 245 "ldgram.y"
{ yyval.name = (char *)NULL;} break;
case 63:
# line 249 "ldgram.y"
{ lang_add_wild(yypvt[-0].name, current_file); } break;
case 64:
# line 250 "ldgram.y"
{ lang_add_wild(yypvt[-0].name, current_file); } break;
case 65:
# line 254 "ldgram.y"
{ lang_add_wild((char *)NULL, yypvt[-1].name); } break;
case 66:
# line 255 "ldgram.y"
{ current_file = (char *)NULL; } break;
case 68:
# line 256 "ldgram.y"
{ current_file = yypvt[-0].name; } break;
case 70:
# line 257 "ldgram.y"
{ current_file = (char *)NULL; } break;
case 72:
# line 260 "ldgram.y"
{ yyval.integer = yypvt[-0].token; } break;
case 73:
# line 261 "ldgram.y"
{ yyval.integer = yypvt[-0].token; } break;
case 74:
# line 262 "ldgram.y"
{ yyval.integer = yypvt[-0].token; } break;
case 75:
# line 266 "ldgram.y"
{ yyval.integer = yypvt[-0].integer; ldgram_getting_exp = false;} break;
case 76:
# line 267 "ldgram.y"
{ extern int default_fill_value;
	    yyval.integer = default_fill_value | LDLANG_DFLT_FILL;
	  } break;
case 77:
# line 273 "ldgram.y"
{ yyval.token = '+';  ldgram_getting_exp = true; } break;
case 78:
# line 274 "ldgram.y"
{ yyval.token = '-';  ldgram_getting_exp = true; } break;
case 79:
# line 275 "ldgram.y"
{ yyval.token = '*';  ldgram_getting_exp = true; } break;
case 80:
# line 276 "ldgram.y"
{ yyval.token = '/';  ldgram_getting_exp = true; } break;
case 81:
# line 277 "ldgram.y"
{ yyval.token = LSHIFT;  ldgram_getting_exp = true; } break;
case 82:
# line 278 "ldgram.y"
{ yyval.token = RSHIFT;  ldgram_getting_exp = true; } break;
case 83:
# line 279 "ldgram.y"
{ yyval.token = '&';  ldgram_getting_exp = true; } break;
case 84:
# line 280 "ldgram.y"
{ yyval.token = '|';  ldgram_getting_exp = true; } break;
case 87:
# line 286 "ldgram.y"
{ldgram_getting_exp = true; } break;
case 88:
# line 299 "ldgram.y"
{
		ldsym_type *temp;
		temp = ldsym_get(yypvt[-5].name);
		if (temp->defsym_flag)
			info("%F Attempt to assign CHECKSUM to symbol %s defined on command line\n",yypvt[-5].name);
		temp->assignment_flag = 1;
	        lang_add_assignment(exp_assop('=',yypvt[-5].name,exp_unop('-',
                   exp_binop(CKSUM,exp_intop(0x0),yypvt[-1].etree)))); 
		ldgram_getting_exp=false;
	  } break;
case 90:
# line 310 "ldgram.y"
{
                if (in_defsym) {
                        ldsym_type *temp;
			/* Create a symbol that remembers it was defined
			with a defsym */
                        temp = ldsym_get(yypvt[-2].name);
                        temp->defsym_flag = true;
			/* This will overwrite any prior definition of this
			symbol */
                        lang_add_assignment(exp_assop('=',yypvt[-2].name,yypvt[-0].etree));
                }
                else {
                        ldsym_type *temp;
                        temp = ldsym_get(yypvt[-2].name);
			/* We only generate an assignment statement if
			this symbol was not previously defined by a defsym */
                        if (!temp->defsym_flag) {
                                lang_add_assignment(exp_assop('=',yypvt[-2].name,yypvt[-0].etree));
				temp->assignment_flag = true;
                        }
                }
		ldgram_getting_exp=false;
          } break;
case 92:
# line 334 "ldgram.y"
{
                if (in_defsym) {
                        ldsym_type *temp;
			/* Create a symbol that remembers it was defined
			with a defsym */
                        temp = ldsym_get(yypvt[-2].name);
                        temp->defsym_flag = true;
			/* This will overwrite any prior definition of this
			symbol */
                        lang_add_assignment(exp_assop('=',yypvt[-2].name,
				exp_binop(yypvt[-1].token,exp_nameop(NAME,yypvt[-2].name),yypvt[-0].etree)));
                }
                else {
                        ldsym_type *temp;
                        temp = ldsym_get(yypvt[-2].name);
			/* We only generate an assignment statement if
			this symbol was not previously defined by a defsym */
                        if (!temp->defsym_flag) {
                        	lang_add_assignment(exp_assop('=',yypvt[-2].name,
					exp_binop(yypvt[-1].token,exp_nameop(NAME,yypvt[-2].name),yypvt[-0].etree)));
				temp->assignment_flag = true;
                        }
                }
		ldgram_getting_exp=false;
	  } break;
case 94:
# line 365 "ldgram.y"
{ yyval.etree = exp_binop(CKSUM,yypvt[-0].etree,yypvt[-2].etree);} break;
case 95:
# line 366 "ldgram.y"
{ yyval.etree = yypvt[-0].etree; } break;
case 101:
# line 378 "ldgram.y"
{ region = lang_memory_region_create(yypvt[-0].name);} break;
case 102:
# line 379 "ldgram.y"
{ } break;
case 103:
# line 384 "ldgram.y"
{ region->origin = exp_get_vma(yypvt[-0].etree, 0L,"origin", lang_first_phase_enum); } break;
case 104:
# line 389 "ldgram.y"
{ region->length_lower_32_bits = exp_get_vma(yypvt[-0].etree, ~((bfd_vma)0), "length",
					       lang_first_phase_enum); } break;
case 105:
# line 394 "ldgram.y"
{ lang_set_flags(&region->flags, yypvt[-1].name); } break;
case 107:
# line 399 "ldgram.y"
{ ldemul_hll(yypvt[-0].name); } break;
case 108:
# line 400 "ldgram.y"
{ ldemul_hll(yypvt[-0].name); } break;
case 109:
# line 404 "ldgram.y"
{ ldemul_syslib(yypvt[-0].name); } break;
case 111:
# line 408 "ldgram.y"
{ yyval.etree = exp_unop('-', yypvt[-0].etree); } break;
case 112:
# line 409 "ldgram.y"
{ yyval.etree = yypvt[-1].etree; } break;
case 113:
# line 410 "ldgram.y"
{ yyval.etree = exp_unop('!', yypvt[-0].etree); } break;
case 114:
# line 411 "ldgram.y"
{ yyval.etree = yypvt[-0].etree; } break;
case 115:
# line 412 "ldgram.y"
{ yyval.etree = exp_unop('~', yypvt[-0].etree);} break;
case 116:
# line 413 "ldgram.y"
{ yyval.etree = exp_binop('*', yypvt[-2].etree, yypvt[-0].etree); } break;
case 117:
# line 414 "ldgram.y"
{ yyval.etree = exp_binop('/', yypvt[-2].etree, yypvt[-0].etree); } break;
case 118:
# line 415 "ldgram.y"
{ yyval.etree = exp_binop('%', yypvt[-2].etree, yypvt[-0].etree); } break;
case 119:
# line 416 "ldgram.y"
{ yyval.etree = exp_binop('+', yypvt[-2].etree, yypvt[-0].etree); } break;
case 120:
# line 417 "ldgram.y"
{ yyval.etree = exp_binop('-' , yypvt[-2].etree, yypvt[-0].etree); } break;
case 121:
# line 418 "ldgram.y"
{ yyval.etree = exp_binop(LSHIFT , yypvt[-2].etree, yypvt[-0].etree); } break;
case 122:
# line 419 "ldgram.y"
{ yyval.etree = exp_binop(RSHIFT , yypvt[-2].etree, yypvt[-0].etree); } break;
case 123:
# line 420 "ldgram.y"
{ yyval.etree = exp_binop(EQ , yypvt[-2].etree, yypvt[-0].etree); } break;
case 124:
# line 421 "ldgram.y"
{ yyval.etree = exp_binop(NE , yypvt[-2].etree, yypvt[-0].etree); } break;
case 125:
# line 422 "ldgram.y"
{ yyval.etree = exp_binop(LE , yypvt[-2].etree, yypvt[-0].etree); } break;
case 126:
# line 423 "ldgram.y"
{ yyval.etree = exp_binop(GE , yypvt[-2].etree, yypvt[-0].etree); } break;
case 127:
# line 424 "ldgram.y"
{ yyval.etree = exp_binop('<' , yypvt[-2].etree, yypvt[-0].etree); } break;
case 128:
# line 425 "ldgram.y"
{ yyval.etree = exp_binop('>' , yypvt[-2].etree, yypvt[-0].etree); } break;
case 129:
# line 426 "ldgram.y"
{ yyval.etree = exp_binop('&' , yypvt[-2].etree, yypvt[-0].etree); } break;
case 130:
# line 427 "ldgram.y"
{ yyval.etree = exp_binop('^' , yypvt[-2].etree, yypvt[-0].etree); } break;
case 131:
# line 428 "ldgram.y"
{ yyval.etree = exp_binop('|' , yypvt[-2].etree, yypvt[-0].etree); } break;
case 132:
# line 429 "ldgram.y"
{ yyval.etree = exp_trinop('?' , yypvt[-4].etree, yypvt[-2].etree, yypvt[-0].etree); } break;
case 133:
# line 430 "ldgram.y"
{ yyval.etree = exp_binop(ANDAND , yypvt[-2].etree, yypvt[-0].etree); } break;
case 134:
# line 431 "ldgram.y"
{ yyval.etree = exp_binop(OROR , yypvt[-2].etree, yypvt[-0].etree); } break;
case 135:
# line 432 "ldgram.y"
{ yyval.etree = exp_nameop(DEFINED, yypvt[-1].name); } break;
case 136:
# line 433 "ldgram.y"
{ yyval.etree = exp_intop(yypvt[-0].integer); } break;
case 137:
# line 434 "ldgram.y"
{ yyval.etree = exp_nameop(SIZEOF_HEADERS,0); } break;
case 138:
# line 435 "ldgram.y"
{ yyval.etree = exp_nameop(SIZEOF,yypvt[-1].name); } break;
case 139:
# line 436 "ldgram.y"
{ yyval.etree = exp_nameop(ADDR,yypvt[-1].name); } break;
case 140:
# line 437 "ldgram.y"
{ yyval.etree = exp_unop(ALIGN_K,yypvt[-1].etree); } break;
case 141:
# line 438 "ldgram.y"
{ yyval.etree = exp_nameop(NAME,yypvt[-0].name); } break;
	}
	goto yystack;		/* reset registers in driver code */
}
