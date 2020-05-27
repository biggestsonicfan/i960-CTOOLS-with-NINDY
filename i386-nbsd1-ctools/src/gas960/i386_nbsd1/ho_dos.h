/* h_dos.h  Generic host-specific header file.
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

/* $Id: ho_dos.h,v 1.4 1993/11/03 21:23:45 dramos Exp $ */

#ifndef hdos
#define hdos
#endif

#include "gnudos.h"

#define M_HDOS

#include <memory.h>
#define bcmp(b1,b2,len)         memcmp(b1,b2,len)
#define bcopy(src,dst,len)      memcpy(dst,src,len)
#define bzero(s,n)              memset(s,0,n)
 
#include <string.h>
#define index(s,c)              strchr(s,c)
#define rindex(s,c)             strrchr(s,c)

#include <malloc.h>

/* end of h_dos.h */
