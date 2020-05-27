#include "config.h"

#ifdef IMSTG
#include "stdio.h"
#include "assert.h"

#ifdef GCC20
#include "i_df_set.h"
#else
#include "df_set.h"
#endif

flex_global_info flex_data;

#define FLEX_NLONG(S)     ((FLEX_POOL(S))->lps)
#define FLEX_BODY_ALLOC(S)    ((FLEX_NLONG(S) - 1))
#define MIN_FLEX_BODY_SIZE(E) (((E)/HINT_BITS + 1))
#define IS_SPARSE(S)   (((FLEX_POOL(S)->attrs & F_SPARSE) != 0))

/* Grab a temp.  Don't keep it long;  just get it out of a small
   circular buffer. */
#define GRAB_FLEX(W) (( \
  ((flex_data.ftno=(flex_data.ftno+1) % (sizeof(flex_data.sets.ft_buf)/sizeof(flex_data.sets.ft_buf[0]))), \
   (new_flex_pool (&(flex_data.pools.ft_pool[flex_data.ftno]), 32*(W), 1, F_DENSE))), \
   (flex_data.sets.ft_buf[flex_data.ftno]=alloc_flex(flex_data.pools.ft_pool[flex_data.ftno])) ))

#define FLEX_PID_BUSY(P) (( (P)==0 || flex_data.flexp[P].lps || flex_data.flexp[P].num_sets_used ))

#define ALLOC_POOL_TEXT(N,P) \
do { \
  if ((P)->text) \
    free ((P)->text); \
  (P)->text = (unsigned long*) xmalloc ((N) * sizeof (unsigned long)); \
} \
while (0)

#define REALLOC_POOL_TEXT(N,P) \
do { \
  (P)->text = (unsigned long *)xrealloc ((P)->text, (N)*sizeof(unsigned long)); \
} \
while (0)

#define FREE_POOL_TEXT(P) \
do { \
  if ((P)->text) \
    free ((P)->text); \
} \
while (0)

#define CLEAR_POOL_TEXT(A,B,P) \
(( bzero ((P)->text+(A), ((B)-(A)) * sizeof (unsigned long)) ))

#define GET_FLEX_HI(p)            ((p)[-1]&0xFFFF)
#define GET_FLEX_LO(p)            ((p)[-1]>>16)
#define PCK_FLEX_LO_HI(p,lo,hi)   ((p)[-1] = ((lo) << 16) | ((hi) & 0xFFFF))

#define GET_SPARSE_ELT(p,i)       (((p)[(i)>>1] >> (((i) & 1) << 4)) & 0xFFFF)
#define SET_SPARSE_ELT(p,i)

#define GET_FLEX_BODY(s,pool_p,body_p) \
do { \
  pool_p = &flex_data.flexp[FLEX_POOL_NUM(s)]; \
  body_p = &pool_p->text[(FLEX_SET_NUM(s) * pool_p->lps) + 1]; \
} while (0)

flex_pool *
new_flex_pool (pool, nume, num_sets, attrs)
flex_pool** pool;
int nume;
int num_sets;
int attrs;
{
  flex_pool* p = 0;
  int pid = 0;

  num_sets++;		/* We keep the 0th set, so we need an extra */

  if (pool != 0)
    p = *pool;

  if (p == 0)
  { /* Grab a free pool ... */
    while (FLEX_PID_BUSY (flex_data.next_pid))
      flex_data.next_pid = (flex_data.next_pid+1) % (sizeof(flex_data.flexp)/sizeof(flex_data.flexp[0]));

    p = &flex_data.flexp[flex_data.next_pid];

    if (pool)
      *pool = p;
  }

  p->attrs = attrs;

  if (attrs & F_SPARSE)
    p->lps = (1+nume) / 2;
  else
    p->lps = MIN_FLEX_BODY_SIZE (nume-1);

  p->lps += 1;

  /* If there isn't room for the number of sets we expect, pump up
     the allocation. */

  if (p->longs_allocated < p->lps * num_sets)
  { p->longs_allocated = num_sets * p->lps;
    ALLOC_POOL_TEXT (p->longs_allocated, p);
  }

  /* Set up the null set as the 0th set ... */
  p->num_sets_used = 1;
  CLEAR_POOL_TEXT (0, p->longs_allocated, p);

  return p;
}

init_flex_temps()
{
  int i;

  /* Establish a small circular buffer of temporary sets. */

  for (i=0;i<sizeof(flex_data.sets.ft_buf)/sizeof(flex_data.sets.ft_buf[0]);i++)
  { new_flex_pool (&(flex_data.pools.ft_pool[i]), 32, 1, F_DENSE);
    flex_data.sets.ft_buf[i] = alloc_flex (flex_data.pools.ft_pool[i]);
  }

  flex_data.ftno = 0;
}

free_flex_temps()
{
  int i;

  for (i=0;i<sizeof(flex_data.sets.ft_buf)/sizeof(flex_data.sets.ft_buf[0]);i++)
  { free_flex_pool (&(flex_data.pools.ft_pool[i]));
    flex_data.sets.ft_buf[i] = 0;
  }

  flex_data.ftno = 0;
}

void free_flex_pool ();
flex_set
reuse_flex (s, nume, num_sets, attrs)
flex_set* s;
int nume;
int num_sets;
int attrs;
{
  assert (s != 0);

  /* Pick a new pool if this set has not been used before, or
     re-initialise the old pool if it has.  Return a fresh
     set from the cleaned pool. */

  if (*s == 0)
    if (num_sets == 0)
      ;
    else
      *s = alloc_flex (new_flex_pool (0, nume, num_sets, attrs));
  else
  { flex_pool* fp = FLEX_POOL (*s);
    assert (fp > flex_data.flexp && fp < fp + sizeof(flex_data.flexp)/sizeof(flex_data.flexp[0]));
    if (num_sets == 0)
    { free_flex_pool (&fp);
      *s = 0;
    }
    else
    { new_flex_pool (&fp, nume, num_sets, attrs);
      *s = alloc_flex (fp);
    }
  }

  return *s;
}

void
free_flex_pool (pool)
flex_pool** pool;
{
  if (*pool)
  {
    flex_data.next_pid = *pool - &flex_data.flexp[0];

    FREE_POOL_TEXT (*pool);

    /* clear out the space in the flex pool array */
    bzero (*pool, sizeof (**pool));
  }

  *pool = 0;
}

flex_set
alloc_flex (pool)
flex_pool *pool;
{
  int need_longs;

  need_longs = pool->lps * (pool->num_sets_used+1);

  if (pool->longs_allocated < need_longs)
  {
    REALLOC_POOL_TEXT (need_longs, pool);
    CLEAR_POOL_TEXT (pool->longs_allocated, need_longs, pool);
    pool->longs_allocated = need_longs;
  }

  pool->num_sets_used += 1;

  return PACK_FLEX(pool - &flex_data.flexp[0], pool->num_sets_used - 1);
}

flex_set
pop_flex (pool)
flex_pool *pool;
{
  pool->num_sets_used -= 1;
  return PACK_FLEX(pool - &flex_data.flexp[0], pool->num_sets_used - 1);
}

sized_flex_set
alloc_sized_flex (pool)
flex_pool *pool;
{
  sized_flex_set ret;
  ret.first = -1;
  ret.last = -1;
  ret.cnt = 0;
  ret.body = alloc_flex (pool);
  return ret;
}

void
change_flex_pool_size (pool, num_longs)
flex_pool* pool;
int num_longs;
{
  int need_longs;

  assert (num_longs != 0);

  num_longs += 1;  /* for the LO and HI indicators */

  need_longs = num_longs * pool->num_sets_used;

  if ((unsigned)num_longs > pool->lps)
  {
    int i = pool->num_sets_used;

    if (pool->longs_allocated < need_longs)
    {
      REALLOC_POOL_TEXT (need_longs, pool);
      pool->longs_allocated = need_longs;
    }

    while (i--)
    {
      int j = pool->lps;

      unsigned long *dst = pool->text + i * num_longs;
      unsigned long *src = pool->text + i * j;

      /* zero out the end of the new set ... */
      bzero (dst+j, (num_longs - j) * sizeof (long));

      /* copy the old text to the new area ... */
      while (j--)
        dst[j] = src[j];
    }
  }
  else if ((unsigned)num_longs < pool->lps)
    /* FIXTHIS ... Handle truncation later */
    assert (0);

  pool->lps = num_longs;
}

flex_set
get_empty_flex (p)
flex_pool* p;
{
  return PACK_FLEX(p - flex_data.flexp, 0);
}

flex_set
get_flex (p,e)
flex_pool* p;
int e;
{
  return PACK_FLEX(p - flex_data.flexp, e);
}

flex_set
pick_flex_temp (s)
flex_set s;
{
  int w;

  if (IS_SPARSE(s))
  { w = last_flex (s);
    w = MIN_FLEX_BODY_SIZE (w);
  }
  else
    w = FLEX_BODY_ALLOC(s);

  return GRAB_FLEX(w);
}

int
is_empty_flex (s)
flex_set s;
{
  unsigned long *lo, *hi;
  unsigned long *fbody_p;
  flex_pool * fpool_p;

  if (s == 0)
    return 1;

  GET_FLEX_BODY(s, fpool_p, fbody_p);

  return (GET_FLEX_HI(fbody_p) == 0);
}

int
equal_flex (a, b)
flex_set a;
flex_set b;
{
  /* return 1 iff a and b are the same. */

  int ret;

  if (a==0)
    ret = is_empty_flex (b);

  else if (b==0)
    ret = is_empty_flex (a);

  else
  { ret = a==b;

    if (!ret)
    { unsigned long *ap,*bp,ax,ay,bx,by;
      int a_sparse;
      int b_sparse;
      flex_pool * tpool_p;

      a_sparse = IS_SPARSE(a);
      b_sparse = IS_SPARSE(b);

      if (a_sparse != b_sparse)
      {
        if (a_sparse)
          a = copy_flex(GRAB_FLEX(4), a);
        if (b_sparse)
          b = copy_flex(GRAB_FLEX(4), b);
        a_sparse = 0;
        b_sparse = 0;
      }

      
      GET_FLEX_BODY(a, tpool_p, ap);
      ax = GET_FLEX_LO(ap);
      if (a_sparse)
        ax = 0;
      ay = GET_FLEX_HI(ap);

      GET_FLEX_BODY(b, tpool_p, bp);
      bx = GET_FLEX_LO(bp);
      if (b_sparse)
        bx = 0;
      by = GET_FLEX_HI(bp);

      ret = ax==bx && ay==by;

      while (ret && ax != ay)
        ret = (ap[ax++]==bp[bx++]);
    }
  }

  return ret;
}

int
in_flex (s, elt)
flex_set s;
int elt;
{ 
  unsigned long *p;
  flex_pool *tpool_p;
  int s_hi;
  int i;

  if (s == 0)
    return 0;

  GET_FLEX_BODY(s, tpool_p, p);
  s_hi = GET_FLEX_HI(p);

  if (s_hi == 0)
    return 0;

  if (!IS_SPARSE(s))
  {
    i = elt/HINT_BITS;
    if (i >= s_hi)
      return 0;
    return ((p[i] & (1 << (elt%HINT_BITS))) != 0);
  }

  /* Sparse case, do linear search.  Sets should be short. */
  elt += 1;
  s_hi <<= 1;

  for (i = 0; i < s_hi; i++)
  {
    if (((unsigned short *)p)[i] >= (unsigned)elt)
      return (((unsigned short *)p)[i] == elt);
  }
  return 0;
}

int
next_flex (src,e)
flex_set src;
register int e;
{
  register int l;
  register int h;
  register unsigned long *p;
  register flex_pool * tpool_p;

  if (src == 0)
    return -1;

  GET_FLEX_BODY(src, tpool_p, p);
  h = GET_FLEX_HI(p);
  if (h == 0)
    return -1;

  if ((tpool_p->attrs & F_SPARSE) != 0)
  {
    if (e == -1)
    {
      PCK_FLEX_LO_HI(p, 0, GET_FLEX_HI(p));
      return (((unsigned short *)p)[0] - 1);
    }

    /* make hi be last non-zero element */
    h = (h << 1) - 1;
    if (((unsigned short *)p)[h] == 0)
      h--;

    e += 1;  /* make up for these being stored one larger */
    l = GET_FLEX_LO(p);
    while (l >= 0 && ((unsigned short *)p)[l] > (unsigned)e)
      l --;

    while (l < h && ((unsigned short *)p)[l + 1] < (unsigned)e)
      l ++;

    if (l < h)
    {
      l += 1;
      PCK_FLEX_LO_HI(p, l, GET_FLEX_HI(p));
      return (((unsigned short *)p)[l] - 1);
    }

    PCK_FLEX_LO_HI(p, 0, GET_FLEX_HI(p));
    return -1;
  }

  /* non sparse case */
  {
    register int i, j;
    register unsigned long mask;
    register unsigned long mask1;

    e += 1;
    l = GET_FLEX_LO(p);

    i = e / HINT_BITS;
    if (i < l)
    {
      i = l;
      j = 0;
      mask = 0xFFFFFFFF;
      mask1 = 1;
    }
    else {
      j = e % HINT_BITS;
      mask = 0xFFFFFFFF << j;
      mask1 = 1 << j;
    }

    for (; i < h; i++)
    {
      register unsigned long v;
      v = p[i];
      for (; j < HINT_BITS && ((v & mask) != 0); j++)
      {
        if ((v & mask1) != 0)
          return (i * HINT_BITS) + j;
        mask <<= 1;
        mask1 <<= 1;
      }

      j = 0;  /* for next loop pass */
      mask = 0xFFFFFFFF;
      mask1 = 1;
    }

    return -1;
  }
}

int
next_sized_flex (src,elt)
sized_flex_set *src;
int elt;
{
  if (src->cnt == 0)
    return -1;

  if (elt == -1 || elt < src->first)
    return src->first;

  if (elt >= src->last)
    return -1;

  return next_flex (src->body, elt);
}

void
and_flex_into (dst, src)
flex_set dst;
flex_set src;
{
  flex_pool *tpool_p;
  unsigned long *dst_body;
  unsigned long *src_body;
  int dst_lo, dst_hi, src_lo, src_hi;
  register int i;
  register int hi;
  register int lo;

  GET_FLEX_BODY(dst, tpool_p, dst_body);
  dst_hi = GET_FLEX_HI(dst_body);
  if (dst_hi == 0)
    /* its already the empty set, and itersection won't change this */
    return;

  dst_lo = GET_FLEX_LO(dst_body);

  if (IS_SPARSE(src))
    src = copy_flex(GRAB_FLEX(4), src);

  GET_FLEX_BODY(src, tpool_p, src_body);
  src_hi = GET_FLEX_HI(src_body);
  src_lo = GET_FLEX_LO(src_body);

  /* figure approximate new lo and hi bounds */
  hi = dst_hi;
  if (hi > src_hi)
    hi = src_hi;

  lo = dst_lo;
  if (lo < src_lo)
    lo = src_lo;

  for (i = dst_lo; i < lo; i++)
    dst_body[i] = 0;

  for (i = hi; i < dst_hi; i++)
    dst_body[i] = 0;

  dst_lo = lo;
  dst_hi = lo;
  for (i = lo; i < hi; i++)
  {
    unsigned long val;

    val = dst_body[i] & src_body[i];
    dst_body[i] = val;
    if (val != 0)
      dst_hi = i + 1;
    else if (dst_lo == i)
      dst_lo = i + 1;
  }

  if (dst_lo >= dst_hi)
    dst_lo = dst_hi = 0;

  PCK_FLEX_LO_HI(dst_body, dst_lo, dst_hi);
}

flex_set
and_flex (out, a, b)
flex_set out;
flex_set a;
flex_set b;
{
  int i, hi, lo;
  flex_set r;
  flex_pool *tpool_p;
  unsigned long *r_body;
  unsigned long *a_body;
  unsigned long *b_body;
  int a_lo, a_hi, b_lo, b_hi;

  if (IS_SPARSE(a))
    a = copy_flex(GRAB_FLEX(4), a);
  if (IS_SPARSE(b))
    b = copy_flex(GRAB_FLEX(4), b);

  if (a != 0)
  {
    GET_FLEX_BODY(a, tpool_p, a_body);
    a_lo = GET_FLEX_LO(a_body);
    a_hi = GET_FLEX_HI(a_body);
  }
  else
  {
    a_lo = 0;
    a_hi = 0;
  }

  if (b != 0)
  {
    GET_FLEX_BODY(b, tpool_p, b_body);
    b_lo = GET_FLEX_LO(b_body);
    b_hi = GET_FLEX_HI(b_body);
  }

  hi = a_hi;
  if (b_hi < hi)
    hi = b_hi;

  lo = a_lo;
  if (b_lo > lo)
    lo = b_lo;

  if (hi <= lo)
    hi = (lo = 0);

  if (out == 0)
    out = GRAB_FLEX (hi);

  if (IS_SPARSE(out))
    r = pick_flex_temp (out);
  else
    r = out;

  if ((unsigned)hi > FLEX_BODY_ALLOC(r))
  {
    change_flex_pool_size (FLEX_POOL(r), hi);

    /* if a and/or b are in the same pool this
     * could change their addresses so need to recompute them.
     */
    if (FLEX_POOL_NUM(a) == FLEX_POOL_NUM(r))
      GET_FLEX_BODY(a, tpool_p, a_body);

    if (FLEX_POOL_NUM(b) == FLEX_POOL_NUM(r))
      GET_FLEX_BODY(b, tpool_p, b_body);
  }

  GET_FLEX_BODY(r, tpool_p, r_body);

  i = GET_FLEX_HI(r_body);
  while (i > hi)
  {
    i --;
    r_body[i] = 0;
  }

  i = hi;
  while (i > 0)
  {
    unsigned long val;
    i --;
    val = a_body[i] & b_body[i];
    r_body[i] = val;

    if (val != 0)
      lo = i;
    else if (hi == i + 1)
      hi = i;
  }

  if (lo >= hi)
    lo = hi = 0;

  PCK_FLEX_LO_HI(r_body, lo, hi);

  if (r != out)
    copy_flex (out, r);

  return out;
}

void
clr_flex (r)
flex_set r;
{
  flex_pool *tpool_p;
  register int lo;
  register int hi;
  unsigned long *r_body;

  if (r == 0)
    return;

  GET_FLEX_BODY(r, tpool_p, r_body);

  if ((tpool_p->attrs & F_SPARSE) != 0)
    lo = 0;
  else
    lo = GET_FLEX_LO(r_body);
  hi = GET_FLEX_HI(r_body);

  while (hi != lo)
    r_body[--hi] = 0;

  PCK_FLEX_LO_HI(r_body, 0, 0);
} 

void
fill_flex (r)
flex_set r;
{
  int wds,t;
  flex_pool *tpool_p;
  unsigned long *body_p;

  assert (!IS_SPARSE(r));

  GET_FLEX_BODY(r, tpool_p, body_p);

  wds = FLEX_BODY_ALLOC(r);
  t   = wds;

  while (wds--)
    body_p[wds] = -1;

  PCK_FLEX_LO_HI(body_p, 0, t);
} 

void
and_compl_flex_into (dst, src)
flex_set dst;
flex_set src;
{
  register int i;
  register unsigned long *dst_body;
  register unsigned long *src_body;
  register int dst_hi;
  register int dst_lo;
  register int quit;
  flex_pool *tpool_p;

  if (src == 0)
    return;

  GET_FLEX_BODY(dst, tpool_p, dst_body);
  if ((dst_hi = GET_FLEX_HI(dst_body)) == 0)
    return;

  GET_FLEX_BODY(src, tpool_p, src_body);
  if (GET_FLEX_HI(src_body) == 0)
    return;

  if ((tpool_p->attrs & F_SPARSE) != 0)
  {
    /*
     * just run through the elements in the sparse set src,
     * clearing them out of the dst set.
     */
    for (i = next_flex(src, -1); i != -1; i = next_flex(src, i))
      clr_flex_elt(dst, i);
    return;
  }

  /* both dense */
  dst_lo = GET_FLEX_LO(dst_body);

  i = dst_hi;
  if (i > GET_FLEX_HI(src_body))
    i = GET_FLEX_HI(src_body);

  quit = dst_lo;
  while (i > quit)
  {
    unsigned long val;
    i --;
    val = dst_body[i] &~ src_body[i];
    dst_body[i] = val;
    if (dst_hi == i + 1 && val == 0)
      dst_hi = i;
  }

  while (dst_lo < dst_hi && dst_body[dst_lo] == 0)
    dst_lo ++;

  if (dst_lo >= dst_hi)
    dst_lo = dst_hi = 0;

  PCK_FLEX_LO_HI(dst_body, dst_lo, dst_hi);
}

flex_set
and_compl_flex (out, a, b)
flex_set out;
flex_set a;
flex_set b;
{
  register int i;
  register unsigned long *a_body;
  register unsigned long *b_body;
  register unsigned long *r_body;
  register int a_hi;
  register int b_hi;
  register int r_hi;
  register int r_lo;
  unsigned long val;
  flex_pool *tpool_p;
  flex_set r;

  if (IS_SPARSE(a))
    a = copy_flex(GRAB_FLEX(4), a);
  if (IS_SPARSE(b))
    b = copy_flex(GRAB_FLEX(4), b);

  GET_FLEX_BODY(a, tpool_p, a_body);
  a_hi = GET_FLEX_HI(a_body);

  if (out == 0)
    out = GRAB_FLEX (a_hi);

  if (IS_SPARSE(out))
    r = pick_flex_temp (out);
  else
    r = out;

  GET_FLEX_BODY(r, tpool_p, r_body);

  if ((unsigned)a_hi > FLEX_BODY_ALLOC(r))
  {
    change_flex_pool_size (tpool_p, a_hi);
    GET_FLEX_BODY(r, tpool_p, r_body);

    /* if a and/or b are in the same pool this
     * could change their addresses so need to recompute them.
     */
    if (FLEX_POOL_NUM(a) == FLEX_POOL_NUM(r))
      GET_FLEX_BODY(a, tpool_p, a_body);

    if (FLEX_POOL_NUM(b) == FLEX_POOL_NUM(r))
      GET_FLEX_BODY(b, tpool_p, b_body);
  }
  else if (a_hi < GET_FLEX_HI(r_body))
    bzero (r_body+a_hi,(GET_FLEX_HI(r_body)-a_hi) * 4);

  b_hi = 0;
  if (b != 0)
  {
    GET_FLEX_BODY(b, tpool_p, b_body);
    b_hi = GET_FLEX_HI(b_body);
  }

  i = a_hi;
  r_lo = i;
  r_hi = i;
  while (i > b_hi)
  {
    i--;
    r_body[i] = val = a_body[i];
    if (val != 0)
      r_lo = i;
  }

  while (i > 0)
  {
    i --;
    val = a_body[i] &~ b_body[i];
    r_body[i] = val;
    if (val != 0)
      r_lo = i;
    else if (r_hi == i + 1)
      r_hi = i;
  }

  if (r_lo >= r_hi)
    r_lo = r_hi = 0;

  PCK_FLEX_LO_HI(r_body, r_lo, r_hi);

  if (r != out)
    copy_flex (out, r);

  return out;
}

/*
 * Union flex set src into the flex set denoted by dst.
 * Dst is guaranteed to be large enough to hold any elements in src.
 * Dst is also guaranteed to be a non-sparse set.
 */
void
union_flex_into (dst, src)
flex_set dst;
flex_set src;
{
  flex_pool * tpool_p;
  register unsigned long *dst_body;
  register unsigned long *src_body;
  register long dst_lo;
  register long dst_hi;
  register long src_lo;
  register long src_hi;

  GET_FLEX_BODY(src, tpool_p, src_body);
  if (GET_FLEX_HI(src_body) == 0)
    return;

  if ((tpool_p->attrs & F_SPARSE) != 0)
  {
    src = copy_flex(GRAB_FLEX(4), src);
    GET_FLEX_BODY(src, tpool_p, src_body);
  }

  GET_FLEX_BODY(dst, tpool_p, dst_body);

  /*
   * only need to or src into dst, therefore bounds of loop are
   * srcs bounds.
   */
  src_lo = GET_FLEX_LO(src_body);
  src_hi = GET_FLEX_HI(src_body);

  /*
   * actual bounds or resulting set will be the lower of
   * the two lower bounds, and the larger of the two upper bounds.
   * If dst is empty set, lo, hi are artificially low, in that case
   * bounds of resulting set are just src bounds.
   * Set dst_lo, dst_hi to the resulting bounds.
   */
  dst_hi = GET_FLEX_HI(dst_body);
  if (dst_hi == 0)
  {
    /* dst is empty */
    dst_lo = src_lo;
    dst_hi = src_hi;
  }
  else
  {
    dst_lo = GET_FLEX_LO(dst_body);
    if (src_lo < dst_lo)
      dst_lo = src_lo;

    if (src_hi > dst_hi)
      dst_hi = src_hi;
  }

  for (; src_lo < src_hi; src_lo++)
    dst_body[src_lo] |= src_body[src_lo];

  PCK_FLEX_LO_HI(dst_body, dst_lo, dst_hi);
}

void
set_flex_elt (s, elt)
flex_set s;
unsigned long elt;
{
  register unsigned long *s_body;
  register unsigned old_hi;
  register int i;
  register int j;
  register int term;
  register unsigned long mask;
  register flex_pool *tpool_p;

  GET_FLEX_BODY(s, tpool_p, s_body);
  old_hi = GET_FLEX_HI(s_body);

  if ((tpool_p->attrs & F_SPARSE) == 0)
  {
    i = elt / HINT_BITS;
    mask = 1 << (elt % HINT_BITS);

    if ((unsigned)i >= FLEX_BODY_ALLOC(s))
    {
      /* get one word extra so we don't have to do this too often */
      change_flex_pool_size(tpool_p, i + 1); 
      GET_FLEX_BODY(s, tpool_p, s_body);
    }

    s_body[i] |= mask;
    if (old_hi == 0)
      PCK_FLEX_LO_HI(s_body, i, i+1);
    else
    {
      int lo = GET_FLEX_LO(s_body);
      if (i < lo)
        PCK_FLEX_LO_HI(s_body, (lo = i), old_hi);
      if (i >= old_hi)
        PCK_FLEX_LO_HI(s_body, lo, i+1);
    }
    return;
  }

  /* SPARSE case */
  /*
   * search for place to put the element,
   * if its already in set, just return.
   */
  /*
   * elements in a sparse set are stored as their actual value + 1.
   */
  elt += 1;

  if (old_hi == 0)
  {
    ((unsigned short *)s_body)[0] = elt;
    PCK_FLEX_LO_HI(s_body, GET_FLEX_LO(s_body), 1);
    return;
  }

  /*
   * figure out the start point for the search.  It either starts at the last
   * element if the last element is < elt, or at thefirst element.
   * Since a lot of calls to this routine are either done in
   * increasing order of elt this catches a common case quickly.
   */
  i = ((old_hi << 1) - 1);
  if (((unsigned short *)s_body)[i] == 0)
    i--;

  if (((unsigned short *)s_body)[i] < elt)
    i = i + 1;
  else
    i = 0;
  
  term = FLEX_BODY_ALLOC(s) << 1;
  for (; 1; i++)
  {
    if (i >= term || ((unsigned short *)s_body)[i] > elt)
      break;

    if (((unsigned short *)s_body)[i] == 0)
    {
      ((unsigned short *)s_body)[i] = elt;
      if ((i >> 1) == old_hi)
        PCK_FLEX_LO_HI(s_body, GET_FLEX_LO(s_body), old_hi + 1);
      return;
    }

    if (((unsigned short *)s_body)[i] == elt)
      return;
  }
  
  /* see if its full, reallocate it bigger if it is. */
  if (((unsigned short *)s_body)[term-1] != 0)
  {
    /* make it big enough to accomadate 4 more elements */
    change_flex_pool_size (tpool_p, (term+4) >> 1);
    GET_FLEX_BODY(s, tpool_p, s_body);
  }

  /*
   * Figure out the new hi bound now. If the last slot in old_hi -1 is
   * zero now then old_hi is not changing, Otherwise the hi is old_hi + 1.
   */
  j = (old_hi << 1) - 1;
  if (((unsigned short *)s_body)[j] != 0)
  {
    PCK_FLEX_LO_HI(s_body, GET_FLEX_LO(s_body), old_hi + 1);
    j += 1;
  }

  for (/* j already set up */; j > i; j--)
    ((unsigned short *)s_body)[j] = ((unsigned short *)s_body)[j-1];

  ((unsigned short *)s_body)[i] = elt;
}

void
clr_flex_elt (s, elt)
flex_set s;
int elt;
{
  register unsigned long *s_body;
  register unsigned old_hi;
  register int i;
  register int j;
  register int term;
  register unsigned long mask;
  register flex_pool *tpool_p;

  GET_FLEX_BODY(s, tpool_p, s_body);
  old_hi = GET_FLEX_HI(s_body);

  if (old_hi == 0)
    return;

  if ((tpool_p->attrs & F_SPARSE) == 0)
  {
    i = elt / HINT_BITS;
    mask = 1 << (elt % HINT_BITS);
    s_body[i] &= ~mask;

    if (i == GET_FLEX_LO(s_body))
    {
      while (i < old_hi && s_body[i] == 0)
        i++;
      if (i >= old_hi)
        i = old_hi = 0;
      PCK_FLEX_LO_HI(s_body, i, old_hi);
    }
    else if (i == (old_hi - 1))
    {
      while (i != 0 && s_body[i] == 0)
        i--;
      /* can't be empty, otherwise it would have been caught above. */
      PCK_FLEX_LO_HI(s_body, GET_FLEX_LO(s_body), i + 1);
    }
    return;
  }

  /* SPARSE case */
  /*
   * search for element to delete.
   * if its not already in set, just return.
   */
  /*
   * elements in a sparse set are stored as their actual value + 1.
   */
  elt += 1;

  term = old_hi << 1;
  for (i = 0; 1; i++)
  {
    if (i >= term ||
        ((unsigned short *)s_body)[i] > (unsigned)elt ||
        ((unsigned short *)s_body)[i] == 0)
      /* its not in the set, so return */
      return;

    if (((unsigned short *)s_body)[i] == elt)
      break;
  }
  
  /* close up the set */
  j = (old_hi << 1) - 1;
  for (/* i already set up */; i < j; i++)
    ((unsigned short *)s_body)[i] = ((unsigned short *)s_body)[i+1];
  ((unsigned short *)s_body)[j] = 0;

  /* get new hi */
  if (s_body[old_hi-1] == 0)
    PCK_FLEX_LO_HI(s_body, GET_FLEX_LO(s_body), old_hi - 1);
}

void
set_sized_flex_elt (r, elt)
sized_flex_set *r;
int elt;
{
  if (r->cnt == 0 || elt < r->first || elt > r->last || !in_flex (r->body, elt))
  {
    if (r->cnt == 0)
      r->first = (r->last = elt);
    else
    { if (elt < r->first)
        r->first = elt;

      if (elt > r->last)
        r->last = elt;
    }
    set_flex_elt (r->body, elt);
    r->cnt++;
  }
}

int
last_flex (s)
flex_set s;
{
  unsigned long *p;
  unsigned long h;
  unsigned short *p_short;
  flex_pool *tpool_p;
  int ret;

  GET_FLEX_BODY(s, tpool_p, p);
  p_short = (unsigned short *)p;
  
  h = GET_FLEX_HI(p);
  if (h == 0)
    return -1;

  if (IS_SPARSE(s))
  {
    ret = p_short[h*2-1]-1;
    if (ret == -1)
      ret = p_short[h*2-2]-1;
  }
  else
  {
    unsigned long t = p[h-1];
    assert (t);

    ret = h * 32 - 1;

    while ((t & 0x80000000) == 0)
    {
      t <<= 1;
      ret--;
    }
  }

  return ret;
}

flex_set
copy_flex (r, s)
flex_set r;
flex_set s;
{
  flex_pool *tpool_p;
  unsigned long *r_body;
  unsigned long *s_body;
  int s_hi;
  int r_hi;
  register int i;

  if (r == s)
    return r;

  GET_FLEX_BODY(s, tpool_p, s_body);
  GET_FLEX_BODY(r, tpool_p, r_body);
  s_hi = GET_FLEX_HI(s_body);

  i = 0;
  if ( IS_SPARSE(r) ) i |= 2;
  if ( IS_SPARSE(s) ) i |= 1;
  switch ( i )
  {
    case 0:
      /* both dense */
    case 3:
      /* both sparse */
      /* make sure r is big enough to hold all of s */
      if ((unsigned)s_hi > FLEX_BODY_ALLOC(r))
      {
        /*
         * Note that r & s cannot be from the same pool if they aren't
         * the same size, so no need to worry about updating s.
         */
        change_flex_pool_size (FLEX_POOL(r), s_hi);
        GET_FLEX_BODY(r, tpool_p, r_body);
      }

      /* clear the upper part of r */
      for (i = GET_FLEX_HI(r_body)-1; i >= s_hi; i--)
        r_body[i] = 0;

      /* now just copy the words one by one. -1 copies HI and LO indicators */
      for (i = -1; i < s_hi; i++)
        r_body[i] = s_body[i];
      return r;

    case 1:
      /* r is dense, s is sparse */
      {
        int r_max_elt;
        int r_min_elt;

        /* clear r first */
        r_hi = GET_FLEX_HI(r_body);
        for (i = GET_FLEX_LO(r_body); i < r_hi; i++)
          r_body[i] = 0;

        if (s_hi == 0)
        {
          /* if s is empty just set the LO and HI and return. */
          PCK_FLEX_LO_HI(r_body, 0, 0);
          return r;
        }

        /*
         * find max element of s, see if r is big enough to hold it.
         * if not allocate r bigger.
         */
        i = (s_hi << 1) - 1;  /* i is index into approx last elem in set */
        if (((unsigned short *)s_body)[i] == 0)
          i--;

        r_max_elt = ((unsigned short *)s_body)[i] - 1;
        r_hi = (r_max_elt / HINT_BITS) + 1;
        if ((unsigned)r_hi > FLEX_BODY_ALLOC(r))
        {
          /*
           * Note that r & s cannot be from the same pool if they aren't
           * the same size, so no need to worry about updating s_body.
           */
          change_flex_pool_size (FLEX_POOL(r), r_hi);
          GET_FLEX_BODY(r, tpool_p, r_body);
        }

        /* now copy the elements into the set */
        for (; i >= 0; i--)
        {
          r_min_elt = ((unsigned short *)s_body)[i] - 1;
          r_body[r_min_elt / HINT_BITS] |= 1 << (r_min_elt % HINT_BITS);
        }

        /* set LO and HI and return r */
        PCK_FLEX_LO_HI(r_body, r_min_elt / HINT_BITS, r_hi);
        return r;
      }

    case 2:
      /* r is sparse, s is dense */
      {
        register int mask;
        register int j;

        /* clear r first */
        for (i = GET_FLEX_HI(r_body)-1; i >= 0; i--)
          r_body[i] = 0;

        /*
         * essentially just next through the set s finding all elements and
         * adding them to r.  If r gets to big, grow it.
         */
        r_hi = 0;
        for (i = GET_FLEX_LO(s_body); i < s_hi; i++)
        {
          unsigned long val = s_body[i];
          for (j = 0, mask = 0x1; val != 0 && j < HINT_BITS; j++, mask <<= 1)
          {
            if ((val & mask) != 0)
            { 
              if (r_hi == FLEX_BODY_ALLOC(r)*2)
              {
                /*
                 * Note that r & s cannot be from the same pool if they aren't
                 * the same size, so no need to worry about updating s_body.
                 */
                change_flex_pool_size (FLEX_POOL(r), FLEX_BODY_ALLOC(r)+1);
                GET_FLEX_BODY(r, tpool_p, r_body);
              }
              ((unsigned short *)r_body)[r_hi++] = (i * HINT_BITS + j) + 1;
              val &= ~mask;
            }
          }
        }

        PCK_FLEX_LO_HI(r_body, 0, (r_hi+1) / 2);
        return r;
      }

    default:
      abort();
  }
}

int
full_sparse_flex(s)
flex_set s;
{
  flex_pool *tpool_p;
  unsigned long *body;

  GET_FLEX_BODY(s, tpool_p, body);
  if (((unsigned short *)body)[((tpool_p->lps - 1) << 1) - 1] == 0)
    return 0;
  return 1;
}

void
dump_flex (file, src)
FILE* file;
flex_set src;
{
  int prev  = -2;
  int i     = -1;
  int state = 0;

  fprintf (file, "[");

  while ((i=next_flex(src,i)) != -1)
  {
    switch (state)
    {
      case 0:
        fprintf (file, "%d", i);
        state = 1;
        break;

      case 1:
        if (prev == i-1)
          state = 2;
        else
          fprintf (file, ",%d", i);
        break;

      case 2:
        if (prev != i - 1)
        {
          fprintf (file, "..%d,%d", prev, i);
          state = 1;
        }
        break;
    }
    prev = i;
  }

  if (state == 2)
    fprintf (file, "..%d", prev);

  fprintf (file, "]");
}

void
dump_sized_flex (file, src)
FILE* file;
sized_flex_set *src;
{
  fprintf (file, "c,f,l:%d,%d,%d ", src->cnt, src->first, src->last);
  dump_flex (file, src->body);
}

void
debug_flex (src)
flex_set src;
{
  int i;
  fprintf (stderr, "flex_set %d,%d: ", FLEX_POOL_NUM(src), FLEX_SET_NUM(src));
  dump_flex (stderr, src);
  fprintf (stderr, "\n");
}
#endif
