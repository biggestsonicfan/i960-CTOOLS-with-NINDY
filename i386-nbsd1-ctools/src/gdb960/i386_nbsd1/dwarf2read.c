/* DWARF version 2.0 debugging format support for GDB.
   Copyright (C) 1991, 1992, 1993, 1994 Free Software Foundation, Inc.

   Written by Peter Swartwout at Intel Corporation.  Based on dwarfread.c,
   the Dwarf version 1.0 reader written by Fred Fish at Cygnus Support.

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "defs.h"
#include "bfd.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "symfile.h"
#include "objfiles.h"
#include "elf.h"	/* For SHF_960_MSB */
#include "libdwarf.h"
#include "buildsym.h"
#include "demangle.h"
#include "expression.h"	/* Needed for enum exp_opcode in language.h, sigh... */
#include "language.h"
#include "complaints.h"
#include "frame.h"	/* Needed for .debug_frame stuff */

#include <fcntl.h>
#include <string.h>

#ifndef	NO_SYS_FILE
#include <sys/file.h>
#endif

/* Some macros to provide DIE info for complaints. */

#define DIE_ID(die) (die ? dwarf_dieoffset(die) : 0)
#define DIE_NAME(die) ((die && dwarf_diename(die)) ? dwarf_diename(die) : "")

/* Complaints that can be issued during DWARF debug info reading. */

static struct complaint no_bfd_get_N =
{
  "DIE @ 0x%x \"%s\", no bfd support for %d byte data object", 0, 0
};

static struct complaint malformed_die =
{
  "DIE @ 0x%x \"%s\", malformed DIE", 0, 0
};

static struct complaint bad_die_ref =
{
  "DIE @ 0x%x \"%s\", reference to DIE (0x%x) outside compilation unit", 0, 0
};

static struct complaint unknown_attribute_form =
{
  "DIE @ 0x%x \"%s\", unknown attribute form (0x%x)", 0, 0
};

static struct complaint unknown_attribute_length =
{
  "DIE @ 0x%x \"%s\", unknown attribute length, skipped remaining attributes", 0, 0
};

static struct complaint unknown_type_modifier =
{
  "DIE @ 0x%x \"%s\", unknown type modifier tag 0x%x", 0, 0
};

static struct complaint unexpected_base_type =
{
  "DIE @ 0x%x \"%s\", unexpected base type encoding 0x%x", 0, 0
};

static struct complaint dup_user_type_allocation =
{
  "DIE @ 0x%x \"%s\", internal error: duplicate user type allocation", 0, 0
};

static struct complaint dup_user_type_definition =
{
  "DIE @ 0x%x \"%s\", internal error: duplicate user type definition", 0, 0
};

static struct complaint missing_tag =
{
  "DIE @ 0x%x \"%s\", missing class, structure, or union tag", 0, 0
};

static struct complaint bad_array_element_type =
{
  "DIE @ 0x%x \"%s\", bad array element type attribute 0x%x", 0, 0
};

static struct complaint subscript_data_items =
{
  "DIE @ 0x%x \"%s\", can't decode subscript data items", 0, 0
};

static struct complaint unhandled_array_subscript_format =
{
  "DIE @ 0x%x \"%s\", array subscript format 0x%x not handled yet", 0, 0
};

static struct complaint unknown_array_subscript_format =
{
  "DIE @ 0x%x \"%s\", unknown array subscript format %x", 0, 0
};

static struct complaint not_row_major =
{
  "DIE @ 0x%x \"%s\", array not row major; not handled correctly", 0, 0
};

static struct complaint missing_type =
{
  "DIE @ 0x%x \"%s\", Expected DW_AT_type attribute", 0, 0
};

static struct complaint dangling_typeref =
{
  "DIE @ 0x%x \"%s\", Forward type reference never resolved", 0, 0
};

static struct complaint missing_lower_bound =
{
  "DIE @ 0x%x \"%s\", Expected DW_AT_lower_bound attribute", 0, 0
};

static struct complaint missing_upper_bound =
{
  "DIE @ 0x%x \"%s\", Expected DW_AT_upper_bound attribute", 0, 0
};

static struct complaint bad_const_value_form =
{
  "DIE @ 0x%x \"%s\", Unimplemented DW_AT_const_value form: %d", 0, 0
};

static struct complaint bad_const_value_bignum_size =
{
  "DIE @ 0x%x \"%s\", Unimplemented DW_AT_const_value bignum size: %d", 0, 0
};

static struct complaint bad_const_value_block_size =
{
  "DIE @ 0x%x \"%s\", Unimplemented DW_AT_const_value block size: %d", 0, 0
};

static struct complaint invalid_location_stack_elem_size =
{
  "Dwarf location stack element size (%d) doesn't match machine address size (%d)", 0, 0
};

static struct complaint unimplemented_location_operation =
{
  "DIE @ 0x%x \"%s\", Unimplemented location operation: 0x%x", 0, 0
};

static struct complaint unknown_location_operation =
{
  "DIE @ 0x%x \"%s\", Unrecognized location operation: 0x%x", 0, 0
};

static struct complaint empty_location_stack =
{
  "DIE @ 0x%x \"%s\", Invalid location expression: stack was popped when empty", 0, 0
};

static struct complaint invalid_location_pick =
{
  "DIE @ 0x%x \"%s\", Invalid pick operation: index 0x%x out of range", 0, 0
};

static struct complaint invalid_location_rotate =
{
  "DIE @ 0x%x \"%s\", Invalid rotate operation: stack too small", 0, 0
};

static struct complaint invalid_binary_operation =
{
  "DIE @ 0x%x \"%s\", Invalid binary location operation: 0x%x", 0, 0
};

static struct complaint invalid_unary_operation =
{
  "DIE @ 0x%x \"%s\", Invalid unary location operation: 0x%x", 0, 0
};

static struct complaint invalid_relational_operation =
{
  "DIE @ 0x%x \"%s\", Invalid relational location operation: 0x%x", 0, 0
};

static struct complaint libdwarf_warning =
{
  "Warning in libdwarf file %s, line %d: %s", 0, 0
};

#ifndef GCC_PRODUCER
#define GCC_PRODUCER "GNU C "
#endif

#ifndef GPLUS_PRODUCER
#define GPLUS_PRODUCER "GNU C++ "
#endif

#ifndef LCC_PRODUCER
#define LCC_PRODUCER "NCR C/C++"
#endif

#ifndef CHILL_PRODUCER
#define CHILL_PRODUCER "GNU Chill "
#endif

/* External variables referenced. */

extern int info_verbose;		/* From main.c; nonzero => verbose */
extern char *warning_pre_print;		/* From utils.c */

static int diecount;	/* Approximate count of dies for compilation unit */

/* Kludge to tell process_die whether or not to process subprograms
   (see comment at read_file_scope) */
static int pass_number;

/* The section offsets used in the current psymtab or symtab. */
static struct section_offsets *base_section_offsets;

/* The generic symbol table building routines have separate lists for
   file scope symbols and all all other scopes (local scopes).  So
   we need to select the right one to pass to add_symbol_to_list().
   We do it by keeping a pointer to the correct list in list_in_scope.

   FIXME:  The original dwarf code just treated the file scope as the first
   local scope, and all other local scopes as nested local scopes, and worked
   fine.  Check to see if we really need to distinguish these in buildsym.c */
static struct pending **list_in_scope = &file_symbols;

/* DIES which have user defined types or modified user defined types refer to
   other DIES for the type information.  Thus we need to associate the offset
   of a DIE for a user defined type with a pointer to the type information.

   Originally this was done using a simple but expensive algorithm, with an
   array of unsorted structures, each containing an offset/type-pointer pair.
   This array was scanned linearly each time a lookup was done.  The result
   was that gdb was spending over half it's startup time munging through this
   array of pointers looking for a structure that had the right offset member.

   The second attempt used the same array of structures, but the array was
   sorted using qsort each time a new offset/type was recorded, and a binary
   search was used to find the type pointer for a given DIE offset.  This was
   even slower, due to the overhead of sorting the array each time a new
   offset/type pair was entered.

   The third attempt uses a fixed size array of type pointers, indexed by a
   value derived from the DIE offset.  Since the minimum DIE size is 4 bytes,
   we can divide any DIE offset by 4 to obtain a unique index into this fixed
   size array.  Since each element is a 4 byte pointer, it takes exactly as
   much memory to hold this array as to hold the DWARF info for a given
   compilation unit.  But it gets freed as soon as we are done with it.
   This has worked well in practice, as a reasonable tradeoff between memory
   consumption and speed, without having to resort to much more complicated
   algorithms. */

/* Update for DWARF V2: The concept of fundamental type is gone.  It has 
   been replaced with the concept of "base type", which is just another type
   whose definition is provided by the compiler.  Thus EVERY type fits the
   model Fred explained above, and we'll use the utypes array to cache EVERY
   type, including base types. */

static struct type **utypes;	/* Pointer to array of user type pointers */
static int numutypes;		/* Max number of user type pointers */

/* The structure-building functions are pretty kludgy because there's 
   no way to know in advance how many members a structure has.  (the 
   member DIES are the children of the struct DIE.)  So we create a list
   of the members, and copy them into the struct type after all have been
   processed.  The struct below is the go-between. */
struct nextfield 
{
    struct field field;
    struct symbol *sym;	/* Used to set the type of enum literals */
    struct nextfield *next;
};

/* Same thing for array subscripts.  (They are the children of the array
   DIE).  There can be (and will be, with gcc) more than one subrange DIE
   owned by a single array DIE.  It's most efficient to build up a list 
   of the subranges and then unroll the list when creating the array type. */
struct typelist
{
    struct typelist *next;
    struct type *type;
};

/* Structures to help with associating concrete inlined instances
   with their abstract counterparts. */

struct absym
{
    struct symbol *sym;
    Dwarf_Die die;
};

struct absym_list
{
    /* Number of entries in the table */
    int count;

    /* Maximum number of entries allowed (can be resized) */
    int limit;

    /* A list of symbols (the table entries) */
    struct absym **list;
};

/* To help with processing of inlined functions/variables. */
static struct absym_list abstract_symbols;

/* Record the language for the compilation unit which is currently being
   processed.  We know it once we have seen the DW_TAG_compile_unit DIE,
   and we need it while processing the DIE's for that compilation unit.
   It is eventually saved in the symtab structure, but we don't finalize
   the symtab struct until we have processed all the DIE's for the
   compilation unit.  We also need to get and save a pointer to the 
   language struct for this language, so we can call the language
   dependent routines for doing things such as creating fundamental
   types. */

static enum language cu_language;
static const struct language_defn *cu_language_defn;

/* The currently operating Dwarf file descriptor.  Used in both partial symtab
   reading and symtab reading.  Stored in the sym_private field of the 
   objfile.  Set initially in dwarf2_build_psymtabs, extracted from objile
   in read_ofile_symtab. */
static Dwarf_Debug current_dwfile;

/* The currently operating compilation unit.  Used in symtab reading only.
   Stored in the read_symtab_private field of the partial symtab.  Set 
   initially in scan_compilation_units, extracted from psymtab in
   read_ofile_symtab. */
static Dwarf_Die current_dwcu;

/* Store the root of a concrete instance tree if we have one. 
   Used by variables to distinguish between a concrete out-of-line instance
   and a concrete inlined instance. */
static Dwarf_Die concrete_instance_root;

/* The Dwarf location expression evaluation stack.  
   See comments in vicinity of init_location_stack() for more information. */
typedef unsigned long stackelem;
static stackelem *stack;	/* The stack */
static long stacksize;		/* Number of allocated elements */
static long stacktop;		/* Index of current stack top */
static Dwarf_Die curr_die;	/* For complaint messages in pop, pick, etc */

/* Forward declarations of static functions so we don't have to worry
   about ordering within this file.  */

static void
build_libdwarf_section_list PARAMS ((bfd *, asection *, PTR));

static void
add_enum_psymbol PARAMS ((Dwarf_Die));

static void
handle_producer PARAMS ((char *));

static void
read_file_scope PARAMS ((void));

static void
read_func_scope PARAMS ((Dwarf_Die));

static void
read_variable PARAMS ((Dwarf_Die));

static void
read_lexical_block_scope PARAMS ((Dwarf_Die));

static void
scan_partial_symbols PARAMS ((Dwarf_Die));

static void
scan_compilation_units PARAMS ((void));

static void
add_partial_symbol PARAMS ((Dwarf_Die));

static void
init_psymbol_list PARAMS ((int));

static void
add_abstract_symbol_to_list PARAMS ((struct symbol *, Dwarf_Die));

static struct symbol *
find_abstract_symbol_in_list PARAMS ((char *, Dwarf_Die));

static void
free_abstract_symbol_list PARAMS ((void));

void
dwarf2_build_psymtabs PARAMS ((struct objfile*, struct section_offsets*, int));

static void
dwarf_psymtab_to_symtab PARAMS ((struct partial_symtab *));

static void
psymtab_to_symtab_1 PARAMS ((struct partial_symtab *));

static void
read_ofile_symtab PARAMS ((struct partial_symtab *));

static void
process_die PARAMS ((Dwarf_Die, PTR));

static void
process_children PARAMS ((Dwarf_Die, PTR));

static void
read_base_type PARAMS ((Dwarf_Die));

static void
read_type_modifier PARAMS ((Dwarf_Die));

static void
read_struct_type PARAMS ((Dwarf_Die));

static void
read_member_type PARAMS ((Dwarf_Die, PTR));

static void
read_array_type PARAMS ((Dwarf_Die));

static void
read_subrange_type PARAMS ((Dwarf_Die, PTR));

static void
read_tag_string_type PARAMS ((Dwarf_Die));

static void
read_subroutine_type PARAMS ((Dwarf_Die));

static void
read_enum_type PARAMS ((Dwarf_Die));

static void
read_enumerator_type PARAMS ((Dwarf_Die, PTR));

static void
decode_line_numbers PARAMS ((void));

static struct type *
decode_die_type PARAMS ((Dwarf_Die, int));

static char *
create_name PARAMS ((char *, struct obstack *));

static struct type *
lookup_utype PARAMS ((Dwarf_Die));

static void
free_utypes PARAMS ((PTR));

static struct type *
alloc_utype PARAMS ((Dwarf_Die, struct type *));

static struct symbol *
new_symbol PARAMS ((Dwarf_Die));

static struct symbol *
raw_symbol PARAMS ((char *, enum address_class));

static void
dwarf_attr_const_value PARAMS ((Dwarf_Die, struct symbol *));

static struct inlin *
new_inline PARAMS ((struct symbol *, struct objfile *));

static void
synthesize_typedef PARAMS ((Dwarf_Die, struct type *));

static enum address_class
dwarf2_eval_location PARAMS ((struct symbol *, CORE_ADDR));

static enum address_class
dwarf2_eval_location_list PARAMS ((struct symbol *, Dwarf_Locdesc, 
				   CORE_ADDR));

static void
locpush PARAMS ((stackelem elem));

static stackelem
locpop PARAMS ((void));

static void
init_location_stack PARAMS ((void));

static void
set_cu_language PARAMS ((Dwarf_Die));

static Dwarf_Signed
dwarf2_errhand PARAMS ((Dwarf_Error));

/*

LOCAL FUNCTION

	set_cu_language -- set local copy of language for compilation unit

DESCRIPTION

	Decode the language attribute for a compilation unit DIE and
	remember what the language was.  We use this at various times
	when processing DIE's for a given compilation unit.

RETURNS

	No return value.

 */

static void
set_cu_language (die)
    Dwarf_Die die;
{
    switch ( DIE_ATTR_UNSIGNED(die, DW_AT_language) )
    {
    case DW_LANG_C89:
    case DW_LANG_C:
        cu_language = language_c;
	break;
    case DW_LANG_C_plus_plus:
	cu_language = language_cplus;
	break;
    case DW_LANG_Chill:
	cu_language = language_chill;
	break;
    case DW_LANG_Modula2:
	cu_language = language_m2;
	break;
    case DW_LANG_Ada83:
    case DW_LANG_Cobol74:
    case DW_LANG_Cobol85:
    case DW_LANG_Fortran77:
    case DW_LANG_Fortran90:
    case DW_LANG_Pascal83:
	/* We don't know anything special about these yet. */
	cu_language = language_unknown;
	break;
    default:
	/* If no at_language, try to deduce one from the filename */
	cu_language = deduce_language_from_filename(DIE_ATTR_STRING(die, DW_AT_name));
	break;
    }
    cu_language_defn = language_def (cu_language);
}

/* 

    LOCAL FUNCTION

	build_libdwarf_section_list

    DESCRIPTION

	This is similar to elf_locate_sections: it is called once per section
	from elf_symfile_read, iff elf_locate_sections located a dwarf section.
	
	Libdwarf needs to know basic information about every section, so that
	it can find all Dwarf sections without needing to know anything in 
	particular about Elf.  Besides, we already know everything about 
	sections via bfd, so why should libdwarf go through all that again?

    BUGS

	The symbol SHF_960_MSB is found in elf.h, which renders this whole
	routine Elf-specific.  For now, Elf is the only known Dwarf2 packaging,
	but in theory any OMF packaging that contains Dwarf2 section should
	work with gdb. 
*/

static void
build_libdwarf_section_list (abfd, sectp, sect_list)
     bfd *abfd;
     asection *sectp;
     PTR sect_list;
{
    Dwarf_Section sections = (Dwarf_Section) sect_list;
    sections[sectp->index].name = (char *) bfd_section_name(abfd, sectp);
    sections[sectp->index].size = bfd_section_size(abfd, sectp);
    sections[sectp->index].file_offset = sectp->filepos;
    sections[sectp->index].big_endian = 
	bfd_get_section_flags(abfd, sectp) & SEC_BIG_ENDIAN;
}

/*

LOCAL FUNCTION

    Free memory allocated by libdwarf, and remove any trace of DWARF
    from the objfile.
*/
static void
free_dwarfinfo(ptr)
    PTR ptr;
{
    struct objfile *objfile = (struct objfile *) ptr;
    if (objfile->sym_private)
    {
	dwarf_finish((Dwarf_Debug) objfile->sym_private);
	objfile->sym_private = NULL;
    }
}

/*

GLOBAL FUNCTION

	dwarf2_build_psymtabs -- build partial symtabs from DWARF debug info

SYNOPSIS

	void dwarf2_build_psymtabs (struct objfile *objfile,
	     struct section_offsets *section_offsets, int mainline)

DESCRIPTION

	This function is called upon to build partial symtabs from files
	containing DIE's (Dwarf Information Entries) and DWARF line numbers.

	It is passed a bfd* containing the DIES	and line number information,
	a table of section offsets for relocating the symbols, a flag
	indicating whether or not this debugging information is from a 
	"main symbol table" rather than a shared library or dynamically 
	linked file.

	This function calls libdwarf's dwarf_init() routine to initialize
	a new dwarf file handle.

RETURNS

	No return value.

 */

void
dwarf2_build_psymtabs (objfile, section_offsets, mainline)
    struct objfile *objfile;
    struct section_offsets *section_offsets;
    int mainline;
{
    struct cleanup *back_to;
    Dwarf_Section sections;

    current_objfile = objfile;

    /* If we are reinitializing, or if we have never loaded syms yet, init.
       Since we have no idea how many DIES we are looking at, we just guess
       some arbitrary value. */
    
    if (mainline || objfile->global_psymbols.size == 0 ||
	objfile->static_psymbols.size == 0)
    {
	init_psymbol_list (1024);
    }
    
    /* Open a libdwarf file handle.  The return value, if non-NULL, is used
       in subsequent libdwarf interaction.  Build a list of the object file
       sections in an OMF-independent fashion.  */
    sections = (Dwarf_Section)
	obstack_alloc (&objfile->psymbol_obstack,
		       sizeof(struct Dwarf_Section) * 
		       bfd_count_sections(objfile->obfd));
    bfd_map_over_sections (objfile->obfd, build_libdwarf_section_list, (PTR)sections);
  
    /* Set up basic libdwarf error handling. */
    dwarf_seterrhand(dwarf2_errhand);
    dwarf_seterrarg("Libdwarf error in dwarf_init");

    /* Create a libdwarf file descriptor. */
    current_dwfile = dwarf_init((FILE *) objfile->obfd->iostream,
				DLC_READ,
				sections,
				bfd_count_sections(objfile->obfd));
    dwarf_seterrarg(NULL);

    if ( current_dwfile == NULL )
	error("Unable to initialize Dwarf info");
    
    /* Save the dwarf file handle into the objfile for use later. */
    objfile->sym_private = (PTR) current_dwfile;

    /* Set up a cleanup so that memory allocated by libdwarf will 
       get free'd if error() is called. */
    back_to = make_cleanup(free_dwarfinfo, (PTR) objfile);

    /* Save the relocation factor where everybody can see it.  */
    base_section_offsets = section_offsets;
    
    /* Initialize the Dwarf2 location expression stack */
    init_location_stack();

    /* Follow the compilation unit sibling chain, building a partial symbol
       table entry for each one.  Save enough information about each 
       compilation unit to locate the full DWARF information later. */
    
    scan_compilation_units ();
#ifdef DEBUG
    _dump_file(current_dwfile);
#endif

    /* Everything's OK, so throw away cleanups.  Libdwarf-allocated memory
       will get free'd in dwarf_finish, called from elf_symfile_finish. */
    discard_cleanups (back_to);
    current_objfile = NULL;
    current_dwfile = NULL;
}

/*

LOCAL FUNCTION

    lookup_utype -- look up a user defined type from die reference

DESCRIPTION

    Given a DIE reference, lookup the user defined type associated with
    that DIE, if it has been registered already.  If not registered, then
    return NULL.  Alloc_utype() can be called to register an empty
    type for this reference, which will be filled in later when the
    actual referenced DIE is processed.
    
    Update for Dwarf 2: The concept of fundamental type is gone.  It has 
    been replaced with the concept of "base type", which is just another type
    whose definition is provided by the compiler.  Thus EVERY type fits the
    "user-defined" model of Dwarf 1, and we'll use the utypes array to cache 
    EVERY type, including base types. 

 */

static struct type *
lookup_utype (die)
    Dwarf_Die die;
{
    struct type *type = NULL;

    unsigned utypeidx = dwarf_cu_dieoffset(die);
    
    if ( utypeidx < 0 )
    {
	complain (&bad_die_ref, DIE_ID(die), DIE_NAME(die));
    }
    else if ( utypeidx < numutypes )
    {
	type = *(utypes + utypeidx);
    }
    return (type);
}

/* 
 * free_utypes: free memory allocated to hold the utypes array.
 *
 * Note that you CAN'T just do make_cleanup(free, utypes) because 
 * utypes may get reallocated, which may move it somewhere else. 
 */
static void
free_utypes (dummy)
    PTR dummy;	/* Not used; to satisfy the prototype */
{
    free(utypes);
}

/*

LOCAL FUNCTION

	alloc_utype  -- add a user defined type for die reference

DESCRIPTION

    Given a pointer to a DIE, and a possible pointer to a user
    defined type UTYPEP, register that this DIE has a user
    defined type and either use the specified type in UTYPEP or
    make a new empty type that will be filled in later.
    
    Returns UTYPEP.

    We should only be called after calling lookup_utype() to verify that
    there is not currently a type registered for DIE.

    Update for Dwarf 2: The concept of fundamental type is gone.  It has 
    been replaced with the concept of "base type", which is just another type
    whose definition is provided by the compiler.  Thus EVERY type fits the
    "user-defined" model of Dwarf 1, and we'll use the utypes array to cache 
    EVERY type, including base types. 

 */

static struct type *
alloc_utype (die, utypep)
    Dwarf_Die die;
    struct type *utypep;
{
    struct type **typep;
    unsigned utypeidx = dwarf_cu_dieoffset(die);

    if ( utypeidx < 0 )
    {
	complain(&bad_die_ref, DIE_ID(die), DIE_NAME(die));
	utypep = builtin_type_int;
	return utypep;
    }
    while ( utypeidx >= numutypes )
    {
	utypes = (struct type **) xrealloc (utypes, (numutypes * 2) * sizeof (struct type *));
	/* Clear the new second half; first half has been written on already */
	memset(utypes + numutypes, 0, numutypes * sizeof(struct type *));
	numutypes *= 2;
    }

    typep = utypes + utypeidx;
    if (*typep != NULL)
    {
	utypep = *typep;
	complain (&dup_user_type_allocation, DIE_ID(die), DIE_NAME(die));
    }
    else
    {
	if (utypep == NULL)
	{
	    utypep = alloc_type (current_objfile);
	}
	*typep = utypep;
    }
    return (utypep);
}

/*

LOCAL FUNCTION

    read_base_type -- translate basic DWARF type into gdb type

DESCRIPTION

    Given a DIE with tag DW_TAG_base_type, translate it into gdb
    (struct type *).  Allocate and initialize a slot in utypes array.

NOTES

    For robustness, if we are asked to translate a base type
    that we are unprepared to deal with, we pretend it's builtin_type_int
    so gdb may at least do something reasonable by default. 
    
    If the type is not in the range of those types defined as
    application specific types, we also issue a warning.
*/

static void
read_base_type (die)
    Dwarf_Die die;
{
    struct type *type;
    unsigned byte_size = 4;
    unsigned encoding = DW_ATE_signed;
    char *name = "unnamed";

    if ( DIE_HAS_ATTR(die, DW_AT_encoding) )
	encoding = DIE_ATTR_UNSIGNED(die, DW_AT_encoding);
    
    if ( DIE_HAS_ATTR(die, DW_AT_byte_size) )
	byte_size = DIE_ATTR_UNSIGNED(die, DW_AT_byte_size);

    if ( DIE_HAS_ATTR(die, DW_AT_name) )
	name = DIE_ATTR_STRING(die, DW_AT_name);

    switch ( encoding )
    {
    case DW_ATE_signed:
    case DW_ATE_signed_char:
	type = init_type (TYPE_CODE_INT, byte_size, 0, name, current_objfile);
	break;
    case DW_ATE_unsigned:
    case DW_ATE_unsigned_char:
	type = init_type (TYPE_CODE_INT, byte_size, TYPE_FLAG_UNSIGNED, name, current_objfile);
	break;
    case DW_ATE_float:
	type = init_type (TYPE_CODE_FLT, byte_size, 0, name, current_objfile);
	break;
    case DW_ATE_address:
    case DW_ATE_boolean:
    case DW_ATE_complex_float:
    default:
	complain (&unexpected_base_type, DIE_ID(die), DIE_NAME(die), encoding);
	type = builtin_type_int;
	break;
    }
    alloc_utype(die, type);
}

/*

LOCAL FUNCTION

    read_type_modifier -- translate DWARF type modifier into gdb type

DESCRIPTION

    Given a DIE with tag DW_TAG_pointer_type, translate it into gdb
    (struct type *).  Allocate and initialize a slot in utypes array.

NOTES

    For robustness, if we are asked to translate a base type that we
    are unprepared to deal with, we pretend it's builtin_type_int,
    so gdb may at least do something reasonable by default. 
	
    If the type is not in the range of those types defined as
    application specific types, we also issue a warning.

BUGS

    We currently recognize, but mishandle, tags const_type, packed_type, 
    and volatile_type, since gdb has no processing available for them.
    As a workaround we'll just point them to the type they modify.
*/

static void
read_type_modifier(die)
    Dwarf_Die die;
{
    Dwarf_Half tag;
    struct type *base_type;  /* type pointed to */
    struct type *type;  /* New type with pointer-to added */

    base_type = decode_die_type(die, 1);
    tag = DIE_TAG(die);

    switch ( tag )
    {
    case DW_TAG_pointer_type:
	type = lookup_pointer_type(base_type);
	break;
    case DW_TAG_reference_type:
	type = lookup_reference_type(base_type);
	break;
    case DW_TAG_const_type:
    case DW_TAG_packed_type:
    case DW_TAG_volatile_type:
	/* Ignore silently */
	type = base_type;
	break;
    default:
	/* Ignore vocally */
	complain (&unknown_type_modifier, DIE_ID(die), DIE_NAME(die), tag);
	type = base_type;
	break;
    }
    alloc_utype(die, type);
}

/*

LOCAL FUNCTION

    Free storage allocated for a type field list.

    Works if called either by hand or in a struct cleanup. 

*/

static void
free_fieldlist(ptr)
    PTR ptr;
{
    register struct nextfield *p, *q;

    for ( p = (struct nextfield *) ptr; p; p = q )
    {
	q = p->next;
	free(p);
    }
}

/*

LOCAL FUNCTION

	read_struct_type -- process all dies owned by struct or union

DESCRIPTION

	Called when we find the DIE that starts a structure, union, or class
	definition to process all dies that define the members of the 
	structure or union.

NOTES

	If DIE represents an incomplete struct/union then suppress creating 
	a symbol table entry for it since gdb only wants to find the one 
	with the complete definition.  Note that if it is complete, we just
	call new_symbol, which does its own checking about whether the 
	struct/union is anonymous or not (and suppresses creating a symbol 
	table entry itself).
	
 */

static void
read_struct_type (die)
    Dwarf_Die die;
{
    struct type *type;
    struct nextfield *fieldlist;	/* list of struct members */
    struct cleanup *back_to;
    char *name;

    /* If there is a partially-constructed type (from a forward reference)
       the code below molds it into a structure type.  Else allocate a new
       type and set the fields from scratch. */
    if ( (type = lookup_utype (die)) == NULL )
	type = alloc_utype (die, NULL);
    
    INIT_CPLUS_SPECIFIC(type);
    switch ( DIE_TAG(die) )
    {
    case DW_TAG_class_type:
        TYPE_CODE (type) = TYPE_CODE_CLASS;
	break;
    case DW_TAG_structure_type:
        TYPE_CODE (type) = TYPE_CODE_STRUCT;
	break;
    case DW_TAG_union_type:
	TYPE_CODE (type) = TYPE_CODE_UNION;
	break;
    default:
	/* Should never happen */
	TYPE_CODE (type) = TYPE_CODE_UNDEF;
	complain (&missing_tag, DIE_ID(die), DIE_NAME(die));
	break;
    }

    /* Some compilers try to be helpful by inventing "fake" names for
       anonymous enums, structures, and unions, like "~0fake" or ".0fake".
       Thanks, but no thanks... */
    if ( (name = DIE_ATTR_STRING(die, DW_AT_name)) 
	&& *name != '~'
	&& *name != '.' )
    {
	TYPE_TAG_NAME (type) = obconcat (&current_objfile->type_obstack, "", "", name);
    }

    /* Use whatever size is known.  Zero is a valid size.  We might however
       wish to check has_at_byte_size to make sure that some byte size was
       given explicitly, but DWARF doesn't specify that explicit sizes of
       zero have to present, so complaining about missing sizes should 
       probably not be the default. */
    TYPE_LENGTH (type) = DIE_ATTR_UNSIGNED(die, DW_AT_byte_size);

    /* Now process the children DIEs (the members.)  We pass an empty list
       of struct field to read_member_type via process_die.  Each invocation 
       of read_member_type will add a field to the head of the list, creating
       a list that is backwards from the original source.  Note that if there
       are no children, process_children returns immediately with the list
       still empty. */
    fieldlist = NULL;
    back_to = make_cleanup(free_fieldlist, fieldlist);
    process_children(dwarf_child(current_dwfile, die), (PTR) &fieldlist);

    if ( fieldlist )
    {
	/* Find out how many fields there are */
	int n, nfields;
	struct nextfield *fp;
	struct symbol *sym;

	for ( nfields = 0, fp = fieldlist; fp; ++nfields, fp = fp->next )
	    ;
	TYPE_NFIELDS (type) = nfields;
	TYPE_FIELDS (type) = (struct field *)
	    TYPE_ALLOC (type, sizeof (struct field) * nfields);
	/* Copy the saved-up fields into the field vector.  This reverses
	   the order of the list, which puts it back to source file order. */
	for ( n = nfields - 1, fp = fieldlist; fp; fp = fp->next, --n )
	{
	    TYPE_FIELD (type, n) = fp->field;
	}
	/* Now record this DIE as a gdb symbol. */
	sym = new_symbol (die);
	if (sym != NULL)
	{
	    SYMBOL_TYPE (sym) = type;
	    if (cu_language == language_cplus)
	    {
		synthesize_typedef (die, type);
	    }
	}
    }
    else
    {
	/* Must be an incomplete type; set STUB flag to clue gdb in to the
	   fact that it needs to search elsewhere for the full definition. */
	TYPE_NFIELDS (type) = 0;
	TYPE_FLAGS (type) |= TYPE_FLAG_STUB;
    }
    /* Everything's OK, so free memory malloc'ed in process_children */
    do_cleanups (back_to);
}

/*

LOCAL FUNCTION

	read_member_type -- process one struct or union member DIE

DESCRIPTION

	Called from process_children when we have already processed a DIE
	that starts a structure, union, or class definition.

	The parameter PARENTPTR is passed in from read_struct_type via
	process_die and represents the head of a list of (struct field *).
	It is up to us to add to this list after we decode the member.
	Add to the head of the list since the parent will peel the members
	off in reverse order anyway.

 */

static void 
read_member_type (die, parentptr)
    Dwarf_Die die;
    PTR parentptr;
{
    struct nextfield **fieldlist = (struct nextfield **) parentptr;
    struct nextfield *fp;
    int anonymous_size;
    char *name;
    Dwarf_Locdesc loc; /* location list; for data_member_location attribute */

    /* Get space to record the next field's data and add to head of list.
       Will be freed in read_struct_type */
    fp = (struct nextfield *) xmalloc (sizeof (struct nextfield));
    fp->next = *fieldlist;
    *fieldlist = fp;

    /* Save the data. */
    name = DIE_ATTR_STRING(die, DW_AT_name);
    fp->field.name = obsavestring (name,
				   strlen (name),
				   &current_objfile->type_obstack);
    fp->field.type = decode_die_type (die, 1);

    /* In Dwarf2, the data_member_location is an oddball attribute: 
       it is a location expression that ASSUMES that the address of the 
       containing structure is already on the location stack.  Since gdb
       only wants the offset from the containing structure anyway, trick
       Dwarf by pushing a 0 onto the stack first, then evaluating the 
       location expression. */
    loc = DIE_ATTR_LOCLIST(die, DW_AT_data_member_location);
    if ( loc )
    {
	locpush(0);
	dwarf2_eval_location_list(NULL, loc, 0);
	fp->field.bitpos = 8 * locpop();
    }
    else
	fp->field.bitpos = 0;

    /* Handle bit fields. */
    fp->field.bitsize = DIE_ATTR_UNSIGNED(die, DW_AT_bit_size);
#if defined(IMSTG)		/* Intel BIG_ENDIAN mods */
    /* We figure this out at runtime, not at configure time */
    if ( BITS_BIG_ENDIAN ) 
    {
	/* For big endian bits, the at_bit_offset gives the additional
	   bit offset from the MSB of the containing anonymous object to
	   the MSB of the field.  We don't have to do anything special
	   since we don't need to know the size of the anonymous object. */
	fp->field.bitpos += DIE_ATTR_UNSIGNED(die, DW_AT_bit_offset);
    }
    else
    {
	/* For little endian bits, we need to have a non-zero at_bit_size,
	   so that we know we are in fact dealing with a bitfield.  Compute
	   the bit offset to the MSB of the anonymous object, subtract off
	   the number of bits from the MSB of the field to the MSB of the
	   object, and then subtract off the number of bits of the field
	   itself.  The result is the bit offset of the LSB of the field. */
	if ( DIE_ATTR_UNSIGNED(die, DW_AT_bit_size) )
	{
	    unsigned long bit_offset, bit_size;

	    if ( DIE_HAS_ATTR(die, DW_AT_byte_size) )
	    {
		/* The size of the anonymous object containing the bit field
		   is explicit, so use the indicated size (in bytes). */
		anonymous_size = DIE_ATTR_UNSIGNED(die, DW_AT_byte_size);
	    }
	    else
	    {
		/* The size of the anonymous object containing the bit field
		   matches the size of an object of the bit field's type.
		   DWARF allows at_byte_size to be left out in such cases,
		   as a debug information size optimization. */
		anonymous_size = TYPE_LENGTH (fp->field.type);
	    }
	    bit_offset = DIE_ATTR_UNSIGNED(die, DW_AT_bit_offset);
	    bit_size = DIE_ATTR_UNSIGNED(die, DW_AT_bit_size);
	    fp->field.bitpos +=
		anonymous_size * 8 
		    - bit_offset
			- bit_size;
	}
    }
#else
    /* Not IMSTG */
#if BITS_BIG_ENDIAN
    /* For big endian bits, the at_bit_offset gives the additional
       bit offset from the MSB of the containing anonymous object to
       the MSB of the field.  We don't have to do anything special
       since we don't need to know the size of the anonymous object. */
    fp->field.bitpos += DIE_ATTR_UNSIGNED(die, DW_AT_bit_offset);
#else
    /* For little endian bits, we need to have a non-zero at_bit_size,
       so that we know we are in fact dealing with a bitfield.  Compute
       the bit offset to the MSB of the anonymous object, subtract off
       the number of bits from the MSB of the field to the MSB of the
       object, and then subtract off the number of bits of the field
       itself.  The result is the bit offset of the LSB of the field. */
    if ( DIE_ATTR_UNSIGNED(die, DW_AT_bit_size) > 0 )
    {
	unsigned long bit_offset, bit_size;

	if ( DIE_HAS_ATTR(die, DW_AT_byte_size) )
	{
	    /* The size of the anonymous object containing the bit field
	       is explicit, so use the indicated size (in bytes). */
	    anonymous_size = DIE_ATTR_UNSIGNED(die, DW_AT_byte_size);
	}
	else
	{
	    /* The size of the anonymous object containing the bit field
	       matches the size of an object of the bit field's type.
	       DWARF allows at_byte_size to be left out in such cases,
	       as a debug information size optimization. */
	    anonymous_size = TYPE_LENGTH (fp->field.type);
	}
	bit_offset = DIE_ATTR_UNSIGNED(die, DW_AT_bit_offset);
	bit_size = DIE_ATTR_UNSIGNED(die, DW_AT_bit_size);
	fp->field.bitpos +=
	    anonymous_size * 8 
		- bit_offset
		    - bit_size;
    }
#endif /* BITS_BIG_ENDIAN */
#endif /* IMSTG */
}

/*

LOCAL FUNCTION

    Free storage allocated for an array range list (NOT the types pointed to!)

    Works if called either by hand or in a struct cleanup. 

*/

static void
free_typelist(ptr)
    PTR ptr;
{
    register struct typelist *p, *q;

    for ( p = (struct typelist *) ptr; p; p = q )
    {
	q = p->next;
	free(p);
    }
}

/*

LOCAL FUNCTION

	read_array_type -- read DW_TAG_array_type DIE

DESCRIPTION

	Extract all information from a DW_TAG_array_type DIE and add to
	the user defined type vector.

	The array type DIE should have a type attribute that describes the
	array elements.  If it does not, we don't die, we complain and keep
	going with builtin_type_int.

 */

static void
read_array_type (array)
    Dwarf_Die array;
{
    Dwarf_Die firstchild;
    struct type *arraytype; /* type of the elements + subscript */
    struct type *elemtype;  /* type of the elements */
    struct typelist *rangelist = NULL; /* type list to pass to read_subrange*/
    struct typelist *p;	    /* temp */
    struct type *utype;	    /* temp */
    struct cleanup *back_to;

    if ( DIE_HAS_ATTR(array, DW_AT_ordering) &&
	 DIE_ATTR_UNSIGNED(array, DW_AT_ordering) != DW_ORD_row_major )
    {
	complain (&not_row_major, DIE_ID(array), DIE_NAME(array));
    }
    
    /* The subrange info lives in the children DIEs.  process_children will
       fill in our rangelist with a list of TYPE_CODE_RANGE types.  This will
       then be recursively applied to the rangetype field of the resulting
       array. */
    back_to = make_cleanup(free_typelist, rangelist);
    process_children(dwarf_child(current_dwfile, array), &rangelist);

    elemtype = decode_die_type(array, 1);
    
    /* There will always be 1 subrange DIE owned by this array DIE.
       But there may have been 2 or more subrange DIEs.  If so, the
       subrange list will be in reverse order from the order seen
       in the source file.  So unroll the list, building array types 
       as you go, and you will have recreated the original type. */
    for ( arraytype = elemtype, p = rangelist; p; p = p->next )
    {
	arraytype = create_array_type(NULL, arraytype, p->type);
    }

    /* Install resulting array into the utypes array */
    if ( (utype = lookup_utype(array)) == NULL )
    {
	/* This is the first reference to this array type.  Store into
	   the utypes array and you're done.  */
	alloc_utype (array, arraytype);
    }
    else if ( TYPE_CODE (utype) == TYPE_CODE_UNDEF )
    {
	/* We have an existing partially constructed type.  There's no 
	   nice way to bash it into the correct type (see read_subroutine)
	   so we'll just copy our newly-constructed type into the existing
	   utype.  We do NOT free arraytype, since it is allocated on an 
	   obstack.  This is not a memory leak, just wasteful. 
	   FIXME-SOMEDAY.  */
	*utype = *arraytype;
    }
    else
    {
	/* We have a DECORATED user type already.  This should not happen. */
	complain (&dup_user_type_definition, DIE_ID(array), DIE_NAME(array));
    }
    /* Everything's OK, so free memory used for the range type list */
    do_cleanups(back_to);
}

/*

LOCAL FUNCTION

    read_subrange_type -- read array subrange info, create range type

DESCRIPTION

    Process subranges.  This will usually (but not necessarily) represent
    the subscripts of an array.
    
    If no type is specified for the type of the subrange it will be set
    to builtin_type_int without complaining.

    The PARENTPTR parameter is passed in from read_array_type via
    process_die and is the address of a list of types that represent the 
    array subscripts.  Add to this list, don't overwrite it, because this
    could be the second, third, etc time this has been called for the same
    parent.

BUGS

    The Dwarf spec (5.10) says that the upper and lower bounds attributes
    can be references to DIE's, not necessarily constants.  This is NOT
    HANDLED (FIXME) i.e. we assume that these attributes are constants.
*/

static void
read_subrange_type (die, parentptr)
    Dwarf_Die die;
    PTR parentptr;
{
    struct typelist **rangetypelist = (struct typelist **) parentptr;
    struct typelist *newtypelist;
    struct type *indextype;
    Dwarf_Die indexdie;
    int lower_bound, upper_bound;

    /* Will be freed in read_array_type or read_enum_type */
    newtypelist = (struct typelist *) xmalloc (sizeof(struct typelist));

    if ( DIE_HAS_ATTR(die, DW_AT_lower_bound) )
	lower_bound = DIE_ATTR_UNSIGNED(die, DW_AT_lower_bound);
    else
    {
	/* According to the Dwarf spec (5.10) the only languages that 
	   have a language-defined default lower bound are C, C++ (default
	   is 0), and Fortran (default is 1).  Others get set to 0, but 
	   with a complaint. */
	switch ( cu_language )
	{
	case language_c:
	case language_cplus:
	    lower_bound = 0;
	    break;
#if 0
	case language_fortran:
	    lower_bound = 1;
	    break;
#endif
	default:
	    lower_bound = 0;
	    complain(&missing_lower_bound, DIE_ID(die), DIE_NAME(die));
	}
    }

    if ( DIE_HAS_ATTR(die, DW_AT_upper_bound) )
	upper_bound = DIE_ATTR_UNSIGNED(die, DW_AT_upper_bound);
    else 
    {
	if ( DIE_HAS_ATTR(die, DW_AT_count) )
	{
	    upper_bound = DIE_ATTR_UNSIGNED(die, DW_AT_count);
	    upper_bound += lower_bound - 1;
	}
	else
	{
	    /* No default values are specified for upper bound */
	    complain(&missing_upper_bound, DIE_ID(die), DIE_NAME(die));
	    upper_bound = UINT_MAX;
	}
    }

    indextype = decode_die_type(die, 0);
	
    newtypelist->type = create_range_type(NULL, indextype, lower_bound, upper_bound);
    newtypelist->next = *rangetypelist;
    *rangetypelist = newtypelist;
}

/*

LOCAL FUNCTION

	read_tag_string_type -- read DW_TAG_string_type DIE

DESCRIPTION

	Extract all information from a DW_TAG_string_type DIE and add to
	the user defined type vector.  It isn't really a user defined
	type, but it behaves like one, with other DIE's using an
	AT_user_def_type attribute to reference it.

NOTES
	
	Used only for FORTRAN and other languages that have a string type.
	(notably not C or C++).  Not implemented.
 */

static void
read_tag_string_type (die)
    Dwarf_Die die;
{
}

/*

LOCAL FUNCTION

	read_subroutine_type -- process DW_TAG_subroutine_type dies

DESCRIPTION

	Handle DIES due to C code like:

	struct foo {
	    int (*funcp)(int a, long l);  (Generates DW_TAG_subroutine_type)
	    int b;
	};

NOTES

	The children of the subroutine type DIE represent the parameters
	(to provide type info for the parameters) or else there is a 
	single child DIE with tag DW_TAG_unspecified_parameters.

	The children ARE NOT PROCESSED (not implemented yet, nor am I even
	sure that gdb can make use of this info)

 */

static void
read_subroutine_type (die)
    Dwarf_Die die;
{
    struct type *rtntype;	/* Type that this function returns */
    struct type *ftype;		/* Function that returns above type */

    /* Decode the type that this subroutine returns */
    if ( DIE_HAS_ATTR(die, DW_AT_type) )
	rtntype = decode_die_type (die, 1);
    else
	rtntype = builtin_type_void;
    
    /* Install into the utypes array */
    if ( (ftype = lookup_utype(die)) == NULL )
    {
	/* This is the first reference to this subroutine type.  Make
	   function-returning-type from this type and store it. */
	ftype = lookup_function_type (rtntype);
	alloc_utype (die, ftype);
    }
    else if ( TYPE_CODE (ftype) == TYPE_CODE_UNDEF )
    {
	/* We have an existing partially constructed type, so bash it
	   into the correct type. */
	TYPE_TARGET_TYPE (ftype) = rtntype;
	TYPE_FUNCTION_TYPE (rtntype) = ftype;
	TYPE_LENGTH (ftype) = 1;
	TYPE_CODE (ftype) = TYPE_CODE_FUNC;
    }
    else
    {
	complain (&dup_user_type_definition, DIE_ID(die), DIE_NAME(die));
    }
}

/*

LOCAL FUNCTION

    read_enum_type -- process dies which define an enumeration

DESCRIPTION

    Given a pointer to a die which begins an enumeration, process all
    the dies that define the members of the enumeration.

NOTES

    While Dwarf1 specified that enumerators be emitted in reverse order from
    the source file, Dwarf2 specifies that enumerators be emitted in the 
    same order that they were seen in the source.  (which is what gdb wants
    anyway.)

 */

static void
read_enum_type (die)
    Dwarf_Die die;
{
    struct type *type;
    struct nextfield *fieldlist;
    struct symbol *sym;
    struct cleanup *back_to;
    char *name;

    /* If there is a partially-constructed type (from a forward reference)
       the code below molds it into an enum type.  Else allocate a new
       type and set the fields from scratch. */
    if ( (type = lookup_utype (die)) == NULL )
	type = alloc_utype (die, NULL);
    
    TYPE_CODE (type) = TYPE_CODE_ENUM;

    /* Some compilers try to be helpful by inventing "fake" names for
       anonymous enums, structures, and unions, like "~0fake" or ".0fake".
       Thanks, but no thanks... */
    if ( (name = DIE_ATTR_STRING(die, DW_AT_name)) 
	&& *name != '~'
	&& *name != '.' )
    {
	TYPE_TAG_NAME (type) = obconcat (&current_objfile->type_obstack, "", "", name);
    }

    TYPE_LENGTH (type) = DIE_ATTR_UNSIGNED(die, DW_AT_byte_size);

    /* Now process the children DIEs (the members.)  We pass an empty list
       of struct field to read_enumerator_type via process_die.  Each 
       invocation of read_enumerator_type will add a field to the head of 
       the list, creating a list that is backwards from the original source.
       Note that if there are no children, process_children returns 
       immediately with the list still empty. */
    fieldlist = NULL;
    back_to = make_cleanup(free_fieldlist, fieldlist);
    process_children(dwarf_child(current_dwfile, die), (PTR) &fieldlist);

    if ( fieldlist )
    {
	/* Find out how many fields there are */
	int n, nfields;
	struct nextfield *fp;
	struct symbol *sym;

	for ( nfields = 0, fp = fieldlist; fp; ++nfields, fp = fp->next )
	    ;
	TYPE_NFIELDS (type) = nfields;
	TYPE_FIELDS (type) = (struct field *)
	    TYPE_ALLOC (type, sizeof (struct field) * nfields);
	/* Copy the saved-up fields into the field vector.  This reverses
	   the order of the list, which puts it back to source file order. 
	   While we're at it, set each enum literal symbol's type field to
	   the type we're building for the enum as a whole. */
	for ( n = nfields - 1, fp = fieldlist; fp; fp = fp->next, --n )
	{
	    TYPE_FIELD (type, n) = fp->field;
	    SYMBOL_TYPE (fp->sym) = type;
	}
    }
    
    /* Now record this DIE as a gdb symbol. */
    sym = new_symbol (die);
    if (sym != NULL)
    {
	SYMBOL_TYPE (sym) = type;
	if (cu_language == language_cplus)
	{
	    synthesize_typedef (die, type);
	}
    }
    /* Everything's OK, so free memory malloc'ed in process_children */
    do_cleanups (back_to);
}


/*
  
LOCAL FUNCTION

    read_enumerator_type -- process one enumeration literal DIE

DESCRIPTION

    Called from process_children when we have already processed a DIE
    that starts an enumeration.

    The parameter PARENTPTR is passed in from read_enum_type via
    process_die and represents the head of a list of (struct field *).
    It is up to us to add to this list after we decode the member.
    Add to the head of the list since the parent will peel the members
    off in reverse order anyway.

NOTES

    With this parentptr system, there's no way to also pass down the type
    of the parent.  This would be nice since gdb's convention is to set the
    type of the symbol that stands for an enum literal to match the type of 
    the enum.  So we'll store a pointer to the enum literal symbol in the 
    nextfield list so that the parent can set the type.

 */

static void
read_enumerator_type (die, parentptr)
    Dwarf_Die die;
    PTR parentptr;
{
    struct nextfield **fieldlist = (struct nextfield **) parentptr;
    struct nextfield *fp;
    struct symbol *sym;
    char *name;

    /* Get space to record the next field's data and add to head of list.
       Will be freed in read_enum_type */
    fp = (struct nextfield *) xmalloc (sizeof (struct nextfield));
    fp->next = *fieldlist;
    *fieldlist = fp;

    /* Save the data. */
    fp->field.type = NULL;
    fp->field.bitsize = 0;
    fp->field.bitpos = DIE_ATTR_UNSIGNED(die, DW_AT_const_value);
    name = DIE_ATTR_STRING(die, DW_AT_name);
    fp->field.name = obsavestring (name,
				   strlen (name),
				   &current_objfile->type_obstack);

    /* Handcraft a new symbol for this enum member. */
    sym = (struct symbol *) obstack_alloc (&current_objfile->symbol_obstack,
					   sizeof (struct symbol));
    memset (sym, 0, sizeof (struct symbol));
    SYMBOL_NAME (sym) = create_name (fp->field.name,
				     &current_objfile->symbol_obstack);
    SYMBOL_INIT_LANGUAGE_SPECIFIC (sym, cu_language);
    SYMBOL_NAMESPACE (sym) = VAR_NAMESPACE;
    SYMBOL_CLASS (sym) = LOC_CONST;
    SYMBOL_VALUE (sym) = fp->field.bitpos;
    add_symbol_to_list (sym, list_in_scope);
    
    /* Save symbol pointer on nextfield list so parent can set its type. */
    fp->sym = sym;
}

/*

LOCAL FUNCTION

    read_func_scope -- process all dies within a function scope

DESCRIPTION

    Process all dies within a given function scope.  We are passed
    a pointer to the DIE which starts the function scope.  Store symbols
    into a list whose name is kept in a global "list_in_scope".  Note that
    because of inlined subroutines, this function MAY be called recursively;
    so we must save and restore the value of list_in_scope.

NOTES ON ABSTRACT ORIGIN

    (See Dwarf spec, 3.3.8.)
    Generally speaking, there are three kinds of DW_TAG_subprogram DIE:
    1. It has a name, and low_pc/high_pc.  No problem, process normally.
    2. It has a name, but no low_pc/high_pc.  This looks like the root of
       an "abstract instance tree".  Use this DIE to make a gdb symbol.
       This DIE gives you the "static" coordinates such as source file
       line and column number.
    3. It does not have a name, but it does have low_pc/high_pc.  This 
       looks like the root of a "concrete out-of-line instance tree."
       The compiler uses this when the address of an inlined function 
       is taken, or just for convenience when it doesn't know in advance
       whether this function will be inlined.  It must have an 
       abstract_origin attribute, which points you back to the
       subprogram DIE, which gives you the "static" coordinates such as
       name and type.  This DIE gives you the "dynamic" coordinates 
       such as low_pc and high_pc.
    Additionally, for inlining, we will see DW_TAG_inlined_subroutine DIEs.
    This will have the form:
       It does not have a name, but it does have low_pc/high_pc.  It must
       have an abstract_origin attribute, which points you back to the
       subprogram DIE, which gives you the "static" coordinates such as
       name and type.  This DIE gives you the "dynamic" coordinates 
       such as low_pc and high_pc.

*/

static void
read_func_scope (die)
    Dwarf_Die die;  /* Parent (function) DIE of the new function scope */
{
    struct pending **save_list_in_scope = list_in_scope;
    struct symbol *sym = NULL;
    struct context_stack *context;
    char *name = DIE_ATTR_STRING(die, DW_AT_name);
    unsigned long low_pc = DIE_ATTR_UNSIGNED(die, DW_AT_low_pc);
    unsigned long high_pc = DIE_ATTR_UNSIGNED(die, DW_AT_high_pc);

#if defined(IMSTG)
    /* Adjust for runtime position-independence */
    if (DIE_HAS_ATTR(die, DW_AT_low_pc))
	low_pc += picoffset;
    if (DIE_HAS_ATTR(die, DW_AT_high_pc))
	high_pc += picoffset;
#endif

    if ( name )
    {
	sym = new_symbol(die);
	if ( DIE_HAS_ATTR(die, DW_AT_inline) )
	{
	    /* This looks like an abstract instance.  Store this symbol
	       away so that we can find it later. */
	    add_abstract_symbol_to_list(sym, die);
	}
    }
    if ( current_objfile->ei.entry_point >= low_pc 
	&& current_objfile->ei.entry_point <  high_pc )
    {
	current_objfile->ei.entry_func_lowpc = low_pc;
	current_objfile->ei.entry_func_highpc = high_pc;
    }
    
    if ( DIE_HAS_ATTR(die, DW_AT_name) 
	&& STREQ (DIE_ATTR_STRING(die, DW_AT_name), "main") )
    {
	/* FIXME: hardwired name */
	current_objfile->ei.main_func_lowpc = low_pc;
	current_objfile->ei.main_func_highpc = high_pc;
    }
    
    if ( DIE_HAS_ATTR(die, DW_AT_abstract_origin) )
    {
	/* This looks like either a concrete inlined instance
	   (tag DW_TAG_inlined_subroutine) or a concrete out-of-line 
	   instance (tag DW_TAG_subprogram) */
	struct inlin *inlin;
	struct symbol *abstract_sym;
	Dwarf_Die abstract_die =
	    dwarf_cu_offdie(current_dwfile, 
			    current_dwcu,
			    DIE_ATTR_UNSIGNED(die, DW_AT_abstract_origin));

	if (DIE_ATTR_UNSIGNED(abstract_die, DW_AT_declaration))
	    /* The abstract instance was only a declaration, but this one
	       popped out of the compiler anyway.  Not an error, just ignore */
	    return;

	/* Since this is root of a concrete instance tree, store its 
	   DIE pointer in a variable that can be looked up by this DIE's
	   descendants to distinguish concrete inlines from concrete 
	   out-of-line instances. */
	concrete_instance_root = die;

	if ( ! (name = DIE_ATTR_STRING(abstract_die, DW_AT_name)) )
	    error("Unnamed abstract origin DIE");

	if ( ! (abstract_sym = 
		find_abstract_symbol_in_list(name, abstract_die)) )
	    error ("Internal error: can't find abstract instance: %s", name);

	if ( DIE_TAG(die) == DW_TAG_inlined_subroutine )
	{
	    /* Create a new symbol, but inherit some info from the 
	       abstract instance. */
	    sym = raw_symbol(name, LOC_BLOCK);
	    add_symbol_to_list (sym, list_in_scope);
	    SYMBOL_TYPE(sym) = SYMBOL_TYPE(abstract_sym);
	    SYMBOL_ABSTRACT_ORIGIN(sym) = abstract_sym;
	    SYMBOL_VALUE(sym) = low_pc;

	    /* Add this concrete instance to the abstract instance's 
	       inline chain. */
	    inlin = new_inline(sym, current_objfile);
	    if ( SYMBOL_INLINE_LIST(abstract_sym) )
	    {
		/* Add new struct at end of existing list */
		struct inlin *p = SYMBOL_INLINE_LIST(abstract_sym);
		while ( p->next )
		    p = p->next;
		p->next = inlin;
	    }
	    else
		SYMBOL_INLINE_LIST(abstract_sym) = inlin;
	}
	else if ( DIE_TAG(die) == DW_TAG_subprogram )
	{
	    /* This is an out-of-line concrete instance, put here on
	       general principles by the compiler, or because the user
	       code took the address of an inlined function.  Don't create
	       a new symbol, decorate the existing abstract instance */
	    sym = abstract_sym;
	}
	else
	{
	    error("Unimplemented; abstract origin for tag 0x%x", 
		  DIE_TAG(die));
	}
    }
    
    /* Process the children DIEs */
    list_in_scope = &local_symbols;
    context = push_context (0, low_pc);
    context->name = sym;
    context->concrete_instance_root = (void *) concrete_instance_root;
    process_children (dwarf_child(current_dwfile, die), NULL);
    context = pop_context ();
    concrete_instance_root = (Dwarf_Die) context->concrete_instance_root;
    finish_block (context->name, &local_symbols, context->old_blocks, 
		  low_pc, high_pc, current_objfile,
		  low_pc == 0 && high_pc == 0);
    local_symbols = context->locals;
    list_in_scope = save_list_in_scope;
}

/*

LOCAL FUNCTION

    read_variable -- process variable or formal parameter

DESCRIPTION

    Front end to new_symbol.  Handles details of abstract origin.

NOTES ON ABSTRACT ORIGIN

    (See Dwarf spec, 3.3.8.  See also read_func_scope.)
    If it is here, a DIE has tag DW_TAG_variable or DW_TAG_formal_parameter.
    Its attributes fall into one of three categories:
    1. It has a name, and a location.  No problem, process normally.
    2. It has a name, but no location.  This looks like a member of
       an "abstract instance tree".  Use this DIE to make a gdb symbol.
       This DIE gives you the "static" coordinates such as source file
       line and column number.
    3. It does not have a name, but it has a location and an abstract
       origin.  This looks like a member of a "concrete instance tree."
       This DIE gives you the "dynamic" coordinates, i.e. how to find
       this instance at runtime.
*/

static void
read_variable (die)
    Dwarf_Die die;
{
    struct symbol *sym = NULL;
    char *name = DIE_ATTR_STRING(die, DW_AT_name);

    if ( name )
    {
	sym = new_symbol(die);
	if ( ! DIE_HAS_ATTR(die, DW_AT_location) )
	{
	    /* This looks like an abstract instance.  Store this symbol
	       away so that we can find it later. */
	    add_abstract_symbol_to_list(sym, die);
	}
    }
    if (DIE_HAS_ATTR(die, DW_AT_location) ||
	DIE_HAS_ATTR(die, DW_AT_const_value))
    {
	int is_constant = DIE_HAS_ATTR(die, DW_AT_const_value);
	if ( DIE_HAS_ATTR(die, DW_AT_abstract_origin) )
	{
	    /* This looks like either a concrete inline instance
	       (root of tree is DW_TAG_inlined_subroutine) or a concrete 
	       out-of-line instance (root of tree is DW_TAG_subprogram) */
	    struct inlin *inlin;
	    struct symbol *abstract_sym;

	    Dwarf_Die abstract_die =
		dwarf_cu_offdie(current_dwfile, 
				current_dwcu,
				DIE_ATTR_UNSIGNED(die, DW_AT_abstract_origin));
	    if ( ! (name = DIE_ATTR_STRING(abstract_die, DW_AT_name)) )
		error("Unnamed abstract origin DIE");

	    if ( ! (abstract_sym = 
		    find_abstract_symbol_in_list(name, abstract_die)) )
		error ("Internal error: can't find abstract instance: %s", 
		       name);

	    if (DIE_TAG(concrete_instance_root) == DW_TAG_inlined_subroutine)
	    {
		/* Create a new symbol, but inherit some info from the 
		   abstract instance. */
		sym = raw_symbol(name, is_constant ? LOC_CONST : LOC_INDIRECT);
		add_symbol_to_list (sym, list_in_scope);
		SYMBOL_TYPE(sym) = SYMBOL_TYPE(abstract_sym);
		SYMBOL_ABSTRACT_ORIGIN(sym) = abstract_sym;

		/* Add this concrete instance to the abstract instance's 
		   inline chain. */
		inlin = new_inline(sym, current_objfile);
		if ( SYMBOL_INLINE_LIST(abstract_sym) )
		{
		    /* Add new struct at end of existing list */
		    struct inlin *p = SYMBOL_INLINE_LIST(abstract_sym);
		    while ( p->next )
			p = p->next;
		    p->next = inlin;
		}
		else
		    SYMBOL_INLINE_LIST(abstract_sym) = inlin;
	    }
	    else if (DIE_TAG(concrete_instance_root) == DW_TAG_subprogram)
	    {
		/* This is an out-of-line concrete instance, put here on
		   general principles by the compiler, or because the user
		   code took the address of an inlined function.  Don't create
		   a new symbol, decorate the existing abstract instance */
		sym = abstract_sym;
		add_symbol_to_list (sym, list_in_scope);
	    }
	    else
	    {
		error("Unimplemented; concrete instance root is 0x%x", 
		      DIE_TAG(concrete_instance_root));
	    }
	}

	/* OK, we have a gdb symbol, with address class defaulted.
	   Store a pointer to a Dwarf2 location list (for variables with 
	   a real-live location).  The list will be evaluated as needed 
	   (but not now) by sym->class_indirect */
	if ( is_constant )
	{
	    /* The compiler has optimized this variable out of existence
	       and replaced it with a constant value. */
	    dwarf_attr_const_value(die, sym);
	}
	else
	{
	    sym->location_indirect = (PTR) 
		DIE_ATTR_LOCLIST(die, DW_AT_location);
	    sym->class_indirect = dwarf2_eval_location;
	    if ( DIE_TAG(die) == DW_TAG_formal_parameter )
		SYMBOL_CLASS(sym) = LOC_INDIRECT_ARG;
	    else
		SYMBOL_CLASS(sym) = LOC_INDIRECT;
	}
    }
}

/* 
    dwarf_attr_const_value

    Given a DIE and a gdb symbol, set the symbol's value appropriately.
    Useful for Dwarf 2's attribute DW_AT_const_value.
     
    If the symbol's type is unsigned, you're done, since the value
    is already stored in 32 bits.  If signed, then we need to sign extend 
    as appropriate, because the original size that was read in by libdwarf 
    might be < 32 bits.

*/

static void
dwarf_attr_const_value(die, sym)
    Dwarf_Die die;
    struct symbol *sym;
{
    Dwarf_Attribute attr = dwarf_attr(die, DW_AT_const_value);
    unsigned long val32 = attr->at_value.val_unsigned;
    unsigned long mask = 0;
	    
    /* For constants in DATA* forms, libdwarf doesn't know whether the 
       data is signed or unsigned so it always uses unsigned.  Handle
       sign-extension here. */
    if ((sym->type && ! TYPE_UNSIGNED(sym->type))
	|| attr->val_type == DLT_SIGNED)
    {
	switch (attr->at_form)
	{
	case DW_FORM_data1:
	    if ( val32 & 0x80 )
		mask = 0xffffff00;
	    break;
	case DW_FORM_data2:
	    if ( val32 & 0x8000 )
		mask = 0xffff0000;
	    break;
	default:
	    break;
	}
    }
    switch(attr->val_type)
    {
    case DLT_SIGNED:
    case DLT_UNSIGNED:
	SYMBOL_CLASS(sym) = LOC_CONST;
	SYMBOL_VALUE(sym) = val32 | mask;
	break;

    case DLT_BIGNUM:
	/* Libdwarf uses BIGNUM for 8-byte floating-point constants.  
	   It stores the low word first (in word[0]), then the high word.
	   It has converted the constant to host byte-order, but LOC_CONST_BYTES
	   expects target byte-order. */
	if (attr->at_value.val_bignum->numwords == 2)
	{
	    char *buf = (char *)
		obstack_alloc (&current_objfile->symbol_obstack, 8);
	    memcpy (buf, (char *) attr->at_value.val_bignum->words, 4);
	    SWAP_TARGET_AND_HOST(buf, 4);
	    memcpy (buf + 4, (char *) attr->at_value.val_bignum->words + 4, 4);
	    SWAP_TARGET_AND_HOST(buf + 4, 4);
	    SYMBOL_VALUE_BYTES (sym) = buf;
	    SYMBOL_CLASS (sym) = LOC_CONST_BYTES;
	}
	else 
	{
	    complain(&bad_const_value_bignum_size, DIE_ID(die), DIE_NAME(die), 
		     attr->at_value.val_bignum->numwords);
	    SYMBOL_VALUE(sym) = 0;
	    SYMBOL_CLASS(sym) = LOC_CONST;
	}
	break;
    case DLT_BLOCK:
	/* Libdwarf uses BLOCK to deliver IEEE-extended constants, packaged
	   in a 16-byte package.  It stores the words in little-endian order,
	   and the byte-order within the words is target byte-order.
	   Since this is already what GDB expects for IEEE-extended numbers,
	   all we have to do now is copy the libdwarf block into safe memory. */
	if (attr->at_value.val_block->bl_len == 16)
	{
	    char *buf = (char *)
		obstack_alloc (&current_objfile->symbol_obstack, 16);
	    memcpy (buf, (char *) attr->at_value.val_block->bl_data, 16);
	    SYMBOL_VALUE_BYTES (sym) = buf;
	    SYMBOL_CLASS (sym) = LOC_CONST_BYTES;
	}
	else 
	{
	    complain(&bad_const_value_block_size, DIE_ID(die), DIE_NAME(die), 
		     attr->at_value.val_block->bl_len);
	    SYMBOL_VALUE(sym) = 0;
	    SYMBOL_CLASS(sym) = LOC_CONST;
	}
	break;
    case DLT_STRING:
	{
	    int len = strlen(attr->at_value.val_string);
	    char *buf = (char *)
		obstack_alloc (&current_objfile->symbol_obstack, len + 1);
	    strcpy(buf, attr->at_value.val_string);
	    SYMBOL_VALUE_BYTES (sym) = buf;
	    SYMBOL_CLASS (sym) = LOC_CONST_BYTES;
	}
	break;
    default:
	complain(&bad_const_value_form, DIE_ID(die), DIE_NAME(die), 
		 attr->val_type);
	SYMBOL_VALUE(sym) = 0;
	SYMBOL_CLASS(sym) = LOC_CONST;
	break;
    }
}
    
/*

LOCAL FUNCTION

	handle_producer -- process the AT_producer attribute

DESCRIPTION

	Perform any operations that depend on finding a particular
	AT_producer attribute.

 */

static void
handle_producer (producer)
    char *producer;
{
    /* If this compilation unit was compiled with g++ or gcc, then set the
       processing_gcc_compilation flag. */
    processing_gcc_compilation =
	STREQN (producer, GPLUS_PRODUCER, strlen (GPLUS_PRODUCER))
	    || STREQN (producer, CHILL_PRODUCER, strlen (CHILL_PRODUCER))
		|| STREQN (producer, GCC_PRODUCER, strlen (GCC_PRODUCER));
    
    /* Select a demangling style if we can identify the producer and if
       the current style is auto.  We leave the current style alone if it
       is not auto.  We also leave the demangling style alone if we find a
       gcc (cc1) producer, as opposed to a g++ (cc1plus) producer. */
    
    if (AUTO_DEMANGLING)
    {
	if (STREQN (producer, GPLUS_PRODUCER, strlen (GPLUS_PRODUCER)))
	{
	    set_demangling_style (GNU_DEMANGLING_STYLE_STRING);
	}
	else if (STREQN (producer, LCC_PRODUCER, strlen (LCC_PRODUCER)))
	{
	    set_demangling_style (LUCID_DEMANGLING_STYLE_STRING);
	}
    }
}


/*

LOCAL FUNCTION

	read_file_scope -- process all dies within a file scope

DESCRIPTION

	Process all dies within a given file scope.  We are passed a
	pointer to the die information structure for the die which
	starts the file scope.

	To maintain GDB's separation of file symbols and local symbols,
	we will read the file in 2 passes.  This is because the compiler 
	may emit local DIES whose type is defined later, at the file level.
	The recursive nature of Dwarf type processing can't tell the difference.
	So we'll read all file-level DIES first, skipping subprograms and all 
	their children, then go back over the file again just for subprograms
	to pick up local symbols.
	
SIDE EFFECT
	Creates GDB's symtab for this file.  Sets its language
	and linetable_has_columns.
 */

static void
read_file_scope ()
{
    struct cleanup *back_to;
    struct symtab *symtab;
    char *filename;
    char *comp_dir;
    char *producer;
    unsigned long low_pc, high_pc;
    Dwarf_Die die;
    
    low_pc = DIE_ATTR_UNSIGNED(current_dwcu, DW_AT_low_pc);
    high_pc = DIE_ATTR_UNSIGNED(current_dwcu, DW_AT_high_pc);

#if defined(IMSTG)
    /* Adjust for runtime position-independence */
    low_pc += picoffset;
    high_pc += picoffset;
#endif
    
    if ( current_objfile->ei.entry_point >= low_pc 
	&& current_objfile->ei.entry_point < high_pc )
    {
	current_objfile->ei.entry_file_lowpc = low_pc;
	current_objfile->ei.entry_file_highpc = high_pc;
    }
    set_cu_language (current_dwcu);
    
    if ( producer = DIE_ATTR_STRING(current_dwcu, DW_AT_producer) )
    {
	handle_producer(producer);
    }

    /* Allocate space for the global type lookup table.  How many types should
       be allocated?  Pick a number: if more are needed later the table will 
       be realloc'ed */
    numutypes = 1024;
    utypes = (struct type **) xmalloc (numutypes * sizeof (struct type *));
    memset(utypes, 0, numutypes * sizeof(struct type *));
    back_to = make_cleanup (free_utypes, NULL);
    make_cleanup (free_abstract_symbol_list, NULL);

    filename = DIE_ATTR_STRING(current_dwcu, DW_AT_name);
    comp_dir = DIE_ATTR_STRING(current_dwcu, DW_AT_comp_dir);

    start_symtab (filename, comp_dir, low_pc);

    decode_line_numbers ();

    pass_number = 1;
    process_children (dwarf_child(current_dwfile, current_dwcu), NULL);

    pass_number = 2;
    for (die = dwarf_child(current_dwfile, current_dwcu);
	 die && DIE_TAG(die);
	 die = dwarf_siblingof(current_dwfile, die) )
    {
	if ( DIE_TAG(die) == DW_TAG_subprogram )
	    process_die (die, NULL);
    }
    pass_number = 0;
    symtab = end_symtab (high_pc, 0, 1, current_objfile, 0);

    if (symtab != NULL)
    {
	symtab->language = cu_language;
	symtab->linetable_has_columns = 1;
    }      
    do_cleanups (back_to);
    utypes = NULL;
    numutypes = 0;
}

/*

LOCAL FUNCTION

	read_lexical_block_scope -- process all dies in a lexical block

DESCRIPTION

	Process all the DIES contained within a lexical block scope.
	Start a new scope, process the dies, and then close the scope.

 */

static void
read_lexical_block_scope (die)
    Dwarf_Die die;  /* Parent DIE of the new block scope */
{
    register struct context_stack *context;
    unsigned long low_pc = DIE_ATTR_UNSIGNED(die, DW_AT_low_pc);
    unsigned long high_pc = DIE_ATTR_UNSIGNED(die, DW_AT_high_pc);
    
#if defined(IMSTG)
    /* Adjust for runtime position-independence */
    if (DIE_HAS_ATTR(die, DW_AT_low_pc))
	low_pc += picoffset;
    if (DIE_HAS_ATTR(die, DW_AT_high_pc))
	high_pc += picoffset;
#endif

    context = push_context (0, low_pc);
    context->concrete_instance_root = (void *) concrete_instance_root;
    if ( dwarf_child(current_dwfile, die) )
	process_children (dwarf_child(current_dwfile, die), NULL);
    context = pop_context ();
    concrete_instance_root = (Dwarf_Die) context->concrete_instance_root;
    if ( local_symbols )
    {
	finish_block (0, &local_symbols, context->old_blocks, 
		      context->start_addr,
		      high_pc,
		      current_objfile,
		      low_pc == 0 && high_pc == 0);
    }
    local_symbols = context->locals;
}

/*

LOCAL FUNCTION

    process_die -- translate a single DIE from Dwarf2->gdb structures

DESCRIPTION

    Called on each DIE during the process of expanding partial symbols 
    into full symbols.  May be (and almost certainly will be) called 
    recursively.

    The PARENTPTR pointer allows the parent invocation to pass information
    to its invocation of process_children (usually, but not necessarily,
    a (struct type *).)

*/

static void
process_die (die, parentptr)
    Dwarf_Die die;
    PTR parentptr;
{
    struct type *type;
    
    if ( (type = lookup_utype(die)) && TYPE_CODE(type) != TYPE_CODE_UNDEF )
	/* There is a filled-in utype entry for this DIE; this means
	   we read this DIE earlier, probably a call from decode_die_type */
	return;
    
    switch ( DIE_TAG(die) )
    {
    case DW_TAG_compile_unit:
	/* Skip compile_unit since we're already inside a compilation unit. */
	break;
    case DW_TAG_subprogram:
    case DW_TAG_inlined_subroutine:
	/* Skip subprograms the first time through the Dwarf file
	   (see comment at read_file_scope)  Also, skip subprograms 
	   that are only declarations, not definitions, as evidenced
	   by the DW_AT_declaration attribute (see Dwarf spec, 2.11) */
	if ( pass_number == 2 && ! DIE_ATTR_UNSIGNED(die, DW_AT_declaration) )
	    read_func_scope (die);
	break;
    case DW_TAG_variable:
    case DW_TAG_formal_parameter:
	/* Skip variables that are only declarations, not definitions, 
	   as evidenced by the DW_AT_declaration attribute (see Dwarf 
	   spec, 2.11) */
	if ( ! DIE_ATTR_UNSIGNED(die, DW_AT_declaration) )
	    read_variable(die);
	break;
    case DW_TAG_lexical_block:
	/* Skip lexical blocks the first time through the Dwarf file
	   because their scope must mesh with subprograms. (see comment 
	   at read_file_scope) */
	if ( pass_number == 2 )
	    read_lexical_block_scope (die);
	break;
    case DW_TAG_class_type:
    case DW_TAG_structure_type:
    case DW_TAG_union_type:
	read_struct_type (die);
	break;
    case DW_TAG_member:
	read_member_type (die, parentptr);
	break;
    case DW_TAG_enumeration_type:
	read_enum_type (die);
	break;
    case DW_TAG_enumerator:
	read_enumerator_type (die, parentptr);
	break;
    case DW_TAG_subroutine_type:
	read_subroutine_type (die);
	break;
    case DW_TAG_array_type:
	read_array_type (die);
	break;
    case DW_TAG_subrange_type:
	read_subrange_type (die, parentptr);
	break;
    case DW_TAG_base_type:
	read_base_type(die);
	break;
    case DW_TAG_pointer_type:
    case DW_TAG_reference_type:
    case DW_TAG_const_type:
    case DW_TAG_volatile_type:
    case DW_TAG_packed_type:
	read_type_modifier(die);
	break;
    case DW_TAG_string_type:
	read_tag_string_type (die);
	break;
    default:
	new_symbol (die);
	break;
    }
}

/*

LOCAL FUNCTION

    process_children -- process all the children of a DIE

DESCRIPTION

    Process all the children of a DIE.  May be (and almost
    certainly will be) called recursively

    The PARENTPTR pointer was passed in from the parent invocation 
    to be passed on to the handler that processes the child DIEs.

 */

static void
process_children (firstchild, parentptr)
    Dwarf_Die firstchild;
    PTR parentptr;
{
    Dwarf_Die die;

    for ( die = firstchild; die && DIE_TAG(die);
	 die = dwarf_siblingof(current_dwfile, die) )
    {
	process_die(die, parentptr);
    }
}

/* Add a symbol that represents an abstract origin DIE to a list so we
   can find it later.  Used in processing inlined functions.

   Memory for both the list and the entries gets free'd in
   free_abstract_symbol_list (called via a cleanup in read_file_scope)
*/

static
void
add_abstract_symbol_to_list (symbol, die)
    struct symbol *symbol;
    Dwarf_Die die;
{
    struct absym *absym = 
	(struct absym *) xmalloc (sizeof(struct absym));
    absym->sym = symbol;
    absym->die = die;

    if (abstract_symbols.count + 1 > abstract_symbols.limit)
    {
	if (abstract_symbols.list) 
	{
	    /* Double the current list size */
	    abstract_symbols.list = 
		(struct absym **) xrealloc(abstract_symbols.list,
					   sizeof(abstract_symbols.list[0]) 
					   * abstract_symbols.limit * 2);
	    memset(abstract_symbols.list + abstract_symbols.limit, 0,
		   abstract_symbols.limit * sizeof(abstract_symbols.list[0]));
	    abstract_symbols.limit *= 2;
	}
	else 
	{
	    /* pick a number; we'll increase it later if necessary */
	    abstract_symbols.limit = 128;
	    abstract_symbols.list = 
		(struct absym **) xmalloc(abstract_symbols.limit * 
					  sizeof(abstract_symbols.list[0]));
	}
    }
    abstract_symbols.list[abstract_symbols.count++] = absym;
}

/* Find an abstract symbol on the global abstract symbol list.  
   Both the NAME of the symbol and the DIE it represents must match.
   This is a tricky way to get around scoping the abstract symbol list.
*/

static
struct symbol *
find_abstract_symbol_in_list (name, die)
    char *name;
    Dwarf_Die die;
{
    register int i = 0;
    register int count = abstract_symbols.count;
    register struct absym **absym = abstract_symbols.list;

    for ( ; i < count; ++i, ++absym )
	if (STREQ(SYMBOL_NAME((*absym)->sym), name) 
	    && (*absym)->die == die)
	    return (*absym)->sym;
    return NULL;
}

static
void
free_abstract_symbol_list()
{
    register int i = 0;
    register int count = abstract_symbols.count;
    register struct absym **absym = abstract_symbols.list;

    for ( ; i < count; ++i, ++absym )
	free(*absym);
    if (abstract_symbols.list)
	free(abstract_symbols.list);
    abstract_symbols.list = NULL;
    abstract_symbols.limit = 0;
    abstract_symbols.count = 0;
}
    
/*

LOCAL FUNCTION

	decode_line_numbers -- decode line numbers for current Comp. Unit

DESCRIPTION

	Translate the DWARF line number information for the current 
	compilation unit into gdb format.  

	For simplicity, read in ALL the line numbers at one time. 
	FIXME: it should be possible to read them in smaller chunks 
	as needed.

BUGS

	Does gdb expect the line numbers to be sorted?  They are now by
	chance/luck, but are not required to be.  (FIXME)

 */

static void
decode_line_numbers ()
{
#ifdef DUMP_LINENOS
    int dumpit;
#endif
    Dwarf_Line linetable = NULL;
    Dwarf_Line line;

    /* Set some locals to point to highly unlikely values.  Use them
       to check for back-to-back duplicates as they are read in. */
    unsigned long lastline = UINT_MAX;
    unsigned long lastcolumn = UINT_MAX;
    unsigned long lastaddress = UINT_MAX;
    char *lastfile = NULL;
    int numln;

    dwarf_seterrarg("Libdwarf error in dwarf_srclines");
    numln = dwarf_srclines(current_dwfile, 
			   current_dwcu, 
			   &linetable);
    dwarf_seterrarg(NULL);

    if ( numln == DLV_NOCOUNT )
	return;

    for ( line = linetable; numln; --numln, ++line )
    {
#ifdef DUMP_LINENOS
	dumpit = 0;
#endif
	if ( line->file && line->file != lastfile )
	{
	    /* Probably there is executable code in an included file. 
	       start_subfile will start a new line number table for it.
	       Or we are returning to the original file.  start_subfile
	       will resume the original line number table.  NOTE1: Both
	       of these cases share the SAME blockvector, i.e. no pushing 
	       and popping of subfiles.  NOTE2: start_subfile sets
	       current_subfile. */
	    start_subfile(line->file, line->dir);
	    lastfile = line->file;
#ifdef DUMP_LINENOS
	    dumpit = 1;
#endif
	}
	    
	if (line->is_stmt &&
	    (line->address != lastaddress || 
	     line->line != lastline ||
	     line->column != lastcolumn))
	{
#if defined(IMSTG)
	    /* Adjust for runtime position-independence */
	    line->address += picoffset;
#endif
	    record_line(current_subfile, 
			lastline = line->line, 
			lastcolumn = line->column,
			lastaddress = line->address);
#ifdef DUMP_LINENOS
	    dumpit = 1;
#endif
	}

#ifdef DUMP_LINENOS
	if ( dumpit )
	    printf_filtered("Line: %10d    Column: %10d    Address: %10x    File: %s\n",
			    line->line, 
			    line->column,
			    line->address,
			    line->file ? line->file : "");
#endif
    }
}

/*

LOCAL FUNCTION

	read_ofile_symtab -- build a full symtab entry from chunk of DIE's

SYNOPSIS

	static void read_ofile_symtab (struct partial_symtab *pst)

DESCRIPTION

	When expanding a partial symbol table entry to a full symbol table
	entry, this is the function that gets called to read in the symbols
	for the compilation unit.  A pointer to the newly constructed symtab,
	which is now the new first one on the objfile's symtab list, is
	stashed in the partial symbol table entry.
	
	Make use of the private data squirrelled away during symbol readin
	(scan_compilation_units).  This is a pointer to the libdwarf DIE
	representing the current compilation unit. 
 */

static void
read_ofile_symtab (pst)
    struct partial_symtab *pst;
{
    Dwarf_Die dwcusib;
    bfd *abfd;
    
    abfd = pst->objfile->obfd;
    current_objfile = pst->objfile;
    current_dwfile = (Dwarf_Debug) pst->objfile->sym_private;
    current_dwcu = (Dwarf_Die) pst->read_symtab_private;

    diecount = 0;
    base_section_offsets = pst->section_offsets;
    
    read_file_scope ();

#ifdef DEBUG
    _dump_tree(current_dwcu);
#endif
    current_objfile = NULL;
    current_dwfile = NULL;
    current_dwcu = NULL;
    pst->symtab = pst->objfile->symtabs;
}

/*

LOCAL FUNCTION

	psymtab_to_symtab_1 -- do grunt work for building a full symtab entry

SYNOPSIS

	static void psymtab_to_symtab_1 (struct partial_symtab *pst)

DESCRIPTION

	Called once for each partial symbol table entry that needs to be
	expanded into a full symbol table entry.

*/

static void
psymtab_to_symtab_1 (pst)
     struct partial_symtab *pst;
{
    int i;
    struct cleanup *old_chain;
    
    if (pst != NULL)
    {
	if (pst->readin)
	{
	    warning ("psymtab for %s already read in.  Shouldn't happen.",
		     pst->filename);
	}
	else
	{
	    QUIT;	/* Make this command interruptable */

	    /* Read in all partial symtabs on which this one is dependent */
	    for (i = 0; i < pst->number_of_dependencies; i++)
	    {
		if (!pst->dependencies[i]->readin)
		{
		    /* Inform about additional files needing to be read in. */
		    if (info_verbose)
		    {
			fputs_filtered (" ", gdb_stdout);
			wrap_here ("");
			fputs_filtered ("and ", gdb_stdout);
			wrap_here ("");
			printf_filtered ("%s...",
					 pst->dependencies[i]->filename);
			wrap_here ("");
			gdb_flush (gdb_stdout); /* Flush output */
		    }
		    psymtab_to_symtab_1 (pst->dependencies[i]);
		}
	    }

	    buildsym_init ();
	    old_chain = make_cleanup (really_free_pendings, 0);
	    read_ofile_symtab (pst);
	    sort_symtab_syms (pst->symtab);
	    do_cleanups (old_chain);
	    pst->readin = 1;
	}
    }
}

/*

LOCAL FUNCTION

	dwarf_psymtab_to_symtab -- build a full symtab entry from partial one

SYNOPSIS

	static void dwarf_psymtab_to_symtab (struct partial_symtab *pst)

DESCRIPTION

	This is the DWARF support entry point for building a full symbol
	table entry from a partial symbol table entry.  We are passed a
	pointer to the partial symbol table entry that needs to be expanded.

*/

static void
dwarf_psymtab_to_symtab (pst)
     struct partial_symtab *pst;
{
    if (pst != NULL)
    {
	if (pst->readin)
	{
	    warning ("psymtab for %s already read in.  Shouldn't happen.",
		     pst->filename);
	}
	else
	{
	    /* Print the message now, before starting serious work, to avoid
	       disconcerting pauses.  */
	    if (info_verbose)
	    {
		printf_filtered ("Reading in symbols for %s...",
				 pst->filename);
		gdb_flush (gdb_stdout);
	    }
	    
	    psymtab_to_symtab_1 (pst);

	    /* Finish up the verbose info message.  */
	    if (info_verbose)
	    {
		printf_filtered ("done.\n");
		gdb_flush (gdb_stdout);
	    }
	}
    }
}

/*

LOCAL FUNCTION

	init_psymbol_list -- initialize storage for partial symbols

DESCRIPTION

	Initializes storage for all of the partial symbols that will be
	created by dwarf_build_psymtabs and subsidiaries.
 */

static void
init_psymbol_list (total_symbols)
     int total_symbols;
{
    /* Free any previously allocated psymbol lists.  */
    if (current_objfile->global_psymbols.list)
    {
	mfree (current_objfile->md, (PTR)current_objfile->global_psymbols.list);
    }
    if (current_objfile->static_psymbols.list)
    {
	mfree (current_objfile->md, (PTR)current_objfile->static_psymbols.list);
    }
  
    /* Current best guess is that approximately a twentieth of the total 
       symbols (in a debugging file) are global or static oriented symbols */
    current_objfile->global_psymbols.size = total_symbols / 10;
    current_objfile->static_psymbols.size = total_symbols / 10;
    current_objfile->global_psymbols.next =
	current_objfile->global_psymbols.list = (struct partial_symbol *)
	    xmmalloc (current_objfile->md, current_objfile->global_psymbols.size
		      * sizeof (struct partial_symbol));
    current_objfile->static_psymbols.next =
	current_objfile->static_psymbols.list = (struct partial_symbol *)
	    xmmalloc (current_objfile->md, current_objfile->static_psymbols.size
		      * sizeof (struct partial_symbol));
}

/*

LOCAL FUNCTION

	add_enum_psymbol -- add enumeration members to partial symbol table

DESCRIPTION

	Given pointer to a DIE that is known to be for an enumeration,
	extract the symbolic names of the enumeration members and add
	partial symbols for them.
*/

static void
add_enum_psymbol (die)
    Dwarf_Die die;
{
    Dwarf_Die firstchild = dwarf_child(current_dwfile, die);
    Dwarf_Half tag;
    char *name;
    struct objfile *objfile = current_objfile; /* To satisfy the heinous macros
						  in symfile.h */

    for ( die = firstchild; 
	 die && (tag = DIE_TAG(die)); 
	 die = dwarf_siblingof(current_dwfile, die) )
    {
	if ( name = DIE_ATTR_STRING(die, DW_AT_name) )
	    ADD_PSYMBOL_TO_LIST (name, 
				 strlen (name),
				 VAR_NAMESPACE, 
				 LOC_CONST,
				 objfile->static_psymbols,
				 0,
				 cu_language,
				 objfile);
    }
}

/*

LOCAL FUNCTION

	add_partial_symbol -- add symbol to partial symbol table

DESCRIPTION

	Given a DIE, if it is one of the types that we want to
	add to a partial symbol table, finish filling in the die info
	and then add a partial symbol table entry for it.

*/

static void
add_partial_symbol (die)
    Dwarf_Die die;
{
    struct objfile *objfile = current_objfile; /* To satisfy the heinous macros
						  in symfile.h */
    char *symname = DIE_ATTR_STRING(die, DW_AT_name);
    unsigned long external = DIE_ATTR_UNSIGNED(die, DW_AT_external);
    unsigned long low_pc = DIE_ATTR_UNSIGNED(die, DW_AT_low_pc);

#if defined(IMSTG)
    /* Adjust for runtime position-independence */
    if (DIE_HAS_ATTR(die, DW_AT_low_pc))
	low_pc += picoffset;
#endif

    if ( symname == NULL )
	return;

    switch ( DIE_TAG(die) )
    {
    case DW_TAG_subprogram:
	if ( external )
	    ADD_PSYMBOL_TO_LIST (symname,
				 strlen (symname),
				 VAR_NAMESPACE, 
				 LOC_BLOCK,
				 current_objfile->global_psymbols,
				 low_pc,
				 cu_language,
				 current_objfile);
	else
	    ADD_PSYMBOL_TO_LIST (symname,
				 strlen (symname),
				 VAR_NAMESPACE, 
				 LOC_BLOCK,
				 current_objfile->static_psymbols,
				 low_pc,
				 cu_language,
				 current_objfile);
	break;
    case DW_TAG_variable:
	if ( external )
	    ADD_PSYMBOL_TO_LIST (symname,
				 strlen (symname),
				 VAR_NAMESPACE, 
				 LOC_INDIRECT,
				 current_objfile->global_psymbols,
				 0,
				 cu_language,
				 current_objfile);
	else
	    ADD_PSYMBOL_TO_LIST (symname,
				 strlen (symname),
				 VAR_NAMESPACE, 
				 LOC_INDIRECT,
				 current_objfile->static_psymbols,
				 0,
				 cu_language,
				 current_objfile);
	break;
    case DW_TAG_typedef:
	ADD_PSYMBOL_TO_LIST (symname,
			     strlen (symname),
			     VAR_NAMESPACE, 
			     LOC_TYPEDEF,
			     current_objfile->static_psymbols,
			     0,
			     cu_language,
			     current_objfile);
	break;
    case DW_TAG_class_type:
    case DW_TAG_structure_type:
    case DW_TAG_union_type:
    case DW_TAG_enumeration_type:
	/* Do not add opaque aggregate definitions to the psymtab.  */
	if ( ! DIE_ATTR_UNSIGNED(die, DW_AT_byte_size) )
	    break;
	ADD_PSYMBOL_TO_LIST (symname,
			     strlen (symname),
			     STRUCT_NAMESPACE, 
			     LOC_TYPEDEF,
			     current_objfile->static_psymbols,
			     0,
			     cu_language,
			     current_objfile);
	if (cu_language == language_cplus)
	{
	    /* For C++, these implicitly act as typedefs as well. */
	    ADD_PSYMBOL_TO_LIST (symname,
				 strlen (symname),
				 VAR_NAMESPACE, 
				 LOC_TYPEDEF,
				 current_objfile->static_psymbols,
				 0,
				 cu_language,
				 current_objfile);
	}
	break;
    }
}

/*

LOCAL FUNCTION

	scan_partial_symbols -- scan DIE's within a single compilation unit

DESCRIPTION

	Process the DIE's within a single compilation unit, looking for
	interesting DIE's that contribute to the partial symbol table entry
	for this compilation unit.

NOTES

	There are some DIE's that may appear both at file scope and within
	the scope of a function.  We are only interested in the ones at file
	scope, and the only way to tell them apart is to keep track of the
	scope.  For example, consider the test case:

		static int i;
		main () { int j; }

	for which the relevant DWARF segment has the structure:
	
		0x51:
		0x23   subprogram      sibling     0x9b
		                       name        main
				       external    true
		                       low_pc      0x800004cc
		                       high_pc     0x800004d4
		                            
		0x74:
		0x23   variable        sibling     0x97
		                       name        j
		                       location    OP_BASEREG 0xe
		                                   OP_CONST 0xfffffffc
		                                   OP_ADD
		0x97:
		0x4         
		
		0x9b:
		0x1d   variable        sibling     0xb8
		                       name        i
		                       location    OP_ADDR 0x800025dc
		                            
		0xb8:
		0x4         

	We want to include the symbol 'i' in the partial symbol table, but
	not the symbol 'j'.  In essence, we want to skip all the dies within
	the scope of an external DW_TAG_subprogram DIE.

	Don't attempt to add anonymous structures or unions since they have
	no name.  Anonymous enumerations however are processed, because we
	want to extract their member names (the check for a tag name is
	done later).

	Also, for variables and subroutines, check that this is the place
	where the actual definition occurs, rather than just a reference
	to an external.

 */

static void
scan_partial_symbols (cu)
    Dwarf_Die cu;    /* The DIE representing the compilation unit to scan */
{
    Dwarf_Die die;

    for (die = dwarf_child(current_dwfile, cu);
	 die;
	 die = dwarf_siblingof(current_dwfile, die))
    {
	switch ( DIE_TAG(die) )
	{
	case DW_TAG_subprogram:
	    if (DIE_HAS_ATTR(die, DW_AT_low_pc) ||
		DIE_HAS_ATTR(die, DW_AT_location))
		add_partial_symbol (die);
	    break;
	case DW_TAG_variable:
	    if (DIE_HAS_ATTR(die, DW_AT_low_pc) ||
		DIE_HAS_ATTR(die, DW_AT_location))
		add_partial_symbol (die);
	    break;
	case DW_TAG_typedef:
	case DW_TAG_class_type:
	case DW_TAG_structure_type:
	case DW_TAG_union_type:
	    add_partial_symbol (die);
	    break;
	case DW_TAG_enumeration_type:
	    add_partial_symbol (die);
	    add_enum_psymbol (die);
	    break;
	default:
	    /* All other tags are ignored */
	    break;
	}
    }
}

/*

LOCAL FUNCTION

	scan_compilation_units -- build a psymtab entry for each compilation

DESCRIPTION

	This is the top level dwarf parsing routine for building partial
	symbol tables.

	It scans from the beginning of the DWARF table looking for the first
	DW_TAG_compile_unit DIE, and then follows the sibling chain to locate
	each additional DW_TAG_compile_unit DIE.
   
	For each DW_TAG_compile_unit DIE it creates a partial symtab structure,
	calls a subordinate routine to collect all the compilation unit's
	global DIE's, file scope DIEs, typedef DIEs, etc, and then links the
	new partial symtab structure into the partial symbol table.  It also
	records the appropriate information in the partial symbol table entry
	to allow the compilation unit DIE to be located and re-read later,
	to generate a complete symbol table entry for the compilation unit.

	Thus it effectively partitions up a chunk of DIE's for multiple
	compilation units into smaller DIE chunks and line number tables,
	and associates them with a partial symbol table entry.

NOTES

	If any compilation unit has no line number table associated with
	it for some reason (a missing at_stmt_list attribute, rather than
	just one with a value of zero, which is valid) then we ensure that
	the recorded file offset is zero so that the routine which later
	reads line number table fragments knows that there is no fragment
	to read.

RETURNS

	Returns no value.

 */

static void
scan_compilation_units ()
{
    Dwarf_Die die;
    Dwarf_Half tag;
    struct partial_symtab *pst;
    char *filename;
    unsigned long low_pc, high_pc;

    for ( die = dwarf_siblingof(current_dwfile, NULL); 
	 die; 
	 die = dwarf_siblingof(current_dwfile, die) )
    {
	QUIT;	/* Make this command interruptable */
	tag = DIE_TAG(die); 
	
	/* Cover rare case where the first DIE is not a Compilation Unit. */
	while ( tag != DW_TAG_compile_unit )
	{
	    if ( (die = dwarf_nextdie(current_dwfile, die)) == NULL )
		return;
	}

	/* We know we have a CU die now. */
	set_cu_language(die);

	low_pc = DIE_ATTR_UNSIGNED(die, DW_AT_low_pc);
	high_pc = DIE_ATTR_UNSIGNED(die, DW_AT_high_pc);
	filename = DIE_ATTR_STRING(die, DW_AT_name);

#if defined(IMSTG)
	/* Adjust for runtime position-independence */
	if (DIE_HAS_ATTR(die, DW_AT_low_pc))
	    low_pc += picoffset;
	if (DIE_HAS_ATTR(die, DW_AT_high_pc))
	    high_pc += picoffset;
#endif

	/* Allocate a new partial symbol table structure */
	pst = start_psymtab_common (current_objfile, 
				    base_section_offsets,
				    filename, 
				    low_pc,
				    current_objfile->global_psymbols.next,
				    current_objfile->static_psymbols.next);
	pst->texthigh = high_pc;

	/* Each partial symbol table entry contains a pointer to private data
	   for the read_symtab() function to use when expanding a partial 
	   symbol table entry to a full symbol table entry.  */
	pst->read_symtab_private = (PTR) die;

	pst->read_symtab = dwarf_psymtab_to_symtab;

	/* Now look for partial symbols */
	scan_partial_symbols (die);
	
	pst->n_global_syms = current_objfile->global_psymbols.next -
	    (current_objfile->global_psymbols.list + pst->globals_offset);
	pst->n_static_syms = current_objfile->static_psymbols.next - 
	    (current_objfile->static_psymbols.list + pst->statics_offset);

	sort_pst_symbols (pst);
    }
}
    
/* raw_symbol -- helper function for new_symbol 

   Create a new symbol.  Makes a copy of the name.
   Allocates the symbol and the name on the current objfile's obstack.
*/
static
struct symbol *
raw_symbol(name, class)
    char *name;
    enum address_class class;
{
    struct symbol *sym = (struct symbol *) 
	obstack_alloc(&current_objfile->symbol_obstack, 
		      sizeof (struct symbol));
    memset(sym, 0, sizeof (struct symbol));
    SYMBOL_NAME(sym) = create_name(name, &current_objfile->symbol_obstack);
    SYMBOL_NAMESPACE (sym) = VAR_NAMESPACE;
    SYMBOL_CLASS (sym) = class;
    return sym;
}

/*

LOCAL FUNCTION

	new_symbol -- make a symbol table entry for a new symbol

DESCRIPTION

	Given a pointer to a DWARF information entry, figure out if we need
	to make a symbol table entry for it, and if so, create a new entry
	and return a pointer to it.
 */

static struct symbol *
new_symbol (die)
    Dwarf_Die die;
{
    struct symbol *sym = NULL;
    char *symname = DIE_ATTR_STRING(die, DW_AT_name);

    if ( symname )
    {
	unsigned int tag = DIE_TAG(die);
	unsigned int low_pc = DIE_ATTR_UNSIGNED(die, DW_AT_low_pc);
	int has_location = DIE_HAS_ATTR(die, DW_AT_location);
	int is_constant = DIE_HAS_ATTR(die, DW_AT_const_value);

#if defined(IMSTG)
	/* Adjust for runtime position-independence */
	if (DIE_HAS_ATTR(die, DW_AT_low_pc))
	    low_pc += picoffset;
#endif

	/* create new symbol with default assumptions */
	sym = raw_symbol(symname, LOC_STATIC);
	SYMBOL_TYPE (sym) = decode_die_type (die, 0);
	
	/* If this symbol is from a C++ compilation, then attempt to cache the
	   demangled form for future reference.  This is a typical time versus
	   space tradeoff, that was decided in favor of time because it sped up
	   C++ symbol lookups by a factor of about 20. */
	
	SYMBOL_LANGUAGE (sym) = cu_language;
	SYMBOL_INIT_DEMANGLED_NAME (sym, &current_objfile->symbol_obstack);

	switch ( tag )
	{
	case DW_TAG_label:
	    SYMBOL_VALUE (sym) = low_pc;
	    SYMBOL_CLASS (sym) = LOC_LABEL;
	    break;
	case DW_TAG_subprogram:
	    SYMBOL_VALUE (sym) = low_pc;
	    SYMBOL_TYPE (sym) = lookup_function_type (SYMBOL_TYPE (sym));
	    SYMBOL_CLASS (sym) = LOC_BLOCK;
	    if ( DIE_ATTR_UNSIGNED(die, DW_AT_external) )
	    {
		add_symbol_to_list (sym, &global_symbols);
	    }
	    else
	    {
		add_symbol_to_list (sym, list_in_scope);
	    }
	    break;
	case DW_TAG_inlined_subroutine:
	    SYMBOL_VALUE (sym) = low_pc;
	    SYMBOL_CLASS (sym) = LOC_BLOCK;
	    add_symbol_to_list (sym, list_in_scope);
	    break;
	case DW_TAG_variable:
	    if ( has_location )
	    {
		/* Store a pointer to a Dwarf2 location list.  This will be
		   evaluated as needed (but not now) by sym->class_indirect */
		sym->location_indirect = (PTR) DIE_ATTR_LOCLIST(die, DW_AT_location);
		sym->class_indirect = dwarf2_eval_location;
		SYMBOL_CLASS (sym) = LOC_INDIRECT;
	    }
	    else if ( is_constant )
	    {
		/* The compiler has optimized this variable out of existence
		   and replaced it with a constant value. */
		dwarf_attr_const_value(die, sym);
	    }
	    if ( DIE_ATTR_UNSIGNED(die, DW_AT_external) )
	    {
		add_symbol_to_list (sym, &global_symbols);
	    }
	    else
	    {
		add_symbol_to_list (sym, list_in_scope);
	    }
	    break;
	case DW_TAG_formal_parameter:
	    if ( has_location )
	    {
		sym->location_indirect = (PTR) DIE_ATTR_LOCLIST(die, DW_AT_location);
		sym->class_indirect = dwarf2_eval_location;
		SYMBOL_CLASS (sym) = LOC_INDIRECT_ARG;
	    }
	    else if ( is_constant )
	    {
		/* The compiler has optimized this parameter out of existence
		   and replaced it with a constant value. */
		dwarf_attr_const_value(die, sym);
	    }
	    add_symbol_to_list (sym, list_in_scope);
	    break;
	case DW_TAG_unspecified_parameters:
	    /* From varargs functions; gdb doesn't seem to have any interest
	       in this information, so just ignore it for now. */
	    break;
	case DW_TAG_class_type:
	case DW_TAG_structure_type:
	case DW_TAG_union_type:
	case DW_TAG_enumeration_type:
	    SYMBOL_CLASS (sym) = LOC_TYPEDEF;
	    SYMBOL_NAMESPACE (sym) = STRUCT_NAMESPACE;
	    add_symbol_to_list (sym, list_in_scope);
	    break;
	case DW_TAG_typedef:
	    SYMBOL_CLASS (sym) = LOC_TYPEDEF;
	    SYMBOL_NAMESPACE (sym) = VAR_NAMESPACE;
	    if ( TYPE_NAME (SYMBOL_TYPE (sym)) == NULL
		&& TYPE_CODE (SYMBOL_TYPE (sym)) != TYPE_CODE_PTR
		&& TYPE_CODE (SYMBOL_TYPE (sym)) != TYPE_CODE_FUNC )
	    {
		/* If the type pointed to by this typedef does not already have
		   a name, set the pointed-to name to the typedef name, with 
		   two exceptions.  (this code copied from stabsread.c; 
		   also appears in coffread.c) */
		/* If we are giving a name to a type such as "pointer to
		   foo" or "function returning foo", we better not set
		   the TYPE_NAME.  If the program contains "typedef char
		   *caddr_t;", we don't want all variables of type char
		   * to print as caddr_t.  This is not just a
		   consequence of GDB's type management; CC and GCC (at
		   least through version 2.4) both output variables of
		   either type char * or caddr_t with the type
		   refering to the C_TPDEF symbol for caddr_t.  If a future
		   compiler cleans this up it GDB is not ready for it
		   yet, but if it becomes ready we somehow need to
		   disable this check (without breaking the PCC/GCC2.4
		   case).

		   Sigh.
		   
		   Fortunately, this check seems not to be necessary
		   for anything except pointers or functions.  */
		TYPE_NAME (SYMBOL_TYPE (sym)) = SYMBOL_NAME (sym);
	    }
	    add_symbol_to_list (sym, list_in_scope);
	    break;
	default:
	    /* Not a tag we recognize.  Hopefully we aren't processing trash
	       data, but since we must specifically ignore things we don't
	       recognize, there is nothing else we should do at this point. */
	    break;
	}
    }
    return (sym);
}


/*

LOCAL FUNCTION

	add_symbol_inline -- add concrete inline info to a symbol's list

DESCRIPTION

 */

static 
struct inlin *
new_inline(concrete, objfile)
    struct symbol *concrete;
    struct objfile *objfile;
{
    struct inlin *inlin = (struct inlin *) 
	obstack_alloc(&objfile->symbol_obstack, sizeof(struct inlin));
    inlin->concrete_instance = concrete;
    inlin->next = NULL;
    return inlin;
}

/*

LOCAL FUNCTION

	synthesize_typedef -- make a symbol table entry for a "fake" typedef

DESCRIPTION

	Given a pointer to a DWARF information entry, synthesize a typedef
	for the name in the DIE, using the specified type.

	This is used for C++ class, structs, unions, and enumerations to
	set up the tag name as a type.

 */

static void
synthesize_typedef (die, type)
    Dwarf_Die die;
    struct type *type;
{
    struct symbol *sym = NULL;
    char *name;

    name = DIE_ATTR_STRING(die, DW_AT_name);

    if ( name )
    {
	sym = (struct symbol *)
	    obstack_alloc (&current_objfile->symbol_obstack, 
			   sizeof (struct symbol));
	memset (sym, 0, sizeof (struct symbol));
	SYMBOL_NAME (sym) = create_name (name, 
					 &current_objfile->symbol_obstack);
	SYMBOL_INIT_LANGUAGE_SPECIFIC (sym, cu_language);
	SYMBOL_TYPE (sym) = type;
	SYMBOL_CLASS (sym) = LOC_TYPEDEF;
	SYMBOL_NAMESPACE (sym) = VAR_NAMESPACE;
	add_symbol_to_list (sym, list_in_scope);
    }
}

/*

LOCAL FUNCTION

	decode_die_type -- decode the DW_AT_type attribute of a DIE

DESCRIPTION

	Translate a Dwarf 2 type attribute into gdb's struct type format.

	If a gdb type structure exists for the type, return it.  Else
	allocate an empty one; it will be filled in when the DIE referred to
	is processed by process_die.

	If the DIE doesn't have a type attribute, return builtin_type_int
	so that further processing can continue.  Generate a complaint only
	if NOTICE_EMPTY_TYPE is nonzero.

NOTES
	General note: Dwarf 2 representation of types is utterly different
	from the Dwarf 1 representation, where all type information was 
	stored in the DIE as a block of modifiers and a fundamental type.

 */

static struct type *
decode_die_type (die, notice_empty_type)
    Dwarf_Die die;
    int notice_empty_type;  /* Complain if no type attribute */
{
    struct type *type;
    Dwarf_Unsigned type_offset;
    Dwarf_Die type_die;
    
    if ( ! DIE_HAS_ATTR(die, DW_AT_type) )
    {
	if ( notice_empty_type )
	    complain(&missing_type, DIE_ID(die), DIE_NAME(die));
	return builtin_type_int;
    }

    type_offset = DIE_ATTR_UNSIGNED(die, DW_AT_type);
    type_die = dwarf_cu_offdie(current_dwfile, current_dwcu, type_offset);
    if ( (type = lookup_utype(type_die)) == NULL )
    {
	/* Assume that this is a forward reference to a DIE we haven't
	   seen yet.  Process the DIE recursively, then try again. */
	process_die(type_die, NULL);
	if ( (type = lookup_utype(type_die)) == NULL )
	{
	    complain(&dangling_typeref, DIE_ID(die), DIE_NAME(die));
	    type = builtin_type_int;
	}
    }
    return type;
}

/*

LOCAL FUNCTION

	create_name -- allocate a fresh copy of a string on an obstack

DESCRIPTION

	Given a pointer to a string and a pointer to an obstack, allocates
	a fresh copy of the string on the specified obstack.

*/

static char *
create_name (name, obstackp)
     char *name;
     struct obstack *obstackp;
{
    char *newname;
    
    newname = (char *) obstack_alloc (obstackp, strlen(name) + 1);
    strcpy (newname, name);
    return (newname);
}

/* 

LOCAL FUNCTIONS

    Helper functions for locval:

    init_location_stack
    locpush
    locpop
    pick
    rot
    binop_signed
    binop_unsigned
    unop
    relop

DESCRIPTION

    These helper functions create and maintain a stack for calculating
    Dwarf2 location expressions.

    The stack is created the first time dwarf_build_psymtabs is called.
    It is implemented as a dynamically allocated array -- so that it can 
    be resized as needed throughout the lifetime of gdb.

    Some stack info:
	-- The stack starts at 0 and grows towards larger addresses as items
	are pushed onto it.  
	-- The top of the stack is the element indexed by stacktop and always
	holds a valid element during use.  (i.e. anytime after the first push.)
	-- When the stack is empty, stacktop == -1.

    Push and pop do bounds checking on the stack.  If you try to push when
    there's not enough stack, more will be allocated silently.  If you try 
    to pop when there's too little stack, then the location expression is
    invalid, so we complain.

NOTES

    These functions could (and should) work at runtime as well as at 
    symbol read-in time as is currently done.  See locval() for more than 
    you ever wanted to know about location expressions.

    The stack is implemented as an array of unsigned long.  This makes 
    the binary arithmetic operations easier.  The Dwarf2 spec (2.4.3.4)
    says that that binary arithmetic should be unsigned (so that overflow 
    wraps like an odometer.)  But using unsigned long isn't quite right:
    according to 2.4.3, the stack elements are the size of an address on 
    the target machine.  As a bone to the spec, we'll test for this in 
    init_location_stack, but realistically, a machine with address > 32 bits 
    would probably break all of gdb, not just this stuff.
    
    The function pick makes an assumption about the stack's indexing
    (the top is assumed to be index 0.)  That comes from the Dwarf2 spec,
    2.4.3.3.  Since that is backwards from the actual representation, some
    index-twiddling has to be done.
*/

static void
init_location_stack()
{
    if ( sizeof(stackelem) != sizeof(CORE_ADDR) )
	/* See Dwarf2 spec, 2.4.3 */
	complain(&invalid_location_stack_elem_size, sizeof(stackelem), sizeof(CORE_ADDR));

    if ( stack == NULL )
    {
	/* Pick a number; 64 is a nice one; if this isn't big enough
	   it will be resized in locpush() */
	stacksize = 64;
	stack = (stackelem *) xmalloc(stacksize * sizeof(stackelem));
	stacktop = -1;
    }
}

static void
locpush(elem)
    stackelem elem;
{
    if ( ++stacktop == stacksize ) 
    {
	/* Overflow; don't whine about it, just resize the entire thing */
	stacksize *= 2;
	stack = (stackelem *) xrealloc(stack, stacksize * sizeof(stackelem));
    }
    stack[stacktop] = elem;
}

static stackelem
locpop()
{
    if ( stacktop < 0 )
    {
	/* Underflow; complain about this, since there's no way to get 
	   a valid result.  Return 0 just to return something. */
	complain(&empty_location_stack, DIE_ID(curr_die), DIE_NAME(curr_die));
	return 0;
    }
    return stack[stacktop--];
}

/* Pick: copy an element from anywhere in the stack onto the top.
   Note the Dwarf2 spec (2.4.3.3) specifies that index 0 means the
   top of the stack.  Since we are backwards from that we have to do
   some fiddling.
*/
static void
pick(index)
    long index;
{
    int myindex = stacktop - index;
    if ( myindex < 0 || myindex >= stacksize )
    {
	/* Garbage request; complain about this, since there's no way to do
	   the right thing.  Return without doing anything. */
	complain(&invalid_location_pick, DIE_ID(curr_die), DIE_NAME(curr_die), index);
    }
    else
	locpush(stack[myindex]);
}

/* Rotate n stack elements 1 position.  n will be 2 or 3 (only).  
   Example, rot(3):
	before: top = 12, second = -4, third = 45.
	after:  top = -4, second = 45, third = 12.
*/
static void
rot(n)
    int n;
{
    if ( n < 2 || n > 3 || (stacktop + 1) < n )
    {
	/* Garbage request; complain about this, since there's no way to do
	   the right thing.  Return without doing anything. */
	complain(&invalid_location_rotate, DIE_ID(curr_die), DIE_NAME(curr_die));
    }
    else
    {
	stackelem mini[3];
	int i;
	for ( i = 0; i < n; ++i )
	    mini[i] = locpop();
	for ( i = n; i > 0; --i )
	    locpush(mini[i % n]);
    }
}

/* The binary operators do basically the same thing: pop the top 2 entries, 
   perform a binary operation, then push the result.  When the operation 
   is not commutative, the operation op is done S op T, where T is the top
   element, and S is the second element.  (Dwarf2 spec 2.4.3.4)  Note that
   this is one place where the signedness of the stack matters.  The spec
   specifically calls out that binary arithmetic should be UNSIGNED, letting
   overflow bits wrap.  
*/
static void
binop (op)
    int op;
{
    stackelem top = locpop();
    stackelem second = locpop();

    switch ( op )
    {
    case DW_OP_and:
	locpush(second & top);
	break;
    case DW_OP_or:
	locpush(second | top);
	break;
    case DW_OP_xor:
	locpush(second ^ top);
	break;
    case DW_OP_plus:
	locpush(second + top);
	break;
    case DW_OP_minus:
	locpush(second - top);
	break;
    case DW_OP_mul:
	locpush(second * top);
	break;
    case DW_OP_div:
	locpush(second / top);
	break;
    case DW_OP_mod:
	locpush(second % top);
	break;
    case DW_OP_shl:
	locpush(second << top);
	break;
    case DW_OP_shr:
	/* Logical right shift */
	locpush(second >> top);
	break;
    case DW_OP_shra:
    {
	/* Arithmetic right shift; remember that C doesn't guarantee that 
	   shifted numbers are sign-extended. */
	stackelem highbit = 1;
	highbit <<= sizeof(stackelem) * 8 - 1;
	if ( second & highbit )
	{
	    /* There's a 1 bit in the MSB position.  Need to sign-extend 
	       by hand to be sure of it */
	    int i;
	    for ( i = 0; i < top; ++i )
	    {
		second >>= 1;
		second |= highbit;
	    }
	}
	else
	    second >>= top;
	locpush(second);
    }
	break;
    default:
	complain(&invalid_binary_operation, DIE_ID(curr_die), DIE_NAME(curr_die), op);
	break;
    }
}
    
/* The unary operators do basically the same thing: pop the top entry,
   do something to it, then push the result.  
*/
static void
unop (op)
    int op;
{
    stackelem top = locpop();

    switch ( op )
    {
    case DW_OP_neg:
	locpush( - ((long) top) );
	break;
    case DW_OP_not:
	locpush( ~ top );
	break;
    default:
	complain(&invalid_unary_operation, DIE_ID(curr_die), DIE_NAME(curr_die), op);
	break;
    }
}

/* The relational operators do basically the same thing: pop the top 2 
   entries, perform a comparison, then push a 1 if the comparison was TRUE
   or 0 if FALSE.  The operation op is done T op S, where T is the top
   element, and S is the second element.  (Dwarf2 spec 2.4.3.4)  Note that
   this is one place where the signedness of the stack matters.  The spec
   specifically calls out that relational comparisons should be SIGNED
   (2.4.3.5)
*/
static void
relop (op)
    int op;
{
    long top = (long) locpop();
    long second = (long) locpop();

    switch ( op )
    {
    case DW_OP_eq:
	locpush( top == second ? 1 : 0 );
	break;
    case DW_OP_ge:
	locpush( top >= second ? 1 : 0 );
	break;
    case DW_OP_gt:
	locpush( top > second ? 1 : 0 );
	break;
    case DW_OP_le:
	locpush( top <= second ? 1 : 0 );
	break;
    case DW_OP_lt:
	locpush( top < second ? 1 : 0 );
	break;
    case DW_OP_ne:
	locpush( top != second ? 1 : 0 );
	break;
    default:
	complain(&invalid_relational_operation, DIE_ID(curr_die), DIE_NAME(curr_die), op);
	break;
    }
}

/* A function that looks like a target vector function but FOR NOW
   is not a target vector function (FIXME).

   This is Dwarf-specific stuff, so I'm not sure what the right thing 
   to do is.  For now, just stick it here and fix in future.

   decode_dwarf_regx returns the gdb register number that corresponds to
   the dwarf location operation "DW_OP_regx regcode" where regcode is 
   the location operand.  This is specified for the 80960 in the 80960 
   ABI, Dwarf section 2.4.2.

   Note the same encodings work fine for the dwarf location operation 
   "DW_OP_bregx offset regcode" where the desired effect is
   (contents-of-regcode + offset).

   Not sure what to do on error.  I guess I'll return -1 since that's sure
   to get detected down the line.  (whereas 0 might not.)
*/

static int
target_decode_dwarf_regx(regcode)
    int regcode;
{
    switch (regcode)
    {
    case DW_OP_regx_i960_fp0:
    case DW_OP_regx_i960_fp1:
    case DW_OP_regx_i960_fp2:
    case DW_OP_regx_i960_fp3:
	return FP0_REGNUM + (regcode - DW_OP_regx_i960_fp0);
    case DW_OP_regx_i960_sf0:
    case DW_OP_regx_i960_sf1:
    case DW_OP_regx_i960_sf2:
    case DW_OP_regx_i960_sf3:
    case DW_OP_regx_i960_sf4:
	return SF0_REGNUM + (regcode - DW_OP_regx_i960_sf0);
    default:
	return -1;
    }
}

target_decode_dwarf_basereg(regcode)
    int regcode;
{
    return -1;
}

/*

LOCAL FUNCTION

	dwarf2_eval_sym_location -- compute the location of a symbol

DESCRIPTION

	Given pointer to a symbol, evaluate the symbol's location.
	If we are here, then the symbol's address class is either 
	LOC_INDIRECT or LOC_INDIRECT_ARG.

	This function will rarely be called directly.  Ordinarily,
	this will be called through struct symbol's class_indirect member.
*/

static enum address_class
dwarf2_eval_location (sym, pc)
    struct symbol *sym;
    CORE_ADDR pc;
{
    enum address_class rtnval = 
	dwarf2_eval_location_list(sym, NULL, pc);
    if (rtnval != LOC_OPTIMIZED_OUT)
	SYMBOL_VALUE(sym) = locpop();
    return rtnval;
}


/*

LOCAL FUNCTION

	dwarf2_eval_location_list -- compute the value of a location list

DESCRIPTION

	Given pointer to a location list, and the current PC, evaluate the 
	location.  Return the resulting address class.  Default to LOC_CONST
	and change to LOC_STATIC (core address) or LOC_REGISTER as needed.
	
	If PC is 0, then any location expression is assumed to span it.
	(the caller doesn't care about or know about location lists.)
	In this case, just evaluate the first (probably only) location 
	expression found on the list.

	If PC is nonzero, pick the first location expression that spans 
	the given PC.  If no expression spans it, return LOC_OPTIMIZED_OUT.
	(closest gdb gets to "not available at this time.")

	Note that this function may be called either during symbol readin
	time, when the inferior may not exist yet, or after the inferior
	exists, when the full flexibiliy of location lists can be used.

	Gdb's design does not mesh all that well with the DWARF notion 
	of a location computing interpreter.  But we can still make use 
	of the flexibility by introducing some judiciously chosen conventions
	to translate dwarf location expressions into gdb address classes.
	We return the address class that matches the symbol's location 
	at the moment its location was evaluated.  The symbol's actual 
	address class field will stay LOC_INDIRECT or LOC_INDIRECT_ARG.

	Return values:
	LOC_STATIC	    value on top of the stack is a memory address
	LOC_REGISTER	    value on top of the stack is a register number
	LOC_REGISTER_ARG    same as LOC_REGISTER except that some callers 
			    want to know that the symbol is a function arg
	LOC_BASEREG	    value on top of stack is an offset from address
			    contained in base register.  Base register number
			    is in symbol->basereg.
	LOC_BASEREG_ARG     same as LOC_BASEREG except that some callers 
			    want to know that the symbol is a function arg
	LOC_CONST	    value on top of stack is just a number
	LOC_OPTIMIZED_OUT   location does not exist at the given PC

	When computing values involving the current value of the frame pointer,
	the value zero is used, which results in a value relative to the frame
	pointer, rather than the absolute value.  This is what GDB wants
	anyway.

 */

static enum address_class
dwarf2_eval_location_list (sym, loclist, pc)
    struct symbol *sym;
    Dwarf_Locdesc loclist;
    CORE_ADDR pc;
{
    enum address_class addr_class = LOC_OPTIMIZED_OUT;
    int is_arg = (sym != NULL && sym->class == LOC_INDIRECT_ARG);
    Dwarf_Locdesc locdesc;
    Dwarf_Loc loc;
    CORE_ADDR lopc, hipc;
    int i;
    int found = 0;

    for (locdesc = loclist ? loclist : (Dwarf_Locdesc) sym->location_indirect;
	 locdesc && found == 0;
	 locdesc = locdesc->next)
    {
	/* Find an expression that spans the given PC */
	/* NOTE: the range (0 - infinity) is a special range that means
	   the location expression is always valid.  This special range
	   should not be adjusted by the PIC offset.  Test for the range
	   with infinity only, since 0 is a legal lopc in a relocatable file. */
	if ((CORE_ADDR) locdesc->ld_hipc == 0xffffffff)
	{
	    lopc = (CORE_ADDR) locdesc->ld_lopc;
	    hipc = (CORE_ADDR) locdesc->ld_hipc;
	}
	else
	{
	    lopc = (CORE_ADDR) locdesc->ld_lopc + picoffset;
	    hipc = (CORE_ADDR) locdesc->ld_hipc + picoffset;
	}
		    
	if ( pc == 0 || (pc >= lopc && pc < hipc) )
	    found = 1;
	else 
	    continue;
	    
	for ( loc = locdesc->ld_loc, i = 0; loc; loc = loc->next, ++i )
	{
	    switch ( loc->lr_operation )
	    {
	    case DW_OP_addr:
		locpush(loc->lr_operand1);
		addr_class = LOC_STATIC;
		break;
	    case DW_OP_const1u:
	    case DW_OP_const1s:
	    case DW_OP_const2u:
	    case DW_OP_const2s:
	    case DW_OP_const4u:
	    case DW_OP_const4s:
	    case DW_OP_constu :
	    case DW_OP_consts :
		locpush(loc->lr_operand1);
		break;
	    case DW_OP_dup:
		pick(0);
		break;
	    case DW_OP_drop:
		locpop();
		break;
	    case DW_OP_over:
		pick(1);
		break;
	    case DW_OP_pick:
		pick(loc->lr_operand1);
		break;
	    case DW_OP_swap:
		rot(2);
		break;
	    case DW_OP_rot:
		rot(3);
		break;
	    case DW_OP_and:
	    case DW_OP_or:
	    case DW_OP_plus:
	    case DW_OP_minus:
	    case DW_OP_shl:
	    case DW_OP_shr:
	    case DW_OP_shra:
	    case DW_OP_xor:
	    case DW_OP_mod:
	    case DW_OP_mul:
	    case DW_OP_div:
		binop(loc->lr_operation);
		break;
	    case DW_OP_abs:
		/* We are already unsigned, so absolute value is meaningless */
		break;
	    case DW_OP_neg:
	    case DW_OP_not:
		unop(loc->lr_operation);
		break;
	    case DW_OP_plus_uconst:
		locpush(loc->lr_operand1 + locpop());
		break;
	    case DW_OP_eq:
	    case DW_OP_ge:
	    case DW_OP_gt:
	    case DW_OP_le:
	    case DW_OP_lt:
	    case DW_OP_ne:
		relop(loc->lr_operation);
		break;
	    case DW_OP_skip_lr:
		/* Libdwarf extension.  Acts like DW_OP_skip, only it skips N
		   location records, not N bytes.  Note N can be negative. Skip
		   N records, starting from the record following this one. */
	    {
		int count = 0;
		i += (Dwarf_Signed) loc->lr_operand1;
		for ( loc = locdesc->ld_loc; count < i; ++count )
		    /* This works for both positive and negative N */
		    loc = loc->next;
		break;
	    }
	    case DW_OP_bra_lr:
		/* Libdwarf extension.  Acts like DW_OP_bra, only it skips N
		   location records, not N bytes.  Note N can be negative. Skip
		   N records, starting from the record following this one. */
		if ( locpop() )
		{
		    int count = 0;
		    i += (Dwarf_Signed) loc->lr_operand1;
		    for ( loc = locdesc->ld_loc; count < i; ++count )
			/* This works for both positive and negative N */
			loc = loc->next;
		}
		break;
	    case DW_OP_reg0:
	    case DW_OP_reg1:
	    case DW_OP_reg2:
	    case DW_OP_reg3:
	    case DW_OP_reg4:
	    case DW_OP_reg5:
	    case DW_OP_reg6:
	    case DW_OP_reg7:
	    case DW_OP_reg8:
	    case DW_OP_reg9:
	    case DW_OP_reg10:
	    case DW_OP_reg11:
	    case DW_OP_reg12:
	    case DW_OP_reg13:
	    case DW_OP_reg14:
	    case DW_OP_reg15:
	    case DW_OP_reg16:
	    case DW_OP_reg17:
	    case DW_OP_reg18:
	    case DW_OP_reg19:
	    case DW_OP_reg20:
	    case DW_OP_reg21:
	    case DW_OP_reg22:
	    case DW_OP_reg23:
	    case DW_OP_reg24:
	    case DW_OP_reg25:
	    case DW_OP_reg26:
	    case DW_OP_reg27:
	    case DW_OP_reg28:
	    case DW_OP_reg29:
	    case DW_OP_reg30:
	    case DW_OP_reg31:
		locpush(loc->lr_operation - DW_OP_reg0);
		addr_class = is_arg ? LOC_REGPARM : LOC_REGISTER;
		break;
	    case DW_OP_regx:
		locpush(target_decode_dwarf_regx(loc->lr_operand1));
		addr_class = is_arg ? LOC_REGPARM : LOC_REGISTER;
		break;
	    case DW_OP_breg0:
	    case DW_OP_breg1:
	    case DW_OP_breg2:
	    case DW_OP_breg3:
	    case DW_OP_breg4:
	    case DW_OP_breg5:
	    case DW_OP_breg6:
	    case DW_OP_breg7:
	    case DW_OP_breg8:
	    case DW_OP_breg9:
	    case DW_OP_breg10:
	    case DW_OP_breg11:
	    case DW_OP_breg12:
	    case DW_OP_breg13:
	    case DW_OP_breg14:
	    case DW_OP_breg15:
	    case DW_OP_breg16:
	    case DW_OP_breg17:
	    case DW_OP_breg18:
	    case DW_OP_breg19:
	    case DW_OP_breg20:
	    case DW_OP_breg21:
	    case DW_OP_breg22:
	    case DW_OP_breg23:
	    case DW_OP_breg24:
	    case DW_OP_breg25:
	    case DW_OP_breg26:
	    case DW_OP_breg27:
	    case DW_OP_breg28:
	    case DW_OP_breg29:
	    case DW_OP_breg30:
	    case DW_OP_breg31:
		locpush(loc->lr_operand1);
		addr_class = is_arg ? LOC_BASEREG_ARG : LOC_BASEREG;
		if ( sym )
		    SYMBOL_BASEREG(sym) = loc->lr_operation - DW_OP_breg0;
		break;
	    case DW_OP_bregx:
		if (loc->lr_operand1 == DW_OP_regx_i960_pic_bias)
		{
		    /* Intercept special-case PIC "base register" before 
		       passing it back to normal GDB processing */
		    locpush(picoffset + loc->lr_operand2);
		}
		else
		{
		    /* Not PIC base; do what GDB expects for register-based 
		       addressing */
		    locpush(loc->lr_operand2);
		    addr_class = is_arg ? LOC_BASEREG_ARG : LOC_BASEREG;
		    if ( sym )
			SYMBOL_BASEREG(sym) = 
			    target_decode_dwarf_regx(loc->lr_operand1);
		}
		break;
	    case DW_OP_fbreg:
		/* FIXME: this is not correct.  Each subprogram DIE is supposed
		   to be able to specify a DW_AT_frame_base attribute, which is
		   a location description.  We simplify this by ignoring the
		   frame_base attribute on symbol read-in and assuming that 
		   the frame base is a register, specified by FP_REGNUM. */
		locpush(loc->lr_operand1);
		addr_class = is_arg ? LOC_BASEREG_ARG : LOC_BASEREG;
		if ( sym )
		    SYMBOL_BASEREG(sym) = FP_REGNUM;
		break;
	    case DW_OP_lit0:
	    case DW_OP_lit1:
	    case DW_OP_lit2:
	    case DW_OP_lit3:
	    case DW_OP_lit4:
	    case DW_OP_lit5:
	    case DW_OP_lit6:
	    case DW_OP_lit7:
	    case DW_OP_lit8:
	    case DW_OP_lit9:
	    case DW_OP_lit10:
	    case DW_OP_lit11:
	    case DW_OP_lit12:
	    case DW_OP_lit13:
	    case DW_OP_lit14:
	    case DW_OP_lit15:
	    case DW_OP_lit16:
	    case DW_OP_lit17:
	    case DW_OP_lit18:
	    case DW_OP_lit19:
	    case DW_OP_lit20:
	    case DW_OP_lit21:
	    case DW_OP_lit22:
	    case DW_OP_lit23:
	    case DW_OP_lit24:
	    case DW_OP_lit25:
	    case DW_OP_lit26:
	    case DW_OP_lit27:
	    case DW_OP_lit28:
	    case DW_OP_lit29:
	    case DW_OP_lit30:
	    case DW_OP_lit31:
		locpush(loc->lr_operation - DW_OP_lit0);
		break;
	    case DW_OP_nop:
		break;
	    case DW_OP_deref_size:
	    case DW_OP_deref:
	    {
		/* The Dwarf spec (2.4.3.3) says we're supposed to zerofill 
		   up to the size of an address on the target machine.  
		   We're currently cheating, hard-coding 4 (FIXME-SOMEDAY). */
		char buf[4];
		stackelem s;
		int deref_size = (loc->lr_operation == DW_OP_deref_size) ? 
		    loc->lr_operand1 : 4;
		CORE_ADDR deref_addr = locpop();
		target_read_memory(deref_addr, buf, deref_size);
		SWAP_TARGET_AND_HOST(buf, deref_size);
		for ( i = 4; i > deref_size; --i )
		    buf[i - 1] = 0;
		locpush((stackelem) buf);
		addr_class = LOC_STATIC;
		break;
	    }
	    case DW_OP_xderef:
	    case DW_OP_xderef_size:
	    case DW_OP_piece:
		/* Error cases.  These have yet to be implemented.  When there 
		   is demand for them (from compiler writers) some brave 
		   volunteer can spend the time to do them. */
		/* Intentional fallthrough */
	    case DW_OP_const8u:
	    case DW_OP_const8s:
		/* Error cases.  These can't be done because we can't 
		   deal with 8-byte integers. */
		/* Intentional fallthrough */
	    case DW_OP_skip:
	    case DW_OP_bra:
		/* Error cases.  We don't know how to skip N bytes in a 
		   location expression once we have it from libdwarf.
		   But we do know how to skip N location records (see 
		   DW_OP_skip_lr, DW_OP_bra_lr) */
		/* Intentional fallthrough */
	    default:
		/* All others are bogus operation numbers. */
		if ( sym )
		{
		    error("Unimplemented location operation: %d in symbol %s",
			  loc->lr_operation,
			  SYMBOL_NAME(sym) ? SYMBOL_NAME(sym) : "");
		}
		else
		{
		    error("Unimplemented location operation: %d",
			  loc->lr_operation);
		}
		break;
	    }
	}
    }
    return addr_class;
}

/* Here is a debugging function to print a human-readable location list.
   This is useful enough that it was made into maintenance command. */

#include "frame.h"	/* For get_selected_block */

void
address_info_1 PARAMS ((struct symbol *, CORE_ADDR));

dwarf_print_location_list(sym)
    struct symbol *sym;
{
    /* If we are here, the symbol's address class is LOC_INDIRECT 
       or LOC_INDIRECT_ARG. */
    Dwarf_Locdesc locdesc = (Dwarf_Locdesc) sym->location_indirect;
    Dwarf_Loc loc;
    CORE_ADDR lopc, hipc;

    for ( ; locdesc; locdesc = locdesc->next)
    {
	if ((CORE_ADDR) locdesc->ld_hipc == 0xffffffff)
	{
	    lopc = (CORE_ADDR) locdesc->ld_lopc;
	    hipc = (CORE_ADDR) locdesc->ld_hipc;
	}
	else
	{
	    lopc = (CORE_ADDR) locdesc->ld_lopc + picoffset;
	    hipc = (CORE_ADDR) locdesc->ld_hipc + picoffset;
	}
	printf_filtered("     0x%x - 0x%x: ", lopc, hipc);
	address_info_1(sym, lopc);
    }
}


void
maintenance_address_info(exp, from_tty)
    char *exp;
    int from_tty;
{
    register struct symbol *sym;
    register struct minimal_symbol *msymbol;
    struct block *block;
    int is_a_field_of_this;	/* C++: lookup_symbol sets this to nonzero
				   if exp is a field of `this'. */
    if (exp == 0)
	error ("Argument required.");
    
    sym = lookup_symbol (exp, block = get_selected_block (), VAR_NAMESPACE, 
			 &is_a_field_of_this, (struct symtab **)NULL);
    if (sym == NULL)
    {
	if (is_a_field_of_this)
	{
	    printf_filtered ("Symbol \"");
	    fprintf_symbol_filtered (gdb_stdout, exp,
				     current_language->la_language, DMGL_ANSI);
	    printf_filtered ("\" is a field of the local class variable `this'\n");
	    return;
	}
	
	msymbol = lookup_minimal_symbol (exp, (struct objfile *) NULL);
	
	if (msymbol != NULL)
	{
	    printf_filtered ("Symbol \"");
	    fprintf_symbol_filtered (gdb_stdout, exp,
				     current_language->la_language, DMGL_ANSI);
	    printf_filtered ("\" is at ");
	    print_address_numeric (SYMBOL_VALUE_ADDRESS (msymbol), 1,
				   gdb_stdout);
	    printf_filtered (" in a file compiled without debugging.\n");
	}
	else
	    error ("No symbol \"%s\" in current context.", exp);
	return;
    }
    
    printf_filtered ("Symbol \"");
    fprintf_symbol_filtered (gdb_stdout, SYMBOL_NAME (sym),
			     current_language->la_language, DMGL_ANSI);
    printf_filtered ("\" is ");
    if (sym->class == LOC_INDIRECT || sym->class == LOC_INDIRECT_ARG)
    {
	printf_filtered("a location list:\n");
	dwarf_print_location_list(sym);
    }
    else
	address_info_1(sym, 0);
}

void
dwarf2_print_stacktop()
{
    char buf[16];
    if ( stacktop == -1 )
	strcpy(buf, "empty");
    else
	sprintf(buf, "0x%x", stack[stacktop]);

    printf_filtered("stack[%d] = %s\n", stacktop, buf);
}

/* 
   The libdwarf error handler.
   
   If the same error occurs more than ERROR_THRESHOLD times,
   return FATAL.  This guarantees a program termination even if the 
   Dwarf file is hopelessly corrupted.

   Libdwarf warnings are complaints now, to avoid printing 
   their messages on the screen unless you ask for them.
*/

#define ERROR_THRESHOLD 40

static
Dwarf_Signed
dwarf2_errhand (error_desc)
    Dwarf_Error error_desc;
{
    static Dwarf_Unsigned lasterror = 0xffffffff;
    static int errcount;
    static int errnum = 0;  /* work around gdb's odd "..." behavior */
    char error_buf[1024];

    if ( error_desc )
    {
	sprintf(error_buf,
		"%s%s %s, line %d: %s\n",
		errnum++ == 0 ? "\n" : "",
		"Error in libdwarf file",
		error_desc->file,
		error_desc->line,
		error_desc->msg);

	switch ( error_desc->severity )
	{
	case DLS_WARNING:
	    /* Convert into a complaint. */
	    complain(&libdwarf_warning, error_desc->file, error_desc->line,
		     error_desc->msg);
	    break;
	case DLS_ERROR:
	    /* Return the severity.  The code that was executing when 
	       the error occurred will continue.  GDB calling functions
	       may need to check their return values and the _lbdw_errno
	       variable to see if an error has been flagged. */
	    fprintf_filtered(gdb_stderr, error_buf);
	    if (error_desc->num != lasterror)
	    {
		errcount = 1;
		lasterror = error_desc->num;
	    }
	    else if (++errcount > ERROR_THRESHOLD)
	    {
		return(DLS_FATAL);
	    }
	    return(error_desc->severity);
	case DLS_FATAL:
	    /* Libdwarf will abort the program if I return this severity.
	       Kinda ugly.  So error() back to the top level and let
	       the user figure out what to do. */
	    error(error_buf);
	    break;
	default:
	    /* Ay yi yi.  God only knows what's going on here. */
	    fprintf_filtered(gdb_stderr, 
			     "Internal error: Unexpected libdwarf error severity: %d\n", 
			     error_desc->severity);
	    gdb_flush(gdb_stderr);
	    return DLS_FATAL;
    	}
    }
    else 
    {
	fprintf_filtered(gdb_stderr, 
			 "Internal error: NULL error descriptor in dwarf2_errhand.\n");
	return DLS_FATAL;
    }
}

/* Maps from a register in dwarf2.h to a REGNUM defined in in tm-i960.h. */
static int map_register(reg)
    int reg;
{
    switch (reg) {
    case DW_CFA_i960_pfp: return PFP_REGNUM;
    case DW_CFA_i960_sp:  return SP_REGNUM;
    case DW_CFA_i960_rip: return RIP_REGNUM;
    case DW_CFA_i960_r3:  return RIP_REGNUM+1;
    case DW_CFA_i960_r4:  return RIP_REGNUM+2;
    case DW_CFA_i960_r5:  return RIP_REGNUM+3;
    case DW_CFA_i960_r6:  return RIP_REGNUM+4;
    case DW_CFA_i960_r7:  return RIP_REGNUM+5;
    case DW_CFA_i960_r8:  return RIP_REGNUM+6;
    case DW_CFA_i960_r9:  return RIP_REGNUM+7;
    case DW_CFA_i960_r10: return RIP_REGNUM+8;
    case DW_CFA_i960_r11: return RIP_REGNUM+9;
    case DW_CFA_i960_r12: return RIP_REGNUM+10;
    case DW_CFA_i960_r13: return RIP_REGNUM+11;
    case DW_CFA_i960_r14: return RIP_REGNUM+12;
    case DW_CFA_i960_r15: return RIP_REGNUM+13;
    case DW_CFA_i960_g0:  return G0_REGNUM;
    case DW_CFA_i960_g1:  return G0_REGNUM+1;
    case DW_CFA_i960_g2:  return G0_REGNUM+2;
    case DW_CFA_i960_g3:  return G0_REGNUM+3;
    case DW_CFA_i960_g4:  return G0_REGNUM+4;
    case DW_CFA_i960_g5:  return G0_REGNUM+5;
    case DW_CFA_i960_g6:  return G0_REGNUM+6;
    case DW_CFA_i960_g7:  return G0_REGNUM+7;
    case DW_CFA_i960_g8:  return G0_REGNUM+8;
    case DW_CFA_i960_g9:  return G0_REGNUM+9;
    case DW_CFA_i960_g10: return G0_REGNUM+10;
    case DW_CFA_i960_g11: return G0_REGNUM+11;
    case DW_CFA_i960_g12: return G12_REGNUM;
    case DW_CFA_i960_g13: return G13_REGNUM;
    case DW_CFA_i960_g14: return G14_REGNUM;
    case DW_CFA_i960_fp:  return FP_REGNUM;
    case DW_CFA_i960_fp0: return FP0_REGNUM+0;
    case DW_CFA_i960_fp1: return FP0_REGNUM+1;
    case DW_CFA_i960_fp2: return FP0_REGNUM+2;
    case DW_CFA_i960_fp3: return FP0_REGNUM+3;
    default: error("Found unexpected register in .debug_frame info: %d.",reg);
    }
}

static unsigned long get_register(reg,frame,value)
    unsigned long reg;
    struct frame_info *frame;
    unsigned long *value;
{
    unsigned long addr;

    if (frame && frame->fsr && get_fsr_address(frame->fsr,reg))
	    addr = get_fsr_address(frame->fsr,reg);
    else
	    addr = 0;
    if (value) {
	if (addr)
		*value = read_memory_integer(addr,4);
	else if (frame && frame->fsr && has_fsr_reg_alias(frame->fsr,reg))
		*value = read_register(get_fsr_reg_alias(frame->fsr,reg));
	else
		*value = read_register(reg);
    }
    return addr;
}

static struct fsr_list {
    struct frame_saved_regs *fsr;
    struct fsr_list *next;
} *top_of_list;

extern struct obstack frame_cache_obstack;

static struct frame_saved_regs *push_fsrs(fsr)
    struct frame_saved_regs *fsr;
{
    struct fsr_list *plist = (struct fsr_list *) obstack_alloc(&frame_cache_obstack,sizeof(struct fsr_list));
    struct frame_saved_regs *pfsr = (struct frame_saved_regs *) obstack_alloc(&frame_cache_obstack,
									      sizeof(struct frame_saved_regs));

    memcpy(pfsr,fsr,sizeof(struct frame_saved_regs));
    plist->fsr = fsr;
    plist->next = top_of_list;
    top_of_list = plist;
    return pfsr;
}

static struct frame_saved_regs *pop_fsrs()
{
    if (top_of_list) {
	struct frame_saved_regs *fsr = top_of_list->fsr;

	top_of_list = top_of_list->next;
	return fsr;
    }
    else
	    return (struct frame_saved_regs *) 0;
}


static void process_insns(n_insns,insns,fi,next_frame,last_pc,
			  cfa_reg_value,cfa_reg_offset,pfp_reg_value,cie_fsrs)
    int n_insns;
    struct Dwarf_Frame_Insn *insns;
    struct frame_info *fi,*next_frame;
    CORE_ADDR last_pc;
    unsigned long *cfa_reg_value;
    unsigned long *cfa_reg_offset;
    unsigned long pfp_reg_value;
    struct frame_saved_regs *cie_fsrs;
{
    int i;

    for (i=0;i < n_insns && insns[i].loc <= last_pc;i++) {
	int reg,alias_reg;
	unsigned long offset;
	unsigned long dummy,addr;

	switch(insns[i].insn) {
    case Dwarf_frame_offset:
	    reg = map_register(insns[i].op2);
	    set_fsr_address(fi->fsr,reg,insns[i].op1 + (*cfa_reg_value) + (*cfa_reg_offset));
	    break;
    case Dwarf_frame_undefined:
	    reg = map_register(insns[i].op1);
	    set_fsr_undefined(fi->fsr,reg);
	    break;
    case Dwarf_frame_same:
	    reg = map_register(insns[i].op1);
	    if (next_frame)
		    fi->fsr->fsr_vals[reg] = next_frame->fsr->fsr_vals[reg];
	    set_fsr_same_value(fi->fsr,reg);
	    break;
    case Dwarf_frame_register:
	    reg = map_register(insns[i].op1);
	    if (1) {
		int alias_reg = map_register(insns[i].op2);
		unsigned long addr;
		if (addr=get_register(alias_reg,next_frame,0))
			set_fsr_address(fi->fsr,reg,addr);
		else
			set_fsr_alias_reg(fi->fsr,reg,alias_reg);
	    }
	    break;
    case Dwarf_frame_restore:
	    reg = map_register(insns[i].op1);
	    set_fsr_address(fi->fsr,reg,get_fsr_address(cie_fsrs,reg));
	    break;
    case Dwarf_frame_remember_state:
	    fi->fsr = push_fsrs(fi->fsr);
	    break;
    case Dwarf_frame_restore_state:
	    fi->fsr = pop_fsrs();
	    break;
    case Dwarf_frame_def_cfa:
	    dummy = get_register(map_register(insns[i].op1),next_frame,cfa_reg_value);
	    (*cfa_reg_offset) = insns[i].op2;
	    break;
    case Dwarf_frame_def_cfa_register:
	    dummy = get_register(map_register(insns[i].op1),next_frame,cfa_reg_value);
	    break;
    case Dwarf_frame_def_cfa_offset:
	    (*cfa_reg_offset) = insns[i].op1;
	    break;
    case Dwarf_frame_pfp_offset:
	    reg = map_register(insns[i].op1);
	    set_fsr_address(fi->fsr,reg,pfp_reg_value + insns[i].op2);
	    break;
	}
    }
}

struct frame_saved_regs *get_dwarf_frame_register_information(fi,next_frame,pst,is_a_leafproc,ra)
    struct frame_info *fi,*next_frame;
    struct partial_symtab *pst;
    int *is_a_leafproc;
    CORE_ADDR *ra;
{
    *is_a_leafproc = 0;
    if (ra)
	    *ra = 0;
    if (pst && pst->objfile && pst->objfile->sym_private) {
	Dwarf_Frame_FDE fde = 
	    dwarf_frame_fetch_fde((Dwarf_Debug) pst->objfile->sym_private,
				  fi->pc - picoffset);
	int i,cfa_reg_value = -1,cfa_reg_offset = 0;

	if (fde == NULL)
	    return NULL;

	fi->fsr = (struct frame_saved_regs *)
		obstack_alloc (&frame_cache_obstack,
			       sizeof (struct frame_saved_regs));
	memset (fi->fsr, '\0', sizeof (struct frame_saved_regs));

	if (fi->inlin) {
	    if (next_frame)
		    *fi->fsr = *next_frame->fsr;
	}
	else {
	    unsigned long pfp_reg_value;
	    unsigned long start_of_func,end_of_func;
	    struct frame_saved_regs cie_regs;
	    struct symbol *func_sym = find_pc_function(fi->pc);
	    char *name;

	    fde->start_ip += picoffset;

	    memset(&cie_regs,'\0',sizeof(cie_regs));
	    if (func_sym) {
		struct block *func_block = SYMBOL_BLOCK_VALUE(func_sym);
		    
		start_of_func = BLOCK_START(func_block);
		end_of_func = BLOCK_END(func_block);
	    }
	    else
		    find_pc_partial_function(fi->pc,&name,&start_of_func,&end_of_func);


	    get_register(PFP_REGNUM,next_frame,&pfp_reg_value);

	    /* First, process CIE insns: */

	    process_insns(fde->cie->n_instructions,fde->cie->insns,fi,next_frame,0xffffffff,
			  &cfa_reg_value,
			  &cfa_reg_offset,
			  pfp_reg_value,
			  &cie_regs);

	    if (is_fsr_same_value(fi->fsr,FP_REGNUM)) { /* If this function's rule for FP is the same
							   as the previous one then this must be a
							   leafproc. */
		unsigned long g14_reg_value;

		*is_a_leafproc = 1;
 		process_insns(fde->n_instructions,fde->insns,fi,next_frame,fi->pc,
 			      &cfa_reg_value,
 			      &cfa_reg_offset,
 			      pfp_reg_value,
 			      &cie_regs);
		get_register(G14_REGNUM,fi,&g14_reg_value);
		if (g14_reg_value >= start_of_func && g14_reg_value < end_of_func) { /* If we got here
											via a call insn
											then. */
		    while (fde->previous_FDE && fde->previous_FDE->start_ip <= start_of_func)
			    fde = fde->previous_FDE;
		    /* Process the Insns for the leaf proc: */
		    process_insns(fde->cie->n_instructions,fde->cie->insns,fi,next_frame,0xffffffff,
				  &cfa_reg_value,
				  &cfa_reg_offset,
				  pfp_reg_value,
				  &cie_regs);
		    process_insns(fde->n_instructions,fde->insns,fi,next_frame,fi->pc,
 				  &cfa_reg_value,
 				  &cfa_reg_offset,
 				  pfp_reg_value,
 				  &cie_regs);
		    if (ra) 
			    get_register(RIP_REGNUM,fi,ra);
		}
		else {/* The fi->fsr now contains the state of all of the registers. */
		    fi->fsr->fsr_vals[RIP_REGNUM] = 
			    fi->fsr->fsr_vals[map_register(fde->cie->return_address_register)];
		    if (ra) 
			    get_register(G14_REGNUM,fi,ra);
		}
	    }
	    else {
		/* Now, process the insns in the fde: */
		process_insns(fde->n_instructions,fde->insns,fi,next_frame,fi->pc,
			      &cfa_reg_value,
			      &cfa_reg_offset,
			      pfp_reg_value,
			      &cie_regs);
		if (ra) 
			get_register(RIP_REGNUM,fi,ra);
	    }
	    /* The fi->fsr now contains the state of all of the registers. */
	}
	return fi->fsr;
    }
    return (struct frame_saved_regs *) 0;
}

extern struct cmd_list_element *maintenanceinfolist;

void
_initialize_dwarf2read()
{
    add_cmd ("address", class_maintenance, maintenance_address_info,
	     "Like info address but print DWARF location lists",
	     &maintenanceinfolist);
}

