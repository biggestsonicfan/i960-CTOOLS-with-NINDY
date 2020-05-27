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

#ifndef	FRAME_H
#define FRAME_H

typedef struct Dwarf_Frame_Real_CIE {
    struct Dwarf_Frame_CIE real_cie;
    int code_alignment_factor;
    int data_alignment_factor;
} *Dwarf_Frame_Real_CIE;

typedef struct Dwarf_Frame_Real_CIE_Tree_Node {
    Dwarf_Frame_Real_CIE the_cie;
    int offset_in_section;
    struct Dwarf_Frame_Real_CIE_Tree_Node *left,*right;  /* A sorted tree based
							    on offsets in section. */
} *Dwarf_Frame_Real_CIE_Tree_Node;


typedef struct Dwarf_Frame_Information {
    /* List of all UNIQUE CIE's from .debug_frame. */
    /* Many FDE's may reference one CIE. */

    Dwarf_Frame_Real_CIE *CIE_table;
    Dwarf_Unsigned  n_CIE;      /* Number of entries in CIE_table. */

    /* This is a place to store a mapping from file offset to a member
       of the CIE_table. */
    Dwarf_Frame_Real_CIE_Tree_Node root_of_cie_offsets_tree;

    /* List of all FDE's from .debug_frame.  It is sorted by start_ip */
    Dwarf_Frame_FDE FDE_table;
    Dwarf_Unsigned n_FDE;  /* Number of FDE's in FDE_table. */
} *Dwarf_Frame_Information;

/* Given a DBG, return Dwarf_Frame_Information: */

#define DWARF_FRAME_INFORM(dbg) ((Dwarf_Frame_Information) ((dbg)->frame_info))

#endif	/* FRAME_H */
