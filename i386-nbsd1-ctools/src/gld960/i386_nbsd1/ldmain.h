/* ldmain.h -

   Copyright (C) 1991 Free Software Foundation, Inc.

   This file is part of GLD, the Gnu Linker.

   GLD is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   GLD is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GLD; see the file COPYING.  If not, write to
   the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef __LDMAIN_H__

#define  __LDMAIN_H__

#include "ld.h"

PROTO(void, Q_enter_global_ref,(asymbol **,int,lang_phase_type));
PROTO(void, ldmain_open_file_read_symbol,(struct lang_input_statement_struct *));

typedef struct command_file_list_node {
    char *filename;
    boolean named_with_T;
} command_file_list;

typedef enum {as_gld960, as_lnk960, as_nobody} HOW_INVOKED;

#endif
