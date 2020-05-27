/*** archive.c -- an attempt at combining the machine-independent parts of
  archives */

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


/* Assumes:
 *	o all archive elements start on an even boundary, newline padded;
 *	o all arch headers are char *;
 *	o all arch headers are the same size (across architectures).
 */


#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "ar.h"

#define ARHDRSZ	sizeof(struct ar_hdr)

/* The Symdef member of an archive contains two things:
 * a table that maps symbol-string offsets to file offsets,
 * and a symbol-string table.  All the symbol names are
 * run together (each with trailing null) in the symbol-string
 * table.  There is a single longword bytecount on the front
 * of each of these tables.  Thus if we have two symbols,
 * "foo" and "_bar", that are in archive members at offsets
 * 200 and 900, it would look like this:
 *      16		; byte count of index table
 *	0		; offset of "foo" in string table
 *	200		; offset of foo-module in file
 *	4		; offset of "bar" in string table
 *	900		; offset of bar-module in file
 *	9		; byte count of string table
 *	"foo\0_bar\0"	; string table
 */

/* Format of __.SYMDEF:
 * First, a longword containing the size of the 'symdef' data that follows.
 * Second, zero or more 'symdef' structures.
 * Third, a longword containing the length of symbol name strings.
 * Fourth, zero or more symbol name strings (each followed by a null).
 */
struct symdef
{
	union {
		unsigned long string_offset;	/* In the file */
		char *name;			/* In memory, sometimes */
	} s;

	/* this points to the front of the file header (AKA member header --
         * a struct ar_hdr), not to the front of the file or into the file).
         * in other words it only tells you which file to read
	 */
	unsigned long file_offset;
};

#define RANLIBMAG       "__.SYMDEF       "
#define BFD_GNU960_ARMAG(abfd)	(BFD_COFF_FILE_P((abfd)) ? ARMAG : \
				 BFD_BOUT_FILE_P((abfd)) ? ARMAGB : \
				 BFD_ELF_FILE_P((abfd)) ? ARMAGE : "barf")

/* We keep a cache of archive filepointers to archive elements to
 * speed up searching the archive by filepos.  We only add an entry to
 * the cache when we actually read one.  We also don't sort the cache;
 * it's short enough to search linearly.
 * Note that the pointers here point to the front of the ar_hdr, not
 * to the front of the contents!
 */
struct ar_cache {
	file_ptr ptr;
	bfd* arelt;
	struct ar_cache *next;
};

#define ar_padchar(abfd) ((abfd)->xvec->ar_pad_char)
#define ar_maxnamelen(abfd) ((abfd)->xvec->ar_max_namelen)

#define arch_hdr(bfd) ((struct ar_hdr *)   \
		       (((struct areltdata *)((bfd)->arelt_data))->arch_header))

boolean
_bfd_generic_mkarchive (abfd)
     bfd *abfd;
{
	set_tdata (abfd, bfd_zalloc(abfd, sizeof (struct artdata)));

	if (bfd_ardata (abfd) == NULL) {
		bfd_error = no_memory;
		return false;
	}
	bfd_ardata(abfd)->cache = 0;
	return true;
}

symindex
bfd_get_next_mapent (abfd, prev, entry)
     bfd *abfd;
     symindex prev;
     carsym **entry;
{
	if (!bfd_has_map (abfd)) {
		bfd_error = invalid_operation;
		return BFD_NO_MORE_SYMBOLS;
	}

	if (prev == BFD_NO_MORE_SYMBOLS) prev = 0;
	else if (++prev >= bfd_ardata (abfd)->symdef_count)
		return BFD_NO_MORE_SYMBOLS;

	*entry = (bfd_ardata (abfd)->symdefs + prev);
	return prev;
}


/* To be called by backends only */
bfd *
_bfd_create_empty_archive_element_shell (obfd)
     bfd *obfd;
{
	bfd *nbfd;

	nbfd = new_bfd_contained_in(obfd);
	if (nbfd == NULL) {
		bfd_error = no_memory;
		return NULL;
	}
	return nbfd;
}


boolean
bfd_set_archive_head (output_archive, new_head)
     bfd *output_archive, *new_head;
{
	output_archive->archive_head = new_head;
	return true;
}


bfd *
look_for_bfd_in_cache (arch_bfd, filepos)
     bfd *arch_bfd;
     file_ptr filepos;
{
	struct ar_cache *cur;

	for (cur = bfd_ardata (arch_bfd)->cache; cur != NULL; cur = cur->next){
		if (cur->ptr == filepos) {
			return cur->arelt;
		}
	}
	return NULL;
}


/* Kind of stupid to call cons for each one, but we don't do too many */
boolean
add_bfd_to_cache (arch_bfd, filepos, new_elt)
     bfd *arch_bfd, *new_elt;
     file_ptr filepos;
{
	struct ar_cache *new_cache = (struct ar_cache *)
				bfd_zalloc(arch_bfd, sizeof (struct ar_cache));

	if (new_cache == NULL) {
		bfd_error = no_memory;
		return false;
	}

	new_cache->ptr = filepos;
	new_cache->arelt = new_elt;
	new_cache->next = (struct ar_cache *)NULL;
	if (bfd_ardata (arch_bfd)->cache == NULL) {
		bfd_ardata (arch_bfd)->cache = new_cache;
	} else {
		struct ar_cache *cur = bfd_ardata (arch_bfd)->cache;

		for (; cur->next != NULL; cur = cur->next) {
			;
		}
		cur->next = new_cache;
	}
	return true;
}

/* The name begins with space.  Hence the rest of the name is an index into
 * the string table.
 */
char *
get_extended_arelt_filename (arch, name)
     bfd *arch;
     char *name;
{
#ifndef errno
#ifndef __HIGHC__
	extern int errno;
#endif
#endif
	unsigned long i = 0;

	/* Should extract string so that I can guarantee not to overflow into
	 * the next region.
	 */
	errno = 0;
	i = strtol (name, NULL, 10);
	if (errno != 0) {
		bfd_error = malformed_archive;
		return NULL;
	}

	return bfd_ardata (arch)->extended_names + i;
}

/* This functions reads an arch header and returns an areltdata pointer,
 * or NULL on error.
 *
 * Presumes the file pointer is already in the right place (ie pointing
 * to the ar_hdr in the file).   Moves the file pointer; on success it
 * should be pointing to the front of the file contents; on failure it
 * could have been moved arbitrarily.
 */
struct areltdata *
snarf_ar_hdr (abfd)
     bfd *abfd;
{
#ifndef errno
#ifndef __HIGHC__
	extern int errno;
#endif
#endif
	struct ar_hdr hdr;
	unsigned int parsed_size;
	struct areltdata *ared;
	char *filename = NULL;
	unsigned int namelen = 0;
	unsigned int allocsize = sizeof(struct areltdata) + ARHDRSZ;
	char *allocptr;

	if (bfd_read((PTR)&hdr,1,ARHDRSZ,abfd) != ARHDRSZ) {
		bfd_error = no_more_archived_files;
		return NULL;
	}

	if (strncmp ((hdr.ar_fmag), ARFMAG, 2)) {
		bfd_error = malformed_archive;
		return NULL;
	}

	errno = 0;
	parsed_size = strtol (hdr.ar_size, NULL, 10);
	if (errno != 0) {
		bfd_error = malformed_archive;
		return NULL;
	}

	/* essentially to preserve time stamps */
	abfd->mtime = atol(hdr.ar_date);
	abfd->mtime_set = true;


	/* extract the filename from the archive - there are two ways to
	 * specify an extendend name table, either the first char of the
	 * name is a space(for coff and bout), or it's a slash (for elf).
	 */
	if ((hdr.ar_name[0] == '/' || hdr.ar_name[0] == ' ')
	&&  bfd_ardata (abfd)->extended_names != NULL) {
		filename = get_extended_arelt_filename (abfd, hdr.ar_name+1);
		if (filename == NULL) {
			bfd_error = malformed_archive;
			return NULL;
		}
	} else {
		/* Find end of the name (a space or a padchar) */
		namelen = 0;
		while (namelen < (unsigned)ar_maxnamelen(abfd) &&
		    ( hdr.ar_name[namelen] != 0 &&
		    hdr.ar_name[namelen] != ' ' &&
		    hdr.ar_name[namelen] != ar_padchar(abfd))) {
			namelen++;
		}
		allocsize += namelen + 1;
	}

	allocptr = bfd_zalloc(abfd, allocsize);
	if (allocptr == NULL) {
		bfd_error = no_memory;
		return NULL;
	}

	ared = (struct areltdata *) allocptr;

	ared->arch_header = allocptr + sizeof (struct areltdata);
	memcpy ((char *) ared->arch_header, &hdr, ARHDRSZ);
	ared->parsed_size = parsed_size;

	if (filename != NULL) ared->filename = filename;
	else {
		ared->filename = allocptr + (sizeof (struct areltdata) + ARHDRSZ);
		if (namelen)
			memcpy (ared->filename, hdr.ar_name, namelen);
		ared->filename[namelen] = '\0';
	}

		
		
	return ared;
}

bfd *
get_elt_at_filepos (archive, filepos)
     bfd *archive;
     file_ptr filepos;
{
	struct areltdata *new_areldata;
	bfd *n_nfd;

	n_nfd = look_for_bfd_in_cache (archive, filepos);
	if (n_nfd) return n_nfd;

	if (0 > bfd_seek (archive, filepos, SEEK_SET)) {
		bfd_error = system_call_error;
		return NULL;
	}

	if ((new_areldata = snarf_ar_hdr (archive)) == NULL) return NULL;

	n_nfd = _bfd_create_empty_archive_element_shell (archive);
	if (n_nfd == NULL) {
		bfd_release (archive, (PTR)new_areldata);
		return NULL;
	}
	n_nfd->origin = bfd_tell (archive);
	n_nfd->arelt_data = (PTR) new_areldata;
	n_nfd->filename = new_areldata->filename;

	if (add_bfd_to_cache (archive, filepos, n_nfd))
		return n_nfd;

	/* huh? */
	bfd_release (archive, (PTR)n_nfd);
	bfd_release (archive, (PTR)new_areldata);
	return NULL;
}


bfd *
bfd_get_elt_at_index (abfd, i)
     bfd *abfd;
     int i;
{
	bfd *result = get_elt_at_filepos(abfd,
			(bfd_ardata (abfd)->symdefs + i)->file_offset);
	return result;
}


/* 
 * If you've got an archive, call this to read each subfile.
 */
bfd *
bfd_openr_next_archived_file (archive, last_file)
     bfd *archive, *last_file;
{

	if ((bfd_get_format (archive) != bfd_archive)
	    ||  (archive->direction == write_direction)) {
		bfd_error = invalid_operation;
		return NULL;
	}
	return BFD_SEND(archive,openr_next_archived_file,(archive, last_file));
}


bfd *bfd_generic_openr_next_archived_file(archive, last_file)
     bfd *archive;
     bfd *last_file;
{
	file_ptr filestart;

	if (!last_file) {
		filestart = bfd_ardata (archive)->first_file_filepos;
	} else {
		unsigned int size = arelt_size(last_file);
		/* Pad to an even boundary... */
		filestart = last_file->origin + size + size%2;
	}
	return get_elt_at_filepos (archive, filestart);
}


bfd_target *
bfd_generic_archive_p (abfd)
     bfd *abfd;
{
	char armag[SARMAG+1];

	if (bfd_read ((PTR)armag, 1, SARMAG, abfd) != SARMAG) {
		bfd_error = wrong_format;
		return 0;
	}

	if (strncmp (armag, BFD_GNU960_ARMAG(abfd), SARMAG)) return 0;

	/* We are setting bfd_ardata(abfd) here, but since bfd_ardata
	 * involves a cast, we can't do it as the left operand of assignment.
	 */
	set_tdata (abfd, bfd_zalloc(abfd,sizeof (struct artdata)));

	if (bfd_ardata (abfd)  == NULL) {
		bfd_error = no_memory;
		return 0;
	}

	bfd_ardata (abfd)->first_file_filepos = SARMAG;

	if (!BFD_SEND (abfd, _bfd_slurp_armap, (abfd))) {
		bfd_release(abfd, bfd_ardata (abfd));
		abfd->tdata = NULL;
		return 0;
	}

	if (!BFD_SEND (abfd, _bfd_slurp_extended_name_table, (abfd))) {
		bfd_release(abfd, bfd_ardata (abfd));
		abfd->tdata = NULL;
		return 0;
	}
	return abfd->xvec;
}


boolean				/* false on error, true otherwise */
bfd_slurp_bsd_armap (abfd)
     bfd *abfd;
{
	int i;
	struct areltdata *mapdata;
	char nextname[17];
	unsigned int counter = 0;
	int *raw_armap, *rbase;
	struct artdata *ardata = bfd_ardata (abfd);
	char *stringbase;

	/* FIXME, if the read fails, this routine quietly returns "true"!!
	 * It should probably do that if the read gives 0 bytes (empty archive),
	 * but fail for any other size...
	 */
	if (bfd_read ((PTR)nextname, 1, 16, abfd) == 16) {
		/* The archive has at least 16 bytes in it */
		bfd_seek (abfd, -16L, SEEK_CUR);

		if (strncmp (nextname, RANLIBMAG, 16)) {
			bfd_has_map (abfd) = false;
			return true;
		}

		mapdata = snarf_ar_hdr (abfd);
		if (mapdata == NULL) return false;

		raw_armap = (int *) bfd_zalloc(abfd,mapdata->parsed_size);
		if (raw_armap == NULL) {
			bfd_error = no_memory;
			bfd_release (abfd, (PTR)mapdata);
			return false;
		}

		if (bfd_read((PTR)raw_armap, 1, mapdata->parsed_size, abfd)
						!= mapdata->parsed_size) {
			bfd_error = malformed_archive;
			bfd_release (abfd, (PTR)raw_armap);
			bfd_release (abfd, (PTR)mapdata);
			return false;
		}

		ardata->symdef_count = bfd_get_32(abfd,raw_armap) /
							sizeof(struct symdef);
		ardata->cache = 0;
		rbase = raw_armap+1;
		ardata->symdefs = (carsym *) rbase;
		stringbase = ((char *)(ardata->symdefs + ardata->symdef_count))
									+ 4;

		for (;counter < ardata->symdef_count; counter++) {
			struct symdef *sym;

			sym = ((struct symdef *) rbase) + counter;
			sym->s.name = bfd_get_32(abfd,
					&(sym->s.string_offset)) + stringbase;
			sym->file_offset = bfd_get_32(abfd,&(sym->file_offset));
		}

		ardata->first_file_filepos = bfd_tell (abfd);
		/* Pad to an even boundary if you have to */
		ardata->first_file_filepos += (ardata-> first_file_filepos) %2;
		bfd_has_map (abfd) = true;
	}
	return true;
}


boolean
bfd_slurp_coff_armap (abfd)
     bfd *abfd;
{
	struct areltdata *mapdata;
	char nextname;
	int *raw_armap, *rawptr;
	struct artdata *ardata = bfd_ardata (abfd);
	char *stringbase;
	carsym *carsyms;
	unsigned int nsymz;
	unsigned int carsym_size;
	unsigned int stringsize;
	int result;

	result = bfd_read ((PTR)&nextname, 1, 1, abfd);
	bfd_seek (abfd, -1L, SEEK_CUR);

	if ( result != 1 || nextname != '/') {
		/* Actually I think this is an error for a COFF archive */
		bfd_has_map (abfd) = false;
		return true;
	}

	mapdata = snarf_ar_hdr (abfd);
	if (mapdata == NULL) return false;

	raw_armap = (int *) bfd_zalloc (abfd,mapdata->parsed_size);
	if (raw_armap == NULL) {
		bfd_error = no_memory;
		bfd_release (abfd,(PTR)mapdata);
		return false;
	}

	if (bfd_read ((PTR)raw_armap, 1, mapdata->parsed_size, abfd) !=
	    mapdata->parsed_size) {
		bfd_error = malformed_archive;
		bfd_release (abfd,(PTR)raw_armap);
		bfd_release (abfd,(PTR)mapdata);
		return false;
	}

	/* The coff armap must be read sequentially.  So we construct a
	 * bsd-style one in core all at once, for simplicity.
	 *
	 * NOTE THAT REGARDLESS OF ENDIANESS OF THE ARCHIVE'S CONTENTS, THE
	 * BINARY VALUES IN A COFF ARMAP (# of symbols and file offsets) ARE
	 * ALWAYS STORED IN BIG-ENDIAN BYTE ORDER.  So use _do_getb32() to
	 * access them.
	 */
	nsymz = _do_getb32( (unsigned char*)raw_armap );
	carsym_size = (nsymz * sizeof (carsym));
	stringsize = mapdata->parsed_size - (4 * (nsymz)) - 4;

	/* Below, sizeof(char *) is to ensure at least a null pointer will be present in
	   the symdefs. */
	ardata->symdefs = (carsym *)bfd_zalloc(abfd,carsym_size+stringsize+sizeof(char *));
	if (ardata->symdefs == NULL) {
		bfd_error = no_memory;
		bfd_release (abfd,(PTR)raw_armap);
		bfd_release (abfd,(PTR)mapdata);
		return false;
	}
	carsyms = ardata->symdefs;

	stringbase = ((char *) ardata->symdefs) + carsym_size;
	memcpy (stringbase, (char*)raw_armap + (4 * nsymz) + 4,  stringsize);


	/* OK, build the carsyms */
	ardata->symdef_count = nsymz;
	rawptr = raw_armap;
	while (  nsymz-- )
	{
		rawptr++;
		carsyms->file_offset = _do_getb32( (unsigned char*)rawptr );
		carsyms->name = stringbase;
		for (; *(stringbase++););
		carsyms++;
	}
	*stringbase = 0;

	ardata->first_file_filepos = bfd_tell (abfd);
	/* Pad to an even boundary if you have to */
	ardata->first_file_filepos += (ardata->first_file_filepos) %2;

	/*
	bfd_release (abfd, (PTR)raw_armap);
	bfd_release (abfd, (PTR)mapdata);
	*/

	bfd_has_map (abfd) = true;
	return true;
}

/* Extended name table.
 *
 * Normally archives support only 14-character filenames.
 *
 * Intel has extended the format: longer names are stored in a special
 * element (the first in the archive, or second if there is an armap);
 * the name in the ar_hdr is replaced by <space><index into filename
 * element>.  Index is the P.R. of an int (radix: 8).  Data General have
 * extended the format by using the prefix // for the special element
 */
boolean				/* false on error, true otherwise */
_bfd_slurp_extended_name_table (abfd)
     bfd *abfd;
{
	char nextname[17];
	struct areltdata *namedata;

	/* FIXME:  Formatting sucks here, and in case of failure of BFD_READ,
	 * we probably don't want to return true.
	 */

	if (bfd_read ((PTR)nextname, 1, 16, abfd) == 16) {
		bfd_seek (abfd, -16L, SEEK_CUR);

#define ELF_STRING_TABLE_NAME   "//              "
#define OTHER_STRING_TABLE_NAME "ARFILENAMES/    "

#define STRING_TABLE_NAME(BFD)  (BFD_ELF_FILE_P((BFD)) ? ELF_STRING_TABLE_NAME : OTHER_STRING_TABLE_NAME)

#define SYMBOL_TABLE_NAME       "/               "

		if (strncmp (nextname, STRING_TABLE_NAME(abfd) , 16) != 0
		&&  strncmp (nextname, SYMBOL_TABLE_NAME , 16) != 0) {
			bfd_ardata (abfd)->extended_names = NULL;
			return true;
		}

		namedata = snarf_ar_hdr (abfd);
		if (namedata == NULL) return false;

		bfd_ardata(abfd)->extended_names =
					bfd_zalloc(abfd,namedata->parsed_size);
		if (bfd_ardata (abfd)->extended_names == NULL) {
			bfd_error = no_memory;
			bfd_release (abfd, (PTR)namedata);
			return false;
		}

		if (bfd_read ((PTR)bfd_ardata (abfd)->extended_names, 1,
		    namedata->parsed_size, abfd) != namedata->parsed_size) {
			bfd_error = malformed_archive;
			bfd_release(abfd,(PTR)(bfd_ardata(abfd)->extended_names));
			bfd_ardata (abfd)->extended_names = NULL;
			bfd_release (abfd, (PTR)namedata);
			return false;
		}

		/* Since the archive is supposed to be printable if it contains
		 * text, the entries in the list are newline-padded, not null
		 * padded. We'll fix that there..
		 */
		{
		    int i;
		    char *temp = bfd_ardata (abfd)->extended_names;
		    for (i=0;i < namedata->parsed_size; ++i,++temp)
			    if (*temp == '\n') *temp = '\0';
		}

		/* Pad to an even boundary if you have to */
		bfd_ardata (abfd)->first_file_filepos = bfd_tell (abfd);
		bfd_ardata (abfd)->first_file_filepos +=
		    (bfd_ardata (abfd)->first_file_filepos) %2;
		bfd_release (abfd, namedata);
	}
	return true;
}

#ifdef DOS
/* This is use to normalize a pathname. In DOS, pathnames can have
 *  '\', or ':'.
 */
#define NORMALIZE(d, s) {if (!d) {d = strrchr(s, '\\'); if (!d) d = strrchr(s, ':'); }}
#else
#define NORMALIZE(d, s) /* no op */
#endif

static
char *normalize(file)
char *file;
{
	char * filename = strrchr(file, '/');

	NORMALIZE( filename, file );
	return filename ? filename+1 : file;
}


/* Follows archive_head and produces an extended name table if necessary.
 * Returns (in tabloc) a pointer to an extended name table, and in tablen
 * the length of the table.  If it makes an entry it clobbers the filename
 * so that the element may be written without further massage.
 * Returns true if it ran successfully, false if something went wrong.
 * A successful return may still involve a zero-length tablen!
 */
boolean
bfd_construct_extended_name_table (abfd, tabloc, tablen)
     bfd *abfd;
     char **tabloc;
     unsigned int *tablen;
{
	unsigned int maxname = abfd->xvec->ar_max_namelen;
	unsigned int total_namelen = 0;
	bfd *cur;
	char *strptr;

	*tablen = 0;

	/* Figure out how long the table should be */
	for (cur = abfd->archive_head; cur != NULL; cur = cur->next){
		unsigned int thislen = strlen (normalize(cur->filename));
		/* leave room for \n */
		if (thislen > maxname) total_namelen += thislen + 1;
	}

	if (total_namelen == 0) return true;

	*tabloc = bfd_zalloc (abfd,total_namelen);
	if (*tabloc == NULL) {
		bfd_error = no_memory;
		return false;
	}

	*tablen = total_namelen;
	strptr = *tabloc;

	for (cur = abfd->archive_head; cur != NULL; cur = cur->next) {
		char *p;
		char *normal = normalize( cur->filename);
		unsigned int thislen = strlen (normal);
		if (thislen > maxname) {
			struct ar_hdr *hdr = arch_hdr(cur);
			strcpy (strptr, normal);
			strptr[thislen] = '\n';
#define SLASH(BFD) (BFD_ELF_FILE_P((BFD)) ? '/' : ' ')
			hdr->ar_name[0] = SLASH(abfd);
			/* We know there will always be enough room (one of
			 * the few cases where you may safely use sprintf).
			 */
			sprintf((hdr->ar_name)+1, "%-d",
						(unsigned) (strptr - *tabloc));
			/* Not all implementations of sprintf return a usable
			 * value, so do this by hand.
			 */
			for (p=hdr->ar_name+2; p < hdr->ar_name + maxname; p++){
				if (*p == '\0') {
					*p = ' ';
				}
			}
			strptr += thislen + 1;
		}
	}
	return true;
}

/** A couple of functions for creating ar_hdrs */

/* Takes a filename, returns an arelt_data for it, or NULL if it can't make one.
 * The filename must refer to a filename in the filesystem.
 * The filename field of the ar_hdr will NOT be initialized
 */
struct areltdata *
DEFUN(bfd_ar_hdr_from_filesystem, (abfd,filename),
      bfd* abfd AND
      CONST char *filename)
{
	struct stat status;
	struct areltdata *ared;
	struct ar_hdr *hdr;
	char *temp, *temp1;

	if (stat (filename, &status) != 0) {
		bfd_error = system_call_error;
		return NULL;
	}

	ared = (struct areltdata *) bfd_zalloc(abfd, ARHDRSZ +
										sizeof(struct areltdata));
	if (ared == NULL) {
		bfd_error = no_memory;
		return NULL;
	}
	hdr = (struct ar_hdr *) (((char *) ared) + sizeof (struct areltdata));

	/* ar headers are space padded, not null padded! */
	temp = (char *) hdr;
	temp1 = temp + ARHDRSZ - 2;
	for (; temp < temp1; *(temp++) = ' ');
	strncpy (hdr->ar_fmag, ARFMAG, 2);

	if (abfd->flags & SUPP_W_TIME)
		sprintf ((hdr->ar_date), "%-12ld", 0); 
	else if (abfd->mtime != 0)
		sprintf ((hdr->ar_date), "%-12ld", abfd->mtime);
	else
		sprintf ((hdr->ar_date), "%-12ld", status.st_mtime); 

#ifdef DOS
	/* status.st_uid and status.st_gid are not meaningful
         * in DOS. Let's set it to zero.
         */
	sprintf ((hdr->ar_uid), "%d", 0);
	sprintf ((hdr->ar_gid), "%d", 0);
#else
	sprintf ((hdr->ar_uid), "%d", status.st_uid);
	sprintf ((hdr->ar_gid), "%d", status.st_gid);
#endif
	sprintf ((hdr->ar_mode), "%-8o", (unsigned) status.st_mode);
	sprintf ((hdr->ar_size), "%-10ld", status.st_size);
	/* Correct for a lossage in sprintf whereby it null-terminates */
	temp = (char *) hdr;
	temp1 = temp + ARHDRSZ - 2;
	for (; temp < temp1; temp++) {
		if (*temp == '\0') *temp = ' ';
	}
	strncpy (hdr->ar_fmag, ARFMAG, 2);
	ared->parsed_size = status.st_size;
	ared->arch_header = (char *) hdr;

	return ared;
}


struct ar_hdr *
DEFUN(bfd_special_undocumented_glue, (abfd, filename),
      bfd *abfd AND
      char *filename)
{
	return (struct ar_hdr *)bfd_ar_hdr_from_filesystem(abfd,filename)->arch_header;
}


/* Analogous to stat call */
int
bfd_generic_stat_arch_elt (abfd, bufp)
     bfd *abfd;
     PTR bufp;
{
	struct stat *buf = (struct stat *)bufp;
	struct ar_hdr *hdr;
	char *aloser;

	if (abfd->arelt_data == NULL) {
		bfd_error = invalid_operation;
		return -1;
	}

	hdr = arch_hdr (abfd);

#define foo(arelt, stelt, size)  \
  buf->stelt = strtol (hdr->arelt, &aloser, size); \
  if (aloser == hdr->arelt) return -1;

	foo (ar_date, st_mtime, 10);
	foo (ar_uid, st_uid, 10);
	foo (ar_gid, st_gid, 10);
	foo (ar_mode, st_mode, 8);
	foo (ar_size, st_size, 10);

	return 0;
}

void
bfd_dont_truncate_arname (abfd, pathname, arhdr)
     bfd *abfd;
     CONST char *pathname;
     char *arhdr;
{
	/* FIXME: This interacts unpleasantly with ar's quick-append option.
	 * Fortunately ic960 users will never use that option.  Fixing this
	 * is very hard; fortunately I know how to do it and will do so once
	 * intel's release is out the door.
	 */
	struct ar_hdr *hdr = (struct ar_hdr *) arhdr;
	int length;
	CONST char *filename = strrchr (pathname, '/');
	int maxlen = ar_maxnamelen (abfd);

	NORMALIZE(filename, pathname);

	filename = (filename == NULL) ? pathname : filename+1;
	length = strlen (filename);
	if (length <= maxlen) {
		memcpy (hdr->ar_name, filename, length);
	}
	if (length < maxlen) {
		(hdr->ar_name)[length] = ar_padchar (abfd);
	}
}

void
bfd_bsd_truncate_arname (abfd, pathname, arhdr)
     bfd *abfd;
     CONST char *pathname;
     char *arhdr;
{
	struct ar_hdr *hdr = (struct ar_hdr *) arhdr;
	int length;
	CONST char *filename = strrchr (pathname, '/');
	int maxlen = ar_maxnamelen (abfd);

	NORMALIZE(filename, pathname);

	filename = (filename == NULL) ? pathname : filename+1;
	length = strlen (filename);
	if (length <= maxlen) {
		memcpy (hdr->ar_name, filename, length);
	} else {
		memcpy (hdr->ar_name, filename, maxlen);
		length = maxlen;
	}

	if (length < maxlen) {
		(hdr->ar_name)[length] = ar_padchar (abfd);
	}
}


/* Store name into ar header.  Truncates the name to fit.
 * 1> strip pathname to be just the basename.
 * 2> if it's short enuf to fit, stuff it in.
 * 3> If it doesn't end with .o, truncate it to fit
 * 4> truncate it before the .o, append .o, stuff THAT in.
 *
 * This is what gnu ar does.  It's better but incompatible with the bsd ar.
 */
void
bfd_gnu_truncate_arname (abfd, pathname, arhdr)
     bfd *abfd;
     CONST char *pathname;
     char *arhdr;
{
	struct ar_hdr *hdr = (struct ar_hdr *) arhdr;
	int length;
	CONST char *filename = strrchr (pathname, '/');
	int maxlen = ar_maxnamelen (abfd);

	NORMALIZE(filename, pathname);

	filename = (filename == NULL) ? pathname : filename+1;
	length = strlen (filename);

	if (length <= maxlen) {
		memcpy (hdr->ar_name, filename, length);
	} else {
		memcpy (hdr->ar_name, filename, maxlen);
		if ((filename[length-2] == '.') && (filename[length-1] == 'o')){
			hdr->ar_name[maxlen-2] = '.';
			hdr->ar_name[maxlen-1] = 'o';
		}
		length = maxlen;
	}
	if (length < 16) {
		(hdr->ar_name)[length] = ar_padchar (abfd);
	}
}

PROTO (boolean, compute_and_write_armap, (bfd *arch, unsigned int elength));

/*
 * The bfd is open for write and has its format set to bfd_archive
 */
boolean
_bfd_write_archive_contents (arch)
     bfd *arch;
{
	bfd *cur;
	char *etable = NULL;
	unsigned int elength = 0;
	boolean makemap = bfd_has_map (arch);
	boolean hasobjects = false;
				/* if no .o's, don't bother to make a map */
	unsigned int i;

	/* Verify the viability of all entries; if any of them live in the
	 * filesystem (as opposed to living in an archive open for input)
	 * then construct a fresh ar_hdr for them.
	 */
	for (cur = arch->archive_head; cur; cur = cur->next) {
		if (bfd_write_p (cur)) {
			bfd_error = invalid_operation;
			return false;
		}
		if (!cur->arelt_data) {
			cur->arelt_data =
			    (PTR)bfd_ar_hdr_from_filesystem(arch,cur->filename);
			if (!cur->arelt_data) return false;

			/* Put in the file name */
			BFD_SEND(arch,_bfd_truncate_arname,(arch,cur->filename,
						    (char *) arch_hdr(cur)));
		}

		if (makemap) {	/* don't bother if we won't make a map! */
			if ((bfd_check_format (cur, bfd_object))
#if 0
			/* FIXME -- these are not set correctly */
			&& ((bfd_get_file_flags (cur) & HAS_SYMS))
#endif
			)
				hasobjects = true;
		}
	}

	if (!bfd_construct_extended_name_table (arch, &etable, &elength))
		return false;

	bfd_seek (arch, 0, SEEK_SET);
	bfd_write (BFD_GNU960_ARMAG(arch), 1, SARMAG, arch);

	if (makemap && hasobjects) {



		if (compute_and_write_armap (arch, elength) != true) {
			return false;
		}
	}

	if (elength != 0) {
		struct ar_hdr hdr;

		memset ((char *)(&hdr), 0, ARHDRSZ);
		sprintf (&(hdr.ar_name[0]), STRING_TABLE_NAME(arch) );
		sprintf (&(hdr.ar_size[0]), "%-10d", (int) elength);
		hdr.ar_fmag[0] = '`'; 
		hdr.ar_fmag[1] = '\n';
		for (i = 0; i < ARHDRSZ; i++){
			if (((char *)(&hdr))[i] == '\0'){
				(((char *)(&hdr))[i]) = ' ';
			}
		}
		bfd_write ((char *)&hdr, 1, ARHDRSZ, arch);
		bfd_write (etable, 1, elength, arch);
		if ((elength % 2) == 1) bfd_write ("\n", 1, 1, arch);
	}

	for (cur = arch->archive_head; cur; cur = cur->next) {
		char buffer[DEFAULT_BUFFERSIZE];
		unsigned int remaining = arelt_size (cur);
		struct ar_hdr *hdr = arch_hdr(cur);
		/* write ar header */

		if ( (bfd_write((char*)hdr,1,sizeof(*hdr),arch) != sizeof(*hdr))
		||   (bfd_seek(cur,0L,SEEK_SET) != 0L) ){
			bfd_error = system_call_error;
			return false;
		}
		while (remaining) {
			unsigned int amt = DEFAULT_BUFFERSIZE;
			if (amt > remaining) {
				amt = remaining;
			}
			if ( (bfd_read (buffer, amt, 1, cur) != amt)

			||   (bfd_write (buffer, amt, 1, arch)   != amt)) {
				bfd_error = system_call_error;
				return false;
			}
			remaining -= amt;
		}
		if ((arelt_size (cur) % 2) == 1) {
			bfd_write ("\n", 1, 1, arch);
		}
	}
	return true;
}

/*
 * Note that the namidx for the first symbol is 0
 */
boolean
compute_and_write_armap (arch, elength)
     bfd *arch;
     unsigned int elength;
{
	bfd *cur;
	file_ptr elt_no = 0;
	struct orl *map;
	int orl_max = 15000;
	int orl_count = 0;
	int stridx = 0;

	/* Dunno if this is the best place for this info... */
	if (elength != 0) {
		elength += ARHDRSZ;
	}
	elength += elength %2 ;

	map = (struct orl *) bfd_zalloc (arch,orl_max * sizeof (struct orl));
	if (map == NULL) {
		bfd_error = no_memory;
		return false;
	}

	/* Map over each archive member */
	for (cur = arch->archive_head; cur; cur = cur->next, elt_no++) {
		asymbol **syms;
		unsigned int storage;
		unsigned int symcount;
		unsigned int i;

		if (!bfd_check_format(cur,bfd_object)
		||  !(bfd_get_file_flags (cur) & HAS_SYMS)
		||  ((storage = get_symtab_upper_bound(cur)) == 0) ) {
			continue;
		}

		syms = (asymbol **) bfd_zalloc (arch,storage);
		if (syms == NULL) {
			bfd_error = no_memory;
			return false;
		}
		symcount = bfd_canonicalize_symtab (cur, syms);

		/* Scan symbol table for globals and commons */
		for (i = 0; i <symcount; i++) {
			if (syms[i]->flags & (BSF_GLOBAL|BSF_FORT_COMM)) {
				if (orl_count == orl_max) {
					orl_max *= 2;
					map = (struct orl *) bfd_realloc(arch,(char*)map,
										orl_max*sizeof(struct orl));
				}
				map[orl_count].name = (char **) &(syms[i]->name);
				map[orl_count].pos = (file_ptr) cur;
				map[orl_count].namidx = stridx;
				stridx += strlen(syms[i]->name) + 1;
				++orl_count;
			}
		}
	}

	/* We've collected all the data; write them out */
	return BFD_SEND(arch,write_armap,(arch,elength,map,orl_count,stridx));
}


boolean
bsd_write_armap (arch, elength, map, orl_count, strsize)
     bfd *arch;
     unsigned int elength;
     struct orl *map;
     int orl_count;
     int strsize;
{
	file_ptr armem_file_ptr;
	int temp;
	struct ar_hdr hdr;
	struct stat statbuf;
	unsigned int i;
	char *p;
	bfd *prev_elt;		/* last element arch seen */

	unsigned int symsize = orl_count * sizeof(struct symdef);
	unsigned int mapsize = 4 + symsize + 4 + strsize;
	int padit = 0;

	if ( mapsize & 1 ){
		padit = 1;
		mapsize++;
	}

	/* FORMAT AND WRITE THE HEADER */
	stat (arch->filename, &statbuf);
	memset ((char *)(&hdr), 0, ARHDRSZ);
	strncpy (hdr.ar_name, RANLIBMAG, sizeof(hdr.ar_name));

	if (arch->flags & SUPP_W_TIME)
		sprintf (hdr.ar_date, "%ld", NULL);
	else if (arch->mtime_set)
		sprintf (hdr.ar_date, "%ld", arch->mtime);
	else
		sprintf (hdr.ar_date, "%ld", statbuf.st_mtime);

	sprintf (hdr.ar_size, "%-10d", (int) mapsize);
	hdr.ar_fmag[0] = '`'; 
	hdr.ar_fmag[1] = '\n';
	for (i = 0, p = (char *)&hdr; i < ARHDRSZ; i++, p++) {
		if (*p == '\0') {
			*p = ' ';
		}
	}
	bfd_write ((char *)&hdr, 1, ARHDRSZ, arch);

	/* WRITE THE SYMBOLS */
	bfd_put_32(arch, symsize, &temp);
	bfd_write (&temp, 1, sizeof (temp), arch);
	prev_elt = arch->archive_head;
	armem_file_ptr = SARMAG + ARHDRSZ + mapsize + elength;
	for (i = 0; i < orl_count; i++) {
		struct symdef outs;
		bfd *cur_elt = (bfd *) map[i].pos;

		while (prev_elt != cur_elt) {
			armem_file_ptr += ARHDRSZ + arelt_size(prev_elt);
			armem_file_ptr += armem_file_ptr & 1;  /* Make even */
			prev_elt = prev_elt->next;
		}
		bfd_put_32(arch, map[i].namidx, &outs.s.string_offset);
		bfd_put_32(arch, armem_file_ptr, &outs.file_offset);
		bfd_write ((PTR)&outs, 1, sizeof(outs), arch);
	}

	/* WRITE THE STRING TABLE */
	if (padit) strsize++;
	bfd_put_32(arch, strsize, &temp);
	bfd_write ((PTR)&temp, 1, sizeof (temp), arch);
	for (i = 0; i < orl_count; i++) {
		char *name;
		name = *(map[i].name);
		bfd_write(name, 1, strlen(name)+1, arch);
	}

	if (padit) {
		bfd_write("\n",1,1,arch);
	}
	return true;
}

/* A coff armap looks like :
 * ARMAG
 * struct ar_hdr with name = '/'
 * number of symbols
 * offset of file for symbol 0
 * offset of file for symbol 1
 *    ..
 * offset of file for symbol n-1
 * symbol name 0
 * symbol name 1
 *    ..
 * symbol name n-1
 */
boolean
coff_write_armap (arch, elength, map, orl_count, strsize)
     bfd *arch;
     unsigned int elength;
     struct orl *map;
     int orl_count;
     int strsize;
{
	file_ptr armem_file_ptr;
	struct ar_hdr hdr;
	int bigendian_tmp;
	int i;
	char *p;
	bfd *cur_elt;
	bfd *prev_elt;

	unsigned int mapsize = strsize + (orl_count * 4) + 4;
	int padit = 0;

	if (!orl_count)
		return true;

	if ( mapsize & 1 ){
		padit = 1;
		mapsize ++;
	}

	memset ((char *)(&hdr), 0, ARHDRSZ);
	hdr.ar_name[0] = '/';
	sprintf (hdr.ar_size, "%-10d", (int) mapsize);

	if (arch->flags & SUPP_W_TIME)
		sprintf (hdr.ar_date, "%ld", 0);
	else if (arch->mtime_set)
		sprintf (hdr.ar_date, "%ld", arch->mtime);
	else
		sprintf (hdr.ar_date, "%ld", (long)time (NULL));

	hdr.ar_uid[0]  = '0';
	hdr.ar_uid[0]  = '0';
	hdr.ar_mode[0] = '0';
	hdr.ar_fmag[0] = '`'; 
	hdr.ar_fmag[1] = '\n';

	for (i = 0, p = (char *)&hdr; i < ARHDRSZ; i++, p++) {
		if (*p == '\0') {
			*p = ' ';
		}
	}

	/* Write the ar header for this item and the number of symbols.
	 *
	 * NOTE THAT REGARDLESS OF ENDIANESS OF THE ARCHIVE'S CONTENTS, THE
	 * BINARY VALUES IN A COFF ARMAP (# of symbols and file offsets) ARE
	 * ALWAYS STORED IN BIG-ENDIAN BYTE ORDER.  So use _do_putb32() to
	 * store them.
	 */
	bfd_write (&hdr, 1, ARHDRSZ, arch);
	_do_putb32 ( orl_count, (unsigned char *)&bigendian_tmp );
	bfd_write (&bigendian_tmp, 1, sizeof (bigendian_tmp), arch);

	/* Two passes, first write the file offsets for each symbol -
	 * remembering that each offset is on a two byte boundary
	 */
	prev_elt = arch->archive_head;
	armem_file_ptr = mapsize + elength + ARHDRSZ + SARMAG;
	for (i = 0; i < orl_count; i++) {
		bfd *cur_elt = (bfd *) map[i].pos;

		while (prev_elt != cur_elt) {
			armem_file_ptr += ARHDRSZ + arelt_size(prev_elt);
			armem_file_ptr += armem_file_ptr & 1;  /* Make even */
			prev_elt = prev_elt->next;
		}
		_do_putb32 (armem_file_ptr, (unsigned char *)&bigendian_tmp);
		bfd_write (&bigendian_tmp, 1, sizeof (bigendian_tmp), arch);
	}

	/* now write the strings themselves */
	for (i = 0; i < orl_count; i++) {
		bfd_write ((PTR)*((map[i]).name), 1,
					strlen(*((map[i]).name))+1, arch);
	}
	/* The spec sez this should be a newline.  But in order to be
	 * bug-compatible for arc960 we use a null.
	 */
	if (padit) {
		bfd_write("\0",1,1,arch);
	}
	return true;
}



