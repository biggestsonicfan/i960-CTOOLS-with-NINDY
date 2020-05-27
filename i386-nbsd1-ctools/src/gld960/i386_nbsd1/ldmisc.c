/*
 * Copyright (C) 1991 Free Software Foundation, Inc.
 *
 * This file is part of GLD, the Gnu Linker.
 *
 * GLD is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * GLD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GLD; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* $Id: ldmisc.c,v 1.35 1995/10/13 20:41:27 paulr Exp $ */

#include "sysdep.h"
#include "bfd.h"
#include "ld.h"
#include "ldmisc.h"
#include "ldlang.h"
#include "ldlex.h"

#if	defined(__STDC__)
#include <stdarg.h>
#else
#include <varargs.h>
#endif

extern char *program_name;
extern ld_config_type config;

#ifndef errno
#ifndef __HIGHC__
extern int errno;
#endif
#endif

void
yyerror(arg) 
char *arg;
{
	info("%P%F: %S %s\n",arg);
}

/* Define malloc() and exit() for dec3100 systems... */

#if HOST_SYS == DEC3100_SYS
#	include <stdlib.h>
#endif


/*
 * %F error is fatal
 * %P print progam name
 * %S print script file and linenumber
 * %E current bfd error or errno
 * %I filename from a lang_input_statement_type
 * %B filename from a bfd
 * %t embellished symbol table entry
 * %T symbol table entry
 * %X no object output, fail return
 * %V hex bfd_vma
 * %C Clever filename:linenumber 
 * %
 */
#if	defined(__STDC__)
void
info(char *the_fmt, ...)
#else
void
info(va_alist)
    va_dcl
#endif
{
	va_list arg;
	char *fmt;
	boolean fatal = false;
	asymbol *symbol;
	bfd *abfd;
	lang_input_statement_type *is;

#if	defined(__STDC__)
	va_start(arg, the_fmt);
	fmt = the_fmt;
#else
	va_start(arg);
	fmt = va_arg(arg, char *);
#endif
	while (*fmt) {
		for ( ; *fmt != '%' && *fmt != '\0'; fmt++ ) {
			fputc(*fmt, stderr);
		}
		if (*fmt == '\0') {
			break;
		}

		fmt++;
		switch (*fmt++) {
		case 'B':
			abfd = va_arg(arg, bfd *);
			if (abfd->my_archive) {
				fprintf(stderr,"%s(%s)", abfd->my_archive->filename,
								abfd->filename);
			} else {
				fprintf(stderr,"%s", abfd->filename);
			}
			break;
		case 'C':
		        {
				CONST char *filename;
				CONST char *functionname;
				unsigned int lineno;
				bfd *abfd = va_arg(arg, bfd *);
				asection *section = va_arg(arg, asection *);
				asymbol **symbols = va_arg(arg, asymbol **);
				bfd_vma offset = va_arg(arg, bfd_vma);

				info("%B",abfd);

				/* Don't try to fetch the line info from the "script file". */
				if (strcmp(abfd->filename,"script file") &&

				    bfd_find_nearest_line(abfd,
							  section,
							  symbols,
							  offset,
							  &filename,
							  &functionname,
							  &lineno))
				    {
					if (filename && functionname)
						fprintf(stderr," %s:%u: (%s)", filename,
							lineno, functionname);
				    }
			}
			break;
		case 'd':
			fprintf(stderr,"%d", va_arg(arg, int));
			break;
		case 'E':
			/* Replace with the most recent errno explanation */
			fputs(bfd_errmsg(bfd_error),stderr);
			break;
		case 'f':
		    {
			double d = va_arg(arg,double);
			fprintf(stderr,"%f",d);
		    }
			break;
		case 'F':
			fatal = true;
			break;
		case 'I':
			is = va_arg(arg,lang_input_statement_type *);
			fprintf(stderr,"%s", is->local_sym_name);
			break;
		case 'P':
			fprintf(stderr,"%s", program_name);
			break;
		case 'S':
			lex_err();
			break;
		case 's':
			fprintf(stderr,"%s", va_arg(arg, char *));
			break;
		case 't':
			symbol = va_arg(arg, asymbol *);
			info("%T from input file: %B ",symbol,symbol->the_bfd);
			if (symbol) {
			    asection *section = symbol->section;
			    if (section)
				    fprintf(stderr,"symbol value: 0x%08x",symbol->value +
					    section->output_offset +
					    (section->output_section ? section->output_section->vma : 0));
			    if (symbol->flags & BSF_HAS_SCALL) {
				more_symbol_info symmi;
				bfd_more_symbol_info(symbol->the_bfd,symbol,&symmi,bfd_sysproc_val);
				fprintf(stderr,"system call value: %d",symmi.sysproc_value);
			    }
			}
			break;
		case 'T':
			symbol = va_arg(arg, asymbol *);
			if (symbol) {
				asection *section = symbol->section;
				fprintf(stderr,"%s", symbol->name);
				if ((symbol->flags & BSF_UNDEFINED) == 0) {
					fprintf(stderr," (%s)", section ?
						section->name : 
						((symbol->flags & BSF_HAS_SCALL) ? "system call" :
						 "absolute"));
				}
			} else {
				fputs("no symbol",stderr);
			}
			break;
		case 'V':
			{
				bfd_vma value = va_arg(arg, bfd_vma);
				fprintf_vma(stderr,value);
			}
			break;
		case 'X':
			config.make_executable = false;
			break;
		case 'x':
			fprintf(stderr,"0x%x", va_arg(arg, int));
			break;
		default:
			fprintf(stderr,"%s", va_arg(arg, char *));
			break;
		}
	}
	if (fatal) {
	    extern char *output_filename;
	    extern int rm_output_filename;

	    if (rm_output_filename)
		    unlink(output_filename);
	    info("%P: Fatal error.  Output file: %s was not created.\n",
		 output_filename?output_filename:"unknown");
	    gnu960_remove_tmps();
	    exit(1);
	}
	va_end(arg);
}


void 
info_assert(file, line)
char *file;
unsigned int line;
{
	info("%F%P internal error %s %d\n", file,line);
}

/* Return a newly-allocated string
 * whose contents concatenate those of S1, S2, S3.
 */
char *
DEFUN(concat, (s1, s2, s3),
    CONST char *s1 AND
    CONST char *s2 AND
    CONST char *s3)
{
	bfd_size_type len1 = (s1 && *s1) ? strlen (s1) : 0;
	bfd_size_type len2 = (s2 && *s2) ? strlen (s2) : 0;
	bfd_size_type len3 = (s3 && *s3) ? strlen (s3) : 0;
	char *result = ldmalloc (len1 + len2 + len3 + 1);

	if (len1 != 0) {
		memcpy(result, s1, len1);
	}
	if (len2 != 0) {
		memcpy(result+len1, s2, len2);
	}
	if (len3 != 0) {
		memcpy(result+len1+len2, s3, len3);
	}
	*(result + len1 + len2 + len3) = 0;
	return result;
}

#ifdef DEBUGHEAP

#define MAX_HEAP_HASH 256
static struct heap_node_struct {
    char * malloc_key;    /* Contains the value that WE RETURN from malloc and realloc(). */
    unsigned size;
    struct heap_node_struct *next_struct;
} *heap_hash_table[MAX_HEAP_HASH];

#define HASH_INDEX(x) ((((int)x)>>4)&(MAX_HEAP_HASH-1))

#define MAGIC_HEADER   "Eric"
#define MAGIC_LENGTH   4
#define HAS_MAGIC_HEADER(X) (!strncmp(MAGIC_HEADER,X->malloc_key-MAGIC_LENGTH,MAGIC_LENGTH))
#define MAGIC_TRAILER  "Paul"
#define HAS_MAGIC_TRAILER(X) (!strncmp(MAGIC_TRAILER,X->malloc_key+X->size,MAGIC_LENGTH))

static putinhash(key,size)
    char *key;
    unsigned size;
{
    struct heap_node_struct **p = &heap_hash_table[HASH_INDEX(key)];

    while (*p)
	    p = &((*p)->next_struct);
    (*p) = (struct heap_node_struct *) (malloc)(sizeof(struct heap_node_struct));
    (*p)->malloc_key = key;
    (*p)->size = size;
    (*p)->next_struct = 0;
    strncpy(key-4,MAGIC_HEADER,MAGIC_LENGTH);
    strncpy(key+size,MAGIC_TRAILER,MAGIC_LENGTH);
}

static int removefromhash(key,destroy)
    char *key;
    int destroy;
{
    struct heap_node_struct *q = 0,*p = heap_hash_table[HASH_INDEX(key)];

    while (p && p->malloc_key != key) {
	q = p;
	p = p->next_struct;
    }
    if (!p)
	    return 0;
    if (!q)
	    /* We remove the root of this heap hash entry. */
	    heap_hash_table[HASH_INDEX(key)] = p->next_struct;
    else
	    q->next_struct = p->next_struct;
    if (destroy)
	    memset(p->malloc_key-MAGIC_LENGTH,-1,2*MAGIC_LENGTH+p->size);
    (free)(p);
    return 1;
}

static checkhash()
{
    int i;

    for (i=0;i < MAX_HEAP_HASH;i++) {
	struct heap_node_struct *p = heap_hash_table[i];

	while (p) {
	    if (!HAS_MAGIC_HEADER(p))
		    fprintf(stderr,"HEADER OVERRAN: 0x%x\n",p);
	    if (!HAS_MAGIC_TRAILER(p))
		    fprintf(stderr,"TRAILER OVERRAN: 0x%x\n",p);
	    p = p->next_struct;
	}
    }
    fflush(stderr);
}
#endif

PTR
DEFUN(ldmalloc, (size),
bfd_size_type size)

{
#ifndef DEBUGHEAP
	PTR result =  (malloc) ((int)size);

	if (result == (char *)NULL && size != 0)
		info("%F%P virtual memory exhausted\n");

	return result;
#else
	PTR result =  (malloc) ((int)size + MAGIC_LENGTH + MAGIC_LENGTH );

	if (result == (char *)NULL && size != 0)
		info("%F%P virtual memory exhausted\n");

	checkhash();
	putinhash(((char *)result)+MAGIC_LENGTH,size);
	return (PTR) (((char*)result) + MAGIC_LENGTH);
#endif
}

PTR
DEFUN(ldrealloc, (ptr,size),
PTR ptr AND
bfd_size_type size)
{
#ifndef DEBUGHEAP
    PTR result =  (realloc) (ptr,(int)size);

    if (result == (char *)NULL && size != 0)
	    info("%F%P virtual memory exhausted\n");

    return result;
#else
    PTR result =  (realloc) (((char*)ptr) - MAGIC_LENGTH,(int)size + MAGIC_LENGTH + MAGIC_LENGTH);

    if (result == (char *)NULL && size != 0)
	    info("%F%P virtual memory exhausted\n");

    checkhash();
    if (!removefromhash(((char*)result)-MAGIC_LENGTH,0))
	    fprintf(stderr,"ldrealloc(): ATTEMPT TO FREE NON MALLOC'd memory: 0x%x\n",ptr);
    fflush(stderr);
    putinhash(((char *)result)+MAGIC_LENGTH,size);
    return (PTR) (((char*)result)+MAGIC_LENGTH);
#endif
}

void
ldfree(ptr)
PTR ptr;
{
#ifndef DEBUGHEAP
    (free)(ptr);
#else
    if (!removefromhash(ptr,1))
	    fprintf(stderr,"ldfree(): ATTEMPT TO FREE NON MALLOC'd memory: 0x%x\n",ptr);
    fflush(stderr);
    (free)(((char*)ptr)-MAGIC_LENGTH);
#endif
}

char *DEFUN(buystring,(x),
CONST char *CONST x)
{
	bfd_size_type  l = strlen(x)+1;
	char *r = ldmalloc(l);
	memcpy(r, x,l);
	return r;
}
