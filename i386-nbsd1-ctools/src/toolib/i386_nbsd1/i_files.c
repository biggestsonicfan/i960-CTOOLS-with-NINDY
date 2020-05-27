/******************************************************************/
/*       Copyright (c) 1994 Intel Corporation

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

   Intel Corporation provides this AS IS, without any warranty,
   including the warranty of merchantability or fitness for a
   particular purpose, and makes no guarantee or representations
   regarding the use of, or the results of the use
   of, the software and documentation in terms of correctness,
   accuracy, reliability, currentness, or otherwise; and you rely
   on the software, documentation and results solely at your own
   risk.                                                          */
/******************************************************************/

#include <ctype.h>
#include <string.h>
#include "i_toolib.h"

char*
base_name_ptr(f)
char*	f;
{
	char*	ret;

	if (f == NULL)
		return NULL;

#if defined(DOS)
	if (*f && f[1] == ':')
		f += 2;
#endif
	for (ret = f; *f; f++)
		if (*f == '/'
#if defined(DOS)
			|| *f == '\\'
#endif
		   )
			ret = f+1;
	return ret;
}

void
path_concat(buf, p1, p2)
char*	buf;
char*	p1;
char*	p2;
{
	if (!buf) return;

	if (p1 != buf)
	{
		if (p1)
			(void) strcpy(buf, p1);
		else
			buf[0] = '\0';
	}

	if (!p2) return;

	/* Append a slash or backslash, but only if needed. */
	/* On DOS, DO NOT add a backslash after "c:", since */
	/* "c:\foo" is distinctly different than "c:foo".   */

	if (buf[0] && p2[0] != '/'
#if defined(DOS)
		&& p2[0] != '\\'
#endif
	   )
	{
		char	end = buf[strlen(buf) - 1];
#if defined(DOS)
		if (end != '/' && end != '\\' &&
				!(end == ':' && strlen(buf) == 2))
			(void) strcat(buf, "\\");
#else
		if (end != '/')
			(void) strcat(buf, "/");
#endif
	}

	(void) strcat(buf, p2);
}

#if defined(DOS)

char*
normalize_file_name(char* name)
{
	char* p = name;
	if (p != NULL)
		for (; *p; p++)
			*p = tolower(*p);
	return name;
}

/* Compare two file names, ignoring case and treating slash == backslash */

int
is_same_file_by_name(f1, f2)
char	*f1;
char	*f2;
{
	while (*f1)
	{
		if (tolower(*f1) != tolower(*f2))
		{
			/* slash matches backslash */
			if (! ((*f1 == '/' && *f2 == '\\') ||
			       (*f1 == '\\' && *f2 == '/')))
				return 0;
		}
		f1++; f2++;
	}

	return !(*f2);
}

#endif /* DOS */
