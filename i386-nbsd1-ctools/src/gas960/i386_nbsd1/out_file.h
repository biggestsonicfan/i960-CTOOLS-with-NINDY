/* out_file.h
   Copyright (C) 1989, 1990, 1991 Free Software Foundation, Inc.

This file is part of GAS.

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

/* $Id: out_file.h,v 1.5 1993/09/28 17:01:17 peters Exp $ */

#ifdef __STDC__

void output_file_append(char *where, long length, char *filename);
void output_file_close(char *filename);
void output_file_create(char *name);
void output_file_setname(void);
void output_file_checkname(void);

#else /* __STDC__ */

void output_file_append();
void output_file_close();
void output_file_create();
void output_file_setname();
void output_file_checkname();

#endif /* __STDC__ */


/* end of output-file.h */
