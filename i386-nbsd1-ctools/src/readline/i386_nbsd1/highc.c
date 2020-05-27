/* highc.c -- routines to support a HIGHC port of various GO32
   functions necessary to make readline functional under DOS. */

/* Copyright 1987, 1989, 1991, 1992 Free Software Foundation, Inc.

   This file is part of the GNU Readline Library, a library for
   reading lines of text with interactive input and history editing.

   The GNU Readline Library is free software; you can redistribute it
   and/or modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 1, or
   (at your option) any later version.

   The GNU Readline Library is distributed in the hope that it will be
   useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   The GNU General Public License is often shipped with GNU software, and
   is generally kept in a file called COPYING or LICENSE.  If you do not
   have a copy of the license, write to the Free Software Foundation,
   675 Mass Ave, Cambridge, MA 02139, USA. */

#include <dos.h>

/* 
 * Normal CTRL-C handling in gdb won't work on DOS; at least, with
 * Metaware/Phar Lap it is observed to crash the system when quit()
 * is called directly from the CTRL-C handler.  So instead, the
 * CTRL-C handler just sets quit_flag (CTRL-BREAK handler just sets
 * ctrlbrk_flag) and then we will poll these flags from various
 * strategic places in the gdb code.
 */

#undef QUIT
#define QUIT() {  \
                         extern int quit_flag, ctrlbrk_flag, kbhit(); \
                         kbhit();                                     \
                         if ( quit_flag || ctrlbrk_flag )             \
                             quit();                                  \
               }  \


static int pc_screen_rows, pc_screen_cols;

static void
_rl_highc_init(void)
{
    char   *cp;
    struct ptr { int off; short seg; };

    /*
     * Determine screen geometry.  Use env vars if defined,
     * else query BIOS, else use hardwired values.
     */
    pc_screen_rows = pc_screen_cols = 0;
    if ((cp = getenv ("COLUMNS")) != NULL)
        pc_screen_cols = atoi(cp);
    if ((cp = getenv ("LINES")) != NULL)
        pc_screen_rows = atoi(cp);
    if (! (pc_screen_cols && pc_screen_rows))
    {
        char         buf[128];
        void   _Far  *ptr = buf;
        union  REGS  r;
        struct SREGS segs;

        r.h.ah = 0x1b;
        r.x.bx = 0;
        segread(&segs);
        segs.es = _FP_SEG(ptr);
        r.x.di  = _FP_OFF(ptr);
        int86x(0x10, &r, &r, &segs);
        if (r.h.al == 0x1b)
        {
            /* This BIOS query is supported. */

            if (! pc_screen_cols)
                pc_screen_cols = *((short *) &buf[0x5]);
            if (! pc_screen_rows)
                pc_screen_rows = buf[0x22];
        }
        else
        {
            pc_screen_cols = 80;
            pc_screen_rows = 25;
        }
    }
}



static int
getkey(void)
{
    union REGS r;

    r.h.ah = 0x10;
    (void) int86(0x16, &r, &r);
    if (r.h.al == 0 || r.h.al == 0xE0)
    {
        /* Special char -- look for the arrow keys */

        if (r.h.ah == 0x48)
        {
            r.h.al = '\033';
            rl_stuff_char('[');
            rl_stuff_char('A');
        }
        else if (r.h.ah == 0x4B)
        {
            r.h.al = '\033';
            rl_stuff_char('[');
            rl_stuff_char('D');
        }
        else if (r.h.ah == 0x4D)
        {
            r.h.al = '\033';
            rl_stuff_char('[');
            rl_stuff_char('C');
        }
        else if (r.h.ah == 0x50)
        {
            r.h.al = '\033';
            rl_stuff_char('[');
            rl_stuff_char('B');
        }
    }
    return (r.h.al);
}



/*
 * Note that this routine can be called directly by GDB and thus, must be
 * paranoid about prior initialization.
 */
int
ScreenCols(void)
{
    if (! pc_screen_cols)
        _rl_highc_init();
    return (pc_screen_cols);
}



/*
 * Note that this routine can be called directly by GDB and thus, must be
 * paranoid about prior initialization.
 */
int
ScreenRows(void)
{
    if (! pc_screen_rows)
        _rl_highc_init();
    return (pc_screen_rows);
}



void
ScreenClear(void)
{
    union REGS r;

    r.h.ah = 0x6;
    r.h.al = 0;
    r.h.bh = 0x7;         /* Magic attribute, seems to matter */
    r.h.ch = r.h.cl = 0;
    r.h.dh = pc_screen_rows;
    r.h.dl = pc_screen_cols;
    (void) int86(0x10, &r, &r);
}



void 
ScreenGetCursor(int *row, int *col)
{
    union REGS r;
    
    r.h.al = 0x0;
    r.h.ah = 0x3;               /* get cursor posn   */
    r.x.bx = 0x0;               /* video page number */
    (void) int86(0x10, &r, &r); /* Return code not meaningful .... */
    *row = r.h.dh;
    *col = r.h.dl;
}



void 
ScreenSetCursor(int row, int col)
{
    union REGS r;
    
    r.h.al = 0x0;
    r.h.ah = 0x2;               /* set cursor posn   */
    r.x.bx = 0x0;               /* video page number */
    r.h.dh = row;
    r.h.dl = col;
    (void) int86(0x10, &r, &r); /* Return code not meaningful .... */
}
