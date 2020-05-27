/*
 * Copyright (C) 1991 Free Software Foundation, Inc.
 *
 * This file is part of GLD, the Gnu Linker.
 *
 * GLD is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * GLD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GLD; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* $Id: ldfile.c,v 1.59 1995/07/21 17:26:34 kevins Exp $ */

#include "sysdep.h"
#include "bfd.h"
#include "ldmain.h"
#include "ldmisc.h"
#include "ldlang.h"
#include "ldfile.h"
#include "ldemul.h"

/* EXPORT */

char *ldfile_input_filename;
CONST char * ldfile_output_machine_name ="";
unsigned long ldfile_output_machine,ldfile_real_output_machine;
enum bfd_architecture ldfile_output_architecture;
char *lib_subdir = "";
#ifdef PROCEDURE_PLACEMENT
int is_really_i960_cf = 0;
#endif
#if !defined(NO_BIG_ENDIAN_MODS)
int target_byte_order_is_big_endian = 0; /* non-zero => big-endian target */
int host_byte_order_is_big_endian = 0; /* non-zero => big-endian host */
#endif /* NO_BIG_ENDIAN_MODS */

/* IMPORT */

extern boolean option_v;
extern boolean had_script;
extern HOW_INVOKED invocation;

/* LOCAL */

typedef struct search_dirs_struct {
    char *name;
    struct search_dirs_struct *next;
} search_dirs_type;

static search_dirs_type *search_head;
static search_dirs_type **search_tail_ptr = &search_head;

typedef struct search_arch_struct {
    char *name;
    struct search_arch_struct *next;
} search_arch_type;

/*
   
   Note:
   
   This list always has either exactly one or two elements.  (see ldfile_add_arch()).
   
   So, why is it a dynamic linked list and not an array?
   
   */

static search_arch_type *search_arch_head;
static search_arch_type **search_arch_tail_ptr = &search_arch_head;


static
void
ldfile_add_library_path_worker(name,warn)
    char *name;
    int warn;
{
    search_dirs_type *new;
#if HOST_SYS != GCC960_SYS
    struct stat st;
    
    if (stat(name,&st) != 0 || !(S_ISDIR(st.st_mode))) {
	if (warn)
		info("%P: warning: `%s' is not a directory.\n",name);
	return;
    }
#endif
    new = (search_dirs_type *)ldmalloc((bfd_size_type)(sizeof(search_dirs_type)));
    new->name = buystring(name);
    new->next = (search_dirs_type*)NULL;
    *search_tail_ptr = new;
    search_tail_ptr = &new->next;
}

#include "ld.h"

void
ldfile_add_library_path(name,warn)
    char *name;
    int warn;
{
    if (invocation == as_gld960 && USE_OLD_SEARCH_RULES) {
	char buff[1024];
	
	sprintf(buff,"%s%s",name,lib_subdir);
	ldfile_add_library_path_worker(buff,0);
    }
    ldfile_add_library_path_worker(name,warn);
}

static bfd *
cached_bfd_openr(attempt,entry)
    CONST char *attempt;
    lang_input_statement_type  *entry;
{
    if (!entry->the_bfd)
	    entry->the_bfd = bfd_openr(attempt, entry->target);
    if (option_v == true ) {
	fprintf(stdout,"attempt to open %s %s\n", attempt,
		entry->the_bfd ? "succeeded" : "failed");
    }
    if (entry->the_bfd) {
	extern boolean inhibit_optimize_calls;
	extern boolean inhibit_optimize_branch;
	
	entry->the_bfd->flags |= inhibit_optimize_calls ? SUPP_CALLS_OPT : 0;
	entry->the_bfd->flags |= inhibit_optimize_branch ? SUPP_BAL_OPT : 0;
    }
    return entry->the_bfd;
}

static bfd *
try_a( path, arch, entry, lib, suffix, file_extension )
    char *path;
    char *arch;
    lang_input_statement_type *entry;
    char *lib;
    char *suffix,*file_extension;
{
    bfd  *desc;
    char *string;
    char buffer[1000];
    
    if (entry->add_suffixes || entry->prepend_lib) {
	sprintf(buffer, "%s/%s%s%s%s%s",
		path, lib, entry->filename, arch, suffix, file_extension);
    } else {
	sprintf(buffer,"%s/%s",path, entry->filename);
    }
    
    string = buystring(buffer);
    desc = cached_bfd_openr (string, entry);
    if (desc) {
	entry->filename = string;
	entry->search_dirs_flag = false;
	entry->the_bfd = desc;
    } else {
	free(string);
    }
    return desc;
}


static bfd *
open_a(arch, entry, lib, suffix, file_extension)
    char *arch;
    lang_input_statement_type *entry;
    char *lib;
    char *suffix,*file_extension;
{
    bfd *desc;
    search_dirs_type *search;
    
    if (entry->filename[0] == '/' ||  entry->filename[0] == '.'
#ifdef DOS
	|| entry->filename[0] == '\\' || entry->filename[1] == ':'
#endif
	) {
	
	/* Absolute path */
	char *string;
	
	string = buystring(entry->filename);
	desc = cached_bfd_openr (string, entry);
	if (desc) {
	    entry->filename = string;
	    entry->search_dirs_flag = false;
	    entry->the_bfd = desc;
	} else {
	    free(string);
	}
	return desc;
    }
    
    for (search = search_head; search; search = search->next) {
	if ( desc = try_a(search->name, arch, entry, lib, suffix, file_extension) ){
	    return desc;
	}
    }
    return (bfd *)NULL;
}

/* Open the input file specified by 'entry', and return a descriptor.
 * The open file is remembered; if the same file is opened twice in a row,
 * a new open is not actually done.
 */
void
ldfile_open_file (entry)
    lang_input_statement_type *entry;
{
    
    if (entry->superfile) {
	ldfile_open_file (entry->superfile);
    }
    
    if (entry->search_dirs_flag) {
	if (entry->add_suffixes || entry->prepend_lib) {

	    if (USE_NEW_SEARCH_RULES || entry->magic_syslib_and_startup) {
		search_arch_type *arch;
		static char *no_suffixes[]      = {         "", NULL};
		static char *qual_ordinary[]    = {         "", NULL};
		static char *qual_pid_new[]     = {"_p","p","", NULL};
		static char *qual_pid_both[]    = {"_p",    "", NULL};
		static char *qual_big[]         = {"_b","b","", NULL};
		static char *qual_pid_and_big[] = {"_e","e","", NULL};
		static char **quals;
		extern int target_byte_order_is_big_endian;
		extern boolean data_is_pi;

		if (entry->add_suffixes) {
		    if (target_byte_order_is_big_endian && data_is_pi)
			    quals = qual_pid_and_big;
		    else if (target_byte_order_is_big_endian)
			    quals = qual_big;
		    else if (data_is_pi) {
			quals = qual_pid_new;
			if (invocation == as_gld960) {
			    if (linker_search_rules == lang_old_and_new_search_rules)
				    quals = qual_pid_both;
			}
		    }
		    else
			    quals = qual_ordinary;
		}
		else
			quals = no_suffixes;
		for (arch = search_arch_head; arch; arch = arch->next) {
		    char **q;

		    for (q=quals;*q;q++) {
			if (open_a(arch->name,entry,"",*q,entry->file_extension))
				return;
			if (entry->prepend_lib && open_a(arch->name,entry,"lib",*q,entry->file_extension))
				return;
		    }
		}
	    }

	    if (USE_OLD_SEARCH_RULES && !entry->magic_syslib_and_startup) {
		/* What we have is the abbreviated form of the
		   library name. We need to build it into each of its
		   four possible forms, then look in each directory on
		   the search path for each name. */
		
		search_arch_type *arch;
		int entry_is_l_1_type = entry->local_sym_name == entry->filename;
		
		/* The search_arch list consists of two entries. The
		   first has a null name, the second has the name of the
		   target architecture (KB, KA, or CA). The search within
		   open_a goes through the entire search path for the name
		   it constructs from what it gets passed. Thus, the for
		   loop below results in four scans through the search
		   path, first for <entry->name>, then for 
		   lib<entry->name>.a, then for <entry->name><arch->name>,
		   then for lib<entry->name><arch->name>.a. */
		
		for (arch = search_arch_head; arch; arch = arch->next) {
		    extern boolean data_is_pi;

		    /* If this is a 'l_1' search type, then do not search for the
		       non-arch specific name. */
		    if (!strcmp(arch->name,"") && entry_is_l_1_type)
			    continue;
		    if (data_is_pi) {
			if (invocation == as_gld960) {
			    if (open_a(arch->name,entry,"","p",entry->file_extension))
				    return;
			    if (open_a(arch->name,entry,"lib","p",entry->file_extension))
				    return;
			}
			else {  /* Invocation was as lnk960. */
			    if (entry_is_l_1_type) {
				if (strcmp(entry->filename,"libh")) {
				    if (open_a(arch->name,entry,"","_p",entry->file_extension))
					    return;
				}
				else {
				    if (open_a(arch->name,entry,"","",entry->file_extension))
					    return;
				}
				continue;
			    }
			}
		    }
		    if (open_a(arch->name,entry,"","",entry->file_extension))
			    return;
		    /* If this is not a 'l_1' search type, then try to search for the
		       lib{name}.a form of the library. */
		    if (entry->local_sym_name != entry->filename &&
			open_a(arch->name,entry,"lib","",entry->file_extension))
			    return;
		}
	    }

	}
	else {
	    /* Just try to open the name we have in each directory
	       on the search path */
	    if (open_a("",entry,"","",""))
		    return;
	}
    }
    else
	    entry->the_bfd = cached_bfd_openr (entry->filename, entry);
	
    if (!entry->the_bfd) {
	static char *sd_help = "%F%P: %E %I\n\
%P: Use the -v option in the linker to show where the linker\n\
%P: looked for %I\n";
	info(entry->search_dirs_flag ? sd_help:"%F%P: %E %I\n", entry,entry);
    }
}
    
static FILE *
try_open_x(name, exten, outputname)
    char *name;
    char *exten, **outputname;
{
    FILE *result = (FILE *) 0;
    char buff[1000];
    if (exten && *exten)
	    sprintf(buff, "%s%s", name, exten);
    else
	    sprintf(buff,"%s",name);
    result = fopen(buff, "r");
    if (option_v)
	    fprintf(stdout,"%s%s\n", result ? "" : "can't find ", buff);
    *outputname = result ? buystring(buff) : NULL;
    return result;
}

static FILE *
try_open(name,target,outputname)
    char *name,**outputname;
    int target;
{
    FILE *f;
    
    return target ? ((f=try_open_x(name,".ld",outputname)) ? f : try_open_x(name,"",outputname)) :
	    ((f=try_open_x(name,"",outputname)) ? f : try_open_x(name,".ld",outputname));
}

FILE
*ldfile_find_command_file(next_file,actualname)
command_file_list *next_file;
char **actualname;
{
    FILE *f;
    search_dirs_type *search;
    char buffer[1000];
    
    f = (FILE *)NULL;
    
    
    if ((!next_file->named_with_T) || (strrchr(next_file->filename,'/'))
#ifdef DOS
	|| (strrchr(next_file->filename,'\\'))
	|| (next_file->filename[1] == ':')
#endif
	) {
	/* If file not named with -T or if a path with given with -T,
	   just check for the name we have, no searchpath stuff */
	f = try_open(next_file->filename,0,actualname);
	if (!f) {
	    info("%P%F cannot open linker directive file %s\n",next_file->filename);
	}
    }
    else {
	/* Try the current search path, which in the GNU 
	   environment will have the current directory, and in
	   both environments will have at least -L search
	   directories */
	for (search = search_head; search; search = search->next) {
	    sprintf(buffer,"%s/%s", search->name,
		    next_file->filename);
	    f = try_open(buffer,1,actualname);
	    if (f) break;
	}
	if (!f) {
	    char *path;
	    int i,pathok;
	    
	    /* Need to search the default directories that
	       may not yet have been added to the end of the
	       search path. */
	    
	    i = 0;
	    path = ldemul_get_searchdir(i++,&pathok);
	    /* Walk through the list of default directories for
	       this environment, exiting when we find our file or
	       run out of directories */
	    while (pathok && !f) {
		if (path) {
		    sprintf(buffer,"%s/%s", path,
			    next_file->filename);
		    f = try_open(buffer,1,actualname);
		}
		path = ldemul_get_searchdir(i++,&pathok);
	    }
	    if (!f) {
		info("%P%F cannot open linker directive file %s\n",
		     next_file->filename);
	    }
	}	
    }
    /* By this point, we have found and opened the file named in
       next_file, and f is a valid FILE* Or, we have exitted from the
       linker because we could not find the named file. */
    return f;
}

void
ldfile_parse_command_file(next_file)
    command_file_list *next_file; 
{
    char *actualfilename;
    FILE *f = ldfile_find_command_file(next_file,&actualfilename);
    
    parse_script_file( f, actualfilename, next_file->named_with_T);
    had_script = true;
}

static
char *
gnu960_map_archname( name , machine , actual_machine )
    CONST char *name;
    unsigned long *machine, *actual_machine;
{
    struct tabentry { 
	char *cmd_switch; 
	char *arch; 
	unsigned long machine,machine_2;
    };
    
#define CA_LIB_SFX "ca"
    
    static struct tabentry arch_tab[] = {
	"KA",     "ka",       bfd_mach_i960_ka_sa,   BFD_960_KA,
	"SA",     "ka",       bfd_mach_i960_ka_sa,   BFD_960_SA, /* Functionally equivalent to KA */
	"KB",     "kb",       bfd_mach_i960_kb_sb,   BFD_960_KB,
	"SB",     "kb",       bfd_mach_i960_kb_sb,   BFD_960_SB, /* Functionally equivalent to KB */
	"CA",     CA_LIB_SFX, bfd_mach_i960_ca,      BFD_960_CA,
	"CA_DMA", CA_LIB_SFX, bfd_mach_i960_ca,      BFD_960_CA,
	"CF",     CA_LIB_SFX, bfd_mach_i960_ca,	     BFD_960_CF, /* Functionally equivalent to CA */
	"CF_DMA", CA_LIB_SFX, bfd_mach_i960_ca,      BFD_960_CF,
	"JA",     "jx",       bfd_mach_i960_jx,      BFD_960_JA,
	"JF",     "jx",       bfd_mach_i960_jx,      BFD_960_JF,
	"JD",     "jx",       bfd_mach_i960_jx,      BFD_960_JD,
	"JL",     "jx",       bfd_mach_i960_jx,      BFD_960_JL,
	"RP",     "jx",       bfd_mach_i960_jx,      BFD_960_RP,
	"HA",     "hx",       bfd_mach_i960_hx,      BFD_960_HA,
	"HD",     "hx",       bfd_mach_i960_hx,      BFD_960_HD,
	"HT",     "hx",       bfd_mach_i960_hx,      BFD_960_HT,
	
	NULL, NULL, 0, 0
    };
    struct tabentry *tp;
    
    for ( tp = arch_tab; tp->cmd_switch != NULL; tp++ ){
	if ( !strcmp(name,tp->cmd_switch) ){
	    break;
	}
    }
#ifdef PROCEDURE_PLACEMENT
    /* We need to know if this is really an i960CF for proc placement */
    /* Paul Reger forced me to add this hackery instead of changing bfd */
    is_really_i960_cf = !(strcmp (name, "CF") && strcmp (name, "CF_DMA"));
#endif
    if ( tp->cmd_switch == NULL ){
	info("%F%P: unknown architecture: %s\n",name);
    }
    *machine = tp->machine;
    *actual_machine = tp->machine_2;
    return tp->arch;
}

/*
   
   ldfile_add_arch is a misnomer.
   
   We enter this function to do one of two things:
   
   1. Set first element of search_arch_list to "" indicating an architecture
   independent library.  ldmain.c does this.  The output architecture is
   NOT set if this is the case.
   
   2. We set the second element which dictates the type of architectures of
   libraries to be sought, AND THE OUTPUT ARCHITECTURE IS SET.
   
   
   */


void
ldfile_add_arch(name)
    CONST char *name;
{
    search_arch_type *new;
    char *temp = NULL;
    unsigned long tempmach,temprealmach;
    
    if ((name && *name == '\0') ||    /* Able to add "" to search path to support -Ldir
					 and -larch w/o having to name the archives with arch
					 stamp of 'kb' 'ca' etc.
					 We are also able to add a 'ka',
					 'kb', 'ca' arch as the second
					 member of the list. */
	(temp=gnu960_map_archname( name, &tempmach, &temprealmach ))) {
	if (search_arch_head == (search_arch_type *) 0 ||
	    search_arch_head->next == (search_arch_type *) 0) {  /* We are
								    adding a
								    member to
								    the list.
								    */
	    new = (search_arch_type *)
		    ldmalloc((bfd_size_type)(sizeof(search_arch_type)));
	    new->next = (search_arch_type *) 0;
	    *search_arch_tail_ptr = new;
	    search_arch_tail_ptr = &new->next;
	    new->name = temp ? temp : "";
	}
	else    /* There are already two elements in the list.  We
		   merely overwrite the name in the second (and final)
		   part of the list. */
		new = search_arch_head->next;
	if (name && *name !='\0') {  /* Don't overwrite the machine name if we passed ""
					to this routine. */
	    ldfile_output_machine_name = temp;  /* Otherwise set it
						   to the blessed
						   string (per
						   gnu960_map_archname() */
	    ldfile_real_output_machine = temprealmach;
	    ldfile_output_machine = tempmach;
	    new->name = temp;   /* If name is "" use it, if name
				   is processed per
				   gnu960_map_archname() use that */
 	    if (tempmach == bfd_mach_i960_jx ||
 		tempmach == bfd_mach_i960_hx) { /* ADD CA_LIB_SFX to bottom of JX and HX lib
 						   search arch path. */
 		*search_arch_tail_ptr = new->next = (search_arch_type *)
 			ldmalloc((bfd_size_type)(sizeof(search_arch_type)));
 		new->next->next = (search_arch_type *) 0;
 		new->next->name = CA_LIB_SFX;
 	    }
	}
    }
    /* gnu960_map_archname issues appropriate hard error if we try to pass an
       invalid arch name.
       */
}
