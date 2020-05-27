/* tell tm.h that this is special gcc960 version - not standard gcc */

#define	GCC960	1
#include "config.h"

#ifdef IMSTG
/* Compiler driver program for cross-compilation for Intel 960
   Copyright (C) 1987,1989 Free Software Foundation, Inc.
   Modified for i960 by Intel Corp.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

This paragraph is here to try to keep Sun CC from dying.
The number of chars here seems crucial!!!!  */


/* This program is the user interface to the C compiler and possibly to
other compilers.  It is used because compilation is a complicated procedure
which involves running several programs and passing temporary files between
them, forwarding the users switches to those programs selectively,
and deleting the temporary files at the end.

CC recognizes how to compile each input file by suffixes in the file names.
Once it knows which kind of compilation to perform, the procedure for
compilation is specified by a string called a "spec".

Specs are strings containing lines, each of which (if not blank)
is made up of a program name, and arguments separated by spaces.
The program name must be exact and start from root, since no path
is searched and it is unreliable to depend on the current working directory.
Redirection of input or output is not supported; the subprograms must
accept filenames saying what files to read and write.

In addition, the specs can contain %-sequences to substitute variable text
or for conditional text.  Here is a table of all defined %-sequences.
Note that spaces are not generated automatically around the results of
expanding these sequences; therefore, you can concatenate them together
or with constant text in a single argument.

 %%	substitute one % into the program name or argument.
 %i     substitute the name of the input file being processed.
 %b     substitute the basename of the input file being processed.
	This is the substring up to (and not including) the last period.
 %g'suf' Use the suffix provided and create a temporary file name.
	%g'suf' also has the same effect of %d.  Only one temporary file
        with the same suffix is ever created during a single gcc960
        invocation.  This is necessary since the specs don't tell
        whether the %g refers to the name for creation, or for use.
 %d	marks the argument containing or following the %d as a
	temporary file name, so that that file will be deleted if CC exits
	successfully.  Unlike %g, this contributes no text to the argument.
 %w	marks the argument containing or following the %w as the
	"output file" of this compilation.  This puts the argument
	into the sequence of arguments that %o will substitute later.
 %W{...}
        like %{...} but mark last argument supplied within
        as a file to be deleted on failure.
 %o	substitutes the names of all the output files, with spaces
	automatically placed around them.  You should write spaces
	around the %o as well or the results are undefined.
	%o is for use in the specs for running the linker.
	Input files whose names have no recognized suffix are not compiled
	at all, but they are included among the output files, so they will
	be linked.
 %p	substitutes the standard macro predefinitions for the
	current target machine.  Use this when running cpp.
 %P	like %p, but puts `__' before and after the name of each macro.
	This is for ANSI C.
 %s     current argument is the name of a library or startup file of some sort.
        Search for that file in a standard list of directories
	and substitute the full pathname found.
 %eSTR  Print STR as an error message.  STR is terminated by a newline.
        Use this when inconsistent options are detected.
 %fSTR  Print STR as an error message and abort the compilation. STR is
        terminated by a newline. Use this when inconsistent options are
        detected, and you don't want processing to continue.

 %a     process ASM_SPEC as a spec.
        This allows config.h to specify part of the spec for running as.
 %l     process LINK_SPEC as a spec.
 %L     process LIB_SPEC as a spec.
 %S     process STARTFILE_SPEC as a spec.  A capital S is actually used here.
 %c	process SIGNED_CHAR_CPP_SPEC as a spec.
 %Z	process SIGNED_CHAR_CC_SPEC as a spec.
 %C     process CPP_SPEC as a spec.  A capital C is actually used here.
 %1	process CC1_SPEC as a spec.
 %2	process CC1PLUS_SPEC as a spec.

 ***

 %A	substitute the architecture specification string in ARCH_SPEC
 %['e']	substitute environment variable 'e'
 %['e':X] substitute environment variable 'e' or X, if 'e' is not set

 %[~]	substitute environment variable defined by ENV_GNUBASE,
	or the flag-specified value for the exec base
 %[B]	substitute environment variable defined by ENV_GNUBIN
 %[L]	substitute environment variable defined by ENV_GNULIB
 %[I]	substitute environment variable defined by ENV_GNUINC
 %[D]   substitite environment variable defined by ENV_GNUPDB

 %[a]	substitute environment variable defined by ENV_GNUAS
 %[C]	substitute environment variable defined by ENV_GNUCPP
 %[1]	substitute environment variable defined by ENV_GNUCC1
 %[2]	substitute environment variable defined by ENV_GNUCC1PLUS
 %[l]	substitute environment variable defined by ENV_GNULD
 %[S]	substitute environment variable defined by ENV_GNUCRT
 %[M]	substitute environment variable defined by ENV_GNUCRT_M
 %[G]	substitute environment variable defined by ENV_GNUCRT_G
 %[b]	substitute environment variable defined by ENV_GNUCRT_B

 *** all of the above come in the ':X' form also

 %[@a]	substitute default name of assembler from GNUAS_DFLT
 %[@l]	substitute default name of linker from GNULD_DFLT
 %[@1]	substitite default name of compiler from GNUCC1_DFLT
 %[@2]	substitite default name of compiler from GNUCC1PLUS_DFLT
 %[@c]	substitute default name of cpp from GNUCPP_DFLT
 %[@S]	substitute default name of crt from GNUCRT_DFLT
 %[@M]	substitute default name of crtm from GNUCRTM_DFLT
 %[@G]	substitute default name of crtg from GNUCRTG_DFLT
 %[@b]	substitute default name of crtg from GNUCRTB_DFLT
 %[@I]	substitute default name of include dir from GNUINC_DFLT

 *** the following combine STANDARD_EXEC_PREFIX or STANDARD_LIB_PREFIX
	with the above to form a standard, default, last-gasp pathname

 %[?a]	substitute the standard path to assembler
 %[?l]	substitute the standard path to linker
 %[?1]	substitute the standard path to cc1
 %[?2]	substitute the standard path to cc1plus
 %[?c]	substitute the standard path to cpp
 %[?S]	substitute the standard path to crt.o
 %[?M]	substitute the standard path to crtm.o
 %[?G]	substitute the standard path to crtg.o
 %[?b]	substitute the standard path to crtb.o
 %[?I]	substitute the standard path to include dir

 ***
 %{@S'Y'} if 'S' is not a WORD_SWITCH_TAKES_ARGS, then read the spec line
	'Y' tag from the filename following the switch, looked up in the
	standard place, and interpolate the read spec line into this spec
 %{@S'Y':X} if 'S' is not a WORD_SWITCH_TAKES_ARGS, then read the spec line
        'Y' tag from the filename following the switch, looked up in the
        standard place, and interpolate the read spec line into this spec.
        If 'Y' tag is not found then substitute X.
 ***

 %{S}   substitutes the -S switch, if that switch was given to CC.
	If that switch was not specified, this substitutes nothing.
	Here S is a metasyntactic variable.
 %{S*}  substitutes all the switches specified to CC whose names start
	with -S.  This is used for -o, -D, -I, etc; switches that take
	arguments.  CC considers `-o foo' as being one switch whose
	name starts with `o'.  %{o*} would substitute this text,
	including the space; thus, two arguments would be generated.
 %{~S*}	substitutes the argument to switch 'S', but without the '-S' switch
	itself
 %{S:X} substitutes X, but only if the -S switch was given to CC.
 %{!S:X} substitutes X, but only if the -S switch was NOT given to CC.
 %{|S:X} like %{S:X}, but if no S switch, substitute `-'.
 %{|!S:X} like %{!S:X}, but if there is an S switch, substitute `-'.

The conditional text X in a %{S:X},%{!S:X} or a %{@S'Y':X} construct
may contain other nested % constructs or spaces, or even newlines.
They are processed as usual, as described above.

Note that it is built into CC which switches take arguments and which
do not.  You might think it would be useful to generalize this to
allow each compiler's spec to say which switches take arguments.  But
this cannot be done in a consistent fashion.  CC cannot even decide
which input files have been specified without knowing which switches
take arguments, and it must know which input files to compile in order
to tell which compilers to run.

CC also knows implicitly that arguments starting in `-l' are to
be treated as output files, and passed to the linker in their proper
position among the other output files.

*/

#ifdef DOS
#	include <errno.h>
#	include <process.h>
#	include <io.h>
#	include <fcntl.h>
#	include <gnudos.h>
#	define OPTION_INDICATORS	"-/"
#else
#	define OPTION_INDICATORS	"-"
extern char* get_960_tools_temp_file();
#endif

#define is_option_indicator(c)	(strchr(OPTION_INDICATORS,(c)) != NULL)

#include <stdio.h>
#include <sys/types.h>
#ifndef DOS
#include <sys/file.h>
#endif
#include <sys/stat.h>
#include <signal.h>
#include "obstack.h"

#define	isspace(c)  (((c) == ' ') || ((c) == '\t'))

#if defined (USG) || defined (DOS)
#ifndef R_OK
#define R_OK 4
#endif
#ifndef W_OK
#define W_OK 2
#endif
#ifndef X_OK
#define X_OK 1
#endif
#define vfork fork
#endif

#ifndef S_ISREG
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif

extern char *strchr();
extern char *getenv();

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free
extern int xmalloc ();
extern int xrealloc ();
extern void free ();
void fancy_abort ();

/* If a stage of compilation returns an exit status >= 1,
   compilation of that file ceases.  */

#define MIN_FATAL_STATUS 1

/* This is the obstack which we use to allocate many strings.  */

struct obstack obstack;

char *get_spec();
char *handle_braces ();
char *handle_brackets ();
char *save_string ();
char *concat ();
int do_spec ();
int do_spec_1 ();
static char *find_lib_file ();
static char *find_exec_file ();
static void print_help();

/* config.h can define ASM_SPEC to provide extra args to the assembler
   or extra switch-translations.  */
#ifndef ASM_SPEC
#define ASM_SPEC "%{@T'as'}"
#endif

/* config.h can define CPP_SPEC to provide extra args to the C preprocessor
   or extra switch-translations.  */
#ifndef CPP_SPEC
#define CPP_SPEC "%{@T'cpp'}"
#endif

/* config.h can define CC1_SPEC to provide extra args to cc1
   or extra switch-translations.  */
#ifndef CC1_SPEC
#define CC1_SPEC "%{@T'cc1'}"
#endif

/* config.h can define CC1PLUS_SPEC to provide extra args to cc1plus
   or extra switch-translations.  */
#ifndef CC1PLUS_SPEC
#define CC1PLUS_SPEC "%{@T'cc1plus'}"
#endif

/* config.h can define LINK_SPEC to provide extra args to the linker
   or extra switch-translations.  */
#ifndef LINK_SPEC
#define LINK_SPEC "%{@T'ld'}"
#endif

/* config.h can define LIB_SPEC to override the default libraries.  */
#ifndef LIB_SPEC
#define LIB_SPEC "%{!p:%{!pg:-lc}}%{p:-lc_p}%{pg:-lc_p}%{@T'lib'}"
#endif

/* config.h can define STARTFILE_SPEC to override the default crt0 files.  */
#ifndef STARTFILE_SPEC
#define STARTFILE_SPEC  \
  "%{!crt:%{pg:%[G:%[L/%[@G]:%[~/lib/gcrt0.o:%[?G]]]]}\
   %{!pg:%{p:%[M:%[L/%[@M]:%[~/lib/mcrt0.o:%[?M]]]]}\
   %{!p:%[S:%[L/%[@S]:%[~/crt0.o:%[?C]]]]}}}"
#endif

#ifndef SIGNED_CHAR_SWITCH_P
#define SIGNED_CHAR_SWITCH_P(p) \
  (strcmp(p, "fsigned-char") == 0 ||\
   strcmp(p, "fno-unsigned-char") == 0 ||\
   strcmp(p, "mic-compat") == 0 ||\
   strcmp(p, "mic2.0-compat") == 0 ||\
   strcmp(p, "mic3.0-compat") == 0)
#endif

#ifndef UNSIGNED_CHAR_SWITCH_P
#define UNSIGNED_CHAR_SWITCH_P(p) \
  (strcmp(p, "fno-signed-char") == 0 ||\
   strcmp(p, "funsigned-char") == 0)
#endif

#ifndef ABI_SWITCH_P
#define ABI_SWITCH_P(p) \
  (strcmp(p, "mabi") == 0)
#endif

/* This spec is used for telling cpp whether char is signed or not.  */
#ifndef SIGNED_CHAR_CPP_SPEC
#define SIGNED_CHAR_CPP_SPEC  \
  (signed_char_flag ? "" : "-D__CHAR_UNSIGNED__")
#endif

#ifndef SIGNED_CHAR_CC_SPEC
#define SIGNED_CHAR_CC_SPEC \
  (signed_char_flag ? "-fsigned-char" : "-funsigned-char")
#endif

#ifndef STANDARD_EXEC_PREFIX
#define STANDARD_EXEC_PREFIX "/usr/local/bin/gnu"
#endif

#ifndef STANDARD_LIB_PREFIX
#define STANDARD_LIB_PREFIX "/usr/local/lib/gnu"
#endif

#ifndef STANDARD_INC_PREFIX
#define STANDARD_INC_PREFIX "/usr/local/lib/gnu/include"
#endif

#ifndef	GNUAS_DFLT
#define	GNUAS_DFLT	"as"
#endif
#ifndef GNULD_DFLT
#define GNULD_DFLT	"ld"
#endif
#ifndef	GNUCC1_DFLT
#define	GNUCC1_DFLT	"cc1"
#endif
#ifndef	GNUCC1PLUS_DFLT
#define	GNUCC1PLUS_DFLT	"cc1plus"
#endif
#ifndef	GNUCPP_DFLT
#define	GNUCPP_DFLT	"gcc-cpp"
#endif
#ifndef	GNUCRT_DFLT
#define	GNUCRT_DFLT	"crt.o"
#endif
#ifndef	GNUCRTM_DFLT
#define	GNUCRTM_DFLT	"crtm.o"
#endif
#ifndef	GNUCRTG_DFLT
#define	GNUCRTG_DFLT	"crtg.o"
#endif
#ifndef	GNUCRTB_DFLT
#define	GNUCRTB_DFLT	"crtb.o"
#endif
#ifndef	GNUINC_DFLT
#define	GNUINC_DFLT	"include"
#endif
#ifndef GNUTMP_DFLT
#if defined(IMSTG) && defined(DOS)
#define GNUTMP_DFLT     "./"
#else
#define GNUTMP_DFLT     "/tmp"
#endif
#endif

#ifndef	ENV_GNUAS
#define	ENV_GNUAS	"GNUAS"
#endif
#ifndef ENV_GNUCPP
#define	ENV_GNUCPP	"GNUCPP"
#endif
#ifndef	ENV_GNUCC1
#define	ENV_GNUCC1	"GNUCC1"
#endif
#ifndef	ENV_GNUCC1PLUS
#define	ENV_GNUCC1PLUS	"GNUCC1PLUS"
#endif
#ifndef	ENV_GNULD
#define	ENV_GNULD	"GNULD"
#endif
#ifndef	ENV_GNUCRT
#define	ENV_GNUCRT	"GNUCRT"
#endif
#ifndef	ENV_GNUCRT_G
#define	ENV_GNUCRT_G	"GNUCRT_G"
#endif
#ifndef	ENV_GNUCRT_M
#define	ENV_GNUCRT_M	"GNUCRT_M"
#endif
#ifndef	ENV_GNUCRT_B
#define	ENV_GNUCRT_B	"GNUCRT_B"
#endif
#ifndef	ENV_GNUBASE
#define	ENV_GNUBASE	"GNUBASE"
#endif
#ifndef	ENV_GNUBIN
#define	ENV_GNUBIN	"GNUBIN"
#endif
#ifndef	ENV_GNULIB
#define	ENV_GNULIB	"GNULIB"
#endif
#ifndef	ENV_GNUINC
#define	ENV_GNUINC	"GNUINC"
#endif
#ifndef ENV_GNUTMP
#define ENV_GNUTMP      "GNUTMP"
#endif

int save_temps_flag;

int signed_char_flag = DEFAULT_SIGNED_CHAR;
int char_sign_specified = 0;

int abi_specified = 0;

/* This structure says how to run one compiler, and when to do so.  */

struct compiler
{
  char *suffix;			/* Use this compiler for input files
				   whose names end in this suffix.  */
  char *spec[4];		/* To use this compiler, concatenate these
                                   specs and pass to do_spec.  */
};

/* Here are the specs for compiling files with various known suffixes.
   A file that does not end in any of these suffixes will be passed
   unchanged to the loader and nothing else will be done to it.  */

struct compiler compilers[] =
{
{ ".c",
"%A %[C:%[L/%[@c]:%[~/lib/%[@c]:%[?c]]]]\
 %{nostdinc} %{E:%{C} %{P}} %{v} %{V:%{!v:-v}} %{!ansi:%p} %P %{\\} %c\
 %{O0:-D__NO_INLINE} %{!O*:-D__NO_INLINE}\
 %{fdb:-U__NO_INLINE -D__OPTIMIZE__} %{fprof:-U__NO_INLINE -D__OPTIMIZE__}\
 %{O1:-U__NO_INLINE -D__OPTIMIZE__} %{O2:-U__NO_INLINE -D__OPTIMIZE__}\
 %{O3:-U__NO_INLINE -D__OPTIMIZE__} %{O4:-U__NO_INLINE -D__OPTIMIZE__}\
 %{O3_1:-U__NO_INLINE -D__OPTIMIZE__} %{O3_2:-U__NO_INLINE -D__OPTIMIZE__}\
 %{O4_1:-U__NO_INLINE -D__OPTIMIZE__} %{O4_2:-U__NO_INLINE -D__OPTIMIZE__}\
 -undef -D__GNUC__=2 %{ansi:-trigraphs -$ -D__STRICT_ANSI__}\
 -lang-c\
 %{D*} %{U*} %{I*} %{M*:%{!E:%{P}}} %{M} %{MM} %{MD:-MD %b.d} %{MMD:-MMD %b.d}\
 %{dM} %{dN} %{dD} %{imacros*} %{include*} %{fmix-asm:-sinfo %d%g'sin'}\
 %{ffancy-errors:-ffancy-errors -sinfo %d%g'sin'} %{clist*}\
 %{clist:%{E:-outz %b.L} -sinfo %d%g'sin'} %{traditional} %{pedantic} %{ic960}\
 %{pedantic-errors} %{Wcomment} %{Wtrigraphs} %{Wall} %{w0} %{w1} %{w} %C\
 %{Felf:%{g:-sinfo %d%g'sin'} %{g1:-sinfo %d%g'sin'} %{g2:-sinfo %d%g'sin'}\
 %{g3:-sinfo %d%g'sin'}}\
 %i %{!M*:%{!E:%g'i'}}%{E:%W{o*}}%{M*:%W{o*}} |\n",
"%{!M*:%{!E:%[1:%[L/%[@1]:%[~/lib/%[@1]:%[?1]]]] %g'i' %1\
 %{O0:-O0}\
 %{fdb:-O1} %{fprof:-O1}\
 %{O1:-O1} %{O2:-O2} %{O3:-O3} %{O4:-O4}\
 %{O3_1:-O3_1} %{O3_2:-O3_2}\
 %{O4_1:-O4_1} %{O4_2:-O4_2}\
 %{!Q:-quiet} -dumpbase %b %{fmix-asm:-sinfo %d%g'sin'}\
 %{Felf:%{g:-sinfo %d%g'sin'} %{g1:-sinfo %d%g'sin'} %{g2:-sinfo %d%g'sin'}\
 %{g3:-sinfo %d%g'sin'}}\
 %{ffancy-errors:-sinfo %d%g'sin'} %{clist*}\
 %{clist:-outz %b.L -tmpz %d%g'ltm' -sinfo %d%g'sin'} %{bsize*} %{bname}\
 %{!bname:-bname_tmp %d%g'btm'} %{m*} %{f*} %Z\
 %{W*} %{w0} %{w1} %{w} %{pedantic} %{ansi} %{traditional} %{ic960}\
 %{pedantic-errors} %{v:-version} %{V:%{!v:-version}}\
 %{dy} %{dr} %{dx} %{dj} %{dG} %{dF} %{dH} %{dI} %{ds} %{dZ}\
 %{dL} %{dB} %{dC} %{dP} %{dt} %{df} %{dc} %{dS} %{dl} %{dg}\
 %{dR} %{dJ} %{dX} %{dT} %{da} %{dm} %{dp}",
"%{Z} %{!Z: %[D$: -Z %[D]]}\
 %{idb: %eidb option no longer used by gcc960 - see -Z option}\
 %{iprof: %eiprof option no longer used by gcc960 - see -Z option}\
 %{S:%W{o*}%{!o*:-o %b.s}}%{!S:-o %g's'} |\n",
"%{!S:%[a:%[B/%[@a]:%[~/bin/%[@a]:%[?a]]]] %{v:-V} %{V:%{!v:-V}} %a\
 %g's' %{c:%W{o*}%{!o*:-o %w%b.o}}%{!c:-o %d%w%b.o}\n }}}"
},

{ ".cc",
"%A %[C:%[L/%[@c]:%[~/lib/%[@c]:%[?c]]]]\
 %{nostdinc} %{E:%{C} %{P}} %{v} %{V:%{!v:-v}} %{!ansi:%p} %P %{\\} %c\
 %{O0:-D__NO_INLINE} %{!O*:-D__NO_INLINE}\
 %{fdb:-U__NO_INLINE -D__OPTIMIZE__} %{fprof:-U__NO_INLINE -D__OPTIMIZE__}\
 %{O1:-U__NO_INLINE -D__OPTIMIZE__} %{O2:-U__NO_INLINE -D__OPTIMIZE__}\
 %{O3:-U__NO_INLINE -D__OPTIMIZE__} %{O4:-U__NO_INLINE -D__OPTIMIZE__}\
 %{O3_1:-U__NO_INLINE -D__OPTIMIZE__} %{O3_2:-U__NO_INLINE -D__OPTIMIZE__}\
 %{O4_1:-U__NO_INLINE -D__OPTIMIZE__} %{O4_2:-U__NO_INLINE -D__OPTIMIZE__}\
 -undef -D__GNUC__=2 %{ansi:-trigraphs -$ -D__STRICT_ANSI__}\
 -lang-c++ -D__GNUG__=2 -D__cplusplus\
 %{D*} %{U*} %{I*} %{M*:%{!E:%{P}}} %{M} %{MM} %{MD:-MD %b.d} %{MMD:-MMD %b.d}\
 %{dM} %{dN} %{dD} %{imacros*} %{include*} %{fmix-asm:-sinfo %d%g'sin'}\
 %{ffancy-errors:-ffancy-errors -sinfo %d%g'sin'} %{clist*}\
 %{clist:%{E:-outz %b.L} -sinfo %d%g'sin'} %{traditional} %{pedantic} %{ic960}\
 %{pedantic-errors} %{Wcomment} %{Wtrigraphs} %{Wall} %{w0} %{w1} %{w} %C\
 %{Felf:%{g:-sinfo %d%g'sin'} %{g1:-sinfo %d%g'sin'} %{g2:-sinfo %d%g'sin'}\
 %{g3:-sinfo %d%g'sin'}}\
 %i %{!M*:%{!E:%g'ii'}}%{E:%W{o*}}%{M*:%W{o*}} |\n",
"%{!M*:%{!E:%[2:%[L/%[@2]:%[~/lib/%[@2]:%[?2]]]] %g'ii' %2\
 %{O0:-O0}\
 %{fdb:-O1} %{fprof:-O1}\
 %{O1:-O1} %{O2:-O2} %{O3:-O3} %{O4:-O4}\
 %{O3_1:-O3_1} %{O3_2:-O3_2}\
 %{O4_1:-O4_1} %{O4_2:-O4_2}\
 %{!Q:-quiet} -dumpbase %b %{fmix-asm:-sinfo %d%g'sin'}\
 %{Felf:%{g:-sinfo %d%g'sin'} %{g1:-sinfo %d%g'sin'} %{g2:-sinfo %d%g'sin'}\
 %{g3:-sinfo %d%g'sin'}}\
 %{ffancy-errors:-sinfo %d%g'sin'} %{clist*}\
 %{clist:-outz %b.L -tmpz %d%g'ltm' -sinfo %d%g'sin'} %{bsize*} %{bname}\
 %{!bname:-bname_tmp %d%g'btm'} %{m*} %{f*} %Z\
 %{W*} %{w0} %{w1} %{w} %{pedantic} %{ansi} %{traditional} %{ic960}\
 %{pedantic-errors} %{v:-version} %{V:%{!v:-version}}\
 %{dy} %{dr} %{dx} %{dj} %{dG} %{dF} %{dH} %{dI} %{ds} %{dZ}\
 %{dL} %{dB} %{dC} %{dP} %{dt} %{df} %{dc} %{dS} %{dl} %{dg}\
 %{dR} %{dJ} %{dX} %{dT} %{da} %{dm} %{dp}",
"%{Z} %{!Z: %[D$: -Z %[D]]}\
 %{idb: %eidb option no longer used by gcc960 - see -Z option}\
 %{iprof: %eiprof option no longer used by gcc960 - see -Z option}\
 %{S:%W{o*}%{!o*:-o %b.s}}%{!S:-o %g's'} |\n",
"%{!S:%[a:%[B/%[@a]:%[~/bin/%[@a]:%[?a]]]] %{v:-V} %{V:%{!v:-V}} %a\
 %g's' %{c:%W{o*}%{!o*:-o %w%b.o}}%{!c:-o %d%w%b.o}\n }}}"
},

{ ".i",
"%A %[1:%[L/%[@1]:%[~/lib/%[@1]:%[?1]]]] %i %1\
 %{O0:-O0}\
 %{fdb:-O1} %{fprof:-O1}\
 %{O1:-O1} %{O2:-O2} %{O3:-O3} %{O4:-O4}\
 %{O3_1:-O3_1} %{O3_2:-O3_2}\
 %{O4_1:-O4_1} %{O4_2:-O4_2}\
 %{!Q:-quiet} -dumpbase %b\
 %{fmix-asm:-sinfo %d%g'sin'} %{ffancy-errors:-sinfo %d%g'sin'} %{clist*}\
 %{clist:-outz %b.L -tmpz %d%g'ltm' -sinfo %d%g'sin'} %{bsize*} %{bname}\
 %{!bname:-bname_tmp %d%g'btm'} %{m*} %{f*} %Z\
 %{W*} %{w} %{pedantic} %{ansi} %{traditional} %{ic960} %{pedantic-errors}\
 %{v:-version} %{V:%{!v:-version}} %{dy} %{dr} %{dx} %{dj} %{dG} %{dF} %{dH}\
 %{dI} %{ds} %{dZ} %{dL} %{dB} %{dC} %{dP} %{dt} %{df} %{dc} %{dS} %{dl} %{dg}\
 %{dR} %{dJ} %{dX} %{dT} %{da} %{dm} %{dp}",
"%{Z} %{!Z: %[D$: -Z %[D]]}\
 %{idb: %eidb option no longer used by gcc960 - see -Z option}\
 %{iprof: %eiprof option no longer used by gcc960 - see -Z option}\
 %{S:%W{o*}%{!o*:-o %b.s}}%{!S:-o %g's'} |\n\
 %{!S:%[a:%[B/%[@a]:%[~/bin/%[@a]:%[?a]]]] %{v:-V} %{V:%{!v:-V}}\
 %a %g's' %{c:%W{o*}%{!o*:-o %w%b.o}}%{!c:-o %d%w%b.o}\n }"
},

{ ".ii",
"%A %[2:%[L/%[@2]:%[~/lib/%[@2]:%[?2]]]] %i %2\
 %{O0:-O0}\
 %{fdb:-O1} %{fprof:-O1}\
 %{O1:-O1} %{O2:-O2} %{O3:-O3} %{O4:-O4}\
 %{O3_1:-O3_1} %{O3_2:-O3_2}\
 %{O4_1:-O4_1} %{O4_2:-O4_2}\
 %{!Q:-quiet} -dumpbase %b\
 %{fmix-asm:-sinfo %d%g'sin'} %{ffancy-errors:-sinfo %d%g'sin'} %{clist*}\
 %{clist:-outz %b.L -tmpz %d%g'ltm' -sinfo %d%g'sin'} %{bsize*} %{bname}\
 %{!bname:-bname_tmp %d%g'btm'} %{m*} %{f*} %Z\
 %{W*} %{w} %{pedantic} %{ansi} %{traditional} %{ic960} %{pedantic-errors}\
 %{v:-version} %{V:%{!v:-version}} %{dy} %{dr} %{dx} %{dj} %{dG} %{dF} %{dH}\
 %{dI} %{ds} %{dZ} %{dL} %{dB} %{dC} %{dP} %{dt} %{df} %{dc} %{dS} %{dl} %{dg}\
 %{dR} %{dJ} %{dX} %{dT} %{da} %{dm} %{dp}",
"%{Z} %{!Z: %[D$: -Z %[D]]}\
 %{idb: %eidb option no longer used by gcc960 - see -Z option}\
 %{iprof: %eiprof option no longer used by gcc960 - see -Z option}\
 %{S:%W{o*}%{!o*:-o %b.s}}%{!S:-o %g's'} |\n\
 %{!S:%[a:%[B/%[@a]:%[~/bin/%[@a]:%[?a]]]] %{v:-V} %{V:%{!v:-V}}\
 %a %g's' %{c:%W{o*}%{!o*:-o %w%b.o}}%{!c:-o %d%w%b.o}\n }"
},

{ ".s",
"%A %{!S:%[a:%[B/%[@a]:%[~/bin/%[@a]:%[?a]]]] %{v:-V} %{V:%{!v:-V}}\
 %a %i %{c:%W{o*}%{!o*:-o %w%b.o}}%{!c:-o %d%w%b.o}\n }"
},

{ ".S",
"%A %[C:%[L/%[@c]:%[~/lib/%[@c]:%[?c]]]] %{nostdinc} %{E:%{C} %{P}}\
 %{v} %{V:%{!v:-v}} %{D*} %{U*} %{I*}\
 %{M*:%{!E:%{P}}} %{M} %{MM} %{MD:-MD %b.d} %{MMD:-MMD %b.d}\
 %{dM} %{dN} %{dD} %{imacros*} %{include*}\
 -undef -D__GNUC__=2 -$ %p %P\
 %{clist*} %{clist:-outz %g'L'}\
 %c %{O*:%{!O0:-D__OPTIMIZE__}} %{traditional} %{pedantic} %{ic960}\
 %{pedantic-errors} %{Wcomment} %{Wtrigraphs} %{Wall} %{w0} %{w1} %{w} %C \
 %i %{!M*:%{!E:%g's'}}%{E:%W{o*}}%{M*:%W{o*}} |\n\
 %{!M*:%{!E:%{!S:%[a:%[B/%[@a]:%[~/bin/%[@a]:%[?a]]]] %{v:-V} %{V:%{!v:-V}}\
 %a %g's'\
 %{c:%W{o*}%{!o*:-o %w%b.o}}%{!c:-o %d%w%b.o}\n }}}"
},

/* Mark end of table */
{0, 0}
};

/* Here is the spec for running the linker, after compiling all files.  */

char *link_spec =
	"%{!c:%{!M*:%{!E:%{!S:%[l:%[B/%[@l]:%[~/bin/%[@l]:%[?l]]]] %{o*}\
	%{gcdm*} %{v:-V} %{V:%{!v:-V}} %l %{!crt:%S} %{L*} %o %{!nostdlib:%L}\n }}}}";


/* Flag indicating whether we should print the command and arguments */

unsigned char vflag;
unsigned char Vflag;

/* Record the names of temporary files we tell compilers to write,
   and delete them at the end of the run.  */

/* Define the list of temporary files to delete.  */

struct temp_file
{
  char *name;
  struct temp_file *next;
};

/* Queue of files to delete on success or failure of compilation.  */
static struct temp_file *always_delete_queue;

/* Queue of files to delete on failure of compilation.  */
static struct temp_file *failure_delete_queue;

/* Queue of all temp files created by gcc960 */
static struct temp_file *all_temp_files;

static struct temp_file *
find_temp_with_suffix(name)
char * name;
{
  register struct temp_file *temp;
  register char *suffix = strchr(name, '.');
  register char *srch_suff;

  for (temp = all_temp_files; temp; temp = temp->next)
  {
    srch_suff = strchr(temp->name, '.');
    if (srch_suff == 0 && suffix == 0)
      return temp;

    if (suffix == 0 || srch_suff == 0)
      continue;

    if (! strcmp (suffix, srch_suff))
      return temp;
  }

  return 0;
}

static
char *get_temp_filename(suffix)
char *suffix;
{
  char buf[13];  /* names are 8.3 so this is plenty */
  register struct temp_file *temp;
  char *tname;

  sprintf (buf, "ccXXXXXX.%.3s", suffix);

  if ((temp = find_temp_with_suffix(buf)) != 0)
    return temp->name;

  tname = get_960_tools_temp_file(buf, xmalloc);
  return tname;
}

/* Record FILENAME as a file to be deleted automatically.
   ALWAYS_DELETE nonzero means delete it if all compilation succeeds;
   otherwise delete it in any case.
   FAIL_DELETE nonzero means delete it if a compilation step fails;
   otherwise delete it in any case.  */

static void
record_temp_file (filename, always_delete, fail_delete)
     char *filename;
     int always_delete;
     int fail_delete;
{
  register char *name;
  register struct temp_file *temp;
  name = (char *)xmalloc (strlen (filename) + 1);
  strcpy (name, filename);

  if (always_delete)
    {
      for (temp = always_delete_queue; temp; temp = temp->next)
        if (! strcmp (name, temp->name))
          goto already1;
      temp = (struct temp_file *) xmalloc (sizeof (struct temp_file));
      temp->next = always_delete_queue;
      temp->name = name;
      always_delete_queue = temp;
    already1:;
    }

  if (fail_delete)
    {
      for (temp = failure_delete_queue; temp; temp = temp->next)
        if (! strcmp (name, temp->name))
          goto already2;
      temp = (struct temp_file *) xmalloc (sizeof (struct temp_file));
      temp->next = failure_delete_queue;
      temp->name = name;
      failure_delete_queue = temp;
    already2:;
    }

  if ((temp = find_temp_with_suffix(name)) == 0)
  {
    temp = (struct temp_file *) xmalloc (sizeof (struct temp_file));
    temp->next = all_temp_files;
    temp->name = name;
    all_temp_files = temp;
  }
}

/* Delete all the temporary files whose names we previously recorded.  */

static void
delete_temp_files ()
{
  register struct temp_file *temp;

  for (temp = always_delete_queue; temp; temp = temp->next)
    {
#ifdef DEBUG
      int i;
      printf ("Delete %s? (y or n) ", temp->name);
      fflush (stdout);
      i = getchar ();
      if (i != '\n')
        while (getchar () != '\n') ;
      if (i == 'y' || i == 'Y')
#endif /* DEBUG */
        {
          struct stat st;
          if (stat (temp->name, &st) >= 0)
            {
              /* Delete only ordinary files.  */
              if (S_ISREG (st.st_mode))
                if (unlink (temp->name) < 0)
                  if (vflag)
                    perror_with_name (temp->name);
            }
        }
    }

  always_delete_queue = 0;
}

/* Delete all the files to be deleted on error.  */

static void
delete_failure_queue ()
{
  register struct temp_file *temp;

  for (temp = failure_delete_queue; temp; temp = temp->next)
    {
#ifdef DEBUG
      int i;
      printf ("Delete %s? (y or n) ", temp->name);
      fflush (stdout);
      i = getchar ();
      if (i != '\n')
        while (getchar () != '\n') ;
      if (i == 'y' || i == 'Y')
#endif /* DEBUG */
        {
          if (unlink (temp->name) < 0)
            if (vflag)
              perror_with_name (temp->name);
        }
    }
}

static void
clear_failure_queue ()
{
  failure_delete_queue = 0;
}

/* Look up an environment variable.
 * Return NULL if either the variable is undefined or it's defined as a NULL
 * string;  otherwise, return a pointer to the definition.
 */
char *
genv( s )
	char *s;
{
	char *p;

	if (p = getenv(s)) {
		if (*p == '\0') {
			p = 0;
		}
	}
	return p;
}


/* Accumulate a command (program name and args), and run it.  */

/* Vector of pointers to arguments in the current line of specifications.  */

char **argbuf;

/* Number of elements allocated in argbuf.  */

int argbuf_length;

/* Number of elements in argbuf currently in use (containing args).  */

int argbuf_index;

unsigned char help_flag;

/* Default prefixes to attach to command names.  */

/* Clear out the vector of arguments (after a command is executed).  */

void
clear_args ()
{
  argbuf_index = 0;
}

/* Add one argument to the vector at the end.
   This is done when a space is seen or at the end of the line.
   If DELETE_ALWAYS is nonzero, the arg is a filename
    and the file should be deleted eventually.
   If DELETE_FAILURE is nonzero, the arg is a filename
    and the file should be deleted if this compilation fails.  */

static void
store_arg (arg, delete_always, delete_failure)
     char *arg;
     int delete_always, delete_failure;
{
  if (argbuf_index + 1 == argbuf_length)
    {
      argbuf = (char **) xrealloc (argbuf, (argbuf_length *= 2) * sizeof (char *
));
    }

  argbuf[argbuf_index++] = arg;
  argbuf[argbuf_index] = 0;

  if (delete_always || delete_failure)
    record_temp_file (arg, delete_always, delete_failure);
}

/* Validate the exec file path */
/* Return 0 if not found, otherwise return its name */

static char *
find_exec_file (prog)
     char *prog;
{
  if (access(prog,X_OK) != 0) {
	pfatal_with_name(prog);
	return 0;
  }
  return(prog);

}

/* Fork one piped subcommand.  FUNC is the system call to use
   (either execv or execvp).  ARGV is the arg vector to use.
   NOT_LAST is nonzero if this is not the last subcommand
   (i.e. its output should be piped to the next one.)  */

static int
pexecute (func, program, argv, not_last)
     char *program;
     int (*func)();
     char *argv[];
     int not_last;
{
  int status = -1;
  int pid;
  static char *c960_cmd_name = 0;
  char **nargv = 0;
  int nargc = 0;

  if (c960_cmd_name == 0)
    c960_cmd_name = get_temp_filename("gt1");

  for (nargc = 0; argv[nargc] != 0; nargc++) ;

  nargv = (char **) xmalloc(sizeof(char *) * (nargc + 3));

  for (nargc = 0; argv[nargc] != 0; nargc++)
    nargv[nargc] = argv[nargc];

  nargv[nargc++] = "-c960";
  nargv[nargc++] = c960_cmd_name;
  nargv[nargc] = 0;

  if (vflag)
  {
    for (argv = nargv; *argv != 0; argv++)
      fprintf (stderr, " %s", *argv);
    fprintf (stderr, "\n");
    fflush (stderr);
  }

#ifdef DOS
  /* Do special argv handling for DOS. */
  argv = check_dos_args(nargv);
 
  /* Currently, there is no way to get the process id of the child
   * in DOS.  The best thing that can be returned is the exit
   * status of the child.
   */

  status = spawnvp(P_WAIT, program, argv);

  /*
   * no multi processing on DOS so we can delete any temp response file
   */
  delete_response_file ();
#else
  argv = nargv;

  pid = vfork ();

  switch (pid)
  {
    case -1:
      pfatal_with_name ("vfork");
      break;

    case 0: /* child */
      /* Exec the program.  */

      (*func) (program, argv);
      exit (-1);
      /* NOTREACHED */

    default: ;
      /* Wait for our child to terminate */
      while (wait(&status) != pid) ;
                                ;
      if ((status & 0x7F) != 0)
      {
        (void) unlink(c960_cmd_name);
	fatal ("Program %s got fatal signal %d.", argv[0], (status & 0x7F));
      }

      if (((status & 0xFF00) >> 8) >= MIN_FATAL_STATUS)
	status = -1;
      break;
  }
#endif

  free(nargv);

  if (status == 0)
  {
    char *argv[6];
    int argc = 0;

    argv[argc++] = "cc_x960";
    argv[argc++] = c960_cmd_name;
    if (vflag)
      argv[argc++] = "-v";
    argv[argc] = 0;

    status = db_x960(argc, argv);
  }
  (void) unlink(c960_cmd_name);

  return status;
}

/* Execute the command specified by the arguments on the current line of spec.
   When using pipes, this includes several piped-together commands
   with `|' between them.

   Return 0 if successful, -1 if failed.  */

int
execute ()
{
  int i, j;
  int n_commands;		/* # of command.  */
  char *string;
  struct command
    {
      char *prog;		/* program name.  */
      char **argv;		/* vector of args.  */
      int pid;			/* pid of process for this command.  */
    };

  struct command *commands;	/* each command buffer with above info.  */

  /* Count # of piped commands.  */
  for (n_commands = 1, i = 0; i < argbuf_index; i++)
    if (strcmp (argbuf[i], "|") == 0)
      n_commands++;

  /* Get storage for each command.  */
  commands
    = (struct command *) xmalloc(n_commands * sizeof (struct command));

  /* Split argbuf into its separate piped processes,
     and record info about each one.
     Also search for the programs that are to be run.  */

  commands[0].prog = argbuf[0]; /* first command.  */
  commands[0].argv = &argbuf[0];
  string = find_exec_file (commands[0].prog);
  if (string)
    commands[0].argv[0] = string;

  for (n_commands = 1, i = 0; i < argbuf_index; i++)
    if (strcmp (argbuf[i], "|") == 0)
      {				/* each command.  */
	argbuf[i] = 0;	/* termination of command args.  */
	commands[n_commands].prog = argbuf[i + 1];
	commands[n_commands].argv = &argbuf[i + 1];
	string = find_exec_file (commands[n_commands].prog);
	if (string)
	  commands[n_commands].argv[0] = string;
	n_commands++;
      }

  argbuf[argbuf_index] = 0;

  /* Run each piped subprocess.  */

  for (i = 0; i < n_commands; i++)
    {
      extern int execv(), execvp();
      char *string = commands[i].argv[0];

      commands[i].pid = pexecute ((string != commands[i].prog ? execv : execvp),
				  string, commands[i].argv,
				  i + 1 < n_commands);

      if (string != commands[i].prog)
	free (string);
    }

  {
    int ret_code = 0;

    /* Place the exit status of the child into ret_code. */
    /* This had to be passed through the pid due to DOS limitations. */
    ret_code = 0;
    for (i = 0; i < n_commands; i++)
       if ( commands[i].pid != 0 )
           ret_code = -1;

    return ret_code;
  }
}

/* Find all the switches given to us
   and make a vector describing them.
   The elements of the vector a strings, one per switch given.
   If a switch uses the following argument, then the `part1' field
   is the switch itself and the `part2' field is the following argument.  */

struct switchstr
{
  char *part1;
  char *part2;
};

struct switchstr *switches;

int n_switches;

/* Also a vector of input files specified.  */

char **infiles;

int n_infiles;

/* And a vector of corresponding output files is made up later.  */

char **outfiles;

char *
make_switch (p1, s1, p2, s2)
     char *p1;
     int s1;
     char *p2;
     int s2;
{
  register char *new;
  if (p2 && s2 == 0)
    s2 = strlen (p2);
  new = (char *) xmalloc (s1 + s2 + 2);
  bcopy (p1, new, s1);
  if (p2)
    {
      new[s1++] = ' ';
      bcopy (p2, new + s1, s2);
    }
  new[s1 + s2] = 0;
  return new;
}

/* Create the vector `switches' and its contents.
   Store its length in `n_switches'.  */

void
process_command (argc, argv)
     int argc;
     char **argv;
{
  int seenTswitch = 0;
  char *Tspec;
  char *Targv[100];
  int Targc = 0;
  register int i;
  n_switches = 0;
  n_infiles = 0;

  /* Scan argv twice.  Here, the first time, just count how many switches
     there will be in their vector, and how many input files in theirs.
     Here we also parse the switches that cc itself uses (e.g. -v).  */

  for (i = 1; i < argc; i++)
    {
      if (is_option_indicator(argv[i][0]) && argv[i][1] != 'l')
	{
	  register char *p = &argv[i][1];
	  register int c = *p;

          if (ABI_SWITCH_P(p))
          {
            if (char_sign_specified && signed_char_flag == 0)
              fatal("Illegal option combination: -mabi and -funsigned-char");
            signed_char_flag = 1;
            abi_specified = 1;
            char_sign_specified = 1;
          }
          else if (SIGNED_CHAR_SWITCH_P(p))
          {
            signed_char_flag = 1;
            char_sign_specified = 1;
          }
          else if (UNSIGNED_CHAR_SWITCH_P(p))
          {
            if (abi_specified)
              fatal("Illegal option combination: -mabi and -funsigned-char");
            signed_char_flag = 0;
            char_sign_specified = 1;
          }

	  switch (c)
	    {
	    case 'v':	/* Print our subcommands and print versions.  */
	      vflag++;
	      n_switches++;
	      break;

            case 'V':
              Vflag++;
              n_switches++;
              break;

            case 'h':
              if (strcmp(p, "h") == 0 || strcmp(p, "help") == 0)
              {
                help_flag = 1;
                break;
              }
              goto try_other_arg;

            case 's':
              if (strcmp(p, "save-temps") == 0)
              {
                save_temps_flag = 1;
                break;
              }
              goto try_other_arg;

	    case 'T':	/* special behaviour for -T target switch */
		if (!WORD_SWITCH_TAKES_ARG(p)) {

			if (seenTswitch) {
				fatal("multiple -T switches");
				return;
			}
			seenTswitch++;
			if (argv[i][2] == '\0') {
				fatal("no target specified for -T switch");
				return;
			}
			Tspec = get_spec(&argv[i][2],"gcc:");
			{
			register char *p;
			register char *x;
			Targc = 0;
			x = (char *) xmalloc(strlen(Tspec)+1);
			strcpy(x,Tspec);
			Targv[Targc++] = x;
			for (p=x; *p; p++) {
				if (isspace(*p)) {
					*p = '\0';
				} else if (p != x && p[-1] == '\0') {
					Targv[Targc++] = p;
				}
			}
			Targv[Targc] = 0;
			}
			n_switches++;
			break;
		}
		/* FALLTHROUGH */

	    default:
              try_other_arg: ;

	      n_switches++;

	      if (SWITCH_TAKES_ARG (c) && p[1] == 0)
		i++;
	      else if (WORD_SWITCH_TAKES_ARG (p))
		i++;
	      break;
            }
	}
      else
	n_infiles++;
    }

  /* Then create the space for the vectors and scan again.  */

  switches = ((struct switchstr *)
	      xmalloc ((n_switches + Targc + 3) * sizeof (struct switchstr)));
  infiles = (char **) xmalloc ((n_infiles + Targc + 3) * sizeof (char *));
  n_switches = 0;
  n_infiles = 0;

  /* This, time, copy the text of each switch and store a pointer
     to the copy in the vector of switches.
     Store all the infiles in their vector.  */

  if (Targc)
  {
    for (i = 0; i < Targc; i++)
    {
      if (is_option_indicator(Targv[i][0]) && Targv[i][1] == 'A')
      {
        register int j;
        for (j = 1; j < argc; j++)
        {
          if (is_option_indicator(argv[j][0]) && argv[j][1] == 'A')
          {
            j = -1;
            break;
          }
        }
        /* don't record a second -A switch */
        if (j == -1)
          continue;
      }

      if (is_option_indicator(Targv[i][0]) && Targv[i][1] != 'l')
      {
        register char *p = &Targv[i][1];
        register int c = *p;

        switches[n_switches].part1 = p;
        if ((SWITCH_TAKES_ARG (c) && p[1] == 0) || WORD_SWITCH_TAKES_ARG (p))
          switches[n_switches].part2 = Targv[++i];
        else
          switches[n_switches].part2 = 0;
        n_switches++;
      }
    }
  }

  /*
   * Make sure to keep all user files and specified libraries in their
   * original order, then let all files from Targv come after them.
   */
  for (i = 1; i < argc; i++)
  {
    if (is_option_indicator(argv[i][0]) && argv[i][1] != 'l')
    {
      register char *p = &argv[i][1];
      register int c = *p;

      if (strcmp(p, "save-temps") == 0)
        continue;

      switches[n_switches].part1 = p;
      if ((SWITCH_TAKES_ARG (c) && p[1] == 0) || WORD_SWITCH_TAKES_ARG (p))
        switches[n_switches].part2 = argv[++i];
      else
        switches[n_switches].part2 = 0;
      n_switches++;
    }
    else
#if defined(DOS)
      infiles[n_infiles++] = normalize_file_name(argv[i]);
#else
      infiles[n_infiles++] = argv[i];
#endif
  }

  if (Targc)
  {
    for (i = 0; i < Targc; i++)
    {
      if (is_option_indicator(Targv[i][0]) && Targv[i][1] != 'l')
      {
        register char *p = &Targv[i][1];
        register int c = *p;

        if ((SWITCH_TAKES_ARG (c) && p[1] == 0) || WORD_SWITCH_TAKES_ARG (p))
          i++;
      }
      else
#if defined(DOS)
        infiles[n_infiles++] = normalize_file_name(Targv[i]);
#else
        infiles[n_infiles++] = Targv[i];
#endif
    }
  }

  switches[n_switches].part1 = 0;
  infiles[n_infiles] = 0;
}

/* Process a spec string, accumulating and running commands.  */

/* These variables describe the input file name.
   input_file_number is the index on outfiles of this file,
   so that the output file name can be stored for later use by %o.
   input_basename is the start of the part of the input file
   sans all directory names, and basename_length is the number
   of characters starting there excluding the suffix .c or whatever.  */

char *input_filename;
int input_file_number;
int input_filename_length;
int basename_length;
char *input_basename;
#if 0
char *input_modname;
int modname_length;
#endif

/* These are variables used within do_spec and do_spec_1.  */

/* Nonzero if an arg has been started and not yet terminated
   (with space, tab or newline).  */
int arg_going;

/* Nonzero means %d or %g has been seen; the next arg to be terminated
   is a temporary file name.  */
int delete_this_arg;

/* Nonzero means %w has been seen; the next arg to be terminated
   is the output file name of this compilation.  */
int this_is_output_file;

/* Nonzero means %s has been seen; the next arg to be terminated
   is the name of a library file and we should try the standard
   search dirs for it.  */
int this_is_library_file;

/* Process the spec SPEC and run the commands specified therein.
   Returns 0 if the spec is successfully processed; -1 if failed.  */

int
do_spec (spec)
     char *spec;
{
  int value;

  clear_args ();
  arg_going = 0;
  delete_this_arg = 0;
  this_is_output_file = 0;
  this_is_library_file = 0;

  value = do_spec_1 (spec, 0);

  /* Force out any unfinished command.
     If -pipe, this forces out the last command if it ended in `|'.  */
  if (value == 0)
    {
      if (argbuf_index > 0 && !strcmp (argbuf[argbuf_index - 1], "|"))
	argbuf_index--;

      if (argbuf_index > 0)
	value = execute ();
    }

  return value;
}

/* Process the sub-spec SPEC as a portion of a larger spec.
   This is like processing a whole spec except that we do
   not initialize at the beginning and we do not supply a
   newline by default at the end.
   INSWITCH nonzero means don't process %-sequences in SPEC;
   in this case, % is treated as an ordinary character.
   This is used while substituting switches.
   INSWITCH nonzero also causes SPC not to terminate an argument.

   Value is zero unless a line was finished
   and the command on that line reported an error.  */

int
do_spec_1 (spec, inswitch)
     char *spec;
     int inswitch;
{
  register char *p = spec;
  register int c;
  char *string;

  while (c = *p++)
    /* If substituting a switch, treat all chars like letters.
       Otherwise, NL, SPC, TAB and % are special.  */
    switch (inswitch ? 'a' : c)
      {
      case '\n':
	/* End of line: finish any pending argument,
	   then run the pending command if one has been started.  */
	if (arg_going)
	  {
	    obstack_1grow (&obstack, 0);
	    string = obstack_finish (&obstack);
	    if (this_is_library_file)
	      string = find_lib_file (string);
            store_arg (string, delete_this_arg, this_is_output_file);
	    if (this_is_output_file)
	      outfiles[input_file_number] = string;
	  }
	arg_going = 0;

	if (argbuf_index > 0 && !strcmp (argbuf[argbuf_index - 1], "|"))
	  {
	    argbuf_index--;
	  }

	if (argbuf_index > 0)
	  {
	    int value = execute ();
	    if (value)
	      return value;
	  }
	/* Reinitialize for a new command, and for a new argument.  */
	clear_args ();
	arg_going = 0;
	delete_this_arg = 0;
	this_is_output_file = 0;
	this_is_library_file = 0;
	break;

      case '|':
	/* End any pending argument.  */
	if (arg_going)
	  {
	    obstack_1grow (&obstack, 0);
	    string = obstack_finish (&obstack);
	    if (this_is_library_file)
	      string = find_lib_file (string);
            store_arg (string, delete_this_arg, this_is_output_file);
	    if (this_is_output_file)
	      outfiles[input_file_number] = string;
	  }

	/* Use pipe */
	obstack_1grow (&obstack, c);
	arg_going = 1;
	break;

      case '\t':
      case ' ':
	/* Space or tab ends an argument if one is pending.  */
	if (arg_going)
	  {
	    obstack_1grow (&obstack, 0);
	    string = obstack_finish (&obstack);
	    if (this_is_library_file)
	      string = find_lib_file (string);
            store_arg (string, delete_this_arg, this_is_output_file);
	    if (this_is_output_file)
	      outfiles[input_file_number] = string;
	  }
	/* Reinitialize for a new argument.  */
	arg_going = 0;
	delete_this_arg = 0;
	this_is_output_file = 0;
	this_is_library_file = 0;
	break;

      case '%':
	switch (c = *p++)
	  {
	  case 0:
	    fatal ("Invalid specification!  Bug in cc.");

	  case 'b':
	    obstack_grow (&obstack, input_basename, basename_length);
	    arg_going = 1;
	    break;

	  case 'd':
	    delete_this_arg = 2;
	    break;

	  case 'e':
	    /* {...:%efoo} means report an error with `foo' as error message
	       and don't execute any more commands for this file.  */
	    {
	      char *q = p;
	      char buf[256];
	      while (*p != 0 && *p != '\n' && (p-q) < 255) p++;
	      strncpy (buf, q, p - q);
              buf[p-q] = '\0';
	      error ("%s", buf);
	      return -1;
	    }
	    break;

	  case 'f':
	    /* {...:%ffoo} means report a fatal error with `foo' as error
               message. */
	    {
	      char *q = p;
	      char buf[256];
	      while (*p != 0 && *p != '\n' && (p-q) < 255) p++;
	      strncpy (buf, q, p - q);
              buf[p-q] = '\0';
	      fatal ("%s", buf);
	      return -1;
	    }
	    break;

	  case 'g':
            {
              char *suffix = "";
              char *q;

	      if (*p != '\'')
                abort();

              p++;
              /* a suffix has been provided */
	      if ((q = strchr(p,'\'')) == 0)
		   abort();

              /* cut suffix to three */
              if ((q-p) > 3)
		suffix = save_string(p,3);
              else
		suffix = save_string(p,q-p);
              p = q + 1;  /* point after closing '\'' */

              if (save_temps_flag)
              {
                obstack_grow (&obstack, input_basename, basename_length);
                obstack_grow (&obstack, ".", 1);
                obstack_grow (&obstack, suffix, strlen(suffix));
              }
              else
              {
                char *tname;

                tname = get_temp_filename(suffix);

                obstack_grow (&obstack, tname, strlen(tname));
	        delete_this_arg = 1;
              }
	      arg_going = 1;
            }
	    break;

	  case 'i':
	    obstack_grow (&obstack, input_filename, input_filename_length);
	    arg_going = 1;
	    break;

	  case 'o':
	    {
	      register int f;
	      for (f = 0; f < n_infiles; f++)
		store_arg (outfiles[f], 0, 0);
	    }
	    break;

	  case 's':
	    this_is_library_file = 1;
	    break;

	  case 'w':
	    this_is_output_file = 1;
	    break;

          case 'W':
            {
              int index = argbuf_index;
              /* Handle the {...} following the %W.  */
              if (*p != '{')
                abort ();
              p = handle_braces (p + 1);
              if (p == 0)
                return -1;
              /* If any args were output, mark the last one for deletion
                 on failure.  */
              if (argbuf_index != index)
                record_temp_file (argbuf[argbuf_index - 1], 0, 1);
              break;
            }

	  case '{':
	    p = handle_braces (p);
	    if (p == 0)
	      return -1;
	    break;

	  case '[':
	    p = handle_brackets (p);
	    if (p == 0)
	      return -1;
	    break;

	  case '%':
	    obstack_1grow (&obstack, '%');
	    break;

/*** The rest just process a certain constant string as a spec.  */

	  case '1':
	    do_spec_1 (CC1_SPEC, 0);
	    break;

	  case '2':
	    do_spec_1 (CC1PLUS_SPEC, 0);
	    break;

	  case 'a':
	    do_spec_1 (ASM_SPEC, 0);
	    break;

	  case 'c':
	    do_spec_1 (SIGNED_CHAR_CPP_SPEC, 0);
	    break;

          case 'Z':
            do_spec_1 (SIGNED_CHAR_CC_SPEC, 0);
            break;

	  case 'C':
	    do_spec_1 (CPP_SPEC, 0);
	    break;

	  case 'l':
	    do_spec_1 (LINK_SPEC, 0);
	    break;

	  case 'L':
	    do_spec_1 (LIB_SPEC, 0);
	    break;

	  case 'p':
	    do_spec_1 (CPP_PREDEFINES, 0);
	    break;

	  case 'P':
	    {
	      char *x = (char *) xmalloc (strlen (CPP_PREDEFINES) * 2 + 40);
	      char *buf = x;
	      char *y = CPP_PREDEFINES;

	      /* Copy all of CPP_PREDEFINES into BUF,
		 but put __ after every -D and at the end of each arg,  */
	      while (1)
		{
		  if (! strncmp (y, "-D", 2))
		    {
		      *x++ = '-';
		      *x++ = 'D';
		      *x++ = '_';
		      *x++ = '_';
		      y += 2;
		    }
		  else if (*y == ' ' || *y == 0)
		    {
		      *x++ = '_';
		      *x++ = '_';
		      if (*y == 0)
			break;
		      else
			*x++ = *y++;
		    }
		  else
		    *x++ = *y++;
		}
	      *x = ' ';
              x += 1;
              
              {
                extern char gcc960_ver_mac[];
                sprintf (x, "-D__GCC960_VER=%s", gcc960_ver_mac);
              }
	      do_spec_1 (buf, 0);
	    }
	    break;

	  case 'S':
	    do_spec_1 (STARTFILE_SPEC, 0);
	    break;

	  case 'A':
#ifdef ARCH_SPEC
	    do_spec_1 (ARCH_SPEC, 0);
#endif
	    break;

	  default:
	    abort ();
	  }
	break;

      default:
	/* Ordinary character: put it into the current argument.  */
	obstack_1grow (&obstack, c);
	arg_going = 1;
      }

  return 0;		/* End of string */
}


char *
get_spec(file,spectype)
char *file;
char *spectype;
{
      register char *p, *s;
      char filebuf[256];
      char linebuf[1024];
      static char specbuf[10240];
      int cont_line;
      FILE *sf;

	
	if (*file == '\0') {
		fatal("no target specified for -T switch");
		return 0;
	}
	strcpy(filebuf,file);
	p = filebuf + strlen(filebuf) - strlen(GNU_SPEC_SUFFIX);

	if (strcmp(p,GNU_SPEC_SUFFIX) != 0)
		strcat(filebuf,GNU_SPEC_SUFFIX);

	if (!(s = find_lib_file(filebuf))) {
		fatal("cannot find target file %s", filebuf);
		return 0;
	}
	if ((sf = fopen(s,"r")) == NULL) {
		fatal("cannot open target file %s", s);
		return 0;
	}
	p = linebuf;
	specbuf[0] = '\0';
	cont_line = 0;
	while (fgets(p,1024,sf)) {
		if ((strncmp(p,spectype,strlen(spectype)) != 0) && !cont_line)
			continue;
		if (!cont_line)
			p += strlen(spectype);
		while (isspace(*p))
			p++;
		
		for (s = p; *s; s++) {
			if (*s == '#' || *s == '\0' || *s == '\n') {
				if (*s == '#' && s[-1] == '\\')
					continue;
				*s = '\0';
				break;
			}
		}
		if (s[-1] == '\\') {
			s[-1] = '\0';
			if (p != s-1)
				strcat(specbuf,p);
			cont_line++;
			p = linebuf;
			continue;
		}
		strcat(specbuf,p);
		break;
      }
      fclose(sf);
      return specbuf;
}

/* Return 0 if we call do_spec_1 and that returns -1.  */

char *
handle_braces (p)
     register char *p;
{
  register char *q;
  char *filter;
  int pipe = 0;
  int negate = 0;
  int tilde = 0;
  int spec = 0;

  if (*p == '|')
    /* A `|' after the open-brace means,
       if the test fails, output a single minus sign rather than nothing.
       This is used in %{|!pipe:...}.  */
    pipe = 1, ++p;

  if (*p == '!')
    /* A `!' after the open-brace negates the condition:
       succeed if the specified switch is not present.  */
    negate = 1, ++p;

  if (*p == '~')
    /* A '~' means to substitute the argument, but not the switch itself */
    tilde++, ++p;

  if (*p == '@')
    spec++, ++p;

  filter = p;
  while (*p != ':' && *p != '}') p++;
  if (*p != '}')
    {
      register int count = 1;
      q = p + 1;
      while (count > 0)
	{
	  if (*q == '{')
	    count++;
	  else if (*q == '}')
	    count--;
	  else if (*q == 0)
	    abort ();
	  q++;
	}
    }
  else
    q = p + 1;

  /* handle special rules for inclusion of spec files */
  if (spec)
  {
    register int i;
    char *file, *s;
    char *t_ptr;
    char spectype[10];
    int specfound = 0;
    
    file = filter;
    while (*file != '\'' && *file != '\0')
      file++;
    if (!*file) {
      fatal("internal error: no line specifier for spec inclusion");
      return 0;
    }
    s = ++file;
    t_ptr = s+1;
    while (*t_ptr != '\'' && *t_ptr != '\0')
      t_ptr++;
    if (!*t_ptr) {
      fatal("internal error: unterminated line specifier in spec inclusion");
      return 0;
    }
    strncpy(spectype,file,t_ptr-file);
    spectype[t_ptr-file] = '\0';
    strcat(spectype,":");

    for (i = 0; i < n_switches; i++)
    {
      char *this_spec;
      if (strncmp (switches[i].part1, filter, s - filter - 1) != 0)
	continue;

      if (WORD_SWITCH_TAKES_ARG(switches[i].part1))
	continue;

      file = switches[i].part1 + (s - filter - 1);
      this_spec = get_spec(file,spectype);

      if (this_spec[0] != '\0')
      {
        specfound = 1;
        if ((do_spec_1(this_spec, this_spec[0] == '%' ? 0 : 1) != 0))
        {
	  fatal("error in target spec line '%s'", spectype);
	  return 0;
        }
      }
    }

    /*
     * if no spec was found and there is a colon following then
     * do the spec following the colon.  If a spec was found they
     * ignore the spec following the colon.
     */
    if (specfound || *p == '}')
      do_spec_1(" ",0);	/* end any pending switches */
    else
    {
      if (do_spec_1 (save_string (p + 1, q - p - 2), 0) < 0)
        return 0;
    }
    return q;
  }

  if (p[-1] == '*' && p[0] == '}')
    {
      /* Substitute all matching switches as separate args.  */
      register int i;
      --p;
      for (i = 0; i < n_switches; i++)
	if (!strncmp (switches[i].part1, filter, p - filter))
	  {
	    give_switch (i, tilde);
	  }
    }
  else
    {
      /* Test for presence of the specified switch.  */
      register int i;
      int present = 0;

      /* If name specified ends in *, as in {x*:...},
	 check for presence of any switch name starting with x.  */
      if (p[-1] == '*')
	{
	  for (i = 0; i < n_switches; i++)
	    {
	      if (!strncmp (switches[i].part1, filter, p - filter - 1))
		{
		  present = 1;
		  break;
		}
	    }
	}
      /* Otherwise, check for presence of exact name specified.  */
      else
	{
	  for (i = 0; i < n_switches; i++)
	    {
	      if (!strncmp (switches[i].part1, filter, p - filter)
		  && switches[i].part1[p - filter] == 0)
		{
		  present = 1;
		  break;
		}
	    }
	}

      /* If it is as desired (present for %{s...}, absent for %{-s...})
	 then substitute either the switch or the specified
	 conditional text.  */
      if (present != negate)
	{
	  if (*p == '}')
	    {
	      give_switch (i,tilde);
	    }
	  else
	    {
	      if (do_spec_1 (save_string (p + 1, q - p - 2), 0) < 0)
		return 0;
	    }
	}
      else if (pipe)
	{
	  /* Here if a %{|...} conditional fails: output a minus sign,
	     which means "standard output" or "standard input".  */
	  do_spec_1 ("-");
	}
    }

  return q;
}



/* we enter pointing to the character after the '[' */
char *
handle_brackets (p)
	register char *p;
{
	register char *q,*e,*end;
	register char *spec,*alt;
	static int toplevel = 0;
	int brac_depth;
	int curl_depth;

	/* find matching close bracket */
	brac_depth = 0;
	curl_depth = 0;
	end = 0;
	alt = 0;
	for(q = p; *q != '\0'; q++) {
		if (*q == '[') {
			brac_depth++;
		}
		if (*q == ']') {
			if (brac_depth == 0) {
				end = q;
				break;
			}
			brac_depth--;
		}
		if (*q == '{' && q[-1] == '%')
			curl_depth++;
		if (*q == '}' && curl_depth)
			curl_depth--;
		if (*q == ':' && brac_depth == 0 && curl_depth == 0) {
			alt = q;
		}
	}
	if ((end == 0) || (brac_depth != 0)) {
		error("malformed specification string '%s' in driver", p);
		return 0;
	}

	spec = 0;

	switch (*p++) {
	case '\'':
		/* single-quote after '[' indicates a literal environment */
		/* variable name  to look up */
		if ((q = strchr(p,'\'')) == 0) {
			abort();
		}
		e = save_string(p,q-p-1);
		p = q + 1;	/* point after closing '\'' */
		if (q = getenv(e)) {
			spec = save_string(q,strlen(q));
		}
		break;

	case '~':	/* base of directories */
		if (q = genv(ENV_GNUBASE)) {
			spec = save_string(q,strlen(q));
		}
		break;
		
	case 'B':	/* bin directory */
		if (q = genv(ENV_GNUBIN)) {
			spec = save_string(q,strlen(q));
		}
		break;
	case 'L':	/* lib directory */
		if (q = genv(ENV_GNULIB)) {
			spec = save_string(q,strlen(q));
		}
		break;
	case 'I':	/* include directory */
		if (q = genv(ENV_GNUINC)) {
			spec = save_string(q,strlen(q));
		}
		break;
        case 'D':	/* database directory */
		if (q = genv(ENV_GNUPDB)) {
			spec = save_string(q,strlen(q));
		}
		break;
	
	case 'a':	/* assembler */
		if (q = genv(ENV_GNUAS)) {
			spec = save_string(q,strlen(q));
		}
		break;
	case 'S':	/* crt0 */
		if (q = genv(ENV_GNUCRT)) {
			spec = save_string(q,strlen(q));
		}
		break;
	case 'M':	/* mcrt0 */
		if (q = genv(ENV_GNUCRT_M)) {
			spec = save_string(q,strlen(q));
		}
		break;
	case 'G':	/* gcrt0 */
		if (q = genv(ENV_GNUCRT_G)) {
			spec = save_string(q,strlen(q));
		}
		break;
	case 'b':	/* gcrt0 */
		if (q = genv(ENV_GNUCRT_B)) {
			spec = save_string(q,strlen(q));
		}
		break;
	case 'C':	/* cpp */
		if (q = genv(ENV_GNUCPP)) {
			spec = save_string(q,strlen(q));
		}
		break;
	case '1':	/* cc1 */
		if (q = genv(ENV_GNUCC1)) {
			spec = save_string(q,strlen(q));
		}
		break;
	case '2':	/* cc1plus */
		if (q = genv(ENV_GNUCC1PLUS)) {
			spec = save_string(q,strlen(q));
		}
		break;
	case 'l':	/* ld */
		if (q = genv(ENV_GNULD)) {
			spec = save_string(q,strlen(q));
		}
		break;
	case '@':
		switch (*p++) {
		case 'a':
			spec = GNUAS_DFLT;
			break;
		case 'l':
			spec = GNULD_DFLT;
			break;
		case '1':
			spec = GNUCC1_DFLT;
			break;
		case '2':
			spec = GNUCC1PLUS_DFLT;
			break;
		case 'c':
			spec = GNUCPP_DFLT;
			break;
		case 'S':
			spec = GNUCRT_DFLT;
			break;
		case 'M':
			spec = GNUCRTM_DFLT;
			break;
		case 'G':
			spec = GNUCRTG_DFLT;
			break;
		case 'b':
			spec = GNUCRTB_DFLT;
			break;
		case 'I':
			spec = GNUINC_DFLT;
			break;
		}
		break;

	case '?':
		spec = (char *) xmalloc(strlen(STANDARD_EXEC_PREFIX) + 100);

		switch (*p++) {
		case 'a':
			strcpy(spec,STANDARD_EXEC_PREFIX);
			strcat(spec,GNUAS_DFLT);
			break;
		case 'l':
			strcpy(spec,STANDARD_EXEC_PREFIX);
			strcat(spec,GNULD_DFLT);
			break;
		case '1':
			strcpy(spec,STANDARD_LIB_PREFIX);
			strcat(spec,GNUCC1_DFLT);
			break;
		case '2':
			strcpy(spec,STANDARD_LIB_PREFIX);
			strcat(spec,GNUCC1PLUS_DFLT);
			break;
		case 'c':
			strcpy(spec,STANDARD_LIB_PREFIX);
			strcat(spec,GNUCPP_DFLT);
			break;
		case 'S':
			strcpy(spec,STANDARD_LIB_PREFIX);
			strcat(spec,GNUCRT_DFLT);
			break;
		case 'M':
			strcpy(spec,STANDARD_LIB_PREFIX);
			strcat(spec,GNUCRTM_DFLT);
			break;
		case 'G':
			strcpy(spec,STANDARD_LIB_PREFIX);
			strcat(spec,GNUCRTG_DFLT);
			break;
		case 'b':
			strcpy(spec,STANDARD_LIB_PREFIX);
			strcat(spec,GNUCRTB_DFLT);
			break;
		case 'I':
			strcpy(spec,STANDARD_INC_PREFIX);
			break;
		default:
			spec = 0;
		}
		break;

	default:
		return 0;
	}

	if (spec) {
		/* append remainder of string to spec */
		if (*p != ']' && *p != ':' && *p != '$') {
			if (alt && *alt == ':') {
				e = alt;
			} else {
				e = end;
			}
			q = (char *) xmalloc(strlen(spec)+(e-p)+1);
			strcpy(q,spec);
			strncat(q,p,e-p);
			spec = q;
		}
                if (*p == '$' && alt && *alt == ':')
		{
			alt++;
			spec = save_string(alt,end-alt);
		}
	}

	if (spec == 0) {
		/* getenv failed - see if there's another clause */
		if (alt && *alt == ':') {
			if (*p != '$') {
				alt++;
				spec = save_string(alt,end-alt);
			}
			else 
				spec = "";
		} else if (toplevel == 0) {
			fatal("program/library lookup failed - set $%s\n",ENV_GNUBASE);
			return 0;
		}
	}
	toplevel++;
	if (do_spec_1(spec,0) < 0) {
		toplevel--;
		return 0;
	}
	toplevel--;
	return(end+1);
}

/* Pass a switch to the current accumulating command
   in the same form that we received it.
   SWITCHNUM identifies the switch; it is an index into
   the vector of switches gcc received, which is `switches'.
   This cannot fail since it never finishes a command line.  */

give_switch (switchnum, omitswitch)
     int switchnum;
     int omitswitch;
{
  if (omitswitch == 0) {
     do_spec_1 ("-", 0);
     do_spec_1 (switches[switchnum].part1, 1);
     do_spec_1 (" ", 0);
  }
  if (switches[switchnum].part2 != 0)
    {
      do_spec_1 (switches[switchnum].part2, 1);
      do_spec_1 (" ", 0);
    }
}

/* Search for a file named NAME trying various prefixes including the
   user's -B prefix and some standard ones.
   Return the absolute pathname found.  If nothing is found, return NAME.  */

static char *
find_lib_file(name)
char *name;
{
	char *q;
	static char buf[1000];

	if (access(name,R_OK) == 0) {
		strcpy(buf,name);
		return buf;
	}
	if (q = genv(ENV_GNULIB)) {
		sprintf(buf,"%s/%s",q,name);
		if (access(buf,R_OK) == 0) {
			return buf;
		}
	}
	if (q = genv(ENV_GNUBASE)) {
		sprintf(buf,"%s/lib/%s",q,name);
		if (access(buf,R_OK) == 0) {
			return buf;
		}
	}
	sprintf(buf,"%s/%s",STANDARD_LIB_PREFIX,name);
	if (access(buf,R_OK) == 0) {
		return buf;
	}
	return name;
}

/* Name with which this program was invoked.  */

char *programname;

/* On fatal signals, delete all the temporary files.  */

void
fatal_error (signum)
     int signum;
{
  signal (signum, SIG_DFL);
  delete_failure_queue ();
  delete_temp_files ();
  /* Get the same signal again, this time not handled,
     so its normal effect occurs.  */
  kill (getpid (), signum);
}
#if defined(IMSTG)

static char* word_switches_with_arguments[] = MLOS_WITH_ARG;

int
i960_word_switch_takes_arg(option)
char* option;
{
	char** tmp = &word_switches_with_arguments[0];
	for (; *tmp != NULL; tmp++)
		if (strcmp(*tmp, option) == 0)
			return 1;
	return 0;
}

extern char gnu960_ver[];
#include "ver960.h"	/* Support check_v960 */

#endif

int
main (argc, argv)
     int argc;
     char **argv;
{
  register int i;
  int value;
  int nolink = 0;
  int error_count = 0;

#if defined(IMSTG)
  argc = get_response_file(argc,&argv);
#endif

  programname = argv[0];

/* special hack for 960 version */
  check_v960( argc, argv );
/* end hack */

  if (signal (SIGINT, SIG_IGN) != SIG_IGN)
    signal (SIGINT, fatal_error);
#ifdef SIGHUP
  if (signal (SIGHUP, SIG_IGN) != SIG_IGN)
    signal (SIGHUP, fatal_error);
#endif 
  if (signal (SIGTERM, SIG_IGN) != SIG_IGN)
    signal (SIGTERM, fatal_error);

  argbuf_length = 10;
  argbuf = (char **) xmalloc (argbuf_length * sizeof (char *));

  obstack_init (&obstack);

  /* Make a table of what switches there are (switches, n_switches).
     Make a table of specified input files (infiles, n_infiles).  */

  process_command (argc, argv);

  if (vflag || Vflag)
    {
      fprintf (stderr, "%s\n", gnu960_ver);
      if (n_infiles == 0)
	exit (0);
    }

  if (help_flag)
  {
    print_help();
    if (n_infiles == 0)
      exit(0);
  }

  if (n_infiles == 0)
    fatal ("No input files specified.");

  /* Make a place to record the compiler output file names
     that correspond to the input files.  */

  outfiles = (char **) xmalloc (n_infiles * sizeof (char *));
  bzero (outfiles, n_infiles * sizeof (char *));

  for (i = 0; i < n_infiles; i++)
    {
      register struct compiler *cp;
      int this_file_error = 0;

      /* Tell do_spec what to substitute for %i.  */

      input_filename = infiles[i];
      input_filename_length = strlen (input_filename);
      input_file_number = i;

      /* Use the same thing in %o, unless cp->spec says otherwise.  */

      outfiles[i] = input_filename;

      /* Figure out which compiler from the file's suffix.  */

      for (cp = compilers; cp->spec[0] != 0; cp++)
	{
	  if (strlen (cp->suffix) < input_filename_length
	      && !strcmp (cp->suffix,
			  infiles[i] + input_filename_length
			  - strlen (cp->suffix)))
	    {
	      /* Ok, we found an applicable compiler.  Run its spec.  */
	      /* First say how much of input_filename to substitute for %b  */
	      register char *p;
              int len;
              int j;

	      input_basename = input_filename;
#if defined(DOS)
	      if (*input_basename && input_basename[1] == ':')
		input_basename +=2;
#endif
	      for (p = input_basename; *p; p++)
		if (*p == '/'
#if defined(DOS)
			|| *p == '\\'
#endif
		   )
		  input_basename = p + 1;
	      basename_length = (input_filename_length - strlen (cp->suffix)
				 - (input_basename - input_filename));

              /* concatenate the spec */
              len = 0;
              for (j = 0; j < sizeof(cp->spec)/sizeof(cp->spec[0]); j++)
                if (cp->spec[j])
                  len += strlen (cp->spec[j]);

              p = (char *) xmalloc (len + 1);

              len = 0;
              for (j = 0; j < sizeof(cp->spec)/ sizeof(cp->spec[0]); j++)
                if (cp->spec[j])
                  {
                    strcpy (p + len, cp->spec[j]);
                    len += strlen (cp->spec[j]);
                  }

              /* interpret the spec */
              value = do_spec (p);
              free (p);

	      if (value < 0)
                this_file_error = 1;
	      break;
	    }
	}

      /* If this file's name does not contain a recognized suffix,
	 don't do anything to it, but do feed it to the link spec
	 since its name is in outfiles.  */

      if (! cp->spec && nolink)
	{
	  /* But if this happens, and we aren't going to run the linker,
	     warn the user.  */
	  error ("%s: linker input file unused since linking not done",
		 input_filename);
	}

      /* Clear the delete-on-failure queue, deleting the files in it
         if this compilation failed.  */

      if (this_file_error)
        {
          delete_failure_queue ();
          error_count++;
        }
      /* If this compilation succeeded, don't delete those files later.  */
      clear_failure_queue ();
    }

  /* Run ld to link all the compiler output files.  */

  if (! nolink && error_count == 0)
    {
      value = do_spec (link_spec);
      if (value < 0)
	error_count = 1;
    }

  /* Delete some or all of the temporary files we made.  */

  if (error_count)
    delete_failure_queue ();
  delete_temp_files ();

  exit (error_count);
}

int
xmalloc (size)
     int size;
{
  register int value = (int) malloc (size);
  if (value == 0)
    fatal ("Virtual memory full.");
  return value;
}

int
xrealloc (ptr, size)
#ifdef DOS
     void *ptr;
     size_t size;
#else
     int ptr, size;
#endif
{
  register int value = (int) realloc (ptr, size);
  if (value == 0)
    fatal ("Virtual memory full.");
  return value;
}

fatal (msg, arg1, arg2)
     char *msg, *arg1, *arg2;
{
  error (msg, arg1, arg2);
  delete_temp_files (0);
  exit (1);
}

error (msg, arg1, arg2)
     char *msg, *arg1, *arg2;
{
  fprintf (stderr, "%s: ", programname);
  fprintf (stderr, msg, arg1, arg2);
  fprintf (stderr, "\n");
}

/* Return a newly-allocated string whose contents concatenate those of s1, s2, s3.  */

char *
concat (s1, s2, s3)
     char *s1, *s2, *s3;
{
  int len1 = strlen (s1), len2 = strlen (s2), len3 = strlen (s3);
  char *result = (char *) xmalloc (len1 + len2 + len3 + 1);

  strcpy (result, s1);
  strcpy (result + len1, s2);
  strcpy (result + len1 + len2, s3);
  *(result + len1 + len2 + len3) = 0;

  return result;
}

char *
save_string (s, len)
     char *s;
     int len;
{
  register char *result = (char *) xmalloc (len + 1);

  bcopy (s, result, len);
  result[len] = 0;
  return result;
}

pfatal_with_name (name)
     char *name;
{
#ifndef DOS
  extern int errno;
#endif
  extern int sys_nerr;
#ifdef I386_NBSD1
  extern const char *const sys_errlist[];
#else
  extern char *sys_errlist[];
#endif
  char *s;

  if (errno < sys_nerr)
    s = concat ("", sys_errlist[errno], " for %s");
  else
    s = "cannot open %s";
  fatal (s, name);
}

perror_with_name (name)
     char *name;
{
#ifndef DOS
  extern int errno;
#endif
  extern int sys_nerr;
#ifdef I386_NBSD1
  extern const char *const sys_errlist[];
#else
  extern char *sys_errlist[];
#endif
  char *s;

  if (errno < sys_nerr)
    s = concat ("", sys_errlist[errno], " for %s");
  else
    s = "cannot open %s";
  error (s, name);
}

/* More 'friendly' abort that prints the line and file.
   config.h can #define abort fancy_abort if you like that sort of thing.  */

void
fancy_abort ()
{
  fatal ("Internal gcc abort.");
}

static char *help_text[] =
{
"",
"gcc960 C cross compiler for the i960 microprocessor",
"",
"Usage: gcc960 [options] files",
"",
"Options:",
"",
"  -Aarch:            specifies an 80960 architecture for the output file",
"                     [valid arch values are: KA,SA,KB,SB,CA,CF,JA,JD,JF,",
"                     HA,HD,HT,RP]",
"  -ansi:             turns off features of gcc960 that are incompatible",
"                     with strict ANSI C",
"  -bname str:        specifies str as the module name",
"  -C:                tells preprocessor to keep comments in its output",
"  -c:                stops compilation after creating object files",
"  -clist str:        specifies listing option string",
"  -crt:              causes gcc960 not to use standard startup file",
"                     for linking",
"  -D name:           defines preprocessor macro name as 1",
"  -D name=defn:      defines preprocessor macro name as defn",
"  -dstr:             dumps various compiler debugging information",
"  -E:                stops after preprocessing input files",
"  -e name:           specifies symbol name as entry point for load module",
"  -Fbout:            specifies B.OUT object format output",
"  -Fcoff:            specifies COFF object format output",
"  -Felf:             specifies Elf/DWARF object format output",
"  -fsigned-char:     makes default char type be signed",
"  -funsigned-char:   makes default char type be unsigned char",
"  -fvolatile:        makes all memory references through pointers volatile",
"  -fvolatile-global: makes all global variables references volatile",
"  -fwritable-strings:makes string go into the .data section",
"  -fstr:             specifies various specific optimization options",
"  -G:                specifies that target is big-endian",
"  -g[0-3]:           generates source level debug information",
"                     -g1 is terse, -g and -g2 are normal, -g3 is verbose",
"                     -g0 disables generation of debug information",
"  -h:                prints this help information",
"  -help:             prints this help information",
"  -Idir:             specifies include directory for #include directives",
"  -include file:     specifies file to preinclude prior to all source files",
"  -imacros file:     specifies file to preinclude for macro definitions",
"  -L dir:            specifies library search directory for linking",
"  -llibrary:         specifies library liblibrary.a for linking",
"  -M:                generates makefile dependencies, stops compilation",
"                     after preprocessing",
"  -MM:               same as -M except only creates dependencies for",
"                     #include \"file\" format includes",
"  -MD:               same as -M, except output goes to file.d, and the",
"                     compilation doesn't stop after preprocessing",
"  -MMD:              same as -MD, except only creates dependencies for",
"                     #include \"file\" format includes",
"  -m[no-]leaf-procedures:",
"                     turns [off]on leaf procedure optimization",
"  -m[no-]tail-call:  turns [off]on tail call optimization",
"  -mlong-calls:      makes all calls in a module use \"callx\" instruction",
"  -msoft-float:      generates calls to software floating point libraries",
"                     automatically used for KA,SA,CA,CF,JA,JD,JF,",
"                     HA,HD,HT and RP processors",
"  -mpic:             generates position independent references to objects",
"                     in the .text section",
"  -mpid:             generates position independent references to objects",
"                     in the .data, .bss, and common sections",
"  -mcave:            generates all functions as CAVE secondary",
"  -mwait=[0-32]:     sets the expected memory wait-state for data",
"                     accesses.",
"  -mic-compat:       generates code using ic960 R2.0 rules for size and",
"                     type alignment, and integer promotion rules",
"  -mic2.0-compat:    same as -mic-compat",
"  -mic3.0-compat:    emulates ic960 3.0 enumeration typing, and",
"                     makes default char type be signed char",
"  -mstr:             specifies various other machine specific options",
"  -nostdlib:         specifies not to use the standard libraries",
"  -nostdinc:         specifies not to use standard include files",
"  -O[0-4]:           specifies optimization level",
"  -O4_1:             specifies first pass of profile-driven compilation",
"  -O4_2:             specifies second pass of profile-driven compilation",
"  -o file:           directs output to file",
"  -pedantic:         issues all warnings demanded by strict ANSI standard C",
"  -pedantic-errors:  like -pedantic, but issues errors instead of warnings",
"  -r:                causes linker to generate relocatable output file",
"  -S:                stops after producing assembly source for .c files",
"  -s:                strips the symbol information from the load module",
"                     produced during the link stage",
"  -save-temps:       saves the intermediate files instead of deleting them",
"  -Ttext addr:       causes the linker to start the .text section at addr",
"  -Tdata addr:       causes the linker to start the .data section at addr",
"  -Tbss addr:        causes the linker to start the .bss section at addr",
"  -Tstr:             causes gcc960 to use str.gld file for configuring",
"                     itself for a particular target board",
"  -traditional:      supports some aspects of \"traditional\" C compilers",
"  -U name:           undefines preprocessor macro name",
"  -u name:           creates an unresolved symbol name in the",
"                     linker symbol table at the start of the linking process",
"  -V:                displays tool version numbers",
"  -v:                displays tool version numbers and subprocess commands",
"  -v960:             displays gcc960 version number and exits",
"  -Wstr:             enables specific warnings based on str",
"  -w:                inhibits all warning messages",
"  -X:                strips certain symbols from linker output",
"  -x:                strips certain symbols from linker output",
"  -Z dir:            specifies program database directory for",
"                     profile-driven optimizing compilation",
"  -z:                causes assembler and linker to use time stamp 0",
"                     in all object and load files produced",
"",
"See the user's manual for more complete information",
0,
};

static void
print_help()
{
  paginator (help_text);
}

#endif
