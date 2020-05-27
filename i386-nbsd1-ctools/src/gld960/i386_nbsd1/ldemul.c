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

/*
 * $Id: ldemul.c,v 1.12 1995/09/06 21:41:56 paulr Exp $ 
 */

/*
 * clearing house for ld emulation states 
 */

#include "sysdep.h"
#include "bfd.h"
#include "config.h"
#include "ld.h"
#include "ldemul.h"
#include "ldmisc.h"

extern ld_emulation_xfer_type ld_lnk960_emulation;
extern ld_emulation_xfer_type ld_gld960_emulation;

static ld_emulation_xfer_type *ld_emulation;

void
ldemul_hll(name)
    char *name;
{
	ld_emulation->hll(name);
}


void
ldemul_syslib(name)
    char *name;
{
	ld_emulation->syslib(name);
}

void
ldemul_after_parse()
{
	ld_emulation->after_parse();
}

void
ldemul_before_parse()
{
	ld_emulation->before_parse();
}

void
ldemul_set_output_arch()
{
	ld_emulation->set_output_arch();
}

char *
ldemul_choose_target()
{
	return ld_emulation->choose_target();
}

char *
ldemul_get_script()
{
	return ld_emulation->get_script();
}

char *
ldemul_get_searchdir(search_index,indexok)
	int search_index,*indexok;
{
    char *p;

    if (search_index < 0 || search_index > ld_emulation->search_dir_max[linker_search_rules]) {
	*indexok = 0;
	return NULL;
    }
    *indexok = 1;
    if (ld_emulation->search_dirs[linker_search_rules][search_index].realname)
	    return ld_emulation->search_dirs[linker_search_rules][search_index].realname;
    if (!ld_emulation->search_dirs[linker_search_rules][search_index].queryenv)
	    p = ld_emulation->search_dirs[linker_search_rules][search_index].pseudoname;
    else {
	if (!(p = (char *)getenv(ld_emulation->search_dirs[linker_search_rules][search_index].pseudoname)))
		return NULL;
    }
    if (ld_emulation->search_dirs[linker_search_rules][search_index].addlib)
	    p = concat(p,"/lib","");
    return ld_emulation->search_dirs[linker_search_rules][search_index].realname = buystring(p);
}

#include "ldmain.h"

void
ldemul_choose_mode()
{
    extern HOW_INVOKED invocation;

    if (invocation == as_lnk960)
	    ld_emulation = &ld_lnk960_emulation;
    else
	    ld_emulation = &ld_gld960_emulation;
}

void
ldemul_add_searchpaths()
{
    char *next_dir ;
    int search_index,searchindexok=1;

    search_index = 0;
    while (searchindexok)
	    if (next_dir = ldemul_get_searchdir(search_index++,&searchindexok))
		    ldfile_add_library_path(next_dir,0);
}
