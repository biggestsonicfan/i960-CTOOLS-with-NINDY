/*** gnu960.c -- things that are only relevant to an Intel GNU/960 release */

/* Copyright (C) 1990, 1991 Free Software Foundation, Inc.

This file is part of BFD, the Binary File Diddler.

BFD is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

BFD is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with BFD; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */


#include "sysdep.h"
#include "bfd.h"
#include "cc_info.h"


/*****************************************************************************
 * Produce name of the target with the specified "flavour" (file format) and
 * and the specified header byte order.  Return pointer to new target name.
 *
 * Return a bogus name if a bogus format is input.
 *****************************************************************************/
char *
#if !defined(NO_BIG_ENDIAN_MODS)
bfd_make_targ_name( format, host_big_endian, target_big_endian )
    enum target_flavour_enum format;
    int host_big_endian;
    int target_big_endian;
#else
bfd_make_targ_name( format, big_endian )
    enum target_flavour_enum format;
    int big_endian;
#endif /* NO_BIG_ENDIAN_MODS */
{
	switch ( format ){

	case BFD_COFF_FORMAT:
#if !defined(NO_BIG_ENDIAN_MODS)
                if (target_big_endian)
                        return host_big_endian ? BFD_BIG_BIG_COFF_TARG
                                               : BFD_LITTLE_BIG_COFF_TARG ;
                else
                        return host_big_endian ? BFD_BIG_COFF_TARG
                                               : BFD_LITTLE_COFF_TARG ;
#else
                return big_endian ? BFD_BIG_COFF_TARG : BFD_LITTLE_COFF_TARG ;
#endif /* NO_BIG_ENDIAN_MODS */

	case BFD_BOUT_FORMAT:
#if !defined(NO_BIG_ENDIAN_MODS)
                /* note: target_big_endian ignored for b.out files */
                return host_big_endian ? BFD_BIG_BOUT_TARG : BFD_LITTLE_BOUT_TARG ;
#else
                return big_endian ? BFD_BIG_BOUT_TARG : BFD_LITTLE_BOUT_TARG ;
#endif /* NO_BIG_ENDIAN_MODS */

	case BFD_ELF_FORMAT:
		return host_big_endian ? BFD_BIG_ELF_TARG : BFD_LITTLE_ELF_TARG;
	default:
		return "bogus";

	}
}

/*****************************************************************************
 * The following support GNU/960 2-pass compiler optimization info (ccinfo)
 * stored in object files.  The info, if present, is stored after the normal
 * end of the file, i.e., after the symbol and (if any) string tables.
 * It is always stripped if/when the symbol table is stripped.
 *****************************************************************************/

/* This is a pointer to a routine to be called to actually write the ccinfo.
 * We use a callback rather than a pointer because the ccinfo coming out of
 * the linker can be enormous;  the callback routine can generate/write it
 * on the fly rather than allocating yet another god-awful buffer.
 */
static int (*ccinfo_callback)();

/* Application calls this routine to notify us that output file will contain
 * ccinfo and to tell us where the callback is that will actually write it.
 */
void
bfd960_set_ccinfo( abfd, callback )
    bfd *abfd;
    int (*callback)();
{
	ccinfo_callback = callback;
	bfd_get_file_flags(abfd) |= HAS_CCINFO;
}


/* Assumes output pointer already positioned immediately past symbol and
 * string tables.  Called internally by BFD.
 */
int
bfd960_write_ccinfo( abfd )
    bfd *abfd;
{
	static int coff_delimiter = -1;

	if ( (bfd_get_file_flags(abfd) & HAS_CCINFO)
	&&   (ccinfo_callback != NULL) ){
		if ( BFD_COFF_FILE_P(abfd) && (abfd->flags & HAS_RELOC))
			bfd_write(&coff_delimiter,4,1,abfd);
		(*ccinfo_callback)(abfd);
	}
	return 1;
}


/* Called by application to position file pointer at start of ccinfo in an
 * input file.  Returns:
 *	 0 if no ccinfo in file
 *	-1 on error
 *	otherwise # bytes of ccinfo in file
 */
int
bfd960_seek_ccinfo( abfd )
    bfd *abfd;
{
    char ccinfo_len[4];
    int len;

    if (!(bfd_get_file_flags(abfd) & HAS_CCINFO))
	    return 0;

    if ( BFD_COFF_FILE_P(abfd) ) {
	if ( !icoff_seek_ccinfo(abfd) )
		return -1;
    }
    else if (BFD_BOUT_FILE_P(abfd)) {
	if ( !bout_seek_ccinfo(abfd) )
		return -1;
    } 
    else if (BFD_ELF_FILE_P(abfd)) {
	if ( !elf_seek_ccinfo(abfd) )
		return -1;
    }
    else
	    return -1;

    if ( (bfd_seek(abfd,CI_HEAD_TOT_SIZE_OFF,SEEK_CUR) < 0)
	||   (bfd_read(ccinfo_len,1,4,abfd) != 4)
	||   (bfd_seek(abfd,-(CI_HEAD_TOT_SIZE_OFF+4),SEEK_CUR) < 0) ){
	bfd_error = system_call_error;
	return -1;
    }
    CI_U32_FM_BUF(ccinfo_len,len);
    return len;
}

#if !defined(NO_BIG_ENDIAN_MODS)
#include "coff.h"

static short host_big_endian = 0x0100;
#define HOST_BIG_ENDIAN (*(char *)&host_big_endian != 0)

/*
 * This function returns non-zero if name is a COFF
 * file built to run on a big-endian i960 target.
 */

int
is_for_a_big_endian_target(name)
char *name;
{
   bfd *abfd;

   abfd = bfd_openr(name, (char *)0);

   if (BFD_COFF_FILE_P(abfd)) {
      struct filehdr f;
      FILE *fd;

      fd = fopen(name, "rb");
      fread(&f, 1, sizeof(f), fd);

      if (( BFD_BIG_ENDIAN_FILE_P(abfd) && !HOST_BIG_ENDIAN)
      ||  (!BFD_BIG_ENDIAN_FILE_P(abfd) &&  HOST_BIG_ENDIAN))
         /*
          * swap the f_flags since the host endianness doesn't
          * match the coff header endianness (ie, the file was
          * built on a host with the opposite endianness)
          */
         f.f_flags = ((f.f_flags & 0xff) << 8) | ((f.f_flags >> 8) & 0xff);

      fclose(fd);
      bfd_close(abfd);

      return(f.f_flags & F_BIG_ENDIAN_TARGET);
   }

   /* not a COFF file - fail. */
   bfd_close(abfd);
   return 0;
}
#endif /* NO_BIG_ENDIAN_MODS */
