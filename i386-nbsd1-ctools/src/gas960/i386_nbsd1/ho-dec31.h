/* ho-dec3100.h  Host-specific header file for decstation 3100.
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

/* $Id: ho-dec31.h,v 1.4 1993/09/28 16:59:30 peters Exp $ */

#define M_DEC3100 1

#ifndef __STDC__
#define NO_STDARG
extern char *malloc();
extern int free();
#endif

#include <string.h>

/* end of ho-dec3100.h */
