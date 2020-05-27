/*****************************************************************************
 * Copyright (C) 1991 Free Software Foundation, Inc.
 *
 * This file is part of GLD, the Gnu Linker.
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
 ******************************************************************************/


/******************************************************************************
 * This file provides linktime generation of code in support of the gcc960
 * profiling and SRAM (onchip 960CA RAM) initialization features.
 *
 * The routine gnu960_enter_special_symbol() should be called each time a
 * reference to a global symbol whose name begins with "___" is about to be
 * entered.  Some such symbols are special to us:  either they give pointers
 * to file-specific profile counter tables that must be united with a top-level 
 * table, or they give the lengths/addresses of SRAM data items that must
 * be initialized by the crt file.  Information about relevant symbols is
 * tracked via a simple linked list.
 *
 * After the global symbols of all input files have been entered,
 * gnu960_gen_linktime_code() should be called.  It generates the necessary
 * tables and/or initialization code as assembler source, then executes the
 * assembler to assemble it (this turned out to be infinitely easier than
 * constructing a dummy input file within the linker).  If such an object file
 * was created, gnu960_gen_linktime_code() will return a pointer to it's name;
 * it should be added to the list of input files and searched for global
 * symbols before linking continues.
 * 
 * A brief summary of the linktime code generated follows:
 *
 * SRAM INITIALIZATION CODE
 *
 *	A global SRAM initialization routine called '___sram_init' is generated.
 *	For each symbol with a name of the form '___sram_length.NNNNN', the
 *	init routine contains the following code:
 *	
 *		lda	NNNN,g0
 *		lda	___sram.NNNN,g1
 *		lda	LEN,g2
 *		callj	__copy
 *
 *	Where 'LEN' is the value of the symbol ___sram_length.NNNNN and
 *	__copy is a local version of memcpy that is also generated.
 *
 * Note that some code is generated even if there
 * sram initializations, so that the link will always resolve all of the
 * symbols in in compiler generated code.
 *
 *****************************************************************************/

#include <stdio.h>
#include "bfd.h"

#ifdef USG
#if !defined(__HIGHC__) && !defined(WIN95)
#	include <unistd.h>
#endif
#	include "sysv.h"
#else   /* BSD */
#	include "string.h"
#ifndef GNU960
#	include <sys/time.h>
#else
#	include <time.h>
#endif
#endif

#include "sysdep.h"
#include "ld.h"
#include "ldsym.h"
#include "ldlang.h"
#include "ldmisc.h"
#include "cc_info.h"

#ifndef DOS
#   define dosslash(a) a
#endif

#define PROF_CTR_SYM_PREFIX	CI_PROF_CTR_PREFIX
#define PROF_CTR_SYM_PREFIX_LEN	( sizeof(PROF_CTR_SYM_PREFIX) - 1 )

#define SRAM_LEN_SYM_PREFIX	"___sram_length."
#define SRAM_LEN_SYM_PREFIX_LEN	( sizeof(SRAM_LEN_SYM_PREFIX) - 1 )

/* Names of linker-generated global symbols */
static char *magic_names[] = {
    "__Btext", "__Etext",
    "__Bdata", "__Edata",
    "__Bbss", "__Ebss",
    "___profile_data_start", "___profile_data_length",
	"_etext",
	"_edata",
	"_end",
	"_bss_start",
	"___sram_init",
	PROF_CTR_SYM_PREFIX,
	SRAM_LEN_SYM_PREFIX,
	NULL
};

/* Format of an entry in linked list of special symbols
 */
typedef struct list {
	struct list *next;
	char *name;
	int origin,length;
} LIST;

/* Dummy first entries of linked lists.
 * Only the 'next' field is of interest in these entries.
 */
#define SRAM_VAR_HASH_SIZE 16
extern void gnu960_add_profile_symbol();
static LIST *sram_var_hash_table[SRAM_VAR_HASH_SIZE];	/* List of SRAM variable symbols */
static int sram_var_count;                              /* Number of elements in the hash table. */

#define TMPNAME	"ldXXXXXX"
#define GNUTMP_DFLT "tmp"

static char *objfile;

void
gnu960_remove_tmps()
{
    if ( objfile && objfile[0] != '\0' ){
#ifdef DOS
	bfd_close_all();
#endif
	unlink( objfile );
	objfile[0] = 0;
    }
}


/* Declare getenv() for rs6000 and dec3100 hosts... */
#if HOST_SYS == AIX_SYS || HOST_SYS == DEC3100_SYS || HOST_SYS == DOS_SYS
#	include <stdlib.h>
#endif

#include "ldmain.h"

#include "dio.h"

static asymbol **
add_symbol(abfd,sym_tab,max_sym_tab,curr_sym_tab,name,offset,flags,section)
    bfd *abfd;
    asymbol **sym_tab;
    int max_sym_tab,*curr_sym_tab;
    char *name;
    unsigned long offset,flags;
    asection *section;
{
    asymbol *t = (asymbol *) 0;

    if (++(*curr_sym_tab) >= max_sym_tab)
	    abort();
    else if (name) {
	t = bfd_make_empty_symbol(abfd);
	t->name = buystring(name);
	t->value = offset;
	t->flags = flags;
	t->section = section;
    }
    sym_tab[(*curr_sym_tab)-1] = t;
    return sym_tab + ((*curr_sym_tab)-1);
}

static void add_code(be,c,code,max_code_size,curr_code_size)
    int be,c;
    char **code;
    int *max_code_size,*curr_code_size;
{
    unsigned long t;

    if (((*curr_code_size) += 4) > (*max_code_size))
	    (*code) = (char *) ldrealloc(*code,((*max_code_size) *= 2));
    if (be) {
	extern void _do_putb32();

	_do_putb32(c,&t);
    }
    else {
	extern void _do_putl32();

	_do_putl32(c,&t);
    }
    memcpy((*code) + ((*curr_code_size) - 4),&t,4);
}

static void add_relocation(relentlist,max_rel_ents,curr_rel_ents,sym_tab_entry,offset,ht)
    arelent **relentlist;
    int max_rel_ents,*curr_rel_ents;
    asymbol **sym_tab_entry;
    unsigned long offset;
    reloc_howto_type *ht;
{
    if (++(*curr_rel_ents) >= max_rel_ents)
	    abort();
    else {
	arelent *t = (arelent *) ldmalloc(sizeof(arelent));
	t->sym_ptr_ptr = sym_tab_entry;
	t->address = offset;
	t->howto = ht;
	relentlist[(*curr_rel_ents)-1] = t;
    }
}

char *
gnu960_gen_linktime_code(need_sram_init,need_clear_bss,named_bss_sect_name)
    boolean need_sram_init;
    boolean need_clear_bss;
    int (*named_bss_sect_name)();
{
    bfd *objbfd;
    extern bfd *output_bfd;
    asection *text_sec;
    int max_code_size = 128,curr_code_size = 0;
    char *code = ldmalloc(max_code_size);
    char *dummy1,*dummy2;
    int number_of_named_bss_sections = named_bss_sect_name(&dummy1,&dummy2,-1);

    /* We are going to make exactly the following number of symbol definitions and
       references: */

    int max_sym_tab_len = sram_var_count + 6 + 2*number_of_named_bss_sections,
    curr_sym_tab_len = 0;
    int max_rel_ents = max_sym_tab_len;
    LIST *p;
    char * q;
    int i;
    asymbol **temp_asym;   /* Temporary asymbol. */
    static reloc_howto_type howto = { NULL,0,bfd_reloc_type_32bit_abs};
    extern boolean data_is_pi,code_is_pi;
    extern int target_byte_order_is_big_endian;

    /* Create tmp object file: */

    if (!(q = (char *)getenv("GLD960TMP"))) {
	extern char *get_960_tools_temp_file();

	objfile = get_960_tools_temp_file("LDXXXXXX.o",ldmalloc);
    }
    else {
	objfile = ldmalloc(strlen(q) + 3);
	strcpy(objfile,q);
	strcat(objfile,".o");
    }

    if ( !(objbfd = bfd_openw(objfile,
			      (target_byte_order_is_big_endian ?
			       BFD_BIG_BIG_COFF_TARG :
			       BFD_BIG_COFF_TARG )
			      ))) {
	info( "%FCan't open temp file '%s'\n", objfile );
	/* NO RETURN */
    }

    bfd_set_format(objbfd,bfd_object);

    if (data_is_pi)
	    objbfd->flags |= HAS_PID;

    if (code_is_pi)
	    objbfd->flags |= HAS_PIC;

    bfd_set_arch_mach(objbfd,bfd_arch_i960,bfd_mach_i960_core,BFD_960_GENERIC);

    if (1) {
	static char *names[] = {".text",
#if 0
					/* If we want to include the following sections for fun: */
					".data", ".bss",
#endif
					NULL},**p;

	for (p=names;*p;p++) {
	    asection *as;

	    if (!(as=bfd_make_section(objbfd,*p)))
		    info( "%FCan't create %s output section for sram init code\n", *p );
	    if (p == names)
		    text_sec = as;
	    as->output_section = as;
	    as->alignment_power = 2;
	}
    }

    if (target_byte_order_is_big_endian)
	    text_sec->flags |= SEC_IS_BIG_ENDIAN;

    objbfd->outsymbols = (asymbol **) ldmalloc(max_sym_tab_len * sizeof(asymbol *));
    text_sec->orelocation =  (arelent ** )ldmalloc(max_rel_ents * sizeof(arelent *));

#define ADD_SYMBOL(NAME,OFFSET,FLAGS,SECTION) add_symbol(objbfd,objbfd->outsymbols,\
							 max_sym_tab_len,&objbfd->symcount,\
							 NAME,OFFSET,FLAGS,SECTION)
#define ADD_CODE(C) add_code(target_byte_order_is_big_endian,C,&code,&max_code_size,&text_sec->size)
#define ADD_RELOCATION(SYMTAB_ENTRY,OFFSET) add_relocation(text_sec->orelocation,\
								 max_rel_ents,&text_sec->reloc_count,\
								 SYMTAB_ENTRY,OFFSET,&howto)

/* This is an alphebetized list of the instruction templates that we use below: */
#define ADDO_4_G0_G0          0x59840804 /* addo 4,g0,g0 */
#define ADDO_G0_1_G0          0x59805010 /* addo g0,1,g0 */
#define ADDO_G1_1_G1          0x59885011 /* addo g1,1,g1 */
#define CALL                  0x09000000 /* call (cntrol insn) */
#define CMPIBNE_0_G2_DOT_M_20 0x3d04bfec /* cmpiibne 0,g2,.-20 */
#define CMPOBL_G0_G1_DOT_M_8  0x34845ff8 /* cmpobl g0,g1,.-8 */
#define LDA_MEMB_G12_G0       0x8c873400 /* lda <memb>(g12),g0 */
#define LDA_MEMB_G12_G1       0x8c8f3400 /* lda <memb>(g12),g1 */
#define LDA_MEMB_G0           0x8c803000 /* lda <memb>,g0 */
#define LDA_MEMA_G0           0x8c800000 /* lda <mema>,g0 */
#define LDA_MEMB_G1           0x8c883000 /* lda <memb>,g1 */
#define LDA_MEMA_G1           0x8c880000 /* lda <mema>,g1 */
#define LDA_MEMB_G2           0x8c903000 /* lda <memb>,g2 */
#define LDA_MEMA_G2           0x8c900000 /* lda <mema>,g2 */
#define LDOB_G0_G4            0x80a41000 /* ldob (g0),g4 */
#define RET                   0x0a000000 /* ret */
#define ST_G14_G0             0x92f41000 /* st g14,(g0) */
#define STOB_G4_G1            0x82a45000 /* stob g4,(g1) */
#define SUBO_1_G2_G2          0x59948901 /* subo 1,g2,g2 */

    if (need_sram_init && sram_var_count) {
	/* Only generate this code if there are variables
	   in SRAM whose space it should initialize */
	static unsigned long copy_code[] = {
            LDOB_G0_G4,            /*  0  */
            STOB_G4_G1,            /*  4  */
	    SUBO_1_G2_G2,          /*  8  */
	    ADDO_G0_1_G0,          /*  c  */
	    ADDO_G1_1_G1,          /* 10  */
/*__copy:*/ CMPIBNE_0_G2_DOT_M_20, /* 14  (the symbol __copy: should be tacked to address 0x14). */
	    RET                    /* 18  */
	}; 
#define COPY_OFFSET 0x14
	int i;

	for (i=0;i < sizeof(copy_code)/sizeof(copy_code[0]);i++)
		ADD_CODE(copy_code[i]);
	ADD_SYMBOL("__copy",COPY_OFFSET,BSF_LOCAL,text_sec);
    }

    if (need_sram_init) {
	ADD_SYMBOL("___sram_init",text_sec->size,BSF_GLOBAL|BSF_EXPORT,text_sec);
	for (i=0;i < SRAM_VAR_HASH_SIZE;i++) {
	    for ( p = sram_var_hash_table[i]; p; p = p->next ){
		char buff[128];

		if (data_is_pi)
			ADD_CODE(LDA_MEMB_G12_G0);
		else
			ADD_CODE(LDA_MEMB_G0);
		sprintf(buff,"___sram.%u",p->origin);
		temp_asym = ADD_SYMBOL(buff,0,BSF_UNDEFINED,(asection *) 0);
		objbfd->flags |= HAS_RELOC;
		ADD_RELOCATION(temp_asym,text_sec->size);
		/* The following word is for the memb */
		ADD_CODE(0);
		/* Can the origin fit in 12 bits? If so, generate a MEMA,
		   else generate a MEMB. */
		if (p->origin & (~0xfff)) {
		    ADD_CODE(LDA_MEMB_G1);
		    /* The memb word: */
		    ADD_CODE(p->origin);
		}
		else
			ADD_CODE(LDA_MEMA_G1 | p->origin);
		/* Can the length fit in 12 bits? If so, generate a MEMA,
		   else generate a MEMB. */
		if (p->length & (~0xfff)) {
		    ADD_CODE(LDA_MEMB_G2);
		    /* The memb word: */
		    ADD_CODE(p->length);
		}
		else
			ADD_CODE(LDA_MEMA_G2 | p->length);
		/* Call to __copy (above) */
		ADD_CODE(CALL | (0x00ffffff & (COPY_OFFSET - text_sec->size)));
	    }
	}
	ADD_CODE( RET );
    }
    if (need_clear_bss) {
	char *start_sect_sym_name,*end_sect_sym_name;
	int idx = 0,zero_utility_offset = 20*number_of_named_bss_sections + 4;

	ADD_SYMBOL("___clear_named_bss_sections",text_sec->size,BSF_GLOBAL|BSF_EXPORT,text_sec);
	do {
	    named_bss_sect_name(&start_sect_sym_name,&end_sect_sym_name,idx++);
	    if (start_sect_sym_name) {
		if (data_is_pi)
			ADD_CODE( LDA_MEMB_G12_G0 );
		else
			ADD_CODE( LDA_MEMB_G0 );
		temp_asym = ADD_SYMBOL(start_sect_sym_name,0,BSF_UNDEFINED,(asection *) 0);
		ADD_RELOCATION(temp_asym,text_sec->size);
		ADD_CODE(0);
		if (data_is_pi)
			ADD_CODE( LDA_MEMB_G12_G1 );
		else
			ADD_CODE( LDA_MEMB_G1 );
		temp_asym = ADD_SYMBOL(end_sect_sym_name,0,BSF_UNDEFINED,(asection *) 0);
		ADD_RELOCATION(temp_asym,text_sec->size);
		ADD_CODE(0);
		zero_utility_offset -= 16;
		ADD_CODE(CALL | (0x00ffffff & zero_utility_offset));
		zero_utility_offset -= 4;
	    }
	} while (start_sect_sym_name);
	ADD_CODE( RET );
	if (number_of_named_bss_sections) {
	    ADD_SYMBOL("zero_g0_through_g1",text_sec->size,BSF_LOCAL,text_sec);
	    ADD_CODE( ST_G14_G0 );
	    ADD_CODE( ADDO_4_G0_G0 );
	    ADD_CODE( CMPOBL_G0_G1_DOT_M_8 );
	    ADD_CODE( RET );
	}
    }

    /* Add a trailing null symbol table entry: */
    ADD_SYMBOL(NULL,0,0,(asection*)0);
    objbfd->symcount--;
    bfd_set_symtab(objbfd, objbfd->outsymbols,&objbfd->symcount);
    if (!bfd_set_section_contents(objbfd,text_sec,code,0,text_sec->size))
	    abort();
    bfd_close( objbfd );
    for (i=0;i < text_sec->reloc_count;i++)
	    free(text_sec->orelocation[i]);
    free(text_sec->orelocation);
    for (i=0;i < curr_sym_tab_len;i++) {
	free((char *)objbfd->outsymbols[i]->name);
	free(objbfd->outsymbols[i]);
    }
    free(objbfd->outsymbols);
    free(text_sec);
    free(code);
    return objfile;
}

static lang_memory_region_type *ld960sym_memory_region_list_copy;

void
ld960sym_save_off_memory_regions()
{
    lang_memory_region_type *p,**q = &ld960sym_memory_region_list_copy,
    *r = (lang_memory_region_type *) 0;
    extern lang_memory_region_type *lang_memory_region_list;

    for (p=lang_memory_region_list;p;p = p->down) {
	(*q) = (lang_memory_region_type *) ldmalloc(sizeof(lang_memory_region_type));
	(*q)->up = r;
	(*q)->down = (lang_memory_region_type *) 0;
	r = (*q);

	(*q)->name = p->name;
	(*q)->origin = p->origin;
	(*q)->length_lower_32_bits = p->length_lower_32_bits;
	(*q)->length_high_bit = p->length_high_bit;
	(*q)->flags = p->flags;
	q = &((*q)->down);
    }
}

void
ld960sym_mark_sram_memory_regions_used()
{
    extern unsigned long ldfile_output_machine;
    int i;
    lang_memory_region_type *p,*q;

    if ((ldfile_output_machine == bfd_mach_i960_ca) ||
	(ldfile_output_machine == bfd_mach_i960_jx)) {
	/* Ultimate hokiness: */
	ldsym_type *fpem_CA_AC = ldsym_get_soft("fpem_CA_AC");

	if (fpem_CA_AC && fpem_CA_AC->sdefs_chain) {
	    asymbol *sym_def = *(fpem_CA_AC->sdefs_chain);

	    if (flag_is_absolute(sym_def->flags)) {
		if (address_available(sym_def->value,4,0,ld960sym_memory_region_list_copy)) {
		    if (!mark_memory_region_used(sym_def->value,4)) {
			info("Warning: Can not allocate fpem_CA_AC to address: %x, length: 4\n",
			     sym_def->value);
		    }
		}
	    }
	}
    }

    for (i=0;i < SRAM_VAR_HASH_SIZE;i++) {
	LIST *q,*p;

	for ( p = sram_var_hash_table[i]; p; p = q ) {

	    if (address_available(p->origin,p->length,0,ld960sym_memory_region_list_copy)) {
		if (!mark_memory_region_used(p->origin,p->length))
			info("Can not allocate ___sram.%d to address: %x, length: %d\n",
			     p->origin,p->origin,p->length);
	    }
	    q = p->next;
	    free(p);
	}
    }
    /* Free copy of memory region list. */
    for(p=ld960sym_memory_region_list_copy;p;) {
	q = p;
	p = p->down;
	free(q);
    }
    ld960sym_memory_region_list_copy = (lang_memory_region_type *) 0;
}

static void
add_to_sram_hash_table( name, length )
    CONST char *name;
    int length;
{
    /* We may be dealing with a common variable, so, only put in one reference to the
       sram variable into the hash table.  The linker itself will deal with the
       problem of multi-defs.  We will meld common sizes also below.
       */
    char *p;
    unsigned origin = strtol(name,&p,10),hash_key = (SRAM_VAR_HASH_SIZE-1)&(origin/4),found_it = 0;
    LIST **q;

    if (p && *p)
	    info("Can not convert: %s to unsigned (sram_init code).\n",name);

    for (q = &sram_var_hash_table[hash_key];!found_it && q && *q;)
	    if (!(found_it = (*q)->origin == origin))
		    q = &(*q)->next;
    /* If we did not find a definition of the variable in the hash table then ... */
    if (!found_it) {
	/* Addit to the hash table. */
	(*q) = (LIST *) ldmalloc(sizeof(LIST));
	(*q)->name = NULL;
	(*q)->origin = origin;
	(*q)->length = length;
	(*q)->next = 0;
	sram_var_count++;
    }
    else {
	/* This is either a multi def or a common variable.  Let's assume a common
	   as multi-defs are addressed in ldmain.c */
	if (length > (*q)->length)
		(*q)->length = length;
    }
}

unsigned ld960sym_sizeof_profile_table;

int
ld960sym_is_a_profile_counter(name)
    CONST char *name;
{
    return 0 == strncmp(name,PROF_CTR_SYM_PREFIX,PROF_CTR_SYM_PREFIX_LEN);
}

void
gnu960_enter_special_symbol( sym, sp, b )
    asymbol *sym;
    ldsym_type *sp;
    bfd *b;
{
    CONST char *name = sym->name;

    if ( ld960sym_is_a_profile_counter(name) ) {
	if ( flag_is_undefined(sym->flags) ){
	    return;
	}
	if ( !flag_is_common(sym->flags) ){
	    info("%Fprofile table '%s' not in common\n", name);
	    /* NO RETURN */
	}
	if ( sp->sdefs_chain || sp->scoms_chain ){
	    info( "%Fprofile table '%s' redeclared\n", name );
	    /* NO RETURN */
	}
	gnu960_add_profile_symbol (name, sym->value );
	ld960sym_sizeof_profile_table += sym->value;
	bfd_make_section(b,"PROFILE_COUNTER")->flags |= SEC_IS_BSS;

    } else if (!strncmp(name,SRAM_LEN_SYM_PREFIX,SRAM_LEN_SYM_PREFIX_LEN)){

	if ( !flag_is_absolute(sym->flags) ){
	    info( "%FSRAM length symbol '%s' not absolute\n", name);
	    /* NO RETURN */
	}
	add_to_sram_hash_table( name+SRAM_LEN_SYM_PREFIX_LEN, sym->value );
    }
}


/* Return non-zero iff passed string is name of a global linker-generated
 * symbol.
 */
int
linker_generated_symbol( s )
    char *s;
{
	char **p;

	for ( p=magic_names; *p; p++ ){
		if ( !strcmp(*p,s) ){
			return 1;
		}
	}
	return 0;
}
