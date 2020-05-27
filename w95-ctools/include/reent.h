#ifndef __REENT_H__
#define __REENT_H__

/* 
 * header file for reentrancy hooks.
 */

#include <__macros.h>

#define _INIT_OK		0
#define _EXIT_INIT_ERROR	1
#define _STDIO_INIT_ERROR	2
#define _THREAD_INIT_ERROR	3
#define _EXIT_PTR		_exit_ptr()
#define _THREAD_PTR		_thread_ptr()

#define _EXIT_HANDLER_MAX 32

#pragma i960_align _exit = 16
struct _exit
{
  void	*open_stream_sem;
  struct _iobuf	*open_stream_list; /* the list is maintained as a stack */
  void	*exit_handler_sem;
  int	exit_handler_count;
  void	(*exit_handler_list[_EXIT_HANDLER_MAX])();
};

#pragma i960_align _thread = 16
struct _thread
{
  int		errno;
  char		*_strtok_buffer;
  struct tm	*_gmtime_buffer;
  void		*_malloc_sem;
  unsigned long	_rand_seed;
  char		_asctime_buffer[26];
};

#pragma i960_align _tzset = 16
struct _tzset
{
  char	*_tzname[2];
  long	_timezone;
  int	_daylight;
};

__EXTERN int		(_exit_init)(__NO_PARMS);
__EXTERN int		(_stdio_init)(__NO_PARMS);
__EXTERN int		(_thread_init)(__NO_PARMS);
__EXTERN struct _exit*  (_exit_create)(unsigned);
__EXTERN struct _stdio* (_stdio_create)(unsigned);
__EXTERN struct _thread*(_thread_create)(unsigned);
__EXTERN int		(_stdio_stdopen)(int);
__EXTERN struct _exit*  (_exit_ptr)(__NO_PARMS);
__EXTERN struct _stdio* (_stdio_ptr)(__NO_PARMS);
__EXTERN struct _thread*(_thread_ptr)(__NO_PARMS);
__EXTERN struct _tzset* (_tzset_ptr)(__NO_PARMS);
__EXTERN void		(_semaphore_init)(void **);
__EXTERN void		(_semaphore_wait)(void **);
__EXTERN void		(_semaphore_signal)(void **);
__EXTERN void		(_semaphore_delete)(void **);

#endif /* __REENT_H__ */

#ifdef __STDIO_H__
#ifndef _stdio_stream
#define _stdio_stream
struct _stdio
{
	FILE	_stdin;			/* stdin stream */
	FILE	_stdout;		/* stdout stream */
	FILE	_stderr;		/* stderr stream */
};
#endif /* _stdio_stream */
#endif /* __STDIO_H__ */
