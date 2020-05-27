/* ar.c - Archive modify and extract. */

#include "sysdep.h"
#include "bfd.h"
#include "ar.h"
#include <stdio.h>
#include <dio.h>
#ifdef DOS
#	include <io.h>
#else
#	include <sys/time.h>
#endif
#include <errno.h>


#ifdef DOS
/*
For some real bizarre reason, dos passes this name to the executable as argv[0]

"GRAN960.exe"

(Note the mixture of case.)

Just to make sure, we upcase the entire argv[0]:
*/

#include <ctype.h>

static void
upcase(s)
char *s;
{
    char *q = s;

    while (*s) {
	*s = toupper(*s);
	s++;
    }
}

#	define RANLIB	"GRAN960.EXE"
#else
#	define RANLIB	"gran960"
#endif

#ifdef __HIGHC__ /* Metaware recommends 4K of stack/buff size */
#define BUFSIZE 3072  /* let's be convervative */
#else
#define BUFSIZE 8192
#endif

PROTO(size_t, bfd_read, (void *ptr, size_t size, size_t nitems, bfd * abfd));
PROTO(size_t, bfd_write, (void *ptr, size_t size, size_t nitems, bfd * abfd));

#ifdef __STDC__
static void     open_inarch(char *archive_filename);
#else
static void     open_inarch();
#endif				/* __STDC__ */

PROTO(void, map_over_members, (void (*function) (), char **files, int count));
PROTO(void, print_contents, (bfd * member));
PROTO(void, extract_file, (bfd * abfd));
PROTO(void, delete_members, (char **files_to_delete));
PROTO(void, do_quick_append, (char *archive_filename, char **files_to_append));
PROTO(void, move_members, (char **files_to_move));
PROTO(void, replace_members, (char **files_to_replace));
PROTO(void, replace_all_members, ());
PROTO(void, print_descr, (bfd * abfd));
PROTO(void, ranlib_only, (char *archname));


extern char *program_name;
static char *default_target_name;
bfd bogus_archive;
static bfd *inarch;		/* The input arch we're manipulating */

int silent_create = 0;	/* 1 => don't warn about creating a new archive file */
int verbose = 0;	/* 1 => describe each action performed  */
int preserve_dates = 0;	/* 1 => preserve dates of members when extracting  */
int newer_only = 0;	/* 1 => don't replace existing members whose dates
			 *	are more recent than the corresponding files
			 */
int write_armap = 0;	/* 1 => write a symbol map into the modified archive */
int ignore_symdef = 0;	/* 1 => don't update symbol map unless command line
			 *	explicitly requested it
			 */
int local_create = 0;   /* 1 => create temporary workfiles under cur directory */
int suppress_time_stamp = 0;  /* zero archive header time stamp */

char *posname = NULL;	/* Non-NULL => name of an existing member; position new
			 *	or moved files with respect to this one
			 */

/* how to use `posname' */
enum pos {
	pos_default,	/* default appropriately (posname == NULL)	*/
	pos_before,	/* position before that member			*/
	pos_after,	/* position after that member			*/
	pos_end		/* always at end (posname == NULL)		*/
} postype = pos_default;


/* default directory for temporary files is derived from P_tmpdir as
 * defined in stdio.h.
 */
char *tmpdir = P_tmpdir; /* default temp file directory, if environment
                        * TMPDIR is not defined .
                        */

char *utext[] = {
        "",
        "Librarian for the i960 processor",
        "",
        "Key Characters:",
        "   -F{coff|elf|bout}:   Explicitly specify what OMF you want to output library to be",
        "   -d:   delete file from library lib",
        "   -m:   move file to the end of library lib",
	"          [unless a position option is specified--see abi options]",
        "   -p:   print file from the library lib to stdout",
        "   -r:   replace file in the library lib",
	"          [new files are placed at the end of the library unless a",
	"           position option is specified--see abi options]",
        "   -t:   print information about element file in the library lib",
	"          [if no files are specified, a table of contents of the",
	"           entire library is printed]",
        "   -u:   add a new file or replace a file in the library lib",
	"          [the file is replaced if the last modified-date is newer than", 
	"           the already-archived version, new files are placed at the",
	"           end of the library unless a position option is specified]",
        "   -x:   extract file from the library lib",
	"          [if no files are specified, all files in the library are", 
	"           extracted, but lib is not altered.  The last modified date", 
	"           of each extracted file is the date it is extracted", 
	"           (unless the -o option is used)]", 
        "   -V:   print the version number and continue",
        "-help:   display this help message",
        "-v960:   print the version number and exit",
        "",
	"Option Characters:",
        " -abi:   when adding with -r or -u, or moving with -m, place",
        "         file after(a), or before (b or i) file posname in library",
        "         lib, posname must be specified",
        "   -c:   suppress the message issued if a lib is created",
        "   -l:   place temporary work files in CWD",
        "          [such files are otherwise placed in the environment variable",
        "           TMPDIR, or in P_tmpdir as defined in stdio.h if TMPDIR is not",
	"           set (on Unix this is usually /var/tmp or /tmp]",
        "   -o:   create file with the last-modified date recorded in the library",
        "          [used when extracting with the -x option]",
        "   -s:   create a symbol cross-reference table for the linker in lib",
        "   -u:   ignore those files specified with the -r command that have",
        "         last-modified dates older than the already archived versions",
        "          [this does not speed up the process however]",
        "   -v:   give a verbose description of the operation",
        "   -z:   zero out archive header time stamp",
        "",
        "See your user's guide for a complete command-line description",
        "",
	NULL
};
	
void
gnu960_verify_object(abfd)
bfd *abfd;
{
    if ( abfd->format == bfd_unknown ){
	/* ignore return from bfd_check_format to support
	 * not only object file but also text file
	 */
	bfd_check_format(abfd, bfd_object);
    }
    if ( (inarch == &bogus_archive) && !(inarch->xvec) && bfd_check_format(abfd, bfd_object)) {
	/* We will be creating a new archive, and this object file
	 * is the first one to go into it:  make the archive the
	 * same target type as the object file.
	 */
	inarch->xvec = abfd->xvec;
    } else if ( abfd->format == bfd_object &&
	       abfd->xvec->flavour != inarch->xvec->flavour ) {
	fatal("Can't add %s file (%s) to %s archive: operation aborted",
	      BFD_COFF_FILE_P(abfd) ? "COFF" : BFD_ELF_FILE_P(abfd) ? "ELF" : "b.out", 
	      abfd->filename,
	      BFD_COFF_FILE_P(inarch) ? "COFF" : BFD_ELF_FILE_P(inarch) ? "ELF" : "b.out" 
	      );
    }
}



int operation_grows_arch = 0;

/* Some error-reporting functions */

void
put_gar_help()
{
   	int i;
	
	printf( "\nUsage:  %s key [posname] [options ...] lib file ...\n",program_name );
        paginator(utext); 
}

void
usage ()
{
	fprintf(stderr, "\nUsage: %s -{F{coff|elf|bout}d|m|p|r|t|u|x|h|v960} [-closuvVz] [-abi posname] archive [name ...]\n", program_name);
	fprintf(stderr, "Use the -h option to get help\n\n");
	exit(1);
}

/*
 * The option parsing should be in its own function.  It will be when I have
 * getopt working.
 */
int
main(argc, argv)
    int    argc;
    char **argv;
{
	char * arg_ptr;
	char   c;
	int    i, files_arg_index, arg_index, pos_index = 0;
	char **files;
	char * inarch_filename;
	char * temp;

	enum { none = 0, delete, replace, print_table,
		    print_files, extract, move, quick_append
	} operation = none;

	/* Check the command line for a resonse file, and handle it if found.*/
	argc = get_response_file(argc,&argv);

	check_v960( argc, argv );
	program_name = argv[0];
	
#ifdef DOS
#	define SLASH '\\'

	upcase(program_name);

#else
#	define SLASH '/'
#endif

	if (argc == 1) { 	/* no-args == help */
		put_gar_help();
		exit(0);
	}

	temp = strrchr(program_name, SLASH);
	if (temp) {
		temp++;
		program_name = temp;
	} else {
		temp = program_name;	/* shouldn't happen, but... */
	}

	if (!strcmp(temp, RANLIB)) {
		if (argc < 2) {
			fprintf(stderr, "Too few command arguments.\n");
			usage();
		}
		for ( argv++; --argc; argv++ ){
			ranlib_only(*argv);
		}
		exit(0);
	}

	for (i = arg_index = 1; i < argc; i++ ) {
	    arg_ptr = argv[i];

	    if ( (i > 1) && ((*arg_ptr != '-') 
#ifdef DOS
		     && (*arg_ptr != '/')
#endif
	     )) {
		continue;   /* '-' optional for first switch */
	        }
	    if ((*arg_ptr == '-') 
#ifdef DOS
	        ||  (*arg_ptr == '/')
#endif
	      ) {
		++arg_ptr;		/* compatibility */
	        }
            arg_index++;

	    while (c = *arg_ptr++) {
		switch (c) {
	        case 'd':
		case 'm':
		case 'p':
		case 'r':
		case 't':
		case 'x':
			if (operation != none) {
				fprintf(stderr, "Two different operation switches specified\n");
				usage();
			}

			switch (c) {
			case 'd': operation = delete;		break;
			case 'm': operation = move;		break;
			case 'p': operation = print_files;	break;
			case 'q':
				operation = quick_append;
				operation_grows_arch = 1;
				break;
			case 'r':
				operation = replace;
				operation_grows_arch = 1;
				write_armap = 1;
				break;
			case 't': operation = print_table;	break;
			case 'x': operation = extract;		break;
			}
			break;

		case 'c': silent_create = 1;	break;
		case 'l': local_create = 1;     break;
		case 'o': preserve_dates = 1;	break;
		case 's': write_armap = 1;	break;
		case 'u': 
			operation = replace;
			newer_only = 1;
			write_armap = 1;
			operation_grows_arch = 1;
			break;
		case 'v': verbose = 1;		break;
		case 'h': put_gar_help();	exit(0);
		case 'V':
			/* -V means print version info but don't exit */
			gnu960_put_version();
			break;
		case 'a': 
		case 'b': 
		case 'i': 
			switch (c) {
			case 'a': postype = pos_after;  break;
			case 'b': postype = pos_before; break;
			case 'i': postype = pos_before; break;
			}
			/* See if any of the positional options are
			 * bunched together, e.g. -rabi. In this case
			 * only acknowledge the last specifier, 'i' and
			 * ignore 'a' and 'b'.
			 */
			if (pos_index && ( (pos_index + 1) == arg_index)) {
				break;
			}
			if (pos_index) {
				fprintf(stderr, "WARNING: Position %s is ignored\n", argv[pos_index]);
			}

			pos_index = arg_index++;
			break;
			
		case 'z':
			suppress_time_stamp = 1;
			break;
		case 'F':
		    {
			char *F_arg_value = "";
			enum target_flavour_enum flavour;

			if (*arg_ptr) {
			    F_arg_value = arg_ptr;
			    arg_ptr += strlen(arg_ptr);
			}
			else if (++i < argc)
				F_arg_value = argv[++i];
			if (!strcmp("coff",F_arg_value))
				flavour = bfd_target_coff_flavour_enum;
			else if (!strcmp("elf",F_arg_value))
				flavour = bfd_target_elf_flavour_enum;
			else if (!strcmp("bout",F_arg_value))
				flavour = bfd_target_aout_flavour_enum;
			else {
			    fprintf(stderr,"-F requires a value of coff, elf or bout\n");
			    usage();
			}
			default_target_name = bfd_make_targ_name(flavour,0,0);
		    }
			break;

		default:
			fprintf(stderr, "Invalid option %c\n", c);
			usage();
		}
	  }
	}

	if (operation == none) {
	    if ( (write_armap) && (argc >= 3) ) {
		ranlib_only(argv[2]);
		exit(0);
	    } else {
                     fprintf(stderr, "Too few command arguments\n");
                     usage();
		   }
	}
 
	if (newer_only && operation != replace) {
		fprintf(stderr, "'u' only meaningful with 'r' option.\n");
		usage();
	}

	/* At this point make sure we have enough arguments */
	if (argc < 3) {
	        fprintf(stderr, "Too few command arguments\n");
	        usage();
	}

	if (postype != pos_default) {
		posname = argv[pos_index];
		if (posname == NULL) {
		        fprintf(stderr, "Position modifier requires an argument\n");
			usage();
		}	  
	}

	inarch_filename = argv[arg_index++];
	if (inarch_filename == NULL) {
	        fprintf(stderr, "No archive name specified\n");
		usage();
        } 
		  
	files_arg_index = arg_index;
	if (arg_index < argc) {
		files = argv + arg_index;
		while (arg_index < argc) {
			if (!strcmp(argv[arg_index++], "__.SYMDEF")) {
				ignore_symdef = 1;
				break;
			}
		}
	} 
	else {
	    files = NULL; 
	}

	open_inarch(inarch_filename);

	/*
         * If we have no archive and we've been asked to replace, create one
	 */

	switch (operation) {

	case print_table:
		map_over_members(print_descr, files, argc - files_arg_index);
		break;

	case print_files:
		map_over_members(print_contents, files, argc - files_arg_index);
		break;

	case extract:
		map_over_members(extract_file, files, argc - files_arg_index);
		break;

	case delete:
		if (files) {
			delete_members(files);
		}
		break;

	case move:
		if (files) {
			move_members(files);
		}
		break;

	case replace:
		if (files || write_armap) {
			if (files) {
				replace_members(files);
			}
			else {
				replace_all_members();
			}
		}
		break;

	default:
		fprintf(stderr, "Sorry; '-%c' option not implemented.\n", c);
		usage();
	}

	return 0;
}

static
char *normalize(file)
    char *file;
{
	char *filename = strrchr(file, SLASH);
#ifdef DOS
	/* We allow '/' in DOS pathnames so try for it */
	if (filename == NULL) {
	      filename = strrchr(file, '/');
	      /* remove drive specification */
	      if (filename == NULL) {
		      filename = strrchr(file, ':');
	      }
	}
#endif
	return filename ? filename+1 : file;
}

static void
open_inarch(archive_filename)
    char *archive_filename;
{
	bfd  **last_one;
	bfd   *next_one;
	struct stat sbuf;

	bfd_error = no_error;
	if (stat(archive_filename, &sbuf) != 0) {
		if (errno != ENOENT) {
			perror(archive_filename);
			exit(1);
		}
		if (!operation_grows_arch) {
			fprintf (stderr, "%s: %s not found.\n", program_name,
							archive_filename);
			exit (1);
		}
		if (!silent_create) {
			fprintf(stdout, "%s: creating %s\n",
						program_name, archive_filename);
		}
		inarch = &bogus_archive;
		inarch->filename = archive_filename;
		inarch->has_armap = 1;
	} else {
		inarch = bfd_openr(archive_filename, NULL);
		if (inarch == NULL) {
			bfd_perror(archive_filename);
			exit(1);
		}

		if ( !bfd_check_format(inarch, bfd_archive)) {
			bfd_perror(archive_filename);
			exit(1);
		}
		last_one = &(inarch->next);

		/* Read all the contents right away, regardless. */
		next_one = bfd_openr_next_archived_file(inarch, NULL);
		while ( next_one ){
			*last_one = next_one;
			last_one = &next_one->next;
			next_one= bfd_openr_next_archived_file(inarch,next_one);
		}
		*last_one = (bfd *) NULL;
		if (bfd_error != no_more_archived_files) {
			bfd_perror(archive_filename);
			exit(1);
		}
	}
}



/*
 * If count is 0, then function is called once on each entry. if nonzero,
 * count is the length of the files chain; function is called on each entry
 * whose name matches one in files
 */
void
map_over_members(function, files, count)
    void (*function) ();
    char **files;
    int  count;
{
	bfd *head;

	if (count == 0) {
		for (head = inarch->next; head; head = head->next) {
			function(head);
		}
		return;
	}

	/*
	 * We have to iterate over the filenames in order to notice where
	 * a filename is requested but does not exist in the archive.  Ditto
	 * mapping over each file each time -- we want to hack multiple
	 * references.
	 */
	for (; count > 0; files++, count--) {
		int found = 0;
		for (head = inarch->next; head; head = head->next)
#ifdef DOS
#define STRCMP stricmp
#else
#define STRCMP strcmp
#endif
			if (head->filename && !STRCMP(*files, head->filename)){
				found = 1;
				function(head);
                                break;
			}
		if (!found) {
			fatal("No entry %s in archive", *files);
			
		}
	}
}


/* Things which are interesting to map over all or some of the files: */

void
print_descr(abfd)
    bfd *abfd;
{
	print_arelt_descr(abfd, verbose, suppress_time_stamp);
}

void
print_contents(abfd)
    bfd *abfd;
{
	int ncopied = 0;
	struct stat buf;
	long size;

	if (bfd_stat_arch_elt(abfd, &buf) != 0) {
		fatal("Internal stat error on %s", abfd->filename);
	}
	if (verbose) {
		printf("\n<member %s>\n\n", abfd->filename);
	}
	bfd_seek(abfd, 0, SEEK_SET);

	size = buf.st_size;
	while (ncopied < size) {
		char  cbuf[BUFSIZE];
		int   nread;
		int   tocopy = size - ncopied;

		if (tocopy > BUFSIZE) {
			tocopy = BUFSIZE;
		}
		nread = bfd_read(cbuf, 1, tocopy, abfd);
		if (nread != tocopy) {
			fatal("file %s not a valid archive",
						abfd->my_archive->filename);
		}
		fwrite(cbuf, 1, nread, stdout);
		ncopied += tocopy;
	}
}


/*
 * Extract a member of the archive into its own file.
 *
 * We defer opening the new file until after we have read a BUFSIZ chunk of the
 * old one, since we know we have just read the archive header for the old
 * one.  Since most members are shorter than BUFSIZ, this means we will read
 * the old header, read the old data, write a new inode for the new file, and
 * write the new data, and be done. This 'optimization' is what comes from
 * sitting next to a bare disk and hearing it every time it seeks.
 */
void
extract_file(abfd)
    bfd *abfd;
{
	FILE *ostream;
	char  cbuf[BUFSIZE];
	int   nread, tocopy;
	int   ncopied = 0;
	long  size;
	struct stat  buf;

	if (bfd_stat_arch_elt(abfd, &buf)) {
		fatal("Internal stat error on %s", abfd->filename);
	}
	size = buf.st_size;
	if (verbose) {
		printf("x - %s\n", abfd->filename);
	}

	bfd_seek(abfd, 0, SEEK_SET);
	ostream = 0;
	if (size == 0) {
		ostream = fopen(abfd->filename, FWRTBIN);

		if (!ostream) {
			perror(abfd->filename);
			exit(1);
		}
	} else {
		while (ncopied < size) {
			tocopy = size - ncopied;
			if (tocopy > BUFSIZE) {
				tocopy = BUFSIZE;
			}
			nread = bfd_read(cbuf, 1, tocopy, abfd);
			if (nread != tocopy) {
				fatal("file %s not a valid archive",
						abfd->my_archive->filename);
			}

			/* see comment above -- this saves disk arm motion */
			if (!ostream) {
				ostream = fopen(abfd->filename, FWRTBIN);
				if (!ostream) {
					perror(abfd->filename);
					exit(1);
				}
			}
			/* no need to byte-swap; the two formats are
			 * presumably compatible
			 */
			fwrite(cbuf, 1, nread, ostream);
			ncopied += tocopy;
		}
	}

	fclose(ostream);
	chmod(abfd->filename, buf.st_mode);
	if (preserve_dates) {
#ifdef USG
		long            tb[2];
		tb[0] = buf.st_mtime;
		tb[1] = buf.st_mtime;
		utime(abfd->filename, tb);	/* FIXME check result */
#else
		struct timeval  tv[2];
		tv[0].tv_sec = buf.st_mtime;
		tv[0].tv_usec = 0;
		tv[1].tv_sec = buf.st_mtime;
		tv[1].tv_usec = 0;
		utimes(abfd->filename, tv);	/* FIXME check result */
#endif
	}
}

/* Copy one file to the other */
static
void CopyFile(File_In, File_Out) 
    char *File_In;
    char *File_Out;
{
        FILE *istream, *ostream;
	struct stat Fstat;
	off_t rwlength;
	size_t readlen;
	char buffer[BUFSIZE];

	istream = fopen(File_In, FRDBIN);
	if (!istream) {
	        perror( File_In );
		exit(1);
	}
	ostream = fopen(File_Out, FWRTBIN);
	if (!ostream) {
	        perror( File_Out );
		fclose(istream);
		exit(1);
	}
	if ( fstat( fileno( istream ), (struct stat *)&Fstat ) ) {
	        perror( File_In );
		fclose(istream);
		fclose(ostream);
		exit(1);
	}
	/* now start copying ... */
	rwlength = Fstat.st_size;
	while (rwlength) {
	        if ((readlen = rwlength) > sizeof(buffer))
			readlen = sizeof(buffer);
		fread(buffer, 1, readlen, istream );
		if (fwrite(buffer, 1, readlen, ostream) < readlen ) {
		       perror( File_Out );
		       fclose(istream);
		       fclose(ostream);
		       exit(1);
		}
		rwlength -= readlen;
	    }
	fclose(istream);
	fclose(ostream);
}

extern char *xmalloc();

/* Create a temp file */
static
char *
make_tempname()
{
    static char template[] = "arXXXXXX";
    char *tmpname = xmalloc(sizeof(template));

    strcpy(tmpname, template);
    mktemp(tmpname);
    return tmpname;
}


void
write_archive()
{
	bfd  *obfd;
	char *new_name;
	bfd  *contents_head = inarch->next;

	new_name = make_tempname();
	obfd = bfd_openw(new_name, (inarch->xvec) ? bfd_get_target(inarch) : default_target_name);

	if (obfd == NULL){
		bfd_fatal(new_name);
		exit(1);
	}

	if (suppress_time_stamp)
		obfd->flags |= SUPP_W_TIME;

	bfd_set_format(obfd, bfd_archive);
	obfd->has_armap = write_armap;

	if (!bfd_set_archive_head(obfd, contents_head)) {
		bfd_fatal(inarch->filename);
	}
	if (!bfd_close(obfd)) {
		bfd_fatal(obfd->filename);
	}
	if (inarch != &bogus_archive && !bfd_close(inarch)) {
	    bfd_fatal(inarch->filename);
	}

	if ( local_create ) {
#ifdef DOS
	    /* The rename function in the Microsoft C rtl differs from UNIX.  */
	    /* Therefore, we must first ensure that the file doesn't exist.   */
	    if (access(inarch->filename,CHECK_FILE_EXISTENCE) == FILE_EXISTS) {
		if (remove(inarch->filename)) {
		    bfd_fatal(inarch->filename);
		}
	    }
#endif 
	    if (rename(new_name, inarch->filename)) {
		bfd_fatal(inarch->filename);
	    }
	} else {
	    /* If temp files are in another system file, rename will fail.
             * In this case we have to do a less efficient method,
             * unfortunately. We're going to do a copy from temp to
             * the destination file, then remove temp file.
             */
	    CopyFile(new_name, inarch->filename);
	    if ( unlink( new_name ) ) {
		bfd_fatal( new_name );
	    }
	}
}



/*
 * returns a pointer to the pointer to the entry which should be rplacd'd
 * into when altering.  default_pos should be how to interpret pos_default,
 * and should be a pos value.
 */
bfd **
get_pos_bfd(contents, default_pos)
    bfd      **contents;
    enum pos   default_pos;
{
	bfd   **after_bfd;
	enum pos realpos = (postype == pos_default ? default_pos : postype);
	int File_Pos_Found = 0;

	switch (realpos) {

	case pos_end:
		after_bfd = contents;
		while (*after_bfd) {
			after_bfd = &((*after_bfd)->next);
		}

		break;
	case pos_before:
		for (after_bfd=contents; *after_bfd; after_bfd= &((*after_bfd)->next)){
			if (!STRCMP((*after_bfd)->filename, posname)){
			        File_Pos_Found = 1;
				break;
			}
		}
		if (!File_Pos_Found) {
		    fprintf(stderr, "WARNING: Position %s not found\n", posname);
		}
		break;
	case pos_after:
		after_bfd = contents;
		for (after_bfd=contents; *after_bfd; after_bfd= &((*after_bfd)->next)){ 
			if (!STRCMP((*after_bfd)->filename, posname)) {
				after_bfd = &((*after_bfd)->next);
				File_Pos_Found = 1;
				break;
			}
		}
		if (!File_Pos_Found) {
		    fprintf(stderr, "WARNING: Position %s not found\n", posname);
		}
	}
	return after_bfd;
}


void
delete_members(files_to_delete)
    char **files_to_delete;
{
	bfd  **current_ptr_ptr;
	int    found;
	int    something_changed = 0;

	for (; *files_to_delete != NULL; ++files_to_delete) {
		/*
		 * In b.out, the armap is optional and is named __.SYMDEF.
		 * So if the user asked to delete it, we should remember
		 * that fact. The name is NULL in COFF archives, so using this
		 * as a key is as good as anything I suppose
		 */
		if (!strcmp(*files_to_delete, "__.SYMDEF")) {
			inarch->has_armap = 0;
			write_armap = 0;
			continue;
		}

		found = 0;
		current_ptr_ptr = &(inarch->next);
		while (*current_ptr_ptr) {
			if (!STRCMP(*files_to_delete,(*current_ptr_ptr)->filename)) {
				found = 1;
				something_changed = 1;
				if (verbose) {
					printf("d - %s\n", *files_to_delete);
				}
				*current_ptr_ptr = ((*current_ptr_ptr)->next);
				break;
			} else {
				current_ptr_ptr = &((*current_ptr_ptr)->next);
			}
		}
		if (!found) {
			fatal("No member named %s", *files_to_delete);
		}
	}
	if (something_changed == 1) {
		write_archive();
	}
}


void
replace_members(files_to_move)
    char **files_to_move;
{
	bfd **after_bfd;	/* New entries go after this one */
	bfd  *cur;
	bfd **curp;
	bfd  *temp;

	/*
	 * If the first item in the archive is __.SYMDEF, remove it
	 */
	if (inarch->next && !strcmp(inarch->next->filename, "__.SYMDEF")) {
		inarch->next = inarch->next->next;
	}

	for  ( ; files_to_move && *files_to_move; files_to_move++) {
		for ( curp = &inarch->next; *curp; curp = &(cur->next)) {
			cur = *curp;
			if (STRCMP(normalize(*files_to_move),cur->filename)) {
				continue;
			}

			if (newer_only) {
				struct stat fsbuf, asbuf;

				if (cur->arelt_data == NULL) {
					/* Can only happen if file specified
					 * more than once on the command line
					 */
					fprintf(stderr,"Duplicate file specified: %s -- skipping.\n", *files_to_move);
					goto next_file;
				}

				if (stat(*files_to_move, &fsbuf) != 0) {
					if (errno != ENOENT) {
						bfd_fatal(*files_to_move);
					}
					goto next_file;
				}
				if (bfd_stat_arch_elt(cur, &asbuf) != 0) {
					fatal("Internal stat error on %s",
								cur->filename);
				}
				if (fsbuf.st_mtime <= asbuf.st_mtime) {
					goto next_file;
				}
			}
                        /* if posname is not specified then look for
                         * the specified member to replace, then set
                         * position before that found member - so we can
                         * clip and replace easily.
                         */
			if (posname == NULL) {
				enum pos sav_postype = postype;
				posname = (char *) cur->filename;
				after_bfd = get_pos_bfd(&inarch->next, postype = pos_before);
				posname = NULL;
				postype = sav_postype;
			}
                        else {
				/* see presence of posname, and warn appropriately */
                                get_pos_bfd(&inarch->next, postype);
                                after_bfd = curp;
			}
			/* snip out this entry from the chain */
			*curp = cur->next; 

			temp = *after_bfd;
			*after_bfd = bfd_openr(*files_to_move, NULL);
			if (*after_bfd == (bfd *) NULL)
				fprintf(stderr, "warning: can't open file %s\n",
					*files_to_move);
			else {
			    gnu960_verify_object(*after_bfd); /* Exits on failure */
			    (*after_bfd)->next = temp;

			    if (verbose)
				    printf("%c - %s\n",
					   (postype == pos_after ? 'a' : 'r'),
					   *files_to_move);
			}
			break;
		    }

		if ( !(*curp) ){
		    /* It isn't in there, so add to end */

		    after_bfd = get_pos_bfd(&inarch->next, pos_end);
		    temp = *after_bfd;
		    *after_bfd = bfd_openr(*files_to_move, NULL);
		    if (*after_bfd == (bfd *) NULL)
			    fprintf(stderr, "warning: can't open file %s\n",
				    *files_to_move);
		    else {
			gnu960_verify_object(*after_bfd); /* Exits on failure */
			if (verbose)
				printf("a - %s\n", *files_to_move);
			(*after_bfd)->next = temp;
		    }
		}
next_file:
		;
	}
	write_archive();
}

/* Reposition existing members within an archive */
void
move_members(files_to_move)
    char **files_to_move;
{
	bfd **after_bfd;	/* New entries go after this one */
	bfd  *cur;
	bfd **curp;
	bfd  *temp;
        int  Entry_Found = 0;

	for  ( ; files_to_move && *files_to_move; files_to_move++) {
		for ( curp = &inarch->next; *curp; curp = &(cur->next)) {
			cur = *curp;
			if (STRCMP(normalize(*files_to_move),cur->filename)) {
				continue;
			}

			after_bfd = get_pos_bfd(&inarch->next, pos_end);

                        /*  if the move will end up at the same original
                         *  order then don't bother
                         */
                        if (*after_bfd == cur->next) {
                                Entry_Found = 1;
                                continue;
                        }
 			/* snip out this entry from the chain */
			*curp = cur->next;  

			temp = *after_bfd;
			*after_bfd = cur;
			(*after_bfd)->next = temp;

			if (verbose) {
				printf("m - %s\n", *files_to_move);
			}		
			Entry_Found = 1;
			break;
		}

		if (!Entry_Found) {
			fatal("No entry %s in archive %s!",
					*files_to_move, inarch->filename);
		}
	}
	write_archive();
}

void
replace_all_members()
{
        char **files_to_replace = NULL;
	bfd **curp;
        bfd *cur;
	char **Index;
        int no_of_members = 0;

        /* gather all members in the lib */
	for ( curp = &inarch->next; *curp; curp = &(cur->next)) {
	         cur = *curp;
                 no_of_members++;
        }
       
        if (no_of_members == 0) {
                /* force an empty library to be created */
                replace_members(files_to_replace);
                return;
        }

        files_to_replace = Index = (char ** ) xmalloc( (no_of_members + 1) 
						      * sizeof(char *));
	for ( curp = &inarch->next; *curp; curp = &(cur->next)) {
	         cur = *curp;
		 *Index = xmalloc( strlen(cur->filename) + 1 );
		 strcpy( *Index, cur->filename );
                 Index++;
	}
        *Index = NULL; /* indicate end of filename list */
	replace_members( files_to_replace );

	/* attempt to free */
	for ( Index = files_to_replace; Index && *Index; Index++) {
	        free (*Index);
        }  
	free (files_to_replace);  
}

void
ranlib_only(archname)
    char *archname;
{
	write_armap = 1;
	open_inarch(archname);
	write_archive();
	bfd_close(inarch);
}


