/* Support for GDB maintenance commands.
   Copyright (C) 1992, 1993, 1994 Free Software Foundation, Inc.
   Written by Fred Fish at Cygnus Support.

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


#include "defs.h"
#if defined(WIN95)
#include "gnudos.h"
#endif

#if MAINTENANCE_CMDS	/* Entire file goes away if not including maint cmds */

#include <signal.h>
#include "command.h"
#include "gdbcmd.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "demangle.h"
#include "gdbcore.h"
#include "expression.h" /* For language.h */
#include "language.h"

static void
maintenance_command PARAMS ((char *, int));

static void
maintenance_dump_me PARAMS ((char *, int));

static void
maintenance_demangle PARAMS ((char *, int));

/*

LOCAL FUNCTION

	maintenance_command -- access the maintenance subcommands

SYNOPSIS

	void maintenance_command (char *args, int from_tty)

DESCRIPTION

*/

static void
maintenance_command (args, from_tty)
     char *args;
     int from_tty;
{
  printf_unfiltered ("\"maintenance\" must be followed by the name of a maintenance command.\n");
  help_list (maintenancelist, "maintenance ", -1, gdb_stdout);
}


/* ARGSUSED */
static void
maintenance_dump_me (args, from_tty)
     char *args;
     int from_tty;
{
  if (query ("Should GDB dump core? "))
    {
      signal (SIGQUIT, SIG_DFL);
      kill (getpid (), SIGQUIT);
    }
}

/*  Someday we should allow demangling for things other than just
    explicit strings.  For example, we might want to be able to
    specify the address of a string in either GDB's process space
    or the debuggee's process space, and have gdb fetch and demangle
    that string.  If we have a char* pointer "ptr" that points to
    a string, we might want to be able to given just the name and
    have GDB demangle and print what it points to, etc.  (FIXME) */

static void
maintenance_demangle (args, from_tty)
     char *args;
     int from_tty;
{
  char *demangled;

  if (args == NULL || *args == '\0')
    {
      printf_unfiltered ("\"maintenance demangle\" takes an argument to demangle.\n");
    }
  else
    {
      demangled = cplus_demangle (args, DMGL_ANSI | DMGL_PARAMS);
      if (demangled != NULL)
	{
	  printf_unfiltered ("%s\n", demangled);
	  free (demangled);
	}
      else
	{
	  printf_unfiltered ("Can't demangle \"%s\"\n", args);
	}
    }
}

/* The "maintenance info" command is defined as a prefix, with allow_unknown 0.
   Therefore, its own definition is called only for "maintenance info" with
   no args.  */

/* ARGSUSED */
static void
maintenance_info_command (arg, from_tty)
     char *arg;
     int from_tty;
{
  printf_unfiltered ("\"maintenance info\" must be followed by the name of an info command.\n");
  help_list (maintenanceinfolist, "maintenance info ", -1, gdb_stdout);
}

static void
print_section_table (abfd, asect, ignore)
     bfd *abfd;
     asection *asect;
     PTR ignore;
{
  flagword flags;

  flags = bfd_get_section_flags (abfd, asect);

  /* FIXME-32x64: Need print_address_numeric with field width.  */
  printf_filtered ("    %s",
		   local_hex_string_custom
		     ((unsigned long) bfd_section_vma (abfd, asect), "08l"));
  printf_filtered ("->%s",
		   local_hex_string_custom
		     ((unsigned long) (bfd_section_vma (abfd, asect)
				       + bfd_section_size (abfd, asect)),
		      "08l"));
  printf_filtered (" at %s",
		   local_hex_string_custom
		     ((unsigned long) asect->filepos, "08l"));
  printf_filtered (": %s", bfd_section_name (abfd, asect));

  if (flags & SEC_ALLOC)
    printf_filtered (" ALLOC");
  if (flags & SEC_LOAD)
    printf_filtered (" LOAD");
  if (flags & SEC_RELOC)
    printf_filtered (" RELOC");
  if (flags & SEC_READONLY)
    printf_filtered (" READONLY");
  if (flags & SEC_CODE)
    printf_filtered (" CODE");
  if (flags & SEC_DATA)
    printf_filtered (" DATA");
  if (flags & SEC_ROM)
    printf_filtered (" ROM");
  if (flags & SEC_CONSTRUCTOR)
    printf_filtered (" CONSTRUCTOR");
  if (flags & SEC_HAS_CONTENTS)
    printf_filtered (" HAS_CONTENTS");
  if (flags & SEC_NEVER_LOAD)
    printf_filtered (" NEVER_LOAD");
  if (flags & SEC_COFF_SHARED_LIBRARY)
    printf_filtered (" COFF_SHARED_LIBRARY");
  if (flags & SEC_IS_COMMON)
    printf_filtered (" IS_COMMON");

  printf_filtered ("\n");
}

/* ARGSUSED */
static void
maintenance_info_sections (arg, from_tty)
     char *arg;
     int from_tty;
{
  if (exec_bfd)
    {
      printf_filtered ("Exec file:\n");
      printf_filtered ("    `%s', ", bfd_get_filename(exec_bfd));
      wrap_here ("        ");
      printf_filtered ("file type %s.\n", bfd_get_target(exec_bfd));
      bfd_map_over_sections(exec_bfd, print_section_table, 0);
    }

  if (core_bfd)
    {
      printf_filtered ("Core file:\n");
      printf_filtered ("    `%s', ", bfd_get_filename(core_bfd));
      wrap_here ("        ");
      printf_filtered ("file type %s.\n", bfd_get_target(core_bfd));
      bfd_map_over_sections(core_bfd, print_section_table, 0);
    }
}

/* The "maintenance print" command is defined as a prefix, with allow_unknown
   0.  Therefore, its own definition is called only for "maintenance print"
   with no args.  */

/* ARGSUSED */
static void
maintenance_print_command (arg, from_tty)
     char *arg;
     int from_tty;
{
  printf_unfiltered ("\"maintenance print\" must be followed by the name of a print command.\n");
  help_list (maintenanceprintlist, "maintenance print ", -1, gdb_stdout);
}

#endif	/* MAINTENANCE_CMDS */

#if defined(IMSTG)
/* Print a sorted list of help text for every gdb960 command. */
struct helpnode
{
    char *name;
    char *doc;
} helplist[512];

struct helpnode *hp;

/* Comparator for qsort */
static int
helpcmp(p1, p2)
const void *p1, *p2;
{
    struct helpnode *mn1 = (struct helpnode *) p1;
    struct helpnode *mn2 = (struct helpnode *) p2;
    return strcmp(mn1->name, mn2->name);
}

static void
build_helplist(cp, prefix)
struct cmd_list_element *cp;
char *prefix;
{
    for ( ; cp; cp = cp->next )
    {
	if ( cp->abbrev_flag )
	    continue;
	if ( prefix )
	{
	    hp->name = (char *) malloc (strlen(prefix) + strlen(cp->name) + 1);
	    make_cleanup(free, hp->name);
	    strcpy(hp->name, prefix);
	    strcat(hp->name, cp->name);
	}
	else
	    hp->name = cp->name;
	hp->doc = cp->doc;
	++hp;
	if ( cp->prefixlist )
	    build_helplist(*cp->prefixlist, cp->prefixname);
    }
}

/* Build a list of every help command in the debugger.
   Don't include abbreviations in the list.
   Sort the list by command name. 
   Print the sorted list. */
static void
maintenance_print_help(dummy1, dummy2)
     char *dummy1;
     int dummy2;
{
    struct cmd_list_element *cp;
    struct cleanup *old_chain;
    char *outbuf;
    int outbuflen = 2048;
    int i;
    hp = &helplist[0];

    build_helplist(cmdlist, NULL);

    qsort(helplist, hp - helplist, sizeof(struct helpnode), helpcmp);

    outbuf = (char *) xmalloc(outbuflen);
    old_chain = make_cleanup(free, outbuf);

    for ( hp = &helplist[0]; *hp->name; ++hp )
    {
	/* Brain-damaged printf_filtered has a line-length restriction;
	   so we must use fputs_filtered instead. */
	if ( strlen(hp->name) + strlen(hp->doc) + 4 > outbuflen )
	{
	    do_cleanups(old_chain);
	    outbuflen *= 2;
	    outbuf = (char *) xmalloc(outbuflen);
	    old_chain = make_cleanup(free, outbuf);
	}
	sprintf(outbuf, "%s\n%s\n\n", hp->name, hp->doc);
	puts_filtered(outbuf);
    }
}
#endif /* IMSTG */

void
_initialize_maint_cmds ()
{
#if MAINTENANCE_CMDS	/* Entire file goes away if not including maint cmds */
  add_prefix_cmd ("maintenance", class_maintenance, maintenance_command,
		  "Commands for use by GDB maintainers.\n\
Includes commands to dump specific internal GDB structures in\n\
a human readable form, to cause GDB to deliberately dump core,\n\
to test internal functions such as the C++ demangler, etc.",
		  &maintenancelist, "maintenance ", 0,
		  &cmdlist);

  add_com_alias ("mt", "maintenance", class_maintenance, 1);

  add_prefix_cmd ("info", class_maintenance, maintenance_info_command,
		  "Show internal info about the program being debugged.",
		  &maintenanceinfolist, "maintenance info ", 0,
		  &maintenancelist);

  add_cmd ("sections", class_maintenance, maintenance_info_sections,
	   "List the BFD sections of the exec and core files.",
	   &maintenanceinfolist);

  add_prefix_cmd ("print", class_maintenance, maintenance_print_command,
		  "Maintenance command for printing GDB internal state.",
		  &maintenanceprintlist, "maintenance print ", 0,
		  &maintenancelist);

  add_cmd ("dump-me", class_maintenance, maintenance_dump_me,
	   "Get fatal error; make debugger dump its core.\n\
GDB sets it's handling of SIGQUIT back to SIG_DFL and then sends\n\
itself a SIGQUIT signal.",
	   &maintenancelist);

  add_cmd ("demangle", class_maintenance, maintenance_demangle,
	   "Demangle a C++ mangled name.\n\
Call internal GDB demangler routine to demangle a C++ link name\n\
and prints the result.",
	   &maintenancelist);

#if defined(IMSTG)
  add_cmd ("help", class_maintenance, maintenance_print_help,
	   "Print a sorted list of help text for all GDB commands.\n\
Abbreviations are not printed, just the full command names.",
	   &maintenanceprintlist);
#endif

  add_cmd ("type", class_maintenance, maintenance_print_type,
	   "Print a type chain for a given symbol.\n\
For each node in a type chain, print the raw data for each member of\n\
the type structure, and the interpretation of the data.",
	   &maintenanceprintlist);

  add_cmd ("symbols", class_maintenance, maintenance_print_symbols,
	   "Print dump of current symbol definitions.\n\
Entries in the full symbol table are dumped to file OUTFILE.\n\
If a SOURCE file is specified, dump only that file's symbols.",
	   &maintenanceprintlist);

  add_cmd ("msymbols", class_maintenance, maintenance_print_msymbols,
	   "Print dump of current minimal symbol definitions.\n\
Entries in the minimal symbol table are dumped to file OUTFILE.\n\
If a SOURCE file is specified, dump only that file's minimal symbols.",
	   &maintenanceprintlist);

  add_cmd ("psymbols", class_maintenance, maintenance_print_psymbols,
	   "Print dump of current partial symbol definitions.\n\
Entries in the partial symbol table are dumped to file OUTFILE.\n\
If a SOURCE file is specified, dump only that file's partial symbols.",
	   &maintenanceprintlist);

  add_cmd ("objfiles", class_maintenance, maintenance_print_objfiles,
	   "Print dump of current object file definitions.",
	   &maintenanceprintlist);

  add_cmd ("check-symtabs", class_maintenance, maintenance_check_symtabs,
	   "Check consistency of psymtabs and symtabs.",
	   &maintenancelist);
#endif	/* MAINTENANCE_CMDS */
}
