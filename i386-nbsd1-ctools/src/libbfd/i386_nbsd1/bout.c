/* BFD back-end for i960 b.out binaries */

/*
 * Copyright (C) 1991 Free Software Foundation, Inc.
 *
 * This file is part of BFD, the Binary File Diddler.
 *
 * GLD is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * GLD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GLD; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "bout.h"
#include "stab.h"
#include "libaout.h"		/* BFD a.out internal data structures */

/* Align an address by rounding it up to a power of two.  It leaves the
 * address unchanged if align == 0 (2^0 = alignment of 1 byte)
 */
#define	i960_align(addr, align)	(((addr) + ((1<<(align))-1)) & (-1 << (align)))

PROTO (static boolean, bout_squirt_out_relocs,(bfd *abfd, asection *section));
PROTO (static bfd_target *, bout_callback, (bfd *));
PROTO (boolean, aout_slurp_symbol_table, (bfd *abfd));
PROTO (void , aout_write_syms, ());
PROTO (static void, swap_exec_header, (bfd *abfd, struct exec *execp));

PROTO(bfd_ghist_info *,aout_fetch_ghist_info,(bfd *,unsigned int *,unsigned int *));

static bfd_target *
bout_little_object_p (abfd)
     bfd *abfd;
{
	unsigned char magicbytes[LONG_SIZE];
	struct exec anexec;

	if (bfd_read ((PTR)magicbytes, 1, LONG_SIZE, abfd) != LONG_SIZE) {
		bfd_error = system_call_error;
		return 0;
	}
	anexec.a_magic = _do_getl32 (magicbytes);

	if (N_BADMAG (anexec)) {
		bfd_error = wrong_format;
		return 0;
	}
	return aout_some_aout_object_p (abfd, bout_callback);
}

static bfd_target *
bout_big_object_p (abfd)
     bfd *abfd;
{
	unsigned char magicbytes[LONG_SIZE];
	struct exec anexec;

	if (bfd_read ((PTR)magicbytes, 1, LONG_SIZE, abfd) != LONG_SIZE) {
		bfd_error = system_call_error;
		return 0;
	}

	anexec.a_magic = _do_getb32 (magicbytes);

	if (N_BADMAG (anexec)) {
		bfd_error = wrong_format;
		return 0;
	}
	return aout_some_aout_object_p (abfd, bout_callback);
}

/* Finish up the opening of a b.out file for reading.  Fill in all the
 * fields that are not handled by common code.
 */
static bfd_target *
bout_callback (abfd)
     bfd *abfd;
{
	struct exec anexec;
	struct exec *execp = &anexec;
	unsigned long bss_start;

	/* Reread exec header, because the common code didn't get all of
	 * our extended header.
	 */
	if (bfd_seek (abfd, 0L, SEEK_SET) < 0) {
		bfd_error = system_call_error;
		return 0;
	}

	if (bfd_read ((PTR) execp, 1, sizeof (struct exec), abfd)
	    != sizeof (struct exec)) {
		bfd_error = wrong_format;
		return 0;
	}
	swap_exec_header (abfd, execp);

	/* Architecture and machine type */
	abfd->obj_arch = bfd_arch_i960;	/* B.out only used on i960 */
	abfd->obj_machine = bfd_mach_i960_core;	/* Default */
	bfd_set_target_arch(abfd,BFD_960_GENERIC);
	bfd_set_target_attributes(abfd,BFD_960_MACH_CORE_1);
	/* FIXME:  Set real machine type from file here */

	/* The positions of the string table and symbol table.  */
	obj_str_filepos (abfd) = N_STROFF (anexec);
	obj_sym_filepos (abfd) = N_SYMOFF (anexec);

	/* The alignments of the sections */
	obj_textsec (abfd)->alignment_power = execp->a_talign;
	obj_datasec (abfd)->alignment_power = execp->a_dalign;
	obj_bsssec  (abfd)->alignment_power = execp->a_balign;

	/* The starting addresses of the sections */
	obj_textsec (abfd)->vma = obj_textsec (abfd)->pma = anexec.a_tload;
	obj_datasec (abfd)->vma = obj_datasec (abfd)->pma = anexec.a_dload;
	bss_start = anexec.a_dload + anexec.a_data; /* BSS = end of data section */
	obj_bsssec (abfd)->vma = obj_bsssec (abfd)->pma = 
	        i960_align (bss_start, anexec.a_balign);

	obj_textsec(abfd)->paddr_file_offset = ((unsigned long) (&anexec.a_tload)) -
		((unsigned long) (&anexec));
	obj_datasec(abfd)->paddr_file_offset = ((unsigned long) (&anexec.a_dload)) -
		((unsigned long) (&anexec));

	obj_bsssec(abfd)->paddr_file_offset = BAD_PADDR_FILE_OFFSET;

	/* The file positions of the sections */
	obj_textsec (abfd)->filepos = N_TXTOFF(anexec);
	obj_datasec (abfd)->filepos = N_DATOFF(anexec);

	/* The file positions of the relocation info */
	obj_textsec (abfd)->rel_filepos = N_TROFF(anexec);
	obj_datasec (abfd)->rel_filepos =  N_DROFF(anexec);

	if (N_HAS_CCINFO(anexec)) {
		abfd->flags |= HAS_CCINFO;
	}

	return abfd->xvec;
}


static boolean
bout_mkobject (abfd)
     bfd *abfd;
{
	char *rawptr;

	rawptr = (char *) bfd_zalloc(abfd,sizeof(struct aoutdata) + sizeof(struct exec));
	if (!rawptr) {
		bfd_error = no_memory;
		return 0;
	}

	set_tdata(abfd, (struct aoutdata *) rawptr);
	exec_hdr(abfd) = (struct exec*)(rawptr + sizeof(struct aoutdata));

	/* For simplicity's sake we just make all the sections right here. */
	obj_textsec (abfd) = (asection *)NULL;
	obj_datasec (abfd) = (asection *)NULL;
	obj_bsssec (abfd) = (asection *)NULL;

	bfd_make_section (abfd, ".text");
	bfd_make_section (abfd, ".data");
	bfd_make_section (abfd, ".bss");

	return 1;
}

#define HOWTO_ABS32	1
#define HOWTO_PCREL24	2
#define HOWTO_CALLJ	3
#define HOWTO_CALLJX    4

			/* name       type          pc-rel  mask	*/
			/* ----       ----          ------  ----	*/
static reloc_howto_type
howto_reloc_abs32 =	{ "abs32",   HOWTO_ABS32,   bfd_reloc_type_32bit_abs  };

static reloc_howto_type
howto_reloc_pcrel24 =	{ "pcrel24", HOWTO_PCREL24, bfd_reloc_type_24bit_pcrel };

static reloc_howto_type
howto_reloc_callj =	{ "callj",   HOWTO_CALLJ,   bfd_reloc_type_opt_call };

static reloc_howto_type
howto_reloc_calljx =	{ "calljx",  HOWTO_CALLJX,  bfd_reloc_type_opt_callx };


/* Remove unneeded relocation directives, removed in bfd_perform_relocation(). */
static void
bout_futz_relocs(abfd,section)
    bfd *abfd;
    asection *section;
{
    unsigned int count = section->reloc_count;
    arelent **src,**dest;

    if (count == 0)
	    return;

    src = dest = section->orelocation;

    for (; count > 0; --count, ++src) {
	arelent *g = *src;

	switch (g->howto->reloc_type) {
    case bfd_reloc_type_32bit_abs:
	    *dest = *src;
	    (*dest)->howto = &howto_reloc_abs32;
	    dest++;
	    break;
    case bfd_reloc_type_24bit_pcrel:
	    *dest = *src;
	    (*dest)->howto = &howto_reloc_pcrel24;
	    dest++;
	    break;
    case bfd_reloc_type_none:
	    section->reloc_count--;
	    break;
    case bfd_reloc_type_opt_call:
	    *dest = *src;
	    (*dest)->howto = &howto_reloc_callj;
	    dest++;
	    break;
    case bfd_reloc_type_opt_callx:
	    *dest = *src;
	    (*dest)->howto = &howto_reloc_calljx;
	    dest++;
	    break;
    default:
	    fprintf(stderr,"File: %s inherited unsupported relocation type: %d, from section: %s\n",
		    abfd->filename,g->howto->reloc_type,section->name);
	    exit(1);
	}
    }
}

static boolean
bout_write_object_contents (abfd)
     bfd *abfd;
{
	struct exec swapped_hdr;

	exec_hdr(abfd)->a_magic = BMAGIC;
	exec_hdr(abfd)->a_text  = obj_textsec(abfd)->size;
	exec_hdr(abfd)->a_data  = obj_datasec(abfd)->size;
	exec_hdr(abfd)->a_bss   = obj_bsssec(abfd)->size;
	exec_hdr(abfd)->a_syms  = bfd_get_symcount(abfd)*sizeof(struct nlist);
	exec_hdr(abfd)->a_entry = bfd_get_start_address(abfd);
	exec_hdr(abfd)->a_talign= obj_textsec(abfd)->alignment_power;
	exec_hdr(abfd)->a_dalign= obj_datasec(abfd)->alignment_power;
	exec_hdr(abfd)->a_balign= obj_bsssec(abfd)->alignment_power;
	exec_hdr(abfd)->a_tload = obj_textsec(abfd)->vma;
	exec_hdr(abfd)->a_dload = obj_datasec(abfd)->vma;
	if ((bfd_get_file_flags(abfd) & HAS_CCINFO) &&
	    (bfd_get_file_flags(abfd) & HAS_RELOC))
		exec_hdr(abfd)->a_ccinfo = N_CCINFO;
	/* Now write out reloc info, followed by syms and strings */
	if (bfd_get_symcount (abfd) != 0) {
	    bout_futz_relocs(abfd,obj_textsec(abfd));
	    bout_futz_relocs(abfd,obj_datasec(abfd));

	    exec_hdr(abfd)->a_trsize= obj_textsec(abfd)->reloc_count *
		    sizeof(struct relocation_info);
	    exec_hdr(abfd)->a_drsize= obj_datasec(abfd)->reloc_count *
		    sizeof(struct relocation_info);

	    bfd_seek (abfd, (long)(N_SYMOFF(*exec_hdr(abfd))), SEEK_SET);
	    aout_write_syms (abfd);
	    /* ccinfo, if any, must immmediately follow */
	    if (bfd_get_file_flags(abfd) & HAS_CCINFO) {
		if (!bfd960_write_ccinfo (abfd)){
		    return 0;
		}
	    }
	    bfd_seek (abfd,(long)(N_TROFF(*exec_hdr(abfd))), SEEK_SET);
	    if (!bout_squirt_out_relocs (abfd, obj_textsec (abfd))){
		return 0;
	    }
	    bfd_seek (abfd, (long)(N_DROFF(*exec_hdr(abfd))), SEEK_SET);
	    if (!bout_squirt_out_relocs (abfd, obj_datasec (abfd))) {
		return 0;
	    }
	}
	else {
	    exec_hdr(abfd)->a_trsize= obj_textsec(abfd)->reloc_count *
		    sizeof(struct relocation_info);
	    exec_hdr(abfd)->a_drsize= obj_datasec(abfd)->reloc_count *
		    sizeof(struct relocation_info);
	}
	swapped_hdr = *exec_hdr(abfd);
	swap_exec_header (abfd, &swapped_hdr);

	bfd_seek (abfd, 0L, SEEK_SET);
	bfd_write ((PTR) &swapped_hdr, 1, sizeof (struct exec), abfd);
	return 1;
}

static void
swap_exec_header (abfd, execp)
     bfd *abfd;
     struct exec *execp;
{
#define swapme(field)	field = GETWORD(abfd,field);

	swapme(execp->a_magic);
	swapme(execp->a_text);
	swapme(execp->a_data);
	swapme(execp->a_bss);
	swapme(execp->a_syms);
	swapme(execp->a_entry);
	swapme(execp->a_trsize);
	swapme(execp->a_drsize);
	swapme(execp->a_tload);
	swapme(execp->a_dload);
	/* Others are 1-byte fields, don't need swap */

#undef swapme
}


/* Allocate enough room for all the reloc entries, plus pointers to them all
 */
static boolean
bout_slurp_reloc_table (abfd, asect, symbols)
     bfd *abfd;
     sec_ptr asect;
     asymbol **symbols;
{
	unsigned int count;
	size_t  reloc_size;
	struct relocation_info *relocs;
	arelent *relp;
	register struct relocation_info *rptr;
	int extern_mask, pcrel_mask, callj_mask, calljx_mask;

	if (asect->relocation) return 1;
	if (!aout_slurp_symbol_table (abfd)) return 0;

	if (asect == obj_datasec (abfd)) {
		reloc_size = exec_hdr(abfd)->a_drsize;
	} else if (asect == obj_textsec (abfd)) {
		reloc_size = exec_hdr(abfd)->a_trsize;
	} else {
		bfd_error = invalid_operation;
		return 0;
	}

	bfd_seek (abfd, (long)(asect->rel_filepos), SEEK_SET);
	count = reloc_size / sizeof (struct relocation_info);

	if (reloc_size) {
	    relocs = (struct relocation_info *) bfd_alloc (abfd,reloc_size);
	    relp = (arelent *) bfd_alloc (abfd,(count+1) * sizeof (arelent));
	    if (!relp) {
		bfd_release(abfd,(char*)relocs);
		bfd_error = no_memory;
		return 0;
	    }
	    if (bfd_read ((PTR) relocs, 1, reloc_size, abfd) != reloc_size) {
		bfd_error = system_call_error;
		bfd_release(abfd,relp);
		bfd_release(abfd,relocs);
		return 0;
	    }

	    if (abfd->xvec->header_byteorder_big_p) {
		/* Big-endian bit field allocation order */
		pcrel_mask  = 0x80;
		extern_mask = 0x10;
		callj_mask  = 0x02;
		calljx_mask = 0x01;
	    } else {
		/* Little-endian bit field allocation order */
		pcrel_mask  = 0x01;
		extern_mask = 0x08;
		callj_mask  = 0x40;
		calljx_mask = 0x80;
	    }

	    asect->relocation  = relp;
	    asect->reloc_count = count;
	
	    for ( rptr=relocs; count--; rptr++, relp++) {
		unsigned char *raw = (unsigned char *)rptr;
		unsigned int symnum;
		asymbol *ref;

		relp->address = bfd_h_get_32 (abfd, raw + 0);
		if (abfd->xvec->header_byteorder_big_p)
			symnum = (raw[4] << 16) | (raw[5] << 8) | raw[6];
		else
			symnum = (raw[6] << 16) | (raw[5] << 8) | raw[4];

		/* If the extern bit is set then the r_index is a index into
		 * the symbol table; otherwise r_index indicates a section
		 * We either fill in the sym entry with a pointer to the
		 * symbol, or point to the correct section.
		 */
		if (raw[7] & extern_mask)
			relp->sym_ptr_ptr = symbols + symnum;
		else {
		    /* Symbols are relative to the beginning of
		     * the file rather than sections.  The reloc entry
		     * addend has added to it the offset into the file of
		     * the data, so subtract the base to make the reloc
		     * section-relative.
		     */
		    switch (symnum) {
		case N_TEXT:
		case N_TEXT | N_EXT:
			relp->sym_ptr_ptr = &obj_textsec(abfd)->sect_sym;
			break;
		case N_DATA:
		case N_DATA | N_EXT:
			relp->sym_ptr_ptr = &obj_datasec(abfd)->sect_sym;
			break;
		case N_BSS:
		case N_BSS | N_EXT:
			relp->sym_ptr_ptr = &obj_bsssec(abfd)->sect_sym;
			break;
		case N_ABS:
		case N_ABS | N_EXT:  /* This is a noop relocation true?
					Why is this here? */
			break;
		default:
			BFD_ASSERT(0);
			break;
		    }
		}
		ref = *(relp->sym_ptr_ptr);
		ref->flags |= BSF_SYM_REFD_OTH_SECT;

		if (raw[7] & callj_mask)
			relp->howto = &howto_reloc_callj;
		else if (raw[7] & calljx_mask) {
		    relp->howto = &howto_reloc_calljx;
		    relp->address -= 4;
		}
		else if ( raw[7] & pcrel_mask)
			relp->howto = &howto_reloc_pcrel24;
		else
			relp->howto = &howto_reloc_abs32;
	    }
	    bfd_release(abfd,relocs);
	}
	return 1;
}


static boolean
bout_squirt_out_relocs (abfd, section)
     bfd *abfd;
     asection *section;
{
	arelent **generic;
	struct relocation_info *native, *natptr;
	int extern_mask, pcrel_mask, len_2, callj_mask, calljx_mask;
	boolean retval;

	unsigned int count = section->reloc_count;
	size_t natsize = count * sizeof (struct relocation_info);

	if (count == 0){
		return 1;
	}

	generic = section->orelocation;
	native = ((struct relocation_info *) bfd_alloc (abfd,natsize));
	if (!native) {
		bfd_error = no_memory;
		return 0;
	}

	if (abfd->xvec->header_byteorder_big_p) {
		/* Big-endian bit field allocation order */
		pcrel_mask  = 0x80;
		extern_mask = 0x10;
		len_2       = 0x40;
		callj_mask  = 0x02;
		calljx_mask = 0x01;
	} else {
		/* Little-endian bit field allocation order */
		pcrel_mask  = 0x01;
		extern_mask = 0x08;
		len_2       = 0x04;
		callj_mask  = 0x40;
		calljx_mask = 0x80;
	}

	for (natptr = native; count > 0; --count, ++natptr, ++generic) {
	    unsigned int symnum;
	    arelent *g = *generic;
	    unsigned char *raw = (unsigned char *)natptr;

	    bfd_h_put_32(abfd, g->address, raw);

	    /* Find a type in the output format which matches the input
	     * howto.  We assume input format == output format.
	     */

	    raw[7] = len_2;
	    if (g->howto == &howto_reloc_callj) {
		raw[7] |= pcrel_mask | callj_mask;
	    }
	    else if (g->howto == &howto_reloc_calljx) {
		bfd_h_put_32(abfd, g->address + 4, raw);
		raw[7] |= calljx_mask;
	    } else if (g->howto == &howto_reloc_pcrel24) {
		raw[7] |= pcrel_mask;
	    }
	    
	    if (g->sym_ptr_ptr && !((*g->sym_ptr_ptr)->flags & BSF_TMP_REL_SYM)) {
		symnum = (*(g->sym_ptr_ptr))->sym_tab_idx;
		raw[7] |= extern_mask;
	    }
	    else if ((*g->sym_ptr_ptr)->flags & BSF_TMP_REL_SYM) {
		asection *os = (*g->sym_ptr_ptr)->section->output_section;
		if (os == obj_textsec(abfd)) {
		    symnum = N_TEXT;
		} else if (os == obj_datasec(abfd)) {
		    symnum  = N_DATA;
		} else if (os == obj_bsssec(abfd)) {
		    symnum = N_BSS;
		} else {
		    BFD_ASSERT(0);
		}
	    } else {
		symnum = N_ABS;
		BFD_ASSERT(0);
	    }
	    
	    if (abfd->xvec->header_byteorder_big_p) {
		raw[4] = (unsigned char) (symnum >> 16);
		raw[5] = (unsigned char) (symnum >>  8);
		raw[6] = (unsigned char) (symnum      );
	    } else {
		raw[6] = (unsigned char) (symnum >> 16);
		raw[5] = (unsigned char) (symnum >>  8);
		raw[4] = (unsigned char) (symnum      );
	    }
	}
	
	retval = (bfd_write((PTR)native,1,section->reloc_count*sizeof(struct relocation_info),abfd) ==
		  (section->reloc_count*sizeof(struct relocation_info)));
	bfd_release(abfd,(PTR)native);
	return retval;
}


static unsigned int
bout_canonicalize_reloc (abfd, section, relptr, symbols)
    bfd *abfd;
    sec_ptr section;
    arelent **relptr;
    asymbol **symbols;
{
	arelent *tblptr = section->relocation;
	unsigned int count;

	if (!(tblptr || bout_slurp_reloc_table (abfd, section, symbols))) {
		return 0;
	}
	tblptr = section->relocation;
	if (!tblptr) {
		return 0;
	}

	for (count = 0; count < section->reloc_count; count++) {
		*relptr++ = tblptr++;
	}
	*relptr = 0;
	return section->reloc_count;
}


static unsigned int
bout_get_reloc_upper_bound (abfd, asect)
    bfd *abfd;
    sec_ptr asect;
{
#define NUM_RELOCS(a_Xrsize) \
		(exec_hdr(abfd)->a_Xrsize / sizeof(struct relocation_info))

	if (bfd_get_format(abfd) == bfd_object) {
		if (asect == obj_datasec (abfd)){
			return (NUM_RELOCS(a_drsize)+1) * sizeof(arelent*);
		}

		if (asect == obj_textsec (abfd)) {
			return (NUM_RELOCS(a_trsize)+1) * sizeof(arelent*);
		}
	}
	bfd_error = invalid_operation;
	return 0;
}

static boolean
bout_set_section_contents (abfd, section, location, offset, count)
    bfd *abfd;
    sec_ptr section;
    unsigned char *location;
    file_ptr offset;
    int count;
{
	if (abfd->output_has_begun == 0) { /* set by bfd.c handler */
		if (!obj_textsec(abfd) || !obj_datasec(abfd)) {
			bfd_error = invalid_operation;
			return 0;
		}
		obj_textsec (abfd)->filepos = sizeof(struct exec);
		obj_datasec(abfd)->filepos = obj_textsec(abfd)->filepos
						+  obj_textsec (abfd)->size;
	}
	bfd_seek (abfd, section->filepos + offset, SEEK_SET);
	if (count != 0) {
		return bfd_write((PTR)location,1,count,abfd) == count;
	}
	return 0;
}


static boolean
bout_set_arch_mach (abfd, arch, machine, real_machine)
    bfd *abfd;
    enum bfd_architecture arch;
    unsigned long machine;
    unsigned long real_machine;
{
	abfd->obj_arch = arch;
	abfd->obj_machine = machine;
	bfd_set_target_arch(abfd,real_machine);
	if (arch == bfd_arch_unknown)	/* Unknown machine arch is OK */
		return 1;
	if (arch == bfd_arch_i960){
		switch (machine) {
		case bfd_mach_i960_core:
		case bfd_mach_i960_kb_sb:
		case bfd_mach_i960_ca:
		case bfd_mach_i960_jx:
		case bfd_mach_i960_ka_sa:
			return 1;
		}
	}
	return 0;
}

static int
DEFUN(bout_sizeof_headers,(abfd, execable),
      bfd *abfd AND
      boolean execable)
{
	return sizeof(struct exec);
}



/* Assume file contains ccinfo.  Position file pointer at start of it.
 * I.e., seek past the symbol table, read the string table length, and
 * seek past the string table.
 *
 * Return 0 on failure, non-zero on success
 */
int
bout_seek_ccinfo( abfd )
    bfd *abfd;
{
	int strtab_len;

	bfd_error = system_call_error;
	if ( (bfd_seek(abfd,obj_str_filepos(abfd),SEEK_SET) < 0)
	||   (bfd_read(&strtab_len,4,1,abfd) != 4) ){
		return 0;
	}

	strtab_len = bfd_h_get_32(abfd,&strtab_len);
	if ( bfd_seek(abfd,strtab_len-4,SEEK_CUR) < 0 ){
		return 0;
	}
	return 1;
}

static
int
aout_dmp_symtab( abfd, no_headers, no_translation, section_number )
	bfd 	*abfd;
	int	no_headers, no_translation, section_number;
{
    asymbol **p,**syms = (asymbol **) bfd_alloc(abfd,get_symtab_upper_bound(abfd));

    if (!no_headers) {
	bfd_center_header("BOUT SYMBOL TABLE");
	printf("   Value   | S | O | Symbol Name\n");
	printf("-----------+---+---+--------------------------------\n");
    }
    bfd_canonicalize_symtab(abfd,syms);
    for (p=syms;p && *p;p++) {
	asymbol *sym = *p;
	unsigned long val = sym->value;
	char other = ' ',sect_name = '?';

	if (section_number >= 0) {
	    if (!sym->section || (sym->section->secnum+1) != section_number)
		    continue;
	}

	if (sym->section) {
	    val += sym->section->vma;
	    sect_name = *(sym->section->name+1);
	}
	else {
	    if (sym->flags & BSF_ABSOLUTE)
		    sect_name = 'a';
	    else if (sym->flags & BSF_FORT_COMM)
		    sect_name = 'C';
	    else if (sym->flags & BSF_UNDEFINED)
		    sect_name = 'U';
	    else if (sym->flags & BSF_HAS_SCALL)
		    sect_name = 's';
	}

	if (sym->flags & (BSF_EXPORT | BSF_GLOBAL))
		sect_name -= ('a' - 'A');

	if (sym->flags & BSF_HAS_SCALL) {
	    other = 'S';
	    val = sym->related_symbol->value;
	}
	else if (sym->flags & BSF_HAS_BAL)
		other = 'C';
	else if (sym->flags & BSF_HAS_CALL)
		other = 'L';

	printf("0x%08x | %c | %c | %s\n",val,sect_name,other,sym->name ? sym->name : "");
    }
    return 1;
}

static
int
aout_dmp_linenos( abfd, dummy1, dummy2 )
	bfd 	*abfd;
	int	dummy1;  /* for interface compatibility with coff version */
	int	dummy2;  /* ditto */
{
	fprintf(stderr, "Line table dump not implemented for b.out files\n" );
	return 1;

}


static int
aout_more_symbol_info(abfd, asym, m, info_type)
    bfd     *abfd;
    asymbol *asym;
    more_symbol_info *m;
    bfd_sym_more_info_type info_type;
{
    switch (info_type) {
 case bfd_sysproc_val:
	if (asym->flags & BSF_HAS_SCALL) {
	    asymbol *as = asym->related_symbol;

	    m->sysproc_value = as->value;
	    return 1;
	}
	else
		return 0;

 case bfd_set_sysproc_val:
	if (asym->flags & BSF_HAS_SCALL) {
	    asymbol *as = asym->related_symbol;

	    as->value = m->sysproc_value;
	    return 1;
	}
	else
		return 0;
	
 default:
	return 0;
    }
}


static
int
aout_dmp_full_fmt( abfd, syms, symcount, flags )
    bfd *abfd;
    asymbol **syms;
    unsigned long symcount;
    char flags;
{
    return 0; /* 7-column full format display not implemented for b.out */
}

static
int
aout_zero_line_info( abfd )
    bfd *abfd;
{
        return 0; /* zeroing line info not supported for b.out */
}

/* Build the transfer vectors for Big and Little-Endian B.OUT files.  */

/* We use BSD-Unix generic archive files.  */
#define	aout_openr_next_archived_file	bfd_generic_openr_next_archived_file
#define	aout_generic_stat_arch_elt	bfd_generic_stat_arch_elt
#define	aout_slurp_armap		bfd_slurp_bsd_armap
#define	aout_slurp_extended_name_table	_bfd_slurp_extended_name_table
#define	aout_write_armap		bsd_write_armap
#define	aout_truncate_arname		bfd_bsd_truncate_arname

/* We override these routines from the usual a.out file routines.  */
#define	aout_perform_relocation		bout_perform_relocation
#define	aout_canonicalize_reloc		bout_canonicalize_reloc
#define	aout_get_reloc_upper_bound	bout_get_reloc_upper_bound
#define	aout_set_section_contents	bout_set_section_contents
#define	aout_set_arch_mach		bout_set_arch_mach
#define	aout_sizeof_headers		bout_sizeof_headers

static asymbol ** aout_set_sym_tab(abfd,location,symcount)
    bfd *abfd;
    asymbol **location;
    unsigned *symcount;
{
    unsigned int count,nchanges=0;
    aout_symbol_type **canonical;
    int buffer,out_sym_count = 0,max_out_symbols = bfd_get_symcount(abfd)+1;
    asymbol **generic = bfd_get_outsymbols (abfd);

    /* Canonicalize symbol table, which can have gotten scrambled by
       incremental linking */
    canonical = (aout_symbol_type **)bfd_alloc(abfd,sizeof(aout_symbol_type *) * max_out_symbols);

#define ADD_SYM(SYM) if (++out_sym_count > max_out_symbols) {                                        \
			canonical = (aout_symbol_type **) bfd_realloc(abfd,canonical,                \
			     sizeof(aout_symbol_type *) * (max_out_symbols = (max_out_symbols*3)/2));\
		     }                                                                               \
		     canonical[out_sym_count-1] = (SYM);                                             \
	             if (SYM) (SYM)->symbol.flags |= BSF_ALREADY_EMT

    for (count = 0; count < bfd_get_symcount (abfd); count++) {
	asymbol *g = generic[count];

#define IsNativeSymbol(ASYM) (ASYM->the_bfd && BFD_BOUT_FILE_P(ASYM->the_bfd))

	if (IsNativeSymbol(g)) {
	    aout_symbol_type *h;

	    if (g->flags & (BSF_ALREADY_EMT | BSF_TMP_REL_SYM)) {
		nchanges++;
		/* We already wrote this one out OR this is a temporary relocation symbol not
		 native to bout. */
		continue;
	    }
	    h = aout_symbol(g);
	    ADD_SYM(h);
	    if ((g->flags & BSF_HAS_BAL) &&
		!(g->flags & BSF_ALREADY_EMT)) {
		nchanges++;
		ADD_SYM( (aout_symbol_type *) g->related_symbol );
	    }
	}
	else if (g->flags & BSF_ALL_OMF_REP) {
	    if (!(g->flags & (BSF_ALREADY_EMT | BSF_HAS_CALL))) {
		if (((g->flags & BSF_UNDEFINED) &&
		     (0 == (g->flags & BSF_SYM_REFD_OTH_SECT))) ||
		    ((g->flags & BSF_HAS_SCALL) &&
		     (!IS_SYSPROCIDX(g->related_symbol->value)))) {
		    nchanges++;
		    continue;  /* Silently elide this symbol entry. */
		}
		g->native_info = NULL;
		ADD_SYM( (aout_symbol_type *) g );
		if (g->flags & BSF_HAS_BAL) {
		    nchanges++;
		    g->related_symbol->native_info = NULL;
		    ADD_SYM( (aout_symbol_type *) g->related_symbol );
		    g->related_symbol->name = bfd_alloc(abfd,strlen(g->name)+4);
		    sprintf((char *)g->related_symbol->name,"%s$LF",g->name);
		    if (g->flags & (BSF_GLOBAL|BSF_EXPORT))
			    g->related_symbol->flags |= (BSF_GLOBAL|BSF_EXPORT);
		}
	    }
	}
	else
		nchanges++;
    }
    if (nchanges) {
	/* Add a trailing 0 to the output symbol table list: */
	ADD_SYM((aout_symbol_type *)0);
	*symcount = bfd_get_symcount(abfd) = (out_sym_count-1);
	/* Resize canonical list to make sure it does not use too much memory:
	 (it is currently malloc'd for max_out_symbols). */
	bfd_get_outsymbols(abfd) = (asymbol **) bfd_realloc(abfd,canonical,
					       sizeof(aout_symbol_type *) * out_sym_count);
	bfd_release(abfd,generic);
    }
    else
	    bfd_release(abfd,canonical);

    return bfd_get_outsymbols(abfd);
}

static boolean aout_write_outsymbols(abfd)
    bfd *abfd;
{
    bfd_seek (abfd, (long)(N_SYMOFF(*exec_hdr(abfd))), SEEK_SET);
    aout_write_syms (abfd);
}

bfd_target b_out_vec_big_host =
{
  "b.out.big",			/* name */
  bfd_target_aout_flavour_enum,
  false,			/* data byte order is little */
  true,				/* hdr byte order is big */
  (HAS_RELOC | EXEC_P |		/* object flags */
   HAS_LINENO | HAS_DEBUG | HAS_CCINFO |
   HAS_SYMS | HAS_LOCALS | DYNAMIC | WP_TEXT ),
	   /* section flags */
  (SEC_ALL_FLAGS & (~(SEC_IS_BIG_ENDIAN))),
  ' ',				/* ar_pad_char */
  16,				/* ar_max_namelen */
_do_getl64, _do_putl64,  _do_getl32, _do_putl32, _do_getl16, _do_putl16, /* data */
_do_getb64, _do_putb64,  _do_getb32, _do_putb32, _do_getb16, _do_putb16, /* hdrs */
    {_bfd_dummy_target, bout_big_object_p, /* bfd_check_format */
       bfd_generic_archive_p, _bfd_dummy_target},
    {bfd_false, bout_mkobject,	/* bfd_set_format */
       _bfd_generic_mkarchive, bfd_false},
    {bfd_false, bout_write_object_contents,	/* bfd_write_contents */
       _bfd_write_archive_contents, bfd_false},

  JUMP_TABLE(aout)
};


bfd_target b_out_vec_little_host =
{
  "b.out.little",		/* name */
  bfd_target_aout_flavour_enum,
  false,			/* data byte order is little */
  false,			/* header byte order is little */
  (HAS_RELOC | EXEC_P |		/* object flags */
   HAS_LINENO | HAS_DEBUG | HAS_CCINFO |
   HAS_SYMS | HAS_LOCALS | DYNAMIC | WP_TEXT ),
	   /* section flags */
  (SEC_ALL_FLAGS & (~(SEC_IS_BIG_ENDIAN))),
  ' ',				/* ar_pad_char */
  16,				/* ar_max_namelen */
_do_getl64, _do_putl64, _do_getl32, _do_putl32, _do_getl16, _do_putl16, /* data */
_do_getl64, _do_putl64, _do_getl32, _do_putl32, _do_getl16, _do_putl16, /* hdrs */

    {_bfd_dummy_target, bout_little_object_p, /* bfd_check_format */
       bfd_generic_archive_p, _bfd_dummy_target},
    {bfd_false, bout_mkobject,	/* bfd_set_format */
       _bfd_generic_mkarchive, bfd_false},
    {bfd_false, bout_write_object_contents,	/* bfd_write_contents */
       _bfd_write_archive_contents, bfd_false},
  JUMP_TABLE(aout)
};
