
# line 55 "ychexp.y"

#include "defs.h"
#include <ctype.h>
#include "expression.h"
#include "language.h"
#include "value.h"
#include "parser-defs.h"
#include "ch-lang.h"

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror, etc),
   as well as gratuitiously global symbol names, so we can have multiple
   yacc generated parsers in gdb.  Note that these are only the variables
   produced by yacc.  If other parser generators (bison, byacc, etc) produce
   additional global names that conflict at link time, then those parser
   generators need to be fixed instead of adding those names to this list. */

#define	yymaxdepth chill_maxdepth
#define	yyparse	chill_parse
#define	yylex	chill_lex
#define	yyerror	chill_error
#define	yylval	chill_lval
#define	yychar	chill_char
#define	yydebug	chill_debug
#define	yypact	chill_pact
#define	yyr1	chill_r1
#define	yyr2	chill_r2
#define	yydef	chill_def
#define	yychk	chill_chk
#define	yypgo	chill_pgo
#define	yyact	chill_act
#define	yyexca	chill_exca
#define	yyerrflag chill_errflag
#define	yynerrs	chill_nerrs
#define	yyps	chill_ps
#define	yypv	chill_pv
#define	yys	chill_s
#define	yy_yys	chill_yys
#define	yystate	chill_state
#define	yytmp	chill_tmp
#define	yyv	chill_v
#define	yy_yyv	chill_yyv
#define	yyval	chill_val
#define	yylloc	chill_lloc
#define	yyreds	chill_reds		/* With YYDEBUG defined */
#define	yytoks	chill_toks		/* With YYDEBUG defined */

#ifndef YYDEBUG
#define	YYDEBUG	0		/* Default to no yydebug support */
#endif

int
yyparse PARAMS ((void));

static int
yylex PARAMS ((void));

void
yyerror PARAMS ((char *));


# line 120 "ychexp.y"
typedef union 
  {
    LONGEST lval;
    unsigned LONGEST ulval;
    struct {
      LONGEST val;
      struct type *type;
    } typed_val;
    double dval;
    struct symbol *sym;
    struct type *tval;
    struct stoken sval;
    struct ttype tsym;
    struct symtoken ssym;
    int voidval;
    struct block *bval;
    enum exp_opcode opcode;
    struct internalvar *ivar;

    struct type **tvec;
    int *ivec;
  } YYSTYPE;
# define FIXME_01 257
# define FIXME_02 258
# define FIXME_03 259
# define FIXME_04 260
# define FIXME_05 261
# define FIXME_06 262
# define FIXME_07 263
# define FIXME_08 264
# define FIXME_09 265
# define FIXME_10 266
# define FIXME_11 267
# define FIXME_12 268
# define FIXME_13 269
# define FIXME_14 270
# define FIXME_15 271
# define FIXME_16 272
# define FIXME_17 273
# define FIXME_18 274
# define FIXME_19 275
# define FIXME_20 276
# define FIXME_21 277
# define FIXME_22 278
# define FIXME_24 279
# define FIXME_25 280
# define FIXME_26 281
# define FIXME_27 282
# define FIXME_28 283
# define FIXME_29 284
# define FIXME_30 285
# define INTEGER_LITERAL 286
# define BOOLEAN_LITERAL 287
# define CHARACTER_LITERAL 288
# define FLOAT_LITERAL 289
# define GENERAL_PROCEDURE_NAME 290
# define LOCATION_NAME 291
# define SET_LITERAL 292
# define EMPTINESS_LITERAL 293
# define CHARACTER_STRING_LITERAL 294
# define BIT_STRING_LITERAL 295
# define TYPENAME 296
# define FIELD_NAME 297
# define CASE 298
# define OF 299
# define ESAC 300
# define LOGIOR 301
# define ORIF 302
# define LOGXOR 303
# define LOGAND 304
# define ANDIF 305
# define NOTEQUAL 306
# define GTR 307
# define LEQ 308
# define IN 309
# define SLASH_SLASH 310
# define MOD 311
# define REM 312
# define NOT 313
# define POINTER 314
# define RECEIVE 315
# define UP 316
# define IF 317
# define THEN 318
# define ELSE 319
# define FI 320
# define ELSIF 321
# define ILLEGAL_TOKEN 322
# define NUM 323
# define PRED 324
# define SUCC 325
# define ABS 326
# define CARD 327
# define MAX_TOKEN 328
# define MIN_TOKEN 329
# define SIZE 330
# define UPPER 331
# define LOWER 332
# define LENGTH 333
# define GDB_REGNAME 334
# define GDB_LAST 335
# define GDB_VARIABLE 336
# define GDB_ASSIGNMENT 337
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern int yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
YYSTYPE yylval, yyval;
# define YYERRCODE 256

# line 991 "ychexp.y"


/* Implementation of a dynamically expandable buffer for processing input
   characters acquired through lexptr and building a value to return in
   yylval. */

static char *tempbuf;		/* Current buffer contents */
static int tempbufsize;		/* Size of allocated buffer */
static int tempbufindex;	/* Current index into buffer */

#define GROWBY_MIN_SIZE 64	/* Minimum amount to grow buffer by */

#define CHECKBUF(size) \
  do { \
    if (tempbufindex + (size) >= tempbufsize) \
      { \
	growbuf_by_size (size); \
      } \
  } while (0);

/* Grow the static temp buffer if necessary, including allocating the first one
   on demand. */

static void
growbuf_by_size (count)
     int count;
{
  int growby;

  growby = max (count, GROWBY_MIN_SIZE);
  tempbufsize += growby;
  if (tempbuf == NULL)
    {
      tempbuf = (char *) xmalloc (tempbufsize);
    }
  else
    {
      tempbuf = (char *) xrealloc (tempbuf, tempbufsize);
    }
}

/* Try to consume a simple name string token.  If successful, returns
   a pointer to a nullbyte terminated copy of the name that can be used
   in symbol table lookups.  If not successful, returns NULL. */

static char *
match_simple_name_string ()
{
  char *tokptr = lexptr;

  if (isalpha (*tokptr))
    {
      char *result;
      do {
	tokptr++;
      } while (isalnum (*tokptr) || (*tokptr == '_'));
      yylval.sval.ptr = lexptr;
      yylval.sval.length = tokptr - lexptr;
      lexptr = tokptr;
      result = copy_name (yylval.sval);
      for (tokptr = result; *tokptr; tokptr++)
	if (isupper (*tokptr))
	  *tokptr = tolower(*tokptr);
      return result;
    }
  return (NULL);
}

/* Start looking for a value composed of valid digits as set by the base
   in use.  Note that '_' characters are valid anywhere, in any quantity,
   and are simply ignored.  Since we must find at least one valid digit,
   or reject this token as an integer literal, we keep track of how many
   digits we have encountered. */
  
static int
decode_integer_value (base, tokptrptr, ivalptr)
  int base;
  char **tokptrptr;
  int *ivalptr;
{
  char *tokptr = *tokptrptr;
  int temp;
  int digits = 0;

  while (*tokptr != '\0')
    {
      temp = tolower (*tokptr);
      tokptr++;
      switch (temp)
	{
	case '_':
	  continue;
	case '0':  case '1':  case '2':  case '3':  case '4':
	case '5':  case '6':  case '7':  case '8':  case '9':
	  temp -= '0';
	  break;
	case 'a':  case 'b':  case 'c':  case 'd':  case 'e': case 'f':
	  temp -= 'a';
	  temp += 10;
	  break;
	default:
	  temp = base;
	  break;
	}
      if (temp < base)
	{
	  digits++;
	  *ivalptr *= base;
	  *ivalptr += temp;
	}
      else
	{
	  /* Found something not in domain for current base. */
	  tokptr--;	/* Unconsume what gave us indigestion. */
	  break;
	}
    }
  
  /* If we didn't find any digits, then we don't have a valid integer
     value, so reject the entire token.  Otherwise, update the lexical
     scan pointer, and return non-zero for success. */
  
  if (digits == 0)
    {
      return (0);
    }
  else
    {
      *tokptrptr = tokptr;
      return (1);
    }
}

static int
decode_integer_literal (valptr, tokptrptr)
  int *valptr;
  char **tokptrptr;
{
  char *tokptr = *tokptrptr;
  int base = 0;
  int ival = 0;
  int explicit_base = 0;
  
  /* Look for an explicit base specifier, which is optional. */
  
  switch (*tokptr)
    {
    case 'd':
    case 'D':
      explicit_base++;
      base = 10;
      tokptr++;
      break;
    case 'b':
    case 'B':
      explicit_base++;
      base = 2;
      tokptr++;
      break;
    case 'h':
    case 'H':
      explicit_base++;
      base = 16;
      tokptr++;
      break;
    case 'o':
    case 'O':
      explicit_base++;
      base = 8;
      tokptr++;
      break;
    default:
      base = 10;
      break;
    }
  
  /* If we found an explicit base ensure that the character after the
     explicit base is a single quote. */
  
  if (explicit_base && (*tokptr++ != '\''))
    {
      return (0);
    }
  
  /* Attempt to decode whatever follows as an integer value in the
     indicated base, updating the token pointer in the process and
     computing the value into ival.  Also, if we have an explicit
     base, then the next character must not be a single quote, or we
     have a bitstring literal, so reject the entire token in this case.
     Otherwise, update the lexical scan pointer, and return non-zero
     for success. */

  if (!decode_integer_value (base, &tokptr, &ival))
    {
      return (0);
    }
  else if (explicit_base && (*tokptr == '\''))
    {
      return (0);
    }
  else
    {
      *valptr = ival;
      *tokptrptr = tokptr;
      return (1);
    }
}

/*  If it wasn't for the fact that floating point values can contain '_'
    characters, we could just let strtod do all the hard work by letting it
    try to consume as much of the current token buffer as possible and
    find a legal conversion.  Unfortunately we need to filter out the '_'
    characters before calling strtod, which we do by copying the other
    legal chars to a local buffer to be converted.  However since we also
    need to keep track of where the last unconsumed character in the input
    buffer is, we have transfer only as many characters as may compose a
    legal floating point value. */
    
static int
match_float_literal ()
{
  char *tokptr = lexptr;
  char *buf;
  char *copy;
  char ch;
  double dval;
  extern double strtod ();
  
  /* Make local buffer in which to build the string to convert.  This is
     required because underscores are valid in chill floating point numbers
     but not in the string passed to strtod to convert.  The string will be
     no longer than our input string. */
     
  copy = buf = (char *) alloca (strlen (tokptr) + 1);

  /* Transfer all leading digits to the conversion buffer, discarding any
     underscores. */

  while (isdigit (*tokptr) || *tokptr == '_')
    {
      if (*tokptr != '_')
	{
	  *copy++ = *tokptr;
	}
      tokptr++;
    }

  /* Now accept either a '.', or one of [eEdD].  Dot is legal regardless
     of whether we found any leading digits, and we simply accept it and
     continue on to look for the fractional part and/or exponent.  One of
     [eEdD] is legal only if we have seen digits, and means that there
     is no fractional part.  If we find neither of these, then this is
     not a floating point number, so return failure. */

  switch (*tokptr++)
    {
      case '.':
        /* Accept and then look for fractional part and/or exponent. */
	*copy++ = '.';
	break;

      case 'e':
      case 'E':
      case 'd':
      case 'D':
	if (copy == buf)
	  {
	    return (0);
	  }
	*copy++ = 'e';
	goto collect_exponent;
	break;

      default:
	return (0);
        break;
    }

  /* We found a '.', copy any fractional digits to the conversion buffer, up
     to the first nondigit, non-underscore character. */

  while (isdigit (*tokptr) || *tokptr == '_')
    {
      if (*tokptr != '_')
	{
	  *copy++ = *tokptr;
	}
      tokptr++;
    }

  /* Look for an exponent, which must start with one of [eEdD].  If none
     is found, jump directly to trying to convert what we have collected
     so far. */

  switch (*tokptr)
    {
      case 'e':
      case 'E':
      case 'd':
      case 'D':
	*copy++ = 'e';
	tokptr++;
	break;
      default:
	goto convert_float;
	break;
    }

  /* Accept an optional '-' or '+' following one of [eEdD]. */

  collect_exponent:
  if (*tokptr == '+' || *tokptr == '-')
    {
      *copy++ = *tokptr++;
    }

  /* Now copy an exponent into the conversion buffer.  Note that at the 
     moment underscores are *not* allowed in exponents. */

  while (isdigit (*tokptr))
    {
      *copy++ = *tokptr++;
    }

  /* If we transfered any chars to the conversion buffer, try to interpret its
     contents as a floating point value.  If any characters remain, then we
     must not have a valid floating point string. */

  convert_float:
  *copy = '\0';
  if (copy != buf)
      {
        dval = strtod (buf, &copy);
        if (*copy == '\0')
	  {
	    yylval.dval = dval;
	    lexptr = tokptr;
	    return (FLOAT_LITERAL);
	  }
      }
  return (0);
}

/* Recognize a string literal.  A string literal is a nonzero sequence
   of characters enclosed in matching single or double quotes, except that
   a single character inside single quotes is a character literal, which
   we reject as a string literal.  To embed the terminator character inside
   a string, it is simply doubled (I.E. "this""is""one""string") */

static int
match_string_literal ()
{
  char *tokptr = lexptr;

  for (tempbufindex = 0, tokptr++; *tokptr != '\0'; tokptr++)
    {
      CHECKBUF (1);
      if (*tokptr == *lexptr)
	{
	  if (*(tokptr + 1) == *lexptr)
	    {
	      tokptr++;
	    }
	  else
	    {
	      break;
	    }
	}
      tempbuf[tempbufindex++] = *tokptr;
    }
  if (*tokptr == '\0'					/* no terminator */
      || tempbufindex == 0				/* no string */
      || (tempbufindex == 1 && *tokptr == '\''))	/* char literal */
    {
      return (0);
    }
  else
    {
      tempbuf[tempbufindex] = '\0';
      yylval.sval.ptr = tempbuf;
      yylval.sval.length = tempbufindex;
      lexptr = ++tokptr;
      return (CHARACTER_STRING_LITERAL);
    }
}

/* Recognize a character literal.  A character literal is single character
   or a control sequence, enclosed in single quotes.  A control sequence
   is a comma separated list of one or more integer literals, enclosed
   in parenthesis and introduced with a circumflex character.

   EX:  'a'  '^(7)'  '^(7,8)'

   As a GNU chill extension, the syntax C'xx' is also recognized as a 
   character literal, where xx is a hex value for the character.

   Note that more than a single character, enclosed in single quotes, is
   a string literal.

   Also note that the control sequence form is not in GNU Chill since it
   is ambiguous with the string literal form using single quotes.  I.E.
   is '^(7)' a character literal or a string literal.  In theory it it
   possible to tell by context, but GNU Chill doesn't accept the control
   sequence form, so neither do we (for now the code is disabled).

   Returns CHARACTER_LITERAL if a match is found.
   */

static int
match_character_literal ()
{
  char *tokptr = lexptr;
  int ival = 0;
  
  if ((tolower (*tokptr) == 'c') && (*(tokptr + 1) == '\''))
    {
      /* We have a GNU chill extension form, so skip the leading "C'",
	 decode the hex value, and then ensure that we have a trailing
	 single quote character. */
      tokptr += 2;
      if (!decode_integer_value (16, &tokptr, &ival) || (*tokptr != '\''))
	{
	  return (0);
	}
      tokptr++;
    }
  else if (*tokptr == '\'')
    {
      tokptr++;

      /* Determine which form we have, either a control sequence or the
	 single character form. */
      
      if ((*tokptr == '^') && (*(tokptr + 1) == '('))
	{
#if 0     /* Disable, see note above. -fnf */
	  /* Match and decode a control sequence.  Return zero if we don't
	     find a valid integer literal, or if the next unconsumed character
	     after the integer literal is not the trailing ')'.
	     FIXME:  We currently don't handle the multiple integer literal
	     form. */
	  tokptr += 2;
	  if (!decode_integer_literal (&ival, &tokptr) || (*tokptr++ != ')'))
	    {
	      return (0);
	    }
#else
	  return (0);
#endif
	}
      else
	{
	  ival = *tokptr++;
	}
      
      /* The trailing quote has not yet been consumed.  If we don't find
	 it, then we have no match. */
      
      if (*tokptr++ != '\'')
	{
	  return (0);
	}
    }
  else
    {
      /* Not a character literal. */
      return (0);
    }
  yylval.typed_val.val = ival;
  yylval.typed_val.type = builtin_type_chill_char;
  lexptr = tokptr;
  return (CHARACTER_LITERAL);
}

/* Recognize an integer literal, as specified in Z.200 sec 5.2.4.2.
   Note that according to 5.2.4.2, a single "_" is also a valid integer
   literal, however GNU-chill requires there to be at least one "digit"
   in any integer literal. */

static int
match_integer_literal ()
{
  char *tokptr = lexptr;
  int ival;
  
  if (!decode_integer_literal (&ival, &tokptr))
    {
      return (0);
    }
  else 
    {
      yylval.typed_val.val = ival;
      yylval.typed_val.type = builtin_type_int;
      lexptr = tokptr;
      return (INTEGER_LITERAL);
    }
}

/* Recognize a bit-string literal, as specified in Z.200 sec 5.2.4.8
   Note that according to 5.2.4.8, a single "_" is also a valid bit-string
   literal, however GNU-chill requires there to be at least one "digit"
   in any bit-string literal. */

static int
match_bitstring_literal ()
{
  char *tokptr = lexptr;
  int mask;
  int bitoffset = 0;
  int bitcount = 0;
  int base;
  int digit;
  
  tempbufindex = 0;

  /* Look for the required explicit base specifier. */
  
  switch (*tokptr++)
    {
    case 'b':
    case 'B':
      base = 2;
      break;
    case 'o':
    case 'O':
      base = 8;
      break;
    case 'h':
    case 'H':
      base = 16;
      break;
    default:
      return (0);
      break;
    }
  
  /* Ensure that the character after the explicit base is a single quote. */
  
  if (*tokptr++ != '\'')
    {
      return (0);
    }
  
  while (*tokptr != '\0' && *tokptr != '\'')
    {
      digit = tolower (*tokptr);
      tokptr++;
      switch (digit)
	{
	  case '_':
	    continue;
	  case '0':  case '1':  case '2':  case '3':  case '4':
	  case '5':  case '6':  case '7':  case '8':  case '9':
	    digit -= '0';
	    break;
	  case 'a':  case 'b':  case 'c':  case 'd':  case 'e': case 'f':
	    digit -= 'a';
	    digit += 10;
	    break;
	  default:
	    return (0);
	    break;
	}
      if (digit >= base)
	{
	  /* Found something not in domain for current base. */
	  return (0);
	}
      else
	{
	  /* Extract bits from digit, starting with the msbit appropriate for
	     the current base, and packing them into the bitstring byte,
	     starting at the lsbit. */
	  for (mask = (base >> 1); mask > 0; mask >>= 1)
	    {
	      bitcount++;
	      CHECKBUF (1);
	      if (digit & mask)
		{
		  tempbuf[tempbufindex] |= (1 << bitoffset);
		}
	      bitoffset++;
	      if (bitoffset == HOST_CHAR_BIT)
		{
		  bitoffset = 0;
		  tempbufindex++;
		}
	    }
	}
    }
  
  /* Verify that we consumed everything up to the trailing single quote,
     and that we found some bits (IE not just underbars). */

  if (*tokptr++ != '\'')
    {
      return (0);
    }
  else 
    {
      yylval.sval.ptr = tempbuf;
      yylval.sval.length = bitcount;
      lexptr = tokptr;
      return (BIT_STRING_LITERAL);
    }
}

/* Recognize tokens that start with '$'.  These include:

	$regname	A native register name or a "standard
			register name".
			Return token GDB_REGNAME.

	$variable	A convenience variable with a name chosen
			by the user.
			Return token GDB_VARIABLE.

	$digits		Value history with index <digits>, starting
			from the first value which has index 1.
			Return GDB_LAST.

	$$digits	Value history with index <digits> relative
			to the last value.  I.E. $$0 is the last
			value, $$1 is the one previous to that, $$2
			is the one previous to $$1, etc.
			Return token GDB_LAST.

	$ | $0 | $$0	The last value in the value history.
			Return token GDB_LAST.

	$$		An abbreviation for the second to the last
			value in the value history, I.E. $$1
			Return token GDB_LAST.

    Note that we currently assume that register names and convenience
    variables follow the convention of starting with a letter or '_'.

   */

static int
match_dollar_tokens ()
{
  char *tokptr;
  int regno;
  int namelength;
  int negate;
  int ival;

  /* We will always have a successful match, even if it is just for
     a single '$', the abbreviation for $$0.  So advance lexptr. */

  tokptr = ++lexptr;

  if (*tokptr == '_' || isalpha (*tokptr))
    {
      /* Look for a match with a native register name, usually something
	 like "r0" for example. */

      for (regno = 0; regno < NUM_REGS; regno++)
	{
	  namelength = strlen (reg_names[regno]);
	  if (STREQN (tokptr, reg_names[regno], namelength)
	      && !isalnum (tokptr[namelength]))
	    {
	      yylval.lval = regno;
	      lexptr += namelength + 1;
	      return (GDB_REGNAME);
	    }
	}

      /* Look for a match with a standard register name, usually something
	 like "pc", which gdb always recognizes as the program counter
	 regardless of what the native register name is. */

      for (regno = 0; regno < num_std_regs; regno++)
	{
	  namelength = strlen (std_regs[regno].name);
	  if (STREQN (tokptr, std_regs[regno].name, namelength)
	      && !isalnum (tokptr[namelength]))
	    {
	      yylval.lval = std_regs[regno].regnum;
	      lexptr += namelength;
	      return (GDB_REGNAME);
	    }
	}

      /* Attempt to match against a convenience variable.  Note that
	 this will always succeed, because if no variable of that name
	 already exists, the lookup_internalvar will create one for us.
	 Also note that both lexptr and tokptr currently point to the
	 start of the input string we are trying to match, and that we
	 have already tested the first character for non-numeric, so we
	 don't have to treat it specially. */

      while (*tokptr == '_' || isalnum (*tokptr))
	{
	  tokptr++;
	}
      yylval.sval.ptr = lexptr;
      yylval.sval.length = tokptr - lexptr;
      yylval.ivar = lookup_internalvar (copy_name (yylval.sval));
      lexptr = tokptr;
      return (GDB_VARIABLE);
    }

  /* Since we didn't match against a register name or convenience
     variable, our only choice left is a history value. */

  if (*tokptr == '$')
    {
      negate = 1;
      ival = 1;
      tokptr++;
    }
  else
    {
      negate = 0;
      ival = 0;
    }

  /* Attempt to decode more characters as an integer value giving
     the index in the history list.  If successful, the value will
     overwrite ival (currently 0 or 1), and if not, ival will be
     left alone, which is good since it is currently correct for
     the '$' or '$$' case. */

  decode_integer_literal (&ival, &tokptr);
  yylval.lval = negate ? -ival : ival;
  lexptr = tokptr;
  return (GDB_LAST);
}

struct token
{
  char *operator;
  int token;
};

static const struct token idtokentab[] =
{
    { "length", LENGTH },
    { "lower", LOWER },
    { "upper", UPPER },
    { "andif", ANDIF },
    { "pred", PRED },
    { "succ", SUCC },
    { "card", CARD },
    { "size", SIZE },
    { "orif", ORIF },
    { "num", NUM },
    { "abs", ABS },
    { "max", MAX_TOKEN },
    { "min", MIN_TOKEN },
    { "mod", MOD },
    { "rem", REM },
    { "not", NOT },
    { "xor", LOGXOR },
    { "and", LOGAND },
    { "in", IN },
    { "or", LOGIOR }
};

static const struct token tokentab2[] =
{
    { ":=", GDB_ASSIGNMENT },
    { "//", SLASH_SLASH },
    { "->", POINTER },
    { "/=", NOTEQUAL },
    { "<=", LEQ },
    { ">=", GTR }
};

/* Read one token, getting characters through lexptr.  */
/* This is where we will check to make sure that the language and the
   operators used are compatible.  */

static int
yylex ()
{
    unsigned int i;
    int token;
    char *simplename;
    struct symbol *sym;

    /* Skip over any leading whitespace. */
    while (isspace (*lexptr))
	{
	    lexptr++;
	}
    /* Look for special single character cases which can't be the first
       character of some other multicharacter token. */
    switch (*lexptr)
	{
	    case '\0':
	        return (0);
	    case ',':
	    case '=':
	    case ';':
	    case '!':
	    case '+':
	    case '*':
	    case '(':
	    case ')':
	    case '[':
	    case ']':
		return (*lexptr++);
	}
    /* Look for characters which start a particular kind of multicharacter
       token, such as a character literal, register name, convenience
       variable name, string literal, etc. */
    switch (*lexptr)
      {
	case '\'':
	case '\"':
	  /* First try to match a string literal, which is any nonzero
	     sequence of characters enclosed in matching single or double
	     quotes, except that a single character inside single quotes
	     is a character literal, so we have to catch that case also. */
	  token = match_string_literal ();
	  if (token != 0)
	    {
	      return (token);
	    }
	  if (*lexptr == '\'')
	    {
	      token = match_character_literal ();
	      if (token != 0)
		{
		  return (token);
		}
	    }
	  break;
        case 'C':
        case 'c':
	  token = match_character_literal ();
	  if (token != 0)
	    {
	      return (token);
	    }
	  break;
	case '$':
	  token = match_dollar_tokens ();
	  if (token != 0)
	    {
	      return (token);
	    }
	  break;
      }
    /* See if it is a special token of length 2.  */
    for (i = 0; i < sizeof (tokentab2) / sizeof (tokentab2[0]); i++)
	{
	    if (STREQN (lexptr, tokentab2[i].operator, 2))
		{
		    lexptr += 2;
		    return (tokentab2[i].token);
		}
	}
    /* Look for single character cases which which could be the first
       character of some other multicharacter token, but aren't, or we
       would already have found it. */
    switch (*lexptr)
	{
	    case '-':
	    case ':':
	    case '/':
	    case '<':
	    case '>':
		return (*lexptr++);
	}
    /* Look for a float literal before looking for an integer literal, so
       we match as much of the input stream as possible. */
    token = match_float_literal ();
    if (token != 0)
	{
	    return (token);
	}
    token = match_bitstring_literal ();
    if (token != 0)
	{
	    return (token);
	}
    token = match_integer_literal ();
    if (token != 0)
	{
	    return (token);
	}

    /* Try to match a simple name string, and if a match is found, then
       further classify what sort of name it is and return an appropriate
       token.  Note that attempting to match a simple name string consumes
       the token from lexptr, so we can't back out if we later find that
       we can't classify what sort of name it is. */

    simplename = match_simple_name_string ();

    if (simplename != NULL)
      {
	/* See if it is a reserved identifier. */
	for (i = 0; i < sizeof (idtokentab) / sizeof (idtokentab[0]); i++)
	    {
		if (STREQ (simplename, idtokentab[i].operator))
		    {
			return (idtokentab[i].token);
		    }
	    }

	/* Look for other special tokens. */
	if (STREQ (simplename, "true"))
	    {
		yylval.ulval = 1;
		return (BOOLEAN_LITERAL);
	    }
	if (STREQ (simplename, "false"))
	    {
		yylval.ulval = 0;
		return (BOOLEAN_LITERAL);
	    }

	sym = lookup_symbol (simplename, expression_context_block,
			     VAR_NAMESPACE, (int *) NULL,
			     (struct symtab **) NULL);
	if (sym != NULL)
	  {
	    yylval.ssym.stoken.ptr = NULL;
	    yylval.ssym.stoken.length = 0;
	    yylval.ssym.sym = sym;
	    yylval.ssym.is_a_field_of_this = 0;	/* FIXME, C++'ism */
	    switch (SYMBOL_CLASS (sym))
	      {
	      case LOC_BLOCK:
		/* Found a procedure name. */
		return (GENERAL_PROCEDURE_NAME);
	      case LOC_STATIC:
		/* Found a global or local static variable. */
		return (LOCATION_NAME);
	      case LOC_REGISTER:
	      case LOC_ARG:
	      case LOC_REF_ARG:
	      case LOC_REGPARM:
	      case LOC_LOCAL:
	      case LOC_LOCAL_ARG:
		if (innermost_block == NULL
		    || contained_in (block_found, innermost_block))
		  {
		    innermost_block = block_found;
		  }
		return (LOCATION_NAME);
		break;
	      case LOC_CONST:
	      case LOC_LABEL:
		return (LOCATION_NAME);
		break;
	      case LOC_TYPEDEF:
		yylval.tsym.type = SYMBOL_TYPE (sym);
		return TYPENAME;
	      case LOC_UNDEF:
	      case LOC_CONST_BYTES:
	      case LOC_OPTIMIZED_OUT:
		error ("Symbol \"%s\" names no location.", simplename);
		break;
	      }
	  }
	else if (!have_full_symbols () && !have_partial_symbols ())
	  {
	    error ("No symbol table is loaded.  Use the \"file\" command.");
	  }
	else
	  {
	    error ("No symbol \"%s\" in current context.", simplename);
	  }
      }

    /* Catch single character tokens which are not part of some
       longer token. */

    switch (*lexptr)
      {
	case '.':			/* Not float for example. */
	  lexptr++;
	  while (isspace (*lexptr)) lexptr++;
	  simplename = match_simple_name_string ();
	  if (!simplename)
	    return '.';
	  return FIELD_NAME;
      }

    return (ILLEGAL_TOKEN);
}

void
yyerror (msg)
     char *msg;	/* unused */
{
  printf ("Parsing:  %s\n", lexptr);
  if (yychar < 256)
    {
      error ("Invalid syntax in expression near character '%c'.", yychar);
    }
  else
    {
      error ("Invalid syntax in expression");
    }
}
int yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 17,
	40, 121,
	-2, 99,
-1, 130,
	297, 30,
	314, 30,
	40, 30,
	-2, 97,
	};
# define YYNPROD 142
# define YYLAST 753
int yyact[]={

    60,    89,   190,   237,   191,    79,   219,   141,   203,   107,
   252,   102,    87,    88,   108,    98,    94,    96,    84,    85,
    86,   142,   196,    45,    46,    47,    48,   197,   101,    49,
    50,    51,    52,   133,   193,    93,   103,    91,   104,   164,
   227,   165,   231,   225,   159,   160,   140,    90,    38,   200,
   192,    17,    83,   114,     3,   224,   183,    61,   251,   222,
    15,     2,    78,   250,   223,   202,   238,    11,   199,   247,
   246,   229,   228,   189,   228,   245,   244,   243,    60,    39,
   241,   240,   218,    79,   217,   216,   212,   211,   210,   209,
   208,    18,   207,   206,   205,   204,   166,    60,   215,   214,
   213,   125,   124,   123,   122,   121,   120,   119,   118,   117,
   116,   115,   112,   111,   161,     1,    26,     8,   132,   195,
    92,   179,   178,   177,   187,   175,   163,   230,   127,   127,
   127,   162,    55,   131,   226,   158,   157,    54,    43,    42,
    41,    40,   126,   129,     7,   194,     9,     5,   137,   138,
    37,   139,   134,   135,   136,   106,    60,    36,    57,    35,
    34,    79,    33,    83,    32,   153,   154,   155,   156,    31,
    60,   127,    30,    29,   131,    28,   176,   185,   185,    27,
    25,   186,    24,   150,   151,   152,   143,   144,   145,   146,
   147,   148,   149,    16,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,    10,     0,    23,
    53,    56,    58,    59,     0,    83,     0,    62,    63,    64,
    65,    66,     0,     0,    83,     0,     0,     0,     0,   221,
     0,   220,     0,     0,     0,     0,    45,    46,    47,    48,
    44,    19,    49,    50,    51,    52,     6,    60,    14,   232,
     0,    95,    97,    99,   100,   235,     0,     0,   236,     0,
     0,    60,     0,    80,    81,    82,     0,    13,   109,   110,
     0,     0,     0,    67,    68,    69,    70,    71,    72,    73,
    74,    75,    76,    77,    21,    20,    22,    23,    53,    56,
    58,    59,   198,   105,   249,    62,    63,    64,    65,    66,
   248,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,    45,    46,    47,    48,    44,    19,
    49,    50,    51,    52,     6,     0,    14,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,    80,    81,    82,     0,    13,     0,     0,     0,     0,
     0,    67,    68,    69,    70,    71,    72,    73,    74,    75,
    76,    77,    21,    20,    22,    23,    53,    56,    58,    59,
     0,     0,     0,    62,    63,    64,    65,    66,     0,    23,
    53,    56,    58,    59,   180,   181,   182,    62,    63,    64,
    65,    66,    45,    46,    47,    48,    44,    19,    49,    50,
    51,    52,     6,     0,     0,     0,    45,    46,    47,    48,
    44,    19,    49,    50,    51,    52,     6,     0,     0,    80,
    81,    82,     0,     0,     0,     0,     0,     0,     0,    67,
    68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
    21,    20,    22,    67,    68,    69,    70,    71,    72,    73,
    74,    75,    76,    77,    21,    20,    22,     4,     0,     0,
     0,     0,     0,     0,     0,     0,    23,    53,    56,    58,
    59,     0,     0,     0,    62,    63,    64,    65,    66,     0,
    23,    53,    56,    58,    59,     0,     0,     0,    62,    63,
    64,    65,    66,    45,    46,    47,    48,    44,    19,    49,
    50,    51,    52,     6,   128,    12,     0,    45,    46,    47,
    48,    44,    19,    49,    50,    51,    52,     6,   113,     0,
     0,    81,    82,     0,     0,     0,     0,     0,     0,     0,
    67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
    77,    21,    20,    22,    67,    68,    69,    70,    71,    72,
    73,    74,    75,    76,    77,    21,    20,    22,     0,     0,
     0,     0,     0,     0,     0,    12,     0,     0,     0,     0,
     0,     0,     0,   167,   168,   169,   170,   171,   172,   173,
     0,   184,   184,   188,     0,     0,   130,     0,     0,     0,
     0,     0,     0,     0,    12,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,   201,
    12,    12,    12,    12,    12,    12,    12,   174,    12,    12,
    12,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,    12,     0,     0,     0,
     0,   233,   234,   201,     0,     0,     0,     0,     0,     0,
     0,   239,     0,     0,     0,     0,   242,     0,     0,     0,
     0,     0,     0,     0,     0,   239,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,    12,    12,
    12,     0,     0,     0,     0,     0,     0,     0,    12,     0,
     0,     0,     0,    12,     0,     0,     0,     0,     0,     0,
     0,     0,    12 };
int yypact[]={

   -40, -1000, -1000,    57, -1000, -1000, -1000,  -283, -1000, -1000,
 -1000,  -292,  -336,  -244,  -247,   -45, -1000,  -286,    -7, -1000,
 -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000,
 -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000,  -263,   -33,
 -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000,
 -1000, -1000, -1000, -1000,    73,    72, -1000, -1000, -1000, -1000,
    38, -1000, -1000, -1000, -1000, -1000, -1000,    71,    70,    69,
    68,    67,    66,    65,    64,    63,    62,    61, -1000,   217,
   217,   231,  -252, -1000,   116,   116,   116,   116,   116,   -40,
  -311, -1000,  -278, -1000,   116,   116,   116,   116,   116,   116,
   116, -1000, -1000,   116,   116,   116, -1000,   116,   116,   116,
   116,  -228,  -237,    55,    57,    38,    38,    38,    38,    38,
    38,    38,   130,    38,    38,    38, -1000, -1000, -1000, -1000,
 -1000,  -286, -1000, -1000,  -292,  -292,  -292,   -45,   -45, -1000,
  -317,  -249,  -262,    -7,    -7,    -7,    -7,    -7,    -7,    -7,
   -33,   -33,   -33, -1000, -1000, -1000, -1000,   -14,    10, -1000,
 -1000,    38,     7,  -308, -1000, -1000, -1000,    54,    53,    52,
    51,    49,    48,    47,    46,    45,    57,    60,    59,    58,
 -1000, -1000, -1000,    44, -1000,    57,    43,    41, -1000,  -314,
  -249,  -244, -1000, -1000,   -32,     6, -1000, -1000,  -232,  -234,
    30, -1000,  -235,  -232, -1000, -1000, -1000, -1000, -1000, -1000,
 -1000, -1000, -1000,    38,    38,    38, -1000, -1000, -1000, -1000,
 -1000,  -311,  -316,    38,    40, -1000,    39, -1000,    38, -1000,
    36, -1000,    35,    34,    29,    28,  -317,    38,     4, -1000,
 -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000, -1000,   -35,
 -1000,  -290, -1000 };
int yypgo[]={

     0,   514,   193,    51,   182,   180,   116,   179,   175,   173,
   172,   169,   164,   162,   160,   159,   158,   157,   150,    48,
    61,   147,   467,   146,    46,    73,    66,   145,   144,    67,
    60,    91,    79,    57,    62,   141,   140,   139,   138,   137,
   136,   135,   134,    55,   132,    49,   131,   127,   126,   125,
    56,   124,   123,   122,   121,    47,   120,    50,   119,   118,
   117,    53,   115,   114 };
int yyr1[]={

     0,    62,    62,    20,    20,    21,     1,     1,     2,     2,
     2,     2,     2,    45,    45,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     4,     5,     5,     5,     5,     5,     6,     6,     6,     6,
     6,     6,     6,     6,     7,     8,     9,     9,    63,    10,
    11,    11,    12,    13,    14,    15,    17,    18,    19,    22,
    22,    22,    23,    23,    24,    25,    25,    26,    27,    28,
    28,    28,    28,    29,    29,    29,    30,    30,    30,    30,
    30,    30,    30,    30,    31,    31,    31,    31,    32,    32,
    32,    32,    32,    33,    33,    33,    33,    34,    34,    34,
    60,    16,    16,    16,    16,    16,    16,    16,    16,    16,
    16,    16,    16,    49,    49,    49,    49,    61,    50,    50,
    51,    44,    52,    53,    54,    35,    36,    37,    38,    39,
    40,    41,    42,    43,    46,    47,    48,    55,    56,    57,
    58,    59 };
int yyr2[]={

     0,     3,     3,     3,     3,     3,     2,     5,     3,     3,
     3,     3,     3,     3,     7,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     9,    13,    13,     1,    11,
    13,    13,     5,     5,     3,     3,     3,     3,     7,     3,
     3,     3,    11,    19,     5,     5,     9,     3,     9,     3,
     7,     7,     7,     3,     7,     7,     3,     7,     7,     7,
     7,     7,     7,     7,     3,     7,     7,     7,     3,     7,
     7,     7,     7,     3,     5,     5,     5,     5,     5,     3,
     7,     9,     9,     9,     9,     9,     9,     9,     9,     9,
     9,     9,     9,     3,     9,     9,     9,     2,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3 };
int yychk[]={

 -1000,   -62,   -20,   -61,   -22,   -21,   296,   -28,   -60,   -23,
   257,   -29,    -1,   317,   298,   -30,    -2,    -3,   -31,   291,
   335,   334,   336,   259,    -4,    -5,    -6,    -7,    -8,    -9,
   -10,   -11,   -12,   -13,   -14,   -15,   -17,   -18,   -19,   -32,
   -35,   -36,   -37,   -38,   290,   286,   287,   288,   289,   292,
   293,   294,   295,   260,   -39,   -44,   261,   -16,   262,   263,
    40,   -33,   267,   268,   269,   270,   271,   323,   324,   325,
   326,   327,   328,   329,   330,   331,   332,   333,   -34,    45,
   313,   314,   315,   -19,   301,   302,   303,   304,   305,   337,
   -55,   281,   -56,   282,    61,   306,    62,   307,    60,   308,
   309,   314,   297,    43,    45,   310,    -6,    42,    47,   311,
   312,    40,    40,   -22,   -61,    40,    40,    40,    40,    40,
    40,    40,    40,    40,    40,    40,   -34,   -19,    -1,   -34,
    -1,    -3,   -59,   285,   -29,   -29,   -29,   -30,   -30,   -20,
   -24,   318,   299,   -31,   -31,   -31,   -31,   -31,   -31,   -31,
   -32,   -32,   -32,   -33,   -33,   -33,   -33,   -40,   -41,   272,
   273,   -63,   -46,   -48,   276,   278,    41,   -22,   -22,   -22,
   -22,   -22,   -22,   -22,    -1,   -49,   -61,   -52,   -53,   -54,
   264,   265,   266,   -50,   -22,   -61,   -50,   -51,   -22,   -25,
   319,   321,   -57,   283,   -27,   -58,   284,    41,   316,    58,
   -45,   -22,    58,   316,    41,    41,    41,    41,    41,    41,
    41,    41,    41,    40,    40,    40,    41,    41,    41,   320,
   -57,   -55,    91,    58,   -43,   275,   -42,   274,    44,    41,
   -47,   277,   -43,   -22,   -22,   -45,   -24,   319,   -26,   -22,
    41,    41,   -22,    41,    41,    41,    41,    41,   -25,   -26,
    59,    93,   300 };
int yydef[]={

     0,    -2,     1,     2,     3,     4,   117,    59,    60,    61,
     5,    69,    30,     0,     0,    73,     6,    -2,    76,     8,
     9,    10,    11,    12,    15,    16,    17,    18,    19,    20,
    21,    22,    23,    24,    25,    26,    27,    28,    29,    84,
    31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
    41,    42,    43,    44,     0,     0,    54,    55,    56,    57,
     0,    88,   125,   126,   127,   128,   129,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,    93,     0,
     0,     0,     0,    53,     0,     0,     0,     0,     0,     0,
     0,   137,     0,   138,     0,     0,     0,     0,     0,     0,
     0,     7,    52,     0,     0,     0,    96,     0,     0,     0,
     0,     0,    48,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,    94,    29,    30,    95,
    -2,   121,    98,   141,    70,    71,    72,    74,    75,   100,
     0,     0,     0,    77,    78,    79,    80,    81,    82,    83,
    85,    86,    87,    89,    90,    91,    92,     0,     0,   130,
   131,     0,     0,     0,   134,   136,    58,     0,     0,     0,
     0,     0,     0,     0,    30,     0,   113,     0,     0,     0,
   122,   123,   124,     0,   118,   119,     0,     0,   120,     0,
     0,     0,    64,   139,     0,     0,   140,    45,     0,     0,
     0,    13,     0,     0,   101,   102,   103,   104,   105,   106,
   107,   108,   109,     0,     0,     0,   110,   111,   112,    62,
    65,     0,     0,     0,     0,   133,     0,   132,     0,    49,
     0,   135,     0,     0,     0,     0,     0,     0,     0,    67,
    47,    46,    14,    50,    51,   114,   115,   116,    66,     0,
    68,     0,    63 };
typedef struct { char *t_name; int t_val; } yytoktype;
#ifndef YYDEBUG
#	define YYDEBUG	0	/* don't allow debugging */
#endif

#if YYDEBUG

yytoktype yytoks[] =
{
	"FIXME_01",	257,
	"FIXME_02",	258,
	"FIXME_03",	259,
	"FIXME_04",	260,
	"FIXME_05",	261,
	"FIXME_06",	262,
	"FIXME_07",	263,
	"FIXME_08",	264,
	"FIXME_09",	265,
	"FIXME_10",	266,
	"FIXME_11",	267,
	"FIXME_12",	268,
	"FIXME_13",	269,
	"FIXME_14",	270,
	"FIXME_15",	271,
	"FIXME_16",	272,
	"FIXME_17",	273,
	"FIXME_18",	274,
	"FIXME_19",	275,
	"FIXME_20",	276,
	"FIXME_21",	277,
	"FIXME_22",	278,
	"FIXME_24",	279,
	"FIXME_25",	280,
	"FIXME_26",	281,
	"FIXME_27",	282,
	"FIXME_28",	283,
	"FIXME_29",	284,
	"FIXME_30",	285,
	"INTEGER_LITERAL",	286,
	"BOOLEAN_LITERAL",	287,
	"CHARACTER_LITERAL",	288,
	"FLOAT_LITERAL",	289,
	"GENERAL_PROCEDURE_NAME",	290,
	"LOCATION_NAME",	291,
	"SET_LITERAL",	292,
	"EMPTINESS_LITERAL",	293,
	"CHARACTER_STRING_LITERAL",	294,
	"BIT_STRING_LITERAL",	295,
	"TYPENAME",	296,
	"FIELD_NAME",	297,
	".",	46,
	";",	59,
	":",	58,
	"CASE",	298,
	"OF",	299,
	"ESAC",	300,
	"LOGIOR",	301,
	"ORIF",	302,
	"LOGXOR",	303,
	"LOGAND",	304,
	"ANDIF",	305,
	"=",	61,
	"NOTEQUAL",	306,
	">",	62,
	"GTR",	307,
	"<",	60,
	"LEQ",	308,
	"IN",	309,
	"+",	43,
	"-",	45,
	"*",	42,
	"/",	47,
	"SLASH_SLASH",	310,
	"MOD",	311,
	"REM",	312,
	"NOT",	313,
	"POINTER",	314,
	"RECEIVE",	315,
	"[",	91,
	"]",	93,
	"(",	40,
	")",	41,
	"UP",	316,
	"IF",	317,
	"THEN",	318,
	"ELSE",	319,
	"FI",	320,
	"ELSIF",	321,
	"ILLEGAL_TOKEN",	322,
	"NUM",	323,
	"PRED",	324,
	"SUCC",	325,
	"ABS",	326,
	"CARD",	327,
	"MAX_TOKEN",	328,
	"MIN_TOKEN",	329,
	"SIZE",	330,
	"UPPER",	331,
	"LOWER",	332,
	"LENGTH",	333,
	"GDB_REGNAME",	334,
	"GDB_LAST",	335,
	"GDB_VARIABLE",	336,
	"GDB_ASSIGNMENT",	337,
	"-unknown-",	-1	/* ends search */
};

char * yyreds[] =
{
	"-no such reduction-",
	"start : value",
	"start : mode_name",
	"value : expression",
	"value : undefined_value",
	"undefined_value : FIXME_01",
	"location : access_name",
	"location : primitive_value POINTER",
	"access_name : LOCATION_NAME",
	"access_name : GDB_LAST",
	"access_name : GDB_REGNAME",
	"access_name : GDB_VARIABLE",
	"access_name : FIXME_03",
	"expression_list : expression",
	"expression_list : expression_list ',' expression",
	"primitive_value : location_contents",
	"primitive_value : value_name",
	"primitive_value : literal",
	"primitive_value : tuple",
	"primitive_value : value_string_element",
	"primitive_value : value_string_slice",
	"primitive_value : value_array_element",
	"primitive_value : value_array_slice",
	"primitive_value : value_structure_field",
	"primitive_value : expression_conversion",
	"primitive_value : value_procedure_call",
	"primitive_value : value_built_in_routine_call",
	"primitive_value : start_expression",
	"primitive_value : zero_adic_operator",
	"primitive_value : parenthesised_expression",
	"location_contents : location",
	"value_name : synonym_name",
	"value_name : value_enumeration_name",
	"value_name : value_do_with_name",
	"value_name : value_receive_name",
	"value_name : GENERAL_PROCEDURE_NAME",
	"literal : INTEGER_LITERAL",
	"literal : BOOLEAN_LITERAL",
	"literal : CHARACTER_LITERAL",
	"literal : FLOAT_LITERAL",
	"literal : SET_LITERAL",
	"literal : EMPTINESS_LITERAL",
	"literal : CHARACTER_STRING_LITERAL",
	"literal : BIT_STRING_LITERAL",
	"tuple : FIXME_04",
	"value_string_element : string_primitive_value '(' start_element ')'",
	"value_string_slice : string_primitive_value '(' left_element ':' right_element ')'",
	"value_string_slice : string_primitive_value '(' start_element UP slice_size ')'",
	"value_array_element : array_primitive_value '('",
	"value_array_element : array_primitive_value '(' expression_list ')'",
	"value_array_slice : array_primitive_value '(' lower_element ':' upper_element ')'",
	"value_array_slice : array_primitive_value '(' first_element UP slice_size ')'",
	"value_structure_field : primitive_value FIELD_NAME",
	"expression_conversion : mode_name parenthesised_expression",
	"value_procedure_call : FIXME_05",
	"value_built_in_routine_call : chill_value_built_in_routine_call",
	"start_expression : FIXME_06",
	"zero_adic_operator : FIXME_07",
	"parenthesised_expression : '(' expression ')'",
	"expression : operand_0",
	"expression : single_assignment_action",
	"expression : conditional_expression",
	"conditional_expression : IF boolean_expression then_alternative else_alternative FI",
	"conditional_expression : CASE case_selector_list OF value_case_alternative '[' ELSE sub_expression ']' ESAC",
	"then_alternative : THEN subexpression",
	"else_alternative : ELSE subexpression",
	"else_alternative : ELSIF boolean_expression then_alternative else_alternative",
	"sub_expression : expression",
	"value_case_alternative : case_label_specification ':' sub_expression ';'",
	"operand_0 : operand_1",
	"operand_0 : operand_0 LOGIOR operand_1",
	"operand_0 : operand_0 ORIF operand_1",
	"operand_0 : operand_0 LOGXOR operand_1",
	"operand_1 : operand_2",
	"operand_1 : operand_1 LOGAND operand_2",
	"operand_1 : operand_1 ANDIF operand_2",
	"operand_2 : operand_3",
	"operand_2 : operand_2 '=' operand_3",
	"operand_2 : operand_2 NOTEQUAL operand_3",
	"operand_2 : operand_2 '>' operand_3",
	"operand_2 : operand_2 GTR operand_3",
	"operand_2 : operand_2 '<' operand_3",
	"operand_2 : operand_2 LEQ operand_3",
	"operand_2 : operand_2 IN operand_3",
	"operand_3 : operand_4",
	"operand_3 : operand_3 '+' operand_4",
	"operand_3 : operand_3 '-' operand_4",
	"operand_3 : operand_3 SLASH_SLASH operand_4",
	"operand_4 : operand_5",
	"operand_4 : operand_4 '*' operand_5",
	"operand_4 : operand_4 '/' operand_5",
	"operand_4 : operand_4 MOD operand_5",
	"operand_4 : operand_4 REM operand_5",
	"operand_5 : operand_6",
	"operand_5 : '-' operand_6",
	"operand_5 : NOT operand_6",
	"operand_5 : parenthesised_expression literal",
	"operand_6 : POINTER location",
	"operand_6 : RECEIVE buffer_location",
	"operand_6 : primitive_value",
	"single_assignment_action : location GDB_ASSIGNMENT value",
	"chill_value_built_in_routine_call : NUM '(' expression ')'",
	"chill_value_built_in_routine_call : PRED '(' expression ')'",
	"chill_value_built_in_routine_call : SUCC '(' expression ')'",
	"chill_value_built_in_routine_call : ABS '(' expression ')'",
	"chill_value_built_in_routine_call : CARD '(' expression ')'",
	"chill_value_built_in_routine_call : MAX_TOKEN '(' expression ')'",
	"chill_value_built_in_routine_call : MIN_TOKEN '(' expression ')'",
	"chill_value_built_in_routine_call : SIZE '(' location ')'",
	"chill_value_built_in_routine_call : SIZE '(' mode_argument ')'",
	"chill_value_built_in_routine_call : UPPER '(' upper_lower_argument ')'",
	"chill_value_built_in_routine_call : LOWER '(' upper_lower_argument ')'",
	"chill_value_built_in_routine_call : LENGTH '(' length_argument ')'",
	"mode_argument : mode_name",
	"mode_argument : array_mode_name '(' expression ')'",
	"mode_argument : string_mode_name '(' expression ')'",
	"mode_argument : variant_structure_mode_name '(' expression_list ')'",
	"mode_name : TYPENAME",
	"upper_lower_argument : expression",
	"upper_lower_argument : mode_name",
	"length_argument : expression",
	"array_primitive_value : primitive_value",
	"array_mode_name : FIXME_08",
	"string_mode_name : FIXME_09",
	"variant_structure_mode_name : FIXME_10",
	"synonym_name : FIXME_11",
	"value_enumeration_name : FIXME_12",
	"value_do_with_name : FIXME_13",
	"value_receive_name : FIXME_14",
	"string_primitive_value : FIXME_15",
	"start_element : FIXME_16",
	"left_element : FIXME_17",
	"right_element : FIXME_18",
	"slice_size : FIXME_19",
	"lower_element : FIXME_20",
	"upper_element : FIXME_21",
	"first_element : FIXME_22",
	"boolean_expression : FIXME_26",
	"case_selector_list : FIXME_27",
	"subexpression : FIXME_28",
	"case_label_specification : FIXME_29",
	"buffer_location : FIXME_30",
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
	yyv = (YYSTYPE*)xmalloc(yymaxdepth*sizeof(YYSTYPE));
	yys = (int*)xmalloc(yymaxdepth*sizeof(int));
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
			** xreallocate and recover.  Note that pointers
			** have to be reset, or bad things will happen
			*/
			int yyps_index = (yy_ps - yys);
			int yypv_index = (yy_pv - yyv);
			int yypvt_index = (yypvt - yyv);
			yymaxdepth += YYMAXDEPTH;
			yyv = (YYSTYPE*)xrealloc((char*)yyv,
				yymaxdepth * sizeof(YYSTYPE));
			yys = (int*)xrealloc((char*)yys,
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
		
case 1:
# line 312 "ychexp.y"
{ } break;
case 2:
# line 314 "ychexp.y"
{ write_exp_elt_opcode(OP_TYPE);
			  write_exp_elt_type(yypvt[-0].tsym.type);
			  write_exp_elt_opcode(OP_TYPE);} break;
case 3:
# line 320 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 4:
# line 324 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 5:
# line 330 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 7:
# line 339 "ychexp.y"
{
			  write_exp_elt_opcode (UNOP_IND);
			} break;
case 8:
# line 347 "ychexp.y"
{
			  write_exp_elt_opcode (OP_VAR_VALUE);
			  write_exp_elt_sym (yypvt[-0].ssym.sym);
			  write_exp_elt_opcode (OP_VAR_VALUE);
			} break;
case 9:
# line 353 "ychexp.y"
{
			  write_exp_elt_opcode (OP_LAST);
			  write_exp_elt_longcst (yypvt[-0].lval);
			  write_exp_elt_opcode (OP_LAST); 
			} break;
case 10:
# line 359 "ychexp.y"
{
			  write_exp_elt_opcode (OP_REGISTER);
			  write_exp_elt_longcst (yypvt[-0].lval);
			  write_exp_elt_opcode (OP_REGISTER); 
			} break;
case 11:
# line 365 "ychexp.y"
{
			  write_exp_elt_opcode (OP_INTERNALVAR);
			  write_exp_elt_intern (yypvt[-0].ivar);
			  write_exp_elt_opcode (OP_INTERNALVAR); 
			} break;
case 12:
# line 371 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 13:
# line 379 "ychexp.y"
{
			  arglist_len = 1;
			} break;
case 14:
# line 383 "ychexp.y"
{
			  arglist_len++;
			} break;
case 15:
# line 390 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 16:
# line 394 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 17:
# line 398 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 18:
# line 402 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 19:
# line 406 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 20:
# line 410 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 21:
# line 414 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 22:
# line 418 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 23:
# line 422 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 24:
# line 426 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 25:
# line 430 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 26:
# line 434 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 27:
# line 438 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 28:
# line 442 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 29:
# line 446 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 30:
# line 454 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 31:
# line 462 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 32:
# line 466 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 33:
# line 470 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 34:
# line 474 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 35:
# line 478 "ychexp.y"
{
			  write_exp_elt_opcode (OP_VAR_VALUE);
			  write_exp_elt_sym (yypvt[-0].ssym.sym);
			  write_exp_elt_opcode (OP_VAR_VALUE);
			} break;
case 36:
# line 488 "ychexp.y"
{
			  write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (yypvt[-0].typed_val.type);
			  write_exp_elt_longcst ((LONGEST) (yypvt[-0].typed_val.val));
			  write_exp_elt_opcode (OP_LONG);
			} break;
case 37:
# line 495 "ychexp.y"
{
			  write_exp_elt_opcode (OP_BOOL);
			  write_exp_elt_longcst ((LONGEST) yypvt[-0].ulval);
			  write_exp_elt_opcode (OP_BOOL);
			} break;
case 38:
# line 501 "ychexp.y"
{
			  write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (yypvt[-0].typed_val.type);
			  write_exp_elt_longcst ((LONGEST) (yypvt[-0].typed_val.val));
			  write_exp_elt_opcode (OP_LONG);
			} break;
case 39:
# line 508 "ychexp.y"
{
			  write_exp_elt_opcode (OP_DOUBLE);
			  write_exp_elt_type (builtin_type_double);
			  write_exp_elt_dblcst (yypvt[-0].dval);
			  write_exp_elt_opcode (OP_DOUBLE);
			} break;
case 40:
# line 515 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 41:
# line 519 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 42:
# line 523 "ychexp.y"
{
			  write_exp_elt_opcode (OP_STRING);
			  write_exp_string (yypvt[-0].sval);
			  write_exp_elt_opcode (OP_STRING);
			} break;
case 43:
# line 529 "ychexp.y"
{
			  write_exp_elt_opcode (OP_BITSTRING);
			  write_exp_bitstring (yypvt[-0].sval);
			  write_exp_elt_opcode (OP_BITSTRING);
			} break;
case 44:
# line 539 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 45:
# line 548 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 46:
# line 556 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 47:
# line 560 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 48:
# line 570 "ychexp.y"
{ start_arglist (); } break;
case 49:
# line 572 "ychexp.y"
{
			  write_exp_elt_opcode (MULTI_SUBSCRIPT);
			  write_exp_elt_longcst ((LONGEST) end_arglist ());
			  write_exp_elt_opcode (MULTI_SUBSCRIPT);
			} break;
case 50:
# line 582 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 51:
# line 586 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 52:
# line 594 "ychexp.y"
{ write_exp_elt_opcode (STRUCTOP_STRUCT);
			  write_exp_string (yypvt[-0].sval);
			  write_exp_elt_opcode (STRUCTOP_STRUCT);
			} break;
case 53:
# line 603 "ychexp.y"
{
			  write_exp_elt_opcode (UNOP_CAST);
			  write_exp_elt_type (yypvt[-1].tsym.type);
			  write_exp_elt_opcode (UNOP_CAST);
			} break;
case 54:
# line 613 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 55:
# line 621 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 56:
# line 629 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 57:
# line 637 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 58:
# line 645 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 59:
# line 653 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 60:
# line 657 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 61:
# line 661 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 62:
# line 667 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 63:
# line 671 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 64:
# line 677 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 65:
# line 683 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 66:
# line 687 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 67:
# line 693 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 68:
# line 699 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 69:
# line 707 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 70:
# line 711 "ychexp.y"
{
			  write_exp_elt_opcode (BINOP_BITWISE_IOR);
			} break;
case 71:
# line 715 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 72:
# line 719 "ychexp.y"
{
			  write_exp_elt_opcode (BINOP_BITWISE_XOR);
			} break;
case 73:
# line 727 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 74:
# line 731 "ychexp.y"
{
			  write_exp_elt_opcode (BINOP_BITWISE_AND);
			} break;
case 75:
# line 735 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 76:
# line 743 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 77:
# line 747 "ychexp.y"
{
			  write_exp_elt_opcode (BINOP_EQUAL);
			} break;
case 78:
# line 751 "ychexp.y"
{
			  write_exp_elt_opcode (BINOP_NOTEQUAL);
			} break;
case 79:
# line 755 "ychexp.y"
{
			  write_exp_elt_opcode (BINOP_GTR);
			} break;
case 80:
# line 759 "ychexp.y"
{
			  write_exp_elt_opcode (BINOP_GEQ);
			} break;
case 81:
# line 763 "ychexp.y"
{
			  write_exp_elt_opcode (BINOP_LESS);
			} break;
case 82:
# line 767 "ychexp.y"
{
			  write_exp_elt_opcode (BINOP_LEQ);
			} break;
case 83:
# line 771 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 84:
# line 780 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 85:
# line 784 "ychexp.y"
{
			  write_exp_elt_opcode (BINOP_ADD);
			} break;
case 86:
# line 788 "ychexp.y"
{
			  write_exp_elt_opcode (BINOP_SUB);
			} break;
case 87:
# line 792 "ychexp.y"
{
			  write_exp_elt_opcode (BINOP_CONCAT);
			} break;
case 88:
# line 800 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 89:
# line 804 "ychexp.y"
{
			  write_exp_elt_opcode (BINOP_MUL);
			} break;
case 90:
# line 808 "ychexp.y"
{
			  write_exp_elt_opcode (BINOP_DIV);
			} break;
case 91:
# line 812 "ychexp.y"
{
			  write_exp_elt_opcode (BINOP_MOD);
			} break;
case 92:
# line 816 "ychexp.y"
{
			  write_exp_elt_opcode (BINOP_REM);
			} break;
case 93:
# line 824 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 94:
# line 828 "ychexp.y"
{
			  write_exp_elt_opcode (UNOP_NEG);
			} break;
case 95:
# line 832 "ychexp.y"
{
			  write_exp_elt_opcode (UNOP_LOGICAL_NOT);
			} break;
case 96:
# line 838 "ychexp.y"
{
			  write_exp_elt_opcode (BINOP_CONCAT);
			} break;
case 97:
# line 846 "ychexp.y"
{
			  write_exp_elt_opcode (UNOP_ADDR);
			} break;
case 98:
# line 850 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 99:
# line 854 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 100:
# line 864 "ychexp.y"
{
			  write_exp_elt_opcode (BINOP_ASSIGN);
			} break;
case 101:
# line 873 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 102:
# line 877 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 103:
# line 881 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 104:
# line 885 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 105:
# line 889 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 106:
# line 893 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 107:
# line 897 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 108:
# line 901 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 109:
# line 905 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 110:
# line 909 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 111:
# line 913 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 112:
# line 917 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 113:
# line 923 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 114:
# line 927 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 115:
# line 931 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 116:
# line 935 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 118:
# line 944 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 119:
# line 948 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 120:
# line 954 "ychexp.y"
{
			  yyval.voidval = 0;	/* FIXME */
			} break;
case 121:
# line 962 "ychexp.y"
{
			  yyval.voidval = 0;
			} break;
case 122:
# line 970 "ychexp.y"
{ yyval.voidval = 0; } break;
case 123:
# line 971 "ychexp.y"
{ yyval.voidval = 0; } break;
case 124:
# line 972 "ychexp.y"
{ yyval.voidval = 0; } break;
case 125:
# line 973 "ychexp.y"
{ yyval.voidval = 0; } break;
case 126:
# line 974 "ychexp.y"
{ yyval.voidval = 0; } break;
case 127:
# line 975 "ychexp.y"
{ yyval.voidval = 0; } break;
case 128:
# line 976 "ychexp.y"
{ yyval.voidval = 0; } break;
case 129:
# line 977 "ychexp.y"
{ yyval.voidval = 0; } break;
case 130:
# line 978 "ychexp.y"
{ yyval.voidval = 0; } break;
case 131:
# line 979 "ychexp.y"
{ yyval.voidval = 0; } break;
case 132:
# line 980 "ychexp.y"
{ yyval.voidval = 0; } break;
case 133:
# line 981 "ychexp.y"
{ yyval.voidval = 0; } break;
case 134:
# line 982 "ychexp.y"
{ yyval.voidval = 0; } break;
case 135:
# line 983 "ychexp.y"
{ yyval.voidval = 0; } break;
case 136:
# line 984 "ychexp.y"
{ yyval.voidval = 0; } break;
case 137:
# line 985 "ychexp.y"
{ yyval.voidval = 0; } break;
case 138:
# line 986 "ychexp.y"
{ yyval.voidval = 0; } break;
case 139:
# line 987 "ychexp.y"
{ yyval.voidval = 0; } break;
case 140:
# line 988 "ychexp.y"
{ yyval.voidval = 0; } break;
case 141:
# line 989 "ychexp.y"
{ yyval.voidval = 0; } break;
	}
	goto yystack;		/* reset registers in driver code */
}
