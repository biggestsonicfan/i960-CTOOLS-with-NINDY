/*(c**************************************************************************** *
 * Copyright (c) 1990, 1991, 1992, 1993, 1994 Intel Corporation
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
#include <stdlib.h>
#include <io.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <process.h>
#include "i_toolib.h"

#define TEMP_EXT ".tmp"

/* Name of the temporary file created to invoke a subprogram. */
static char * remove_file_name;

/* The following variables can be reassigned by the user to modify
 * the default behavior of this module.  Set these variables in
 * main() prior to calling check_dos_args().
 */

/*************************************************************************
 * xslash(pathname)
 *	This routine is used to replace all backslash charaters in a
 *	string with forward slash characters.  This generally is used
 *	to make an MS/DOS directory string appear the same as a Unix
 *	directory pathname.
 ***************************************************************************/
xslash(pathname)
    char *pathname;		/* Pathname string to modify	*/
{
	char *slashloc;

	while (slashloc = strchr(pathname, (int)'\\')){
		*slashloc = '/';
	}
}


/*************************************************************************
 * dosslash()
 *	This routine is used to replace all forward slash charaters in a
 *	string with back slash characters.  This generally is used
 *	to make an Unix directory string appear the same as a MS/Dos
 *	directory pathname.
 ***************************************************************************/
char *
dosslash(pathname)
    char *pathname;		/* Pathname string to modify	*/
{
	char *slashloc;

	while (slashloc = strchr(pathname, (int)'/')) {
		*slashloc = '\\';
	}
	return pathname;
}


/*************************************************************************
 * xmalloc()
 *	malloc with error checking.
 ***************************************************************************/
static char *
xmalloc( n )
    int n;
{
	char *p;

	p = malloc(n);
	if (p == NULL){
		fprintf(stderr, "malloc error\n");
		exit(1);
	}
	return p;
}

static char *
copy_string(s)
char *s;
{
	char	*t;
	if (!s) return NULL;
	t = xmalloc(strlen(s) + 1);
	(void) strcpy(t, s);
	return t;
}

/***************************************************************************
 * check_dos_args()
 *	This routine works around the 128-character invocation line
 *	limitation in DOS, for tools that must invoke other tools as
 *	subprocesses.
 *
 *	It checks to see if the number of characters in the argv exceeds the
 *	DOS limitation.  If so, it writes a response file and modifies the
 *	passed argv to use it.
 ***************************************************************************/
char **
check_dos_args_with_name(char *argv[], char *rfile_name)
{
#define GNUTMP_DFLT     "."
#define TMPNAME		"ccXXXXXX"
#define MAXCHARS	127	/* Yes, 127 not 128 */

	int i;
	int nchars;
	char *fn;	/* filename */
	FILE *fd;
        char *tmp = NULL;
        char *q;

	/* Count up the number of characters in argv */

	nchars = strlen(argv[0]);

	remove_file_name = 0;
	for (i=1; argv[i]; i++ )
		nchars += strlen(argv[i]) + 1;

	if ( nchars > MAXCHARS )
	{
		if (rfile_name != NULL)
		{
			fn =(char*) xmalloc(strlen(rfile_name) + 2);
			fn[0] = '@';
			(void) strcpy (fn+1, rfile_name);
			/* DO NOT record rfile_name for removal. */
		}
		else
		{
			/* Build temp filename and open the file. */
		        extern char *get_960_tools_temp_dir();
		        char	*mktemp_ret;

			q = get_960_tools_temp_dir();
			if (q != NULL && *q != '\0')
				tmp = copy_string(q);
			else
				tmp = copy_string(GNUTMP_DFLT);

			/* '+ 3' for leading '@', for trailing nul, and
			 * for potential additional slash.
			 */
			fn =(char *) xmalloc (strlen(tmp)
						+ strlen(TMPNAME)
						+ strlen(TEMP_EXT)
						+ 3);
			fn[0] = '@';
			strcpy (fn+1, tmp);
			path_concat(fn+1, fn+1, TMPNAME);
			if (fn[1] == '/')
				fn[1] = '\\';
			mktemp_ret = mktemp (fn+1);
			if (!mktemp_ret || *mktemp_ret == '\0') {
				fprintf(stderr,"PROGRAM INVOCATION ERROR -- unable to obtain a temporary response file name\n");
				exit(1);
			}
			(void) strcat(fn, TEMP_EXT);
			remove_file_name = fn+1;
		}

		if ((fd = fopen(fn+1,"w")) == NULL) {
			fprintf(stderr,"PROGRAM INVOCATION ERROR -- Cannot create response file %s\n",fn+1);
			exit(1);
		}

		for (i=1; argv[i]; i++)
		{
			int	has_white_space = 0;
			char	*chp;
			for (chp = argv[i]; *chp; chp++) {
				if (isspace(*chp)) {
					has_white_space = 1;
					break;
				}
			}

			/* Write argv to the file, adding surrounding quotes
			 * if needed, and escaping double quotes.
			 */
			if (has_white_space)
				fprintf(fd,"\"");
			for (chp = argv[i]; *chp; chp++) {
				fprintf(fd, "%c", *chp);
				if (*chp == '"')
					fprintf(fd,"\"");
			}

			if (has_white_space)
				fprintf(fd,"\"");
			fprintf(fd,"\n");
		}
		fclose(fd);

		argv[1]= fn;
		argv[2]= NULL;
		if ((strlen(argv[0]) + strlen(argv[1]) + 1) > MAXCHARS)
		{
			fprintf(stderr,"PROGRAM INVOCATION ERROR -- command line too long: %s %s\n", argv[0], argv[1]);
			exit(1);
		}

		if (tmp)
			free(tmp);
	}
	return argv;
}

char **
check_dos_args(argv)
char **argv;
{
	return check_dos_args_with_name(argv, (char*)NULL);
}

void
delete_response_file ()
{
	if (remove_file_name) {
		remove(remove_file_name);
		free(remove_file_name - 1);
		remove_file_name = 0;
	}
}


#ifdef __HIGHC__
/* _fullpath (_fullpat.c) - Convert absolute path name to relative path name
 * This is equivalent to CodeBuilder's _fullpath, and added here for
 * compilers that don't supply it (such as Metaware C)
 */

#define ISSLASH(_c)  ((_c == '\\') || (_c == '/'))

char *_fullpath ( char *path, const char *rel, size_t maxlen )
   {
   char *p;
   const char *tps;
   char *tpd;
   int drive;

   /* - - - - - - - -  Allocate path buffer if not provided - - - - - - - -  */
   if ( path == NULL )                   /* Check if path buffer is provided */
      {                              /* Allocate path buffer if not provided */
      if ( (path = (char *) malloc ( (size_t) _MAX_PATH )) == NULL )
         return ( NULL );          /* Return NULL if error allocating buffer */

      maxlen = _MAX_PATH;    /* Ignore maxlen parameter, use new buffer size */
      }
   *path = 0;                                           /* Clear path buffer */


   /* - - - - - - - - - -  Assemble relative path name - - - - - - - - - - - */
   if ( (rel != NULL) && (*(rel+1) == ':') )     /* Check if drive specified */
      {
      tps = rel + 2;                 /* Source pointer skips drive and colon */
      if ( !ISSLASH(*tps) )
         {
         drive = toupper(*rel) - 'A' + 1;    /* Get drive from relative path */
         if ( (void *) _getcwd ( path, _MAX_PATH ) == NULL )  /* cwd */
            {
            *path = 0;
            return ( NULL );                            /* Error getting cwd */
            }

         if ( !ISSLASH(*tps) )                    /* Check if slash required */
            if ( strlen ( path ) > 3 )    /* If equal to 3, no slash ('C:\') */
               strcat ( path, "\\" );    /* Add leading slash if not present */
         }
      else                        /* Cwd not required. Path relative to root */
         tps -= 2;             /* Source pointer points to actual rel string */
      }

   else                               /* No drive specified in relative path */
      {
      if ( (rel == NULL) || !ISSLASH(*rel) )     /* Check if cwd is required */
         {
         if ( (void *) _getcwd ( path, _MAX_PATH ) == NULL )            {
            *path = 0;
            return ( NULL );                            /* Error getting cwd */
            }

         if ( (rel != NULL) && !ISSLASH(*rel) )   /* Check if slash required */
            if ( strlen ( path ) > 3 )    /* If equal to 3, no slash ('C:\') */
               strcat ( path, "\\" );    /* Add leading slash if not present */
         }
      else
         {                        /* Cwd not required. Path relative to root */
         *path = drive - 1 + 'A';              /* Copy default drive to path */
         *(path+1) = ':';                    /* Add colon to drive specifier */
         *(path+2) = 0;                          /* Terminate path with null */
         }

      tps = rel;              /* Source pointer points to relative path name */
      }

   p = path;
   while ( *p++ );
   p--;

   if ( (tps != NULL) && (*tps != 0) )   /* Check if relative path specified */
      {
      if ( ( strlen ( path ) + strlen ( tps ) ) >= maxlen )  /* Check length */
         {
         *path = 0;
         return ( NULL );                         /* Path exceeds max length */
         }
      else {
         tpd = path;                      /* Length ok. Append relative path */
         if ( *tpd )                         /* If absolute path is not NULL */
            while ( *++tpd );             /* Advance to end of absolute path */
         while ( *tps ) {           /* Append relative path to absolute path */
            if ( *tps == '/' )
               *tpd++ = '\\';      /* Change forward slash to backward slash */
            else
               *tpd++ = *tps;
            tps++;
            }
         *tpd = 0;                              /* Terminate appended string */
         }
      }


   /* - - - - - -  Convert relative path to absolute path name - - - - - - - */
   while ( *p )                                       /* Until end-of-string */
      {
      /*
      printf ( "            Relative Path : '%s'\n", path );
      printf ( "                             " );
      for ( i=0 ; i < (p-path) ; i++ )
         printf (" ");
      printf ( "^ (%d)\n", (p-path) );
      */

      if ( *p++ == '.' )     /* Current or parent directory designator found */
         {
         if ( *p == '.' )           /* Check for parent directory designator */
            {
            tps = p + 1;        /* Source pointer is first character past .. */
            p -= 2;      /* Move search pointer to first character before .. */

            p--;                                /* Step backwards over slash */
            while ( !ISSLASH(*p) )        /* Search backwards for next slash */
               if ( *p-- == ':' )                    /* Check if colon found */
                  {
                  *path = 0;
                  return ( NULL );                           /* Invalid path */
                  }
            }

         else                 /* Possible current directory designator ('.') */
            {
            if ( (*(p-2) == ':') || ISSLASH(*(p-2)) ) /* : or \ must precede */
               {
               tps = p;          /* Source pointer is first character past . */
               p -= 2;                              /* Adjust search pointer */
               }
            else
               return path;    /* Filename/extension delimiter. Parsing done */
            }

         strcpy ( p, tps );      /* Delete subpath, 'C:\D1\..\D2' -> 'C:\D2' */
         }
      }

   if ( (strlen(path) == 2) && (*(path+1) == ':') )
      strcat ( path, "\\" );                        /* Convert 'C:' to 'C:\" */

   else if ( (strlen(path) != 3) && ISSLASH(*(path+strlen(path)-1)) )
      *(path+strlen(path)-1) = 0;           /* Convert 'C:\dir\' to 'C:\dir' */

   return path;                    /* Return pointer to absolute path buffer */
   }

#endif

