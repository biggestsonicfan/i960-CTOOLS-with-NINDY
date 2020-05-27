
/*(c****************************************************************************** *
 * Copyright (c) 1994 Intel Corporation
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
 *****************************************************************************c)*/

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"

#define OPC_MASK     0xff000000 /* OPCODE mask. */
#define CALLS        0x66003800	/* Template for 'calls' instruction	*/
#define CALLSG13     0x6600301d	/* Template for 'calls g13' instruction	*/
#define BAL          0x0b000000	/* Template for 'bal' instruction	*/
#define BALXG14      0x85f00000	/* Template for 'balx' instruction	*/
#define BALXG14_MASK 0xfff80000	/* Allows simple determination if this word was already relocated. */
#define LDAG13       0x8ce80000 /* Template for LDA instruction. */

/* Return the absolute resting place of the asymbol. */
static unsigned long absolute_value(as)
    asymbol *as;
{
    unsigned long v = 0;

    if (as->section) {
	if (!as->section->output_section) {
	    fprintf(stderr,"Found a symbol that resides in no output section, and is used in a relocation\n");
	    fprintf(stderr,"directive named: %s.\n",as->name);
	    exit(1);
	}
	v += as->section->output_offset + as->section->output_section->vma;
    }

    if (!(as->flags & BSF_FORT_COMM))
	    v += as->value;

    return v;
}

static void elf_perform_no_relocation();
static void elf_perform_12bit_abs_relocation();
static void elf_perform_32bit_abs_relocation();
static void elf_perform_24bit_rel_relocation();
static void elf_perform_optcall_relocation();
static void elf_perform_optcallx_relocation();
static void elf_perform_32bit_abs_sub_relocation();

static reloc_class(abfd,as,isect,inter_file,inter_section)
    bfd *abfd;
    asymbol *as;
    asection *isect;
    int *inter_file,*inter_section;
{
    *inter_section = *inter_file = 0;

    if (as->the_bfd != abfd)
	    *inter_file = 1;
    else if (isect != as->section && as->section)
	    *inter_section = 1;
}

static void elf_perform_no_relocation(abfd,as,x1,x2,isec,retval,reloc)
    bfd *abfd;
    asymbol *as;
    unsigned long *x1,*x2;
    asection *isec;
    bfd_reloc_status_enum_type *retval;
    arelent *reloc;
{
    *retval = bfd_reloc_ok;
}

static reloc_howto_type
	howto_reloc_rel_ip24     = { "ip_24",  0, bfd_reloc_type_24bit_pcrel };

static reloc_howto_type
	howto_reloc_rel_none     = { "none",   0, bfd_reloc_type_none };

static reloc_howto_type
	howto_reloc_rel_32       = { "rel_32", 0, bfd_reloc_type_32bit_abs };

static void elf_perform_12bit_abs_relocation(abfd,as,x1,x2,isec,retval,reloc)
    bfd *abfd;
    asymbol *as;
    unsigned long *x1,*x2;
    asection *isec;
    bfd_reloc_status_enum_type *retval;
    arelent *reloc;
{
    int inter_file,inter_section;
    unsigned long relocation = absolute_value(as);

    reloc_class(abfd,as,isec,&inter_file,&inter_section);
    if (!inter_file) {
	if (as->section)
		relocation -= as->section->vma;
	if (!(as->flags & (BSF_FORT_COMM|BSF_OLD_COMMON)) )
		relocation -= as->value;
    }

    if ((((*x1) & 0xfff) + relocation) & 0xfffff000)
	    /* This relocation will overflow the twelve address bits
	       available in the MEMA instruction */
	    *retval = bfd_reloc_outofrange;
    if (as->flags & BSF_ABSOLUTE)
	    reloc->howto = &howto_reloc_rel_none;
    *x1 += relocation;
}

static void elf_perform_32bit_abs_relocation(abfd,as,x1,x2,isec,retval,reloc)
    bfd *abfd;
    asymbol *as;
    unsigned long *x1,*x2;
    asection *isec;
    bfd_reloc_status_enum_type *retval;
    arelent *reloc;
{
    int inter_file,inter_section;
    unsigned long relocation = absolute_value(as),result;

    reloc_class(abfd,as,isec,&inter_file,&inter_section);
    if (!inter_file) {
	if (as->section)
		relocation -= as->section->vma;
	if (!(as->flags & (BSF_FORT_COMM|BSF_OLD_COMMON)) )
		relocation -= as->value;
    }

    *x1 += relocation;
    if (as->flags & BSF_ABSOLUTE)
	    reloc->howto = &howto_reloc_rel_none;
}

static void elf_perform_24bit_rel_relocation(abfd,as,x1,x2,isec,retval,reloc)
    bfd *abfd;
    asymbol *as;
    unsigned long *x1,*x2;
    asection *isec;
    bfd_reloc_status_enum_type *retval;
    arelent *reloc;
{
    int inter_file,inter_section;
    unsigned long relocation,offset,sign,least_two_bits = (*x1) & 3;

    reloc_class(abfd,as,isec,&inter_file,&inter_section);
    if (!(as->flags & (BSF_FORT_COMM|BSF_OLD_COMMON)) && !inter_file && !inter_section)
	    /* Both intra_file and intra_section. */
	    relocation = 0;
    else if (inter_file || (as->flags & (BSF_FORT_COMM|BSF_OLD_COMMON))) { 
	/* inter_file implies both inter file and inter section. */
	relocation = absolute_value(as) -
		(isec->output_section->vma - isec->vma)
			- isec->output_offset;
    }
    else {  /* Must be inter_section, intra_file. */
	relocation = ((as->section->output_section->vma +
		       as->section->output_offset) -
		      (isec->output_section->vma + isec->output_offset)) -
			      /* The below expression is the OLD distance between the two
				 sections: */
			      (as->section->vma - isec->vma);
    }
    offset = (*x1) & 0x00ffffff;
    if (offset & 0x00800000)
	    /* sign-extend */
	    offset |= 0xff000000;
    offset += relocation;
    sign = offset & 0xff800000;
    if ( (sign != 0xff800000) && (sign != 0x00000000) )
	    /* Sign of 32-bit value not same as sign
	     * of 24-bit value: we overflowed. */
	    *retval = bfd_reloc_overflow;
    *x1 = ((*x1) & 0xff000000) | (offset & 0x00ffffff);

    if (least_two_bits != ((*x1) & 3))
	    *retval = bfd_reloc_dangerous;

    if (as->section && as->section->output_section == isec->output_section)
	    reloc->howto = &howto_reloc_rel_none;  /* Remove it so we do not have to deal
						      with it again. */
}

static void elf_perform_optcall_relocation(abfd,as,x1,x2,isec,retval,reloc)
    bfd *abfd;
    asymbol *as;
    unsigned long *x1,*x2;
    asection *isec;
    bfd_reloc_status_enum_type *retval;
    arelent *reloc;
{
    if (as->flags & BSF_HAS_BAL) {
	if (0 == (abfd->flags & SUPP_BAL_OPT)) {
	    unsigned offset;

	    /* Do not allow the relocation to occur twice.  To keep from doing
	     so, we peek into the code.  If a BAL is already there, we do not
	     add the offset for the leaf into the instruction again: */
	    if (((*x1) & OPC_MASK) == BAL)
		    offset = 0;
	    else
		    offset = as->related_symbol->value - as->value;

	    /* First change call -> bal instruction, retaining the addressing
	       mode: */
	    *x1 = BAL | (((*x1) & (~OPC_MASK)) + offset);

	    /* Switch the relocation type to ip 24 for the future so this relocation will
	       not occur twice. */
	    reloc->howto = &howto_reloc_rel_ip24;

	    /* Now, calculate the 24bit relative relocation on the same word: */
	    elf_perform_24bit_rel_relocation(abfd,as,x1,0,
						 isec,retval,reloc);
	}
	else {
	    /* The user has suppressed call -> bal optimization relocations.
	       Do a 24 bit relocation on the word only.  To do so, use
	       the usual 24 bit relocation code.  But, before we do, we save the
	       relocation type away because the 24bit code can eliminate it.: */
	    reloc_howto_type *tmp = reloc->howto;
	    
	    elf_perform_24bit_rel_relocation(abfd,as,x1,0,isec,retval,reloc);
	    reloc->howto = tmp;
	}
    }
    else if (as->flags & BSF_HAS_SCALL) {
	if (0 == (abfd->flags & SUPP_CALLS_OPT)) {
	    unsigned sysproc_index = as->related_symbol->value;

	    if (sysproc_index >= 0 && sysproc_index <= 31) {

		/* REMOVE OLD CODE COMPLETELY.  OVERWRITE WITH CALLS spi. */
		*x1 = CALLS | sysproc_index;

		/* Change relocation directive to no relocation: */
		reloc->howto = &howto_reloc_rel_none;
	    }
	    else if (sysproc_index == -1)
		    *retval = bfd_reloc_undefined;
	    else
		    *retval = bfd_reloc_outofrange;
	}
	else {
	    /* The user has suppressed call -> calls optimization relocations.
	       Do a 24 bit relocation on the word only.  To do so, use
	       the usual 24 bit relocation code.  But before we do so, we save off
	       the relocation howto because the 24bit code can eliminate it: */
	    if (as->section) {
		reloc_howto_type *tmp = reloc->howto;
		elf_perform_24bit_rel_relocation(abfd,as,x1,0,isec,retval,reloc);
		reloc->howto = tmp;
	    }
	    else
		    *retval = bfd_reloc_no_code_for_syscall;
	}
    }
    else {
	/* Switch the relocation type to ip 24: */
	reloc->howto = &howto_reloc_rel_ip24;
	/* Leave the call instruction alone and relocate the relative 24 bits: */
	elf_perform_24bit_rel_relocation(abfd,as,x1,0,isec,retval,reloc);
    }
}

static void elf_perform_optcallx_relocation(abfd,as,x1,x2,isec,retval,reloc)
    bfd *abfd;
    asymbol *as;
    unsigned long *x1,*x2;
    asection *isec;
    bfd_reloc_status_enum_type *retval;
    arelent *reloc;
{
    if (as->flags & BSF_HAS_BAL) {
	if (0 == (abfd->flags & SUPP_BAL_OPT)) {
	    unsigned offset;

	    /* Do not allow the relocation to occur twice.  To keep from doing
	     so, if we see that the code alreay contains a BALXG14 instruction, we
	     do not add the offset in again: */

	    if (((*x1) & BALXG14_MASK) == BALXG14)
		    offset = 0;
	    else
		    offset = as->related_symbol->value - as->value;

	    /* First change callx -> balx instruction, retaining the addressing
	       mode: */
	    *x1 = BALXG14 | ((*x1) & (~BALXG14_MASK));
	    *x2 += offset;
	    /* Now, calculate the 32bit absolute relocation on the following word: */
	    elf_perform_32bit_abs_relocation(abfd,as,x2,0,isec,retval,reloc);
	    /* Switch the relocation to a 32bit absolute relocation to prevent this
	       relocation from happening twice: */
	    reloc->address += 4;
	    reloc->howto = &howto_reloc_rel_32;
	}
	else
		/* The user has suppressed callx -> balx optimization relocations.
		   Do a 32 bit relocation on the FOLLOWING word only.  To do so, use
		   the usual 32 bit relocation code: */
		elf_perform_32bit_abs_relocation(abfd,as,x2,0,isec,retval,reloc);
    }
    else if (as->flags & BSF_HAS_SCALL) {
	if (0 == (abfd->flags & SUPP_CALLS_OPT)) {
	    unsigned sysproc_index = as->related_symbol->value;

	    if (sysproc_index >= 0 && sysproc_index <= 259) {

		/* REMOVE OLD CODE COMPLETELY.  OVERWRITE WITH CALLS G13. */
		*x1 = LDAG13 | sysproc_index;

		/* The following word gets a CALLS G13 instruction: */
		*x2 = CALLSG13;

		/* Change relocation directive to no relocation: */
		reloc->howto = &howto_reloc_rel_none;
	    }
	    else if (sysproc_index == -1)
		    *retval = bfd_reloc_undefined;
	    else
		    *retval = bfd_reloc_outofrange;
	}
	else {
	    /* The user has suppressed callx -> calls optimization relocations.
	       Do a 32 bit relocation on the FOLLOWING word only.  To do so, use
	       the usual 32 bit relocation code below: */
	    if (as->section)
		    elf_perform_32bit_abs_relocation(abfd,as,x2,0,isec,retval,reloc);
	    else
		    *retval = bfd_reloc_no_code_for_syscall;
	}
    }
    else {
	/* Leave the callx instruction alone in x1, relocate x2 now: */
	elf_perform_32bit_abs_relocation(abfd,as,x2,0,isec,retval,reloc);
	/* Switch the relocation to a 32bit absolute relocation to save
	   us from having to do this relocation again: */
	reloc->address += 4;
	reloc->howto = &howto_reloc_rel_32;
    }
}

static void elf_perform_32bit_abs_sub_relocation(abfd,as,x1,x2,isec,retval,reloc)
    bfd *abfd;
    asymbol *as;
    unsigned long *x1,*x2;
    asection *isec;
    bfd_reloc_status_enum_type *retval;
    arelent *reloc;
{
    int inter_file,inter_section;
    unsigned long relocation = absolute_value(as),result;

    reloc_class(abfd,as,isec,&inter_file,&inter_section);
    if (!inter_file) {
	if (as->section)
		relocation -= as->section->vma;
	if (!(as->flags & (BSF_FORT_COMM|BSF_OLD_COMMON)) )
		relocation -= as->value;
    }

    *x1 -= relocation;
}

bfd_reloc_status_enum_type
DEFUN(bfd_perform_relocation,(abfd, reloc, data, isec, obfd),
      bfd *abfd AND
      arelent *reloc AND
      PTR data AND
      asection *isec AND	/* Input section */
      bfd *obfd)		/* Output bfd: non-NULL means relocations will
				 *	get written to output file
				 */
{
    asymbol *symp                     = *(reloc->sym_ptr_ptr);
    bfd_reloc_status_enum_type retval = bfd_reloc_ok;
    long x1,x2;
	
    if (reloc->address > (bfd_vma)(isec->size))
	    retval = bfd_reloc_outofrange;
    else {
	int reloc_type = reloc->howto->reloc_type;

#define ELF_GET_32(ADDR,BIG) (BIG ? _do_getb32((bfd_byte *)ADDR) : _do_getl32((bfd_byte *)ADDR))

	data = (PTR)((char *)data + reloc->address);
	x1 = ELF_GET_32(data,isec->flags & SEC_IS_BIG_ENDIAN);
	if (reloc->howto->reloc_type == bfd_reloc_type_opt_callx)
		x2 = ELF_GET_32(((char*)data)+4,isec->flags & SEC_IS_BIG_ENDIAN);

	if ((symp->flags & BSF_UNDEFINED) ||
	    (obfd && (symp->flags & BSF_HAS_SCALL) && (symp->related_symbol->value == -1))) {
	    if (!obfd)
		    retval = bfd_reloc_undefined;
	    else if (reloc->howto->reloc_type == bfd_reloc_type_24bit_pcrel ||
		     reloc->howto->reloc_type == bfd_reloc_type_opt_call) {
		unsigned long offset = x1 & ~OPC_MASK,sign;
		unsigned relocation = isec->vma -
			(isec->output_section->vma + isec->output_offset);

		if (offset & 0x00800000)
			offset |= 0xff000000;
		offset += relocation;
		sign = offset & 0xff800000;
		if ((sign != 0xff800000) && (sign != 0))
			retval = bfd_reloc_overflow;
		x1 = (x1 & 0xff000000) | (offset & 0x00ffffff);
	    }
	}
	else {
	    int idx;
	    typedef void (*void_func_ptr)();
	    static void_func_ptr reloc_funcs[7] = {
		elf_perform_12bit_abs_relocation,
		elf_perform_32bit_abs_relocation,
		elf_perform_24bit_rel_relocation,
		elf_perform_optcall_relocation,
		elf_perform_optcallx_relocation,
		elf_perform_32bit_abs_sub_relocation,
		elf_perform_no_relocation,
	    };

	    switch (reloc->howto->reloc_type) {
	case bfd_reloc_type_12bit_abs:
		idx = 0;
		break;
	case bfd_reloc_type_32bit_abs:
		idx = 1;
		break;
	case bfd_reloc_type_24bit_pcrel:
		idx = 2;
		break;
	case bfd_reloc_type_opt_call:
		idx = 3;
		break;
	case bfd_reloc_type_opt_callx:
		idx = 4;
		break;
	case bfd_reloc_type_32bit_abs_sub:
		idx = 5;
		break;
	case bfd_reloc_type_none:
		idx = 6;
		break;
	default:
		idx = -1;
		break;
	    }
	    reloc_funcs[idx](abfd,symp,&x1,&x2,isec,&retval,reloc);
	}

#define ELF_PUT_32(ADDR,VAL,BIG) (BIG ? _do_putb32(VAL,(bfd_byte *)ADDR) : _do_putl32(VAL,(bfd_byte *)ADDR))
	ELF_PUT_32(data,x1,isec->flags & SEC_IS_BIG_ENDIAN);
	if (reloc_type == bfd_reloc_type_opt_callx)
		ELF_PUT_32(((char *)data)+4,x2,isec->flags & SEC_IS_BIG_ENDIAN);
    }
    /* Update the relocation directive in case of relinkable output */
    if (obfd)
	    reloc->address += isec->output_offset;

    return retval;
}
