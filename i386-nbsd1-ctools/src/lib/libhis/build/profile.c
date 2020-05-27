/*******************************************************************************
 * 
 * Copyright (c) 1993, 1995 Intel Corporation
 * 
 * Intel hereby grants you permission to copy, modify, and distribute this
 * software and its documentation.  Intel grants this permission provided
 * that the above copyright notice appears in all copies and that both the
 * copyright notice and this permission notice appear in supporting
 * documentation.  In addition, Intel grants this permission provided that
 * you prominently mark as "not part of the original" any modifications
 * made to this software or documentation, and that the name of Intel
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software or the documentation without specific,
 * written prior permission.
 * 
 * Intel Corporation provides this AS IS, WITHOUT ANY WARRANTY, EXPRESS OR
 * IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY
 * OR FITNESS FOR A PARTICULAR PURPOSE.  Intel makes no guarantee or
 * representations regarding the use of, or the results of the use of,
 * the software and documentation in terms of correctness, accuracy,
 * reliability, currentness, or otherwise; and you rely on the software,
 * documentation and results solely at your own risk.
 *
 * IN NO EVENT SHALL INTEL BE LIABLE FOR ANY LOSS OF USE, LOSS OF BUSINESS,
 * LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES
 * OF ANY KIND.  IN NO EVENT SHALL INTEL'S TOTAL LIABILITY EXCEED THE SUM
 * PAID TO INTEL FOR THE PRODUCT LICENSED HEREUNDER.
 * 
 ******************************************************************************/

/*********************************************************************
 *
 * 80960 Profiler, Mon960 Library File
 *
 * This file exists as part of the nindy runtime library. It's
 * contents support runtime profiling by providing a set of common
 * functions to initialize profiling timer & interrupts, malloc
 * profile data space, handle timer interrupts, and writing profile
 * data to a file upon program completion.
 *
 *********************************************************************/

	/************************
	 *			*
	 *    INCLUDE FILES     *
	 *			*
	 ************************/

#include <stdio.h>
#include <fcntl.h>

	/************************
	 *			*
	 * EXTERNAL DEFINITIONS *
	 *			*
	 ************************/

extern void	_buck_size();
extern void	_timer_freq();
extern void	_prof_start();
extern void	_prof_end();
extern void	etext();
extern void	_init_int_vector();
extern void	_init_p_timer();
extern void	_term_p_timer();

#pragma noinline     /* Prevent code bloat */

	/************************
	 *			*
	 *   GLOBAL VARIABLES   *
	 *			*
	 ************************/

/*
 * Profile_struct is the wrapper structure for all
 * the parameters for any one profile execution.
 */
typedef struct {
	int		frequency;	/* timer interrupt frequency flag */
	int		bucket_size;	/* size of each hit bucket */
	unsigned int	*bucket_store; 	/* pointer to bucket base */
	unsigned int	code_begin;  	/* start address of code */
	unsigned int	code_end;  	/* end address of code */
} profile_struct;

static volatile profile_struct	prof;		/* profile parameters */
static int              		mem_ok;		/* malloc success flag */

unsigned int	__prof_start	= (unsigned int)&_prof_start;
unsigned int	__prof_end	= (unsigned int)&_prof_end;
unsigned int	__buck_size	= (unsigned int)&_buck_size;
unsigned int	__timer_freq	= (unsigned int)&_timer_freq;



/*********************************************************************
 *
 * NAME
 *	_init_profile - ready timer, bucket space, & interrupts
 *
 * DESCRIPTION
 *	This function is responsible for initializing everything
 *	required for profiling. This means malloc'ing space for
 *	the buckets, setting and enabling the timer, and getting the
 *	interrupt vector table to call our interrupt service routine.
 *	Note that all timer functions are resident in the board
 *	specific library.
 *
 * PARAMETERS
 *	None.
 *
 * RETURNS
 *	Void.
 *
 *********************************************************************/

static void _init_profile();

void (* const __ghist_profile_init_ptr)() = _init_profile;

int __ghist_no_fs_func,__ghist_timer_designator;

static void
_init_profile()
{
    int		i;		/* loop control */
    int		bucket_count;	/* number of buckets to malloc */
    unsigned int	*temp_ptr;	/* temporary pointer to malloc mem */
    void		_term_profile();

    /*
     * Set up the profiler parameters. Using the linkers defsym
     * invocation line command, the following four profiler
     * parameters are alterable by the user.
     */
    prof.bucket_size = __buck_size;
    prof.frequency = __timer_freq;

    if (__prof_start != 1)
	    prof.code_begin = __prof_start;
    else {
	extern char _Btext;

	prof.code_begin = (unsigned int) &_Btext;
    }

    if (__prof_end != 1)
	    prof.code_end = __prof_end;
    else
	    prof.code_end = (unsigned int) etext;

    /*
     * The best way to store timer interrupt information
     * (unfortunately) is to malloc a table for storage of the
     * buckets. This is done by taking the code size and dividing
     * by the bucket size to get the number of buckets.
     */
    bucket_count = ((prof.code_end - prof.code_begin) / prof.bucket_size);
    prof.bucket_store = (unsigned int *) malloc(bucket_count * sizeof(int));
    if (prof.bucket_store == NULL) {
	write (2, "ghist960 profile memory allocation failed!\n", 44);
	write (2, "        No statistics will be kept.     \n\n", 44);
	mem_ok = 0;
	return;
    }

    /*
     * register our exit handler to write the profile data to host
     * Do this now so if it fails we won't have already set up the
     * stuff.
     */
    if(atexit(_term_profile)) {
	write (2, "ghist960 exit handler registration failed!\n", 44);
	write (2, "        No statistics will be kept.     \n\n", 44);
	return;
    }

    mem_ok = 1;

    /*
     * Zero the bucket memory space.
     */
    temp_ptr = (unsigned int *) prof.bucket_store;
    for (i = 0; i < bucket_count; i++) {
	*temp_ptr++ = 0;
    }

    if (1) {
	/* FIXME: move these types to sdm.h and then #include "sdm.h" and include
	 the path to sdm.h to the makefile. */
	typedef enum {DEFAULT_TIMER, t0, t1, t2, t3, t4, t5, NO_TIMER} logical_timer;
	typedef enum {ghist_timer_client, bentime_timer_client, other_timer_client} timer_client;
	int _set_p_timer(timer_client, logical_timer , void (*)());
	static void _store_prof_data();
	int x;

	x = ((int) &__ghist_timer_designator) & 3;

	_set_p_timer(ghist_timer_client,(logical_timer) x,_store_prof_data);
    }

    /*
     * Call the function responsible for writing the timer
     * interrupt service routine address to the interrupt
     * vector table.
     */
    _init_int_vector();
 	
    /*
     * Call the functions responsible for initializing the
     * timer hardware and kicking off the counter at the
     * correct frequency.
     */
    _init_p_timer(prof.frequency);
}


/*********************************************************************
 *
 * NAME
 *	_store_prof_data - map the IP to a bucket, recording hit
 *
 * DESCRIPTION
 *	This function maps the current instruction's IP to a 
 *	single bucket, then bumps the hit count of that bucket.
 *
 * PARAMETERS
 *	The IP address to map.
 *
 * RETURNS
 *	Nothing.
 *
 *********************************************************************/

static void
_store_prof_data(ip)
    unsigned int ip;
{
	unsigned int	*bucket_ptr;	/* pointer to the IP's bucket */
	unsigned int	tmp;		/* address calculation temp */

	if ((ip > prof.code_begin) && (ip < prof.code_end))
	{
		/*
		 * Subtract the profile text address start to get
		 * the offset into bucket space, then divide by the
		 * bucket size to obtain the bucket number.
		 */
		tmp = (ip - prof.code_begin);
		tmp = (tmp / ((unsigned int) prof.bucket_size));

		/*
		 * Map the instruction pointer's bucket number into
		 * a bucket address, and increment.
		 */
		bucket_ptr = (prof.bucket_store + tmp);
 		(*bucket_ptr)++; 
	}
}

	
static
int le_int_write(int desc,int *intptr,int count)
{
    short be_probe = 0x0100;
    unsigned char tmpbuff[sizeof(int)*BUFSIZ*2];
    
    if (*(char *)&be_probe == 0x01) {  /* We are running in big endian mode. */
	int i,j;
	
	for (i=0;i < count;i++)
		for (j=0;j < sizeof(int);j++)
			tmpbuff[i*sizeof(int)+j] = ((intptr[i]) >> (8*j)) & 0xff;
	intptr = (int *) tmpbuff;
    }
    if (write(desc,intptr,sizeof(int)*count) != sizeof(int)*count)
	    return -1;
    return count;
}


/*********************************************************************
 *
 * NAME
 *	_term_profile - terminate profile & write results
 *
 * DESCRIPTION
 *	This function is calls the board specific function which
 *	terminates the profiling timer interrupts, then it cycles
 *	through the bucket space, writing bucket hits and associated
 *	text space addresses to a data file always name "ghist.data"
 *	for post-processing.
 *
 * PARAMETERS
 *	None.
 *
 * RETURNS
 *	Void.
 *
 *********************************************************************/

void
_term_profile()
{
    unsigned int	*bucket_ptr;	/* pointer to bucket space */
    unsigned int	*code_count;	/* address of bucket hit */
    unsigned int	code_inc;	/* address counter increment */
    int	 	bucket_count;	/* number of buckets */
    int	 	i;		/* loop control */
    int		data_desc;	/* ghist data storage file */
    int		buf[BUFSIZ*2];	/* buffer for data storage */
    int		n_in_buf;
    
    /*
     * If the malloc failed in init no need to
     * continue with anything else.
     */
    if (mem_ok == 0) {
	return;
    }
    
    /*
     * Call the function which disables the timer. Then
     * calculate the bucket address start, number of buckets,
     * code address start, and code address increment.
     */
    _term_p_timer();
    
    code_count = (unsigned int *) prof.code_begin;
    bucket_count = ((prof.code_end - prof.code_begin) / prof.bucket_size);
    bucket_ptr = (unsigned int *) prof.bucket_store;
    code_inc = (prof.bucket_size / sizeof(int));
    
    /*
     * Open the ghist data storage file.
     *  NOTE:::: O_BINARY doesn't work on some UNIX hosts under NINDY.
     */
    data_desc = open ("ghist.dat", (O_TRUNC|O_WRONLY|O_BINARY|O_CREAT), 0666);
    if (data_desc < 0) {
	if (__ghist_no_fs_func) {
	    typedef void (*void_func_ptr)();
	    
	    ((void_func_ptr) &__ghist_no_fs_func)(bucket_ptr,bucket_count,prof.code_begin,
						  prof.bucket_size);
	    return;
	}
	
	write(2, "Cannot open file ghist.dat for write.\n", 40);
	free(prof.bucket_store);
	close(data_desc);
	return;
    }
    
    /*
     * Write the bucket size to the file.
     */
    if (le_int_write(data_desc, &prof.bucket_size, 1) != 1) {
	write (2, "Data write error to file ghist.dat.\n", 37);
	free(prof.bucket_store);
	close(data_desc);
	return;
    }
    
    /*
     * Cycle through the text buckets writing the bucket IP and
     * it's number of hits.
     */
    n_in_buf = 0;
    for (i = 0; i < bucket_count; i++, bucket_ptr++)
        {
	    if (*bucket_ptr != 0)
		{
		    if (n_in_buf >= BUFSIZ*2)
			{
			    if (le_int_write(data_desc, buf, n_in_buf) !=
				n_in_buf)
				{
				    write (2, "Data write error to file ghist.dat.\n", 37);
				    free(prof.bucket_store);
				    close(data_desc);
				    return;
				}
			    n_in_buf = 0;
			}
		    
		    buf[n_in_buf++] = (int)code_count;
		    buf[n_in_buf++] = *bucket_ptr;
		}
	    code_count += code_inc;
	}
    
    if (n_in_buf > 0)
        {
	    if (le_int_write(data_desc, buf, n_in_buf) !=
		n_in_buf)
		{
		    write (2, "Data write error to file ghist.dat.\n", 37);
		    free(prof.bucket_store);
		    close(data_desc);
		    return;
		}
        }
    close(data_desc);
    free(prof.bucket_store);
}
