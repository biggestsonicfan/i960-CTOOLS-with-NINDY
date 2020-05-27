/*
    Link-time substitution and object module dependency analysis code.

    Copyright (C) 1995 Intel Corporation.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 2.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include "assert.h"

#include "cc_info.h"
#include "gcdm960.h"
#include "subst.h"
#include "i_toolib.h"

static struct subst_idx_ {
  unsigned char idx;
  char* text;
} *subst_idx;

int
get_target (p, i)
st_node* p;
int i;
{
  unsigned long sz [NUM_TS];

  db_unpack_sizes (db_tsinfo(p), sz);
  return sz[i];
}

int
set_fdef_size(p, sz)
st_node* p;
{
  FDX(p).fdef_size = sz;
  return sz;
}

int
get_fdef_size(p)
st_node* p;
{
  return FDX(p).fdef_size;
}


static int
regex_match (r, s)
char* r;
char* s;
{
  int c,n,i,j;
  char* re;

  /* r is a shell-style pattern; s is a string.  If s matches r, return
     1, else return 0. */

  if (r == 0)
    r = "";

  if (s == 0)
    s = "";

  /* Translate the pattern the user gave us to an RE in the 'ex' style.
     That is:

     - The only special characters the user knows are * ? [] and ![],
       and all others are to be literals.  So, we quote all characters
       special to the RE package with '\'.

     - We translate "*" to ".*"

     - We translate "?" to "."

     - We translate ![chars] to [^chars].
  */

  /* Assume we are going to have to quote all of r */
  n  = strlen (r);
  re = db_malloc (2*n + 3);

  i=j=0;
  re[j++]='^';
  while (i < n)
  { c = r[i];
    if ((c=='^' && i==0) || (c=='$' && i==(n-1)) || c=='\\' || c=='.')
    { re[j++]='\\';
      re[j++]=c;
    }
    else if (c=='*')
    { re[j++]='.';
      re[j++]='*';
    }
    else if (c=='?')
      re[j++] = '.';
    else if (c=='!' && r[i+1]=='[')
      re[j++]='^';
    else if (c=='[')
    { re[j++] = '[';
      if (r[i+1]=='^')
      { re[j++]='\\';
      }
    }
    else
      re[j++] = c;
    i++;
  }

  re[j++]='$';
  re[j] = '\0';

  i = 0;

  if (re[0] == '\0')
    /* null RE matches only null string */
    i = (s[0]=='\0');
  else
    if (re_comp(re) != 0)
      db_fatal ("illegal module_name specification '%s'", r);
    else
      if ((i = re_exec (s)) == -1)
        db_fatal ("internal error processing module_name '%s'", r);

  return i;
}

static int
prog_write_del_fmt(p)
st_node* p;
{
  /* Return a mask with 1's for all of the lists we should delete on output,
     -1 if we should delete the entire record. */

  int typ = p->rec_typ;
  int ret = 0;

  switch (typ)
  { 
#if 0
    case CI_PROF_REC_TYP:
      ret = -1;
      break;
#endif

#if 0
    case CI_SRC_REC_TYP:
      ret = (1 << CI_SRC_SRC_LIST)
           |(1 << CI_SRC_CC1_LIST)
        ;
      break;
#endif
  }

  return ret;
}

static int
prog_rec_skip_fmt_(p)
st_node *p;
{
  /* Return a mask with 1's for all of the lists we should seek past. */

  int typ = p->rec_typ;
  int ret = 0;

  switch (typ)
  { case CI_SRC_REC_TYP:
      ret = (1 << CI_SRC_SRC_LIST)
           |(1 << CI_SRC_CC1_LIST)
        ;
      break;
  }

  return ret;
}

static int
prev_rec_skip_fmt_(p)
st_node *p;
{
  int typ = p->rec_typ;
  int ret = 0;

  /* Return a mask with 1's for all of the lists we should seek past. */

  return ret;
}

static int
prog_post_read_qsort_st_node(p, q)
st_node** p;
st_node** q;
{
  int ret;

  ret = (*p)->rec_typ - (*q)->rec_typ;

  if (ret == 0)
    switch ((*p)->rec_typ)
    {
      case CI_SRC_REC_TYP:
      { static char *pbuf, *qbuf;

        db_oname (&pbuf, *p);
        db_oname (&qbuf, *q);

        assert (*pbuf && *qbuf);

        ret = strcmp (pbuf, qbuf);
      }
    }

  if (ret == 0)
    ret = *p - *q;

  return ret;
}


static void prog_post_read_();

dbase prog_db = { prog_post_read_, prog_rec_skip_fmt_, 0, prog_write_del_fmt };
dbase prev_db = { 0, prev_rec_skip_fmt_ };

expand_gcdm_switches (argcp, argvp)
int* argcp;
char*** argvp;
{
  static char** argv;
  int    argc = 0;

  int i;

  for (i = 0; i < *argcp; i++)
  { char* s = (*argvp)[i];

    db_set_arg (&argc, &argv, s);

    if (i>0 && IS_OPTION_CHAR(*s) && !strncmp (s+1,"gcdm",4))
    { char *arg1, *arg2;

      flag_gcdm = 1;

      s += 5;

      if (*s=='\0')
        /* For now, allow gcdm switch argument to be in the following
           argv slot. */
        s = (*argvp)[++i];

      argc--;	/* Cover up the /gcdm */

      while (db_find_arg (&s, &arg1, &arg2))
      { db_set_arg (&argc, &argv, arg1);
        if (arg2)
          db_set_arg (&argc, &argv, arg2);
      }
    }
  }
  *argcp = argc;
  *argvp = argv;
  argv = 0;
}

static int
subst_opt_sort (p, q)
struct subst_idx_ *p;
struct subst_idx_ *q;
{
  int ret = q->idx - p->idx;

  if (ret == 0)
    ret = !!q->text - !!p->text;

  if (ret == 0)
    if (p->text)
      ret = strcmp (p->text, q->text);

  return ret;
}

static char* subst_kind[] = {
  "'nosubst'",
  "module-local",
  "program-wide",
  "module-local or program-wide",
};

static char** subst_opt;
static unsigned subst_cnt[4];
static unsigned char subst_msk[4];
#define SUBST_CNT (( subst_cnt[sizeof(subst_cnt)/sizeof(subst_cnt[0]) -1] ))

static void
init_subst_opt ()
{
  /* Set up tables for parsing subst modifiers.  */

  int argc = 0;
  char** argv = 0;

  /* nosubst must appear by itself */
  db_set_arg (&argc, &argv, "nosubst");
  subst_cnt[0] = argc;
  subst_msk[0] = 1;

  /* pass1 subst only */
  db_set_arg (&argc, &argv, "fprof");

  db_set_arg (&argc, &argv, "O0");
  db_set_arg (&argc, &argv, "O1");
  db_set_arg (&argc, &argv, "O2");
  db_set_arg (&argc, &argv, "O3");
  db_set_arg (&argc, &argv, "O4");

  subst_cnt[1] = argc;
  subst_msk[1] = 2;

  /* pass2 subst only */
  db_set_arg (&argc, &argv, "O5");
  db_set_arg (&argc, &argv, "fuse-lomem");
  db_set_arg (&argc, &argv, "fno-use-lomem");
  subst_cnt[2] = argc;
  subst_msk[2] = 4;

  /* Subst can be first or second pass */
  db_set_arg (&argc, &argv, "g");
  db_set_arg (&argc, &argv, "asm_pp+");
  db_set_arg (&argc, &argv, "clist+");
  db_set_arg (&argc, &argv, "fbbr");
  db_set_arg (&argc, &argv, "fno-bbr");
  db_set_arg (&argc, &argv, "fcoalesce");
  db_set_arg (&argc, &argv, "fno-coalesce");
  db_set_arg (&argc, &argv, "fcoerce");
  db_set_arg (&argc, &argv, "fno-coerce");
  db_set_arg (&argc, &argv, "fcondxform");
  db_set_arg (&argc, &argv, "fno-condxform");
  db_set_arg (&argc, &argv, "fconstcomp");
  db_set_arg (&argc, &argv, "fno-constcomp");
  db_set_arg (&argc, &argv, "fconstprop");
  db_set_arg (&argc, &argv, "fno-constprop");
  db_set_arg (&argc, &argv, "fconstreg");
  db_set_arg (&argc, &argv, "fno-constreg");
  db_set_arg (&argc, &argv, "fcopyprop");
  db_set_arg (&argc, &argv, "fno-copyprop");
  db_set_arg (&argc, &argv, "fdead_elim");
  db_set_arg (&argc, &argv, "fno-dead_elim");
  db_set_arg (&argc, &argv, "fmarry_mem");
  db_set_arg (&argc, &argv, "fno-marry_mem");
  db_set_arg (&argc, &argv, "fmix-asm");
  db_set_arg (&argc, &argv, "fno-mix-asm");
  db_set_arg (&argc, &argv, "fsblock");
  db_set_arg (&argc, &argv, "fno-sblock");
  db_set_arg (&argc, &argv, "fsched_sblock");
  db_set_arg (&argc, &argv, "fno-sched_sblock");
  db_set_arg (&argc, &argv, "fshadow-globals");
  db_set_arg (&argc, &argv, "fno-shadow-globals");
  db_set_arg (&argc, &argv, "fshadow-mem");
  db_set_arg (&argc, &argv, "fno-shadow-mem");
  db_set_arg (&argc, &argv, "fspace-opt");
  db_set_arg (&argc, &argv, "fno-space-opt");
  db_set_arg (&argc, &argv, "fsplit_mem");
  db_set_arg (&argc, &argv, "fno-split_mem");

  db_set_arg (&argc, &argv, "fcse-follow-jumps");
  db_set_arg (&argc, &argv, "fno-cse-follow-jumps");
  db_set_arg (&argc, &argv, "fcse-skip-blocks");
  db_set_arg (&argc, &argv, "fno-cse-skip-blocks");
  db_set_arg (&argc, &argv, "fexpensive-optimizations");
  db_set_arg (&argc, &argv, "fno-expensive-optimizations");
  db_set_arg (&argc, &argv, "fthread-jumps");
  db_set_arg (&argc, &argv, "fno-thread-jumps");
  db_set_arg (&argc, &argv, "fstrength-reduce");
  db_set_arg (&argc, &argv, "fno-strength-reduce");
  db_set_arg (&argc, &argv, "funroll-loops");
  db_set_arg (&argc, &argv, "fno-unroll-loops");
  db_set_arg (&argc, &argv, "funroll-all-loops");
  db_set_arg (&argc, &argv, "fno-unroll-all-loops");
  db_set_arg (&argc, &argv, "fpeephole");
  db_set_arg (&argc, &argv, "fno-peephole");
  db_set_arg (&argc, &argv, "ffunction-cse");
  db_set_arg (&argc, &argv, "fno-function-cse");
  db_set_arg (&argc, &argv, "frerun-cse-after-loop");
  db_set_arg (&argc, &argv, "fno-rerun-cse-after-loop");
  db_set_arg (&argc, &argv, "fschedule-insns");
  db_set_arg (&argc, &argv, "fno-schedule-insns");
  db_set_arg (&argc, &argv, "fschedule-insns2");
  db_set_arg (&argc, &argv, "fno-schedule-insns2");

  db_set_arg (&argc, &argv, "da");
  db_set_arg (&argc, &argv, "function+");

  subst_cnt[3] = argc;
  subst_msk[3] = 2|4;

  assert (SUBST_CNT == subst_cnt[3]);

  subst_opt = argv;

  /* We do not initialize this here because it is not a constant */
  subst_idx = (struct subst_idx_ *) db_malloc (argc * sizeof(subst_idx[0]));
};

static
parse_size (t)
char* t;
{
  unsigned long ts;

  if (t == 0)
    t = "";

  while (*t == '+')
    t++;

  if (!get_num (t, &ts) || ts == 0)
    db_fatal ("code_size argument '%s' is illegal", t);

  return ts;
}

static char* mfirst;
static char* mfollow;

static int
match_opt (p)
char* p;
{
  /*
     p points at what should be a substitution option.  If it
     matches an option we know about, return the index number of
     the matched option in the substitution table.

     We also return in mfirst the original text at 'p' for
     use in error message reporting, and we return in mfollow the
     argument (if any) that follows the substitution option.

     This routine is complicated by the fact that we also want to
     match some synonyms and have those cases return the index of
     an option in the table which may not exactly match the option
     at p.  For example, if p is "O2" and we were invoked by ic960,
     we want to match "O4".
  */

  int   i;
  char *t;
  
  static char* buf;

  /* Remember what we had originally, up to '+', for use in error messages. */
  db_buf_at_least (&mfirst, strlen(p)+32);
  strcpy (mfirst, p);
  if (t = strchr (mfirst,'+'))
    *t = '\0';

  /* Allocate space for arguments to be concatenated to the option */
  db_buf_at_least (&mfollow, strlen(p)+32);
  *mfollow = '\0';

  /* Work with copy of original string so we can change it if it is a synonym */
  db_buf_at_least (&buf, strlen(p)+32);
  strcpy ((t=buf), p);

  /* Map ic960 options O2..O4 to gcc960 -O4 */
  if (flag_ic960 &&t[0]=='O' &&t[1]>='2' &&t[1]<='4' &&(t[2]=='\0'||t[2]=='+'))
  { if (t[1] != '2')
      db_warning ("treating illegal ic960 substitution '+O%c' as '+O2'",t[1]);

    t[1] = '4';
  }

  /* See if we find a match for t in the subst table ... */
  for (i=0; i < SUBST_CNT; i++)
  { int n = strlen (subst_opt[i]);

    if (!strncmp(t,subst_opt[i],n) &&
         (  (t[n-1]=='+' && !(t[n]=='\0' || t[n]=='+'))
          ||(t[n-1]!='+' &&  (t[n]=='\0' || t[n]=='+'))))
    { /* We found a match. */

      if (t[n-1]=='+')
      { /* Get the extra stuff for multi-part options */ 
        strcpy (mfollow, t+n);
        if (t = strchr (mfollow, '+'))
          *t = '\0';
      }
      break;
    }
  }
  return i;
}

static void
parse_modifiers (m, mkind, info)
module_name* m;
mname_kind mkind;
subst_info* info;
{
  /* Parse a substitution request.

     Upon return, info->kind indicates the type (none, 1pass, 2pass) of
     substitution which will be done in the linker, and info->key defines the
     tag that we will embed in the substitution history for those modules
     which this subst gets associated with.

     info->pkey combines the current inlining level, the modifier list from the
     subst, and the 'kind' information into a tag which we use to
     store and look up historical information about the modules the
     subst is associated with.
  */
     
  int siz = 0;
  int cnt = 0;
  int kind = 7;
  char* t;

  char* modifier = m->modifier;

  memset (info, 0, sizeof (*info));

  if (mkind == mn_ref)
    return;

  if (modifier == 0)
    modifier = "";

  info->original_modifier = modifier;

  if (mkind == mn_size)
  { m->grp_ts = parse_size (modifier);
    return;
  }

  assert (mkind == mn_subst);

  if (subst_opt == 0)
    init_subst_opt ();

  kind = 7;

  /* Clear the vector that keeps track of the options we have seen. */
  memset (subst_idx, 0, SUBST_CNT * sizeof (subst_idx[0]));

  t = modifier;
  while (t && *t)
  { while (*t == '+')
      t++;

    if (*t)
    { int i,k;

      if ((i = match_opt (t)) >= SUBST_CNT)
        db_fatal ("'%s' is not legal in a subst spec", t);

      k = 0;
      while (i >= subst_cnt[k])
        k++;

      if ((kind &= subst_msk[k]) == 0)
        db_fatal ("'%s' is legal only in %s substitutions",
                  mfirst, subst_kind[k]);
      else
      { int n = strlen(subst_opt[i]) + strlen(mfollow);

        cnt++;
        if (subst_idx[i].idx)
          db_fatal ("duplicate '%s' in %s", mfirst, modifier);

        subst_idx[i].idx = i+1;

        assert (subst_idx[i].text == 0);

        db_buf_at_least (&(subst_idx[i].text), n+1);

        strcpy (subst_idx[i].text, subst_opt[i]);
        strcat (subst_idx[i].text, mfollow);
        siz += n + 2;

        if (!strcmp (subst_opt[i], "clist+"))
        { info->is_sinfo = 1;
          info->is_clist = 1;
        }
        else if (!strcmp (subst_opt[i], "fmix-asm"))
          info->is_sinfo = 1;
        else if (!strncmp (subst_opt[i], "asm_pp+", 7))
          info->asm_pp = strchr (subst_idx[i].text, '+') + 1;

        /* Get past the argument we just processed */
        if (*mfollow)
          t += strlen(mfirst) + strlen(mfollow) + 1;
        else
          t += strlen(mfirst);
      }
    }
  }

  if (cnt == 0)
  { cnt = 1;
    siz = strlen (t=subst_opt[0])+2;
    subst_idx[0].idx = 1;
    strcpy (db_buf_at_least (&(subst_idx[0].text), siz-1), t);
  }
  else
  { 
    qsort (subst_idx, SUBST_CNT, sizeof(subst_idx[0]), subst_opt_sort);

    /* Figure out whether we have type 0, 1, or 2 subst command.  If
       kind has more than 1 bit, we take the lowest. */

    assert (kind);
    while ((kind & 1) == 0)
    { kind >>= 1;
      info->kind++;
    }
  }

  info->key = t = (char*) db_malloc(5 + siz);

  *t++ = '0';
  *t++ = '0';

  /* Record the inline level so we can remember multiple sets of
     inline decisions.  For non-type 2 substs, we will never inline so
     we map all of these cases to inline level 0. */

  if (info->kind == 2)
    *t += inline_level();

  *t++ = '0' + info->kind;

  /* Lastly, stick the selected strings into the key, sorted by
     where they were in subst_opt. */

{ int i;
  for (i=0; i < cnt; i++)
  { assert (subst_idx[i].text);
    if (subst_idx[i].text[0])
    { *t++ = '+'; 
      strcpy (t, subst_idx[i].text);
      t += strlen (t);
    }
  }
}
  *t++ = '+';
  *t = '\0';

#ifdef DEBUG
  db_comment (0, "parsed %s substitution modifiers '%s' into key '%s'\n",
                 subst_kind[info->kind], modifier, info->key);
#endif
}

st_node*
get_bname (p)
st_node* p;
{
  if (CI_ISFDEF(p->rec_typ))
    p = dbp_bname_sym (&prog_db, CI_LIST_TEXT (p, CI_FDEF_FILE_LIST));

  assert (p->rec_typ == CI_SRC_REC_TYP);
  return p;
}

module_name*
ts_grp(p)
st_node* p;
{
  p = get_bname(p);
  return SRCX(p).raw_info[mn_size];
}

int
is_lib_member(p)
st_node* p;
{
  unsigned char* t;

  p = get_bname(p);
  t = CI_LIST_TEXT(p, CI_SRC_LIB_LIST);

  assert (t);

  return (*t != '\0');
}

int recomp_kind(p)
st_node* p;
{
  int ret;

  if (mname_head[mn_subst])
  { p   = get_bname(p);
    ret = SUBST_KIND(p);
  }
  else
    ret = is_lib_member(p) ? 0 : 2;

  return ret;
}

int
fdef_inlinable(p)
st_node* p;
{
  int ret = 0;

  assert (CI_ISFDEF(p->rec_typ));

  if (flag_no_inline_libs == 0 || !is_lib_member(p))
    ret = db_rec_inlinable(p);

  if (ret && mname_head[mn_subst])
  { /* For /subst users, we say not inlinable unless 2nd pass substitution */

    p = get_bname(p);
    if (SUBST_KIND(p) != 2)
      ret = 0;
  }
  return ret;
}

static int
is_force_ref(p)
st_node* p;
{
  /* Tell the caller if we are supposed to force addr_taken for p
     to be true because of a /ref command. */

  assert (p->rec_typ == CI_SRC_REC_TYP && p->extra);

  {
    module_name*   m = ((src_extra *)(p->extra))->raw_info[mn_ref];
    int          ret = (m && m->modifier && !strcmp (m->modifier, "ref"));

    return ret;
  }
}

static int
matched_module (p, m, info, mkind)
st_node *p;
module_name *m;
subst_info *info;
mname_kind mkind;
{
  src_extra *extra = (src_extra*) p->extra;
  int ret = 0;

  /* This record's archive:member has been matched with a subst/nosubst,
     ref/noref, or code_size.

     If we haven't seen a match for this record, attach the node to the record.
  */

  assert (m != 0 && p != 0 && p->rec_typ == CI_SRC_REC_TYP && extra != 0);

  if (extra->raw_info[mkind] == 0)
  { 
    static subst_info empty;
    assert (!memcmp (&(extra->parsed_info[mkind]), &empty, sizeof (empty)));

    ret = 1;

    extra->raw_info[mkind] = m;

    /* If we don't have source info, we cannot do a recompilation. */

    if ((mkind==mn_subst && info->kind && !(p->db_list[CI_SRC_SRC_LIST]->size))
      ||(mkind==mn_size && !(p->db_list[CI_SRC_SRC_LIST]->size)))
    { unsigned char* archive = p->db_list[CI_SRC_LIB_LIST]->text + 4;
      unsigned char* member  = p->db_list[CI_SRC_MEM_LIST]->text + 4;
      char* s = (mkind == mn_subst) ? "subst" : "code_size";
      
      if (*archive)
        db_warning
        ("ignoring '%s %s:%s %s';  the module has no '-fdb' info",
          s, archive, member, info->original_modifier);
      else
        db_warning
        ("ignoring '%s %s %s';  the module has no '-fdb' info",
          s, member, info->original_modifier);
    }
    else
      extra->parsed_info[mkind] = *info;
  }
  return ret;
}

module_name *mname_head[NUM_MKIND];
int num_mname[NUM_MKIND];

static void
expand_module_name (mkind)
mname_kind mkind;
{
  /* head is a list of substitution or ref commands collected from the
     option scanner in main().  We have since read the pass1 database, so
     we now have the information to expand the entries in the list.

     We walk the module_name list, parsing each node to get modifier, archive,
     and name fields.  

     If the node doesn't have a colon, we see if it is a public def in the
     database, and if so we call matched_module for the defining CI_SRC records
     of the def.

     If the node does have a colon, for all CI_SRC records in the database, we
     call regex_match on both the archive and member names.  For those cases
     where regex_match returns true for both the archive and member names, we
     call matched_module.

     Since the list is collected backwards, the head of the list is
     the last specification given by the user, and is thus the one that
     "sticks".  Thus, matched_module takes action only on the first instance
     of a subst or ref command for any given CI_SRC record. */

  module_name* head = mname_head[(int)mkind];
  module_name* p = head;
  dbase* db = &prog_db;
  int i, j;

  st_node** lo_src = db->extra->st_nodes + db->extra->first[CI_SRC_REC_TYP];
  st_node** hi_src = db->extra->st_nodes + db->extra->lim[CI_SRC_REC_TYP];

  while (p)
  {
    static char* match_com;
    char* m = p->text;
    int   saw_match = 0;
    char  match_mod[1024];

    subst_info info;

    while (*m)
    { if ((*m==':' && *(m+1)==':') ||
          (*m=='+' && *(m+1)=='+'))
      { for ((i=0),(j=strlen(m)); i < j; i++)
          m[i] = m[i+1];
      }
      else if (*m==':')
      { if (p->member)
          db_fatal ("unescaped ':' in library member name '%s'", p->member);
        else
        { p->member  = m+1;
          p->archive = p->text;
          *m = '\0';
        }
      }
      else if (*m=='+')
      { if (p->modifier)
          db_fatal ("'%s' is not allowed with '%s'", m, p->modifier);

        if (m==p->text && p->member == 0)
        { p->member  = "*";
          p->archive = "";
        }

        p->modifier = m+1;
        *m = '\0';
        break;
      }
      m++;
    }

    parse_modifiers (p, mkind, &info);

    if (mkind == mn_subst)
    {
      if (info.kind == 0)
        strcpy (match_mod, "nosubst");
      else
        sprintf (match_mod, "subst+%s", p->modifier);
    }
    else
      strcpy (match_mod, (p->modifier ? p->modifier : ""));

    db_comment (&match_com, "'%s ", match_mod);

    if (p->member == 0)
    { 
      /* User gave us a public definition.  Look it up; if it is not
         a var def or fdef, complain;  otherwise, loop thru the bname
         list invoking matched_module on each bname (it is possible to
         have multiple bnames because of common) */

      st_node* match = dbp_lookup_sym (db, p->text);
      assert (p->archive == 0);

      db_comment (&match_com, "%s'", p->text);

      if (match)
      { db_list_node* l;

        if (CI_ISFDEF(match->rec_typ))
          l = match->db_list[CI_FDEF_FILE_LIST];
        else if (CI_ISVDEF(match->rec_typ))
          l = match->db_list[CI_VDEF_FILE_LIST];
        else
          l = 0;

        if (l)
        { int n = l->size;
          unsigned char *q, *r;
          
          assert (l->text != 0 && n > 0);
          q = l->text + 4;
          r = q + n;

          while (q < r)
          { unsigned char *archive, *member;

            st_node* bname = dbp_bname_sym (db, q);
            assert (bname && bname->rec_typ == CI_SRC_REC_TYP);

            archive = bname->db_list[CI_SRC_LIB_LIST]->text + 4;
            member  = bname->db_list[CI_SRC_MEM_LIST]->text + 4;

            if (matched_module (bname, p, &info, mkind))
            {
              if (saw_match == 0)
              { db_comment (&match_com, " applies to");
                saw_match=1;
              }

              if (*archive)
                db_comment (&match_com, " %s:%s", archive, member);
              else
                db_comment (&match_com, " %s", member);
            }

            q += strlen((char*)q) + 1;
          }
        }
      }
    }
    else
    {
      st_node** n = lo_src;

      db_comment (&match_com, "%s:%s'", p->archive, p->member);

      while (n < hi_src)
      { unsigned char* archive = (*n)->db_list[CI_SRC_LIB_LIST]->text + 4;
        unsigned char* member  = (*n)->db_list[CI_SRC_MEM_LIST]->text + 4;
        if (regex_match (p->archive,archive) &&
            regex_match (p->member, member))
        { 
          if (matched_module (*n, p, &info, mkind))
          {
            if (saw_match==0)
            { db_comment (&match_com, " applies to");
              saw_match=1;
            }

            if (*archive)
              db_comment (&match_com, " %s:%s", archive, member);
            else
              db_comment (&match_com, " %s", member);
          }
        }
        n++;
      }
    }

    if (saw_match == 0)
    { /* Put out a warning that nothing matched */
      db_warning ("no -fdb modules match %s",match_com);
      match_com[0] = '\0';
    }
    else
      db_comment (&match_com, "\n");

    p = p->next;
  }
}


static int
set_change(f,v,c)
st_node* f;
int v;
char* c;
{
  assert (f && f->name && v > 0 && v < 3);
  (f)->sxtra = v;
  if (c)
    if (v == 1)
#ifdef DEBUG
      db_comment (0, "%s invalidates %s\n", c, f->name)
#endif
        ;
    else if (v == 2)
#ifdef DEBUG
      db_comment (0, "%s invalidates tclose (%s)\n", c, f->name)
#endif
        ;
    else
      assert (0);
      
  return v;
}

static int
alias_info_change(cur,prev)
st_node* cur;
st_node* prev;
{
  int ret = 0;
  assert (cur && prev);

  if ((CI_ISFUNC(cur->rec_typ) && !CI_ISFUNC(prev->rec_typ))
  ||  (CI_ISVAR(cur->rec_typ) && !CI_ISVAR(prev->rec_typ))
  ||  (db_rec_var_size(cur) != db_rec_var_size(prev))
  ||  (db_rec_addr_taken(cur) != db_rec_addr_taken(prev)))
    ret = 1;

  return ret;
}

/*
 * return 1 if the lomemability of cur is different from the
 * lomemability of prev.  A variable can be lomem'd if it was
 * allocated SRAM, or fast memory, and that fast memory is
 * at an absolute address < 4096.
 */
static int
lomem_info_change(cur, prev)
st_node *cur;
st_node *prev;
{
  unsigned long o_sram = 0;
  unsigned long n_sram = 0;

  if (CI_ISVAR(cur->rec_typ))
    n_sram = db_rec_var_sram (cur);

  if (CI_ISVAR(prev->rec_typ))
    o_sram = db_rec_var_sram (prev);

  /* see if prev is .lomem able, and that cur isn't */
  if (o_sram > 0 && o_sram < 4096 &&
      !(n_sram > 0 && n_sram < 4096))
    return 1;

  /* see if cur is .lomem able, and that prev wasn't */
  if (n_sram > 0 && n_sram < 4096 &&
      !(o_sram > 0 && o_sram < 4096))
    return 1;

  return 0;
}

static int
sram_binding_change(cur,prev)
st_node* cur;
st_node* prev;
{
  int ret = 0;

  assert (cur && prev && CI_ISVDEF(cur->rec_typ));

  { int sram = db_rec_var_sram (cur);

    if ((sram != db_rec_var_sram (prev))
    ||  (sram != 0 && db_rec_var_size(cur) != db_rec_var_size(prev)))
      ret = 1;
  }
  return ret;
}

int num_inlined_arcs;

int extra_size (t)
int t;
{
  char sz;

  if (t == CI_SRC_REC_TYP)
    sz = sizeof (src_extra);
  else if (CI_ISFDEF(t))
    sz = sizeof (fdef_extra);
  else if (CI_ISFUNC(t))
    sz = sizeof (fref_extra);
  else
    sz = 0;

  return sz;
}

static
char* prt_name (p, buf)
st_node* p;
char* buf;
{
  unsigned char* archive = (p)->db_list[CI_SRC_LIB_LIST]->text + 4;
  unsigned char* member  = (p)->db_list[CI_SRC_MEM_LIST]->text + 4;

  if (*archive)
    sprintf (buf, "'%s:%s'", archive, member);
  else
    sprintf (buf, "'%s'", member);
  return buf;
}

long
adjust_sizes (p, new_size)
st_node* p;
long new_size;
{ long sz[NUM_TS], old_size;
  int i;

  /* Get original sizes */
  db_unpack_sizes (db_tsinfo(p), sz);

  old_size = sz[TS_INL];

  /* Adjust sizes for phases by multiplying by (new_size/old_size).*/
  for (i = 0; i < NUM_TS; i++)
    sz[i] = db_size_ratio (new_size, old_size, sz[i]);

  /* Store them back */
  db_pack_sizes (db_tsinfo(p), sz);

  return old_size;
}

void
report_targets()
{
  module_name *m = mname_head[mn_size];
  while (m)
  { char buf [1024];
    st_node **lo = prog_db.extra->st_nodes+prog_db.extra->first[CI_SRC_REC_TYP];
    st_node **hi = prog_db.extra->st_nodes+prog_db.extra->lim[CI_SRC_REC_TYP];
    st_node **cur;

#if 0
    db_comment (0, "targeting %d bytes for %d bytes of post-inlining code:\n", m->grp_ts, m->grp_sz);
#endif

    /* Report code size targets for each module */
    for (cur=lo; cur < hi; cur++)
    { src_extra *s = &SRCX(*cur);
      if (m == s->raw_info[mn_size])
      { unsigned perc = db_size_ratio (s->mod_sz, m->grp_sz, 1);
        char *sn = prt_name (*cur, buf);
#if 0
        db_comment(0, "  %d bytes for %s (was %d)\n", s->mod_ts,sn,s->mod_sz);
#endif
      }
    }

    m = m->next;
  }
}

int
prep_code_sizes ()
{
  int typ;
  int ret = 0;
  
  /* Adjust the recorded sizes of each function def to account
     for optimization level differences between passes 1 and 2,
     and return an amount to be subtracted from the initial estimate
     of code space used. */

  for (typ = 0; typ < CI_MAX_REC_TYP; typ++)
    if (CI_ISFDEF(typ))
    { st_node **lo = prog_db.extra->st_nodes + prog_db.extra->first[typ];
      st_node **hi = prog_db.extra->st_nodes + prog_db.extra->lim[typ];
      st_node **cur;
  
      for (cur = lo; cur < hi; cur++)
        if (recomp_kind (*cur) >= 2)
        { long sz[NUM_TS];

          /* Get original sizes */
          db_unpack_sizes (db_tsinfo(*cur), sz);

          /* Accumulate amount spent on profiling instrumentation */
          ret += sz [TS_PROF];

          /* Store them back */
          db_pack_sizes (db_tsinfo(*cur), sz);
        }
    }
  return ret;
}

set_code_size_targets ()
{
  int typ;
  
  /* Adjust the recorded sizes of each function def to account
     for the global inlining we've done, and collect module and group
     totals. */

  for (typ = 0; typ < CI_MAX_REC_TYP; typ++)
    if (CI_ISFDEF(typ))
    { st_node **lo = prog_db.extra->st_nodes + prog_db.extra->first[typ];
      st_node **hi = prog_db.extra->st_nodes + prog_db.extra->lim[typ];
      st_node **cur;
  
      for (cur = lo; cur < hi; cur++)
        if (recomp_kind (*cur) >= 2)
        { st_node* bname = get_bname (*cur);
          src_extra *s = &SRCX(bname);
          module_name* m = s->raw_info[mn_size];

          long new_sz = FDX(*cur).fdef_size;

          /* Adjust recorded sizes for each phase of the function to
             account for the inlining we just did */

          adjust_sizes (*cur, new_sz);

          /* Add the adjusted size into the module and group totals. */

          s->mod_sz += new_sz;
          m->grp_sz += new_sz;

          /* grp_ts has meant "extra space available" until now.  The compiler
             wants a target instead, so add in the space already consumed. */

          m->grp_ts += new_sz;
        }
    }

  { /* Set targets for modules as their percentage of the group targets. */

    st_node** lo = prog_db.extra->st_nodes+prog_db.extra->first[CI_SRC_REC_TYP];
    st_node** hi = prog_db.extra->st_nodes+prog_db.extra->lim[CI_SRC_REC_TYP];
    st_node** cur;

    for (cur=lo; cur < hi; cur++)
      if (recomp_kind(*cur) >= 2)
      { src_extra *s = &SRCX(*cur);
        module_name *m = s->raw_info[mn_size];

        s->mod_ts = db_size_ratio (s->mod_sz, m->grp_sz, m->grp_ts);
      }
  }

  /* Set targets for functions as their percentage of the module targets. */
  for (typ = 0; typ < CI_MAX_REC_TYP; typ++)
    if (CI_ISFDEF(typ))
    { st_node **lo = prog_db.extra->st_nodes + prog_db.extra->first[typ];
      st_node **hi = prog_db.extra->st_nodes + prog_db.extra->lim[typ];
      st_node **cur;
  
      for (cur = lo; cur < hi; cur++)
        if (recomp_kind(*cur) >= 2)
        { st_node* bname = get_bname (*cur);
          src_extra *s = &SRCX(bname);
          module_name* m = s->raw_info[mn_size];

          /* Calculate function target as percentage of module target */
          long ts = db_size_ratio(get_target(*cur,TS_INL), s->mod_sz,s->mod_ts);

          adjust_sizes (*cur, ts);
        }
    }
  report_targets ();
}

static void
prog_post_read_ (db, input, st_nodes, num_stabs)
dbase *db;
DB_FILE* input;
st_node* st_nodes;
{
  int i, n, m;
  char* t;

  FILE* f = (flag_print_decisions || flag_print_summary) ? decision_file.fd : 0;

  /* This version of post_read starts the /subst and /ref processing; it
     is called immediately before the normal gcdm work is to begin, right after
     pass1.db has been read.  We will set up some helper data structures,
     and then we will walk the gcdm sommand line, doing the initial
     processing for /subst and /ref.

     First, set up some dope associated with the program database
     so that we can easily determine how many nodes there are of a given
     rec_typ, and so that we can easily find all nodes of a given rec_typ.

     This information is used both by the command line processing code
     below, and in finish_subst, after the rest of the normal gcdm
     decision work is done. */

  /* m is size of the dbase_extra node, less the 1 elt array at the end */
  m = sizeof(dbase_extra) - sizeof(st_node*);

  /* n is the total size of the dbase_extra node */
  n = m + num_stabs * sizeof(st_node*);

  db->extra = (dbase_extra *) (t = db_malloc (n));

  /* zero everything except the nodes vector */
  memset (t, 0, m);

  /* copy the st_node pointers into the nodes vector */
  for (i = 0; i < num_stabs; i++)
  { int sz = extra_size (st_nodes[i].rec_typ);
    db->extra->st_nodes[i] = &st_nodes[i];
    st_nodes[i].extra = db_malloc (sz);
    memset (st_nodes[i].extra, 0, sz);
  }

  /* sort the nodes vector by record type */
  qsort (t+m, num_stabs, sizeof (st_node *), prog_post_read_qsort_st_node);

  /* Record the count and index within the st_nodes vector of each type
     of record.  */

  for (i = 0; i <  num_stabs; i++)
  { int t = db->extra->st_nodes[i]->rec_typ;

    assert (t > 0 && t <= CI_MAX_REC_TYP);

    if (db->extra->count[t]++ == 0)
      db->extra->first[t] = i;

    db->extra->lim[t] = i+1;
  
    if (extra_size (t))
    { src_extra* extra = (src_extra*) (db->extra->st_nodes[i]->extra);
      assert (extra);
      extra->dope_index = i;
    }
  }

  /* Do wildcard expansion on module names in the subst, ref, and code_size
     lists, parse modifiers so we know the substitution to be made for each
     name, mark symbols as FDEF/LIB_FDEF or VDEF/LIB_VDEF according to whether
     or not they have an entry on the subst list, and set addr_taken
     appropriately if there were ref/noref commands.

     We have to do this now, before the normal gcdm960 run, because
     it affects inlining decisions.  We finish the rest of the work
     for /subst later, in finish_subst.  */

  for (i = 0; i < NUM_MKIND; i++)
    if (mname_head[i])
      expand_module_name (i);

  { int typ;
    module_name* m;

    int prof_cost = 0;

    st_node **lo = prog_db.extra->st_nodes+prog_db.extra->first[CI_SRC_REC_TYP];
    st_node **hi = prog_db.extra->st_nodes+prog_db.extra->lim[CI_SRC_REC_TYP];
    st_node **cur;
  
    unsigned long used_size = dbp_lookup_gld_value (&prog_db, "__Stext");

    prof_cost  = prep_code_sizes ();
    used_size -= prof_cost;

    if (f)
    { fprintf (f, "initial linked text size was %d", used_size + prof_cost);

      if (prof_cost)
        fprintf (f, ", with about %d bytes of instrumentation",prof_cost);

      fprintf (f, "\n");
    }

    /* Make the head of the tsinfo list be the universal entry... */
    m = (module_name*) db_malloc (sizeof (*m));
    memset (m, 0, sizeof (*m));

    m->next = mname_head[mn_size];
    mname_head[mn_size] = m;
    m->id = num_mname[mn_size]++;

    if (text_size)
      /* Set ts based on text_size - overhead */
      m->grp_ts = text_size - used_size;
    else
      /* Set ts as 5% for each inline level. */
      m->grp_ts = inline_level() * .05 * used_size;

    if (f)
      fprintf (f, "about %d bytes are assumed available for the final text section\n", used_size + m->grp_ts);

    /* Attach the universal target to the modules that don't have a target */

    for (cur=lo; cur < hi; cur++)
      if (SRCX(*cur).raw_info[mn_size] == 0)
        SRCX(*cur).raw_info[mn_size] = mname_head[mn_size];

    /* Change all LIB_VDEF/LIB_FDEF nodes in files subject to 2nd
       pass substitution to VDEF/FDEF nodes, and set addr_taken to true
       for all defs which come from files subject to /ref.  Note we are
       making our dope (prog_db.extra) slightly inaccurate, because we
       are changing categories for some LIB_ def nodes.  However, we don't
       care, because for finish_subst, the distinction between the kinds of
       defs is not is not important. */

    for (typ = 0; typ < CI_MAX_REC_TYP; typ++)
    { int l = 0;

      switch (typ)
      { case CI_FDEF_REC_TYP:     l = CI_FDEF_FILE_LIST; break;
        case CI_LIB_FDEF_REC_TYP: l = CI_FDEF_FILE_LIST; break;
        case CI_VDEF_REC_TYP:     l = CI_VDEF_FILE_LIST; break;
        case CI_LIB_VDEF_REC_TYP: l = CI_VDEF_FILE_LIST; break;
      }

      if (l)
      { lo = prog_db.extra->st_nodes + prog_db.extra->first[typ];
        hi = prog_db.extra->st_nodes + prog_db.extra->lim[typ];
  
        for (cur = lo; cur < hi; cur++)
        { /* Get this symbol's bname list ... */
          char* b = (char*) ((*cur)->db_list[l]->text+4);
          int   n = (*cur)->db_list[l]->size;

          /* ... and walk it. */
          while (n > 0)
          { st_node* bname = dbp_bname_sym (&prog_db, b);
            int        len = strlen (b) + 1;

            if (CI_ISDEF(typ) && mname_head[mn_subst])
              /* The decision as to whether to allow inlining into *cur,
                 and whether to consider *cur for possible deletion, is
                 determined by whether or not *cur is a CI_FDEF or CI_LIB_FDEF.

                 This decision has already been defaulted by the linker based on
                 whether or not *cur came out of an archive.

                 If no subst commands were seen, we leave this decision
                 alone.  If subst commands were seen, we only allow
                 inlining into routines and consideration for deletion
                 when there will be a 2nd pass substitution for the routine.

                 An analagous test is made in subst_inlinable for deciding
                 whether to allow other routines to inline a function. */
            
              if (SUBST_KIND(bname) == 2)
                (*cur)->rec_typ = CI_ISFUNC(typ)
                                ? CI_FDEF_REC_TYP : CI_VDEF_REC_TYP;
              else
                (*cur)->rec_typ = CI_ISFUNC(typ)
                                ? CI_LIB_FDEF_REC_TYP : CI_LIB_VDEF_REC_TYP;

            if (is_force_ref (bname))
              db_rec_set_addr_taken (*cur);

            n -= len;
            b += len;
          }
          assert (n==0);
        }
      }
    }
  }
}

char*
find_cc1_opt(t)
char* t;
{
  /* Return the next option we should give to cc1.960 from t. */
  int skip;

  do
  { int n;

    while (*t=='+')
      t++;

    if ((skip = !strncmp (t, "asm_pp+", (n=7))))
    { char* u = strchr (t+n, '+');
      if (u)
        t=u+1;
      else
        t+=strlen (t);
    }
  }
  while
    (skip);

  return t;
}

static int
find_text (l, value, voff, pindex, poffset)
db_list_node* l;
unsigned char* value;
int voff;
int* pindex;
int* poffset;
{
  int offset = 4;
  int index  = 0;

  assert (l->size >= 0 && l->text && !l->tell && !l->next);

  while ((offset+voff) < (l->size+4) &&
         strcmp ((char *)(l->text+offset+voff), (char *)(value+voff)))
  {
    int n = strlen ((char *)(l->text+offset+voff))+1;

    offset += n + voff;
    index++;
  }

  if (pindex)
    *pindex = index;

  if (poffset)
    *poffset = offset;

  assert (offset <= (l->size+4));
  return (offset < (l->size+4));
}

static void
delete_text (sym, lnum, offset, n)
st_node* sym;
int lnum;
int offset;
int n;
{ /* Delete n bytes starting at into->db_list[num]->text+offset */

  db_list_node* l = sym->db_list[lnum];
  int i, size;

  if (n == 0)
    n = 1+strlen ((char *)(l->text+offset));

  assert (n > 0);

  /* Subtract n from all of the sizes that care... */

  CI_U32_FM_BUF(l->text, size);
  assert (size == l->size + 4);
  CI_U32_TO_BUF(l->text, size-n);

  sym->db_rec_size -= n;
  sym->db_list_size -= n;
  l->size -= n;

  /* Delete the text by moving the remainder of this chunk over it. */
  for (i = offset; i < size; i++)
    l->text[i] = l->text[i+n];
}

static void
insert_text (into, num, offset, n, value)
st_node* into;
int num;
int offset;
int n;
unsigned char* value;
{ /* Insert value[0] .. value[n-1] at into->db_list[num]->text+offset */

  unsigned char *ntext, *otext;
  int osize, i;

  assert (into->db_list[num] && value && n && offset >= 4);
  offset-= 4;

  otext = into->db_list[num]->text;
  assert (otext);

  CI_U32_FM_BUF (otext, osize);
  assert (osize == into->db_list[num]->size+4);
  otext += 4;
  osize -= 4;

  into->db_list_size += n;
  into->db_rec_size  += n;

  into->db_list[num]->size += n;
  into->db_list[num]->text  = ntext = (unsigned char*) db_malloc (osize+n+4);

  CI_U32_TO_BUF (ntext, osize+n+4);
  ntext += 4;

  for (i = 0; i < offset; i++)
    *ntext++ = *otext++;

  for (i = 0; i < n; i++)
    *ntext++ = *value++;

  for (i = offset; i < osize; i++)
    *ntext++ = *otext++;
}

void
append_list (into, num, l)
st_node* into;
int num;
db_list_node* l;
{
  assert (into->db_list[num] && l->text && !l->next && !l->tell);

  if (l->size == 0)
    return;

  if (into->db_list[num]->size == 0)
  { db_replace_list (into, num, l);
    return;
  }

  insert_text (into, num, into->db_list[num]->size+4, l->size, l->text+4);
}

finish_subst ()
{
  /* Called after all normal gcdm work has been done.  Use prev db (if present)
     to cut down the number of compiles if possible, and merge prev_db into
     prog_db. */

  int fset_ints  = FSET_INTS;
  int fset_bytes = fset_ints * sizeof (unsigned);

  int num_fdefs = prog_db.extra->count[CI_FDEF_REC_TYP] +
                  prog_db.extra->count[CI_LIB_FDEF_REC_TYP];

  /* Allocate an FSET for the 2nd pass files which are out of date.  This
     set will include files which we will not recompile because they were
     not subject to subst commands; we'll ignore those when we actually
     get around to issuing compiles and substitutions. */

  unsigned* bad_files = (unsigned*) db_malloc (fset_bytes);

  /* Allocate space for an FSET for each fdef.  This set will hold the
     transitive closure of all files into which the fdef is inlined. */

  unsigned* tclose = (unsigned*) db_malloc (num_fdefs * fset_bytes);

  /* Allocate space to hold the st_nodes of every function directly
     inlined into each fdef.  We use this while calculating tclose.
     Note that we would really only need to look at any given called
     function once per fdef; instead, we use call sites, which means
     extra space and time for duplicates.  Fixing this is probably not
     worth the trouble. */

  st_node** inlinev = (st_node**) db_malloc (num_inlined_arcs*sizeof(st_node*));

  set_code_size_targets ();

  memset (bad_files, 0, fset_bytes);
  memset (tclose, 0, num_fdefs * fset_bytes);

  { /* The primary function of this loop is to analyze direct dependencies.
       That is, when the loop is done, we will have a set containing those
       files which will have to be recompiled because:

         1)  The first pass object file changed;

         2)  The first pass object didn't change, but we have no memory of
             compiling the file with the requested options;

         3)  We remembered compiling the file, but it is not present in
             the pdb.  This could happen, for example, if somebody hit
             ctrl-C after we had written out pass2.db, while we were
             actually doing compiles. */

    st_node** lo = prog_db.extra->st_nodes+prog_db.extra->first[CI_SRC_REC_TYP];
    st_node** hi = prog_db.extra->st_nodes+prog_db.extra->lim[CI_SRC_REC_TYP];
    st_node** cur;
  
    /* Walk all current CI_SRC records ... */
    for (cur=lo; cur < hi; cur++)
    { /* Retrieve the substitution info for this module that we saved in
         expand_module_name when we examined prog_db right after we read it. */
  
      char* sub;
      st_node* prev;
      int force_compile;

      /* If we didn't see a subst for the src, set up a "nosubst" */
      if (!(sub = SUBST_KEY(*cur)))
      { static subst_info info;
        static module_name m;

        if (info.key == 0)
          parse_modifiers (&m, mn_subst, &info);

        SUBST_KEY(*cur) = sub = info.key;
      }

      /* Get the CI_SRC record from prev_db which corresponds to *cur... */
      prev = dbp_lookup_sym (&prev_db, (*cur)->name);

      if (prev && !strcmp(db_member(prev), db_member(*cur)))
      { force_compile = (db_fstat(prev) != db_fstat(*cur));

        /* Once we've chosen the disambiguation index for the non-substitution
           part of the filename for the output, we keep it provided the 1st
           pass object file name doesn't change.  */

        db_set_dindex (*cur, db_dindex(prev));
      }
      else
        force_compile = 1;

      /* Should have empty subst history at this point */
      assert ((*cur)->db_list[CI_SRC_SUBST_LIST]->size == 0);

      if (!force_compile)
        /* We have a matching CI_SRC record in prev_db, so move it's
           substitution history onto the current CI_SRC record.  There is an
           index stored with each subst in the history.  This index is used to
           distinguish the output files for each different subst for this
           module from one another.

           When we get above CI_MAX_SUBST remembered substitutions, we throw
           out the oldest and assign it's substitution index to the current
           substitution. */
           
        append_list (*cur, CI_SRC_SUBST_LIST, prev->db_list[CI_SRC_SUBST_LIST]);
  
      if (SUBST_KIND(*cur))
      { int si, idx, off, del;
        db_list_node* l = (*cur)->db_list[CI_SRC_SUBST_LIST];

        del = 1;

        if (find_text(l, sub, IVK_OFF, &idx, &off))
          /*  We have compiled this file with this subst in recent memory.
              Reuse the substitution index; the file may still be good.  If
              it isn't, the filename will still be the same as the last
              time, which is nice. */

          si = l->text[off];

        else
        { force_compile = 1;

          /* We have no memory of compiling this file with these options. */

          if (idx >= CI_MAX_SUBST)
          { /* Throw out the last substitution, and take his sindex. */
            assert (off == l->size+4 && l->text[off-1] == '\0');

            off--;
            while (off > 4 && l->text[off-1])
              off--;

            si = l->text[off];
          }
          else
          { si = '0' + idx;
            del = 0;
          }
        }

        if (del)
          delete_text (*cur, CI_SRC_SUBST_LIST, off, 0);

        assert (si >= '0');
        db_set_sindex (*cur, sub[0]=si);

        /* Make the current substitution be the most recent one. */
        insert_text (*cur, CI_SRC_SUBST_LIST, 4, 1+strlen(sub), sub);
      }

      if (force_compile)
        SET_FSET (bad_files, cur-lo);
    }
  }

  { /* Note the effects of changes in alias information and variable binding
       information by walking the list of functions that mention the variable
       (or take the function's address) and marking those functions as having
       changes with global effects.  Later, when we process FDEF records in the
       following loop, we will propagate these changes on into the transitive
       closure of the files that inline each fdef.  */

    int typ;
    for (typ = 0; typ < CI_MAX_REC_TYP; typ++)
    { int l = 0;

      switch (typ)
      { case CI_FDEF_REC_TYP:     l = CI_FDEF_FUNC_LIST; break;
        case CI_LIB_FDEF_REC_TYP: l = CI_FDEF_FUNC_LIST; break;
        case CI_VDEF_REC_TYP:     l = CI_VDEF_FUNC_LIST; break;
        case CI_LIB_VDEF_REC_TYP: l = CI_VDEF_FUNC_LIST; break;
        case CI_VREF_REC_TYP:     l = CI_VREF_FUNC_LIST; break;
        case CI_FREF_REC_TYP:     l = CI_FREF_FUNC_LIST; break;
      }

      if (l)
      { st_node** lo = prog_db.extra->st_nodes + prog_db.extra->first[typ];
        st_node** hi = prog_db.extra->st_nodes + prog_db.extra->lim[typ];
        st_node** cur;
  
        for (cur = lo; cur < hi; cur++)
        { st_node* prev = dbp_lookup_sym (&prev_db, (*cur)->name);

          if (prev!=0 &&
              (alias_info_change (*cur, prev) || lomem_info_change(*cur, prev)))
          { /* We have detected a change that should propagate into all
               functions on the reference list. */
 
            char* u = (char*) ((*cur)->db_list[l]->text+4);
            int   n = (*cur)->db_list[l]->size;
          
            SET_CHANGE(*cur, 2, "vdef alias change");

            while (n > 0)
            { st_node* f = dbp_lookup_sym (&prog_db, u);
              int    len = strlen (u)+1;

              assert (f);
              SET_CHANGE(f, 2, "  indirectly");

              n -= len;
              u += len;
            }
            assert (n==0);
          }

          if (prev&& CI_ISVDEF((*cur)->rec_typ)&&sram_binding_change(*cur,prev))
          { /* Make sure we invalidate the defining files, because we
               do not make another pass for VDEFs.  This should be the only
               change which can happen to a variable def which could cause
               code gen differences in the variable binding even when
               the first pass object file did not change. */

            char* b = (char*) ((*cur)->db_list[CI_VDEF_FILE_LIST]->text+4);
            int   n = (*cur)->db_list[CI_VDEF_FILE_LIST]->size;

#ifdef DEBUG
            db_comment(0,"sram binding change to %s invalidates ",(*cur)->name);
#endif
            while (n > 0)
            { st_node* bname = dbp_bname_sym (&prog_db, b);
              int        len = strlen (b) + 1;
              int      index = ((src_extra*)(bname->extra))->dope_index -
                                prog_db.extra->first[CI_SRC_REC_TYP];

              assert (index >= 0 && index < (fset_ints * 32));
              SET_FSET (bad_files, index);
#ifdef DEBUG
              db_comment (0, " %s", bname->name);
#endif

              n -= len;
              b += len;
            }
            assert (n==0);
#ifdef DEBUG
            db_comment (0, "\n");
#endif
          }
        }
      }
    }
  }

  /* Walk all current function definition records, initializing transitive
     closure and updating change info, and copying over inline decision history
     from prev_db if there have been no changes in the rtl. */

  { int typ = CI_FDEF_REC_TYP;

    for (; typ; typ = (typ==CI_FDEF_REC_TYP ? CI_LIB_FDEF_REC_TYP : 0))
    { st_node** lo = prog_db.extra->st_nodes + prog_db.extra->first[typ];
      st_node** hi = prog_db.extra->st_nodes + prog_db.extra->lim[typ];
      st_node** cur;
  
      for (cur = lo; cur < hi; cur++)
      {
        char* bname_str = (char*)((*cur)->db_list[CI_FDEF_FILE_LIST]->text+4);
        st_node*  bname = dbp_bname_sym (&prog_db, bname_str);

        /* Get the name for the inline vector for this substitution */
        char* ivk = SUBST_KEY(bname) + IVK_OFF;

        int   ncall   = db_rec_n_calls(*cur);
        int   cv_size = CI_FDEF_IVECT_SIZE(ncall);
        char* cv   = db_rec_call_vec(*cur);

        unsigned char* decv = db_inln_dec_vec (*cur);

        int rsize = db_rec_rtl_size (*cur);

        /* Grab the entry in the old database for this FDEF */
        st_node*  prev = dbp_lookup_sym (&prev_db, (*cur)->name);
  
        int i;
  
        /* Set up the extra dope required to calculate the transitive
           closure of files that each fdef is inlined into. */
  
        assert ((*cur)->extra);

        FDX(*cur).bname_index = ((src_extra*)(bname->extra))->dope_index -
                             prog_db.extra->first[CI_SRC_REC_TYP];
        FDX(*cur).inlinev     = inlinev;
        FDX(*cur).tclose      = tclose;

        tclose += fset_ints;
        
        for (i=0; i < ncall; i++)
        { if (CI_FDEF_IVECT_VAL(decv, i))
            *inlinev++ = dbp_lookup_sym (&prog_db, cv);

          cv += strlen (cv)+1;
        }

        FDX(*cur).num_inlv = inlinev - FDX(*cur).inlinev;
  
        /* Initialize the name list for the inline history for this FDEF by
           making the 0th entry be the key (a function mostly of inline level
           but partially of subst options) for the file containing the
           definition of the function.
  
           The other part of this history is the list of inline decision
           vectors, which is already initialized by virtue of the fact
           that the current decision vector (the one just decided by
           gcdm) is already set up as the 0th one. */
  
        insert_text (*cur, CI_FDEF_HIST_LIST, 4, 1+strlen(ivk), ivk);
  
        /* Determine the extent to which the rtl and inline decisions
           have changed, and mark the fdef with 1 if there were changes which
           affect only the fdef's defining object file, or 2 if there were
           changes which could affect all object files which fdef is inlined
           into.
  
           Also, if there were no rtl changes, append the inline decision
           history from the previous 2nd pass database. */
  
        if (!prev || !CI_ISFDEF(prev->rec_typ) || db_rec_rtl_size(prev)!=rsize
        ||  (db_rec_n_calls(prev) != ncall)
        ||  (rsize && memcmp (db_rec_rtl(prev), db_rec_rtl(*cur), rsize)))
  
          /* There were gross changes to the rtl; the inline history is no
             longer valid, so we don't copy it over from prev_db. */
  
          SET_CHANGE (*cur, 2, (prev ? "fdef rtl change" : 0));
   
        else
        { db_list_node* prev_hist = prev->db_list[CI_FDEF_HIST_LIST];

          unsigned del_msk, can_del;
          int idx, off;

          /* The deletion history is recorded as a bit mask, with the bits
             indexed by the same key as the inline decision vectors.  Bit 0
             refers to the current deletion information. */

          CI_U8_FM_BUF (prev->db_rec+CI_FDEF_CAN_DELETE_OFF, del_msk);
          CI_U8_FM_BUF ((*cur)->db_rec+CI_FDEF_CAN_DELETE_OFF, can_del);
          assert ((can_del &~1) == 0);

          /* The rtl we recorded in prev (if any) did not change;  we can keep
             the inline decision and deletion history, except that we want to:
  
               1)  Notice whether the history for the current substitution
                   for this fdef has changed;
  
               2)  If so, mark the fdef with change==2 to force recompilation
                   of all files the current fdef is inlined into;
  
               3)  If not, check the fdef further for changes local to
                   it's definition file;
  
               4)  Delete the previous inline decision vector for the
                   current substitution and the deletion history bits,
                   place the rest of the previous inline decision vectors
                   after the current inline decision vector, and
                   shift the 0..n of the previous deletion history
                   into bits 1..n+1 of the current deletion history. */
  
          if (rsize != 0)
          { assert (db_rec_n_insns(prev) == db_rec_n_insns(*cur));
            assert (db_rec_n_parms(prev) == db_rec_n_parms(*cur));
          }
  
          if (find_text (prev_hist, ivk, 0, &idx, &off))
          { /* We had a previous decision vector (and deletion history)
               for the current key.

               First, delete the name from prev, then retrieve the corresponding
               vector (using the index number of the key's name) and compare it
               to the current one, then delete the vector from prev. */
  
            delete_text (prev, CI_FDEF_HIST_LIST, off, 0);

            if (cv_size)  /* No calls == no vector space */
            { /* Did the current decision vector change ? */
              if (GET_CHANGE(*cur)<2)
              { db_list_node* prev_ivec = prev->db_list[CI_FDEF_IVEC_LIST];

                int ivo = 4 + cv_size * idx;

                if (memcmp(db_inln_dec_vec(*cur), prev_ivec->text+ivo, cv_size))
                  SET_CHANGE(*cur, 2, "inline decisions change");
  
                delete_text (prev, CI_FDEF_IVEC_LIST, ivo, cv_size);
              }
            }
            else
            { assert ((*cur)->db_list[CI_FDEF_IVEC_LIST]->size == 0);
              assert (prev->db_list[CI_FDEF_IVEC_LIST]->size == 0);
            }

            /* The deletability info is bit 'i'.  Notice if it changed,
               and then delete the bit.  This treatment is analagous to
               the deletion of the current decision vector from prev, above. */

            { unsigned del_lo = del_msk & ~((-1) << idx);

              del_msk >>= idx;

              if (GET_CHANGE(*cur)<1 && can_del != (del_msk & 1))
                SET_CHANGE(*cur, 1, "deletability change");

              del_msk = ((del_msk >> 1) << idx) | del_lo;
            }
          }
          else
            if (GET_CHANGE(*cur) < 2)
              SET_CHANGE(*cur, 2, "missing history");
  
          if (GET_CHANGE(*cur) < 2)
            if (db_fdef_prof_change (prev, *cur))
              SET_CHANGE(*cur, 2, "profile change");

          /* Now, the current name and decision info are gone
             from prev;  append prev's other decisions and key names
             behind the current ones.  Then, shift the deletion history
             left by 1 into the current deletion info.

             We don't want to let the history grow without bound, so
             we remove the oldest entry if we are going to be above the
             limit after the append. */

          i = find_text (prev_hist, ivk, 0, &idx, &off);
          assert (i == 0);

          if (idx >= CI_MAX_SUBST)
          { /* Throw out the oldest information we remember, because we are
               already above the limit and we are about to add one more. */

            assert (off == prev_hist->size+4 && prev_hist->text[off-1] == '\0');
            off--;
            while (off > 4 && prev_hist->text[off-1])
              off--;

            delete_text (prev, CI_FDEF_HIST_LIST, off, 0);

            del_msk &= ~((-1) << (i-1));

            if (cv_size)
              delete_text (prev, CI_FDEF_IVEC_LIST, 4+ cv_size*idx, cv_size);
          }

          append_list
            (*cur, CI_FDEF_HIST_LIST, prev->db_list[CI_FDEF_HIST_LIST]);
          append_list
            (*cur, CI_FDEF_IVEC_LIST, prev->db_list[CI_FDEF_IVEC_LIST]);

          del_msk <<= 1;
          del_msk |= can_del;
          CI_U8_TO_BUF ((*cur)->db_rec+CI_FDEF_CAN_DELETE_OFF, del_msk);
        }

        SET_FSET (FDX(*cur).tclose, FDX(*cur).bname_index);
  
        if (GET_CHANGE(*cur))
        { /* This is here for those cases where a change happens because
             of information discovered globally - e.g, when addr_taken,
             profile counts, or inline decisions change. The bit will already
             have been set if there was a local rtl change, because the defining
             file would have been caught as out-of-date in the CI_SRC_REC
             loop, above. */
  
          SET_FSET (bad_files, FDX(*cur).bname_index);
        }
      }
    }
  }

  /* Walk all fdefs until transitive closure settles, updating bad_files
     when there is a change to an inlinee's closure, and that inlinee
     has been marked as having a change that should propogate into
     the closure of files it is inlined into. */

  for (;;)
  {
    int typ = CI_FDEF_REC_TYP;
    int change = 0;

    for (; typ; typ = (typ==CI_FDEF_REC_TYP ? CI_LIB_FDEF_REC_TYP : 0))
    {
      st_node** lo = prog_db.extra->st_nodes + prog_db.extra->first[typ];
      st_node** hi = prog_db.extra->st_nodes + prog_db.extra->lim[typ];
      st_node** cur;

      for (cur = lo; cur < hi; cur++)
      { fdef_extra* x = (fdef_extra*) (*cur)->extra;
        st_node**  i0 = x->inlinev;
        st_node**  in = x->inlinev + x->num_inlv;
        unsigned*  s0 = x->tclose;

        st_node**   i;

        for (i=i0; i!=in; i++)
        { fdef_extra* y = (fdef_extra*) ((*i)->extra);
          unsigned*  t0 = y->tclose;
          unsigned*  tn = y->tclose + fset_ints;

          unsigned ch, *s, *t;

          /* union the caller's closure into the callee's closure */
          for ((ch=0),(s=s0),(t=t0); t != tn; s++,t++)
          { int old = *t;
            int new = *t |= *s;

            ch |= (new ^ old);
          }

          /* When anybody's closure changes, if that routine had changes which
             are supposed to propogate into all the files it is inlined into,
             we update bad_files.  Thus, bad_files is always up to date, and is
             the final set to recompile when the closure calculation
             (set of files inlined into) settles. */

          if (ch != 0 && GET_CHANGE(*i) == 2)
            for ((s=bad_files),(t=t0); t != tn; s++,t++)
              *s |= *t;

          change |= ch;
        }
      }
    }
    if (!change)
      break;
  }

  compile_objects (bad_files);
}

