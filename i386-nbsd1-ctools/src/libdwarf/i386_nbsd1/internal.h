/******************************************************************************
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
 *****************************************************************************/

#ifndef _INTERNAL_H
#define _INTERNAL_H

#ifdef DOS
#define READ_BINARY "rb"
#define READ_TEXT "r"
#else
#define READ_BINARY "r"
#define READ_TEXT "r"
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
 
#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#define DWARF_VERSION		2
#define DW_CU_HDR_LENGTH	11  /* Size of Compilation Unit Header. */

/* 
 * A heuristic for reading the CU DIE's.  This is what will
 * be read in as the CU DIE at startup, we are assuming that 
 * a DIE will not be longer than this. 
 */
#define DW_MAX_DIE_LENGTH	1024

/* 
 * Number of abbreviations (per CU) we start with 
 * when building the abbrev_table 
 */
#define DW_NUM_OF_ABBREVS	50  

/*
 * A constant that distiguishes a CIE from an FDE (.debug_frame)
 */
#define DW_CIE_ID  0xffffffff

/*
 * MACRO FOR ERROR HANDLING
 *
 * This macro first fills the error descriptor in fill_err(), then
 * calls whatever error handler is turned on at the given time to
 * deal with the error.  Notice that it will abort if the error
 * handler returns an error of the DLS_FATAL variety, otherwise 
 * it will return the severity of the error.
 */
#define fill_err(a)     _dw_fill_err(a, __FILE__, __LINE__)

extern Dwarf_Signed _lbdw_errsev;   /* defined in error.c */
#define LIBDWARF_ERR(dw_error) \
	((_lbdw_errsev = (*_lbdw_errhand)(fill_err(dw_error))) == DLS_FATAL ? \
	(abort(), DLS_FATAL) : _lbdw_errsev)

/*
 * MACRO FOR ERROR HANDLING
 *
 * This macro first resets the global error variable to DLE_NE.
 */
#define RESET_ERR() (_lbdw_errno = DLE_NE)

/*
 * TYPEDEFS for various purposes.  Most of these are declared in
 * their own header files; e.g. struct Dwarf_Line is declared in lines.h.
 */
typedef struct dwarf_abbrev		Dwarf_Abbrev;
typedef struct dwarf_abbrev_table	Dwarf_Abbrev_Table;
typedef struct dwarf_aranges_prolog	Dwarf_Aranges_Prolog;
typedef struct dwarf_aranges_table	Dwarf_Aranges_Table;
typedef struct dwarf_attrib_form	Dwarf_Attrib_Form;
typedef struct dwarf_cu_list		Dwarf_CU_List;
typedef struct dwarf_die_tree		Dwarf_Die_Tree;
typedef struct dwarf_frame_info		Dwarf_Frame_Info;
typedef struct dwarf_info_cu_hdr	Dwarf_Info_CU_Hdr;
typedef struct dwarf_line_prolog	Dwarf_Line_Prolog;
typedef struct dwarf_line_table		Dwarf_Line_Table;
typedef struct dwarf_memory_table	Dwarf_Memory_Table;
typedef struct dwarf_pubnames_prolog	Dwarf_Pubnames_Prolog;
typedef struct dwarf_pubnames_table	Dwarf_Pubnames_Table;

/* 
 * DIE structures, used by almost everybody
 */
struct Dwarf_Die 
{
    Dwarf_Unsigned   code;	/* reference into .abbrev table */

    Dwarf_Unsigned   length;

    Dwarf_Half       tag;

    /* offset from the beginning of .debug_info */
    Dwarf_Unsigned   section_offset;	

    /* offset from the beginning of the CU */
    Dwarf_Unsigned   cu_offset;		

    /* 1 if DIE has at least one child */
    Dwarf_Small      has_child;

    /* 1 if DIE has sibling attribute */
    Dwarf_Small      has_sib_attr;

    Dwarf_Die	    parent_ptr;

    Dwarf_Die	    child_ptr;

    Dwarf_Die	    sibling_ptr;

    /* which CU it is in */
    Dwarf_CU_List   *cu_node; 

    /* list of attributes for this DIE */
    Dwarf_Attribute attr_list;	
};

struct dwarf_info_cu_hdr
{
    Dwarf_Half     version;

    Dwarf_Small    target_addr_size;

    Dwarf_Unsigned length;

    /* offset into .debug_abbrev */
    Dwarf_Unsigned abbrev_offset;
};
 
struct dwarf_die_tree
{
    /* offset in the file where .debug_info section starts */
    Dwarf_Unsigned    file_offset;

    /* offset in the .debug_info section where 1st DIE starts */
    Dwarf_Unsigned    section_offset; 

    /* endianness of .debug_info section */
    Dwarf_Bool	      big_endian;     

    /* Beginning of DIE-chain for this CU */
    Dwarf_Die	      die_list;	

    /* the CU header for this CU */
    Dwarf_Info_CU_Hdr *cu_header;
};

/*
 * ABBREV STRUCTURES
 */
struct dwarf_attrib_form 
{
    /* attribute encoding */
    Dwarf_Half	name;

    /* what form does the attribute's value take? */
    Dwarf_Small	form;

    struct dwarf_attrib_form *next;
};

struct dwarf_abbrev
{
    /* ties this abbrev back to particular DIE */
    Dwarf_Unsigned    code;       

    /* attributes for this DIE */
    Dwarf_Attrib_Form *attr_list; 

    /* DWARF tag for this type of DIE */
    Dwarf_Half        tag;        

    /* 1 == this type of DIE has children */
    Dwarf_Small       child;      
};

struct dwarf_abbrev_table
{
    /* endianness of the section */
    Dwarf_Bool	   big_endian;

    /* number of entries in the abbrev_array */
    Dwarf_Unsigned entry_count;    

    /* the abbrevs */
    Dwarf_Abbrev   *abbrev_array;
};
 
/* 
 * GLOBAL STRUCTURES
 */
struct dwarf_cu_list
{
    Dwarf_Die_Tree	    *die_head;

    Dwarf_Line_Table	    *line_table;

    Dwarf_Abbrev_Table	    *abbrev_table;  

    Dwarf_Aranges_Table	    *aranges_table; 

    Dwarf_Pubnames_Table    *pubnames_table;

    /* ptr to whatever dbg it comes from */
    Dwarf_Debug		    dbg_ptr;

    struct dwarf_cu_list    *next;
};

struct dwarf_memory_table
{
    /* Number of entries in the table */
    Dwarf_Unsigned	count;

    /* Maximum number of entries allowed (can be resized) */
    Dwarf_Unsigned	limit;

    /* A list of addresses (the table entries) */
    char		**table;
};

struct Dwarf_Debug 
{
    /* Filename if known; otherwise NULL */
    char *filename;
    
    /* The all-purpose file pointer */
    FILE *stream;

    /* Number of .debug_* sections */
    Dwarf_Unsigned dbg_num_dw_scns;

    /* Array of section info from client */
    Dwarf_Section dbg_sections;

    /* Chain of CU headers */
    Dwarf_CU_List *dbg_cu_list;	

    /* Dwarf frame information */
    Dwarf_Frame_Info *frame_info;
	
    /* Memory allocation list; for fast deallocation */
    Dwarf_Memory_Table *memory_table;
};

/* Function prototypes for internal libdwarf functions */

Dwarf_Die
_dw_build_a_die PARAMS ((Dwarf_Debug, Dwarf_CU_List *, Dwarf_Die));

char *
_dw_build_a_string PARAMS ((Dwarf_Debug));

int
_dw_build_cu_list PARAMS ((Dwarf_Debug));

Dwarf_Line
_dw_build_ln_list PARAMS ((Dwarf_Debug, Dwarf_Line_Prolog *, 
		       Dwarf_Unsigned *, int));

void
_dw_build_pubnames PARAMS ((Dwarf_Debug));

void 
_dw_build_aranges PARAMS((Dwarf_Debug));

Dwarf_Die
_dw_build_tree PARAMS ((Dwarf_Debug, Dwarf_CU_List *, Dwarf_Die));

int
_dw_check_version PARAMS ((Dwarf_Unsigned));

char *
_dw_clear PARAMS ((char *, int));

Dwarf_Signed	 
_dw_error_handler PARAMS ((Dwarf_Error));

Dwarf_Die
_dw_find_cu_offset PARAMS ((Dwarf_Debug, Dwarf_Die, Dwarf_Off));

Dwarf_Section
_dw_find_section PARAMS ((Dwarf_Debug, char *));

Dwarf_Error
_dw_fill_err PARAMS ((Dwarf_Unsigned, char *, int));

Dwarf_Signed
_dw_fread PARAMS ((Dwarf_Small *, int, int, FILE *));

void 
_dw_free PARAMS ((Dwarf_Debug, char *));

int
_dw_fseek PARAMS ((FILE *, int, int));

long
_dw_ftell PARAMS ((FILE *));

Dwarf_Unsigned
_dw_leb128_to_ulong PARAMS ((Dwarf_Debug));

Dwarf_Signed
_dw_leb128_to_long PARAMS ((Dwarf_Debug));

Dwarf_Pn_Tuple
_dw_lookup_pn_tuple PARAMS ((Dwarf_Debug, char *,
			     Dwarf_Pn_Tuple, Dwarf_Unsigned));

Dwarf_Ar_Tuple
_dw_lookup_ar_tuple PARAMS ((Dwarf_Debug, Dwarf_Addr, 
                             Dwarf_Ar_Tuple, Dwarf_Unsigned));

char *
_dw_malloc PARAMS ((Dwarf_Debug, Dwarf_Unsigned));

Dwarf_Unsigned
_dw_read_constant PARAMS ((Dwarf_Debug, Dwarf_Signed, Dwarf_Bool));

Dwarf_Bignum
_dw_read_large_constant PARAMS ((Dwarf_Debug, Dwarf_Signed, Dwarf_Bool));

Dwarf_Line_Prolog *
_dw_read_line_prolog PARAMS ((Dwarf_Debug, int));

char *
_dw_realloc PARAMS ((Dwarf_Debug, char *, int));

void
_dw_swap_bytes PARAMS ((unsigned char *, Dwarf_Unsigned, Dwarf_Bool));

void
_dw_update_err PARAMS ((Dwarf_Error));
#endif /* _INTERNAL_H */

