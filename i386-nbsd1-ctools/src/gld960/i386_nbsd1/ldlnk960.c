/* Copyright (C) 1991 Free Software Foundation, Inc.
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
 */

/* $Id: ldlnk960.c,v 1.56 1995/06/30 23:07:10 paulr Exp $ */

#include "sysdep.h"
#include "bfd.h"
#include "ld.h"
#include "config.h"
#include "ldemul.h"
#include "ldmisc.h"
#include "ldlang.h"
#include "ldmain.h"
#include "ldfile.h"

extern boolean lang_float_flag;
extern boolean data_is_pi;
extern bfd *output_bfd;
extern enum bfd_architecture ldfile_output_architecture;
extern unsigned long ldfile_output_machine,ldfile_real_output_machine;
extern char *ldfile_output_machine_name;
extern HOW_INVOKED invocation;
extern int host_byte_order_is_big_endian;

typedef struct lib_list {
	char *name;
	lang_input_file_enum_type search_type;
	struct lib_list *next;
} lib_list_type;

static lib_list_type *hll_list;
static lib_list_type **hll_list_tail = &hll_list;

static lib_list_type *syslib_list;
static lib_list_type **syslib_list_tail = &syslib_list;

static char *clib, *mlib, *flib;
static char clib_is_l_1,mlib_is_l_1,flib_is_l_1;

static void
append(list, name, search_type)
    lib_list_type ***list;
    char *name;
    lang_input_file_enum_type search_type;
{
	lib_list_type *element = 
	    (lib_list_type *)(ldmalloc(sizeof(lib_list_type)));

	element->name = name;
	element->search_type = search_type;
	element->next = (lib_list_type *)NULL;
	**list = element;
	*list = &element->next;
}

static boolean had_hll = false;
static boolean had_hll_name = false;

void
lnk960_hll(name)
char *name;
{
    had_hll = true;
    if (name) {
	had_hll_name = true;
	if (USE_OLD_SEARCH_RULES) {
	    append(&hll_list_tail,name,(invocation == as_gld960) ? lang_input_file_is_l_enum :
		   lang_input_file_is_search_file_enum);
	}
	else
		append(&hll_list_tail,name,lang_input_file_is_l_enum);
    }
}

void
lnk960_syslib(name)
char *name;
{
    if (name && name[strlen(name)-1] == '*') {
	name[strlen(name)-1] = 0;
	append(&syslib_list_tail,name,lang_input_file_is_magic_syslib_and_startup_enum);
    }
    else
	    append(&syslib_list_tail,name,lang_input_file_is_search_file_enum);
}

static void
lnk960_before_parse()
{
	extern char *lib_subdir;

	if (invocation == as_gld960)
	{
		extern int target_byte_order_is_big_endian;
		if (target_byte_order_is_big_endian)
			lib_subdir = "/libcfbe";
		else
			lib_subdir = "/libcoff";
	}
        else
		lib_subdir = "";
	ldfile_output_architecture = bfd_arch_i960;
}

static void
add_on(list)
    lib_list_type *list;
{
    while (list) {
	lang_add_input_file(list->name, list->search_type, (char *)NULL,
			    (bfd *)0,-1,-1);
	list = list->next;
    }
}

void
lnk960_after_parse()
{
    char *gnu_c_lib = "cg",*gnu_m_lib = "mg",*gnu_h_lib = "hg";

    if (USE_NEW_SEARCH_RULES) {
	gnu_c_lib = "c";
	gnu_m_lib = "m";
	gnu_h_lib = "h";
    }
    ldemul_add_searchpaths();
    /* if there has been no hll list then add our own */
    if(had_hll && !had_hll_name) {
	if (invocation == as_gld960) {
	    append(&hll_list_tail,gnu_c_lib,lang_input_file_is_l_enum);
	    if (lang_float_flag) {
		append(&hll_list_tail,gnu_m_lib,lang_input_file_is_l_enum);
	    }
	    else {
		if (USE_NEW_SEARCH_RULES)
			append(&hll_list_tail,"libmstb",
			       lang_input_file_is_search_file_add_suffixes_enum);
		else
			append(&hll_list_tail,"libmstub.a",
			       lang_input_file_is_search_file_enum);
	    }
	    if (ldfile_output_machine == bfd_mach_i960_ka_sa
		||  ldfile_output_machine == bfd_mach_i960_ca
		||  ldfile_output_machine == bfd_mach_i960_hx
		||  ldfile_output_machine == bfd_mach_i960_jx) {
		append(&hll_list_tail,gnu_h_lib,lang_input_file_is_l_enum);
	    }
	}
	else {
	    if (USE_NEW_SEARCH_RULES) {
		append(&hll_list_tail,"c",lang_input_file_is_l_enum);
		if (lang_float_flag) {
		    append(&hll_list_tail,"m",lang_input_file_is_l_enum);
		}
		else
			append(&hll_list_tail,"libmstb",lang_input_file_is_search_file_add_suffixes_enum);
		if (ldfile_output_machine == bfd_mach_i960_ka_sa
		    ||  ldfile_output_machine == bfd_mach_i960_ca
		    ||  ldfile_output_machine == bfd_mach_i960_hx
		    ||  ldfile_output_machine == bfd_mach_i960_jx) {
		    append(&hll_list_tail,"h",lang_input_file_is_l_enum);
		}
	    }
	    else {
		build_hll_library_names();
		append(&hll_list_tail,clib, clib_is_l_1 ? lang_input_file_is_l_1_enum :
		       lang_input_file_is_search_file_enum);
		append(&hll_list_tail,mlib, mlib_is_l_1 ? lang_input_file_is_l_1_enum :
		       lang_input_file_is_search_file_enum);
		if (*flib)
			append(&hll_list_tail,flib, flib_is_l_1 ? lang_input_file_is_l_1_enum :
			       lang_input_file_is_search_file_enum);
	    }
	}
    }
    else if (!had_hll && lang_float_flag) {
	/* This is a pathological case where the user has, for
	   reasons of her own, handed us a script file containing
	   a FLOAT directive with no HLL() directive. To do what
	   R3.5 did, we include the math and floating point
	   libraries. */
	if (invocation == as_gld960) {
	    append(&hll_list_tail,gnu_m_lib,lang_input_file_is_l_enum);
	    if (ldfile_output_machine == bfd_mach_i960_ka_sa
		||  ldfile_output_machine == bfd_mach_i960_ca
		||  ldfile_output_machine == bfd_mach_i960_hx
		||  ldfile_output_machine == bfd_mach_i960_jx) {
		append(&hll_list_tail,gnu_h_lib,lang_input_file_is_l_enum);
	    }
	}
	else {
	    if (USE_NEW_SEARCH_RULES) {
		append(&hll_list_tail,"m",lang_input_file_is_l_enum);
		if (ldfile_output_machine == bfd_mach_i960_ka_sa
		    ||  ldfile_output_machine == bfd_mach_i960_ca
		    ||  ldfile_output_machine == bfd_mach_i960_hx
		    ||  ldfile_output_machine == bfd_mach_i960_jx)
			append(&hll_list_tail,"h",lang_input_file_is_l_enum);
	    }
	    else {
		build_hll_library_names();
		append(&hll_list_tail,mlib,mlib_is_l_1 ? lang_input_file_is_l_1_enum :
		       lang_input_file_is_search_file_enum);
		if (*flib)
			append(&hll_list_tail,flib,flib_is_l_1 ? lang_input_file_is_l_1_enum :
			       lang_input_file_is_search_file_enum);
	    }
	}
    }
    /* Under gld960, the HLL names are only the "kernel" strings,
       e.g., "cg". Under lnk960, they are the full file name and should
       not be used as if they were just "kernels".  The second argument
       to add_on specifies this characteristic of the file name. */
    add_on(hll_list);
    add_on(syslib_list);
}

void
lnk960_set_output_arch()
{
    bfd_set_arch_mach(output_bfd, ldfile_output_architecture,
		      ldfile_output_machine,
		      ldfile_real_output_machine);
}

static char *
lnk960_choose_target()
{

    if (invocation == as_lnk960) {
	short big_probe = 0x0100;

	host_byte_order_is_big_endian =

	/* The following expression evaluates to 1 on big endian hosts,
	   and 0 on little endian hosts: */

		((char *) &big_probe)[0] == (char) 0x01;
    }
    /* If host_is_big_endian != 0, this directs bfd_make_targ_name() to
	make big endian output, else it emits little endian output. */

#if !defined(NO_BIG_ENDIAN_MODS)
    return bfd_make_targ_name(BFD_COFF_FORMAT,host_byte_order_is_big_endian,0);
#else
    return bfd_make_targ_name(BFD_COFF_FORMAT,host_byte_order_is_big_endian);
#endif /* NO_BIG_ENDIAN_MODS */
}

/* The default script if none is offered */
static char lnk960_script[] = 
"SECTIONS {                           \
	.text : {                     \
		*(.text)              \
	}                             \
        .data : {                     \
	        *(.data)              \
        }                             \
        .bss  : {                     \
	        *(.bss)               \
	        [COMMON]              \
        }                             \
}";


static char *
lnk960_get_script()
{
	return lnk960_script;
}

build_hll_library_names()
{
    char mach_name[3];

    strcpy(mach_name,ldfile_output_machine_name);
    /* Set aside enough space to write out the names of the default
       libraries. NOTE!!! WARNING!!! HARD-CODED CONSTANTS IN CODE!!!!*/
    clib = (char *)ldmalloc(11);
    mlib = (char *)ldmalloc(11);
    flib = (char *)ldmalloc(9);
    strcpy(clib,"libc");
    strcpy(mlib,"libm");
    /* We may not need to put anything into flib */
    *flib = '\0';
    if (ldfile_output_machine == bfd_mach_i960_jx ||
	ldfile_output_machine == bfd_mach_i960_hx)
	    clib_is_l_1 = 1;
    else {
	strcat(clib,mach_name);
	if (data_is_pi)
		strcat(clib,"_p");
	strcat(clib,".a");
    }
    if (lang_float_flag) {
	if (ldfile_output_machine == bfd_mach_i960_jx ||
	    ldfile_output_machine == bfd_mach_i960_hx)
		mlib_is_l_1 = 1;
	else {
	    strcat(mlib,mach_name);
	    if (data_is_pi)
		    strcat(mlib,"_p");
	    strcat(mlib,".a");
	}
	if ((ldfile_output_machine == bfd_mach_i960_ca) ||
	    (ldfile_output_machine == bfd_mach_i960_hx) ||
	    (ldfile_output_machine == bfd_mach_i960_ka_sa) ||
	    (ldfile_output_machine == bfd_mach_i960_jx)) {
	    strcpy(flib,"libh");
	    if (ldfile_output_machine == bfd_mach_i960_jx ||
		ldfile_output_machine == bfd_mach_i960_hx)
		    flib_is_l_1 = 1;
	    else {
		strcat(flib,mach_name);
		strcat(flib,".a");
	    }
	}
    }
    else
	    strcat(mlib,"stub.a");
    return;
}

static ldemul_search_dir_type lnk960_searchdirs[] = {
    {"I960LIB", NULL,1,0},
    {"I960LLIB",NULL,1,0},
    {"I960BASE",NULL,1,1},
    {".",       NULL,0,0}
};

struct ld_emulation_xfer_struct ld_lnk960_emulation = {
	lnk960_before_parse,
	lnk960_syslib,
	lnk960_hll,
	lnk960_after_parse,
	lnk960_set_output_arch,
	lnk960_choose_target,
	lnk960_get_script,
	{lnk960_searchdirs,lnk960_searchdirs,lnk960_searchdirs},
	{3,3,3}
};
