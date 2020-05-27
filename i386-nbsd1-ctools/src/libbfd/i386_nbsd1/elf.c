/*(c****************************************************************************** *
 * Copyright (c) 1994 Intel Corporation
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
 *****************************************************************************c)*/

/* This is the elf target code for bfd. */

#include "sysdep.h"
#include "elf.h"
#include "bfd.h"
#include "libbfd.h"

/* If a symbol is read from an elf file, it will have one of these throughout its
   lifetime.  Else, it will only have one of these during the brief period after
   the symbol is written to the output file. */

typedef struct {
    unsigned long st_size,st_info;
} native_elf_info;

/* The aliased asymbol.  See elf_make_empty_symbol below */
typedef struct {
    asymbol asym;
    native_elf_info native_info;
} elf_symbol_type;

#define fetch_native_elf_info(ASYM) ((native_elf_info *) ASYM->native_info)
#define put_native_elf_info(ASYM) (ASYM->native_info = (char *) &((elf_symbol_type *)ASYM)->native_info)

typedef struct elf_section_struct {
    asection asect;
    int sh_entsize;
    struct elf_section_struct *sh_link,*sh_info,*next_reloc_section;

    /*	For input sections:

	if section is a relocation section sh_link points to the symbol table section
	"    "     "  " relocation section sh_info  "     "   "  section to which it applies
	"    "     "  " symbol table       sh_link  "     "   "  string table   "
	"    "     "  " symbol table       sh_info  is NULL
	"    "     "  " section with relocs sh_link "     "   "  relocation section
	"    "     "  " section with relocs sh_info is NULL

	For output sections, only relocation sections will be elf_sections, everything else is an
	asection:

       if section is a relocation section sh_link points to the symbol table asection
       if section is a relocation section sh_info points to the asection to which the relocations apply
  */
} elf_section;

typedef union {
    struct {
	elf_symbol_type *symbols;                   /* For input bfd's only. */
	elf_section *sections,                      /* Ditto. */
	*symbol_table;				    /* Ditto. */
	int ccinfo_section_offset;                  /* Ditto. */
    } elfinputdata;

#define EDT_SYMBOLS               elfinputdata.symbols
#define EDT_SECTIONS              elfinputdata.sections
#define EDT_SYMBOL_TABLE          elfinputdata.symbol_table
#define EDT_CCINFO_SECTION_OFFSET elfinputdata.ccinfo_section_offset

    struct {
	elf_section *reloc_sections;		    /* For output bfd's only. */
	asection *shstrtab,*strtab,*symtab;         /* Ditto. */
	hashed_str_tab *hst_shstrtab,*hst_strtab;   /* Ditto. */
	int section_header_base,one_past_stb_local; /* Ditto. */
	int program_header_count;                   /* Ditto. */
	int section_header_count;                   /* Ditto. */
	int shstrtab_idx;                           /* Ditto. */
    } elfoutputdata;

#define EDT_RELOC_SECTIONS      elfoutputdata.reloc_sections
#define EDT_SHSTRTAB            elfoutputdata.shstrtab
#define EDT_STRTAB              elfoutputdata.strtab
#define EDT_SYMTAB              elfoutputdata.symtab
#define EDT_HST_SHSTRTAB        elfoutputdata.hst_shstrtab
#define EDT_HST_STRTAB          elfoutputdata.hst_strtab
#define EDT_SECTION_HEADER_BASE elfoutputdata.section_header_base
#define EDT_ONE_PAST_STB_LOCAL  elfoutputdata.one_past_stb_local
#define EDT_PROG_HDR_COUNT      elfoutputdata.program_header_count
#define EDT_SECT_HDR_COUNT      elfoutputdata.section_header_count
#define EDT_SHSTRTAB_IDX        elfoutputdata.shstrtab_idx

} elf_data_type;

/* In each bfd's tdata field we stuff an elf_data_type ptr.  Here, we provide a means
 to extract it by a cast: */

#define elf_data(bfd)		((elf_data_type *) ((bfd)->tdata))

/* Simple subscripting mechanism for input bfd's elf_sections.  */

#define Nth_section(abfd,n)  ((elf_section *) (elf_data(abfd)->EDT_SECTIONS + n))
#define Nth_asection(abfd,n) ((asection *) &(Nth_section(abfd,n)->asect))

/* Byte swapping stuff: */

#define GETIT(T,I,BIG,S) \
(BIG ? (\
	(S == 4) ? (\
		    _do_getb32((bfd_byte *)&I->T)\
		    )\
	: (\
	   (S == 2) ? (\
		       _do_getb16((bfd_byte *)&I->T)\
		       )\
	   : (I->T)\
	   )\
	) : (\
	     (S == 4) ? (\
			 _do_getl32((bfd_byte *)&I->T)\
			 )\
	     : (\
		(S == 2) ? (\
			    _do_getl16((bfd_byte *)&I->T)\
			    )\
		: (I->T)\
		)\
	     )\
)


#define SWAP(T,O,I,BIG)  O->T = GETIT(T,I,BIG,sizeof(I->T))

static void 
	DEFUN(elf_swap_proghdr_in,(abfd, ext, in, be),
	      bfd    *abfd    AND
	      Elf32_Phdr *ext AND
	      Elf32_Phdr *in  AND
	      int be)
{
#define sh(T) SWAP(T,in,ext,be)

    sh(p_type);
    sh(p_offset);
    sh(p_vaddr);
    sh(p_paddr);
    sh(p_filesz);
    sh(p_memsz);
    sh(p_flags);
    sh(p_align);
}

static void 
	DEFUN(elf_swap_scnhdr_in,(abfd, ext, in, be),
	      bfd    *abfd    AND
	      Elf32_Shdr *ext AND
	      Elf32_Shdr *in  AND
	      int be)
{
    sh(sh_name);
    sh(sh_type);
    sh(sh_flags);
    sh(sh_addr);
    sh(sh_offset);
    sh(sh_size);
    sh(sh_link);
    sh(sh_info);
    sh(sh_addralign);
    sh(sh_entsize);
}

static void 
	DEFUN(elf_swap_Elfhdr_in,(abfd, ext, in, be),
	      bfd    *abfd    AND
	      Elf32_Ehdr *ext AND
	      Elf32_Ehdr *in  AND
	      int be)
{
    sh(e_type);
    sh(e_machine);
    sh(e_version);
    sh(e_entry);
    sh(e_phoff);
    sh(e_shoff);
    sh(e_flags);
    sh(e_ehsize);
    sh(e_phentsize);
    sh(e_phnum);
    sh(e_shentsize);
    sh(e_shnum);
    sh(e_shstrndx);
}

static void 
	DEFUN(elf_swap_Elf32_Sym_in,(abfd, ext, in, be),
	      bfd    *abfd   AND
	      Elf32_Sym *ext AND
	      Elf32_Sym *in  AND
	      int be)
{
    sh(st_name);
    sh(st_value);
    sh(st_size);
    sh(st_info);
    sh(st_other);
    sh(st_shndx);
}

static void 
	DEFUN(elf_swap_Elf32_Rel_in,(abfd, ext, in, be),
	      bfd    *abfd   AND
	      Elf32_Rel *ext AND
	      Elf32_Rel *in  AND
	      int be)
{
    sh(r_offset);
    sh(r_info);
}

static char *read_in_string_table();

/* Read in the raw Elf32_Sym's from the input abfd. */

static Elf32_Sym *read_raw_syms(abfd,nsyms,str_tab)
    bfd *abfd;
    int *nsyms;
    char **str_tab;
{
    elf_section *sts = elf_data(abfd)->EDT_SYMBOL_TABLE;
    asection *sta = &sts->asect;
    Elf32_Sym *raw_read_syms;
    Elf32_Sym *raw_syms;

    if (!sts)
	    return (Elf32_Sym *) 0;

    *nsyms = 0;
    *str_tab = read_in_string_table(abfd,sts->sh_link);
    if (!(*str_tab) || bfd_seek(abfd,sta->filepos,SEEK_SET) < 0) {
	return (Elf32_Sym *) 0;
    }

    raw_read_syms = (Elf32_Sym *) bfd_alloc(abfd,sta->size);

    if (bfd_read(raw_read_syms,1,sta->size,abfd) != sta->size) {
	bfd_release(abfd,*str_tab);
	bfd_release(abfd,raw_read_syms);
	return (Elf32_Sym *) 0;
    }
    else {
	/* Swap all of the symbol table entries: */
	char *q,*end;
	Elf32_Sym *rsyms;
	*nsyms = sta->size / sts->sh_entsize;
	rsyms = raw_syms = (Elf32_Sym *) bfd_alloc(abfd,(*nsyms) * sizeof(Elf32_Sym));

	for (q=(char *)raw_read_syms,end=q+sta->size;q < end;rsyms++,q += sts->sh_entsize) {
	    Elf32_Sym *p = (Elf32_Sym *) q;
	    elf_swap_Elf32_Sym_in(abfd,p,rsyms,sta->flags & SEC_IS_BIG_ENDIAN);
	}
	bfd_release(abfd,raw_read_syms);
	return raw_syms;
    }
}

static boolean is_debug_section_name(name,beginner)
    char *name;
    int beginner;
{
    static char *debug_sections[] = { ".debug", ".debug_abbrev", ".debug_aranges", ".debug_frame",
					      ".debug_info", ".debug_line", ".debug_loc",
					      ".debug_macinfo", ".debug_pubnames", NULL };
    char **p = debug_sections;

    if (beginner) {
	if (!strncmp(name,"__B",3))
		name += 3;
	else
		return false;
    }
	    
    for (;*p;p++)
	    if (!strcmp(*p,name))
		    return true;

    return false;
}

static boolean
	DEFUN(elf_slurp_symbol_table,(abfd),
	      bfd *abfd)
{
    if (elf_data(abfd)->EDT_SYMBOLS)
	    return true;
    else if (abfd->flags & HAS_SYMS) {
	char *str_tab;
	Elf32_Sym *raw_syms = read_raw_syms(abfd,&abfd->symcount,&str_tab);

	if (!str_tab || !abfd->symcount || !raw_syms)
		return false;
	else {
	    Elf32_Sym *rsym = raw_syms;
	    elf_data_type *edt = elf_data(abfd);
	    elf_symbol_type *est,*previous_symbol;
	    int i;

	    est = edt->EDT_SYMBOLS = (elf_symbol_type *)
		    bfd_zalloc(abfd,abfd->symcount * sizeof(elf_symbol_type));
	    for (i=0;i < abfd->symcount;i++,rsym++,est++) {
		asymbol *as = &(est->asym);

		put_native_elf_info(as);
		fetch_native_elf_info(as)->st_size = rsym->st_size;
		fetch_native_elf_info(as)->st_info = rsym->st_info;
		as->the_bfd = abfd;
		as->flags = (i == 0) ? BSF_ZERO_SYM: 0;
		switch (ELF32_ST_TYPE(rsym->st_info)) {
	    case STT_SECTION:
		    as->flags |= BSF_SECT_SYM;
		    as->name = Nth_asection(abfd,rsym->st_shndx)->name;
		    if ((Nth_asection(abfd,rsym->st_shndx)->flags & SEC_ALLOC) &&
			(!strcmp(".text",as->name) ||
			 !strcmp(".data",as->name) ||
			 !strcmp(".bss",as->name)))
			    as->flags |= BSF_ALL_OMF_REP;
		    break;
	    case STT_FILE:
		    as->flags |= BSF_DEBUGGING;
	    default:
		    as->name = str_tab + rsym->st_name;
		}
		as->value = rsym->st_value;
		switch (ELF32_ST_BIND(rsym->st_info)) {
	    case STB_LOCAL:
		    if (ELF32_ST_TYPE(rsym->st_info) != STT_FILE &&
			ELF32_ST_TYPE(rsym->st_info) != STT_SECTION &&
			i != 0)
			    as->flags |= BSF_ALL_OMF_REP;
		    if (i == 0 || rsym->st_shndx != SHN_UNDEF)
			    as->flags |= BSF_LOCAL | BSF_XABLE;
		    break;
	    case STB_GLOBAL:
		    if (rsym->st_shndx != SHN_UNDEF)
			    as->flags |= BSF_GLOBAL | BSF_EXPORT | BSF_ALL_OMF_REP;
		    break;
	    case STB_WEAK:
		    as->flags |= BSF_WEAK;
		    break;
	    default:
		    return false;
		}
		switch (rsym->st_shndx) {
	    case SHN_UNDEF:
		    if (i != 0) {
			if (ELF32_ST_TYPE(rsym->st_info) == STT_FILE ||
			    i == 0)
				as->flags |= BSF_DEBUGGING;
			else
				as->flags |= BSF_UNDEFINED | BSF_ALL_OMF_REP;
		    }
		    as->section = 0;
		    break;
	    case SHN_ABS:
		    as->value = rsym->st_value;
		    as->flags |= BSF_ABSOLUTE;
		    if (ELF32_ST_TYPE(rsym->st_info) != STT_FILE)
			    as->flags |= BSF_ALL_OMF_REP;
		    as->section = 0;
		    break;
	    case SHN_COMMON:
		    as->flags |= BSF_FORT_COMM | BSF_ALL_OMF_REP;
		    as->value = rsym->st_size;
		    as->value_2 = rsym->st_value;
		    as->section = 0;
		    break;
	    default:
		    as->section = Nth_asection(abfd,rsym->st_shndx);
		    /* Executable files in elf have symbols with section vma added in.
		       We subtract it here to use bfd-section relative values. */
		    if (abfd->flags & EXEC_P)
			    as->value -= as->section->vma;
		}
		if (rsym->st_other & STO_960_HAS_LEAFPROC) {
		    as->flags |= BSF_HAS_BAL;
		}
		if (rsym->st_other & STO_960_HAS_SYSPROC) {
		    as->flags |= BSF_HAS_SCALL;
		}
		if (rsym->st_other & STO_960_IS_LEAFPROC) {
		    as->flags |= BSF_BAL;
		    if (previous_symbol->asym.flags & BSF_HAS_BAL) {
			as->flags |= BSF_HAS_CALL;
			as->flags &= ~(BSF_GLOBAL|BSF_EXPORT);
			as->flags |= BSF_LOCAL;
			as->related_symbol = &previous_symbol->asym;
			previous_symbol->asym.related_symbol = as;
		    }
		}
		if (rsym->st_other & STO_960_IS_SYSPROC) {
		    as->flags |= BSF_SCALL;
		    if (as->value != -1)
			    as->flags &= ~BSF_UNDEFINED;  /* Knock out an undefined bit if there is one. */

		    if (previous_symbol->asym.flags & BSF_HAS_SCALL) {
			if (as->value != -1) {
			    if (previous_symbol->asym.flags & BSF_UNDEFINED) { /* Knock out an undefined */
				previous_symbol->asym.flags &= ~BSF_UNDEFINED;  /* bit if there is one. */
				previous_symbol->asym.flags |= (ELF32_ST_BIND(rsym->st_info) == STB_GLOBAL) ?
					BSF_GLOBAL : BSF_LOCAL;
				as->flags &= ~(BSF_GLOBAL|BSF_EXPORT);
			    }
			}
			as->flags &= ~(BSF_GLOBAL|BSF_EXPORT);
			as->flags |= BSF_HAS_CALL | BSF_LOCAL;
			as->related_symbol = &previous_symbol->asym;
			previous_symbol->asym.related_symbol = as;
		    }
		}

		as->udata = 0;
		previous_symbol = est;
	    }
	    bfd_release(abfd,raw_syms);
	    return true;
	}
    }
    else
	    return false;
}

#define SECTION_FLAG_MAP( APPLY )                                    \
	APPLY( (SEC_ALLOC | SEC_LOAD)  , SHF_ALLOC                   );\
	APPLY( SEC_CODE                , (SHF_EXECINSTR | SHF_ALLOC) );\
	APPLY( SEC_DATA                , (SHF_WRITE | SHF_ALLOC)     );\
	APPLY( SEC_IS_READABLE         , SHF_960_READ                );\
	APPLY( SEC_IS_WRITEABLE        , SHF_WRITE                   );\
	APPLY( SEC_IS_EXECUTABLE       , SHF_EXECINSTR               );\
	APPLY( SEC_IS_SUPER_READABLE   , SHF_960_SUPER_READ          );\
	APPLY( SEC_IS_SUPER_WRITEABLE  , SHF_960_SUPER_WRITE         );\
	APPLY( SEC_IS_SUPER_EXECUTABLE , SHF_960_SUPER_EXECINSTR     );\
	APPLY( SEC_IS_BIG_ENDIAN       , SHF_960_MSB                 );\
	APPLY( SEC_IS_LINK_PIX         , SHF_960_LINK_PIX            );\
	APPLY( SEC_IS_PI               , SHF_960_PI                  );

static boolean
	DEFUN(make_a_section_from_file,(abfd, hdr, as, string_table, secnum),
	      bfd            *abfd AND
	      Elf32_Shdr  *hdr     AND
	      asection *as         AND
	      char *string_table   AND
	      int secnum)
{
    asection *sec;
    unsigned int i;
    elf_data_type *edt = elf_data(abfd);
    char *name = string_table + hdr->sh_name;

    as->secnum = secnum;
    as->name = name;
    as->flags = 0;
#define set_sec_flags(BFDISM,ELFISM) if ((hdr->sh_flags & ELFISM) == ELFISM) as->flags |= BFDISM
    SECTION_FLAG_MAP( set_sec_flags );
#undef set_sec_flags

    if (hdr->sh_type == SHT_NOBITS &&
	((hdr->sh_flags & (SHF_WRITE | SHF_ALLOC)) == (SHF_WRITE | SHF_ALLOC))) {
	as->flags |= SEC_IS_BSS;
	as->flags &= ~(SEC_LOAD | SEC_DATA);  /* Unset these two bits. */
    }
    else if (hdr->sh_type == SHT_PROGBITS)
	    as->flags |= SEC_HAS_CONTENTS;
    
    if (is_debug_section_name(as->name,0))
	    as->flags |= SEC_IS_DEBUG;

    if (hdr->sh_flags & SHF_960_LINK_PIX)
	    abfd->flags |= CAN_LINKPID;

    as->size	 = hdr->sh_size;
    as->filepos  = hdr->sh_offset;
    as->vma	 = hdr->sh_addr;
    as->pma	 = hdr->sh_addr;

    for (i = 0; i < 32; i++) {
	if ( (int)(1<<i) >= hdr->sh_addralign) {
	    as->alignment_power = i;
	    break;
	}
    }
    return true;
}

static char * read_string_table(abfd,offset,size)
    bfd *abfd;
    int offset,size;
{
    if (bfd_seek(abfd,offset,SEEK_SET) < 0)
	    return NULL;
    else {
	char *p = bfd_alloc(abfd,size);
	if (bfd_read(p,1,size,abfd) == size)
		return p;
	else {
	    bfd_release(abfd,p);
	    return NULL;
	}
    }
}

#define BFD_ELF_ARCH_TABLE( APPLY )  \
	APPLY(EF_960_SA,BFD_960_SA); \
	APPLY(EF_960_SB,BFD_960_SB); \
	APPLY(EF_960_KA,BFD_960_KA); \
	APPLY(EF_960_KB,BFD_960_KB); \
	APPLY(EF_960_CA,BFD_960_CA); \
	APPLY(EF_960_CF,BFD_960_CF); \
	APPLY(EF_960_JA,BFD_960_JA); \
	APPLY(EF_960_JD,BFD_960_JD); \
	APPLY(EF_960_JF,BFD_960_JF); \
	APPLY(EF_960_HA,BFD_960_HA); \
	APPLY(EF_960_HD,BFD_960_HD); \
	APPLY(EF_960_HT,BFD_960_HT); \
	APPLY(EF_960_RP,BFD_960_RP); \
	APPLY(EF_960_JL,BFD_960_JL); \
	APPLY(EF_960_GENERIC,BFD_960_GENERIC)

#define BFD_ELF_ATTR_TABLE( APPLY )                                         \
	APPLY( EF_960_H_SERIES, BFD_960_MACH_HX,     bfd_mach_i960_hx);     \
	APPLY( EF_960_J_SERIES, BFD_960_MACH_JX,     bfd_mach_i960_jx);     \
	APPLY( EF_960_CORE2,    BFD_960_MACH_CORE_2, bfd_mach_i960_core2);  \
	APPLY( EF_960_C_SERIES, BFD_960_MACH_CX,     bfd_mach_i960_ca);     \
	APPLY( EF_960_FP1,      BFD_960_MACH_FP1,    bfd_mach_i960_kb_sb);  \
	APPLY( EF_960_K_SERIES, BFD_960_MACH_KX,     bfd_mach_i960_ka_sa);  \
	APPLY( EF_960_CORE1,    BFD_960_MACH_CORE_1, bfd_mach_i960_core)    \


static int compare_prog_hdrs(l,r)
    Elf32_Phdr *l,*r;
{
#define COMPARE(ELEMENT) if (l->ELEMENT < r->ELEMENT) return -1; else if (l->ELEMENT > r->ELEMENT) return 1

    COMPARE(p_vaddr);
    COMPARE(p_paddr);
    COMPARE(p_type);
    COMPARE(p_offset);
    COMPARE(p_filesz);
    COMPARE(p_memsz);
    COMPARE(p_flags);
    COMPARE(p_align);

#undef COMPARE

    return 0;
}

static Elf32_Phdr *lookup_prog_header(as,phdrs,nphdrs,phdrs_offset,file_offset)
    asection *as;
    Elf32_Phdr *phdrs;
    int nphdrs,phdrs_offset,*file_offset;
{
    int i;

    for (i=0;i < nphdrs;i++) {
	if (as->vma > phdrs[i].p_vaddr)
		continue;
	else if (as->vma < phdrs[i].p_vaddr)
		return (Elf32_Phdr *) 0;
	else {
	    if (as->filepos     == phdrs[i].p_offset &&
		phdrs[i].p_type == PT_LOAD) {
		phdrs[i].p_type = PT_NULL;
		*file_offset = phdrs_offset + (i * sizeof(Elf32_Phdr)) +
			(((int) &phdrs->p_paddr) - ((int) phdrs));
		return phdrs+i;
	    }
	}
    }
}

#define SECTION_IS_LOADED(SECT) ((SECT->flags & SEC_ALLOC) && \
				 ((SECT->flags & (SEC_IS_DSECT | SEC_IS_NOLOAD)) == 0))

static void set_paddr_file_offset(as,raw_prog_hdrs,n_prog_hdrs,prog_hdrs_offset)
    asection *as;
    Elf32_Phdr *raw_prog_hdrs;
    int n_prog_hdrs,prog_hdrs_offset;
{
    if (raw_prog_hdrs) {
	if (SECTION_IS_LOADED(as)) {
	    Elf32_Phdr *p;
	    int file_offset;

	    if (p = lookup_prog_header(as,raw_prog_hdrs,n_prog_hdrs,prog_hdrs_offset,&file_offset)) {
		as->paddr_file_offset = file_offset;
		as->pma = p->p_paddr;
		return;
	    }
	}
    }
    as->paddr_file_offset = BAD_PADDR_FILE_OFFSET;
}
    
	
static char *
read_in_string_table(abfd,sec)
    bfd *abfd;
    elf_section *sec;
{
    return read_string_table(abfd,sec->asect.filepos,sec->asect.size);
}

int elf_seek_ccinfo(abfd)
    bfd *abfd;
{
    if (bfd_seek(abfd,elf_data(abfd)->EDT_CCINFO_SECTION_OFFSET,SEEK_SET) < 0)
	    return 0;
    return 1;
}

static boolean
DEFUN(elf_mkobject,(abfd),
    bfd *abfd)
{
    set_tdata (abfd, bfd_zalloc (abfd,sizeof(elf_data_type)));
    if (elf_data(abfd) == 0) {
	bfd_error = no_memory;
	return false;
    }
    return true;
}

static bfd_target *
	DEFUN(elf_object_p,(abfd),
    bfd *abfd)
{
    int   nscns,readsize;
    Elf32_Ehdr filehdr;
    Elf32_Shdr *raw_sect_hdrs;
    Elf32_Phdr *raw_prog_hdrs;
    elf_data_type *edata;
    char *section_header_string_table;

    bfd_error = system_call_error;

    if (bfd_seek(abfd,0,SEEK_SET) < 0)
	    return 0;

    /* figure out how much to read */

#define Elf32_Ehdr_SZ (sizeof(Elf32_Ehdr))

    if (bfd_read((PTR) &filehdr, 1, Elf32_Ehdr_SZ, abfd) != Elf32_Ehdr_SZ)
	    return 0;

    if (filehdr.e_ident[EI_MAG0] != ELFMAG0 ||
	filehdr.e_ident[EI_MAG1] != ELFMAG1 ||
	filehdr.e_ident[EI_MAG2] != ELFMAG2 ||
	filehdr.e_ident[EI_MAG3] != ELFMAG3)
	    return 0;

    switch (filehdr.e_ident[EI_DATA]) {
 case ELFDATA2LSB:
	if (abfd->xvec->header_byteorder_big_p)
		return 0;
	break;
 case ELFDATA2MSB:
	if (!abfd->xvec->header_byteorder_big_p)
		return 0;
	break;
 default:
	return 0;
    }
	
    elf_swap_Elfhdr_in(abfd,&filehdr,&filehdr,abfd->xvec->header_byteorder_big_p);

    /* FIXME:  Got to remove the '0 &&' after pete stamps EM_960 into gas output files
       or at some time in the future when we do not care to dump sys v r 4 elf files. */

    if (0 && filehdr.e_machine != EM_960)
	    return 0;

    nscns = filehdr.e_shnum;

    abfd->obj_machine = abfd->obj_machine_2 = 0;

    switch (filehdr.e_flags & EF_960_MASKPROC) {

#define SET_ELF_ARCH(X,Y) case (X): bfd_set_target_arch(abfd,(Y)); break

	BFD_ELF_ARCH_TABLE( SET_ELF_ARCH );
 default:
	fprintf(stderr,"warning: file: %s contains architecture: 0x%x\n",abfd->filename,
		filehdr.e_flags & EF_960_MASKPROC);
	bfd_set_target_arch(abfd,BFD_960_GENERIC);
    }

#define set_bfd_attrs(ELFISM,BFDISM,BFD2ISM)     \
    if (filehdr.e_flags & ELFISM) {              \
       bfd_set_target_attributes(abfd, BFDISM ); \
       if (abfd->obj_machine == 0)               \
	       abfd->obj_machine = BFD2ISM;      \
    }
    BFD_ELF_ATTR_TABLE( set_bfd_attrs );

#undef set_bfd_attrs

    if (((filehdr.e_flags & EF_960_MASKPROC) == EF_960_GENERIC) ||
	(!abfd->obj_machine))
	    abfd->obj_machine = bfd_mach_i960_core;

    if (!elf_mkobject(abfd))
	    return 0;

    abfd->obj_arch = bfd_arch_i960;
    edata = elf_data(abfd);
    abfd->start_address = filehdr.e_entry;

    if (filehdr.e_type == ET_EXEC)
	    abfd->flags |= EXEC_P;

    /* FIXME Add code to free stuff malloc'd on failure.  */
    if (filehdr.e_phnum) {
	raw_prog_hdrs = (Elf32_Phdr *) bfd_alloc(abfd,readsize = filehdr.e_phnum * filehdr.e_phentsize);
	if (bfd_seek(abfd,filehdr.e_phoff,SEEK_SET) < 0) {
	}
	if (bfd_read((PTR)raw_prog_hdrs, 1, readsize, abfd) != readsize) {
	}
	if (1) {
	    int i;
	    unsigned char *p = (unsigned char *) raw_prog_hdrs;

	    for (i=0;i < filehdr.e_phnum;i++,p += filehdr.e_phentsize) {
		Elf32_Phdr *tmp = (Elf32_Phdr *) p;
		elf_swap_proghdr_in(abfd, p, p, abfd->xvec->header_byteorder_big_p);
		tmp->p_type = PT_LOAD;
	    }
	}
    }
    else
	    raw_prog_hdrs = 0;
	

    if (nscns != 0) {
	int i;
	char *q;
	asection **sect_list = &abfd->sections;

	raw_sect_hdrs = (Elf32_Shdr *) bfd_alloc(abfd,readsize = nscns * filehdr.e_shentsize);
	edata->EDT_SECTIONS = (elf_section *) bfd_zalloc(abfd,nscns * sizeof(elf_section));
	if (bfd_seek(abfd,filehdr.e_shoff,SEEK_SET) < 0) {
	    bfd_release(abfd,edata);
	    bfd_release(abfd,raw_sect_hdrs);
	    bfd_release(abfd,edata->EDT_SECTIONS);
	    return (bfd_target *)NULL;
	}

	if (bfd_read((PTR)raw_sect_hdrs, 1, readsize, abfd) != readsize) {
	    bfd_release(abfd,edata);
	    bfd_release(abfd,raw_sect_hdrs);
	    bfd_release(abfd,edata->EDT_SECTIONS);
	    return (bfd_target *)NULL;
	}

	/* First, read in the string table for the section headers: */
	if (filehdr.e_shstrndx) {
	    unsigned char *q = ((unsigned char *) raw_sect_hdrs) +
		    (filehdr.e_shentsize * filehdr.e_shstrndx);
	    Elf32_Shdr *p = (Elf32_Shdr *)q;
	    
	    elf_swap_scnhdr_in(abfd, p, p, abfd->xvec->header_byteorder_big_p);
	    if (!(section_header_string_table =
		  read_string_table(abfd,p->sh_offset,p->sh_size))) {
		bfd_release(abfd, edata);
		bfd_release(abfd,raw_sect_hdrs);
		bfd_release(abfd,edata->EDT_SECTIONS);
		return (bfd_target *)NULL;
	    }
	}

	/* Next, flip all of the sections making the all host endian and populate the
	   bfd with the section information.. */

	for (i=0,q=(char *)raw_sect_hdrs;i < nscns;i++,q += filehdr.e_shentsize) {
	    Elf32_Shdr *p = (Elf32_Shdr *) q;

	    if (i != filehdr.e_shstrndx)
		    elf_swap_scnhdr_in(abfd, p, p, abfd->xvec->header_byteorder_big_p);

	    if (!edata->EDT_SECTIONS[i].sh_link && p->sh_link)
		    edata->EDT_SECTIONS[i].sh_link = Nth_section(abfd,p->sh_link);
	    if (!edata->EDT_SECTIONS[i].sh_info && p->sh_info)
		    edata->EDT_SECTIONS[i].sh_info = Nth_section(abfd,p->sh_info);
	    switch (p->sh_type) {
	case SHT_SYMTAB :
		abfd->flags |= HAS_SYMS;
		edata->EDT_SYMBOL_TABLE = Nth_section(abfd,i);
		break;
	case SHT_REL :
		abfd->flags |= HAS_RELOC;
		Nth_section(abfd,p->sh_info)->sh_link = Nth_section(abfd,i);
		Nth_asection(abfd,p->sh_info)->reloc_count = p->sh_size / p->sh_entsize;
		break;
	case SHT_960_INTEL_CCINFO:
		edata->EDT_CCINFO_SECTION_OFFSET = p->sh_offset;
		abfd->flags |= HAS_CCINFO;
		break;
	case SHT_NULL:
	case SHT_STRTAB:
		break;
	case SHT_PROGBITS:
	case SHT_NOTE:
	case SHT_NOBITS:
		abfd->section_count++;
		(*sect_list) = &edata->EDT_SECTIONS[i].asect;
		sect_list = &edata->EDT_SECTIONS[i].asect.next;
		break;
	default:
		fprintf(stderr,"Ignoring section with unknown section header type: 0x%x, from input file: %s\n",
			p->sh_type,abfd->filename);
		break;
	    }
	    make_a_section_from_file(abfd,p,&edata->EDT_SECTIONS[i].asect,
				     section_header_string_table,i);
	    if ((edata->EDT_SECTIONS[i].asect.flags & (SEC_DATA | SEC_IS_PI)) ==
		(SEC_DATA | SEC_IS_PI))
		    abfd->flags |= HAS_PID;
	    if ((edata->EDT_SECTIONS[i].asect.flags & (SEC_CODE | SEC_IS_PI)) ==
		(SEC_CODE | SEC_IS_PI))
		    abfd->flags |= HAS_PIC;
	    edata->EDT_SECTIONS[i].sh_info = Nth_section(abfd,p->sh_info);
	    edata->EDT_SECTIONS[i].sh_entsize = p->sh_entsize;
	    set_paddr_file_offset(&edata->EDT_SECTIONS[i].asect,
				  raw_prog_hdrs,
				  filehdr.e_phnum,
				  filehdr.e_phoff);
	}
	*sect_list = (asection *) 0;
	bfd_release(abfd,raw_sect_hdrs);
    }

    if (raw_prog_hdrs)
	    bfd_release(abfd,raw_prog_hdrs);
    return abfd->xvec;
}


/* Byte swapping stuff for writing symbols and the like. */

static void PUT_IT_THERE_REAL(it,there,big,size)
    unsigned long it;
    char *there;
    int big,size;
    
{
    if (big)
	    switch (size) {
	case 4:
		_do_putb32(it,there);
		break;
	case 2:
		_do_putb16(it,there);
		break;
	case 1:
		*(there) = it;
	    }
    else
	    switch (size) {
	case 4:
		_do_putl32(it,there);
		break;
	case 2:
		_do_putl16(it,there);
		break;
	case 1:
		*(there) = it;
	    }
}

#define PUT_IT_THERE(ABFD,IT,THERE) PUT_IT_THERE_REAL(IT,&(THERE),BFD_BIG_ENDIAN_FILE_P(ABFD),sizeof(THERE))

/* Returns -1 on error.  Else the number of symbols written: 0, 1 or 2.
 Retains the sacred ordering scheme of call entry points, leafs and calls ep's. */

static int
write_elf_symbol(abfd,as,idx,isleaf,issysproc)
    bfd *abfd;
    asymbol *as;
    int idx,isleaf,issysproc;
{
    elf_data_type *edt = elf_data(abfd);
    Elf32_Sym elf_sym;
    unsigned w;
    int rv = -1;

    memset(&elf_sym,0,sizeof(elf_sym));
    if (isleaf == 0 && issysproc == 0 &&
	((as->flags & BSF_ALREADY_EMT) ||
	 (as->flags & BSF_HAS_CALL)))
	    return 0;

    if (as->name)
	    w = _bfd_lookup_hashed_str_tab_offset(abfd,edt->EDT_HST_STRTAB,as->name,strlen(as->name));
    else
	    w = 0;
    PUT_IT_THERE(abfd,w,elf_sym.st_name);

    if (1) {
	int value;

	if (as->flags & BSF_FORT_COMM)
		value = as->value_2;
	else if (!issysproc) {
	    value = as->value + (as->section ? as->section->output_offset : 0);
	    /* Executable files in elf will have symbols which have their virtual addresses
	       added in. */
	    if (abfd->flags & EXEC_P)
		    value += (as->section && as->section->output_section) ?
			    as->section->output_section->vma :
				    0;
	}
	else
		value = as->value;

	PUT_IT_THERE(abfd,value,elf_sym.st_value);
    }

    if (fetch_native_elf_info(as)) {
	PUT_IT_THERE(abfd,fetch_native_elf_info(as)->st_size,elf_sym.st_size);
	PUT_IT_THERE(abfd,fetch_native_elf_info(as)->st_info,elf_sym.st_info);
	as->sym_tab_idx = idx;
    }
    else {
	int binding = 0;
	int flags = ((as->flags & BSF_HAS_CALL) && as->related_symbol) ?
		as->related_symbol->flags :
			as->flags;

	PUT_IT_THERE(abfd,0,elf_sym.st_size);
	if (flags & (BSF_GLOBAL | BSF_FORT_COMM | BSF_UNDEFINED))
		binding |= STB_GLOBAL;
	else if (flags & BSF_WEAK)
		binding |= STB_WEAK;
	elf_sym.st_info = ELF32_ST_INFO(binding,0);
	as->sym_tab_idx = idx;
	/*
	  put_native_elf_info(as);
	  fetch_native_elf_info(as)->st_info = elf_sym.st_info;
	  */
    }
    as->flags |= BSF_ALREADY_EMT;
    if (as->flags & BSF_FORT_COMM)
	    PUT_IT_THERE(abfd,as->value,elf_sym.st_size);
    if (isleaf) {
	PUT_IT_THERE(abfd,STO_960_IS_LEAFPROC,elf_sym.st_other);
    }
    else if (issysproc) {
	PUT_IT_THERE(abfd,STO_960_IS_SYSPROC,elf_sym.st_other);
	PUT_IT_THERE(abfd,SHN_ABS,elf_sym.st_shndx);
    }
    if (as->section)
	    PUT_IT_THERE(abfd,as->section->output_section->secnum+1,elf_sym.st_shndx);
    else if (as->flags & BSF_FORT_COMM)
	    PUT_IT_THERE(abfd,SHN_COMMON,elf_sym.st_shndx);
    else if ((as->flags & BSF_ABSOLUTE) ||
	     (fetch_native_elf_info(as) &&
	      ELF32_ST_TYPE(fetch_native_elf_info(as)->st_info) == STT_FILE))
	    PUT_IT_THERE(abfd,SHN_ABS,elf_sym.st_shndx);
    else
	    PUT_IT_THERE(abfd,SHN_UNDEF,elf_sym.st_shndx);

    if (as->flags & BSF_HAS_BAL)
	    PUT_IT_THERE(abfd,STO_960_HAS_LEAFPROC,elf_sym.st_other);
    if (as->flags & BSF_HAS_SCALL)
	    PUT_IT_THERE(abfd,STO_960_HAS_SYSPROC,elf_sym.st_other);

    if (bfd_write(&elf_sym,1,sizeof(Elf32_Sym),abfd) == sizeof(Elf32_Sym)) {
	rv = 1;
	if (as->flags & BSF_HAS_BAL) {
	    asymbol *asym = as->related_symbol;
	    int rvl = write_elf_symbol(abfd,asym,idx+rv,1,0);

	    if (rvl == -1)
		    return -1;
	    rv += rvl;
	}
	if (as->flags & BSF_HAS_SCALL) {
	    asymbol *asym = as->related_symbol;
	    int rvl = write_elf_symbol(abfd,asym,idx+rv,0,1);

	    if (rvl == -1)
		    return -1;
	    rv += rvl;
	}
    }
    return rv;
}

static boolean
write_elf_header(abfd,phdr_cnt)
    bfd *abfd;
    int phdr_cnt;
{
    Elf32_Ehdr elf_header;
    elf_data_type *edt = elf_data(abfd);

    memset(elf_header.e_ident,0,EI_NIDENT);

    elf_header.e_ident[EI_MAG0] = ELFMAG0;
    elf_header.e_ident[EI_MAG1] = ELFMAG1;
    elf_header.e_ident[EI_MAG2] = ELFMAG2;
    elf_header.e_ident[EI_MAG3] = ELFMAG3;
    elf_header.e_ident[EI_CLASS] = ELFCLASS32;
    elf_header.e_ident[EI_DATA] = abfd->xvec->header_byteorder_big_p ? ELFDATA2MSB : ELFDATA2LSB;
    elf_header.e_ident[EI_VERSION] = EV_CURRENT;

    PUT_IT_THERE(abfd,EM_960,elf_header.e_machine);
    PUT_IT_THERE(abfd,EV_CURRENT,elf_header.e_version);
    PUT_IT_THERE(abfd,abfd->start_address,elf_header.e_entry);

    if (phdr_cnt == 0) {
	PUT_IT_THERE(abfd,0,elf_header.e_phoff);
	PUT_IT_THERE(abfd,0,elf_header.e_phnum);
    }
    else {
	PUT_IT_THERE(abfd,sizeof(Elf32_Ehdr),elf_header.e_phoff);
	PUT_IT_THERE(abfd,phdr_cnt,elf_header.e_phnum);
    }

    if (abfd->flags & EXEC_P)
	    PUT_IT_THERE(abfd,ET_EXEC,elf_header.e_type);
    else
	    PUT_IT_THERE(abfd,ET_REL,elf_header.e_type);

    PUT_IT_THERE(abfd,edt->EDT_SECTION_HEADER_BASE,elf_header.e_shoff);

    if (1) {
	unsigned long eflags = 0;
	unsigned long attrs = bfd_get_target_attributes(abfd);

	switch (bfd_get_target_arch(abfd)) {

#define SET_EFLAGS_ARCH(Y,X) case (X): eflags = Y; break
	    BFD_ELF_ARCH_TABLE( SET_EFLAGS_ARCH );
    default:
	    fprintf(stderr,"warning: %s is being marked with generic architecture.\n",abfd->filename);
	    eflags = EF_960_GENERIC;
	    attrs |= EF_960_CORE1;
	}

#define set_elf_attrs(ELFISM, BFDISM, IGNORED) if (attrs & BFDISM) eflags |= ELFISM

	BFD_ELF_ATTR_TABLE( set_elf_attrs );

#undef set_elf_attrs

	PUT_IT_THERE(abfd,eflags,elf_header.e_flags);
    }
    PUT_IT_THERE(abfd,sizeof(Elf32_Ehdr),elf_header.e_ehsize);
    PUT_IT_THERE(abfd,sizeof(Elf32_Phdr),elf_header.e_phentsize);
    PUT_IT_THERE(abfd,sizeof(Elf32_Shdr),elf_header.e_shentsize);
    PUT_IT_THERE(abfd,edt->EDT_SECT_HDR_COUNT,elf_header.e_shnum);
    PUT_IT_THERE(abfd,edt->EDT_SHSTRTAB_IDX,elf_header.e_shstrndx);

    return bfd_write(&elf_header,1,sizeof(Elf32_Ehdr),abfd) == sizeof(Elf32_Ehdr);
}

static int reloc_type_to_type_map(reloc_type)
    bfd_reloc_type reloc_type;
{
    switch(reloc_type) {
 case bfd_reloc_type_none:
	return R_960_NONE;

 case bfd_reloc_type_opt_call:
	return R_960_OPTCALL;

 case bfd_reloc_type_opt_callx:
	return R_960_OPTCALLX;

 case bfd_reloc_type_32bit_abs:
	return R_960_32;

 case bfd_reloc_type_12bit_abs:
	return R_960_12;

 case bfd_reloc_type_24bit_pcrel:
	return R_960_IP24;

 case bfd_reloc_type_32bit_abs_sub:
	return R_960_SUB;

 case bfd_reloc_type_unknown:
 default:
	abort();
    }

}

static boolean
DEFUN(elf_write_elf32_rel,(abfd,rel),
    bfd *abfd AND
      arelent *rel)
{
    Elf32_Rel erel;
    asymbol *p = *(rel->sym_ptr_ptr);
    int idx = p->sym_tab_idx;

    if (idx == 0)  {
	if (p->section && p->section->output_section)
		/* This is the case of an orphaned section symbol used
		   as a relocation directive.  We point the relocation at the
		   new section symbol generated in set_sym_tab below. */
		idx = p->section->output_section->secnum + 1;
	else {
	    fprintf(stderr,"Found a symbol that resides in no section, and is used in a relocation\n");
	    fprintf(stderr,"directive named: %s, dir_type: %s, address: 0x%x \n",p->name,rel->howto->name,
		    rel->address);
	    exit(1);
	}
    }
    PUT_IT_THERE(abfd,rel->address,erel.r_offset);
    PUT_IT_THERE(abfd,ELF32_R_INFO(idx,reloc_type_to_type_map(rel->howto->reloc_type)),erel.r_info);

    return bfd_write(&erel,1,sizeof(Elf32_Rel),abfd) == sizeof(Elf32_Rel);
}

static          boolean
DEFUN(write_elf_rel_section,(abfd,es),
      bfd *abfd AND
      elf_section *es)
{
    elf_data_type *edt = elf_data(abfd);
    asection *as = &es->asect;
    Elf32_Shdr rsh;
    unsigned w = _bfd_lookup_hashed_str_tab_offset(abfd,edt->EDT_HST_SHSTRTAB,as->name,strlen(as->name));

    PUT_IT_THERE(abfd,w,rsh.sh_name);
    w = SHT_REL;
    PUT_IT_THERE(abfd,w,rsh.sh_type);
    w = (es->asect.flags & SEC_IS_BIG_ENDIAN) ? SHF_960_MSB : 0;
    PUT_IT_THERE(abfd,w,rsh.sh_flags);
    PUT_IT_THERE(abfd,as->vma,rsh.sh_addr);
    PUT_IT_THERE(abfd,as->filepos,rsh.sh_offset);
    PUT_IT_THERE(abfd,as->size,rsh.sh_size);
    PUT_IT_THERE(abfd,es->sh_link->asect.secnum+1,rsh.sh_link);
    PUT_IT_THERE(abfd,es->sh_info->asect.secnum+1,rsh.sh_info);
    PUT_IT_THERE(abfd,0,rsh.sh_addralign);
    PUT_IT_THERE(abfd,es->sh_entsize,rsh.sh_entsize);

    /* First, write the section header: */
    if (bfd_write(&rsh,1,sizeof(Elf32_Shdr),abfd) != sizeof(Elf32_Shdr))
	    return false;
    else {
	int cnt,reloc_count = es->sh_info->asect.reloc_count;
	arelent **orelocs = es->sh_info->asect.orelocation;

	/* Now, write the relocations themselves: */
	if (bfd_seek(abfd,es->asect.filepos,SEEK_SET) < 0)
		return false;
	for (cnt=0;cnt < reloc_count;cnt++)
		if (!elf_write_elf32_rel(abfd,orelocs[cnt]))
			return false;
	return true;
    }
}

static          boolean
DEFUN(write_elf_section,(abfd,as),
      bfd *abfd AND
      asection *as)
{
    elf_data_type *edt = elf_data(abfd);
    Elf32_Shdr esh;

    if (as) {
	unsigned w = _bfd_lookup_hashed_str_tab_offset(abfd,edt->EDT_HST_SHSTRTAB,as->name,strlen(as->name));
	unsigned es = 0;

	PUT_IT_THERE(abfd,w,esh.sh_name);
	if (as == edt->EDT_SYMTAB) {
	    w = SHT_SYMTAB;
	    es = sizeof(Elf32_Sym);
	}
	else if (as == edt->EDT_SHSTRTAB || as == edt->EDT_STRTAB)
		w = SHT_STRTAB;
	else if (as->flags & SEC_IS_CCINFO) {
	    unsigned long where = bfd_tell(abfd);
	    int r;

	    w = SHT_960_INTEL_CCINFO;
	    bfd_seek(abfd,as->filepos,SEEK_SET);
	    r = bfd960_write_ccinfo(abfd);
	    bfd_seek(abfd,where,SEEK_SET);
	    BFD_ASSERT(abfd->flags & HAS_RELOC);   /* This will only be done for relocatable files. */
	}
	else if ((as->flags & SEC_HAS_CONTENTS) &&
		 ((as->flags & (SEC_IS_DSECT | SEC_IS_NOLOAD)) == 0))
		w = SHT_PROGBITS;
	else {
	    w = SHT_NOBITS;
	}
	PUT_IT_THERE(abfd,w,esh.sh_type);

	w = 0;

	if ((abfd->flags & HAS_PIC) &&
	    (as->flags & SEC_CODE))
		as->flags |= SEC_IS_PI;

	if ((abfd->flags & HAS_PID) &&
	    (as->flags & SEC_DATA))
		as->flags |= SEC_IS_PI;

#define set_elf_sec_flags( BFDISM , ELFISM) if (as->flags & BFDISM) w |= ELFISM

	SECTION_FLAG_MAP( set_elf_sec_flags );

#undef set_elf_sec_flags

	if (!(abfd->flags & CAN_LINKPID))
		w &= ~SHF_960_LINK_PIX;

	if (as->flags & (SEC_IS_DSECT | SEC_IS_NOLOAD))
		w &= ~SHF_ALLOC;

	PUT_IT_THERE(abfd,w,esh.sh_flags);
	PUT_IT_THERE(abfd,as->vma,esh.sh_addr);
	PUT_IT_THERE(abfd,as->filepos,esh.sh_offset);
	if (!(as->flags & SEC_IS_DSECT))
		PUT_IT_THERE(abfd,as->size,esh.sh_size);
	else
		PUT_IT_THERE(abfd,0,esh.sh_size);
	if (as == edt->EDT_SYMTAB) {
	    PUT_IT_THERE(abfd,edt->EDT_STRTAB->secnum+1,esh.sh_link);
	    PUT_IT_THERE(abfd,edt->EDT_ONE_PAST_STB_LOCAL,esh.sh_info);
	}
	else {
	    PUT_IT_THERE(abfd,0,esh.sh_link);
	    PUT_IT_THERE(abfd,0,esh.sh_info);
	}
	PUT_IT_THERE(abfd,as->alignment_power ? (1 << as->alignment_power) : 0,esh.sh_addralign);
	PUT_IT_THERE(abfd,es,esh.sh_entsize);
    }
    else
	    memset(&esh,0,sizeof(Elf32_Shdr));
    return bfd_write(&esh,1,sizeof(Elf32_Shdr),abfd) == sizeof(Elf32_Shdr);
}

static elf_compute_section_file_positions(abfd)
    bfd *abfd;
{
    unsigned long sofar = sizeof(Elf32_Ehdr);      /* first is the elf file header. */
    asection *p;
    elf_data_type *edt = elf_data(abfd);

#define WORD_ALIGN( V ) (((V) + 3) & ~3)
                                                   /* Next, is the program header table. */
    if (abfd->flags & EXEC_P) {
	edt->EDT_PROG_HDR_COUNT = 0;
	for (p=abfd->sections; p; p = p->next) {

	    if (SECTION_IS_LOADED(p)) {
		edt->EDT_PROG_HDR_COUNT++;
		sofar += sizeof(Elf32_Phdr);
	    }
	}
    }

    edt->EDT_SECT_HDR_COUNT = 1;  /* This is one-based because the first entry
				     is the null entry. */
    if (bfd_get_symcount(abfd)) {                  /* Next is the sections themselves. */
	edt->EDT_SHSTRTAB_IDX = edt->EDT_SHSTRTAB->secnum+1;
	for (p=abfd->sections; p; p = p->next) {
	    elf_section *es = (elf_section *) p;

	    if (p == edt->EDT_SYMTAB ||                         /* This section is .symtab. */
		es->sh_link == (elf_section *) edt->EDT_SYMTAB) /* This section is a .rel* section. */
		    p->filepos = WORD_ALIGN(sofar);
	    else
		    p->filepos = sofar;

	    edt->EDT_SECT_HDR_COUNT++;
	    if ((p->flags & SEC_HAS_CONTENTS) &&
		((p->flags & (SEC_IS_NOLOAD | SEC_IS_DSECT)) == 0))
		    sofar += p->size;
	}
    }
    else {
	for (p=abfd->sections; p; p = p->next) {
	    if (SECTION_IS_LOADED(p) || p == edt->EDT_SHSTRTAB) {
		elf_section *es = (elf_section *) p;

		if (p == edt->EDT_SHSTRTAB)
			edt->EDT_SHSTRTAB_IDX = edt->EDT_SECT_HDR_COUNT;
		edt->EDT_SECT_HDR_COUNT++;
		if (p == edt->EDT_SYMTAB ||                         /* This section is .symtab. */
		    es->sh_link == (elf_section *) edt->EDT_SYMTAB) /* This section is a .rel* section. */
			p->filepos = WORD_ALIGN(sofar);
		else
			p->filepos = sofar;
		if (p->flags & SEC_HAS_CONTENTS)
			sofar += p->size;
	    }
	}
    }                                               /* Last is the section header table. */

    edt->EDT_SECTION_HEADER_BASE = WORD_ALIGN(sofar);

#undef WORD_ALIGN

}

static          boolean
DEFUN(elf_write_object_contents,(abfd),
    bfd *abfd)
{
    elf_data_type *edt = elf_data(abfd);
    int cnt;
    asection *as;
    elf_section *es;

    if (abfd->output_has_begun == false)	/* set by bfd.c handler */
	    elf_compute_section_file_positions(abfd);

    /* First, write the section header string table: */
    if (bfd_seek(abfd,edt->EDT_SHSTRTAB->filepos,SEEK_SET) < 0)
	    return false;
    if (bfd_write(edt->EDT_HST_SHSTRTAB->strings,1,cnt=edt->EDT_HST_SHSTRTAB->current_size,abfd) != cnt)
	    return false;

    /* Write the symbols (if any): */
    if (abfd->symcount) {
	int idx;

	/* Write the string table for the symbol table: */
	if (bfd_seek(abfd,edt->EDT_STRTAB->filepos,SEEK_SET) < 0)
		return false;
	if (bfd_write(edt->EDT_HST_STRTAB->strings,1,cnt=edt->EDT_HST_STRTAB->current_size,abfd) != cnt)
		return false;

	/* Now, write the symbols: */
	if (bfd_seek(abfd,edt->EDT_SYMTAB->filepos,SEEK_SET) < 0)
		return false;
	for (idx=0,cnt=0;cnt < bfd_get_symcount(abfd);cnt++) {
	    int rv = write_elf_symbol(abfd,bfd_get_outsymbols(abfd)[cnt],idx,0,0);
	    if (rv == -1)
		    return false;
	    idx += rv;
	}
    }

    /* Write the other section headers: */
    if (bfd_seek(abfd,edt->EDT_SECTION_HEADER_BASE,SEEK_SET) < 0)
	    return false;

    /* Write out section 0. */
    write_elf_section(abfd,0);

    if (1) {
	int total_sect_hdrs_written = 1;

	/* Write out the other sections: */
	for (es=edt->EDT_RELOC_SECTIONS,as=abfd->sections;
	     as;
	     as = as->next) {
	    if (bfd_seek(abfd,edt->EDT_SECTION_HEADER_BASE+total_sect_hdrs_written*sizeof(Elf32_Shdr),SEEK_SET) < 0)
		    return false;
	    if (es && as == &es->asect) {
		if (!write_elf_rel_section(abfd,es))
			return false;
		es = es->next_reloc_section;
	    }
	    else if (abfd->symcount) {
		if (!write_elf_section(abfd,as)) 
			return false;
	    }
	    else {
		if (SECTION_IS_LOADED(as) || as == edt->EDT_SHSTRTAB) {
		    if (!write_elf_section(abfd,as)) 
			    return false;
		}
	    }
	    total_sect_hdrs_written++;
	}
    }

    if ((abfd->flags & HAS_CCINFO) && ! (abfd->flags & HAS_RELOC))
	    bfd960_write_ccinfo(abfd);

    if (1) {
	static int elf_write_program_headers();

	/* Write the program headers: */
	int phdr_cnt = elf_write_program_headers(abfd);

	if (phdr_cnt == -1)
		return false;

	/* Finally, write out the Elf header itself: */
	if (bfd_seek(abfd,0,SEEK_SET) < 0)
		return false;

	return write_elf_header(abfd,phdr_cnt);
    }
}

static void elf_make_program_header(abfd,as,phdr)
    bfd *abfd;
    asection *as;
    Elf32_Phdr *phdr;
{
    unsigned long wf = 0;

    if (as && SECTION_IS_LOADED(as)) {
	phdr->p_type = PT_LOAD;
	phdr->p_offset = as->filepos;
	phdr->p_paddr = as->pma;
	phdr->p_vaddr = as->vma;
	if ((as->flags & SEC_HAS_CONTENTS) == 0)
		phdr->p_filesz = 0;
	else
		phdr->p_filesz = as->size;
	phdr->p_memsz = as->size;
	if (as->flags & SEC_CODE)
		wf |= (PF_X | PF_R);
	if (as->flags & (SEC_IS_BSS|SEC_DATA))
		wf |= (PF_W | PF_R);
	phdr->p_flags = wf;
	phdr->p_align = 1;
    }
}

static int elf_write_program_header(abfd,phdr)
    bfd *abfd;
    Elf32_Phdr *phdr;
{
    unsigned long wf = 0;
    Elf32_Phdr wphdr;

#define X(E) PUT_IT_THERE(abfd,phdr->E,wphdr.E)
    X(p_type);
    X(p_offset);
    X(p_paddr);
    X(p_vaddr);
    X(p_filesz);
    X(p_memsz);
    X(p_flags);
    X(p_align);
#undef X
    return bfd_write(&wphdr,1,sizeof(Elf32_Phdr),abfd) == sizeof(Elf32_Phdr);
}

static int elf_write_program_headers(abfd)
    bfd *abfd;
{
    if (abfd->flags & EXEC_P) {
	asection *p;
	elf_data_type *edt = elf_data(abfd);
	if (edt->EDT_PROG_HDR_COUNT > 0) {
	    int i;
	    Elf32_Phdr *phdrs = (Elf32_Phdr *) bfd_zalloc(abfd,edt->EDT_PROG_HDR_COUNT * sizeof(Elf32_Phdr));

	    /* Sort the program headers: */
	    for (i=0,p=abfd->sections; p; p = p->next)
		    if (SECTION_IS_LOADED(p)) {
			elf_make_program_header(abfd,p,phdrs+i);
			i++;
		    }

	    qsort(phdrs,edt->EDT_PROG_HDR_COUNT,sizeof(Elf32_Phdr),compare_prog_hdrs);

	    if (bfd_seek(abfd,sizeof(Elf32_Ehdr),SEEK_SET) < 0)
		    return -1;
	    for (i=0;i < edt->EDT_PROG_HDR_COUNT;i++)
		    if (!elf_write_program_header(abfd,phdrs+i))
			    return -1;
	    bfd_release(abfd,phdrs);
	    return edt->EDT_PROG_HDR_COUNT;
	}
    }
    return 0;
}


static boolean
DEFUN(elf_set_section_contents,(abfd, section, location, offset, count),
    bfd      *abfd AND
    sec_ptr   section AND
    PTR       location AND
    file_ptr  offset AND
    size_t    count)
{
    if (abfd->output_has_begun == false)	/* set by bfd.c handler */
	    elf_compute_section_file_positions(abfd);

    bfd_seek(abfd, (file_ptr) (section->filepos + offset), SEEK_SET);

    if (count != 0) {
	return bfd_write(location,1,count,abfd) == count;
    }
    return true;
}

static boolean
DEFUN(elf_new_section_hook,(abfd_ignore, section_ignore),
    bfd      *abfd_ignore AND
    asection *section_ignore)
{
	return true;
}

/* Relocation stuff. */

static reloc_howto_type
howto_reloc_rel_none     = { "none",     R_960_NONE,     bfd_reloc_type_none };

static reloc_howto_type
howto_reloc_rel_12       = { "rel_12",   R_960_12,       bfd_reloc_type_12bit_abs };

static reloc_howto_type
howto_reloc_rel_32       = { "rel_32",   R_960_32,       bfd_reloc_type_32bit_abs };

static reloc_howto_type
howto_reloc_rel_ip24     = { "ip_24",    R_960_IP24,     bfd_reloc_type_24bit_pcrel };

static reloc_howto_type
howto_reloc_rel_optcall  = { "optcall",  R_960_OPTCALL,  bfd_reloc_type_opt_call };

static reloc_howto_type
howto_reloc_rel_optcallx = { "optcallx", R_960_OPTCALLX, bfd_reloc_type_opt_callx };

static reloc_howto_type
howto_reloc_rel_sub      = { "sub32",    R_960_SUB,      bfd_reloc_type_32bit_abs_sub };

static reloc_howto_type
howto_reloc_rel_unknown  = { "unknown?", 0,              bfd_reloc_type_unknown };


static unsigned int
	elf_get_symtab_upper_bound(abfd)
bfd *abfd;
{
    elf_slurp_symbol_table(abfd);
    return (abfd->symcount + 1) * sizeof(asymbol *);
}

static unsigned int
elf_get_symtab(abfd, alocation)
    bfd      *abfd;
    asymbol **alocation;
{
    elf_symbol_type *syms = elf_data(abfd)->EDT_SYMBOLS;
    unsigned int  i;

    for (i=0; i < bfd_get_symcount(abfd); i++,syms++,alocation++)
	    *alocation = &syms->asym;
    *alocation = 0;
    return bfd_get_symcount(abfd);
}

static unsigned int
elf_get_reloc_upper_bound(abfd, asect)
    bfd     *abfd;
    sec_ptr  asect;
{
    return (asect->reloc_count + 1) * sizeof(arelent *);
}

static Elf32_Rel *read_raw_relocs(abfd,section)
    bfd *abfd;
    asection *section;
{
    elf_section *es = (elf_section *) section;  /* This is the elf_section of the
						   section for which relocations will
						   be returned.  (e.g. .text) */
    elf_section *res = es->sh_link;		 /* This is the elf_section of
							 the relocation section.
							 (e.g. .rel.text) */
    Elf32_Rel *raw_read_relocs;
    Elf32_Rel *raw_relocs;

    if (bfd_seek(abfd,res->asect.filepos,SEEK_SET) < 0)
	    return (Elf32_Rel *) 0;

    raw_read_relocs = (Elf32_Rel *) bfd_alloc(abfd,res->asect.size);
    if (bfd_read(raw_read_relocs,1,res->asect.size,abfd) != res->asect.size) {
	bfd_release(abfd,raw_read_relocs);
	return (Elf32_Rel *) 0;
    }
    else {
	/* Swap all of the raw relocation table entries: */
	char *q,*end;
	Elf32_Rel *rrels;

	rrels = raw_relocs = (Elf32_Rel *) bfd_alloc(abfd,(section->reloc_count) * sizeof(Elf32_Rel));

	for (q=(char *)raw_read_relocs,end=q+res->asect.size;q < end;rrels++,q += res->sh_entsize) {
	    Elf32_Rel *p = (Elf32_Rel *) q;
	    elf_swap_Elf32_Rel_in(abfd,p,rrels,res->asect.flags & SEC_IS_BIG_ENDIAN);
	}
	if (raw_read_relocs)
		bfd_release(abfd,raw_read_relocs);
	return raw_relocs;
    }
}


static boolean
	DEFUN(elf_slurp_reloc_table,(abfd, asect, symbols),
	      bfd      *abfd  AND
	      sec_ptr   asect AND
	      asymbol **symbols)
{
    Elf32_Rel *r,*raw_relocs;
    int i,sym_flag;
    arelent *reloc_cache;
    
    if (asect->relocation || asect->reloc_count == 0)
	    return true;
    if (((abfd->flags & HAS_RELOC) == 0) || !elf_slurp_symbol_table(abfd))
	    return false;

    raw_relocs = read_raw_relocs(abfd,asect);

    reloc_cache = (arelent *)
	    bfd_alloc(abfd, (size_t) (asect->reloc_count * sizeof(arelent)));

    asect->relocation = reloc_cache;

    if (asect->flags & SEC_IS_DEBUG)
	    sym_flag = BSF_SYM_REFD_DEB_SECT;
    else
	    sym_flag = BSF_SYM_REFD_OTH_SECT;

    for (r=raw_relocs,i=0;i < asect->reloc_count;i++,reloc_cache++,r++) {
	asymbol *ref;

	reloc_cache->sym_ptr_ptr = symbols + ELF32_R_SYM(r->r_info);
	ref = *(reloc_cache->sym_ptr_ptr);
	ref->flags |= sym_flag;
	reloc_cache->address = r->r_offset;
	switch (ELF32_R_TYPE(r->r_info)) {
    case R_960_12       :
	    reloc_cache->howto = &howto_reloc_rel_12;
	    break;
    case R_960_32       :
	    reloc_cache->howto = &howto_reloc_rel_32;
	    break;
    case R_960_IP24     :
	    reloc_cache->howto = &howto_reloc_rel_ip24;
	    break;
    case R_960_OPTCALL  :
	    reloc_cache->howto = &howto_reloc_rel_optcall;
	    break;
    case R_960_OPTCALLX :
	    reloc_cache->howto = &howto_reloc_rel_optcallx;
	    break;
    case R_960_SUB      :
	    reloc_cache->howto = &howto_reloc_rel_sub;
	    break;
    case R_960_NONE     :
	    reloc_cache->howto = &howto_reloc_rel_none;
	    break;
    default:
	    reloc_cache->howto = &howto_reloc_rel_unknown;
	    break;
	}
    }
    bfd_release(abfd,raw_relocs);
    return true;
}


static unsigned int
elf_canonicalize_reloc(abfd, section, relptr, symbols)
    bfd      *abfd;
    sec_ptr   section;
    arelent **relptr;
    asymbol **symbols;
{
    int i;
    arelent *p;

    if (!elf_slurp_reloc_table(abfd,section,symbols) ||
	!section->relocation)
	    return 0;

    for ( p = section->relocation, i = 0; i < section->reloc_count; i++ )
	    *relptr++ = p++;
    *relptr = 0;
    return section->reloc_count;
}

static asymbol *
elf_make_empty_symbol(abfd)
    bfd *abfd;
{
    elf_symbol_type *p = (elf_symbol_type *) bfd_zalloc(abfd,sizeof(elf_symbol_type));
    p->asym.the_bfd = abfd;
    return &p->asym;
}

static void 
DEFUN(elf_print_symbol,(ignore_abfd, file, symbol, how),
    bfd            *ignore_abfd AND
    FILE           *file AND
    asymbol        *symbol AND
    bfd_print_symbol_enum_type how)
{
}

static alent   *
DEFUN(elf_get_lineno,(ignore_abfd, symbol),
    bfd            *ignore_abfd AND
    asymbol        *symbol)
{
    return (alent *)NULL;
}

static          boolean
DEFUN(elf_set_arch_mach,(abfd, arch, machine, real_machine),
    bfd         *abfd AND
    enum	bfd_architecture arch AND
    unsigned long   machine           AND
    unsigned long   real_machine)
{
    abfd->obj_arch = arch;
    abfd->obj_machine = machine;
    bfd_set_target_arch(abfd,real_machine);
    if (arch == bfd_arch_unknown)	/* Unknown machine arch is OK */
	    return 1;
    if (arch == bfd_arch_i960){
	switch (machine) {
    case bfd_mach_i960_core:
    case bfd_mach_i960_core2:
    case bfd_mach_i960_kb_sb:
    case bfd_mach_i960_ca:
    case bfd_mach_i960_jx:
    case bfd_mach_i960_ka_sa:
	    return 1;
	}
    }
    return 0;
}

static boolean
DEFUN(elf_find_nearest_line,(abfd,
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
    return 0;
}

static int 
DEFUN(elf_sizeof_headers,(abfd, reloc),
      bfd *abfd AND
      boolean reloc)
{
    return 0;
}

/* Elf symbol tabledumper code starts here. */

static char *sym_type(s)
    Elf32_Sym *s;
{
    static struct type_names {
	int value;
	char *name;
    } typenames[] = {
    { STT_NOTYPE,  "NONE" },
    { STT_OBJECT,  "OBJECT" },
    { STT_FUNC,    "FUNC" },
    { STT_SECTION, "SECTION" },
    { STT_FILE,    "FILE" },
    { 0, NULL } };
    struct type_names *ptn;

    for (ptn=typenames;ptn->name;ptn++)
	    if (ELF32_ST_TYPE(s->st_info) == ptn->value)
		return ptn->name;
    return "UNKNOWN";
}

static CONST char *sym_section(abfd,s)
    bfd *abfd;
    Elf32_Sym *s;
{
    switch (s->st_shndx) {
 case SHN_UNDEF:
	return "UNDEFINED";
 case SHN_ABS:
	return "ABSOLUTE";
 case SHN_COMMON:
	return "COMMON";
 default:
	return Nth_asection(abfd,s->st_shndx)->name;
    }
}

static char *sym_binding(s)
    Elf32_Sym *s;
{
    static struct bind_names {
	int value;
	char *name;
    } bindnames[] =
	{
	{ STB_LOCAL,  "LOCAL" },
        { STB_GLOBAL, "GLOBAL" },
        { STB_WEAK,   "WEAK" },
	{ 0, NULL } };
    struct bind_names *pbn;

    for (pbn=bindnames;pbn->name;pbn++)
	    if (ELF32_ST_BIND(s->st_info) == pbn->value)
		    return pbn->name;
    return "UNKNOWN";
}

static int elf_dmp_symtab(abfd, suppress_headers, suppress_translation, section_number) 
    bfd 	*abfd;
    int	suppress_headers;	/* 1 = Just the facts, ma'am */
    int	suppress_translation;	/* 1 = Print type, sclass in hex */
    int	section_number;		/* -1 = Print all symbols;
				 * >= 0 = Print symbols from this 
				 * section only 
				 */
{
    int i,nsyms;
    char *str_tab;
    Elf32_Sym *raw_syms = read_raw_syms(abfd,&nsyms,&str_tab);

    if (!str_tab || !nsyms || !raw_syms)
	    return 0;

    if (!suppress_headers) {
	printf("Index Value      Size   Binding    Type        Section          Oth Name\n");
    }
    if (section_number != -1)
	    section_number--;  /* Adjust for bizarre bfd coff-ism per dumper. */
    for (i=0;i < nsyms;i++) {
	Elf32_Sym *s = raw_syms + i;

	if (section_number == -1 || s->st_shndx == section_number)
		printf("%5d 0x%08x %6d %-10s %-11s %-16s 0x%x %s\n",i,s->st_value,s->st_size,
		       sym_binding(s),sym_type(s),sym_section(abfd,s),s->st_other,s->st_name + str_tab);
    }
    return 1;
}

static int elf_dmp_linenos()
{
    fprintf(stderr, "Line table dump not implemented for elf files\n" );
    return 1;
}

static int elf_dmp_full_fmt()
{
    return 0;
}



static int
elf_more_symbol_info(abfd, asym, m, info_type)
    bfd *abfd;	
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
	break;
	
 case bfd_set_sysproc_val:
	if (asym->flags & BSF_HAS_SCALL) {
	    asymbol *as = asym->related_symbol;

	    as->value = m->sysproc_value;
	    return 1;
	}
	else
		return 0;
	break;

#define IsNativeSymbol(ASYM) (ASYM->the_bfd && BFD_ELF_FILE_P(ASYM->the_bfd))

 case bfd_function_size:
	if (IsNativeSymbol(asym)) {
	    native_elf_info *nei = fetch_native_elf_info(asym);

	    if (ELF32_ST_TYPE(nei->st_info) == STT_FUNC) {
		m->function_size = nei->st_size;
		return 1;
	    }
	}
	return 0;
	break;

 case bfd_symbol_type:
	if (IsNativeSymbol(asym)) {
	    native_elf_info *nei = fetch_native_elf_info(asym);

	    m->sym_type = nei->st_info;
	    return 1;
	}
	return 0;
	break;

 default:
	    return 0;
    }
}

static int elf_zero_line_info()
{
    return 0;
}

static bfd_ghist_info *
elf_fetch_ghist_info(abfd, nelements, nlinenumbers)
    bfd *abfd;
    unsigned int *nelements,*nlinenumbers;
{
    bfd_ghist_info	*p = NULL;
    elf_symbol_type	*sym_ptr;
    int 		p_max, i;
    unsigned int	endtext = 0;
    char		*file_name = _BFD_GHIST_INFO_FILE_UNKNOWN; 

    *nelements = *nlinenumbers = 0;

    if (!elf_slurp_symbol_table(abfd))
	    return p;

    p_max = bfd_get_symcount(abfd);
    p = (bfd_ghist_info *)bfd_alloc(abfd, p_max*sizeof(bfd_ghist_info));

    /* Build function table from elf info */
    for (sym_ptr = elf_data(abfd)->EDT_SYMBOLS, i = 0; i < p_max; sym_ptr++, i++)
    {
	asymbol	*as = &(sym_ptr->asym);
	unsigned int addr;
	CONST char *sym_name;

	if (!as->section || 
	    (as->section->flags & SEC_CODE) == 0 ||
	    strchr(as->name, '.') ||
	    strcmp(as->name, "") == 0 ||
	    strcmp(as->name, "___gnu_compiled_c") == 0)
	{
	    if (strcmp(as->name, "__Etext") == 0)
	        endtext = (as->value > endtext) ? as->value : endtext;
	    continue;
	}
	addr = as->value + as->section->vma;
	sym_name = _bfd_trim_under_and_slashes(as->name, 1);
	_bfd_add_bfd_ghist_info(&p, nelements, &p_max, addr, sym_name,
	    file_name, 0);
    }
    BFD_ASSERT(endtext);
    _bfd_add_bfd_ghist_info(&p, nelements, &p_max, endtext, "__Etext",
	file_name, 0);

    qsort(p, *nelements, sizeof(bfd_ghist_info), _bfd_cmp_bfd_ghist_info);

    return p;
}

static elf_symbol_type *make_section_symbol(abfd,name,as,size)
    bfd *abfd;
    char *name;
    asection *as;
    unsigned long size;
{
    asymbol *asym = elf_make_empty_symbol(abfd);

    asym->name = name;
    asym->section = as;
    asym->flags = BSF_LOCAL;
    put_native_elf_info(asym);
    fetch_native_elf_info(asym)->st_size = size;
    fetch_native_elf_info(asym)->st_info = ELF32_ST_INFO(STB_LOCAL,STT_SECTION);
    return (elf_symbol_type *) asym;
}

static void add_symbol(abfd,sym_list,max_cnt,curr_cnt,sym)
    bfd *abfd;
    elf_symbol_type ***sym_list;
    unsigned long *max_cnt,*curr_cnt;
    elf_symbol_type *sym;
{
    if ((*curr_cnt)+1 >= (*max_cnt))
	    *sym_list = (elf_symbol_type **)bfd_realloc(abfd,*sym_list,((*max_cnt) *= 2)*sizeof(elf_symbol_type *));
    (*sym_list)[*curr_cnt] = sym;
    (*curr_cnt)++;
}

static elf_symbol_type *elf_make_file_symbol(abfd,name,edt)
    bfd *abfd;
    char *name;
    elf_data_type *edt;
{
    asymbol *asym = elf_make_empty_symbol(abfd);

    asym->name = name;
    _bfd_lookup_hashed_str_tab_offset(abfd,edt->EDT_HST_STRTAB,name,strlen(name));
    put_native_elf_info(asym);
    fetch_native_elf_info(asym)->st_info = ELF32_ST_INFO(STB_LOCAL,STT_FILE);
    return (elf_symbol_type *) asym;
}

static asection *
lookup_elf_section(abfd,sect_name)
    bfd *abfd;
    char *sect_name;
{
    asection *sec;

    for (sec=abfd->sections;sec;sec = sec->next)
	    if (!strcmp(sect_name,sec->name))
		    return sec;
    return (asection *) 0;
}

static asymbol ** elf_set_sym_tab(abfd,location,symcount)
    bfd *abfd;
    asymbol **location;
    unsigned *symcount;
{
    unsigned long i,actual_number_of_symbols = 0,actual_number_of_global_non_native_symbols = 0,
    actual_number_of_global_non_native_symbols_max = *symcount,
    actual_number_of_symbols_max = *symcount + abfd->section_count + 4; 	/* zero symbol
									   File symbol 
									   str tab
									   sym tab     == 4
									   sections * 2 (so each can have a
									   .rel symbol)
									   symcount  */
    elf_symbol_type **actual_symbol_tablelist,**global_non_native_symbol_list,*est;
    asection *as;
    asymbol *asym;
    elf_data_type *edt = elf_data(abfd);

    if (abfd->output_has_begun)
	    return bfd_get_outsymbols(abfd);

    edt->EDT_SHSTRTAB = bfd_make_section(abfd,".shstrtab");
    edt->EDT_SHSTRTAB->flags |= SEC_HAS_CONTENTS;
    edt->EDT_SHSTRTAB->output_section = edt->EDT_SHSTRTAB;
    edt->EDT_HST_SHSTRTAB = _bfd_init_hashed_str_tab(abfd,0);

    /* Force the first element of the string tables to be a null string: */

    _bfd_lookup_hashed_str_tab_offset(abfd,edt->EDT_HST_SHSTRTAB,"",0);

    if (*symcount) {
	asection **bottom_of_asection_list;
	elf_section **bottom_of_elf_section_list;

	bottom_of_elf_section_list = &edt->EDT_RELOC_SECTIONS;
	actual_symbol_tablelist = (elf_symbol_type **)
		bfd_alloc(abfd,actual_number_of_symbols_max * sizeof(elf_symbol_type *));
	global_non_native_symbol_list = (elf_symbol_type **)
		bfd_alloc(abfd,actual_number_of_global_non_native_symbols_max * sizeof(elf_symbol_type *));

#define ADD_SYM(SYM) add_symbol(abfd,&actual_symbol_tablelist,&actual_number_of_symbols_max,\
				    &actual_number_of_symbols,SYM)

#define ADD_G_NN_SYM(SYM) add_symbol(abfd,&global_non_native_symbol_list,\
				     &actual_number_of_global_non_native_symbols_max,\
				    &actual_number_of_global_non_native_symbols,SYM)

	edt->EDT_STRTAB   = bfd_make_section(abfd,".strtab");
	edt->EDT_STRTAB->flags |= SEC_HAS_CONTENTS;
	edt->EDT_STRTAB->output_section = edt->EDT_STRTAB;
	edt->EDT_SYMTAB   = bfd_make_section(abfd,".symtab");
	edt->EDT_SYMTAB->flags |= SEC_HAS_CONTENTS;
	if (BFD_BIG_ENDIAN_FILE_P(abfd))
		edt->EDT_SYMTAB->flags |= SEC_IS_BIG_ENDIAN;
	edt->EDT_SYMTAB->output_section = edt->EDT_SYMTAB;
	edt->EDT_HST_STRTAB   = _bfd_init_hashed_str_tab(abfd,0);
	_bfd_lookup_hashed_str_tab_offset(abfd,edt->EDT_HST_STRTAB,"",0);

	for (bottom_of_asection_list = &abfd->sections;*bottom_of_asection_list;
	     bottom_of_asection_list = &(*bottom_of_asection_list)->next)
		;

	/* Entry 0 of the symtab is all zeroes: */
	ADD_SYM(elf_make_empty_symbol(abfd));

	/* Entries 1  ... are the SECTION symbols. */
	for (as=abfd->sections;as;as = as->next) {
	    /* FIXME?  Should section symbols contain the size of the section?.
	       We put out 0 for the size unconditionally because we do not know the size of
	       some of the sections yet:                 vvvvvvvvvvvvvvvv  */
	    ADD_SYM(make_section_symbol(abfd,as->name,as,0 ? as->size : 0));
	    _bfd_lookup_hashed_str_tab_offset(abfd,edt->EDT_HST_SHSTRTAB,as->name,strlen(as->name));
	    _bfd_lookup_hashed_str_tab_offset(abfd,edt->EDT_HST_STRTAB,as->name,strlen(as->name));
	    /* Have to add back in the relocation sections if asked to do so: */
	    if ((abfd->flags & HAS_RELOC) && as->reloc_count) {
		char *rel_sect_name = bfd_alloc(abfd,5 /* '.rel NULL' is 5 chars. */ +
						strlen(as->name));

		sprintf(rel_sect_name,".rel%s",as->name);
		if (lookup_elf_section(abfd,rel_sect_name))
			bfd_release(abfd,rel_sect_name);
		else {
		    elf_section *rel_sect = (elf_section *) bfd_zalloc(abfd,sizeof(elf_section));;

		    (*bottom_of_asection_list) = &rel_sect->asect;
		    bottom_of_asection_list = &rel_sect->asect.next;

		    rel_sect->asect.flags |= SEC_HAS_CONTENTS;
		    if (BFD_BIG_ENDIAN_FILE_P(abfd))
			    rel_sect->asect.flags |= SEC_IS_BIG_ENDIAN;
		    rel_sect->sh_entsize = sizeof(Elf32_Rel);
		    rel_sect->sh_link = (elf_section *) edt->EDT_SYMTAB;
		    rel_sect->sh_info = (elf_section *) as;

		    rel_sect->asect.size = as->reloc_count * sizeof(Elf32_Rel);
		    rel_sect->asect.name = rel_sect_name;
		    rel_sect->asect.output_section = &rel_sect->asect;

		    (*bottom_of_elf_section_list) = rel_sect;
		    bottom_of_elf_section_list = &rel_sect->next_reloc_section;

		    rel_sect->asect.secnum = abfd->section_count++;
		}
	    }
	}

	for (i=0;i < *symcount;i++) {
	    asymbol *as = location[i];
	    elf_symbol_type *est = (elf_symbol_type *) 0;

	    if (IsNativeSymbol(as)) {
		est = (elf_symbol_type *) as;
		if (fetch_native_elf_info(as)) {
		    if (((ELF32_ST_TYPE(fetch_native_elf_info(as)->st_info) == STT_SECTION)) ||
			(est->asym.flags & BSF_ZERO_SYM) ||
			((abfd->flags & STRIP_DEBUG) && is_debug_section_name(as->name,1) &&
			 (as->flags & BSF_UNDEFINED)))
			    continue;
		    if (as->flags & BSF_HAS_CALL)
			    continue;
		    /* If we strip an elf/dwarf file, and a .debug section defines
		       a symbol, we turn the definition into an undefined symbol. */
		    if ((abfd->flags & STRIP_LINES) &&
			as->section &&
			!as->section->output_section &&
			(as->section->flags & SEC_IS_DEBUG)) {
			as->section = 0;
			as->flags = BSF_UNDEFINED | BSF_ALL_OMF_REP;
		    }
		    if (edt->EDT_ONE_PAST_STB_LOCAL == 0 &&
			ELF32_ST_BIND(fetch_native_elf_info(as)->st_info) != STB_LOCAL)
			    edt->EDT_ONE_PAST_STB_LOCAL = actual_number_of_symbols;
		    if (as->flags & (BSF_HAS_SCALL | BSF_HAS_BAL)) {
			ADD_SYM(est);
			_bfd_lookup_hashed_str_tab_offset(abfd,edt->EDT_HST_STRTAB,
							  est->asym.name,strlen(est->asym.name));
			est = (elf_symbol_type *) est->asym.related_symbol;
		    }
		}
		else if (edt->EDT_ONE_PAST_STB_LOCAL == 0 &&
			 (as->flags & BSF_GLOBAL))
			edt->EDT_ONE_PAST_STB_LOCAL = actual_number_of_symbols;
		ADD_SYM(est);
		if (est->asym.name)
			_bfd_lookup_hashed_str_tab_offset(abfd,edt->EDT_HST_STRTAB,est->asym.name,
							  strlen(est->asym.name));
	    }
	    else {
		if (as->flags & BSF_ALL_OMF_REP) {
		    if ((as->flags & BSF_LOCAL) && !(as->the_bfd->flags & ADDED_FILE_SYMBOL)) {
			ADD_SYM(elf_make_file_symbol(abfd,as->the_bfd->filename,edt));
			_bfd_lookup_hashed_str_tab_offset(abfd,edt->EDT_HST_STRTAB,as->the_bfd->filename,
							  strlen(as->the_bfd->filename));
			as->the_bfd->flags |= ADDED_FILE_SYMBOL;
		    }
		    if (as->flags & (BSF_HAS_BAL|BSF_HAS_SCALL)) {
			if (as->flags & BSF_HAS_SCALL)
				as->related_symbol->flags |= BSF_ABSOLUTE;
			if (as->flags & BSF_GLOBAL) {
			    ADD_G_NN_SYM((elf_symbol_type *) as);
			}
			else {
			    ADD_SYM((elf_symbol_type *) as);
			    if (as->name)
				    _bfd_lookup_hashed_str_tab_offset(abfd,edt->EDT_HST_STRTAB,as->name,
								      strlen(as->name));
			}
			as->native_info = NULL;
			as = as->related_symbol;
		    }
		    else if (as->flags & BSF_HAS_CALL)
			    continue;
		    est = (elf_symbol_type *) as;
		    as->native_info = NULL;
		    if (as->flags & (BSF_GLOBAL|BSF_UNDEFINED|BSF_FORT_COMM))
			    ADD_G_NN_SYM((elf_symbol_type *) as);
		    else {
			ADD_SYM((elf_symbol_type *) as);
			if (as->name)
				_bfd_lookup_hashed_str_tab_offset(abfd,edt->EDT_HST_STRTAB,as->name,
								  strlen(as->name));
		    }
		}
	    }
	}
	if (actual_number_of_global_non_native_symbols) {
	    int i;

	    if (!edt->EDT_ONE_PAST_STB_LOCAL)
		    edt->EDT_ONE_PAST_STB_LOCAL = actual_number_of_symbols;
	    for (i=0;i < actual_number_of_global_non_native_symbols;i++) {
		elf_symbol_type *est = global_non_native_symbol_list[i];
		ADD_SYM(est);
		if (est->asym.name)
			_bfd_lookup_hashed_str_tab_offset(abfd,edt->EDT_HST_STRTAB,est->asym.name,
							  strlen(est->asym.name));
	    }
	}
	else if (edt->EDT_ONE_PAST_STB_LOCAL == 0)
		edt->EDT_ONE_PAST_STB_LOCAL = actual_number_of_symbols;
	ADD_SYM((elf_symbol_type *) 0);

	free(location);
	free(global_non_native_symbol_list);
	bfd_get_outsymbols(abfd) = (asymbol **) actual_symbol_tablelist;
	*symcount = bfd_get_symcount(abfd) = --actual_number_of_symbols;
	edt->EDT_SYMTAB->size = actual_number_of_symbols * sizeof(Elf32_Sym);
	edt->EDT_STRTAB->size = edt->EDT_HST_STRTAB->current_size;
    }
    else
	    for (as=abfd->sections;as;as = as->next)
		    if (SECTION_IS_LOADED(as) || as == edt->EDT_SHSTRTAB)
			    _bfd_lookup_hashed_str_tab_offset(abfd,edt->EDT_HST_SHSTRTAB,as->name,strlen(as->name));

    edt->EDT_SHSTRTAB->size = edt->EDT_HST_SHSTRTAB->current_size;
    return bfd_get_outsymbols(abfd);
}

static boolean elf_write_outsymbols(abfd)
    bfd *abfd;
{
}

#define elf_slurp_armap			bfd_slurp_coff_armap
#define elf_slurp_extended_name_table	_bfd_slurp_extended_name_table
#define elf_truncate_arname		bfd_dont_truncate_arname
#define elf_openr_next_archived_file	bfd_generic_openr_next_archived_file
#define elf_generic_stat_arch_elt	bfd_generic_stat_arch_elt
#define	elf_get_section_contents	bfd_generic_get_section_contents
#define	elf_close_and_cleanup		bfd_generic_close_and_cleanup
#define	elf_write_armap			coff_write_armap

bfd_target elf_vec_big_host =
{
  BFD_BIG_ELF_TARG,	/* name */
  bfd_target_elf_flavour_enum,
  false,				/* target byte order is big (irrelevant for elf). */
  true,				/* header byte order is big */
  (HAS_RELOC | EXEC_P |		/* object flags */
   HAS_SYMS | HAS_LOCALS |
   HAS_CCINFO | HAS_PID | HAS_PIC | CAN_LINKPID | HAS_LINENO | HAS_DEBUG |
   DO_NOT_ALTER_RELOCS),
  ( SEC_ALL_FLAGS ),/* section flags */
  '/',				/* ar_pad_char */
  15,				/* ar_max_namelen */

_do_getl64, _do_putl64,  _do_getl32, _do_putl32, _do_getl16, _do_putl16, /* data */
_do_getb64, _do_putb64,  _do_getb32, _do_putb32, _do_getb16, _do_putb16, /* hdrs */

  {_bfd_dummy_target, elf_object_p, /* bfd_check_format */
     bfd_generic_archive_p, _bfd_dummy_target},
  {bfd_false, elf_mkobject,	/* bfd_set_format */
     _bfd_generic_mkarchive, bfd_false},
  {bfd_false, elf_write_object_contents,	/* bfd_write_contents */
     _bfd_write_archive_contents, bfd_false},
  JUMP_TABLE(elf),
};

bfd_target elf_vec_little_host =
{
  BFD_LITTLE_ELF_TARG,	/* name */
  bfd_target_elf_flavour_enum,
  false,				/* target byte order is big (irrelevant for elf). */
  false,				/* header byte order is big */
  (HAS_RELOC | EXEC_P |		/* object flags */
   HAS_SYMS | HAS_LOCALS |
   HAS_CCINFO | HAS_PID | HAS_PIC | CAN_LINKPID | HAS_LINENO | HAS_DEBUG |
   DO_NOT_ALTER_RELOCS),
  ( SEC_ALL_FLAGS ),/* section flags */
  '/',				/* ar_pad_char */
  15,				/* ar_max_namelen */

_do_getl64, _do_putl64,  _do_getl32, _do_putl32, _do_getl16, _do_putl16, /* data */
_do_getl64, _do_putl64,  _do_getl32, _do_putl32, _do_getl16, _do_putl16, /* hdrs */

  {_bfd_dummy_target, elf_object_p, /* bfd_check_format */
     bfd_generic_archive_p, _bfd_dummy_target},
  {bfd_false, elf_mkobject,	/* bfd_set_format */
     _bfd_generic_mkarchive, bfd_false},
  {bfd_false, elf_write_object_contents,	/* bfd_write_contents */
     _bfd_write_archive_contents, bfd_false},
  JUMP_TABLE(elf),
};
