
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

#include <sysdep.h>

/* Returns 1, if and only if name is a directory. */

static int Is_a_dir(name)
    char *name;
{
    struct stat stat_struct;

    if ((stat(name,&stat_struct) != -1) &&
	((stat_struct.st_mode & S_IFMT) == S_IFDIR))
	    return 1;
    else
	    return 0;
}

/* Returns the standard 960tools temporary directory.  All 960tools
   use this entry point when they need to write a scratch file to
   a temp directory.  This will always return a directory with exactly
   one trailing '/' (on unix) or '\' on dos.  For example:

   /tmp/
   c:\tmp\

   This will be appropriate for direct catenation of a temporary filename
   without dealing with whether the last char is a '/' or '\'.

   paul reger
   Thu Apr 27 08:59:10 PDT 1995
*/

char *get_960_tools_temp_dir()
{
    static char *r = NULL;

    /* We've already found out the name of it, let's return it: */
    if (r)
	    return r;

    if ((r=getenv("TMPDIR")) && (Is_a_dir(r)))
	    ;
    else if ((r=getenv("G960TMP")) && (Is_a_dir(r)))
	    ;
    else if ((r=getenv("TEMP")) && (Is_a_dir(r)))
	    ;
    else if ((r=getenv("TMP"))  && (Is_a_dir(r)))
	    ;
    else {
#ifdef DOS

#define SLASHS "\\"
#define IS_SLASH(C) (((C) == '\\') || ((C) == '/'))

	r = ".\\";
#else /* we are on unix. */

#define SLASHS "/"
#define IS_SLASH(C) ((C) == '/')

	if (Is_a_dir("/tmp"))
		r = "/tmp/";
	else if (Is_a_dir("/usr/tmp"))
		r ="/usr/tmp/";
	else
		r = "./";
#endif
        return r;
    }

    /* The following code makes sure there is exactly one trailing '/'
       (on unix) or '\' on dos. */

    if (1) {
	char *ret_val = malloc(strlen(r) + 2);  /* +2 because trailing
						   NULL + possibly
						   adding a slash as
						   last char. */
	int i;

	if (!ret_val)  /* Out of memory.  Bug out. */
		return r;

	strcpy(ret_val,r);

	/* If the last character is a SLASH character then... */
	if (IS_SLASH(ret_val[strlen(ret_val)-1])) {

	    /* Let's make sure there is exactly one slash. */
	    for (i=strlen(ret_val)-1;i >= 0 && IS_SLASH(ret_val[i]);i--)
		    continue;
	    i += 2;
	    ret_val[i] = 0;
	    return r = ret_val;
	}
	else {
#ifdef DOS
	    /* For DOS, add a slash if and only if the last character is not a ':'. */
	    if (ret_val[strlen(ret_val)-1] != ':')
#endif
		    /* The last character was not a slash character, so we add one here: */
		    strcat(ret_val,SLASHS);
	    return r = ret_val;
	}
    }
}

#ifdef DOS

/* This code translates an unsigned long into 6 characters
   of base 37 digits: 0,1,2, ... 9, A,B,C ... Z, _

   The most significant digit is not used. */

static
char *Ul_As_Base26(ul,template,exes_index)
    unsigned long ul;
    char *template;
    int exes_index;
{
    static char rbuff[13];      /* Template must be 8x3, therefore will
				   never be longer than 12 chars (13
				   plus null). */
    int i;

    strcpy(rbuff,template);
    for(i=exes_index;i < exes_index+6;i++) {
	unsigned long q = ul / 37;
	unsigned long r = ul % 37;

	ul = q;
	if (r <= 9)
		rbuff[i] = '0' + r;
	else if (r <= 35)
		rbuff[i] = 'A' + (r-10);
	else
		rbuff[i] = '_';
    }
    return rbuff;
}
#endif

/* Create a unique temporary filename, of the form:

   {temp_dir}{filename}

   - {temp_dir} from get_960_tools_temp_dir()
   - {filename} is formed from a template.

   The template:

   - Must contain the string 'XXXXXX' somewhere in it
     (that is exactly 6 upper case exes).
   - Must be a valid filename on any host, therefore it:
     - Must be 8x3.
     - Must not have slashes or, otherwise illegal filename characters
       in it.

   Examples of good templates:

   "ASXXXXXX.lst" "LDXXXXXX.o" "CCXXXXXX.s" "CCXXXXXX.i"

   The returned filename is placed on the heap via the guarded_malloc()
   function passed as a parameter.

   For example, get_960_tools_temp_file("ASXXXXXX.lst",my_malloc) might
   return:

   /tmp/AS123456.lst (on unix)
   c:\temp\AS123456.lst (on dos)

 */

char *get_960_tools_temp_file(template,guarded_malloc)
    char *template,*(*guarded_malloc)();
{
    char *temp_dir = get_960_tools_temp_dir();
    int length_of_temp_file_name = strlen(temp_dir) + strlen(template) + 1;
    char *temp_file_name = guarded_malloc(length_of_temp_file_name);
    char *exes_string = strstr(template,"XXXXXX");

    if (!exes_string || strlen(template) > 12)
	    abort();

    if (1) {
#ifdef DOS
	int ctr = time(0);

	/* This will loop until we find the name of a file that does not
	   exist. */
	for (;;ctr++) {
	    FILE *f;

	    strcpy(temp_file_name,temp_dir);
	    strcat(temp_file_name,Ul_As_Base26((unsigned long)ctr,template,exes_string-template));

	    if (f=fopen(temp_file_name,"r"))
		    fclose(f);
	    else
		    return temp_file_name;
	}
#else  /* We are on UNIX. */
	/* Here, we use mktemp() allowing for the optional
	extension passed into get_960_tools_temp_file() */

	extern char *mktemp();
	char *ext = strrchr(template,'.');

	strcpy(temp_file_name,temp_dir);
	strcat(temp_file_name,template);
	/* Make sure any extension is NULLed prior to call to mktemp(): */
	temp_file_name[strlen(temp_dir)+(exes_string-template)+6] = 0;
	mktemp(temp_file_name);
	strcat(temp_file_name,exes_string+6);
	return temp_file_name;
#endif
    }
}

#ifdef TEST

main() {
    extern char * malloc();
    char *t1 = get_960_tools_temp_file("ASXXXXXX.s",malloc),
    *t2 = get_960_tools_temp_file("ASXXXXXX.o",malloc),
    *t3 = get_960_tools_temp_file("ASXXXXXX",malloc);
    printf("%s %s %s\n",t1,t2,t3);
}

#endif
