#include "config.h"

#ifdef IMSTG
#include <stdio.h>	/* only necessary for dataflow.h */
#include <setjmp.h>
#include "tree.h"	/* only necessary for dataflow.h */
#include "rtl.h"
#include "regs.h"
#include "flags.h"

#include "assert.h"
#include "basic-block.h"
#include "i_dataflow.h"
#include "i_df_set.h"
#include "insn-flags.h"

/* Size in bits is hi part, lsbit is low part */
static req req_min      = 128 | (0 << 8);
static req req_max      = 0   | (128 << 8);
static int total_byte_cmp[4], total_shrt_cmp[4], total_word_cmp[4], coer_deleted[4], coerce_pass;

#ifdef TMC_DEBUG
coercion_stats ()
{
  char debug_buf[256];
  int i,del,byte,word,shrt;

  del = byte = word = shrt = 0;

  for (i = 0; i < 4; i++)
    del += coer_deleted[i];

  sprintf (debug_buf, "coerce1 saw %3d byte cmp,%3d shrt cmp,%3d word cmp",
           total_byte_cmp[1], total_shrt_cmp[1], total_word_cmp[1]);
  warning ("%s", debug_buf);
  sprintf (debug_buf, "coerce1 left%3d byte cmp,%3d shrt cmp,%3d word cmp, deleted%3d/%d",
           total_byte_cmp[2], total_shrt_cmp[2], total_word_cmp[2], coer_deleted[1],del);
  warning ("%s", debug_buf);
  sprintf (debug_buf, "coerce2 left%3d byte cmp,%3d shrt cmp,%3d word cmp, deleted%3d/%d",
           total_byte_cmp[3], total_shrt_cmp[3], total_word_cmp[3], coer_deleted[2], del);
  warning ("%s", debug_buf);
}
#endif

int
get_lbit (r)
req r;
{
  return r & 0xff;
}

int
get_bsiz (r)
req r;
{
  return r >> 8;
}

void
set_lbit (lb, r)
unsigned char lb;
req *r;
{
  *r &= 0xff00;
  *r |= lb;
}

req
mk_req (l, s)
unsigned char l;
unsigned char s;
{
  return l | (s << 8);
}

void
set_bsiz (bs, r)
unsigned char bs;
req *r;
{
  *r &= 0x00ff;
  *r |= (bs << 8);
}

static int
adjust_req (new, old)
req *old;
req new;
{
  int ret = 0;

  if (get_bsiz(*old) < get_bsiz(new))
  { set_bsiz(get_bsiz(new), old); ret = 1; } 

  if (get_lbit(*old) > get_lbit(new))
  { set_lbit(get_lbit(new),old); ret = 1; } 

  return ret;
}

static req
pick_mode_req (m, req_in)
enum machine_mode m;
req req_in;
{
  if (GET_MODE_CLASS(m)==MODE_INT)
    set_bsiz (MIN (GET_MODE_BITSIZE(m), get_bsiz(req_in)), &req_in);

  return req_in;
}

static req
pick_rtx_req (x, req_in, mode)
rtx x;
req req_in;
enum machine_mode *mode;
{ /*  req_in is a given required attribute.  If it can be improved
      by using information apparent in x, do so and return it. */

  enum rtx_code c = GET_CODE(x);
  enum machine_mode m = GET_MODE(x);

  if (c==IF_THEN_ELSE && m==VOIDmode &&GET_MODE(XEXP(x,1))==GET_MODE(XEXP(x,2)))
    m = GET_MODE(XEXP(x,1));

  req_in = pick_mode_req (m, req_in);

  if (c==ZERO_EXTRACT||c==SIGN_EXTRACT)
  { int esiz = XINT(XEXP(x,1),0);
    int epos = XINT(XEXP(x,2),0);
  
    req_in = mk_req (MAX (epos, get_lbit (req_in)), 
                     MIN (epos+esiz, get_bsiz (req_in)));
  }
  else
    if (IS_LVAL_PARENT_CODE(c))
      req_in = pick_rtx_req (XEXP(x,0), req_in, &m);

  *mode = m;
  return req_in;
}

static rng compute_expr_rng();
static rng merge_rng();

static int bsiz(v)
unsigned v;
{
  int ret = 0;

  while (v)
  { ret++;
    v >>= 1;
  }
  return ret;
}

static int
lbit(v)
unsigned v;
{
  int ret;

  if (v==0)
    ret = 128;
  else
  { ret = 0;
    while ((v&1)==0)
    { ret++;
      v >>= 1;
    }
  }
  return ret;
}

static int
search_for_rtx (x, r, x_p)
rtx x;
rtx r;
rtx* x_p;
{ /* Search for r within x;  when it is found, set
     *x_p to the rtx that contains it, and return
     the offset in bytes from the base *x_p to the
     field which points to r. */

  enum rtx_code c = GET_CODE(x);

  char* fmt = GET_RTX_FORMAT (c);
  int    rn = GET_RTX_LENGTH(c);
  int   ret = -1;
  int i;

  for (i=0; i < rn && ret < 0; i++)
    if (fmt[i] == 'e')
    { if (XEXP(x,i) == r)
      { *x_p = x;
        ret  = XEXP_OFFSET(x,i);
      }
    }

    else if (fmt[i] == 'E')
    { int j = XVECLEN(x, i);
      while (--j >= 0 && ret < 0)
        if (XVECEXP(x,i,j) == r)
        { *x_p = (rtx) &XVECEXP(x,i,j);
          ret = 0;
        }
    }

  for (i=0; i < rn && ret < 0; i++)
    if (fmt[i] == 'e')
      ret = search_for_rtx (XEXP(x,i), r, x_p);

    else if (fmt[i] == 'E')
    { int j = XVECLEN(x, i);
      while (--j >= 0 && ret < 0)
        ret = search_for_rtx (XVECEXP(x,i,j), r, x_p);
    }
  return ret;
}

static unsigned 
msk_req (r)
req r;
{
  int l = get_lbit (r);
  int n = get_bsiz (r);
  unsigned m = ((n<32) ?((-1)<<n) :0) | ((l<32)?((1<<l)-1):(-1));

  return ~m;
}

static unsigned
msk (x)
rng x;
{
  unsigned z, t, u;

  z = (x.zeroes < 32) ? ((1 << x.zeroes) - 1) : -1;

  if (x.bits >= 32 || x.tag != R_UNSIGNED)
    t = -1;
  else
    t = (1<<x.bits) - 1;

  return t & ~z;
}

#ifdef TMC_DEBUG
void
message (df, i, c, v, m)
ud_info *df;
int i;
rtx c;
int v;
int m;
{
  char mbuf[255];
  char* mp = mbuf;
  int uline;
  char* ufile;

  sprintf (mp,"deleted %08x %s %08x; context %08x", msk(UDN_FULL_RNG(df,i)), GET_RTX_NAME(GET_CODE(c)),v, m);
  get_ud_position (df, i, &ufile, &uline);
  warning_with_file_and_line (ufile, uline, 0, mbuf, 0, 0);
}
#endif

static int
is_cons_shift (x, code, count)
rtx x;
enum rtx_code* code;
int* count;
{
  int ret = 0;

  if (GET_CODE(x)==ASHIFT ||
      GET_CODE(x)==LSHIFT ||
      GET_CODE(x)==ASHIFTRT ||
      GET_CODE(x)==LSHIFTRT)

    if (ret = (GET_CODE(XEXP(x,1))==CONST_INT))
    { *code = GET_CODE(x);
      *count = XINT(XEXP(x,1),0);
    }

  return ret;
}

int
chk_dead_shift (insn, c, req_in, dead)
rtx insn;
rtx c;
req req_in;
ud_set dead;
{
  /*  If c is a dead shift, given req_in as the known context,
      remove it if 'dead' is 0, or remember it in 'dead' if not. */

  enum rtx_code code, first_code;
  int i,j,count, first_count;
  ud_info* df;
  int extra = 0;
  rtx s;

  if (GET_CODE(insn)==INSN && GET_CODE(s=PATTERN(insn))==SET &&
      SET_SRC(s)==c && is_cons_shift(c, &code, &count) &&
      (i=lookup_ud(INSN_UID(insn),&XEXP(c,0),&df))!= -1 &&df==df_info+IDF_REG &&
      (j=lookup_ud(INSN_UID(insn),&SET_DEST(s),&df))!= -1 &&df==df_info+IDF_REG)
  {
    int d = next_flex (df->maps.defs_reaching[i], -1);
    ud_rec* ip = df->maps.ud_map + i;

    if (d != -1 && next_flex (df->maps.defs_reaching[i],d) == -1)
    {
      int u = -1;
      rtx di = UDN_INSN_RTX(df,d);
      while ((u=next_flex (df->maps.uses_reached[d],u)) != -1)
        if (u != i && (UDN_ATTRS(df,u) & S_USE)!=0)
          break;

      if (u==-1 && (UDN_ATTRS(df,d) & S_KILL) &&
                    GET_CODE(PATTERN(di))==SET)
      { rtx s2 = SET_SRC(PATTERN(di));

        if (is_cons_shift (s2, &first_code, &first_count))
        { rng x;
          int s2_d;

          s2_d =lookup_ud(INSN_UID(di),&XEXP(s2,0),&df);

          if (s2_d != -1)
          {
            assert ((UDN_ATTRS(df,s2_d) & S_DEF)==0);
            x = UDN_FULL_RNG (df, s2_d);
  
            if (first_code==LSHIFT||first_code==ASHIFT)
              if (code==LSHIFTRT)
              {
                if ((x.tag==R_UNSIGNED && x.bits <= (unsigned)(32-first_count)) ||
                    (get_bsiz(req_in) <= 32 - count))
                  extra = 1 + (count == first_count);
              }
              else if (code == ASHIFTRT)
              {
                if (x.tag==R_SIGNED && x.bits <= (unsigned)(32-first_count) ||
                    x.tag==R_UNSIGNED && x.bits < (unsigned)(32-first_count) ||
                    get_bsiz(req_in) <= 32 - count)
                  extra = 1 + (count == first_count);
              }
  
            if (extra && !dead)
            { /* One or both of the shifts are redundant */
              if (extra == 2)
              { /* Remove the first shift completely */
                ud_rec* s2_dp = df->maps.ud_map+s2_d;
#ifdef TMC_DEBUG2
                message (df, s2_d, s2, XINT(XEXP(s2,1),0), msk_req (req_in));
#endif
                s2_dp->off = search_for_rtx(PATTERN(di),s2, &s2_dp->user);
                assert (s2_dp->off > 0);
                NDX_RTX (s2_dp->user,s2_dp->off) = XEXP(s2,0);
              }
              else if (count>first_count)
              { PUT_CODE(s2,code);
                XEXP(s2,1)=gen_rtx(CONST_INT,VOIDmode,count-first_count);
              }
              else
              { PUT_CODE(s2,first_code);
                XEXP(s2,1)=gen_rtx(CONST_INT,VOIDmode,first_count-count);
              }
  
              INSN_CODE(di) = -1;
              REG_NOTES(di) = 0;
  
#ifdef TMC_DEBUG2
              message (df, i, c, XINT(XEXP(c,1),0), msk_req(req_in));
#endif
  
              /* The first shift (or copy, if extra==2) now represents the same
                 calculated value that the original shift did. */
  
              UDN_FULL_RNG(df,d) = UDN_FULL_RNG(df,j);
  
              /* Always remove the top shift operation completely */
              ip->off = search_for_rtx (PATTERN(insn), c, &ip->user);
              assert (ip->off > 0);
              NDX_RTX (ip->user,ip->off) = XEXP(c,0);
              INSN_CODE(insn) = -1;
              REG_NOTES(insn) = 0;
  
              coer_deleted[coerce_pass] += extra;
            }
            else
              if (extra)
                set_flex_elt (dead, INSN_UID(insn));
          }
        }
      }
    }
  }
  return extra;
}

int
chk_dead_and (insn, x, req_in, dead)
rtx insn;
rtx x;
req req_in;
ud_set dead;
{
  int is_dead = 0;

  /*  If c is a dead and, given req_in as the known context,
      remove it if 'dead' is 0, or remember it in 'dead' if not. */

  if (GET_CODE(x)==AND)
  {
    int rc, rr, u;
    ud_info* df;
  
    if (((rc=1),(rr=0)),(GET_CODE(XEXP(x,rc))!=CONST_INT))
      if (((rc=0),(rr=1)),(GET_CODE(XEXP(x,rc))!=CONST_INT))
        rc = -1;
  
    if (rc >= 0 && (u=lookup_ud(INSN_UID(insn),&XEXP(x,rr),&df)) != -1)
    {
      int v = XINT(XEXP(x,rc),0);
      int m = msk_req  (req_in);
      int l = get_lbit (req_in);
      int n = get_bsiz (req_in);
      int r = msk(UDN_FULL_RNG(df,u));	/* 1's for bits possibly set */
  
      /* If no bits we care about get cleared by the and, it is dead */
      is_dead = (l >= n) || ((v|(~r)|(~m)) == -1);
  
      /* Once we have decided that the AND is not dead, it is
         not possible to have the AND be dead in the future.  Remember,
         the passed-in context (req_in) is gradually widening;  the
         effect is to knock bits out of (~m) in the expression above,
         which can only cause us to move towards deciding that the AND
         is not needed.  If this progression were not the case, it
         would not be correct to pass in restricted context once we
         decide the AND is not needed.  */
  
      if (is_dead)
        if (dead)
          /* Not settled; just record this for a future look. */
          set_flex_elt (dead, INSN_UID(insn));
        else
        { ud_rec* p = df->maps.ud_map + u;
  
#ifdef TMC_DEBUG2
          message (df,u,x,v,m);
#endif
  
          /* Change recorded context of register operand */
          p->off = search_for_rtx (PATTERN(insn), x, &p->user);
          assert (p->off > 0);
  
          /* Change the rtl */
          NDX_RTX (p->user,p->off) = XEXP(x,rr);
          INSN_CODE(insn) = -1;
          REG_NOTES(insn) = 0;
          coer_deleted[coerce_pass]++;
        }
    }
  }

  return is_dead;
}

int
is_equality_cmp (insn)
rtx insn;
{ /* insn is a compare; iff everybody using it is an equality
     comparison, return 1. */

  rtx i = insn;
  rtx p = SET_DEST(PATTERN(insn));
  rtx u;

  while ((i=NEXT_INSN(i)))
  { enum rtx_code c = GET_CODE(i);
 
    if (c==INSN || c==JUMP_INSN || c==CALL_INSN)
    {
      int off = search_for_rtx (PATTERN(i), p, &u);
  
      if (off >= 0)
      { assert (off > 0);
        switch (GET_CODE(u))
        {
          default:
            assert (0);
            break;
  
          case SET:	/* Redef means we are done */
            assert (SET_DEST(u)==p);
            return 1;
  
          case LT:
          case LE:
          case GT:
          case GE:
          case LTU:
          case LEU:
          case GTU:
          case GEU:
            return 0;
  
          case EQ:
          case NE:
            break;
        }
      }
    }

    if (c!=INSN && c!=NOTE)
      break;
  }
  return 1;
}

unsignize_cmp (i, x)
rtx i;
rtx x;
{ /* x (in i) is a signed compare; there are 0 or more users of
     the signed cc object it defines.  Make everybody refer to
     a new, unsigned cc object, and change the codes (LT, ...)
     in the users to be the unsigned versions (LTU, ...)
  */

  rtx cc  = gen_rtx (REG, CC_UNSmode, 36);
  rtx p   = SET_DEST(PATTERN(i));
  rtx u;
  int off = search_for_rtx (i, p, &u);
  int go  = 1;

  assert (off > 0 && GET_CODE(u)==SET && NDX_RTX(u,off)==p);

  /* Set the dest in the definition, and set the mode of the compare ... */
  NDX_RTX (u,off) = cc;
  x->mode=CC_UNSmode;
  INSN_CODE(i) = -1;
  REG_NOTES(i) = 0;

  while (go && (i=NEXT_INSN(i)))
  { enum rtx_code c = GET_CODE(i);
 
    if (c==INSN || c==JUMP_INSN || c==CALL_INSN)
    { off = search_for_rtx (PATTERN(i), p, &u);
  
      if (off >= 0)
      { assert (off > 0);
  
        if (GET_CODE(u)==SET && NDX_RTX(u,off)==p)
          go = 0;
        else
        { switch (GET_CODE(u))
          { case LT: u->code = LTU;  break;
            case LE: u->code = LEU;  break;
            case GT: u->code = GTU;  break;
            case GE: u->code = GEU;  break;
            default: assert (0); break;
          }
          NDX_RTX (u,off) = cc;
          INSN_CODE(i) = -1;
          REG_NOTES(i) = 0;
        }
      }
  
      if (c!=INSN && c!=NOTE)
        go = 0;	/* End of block */
    }
  }
}

#ifdef HAVE_cmpqi
void
chk_shrink_cmp (insn, x, changed)
rtx insn;
rtx x;
flex_set changed;
{
  /* Shrink this compare from 32 bits to 8/16 bits, if possible.  */

  enum rtx_code c = GET_CODE(x);
  enum machine_mode m;

  if (c==COMPARE && (HAVE_cmpqi||HAVE_cmpqi) &&
      ((m=COMPARE_MODE(x))==HImode||m==SImode))
  { rng r0,r1,r;
    enum machine_mode nm;

    r0 = compute_expr_rng(insn,&XEXP(x,0),changed);
    r1 = compute_expr_rng(insn,&XEXP(x,1),changed);
    r  = merge_rng (r0, r1);

    for (nm=QImode; nm!=m; nm=GET_MODE_WIDER_MODE(nm))
      if ((nm==QImode && HAVE_cmpqi) || (nm==HImode && HAVE_cmphi))
      {
        int bits = GET_MODE_BITSIZE(nm);

        /* If the compare arguments are appropriately clean ... */
        if (r.bits <= bits && (r.tag==R_SIGNED || r.tag==R_UNSIGNED))
        { int ok;
  
          if (!(ok = ((r.tag==R_UNSIGNED)==(GET_MODE(x)==CC_UNSmode))))
            if (!(ok = (r.bits <=(bits-1) && r.tag==R_UNSIGNED)))
              if (!(ok = (is_equality_cmp (insn))))
                if (ok = (r.tag==R_UNSIGNED))
                  unsignize_cmp (insn, x);
  
          if (ok)
          { INSN_CODE(insn) = -1;
            REG_NOTES(insn) = 0;
            if (GET_MODE(XEXP(x,0))!=VOIDmode)
              XEXP(x,0) = pun_rtx (XEXP(x,0), nm);
    
            if (GET_MODE(XEXP(x,1))!=VOIDmode)
              XEXP(x,1) = pun_rtx (XEXP(x,1), nm);
    
            COMPARE_MODE(x) = nm;
            break;
          }
        }
      }
  }
}

void
chk_grow_cmp (insn, x, changed)
rtx insn;
rtx x;
flex_set changed;
{
  /* Grow this compare from 8/16 bits to 32 bits, if possible.  */

  enum rtx_code c = GET_CODE(x);
  enum machine_mode m;

  if (c==COMPARE && ((m=COMPARE_MODE(x))==QImode||m==HImode))
  { rng r0,r1,r;
    int bits;

    r0   = compute_expr_rng(insn,&XEXP(x,0),changed);
    r1   = compute_expr_rng(insn,&XEXP(x,1),changed);
    r    = merge_rng (r0, r1);

    bits = GET_MODE_BITSIZE(m);

    /* If the compare arguments are appropriately clean ... */
    if (r.bits <= bits && (r.tag==R_SIGNED || r.tag==R_UNSIGNED))
    { int ok;
  
      if (!(ok = ((r.tag==R_UNSIGNED)==(GET_MODE(x)==CC_UNSmode))))
        if (!(ok = (r.bits <=(bits-1) && r.tag==R_UNSIGNED)))
          ok = is_equality_cmp (insn);
  
      if (ok)
      { INSN_CODE(insn) = -1;
        REG_NOTES(insn) = 0;
        if (GET_MODE(XEXP(x,0))!=VOIDmode)
          XEXP(x,0) = pun_rtx (XEXP(x,0), SImode);

        if (GET_MODE(XEXP(x,1))!=VOIDmode)
          XEXP(x,1) = pun_rtx (XEXP(x,1), SImode);

        COMPARE_MODE(x) = SImode;
      }
    }
  }
}
#endif

static void
push_required (insn, x_p, changed, dead, req_in) 
rtx insn;
rtx *x_p;
flex_set changed;
flex_set dead;
req req_in;
{
  /* Visit everybody in this subtree, pushing requirements down to
     the leaves.  For register set operations, we pass down the
     latest known required bit range for the register, which is kept
     in UDN_REQ_RNG for each register ud.  This datum starts out as
     zero (i.e, lbit > bsiz) and grows as users are discovered.

     Since this is a relaxation process, it is imperative that when a
     register requirement is non-zero, that requirement be marked in
     all definitions reaching the register.  Insns which may trap require
     particular care.  Suppose, for example, the the requirements at the
     use of a MEM indicate that the user doesn't care about the value of
     the mem (because the MEM will be ANDed with 0).  Unless we can guarantee
     that the MEM will not be loaded, we still have to walk the address of the
     mem, pushing appropriate requirements for the address calculation.  The
     same is true for the right operand of a divide.

     If we notice instructions that can be removed with current
     context information, if 'dead' is 0, we remove them.  Otherwise,
     dead is a flex set which we use to remember the fact that this
     insn could potentially be changed for the better.
  */

  req pass[32];

  rtx x;
  ud_info *df;
  int i,l,m,n,u,rc,rr,rn,is_dead;
  char* fmt;

  enum rtx_code c;
  enum machine_mode xm;

  if (*x_p == 0)
    return;

  u = lookup_ud(INSN_UID(insn), x_p, &df);
  x = *x_p;
  c = GET_CODE(x);

  fmt = GET_RTX_FORMAT (c);
  rn  = GET_RTX_LENGTH(c);
    
  assert (rn < sizeof(pass)/sizeof(pass[0]));

  /* Initialize passed attributes to worst case */
  for (i = 0; i < rn; i++)
    pass[i] = req_max;

  /* Try to narrow the passed-in requirement */
  req_in = pick_rtx_req (x, req_in, &xm);

  l = get_lbit (req_in);	/* lsbit our users care about */
  n = get_bsiz (req_in);	/* bit size our users care about */
  m = msk_req(req_in);		/* 1's for bits our users care about */

  is_dead = 0;

  if (u != -1 && df==df_info+IDF_REG)
  { /*  Check to see if the requirement causes widening of the
        known requirements of the reference. */

    if (adjust_req (req_in, &UDN_REQ_RNG(df,u)))
    { /* If this is a def, mark its insn for rescanning.  A pure use
         will not cause a rescan of the current insn;  however, the
         defs whose requirements it widens will be rescanned. */

      if (REAL_INSN(insn) && UDN_ATTRS(df,u) & S_DEF)
        set_flex_elt (changed, UDN_INSN_UID(df,u));
  
      if (UDN_ATTRS(df,u) & S_USE)
      { /* Push requirements of this use into each def reaching the use.
           Any insns which contain defs whose requirements widen as a result
           of this will be marked for later rescan. */
  
        int d = -1;
        while ((d=next_flex(UDN_DEFS(df,u),d)) != -1)
          push_required(UDN_INSN_RTX(df,d),&UDN_RTX(df,d),changed,dead,req_in);
      }
    }
  }

  else if (c == SET)
  {
    u = lookup_ud(INSN_UID(insn), &SET_DEST(x), &df);

    if (u!= -1 && df==df_info+IDF_REG)
    { pass[0] = UDN_REQ_RNG(df,u);
      pass[1] = pass[0];
    }
    else
      pass[1] = pick_rtx_req (SET_DEST(x), req_max, &xm);
  }

  else if (c == AND)
  { /* We try to replace (reg & const) with (reg & -1),
       (unless const is already -1) */

    if (chk_dead_and (insn, x, req_in, dead) || (l >= n))
      if (dead == 0)
        return;
      else
      { pass[0] = req_in;
        pass[1] = req_in;
      }
    else
    { /*  Where we have zeroes on the left,  restrict context on the right;
          where we have zeroes on the right, restrict context on the left. */

      int l0,l1,n0,n1,m0,m1;
 
      l0 = l1 = l;
      n0 = n1 = n;

      m0 = msk (compute_expr_rng(insn,&XEXP(x,0),changed));
      m1 = msk (compute_expr_rng(insn,&XEXP(x,1),changed));

      if (lbit(m1) > lbit(m0)) l0 = MAX(l0,lbit(m1));
      else                     l1 = MAX(l1,lbit(m0));

      if (bsiz(m1) < bsiz(m0)) n0 = MIN(n0,bsiz(m1));
      else                     n1 = MIN(n1,bsiz(m0));

      pass[0] = mk_req (l0, n0);
      pass[1] = mk_req (l1, n1);
    }
  }


  else
  { 
    for (i = 0; i < rn; i++)
    {
      if (GET_MODE_CLASS(xm)==MODE_INT && fmt[i]=='e')
      { rtx r = XEXP(x,i);
        enum machine_mode rm = GET_MODE(r);

        if (rm==VOIDmode || GET_MODE_CLASS(rm)==MODE_INT)
        {
          if (get_lbit (req_in) >= get_bsiz (req_in))
          { /* We don't have anybody who cares about the results, so provided
               we don't cause a trap, we don't care about the arguments */
            if (!(c==MEM || (i==1 && (c==DIV||c==UDIV||c==MOD||c==UMOD))))
              pass[i] = req_min;
          }
          else
            switch (c)
            {
              case IF_THEN_ELSE:
                if (i==1 || i==2)
                  pass[i] = req_in;
                break;

              case USE:
              case IOR:
              case XOR:
              case NOT:
                pass[i] = req_in;
                break;

              case PLUS:
              case MINUS:
              case MULT:
                pass[i] = mk_req (0, get_bsiz(req_in));
                break;
  
              case LSHIFTRT:
              case ASHIFTRT:
                if (i==0 && GET_CODE(XEXP(x,1)) == CONST_INT)
                { int v = XINT(XEXP(x,1),0) & ((GET_MODE_BITSIZE(xm))-1);
                  int t = GET_MODE_BITSIZE(xm);
        
                  if (chk_dead_shift (insn, x, req_in, dead))
                    if (dead)
                      pass[0]=mk_req (0, MAX(n+v,t));
                    else
                      return;
                  else
                    if ((l+v) >= t)
                      pass[0] = req_min;
                    else
                      pass[0]=mk_req (l+v, n+v);
                }
                break;
               
              case LSHIFT:
              case ASHIFT:
                if (i==0)
                  if (GET_CODE(XEXP(x,1)) == CONST_INT)
                  { int v = XINT(XEXP(x,1),0) & ((GET_MODE_BITSIZE(xm))-1);
          
                    pass[0] = mk_req (MAX(l-v,0), MAX(n-v,0));
                  }
                  else
                    pass[0] = mk_req (0, n);
                break;
            }
        }
      }
    }
  }

  for (i=0; i < rn; i++)
    if (fmt[i] == 'e')
      push_required (insn, &XEXP(x,i), changed,dead,pass[i]);

    else if (fmt[i] == 'E')
    { int j = XVECLEN(x, i);
      while (--j >= 0)
        push_required (insn, &XVECEXP(x,i,j), changed,dead, pass[i]);
    }
}

static rng rng_s8       = { R_SIGNED,  8, 0 };
static rng rng_u8       = { R_UNSIGNED,  8, 0 };
static rng rng_s16      = { R_SIGNED,  16, 0 };
static rng rng_u16      = { R_UNSIGNED,  16, 0 };
static rng rng_s32      = { R_SIGNED,  32, 0 };
static rng rng_shift    = { R_UNSIGNED, 5, 0 };

static rng
fix_rng (r0)
rng r0;
{
  if (r0.bits < 0)
    r0.bits = 0;

  if (r0.zeroes < 0)
    r0.zeroes = 0;

  if (r0.tag < (unsigned)R_UNSIGNED)
    r0 = rng_s32;

  else if (r0.zeroes >= 32)
  { r0.tag = R_UNSIGNED;
    r0.bits = 0;
    r0.zeroes = 32;
  }

  else if (r0.bits >= 32)
  { r0.bits = 32;
    r0.tag  = R_SIGNED;
  }

  return r0;
}

static rng
umode_rng(m)
enum machine_mode m;
{
  rng ret;

  if (m==SImode||m==VOIDmode)
    ret = rng_s32;
  else if (m==HImode)
    ret = rng_u16;
  else if (m==QImode)
    ret = rng_u8;
  else
  { assert (0);
    ret = rng_s32;
  }
  return ret;
}

static rng
smode_rng(m)
enum machine_mode m;
{
  rng ret;

  if (m==SImode||m==VOIDmode)
    ret = rng_s32;
  else if (m==HImode)
    ret = rng_s16;
  else if (m==QImode)
    ret = rng_s8;
  else
  { assert (0);
    ret = rng_s32;
  }
  return ret;
}

static rng
type_rng(t)
tree t;
{
  rng ret;
  ret = rng_s32;

  /* FIX this - do a better job for types other
     than 8,16,32 (i.e, enums) */

  if (t && TREE_CODE(t)==INTEGER_TYPE && GET_MODE_CLASS(TYPE_MODE(t))==MODE_INT)
  { int lo = TREE_INT_CST_LOW (TYPE_MIN_VALUE(t));
    int hi = TREE_INT_CST_LOW (TYPE_MAX_VALUE(t));

    if (lo < 0 && hi > 0)
    { if (lo < -32768 || hi > 32767)
        ret = rng_s32;
      else if (lo < -128 || hi > 127)
        ret = rng_s16;
      else
        ret = rng_s8;
    }
    else if (hi >= lo)
    { if (hi > 65535)
        ret = rng_s32;
      else if (hi > 255)
        ret = rng_u16;
      else
        ret = rng_u8;
    }
  }
  return ret;
}

static char*
print_rng(r,p)
rng r;
char* p;
{
  char t = '?';
  static char mybuf[32];

  if (p == 0)
    p = mybuf;

  if (r.tag == R_SIGNED)
    t = 's';
  else if (r.tag == R_UNSIGNED)
    t = 'u';

  sprintf (p, "0x%08x:%d%c", msk(r),r.bits,t);
  return p;
}

static int
same_rng (r0, r1)
rng r0;
rng r1;
{
  assert (r0.tag >= (unsigned)R_UNSIGNED && r1.tag >= (unsigned)R_UNSIGNED);

  return r0.tag == r1.tag && r0.bits == r1.bits && r0.zeroes == r1.zeroes;
}

static rng
merge_rng(r0, r1)
rng r0;
rng r1;
{
  rng ret;

  r0 = fix_rng (r0);
  r1 = fix_rng (r1);

  ret.tag    = MAX (r0.tag, r1.tag);		/* R_SIGNED > R_UNSIGNED */
  ret.zeroes = MIN (r0.zeroes, r1.zeroes);
  ret.bits   = MAX (r0.bits, r1.bits);

  if (ret.bits < 32)
  { if (r0.tag == R_SIGNED && r1.tag == R_UNSIGNED)
    { if (r0.bits <= r1.bits)
        ret.bits++;				/* May go to 0 (infinity) */
    }
    else if (r1.tag == R_SIGNED && r0.tag == R_UNSIGNED)
    { if (r1.bits <= r0.bits)
        ret.bits++;				/* May go to 0 (infinity) */
    }
  }
  else
    ret.tag = R_SIGNED;

  ret = fix_rng (ret);

  return ret;
}

static rng
isect_rng(r0,r1)
rng r0;
rng r1;
{

  rng ret;

  r0 = fix_rng (r0);
  r1 = fix_rng (r1);

  ret.tag    = MIN (r0.tag, r1.tag);
  ret.bits   = MIN (r0.bits,r1.bits);
  ret.zeroes = MAX (r0.zeroes,r1.zeroes);

  ret = fix_rng (ret);

  return ret;
}

static rng
compute_expr_rng(insn, x_p, changed)
rtx insn;
rtx *x_p;
flex_set changed;
{
  /*  Compute the known size of the expression at *x_p in bits,
      and the number of bits in the low part known to be zero. */
      
  rtx x;
  ud_info *df;
  rng r0, r1, ret;
  int u,d;
  enum machine_mode m;
  enum rtx_code c;

  ret = rng_s32;

  if (*x_p == 0)
    return ret;

  u   = lookup_ud(INSN_UID(insn), x_p, &df);
  x   = *x_p;
  m   = GET_MODE(x);
  c   = GET_CODE(x);

  if (u != -1 && (m==VOIDmode||m==QImode||m==HImode||m==SImode))
  {
    assert (m!=VOIDmode || c==SYMBOL_REF);

    ret = UDN_FULL_RNG(df,u);

    switch (c)
    { case MEM:
        compute_expr_rng(insn, &XEXP(x,0),changed);
        ret = isect_rng (umode_rng(m), ret);
        break;

      case SIGN_EXTEND:
        ret = isect_rng (smode_rng(GET_MODE(XEXP(x,0))), ret);
        break;

      case ZERO_EXTEND:
        ret = isect_rng (umode_rng(GET_MODE(XEXP(x,0))), ret);
        break;

      case SYMBOL_REF:
      { int n = SYMREF_SIZE(x);

        if      (n >=16) ret.zeroes = MAX(ret.zeroes,4);
        else if (n >= 8) ret.zeroes = MAX(ret.zeroes,3);
        else if (n >= 4) ret.zeroes = MAX(ret.zeroes,2);
        else if (n >= 2) ret.zeroes = MAX(ret.zeroes,1);
      }
    }

    if (!(UDN_ATTRS(df,u) & S_DEF))
    {
      /* Intersect (improve) the current range with the union
         of the ranges of the defs reaching this use.

         We have set things up so that all REGs get an initial
         definition, with range 32u.  This is important, because mask
         operations applied to uninitialized registers are not eliminable.

         For now, we don't look at the defs reaching MEMs - we just take the
         apparent range of the reference (as indicated by the mode of the MEM).

         If we look at the ranges reaching MEMs, we need to ensure that
         all MEMs are defined initially (currently, stack MEMs have no
         initial definitions) as we do for REGs.

         We don't look at the reaching defs for SYMBOL_REFS as those ranges
         are apparent at each use and will not improve anyway.
      */

      if (df==df_info+IDF_REG && (d=next_flex(UDN_DEFS(df,u),-1)) != -1)
      {
        r0 = compute_expr_rng(UDN_INSN_RTX(df,d),&UDN_RTX(df,d),changed);

        while ((d=next_flex(UDN_DEFS(df,u),d)) != -1)
        { r1 = compute_expr_rng(UDN_INSN_RTX(df,d),&UDN_RTX(df,d),changed);
          r0 = merge_rng (r0, r1);
        }

        ret = isect_rng (r0, ret);
      }

      UDN_FULL_RNG(df,u) = ret;
    }
  }

  else
    switch (c)
    {
      default:
      { char* fmt = GET_RTX_FORMAT (c);
        int i;

        for (i=0; i < GET_RTX_LENGTH(c); i++)
          if (fmt[i] == 'e')
            compute_expr_rng (insn, &XEXP(x,i), changed);

          else if (fmt[i] == 'E')
          { int j = XVECLEN(x, i);
            while (--j >= 0)
              compute_expr_rng (insn, &XVECEXP(x,i,j), changed);
          }
        break;
      }

      case SET:
      {
        r0  = compute_expr_rng(insn, &SET_SRC(x),changed);
        r1  = compute_expr_rng(insn, &SET_DEST(x),changed);
        d   = lookup_ud(INSN_UID(insn), &SET_DEST(x), &df);

        if (d != -1)
        {
          ret = isect_rng (r0,r1);

          UDN_FULL_RNG(df,d) = ret;

          if (!same_rng (ret,r1) && (GET_DF_STATE(df) & DF_REACHING_DEFS))
          { u = -1;
            while ((u=next_flex(UDN_USES(df,d),u)) != -1)
              if (REAL_INSN(UDN_INSN_RTX(df,u)) && !(UDN_ATTRS(df,u) & S_DEF))
                set_flex_elt (changed, UDN_INSN_UID(df,u));
          }
        }
        break;
      }

      case IF_THEN_ELSE:
        compute_expr_rng (insn, &XEXP(x,0),changed);
        r0  = compute_expr_rng (insn, &XEXP(x,1),changed);
        r1  = compute_expr_rng (insn, &XEXP(x,2),changed);
        ret = merge_rng (r0, r1);
        break;

      case SIGN_EXTEND:
        r0  = compute_expr_rng (insn, &XEXP(x,0),changed);
        ret = isect_rng (smode_rng(GET_MODE(XEXP(x,0))), r0);
        break;

      case ZERO_EXTEND:
        r0  = compute_expr_rng (insn, &XEXP(x,0),changed);
        ret = isect_rng (umode_rng(GET_MODE(XEXP(x,0))), r0);
        break;

      case CONST_INT:
      { int i      = XINT(x,0);
        int tag    = (i < 0) ? R_SIGNED : R_UNSIGNED;
  
        int bits, zeroes;
  
        if (i == 0)
        { bits = 1;
          zeroes = 32;
        }
  
        else
        {
          if (i == 0x80000000)
          { bits = 32;
            zeroes = 31;
          }
  
          else
          { bits = zeroes = 0;
    
            if (i < 0)
            { int m = -1;
  
              while (m >= i)
              { bits++;
                m *= 2;
              }
            }
            else
            { unsigned m = 1;
  
              while (m <= i)
              { bits++;
                m *= 2;
              }
            }
  
            while ((i & 1) == 0)
            { zeroes++;
              i >>= 1;
            }
          }
        }
  
        ret.tag    = tag;
        ret.bits   = bits;
        ret.zeroes = zeroes;

        ret = fix_rng (ret);
        break;
      }
  
      case AND:
        r0 = compute_expr_rng (insn, &XEXP(x,0),changed);
        r1 = compute_expr_rng (insn, &XEXP(x,1),changed);
  
        ret.zeroes = MAX (r0.zeroes, r1.zeroes);
        ret.tag    = MIN (r0.tag, r1.tag);

        if (r0.tag == R_UNSIGNED && r1.tag == R_UNSIGNED)
          ret.bits = MIN (r0.bits, r1.bits);
        else if (r0.tag == R_UNSIGNED && r1.tag == R_SIGNED)
          ret.bits = r0.bits;
        else if (r0.tag == R_SIGNED && r1.tag == R_UNSIGNED)
          ret.bits = r1.bits;
        else
          ret.bits = MAX (r0.bits, r1.bits);

        ret = fix_rng (ret);
        break;

      case IOR:
      case XOR:
        r0 = compute_expr_rng (insn, &XEXP(x,0),changed);
        r1 = compute_expr_rng (insn, &XEXP(x,1),changed);

        ret  = merge_rng (r0, r1);
        break;

  
      case LSHIFT:
      case ASHIFT:
        r0 = compute_expr_rng (insn, &XEXP(x,0),changed);
        r1 = compute_expr_rng (insn, &XEXP(x,1),changed);
        r1 = isect_rng (r1, rng_shift);
  
        ret = r0;
        ret.bits = MIN ((ret.bits << r1.bits), (unsigned)32);
  
        if (GET_CODE(XEXP(x,1)) == CONST_INT)
          ret.zeroes = MIN(ret.zeroes+(XINT(XEXP(x,1),0) & 31), (unsigned)32);

        ret = fix_rng (ret);
        break;
  
      case ASHIFTRT:
      case LSHIFTRT:
        r0 = compute_expr_rng (insn, &XEXP(x,0),changed);
  
        /* Logical shift of signed range causes discontinuity, since 0's
           come into hi part.  Treat as 32 bit signed, then map to unsigned. */

        if (c==LSHIFTRT && r0.tag==R_SIGNED)
          r0 = rng_s32;

        r1 = compute_expr_rng (insn, &XEXP(x,1),changed);
        r1 = isect_rng (r1, rng_shift);

        ret = r0;
  
        assert (ret.bits < 32 || ret.tag == R_SIGNED);
  
        if (GET_CODE(XEXP(x,1)) == CONST_INT)
        { int n = XINT(XEXP(x,1),0) & 31;

          ret.zeroes = MAX(((int)(ret.zeroes-n)), 0);
          ret.bits   = MAX(((int)(ret.bits  -n)), 0);
        }
        else
          if (r1.zeroes)
          { int c = (1 << r1.zeroes);
   
            /* Note that we cannot reduce # of bits, because even
               though we have known zeroes, we might still have a zero
               shift count. */

            ret.zeroes = MAX(((int)(ret.zeroes-c)), 0);
          }
          else
            ret.zeroes = 0;

        if (c==LSHIFTRT && ret.bits < 32)
          ret.tag = R_UNSIGNED;

        /* Can't shift the last bit out of signed range */
        if (ret.bits == 0 && ret.tag == R_SIGNED)
          ret.bits = 1;

        ret = fix_rng (ret);
        break;
  
      case PLUS:
      case MINUS:
        r0 = compute_expr_rng (insn, &XEXP(x,0),changed);
        r1 = compute_expr_rng (insn, &XEXP(x,1),changed);
  
        ret = merge_rng (r0,r1);
  
        if (ret.bits < 32)
        { ret.bits++;
          ret = fix_rng (ret);
        }
        break;

      case DIV:
        ret = compute_expr_rng (insn, &XEXP(x,0),changed);
        r1  = compute_expr_rng (insn, &XEXP(x,1),changed);

        if (ret.bits<32 && ret.tag==R_SIGNED && r1.tag==R_SIGNED &&r1.zeroes==0)
          ret.bits++;
        ret.zeroes = 0;
        break;

      case UDIV:
        ret = compute_expr_rng (insn, &XEXP(x,0),changed);
        r1  = compute_expr_rng (insn, &XEXP(x,1),changed);
        ret.zeroes = 0;
        break;

      case MOD:
        r0 = compute_expr_rng (insn, &XEXP(x,0),changed);
        r1 = compute_expr_rng (insn, &XEXP(x,1),changed);
        ret  = merge_rng (r0, r1);
        ret.zeroes = 0;
        break;

      case UMOD:
        r0  = compute_expr_rng (insn, &XEXP(x,0),changed);
        ret = compute_expr_rng (insn, &XEXP(x,1),changed);
        ret.zeroes = 0;
        break;

      case MULT:
        r0  = compute_expr_rng (insn, &XEXP(x,0),changed);
        r1  = compute_expr_rng (insn, &XEXP(x,1),changed);

        ret.tag    = MAX (r0.tag, r1.tag);
        ret.bits   = MIN (r0.bits+r1.bits, (unsigned)32);
        ret.zeroes = MIN (r0.zeroes+r1.zeroes, (unsigned)32);

        ret = fix_rng (ret);
        break;
    }
  return ret;
}

compute_full_rng()
{
  int i,k;
  rtx r;

  /* Calculate apparent ranges of expressions, without regard
     to context. */

  flex_set change1 = 0;
  flex_set change2 = 0;

  reuse_flex (&change1, NUM_UID, 1, F_DENSE);
  reuse_flex (&change2, NUM_UID, 1, F_DENSE);

  for (k = 0; k < (int)NUM_DFK; k++)
    for (i = 0; i < NUM_UD(df_info+k); i++)
      UDN_FULL_RNG(df_info+k,i) = rng_s32;

  for (r=get_insns(); r; r=NEXT_INSN(r))
    if (REAL_INSN(r))
    { compute_expr_rng (r, &PATTERN(r), change1);
#ifdef CALL_INSN_FUNCTION_USAGE
      if (GET_CODE(r)==CALL_INSN)
        compute_expr_rng (r, &CALL_INSN_FUNCTION_USAGE(r), change1);
#endif
    }

  while (!is_empty_flex(change1))
  {
    copy_flex (change2, change1);
    clr_flex  (change1);

    for (r=get_insns(); r; r=NEXT_INSN(r))
      if (in_flex (change2, INSN_UID(r)))
      { compute_expr_rng (r, &PATTERN(r), change1);
#ifdef CALL_INSN_FUNCTION_USAGE
        if (GET_CODE(r)==CALL_INSN)
          compute_expr_rng (r, &CALL_INSN_FUNCTION_USAGE(r), change1);
#endif
      }
  }

  reuse_flex (&change2, 0, 0, 0);
  reuse_flex (&change1, 0, 0, 0);
}

int
find_dead_coercions ()
{
  ud_info *mem, *reg, *sym;
  int i,j;
  rtx r;

  flex_set change1 = 0;
  flex_set change2 = 0;
  flex_set dead    = 0;

  reg = df_info+IDF_REG;
  mem = df_info+IDF_MEM;
  sym = df_info+IDF_SYM;

  if (GET_DF_STATE(reg) & DF_ERROR)
    return 0;

  if (GET_DF_STATE(mem) & DF_ERROR)
    return 0;

  if (GET_DF_STATE(sym) & DF_ERROR)
    return 0;

  ALLOC_MAP (&mem->maps.ud_full_rng, NUM_UD(mem), rng_map);
  ALLOC_MAP (&reg->maps.ud_full_rng, NUM_UD(reg), rng_map);
  ALLOC_MAP (&sym->maps.ud_full_rng, NUM_UD(sym), rng_map);

  ALLOC_MAP (&reg->maps.ud_req_rng, NUM_UD(reg), req_map);

  reuse_flex (&change1, NUM_UID, 1, F_DENSE);
  reuse_flex (&change2, NUM_UID, 1, F_DENSE);
  reuse_flex (&dead,    NUM_UD(reg), 1, F_DENSE);

  for (i = 0; i < NUM_UD(reg); i++)
    UDN_REQ_RNG(reg,i) = req_min;

  compute_full_rng();

  /* Make a pass getting rid of those shifts and masks which
     can be removed regardless of context. It should be
     possible to remove this loop and still get correct behaviour
     in the context removal loop, but the results will often
     be different. I think this (removing context-free cases
     first) will probably run faster and give more consistent
     results.  Most eliminable coercions are currently caught
     in this loop.  TMC Spring 93 */

  for (i = 0; i < NUM_UD(reg); i++)
  { ud_rec* ip = reg->maps.ud_map + i;

    if (!(UD_ATTRS(*ip) & S_DEF))
    { rtx c = UD_CONTEXT (*ip);

      if (c)
      { rtx ii = UD_INSN_RTX (*ip);
        if (!chk_dead_and (ii,c,req_max,0) && !chk_dead_shift (ii,c,req_max,0))
#ifdef HAVE_cmpqi
          if (HAVE_cmpqi)
            chk_shrink_cmp (ii, c, change1, QImode);
#else
          ;
#endif
      }
    }
  }
  assert (is_empty_flex (change1));

  /* Calculate context by assuming none and continuing to
     enlarge it until there are no changes.  Whenever the
     context settles, run the list of last known changes
     looking for shifts or ands to remove.  When one is
     removed, make the appropriate context enlargement
     (which can cause more things to go onto the change list).

     Quit when there are no changes. */

  for (r=get_insns(); r; r=NEXT_INSN(r))
    if (REAL_INSN(r))
    { push_required (r, &PATTERN(r), change1, dead, req_max);
#ifdef CALL_INSN_FUNCTION_USAGE
      if (GET_CODE(r)==CALL_INSN)
        push_required (r, &CALL_INSN_FUNCTION_USAGE(r), change1, dead, req_max);
#endif
    }

  while (!is_empty_flex(change1) || !is_empty_flex(dead))
  { 
    copy_flex (change2, change1);
    clr_flex  (change1);

    for (r=get_insns(); r; r=NEXT_INSN(r))
      if (is_empty_flex (change1) && is_empty_flex(change2))
      { if (in_flex (dead, INSN_UID(r)))
        { clr_flex_elt (dead, INSN_UID(r));

          /* Passing in 0 instead of 'dead' here tells push_required
             that it is actually safe to remove code instead of
             noting that it may be dead. */

          push_required (r, &PATTERN(r), change1, 0, req_max);
#ifdef CALL_INSN_FUNCTION_USAGE
          if (GET_CODE(r)==CALL_INSN)
            push_required (r, &CALL_INSN_FUNCTION_USAGE(r), change1, 0,req_max);
#endif
        }
      }
      else
        if (in_flex (change2, INSN_UID(r)))
          /*  If this insn might be dead, remember it so we can look
              again after context settles to see if it is still dead. */

        { push_required (r, &PATTERN(r), change1, dead, req_max);
#ifdef CALL_INSN_FUNCTION_USAGE
          if (GET_CODE(r)==CALL_INSN)
            push_required (r, &CALL_INSN_FUNCTION_USAGE(r), change1, dead,req_max);
#endif
        }
  }

#ifdef HAVE_cmpqi
  if (HAVE_cmpqi)
  { /* Make a final pass enlarging compares to SI where possible. */
    compute_full_rng();
    for (i = 0; i < NUM_UD(reg); i++)
    { ud_rec* ip = reg->maps.ud_map + i;
  
      if (!(UD_ATTRS(*ip) & S_DEF))
      { rtx c = UD_CONTEXT (*ip);
        if (c)
          chk_grow_cmp (UD_INSN_RTX(*ip), c, change1, SImode);
      }
    }
  }
#endif

  reuse_flex (&dead,    0, 0, 0);
  reuse_flex (&change2, 0, 0, 0);
  reuse_flex (&change1, 0, 0, 0);
  return coer_deleted[coerce_pass];
}

void
subst_si_for_qihi (context, offset)
rtx context;
int offset;
{
  /* Completely (recursively) scan this subtree and replace the QI/SI
     pseudo registers within it with SUBREGs of SI registers. */

  rtx *x_addr   = &NDX_RTX (context, offset);
  rtx  x        = *x_addr;
  
  if (x != 0)
  {
    enum rtx_code     c      = GET_CODE(x);
    enum machine_mode x_mode = GET_MODE(x);
  
    switch (c)
    {
      default:
      { char* fmt = GET_RTX_FORMAT (c);
        int i;
      
        for (i=0; i < GET_RTX_LENGTH(c); i++)
          if (fmt[i] == 'e')
            subst_si_for_qihi (x, XEXP_OFFSET (x, i));
      
          else if (fmt[i] == 'E')
          { int j = XVECLEN(x, i);
            while (--j >= 0)
              subst_si_for_qihi (&XVECEXP(x,i,j), 0);
          }
        break;
      }
  
      case COMPARE:
      { if (COMPARE_MODE(x)==VOIDmode)
          if (GET_MODE(XEXP(x,0))!=VOIDmode)
            COMPARE_MODE(x)=GET_MODE(XEXP(x,0));
          else
            COMPARE_MODE(x)=GET_MODE(XEXP(x,1));

        if (COMPARE_MODE(x)==QImode)
          total_byte_cmp[coerce_pass]++;
        else if (COMPARE_MODE(x)==HImode)
          total_shrt_cmp[coerce_pass]++;
        else if (COMPARE_MODE(x)==SImode)
          total_word_cmp[coerce_pass]++;

        subst_si_for_qihi (x, XEXP_OFFSET (x,0));
        subst_si_for_qihi (x, XEXP_OFFSET (x,1));

        break;
      }

      case SUBREG:
      { rtx r = SUBREG_REG(x);
        enum machine_mode rm = GET_MODE(r);
  
        /* We should be in a note if GET_CODE(r) != REG */
        if (GET_CODE(r)==REG)
          if (REGNO(r) < FIRST_PSEUDO_REGISTER)
            *x_addr = pun_rtx (x, x_mode);
          else
            if (x_mode==SImode && (rm==QImode||rm==HImode))
              *x_addr=r;		/* Register will be made SI later */
            else
              subst_si_for_qihi (x, XEXP_OFFSET(x,0));
  
        break;
      }
  
      case REG:
      { /* Note that this code is not used for registers in most SET rtls;
           those SET rtls which can be punned up to SI are handled specially
           in the code for the SET case below. */
  
        int r = REGNO(x);
  
        /* If we are going to change mode of this reg ... */
        if ((x_mode==QImode||x_mode==HImode) && r >= FIRST_PSEUDO_REGISTER)

          /* And it isn't already being treated as another mode anyway ... */
          if (GET_CODE(context) != SUBREG)

            /* Give the user a reference with the correct mode */
            *x_addr = gen_rtx (SUBREG, x_mode, x, 0);

        break;
      }
  
      case SET:
      { rtx d = SET_DEST(x);
        rtx s = SET_SRC(x);
  
        enum machine_mode dm = GET_MODE(d);
        enum machine_mode sm = GET_MODE(s);
  
        if ((dm!=QImode && dm!=HImode) ||
            (GET_CODE(d)!=REG &&
              (GET_CODE(d)!=SUBREG||GET_CODE(SUBREG_REG(d))!=REG)) ||
            !(sm==VOIDmode || GET_CODE(s)==REG || GET_CODE(s)==SUBREG ||
              GET_CODE(s)==MEM ||
              GET_CODE(s)==ZERO_EXTEND || GET_CODE(s)==SIGN_EXTEND))
  
        { subst_si_for_qihi (x, XEXP_OFFSET (x, 0));
          subst_si_for_qihi (x, XEXP_OFFSET (x, 1));
        }
        else
        { /* Assignment to QI/HI register; rewrite as assignment to SI reg */
  
          if (GET_CODE(d)==REG)
          { if (REGNO(d) < FIRST_PSEUDO_REGISTER)
            { SET_DEST(x) = gen_rtx (REG, SImode, REGNO(d));
              RTX_TYPE(SET_DEST(x)) = RTX_TYPE(d);
            }
          }
          else
          { rtx r = SUBREG_REG(d);
            enum machine_mode rm = GET_MODE(r);
  
            assert (GET_CODE(r)==REG);
  
            if (REGNO(r) < FIRST_PSEUDO_REGISTER)
              SET_DEST(x)=pun_rtx(d,SImode);
            else if (rm==QImode||rm==HImode||rm==SImode)
              SET_DEST(x)=r;		/* Register will be made SI later */
            else
              PUT_MODE(d,SImode);		/* Can't change the register */
          }
  
          if (sm == VOIDmode)
            subst_si_for_qihi (x, XEXP_OFFSET (x, 1));
  
          else
          { rng tr;
            assert (sm==dm);
  
            if (GET_CODE(s)==REG)
            { if (REGNO(s) < FIRST_PSEUDO_REGISTER)
              { SET_SRC(x) = gen_rtx (REG, SImode, REGNO(s));
                RTX_TYPE(SET_SRC(x)) = RTX_TYPE(s);
              }
            }
            else if (GET_CODE(s)==SUBREG)
            { rtx r = SUBREG_REG(s);
              enum machine_mode rm = GET_MODE(r);
  
              if (GET_CODE(r)==MEM)
              { if (rm==QImode || rm==HImode)
                { 
                  tr = type_rng (RTX_TYPE(RTX_LVAL(d)));
                  /* Use type from mem if reg is no help */
                  if (tr.bits > (unsigned)GET_MODE_BITSIZE(rm) ||
                      (tr.tag!=R_SIGNED && tr.tag!=R_UNSIGNED))
                    tr = type_rng (RTX_TYPE(r));
  
                  if (tr.bits<=(unsigned)GET_MODE_BITSIZE(rm) && tr.tag==R_SIGNED)
                    SET_SRC(x)=gen_rtx (SIGN_EXTEND, SImode, r);
                  else
                    SET_SRC(x)=gen_rtx (ZERO_EXTEND, SImode, r);
                }
                else
                  if (rm==SImode)
                    SET_SRC(x)=r;
                  else
                    PUT_MODE(s,SImode);
  
                subst_si_for_qihi (r, XEXP_OFFSET (r, 0));
              }
              else
              { assert (GET_CODE(r)==REG);
  
                if (REGNO(r) < FIRST_PSEUDO_REGISTER)
                  SET_SRC(x)=pun_rtx(s,SImode);
                else if (rm==QImode||rm==HImode||rm==SImode)
                  SET_SRC(x)=r;		/* Register will be made SI later */
                else
                  PUT_MODE(s,SImode);	/* Won't be changing the register */
              }
            }
            else if (GET_CODE(s)==SIGN_EXTEND || GET_CODE(s)==ZERO_EXTEND)
            { PUT_MODE(s,SImode);
              subst_si_for_qihi (s, XEXP_OFFSET (s, 0));
            }
  
            else if (GET_CODE(s)==MEM)
            { tr = type_rng (RTX_TYPE(RTX_LVAL(d)));
              /* Use type from mem if reg is no help */
              if (tr.bits > (unsigned)GET_MODE_BITSIZE(sm) ||
                  (tr.tag!=R_SIGNED && tr.tag!=R_UNSIGNED))
                tr = type_rng (RTX_TYPE(s));
  
              if (tr.bits<=(unsigned)GET_MODE_BITSIZE(sm) && tr.tag==R_SIGNED)
                SET_SRC(x) = gen_rtx (SIGN_EXTEND, SImode, s);
              else
                SET_SRC(x) = gen_rtx (ZERO_EXTEND, SImode, s);
              subst_si_for_qihi (s, XEXP_OFFSET (s, 0));
            }
            else
              assert (0);
          }
        }
      }
    }
  }
}

insn_subst_si_for_qihi (coer_pass)
{ /* Scan the function, substituting si pseudo regs for qi/hi pseudo regs. */

  rtx x;
  int r;

  int start_byte, start_word;
  extern rtx *regno_reg_rtx;
  extern int reg_rtx_no;

  coerce_pass = coer_pass;

  for (x = get_insns(); x; x = NEXT_INSN(x))
    if (REAL_INSN(x))
    { subst_si_for_qihi (&x, 0);
      INSN_CODE(x) = -1;
    }

  for (r=FIRST_PSEUDO_REGISTER; r < reg_rtx_no; r++)
  { enum machine_mode m;

    x=regno_reg_rtx[r];

    if (x != 0 && GET_CODE(x)==REG)
      if ((m=GET_MODE(x))==QImode || m==HImode)
        PUT_MODE (x, SImode);
  }
}
#endif
