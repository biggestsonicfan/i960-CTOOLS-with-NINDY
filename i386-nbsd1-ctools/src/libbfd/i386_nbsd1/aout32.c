/* BFD semi-generic back-end for a.out binaries */

/* Copyright (C) 1990, 1991 Free Software Foundation, Inc.

This file is part of BFD, the Binary File Diddler.

BFD is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

BFD is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with BFD; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */


#define ARCH_SIZE 32

#include "sysdep.h"
#include "bfd.h"
#include "bout.h"
#include "libaout.h"
#include "libbfd.h"
#include "stab.h"
#include "ar.h"

#define EXEC_BYTES_SIZE	sizeof(struct exec)

extern bfd_error_vector_type bfd_error_vector;

void
DEFUN(aout_swap_exec_header_in,(abfd, bytes, execp),
      bfd *abfd AND
      struct exec *bytes AND
      struct exec *execp)
{
  /* Now fill in fields in the execp, from the bytes in the raw data.  */
  execp->a_magic  = GETWORD (abfd, bytes->a_magic);
  execp->a_text   = GETWORD (abfd, bytes->a_text);
  execp->a_data   = GETWORD (abfd, bytes->a_data);
  execp->a_bss    = GETWORD (abfd, bytes->a_bss);
  execp->a_syms   = GETWORD (abfd, bytes->a_syms);
  execp->a_entry  = GETWORD (abfd, bytes->a_entry);
  execp->a_trsize = GETWORD (abfd, bytes->a_trsize);
  execp->a_drsize = GETWORD (abfd, bytes->a_drsize);
}


void
DEFUN(aout_swap_exec_header_out,(abfd, execp, bytes),
     bfd *abfd AND
     struct exec *execp AND 
     struct exec *bytes)
{
  /* Now fill in fields in the raw data, from the fields in the exec struct. */
  PUTWORD (abfd, execp->a_magic , bytes->a_magic);
  PUTWORD (abfd, execp->a_text  , bytes->a_text);
  PUTWORD (abfd, execp->a_data  , bytes->a_data);
  PUTWORD (abfd, execp->a_bss   , bytes->a_bss);
  PUTWORD (abfd, execp->a_syms  , bytes->a_syms);
  PUTWORD (abfd, execp->a_entry , bytes->a_entry);
  PUTWORD (abfd, execp->a_trsize, bytes->a_trsize);
  PUTWORD (abfd, execp->a_drsize, bytes->a_drsize);
}

struct container {
    struct aoutdata a;
    struct exec e;
};

/* Some A.OUT variant thinks that the file whose format we're checking
   is an a.out file.  Do some more checking, and set up for access if
   it really is.  Call back to the calling environments "finish up"
   function just before returning, to handle any last-minute setup.  */
 
bfd_target *
DEFUN(aout_some_aout_object_p,(abfd, callback_to_real_object_p),
      bfd *abfd AND
      bfd_target *(*callback_to_real_object_p) ())
{
  struct exec exec_bytes;
  struct exec *execp;
  struct container *rawptr;

  if (bfd_seek (abfd, 0L, false) < 0) {
    bfd_error = system_call_error;
    return 0;
  }

  if ( bfd_read((PTR)&exec_bytes,1,EXEC_BYTES_SIZE,abfd) != EXEC_BYTES_SIZE) {
    bfd_error = wrong_format;
    return 0;
  }

  /* Use an intermediate variable for clarity */
  rawptr = (struct container *) bfd_zalloc (abfd, sizeof (struct container));

  if (rawptr == NULL) {
    bfd_error = no_memory;
    return 0;
  }

  set_tdata (abfd, rawptr);
  exec_hdr (abfd) = execp = &(rawptr->e);
  aout_swap_exec_header_in(abfd, &exec_bytes, execp);

  /* Set the file flags */
  abfd->flags = NO_FLAGS;
  if (execp->a_drsize || execp->a_trsize)
    abfd->flags |= HAS_RELOC;
  if (execp->a_entry) 
    abfd->flags |= EXEC_P;
  if (execp->a_syms) 
    abfd->flags |= HAS_LINENO | HAS_DEBUG | HAS_SYMS | HAS_LOCALS;

  if (N_MAGIC (*execp) == ZMAGIC) abfd->flags |= D_PAGED;
  if (N_MAGIC (*execp) == NMAGIC) abfd->flags |= WP_TEXT;

  bfd_get_start_address (abfd) = execp->a_entry;

  obj_aout_symbols (abfd) = (aout_symbol_type *)NULL;
  bfd_get_symcount (abfd) = (execp->a_syms / sizeof (struct nlist)) + 3;  /* 3 here is for the 3
									     section symbols. */

  /* Set the default architecture and machine type.  These can be
     overridden in the callback routine.  */
  abfd->obj_arch = bfd_arch_unknown;
  abfd->obj_machine = abfd->obj_machine_2 = 0;

  obj_reloc_entry_size (abfd) = sizeof( struct relocation_info );

  /* create the sections.  This is raunchy, but bfd_close wants to reclaim
     them */
  obj_textsec (abfd) = (asection *)NULL;
  obj_datasec (abfd) = (asection *)NULL;
  obj_bsssec (abfd) = (asection *)NULL;
  (void)bfd_make_section(abfd, ".text");
  (void)bfd_make_section(abfd, ".data");
  (void)bfd_make_section(abfd, ".bss");

  abfd->sections = obj_textsec (abfd);
  obj_textsec (abfd)->next = obj_datasec (abfd);
  obj_datasec (abfd)->next = obj_bsssec (abfd);

  obj_datasec (abfd)->size = execp->a_data;
  obj_bsssec (abfd)->size = execp->a_bss;
  obj_textsec (abfd)->size = execp->a_text;

  if (abfd->flags & D_PAGED) {
    obj_textsec (abfd)->size -=  EXEC_BYTES_SIZE;
  }
    

  obj_textsec(abfd)->flags = execp->a_trsize  ?
	  (SEC_ALLOC | SEC_LOAD | SEC_HAS_CONTENTS | SEC_CODE | SEC_RELOC )
	  :
	  (SEC_ALLOC | SEC_LOAD | SEC_HAS_CONTENTS | SEC_CODE);

  obj_datasec(abfd)->flags = execp->a_drsize ?
	  (SEC_ALLOC | SEC_LOAD | SEC_HAS_CONTENTS | SEC_DATA | SEC_IS_WRITEABLE | SEC_RELOC)
	  :
	  (SEC_ALLOC | SEC_LOAD | SEC_HAS_CONTENTS | SEC_DATA | SEC_IS_WRITEABLE);

  obj_bsssec (abfd)->flags = SEC_ALLOC | SEC_IS_BSS | SEC_IS_WRITEABLE;

#ifdef THIS_IS_ONLY_DOCUMENTATION
  /* Call back to the format-dependent code to fill in the rest of the 
     fields and do any further cleanup.  Things that should be filled
     in by the callback:  */

  struct exec *execp = exec_hdr (abfd);

  /* The virtual memory addresses of the sections */
  obj_datasec (abfd)->vma = N_DATADDR(*execp);
  obj_bsssec (abfd)->vma = N_BSSADDR(*execp);
  obj_textsec (abfd)->vma = N_TXTADDR(*execp);

  /* The file offsets of the sections */
  obj_textsec (abfd)->filepos = N_TXTOFF(*execp);
  obj_datasec (abfd)->filepos = N_DATOFF(*execp);

  /* The file offsets of the relocation info */
  obj_textsec (abfd)->rel_filepos = N_TROFF(*execp);
  obj_datasec (abfd)->rel_filepos = N_DROFF(*execp);

  /* The file offsets of the string table and symbol table.  */
  obj_str_filepos (abfd) = N_STROFF (*execp);
  obj_sym_filepos (abfd) = N_SYMOFF (*execp);

  /* This common code can't fill in those things because they depend
     on either the start address of the text segment, the rounding
     up of virtual addersses between segments, or the starting file 
     position of the text segment -- all of which varies among different
     versions of a.out.  */

  /* Determine the architecture and machine type of the object file.  */
  switch (N_MACHTYPE (*exec_hdr (abfd))) {
  default:
    abfd->obj_arch = bfd_arch_obscure;
    break;
  }

  /* Determine the size of a relocation entry */
  switch (abfd->obj_arch) {
  case bfd_arch_sparc:
  case bfd_arch_a29k:
    obj_reloc_entry_size (abfd) = RELOC_EXT_SIZE;
  default:
    obj_reloc_entry_size (abfd) = RELOC_STD_SIZE;
  }

  return abfd->xvec;

  /* The architecture is encoded in various ways in various a.out variants,
     or is not encoded at all in some of them.  The relocation size depends
     on the architecture and the a.out variant.  Finally, the return value
     is the bfd_target vector in use.  If an error occurs, return zero and
     set bfd_error to the appropriate error code.
     
     Formats such as b.out, which have additional fields in the a.out
     header, should cope with them in this callback as well.  */
#endif				/* DOCUMENTATION */


  return (*callback_to_real_object_p)(abfd);
}

/* exec and core file sections */

boolean
DEFUN(aout_new_section_hook,(abfd, newsect),
      bfd *abfd AND
      asection *newsect)
{
  /* No default alignment */
  newsect->alignment_power = 0;
    
  if (bfd_get_format (abfd) == bfd_object) {
    if (obj_textsec(abfd) == NULL && !strcmp(newsect->name, ".text")) {
      obj_textsec(abfd)= newsect;
      return true;
    }
      
    if (obj_datasec(abfd) == NULL && !strcmp(newsect->name, ".data")) {
      obj_datasec(abfd) = newsect;
      return true;
    }
      
    if (obj_bsssec(abfd) == NULL && !strcmp(newsect->name, ".bss")) {
      obj_bsssec(abfd) = newsect;
      return true;
    }

    if (!strcmp(newsect->name,"COMMON") || !strcmp(newsect->name,"PROFILE_COUNTER"))
	    return true;
  }
    
  /* We do not allow any but the sacred three sections, COMMON and PROFILE_COUNTER
     in aout format. */
  return false;
}

/* Classify stabs symbols */

#define sym_in_text_section(sym) \
(((fetch_native_aout_info(&sym->symbol)->type  & (N_ABS | N_TEXT | N_DATA | N_BSS))== N_TEXT))

#define sym_in_data_section(sym) \
(((fetch_native_aout_info(&sym->symbol)->type  & (N_ABS | N_TEXT | N_DATA | N_BSS))== N_DATA))

#define sym_in_bss_section(sym) \
(((fetch_native_aout_info(&sym->symbol)->type  & (N_ABS | N_TEXT | N_DATA | N_BSS))== N_BSS))

/* Symbol is undefined if type is N_UNDF|N_EXT and if it has
zero in the "value" field.  Nonzeroes there are fortrancommon
symbols.  */
#define sym_is_undefined(sym) \
(fetch_native_aout_info(&sym->symbol)->other == 0 && \
(fetch_native_aout_info(&sym->symbol)->type == (N_UNDF | N_EXT) && (sym)->symbol.value == 0))

/* Symbol is a global definition if N_EXT is on and if it has
a nonzero type field.  */
#define sym_is_global_defn(sym) \
(IS_SYSPROCIDX(fetch_native_aout_info(&sym->symbol)->other) || \
(((fetch_native_aout_info(&sym->symbol)->type & N_EXT) && (fetch_native_aout_info(&sym->symbol)->type & N_TYPE))))

/* Symbol is debugger info if any bits outside N_TYPE or N_EXT
are on.  */
#define sym_is_debugger_info(sym) \
((fetch_native_aout_info(&sym->symbol)->type & ~(N_EXT | N_TYPE)))

#define sym_is_fortrancommon(sym)       \
(((fetch_native_aout_info(&sym->symbol)->type == (N_EXT)) && (sym)->symbol.value != 0))

/* Symbol is absolute if it has N_ABS set */
#define sym_is_absolute(sym) \
(((fetch_native_aout_info(&sym->symbol)->type  & N_TYPE)== N_ABS))


#define sym_is_indirect(sym) \
(((fetch_native_aout_info(&sym->symbol)->type & N_ABS)== N_ABS))

/* Only in their own functions for ease of debugging; when sym flags have
stabilised these should be inlined into their (single) caller */

static void
DEFUN(translate_from_native_sym_flags,(sym_pointer, cache_ptr, abfd),
      struct nlist *sym_pointer AND
      aout_symbol_type *cache_ptr AND
      bfd *abfd)
{
  if (sym_is_debugger_info (cache_ptr)) {
    cache_ptr->symbol.flags = BSF_DEBUGGING ;
    /* Work out the section correct for this symbol */
    switch (fetch_native_aout_info(&cache_ptr->symbol)->type & N_TYPE) 
	{
	case N_TEXT:
	case N_FN:
	  cache_ptr->symbol.section = obj_textsec (abfd);
	  cache_ptr->symbol.value -= obj_textsec(abfd)->vma;
	  break;
	case N_DATA:
	  cache_ptr->symbol.value  -= obj_datasec(abfd)->vma;
	  cache_ptr->symbol.section = obj_datasec (abfd);
	  break;
	case N_BSS :
	  cache_ptr->symbol.section = obj_bsssec (abfd);
	  cache_ptr->symbol.value -= obj_bsssec(abfd)->vma;
	  break;
	case N_ABS:
	default:
	  cache_ptr->symbol.section = 0;
	  break;
	}
  }
  else {
    if (sym_is_fortrancommon (cache_ptr))
	{
	  cache_ptr->symbol.flags = BSF_FORT_COMM | BSF_ALL_OMF_REP;
	  cache_ptr->symbol.section = (asection *)NULL;
	}
    else {
      if (sym_is_undefined (cache_ptr)) {
	cache_ptr->symbol.flags = BSF_UNDEFINED | BSF_ALL_OMF_REP;
      }
      else if (sym_is_global_defn (cache_ptr)) {
	cache_ptr->symbol.flags = BSF_GLOBAL | BSF_EXPORT | BSF_ALL_OMF_REP;
      }
      else {
	cache_ptr->symbol.flags = BSF_LOCAL | BSF_XABLE | BSF_ALL_OMF_REP;
      }

      if (sym_is_absolute (cache_ptr)) {
	cache_ptr->symbol.flags |= BSF_ABSOLUTE | BSF_ALL_OMF_REP;
      }

      /* In a.out, the value of a symbol is always relative to the 
       * start of the file, if this is a data symbol we'll subtract
       * the size of the text section to get the section relative
       * value. If this is a bss symbol (which would be strange)
       * we'll subtract the size of the previous two sections
       * to find the section relative address.
       */

      if (sym_in_text_section (cache_ptr))   {
	cache_ptr->symbol.value -= obj_textsec(abfd)->vma;
	cache_ptr->symbol.section = obj_textsec (abfd);
      }
      else if (sym_in_data_section (cache_ptr)){
	cache_ptr->symbol.value -= obj_datasec(abfd)->vma;
	cache_ptr->symbol.section = obj_datasec (abfd);
      }
      else if (sym_in_bss_section(cache_ptr)) {
	cache_ptr->symbol.section = obj_bsssec (abfd);
	cache_ptr->symbol.value -= obj_bsssec(abfd)->vma;
      }
      else {
	cache_ptr->symbol.section = (asection *)NULL;

        /* symbols cannot be undefined and absolute at the
         * same time.
         */
        if (!(cache_ptr->symbol.flags & BSF_UNDEFINED)) {
                cache_ptr->symbol.flags |= BSF_ABSOLUTE;
        }
      }
    }
  }
}

static void
DEFUN(translate_to_native_sym_flags,(sym_pointer, cache_ptr, abfd),
     struct nlist *sym_pointer AND
     asymbol *cache_ptr AND
     bfd *abfd)
{
  bfd_vma value = cache_ptr->value;

  if (bfd_get_section(cache_ptr)) {
    if (bfd_get_output_section(cache_ptr) == obj_bsssec (abfd)) {
      sym_pointer->n_type |= N_BSS;
    }
    else if (bfd_get_output_section(cache_ptr) == obj_datasec (abfd)) {
      sym_pointer->n_type |= N_DATA;
    }
    else  if (bfd_get_output_section(cache_ptr) == obj_textsec (abfd)) {
      sym_pointer->n_type |= N_TEXT;
    }
    else {
      bfd_error_vector.nonrepresentable_section(abfd,
						bfd_get_output_section(cache_ptr)->name);
    }
    /* Turn the symbol from section relative to absolute again */
    
    value +=
      cache_ptr->section->output_section->vma 
	+ cache_ptr->section->output_offset ;
  }
  else {
    sym_pointer->n_type |= N_ABS;
  }
  
  if (cache_ptr->flags & (BSF_FORT_COMM | BSF_UNDEFINED)) {
    sym_pointer->n_type = (N_UNDF | N_EXT);
  }
  else {
    if (cache_ptr->flags & BSF_ABSOLUTE) {
      sym_pointer->n_type |= N_ABS;
    }
    
    if (cache_ptr->flags & (BSF_GLOBAL | BSF_EXPORT)) {
      sym_pointer->n_type |= N_EXT;
    }
    if (cache_ptr->flags & BSF_DEBUGGING) {
      sym_pointer->n_type = fetch_native_aout_info(cache_ptr)->type;
    }
  }
  PUTWORD(abfd, value, sym_pointer->n_value);
}

/* Native-level interface to symbols. */

/* We read the symbols into a buffer, which is discarded when this
function exits.  We read the strings into a buffer large enough to
hold them all plus all the cached symbol entries. */

asymbol *
DEFUN(aout_make_empty_symbol,(abfd),
      bfd *abfd)
{
    aout_symbol_type  *new =
      (aout_symbol_type *)bfd_zalloc (abfd, sizeof (aout_symbol_type));
    new->symbol.the_bfd = abfd;
    
    return &new->symbol;
}

boolean
DEFUN(aout_slurp_symbol_table,(abfd),
      bfd *abfd)
{
    bfd_size_type symbol_size;
    bfd_size_type string_size;
    unsigned int string_chars;
    struct nlist *syms;
    char *strings;
    aout_symbol_type *cached;
    
    /* If there's no work to be done, don't do any */
    if (obj_aout_symbols (abfd) != (aout_symbol_type *)NULL) return true;
    symbol_size = exec_hdr(abfd)->a_syms;
    if (symbol_size == 0) {
	if (bfd_get_symcount (abfd)) {
	    asection *s;
	    aout_symbol_type *cache_ptr = (aout_symbol_type *)
		    bfd_zalloc(abfd, (bfd_size_type)(bfd_get_symcount (abfd) *
						     sizeof(aout_symbol_type)));

	    obj_aout_symbols(abfd) = cache_ptr;
	    for (s=abfd->sections;s;s = s->next,cache_ptr++) {
		asymbol *sym = &cache_ptr->symbol;

		sym->the_bfd = abfd;
		sym->name = s->name;
		sym->flags |= BSF_LOCAL | BSF_ALL_OMF_REP | BSF_TMP_REL_SYM | BSF_SECT_SYM;
		sym->section = s;
		s->sect_sym = sym;
	    }
	    return true;
	}
	else
		return false;
    }
    bfd_seek (abfd, obj_str_filepos (abfd), SEEK_SET);
    if (bfd_read ((PTR)&string_chars, BYTES_IN_WORD, 1, abfd) != BYTES_IN_WORD)
      return false;
    string_size = GETWORD (abfd, string_chars);
    
    strings =(char *) bfd_alloc(abfd, string_size + 1);
    cached = (aout_symbol_type *)
      bfd_zalloc(abfd, (bfd_size_type)(bfd_get_symcount (abfd) *
				       sizeof(aout_symbol_type)));
    syms = (struct nlist *) bfd_alloc(abfd, symbol_size);
    
    bfd_seek (abfd, obj_sym_filepos (abfd), SEEK_SET);
    if (bfd_read ((PTR)syms, 1, symbol_size, abfd) != symbol_size) {
    bailout:
      if (syms) 	bfd_release (abfd, syms);
      if (cached)	bfd_release (abfd, cached);
      if (strings)	bfd_release (abfd, strings);
      return false;
    }
    
    bfd_seek (abfd, obj_str_filepos (abfd), SEEK_SET);
    if (bfd_read ((PTR)strings, 1, string_size, abfd) != string_size) {
      goto bailout;
    }
    
    /* OK, now walk the new symtable, cacheing symbol properties */
      {
	register struct nlist *sym_pointer;
	register struct nlist *sym_end = syms + (bfd_get_symcount (abfd) - 3);
	register aout_symbol_type *cache_ptr = cached;
	
	/* Run through table and copy values */
	for (sym_pointer = syms, cache_ptr = cached;
	     sym_pointer < sym_end; sym_pointer++, cache_ptr++) 
	    {
	      bfd_vma x = GETWORD(abfd, sym_pointer->n_un.n_name);
	      cache_ptr->symbol.the_bfd = abfd;
	      if (x) {
		cache_ptr->symbol.name = x + strings;
		}
	      else
		cache_ptr->symbol.name = (char *)NULL;
	      
	      cache_ptr->symbol.value_2 = 0;
	      cache_ptr->symbol.value = GETWORD(abfd,  sym_pointer->n_value);
	      cache_ptr->symbol.native_info = (char *) &cache_ptr->native_aout_sym_info;
	      fetch_native_aout_info(&cache_ptr->symbol)->desc  = GETHALF(abfd,sym_pointer->n_desc);
	      fetch_native_aout_info(&cache_ptr->symbol)->other = bfd_get_8(abfd, &sym_pointer->n_other);
	      fetch_native_aout_info(&cache_ptr->symbol)->type  = bfd_get_8(abfd, &sym_pointer->n_type);
	      cache_ptr->symbol.udata = 0;
	      translate_from_native_sym_flags (sym_pointer, cache_ptr, abfd);
	      if (IS_BALNAME(fetch_native_aout_info(&cache_ptr->symbol)->other)) {
			/* if we are at the second half of a two-symbol
			call-entry/bal-entry leaf procedure, we need to
			make sure the first half remembers where its
			second half is */
			aout_symbol_type *maybe_call;

			maybe_call = cache_ptr - 1;
			if (cache_ptr > cached &&
			    IS_CALLNAME(fetch_native_aout_info(&maybe_call->symbol)->other)) {
				maybe_call->symbol.flags |= BSF_HAS_BAL;
				maybe_call->symbol.related_symbol = &cache_ptr->symbol;
				cache_ptr->symbol.flags |= BSF_HAS_CALL;
				cache_ptr->symbol.related_symbol = &maybe_call->symbol;
			}
			else {
			    asymbol *bal_sym = aout_make_empty_symbol(abfd);

			    (*bal_sym) = cache_ptr->symbol;
			    cache_ptr->symbol.flags |= BSF_HAS_BAL;
			    cache_ptr->symbol.related_symbol = bal_sym;
			    bal_sym->flags |= BSF_TMP_REL_SYM | BSF_LOCAL | BSF_HAS_CALL;
			    bal_sym->flags &= (~BSF_GLOBAL);
			    bal_sym->name = "";
			    bal_sym->related_symbol = &cache_ptr->symbol;
			}
	      }
	      else if (IS_SYSPROCIDX(fetch_native_aout_info(&cache_ptr->symbol)->other)) {
		  cache_ptr->symbol.flags &= ~BSF_ABSOLUTE;
		  cache_ptr->symbol.flags |= BSF_HAS_SCALL;
		  cache_ptr->symbol.related_symbol = (asymbol *) bfd_zalloc(abfd,sizeof(asymbol));
		  cache_ptr->symbol.related_symbol->flags |= BSF_HAS_CALL;
		  cache_ptr->symbol.related_symbol->related_symbol = &cache_ptr->symbol;
		  cache_ptr->symbol.related_symbol->value =
			  GET_UBITS(fetch_native_aout_info(&cache_ptr->symbol)->other);
	      }
	    }
	if (1) {
	    asection *s;

	    for (s=abfd->sections;s;s = s->next,cache_ptr++) {
		asymbol *sym = &cache_ptr->symbol;

		sym->the_bfd = abfd;
		sym->name = s->name;
		sym->flags |= BSF_LOCAL | BSF_ALL_OMF_REP | BSF_TMP_REL_SYM | BSF_SECT_SYM;
		sym->section = s;
		s->sect_sym = sym;
	    }
	}
    }
    
    obj_aout_symbols (abfd) =  cached;
    bfd_release (abfd, (PTR)syms);

    return true;
}


void
DEFUN(aout_write_syms,(abfd),
      bfd *abfd)
{
    unsigned int count;
    int buffer;
    asymbol **generic = bfd_get_outsymbols(abfd);
    bfd_size_type stindex = BYTES_IN_WORD; /* initial string length */

    for (count = 0; count < bfd_get_symcount (abfd); count++) {
	asymbol *g = generic[count];
	struct nlist nsp;

	if (g->name) {
	    unsigned int length = strlen(g->name) +1;
	    PUTWORD(abfd,stindex,nsp.n_un.n_name);
	    stindex += length;
	}
	else
		nsp.n_un.n_name = 0;

	if (!g->native_info) {
	    nsp.n_desc = nsp.n_other = nsp.n_type = 0;
	    if (g->flags & (BSF_HAS_CALL | BSF_HAS_BAL | BSF_HAS_SCALL)) {
		if (g->flags & BSF_HAS_CALL)
			PUTBYTE(abfd, N_BALNAME, nsp.n_other);
		else if (g->flags & BSF_HAS_SCALL)
			PUTBYTE(abfd, g->related_symbol->value, nsp.n_other);
		else if (g->flags & BSF_HAS_BAL)
			PUTBYTE(abfd, N_CALLNAME, nsp.n_other);
	    }
	}
	else {
	    PUTHALF(abfd, fetch_native_aout_info(g)->desc,   nsp.n_desc);
	    PUTBYTE(abfd, fetch_native_aout_info(g)->other,  nsp.n_other);
	    PUTBYTE(abfd, fetch_native_aout_info(g)->type,   nsp.n_type);
	}

	translate_to_native_sym_flags (&nsp, (PTR)g, abfd);
	bfd_write((PTR)&nsp,1,sizeof(struct nlist), abfd);
    }

    /* Now output the strings.  Be sure to put string length into
     * correct byte ordering before writing it.
     */
    PUTWORD (abfd, stindex, buffer);
    bfd_write((PTR)&buffer, 1, BYTES_IN_WORD, abfd);

    generic = bfd_get_outsymbols(abfd);
    for (count = 0; count < bfd_get_symcount(abfd); count++) {
	asymbol *g = *(generic++);

	if (g->name) {
	    size_t length = strlen(g->name)+1;
	    bfd_write((PTR)g->name, 1, length, abfd);
	}
	if ((g->flags & BSF_FAKE)==0)
		g->sym_tab_idx = count;
    }
}

unsigned int
DEFUN(aout_get_symtab,(abfd, location),
      bfd *abfd AND
      asymbol **location)
{
  unsigned int counter = 0;
  aout_symbol_type *symbase;
  
  if (!aout_slurp_symbol_table(abfd)) return 0;
  
  for (symbase = obj_aout_symbols(abfd); counter++ < bfd_get_symcount (abfd);)
    *(location++) = (asymbol *)( symbase++);
  *location++ =0;
  return bfd_get_symcount(abfd);
}

unsigned int
DEFUN(aout_get_symtab_upper_bound,(abfd),
     bfd *abfd)
{
  if (!aout_slurp_symbol_table(abfd)) return 0;

  return (bfd_get_symcount (abfd)+1) * (sizeof (aout_symbol_type *));
}

alent *
DEFUN(aout_get_lineno,(ignore_abfd, ignore_symbol),
      bfd *ignore_abfd AND
      asymbol *ignore_symbol)
{
  return (alent *)NULL;
}


void 
DEFUN(aout_print_symbol,(ignore_abfd, afile, symbol, how),
      bfd *ignore_abfd AND
      PTR afile AND
      asymbol *symbol AND
      bfd_print_symbol_enum_type how)
{
  FILE *file = (FILE *)afile;

  switch (how) {
  case bfd_print_symbol_name_enum:
    fprintf(file,"%s", symbol->name);
    break;
  case bfd_print_symbol_type_enum:
    fprintf(file,"%4x %2x %2x",(unsigned)(fetch_native_aout_info(symbol)->desc & 0xffff),
	    (unsigned)(fetch_native_aout_info(symbol)->other & 0xff),
	    (unsigned)(fetch_native_aout_info(symbol)->type));
    break;
  case bfd_print_symbol_all_enum:
    {
   CONST char *section_name = symbol->section == (asection *)NULL ?
	"*abs" : symbol->section->name;

      bfd_print_symbol_vandf((PTR)file,symbol);

      fprintf(file," %-5s %04x %02x %02x %s",
	      section_name,
	      (unsigned)(fetch_native_aout_info(symbol)->desc & 0xffff),
	      (unsigned)(fetch_native_aout_info(symbol)->other & 0xff),
	      (unsigned)(fetch_native_aout_info(symbol)->type  & 0xff),
	      symbol->name);
    }
    break;
  }
}

/* 
 provided a bfd, a section and an offset into the section, calculate
 and return the name of the source file and the line nearest to the
 wanted location.
*/
 
boolean
DEFUN(aout_find_nearest_line,(abfd,
				     section,
				     symbols,
				     offset,
				     filename_ptr,
				     functionname_ptr,
				     line_ptr),
      bfd *abfd AND
      asection *section AND
      asymbol **symbols AND
      bfd_vma offset AND
      CONST char **filename_ptr AND
      CONST char **functionname_ptr AND
      unsigned int *line_ptr)
{
    /* Run down the file looking for the filename, function and linenumber */
    asymbol **p;
    static  char buffer[100];
    bfd_vma high_line_vma = ~0;
    bfd_vma low_func_vma = 0;
    asymbol *func = 0;

    *filename_ptr = abfd->filename;
    *functionname_ptr = 0;
    *line_ptr = 0;
    if (symbols != (asymbol **)NULL) {
	for (p = symbols; *p; p++) {
	    if (fetch_native_aout_info(*p)) {
		aout_symbol_type  *q = (aout_symbol_type *)(*p);
		switch (fetch_native_aout_info(&q->symbol)->type){
	    case N_SO:
		    *filename_ptr = q->symbol.name;
		    if (obj_textsec(abfd) != section) {
			return true;
		    }
		    break;
	    case N_SLINE:
	    case N_DSLINE:
	    case N_BSLINE:
		    /* We'll keep this if it resolves nearer than the one we have already */
		    if (q->symbol.value >= offset &&
			q->symbol.value < high_line_vma) {
			*line_ptr = fetch_native_aout_info(&q->symbol)->desc;
			high_line_vma = q->symbol.value;
		    }
		    break;
	    case N_FUN:
		{
		    /* We'll keep this if it is nearer than the one we have already */
		    if (q->symbol.value >= low_func_vma &&
			q->symbol.value <= offset) {
			low_func_vma = q->symbol.value;
			func = (asymbol *)q;
		    }
		    if (*line_ptr && func) {
			CONST char *function = func->name;
			char *p;
			strncpy(buffer, function, sizeof(buffer)-1);
			buffer[sizeof(buffer)-1] = 0;
			/* Have to remove : stuff */
			p = strchr(buffer,':');
			if (p != 0) {*p = 0; }
			*functionname_ptr = buffer;
			return true;
		    }
		}
		    break;
		}
	    }
	}
    }
    return true;
}


/* Temp kludge to get this info to gdb (dbxread.c) */
file_ptr
aout_sym_filepos(abfd)
bfd *abfd;
{
	return obj_sym_filepos(abfd);
}

file_ptr
aout_str_filepos(abfd)
bfd *abfd;
{
	return  obj_str_filepos(abfd);
}

bfd_ghist_info *aout_fetch_ghist_info(abfd,nelements,nlinenumbers)
    bfd *abfd;
    unsigned int *nelements,*nlinenumbers;
{
    unsigned int p_max = bfd_get_symcount(abfd),i,lines_start = -1;
    bfd_ghist_info *p = (bfd_ghist_info *) 0;
    aout_symbol_type *symbase;
    char *file_name = NULL,*func_name = NULL,tmpbuffer[100];

    *nelements = 0;
    *nlinenumbers = 0;
    if (!aout_slurp_symbol_table(abfd))
	    return p;
    p = (bfd_ghist_info *) bfd_alloc(abfd,p_max*sizeof(bfd_ghist_info));
    for (i=0,symbase = obj_aout_symbols(abfd);i < p_max;symbase++,i++) {
	if (fetch_native_aout_info(&symbase->symbol)) {
	    if (!sym_is_debugger_info(symbase) &&
		sym_in_text_section(symbase) &&
		!strchr(symbase->symbol.name,'.') &&
	        strcmp(symbase->symbol.name, "___gnu_compiled_c") != 0)
		    _bfd_add_bfd_ghist_info(&p,nelements,&p_max,
					    obj_textsec(abfd)->vma+symbase->symbol.value,
					    _bfd_trim_under_and_slashes(symbase->symbol.name,1),
					    _BFD_GHIST_INFO_FILE_UNKNOWN,0);
	    else if (sym_is_debugger_info(symbase)) {
		switch (fetch_native_aout_info(&symbase->symbol)->type) {
	    case N_SO:
	    case N_SOL:
		    file_name = (char *) _bfd_trim_under_and_slashes(symbase->symbol.name,0);
		    break;
	    case N_SLINE:
	    case N_DSLINE:
	    case N_BSLINE:
		    (*nlinenumbers)++;
		    if (lines_start == -1)
			    lines_start = (*nelements);
		    _bfd_add_bfd_ghist_info(&p,nelements,&p_max,
					    obj_textsec(abfd)->vma+symbase->symbol.value,
					    NULL,file_name,fetch_native_aout_info(&symbase->symbol)->desc);
		    break;
	    case N_FUN:
	    case N_FNAME:
		{
		    asymbol *asym = (asymbol *) symbase;
		    char *t;
		    int j;

		    strncpy(tmpbuffer,asym->name,sizeof(tmpbuffer)-1);
		    tmpbuffer[sizeof(tmpbuffer)-1] = 0;
		    t = strchr(tmpbuffer,':');
		    if (t)
			    *t = 0;
		    func_name = _bfd_buystring(tmpbuffer);
		    for(j=lines_start;j < (*nelements);j++)
			    p[j].func_name = func_name;
		    lines_start = -1;
		    _bfd_add_bfd_ghist_info(&p,nelements,&p_max,
					    obj_textsec(abfd)->vma+symbase->symbol.value,
					    func_name,file_name,0);
		}
		}
	    }
	}
    }
    _bfd_add_bfd_ghist_info(&p,nelements,&p_max,
			    obj_textsec(abfd)->vma+obj_textsec(abfd)->size,
			    "_etext",
			    _BFD_GHIST_INFO_FILE_UNKNOWN,0);
    qsort(p,*nelements,sizeof(bfd_ghist_info),_bfd_cmp_bfd_ghist_info);
    for (i=0;i < (*nelements);i++) {
 try_again:
	if ((i+1) < (*nelements)            &&
	    p[i].address == p[i+1].address) {
	    if (p[i].file_name                  &&	
		!strcmp(_BFD_GHIST_INFO_FILE_UNKNOWN,p[i].file_name)) {
		_bfd_remove_ghist_info_element(p,i,nelements);
		goto try_again;
	    }
	    else if (p[i+1].file_name                  &&	
		     !strcmp(_BFD_GHIST_INFO_FILE_UNKNOWN,p[i+1].file_name)) {
		_bfd_remove_ghist_info_element(p,i+1,nelements);
		goto try_again;
	    }
	    else if (!p[i].line_number) {
		_bfd_remove_ghist_info_element(p,i,nelements);
		goto try_again;
	    }
	    else if (!p[i+1].line_number) {
		_bfd_remove_ghist_info_element(p,i+1,nelements);
		goto try_again;
	    }
	}
    }
    if (p_max > (*nelements))
	    p = (bfd_ghist_info *) bfd_realloc(abfd,p,sizeof(bfd_ghist_info)*(*nelements));
    return p;
}
