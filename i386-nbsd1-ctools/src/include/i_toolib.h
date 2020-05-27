/*(c**************************************************************************
 * Copyright (c) 1994 Intel Corporation
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
 **************************************************************************c)*/

#if !defined(__I_TOOLIB_H__)
#define __I_TOOLIB_H__ 1

/*
 * Declarations for using common functionality in the toolib library.
 * Many of these functions provide a host-independent interface
 * to host-dependent implementations.
 */

/* If 'd' has any extra bits, get rid of them by forcing store and reload. */

extern double vclean_dbl;
extern double *pclean_dbl;
#define clean_dbl(d) (( (*pclean_dbl=(d)), vclean_dbl ))

extern float vclean_flt;
extern float *pclean_flt;
#define clean_flt(d) (( (*pclean_flt=(d)), vclean_flt ))

extern long dbl_to_long();

#if !defined(IS_OPTION_CHAR)
#if defined(DOS)
#define IS_OPTION_CHAR(c)	((c) == '-' || (c) == '/')
#else
#define IS_OPTION_CHAR(c)	((c) == '-')
#endif
#endif

extern char*	base_name_ptr( /* char* */ );
	/* Return a pointer to the base name portion of the given path name. */

extern void	path_concat( /* char* buf, char* p1, char* p2 */ );
	/* Concatenate p1 and p2 as file system path names, inserting
	 * a slash or backslash inbetween IF NEEDED, and write the result
	 * into buf.  buf is assumed to be large enough.
	 * Call with buf == p1 to simply append p2 in the same
	 * buffer where p1 resides.
	 */

extern int	get_response_file( /* int argc, char ***argvp */ );
	/* Expand command line response files, returning the new argc,
	 * and allocating a new argv array if needed.
	 */

extern int      get_cmd_file(/* int argc, char **argv */);
        /* Find the -c960 option that provides a command file for
         * a tool to produce if it needs any subprocesses run.  Record
         * command file name and remove option from argc, argv
         */

extern char *   get_cmd_file_name();
        /* Return the command file name found by get_cmd_file */
         
extern char	*get_960_tools_temp_dir();
	/* Return the name of the directory to be used for temporary files. */

extern char	*get_960_tools_temp_file(
				/* char *template,
				   char* (*guarded_malloc)()
				 */ );

#if !defined(DOS)
#define		normalize_file_name(f)		(f)
#define		is_same_file_by_name(a,b)	(strcmp((a),(b)) == 0)
#else
extern char*	normalize_file_name( /* char* */ );
	/* Change all letters to lowercase. */

extern int	is_same_file_by_name( /* char*, char* */ );
	/* Compare two file names, ignoring case and treating
	 * slash == backslash.
	 */
#endif /* DOS */


/*
 * Only DOS-specific functionality follows.
 */

#if defined(DOS)

extern char **	check_dos_args( /* char *argv[] */ );
	/* Work around the 128-character invocation line limitation in DOS,
	 * for tools that must invoke other tools as subprocesses.
	 * If argv exceeds the DOS length limit, arguments in argv are
	 * written to a temporary file, and argv[1] is modified to be
	 * @filename.
	 */

extern char **	check_dos_args_with_name( /* char *argv[], char* filename */ );
	/* Like check_dos_args, but uses the given file name instead of
	 * a temporary file name.
	 */

extern void	delete_response_file( /* void */ );
	/* Call this to delete the temporary response file, if any,
	 * that was created by check_dos_args().
	 */

#endif	/* DOS */

#endif /* __I_TOOLIB_H__ */
