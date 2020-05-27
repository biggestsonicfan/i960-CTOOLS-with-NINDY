/*
 * varargs for metaware compiler which only supports stdarg.h
 */
#ifndef _VARARG_H

#define _VARARG_H
#pragma push_align_members(64);

#define va_alist	_first, ...
#define va_dcl		int _first;
#define va_list		char *
#define va_start(ap)	(ap = (char *) &_first)
#define va_arg(ap,type)	(* (type *) ((ap += ((sizeof (type) + 3)&~3)) - \
					   ((sizeof (type) + 3)&~3)) )


#pragma pop_align_members();

#endif
