#include "config.h"

#ifdef IMSTG

#include <stdio.h>
#include "i_prof_form.h"

void
copy_formula (targ, form1, n_counters)
prof_formula_type targ;
prof_formula_type form1;
int n_counters;
{
  int i;
  for (i = 0; i < n_counters; i++)
    targ[i] = form1[i];
}

void
add_formula(targ, form1, n_counters)
prof_formula_type targ;
prof_formula_type form1;
int n_counters;
{
  int i;
  for (i = 0; i < n_counters; i++)
    targ[i] = targ[i] + form1[i];
}

void
sub_formula(targ, form1, n_counters)
prof_formula_type targ;
prof_formula_type form1;
int n_counters;
{
  int i;
  for (i = 0; i < n_counters; i++)
    targ[i] = targ[i] - form1[i];
}

#if 0
void
debug_formula(form, n_counters, func_offset)
prof_formula_type form;
int n_counters;
int func_offset;
{
  int one_printed = 0;
  int i;

  for (i = 0; i < n_counters; i++)
  {
    int c_i;
    if ((c_i = GET_COEF(form, i)) != COEF_ZERO)
    {
      if (c_i == COEF_POS)
      {
        if (one_printed)
          fprintf(stderr, "+%d", func_offset+(i*4));
        else
          fprintf(stderr, "%d", func_offset+(i*4));
      }
      else if (c_i == COEF_NEG)
      {
        if (one_printed)
          fprintf(stderr, "-%d", func_offset+(i*4));
        else
          fprintf(stderr, "%d", func_offset+(i*4));
      }
      else
        abort();
      one_printed = 1;
    }
  }
}
#endif
#endif
