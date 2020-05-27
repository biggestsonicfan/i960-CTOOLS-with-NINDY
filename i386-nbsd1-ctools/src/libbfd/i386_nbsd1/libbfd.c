/* Copyright (C) 1990, 1991 Free Software Foundation, Inc.
 *
 * This file is part of BFD, the Binary File Diddler.
 *
 * BFD is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * BFD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with BFD; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*** libbfd.c -- random bfd support routines used internally only. */
#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"

#ifdef DOS
#	include <malloc.h>
#endif


/* DUMMIES
 *	for targets that don't want or need to implement certain operations
 */

boolean
DEFUN(_bfd_dummy_new_section_hook,(ignore, ignore_newsect),
    bfd *ignore AND
    asection *ignore_newsect)
{
	return true;
}

boolean
DEFUN(bfd_false ,(ignore),
    bfd *ignore)
{
	return false;
}

boolean
DEFUN(bfd_true,(ignore),
    bfd *ignore)
{
	return true;
}

bfd_target *
DEFUN(_bfd_dummy_target,(ignore_abfd),
    bfd *ignore_abfd)
{
	return 0;
}

#if 0

PTR
DEFUN(bfd_nullvoidptr,(ignore),
    bfd *ignore)
{
	return (PTR)NULL;
}

int 
DEFUN(bfd_0,(ignore),
    bfd *ignore)
{
	return 0;
}

unsigned int 
DEFUN(bfd_0u,(ignore),
    bfd *ignore)
{
	return 0;
}

void 
DEFUN(bfd_void,(ignore),
    bfd *ignore)
{
}

boolean
DEFUN(_bfd_dummy_core_file_matches_executable_p,(ignore_core_bfd, ignore_exec_bfd),
    bfd *ignore_core_bfd AND
    bfd *ignore_exec_bfd)
{
	bfd_error = invalid_operation;
	return false;
}


char *
DEFUN(_bfd_dummy_core_file_failing_command,(ignore_abfd),
    bfd *ignore_abfd)
{
	return (char *)NULL;
}

int
DEFUN(_bfd_dummy_core_file_failing_signal,(ignore_abfd),
    bfd *ignore_abfd)
{
	return 0;
}


/* MEMORY ALLOCATION */

DEFUN(PTR bfd_alloc_by_size_t,(abfd, size),
    bfd *abfd AND
    size_t size)
{
	PTR res = obstack_alloc(&(abfd->memory), size);
	return res;
}

DEFUN(void bfd_alloc_grow,(abfd, ptr, size),
    bfd *abfd AND
    PTR ptr AND
    bfd_size_type size)
{
	obstack_grow(&(abfd->memory), ptr, size);
}

DEFUN(PTR bfd_alloc_finish,(abfd),
    bfd *abfd)
{
	return obstack_finish(&(abfd->memory));
}

#endif

DEFUN(PTR __bfd_alloc, (abfd, size, file, line),
      bfd *abfd          AND
      bfd_size_type size AND
      char *file         AND
      int line)
{
    PTR p;

    if (size > 0)
	    p = (PTR) malloc(size);
    else
	    p = NULL;

    if (p || size == 0)
	    return p;
    fprintf(stderr,"bfd memory allocation failure: (%s:%d)\n",file,line);
    exit (1);
}

DEFUN(PTR __bfd_zalloc,(abfd, size, file, line),
      bfd *abfd          AND
      bfd_size_type size AND
      char *file         AND
      int line)
{
    if (size > 0) {
	PTR res = __bfd_alloc(abfd, size, file, line);
	memset(res, 0, (size_t)size);
	return res;
    }
    else
	    return NULL;
}

DEFUN(PTR __bfd_realloc,(abfd, old, size, file, line),
      bfd *abfd          AND
      PTR old            AND
      bfd_size_type size AND
      char *file         AND
      int line)
{
    if (size > 0) {
	PTR p = (PTR) realloc(old, size);
	if (p)
		return p;
	fprintf(stderr,"bfd memory allocation failure: (%s:%d)\n",file,line);
	exit(1);
    }
    else {
	free(old);
	return NULL;
    }
}

DEFUN(void __bfd_release,(abfd, ptr, file, line),
    bfd *abfd  AND
    PTR ptr    AND
    char *file AND
    int line)
{
    if (ptr == NULL) {
	fprintf(stderr,"warning: bfd attempted to free address 0 (%s:%d)\n",file,line);
	return;
    }
    free(ptr);
}

#if 0

DEFUN(bfd_size_type bfd_alloc_size,(abfd),
    bfd *abfd)
{
	struct _obstack_chunk *chunk = abfd->memory.chunk;
	size_t size = 0;
	while (chunk) {
		size += chunk->limit - &(chunk->contents[0]);
		chunk = chunk->prev;
	}
	return size;
}
#endif

/* Some IO code */


/* Note that archive entries don't have streams; they share their parent's.
 * This allows someone to play with the iostream behind bfd's back.
 *
 * Also, note that the origin pointer points to the beginning of a file's
 * contents (0 for non-archive elements).  For archive entries this is the
 * first octet in the file, NOT the beginning of the archive header.
 */

static 
int DEFUN(real_read,(where, a,b, file),
    PTR where AND
    int a AND
    int b AND
    FILE *file)
{
	return fread(where, a,b,file);
}

bfd_size_type
DEFUN(bfd_read,(ptr, size, nitems, abfd),
    PTR ptr AND
    bfd_size_type size AND
    bfd_size_type nitems AND
    bfd *abfd)
{
	return (bfd_size_type)
		real_read(ptr,1,(int)(size*nitems),bfd_cache_lookup(abfd));
}

bfd_size_type
DEFUN(bfd_write,(ptr, size, nitems, abfd),
    PTR ptr AND
    bfd_size_type size AND
    bfd_size_type nitems AND
    bfd *abfd)
{
	return fwrite (ptr, 1, (int)(size*nitems), bfd_cache_lookup(abfd));
}

int
DEFUN(bfd_seek,(abfd, position, direction),
    bfd * CONST abfd AND
    file_ptr position AND
    CONST int direction)
{
	/* For the time being, a bfd may not seek to it's end.  The
	 * problem is that we don't easily have a way to recognize
	 * the end of an element in an archive.
	 */

	BFD_ASSERT(direction == SEEK_SET || direction == SEEK_CUR);

	if (direction == SEEK_SET && abfd->my_archive) {
		/* This is a set within an archive, so we need to
		 * add the base of the object within the archive
		 */
		position += abfd->origin;
	}
	return(fseek(bfd_cache_lookup(abfd), position, direction));
}

long
DEFUN(bfd_tell,(abfd),
    bfd *abfd)
{
	file_ptr ptr;

	ptr = ftell (bfd_cache_lookup(abfd));

	if (abfd->my_archive){
		ptr -= abfd->origin;
	}
	return ptr;
}

/* Make a string table */

#if 0
/* Add string to table pointed to by table, at location starting with free_ptr.
 * resizes the table if necessary (if it's NULL, creates it, ignoring
 * table_length).  Updates free_ptr, table, table_length
 */
boolean
DEFUN(bfd_add_to_string_table,(table, new_string, table_length, free_ptr),
    char **table AND
    char *new_string AND
    unsigned int *table_length AND
    char **free_ptr)
{
	size_t string_length = strlen (new_string) + 1; /* include null here */
	char *base = *table;
	size_t space_length = *table_length;
	unsigned int offset = (base ? *free_ptr - base : 0);

	if (base == NULL) {
		/* Avoid a useless regrow if we can (but of course we still
		 * take it next time)
		 */
		space_length = (string_length < DEFAULT_STRING_SPACE_SIZE ?
				DEFAULT_STRING_SPACE_SIZE : string_length+1);
		base = zalloc (space_length);
		if (base == NULL) {
			bfd_error = no_memory;
			return false;
		}
	}

	if ((size_t)(offset + string_length) >= space_length) {
		/* Make sure we will have enough space */
		while ((size_t)(offset + string_length) >= space_length) {
			space_length += space_length/2; /* grow by 50% */
		}
		base = (char *) bfd_realloc (abfd,base, space_length);
		if (base == NULL) {
			bfd_error = no_memory;
			return false;
		}

	}

	memcpy (base + offset, new_string, string_length);
	*table = base;
	*table_length = space_length;
	*free_ptr = base + offset + string_length;

	return true;
}
#endif


/** The do-it-yourself (byte) sex-change kit */

/* The middle letter e.g. get<b>short indicates Big or Little endian
 * target machine.  It doesn't matter what the byte order of the host
 * machine is; these routines work for either.
 */

unsigned int
DEFUN(_do_getb16,(addr),
    register bfd_byte *addr)
{
	return (addr[0] << 8) | addr[1];
}

unsigned int
DEFUN(_do_getl16,(addr),
    register bfd_byte *addr)
{
	return (addr[1] << 8) | addr[0];
}

void
DEFUN(_do_putb16,(data, addr),
    int data AND
    register bfd_byte *addr)
{
	addr[0] = (bfd_byte)(data >> 8);
	addr[1] = (bfd_byte )data;
}

void
DEFUN(_do_putl16,(data, addr),
    int data AND		
    register bfd_byte *addr)
{
	addr[0] = (bfd_byte )data;
	addr[1] = (bfd_byte)(data >> 8);
}

unsigned int
DEFUN(_do_getb32,(addr),
    register bfd_byte *addr)
{
	return ((((addr[0] << 8) | addr[1]) << 8) | addr[2]) << 8 | addr[3];
}

unsigned int
_do_getl32 (addr)
    register bfd_byte *addr;
{
	return ((((addr[3] << 8) | addr[2]) << 8) | addr[1]) << 8 | addr[0];
}

int
DEFUN(_do_getb64,(addr,val),
    register bfd_byte *addr AND
      bfd_64_type *val)
{
	BFD_FAIL();
}

int
DEFUN(_do_getl64,(addr,val),
    register bfd_byte *addr AND
      bfd_64_type *val)
{
	BFD_FAIL();
}

void
DEFUN(_do_putb32,(data, addr),
    unsigned long data AND
    register bfd_byte *addr)
{
	addr[0] = (bfd_byte)(data >> 24);
	addr[1] = (bfd_byte)(data >> 16);
	addr[2] = (bfd_byte)(data >>  8);
	addr[3] = (bfd_byte)data;
}

void
DEFUN(_do_putl32,(data, addr),
    unsigned long data AND
    register bfd_byte *addr)
{
	addr[0] = (bfd_byte)data;
	addr[1] = (bfd_byte)(data >>  8);
	addr[2] = (bfd_byte)(data >> 16);
	addr[3] = (bfd_byte)(data >> 24);
}
void
DEFUN(_do_putb64,(data, addr),
    bfd_64_type data AND
    register bfd_byte *addr)
{
	BFD_FAIL();
}

void
DEFUN(_do_putl64,(data, addr),
    bfd_64_type data AND
    register bfd_byte *addr)
{
	BFD_FAIL();
}

/* Default implementation */

boolean
DEFUN(bfd_generic_get_section_contents,(abfd,section,location,offset,count),
    bfd *abfd AND
    sec_ptr section AND
    PTR location AND
    file_ptr offset AND
    bfd_size_type count)
{
	if (count == 0) {
		return true;
	}
	if ((bfd_size_type)offset >= section->size
	||   bfd_seek(abfd,(file_ptr)(section->filepos+offset),SEEK_SET) == -1
	||   bfd_read(location,(bfd_size_type)1,count,abfd) != count) {
		return false;
	}
	return true;
}


void
_bfd_add_bfd_ghist_info(p,p_len,p_max,a,func,file,line)
    bfd_ghist_info **p;
    unsigned int *p_len,*p_max;
    unsigned int a;
    CONST char         *func;
    CONST char         *file;
    int           line;
{
    if ((*p_len)+1 >= (*p_max)) {
	(*p_max) += 100;
	if (!((*p) = (bfd_ghist_info *) bfd_realloc((bfd *)0,(*p),(*p_max)*sizeof(bfd_ghist_info)))) {
	    perror("libbfd.c");
	    exit(1);
	}
    }
    (*p)[(*p_len)].address = a;
    (*p)[(*p_len)].func_name = func;
    (*p)[(*p_len)].file_name = file;
    (*p)[(*p_len)].line_number = line;
    (*p)[(*p_len)].num_hits = 0;
    (*p_len)++;    
}

int _bfd_cmp_bfd_ghist_info(l,r)
    bfd_ghist_info *l,*r;
{
    if (l->address < r->address)
	    return -1;
    else if (l->address > r->address)
	    return 1;
    else {
	int s;
	/* Both addresses are the same, the result is sorted in the
	   following order:
	   1. with filename == unknown and with func name
	   2. with filename            and with func name
	   3. with filename            and without func name
	   4. without filename         and with funcname     (dk)
	   5. without filename         and without funcname  (dk)
           6. Sort by file name.
	   7. Sort by function name.
	   8. Sort by line_number.  */
	   
#define CHECK_NULLS(left,right) if (left == NULL && right != NULL) return 1; else if (left != NULL && right == NULL) return -1
	CHECK_NULLS(l->file_name,r->file_name);
	CHECK_NULLS(l->func_name,r->func_name);
	if (l->file_name && r->file_name && (s=strcmp(l->file_name,r->file_name)))
		return s;
	if (l->func_name && r->func_name && (s=strcmp(l->func_name,r->func_name)))
		return s;
	return l->line_number - r->line_number;
    }
}

char *_bfd_buystring(s)
    char *s;
{
    char *p = (char *) bfd_alloc((bfd *)0,strlen(s) + 1);
    strcpy(p,s);
    return p;
}

CONST char
*_bfd_trim_under_and_slashes(s,tu)
    CONST char *s;
    int tu;
{
#ifdef DOS
#define LOOKFOR "\\/:"
#else
#define LOOKFOR "/"
#endif
    char *t = NULL,*t1,*p;

    s = (tu && s && *s == '_') ? (s+1) : s;
    for(p=LOOKFOR;*p;p++)
	    if ((t1=strrchr(s,*p)) > t)
		    t = t1+1;
    return (t) ? t : s;
}

void
_bfd_remove_ghist_info_element(p,i,ip)
    bfd_ghist_info *p;
    unsigned int i,*ip;
{
    int j;

    (*ip)--;
    for (j=i;j < (*ip);j++) {
	p[j].address     = p[j+1].address;
	p[j].line_number = p[j+1].line_number;
	p[j].func_name   = p[j+1].func_name;
	p[j].file_name   = p[j+1].file_name;
    }
}

#define HASHED_STR_TAB_SIZE 199

hashed_str_tab *
DEFUN(_bfd_init_hashed_str_tab,(abfd,min_size),
      bfd *abfd AND
      int min_size)
{
    hashed_str_tab *p = (hashed_str_tab *) bfd_zalloc(abfd,sizeof(hashed_str_tab));
    p->buckets = (hashed_str_tab_bucket **)
	    bfd_zalloc(abfd,sizeof(hashed_str_tab_bucket *) * HASHED_STR_TAB_SIZE);
    p->current_size = 0;
    p->max_size = 100;
    p->strings = bfd_alloc(abfd,p->max_size);
    p->min_size = min_size;
    return p;
}

static int
hash_key(name)
    char *name;
{
    char *p = name;
    int hashkey = 0;

    for (;p && *p;p++)
	    hashkey += *p;
    hashkey = hashkey % HASHED_STR_TAB_SIZE;
    return hashkey;
}

static hashed_str_tab_bucket **lookfor(name,hst)
    char *name;
    hashed_str_tab *hst;
{
    unsigned long hashkey = hash_key(name);
    hashed_str_tab_bucket **q;

    q = &(hst->buckets[hashkey]);
    while (*q) {
	if (!strcmp(name,hst->strings+(*q)->offset)) {
	    /* The string is already in the string table. */
	    return q;
	}
	q = &((*q)->next_bucket);
    }
    return q;
}

void
DEFUN(_bfd_release_hashed_str_tab,(abfd,hst),
      bfd *abfd           AND
      hashed_str_tab *hst)
{
    int i;

    for (i=0;i < HASHED_STR_TAB_SIZE;i++) {
	hashed_str_tab_bucket *p = hst->buckets[i],*q;

	for (;p;p = q) {
	    q = p->next_bucket;
	    bfd_release(abfd,p);
	}
    }
    bfd_release(abfd,hst->strings);
    bfd_release(abfd,hst->buckets);
    bfd_release(abfd,hst);
}

int
DEFUN(_bfd_lookup_hashed_str_tab_offset,(abfd,hst,name,name_length),
      bfd *abfd           AND
      hashed_str_tab *hst AND
      CONST char *name    AND
      int name_length)
{
    int i = 0,rv;

    do {
	hashed_str_tab_bucket **q = lookfor(name+i,hst);

	if (!(*q)) {
	    /* The string was not in the hash table before, so we add it to the hash table: */
	    (*q) = (hashed_str_tab_bucket *) bfd_zalloc(abfd,sizeof(hashed_str_tab_bucket));
	    (*q)->offset = hst->current_size;
	    /* Now, add the string to the end of the long_string_table: */
	    if (i == 0) {
		int new_length = hst->current_size+name_length+1;

		rv = (*q)->offset;
		if (new_length >= hst->max_size)
			hst->strings = (char *) bfd_realloc(abfd,hst->strings,
							    hst->max_size = (hst->max_size*2 > new_length) ?
							    (hst->max_size*2) : new_length);
		strncpy(hst->strings+hst->current_size,name,name_length + 1);
		hst->current_size += name_length + 1;
	    }
	    else {
		(*q)->offset = rv + i;
	    }
	}
	else {
	    if (i == 0)
		    return (*q)->offset;
	    else
		    return rv;
	}
	i ++;
    } while (((abfd->flags & DO_NOT_STRIP_ORPHANED_TAGS) == 0) && name_length-i >= hst->min_size);
    return rv;
}
