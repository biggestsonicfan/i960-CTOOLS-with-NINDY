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

/* $Id: ldgld960.c,v 1.36 1995/10/05 16:44:00 paulr Exp $ */

#include "sysdep.h"
#include "bfd.h"
#include "ld.h"
#include "config.h"
#include "ldemul.h"
#include "ldmisc.h"
#include "ldmain.h"
#include "ldfile.h"


/* IMPORTS */
extern char *output_filename;
extern boolean lang_float_flag;
extern enum bfd_architecture ldfile_output_architecture;
extern unsigned long ldfile_output_machine,ldfile_real_output_machine;
extern char *ldfile_output_machine_name;
extern bfd *output_bfd;
extern int host_byte_order_is_big_endian;

static void
gld960_before_parse()
{
	extern char *lib_subdir;
	extern int target_byte_order_is_big_endian;
	extern enum target_flavour_enum output_flavor;

	if (output_flavor == BFD_COFF_FORMAT) {
	    if (target_byte_order_is_big_endian)
		    lib_subdir = "/libcfbe";
	    else
		    lib_subdir = "/libcoff";
	}
	else
		lib_subdir = "/libbout";
	ldfile_output_architecture = bfd_arch_i960;
}

#if 0
static void
gld960_set_output_arch()
{
    if (BFD_ELF_FILE_P(output_bfd))
	    bfd_set_arch_mach(output_bfd, ldfile_output_architecture,
			      ldfile_output_machine, ldfile_real_output_machine);
    else /* Itsa bout file. */
	    bfd_set_arch_mach(output_bfd, ldfile_output_architecture,
			      bfd_mach_i960_core, ldfile_real_output_machine);
}
#endif

static char *
gld960_choose_target()
{
    extern enum target_flavour_enum output_flavor;

    if (output_flavor == BFD_BOUT_FORMAT)
	    output_filename = "b.out";
    else if (output_flavor == BFD_COFF_FORMAT)
	    output_filename = "a.out";
    else
	    output_filename = "e.out";

    host_byte_order_is_big_endian = 0;	/* always put out b.out or elf superstructure in
					   little-endian format */
#if !defined(NO_BIG_ENDIAN_MODS)
    return bfd_make_targ_name(output_flavor,0,0);
#else
    return bfd_make_targ_name(output_flavor,0);
#endif /* NO_BIG_ENDIAN_MODS */
}

static char script[] = 
"SECTIONS        \
{                \
  .text : {      \
	*(.text) \
  }              \
  .data : {      \
	*(.data) \
  }              \
  .bss  : {      \
	*(.bss)	 \
	[COMMON] \
   }             \
}";


static char *
gld960_get_script()
{
	return script;
}

static ldemul_search_dir_type gld960_old_searchdirs[] = {
    {".",       NULL,0,0},
    {"G960LIB", NULL,1,0},
    {"G960BASE",NULL,1,1},
    {"I960BASE",NULL,1,1},
};

static ldemul_search_dir_type gld960_new_searchdirs[] = {
    {"G960LIB", NULL,1,0},
    {"G960LLIB",NULL,1,0},
    {"G960BASE",NULL,1,1},
    {".",       NULL,0,0},
};

static ldemul_search_dir_type gld960_both_old_and_new_searchdirs[] = {
    {".",       NULL,0,0},
    {"G960LIB", NULL,1,0},
    {"G960BASE",NULL,1,1},
    {"I960BASE",NULL,1,1},
    {"G960LLIB",NULL,1,0},
};

extern void lnk960_syslib(),lnk960_hll(),lnk960_after_parse(),lnk960_set_output_arch();

struct ld_emulation_xfer_struct ld_gld960_emulation = 
{
	gld960_before_parse,
	lnk960_syslib,
	lnk960_hll,
	lnk960_after_parse,
	lnk960_set_output_arch,
	gld960_choose_target,
	gld960_get_script,
	{gld960_old_searchdirs,	gld960_new_searchdirs,	gld960_both_old_and_new_searchdirs },
	{3,3,4}
};
