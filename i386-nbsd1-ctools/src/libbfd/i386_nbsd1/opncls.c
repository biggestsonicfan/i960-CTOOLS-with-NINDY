/* opncls.c -- open and close a bfd. */

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
#include "dio.h"

#if HOST_SYS == AIX_SYS || HOST_SYS == DEC3100_SYS
#	include <stdlib.h>
#endif

extern void bfd_cache_init();
FILE *bfd_open_file();

/* Locking 
 *
 * Locking is loosely controlled by the preprocessor variable
 * BFD_LOCKS.  I say loosely because Unix barely understands locking
 * -- at least in BSD it doesn't affect programs which don't
 * explicitly use it!  That is to say it's practically useless, though
 * if everyone uses this library you'll be OK.
 *
 * From among the many and varied lock facilities available, (none of
 * which, of course, knows about any other) we use the fcntl locks,
 * because they're Posix.
 *
 * The reason that bfd_openr and bfd_fdopenr exist, yet only bfd_openw
 * exists is because of locking.  When we do output, we lock the
 * filename file for output, then open a temporary file which does not
 * actually get its correct filename until closing time.  This is
 * safest, but requires the asymmetry in read and write entry points.
 */

#ifdef DOS

/* For information about this code, please see the linker's source file:
   ld960sym.c.  Search for bfd_close_all().  */

static struct bfd_list_node {
    bfd *b;
    struct bfd_list_node *next;
} *bfd_list,**bottom_of_bfd_list=&bfd_list;

#endif

/* Return a new BFD.  All BFD's are allocated through this routine.  */

bfd *new_bfd()
{
	bfd *nbfd;

	nbfd = (bfd *)bfd_zalloc ((bfd*)0,sizeof (bfd));
	if (!nbfd){
		return 0;
	}
	nbfd->direction = no_direction;
	nbfd->iostream = NULL;
	nbfd->where = 0;
	nbfd->sections = (asection *)NULL;
	nbfd->format = bfd_unknown;
	nbfd->my_archive = (bfd *)NULL;
	nbfd->origin = 0;
	nbfd->opened_once = false;
	nbfd->output_has_begun = false;
	nbfd->section_count = 0;
	nbfd->usrdata = (PTR)NULL;
	nbfd->sections = (asection *)NULL;
	nbfd->cacheable = false;
	nbfd->flags = NO_FLAGS;
	nbfd->mtime_set = 0;
	return nbfd;
}

/* Allocate a new BFD as a member of archive OBFD.  */
bfd *
new_bfd_contained_in(obfd)
    bfd *obfd;
{
	bfd *nbfd = new_bfd();
	nbfd->xvec = obfd->xvec;
	nbfd->my_archive = obfd;
	nbfd->direction = read_direction;
	return nbfd;
}

/* bfd_openr, bfd_fdopenr -- open for reading.
 * Returns a pointer to a freshly-allocated bfd on success, or NULL.
 */
static bfd *
DEFUN(_bfd_opener, (filename, target, direction),
CONST char *filename AND
CONST char *target   AND
CONST enum bfd_direction direction)
{
	bfd *nbfd;
	bfd_target *target_vec;

	nbfd = new_bfd();
	if (nbfd == NULL) {
		bfd_error = no_memory;
		return NULL;
	}

	target_vec = bfd_find_target (target, nbfd);
	if (target_vec == NULL) {
		bfd_error = invalid_target;
		return NULL;
	}

	nbfd->filename = filename;
	nbfd->direction = direction;

	if (bfd_open_file (nbfd) == NULL) {
		bfd_error = system_call_error;
		bfd_release((bfd*)0,nbfd);
		return (bfd *) 0;
	}
	return nbfd;
}

bfd *
DEFUN(bfd_openr, (filename, target),
CONST char *filename AND
CONST char *target)
{
    return _bfd_opener(filename,target,read_direction);
}

bfd *
DEFUN(bfd_openrw, (filename, target),
CONST char *filename AND
CONST char *target)
{
    return _bfd_opener(filename,target,both_direction);
}

/* Don't try to `optimize' this function:
 *
 * o We lock using stack space so that interrupting the locking
 *   won't cause a storage leak.
 *
 * o We open the file stream last, since we don't want to have to
 *   close it if anything goes wrong.  Closing the stream means closing
 *   the file descriptor too, even though we didn't open it.
 */
bfd *
DEFUN(bfd_fdopenr,(filename, target, fd),
CONST char *filename AND
CONST char *target AND
int fd)
{
	bfd *nbfd;
	bfd_target *target_vec;
#ifdef BFD_LOCKS
	struct flock lock, *lockp = &lock;
#endif

	bfd_error = system_call_error;

#ifdef BFD_LOCKS
	lockp->l_type = F_RDLCK;
	if (fcntl (fd, F_SETLKW, lockp) == -1) return NULL;
#endif

	nbfd = new_bfd();

	if (nbfd == NULL) {
		bfd_error = no_memory;
		return NULL;
	}

	target_vec = bfd_find_target (target, nbfd);
	if (target_vec == NULL) {
		bfd_error = invalid_target;
		return NULL;
	}

#ifdef BFD_LOCKS
	nbfd->lock = (struct flock *) (nbfd + 1);
#endif
	nbfd->iostream = (char *) fdopen (fd, FRDBIN);
	if (nbfd->iostream == NULL) {
		return NULL;
	}

	nbfd->filename = filename;
	nbfd->xvec = target_vec;
	nbfd->direction = read_direction;

#ifdef BFD_LOCKS
	memcpy (nbfd->lock, lockp, sizeof (struct flock))
#endif
	bfd_cache_init (nbfd);
	return nbfd;
}

/* bfd_openw -- open for writing.
 * Returns a pointer to a freshly-allocated bfd on success, or NULL.
 *
 * See comment by bfd_fdopenr before you try to modify this function.
 */
bfd *
DEFUN(bfd_openw,(filename, target),
    CONST char *filename AND
    CONST char *target)
{
	bfd *nbfd;
	bfd_target *target_vec;

	bfd_error = system_call_error;

	nbfd = new_bfd();
	if (nbfd == NULL) {
		bfd_error = no_memory;
		return NULL;
	}

	target_vec = bfd_find_target (target, nbfd);
	if (target_vec == NULL) return NULL;

	nbfd->filename = filename;
	nbfd->direction = write_direction;

	if (bfd_open_file (nbfd) == NULL) {
		bfd_error = system_call_error;	/* File not writeable, etc */
		return NULL;
	}
	return nbfd;
}

#ifndef S_IXUSR
#define S_IXUSR 0100	/* Execute by owner.  */
#endif
#ifndef S_IXGRP
#define S_IXGRP 0010	/* Execute by group.  */
#endif
#ifndef S_IXOTH
#define S_IXOTH 0001	/* Execute by others.  */
#endif

/* Close up shop, get your deposit back. */
boolean
bfd_close (abfd)
    bfd *abfd;
{
	if (((abfd->direction == write_direction) ||
	     ((abfd->direction == both_direction) && (abfd->flags & WRITE_CONTENTS))) &&
	    ((abfd->format != bfd_unknown) && !BFD_SEND_FMT(abfd,_bfd_write_contents,(abfd)))) {
	    return false;
	}
	if ( !BFD_SEND(abfd, _close_and_cleanup, (abfd))) {
		return false;
	}

	bfd_cache_close(abfd);

	/* If the file was open for writing and is now executable, make it so */
	if (abfd->direction == write_direction && abfd->flags & EXEC_P) {
		struct stat buf;
		stat(abfd->filename, &buf);
		chmod(abfd->filename,buf.st_mode|S_IXUSR|S_IXGRP|S_IXOTH);
	}
	return true;
}

/* Create a bfd with no associated file or target.
 */
bfd *
DEFUN(bfd_create,(filename, template),
    CONST char *filename AND
    CONST bfd *template)
{
	bfd *nbfd = new_bfd();
	if (nbfd == (bfd *)NULL) {
		bfd_error = no_memory;
		return (bfd *)NULL;
	}
	nbfd->filename = filename;
	if(template) {
		nbfd->xvec = template->xvec;
	}
	nbfd->direction = no_direction;
	bfd_set_format(nbfd, bfd_object);
	return nbfd;
}

/* FILE DESCRIPTOR CACHE
 *	Allows you to have more bfds open than your system has fds.
 */

/* The maximum number of FDs opened by bfd */
#define BFD_CACHE_MAX_OPEN 10

/* when this exceeds BFD_CACHE_MAX_OPEN, we get to work */
static int open_files;

static bfd *cache_sentinel;	/* Chain of bfds with active fds we've opened */

bfd *bfd_last_cache;		/* Zero, or a pointer to the topmost
				 * bfd on the chain.  This is used by the
				 * bfd_cache_lookup() macro in libbfd.h to
				 * determine when it can avoid a function call.
				 */
static void bfd_cache_delete();

static void
DEFUN_VOID(close_one)
{
	bfd *kill = cache_sentinel;

	if (kill == 0){		/* Nothing in the cache */
		return;
	}

	/* We can only close files that want to play this game */
	while (!kill->cacheable) {
		kill = kill->lru_prev;
		if (kill == cache_sentinel){
			return;
		}
	}
	kill->where = ftell((FILE *)(kill->iostream));
	bfd_cache_delete(kill);
}

/* Cuts the bfd abfd out of the chain in the cache */
static void 
DEFUN(snip,(abfd),
    bfd *abfd)
{
	abfd->lru_prev->lru_next = abfd->lru_next;
	abfd->lru_next->lru_prev = abfd->lru_prev;
	if (cache_sentinel == abfd){
		cache_sentinel = (bfd *)NULL;
	}
}

static void
DEFUN(bfd_cache_delete,(abfd),
    bfd *abfd)
{
	fclose ((FILE *)(abfd->iostream));
	snip (abfd);
	abfd->iostream = NULL;
	open_files--;
	bfd_last_cache = 0;
}

static bfd *
DEFUN(insert,(x,y),
    bfd *x AND
    bfd *y)
{
	if (y) {
		x->lru_next = y;
		x->lru_prev = y->lru_prev;
		y->lru_prev->lru_next = x;
		y->lru_prev = x;

	} else {
		x->lru_prev = x;
		x->lru_next = x;
	}
	return x;
}

/* Initialize a BFD by putting it on the cache LRU.  */
void
DEFUN(bfd_cache_init,(abfd),
    bfd *abfd)
{
	cache_sentinel = insert(abfd, cache_sentinel);
}

void
DEFUN(bfd_cache_close,(abfd),
    bfd *abfd)
{
	/* If this file is open then remove from the chain */
	if (abfd->iostream) {
		bfd_cache_delete(abfd);
	}
}
 
/* Call the OS to open a file for this BFD.  Returns the FILE *
 * (possibly null) that results from this operation.  Sets up the
 * BFD so that future accesses know the file is open.
 */
FILE *
DEFUN(bfd_open_file, (abfd),
bfd *abfd)
{
	abfd->cacheable = true;	/* Allow it to be closed later. */
	if(open_files >= BFD_CACHE_MAX_OPEN) {
		close_one();
	}
	switch (abfd->direction) {
	case read_direction:
	case no_direction:
		abfd->iostream = (char *) fopen(abfd->filename, FRDBIN);
		break;
	case both_direction:
		if (abfd->opened_once == true) {
		    abfd->iostream = (char *)fopen(abfd->filename,FRDPBIN);
		    if (!abfd->iostream) {
			abfd->iostream = (char *)
				fopen(abfd->filename,FWRTPBIN);
		    }
		}
		else {
		    abfd->iostream = (char *)fopen(abfd->filename,FRDPBIN);
		    if (!abfd->iostream) {
			/*open for creat */
			abfd->iostream = (char *)
				fopen(abfd->filename,FWRTPBIN);
		    }
		    abfd->opened_once = true;
		}
		break;
	case write_direction:
		if (abfd->opened_once == true) {
			abfd->iostream = (char *)fopen(abfd->filename,FRDPBIN);
			if (!abfd->iostream) {
				abfd->iostream = (char *)
						fopen(abfd->filename,FWRTPBIN);
			}
		} else {
			/*open for creat */
			abfd->iostream = (char*)fopen(abfd->filename,FWRTBIN);
			abfd->opened_once = true;
		}
		break;
	}

	if (abfd->iostream) {
		open_files++;
		bfd_cache_init (abfd);
#ifdef DOS
		if (1) {
		    /* For information about this code, please see the linker's source file:
		       ld960sym.c.  Search for bfd_close_all().  */

		    struct bfd_list_node *p;

		    if (!(p = (*bottom_of_bfd_list) = bfd_zalloc((bfd*)0,sizeof(struct bfd_list_node)))) {
			return 0;
		    }
		    bottom_of_bfd_list = &(p->next);
		    p->next = (struct bfd_list_node *) 0;
		    p->b = abfd;
		}
#endif
	    }
	return (FILE *)(abfd->iostream);
}

/* Find a file descriptor for this BFD.  If necessary, open it.
 * If there are already more than BFD_CACHE_MAX_OPEN files open, try to close
 * one first, to avoid running out of file descriptors.
 */
FILE *
DEFUN(bfd_cache_lookup_worker,(abfd),
    bfd *abfd)
{
	if (abfd->my_archive) {
		abfd = abfd->my_archive;
	}

	/* If this file already open .. quick exit */
	if (abfd->iostream) {
		if (abfd != cache_sentinel) {
			/* Place onto head of lru chain */
			snip (abfd);
			cache_sentinel = insert(abfd, cache_sentinel);
		}
	} else {
		/* This is a bfd without a stream -
		 * so it must have been closed or never opened.
		 * find an empty cache entry and use it.
		 */
		if (open_files >= BFD_CACHE_MAX_OPEN) {
			close_one();
		}
		BFD_ASSERT(bfd_open_file (abfd) != (FILE *)NULL) ;
		fseek((FILE *)(abfd->iostream), abfd->where, false);
	}
	bfd_last_cache = abfd;
	return (FILE *)(abfd->iostream);
}

#ifdef DOS

/* For information about this code, please see the linker's source file:
   ld960sym.c.  Search for bfd_close_all().  */

bfd_close_all()
{
    while (bfd_list) {
	struct bfd_list_node *p = bfd_list;

	if (bfd_list->b->iostream) {
	    bfd_list->b->where = ftell((FILE *)(bfd_list->b->iostream));
	    fclose(bfd_list->b->iostream);
	    snip(bfd_list->b);
	    bfd_list->b->iostream = NULL;
	    open_files--;
	}
	bfd_list = bfd_list->next;
	bfd_release((bfd *)0,p);
    }
    bottom_of_bfd_list = &bfd_list;
    bfd_last_cache = 0;
}
#endif
