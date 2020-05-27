
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

/* COFF - to - MEM 
 * Andy Wilson, 9-Sep-86.
 * Max G. Webb, 14-Jul-86
 *	(stripping down for ROMBLD)
 *
 */

static char rcsid[] = "$Header: /ffs/p1/dev/src/rom960/common/RCS/p7mem.c,v 1.19 1995/08/28 21:31:44 paulr Exp $ $Locker:  $\n";

#if !defined(MSDOS) || defined(MWC) || defined(CBLD)
#define	myalloc(a)	malloc(a)
#define	MAXDRW		0x7FFFFFFF

# else
#include <assert.h>
#include <malloc.h>

#define	free(a)		hfree(a)
#define	myalloc(a)	halloc(a,1)

#define	MAXDRW		32767
# endif

/* system includes */
#include "rom960.h"
#include "p7mem.h"
#include "err_msg.h"
#include <setjmp.h>


extern jmp_buf parse_err;
extern unsigned long bytes_in_rom;

extern int debug;	/* turn debugging on/off */
char *err_msg;

#define EXIT_OK 0
#define EXIT_FAIL 1
#define COPYBUF 4096	/* size of buffer through which section data are copied
			from COFF file to image file */
#define S_PADDR_OFFSET 8	/* offset of the s_paddr field from the start
				of a struct scnhdr */

unsigned long	image_current;
unsigned long	calc_size();

FILE *open_output();
bfd *open_obj();
void byteswap();
extern char *copy();
extern bfd *new_bfd();

obj_to_mem(filename,outfile,fillchar,nl,ns)
char *filename; 
char *outfile;
char fillchar;
struct name_list *nl;
int ns;
{
    register int i;
    bfd *infile;
    FILE  *imagefile;
    sec_ptr *sects;
    int nsects,warned;
    unsigned long image_size,image_start,image_end;

    /*
     * Open and validate COFF file
     */

    if ((infile = open_obj(filename)) == NULL) {
	error_out(ROMNAME,NOT_OPEN,0,filename);
	longjmp(parse_err,1);
    }
    nsects = (ns == 0) ? bfd_count_sections(infile) : ns;
    sects = (sec_ptr *)malloc(nsects*sizeof(struct sec));
    if (sects == NULL) {
	error_out(ROMNAME,ALLO_MEM,0);
	longjmp(parse_err,1);
    }

    /*
     * Open output files
     */

    if ((imagefile = open_output(outfile)) == 0) {
	error_out(ROMNAME,NOT_OPEN,0,filename);
	longjmp(parse_err,1);
    }

    /*
     * Retrieve information for each COFF section and create
     *  header for .boot file if appropriate.
     */

    read_section_info(infile,nsects,sects,filename,nl);

    /* sort by start addr */
    sort_sections(nsects,sects);

    /* figure out how big the image is going to be */
    
    /* find the lowest-addressed section that will be in the image */
    i = 0;
    while ((i < nsects) &&
	   ((sects[i]->flags & SEC_IS_NOLOAD) ||
	    (0 == (sects[i]->flags & (SEC_CODE | SEC_DATA))) ||
	    (sects[i]->size <= 0)
	    )) i++;
    image_start = sects[i < nsects ? i : 0]->pma;

    /* find the highest-addressed section that will be in the image */
    i = nsects-1;
    while ((i>0) &&
	   ((sects[i]->flags & SEC_IS_NOLOAD) ||
	    (0 == (sects[i]->flags & (SEC_CODE | SEC_DATA))) ||
	    (sects[i]->size <= 0)
	    )) i--;
    image_end = sects[i >= 0 ? i : 0]->pma + sects[i >= 0 ? i : 0]->size - 1;

    /*
     * Fill the outfile because dos implied 0 writes dont work
     * This used to be #ifdef'ed out for DOS only, but with 'mkfill' 
     * it became usefull to designate the fill character, so the
     * tool may run a little slower on UNIX, but we get to specify
     * the fill character. 
     */

    if (1) {
	unsigned long	zsize;
	unsigned long	val = 0;
	char		zb[BUFSIZ];

	/*
	 *	calc fill size, last real section.paddr + last real
	 *	section.size - first real section.paddr
	 */

	zsize = image_end - image_start;
	for (val = BUFSIZ -1; val; --val)
		zb[val]=(char) fillchar;
	zb[val]=fillchar;

	for ( val = (long)(zsize / BUFSIZ); val; --val )
		if (fwrite(zb, BUFSIZ, 1, imagefile) != 1) {
		    fclose(imagefile);
		    unlink(outfile);
		    bfd_close(infile);
		    error_out(ROMNAME,ROM_NO_IMAGE_WRITE,0);
		    longjmp(parse_err,1);
		}
	for ( val = (long)(zsize % BUFSIZ); val; --val )
		if (fwrite(zb, 1, 1, imagefile) != 1) {
		    fclose(imagefile);
		    unlink(outfile);
		    bfd_close(infile);
		    error_out(ROMNAME,ROM_NO_IMAGE_WRITE,0);
		    longjmp(parse_err,1);
		}

	fseek(imagefile, 0L, 0);
    }
	
    /*
     * Write each section out, except for uninitialized data with -R
     */

#ifdef DEBUG
    printf("0x%x sections to copy\n",nsects);
#endif
    image_current = 0;
    image_size = 0;
    warned = 0;
    for (i=0; i < nsects; i++) {
	sec_ptr scn = sects[i];
	if ((!(scn->flags & SEC_IS_NOLOAD)) &&	
	    ((scn->flags & SEC_CODE) || (scn->flags & SEC_DATA))) {
	    if (scn->size) {
		image_size += (scn->size);
		/* if they told us how big ROM space was,
		   and the image-size is more than twice as
		   big, and we haven't warned them yet ... */
		if ((bytes_in_rom > 0) &&
		    (image_size > bytes_in_rom) &&
		    !warned) {
		    warn_out(ROMNAME,ROM_IMAGE_SIZE,0);
		    warned = 1;
		}
		copy_section(infile, imagefile, scn);
	    }
	}
    }

    /* clean up */

    free(sects);
    bfd_close(infile);
    fclose (imagefile);
}

/******************************************************************************
 * byteswap:
 *      Reverse the order of a string of bytes.
 *
 ******************************************************************************/
void
byteswap( p, n )
    char *p;    /* Pointer to source/destination string */
    int n;      /* Number of bytes in string            */
{
        int i;  /* Index into string                    */
        char c; /* Temp holding place during 2-byte swap*/
 
        for ( n--, i = 0; i < n; i++, n-- ){
                c = p[i];
                p[i] = p[n];
                p[n] = c;
        }
}

bfd *
open_obj(filename)
register char *filename;
{
	bfd *infile = NULL;
	register int nsects;
	
	infile = bfd_openrw(filename,(char *)NULL);
	if (!infile) {
		error_out(ROMNAME,NOT_OPEN,0,filename);
		longjmp(parse_err,1);
	}
	if (!bfd_check_format(infile,bfd_object)) { 
		bfd_close(infile);
		error_out(ROMNAME,ROM_NOT_OBJECT,0,filename);
		longjmp(parse_err,1);
	}
	return(infile);
}

FILE *
open_output(outfile)
register char *outfile;
{
	FILE *imagefile;

	if ((imagefile = fopen(outfile, FOPEN_WRONLY_TRUNC)) == 0) {
		error_out(ROMNAME,NOT_OPEN,0,outfile);
		longjmp(parse_err,1);
	}
	return(imagefile);
}


copy_section(infile,ifp,scn)
bfd *infile; /* COFF file */
FILE *ifp;	/* image file */
sec_ptr scn;
{
	unsigned long bytes_to_read, bytes_read;
	char transfer_buf[BUFSIZE];

	/* Using the filepos member of the section struction, position the
		input file to the start of the section raw data */
	if (bfd_seek(infile,scn->filepos,SEEK_SET)) {
		/* Seek failed; return error */
		copy_section_error(infile->filename,scn->name);
	}
	/* Using the calculation stolen from the late, lamented routine
		copy_mem_seg(), position the output file to the place to
		which the section raw data is to be written. Note that
		image_current tells where the image file thinks it is,
		not where it actually is. Thus, an image that starts at
		0x80000000 in memory will have an image_current that is
		0x80000000 greater than the actual image file offset, 
		for obvious reasons. */
	if (image_current) {
		if (fseek(ifp,(scn->pma)-image_current,SEEK_CUR))  {
			error_out(ROMNAME,ROM_PAD_FSEEK,0,"image file",
				scn->name);
			longjmp(parse_err,1);
		}
	}
	else { 	/* make sure ifp is at its beginning */
		if (fseek(ifp,0,SEEK_SET))  {
			error_out(ROMNAME,ROM_PAD_FSEEK,0,"image file",
				scn->name);
			longjmp(parse_err,1);
		}
	}
	image_current = scn->pma;

	/* Using the algorithm currently in copy_mem_seg, reading in and
		writing out a buffer-full of data at a time, copy the data
		from the input file to the output file */

	bytes_to_read = scn->size;

	while (bytes_read = bfd_read(transfer_buf, 1,
		min(BUFSIZE, bytes_to_read), infile)) {
		bytes_to_read -= bytes_read;
		if (fwrite (transfer_buf, 1, bytes_read, ifp) < bytes_read) {
			error_out(ROMNAME,ROM_NO_IMAGE_WRITE,0);
			longjmp(parse_err,1);
		}
		image_current += bytes_read;
	}
	return;
}

copy_section_error(filename,sectionname)
char *filename, *sectionname;
{

	/* grab enough space to write the file name, a ":", the section name,
	and a terminating NULL */
	err_msg = (char *) malloc(strlen(filename)+strlen(sectionname)+2);
	/* if we can't allocate space to store the whole error message
	string, we just won't tell which section we couldn't find */
	if (err_msg == NULL) 
		err_msg = filename;
	else
		sprintf(err_msg,"%s:%s",filename,sectionname);
	error_out(ROMNAME,NO_SECHD,0,err_msg);
	free(err_msg);
	longjmp(parse_err,1);
}


/*
 * Check if there are any section overlaps another section.
 */
static
check_sect_overlaps(objname)
char *objname;
{
        int nsects;
        sec_ptr *sects;
        sec_ptr prev_scn;
 
        unsigned long int prev_end_addr;
        bfd *infile;
        int i;
 
        if ((infile = open_obj(objname)) == NULL) {
                error_out(ROMNAME,NOT_OPEN,0,objname);
                longjmp(parse_err,1);
        }
        nsects = bfd_count_sections(infile);
        sects = (sec_ptr *)malloc(nsects*sizeof(sec_ptr));
        if (sects == NULL) {
                error_out(ROMNAME,ALLO_MEM,0);
                longjmp(parse_err,1);
        }
 
        /*
         * Retrieve information for each COFF section and create
         *  header for .boot file if appropriate.
         */
 
        read_section_info(infile,nsects,sects,objname,(struct name_list *)0);
 
        /* sort by start addr */
        sort_sections(nsects,sects);
        prev_end_addr = 0xffffffff;
        prev_scn = (sec_ptr )0;
 
        for (i=0; i < nsects; i++) {
            sec_ptr scn = sects[i];
            if ((!(scn->flags & SEC_IS_NOLOAD)) &&
		(scn->flags & (SEC_LOAD | SEC_IS_BSS))) {
                if (prev_end_addr != 0xffffffff && scn->size > 0 && prev_scn &&
                    scn->pma < prev_end_addr)
                        warn_out(ROMNAME,ROM_WARN_OVERLAP, 1, scn->name, prev_scn->name);
                prev_end_addr = scn->pma + scn->size;
                prev_scn = scn;
            }
        }
}


/*
 *	Move_section changes the "physical address" of a section in a
 *	COFF file.
 */

move_section(objname, section, addr, issue_hard_error, asectname)
	char *objname;			/* ->name of object file.		*/
	char *section;			/* ->name of section to move.	*/
	unsigned long addr;		/* New address.  If -1, section is
					     moved after .text.		*/
        int issue_hard_error;
    char *asectname;
{
    bfd *infile;			/* ->opened INPUT file.		*/
    asection *secthead;

    /* Open & check INPUT file.	*/
    if ((infile = open_obj(objname)) == NULL) {
	error_out(ROMNAME,NOT_OPEN,0,objname);
	longjmp(parse_err,1);
    }
    if (addr == -1 || addr == -2) {
	secthead = bfd_get_section_by_name(infile,asectname = (addr == -1) ? ".text" : asectname );
	if (!secthead) {
	    error_out(ROMNAME,ROM_READ_TEXT,0,asectname);
	    bfd_close(infile);
	    longjmp(parse_err,1);
	}
	addr = secthead->pma + secthead->size;
    }
    if (!(secthead = bfd_get_section_by_name(infile,section))) {
	err_msg = (char *) malloc(strlen(objname) + strlen(section)
				  + 2);
	if (err_msg == NULL)
		/* no space for error message, just tell them
		   which section, not which filename */
		err_msg = section;
	else
		sprintf(err_msg,"%s:%s",section,objname);
	error_out(ROMNAME,NO_SECHD,0,err_msg);
	free(err_msg);
	bfd_close(infile);
	longjmp(parse_err,1);
    }

    if (secthead->paddr_file_offset == BAD_PADDR_FILE_OFFSET) {
	if (issue_hard_error) {
	    err_msg = (char *) malloc(strlen(objname) + strlen(section)
				      + 2);
	    if (err_msg == NULL)
		    /* no space for error message, just tell them
		       which section, not which filename */
		    err_msg = section;
	    else
		    sprintf(err_msg,"%s:%s",section,objname);
	    error_out(ROMNAME,SECT_HAS_BAD_PADDR_FILE_OFFSET,0,err_msg);
	    free(err_msg);
	    bfd_close(infile);
	    longjmp(parse_err,1);
	}
	else
		return;
    }

    if (!bfd_seek(infile,secthead->paddr_file_offset,SEEK_SET)) {
	unsigned long byte_swapped_addr;

	bfd_h_put_32(infile,addr,&byte_swapped_addr);
	if (bfd_write(&byte_swapped_addr,1,sizeof(byte_swapped_addr),infile) == sizeof(byte_swapped_addr)) {
	    bfd_close(infile);
	    /* Check overlaps in the created image */
	    check_sect_overlaps(objname);
	    return;
	}
	else {
	    err_msg = (char *) malloc(strlen(objname) + strlen(section)
				  + 2);
	    if (err_msg == NULL)
		    /* no space for error message, just tell them
		       which section, not which filename */
		    err_msg = section;
	    else
		    sprintf(err_msg,"%s:%s",section,objname);
	    error_out(ROMNAME,FILE_WRITE_ERROR,0,err_msg);
	    free(err_msg);
	    bfd_close(infile);
	    longjmp(parse_err,1);
	}
    }
    else {
	err_msg = (char *) malloc(strlen(objname) + strlen(section)
				  + 2);
	if (err_msg == NULL)
		/* no space for error message, just tell them
		   which section, not which filename */
		err_msg = section;
	else
		sprintf(err_msg,"%s:%s",section,objname);
	error_out(ROMNAME,FILE_SEEK_ERROR,0,err_msg);
	free(err_msg);
	bfd_close(infile);
	longjmp(parse_err,1);
    }
}

map(s)
char *s; 
{
	register int i;
	bfd *infile;
	sec_ptr *sects;
	int nsects,flag;
	char *objname;			/* ->COFF file.			*/
	unsigned long int startaddr,endaddr,totalsize;
	char secname[9];

	objname = copy(get_file_name(&s, "object file to map:"));

	/*
	 * Open and validate COFF file
	 */

	if ((infile = open_obj(objname)) == NULL) {
		error_out(ROMNAME,NOT_OPEN,0,objname);
		longjmp(parse_err,1);
	}
	nsects = bfd_count_sections(infile);
	sects = (sec_ptr *)malloc(nsects*sizeof(sec_ptr));
	if (sects == NULL) {
		error_out(ROMNAME,ALLO_MEM,0);
		longjmp(parse_err,1);
	}

	/*
	 * Retrieve information for each COFF section and create
	 *  header for .boot file if appropriate.
	 */

	read_section_info(infile,nsects,sects,objname,(struct name_list *)0);

        /* sort by start addr */
        sort_sections(nsects,sects);
        printf("Section name  Physical address      Size\n");
 
        totalsize = 0;
        startaddr = 0xffffffff;
        endaddr = 0;
 
        for (i=0; i < nsects; i++) {
                sec_ptr scn = sects[i];
                char phys_buf[20];
                char size_buf[20];
 
                sprintf(phys_buf, "0x%x", scn->pma);
                sprintf(size_buf, "0x%x", scn->size);
                secname[0]='\0';
                strncat(&secname[0],scn->name,8);
                printf("%-8s         %10s     %10s\n",
                        &secname[0], phys_buf, size_buf);
                /* don't use .bss to calculate size */
                if ((!(scn->flags & SEC_IS_NOLOAD)) &&
                    ((scn->flags & SEC_CODE) || (scn->flags & SEC_DATA)) &&
                    (scn->pma < startaddr))
                        startaddr = scn->pma;
                if ((scn->flags & SEC_LOAD) &&
                    (!(scn->flags & SEC_IS_NOLOAD)) &&
                    ((scn->pma + scn->size - 1) > endaddr)) {
                    /* if pma+size will wrap-around, fix enddr to 0xffffffff */
                    if ((scn->pma + scn->size) <= 0) {
                        endaddr = 0xffffffff;
                        warn_out(ROMNAME,ROM_WARN_WRAP_ADDR,0);
                    }
                    else
                            endaddr = scn->pma + scn->size;
                }
        }
        totalsize = endaddr - startaddr;
        printf("Image made from %s will be %u (decimal) bytes long\n\n",objname,
                totalsize);
        /* if they told us how big ROM space should be and this image
        would be more than that big, warn them */
        if (bytes_in_rom != 0)
                if (totalsize > bytes_in_rom)
                        warn_out(ROMNAME,ROM_IMAGE_SIZE,0);
 
        /* clean up */
 
        free(sects);
        bfd_close (infile);
}
