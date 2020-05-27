/* reloc.h -- Header file for relocation information.
   Copyright (C) 1989,1990 Free Software Foundation, Inc.

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

/* $Id: reloc.h,v 1.5 1994/08/11 17:10:17 peters Exp $ */

/*
 * The following enum and struct were borrowed from
 * sunOS  /usr/include/sun4/a.out.h  and extended to handle
 * other machines.
 */

enum reloc_type
{
    RELOC_8,        RELOC_16,        RELOC_32,       RELOC_DISP8,
    RELOC_DISP16,   RELOC_DISP32,    RELOC_WDISP30,  RELOC_WDISP22,
    RELOC_HI22,     RELOC_22,        RELOC_13,       RELOC_LO10,
    RELOC_SFA_BASE, RELOC_SFA_OFF13, RELOC_BASE10,   RELOC_BASE13,
    RELOC_BASE22,   RELOC_PC10,      RELOC_PC22,     RELOC_JMP_TBL,
    RELOC_SEGOFF16, RELOC_GLOB_DAT,  RELOC_JMP_SLOT, RELOC_RELATIVE,

/* 29K relocation types */
    RELOC_JUMPTARG, RELOC_CONST,     RELOC_CONSTH,

    NO_RELOC
};

struct reloc_info_generic
{
    unsigned long r_address;
    unsigned int r_index;
    unsigned r_extern   : 1;
    unsigned r_pcrel:1;
    unsigned r_length:2;	/* 0=>byte 1=>short 2=>long 3=>8byte */
    unsigned r_bsr:1;		/* NS32K */
    unsigned r_disp:1;		/* NS32k */
    unsigned r_callj:1;		/* i960 */
    unsigned r_calljx:1;	/* i960 */
    enum reloc_type r_type;
    long r_addend;
};

/* end of reloc.h */
