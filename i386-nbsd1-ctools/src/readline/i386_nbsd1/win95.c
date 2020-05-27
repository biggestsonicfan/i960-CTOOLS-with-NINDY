/* win95.c -- routines to support a MSVC port of various GO32
   functions necessary to make readline functional under Win95. */

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

#include <windows.h>

static int    pc_screen_rows, pc_screen_cols, pendingEvent;
static HANDLE conin, conout;
static DWORD  old_con_mode;

typedef struct _dir
{
    int             first;
    HANDLE          findHandle;
    WIN32_FIND_DATA fd;
    dirent          dent;
} _DIR;


static void
getWindowSize(void)
{
    char                       *cp;
    CONSOLE_SCREEN_BUFFER_INFO info;

    conin  = GetStdHandle(STD_INPUT_HANDLE);
    conout = GetStdHandle(STD_OUTPUT_HANDLE);

    /*
     * Determine screen geometry.  Use env vars if defined,
     * else query Windows, else use hardwired values.
     */
    pc_screen_rows = pc_screen_cols = 0;
    if ((cp = getenv ("COLUMNS")) != NULL)
        pc_screen_cols = atoi(cp);
    if ((cp = getenv ("LINES")) != NULL)
        pc_screen_rows = atoi(cp);
    if (! (pc_screen_cols && pc_screen_rows))
    {
        if (GetConsoleScreenBufferInfo(conout, &info))
        {
            pc_screen_rows = info.dwMaximumWindowSize.Y;
            pc_screen_cols = info.dwMaximumWindowSize.X;
        }
        else
        {
            pc_screen_cols = 80;
            pc_screen_rows = 25;
        }
    }
}



static void
_rl_win95_init(void)
{
    pendingEvent = 0;
    getWindowSize();
    GetConsoleMode(conin, &old_con_mode);
    SetConsoleMode(conin, ENABLE_WINDOW_INPUT|ENABLE_MOUSE_INPUT);
}



static void
_rl_win95_cleanup(void)
{
    FlushConsoleInputBuffer(conin);
    SetConsoleMode(conin, old_con_mode);
}



static int
getkey(void)
{
    static INPUT_RECORD ir;
    KEY_EVENT_RECORD    *ke;
    DWORD               nr;

    while (1)
    {
        ke = &ir.Event.KeyEvent;
        if (pendingEvent)
        {
            ke->wRepeatCount--;
            if (ke->wRepeatCount <= 0)
                pendingEvent = 0;
        }
        else if (! ReadConsoleInput(conin, &ir, 1, &nr))
            continue;

        switch (ir.EventType)
        {
            case KEY_EVENT:
                if (ke->bKeyDown)
                {
                    if (ke->wRepeatCount > 1)
                    {
                        ke->wRepeatCount--;
                        pendingEvent = 1;
                    }
                    if (ke->wVirtualKeyCode == VK_LEFT)
                    {
                        rl_stuff_char('[');
                        rl_stuff_char('D');
                        return ('\033');
                    }
                    else if (ke->wVirtualKeyCode == VK_RIGHT)
                    {
                        rl_stuff_char('[');
                        rl_stuff_char('C');
                        return ('\033');
                    }
                    else if (ke->wVirtualKeyCode == VK_UP)
                    {
                        rl_stuff_char('[');
                        rl_stuff_char('A');
                        return ('\033');
                    }
                    else if (ke->wVirtualKeyCode == VK_DOWN)
                    {
                        rl_stuff_char('[');
                        rl_stuff_char('B');
                        return ('\033');
                    }
                    else if (ke->uChar.AsciiChar)
                        return (ke->uChar.AsciiChar);

                    pendingEvent = 0;  /* Not a key we care about */
                }
                break;
            case WINDOW_BUFFER_SIZE_EVENT:
                (void) getWindowSize();
                (void) rl_refresh_line();  /* redraw current line. */
                break;
        }
    }
}



/*
 * Note that this routine can be called directly by GDB and thus, must be
 * paranoid about prior initialization.
 */
int
ScreenCols(void)
{
    if (! pc_screen_cols)
        getWindowSize();
    return (pc_screen_cols);
}



/*
 * Note that this routine can be called directly GDB and thus, must be
 * paranoid about prior initialization.
 */
int
ScreenRows(void)
{
    if (! pc_screen_rows)
        getWindowSize();
    return (pc_screen_rows);
}



void
ScreenClear(void)
{
    COORD cur;
    DWORD dummy;

    cur.X = cur.Y = 0;
    (void) FillConsoleOutputCharacter(conout, 
                                      ' ', 
                                      pc_screen_cols * pc_screen_rows,
                                      cur,
                                      &dummy);
}



void 
ScreenGetCursor(int *row, int *col)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    
    (void) GetConsoleScreenBufferInfo(conout, &info);
    *row = info.dwCursorPosition.Y;
    *col = info.dwCursorPosition.X;
}



void 
ScreenSetCursor(int row, int col)
{
    COORD coord;

    coord.X = col;
    coord.Y = row;
    (void) SetConsoleCursorPosition(conout, coord);
}



DIR *
opendir(char *dirname)
{
    DIR  *dirp;
    char tmp[MAX_PATH], *cp;

    if (! dirname[0])
        return (NULL);

    if (dirname[0] == '.' && dirname[1] == '\0')
        strcpy(tmp, "*.*");
    else
    {
        strcpy(tmp, dirname);
        cp = dirname + strlen(dirname) - 1;
        if (*cp == '/' || *cp == '\\')
            strcat(tmp, "*.*");
        else
            strcat(tmp, "/*.*");
    }
    dirp = (DIR *) xmalloc(sizeof(DIR));
    if ((dirp->findHandle = FindFirstFile(tmp, &dirp->fd))
                                                    == INVALID_HANDLE_VALUE)
    {
        free(dirp);
        return (NULL);
    }
    dirp->first = 1;
    return (dirp);
}



int
closedir(DIR *dirp)
{
    FindClose(dirp->findHandle);
    free(dirp);
    return (0);
}



dirent *
readdir(DIR *dirp)
{
    if (dirp->first)
        dirp->first = 0;
    else if (! FindNextFile(dirp->findHandle, &dirp->fd))
        return (NULL);
    dirp->dent.d_name = dirp->fd.cFileName;
    return (&dirp->dent);
}
