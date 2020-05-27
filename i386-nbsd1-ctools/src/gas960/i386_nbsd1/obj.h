/* obj.h - defines the object dependent hooks for all object
   format backends.

   Copyright (C) 1987, 1990, 1991 Free Software Foundation, Inc.

This file is part of GAS, the GNU Assembler.

GAS is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GAS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GAS; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* static const char rcsid[] = "$Id: obj.h,v 1.7 1993/11/06 02:24:08 peters Exp $"; */

#ifdef __STDC__

void obj_crawl_symbol_chain(object_headers *headers);
void obj_emit_relocations(fixS *fixP, unsigned long seg_addr);
void obj_emit_strings(void);
void obj_emit_symbols(symbolS *symbol_rootP, object_headers *headers);
void obj_header_append(object_headers *headers);
void obj_read_begin_hook(void);
void obj_symbol_new_hook(symbolS *symbolP);
void obj_symbol_to_chars(symbolS *symbolP);

#ifndef obj_pre_write_hook
void obj_pre_write_hook(object_headers *headers);
#endif /* obj_pre_write_hook */

#else

void obj_crawl_symbol_chain();
void obj_emit_relocations();
void obj_emit_strings();
void obj_emit_symbols();
void obj_header_append();
void obj_read_begin_hook();
void obj_symbol_new_hook();
void obj_symbol_to_chars();

#ifndef obj_pre_write_hook
void obj_pre_write_hook();
#endif /* obj_pre_write_hook */

#endif /* __STDC__ */

extern const pseudo_typeS obj_pseudo_table[];

/*
 * Local Variables:
 * comment-column: 0
 * fill-column: 131
 * End:
 */

/* end of obj.h */
