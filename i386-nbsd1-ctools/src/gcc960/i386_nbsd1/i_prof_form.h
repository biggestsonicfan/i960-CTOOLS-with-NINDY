#ifndef I_PROF_FORM_H
#define I_PROF_FORM_H

/*
 * A profile formula is a summation of all the profile counters for
 * a function, with the only possible coefficients being 1, 0, -1.
 */
typedef unsigned char * prof_formula_type;

#define FORMULA_SIZE(n_counters) (n_counters)

#define SET_COEF(formula, index, val) ((formula)[(index)] = (val))
#define GET_COEF(formula, index)      ((formula)[(index)])

#define COEF_NEG  255  /* because we have to use unsigned */
#define COEF_POS  1
#define COEF_ZERO 0

extern void dump_prof_block_info();
extern void save_next_blk_prof_info();
extern void copy_formula();
extern void add_formula();
extern void sub_formula();
extern void print_formula();

#endif
