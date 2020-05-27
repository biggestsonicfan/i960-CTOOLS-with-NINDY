/*****************************************************************************
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
 ******************************************************************************/

/* 
 *  Written by Steve Chamberlain steve@cygnus.com
 * $Id: ldmain.c,v 1.160 1996/01/19 21:22:29 timc Exp $ 
 */

#include "sysdep.h"
#include "bfd.h"
#include "config.h"
#include "ld.h"
#include "ldmain.h"
#include "ldmisc.h"
#include "ldwrite.h"
#include "ldgramtb.h"
#include "ldsym.h"
#include "ldlang.h"
#include "ldemul.h"
#include "ldexp.h"
#include "ldfile.h"

#ifdef DOS
#	include "gnudos.h"
#endif /*DOS*/

/* IMPORTS */

extern boolean lang_has_input_file;
extern lang_statement_list_type input_file_chain;

/* EXPORTS */

ld_config_type config;
args_type command_line;
char *default_target;
static char A_DOT_OUT[] = "a.out";
static char B_DOT_OUT[] = "b.out";
static char E_DOT_OUT[] = "e.out";
char *output_filename                  = A_DOT_OUT;
boolean C_switch_seen                  = false;    /* -C file.o was seen */
boolean do_cave                        = true;     /* By default, we form cave sections,
 						      -D says do not form cave sections. */
boolean B_switch_seen                  = false;    /* -B section-name
							section-start-address
							was seen */
static boolean dash_j                  = false;    /* -j for compression of coff tags. */
static boolean dash_J                   = false;   /* -J for compression of coff tags. */
boolean e_switch_seen                  = false;    /* -e <entry point> was
							seen */
boolean o_switch_seen                  = false;    /* -o <output file name>
							was seen */
boolean code_is_pi                     = false;    /* Position independent stuff -px. */
boolean data_is_pi                     = false;
boolean inhibit_optimize_calls         = false;    /* -Oc -Ob */
boolean inhibit_optimize_branch        = false;    /* -Ob -Ob */
boolean suppress_all_warnings           = false;    /* -W */
boolean suppress_mult_def_size_warnings = false;    /* -t */
boolean show_mult_def_warnings         = false;    /* -M */
boolean suppress_time_stamp            = false;    /* -z */
boolean emit_version                   = false;    /* -V */
int     default_fill_value             = 0;        /* -f value */
char *  dash_A_option;
boolean flat_memory_option             = false;    /* -a (old lnk960's -F option) */
char *program_name;		/* Name this program was invoked by.  */
bfd *output_bfd = 0;		/* The file that we're creating */
boolean option_v = false;
char lprefix = 'L';		/* The local symbol prefix */
int multiple_def_count = 0;	/* Number of global symbols multiply defined */
lang_search_rules_type linker_search_rules = lang_old_and_new_search_rules;

/* Number of symbols defined through common declarations.
 * This count is referenced in symdef_library, linear_library, and
 * modified by enter_global_ref.
 *
 * It is incremented when a symbol is created as a common, and
 * decremented when the common declaration is overridden
 *
 * Another way of thinking of it is that this is a count of
 * all ldsym_types with a ->scoms field
 */
unsigned int commons_pending = 0;

/* Number of global symbols referenced and not defined. 
 * Common symbols are not included in this count.
 */
unsigned int undefined_global_sym_count = 0;


int warning_count = 0;		/* Number of warning symbols encountered */
boolean had_script = false;	/* Have we had a load script ? */
boolean write_map = false;	/* true => write load map.  */
static char *map_filename;
FILE *map_file;
enum target_flavour_enum output_flavor; /* output file format: b.out or coff */

/* output executable file, even if there are non-fatal errors */
boolean force_make_executable = false;
boolean circular_lib_search = false;

#ifdef PROCEDURE_PLACEMENT
boolean form_call_graph = false;
#endif

/* Force the output of profiling initialization routines.  Even if relocatable
   output.  Used in ldlang.c. */
boolean force_profiling = false;

/* Total number of local symbols ever seen - sum of
 * symbol_count field of each newly read afile.
 */
unsigned int total_symbols_seen;

/* Number of read files - the same as the number of elements in file_chain */
unsigned int total_files_seen;

strip_symbols_type  strip_symbols  = STRIP_NONE;
discard_locals_type discard_locals = DISCARD_NONE;

HOW_INVOKED invocation;

static void parse_args();

char * CCINFO_dir;    /* -Z dir and I960PDB and G960PDB environment variables. */

static void
add_command_file(name,with_T)
    char *name;
    boolean with_T;
{
    command_file_list p;

    p.filename = name;
    p.named_with_T = with_T;
    ldfile_parse_command_file(&p);
}

/*
 * The following code is to support the -y<symbol to be traced> and the hidden
 * -Y (trace all symbols) options.
 *
 * This routine places all of the symbols into the linked list.  It is called from
 * from process_switch().
*/

static int Trace_all = 0;

struct trace_symbol_list_node {
    char *name;
    struct trace_symbol_list_node *next;
} *Trace_symbol_list_root = (struct trace_symbol_list_node *) 0;

static void
add_trace_symbol(s)
    char *s;
{
    struct trace_symbol_list_node *new;

    new = (struct trace_symbol_list_node *) ldmalloc(sizeof(struct trace_symbol_list_node));
    new->next = Trace_symbol_list_root;
    Trace_symbol_list_root = new;
    new->name = buystring(s);
}

static unsigned long 
DEFUN(getnum,(where_we_are,chars_we_use),
	CONST char *where_we_are AND
	int *chars_we_use)
{
	/* Return the value represented by the next contiguous string of
	characters. */ 
	long base;
	long l;
	char ch;
	char *in_ptr;

	l = 0;
	in_ptr = (char *)where_we_are;
	ch = *in_ptr++;
	*chars_we_use = 1;

	if (ch == '0') {
		base = 8;
		ch = *in_ptr++;
		(*chars_we_use)++;
		if ( ch == 'x' || ch == 'X' ){
		        ch = *in_ptr++;
			(*chars_we_use)++;
			base = 16;
		}

	} else
		base = 10;

	while (1) {
		switch (ch) {
		case '8': case '9':
		        if (base == 8)
			        goto ejectit;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': 
			l = l * base + ch - '0';
			break;

		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		        if (base < 16) goto ejectit;
			l = (l*base) + ch - 'a' + 10;
			break;

		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		        if (base < 16) goto ejectit;
			l = (l*base) + ch - 'A' + 10;
			break;

		case 'k': case 'K':
			l = l * 1024;
			return (l);

		case 'm': case 'M':
			l = l * 1024 * 1024;
			return (l);

		default:
ejectit:
			return (l);
		}
		ch = *in_ptr++;
		(*chars_we_use)++;
	}
}

/* This routine parses an expression that is to be used to set the start
address of a section. It allows constants (hex, octal, or decimal), '+',
'-', and symbolic names. It builds an expression tree that will be
evaluated in lang_allocate_by_start_address(). */

static char tempstring[128];

etree_type *
DEFUN(parse_B_arg,(addrstring),
	CONST char *addrstring)
{
	int i;	/* pointer to character in addrstring currently under
		consideration */
	int charcount;	/* remembers how many characters we used to to get
			a particular number or symbol name out of addrstring */
	char this_operator;
	etree_type *return_tree;

	i = 0;
	this_operator = (char)0;
	
	/* Anything that starts with a decimal digit is a number; anything
	that starts with a letter, a dot, or an underscore is a symbolic name;
	we accept only + and - as operators, no parentheses, no subscripts, no
	keywords. We expect a strict operand-operator-operand-operator...
	sequence. */
	while (addrstring[i]) {
		if (isdigit(addrstring[i])) {

			unsigned long numback;

			if ((i > 0) && (!this_operator)) {
				/* We should be getting an operator, this
				is an operand, emit error */
				info("%FInvalid string %s given as second argument to -B\n",addrstring);
			}
			/* Collect the whole number, translate it to int */
			charcount = 0;
			numback = getnum(&(addrstring[i]),(int *)&charcount);
			if (i==0) {
				return_tree = exp_intop(numback);
			}
			else {
				return_tree =
					exp_binop(this_operator,return_tree,exp_intop(numback));
			}
			i += (charcount - 1);
			this_operator = (char)0;
		}
		else if ((isalpha(addrstring[i])) || (addrstring[i] == '_') ||
			(addrstring[i] == '.')) { 

			int save_i,j;

			save_i = i;
			if ((i > 0) && (!this_operator)) {
				/* We should be getting an operator, this
				is an operand, emit error */
				info("%FInvalid string %s given as second argument to -B\n",addrstring);
			}
			/* Collect the whole symbol name, translate it via
			exp_nameop(), set up to get operator next, bump i */
			tempstring[0] = addrstring[i];
			i++;
			j = 1;
			while ((isalnum(addrstring[i])) ||
				(addrstring[i] == '_') ||
				(addrstring[i] == ',') ||
				(addrstring[i] == '$')) {
				tempstring[j++] = addrstring[i++];
			}
			tempstring[j] = '\0';
			if (save_i==0) {
				return_tree = exp_nameop(NAME,buystring(tempstring));
			}
			else {
				return_tree =
					exp_binop(this_operator,return_tree,
						  exp_nameop(NAME,buystring(tempstring)));
			}
			this_operator = (char)0;
		}
		else if ((addrstring[i] == '+') || (addrstring[i] == '-')) {
			if (this_operator) {
				/* We should be getting an operand, this
				is an operator, emit error */
				info("%FInvalid string %s given as second argument to -B\n",addrstring);
			}
			this_operator = addrstring[i];
			i++;
		}
		else if (isspace(addrstring[i])) {
			/* Read past white space */
			i++;
			while (isspace(addrstring[i])) {
				i++;
			}
		}
		else {
			/* Got something that should not be in the second
			argument to -B */
			char temp[2];

			temp[0] = addrstring[i];
			temp[1] = '\0';
			info("%F Character \'%s\' not acceptable in section-start-address argument to -B\n",
				&(temp[0]));
		}
	}
	return (return_tree);
}

static void
dump_asymbols(asymbols,count,name)
    asymbol **asymbols;
    int count;
    char *name;
{
    int i;

    if (asymbols == (asymbol **) 0)
	    return;
    for (i=0;i < count;i++) {
	/*
	 * We have to be careful here.
	 * The symbol may be a line number.
	 */
	if (asymbols[i]->name != NULL &&
	    ((name[0] == '\0') ||
	     (strcmp(name,asymbols[i]->name) == 0))) {
	    char *ref_type;
	    char buff[128];

	    if (asymbols[i]->section) {
		sprintf(buff,"defined in %s section%s",asymbols[i]->section->name,
			(asymbols[i]->flags & BSF_LOCAL) ? " (local symbol)" : "");
		ref_type = buff;
	    }
	    else if (asymbols[i]->flags & BSF_FORT_COMM)
		    ref_type = "defined as common";
	    else if (asymbols[i]->flags & BSF_UNDEFINED)
		    ref_type = "referenced";
	    else if (asymbols[i]->flags & BSF_DEBUGGING)
		    ref_type = "debugging information";
	    else if (asymbols[i]->flags & BSF_ABSOLUTE)
		    ref_type = "absolute symbol";
	    else {
		sprintf(buff,"***UNKNOWN*** flags = 0x%x",asymbols[i]->flags);
		ref_type = buff;
	    }
	    printf("symbol %s %s",asymbols[i]->name,ref_type);
	    if (asymbols[i]->the_bfd != (struct _bfd *) 0) {
		printf(" in %s",asymbols[i]->the_bfd->filename);
		if (asymbols[i]->the_bfd->my_archive)
			printf(" (%s)",asymbols[i]->the_bfd->my_archive->filename);
	    }
	    putchar('\n');
	}
    }
}

static void
check_symbols_for_traces(count,asymbols)
    int count;
    asymbol **asymbols;
{
    struct trace_symbol_list_node *p = Trace_symbol_list_root;
    
    for (;p != (struct trace_symbol_list_node *) 0;p = p->next)
	    dump_asymbols(asymbols,count,p->name);
}

/*
  This routine returns 1 iffi the filename corresponds to a script file.
*/

static int
is_a_script(name,target_type,abfd,devno,ino)
    char *name,*target_type;
    bfd **abfd;
    unsigned long *devno,*ino;
{
    struct stat st;
    int it_is_a_script;
	
	(*abfd) = (bfd*) 0;
    if (stat(name,&st) == 0) {
	*devno = st.st_dev;
	*ino = st.st_ino;
	if ((*abfd) = bfd_openr(name,target_type)) {
	    if (bfd_check_format(*abfd,bfd_object) ||
		bfd_check_format(*abfd,bfd_archive))
		    it_is_a_script = 0;
	    else
		    /* It is not an archive or an object file as far as
		       bfd is concerend. */
		    it_is_a_script = 1;
	}
	else
		it_is_a_script = 1;  /* Bfd can't open it ???  But we can stat it ???  There is probably
					a bug in bfd if we can get here, but let's not get too excited
					about this. */
    }
    else {
	/* Can not stat it?.  It is definitely not a script, but it is also not an object file.
	   We'll catch the error later on in cached_bfd_openr() in ldfile.c */
	*devno = -1;
	*ino = -1;
	it_is_a_script = 0;
    }
    if (it_is_a_script)
	    bfd_close(*abfd);
    return it_is_a_script;
}

/* Emit command line options to the map file. */
static void
emit_args(argc,argv)
    int argc;
    char *argv[];
{
    int i,len=0;

    fprintf(map_file,"\n**COMMAND LINE OPTIONS**\n");
    for (i=1;i < argc;i++) {
	if (len+strlen(argv[i]) > 79) {
	    fputc('\n',map_file);
	    len = 0;
	}
	len += fprintf(map_file,"%s ",argv[i]);
    }
    fprintf(map_file,"\n\n");
}

static struct filelist_node {
    char *filename,*archname,*symbolname;
    struct filelist_node *left,*right;
} *filelistsroots[(int)LAST_FILELIST_NODE_TYPE];

static struct filelist_node *
put_in_tree(p,s1,s2)
    struct filelist_node **p;
    char *s1,*s2;
{
    int s,i;

    if ((*p) == (struct filelist_node *) 0)
	    return ((*p) = (struct filelist_node *) ldmalloc(sizeof(struct filelist_node)));

    for (i=0,s=strcmp(s1,(*p)->filename);i < 2;i++) {
	if (s < 0)
		return put_in_tree(&(*p)->left,s1,s2);
	else if (s > 0)
		return put_in_tree(&(*p)->right,s1,s2);
	else if (s2 && (*p)->archname)
		s = strcmp(s2,(*p)->archname);
	else
		return 0;
    }
    
}

void
add_file_to_filelist(type,s1,s2,s3)
    enum filelist_node_type type;
    char *s1,*s2,*s3;
{
    struct filelist_node *p = put_in_tree(&filelistsroots[(int)type],s1,s2);

    if (p) {
	p->left = p->right = (struct filelist_node *) 0;
	p->filename = buystring(s1);
	switch (type) {
    case outputfile:
    case ldfile:
    case objfile:
    case dash_r_file:
    case archivefile:
	    p->archname = NULL;
	    p->symbolname = NULL;
	    break;
    case archive_member:
	    p->archname = buystring(s2);
	    p->symbolname = buystring(s3);
	    break;
	}
    }
}

static void
print_filelist_worker(p,fmt)
    struct filelist_node *p;
    char *fmt;
{
    if (p) {
	if (p->left)
		print_filelist_worker(p->left,fmt);
	fprintf(map_file,fmt,p->filename,p->archname,p->symbolname);
	if (p->right)
		print_filelist_worker(p->right,fmt);
	free(p->filename);
	if (p->archname)
		free(p->archname);
	if (p->symbolname)
		free(p->symbolname);
	free(p);
    }
}

void
ldmain_emit_filelist()
{
    static char *nametypes[] = {"Linker Directive Files",
					"Object Files",
					"Symbols only (-R) Files",
					"Archive members",
					"Output File",
					"Archive Files"};

    enum filelist_node_type t;
    fprintf(map_file,"**FILES INCLUDED IN LINK**\n");

    for (t=ldfile;t < LAST_FILELIST_NODE_TYPE;t++) {
	fprintf(map_file,"%s:\n",nametypes[(int)t]);
	if (filelistsroots[(int)t]) {
	    print_filelist_worker(filelistsroots[(int)t],t == archive_member ? 
				  "%s from %s\n\t needed due to %s\n" : "%s\n");
	    fprintf(map_file,"\n");
	}
	else
		fprintf(map_file,"NONE\n\n");
    }
    fprintf(map_file,"\n");
}

#if !defined(NO_BIG_ENDIAN_MODS)
extern int target_byte_order_is_big_endian;
#endif /* NO_BIG_ENDIAN_MODS */

int
main (argc, argv)
    char **argv;
    int argc;
{
    char *invocation_name, *temporary;
    int i;

    db_set_prog (argv[0]);
    db_set_signal_handlers();

#ifdef DOS
#ifdef _INTELC32_
	/********************************************************
	**  Initialize the internal Code Builder run time libraries
	**  to accept the maximum number of files open at a time
	**  that it can.  With this set high, the number of files
	**  that can be open at a time is completely dependent on 
	**  DOS config.sys FILES= command.
	*********************************************************/
	_init_handle_count(MAX_CB_FILES);
#endif
#endif
	argc = get_response_file(argc,&argv);
        argc = get_cmd_file(argc, argv);

	/* Save away argc and argv for later use by 2pass stuff... */
	gnu960_set_argc_argv (argc, argv);

	map_file = stdout;
	invocation_name = program_name = argv[0];
	/* We want to see the name by which the linker was invoked this
	time. To get this, we need to eliminate any preliminary directory
	information included in the invocation. */
	
	temporary = strrchr(program_name,(int)'/');
	if (temporary) {
		invocation_name = temporary+1;
	}
#ifdef DOS
	/* Deal with DOS's non-UNIX-like directory notation */
	else {
		temporary = strrchr(program_name,(int)'\\');
		if (temporary) {
			invocation_name = temporary+1;
		}
		else {
			temporary = strrchr(program_name,(int)':');
			if (temporary) {
				invocation_name = temporary+1;
			}
		}
	}
#endif
	/* By this point, invocation_name should point to the part of what
	the linker was invoked as that falls to the right of all directory
	specification. */

	check_v960( argc, argv );
	if(argc==1) {
		put_gld_help();
		exit(0);
	}
#ifdef DOS
	if (!strnicmp(invocation_name,"gld960",6))
#else
	if (!strncmp(invocation_name,"gld960",6))
#endif
        {
		invocation = as_gld960;
		output_flavor = BFD_BOUT_FORMAT;	
	}
	else {
		invocation = as_lnk960;
   		output_flavor = BFD_COFF_FORMAT;	
	}

	parse_args(argc, argv, 1);
	if (target_byte_order_is_big_endian && (output_flavor == BFD_BOUT_FORMAT))
	{
		info("%F%P: BOUT format does not support big-endian target applications\n"); /* no return */
	}

	if (invocation == as_lnk960 && output_flavor == BFD_BOUT_FORMAT)
		info("%F%P: BOUT is not supported when linker is emulating lnk960\n");

	ldfile_add_arch("");    /* First search for non arch specific libraries. */
	ldfile_add_arch("KB");  /* Default architecture is this. */
	if (invocation == as_gld960) {
		/* Note that this will be overwritten later
		by a command line or script file -A switch */
		char *arch = (char *)getenv("G960ARCH"),*pdb = (char *)getenv("G960PDB");
		if (arch)
			ldfile_add_arch(arch);
		if (pdb)
			CCINFO_dir = pdb;
	} 
	else {
		char *arch = (char *)getenv("I960ARCH"),*pdb = (char *)getenv("I960PDB");;
		if (arch)
			ldfile_add_arch(arch);
		if (pdb)
			CCINFO_dir = pdb;
	}
	command_line.force_common_definition = false;
	config.relocateable_output = false;
	config.make_executable = true;
	config.magic_demand_paged = true;

	ldemul_choose_mode();
	default_target = ldemul_choose_target();

	lang_init();
	ldemul_before_parse();
	lang_has_input_file = false;

	if (option_v) {
		if (invocation == as_gld960) {
			fprintf(stdout,"Linker will behave like gld960\n");
		}
		else {
			fprintf(stdout,"Linker will behave like lnk960\n");
		}
	}

	parse_args(argc, argv, 0);

	if (write_map) {
	    if (map_filename) {
		if (!(map_file = fopen(map_filename,"w"))) {
		    perror(map_filename);
		    fprintf(stderr,"Sending map to stdout\n");
		    map_file = stdout;
		}
	    }
	    emit_args(argc,argv);
	}

	if (dash_A_option)
		ldfile_add_arch(dash_A_option);

	if (lang_has_input_file == false) {
		info("%P%F: No input files\n");
	}

	ldemul_after_parse();
	lang_add_output(output_filename);
	if (write_map)
		add_file_to_filelist(outputfile,output_filename,NULL,NULL);
	lang_process();

	if (suppress_time_stamp)
		output_bfd->flags |= SUPP_W_TIME;

	/* FIX ME:

	   This block of code is out of place.  It should be a function, or its functionality
	   should be moved to the read file snarf symbol table routines above (Q_*).

	   This code checks mixing pix / non-pix code.

	   */

	if (!suppress_all_warnings &&
 	    ((output_flavor == bfd_target_coff_flavour_enum) ||
 	     (output_flavor == bfd_target_elf_flavour_enum)) &&
	    (data_is_pi || code_is_pi)) {
	    lang_input_statement_type *infile;

	    infile = (lang_input_statement_type *)input_file_chain.head;

	    while (infile) {
		if ((infile->real) && (infile->filename)) {
		    if (infile->the_bfd) {
			if (gnu960_check_format(infile->the_bfd, bfd_archive)) {
			    if (infile->subfiles) {
				lang_input_statement_type *now;

				now = infile->subfiles;
				while ((now) && (now->filename)){
				    if ((code_is_pi) &&
					(!(now->the_bfd->flags & HAS_PIC)) &&
					(!(now->the_bfd->flags & CAN_LINKPID)))
					    info("WARNING: non-position-independent code in %s (from %s)\n",
						 now->filename,infile->filename);
				    if ((data_is_pi) &&
					(!(now->the_bfd->flags & HAS_PID)) &&
					(!(now->the_bfd->flags & CAN_LINKPID)))
					    info("WARNING: non-position-independent data in %s (from %s)\n",
						 now->filename,infile->filename);
				    now = (lang_input_statement_type *)now->chain;
				}
			    }
			}
			else {
			    if ((code_is_pi) &&
				(!(infile->the_bfd->flags & HAS_PIC)) &&
				(!(infile->the_bfd->flags & CAN_LINKPID)))
				    info("WARNING: non-position-independent code in %s\n",
					 infile->filename);
			    if ((data_is_pi) &&
				(!(infile->the_bfd->flags & HAS_PID)) &&
				(!(infile->the_bfd->flags & CAN_LINKPID)))
				    info("WARNING: non-position-independent data in %s\n",
					 infile->filename);
			}
		    }
		}
		infile = (lang_input_statement_type *)infile->next_real_file;
	    }
	}

	if (code_is_pi)
		output_bfd->flags |= HAS_PIC;

	if (data_is_pi)
		output_bfd->flags |= HAS_PID;

	if (dash_J)
		output_bfd->flags |= STRIP_DUP_TAGS;

	if (dash_j)
		output_bfd->flags |= DO_NOT_STRIP_ORPHANED_TAGS;

	if (discard_locals == DISCARD_ALL || strip_symbols == STRIP_DEBUGGER)
		output_bfd->flags |= STRIP_LINES;

	if (strip_symbols == STRIP_DEBUGGER)
		output_bfd->flags |= STRIP_DEBUG;

	if (!config.relocateable_output || force_make_executable) {
		output_bfd->flags |= EXEC_P;
	}

	if (config.relocateable_output) {
		output_bfd->flags &= ~D_PAGED;
	}

	ldwrite();

	if (write_map) {
	    lang_map_output_sections(map_file);
	    fflush(map_file);
	    if (map_file != stdout)
	        fclose(map_file);
	}

	bfd_close(output_bfd);
	gnu960_remove_tmps();

	if ( !config.make_executable && !force_make_executable) {
	    info("%P: Fatal error.  Output file: %s was not created.\n",
		 output_filename?output_filename:"unknown");
	    unlink(output_filename);
	}

	if (config.make_executable)
	    /* If /gcdm was supplied, invoke gcdm960 and don't come back. */
	    gnu960_do_gcdm();

	return (!config.make_executable);
}

static int
hexint(s,np)
    char *s;
    int *np;
{
	/* Skip over optional leading "0x" */
	if ( (s[0] == '0') && (s[1] == 'x') ){
		s += 2;
	}

	if ( (int)strlen(s) > 8 ){
		return 0;
	}
	for ( *np=0; *s; s++ ){
		*np <<= 4;
		switch ( *s ){
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			*np += *s - '0';
			break;
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
			*np += *s - 'a' + 10;
			break;
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
			*np += *s - 'A' + 10;
			break;
		default:
			return 0;
		}
	}
	return 1;
}

static void
script_scope_error(name,t_or_i)
    char *name,*t_or_i;
{
    info("%P: syntax error at ");
    lex_err();
    info("\n%P: Either %s has bad format, or you attempted to nest a script\n",name);
    info("%P%F: file in a command script w/o %s(%s).\n",t_or_i,name);
}

int		/* return 2 if third arg is consumed, 1 if next_arg consumed, 0 otherwise */
process_switch( s, next_arg, third_arg, from_command_line, was_a_target_or_defsym, preliminary)
    char *s;		/* The switch on entry includes the "-" (or "/" on dos) */
    char *next_arg, *third_arg;
    int from_command_line;
    int *was_a_target_or_defsym,preliminary;
{
#define missing_arg(swt) info("%F%P: Missing argument for switch -%s\n",swt)

    *was_a_target_or_defsym = 0;
    s++;   /* Skip past the '-' or '/' on entry. */

    if (*s == '\0') {
	info("%F%P: Missing switch\n");
	return 0;
    }

    for (;;s++)
	    switch (s[0]) {
	case '\0':
		return 0;

	case 'a':
		flat_memory_option = true;
		continue;

	case 'A':
		/* -A CA == -ACA. */
		if ( s[1] != '\0' ){
		    dash_A_option = s+1;
		    return 0;
		}
		else if (!next_arg) {
		    missing_arg(s);
		    return 0;
		}
		else {
		    dash_A_option = next_arg;
		    return 1;
		}

 	case 'b':
#ifdef PROCEDURE_PLACEMENT
 		form_call_graph = true;
#endif
 		continue;


	case 'B':
		/* -B */
		B_switch_seen = true;
	    {
		    char *secname,*addrstring;
		    int retval;
		    
		    /* Assume we got -Bname address */
		    secname = buystring(&s[1]);
		    if ( !next_arg ){
			missing_arg(s);
			return 0;
		    }
		    addrstring = buystring(next_arg);
		    retval = 1;
		    if (s[1] == '\0') {
			/* If we got -B name address */
			secname = buystring(next_arg);
		    	if ( !third_arg ){
				missing_arg(s);
				return 0;
			}
			addrstring = buystring(third_arg);
			retval = 2;
		    }
		    if (!preliminary)
			    lang_section_start(secname,parse_B_arg(addrstring));
		    return retval;
		}

	case 'c':
		/* -c */
		circular_lib_search = true;
		continue;

	case 'C':
		/* -C */
		C_switch_seen = true;
		continue;
		
 	case 'D':
 		do_cave = false;
 		continue;

	case 'd':
		/* -dc == -d c == -dp == -d p.
		   -d != -dc  (new) */	
		if ((s[1] == '\0') || !strcmp(s,"dc") || !strcmp(s,"dp")){
		    command_line.force_common_definition = true;
		    if (s[1] != '\0')   /* -dc and -dp case. */ {
			return 0;
		    }
		    else if (next_arg && (!strcmp(next_arg,"p") || !strcmp(next_arg,"c"))) {
			return 1;
		    }
		    else {
			missing_arg(s);
			return 0;
		    }
		} else if ( !strcmp(s,"defsym") ) {
		    if ( !next_arg ){
			missing_arg(s);
			return 0;
		    }
		    if (!preliminary)
			    parse_defsym( next_arg );
		    *was_a_target_or_defsym = 1;
		    return 1;
		}
		goto bad_switch;
		
	case 'e':
		/* -e sym == -esym */
		e_switch_seen = true;
		if ( s[1] == '\0' ){
		    if ( !next_arg ) {
			missing_arg(s);
			return 0;
		    }
		    if (!preliminary)
			    lang_add_entry(next_arg);
		    return 1;
		}
		else {
		    if (!preliminary)
			    lang_add_entry(s+1);
		    return 0;
		}
		
	case 'F':
		/* -Fcoff == -F coff, -Fbout == -F bout, -Felf == -F elf */
		/* Make sure we flag the old -F (flat) option
		   in 3.5 as unsupported: */
	    { char *tmp  = s[1] ? s+1 : next_arg;
	      int rvalue = s[1] ? 0   : 1;

#define RESET_DEFAULT_OUTPUT_FILENAME(X) (output_filename = (output_filename == A_DOT_OUT || \
		                                            output_filename == B_DOT_OUT ||  \
							    output_filename == E_DOT_OUT ) ? (X) : output_filename)
	      if (!strcmp(tmp,"coff")) {
		  RESET_DEFAULT_OUTPUT_FILENAME(A_DOT_OUT);
		  output_flavor = BFD_COFF_FORMAT;
	      }
	      else if (!strcmp(tmp,"bout")) {
		  RESET_DEFAULT_OUTPUT_FILENAME(B_DOT_OUT);
		  output_flavor = BFD_BOUT_FORMAT;
	      }
	      else if (!strcmp(tmp,"elf")) {
		  RESET_DEFAULT_OUTPUT_FILENAME(E_DOT_OUT);
		  output_flavor = BFD_ELF_FORMAT;
	      }
	      else
		      info("%F%P: unexpected arg supplied to -F option %s\n",tmp);
	      return rvalue;
	  }
	case 'f':	
		/* -f fill_val == -ffill_val */
	    {
		char *arg,*tmp;
		int v;
		int rval = 0;
		
		if ( s[1] == '\0' ){
		    if ( !next_arg ) {
			missing_arg(s);
			return 0;
		    }
		    else {
			arg = next_arg;
			rval = 1;
		    }
		}
		else
			arg = s+1;
		v = strtol(arg,&tmp,0);
		if (tmp && *tmp)
			info("%F%P: Illegal fill value after -f %s\n",arg);
		if ((0x0000ffff & v) != v)
			info("%F%P: no more than 2 bytes required after -f 0x%x \n",v);
	        default_fill_value = v;
		return rval;
	    }
		
#if !defined(NO_BIG_ENDIAN_MODS)
	case 'G':	/* big-endian */
		target_byte_order_is_big_endian = 1;
		continue;

#endif /* NO_BIG_ENDIAN_MODS */

	case 'g':
		if (!strncmp(s,"gcdm",4)) {
		    if (!preliminary) {
			force_profiling = true;
			gnu960_set_gcdm(s);
		    }
		    return 0;
		}
		/* "-g" ignored */
		continue;
	
	case 'h':		/* help switch */
		put_gld_help();	
		exit(0);	
		
	case 'H':
		config.sort_common = true;
		continue;

	case 'j':
		dash_j = true;
		continue;

	case 'J':
		dash_J = true;
		continue;

	case 'L':
		/* -Ldir == -L dir */
		if (s[1] != '\0'){
		    if (!preliminary)
			    ldfile_add_library_path(s+1,1);
		    return 0;
		}
		else if (next_arg) {
		    if (!preliminary)
			    ldfile_add_library_path(next_arg,1);
		    return 1;
		}
		else {
		    missing_arg(s);
		    return 0;
		}
		
	case 'l':
		/* -lc == -l c */
		if (s[1] != '\0' ){
		    if (!preliminary)
			    lang_add_input_file(s+1,
						lang_input_file_is_l_enum, (char*)NULL,(bfd *)0,-1,-1);
		    return 0;
		}
		else if (next_arg) {
		    if (!preliminary)
			    lang_add_input_file(next_arg,
						lang_input_file_is_l_enum, (char*)NULL,(bfd *) 0,-1,-1);
		    return 1;
		}
		else {
		    missing_arg(s);
		    return 0;
		}
		
	case 'm':
		/* Old -M is now -m. */
		write_map = true;
		continue;
		
	case 'M':
		/* Old -M is now show warnings per lnk960. */
		show_mult_def_warnings = true;
		continue;
		
	case 'n':
		force_make_executable = true;
		continue;

	case 'N':
		if (s[1] == '\0' ) {
		    if (!next_arg) {
			missing_arg(s);
			return 0;
		    }
		    map_filename = next_arg;
		    return 1;
		}
		else {
		    map_filename = s+1;
		    return 0;
		}
		break;
		
	case 'o':
		/* -ooutput == -o output */
		o_switch_seen = true;
		if (s[1] == '\0' ) {
		    if (!next_arg) {
			missing_arg(s);
			return 0;
		    }
		    output_filename = next_arg;
		    return 1;
		}
		else {
		    output_filename = s+1;
		    return 0;
		}

	case 'O':
		/* -Ob == -O b, -Os == -Os */
		if ((s[1] == '\0') || !strcmp(s,"Ob") || !strcmp(s,"Os")){
		    char b_or_s[2];
		    int rval = 0;

		    b_or_s[0] = s[1];
		    b_or_s[1] = 0;
		    if (s[1] == '\0') {
			if (next_arg) {
			    b_or_s[0] = *next_arg;
			    rval = 1;
			}
		    }
		    if (!b_or_s[0]) {
			missing_arg(s);
			return 0;
		    }
		    if (b_or_s[0] == 'b') {
			inhibit_optimize_branch = true;
			return rval;
		    }
		    else if (b_or_s[0] == 's') {
			inhibit_optimize_calls = true;
			return rval;
		    }
		    else  /* Fatal error */
			    info("%F%P: either 'b' or 's' required after -%s\n",s);
		}
		goto bad_switch;  /* fall thru on break mean -O z e.g. */
		
	case 'p':
		/* -pb == -p b, -pd == -p d, -pc == -p c */
		
		if (!strcmp(s,"postlink"))
		{
		    if (!preliminary)
			    gnu960_set_postlink();
		    return 0;
		}

		if ((s[1] == '\0') || !strcmp(s,"pb") || !strcmp(s,"pd") || !strcmp(s,"pc")) {
		    char b_d_or_c[2];
		    int rval = 0;
		    
		    b_d_or_c[0] = s[1];
		    b_d_or_c[1] = 0;
		    if (s[1] == '\0') {
			if (next_arg) {
			    b_d_or_c[0] = *next_arg;
			    rval = 1;
			}
		    }
		    if (!b_d_or_c[0]) {
			missing_arg(s);
			return 0;
		    }
		    if (b_d_or_c[0] == 'b') {
			code_is_pi = true;
			data_is_pi = true;
			return rval;
		    }
		    else if (b_d_or_c[0] == 'd') {
			data_is_pi = true;
			return rval;
		    }
		    else if (b_d_or_c[0] == 'c') {
			code_is_pi = true;
			return rval;
		    }
		    else  /* Fatal error */
			    info("%F%P: either 'b', 'd' or 'c' required after -%s\n",s);
		}
		goto bad_switch;  /* fall thru on break mean -p z e.g. */
		
	case 'P':
		/* -P is old -p option  means even with -r emit bss initialization code. */
		force_profiling = true;
		continue;
		
	case 'q':
		if (1) {
		    char *arg = (s[1]) ? (s+1) : next_arg;
		    int rval = 0;

		    if (!arg)
			    missing_arg(s);
		    if (!strcmp(arg,"o"))
			    linker_search_rules = lang_old_search_rules;
		    else if (!strcmp(arg,"n"))
			    linker_search_rules = lang_new_search_rules;
		    else if (!strcmp(arg,"b"))
			    linker_search_rules = lang_old_and_new_search_rules;
		    else
			    info("%P%F: either 'o', 'n' or 'b' required after -%s\n",s);
		    return s[1] ? 0 : 1;
		}			    
		break;

	case 'R':
		/* -Rfile == -R file */
		if (s[1] == '\0') {
		    if (!next_arg) {
			missing_arg(s);
			return 0;
		    }
		    if (!preliminary)
			    lang_add_input_file(next_arg,
						lang_input_file_is_symbols_only_enum,
						(char *)NULL,(bfd *)0,-1,-1);
		    return 1;
		}
		else {
		    if (!preliminary)
			    lang_add_input_file(s+1,
						lang_input_file_is_symbols_only_enum,
						(char *)NULL,(bfd *)0,-1,-1);
		    return 0;
		}
		
	case 'r':
		if (strip_symbols == STRIP_ALL)
			info("%F%P: stripping is pointless with relocatable output\n");
		config.relocateable_output = true;
		config.build_constructors = false;
		config.magic_demand_paged = false;
		continue;
		
	case 'S':
		strip_symbols = STRIP_DEBUGGER;
		continue;
		
	case 's':
		if (!config.relocateable_output) {
		    strip_symbols = STRIP_ALL;
		    continue;
		}
		else
			info("%F%P: stripping is pointless with relocatable output\n");
		break;
		
	case 'T':
		/* First is it: -T[text!data!bss] org ? */
		if (!strcmp(s,"Ttext")||!strcmp(s,"Tdata")||!strcmp(s,"Tbss")){
		    char *secname;
		    int n;
		    
		    if ( !next_arg ){
			missing_arg(s);
			return 0;
		    }
		    if ( !hexint(next_arg,&n) ){
			info("%F%P: hex value required after -%s\n",s);
			/* Fatal error */
		    }
		    switch (s[1]) {
		case 't':
			secname = ".text";
			break;
		case 'd':
			secname = ".data";
			break;
		case 'b':
			secname = ".bss";
			break;
		    }
		    if (!preliminary)
			    lang_section_start(secname,exp_intop(n));
		    return 1;
		}
		else {
		    char *tfilename = NULL;
		    int rvalue = 0;

		    if (s[1] == '\0') {  /* It must be a script -T script == -Tscript */
			if ( !next_arg ){
			    missing_arg(s);
			    return 0;
			}
			tfilename = next_arg;
			rvalue = 1;
		    }
		    else if (s[1] != '\0') {
			tfilename = s+1;
		    }
		    else {
			missing_arg(s);
			return 0;
		    }
		
		    if (!from_command_line) {
			command_file_list t;
			FILE *f;
			char *realname;

			*was_a_target_or_defsym = 1;
			t.named_with_T = true;
			t.filename = tfilename;
			if (!preliminary) {
			    f = ldfile_find_command_file(&t,&realname);
			    parse_script_file(f,realname,1);
			}
		    }
		    else
			    if (!preliminary)
				    add_command_file(tfilename,true);
		    return rvalue;
		}
			
	case 't':
		/* -t was trace files, it is now inside -v option... */
		suppress_mult_def_size_warnings = true;
		continue;
		
	case 'u':
		/* -usym == -u sym */
		if (s[1] == '\0' ){
		    if ( !next_arg ){
			missing_arg(s);
			return 0;
		    }
		    if (!preliminary)
			    ldlang_add_undef(next_arg);
		    return 1;
		}
		else {
		    if (!preliminary)
			    ldlang_add_undef(s+1);
		    return 0;
		}
		
	case 'V':
		if (!emit_version) {
		    emit_version = true;
		    gnu960_put_version();
		}
		continue;
		
	case 'v':
		option_v = true;
		continue;
		
	case 'W':
		suppress_all_warnings = true;
		continue;
		
	case 'X':
		discard_locals = (discard_locals != DISCARD_NONE) ? discard_locals : DISCARD_L;
		continue;
		
	case 'x':
		discard_locals = DISCARD_ALL;
		continue;
		
	case 'y':
		/* -ysym == -y sym */
		if ( s[1] == '\0' ){
		    if ( !next_arg ){
			missing_arg(s);
			return 0;
		    }
		    if (!preliminary)
			    add_trace_symbol(next_arg);
		    return 1;
		}
		else {
		    if (!preliminary)
			    add_trace_symbol(s+1);
		    return 0;
		}
		
	case 'Y':
		Trace_all = 1;
		continue;	
		
	case 'z' :
		suppress_time_stamp = true;
		continue;

	case 'Z' :
		/* -Zdir == -Z dir */
		if ( s[1] == '\0' ){
		    if ( !next_arg ){
			missing_arg(s);
			return 0;
		    }
		    CCINFO_dir = next_arg;
		    return 1;
		}
		else {
		    CCINFO_dir = s+1;
		    return 0;
		}

	default : goto bad_switch;
	    }
    
bad_switch: 
    info("%F%P: unrecognized option '-%s'\nUse the -h option to get help\n", s);
    return 0;
}



 static char *utext[] = {
                "",
                "gld960/lnk960 Cross linkers for the i960 Processor",
                "",
                "Options:",
                "\t   -A: specify an 80960 architecture for the output file",
                "\t\t[valid architectures are: KA,SA,KB,SB,CA,CF,JA,JF,JD,",
		"\t\t HA,HD,HT,RP.  If the linker is invoked as gld960 without",
		"\t\t this switch, the architecture is set from the linker",
		"\t\t directive OUTPUT_ARCH. If an OUTPUT_ARCH directive is",
		"\t\t not present, it will use the environment variable",
		"\t\t $G960ARCH. If the linker is invoked as lnk960 without",
		"\t\t this switch the architecture is set from the linker",
		"\t\t directive OUTPUT_ARCH.  If an OUTPUT_ARCH directive is",
		"\t\t not present, it will use the environment variable",
		"\t\t $I960ARCH.  KB is the default architecture.]",
                "\t   -b: Arrange input sections to avoid cache misses (currently disabled).",
                "\t   -B<section_name> <section_start_address>: specify a starting address",
		"\t\tfor an output section",
                "\t\t[this option over-rides start addresses given either in",
		"\t\t SECTIONS directives or with the -T option]",
                "\t   -C: ignore any STARTUP directive found in linker directive files",
		"\t   -c: iteratively search through libraries to resolve",
		"\t\t circular references",
                "\t   -dc|-dp: these are equivalent;  these options assign space for common",
                "\t\t symbols even when specifying a relocatable output file with -r,",
                "\t\t (this has the same effect as the linker directive",
		"\t\t  FORCE_COMMON_ALLOCATION)",
                "\t   -defsym <symbol=expression>: define an external absolute symbol",
                "\t\t[this definition overrides any library definition of the symbol",
                "\t\t and suppresses loading of any library members otherwise needed",
		"\t\t to resolve references to the symbol]",
                "\t   -e<entry>: define an entry point other than the default",
                "\t\t[the <entry> symbol name becomes the new entry point]",
		"\t   -f fill_value initializes filler with the indicated value",
                "\t   -Fbout|-Fcoff|-Felf: specify the output object module format.",
                "\t\t[-Fbout by default produces an output file named b.out; -Fcoff ",
		"\t\t by default produces an output file named a.out; -Felf by default ",
		"\t\t produces an output file named e.out.  When no format is specified, ",
		"\t\t the linker uses the invocation name to determine",
		"\t\t the object-file format.  for gld960 the default is -Fbout, for",
		"\t\t lnk960 the default is -Fcoff (specifying -Fbout as a switch to",
		"\t\t lnk960 is not supported)]",
                "\t   -g: the linker accepts and ignores this option (for compatibility)",
                "\t   -G: inform gld960 that the target is an i960 processor with big-endian",
		"\t\tmemory regions.  This option is meaningful only for COFF or ELF output.",
                "\t   -h: display this help message",
                "\t   -H: Sort common symbols based on symbol size, intra-input file.  Resulting",
		"\t\tsymbol table in the output load module will reflect the sorted order.",
		"\t   -J: Coff files only.  Remove duplicated tags and compress the string",
		"\t\ttable",
                "\t   -L<searchdir>: add searchdir to the list of paths the linker searches",
                "\t\tfor archive libraries",
                "\t\t[the linker directive SEARCH_DIR will also add directories to",
		"\t\t the search path, such directories are appended after any",
		"\t\t directories specified with -L]",
                "\t   -l<ar>: add an archive file to the list of files to link",
                "\t   -M: issue warnings when multiple definitions of symbols are found",
                "\t\t[these warnings will be issued even when the definitions are",
		"\t\t identical]",
                "\t   -m: send linker map to standard output",
		"\t   -n:  Do not inhibit production of an output file even in",
		"\t\t the case of errors",
		"\t   -N map_file:  Specify the output of map to goto the named file",
                "\t   -Ob|-Os: suppress optimization of CALLJ/CALLX to BAL/BALX or to CALLS",
		"\t\trespectively",
                "\t   -o <output>: specify a name for the linker output file",
                "\t\t[the default is a.out for COFF output and b.out for b.out output",
		"\t\t this option overrides any OUTPUT linker directive]",
		"\t   -P: put two-pass optimization profiling startup code in the",
		"\t\tlinked output",
                "\t\t[the profiling information will be put in the output file even",
		"\t\t when the -r option is used]",
                "\t   -pc|-pd|-pb: link and mark output files with position-independent",
		"\t\tcode, data, or both [use -pd to select libraries with position independent data.",
		"\t\t(all runtime libraries are position-independent)].",
                "\t   -R <filename>: read symbol names and addresses from the <filename>",
		"\t\tobject file without relocating or including the",
		"\t\t<filename> object in your linked output file",
                "\t   -r: generate relocatable linker output",
                "\t\t[the resulting output file can be used for input to a subsequent",
		"\t\t linker invocation;  unless the -P option is specified, the",
		"\t\t linker will not include profiling startup code in the",
		"\t\t relocatable output]", 
                "\t   -s: omit all symbol information from the output file",
		"\t   -S: omit only the debug information from the output file",
                "\t   -Ttext|-Tdata|-Tbss <addr>: specify a starting address other than the",
		"\t\tdefault",
                "\t\t[the -B option offers a more general capability] ",
                "\t   -T<commandfile>: specify a linker directive file located in the",
		"\t\tsearch path",
                "\t\t[if a linker directive file is given without the -T option, only",
		"\t\t the current directory is searched]",
                "\t   -t: suppress warnings of multiple symbol definitions, even if they",
		"\t\tdiffer in size",
                "\t   -u <sym>: enter sym in the output file as an undefined symbol",
                "\t\t[this can be used one or more times, it can cause the linker to",
		"\t\t pull in additional modules from standard libraries]",
                "\t   -V: print version information and continue",
                "\t   -v: print verbose output about the action of the linker",
                "\t-v960: print version information and exit",
                "\t   -W: suppress all warnings except multiple symbol definition warnings",
                "\t   -X: delete local symbols beginning with 'L'", 
                "\t   -x: used with the -s option it deletes all local symbols",
                "\t   -y <sym>: trace the symbol <sym>, indicating each file in which <sym>",
		"\t\tappears",
                "\t\t[multiple symbols may be traced with multiple y options, if",
		"\t\t <sym> came from a C program it must be preceded with an",
		"\t\t underscore]",
                "\t   -z: suppress COFF time stamp",
                "\t\t[the output file is marked as being created at UNIX time zero",
		"\t\t (4:00 p.m., 12/31/69)].",
                "",
                "Please see your user's guide for a complete command-line description.",
                "",
                NULL
        };

/* Routine to output help message */
put_gld_help()
{
        fprintf(stdout,"\nUsage: gld960 [options] file [...]\n");
        fprintf(stdout,"Usage: lnk960 [options] file [...]\n");
        paginator(utext);       /* prints so it can be seen on one page */
} /* put_gld_help */


int
parse_three_args(arg1,arg2,arg3,from_command_line, preliminary)
    char *arg1,*arg2,*arg3;
    int from_command_line,preliminary;
{
    unsigned long devno,ino;
    int dummy;
    bfd *abfd;

    if ( arg1[0] == '-'
#ifdef DOS
	|| arg1[0] == '/'
#endif
	)
	    return process_switch(arg1,arg2,arg3,from_command_line,&dummy,preliminary);
    if (!preliminary) {
	if (is_a_script(arg1,default_target,&abfd,&devno,&ino)) {
	    /* Second argument says this script was not named
	       with a -T command-line switch */
	    if (!from_command_line)
		    script_scope_error(arg1,"INCLUDE");
	    else
		    add_command_file(arg1,false);
	}
	else {
	    lang_add_input_file(arg1,
				lang_input_file_is_file_enum, (char*)NULL,
				abfd,devno,ino);
	}
    }
    return 0;  /* This means we consumed only one of the two args. */
}

static void
parse_args(argc, argv, preliminary)
    int argc;
    char *argv[];
    int preliminary;
{
	int i;
	char *next_arg, *third_arg;

	for ( i=1; i < argc; i++ ) {
	    next_arg = (i+1 >= argc) ? (char*)NULL : argv[i+1];
	    third_arg = (i+2 >= argc) ? (char*)NULL : argv[i+2];
	    i += parse_three_args( argv[i], next_arg, third_arg, 1, preliminary);
	}
}


void
Q_read_entry_symbols (desc, entry)
    bfd *desc;
    struct lang_input_statement_struct *entry;
{
	if (entry->asymbols == (asymbol **)NULL) {
		bfd_size_type table_size = get_symtab_upper_bound(desc);
		entry->asymbols = (asymbol **)ldmalloc(table_size);
		entry->symbol_count =
			bfd_canonicalize_symtab(desc, entry->asymbols);
		if (Trace_symbol_list_root != (struct trace_symbol_list_node *) 0)
			check_symbols_for_traces(entry->symbol_count,
						 entry->asymbols);
		else if (Trace_all)
			dump_asymbols(entry->asymbols,entry->symbol_count,"");
	}
}


/*
 * turn this item into a reference 
 */
static void
refize(sp, nlist_p)
ldsym_type *sp;
asymbol **nlist_p;
{
	asymbol *sym = *nlist_p;
	sym->value = 0;
	sym->flags |= BSF_UNDEFINED;
	sym->section = (asection *)NULL;
	sym->udata =(PTR)( sp->srefs_chain);
	sp->srefs_chain = nlist_p;
}


/*
 * This function is called for each name which is seen which has a global
 * scope. It enters the name into the global symbol table in the correct
 * symbol on the correct chain. Remember that each ldsym_type has three
 * chains attatched, one of all definitions of a symbol, one of all
 * references of a symbol and one of all common definitions of a symbol.
 * 
 * When the function is over, the supplied is left connected to the bfd
 * to which is was born, with its udata field pointing to the next member
 * on the chain in which it has been inserted.
 * 
 * A certain amount of jigery pokery is necessary since commons come
 * along and upset things, we only keep one item in the common chain; the
 * one with the biggest size seen sofar. When another common comes along
 * it either bumps the previous definition into the ref chain, since it
 * is bigger, or gets turned into a ref on the spot since the one on the
 * common chain is already bigger. If a real definition comes along then
 * the common gets bumped off anyway.
 * 
 * Whilst all this is going on we keep a count of the number of multiple
 * definitions seen, undefined global symbols and pending commons.
 */
void
Q_enter_global_ref (nlist_p, assignment_statement, allocation_done)
    asymbol **nlist_p;
    int assignment_statement;
    lang_phase_type allocation_done;
{
    asymbol *sym = *nlist_p;
    CONST char *name = sym->name;
    ldsym_type *sp = ldsym_get (name);
    flagword this_symbol_flags = sym->flags;
    /* This helps us keep from counting multiple definitions as resolved
    globals */
    boolean just_saw_multi_def = false;
    extern boolean saw_decompression_table_symbol;
 
    /* FIXME: this is really cludgy. */
    if (!strcmp("__decompression_table",sp->name))
 	    saw_decompression_table_symbol = true;


    ASSERT(sym->udata == 0);

    if ( ! (bfd_get_file_flags(sym->the_bfd) & HAS_CCINFO) ){
	sp->non_ccinfo_ref = 1;
    }
    if ( (name[0] == '_') && (name[1] == '_') && (name[2] == '_') ){
	gnu960_enter_special_symbol( sym, sp, sym->the_bfd );
    }

    /* Just place onto correct chain */
    if (flag_is_common(this_symbol_flags)) {
	/* If we have a definition of this symbol already then
	 * this common turns into a reference. Also we only
	 * ever point to the largest common, so if we
	 * have a common, but it's bigger that the new symbol
	 * the turn this into a reference too.
	 */
	if (sp->sdefs_chain) {
	    /* This is a common symbol, but we already have a
	     * definition for it, so just link it into the ref
	     * chain as if it were a reference
	     */
	    refize(sp, nlist_p);
	} else if (sp->scoms_chain) {
	    /* If we have a previous common, keep only the biggest. */
	    if ( (*(sp->scoms_chain))->value > sym->value) {
		/* The common in the symbol table is bigger than the new one.
		   Throw the new one away (keeping the first). */
		refize(sp, nlist_p);
	    }
	    else if ((*(sp->scoms_chain))->value < sym->value) {
		/* The common in the symbol table is smaller than the new one.
		   Throw the existing one in the symbol table away (keeping the new one). */
		refize(sp, sp->scoms_chain);
		sp->scoms_chain = nlist_p;
	    }
	    else {
		/* The new common and the one in the symbol table are the same size.  */
		/* If the new symbol has debug information, then keep it and refize the
		   old one.  */
		if (this_symbol_flags & BSF_HAS_DBG_INFO) {
		    refize(sp, sp->scoms_chain);
		    sp->scoms_chain = nlist_p;
		}
		else
			/* Else, we just refize the old one. */
			refize(sp, nlist_p);
	    }
	}
	else {
	    /* This is the first time we've seen a common, so
	     * remember it - if it was undefined before, we know
	     * it's defined now
	     */
	    if (sp->srefs_chain) {
		undefined_global_sym_count--;
	    }
	    commons_pending++;
	    sp->scoms_chain = nlist_p;
	}
    }
    else if (flag_is_defined(this_symbol_flags)) {
	/* This is a definition of the symbol.
	   First, determine if it is a multi-definition. */

	if (sp->sdefs_chain) {  /* It was previously defined. */

	    /* Special case here for system calls:  The symbol is NOT multiply defined
	       unless certain circumstances exist.
	     */
		
	    if (((*(sp->sdefs_chain))->flags & BSF_HAS_SCALL) &&  /* It was previously defined
								 as a system call. */
		(this_symbol_flags & BSF_HAS_SCALL)) {         /* And it is currently being defined
							       as a system call. */
		/* It is a multiple definition of a system call if its sysproc value differs. */
		more_symbol_info newone,oldone,newestone;

		bfd_more_symbol_info(output_bfd,sym,&newone,bfd_sysproc_val);
		bfd_more_symbol_info(output_bfd,*(sp->sdefs_chain),&oldone, bfd_sysproc_val);
		if (newone.sysproc_value != -1 && oldone.sysproc_value != -1 &&
		    newone.sysproc_value != oldone.sysproc_value) {
		    info("Attempt to redefine sysproc value to %d from %d\n",
			 newone.sysproc_value,oldone.sysproc_value);
		    goto multi_def;
		}

		/* Guarantee that both the newone and the oldone have
		   the same sysproc index value. */
		newestone.sysproc_value = newone.sysproc_value != -1 ?
			newone.sysproc_value :  oldone.sysproc_value;
		bfd_more_symbol_info(output_bfd,sym,&newestone,
				     bfd_set_sysproc_val);
		bfd_more_symbol_info(output_bfd,*(sp->sdefs_chain),&newestone,
				     bfd_set_sysproc_val);

		/* Or it is a multiple definition if it was previously allocated to a
		   section and it currently is being allocated
		   to a section, and these sections are not the same. */
		if ((sym->section != 0) && ((*(sp->sdefs_chain))->section != 0) &&  
		    ((*(sp->sdefs_chain))->section != sym->section))
			goto multi_def;

		just_saw_multi_def = true;  /* When this flag is false,
					       it means 'we have just
					       resolved a referenced
					       external', and if the
					       flag is true, it means we
					       did not just resolve an
					       external. */

		/* Now, sysproc values are both equal and set to the
		   legitimate value or -1 if there is no legitimate
		   value yet.  Now, we figure out which one to keep
		   based on the section pointers: If one is set, then
		   use it else leave it undefined. */

		if (sym->section) {
		    /* The newone has a section definition.  So, remove
		       old definition, and replace it with the newone. */
		    refize(sp, sp->sdefs_chain);
		    sp->sdefs_chain = nlist_p;
		}
		else /* The oldone has the section, or neither has the
			section.  So, we toss the newone. */ 
		    refize(sp,nlist_p);
	    }
	    else if ((*(sp->sdefs_chain))->section != sym->section) {
		/* Possible multiple definition */
		asymbol *sy, **stat1_symbols, **stat_symbols;
		lang_input_statement_type *stat,*stat1;
multi_def:
		sy  = *(sp->sdefs_chain);
		if (sym->flags & BSF_ABSOLUTE) {
		    /* Override previous definition with absolute definition */
		    info("Overriding previous definition of %t\nwith new definition: %t\n",sy,sym);
		    refize(sp, sp->sdefs_chain);
		    sp->sdefs_chain = nlist_p;
		}
		stat = (lang_input_statement_type*)sy->the_bfd->usrdata;
		stat1 = (lang_input_statement_type *) sym->the_bfd->usrdata;
		stat1_symbols= stat1 ? stat1->asymbols: 0;
		stat_symbols = stat  ? stat->asymbols : 0;
		multiple_def_count++;
		just_saw_multi_def = true;
		info("%C: multiple definition of `%T'\n",
		     sym->the_bfd,
		     sym->section,
		     stat1_symbols,
		     sym->value,
		     sym);
	
		info("%C: first seen here\n",
		     sy->the_bfd,
		     sy->section,
		     stat_symbols,
		     sy->value);
	    }
	    else if (!sym->section &&
		     (!assignment_statement ||
		      (assignment_statement && allocation_done == lang_final_phase_enum &&
		       sp->assignment_phase == lang_final_phase_enum)) &&
		     (sym->value != (*(sp->sdefs_chain))->value))
		    goto multi_def;
	    else {
		/* The sections are totally equal.  We are redefining a symbol
		   to the exact same section or absolute symbol with the exact same
		   value.  (I believe .bss is the only section that
		   can do this).  Either case, we redefine the value here: */
		sym->udata =(PTR)( sp->sdefs_chain);
		sp->sdefs_chain = nlist_p;
		sp->assignment_phase = allocation_done;
		just_saw_multi_def = true;
	    }
	}
	else {   /*  It was not previously defined. */
	    sym->udata =(PTR)( sp->sdefs_chain);
	    sp->sdefs_chain = nlist_p;
	}
	/* A definition overrides a common symbol */
	if (sp->scoms_chain) {
	    refize(sp, sp->scoms_chain);
	    sp->scoms_chain = 0;
	    commons_pending--;
	}
	else if ((sp->srefs_chain) && !just_saw_multi_def) {
	    /* If previously was undefined, then remember as
	     * defined.
	     */
	    undefined_global_sym_count--;
	}
    }
    else {  /* if is not a common nor a definition it must be a reference
	       to an undefined. */
	if (sp->scoms_chain == (asymbol **)NULL 
	    &&  sp->srefs_chain == (asymbol **)NULL 
	    &&  sp->sdefs_chain == (asymbol **)NULL) {
	    /* And it's the first time we've seen it */
	    undefined_global_sym_count++;
	}
	refize(sp, nlist_p);
    }

    ASSERT(sp->sdefs_chain == 0 || sp->scoms_chain == 0);
    ASSERT(sp->scoms_chain ==0 || (*(sp->scoms_chain))->udata == 0);
}

static void
Q_enter_file_symbols (entry)
    lang_input_statement_type *entry;
{
	asymbol **q ;
	entry->common_section = bfd_make_section(entry->the_bfd, "COMMON");

	/* This is as good a time as any to mark the new, common 
	   section as a bss section. */
	entry->common_section->flags |= SEC_IS_BSS | SEC_ALLOC;

	if (option_v) {
                if (entry->just_syms_flag) {
                        fprintf(stdout,"%s (symbols only)\n",
                                entry->local_sym_name);
                }
                else {
                        fprintf(stdout,"%s\n",entry->local_sym_name);
                }
	}
        if (entry->just_syms_flag) {
                /* Just get the symbols, and generate for each an absolute
                symbol in the output file with the value of the input
                symbol's absolute address and a BSF_ABSOLUTE|BSF_EXPORT|
                BSF_GLOBAL type. */
 
                if ( entry->symbol_count > 0 ){
                        for (q = entry->asymbols; *q; q++) {
                                asymbol *p = *q;
 
                                if (flag_is_global(p->flags) &&
                                        (!(linker_generated_symbol(p->name)))) {
                                        asymbol *newsym;
 
                                        newsym = create_symbol(p->name,
                                                BSF_ABSOLUTE|BSF_EXPORT|BSF_GLOBAL,
                                                (asection *)NULL);
                                        newsym->value = p->value +
                                                (p->section ? p->section->vma : 0);
                                }
                        }
                }
                return;
        }


	ldlang_add_file(entry);
	total_files_seen++;
	if ( entry->symbol_count > 0 ){
		total_symbols_seen += entry->symbol_count;
		for (q = entry->asymbols; *q; q++) {
			asymbol *p = *q;

			if (flag_is_undefined_or_global_or_common(p->flags)) {
					Q_enter_global_ref(q,0,lang_first_phase_enum);
			}
			ASSERT(p->flags != 0);
		}
	}
}


/* Searching libraries */

struct lang_input_statement_struct *decode_library_subfile ();
void linear_library (), symdef_library ();

/* Search the library ENTRY, already open on descriptor DESC.
 * This means deciding which library members to load,
 * making a chain of `struct lang_input_statement_struct' for those members,
 * and entering their global symbols in the hash table.
 */
void
search_library (entry)
    struct lang_input_statement_struct *entry;
{

	/* No need to load a library if no undefined symbols */
	if (!undefined_global_sym_count) {
		return;
	}

	if (bfd_has_map(entry->the_bfd)) {
		symdef_library (entry);
	} else {
		linear_library (entry);
	}
}


boolean
gnu960_check_format (abfd, format)
bfd *abfd;
bfd_format format;
{
	return bfd_check_format(abfd,format);
}


void
ldmain_open_file_read_symbol (entry)
struct lang_input_statement_struct *entry;
{
	if (entry->asymbols == (asymbol **)NULL
	&& entry->real
	&& entry->filename) {

		ldfile_open_file (entry);
		if (gnu960_check_format(entry->the_bfd, bfd_object)) {
		    if (write_map) 
			    add_file_to_filelist(entry->just_syms_flag ?
						 dash_r_file : objfile,(char *)entry->filename,NULL,NULL);
#if 0
		    /* This is part of the fix for omega 585.  #ifdef'd out here
		       because it is only implemented partially, and produces spurious
		       warnings which are confusing. */
		    if (!entry->just_syms_flag &&
			((entry->the_bfd->flags & EXEC_P) &&
			 (0 == (entry->the_bfd->flags & HAS_RELOC))))
			    info("%P Warning: %B has EXEC property and may not relocate correctly.\n",
				 entry->the_bfd);
#endif

		    { extern char* gnu960_replace_object_file();

		      /* Replace entry's bfd with a version from the pdb. */
		      char* file = gnu960_replace_object_file(entry->the_bfd);

		      if (file)
		      { entry->the_bfd = 0;
			entry->filename = file;
			ldfile_open_file (entry);
			if (!entry->the_bfd || !gnu960_check_format (entry->the_bfd, bfd_object))
			  info ("%Fcorrupted or unreadable substitution object %s\n", file);
		      }
		    }

		    entry->the_bfd->usrdata = (PTR)entry;
		    Q_read_entry_symbols (entry->the_bfd, entry);
		    Q_enter_file_symbols (entry);
		} else if (gnu960_check_format(entry->the_bfd, bfd_archive)) {
		    if (write_map) 
			    add_file_to_filelist(archivefile,(char *)entry->filename,NULL,NULL);
		    entry->the_bfd->usrdata = (PTR)entry;
		    search_library (entry);
		} else {
			info("%F%B: malformed input file (not rel or archive)\n",
			    entry->the_bfd);
		}
	}
}


/* Construct and return a lang_input_statement_struct for a library member.
 * The library's lang_input_statement_struct is library_entry,
 * and the library is open on DESC.
 * SUBFILE_OFFSET is the byte index in the library of this member's header.
 * We store the length of the member into *LENGTH_LOC.
 */
lang_input_statement_type *
decode_library_subfile (library_entry, subfile_offset)
    struct lang_input_statement_struct *library_entry;
    bfd *subfile_offset;
{
        register struct lang_input_statement_struct *subentry;

        subentry = (struct lang_input_statement_struct *)
        ldmalloc ((bfd_size_type)(sizeof(struct lang_input_statement_struct)));

        memset( subentry, 0, sizeof(struct lang_input_statement_struct));

        subentry->filename = subfile_offset -> filename;
        subentry->local_sym_name  = subfile_offset->filename;
        subentry->the_bfd = subfile_offset;
        subentry->superfile = library_entry;
        subentry->just_syms_flag = false;
        subentry->loaded = false;
        return subentry;
}


static boolean subfile_wanted_p ();

void
clear_syms(entry, offset)
    struct lang_input_statement_struct *entry;
    file_ptr offset;
{
	carsym *car;
	unsigned long indx;

	indx = bfd_get_next_mapent(entry->the_bfd, BFD_NO_MORE_SYMBOLS, &car);
	while (indx != BFD_NO_MORE_SYMBOLS) {
		if (car->file_offset == offset) {
			car->name = 0;
		}
		indx = bfd_get_next_mapent(entry->the_bfd, indx, &car);
	}
}


/* Search a library that has a map
 */
void
symdef_library (entry)
     struct lang_input_statement_struct *entry;

{
  register struct lang_input_statement_struct *prev;

  boolean not_finished = true;

  /* Goto bottom of subfiles chain: */

  for (prev=entry->subfiles;prev && prev->chain;prev = prev->chain)
	  ;

  while (not_finished == true)
    {
      carsym *exported_library_name;
      bfd *prev_archive_member_bfd = 0;    

      int idx = bfd_get_next_mapent(entry->the_bfd,
				    BFD_NO_MORE_SYMBOLS,
				    &exported_library_name);

      not_finished = false;

      while (idx != BFD_NO_MORE_SYMBOLS  && undefined_global_sym_count)
	{

	  if (exported_library_name->name) 
	    {

	      ldsym_type *sp =  ldsym_get_soft (exported_library_name->name);

	      /* If we find a symbol that appears to be needed, think carefully
		 about the archive member that the symbol is in.  */
	      /* So - if it exists, and is referenced somewhere and is
		 undefined or */
	      if (sp && sp->srefs_chain && !sp->sdefs_chain)
		{
		  bfd *archive_member_bfd = bfd_get_elt_at_index(entry->the_bfd, idx);
		  struct lang_input_statement_struct *archive_member_lang_input_statement_struct;

		  archive_member_bfd->flags |= inhibit_optimize_calls ? SUPP_CALLS_OPT : 0;
		  archive_member_bfd->flags |= inhibit_optimize_branch ? SUPP_BAL_OPT : 0;
		  if (archive_member_bfd && gnu960_check_format(archive_member_bfd, bfd_object)) {

#if 0
		    /* This is part of the fix for omega 585.  #ifdef'd out here
		       because it is only implemented partially, and produces spurious
		       warnings which are confusing. */
		      if (archive_member_bfd->flags & EXEC_P)
			      info("%P Warning: %B has EXEC property and may not relocate correctly.\n",
				   archive_member_bfd);
#endif

		      /* Don't think carefully about any archive member
			 more than once in a given pass.  */

		      if (prev_archive_member_bfd != archive_member_bfd)
			{

			  prev_archive_member_bfd = archive_member_bfd;

			  /* Read the symbol table of the archive member.  */

			  if (archive_member_bfd->usrdata != (PTR)NULL) {

			    archive_member_lang_input_statement_struct =(lang_input_statement_type *) archive_member_bfd->usrdata;
			  }
			  else {

			    archive_member_lang_input_statement_struct =
			      decode_library_subfile (entry, archive_member_bfd);
			    archive_member_bfd->usrdata = (PTR) archive_member_lang_input_statement_struct;

			  }

	  if (archive_member_lang_input_statement_struct == 0) {
	    info ("%F%I contains invalid archive member %s\n",
		    entry,
		    sp->name);
	  }

			  if (archive_member_lang_input_statement_struct->loaded == false)  
			    {

			      Q_read_entry_symbols (archive_member_bfd, archive_member_lang_input_statement_struct);
			      /* Now scan the symbol table and decide whether to load.  */


			      if (subfile_wanted_p (archive_member_lang_input_statement_struct,entry->filename) == true)

				{
				  /* This member is needed; load it.
				     Since we are loading something on this pass,
				     we must make another pass through the symdef data.  */

				  extern char* gnu960_replace_object_file();
				
				  /* Replace entry's bfd with a version from the pdb, perhaps. */

				  char* file = gnu960_replace_object_file(archive_member_bfd);

				  if (file == 0) {
				    if (prev)
				      prev->chain = archive_member_lang_input_statement_struct;
				    else
				      entry->subfiles = archive_member_lang_input_statement_struct;

				    prev = archive_member_lang_input_statement_struct;
				    Q_enter_file_symbols (archive_member_lang_input_statement_struct);
				  }
				  else {
				    lang_input_statement_type *new_stat =
				      lang_add_input_file (file, lang_input_file_is_file_enum, NULL,NULL,-1,-1);

				    if (!new_stat->the_bfd)
				      ldfile_open_file (new_stat);

				    if (!new_stat->the_bfd || !gnu960_check_format (new_stat->the_bfd, bfd_object))
				      info ("%Fcorrupted or unreadable substitution object %s\n", file);

                                    new_stat->the_bfd->usrdata = (PTR) new_stat;

				    Q_read_entry_symbols(new_stat->the_bfd, new_stat);
				    Q_enter_file_symbols(new_stat);
				  }

				  not_finished = true;

				  /* Clear out this member's symbols from the symdef data
				     so that following passes won't waste time on them.  */
				  clear_syms(entry, exported_library_name->file_offset);
				  archive_member_lang_input_statement_struct->loaded = true;
				}
			    }
			}
		    }
		}
	    }
	  idx = bfd_get_next_mapent(entry->the_bfd, idx, &exported_library_name);
	}
    }
}

void
linear_library (entry)
struct lang_input_statement_struct *entry;
{
	boolean more_to_do = true;
	register struct lang_input_statement_struct *prev = 0;
	bfd *archive;
	register struct lang_input_statement_struct *subentry;

	while (more_to_do) {
		archive = bfd_openr_next_archived_file(entry->the_bfd,0);
		more_to_do = false;
		while (archive) {
			if (gnu960_check_format(archive, bfd_object)){
				subentry= decode_library_subfile(entry,archive);
				archive->usrdata = (PTR) subentry;
				if (!subentry) {
					return;
				}
				if ( !subentry->loaded) {
					Q_read_entry_symbols(archive,subentry);
					if (subfile_wanted_p(subentry,entry->filename)) {
						Q_enter_file_symbols (subentry);
						if (prev) {
							prev->chain = subentry;
						} else  {
							entry->subfiles = subentry;
						}
						prev = subentry;
						more_to_do = true;
						subentry->loaded = true;
					}
				}
			}
			archive =
			   bfd_openr_next_archived_file(entry->the_bfd,archive);
		}
	}
}



/* ENTRY is an entry for a library member.
 * Its symbols have been read into core, but not entered.
 * Return nonzero if we ought to load this member.
 */
static boolean
subfile_wanted_p (entry, libname)
struct lang_input_statement_struct *entry;
CONST char *libname;
{
  asymbol **q;

  for (q = entry->asymbols; *q; q++)
    {
      asymbol *p = *q;

      /* If the symbol has an interesting definition, we could
	 potentially want it.  */

      if (p->flags & BSF_FORT_COMM 
	  || p->flags & BSF_GLOBAL)
	{
	  register ldsym_type *sp = ldsym_get_soft (p->name);


	  /* If this symbol has not been hashed, we can't be looking for it.
	     Also, we don't want it if it is not defined also we do not want it
	     if it is defined in a defsym or linker directive file. */
	  if (sp && (sp->sdefs_chain == (asymbol **)NULL && !sp->defsym_flag &&
		     !sp->assignment_flag)) {
	    if (sp->srefs_chain || sp->scoms_chain)
	      {
		/* This is a symbol we are looking for.  It is either
		   not yet defined or common.  */

		if (flag_is_common(p->flags))
		  {
		    /* This libary member has something to
		       say about this element. We should 
		       remember if its a new size  */
		    /* Move something from the ref list to the com list */
		    if (sp->scoms_chain) {
		      /* Already a common symbol, maybe update it */
		      if (p->value > (*(sp->scoms_chain))->value) {
			(*(sp->scoms_chain))->value = p->value;
		      }
		    }
		    else {
		      /* Take a value from the ref chain
			 Here we are moving a symbol from the owning bfd
			 to another bfd. We must set up the
			 common_section portion of the bfd thing */

		      

		      sp->scoms_chain = sp->srefs_chain;
		      sp->srefs_chain =
			(asymbol **)((*(sp->srefs_chain))->udata);
		      (*(sp->scoms_chain))->udata = (PTR)NULL;

		      (*(  sp->scoms_chain))->flags = BSF_FORT_COMM | BSF_ALL_OMF_REP;
		      /* Remember the size of this item */
		      sp->scoms_chain[0]->value = p->value;
		      commons_pending++;
		      undefined_global_sym_count--;
		    } {
		      asymbol *com = *(sp->scoms_chain);
		      if (com && com->the_bfd && com->the_bfd->usrdata &&
			   (((lang_input_statement_type *)
			   (com->the_bfd->usrdata))->common_section ==
			   (asection *)NULL)) {
			((lang_input_statement_type *)
			 (com->the_bfd->usrdata))->common_section = 
			   bfd_make_section(com->the_bfd, "COMMON");
		      }
		    }
		    ASSERT(p->udata == 0);
		  }
	      
		else {
		    if (write_map)
			    add_file_to_filelist(archive_member,
                                                 (char *)entry->local_sym_name,
                                                 (char *)libname,
						 (char*)sp->name);
		  return true;
		}
	      }
	  }
	}
    }

  return false;
}
