/* Copyright (C) 1990, 1991 Free Software Foundation, Inc.

This file is part of BFD, the Binary File Diddler.

BFD is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

BFD is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with BFD; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */



/* This -*- C -*- source file will someday be machine-generated */

/*** Defines the target vector through which operations dispatch */
#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"

extern bfd_target b_out_vec_little_host;
extern bfd_target b_out_vec_big_host;
extern bfd_target icoff_little_vec;
extern bfd_target icoff_big_vec;
#if !defined(NO_BIG_ENDIAN_MODS)
extern bfd_target icoff_little_big_vec; /* big-endian memory */
extern bfd_target icoff_big_big_vec;    /* big-endian memory */
#endif /* NO_BIG_ENDIAN_MODS */
extern bfd_target elf_vec_little_host; /* big-endian memory */
extern bfd_target elf_vec_big_host;    /* big-endian memory */

bfd_target *target_vector[] = {
    &icoff_little_vec,
    &icoff_big_vec,
    &elf_vec_little_host,
    &elf_vec_big_host,
#if !defined(NO_BIG_ENDIAN_MODS)
    &icoff_little_big_vec,  /* big-endian memory */
    &icoff_big_big_vec,     /* big-endian memory */
#endif /* NO_BIG_ENDIAN_MODS */
    &b_out_vec_little_host,
    &b_out_vec_big_host,
    NULL		/* end of list marker */
};
