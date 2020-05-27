/*
 * Copyright (C) 1991 Free Software Foundation, Inc.
 *
 * This file is part of ?GLD, the Gnu Linker.
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

PROTO(void,ldemul_hll,(char *));
PROTO(void,ldemul_syslib,(char *));
PROTO(void,ldemul_after_parse,(void));
PROTO(void,ldemul_before_parse,(void));
PROTO(void,ldemul_set_output_arch,(void));
PROTO(char,*ldemul_choose_target,(void));
PROTO(void,ldemul_choose_mode,(void));
PROTO(char *,ldemul_get_script,(void));
PROTO(char *,ldemul_get_searchdir,(int,int*));
PROTO(void,ldemul_add_searchpaths,(void));

typedef struct {
    char *pseudoname,*realname;
    int queryenv,addlib;
} ldemul_search_dir_type;

typedef struct ld_emulation_xfer_struct {
	SDEF(void,before_parse,(void));
	SDEF(void,syslib,(char *));
	SDEF(void ,hll,(char *));
	SDEF(void,after_parse,(void));
	SDEF(void,set_output_arch,(void));
	SDEF(char *,choose_target,(void));
	SDEF(char *,get_script,(void));
	ldemul_search_dir_type *search_dirs[3];  /* [0] is the old,
						    [1] is the new,
						    and [2] is the old and new */
	int search_dir_max[3];   /* Ibid. */
} ld_emulation_xfer_type;

typedef enum {
	intel_ic960_ld_mode_enum,
	intel_gld960_ld_mode_enum,
	default_mode_enum
} lang_emulation_mode_enum_type;
