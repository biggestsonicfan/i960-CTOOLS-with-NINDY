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

#include "assert.h"
#include "cc_info.h"

unsigned char*
db_get_list (p, l)
st_node* p;
int l;
{
  unsigned char* t;
  int len;

  if (p->db_rec == 0)
    cache_db_rec (p);

  assert (l < CI_REC_LIST_HI(p->rec_typ));

  t = (p->db_rec + CI_REC_FIXED_SIZE(p->rec_typ));

  l -= CI_REC_LIST_LO(p->rec_typ);
  assert (l >= 0);

  while (l-- > 0)
  { CI_U32_FM_BUF (t, len);
    assert (len >= 4);
    t += len;
  }
  CI_U32_FM_BUF (t, len);
  assert (len >= 4);

  return t;
}
