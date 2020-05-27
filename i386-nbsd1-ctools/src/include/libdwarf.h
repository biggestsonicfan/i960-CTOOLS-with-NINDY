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

#ifndef _LIBDWARF_H
#define _LIBDWARF_H
#include <stdio.h>
#include "dwarf2.h"
#include "dw2i960.h"

/*
 *  PARAMS macro for function prototypes.  This allows only one declaration
 *  per function.
 *
 *  NOTE: All known AIX compilers implement prototypes, but don't always
 *  define __STDC__
 */
#if ! defined(PARAMS)
#if defined(__STDC__) || defined(_AIX)
#define PARAMS(paramlist)       paramlist
#else
#define PARAMS(paramlist)       ()
#endif
#endif
 
/*
 *  BASIC TYPES
 */
typedef int            Dwarf_Bool;         /* boolean type */
typedef unsigned int   Dwarf_Off;          /* 4 byte file offset */
typedef unsigned int   Dwarf_Unsigned;     /* 4 byte unsigned value */
typedef unsigned short Dwarf_Half;         /* 2 byte unsigned value */
typedef unsigned char  Dwarf_Small;        /* 1 byte unsigned value */
typedef int	       Dwarf_Signed;       /* 4 byte signed value */
typedef void*	       Dwarf_Addr;         /* memory address */


/*
 *  AGGREGATE TYPES
 */
struct Dwarf_Block 
{  
    /* uninterpreted block of data */
    Dwarf_Unsigned bl_len;
    Dwarf_Addr     bl_data;
};

struct Dwarf_Loc 
{
    /* a location record */
    Dwarf_Small		lr_operation; /* one of DW_OP* in dwarf2.h */
    Dwarf_Unsigned	lr_operand1; 
    Dwarf_Unsigned	lr_operand2;
    struct Dwarf_Loc	*next;
};

struct Dwarf_Locdesc 
{  
    /* a location expression */
    Dwarf_Addr		    ld_lopc; /* beginning of active range */
    Dwarf_Addr		    ld_hipc; /* end of active range */
    struct Dwarf_Loc	    *ld_loc; /* pointer to list of location records */
    struct Dwarf_Locdesc    *next;
};

struct Dwarf_Ellist 
{  
    /* element list */
    Dwarf_Signed el_value;   /* value of element */
    char*	 el_name;    /* name of element */
};

struct Dwarf_Bounds 
{ 
    /* subscript bounds information */
    Dwarf_Bool bo_isconst;
    union {
        Dwarf_Unsigned       constant;
        struct Dwarf_Locdesc locdesc;
    } bo_;
};

struct Dwarf_Section 
{  
    /* for passing section info to dwarf_init() */
    Dwarf_Unsigned size;        /* section size */
    Dwarf_Unsigned file_offset; /* offset of section within the file */
    char*          name;        /* name of section */
    Dwarf_Bool     big_endian;  /* section big-endian? */
};

struct Dwarf_Bignum
{
    /* a data quantity bigger than 4 bytes */
    Dwarf_Unsigned *words;   /* two or more words */
    Dwarf_Unsigned numwords; /* number of words used */
};

typedef struct Dwarf_Block	*Dwarf_Block;
typedef struct Dwarf_Loc	*Dwarf_Loc;
typedef struct Dwarf_Locdesc	*Dwarf_Locdesc;
typedef struct Dwarf_Ellist	*Dwarf_Ellist;
typedef struct Dwarf_Bounds	*Dwarf_Bounds;
typedef struct Dwarf_Section	*Dwarf_Section;
typedef struct Dwarf_Bignum	*Dwarf_Bignum;

/*
 * ERROR HANDLING
 */
struct Dwarf_Error {
	Dwarf_Unsigned num;     /* the message number from libdwarf.h */
        char    *msg;           /* the main error message */
        Dwarf_Signed severity;  /* the severity of the error
                                 * -1 DLS_FATAL, 0 = DLS_WARNING, 1 = DLS_ERROR */
        char    *file;          /* __FILE__ */
        int 	line;           /* __LINE__ */
};


/* 
 * All the attributes boil down to one of the following types,
 * we're trying to deliver the attribute in the form it is
 * really represented, this means we could throw away the form
 * if we didn't need it to distinguish between the referencing
 * modes (see comment below).
 */ 
#define DLT_UNSIGNED    0
#define DLT_SIGNED      1
#define DLT_LOCLIST     2
#define DLT_BLOCK       3
#define DLT_STRING      4
#define DLT_BIGNUM	5
 
struct Dwarf_Attribute {
    Dwarf_Half  at_name;    /* one of DW_AT* from dwarf2.h */
/* 
 * we need to keep the form around to distinguish between 
 * the two types of referencing modes, those being CU-relative 
 * and section-relative.  See section 7.5.4 of the TIS spec. 
 */
    Dwarf_Small at_form;   	 
    Dwarf_Small val_type;   /* one of DLT_* (above)*/
    union {
        /* DLT_UNSIGNED: the atrtribute value was in form:
            (Dwarf 1) addr, ref, data[24]
            (Dwarf 2) addr, ref[124], ref_addr, ref_udata,
            data[124], udata, flag, strp */
        Dwarf_Unsigned  val_unsigned;
 
        /* DLT_SIGNED: the attribute value was in form:
            (Dwarf 2) sdata */
        Dwarf_Signed val_signed;
 
        /* DLT_LOCLIST: the atrtribute value was in form:
            (Dwarf 1) block
            (Dwarf 2) block, or offset into .debug_loc section.
            In either case, location attributes will be represented
            in the most general way, a location list  */
        Dwarf_Locdesc val_loclist;
 
        /* DLT_BLOCK: the atrtribute value was in form:
            (Dwarf 1) block[24]
            (Dwarf 2) block, block[124] */
        Dwarf_Block val_block;
 
        /* DLT_STRING: the atrtribute value was in form:
            (Dwarf 1) string
            (Dwarf 2) string */
        char *val_string;
	
	/* DLT_BIGNUM: the attribute value was in form:
	    (Dwarf 1) data8
	    (Dwarf 2) data8, ref8, udata, ref_udata 
	    In all cases, libdwarf delivers a pointer to a struct
	    so that the size of the value is still only 4 bytes. */
	Dwarf_Bignum val_bignum;
    } at_value;
    struct Dwarf_Attribute *next;
};

/* frame information. */
/* libdwarf abstracts all of the Dwarf 2 frame opcodes into one of the following.  The number
   and meaning of each of the operands are given below.  Offset is the actual offset (which
   includes the factor in it).  Reg refers to a DW_CFA_R? (from dwarf2.h) register.  */

typedef enum {                      /* op1:     op2:    */
    Dwarf_frame_offset,             /* offset   reg    */
    Dwarf_frame_restore,            /* reg      ---    */
    Dwarf_frame_undefined,          /* reg      ---    */
    Dwarf_frame_same,               /* reg      ---    */
    Dwarf_frame_register,           /* reg      reg (sets rule of the first reg to be the 2nd). */
    Dwarf_frame_remember_state,     /* ---      ---    */
    Dwarf_frame_restore_state,      /* ---      ---    */
    Dwarf_frame_def_cfa,            /* reg      offset */
    Dwarf_frame_def_cfa_register,   /* reg      ---    */
    Dwarf_frame_def_cfa_offset,     /* offset   ---    */
    Dwarf_frame_pfp_offset          /* reg      offset */
} Dwarf_Frame_Insn_Type;

typedef struct Dwarf_Frame_Insn {
    Dwarf_Unsigned  loc;          /* This insn regards location loc. This will be relative to the
				     start of the function for all CIE's, and in FDE's this will be
				     a fully resolved address. */
    Dwarf_Frame_Insn_Type insn;   /* From the enumeration above. */
    Dwarf_Unsigned op1, op2;      /* The two operands as described above (also see
					6.4.2 of the ABI) */
} *Dwarf_Frame_Insn;

/* if the following value is stored in the return_address_register member of any CIE, then
   the augmentation for the CIE was not recognized, and therefore the information in the CIE
   and the FDE's are limited. */

#define DWARF_FRAME_UNKNOWN_AUG (-1)

typedef struct Dwarf_Frame_CIE {
    int return_address_register;
    Dwarf_Unsigned   n_instructions; /* Number of initial instructions in this CIE. */
    Dwarf_Frame_Insn insns;          /* The instructions. */
} *Dwarf_Frame_CIE;

typedef struct Dwarf_Frame_FDE *Dwarf_Frame_FDE;

struct Dwarf_Frame_FDE {
    Dwarf_Frame_CIE cie;
    Dwarf_Unsigned start_ip,address_range,n_instructions;
    struct Dwarf_Frame_Insn *insns;
    Dwarf_Frame_FDE previous_FDE,next_FDE;
};

/* Line numbers.  We're representing these in an open structure, 
   defined below, for efficiency when communicating with the client.
   But we'll keep pretending that Dwarf_Line is an opaque type for
   consistency with the rest of the functional interface.  (see the
   typedef for Dwarf_Line in the list below)
*/
struct Dwarf_Line
{
    /* The program-counter value corresponding to a machine instruction
       generated by the compiler */
    Dwarf_Unsigned address;

    /* The source line number.  Lines are numbered beginning at 1.
       The compiler may emit the value 0 in cases where an instruction 
       can't be attributed to any source line. */
    Dwarf_Unsigned line;

    /* The source column number.  Columns are numbered beginning at 1.
       The value 0 is reserved to indicate that a statement begins 
       at the "left edge" of the line. */
    Dwarf_Unsigned column;

    /* The source file corresponding to a machine instruction.
       This can be a fully-qualified path name if that is how the compiler
       specified it; or it can be a relative path name used in conjunction
       with the "dir" field.  (see define_file opcode (6.2.5.3)) */
    char *file;

    /* The directory name in which the file was found.  This will be NULL
       if "file" is a full pathname or if "file" was found in the current
       directory during compilation. */
    char *dir;

    /* The "file number" of the file specified in the "file" member. 
       This can be used to look up extra info like the modification date
       and size of the file.  (once the lookup is implemented - FIXME) */
    Dwarf_Unsigned filenum;

    /* A boolean indicating that the current instruction is the beginning
       of a source language statement. */
    Dwarf_Unsigned is_stmt:1;

    /* A boolean indicating that the current instruction is the beginning 
       of a basic block.  */
    Dwarf_Unsigned basic_block:1;

    /* A boolean indicating that the current address is the first byte
       after the end of a sequence of target machine instructions. */
    Dwarf_Unsigned end_sequence:1;
};

/* pubnames */
struct Dwarf_Pn_Tuple
{
    /* Symbol name */
    char *name;
 
    /* Offset from the start of the CU that corresponds to this chunk
       of pubnames to the DIE whose name matches "name" */
    Dwarf_Unsigned offset;
};
 
/* aranges */
struct Dwarf_Ar_Tuple
{
    /* Associates an address range with a compilation unit.
       The legal address range represented by the tuple is
       low_addr <= x < hi_addr */
 
    Dwarf_Addr low_addr;
 
    Dwarf_Addr high_addr;
};

/*
 *  OPAQUE TYPES
 */
typedef struct Dwarf_Debug	*Dwarf_Debug; 
typedef struct Dwarf_Die	*Dwarf_Die;
typedef struct Dwarf_Line	*Dwarf_Line;
typedef struct Dwarf_Pn_Tuple   *Dwarf_Pn_Tuple;
typedef struct Dwarf_Ar_Tuple   *Dwarf_Ar_Tuple;
typedef struct Dwarf_Attribute	*Dwarf_Attribute;
typedef struct Dwarf_Error	*Dwarf_Error;
typedef void	*Dwarf_Subscript;
typedef void	*Dwarf_Type;
typedef void	*Dwarf_Global;
typedef void	*Dwarf_LineTab;
typedef void	*Dwarf_Typemap;	


/*
 * ERROR HANDLER
 */
typedef int (*Dwarf_Handler) PARAMS ((Dwarf_Error error_desc));

/* globals to deal with error handling */
extern Dwarf_Unsigned _lbdw_errno;  /* error number */
extern Dwarf_Handler _lbdw_errhand; /* global error handler */
extern Dwarf_Addr _lbdw_errarg;     /* global error arg */


/*
 * DWARF_DEALLOC() type arguments
 */
#define       DLA_STRING      0x01    /* points to char */
#define       DLA_LOC         0x02    /* points to Dwarf_Loc */
#define       DLA_LOCDESC     0x03    /* points to Dwarf_Locdesc */
#define       DLA_ELLIST      0x04    /* points to Dwarf_Ellist */
#define       DLA_BOUNDS      0x05    /* points to Dwarf_Bounds */
#define       DLA_BLOCK       0x06    /* points to Dwarf_Block */
#define       DLA_DIE         0x07    /* points to Dwarf_Die */
#define       DLA_LINE        0x08    /* points to Dwarf_Line */
#define       DLA_ATTR        0x09    /* points to Dwarf_Attribute */
#define       DLA_TYPE        0x0A    /* points to Dwarf_Type */
#define       DLA_SUBSCR      0x0B    /* points to Dwarf_Subscr */
#define       DLA_GLOBAL      0x0C    /* points to Dwarf_Global */
#define       DLA_ERROR       0x0D    /* points to Dwarf_Error */
#define       DLA_LIST        0x0E    /* points to a list */
#define       DLA_TREE        0x0F    /* points to a DIE tree */

/*
 *  ARGUMENT VALUES
 */
/* dwarf_init() file_access arguments */
#define DLC_READ        0        /* read only access */
#define DLC_WRITE       1        /* write only access */
#define DLC_RDWR        2        /* read/write access */

/*
 * dwarf_openscn() access arguments
 */
#define DLS_BACKWARD    -1     /* slide backward to find line */
#define DLS_NOSLIDE     0      /* match exactly without sliding */
#define DLS_FORWARD     1      /* slide forward to find line */


/* 
 * LIBDWARF error numbers  
 */
#define DLE_NE          0x00        /* no error */
#define DLE_VMM         0x01        /* version of DWARF newer/older than libdwarf */
#define DLE_MAP         0x02        /* memory map failure */
#define DLE_ID          0x06        /* invalid descriptor */
#define DLE_IOF         0x07        /* I/O failure */
#define DLE_MAF         0x08        /* memory allocation failure */
#define DLE_IA          0x09        /* invalid argument */
#define DLE_MDE         0x0A        /* mangled debugging entry */
#define DLE_MLE         0x0B        /* mangled line number entry */
#define DLE_FNO         0x0C        /* file descriptor doesn't refer to open file */
#define DLE_FNR         0x0D        /* not a regular file */
#define DLE_FWA         0x0E        /* file opened with the wrong access */
#define DLE_NOB         0x0F        /* not an object file */
#define DLE_MOF         0x10        /* mangled object file headers */
#define DLE_UNKN        0x11        /* unknown libdwarf error */
#define DLE_LAST       	DLE_UNKN
#define DLE_LO_USER     0x10000  /* lower bound of implementation-specific codes */
#define DLE_D1          0x10001  /* dwarf 1 info */
#define DLE_IH          0x10002  /* invalid handle */
#define DLE_NOT_D2	0x10003	 /* not dwarf 2 info */
#define DLE_NO_ABBR_ENTRY	0x10004	 /* no entry in abbrev table */
#define DLE_NO_TAG	0x10005	 /* no tag in abbrev table */
#define DLE_BAD_CU_LEN	0x10006	 /* length of CU contribution to .debug_info doesn't match what's expected */
#define DLE_BAD_INFO_LEN 0x10007 /* length of .debug_info doesn't match what's expected */
#define DLE_UNSUPPORTED_FORM	0x10008	 /* the particular form is unsupported */ 
#define DLE_NO_FORM	0x10009	 /* the form entry is 0 */ 
#define DLE_BAD_LOC	0x1000A	 /* the particular location is unsupported */ 
#define DLE_NOABBR      0x1000B  /* no .debug_abbrev */
#define DLE_NOLOC       0x1000C	 /* no .debug_loc */
#define DLE_NOINFO      0x1000D  /* no .debug_info */
#define DLE_NOPUB       0x1000E  /* no .debug_pubnames */
#define DLE_NOARANG     0x1000F  /* no .debug_aranges */
#define DLE_NOLINE      0x10010  /* no .debug_line */
#define DLE_NOFRAME     0x10011  /* no .debug_frame */
#define DLE_NOMAC       0x10012  /* no .debug_mac */
#define DLE_NOSTR       0x10013  /* no .debug_str */
#define DLE_NOT_CU_DIE  0x10014  /* the particular DIE is not a CU-DIE */
#define DLE_NI   	0x10016  /* not implemented (for functions that aren't implemented) */
#define DLE_BAD_CU_OFFSET 0x10017 /* invalid CU offset */
#define DLE_BAD_SECT_OFFSET 0x10018 /* invalid section offset */
#define DLE_NOT_ARRAY 	0x1001E  /* DIE doesn't represent an array */
#define DLE_BAD_OPCODE 	0x1001F  /* Unrecognized line number opcode */ 
#define DLE_BAD_LINENO 	0x10020  /* negetive line number */ 
#define DLE_BAD_FREAD    0x10106
#define DLE_BAD_SEEK    0x1010E
#define DLE_BAD_SWAP    0x1010F
#define DLE_NO_FRAME    0x10110 /* Frame info could not find and FDE for the given ip. */
#define DLE_BAD_FRAME   0x10111 /* Could not parse the .debug_frame_section. */
#define DLE_HUGE_LEB128 0x10112 /* leb128 too large to represent */
#define DLE_UNK_AUG     0x10113 /* An unknown augmentation was found when reading a CIE  */
#define DLE_BAD_ERRNUM  0x10114 /* Err num out of range (dwarf_seterrsev) */
#define DLE_BAD_ERRSEV  0x10115 /* Err severity out of range(dwarf_seterrsev) */

#define DLE_END_ERR_TBL 0x1011A	

/* Leave DLE_END_ERR_TBL last because libdwarf_error() will search
   the error table until DLE_END_ERR_TBL is found, and then stop looking. */

/*
 *  RETURN VALUES INDICATING ERRORS
 */
#ifndef NULL
#define NULL             0                   /*  Pointer or Handle  */
#endif
#define DLV_BADADDR      ((Dwarf_Addr)0)     /* Address */
#define DLV_NOCOUNT      ((Dwarf_Signed)-1)  /* Count or Size */
#define DLV_BADOFFSET    ((Dwarf_Off)0)      /* Offset */


/*
 *  RETURN VALUES ERROR SEVERITY
 */
#define DLS_FATAL	-1
#define DLS_WARNING	 0
#define DLS_ERROR	 1

/*
 * Libdwarf extensions to the Dwarf 2.0 spec 
 *
 * These are similar to DW_OP_skip and DW_OP_bra, but instead of skipping
 * a certain number of bytes, they skip a certain number of location records.
 */
#define DW_OP_skip_lr 0xe0
#define DW_OP_bra_lr  0xe1


/*
 *  MACROS FOR DIE INFORMATION RETRIEVAL
 *
 *  These are simply cosmetic; they do not save you a function call.
 */
#define DIE_TAG(die) dwarf_tag(die)
#define DIE_HAS_ATTR(die, attr) dwarf_hasattr(die, attr)

/*
 *  MACROS FOR FAST ACCESS TO DIE ATTRIBUTE VALUES
 *
 *  These macros combine value retrieval with an attribute existence check.
 *  If the attribute does not exist in the given die, 0 (or NULL) will be
 *  returned.  Else the value will be returned in a form appropriate for
 *  the attribute. (see the definition of Dwarf_Attribute above)
 * 
 *  CAVEAT: since these macros simply package a call to dwarf_attr,
 *  they make use of one common temporary variable.  That means that
 *  using two or more macro calls within a single expression can have
 *  unpredictable results.  For example,
 *  unsigned span = DIE_ATTR_UNSIGNED(die, DW_AT_high_pc) -
 *                  DIE_ATTR_UNSIGNED(die, DW_AT_low_pc);
 *  will not do what you expect!
 *
 *  CAVEAT 2: the unsigned and signed versions return 0 if the attribute
 *  does not exist.  If 0 is a legitimate value for the attribute in the
 *  given context, use DIE_HAS_ATTR first, or else call dwarf_attr
 *  directly.
 */
extern Dwarf_Attribute _dw_tmp_attrval;	    /* defined in attr.c */
#define DIE_ATTR_UNSIGNED(die, attr) \
    ((_dw_tmp_attrval = dwarf_attr(die, attr)) ? \
      _dw_tmp_attrval->at_value.val_unsigned : 0)

#define DIE_ATTR_SIGNED(die, attr) \
    ((_dw_tmp_attrval = dwarf_attr(die, attr)) ? \
      _dw_tmp_attrval->at_value.val_signed : 0)
 
#define DIE_ATTR_STRING(die, attr) \
    ((_dw_tmp_attrval = dwarf_attr(die, attr)) ? \
      _dw_tmp_attrval->at_value.val_string : NULL)
 
#define DIE_ATTR_BLOCK(die, attr) \
    ((_dw_tmp_attrval = dwarf_attr(die, attr)) ? \
      _dw_tmp_attrval->at_value.val_block : NULL)
 
#define DIE_ATTR_LOCLIST(die, attr) \
    ((_dw_tmp_attrval = dwarf_attr(die, attr)) ? \
      _dw_tmp_attrval->at_value.val_loclist : NULL)
 
/*
 *  FUNCTION PROTOTYPES
 */

/* initialization and termination operations */
 
Dwarf_Debug
dwarf_init          PARAMS ((FILE *fp, Dwarf_Unsigned access,
			     Dwarf_Section sect_hdr, Dwarf_Unsigned sect_nbr));

void
dwarf_finish        PARAMS ((Dwarf_Debug dbg));
 
/* DIE delivery operations */
 
Dwarf_Die
dwarf_nextdie       PARAMS ((Dwarf_Debug dbg,
                             Dwarf_Die die));
 
Dwarf_Die
dwarf_siblingof     PARAMS ((Dwarf_Debug dbg,
                             Dwarf_Die die));
 
Dwarf_Die
dwarf_offdie        PARAMS ((Dwarf_Debug dbg, Dwarf_Die die,
                             Dwarf_Off offset));
 
Dwarf_Die
dwarf_cu_offdie     PARAMS ((Dwarf_Debug dbg, Dwarf_Die die,
                             Dwarf_Off offset));
 
Dwarf_Die
dwarf_pcfile        PARAMS ((Dwarf_Debug dbg,
                             Dwarf_Addr pc));
 
Dwarf_Die
dwarf_pcsubr        PARAMS ((Dwarf_Debug dbg,
                             Dwarf_Addr pc));
 
Dwarf_Die
dwarf_child         PARAMS ((Dwarf_Debug dbg, Dwarf_Die die));
 
Dwarf_Die
dwarf_parent        PARAMS ((Dwarf_Debug dbg, Dwarf_Die die));
 
/* query operations for DIEs */
 
Dwarf_Signed
dwarf_childcnt      PARAMS ((Dwarf_Debug dbg, Dwarf_Die die));
 
Dwarf_Half
dwarf_tag           PARAMS ((Dwarf_Die die));
 
Dwarf_Off
dwarf_dieoffset     PARAMS ((Dwarf_Die die));
 
Dwarf_Off
dwarf_cu_dieoffset  PARAMS ((Dwarf_Die die));
 
char*
dwarf_diename       PARAMS ((Dwarf_Die die));
 
Dwarf_Bool
dwarf_hasattr       PARAMS ((Dwarf_Die die, Dwarf_Unsigned attr));
 
Dwarf_Attribute
dwarf_attr          PARAMS ((Dwarf_Die die, Dwarf_Unsigned attr));
 
Dwarf_Die
dwarf_typeof        PARAMS ((Dwarf_Debug dbg, Dwarf_Die die));
 
Dwarf_Signed
dwarf_loclist       PARAMS ((Dwarf_Die die, Dwarf_Locdesc llbuf));
 
Dwarf_Locdesc
dwarf_stringlen     PARAMS ((Dwarf_Die die));
 
Dwarf_Addr
dwarf_lowpc         PARAMS ((Dwarf_Die die));
 
Dwarf_Addr
dwarf_highpc        PARAMS ((Dwarf_Die die));
 
Dwarf_Signed
dwarf_bytesize      PARAMS ((Dwarf_Die die));
 
Dwarf_Bool
dwarf_isbitfield    PARAMS ((Dwarf_Die die));
 
Dwarf_Signed
dwarf_bitsize       PARAMS ((Dwarf_Die die));
 
Dwarf_Signed
dwarf_bitoffset     PARAMS ((Dwarf_Die die));
 
Dwarf_Signed
dwarf_version       PARAMS ((Dwarf_Die die));

Dwarf_Signed
dwarf_srclang       PARAMS ((Dwarf_Die die));
 
Dwarf_Signed
dwarf_arrayorder    PARAMS ((Dwarf_Die die));
 
Dwarf_Signed
dwarf_attrlist      PARAMS ((Dwarf_Die die, Dwarf_Attribute* attrbuf));
 
/* query operations for types */
 
Dwarf_Bool
dwarf_isbasictype   PARAMS ((Dwarf_Type typ));
 
Dwarf_Signed
dwarf_typesize      PARAMS ((Dwarf_Type typ));
 
Dwarf_Signed
dwarf_encoding      PARAMS ((Dwarf_Type typ));
 
char*
dwarf_typename      PARAMS ((Dwarf_Type typ));
 
/* query operations for attributes */
 
Dwarf_Half
dwarf_atname        PARAMS ((Dwarf_Attribute attr));
 
Dwarf_Bool
dwarf_hasform       PARAMS ((Dwarf_Attribute attr, Dwarf_Unsigned form));

Dwarf_Off
dwarf_formref       PARAMS ((Dwarf_Attribute attr));
 
Dwarf_Addr
dwarf_formaddr      PARAMS ((Dwarf_Attribute attr));
 
Dwarf_Unsigned
dwarf_formudata     PARAMS ((Dwarf_Attribute attr));
 
Dwarf_Signed
dwarf_formsdata     PARAMS ((Dwarf_Attribute attr));
 
Dwarf_Block
dwarf_formblock     PARAMS ((Dwarf_Attribute attr));
 
char*
dwarf_formstring    PARAMS ((Dwarf_Attribute attr));
 
/* frame operations */

Dwarf_Frame_FDE
dwarf_frame_fetch_fde PARAMS((Dwarf_Debug dbg,Dwarf_Unsigned func_ip));

/* line number operations */
 
Dwarf_Line
dwarf_nextline      PARAMS ((Dwarf_Debug dbg, Dwarf_Line line));
 
Dwarf_Line
dwarf_prevline      PARAMS ((Dwarf_Debug dbg, Dwarf_Line line));
 
Dwarf_Signed
dwarf_pclines       PARAMS ((Dwarf_Debug dbg, Dwarf_Addr pc,
                             Dwarf_Line **linebuf, Dwarf_Signed slide));
 
Dwarf_Line
dwarf_dieline       PARAMS ((Dwarf_Die die));
 
Dwarf_Signed
dwarf_srclines      PARAMS ((Dwarf_Debug dbg, Dwarf_Die die,
                             Dwarf_Line *linebuf));
 
Dwarf_Bool
dwarf_is1stline     PARAMS ((Dwarf_Line line));
 
/* .debug_pubnames operations */
 
Dwarf_Die
dwarf_pubdie        PARAMS ((Dwarf_Debug dbg, char *name));
 
Dwarf_Die
dwarf_cu_pubdie     PARAMS ((Dwarf_Debug dbg, char *name));

Dwarf_Signed
dwarf_srcpubs       PARAMS ((Dwarf_Debug dbg, Dwarf_Die die,
                             Dwarf_Pn_Tuple *pubnames_buf));
 
/* .debug_aranges operations */
 
Dwarf_Die
dwarf_cu_rangedie   PARAMS ((Dwarf_Debug dbg, Dwarf_Addr addr));

Dwarf_Signed
dwarf_srcranges     PARAMS ((Dwarf_Debug dbg, Dwarf_Die die,
                             Dwarf_Ar_Tuple *aranges_buf));
/* utility operations */
 
Dwarf_Unsigned
dwarf_errno         PARAMS ((Dwarf_Error error));
 
char*
dwarf_errmsg        PARAMS ((Dwarf_Error error));
 
Dwarf_Handler
dwarf_seterrhand    PARAMS ((Dwarf_Handler errhand));
 
Dwarf_Addr
dwarf_seterrarg     PARAMS ((Dwarf_Addr errarg));
 
char *
dwarf_seterrmsg	    PARAMS ((Dwarf_Unsigned, char *));

Dwarf_Signed
dwarf_seterrsev	    PARAMS ((Dwarf_Unsigned, Dwarf_Signed));

void
dwarf_dealloc       PARAMS ((void* space, Dwarf_Unsigned typ));
 
#endif /* _LIBDWARF_H */
