
#ifndef __LDSYM_H_

#define __LDSYM_H_

#include "ld.h"


/* ldsym.h -

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

typedef struct user_symbol_struct
{
  /* Point to next symbol in this hash chain */
  struct user_symbol_struct *link;

  /* Name of this symbol.  */
  CONST char *name;			

  /* Pointer to next symbol in order of symbol creation */
  struct user_symbol_struct *next; 

  /* Chain of asymbols we see from input files 
     note that we point to the entry in the canonical table of 
     the pointer to the asymbol, *not* the asymbol. This means
     that we can run back and fix all refs to point to the
     defs nearly for free.
     */
  asymbol **srefs_chain;
  asymbol **sdefs_chain;

  /* only ever point to the largest ever common definition -
   * all the rest are turned into refs 
   * scoms and sdefs are never != NULL at same time
   */
  asymbol **scoms_chain;

#ifdef GNU960
  /* Non-zero iff this symbol is ever defined or referenced by from a file
   * that does *not* contain a cc info block (data for 2-pass compiler
   * optimization).  This, of course, would include symbols that are not
   * from a genuine object file, e.g., defined in a script or on the
   * invocation line.
   */
  char non_ccinfo_ref;
#endif
  /* Set true if this is a definition from a command line -defsym switch */
  boolean defsym_flag;
  /* Set true if this is a definition from an assignment statement in a linker
     directive file. */
  boolean assignment_flag;
  /* If either of the above booleans are set, then this corresponds to the 
     phase it was last defined in.  This will enable emitting a warning for
     multiple definitions of absolute symbols in script files. */
  lang_phase_type assignment_phase;
} ldsym_type;


PROTO(ldsym_type *, ldsym_get, (CONST char *));
PROTO(ldsym_type *, ldsym_get_soft, (CONST char *));
PROTO(void, ldsym_print_symbol_table,(void));
PROTO(void, ldsym_write, (void));
PROTO(boolean, ldsym_undefined, (CONST char *));
#define FOR_EACH_LDSYM(x)						\
	extern ldsym_type *symbol_head;					\
	ldsym_type *x;							\
	for (x = symbol_head; x != (ldsym_type *)NULL; x = x->next) 	


#endif
