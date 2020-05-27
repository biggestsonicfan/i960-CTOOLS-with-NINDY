/* Host support for i386/DOS.  (Originally copied from xm-i386v.h)
   Copyright 1986, 1987, 1989, 1992 Free Software Foundation, Inc.
   Changes for 80386 by Pace Willisson (pace@prep.ai.mit.edu), July 1988.

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Make sure files can be opened in binary mode if desired. */
#include "fopen-bin.h"

#define HOST_BYTE_ORDER LITTLE_ENDIAN

/* 
On some machines, gdb crashes when it's starting up while calling the
vendor's termio tgetent() routine.  It always works when run under
itself (actually, under 3.2, it's not an infinitely recursive bug.)
After some poking around, it appears that depending on the environment
size, or whether you're running YP, or the phase of the moon or something,
the stack is not always long-aligned when main() is called, and tgetent()
takes strong offense at that.  On some machines this bug never appears, but
on those where it does, it occurs quite reliably.  */
#define ALIGN_STACK_ON_STARTUP

/* define USG if you are using sys5 /usr/include's */
#define USG

/* #define HAVE_TERMIO */

/* This is the amount to subtract from u.u_ar0
   to get the offset in the core file of the register values.  */

#define KERNEL_U_ADDR 0xe0000000

/* The default separator for lists of filenames, e.g. PATH, is ':' which
   won't work on DOS because of the dreaded C:\path\name syntax. 
   Used in source.c */
#define DIRNAME_SEPARATOR ';'

/* Fix the dreaded backslashes in a generalized way. */
#define isslash(a) (((a) == '/') || ((a) == '\\'))

/* Normal ".gdbinit" is too long for DOS filename extension */
#define	GDBINIT_FILENAME	"init.gdb"

/* Normal CTRL-C handling in gdb won't work on DOS; at least, with Metaware/
   Phar Lap it is observed to crash the system when quit() is called directly
   from the CTRL-C handler.  So instead, the CTRL-C handler just sets 
   quit_flag (CTRL-BREAK handler just sets ctrlbrk_flag) and then we will
   poll these flags from various strategic places in the gdb code. */
#undef QUIT
#define QUIT { kbhit(); if ( quit_flag || ctrlbrk_flag ) quit(); }

extern int ctrlbrk_flag;
#ifdef __HIGHC__
#include <dos.h>
/* Can't use signal() to trap CTRL-BREAK as SIGINT.  See comment at
   main.c:init_signals.  These functions are defined in toolib.  */
void  install_ctrlbrk_handler();
void  de_install_ctrlbrk_handler();
#endif
