/*********************************************************************
 *
 * 		Copyright (c) 1989, Intel Corporation
 *
 * Intel hereby grants you permission to copy, modify, and 
 * distribute this software and its documentation.  Intel grants
 * this permission provided that the above copyright notice 
 * appears in all copies and that both the copyright notice and
 * this permission notice appear in supporting documentation.  In
 * addition, Intel grants this permission provided that you
 * prominently mark as not part of the original any modifications
 * made to this software or documentation, and that the name of 
 * Intel Corporation not be used in advertising or publicity 
 * pertaining to distribution of the software or the documentation 
 * without specific, written prior permission.  
 *
 * Intel Corporation does not warrant, guarantee or make any 
 * representations regarding the use of, or the results of the use
 * of, the software and documentation in terms of correctness, 
 * accuracy, reliability, currentness, or otherwise; and you rely
 * on the software, documentation and results solely at your own 
 * risk.
 *
 *********************************************************************/

/*********************************************************************
 *
 * 80960 Profiler, Nindy Library File
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
extern void	default_pstart();
extern void	init_p_timer();
extern void	term_p_timer();


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

static profile_struct	prof;		/* profile parameters */
static int		mem_ok;		/* malloc success flag */



/*********************************************************************
 *
 * NAME
 *	init_profile - ready timer, bucket space, & interrupts
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

void
init_profile() 

{
	int		i;		/* loop control */
	int		bucket_count;	/* number of buckets to malloc */
	unsigned int	*temp_ptr;	/* temporary pointer to malloc mem */

	/*
	 * Set up the profiler parameters. Using the linkers defsym
	 * invocation line command, the following four profiler
	 * parameters are alterable by the user.
	 */
	prof.bucket_size = (int) _buck_size;
	prof.frequency = (int) _timer_freq;

	if (((void *) _prof_start) != ((void *) 1)) {
		prof.code_begin = (unsigned int) _prof_start;
	}
	else {
		prof.code_begin = (unsigned int) default_pstart;
	}
	if (((void *) _prof_end) != ((void *) 1)) {
		prof.code_end = (unsigned int) _prof_end;
	}
	else {
		prof.code_end = (unsigned int) etext;
	}

	/*
	 * The best way to store timer interrupt information
	 * (unfortunately) is to malloc a table for storage of the
	 * buckets. This is done by taking the code size and dividing
	 * by the bucket size to get the number of buckets.
	 */
	bucket_count = ((prof.code_end - prof.code_begin) / prof.bucket_size);
	prof.bucket_store = (unsigned int *) malloc(bucket_count * sizeof(int));
	if (prof.bucket_store == NULL) {
		printf ("libnin: profiler memory allocation failed!\n");
		printf ("        No statistics will be kept.\n\n");
		mem_ok = 0;
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

	/*
	 * Call the function responsible for writing the timer
	 * interrupt service routine address to the interrupt
	 * vector table.
	 */
	init_int_vector();
	
	/*
	 * Call the functions responsible for initializing the
	 * timer hardware and kicking off the counter at the
	 * correct frequency.
	 */
	init_p_timer(prof.frequency);
}



/*********************************************************************
 *
 * NAME
 *	store_prof_data - map the IP to a bucket, recording hit
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

void
store_prof_data(ip)

unsigned int ip;

{
	unsigned int	*bucket_ptr;	/* pointer to the IP's bucket */
	unsigned int	tmp;		/* address calculation temp */

	if ((ip > prof.code_begin) && (ip < prof.code_end)) {

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

	

/*********************************************************************
 *
 * NAME
 *	term_profile - terminate profile & write results
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
term_profile()

{
	unsigned int	*bucket_ptr;	/* pointer to bucket space */
	unsigned int	*code_count;	/* address of bucket hit */
	unsigned int	code_inc;	/* address counter increment */
	int	 	bucket_count;	/* number of buckets */
	int	 	i;		/* loop control */
	FILE		*data_desc;	/* ghist data storage file */

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
	term_p_timer();
	code_count = (unsigned int *) prof.code_begin;
	bucket_count = ((prof.code_end - prof.code_begin) / prof.bucket_size);
	bucket_ptr = (unsigned int *) prof.bucket_store;
	code_inc = (prof.bucket_size / sizeof(int));

#ifdef ASCII_DATA
	/*
	 * Print the ghist data header and the bucket size.
	 */
	printf("---Begin-Profile---\n");
	printf("bucketsize %d\n", prof.bucket_size);

	/*
	 * Cycle through the text buckets printing the number
	 * hits.
	 */
	for (i = 0; i < bucket_count; i++, bucket_ptr++) {
		if (*bucket_ptr != 0) {
			printf("address I 0x%x hits %d\n", code_count, *bucket_ptr); 
		}
		code_count += code_inc;
	}

	printf("---End-Profile---\n");
#else

	/*
	 * Open the ghist data storage file.
	 */
	if ((data_desc = fopen("ghist.data", "w")) == NULL) {
		printf("libnin: could not open ghist960 data file\n");
		free(prof.bucket_store);
		return;
	}

	/*
	 * Write the bucket size to the file.
	 */
	if (fwrite(&prof.bucket_size, sizeof(int), 1, data_desc) < 1) {
		printf("libnin: ghist960 data write error\n");
		free(prof.bucket_store);
		fclose(data_desc);
		return;
	}

	/*
	 * Cycle through the text buckets writing the bucket IP and
	 * it's number of hits.
	 */
	printf("Writing ghist960 profile data");
	for (i = 0; i < bucket_count; i++, bucket_ptr++) {
		if ((i % 500) == 0) {
			printf(".");
		}
		if (*bucket_ptr != 0) {
			if (fwrite(&code_count, sizeof(int), 1, data_desc) < 1){
				printf("\nlibnin: ghist960 data write error\n");
				free(prof.bucket_store);
				fclose(data_desc);
				return;
			}
			if (fwrite(bucket_ptr, sizeof(int), 1, data_desc) < 1) {
				printf("\nlibnin: ghist960 data write error\n");
				free(prof.bucket_store);
				fclose(data_desc);
				return;
			}
		}
		code_count += code_inc;
	}
	printf("done\n");
	fclose(data_desc);
#endif
	free(prof.bucket_store);
}
