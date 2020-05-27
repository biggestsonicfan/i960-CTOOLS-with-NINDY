/* Copyright (C) 1990, 1991 Free Software Foundation, Inc.
 * 
 * This file is part of BFD, the Binary File Diddler.
 * 
 * BFD is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 * 
 * BFD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with BFD; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/* All the host-dependent include files boiled into one place
 */
   

#ifndef _SYSDEP_H
#define _SYSDEP_H

/* The including makefile must define HOST_SYS to be one of these.
 * Each combination of Machine and OS (and maybe OS Version) must
 * have a different number.
 */
#define SUN4_SYS	2
#define POSIX_SYS	3
#define AIX_SYS		4
#define VAX_ULTRIX_SYS	5
#define i386_SYSV_SYS	6
#define SUN3_SYS	7
#define UNKNOWN_SYS	8
#define DEC3100_SYS	9
#define HP9000_SYS	10
#define APOLLO400_SYS	11
#define DOS_SYS		12
#define MACAUX_SYS	13
#define NCR_SYSV_SYS	14
#define SOL_SUN4_SYS	15
#define GCC960_SYS	16
#define I386_NBSD1_SYS  17

#include "ansidecl.h"

#if __STDC__
#	define PROTO(type, name, arglist) type name arglist
#else
#	define PROTO(type, name, arglist) type name ()
#	define NO_STDARG
#endif

#ifndef HOST_SYS
	Hey HOST_SYS_has_not_been_defined!
#endif

#if HOST_SYS==SOL-SUN4_SYS
#	define HOST_IS_SUN4 1
#	include <sys/h-usg.h>
#endif

#if HOST_SYS==SUN4_SYS
#	define HOST_IS_SUN4 1
#	include <sys/h-sun4.h>
#endif

#if HOST_SYS==POSIX_SYS
#	define HOST_IS_POSIX 1
#endif 

#if HOST_SYS==AIX_SYS
#	define HOST_IS_AIX 1
#	include <sys/h-aix.h>
#endif

#if HOST_SYS==VAX_ULTRIX_SYS
#	define HOST_IS_VAX_ULTRIX 1
#	include <sys/h-generic.h>
#endif

#if HOST_SYS==i386_SYSV_SYS
#	define HOST_IS_i386_SYSV 1
#	define USG 
#	include <sys/h-usg.h>
#endif

#if HOST_SYS==NCR_SYSV_SYS
#	define HOST_IS_NCR_SYSV 1
#	define USG 
#	include <sys/h-usg.h>
#endif

#if HOST_SYS==SUN3_SYS
#	define HOST_IS_SUN3 1
#	include <sys/h-sun3.h>
#endif

#if HOST_SYS==DEC3100_SYS
#	define HOST_IS_DEC3100 1
#	include <sys/h-generic.h>
#endif

#if HOST_SYS==HP9000_SYS
#	define HOST_IS_HP9000 1
#	define USG 
#	include <sys/h-usg.h>
#endif

#if HOST_SYS==APOLLO400_SYS
#	define HOST_IS_APOLLO400 1
#	include <sys/h-ap400.h>
#endif
 
#if HOST_SYS==DOS_SYS
#	define HOST_IS_DOS 1
#	include <sys/h_dos.h>
#endif

#if HOST_SYS==MACAUX_SYS
#	define HOST_IS_MACAUX 1
#	define USG 
#	include <sys/h-macaux.h>
#endif

#if HOST_SYS==GCC960_SYS
#	define HOST_IS_GCC960 1
#	include <sys/h-gcc960.h>
#endif

#if HOST_SYS==I386_NBSD1_SYS
#	define HOST_IS_I386_NBSD1 1
#	include <sys/h-usg.h>
#endif

#endif 	/* ifndef _SYSDEP_H */
