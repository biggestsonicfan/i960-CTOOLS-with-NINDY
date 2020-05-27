
/*(c**************************************************************************** *
 * Copyright (c) 1990, 1991, 1992, 1993 Intel Corporation
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
 ***************************************************************************c)*/


/* 
** bfio.h - Binary file interface definitions
**          Developed for VAX/VMS. Defines standard functions
**          for non-VMS
*/
#ifndef BFIO_H
#define BFIO_H 1

/*
 * INCLUDE FILE: bfio.h
 *
 * SUPPORTED HOSTS:
 *     All currently supported host environments.  The list of supported
 *     hosts at this time includes VMS, Ultrix, Sun-OS, HPUX, Unix
 *     System V3.2, Macintosh OS, DOS (Microsoft C Compiler), Extended
 *     DOS (Metaware C Compiler), and OS2 V1.2.
 *
 * DESCRIPTION:
 *     bfio.h contains the required definitions for the DTO Binary
 *     File Interface (BFI).  The BFI provides a set of functions that
 *     access and modify binary files in a host independent manner.
 *     For VMS, bfio.h contains the data structures and external
 *     function declarations needed by the BFI.  For all other hosts,
 *     the BFI function names are defined to be their ANSI standard
 *     stdio equivalents since the stdio functions provide sufficient
 *     support for binary files.
 *
 *     The BFI supports the following functions for binary files:
 *
 *         btmpfile  - binary version of ANSI standard tmpfile.
 *         bfclose   - binary version of ANSI standard fclose.
 *         bfflush   - binary version of ANSI standard fflush.
 *         bfopen    - binary version of ANSI standard fopen.
 *         bfread    - binary version of ANSI standard fread.
 *         bgetc     - binary version of ANSI standard getc (obsolete).
 *         bfwrite   - binary version of ANSI standard fwrite.
 *         bfseek    - binary version of ANSI standard fseek.
 *         bftell    - binary version of ANSI standard ftell.
 *         brewind   - binary version of ANSI standard rewind.
 *         bfeof     - binary version of ANSI standard feof.
 *         bclearerr - binary version of ANSI standard clearerr.
 *         bferror   - binary version of ANSI standard ferror.
 *
 *     bfio.h also contains a set of macros to be used when opening
 *     binary files with bfopen.  These macros define the appropriate
 *     protection string and any other information required to open the
 *     file correctly.  The append modes ("a", "ab", "a+", "ab+") are
 *     not currently supported for binary files on any host due to their
 *     non-standard implementation in the VMS C Run Time Library.  They
 *     may or may not be supported when the VMS version of the BFI is
 *     modified to bypass the VMS C Run Time Library and call the DEC
 *     Record Management Services (RMS) directly.
 *
 *     A set of macros to be used with fopen for text files is also
 *     provided.  Although the BFI does not support text files, there
 *     are some special options that must be passed to fopen for text
 *     files on some systems.  These macros could be moved to another
 *     include file if they promote confusion.
 *
 * ASSUMPTIONS:
 *     The BFI does not depend on the _fmode variable used by some C
 *     Compilers (such as the Microsoft C Compiler) for determining
 *     the type of newly opened files.  All files are opened with
 *     explicit file permissions.
 */

#ifdef VMS

/*
 * The data structures and functions for the VMS version of the BFI are
 * required to access fixed-length 512 byte record files in a manner
 * that requires minimal knowledge of fixed-length files at the
 * application level.  Fixed-length files are used for binary files
 * because they are the most efficient representation for binary data
 * on VMS.
 */
#include <stdio.h>

/* Define values for TRUE and FALSE. */
#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

/*
 * Define the size of the fixed-length records used by binary files
 * created for DTO applications.  This size is expected to be 512, but
 * other integer multiples of 512 could be used.  The unit tests for
 * the BFI will not depend on a record size of 512.
 *
 * The record size specification to be used when binary files are opened
 * is also specified here.  The numbers in these two macros must match.
 */
#define FIX_REC_SIZ 512
#define FIX_REC_SPEC "mrs=512"

/*
 * Define the size of the internal buffer associated with each file
 * pointer.  The buffer size must be an integer multiple of the record
 * size (FIX_REC_SIZ).
 */
#ifndef FIL_BUF_SIZ
#ifdef UNIT_TEST
#define FIL_BUF_SIZ FIX_REC_SIZ
#else
#define FIL_BUF_SIZ (FIX_REC_SIZ*16)
#endif
#endif

struct _bfio_binary_file {
	short flags;              /* Attributes associated with the file */
	short index;              /* Index within array of open binary files */
	long file_size;           /* Size of file as of last write */
	FILE *fd;                 /* Host operating system file descriptor */
	char *buf;                /* Buffer of information from file */
	long start_pos;           /* File start position of buffer */
	long curr_pos;            /* Current offset within buffer */
	long farthest_write;      /* Highest position in buffer receiving write */
	char *file_name;          /* name of file - required for deletion */
};
typedef struct _bfio_binary_file BFILE;

/*
 * Define the file attributes represented by the "flags field of the
 * binary file pointer.
 */
#define BFILE_FIXED_LENGTH   1  /* Enabled if fixed-length file */
#define BFILE_STREAM_LF      2  /* Enabled if fixed-length file */
#define BFILE_DATA_WRITTEN   4  /* Enabled if data written to file */
#define BFILE_DATA_READABLE  8  /* Enabled if file is user readable */
#define BFILE_TEMP_FILE     16  /* Enabled if file is temporary */
#define BFILE_RW_ERROR      32  /* Enabled if read or write error occurred */

/*
 * Define the macros to enable, disable, and determine the state of the
 * file attributes flags.
 */
#define bfio_enable_fixed_length(bfp) ((bfp)->flags |= BFILE_FIXED_LENGTH)
#define bfio_is_fixed_length(bfp) ((bfp)->flags & BFILE_FIXED_LENGTH)

#define bfio_enable_stream_lf(bfp) ((bfp)->flags |= BFILE_STREAM_LF)
#define bfio_is_stream_lf(bfp) ((bfp)->flags & BFILE_STREAM_LF)

#define bfio_enable_data_written(bfp) ((bfp)->flags |= BFILE_DATA_WRITTEN)
#define bfio_disable_data_written(bfp) ((bfp)->flags & ~BFILE_DATA_WRITTEN)
#define bfio_is_data_written(bfp) ((bfp)->flags & BFILE_DATA_WRITTEN)

#define bfio_enable_data_readable(bfp) ((bfp)->flags |= BFILE_DATA_READABLE)
#define bfio_is_data_readable(bfp) ((bfp)->flags & BFILE_DATA_READABLE)

#define bfio_enable_temp_file(bfp) ((bfp)->flags |= BFILE_TEMP_FILE)
#define bfio_is_temp_file(bfp) ((bfp)->flags & BFILE_TEMP_FILE)

#define bfio_enable_rw_error(bfp) ((bfp)->flags |= BFILE_RW_ERROR)
#define bfio_disable_rw_error(bfp) ((bfp)->flags & ~BFILE_RW_ERROR)
#define bfio_is_rw_error(bfp) ((bfp)->flags & BFILE_RW_ERROR)

/*
 * Declare the functions supported by the BFI.
 */	
extern BFILE *btmpfile(void);
extern int bfclose(BFILE *);
extern int bfflush(BFILE *);
extern BFILE *bfopen(const char *, const char *);
extern size_t bfread(char *, size_t, size_t, BFILE *);
extern int bgetc(BFILE *);  /* obsolete */
extern size_t bfwrite(const char *, size_t, size_t, BFILE *);
extern int bfseek(BFILE *, long int, int);
extern long bftell(BFILE *);
extern void brewind(BFILE *);
extern int bfeof(BFILE *);
extern void bclearerr(BFILE *);
extern int bferror(BFILE *);

/*
 * Define the mode string macros for opening binary files with bfopen.
 */
#define BFOPEN_RDONLY "rb"
#define BFOPEN_WRONLY_TRUNC "wb"
/* #define BFOPEN_WRONLY_APPEND "ab" */
#define BFOPEN_RDWR "rb+"
#define BFOPEN_RDWR_TRUNC "wb+"
/* #define BFOPEN_RDWR_APPEND "ab+" */

/*
 * Define the mode macros for opening text files with fopen.  Text files
 * are created as variable length carriage control files instead of the
 * VAX C RTL default stream_lf format to make them more compatible with
 * other VMS utilities.  For example, many of the VMS editors have
 * trouble reading stream_lf files.
 */
#define FOPEN_RDONLY "r"
#define FOPEN_WRONLY_TRUNC "w","rfm=var","rat=cr"
#define FOPEN_WRONLY_APPEND "a","rfm=var","rat=cr"
#define FOPEN_RDWR "r+"
#define FOPEN_RDWR_TRUNC "w+","rfm=var","rat=cr"
#define FOPEN_RDWR_APPEND "a+","rfm=var","rat=cr"

/*
 * Define the macros for reopening stdin, stdout, and stderr as files.
 */
#define reopen_stdin(fn) freopen(fn, "r", stdin)
#define reopen_stdout(fn) freopen(fn, "w", stdout, "rfm=var", "rat=cr")
#define reopen_stderr(fn) freopen(fn, "w", stderr, "rfm=var", "rat=cr")

#else /* VMS */
	
#include <stdio.h>

/*
 * Define the file pointer for binary files to be its ANSI stdio
 * counterpart.
 */	
#define BFILE FILE

/*
 * Define the functions supported by the BFI to be their ANSI stdio
 * counterparts.
 */	
#define btmpfile tmpfile
#define bfclose fclose
#define bfflush fflush
#define bfopen fopen
#define bfread fread
#define bgetc getc
#define bfwrite fwrite
#define bfseek fseek
#define bftell ftell
#define brewind rewind
#define bfeof feof
#define bclearerr clearerr
#define bferror ferror

/*
 * Define the mode macros for opening binary files with bfopen.
 *
 * NOTE: As ANSI C compilers become available on additional hosts, the
 *       "b" entry in the mode string should be supported on all hosts.
 *
 * NOTE: The "ab" and "ab+" options are not supported for binary files
 *       at this time.  They may or may not be supported in the future.
 */
#if defined(MSDOS) || defined(OS2_12) || defined(MWC)
#define BFOPEN_RDONLY "rb"
#define BFOPEN_WRONLY_TRUNC "wb"
/* #define BFOPEN_WRONLY_APPEND "ab" */
#define BFOPEN_RDWR "rb+"
#define BFOPEN_RDWR_TRUNC "wb+"
/* #define BFOPEN_RDWR_APPEND "ab+" */
#else
#define BFOPEN_RDONLY "r"
#define BFOPEN_WRONLY_TRUNC "w"
/* #define BFOPEN_WRONLY_APPEND "a" */
#define BFOPEN_RDWR "r+"
#define BFOPEN_RDWR_TRUNC "w+"
/* #define BFOPEN_RDWR_APPEND "a+" */
#endif

/*
 * Define the mode macros for opening text files with fopen.
 *
 * NOTE: The "t" entry in the mode string is used on DOS and OS/2 hosts.
 *       It is used in these macro definitions so the BFI does not need
 *       to depend on the _fmode variable on these hosts.  This is not
 *       a feature supported by the ANSI standard.
 */
#if defined(MWC)
#define FOPEN_RDONLY "rt"
#define FOPEN_WRONLY_TRUNC "wt"
#define FOPEN_WRONLY_APPEND "at"
#define FOPEN_RDWR "r+t"
#define FOPEN_RDWR_TRUNC "w+t"
#define FOPEN_RDWR_APPEND "a+t"
#else
#if defined(MSDOS) || defined(OS2_12) 
#define FOPEN_RDONLY "rt"
#define FOPEN_WRONLY_TRUNC "wt"
#define FOPEN_WRONLY_APPEND "at"
#define FOPEN_RDWR "rt+"
#define FOPEN_RDWR_TRUNC "wt+"
#define FOPEN_RDWR_APPEND "at+"
#else
#define FOPEN_RDONLY "r"
#define FOPEN_WRONLY_TRUNC "w"
#define FOPEN_WRONLY_APPEND "a"
#define FOPEN_RDWR "r+"
#define FOPEN_RDWR_TRUNC "w+"
#define FOPEN_RDWR_APPEND "a+"
#endif
#endif
/*
 * Define the macros for reopening stdin, stdout, and stderr as files.
 */
#if defined(MSDOS) || defined(OS2_12) || defined(MWC)
#define reopen_stdin(fn) freopen(fn, "rt", stdin)
#define reopen_stdout(fn) freopen(fn, "wt", stdout)
#define reopen_stderr(fn) freopen(fn, "wt", stderr)
#else
#define reopen_stdin(fn) freopen(fn, "r", stdin)
#define reopen_stdout(fn) freopen(fn, "w", stdout)
#define reopen_stderr(fn) freopen(fn, "w", stderr)
#endif

#endif /* VMS */

#endif /* BFIO_H */
