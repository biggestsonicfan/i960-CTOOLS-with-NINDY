/* ho-gcc960.h  Host-specific header file for self-hosting with gcc960.
   Copyright (C) 1987, 1991 Free Software Foundation, Inc.

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

/* $Id: ho-gcc.h,v 1.3 1994/05/03 21:26:19 peters Exp $ */

#ifndef M_GCC960

#ifndef __GNUC__
/* Using ic960 */
#define alloca __builtin_alloca
#endif

#define index(s,c)              strchr(s,c)
#define rindex(s,c)             strrchr(s,c)

#include <errno.h>
#include <std.h>

#define M_GCC960 1
#endif
/* end of ho-gcc960.h */
