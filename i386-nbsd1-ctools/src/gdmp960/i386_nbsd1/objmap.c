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
 
/* 
 * objmap is a utility for graphically examining b.out, coff and elf format 
 * files
 */
 
#include <stdio.h>
#include "sysdep.h"
#include "bfd.h"
#include "bout.h"
#undef N_ABS
#include "coff.h"
#include "elf.h"

#define MAXCHARS 20
#define SEP "+----------------------+"
#define  BYTESWAP(n)    byteswap(&n,sizeof(n))

extern get_elf_hdrs();
extern get_coff_hdrs();

extern char host_is_big_endian;        /* TRUE or FALSE */
extern char file_is_big_endian;        /* TRUE or FALSE */

int prev_scn_ptr;
int prev_scn_size;

center( s, buf, n )
    char *s;
    char *buf;
    int n;
{
	int i;
	int l;

	for ( i=0; i<n; i++ ){
		buf[i] = ' ';
	}
	l = strlen(s);
	buf[i] = 0;
	strncpy( buf+((n-l)/2), s, l );
}


say( s, l, addr )
    char *s;
    int l;
    int addr;
{
	char buf[MAXCHARS+10];
	char buf2[MAXCHARS+10];

	center( s, buf, MAXCHARS );
	printf( "\t| %s |\n", buf );

	sprintf( buf2,"0x%x (%d)", l, l );
	center( buf2, buf, MAXCHARS );
	printf( "\t| %s |\n", buf );

	addr += l;
	printf( "\t%s %x\t%d\t%o\n", SEP, addr, addr, addr  );
}

begin_map()
{
	int i;

	putchar( '\t' );
	for ( i = strlen(SEP); i; i-- ){
		putchar( ' ' );
	}
	printf( " HEX\tDEC\tOCT\n" );
	putchar( '\t' );
	for ( i = strlen(SEP); i; i-- ){
		putchar( ' ' );
	}
	printf( " ---\t---\t---\n" );
	printf( "\t%s 0\t0\t0\n", SEP );
}

static void say_ccinfo(abfd)
    bfd *abfd;
{
    if ( bfd_get_file_flags(abfd) & HAS_CCINFO ){
	int ccinfo_len = bfd960_seek_ccinfo(abfd);
	int ccinfo_addr = bfd_tell(abfd);
	if ( BFD_COFF_FILE_P(abfd) ){
	    addsay("0xffffffff",4,ccinfo_addr-4);
	}
	addsay("cc info",ccinfo_len,ccinfo_addr);
    }
}

map_bout(abfd)
    bfd *abfd;
{
	struct exec hdr;
	int strtab_len;

	xseek( 0 );
	xread( &hdr, sizeof(hdr) );
	if ( host_is_big_endian != file_is_big_endian ){
		BYTESWAP( hdr.a_magic );
		BYTESWAP( hdr.a_text );
		BYTESWAP( hdr.a_data );
		BYTESWAP( hdr.a_bss );
		BYTESWAP( hdr.a_syms );
		BYTESWAP( hdr.a_entry );
		BYTESWAP( hdr.a_trsize );
		BYTESWAP( hdr.a_drsize );
		BYTESWAP( hdr.a_tload );
		BYTESWAP( hdr.a_dload );
	}

	addsay( "Header", sizeof(hdr), 0 );
	prev_scn_ptr = 0;
	prev_scn_size = sizeof(hdr);

	if ( hdr.a_text ){
		addsay( "Text", hdr.a_text, N_TXTOFF(hdr) );
	}

	if ( hdr.a_data ){
		addsay( "Data", hdr.a_data, N_DATOFF(hdr) );
	}

	if ( hdr.a_trsize ){
		addsay( "Text Relocations", hdr.a_trsize, N_TROFF(hdr) );
	}

	if ( hdr.a_drsize ){
		addsay( "Data Relocations", hdr.a_drsize, N_DROFF(hdr) );
	}

	if ( hdr.a_syms ){
		addsay( "Symbols", hdr.a_syms, N_SYMOFF(hdr) );
		xseek( N_STROFF(hdr) );
		xread( &strtab_len, 4 );
		if ( host_is_big_endian != file_is_big_endian ){
			BYTESWAP( strtab_len );
		}
		addsay( "Strings", strtab_len, N_STROFF(hdr) );
	}

	say_ccinfo(abfd);

	justsayit();
}

map_coff(abfd)
    bfd *abfd;
{
	struct filehdr f;	/* File header				*/
	struct aouthdr a;	/* Optional a.out header		*/
	struct scnhdr *shdrs;	/* Array of section headers		*/
	struct scnhdr *sP;	/* Pointer to an entry in shdrs array	*/
	int i;			/* Loop counter				*/
	int strtab_len;
	int addr;
	char buf[100];
	
	addr = 0;
	shdrs = (struct scnhdr*) get_coff_hdrs( &f, &a );

	addsay( "File Header", sizeof(f), addr );
	addr += sizeof(f);
	
	if ( f.f_opthdr ){
		addsay( "Optional Hdr.", f.f_opthdr, addr );
		addr += f.f_opthdr;
	}

	addsay( "Section Hdrs.", f.f_nscns * SCNHSZ, addr );

	prev_scn_ptr = addr;
	prev_scn_size = f.f_nscns * SCNHSZ;
	for ( i=0, sP=shdrs; i<(int)f.f_nscns; i++, sP++ ){
		if ( sP->s_scnptr ){
		   addsay( sP->s_name, sP->s_size, sP->s_scnptr );
		   prev_scn_ptr = sP->s_scnptr;
		   prev_scn_size = sP->s_size;
		}
	}

	for ( i=0, sP=shdrs; i<(int)f.f_nscns; i++, sP++ ){
		if ( sP->s_relptr ){
			sprintf( buf, "%s Relocation", sP->s_name );
			addsay( buf, sP->s_nreloc*RELSZ, sP->s_relptr );
			prev_scn_ptr = sP->s_relptr;
			prev_scn_size = sP->s_nreloc*RELSZ;
		}
	}

	for ( i=0, sP=shdrs; i<(int)f.f_nscns; i++, sP++ ){
		if ( sP->s_lnnoptr ){
			sprintf( buf, "%s Line Nos.", sP->s_name );
			addsay( buf, sP->s_nlnno*LINESZ, sP->s_lnnoptr );
			prev_scn_ptr = sP->s_lnnoptr;
			prev_scn_size = sP->s_nlnno*LINESZ;
		}
	}

	if ( f.f_nsyms ){
		addsay( "Symbols", f.f_nsyms*SYMESZ, f.f_symptr );
		prev_scn_ptr = f.f_symptr;
		prev_scn_size = f.f_nsyms*SYMESZ;
		xseek( f.f_symptr + f.f_nsyms*SYMESZ );
		i = xread_rs( &strtab_len, 4 );
		if ( i != 0 ){
			if ( i != 4 ){
				printf( "Read failed\n" );
				exit(3);
			}
			if ( host_is_big_endian != file_is_big_endian ){
			    BYTESWAP( strtab_len );
			}
			addsay( "Strings", strtab_len ? strtab_len : 4,
			       f.f_symptr + f.f_nsyms*SYMESZ );
		}
	} 

	say_ccinfo(abfd);

	justsayit();
}

struct sayit_struct {
    char *sayit;
    unsigned long offset,length;
    struct sayit_struct *next_sayit_struct;
} *sayit_table;

addsay(sayit,length,offset)
    char *sayit;
    unsigned long length,offset;
{
    struct sayit_struct **p = &sayit_table,*q = (struct sayit_struct *) 0;
    struct sayit_struct *t = (struct sayit_struct *) xmalloc(sizeof(struct sayit_struct));

    t->sayit = sayit;
    t->length = length;
    t->offset = offset;
    while (*p && (*p)->offset+(*p)->length <= offset+length) {
	q = *p;
	p = &((*p)->next_sayit_struct);
    }

    if (!(*p)) {
	t->next_sayit_struct = (*p);
	*p = t;
    }
    else {
	struct sayit_struct *z = q->next_sayit_struct;

	q->next_sayit_struct = t;
	t->next_sayit_struct = z;
    }
}


justsayit()
{
    struct sayit_struct *p = sayit_table,*previous = (struct sayit_struct *) 0,
    *previous_previous = (struct sayit_struct *) 0;

    begin_map();

    while (p) {
	if (previous) {
	    if (previous->offset+previous->length > p->offset) {
		say("*** OVERLAP ***",previous->offset+previous->length - p->offset,p->offset);
	    }
	    if (previous->offset+previous->length < p->offset) {
		say("*** GAP ***",p->offset - (previous->offset+previous->length),
		    previous->offset+previous->length);
	    }
	}
	say(p->sayit,p->length,p->offset);
	if (previous_previous)
		free(previous_previous);
	previous_previous = previous;
	previous = p;
	p = p->next_sayit_struct;
    }
    if (previous_previous)
	    free(previous_previous);
    if (previous) {
	if (xsize() > previous->offset+previous->length)
		say("*** GARBAGE ***",xsize() - (previous->offset+previous->length),
		    previous->offset+previous->length);
	free(previous);
    }
    sayit_table = (struct sayit_struct *) 0;
    say("EOF OF FILE",0,xsize());
}

map_elf()
{
    Elf32_Ehdr *f;	  /* Elf header */
    Elf32_Shdr *shdrs;    /* Section headers */
    Elf32_Phdr *phdrs;    /* Program headers */
    int i;

    get_elf_hdrs(&f,&shdrs,&phdrs);

    addsay("ELF Header",sizeof(*f),0);

    if (f->e_phnum) {
	    addsay("Program Header Table",f->e_phnum * f->e_phentsize,f->e_phoff);
    }	

    if (f->e_shnum) {
	Elf32_Shdr *t = shdrs + f->e_shstrndx;
	char *string_table = (char*)xmalloc(t->sh_size);
	xseek(t->sh_offset);
	xread(string_table,t->sh_size);
        
	for (i=0;i < (int)f->e_shnum;i++) {
	    t = shdrs + i;
	    if (t->sh_offset) {
		char *sect_name = t->sh_name + string_table;
		int length;
		char *template;

		if ((int)strlen(sect_name) > 12) {
		    length = strlen(sect_name) + 1;
		    template = (char *) xmalloc(length);
		    strcpy(template,sect_name);
		}
		else {
#define SAY_SECT_HDR "Section: %s"
		    length = strlen(SAY_SECT_HDR) + strlen(sect_name);
		    template = (char *) xmalloc(length);
		    sprintf(template,SAY_SECT_HDR,sect_name);
		}
		addsay(template,t->sh_type == SHT_NOBITS ? 0 :t->sh_size,t->sh_offset);
	    }
	}
	addsay("Section header table",f->e_shnum * f->e_shentsize,f->e_shoff);
    }
    justsayit();
}
