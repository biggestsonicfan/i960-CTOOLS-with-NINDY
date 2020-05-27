
/*(c****************************************************************************** *
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
 *****************************************************************************c)*/

#ifndef _GNUDOS_
#define _GNUDOS_

/* malloc.h pulls in stdlib.h which unconditionally tries to define
 * min and max, which can cause problems.
 */
#ifdef min
#	undef min
#endif
#ifdef max
#	undef max
#endif
#include <malloc.h>
#undef min
#undef max

#ifndef HUPCL
#define HUPCL 0
#endif

#ifndef __HIGHC__
#if defined(WIN95)
#define SIGALRM	14
#else
#define SIGALRM	SIGUSR1
#define SIGCLD	SIGUSR2
#endif
#define SIGQUIT	SIGTERM
#define SIGKILL	SIGABRT
#define SIGEMT	SIGSEGV
#if !defined(WIN95)
#define SIGTRAP	SIGUSR3
#endif
#define SIGHUP	SIGTERM
#endif

#define TCGETA		1
#define TCSETAF		2
#define TCFLSH		4
#define TCSBRK		3
#define TIOCGETP	TCGETA
#define TIOCSETP	TCSETAF
#define TCSETA		TCSETAF
#define TCSETAW		8
#define TCXONC		9

#if !defined(CSIZE)
/* To force readline to compile on DOS. */
#define CSIZE 0
#endif

/* Version 3.2 of highc did not define NSIG, so define it to something */
/* compatible with version 3.04. */
#if defined (__HIGHC__) && !defined(NSIG)
#define NSIG 32
#endif

/* Simulate System V passwd functionality */

#include <sys/types.h>

#if defined(__HIGHC__) || defined(WIN95)
/* Special considerations when compiling with Metaware C */

typedef short uid_t;
typedef uid_t gid_t;

#ifndef F_OK
#define	F_OK	0	/* Test for existence of file */
#endif
#ifndef X_OK
#define	X_OK	1	/* Test for execute permission; no equivalent
			 * with Metaware; but directories are executable 
			 * by default on DOS 
			 */
#endif
#ifndef W_OK
#define	W_OK	2	/* Test for write permission */
#endif
#ifndef R_OK
#define	R_OK	4	/* Test for read permission */
#endif

/* Fudge Unix-style fcntl() requests */
#define	F_DUPFD		0	/* Duplicate fildes */
#define	F_GETFD		1	/* Get fildes flags */
#define	F_SETFD		2	/* Set fildes flags */
#define	F_GETFL		3	/* Get file flags */
#define	F_SETFL		4	/* Set file flags */

/* Fudge Unix-style dirent facility */
#define MAXNAMLEN	512		/* maximum filename length */
#define DIRBUF		1048		/* buffer size for fs-indep. dirs */

struct dirent {
	ino_t		d_ino;		/* "inode number" of entry */
	off_t		d_off;		/* offset of disk directory entry */
	unsigned short	d_reclen;	/* length of this record */
	char		d_name[1];	/* name of file */
};

#if defined(__STDC__)
int getdents(int, struct dirent *, unsigned);
#else
int getdents( );
#endif

typedef struct
	{
	int		dd_fd;		/* file descriptor */
	int		dd_loc;		/* offset in block */
	int		dd_size;	/* amount of valid data */
	char		*dd_buf;	/* directory block */
	}	DIR;			/* stream data from opendir() */

#if defined(__STDC__)
extern void		rewinddir( DIR * );
#else
extern void		rewinddir();
#endif

#endif 	/* Metaware HIGH-C or Microsoft C 9.00 (Windows 95) */

struct passwd {
	char *  pw_name;
	char *  pw_passwd;
	uid_t   pw_uid;
	uid_t   pw_gid;
	char *  pw_age;
	char *  pw_comment;
	char *  pw_gecos;
	char *  pw_dir;
	char *  pw_shell;
};


/* For file access() calls */
#define CHECK_FILE_EXISTENCE 0
#define FILE_EXISTS          0


/* Function prototypes */
extern int get_response_file(int, char ***);
char **check_dos_args(char **);
char **check_dos_args_with_name(char **, char *);
char *dosslash( char * );
char *set_dos_baud( char * );


/* Compare two file names, ignoring case and treating slash == backslash */
extern int  is_same_file_by_name(char*, char*);
extern char *normalize_file_name(char*);

/* Variables controlling behavior of check_dos_args and get_response_file */

extern	int	perform_file_name_normalization;
extern	int	perform_unix_dos_translation;
extern	int	support_contiguous_slos;
extern	char	*default_temp_dir_env_var;
extern	char	*slo_with_arg;
extern	char	*slo_with_narg;
extern	char	**mlo_with_arg;
extern	char	**mlo_with_narg;

#if !defined(WIN95)
#define fcntl(a,b,c)	(0)	/* No-op */

/******************************************************
 * sys V "termio" emulation
 *****************************************************/
#include <bios.h>
#endif

/* control characters */
#define VINTR   0
#define VQUIT   1
#define VERASE  2
#define VKILL   3
#define VEOF    4
#define VEOL    5
#define VEOL2   6
#define VSWTCH  7
#define VMIN    8
#define VTIME   9
 
#define NCC	10

struct termio {
	unsigned short	c_iflag;	/* input modes */
	unsigned short	c_oflag;	/* output modes */
	unsigned long	c_cflag;	/* control modes */
	unsigned short	c_lflag;	/* line discipline modes */
	char	c_line;			/* line discipline */
	unsigned char	c_cc[NCC];	/* control chars */
};

/* control modes */
#define	B110	(110<<16)
#define	B300	(300<<16)
#define	B600	(600<<16)
#define	B1200	(1200<<16)
#define	B2400	(2400<<16)
#define	B4800	(4800<<16)
#define	B9600	(9600<<16)
#define	B19200	(19200<<16)
#define	EXTA	B19200
#define	B38400	(38400<<16)
#define	EXTB	B38400
#define CBAUD	0xffff0000

/* number of data bits */
#if defined(WIN95)
#ifndef S_ISREG
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif
#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif
#define S_IFBLK	0
#define	CS8	0
#define	CS7	0

/* number of stop bits */
#define CSTOPB	0
#else
#define	CS8	_COM_CHR8
#define	CS7	_COM_CHR7

/* number of stop bits */
#define CSTOPB	_COM_STOP2	/* normally one stop bit */
#endif

/* parity normally enabled, even */
#define PARODD	STICKPARITY
#define PARENB	0x38

#define	CREAD	0
#define	CLOCAL	0

#if 0
/* PS: Found here 12/93 */
#define	IXON	0x00000001
#define	IXOFF	0x00000002
#define	BRKINT	0x00000004
#define	IGNBRK	0x00000008
#define IXANY	0x00000010
#define ISTRIP	0x00000020
#define INPCK	0x00000040
#endif

/* Found in SVR4 termios.h */
#define	IGNBRK	0000001
#define	BRKINT	0000002
#define	IGNPAR	0000004
#define	PARMRK	0000010
#define	INPCK	0000020
#define	ISTRIP	0000040
#define	INLCR	0000100
#define	IGNCR	0000200
#define	ICRNL	0000400
#define	IUCLC	0001000
#define	IXON	0002000
#define	IXANY	0004000
#define	IXOFF	0010000

#define TCOOFF		0  /* suspend output */
#define TCOON		1  /* restart suspended output */
#define TCIOFF		2  /* suspend input */
#define TCION		3  /* restart suspended input */

#define ICANON	0x1

#ifndef ECHO
	/* Conflicts with a symbol generated by lex, and 
	 * is not really needed by gld960:ldlex.l
	 */
#define ECHO	0x2
#endif

/* line discipline 0 modes */
#define	ISIG	0000001

#ifdef _INTELC32_
/********************************************************
**  Initialize the internal Code Builder run time libraries
**  to accept the maximum number of files open at a time
**  that it can.  With this set high, the number of files
**  that can be open at a time is completely dependent on
**  DOS config.sys FILES= command.
***********************************************************/
#define MAX_CB_FILES	255
#endif

#endif /*_GNUDOS_*/
