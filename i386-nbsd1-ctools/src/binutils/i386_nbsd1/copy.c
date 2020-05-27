
#include "sysdep.h"
#include "bfd.h"
#include "getopt.h"
#include <string.h>

asymbol       **sympp;

/* Output symbol table asymbols. */

static asymbol **out_sympp;

char           *input_target = NULL;
char           *output_target = NULL;
char           *input_filename = NULL;
char           *output_filename = NULL;

static void     setup_sections();
static void     copy_sections();
static boolean  strip;
static boolean  strip_promoted;
static boolean  strip_ccinfo_only;
static boolean  verbose;
static boolean  copy_or_mv_file;   /* When false, we move the file, not copy. */
static boolean	dash_j;
static boolean	dash_J;
static void     preserve_time_stamp();

/* IMPORTS */
extern char    *program_name;
extern char *xmalloc();

typedef enum prog_type_tag { unknown, stripper, copier, converter } prog_type;
prog_type prog_kind = stripper;

static char *prog_str[] =
{
	"",
	"Stripper",
	"Copier/Converter",
	"Copier/Converter"
};
		
static int out_endian_specified = false;      /* endian specifier present   */
static int out_big_endian;                    /* big endian output selected */
static int reverse_flag;                      /* reverse endianess          */
static int suppress_time_stamp;               /* reverse endianess          */

int out_format_specified = false;             /* b.out or coff specified    */
enum target_flavour_enum out_format;

static int leave_only_block_info = false;     /* strip all, retain only block info */
static int leave_only_externals = false;      /* strip all, retain externals & statics */
static int strip_lines_only = false;          /* strip line info ONLY */
static int leave_only_relocation = false;     /* strip all, except relocation info */
static int partial_strip = false;             /* only subset of symbols are stripped */
static int convert_in_place = false;          /* accept *.o and convert every file in place,
                                               * without having to specify an output
                                               */
static int conversion_opt_specified = false;   /* No conversion option specified
					       */
static struct option long_options[] = { { 0,0,0,0 } }; 

PROTO( static char *, pick_target,  (bfd *abfd) );
PROTO( static void, strip_promoted_syms,
      (asymbol **symppp, unsigned int isymcount, bfd *ibfd, bfd *obfd) );
PROTO( static void, strip_symbols,
      (asymbol **symppp, unsigned int isymcount, bfd *ibfd, bfd *obfd) );
#if 0
PROTO( static void, update_reloc_sym_ptrs,
      (bfd *ibfd, sec_ptr isec, bfd *obfd) );
#endif


#ifdef DOS
#define SLASH '\\'
#else
#define SLASH '/'
#endif

static
char *objcopy_usage[] = {
	"",
	"Copy infile to outfile, performing any transformations indicated by",
	"optional switches.  If outfile is omitted, infile is modified in place.",
	"Byte-order and format describe the output file.",
	"",
	"Byte-order --  one of:",
	"",
	"    -b    big-endian",
	"    -c    Copy the file instead of moving it.",
	"    -l    little-endian",
	"    -h    same byte order as host on which we're running",
        "    -r    convert from little-endian to big-endian",
        "    -z    suppress writing a time-stamp in the COFF file header",
	"",
 	"    default: same byte order as infile",
	"",
	"Format -- one of:",
	"",
	"    -Fcoff  Intel COFF",
	"    -Felf   ELF",
	"    -Fbout  b.out",
	"",
	"    default: same object file format as input file",
	"",
	"    CHANGING FILE FORMAT WILL CAUSE THE OUTPUT FILE'S SYMBOL TABLE TO BE TRUNCATED.",
	"",
	"Options:",
	"    -C      Strip CCINFO from output.",
	"    -J      Coff files only.  Compress symbol table, merge dup tags, compress string table.",
	"    -p      Convert files specified in place. Input is also the output.",
	"    -q      Do not print information about what's being done. ",
	"    -S      Strip relocations and symbol table info from output.",
	"    -x      Strip global symbols which were promoted from locals.",
	"    -v      Print information about what's being done.",
	"    -V      Display version and perform transformation",
	"    -v960   Display version information and do nothing else.",
	"    -help   Display this help message.",
	"",
	"See your user's guide for a complete command-line description",
	"",
	NULL
};

static 
char *stripper_usage[] = {
	"",
	"Remove symbols and relocations from an object file",
	"",
	"Options:",
	"    -a      Strip all symbols. This is the default.",
        "    -b      Strip all locals symbols and line numbers but leave block info.",
	"    -C      Strip CCINFO ONLY.",
        "    -h      Display this help message.",
        "    -l      Strip line information only.",
        "    -r      Strip all but leave relocation information.",
        "    -x      Strip all but leave static and external information.",
	"    -v960   Display version information and do nothing else.",
	"    -V      Display version and perform transformation.",
        "    -z      Suppress writing a time-stamp in the COFF file header",
	"",
	"See your user's guide for a complete command-line description",
	"",
        NULL
};

static
void
make_same_modes(input_filename,output_filename)
    char *input_filename,*output_filename;
{
    struct stat st;

    if (!stat(input_filename,&st)) {
#ifdef DOS
	int new_mode = st.st_mode & (S_IREAD | S_IWRITE | S_IEXEC);
#else
	mode_t new_mode = st.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
#endif

	chmod(output_filename,new_mode);
    }
}

static
void
cp_or_mv_file(from,to,copyit)
    char *from,*to;
    int copyit;
{
    if (copyit) {
#ifdef DOS
	FILE *fin = fopen(from,"rb");
	FILE *fout = fopen(to,"wb");
#else
	FILE *fin = fopen(from,"r");
	FILE *fout = fopen(to,"w");
#endif
	char buff[512];
	int n;
#define _CHECK(x,y) if (!x) { fprintf(stderr,"Can not open %s?\n",y); perror(y); exit(1); }
	_CHECK(fin,from);
	_CHECK(fout,to);
	while ((n=fread(buff,1,512,fin)) > 0) {
	    fwrite(buff,1,n,fout);
	}
	fclose(fin);
	fclose(fout);
	make_same_modes(from,to);
	unlink(from);
    }
    else {
#ifdef DOS
	/* The rename function in DOS differs from UNIX.  
	 * Therefore, we must first ensure that the file doesn't exist.   
	 */  
	if (access(to,CHECK_FILE_EXISTENCE) == FILE_EXISTS) {
	    if (remove(to)) {
		fprintf(stderr, "ERROR: can't write over file %s\n",to);
		exit(1);
	    }
	}
#endif
	rename(from,to);
    }
}
	
static
void            
usage( which )
	prog_type which;
{
	char **p;

	if (which == stripper) {
		fprintf(stderr, "\nUsage %s [options] objfile ...\n", program_name);
		paginator(stripper_usage);
	} else {
		fprintf(stderr,
			"\nUsage: %s [ byte-order ] [ format ] [options] infile [outfile]\n",
			program_name);
		paginator(objcopy_usage);
	}
}


check_help( argc, argv, which )
    int argc;
    char *argv[];
    prog_type which;
{
	int i;
	for ( i = 1; i < argc; i++ ){
                if ( !strcmp(argv[i],"-help")
#ifdef DOS
                        || !strcmp(argv[i], "/help")
#endif
              ) {
			usage(which);
                       	exit (0);
               	} 
        }
}


/* Create a temp file in the same directory as supplied */
static
char *
make_tempname(filename)
char *filename;
{
	static char template[] = "stXXXXXX";
	char *tmpname;
	char *      slash = strrchr( filename, SLASH );
	if (slash != (char *)NULL){
		*slash = 0;
		tmpname = xmalloc(strlen(filename) + sizeof(template) + 1 );
		strcpy(tmpname, filename);
#ifdef DOS
		strcat(tmpname, "\\" );
#else
		strcat(tmpname, "/" );
#endif
		strcat(tmpname, template);
		mktemp(tmpname );
		*slash = '/';
	} else {
		tmpname = xmalloc(sizeof(template));
		strcpy(tmpname, template);
		mktemp(tmpname);
	}
	return tmpname;
}

static int changing_omfs;

/*
 * All the symbols have been read in and point to their owning input section.
 * They have been relocated to that they are all relative to the base of
 * their owning section. On the way out, all the symbols will be relocated to
 * their new location in the output file, through some complex sums.
 */
static void
mangle_sections(ibfd, obfd)
    bfd *ibfd;
    bfd *obfd;
{
	asection *sec;

	for (sec = ibfd->sections; sec != NULL; sec = sec->next) {
	    if ((changing_omfs || strip || leave_only_externals || leave_only_relocation) &&
		(sec->flags & SEC_IS_DEBUG))
		    continue;
	    else {
		sec->output_section = bfd_get_section_by_name(obfd, sec->name);
		sec->output_section->output_section = sec->output_section;
		sec->output_offset = 0;
	    }
	}
}

static
void
say_target( abfd )
    bfd *abfd;
{
	printf( "(%s-endian %s)",
	    BFD_BIG_ENDIAN_FILE_P(abfd) ? "big" : "little",
	    BFD_COFF_FILE_P(abfd) ? "COFF" : BFD_ELF_FILE_P(abfd) ? "ELF" : "b.out" );
}


/* The following support copying of GNU/960 2-pass compiler optimization
 * info.  'ccinfop' is a pointer to a dynamically allocated memory block
 * containing the ccinfo read from the input file. 'ccinfo_len' is the number
 * of bytes in the block.  'write_ccinfo' is a "callback" routine that will
 * be invoked from BFD at file output time, when the bfd is positioned at
 * the correct place in the output file for the ccinfo.
 */
static char *ccinfop = NULL;
static int   ccinfo_len = 0;

void
write_ccinfo( abfd )
    bfd *abfd;
{
	if (ccinfop) {
		/* Blast it all out and free memory */
		if (bfd_write(ccinfop, 1, ccinfo_len, abfd) != ccinfo_len) {
			bfd_fatal(abfd->filename);
		}
		free( ccinfop );
		ccinfop = NULL;
		ccinfo_len = 0;
	}
}

/*
 * verbose_message() 
 *        Use for displaying info when -v is specified
 */
static
void
verbose_message(ibfd)
bfd *ibfd;
{
	char *descp;

	if (strip)
		descp = "strip";
	else if (strip_promoted)
		descp = "strip promoted locals from";
	else if (output_filename)
		descp = "copy";
	else if (!conversion_opt_specified)
		descp = "file byte order of";
	else
		descp = "convert";
	printf("%s %s ", descp, ibfd->filename);
	say_target( ibfd );
}

static 
void
copy_object(ibfd, obfd)
bfd *ibfd;
bfd *obfd;
{
	unsigned int symcount;

 	if (strip_ccinfo_only)
 		ibfd->flags &= (~(HAS_CCINFO));

	if (!bfd_set_format(obfd, bfd_get_format(ibfd)))
		bfd_fatal(output_filename);

	if (verbose){
		verbose_message(ibfd);

		if ( output_filename || (ibfd->xvec != obfd->xvec) ){
			printf(" to %s",
					output_filename ? obfd->filename : "" );
			say_target( obfd );
		}
		printf("\n");
	}

	if (!bfd_set_start_address(obfd, bfd_get_start_address(ibfd)))
		bfd_fatal(bfd_get_filename(ibfd));

    {
        /* If we are stripping the file, then the flags of the output file should match
           the flags of the input file, with the symbol flags removed.
           If we are not stripping the file, then we are copying the file verbatim, or may
	   be changing the endianness of the file.  In this case, we retain all of the
           input file's flags. */

	flagword inflags = bfd_get_file_flags(ibfd);
#define SYMFLAGS (HAS_RELOC|HAS_LINENO|HAS_DEBUG|HAS_SYMS|D_PAGED|HAS_LOCALS|HAS_CCINFO|SYMTAB_COMPRESSED)
        flagword f = (strip) ? (inflags & ~SYMFLAGS) : inflags;

	
	if (!bfd_set_file_flags(obfd,f)) {
	    bfd_fatal(bfd_get_filename(ibfd));
	}
	if (strip || leave_only_externals || leave_only_relocation)
		obfd->flags |= STRIP_LINES;
    }

	/* Copy architecture of input file to output file */
	if (!bfd_set_arch_mach(obfd, bfd_get_architecture(ibfd), bfd_get_machine(ibfd),
			       bfd_get_target_arch(ibfd))) {
		fprintf(stderr, "Output file cannot represent architecture %s\n",
		    bfd_printable_arch_mach(bfd_get_architecture(ibfd),
		    bfd_get_machine(ibfd)));
	}

	bfd_set_target_attributes(obfd, bfd_get_target_attributes(ibfd));

	if (!bfd_set_format(obfd, bfd_get_format(ibfd))) {
		bfd_fatal(ibfd->filename);
	}

	if (suppress_time_stamp) 
		obfd->flags |= SUPP_W_TIME;
	else 
		preserve_time_stamp(ibfd, obfd);

	if (dash_J)
		obfd->flags |= STRIP_DUP_TAGS;
	
	if (prog_kind == stripper || dash_j)
		obfd->flags |= DO_NOT_STRIP_ORPHANED_TAGS;

	if (!changing_omfs) {
	    ibfd->flags |= DO_NOT_ALTER_RELOCS;
	    obfd->flags |= DO_NOT_ALTER_RELOCS;
	}

	if ( strip != true ) {
	    sympp = (asymbol **) xmalloc(get_symtab_upper_bound(ibfd));
	    symcount = bfd_canonicalize_symtab(ibfd, sympp);
	}

	/* bfd mandates that all output sections be created and sizes set
	 * before any output is done.  Thus, we traverse all sections twice.
	 * Here, to setup the sections, and later to set the section's contents.
	 */
	bfd_map_over_sections(ibfd, setup_sections, (void *) obfd);
	mangle_sections(ibfd, obfd);

	if (strip != true) {
	    if ( bfd_get_file_flags(ibfd) & HAS_CCINFO ){
		/* If input file has ccinfo, allocate memory and read it.
		 * Notify BFD to call us back (at write_ccinfo()) at output
		 * time.
		 */
		ccinfo_len = bfd960_seek_ccinfo( ibfd );
		if (ccinfo_len <= 0)
			bfd_fatal(ibfd->filename);
		ccinfop = xmalloc( ccinfo_len );
		if (bfd_read(ccinfop,1,ccinfo_len,ibfd) != ccinfo_len)
			bfd_fatal(ibfd->filename);
		bfd960_set_ccinfo( obfd, write_ccinfo );
		if (BFD_ELF_FILE_P(obfd)) {
		    asection *ccinfo_sect = bfd_make_section(obfd,".960.intel.ccinfo");
		    ccinfo_sect->flags |= SEC_IS_CCINFO | SEC_HAS_CONTENTS;
		    ccinfo_sect->size = ccinfo_len;
		    ccinfo_sect->output_section = ccinfo_sect;
		}
	    }
	    out_sympp = (asymbol **) xmalloc(get_symtab_upper_bound(ibfd));
	    symcount = bfd_canonicalize_symtab(ibfd, out_sympp);
	}
	else {
	    symcount = 0;
	    out_sympp = bfd_set_symtab(obfd, (asymbol **) 0, &symcount);
	}

#if 1
/* THIS CODE WAS MOVED UP TO SEE IF THE CODE COULD WORK IN THIS POSITION. */

        /* If option strip_promoted is selected,
         * then remove any local variables that were promoted
         * to globals by gcc960.
         */
        if ( strip_promoted ) { 
                strip_promoted_syms(out_sympp, symcount, ibfd, obfd);
        } else if (partial_strip && ! strip_ccinfo_only) {
		strip_symbols(out_sympp, symcount, ibfd, obfd);
	}
	else
		out_sympp = bfd_set_symtab(obfd, out_sympp, &symcount);
#endif
	bfd_map_over_sections(ibfd, copy_sections, (void *) obfd);

#if 0
        /* If option strip_promoted is selected,
         * then remove any local variables that were promoted
         * to globals by gcc960.
         */
        if ( strip_promoted ) { 
                strip_promoted_syms(out_sympp, symcount, ibfd, obfd);
        } else if (partial_strip && ! strip_ccinfo_only) {
		strip_symbols(out_sympp, symcount, ibfd, obfd);
	}
#endif
}

/*
 * copy_unknown_object - as the name implies this copies non-bout
 *         and non-coff files. This is necessary to allow text files
 *         on coff archive files.
 */

#ifdef __HIGHC__  /* Metaware recommends 4K of stack/buf */
#define BUFSIZE 3072 /* however, let's be conservative */
#else
#define BUFSIZE 8192
#endif

static 
void copy_unknown_object(ibfd, obfd) 
	bfd  *ibfd;
	bfd  *obfd;
{
	struct stat Fstat;
	off_t rwlength;
	size_t readlen;
	char buffer[BUFSIZE];

	bfd_set_format(obfd, bfd_unknown); 

	if (verbose){
		printf("WARNING: Archive member %s not 80960 binary\n", ibfd->filename);
	}

	bfd_stat_arch_elt(ibfd, &Fstat); 
	rwlength = Fstat.st_size; 

	/* now start copying ... */
	while (rwlength) 
	{
	        if ((readlen = rwlength) > sizeof(buffer))
			readlen = sizeof(buffer);
		bfd_read(buffer, 1, readlen,  ibfd );
		if (bfd_write(buffer, 1, readlen, obfd) < readlen ) 
		{
			bfd_fatal(obfd->filename);
		}
		rwlength -= readlen;
	}
	/* ensure file status is updated */
	fflush((FILE *)obfd->iostream);
}

/*
 * strstr_x - find first occurrence of substring wanted in string s
 *
 * Version of ANSI C strstr.
 * This function is included here because it is not supported by cc
 * for all hosts.
 */
char *				/* found string, or NULL if none */
strstr_x(s, wanted)
        char *s;
        char *wanted;
{
	register char *scan;
	register int len;
	register char firstc;

	/*
	 * The odd placement of the two tests is so "" is findable.
	 * Also, we inline the first char for speed.
	 * The ++ on scan has been moved down for optimization.
	 */
	firstc = *wanted;
	if (firstc == 0)
	        return (char *)s;	/* as per ANSI */
	len = strlen(wanted);
	for (scan = s; *scan != firstc || strncmp(scan, wanted, len) != 0; )
		if (*scan++ == '\0')
			return(NULL);
	return((char *)scan);
}



/*
 *  Array of special substrings of symbols with the '.' character which
 *  must be retained.
 */
static char *sym_special[] =
{
    "gcc_compiled.",
    "gcc2_compiled.",
    "ic_name_rules.",
    ".text",
    ".data",
    ".bss",
    NULL
};

static char *sym_block[] =
{
	".bb",
	".eb",
	".bf",
	".ef",
	NULL
};

/* strip_symbols
 *
 * Strip symbols per the specified options
 *
 */
static
void
strip_symbols(sympp, isymcount, ibfd, obfd)
    asymbol **sympp;                   /* pointer to the main pointer */
    unsigned int isymcount;            /* input symbol count */
    bfd *ibfd;
    bfd *obfd;
{
    asymbol **ipp;                      /* input array of symbol pointers */
    asymbol **osympp, **opp;            /* output array of symbol pointers */
    unsigned int osymcount = 0;         /* output symbol count */
    unsigned int i,j;
    boolean keep = false;            /* true = keep symbol */
    char *s, *sdot;
    unsigned int sym_upper;

    /* 
     *  Loop thru symbol entries, removing promoted locals.
     */
    ipp = sympp;
    sym_upper = get_symtab_upper_bound(ibfd);
    opp = osympp = (asymbol **) xmalloc(sym_upper);

#define KEEP_FLAGS (BSF_GLOBAL|BSF_UNDEFINED|BSF_FORT_COMM|BSF_KEEP|BSF_LOCAL)

    for (i=0; i < isymcount; i++, ipp++) {
	    flagword sflags = (*ipp)->flags;

            s = (char *)bfd_asymbol_name(*ipp);
	    if (s == NULL) {
                *ipp = NULL;            /* no symbol name - remove */
                continue;
            }

	    if ( (sflags == BSF_DEBUGGING) || (sflags == BSF_LOCAL) ) { 
		    keep = true; /* always keep debugging and unknown-flagged
                                  * symbols.
                                  */
	    }
	    if (leave_only_externals || leave_only_relocation) {
		    keep |= sflags & KEEP_FLAGS ? true : false;
		    if (keep == true) { /* remove .bf, .eb, .bf, .ef symbols, as well */
    			    for (j = 0; sym_block[j] != NULL; j++) {
				    if (strstr_x(s, sym_block[j]) != NULL) {
					    keep = false;
					    break;
				    }
			    }
		    }
	    }
	    if (leave_only_block_info) {
		    keep |= sflags & KEEP_FLAGS ? true : false;
		    if (keep == false) {
			    for (j = 0; sym_block[j] != NULL; j++) {
				    if (strstr_x(s, sym_block[j]) != NULL) {
					    keep = true;
					    break;
				    }
			    }
		    }
	    } 

	    /*
	     *  sdot != NULL  means that sybmol name has a '.'
	     *  *s != '_'     means that symbol is not derived from a
	     *                      C language variable.
	     *  Note that the compiler may have generated its own
	     *  local symbol names which do not begin with '_'.
	     */
	    sdot = strchr(s, '.');
	    if (   (sdot != NULL) && (*s != '_') && (keep == false) ) {
		    for (j = 0; sym_special[j] != NULL; j++) {  
			    if ( strstr_x(s, sym_special[j]) != NULL) {
				    keep = true;
				    break;
			    }
		    }
	    }

	    if ((strip || partial_strip) &&
		BFD_ELF_FILE_P(obfd) &&
		BFD_ELF_FILE_P(ibfd) &&
		(sflags & BSF_UNDEFINED) &&
		(0 == (sflags & BSF_SYM_REFD_OTH_SECT))) {
		keep = false;
	    }
	    if ( (keep == true) || (strip_lines_only == true) ) {
		    keep = false;
		    *opp = *ipp;        /* put it in output array */
		    osymcount++;
		    /*
		     *  Modify the input (original) array of pointers
		     *  so that it can be used to update relocation entries.
		     *  If symbol is retained, then point to the correct
		     *  pointer in output array.
		     */
		    *ipp = (asymbol *) opp++;
	    } else {
		    *ipp = NULL;        /* symbol removed */
	    }
    }
    *opp = NULL; /* last place in array */

    /*
     *  Update variables for output.
     */
    out_sympp = bfd_set_symtab(obfd, osympp, &osymcount);

    /* zero line info of obfd->outsymbols */
    /* Currently only works for coff. */
    bfd_zero_line_info(obfd);

#if 0
    /*
     *  Update symbol pointers (sym_ptr_ptr) in relocation entries
     *  for all sections.
     */
    bfd_map_over_sections(obfd, update_reloc_sym_ptrs, (void *) NULL);
#endif
}
/*
 *  strip_promoted_syms
 * 
 *  Remove any local variables that were promoted to globals by gcc960.
 *
 *  This function is called after the output bfd has been completely
 *  updated from the input bfd.
 *  A new array of pointers to symbols is used for output.
 *  The original array of pointers to symbols is modified so that it
 *  can be used for updating relocation entries that are based on symbols.
 *  bfd_set_symtab is called to use the new count of symbols and the new
 *  array of pointers to symbols.
 */
static
void
strip_promoted_syms(sympp, isymcount, ibfd, obfd)
    asymbol **sympp;                   /* pointer to the main pointer */
    unsigned int isymcount;            /* input symbol count */
    bfd *ibfd;
    bfd *obfd;
{
    asymbol **ipp;                      /* input array of symbol pointers */
    asymbol **osympp, **opp;            /* output array of symbol pointers */
    unsigned int osymcount = 0;         /* output symbol count */
    unsigned int i;
    unsigned int j;
    boolean keep = false;            /* true = keep symbol */
    char *sdot;
    char *s;
    unsigned int sym_upper;
    
    /* 
     *  Loop thru symbol entries, removing promoted locals.
     */
    ipp = sympp;
    sym_upper = get_symtab_upper_bound(ibfd);
    opp = osympp = (asymbol **) xmalloc(sym_upper);
    for (i=0; i < isymcount; i++, ipp++) {
            s = (char *)bfd_asymbol_name(*ipp);
            if (s != NULL)
                sdot = strchr(s, '.');
            else {
                *ipp = NULL;            /* no symbol name - remove */
                continue;
            }

            if ((*ipp)->flags & BSF_DEBUGGING)
                keep = true;            /* keep all debug symbols */

            /*
             *  sdot != NULL  means that sybmol name has a '.'
             *  *s != '_'     means that symbol is not derived from a
             *                      C language variable.
             *  Note that the compiler may have generated its own
             *  local symbol names which do not begin with '_'.
             */
            if (   (sdot != NULL)
                && (*s != '_')
                && (keep == false) ) {
                    for (j = 0; sym_special[j] != NULL; j++) {  
                            if ( strstr_x(s, sym_special[j]) != NULL) {
                                    keep = true;
                                    break;
                            }
                    }
		    for (j = 0;!keep && sym_block[j] != NULL;j++) {
			if ( strstr_x(s, sym_block[j]) != NULL) {
			    keep = true;
			    break;
			}
		    }
            }
            if ((sdot == NULL) || (keep == true)) {
                    keep = false;
                    *opp = *ipp;        /* put it in output array */
                    osymcount++;
                    /*
                     *  Modify the input (original) array of pointers
                     *  so that it can be used to update relocation entries.
                     *  If symbol is retained, then point to the correct
                     *  pointer in output array.
                     */
                    *ipp = (asymbol *) opp++;
            }
            else {
                    *ipp = NULL;        /* symbol removed */
            }
    }
    *opp = NULL;                        /* last place in array */

    /*
     *  Update variables for output.
     */
    out_sympp = bfd_set_symtab(obfd, osympp, &osymcount);

#if 0
    /*
     *  Update symbol pointers (sym_ptr_ptr) in relocation entries
     *  for all sections.
     */
    bfd_map_over_sections(obfd, update_reloc_sym_ptrs, (void *) NULL);
#endif

}

#if 0

If anyone can tell me why we need this code, I am very anxious to
hear why.

Paul Reger

/*
 *  update_reloc_sym_ptrs
 *
 *  Update symbol pointers (sym_ptr_ptr) in relocation entries
 *  for the given section.
 */
static 
void
update_reloc_sym_ptrs(obfd, osec, dummy)
    bfd            *obfd;
    sec_ptr         osec;
    void           *dummy;              /* not used */
{
        arelent        *relp;
	unsigned int    relcount;
        unsigned int    i;
        /* pointers to array of pointers to symbols */
        asymbol       **spp, **ospp;    

	if (get_reloc_upper_bound(obfd, osec) == 0) {
                return;
        }
        relcount = osec->reloc_count;
        if (relcount) {                 /* if any relocation entries */
                relp = *(osec->orelocation);
                for (i=0; i < relcount; i++, relp++) {
                        /*  sym_ptr_ptr points to the original
                         *  array of pointers to symbols.
                         *  The original array of pointers now
                         *  contains pointers into the output array of
                         *  pointers to symbols.
                         *  So, if it is non-NULL, then update
                         *  sym_ptr_ptr with the value from the output array.
                         */
                        spp = relp->sym_ptr_ptr;
                        if (spp != NULL) {
                                ospp = (asymbol **) *spp;
#ifdef DEBUG
                                /*  A NULL value in the original array
                                 *  indicates that a symbol was removed.
                                 *  If sym_ptr_ptr is not NULL and
                                 *  points to a NULL pointer,
                                 *  then we removed a symbol that we shouldn't
                                 *  have.
                                 */
                                if (ospp == NULL)
                                        printf("Strip caused invalid relocation entry\n");
#endif
                                relp->sym_ptr_ptr = ospp;
                        }
                }
        }
}
#endif


static
char *
cat(a,b,c)
char *a;
char *b;
char *c;
{
	int size = strlen(a) + strlen(b) + strlen(c);
	char *r = xmalloc(size+1);
	strcpy(r,a);
	strcat(r,b);
	strcat(r,c);
	return r;
}

static void
preserve_time_stamp (ibfd, obfd)
	bfd *ibfd;
	bfd *obfd;
{
	obfd->mtime_set = 1;
	obfd->mtime = ibfd->mtime;
}

static void 
copy_archive(ibfd, obfd)
bfd *ibfd;
bfd *obfd;
{
    bfd **ptr = &(obfd->archive_head);
    struct bfd_list {
	bfd *abfd;
	struct bfd_list *next_bfd;
    } *first_bfd = 0,**bfd_ptr = &first_bfd;

    bfd *cur_bfd;
    int i = 0;

    /* Read each archive element in turn from the input, copy the
     * contents to a temp file, and keep the temp file handle
     */
    char *dir = make_tempname("");

    if (!bfd_set_format(obfd, bfd_get_format(ibfd)))
	    bfd_fatal(output_filename);

    /* Make a temp directory to hold the contents */
    /* FIX ME!! Should add error checking here issue an internal error if the mkdir
       fails. */
#ifdef DOS	/* MetaWare 3.2 only uses one arg */
    mkdir(dir);
#else
    mkdir(dir, 0777);
#endif	
    obfd->has_armap = ibfd->has_armap;
    if (!changing_omfs)
	    ibfd->flags |= DO_NOT_ALTER_RELOCS;

    cur_bfd = bfd_openr_next_archived_file(ibfd, NULL);
    if (!changing_omfs)
	    cur_bfd->flags |= DO_NOT_ALTER_RELOCS;

    ibfd->archive_head = cur_bfd;


    while (cur_bfd) {
	/* Create an output file for this member */
	char *output_name = cat(dir, "/",cur_bfd->filename);
	bfd *output_bfd;
	/* Note that we MUST interrogate the format BEFORE picking the output
	   target due to the fact that the interrogation decorates the bfd
	   with information needed by pick_target(). */
	int it_is_an_object = bfd_check_format(cur_bfd, bfd_object);


	output_target = pick_target(cur_bfd);
	output_bfd = bfd_openw(output_name, output_target);
	if (output_bfd == (bfd *)NULL)
		bfd_fatal(output_name);

	if (suppress_time_stamp) 
		output_bfd->flags |= SUPP_W_TIME;
	else
		preserve_time_stamp(cur_bfd, output_bfd);

	if (leave_only_externals || leave_only_relocation)
		output_bfd->flags |= STRIP_LINES;

	if (dash_J)
		output_bfd->flags |= STRIP_DUP_TAGS;

	if (prog_kind == stripper || dash_j)
		output_bfd->flags |= DO_NOT_STRIP_ORPHANED_TAGS;
		
	if (it_is_an_object)
		copy_object(cur_bfd, output_bfd);
	else
		copy_unknown_object(cur_bfd, output_bfd);

	bfd_close(output_bfd);

	/* Now open the newly output file and attatch to
	 * our list
	 */

	output_bfd = bfd_openr(output_name, output_target);
	if (output_bfd == (bfd *)NULL)
		bfd_fatal(output_name);
	/* Include it in the archive chain: */
	*ptr = output_bfd;
	ptr = &(output_bfd->next);
	
	if (suppress_time_stamp) 
		output_bfd->flags |= SUPP_W_TIME;
	else
		preserve_time_stamp(cur_bfd, output_bfd);
	
	/* Mark it for deletion */
	if (1) {
	    struct bfd_list *p = (struct bfd_list *) xmalloc(sizeof(struct bfd_list));
	    
	    p->abfd = output_bfd;
	    *bfd_ptr = p;
	    bfd_ptr = &(p->next_bfd);
	}
	
	cur_bfd->next = bfd_openr_next_archived_file(ibfd,cur_bfd);
	
	if ((cur_bfd = cur_bfd->next) && !changing_omfs)
		cur_bfd->flags |= DO_NOT_ALTER_RELOCS;
    }
    *bfd_ptr = 0;
    *ptr = 0;
    
    if (suppress_time_stamp) 
	    obfd->flags |= SUPP_W_TIME;
    else
	    preserve_time_stamp(ibfd, obfd);
    
    if (!bfd_close(obfd)){
	bfd_fatal(output_filename);
    }
    
    /* Now delete all the files that we opened.
     * Construct their names again, unfortunately, but so what;
     * we're about to exit anyway.
     */
    for (;first_bfd;first_bfd = first_bfd->next_bfd) {
	CONST char *name = first_bfd->abfd->filename;
	
	bfd_close(first_bfd->abfd);
	unlink(name);
    }
    
    /* The following code is due to a bug in some implementations
       of NFS.  NFS sometimes creates hidden, volatile .nfs* files
       in the temporary directory that we have created.  We can not
       remove those files, sync() wont get rid of them either, rm
       dumps core trying to remove them.  So, we are left with
       leaving it around and being silent, or issuing this rude
       message.  We feel the least of two evils is for us to issue
       this message. */
    
    do {
	rmdir(dir);
	i++;
    } while (i <= 3 && errno);
    
    if (errno) {
	fprintf(stderr, "WARNING: Failed to remove ");
	perror(dir);
    }
    
    if (!bfd_close(ibfd)){
	bfd_fatal(input_filename);
    }
}

static
boolean
copy_file(input_filename, output_filename)
    char           *input_filename;
    char           *output_filename;
{
	bfd            *ibfd;

	ibfd = bfd_openr(input_filename, input_target);
	if (ibfd == NULL)
		bfd_fatal(input_filename);


	if (bfd_check_format(ibfd, bfd_object)) {
		bfd * obfd;

		output_target = pick_target(ibfd);
		obfd = bfd_openw(output_filename, output_target);
		if (obfd == NULL){
			bfd_fatal(output_filename);
		}
		if ( !strip && (ibfd->xvec->flavour != obfd->xvec->flavour) ){
		    fprintf(stderr,
			    "%s: object format change: truncating symbols\n",
			    program_name );
		    changing_omfs = 1;
		}

		copy_object(ibfd, obfd);

		if (!bfd_close(obfd)) {
			bfd_fatal(output_filename);
		}
		if (!bfd_close(ibfd)) {
			bfd_fatal(input_filename);
		}
	} else if (bfd_check_format(ibfd, bfd_archive)) {
		bfd * obfd;

		output_target = pick_target(ibfd);
		obfd = bfd_openw(output_filename, output_target);
		if (obfd == NULL){
			bfd_fatal(output_filename);
		}
		if ( ibfd->xvec->flavour != obfd->xvec->flavour ){
			fprintf (stderr,
				"%s: object format change: truncating symbols\n",
				program_name );
		}
		if (leave_only_externals || leave_only_relocation)
			obfd->flags |= STRIP_LINES;
		if ( strip ){
			fprintf (stderr,
			    "%s: Stripping an archive makes it worthless -- ignored\n",
			    program_name );
			return 0;
		}
		copy_archive(ibfd, obfd);
	} else {
		fprintf (stderr,
		   "%s: '%s' not a recognized object or archive format\n",
		   program_name, input_filename );
		exit(1);
	}
	make_same_modes(input_filename,output_filename);
	return 1;
}


/** Actually do the work */
static void
setup_sections(ibfd, isec, obfd)
    bfd            *ibfd;
    sec_ptr         isec;
    bfd            *obfd;
{
    sec_ptr         osec;
    char           *err;

    if ((changing_omfs || strip || leave_only_externals || leave_only_relocation) &&
	(isec->flags & SEC_IS_DEBUG))
	    return;
	
    osec = bfd_make_section(obfd, bfd_section_name(ibfd, isec));

    if (osec == NULL) {
	err = "making";
    } else if (!bfd_set_section_size(obfd,osec,bfd_section_size(ibfd,isec))){
	err = "size";
    } else if (!bfd_set_section_vma(obfd,osec,bfd_section_vma(ibfd,isec))) {
	err = "vma";
    } else if (!bfd_set_section_pma(obfd,osec,bfd_section_pma(ibfd,isec))) {
	err = "pma";
    } else if (!bfd_set_section_alignment(obfd,osec,bfd_section_alignment(ibfd,isec)) ){
	err = "alignment";
    } else if (!bfd_set_section_flags(obfd,osec,bfd_get_section_flags(ibfd,isec))) {
	err = "flags";
    } else {
	/* All went well */
	/* 
         * Remember the flags from the input section so that, for COFF, we
         * can remember whether a section was BSS-type or not. Since objcopy
         *  will not combine sections, we can just copy straight across. 
         */
	arelent       **relpp;
	int             relcount;

	osec->insec_flags = isec->insec_flags;

	if ( (strip) || get_reloc_upper_bound(ibfd, isec) == 0) {
	    bfd_set_reloc(obfd, osec, (arelent **)NULL, 0);
	} else {
	    relpp = (arelent **) xmalloc(get_reloc_upper_bound(ibfd, isec));
	    relcount = bfd_canonicalize_reloc(ibfd, isec, relpp, sympp);
	    bfd_set_reloc(obfd, osec, relpp, relcount);
	}
	return;
    }



    fprintf(stderr, "%s: file \"%s\", section \"%s\": error setting section's %s: %s\n",
	    program_name,
	    bfd_get_filename(ibfd), bfd_section_name(ibfd, isec),
	    err, bfd_errmsg(bfd_error));
    if (BFD_BOUT_FILE_P(obfd) && (isec->flags & SEC_IS_BIG_ENDIAN))
	    fprintf(stderr, "%s: The bout omf does not support Big endian code / data.\n",program_name);
    exit(1);
}

/*
 * Copy all the section related data from an input section
 * to an output section
 *
 * If stripping then don't copy any relocation info
 */
static void
copy_sections(ibfd, isec, obfd)
    bfd            *ibfd;
    sec_ptr         isec;
    bfd            *obfd;
{
	sec_ptr         osec;
	unsigned long   size;

	if ((changing_omfs || strip || leave_only_externals || leave_only_relocation) &&
	    (isec->flags & SEC_IS_DEBUG))
		return;
		
        /*
         * The input file should not contain section names other than
         * the standard, .text, .data, .bss
         */
        if ( out_format_specified && (out_format == BFD_BOUT_FORMAT) ) {
                char *sec_name = (char *)bfd_section_name(ibfd, isec);
                if (strcmp(sec_name, ".text") && strcmp(sec_name, ".data") &&
                    strcmp(sec_name, ".bss")) {
                        fprintf(stderr, "ERROR: %s contains non-standard section '%s'\n",
                                ibfd->filename, sec_name);
                        bfd_close(obfd);
                        exit(1);
                }
        }
  	osec = bfd_get_section_by_name(obfd, bfd_section_name(ibfd, isec));

	size = bfd_section_size(ibfd, isec);
	if (size == 0){
		return;
	}

	if (bfd_get_section_flags(ibfd, isec) & SEC_HAS_CONTENTS) {
		unsigned char *memhunk = (unsigned char *) xmalloc(size);

		if (!bfd_get_section_contents(ibfd, isec, memhunk, 0, size)) {
			bfd_fatal(bfd_get_filename(ibfd));
		}
		if (!bfd_set_section_contents(obfd, osec, memhunk, 0, size)) {
			bfd_fatal(bfd_get_filename(obfd));
		}
		free(memhunk);
	}
}

/* Because elf stores sections in big and little endian oriientation,
   we have to translate this into COFF's file oriented notion. */
static int
big_sections_only(section_ptr, format, inputfilename)
    asection *section_ptr;
    enum target_flavour_enum format;
    char *inputfilename;
{
    int le_sect_count = 0,be_sect_count = 0;

    for (;section_ptr;section_ptr = section_ptr->next) {
	if (section_ptr->flags & SEC_HAS_CONTENTS) {
	    if ((section_ptr->flags & SEC_IS_BIG_ENDIAN) == 0)
		    le_sect_count++;
	    else
		    be_sect_count++;
	}
    }
    if (be_sect_count && !le_sect_count)
	    return 1;
    else if ((!be_sect_count && le_sect_count) ||
	     (!be_sect_count && !le_sect_count))
	    return 0;
    else {
	if (format != bfd_target_elf_flavour_enum) {
	    fprintf(stderr,"ERROR: input file %s, contains a mix of byte orders, and is\n", inputfilename);
	    fprintf(stderr,"not representable in the %s format.\n",format == bfd_target_aout_flavour_enum ?
		    "bout" : "coff");
	    exit(1);		
	}
	/* Must be a mix be_sect_count > 0 and le_sect_count > 0. */
    }
}

static
char *
pick_target( abfd )
    bfd *abfd;
{
	int big_endian;
	enum target_flavour_enum format;

        if (reverse_flag && BFD_BIG_ENDIAN_FILE_P(abfd)) {
                fprintf(stderr, "ERROR: Already converted: %s\n", abfd->filename);
                exit(1);
        }
 	format     = out_format_specified ? out_format : (abfd)->xvec->flavour ;
	big_endian = out_endian_specified ?
				out_big_endian : BFD_BIG_ENDIAN_FILE_P(abfd) ||
					reverse_flag;
#if !defined(NO_BIG_ENDIAN_MODS)
	return bfd_make_targ_name( format, big_endian, big_sections_only((abfd)->sections, format,
									 abfd->filename));
#else
	return bfd_make_targ_name( format, big_endian );
#endif /* NO_BIG_ENDIAN_MODS */
}


/*
 * Determine whether we are running a stripper, copier, or
 * converter.
 */
static
prog_type
get_prog_kind(program_name)
	char *program_name;
{
	char *temp = strrchr(program_name, SLASH);
	int prog_len;

	if (temp) {
		temp++;
		program_name = temp;
	}
	prog_len = strlen(program_name);

/* make sure it is not case sensitive in DOS */

#ifdef DOS
#define SCMP strnicmp
#else
#define SCMP strncmp
#endif

	if (prog_len >= 8) {
		if (!SCMP(program_name, "gstrip96", 8)) {
			return (stripper);
		}
	}
	if (prog_len >= 7) {
		if (!SCMP(program_name, "objcopy", 7)) {
			return (copier);
		}
	}
	if (prog_len >= 6 ) {
		if (!SCMP(program_name, "cof960", 6)) {
			return (converter);
		}
		if (!SCMP(program_name, "str960", 6)) {
                        return (stripper);
                }
        }
	return (stripper); /* if it cannot recognize return as stripper */
}


int
main(argc, argv)
    int  argc;
    char *argv[];
{
	int i = 1;
	int input_index;
	char *tmpname;
	char c;
	short little_endian_host = 1;

	program_name = argv[0];
	verbose = false;


	prog_kind = get_prog_kind(program_name);

        /* Check the command line for a response file, and handle it if found.*/
        argc = get_response_file(argc,&argv);
	check_v960_str( argc, argv, prog_str[(int)prog_kind] );
	check_help( argc, argv, prog_kind );

	if (prog_kind == stripper) {
		if ( argc < 2 ){
			usage(stripper);
			exit(0);
		}

		strip = true;
		while ((c=getopt_long(argc, argv, "abCxlrVzh", long_options, &i)) != EOF ) {
			switch (c) {
			case 'a':
				strip = true;
				break;
			case 'b':
				leave_only_block_info = true;
				partial_strip = true;
				break;
 			case 'C':
 				strip_ccinfo_only = true;
 				partial_strip = true;
 				break;
			case 'x':
				leave_only_externals = true;
				partial_strip = true;
				break;
			case 'h':
				usage(stripper);
				exit(0);
			case 'l':
				strip_lines_only = true;
				partial_strip = true;
				break;
			case 'r':
				leave_only_relocation = true;
				partial_strip = true;
				break;
			case 'V':
				/* print version but don't exit */
				gnu960_put_version_str(prog_str[(int)prog_kind]);
				break;
			case 'z':
				suppress_time_stamp = true;
				break;
			default:
				usage(stripper);
				exit(1);
			}
		}
		strip = partial_strip ? false : true;
		for ( i=optind ; i < argc; i++ ) {
			tmpname = make_tempname(argv[i]);
                        if(copy_file(argv[i], tmpname)) {
                            cp_or_mv_file(tmpname,argv[i],1);
                        }
		/* if we don't go into cp_or_mv_file, this will get rid of the */ 
		/* temp file */
			unlink(tmpname);   
		}
		return 0;
	}
	/* objcopy/cof960 processing starts here */
	

        if ( argc < 2 ){
        	usage(prog_kind);
                exit(0);
        }

	for (i = 1; i < argc; i++) {
		char *arg_ptr;

		if (argv[i][0] == '-' 
#ifdef DOS
		    || argv[i][0] == '/'
#endif
		) {
			arg_ptr = argv[i];
			while (c = *++arg_ptr) {
				switch (c) {
			        case 'c':
				        copy_or_mv_file = 1;
					break;
					
				case 'j':
					dash_j = true;
					break;

				case 'J':
					dash_J = true;
					break;
					
				case 'q':
					verbose = false;
					break;

				case 'v':
					/* Note: '-v' without any other options  means 
					 * display byte order of file.
					 */
					verbose = true;
					break;
				case 'V':
					/* print version but don't exit */
					gnu960_put_version_str(prog_str[(int)prog_kind]);
					break;
				case 'p':
					/* Some objcopy behavior as far as input that is worth
					 * noting.  "objcopy *.o" is illegal unless a -p
					 * switch is on. Then "objcopy -p *.o" means convert all files
					 * in cwd that're prefixed with .o, in place.
					 */
					convert_in_place = 1;
					break;
				
				case 'C':
				case 'S':
				case 'x':
				case 'F':
				case 'b':
				case 'h':
				case 'l':
				case 'r':
				case 'z':
					/* if NONE of the above options are specified
					 * ('S' through 'z') and we are converting
					 * in place, i.e. when one input file or
					 * -p option, make sure that we warn user
					 * that no conversion will take place.
					 */
					conversion_opt_specified = true; 
					
					switch (c) {
				        case 'C':
 					        strip_ccinfo_only = true;
 						break;
					case 'S':
						strip = true;
						break;

					case 'x':
						strip_promoted = true;
						break;
						
					case 'F':
						arg_ptr++;
						if ( !strncmp(arg_ptr,"coff", 4) ){
							out_format_specified = 1;
							out_format = BFD_COFF_FORMAT;
							arg_ptr += 3;
						} else if ( !strncmp(arg_ptr,"elf",3) ){
							out_format_specified = 1;
							out_format = BFD_ELF_FORMAT;
							arg_ptr += 2;
						} else if ( !strncmp(arg_ptr,"bout",4) ){
							out_format_specified = 1;
							out_format = BFD_BOUT_FORMAT;
							arg_ptr += 3;
						} else {
							usage(prog_kind);
							exit(1);
							/* NO RETURN */
						}
						break;
					case 'b':
						out_endian_specified = 1;
						out_big_endian = 1;
						reverse_flag = 0;
						break;
					case 'h':
						out_endian_specified = 1;
						out_big_endian= !(*(char *)&little_endian_host);
						reverse_flag = 0;
						break;
					case 'l':
						out_endian_specified = 1;
						out_big_endian = 0;
						reverse_flag = 0;
						break;
					case 'r':
						reverse_flag = 1;
						out_endian_specified = 0;
						break;
					case 'z':
						suppress_time_stamp = true;
						break;
					}
					break;
					
				default:
					usage(prog_kind);
					exit(1);/* NO RETURN */
				}
			}
		} else if (!input_filename) {
			input_filename = argv[i];
			input_index = i;
		} else if (!output_filename) {
			output_filename = argv[i];
		} else if (!convert_in_place) {
			usage(prog_kind);
			exit(1);/* NO RETURN */
		}
	}

	if (input_filename == (char *) NULL){
		usage(prog_kind);
		exit(1);/* NO RETURN */
	}

	/* If there is no destination file then create a temp and rename
	 * the result into the input 
         */

	if (output_filename == (char *)NULL || convert_in_place) {
		int j;
                output_filename = (char *) NULL; /* force conversion in place */
                for (j = input_index ; j < argc ; j++) {
			if (conversion_opt_specified) {
				tmpname = make_tempname(argv[j]);

				if(copy_file(argv[j], tmpname)) {
					cp_or_mv_file(tmpname,argv[j],copy_or_mv_file);
				}
			/* if we don't go into cp_or_mv_file, this will get rid of the */ 
			/* temp file */
				unlink(tmpname);  
			} else { /* no conversion option specified */
				if (verbose) { 
					bfd *tmp_bfd = bfd_openr(argv[j], input_target);
					if (tmp_bfd == NULL ) {
						bfd_fatal(argv[j]);
					}
					if (bfd_check_format(tmp_bfd, bfd_object)) {
						verbose_message(tmp_bfd); printf("\n");
					} else if (bfd_check_format(tmp_bfd, bfd_archive)) {
						bfd *ar_file = NULL;
						for (;;) {
							ar_file = bfd_openr_next_archived_file
								(tmp_bfd, ar_file);
							if (ar_file == NULL) {
								if (bfd_error != no_more_archived_files){
									bfd_fatal(argv[j]);
								}
								break;
							}
							if (!bfd_check_format(ar_file, bfd_object)) {
								printf("%s: not an object file\n",
								       ar_file->filename);
							} else
								verbose_message(ar_file); printf("\n");
						}
					} else
						bfd_fatal(argv[j]);

					bfd_close(tmp_bfd);
				}
				fprintf(stderr, "WARNING: No conversion done on '%s'. Specify option b,F,h,l,r,S,x or z.\n", argv[j]); 
			}
		} /* for */
	} else {
		copy_file(input_filename, output_filename);
	}

	return 0;
}
