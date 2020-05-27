/*
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
 */

#include "ldmain.h"

enum filelist_node_type {ldfile,objfile,
				 dash_r_file,archive_member,outputfile,
				 archivefile,LAST_FILELIST_NODE_TYPE};

PROTO(void,add_file_to_filelist,(enum filelist_node_type,char *,char *,char *));
PROTO(void,ldfile_add_arch,(CONST char *CONST));
PROTO(void,ldfile_add_library_path,(char *,int));
PROTO(FILE *,ldfile_find_command_file,(command_file_list *,char **));
PROTO(void,ldfile_parse_command_file,(command_file_list *));
PROTO(void,ldfile_open_file,(struct lang_input_statement_struct *));
PROTO(void,ldfile_set_output_arch,(CONST char *));
