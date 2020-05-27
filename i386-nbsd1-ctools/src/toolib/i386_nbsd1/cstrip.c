
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



#include <stdio.h>
#include <sys/types.h>	/* Needed by file.h on Sys V */
#include <sys/stat.h>
#if !defined(NO_BIG_ENDIAN_MODS)
#include "coff.h"
#define SWAP_SHORT(x) ((((x) & 0xFF) << 8) | (((x) >> 8) & 0xFF))
#endif /* NO_BIG_ENDIAN_MODS */

#ifdef USG
#if !defined(__HIGHC__) && !defined(WIN95)
#	include <unistd.h>
#endif
#	include "sysv.h"
#else	/* BSD */
#	include "string.h"
#	include <sys/time.h>
#endif

#if defined(__HIGHC__) || defined(WIN95)
#	include <stdlib.h>
#if defined(WIN95)
extern char *dosslash(char *);
#endif
#else
extern char *malloc();
extern void free();
#endif

/******************************************************************************
 * exists:
 *	Creates a full pathname by concatenating up to three name components
 *	onto a specified base name; optionally looks up the base name as a
 *	runtime environment variable;  and checks to see if the file or
 *	directory specified by the pathname actually exists.
 *
 *	Returns:  the full pathname if it exists, NULL otherwise.
 *		(returned pathname is in malloc'd memory and must be freed
 *		by caller).
 *****************************************************************************/

#ifdef DOS
#	define DIRCHR	'\\'
#	define DIRSTR	"\\"
#else
#	define DIRCHR	'/'
#	define DIRSTR	"/"
#endif /*DOS*/

char *
exists( base, c1, c2, c3, env )
    char *base;		/* Base directory of path */
    char *c1, *c2, *c3;	/* Components (subdirectories and/or file name) to be
			 *	appended onto the base directory name.  One or
			 *	more may be omitted by passing NULL pointers.
			 */
    int env;		/* If 1, '*base' is the name of an environment variable
			 *	to be examined for the base directory name;
			 *	otherwise, '*base' is the actual name of the
			 *	base directory.
			 */
{
	struct stat buf;/* For call to 'stat' -- never examined */
	char *path;	/* Pointer to full pathname (malloc'd memory) */
	int len;	/* Length of full pathname (incl. terminator) */
	extern char *getenv();


	if ( env ){
		base = getenv( base );
		if ( base == NULL ){
			return NULL;
		}
	}

	len = strlen(base) + 4;
			/* +4 for terminator and "/" before each component */
	if ( c1 != NULL ){
		len += strlen(c1);
	}
	if ( c2 != NULL ){
		len += strlen(c2);
	}
	if ( c3 != NULL ){
		len += strlen(c3);
	}

	path = malloc( len );

	strcpy( path, base );
	if ( c1 != NULL ){
		strcat( path, DIRSTR );
		strcat( path, c1 );
		if ( c2 != NULL ){
			strcat( path, DIRSTR );
			strcat( path, c2 );
			if ( c3 != NULL ){
				strcat( path, DIRSTR );
				strcat( path, c3 );
			}
		}
	}

	if ( stat(path,&buf) != 0 ){
		free( path );
		path = NULL;
	}

#ifdef DOS
	path = dosslash(path);
#endif /*DOS*/
	return path;
}


/*****************************************************************************
 * coffstrip:
 *	Passed the name of an executable object file in either b.out or
 *	COFF format.
 *
 *	If the file is in b.out format, it is converted to little-endian
 *	COFF format (i.e., the format suitable for downloading to NINDY).
 *	In either case, the COFF file is then stripped of all relocation
 *	and symbol information, to speed up its download.
 *
 * RETURNS:
 *	pointer to the name of the stripped COFF file (a tmp file that has
 *	been created and closed); NULL on error.
 *****************************************************************************/

#ifdef DOS
#	define GSTRIP	"objcopy.exe" /* bout -> coff converter, gnu historical name */
#	define CSTRIP	"cof960.exe" /* bout -> coff converter, ctools historical name */
#	define TEMPNAME "cmXXXXXX"	/* update template[] if length grows */
#else
#	define GSTRIP	"objcopy" /* bout -> coff converter, gnu historical name */
#	define CSTRIP	"cof960" /* bout -> coff converter, ctools historical name */
#	define TEMPNAME "commXXXXXX"	/* update template[] if length grows */
#endif /*DOS*/

static char template[16];

char *
coffstrip( fn, bigendian )
    char *fn;	/* Name of object file */
#if !defined(NO_BIG_ENDIAN_MODS)
    int  bigendian;	/* =0 => result file has little-endian host info */
			/* >0 => result file has big-endian    host info */
			/* <0 => result file has host info in same byte  */
			/*       order as target info                    */
#else
    int  bigendian;   /* == 0 iffi you want the resulting file to be little endian. */
#endif /* NO_BIG_ENDIAN_MODS */
{
	extern char *mktemp();
	char *strip;	/* Pointer to full pathname of strip utility	*/
	char *tempfile;	/* Stripped copy of object file			*/
	char buf[400];
#if !defined(NO_BIG_ENDIAN_MODS)
	char big_or_little;

	/* determine result host info byte order */
	if ( 0 == bigendian ) big_or_little = 'l';
	else if ( 0 < bigendian ) big_or_little = 'b';
	else 
	{
		FILE *fp;
		FILHDR fhdr;

#ifdef DOS
		if ((fp = fopen(fn, "rb")) == (FILE *)NULL)
#else
		if ((fp = fopen(fn, "r")) == (FILE *)NULL)
#endif /* DOS */
		{
			perror(fn);
			return(NULL);
		}

		if (fread(&fhdr, FILHSZ, 1, fp) != 1)
		{
			perror(fn);
			return(NULL);
		}

		fclose(fp);

		if (ISCOFF(fhdr.f_magic))
			big_or_little = ((fhdr.f_flags & F_BIG_ENDIAN_TARGET) ? 'b' : 'l');
		else if (ISCOFF(SWAP_SHORT(fhdr.f_magic)))
			big_or_little = ((SWAP_SHORT(fhdr.f_flags) & F_BIG_ENDIAN_TARGET) ? 'b' : 'l');
		else /* not a COFF file */
			big_or_little = 'l'; /* assumes b.out format */
	}
#else
	char big_or_little = (bigendian ? 'b' : 'l');
#endif /* NO_BIG_ENDIAN_MODS */

#ifdef  DOS
	/* objcopy can't handle file names beginning with '/' */
	if ( fn && *fn == '/' )
		*fn = '\\';
#endif
	/* get_960_tools_temp_file() always returns a valid temporary
	   temp file. */
	if (1) {
	    tempfile = (char *) get_960_tools_temp_file("STXXXXXX",malloc);
	}

	/* Make sure the strip utility is findable.
	 */
	if ( (strip = exists("G960BIN",GSTRIP,NULL,NULL,1)) == NULL
	    && (strip = exists("G960BASE","bin",GSTRIP, NULL,1)) == NULL
	    && (strip = exists("I960BASE","bin",CSTRIP, NULL,1)) == NULL )
	{
	    strip = GSTRIP;
	}

	sprintf( buf, "%s -S -q -%c -Fcoff %s %s", strip, big_or_little,
		fn, tempfile );
	if ( system(buf) == 0 ) 
	    return tempfile;
	else
	{
	    fprintf(stderr, "Error invoking '%s' utility (a.k.a. '%s')\n", GSTRIP, CSTRIP);
	    fprintf(stderr, "Check env variables G960BIN, G960BASE, I960BASE\n");
	    return NULL;
	}
}


