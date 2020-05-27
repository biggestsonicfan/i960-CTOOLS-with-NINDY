
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

static char rcsid[] = "$Id: boutdmp.c,v 1.13 1995/02/03 20:11:59 paulr Exp $";

/* This file contains routines specific to b.out format files.
 */

#include <stdio.h>
#include "bout.h"
#include "gdmp960.h"

/* This array holds 1 in the nth place if cmd-line option n was seen */
extern char 	flagseen [];

/* Symbol and string stuff 
 */
static struct nlist 	*symtab;
static char 		*stringtab;
static int		stringtab_len;

/*
 * printf format strings
 */
static char	*bout_rel_hdr_fmt = "   address  sym# pc len ext callj calljx  symbol\n\n";
static char	*bout_rel_fmt = " 0x%08x  %03d  %d  %d   %d    %d      %d    %s\n";
static char	*bout_strtab_hdr_fmt = "%10s        %s\n\n";
static char	*bout_strtab_fmt =     "%10d        %s\n";
extern char	*sec_name_fmt;

static
int
read_hdr(hdr)
    struct exec *hdr;
{
	xseek( 0 );
	xread( (char *) hdr, sizeof(*hdr) );
	if ( host_is_big_endian != file_is_big_endian ){
		BYTESWAP( hdr->a_magic );
		BYTESWAP( hdr->a_text );
		BYTESWAP( hdr->a_data );
		BYTESWAP( hdr->a_bss );
		BYTESWAP( hdr->a_syms );
		BYTESWAP( hdr->a_entry );
		BYTESWAP( hdr->a_trsize );
		BYTESWAP( hdr->a_drsize );
		BYTESWAP( hdr->a_tload );
		BYTESWAP( hdr->a_dload );
	}
	return 1;
}

dmp_bout_hdr()
{
	struct exec hdr;

	read_hdr(&hdr);

	if ( ! flagseen['p'] )
		bfd_center_header( "FILE HEADER" );
	printf( "a_magic  = %o\n", hdr.a_magic );
	printf( "a_text   = 0x%x\t(%d)\n", hdr.a_text, hdr.a_text );
	printf( "a_data   = 0x%x\t(%d)\n", hdr.a_data, hdr.a_data );
	printf( "a_bss    = 0x%x\t(%d)\n", hdr.a_bss, hdr.a_bss );
	printf( "a_syms   = 0x%x\t(%d)\n", hdr.a_syms, hdr.a_syms );
	printf( "a_entry  = 0x%x\n", hdr.a_entry );
	printf( "a_trsize = 0x%x\t(%d)\n", hdr.a_trsize, hdr.a_trsize );
	printf( "a_drsize = 0x%x\t(%d)\n", hdr.a_drsize, hdr.a_drsize );
	printf( "a_tload  = 0x%x\n", hdr.a_tload );
	printf( "a_dload  = 0x%x\n", hdr.a_dload );
	printf( "a_talign = %d\n", hdr.a_talign );
	printf( "a_dalign = %d\n", hdr.a_dalign );
	printf( "a_balign = %d\n", hdr.a_balign );
	printf( "a_ccinfo = %d\n", hdr.a_ccinfo );
	return 1; /* success */
}

static
read_bout_symtab()
{
	struct exec hdr;

	read_hdr(&hdr);
	if ( hdr.a_syms == 0 ){
		symtab = NULL;
		return;
	}

	symtab = (struct nlist *) xmalloc( hdr.a_syms );
	xseek( N_SYMOFF(hdr) );
	xread( (char *) symtab, hdr.a_syms );
	if ( host_is_big_endian != file_is_big_endian ){
		swap_syms( hdr.a_syms/sizeof(struct nlist), symtab );
	}

	xseek( N_STROFF(hdr) );
	xread( (char *) &stringtab_len, 4 );
	if ( host_is_big_endian != file_is_big_endian ){
		BYTESWAP( stringtab_len );
	}

	if ( stringtab_len > 4 ){
		stringtab_len -= 4;
		stringtab = xmalloc( stringtab_len );
		xread( stringtab, stringtab_len );
	}
}

int
dmp_bout_stringtab()
{
	register char	*c;
	register int	len;

	read_bout_symtab();
	if ( symtab == NULL )
		error ("No symbols found.");

	if ( stringtab )
	{
		if ( ! flagseen['p'] )
		{
			bfd_center_header( "STRING TABLE" );
			printf ( bout_strtab_hdr_fmt, "Offset", "String" );
		}
		for ( c = stringtab, len = stringtab_len; len > 0; )
		{
			printf ( bout_strtab_fmt, c - stringtab + 4, c);
			len -= (strlen( c ) + 1);
			c += (strlen( c ) + 1);
		}
	}
	else
		error ("No strings found.");

	return 1;
}

static
free_symtab()
{
	if ( symtab ){
		free( symtab );
		if ( stringtab ) {
			free( stringtab );
		}
	}
	symtab = NULL;
}


/* swap_syms:	Change byte order of symbol table entries
 */
swap_syms( nsyms, symp )
    int nsyms;              /* Number of symbols in object file */
    struct nlist *symp;     /* Pointer to next symbol to convert */
{
	for ( ; nsyms--; symp++ ){
		BYTESWAP( symp->n_un.n_strx );
		BYTESWAP( symp->n_desc );
		BYTESWAP( symp->n_value );
	}
}

/* swap_relocs:	Change byte order of relocation directives
 *
 *	A relocation directive consists of a int address of the item to be
 *	relocated (no problem: we reverse byte order) and a series of bit
 *	fields.
 *
 *	There is one 24-bit field (the symbol number) and a 2-bit field, and a
 *	bunch of 1-bitters.  If the the symbol number is 0x123456 and if xx
 *	represents the other fields, we see the following string of bytes in
 *	memory (from low mem address to high):
 *
 *		big-endian host:	12 34 56 xx	(at least, on a Sun 3)
 *		little-endian host:	56 34 12 xx
 *
 *	So we just swap the first three bytes, then deal with the "xx" byte.
 *	The "xx" byte differs from big- to little-endian as follows:
 *		o the bits in the byte come in the opposite order, from high-
 *			to low-end of the byte, EXCEPT THAT
 *		o the bits of the 2-bit field must not be swapped relative to
 *			each other.
 *
 *	We do the "xx" conversion with a table look-up.  The algorithm for
 *	generating a byte in one of the tables follows;  its scheme is to
 *	reverse ALL of the bits in the byte, then find and swap the bits of
 *	the 2-bit field.
 *
 *	unsigned char
 *	bitswap( old )
 *	    unsigned char old;
 *	{
 *		unsigned char new;
 *		int i;
 *		static char swap_5_6[] = { 0x00, 0x40, 0x20, 0x60 };
 *		static char swap_1_2[] = { 0x00, 0x04, 0x02, 0x06 };
 *
 *		for ( new = 0, i = 0; i < 8; i++ ){	>>> reverse bit order<<<
 *			new <<= 1;
 *			new |= old & 1;
 *			old >>= 1;
 *		}
 *		if ( in_order == LITEND && out_order == BIGEND ){
 *							>>> swap bits 5 & 6 <<<
 *			return (new & 0x9f) | swap_5_6[ (new>>5) & 3 ];
 *		} else {				>>> swap bits 1 & 2 <<<
 *			return (new & 0xf9) | swap_1_2[ (new>>1) & 3 ];
 *		}
 *	}
 */

/* reloc_btol[n] is the little-endian version of the big-endian byte 'n' */
unsigned char reloc_btol[] = {
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 
	0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0, 
	0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 
	0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8, 
	0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 
	0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2, 
	0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 
	0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa, 
	0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 
	0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4, 
	0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 
	0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc, 
	0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 
	0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6, 
	0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 
	0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe, 
	0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 
	0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1, 
	0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 
	0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9, 
	0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 
	0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3, 
	0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 
	0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb, 
	0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 
	0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5, 
	0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 
	0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd, 
	0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 
	0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7, 
	0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 
	0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff, 
};

/* reloc_ltob[n] is the big-endian version of the little-endian byte 'n' */
unsigned char reloc_ltob[] = {
	0x00, 0x80, 0x20, 0xa0, 0x40, 0xc0, 0x60, 0xe0, 
	0x10, 0x90, 0x30, 0xb0, 0x50, 0xd0, 0x70, 0xf0, 
	0x08, 0x88, 0x28, 0xa8, 0x48, 0xc8, 0x68, 0xe8, 
	0x18, 0x98, 0x38, 0xb8, 0x58, 0xd8, 0x78, 0xf8, 
	0x04, 0x84, 0x24, 0xa4, 0x44, 0xc4, 0x64, 0xe4, 
	0x14, 0x94, 0x34, 0xb4, 0x54, 0xd4, 0x74, 0xf4, 
	0x0c, 0x8c, 0x2c, 0xac, 0x4c, 0xcc, 0x6c, 0xec, 
	0x1c, 0x9c, 0x3c, 0xbc, 0x5c, 0xdc, 0x7c, 0xfc, 
	0x02, 0x82, 0x22, 0xa2, 0x42, 0xc2, 0x62, 0xe2, 
	0x12, 0x92, 0x32, 0xb2, 0x52, 0xd2, 0x72, 0xf2, 
	0x0a, 0x8a, 0x2a, 0xaa, 0x4a, 0xca, 0x6a, 0xea, 
	0x1a, 0x9a, 0x3a, 0xba, 0x5a, 0xda, 0x7a, 0xfa, 
	0x06, 0x86, 0x26, 0xa6, 0x46, 0xc6, 0x66, 0xe6, 
	0x16, 0x96, 0x36, 0xb6, 0x56, 0xd6, 0x76, 0xf6, 
	0x0e, 0x8e, 0x2e, 0xae, 0x4e, 0xce, 0x6e, 0xee, 
	0x1e, 0x9e, 0x3e, 0xbe, 0x5e, 0xde, 0x7e, 0xfe, 
	0x01, 0x81, 0x21, 0xa1, 0x41, 0xc1, 0x61, 0xe1, 
	0x11, 0x91, 0x31, 0xb1, 0x51, 0xd1, 0x71, 0xf1, 
	0x09, 0x89, 0x29, 0xa9, 0x49, 0xc9, 0x69, 0xe9, 
	0x19, 0x99, 0x39, 0xb9, 0x59, 0xd9, 0x79, 0xf9, 
	0x05, 0x85, 0x25, 0xa5, 0x45, 0xc5, 0x65, 0xe5, 
	0x15, 0x95, 0x35, 0xb5, 0x55, 0xd5, 0x75, 0xf5, 
	0x0d, 0x8d, 0x2d, 0xad, 0x4d, 0xcd, 0x6d, 0xed, 
	0x1d, 0x9d, 0x3d, 0xbd, 0x5d, 0xdd, 0x7d, 0xfd, 
	0x03, 0x83, 0x23, 0xa3, 0x43, 0xc3, 0x63, 0xe3, 
	0x13, 0x93, 0x33, 0xb3, 0x53, 0xd3, 0x73, 0xf3, 
	0x0b, 0x8b, 0x2b, 0xab, 0x4b, 0xcb, 0x6b, 0xeb, 
	0x1b, 0x9b, 0x3b, 0xbb, 0x5b, 0xdb, 0x7b, 0xfb, 
	0x07, 0x87, 0x27, 0xa7, 0x47, 0xc7, 0x67, 0xe7, 
	0x17, 0x97, 0x37, 0xb7, 0x57, 0xd7, 0x77, 0xf7, 
	0x0f, 0x8f, 0x2f, 0xaf, 0x4f, 0xcf, 0x6f, 0xef, 
	0x1f, 0x9f, 0x3f, 0xbf, 0x5f, 0xdf, 0x7f, 0xff, 
};

static
swap_reloc( relocp, nrelocs )
    struct relocation_info *relocp;
    int nrelocs;
{
	int i;
	unsigned char *rp;
	unsigned char *tabptr;

	tabptr = host_is_big_endian ? reloc_ltob : reloc_btol;
	for ( i = 0; i < nrelocs; i++, relocp++ ){
		BYTESWAP( relocp->r_address );
		byteswap( ((char*)(&relocp->r_address)) + 4, 3 );
		rp = ((unsigned char*)(&relocp->r_address)) + 7;
		*rp = tabptr[*rp];
	}
}

static
dmprel( len, filepos, offset )
    unsigned len;
    unsigned filepos;
    unsigned offset;
{
	struct relocation_info *buf;
	struct nlist *symp;
	int i;
	char * symname;
	int nrelocs;

	if ( len == 0 ){
		return;
	}

	nrelocs = len / sizeof(struct relocation_info);
	buf = (struct relocation_info *) xmalloc( len );
	xseek( filepos );
	xread( (char *) buf, len );
	if ( host_is_big_endian != file_is_big_endian ){
		swap_reloc( buf, nrelocs );
	}

	if ( ! flagseen['p'] )
		printf( bout_rel_hdr_fmt );
	for ( i = 0; i < nrelocs; i++ )
	{
		if ( buf[i].r_extern ){
			if ( symtab ) {
				symp = &symtab[ buf[i].r_symbolnum ];
				symname = &stringtab[ symp->n_un.n_strx ] - 4;
				/* -4 because length of string table not saved
				 * in stringtab.
				 */
			} else {
				symname = "";
			}
		} else {
			switch ( buf[i].r_symbolnum ){
			case N_TEXT:
				symname = ".text";
				break;
			case N_DATA:
				symname = ".data";
				break;
			case N_BSS:
				symname = ".bss";
				break;
			default:
				symname = "???";
				break;
			}
		}

		printf( bout_rel_fmt,
			 buf[i].r_address + offset,
			 buf[i].r_symbolnum,
			 buf[i].r_pcrel,
			 buf[i].r_length,
			 buf[i].r_extern,
			 buf[i].r_callj,
		         buf[i].r_calljx,
			 symname);
	}
	free( buf );
}


dmp_bout_rel()
{
	struct exec hdr;

	read_hdr(&hdr);
	read_bout_symtab();
	if ( ! flagseen['p'] )
	{
		bfd_center_header( "RELOCATION INFORMATION" );
		putchar ('\n');
	}

	printf( sec_name_fmt, ".text", "" );
	dmprel( hdr.a_trsize, N_TROFF(hdr), hdr.a_tload );
	putchar ('\n');

	printf( sec_name_fmt, ".data", "" );
	dmprel( hdr.a_drsize, N_DROFF(hdr), hdr.a_dload );
	putchar ('\n');

	free_symtab();
}
