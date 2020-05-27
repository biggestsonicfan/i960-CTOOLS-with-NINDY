
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

static char rcsid[] = "$Id: coffdmp.c,v 1.36 1995/11/17 12:01:47 peters Exp $";

/* This file contains routines specific to coff-format files
 */

#include <stdio.h>
#if !defined(WIN95) && !defined(__HIGHC__) && !defined(APOLLO_400)  /* Microsoft C 9.00 (Windows 95), Metaware C or Apollo 400 systems*/
#include <unistd.h>
#endif
#include "bfd.h"
#include "coff.h"
#include "gdmp960.h"

/* The following externs are declared in gdmp960.c */
extern bfd 	*abfd;		/* BFD descriptor of object file */
extern char 	flagseen [];	/* Command-line options */
extern special_type	*special_sect;	/* Dump only this one section */

/*
 * printf format strings
 */
static char	*coff_rel_hdr_fmt = "%9s%14s           %s\n\n";
static char 	*coff_rel_hex_fmt = "%4s%08x       %-15s%s\n";
static char 	*coff_rel_oct_fmt = "  %011o      %-15s%s\n";
static char 	*coff_fh_hdr_fmt = "Magic      Nscns     Time/Date          Symptr      Nsyms Opthdr  Flags\n\n";
static char	*coff_fh_line1_fmt = "0x%04x      % 2d  %s  0x%08x    % 4d  0x%04x  0x%04x\n";
static char 	*coff_fh_line2_fmt = "%66s%s\n";
static char	*coff_fh_unknown_flag_fmt = "0x%04x";
static char	*coff_oh_hdr_line1_fmt = "\tMagic       Vstamp      Tsize       Dsize       Bsize\n";
static char	*coff_oh_hdr_line2_fmt = "\t            Entry       Tstart      Dstart      Tag\n\n";
static char	*coff_oh_line1_fmt = "\t0x%04x    0x%04x      0x%08x  0x%08x  0x%08x\n";
static char	*coff_oh_line2_fmt = "\t          0x%08x  0x%08x  0x%08x  0x%08x\n";
static char	*coff_sh_hdr_line1_fmt = "    Name        Paddr       Vaddr       Scnptr      Relptr      Lnnoptr\n";
static char	*coff_sh_hdr_line2_fmt = "                Align       Size        Nreloc      Nlnno       Flags\n";
static char	*coff_sh_line1_fmt = "\n    %-10s  0x%08x  0x%08x  0x%08x  0x%08x  0x%08x\n";
static char	*coff_sh_line2_fmt = "                0x%08x  0x%08x  %7d     %7d     %s\n";
static char	*coff_sh_flag_fmt = "%s";
static char	*coff_sh_unknown_flag_fmt = "0x%02x";
static char	*coff_strtab_hdr_fmt = "%10s        %s\n\n";
static char	*coff_strtab_fmt =     "%10d        %s\n";
extern char	*sec_name_fmt;



struct scnhdr *
get_coff_hdrs( f, a )
    struct filehdr *f;	/* Put file header here			*/
    struct aouthdr *a;	/* Put optional a.out header here	*/
{
	struct scnhdr *shdrs;	/* Malloc array of section headers here	*/
	struct scnhdr *sP;	/* Pointer to an entry in shdrs array	*/
	int shdr_size;		/* Size of shdrs array, in bytes	*/
	unsigned short i;			/* Loop counter	*/

	xseek( 0 );

	/* Read file header, put it into host byte order
	 */
	xread( (char *) f, sizeof(*f) );
        if ( host_is_big_endian != file_is_big_endian ){
		BYTESWAP( f->f_magic  );
		BYTESWAP( f->f_nscns  );
		BYTESWAP( f->f_timdat );
		BYTESWAP( f->f_symptr );
		BYTESWAP( f->f_nsyms  );
		BYTESWAP( f->f_opthdr );
		BYTESWAP( f->f_flags  );
	}

	/* If there's an "optional" aout header:  read it, put into host
	 * byte order.
	 */
	if ( f->f_opthdr ){
		xread( (char *) a, sizeof(*a) );
		if ( host_is_big_endian != file_is_big_endian ){
			BYTESWAP( a->magic );
			BYTESWAP( a->vstamp );
			BYTESWAP( a->tsize );
			BYTESWAP( a->dsize );
			BYTESWAP( a->bsize );
			BYTESWAP( a->entry );
			BYTESWAP( a->text_start );
			BYTESWAP( a->data_start );
			BYTESWAP( a->tagentries );
		}
	}

	/* Read in ALL the section headers.
	 */
	shdr_size = f->f_nscns * sizeof(struct scnhdr);
	shdrs = (struct scnhdr *) xmalloc( shdr_size ); 
	xread( (char *) shdrs, shdr_size );
	if ( host_is_big_endian != file_is_big_endian ){
		for ( i=0; i < f->f_nscns; i++ ){
			sP = &shdrs[i];
        		BYTESWAP( sP->s_paddr   );
        		BYTESWAP( sP->s_vaddr   );
        		BYTESWAP( sP->s_size    );
        		BYTESWAP( sP->s_scnptr  );
        		BYTESWAP( sP->s_relptr  );
        		BYTESWAP( sP->s_lnnoptr );
        		BYTESWAP( sP->s_nreloc  );
        		BYTESWAP( sP->s_nlnno   );
        		BYTESWAP( sP->s_flags   );
        		BYTESWAP( sP->s_align   );

		}
	}
	return shdrs;
}



dmp_coff_file_hdr()
{
	struct filehdr 	f;	/* File header				*/
	struct aouthdr 	a;	/* Optional a.out header		*/
	struct scnhdr 	*shdrs;	/* Array of section headers		*/
	char 		*date;
	extern char 	*ctime();

	shdrs = get_coff_hdrs( &f, &a );
	free (shdrs);		/* memory gets allocated even if you don't need it */

	/* Read/print file header.
	 *	First get date from header, deleting leading day of week and
	 *	trailing newline.
	 */
	if (f.f_timdat == 0)  {
		date = "Dec 31 16:00:00 1969"; /* epoch time in most systems */
	}
	else {
		date = ctime(&f.f_timdat);

		while ( *date != ' ' ){ date++; }
		while ( *date == ' ' ){ date++; }
		date[strlen(date)-1] = 0;
	}

	if ( ! flagseen['p'] )
	{
		bfd_center_header( "FILE HEADER" );
		printf( coff_fh_hdr_fmt );
	}
	printf( coff_fh_line1_fmt, f.f_magic, f.f_nscns, date, f.f_symptr, f.f_nsyms, f.f_opthdr, f.f_flags);

	switch ( f.f_flags & F_I960TYPE )
	{
	case F_I960CORE:  printf( coff_fh_line2_fmt, "", "CORE" );  break;
  	case F_I960CORE2: printf( coff_fh_line2_fmt, "", "CORE2" ); break;
	case F_I960KA:	  printf( coff_fh_line2_fmt, "", "KA" );    break;
	case F_I960KB:	  printf( coff_fh_line2_fmt, "", "KB" );    break;
	case F_I960CA:	  printf( coff_fh_line2_fmt, "", "CA" );    break;
	case F_I960JX:	  printf( coff_fh_line2_fmt, "", "JX" );    break;
	case F_I960HX:	  printf( coff_fh_line2_fmt, "", "HX" );    break;
	default:
		break;
	}

	if ( f.f_flags & F_RELFLG ) printf( coff_fh_line2_fmt, "", "NO_RELOC" ); 
	if ( f.f_flags & F_EXEC   ) printf( coff_fh_line2_fmt, "", "EXEC" ); 
	if ( f.f_flags & F_LNNO   ) printf( coff_fh_line2_fmt, "", "NO_LNNO" ); 
	if ( f.f_flags & F_LSYMS  ) printf( coff_fh_line2_fmt, "", "NO_LSYMS" ); 
	if ( f.f_flags & F_COMP_SYMTAB ) printf( coff_fh_line2_fmt, "", "COMP_SYMTAB" ); 
	if ( f.f_flags & F_PIC    ) printf( coff_fh_line2_fmt, "", "PIC" ); 
	if ( f.f_flags & F_PID    ) printf( coff_fh_line2_fmt, "", "PID" ); 
	if ( f.f_flags & F_LINKPID) printf( coff_fh_line2_fmt, "", "LINKPID" ); 
	if ( f.f_flags & F_CCINFO)  printf( coff_fh_line2_fmt, "", "CCINFO" ); 
        if ( f.f_flags & F_AR32WR ) printf( coff_fh_line2_fmt, "", "AR32WR" );
        if ( f.f_flags & F_BIG_ENDIAN_TARGET)
                printf( coff_fh_line2_fmt, "", "BIG_ENDIAN_T" );
	return 1; /* success */
} /* dmp_coff_file_hdr() */


int dmp_coff_optional_hdr()
{
	struct filehdr f;	/* File header				*/
	struct aouthdr a;	/* Optional a.out header		*/
	struct scnhdr *shdrs;	/* Array of section headers		*/

	shdrs = get_coff_hdrs( &f, &a );
	free (shdrs);		/* memory gets allocated even if you don't need it */

	/* Print the optional aout header only if there is one.
	 */
	if ( f.f_opthdr )
	{
		if ( ! flagseen['p'] )
		{
			bfd_center_header("OPTIONAL HEADER");
			printf( coff_oh_hdr_line1_fmt );
			printf( coff_oh_hdr_line2_fmt );
		}
		printf(coff_oh_line1_fmt, 
		       a.magic, a.vstamp, a.tsize, a.dsize, a.bsize );
		printf(coff_oh_line2_fmt,
		       a.entry,a.text_start,a.data_start,a.tagentries);
	}
	else
	{
		error ("\nNo optional header information available.\n");
		return 0;
	}
	return 1;
}


dmp_coff_section_hdrs()
{
	struct filehdr 	f;	/* File header				*/
	struct aouthdr 	a;	/* Optional a.out header		*/
	struct scnhdr 	*shdrs;	/* Array of section headers		*/
	struct scnhdr 	*sP;	/* Pointer to an entry in shdrs array	*/
	int 		i;
	char 		flag_str[64];
	char		secname[10];

	shdrs = get_coff_hdrs( &f, &a );

	/* Print the section headers.
	 */
	if ( ! flagseen['p'] )
	{
		bfd_center_header( "SECTION HEADERS" );
		printf( coff_sh_hdr_line1_fmt );
		printf( coff_sh_hdr_line2_fmt );
	}
	for ( sP=shdrs; f.f_nscns--; sP++ )
	{
		if ( flagseen['n'] && strcmp (sP->s_name, special_sect->name) )
			/* User asked for only one section to be printed. */
			continue;

		switch ( sP->s_flags & 0x001f )
		{
		case STYP_REG:    strcpy( flag_str, "REG");	break;
		case STYP_DSECT:  strcpy( flag_str, "DSECT");	break;
		case STYP_NOLOAD: strcpy( flag_str, "NOLOAD");	break;
		case STYP_GROUP:  strcpy( flag_str, "GROUP");	break;
		case STYP_PAD:    strcpy( flag_str, "PAD");	break;
		case STYP_COPY:   strcpy( flag_str, "COPY");	break;

		default:
			sprintf( flag_str, coff_sh_unknown_flag_fmt, sP->s_flags & 0x001f );
			break;
		}

		if ( sP->s_flags & STYP_TEXT )
			strcat( flag_str, ", TEXT" );

		if ( sP->s_flags & STYP_DATA )
			strcat( flag_str, ", DATA" );

		if ( sP->s_flags & STYP_BSS )
			strcat( flag_str, ", BSS" );

		if ( sP->s_flags & STYP_INFO )
			strcat( flag_str, ", INFO" );
		
		strncpy (secname, sP->s_name, 8);
		secname[8] = 0;

		printf(coff_sh_line1_fmt, 
		       secname, sP->s_paddr, sP->s_vaddr, sP->s_scnptr,
		       sP->s_relptr, sP->s_lnnoptr );
		printf(coff_sh_line2_fmt, 
		       sP->s_align, sP->s_size, sP->s_nreloc, sP->s_nlnno, flag_str );
	}
	free( shdrs );
	return 1;
}


/* Dump the relocation directives for a section from a coff file.
 * Coff relocation directives have only 3 pieces of information
 * (section address, symbol number, and relocation type) all of
 * which are more or less preserved by the bfd routines.  So rather
 * than go through the trouble of reading/swapping the coff symbol
 * table and relocation tables, we just use the bfd routines here.
 */
static void
dmp_sec_reloc(abfd, sec, sympp)
bfd *abfd;	/* bfd descriptor of object file */
sec_ptr sec;	/* pointer to bfd section descriptor */
asymbol **sympp;
{
	arelent **relpp;
	int relcount;
	char *type;
	int i;
	unsigned long secaddr;
	int reloc_bytes;

	printf( sec_name_fmt, bfd_section_name(abfd,sec), "" );
	abfd->flags |= DO_NOT_ALTER_RELOCS;
	if ( (reloc_bytes = get_reloc_upper_bound(abfd,sec)) > 0 )
	{
		secaddr = bfd_section_vma(abfd,sec);
		relpp = (arelent **) xmalloc( reloc_bytes );
		relcount = bfd_canonicalize_reloc(abfd,sec,relpp,sympp);
		if ( relcount && ! flagseen['p'] )
			printf( coff_rel_hdr_fmt, "Vaddr", "Type", "Name" );
		for ( i=0; i < relcount; i++ )
		{
			switch ( relpp[i]->howto->type )
			{
			case R_RELLONG:	 	type = "RELLONG ";     break;

#ifdef R_RELLONG_SUB

			case R_RELLONG_SUB:	type = "RELLONG_SUB "; break;

#endif

			case R_RELSHORT: 	type = "RELSHORT";     break;
			case R_IPRMED:	 	type = "IPRMED  ";     break;
			case R_OPTCALL:	 	type = "OPTCALL ";     break;
			case R_OPTCALLX: 	type = "OPTCALLX";     break;
			default:	 	type = "????    ";     break;
			}
			if ( flagseen['o'] )
				printf (coff_rel_oct_fmt, secaddr+relpp[i]->address,
					type, (*(relpp[i]->sym_ptr_ptr))->name);
			else
				printf (coff_rel_hex_fmt, "0x", secaddr+relpp[i]->address,
					type, (*(relpp[i]->sym_ptr_ptr))->name);
		}
	}
	putchar ('\n');
} /* dmp_sec_reloc */

dmp_coff_rel( abfd )
bfd *abfd;
{
	asymbol **sympp;
	asection *sect;
	int symcount;

	sympp = (asymbol **) xmalloc( get_symtab_upper_bound(abfd) );
	symcount = bfd_canonicalize_symtab( abfd, sympp );

	if ( ! flagseen['p'] )
	{
		bfd_center_header( "RELOCATION INFORMATION" );
		putchar ('\n');
	}

	if ( flagseen['n'] )
	{
		/* User wants to dump only one section */
		for ( sect = abfd->sections; sect; sect = sect->next)
		{
			if ( ! strcmp (sect->name, special_sect->name) )
			{
				dmp_sec_reloc (abfd, sect, sympp);
				break;
			}
		}
	}
	else
		bfd_map_over_sections( abfd, dmp_sec_reloc, sympp );
}


int
dmp_coff_stringtab()
{
	struct filehdr 	f;	/* File header				*/
	struct aouthdr 	a;	/* Optional a.out header		*/
	struct scnhdr 	*shdrs;	/* Array of section headers		*/
	unsigned int 	stringtab_len;
	char 		*stringtab;
	long 		pos;

	/* Find beginning of string table (end of symbol table) */
	shdrs = get_coff_hdrs( &f, &a );
	free (shdrs);		/* memory gets allocated even if you don't need it */

	if (!f.f_symptr) {
	    /* String table was omitted because there are no strings */
	    error ("No strings found.");
	    return 1;
	}

	pos = xseek( f.f_symptr + f.f_nsyms * SYMESZ );
	if ( pos == xseekend() )
	{
		/* String table was omitted because there are no strings */
		error ("No strings found.");
		return 1;
	}
	else
		xseek( f.f_symptr + f.f_nsyms * SYMESZ );
	xread( (char *) &stringtab_len, 4 );
	if ( host_is_big_endian != file_is_big_endian )
		BYTESWAP( stringtab_len );

	if ( stringtab_len > 4 )
	{
		register char		*c;
		register unsigned int	len;

		stringtab_len -= 4;
		stringtab = xmalloc( stringtab_len );
		xread( stringtab, stringtab_len );

		if ( ! flagseen['p'] )
		{
			bfd_center_header( "STRING TABLE" );
			printf ( coff_strtab_hdr_fmt, "Offset", "String" );
		}
		for ( c = stringtab, len = stringtab_len; len > 0; )
		{
			printf ( coff_strtab_fmt, c - stringtab + 4, c);
			len -= (strlen( c ) + 1);
			c += (strlen( c ) + 1);
		}
	}
	else
		error ("No strings found.");

	return 1;
} /* dmp_coff_stringtab() */
