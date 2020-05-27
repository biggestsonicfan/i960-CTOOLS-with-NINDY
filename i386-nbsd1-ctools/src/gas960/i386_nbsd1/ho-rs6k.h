/* ho-rs6000.h  Rs6000 host-specific header file.
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

#define M_RS6000 1

/* FIXME-SOMEDAY: For error messages, and other uses of varargs;
 * the following forces the old-style varargs.  When and if we
 * ever go to full ANSI compatibility, remove the next line. 
 */
#define NO_STDARG

#define bcopy(from,to,n) memcpy((to),(from),(n))
#define bzero(s,n) memset((s),0,(n))
#define setbuffer(stream, buf, size) setvbuf((stream), (buf), _IOLBF, (size))

#include <stdlib.h>
/* end of ho-sysv.h */

/* end of ho-rs6000.h */
