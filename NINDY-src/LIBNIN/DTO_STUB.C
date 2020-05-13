/******************************************************************/
/* 		Copyright (c) 1989, Intel Corporation

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

   Intel Corporation does not warrant, guarantee or make any 
   representations regarding the use of, or the results of the use
   of, the software and documentation in terms of correctness, 
   accuracy, reliability, currentness, or otherwise; and you rely
   on the software, documentation and results solely at your own 
   risk.							  */
/******************************************************************/

/********************************************************
The functions in this file may be implemented to create 
reentrant high level libraries.  These functions are used 
by the ic960 v2.0 libraries from Intel.  This file may be 
omitted if you are using other libraries or other compilers.
For information on implementing these functions see the
"ic960 Library Supplement".
********************************************************/

#include <stdio.h>
#include <reent.h>
#include <time.h>

static struct _exit _exit_str;	     /* allocate for _exit statically */
static struct _stdio _stdio_str;    /* allocate for _stdio statically */
static struct {
	struct _thread	t;
	struct tm	gmtime_buffer;
} _thread_str;			   /* allocate for _thread statically */

static struct _tzset data = {
    {"PST", "PDT"}, 8 * 60 * 60, 0
};

/********************************************************/
/* MAP_LENGTH						*/
/*							*/
/* _map_length - maps between C text stream and OS text */
/* 	data formats					*/
/********************************************************/
int _map_length (int fd, const void *buffer, size_t count)
{
const char *ptr;
int result = count;

    return (result);     /* the number of newline encountered */
}

/********************************************************/
/* EXIT_CREATE						*/
/*							*/
/* _exit_create() - allocate memory for struct _exit	*/
/********************************************************/
struct _exit *_exit_create(size_t size)
{
	return (&_exit_str);
}

/********************************************************/
/* STDIO_CREATE						*/
/*							*/
/* _stdio_create() - allocate memory for struct _stdio  */
/********************************************************/
struct _stdio *_stdio_create(size_t size)
{
	return (&_stdio_str);
}

/********************************************************/
/* THREAD_CREATE					*/
/*							*/
/* _thread_create() - allocate memory for struct _thread*/
/********************************************************/
struct _thread *_thread_create(size_t size)
{
	return (&_thread_str.t);
}

/********************************************************/
/* EXIT_PTR    						*/
/*							*/
/* _exit_ptr() - get pointer to struct _exit		*/
/********************************************************/
struct _exit *_exit_ptr(void)
{
	return (&_exit_str);
}

/********************************************************/
/* STDIO_PTR   						*/
/*							*/
/* _stdio_ptr() - get pointer to struct _stdio		*/
/********************************************************/
struct _stdio *_stdio_ptr(void)
{
	return (&_stdio_str);
}

/********************************************************/
/* THREAD_PTR  						*/
/*							*/
/* _thread_ptr() - get pointer to struct _thread 	*/
/********************************************************/
struct _thread *_thread_ptr(void)
{
	return (&_thread_str.t);
}

/********************************************************/
/* STDIO_STDOPEN					*/
/*							*/
/* _stdio_stdopen() - initialize standard streams	*/
/********************************************************/
int _stdio_stdopen(int fd)
{
    return (fd);
}

/********************************************************/
/* TZSET_PTR    					*/
/*							*/
/* tzset - set the time zone vars from the environment  */
/********************************************************/
struct _tzset * _tzset_ptr()
{
	return (&data);
}

/********************************************************/
/* SEMAPHORE_INIT					*/
/*							*/
/* _semaphore_init() - initialize semaphore		*/
/********************************************************/
void _semaphore_init(void **sema)
{
}

/********************************************************/
/* SEMAPHORE_WAIT					*/
/*							*/
/* _semaphore_wait() - wait for the event to occur 	*/
/********************************************************/
void _semaphore_wait(void **sema)
{
}

/********************************************************/
/* SEMAPHORE_SIGNAL					*/
/*							*/
/* _semaphore_signal() - clear the event		*/
/********************************************************/
void _semaphore_signal(void **sema)
{
}

/********************************************************/
/* SEMAPHORE_DELETE					*/
/*							*/
/* _semaphore_delete() - remove the event		*/
/********************************************************/
void _semaphore_delete(void **sema)
{
}

/* default signal handler routines */
_sig_null(){}
_sig_ill_dfl(){}
_sig_int_dfl(){}
_sig_alloc_dfl(){}
_sig_free_dfl(){}
_sig_term_dfl(){}
_sig_read_dfl(){}
_sig_write_dfl(){}
_sig_fpe_dfl(){}
_sig_segv_dfl(){}
_sig_abrt_dfl(){}

/********************************************************/
/* GETEND          					*/
/*							*/
/* Sets the end of memory for malloc (done differently  */
/* by the NINDY libraries				*/
/********************************************************/
void getend(void)
{
}

/********************************************************/
/* C_INIT              					*/
/*							*/
/* Initializes com subsystem, argc, argv (embedded?),   */
/* sets environment variables and sets board level func-*/
/* tionality to known state.  This is done by NINDY in  */
/* the CRTNIN.O file.					*/ 
/********************************************************/
void c_init(void)
{
}

/********************************************************/
/* C_TERM              					*/
/*							*/
/* Terminates com subsystem.  This is done by NINDY in  */
/* the CRTNIN.O file.					*/ 
/********************************************************/
void c_term(void)
{
}

/********************************************************/
/* _ARG_INIT            				*/
/*							*/
/* This function initializes argv and argc if desired	*/
/********************************************************/
void _arg_init(void)
{
}
