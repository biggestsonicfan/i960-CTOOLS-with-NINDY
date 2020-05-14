#ifndef __SIGNAL_H__
#define __SIGNAL_H__

/*
 * <signal.h> : signal & raise, etc.
 *
 */

#include <__macros.h>

typedef char sig_atomic_t;

#define SIGILL   1    /* illegal instruction signal */
#define SIGINT   2    /* ^C */
#define SIGTERM  5    /* terminate signal */
#define SIGFPE   8    /* floating point exception signal */
#define SIGSEGV  9    /* segment violation signal */
#define SIGABRT  10   /* abort - I assume that this is the UNIX "quit" */

/* non-ANSI macros */
#define SIGUSR1  0    /* user-defined signal */
#define SIGALLOC 3    /* alloc, malloc, realloc */
#define SIGFREE  4    /* bad free pointer */
#define SIGREAD  6    /* read error */
#define SIGWRITE 7    /* write error */
#define SIGUSR2  11   /* user-defined signal */

#define SIGSIZE  12   /* number of defined signals */

__EXTERN void (_sig_abrt_dfl)(__NO_PARMS);
__EXTERN void (_sig_fpe_dfl)(__NO_PARMS);
__EXTERN void (_sig_ill_dfl)(__NO_PARMS);
__EXTERN void (_sig_int_dfl)(__NO_PARMS);
__EXTERN void (_sig_segv_dfl)(__NO_PARMS);
__EXTERN void (_sig_term_dfl)(__NO_PARMS);
__EXTERN void (_sig_read_dfl)(__NO_PARMS);
__EXTERN void (_sig_write_dfl)(__NO_PARMS);
__EXTERN void (_sig_alloc_dfl)(__NO_PARMS);
__EXTERN void (_sig_free_dfl)(__NO_PARMS);
__EXTERN void (_sig_null)(__NO_PARMS);
__EXTERN void (_sig_err_dummy)(int);
__EXTERN void (_sig_dfl_dummy)(int);
__EXTERN void (_sig_ign_dummy)(int);

__EXTERN void (*(_sig_dfl(int)))();

/* Signal vector arrays */
__EXTERN void (*_sig_eval[SIGSIZE])();

/* Signal processing macros */
#define SIG_IGN  (&_sig_ign_dummy)
#define SIG_DFL  (&_sig_dfl_dummy)
#define SIG_ERR  (&_sig_err_dummy)

/* ANSI functions */
__EXTERN void (*(signal) (int, void (*)(int)))(int);
__EXTERN int (raise)(int);

#endif
