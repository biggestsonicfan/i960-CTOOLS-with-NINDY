/* segs.h - segment (section) declarations
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

/* This is the typedef for an internal structure to describe generalized
 * segments.  (aka "sections".  The term segment is historical in gas.)
 *
 * There can be any number of these in a program.  The only place that 
 * a new one can be created is in s_section() in read.c.  They are 
 * organized as a resizable list of segS's, but the list will usually
 * be accessed by index as if it were an array.  In this case the array
 * index is the segment number. 
 *
 * Note that we carry this generalized segment description around until 
 * the very end (write_*_file, where * is the OMF), when each OMF provides
 * a conversion routine to convert this structure into the OMF-specific
 * structure that will be written to the object file.
 */
struct segment
{
    /* Segment number; anything, even < 0 */
    short	    seg_no;		

    /* Segment attributes, e.g. lomem */
    unsigned short  seg_flags;	
	
    /* Segment type (ELF only) */
    /* FIXME: OMF-dependent */
    unsigned long   seg_elf_name; /* index into string table */

    /* One of the current segT enums; defined in as.h */
    segT	    seg_type;	
    
    /* Name string */
    char	    *seg_name;	

    /* Number of bytes in the segment */
    unsigned long   seg_size;
    
    /* Alignment.  Power-of-2, converted to a number in write_obj_file */
    unsigned char   seg_align;

    /* Number of relocation entries (COFF only) */
    unsigned long   seg_nreloc;

    /* Number of line number entries (COFF only) */
    unsigned long   seg_nlnno;

    /* Starting physical address */
    unsigned long   seg_paddr;

    /* Starting virtual address (FIXME: difference from paddr?) */
    unsigned long   seg_vaddr;

    /* A link to another section number.  Means different things in 
       different contexts. */
    short 	    seg_link;

    /* A link to another section number.  Means different things in 
       different contexts. */
    short 	    seg_info;

    /* File offset of start of raw data for this section.  Used just before
       writing the OMF-specific section header. */
    unsigned long   seg_scnptr;

    /* File offset of start of relocation data for this section. (COFF only)
       Used just before writing the OMF-specific section header. */
    unsigned long   seg_relptr;

    /* File offset of start of relocation data for this section. (COFF only)
       Used just before writing the OMF-specific section header. */
    unsigned long   seg_lnnoptr;

    /* Root of reloc chain for this segment */
    struct fix	    *seg_fix_root;

    /* End of reloc chain */
    struct fix	    *seg_fix_last;

    /* Root of frag chain for this segment */
    struct frag	    *seg_frag_root;	
    
    /* End of frag chain */
    struct frag	    *seg_frag_last;	

#ifdef OBJ_COFF
    /* FIXME: this is OMF-dependent */
    /* Root of line number chain for this segment (COFF only) */
    lineno	    *seg_line_root;
    
    /* End of line number chain (COFF only) */
    lineno	    *seg_line_last; 
#endif
};

typedef struct segment segS;


/*
 * The macros below stand for the special "segments" used in expression 
 * evaluation, plus .text, .data, and .bss, i.e. all the segments not created
 * directly by the user in the program text.
 *
 * IMPORTANT:  The ORDERING of these guys is significant.  It follows the 
 * order in which segments are created in segs_begin().  If the order is ever
 * changed, or members added or deleted, update segs_begin() in segs.c also.
 *
 * FIXME-SOMEDAY:  A better solution is to reconcile the order in segs[]
 * with the order of the original segT enum given in as.h.  This would involve
 * the logic of b.out segment access macros, and hence is non-trivial.
 */
#define	MYTHICAL_REGISTER_SEGMENT	0
#define	MYTHICAL_ABSENT_SEGMENT		1
#define	MYTHICAL_PASS1_SEGMENT		2
#define	MYTHICAL_BIG_SEGMENT		3
#define	MYTHICAL_DIFFERENCE_SEGMENT	4
#define	MYTHICAL_GOOF_SEGMENT		5
#define	MYTHICAL_DEBUG_SEGMENT		6
#define	MYTHICAL_ABSOLUTE_SEGMENT	7
#define	MYTHICAL_UNKNOWN_SEGMENT	8

/* 
 * It is useful to know the number of the first substantive segment,
 * i.e. the first segment containing program code or data.
 */
#define FIRST_PROGRAM_SEGMENT   	9

/* 
 * Macros to set and get info from segment structs.
 */
#define SEG_LOMEM_MASK		0x1
#define SEG_IS_LOMEM(seg)	(segs[(seg)].seg_flags & SEG_LOMEM_MASK)

#define SECT_ATTR_LOMEM       (SEG_LOMEM_MASK)
#define SECT_ATTR_INFO        0x0002
#define SECT_ATTR_ALLOC       0x0004
#define SECT_ATTR_READ        0x0008
#define SECT_ATTR_WRITE       0x0010
#define SECT_ATTR_EXEC        0x0020
#define SECT_ATTR_SUPER_READ  0x0040
#define SECT_ATTR_SUPER_WRITE 0x0080
#define SECT_ATTR_SUPER_EXEC  0x0100
#define SECT_ATTR_MSB         0x0200
#define SECT_ATTR_PI          0x0400
#define SECT_ATTR_BSS_T       0x0800
#define SECT_ATTR_DATA_T      0x1000

#define SECT_ATTR_TEXT  (SECT_ATTR_ALLOC | SECT_ATTR_EXEC | SECT_ATTR_READ)
#define SECT_ATTR_DATA  (SECT_ATTR_ALLOC | SECT_ATTR_WRITE | SECT_ATTR_READ | SECT_ATTR_DATA_T)
#define SECT_ATTR_BSS   (SECT_ATTR_ALLOC | SECT_ATTR_WRITE | SECT_ATTR_READ | SECT_ATTR_BSS_T)

#if defined( OBJ_ELF )
#	define SEG_IS_BSS(seg)         (segs[seg].seg_type == SEG_BSS)
#	define SEG_IS_BIG_ENDIAN(seg)  (segs[(seg)].seg_flags & SECT_ATTR_MSB)
#endif
#if defined( OBJ_COFF )
#	define SEG_IS_BSS(seg)         ((segs[seg].seg_flags & SECT_ATTR_BSS) == SECT_ATTR_BSS)
#	define SEG_IS_BIG_ENDIAN(seg)  (flagseen['G'])
#endif
#if defined( OBJ_BOUT )
#	define SEG_IS_BSS(seg)         0
#	define SEG_IS_BIG_ENDIAN(seg)  0
#endif

#define SEG_GET_NAME(seg)	(segs[(seg)].seg_name)
#define SEG_GET_TYPE(seg)	(segs[(seg)].seg_type)
#define SEG_GET_FLAGS(seg)	(segs[(seg)].seg_flags)
#define SEG_GET_FRAG_ROOT(seg)	(segs[(seg)].seg_frag_root)
#define SEG_GET_FRAG_LAST(seg)	(segs[(seg)].seg_frag_last)
#define SEG_GET_FIX_ROOT(seg)	(segs[(seg)].seg_fix_root)
#define SEG_GET_FIX_LAST(seg)	(segs[(seg)].seg_fix_last)
#ifdef OBJ_COFF
/* FIXME: OMF-dependent */
#define SEG_GET_LINE_ROOT(seg)	(segs[(seg)].seg_line_root)
#define SEG_GET_LINE_LAST(seg)	(segs[(seg)].seg_line_last)
#endif
#define SEG_GET_ALIGN(seg)	(segs[(seg)].seg_align)
#define	SEG_GET_SIZE(seg)	(segs[(seg)].seg_size)
#define	SEG_GET_NRELOC(seg) 	(segs[(seg)].seg_nreloc)
#define	SEG_GET_NLNNO(seg)	(segs[(seg)].seg_nlnno)

#define SEG_SET_LOMEM(seg)	(segs[(seg)].seg_flags |= SEG_LOMEM_MASK)
#define SEG_SET_ALIGN(seg, al)	(segs[(seg)].seg_align = (al))
#define SEG_SET_SIZE(seg, sz)	(segs[(seg)].seg_size = (sz))
#define SEG_SET_LINK(seg, l)	(segs[(seg)].seg_link = (l))
#define SEG_SET_INFO(seg, i)	(segs[(seg)].seg_info = (i))
#define SEG_SET_FRAG_ROOT(seg, f)	(segs[(seg)].seg_frag_root = (f))
#define SEG_SET_FRAG_LAST(seg, f)	(segs[(seg)].seg_frag_last = (f))
#ifdef OBJ_COFF
/* FIXME: OMF-dependent */
#define SEG_SET_LINE_ROOT(seg, ln)	(segs[(seg)].seg_line_root = (ln))
#define SEG_SET_LINE_LAST(seg, ln)	(segs[(seg)].seg_line_last = (ln))
#endif
#define	SEG_SET_NRELOC(seg, rels) (segs[(seg)].seg_nreloc = (rels))
#define	SEG_SET_NLNNO(seg, lns)	(segs[(seg)].seg_nlnno = (lns))
#define	SEG_SET_PADDR(seg, pad)	(segs[(seg)].seg_paddr = (pad))
#define SEG_SET_VADDR(seg, vad)	(segs[(seg)].seg_vaddr = (vad))
#define	SEG_SET_RELPTR(seg, rp)	(segs[(seg)].seg_relptr = (rp))
#define	SEG_SET_SCNPTR(seg, sp)	(segs[(seg)].seg_scnptr = (sp))
#define SEG_SET_LNNOPTR(seg, lp)	(segs[(seg)].seg_lnnoptr = (lp))

/* 
 * Get the name of a segment. (for error messages, etc.)
 */
#define segment_name(v)	(segs[(v)].seg_name)
