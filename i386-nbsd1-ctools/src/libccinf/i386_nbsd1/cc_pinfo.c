/******************************************************************/
/*       Copyright (c) 1990,1991,1992,1993 Intel Corporation

   Intel hereby grants you permission to copy, modify, and 
   distribute this software and its documentation.  Intel grants
   this permission provided that the above copyright notice 
   appears in all copies and that both the copyright notice and
   this permission notice appear in supporting documentation.  In
   addition, Intel grants this permission provided that you
   prominently mark as not part of the original any modifications
   made to this software or documentation, and that the name of 
   Intel Corporation not be used in advertising or publicity 
   pertaining to distribution of the software or the documentation 
   without specific, written prior permission.  

   Intel Corporation provides this AS IS, without any warranty,
   including the warranty of merchantability or fitness for a
   particular purpose, and makes no guarantee or representations
   regarding the use of, or the results of the use of, the software 
   and documentation in terms of correctness, accuracy, reliability, 
   currentness, or otherwise; and you rely on the software, 
   documentation and results solely at your own risk.
 */
/******************************************************************/

#include <stdio.h>
#include "cc_info.h"
#include "cc_pinfo.h"
#include "assert.h"

static unsigned char *
fdef_prof(p)
st_node* p;
{
  unsigned char* ret;

  assert (p->rec_typ == CI_FDEF_REC_TYP || p->rec_typ == CI_LIB_FDEF_REC_TYP);
  ret = CI_LIST_TEXT (p, CI_FDEF_PROF_LIST);

  return ret;
}

int
fdef_prof_size(p)
st_node* p;
{
  unsigned char* t;
  int i;

  t = fdef_prof (p);
  CI_U32_FM_BUF(t-4,i);		/* Length of counts vector */
  i -= 4;
  assert (i >= 0 && (i & 3) == 0);
  return i;
}

int
db_fdef_has_prof(p)
st_node* p;
{
  return fdef_prof_size (p) != 0;
}

int
db_fdef_prof_change (p,q)
st_node* p;
st_node* q;
{
  int r = 1;
  int n = fdef_prof_size(p);

  /* p and q refer to the same function.  return 1 iff the counts
     the code generator sees for q will be different than they
     were for p. */
    
  if (n == fdef_prof_size(q))
  { r = 0;

    if (n != 0 && memcmp (fdef_prof(p), fdef_prof(q), n))
      r = 1;
  }

  return r;
}

/*
 * add_counts performs saturating addition of t1 and t2.
 * This assures counters don't accidentally wrap around.
 */
static
unsigned long
add_counts(t1, t2)
unsigned long t1;
unsigned long t2;
{
  unsigned long t3 = t1 + t2;

  if (t3 < t1)
    t3 = 0xFFFFFFFF;

  return t3;
}

/*
 * sub_counts performs saturating subtraction of t2 from t1.
 * This assures counters don't accidentally wrap-around.
 */
static
unsigned long
sub_counts(t1, t2)
unsigned long t1;
unsigned long t2;
{
  unsigned long t3 = t1 - t2;

  if (t3 > t1)
    t3 = 0;

  return t3;
}

static unsigned
mul_counts(a,b)
unsigned a;
unsigned b;
{
  unsigned t,u;

  if (a < b)
  { t = a; a = b; b = t; }

  t = (unsigned) -1;

  if (b <= 0xffff)
    if ((u = (a >> 16) * b) <= 0xffff)
      t = u << 16;

  u = (a & 0xffff) * (b & 0xffff);
  a = t + u;

  if (a < t || a < u)
    a = -1;

  return a;
}


static int
quality_range(n)
int n;
{
  /* Return an estimate of the quality of a profile with
     internal quality level n, as follows:

    -1    == no profile
     0    == complete guess
     1..8 == interpolated
     9    == "perfect" */

  int ret;

  assert (n >= 0 && n <= CI_MAX_PROF_QUALITY);

  if (n == 0)
    ret = -1;

  else if (n == 1)
    ret = 0;

  else if (n == CI_MAX_PROF_QUALITY)
    ret = 9;

  else
    /* Map 2..65534 to 1..8 */
    ret = 1 + (n-2) / ((CI_MAX_PROF_QUALITY-2) / 7);

  return ret;
}

int
fdef_prof_quality(p)
st_node *p;
{
  int t;
  CI_U16_FM_BUF(p->db_rec + CI_FDEF_PQUAL_OFF, t);
  assert (t <= CI_MAX_PROF_QUALITY);
  return t;
}

int
db_fdef_prof_quality(p)
{ return quality_range (fdef_prof_quality(p));
}

int
set_fdef_prof_quality(p,t)
st_node *p;
unsigned t;
{
  assert (t <= CI_MAX_PROF_QUALITY);
  CI_U16_TO_BUF(p->db_rec + CI_FDEF_PQUAL_OFF, t);
  return t;
}

static int
fdef_prof_voters(p)
st_node *p;
{
  int t;
  CI_U16_FM_BUF(p->db_rec + CI_FDEF_VOTER_OFF, t);
  assert ((t!=0) == db_fdef_has_prof(p));
  return t;
}

int
set_fdef_prof_voters(p,t)
st_node *p;
unsigned t;
{
  t &= 0xffff;
  CI_U16_TO_BUF(p->db_rec + CI_FDEF_VOTER_OFF, t);
  assert ((t!=0) == db_fdef_has_prof(p));
  return t;
}


static
unsigned voter_lcm;

static void
compute_voter_lcm (db)
dbase* db;
{
  int stabi;
  st_node* fdef;

  voter_lcm = 1;

  for (stabi=0; stabi < CI_HASH_SZ; stabi++)
    for (fdef = db->db_stab[stabi]; fdef; fdef = fdef->next)
      if (fdef->db_rec_size && CI_ISFDEF(fdef->rec_typ))
      { unsigned t = fdef_prof_voters (fdef);
        if (t && (voter_lcm % t))
          if (t % voter_lcm)
            voter_lcm *= t;
          else
            voter_lcm = t;
      }
}

double
db_fdef_prof_counter(p, n)
st_node* p;
unsigned n;
{
  unsigned char* t;
  unsigned i;
  double ret;

  n *= 4;
  t = fdef_prof (p);

  CI_U32_FM_BUF(t-4,i);		/* Length of counts vector */
  i -= 4;
  assert ((i & 3) == 0 && n < i);

  t += n;
  CI_U32_FM_BUF (t, n);

  if (voter_lcm)
  { unsigned v = fdef_prof_voters(p);
    assert (v && (voter_lcm % v) == 0);

    n = mul_counts (n, voter_lcm / v);
  }

  /*
   * all counts is data base are multiplied by 100, divide by 100 before
   * giving to "user's" count.
   */
  ret = n;
  ret = ret / 100.0;
  return ret;
}

static unsigned long
raw_prof_count (iprof_data, file_offset, info_offset)
unsigned char* iprof_data;
int file_offset;
int info_offset;
{
  unsigned long count;
  unsigned char	*ctr_p;

  if (iprof_data == NULL || file_offset == -1 || info_offset == -1)
    return 0;

  ctr_p = iprof_data + file_offset + info_offset;

  CI_U32_FM_BUF (ctr_p, count);
  return (count);
}

unsigned char *
db_unpk_num(p, num_p)
unsigned char *p;
int *num_p;
{
  int t;
  switch (((t = *p++) >> 6) & 0x3)
  {
    case 0:
      if ((t & 0x20) != 0)
        t |= (-1 << 6);
      break;

    case 1:
      t &= 0x3F;
      t |= *p++ << 6;
      if ((t & 0x2000) != 0)
        t |= (-1 << 14);
      break;

    case 2:
      t &= 0x3F;
      t |= *p++ << 6;
      t |= *p++ << 14;

      if ((t & 0x200000) != 0)
        t |= (-1 << 22);
      break;

    case 3:
      t &= 0x3F;
      t |= *p++ << 6;
      t |= *p++ << 14;
      t |= *p++ << 22;

      if ((t & 0x20000000) != 0)
        t |= (-1 << 30);
      break;
  }
  *num_p = t;
  return p;
}

static
void
add_u32_to_64(hi_p, lo_p, a32)
long *hi_p;
unsigned long *lo_p;
unsigned long a32;
{
  long res_hi = *hi_p;
  unsigned long res_lo = *lo_p;
  unsigned long old_lo = res_lo;

  res_lo += a32;

  if (res_lo < old_lo)
    res_hi += 1;

  *hi_p = res_hi;
  *lo_p = res_lo;
}

static
void
sub_u32_fm_64(hi_p, lo_p, a32)
long *hi_p;
unsigned long *lo_p;
unsigned long a32;
{
  long res_hi = *hi_p;
  unsigned long res_lo = *lo_p;
  unsigned long old_lo = res_lo;

  res_lo -= a32;

  if (res_lo > old_lo)
    res_hi -= 1;

  *hi_p = res_hi;
  *lo_p = res_lo;
}

static void
add_64s(hi_p, lo_p, hi1_p, lo1_p)
long *hi_p;
unsigned long *lo_p;
long *hi1_p;
unsigned long *lo1_p;
{
  long res_hi = *hi_p;
  unsigned long res_lo = *lo_p;
  unsigned long old_lo = res_lo;

  res_lo += *lo1_p;
  res_hi += *hi1_p;

  if (res_lo < old_lo)
    res_hi += 1;
  
  *hi_p = res_hi;
  *lo_p = res_lo;
}

static void
shr64_by_1(hi_p, lo_p)
long *hi_p;
unsigned long *lo_p;
{
  int lo_hi_bit;
  long res_hi = *hi_p;
  unsigned long res_lo = *lo_p;

  lo_hi_bit = res_lo >> 31;

  res_lo <<= 1;
  res_hi <<= 1;

  res_hi |= (lo_hi_bit & 1);

  *lo_p = res_lo;
  *hi_p = res_hi;
}

static void
mul_64_by_u32(hi_p, lo_p, a32)
long *hi_p;
unsigned long *lo_p;
unsigned long a32;
{
  int i;
  long res_hi = 0;
  unsigned long res_lo = 0;
  long t_hi = *hi_p;
  unsigned long t_lo = *lo_p;

  for (i = 0; i < 32; i++)
  {
    if ((a32 & (1 << i)) != 0)
      add_64s(&res_hi, &res_lo, &t_hi, &t_lo);

    shr64_by_1(&t_hi, &t_lo);
  }

  *hi_p = res_hi;
  *lo_p = res_lo;
}

static unsigned char *
unpk_compute_form(iprof_data, buf_p, file_off, count_p)
unsigned char * iprof_data;
unsigned char * buf_p;
int file_off;
unsigned long *count_p;
{
  long count_hi = 0;
  unsigned long count_lo = 0;
  int tmp;

  buf_p = db_unpk_num(buf_p, &tmp);
  while (tmp != 0)
  {
    if (tmp < 0)
      sub_u32_fm_64(&count_hi, &count_lo,
                    raw_prof_count(iprof_data, file_off, (-tmp-1)*4));
    else
      add_u32_to_64(&count_hi, &count_lo,
                    raw_prof_count(iprof_data, file_off, (tmp-1)*4));

    buf_p = db_unpk_num(buf_p, &tmp);
  }

  /*
   * multiply by 100, this allows direct correlation with
   * estimated profiles.
   */
  mul_64_by_u32(&count_hi, &count_lo, 100);

  /* now make 64 bit count fit in unsigned long */
  if (count_hi < 0)
  {
    count_lo = 0;
    count_hi = 0;

    /* warning, underflow in profile counter calculation */
  }

  if (count_hi != 0)
  {
    count_hi = 0;
    count_lo = 0xFFFFFFFF;

    /* warning, overflow in profile counter calculation */
  }

  *count_p = count_lo;
  return buf_p;
}

static int
line_cmp(s1, s2)
struct lineno_block_info *s1, *s2;
{
  if (s1->lineno > s2->lineno)
    return 1;

  if (s1->lineno < s2->lineno)
    return -1;

  if (s1->id > s2->lineno)
    return 1;
  else
    return -1;
}

static void
init_fdef_prof_counts (prof_db, prof_sym, prof_cnts, nprof)
dbase* prof_db;
st_node* prof_sym;
unsigned char* prof_cnts;
int nprof;
{ 
  st_node* fdef;
  int i;

  for (i = 0; i < CI_HASH_SZ; i++)
    for (fdef = prof_db->db_stab[i]; fdef != 0; fdef = fdef->next)
      if (fdef->db_rec_size && CI_ISFDEF(fdef->rec_typ))
      {
        char* file = (char*) ((fdef)->db_list[CI_FDEF_FILE_LIST]->text+4);
      
        /* If fdef is defined in prof_sym->name ... */
        if (!strcmp (file, prof_sym->name))
        {
          db_list_node* l = GET_FDEF_PROF(fdef);
          unsigned lo, hi, size;
      
          /* Determine which of the counts which belongs to the function ... */
          db_func_block_range (fdef, &lo, &hi);
          assert (lo != -1 && hi >= lo);
          size = ((hi-lo)+1) * 4;
      
          /* We have counts, so we better have had instrumentation. */
          assert (lo != ((unsigned)-1) && lo <= hi);
      
          /* Replace the existing counts, which may be empty or may have been
             defaulted by the compiler.  If the latter, the size should
             match exactly and we can reuse the space. */

          if (l->size != size)
          { assert (l->size == 0);
            l = db_new_list (0, prof_cnts + 4*lo, size);
            SET_FDEF_PROF (fdef, l);
          }

          memcpy (l->text+4, prof_cnts + 4*lo, size);
      
          set_fdef_prof_quality (fdef, CI_MAX_PROF_QUALITY);
          set_fdef_prof_voters  (fdef, nprof);
        }
      }
}

void
dbp_assign_var_counts (prof_db)
dbase* prof_db;
{ 
  st_node* fdef;
  int stabi;

  for (stabi=0; stabi < CI_HASH_SZ; stabi++)
    for (fdef = prof_db->db_stab[stabi]; fdef; fdef = fdef->next)
      if (fdef->db_rec_size && CI_ISFDEF(fdef->rec_typ) &&
          db_fdef_has_prof (fdef))
      {
        unsigned char* file = CI_LIST_TEXT (fdef, CI_FDEF_FILE_LIST);
        st_node*   prof_sym = dbp_lookup_sym (prof_db, file);

        unsigned lo, hi, b;
        unsigned char* p;
      
        assert (prof_sym != 0 && prof_sym->rec_typ == CI_PROF_REC_TYP);

        p = CI_LIST_TEXT (prof_sym, CI_PROF_FNAME_LIST);
      
        db_func_block_range (fdef, &lo, &hi);
    
        /* We better have had instrumentation. */
        assert (lo != ((unsigned)-1) && lo <= hi);

        for (b = 0; b <= hi; b++)
        { int i;

          p += strlen (p) + 1;
          
          while (((p = db_unpk_num(p, &i)), i != 0)) ;	/* skip line numbers */
          while (((p = db_unpk_num(p, &i)), i != 0)) ;	/* skip formulas */

          while (*p)
          {
            if (b >= lo)
            { st_node* v = dbp_lookup_sym (prof_db, p);

              if (v != 0 && v->rec_typ == CI_VDEF_REC_TYP)
              { unsigned c = db_rec_var_usage (v);
                c = add_counts (c, (unsigned)db_fdef_prof_counter (fdef, b-lo));
                set_db_rec_var_usage (v, c);
              }
            }

            p += strlen(p)+1;
          }
          p += 1;
        }
      }
}

void
dbp_print_profile_counts (prof_db, f)
dbase* prof_db;
named_fd* f;
{
  int stabi;
  st_node *prof_sym;
    
  for (stabi=0; stabi < CI_HASH_SZ; stabi++)
    for (prof_sym = prof_db->db_stab[stabi]; prof_sym; prof_sym=prof_sym->next)
      if (prof_sym->db_rec_size && prof_sym->rec_typ == CI_PROF_REC_TYP)
      { unsigned lo, hi;
        int n_blocks, j, lines_printed, n_lines, line_cnt, is_prof;
        unsigned char *p, *file_name;
        struct lineno_block_info *lines;
        st_node* fdef;

        CI_U32_FM_BUF(prof_sym->db_rec + CI_PROF_NBLK_OFF, n_blocks);
        CI_U32_FM_BUF(prof_sym->db_rec + CI_PROF_NLINES_OFF, n_lines);
      
        file_name = CI_LIST_TEXT (prof_sym, CI_PROF_FNAME_LIST);
        p = file_name + strlen (file_name) + 1;
          
        lines = (struct lineno_block_info *)
                  db_malloc(sizeof(struct lineno_block_info) * n_lines);
          
        line_cnt = 0;
        for (j = 0; j < n_blocks; j++)
        { unsigned lineno;
      
          while ((p = db_unpk_num(p, &lineno)), lineno)
          { /* record basic block to line number correlation */
      
            if (line_cnt >= n_lines)
              db_fatal("profile information has been corrupted");
      
            lines[line_cnt].id = line_cnt;
            lines[line_cnt].lineno = lineno;
            lines[line_cnt].blockno = j;
            line_cnt += 1;
          }
      
          /* skip formulas and variable info */
          while ((p = db_unpk_num(p, &lineno)), lineno) ;
          while (*p != 0) p += strlen(p)+1;
          p++;
        }
          
        /* sort the lines array before printing */
        qsort(lines, n_lines, sizeof(struct lineno_block_info), line_cmp);
        
        lines_printed = 0;
        lo = hi = -1;
        fdef = 0;
        is_prof = 0;

        for (j = 0; j < n_lines; j++)
        { unsigned b = lines[j].blockno;
      
          assert (b != -1);
      
          if (fdef == 0 || b < lo || b > hi)
          { fdef = dbp_prof_block_fdef (prof_db, prof_sym, b, &lo, &hi);
            is_prof = db_fdef_has_prof (fdef);
          }

          assert (lo != -1);
      
          if (is_prof)
          { int qual = db_fdef_prof_quality (fdef);
            int votes = fdef_prof_voters (fdef);
            char note[128];

            assert (qual >= 0 && qual < 10);

            if (qual == 0)
              sprintf (note, "guess");
            else if (qual == 9)
              sprintf (note, "%d raw inputs", votes);
            else
              sprintf (note, "%d stretched inputs", votes);
       
            if (lines_printed == 0)
            {
              fprintf (f->fd, "^L\n");
              fprintf (f->fd, "Profile counts for module %s=%s\n",
                       file_name, prof_sym->name);
              fprintf (f->fd, "Function name                    ");
              fprintf (f->fd, "  Line#  Block#   Times hit   From\n");
              fprintf (f->fd, "================================|");
              fprintf (f->fd, "========|=======|===========|======\n");
              lines_printed = 5;
            }
            fprintf (f->fd, "%-32s|%7u |%6u |%10u | %s\n", fdef->name,
                             GET_LINE(lines[j].lineno),
                             b, (unsigned)db_fdef_prof_counter(fdef, b-lo),
                             note);
            lines_printed += 1;
          }
        
          if (lines_printed >= db_page_size())
            lines_printed = 0;
        }
      }
}

static char* iprof_comment;

static void
iprof_open_read (fd, name)
named_fd* fd;
char* name;
{
  /* Open name as ascii if it is a text profile file, binary otherwise. */

  db_comment (&iprof_comment, " %s", name);

  if (db_is_iprof(name) > 1)
  { dbf_open_read_ascii (fd, name, db_fatal);
    db_comment (&iprof_comment, "(ascii)");
  }
  else
  { dbf_open_read (fd, name, db_fatal);
    db_comment (&iprof_comment, "(binary)");
  }
}

static void
iprof_read_int (fd, buf, n)
named_fd* fd;
unsigned char* buf;
int n;
{
  /* Read 'n' integers into buf.  If the file is ascii, use fscanf,
     else, just read 'em straight in. */

  if (fd->is_ascii)
  {
    while (n--)
    { int v;

      if (fscanf (fd->fd, "%ld", &v) != 1)
        db_fatal ("read failed on '%s'; expected a decimal integer", fd->name);

      CI_U32_TO_BUF (buf, v);
      buf += 4;
    }
  }
  else
    dbf_read (fd, buf, n * 4);
}

int
db_get_iprof_data (pmap_fd, iprofc, iprofv, iprof_data, expected_len)
named_fd* pmap_fd;
int iprofc;
char** iprofv;
unsigned char** iprof_data;
int expected_len;
{
  /* Read and merge the raw profiles in iprofv into the array iprof_data.

     If pmap_fd is available, check it's mod time against the time of each
     raw profile, and warn for each raw profile which is older than the map;
     also, check the length of of each raw profile against the expected
     length from pmap_fd.
  */

  named_fd iprof_fd;
  int i,len;
  unsigned char buf[4];

  db_comment (&iprof_comment, "reading raw profiles:");
  assert (iprofc > 0);

  /* read first iprof, get len ... */
  iprof_open_read (&iprof_fd, iprofv[0]);
  iprof_read_int (&iprof_fd, buf, 1);
  CI_U32_FM_BUF(buf, len);
  if (len <= 0 || (len & 3))
    db_fatal ("illegal length field '%d' in raw profile '%s'",
              len, iprof_fd.name);

  if (pmap_fd && expected_len && len != expected_len)
    db_fatal ("raw profile '%s' has length field %d, but '%s' expects %d",
              iprof_fd.name, len, pmap_fd->name, expected_len);

  /* Allocate the array, read profile data, close the file. */
  *iprof_data = (unsigned char*) db_malloc (len);
  iprof_read_int (&iprof_fd, *iprof_data, len / 4);
  dbf_close(&iprof_fd);

  /* Read the rest of the iprof files and merge 'em into iprof_data. */
  for (i = 1; i < iprofc; i++)
  { int n;
    unsigned char* t = *iprof_data;

    iprof_open_read (&iprof_fd, iprofv[i]);
    iprof_read_int (&iprof_fd, buf, 1);
    CI_U32_FM_BUF(buf, n);

    if (n != len)
      db_fatal ("data length '%d' in '%s' is inconsistent with '%d' in '%s'",
                n, iprofv[i], len, iprofv[0]);

    while (n != 0)
    { unsigned c1, c2;

      CI_U32_FM_BUF(t, c1);

      iprof_read_int (&iprof_fd, buf, 1);
      CI_U32_FM_BUF(buf, c2);

      c1 = add_counts(c1, c2);
      CI_U32_TO_BUF(t, c1);

      t += 4;
      n -= 4;
    }

    dbf_close (&iprof_fd);
  }

  db_comment (&iprof_comment, "\n");
  return len;
}

static int
get_iprof_map (pmap_fd, iprofc, iprofv, iprof_map, prof_db)
named_fd* pmap_fd;
int iprofc;
char** iprofv;
unsigned char** iprof_map;
dbase* prof_db;
{
  unsigned char buf[4];
  int len, cur, end, iprof_len;
  time_t expected_db_time;

  assert (prof_db && pmap_fd);
  dbf_open_read (pmap_fd, db_pdb_file (0, "prof.map"), db_fatal);

  /* Read check value of the associated pass1.db ... */
  dbf_read (pmap_fd, buf, sizeof(int));
  CI_U32_FM_BUF(buf, expected_db_time);

  /* Get expected raw profile length ... */
  dbf_read (pmap_fd, buf, sizeof(int));
  CI_U32_FM_BUF(buf, iprof_len);

  /* Get profile map length ... */
  dbf_read (pmap_fd, buf, sizeof(int));
  CI_U32_FM_BUF(buf, len);

  cur = dbf_tell (pmap_fd);
  dbf_seek_end (pmap_fd, 0);
  end = dbf_tell (pmap_fd);
  dbf_seek_set (pmap_fd, cur);

  if (len <= 0 || iprof_len <= 0 || (iprof_len & 3) != 0 || end != cur + len)
    if (expected_db_time == end-4)
      db_fatal ("'%s' has an obsolete format; you should rebuild",
                pmap_fd->name);
    else
      db_fatal ("'%s' is corrupted; you should rebuild", pmap_fd->name);

  /* Allocate space, read profile map, close the file */
  *iprof_map = (unsigned char*) db_malloc (len);
  dbf_read (pmap_fd, *iprof_map, len);
  dbf_close(pmap_fd);

  return iprof_len;
}

static void
attach_iprof_data (prof_db, iprofc, iprofv)
dbase* prof_db;
int iprofc;
char** iprofv;
{
  int i, iprof_len;

  unsigned char* iprof_map  = 0;
  unsigned char* iprof_data = 0;

  named_fd pmap_fd;

  assert (prof_db && iprofc && iprofv);

  iprof_len = get_iprof_map (&pmap_fd, iprofc, iprofv, &iprof_map, prof_db);
  assert (iprof_map && pmap_fd.name);

  db_get_iprof_data (&pmap_fd, iprofc, iprofv, &iprof_data, iprof_len);
  assert (iprof_data);

  /* Now we must build the information from the counters and formulas. */
  for (i = 0; ; i += 8)
  {
    int file_name_index;
    st_node *prof_sym;

    CI_U32_FM_BUF(iprof_map + i, file_name_index);

    if (file_name_index == 0)
      break;

    /*
     * lookup the file name in the symbol table.  The profile RECORD should
     * be stored using this name.
     */

    prof_sym = dbp_lookup_sym (prof_db, (iprof_map + file_name_index));
    if (prof_sym != 0 && prof_sym->rec_typ == CI_PROF_REC_TYP)
    {
      int n_blocks;
      int file_offset;
      unsigned long count;
      int j;
      unsigned char * prof_info;
      unsigned char * prof_cnts;
      unsigned char * file_name;
      int file_name_length;
      unsigned char *bp;

      CI_U32_FM_BUF(iprof_map + i + 4, file_offset);
      CI_U32_FM_BUF(prof_sym->db_rec + CI_PROF_NBLK_OFF, n_blocks);
      file_name = CI_LIST_TEXT (prof_sym, CI_PROF_FNAME_LIST);
      file_name_length = strlen(file_name);
      prof_info = file_name + file_name_length + 1;

      prof_cnts = (unsigned char *)db_malloc(sizeof(int) * n_blocks);

      for (j = 0; j < n_blocks; j++)
      { int lineno;

        while ((prof_info = db_unpk_num(prof_info, &lineno)), lineno)
          ;

        prof_info = unpk_compute_form(iprof_data, prof_info,file_offset, &count);
        bp = &prof_cnts[j*4];
        CI_U32_TO_BUF(bp, count);

        while (*prof_info != 0)
          prof_info += strlen(prof_info)+1;

        prof_info += 1;
      }

      init_fdef_prof_counts (prof_db, prof_sym, prof_cnts, iprofc);

      prof_sym->file_name = db_malloc (file_name_length + 1);
      strcpy(prof_sym->file_name, file_name);
    }
    else
      db_fatal ("missing profile record for prof.map entry '%s'",
                iprof_map+file_name_index);
  }
  free (iprof_map);
  free (iprof_data);
}

static dbase* merge_out;

static void
merge_spf_into_fdef (spf,i)
st_node *spf;
int i;
{
  if (spf->db_rec_size && CI_ISFDEF(spf->rec_typ))
  {
    st_node* out = dbp_lookup_sym (merge_out, spf->name);

    if (out == 0)
      ; /* Function no longer interesting; no ccinfo in merge_out */
    else
    {
      int out_qual = fdef_prof_quality (out);
      int spf_qual = fdef_prof_quality (spf);

      /* Don't even bother unless it is possible for the spf to be as good
         as what we have. */

      if (spf_qual >= out_qual)
        spf_qual = stretch_fdef_prof (spf, out);

      if (out_qual < spf_qual)
      { /* This spf is better than what we had; keep it instead. */

        SET_FDEF_PROF (out, GET_FDEF_PROF(spf));
        set_fdef_prof_quality (out, spf_qual);
        set_fdef_prof_voters  (out, fdef_prof_voters (spf));
      }

      else if (out_qual > spf_qual)
        ; /* Ignore this spf;  we already have better stuff. */

      else
      { /* Merge counts when quality levels are identical, and remember
           how many distinct opinions about the function's profile
           we now have by adding the voter counts together. */

        unsigned char* t0 = fdef_prof (out);
        unsigned char* t1 = fdef_prof (spf);

        int n = fdef_prof_size (out) / 4;

        while (n--)
        { unsigned c0, c1;

          CI_U32_FM_BUF(t0, c0);
          CI_U32_FM_BUF(t1, c1);

          c0 = add_counts (c0, c1);
          CI_U32_TO_BUF(t0, c0);
          t0 += 4;
          t1 += 4;
        }

        set_fdef_prof_voters (out, fdef_prof_voters(spf)+fdef_prof_voters(out));
      }
    }
  }
}

void
dbp_get_prof_info (pmerge_out, iprofc, iprofv, sprofc, sprofv)
dbase* pmerge_out;
int iprofc;
char** iprofv;
int sprofc;
char** sprofv;
{
  static char* sprof_comment;

  int i;

  merge_out = pmerge_out;

  if (iprofc)
    attach_iprof_data (pmerge_out, iprofc, iprofv);

  if (sprofc)
    db_comment (&sprof_comment, "reading spf profiles:");

  for (i = 0; i < sprofc; i++)
  { named_fd sp;
    dbase sp_db;

    db_comment (&sprof_comment, " %s", sprofv[i]);

    memset (&sp_db, 0, sizeof (sp_db));
    dbf_open_read (&sp, sprofv[i], db_fatal);
    dbp_read_ccinfo (&sp_db, &sp);
    dbp_for_all_sym (&sp_db, merge_spf_into_fdef);

    dbf_close (&sp);
  }

  if (sprofc)
    db_comment (&sprof_comment, "\n");

  compute_voter_lcm (pmerge_out);
}

