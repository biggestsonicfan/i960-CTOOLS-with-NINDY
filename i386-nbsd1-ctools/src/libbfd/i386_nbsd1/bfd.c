/*** bfd
 *	binary file diddling routines by Gumby Wallace of Cygnus Support.
 *	Every definition in this file should be exported and declared
 *	in bfd.h.  If you don't want it to be user-visible, put it in
 *	libbfd.c!
 */

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


#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"



/* Error handling
 *  o Most functions return nonzero on success (check doc for
 *	precise semantics); 0 or NULL on error.
 *  o Internal errors are documented by the value of bfd_error.
 *	If that is system_call_error then check errno.
 *  o The easiest way to report this to the user is to use bfd_perror.
 */

bfd_ec bfd_error = no_error;

char *bfd_errmsgs[] = {
	"No error",
	"System call error",
	"Invalid target",
	"File in wrong format",
	"Invalid operation",
	"Memory exhausted",
	"No symbols",
	"No relocation info",
	"No more archived files",
	"Malformed archive",
	"Symbol not found",
	"File format not recognized",
	"File format is ambiguous",
	"Section has no contents",
	"Nonrepresentable section on output",
	"#<Invalid error code>"
};

static
void
DEFUN(bfd_nonrepresentable_section,(abfd, name),
    CONST  bfd * CONST abfd AND
    CONST  char * CONST name)
{
	printf("bfd error writing file %s, format %s can't represent section %s\n",
	    abfd->filename,
	    abfd->xvec->name,
	    name);
	exit(1);
}


bfd_error_vector_type bfd_error_vector = {
	bfd_nonrepresentable_section
};

#if !defined(ANSI_LIBRARIES) && !defined(__STDC__) && HOST_SYS != GCC960_SYS
char *
strerror (code)
     int code;
{
	extern int sys_nerr;
	extern char *sys_errlist[];

	return (((code < 0) || (code >= sys_nerr)) ? "(unknown error)" :
						    sys_errlist [code]);
}
#endif /* not ANSI_LIBRARIES */


char *
bfd_errmsg (error_tag)
     bfd_ec error_tag;
{
#ifndef errno
#ifndef __HIGHC__
	extern int errno;
#endif
#endif
	if (error_tag == system_call_error)
		return strerror (errno);

	if ((((int)error_tag <(int) no_error)
	|| ((int)error_tag > (int)invalid_error_code))){
		error_tag = invalid_error_code;/* sanity check */
	}

	return bfd_errmsgs [(int)error_tag];
}


void
bfd_default_error_trap(error_tag)
    bfd_ec error_tag;
{
	  printf("bfd assert fail (%s)\n", bfd_errmsg(error_tag));
}

void (*bfd_error_trap)() = bfd_default_error_trap;
void (*bfd_error_nonrepresentabltrap)() = bfd_default_error_trap;

void
DEFUN(bfd_perror,(message),
      CONST char *message)
{
	if (bfd_error == system_call_error){
		perror((char *)message);	/* must be system error */
	} else if (message == NULL || *message == '\0'){
		fprintf (stderr, "%s\n", bfd_errmsg (bfd_error));
	} else {
		fprintf(stderr,"%s: %s\n",message,bfd_errmsg(bfd_error));
	}
}

/* for error messages */
char *
bfd_format_string (format)
     bfd_format format;
{
	if (((int)format <(int) bfd_unknown)
	|| ((int)format >=(int) bfd_type_end)){
		return "invalid";
	}

	switch (format) {
	case bfd_object:	return "object";	/* ld/as/cc output */
	case bfd_archive:	return "archive";	/* object archive */
	case bfd_core:		return "core";		/* core dump */
	default:		return "unknown";
	}
}

/** Target configurations */

extern bfd_target *target_vector[];

/* Returns a pointer to the transfer vector for the object target
 * named target_name.  If target_name is NULL, chooses the one in the
 * environment variable GNUTARGET; if that is null or not defined then
 * the first entry in the target list is chosen.  Passing in the
 * string "default" or setting the environment variable to "default"
 * will cause the first entry in the target list to be returned.
 */

bfd_target *
DEFUN(bfd_find_target,(target_name, abfd),
      CONST char *target_name AND
      bfd *abfd)
{
	bfd_target **target;
	extern char *getenv ();
	CONST char *targname = target_name ? target_name : getenv ("GNUTARGET");

	/* This is safe; the vector cannot be null */
	if (targname == NULL || !strcmp (targname, "default")) {
		return abfd->xvec = target_vector[0];
	}
	for (target = &target_vector[0]; *target; target++) {
		if (!strcmp (targname, (*target)->name))
			return abfd->xvec = *target;
	}
	bfd_error = invalid_target;
	return NULL;
}


/* Returns a freshly-consed, NULL-terminated vector of the names of all the
 * valid bfd targets.  Do not modify the names.
 */
char **
bfd_target_list ()
{
	int vec_length= 0;
	bfd_target **target;
	char **name_list, **name_ptr;

	for (target = &target_vector[0]; *target; target++)
		vec_length++;

	name_ptr = name_list = (char**)bfd_zalloc((bfd *)0,(vec_length+1) * sizeof(char**));

	if (name_list == NULL) {
		bfd_error = no_memory;
		return NULL;
	}

	for (target = &target_vector[0]; *target; target++){
		*(name_ptr++) = (*target)->name;
	}
	return name_list;
}

extern bfd_target b_out_vec_little_host;
extern bfd_target b_out_vec_big_host;
extern bfd_target icoff_little_vec;
extern bfd_target icoff_big_vec;
extern bfd_target icoff_little_big_vec; /* big-endian memory */
extern bfd_target icoff_big_big_vec;    /* big-endian memory */
extern bfd_target elf_vec_little_host; /* big-endian memory */
extern bfd_target elf_vec_big_host;    /* big-endian memory */

static struct magic_strings {
    char *magic_string;
    char length;
    bfd_target *try_list[2];
} magic_strings[]    /* THIS MUST BE MAINTAINED IN SORTED ORDER!!! */
= {
    /* first, objects */

    /* coff objects */
    { /* big endian coff object */ "\1\140",2,{&icoff_big_vec,&icoff_big_big_vec}},
    { /* big endian coff object */ "\1\141",2,{&icoff_big_vec,&icoff_big_big_vec}},
    { /* lit endian coff object */ "\140\1",2,{&icoff_little_vec,&icoff_little_big_vec}},
    { /* lit endian coff object */ "\141\1",2,{&icoff_little_vec,&icoff_little_big_vec}},

    /* bout objects */
    { /* big endian bout object */ "\0\0\1\15",4,{&b_out_vec_big_host,0}},
    { /* lit endian bout object */ "\15\1\0\0",4,{&b_out_vec_little_host,0}},

    /* elf objects */
    { /* lit endian elf object */ "\177ELF\1\1",6,{&elf_vec_little_host,0}},
    { /* big endian elf object */ "\177ELF\1\2",6,{&elf_vec_big_host,0}},

    /* Next, archives: */

    /* coff archives */
    { /* big/lit coff archives. */ "!<arch>\n",8,{&icoff_big_vec,&icoff_big_big_vec}},
    /* bout archives */
    { /* big/lit bout archives. */ "!<bout>\n",8,{&b_out_vec_big_host,0}},
    /* elf archives */
    { /* big/lit elf archives. */ "!<elf_>\n",8,{&elf_vec_big_host,0}},
    };

#define NUM_MAGIC_STRINGS (sizeof(magic_strings) / sizeof(magic_strings[0]) )

static int compar_ms(l,r)
    struct magic_strings *l,*r;
{
    if (l->length < r->length)
	    return -1;
    else if (l->length > r->length)
	    return 1;
    else
	    return strncmp(l->magic_string,r->magic_string,l->length);
}

/* Init a bfd for read of the proper format.  If the target was unspecified,
 * search all the possible targets.
 */
boolean
DEFUN(bfd_check_format,(abfd, format),
      bfd *abfd AND
      bfd_format format)
{
	bfd_target **target, *save_targ, *right_targ;
	int match_count;

	if (!bfd_read_p (abfd)
	|| ((int)(abfd->format) < (int)bfd_unknown)
	|| ((int)(abfd->format) >= (int)bfd_type_end)) {
		bfd_error = invalid_operation;
		return false;
	}

	if (abfd->format != bfd_unknown)
		return (abfd->format == format);

	/* presume the answer is yes */
	abfd->format = format;

	bfd_seek (abfd, (file_ptr)0, SEEK_SET);	/* rewind! */

#if 1
	if (1) {
	    static int amap[] = {8,0};
	    static int omap[] = {6,2,4,0};
	    int *i = format == bfd_archive ? amap : omap;
	    char first_8_bytes[8];
	    struct magic_strings q,*p;

	    save_targ = abfd->xvec;
	
	    bfd_read(first_8_bytes,1,8,abfd);
/*
   We do not sort the magic_strings table because it is stored in sorted order.
   qsort((char*)magic_strings,NUM_MAGIC_STRINGS,
   sizeof(magic_strings[0]),compar_ms);
*/

	    q.magic_string = first_8_bytes;
	    for (;*i;i++) {
		q.length = *i;
		if (p=(struct magic_strings *) bsearch((char*)&q,(char*)magic_strings,
						       NUM_MAGIC_STRINGS,sizeof(q),compar_ms)) {
		    int j;

		    for (j=0;j < 2;j++)
			    if (p->try_list[j]) {
				abfd->xvec = p->try_list[j];
				bfd_seek (abfd, (file_ptr)0, SEEK_SET);	/* rewind! */
				right_targ = BFD_SEND_FMT (abfd, _bfd_check_format, (abfd));
				if (right_targ) {
				    abfd->xvec = right_targ;/* Set the target as returned */
				    return true;		/* File position has moved, BTW */
				}
			    }
		}
	    }
	}
	/* No joy! */	
	abfd->xvec = save_targ;		/* Restore original target type */
	abfd->format = bfd_unknown;	/* Restore original format */
	bfd_error = file_not_recognized;
	return false;
#else

	right_targ = BFD_SEND_FMT (abfd, _bfd_check_format, (abfd));
	if (right_targ) {
		abfd->xvec = right_targ;/* Set the target as returned */
		return true;		/* File position has moved, BTW */
	}

	save_targ = abfd->xvec;
	match_count = 0;
	right_targ = 0;

	for (target = target_vector; *target; target++) {
		bfd_target *temp;

		abfd->xvec = *target;	/* Change BFD's target temporarily */
		bfd_seek (abfd, (file_ptr)0, SEEK_SET);
		temp = BFD_SEND_FMT (abfd, _bfd_check_format, (abfd));
		if (temp) {		/* This format checks out as ok! */
			right_targ = temp;
			match_count++;
			/* Big- and little-endian b.out archives look the same,
			 * but it doesn't matter: there is no difference in
			 * their headers, and member file byte orders will
			 * (I hope) be handled appropriately by bfd.  Ditto
			 * for big and little coff archives.  And the 4
			 * coff/b.out object formats are unambiguous.  So
			 * accept the first match we find.
			 */
			break;
		}
	}

	if (match_count == 1) {
		abfd->xvec = right_targ;/* Change BFD's target permanently */
		return true;		/* File position has moved, BTW */
	}

	abfd->xvec = save_targ;		/* Restore original target type */
	abfd->format = bfd_unknown;	/* Restore original format */
	bfd_error = match_count == 0 ? file_not_recognized :
					    file_ambiguously_recognized;
	return false;
#endif
}

boolean
DEFUN(bfd_set_format,(abfd, format),
      bfd *abfd AND
      bfd_format format)
{

	if (((int)abfd->format < (int)bfd_unknown)
	    || ((int)abfd->format >= (int)bfd_type_end)) {
	    bfd_error = invalid_operation;
	    return false;
	}

	if (abfd->format != bfd_unknown){
		return (abfd->format == format);
	}

	/* presume the answer is yes */
	abfd->format = format;

	if (!BFD_SEND_FMT (abfd, _bfd_set_format, (abfd))) {
		abfd->format = bfd_unknown;
		return false;
	}

	return true;
}

/* Hack object and core file sections */

sec_ptr
DEFUN(bfd_get_section_by_name,(abfd, name),
      bfd *abfd AND
      CONST char *name)
{
	asection *sect;

	for (sect = abfd->sections; sect; sect = sect->next){
		if (!strcmp (sect->name, name)){
			return sect;
		}
	}
	return NULL;
}

/* If you try to create a section with a name which is already in use,
 * returns the old section by that name instead
 */
sec_ptr
DEFUN(bfd_make_section,(abfd, name),
      bfd *abfd AND
      CONST char *CONST name)
{
	asection  *newsect;
	asection **prev = &abfd->sections;
	asection  *sect = abfd->sections;

	if (abfd->output_has_begun) {
		bfd_error = invalid_operation;
		return NULL;
	}

	while (sect) {
		if (!strcmp(sect->name, name)) return sect;
		prev = &sect->next;
		sect = sect->next;
	}

	newsect = (asection *) bfd_zalloc(abfd, sizeof (asection));
	if (newsect == NULL) {
		bfd_error = no_memory;
		return NULL;
	}
	newsect->name = name;
	newsect->secnum = abfd->section_count++;
	newsect->insec_flags = 0;

	if ( !BFD_SEND (abfd, _new_section_hook, (abfd, newsect))) {
		bfd_release(abfd,newsect);
		return NULL;
	}

	*prev = newsect;
	return newsect;
}

/* Call operation on each section.  Operation gets three args: the bfd,
 * the section, and a void * pointer (whatever the user supplied).
 */
/*VARARGS2*/
void
bfd_map_over_sections (abfd, operation, user_storage)
     bfd *abfd;
     void (*operation)();
     PTR user_storage;
{
	asection *sect;

	for ( sect = abfd->sections; sect; sect = sect->next){
		(*operation) (abfd, sect, user_storage);
	}
}

boolean
bfd_set_section_flags (abfd, section, flags)
     bfd *abfd;
     sec_ptr section;
     flagword flags;
{
	if ((flags & bfd_applicable_section_flags (abfd)) != flags) {
		bfd_error = invalid_operation;
		return false;
	}
	section->flags = flags;
	return true;
}


boolean
bfd_set_section_size (abfd, ptr, val)
     bfd *abfd;
     sec_ptr ptr;
     unsigned long val;
{
	/* Once you've started writing to any section you cannot create or
	 * change the size of any others.
	 */
	if (abfd->output_has_begun) {
		bfd_error = invalid_operation;
		return false;
	}
	ptr->size = val;
	return true;
}

boolean
DEFUN(bfd_set_section_contents,(abfd, section, location, offset, count),
      bfd *abfd AND
      sec_ptr section AND
      PTR location AND
      file_ptr offset AND
      bfd_size_type count)
{
    /* We were told to set section contents, therefore we OR in the
       SEC_HAS_CONTENTS flag. */
    bfd_get_section_flags(abfd, section) |= SEC_HAS_CONTENTS;
    if (BFD_SEND (abfd, _bfd_set_section_contents,
		  (abfd, section, location, offset, count))) {
	abfd->output_has_begun = true;
	return true;
    }
    return false;
}

boolean
DEFUN(bfd_get_section_contents,(abfd, section, location, offset, count),
      bfd *abfd AND
      sec_ptr section AND
      PTR location AND
      file_ptr offset AND
      bfd_size_type count)
{
	if ((section->flags & SEC_HAS_CONTENTS) == 0) {
	    /* If section is a bss section and its section data is requested,
	       set it to zeros. */
	    memset(location, 0, (unsigned)count);
	    return true;
	}
	else
		return  (BFD_SEND (abfd, _bfd_get_section_contents,
				   (abfd, section, location, offset, count)));
}


/** Symbols */

asymbol **
bfd_set_symtab (abfd, location, symcount)
     bfd *abfd;
     asymbol **location;
     unsigned int *symcount;
{
	if ((abfd->format != bfd_object) || (bfd_read_only_p (abfd))) {
		bfd_error = invalid_operation;
		return false;
	}

	bfd_get_outsymbols (abfd) = location;
	bfd_get_symcount (abfd) = *symcount;
	return BFD_SEND(abfd, _bfd_set_sym_tab,(abfd,location,symcount));
}

/* returns the number of octets of storage required */
unsigned int
get_reloc_upper_bound (abfd, asect)
     bfd *abfd;
     sec_ptr asect;
{
	if (abfd->format != bfd_object) {
		bfd_error = invalid_operation;
		return 0;
	}

	return BFD_SEND (abfd, _get_reloc_upper_bound, (abfd, asect));
}

unsigned int
bfd_canonicalize_reloc (abfd, asect, location, symbols)
     bfd *abfd;
     sec_ptr asect;
     arelent **location;
     asymbol **symbols;
{
	if (abfd->format != bfd_object) {
		bfd_error = invalid_operation;
		return 0;
	}
	return BFD_SEND(abfd, _bfd_canonicalize_reloc,
					(abfd, asect, location, symbols));
}

void
bfd_print_symbol_vandf(file, symbol)
    PTR file;
    asymbol *symbol;
{
	flagword type = symbol->flags;
	if (symbol->section) {
		fprintf_vma(file, symbol->value+symbol->section->vma);
	} else {
		fprintf_vma(file, symbol->value);
	}
	fprintf(file," %c%c%c%c%c%c%c",	(type & BSF_LOCAL)     ? 'l' : ' ',
					(type & BSF_GLOBAL)    ? 'g' : ' ',
					(type & BSF_IMPORT)    ? 'i' : ' ',
					(type & BSF_EXPORT)    ? 'e' : ' ',
					(type & BSF_UNDEFINED) ? 'u' : ' ',
					(type & BSF_FORT_COMM) ? 'c' : ' ',
					(type & BSF_DEBUGGING) ? 'd' : ' ');
}


boolean
bfd_set_file_flags (abfd, flags)
     bfd *abfd;
     flagword flags;
{
	if (abfd->format != bfd_object) {
		bfd_error = wrong_format;
		return false;
	}
	if (bfd_read_p (abfd)
	||  ((flags & bfd_applicable_file_flags (abfd)) != flags)){
		bfd_error = invalid_operation;
		return false;
	}

	bfd_get_file_flags (abfd) = flags;
	return true;
}


void
bfd_set_reloc (ignore_abfd, asect, location, count)
     bfd *ignore_abfd;
     sec_ptr asect;
     arelent **location;
     unsigned int count;
{
	asect->orelocation = location;
	asect->reloc_count = count;
}


void
bfd_assert(file, line)
    char *file;
    int line;
{
	printf("bfd assertion fail %s:%d\n",file,line);
}


boolean
bfd_set_start_address(abfd, vma)
    bfd *abfd;
    bfd_vma vma;
{
	abfd->start_address = vma;
	return true;
}


bfd_vma
bfd_log2(x)
    bfd_vma x;
{
	bfd_vma result;

	for ( result=0; (bfd_vma)(1<< result) < x; result++ ){
		;
	}
	return result;
}

/* bfd_get_mtime:  Return cached file modification time (e.g. as read
 * from archive header for archive members, or from file system if we have
 * been called before); else determine modify time, cache it, and
 * return it.
 */
long
bfd_get_mtime (abfd)
     bfd *abfd;
{
	FILE *fp;
	struct stat buf;

	if (abfd->mtime_set)
		return abfd->mtime;

	fp = bfd_cache_lookup (abfd);
	if (0 != fstat (fileno (fp), &buf))
		return 0;

	abfd->mtime_set = true;
	abfd->mtime = buf.st_mtime;
	return abfd->mtime;
}
