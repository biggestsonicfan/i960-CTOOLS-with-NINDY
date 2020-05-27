/*****************************************************************************
 * Copyright (c) 1990, 1991, 1992, 1993 Intel Corporation
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
 ***************************************************************************/

/* Associate a numeric constant with a string. */
struct name_assoc
{
    unsigned long code;         /* Numeric name */
    char *name;                 /* Human-readable name */
};

/* Associate a numeric constant with a function. */
struct func_assoc
{
    unsigned long code;         /* Numeric name */
    void (*func)();      /* Function to associate with this number */
};

/* Package a list of associations */
struct association_list
{
    unsigned long size;         /* Number of elements in list */
    void *list;         /* Head pointer to struct association list */
};
/* Describe a location operation */
struct loc_op
{
    char numops;    /* 0, 1, or 2 */
    char sign;      /* 1 == expects signed operand, 0 == unsigned */
    char opsize;    /* if numops > 0, then 0, 1, 2, 4, or 8; 0 == LEB128 */
};

/* Output: make a semi-fancy output buffer system so that we can squish the
   output as much as possible while still preserving readability.
   This is initialized in init_outbuf() and printed in flush_outbuf() */
struct output_buffer
{
    unsigned long addr; /* Address of current DIE relative to current C.U. */
    char *tag;          /* Dwarf tag */
    char *name;         /* Attribute name, or blank if none */
    char *sibling;      /* Sibling address, or blank if none */
    char *data;         /* Big buffer: format varies per attribute type */
    char *dp;           /* Moves along data[] a la printf */
    int namelen;        /* For resizing name buffer if needed */
    int indent;         /* Number of spaces to indent output */
#  define current_indent() (outbuf.indent)
#  define bump_indent()    (outbuf.indent += 2)
#  define drop_indent()    (outbuf.indent -= 2)
};

#define move_dp *(outbuf.dp += strlen(outbuf.dp)) = 0;

/* globals defined in dwarfdmp.c & gdmp960.c */
extern struct output_buffer outbuf;
extern char flagseen[256];

/* Long, boring lists to associate attributes with descriptive strings;
   defined in names.c */
extern struct association_list  tag_name_list;
extern struct association_list  attribute_name_list;
extern struct association_list  form_name_list;
extern struct association_list  operation_name_list;
extern struct association_list  basetype_name_list;
extern struct association_list  access_name_list;
extern struct association_list  visibility_name_list;
extern struct association_list  virtuality_name_list;
extern struct association_list  language_name_list;
extern struct association_list  idcase_name_list;
extern struct association_list  callingconvention_name_list;
extern struct association_list  inlining_name_list;
extern struct association_list  ordering_name_list;
extern struct association_list  descriptor_name_list;
extern struct association_list  flag_name_list;
extern struct association_list  line_standard_name_list;
extern struct association_list  line_extended_name_list;
extern struct association_list  macinfo_name_list;
extern struct association_list  frameinfo_name_list;
extern struct association_list  framereg_name_list;
 
extern struct association_list  *master_name_list[];

/* Long, somewhat more interesting list of functions that do pretty-printing
   on the given attribute value; defined in print.c */
extern struct association_list   pretty_printer_list;

/* A table to help decipher location expressions; defined in ops.h;
   Used in print.c */
extern struct loc_op optable[];

/* Macros to connect a lookup request with the correct association list */
#define lookup_tag_name(key) \
    lookup_association_name(tag_name_list.list, tag_name_list.size, key)
#define lookup_attribute_name(key) \
    lookup_association_name(attribute_name_list.list, attribute_name_list.size, key)
#define lookup_form_name(key) \
    lookup_association_name(form_name_list.list, form_name_list.size, key)
#define lookup_operation_name(key) \
    lookup_association_name(operation_name_list.list, operation_name_list.size, key)
#define lookup_basetype_name(key) \
    lookup_association_name(basetype_name_list.list, basetype_name_list.size, key)
#define lookup_access_name(key) \
    lookup_association_name(access_name_list.list, access_name_list.size, key)
#define lookup_visibility_name(key) \
    lookup_association_name(visibility_name_list.list, visibility_name_list.size, key)
#define lookup_virtuality_name(key) \
    lookup_association_name(virtuality_name_list.list, virtuality_name_list.size, key)
#define lookup_language_name(key) \
    lookup_association_name(language_name_list.list, language_name_list.size, key)
#define lookup_idcase_name(key) \
    lookup_association_name(idcase_name_list.list, idcase_name_list.size, key)
#define lookup_callingconvention_name(key) \
    lookup_association_name(callingconvention_name_list.list, callingconvention_name_list.size, key)
#define lookup_inlining_name(key) \
    lookup_association_name(inlining_name_list.list, inlining_name_list.size, key)
#define lookup_ordering_name(key) \
    lookup_association_name(ordering_name_list.list, ordering_name_list.size, key)
#define lookup_descriptor_name(key) \
    lookup_association_name(descriptor_name_list.list, descriptor_name_list.size, key)
#define lookup_flag_name(key) \
    lookup_association_name(flag_name_list.list, flag_name_list.size, key)
#define lookup_line_standard_name(key) \
    lookup_association_name(line_standard_name_list.list, line_standard_name_list.size, key)
#define lookup_line_extended_name(key) \
    lookup_association_name(line_extended_name_list.list, line_extended_name_list.size, key)
#define lookup_macinfo_name(key) \
    lookup_association_name(macinfo_name_list.list, macinfo_name_list.size, key)
#define lookup_frameinfo_name(key) \
    lookup_association_name(frameinfo_name_list.list, frameinfo_name_list.size, key)
#define lookup_framereg_name(key) \
    lookup_association_name(framereg_name_list.list, framereg_name_list.size, key)

char *lookup_association_name();
void (*lookup_pretty_printer())();

int compare_pretties();
int compare_asses();
