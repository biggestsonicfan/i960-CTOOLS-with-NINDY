/* Intel 960 COFF support for BFD.  */

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
#include "coff.h"
#include "libcoff.h"


/* Object file tdata; access macros */

#define exec_hdr(bfd)		(coff_data(bfd)->hdr)
#define obj_symbols(bfd)	(coff_data(bfd)->symbols)
#define	obj_sym_filepos(bfd)	(coff_data(bfd)->sym_filepos)
#define obj_str_filepos(bfd)	(coff_data(bfd)->str_filepos)
#define obj_relocbase(bfd)	(coff_data(bfd)->relocbase)
#define obj_convert(bfd)	(coff_data(bfd)->conversion_table)
#define obj_string_table(bfd)	(coff_data(bfd)->string_table)


/* Align an address upward to a boundary, expressed as a number of bytes.
 * E.g. align to an 8-byte boundary with argument of 8.
 */
#define ALIGN(this,boundary)	(((this) + ((boundary) -1)) & (~((boundary)-1)))

/* Align an address upward to a power of two.  Argument is the power
 * of two, e.g. 8-byte alignment uses argument of 3 (8 == 2^3).
 */
#define	i960_align(addr,align)	(((addr) + ((1<<(align))-1)) & (-1 << (align)))


#define GETWORD(abfd,src)	bfd_h_get_32(abfd,(bfd_byte *)&src)
#define GETHALF(abfd,src)	bfd_h_get_16(abfd,(bfd_byte *)&src)
#define PUTWORD(abfd,src,dst)	bfd_h_put_32(abfd,src,(bfd_byte *)&dst)
#define PUTHALF(abfd,src,dst)	bfd_h_put_16(abfd,src,(bfd_byte *)&dst)

			/* name    type       pc-rel  mask    */
			/* ----    ----       ------  ----    */
static reloc_howto_type
howto_reloc_rellong =   { "rellong",     R_RELLONG,     bfd_reloc_type_32bit_abs };
 
static reloc_howto_type
howto_reloc_relshort =  { "relshort",    R_RELSHORT,    bfd_reloc_type_12bit_abs };
 
static reloc_howto_type
howto_reloc_iprmed =    { "iprmed",      R_IPRMED,      bfd_reloc_type_24bit_pcrel };

static reloc_howto_type
howto_reloc_callj =     { "optcall",     R_OPTCALL,     bfd_reloc_type_opt_call };

static reloc_howto_type
howto_reloc_calljx =    { "optcallx",    R_OPTCALLX,    bfd_reloc_type_opt_callx };

static reloc_howto_type
howto_reloc_none   =    { "noreloc",     0,             bfd_reloc_type_none };

#ifdef R_RELLONG_SUB

static reloc_howto_type
howto_reloc_rellong_sub={ "rellong_sub", R_RELLONG_SUB, bfd_reloc_type_32bit_abs_sub };

#endif

static more_symbol_info coff_more_symbol_info_stat;

static int
coff_more_symbol_info(abfd, asym, m, info_type)
    bfd *abfd;	
    asymbol *asym;
    more_symbol_info *m;
    bfd_sym_more_info_type info_type;	
{
    coff_symbol_type *cs = coffsymbol(asym);
    SYMENT *s = &(fetch_native_coff_info(asym)->u.syment);
    AUXENT *aux;

    switch (info_type) 
    {
    case bfd_unknown_info:
	    return 0;

    case bfd_sysproc_val:
	    if (asym->flags & BSF_HAS_SCALL) {
		    coff_more_symbol_info_stat.sysproc_value = asym->related_symbol->value;
		    *m = coff_more_symbol_info_stat;
		    return 1;
	    }
	    else
		    return 0;

    case bfd_function_size:
	    if ( ISFCN(s->n_type) )
	    {
		    aux = &((fetch_native_coff_info(asym)+1)->u.auxent);
		    coff_more_symbol_info_stat.function_size = aux->x_sym.x_misc.x_fsize;
		    *m = coff_more_symbol_info_stat;
		    return 1;
	    }
	    else
		    return 0;

    case bfd_storage_class:

	    coff_more_symbol_info_stat.sym_class = s->n_sclass;
	    *m = coff_more_symbol_info_stat;
	    return 1;

    case bfd_symbol_type:

	    coff_more_symbol_info_stat.sym_type = s->n_type;
	    *m = coff_more_symbol_info_stat;
	    return 1;

    case bfd_set_sysproc_val:
	    if (asym->flags & BSF_HAS_SCALL) {
		/* Set related symbol: */
		asym->related_symbol->value = m->sysproc_value;
		/* AND, set 2nd aux: */
		aux = &((fetch_native_coff_info(asym)+2)->u.auxent);
		aux->x_sc.x_stindx = m->sysproc_value;
		return 1;
	    }
	    else
		    return 0;
    default:
	    return 0;
    }
}


/* Assume file contains ccinfo.  Position file pointer at start of it.
 * I.e., seek past the symbol table, read the string table length, and
 * seek past the string table.  The string table should be followed by
 * a delimiter word of -1, which must also be sought past.
 *
 * Note that a COFF file may not have a string table even though it has
 * a symbol table.  In that case, we will pick up the -1 delimeter as
 * the (impossible) length of the string table and we are done.
 *
 * Return 0 on failure, non-zero on success
 */
int
icoff_seek_ccinfo( abfd )
    bfd *abfd;
{
	int strtab_len;

	bfd_error = system_call_error;
	
	if ( (bfd_seek(abfd,obj_str_filepos(abfd),SEEK_SET) < 0)
	||   (bfd_read(&strtab_len,4,1,abfd) != 4) ){
		return 0;
	}

	if ( strtab_len != -1 ){
		/* String table is present -- skip over it */
		strtab_len = GETWORD(abfd,strtab_len);
		if ( (bfd_seek(abfd,strtab_len-4,SEEK_CUR) < 0)
		||   (bfd_read(&strtab_len,4,1,abfd) != 4)
		||   (strtab_len != -1) ){
			return 0;
		}
	}
	return 1;
}

/***********************************************************************
Here are all the routines for swapping the structures seen in the
outside world into the internal forms. 
*/


static void
DEFUN(bfd_swap_reloc_in,(abfd, reloc_src, reloc_dst),
    bfd   *abfd AND
    RELOC *reloc_src AND
    RELOC *reloc_dst)
{
	reloc_dst->r_vaddr = GETWORD(abfd, reloc_src->r_vaddr);
	reloc_dst->r_symndx = GETWORD(abfd, reloc_src->r_symndx);
	reloc_dst->r_type = GETHALF(abfd, reloc_src->r_type);
}


static void
DEFUN(bfd_write_reloc,(abfd, reloc),
    bfd   *abfd AND
    RELOC *reloc)
{
	RELOC tmp;

	PUTWORD(abfd, reloc->r_vaddr, tmp.r_vaddr);
	PUTWORD(abfd, reloc->r_symndx,tmp.r_symndx);
	PUTHALF(abfd, reloc->r_type,  tmp.r_type);
	tmp.pad[0] = tmp.pad[1] = 0;
	bfd_write((PTR) &tmp, 1, RELSZ, abfd);
}


static void
DEFUN(bfd_swap_filehdr_in,(abfd, filehdr_src, filehdr_dst),
    bfd    *abfd AND
    FILHDR *filehdr_src AND
    FILHDR *filehdr_dst)
{
	filehdr_dst->f_magic  = GETHALF(abfd,filehdr_src->f_magic);
	filehdr_dst->f_nscns  = GETHALF(abfd,filehdr_src->f_nscns);
	filehdr_dst->f_timdat = GETWORD(abfd,filehdr_src->f_timdat);
	filehdr_dst->f_symptr = GETWORD(abfd,filehdr_src->f_symptr);
	filehdr_dst->f_nsyms  = GETWORD(abfd,filehdr_src->f_nsyms);
	filehdr_dst->f_opthdr = GETHALF(abfd,filehdr_src->f_opthdr);
	filehdr_dst->f_flags  = GETHALF(abfd,filehdr_src->f_flags);
}


static  void 
DEFUN(bfd_write_filehdr,(abfd, filehdr),
    bfd    *abfd AND
    FILHDR *filehdr)
{
	FILHDR tmp;

	PUTHALF(abfd, filehdr->f_magic,  tmp.f_magic);
	PUTHALF(abfd, filehdr->f_nscns,  tmp.f_nscns);
	PUTWORD(abfd, filehdr->f_timdat, tmp.f_timdat);
	PUTWORD(abfd, filehdr->f_symptr, tmp.f_symptr);
	PUTWORD(abfd, filehdr->f_nsyms,  tmp.f_nsyms);
	PUTHALF(abfd, filehdr->f_opthdr, tmp.f_opthdr);
	PUTHALF(abfd, filehdr->f_flags,  tmp.f_flags);
	bfd_write((PTR) &tmp, 1, FILHSZ, abfd);
}


static void 
DEFUN(coff_swap_sym_in,(abfd, ext, in),
    bfd    *abfd AND
    SYMENT *ext  AND
    SYMENT *in)
{
	if( ext->n_name[0] == 0) {
		in->n_zeroes = 0;
		in->n_offset = GETWORD(abfd, ext->n_offset);
	} else {
		memcpy(in->n_name, ext->n_name, SYMNMLEN);
	}
	in->n_value  = GETWORD(abfd, ext->n_value);
	in->n_scnum  = GETHALF(abfd, ext->n_scnum);
	in->n_flags  = GETHALF(abfd, ext->n_flags);
	in->n_type   = GETWORD(abfd, ext->n_type);

	/* Byte values need no swapping */
	in->n_sclass = ext->n_sclass;
	in->n_numaux = ext->n_numaux;
}

static void 
DEFUN(coff_write_sym,(abfd,in),
    bfd    *abfd AND
    SYMENT *in)
{
	SYMENT tmp;

	if(in->n_name[0] == 0) {
		tmp.n_zeroes = 0;
		PUTWORD(abfd, in->n_offset, tmp.n_offset);
	} else {
		memcpy(tmp.n_name, in->n_name, SYMNMLEN);
	}
	PUTWORD(abfd, in->n_value, tmp.n_value);
	PUTHALF(abfd, in->n_scnum, tmp.n_scnum);
	PUTHALF(abfd, in->n_flags, tmp.n_flags);
	PUTWORD(abfd, in->n_type,  tmp.n_type);

	/* Byte and 0 values need no swapping */
	tmp.n_sclass = in->n_sclass;
	tmp.n_numaux = in->n_numaux;
	tmp.pad2[0] = tmp.pad2[1] = 0;

	bfd_write((PTR)&tmp, 1, SYMESZ, abfd);
}

static
int
has_ary(type)
    unsigned int type;
{
    for (;(type & N_TMASK) && (!ISFCN(type));type >>= N_TSHIFT)
	    if (ISARY(type))
		    return 1;
    return 0;
}



static void
DEFUN(coff_swap_aux_in,(abfd, ext, type, class, in, file_aux_cnt),
    bfd    *abfd AND
    AUXENT *ext AND
    int     type AND
    int     class AND
    AUXENT *in AND
    int file_aux_cnt)
{
	switch (class) {
	case C_FILE:
		if ( file_aux_cnt ){
			/* 2nd and 3rd aux (if present) for a file symbol are
			 * ident records, not filenames
			 */
			in->x_ident.x_timestamp =
				GETWORD(abfd, ext->x_ident.x_timestamp);
			memcpy(in->x_ident.x_idstring,ext->x_ident.x_idstring,
						sizeof(in->x_ident.x_idstring));

		} else if (ext->x_file.x_n.x_zeroes == 0) {
		    in->x_file.x_n.x_zeroes = 0;
		    in->x_file.x_n.x_offset =
			    GETWORD(abfd, ext->x_file.x_n.x_offset);
		} else {
			memcpy(in->AUX_FNAME,ext->AUX_FNAME,sizeof(in->AUX_FNAME));
		}
		break;

	case C_STAT:
		if (type == T_NULL) {
			/* This is a section symbol */
			in->x_scn.x_scnlen = GETWORD(abfd, ext->x_scn.x_scnlen);
			in->x_scn.x_nreloc = GETHALF(abfd, ext->x_scn.x_nreloc);
			in->x_scn.x_nlinno = GETHALF(abfd, ext->x_scn.x_nlinno);
			break;
		}
	default:
		in->AUX_TAGNDX = GETWORD(abfd, ext->AUX_TAGNDX);
		in->x_sym.x_tvndx  = 0;

		if (has_ary(type)) {
			in->AUX_DIMEN[0] = GETHALF(abfd, ext->AUX_DIMEN[0]);
			in->AUX_DIMEN[1] = GETHALF(abfd, ext->AUX_DIMEN[1]);
			in->AUX_DIMEN[2] = GETHALF(abfd, ext->AUX_DIMEN[2]);
			in->AUX_DIMEN[3] = GETHALF(abfd, ext->AUX_DIMEN[3]);
		} else {
			in->AUX_LNNOPTR = GETWORD(abfd, ext->AUX_LNNOPTR);
			in->AUX_ENDNDX  = GETWORD(abfd, ext->AUX_ENDNDX);
		}

		if (ISFCN(type)) {
			in->x_sym.x_misc.x_fsize =
				GETWORD(abfd, ext->x_sym.x_misc.x_fsize);
		} else {
			in->AUX_LNNO = GETHALF(abfd, ext->AUX_LNNO);
			in->AUX_SIZE = GETHALF(abfd, ext->AUX_SIZE);
		}
	}
}


static void
DEFUN(coff_write_aux,(abfd, in, type, class, file_aux_cnt),
    bfd    *abfd  AND
    AUXENT *in    AND
    int     type  AND
    int     class AND
    int file_aux_cnt)
{
	AUXENT tmp;

	memset( &tmp, 0, sizeof(tmp) );

	switch (class) {
	case C_FILE:
		if ( file_aux_cnt ){
			/* 2nd and 3rd aux (if present) for a file symbol are
			 * ident records, not filenames
			 */
			PUTWORD(abfd, in->x_ident.x_timestamp,
						tmp.x_ident.x_timestamp);
			memcpy(tmp.x_ident.x_idstring,in->x_ident.x_idstring,
						sizeof(in->x_ident.x_idstring));

		} else if (in->AUX_FNAME[0] == 0) {
			PUTWORD(abfd, 0, tmp.x_file.x_n.x_zeroes );
			PUTWORD(abfd, in->x_file.x_n.x_offset,
						tmp.x_file.x_n.x_offset);
		} else {
		    int len = 0,i;

		    for (i=0;in->AUX_FNAME[i] && i < FILNMLEN;i++)
			    len = i;
		    
		    memset(tmp.AUX_FNAME,0,FILNMLEN);
		    if (len == FILNMLEN)
			    strncpy (tmp.AUX_FNAME,in->AUX_FNAME, FILNMLEN);
		    else
			    strcpy(tmp.AUX_FNAME,in->AUX_FNAME);
		}
		break;

	case C_STAT:
		if (type == T_NULL) {
			/* This is a section symbol */
			PUTWORD(abfd, in->x_scn.x_scnlen, tmp.x_scn.x_scnlen);
			PUTHALF(abfd, in->x_scn.x_nreloc, tmp.x_scn.x_nreloc);
			PUTHALF(abfd, in->x_scn.x_nlinno, tmp.x_scn.x_nlinno);
			break;
		}

	default:
		tmp.x_sym.x_tvndx = 0;
		PUTWORD(abfd, in->AUX_TAGNDX, tmp.AUX_TAGNDX);

		if (has_ary(type)) {
			PUTHALF(abfd, in->AUX_DIMEN[0], tmp.AUX_DIMEN[0]);
			PUTHALF(abfd, in->AUX_DIMEN[1], tmp.AUX_DIMEN[1]);
			PUTHALF(abfd, in->AUX_DIMEN[2], tmp.AUX_DIMEN[2]);
			PUTHALF(abfd, in->AUX_DIMEN[3], tmp.AUX_DIMEN[3]);
		} else {
			PUTWORD(abfd, (abfd->flags & STRIP_LINES) ? 0 : in->AUX_LNNOPTR,
				tmp.AUX_LNNOPTR);
			PUTWORD(abfd, in->AUX_ENDNDX,  tmp.AUX_ENDNDX);
		}

		if (ISFCN(type)) {
			PUTWORD(abfd, in->x_sym.x_misc.x_fsize,
						tmp.x_sym.x_misc.x_fsize);
		} else {
			PUTHALF(abfd, in->AUX_LNNO, tmp.AUX_LNNO);
			PUTHALF(abfd, in->AUX_SIZE, tmp.AUX_SIZE);
		}
	}
	bfd_write((PTR) &tmp, 1, AUXESZ, abfd);
}


static void
DEFUN(coff_swap_lineno_in,(abfd, ext, in),
    bfd    *abfd AND
    LINENO *ext  AND
    LINENO *in)
{
	in->l_addr.l_symndx = GETWORD(abfd, ext->l_addr.l_symndx);
	in->l_lnno = GETHALF(abfd, ext->l_lnno);
}


static void
DEFUN(coff_write_lineno,(abfd, in),
    bfd    *abfd AND
    LINENO *in)
{
	LINENO tmp;

	PUTWORD(abfd, in->l_addr.l_symndx, tmp.l_addr.l_symndx);
	PUTHALF(abfd, in->l_lnno, tmp.l_lnno);
	tmp.padding[0] = tmp.padding[1] = 0;
	bfd_write((PTR) &tmp, 1, LINESZ, abfd);
}


static void 
DEFUN(bfd_swap_aouthdr_in,(abfd, aouthdr_ext, aouthdr_int),
    bfd     *abfd        AND
    AOUTHDR *aouthdr_ext AND
    AOUTHDR *aouthdr_int)
{
	aouthdr_int->magic = GETHALF(abfd, aouthdr_ext->magic);
	aouthdr_int->vstamp = GETHALF(abfd, aouthdr_ext->vstamp);
	aouthdr_int->tsize = GETWORD(abfd, aouthdr_ext->tsize);
	aouthdr_int->dsize = GETWORD(abfd, aouthdr_ext->dsize);
	aouthdr_int->bsize = GETWORD(abfd, aouthdr_ext->bsize);
	aouthdr_int->entry = GETWORD(abfd, aouthdr_ext->entry);
	aouthdr_int->text_start= GETWORD(abfd, aouthdr_ext->text_start);
	aouthdr_int->data_start= GETWORD(abfd, aouthdr_ext->data_start);
	aouthdr_int->tagentries= GETWORD(abfd, aouthdr_ext->tagentries);
}


static void 
DEFUN(bfd_write_aouthdr,(abfd, in),
    bfd     *abfd AND
    AOUTHDR *in)
{
	AOUTHDR tmp;

	PUTHALF(abfd, in->magic,  tmp.magic);
	PUTHALF(abfd, in->vstamp, tmp.vstamp);
	PUTWORD(abfd, in->tsize,  tmp.tsize);
	PUTWORD(abfd, in->dsize,  tmp.dsize);
	PUTWORD(abfd, in->bsize,  tmp.bsize);
	PUTWORD(abfd, in->entry,  tmp.entry);
	PUTWORD(abfd, in->text_start, tmp.text_start);
	PUTWORD(abfd, in->data_start, tmp.data_start);
	PUTWORD(abfd, in->tagentries, tmp.tagentries);
	bfd_write((PTR) &tmp, 1, AOUTSZ, abfd);
}


static void 
DEFUN(coff_swap_scnhdr_in,(abfd, ext, in),
    bfd    *abfd AND
    SCNHDR *ext  AND
    SCNHDR *in)
{
	memcpy(in->s_name, ext->s_name, sizeof(in->s_name));
	in->s_paddr  = GETWORD(abfd, ext->s_paddr);
	in->s_vaddr  = GETWORD(abfd, ext->s_vaddr);
	in->s_size   = GETWORD(abfd, ext->s_size);
	in->s_scnptr = GETWORD(abfd, ext->s_scnptr);
	in->s_relptr = GETWORD(abfd, ext->s_relptr);
	in->s_lnnoptr= GETWORD(abfd, ext->s_lnnoptr);
	in->s_nreloc = GETHALF(abfd, ext->s_nreloc);
	in->s_nlnno  = GETHALF(abfd, ext->s_nlnno);
	in->s_flags  = GETWORD(abfd, ext->s_flags);
	in->s_align  = GETWORD(abfd, ext->s_align);
}


static void 
DEFUN(write_scnhdr,(abfd, in),
    bfd    *abfd AND
    SCNHDR *in)
{
	SCNHDR tmp;

	memcpy(tmp.s_name, in->s_name, sizeof(in->s_name));
	PUTWORD(abfd, in->s_paddr,  tmp.s_paddr);
	PUTWORD(abfd, in->s_vaddr,  tmp.s_vaddr);
	PUTWORD(abfd, in->s_size,   tmp.s_size);
	PUTWORD(abfd, in->s_scnptr, tmp.s_scnptr);
	PUTWORD(abfd, in->s_relptr, tmp.s_relptr);
	PUTWORD(abfd, in->s_lnnoptr,tmp.s_lnnoptr);
	PUTHALF(abfd, in->s_nreloc, tmp.s_nreloc);
	PUTHALF(abfd, in->s_nlnno,  tmp.s_nlnno);
	PUTWORD(abfd, in->s_flags,  tmp.s_flags);
	PUTWORD(abfd, in->s_align,  tmp.s_align);
	bfd_write((PTR)&tmp, 1, SCNHSZ, abfd);
}


/* Return a pointer to a malloc'd copy of 'name'.  'name' may not be
 * \0-terminated, but will not exceed 'maxlen' characters.  The copy *will*
 * be \0-terminated.
 */
static char *
DEFUN(copy_name,(abfd, name, maxlen),
    bfd *abfd AND
    char *name AND
    int maxlen)
{
	int  len;
	char *newname;

	for (len = 0; len < maxlen; ++len) {
		if (name[len] == '\0') {
			break;
		}
	}

	if ((newname = (PTR) bfd_alloc(abfd, len+1)) == NULL) {
		bfd_error = no_memory;
		return (NULL);
	}
	strncpy(newname, name, len);
	newname[len] = '\0';
	return newname;
}


/* Initialize a section structure with information peculiar to this
 * particular implementation of coff
 */
static boolean
DEFUN(coff_new_section_hook,(abfd_ignore, section_ignore),
    bfd      *abfd_ignore AND
    asection *section_ignore)
{
	return true;
}

/* Take a section header read from a coff file (in HOST byte order),
 * and make a BFD "section" out of it.
 */
static          boolean
DEFUN(make_a_section_from_file,(abfd, hdr, file_offset_of_section_header),
    bfd            *abfd AND
    struct scnhdr  *hdr  AND
    unsigned long  file_offset_of_section_header)
{
	asection *sec;
	unsigned int i;
	char *name;

	if ( (name = bfd_alloc(abfd, sizeof (hdr->s_name)+1)) == NULL) {
		bfd_error = no_memory;
		return false;
	}
	strncpy(name, hdr->s_name, sizeof(hdr->s_name));
	name[sizeof(hdr->s_name)] = 0;

	sec = bfd_make_section(abfd, name);

	sec->vma	= hdr->s_vaddr;
	sec->pma	= hdr->s_paddr;
	sec->size	= hdr->s_size;
	sec->filepos	= hdr->s_scnptr;
	sec->rel_filepos= hdr->s_relptr;
	sec->reloc_count= hdr->s_nreloc;
	sec->line_filepos= hdr->s_lnnoptr;
	sec->lineno_count= hdr->s_nlnno;
	sec->userdata	= NULL;
	sec->next	= (asection *) NULL;
	sec->flags	= 0;
	sec->paddr_file_offset = file_offset_of_section_header +
		(((unsigned long) &hdr->s_paddr) - ((unsigned long) hdr));
	/* Remember inheritable characteristics of an input section */
	sec->insec_flags = hdr->s_flags;

	sec->alignment_power = hdr->s_align;
	for (i = 0; i < 32; i++) {
		if ( (int)(1<<i) >= (int)(sec->alignment_power)) {
			sec->alignment_power = i;
			break;
		}
	}

	if (hdr->s_flags & STYP_TEXT) {
		sec->flags |= SEC_CODE | SEC_LOAD | SEC_ALLOC;
	}
	if (hdr->s_flags & STYP_DATA) {
		sec->flags |= SEC_DATA | SEC_LOAD | SEC_ALLOC | SEC_IS_WRITEABLE;
	}
	if (hdr->s_flags & STYP_BSS) {
		sec->flags |= SEC_ALLOC | SEC_IS_BSS | SEC_IS_WRITEABLE;
	}
	if (hdr->s_nreloc != 0) {
		sec->flags |= SEC_RELOC;
	}
	if (hdr->s_flags & STYP_NOLOAD) {
	    sec->flags |= SEC_IS_NOLOAD;
	}
	if (hdr->s_flags & STYP_INFO) {
	    sec->flags |= SEC_IS_INFO;
	}
	if (hdr->s_scnptr != 0) {
	    sec->flags |= SEC_HAS_CONTENTS;
	}
	if (abfd->xvec->byteorder_big_p) {
	    sec->flags |= SEC_IS_BIG_ENDIAN;
	}
	if ((abfd->flags & HAS_PIC) && (sec->flags & SEC_CODE))
		sec->flags |= SEC_IS_PI;
	if ((abfd->flags & HAS_PID) && (sec->flags & SEC_DATA))
		sec->flags |= SEC_IS_PI;
	return true;
}


static boolean
DEFUN(coff_mkobject,(abfd),
    bfd *abfd)
{
	set_tdata (abfd, bfd_zalloc (abfd,sizeof(coff_data_type)));
	if (coff_data(abfd) == 0) {
		bfd_error = no_memory;
		return false;
	}
	coff_data(abfd)->relocbase = 0;
	return true;
}


static
bfd_target     *
DEFUN(coff_real_object_p,(abfd, nscns, filhdr, aouthdr),
    bfd      *abfd AND
    unsigned  nscns AND
    FILHDR   *filhdr AND
    AOUTHDR  *aouthdr)
{
	coff_data_type *coff;
	size_t readsize;	/* length of file_info */
	SCNHDR *external_sections;

	/* Build a play area */
	if (coff_mkobject(abfd) != true)
		return 0;
	coff = coff_data(abfd);


	external_sections = (SCNHDR *)bfd_alloc(abfd, readsize = (nscns * SCNHSZ));

	if (bfd_read((PTR)external_sections, 1, readsize, abfd) != readsize) {
		bfd_release(abfd, coff);
		return (bfd_target *)NULL;
	}

	/* Now copy data as required; construct all asections etc */
	coff->relocbase =0;
	coff->raw_syment_count = 0;
	coff->raw_linenos = 0;
	coff->raw_syments = 0;
	coff->sym_filepos =0;
	coff->flags = filhdr->f_flags;

	/* Determine the machine architecture and type.  */
	abfd->obj_machine = abfd->obj_machine_2 = 0;
	switch (filhdr->f_magic) {
	case I960ROMAGIC:
	case I960RWMAGIC:
		abfd->obj_arch = bfd_arch_i960;
		switch (F_I960TYPE & filhdr->f_flags) {
		default:
		    fprintf(stderr,"warning file: %s contains architecture info: 0x%x\n",
			    abfd->filename,filhdr->f_flags & F_I960TYPE);
		case F_I960CORE:  abfd->obj_machine = bfd_mach_i960_core;
		    bfd_set_target_arch(abfd,BFD_960_GENERIC);
		    bfd_set_target_attributes(abfd,BFD_960_MACH_CORE_1);
		    break;
		case F_I960CORE2: abfd->obj_machine = bfd_mach_i960_core2;
		    bfd_set_target_arch(abfd,BFD_960_GENERIC);
		    bfd_set_target_attributes(abfd,BFD_960_MACH_CORE_2);
		    break;
		case F_I960KA:    abfd->obj_machine = bfd_mach_i960_ka_sa;
		    bfd_set_target_arch(abfd,BFD_960_KA);
		    bfd_set_target_attributes(abfd,BFD_960_MACH_KX);
		    break;
		case F_I960KB:    abfd->obj_machine = bfd_mach_i960_kb_sb;
		    bfd_set_target_arch(abfd,BFD_960_KB);
		    bfd_set_target_attributes(abfd,BFD_960_MACH_FP1);
		    break;
		case F_I960CA:    abfd->obj_machine = bfd_mach_i960_ca;
		    bfd_set_target_arch(abfd,BFD_960_CA);
		    bfd_set_target_attributes(abfd,BFD_960_MACH_CX);
		    break;
		case F_I960JX:    abfd->obj_machine = bfd_mach_i960_jx;
		    bfd_set_target_arch(abfd,BFD_960_JA);
		    bfd_set_target_attributes(abfd,BFD_960_MACH_JX);
		    break;
		case F_I960HX:    abfd->obj_machine = bfd_mach_i960_hx;
		    bfd_set_target_arch(abfd,BFD_960_HA);
		    bfd_set_target_attributes(abfd,BFD_960_MACH_HX);
		    break;
		}
		break;

	default:			/* Unreadable input file type */
		abfd->obj_arch = bfd_arch_obscure;
		break;
	}

	if (filhdr->f_nsyms)			abfd->flags |= HAS_SYMS;
	if (filhdr->f_flags & F_CCINFO)		abfd->flags |= HAS_CCINFO;
	if (filhdr->f_flags & F_EXEC)		abfd->flags |= EXEC_P;
	if (!(filhdr->f_flags & F_RELFLG))	abfd->flags |= HAS_RELOC;
	if (!(filhdr->f_flags & F_LNNO))	abfd->flags |= HAS_LINENO;
	if (!(filhdr->f_flags & F_LSYMS))	abfd->flags |= HAS_LOCALS;
	if (filhdr->f_flags & F_PIC)	        abfd->flags |= HAS_PIC;
	if (filhdr->f_flags & F_PID)	        abfd->flags |= HAS_PID;
        if (filhdr->f_flags & F_LINKPID)        abfd->flags |= CAN_LINKPID;
        if (filhdr->f_flags & F_COMP_SYMTAB)    abfd->flags |= SYMTAB_COMPRESSED;

	if (nscns != 0) {
		unsigned int i;
		for (i = 0; i < nscns; i++) {
			struct scnhdr tmp;
			coff_swap_scnhdr_in(abfd, external_sections + i, &tmp);
			make_a_section_from_file(abfd,&tmp,FILHSZ+filhdr->f_opthdr+i*SCNHSZ);
		}
	}

	if (!(abfd->flags & SUPP_W_TIME) && (abfd->mtime == 0)) {
		abfd->mtime = filhdr->f_timdat;
		abfd->mtime_set = true;
	}

	coff->sym_filepos = filhdr->f_symptr;
	coff->str_filepos = filhdr->f_symptr + (filhdr->f_nsyms * SYMESZ);
	coff->symbols = (coff_symbol_type *) NULL;
	bfd_get_symcount(abfd) = filhdr->f_nsyms;
	bfd_get_start_address(abfd) = filhdr->f_opthdr ? aouthdr->entry : 0;
	return abfd->xvec;
}


static bfd_target *
DEFUN(coff_object_p,(abfd),
    bfd *abfd)
{
	int   nscns;
	FILHDR filehdr;
	AOUTHDR opthdr;
	FILHDR internal_f;
	AOUTHDR internal_a;

	bfd_error = system_call_error;

	/* figure out how much to read */
	if (bfd_read((PTR) &filehdr, 1, FILHSZ, abfd) != FILHSZ)
		return 0;

	bfd_swap_filehdr_in(abfd, &filehdr, &internal_f);

	if (I960BADMAG(internal_f)) {
		bfd_error = wrong_format;
		return 0;
	}
#if !defined(NO_BIG_ENDIAN_MODS)
        /* check big-endian/little-endian target order */
        if (abfd->xvec->byteorder_big_p) { /* this is a big-endian bfd_target */
                if (!(F_BIG_ENDIAN_TARGET & internal_f.f_flags)) {
                        bfd_error = wrong_format;
                        return 0;
                }
        } else { /* this is a little-endian bfd_target */
                if ( (F_BIG_ENDIAN_TARGET & internal_f.f_flags)) {
                        bfd_error = wrong_format;
                        return 0;
                }
        }
#endif /* NO_BIG_ENDIAN_MODS */

	nscns =internal_f.f_nscns;

	if (internal_f.f_opthdr) {
		if (bfd_read((PTR) &opthdr, 1,AOUTSZ, abfd) != AOUTSZ) {
			return 0;
		}
		bfd_swap_aouthdr_in(abfd, &opthdr, &internal_a);
	}

	/* Seek past the opt hdr stuff */
	bfd_seek(abfd, internal_f.f_opthdr + FILHSZ, SEEK_SET);

	if (internal_f.f_opthdr != 0 && AOUTSZ != internal_f.f_opthdr)
		return (bfd_target *)NULL;

	return coff_real_object_p(abfd, nscns, &internal_f, &internal_a);
}



/* 
 * Takes a bfd and a symbol, returns a pointer to the coff specific area
 * of the symbol if there is one.
 */
static coff_symbol_type *
DEFUN(coff_symbol_from,(abfd, symbol),
    bfd     *abfd AND
    asymbol *symbol)
{
	if ( (symbol->the_bfd->xvec->flavour != bfd_target_coff_flavour_enum)
	    ||   (symbol->the_bfd->tdata == (PTR)NULL)) {
	    return (coff_symbol_type *)NULL;
	}
	return ((coff_symbol_type *) symbol);
}


static void 
DEFUN(coff_count_linenumbers,(abfd),
    bfd *abfd)
{
	unsigned int   i;
	asymbol   **p;
	unsigned int limit = bfd_get_symcount(abfd);
	asection *s = abfd->sections->output_section;

	while (s) {
		BFD_ASSERT(s->lineno_count == 0);
		s = s->next;
	}

	for (p = abfd->outsymbols, i = 0; i < limit; i++, p++) {
		asymbol *q_maybe = *p;
		if (q_maybe->the_bfd->xvec->flavour == bfd_target_coff_flavour_enum) {
			coff_symbol_type *q = coffsymbol(q_maybe);
                        if ((q->symbol.section) &&
				(q->symbol.section->output_section) &&
				(q->symbol.section->output_section->flags & SEC_IS_DSECT)) {
				/* DSECT sections don't remember line
				numbers */
				continue;
			}
			if (q->lineno) {
				/*
				 * This symbol has a linenumber, increment
				 * the owning section's linenumber count.
				 */
				alent *l = q->lineno;
				q->symbol.section->output_section->lineno_count++;
				l++;
				while (l->line_number) {
					q->symbol.section->output_section->lineno_count++;
					l++;
				}
			}
		}
	}
}


static void
DEFUN(fixup_symbol_value,(asym, s),
    asymbol *asym AND
    combined_entry_type *s)
{
	SYMENT *syment = &s->u.syment;

	if (asym->flags & BSF_FORT_COMM) {
		/* a common symbol is undefined with a value */
		syment->n_scnum = N_UNDEF;
		syment->n_value = asym->value;

	} else if (!(asym->flags & BSF_LOCAL) && asym->flags & BSF_DEBUGGING) {
		syment->n_value = asym->value;

	} else if (asym->flags & BSF_UNDEFINED) {
		syment->n_scnum = N_UNDEF;
		syment->n_value = 0;

	} else if (asym->flags & BSF_ABSOLUTE) {
		syment->n_scnum = N_ABS;
		syment->n_value = asym->value;
	} else if (asym->section && asym->section->output_section) {
		syment->n_scnum = 
		    asym->section->output_section->secnum + 1;

		syment->n_value = asym->value
				    + asym->section->output_offset
				    + asym->section->output_section->vma;

		/* Also set bal entry point address in 2nd auxent of leafproc
		 * symbols.  Note that the bal address in the native symbol's
	         * auxent was converted to an offset from the call address when
		 * the symbol was first read in.
		 */
		if ( (syment->n_sclass == C_LEAFSTAT)
		||   (syment->n_sclass == C_LEAFEXT) ){
			AUXENT *aux = &((s+2)->u.auxent);
			aux->x_bal.x_balntry += syment->n_value;
		}
	}
	else {
		syment->n_scnum = N_UNDEF;
		syment->n_value = asym->value;
	}
}

/* Run through all the symbols in the symbol table and work out what
 * their indices in the output symbol table will be.
 *
 * COFF requires that each C_FILE symbol point to the next one in the
 * table, and that the last one point to the first external symbol.
 * Do that here too.
 */
static void
DEFUN(coff_renumber_symbols,(abfd),
    bfd *abfd)
{
	unsigned int idx;	/* Index of symbol in output file */
	int i;
	combined_entry_type	*s;
	coff_symbol_type	*coff_symptr;
	asymbol				*asym;
	SYMENT *prev_file         = (SYMENT *)NULL;
	unsigned int native_index = 0;
	int first_ext             = 1;

	for (idx = 0; idx < bfd_get_symcount(abfd); idx++) {
		coff_symptr = coff_symbol_from(abfd,(abfd->outsymbols)[idx]);
		if (!coff_symptr || !fetch_native_coff_info(&coff_symptr->symbol)) {
		    asymbol *sym = (abfd->outsymbols)[idx];
		    /* The first external symbol might be a global symbol
		       made by the linker or input from a non coff file
		       If so, mark the prev_file chain with this
		       symbol as for C_EXT native types: */
		    if (first_ext && (sym->flags & (BSF_GLOBAL|BSF_FORT_COMM))) {
			if ( prev_file )
				prev_file->n_value = native_index;
			first_ext = 0;
		    }
		    /* 3 == 1 syment + 2 auxes. */
		    if (sym->flags & (BSF_HAS_SCALL | BSF_HAS_BAL))
			    native_index += 3;
		    /* 2 == 1 syment + 1 auxes. */
		    else if (sym->flags & BSF_SECT_SYM)
			    native_index += 2;
		    /* 1 == 1 syment. */
		    else
			    native_index += 1;
		    continue;
		}

		s = fetch_native_coff_info(&coff_symptr->symbol);
		asym = &coff_symptr->symbol;

		switch (s->u.syment.n_sclass) {
		case C_FILE:
			if (prev_file) {
				prev_file->n_value = native_index;
			}
			prev_file = &(s->u.syment);
			break;
		case C_EXT:
			/* If this is the external sym that is not a function,
			 * no more .file symbols should follow it.
			 * In that case point the final (previous) .file at it.
			 */
			if (first_ext && !ISFCN(s->u.syment.n_type)) {
				if ( prev_file ){
					prev_file->n_value = native_index;
				}
				first_ext = 0;
			}
			break;
		}

		if (s->u.syment.n_sclass != C_FILE) {
			/* Modify sym value according to section/type */
			fixup_symbol_value(asym,s);
		}

		for (i = 0; i < s->u.syment.n_numaux + 1; i++) {
			s[i].offset = native_index ++;
		}
	}
}


/*
 * Run through the symbol table again, changing all pointers to
 * entries to indices of those entries in the output symbol table.
 */
static void 
DEFUN(coff_mangle_symbols,(abfd),
    bfd *abfd)
{
	unsigned int idx;	/* Index of symbol in internal symbol table */
	unsigned int symbol_count = bfd_get_symcount(abfd);
	asymbol **symbol_ptr_ptr = abfd->outsymbols;
	unsigned int native_index = 0;


	for (idx = 0; idx < symbol_count; idx++) {
		coff_symbol_type *coff_symbol_ptr;

		coff_symbol_ptr = coff_symbol_from(abfd, symbol_ptr_ptr[idx]);
		if (coff_symbol_ptr && fetch_native_coff_info(&coff_symbol_ptr->symbol) ) {
			int i;
			combined_entry_type *s = fetch_native_coff_info(&coff_symbol_ptr->symbol);
			if (s != NULL) {
				for (i = 0; i < s->u.syment.n_numaux ; i++) {
					combined_entry_type *a = s + i + 1;
					combined_entry_type *tmp;

					if (a->fix_tag) {
						tmp = (combined_entry_type *)
							(a->u.auxent.AUX_TAGNDX);
						a->u.auxent.AUX_TAGNDX = tmp->offset;
					}
					if (a->fix_end) {
						tmp = (combined_entry_type *)
							(a->u.auxent.AUX_ENDNDX);
						a->u.auxent.AUX_ENDNDX = tmp->offset;
					}
				}
			}
		}
	}
}

static unsigned string_size;  /* Running Sizeof the string_table if we are not
				 compressing the string table. */
static hashed_str_tab *long_string_table_hash_table; /* Else, if we are compressing the string
							table, we use this puppy. */

static void
DEFUN(coff_compress_long_string_table,(abfd),
    bfd *abfd)
{
    int i;
    asymbol **ptr;

    long_string_table_hash_table = _bfd_init_hashed_str_tab(abfd,8);

    for (ptr=abfd->outsymbols;*ptr;ptr++) {
	asymbol *p = *ptr;
	int string_length = strlen(p->name);
	int maxlen;
	coff_symbol_type *c_symbol = coff_symbol_from(abfd, p);
	
	if ( c_symbol
	    &&   fetch_native_coff_info(&c_symbol->symbol)
	    &&   (fetch_native_coff_info(&c_symbol->symbol)->u.syment.n_sclass == C_FILE) )
		maxlen = FILNMLEN;
	else
		maxlen = SYMNMLEN;
	if (string_length > maxlen) {
	    int long_string_table_offset =
		    _bfd_lookup_hashed_str_tab_offset(abfd,
						      long_string_table_hash_table,
						      p->name,string_length);
	    if (c_symbol && fetch_native_coff_info(&c_symbol->symbol)) {
		combined_entry_type *nat = fetch_native_coff_info(&c_symbol->symbol);
		
		if (nat->u.syment.n_sclass == C_FILE) {
		    AUXENT *auxent = &(nat+1)->u.auxent;
		    auxent->x_file.x_n.x_offset = long_string_table_offset + 4;
		    auxent->x_file.x_n.x_zeroes = 0;
		}
		else { 	/* NOT A C_FILE SYMBOL */
		    nat->u.syment.n_offset = long_string_table_offset + 4;
		    nat->u.syment.n_zeroes = 0;
		}
	    }
	    /* The symbol is not native.  We will have to fix up the non native symbols
	       in coff_fix_symbol_name() when writing the symbol. */
	}
    }
}

/* If we are doing tags compression, then we also do string table compression.
 Else, we do no string table compression. */

#define MERGE_DUPLICATED_TAGS(BFD_PTR) (((BFD_PTR->flags & SYMTAB_COMPRESSED) == 0) && \
	                                ((BFD_PTR->flags & DO_NOT_STRIP_ORPHANED_TAGS) == 0) && \
					(BFD_PTR->flags & STRIP_DUP_TAGS))

#define COMPRESS_STRINGS(BFD_PTR)      (MERGE_DUPLICATED_TAGS(BFD_PTR) || \
					(BFD_PTR->flags & SYMTAB_COMPRESSED))

static void
DEFUN(coff_fix_symbol_name,(abfd, symbol, native, alien),
    bfd *abfd AND
    asymbol *symbol AND
    combined_entry_type *native  AND
    int alien)
{
    unsigned int    name_length;
    AUXENT *auxent;
    CONST char *  name = symbol->name;

    if (name == (char *) NULL)
	    /* coff symbols always have names: make one up */
	    name =  symbol->name = "strange";
    name_length = strlen(name);

    if (native->u.syment.n_sclass == C_FILE) {
	strncpy(native->u.syment.n_name, ".file", SYMNMLEN);
	auxent = &(native+1)->u.auxent;

	if (name_length <= FILNMLEN)
		strncpy(auxent->AUX_FNAME, name, FILNMLEN);
	else if (!COMPRESS_STRINGS(abfd)) {
	    /* If we are doing tags compression, then we also do string table compression.
	       Else, we do no string table compression. */
	    auxent->x_file.x_n.x_offset = string_size + 4;
	    auxent->x_file.x_n.x_zeroes = 0;
	    string_size += name_length + 1;
	}
    } else {				/* NOT A C_FILE SYMBOL */
	if (name_length <= SYMNMLEN)
		/* This name will fit into the symbol neatly */
		strncpy(native->u.syment.n_name,symbol->name,SYMNMLEN);
	else {
	    if (!COMPRESS_STRINGS(abfd)) {
		/* If we are doing tags compression, then we also do string table compression.
		   Else, we do no string table compression. */
		native->u.syment.n_offset = string_size + 4;
		native->u.syment.n_zeroes = 0;
		string_size += name_length + 1;
	    }
	    else {
		if (alien) {
		    native->u.syment.n_offset =
			    _bfd_lookup_hashed_str_tab_offset(abfd,
							      long_string_table_hash_table,
							      name,name_length) + 4;
		    native->u.syment.n_zeroes = 0;
		}
	    }
	}
    }
}

static unsigned int 
DEFUN(coff_write_symbol,(abfd, symbol, native, written, alien),
    bfd *abfd AND
    asymbol *symbol AND
    combined_entry_type *native AND
    unsigned int written AND
    int alien)
{
	unsigned int    numaux = native->u.syment.n_numaux;
	int             type = native->u.syment.n_type;
	int             class =  native->u.syment.n_sclass;
	unsigned int j;

	coff_fix_symbol_name(abfd, symbol, native, alien);
	coff_write_sym(abfd, &native->u.syment);
	for (j = 0; j != native->u.syment.n_numaux;  j++) {
		coff_write_aux(abfd, &((native+j+1)->u.auxent), type, class, j);
	}

	symbol->value = written;
	return written + 1 + numaux;
}


/*
 * A symbol has been created by the loader, or come from a non-coff
 * format. It has no native element to inherit, make our own.
 */
static unsigned int
DEFUN(coff_write_alien_symbol,(abfd, symbol, written, flags),
    bfd *abfd AND
    asymbol *symbol AND
    unsigned int written AND
    unsigned int flags)
{
	combined_entry_type native[3];
	SYMENT *sym = &native[0].u.syment;

	memset(native,0,sizeof(native));
	sym->n_type = 0;
	sym->n_flags = flags;
	sym->n_numaux = 0;
	sym->n_sclass = (symbol->flags & BSF_LOCAL) ? C_STAT : C_EXT;

	if (symbol->flags & BSF_ABSOLUTE) {
		sym->n_scnum  =  N_ABS;
		sym->n_value =  symbol->value;
	} else if (symbol->flags & (BSF_UNDEFINED | BSF_FORT_COMM)) {
		sym->n_scnum =  N_UNDEF;
		sym->n_value =  symbol->value;
	} else if (symbol->section && symbol->section->output_section) {
	    sym->n_scnum = symbol->section->output_section->secnum + 1;
	    sym->n_value = symbol->value
		    + symbol->section->output_section->vma
			    + symbol->section->output_offset;
	    if (symbol->flags & BSF_SECT_SYM) {
		AUXENT *first_aux = &(native[1].u.auxent);

		sym->n_numaux = 1;
		sym->n_flags |= F_SECT_SYM;
		first_aux->x_scn.x_scnlen = symbol->section->size;
		first_aux->x_scn.x_nreloc = symbol->section->reloc_count;
	    }
	} else if (!(symbol->flags & BSF_HAS_SCALL)) {
	    /* remove name so it doesn't take up any space */
	    symbol->name = "";
	}
	if (symbol->flags & (BSF_HAS_SCALL | BSF_HAS_BAL)) {
	    AUXENT *second_aux = &(native[2].u.auxent);

	    sym->n_numaux = 2;
	    if (symbol->flags & BSF_HAS_SCALL) {
		sym->n_sclass = C_SCALL;
		second_aux->x_sc.x_stindx = symbol->related_symbol->value;
	    }
	    else {
		sym->n_sclass = (symbol->flags & BSF_LOCAL) ? C_LEAFSTAT : C_LEAFEXT;
		second_aux->x_bal.x_balntry = symbol->related_symbol->value +
			symbol->section->output_offset +
				symbol->section->output_section->vma;
	    }
	}
	return coff_write_symbol(abfd, symbol, native, written, 1);
}


static unsigned int 
DEFUN(coff_write_native_symbol,(abfd, symbol,   written),
    bfd *abfd AND
    coff_symbol_type *symbol AND
    unsigned int written)
{
	/*
	 * Does this symbol have an ascociated line number - if so then
	 * make it remember this symbol index. Also tag the auxent of
	 * this symbol to point to the right place in the lineno table
	 */
        combined_entry_type *native = fetch_native_coff_info(&symbol->symbol);
	alent *lineno = symbol->lineno;
	unsigned int count = 0;

	if (!(abfd->flags & STRIP_LINES) && lineno) {
		lineno[count].u.offset = written;
		if (native->u.syment.n_numaux) {
			AUXENT  *a = &((native+1)->u.auxent);

			a->AUX_LNNOPTR =  
			    symbol->symbol.section->output_section->moving_line_filepos;
		}

		/* Count and relocate all other linenumbers */
		count++;
		while (lineno[count].line_number) {
			lineno[count].u.offset +=
			    symbol->symbol.section->output_section->vma
			    + symbol->symbol.section->output_offset;
			count++;
		}
		symbol->symbol.section->output_section->moving_line_filepos +=
						    count * LINESZ;
	}
	return coff_write_symbol(abfd, &( symbol->symbol), native, written, 0);
}


static void 
DEFUN(coff_write_symbols,(abfd,flags),
    bfd *abfd AND
    unsigned int flags)
{
    unsigned int i;
    unsigned int written;
    asymbol **p;
    unsigned int size = 0;

    unsigned int limit = bfd_get_symcount(abfd);
    
    if (!COMPRESS_STRINGS(abfd))
	    string_size = 0;

    /* Seek to the right place */

    bfd_seek(abfd, obj_sym_filepos(abfd), SEEK_SET);

    /* Output all the symbols we have */
    
    written = 0;
    for (p = abfd->outsymbols, i = 0; i < limit; i++, p++) {
	asymbol *symbol = *p;
	coff_symbol_type *c_symbol = coff_symbol_from(abfd, symbol);

	if (c_symbol == (coff_symbol_type *) NULL
	    ||  fetch_native_coff_info(&c_symbol->symbol) == (combined_entry_type *)NULL ) {
	    written = coff_write_alien_symbol(abfd,symbol,written,flags);
	} else {
	    
	    /* save the bit that says whether this symbol represents
	       the name of a section in the original assembler
	       output of this source */
	    fetch_native_coff_info(&c_symbol->symbol)->u.syment.n_flags &= F_SECT_SYM;
	    
	    /* otherwise, make it reflect the flags field of the
	       file header, for reasons lost in the mists of antiquity */
	    fetch_native_coff_info(&c_symbol->symbol)->u.syment.n_flags |= flags;
	    written = coff_write_native_symbol(abfd,c_symbol,written);
	    
	}
    }
    bfd_get_symcount(abfd) = written;
    
    /* Now write out the long strings */

    if (COMPRESS_STRINGS(abfd))
	    string_size = long_string_table_hash_table->current_size;
    
    if (string_size != 0) {
	PUTWORD( abfd, string_size+4, size );
	bfd_write((PTR) &size, 1, sizeof(size), abfd);
	
	if (!COMPRESS_STRINGS(abfd)) {
	    for (p = abfd->outsymbols, i = 0; i < limit; i++, p++) {
		int maxlen;
		asymbol *q = *p;
		size_t name_length = strlen(q->name);
		coff_symbol_type *c_symbol = coff_symbol_from(abfd, q);
		
		if ( c_symbol
		    &&   fetch_native_coff_info(&c_symbol->symbol)
		    &&   (fetch_native_coff_info(&c_symbol->symbol)->u.syment.n_sclass == C_FILE) )
			maxlen = FILNMLEN;
		else
			maxlen = SYMNMLEN;
		if (name_length > maxlen)
			bfd_write((PTR)(q->name),1,name_length+1,abfd);
	    }
	}
	else
		bfd_write(long_string_table_hash_table->strings,1,string_size,abfd);
    }
    else { /* Zero length means we still need to generate a
	    * length field of 4 bytes, with zeroes in them.
	    */
	size = 0;
	PUTWORD(abfd, 0, size);
	bfd_write((PTR) &size, 1, sizeof(size), abfd);
    }
    if (COMPRESS_STRINGS(abfd))
	    _bfd_release_hashed_str_tab(abfd,long_string_table_hash_table);
}

static void 
	DEFUN(coff_write_relocs,(abfd),
	      bfd *abfd)
{
    asection *s;
    unsigned int i;
    struct reloc dst;
    arelent **p;
    
    for (s = abfd->sections; s; s = s->next) {
	p = s->orelocation;
	bfd_seek(abfd, s->rel_filepos, SEEK_SET);
	for (i = 0; i < s->reloc_count; i++) {
	    struct reloc  n;
	    arelent  *q = p[i];
	    
	    memset((PTR)&n, 0, sizeof(n));
	    n.r_vaddr = q->address + s->vma;
	    if (q->sym_ptr_ptr)
		n.r_symndx = (int) (*(q->sym_ptr_ptr))->value;
	    n.r_type = q->howto->type;
	    bfd_write_reloc(abfd, &n);
	}
    }
}

static void 
DEFUN(coff_write_linenumbers,(abfd),
    bfd *abfd)
{
	asection *s;
	asymbol **q;

	for (s = abfd->sections; s; s = s->next) {
		if (s->lineno_count == 0) {
			continue;
		}

		bfd_seek(abfd, s->line_filepos, SEEK_SET);

		/* Find all the linenumbers in this section
		 */
		for ( q = abfd->outsymbols; *q; q++ ){
			asymbol *p = *q;
			alent *l;
			
			if ((!p->section) || (!p->section->output_section) ||
				 (!p->section->output_section->name)) {
				/* Some symbols, for example, those out of
				libraries, do not have these structures set
				up and are not relevant to this operation */
				continue;
			}
			if (strcmp(s->name,p->section->output_section->name)) {
				/* We only want symbols from this section */
				continue;
			}
			l = BFD_SEND(p->the_bfd,_get_lineno,(p->the_bfd,p));
			if (l) {
				/* Found a linenumber entry for this section,
				output */
				LINENO out;
				out.l_lnno = 0;
				out.l_addr.l_symndx = l->u.offset;
				coff_write_lineno(abfd, &out);
				l++;
				while (l->line_number) {
					out.l_lnno = l->line_number;
					out.l_addr.l_symndx = l->u.offset;
					coff_write_lineno(abfd, &out);
					l++;
				}
			}
		}
	}
}


static asymbol *
coff_make_empty_symbol(abfd)
    bfd *abfd;
{
	coff_symbol_type *new;

	new = (coff_symbol_type *) bfd_zalloc(abfd, sizeof(coff_symbol_type));
	if (new == NULL) {
		bfd_error = no_memory;
		return (NULL);
	}
	new->symbol.the_bfd = abfd;
	return &new->symbol;
}


static void 
DEFUN(coff_print_symbol,(ignore_abfd, file, symbol, how),
    bfd            *ignore_abfd AND
    FILE           *file AND
    asymbol        *symbol AND
    bfd_print_symbol_enum_type how)
{
	CONST char *secname;

	switch (how) {
	case bfd_print_symbol_name_enum:
		fprintf(file, "%s", symbol->name);
		break;
	case bfd_print_symbol_type_enum:
		fprintf(file, "coff %lx %lx",
			(unsigned long) fetch_native_coff_info(symbol),
			(unsigned long) coffsymbol(symbol)->lineno);
		break;
	case bfd_print_symbol_all_enum:
		secname = symbol->section ? symbol->section->name : "*abs" ;
		bfd_print_symbol_vandf((PTR) file, symbol);
		fprintf(file, " %-5s %s %s %s",
				secname,
				fetch_native_coff_info(symbol) ? "n" : "g",
				coffsymbol(symbol)->lineno ? "l" : " ",
				symbol->name);
		break;
	}
}

static alent   *
DEFUN(coff_get_lineno,(ignore_abfd, symbol),
    bfd            *ignore_abfd AND
    asymbol        *symbol)
{
	return coffsymbol(symbol)->lineno;
}


static          boolean
DEFUN(coff_set_flags,(abfd, flagsp),
    bfd            *abfd AND
    unsigned short *flagsp)
{
	if (abfd->obj_arch != bfd_arch_i960) {
		return false;
	}

	switch (abfd->obj_machine) {
	case bfd_mach_i960_core:	*flagsp = F_I960CORE;	break;
	case bfd_mach_i960_core2:	*flagsp = F_I960CORE2;	break;
	case bfd_mach_i960_kb_sb:	*flagsp = F_I960KB;	break;
	case bfd_mach_i960_ca:		*flagsp = F_I960CA;	break;
	case bfd_mach_i960_jx:		*flagsp = F_I960JX;	break;
	case bfd_mach_i960_hx:		*flagsp = F_I960HX;	break;
	case bfd_mach_i960_ka_sa:	*flagsp = F_I960KA;	break;
	default:
		return false;
	}
	return true;
}


static          boolean
DEFUN(coff_set_arch_mach,(abfd, arch, machine, real_machine),
    bfd         *abfd AND
    enum	bfd_architecture arch AND
    unsigned long   machine           AND
    unsigned long   real_machine)
{
	unsigned short    dummy;

	abfd->obj_arch = arch;
	abfd->obj_machine = machine;
	bfd_set_target_arch(abfd,real_machine);
	if (arch != bfd_arch_unknown && coff_set_flags(abfd,&dummy) != true)
		return false;		/* We can't represent this type */
	return true;
}


/* Calculate the file position for each section. */

static void 
DEFUN(coff_compute_section_file_positions,(abfd),
    bfd *abfd)
{
	asection *current;
	file_ptr  sofar = FILHSZ;

	if (bfd_get_start_address(abfd)) {
		/*
		 * A start address may have been added to the original file.
		 * In this case it will need an optional header to record it.
		 */
		abfd->flags |= EXEC_P;
	}
	if (abfd->flags & EXEC_P){
		sofar += AOUTSZ;
	}

	sofar += abfd->section_count * SCNHSZ;
	for (current = abfd->sections; current ; current = current->next) {
		/* Only deal with sections which have contents */
		if ((!(current->flags & SEC_HAS_CONTENTS)) ||
			(current->flags & SEC_IS_DSECT) ||
			(current->flags & SEC_IS_NOLOAD))
			continue;

		current->filepos = sofar;
		sofar += current->size;
	}
	obj_relocbase(abfd) = sofar;
}

/* The following is the symbol table compression code.  Duplicated and orphaned
   tags are removed from the coff symbol table. */

/* Hash table for coff tags. Will contain only unique tags. */
#define CET_HASH_TABLE_SIZE 199
static struct cet_list {
    combined_entry_type *cet;
    struct cet_list *next_cet;
} **cet_hash_table;

/* Used to detect cyclical references in tags when comparing if two tags are equal: */
static combined_entry_type **cet_stack;
static int cet_stack_max = 10;

/* Return the original C tag name, remove futz from compiler. */
static char *
DEFUN(tagname,(abfd,s),
      bfd *abfd AND
      SYMENT *s)
{
    char *name,*endofname;
    int namelength;
    static char *buff[2] = {NULL,NULL};
    static unsigned bufflength[2] = {0,0},current_buff = 0;

    name = s->n_ptr;
    /* If it aint a tagname, let's just return the syms name. */
    if (!ISTAG(s->n_sclass))
	    return name;
    current_buff = (current_buff+1) & 1;
    /* We will copy the name into one of two static buffers, buff. */
    if ((namelength=strlen(name)+1) >= bufflength[current_buff]) {
	if (!bufflength[current_buff])
		buff[current_buff] = bfd_alloc(abfd,bufflength[current_buff]=namelength);
	else
		buff[current_buff] = bfd_realloc(abfd,buff[current_buff],
						 bufflength[current_buff]=namelength);
    }
    strcpy(buff[current_buff],name);
    name = buff[current_buff];

    /* Check to see if it is .1fake e.g. . */
    if (*name == '.' && isdigit(*(name+1))) {
	char *s = name+1;

	while (isdigit(*s))
		s++;
	if (!strcmp("fake",s))
		return "fake";
    }
    /* Check to see if '_[digits]' follows name. */
    if (name && *name) {
	for (endofname=name;*endofname;endofname++)
		;
	endofname--;
	if (name != endofname && isdigit(*endofname)) {
	    while (isdigit(*endofname))
		    endofname--;
	    if (*endofname == '_') {
		*endofname = 0;
		return name;
	    }
	}
    }
    return name;
}

/* This goody compares two coff syments and returns 1 if they essentially contain the
   same information together with everthing it references.  Else it returns 0. */
static int
DEFUN(tags_are_equal,
      (abfd,tag1,tag2,tae_depth),
      bfd *abfd                  AND
      combined_entry_type *tag1  AND
      combined_entry_type *tag2  AND
      int tae_depth)
{
    SYMENT *s1 = &tag1->u.syment;
    SYMENT *s2 = &tag2->u.syment;
    int i,tags_in_stack = 0;

    /* Clearly if the same pointer is passed, the two tags are the same.
     But, also, if we previously looked at these two symbols, and we previously
     found they were the same, there is no reason to look at them again.  They are
     still the same. */
    if ((tag1 == tag2)                                 ||   /* Case 0. */
	(tag1 == tag2->tags_are_equal_equivalent_tag)  ||   /* Case 1. */
	(tag1->tags_are_equal_equivalent_tag == tag2)  ||   /* Case 2. */
	(tag1->tags_are_equal_equivalent_tag &&             /* Case 3. */
	 tag1->tags_are_equal_equivalent_tag ==
	 tag2->tags_are_equal_equivalent_tag))
	    return 1;

    /* If the tags are both in the hash table, then we have previously looked at
       both of them and we can guarantee they they are not equal. */
    if ((tag1->tag_flags & TAG_IN_HASH_TABLE) &&
	(tag2->tag_flags & TAG_IN_HASH_TABLE))
	    return 0;

    /* Check for recursive references.  Only if two structures
       are referring to each other in identically lockstep fashion are they
       equal.  However, if either attempt to recurse in different order, then
       the two tags are not same.

       The same:
       tag1:                tag2:
       struct y {           struct y {
            struct x *z;               struct x *z;
            ...                        ...
       struct x {           struct x {
            struct y *z;               struct y *z;
            ...                        ...

       different:
       tag1:                tag2:
       struct y {           struct y {
            struct x *z;               struct x *z;
            ...                        ...
       struct x {           struct x {
            struct y *z;               struct x *z;
            ...                        ...
	    */

    for (i=0;tags_in_stack < 2 && i < tae_depth;i += 2)
	    if (cet_stack[i] == tag1 &&
		cet_stack[i+1] == tag2)
		    return 1;
	    else
		    tags_in_stack += (cet_stack[i] == tag1) + (cet_stack[i+1] == tag2);

    if (tags_in_stack == 2)
 	    return 1;

 tryagain:
    if (tae_depth+2 < cet_stack_max) {
	cet_stack[tae_depth++] = tag1;
	cet_stack[tae_depth++] = tag2;
    }
    else {
	cet_stack =
		(combined_entry_type **)
			bfd_realloc(abfd,cet_stack,(cet_stack_max *= 2) * sizeof(combined_entry_type *));
	goto tryagain;
    }

    /* Different sections. */
    if (s1->n_scnum != s2->n_scnum)
	    return 0;
    /* Different types. */
    if (s1->n_type != s2->n_type)
	    return 0;
    /* Different values: */
    if (s1->n_value != s2->n_value)
	    return 0;
    /* Different storage classes. */
    if (s1->n_sclass != s2->n_sclass)
	    return 0;
    /* Different number of auxents. */
    if (s1->n_numaux != s2->n_numaux)
	    return 0;

    /* Different names -> the tags are different. */
    if (strcmp(tagname(abfd,s1),tagname(abfd,s2)))
	return 0;

    /* Check auxents (breadth first): */

    for (i=1;i <= s1->n_numaux;i++) {
	AUXENT *a1,*a2;
	a1 = &(tag1+i)->u.auxent;
	a2 = &(tag2+i)->u.auxent;

	/* If not both of the fix tags, then they are different. */
	if ((tag1+i)->fix_tag != (tag2+i)->fix_tag)
		return 0;

	/* If the rest of the auxent is treated as an array... */
	if (has_ary(s1->n_type)) {
	    int j;

	    for (j=0;j < 4;j++)
		    if (a1->AUX_DIMEN[j] != a2->AUX_DIMEN[j])
			    return 0;
	}
	/* Look at the size of the aux... Notice how we sneakily swipe the size
	 of the symbol if only one symbol does not know about the size.  However,
	 if both symbols know about a size, and they differ, this implies there
	 are two different types. */
 	if (a1->AUX_SIZE != a2->AUX_SIZE) {
 	    if (a2->AUX_SIZE == 0)
 		    a2->AUX_SIZE = a1->AUX_SIZE;
 	    else if (a1->AUX_SIZE == 0)
		    a1->AUX_SIZE = a2->AUX_SIZE;
	    else
 		    return 0;
 	}
    }

    /* Now, do depth of the aux's: */
    for (i=1;i <= s1->n_numaux;i++)
 	    /* If the auxent references another tag, let's look at it recursively: */
 	    if ((tag1+i)->fix_tag && !tags_are_equal(abfd,
 						     (combined_entry_type *)((tag1+i)->u.auxent.AUX_TAGNDX),
 						     (combined_entry_type *)((tag2+i)->u.auxent.AUX_TAGNDX),
 						     tae_depth))
 		    return 0;

 
    /* For structure union and enumeration tags, look at the structure,
       union and enumeration elements. These are the CMOE, C_MOS and C_MOU
       storage classes.*/
    if (ISTAG(s1->n_sclass))
	    for (i=1;;i++) {
		SYMENT *tmp = &(tag1+s1->n_numaux+i)->u.syment;

		if (!tags_are_equal(abfd,tag1+s1->n_numaux+i,tag2+s1->n_numaux+i,tae_depth))
			return 0;
		if (tmp->n_sclass == C_EOS)
			break;
		i += tmp->n_numaux;
	    }

    /* The two tags have to be the same, we looked at everything that mattered.
     Thus we set the equivalent_tag of one another symbols so as to save us time
     if we try to look at these two again: */
    tag1->tags_are_equal_equivalent_tag = tag2;
    tag2->tags_are_equal_equivalent_tag = tag1;
    return 1;
}

static void add_key(hk_ptr,ptr,n)
    unsigned int *hk_ptr;
    unsigned char *ptr;
    int n;
{
    int i;

    for (i=0;i < n;i++)
	    (*hk_ptr) += (*ptr);
}

/* Make a hash key out of a tag for storage and retrieval from the hash table. */
static unsigned int
DEFUN(hash_key,(abfd,tag),bfd *abfd AND combined_entry_type *tag)
{
    char *name = tagname(abfd,&(tag->u.syment));
    int name_length = strlen(name);
    int i;
    unsigned int hk = 0;

    add_key(&hk,name,strlen(name));
    add_key(&hk,&tag->u.syment.n_sclass,sizeof(tag->u.syment.n_sclass));
    if (ISTAG(tag->u.syment.n_sclass))   /* We have to have this in order to accomadate
					    typedefs. */
	    for (i=1;;i++) {
		SYMENT *member_syment = &(tag+tag->u.syment.n_numaux+i)->u.syment;

		name = tagname(abfd,member_syment);
		add_key(&hk,name,strlen(name));
		add_key(&hk,&member_syment->n_type,sizeof(member_syment->n_type));
		if (member_syment->n_sclass == C_EOS)
			break;
		i += member_syment->n_numaux;
	    }
    return hk % CET_HASH_TABLE_SIZE;
}

/* The following code looks in the hash table if a tag of the indicated type already exists.
   If it already exists, then it returns the element.  Else, it installs the tag into the
   hash table. */
static combined_entry_type *
DEFUN(lookup_tag,(abfd,tag),
      bfd *abfd                AND
      combined_entry_type *tag)
{
    /* An optmization.  If the tag we are looking for is already in the hash table,
       or, if it is already referenced, then just return it because we will not be
       able to find one just like it, because we already looked for one. */
    if (tag->tag_flags & (TAG_IN_HASH_TABLE | TAG_REFERENCED))
	    return tag;
    /* Else, if we already know the tag in the hash table for this tag, then
       return it. */
    else if (tag->lookup_equivalent_tag)
 	    return tag->lookup_equivalent_tag;
    else {
	/* Else, we have to look up the tag in the hash table, and compare all
	   the buckets. */
	int hk = hash_key(abfd,tag);
	struct cet_list **p;

	p = &(cet_hash_table[hk]);
	while (p && *p) {
	    if (tags_are_equal(abfd,tag,(*p)->cet,0))
		    /* We found that it is already in the table.  Let's save off
		       the equivalent tag in case it is referenced in the future. */
		    return tag->lookup_equivalent_tag = (*p)->cet;
	    p = &((*p)->next_cet);
	}
	/* We did not find it in the hash table.  Let's add it: */
	(*p) = (struct cet_list *) bfd_zalloc(abfd,sizeof(struct cet_list));
	tag->tag_flags |= TAG_IN_HASH_TABLE;
	return (*p)->cet = tag;
    }
}

/* Marks all of the tags that a tag can reference as they are referenced.
   This is typically used for structure pointers within structures:

   struct x {
       int i;
   };

   struct y {
       struct x *z;
   } a;

   when the tag reference for struct y is obtained, it is marked referenced by lookup_tag().
   But, struct x is considered part of the closure of struct y, since struct x refers to it
   AND struct x is not marked by lookup tag.

   So, here, we mark it.

   Note that here, tag is a syment and not necessairily a tag.

   This code can sever previous set up tags but it will only do so
   with equivalent tags (per lookup_tag()).
*/
static void
DEFUN(mark_tag_closure_as_referenced,
      (abfd,tag),
      bfd *abfd                  AND
      combined_entry_type *tag)
{
    SYMENT *s = &tag->u.syment;
    int i;

    /* An optimization.  If we have already marked this tag as referenced, there is no need
       to mark it again. */
    if (tag->tag_flags & TAG_REFERENCED)
	    return;
    tag->tag_flags |= TAG_REFERENCED;

    /* Mark the auxents of this tag (this code will be reached
       when mark_tag_closure_as_referenced() is called recursively. */
    for (i=1;i <= s->n_numaux;i++) {
	AUXENT *a;
	a = &(tag+i)->u.auxent;

	if ((tag+i)->fix_tag) {
	    if (abfd->flags & STRIP_DUP_TAGS) {
		combined_entry_type *t = lookup_tag(abfd,
						    (combined_entry_type *)((tag+i)->u.auxent.AUX_TAGNDX));
		(tag+i)->u.auxent.AUX_TAGNDX = (long) t;
	    }
	    mark_tag_closure_as_referenced(abfd,(combined_entry_type *)((tag+i)->u.auxent.AUX_TAGNDX));
	}
    }

    /* For structure union and enumeration tags, mark the structure,
       union and enumeration elements. */
    if (ISTAG(s->n_sclass))
	    for (i=1;;i++) {
		SYMENT *tmp = &(tag+s->n_numaux+i)->u.syment;

		mark_tag_closure_as_referenced(abfd,tag+s->n_numaux+i);
		if (tmp->n_sclass == C_EOS)
			break;
		i += tmp->n_numaux;
	    }
}

/* We traverse the output symbol table setting up the unique tags hash table.
   Along the way we track which tags are referenced (an assumption coming in is that
   no tags are referenced).
*/
static void
DEFUN(coff_mark_all_tag_references,(abfd),bfd *abfd)
{
    unsigned int         idx;      /* Index of symbol in output file */
    combined_entry_type	*s;
    coff_symbol_type	*coff_symptr;

    /* Allocate storage for the hash table and stack used by tags_are_equal(): */
    cet_hash_table = (struct cet_list **)
	    bfd_zalloc(abfd,CET_HASH_TABLE_SIZE * sizeof(struct cet_list *));
    cet_stack  = (combined_entry_type **)
	    bfd_alloc(abfd,cet_stack_max * sizeof(combined_entry_type *));

    /* For all symbols in the output symbol table. */
    for (idx = 0; idx < bfd_get_symcount(abfd);idx++) {
	coff_symptr = coff_symbol_from(abfd,(abfd->outsymbols)[idx]);
	if (!coff_symptr || !fetch_native_coff_info(&coff_symptr->symbol))
		continue;

	s = fetch_native_coff_info(&coff_symptr->symbol);

	if (s->u.syment.n_numaux) {
	    combined_entry_type *first_aux = (s+1);

	    switch (s->u.syment.n_sclass) {
		/* Let's make sure that at least one of each tag type gets emitted
		   into the output symbol table: */
	case C_ENTAG:
	case C_STRTAG:
	case C_UNTAG:
		if (s == lookup_tag(abfd,s))
			/* Make sure the closure of the tag, is marked as referenced. */
			mark_tag_closure_as_referenced(abfd,s);
		break;

		/* We only consider the storage classes that represent storage, AND
		   typedefs to consider if a tag is to be considered referenced. */

	case C_AUTO:
	case C_EXT:
	case C_STAT:
	case C_REG:
	case C_ARG:
	case C_REGPARM:
	case C_AUTOARG:
	case C_SCALL:
	case C_LEAFEXT:
	case C_LEAFSTAT:
	case C_TPDEF:
		/* The fix_tag is treated as a boolean here to determine if we should
		   look at the tag referenced in the first auxent as a tag index. */
		if (first_aux->fix_tag) {
		    combined_entry_type *t;

		    t = lookup_tag(abfd,(combined_entry_type *) (first_aux->u.auxent.AUX_TAGNDX));
		    if (abfd->flags & STRIP_DUP_TAGS)
			    first_aux->u.auxent.AUX_TAGNDX = (long) t;
		    mark_tag_closure_as_referenced(abfd,
						   (combined_entry_type *)first_aux->u.auxent.AUX_TAGNDX);
		}
		/* Make sure the closure of tag, t is marked as referenced. */
	    }
	}
    }

    if (1) {	    
	struct cet_list *p,*q;

	/* Free up the storage previously allocated. */
	bfd_release(abfd,cet_stack);
	for (idx=0;idx <  CET_HASH_TABLE_SIZE;idx++) {
	    for (p=cet_hash_table[idx];p;) {
		q = p;
		p = p->next_cet;
		bfd_release(abfd,q);
	    }
	}
	bfd_release(abfd,cet_hash_table);
    }
}

/* Remove all of the tags in the symbol table that have not been referenced in the
   symbol table.  Also, while we are at it, we remake the end index chain for the
   tag and function lists. */
static void
DEFUN(coff_remove_orphaned_tags,(abfd),bfd *abfd)
{
    unsigned int         idx;      /* Index of symbol in output file. */
    combined_entry_type	*s,**previous_end_index_tags = (combined_entry_type **) 0;
    combined_entry_type	**previous_end_index_funcs = (combined_entry_type **) 0;
    coff_symbol_type	*coff_symptr;
    asymbol            **source,**destination;

    source = destination = abfd->outsymbols;
    /* For all of the symbols in the output symbol table. */
    for (idx = 0; idx < bfd_get_symcount(abfd);idx++) {

 doitagain:
	coff_symptr = coff_symbol_from(abfd,*source);
	if (!coff_symptr || !fetch_native_coff_info(&coff_symptr->symbol)) {
	    *destination++ = *source++;
	    continue;
	}

	s = fetch_native_coff_info(&coff_symptr->symbol);

	/* If the symbol is a tag, and it is not referenced, then remove it from the output symbol
	   table. */
	if (ISTAG(s->u.syment.n_sclass) && ((s->tag_flags & TAG_REFERENCED) == 0)) {
	    int i;

	    for (source++,bfd_get_symcount(abfd)--;;bfd_get_symcount(abfd)--,source++) {
		coff_symptr = (coff_symbol_type *) *source;
		
		s = fetch_native_coff_info(&coff_symptr->symbol);
		if (s->u.syment.n_sclass == C_EOS) {
		    bfd_get_symcount(abfd)--;
		    source++;
		    break;
		}
	    }
	    goto doitagain;
	}
	/* We fix up the end indices of the tag lists in the symbol table here. */
	else if (ISTAG(s->u.syment.n_sclass) && (s+1)->fix_end) {
	    if (previous_end_index_tags)
		    (*previous_end_index_tags) = s;
	    previous_end_index_tags = (combined_entry_type **) &((s+1)->u.auxent.AUX_ENDNDX);
	}
	else if (ISFCN(s->u.syment.n_type) && (s+1)->fix_end) {
	    if (previous_end_index_funcs)
		    (*previous_end_index_funcs) = s;
	    previous_end_index_funcs = (combined_entry_type **) &((s+1)->u.auxent.AUX_ENDNDX);
	}
	else if (s->u.syment.n_sclass == C_STAT) {
	    if (previous_end_index_tags)
		    (*previous_end_index_tags) = s;
	    if (previous_end_index_funcs)
		    (*previous_end_index_funcs) = s;
	    previous_end_index_tags = (combined_entry_type **) 0;
	    previous_end_index_funcs = (combined_entry_type **) 0;
	}
	*destination++ = *source++;
    }
    *destination = (asymbol *) 0;
}

static void coff_futz_relocs(abfd,asect)
    bfd *abfd;
    asection *asect;
{
    if (asect->orelocation && asect->reloc_count) {
	arelent **t = asect->orelocation;
	int i;

	/* First translate the following sequences:  Into the following sequences:
	   optcall,  none                            optcall, iprmed
	   optcallx, none                            optcallx, relllong
	   rellong,  none (where address is same).   rellong address+4 respectively. */
	for (i=0;i < (asect->reloc_count-1);i++) {
	    if ((t[i]->address == t[i+1]->address) &&
		(t[i]->howto->reloc_type == bfd_reloc_type_opt_call) &&
		(t[i+1]->howto->reloc_type == bfd_reloc_type_none))
		    t[i+1]->howto = &howto_reloc_iprmed;
	    else if ((t[i]->address+4 == t[i+1]->address) &&
		     (t[i]->howto->reloc_type == bfd_reloc_type_opt_callx) &&
		     (t[i+1]->howto->reloc_type == bfd_reloc_type_none ))
		    t[i+1]->howto = &howto_reloc_rellong;
	}
	/* Next, remove all remaining none relocations and repoint all of the howto's
	 to the native coff howto's.  Also, point the section pointer relative relocations
	 to real sections: */
	if (1) {
	    int src,dst;

	    for (src=0,dst=0;src < asect->reloc_count;) {
		while (src < asect->reloc_count &&
		       t[src]->howto->reloc_type == bfd_reloc_type_none)
			src++;
		if (src < asect->reloc_count) {
		    t[dst] = t[src++];
		    switch (t[dst]->howto->reloc_type) {
		case bfd_reloc_type_opt_call:
			t[dst]->howto = &howto_reloc_callj;
			break;
		case bfd_reloc_type_opt_callx:
			t[dst]->howto = &howto_reloc_calljx;
			break;
		case bfd_reloc_type_32bit_abs:
			t[dst]->howto = &howto_reloc_rellong;
			break;
		case bfd_reloc_type_12bit_abs:
			t[dst]->howto = &howto_reloc_relshort;
			break;
		case bfd_reloc_type_24bit_pcrel:
			t[dst]->howto = &howto_reloc_iprmed;
			break;
#ifdef R_RELLONG_SUB

		case bfd_reloc_type_32bit_abs_sub:
			t[dst]->howto = &howto_reloc_rellong_sub;
			break;

#endif
		default:
			fprintf(stderr,"File: %s inherited unsupported relocation type: %d, from section: %s\n",
				abfd->filename,t[dst]->howto->reloc_type,asect->name);
			exit(1);
		    }
		    
		    dst++;
		}
	    }
	    asect->reloc_count = dst;
	}
    }
}

static          boolean
DEFUN(coff_write_object_contents,(abfd),
    bfd *abfd)
{
	asection       *sec;
	boolean         hasrelocs = false;
	boolean         haslinno = false;
	file_ptr        reloc_base;
	file_ptr        lineno_base;
	file_ptr        sym_base;
	file_ptr        scn_base;
	unsigned long   reloc_size = 0;
	unsigned long   lnno_size = 0;
	asection       *text_sec = NULL;
	asection       *data_sec = NULL;
	asection       *bss_sec = NULL;

	FILHDR internal_f;
	AOUTHDR internal_a;

	struct icofdata *coff = coff_data(abfd);

	memset( &internal_f, 0, sizeof(internal_f) );
	memset( &internal_a, 0, sizeof(internal_a) );

	bfd_error = system_call_error;

	if(abfd->output_has_begun == false) {
		coff_compute_section_file_positions(abfd);
	}

	scn_base = abfd->sections ? (file_ptr) ((abfd->flags & EXEC_P) ? (FILHSZ + AOUTSZ) : FILHSZ)
		: (file_ptr) 0;

	if (bfd_seek(abfd, scn_base, SEEK_SET) != 0)
		return false;
	reloc_base = obj_relocbase(abfd);

	/* Make a pass through the symbol table to count line number
	 * entries and put them into the correct asections.
	 */

	if (!(abfd->flags & STRIP_LINES))
		coff_count_linenumbers(abfd);

	/* Work out the size of the reloc and linno areas */

	for (sec = abfd->sections; sec; sec = sec->next) {
	    if (!(abfd->flags & DO_NOT_ALTER_RELOCS))
		    coff_futz_relocs(abfd,sec);
	    reloc_size += sec->reloc_count * RELSZ;
	    lnno_size += sec->lineno_count * LINESZ;
	}

	lineno_base = reloc_base + reloc_size;
	sym_base = lineno_base + lnno_size;

	/* Indicate in each section->line_filepos its actual file
	 * address
	 */
	for (sec = abfd->sections; sec; sec = sec->next) {
		if (sec->lineno_count) {
			sec->line_filepos = lineno_base;
			sec->moving_line_filepos = lineno_base;
			lineno_base += sec->lineno_count * LINESZ;
		} else {
			sec->line_filepos = 0;
		}

		if (sec->reloc_count) {
			sec->rel_filepos = reloc_base;
			reloc_base += sec->reloc_count * sizeof(struct reloc);
		} else {
			sec->rel_filepos = 0;
		}
	}

	/* Write section headers to the file.  */

	bfd_seek(abfd,scn_base,SEEK_SET);

	for (sec = abfd->sections; sec; sec = sec->next) {
		SCNHDR section;

		strncpy(&(section.s_name[0]), sec->name, 8);
		section.s_vaddr = sec->vma;
		section.s_paddr = sec->pma;
		if (sec->flags & SEC_IS_DSECT) {
			/* DSECTS show no size in their section headers */
			section.s_size = 0;
		}
		else {
			section.s_size = sec->size;
		}
		/*
		 * If this section has no size or is unloadable then
		 * the scnptr will be 0 too
		 */
		if (((sec->flags & SEC_LOAD) == 0) && ((sec->flags & SEC_HAS_CONTENTS) == 0)) {
			section.s_scnptr = 0;
		} else {
			section.s_scnptr = sec->filepos;
		}
		section.s_relptr = sec->rel_filepos;
		section.s_lnnoptr = sec->line_filepos;
		section.s_nreloc = sec->reloc_count;
		section.s_nlnno = sec->lineno_count;
		if (sec->reloc_count != 0)
			hasrelocs = true;
		if (sec->lineno_count != 0)
			haslinno = true;

		/* Remember the characteristics we might have inherited from
		input sections */
		section.s_flags = sec->insec_flags;

		if (!strcmp(sec->name, _TEXT)) {
			text_sec = sec;
		} else if (!strcmp(sec->name, _DATA)) {
			data_sec = sec;
		} else if (!strcmp(sec->name, _BSS)) {
			bss_sec = sec;
		}

		if (sec->flags & SEC_CODE) {
			section.s_flags |= STYP_TEXT;
		}
		if (sec->flags & SEC_DATA) {
			section.s_flags |= STYP_DATA;
		}
		if (sec->flags & SEC_IS_NOLOAD) {
			section.s_flags |= STYP_NOLOAD;
			section.s_scnptr = NULL;
		}
		if (sec->flags & SEC_IS_DSECT) {
			section.s_flags |= STYP_DSECT;
		}
		if (sec->flags & SEC_IS_COPY) {
			section.s_flags |= STYP_COPY;
		}
		if (sec->flags & SEC_IS_BSS) {
			section.s_flags |= STYP_BSS;
		}
		if (sec->flags & SEC_IS_INFO) {
			section.s_flags |= STYP_INFO;
		}
		/* If a section is identified as STYP_TEXT or STYP_DATA, then
		it can no longer be STYP_BSS, since it will have space
		allocated to it. */
		if ((section.s_flags & STYP_TEXT) ||
			(section.s_flags & STYP_DATA)) {
			section.s_flags &= ~STYP_BSS;
		}
		section.s_align = sec->alignment_power ?
					1 << sec->alignment_power : 0;
		write_scnhdr(abfd, &section);
	}

	/* OK, now set up the filehdr... */
	internal_f.f_nscns = abfd->section_count;

	if (abfd->flags & SUPP_W_TIME)
		internal_f.f_timdat = 0;
	else if (!abfd->mtime_set)
		internal_f.f_timdat = time(0);		
	else
		internal_f.f_timdat = abfd->mtime;

	if (bfd_get_symcount(abfd) != 0){
		internal_f.f_symptr = sym_base;
	}
	if (abfd->flags & EXEC_P){
		internal_f.f_opthdr = AOUTSZ;
	}

	internal_f.f_magic = I960ROMAGIC;

	coff_set_flags(abfd, &internal_f.f_flags);
	if (!hasrelocs)			internal_f.f_flags |= F_RELFLG;
	if (!haslinno)			internal_f.f_flags |= F_LNNO;
	if (bfd_get_symcount(abfd)==0)	internal_f.f_flags |= F_LSYMS;
	if (abfd->flags & EXEC_P)	internal_f.f_flags |= F_EXEC;
	if ((abfd->flags & HAS_CCINFO) &&
	    (abfd->flags & HAS_RELOC))	internal_f.f_flags |= F_CCINFO;
#if 0
#if !defined(NO_BIG_ENDIAN_MODS)
	if (abfd->xvec->byteorder_big_p)
		internal_f.f_flags |= F_BIG_ENDIAN_TARGET;      /* big-endian */
	else
		internal_f.f_flags |= F_AR32WR; /* little-endian */
#else
	if (!abfd->xvec->byteorder_big_p)internal_f.f_flags |= F_AR32WR;
#endif /* NO_BIG_ENDIAN_MODS */
#endif
	if (abfd->xvec->byteorder_big_p)
		internal_f.f_flags |= F_BIG_ENDIAN_TARGET;      /* big-endian */
	if (!BFD_BIG_ENDIAN_FILE_P(abfd))internal_f.f_flags |= F_AR32WR;
	if ((abfd->flags & SYMTAB_COMPRESSED) ||
	    MERGE_DUPLICATED_TAGS(abfd)) internal_f.f_flags |= F_COMP_SYMTAB;
	if (abfd->flags & HAS_PIC)       internal_f.f_flags |= F_PIC;
	if (abfd->flags & HAS_PID)       internal_f.f_flags |= F_PID;
        if (abfd->flags & CAN_LINKPID)   internal_f.f_flags |= F_LINKPID;

	/* Now should write relocs, strings, syms */
	obj_sym_filepos(abfd) = sym_base;

	if (bfd_get_symcount(abfd) != 0) {
#if 0
	    The following code was moved down into the process
	    It used to be in set_symtab, but this made stripping unfunctional.
#else
	    if (!(abfd->flags & SYMTAB_COMPRESSED) &&
		!(abfd->flags & DO_NOT_STRIP_ORPHANED_TAGS)) {
		/* Strategy for compressing coff symbol table:
		   1st mark all of the tags that are referenced as referenced.
		   also make sure that the set that are referenced are as small
		   as possible by comparing likenamed tags.
		   2nd remove the resulting orphaned tags. */
		coff_mark_all_tag_references(abfd);
		coff_remove_orphaned_tags(abfd);
	    }
	    coff_renumber_symbols(abfd);
	    coff_mangle_symbols(abfd);
	    if (COMPRESS_STRINGS(abfd))
		    coff_compress_long_string_table(abfd);
#endif
	    coff_write_symbols(abfd, internal_f.f_flags);
	    bfd960_write_ccinfo( abfd );
	    coff_write_linenumbers(abfd);
	    coff_write_relocs(abfd);
	}

	internal_a.magic = NMAGIC;
	if (text_sec) {
		internal_a.tsize = text_sec->size;
		internal_a.text_start= text_sec->size ? text_sec->vma : 0;
	}
	if (data_sec) {
		internal_a.dsize = data_sec->size;
		internal_a.data_start = data_sec->size ? data_sec->vma : 0;
	}
	if (bss_sec) {
		internal_a.bsize =  bss_sec->size;
	}

	internal_a.entry = bfd_get_start_address(abfd);
	internal_f.f_nsyms =  bfd_get_symcount(abfd);

	/* now write them */
	if (bfd_seek(abfd, 0L, SEEK_SET) != 0)
		return false;

	bfd_write_filehdr(abfd, &internal_f);

	if (abfd->flags & EXEC_P) {
		bfd_write_aouthdr(abfd, &internal_a);
	}
	return true;
}

/*
 * Transform indices into the symbol table into pointers to the
 * table entries.  In the case of an end index, point the final
 * member of the function/tag/block rather than the first following
 * symbol:  the latter may get moved around before the symbol table
 * is actually output, and the end index should end up pointing to
 * whatever actually follows at output time.
 */
static void
DEFUN(coff_pointerize_aux,(abfd, symtab, sym, auxent),
    bfd			*abfd AND
    combined_entry_type	*symtab AND
    SYMENT		*sym AND
    combined_entry_type	*auxent )
{
	AUXENT *aux = &auxent->u.auxent;
	int class = sym->n_sclass;
	int type  = sym->n_type;

	if ( (class == C_FILE)				/* File symbol */
	||   (class == C_STAT && type == T_NULL) ){	/* Section symbol */
		/* These guys contain no symbol indices */
		return;
	}

	/* Otherwise patch up */

	if (ISFCN(type)
	||  ISTAG(class)
	||  (class == C_BLOCK && sym->n_name[1] == 'b')) {
		/* Test for 'b' makes this apply to ".bb" symbols only,
		 * excluding ".eb" symbols.
		 */
	    if (aux->AUX_ENDNDX) {
		aux->AUX_ENDNDX = (long)(symtab+(aux->AUX_ENDNDX));
		auxent->fix_end = 1;
	    }
	}
	if (aux->AUX_TAGNDX != 0) {
		aux->AUX_TAGNDX = (long)(symtab+(aux->AUX_TAGNDX));
		auxent->fix_tag = 1;
	}
}


static boolean
DEFUN(coff_set_section_contents,(abfd, section, location, offset, count),
    bfd      *abfd AND
    sec_ptr   section AND
    PTR       location AND
    file_ptr  offset AND
    size_t    count)
{
	if (abfd->output_has_begun == false)	/* set by bfd.c handler */
		coff_compute_section_file_positions(abfd);

	bfd_seek(abfd, (file_ptr) (section->filepos + offset), SEEK_SET);

	if (count != 0) {
		return bfd_write(location,1,count,abfd) == count;
	}
	return true;
}

static PTR 
buy_and_read(abfd, where, seek_direction, size)
    bfd      *abfd;
    file_ptr  where;
    int       seek_direction;
    size_t    size;
{
	PTR area = (PTR) bfd_alloc(abfd, size);

	if (!area) {
		bfd_error = no_memory;
		return (NULL);
	}
	bfd_seek(abfd, where, seek_direction);
	if (bfd_read(area, 1, size, abfd) != size) {
		bfd_error = system_call_error;
		return (NULL);
	}
	return (area);
}


/*
 * On entry we should be "seek"'d to the end of the
 * symbols, i.e., to string table size.
 */
static char *
DEFUN(build_string_table,(abfd),
    bfd *abfd)
{
	char buffer[4],*msg;
	unsigned int strtab_len;
	char *string_table;

	if (bfd_read(buffer,sizeof(buffer),1,abfd) != sizeof(strtab_len)) {
	    msg = "read size of string table";
	    goto fatal_error;
	}

	strtab_len = bfd_h_get_32(abfd, buffer);
	if ((string_table = (PTR) bfd_alloc(abfd, strtab_len -= 4)) == NULL) {
	    msg = "allocate bytes for string table";
	    goto fatal_error;
	}
	if (bfd_read(string_table, strtab_len, 1, abfd) != strtab_len) {
	    msg = "read string table";
	    goto fatal_error;
	}
	return string_table;
fatal_error:
	fprintf(stderr,"FATAL ERROR: can not %s from file: %s\n",msg,abfd->filename);
	exit(1);
}

/*
 * Read a symbol table into freshly mallocated memory, swap it, and
 * knit the symbol names into a "normalized" form: all symbols have an
 * n_offset pointer that points to a '\0'-terminated string.
 */
static combined_entry_type *
DEFUN(get_normalized_symtab,(abfd,num_syments),
    bfd *abfd AND
    unsigned int *num_syments)
{
	combined_entry_type *internal;
	combined_entry_type *internal_end;
	combined_entry_type *ip;
	SYMENT *raw;
	SYMENT *raw_src;
	SYMENT *raw_end;
	char   *string_table = NULL;
	unsigned long size;
	unsigned int raw_size;

	*num_syments = 0;
	if (obj_raw_syments(abfd)) {
	    *num_syments = bfd_get_symcount(abfd);
	    return obj_raw_syments(abfd);
	}

	size = bfd_get_symcount(abfd) * sizeof(combined_entry_type);
	if (size == 0) {
		bfd_error = no_symbols;
		return (NULL);
	}

	internal = (combined_entry_type *)bfd_alloc(abfd, size);
	internal_end = internal + bfd_get_symcount(abfd);

	raw_size =  bfd_get_symcount(abfd) * SYMESZ;
	raw = (SYMENT *)bfd_alloc(abfd,raw_size);

	if (bfd_seek(abfd, obj_sym_filepos(abfd), SEEK_SET) == -1
	||  bfd_read((PTR)raw, raw_size, 1, abfd) != raw_size) {
		bfd_error = system_call_error;
		return (NULL);
	}


	/* Swap all the raw entries */
	raw_end = raw + bfd_get_symcount(abfd);
	for (raw_src = raw, ip = internal; raw_src < raw_end; raw_src++, ip++) {

		unsigned int i;
		SYMENT *sym;

		(*num_syments) += 1;
		ip->fix_tag = ip->fix_end = ip->offset = ip->tag_flags = 0;
		ip->lookup_equivalent_tag = ip->tags_are_equal_equivalent_tag = 0;

		sym = &ip->u.syment;
		coff_swap_sym_in(abfd, raw_src, sym);

		for (i = 1; i <= sym->n_numaux; i++) {
			ip++;
			raw_src++;

			ip->fix_tag = ip->fix_end = ip->offset = ip->tag_flags = 0;
			ip->lookup_equivalent_tag = ip->tags_are_equal_equivalent_tag = 0;

			coff_swap_aux_in(abfd, (AUXENT *)raw_src, sym->n_type,
						sym->n_sclass, &ip->u.auxent,i-1);

			if (i == 1){
				coff_pointerize_aux(abfd, internal, sym, ip);

			}
			else if ((i == 2)
				 && ((sym->n_sclass == C_LEAFEXT) || (sym->n_sclass == C_LEAFSTAT)) ){
			    /* Convert bal address of leafproc to an
			     * offset from its call address
			     */
			    ip->u.auxent.x_bal.x_balntry -= sym->n_value;
			}
		}
	}

	/* Free all the raw stuff */
	bfd_release(abfd, raw);

	for (ip=internal; ip<internal_end; ip++){
		SYMENT *sym = &ip->u.syment;
		AUXENT *aux = &(ip+1)->u.auxent;

		if (sym->n_sclass == C_FILE) {
			/* make a file symbol point to the name in the auxent,
			 * since the text ".file" is redundant
			 */
			if (aux->x_file.x_n.x_zeroes == 0) {
				/* the filename is a long one, point into the
				 * string table
				 */
				if (string_table == NULL) {
					string_table = build_string_table(abfd);
				}
				sym->n_offset = (int)(string_table - 4
						+ aux->x_file.x_n.x_offset);
			} else {
				/* ordinary short filename, put into memory
				 * anyway
				 */
				sym->n_offset = (int)
				    copy_name(abfd, aux->AUX_FNAME, FILNMLEN);
			}
		} else {
			if (sym->n_zeroes) {
				/* This is a "short" name.  Make it long */
				int i;
				char *newstring;

				/* Find the length of this string without
				 * walking into memory that isn't ours.
				 */
				for (i = 0; i < 8; ++i) {
					if (sym->n_name[i] == '\0') {
						break;
					}
				}
				newstring = (PTR) bfd_alloc(abfd, ++i);
				if (newstring == NULL) {
					bfd_error = no_memory;
					return (NULL);
				}
				memset(newstring, 0, i);
				strncpy(newstring, sym->n_name, i-1);
				sym->n_offset = (int) newstring;
				sym->n_zeroes = 0;
			} else {
				/* This is a long name already.
				 * Just point it at the string in memory.
				 */
				if (string_table == NULL) {
					string_table = build_string_table(abfd);
				}
				sym->n_offset += (int)(string_table - 4);
			}
		}
		ip += sym->n_numaux;
	}
	obj_raw_syments(abfd) = internal;
	obj_string_table(abfd) = string_table;
	return (internal);
}


static
struct sec *
DEFUN(section_from_bfd_index,(abfd, i),
    bfd *abfd AND
    int i)
{
	if (i > 0) {
		struct sec *answer = abfd->sections;
		while (--i) {
			answer = answer->next;
		}
		return answer;
	}
	return 0;
}


static boolean
coff_slurp_line_table(abfd, asect)
    bfd      *abfd;
    asection *asect;
{
	LINENO		*native_lineno;
	LINENO		*src;
	unsigned int	 i;
	alent		*cache_ptr;

	BFD_ASSERT(asect->lineno == (alent *) NULL);

	asect->lineno = cache_ptr = (alent *)
		bfd_alloc(abfd, (size_t)((asect->lineno_count+1) * sizeof(alent)));

	if (asect->lineno_count > 0) {
	    native_lineno = (LINENO *)
		    buy_and_read(abfd, asect->line_filepos, SEEK_SET,
				 (size_t)(LINESZ * asect->lineno_count));
	    src = native_lineno;
	    for (i = 0; i < asect->lineno_count; i++, cache_ptr++) {
		LINENO dst;
		coff_swap_lineno_in(abfd, src, &dst);
		cache_ptr->line_number = dst.l_lnno;

		if (cache_ptr->line_number == 0) {
		    coff_symbol_type *sym =
			    (coff_symbol_type *) (dst.l_addr.l_symndx
						  + obj_raw_syments(abfd))->u.syment.n_zeroes;
		    cache_ptr->u.sym = (asymbol *) sym;
		    if (sym == NULL) {
			fprintf(stderr, 
				"FATAL ERROR: symbol index: %d of file: %s has bad line number information\n",
				dst.l_addr.l_symndx, abfd->filename);
			exit(1);
		    }
		    sym->lineno = cache_ptr;
		} else {
		    cache_ptr->u.offset = dst.l_addr.l_paddr
			    - bfd_section_vma(abfd, asect);
		}
		src++;
	    }
	    bfd_release(abfd,native_lineno);
	}
	cache_ptr->line_number = 0;
	return true;
}

static void add_bal_entry(abfd,as,aux)
    bfd *abfd;
    asymbol *as;
    AUXENT *aux;
{
    as->flags |= BSF_HAS_BAL;
    as->related_symbol = (asymbol *) bfd_zalloc(abfd,sizeof(asymbol));
    as->related_symbol->flags |= BSF_HAS_CALL;
    as->related_symbol->related_symbol = as;
    as->related_symbol->section = as->section;
    as->related_symbol->value = as->value + aux->x_bal.x_balntry;
}

boolean
DEFUN(coff_slurp_symbol_table,(abfd),
    bfd *abfd)
{
	combined_entry_type	*src;
	coff_symbol_type	*dst;
	unsigned int		*table_ptr;
	unsigned int		 symnum;
	unsigned int		 num_raw_syms;
	unsigned int		 i,num_syments;
	asection		*p;

	if (obj_symbols(abfd)){
		return true;
	}

	/* Read in the symbol table */
	bfd_seek(abfd, obj_sym_filepos(abfd), SEEK_SET);
	if ((src = get_normalized_symtab(abfd,&num_syments)) == NULL) {
		return false;
	}

	/* Allocate enough room for all the symbols in cached form */
	coff_data(abfd)->raw_syment_count
		= num_raw_syms = bfd_get_symcount(abfd);
	dst = (coff_symbol_type *)
	    bfd_alloc(abfd, (size_t)(num_syments * sizeof(coff_symbol_type)));
	table_ptr = (unsigned int *)
	    bfd_alloc(abfd, (size_t)(num_raw_syms * sizeof(unsigned int)));
	if (dst == NULL || table_ptr == NULL) {
		bfd_error = no_memory;
		return false;
	}

	obj_raw_syments(abfd) = src;
	obj_symbols(abfd) = dst;
	obj_convert(abfd) = table_ptr;

	for (i = 0, symnum=0; i < num_raw_syms; i++, src++) {
		table_ptr[i] = symnum;
		dst->symbol.the_bfd = abfd;
		dst->symbol.name = (char *)(src->u.syment.n_offset);

		/* Use the native name field to point to the cached field */
		src->u.syment.n_zeroes = (int) dst;
		dst->symbol.section =
			section_from_bfd_index(abfd, src->u.syment.n_scnum);

		/* Start out assuming symbol value will carry through */
		dst->symbol.value = src->u.syment.n_value;
		dst->symbol.value_2 = 0;

		dst->symbol.flags = (src->u.syment.n_flags & F_SECT_SYM) ? BSF_SECT_SYM : 0;

		if (src->u.syment.n_sclass != C_FILE &&
		    src->u.syment.n_sclass != C_STAT &&
		    src->u.syment.n_sclass != C_EXT &&
		    src->u.syment.n_sclass != C_LEAFEXT &&
		    src->u.syment.n_sclass != C_LEAFSTAT &&
		    src->u.syment.n_sclass != C_SCALL)
			dst->symbol.flags |= BSF_XABLE;
		switch (src->u.syment.n_sclass) {
		case C_SCALL:
		        dst->symbol.flags |= BSF_ALL_OMF_REP | BSF_EXPORT | BSF_GLOBAL |
				BSF_HAS_SCALL | BSF_NOT_AT_END;
			if ( dst->symbol.section )
				dst->symbol.value -= dst->symbol.section->vma;
			dst->symbol.related_symbol = (asymbol *) bfd_zalloc(abfd,sizeof(asymbol));
			dst->symbol.related_symbol->flags |= (BSF_HAS_CALL|BSF_SCALL);
			dst->symbol.related_symbol->related_symbol = &dst->symbol;
			if (1) {
			    AUXENT *aux = &(src+2)->u.auxent;
			    dst->symbol.related_symbol->value = aux->x_sc.x_stindx;
			}
			break;
		case C_LEAFEXT:
		case C_EXT:
			dst->symbol.flags |= BSF_ALL_OMF_REP;
			if (src->u.syment.n_scnum == 0) {
			    if (!dst->symbol.value)
				    dst->symbol.flags |= BSF_UNDEFINED;
			    else {
				dst->symbol.flags |= (BSF_FORT_COMM |
						      ((src->fix_tag || src->u.syment.n_type) *
						       BSF_HAS_DBG_INFO));
			    }

			} else if (dst->symbol.section == (asection *) NULL) {
				dst->symbol.flags |=
					BSF_EXPORT | BSF_GLOBAL | BSF_ABSOLUTE;

			} else {
				/* Convert value -> offset into the section */
				dst->symbol.value -= dst->symbol.section->vma;
				dst->symbol.flags |= BSF_EXPORT | BSF_GLOBAL;
				if (src->u.syment.n_sclass == C_LEAFEXT)
					add_bal_entry(abfd,&dst->symbol,&(src+2)->u.auxent);
			}

			/* External functions don't go at end of symbol table
			 * with the other externals
			 */
			if (ISFCN((src->u.syment.n_type))) {
				dst->symbol.flags |= BSF_NOT_AT_END;
			}
			break;

		case C_BLOCK:		/* ".bb" or ".eb"		*/
		case C_FCN:		/* ".bf" or ".ef"		*/
			dst->symbol.flags |= BSF_DEBUGGING;
		case C_LABEL:		/* label			*/
		case C_LEAFSTAT:	/* static leaf procedure        */
		case C_STAT:		/* static			*/
			dst->symbol.flags |= BSF_LOCAL;
			if (src->u.syment.n_sclass != C_BLOCK &&
			    src->u.syment.n_sclass != C_FCN)
				dst->symbol.flags |= BSF_ALL_OMF_REP;
			/* Convert value to an offset into the section */
			if ( dst->symbol.section ){
				dst->symbol.value -= dst->symbol.section->vma;
				if (src->u.syment.n_sclass == C_LEAFSTAT)
					add_bal_entry(abfd,&dst->symbol,&(src+2)->u.auxent);
			} else {
				dst->symbol.flags |= BSF_ABSOLUTE;
			}
			break;

		case C_ARG:
		case C_AUTO:		/* automatic variable */
		case C_AUTOARG:		/* 960-specific storage class */
		case C_ENTAG:		/* enumeration tag		*/
		case C_EOS:		/* end of structure		*/
		case C_FIELD:		/* bit field */
		case C_FILE:		/* file name			*/
		case C_MOE:		/* member of enumeration	*/
		case C_MOS:		/* member of structure	*/
		case C_MOU:		/* member of union		*/
		case C_REG:		/* register variable		*/
		case C_REGPARM:		/* register parameter		*/
		case C_STRTAG:		/* structure tag		*/
		case C_TPDEF:		/* type definition		*/
		case C_UNTAG:		/* union tag			*/
			dst->symbol.flags |= BSF_DEBUGGING;
			break;

		case C_ALIAS:		/* duplicate tag		*/
		case C_EFCN:		/* physical end of function	*/
		case C_EXTDEF:		/* external definition		*/
		case C_HIDDEN:		/* ext symbol in dmert public lib */
		case C_LINE:		/* line # reformatted as symtab entry */
		case C_NULL:
		case C_ULABEL:		/* undefined label		*/
		case C_USTATIC:		/* undefined static		*/
		default:
                        fprintf(stderr,"FATAL ERROR: symbol index: %d of file: %s ",i,abfd->filename);
                        fprintf(stderr,"has unsupported storage class: %d\n",src->u.syment.n_sclass);
                        exit(1);
		}

		dst->symbol.native_info = (char *)src;
		dst->symbol.udata = 0;
		dst->lineno = (alent *) NULL;
		i += src->u.syment.n_numaux;
		src += src->u.syment.n_numaux;
		dst++;
		symnum++;
	}				/* walk the native symtab */

	bfd_get_symcount(abfd) = symnum;

	/* Slurp the line tables for each section too */
	for (p = abfd->sections; p; p = p->next) {
		coff_slurp_line_table(abfd, p);
	}

	return true;
}				/* coff_slurp_symbol_table() */


static unsigned int
coff_get_symtab_upper_bound(abfd)
    bfd *abfd;
{
	if (!coff_slurp_symbol_table(abfd))
		return 0;

	return (bfd_get_symcount(abfd) + 1) * (sizeof(coff_symbol_type *));
}


static unsigned int
coff_get_symtab(abfd, alocation)
    bfd      *abfd;
    asymbol **alocation;
{
	unsigned int  i;
	coff_symbol_type *symbase;
	coff_symbol_type **location = (coff_symbol_type **) (alocation);

	if (!coff_slurp_symbol_table(abfd))
		return 0;

	for (i=0, symbase=obj_symbols(abfd); i < bfd_get_symcount(abfd); i++){
		*(location++) = symbase++;
	}
	*location++ = 0;
	return bfd_get_symcount(abfd);
}


static unsigned int
coff_get_reloc_upper_bound(abfd, asect)
    bfd     *abfd;
    sec_ptr  asect;
{
	if (bfd_get_format(abfd) != bfd_object) {
		bfd_error = invalid_operation;
		return 0;
	}
	return (asect->reloc_count + 1) * sizeof(arelent *);
}

static int cmp_two_relocs(left,right)
    arelent *left,*right;
{
    if (left->address < right->address)
	    return -1;
    else if (left->address > right->address)
	    return 1;
    else
	    return left->howto->reloc_type - right->howto->reloc_type;
}

static boolean
DEFUN(coff_slurp_reloc_table,(abfd, asect, symbols),
    bfd      *abfd AND
    sec_ptr   asect AND
    asymbol **symbols)
{
	RELOC   *native_relocs;
	RELOC   *src;
	arelent *reloc_cache;
	int     count;

	if (asect->relocation || asect->reloc_count == 0){
		return true;
	}
	if (!coff_slurp_symbol_table(abfd)){
		return false;
	}

	native_relocs = (RELOC*)buy_and_read(abfd, asect->rel_filepos, SEEK_SET,
					(size_t) (RELSZ * asect->reloc_count));
	reloc_cache = (arelent *)
	    bfd_alloc(abfd, (size_t) (asect->reloc_count * sizeof(arelent)));

	if (reloc_cache == NULL) {
		bfd_error = no_memory;
		return false;
	}

	asect->relocation = reloc_cache;
	src = native_relocs;
	for ( count=asect->reloc_count; count--; reloc_cache++, src++) {
		struct reloc dst;
		asymbol *ref;

		bfd_swap_reloc_in(abfd, src, &dst);

		reloc_cache->sym_ptr_ptr = symbols
					     + obj_convert(abfd)[dst.r_symndx];
		ref = *(reloc_cache->sym_ptr_ptr);
		ref->flags |= BSF_SYM_REFD_OTH_SECT;
		reloc_cache->address = dst.r_vaddr - asect->vma;

		switch (dst.r_type) {
	        case R_OPTCALLX:
		        reloc_cache->howto = &howto_reloc_calljx;
			break;
		case R_OPTCALL:
			reloc_cache->howto = &howto_reloc_callj;
			break;
		case R_RELLONG:
			reloc_cache->howto = &howto_reloc_rellong;
			break;
#ifdef R_RELLONG_SUB
		case R_RELLONG_SUB:
			reloc_cache->howto = &howto_reloc_rellong_sub;
			break;
#endif
		case R_RELSHORT:
			reloc_cache->howto = &howto_reloc_relshort;
			break;
		case R_IPRMED:
			reloc_cache->howto = &howto_reloc_iprmed;
			break;
		default:
                        fprintf(stderr,"FATAL ERROR: address: 0x%x file: %s\n",dst.r_vaddr,abfd->filename);
                        fprintf(stderr,"has unsupported relocation directive: 0x%x\n",dst.r_type);
                        fprintf(stderr,"symbol index: %d\n",dst.r_symndx);
			exit(1);
			break;
		}
	}

	if (!(abfd->flags & DO_NOT_ALTER_RELOCS)) {
	    /* Merge relocations into those that bfd_perform_relocation() understands: */
	    qsort(asect->relocation,asect->reloc_count,sizeof(asect->relocation[0]),cmp_two_relocs);
	    if (1) {
		arelent *t = asect->relocation;
		int i;

		for (i=0;i < asect->reloc_count-1;i++)
			if ((t[i].address == t[i+1].address) &&
			    (t[i].howto->reloc_type == bfd_reloc_type_opt_call) &&
			    (t[i+1].howto->reloc_type == bfd_reloc_type_24bit_pcrel))
				t[i+1].howto = &howto_reloc_none;
			else if ((t[i].address+4 == t[i+1].address) &&
				 (t[i].howto->reloc_type == bfd_reloc_type_opt_callx) &&
				 (t[i+1].howto->reloc_type == bfd_reloc_type_32bit_abs))
				t[i+1].howto = &howto_reloc_none;
	    }
	}
	bfd_release(abfd,native_relocs);
	return true;
}

static unsigned int
coff_canonicalize_reloc(abfd, section, relptr, symbols)
    bfd      *abfd;
    sec_ptr   section;
    arelent **relptr;
    asymbol **symbols;
{
	unsigned int    i;
	arelent        *p;

	if ( !coff_slurp_reloc_table(abfd, section, symbols)
	||   (section->relocation == 0) ){
		return 0;
	}

	for ( p = section->relocation, i = 0; i < section->reloc_count; i++ ){
		*relptr++ = p++;
	}
	*relptr = 0;
	return section->reloc_count;
}


/*
 * Given a bfd, a section and an offset into the section, calculate and
 * return the name of the source file and the line nearest to the
 * specified location.
 */
static boolean
DEFUN(coff_find_nearest_line,(abfd,
			      section,
			      symbols,
			      offset,
			      filename_ptr,
			      functionname_ptr,
			      line_ptr),
    bfd           *abfd AND
    asection      *section AND
    asymbol      **symbols AND
    bfd_vma        offset AND
    CONST char   **filename_ptr AND
    CONST char   **functionname_ptr AND
    unsigned int  *line_ptr)
{
    static bfd_ghist_info *coff_fetch_ghist_info();
    unsigned int nlines,nelements,low,high,mid;
    bfd_ghist_info *info;

    *functionname_ptr = *filename_ptr = NULL;
    *line_ptr = 0;

    if (!(info = coff_fetch_ghist_info(abfd,&nelements,&nlines)))
	    return true;

    if (section)
	    offset += section->vma;
    else {
	bfd_release(abfd,info);
	return true;
    }

    if (offset >= info[high=nelements-1].address || offset < info[0].address) {
	bfd_release(abfd,info);
	return true;
    }
    for(low=0,mid=(high+low)/2;low < high-1;mid = (low+high)/2) {
	if (offset > info[mid].address)
		low = mid;
	else if (offset < info[mid].address)
		high = mid;
	else
		break;
    }
    *filename_ptr = info[mid].file_name;
    *functionname_ptr = info[mid].func_name;
    *line_ptr = info[mid].line_number;
    bfd_release(abfd,info);
    return true;
}


/* FOR GDB */
file_ptr
coff_sym_filepos(abfd)
bfd *abfd;
{
	return obj_sym_filepos(abfd);
}


static int 
DEFUN(coff_sizeof_headers,(abfd, reloc),
      bfd *abfd AND
      boolean reloc)
{
	int size;

	size = FILHSZ + (abfd->section_count * SCNHSZ);
	if  (reloc) {
		size += AOUTSZ;
	}
	return size;
}


/* currently called directly by stripper */
int
DEFUN(coff_zero_line_info, (abfd),
	bfd *abfd )
{
        int i,j;
        asection *sec;

	/* First see which is the quickest path in zeroing line info.
         * If the outsymbols is not empty then zero them, otherwise
         * proceed with the less efficient manner by slurping the
         * the raw symbols and zeroing their line info.
         */
        if (abfd->outsymbols == NULL) { /* zero raw symbols */
                SYMENT *S;
                AUXENT *A;
                combined_entry_type *symp = obj_raw_syments(abfd);

                if (!coff_slurp_symbol_table(abfd))
                        return 1; /* error */

                for (i=0; i < coff_data(abfd)->raw_syment_count; i++, symp++) {
                        S = &symp->u.syment;
                        if (!ISFCN(S->n_type)  && !ISARY(S->n_type)) {
                                j = i + S->n_numaux;
                                while (i < j) {
                                        i++;
                                        symp++;
                                        A = &symp->u.auxent;
                                        A->AUX_LNNO = 0;  /* effectively, strip line info */
                                        A->AUX_LNNOPTR = NULL;
                                }
                        }
                }
        }
        else { /* zero the outsymbols */
                asymbol **p;
                unsigned int limit = bfd_get_symcount(abfd);

                for (p = abfd->outsymbols, i=0; i<limit ; i++, p++) {
                        coff_symbol_type *q = coffsymbol(*p);
                        if (q->lineno) {
                                alent *l = q->lineno;
                                q->symbol.section->output_section->lineno_count = 0;
                                while (l->line_number) {
                                        l->line_number = 0;
                                        l++;
                                }
                                q->lineno = NULL;
                        }
                }
        }
        return 0;
}

static
bfd_ghist_info *coff_fetch_ghist_info(abfd,nelements,nlinenumbers)
    bfd *abfd;
    unsigned int *nelements,*nlinenumbers;
{
    unsigned int p_max = bfd_get_symcount(abfd),endofcode = 0;
    bfd_ghist_info *p = (bfd_ghist_info *) 0;
    char *file_name = NULL;
    coff_symbol_type *sym_ptr;
    int firstexternal,i,native_index = 0;

    *nelements = *nlinenumbers = 0;
    if (!coff_slurp_symbol_table(abfd))
	    return p;
    p = (bfd_ghist_info *) bfd_alloc(abfd,p_max*sizeof(bfd_ghist_info));

    for (sym_ptr=obj_symbols(abfd),i=0 ; native_index < coff_data(abfd)->raw_syment_count ; 
	 native_index += 1 + fetch_native_coff_info(&sym_ptr->symbol)->u.syment.n_numaux,sym_ptr++) {
	if (!fetch_native_coff_info(&sym_ptr->symbol)) {
	    bfd_release(abfd,p);
	    return (bfd_ghist_info *) 0;
	}
	switch (fetch_native_coff_info(&sym_ptr->symbol)->u.syment.n_sclass) {
    case C_FILE:
	    file_name = (char *) fetch_native_coff_info(&sym_ptr->symbol)->u.syment.n_offset;
	    firstexternal = (fetch_native_coff_info(&sym_ptr->symbol)->u.syment.n_value == 0) ?
		    0x7fffffff : fetch_native_coff_info(&sym_ptr->symbol)->u.syment.n_value;
	    break;
    case C_STAT:
    case C_LABEL:
    case C_EXT:
    case C_SCALL:
    case C_LEAFEXT:
    case C_LEAFSTAT:
	    if (!sym_ptr->symbol.section ||
		((sym_ptr->symbol.section->flags & SEC_CODE) == 0) ||
		strchr(sym_ptr->symbol.name,'.') ||
	        strcmp(sym_ptr->symbol.name, "___gnu_compiled_c") == 0)
		    continue;
	    if (native_index >= firstexternal)
		    file_name = _BFD_GHIST_INFO_FILE_UNKNOWN;
	{
	    unsigned int secstart = sym_ptr->symbol.section->vma;
	    unsigned int secend = sym_ptr->symbol.section->vma + sym_ptr->symbol.section->size;
	    CONST char *fname;
	    endofcode = secend > endofcode ? secend : endofcode;
	    _bfd_add_bfd_ghist_info(&p,nelements,&p_max,
				    secstart+sym_ptr->symbol.value,
				    fname=_bfd_trim_under_and_slashes(sym_ptr->symbol.name,1),
				    file_name,0);
	    if (sym_ptr->lineno) {
		combined_entry_type  *s = fetch_native_coff_info(&sym_ptr->symbol);
		struct lineno_cache_entry *l = (sym_ptr->lineno+1);
		unsigned int line_base = 0;

		s = s + 1 + s->u.syment.n_numaux;
		/* S should now point to the function's .bf */
		if (s->u.syment.n_numaux) {
		    /* linenumber is stored in the auxent */
		    AUXENT   *a = &((s + 1)->u.auxent);
		    line_base = a->AUX_LNNO;
		}
		while (l && l->line_number) {
		    (*nlinenumbers)++;
		    _bfd_add_bfd_ghist_info(&p,nelements,&p_max,
					    secstart+l->u.offset,
					    fname,
					    file_name,line_base+l->line_number-1);
		    l++;
		}
	    }
	}
	}
    }
/*
 The following is a a lie, because in coff, you can have multiple
 text sections.  But, better to be close than wrong.
*/

    _bfd_add_bfd_ghist_info(&p,nelements,&p_max,
			    endofcode,
			    "_Etext",_BFD_GHIST_INFO_FILE_UNKNOWN,0);

    qsort(p,*nelements,sizeof(bfd_ghist_info),_bfd_cmp_bfd_ghist_info);

    /* Find all of the adjacent entries that are the same in every respect except for
       one of the line_number entries are zero.  Remove the one that is zero. */

    for (i=0;i < (*nelements)-1;i++) {
	if (p[i].address == p[i+1].address && (p[i].line_number == 0 || p[i+1].line_number == 0) &&
	    !strcmp(p[i].func_name,p[i+1].func_name) &&
	    !strcmp(p[i].file_name,p[i+1].file_name)) {
	    int j;

	    p[i+1].line_number = p[i].line_number = p[i].line_number + p[i+1].line_number;
	    for (j=i;j < (*nelements)-1;j++) {
		p[j].address = p[j+1].address;
		p[j].func_name = p[j+1].func_name;
		p[j].file_name = p[j+1].file_name;
		p[j].line_number = p[j+1].line_number;
	    }
	    (*nelements)--;
	}
    }

    if (p_max > (*nelements))
	    p = (bfd_ghist_info *) bfd_realloc(abfd,p,sizeof(bfd_ghist_info)*(*nelements));
    return p;
}

#define coff_slurp_armap		bfd_slurp_coff_armap
#define coff_slurp_extended_name_table	_bfd_slurp_extended_name_table
#define coff_truncate_arname		bfd_dont_truncate_arname
#define coff_openr_next_archived_file	bfd_generic_openr_next_archived_file
#define coff_generic_stat_arch_elt	bfd_generic_stat_arch_elt
#define	coff_get_section_contents	bfd_generic_get_section_contents
#define	coff_close_and_cleanup		bfd_generic_close_and_cleanup

static asymbol *coff_make_file_symbol(abfd,name)
    bfd *abfd;
    char *name;
{
    asymbol *ret = coff_make_empty_symbol(abfd);
    combined_entry_type *p;

    ret->name = name;
    p = (combined_entry_type *) bfd_zalloc(abfd,2*sizeof(combined_entry_type));
    ret->native_info = (char *) p;
    p->u.syment.n_sclass = C_FILE;
    p->u.syment.n_numaux = 1;
    p->u.syment.n_scnum = N_DEBUG;
    ret->flags |= BSF_DEBUGGING;
    return ret;
}

static void add_symbol(abfd,sym_list,max_cnt,curr_cnt,sym)
    bfd *abfd;
    asymbol ***sym_list;
    unsigned long *max_cnt,*curr_cnt;
    asymbol *sym;
{
    if ((*curr_cnt)+1 >= (*max_cnt))
	    *sym_list = (asymbol **)bfd_realloc(abfd,*sym_list,((*max_cnt) *= 2)*sizeof(asymbol *));
    (*sym_list)[*curr_cnt] = sym;
    (*curr_cnt)++;
}

static asymbol ** coff_set_sym_tab(abfd,location,symcount)
    bfd *abfd;
    asymbol **location;
    unsigned *symcount;
{
    int i;
    unsigned long max_syms = (*symcount)+1,max_nn_globals = (*symcount)+1,current_syms = 0,
    current_nn_globals = 0;
    asymbol **outsyms = (asymbol **) bfd_alloc(abfd,max_syms * sizeof(asymbol*)),
    **out_nn_globals = (asymbol **) bfd_alloc(abfd,max_nn_globals * sizeof(asymbol*));

#define IsNativeSymbol(ASYM) (ASYM->the_bfd && BFD_COFF_FILE_P(ASYM->the_bfd))
#define ADD_SYM(SYM) add_symbol(abfd,&outsyms,&max_syms,\
				&current_syms,SYM)
#define ADD_NN_G_SYM(SYM) add_symbol(abfd,&out_nn_globals,&max_nn_globals,\
				&current_nn_globals,SYM)

    for (i=0;i < (*symcount);i++) {
	if (! (IsNativeSymbol(location[i])) ) {
	    if ((location[i])->flags & BSF_ALL_OMF_REP) {

		if (location[i]->flags & BSF_ALREADY_EMT)
			continue;
		if ((location[i]->flags & BSF_HAS_CALL) ||
		    ((location[i]->flags & BSF_UNDEFINED) &&
		     (0 == (location[i]->flags & BSF_SYM_REFD_OTH_SECT))))
			continue;  /* Do not put out the related symbol. */
		/* FIXME: got to add a file symbol if no other file symbols are in the linkage. */
		if (location[i]->flags & BSF_LOCAL) {
		    if (!(location[i]->the_bfd->flags & ADDED_FILE_SYMBOL)) {
			ADD_SYM(coff_make_file_symbol(abfd,location[i]->the_bfd->filename));
			location[i]->the_bfd->flags |= ADDED_FILE_SYMBOL;
		    }
		    ADD_SYM(location[i]);
		    location[i]->native_info = NULL;
		    if (location[i]->related_symbol && location[i]->related_symbol->name &&
			strcmp("",location[i]->related_symbol->name)) {
			ADD_SYM(location[i]->related_symbol);
			location[i]->related_symbol->flags |= BSF_ALREADY_EMT;
			location[i]->related_symbol->native_info = NULL;
		    }
		}
		else {
		    ADD_NN_G_SYM(location[i]);
		    if (location[i]->related_symbol && location[i]->related_symbol->name &&
			strcmp("",location[i]->related_symbol->name)) {
			ADD_NN_G_SYM(location[i]->related_symbol);
			location[i]->related_symbol->flags |= BSF_ALREADY_EMT;
			location[i]->related_symbol->native_info = NULL;
		    }
		}
		location[i]->native_info = NULL;
	    }
	}
	else
		ADD_SYM(location[i]);
    }
    if (current_nn_globals) {
	int i;

	for (i=0;i < current_nn_globals;i++)
		ADD_SYM(out_nn_globals[i]);
    }
    bfd_release(abfd,out_nn_globals);
    ADD_SYM((asymbol*)0);
    bfd_get_outsymbols (abfd) = outsyms;
    *symcount = bfd_get_symcount (abfd) = --current_syms;
#if 0
    if (!(abfd->flags & SYMTAB_COMPRESSED) &&
	!(abfd->flags & DO_NOT_STRIP_ORPHANED_TAGS)) {
	/* Strategy for compressing coff symbol table:
	   1st mark all of the tags that are referenced as referenced.
	   also make sure that the set that are referenced are as small
	   as possible by comparing likenamed tags.
	   2nd remove the resulting orphaned tags. */
	coff_mark_all_tag_references(abfd);
	coff_remove_orphaned_tags(abfd);
    }
    coff_renumber_symbols(abfd);
    coff_mangle_symbols(abfd);
    if (COMPRESS_STRINGS(abfd))
	    coff_compress_long_string_table(abfd);
#endif
    return bfd_get_outsymbols(abfd);
}

static boolean coff_write_outsymbols(abfd)
    bfd *abfd;
{
    FILHDR internal_f;

    coff_write_symbols(abfd,0);
    bfd_seek(abfd, 0, SEEK_SET);
    bfd_read((PTR) &internal_f, 1, FILHSZ, abfd);
    PUTWORD(abfd, bfd_get_symcount(abfd), internal_f.f_nsyms);
    bfd_seek(abfd, 0, SEEK_SET);
    bfd_write((PTR) &internal_f, 1, FILHSZ, abfd);
    coff_write_linenumbers(abfd);
}

bfd_target icoff_little_vec =
{
#if !defined(NO_BIG_ENDIAN_MODS)
  BFD_LITTLE_COFF_TARG,         /* name */
#else
  "coff-Intel-little",          /* name */
#endif /* NO_BIG_ENDIAN_MODS */

  bfd_target_coff_flavour_enum,
  false,			/* data byte order is little */
  false,			/* header byte order is little */

  (HAS_RELOC | EXEC_P |		/* object flags */
   HAS_LINENO | HAS_DEBUG |
   HAS_SYMS | HAS_LOCALS | DYNAMIC | WP_TEXT |
   HAS_CCINFO | HAS_PID | HAS_PIC | CAN_LINKPID | SYMTAB_COMPRESSED |
   DO_NOT_ALTER_RELOCS),

  (SEC_ALL_FLAGS), /* section flags. */

  '/',				/* ar_pad_char */
  15,				/* ar_max_namelen */

  _do_getl64, _do_putl64, _do_getl32, _do_putl32, _do_getl16, _do_putl16, /* data */
  _do_getl64, _do_putl64, _do_getl32, _do_putl32, _do_getl16, _do_putl16, /* hdrs */

    {_bfd_dummy_target, coff_object_p, /* bfd_check_format */
       bfd_generic_archive_p, _bfd_dummy_target},
    {bfd_false, coff_mkobject,	/* bfd_set_format */
       _bfd_generic_mkarchive, bfd_false},
    {bfd_false, coff_write_object_contents, /* bfd_write_contents */
       _bfd_write_archive_contents, bfd_false},
  JUMP_TABLE(coff),
  };


bfd_target icoff_big_vec =
{
#if !defined(NO_BIG_ENDIAN_MODS)
  BFD_BIG_COFF_TARG,            /* name */
#else
  "coff-Intel-big",             /* name */
#endif /* NO_BIG_ENDIAN_MODS */

  bfd_target_coff_flavour_enum,
  false,			/* data byte order is little */
  true,				/* header byte order is big */

  (HAS_RELOC | EXEC_P |		/* object flags */
   HAS_LINENO | HAS_DEBUG |
   HAS_SYMS | HAS_LOCALS | DYNAMIC | WP_TEXT |
   HAS_CCINFO | HAS_PID | HAS_PIC | CAN_LINKPID | SYMTAB_COMPRESSED |
   DO_NOT_ALTER_RELOCS),

  (SEC_ALL_FLAGS), /* section flags. */

  '/',				/* ar_pad_char */
  15,				/* ar_max_namelen */

_do_getl64, _do_putl64,  _do_getl32, _do_putl32, _do_getl16, _do_putl16, /* data */
_do_getb64, _do_putb64,  _do_getb32, _do_putb32, _do_getb16, _do_putb16, /* hdrs */

  {_bfd_dummy_target, coff_object_p, /* bfd_check_format */
     bfd_generic_archive_p, _bfd_dummy_target},
  {bfd_false, coff_mkobject,	/* bfd_set_format */
     _bfd_generic_mkarchive, bfd_false},
  {bfd_false, coff_write_object_contents,	/* bfd_write_contents */
     _bfd_write_archive_contents, bfd_false},
  JUMP_TABLE(coff),
};

#if !defined(NO_BIG_ENDIAN_MODS)
/*
 * icoff_little_big_vec is for host byte order little-endian and i960
 * byte order big-endian.
 */

bfd_target icoff_little_big_vec =
{
  BFD_LITTLE_BIG_COFF_TARG,	/* name */
  bfd_target_coff_flavour_enum,
  true,				/* target byte order is big */
  false,			/* header byte order is little */

  (HAS_RELOC | EXEC_P |		/* object flags */
   HAS_LINENO | HAS_DEBUG |
   HAS_SYMS | HAS_LOCALS | DYNAMIC | WP_TEXT |
   HAS_CCINFO | HAS_PID | HAS_PIC | CAN_LINKPID | SYMTAB_COMPRESSED |
   DO_NOT_ALTER_RELOCS),

  (SEC_ALL_FLAGS), /* section flags. */

  '/',				/* ar_pad_char */
  15,				/* ar_max_namelen */

  _do_getb64, _do_putb64, _do_getb32, _do_putb32, _do_getb16, _do_putb16, /* data */
  _do_getl64, _do_putl64, _do_getl32, _do_putl32, _do_getl16, _do_putl16, /* hdrs */

    {_bfd_dummy_target, coff_object_p, /* bfd_check_format */
       bfd_generic_archive_p, _bfd_dummy_target},
    {bfd_false, coff_mkobject,	/* bfd_set_format */
       _bfd_generic_mkarchive, bfd_false},
    {bfd_false, coff_write_object_contents, /* bfd_write_contents */
       _bfd_write_archive_contents, bfd_false},
  JUMP_TABLE(coff),
  };

/*
 * icoff_big_big_vec is for host byte order big-endian and i960
 * byte order big-endian.
 */

bfd_target icoff_big_big_vec =
{
  BFD_BIG_BIG_COFF_TARG,	/* name */
  bfd_target_coff_flavour_enum,
  true,				/* target byte order is big */
  true,				/* header byte order is big */

  (HAS_RELOC | EXEC_P |		/* object flags */
   HAS_LINENO | HAS_DEBUG |
   HAS_SYMS | HAS_LOCALS | DYNAMIC | WP_TEXT |
   HAS_CCINFO | HAS_PID | HAS_PIC | CAN_LINKPID | SYMTAB_COMPRESSED |
   DO_NOT_ALTER_RELOCS),

  (SEC_ALL_FLAGS), /* section flags. */

  '/',				/* ar_pad_char */
  15,				/* ar_max_namelen */

_do_getb64, _do_putb64,  _do_getb32, _do_putb32, _do_getb16, _do_putb16, /* data */
_do_getb64, _do_putb64,  _do_getb32, _do_putb32, _do_getb16, _do_putb16, /* hdrs */

  {_bfd_dummy_target, coff_object_p, /* bfd_check_format */
     bfd_generic_archive_p, _bfd_dummy_target},
  {bfd_false, coff_mkobject,	/* bfd_set_format */
     _bfd_generic_mkarchive, bfd_false},
  {bfd_false, coff_write_object_contents,	/* bfd_write_contents */
     _bfd_write_archive_contents, bfd_false},
  JUMP_TABLE(coff),
};
#endif /* NO_BIG_ENDIAN_MODS */
