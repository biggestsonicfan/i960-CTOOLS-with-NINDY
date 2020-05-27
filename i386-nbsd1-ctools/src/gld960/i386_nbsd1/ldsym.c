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

/* $Id: ldsym.c,v 1.23 1996/01/08 21:13:35 paulr Exp $ */

/* 
 * All symbol handling for the linker
 *	Written by Steve Chamberlain steve@cygnus.com
 */

#include "sysdep.h"
#include "bfd.h"
#include "ld.h"
#include "ldsym.h"
#include "ldmisc.h"
#include "ldlang.h"

extern bfd *output_bfd;
extern strip_symbols_type strip_symbols;
extern discard_locals_type discard_locals;

/*
 * Head and tail of global symbol table chronological list
 */
ldsym_type *symbol_head = (ldsym_type *)NULL;
ldsym_type **symbol_tail_ptr = &symbol_head;

/*
 * Incremented for each symbol in the ldsym_type table
 * no matter what flavour it is 
 */
unsigned int global_symbol_count;

#define	TABSIZE	1009
static ldsym_type *global_symbol_hash_table[TABSIZE];


/* Compute the hash code for symbol name KEY.
 */
static int
hash_string (key)
    CONST char *key;
{
	register CONST char *cp;
	register int k;

	cp = key;
	k = 0;
	while (*cp){
		k = (((k << 1) + (k >> 14)) ^ (*cp++)) & 0x3fff;
	}
	return k;
}


/*
 * Get the symbol table entry for the global symbol named KEY.
 * Create one if there is none.
 */
ldsym_type *
DEFUN(ldsym_get,(key),
    CONST char *key)
{
	register int hashval;
	register ldsym_type *bp;

	/* Find and search the proper bucket */
	hashval = hash_string (key) % TABSIZE;
	for (bp = global_symbol_hash_table[hashval]; bp; bp = bp->link) {
		if (! strcmp (key, bp->name)) {
			return bp;
		}
	}

	/* Nothing was found; create a new symbol table entry. */

	bp = (ldsym_type *) ldmalloc ((bfd_size_type)(sizeof (ldsym_type)));
	bzero( bp, sizeof(*bp) );
	bp->name = buystring(key);

	/* Add the entry to the bucket. */

	bp->link = global_symbol_hash_table[hashval];
	global_symbol_hash_table[hashval] = bp;

	/* Keep the chronological list up to date too */
	*symbol_tail_ptr = bp;
	symbol_tail_ptr = &bp->next;
	bp->next = 0;
	bp->defsym_flag = false;
	bp->assignment_flag = false;
	bp->assignment_phase = lang_first_phase_enum;
	global_symbol_count++;

	return bp;
}


/*
 * Like `ldsym_get' but return 0 if the symbol is not already known.
 */
ldsym_type *
DEFUN(ldsym_get_soft,(key),
    CONST char *key)
{
	register int hashval;
	register ldsym_type *bp;

	/* Find and search the bucket. */
	hashval = hash_string (key) % TABSIZE;
	for (bp = global_symbol_hash_table[hashval]; bp; bp = bp->link) {
		if (! strcmp (key, bp->name)) {
			return bp;
		}
	}
	return 0;
}


static void
list_file_locals (entry)
    lang_input_statement_type *entry;
{
	asymbol **q;

	fputs ( "\nLocal symbols of ",stdout);
	fprintf(stdout,"%s",entry->local_sym_name);
	fputs (":\n",stdout);
	if (entry->asymbols) {
		for (q = entry->asymbols; *q; q++) {
			asymbol *p = *q;
			/* If this is a definition, update it if
			 * necessary by this file's start address.
			 */
			if (p->flags & BSF_LOCAL) {
				fprintf(stdout,"  0x%lx %s\n",p->value, p->name);
			}
		}
	}
}


static asymbol **
write_file_locals(output_buffer)
    asymbol **output_buffer;
{
	extern lang_output_section_statement_type *create_object_symbols;
	extern lang_input_statement_type *script_file;
	extern char lprefix;
	extern lang_statement_list_type file_chain;

	asymbol *newsym;
	unsigned int i;
	asection *s;
	lang_input_statement_type *stmt;

	/* Run through the symbols and work out what to do with them.
	 * Add one for the filename symbol if needed.
	 */

	for (	stmt = (lang_input_statement_type *)file_chain.head;
		stmt;
		stmt = (lang_input_statement_type *)stmt->next)
	{
		if (create_object_symbols && stmt != script_file) {
			for (s = stmt->the_bfd->sections; s; s = s->next) {
				if (s->output_section == create_object_symbols->bfd_section) {
					/* Add symbol to this section */
					newsym = (asymbol *)bfd_make_empty_symbol(stmt->the_bfd);
					newsym->name = stmt->local_sym_name;

					/*
					 * The symbol belongs to the output
					 * file's text section. The value is
					 * the start of this section in the
					 * output file.
					 */
					newsym->value = 0;
					newsym->flags = BSF_LOCAL;
					newsym->section = s;
					*output_buffer++ = newsym;
					break;
				}
			}
		}

		for (i = 0; i < stmt->symbol_count; i++) {
		    asymbol *p = stmt->asymbols[i];

		    if (flag_is_global(p->flags)) {
			/* We are only interested in outputting globals
			 * at this stage in special circumstances
			 */
			if (p->the_bfd == stmt->the_bfd 
			    && flag_is_not_at_end(p->flags)) {
			    /* And this is one of them */
			    p->flags |= BSF_KEEP;
			    *output_buffer++ = p;
			}
		    }
		    else if (flag_is_absolute(p->flags)) {
			p->flags |= BSF_KEEP;
			*output_buffer++ = p;
		    }
		    else if (p->flags & BSF_XABLE) {
			if (discard_locals == DISCARD_ALL) {   /* -x option overrules -S and -X. */
			    ;
			}
			else if ((discard_locals == DISCARD_L) &&
				 (!flag_is_debugger(p->flags)) &&  /* Eliminates .bf/.ef/.bb/.eb */
				 ((p->name[0] == lprefix) ||
				     (p->name[0] == '.'))) {
			    ;
			} else if (p->flags == BSF_WARNING) {
			    ;
			} else if (flag_is_debugger(p->flags)) {
			    if (strip_symbols == STRIP_NONE)
				    *output_buffer++ = p;
			}
			else if (p->flags & BSF_LOCAL) {
				*output_buffer++ = p;
			    }
			else {
			    FAIL();
			}
		    }
		    else if (flag_is_debugger(p->flags)) {
			/* We deliberately do not consider strip_symbols here for COFF.
			 Coff's symbol table does not make a lot of sense unless at least
			 we have .file symbols in it. These are the only class of symbols
			 in coff that are debug but not XABLE. */
			*output_buffer++ = p;
		    }
		    else if (flag_is_undefined(p->flags)) {
			/* This must be global */
		    } else if (flag_is_common(p->flags)) {
			/* And so must this */
		    } else if (p->flags & BSF_CTOR) {
			/* Throw it away */
		    } else if (p->flags & BSF_LOCAL) {
			*output_buffer++ = p;
		    }
		    else {
			FAIL();
		    }
		}
	    }
	return output_buffer;
}

extern ld_config_type config;

static asymbol **
write_file_globals(symbol_table)
asymbol **symbol_table;
{
    int three_times_through_list;

    for (three_times_through_list=0;three_times_through_list < 3;three_times_through_list++) {
	FOR_EACH_LDSYM(sp) {
	    switch (three_times_through_list) {
	case 0:   /* Emit global defineds that are not common. */
		if (sp->sdefs_chain != (asymbol **)NULL) {
		    asymbol *bufp = (*(sp->sdefs_chain));

		    if ((bufp->flags & BSF_KEEP) ==0) {
			ASSERT(bufp != (asymbol *)NULL);

			bufp->name = sp->name;

			if (sp->scoms_chain == (asymbol **)NULL) {
			    bufp = *(sp->sdefs_chain);
			    *symbol_table++ = bufp;
			}
		    }
		}
		break;
	case 1:  /* Emit global defineds that are common. */
		if (sp->scoms_chain != (asymbol **)NULL) {
		    asymbol *bufp = (*(sp->scoms_chain));

		    *symbol_table++ = bufp;
		}
		break;
	case 2: /* Emit referenced undefineds. */
		if (sp->srefs_chain != (asymbol **)NULL && sp->sdefs_chain == (asymbol **)NULL &&
		    sp->scoms_chain == (asymbol **)NULL) {
		    asymbol *bufp = (*(sp->srefs_chain));

		    *symbol_table++ = bufp;
		}
		break;
	    }
	}
    }
    return symbol_table;
}


void
ldsym_write()
{
	extern unsigned int total_files_seen;
	extern unsigned int total_symbols_seen;

	if (strip_symbols != STRIP_ALL) {
		/* We know the maximum size of the symbol table -
		 * it's the size of all the global symbols ever seen +
		 * the size of all the symbols from all the files +
		 * the number of files (for the per file symbols)
		 * +1 (for the null at the end)
		 */

		asymbol **symbol_table = (asymbol **)
			    ldmalloc ((bfd_size_type)(global_symbol_count
				+ total_files_seen
				+ total_symbols_seen + 1) * sizeof(asymbol*));
		asymbol ** tablep = write_file_locals(symbol_table);
		tablep = write_file_globals(tablep);

		*tablep = (asymbol *)NULL;
		if (1) {
		    unsigned n = (unsigned)(tablep - symbol_table),i;

		    symbol_table = bfd_set_symtab(output_bfd, symbol_table,&n);

		    if (!config.relocateable_output)
			    for (i=0;i < n;i++)
				    if (symbol_table[i]->flags & BSF_UNDEFINED)
					    info("%P: warning: emitting undefined symbol %s to output file: %B\n",symbol_table[i]->name,output_bfd);
		}
					
	}
	else {
	    unsigned n = 0;

	    bfd_set_symtab(output_bfd, (asymbol **) 0, &n);
	}
}


/*
 * Return true if the supplied symbol name is not in the 
 * linker symbol table
 */
boolean 
DEFUN(ldsym_undefined,(sym),
CONST char *sym)
{
	ldsym_type *from_table = ldsym_get_soft(sym);

	if (from_table) {
		return !(from_table->sdefs_chain);
	}
	return true;
}
