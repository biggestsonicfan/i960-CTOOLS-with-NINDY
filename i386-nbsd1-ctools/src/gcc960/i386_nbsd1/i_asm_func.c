#include "config.h"

#ifdef IMSTG
#include <stdio.h>
#include "tree.h"
#include "rtl.h"
#include "input.h"
#include "c-lex.h"
#include "c-parse.h"
#include "insn-config.h"
#include "assert.h"
#include "expr.h"
#include "i_lister.h"

#define isalnum(char) ((char >= 'a' && char <= 'z') || (char >= 'A' && char <= 'Z') || (char >= '0' && char <= '9'))
#define isdigit(char) (char >= '0' && char <= '9')

extern int dollars_in_ident;
extern char *extend_token_buffer();

extern rtx get_parm_rtx ();

int in_asm_ctrl = -1;

typedef enum
{
  AC_NOMATCH,
  AC_OK,
  AC_CVT_IF,
  AC_CVT_FI,
  AC_MOVE
} asm_coercion;

typedef enum
{
  ASM_NONE,
  ASM_VOID, ASM_FIRST_PCLASS=ASM_VOID,
  ASM_LABEL,
  ASM_CONST,
  ASM_REGLIT, ASM_FIRST_REG=ASM_REGLIT,
  ASM_FREGLIT,
  ASM_TMPREG,
  ASM_FTMPREG, ASM_LAST_PCLASS=ASM_FTMPREG, ASM_LAST_REG=ASM_FTMPREG,
  ASM_PURE,
  ASM_SPILLALL,
  ASM_USE,
  ASM_CALL,
  ASM_ERROR,
  ASM_RETURN,
  ASM_LAST_RESERVED = ASM_RETURN,
} asm_keywords;

typedef struct
{
  tree tok;
  unsigned char pclass;
  unsigned char parm_num;
  unsigned char varno;
  int size;
  int lo;
  int hi;
  int const_kind;
  int ali;
  int bsiz;
  enum machine_mode mode;
  int vclass;
  char* cspec;
} asm_term;

typedef struct _asm_info
{
  char* asm_call;
  char* asm_template;
  int   asm_call_message;
  int   asm_error_line;
  int   in_asm_template;
  int   asm_template_alloc;
  int   asm_error;
  int   asm_pure;
  int   asm_spillall;
  int   asm_max_table;
  int   asm_varno;
  int   asm_labno;
  int   asm_nparms;
  int   asm_is_cond;
  unsigned asm_iuses;
  unsigned asm_fuses;
  int   asm_num_uses;
  int   asm_num_var[4];
  asm_term asm_table[255];
  int   asm_parm_map[255];
  int   asm_var_map[MAX_RECOG_OPERANDS];
  int   asm_too_many_decls;
  struct _asm_info *next;
} asm_info;

static enum machine_mode
asm_mode_table[] = { VOIDmode, SImode, DImode, TImode, TImode, VOIDmode };

static enum machine_mode
asm_fmode_table[] = { VOIDmode, SFmode, DFmode, VOIDmode, TFmode, VOIDmode };


static asm_info *ai,*ai_head;
static int  asm_id_class;
static int  asm_id_size;
static int  asm_id_lo;
static int  asm_id_hi;
static int  asm_id_const_kind;
static int  asm_dead_warning;

static int
get_asm_id (t,addit,p)
tree t;
int addit;
asm_info *p;
{ /*  Look up t in this leaf's symbol table.  */
  int i;

  assert (t);
  i = p->asm_max_table;

  while (i--)
    if (p->asm_table[i].tok == t)
      return i;

  if (!addit)
    return -1;

  p->asm_table[p->asm_max_table].tok = t;
  return p->asm_max_table++;
}

int
get_asm_ctrl_id (s,p)
char *s;
YYSTYPE *p;
{ /* Called from yylex() for identifiers when in in_asm_ctrl==1.
     Hides the asm symbol table mechanism from yylex(). */

  p->itype = get_asm_id (get_identifier(s),1,ai);
  return IMSTG_ASM_IDENTIFIER;
}

int
asm_ctrl_key(arg1)
{
  /* Called from c-parse.y when we parse a single identifier
     with no constant expression or range following it.
  */

  asm_id_class = 0;
  asm_id_size  = 0;
  asm_id_lo    = 0;
  asm_id_hi    = 0;
  asm_id_const_kind = 0;

  switch (arg1)
  {
    default:
      error ("'%s' is not a legal asm control",
              IDENTIFIER_POINTER(ai->asm_table[arg1].tok));
      break;

    case ASM_PURE:
      ai->asm_pure = 1;
      break;

    case ASM_SPILLALL:
      ai->asm_spillall = 1;
      break;

    case ASM_CALL:
    { char *s = IDENTIFIER_POINTER(DECL_NAME(current_function_decl));
      if (ai->asm_call && strcmp (ai->asm_call, s))
        error ("this leaf cannot call '%s'; it already calls '%s'", s, ai->asm_call);
      else
        ai->asm_call = s;
      break;
    }

    case ASM_ERROR:
      if (ai->asm_error)
        error ("cannot have more than 1 error control per asm control");
      else
        ai->asm_error = 1;
      break;
  }
}


int
check_number (t,pv,real_ok)
tree t;
int *pv;
int real_ok;
{
  /* Check out the number in t, report an error and return
     0 if it isn't the right kind of thing.  Return 'i' or 'f'
     appropriately, if it is integer or float.
  */

  int kind = 0;
  int v;

  while (TREE_CODE(t)==NOP_EXPR && TREE_TYPE(t)==TREE_TYPE(TREE_OPERAND(t,0)))
    t = TREE_OPERAND(t,0);

  t = fold (t);

  if (TREE_CODE(t) == REAL_CST)
    if (real_ok)
    { if (tree_real_cst_to_int (t, &v) && (v==0 || v==1))
        kind = 'f';
      else
        error ("floating point number in asm specification must be 0.0 or 1.0");
    }
    else
      error ("integer expected in asm specification");

  else if (TREE_CODE(t)==INTEGER_CST)
  { int i = TREE_INT_CST_LOW(t);
    if ((i < 0 && TREE_INT_CST_HIGH(t) != -1) ||
        (i >= 0 && TREE_INT_CST_HIGH(t) != 0))
      error ("integer constant in asm specification is too big");
    else
    { v = i;
      kind = 'n';
    }
  }
  else
    error ("illegal number in asm specification");

  if (kind)
    *pv = v;

  return kind;
}

int
asm_ctrl_class (arg1, size)
int arg1;
tree size;
{
  /*  Called from c-parse.y when we see the identifier that
      is supposed to start a control. */

  int n = 1;

  asm_id_class = 0;
  asm_id_size  = 0;
  asm_id_lo    = 0;
  asm_id_hi    = 0;
  asm_id_const_kind = 0;

  switch (arg1)
  {
    default:
      error ("an asm control cannot start with '%s'",
              IDENTIFIER_POINTER(ai->asm_table[arg1].tok));
      break;

    case ASM_PURE:
    case ASM_SPILLALL:
    case ASM_RETURN:
    case ASM_ERROR:
      error ("asm control '%s' cannot have arguments",
              IDENTIFIER_POINTER(ai->asm_table[arg1].tok));
      break;

    case ASM_VOID:
    case ASM_LABEL:
    case ASM_CALL:
    case ASM_USE:
    case ASM_FREGLIT:
    case ASM_FTMPREG:
      if (size)
        error ("expression is not allowed after '%s'",
                IDENTIFIER_POINTER(ai->asm_table[arg1].tok));
      else
        if (!(TARGET_NUMERICS) && (arg1==ASM_FREGLIT||arg1==ASM_FTMPREG))
          error ("asm parameter class '%s' is legal only with hardware floating point",
                IDENTIFIER_POINTER(ai->asm_table[arg1].tok));
        else
          asm_id_class = arg1;

      break;

    case ASM_REGLIT:
    case ASM_TMPREG:
      asm_id_class = arg1;
      asm_id_size  = 1;

      if (size)
        if (TREE_CODE(size) == TREE_LIST)
        { asm_id_class = 0;
          asm_id_size  = 0;
          error ("range is not allowed after '%s'",
                 IDENTIFIER_POINTER(ai->asm_table[arg1].tok));
        }
        else
        { int kind,v;

          if (kind = check_number (size,&v,0))
            if (kind != 'n' || (v != 1 && v != 2 && v != 3 && v != 4))
            { asm_id_class = 0;
              asm_id_size  = 0;
              error ("illegal asm size specification");
            }
            else
              if (v == 3)
                asm_id_size = 4;
              else
                asm_id_size = v;
        }
      break;

    case ASM_CONST:
      asm_id_class = arg1;
      
      if (size)
      {
        tree tlo, thi;
        int k1,k2,lo,hi;

        if (TREE_CODE(size) == TREE_LIST)
        { tlo = TREE_PURPOSE(size);
          thi = TREE_VALUE(size);
          k1  = check_number (tlo,&lo,1);
          k2  = check_number (thi,&hi,1);
        }
        else
        { tlo = size;
          k1  = check_number (tlo,&lo,1);
          k2  = k1;
          thi = tlo;
          hi = lo;
        }

        if (k1 && k2)
          if (lo>hi)
            error ("low bound of asm range specification exceeds high bound");
          else
            if (!(TARGET_NUMERICS) && (k1=='f' || k2=='f'))
              error ("asm class 'const' with floating point expression is not legal with software floating point");
            else
            { asm_id_const_kind = (k1=='f' || k2=='f') ?'f' :'n';
              asm_id_lo = lo;
              asm_id_hi = hi;
            }
      }
      break;
  }
}

int
asm_ctrl_class_id (arg2)
int arg2;
{
  /* Called from c-parse.y when we have the id following a
     parameter class. */

  int c = asm_id_class;
  int n = asm_id_size;

  if (c >= ASM_FIRST_PCLASS && c <= ASM_LAST_PCLASS)
  { int i;

    if (i=ai->asm_table[arg2].pclass)
      error ("asm identifier '%s' is already declared as '%s'",
              IDENTIFIER_POINTER(ai->asm_table[arg2].tok),
              IDENTIFIER_POINTER(ai->asm_table[i].tok));
    else
    {
      if (arg2 == ASM_RETURN)
      {
        if (c == ASM_CONST || c == ASM_LABEL)
        { error ("asm identifier 'return' cannot be declared as '%s'",
                 IDENTIFIER_POINTER(ai->asm_table[c].tok));
          c = 0;
        }
        else if (c != ASM_VOID && ai->asm_table[ASM_RETURN].mode == VOIDmode)
        { error ("asm identifier 'return' cannot be declared '%s' in void function",
                 IDENTIFIER_POINTER(ai->asm_table[c].tok));
          c = 0;
        }
      }
      else
      {
        if (c == ASM_VOID)
        { error ("asm class 'void' can only be used for 'return'");
          c = 0;
        }
        else if (c == ASM_LABEL && ai->asm_table[arg2].parm_num)
        { error ("asm parameter '%s' cannot be declared as 'label'",
                 IDENTIFIER_POINTER(ai->asm_table[arg2].tok));
          c = 0;
        }
      }

      if (c)
        if (n && ai->asm_table[arg2].size && (n != ai->asm_table[arg2].size))
          error ("size of asm parameter '%s' must be (%d)",
                  IDENTIFIER_POINTER(ai->asm_table[arg2].tok),ai->asm_table[arg2].size);
        else
          if ((c==ASM_FREGLIT||c==ASM_FTMPREG) && ai->asm_table[arg2].mode
          &&  (GET_MODE_CLASS(ai->asm_table[arg2].mode) != MODE_FLOAT))
          { error ("asm parameter '%s' is of inappropriate type for class '%s'",
                   IDENTIFIER_POINTER(ai->asm_table[arg2].tok),
                   IDENTIFIER_POINTER(ai->asm_table[c].tok));
            c=(c==ASM_FREGLIT) ? ASM_REGLIT : ASM_TMPREG;
          }

      if (c)
        if (arg2 != ASM_RETURN && ai->asm_table[arg2].parm_num==0)
          if (c==ASM_REGLIT)
          { warning ("assuming 'tmpreg' for 'reglit' asm temporary '%s'",
                     IDENTIFIER_POINTER(ai->asm_table[arg2].tok));
            c=ASM_TMPREG;
          }
          else if (c==ASM_FREGLIT)
          { warning ("assuming 'ftmpreg' for 'freglit' asm temporary '%s'",
                     IDENTIFIER_POINTER(ai->asm_table[arg2].tok));
            c=ASM_FTMPREG;
          }
          else if (c==ASM_CONST)
          { error ("asm temporary '%s' cannot be 'const'",
                    IDENTIFIER_POINTER(ai->asm_table[arg2].tok));
            c=0;
          }

      if (c)
      { /* OK. We got a good class for this declaration. */

        ai->asm_table[arg2].pclass = c;

        if (ai->asm_table[arg2].size == 0)
          ai->asm_table[arg2].size = n;
        else
          n = ai->asm_table[arg2].size;
  
        /* Make sure all register declarations have a size and a mode.
           The only ones which should be sizeless at this point (besides void
           RETURNs, which stay sizeless), are local declarations
           which are ASM_FTMPREG.  ASM_TMPREG locals have a size.
           Neither flavor will have a mode yet. */
           
        if (arg2 != ASM_RETURN && c >= ASM_FIRST_REG && c <= ASM_LAST_REG &&
            (n==0 || ai->asm_table[arg2].mode==0))
        { enum machine_mode m;
          assert (!ai->asm_table[arg2].parm_num && !ai->asm_table[arg2].mode);
  
          if (n==0)
          { assert (c==ASM_FTMPREG);
            m = TFmode;
            n = 4;
          }
          else
          { assert (c==ASM_TMPREG);
            assert (n==1 || n==2 || n==4);
            m = asm_mode_table[n];
          }
          ai->asm_table[arg2].mode = m;
          ai->asm_table[arg2].size = n;
        }
  
        ai->asm_table[arg2].const_kind = asm_id_const_kind;
        ai->asm_table[arg2].lo = asm_id_lo;
        ai->asm_table[arg2].hi = asm_id_hi;
      }
    }
  }
  else
    switch (c)
    {
      default:
        assert (0);
        break;

      case 0:
        break;

      case ASM_CALL:
      { char *s = IDENTIFIER_POINTER(ai->asm_table[arg2].tok);
        if (ai->asm_call && strcmp (ai->asm_call,s))
          error ("this leaf cannot call '%s'; it already calls '%s'", s, ai->asm_call);
        else
          ai->asm_call = s;
        break;
      }

      case ASM_USE:
      {
        char *p;
        int c,r,err;

        r = -1;
        p = IDENTIFIER_POINTER(ai->asm_table[arg2].tok);
        c = *p++;
        
        if (c=='f' && p[0]=='p' && isdigit(p[1]) && p[2]=='\0')
        { r = p[1]-'0';
          if (r < 4)
            ai->asm_fuses |= (1 << r);
          else
            r = -1;
        }

        else
        {
          if ((c=='g' || c=='r') && isdigit(*p))
          { r = *p++ - '0';
            if (isdigit(*p))
              r = r*10 + *p++ - '0';
            if (*p != '\0')
              r = -1;
            else
              if (c=='r')
                r += 16;
          }

          /* These restrictions are from the ic960 manual */

          if((r>= 0 && r <32)
          && (r!=16 && r!=17 && r!=18)	/* pfp,rip */
          && (r!=14)			/* g14 */
          && (r!=FRAME_POINTER_REGNUM)
          && (r!=STACK_POINTER_REGNUM))
            ai->asm_iuses |= (1 << r);
          else
            r = -1;
        }

        if (r == -1)
        { ai->asm_num_uses = -1;
          error ("'%s' is not legal in an asm  'use' directive",
                  IDENTIFIER_POINTER(ai->asm_table[arg2].tok));
        }
        else
          if (ai->asm_num_uses != -1)
            ai->asm_num_uses++;
        break;
      }
    }
}

void
asm_saw_percent()
{
  /* We are about to start up a new asm leaf.  Allocate a
     new leaf entry and initialize it. */

  asm_info *p;
  tree t,type;
  int i,n,b;

  assert (in_asm_ctrl == -1);

  in_asm_ctrl = 0;

  p = (asm_info*) xmalloc (sizeof (asm_info));
  bzero (p, sizeof (*p));

  if (ai == 0)
    ai_head = ai = p;
  else
  {
    if (ai->asm_is_cond)
      ai->next = p;
    else
    {
      if (asm_dead_warning == 0)
      { warning ("this asm control line (and any that follow) can never match");
        asm_dead_warning = 1;
      }
      ai->next = 0;
    }
  }

  ai = p;

  p->asm_table[ASM_VOID].tok = get_identifier("void");
  p->asm_table[ASM_CONST].tok = get_identifier("const");
  p->asm_table[ASM_REGLIT].tok = get_identifier("reglit");
  p->asm_table[ASM_FREGLIT].tok = get_identifier("freglit");
  p->asm_table[ASM_TMPREG].tok = get_identifier("tmpreg");
  p->asm_table[ASM_FTMPREG].tok = get_identifier("ftmpreg");
  p->asm_table[ASM_PURE].tok = get_identifier("pure");
  p->asm_table[ASM_SPILLALL].tok = get_identifier("spillall");
  p->asm_table[ASM_USE].tok = get_identifier("use");
  p->asm_table[ASM_LABEL].tok = get_identifier("label");
  p->asm_table[ASM_CALL].tok = get_identifier("call");
  p->asm_table[ASM_ERROR].tok = get_identifier("error");
  p->asm_table[ASM_RETURN].tok = get_identifier("return");

  p->asm_max_table = ASM_LAST_RESERVED+1;

  i = ASM_RETURN;
  t = DECL_RESULT(current_function_decl);

  type = TREE_TYPE(t);

  if (TYPE_MODE(type) == VOIDmode)
    b = 0;
  else
  {
    b = int_size_in_bytes(type);
    assert (b > 0);
    p->asm_table[i].bsiz = b;
    b = i960_object_bytes_bitalign(b)/32;
  }

  p->asm_table[i].size = b;
  p->asm_table[i].ali  = TYPE_ALIGN(type) / BITS_PER_UNIT;

  if ((p->asm_table[i].mode = TYPE_MODE(type)) == BLKmode)
    p->asm_table[i].mode = asm_mode_table[MIN(b,4)];
  assert (p->asm_table[i].mode != VOIDmode || b == 0);
  assert (p->asm_table[i].mode != BLKmode);

  p->asm_parm_map[0] = i;

  t = DECL_ARGUMENTS(current_function_decl);
  n = 0;

  while (t)
  {
    if (TREE_CODE(t) == PARM_DECL)
    {
      i = get_asm_id (DECL_NAME(t),1,p);
      p->asm_table[i].parm_num = ++n;

      type = TREE_TYPE(t);

      /* The normal parameter matching stuff will be called on the actual
         argument list before we try to expand an asm function call.  So,
         make the formal parameter mode correspond to what the actual mode
         will be at the call.  I.e, accept ordinals smaller than int only
         if !(TARGET_CLEAN_LINKAGE). */

      if (TREE_CODE (type) == INTEGER_TYPE && PROMOTE_PROTOTYPES
      && (TYPE_PRECISION (type) < TYPE_PRECISION (integer_type_node)))
        type = integer_type_node;

      b = int_size_in_bytes(type);
      p->asm_table[i].bsiz = b;
      b = i960_object_bytes_bitalign(b)/32;

      p->asm_table[i].size = b;
      p->asm_table[i].ali  = TYPE_ALIGN(type) / BITS_PER_UNIT;

      if ((p->asm_table[i].mode = TYPE_MODE(type)) == BLKmode)
        p->asm_table[i].mode = asm_mode_table[MIN(b,4)];
      assert (p->asm_table[i].mode != VOIDmode);
      assert (p->asm_table[i].mode != BLKmode);

      p->asm_parm_map[n] = i;
    }
    t = TREE_CHAIN(t);
  }
  p->asm_nparms = n;
  p->in_asm_template = -1;
}

static int
asm_yylex()
{
  /* This routine is used for tokenizing ic960 asm templates.  We
     have to tokenize the input stream so that we can do the identifier
     substitution exactly the same way ic960 would.

     Mostly, we care about scanning past identifiers,numbers, character
     constants, and strings in the same manner as ic960 would.  It is
     generally harmless to be inaccurate about other things.

     We report no errors.
  */

  char *p;
  int c, value;

  c = getc (finput);

  p = token_buffer;
  if (p >= token_buffer + get_maxtoken())
    p = extend_token_buffer (p);

  *p++ = value = c;

  switch (c)
  {
    default:
      break;

    case 0:
      /* Don't make yyparse think this is eof.  */
      p[-1] = value = 1;
      break;

    case EOF:
      p[-1] = value = EOF;
      break;

    case '$':
      if (dollars_in_ident)
        goto letter;
      break;
      
    case 'L':	/* May start a wide-string or wide-character constant. */

      if (p >= token_buffer + get_maxtoken())
        p = extend_token_buffer (p);

      *p++ = value = c = getc (finput);

      if (c == '\'' || c == '"')
      {
        case '\'':
        case '"' :

        while ((c=getc(finput))!=value && c != EOF && c != '\n')
        { if (p >= token_buffer + get_maxtoken())
            p = extend_token_buffer (p);

          if ((*p++ = c) == '\\')
          { if ((c=getc(finput))==EOF || c=='\n') 
              break;
          
            if (p >= token_buffer + get_maxtoken())
              p = extend_token_buffer (p);
            *p++ = c;
          }
        }

        if (c != value)
          ungetc (c,finput);
        else
        { if (p >= token_buffer + get_maxtoken())
            p = extend_token_buffer (p);

          *p++ = c;
        }
        break;
      }
      else
      { ungetc (c, finput);
        p--;
        /* Fall thru to letter */
      }

    case '_':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K':           case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
    case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
    case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
    case 'v': case 'w': case 'x': case 'y': case 'z': 
    letter:

      value = IDENTIFIER;
      while ((c=getc(finput))=='_' ||(c=='$' && dollars_in_ident) || isalnum(c))
      { if (p >= token_buffer + get_maxtoken())
          p = extend_token_buffer (p);
        *p++ = c;
      }

      ungetc (c,finput);
      break;

    case '0':  case '1':  case '2':  case '3':  case '4':
    case '5':  case '6':  case '7':  case '8':  case '9':

      value = CONSTANT;
      while ((c=getc(finput))=='_' ||(c=='.') || isalnum(c))
      { if (p >= token_buffer + get_maxtoken())
          p = extend_token_buffer (p);
        *p++ = c;
      }

      ungetc (c,finput);
      break;
  }

  if (p >= token_buffer + get_maxtoken())
    p = extend_token_buffer (p);
  *p++ = 0;
  return value;
}

static void
append_ch (c,p)
int c;
asm_info *p;
{ /* Append a character to p's asm template. */
  assert (p->asm_template_alloc > 0 && p->asm_template != 0);

  if (p->asm_template_alloc < (p->in_asm_template+2))
  { p->asm_template_alloc = p->in_asm_template + 255;
    p->asm_template = xrealloc (p->asm_template, p->asm_template_alloc);
  }

  p->asm_template[p->in_asm_template++] = c;
  p->asm_template[p->in_asm_template] = '\0';
}

static void
append_st (s,p)
char *s;
asm_info *p;
{ /* Append s */
  int c,start;

  while ((c = *s++) != '\0')
    append_ch (c,p);
}

static int
echo_while (s,p)
char *s;
asm_info *p;
{
  /* While the next input character isn't in 's',
     echo it to the asm template and read another. */

  int c,start;
  
  c = getc(finput);
  while (index (s,c) != 0)
  { append_ch (c,p);
    c = getc (finput);
  }

  return c;
}

static void
trunc_in (mark,p)
int mark;
asm_info *p;
{ assert (mark >= 0 && p->in_asm_template >= 0);
  assert (mark <= p->in_asm_template);
  p->asm_template[p->in_asm_template = mark] = '\0';
}

static int
mark_in (p)
asm_info *p;
{ return p->in_asm_template;
}

static int
swallow_asm_line (p)
asm_info *p;
{
  int c,start;

  /* We have seen a newline to start the asm template
     part of the asm leaf.  If the line is a #line,
     hand it off to check_newline for processing;
     otherwise, append it to p->asm_template */

  lineno++;
  INC_LIST_LINE;

  start = mark_in (p);
  c = echo_while (" \t", p);

  if (c=='%' || c=='}' || c=='#')
  {
    /* Get rid of the white space we just ate... */
    trunc_in (start,p);

    if (c == '#')
    { /* check_newline will bump the counter again and read the # */
      lineno--;
      DEC_LIST_LINE;
      ungetc (c, finput);
      c = check_newline();
    }
  }
  else
  { 
    advance_tok_info();

    ungetc (c,finput);

    while ((c = asm_yylex ()) != '\n' && c != EOF)
    { int id = 0;
      int  v = 0;
      char vbuf[255];

      if (p->asm_call || p->asm_error)
      { 
        if (p->asm_call)
          if (p->asm_call_message == 0)
          { error ("an asm template cannot follow an asm 'call' control");
            p->asm_call_message = 1;
          }

        if (p->asm_error)
          if (p->asm_error_line == 0)
          {
            p->in_asm_template = 0;
            while (c != '\n' && c != EOF)
            { append_st (token_buffer, p);
              c = asm_yylex();
            }
            ungetc (c,finput);
            p->asm_error_line++;
          }
          else
            if (p->asm_error_line == 1)
            { warning ("only 1 message line can follow an asm 'error' control");
              p->asm_error_line++;
            }
      }

      else if (c==IDENTIFIER &&
          ((id=get_asm_id(get_identifier(token_buffer),0,p))!=-1) &&
          ((v = p->asm_table[id].varno-1) != -1 || id==ASM_RETURN))
      {
        if (v == -1)
        { assert (id == ASM_RETURN && p->asm_table[id].pclass==ASM_VOID);
          error ("asm identifier 'return' cannot appear in this asm template");
        }

        else if (p->asm_table[id].pclass == ASM_LABEL)
        {
          /* Translate labels to the gnu local label mechanism.  */

          append_st (".A%=",p);
          append_ch (v/100 + '0',p);
          v %= 100;
          append_ch (v/10 + '0',p);
          v %= 10;
          append_ch (v%10 + '0',p);
        }
        else if (p->asm_table[id].pclass == ASM_TMPREG ||
                 p->asm_table[id].pclass == ASM_REGLIT)
        {
          int reread = 1;
          int regnum = 0;

          append_ch ('%',p);
          start = mark_in (p);
          sprintf (vbuf, "%d", v);
          append_st (vbuf, p);

          /* Translate reg(offset) to %D,%T,or %X. */

          if ((c = echo_while (" \t", p)) == '(')
          { append_ch ('(',p);
            c = echo_while (" \t", p);
            if (isdigit(c))
            {
              /* This is very arbitrary.  ic960 seems to require that once
                 (d is seen, the specification must be legal;  otherwise,
                 there is no message, and no substitution is tried.
                 'legal' means a single decimal digit, 0..nwords-1 */

              append_ch (c,p);
              regnum = c - '0';
              if ((c = echo_while (" \t", p)) == ')')
              { append_ch (')',p);
                reread = 0;
                if (regnum >= 0 && regnum < p->asm_table[id].size)
                {
                  trunc_in(start,p);

                  if (regnum != 0)
                    append_ch ("DTX"[regnum-1],p);

                  sprintf (vbuf,"%d",v);
                  append_st (vbuf,p);
                }
                else
                  error ("illegal register offset '%d' in asm template",regnum);
              }
              else
                error ("')' must follow register offset digit in asm template");
            }
          }
          if (reread)
            ungetc (c,finput);
        }

        else if (p->asm_table[id].pclass == ASM_FTMPREG ||
                 p->asm_table[id].pclass == ASM_FREGLIT ||
                 p->asm_table[id].pclass == ASM_CONST)
        { sprintf(vbuf, "%%%d", v);
          append_st (vbuf,p);
        }
  
        else
          append_st (token_buffer,p);
      }
      else
        append_st (token_buffer,p);
    }

    append_ch ('\n',p);
  }
  return c;
}

static void
cspec (p,i,k)
asm_info *p;
{
  /* Assign a variable number and constraint string to
     the ith entry in this asm leafs' symbol table.  The entry
     is known to be a variable or parameter.

     If k==1, the variable is strictly a definition;
     If k==2, it is both a use and a definition
              (TMPREG or FTMPREG which is also a parameter;
     If 3, it is strictly a use.
  */

  char* s;
  int c,l,n;

  if (p->asm_too_many_decls)
    return;

  /* Does the current number of asm operands (counting plus this one) exceed
     MAX_RECOG_OPERANDS ?  If so, gcc base will die horribly.  The
     total number of asm operands is defs + uses + 2 * (uses + defs) */

  n = p->asm_var_map[1] +p->asm_var_map[3] +(2* p->asm_var_map[2]) +1 +(k==2);

  if (n > MAX_RECOG_OPERANDS)
  { p->asm_too_many_decls = 1;
    error ("too many asm register or constant declarations");
    return;
  }

  /* Is the highest numbered matching constraint (required for k==2) greater
     than a single digit ?  Gcc base can't handle this yet */

  if ((k==2 || p->asm_var_map[2]) &&
      ((p->asm_num_var[1]+p->asm_num_var[2]+(k==2)) >10))
  { p->asm_too_many_decls = 1;
    error ("more than 10 asm tmpreg declarations");
    return;
  }

  s = (char *) xmalloc (12);
  c = p->asm_table[i].pclass;
  l = 0;

  p->asm_var_map[p->asm_varno++] = i;
  p->asm_num_var[k]++;
  p->asm_table[i].varno  = p->asm_varno;
  p->asm_table[i].cspec  = s;
  p->asm_table[i].vclass = k;

  if (k < 3)
  { s[l++] = '='; s[l++] = '&'; }

  if (c==ASM_FREGLIT||c==ASM_FTMPREG)
  { s[l++] = 'f';
    if (c==ASM_FREGLIT)
    { s[l++]='G'; s[l++] = 'H'; }
  }
  else if (c==ASM_REGLIT || c==ASM_TMPREG)
  { int n = p->asm_table[i].size-1;
    assert (n==0 || n==1 || n==3);
    s[l++] = "dt?q"[n];
    if (c==ASM_REGLIT)
      s[l++] = 'I';
  }
  else if (c==ASM_CONST)
  {
    s[l++]='n';
    if (TARGET_NUMERICS)
    { s[l++]='G';
      s[l++]='H';
    }
    s[l++]='\0';
  }
  else
    assert (0);
  s[l] = 0;
}

int
asm_saw_nl ()
{
  /* We have parsed the control line.  Make explicit any remaining
     default semantics (e.g, declare those parms which were not declared)
     and then parse the asm template.
  */

  int c,i,n;

  assert (in_asm_ctrl != -1);
  assert (ai->asm_labno==0 && ai->asm_varno==0 && ai->asm_template==0 && ai->asm_template_alloc==0);
  in_asm_ctrl = -1;

  if (ai->asm_error || ai->asm_call)
  {
    char *s = 0;

    if (ai->asm_error && ai->asm_call)
    { s = "'error' or 'call'";
      error ("cannot specify both 'error' and 'call' in an asm control");
    }

    else if (ai->asm_error)
      s = "'error'";

    else
        s = "'call'";

    if (ai->asm_spillall || ai->asm_num_uses || ai->asm_labno)
      error ("'spillall', 'use', or 'label' specified in %s asm control", s);
  }
  else
  {
    /* Make default declarations for parameters ... */
    for (i=1; i <= ai->asm_nparms; i++)
      if (ai->asm_table[ai->asm_parm_map[i]].pclass == 0)
        ai->asm_table[ai->asm_parm_map[i]].pclass = ASM_REGLIT;

    /* Make default declaration for return */
    if ((c=ai->asm_table[ASM_RETURN].pclass) == 0)
      if (ai->asm_table[ASM_RETURN].size > 0)
        c = ASM_TMPREG;
      else
        c = ASM_VOID;
    else
      if (c != ASM_VOID)
        if (c == ASM_FREGLIT)
          c = ASM_FTMPREG;
        else
          c = ASM_TMPREG;
    
    ai->asm_table[ASM_RETURN].pclass = c;
  }
  
  /* Decide if this leaf is conditional or not.  The last
     conditional leaf is special because it is subject to
     coercion, and any leaves past the first unconditional 
     leaf must be called out to the user as unmatchable. */

  for (i = 0; i < ai->asm_max_table; i++)
    if (ai->asm_table[i].pclass != 0 && (i==ASM_RETURN||ai->asm_table[i].parm_num))
      ai->asm_is_cond = 1;

  /* Assign variable numbers first to outputs, then input-outputs,
     then inputs.  This ordering is important! */

  /* outputs are TMPREGS which are not parms ... */
  for (i = 0; i < ai->asm_max_table; i++)
    if (ai->asm_table[i].parm_num==0 &&
        ((c=ai->asm_table[i].pclass) == ASM_TMPREG || c == ASM_FTMPREG))
      cspec (ai,i,1);

  /* input-outputs are TMPREGS which are parms ... */
  for (i = 0; i < ai->asm_max_table; i++)
    if (ai->asm_table[i].parm_num!=0 &&
        ((c=ai->asm_table[i].pclass) == ASM_TMPREG || c == ASM_FTMPREG))
      cspec (ai,i,2);

  /* inputs are REGLIT and CONST parms ... */
  for (i = 0; i < ai->asm_max_table; i++)
    if (ai->asm_table[i].parm_num!=0 &&
        ((c=ai->asm_table[i].pclass)==ASM_REGLIT||c==ASM_FREGLIT||c==ASM_CONST))
      cspec (ai,i,3);

  /* Assign variable numbers for all local labels. */
  for (i = 0; i < ai->asm_max_table; i++)
    if (ai->asm_table[i].pclass == ASM_LABEL)
      ai->asm_table[i].varno = ++ai->asm_labno;
      
  ai->asm_template_alloc = 255;
  ai->asm_template = (char *) xmalloc (ai->asm_template_alloc);
  ai->asm_template[0] = '\n';
  ai->asm_template[1] = '\0';
  ai->in_asm_template = 1;

  /* Swallow the asm template ... */
  while ((c=swallow_asm_line(ai)) == '\n')
    ;

  ai->in_asm_template = -1;

  /* Push % or } back onto the input */
  ungetc (c, finput);
}

int
check_asm_parms()
{
  /* We have seen the asm declarator and parameter list.
     Check out the funtion for basic suitability as an
     asm function.  The stuff reported here is stuff
     we want reported exactly once per function def, as
     opposed to once per asm leaf.
  */

  tree t,p;
  int b,x;

  reinit_parse_for_function ();
  store_parm_decls ();

  t = TREE_TYPE(current_function_decl);

  if (TYPE_ARG_TYPES(t) == 0)
    error("asm function definition must have a function prototype");

  if (TYPE_ARG_TYPES(t) != 0
  && TREE_VALUE(tree_last(TYPE_ARG_TYPES(t))) != void_type_node)
    error("asm function definition cannot have a variable number of arguments");

  t  = DECL_RESULT(current_function_decl);

  if (TYPE_MODE(TREE_TYPE(t)) != VOIDmode)
  { b = int_size_in_bytes (TREE_TYPE(t));
    if (b > 16)
    { error ("asm function return value must be 16 bytes or smaller");
      assert (TYPE_MODE(TREE_TYPE(t)) == BLKmode);
    }
  }

  t = DECL_ARGUMENTS(current_function_decl);

  if (t && (p=tree_last(t)) && DECL_NAME (p) && 
            !strcmp (IDENTIFIER_POINTER(DECL_NAME (p)), "__builtin_va_alist"))
    error("asm function definition cannot have a variable number of arguments");

  while (t)
  { int b;

    if (TREE_CODE(t) == PARM_DECL)
    { b = int_size_in_bytes (TREE_TYPE(t));
      if (b > 16)
      { error ("asm function parameter '%s' must be 16 bytes or smaller",
               IDENTIFIER_POINTER(DECL_NAME(t)));
        assert (TYPE_MODE(TREE_TYPE(t)) == BLKmode);
      }
    }

    t = TREE_CHAIN(t);
  }
}

int
start_asm_function (declspecs, declarator, nested)
tree declarator, declspecs;
int nested;
{
  int ret;

  assert (ai == 0);

  if (ret = start_function (declspecs, declarator, nested))
  {
    if (nested)
    { error ("nested asm functions are not allowed");
      ret = 0;
    }
    else
    {
    }
  }
  return ret;
}

int
clear_asm_state ()
{
  ai = ai_head = 0;
  in_asm_ctrl = -1;
  asm_dead_warning = 0;
}

int
finish_asm_function (nested)
int nested;
{
  /* Finish processing this asm function declaration.
     We have to do some normal bookkeeping pulled
     from the regular 'finish_function', and we
     make the asm function look like an inline function
     whose body has already been generated.

     In place of the saved insns, we store the saved
     information which will allow us to match and
     generate code for the proper leaf. */

  register tree fndecl = current_function_decl;

  /* This is bookkeeping stolen form finish_function */
  poplevel (1, 0, 1);
  BLOCK_SUPERCONTEXT (DECL_INITIAL (fndecl)) = fndecl;

  /* Must mark the RESULT_DECL as being in this function.  */

  DECL_CONTEXT (DECL_RESULT (fndecl)) = fndecl;

  /* This is the good stuff */
  SET_TREE_ASM_FUNCTION(fndecl);
  DECL_SAVED_INSNS(fndecl) = (rtx) ai_head;
  TREE_ASM_WRITTEN(fndecl) = 1;

  clear_asm_state();

  /* More stolen bookkeeping */

  /* Free all the tree nodes making up this function.  */
  /* Switch back to allocating nodes permanently
     until we start another function.  */

  if (!nested)
    permanent_allocation (1);

  /* Stop pointing to the local nodes about to be freed.  */
  /* But DECL_INITIAL must remain nonzero so we know this
     was an actual function definition.  */

  DECL_INITIAL (fndecl) = error_mark_node;
  DECL_ARGUMENTS (fndecl) = 0;

  if (! nested)
    /* Let the error reporting routines know that we're outside a
       function.  For a nested function, this value is used in
       pop_c_function_context and then reset via pop_function_context.  */
      current_function_decl = NULL;
}

static int
match_reg (v,m,var_ok)
rtx v;
enum machine_mode m;
int var_ok;
{
  /* If v is a pseudo register suitable for use directly
     as an asm argument, return 1.  'var_ok' means that
     the register won't get destroyed by the asm, or that
     it is actually the result of the asm.
  */

  assert (m != VOIDmode);

  if (m != GET_MODE(v))
    return 0;

  if (GET_CODE(v) == SUBREG)
    v = SUBREG_REG(v);

  return GET_CODE(v)==REG && REGNO(v)>LAST_VIRTUAL_REGISTER &&
         (var_ok || !REG_USERVAR_P(v));
}

asm_coercion
match_parm (i,args,info)
int i;
char* args;
asm_info *info;
{
  /* Try to match the ith parameter of the current call
     to its constraint, and return an enumeration indicating
     what has to be done to make it match. */

  int ret,p,c,d,n,o;
  rtx v;
  enum machine_mode m;

  assert (i < info->asm_nparms);
  v = get_parm_rtx (i, args);
  assert (v != 0);

  m = GET_MODE(v);
  p = info->asm_parm_map[i+1];
  c = info->asm_table[p].pclass;
  ret = AC_NOMATCH;

  if (d=(GET_CODE(v)==CONST_DOUBLE && GET_MODE_CLASS(m)==MODE_FLOAT))
  { o = 0;
    if (IS_CONST_ZERO_RTX(v))
      n = 0;
    else	/* Only accept floating consts which are really integers */
      d = const_double_to_int (v, &n);
  }
  else
    if (o=(GET_CODE(v)==CONST_INT))
      n = INTVAL(v);
    else
      if (o=(GET_CODE(v)==CONST_DOUBLE &&
          (m==VOIDmode || GET_MODE_CLASS(m)==MODE_INT)))
      { n = CONST_DOUBLE_LOW(v);
        if (CONST_DOUBLE_HIGH(v) != -(n < 0))
          o = 0;
      }

  switch (c)
  {
    default:
      assert (0);
      break;

    case ASM_CONST:
      if (d | o)
        if (info->asm_table[p].const_kind == 0)
          /* No constant range specified.  Accept 0.0, 1.0, or integer value */
          if (d)
            if ((TARGET_NUMERICS) && (n==0 || n==1))
              ret = AC_OK;
            else
              ret = AC_CVT_FI;
          else
            ret = AC_OK;

        else
          if (n >= info->asm_table[p].lo && n <= info->asm_table[p].hi)
            if (info->asm_table[p].const_kind == 'n')
              ret = d ? AC_CVT_FI : AC_OK;
            else if (info->asm_table[p].const_kind == 'f')
            { assert ((TARGET_NUMERICS));
              ret = d ? AC_OK : AC_CVT_IF;
            }
            else
              assert (0);
      break;

    case ASM_TMPREG:
    case ASM_FTMPREG:
      if (match_reg(v,info->asm_table[p].mode,0))
        ret = AC_OK;
      else
        ret = AC_MOVE;
      break;

    case ASM_REGLIT:
      if (match_reg(v,info->asm_table[p].mode,1) || (o && n>=0 && n < 32))
        ret = AC_OK;
      else
        if (d && n>=0 && n < 32)
          ret = AC_CVT_FI;
        else
          ret = AC_MOVE;
      break;

    case ASM_FREGLIT:
      if (match_reg(v,info->asm_table[p].mode,1) || (d && n>=0 && n < 2))
        ret = AC_OK;
      else
        if (o && n>=0 && n < 2)
          ret = AC_CVT_IF;
        else
          ret = AC_MOVE;
      break;
  }
  return ret;
}

static rtx
expand_asm_func (p,struct_size,args,target)
asm_info *p;
int struct_size;
char *args;
rtx target;
{
  /* We have selected 'p' as the appropriate asm leaf
     for a call to an ic960 asm function.

     Emit the rtl into the current sequence to load any
     parameters which need loading, and then issue
     appropriate rtl for the asm itself.

     'args' is the argument information for the current
     function being called;  the format of this information is
     private to calls.c.  We use the get_parm_rtx(args,i)
     to retrieve the rtl for the ith parameter to the current call.
  */

  int nclobbers,ninputs,noutputs,i,j,n,v,def_only,use_only,def_use;
  unsigned u;
  char *string,*c;
  rtx body,insn,l,r,t,output_rtx[MAX_RECOG_OPERANDS];
  rtvec argvec, constraints;
  asm_coercion ac;
  enum machine_mode m,jm;

  char *file = input_filename;
  int line   = lineno;

  bzero (output_rtx, sizeof(output_rtx));

  nclobbers = ((p->asm_num_uses<0) ? 0 : p->asm_num_uses) + p->asm_spillall;
  string    = p->asm_template;

  def_only = p->asm_num_var[1];
  def_use  = p->asm_num_var[2];
  use_only = p->asm_num_var[3];

  noutputs  = def_only + def_use;
  ninputs   = use_only + def_use;

  argvec      = rtvec_alloc (ninputs);
  constraints = rtvec_alloc (ninputs);

  body =gen_rtx(ASM_OPERANDS,VOIDmode,string,"",0,argvec,constraints,file,line);
  MEM_VOLATILE_P(body) = !p->asm_pure;

  i = 0;
  if (p->asm_table[ASM_RETURN].varno == 1 && target != 0 &&
      match_reg(target,p->asm_table[ASM_RETURN].mode, 1))
    output_rtx[i++] = target;

  while (i < def_only)
  { j = p->asm_var_map[i];
    output_rtx[i++] = gen_reg_rtx (p->asm_table[j].mode);
  }

  for (i=0; i < ninputs; i++)
  {
    if (i < use_only)
    { j = p->asm_var_map[noutputs+i];
      c = p->asm_table[j].cspec;
    }
    else
      j = p->asm_var_map[def_only + (i - use_only)];

    v = p->asm_table[j].varno-1;
    assert (v >= def_only);

    n = p->asm_table[j].parm_num - 1;
    assert (n >= 0);

    r = get_parm_rtx (n,args);
    m = GET_MODE(r);
    ac = match_parm(n,args,p);

    switch (ac)
    {
      default:
        assert (0);
        break;

      case AC_OK:
        break;

      case AC_CVT_IF:
      { REAL_VALUE_TYPE hv;
        if (GET_CODE(r)==CONST_INT)
        { assert (INTVAL(r) >= 0 && INTVAL(r) < 2);
#ifdef IMSTG_REAL
          REAL_VALUE_FROM_INT (hv, INTVAL(r), 0);
#else
          hv = INTVAL(r);
#endif
          r = immed_real_const_1 (hv, DFmode);
        }
        else if (GET_CODE(r)==CONST_DOUBLE)
        { assert (GET_MODE_CLASS(m)==MODE_INT||m==VOIDmode);
          assert (CONST_DOUBLE_LOW(r) >= 0 && CONST_DOUBLE_LOW(r) < 2);
          assert (CONST_DOUBLE_HIGH(r) == 0);
#ifdef IMSTG_REAL
          REAL_VALUE_FROM_INT (hv, CONST_DOUBLE_LOW(r), 0);
#else
          hv = CONST_DOUBLE_LOW(r);
#endif
          r = immed_real_const_1 (hv, DFmode);
        }
        else
          assert (0);
        break;
      }

      case AC_CVT_FI:
      { int d,n;
        assert (GET_CODE(r)==CONST_DOUBLE);
        assert (GET_MODE_CLASS(m)==MODE_FLOAT);
        d = const_double_to_int (r, &n);
        assert (d);
        r = gen_rtx (CONST_INT, VOIDmode, n);
        break;
      }

      case AC_MOVE:
      { t  = r;
        jm = p->asm_table[j].mode;
        r  = gen_reg_rtx (jm);
  
        if (m == BLKmode)
        { int ali;

          assert (GET_CODE(t)==MEM);
          ali = i960_mem_alignment(t,1,p->asm_table[j].ali);

          if (ali >= GET_MODE_SIZE(jm))
            /* We have good enough alignment to just load it */
            emit_move_insn (r, change_address (t, jm, 0));

          else
          {
            /* We have to allocate a stack temp, move t to it, and load
               from the stack temp. */

            rtx l = assign_stack_temp (jm,GET_MODE_SIZE(jm),0);
            rtx s = change_address (l, BLKmode, 0);
            rtx c = gen_rtx (CONST_INT, VOIDmode, p->asm_table[j].bsiz);
          
            emit_block_move (s, t, c, ali);
            emit_move_insn  (r, l);
          }
        }
        else
          emit_move_insn (r, t);
        break;
      }
    }

    if (i >= use_only)
    { assert (v >= 0 && v < noutputs && v < 100);
      sprintf ((c=xmalloc(3)),"%d",v);
      output_rtx[v] = r;
    }

    /* argvec */
    XVECEXP (body,3,i) = r;

    /* constraints */
    XVECEXP (body,4,i) = gen_rtx (ASM_INPUT,p->asm_table[j].mode,c);
  }

  /* Protect all the operands from the queue,
     now that they have all been evaluated.  */

  for (i = 0; i < ninputs; i++)
    XVECEXP (body,3,i) = protect_from_queue (XVECEXP (body,3,i), 0);

  for (i = 0; i < noutputs; i++)
    output_rtx[i] = protect_from_queue (output_rtx[i], 1);

  /* Now, for each output, construct an rtx
     (set OUTPUT (asm_operands INSN OUTPUTNUMBER OUTPUTCONSTRAINT
                   ARGVEC CONSTRAINTS))
     If there is more than one, put them inside a PARALLEL.  */

  if (noutputs == 0 && nclobbers == 0)
    /* No output operands: put in a raw ASM_OPERANDS rtx.  */
    insn = emit_insn (body);

  else if (noutputs == 1 && nclobbers == 0)
  { XSTR (body, 1) = p->asm_table[p->asm_var_map[0]].cspec;
    insn = emit_insn (gen_rtx (SET, VOIDmode, output_rtx[0], body));
  }

  else
  {
    i = 0;
    if (noutputs==0)
    { /* store the ASM_OPERANDS directly into the PARALLEL.  */
      r    = body;
      body = gen_rtx (PARALLEL,VOIDmode, rtvec_alloc(1+ nclobbers));
      XVECEXP (body,0,i++) = r;
    }
    else
    { /* For each output operand, store a SET.  */
      body = gen_rtx (PARALLEL,VOIDmode, rtvec_alloc(noutputs+nclobbers));

      while (i < noutputs)
      { char* c = p->asm_table[p->asm_var_map[i]].cspec;

        XVECEXP (body,0,i) = gen_rtx (SET, VOIDmode, output_rtx[i],
        gen_rtx(ASM_OPERANDS,VOIDmode,string,c,i,argvec,constraints,file,line));

        MEM_VOLATILE_P (SET_SRC (XVECEXP (body, 0, i))) = !p->asm_pure;
        i++;
      }
    }

    /* Issue integer register clobbers... */
    u = p->asm_iuses;
    j = 0;
    while (u)
    { if (u & 1)
        XVECEXP(body,0,i++) =gen_rtx(CLOBBER,VOIDmode,gen_rtx(REG,QImode,j));
      j++;
      u >>= 1;
    }

    /* Issue floating point register clobbers ... */
    u = p->asm_fuses;
    j = 0;
    while (u)
    { if (u & 1)
        XVECEXP(body,0,i++) =gen_rtx(CLOBBER,VOIDmode,gen_rtx(REG,QImode,j));
      j++;
      u >>= 1;
    }

    if (p->asm_spillall)
      XVECEXP(body,0,i++) = gen_rtx (CLOBBER,VOIDmode,
        gen_rtx (MEM,QImode, gen_rtx(SCRATCH,VOIDmode,0)));

    insn = emit_insn (body);
  }

  if (struct_size && target)
  { int ali,siz; enum machine_mode m;

    assert (GET_CODE(target)==MEM);
    assert (GET_MODE(target)==BLKmode && p->asm_table[ASM_RETURN].varno==1);

    siz = MAX(struct_size, p->asm_table[ASM_RETURN].bsiz);
    ali = i960_mem_alignment (target, 1, p->asm_table[ASM_RETURN].ali);
    m   = GET_MODE(r=output_rtx[0]);

    if (ali >= GET_MODE_SIZE(m) && GET_MODE_SIZE(m) <= siz)
      /* We can just store it */
      emit_move_insn (change_address (target, m, 0), r);

    else
    { /* We have to allocate a stack temp, store r to it, and block
         move the temp to the target. */

      rtx l = assign_stack_temp (m, GET_MODE_SIZE(m), 0);
      rtx c = gen_rtx (CONST_INT, VOIDmode, siz);
          
      emit_move_insn  (l, r);
      emit_block_move (target, change_address(l,BLKmode,0), c, ali);
    }
  }
  free_temp_slots ();

  if (p->asm_table[ASM_RETURN].varno == 1)
    target = output_rtx[0];
  else
    target = 0;

  return target;
}

int
try_expand_asm_func (fndecl,ignore,struct_size,args,pfunexp,ptarget)
tree fndecl;
int ignore;
int struct_size;
char* args;
rtx *pfunexp;
rtx *ptarget;
{
  /* ic960 asm call expander.

     If fndecl is not an asm function,
       return 0.

     else

       Select the appropriate asm leaf, using the ic960 asm
       matching rules.

       If the leaf is a "call"

         Change the call target (*pfunexp) if required,
         then return 0.  This allows the normal calling
         mechanism to emit the code for the call.

       else

         Generate the appropriate rtl for the leaf, or issue
         an error, as required.  Modify *target to reflect
         where the return value from the "call" will be found,
         then return 1.
  */

  int ret = 0;

  if (IS_TREE_ASM_FUNCTION(fndecl))
  {
    asm_info *p,*match, *coerce;
    rtx target;

    if (ignore |= (TYPE_MODE(TREE_TYPE(DECL_RESULT(fndecl))) == VOIDmode))
      target = 0;
    else
      target = *ptarget;

    p = (asm_info *) DECL_SAVED_INSNS(fndecl);
    assert (p);

    /* Find first exact match, last coerceable match. */
    match = coerce = 0;
    while (p != 0 && p->asm_is_cond != 0 && match == 0)
    { int i,ok,exact;

      if (exact=(ok=(ignore || p->asm_table[ASM_RETURN].pclass != ASM_VOID)))
        for (i = 0; i < p->asm_nparms && ok != 0; i++)
        { asm_coercion ac = match_parm (i,args,p);
          exact &= (ac == AC_OK);
          ok    &= (ac != AC_NOMATCH);
        }

      if (ok)
        if (exact)
          match = p;
        else
          coerce = p;

      p = p->next;
    }

    if (match == 0)
      /* Exact match failed.  Use the last coerceble conditional pattern. */
      match = coerce;

    if (match == 0 && p != 0)
    { assert (p->asm_is_cond == 0 && (p->asm_error || p->asm_call));
      match = p;
    }

    if (match==0 || match->asm_call)
    {
      if (match == 0 || !match->asm_is_cond)
      { char buf[255],*s;

        s = (match) ? match->asm_call : XSTR(*pfunexp,0);
        sprintf (buf,"selecting unconditional call to %s in asm function %s",
                     s,IDENTIFIER_POINTER(DECL_NAME(fndecl)));

        warning (buf);
      }

      if (match && strcmp (XSTR(*pfunexp,0),match->asm_call))
        *pfunexp = gen_rtx (SYMBOL_REF, GET_MODE(*pfunexp), match->asm_call);
    }

    else if (match->asm_error)
      error (match->asm_template);

    else
    { /* Normal case.  Go do the expansion. */
      *ptarget = expand_asm_func (match, struct_size, args, target);
      ret = 1;
    }
  }
  return ret;
}

#endif
