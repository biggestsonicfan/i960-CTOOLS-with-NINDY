
/*(c**************************************************************************** *
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
 ***************************************************************************c)*/

#include <stdio.h>
#include "bfd.h"
#include "coff.h"

#ifdef MWC
#define FOPEN_RDONLY "rb"
#define FOPEN_WRONLY_TRUNC "wb"
#define FOPEN_WRONLY_TRUNC_ASC "w"
#define FOPEN_RDWR "r+b"
#define FOPEN_RDWR_TRUNC "w+b"
#else
#define FOPEN_RDONLY "r"
#define FOPEN_WRONLY_TRUNC "w"
#define FOPEN_WRONLY_TRUNC_ASC "w"
#define FOPEN_RDWR "r+"
#define FOPEN_RDWR_TRUNC "w+"

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#define ROMNAME         "rom960"

#define MODE16	1	/* in ihex command, user asked for extended segment
			(8086) mode */
#define MODE32	2	/* in ihex command, user asked for extended address
			mode (the default) */


/* Pulled out of bout.h */
struct exec {
        /* Standard stuff */
        unsigned long a_magic;  /* Identifies this as a b.out file      */
        unsigned long a_text;   /* Length of text                       */
        unsigned long a_data;   /* Length of data                       */
        unsigned long a_bss;    /* Length of runtime uninitialized data area */
        unsigned long a_syms;   /* Length of symbol table               */
        unsigned long a_entry;  /* Runtime start address                */
        unsigned long a_trsize; /* Length of text relocation info       */
        unsigned long a_drsize; /* Length of data relocation info       */

        /* Added for i960 */
        unsigned long a_tload;  /* Text runtime load address            */
        unsigned long a_dload;  /* Data runtime load address            */
        unsigned char a_talign; /* Alignment of text segment            */
        unsigned char a_dalign; /* Alignment of data segment            */
        unsigned char a_balign; /* Alignment of bss segment             */
        unsigned char a_ccinfo; /* See below                            */
};

struct name_list {
    char *name;
    struct name_list *next_name;
};
