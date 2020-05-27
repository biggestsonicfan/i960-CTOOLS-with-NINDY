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

#ifndef LINES_H
#define LINES_H

/*
 * LINE NUMBER STRUCTURES
 */
/* Struct filename is used to store information about a file used during
   compilation.  An array of these will be maintained in the prolog for
   this compilation unit. */
struct filename
{
    /* File name as received from the compiler; This can come from either
       the prolog or from a "define_file" opcode. */
    char *name;
 
    /* Julian date of last modification of the file (0 if unused) */
    Dwarf_Unsigned date;
 
    /* Number of bytes in the file (0 if unused) */
    Dwarf_Unsigned len;
 
    /* Index into the prolog's dirlist array. (directories where files
       were found during compilation.)  0 means "use current directory". */
    Dwarf_Unsigned dirnum;
};
 
/* Line number prolog.  There will be one prolog per compilation unit,
   immediately followed by the raw line numbers for that compilation unit.
   dwarf_init stores a pointer to this structure as part of the
   Dwarf_Line_Table structure. */
struct dwarf_line_prolog
{
    /* The total contribution (in bytes) of line numbers to this CU,
       including the prolog */
    Dwarf_Unsigned cu_len;
 
    /* Total bytes in the prolog */
    Dwarf_Unsigned prolog_len;
 
    /* Number of operands for standard opcodes */
    unsigned char *std_opcode_numops;
 
    /* Array of file names (and size and date info) used to build this CU.
       This is represented as an array, not a linked list, so that a number
       can easily be used to index into the list. */
    struct filename **filelist;
 
    /* Total number of (struct filename *) currently available in the
       filelist array.  Will be increased as needed if the number of files
       in use exceeds this number. */
    Dwarf_Unsigned filelist_size;
 
    /* Current file number.  This is the index of the next file to be
       added to the filelist array. */
    Dwarf_Unsigned next_filenum;
 
    /* Array of directory names that were searched by the compiler to find
       files at compile time.  Index 0 is never used; it stands for the
       current directory. */
    char **dirlist;
 
    /* Total number of (char *) currently available in the dirlist array.
       Will be increased as needed if the number of directories in use
       exceeds this number. */
    Dwarf_Unsigned dirlist_size;
 
    /* Next directory number.  This is the index of the next directory name
       to be added to the dirlist array. */
    Dwarf_Unsigned next_dirnum;
 
    /* Minimum change to line register */
    Dwarf_Signed line_base;
 
    /* Line # version; same as dwarf version? */
    unsigned short version;
 
    /* Minimum instruction length */
    unsigned char min_ins_len;
 
    /* Starting value for is_stmt register */
    unsigned char dflt_is_stmt;
 
    /* Range of change to line register */
    unsigned char line_range;
 
    /* Number to assign to first special opcode */
    unsigned char opcode_base;
};

struct dwarf_line_table
{ 
    /* endianness of the section */
    Dwarf_Bool	       big_endian; 
     
    /* Line number array for this CU */
    Dwarf_Line	       ln_list;

    /* Number of ln's in this CU */
    Dwarf_Unsigned     ln_count;  	

    /* Line number prolog for this CU */
    Dwarf_Line_Prolog  *ln_prolog;	
};

#endif	/* LINES_H */
