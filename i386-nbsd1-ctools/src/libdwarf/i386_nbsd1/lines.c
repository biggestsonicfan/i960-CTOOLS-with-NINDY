/*****************************************************************************
 * 
 * Copyright (c) 1995 Intel Corporation
 * 
 * Intel hereby grants you permission to copy, modify, and distribute this
 * software and its documentation.  Intel grants this permission provided
 * that the above copyright notice appears in all copies and that both the
 * copyright notice and this permission notice appear in supporting
 * documentation.  In addition, Intel grants this permission provided that
 * you prominently mark as "not part of the original" any modifications
 * made to this software or documentation, and that the name of Intel
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software or the documentation without specific,
 * written prior permission.
 * 
 * Intel Corporation provides this AS IS, WITHOUT ANY WARRANTY, EXPRESS OR
 * IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY
 * OR FITNESS FOR A PARTICULAR PURPOSE.  Intel makes no guarantee or
 * representations regarding the use of, or the results of the use of,
 * the software and documentation in terms of correctness, accuracy,
 * reliability, currentness, or otherwise; and you rely on the software,
 * documentation and results solely at your own risk.
 *
 * IN NO EVENT SHALL INTEL BE LIABLE FOR ANY LOSS OF USE, LOSS OF BUSINESS,
 * LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES
 * OF ANY KIND.  IN NO EVENT SHALL INTEL'S TOTAL LIABILITY EXCEED THE SUM
 * PAID TO INTEL FOR THE PRODUCT LICENSED HEREUNDER.
 * 
 ****************************************************************************/

/* Support routines for .debug_line delivery functions.
   Functions here are not intended to be called directly by the debugger.  */

#include "libdwarf.h"
#include "internal.h"
#include "lines.h"

/* Functions used in this file only */
static void
add_dir PARAMS ((Dwarf_Debug, Dwarf_Line_Prolog *, char *));

static void
add_file PARAMS ((Dwarf_Debug, Dwarf_Line_Prolog *, struct filename *));

static void
add_ln PARAMS ((Dwarf_Debug, Dwarf_Line, Dwarf_Line *, 
		Dwarf_Unsigned *, Dwarf_Unsigned));

static void
reset_ln PARAMS ((Dwarf_Line, Dwarf_Line_Prolog *));

/*  add_dir -- helper function for _dw_read_line_prolog
    Add one directory name to the prolog's directory list.
    Reallocate the list if needed (if adding a new name will exceed 
    the current size of the list.)

    This just manages the directory LIST, i.e. storage for the new 
    directory name is assumed to be already allocated and permanent.
*/
static void
add_dir(dbg, prolog, newdir)
    Dwarf_Debug dbg;
    Dwarf_Line_Prolog *prolog;
    char *newdir;
{
    if ( prolog->next_dirnum >= prolog->dirlist_size )
    {
	if ( prolog->dirlist )
	{
	    /* Realloc for twice the current size */
	    prolog->dirlist = (char **)
		_dw_realloc(dbg, (void*)prolog->dirlist, 
			     prolog->dirlist_size * sizeof(char *) * 2);
	    prolog->dirlist_size *= 2;
	}
	else
	{
	    /* First time this has been called.  Allocate a reasonable number
	       of directory names; we'll realloc later if needed. */
	    prolog->dirlist_size = 8;
	    prolog->dirlist = (char **)
		_dw_malloc(dbg, prolog->dirlist_size * sizeof(char *));
	}
    }
    prolog->dirlist[prolog->next_dirnum++] = newdir;
}

	    
/*  add_file -- helper function for _dw_read_line_prolog
    Add one file name to the prolog's file list.
    Reallocate the list if needed (if adding a new name will exceed 
    the current size of the list.)

    This just manages the file LIST, i.e. storage for the new 
    file name is assumed to be already allocated and permanent.
*/
static void
add_file(dbg, prolog, newfile)
    Dwarf_Debug dbg;
    Dwarf_Line_Prolog *prolog;
    struct filename *newfile;
{
    if ( prolog->next_filenum >= prolog->filelist_size )
    {
	if ( prolog->filelist )
	{
	    /* Realloc for twice the current size */
	    prolog->filelist = (struct filename **)
		_dw_realloc(dbg, (void*)prolog->filelist, 
			     prolog->filelist_size * sizeof(struct filename *) * 2);
	    prolog->filelist_size *= 2;
	}
	else
	{
	    /* First time this has been called.  Allocate a reasonable number
	       of file names; we'll realloc later if needed. */
	    prolog->filelist_size = 8;
	    prolog->filelist = (struct filename **)
		_dw_malloc(dbg, 
			   prolog->filelist_size * sizeof(struct filename *));
	}
    }
    prolog->filelist[prolog->next_filenum++] = newfile;
}

	
/*  add_ln -- helper function for _dw_build_ln_list 

    Add line number LN at index INDEX of line number list LN_LIST.
    Size of the list is maintained by the caller and passed in via
    LN_LIST_SIZE.

    Reallocate the list if needed (if adding a new line will exceed 
    the current size of the list.)

    A new line number struct will be allocated each time this is called.
    The fields of the new line number will be copied from the LN parameter.
*/
static void
add_ln(dbg, ln, ln_list, ln_list_size, index)
    Dwarf_Debug dbg;
    Dwarf_Line ln;
    Dwarf_Line *ln_list;
    Dwarf_Unsigned *ln_list_size;
    Dwarf_Unsigned index;
{
    if ( index >= *ln_list_size )
    {
	if ( *ln_list )
	{
	    /* Realloc for twice the current size */
	    *ln_list = (Dwarf_Line)
		_dw_realloc(dbg, (void*) *ln_list,
			     *ln_list_size * sizeof(struct Dwarf_Line) * 2);
	    *ln_list_size *= 2;
	}
	else
	{
	    /* First time this has been called.  Allocate a reasonable number
	       of line numbers; we'll realloc later if needed. */
	    *ln_list_size = 256;
	    *ln_list = (Dwarf_Line)
		_dw_malloc(dbg, *ln_list_size * sizeof(struct Dwarf_Line));
	}
    }
    (*ln_list)[index] = *ln;
}
   
 
/*  reset_ln -- helper function for _dw_build_ln_list 

    Set all the fields of a Dwarf_Line struct to their defaults.
    See Dwarf2 spec 6.2.2 for how the defaults are figured out.
*/
static void
reset_ln(ln, prolog)
    Dwarf_Line ln;
    Dwarf_Line_Prolog *prolog;
{
    ln->address = 0;
    ln->file = prolog->filelist ? prolog->filelist[1]->name : NULL;
    ln->dir = prolog->dirlist ? prolog->dirlist[prolog->filelist[1]->dirnum] : NULL;
    ln->line = 1;
    ln->column = 0;
    ln->is_stmt = prolog->dflt_is_stmt;
    ln->basic_block = 0;
    ln->end_sequence = 0;
}


/* 
    INTERNAL FUNCTION

	_dw_read_line_prolog -- Initialize a line number prolog structure
	from the raw Dwarf bits.

    SYNOPSIS

	Dwarf_Line_Prolog *_dw_read_line_prolog(Dwarf_Debug dbg,
	                                    int section_byte_order);
	
	DBG represents the open Dwarf file.  The underlying stream is
	assumed to be aimed at the start of the raw Dwarf bits for this 
	compilation unit.

	SECTION_BYTE_ORDER is 1 if this section has big-endian memory.

    DESCRIPTION

	This function allocates permanent storage for a line number prolog,
	and initializes the fields based on the information read from the
	Dwarf file.

    RETURNS

	A pointer to the line number prolog structure.
*/

Dwarf_Line_Prolog *
_dw_read_line_prolog(dbg, section_byte_ordering)
    Dwarf_Debug dbg;
    int section_byte_ordering;
{
    unsigned int ch; /* For reading a (possibly) signed 1-byte quantity */
    Dwarf_Line_Prolog *pp; /* The new prolog */
    
    RESET_ERR();
    pp = (Dwarf_Line_Prolog *) _dw_malloc(dbg, sizeof(Dwarf_Line_Prolog));
    _dw_clear((char *) pp, sizeof(Dwarf_Line_Prolog));

    pp->cu_len = _dw_read_constant(dbg, 4, section_byte_ordering) + 4;
    pp->version = _dw_read_constant(dbg, 2, section_byte_ordering);

    /* check version */
    if(!_dw_check_version(pp->version))
	return(NULL);	/* if not dwarf 2 */
    
    pp->prolog_len = _dw_read_constant(dbg, 4, section_byte_ordering) + 10;
    pp->min_ins_len = _dw_read_constant(dbg, 1, section_byte_ordering);
    pp->dflt_is_stmt = _dw_read_constant(dbg, 1, section_byte_ordering);

    /* Check for a signed 1-byte quantity, since we don't know for sure 
       that type "char" is signed on this host machine */
    ch = _dw_read_constant(dbg, 1, section_byte_ordering);
    if ( ch & 0x80 )
    {
	/* It's negative: convert to a 32-bit signed quantity.  We have to
	   do our own sign-extension since the >> operator is not guaranteed
	   to shift in 1 bits */
	pp->line_base = ch | 0xffffff00;
    }
    pp->line_range = _dw_read_constant(dbg, 1, section_byte_ordering);
    pp->opcode_base = _dw_read_constant(dbg, 1, section_byte_ordering);


    /* Standard opcodes are allowed to have compiler-specified number of
       operands.  Copy into std_opcode_numops starting at index 1, so that
       finding an opcode length amounts to reading std_opcode_numops[opcode].
       See Dwarf spec 6.2.4, standard_opcode_lengths */
    pp->std_opcode_numops = (unsigned char *) _dw_malloc(dbg, pp->opcode_base);
    pp->std_opcode_numops[0] = 0;
    _dw_fread(pp->std_opcode_numops + 1, pp->opcode_base - 1, 1, dbg->stream);

    /* Do include dirs and files if present in the prolog */
    pp->next_dirnum = pp->next_filenum = 1;

    while ( 1 )
    {
	/* Allocate permanent storage for each directory name.
	   Storage for the pointers will get allocated in add_dir. */
	char *dir = _dw_build_a_string(dbg);
	if ( *dir )
	    add_dir(dbg, pp, dir);
	else 
	{
	    _dw_free(dbg, dir);
	    break;
	}
    }

    while ( 1 )
    {
	/* Allocate permanent storage for each file name.
	   Storage for the pointers will get allocated in add_file. */
	struct filename *fn = 
	    (struct filename *) _dw_malloc(dbg, sizeof(struct filename));
	fn->name = _dw_build_a_string(dbg);
	if ( *fn->name )
	{
	    fn->dirnum = _dw_leb128_to_ulong(dbg);
	    fn->date = _dw_leb128_to_ulong(dbg);
	    fn->len = _dw_leb128_to_ulong(dbg);
	    add_file(dbg, pp, fn);
	}
	else
	{
	    _dw_free(dbg, fn->name);
	    _dw_free(dbg, (char *) fn);
	    break;
	}
    }
    return pp;
}


/* 
    INTERNAL FUNCTION

	_dw_build_ln_list -- Read one compilation unit's worth of the
		.debug_line section into a usable format.

    SYNOPSIS

	Dwarf_Line *_dw_build_ln_list(Dwarf_Debug dbg,
	                          Dwarf_Line_Prolog *prolog, 
	                          Dwarf_Unsigned *ln_count,
				  int section_byte_order);

	DBG represents the open Dwarf file, with underlying stream aimed
	correctly.
	
	PROLOG points to the line number prolog for this CU (previously 
	initialized in _dw_read_line_prolog). 
	
	LN_COUNT points to storage to hold the number of line numbers
	in the list, once that is known.

	SECTION_BYTE_ORDER is 1 if this section has big-endian memory.

    DESCRIPTION

	This function allocates permanent storage for an array of struct
	Dwarf_Line.

	Dwarf2 line numbers model the user program with a state machine, 
	which has a set of registers.  (See Dwarf spec 6.2.2)  At any instant,
	a snapshot of the machine can be taken by examining the registers.
	The registers contain, most importantly: the program counter, the 
	source file from which the PC resulted, and the line number within 
	the source file.  The compiler emits line number information whenever
	the source line or source file changes, and some other times as well.

	Our job is to track the machine registers as they change.  We will 
	generate a Dwarf_Line structure for each special opcode, for each
	"copy" standard opcode, and for each "end_sequence" extended opcode.

    FIXME
	
	It's all or nothing on this table.  We allocate the entire table
	for an entire compilation unit.  It should be possible to allocate 
	only as much as you need at the time, although as decribed above, 
	it will always be necessary to start at the beginning and work until
	you've read enough.

    RETURNS

	A pointer to the start of the newly-created line number table.
    
    ERROR 

	Returns NULL.
*/

Dwarf_Line
_dw_build_ln_list(dbg, prolog, ln_count, byte_order)
    Dwarf_Debug dbg;
    Dwarf_Line_Prolog *prolog;
    Dwarf_Unsigned *ln_count;
    int byte_order;
{
    Dwarf_Line ln_list = NULL;    /* The result */
    Dwarf_Unsigned ln_list_size = 0;	/* Will be set by add_ln */
    Dwarf_Unsigned index = 0;	/* Current index into the line number array */
    Dwarf_Signed tmpline;   /* For intermediate ln calculation */
    struct Dwarf_Line ln;   /* A temporary working line number */
    int bytecount = 0;	/* How many bytes have we read? */
    int total_bytes = prolog->cu_len - prolog->prolog_len;
    long start = _dw_ftell(dbg->stream);

    RESET_ERR();
    /* Create a single line number entry and initialize to the defaults.
       (see Dwarf spec 6.2.2)  Then we'll modify this one line number 
       entry as new line number info is read in from the file. */
    reset_ln(&ln, prolog);

    while ( bytecount < total_bytes )
    {
	Dwarf_Unsigned ul, length;
	Dwarf_Signed sl;
	Dwarf_Small opcode, extended_opcode;
	
	opcode = _dw_read_constant(dbg, 1, byte_order);

	if ( opcode == 0 )
	{
	    /* This is an extended opcode.  Next is an LEB128 giving the 
	       length of the instruction.  We ignore it, since we advance
	       the file pointer as we go for each type of opcode. */
	    length = _dw_leb128_to_ulong(dbg);
	    extended_opcode = _dw_read_constant(dbg, 1, byte_order);
	    switch ( extended_opcode )
	    {
	    case DW_LNE_end_sequence:
		ln.end_sequence = 1;
		add_ln(dbg, &ln, &ln_list, &ln_list_size, index++);
		reset_ln(&ln, prolog);
		break;
	    case DW_LNE_set_address:
		/* FIXME: the "4" here should be something like gdb's
		   sizeof(CORE_ADDR) */
		ln.address = _dw_read_constant(dbg, 4, byte_order);
		break;
	    case DW_LNE_define_file:
	    {
		/* Create a new file name structure and add to the permanent
		   list in the prolog.  Note that this does NOT affect the 
		   registers, nor do we do an add_ln.  Just enter the file
		   name for later use. */
		struct filename *fn = 
		    (struct filename *) _dw_malloc(dbg, 
						   sizeof(struct filename));
		fn->name = _dw_build_a_string(dbg);
		fn->dirnum = _dw_leb128_to_ulong(dbg);
		fn->date = _dw_leb128_to_ulong(dbg);
		fn->len = _dw_leb128_to_ulong(dbg);
		add_file(dbg, prolog, fn);
	    }
		break;
	    default:
		/* GACK: don't know what to do for this.  Use the ignored
		   "length" field to try to recover as best we can. */
	        if(LIBDWARF_ERR(DLE_BAD_OPCODE) == DLS_ERROR)
                    return(NULL);
		_dw_fseek(dbg->stream, length - 1, SEEK_CUR);
		break;
	    }
	}
	else if ( opcode < prolog->opcode_base )
	{
	    /* This is a "standard" opcode; there should be a constant
	       for it in dwarf2.h.  The opcode_base field of the prolog
	       allows for more standard opcodes to be added later. */
	    switch ( opcode )
	    {
	    case DW_LNS_copy:
		add_ln(dbg, &ln, &ln_list, &ln_list_size, index++);
		ln.basic_block = 0;
		break;
	    case DW_LNS_advance_pc:
		ul = _dw_leb128_to_ulong(dbg);
		ln.address += ul * prolog->min_ins_len;
		break;
	    case DW_LNS_advance_line:
		sl = _dw_leb128_to_long(dbg);
		tmpline = ln.line;
		tmpline += sl;
		/* Make sure line never drops below zero. */
		if ( tmpline < 0 )
		{
	            if(LIBDWARF_ERR(DLE_BAD_LINENO) == DLS_ERROR)
                        return(NULL);
		    tmpline = 0;
		}
		ln.line = tmpline;
		add_ln(dbg, &ln, &ln_list, &ln_list_size, index++);
		break;
	    case DW_LNS_set_file:
		ul = _dw_leb128_to_ulong(dbg);
		ln.file = prolog->filelist[ul]->name;
		ln.dir = 
		    prolog->dirlist ? 
			prolog->dirlist[prolog->filelist[ul]->dirnum] : 
			    NULL; 
		break;
	    case DW_LNS_set_column:
		ul = _dw_leb128_to_ulong(dbg);
		ln.column = ul; 
		break;
	    case DW_LNS_negate_stmt:
		ln.is_stmt = ! ln.is_stmt;
		break;
	    case DW_LNS_set_basic_block:
		ln.basic_block = 1;
		break;
	    case DW_LNS_const_add_pc:
		/* Weirdo.  Acts like special opcode 255, except that it does
		   not append a line to the line number list. */
		ln.address += (Dwarf_Unsigned) 
		    (255 - prolog->opcode_base) / prolog->line_range;
		break;
	    case DW_LNS_fixed_advance_pc:
		/* Weirdo.  Read the address increment directly. */
		ln.address += _dw_read_constant(dbg, 2, byte_order);
		break;
	    default:
	        if(LIBDWARF_ERR(DLE_BAD_OPCODE) == DLS_ERROR);
                    return(NULL);
	    }
	}
	else
	{
	    /* This is a "special" opcode.  It is an overloaded opcode 
	       taking no arguments, and having the effect of changing 
	       both the line register and the address register in a 
	       compact format. */
	    ln.address += (Dwarf_Unsigned) 
		(opcode - prolog->opcode_base) / prolog->line_range;
	    tmpline = ln.line;
	    tmpline += prolog->line_base + 
		((Dwarf_Unsigned) 
		 (opcode - prolog->opcode_base) % prolog->line_range);
	    /* Make sure line never drops below zero. */
	    if ( tmpline < 0 )
	    {
	        if(LIBDWARF_ERR(DLE_BAD_LINENO) == DLS_ERROR)
                    return(NULL);
		tmpline = 0;
	    }
	    ln.line = tmpline;
	    add_ln(dbg, &ln, &ln_list, &ln_list_size, index++);
	}
	bytecount = _dw_ftell(dbg->stream) - start;
    }
    *ln_count = index;
    return ln_list;
}

/*
 * DEBUGGING FUNCTIONS 
 */
#if defined(DEBUG)
dump_prolog(pp)
    Dwarf_Line_Prolog *pp;
{
    char **dp = pp->dirlist;
    struct filename **fp = pp->filelist;
    Dwarf_Unsigned i;
    unsigned char *cp;
    printf("\nLine number prolog:\n");
    printf("%4s%-40s%u\n", "", "Compilation Unit length:", pp->cu_len);	
    printf("%4s%-40s%u\n", "", "Prologue length:", pp->prolog_len);	
    printf("%4s%-40s\n", "", "'Standard' opcode number of operands:");
    for ( i = 1, cp = pp->std_opcode_numops + 1; i < pp->opcode_base; ++i, ++cp )
	printf("%8s%-10d%-10d\n", "", i, *cp);
    printf("%4s%-40s\n", "", "Include file list:");
    for ( i = 1; i < pp->next_dirnum; ++i, ++dp )
	printf("%8s%s\n", "", *dp);
    printf("%4s%-40s\n", "", "Source file list:");	
    for ( i = 1; i < pp->next_filenum; ++i, ++fp )
    {
	printf("%8s%3d %s\n", "", i, (*fp)->name);
	printf("%12s%-20s%u\n", "", "Include directory:", (*fp)->dirnum);
	printf("%12s%-20s%u\n", "", "Length:", (*fp)->len);
	printf("%12s%-20s%u\n", "", "Modification date:", (*fp)->date);
    }
    printf("%4s%-40s%u\n", "", "Line Number Info version:", pp->version);	
    printf("%4s%-40s%u\n", "", "Minimum instruction length:", pp->min_ins_len);  
    printf("%4s%-40s%u\n", "", "Default 'is_stmt' value:", pp->dflt_is_stmt);
    printf("%4s%-40s%d\n", "", "Line base:", pp->line_base);
    printf("%4s%-40s%u\n", "", "Line range:", pp->line_range);	
    printf("%4s%-40s%u\n", "", "First 'special' opcode number:", pp->opcode_base);	
}
#endif /* DEBUG */

