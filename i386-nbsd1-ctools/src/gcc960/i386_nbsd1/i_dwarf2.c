/*
   This file implements generation of DWARF version 2 debug information.
   DWARF version 1 is implemented in dwarfout.c, and since both versions have
   almost identical entry points, only one version can be selected in
   any given build.

   The external entry points are:

	dwarfout_start_new_source_file
	dwarfout_resume_previous_source_file
	dwarfout_define
	dwarfout_undef
		These are called by the front end when a #define or #undef
		directive is seen, or when a #line directive is seen that
		changes the current input file.  They generate entries in
		the .debug_macinfo section, and may also cause the
		generation of DW_LNE_define_file instructions in
		the .debug_line section.
		Because the front end peeks into the input file for a #line
		directive that defines the name of the primary source file,
		these functions might get called before dwarfout_init().

	dwarfout_init
	dwarfout_finish
		These functions are called to initialize and wrap up
		generation of DWARF debug information.  The remaining
		entry points are only called after dwarfout_init has
		been called, and before dwarfout_finish.

	dwarfout_file_scope_decl
		Called by the front end to emit a debug information entry (DIE)
		for the given file-scope _DECL node.  For function _DECLs, this
		function also emits DIEs for objects owned by the function,
		such as its parameters, variables and nested scopes.

	dwarfout_begin_function
	dwarfout_begin_block
	dwarfout_end_block
	dwarfout_label
	dwarfout_le_boundary
		These are called by final_scan_insn when a note of the
		appropriate form is seen.  They cause a label to be defined
		in the .text section that marks the location of an object.
		These labels are referred to in various DIE attribute values.

	dwarfout_prep_for_location_tracking()
		This is called just before register allocation to mark each
		insn that defines a parameter or local variable.  Such insns
		are used later, in dwarfout_find_le_boundaries(), to determine
		the extended live range of these objects.

	dwarfout_find_le_boundaries()
		This is called by final_start_function() to insert
		NOTE_INSN_DWARF_LE_BOUNDARY notes into the insn stream.
		These notes mark the boundary of the valid ranges for
		one or more location expressions.  When final_scan_insn()
		encounters these notes, it calls dwarfout_le_boundary
		to emit a label.

	dwarfout_end_function
	dwarfout_end_epilogue
		These are called by final_end_function.

	dwarfout_line
		This is called by output_source_line, which is called by
		final_start_function and by final_scan_insn when it
		encounters a line number note.  This function generates
		instructions in the .debug_line section that describe
		the machine-address to source-file-position mapping.

	dwarfout_simplify_notes
		This is called by final_start_function() to eliminate
		redundant line number notes in the insn stream.
		It also initializes each note's REAL_SRC_COL field.

	dwarfout_effective_note
		Return the line number note corresponding to some insn.

	dwarfout_last_insn_p
		Determine if an insn is the only remaining active insn
		corresponding to its line number note.

	dwarfout_disable_notes
		Disable the line number note corresponding to an insn.
		This should be called by optimizations that delete the
		last active insn for a note, so the note is not mistakenly
		associated with the RTL for some other statement.

	dwarfout_cie
		Record a .debug_frame CIE that will be referenced by
		some FDE.  All the CIEs are emitted in dwarfout_finish().

 */

#include "config.h"

#if defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "tree.h"
#include "c-tree.h"
#include "flags.h"
#include "rtl.h"
#include "hard-reg-set.h"
#include "insn-config.h"
#include "reload.h"
#include "regs.h"
#include "output.h"
#include "defaults.h"
#include "expr.h"
#include "basic-block.h"
#include "i_dataflow.h"
#include "i_dwarf2.h"
#include "i_toolib.h"

/* Extract the column number from a line number note. */

#if !defined(NOTE_COLUMN_NUMBER)
#define NOTE_COLUMN_NUMBER(n)		(0)
#endif

/* Extract the column number from a _DECL node */
/* DECL_SOURCE_POS is an Intel 80960 compiler extension. */

#if defined(DECL_SOURCE_POS)
#define DECL_SOURCE_COLUMN(decl)	imstg_map_sinfo_col_to_real_col(DECL_SOURCE_POS(decl))
#else
#define DECL_SOURCE_COLUMN(decl)	0
#endif

extern char	*getpwd();

extern int	flag_traditional;
extern char	*language_string;
extern FILE	*dwarf_dump_file;

extern void debug_info_section();
extern void debug_abbrev_section();
extern void debug_pubnames_section();
extern void debug_aranges_section();
extern void debug_loc_section();
extern void debug_line_section();
extern void debug_frame_section();
extern void debug_macinfo_section();
extern void debug_str_section();

static void determine_type_die_label();
static void dwarf_avail_locations_analysis();
static void dwarf_debugging_dump();
static void dwarf_output_const_value_attribute();
static void dwarf_output_decl();
static void dwarf_output_decls_for_scope();
static void dwarf_output_location_attribute();
static void dwarf_output_source_coordinates();
static void dwarf_output_type();

#if defined(__lint) || defined(__LINT__)
/* Prevent lint from warning about constant in do { ... } while(0); context */
#define ALWAYS_ZERO   always_zero_for_lint()
static int always_zero_for_lint() { return 0; }
#else
#define ALWAYS_ZERO   0
#endif

#if defined(IMSTG_PRAGMA_SECTION_SEEN)
#define DWARF_PRAGMA_SECTION_SEEN	IMSTG_PRAGMA_SECTION_SEEN
#else
#define DWARF_PRAGMA_SECTION_SEEN	ALWAYS_ZERO
#endif

/* dwarfout_define() and dwarfout_undef() must parse macros and macro bodies.
   Define some <ctype.h>-like lookup tables to help speed parsing.  These
   tables are defined the same as they are in the preprocessor.
 */
static unsigned char is_idstart[256];
static unsigned char is_idchar[256];
static unsigned char is_hor_space[256];

#if !defined(DWARF_SKIP_WHITE_SPACE)
#define DWARF_SKIP_WHITE_SPACE(p)	\
	do { while (is_hor_space[*(p)]) (p)++; } while (ALWAYS_ZERO)
#endif

/* Non-zero if we are compiling for big-endian target memory.
   Initialized in dwarf_preinitialize().
 */
static int	dwarf_target_big_endian = 0;

/* Non-zero if we are performing our file-scope finalization pass,
   indicating we should force out DWARF descriptions of any and all
   file-scope tagged types which are still incomplete types.
 */
static int	finalizing = 0;

/* We must assign lexical-blocks id numbers in the order in which their
   beginnings are encountered.  We output attributes (low_pc, high_pc) that
   refer to the beginnings and ends of the ranges of code for each lexical
   block with assembler labels L_DBn_b and L_DBn_e, where n is the block number.
   The labels themselves are defined when final_scan_insn calls
   dwarfout_begin_block and dwarfout_end_block.  We must be sure to generate
   block numbers in the same order that final_scan_insn does.
 */
static unsigned int	next_block_number = 2;

/* The number of the current function definition that we are generating
   debugging information for.  These numbers range from 1 up to the maximum
   number of function definitions contained within the current compilation
   unit.  These numbers are used to create unique labels for various things
   contained within various function definitions.
 */
static unsigned int	current_funcdef_number = 1;

/* Maintain a counter to generate unique labels for the bounds of the
   valid ranges for location expressions.  Also record the number used
   in the very first and last such labels generated for the current function.
 */

static unsigned int	next_le_boundary_number = 1;
static unsigned int	func_first_le_boundary_number;
       unsigned int	func_last_le_boundary_number;

/* When emitting addresses in .debug_line (in function dwarfout_line()), we
   generally emit 16-bit address "increments" from the previous address.
   The increment is represented by taking the difference between two
   assembly labels.  This has two draw backs:  16-bits artificially limits
   the amount of stuff that can go into the .text section between two
   function source lines; and the difference expression doesn't work when
   the labels come from different sections, as happens when #pragma section
   is used.  To work around these limitations, we use an absolute address
   when emitting the first source position information (LINE_NUMBER_NOTE)
   for each function, and use increments within the function.
   dwarf_next_line_note_is_first_in_function tells dwarfout_line() whether
   the line number note being emitted is the first in a function.  It
   is reset to TRUE in dwarfout_end_epilogue().
 */
static int	dwarf_next_line_note_is_first_in_function = 1;

/* For the .debug_aranges section, rather than emitting one range per object,
   we emit one range per section (text, data and bss).  Furthermore, we
   suppress the range if no objects are defined in the section.
 */
static int	dwarf_emit_text_arange = 0;
static int	dwarf_emit_data_arange = 0;
static int	dwarf_emit_bss_arange = 0;

#define DWARF_FIRST_OR_LAST_BOUNDARY_NOTE	((rtx)(-1))
	/* Used in various contexts involving location expression boundaries.
	   If representing a lower bound, it means the very first
	   NOTE_INSN_DWARF_LE_BOUNDARY note generated for the current
	   function, which uses func_first_le_boundary_number.
	   (Since the RTL for this note is never physically created,
	   we need to handle it specially.)
	   If representing an upper bound, it means the very last
	   NOTE_INSN_DWARF_LE_BOUNDARY note generated for the current
	   function, which uses func_last_le_boundary_number.

	   NULL_RTX, on the other hand, if used in the same context,
	   means something different.  For a given location expression,
	   a NULL_RTX lower bound means the beginning of the owning decl's
	   scope, and a NULL_RTX upper bound means the end of the owning
	   decl's scope.  Though be aware that some ranges which indicate
	   a decl's entire scope, do explicitly use the insn's that bound
	   the scope in these contexts.
	 */

/* sibling_counter is used to generate unique labels for the DW_AT_sibling
   attribute.
 */
static unsigned int	sibling_counter = 1;

/* Define target-specific information. */

/* For each physical register r, dwarf_reg_info defines the
   DWARF location expression opcodes and operands to be used
   to represent r as a register or base register.
   DWARF_REGISTER_INFO must be defined, and must expand to an appropriate
   static initialization for this array.
 */

#if !defined(DWARF_REGISTER_INFO)
-- error -- you need to define DWARF_REGISTER_INFO before proceeding.
#endif

Dwarf_reg_info dwarf_reg_info[FIRST_PSEUDO_REGISTER] = DWARF_REGISTER_INFO;

#if !defined(DWARF_PRODUCER)
#define DWARF_PRODUCER	dwarf_producer
static char dwarf_producer[] = "Free Software Foundation GNU Compiler";
#endif

#if !defined(DWARF_CIE_AUGMENTER)
#define DWARF_CIE_AUGMENTER	DWARF_PRODUCER
#endif

#if !defined(TARGET_POINTER_BYTE_SIZE)
#define TARGET_POINTER_BYTE_SIZE	(POINTER_SIZE/BITS_PER_UNIT)
#endif

#if !defined(TARGET_SEGMENT_DESCR_BYTE_SIZE)
#define TARGET_SEGMENT_DESCR_BYTE_SIZE	(0)
#endif

#if !defined(UNALIGNED_INT_ASM_OP)
#define UNALIGNED_INT_ASM_OP	unaligned_int_asm_op
static char unaligned_int_asm_op[] = ".word";
#endif

#if !defined(UNALIGNED_SHORT_ASM_OP)
#define UNALIGNED_SHORT_ASM_OP	unaligned_short_asm_op
static char unaligned_short_asm_op[] = ".short";
#endif

#if !defined(ASM_BYTE_OP)
#define ASM_BYTE_OP		asm_byte_op
static char asm_byte_op[] = ".byte";
#endif

#if !defined(ASM_SET_OP)
#define ASM_SET_OP		asm_set_op
static char asm_set_op[] = ".set";
#endif

/* Define assembly labels marking the bounds of various sections.
   Most of these labels need not exist in the object module after assembly,
   since they are defined in this module and only referenced from this
   module.  Such labels are either created with ASM_GENERATE_INTERNAL_LABEL,
   or begin with "*L".  This assumes that the assembler omits from the
   symbol table all labels beginning with "L".
 */

/* TEXT_BEGIN_LABEL and TEXT_END_LABEL delimit this module's .text section. */

#if !defined(TEXT_BEGIN_LABEL)
#define TEXT_BEGIN_LABEL	text_begin_label
static char text_begin_label[] = "*L_Dtext_b";
#endif

#if !defined(TEXT_END_LABEL)
#define TEXT_END_LABEL	text_end_label
static char text_end_label[] = "*L_Dtext_e";
#endif

/* DATA_BEGIN_LABEL and DATA_END_LABEL delimit this module's .data section. */

#if !defined(DATA_BEGIN_LABEL)
#define DATA_BEGIN_LABEL	data_begin_label
static char data_begin_label[] = "*L_Ddata_b";
#endif

#if !defined(DATA_END_LABEL)
#define DATA_END_LABEL	data_end_label
static char data_end_label[] = "*L_Ddata_e";
#endif

/* BSS_BEGIN_LABEL and BSS_END_LABEL delimit this module's .bss section.
   Note that with many assemblers, declaring objects in the .bss section
   differs from .text and .data.  Generally you cannot go into the .bss
   section, define a label, and then exit the bss section.  Rather, you
   use a .bss directive to declare a label AND allocate memory for it.
   We don't want these labels to consume any target memory.  Fortunately,
   the 80960 assembler allows you to specify an allocation size of 0,
   which supports declaring the label.
 */

#if !defined(BSS_BEGIN_LABEL)
#define BSS_BEGIN_LABEL	bss_begin_label
static char bss_begin_label[] = "*L_Dbss_b";
#endif

#if !defined(BSS_END_LABEL)
#define BSS_END_LABEL	bss_end_label
static char bss_end_label[] = "*L_Dbss_e";
#endif

#if !defined(DEBUG_INFO_GLOBAL_BEGIN_LABEL)
#define DEBUG_INFO_GLOBAL_BEGIN_LABEL	debug_info_global_begin_label
static char debug_info_global_begin_label[] = "*__B.debug_info";
	/* This label, defined by the linker, marks the
	   program-wide beginning of the .debug_info section.
	 */
#endif

#if !defined(DEBUG_INFO_BEGIN_LABEL)
#define DEBUG_INFO_BEGIN_LABEL	debug_info_begin_label
static char debug_info_begin_label[] = "*L_D_b";
	/* This label marks the beginning of this compilation unit's
	   .debug_info section.
	 */
#endif

#if !defined(DEBUG_INFO_END_LABEL)
#define DEBUG_INFO_END_LABEL	debug_info_end_label
static char debug_info_end_label[] = "*L_D_e";
	/* This label marks the end of this compilation unit's
	   .debug_info section.
	 */
#endif

#if !defined(DEBUG_LOC_GLOBAL_BEGIN_LABEL)
#define DEBUG_LOC_GLOBAL_BEGIN_LABEL	debug_loc_global_begin_label
static char debug_loc_global_begin_label[] = "*__B.debug_loc";
	/* This label, defined by the linker, marks the
	   program-wide beginning of the .debug_loc section.
	 */
#endif

#if !defined(DEBUG_LOC_LABEL_CLASS)
#define DEBUG_LOC_LABEL_CLASS	debug_loc_label_class
static char debug_loc_label_class[] = "L_DLL";
	/* Format of the label defined on a location list.
	   The label is referenced in the DW_AT_location attribute
	   of some _DECL DIE.
	 */
#endif

#if !defined(DEBUG_PUBNAMES_BEGIN_LABEL)
#define DEBUG_PUBNAMES_BEGIN_LABEL  debug_pubnames_begin_label
static char debug_pubnames_begin_label[] = "*L_DP_b";
	/* This label marks the beginning of this compilation unit's
	   .debug_pubnames section.
	 */
#endif

#if !defined(DEBUG_PUBNAMES_END_LABEL)
#define DEBUG_PUBNAMES_END_LABEL	debug_pubnames_end_label
static char debug_pubnames_end_label[] = "*L_DP_e";
	/* This label marks the end of this compilation unit's
	   .debug_pubnames section.
	 */
#endif

#if !defined(DEBUG_ARANGES_BEGIN_LABEL)
#define DEBUG_ARANGES_BEGIN_LABEL  debug_aranges_begin_label
static char debug_aranges_begin_label[] = "*L_DAR_b";
	/* This label marks the beginning of this compilation unit's
	   .debug_aranges section.
	 */
#endif

#if !defined(DEBUG_ARANGES_END_LABEL)
#define DEBUG_ARANGES_END_LABEL	debug_aranges_end_label
static char debug_aranges_end_label[] = "*L_DAR_e";
	/* This label marks the end of this compilation unit's
	   .debug_aranges section.
	 */
#endif

#if !defined(DEBUG_ABBREV_GLOBAL_BEGIN_LABEL)
#define DEBUG_ABBREV_GLOBAL_BEGIN_LABEL	debug_abbrev_global_begin_label
static char debug_abbrev_global_begin_label[] = "*__B.debug_abbrev";
	/* This label, defined by the linker, marks the
	   program-wide beginning of the .debug_abbrev section.
	 */
#endif

#if !defined(DEBUG_ABBREV_BEGIN_LABEL)
#define DEBUG_ABBREV_BEGIN_LABEL	debug_abbrev_begin_label
static char debug_abbrev_begin_label[] = "*L_DAB_b";
	/* This label marks the beginning of this compilation unit's
	   .debug_abbrev section.
	 */
#endif

#if !defined(DEBUG_LINE_GLOBAL_BEGIN_LABEL)
#define DEBUG_LINE_GLOBAL_BEGIN_LABEL	debug_line_global_begin_label
static char debug_line_global_begin_label[] = "*__B.debug_line";
	/* This label, generally defined by the linker, marks the
	   program-wide beginning of the .debug_line section.
	 */
#endif

#if !defined(DEBUG_LINE_BEGIN_LABEL)
#define DEBUG_LINE_BEGIN_LABEL	debug_line_begin_label
static char debug_line_begin_label[] = "*L_DL_b";
	/* This label marks the beginning of this compilation unit's
	   .debug_line section.
	 */
#endif

#if !defined(DEBUG_LINE_POST_PROLOGUE_LENGTH_LABEL)
#define DEBUG_LINE_POST_PROLOGUE_LENGTH_LABEL	debug_line_post_prologue_length_label
static char debug_line_post_prologue_length_label[] = "*L_DL_ppl";
	/* The label defined after the prologue length field in the statement
	   program prologue.  This label is used to compute that length.
	 */
#endif

#if !defined(DEBUG_LINE_BEGIN_PROGRAM_LABEL)
#define DEBUG_LINE_BEGIN_PROGRAM_LABEL	debug_line_begin_program_label
static char debug_line_begin_program_label[] = "*L_DL_bp";
	/* This label is defined after the statement program prologue,
	   marking the beginning of the statement program itself.
	 */
#endif

#if !defined(DEBUG_LINE_END_LABEL)
#define DEBUG_LINE_END_LABEL	debug_line_end_label
static char debug_line_end_label[] = "*L_DL_e";
	/* This label marks the end of this compilation unit's
	   .debug_line section.
	 */
#endif

#if !defined(DEBUG_FRAME_GLOBAL_BEGIN_LABEL)
#define DEBUG_FRAME_GLOBAL_BEGIN_LABEL	debug_frame_global_begin_label
static char debug_frame_global_begin_label[] = "*__B.debug_frame";
	/* This label, generally defined by the linker, marks the
	   program-wide beginning of the .debug_frame section.
	 */
#endif

#if !defined(DWARF_CIE_BEGIN_LABEL_CLASS)
#define DWARF_CIE_BEGIN_LABEL_CLASS	dwarf_cie_begin_label_class
static char dwarf_cie_begin_label_class[] = "L_DCIEB";
	/* This label class is used in ASM_GENERATE_INTERNAL_LABEL to
	   generate labels in .debug_frame that mark the beginning of a CIE.
	 */
#endif

#if !defined(DWARF_CIE_END_LABEL_CLASS)
#define DWARF_CIE_END_LABEL_CLASS	dwarf_cie_end_label_class
static char dwarf_cie_end_label_class[] = "L_DCIEE";
	/* This label class is used in ASM_GENERATE_INTERNAL_LABEL to
	   generate labels in .debug_frame that mark the end of a CIE.
	 */
#endif

#if !defined(DWARF_FDE_BEGIN_LABEL_CLASS)
#define DWARF_FDE_BEGIN_LABEL_CLASS	dwarf_fde_begin_label_class
static char dwarf_fde_begin_label_class[] = "L_DFDEB";
	/* This label class is used in ASM_GENERATE_INTERNAL_LABEL to
	   generate labels in .debug_frame that mark the beginning of an FDE.
	 */
#endif

#if !defined(DWARF_FDE_END_LABEL_CLASS)
#define DWARF_FDE_END_LABEL_CLASS	dwarf_fde_end_label_class
static char dwarf_fde_end_label_class[] = "L_DFDEE";
	/* This label class is used in ASM_GENERATE_INTERNAL_LABEL to
	   generate labels in .debug_frame that mark the end of an FDE.
	 */
#endif

#if !defined(DEBUG_MACINFO_GLOBAL_BEGIN_LABEL)
#define DEBUG_MACINFO_GLOBAL_BEGIN_LABEL	debug_macinfo_global_begin_label
static char debug_macinfo_global_begin_label[] = "*__B.debug_macinfo";
	/* This label, generally defined by the linker, marks the
	   program-wide beginning of the .debug_macinfo section.
	 */
#endif

#if !defined(DEBUG_MACINFO_BEGIN_LABEL)
#define DEBUG_MACINFO_BEGIN_LABEL	debug_macinfo_begin_label
static char debug_macinfo_begin_label[] = "*L_DM_b";
	/* This label marks the beginning of this compilation unit's
	   .debug_macinfo section.
	 */
#endif

#if !defined(TYPE_DIE_LABEL_FMT)
#define TYPE_DIE_LABEL_FMT	type_die_label_fmt
static char type_die_label_fmt[] = "*L_DT%s%u";
	/* Format of the label on a _TYPE DIE.
	   %s is "c", "v" or "cv" for qualified types, and "" otherwise.
	   The %u is for the TYPE_UID of the type's main variant.
	 */
#endif

#if !defined(DECL_DIE_LABEL_CLASS)
#define DECL_DIE_LABEL_CLASS	decl_die_label_class
static char decl_die_label_class[] = "L_DD";
	/* Format of the label defined on a _DECL DIE.
	   The decl's DECL_UID is encorporated into the label
	   by ASM_GENERATE_INTERNAL_LABEL.
	   This label is used to reference the _DECL DIE from a
	   DW_AT_abstract_origin attribute.
	 */
#endif

#if !defined(PUB_DIE_LABEL_CLASS)
#define PUB_DIE_LABEL_CLASS	pub_die_label_class
static char pub_die_label_class[] = "L_DP";
	/* A label with this format is defined at the beginning of DIE's
	   that are listed in the .debug_pubnames section.  Such DIE's
	   represent _DECL nodes, and the label name will include
	   the decl's DECL_UID.
	 */
#endif

#if !defined(BLOCK_BEGIN_LABEL_FMT)
#define BLOCK_BEGIN_LABEL_FMT	block_begin_label_fmt
static char block_begin_label_fmt[] = "*L_DB%u_b";
	/* A label with this format is defined in the .text section when
	   final_scan_insn encounters a NOTE_INSN_BLOCK_BEG.
	   It is used for the low_pc attribute value of lexical blocks
	   and inlined functions.
	   The %u is for the block number passed in to dwarfout_begin_block.
	 */
#endif

#if !defined(BLOCK_END_LABEL_FMT)
#define BLOCK_END_LABEL_FMT	block_end_label_fmt
static char block_end_label_fmt[] = "*L_DB%u_e";
	/* A label with this format is defined in the .text section when
	   final_scan_insn encounters a NOTE_INSN_BLOCK_END.
	   It is used for the high_pc attribute value of lexical blocks
	   and inlined functions.
	   The %u is for the block number passed in to dwarfout_end_block.
	 */
#endif

#if !defined(INSN_LABEL_FMT)
#define INSN_LABEL_FMT		insn_label_fmt
static char insn_label_fmt[] = "*L_DIL%u_%u";
	/* A label with this format is defined in the .text section at
	   the point where a user-label was defined in the source.
	   It is referenced in the low_pc attribute value for the label's DIE.
	   The first %u is for current_funcdef_number.
	   The second %u is for the label insn's INSN_UID.
	 */
#endif

#if !defined(INSN_LE_BOUNDARY_LABEL_CLASS)
#define INSN_LE_BOUNDARY_LABEL_CLASS	insn_le_boundary_label_class
static char insn_le_boundary_label_class[] = "L_DIB";
	/* This label class is used in ASM_GENERATE_INTERNAL_LABEL to
	   generate labels in .text that mark the boundary of the valid
	   range for one or more location expressions.
	 */
#endif

#if !defined(FUNC_END_LABEL_FMT)
#define FUNC_END_LABEL_FMT	func_end_label_fmt
static char func_end_label_fmt[] = "*L_DF%u_e";
	/* A label with this format is defined in the .text section at
	   the end of each function to mark its high_pc value.
	   The %u is for the current_funcdef_number.
	 */
#endif

#if !defined(LINE_CODE_LABEL_CLASS)
#define LINE_CODE_LABEL_CLASS	line_code_label_class
static char line_code_label_class[] = "L_DLC";
	/* This label class is used in ASM_GENERATE_INTERNAL_LABEL to
	   generate labels in .text at the beginning of source statements.

	   Differences between two such labels are used in the .debug_line
	   section to increment the state machine's 'address' register.
	   Internal labels are used here because they need not exist
	   after assembly.
	 */
#endif

/* LOC_BEGIN_LABEL_CLASS and LOC_END_LABEL_CLASS are used in
   ASM_GENERATE_INTERNAL_LABEL to define labels delimiting
   a location expression.  These labels are used to compute the length
   of the expression.  Internal labels are used here because they need
   not exist after assembly.
 */

#if !defined(LOC_BEGIN_LABEL_CLASS)
#define LOC_BEGIN_LABEL_CLASS	loc_begin_label_class
static char loc_begin_label_class[] = "L_Dlb";
#endif

#if !defined(LOC_END_LABEL_CLASS)
#define LOC_END_LABEL_CLASS	loc_end_label_class
static char loc_end_label_class[] = "L_Dle";
#endif

#if !defined(SIBLING_LABEL_CLASS)
#define SIBLING_LABEL_CLASS	sibling_label_class
static char sibling_label_class[] = "L_DS";
	/* Labels of this class are used to implement the DW_AT_sibling
	   attribute.
	 */
#endif

#if !defined(INLINE_ATTR_LABEL_CLASS)
#define INLINE_ATTR_LABEL_CLASS inline_attr_label_class
static char inline_attr_label_class[] = "L_Din";
	/* Symbols of this class are used to implement the DW_AT_inline
	   attribute.  Since we don't know the attribute's value at the
	   time it needs to be emitted, we emit the attribute value
	   symbolically, for example using   .byte L_Din99   where 99
	   is the function decl's DECL_UID.  In dwarfout_finish, we then
	   use     .set L_Din99,value   where 'value' is the appropriate
	   attribute value and depends on whether the function was actually
	   inlined anywhere.
	 */
#endif

/* Define an enumeration to give all abbreviations a name and id */

enum abbrev_code {

#define DEFABBREVCODE(id, name, tag, children, attrs, forms)	id,
#include "i_dwarf2.def"
	LAST_AND_UNUSED_ABBREV_CODE
};

/* Define an array that describes all abbreviations.
   The array can be indexed by an "enum abbrev_code".
 */

typedef struct {
	enum abbrev_code abbrev_id;	/* DW_ABBREV_... */
	char		*name;
	unsigned long	die_tag;	/* DW_TAG_... */
	int		has_children;	/* DW_CHILDREN_yes or DW_CHILDREN_no */
	int		is_referenced;	/* True if this abbreviation was
					   referenced by some DIE.  If not,
					   we don't need to describe this
					   abbreviation in .debug_abbrev.
					 */
	char		*attributes;	/* attributes is statically initialized
					   at compile time with a string that
					   encodes the attribute for this DIE.
					   Before use, we replace the string
					   with an array of 'unsigned long's
					   that identify the attributes
					   directly with their DW_AT_* id.
					   This is a time saver, eliminating
					   the need to map an encoded name to
					   its attribute id at runtime.
					 */

/* Retrieve the i'th attribute of an abbreviation pointed to by 'a' */
#define	XABBREV_ATTR(a,i)  ((unsigned long*)((a)->attributes))[i]

	char		*form_encodings;
} Debug_Abbrev;

static Debug_Abbrev abbrev_table[] = {

#define DEFABBREVCODE(id, name, tag, children, attrs, forms) \
	{ id, name, tag, children, 0, attrs, forms },
#include "i_dwarf2.def"

	{ LAST_AND_UNUSED_ABBREV_CODE, "", 0, 0, 0, NULL, NULL }
};

/* Define a table of attribute information for mapping between our
   encoded attribute name, the attribute id, and the attribute name.
 */

static struct {
	unsigned long	id;			/* DW_AT_...   */
	char		*encoded_name;		/* "xy"        */
	char		*id_name;		/* "DW_AT_..." */
} attr_mapping[] = {

#define DEFATTRENCODE(enc, id, name)	{ id, enc, name },
#include "i_dwarf2.def"
		{ 0, NULL, NULL }
};

static struct attr_form {
	char		*encoded_name;
	unsigned long	id;
	char		*id_name;
} form_mapping[] = {
#define DEFFORMCODE(enc, id, name)	{ enc, id, name },
#include "i_dwarf2.def"
	{ NULL, 0, NULL }
};

/* Map a DW_AT_ attribute id to its printable name */

static char*
dwarf_attribute_name(attr_id)
unsigned long	attr_id;
{
	int	i;

	for (i=0; attr_mapping[i].encoded_name; i++)
	{
		if (attr_id == attr_mapping[i].id)
			return attr_mapping[i].id_name;
	}
	abort();
	return NULL;
}

/* Map a DW_TAG_ id to its printable name */

static char*
dwarf_tag_name(tag)
unsigned long	tag;
{
	char	*name;

	switch (tag)
	{
	case DW_TAG_array_type:		name = "DW_TAG_array_type"; break;
	case DW_TAG_class_type:		name = "DW_TAG_class_type"; break;
	case DW_TAG_entry_point:	name = "DW_TAG_entry_point"; break;
	case DW_TAG_enumeration_type:	name = "DW_TAG_enumeration_type";break;
	case DW_TAG_formal_parameter:	name = "DW_TAG_formal_parameter";break;
	case DW_TAG_imported_declaration:
				name = "DW_TAG_imported_declaration"; break;
	case DW_TAG_label:		name = "DW_TAG_label"; break;
	case DW_TAG_lexical_block:	name = "DW_TAG_lexical_block"; break;
	case DW_TAG_member:		name = "DW_TAG_member"; break;
	case DW_TAG_pointer_type:	name = "DW_TAG_pointer_type"; break;
	case DW_TAG_reference_type:	name = "DW_TAG_reference_type"; break;
	case DW_TAG_compile_unit:	name = "DW_TAG_compile_unit"; break;
	case DW_TAG_string_type:	name = "DW_TAG_string_type"; break;
	case DW_TAG_structure_type:	name = "DW_TAG_structure_type"; break;
	case DW_TAG_subroutine_type:	name = "DW_TAG_subroutine_type"; break;
	case DW_TAG_typedef:		name = "DW_TAG_typedef"; break;
	case DW_TAG_union_type:		name = "DW_TAG_union_type"; break;
	case DW_TAG_unspecified_parameters:
				name = "DW_TAG_unspecified_parameters"; break;
	case DW_TAG_variant:		name = "DW_TAG_variant"; break;
	case DW_TAG_common_block:	name = "DW_TAG_common_block"; break;
	case DW_TAG_common_inclusion:	name = "DW_TAG_common_inclusion";break;
	case DW_TAG_inheritance:	name = "DW_TAG_inheritance"; break;
	case DW_TAG_inlined_subroutine:
				name = "DW_TAG_inlined_subroutine"; break;
	case DW_TAG_module:		name = "DW_TAG_module"; break;
	case DW_TAG_ptr_to_member_type:
				name = "DW_TAG_ptr_to_member_type"; break;
	case DW_TAG_set_type:		name = "DW_TAG_set_type"; break;
	case DW_TAG_subrange_type:	name = "DW_TAG_subrange_type"; break;
	case DW_TAG_with_stmt:		name = "DW_TAG_with_stmt"; break;
	case DW_TAG_access_declaration:
				name = "DW_TAG_access_declaration"; break;
	case DW_TAG_base_type:		name = "DW_TAG_base_type"; break;
	case DW_TAG_catch_block:	name = "DW_TAG_catch_block"; break;
	case DW_TAG_const_type:		name = "DW_TAG_const_type"; break;
	case DW_TAG_constant:		name = "DW_TAG_constant"; break;
	case DW_TAG_enumerator:		name = "DW_TAG_enumerator"; break;
	case DW_TAG_file_type:		name = "DW_TAG_file_type"; break;
	case DW_TAG_friend:		name = "DW_TAG_friend"; break;
	case DW_TAG_namelist:		name = "DW_TAG_namelist"; break;
	case DW_TAG_namelist_item:	name = "DW_TAG_namelist_item"; break;
	case DW_TAG_packed_type:	name = "DW_TAG_packed_type"; break;
	case DW_TAG_subprogram:		name = "DW_TAG_subprogram"; break;
	case DW_TAG_template_type_param:
				name = "DW_TAG_template_type_param"; break;
	case DW_TAG_template_value_param:
				name = "DW_TAG_template_value_param"; break;
	case DW_TAG_thrown_type:	name = "DW_TAG_thrown_type"; break;
	case DW_TAG_try_block:		name = "DW_TAG_try_block"; break;
	case DW_TAG_variant_part:	name = "DW_TAG_variant_part"; break;
	case DW_TAG_variable:		name = "DW_TAG_variable"; break;
	case DW_TAG_volatile_type:	name = "DW_TAG_volatile_type"; break;
	case DW_TAG_lo_user:		name = "DW_TAG_lo_user"; break;
	case DW_TAG_hi_user:		name = "DW_TAG_hi_user"; break;
	default: name = "DW_TAG_<unknown>"; break;
	}

	return name;
}

/*
 * ------------------------------------------------------------------------
 * Define functions for emitting the assembly language for all forms of
 * DWARF data.  Each of these functions takes a 'comment' parameter, which
 * is a character string that is emitted along with the data as an assembly
 * language comment when flag_verbose_asm is non-zero.
 * ------------------------------------------------------------------------
 */

#if !defined(ASM_COMMENT_START)
#define ASM_COMMENT_START asm_comment_start
static char asm_comment_start[] = ";#";
#endif

#define NO_COMMENT	((char*)NULL)

/* Append an assembly language comment to the end of the current line. */

#define DWARF_ASM_COMMENT(FILE, COMMENT)				\
  do {									\
    if (flag_verbose_asm && (COMMENT))					\
      (void) fprintf((FILE), "\t%s %s", ASM_COMMENT_START, (COMMENT));	\
  } while (ALWAYS_ZERO)

/* Determine the number of bytes required to represent
   an unsigned LEB128 number.  */

static int
udata_byte_length(u)
unsigned long	u;	/* Handle only 4-byte numbers for now. */
{
	if (u <= 0x7f) return 1;
	if (u <= 0x3fff) return 2;
	if (u <= 0x1fffff) return 3;
	if (u <= 0xfffffff) return 4;
	return 5;
}

/* Output an unsigned LEB128 value. */

static void
dwarf_output_udata(value, comment)
unsigned long	value;
char		*comment;
{
	unsigned long	orig_value = value;

#if !defined(ONE_VALUE_PER_BYTE_DIRECTIVE)
	/* Put all byte values, comma separated, in one directive. */
	(void) fprintf(asm_out_file, "\t%s\t", ASM_BYTE_OP);
#endif

	do {
		unsigned int	byte = value & 0x7f;

		value >>= 7;
		if (value != 0)	/* more bytes to follow */
			byte |= 0x80;
#if defined(ONE_VALUE_PER_BYTE_DIRECTIVE)
		(void) fprintf(asm_out_file,"\t%s\t", ASM_BYTE_OP);
#endif
		(void) fprintf(asm_out_file, "0x%x", byte);

		if (value) {	/* more bytes to follow */
#if defined(ONE_VALUE_PER_BYTE_DIRECTIVE)
			(void) fputc('\n', asm_out_file);
#else
			(void) fprintf(asm_out_file, ", ");
#endif
		} else {
			if (flag_verbose_asm) {
				(void) fprintf(asm_out_file,
					"\t%s ULEB128 = %lu",
					ASM_COMMENT_START, orig_value);
				if (orig_value > 0x7f)
				  (void) fprintf(asm_out_file,
						" = 0x%lx", orig_value);
				if (comment && *comment)
				  (void) fprintf(asm_out_file,", %s",comment);
			}
			(void) fputc('\n', asm_out_file);
		}
	} while (value);
}

/* Output a signed LEB128 value.  This function is rarely used.  */

static void
dwarf_output_sdata(value, comment)
long	value;
char	*comment;
{
	long	orig_value = value;
	int	negative = value < 0;
	int	more;

#if !defined(ONE_VALUE_PER_BYTE_DIRECTIVE)
	/* Put all byte values, comma separated, in one directive. */
	(void) fprintf(asm_out_file, "\t%s\t", ASM_BYTE_OP);
#endif

	do {
		unsigned int	byte = value & 0x7f;

		value >>= 7;
		if (negative)
			value |= 0xfe000000;  /* sign extend */
		if ((value == 0 && (byte & 0x40) == 0)
		     || (value == -1 && (byte & 0x40)))
			more = 0;
		else {
			byte |= 0x80;
			more = 1;
		}

#if defined(ONE_VALUE_PER_BYTE_DIRECTIVE)
		(void) fprintf(asm_out_file,"\t%s\t", ASM_BYTE_OP);
#endif
		(void) fprintf(asm_out_file, "0x%x", byte);

		if (more) {
#if defined(ONE_VALUE_PER_BYTE_DIRECTIVE)
			(void) fputc('\n', asm_out_file);
#else
			(void) fprintf(asm_out_file, ", ");
#endif
		} else {
			if (flag_verbose_asm) {
				(void) fprintf(asm_out_file,
					"\t%s SLEB128 = %ld",
					ASM_COMMENT_START, orig_value);
				if (negative || orig_value > 0x3f)
					(void) fprintf(asm_out_file,
						" = 0x%.8lx", orig_value);
				if (comment && *comment)
				  (void) fprintf(asm_out_file,", %s",comment);
			}
			(void) fputc('\n', asm_out_file);
		}
	} while (more);
}

/* Output the abbreviation code at the beginning of a DIE. */

#define dwarf_output_abbreviation(a)					\
  do {									\
    dwarf_output_udata((unsigned long) (a), abbrev_table[(a)].name);	\
    abbrev_table[(a)].is_referenced = 1;				\
  } while (ALWAYS_ZERO)

/* Output a NULL DIE, usually used to mark the end of a list of DIEs. */

#define dwarf_output_null_die(comment)	dwarf_output_udata(0, (comment))

static void
dwarf_output_data1(value, comment)
int	value;
char	*comment;
{
#if defined(ASM_OUTPUT_DWARF_BYTE)
	ASM_OUTPUT_DWARF_BYTE(asm_out_file, value);
#else
	(void)fprintf(asm_out_file,"\t%s\t%d", ASM_BYTE_OP, value);
#endif
	DWARF_ASM_COMMENT(asm_out_file, comment);
	(void) fputc('\n', asm_out_file);
}

static void
dwarf_output_data1_symbolic(value, comment)
char	*value;
char	*comment;
{
	(void)fprintf(asm_out_file,"\t%s\t", ASM_BYTE_OP);
	assemble_name(asm_out_file, value);
	DWARF_ASM_COMMENT(asm_out_file, comment);
	(void) fputc('\n', asm_out_file);
}

#define dwarf_output_flag(f, comment)  dwarf_output_data1((f)?1:0, (comment))

#define dwarf_output_loc_expr_opcode(opc, comment) \
			dwarf_output_data1((int) (opc), (comment))

static void
dwarf_output_data2(value, comment)
int	value;
char	*comment;
{
#if defined(ASM_OUTPUT_DWARF_SHORT)
	ASM_OUTPUT_DWARF_SHORT(asm_out_file, value);
#else
	(void) fprintf(asm_out_file, "\t%s\t%d", UNALIGNED_SHORT_ASM_OP, value);
#endif
	DWARF_ASM_COMMENT(asm_out_file, comment);
	(void) fputc('\n', asm_out_file);
}

static void
dwarf_output_data4(value, comment)
int	value;
char	*comment;
{
#if defined(ASM_OUTPUT_DWARF_WORD)
	ASM_OUTPUT_DWARF_WORD(asm_out_file, value);
#else
	(void) fprintf(asm_out_file, "\t%s\t%d", UNALIGNED_INT_ASM_OP, value);
#endif
	DWARF_ASM_COMMENT(asm_out_file, comment);
	(void) fputc('\n', asm_out_file);
}

static void
dwarf_output_string(value, comment)
char	*value;
char	*comment;
{
#if !defined(ASM_OUTPUT_DWARF_STRINGZ)
#define ASM_OUTPUT_DWARF_STRINGZ(FILE,S)			\
	do {							\
		ASM_OUTPUT_ASCII((FILE), (S), strlen(S)+1);	\
	} while (ALWAYS_ZERO)
#endif
	/* Put comment first, since default ASM_OUTPUT_ASCII emits a newline */
	if (flag_verbose_asm && comment && *comment)
		(void) fprintf(asm_out_file, "\t%s %s\n",
					ASM_COMMENT_START, comment);
	ASM_OUTPUT_DWARF_STRINGZ(asm_out_file, value);
}

/* Output, in 1-byte, the difference between two labels
   that are in the same section.
 */

static void
dwarf_output_delta1(minuend, subtrahend, comment)
char	*minuend;
char	*subtrahend;	/* value subtracted from minuend */
char	*comment;
{
	(void) fprintf(asm_out_file,"\t%s\t", ASM_BYTE_OP);
	assemble_name(asm_out_file, minuend);
	(void) fprintf(asm_out_file, " - ");
	assemble_name(asm_out_file, subtrahend);
	DWARF_ASM_COMMENT(asm_out_file, comment);
	(void) fputc('\n', asm_out_file);
}

/* Output, in a 2-byte word, the difference between two labels
   that are in the same section.
 */

static void
dwarf_output_delta2(minuend, subtrahend, comment)
char	*minuend;
char	*subtrahend;	/* value subtracted from minuend */
char	*comment;
{
	(void) fprintf(asm_out_file,"\t%s\t", UNALIGNED_SHORT_ASM_OP);
	assemble_name(asm_out_file, minuend);
	(void) fprintf(asm_out_file, " - ");
	assemble_name(asm_out_file, subtrahend);
	DWARF_ASM_COMMENT(asm_out_file, comment);
	(void) fputc('\n', asm_out_file);
}

/* Output, in a 4-byte word, the difference between two labels
   that are in the same section.
 */

static void
dwarf_output_delta4(minuend, subtrahend, comment)
char	*minuend;
char	*subtrahend;	/* value subtracted from minuend */
char	*comment;
{
	(void) fprintf(asm_out_file,"\t%s\t", UNALIGNED_INT_ASM_OP);
	assemble_name(asm_out_file, minuend);
	(void) fprintf(asm_out_file, " - ");
	assemble_name(asm_out_file, subtrahend);
	DWARF_ASM_COMMENT(asm_out_file, comment);
	(void) fputc('\n', asm_out_file);
}

/* Output, in a 4-byte word, the difference between two labels
   that are in the same section, minus 4.  This is primarily used
   to emit the four byte length field at the beginning of some records.
   We need to subtract 4 because the length if often defined as not
   including the 4-byte length word itself.  The resultant code appears as:

	lab1: .word  lab2 - lab1 - 4
	      ....
	lab2:
 */

static void
dwarf_output_delta4_minus4(minuend, subtrahend, comment)
char	*minuend;
char	*subtrahend;	/* value subtracted from minuend */
char	*comment;
{
	(void) fprintf(asm_out_file,"\t%s\t", UNALIGNED_INT_ASM_OP);
	assemble_name(asm_out_file, minuend);
	(void) fprintf(asm_out_file, " - ");
	assemble_name(asm_out_file, subtrahend);
	(void) fprintf(asm_out_file, " - 4");
	DWARF_ASM_COMMENT(asm_out_file, comment);
	(void) fputc('\n', asm_out_file);
}

/* Output the offset of a DIE from the beginning of the .debug_info section. */

#define dwarf_output_ref4(die_label, comment)	\
	dwarf_output_delta4((die_label), DEBUG_INFO_BEGIN_LABEL, (comment))

static void
dwarf_output_addr(addr, comment)
char	*addr;
char	*comment;
{
	(void) fprintf(asm_out_file, "\t%s\t", UNALIGNED_INT_ASM_OP);
	assemble_name(asm_out_file, addr);
	DWARF_ASM_COMMENT(asm_out_file, comment);
	(void) fputc('\n', asm_out_file);
}

static void
dwarf_output_addr_const(rtl, comment)
rtx	rtl;
char	*comment;
{
	(void) fprintf(asm_out_file, "\t%s\t", UNALIGNED_INT_ASM_OP);
	output_addr_const(asm_out_file, rtl);
	DWARF_ASM_COMMENT(asm_out_file, comment);
	(void) fputc('\n', asm_out_file);
}

/* Define a hash table to record function decls for which the value of
   the DW_AT_inline attribute must yet be emitted.
 */

#define INLINE_ATTR_HASH_TABLE_SIZE	31

static struct inline_attr_info {
	struct inline_attr_info	*next;
	tree			func_decl;
	int			decl_uid;
		/* decl_uid == -1 indicates that an abstract instance DIE for
		   this function has not been emitted.
		 */
	unsigned int		declared_inline : 1;
	unsigned int		actually_inlined : 1;
} *inline_attr_info_hash_table[INLINE_ATTR_HASH_TABLE_SIZE];

/* Record either the 'declared inline' or 'actually inlined' part of the
   DW_AT_inline attribute for a given function.
 */
static void
dwarf_set_function_inline_attribute(func_decl, declaration)
tree	func_decl;
int	declaration;	/* Are we recording the declaration of this function,
			   or an inlining of it?
			 */
{
	struct inline_attr_info	*p;
	unsigned int		hval = 0;
	char			*name;
	unsigned char		*cp;

	name = IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(func_decl));
	for (cp = (unsigned char*) name; *cp; cp++)
		hval += *cp;

	hval %= INLINE_ATTR_HASH_TABLE_SIZE;

	for (p = inline_attr_info_hash_table[hval]; p; p = p->next)
		if (strcmp(name, IDENTIFIER_POINTER(
				DECL_ASSEMBLER_NAME(p->func_decl))) == 0)
			break;

	if (!p)
	{
		p = (struct inline_attr_info*) xmalloc(
					sizeof(struct inline_attr_info));
		p->func_decl = func_decl;
		p->declared_inline = DECL_INLINE(func_decl) ? 1 : 0;
		p->actually_inlined = declaration ? 0 : 1;
		p->decl_uid = declaration ? DECL_UID(func_decl) : -1;

		p->next = inline_attr_info_hash_table[hval];
		inline_attr_info_hash_table[hval] = p;
	}
	else if (!declaration)
	{
		p->actually_inlined = 1;
	}
	else if (p->decl_uid == -1)
	{
		/* An entry for this function exists even though its abstract
		   instance DIE hasn't been emitted.  This happens when the
		   function gets inlined before having had its abstract
		   instance emitted.
		 */

		/* p->func_decl points to the _DECL that was inlined.  Make
		   it point to the abstract instance _DECL instead, just in
		   case they are different.
		 */
		p->func_decl = func_decl;
		p->declared_inline = DECL_INLINE(func_decl) ? 1 : 0;
		p->decl_uid = DECL_UID(func_decl);
	}
	else
		/* We should never record an abstract instance twice! */
		abort();
}

void
dwarfout_set_function_inlined(func_decl)
tree	func_decl;
{
	/* Ignore this FUNCTION_DECL if it refers to a builtin
	   declaration of a builtin function.
	 */
	if (!DECL_EXTERNAL(func_decl) || !DECL_FUNCTION_CODE(func_decl))
		dwarf_set_function_inline_attribute(func_decl, 0);
}

static void
dwarf_output_inline_attribute_values()
{
	int	i;
	struct inline_attr_info	*p;
	char	sym[MAX_ARTIFICIAL_LABEL_BYTES];

	for (i = 0; i < INLINE_ATTR_HASH_TABLE_SIZE; i++)
	{
		for (p = inline_attr_info_hash_table[i]; p; p = p->next)
		{
			int	value;

			/* If we inlined a function, but never emitted
			   an abstract instance DIE for it, don't bother
			   to define the value of its DW_AT_inline attribute.
			 */
			if (p->decl_uid == -1)
				continue;

			if (p->declared_inline)
			{
				value = p->actually_inlined
						? DW_INL_declared_inlined
						: DW_INL_declared_not_inlined;
			}
			else
			{
				value = p->actually_inlined
						? DW_INL_inlined
						: DW_INL_not_inlined;
			}

			(void) ASM_GENERATE_INTERNAL_LABEL(sym,
						INLINE_ATTR_LABEL_CLASS,
						p->decl_uid);
			(void) fprintf(asm_out_file, "\t%s\t", ASM_SET_OP);
			assemble_name(asm_out_file, sym);
			(void) fprintf(asm_out_file, ",%d", value);

			if (flag_verbose_asm)
			{
				char	*name;

				switch (value) {
				case DW_INL_declared_inlined:
					name = "DW_INL_declared_inlined";
					break;
				case DW_INL_declared_not_inlined:
					name = "DW_INL_declared_not_inlined";
					break;
				case DW_INL_inlined:
					name = "DW_INL_inlined";
					break;
				case DW_INL_not_inlined:
					name = "DW_INL_not_inlined";
					break;
				default: abort();
				}

				DWARF_ASM_COMMENT(asm_out_file, name);
			}
			(void) fputc('\n', asm_out_file);
		}
	}
}

/*
 * -------------------------------------------------------
 * Define functions for emitting various attribute values.
 * -------------------------------------------------------
 */

/* Output a DW_AT_sibling attribute.  This function has the side effect
   of writing the referenced sibling label into 'buf'.
   It is the caller's responsibility to ensure this label gets defined
   at the beginning of the sibling DIE.
 */
static void
dwarf_output_sibling_attribute(buf)
char	*buf;
{
	(void) ASM_GENERATE_INTERNAL_LABEL(buf, SIBLING_LABEL_CLASS,
				sibling_counter);
	sibling_counter++;
	dwarf_output_ref4(buf, "DW_AT_sibling");
}

static void
dwarf_output_decl_abstract_origin_attribute(decl)
tree	decl;
{
	char	origin_label[MAX_ARTIFICIAL_LABEL_BYTES];

	(void) ASM_GENERATE_INTERNAL_LABEL(origin_label, DECL_DIE_LABEL_CLASS,
						DECL_UID(decl));
	dwarf_output_ref4(origin_label, "abstract origin");
}

/* Emit some form of _DECL DIE.
   The attributes included in many different _DECL DIEs are basically the
   same.  This function simply emits the abbreviation, followed by the
   appropriate attributes in their proper order.
 */

static void
dwarf_output_common_decl_attributes(abbrev, decl, origin, low_pc, high_pc)
int	abbrev;
tree	decl;
tree	origin;		/* ultimate abstract origin */
char	*low_pc;
char	*high_pc;
{
	char		*name;
	char		*form;
	int		attr_index;
	unsigned long	attr_id;
	int		saw_sibling_attribute = 0;
	char		sib_label[MAX_ARTIFICIAL_LABEL_BYTES];

	/* If this is an abstract instance, emit a label at the start
	   of the DIE that will be referenced by concrete instances.
	 */

	(void) fputc('\n', asm_out_file);
	if (DECL_ABSTRACT(decl))
	{
		char	lab[MAX_ARTIFICIAL_LABEL_BYTES];
		(void) ASM_GENERATE_INTERNAL_LABEL(lab, DECL_DIE_LABEL_CLASS,
						DECL_UID(decl));
		ASM_OUTPUT_LABEL(asm_out_file, lab);
	}

	dwarf_output_abbreviation(abbrev);

	for (attr_index = 0, form = abbrev_table[abbrev].form_encodings;
	     (attr_id = XABBREV_ATTR(&abbrev_table[abbrev], attr_index)) != 0;
	     attr_index++, form++)
	{
		switch (attr_id)
		{
		case DW_AT_name:
			if (DECL_NAME(decl))
				name = IDENTIFIER_POINTER(DECL_NAME(decl));
			if (!name)
				name = "";
			dwarf_output_string(name, NO_COMMENT);
			break;

			/* The file, line and column attributes are always
			   grouped together.  Emit all of them when we see
			   the file attribute.
			 */
		case DW_AT_decl_file:
			dwarf_output_source_coordinates(decl);
			break;
		case DW_AT_decl_line:
			break;
		case DW_AT_decl_column:
			break;

		case DW_AT_type:
		    {
			char	type_label[MAX_ARTIFICIAL_LABEL_BYTES];

			determine_type_die_label(TREE_TYPE(decl), type_label,
				TREE_READONLY(decl), TREE_THIS_VOLATILE(decl));
			dwarf_output_ref4(type_label, "type");
			break;
		    }
		case DW_AT_abstract_origin:
			dwarf_output_decl_abstract_origin_attribute(origin);
			break;
		case DW_AT_low_pc:
			dwarf_output_addr(low_pc, "DW_AT_low_pc");
			break;
		case DW_AT_high_pc:
			dwarf_output_addr(high_pc, "DW_AT_high_pc");
			break;
		case DW_AT_declaration:
			dwarf_output_flag(DECL_EXTERNAL(decl),
						"DW_AT_declaration");
			break;
		case DW_AT_external:
			dwarf_output_flag(TREE_PUBLIC(decl), "DW_AT_external");
			break;
		case DW_AT_const_value:
			dwarf_output_const_value_attribute();
			break;
		case DW_AT_location:
			/* dwarf_location_list should already have
			   been initialized.
			 */
			dwarf_output_location_attribute(form);
			break;
		case DW_AT_sibling:
			saw_sibling_attribute = 1;
			dwarf_output_sibling_attribute(sib_label);
			break;
		default:
			abort();
		}
	}

	if (saw_sibling_attribute)
	{
		/* Emit a label at the beginning of the next DIE.
		   Don't do it now if this DIE has children DIEs
		   which must be emitted first.
		 */

		assert(abbrev_table[abbrev].has_children == DW_CHILDREN_no);
		ASM_OUTPUT_LABEL(asm_out_file, sib_label);
	}
}

/* Support emitting location expressions, constant-valued expressions,
   and location lists.

   'dwarf_location_list' is essentially an array of <address range, location>
   pairs for the current _DECL.
 */

typedef struct {
	rtx	rtl;	/* RTL describing a location or constant value. */
	int	kind;	/* What kind of rtl do we have here? */
#define			GDW_LE_CONST_VALUE	1
#define			GDW_LE_LOCATION		2
#define			GDW_LE_OTHER		3
	/* The 'G' in 'GDW' is for GNU.  It's purpose is to distinguish
	   these identifiers from the DW_* identifiers defined by
	   the DWARF standard.  See dwarf_classify_location() for the
	   meaning of these macros.
	 */

	tree	type;		/* The rtl's _TYPE, used only if kind
				   is GDW_LE_CONST_VALUE.
				 */

	int	low_range;	/* Number used in INSN_LE_BOUNDARY_LABEL_CLASS
				   label that delimits the beginning of the
				   valid range for this location.
				 */
	int	high_range;	/* Number used in INSN_LE_BOUNDARY_LABEL_CLASS
				   label that delimits the end of the
				   valid range for this location.
				 */
} Location;

static Location *dwarf_location_list = NULL;
static int      dwarf_next_location_index;
static int      dwarf_max_locations;

#define dwarf_clear_location_list()	(dwarf_next_location_index = 0)

static void
dwarf_add_location_expression(rtl, kind, type, low_range, high_range)
rtx	rtl;
int	kind;
tree	type;
rtx	low_range;
rtx	high_range;
{
	Location	*lp;

	if (dwarf_next_location_index == dwarf_max_locations)
	{
		int new_size;

		dwarf_max_locations += 10;
		new_size = dwarf_max_locations * sizeof(Location);

		dwarf_location_list = dwarf_location_list
			? (Location*) xrealloc(dwarf_location_list, new_size)
			: (Location*) xmalloc(new_size);
	}

	lp = &dwarf_location_list[dwarf_next_location_index];

	if (kind == GDW_LE_OTHER)
		lp->rtl = NULL;
	else
		lp->rtl = rtl;
	lp->kind = kind;
	lp->type = type;

	/* Translate the RTX bounds into the integers used in labels
	   that mark their locations.
	 */
	if (low_range == NULL_RTX)
		lp->low_range = 0;
	else if (low_range == DWARF_FIRST_OR_LAST_BOUNDARY_NOTE)
		lp->low_range = func_first_le_boundary_number;
	else
		lp->low_range = NOTE_LE_BOUNDARY_NUMBER(low_range);

	if (high_range == NULL_RTX)
		lp->high_range = 0;
	else if (high_range == DWARF_FIRST_OR_LAST_BOUNDARY_NOTE)
		lp->high_range = func_last_le_boundary_number;
	else
		lp->high_range = NOTE_LE_BOUNDARY_NUMBER(high_range);

	dwarf_next_location_index++;
}

/* Determine if the given rtl, which describes a _DECL object's location
   or const value, contains a pseudo-register.  This would indicate that
   the object has no location.
 */

static int
dwarf_location_uses_pseudo_reg(rtl)
rtx	rtl;
{
	char	*fmt;
	int	i;
	RTX_CODE code;

	if (rtl == NULL_RTX)
		return 0;

	code = GET_CODE(rtl);

	if (code == SUBREG)
		do {
			rtl = SUBREG_REG(rtl);
			code = GET_CODE(rtl);
		} while (code == SUBREG);

	if (code == REG)
		return (REGNO(rtl) >= FIRST_PSEUDO_REGISTER);

	fmt = GET_RTX_FORMAT(code);
	for (i = GET_RTX_LENGTH(code) - 1; i >= 0; i--)
	{
		if (fmt[i] == 'e')
		{
			if (dwarf_location_uses_pseudo_reg(XEXP(rtl,i)))
				return 1; 
		}
		else if (fmt[i] == 'E')
		{
			int j;
			for (j = 0; j < XVECLEN(rtl, i); j++)
				if (dwarf_location_uses_pseudo_reg(
							XVECEXP(rtl,i,j)))
					return 1; 
		}
	}

	return 0;
}

/* Classify an RTX that describes the location or const-ness of some object.
   There are three interesting cases:
     1) The object's value is a constant representable at assembly time.
     2) The object resides in a physical register or in a memory location
        whose address can be described with DWARF location expressions.
     3) All other cases, in which we can't describe the object's location.
	This includes RTX's that still contain pseudo registers.
 */

static int
dwarf_classify_location(rtl)
rtx	rtl;
{
	if (rtl == NULL_RTX)
		return GDW_LE_OTHER;

	switch (GET_CODE(rtl))
	{
	case CONST_INT:
	case CONST_DOUBLE:
	case CONST_STRING:
	case LABEL_REF:
	case CONST:
		return GDW_LE_CONST_VALUE;

	case SYMBOL_REF:
		/* Can't represent PIC or PID addresses at assembly time */
		if ((TARGET_PIC && (SYMREF_ETC(rtl) & SYMREF_PICBIT) != 0)
		    || (TARGET_PID && (SYMREF_ETC(rtl) & SYMREF_PIDBIT) != 0))
			return GDW_LE_LOCATION;
		return GDW_LE_CONST_VALUE;

	case PLUS:
		if (GET_CODE(XEXP(rtl,0)) == SYMBOL_REF
		    && (!TARGET_PIC
			|| (SYMREF_ETC(XEXP(rtl,0)) & SYMREF_PICBIT) == 0)
		    && (!TARGET_PID
		        || (SYMREF_ETC(XEXP(rtl,0)) & SYMREF_PIDBIT) == 0)
		    && GET_CODE(XEXP(rtl,1)) == CONST_INT)
			return GDW_LE_CONST_VALUE;
		return GDW_LE_OTHER;

	case SUBREG:
	case REG:
	case MEM:
		return dwarf_location_uses_pseudo_reg(rtl)
			? GDW_LE_OTHER
			: GDW_LE_LOCATION;
	}

	return GDW_LE_OTHER;
}

/* Output a DW_AT_const_value attribute for a variable or a parameter which
   does not have a "location" either in memory or in a register.  These
   things can arise in GNU C when a constant is passed as an actual
   parameter to an inlined function.  They can also arise in C++ where
   declared constants do not necessarily get memory "homes".

   The RTL describing the const value should reside in dwarf_location_list.
 */

static void
dwarf_output_const_value_attribute()
{
	rtx	rtl;
	tree	type;

	assert(dwarf_next_location_index == 1
		&& dwarf_location_list[0].kind == GDW_LE_CONST_VALUE);

	rtl = dwarf_location_list[0].rtl;
	type = dwarf_location_list[0].type;

	switch (GET_CODE(rtl))
	{
	case CONST_INT:
	    {
		int	val = INTVAL(rtl);
		unsigned int uval = val;

		/* Choose the smallest form in which to emit the value.
		   To avoid problems with unsigned types, use an extra
		   byte if possible so that the MSB is 0.
		 */

		if ((TREE_UNSIGNED(type) && uval <= 127)
		    || (!TREE_UNSIGNED(type) && val >= -128 && val <= 127))
		{
			dwarf_output_data1(DW_FORM_data1, "DW_FORM_data1");
			dwarf_output_data1(val, NO_COMMENT);
		}
		else if ((TREE_UNSIGNED(type) && uval <= 0x7fff)
			 || (!TREE_UNSIGNED(type) && val >= -0x8000
						  && val <= 0x7fff))
		{
			dwarf_output_data1(DW_FORM_data2, "DW_FORM_data2");
			dwarf_output_data2(val, NO_COMMENT);
		}
		else
		{
			dwarf_output_data1(DW_FORM_data4, "DW_FORM_data4");
			dwarf_output_data4(val, NO_COMMENT);
		}

		break;
	    }

	case CONST_DOUBLE:
	    {
		/* Note that a CONST_DOUBLE rtx could represent either an
		   integer or a floating-point constant.  A CONST_DOUBLE is
		   used whenever the constant requires more than one word in
		   order to be adequately represented.  In all such cases,
		   the original mode of the constant value is preserved as
		   the mode of the CONST_DOUBLE rtx.
		 */

		REAL_VALUE_TYPE	rv;

		if (GET_MODE(rtl) == DImode)
		{
			dwarf_output_data1(DW_FORM_data8, "DW_FORM_data8");
			dwarf_output_data4(CONST_DOUBLE_LOW(rtl), NO_COMMENT);
			dwarf_output_data4(CONST_DOUBLE_HIGH(rtl), NO_COMMENT);
			/* EARLY EXIT */
			break;
		}

		REAL_VALUE_FROM_CONST_DOUBLE(rv, rtl);
		/* We need to emit the form before the value.
		   For SFmode use DW_FORM_data4, for DFmode use
		   DW_FORM_data8.  For larger ?Fmodes use DW_FORM_block1.
		 */
		if (GET_MODE(rtl) == SFmode)
			dwarf_output_data1(DW_FORM_data4, "DW_FORM_data4");
		else if (GET_MODE(rtl) == DFmode)
			dwarf_output_data1(DW_FORM_data8, "DW_FORM_data8");
		else if (GET_MODE(rtl) == TFmode)
		{
			dwarf_output_data1(DW_FORM_block1, "DW_FORM_block1");
			dwarf_output_data1(GET_MODE_SIZE(GET_MODE(rtl)),
					"length of long double constant");
		}
		else
		{
			/* We have a CONST_DOUBLE with an unrecognized mode.
			   Rather than aborting, put out *something*.
			   We choose to put out a "double" value of 0.
			 */
			dwarf_output_data1(DW_FORM_data8, "DW_FORM_data8");
			dwarf_output_data4(0, NO_COMMENT);
			dwarf_output_data4(0, NO_COMMENT);
			/* EARLY EXIT */
			break;
		}

		assemble_real(rv, GET_MODE(rtl));

		break;
	    }

	case CONST_STRING:
		dwarf_output_data1(DW_FORM_string, "DW_FORM_string");
		dwarf_output_string(XSTR(rtl, 0), NO_COMMENT);
		break;

	case PLUS:
		if (GET_CODE(XEXP(rtl,0)) != SYMBOL_REF
		    || GET_CODE(XEXP(rtl,1)) != CONST_INT)
			abort();
		/* Fall Through */

	case SYMBOL_REF:
	case LABEL_REF:
	case CONST:
		dwarf_output_data1(DW_FORM_addr, "DW_FORM_addr");
		dwarf_output_addr_const(rtl, NO_COMMENT);
		break;

	default:
		abort();
	}
}

/* A recursive function that emits the postfix DWARF location
   expression to compute the address given by 'rtl'.
 */

static void
dwarf_output_mem_loc_expr(rtl)
rtx	rtl;
{
	unsigned int	regno, opcode, operand;
	char		comment[48];

	switch (GET_CODE(rtl))
	{
	case SUBREG:
		do {
			rtl = SUBREG_REG(rtl);
		} while (GET_CODE(rtl) == SUBREG);

		/* Fall Through */

	case REG:

		regno = REGNO(rtl);
		assert(regno < FIRST_PSEUDO_REGISTER);

		opcode = dwarf_reg_info[regno].base_reg_opcode;
		operand = dwarf_reg_info[regno].base_reg_operand;

		if (flag_verbose_asm)
		{
			if (opcode == DW_OP_bregx)
				(void) sprintf(comment, "DW_OP_bregx %u == %s",
					operand, reg_names[regno]);
			else
				(void) sprintf(comment, "DW_OP_breg%u == %s",
					opcode - DW_OP_breg0, reg_names[regno]);
		}
		else
			comment[0] = '\0';

		dwarf_output_loc_expr_opcode(opcode, comment);
		if (opcode == DW_OP_bregx)
			dwarf_output_udata((unsigned long) operand, NO_COMMENT);
		dwarf_output_sdata(0, "offset from base reg");
		break;

	case MEM:
		dwarf_output_mem_loc_expr(XEXP(rtl, 0));
		dwarf_output_loc_expr_opcode(DW_OP_deref, "DW_OP_deref");
		break;

	case CONST:
		dwarf_output_loc_expr_opcode(DW_OP_addr, "DW_OP_addr");
		dwarf_output_addr_const(rtl, NO_COMMENT);
		break;

	case SYMBOL_REF:
		dwarf_output_loc_expr_opcode(DW_OP_addr, "DW_OP_addr");
		dwarf_output_addr_const(rtl, NO_COMMENT);

		/* Add the PIC or PID bias register, if any */
		if (TARGET_PID && (SYMREF_ETC(rtl) & SYMREF_PIDBIT) != 0)
		{
			dwarf_output_mem_loc_expr(pid_reg_rtx);
			dwarf_output_loc_expr_opcode(DW_OP_plus, "DW_OP_plus");
		}

		if (TARGET_PIC && (SYMREF_ETC(rtl) & SYMREF_PICBIT) != 0)
		{
			/* We don't have any rtx describing the pic bias
			   register.  So emit the primitives right here.
			 */
			dwarf_output_loc_expr_opcode(DW_OP_bregx,
				"DW_OP_bregx pic_bias_reg");
			dwarf_output_udata(
				(unsigned long) DW_OP_regx_i960_pic_bias,
				NO_COMMENT);
			dwarf_output_sdata(0, "offset from pic_bias_reg");
			dwarf_output_loc_expr_opcode(DW_OP_plus, "DW_OP_plus");
		}
		break;

	case PLUS:
	    {
		rtx reg, cint;

		reg = XEXP(rtl,0);
		cint = XEXP(rtl,1);

		if (GET_CODE(reg) != REG)
		{
			rtx tmp = reg;
			reg = cint;
			cint = tmp;
		}

		if (GET_CODE(reg) == REG && GET_CODE(cint) == CONST_INT)
		{
			/* Use DW_OP_breg* */
			regno = REGNO(reg);

			assert(regno < FIRST_PSEUDO_REGISTER);

			opcode = dwarf_reg_info[regno].base_reg_opcode;
			operand = dwarf_reg_info[regno].base_reg_operand;

			if (flag_verbose_asm)
			{
				if (opcode == DW_OP_bregx)
					(void) sprintf(comment,
						"DW_OP_bregx %u == %s",
						operand, reg_names[regno]);
				else
					(void) sprintf(comment,
						"DW_OP_breg%u == %s",
						opcode - DW_OP_breg0,
						reg_names[regno]);
			}
			else
				comment[0] = '\0';

			dwarf_output_loc_expr_opcode(opcode, comment);
			if (opcode == DW_OP_bregx)
				dwarf_output_udata((unsigned long) operand,
							NO_COMMENT);

			dwarf_output_sdata((long)XINT(cint,0),
						"offset from base reg");
			break;
		}

		dwarf_output_mem_loc_expr(XEXP(rtl, 0));
		dwarf_output_mem_loc_expr(XEXP(rtl, 1));
		dwarf_output_loc_expr_opcode(DW_OP_plus, "DW_OP_plus");
		break;
	    }

	case CONST_INT:
	    {
		long	val = INTVAL(rtl);

		if (val < -32768)
		{
			dwarf_output_loc_expr_opcode(DW_OP_const4s,
							"DW_OP_const4s");
			dwarf_output_data4((int) val, NO_COMMENT);
		}
		else if (val < -128)
		{
			dwarf_output_loc_expr_opcode(DW_OP_const2s,
							"DW_OP_const2s");
			dwarf_output_data2((int) val, NO_COMMENT);
		}
		else if (val < 0)
		{
			dwarf_output_loc_expr_opcode(DW_OP_const1s,
							"DW_OP_const1s");
			dwarf_output_data1((int) val, NO_COMMENT);
		}
		else if (val <= 31)
		{
			(void) sprintf(comment, "DW_OP_lit%lu", val);
			dwarf_output_loc_expr_opcode(DW_OP_lit0 + val, comment);
		}
		else if (val <= 255)
		{
			dwarf_output_loc_expr_opcode(DW_OP_const1u,
							"DW_OP_const1u");
			dwarf_output_data1((int) val, NO_COMMENT);
		}
		else if (val <= 0xffff)
		{
			dwarf_output_loc_expr_opcode(DW_OP_const2u,
							"DW_OP_const2u");
			dwarf_output_data2((int) val, NO_COMMENT);
		}
		else
		{
			dwarf_output_loc_expr_opcode(DW_OP_const4u,
							"DW_OP_const4u");
			dwarf_output_data4((int) val, NO_COMMENT);
		}

		break;
	    }

	default:
		abort();
	}
}

/* Emit a location expression for the given 'rtl' using a DWARF
   'DW_FORM_block*' form.

   If 'form' is given as DW_FORM_indirect, then DW_FORM_block1 or
   DW_FORM_block2 is chosen depending on the location expression's length.
   Otherwise, 'form' must be DW_FORM_block1, DW_FORM_block2 or DW_FORM_block4.
 */

static void
dwarf_output_location_expression(rtl, form)
rtx		rtl;
unsigned long	form;
{
	char	begin_label[MAX_ARTIFICIAL_LABEL_BYTES];
	char	end_label[MAX_ARTIFICIAL_LABEL_BYTES];
static	unsigned int	loc_expr_label_counter = 1;
	unsigned int	regno, opcode, operand;
static	char		length_comment[] = "location expression length";
	char		comment[48];

	/* Determine which block representation to use, if not specified. */

	if (form == DW_FORM_indirect)
	{
		/* Most expressions will be representable in DW_FORM_block1,
		   but just to be safe only use this form if we know for sure.
		   Certainly, DW_FORM_block2 is sufficient.
		 */
		char	*form_name;

		if (   rtl == NULL
		    || GET_CODE(rtl) == REG
		    || GET_CODE(rtl) == SUBREG
		    || (GET_CODE(rtl) == MEM
			&& (   GET_CODE(XEXP(rtl,0)) == REG
			    || GET_CODE(XEXP(rtl,0)) == SYMBOL_REF
			    || (   GET_CODE(XEXP(rtl,0)) == PLUS
				&& GET_CODE(XEXP(XEXP(rtl,0),0)) == REG
				&& GET_CODE(XEXP(XEXP(rtl,0),1)) == CONST_INT
			       )
			   )
		       )
		   )
		{
			form = DW_FORM_block1;
			form_name = "DW_FORM_block1";
		}
		else
		{
			form = DW_FORM_block2;
			form_name = "DW_FORM_block2";
		}
		dwarf_output_udata(form, form_name);
	}

	/* Don't bother emitting labels for a zero-length expression */
	if (rtl == NULL)
	{
		if (form == DW_FORM_block1)
			dwarf_output_data1(0, length_comment);
		else if (form == DW_FORM_block2)
			dwarf_output_data2(0, length_comment);
		else
			dwarf_output_data4(0, length_comment);
		return;
	}

	/* Define labels before and after the expression.
	   The difference between the labels is the expression's length.
	 */
	(void) ASM_GENERATE_INTERNAL_LABEL(begin_label, LOC_BEGIN_LABEL_CLASS,
						loc_expr_label_counter);
	(void) ASM_GENERATE_INTERNAL_LABEL(end_label, LOC_END_LABEL_CLASS,
						loc_expr_label_counter);
	loc_expr_label_counter++;

	/* Emit the length of the location expression */

	if (form == DW_FORM_block1)
		dwarf_output_delta1(end_label, begin_label, length_comment);
	else if (form == DW_FORM_block2)
		dwarf_output_delta2(end_label, begin_label, length_comment);
	else
		dwarf_output_delta4(end_label, begin_label, length_comment);

	ASM_OUTPUT_LABEL(asm_out_file, begin_label);

	switch (GET_CODE(rtl))
	{
	case SUBREG:
		do {
			rtl = SUBREG_REG(rtl);
		} while (GET_CODE(rtl) == SUBREG);

		/* Fall Through */
	case REG:
		regno = REGNO(rtl);
		assert (regno < FIRST_PSEUDO_REGISTER);

		opcode = dwarf_reg_info[regno].reg_opcode;
		operand = dwarf_reg_info[regno].reg_operand;

		if (flag_verbose_asm)
		{
			if (opcode == DW_OP_regx)
				(void) sprintf(comment, "DW_OP_regx %u == %s",
					operand, reg_names[regno]);
			else
				(void) sprintf(comment, "DW_OP_reg%u == %s",
					opcode - DW_OP_reg0, reg_names[regno]);
		}
		else
			comment[0] = '\0';

		dwarf_output_loc_expr_opcode(opcode, comment);
		if (opcode == DW_OP_regx)
			dwarf_output_udata((unsigned long) operand, NO_COMMENT);
		break;

	case MEM:
		dwarf_output_mem_loc_expr(XEXP(rtl,0));
		break;
	default:
		abort();
	}

	ASM_OUTPUT_LABEL(asm_out_file, end_label);
}

/* Output the location expression(s) that were saved in 'dwarf_location_list'.
 */

static void
dwarf_output_location_attribute(form_char)
char	*form_char;
{
	int	i, use_location_list;
	unsigned long	form_id = (unsigned long) -1;

	/* Get the DW_FORM_* form id from the form_char encoding */
	for (i = 0; form_mapping[i].encoded_name; i++)
	{
		if (*form_char == *form_mapping[i].encoded_name)
		{
			form_id = form_mapping[i].id;
			break;
		}
	}

	/* Determine if we should use a location list, or a single
	   location expression.

	   Note that in the case of a location list, the TIS DWARF
	   specification is somewhat unclear.  It says that, as the value of
	   the DW_AT_location attribute, a location list is encoded as a
	   "constant offset from the beginning of the .debug_loc section to
	   the first byte of the list for the object in question."
	   My interpretation is that this rules out use of the DW_FORM_ref*
	   forms, since they refer to the .debug_info section.  The legitimate
	   constant forms are DW_FORM_data1, DW_FORM_data2, DW_FORM_data4,
	   and DW_FORM_data8.  We'll only use the first three of these for now.
	 */

	use_location_list = 0;

	switch (form_id)
	{
	case DW_FORM_block1:
	case DW_FORM_block2:
	case DW_FORM_block4:
		break;

	case DW_FORM_data1:
	case DW_FORM_data2:
	case DW_FORM_data4:
		use_location_list = 1;
		break;

	case DW_FORM_indirect:
		/* Use a location list if there's more than one location,
		   or if there's exactly one and a range is specified with it.
		 */
		if (dwarf_next_location_index > 1
		    || (dwarf_next_location_index == 1
			&& dwarf_location_list[0].low_range != 0
			&& dwarf_location_list[0].high_range != 0))
			use_location_list = 1;
		break;

	default:
		abort();
	}

	if (use_location_list)
	{
static  int	label_counter = 1;
		char	location_list_label[MAX_ARTIFICIAL_LABEL_BYTES];

		(void) ASM_GENERATE_INTERNAL_LABEL(location_list_label,
				DEBUG_LOC_LABEL_CLASS, label_counter);
		label_counter++;

		switch (form_id)
		{
		case DW_FORM_indirect:
			dwarf_output_udata(DW_FORM_data4, "DW_FORM_data4");
			/* Fall Through */
		case DW_FORM_data4:
			dwarf_output_delta4(location_list_label,
				DEBUG_LOC_GLOBAL_BEGIN_LABEL, "loc list ref");
			break;
		case DW_FORM_data2:
			dwarf_output_delta2(location_list_label,
				DEBUG_LOC_GLOBAL_BEGIN_LABEL, "loc list ref");
			break;
		case DW_FORM_data1:
			dwarf_output_delta1(location_list_label,
				DEBUG_LOC_GLOBAL_BEGIN_LABEL, "loc list ref");
			break;
		default:
			abort();
		}

		(void) fputc('\n', asm_out_file);
		debug_loc_section();
		ASM_OUTPUT_LABEL(asm_out_file, location_list_label);

		for (i=0; i < dwarf_next_location_index; i++)
		{
			char	lab[MAX_ARTIFICIAL_LABEL_BYTES];

			/* Currently, DWARF 2 cannot represent a constant
			   value in a location expression.
			 */
			if (dwarf_location_list[i].kind != GDW_LE_LOCATION)
				continue;

			(void) ASM_GENERATE_INTERNAL_LABEL(lab,
					INSN_LE_BOUNDARY_LABEL_CLASS,
					dwarf_location_list[i].low_range);
			dwarf_output_delta4(lab, TEXT_BEGIN_LABEL,
						"start address range");

			(void) ASM_GENERATE_INTERNAL_LABEL(lab,
					INSN_LE_BOUNDARY_LABEL_CLASS,
					dwarf_location_list[i].high_range);
			dwarf_output_delta4(lab, TEXT_BEGIN_LABEL,
						"end   address range");

			dwarf_output_location_expression(
				dwarf_location_list[i].rtl, DW_FORM_block2);
		}
		dwarf_output_data4(0, "end loc list");
		dwarf_output_data4(0, "end loc list");
		(void) fputc('\n', asm_out_file);
		debug_info_section();
	}
	else if (dwarf_next_location_index == 0)
		dwarf_output_location_expression(NULL_RTX, form_id);
	else if (dwarf_next_location_index == 1)
	{
		if (dwarf_location_list[0].kind == GDW_LE_LOCATION)
			dwarf_output_location_expression(
					dwarf_location_list[0].rtl, form_id);
		else
			dwarf_output_location_expression(NULL_RTX, form_id);
	}
	else
		abort();
}

/* Define data structures used in the "available locations" dataflow.
   This is similar to the classic "available expressions" dataflow,
   but our expressions are RTLs representing an object's location.
   An RTL is available at any given point in the flow graph if it truly
   represents its decl's location at that point, regardless of which
   path is taken to reach the point.

   We use this dataflow to determine valid ranges for each decl location.
   The dataflow is only used when optimization is enabled, though some of
   its data structures are used at -O0 for convenience.

   Each <decl,rtl> pair is given a unique "available location id", or alid,
   for use in the dataflow bit sets.  alid '0' is unused.  Not all
   alid's between 1 and dwarf_max_alid are used, because some alids are
   reserved for locations which never materialize.  For example, for each
   decl we reserve an alid for a spill location, in case the decl is spilled
   from a register to the stack.  If the decl isn't spilled, this alid isn't
   needed.  Also it may turn out that for alids <decl, rtl_1> and <decl,rtl_2>,
   rtl_1 and rtl_2 are different before register allocation but are the same
   afterwards.  Before running the dataflow, we eliminate duplicate rtls
   for each decl so that a previously used alid might now become unused.
   (This redundancy elimination has not yet been implemented.)

   Each alid is described by a Dwarf_avail_location struct (defined below),
   and all such structs are stored in the array 'avail_locations'.  The
   index into this array is the 'alid'.  For a given decl, all of its alids
   are contiguous.  The first one is given by DECL_DWARF_FIRST_ALID(decl).

   The available locations dataflow has the following overall flow:

	o Prior to register allocation, dwarfout_prep_for_location_tracking
	  does the following:

	  - creates the avail_locations data structure and assign alids to
	    each interesting <decl, rtl> pair.

	  - walks the insn stream looking for SET rtxs which update some rtl
	    in one of our <decl, rtl> pairs.  We set the DWARF_SET_ALID field
	    of the SET rtx to the corresponding alid.  This needs to be done
	    prior to register allocation so that we can know when a register
	    update is done on behalf of a decl.  After register allocation
	    there is no way to tell which decl, if any, is associated with
	    a hard register destination of a SET rtx.  But prior to register
	    allocation, an update to a pseudo register that is some decl's
	    home location, must be done on behalf of that decl.
	    (Note that we can currently only record one alid in a SET rtx.
	    If a multi-word register is being updated, and two of its single
	    registers are the homes of two different decls, then we lose
	    information about one of the decls.  Fixing this requires
	    maintaining a list of alids per SET rtx, which I deemed too
	    expensive for a first implementation.  We might want to
	    revisit this at some point.)

	o During register allocation, if a decl is spilled, a new alid
	  is created for its spill location.  This alid was reserved earlier,
	  so the alids for the decl are still contiguous.
	  Note that potentially any optimization which creates a new home
	  for a decl, can use this technique to add another alid.
	  (Creating the spill location alid has not yet been implemented.)

	o During register allocation and later optimizations, the
	  DWARF_SET_ALID fields in the insn stream should be maintained
	  appropriately.  (Note that it's probably okay to run
	  dwarfout_prep_for_location_tracking well before register allocation,
	  but then you need to maintain the DWARF_SET_ALID field in each
	  subsequent optimization phase, which is potentially a lot of work.)
	  (No explicit work has been done yet to guarantee the DWARF_SET_ALID
	  fields are maintained through optimizations.  We currently take our
	  chances and use whatever ends up in these fields.)

	o Just before final, dwarfout_find_le_boundaries does the following:

	  - performs the dataflow to initialize each block's in, out, gen
	    and kill sets.

	  - makes a single dataflow pass over the insns, interpreting the
	    gen and kill sets at each point to determine where each location
	    becomes valid and invalid.  During this pass, note insns
	    (NOTE_INSN_DWARF_LE_BOUNDARY) are inserted into the insn stream
	    to mark the boundaries.  (final() emits a label when it sees
	    these notes.)  Lists of these note insns are recorded in the
	    appropriate alid's avail_locations[] entry.

	o During DIE emission for local decls, the avail_location[] information
	  for the decl is interpreted and copied into an array of 'Location'
	  structs where it is found by the primitive location expression
	  emission routines.  This copying is needed so that these primitives
	  have a single place to look for decl location information.  (Recall
	  that the avail_location[] information applies to local variables
	  only, not to file scope variables.)
 */

typedef struct {
	tree	decl;	/* decl is never NULL, except for unused alid 0. */
	rtx	rtl;	/* rtl may be NULL.  If so, ignore this alid. */
	int	kind;	/* Classifies 'rtl' as one of GDW_LE_*.
			   This is initialized after register allocation.
			   At that point, we assume any rtl that still
			   contains a pseudo register is uninteresting.
			 */

	int	depth;
	rtx	block_begin_note;
	rtx	block_end_note;
		/* NOTE insns marking this decl's scope.
		   These are NULL for PARM_DECL's, and for local VAR_DECL's
		   which are not in a nested scope (and thus are always in
		   scope).  block_end_note is obtained from the decl's context.
		   'depth' is a temporary nesting-depth value used to find
		   block_begin_note.
		 */
	int	in_scope;
		/* Boolean indicating whether this decl is in scope.  Used
		   to suppress creating a valid range when the decl is not
		   in scope.  For this to work, the range-generation dataflow
		   pass must walk the basic blocks in their physical order,
		   so that the begin_block and end_block note insns can
		   trigger the setting and clearing of 'in_scope'.
		 */
	int	same_as_id;
		/* same_as_id is not used yet.  It is intended to be used
		   for the case that the rtls for two distinct alids become
		   equivalent at some point after the alids are created.
		   This may happen, for example, if the DECL_RTL for a
		   parameter is a register which is allocated to the same
		   register as the decl's DECL_INCOMING_RTL.
		   same_as_id is the alid that this alid matches.
		 */
	rtx	*bounds;
		/* During the available locations dataflow, for n even,
		   bounds[n] and bounds[n+1], if non-zero, are the insn's
		   delimiting a valid range for this alid.  After dataflow,
		   NOTE_INSN_DWARF_LE_BOUNDARY notes are inserted into the
		   insn stream to mark these insns, and the bounds[]
		   are changed to point to these notes.

		   The special value DWARF_FIRST_OR_LAST_BOUNDARY_NOTE in
		   bounds[n] for n odd means the lower bound is prior to
		   the function's first insn.  For bounds[n+1], this value
		   means the upper bound is after the function's last insn,
		   unless we're compiling a CAVE stub function, in which
		   case this value for bounds[n+1] means the upper bound
		   is after the call to the dispatcher.

		   The special value NULL_RTX, if present, must occur in
		   both bounds[n] and bounds[n+1].  It means this decl has
		   one location which is valid over its entire scope.  Thus,
		   a location expression (sans valid range) can be emitted
		   directly in the decl's DIE, rather than in a location
		   list in .debug_loc.
		 */
	int	max_bound_index;
		/* The number of elements in use in 'bounds' (ie, bounds has
		   ((max_bound_index+1)/2) ranges).  This number is always odd.
		 */
	int	bounds_size;
		/* The number of elements malloc'd for 'bounds' (ie, bounds has
		   (bounds_size/2) elements, not all of which are in use).
		 */

#define DWARF_CUR_RANGE_LOWER_BOUND(alid) \
	(avail_locations[alid].bounds[avail_locations[alid].max_bound_index-1])
	/* Assumes 'bounds' is allocated */

#define DWARF_CUR_RANGE_UPPER_BOUND(alid) \
	(avail_locations[alid].bounds[avail_locations[alid].max_bound_index])
	/* Assumes 'bounds' is allocated */

#define DWARF_IS_A_RANGE_ACTIVE(alid) \
	(avail_locations[alid].bounds \
	 && DWARF_CUR_RANGE_UPPER_BOUND(alid) == NULL_RTX)
	/* A range is active if its upper bound hasn't been set yet.
	   (The lower bound is always set immediately when the range
	   is created.)
	 */

} Dwarf_avail_location;

static Dwarf_avail_location* avail_locations;
	/* Array describing each location in the dataflow.  We don't need
	   to be too concerned about memory use here.  The number of
	   alids in a function is currently bounded by
	   (4 * number_of_parameters + 2 * number_of_locals).
	   The '4' possible current locations of a PARM_DECL include 2 for
	   its DECL_INCOMING_RTL, one for its DECL_RTL and one for a
	   spill slot.  A VAR_DECL doesn't have any incoming rtls.
	 */

static int	dwarf_max_alid;
	/* Maximum alid assigned so far. */
static int	dwarf_alids_allocated;
	/* Number of elements currently malloc'd for avail_locations.  */

static int	dwarf_max_al_pseudo_regno;
	/* Highest pseudo register that is the location of some decl.  This
	   is an optimization, indicating to dwarf_mark_set_rtx() that it can
	   avoid a scan of avail_locations[] because it will know a pseudo
	   register is not the home of any decl.
	 */

static rtx	dwarf_internal_arg_pointer;
	/* equal to current_function_internal_arg_pointer, if the current
	   function uses an argument block to pass registers.  Else NULL.
	 */

/* Define the in, out, gen and kill bit sets for each basic block.
   Each bit set is implemented as an array of unsigned long.
 */

static int dwarf_dfa_set_size_in_words;
	/* Number of words (ie, unsigned longs) in each dfa bit set */

static unsigned long **dwarf_in_sets;
static unsigned long **dwarf_out_sets;
static unsigned long **dwarf_gen_sets;
static unsigned long **dwarf_kill_sets;
	/* Indexed by basic block number b, dwarf_X_sets[b] points to
	   an array of words which is the X_set bit set for block b.
	 */

static unsigned long *static_in;
static unsigned long *static_gen;
static unsigned long *static_kill;
	/* These point to the current block's in, gen and kill sets during the
	   dataflow, so we can avoid passing them as parameters.
	 */

#define DWARF_SET_DFA_BIT(bitset, alid) \
	((bitset)[(alid) / (HOST_BITS_PER_CHAR * sizeof(unsigned long))] |= \
		1 << ((alid) % (HOST_BITS_PER_CHAR * sizeof(unsigned long))))

#define DWARF_CLEAR_DFA_BIT(bitset, alid) \
	((bitset)[(alid) / (HOST_BITS_PER_CHAR * sizeof(unsigned long))] &= \
		~(1 << ((alid) % (HOST_BITS_PER_CHAR * sizeof(unsigned long)))))

#define DWARF_IS_DFA_BIT_SET(bitset, alid) \
	((bitset)[(alid) / (HOST_BITS_PER_CHAR * sizeof(unsigned long))] & \
		(1 << ((alid) % (HOST_BITS_PER_CHAR * sizeof(unsigned long)))))

#define DWARF_IS_DFA_BIT_AVAILABLE(alid) \
	(DWARF_IS_DFA_BIT_SET(static_gen, alid) \
	|| (DWARF_IS_DFA_BIT_SET(static_in, alid) \
	    && !DWARF_IS_DFA_BIT_SET(static_kill, alid)) \
	)

static unsigned long *dwarf_alids_killed_by[FIRST_PSEUDO_REGISTER];
	/* Indexed by hard regno, each element is a bit set indicating
	   the alids to be killed when the register is updated.
	 */

static int	dwarf_generate_ranges;
	/* Boolean indicating to the dataflow transfer functions that they
	   should use the gen and kill information to generate valid ranges
	   for alids.
	 */

static void
dwarf_free_avail_locations()
{
	if (avail_locations)
	{
		int	i;
		for (i = 0; i <= dwarf_max_alid; i++)
			if (avail_locations[i].bounds)
				free((void*) avail_locations[i].bounds);

		free((void*) avail_locations);
		avail_locations = 0;
	}
}

/* Add space in avail_locations[i] for another range specification */

static void
dwarf_add_valid_range(alid)
int	alid;
{
	Dwarf_avail_location	*alidp = &avail_locations[alid];

	if (alidp->max_bound_index == alidp->bounds_size - 1)
	{
		alidp->bounds_size += 8; /* enough for 4 ranges */
		if (alidp->bounds == NULL)
			alidp->bounds = (rtx*) xmalloc(
				alidp->bounds_size * sizeof(int));
		else
			alidp->bounds = (rtx*) xrealloc(
				alidp->bounds,
				alidp->bounds_size * sizeof(int));
	}
	alidp->bounds[++alidp->max_bound_index] = NULL_RTX;
	alidp->bounds[++alidp->max_bound_index] = NULL_RTX;
}

/* Add a new <decl, location> pair to avail_locations[].  */

static void
dwarf_create_new_decl_location(decl, rtl, context)
tree	decl;
rtx	rtl;
tree	context;	/* BLOCK or FUNCTION_DECL containing this decl. */
{
	Dwarf_avail_location	*alidp;

	if (dwarf_max_alid == dwarf_alids_allocated - 1)
	{
		dwarf_alids_allocated += 64;
		avail_locations = (Dwarf_avail_location*) xrealloc(
					avail_locations,
					dwarf_alids_allocated
						* sizeof(Dwarf_avail_location));
	}

	dwarf_max_alid++;
	alidp = &avail_locations[dwarf_max_alid];

	alidp->decl = decl;
	alidp->rtl = rtl;
	alidp->kind = GDW_LE_OTHER;  /* Initialize after register allocation */

	/* block_begin_note will be initialized in a special pass over the
	   insns.
	 */
	alidp->depth = 0;
	alidp->block_begin_note = NULL_RTX;

	/* block_end_note can be found in the decl's DECL_CONTEXT.
	   PARM_DECL's and non-nested local VAR_DECLs are always in scope.
	   By leaving their begin and end notes NULL, we prevent the
	   dataflow from setting in_scope to 0.
	 */

	alidp->block_end_note = NULL_RTX;
	alidp->in_scope = 0;
	if (decl && context)
	{
		if (TREE_CODE(decl) == PARM_DECL
		    || TREE_CODE(context) == FUNCTION_DECL
		    || is_body_block(context))
			alidp->in_scope = 1;
		else if (TREE_CODE(context) == BLOCK)
			alidp->block_end_note = BLOCK_END_NOTE(context);
	}

	alidp->same_as_id = 0;
	alidp->bounds = NULL;
	alidp->max_bound_index = -1;
	alidp->bounds_size = 0;

	if (decl && !DECL_DWARF_FIRST_ALID(decl))
		DECL_DWARF_FIRST_ALID(decl) = dwarf_max_alid;

	if (rtl && GET_CODE(rtl) == REG
	    && REGNO(rtl) > dwarf_max_al_pseudo_regno)
		dwarf_max_al_pseudo_regno = REGNO(rtl);
}

static void
dwarf_create_new_decl_locations_for_block(block)
tree	block;
{
	tree	decl;

	if (!block)
		return;

	for (decl = BLOCK_VARS(block); decl; decl = TREE_CHAIN(decl))
	{
		if (TREE_CODE(decl) == VAR_DECL
		    && DECL_RTL(decl)
		    /* Don't create a location for a non-defining
		       local extern declaration, eg "{ extern int x; }".
		     */
		    && !DECL_EXTERNAL(decl))
		{
			dwarf_create_new_decl_location(decl, DECL_RTL(decl),
									block);

			/* Reserve an alid for a spill slot location.  If a
			   spill location is needed for this decl, this alid
			   can be found by searching for an avail_locations[]
			   entry with this decl and a null rtl.
			 */
			dwarf_create_new_decl_location(decl, NULL_RTX, block);
		}
	}

	for (decl = BLOCK_SUBBLOCKS(block); decl; decl = BLOCK_CHAIN(decl))
		dwarf_create_new_decl_locations_for_block(decl);
}

/* Set the DWARF_SET_ALID field of the given SET rtx to the alid of the
   "available location" it sets, if any.  Set it to -1 if it sets the
   pseudo register dedicated for current_function_internal_arg_pointer.
   If it doesn't set any known location, set it to zero.  (It should already
   be zero, since no one but us uses this field.  However, dataflow currently
   seems to be inadvertently setting this field to some erroneous value
   in some instances.)
 */

void
dwarf_mark_set_rtx(set_rtx)
rtx	set_rtx;
{
	rtx	x, dst = SET_DEST(set_rtx);
	int	alid;

	while (GET_CODE(dst) == SUBREG)
		dst = SUBREG_REG(dst);

	if (GET_CODE(dst) == REG)
	{
		int regno = REGNO(dst);
		if (regno < FIRST_PSEUDO_REGISTER
		    || regno > dwarf_max_al_pseudo_regno)
			/* No avail_locs are associated with this reg.
			   We assume a hard register is never updated on
			   behalf of a _DECL prior to register allocation.
			 */
			return;

		for (alid = 1; alid <= dwarf_max_alid; alid++)
		{
			if ((x = avail_locations[alid].rtl) != NULL_RTX
			    && GET_CODE(x) == REG
			    && REGNO(x) == regno)
			{
				DWARF_SET_ALID(set_rtx) = alid;
				return;
			}
		}

		/* If this set_rtx initialize's the internal arg pointer, mark
		   it with the special alid (-1).
		 */
		if (dwarf_internal_arg_pointer
		    && regno == REGNO(dwarf_internal_arg_pointer))
		{
			DWARF_SET_ALID(set_rtx) = -1;
			return;
		}
	}
	else if (GET_CODE(dst) == MEM)
	{
		for (alid = 1; alid <= dwarf_max_alid; alid++)
		{
			/* rtx_equal_p is probably an insufficient test here.
			   We will improve on this later.
			 */

			if (rtx_equal_p(avail_locations[alid].rtl, dst))
			{
				DWARF_SET_ALID(set_rtx) = alid;
				return;
			}
		}
	}

	DWARF_SET_ALID(set_rtx) = 0;
}

/* Called just before register allocation, this function marks each SET rtx
   that defines a parameter or local variable.  Such insns are used later
   in the available locations dataflow.

   This routine also assigns alid's to each <decl,location> pair, and
   allocates and initializes the 'avail_locations' array.
 */

void
dwarfout_prep_for_location_tracking(first)
rtx	first;
{
	tree	decl;
	tree	outer_scope;
	rtx	x, insn;
	int	depth;

	if (dwarf_dump_file)
	{
		(void) fprintf(dwarf_dump_file,"\n;; starting function %s\n",
			IDENTIFIER_POINTER(DECL_NAME(current_function_decl)));
		if (!optimize)
			print_rtl(dwarf_dump_file, first);
	}

	/* Determine if this function requires an argument block,
	   and if so find the pseudo register used for
	   current_function_internal_arg_pointer.
	   All the extra checks are because I don't trust that
	   current_function_internal_arg_pointer isn't being inherited
	   from the previous function decl.
	 */
	if (current_func_has_argblk()
	    && current_function_argptr_insn != NULL_RTX
	    && current_function_internal_arg_pointer != NULL_RTX
	    && arg_pointer_rtx != NULL_RTX
	    && GET_CODE(current_function_internal_arg_pointer) == REG
	    && REGNO(current_function_internal_arg_pointer)
				>= FIRST_PSEUDO_REGISTER)
		dwarf_internal_arg_pointer =
			current_function_internal_arg_pointer;
	else
		dwarf_internal_arg_pointer = NULL_RTX;

	/* Start out with 64 locations, grow if needed. */

	dwarf_free_avail_locations();
		/* Just in case this wasn't deleted for the last function.
		   I suspect this may happen if dwarfout_file_scope_decl
		   or dwarf_output_decl decide not to emit anything
		   for a just-compiled function.
		 */
	dwarf_alids_allocated = 64;
	avail_locations = (Dwarf_avail_location*)
		xmalloc(dwarf_alids_allocated * sizeof(Dwarf_avail_location));

	/* Make alid 0 unused. */
	dwarf_max_alid = -1;
	dwarf_create_new_decl_location((tree)NULL, NULL_RTX, (tree)NULL);

	dwarf_max_al_pseudo_regno = FIRST_PSEUDO_REGISTER - 1;

	decl = DECL_ARGUMENTS(current_function_decl);
	for (; decl; decl = TREE_CHAIN(decl))
	{
		if (TREE_CODE(decl) == PARM_DECL)
		{
			/* If this parameter is being passed in an arg block,
			   create another location which identifies its arg
			   block slot using arg_pointer_rtx.  Make sure this
			   new location is the first location for this decl
			   to be added via dwarf_create_new_decl_location.
			   Don't add this extra location for CAVE functions.
			 */

			x = DECL_INCOMING_RTL(decl);

			if (!TARGET_CAVE
			    && x && dwarf_internal_arg_pointer != NULL_RTX
			    && !rtx_equal_p(arg_pointer_rtx,
					dwarf_internal_arg_pointer)
			    && GET_CODE(x) == MEM
			    && ((GET_CODE(XEXP(x,0)) == REG
			        && REGNO(XEXP(x,0)) ==
					REGNO(dwarf_internal_arg_pointer))
			       ||
			        (GET_CODE(XEXP(x,0)) == PLUS
			         && GET_CODE(XEXP(XEXP(x,0),1)) == CONST_INT
			         && GET_CODE(XEXP(XEXP(x,0),0)) == REG
			         && REGNO(XEXP(XEXP(x,0),0)) ==
					REGNO(dwarf_internal_arg_pointer))))
			{
				rtx	y;
				if (GET_CODE(XEXP(x,0)) == REG)
				  y = gen_rtx(MEM,GET_MODE(x),arg_pointer_rtx);
				else
				  y = gen_rtx(MEM, GET_MODE(x),
					gen_rtx(PLUS, GET_MODE(XEXP(x,0)),
					  arg_pointer_rtx,
					  gen_rtx(CONST_INT, VOIDmode,
					    INTVAL(XEXP(XEXP(x,0),1)))));
				dwarf_create_new_decl_location(decl, y,
							DECL_CONTEXT(decl));
			}

			/* Add DECL_INCOMING_RTL */
			if (x)
				dwarf_create_new_decl_location(decl, x,
							DECL_CONTEXT(decl));

			if (!TARGET_CAVE)
			{
				if (DECL_RTL(decl) &&
				    !rtx_equal_p(x, DECL_RTL(decl)))
					dwarf_create_new_decl_location(decl,
							DECL_RTL(decl),
							DECL_CONTEXT(decl));

				/* Reserve an alid for a spill slot location. */
				dwarf_create_new_decl_location(decl, NULL_RTX,
							DECL_CONTEXT(decl));
			}
		}
	}

	outer_scope = DECL_INITIAL(current_function_decl);

	if (!TARGET_CAVE && outer_scope && TREE_CODE(outer_scope) == BLOCK)
		dwarf_create_new_decl_locations_for_block(
				BLOCK_SUBBLOCKS(outer_scope));

	/* That's all we need to do if optimization is disabled. */
	if (!optimize || TARGET_CAVE)
	{
		dwarf_debugging_dump("After alid creation");
		return;
	}

	/* Finish initializing each Dwarf_avail_location by finding the
	   block_begin_note that marks the start scope for each alid.
	 */
	depth = 0;
	for (insn = get_last_insn(); insn; insn = PREV_INSN(insn))
	{
		int	alid;
		if (GET_CODE(insn) != NOTE)
			continue;

		if (NOTE_LINE_NUMBER(insn) == NOTE_INSN_BLOCK_END)
		{
			depth++;
			for (alid = 1; alid <= dwarf_max_alid; alid++)
			{
				if (avail_locations[alid].block_end_note==insn)
					avail_locations[alid].depth = depth;
			}
		}
		else if (NOTE_LINE_NUMBER(insn) == NOTE_INSN_BLOCK_BEG)
		{
			for (alid = 1; alid <= dwarf_max_alid; alid++)
			{
				if (avail_locations[alid].depth == depth)
				{
					avail_locations[alid].depth = 0;
					avail_locations[alid].block_begin_note
									= insn;
				}
			}
			depth--;
		}
	}

	dwarf_debugging_dump("Before marking SET rtxs");

	/* Mark all SET rtx's with the alid of the location they set.
	   CLOBBER's never set a user-visible location to a value
	   meaningful to the user, so ignore them.
	 */

	for (insn = first; insn; insn = NEXT_INSN(insn))
	{
		if (GET_RTX_CLASS(GET_CODE(insn)) == 'i')
		{
			if (GET_CODE(PATTERN(insn)) == SET)
				dwarf_mark_set_rtx(PATTERN(insn));
			else if (GET_CODE(PATTERN(insn)) == PARALLEL)
			{
				int i = XVECLEN(PATTERN(insn), 0) - 1;

				for (; i >= 0; i--)
				{
					x = XVECEXP(PATTERN(insn),0,i);
					if (GET_CODE(x) == SET)
						dwarf_mark_set_rtx(x);
				}
			}
		}
	}

	if (dwarf_dump_file)
		print_rtl(dwarf_dump_file, first);
}

#define IS_ACTIVE_INSN_P(n) \
	( ! ( (GET_RTX_CLASS(GET_CODE(n)) != 'i') \
	       || (GET_CODE(n) == INSN \
	           && (GET_CODE(PATTERN(n)) == USE \
	               || GET_CODE(PATTERN(n)) == CLOBBER))))

/* Emit a NOTE_INSN_DWARF_LE_BOUNDARY note after the given insn.
   Re-use an existing one if already there.
   Return a pointer to this note insn.
   Return DWARF_FIRST_OR_LAST_BOUNDARY_NOTE if attempting to place a note
   after the function's last insn, since a label for that position will be
   emitted specially.
 */
static rtx
dwarf_emit_boundary_note_after(insn)
rtx	insn;
{
	rtx	next;

	if (!insn)
		return DWARF_FIRST_OR_LAST_BOUNDARY_NOTE;

	if (IS_ACTIVE_INSN_P(insn))
		next = NEXT_INSN(insn);
	else
		next = insn;

	/* Look for an existing boundary note. */

	while (next && !IS_ACTIVE_INSN_P(next))
	{
		if (GET_CODE(next) == NOTE
		    && NOTE_LINE_NUMBER(next) ==
					NOTE_INSN_DWARF_LE_BOUNDARY)
			return next;
		next = NEXT_INSN(next);
	}

	if (!next)
		return DWARF_FIRST_OR_LAST_BOUNDARY_NOTE;
	next = emit_note_after(NOTE_INSN_DWARF_LE_BOUNDARY, insn);
	NOTE_LE_BOUNDARY_NUMBER(next) = next_le_boundary_number++;
	return next;
}

/* For each hard register r in x, set first_def[r] to 'insn'
   if first_def[r] isn't already set to something.
 */

static void
dwarf_mark_reg_defs(x, insn, first_def)
rtx	x;
rtx	insn;
rtx	first_def[FIRST_PSEUDO_REGISTER];
{
	RTX_CODE	code = GET_CODE(x);
	int		regno, nregs = -1;

	if (code == SIGN_EXTRACT || code == ZERO_EXTRACT)
	{
		x = XEXP(x,0);
		code = GET_CODE(x);
	}

	if (code == STRICT_LOW_PART)
	{
		nregs = HARD_REGNO_NREGS(REGNO(XEXP(x,0)), GET_MODE(x));
		x = XEXP(x,0);
		code = GET_CODE(x);
	}
	else if (code == SUBREG)
	{
		x = SUBREG_REG(x);
		code = GET_CODE(x);
	}

	if (code == REG && (regno = REGNO(x)) < FIRST_PSEUDO_REGISTER)
	{
		if (nregs == -1)
			nregs = HARD_REGNO_NREGS(regno, GET_MODE(x));

		for (; nregs > 0; nregs--)
			if (first_def[regno + nregs - 1] == NULL_RTX)
				first_def[regno + nregs - 1] = insn;
	}
}

/* Find the boundaries of location expressions' valid ranges.
   If pragma section has been seen, we cannot emit any valid ranges.
   (The DWARF representation for valid ranges uses an offset from
   the beginning of the .text section.  There is no such offset when
   function bodies are emitted in, say, section .text.foo.)
   In this case, we pick one location for each object, and assume
   that location is valid over its entire lifetime.
 */

void
dwarfout_find_le_boundaries(first)
rtx	first;
{
	rtx	insn;
	tree	parm;
	int	i, alid;
	int	func_last_label_used = 0;

	for (alid = 1; alid <= dwarf_max_alid; alid++)
	{
		if (avail_locations[alid].rtl)
		{
			avail_locations[alid].rtl = eliminate_regs(
					avail_locations[alid].rtl, 0, NULL_RTX);
			avail_locations[alid].kind = dwarf_classify_location(
					avail_locations[alid].rtl);
		}

		/* I suspect that this is the time to initialize
		   same_as_id for each alid.
		 */
	}

	/* If compiling a CAVE function, we emit location information
	   only for the parameters of the stub function.  Each parameter
	   will have a single location (its DECL_INCOMING_RTL), which
	   is valid up until the stub calls the dispatcher.
	 */

	if (TARGET_CAVE)
	{
		parm = DECL_ARGUMENTS(current_function_decl);
		for (; parm; parm = TREE_CHAIN(parm))
		{
			/* Extract the first location of the parm (there
			   should be atmost one).
			 */

			if ((alid = DECL_DWARF_FIRST_ALID(parm)) != 0
			     && avail_locations[alid].rtl != NULL_RTX)
			{

				/* Make room for a single range.  As a special
				   case, when DWARF_FIRST_OR_LAST_BOUNDARY_NOTE
				   is used as the upper bound in a CAVE
				   function, it denotes the point of the call
				   to the dispatcher.  Normally, it denotes
				   the end of the function.
				 */
				dwarf_add_valid_range(alid);

				/* If pragma section is in effect, leave the
				   valid range as <NULL,NULL>, implying the
				   location is valid over its entire scope.
				 */
				if (!DWARF_PRAGMA_SECTION_SEEN)
				{
				  DWARF_CUR_RANGE_LOWER_BOUND(alid) =
					DWARF_FIRST_OR_LAST_BOUNDARY_NOTE;
				  DWARF_CUR_RANGE_UPPER_BOUND(alid) =
					DWARF_FIRST_OR_LAST_BOUNDARY_NOTE;
				}
			}
		}

		return;
	}

	/* If optimization is disabled, then we make several simplifying
	   assumptions so we don't have to run a data flow.

	   We assume each local variable has one location, which is valid
	   throughout its scope.

	   We assume each parameter has at most three locations: two from
	   its DECL_INCOMING_RTL and one from its DECL_RTL.  Generally,
	   the DECL_INCOMING_RTL will be one location.  But in the case
	   of a parameter being passed in an argument block, its exact
	   location in the parameter block may need to be specified in
	   two different ways.  The argument block is accessed via
	   arg_pointer_rtx at the very beginning of the function, and
	   later via current_function_internal_arg_pointer.

	   If all locations are the same, then we emit only a single location
	   expression which is valid throughout the function.

	   If the locations differ, then the incoming rtl locations are usually
	   valid up to the NOTE_INSN_FUNCTION_BEG note, and the other location
	   (DECL_RTL) is valid thereafter.  In some cases, however, a register
	   which is some parameter's incoming rtl location may be
	   overwritten before the function begin note.  For example, given

			int foo (int a, int b, register int c, .......)

	   where all parameters are passed in registers, c's home register
	   might be a's or b's incoming register.  So, for each hard register,
	   we need to find the first instruction prior to the function begin
	   note which updates the register.
	 */

	if (!optimize)
	{
		rtx		fbegin_insn = 0;
		rtx		pat;
		RTX_CODE	code;
		rtx		first_def[FIRST_PSEUDO_REGISTER];
		rtx		first_def_note[FIRST_PSEUDO_REGISTER];
		rtx		arg_ptr_note = 0;
		rtx		func_begin_note;
			/* For each hard register, first_def[r] is the first
			   prologue insn, if any, that updates the register.
			   first_def_note[r], if non-NULL, is the
			   NOTE_INSN_DWARF_LE_BOUNDARY note FOLLOWING the
			   the insn first_def[r].  arg_ptr_note, if non-zero,
			   is the NOTE_INSN_DWARF_LE_BOUNDARY note FOLLOWING
			   the insn which copies arg_pointer_rtx.
			   func_begin_note, if non-NULL, is the
			   NOTE_INSN_DWARF_LE_BOUNDARY note FOLLOWING
			   the NOTE_INSN_FUNCTION_BEGIN note.
			 */

		/* If pragma section has been seen, then we emit one location
		   expression for each parameter and local variable, with a
		   NULL valid range (ie, assume valid over its entire scope).
		 */
		if (!DWARF_PRAGMA_SECTION_SEEN)
		{
		  for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
		  {
			first_def[i] = NULL_RTX;
			first_def_note[i] = NULL_RTX;
		  }

		  /* Find the first_def for each hard register. */

		  for(insn = first; insn; insn = NEXT_INSN(insn))
		  {
		    if (GET_RTX_CLASS(GET_CODE(insn)) == 'i')
		    {
		      pat = PATTERN(insn);
		      code = GET_CODE(pat);

		      if (code == SET)
		      {
			/* Don't count a no-op move as an update */
			if (!rtx_equal_p(SET_SRC(pat), SET_DEST(pat)))
			  dwarf_mark_reg_defs(SET_DEST(pat), insn, first_def);
		      }
		      else if (code == CLOBBER)
			dwarf_mark_reg_defs(XEXP(pat,0), insn, first_def);
		      else if (code == PARALLEL)
		      {
			for (i = XVECLEN(pat,0) - 1; i >= 0; i--)
			{
			  code = GET_CODE(XVECEXP(pat,0,i));
			  if (code == SET)
			  {
			    if (!rtx_equal_p(SET_SRC(XVECEXP(pat,0,i)),
					     SET_DEST(XVECEXP(pat,0,i))))
			      dwarf_mark_reg_defs(SET_DEST(XVECEXP(pat,0,i)),
							insn, first_def);
			  }
			  else if (code == CLOBBER)
			    dwarf_mark_reg_defs(XEXP(XVECEXP(pat,0,i),0),
							insn, first_def);
			}
		      }
		    }
		    else if (GET_CODE(insn) == NOTE
			   && NOTE_LINE_NUMBER(insn) == NOTE_INSN_FUNCTION_BEG)
		    {
			fbegin_insn = insn;
			break;
		    }
		  }

		  if (!fbegin_insn)
			abort();

		  /* Create a boundary note after the prologue.  */
		  func_begin_note = dwarf_emit_boundary_note_after(fbegin_insn);
		}

		/* For each parameter, initialize the bounds for each rtl
		   in each of its alids.
		 */

		parm = DECL_ARGUMENTS(current_function_decl);
		for (; parm; parm = TREE_CHAIN(parm))
		{
		  rtx	loc0, loc1 = NULL_RTX, loc2 = NULL_RTX;
		  int	loc0_alid, loc1_alid, loc2_alid;

		  /* Extract the locations of parm.  There are at most
		     three locations, two for the incoming rtl, and one
		     for decl rtl.  There may be less than three.
		   */

		  if ((loc0_alid = DECL_DWARF_FIRST_ALID(parm)) == 0
		      || (loc0 = avail_locations[loc0_alid].rtl) == NULL_RTX)
		    continue;

		  /* If there's only one location, assume it is valid for
		     the entire function.  Leave it's bounds[0..1] as <0,0>.
		   */
		  if ((loc1_alid = loc0_alid + 1) > dwarf_max_alid
		      || avail_locations[loc0_alid].decl !=
					avail_locations[loc1_alid].decl
		      || (loc1 = avail_locations[loc1_alid].rtl) == NULL_RTX)
		  {
		    dwarf_add_valid_range(loc0_alid);
		    continue;
		  }

		  if ((loc2_alid = loc1_alid + 1) <= dwarf_max_alid
		      && avail_locations[loc0_alid].decl ==
					avail_locations[loc2_alid].decl)
		  {
		    if ((loc2 = avail_locations[loc2_alid].rtl) != NULL)
		      if (rtx_equal_p(loc1, loc2))
		        /* Identical locations should have been removed prior
		           to register allocation.
		         */
		        abort();
		  }

		  if (loc2_alid + 1 <= dwarf_max_alid
		      && avail_locations[loc2_alid+1].decl ==
					avail_locations[loc2_alid].decl
		      && avail_locations[loc2_alid+1].rtl != NULL_RTX)
		    abort();	/* Can't be more than three locs at -O0 */

		  /* Now set the valid range for each loc.
		     The valid ranges for the various locs are contiguous,
		     and cover the entire function.
		     Find and mark the points where one range ends and the
		     next begins.
		   */
		  if (!DWARF_PRAGMA_SECTION_SEEN)
		  {
		    /* Make room for one valid range for each location */
		    dwarf_add_valid_range(loc0_alid);
		    dwarf_add_valid_range(loc1_alid);
		    if (loc2)
		      dwarf_add_valid_range(loc2_alid);

		    /* First location's valid range begins at function entry. */
		    avail_locations[loc0_alid].bounds[0] =
					DWARF_FIRST_OR_LAST_BOUNDARY_NOTE;

		    /* We must reference this function's last boundary label*/
		    func_last_label_used = 1;
		  }

		  if (DWARF_PRAGMA_SECTION_SEEN)
		  {
		    /* Pick one location and assume it is valid over
		       the entire function.  The best choice here is
		       the DECL_RTL, which is either loc2 or loc1.
		     */
		    if (loc2)
		      dwarf_add_valid_range(loc2_alid);
		    else
		      dwarf_add_valid_range(loc1_alid);
		  }
		  /* If the second location references an argument block, then
		     it becomes valid when dwarf_internal_arg_pointer is first
		     defined.
		   */
		  else if (dwarf_internal_arg_pointer

		      /* This should always be true when !optimize */
		      && REGNO(dwarf_internal_arg_pointer)
			 < FIRST_PSEUDO_REGISTER

		      && first_def[REGNO(dwarf_internal_arg_pointer)]
		      && first_def[REGNO(dwarf_internal_arg_pointer)]
					== current_function_argptr_insn
		      && GET_CODE(loc1) == MEM
		      && ((GET_CODE(XEXP(loc1,0)) == REG
			    && REGNO(XEXP(loc1,0)) ==
			       REGNO(dwarf_internal_arg_pointer))
		          ||
			  (GET_CODE(XEXP(loc1,0)) == PLUS
			    && GET_CODE(XEXP(XEXP(loc1,0),1)) == CONST_INT
			    && GET_CODE(XEXP(XEXP(loc1,0),0)) == REG
			    && REGNO(XEXP(XEXP(loc1,0),0)) ==
			       REGNO(dwarf_internal_arg_pointer))))
		  {
		    /* Add a label after the insn that defines
		       dwarf_internal_arg_pointer
		     */

		    if (arg_ptr_note == NULL_RTX)
		      arg_ptr_note = dwarf_emit_boundary_note_after(
			      first_def[REGNO(dwarf_internal_arg_pointer)]);

		    avail_locations[loc0_alid].bounds[1] = arg_ptr_note;
		    avail_locations[loc1_alid].bounds[0] = arg_ptr_note;

		    /* If there are three locations, then the range for the
		       third one starts at the function begin note and is
		       valid for the remainder of the function.
		       Otherwise, loc2 is valid for the remainder.
		     */
		    if (loc2)
		    {
		      avail_locations[loc1_alid].bounds[1] = func_begin_note;
		      avail_locations[loc2_alid].bounds[0] = func_begin_note;
		      avail_locations[loc2_alid].bounds[1] =
					DWARF_FIRST_OR_LAST_BOUNDARY_NOTE;
		    }
		    else
		      avail_locations[loc1_alid].bounds[1] =
					DWARF_FIRST_OR_LAST_BOUNDARY_NOTE;
		  }
		  else
		  {
		    int regno, nregs;

		    if (loc2)
		      /* At most two locs if an argument block isn't used.  */
		      abort();

		    /* Find the point at which loc0 ends and loc1 becomes
		       valid.  This point is generally the function begin
		       note, unless loc0's location is a register that has
		       been updated in the prologue before the function
		       begin note.  We're only concerned about such an
		       update if loc0 is a register, we assume a memory
		       home is not arbitrarily updated in the prologue.
		     */

		    code = GET_CODE(loc0);
		    while (code == SUBREG || code == ZERO_EXTRACT
		         || code == SIGN_EXTRACT || code == STRICT_LOW_PART)
		    {
		      loc0 = XEXP(loc0,0);
		      code = GET_CODE(loc0);
		    }

		    if (code == REG
			&& (regno = REGNO(loc0)) < FIRST_PSEUDO_REGISTER)
		    {
		      nregs = HARD_REGNO_NREGS(regno, GET_MODE(loc0));
		      for (; nregs > 0; nregs--)
		      {
		        if (first_def[regno+nregs-1] != NULL_RTX)
		        {
			  if (!first_def_note[regno+nregs-1])
			  {
			    first_def_note[regno+nregs-1] =
					dwarf_emit_boundary_note_after(
						first_def[regno+nregs-1]);
			  }
			  avail_locations[loc0_alid].bounds[1] =
					  first_def_note[regno+nregs-1];
			  break;
		        }
		      }
		    }

		    if (avail_locations[loc0_alid].bounds[1] == NULL_RTX)
		      avail_locations[loc0_alid].bounds[1] = func_begin_note;

		    avail_locations[loc1_alid].bounds[0] =
					avail_locations[loc0_alid].bounds[1];

		    avail_locations[loc1_alid].bounds[1] =
					DWARF_FIRST_OR_LAST_BOUNDARY_NOTE;
		  }
		}

		/* Now create a single valid range for each local var decl. */

		for (alid = 1; alid <= dwarf_max_alid; alid++)
		{
			if (avail_locations[alid].kind != GDW_LE_OTHER
			    && TREE_CODE(avail_locations[alid].decl) ==VAR_DECL)
			{
				dwarf_add_valid_range(alid);
				/* Leave the bounds as NULL_RTX, indicating
				   the location is valid over the decl's
				   entire scope.
				 */
			}
		}
	}
	else
	{
		/* Handle the case where optimization is enabled */

		dwarf_avail_locations_analysis(first);
	}

	if (!DWARF_PRAGMA_SECTION_SEEN && (optimize || func_last_label_used))
	{
		/* Put a boundary label at the end of the function .*/
		insn = emit_note_after(NOTE_INSN_DWARF_LE_BOUNDARY,
						get_last_insn());
		NOTE_LE_BOUNDARY_NUMBER(insn) =
					func_last_le_boundary_number;
	}
}

/* Initialize dwarf_alids_killed_by[regno] for each REG in 'rtl'.  */

static void
dwarf_set_alid_for_regs_used(decl, alid, rtl)
tree	decl;
int	alid;
rtx	rtl;
{
	char	*fmt;
	int	i, r, regno, nregs;
	RTX_CODE code;

	if (rtl == NULL_RTX)
		return;

	/* I'm not sure if the rtl here will ever contain subregs. 
	   If it does, ignore it.  We can refine this later if needed.
	 */
	while ((code = GET_CODE(rtl)) == SUBREG)
		rtl = SUBREG_REG(rtl);

	if (code == REG)
	{
		if ((regno = REGNO(rtl)) < FIRST_PSEUDO_REGISTER)
		{
			nregs = HARD_REGNO_NREGS(regno, GET_MODE(rtl));

			if (nregs == 0 && GET_MODE(rtl) == BLKmode)
				nregs = int_size_in_bytes(TREE_TYPE(decl))
					/ UNITS_PER_WORD;

			for (; nregs > 0; nregs--)
			{
			  r = regno + nregs - 1;
			  DWARF_SET_DFA_BIT(dwarf_alids_killed_by[r],alid);
			}
		}

		return;
	}

	fmt = GET_RTX_FORMAT(code);
	for (i = GET_RTX_LENGTH(code) - 1; i >= 0; i--)
	{
		if (fmt[i] == 'e')
			dwarf_set_alid_for_regs_used(decl, alid, XEXP(rtl,i));
		else if (fmt[i] == 'E')
		{
			int j;
			for (j = XVECLEN(rtl, i) - 1; j >= 0; j--)
				dwarf_set_alid_for_regs_used(decl, alid,
							XVECEXP(rtl,i,j));
		}
	}
}

/* The following several functions peform the avail locations dataflow
   transfer equation on 'insn' (ie they update gen and kill based on the
   updates in the insn).  We use the following rules to determine when a
   given alid is gen'd or killed.

   An alid representing a register location R for a decl D is gen'd:

     o Implicitly at function entry if D is a PARM_DECL and R is D's
       incoming rtl.

     o After an insn that updates R on behalf of D.

   An alid representing a register location R for a decl D is killed:

     o After an insn that updates R, but not on behalf of D.

     o After an insn that assigns some other location on behalf of D.
       For a given decl, we assume only one location is ever live.  When we
       see an update to any one of D's locations, done on behalf of D, then
       that location becomes gen'd and all of D's other locations get killed.

     o When D's scope ends.

   An alid representing a memory location M for a decl D is gen'd:

     o Implicitly at function entry if D is a PARM_DECL and M is D's incoming
       rtl.

     o Implicitly at the beginning of D's scope if no other location for D
       is already available at that point.

     o After the insn that copies arg_pointer_rtx into
       current_function_internal_arg_pointer, in the case that D is a
       PARM_DECL and M is D's argument block location, and M is specified in
       terms of current_function_internal_arg_pointer.  This handles the case
       on the 80960 where a parameter being passed in an argument block has
       an incoming location of the form 'disp(g14)', and then a location of
       the form, say, 'disp(g4)', where g4 is a copy of the original value
       of g14.

     o After an insn that updates M on behalf of D.

   An alid representing a memory location M for a decl D is killed:

     o Whenever a register used to formulate M's address is updated.

     o After an insn that assigns some other location on behalf of D.

     o When D's scope ends.

   An alid representing a constant value for a decl D is gen'd:

     o Implicitly at the beginning of D's scope if no other location for D
       is already available at that point.

   An alid representing a constant value for a decl D is killed:

     o After an insn that assigns some other location on behalf of D.

     o When D's scope ends.

   The quality of this information with regards to memory locations depends
   on how well we can detect if a given MEM update in the insn stream
   overlaps or is identical to a MEM location for some DECL.  Since the
   MEMs of interest are either stack locations or static locals,
   we should be able to do a pretty good job.
   The quality also depends on maintaining DWARF_SET_ALID between its
   initial assignment and final().
 */

/* Gen the given alid and kill all other alids for the same decl. */

static void
dwarf_dfa_transfer_gen_single_loc(alid, insn)
int	alid;
rtx	insn;		/* insn that is gen'ing this alid. */
{
	int	k;
	tree	decl;

	/* Kill all alids for this alid's decl */
	decl = avail_locations[alid].decl;
	for (k = alid-1; k > 0; k--)
	{
		if (avail_locations[k].decl == decl)
		{
			DWARF_CLEAR_DFA_BIT(static_gen, k);
			DWARF_SET_DFA_BIT(static_kill, k);

			if (dwarf_generate_ranges && DWARF_IS_A_RANGE_ACTIVE(k))
				DWARF_CUR_RANGE_UPPER_BOUND(k) = insn;
		}
		else
			break;
	}

	for (k = alid+1; k <= dwarf_max_alid; k++)
	{
		if (avail_locations[k].decl == decl
		    && avail_locations[k].rtl)
		{
			DWARF_CLEAR_DFA_BIT(static_gen, k);
			DWARF_SET_DFA_BIT(static_kill, k);

			if (dwarf_generate_ranges && DWARF_IS_A_RANGE_ACTIVE(k))
				DWARF_CUR_RANGE_UPPER_BOUND(k) = insn;
		}
		else
			break;
	}

	if (dwarf_generate_ranges && avail_locations[alid].in_scope
	    && !DWARF_IS_A_RANGE_ACTIVE(alid))
	{
		dwarf_add_valid_range(alid);
		DWARF_CUR_RANGE_LOWER_BOUND(alid) = insn;
	}

	DWARF_CLEAR_DFA_BIT(static_kill, alid);
	DWARF_SET_DFA_BIT(static_gen, alid);
}

/* Update gen and kill based on set_rtx, which is either a SET or CLOBBER */

static void
dwarf_dfa_transfer_set_or_clobber(set_rtx, insn)
rtx	set_rtx;
rtx	insn;		/* insn that contains set_rtx */
{
	int	alid, set_rtx_alid, w, r, nregs, regno;
	rtx	dst;

	if (GET_CODE(set_rtx) == CLOBBER)
	{
	  /* Arbitrary clobbers of MEMs don't invalidate any locations.
	     Only register clobbers will invalidate a location.
	     Clobbers never gen anything.
	   */

	  dst = XEXP(set_rtx,0);
	  if (GET_CODE(dst) == REG
	      && (regno = REGNO(dst)) < FIRST_PSEUDO_REGISTER)
	  {
	    nregs = HARD_REGNO_NREGS(regno, GET_MODE(dst));

	    for (; nregs > 0; nregs--)
	    {
	      r = regno + nregs - 1;

	      for (w=0; w < dwarf_dfa_set_size_in_words; w++)
	      {
		unsigned long	word = dwarf_alids_killed_by[r][w];

		static_gen[w] &= ~word;
		static_kill[w] |= word;

		if (dwarf_generate_ranges)
		{
		  alid = w * HOST_BITS_PER_CHAR * sizeof(unsigned long);
		  while (word)
		  {
		    if ((word & 1) && DWARF_IS_A_RANGE_ACTIVE(alid))
		      DWARF_CUR_RANGE_UPPER_BOUND(alid) = insn;
		    word >>= 1;
		    alid++;
		  }
		}
	      }
	    }
	  }

	  return;
	}

	if (GET_CODE(set_rtx) != SET)
		return;

	dst = SET_DEST(set_rtx);
	set_rtx_alid = DWARF_SET_ALID(set_rtx);

	/* Now check for a register update.
	   Handle STRICT_LOW_PART and SUBREG.
	 */

	regno = -1;

	if (GET_CODE(dst) == STRICT_LOW_PART)
	{
		int	strict_lowpart_mode;

		dst = XEXP(dst,0);
		assert(GET_CODE(dst) == SUBREG);
		strict_lowpart_mode = GET_MODE(dst);
		dst = SUBREG_REG(dst);
		assert(GET_CODE(dst) == REG);
		regno = REGNO(dst);
		if (regno >= FIRST_PSEUDO_REGISTER)
			abort();

		nregs = HARD_REGNO_NREGS(regno, strict_lowpart_mode);
	}
	else
	{
		while (GET_CODE(dst) == SUBREG)
			dst = SUBREG_REG(dst);
		if (GET_CODE(dst) == REG)
		{
			regno = REGNO(dst);
			nregs = HARD_REGNO_NREGS(regno, GET_MODE(dst));
		}
	}

	/* Are we updating a register? */

	if (regno > -1)
	{
	  /* We've seen an update to 'nregs's registers starting
	     at register 'regno'.
	   */

	  for (; nregs > 0; nregs--)
	  {
	    r = regno + nregs - 1;

	    for (w=0; w < dwarf_dfa_set_size_in_words; w++)
	    {
	      unsigned long	word = dwarf_alids_killed_by[r][w];

	      static_gen[w] &= ~word;
	      static_kill[w] |= word;

	      if (dwarf_generate_ranges)
	      {
		alid = w * HOST_BITS_PER_CHAR * sizeof(unsigned long);
		while (word)
		{
		  if ((word & 1) && DWARF_IS_A_RANGE_ACTIVE(alid))
		    DWARF_CUR_RANGE_UPPER_BOUND(alid) = insn;
		  word >>= 1;
		  alid++;
		}
	      }
	    }
	  }
	}
#if 0
	else if (GET_CODE(dst) == MEM)
	{
		/* A MEM update kills and gens nothing if not done on behalf
		   of some decl.  Otherwise, it kills all other locations for
		   the decl and gens itself.  This is exactly what is
		   implemented below.
		 */
	}
#endif

	if (set_rtx_alid > 0 && set_rtx_alid <= dwarf_max_alid)
		dwarf_dfa_transfer_gen_single_loc(set_rtx_alid, insn);
	else if (set_rtx_alid == -1 && arg_pointer_rtx
		 && GET_CODE(arg_pointer_rtx) == REG
		 && (regno = REGNO(arg_pointer_rtx)) < FIRST_PSEUDO_REGISTER)
	{
		/* Kill any DECL_INCOMING_RTL locations that are in
		   an argument block, and gen those locations that
		   use current_function_internal_arg_pointer to specify
		   their argument block location.
		 */

		int	local_argptr_regno =
				REGNO(current_function_internal_arg_pointer);
		for (w=0; w < dwarf_dfa_set_size_in_words; w++)
		{
			unsigned long	word = dwarf_alids_killed_by[regno][w];

			static_gen[w] &= ~word;
			static_kill[w] |= word;

			if (dwarf_generate_ranges)
			{
			  alid = w * HOST_BITS_PER_CHAR * sizeof(unsigned long);
			  while (word)
			  {
			    if ((word & 1) && DWARF_IS_A_RANGE_ACTIVE(alid))
				DWARF_CUR_RANGE_UPPER_BOUND(alid) = insn;
			    word >>= 1;
			    alid++;
			  }
			}
		}

		for (alid = 1; alid <= dwarf_max_alid; alid++)
		{
			rtx	rtl = avail_locations[alid].rtl;

			if (rtl
			    && TREE_CODE(avail_locations[alid].decl)==PARM_DECL
			    && GET_CODE(rtl) == MEM
			    && (
				(GET_CODE(XEXP(rtl,0)) == REG
				 && REGNO(XEXP(rtl,0)) == local_argptr_regno)
			       ||
				(GET_CODE(XEXP(rtl,0)) == PLUS
				 && GET_CODE(XEXP(XEXP(rtl,0),1)) == CONST_INT
				 && GET_CODE(XEXP(XEXP(rtl,0),0)) == REG
				 && REGNO(XEXP(XEXP(rtl,0),0)) ==
							local_argptr_regno)
			       )
			   )
			{
				dwarf_dfa_transfer_gen_single_loc(alid, insn);
			}
		}
	}
}

static void
dwarf_dfa_transfer(insn)
rtx	insn;
{
	int	alid, r, i, w;

	if (GET_RTX_CLASS(GET_CODE(insn)) == 'i')
	{
	  rtx	pat = PATTERN(insn);

	  if (GET_CODE(insn) == CALL_INSN)
	  {
	    /* Kill any locations depending on regs in call_used_regs[].
	       Explicit clobbers within the pattern will be handled below,
	       perhaps redundantly but that's okay.
	       If generating ranges, first end the live ranges
	       of anything that is getting clobbered.
	     */

	    for (r = 0; r < FIRST_PSEUDO_REGISTER; r++)
	    {
	      /* Implicit updates to the stack pointer and frame pointer
	         because of a function call must not kill any locations
	         on the caller's frame.
	       */

	      if (call_used_regs[r] != 0
	          && r != FRAME_POINTER_REGNUM && r != STACK_POINTER_REGNUM)
	      {
	        for (w=0; w < dwarf_dfa_set_size_in_words; w++)
	        {
		  unsigned long word = dwarf_alids_killed_by[r][w];

		  static_gen[w] &= ~word;
		  static_kill[w] |= word;

		  if (dwarf_generate_ranges)
		  {
		    alid = w * HOST_BITS_PER_CHAR * sizeof(unsigned long);
		    while (word)
		    {
		      if ((word & 1) && DWARF_IS_A_RANGE_ACTIVE(alid))
		        DWARF_CUR_RANGE_UPPER_BOUND(alid) = insn;
		      word >>= 1;
		      alid++;
		    }
		  }
	        }
	      }
	    }
	  }

	  if (GET_CODE(pat) == CLOBBER || GET_CODE(pat) == SET)
		dwarf_dfa_transfer_set_or_clobber(pat, insn);
	  else if (GET_CODE(pat) == PARALLEL)
	  {
		for (i = XVECLEN(pat,0) - 1; i >= 0; i--)
		{
			rtx x = XVECEXP(pat,0,i);
			if (GET_CODE(x) == CLOBBER || GET_CODE(x)==SET)
				dwarf_dfa_transfer_set_or_clobber(x, insn);
		}
	  }
	}
	else if (GET_CODE(insn) == NOTE)
	{
	  if (NOTE_LINE_NUMBER(insn) == NOTE_INSN_BLOCK_BEG)
	  {
	    /* Set the in_scope bit for alids in this scope, and start
	       a valid range for any such alid that is already available
	       at this point.  If none are available, gen any MEM or
	       const-value locations for these decls.  There should only
	       ever be one const-value or MEM location for a decl, so we
	       don't need to worry about gen'ing multiple alids for
	       a given decl simultaneously.
	       Also, record the beginning of the valid range for
	       all decls in this scope that are "live" at this point.
	     */

	    for (alid = 1; alid <= dwarf_max_alid; )
	    {
	      if (avail_locations[alid].block_begin_note == insn)
	      {
		int	start_alid = alid;
		int	gend_alid = -1;
		tree	decl = avail_locations[alid].decl;

		/* Set the in_scope flag for all alids of this decl.
		   If any alid is already available for this decl,
		   start a valid range for the decl.
		 */

		do
		{
		  avail_locations[alid].in_scope = 1;
		  if (DWARF_IS_DFA_BIT_AVAILABLE(alid))
		  {
		    /* Only one alid for the decl can be available at
		       any given time.
		      */
		    assert(gend_alid == -1);
		    gend_alid = alid;
		    if (dwarf_generate_ranges)
		    {
			dwarf_add_valid_range(alid);
			DWARF_CUR_RANGE_LOWER_BOUND(alid) = insn;
		    }
		  }
		  alid++;
		} while (alid <= dwarf_max_alid
		  && avail_locations[alid].decl == decl);

		/* If nothing was available, gen any MEM or const value
		   alid for this decl.
		 */
		if (gend_alid == -1)
		{
		  for (gend_alid = start_alid; gend_alid < alid; gend_alid++)
		  {
		    if (avail_locations[gend_alid].kind == GDW_LE_CONST_VALUE
			||
			(avail_locations[gend_alid].kind == GDW_LE_LOCATION
			 && GET_CODE(avail_locations[gend_alid].rtl) == MEM))
		    {
		      DWARF_CLEAR_DFA_BIT(static_kill, gend_alid);
		      DWARF_SET_DFA_BIT(static_gen, gend_alid);
		      if (dwarf_generate_ranges)
		      {
			dwarf_add_valid_range(gend_alid);
			DWARF_CUR_RANGE_LOWER_BOUND(gend_alid) = insn;
		      }
		      break;	/* Since only one can be gen'd, we're done. */
		    }
		  }
		}
	      }
	      else
		alid++;
	    }
	  }
	  else if (NOTE_LINE_NUMBER(insn) == NOTE_INSN_BLOCK_END)
	  {
	    /* Kill all decls that belong to this scope,
	       and mark them as not in scope.
	       We probably don't need to kill anything in the pass
	       that isn't generating ranges, but it doesn't hurt much
	       performance-wise and provides consistency between the
	       two passes.
	     */

	    for (alid = 1; alid <= dwarf_max_alid; alid++)
	    {
	      if (avail_locations[alid].block_end_note == insn)
	      {
		avail_locations[alid].in_scope = 0;
		if (avail_locations[alid].kind != GDW_LE_OTHER)
		{
		  DWARF_SET_DFA_BIT(static_kill, alid);
		  DWARF_CLEAR_DFA_BIT(static_gen, alid);
		  if (dwarf_generate_ranges && DWARF_IS_A_RANGE_ACTIVE(alid))
		    DWARF_CUR_RANGE_UPPER_BOUND(alid) = insn;
		}
	      }
	    }
	  }
	}
}

/* Determine the locations (registers and stack slots) and their associated
   valid ranges for the current function's parameters and local variables.
 */

static void
dwarf_avail_locations_analysis(first_insn)
rtx	first_insn;
{
	tree	decl, outer_scope;
	int	set_size_in_bytes;	/* size of a single dfa bit set */
	int	alid, i, changes;
	int	b;	/* basic block number */
	int	w;	/* bit set word index */

	unsigned long	*implicit_in;
		/* This bit set holds the alids that are gen'd implicitly
		   at function entry.  It is used in computing the first
		   block's in set.
		 */

	/* Allocate the dfa bit sets.  Round up the size of each bit set
	   to a multiple of the word size (sizeof unsigned long).
	 */

	/* First we need to know how many blocks there are, and all the
	   predecessors of each block.  We need to tweak the basic block
	   boundaries because we want block_begin and block_end notes to
	   participate in our dataflow, and the basic block data structures
	   sometimes exclude note insns.  Because no one but us is, or will
	   be, using basic_block_head and basic_block_end, we update these
	   so that all insns in the insn stream are in some basic block.
	   This means that for all blocks b except the first one,
	   basic_block_head[b] == NEXT_INSN(basic_block_end[b-1]).
	   The first basic block starts at the function's first insn,
	   and the last basic block ends at get_last_insn().
	   When we create a NOTE_INSN_DWARF_LE_BOUNDARY note, we don't need
	   to bother ensuring that this note belongs to some basic block.
	 */
	get_predecessors(0);
	basic_block_head[0] = first_insn;
	for (b = 1; b < n_basic_blocks; b++)
		basic_block_head[b] = NEXT_INSN(basic_block_end[b-1]);
	basic_block_end[n_basic_blocks - 1] = get_last_insn();

	/* Now allocate the arrays of pointers to the bit sets. */

	dwarf_in_sets = (unsigned long**)
			xmalloc(4*n_basic_blocks*sizeof(unsigned long*));
	dwarf_out_sets  = dwarf_in_sets +   n_basic_blocks;
	dwarf_gen_sets  = dwarf_in_sets + 2*n_basic_blocks;
	dwarf_kill_sets = dwarf_in_sets + 3*n_basic_blocks;

	/* Now allocate the bit sets (arrays of unsigned long). */

	dwarf_dfa_set_size_in_words =
		(((dwarf_max_alid+1) +
		   (HOST_BITS_PER_CHAR * sizeof(unsigned long) - 1))
		 / (HOST_BITS_PER_CHAR * sizeof(unsigned long)));

	set_size_in_bytes = dwarf_dfa_set_size_in_words * sizeof(unsigned long);

	implicit_in = (unsigned long*) xmalloc(set_size_in_bytes);
	(void) memset((void*) implicit_in, 0, set_size_in_bytes);

	i = set_size_in_bytes * n_basic_blocks;
	dwarf_in_sets[0] = (unsigned long*) xmalloc(i);
	dwarf_out_sets[0] = (unsigned long*) xmalloc(i);
	dwarf_gen_sets[0] = (unsigned long*) xmalloc(i);
	dwarf_kill_sets[0] = (unsigned long*) xmalloc(i);
	(void) memset((void*) dwarf_in_sets[0], 0, i);
	(void) memset((void*) dwarf_gen_sets[0], 0, i);
	(void) memset((void*) dwarf_kill_sets[0], 0, i);
	/* dwarf_out_sets is initialized below */

	for (b = 1; b < n_basic_blocks; b++)
	{
	  int	start_offset = b * dwarf_dfa_set_size_in_words;

	  dwarf_in_sets[b]   = dwarf_in_sets[0]   + start_offset;
	  dwarf_out_sets[b]  = dwarf_out_sets[0]  + start_offset;
	  dwarf_gen_sets[b]  = dwarf_gen_sets[0]  + start_offset;
	  dwarf_kill_sets[b] = dwarf_kill_sets[0] + start_offset;
	}

	/* To speed up the transfer function we precompute, for each hard reg,
	   the alids which an update to that reg should kill.
	   Store these alids as an array of bit sets, indexed by hard regno.
	 */

	i = set_size_in_bytes * FIRST_PSEUDO_REGISTER;
	dwarf_alids_killed_by[0] = (unsigned long*) xmalloc(i);
	(void) memset((void*) dwarf_alids_killed_by[0], 0, i);
	for (i = 1; i < FIRST_PSEUDO_REGISTER; i++)
		dwarf_alids_killed_by[i] = dwarf_alids_killed_by[0]
					+ i * dwarf_dfa_set_size_in_words;

	for (alid = 1; alid <= dwarf_max_alid; alid++)
		dwarf_set_alid_for_regs_used(avail_locations[alid].decl,
				alid, avail_locations[alid].rtl);

	/* Initialize the gen and kill sets for each block */

	for (b = 0; b < n_basic_blocks; b++)
	{
		rtx	insn = basic_block_head[b];
		rtx	lim = NULL_RTX;

		if (b != n_basic_blocks - 1)
			lim = basic_block_head[b+1];

		/* Make the in, gen and kill sets available to the
		   transfer functions.  The in sets are empty at this point.
		 */
		static_in = dwarf_in_sets[b];
		static_gen = dwarf_gen_sets[b];
		static_kill = dwarf_kill_sets[b];

		while (insn != lim)
		{
			dwarf_dfa_transfer(insn);
			insn = NEXT_INSN(insn);
		}
	}

	/* Initialize the first block's in and out sets specially.
	   The in set essentially consists of the DECL_INCOMING_RTL for all
	   parameters, and the DECL_RTL of MEM-based or constant non-nested
	   local variables.
	   Note that the DECL_RTL for register-based locals is not put in the
	   first block's in set.  These locations will be gen'd when an
	   assignment to them is seen.  (Thus we are making the assumption
	   that a write to a MEM location which is some decl's home, is
	   always done on behalf of that decl, but a write to a register
	   which is some decl's home, may not be done on behalf of that decl.)
	   Go ahead and allocate a valid-range for these objects.  The start
	   of the valid range is marked with a label containing the number
	   func_first_le_boundary_number.  That label is emitted when
	   assemble_start_function() calls dwarfout_begin_function().
	   Since there is no insn in the insn stream to represent this
	   label, we use the special value DWARF_FIRST_OR_LAST_BOUNDARY_NOTE
	   to indicate it.
	 */

	decl = DECL_ARGUMENTS(current_function_decl);
	for (; decl; decl = TREE_CHAIN(decl))
	{
		if (TREE_CODE(decl) == PARM_DECL
		    && (alid = DECL_DWARF_FIRST_ALID(decl)) != 0
		    && avail_locations[alid].kind != GDW_LE_OTHER)
		{
			DWARF_SET_DFA_BIT(implicit_in, alid);
		}
	}

	outer_scope = DECL_INITIAL(current_function_decl);

	if (outer_scope
	    && TREE_CODE(outer_scope) == BLOCK
	    && BLOCK_SUBBLOCKS(outer_scope))
	{
		decl = BLOCK_VARS(BLOCK_SUBBLOCKS(outer_scope));
		for (; decl; decl = TREE_CHAIN(decl))
		{
			if (TREE_CODE(decl) == VAR_DECL
			    && (alid = DECL_DWARF_FIRST_ALID(decl)) != 0
			    && (avail_locations[alid].kind == GDW_LE_CONST_VALUE
				||
			        (avail_locations[alid].kind == GDW_LE_LOCATION
			         && GET_CODE(avail_locations[alid].rtl) == MEM))
			   )
			{
				DWARF_SET_DFA_BIT(implicit_in, alid);
			}
		}
	}

	(void) memcpy((void*) dwarf_in_sets[0], (void*) implicit_in,
					set_size_in_bytes);

	for (w = 0; w < dwarf_dfa_set_size_in_words; w++)
		dwarf_out_sets[0][w] =
				(dwarf_in_sets[0][w] | dwarf_gen_sets[0][w])
				& (~dwarf_kill_sets[0][w]);

	/* Initialize all other blocks' out sets to the universe less its
	   kill set.
	 */

	for (b = 1; b < n_basic_blocks; b++)
	{
		unsigned long	*out_set = dwarf_out_sets[b];
		unsigned long	*kill_set = dwarf_kill_sets[b];

		for (w = 0; w < dwarf_dfa_set_size_in_words; w++)
			out_set[w] = ~kill_set[w];
	}

	/* Run the dataflow */

	do
	{
	  changes = 0;

	  /* Recompute the in and out set for each block.
	     Set 'changes' if any have changed from their previous value.
	   */

	  for (b = 0; b < n_basic_blocks; b++)
	  {
	    unsigned long	*in, *out, *pred_out, *gen, *kill;
	    int			pred;

	    in = dwarf_in_sets[b];
	    out = dwarf_out_sets[b];

	    /* Compute this block's in set as the intersection
	       of it's predecessors out sets.  Don't recompute
	       the in set of a block that has no predecessors
	       (generally only the first block, though the first
	       block may have predecessors too).
	     */

	    pred = next_sized_flex(df_data.maps.preds + b, -1);

	    if (pred >= FIRST_REAL_BLOCK && pred <= LAST_REAL_BLOCK)
	    {
	      /* Don't need to check for in sets changing.
		 If the out sets don't change, then the in sets
		 must have completely settled down on this iteration.
	       */

	      (void) memcpy((void*)in, (void*)dwarf_out_sets[pred],
						set_size_in_bytes);
	      while ((pred = next_sized_flex(df_data.maps.preds+b,pred)) >= FIRST_REAL_BLOCK && pred <= LAST_REAL_BLOCK)
	      {
		pred_out = dwarf_out_sets[pred];
		for (w = 0; w < dwarf_dfa_set_size_in_words; w++)
		  in[w] &= pred_out[w];
	      }

	      /* If the first block has predecessors, we must intersect its
		 in set with implicit_in.  Otherwise its in set might
		 erroneously have been set to the universe of alids.
	       */
	      if (b == 0)
	      {
		for (w = 0; w < dwarf_dfa_set_size_in_words; w++)
		  in[w] &= implicit_in[w];
	      }
	    }

	    /* Compute this block's out set as (in + gen - kill).
	       gen and kill should be disjoint, so the order of arithmetic
	       is irrelevant.
	     */

	    gen = dwarf_gen_sets[b];
	    kill = dwarf_kill_sets[b];

	    /* Don't spend time checking for changes if some out set has
	       already changed.
	     */
	    if (changes)
	    {
	      for (w = 0; w < dwarf_dfa_set_size_in_words; w++)
		out[w] = (in[w] | gen[w]) & (~kill[w]);
	    }
	    else
	    {
	      for (w = 0; w < dwarf_dfa_set_size_in_words; w++)
	      {
		unsigned long	newword = (in[w] | gen[w]) & (~kill[w]);
		if (newword != out[w])
		{
		  out[w] = newword;
		  changes = 1;
		  /* Finish the loop, no need to check for changes */
	          for (w++; w < dwarf_dfa_set_size_in_words; w++)
		    out[w] = (in[w] | gen[w]) & (~kill[w]);
		}
	      }
	    }
	  }
	} while (changes);

	/* Now translate the dataflow information into valid ranges.
	 */

	if (DWARF_PRAGMA_SECTION_SEEN)
	{
		/* Let each decl have at most one location, with
		   a range valid over its entire scope.  As a heuristic,
		   pick a valid location with the highest alid.
		*/

		for (i = 1; i <= dwarf_max_alid; )
		{
			int loc_alid = 0;
			for (alid = i; alid <= dwarf_max_alid
					&& avail_locations[alid].decl
					   == avail_locations[i].decl; alid++)
			{
				if (avail_locations[i].rtl
				    && avail_locations[i].kind != GDW_LE_OTHER)
				loc_alid = alid;
			}

			if (loc_alid > 0)
				dwarf_add_valid_range(loc_alid);

			i = alid;
		}
	}
	else
	{
	  for (b = 0; b < n_basic_blocks; b++)
	  {
		rtx	insn = basic_block_head[b];
		rtx	lim = NULL_RTX;

		if (b != n_basic_blocks - 1)
			lim = basic_block_head[b+1];

		/* Tell the transfer functions that we are generating ranges */
		dwarf_generate_ranges = 1;

		/* Clear the gen and kill sets and make them available to the
		   transfer functions.
		 */

		static_in = dwarf_in_sets[b];
		static_gen = dwarf_gen_sets[b];
		static_kill = dwarf_kill_sets[b];
		for (w = 0; w < dwarf_dfa_set_size_in_words; w++)
		{
			static_gen[w] = 0;
			static_kill[w] = 0;
		}

		/* Start a new valid range for alids which are available at
		   the beginning of this block, are in scope, and for which
		   a range isn't currently active.
		 */

		for (alid = 1; alid <= dwarf_max_alid; alid++)
		{
			if (avail_locations[alid].in_scope
			    && DWARF_IS_DFA_BIT_SET(static_in, alid)
			    && !DWARF_IS_A_RANGE_ACTIVE(alid))
			{
				dwarf_add_valid_range(alid);
				DWARF_CUR_RANGE_LOWER_BOUND(alid) =
				  PREV_INSN(insn)
					? PREV_INSN(insn)
					: DWARF_FIRST_OR_LAST_BOUNDARY_NOTE;
			}
		}

		while (insn != lim)
		{
			dwarf_dfa_transfer(insn);
			insn = NEXT_INSN(insn);
		}

		dwarf_generate_ranges = 0;
	  }

	  /* Give an upper bound to any un-ended valid-ranges. */ 
	  for (alid = 1; alid <= dwarf_max_alid; alid++)
	  {
		if (avail_locations[alid].bounds
		    && DWARF_CUR_RANGE_UPPER_BOUND(alid) == NULL_RTX)
			DWARF_CUR_RANGE_UPPER_BOUND(alid) =
					DWARF_FIRST_OR_LAST_BOUNDARY_NOTE;
	  }
	}

	dwarf_debugging_dump("After avail locations dataflow");

	/* Compress the valid ranges for each alid.
	   For each decl that has only one valid range, and that range
	   covers its entire scope, remove the valid range.  This allows
	   us to emit its location expression directly within its DIE,
	   rather than having to emit it in .debug_loc with an address range.
	 */

	for (alid = 1; alid <= dwarf_max_alid; alid++)
	{
		Dwarf_avail_location	*alidp = &avail_locations[alid];

		for (b = 0; b <= alidp->max_bound_index; b += 2)
		{
		  int	tail;
		  for (tail = b+2; tail <= alidp->max_bound_index;)
		  {
		    if (alidp->bounds[tail] == alidp->bounds[b+1])
		    {
		      int j;
		      /* Shift all bounds after tail left one range position. */
		      alidp->bounds[b+1] = alidp->bounds[tail+1];
		      for (j = tail+2; j <= alidp->max_bound_index; j += 2)
		      {
		        alidp->bounds[j-2] = alidp->bounds[j];
		        alidp->bounds[j-1] = alidp->bounds[j+1];
		      }
		      alidp->max_bound_index -= 2;
		    }
		    else
			tail += 2;
		  }
		}

		/* If the alid has only one valid range, and that range
		   covers its entire scope, change the bounds to NULL_RTX.
		   This allows us to emit its location expression directly
		   within its DIE, rather than having to emit it in .debug_loc
		   with an address range.
		 */
		if (alidp->max_bound_index == 1
		    && (alidp->bounds[0] == DWARF_FIRST_OR_LAST_BOUNDARY_NOTE
			|| alidp->bounds[0] == alidp->block_begin_note)
		    && (alidp->bounds[1] == DWARF_FIRST_OR_LAST_BOUNDARY_NOTE
			|| alidp->bounds[1] == alidp->block_end_note))
		{
			alidp->bounds[0] = NULL_RTX;
			alidp->bounds[1] = NULL_RTX;
		}
	}

	/* Now create NOTE_INSN_DWARF_LE_BOUNDARY notes to mark the boundaries.
	   Note that the DWARF spec says the address marking the end of a
	   location's valid range,
		"marks the first address PAST the end of the
		address range over which the location is valid."

	   So, if a location is killed in insn I, which starts at address A(I),
	   then the location's valid range end address should be A(I)+1 (or
	   perhaps A(I) + M where M is the machine's mininum instruction size
	   in bytes).
	   To implement this, we would want to emit a label L before I, and
	   use "L+1" as the end address.  However, we don't want to add 1
	   when the killing insn is a NOTE_INSN_BLOCK_END note, or is the
	   logical end of the function.  To avoid having to check for these
	   special cases, we simply emit the label L AFTER the killing insn
	   instead of BEFORE it.  This can lead to a too-large range only
	   when I expands to multiple machine instructions, and a debugger
	   user somehow breaks on the second or subsequent such instruction.
	   But this cannot happen by setting a symbolic breakpoint, so we
	   won't worry about it.
	 */

	if (!DWARF_PRAGMA_SECTION_SEEN)
	{
	  for (alid = 1; alid <= dwarf_max_alid; alid++)
	  {
		Dwarf_avail_location	*alidp = &avail_locations[alid];

		for (b = 0; b <= alidp->max_bound_index; b += 2)
		{
			/* Sanity check, verify that if either the lower or
			   upper bound is NULL_RTX, then so is the other one.
			 */
			if ((!alidp->bounds[b] && alidp->bounds[b+1])
			    ||
			    (!alidp->bounds[b+1] && alidp->bounds[b]))
				abort();

			if (alidp->bounds[b] && alidp->bounds[b] !=
					DWARF_FIRST_OR_LAST_BOUNDARY_NOTE)
			{
				rtx	note = dwarf_emit_boundary_note_after(
							alidp->bounds[b]);

				if (note == DWARF_FIRST_OR_LAST_BOUNDARY_NOTE)
				/* This can only happen if a valid range starts
				   AFTER the function's last insn, which means
				   the last insn gen's an alid.  We can't set
				   the lower bound to
				   DWARF_FIRST_OR_LAST_BOUNDARY_NOTE, because
				   in the context of a lower bound, that would
				   be incorrectly construed as the function's
				   first insn.  Instead we delete the range,
				   which would otherwise be empty and thus
				   useless.
				 */
				{
				  /* Delete the range by shifting all
				     ranges left one position.
				  */
				  int j = b + 2;
				  for (; j <= alidp->max_bound_index; j += 2)
				  {
				    alidp->bounds[j-3] = alidp->bounds[j-1];
				    alidp->bounds[j-2] = alidp->bounds[j];
				  }
				  alidp->max_bound_index -= 2;

				  b -= 2;	/* Tweak outer loop iteration
						   to account for the shifted
						   bounds.
						 */
				  continue;
				}

				alidp->bounds[b] = note;
			}

			if (alidp->bounds[b+1] && alidp->bounds[b+1] !=
					DWARF_FIRST_OR_LAST_BOUNDARY_NOTE)
			{
				alidp->bounds[b+1]=
					dwarf_emit_boundary_note_after(
							alidp->bounds[b+1]);
			}
		}
	  }
	}

	/* Clean up */

	free((void*) implicit_in);
	free((void*) dwarf_in_sets[0]);
	free((void*) dwarf_out_sets[0]);
	free((void*) dwarf_gen_sets[0]);
	free((void*) dwarf_kill_sets[0]);
	free((void*) dwarf_in_sets);
	dwarf_in_sets = dwarf_out_sets = dwarf_gen_sets = dwarf_kill_sets = 0;

	free((void*) dwarf_alids_killed_by[0]);
	dwarf_alids_killed_by[0] = 0;
}

/*
 * ------------------------------------------------------------------------
 * Define functions and data structures to support the .debug_line section.
 * ------------------------------------------------------------------------
 */

static char*
xstrdup(s)
char	*s;
{
	char *p = (char *) xmalloc(strlen(s) + 1);

	(void) strcpy(p, s);
	return p;
}

/* Enumerate the standard opcodes, which run from 1 to (DWARF_OPCODE_BASE-1) */

enum dwarf_lns_standard_opcode {
	ZEROTH_AND_UNUSED_LNS_OPCODE,	/* standard opcodes start at 1 */

#define DEFLNSOPCODE(enum_name, id, n_operands)	enum_name,
#include "i_dwarf2.def"

	DWARF_OPCODE_BASE
};

static struct dwarf_lns_standard_opcode_info {
	enum dwarf_lns_standard_opcode	enum_code;
	unsigned int		opcode;
	int			num_operands;
} dwarf_lns_standard_opcode_info[] = {

#define DEFLNSOPCODE(enumerator, opcode, n)	{ enumerator, opcode, n },
#include "i_dwarf2.def"

	{ DWARF_OPCODE_BASE, 0, 0 }
};

/* Define the state machine registers and information used by the
   statement program state machine.
 */

static struct {
	unsigned int	file;		/* file index */
	unsigned long	line;		/* current line number */
	unsigned int	col;		/* current column number */
	int		is_stmt;	/* does insn begin a stmt? */
	int		is_bblock;	/* does insn begin a basic block? */
	/* There is currently no need to simulate the end_seq register. */

	int		default_is_stmt;
	unsigned int	min_insn_length;
	unsigned int	opcode_base;	/* first special opcode */
	int		line_base;	/* minimum line increment */
	unsigned int	line_range;

	int		next_text_label_num;
		/* We use next_text_label_num to generate unique labels in
		   the .text section.  It is monotonically increasing.
		   These labels track the state machine 'address' register.
		   The difference between two such labels, represented as an
		   assembly expression, represents an amount by which we must
		   advance the program counter.
		 */
} sminfo;

/* Set the state machine registers to their initial values.
   This happens when we initialize sminfo, and whenever we
   emit a DW_LNE_end_sequence instruction.
 */

static void
dwarf_reset_smregs()
{
	sminfo.file = 1;
	sminfo.line = 1;
	sminfo.col = 0;
	sminfo.is_stmt = sminfo.default_is_stmt;
	sminfo.is_bblock = 0;
}

static void
dwarf_init_sminfo()
{
	sminfo.default_is_stmt = 1;

#if defined(DWARF_MIN_INSTRUCTION_BYTE_LENGTH)
	sminfo.min_insn_length = DWARF_MIN_INSTRUCTION_BYTE_LENGTH;
#else
	sminfo.min_insn_length = 1;	/* Assume 1-byte instructions. */
#endif

	sminfo.opcode_base = DWARF_OPCODE_BASE;

	/* Set the line base dependent on the optimization level.
	   At low optimization levels, we don't need to support negative
	   line number increments in our special opcodes.
	 */

#if defined(DWARF_LINE_BASE)
	sminfo.line_base = DWARF_LINE_BASE;
#else
	sminfo.line_base = (optimize <= 1) ? 0 : -2;
#endif

	/* Set the line range dependent on the minimum instruction size.
	   If the minimum instruction size is small, assume we have
	   a CISC-like machine so that address increments between
	   instructions will be small.
	   Experimentation really needs to be done to determine optimal values.
	 */

#if defined(DWARF_LINE_RANGE)
	sminfo.line_range = DWARF_LINE_RANGE;
#else
	if (sminfo.min_insn_length <= 2)
		sminfo.line_range = 8;
		/* Allows address increment up to (min_insn_length * 30) */
	else
		sminfo.line_range = 10;
		/* Allows address increment up to (min_insn_length * 24) */
#endif

	sminfo.next_text_label_num = 1;

	/* Make sure the values we set are valid. */

	/* opcode_base must allow room for the predefined standard opcodes
	   and must fit in an unsigned byte.
	 */
	assert(sminfo.opcode_base >= 10 && sminfo.opcode_base <= 255);

	/* line_base must fit in a signed byte. */
	assert(sminfo.line_base >= -128 && sminfo.line_base <= 127);

	/* line_range must be non-zero, and the largest line increment must
	   be representable by some special opcode.
	 */
	assert(sminfo.line_range > 0 &&
		sminfo.line_range <= (256 - sminfo.opcode_base));

	dwarf_reset_smregs();
}

/* Lookup a filename (in the table of filenames that we know about)
   and return its "index".

   If the filename is not found, add it to the table and assign it the
   next available unique index number.  Also generate a DEFINE_FILE
   instruction in the .debug_line section.  Callers should be aware that
   we may leave the assembler output in the .debug_line section.

   Note:  We use the full file_name, as given, for matching file names.
   We do not use just the file's base name.  The #line directives generated
   by the preprocessor always have a full path name, or are relative to the
   compilation directory, so there is no need to strip off the path name and
   use just the base name.

   The include directory index is always set to zero, which is ignored if
   the filename is absolute, and which is correct otherwise (the filename
   is relative to the current directory).

   Note that for DOS, we don't currently have enough information to determine
   the absolute path to a drive-relative pathname such as "D:xyz".
*/

static unsigned int
lookup_filename(file_name)
char	*file_name;
{
	/* Define an array of structures to keep track of source filenames.
	   We need to assign a unique, non-zero, integer to each source file.
	   The assigned integers start with 1, are contiguous and increasing.
	   For n > 0 && n < ft_entries, filename_table[n].number == n.

	   To support fast lookup, element zero of the array is a copy of
	   the most recently looked-up file.  Searches are linear.
	*/

	typedef struct filename_entry {
		unsigned long	number;
		char		*name;
	} filename_entry;

static	filename_entry	*filename_table = NULL;
static	unsigned int	ft_entries_allocated = 0;	/* entries allocated */
static	unsigned int	ft_entries = 0;			/* entries in use */
#define FT_ENTRIES_INCREMENT 64

	filename_entry	*search_p;
	filename_entry	*limit_p = &filename_table[ft_entries];

	if (!file_name || file_name[0] == '\0')
		return 0;

	for (search_p = filename_table; search_p < limit_p; search_p++)
	{
		if (is_same_file_by_name(file_name, search_p->name))
		{
			/* Copy the file info to element 0 so it will
			   be found quickly next time.
			 */
			if (search_p != filename_table)
				filename_table[0] = *search_p;
			return filename_table[0].number;
		}
	}

	if (ft_entries == ft_entries_allocated)
	{
		int	new_size;

		ft_entries_allocated += FT_ENTRIES_INCREMENT;
		new_size = ft_entries_allocated * sizeof(filename_entry);

		/* Avoid calling realloc with a NULL pointer, some hosts
		   can't handle that even though they should.
		 */
		filename_table = (filename_entry *)
			(filename_table ? xrealloc(filename_table, new_size)
					 : xmalloc(new_size));
	}

	/* Add the new entry at the end of the filename table.  */

	filename_table[ft_entries].number = ft_entries;
	filename_table[ft_entries].name = xstrdup(file_name);

	/* If this is the first entry, assign it number 1, and put
	   it in entry 1.  Element 0 is a cache of the most recently
	   referenced element.
	 */

	if (ft_entries == 0) {
		filename_table[0].number = 1;
		filename_table[1] = filename_table[0];
		ft_entries = 2;
	}
	else
		filename_table[0] = filename_table[ft_entries++];

	{
		unsigned long	length;

		/* Generate a .debug_line DEFINE_FILE entry.

		   Currently we always use 0 for the directory index.
		   We do the same for the time stamp and file size, indicating
		   these values are unknown.
		 */

		(void) fputc('\n', asm_out_file);
		debug_line_section();
		dwarf_output_udata(0, "extended opcode indicator");

		 /* This is kind of tricky.  We must output the byte-length
		    of several items before outputting the items themselves.
		 */

		length = 1 			/* size of extended opcode */
			 + strlen(filename_table[0].name) + 1
			 + udata_byte_length(0)	/* directory index */
			 + udata_byte_length(0)	/* modification time */
			 + udata_byte_length(0); /* file length */
		dwarf_output_udata(length, "instruction length");

		dwarf_output_data1(DW_LNE_define_file, "DW_LNE_define_file");
		dwarf_output_string(filename_table[0].name, NO_COMMENT);
		dwarf_output_udata(0, "directory index");
		dwarf_output_udata(0, "modification time");
		dwarf_output_udata(0, "file length");
	}

	return filename_table[0].number;
}

/* The "statement program prologue" is the bookkeeping information generated at
   the beginning of the .debug_line section.  Unfortunately, we have a chicken
   and egg problem when it comes to initializing the .debug_line section.
   Ideally, this would happen in dwarfout_init.  However, calls to
   dwarfout_define, dwarfout_undef, dwarfout_resume_previous_source_file, and
   dwarfout_start_new_source_file can be made (when lang_init() calls
   check_newline) BEFORE dwarfout_init has been called.  And some of these
   dwarfout_* functions must be able to generate instructions in .debug_line.
   Some of these functions also emit entries in the .debug_macinfo section,
   but we must define a starting label in that section before anything is
   emitted there.
   So, we let whoever is called first do some pre-initialization.
 */

static	int	pre_initialize_done = 0;

static void
dwarf_preinitialize()
{
	int	i;

	if (pre_initialize_done)
		return;
	pre_initialize_done = 1;

#if defined(IMSTG_BIG_ENDIAN)
	dwarf_target_big_endian = bytes_big_endian;
#else
	dwarf_target_big_endian = BYTES_BIG_ENDIAN;
#endif

	/* Initialize macro parsing tables */

	for (i = 'a'; i <= 'z'; i++)
	{
		is_idchar[i] = is_idstart[i] = 1;
		is_idchar[i - 'a' + 'A'] = is_idstart[i - 'a' + 'A'] = 1;
	}

	for (i = '0'; i <= '9'; i++)
		is_idchar[i] = 1;

	is_idchar['_'] = is_idstart['_'] = 1;
	is_idchar['$'] = is_idstart['$'] = dollars_in_ident;

	/* horizontal space table (excludes newline) */
	is_hor_space[' '] = is_hor_space['\t'] = is_hor_space['\v'] =
			is_hor_space['\f'] = is_hor_space['\r'] = 1;

	/* Initialize our model of the line number state machine */
	dwarf_init_sminfo();

	/* Start the .debug_macinfo section. */

	(void) fputc('\n', asm_out_file);
	debug_macinfo_section();
	ASM_OUTPUT_LABEL(asm_out_file, DEBUG_MACINFO_BEGIN_LABEL);

	/* Start the .debug_line section. */

	(void) fputc('\n', asm_out_file);
	debug_line_section();
	ASM_OUTPUT_LABEL(asm_out_file, DEBUG_LINE_BEGIN_LABEL);

	/* Output the statement program prologue */
	dwarf_output_delta4_minus4(DEBUG_LINE_END_LABEL,
				DEBUG_LINE_BEGIN_LABEL, "total length - 4");

	dwarf_output_data2(2, "version of line number information");
	dwarf_output_delta4(DEBUG_LINE_BEGIN_PROGRAM_LABEL,
			DEBUG_LINE_POST_PROLOGUE_LENGTH_LABEL,
			"prologue length");
	ASM_OUTPUT_LABEL(asm_out_file, DEBUG_LINE_POST_PROLOGUE_LENGTH_LABEL);

	dwarf_output_data1((int) sminfo.min_insn_length,
				"minimum instruction length");

	dwarf_output_data1(sminfo.default_is_stmt, "default is stmt");
	dwarf_output_data1(sminfo.line_base, "line base");
	dwarf_output_data1((int) sminfo.line_range, "line range");
	dwarf_output_data1((int) sminfo.opcode_base, "opcode base");

	{
		struct dwarf_lns_standard_opcode_info	*p;

		p = &dwarf_lns_standard_opcode_info[0];
		for (; p->enum_code != DWARF_OPCODE_BASE; p++) {
			dwarf_output_data1(p->num_operands,
				"standard opcode length");
		}
	}

	/* We don't list any include directories.  File names are either
	   absolute, or relative to the compilation directory.
	 */
	dwarf_output_data1(0, "end include directory list");

	/* We don't know any of the file names yet, so emit an empty
	   file name list.  We'll use the extended opcode DEFINE_FILE
	   to generate file names on the fly.
	   Note that if the source file comes from stdin, DWARF says we
	   need a file name entry consisting of a single nul-byte.
	   If we put such a filename here, it would be confused with
	   the nul-byte that terminates the list of file names.
	   If such a file name is needed, it must be defined in
	   .debug_line using the define_file extended opcode.
	 */
	dwarf_output_data1(0, "end source file name list");
	ASM_OUTPUT_LABEL(asm_out_file, DEBUG_LINE_BEGIN_PROGRAM_LABEL);

	/* Set the debugger's state machine's 'address' register to the
	   beginning of this compilation unit's .text section.
	   Emit the DW_LNE_set_address extended opcode.
	 */

	dwarf_output_udata(0, "extended opcode indicator");
	/* Assume the size of an address is TARGET_POINTER_BYTE_SIZE */
	dwarf_output_udata(1 + TARGET_POINTER_BYTE_SIZE, "instruction length");
	dwarf_output_data1(DW_LNE_set_address, "DW_LNE_set_address");
	dwarf_output_addr(TEXT_BEGIN_LABEL, NO_COMMENT);

	/* Convert abbreviation attribute lists from strings to integers */

	for (i=0; abbrev_table[i].abbrev_id != LAST_AND_UNUSED_ABBREV_CODE; i++)
	{
		char		*attrs, *forms;
		int		k, num_attrs;
		Debug_Abbrev	*abbrev = &abbrev_table[i];

		attrs = abbrev->attributes;
		forms = abbrev->form_encodings;

		num_attrs = strlen(attrs)/2; /* 2-chars per encoding */
		abbrev->attributes =
			(char*) xmalloc(sizeof(unsigned long) * (num_attrs+1));

		for (k = 0; *attrs; attrs += 2, forms++, k++)
		{
			int		n;
			unsigned long	attr_id = 0;

			/* Determine the DW_AT_ id associated with *attrs */

			for (n = 0; attr_mapping[n].encoded_name; n++)
			{
				char	*p = attr_mapping[n].encoded_name;
				if (*attrs == *p && attrs[1] == p[1])
				{
					attr_id = attr_mapping[n].id;
					break;
				}
			}
			assert(attr_id);

			XABBREV_ATTR(abbrev, k) = attr_id;
		}
		XABBREV_ATTR(abbrev, k) = 0;
		assert(*attrs == '\0' && *forms == '\0' && k == num_attrs);
	}
}

/*
 * ----------------------------------------------------------------------------
 * Define functions and data structures to support the .debug_pubnames section.
 * ----------------------------------------------------------------------------
 */

#define function_should_have_a_pubnames_entry(decl) \
	(TREE_PUBLIC(decl) && !DECL_EXTERNAL(decl) && DECL_INITIAL(decl) \
		    && !DECL_ABSTRACT(decl))

#define var_should_have_a_pubnames_entry(decl) \
	(TREE_PUBLIC(decl) && !DECL_EXTERNAL(decl) \
		&& GET_CODE(DECL_RTL(decl)) == MEM && !DECL_ABSTRACT(decl))

/* Emit a .debug_pubnames entry for the given declaration. */

static void
dwarf_output_pubnames_entry(decl)
tree	decl;
{
	char	pub_label[MAX_ARTIFICIAL_LABEL_BYTES];
	char	*name;

	/* Note that for C++, we will need to output the fully
	   qualified name.  This isn't implemented yet.
	 */

	if (DECL_NAME(decl) && (name = IDENTIFIER_POINTER(DECL_NAME(decl))))
	{
		(void) fputc('\n', asm_out_file);
		debug_pubnames_section();
		(void) ASM_GENERATE_INTERNAL_LABEL(pub_label,
				PUB_DIE_LABEL_CLASS, DECL_UID(decl));
		dwarf_output_ref4(pub_label, NO_COMMENT);
		dwarf_output_string(name, NO_COMMENT);
	}
}

/*
 * -------------------------------------------------------------------------
 * Define functions and data structures to support the .debug_frame section.
 * -------------------------------------------------------------------------
 */

static Dwarf_cie	*dwarf_cie_list = NULL;
	/* Keep all CIE's ever used on this list.  Emit them all together
	   in dwarfout_finish.  We could simply emit them as they are
	   created, but ...  We have to keep them all around anyway, to
	   check for duplicates.  And emitting them inline makes the
	   .text assembly code for the function very unreadable.
	   Switching between .debug_frame and .text or a named .text section
	   also becomes tricky.
	 */

Dwarf_fde	dwarf_fde;
	/* This FDE is filled in during code generation,
	   and emitted by dwarfout_end_epilogue.
	 */

int	dwarf_fde_code_label_counter = 0;
	/* Used to generate unique labels in .text near pro/epi-logue code */

/* Emit the given CIE in the .debug_frame section */

static void
dwarf_output_debug_frame_rule(ciep, reg, opcode, oprnd)
Dwarf_cie	*ciep;
unsigned long	reg;
unsigned long	opcode;
unsigned long	oprnd;		/* unfactored-offset or register number */
{
	unsigned long	offset;
static	char		reg_comment[] = "register column number";

	switch (opcode)
	{
	case DW_CFA_undefined:
		dwarf_output_data1((int)opcode, "DW_CFA_undefined");
		dwarf_output_udata(reg, reg_comment);
		break;

	case DW_CFA_same_value:
		dwarf_output_data1((int)opcode, "DW_CFA_same_value");
		dwarf_output_udata(reg, reg_comment);
		break;

	case DW_CFA_remember_state:
		dwarf_output_data1((int)opcode, "DW_CFA_remember_state");
		break;

	case DW_CFA_restore_state:
		dwarf_output_data1((int)opcode, "DW_CFA_restore_state");
		break;

	case DW_CFA_restore:
		if (reg <= 0x3f)
		{
			/* Pack opcode and regnum into one byte. */
			dwarf_output_data1((int) (DW_CFA_restore | reg),
						"DW_CFA_restore");
			break;
		}
		/* FALL THROUGH */

	case DW_CFA_restore_extended:
		dwarf_output_data1(DW_CFA_restore_extended,
				"DW_CFA_restore_extended");
		dwarf_output_udata(reg, reg_comment);
		break;

	case DW_CFA_def_cfa:
		dwarf_output_data1(DW_CFA_def_cfa, "DW_CFA_def_cfa");
		dwarf_output_udata(reg, reg_comment);
		offset = oprnd / ciep->data_alignment_factor;
		dwarf_output_udata(offset, "factored offset");
		break;

	case DW_CFA_offset:
		if (reg <= 0x3f)
		{
			/* Pack opcode and regnum into one byte. */
			dwarf_output_data1((int) (DW_CFA_offset | reg),
						"DW_CFA_offset");
		}
		else
		{
			dwarf_output_data1(DW_CFA_offset_extended,
					"DW_CFA_offset_extended");
			dwarf_output_udata(reg, reg_comment);
		}
		offset = oprnd / ciep->data_alignment_factor;
		dwarf_output_udata(offset, "factored offset");
		break;

	case DW_CFA_register:
		dwarf_output_data1(DW_CFA_register, "DW_CFA_register");
		dwarf_output_udata(reg, reg_comment);
		dwarf_output_udata(oprnd, "saved in this register");
		break;

	case DW_CFA_i960_pfp_offset:
		dwarf_output_data1(DW_CFA_i960_pfp_offset,
					"DW_CFA_i960_pfp_offset");
		dwarf_output_udata(reg, reg_comment);
		offset = oprnd / ciep->data_alignment_factor;
		dwarf_output_udata(offset, "factored offset");
		break;

	default:
		abort();
	}
}

/* Emit the given CIE in the .debug_frame section */

static void
dwarf_output_cie(ciep)
Dwarf_cie	*ciep;
{
	char	begin_label[MAX_ARTIFICIAL_LABEL_BYTES];
	char	end_label[MAX_ARTIFICIAL_LABEL_BYTES];
	int	align;
	unsigned long	i;

	(void) ASM_GENERATE_INTERNAL_LABEL(begin_label,
				DWARF_CIE_BEGIN_LABEL_CLASS, ciep->label_num);
	/* If another CIE will be emitted immediately after this one,
	   then use its label for this one's end label.
	 */
	if (ciep->next_cie)
		(void) ASM_GENERATE_INTERNAL_LABEL(end_label,
				DWARF_CIE_BEGIN_LABEL_CLASS,
				ciep->next_cie->label_num);
	else
		(void) ASM_GENERATE_INTERNAL_LABEL(end_label,
				DWARF_CIE_END_LABEL_CLASS, ciep->label_num);

	ASM_OUTPUT_LABEL(asm_out_file, begin_label);
	dwarf_output_delta4_minus4(end_label, begin_label, "CIE length");
	dwarf_output_data4(-1, "CIE id");
	dwarf_output_data1(1, "call frame version number");
	dwarf_output_string(ciep->augmentation, "augmenter");
	dwarf_output_udata(ciep->code_alignment_factor,
						"code alignment factor");
	/* DWARF says the data alignment is a signed LEB128. */
	dwarf_output_sdata((long) ciep->data_alignment_factor,
						"data alignment factor");
	dwarf_output_data1(ciep->return_address_register,
						"return address register");

	/* Output the initial CFA and register rules. */

	dwarf_output_debug_frame_rule(ciep, ciep->cfa_register,
					(unsigned long) ciep->cfa_rule,
					ciep->cfa_offset);

	for (i = 0; i < DWARF_CIE_MAX_REG_COLUMNS; i++)
	{
		dwarf_output_debug_frame_rule(ciep, i,
				ciep->rules[i][0], ciep->rules[i][1]);
	}

	/* Make sure the CIE size is a multiple of the code_alignment_factor.
	   The DW_CFA_nop macro is conveniently defined as 0, which is the
	   fill value for most assemblers' .align directive.
	 */

	align = exact_log2(ciep->code_alignment_factor);
	if (align >= 1)
		(void) ASM_OUTPUT_ALIGN(asm_out_file, align);

	if (!ciep->next_cie)
		ASM_OUTPUT_LABEL(asm_out_file, end_label);
}

/* Record a CIE on our cie list, if not already there.  */

Dwarf_cie*
dwarfout_cie(ciep)
Dwarf_cie	*ciep;
{
	int		i, identical;
	Dwarf_cie	*p;
static	int		label_counter = 0;

	for (p = dwarf_cie_list; p; p = p->next_cie)
	{
		/* Compare ciep with p.  If identical, then we've
		   already emitted an appropriate cie.
		 */

		if (strcmp(ciep->augmentation, p->augmentation)
		    || ciep->code_alignment_factor != p->code_alignment_factor
		    || ciep->data_alignment_factor != p->data_alignment_factor
		    || ciep->return_address_register !=
		       p->return_address_register
		    || ciep->cfa_rule != p->cfa_rule
		    || ciep->cfa_register != p->cfa_register
		    || ciep->cfa_offset != p->cfa_offset)
			continue;

		identical = 1;
		for (i = 0; i < DWARF_CIE_MAX_REG_COLUMNS; i++)
		{
			if (ciep->rules[i][0] != p->rules[i][0]
			    || ciep->rules[i][1] != p->rules[i][1])
			{
				identical = 0;
				break;
			}
		}

		if (identical)
			return p;	/* EARLY EXIT */
	}

	/* We need to create and emit a new CIE */

	p = (Dwarf_cie*) xmalloc(sizeof(Dwarf_cie));
	(void) memcpy(p, ciep, sizeof(Dwarf_cie));
	p->label_num = ++label_counter;
	p->next_cie = dwarf_cie_list;
	dwarf_cie_list = p;

	return p;
}

/*
 * ---------------------------------------------------
 * begin definitions of DIE-specific output functions.
 * ---------------------------------------------------
 */

static void
dwarf_output_compile_unit_die(primary_source_file_name)
char	*primary_source_file_name;
{
	unsigned long	language;
	char		*lang_name;

	(void) fputc('\n', asm_out_file);
	dwarf_output_abbreviation(DW_ABBREV_COMPILE_UNIT);

	dwarf_output_addr(TEXT_BEGIN_LABEL, "low pc");
	dwarf_output_addr(TEXT_END_LABEL, "high pc");

	dwarf_output_string(primary_source_file_name, NO_COMMENT);

	if (strcmp(language_string, "GNU C++") == 0)
	{
		language = DW_LANG_C_plus_plus;
		lang_name = "DW_LANG_C_plus_plus";
	}
	else if (strcmp(language_string, "GNU C") == 0)
	{
		if (flag_traditional)
		{
			language = DW_LANG_C;
			lang_name = "DW_LANG_C";
		}
		else
		{
			language = DW_LANG_C89;
			lang_name = "DW_LANG_C89";
		}
	}
	else
		abort();

	dwarf_output_udata(language, lang_name);


	dwarf_output_delta4(DEBUG_LINE_BEGIN_LABEL,
			DEBUG_LINE_GLOBAL_BEGIN_LABEL, NO_COMMENT);
	dwarf_output_delta4(DEBUG_MACINFO_BEGIN_LABEL,
			DEBUG_MACINFO_GLOBAL_BEGIN_LABEL, NO_COMMENT);

	{
		char	*cwd = getpwd();
		dwarf_output_string(cwd ? cwd : ".", NO_COMMENT);
	}
	dwarf_output_string(DWARF_PRODUCER, NO_COMMENT);
}

/*
 * -----------------------------------------------------------------------
 * Define functions and data structures for emitting DIEs for _TYPE nodes.
 * -----------------------------------------------------------------------
 */

/* Define a hash table to help us keep track of whether a type DIE has
   been emitted for each type variant.  A label of the form PN will
   be emitted at the beginning of the DIE for the main variant of a type,
   where P is the common prefix for type DIEs, and N is the type's TYPE_UID.
   For main variants, the TREE_ASM_WRITTEN flag is used to determine if we
   already emitted the DIE for the type.  Unfortunately, the front end does
   not always create a type variant for qualified types, so for example there
   may not be a volatile _TYPE object in which to set the TREE_ASM_WRITTEN
   flag when we emit a DW_TAG_volatile_type DIE which points to its
   unqualified type DIE.
   So for 'const', 'volatile' and 'const volatile' qualified type DIES, we
   we will emit a label of the form PcN, PvN or PcvN where N is the TYPE_UID
   of the qualified type's TYPE_MAIN_VARIANT.  Our hash table will be keyed
   on N, and will record whether a DIE has been emitted for each qualified
   form of the type.
 */

#define QTYPE_HASH_TABLE_SIZE	41

struct qtype_info {
	struct qtype_info	*next;
	unsigned int		type_uid;
	unsigned int		const_written : 1;
	unsigned int		volatile_written : 1;
	unsigned int		const_volatile_written : 1;
} *qtype_hash_table[QTYPE_HASH_TABLE_SIZE];

static struct qtype_info*
lookup_or_add_qtype_info(type_uid)
unsigned int	type_uid;
{
	struct qtype_info	*q;
	unsigned int		hval;

	hval = type_uid % QTYPE_HASH_TABLE_SIZE;

	for (q = qtype_hash_table[hval]; q; q = q->next)
		if (type_uid == q->type_uid)
			return q;
	q = (struct qtype_info *) xmalloc(sizeof(struct qtype_info));
	q->type_uid = type_uid;
	q->const_written = q->volatile_written = q->const_volatile_written = 0;
	q->next = qtype_hash_table[hval];
	qtype_hash_table[hval] = q;
	return q;
}

/* Write into 'buf' the label that would be generated at the beginning
   of the DIE for the given type.

   The front end will often record the const-ness and volatile-ness of
   an object in its _DECL node, rather than in a _TYPE node.
   decl_const and decl_volatile indicate whether you want the 'const'
   and/or 'volatile' qualified form of the type, even if the type itself
   is not so qualified.
 */

static void
determine_type_die_label(type, buf, decl_const, decl_volatile)
tree	type;
char	*buf;
int	decl_const, decl_volatile;
{
	decl_const |= TYPE_READONLY(type);
	decl_volatile |= TYPE_VOLATILE(type);

	(void) sprintf(buf, TYPE_DIE_LABEL_FMT,
				decl_const ? (decl_volatile ? "cv" : "c")
					   : (decl_volatile ? "v" : ""),
				TYPE_UID(TYPE_MAIN_VARIANT(type)));
}

/* Define a list containing _TYPE's for which emitting DIEs has been
   temporarily delayed.  Each type belongs to a scope, and the DIE for
   each type must be a child (in the DWARF sense) of its scope's DIE.
   While emitting DIEs for declarations of some inner scope, we generally
   need to emit DIEs for the types of those declarations.  But if such types
   belong to an outer scope, we must hold off emitting the type DIEs until
   after all DIEs for the inner scope have been emitted.
 */

typedef struct {
	tree		type;	/* The type main variant of the _TYPE. */
	unsigned int	pend_const : 1;
	unsigned int	pend_volatile : 1;
	unsigned int	pend_const_volatile : 1;
		/* pend_* indicate which qualified variants of the type are
		   being pended.  DIEs for the variants of a _TYPE are emitted
		   in the same DWARF scope as the DIE for the _TYPE itself.
		 */

	unsigned int	main_variant_asm_was_written : 1;
		/* Indicates whether the type DIE for the type's main variant
		   had already been emitted at the time the qualified type
		   was pended.  You see when we pend a _TYPE, we want to mark
		   it has having been emitted to prevent it from being written
		   or re-pended.  For an unqualified _TYPE (ie, a type main
		   variant), we do this by setting its TREE_ASM_WRITTEN flag.
		   For a qualified _TYPE, we set the appropriate member
		   in the _TYPE's qtype_info record, AND we set the
		   TREE_ASM_WRITTEN flag of the type's main variant.
		   When we unpend a _TYPE, we want to emit a DIE for the _TYPE.
		   But we don't want to re-emit the DIE for the _TYPE's main
		   variant, if it had already been written.
		 */
} Pended_type;

static Pended_type	*pending_types_list;
static unsigned int	pending_types;			/* number in use */
static unsigned int	pending_types_allocated;	/* number allocated */
#define PENDING_TYPES_INCREMENT 64

/* Record a type in the pending_types_list.  */

static void
pend_type(type, pend_const, pend_volatile)
tree	type;
int	pend_const, pend_volatile;
{
	tree	main_variant = TYPE_MAIN_VARIANT(type);
	int	i;

	/* Search for the main variant type on the pended types list,
	   it may already have been pended with different type qualifiers. */

	for (i = 0; i < pending_types; i++)
		if (pending_types_list[i].type == main_variant)
			break;

	if (i >= pending_types)		/* Not found */
	{
		if (pending_types == pending_types_allocated)
		{
			int	new_size;
			pending_types_allocated += PENDING_TYPES_INCREMENT;
			new_size = sizeof(Pended_type)*pending_types_allocated;
			pending_types_list = (Pended_type *)
				(pending_types_list
					? xrealloc(pending_types_list,new_size)
					: xmalloc(new_size));
		}

		i = pending_types++;
		pending_types_list[i].type = main_variant;
		pending_types_list[i].pend_const = 0;
		pending_types_list[i].pend_volatile = 0;
		pending_types_list[i].pend_const_volatile = 0;
		pending_types_list[i].main_variant_asm_was_written =
				TREE_ASM_WRITTEN(main_variant);
		TREE_ASM_WRITTEN(main_variant) = 1;
	}

	pend_const |= TYPE_READONLY(type);
	pend_volatile |= TYPE_VOLATILE(type);

	/* Mark the pending type as having been output already (even though
	   it hasn't been).  This prevents the qualified type from being added
	   to the pending_types_list more than once.
	 */

	if (pend_const || pend_volatile)
	{
		struct qtype_info	*qinfo;

		qinfo = lookup_or_add_qtype_info(TYPE_UID(main_variant));

		if (pend_const && pend_volatile)
		{
			qinfo->const_volatile_written = 1;
			pending_types_list[i].pend_const_volatile = 1;
		}
		else if (pend_const)
		{
			qinfo->const_written = 1;
			pending_types_list[i].pend_const = 1;
		}
		else
		{
			qinfo->volatile_written = 1;
			pending_types_list[i].pend_volatile = 1;
		}
	}
}

/* Determine if the given _TYPE node is a type that can have a name.
   Namable types include the C base types (int, short, void, etc)
   and tagged types (struct, union, and enum types).
 */

static int
is_named_type(type)
tree	type;
{
	switch (TREE_CODE(type))
	{
		case RECORD_TYPE:
		case UNION_TYPE:
		case QUAL_UNION_TYPE:
		case ENUMERAL_TYPE:
		case BOOLEAN_TYPE:
		case CHAR_TYPE:
		case COMPLEX_TYPE:
		case INTEGER_TYPE:
		case REAL_TYPE:
		case VOID_TYPE:
			return 1;
	}

	return 0;
}

/* Return non-zero if it is legitimate to output DIEs to represent a
   given type while we are generating the list of child DIEs for some
   DIE (e.g. a function or lexical block DIE) associated with a given scope.

   See the comments within the function for a description of when it is
   considered legitimate to output DIEs for various kinds of types.

   Note that TYPE_CONTEXT(type) may be NULL (to indicate global scope)
   or it may point to a BLOCK node (for types local to a block), or to a
   FUNCTION_DECL node (for types local to the heading of some function
   definition), or to a FUNCTION_TYPE node (for types local to the
   prototyped parameter list of a function type specification), or to a
   RECORD_TYPE, UNION_TYPE, or QUAL_UNION_TYPE node
   (in the case of C++ nested types).

   The `scope' parameter should likewise be NULL or should point to a
   BLOCK node, a FUNCTION_DECL node, a FUNCTION_TYPE node, a RECORD_TYPE
   node, a UNION_TYPE node, or a QUAL_UNION_TYPE node.

   This function is used only for deciding when to "pend" and when to
   "un-pend" types to/from the pending_types_list.
*/

static int
is_type_ok_for_scope(type, scope)
tree	type;
tree	scope;
{
	/* Named types must always be output only in the scopes where they
	   actually belong (or else the scoping of their own names and the
	   scoping of their member names will be incorrect).  Non-named-types
	   on the other hand can generally be output anywhere, except that
	   svr4 SDB really doesn't want to see them nested within struct
	   or union types, so here we say it is always OK to immediately
	   output any non-named type, so long as we are not within
	   such a context.
	 */

	/* We output pointer types in the same scope as the type pointed to.
	   Don't trust TYPE_CONTEXT of a pointer type.  Similarly for
	   const and volatile qualified types.
	 */

	type = TYPE_MAIN_VARIANT(type);

	while (TREE_CODE(type) == POINTER_TYPE)
		type = TYPE_MAIN_VARIANT(TREE_TYPE(type));

	return is_named_type(type)
		? (TYPE_CONTEXT(type) == scope)
		: (scope == NULL_TREE || !is_named_type(scope));
}

/* Output any pending types (from the pending_types list) which we can output
   now (taking into account the scope that we are working on now).

   For each type output, remove the given type from the pending_types_list
   *before* we try to output it.

   Note that we have to process the list in beginning-to-end order,
   because the call made here to output_type may cause yet more types
   to be added to the end of the list, and we may have to output some
   of them too.
*/

static void
dwarf_output_pending_types_for_scope(scope)
tree	scope;
{
	unsigned int	i;

	for (i = 0; i < pending_types; )
	{
		Pended_type	ptype, *mover, *limit;

		ptype = pending_types_list[i];

		if (!is_type_ok_for_scope(ptype.type, scope))
		{
			i++;
			continue;
		}

		/* Compress the pending types list.
		   Don't increment 'i' in this case because we will have
		   shifted all of the subsequent pending types down one
		   element in the pending_types_list array.
		*/

		pending_types--;
		limit = &pending_types_list[pending_types];
		mover = &pending_types_list[i];
		for (; mover < limit; mover++)
			*mover = *(mover+1);

		/* Output the unqualified version of the type, if needed. */

		if (!ptype.main_variant_asm_was_written)
		{
			TREE_ASM_WRITTEN(ptype.type) = 0;
			dwarf_output_type(ptype.type, scope, 0, 0);
		}

		/* Now output each qualified version of the type
		   that has been pended.
		*/

		if (ptype.pend_const || ptype.pend_volatile
				     || ptype.pend_const_volatile)
		{
			struct qtype_info	*qinfo;
			qinfo = lookup_or_add_qtype_info(TYPE_UID(ptype.type));

			if (ptype.pend_const)
			{
				qinfo->const_written = 0;
				dwarf_output_type(ptype.type, scope, 1, 0);
			}

			/* Emit the singularly qualified 'volatile' type before
			   the 'const volatile' type, since the latter uses the
			   former.
			*/

			if (ptype.pend_volatile)
			{
				qinfo->volatile_written = 0;
				dwarf_output_type(ptype.type, scope, 0, 1);
			}
			if (ptype.pend_const_volatile)
			{
				qinfo->const_volatile_written = 0;
				dwarf_output_type(ptype.type, scope, 1, 1);
			}
		}
	}
}

/* Given a FUNCTION_TYPE or FUNCTION_DECL, return NULL if the function
   does not return a value (or if the return type is an ERROR_MARK),
   otherwise return a pointer to a _TYPE node describing the function's
   return type.
 */

static tree
function_return_type_or_null(node)
tree	node;
{
	if (TREE_CODE(node) == FUNCTION_DECL)
	{
		if (DECL_RESULT(node) == NULL)
			return NULL;
		node = TREE_TYPE(node);
	}

	assert(node && TREE_CODE(node) == FUNCTION_TYPE);
	node = TREE_TYPE(node);
	if (!node || TREE_CODE(node) == VOID_TYPE
		  || TREE_CODE(node) == ERROR_MARK)
		return NULL;
	return node;
}

/* Return a pointer to the tag name for the given type,
   or NULL if the type has no tag name.
 */

static char*
type_tag_name(type)
tree	type;
{
	char	*name = 0;

	if (TYPE_NAME(type) != 0)
	{
		tree	t = 0;

		if (TREE_CODE(TYPE_NAME(type)) == IDENTIFIER_NODE)
			t = TYPE_NAME(type);
		else if (strcmp(language_string, "GNU C++") == 0)
		{
			/* The comment below was borrowed from dwarfout.c.
			   It might imply that we aren't doing the right
			   thing here for C++.
			 */
			/* The g++ front end makes the TYPE_NAME of *each*
			   tagged type point to a TYPE_DECL node, regardless
			   of whether or not a `typedef' was involved.  This
			   is distinctly different from what the gcc front-end
			   does.  It always makes the TYPE_NAME for each tagged
			   type be either NULL (signifying an anonymous tagged
			   type) or else a pointer to an IDENTIFIER_NODE.
			 */
			if (TREE_CODE(TYPE_NAME(type)) == TYPE_DECL)
				t = DECL_NAME(TYPE_NAME(type));
		}

		if (t != 0)
			name = IDENTIFIER_POINTER(t);
	}

	return (name == 0 || *name == '\0') ? 0 : name;
}

static void
dwarf_output_base_type_die(type)
tree	type;
{
	char			type_label[MAX_ARTIFICIAL_LABEL_BYTES];
	enum abbrev_code	abbrev = DW_ABBREV_BASE_TYPE;
	char			*name;
	unsigned long		encoding;
	char			*encoding_name;
	unsigned long		byte_size, bit_size, bit_offset;

	/* Determine values for all attributes of this base type. */

	if (TREE_CODE(TYPE_NAME(type)) == TYPE_DECL)
		name = IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(type)));
	else if (TREE_CODE(TYPE_NAME(type)) == IDENTIFIER_NODE)
		name = IDENTIFIER_POINTER(TYPE_NAME(type));
	else
		abort();

	assert(TYPE_SIZE(type) && TREE_CODE(TYPE_SIZE(type)) == INTEGER_CST);

	byte_size = TREE_INT_CST_LOW(TYPE_SIZE(type)) / BITS_PER_UNIT;
	bit_size = TYPE_PRECISION(type);

	switch (TREE_CODE(type))
	{
	case VOID_TYPE:
		/* By convention, use the DW_ATE_unsigned attribute
		   encoding, with DW_AT_byte_size equal to zero, to
		   indicate the C "void" type.  Note that we use size
		   zero, disregarding the GNU extension which increments
		   pointers to void by 1 when doing pointer arithmetic.
		 */
		encoding = DW_ATE_unsigned;
		byte_size = 0;
		break;

	case BOOLEAN_TYPE:
		encoding = DW_ATE_boolean;
		break;

	case INTEGER_TYPE:
		/* Use byte_size to distinguish between type 'char' and
		   other integral types */

		if (TREE_UNSIGNED(type))
		{
			if (byte_size == 1)
				encoding = DW_ATE_unsigned_char;
			else
				encoding = DW_ATE_unsigned;
		}
		else
		{
			if (byte_size == 1)
				encoding = DW_ATE_signed_char;
			else
				encoding = DW_ATE_signed;
		}
		break;

	case CHAR_TYPE:
		encoding = (TREE_UNSIGNED(type)) ? DW_ATE_unsigned_char
						 : DW_ATE_signed_char;
		break;

	case COMPLEX_TYPE:
		encoding = DW_ATE_complex_float;
		break;

	case REAL_TYPE:
		encoding = DW_ATE_float;

		if (bit_size != byte_size * BITS_PER_UNIT)
			abbrev = DW_ABBREV_PADDED_BASE_TYPE;
		else if (TYPE_MODE(type) == TFmode)
		{
			/* Generally the TYPE_PRECISION for "long double"
			   is set the same as its TYPE_SIZE, even though not
			   all bits in the representation are used.
			   For the 80960, only 80 of the 128 bits are used in
			   little-endian target memory.  In big-endian memory,
			   the value that resides in memory spans 96 bits,
			   with a gap of 16 bits within the value.  However,
			   when loaded into a register, or when operated on
			   within a debugger, the big-endian case converts
			   to the little endian case.  So, use the same
			   bit size and bit offset for both cases.
			 */

			abbrev = DW_ABBREV_PADDED_BASE_TYPE;
			bit_size = 80;
		}

		bit_offset = (byte_size * BITS_PER_UNIT) - bit_size;

		break;

	default:
		abort();
	}

	switch (encoding)
	{
		case DW_ATE_address:
			encoding_name = "DW_ATE_address"; break;
		case DW_ATE_boolean:
			encoding_name = "DW_ATE_boolean"; break;
		case DW_ATE_complex_float:
			encoding_name = "DW_ATE_complex_float"; break;
		case DW_ATE_float:
			encoding_name = "DW_ATE_float"; break;
		case DW_ATE_signed:
			encoding_name = "DW_ATE_signed"; break;
		case DW_ATE_signed_char:
			encoding_name = "DW_ATE_signed_char"; break;
		case DW_ATE_unsigned:
			encoding_name = "DW_ATE_unsigned"; break;
		case DW_ATE_unsigned_char:
			encoding_name = "DW_ATE_unsigned_char"; break;
		default: abort();
	}

	/* Now output the base type DIE. */

	(void) fputc('\n', asm_out_file);
	determine_type_die_label(type, type_label, 0, 0);
	ASM_OUTPUT_LABEL(asm_out_file, type_label);
	dwarf_output_abbreviation(abbrev);
	dwarf_output_string(name, NO_COMMENT);
	dwarf_output_udata(encoding, encoding_name);
	dwarf_output_udata(byte_size, "byte size");

	if (abbrev == DW_ABBREV_PADDED_BASE_TYPE)
	{
		dwarf_output_udata(bit_size, "bit size");
		dwarf_output_udata(bit_offset, "bit offset");
	}
}

static void
dwarf_output_enumeration_type_die(type)
tree	type;
{
	char	*tag_name = type_tag_name(type);
	tree	enumerator;
	char	sib_label[MAX_ARTIFICIAL_LABEL_BYTES];

	dwarf_output_abbreviation(tag_name ? DW_ABBREV_NAMED_ENUM
					   : DW_ABBREV_ANON_ENUM);

	dwarf_output_sibling_attribute(sib_label);
	if (tag_name)
		dwarf_output_string(tag_name, NO_COMMENT);
	dwarf_output_udata((unsigned long) int_size_in_bytes(type),
					"enumeration size");

	/* Output the enumeration literals in the order they
	   were defined in the source. */

	enumerator = TYPE_FIELDS(type);
	for (; enumerator; enumerator = TREE_CHAIN(enumerator))
	{
		dwarf_output_abbreviation(DW_ABBREV_ENUMERATOR);
		dwarf_output_string(
			IDENTIFIER_POINTER(TREE_PURPOSE(enumerator)),
			NO_COMMENT);
		dwarf_output_sdata(
			(long) TREE_INT_CST_LOW(TREE_VALUE(enumerator)),
			NO_COMMENT);
	}

	dwarf_output_null_die("end enumerator list");
	ASM_OUTPUT_LABEL(asm_out_file, sib_label);
}

static void
dwarf_output_array_type_die(type, element_type, element_const, element_volatile)
tree	type;
tree	element_type;
int	element_const;
int	element_volatile;
{
	char	element_type_label[MAX_ARTIFICIAL_LABEL_BYTES];
	char	sib_label[MAX_ARTIFICIAL_LABEL_BYTES];

	determine_type_die_label(element_type, element_type_label,
				element_const, element_volatile);

	(void) fputc('\n', asm_out_file);
	dwarf_output_abbreviation(DW_ABBREV_ARRAY_TYPE);
	dwarf_output_sibling_attribute(sib_label);
	dwarf_output_ref4(element_type_label, "array element type");

	/* Now output DIE's for the array dimensions.
	   The GNU compilers represent multidimensional array types as
	   sequences of one dimensional array types whose element types are
	   themselves array types.  Here we squish that down, so that each
	   multidimensional array type gets only one array_type DIE in the
	   DWARF debugging info.  The draft DWARF specification says that we
	   are allowed to do this kind of compression in C (because there is
	   no difference between an array of arrays and a multidimensional
	   array in C) but for other source languages (e.g. Ada) we
	   shouldn't do this.
	 */

	for (; TREE_CODE(type) == ARRAY_TYPE; type = TREE_TYPE(type))
	{
		tree	domain = TYPE_DOMAIN(type);
		tree	lower = NULL;
		tree	upper = NULL;

		/* Arrays come in three flavors:  unspecified bounds,
		   fixed bounds, and (in GNU C only) variable bounds.
		*/

		if (domain)
		{
			lower = TYPE_MIN_VALUE(domain);
			upper = TYPE_MAX_VALUE(domain);
		}

		/* Lower bound is always zero for C and C++. */
		if (lower)
			assert(TREE_CODE(lower) == INTEGER_CST
				&& TREE_INT_CST_LOW(lower) == 0);

		if (upper && TREE_CODE(upper) == INTEGER_CST)
		{
			dwarf_output_abbreviation(DW_ABBREV_STATIC_ARRAY_BOUND);
			dwarf_output_udata(
				(unsigned long) TREE_INT_CST_LOW(upper),
				"dimension upper bound");
		}
		else if (domain)
		{
			/* An unspecified array index, eg  foo[][100][200].  */
			dwarf_output_abbreviation(
					DW_ABBREV_UNKNOWN_ARRAY_BOUND);
		}
		else
		{
			/* We have a dynamic upper bound.
			   For now treat this like an incomplete array
			   dimension.  Eventually, we should emit a location
			   expression that tells the debugger how to compute
			   the dynamic upper bound.
			*/
			dwarf_output_abbreviation(
					DW_ABBREV_UNKNOWN_ARRAY_BOUND);
		}
	}

	dwarf_output_null_die("end array dimensions");
	ASM_OUTPUT_LABEL(asm_out_file, sib_label);
}

/* Generate a list of nameless DW_TAG_formal_parameter DIEs (and perhaps a
   DW_TAG_unspecified_parameters DIE) to represent the types of the formal
   parameters as specified in some function type specification (except
   for those which appear as part of a function *definition*).
*/

static void
dwarf_output_formal_types(func_type)
tree	func_type;
{
	tree	link;
	tree	formal_type = NULL;
	char	type_label[MAX_ARTIFICIAL_LABEL_BYTES];

	/* For compatibility with gcc's DWARF 1 implementation, put out all
	   DW_AT_formal_parameter DIEs before putting out the types of the
	   parameters.
	 */

	for (link = TYPE_ARG_TYPES(func_type); link; link = TREE_CHAIN(link))
	{
		if ((formal_type = TREE_VALUE(link)) == void_type_node)
			break;

		/* Output a nameless DIE to represent the formal parameter */
		dwarf_output_abbreviation(DW_ABBREV_PARAMETER_TYPE);

		determine_type_die_label(formal_type, type_label, 0, 0);
		dwarf_output_ref4(type_label, "parameter type");
	}

	/* If this function type has an ellipsis, add a
	   DW_TAG_unspecified_parameters DIE to the end of the parameter list.
	 */

	if (formal_type != void_type_node)
		dwarf_output_abbreviation(DW_ABBREV_UNSPECIFIED_PARAMETERS);

	/* Make sure the types of the parameters get emitted. */

	for (link = TYPE_ARG_TYPES(func_type); link; link = TREE_CHAIN(link))
	{
		if ((formal_type = TREE_VALUE(link)) == void_type_node)
			break;

		dwarf_output_type(formal_type, func_type, 0, 0);
	}
}

static void
dwarf_output_function_type_die(func_type)
tree	func_type;
{
	enum abbrev_code	abbrev;
	tree	return_type = function_return_type_or_null(func_type);
	tree	arg_types = TYPE_ARG_TYPES(func_type);
	char	ret_type_label[MAX_ARTIFICIAL_LABEL_BYTES];
	char	func_type_label[MAX_ARTIFICIAL_LABEL_BYTES];
	char	sib_label[MAX_ARTIFICIAL_LABEL_BYTES];

	abbrev = return_type ? DW_ABBREV_FUNC_TYPE
			     : DW_ABBREV_VOID_FUNC_TYPE;

	(void) fputc('\n', asm_out_file);
	determine_type_die_label(func_type, func_type_label, 0, 0);
	ASM_OUTPUT_LABEL(asm_out_file, func_type_label);

	dwarf_output_abbreviation(abbrev);
	dwarf_output_sibling_attribute(sib_label);

	if (return_type)
	{
		determine_type_die_label(return_type, ret_type_label, 0, 0);
		dwarf_output_ref4(ret_type_label, "function return type");
	}

	dwarf_output_flag((arg_types ? 1 : 0), "DW_AT_prototyped");
	dwarf_output_formal_types(func_type);
	dwarf_output_null_die("end of parameters");
	ASM_OUTPUT_LABEL(asm_out_file, sib_label);
}

/*
 * -----------------------------------------------------------------------
 * Define functions and data structures for emitting DIEs for _DECL nodes.
 * -----------------------------------------------------------------------
 */

/* Determine the "ultimate origin" of a decl.  The decl may be an
   inlined instance of an inlined instance of a decl which is local
   to an inline function, so we have to trace all of the way back
   through the origin chain to find out what sort of node actually
   served as the original seed for the given decl.
 */

static tree
decl_ultimate_origin(decl)
tree	decl;
{
	tree	prev = NULL;
	tree	next = DECL_ABSTRACT_ORIGIN(decl);

	while (next && next != prev)
	{
		prev = next;
		next = DECL_ABSTRACT_ORIGIN(next);
	}
	return prev;
}


/* Determine the "ultimate origin" of a block.  The block may be an
   inlined instance of an inlined instance of a block which is local
   to an inline function, so we have to trace all of the way back
   through the origin chain to find out what sort of node actually
   served as the original seed for the given block.
 */

static tree
block_ultimate_origin(block)
tree	block;
{
	tree	prev = NULL;
	tree	next = BLOCK_ABSTRACT_ORIGIN(block);

	while (next && next != prev)
	{
		prev = next;
		next = (TREE_CODE(next) == BLOCK)
				? BLOCK_ABSTRACT_ORIGIN(next)
				: NULL;
	}
	return prev;
}

/* Determine if a BLOCK node represents the outermost pair of curly braces
   (i.e. the "body block") of a function or method.

   For any BLOCK node representing a "body block" of a function or method,
   the BLOCK_SUPERCONTEXT of the node will point to another BLOCK node
   which represents the outermost (function) scope for the function or
   method (i.e. the one which includes the formal parameters).  The
   BLOCK_SUPERCONTEXT of *that* node in turn will point to the relevant
   FUNCTION_DECL node.
*/

static int
is_body_block(stmt)
tree	stmt;
{
	if (TREE_CODE(stmt) == BLOCK)
	{
		tree	parent = BLOCK_SUPERCONTEXT(stmt);

		if (TREE_CODE(parent) == BLOCK)
		{
			tree	grandparent = BLOCK_SUPERCONTEXT(parent);

			if (TREE_CODE(grandparent) == FUNCTION_DECL)
				return 1;
		}
	}
	return 0;
}

/* Emit DW_AT_decl_file, DW_AT_decl_line and DW_AT_decl_column attributes
   for the given _DECL
 */

static void
dwarf_output_source_coordinates(decl)
tree	decl;
{
	unsigned int	file_num;
	unsigned long	line, column;

	file_num = lookup_filename(DECL_SOURCE_FILE(decl));
	line = DECL_SOURCE_LINE(decl);
	column = DECL_SOURCE_COLUMN(decl);

	/* lookup_filename() may have switched into the .debug_line section,
	   so make sure we're back in .debug_info
	 */

	debug_info_section();

	dwarf_output_udata((unsigned long) file_num, "decl file index");
	dwarf_output_udata(line, "decl line number");
	dwarf_output_udata(column, "decl column number");
}

static void
dwarf_output_tagged_type_instantiation(decl)
tree	decl;
{
	tree	type = TREE_TYPE(decl);
	tree	origin = decl_ultimate_origin(decl);
	char	abstract_instance_label[MAX_ARTIFICIAL_LABEL_BYTES];

	if (type == 0 || TREE_CODE(type) == ERROR_MARK)
		return;

	(void) fputc('\n', asm_out_file);
	determine_type_die_label(TREE_TYPE(origin), abstract_instance_label,
			TREE_READONLY(origin), TREE_THIS_VOLATILE(origin));

	if (TREE_CODE(type) == ENUMERAL_TYPE)
		dwarf_output_abbreviation(DW_ABBREV_CONCRETE_ENUMERATION_TYPE);
	else if (TREE_CODE(type) == RECORD_TYPE)
		dwarf_output_abbreviation(DW_ABBREV_CONCRETE_STRUCT_TYPE);
	else if (TREE_CODE(type) == UNION_TYPE
				|| TREE_CODE(type) == QUAL_UNION_TYPE)
		dwarf_output_abbreviation(DW_ABBREV_CONCRETE_UNION_TYPE);
	else
		abort();

	dwarf_output_ref4(abstract_instance_label, "abstract instance ref");
}

/* Emit a DIE for a non-defining function declaration */

dwarf_output_function_declaration_die(fn)
tree	fn;
{
	tree	return_type = function_return_type_or_null(fn);
	tree	origin = decl_ultimate_origin(fn);
	char	*name = IDENTIFIER_POINTER(DECL_NAME(fn));
	int	prototyped = TYPE_ARG_TYPES(TREE_TYPE(fn)) ? 1 : 0;
	char	sib_label[MAX_ARTIFICIAL_LABEL_BYTES];
	char	abstract_instance_label[MAX_ARTIFICIAL_LABEL_BYTES];
	enum abbrev_code	abbrev;

	abbrev = (origin && !DECL_ABSTRACT(fn))
			? DW_ABBREV_CONCRETE_FUNC_DECL
			: (return_type	? DW_ABBREV_FUNC_DECL
					: DW_ABBREV_VOID_FUNC_DECL);

	(void) fputc('\n', asm_out_file);

	if (DECL_ABSTRACT(fn))
	{
		/* Emit a label so that concrete-instance DIEs can reference
		   this DIE.
		 */
		(void) ASM_GENERATE_INTERNAL_LABEL(abstract_instance_label,
					DECL_DIE_LABEL_CLASS, DECL_UID(fn));
		ASM_OUTPUT_LABEL(asm_out_file, abstract_instance_label);
	}

	dwarf_output_abbreviation(abbrev);

	if (abbrev == DW_ABBREV_CONCRETE_FUNC_DECL)
	{
		/* Concrete instance function declarations have only
		   an abstract origin attribute
		 */
		dwarf_output_decl_abstract_origin_attribute(origin);
		return;
	}

	dwarf_output_sibling_attribute(sib_label);
	dwarf_output_string(name ? name : "", NO_COMMENT);
	dwarf_output_source_coordinates(fn);

	if (return_type)
	{
		char	type_label[MAX_ARTIFICIAL_LABEL_BYTES];

		determine_type_die_label(return_type, type_label, 0, 0);
		dwarf_output_ref4(type_label, "return type");
	}

	dwarf_output_flag(1, "DW_AT_declaration");
	dwarf_output_flag(TREE_PUBLIC(fn), "DW_AT_external");

	dwarf_output_flag(prototyped, "DW_AT_prototyped");
	dwarf_output_formal_types(TREE_TYPE(fn));
	dwarf_output_null_die("end parameter decls");

	ASM_OUTPUT_LABEL(asm_out_file, sib_label);
}

/* Emit a DIE for a defining function declaration, and its children
   This is called for abstract instances, and for out-of-line instances
   (concrete and normal), but not for a concrete inlined instance.
 */

dwarf_output_function_instance(fn)
tree	fn;
{
	tree		return_type = function_return_type_or_null(fn);
	char		return_type_label[MAX_ARTIFICIAL_LABEL_BYTES];
	char		abstract_instance_label[MAX_ARTIFICIAL_LABEL_BYTES];
	tree		origin = decl_ultimate_origin(fn);
	char		*name = IDENTIFIER_POINTER(DECL_NAME(fn));
	int		external = TREE_PUBLIC(fn) ? 1 : 0;
	int		prototyped = TYPE_ARG_TYPES(TREE_TYPE(fn)) ? 1 : 0;
	tree		outer_scope;
	tree		arg_decls;
	tree		last_arg;
	enum abbrev_code abbrev;
	char		inline_attr_symbol[MAX_ARTIFICIAL_LABEL_BYTES];
	char		*low_pc;
	char		high_pc[MAX_ARTIFICIAL_LABEL_BYTES];
	char		sib_label[MAX_ARTIFICIAL_LABEL_BYTES];
	int		has_ellipsis;
	int		attr_index;
	unsigned long	attr_id;

	/* Determine if this is an abstract instance, a concrete out-of-line
	   instance, or a "normal" out-of-line function.
	 */

	(void) ASM_GENERATE_INTERNAL_LABEL(inline_attr_symbol,
				INLINE_ATTR_LABEL_CLASS, DECL_UID(fn));

	if (DECL_ABSTRACT(fn))
	{
		abbrev = return_type ? DW_ABBREV_ABSTRACT_FUNC_DEFN
				     : DW_ABBREV_VOID_ABSTRACT_FUNC_DEFN;
		/* We don't yet know whether this function will actually get
		   inlined anywhere, but we must specify so now.  We do this
		   by emitting an as-yet undefined symbol for the DW_AT_inline
		   value.  At the end of compilation, we emit a .set to define
		   this symbol to the correct value, which depends on whether
		   the function was actually inlined.
		 */
		dwarf_set_function_inline_attribute(fn, 1);
	}
	else if (origin)
		abbrev = DW_ABBREV_CONCRETE_OUT_OF_LINE_FUNC;
	else
	{
		abbrev = return_type ? DW_ABBREV_FUNC_DEFN
				     : DW_ABBREV_VOID_FUNC_DEFN;
	}

	if (return_type)
		determine_type_die_label(return_type, return_type_label, 0, 0);

	if (DECL_ABSTRACT(fn))
	{
		low_pc = NULL;
		high_pc[0] = '\0';
	}
	else
	{
		rtx	x = DECL_RTL(fn);

		low_pc = XSTR(XEXP(x, 0), 0);
		(void) sprintf(high_pc, FUNC_END_LABEL_FMT,
					current_funcdef_number);
	}

	(void) ASM_GENERATE_INTERNAL_LABEL(abstract_instance_label,
			DECL_DIE_LABEL_CLASS,
			origin ? DECL_UID(origin) : DECL_UID(fn));

	/* Emit a label at the start of an abstract instance DIE.
	   This label will be referenced via the DW_AT_abstract_origin
	   attribute of concrete instances of the function.
	 */

	(void) fputc('\n', asm_out_file);
	if (DECL_ABSTRACT(fn))
		ASM_OUTPUT_LABEL(asm_out_file, abstract_instance_label);

	/* Now output the attribute values in the order specified
	   by the abbreviation. */

	dwarf_output_abbreviation(abbrev);

	sib_label[0] = '\0';

	for (attr_index = 0;
	     (attr_id = XABBREV_ATTR(&abbrev_table[abbrev], attr_index)) != 0;
	     attr_index++)
	{
		switch (attr_id)
		{
		case DW_AT_name:
			dwarf_output_string(name, NO_COMMENT);
			break;

			/* The file, line and column attributes are always
			   grouped together.  Emit all of them when we see
			   the file attribute.
			 */
		case DW_AT_decl_file:
			dwarf_output_source_coordinates(fn);
			break;
		case DW_AT_decl_line:
			break;
		case DW_AT_decl_column:
			break;

		case DW_AT_abstract_origin:
			dwarf_output_ref4(abstract_instance_label,
						"DW_AT_abstract_origin");
			break;
		case DW_AT_type:
			dwarf_output_ref4(return_type_label, "return type");
			break;
		case DW_AT_external:
			dwarf_output_flag(external, "DW_AT_external");
			break;
		case DW_AT_inline:
			dwarf_output_data1_symbolic(inline_attr_symbol,
							"DW_AT_inline");
			break;
		case DW_AT_prototyped:
			dwarf_output_flag(prototyped, "DW_AT_prototyped");
			break;
		case DW_AT_low_pc:
			dwarf_output_addr(low_pc, "DW_AT_low_pc");
			break;
		case DW_AT_high_pc:
			dwarf_output_addr(high_pc, "DW_AT_high_pc");
			break;
		case DW_AT_sibling:
			dwarf_output_sibling_attribute(sib_label);
			break;

		default:
			abort();
		}
	}

	/* Now output descriptions of the arguments for this function.
	   Note that the DECL_ARGUMENT list for a FUNCTION_DECL does not
	   indicate if there was a trailing `...' at the end of the formal
	   parameter list.  In order to find this out, we must instead look
	   at the FUNCTION_TYPE associated with the FUNCTION_DECL.  If the
	   chain of type nodes hanging off of this FUNCTION_TYPE node ends
	   with a void_type_node then there should *not* be an ellipsis at
	   the end.
	 */

	arg_decls = DECL_ARGUMENTS(fn);
	last_arg = NULL;

	if (arg_decls && TREE_CODE(arg_decls) != ERROR_MARK)
		last_arg = tree_last(arg_decls);

	/* Generate DIEs to represent all known formal parameters, unless this
	   is a varargs function.
	   A function is a varargs function if and only if its last
	   named argument is named `__builtin_va_alist'.
	 */

	if (!last_arg || !DECL_NAME(last_arg)
		      || strcmp(IDENTIFIER_POINTER(DECL_NAME(last_arg)),
				"__builtin_va_alist"))
	{
		tree	parm;

		for (parm = arg_decls; parm; parm = TREE_CHAIN(parm))
			if (TREE_CODE(parm) == PARM_DECL)
				dwarf_output_decl(parm, fn);
	}

	/* Now try to decide if we should put an ellipsis at the end. */

	has_ellipsis = TRUE;

	if (prototyped)
	{
		/* A prototyped function definition has an ellipsis if and
		   only if the list of formal argument types does not end
		   with a void_type_node.
		 */

		tree	last = tree_last(TYPE_ARG_TYPES(TREE_TYPE(fn)));

		if (TREE_VALUE(last) == void_type_node)
			has_ellipsis = FALSE;
	}
	else if (!arg_decls)
		has_ellipsis = FALSE;	/* no args implies (void) */
	else
	{
		/* We have a non-prototyped function definition which declares
		   one or more formal parameters.
		   Such a function needs an ellipsis if and only if it is a
		   varargs function, which is equivalent to asking if its first
		   formal parameter is named "__builtin_va_alist".
		 */

		if (!DECL_NAME(arg_decls)
		    || strcmp(IDENTIFIER_POINTER(DECL_NAME(arg_decls)),
				"__builtin_va_alist"))
			has_ellipsis = FALSE;
	}

	if (has_ellipsis)
	{
		/* We never inline a function that takes a variable number of
		   arguments, so there should never be a concrete inline or
		   out-of-line instance of "...".
		   If we ever do inline such functions, we'll need some
		   convention for naming the label on the abstract instance
		   DW_TAG_unspecified_parameters DIE.  Our normal method uses
		   a decl's DECL_UID, but there is no _DECL node for "...".
		   Perhaps we could use the FUNCTION_DECL's DECL_UID in
		   conjunction with yet another label prefix.
		 */

		assert(origin == NULL);
		dwarf_output_abbreviation(DW_ABBREV_UNSPECIFIED_PARAMETERS);
	}

	/* Output DIEs for all of the stuff within the body of the function. */

	outer_scope = DECL_INITIAL(fn);

	/* Note that `outer_scope' is a pointer to the outermost BLOCK node
	   created to represent a function.  This outermost BLOCK actually
	   represents the outermost binding contour for the function, i.e.
	   the contour in which the function's formal parameters and labels
	   get declared.

	   It appears that the front end doesn't actually put the PARM_DECL
	   nodes for the current function onto the BLOCK_VARS list for this
	   outer scope.  (They are strung off of the DECL_ARGUMENTS list for
	   the function instead.)  The BLOCK_VARS list for the `outer_scope'
	   does provide us with a list of the LABEL_DECL nodes for the function
	   however, and we output DWARF info for those here.

	   Just within the `outer_scope' there will be another BLOCK node
	   representing the function's outermost pair of curly braces.
	   We must not generate a lexical_block DIE for this outermost pair
	   of curly braces because that is not really an independent scope
	   according to ANSI C rules.  Rather, it is the same scope in which
	   the parameters were declared.
	 */

	if (!TARGET_CAVE && TREE_CODE(outer_scope) != ERROR_MARK)
	{
		tree	a_label = BLOCK_VARS(outer_scope);

		for (; a_label; a_label = TREE_CHAIN(a_label))
			dwarf_output_decl(a_label, outer_scope);

		/* Note here that `BLOCK_SUBBLOCKS(outer_scope)' points to a
		   list of BLOCK nodes which is always only one element long.
		   That one element represents the outermost pair of curly
		   braces for the function body.
		 */

		dwarf_output_decls_for_scope(BLOCK_SUBBLOCKS(outer_scope));

		/* Finally, force out any pending types which are local to
		   the outermost block of this function definition.  These
		   will all have a TYPE_CONTEXT which points to the
		   FUNCTION_DECL node itself.
		 */

		dwarf_output_pending_types_for_scope(fn);
	}

	{
		char	comment[64];
		(void) sprintf(comment,
				"end function children %d", DECL_UID(fn));
		dwarf_output_null_die(comment);
	}

	if (sib_label[0])
		ASM_OUTPUT_LABEL(asm_out_file, sib_label);

	/* The decl location tracking for this function is no longer needed. */
	dwarf_free_avail_locations();
}

/* Output a DIE to represent a formal parameter decl.

   The _DECL argument may be either a PARM_DECL or perhaps a VAR_DECL which
   represents an inlining of some PARM_DECL.
 */

static void
dwarf_output_formal_parameter_die(decl)
tree	decl;
{
	tree	origin = decl_ultimate_origin(decl);
	enum abbrev_code	abbrev;
	rtx	rtl;
	int	cls;

	if (DECL_ABSTRACT(decl))
		abbrev = DW_ABBREV_ABSTRACT_PARAMETER;
	else
	{
		int	i, num_locs = 0;
		int	alid = DECL_DWARF_FIRST_ALID(decl);

		if (alid > 0)
		{
		  for (; alid <= dwarf_max_alid
			 && avail_locations[alid].decl == decl
			 && (rtl = avail_locations[alid].rtl) != NULL_RTX;
			 alid++)
		  {
		    cls = dwarf_classify_location(rtl);

		    for (i=0; i <= avail_locations[alid].max_bound_index; i+=2)
		    {
		      dwarf_add_location_expression(rtl, cls,
					TREE_TYPE(decl),
					avail_locations[alid].bounds[i],
					avail_locations[alid].bounds[i+1]);
		      num_locs++;
		    }
		  }
		}

		if (num_locs == 1 && cls == GDW_LE_CONST_VALUE)
			abbrev = origin
				? DW_ABBREV_CONCRETE_PARAMETER_CONSTANT
				: DW_ABBREV_PARAMETER_CONSTANT;
		else
			abbrev = origin
				? DW_ABBREV_CONCRETE_PARAMETER
				: DW_ABBREV_PARAMETER;
	}

	dwarf_output_common_decl_attributes(abbrev, decl,
						origin,
			/* low_pc => */		(char*) NULL,
			/* high_pc => */	(char*) NULL);
}

/* Output a DIE to represent a declared file-scope data object.
 */

static void
dwarf_output_global_variable_die(decl)
tree	decl;
{
	enum abbrev_code	abbrev;
	rtx			rtl;
	int			cls;

	rtl = eliminate_regs(DECL_RTL(decl), 0, NULL_RTX);
	cls = dwarf_classify_location(rtl);

	abbrev = DECL_EXTERNAL(decl)
			? DW_ABBREV_GLOBAL_VAR_DECL
			: ((cls == GDW_LE_CONST_VALUE)
				? DW_ABBREV_GLOBAL_VAR_CONSTANT
				: DW_ABBREV_GLOBAL_VAR_DEFN);

	/* Make the _DECL's location available if needed */
	dwarf_add_location_expression(DECL_RTL(decl), cls, TREE_TYPE(decl),
						NULL_RTX, NULL_RTX);

	dwarf_output_common_decl_attributes(abbrev, decl,
			/* origin => */		NULL_TREE,
			/* low_pc => */		(char*) NULL,
			/* high_pc => */	(char*) NULL);
}

/* Output a DIE to represent a declared local variable data object.
 */

static void
dwarf_output_local_variable_die(decl)
tree	decl;
{
	tree	origin = decl_ultimate_origin(decl);
	enum abbrev_code	abbrev;
	rtx	rtl;
	int	cls;

	if (DECL_EXTERNAL(decl))
	{
		/* We have a local extern variable declaration, e.g.
			extern int x;
		   Since this decl has no location, an abstract and normal
		   instance contain the same attributes.  One abbreviation,
		   DW_ABBREV_CONCRETE_LOCAL_VAR_DECL, serves for both an
		   abstract instance die and a normal die.
		   A concrete instance for this decl simply references
		   the abstract instance die.
		 */
		abbrev = origin
			? DW_ABBREV_CONCRETE_LOCAL_VAR_DECL
			: DW_ABBREV_LOCAL_VAR_DECL;
	}
	else if (DECL_ABSTRACT(decl))
		abbrev = DW_ABBREV_ABSTRACT_LOCAL_VAR;
	else
	{
		int	i, num_locs = 0;
		int	alid = DECL_DWARF_FIRST_ALID(decl);

		if (alid > 0)
		{
		  for (; alid <= dwarf_max_alid
			 && avail_locations[alid].decl == decl
			 && (rtl = avail_locations[alid].rtl) != NULL_RTX;
			 alid++)
		  {
		    cls = dwarf_classify_location(rtl);

		    for (i=0; i <= avail_locations[alid].max_bound_index; i+=2)
		    {
		      dwarf_add_location_expression(rtl, cls,
					TREE_TYPE(decl),
					avail_locations[alid].bounds[i],
					avail_locations[alid].bounds[i+1]);
		      num_locs++;
		    }
		  }
		}

		if (num_locs == 1 && cls == GDW_LE_CONST_VALUE)
			abbrev = origin
				? DW_ABBREV_CONCRETE_LOCAL_VAR_CONSTANT
				: DW_ABBREV_LOCAL_VAR_CONSTANT;
		else
			abbrev = origin
				? DW_ABBREV_CONCRETE_LOCAL_VAR
				: DW_ABBREV_LOCAL_VAR;
	}

	/* If this is a static local that goes into either .data or .bss,
	   then remember that we must emit a .debug_aranges entry for
	   .data or .bss.  Don't worry about .text; since we're emitting
	   a local variable, we must be emitting a function body and so
	   an arange for .text will be emitted.
	 */
	if (!DECL_EXTERNAL(decl) && !DECL_ABSTRACT(decl) && !origin
	    && TREE_STATIC(decl) && !TREE_READONLY(decl))
	{
		if (DECL_INITIAL(decl))
			dwarf_emit_data_arange = 1;
		else
			dwarf_emit_bss_arange = 1;
	}

	dwarf_output_common_decl_attributes(abbrev, decl,
						origin,
			/* low_pc => */		(char*) NULL,
			/* high_pc => */	(char*) NULL);
}

/* Compute the location attribute values needed in the DIE for a bit field.

   To understand the location information for bit fields, for both big
   and little endian, consider the following struct.

	struct {
	    char	ch[7];
	    unsigned	b5:5;
	    unsigned	b6:6;
	    unsigned	b7:7;
	};

   For each bit field, we need to specify four pieces of information:

	1) The byte offset of the bit field "container".
	2) The size, in bytes, of the bit field "container".
	3) The size, in bits, of the bit field.
	4) The offset, in bits, of the MSB of the bit field from the
	   MSB of the "container".  (For example, if 'b5' resides in
	   the least significant 5 bits of a 32-bit container, its bit
	   offset would be 27.)

   To simplify matters, we will always choose a container size (item 2)
   of one word (32 bits, or 4 bytes).  Also, for item 3, the bit field size
   is simply the number of bits in the bit field declaration.  The tricky
   part, then, is to determine items (1) and (4).  The method for determining
   these values is illustrated below.

   On a little endian machine with 32-bit words, the bits in the example struct
   would be laid out as shown in the table below.  Consecutive words in
   the struct are at addresses A, A+4, A+8 and A+12.  Consecutive bytes in
   the struct are at addresses A+0, A+1, A+2, etc.  The table shows all bits
   in the struct, with the most significant bit to the left.  The first 7 bytes
   are allocated to the member 'ch', and are shown below as zeroes.  The bits
   marked '5' are where the member 'b5' resides.  Similarly, b6 and b7 reside
   in the bits marked '6' and '7'.  In each case, the most significant bit of
   the bit field is on the "left" end of the bit field.

                               Byte Within Word
                   +---------------------------------------+
                   |       A+3      A+2      A+1      A+0  |
            A:     |  00000000 00000000 00000000 00000000  |
                   +---------------------------------------+
                   |       A+7      A+6      A+5      A+4  |
            A+4:   |  00055555 00000000 00000000 00000000  |
Address:           +---------------------------------------+
                   |      A+11     A+10      A+9      A+8  |
            A+8:   |  00000000 00000000 00077777 77666666  |
                   +---------------------------------------+
                   |      A+15     A+14     A+13     A+12  |
            A+12:  |  00000000 00000000 00000000 00000000  |
                   +---------------------------------------+

   Now, for purposes of debugging information, any four byte word that
   completely contains a bit field, can be specified as the container for
   the bit field.  For example, we could say that 'b5's container word starts
   at byte offset 7, and that b5 has an MSB bit offset of 27 (represented more
   succinctly as <7,27>).  Alternately, we could use <6,19>, <5,11> or <4,3>.
   Similarly, for 'b6' we could use any of the values <8,26>, <7,18>, <6,10>
   or <5,2>, and for b7 we could use <8,19>, <7,11> or <6,3>.  To simplify
   our calculations, we will choose the container that places the bit field
   closest to the least significant bytes of the container.  Put another way,
   we will choose the option with the largest bit offset.  Thus, for b5 we
   would choose <7,27>, for b6 <8,26>, and for b7 <8,19>.

   The picture for big-endian target memory is a little difficult to
   comprehend, and is illustrated below.  Be aware that the bytes within
   each word are swapped when the word is loaded from memory into a register.
   For example, though the bits for field 'b7' appear to be non-contiguous,
   they will become contiguous when the bytes at addresses A+9 and A+8
   are loaded into a register.  The MSB of b7 resides in the byte at A+8.
   The MSB of b5 is the leftmost bit in the byte at A+7, and the MSB
   of b6 is the leftmost bit in the byte at A+8.

                               Byte Within Word
                   +---------------------------------------+
                   |       A+3      A+2      A+1      A+0  |
            A:     |  00000000 00000000 00000000 00000000  |
                   +---------------------------------------+
                   |       A+7      A+6      A+5      A+4  |
            A+4:   |  55555000 00000000 00000000 00000000  |
Address:           +---------------------------------------+
                   |      A+11     A+10      A+9      A+8  |
            A+8:   |  00000000 00000000 77777000 66666677  |
                   +---------------------------------------+
                   |      A+15     A+14     A+13     A+12  |
            A+12:  |  00000000 00000000 00000000 00000000  |
                   +---------------------------------------+

   As with little endian we will always use a 32-bit container, and there
   is a choice of where this container begins.  For b5, b6 and b7 we have
   the following choices of <container byte offset, MSB bit offset> pairs:

	b5:	<4,24>  <5,16>  <6,8>   <7,0>
	b6:	<5,24>  <6,16>  <7,8>   <8,0>
	b7:	        <6,22>  <7,14>  <8,6>

   Note that the DECL_FIELD_BITPOS value for a bit field FIELD_DECL will be
   the same regardless of endianness.  To simplify our calculations, for
   big-endian memory we will choose the container with the smallest
   "bit offset" component.  The choice is really arbitrary, but this choice
   allows the same "container determination" computation to be
   used for both big and little endian.
*/

static void
determine_bit_field_location_attributes(decl, ret_bit_size, ret_bit_offset,
					ret_byte_size, ret_byte_offset)
tree		decl;
unsigned long	*ret_bit_size;
unsigned long	*ret_bit_offset;
unsigned long	*ret_byte_size;
unsigned long	*ret_byte_offset;
{
	int	absolute_bit_offset;	/* From beginning of struct. */
	int	bit_offset;
	int	bit_size;
	int	container_bit_size;	/* A multiple of BITS_PER_UNIT */
	int	container_bit_offset;	/* A multiple of BITS_PER_UNIT */

	bit_size = TREE_INT_CST_LOW(DECL_SIZE(decl));

	if (TREE_CODE(DECL_FIELD_BITPOS(decl)) == INTEGER_CST)
		absolute_bit_offset = TREE_INT_CST_LOW(DECL_FIELD_BITPOS(decl));
	else
		absolute_bit_offset = 0;

	/* Find the size and offset of a container around the object.  */

	assert(bit_size <= BITS_PER_WORD);
	container_bit_size = BITS_PER_WORD;
	container_bit_offset = (absolute_bit_offset / BITS_PER_UNIT)
				* BITS_PER_UNIT;

	/* Find the bit offset within the container. */

	if (dwarf_target_big_endian)
		bit_offset = (absolute_bit_offset - container_bit_offset);
	else
		bit_offset = container_bit_size - bit_size
			- (absolute_bit_offset - container_bit_offset);

	*ret_bit_size = bit_size;
	*ret_bit_offset = bit_offset;
	*ret_byte_size = container_bit_size/BITS_PER_UNIT;
	*ret_byte_offset = container_bit_offset/BITS_PER_UNIT;
}

static void
dwarf_output_data_member_die(decl)
tree	decl;
{
	char		*name;
	char		type_label[MAX_ARTIFICIAL_LABEL_BYTES];
	enum abbrev_code abbrev;
	unsigned long	byte_offset, byte_size, bit_offset, bit_size;
	unsigned long	attr_id;
	int		attr_index;

	if (TREE_CODE(decl) == ERROR_MARK)
		return;

	/* First gather all the potential attribute values. */

	name = (DECL_NAME(decl) && IDENTIFIER_POINTER(DECL_NAME(decl)))
			? IDENTIFIER_POINTER(DECL_NAME(decl))
			: "";

	determine_type_die_label(DECL_BIT_FIELD_TYPE(decl)
					? DECL_BIT_FIELD_TYPE(decl)
					: TREE_TYPE(decl),
				  type_label,
				  TREE_READONLY(decl),
				  TREE_THIS_VOLATILE(decl));

	/* byte_offset is needed to output the DW_AT_data_member_location
	   attribute (for struct members only, not union members).

	   byte_size, bit_size and bit_offset are needed only
	   for bit field members.
	 */

	byte_offset = byte_size = bit_offset = bit_size = 0;

	if (DECL_BIT_FIELD_TYPE(decl))
	{
		abbrev = DW_ABBREV_BIT_FIELD_MEMBER;
		determine_bit_field_location_attributes(decl,
			&bit_size, &bit_offset, &byte_size, &byte_offset);
	}
	else if (TREE_CODE(DECL_CONTEXT(decl)) == RECORD_TYPE)
	{
		abbrev = DW_ABBREV_STRUCT_DATA_MEMBER;
		if (TREE_CODE(DECL_FIELD_BITPOS(decl)) == INTEGER_CST)
			byte_offset = TREE_INT_CST_LOW(DECL_FIELD_BITPOS(decl))
					/ BITS_PER_UNIT;
		else
			byte_offset = 0;
	}
	else
		abbrev = DW_ABBREV_UNION_DATA_MEMBER;

	/* Now output the attribute values in the order specified
	   by the abbreviation. */

	(void) fputc('\n', asm_out_file);
	dwarf_output_abbreviation(abbrev);

	for (attr_index = 0;
	     (attr_id = XABBREV_ATTR(&abbrev_table[abbrev], attr_index)) != 0;
	     attr_index++)
	{
		switch (attr_id)
		{
		case DW_AT_name:
			dwarf_output_string(name, NO_COMMENT);
			break;

			/* The file, line and column attributes are always
			   grouped together.  Emit all of them when we see
			   the file attribute.
			 */
		case DW_AT_decl_file:
			dwarf_output_source_coordinates(decl);
			break;
		case DW_AT_decl_line:
			break;
		case DW_AT_decl_column:
			break;

		case DW_AT_type:
			dwarf_output_ref4(type_label, "member type");
			break;
		case DW_AT_byte_size:
			dwarf_output_udata(byte_size, "member size in bytes");
			break;
		case DW_AT_bit_size:
			dwarf_output_udata(bit_size, "member size in bits");
			break;
		case DW_AT_bit_offset:
			dwarf_output_udata(bit_offset, "member bit offset");
			break;
		case DW_AT_data_member_location:
			/* The location expression will be

				DW_OP_plus_uconst member_byte_offset

			   The location expression is represented using the
			   DW_FORM_block1 format, and will consist of one byte
			   holding the block's length (not including the length
			   byte itself), one byte holding the opcode
			   DW_OP_plus_uconst, and up to 5 bytes holding an
			   unsigned LEB128 number which is the member's offset
			   in bytes from the beginning of the struct/union.
			 */

			dwarf_output_data1(1 + udata_byte_length(byte_offset),
					"length of location expression");
			dwarf_output_loc_expr_opcode(DW_OP_plus_uconst,
						"DW_OP_plus_uconst");
			dwarf_output_udata(byte_offset, "member byte offset");
			break;
		default:
			abort();
		}
	}
}

/* Output a DIE for the given _TYPE node.

   If the type has a name and the name belongs to a scope other than the
   current scope, we don't output the type immediately.  Instead we pend
   the type.  It will eventually be output when we pop back out to its
   containing scope and call 'dwarf_output_pending_types_for_scope'.
 */

static void
dwarf_output_type(type, scope, decl_const, decl_volatile)
tree	type;
tree	scope;
int	decl_const, decl_volatile;
{
	char	const_type_label[MAX_ARTIFICIAL_LABEL_BYTES];
	char	vol_type_label[MAX_ARTIFICIAL_LABEL_BYTES];
	char	unqual_type_label[MAX_ARTIFICIAL_LABEL_BYTES];
	char	sib_label[MAX_ARTIFICIAL_LABEL_BYTES];
	tree	unqual_type;
	char	*tag_name;
	struct qtype_info	*qtype_info;
	enum abbrev_code	abbrev;

	if (!type || TREE_CODE(type) == ERROR_MARK)
		return;

	decl_const |= TYPE_READONLY(type);
	decl_volatile |= TYPE_VOLATILE(type);
	unqual_type = TYPE_MAIN_VARIANT(type);

	if (!unqual_type || TREE_CODE(unqual_type) == ERROR_MARK)
		return;

	/* Stop here if we've already emitted a DIE for this type */

	if (decl_const || decl_volatile)
	{
		qtype_info = lookup_or_add_qtype_info(TYPE_UID(unqual_type));

		if (decl_const && decl_volatile)
		{
			if (qtype_info->const_volatile_written)
				return;
		}
		else if (decl_volatile)
		{
			if (qtype_info->volatile_written)
				return;
		}
		else if (qtype_info->const_written)
			return;
	}
	else if (TREE_ASM_WRITTEN(unqual_type))
		return;

	/* Don't emit any DIEs for this type now unless it is OK to do so.
	   Note that DIEs for qualified types are always emitted in the same
	   scope as the unqualified type DIE.
	 */

	if (!is_type_ok_for_scope(unqual_type, scope))
	{
		pend_type(unqual_type, decl_const, decl_volatile);
		return;
	}

	/* Output a DIE for the unqualified version of the type, if not
	   already done.
	   Then output DW_TAG_const_type and DW_TAG_volatile_type DIEs
	   as needed that refer to the unqualified type DIE.
	 */

	determine_type_die_label(unqual_type, unqual_type_label, 0, 0);

	if (!TREE_ASM_WRITTEN(unqual_type))
	{
		int	unqual_type_written = 1;

		switch (TREE_CODE(unqual_type))
		{
		case ERROR_MARK:
			break;

		case POINTER_TYPE:
		    {
			char	base_type_label[MAX_ARTIFICIAL_LABEL_BYTES];
			tree	base_type = TREE_TYPE(unqual_type);

			/* Prevent emitting the pointer type DIE multiple times.
			   This could happen if the base_type is a struct which
			   has a member whose type is the same pointer type.
			 */

			TREE_ASM_WRITTEN(unqual_type) = 1;

			dwarf_output_type(base_type, scope, 0, 0);

			(void) fputc('\n', asm_out_file);
			ASM_OUTPUT_LABEL(asm_out_file, unqual_type_label);
			dwarf_output_abbreviation(DW_ABBREV_POINTER_TYPE);

			determine_type_die_label(base_type,
							base_type_label, 0, 0);
			dwarf_output_ref4(base_type_label, "type pointed to");
			break;
		    }

		case FUNCTION_TYPE:
			dwarf_output_type(TREE_TYPE(unqual_type), scope, 0, 0);
			dwarf_output_function_type_die(unqual_type);
			break;

		case ARRAY_TYPE:
			if (TYPE_STRING_FLAG(unqual_type)
			    && TREE_CODE(TREE_TYPE(unqual_type)) == CHAR_TYPE)
			{
				dwarf_output_type(TREE_TYPE(unqual_type),
						scope, 0, 0);
				abort();	/* No string type in C */
			}
			else
			{
				tree	element_type;

				element_type = TREE_TYPE(unqual_type);
				while (TREE_CODE(element_type) == ARRAY_TYPE)
					element_type = TREE_TYPE(element_type);

				/* In C, 'const' and 'volatile' apply to the
				   array element, not to the array as a whole.
				   So propagate decl_const/volatile to the
				   element type here.
				 */
				dwarf_output_type(element_type, scope,
						decl_const, decl_volatile);

				ASM_OUTPUT_LABEL(asm_out_file,
						unqual_type_label);
				dwarf_output_array_type_die(unqual_type,
						element_type,
						decl_const, decl_volatile);
			}
			break;

		case ENUMERAL_TYPE:
		case RECORD_TYPE:
		case UNION_TYPE:
		case QUAL_UNION_TYPE:
			/* According to comments in dwarfout.c, a non-file-
			   scope type with one of these TREE_CODE's will
			   never be "tentative" at this point.  The type is
			   either complete or incomplete.  If complete, we can
			   output it now.  If incomplete, the type will not be
			   completed within its scope because the front end has
			   already parsed its entire scope, so we can also
			   output it now.

			   For file-scope types with these TREE_CODES, an
			   incomplete type may yet be completed at some point
			   later in the source file.  For this case, we don't
			   output the DIE yet.  We output such types after the
			   entire source file has been seen, when toplev
			   calls dwarfout_file_scope_decl with a null-named
			   TYPE_DECL for the type, and set_finalizing == 1.
			   However, if this type is qualified (const/volatile)
			   we must output the const or volatile type DIE now,
			   since the qualified _TYPE might not come through
			   here again.
			 */

			if (TYPE_SIZE(unqual_type) == 0 && !finalizing
			    && TYPE_CONTEXT(unqual_type) == 0)
			{
				/* Avoid setting TREE_ASM_WRITTEN.  */
				unqual_type_written = 0;
				break;
			}

			/* Prevent infinite recursion in cases where the type
			   of some member of this type is expressed in terms
			   of this type itself.
			 */

			TREE_ASM_WRITTEN(unqual_type) = 1;

			(void) fputc('\n', asm_out_file);
			ASM_OUTPUT_LABEL(asm_out_file, unqual_type_label);
			tag_name = type_tag_name(unqual_type);

			/* Handle incomplete types first, they're easy. */

			if (TYPE_SIZE(unqual_type) == NULL)
			{
				assert(tag_name);

				abbrev = DW_ABBREV_INCOMPLETE_UNION;
				if (TREE_CODE(unqual_type) == ENUMERAL_TYPE)
					abbrev = DW_ABBREV_INCOMPLETE_ENUM;
				else if (TREE_CODE(unqual_type) == RECORD_TYPE)
					abbrev = DW_ABBREV_INCOMPLETE_STRUCT;

				dwarf_output_abbreviation(abbrev);
				dwarf_output_string(tag_name, NO_COMMENT);
				dwarf_output_flag(1, "DW_AT_declaration");
				break;
			}

			if (TREE_CODE(unqual_type) == ENUMERAL_TYPE)
			{
				dwarf_output_enumeration_type_die(unqual_type);
				break;
			}

			/* Output a DIE to represent the struct or union.  */

			if (TREE_CODE(unqual_type) == RECORD_TYPE)
				abbrev = tag_name ? DW_ABBREV_NAMED_STRUCT
						  : DW_ABBREV_ANON_STRUCT;
			else
				abbrev = tag_name ? DW_ABBREV_NAMED_UNION
						  : DW_ABBREV_ANON_UNION;

			dwarf_output_abbreviation(abbrev);
			dwarf_output_sibling_attribute(sib_label);
			if (tag_name)
				dwarf_output_string(tag_name, NO_COMMENT);
			dwarf_output_udata(
				(unsigned long) int_size_in_bytes(type),
							"size in bytes");

			/* Output DIEs for the struct or union's members.  */

			{
				tree	field = TYPE_FIELDS(unqual_type);

				for (; field; field = TREE_CHAIN(field))
					dwarf_output_decl(field, unqual_type);
			}

			/* Mark the end of this _TYPE's children. */
			dwarf_output_null_die("end members of aggregate");
			ASM_OUTPUT_LABEL(asm_out_file, sib_label);

			break;

		case BOOLEAN_TYPE:
		case CHAR_TYPE:
		case COMPLEX_TYPE:
		case INTEGER_TYPE:
		case REAL_TYPE:
		case VOID_TYPE:
			dwarf_output_base_type_die(unqual_type);
			break;

		case REFERENCE_TYPE:
		case OFFSET_TYPE:
		case SET_TYPE:
		case FILE_TYPE:
		case METHOD_TYPE:
		case LANG_TYPE:
		default:
			/* These types aren't used in C */
			abort();
		}

		if (unqual_type_written)
			TREE_ASM_WRITTEN(unqual_type) = 1;
	} /* end if ! TREE_ASM_WRITTEN(unqual_type) */

	/* Now output the qualified types.
	   Note that emitting the unqualified type above may have caused
	   these qualified types to be emitted.  Consider the following:

		struct s {
			struct s * const cp_member;
		} * const cp_var;

	   When emitting the type of cp_var, "const pointer to struct s", we

	   	(1) set TREE_ASM_WRITTEN in the unqualified type
		    (pointer to struct s), and call dwarf_output_type
		    recursively to emit struct s.  This is done above
		    when emitting a POINTER_TYPE.
		(2) emit the unqualified pointer type (above).
		(3) emit the const-qualified pointer type (below).

	   Note that the recursive call in step (1) encounters the type
	   "const pointer to struct s" when emitting the type of cp_member.
	   So in step 3, there is no work left.  We must avoid emitting
	   the const-qualified type again, or a multiple declaration will
	   result.
	 */

	if (decl_volatile)
	{
		(void) sprintf(vol_type_label, TYPE_DIE_LABEL_FMT,
					"v", TYPE_UID(unqual_type));
		if (!qtype_info->volatile_written)
		{
			(void) fputc('\n', asm_out_file);
			ASM_OUTPUT_LABEL(asm_out_file, vol_type_label);
			dwarf_output_abbreviation(DW_ABBREV_VOLATILE_TYPE);
			dwarf_output_ref4(unqual_type_label, "type reference");
			qtype_info->volatile_written = 1;
		}
	}

	if (decl_const)
	{
		determine_type_die_label(type, const_type_label,
						decl_const, decl_volatile);

		if (decl_volatile && !qtype_info->const_volatile_written)
		{
			(void) fputc('\n', asm_out_file);
			ASM_OUTPUT_LABEL(asm_out_file, const_type_label);
			dwarf_output_abbreviation(DW_ABBREV_CONST_TYPE);
			dwarf_output_ref4(vol_type_label, "type reference");
			qtype_info->const_volatile_written = 1;
		}
		else if (!decl_volatile && !qtype_info->const_written)
		{
			(void) fputc('\n', asm_out_file);
			ASM_OUTPUT_LABEL(asm_out_file, const_type_label);
			dwarf_output_abbreviation(DW_ABBREV_CONST_TYPE);
			dwarf_output_ref4(unqual_type_label, "type reference");
			qtype_info->const_written = 1;
		}
	}
}

/* Output a DW_TAG_lexical_block DIE followed by DIEs to represent all of
   the things which are local to the given block.
 */

static void
dwarf_output_block(stmt)
tree	stmt;
{
	int	must_output_die = 0;
	tree	origin;
	enum tree_code	origin_code;

	/* Ignore blocks never really used to make RTL.  */

	if (!stmt || !TREE_USED(stmt))
		return;

	/* Determine the "ultimate origin" of this block.  It may be an
	   inlined instance of an inlined instance of an inline function,
	   so we have to trace all of the way back through the origin chain
	   to find out what sort of node actually served as the original seed
	   for the creation of the current block.
	 */

	origin = block_ultimate_origin(stmt);
	origin_code = (origin != NULL) ? TREE_CODE(origin) : ERROR_MARK;

	/* Don't emit anything for an "abstract inline" function.
	   This is the case where 'stmt' represents a function F at the
	   point it is inlined into some function G, and we are currently
	   emitting the abstract instance tree for G.
	 */

	if (origin_code == FUNCTION_DECL && BLOCK_ABSTRACT(stmt))
		return;

	/* Determine if we need to output any DWARF DIEs at all to
	   represent this block.
	 */

	if (origin_code == FUNCTION_DECL)
		/* The outer scopes for inlinings *must* always be represented.
		   We generate DW_TAG_inlined_subroutine DIEs for them.
		 */
		must_output_die = 1;
	else if (origin == NULL || !is_body_block (origin))
	{
		/* In the case where the current block represents an inlining
		   of the "body block" of an inline function, we must *NOT*
		   output any DIE for this block because we have already output
		   a DIE to represent the whole inlined function scope and the
		   "body block" of any function doesn't really represent a
		   different scope according to ANSI C rules.  So we check
		   here to make sure that this block does not represent a
		   "body block inlining" before trying to set the
		   `must_output_die' flag.
		 */

		must_output_die = (BLOCK_VARS(stmt) != NULL);
	}

	/* It would be a waste of space to generate a DWARF DW_TAG_lexical_block
	   DIE for any block which contains no significant local declarations
	   at all.  Rather, in such cases we just call `output_decls_for_scope'
	   so that any needed DWARF info for any sub-blocks will get properly
	   generated.  Note that in terse mode, our definition of what
	   constitutes a "significant" local declaration gets restricted to
	   include only inlined function instances and local (nested) function
	   definitions.
	 */

	if (!must_output_die)
		dwarf_output_decls_for_scope(stmt);
	else
	{
		char	low_pc[MAX_ARTIFICIAL_LABEL_BYTES];
		char	high_pc[MAX_ARTIFICIAL_LABEL_BYTES];
		char	sib_label[MAX_ARTIFICIAL_LABEL_BYTES];

		(void) sprintf(low_pc,BLOCK_BEGIN_LABEL_FMT,next_block_number);
		(void) sprintf(high_pc,BLOCK_END_LABEL_FMT, next_block_number);

		if (origin_code == FUNCTION_DECL)
		{
			/* Emit a DW_TAG_inlined_subroutine DIE. */
#if 0
			/* Don't represent an "abstract inline" function */
			if (BLOCK_ABSTRACT(stmt))
				dwarf_output_abbreviation(DW_ABBREV_ABSTRACT_INLINED_FUNC);
			else
#endif
				dwarf_output_abbreviation(DW_ABBREV_CONCRETE_INLINED_FUNC);
			dwarf_output_sibling_attribute(sib_label);
			dwarf_output_decl_abstract_origin_attribute(origin);

			if (!BLOCK_ABSTRACT(stmt))
			{
				dwarf_output_addr(low_pc, "low pc");
				dwarf_output_addr(high_pc, "high pc");
			}
		}
		else
		{
			/* Emit a DW_TAG_lexical_block DIE. */

			if (BLOCK_ABSTRACT(stmt))
			{
				dwarf_output_abbreviation(DW_ABBREV_ABSTRACT_BLOCK);
				dwarf_output_sibling_attribute(sib_label);
			}
			else
			{
				dwarf_output_abbreviation(DW_ABBREV_BLOCK);
				dwarf_output_sibling_attribute(sib_label);
				dwarf_output_addr(low_pc, "low pc");
				dwarf_output_addr(high_pc, "high pc");
			}
		}

		dwarf_output_decls_for_scope(stmt);

		dwarf_output_null_die("end block decls");
		ASM_OUTPUT_LABEL(asm_out_file, sib_label);
	}
}

/* Output all of the decls declared within a given scope (also called
   a `binding contour') and (recursively) all of it's sub-blocks.
 */

static void
dwarf_output_decls_for_scope(stmt)
tree	stmt;
{
	tree	decl;

	/* Ignore blocks never really used to make RTL.  */

	if (!stmt || !TREE_USED(stmt))
		return;

	if (!BLOCK_ABSTRACT(stmt))
		next_block_number++;

	/* Output the DIEs to represent all of the data objects, functions,
	   typedefs, and tagged types declared directly within this block
	   but not within any nested sub-blocks.
	 */

	for (decl = BLOCK_VARS(stmt); decl; decl = TREE_CHAIN(decl))
		dwarf_output_decl(decl, stmt);

	/* Output the DIEs to represent all sub-blocks (and the items declared
	   therein) of this block.
	 */

	for (decl = BLOCK_SUBBLOCKS(stmt); decl; decl = BLOCK_CHAIN(decl))
		dwarf_output_block(decl);

	dwarf_output_pending_types_for_scope(stmt);
}

/* Output DWARF .debug_info information for a decl described by DECL.  */

static void
dwarf_output_decl(decl, scope)
tree	decl;
tree	scope;
{
	char	pub_label[MAX_ARTIFICIAL_LABEL_BYTES];
		/* This label will be defined at the beginning of DIEs for
		   public decls, and has already been referenced in the
		   .debug_pubnames section.
		 */

	if (TREE_CODE(decl) == ERROR_MARK || DECL_IGNORED_P(decl))
		return;

	dwarf_clear_location_list();

	switch (TREE_CODE(decl))
	{
	case CONST_DECL:
		/* The enumerators of an enum type get output when we output
		   the relevant enum type itself.  */
		break;

	case FUNCTION_DECL:
	    {
		tree	return_type = TREE_TYPE(TREE_TYPE(decl));

		/* Make sure the function's return type has been described,
		   unless it's "void".
		 */

		if (return_type && TREE_CODE(return_type) != VOID_TYPE)
			dwarf_output_type(return_type, scope, 0, 0);


		if (DECL_INITIAL(decl) == NULL)
			dwarf_output_function_declaration_die(decl);
		else
		{
			/* Define a pubnames label for the DIE, if needed. */
			if (function_should_have_a_pubnames_entry(decl))
			{
				(void) ASM_GENERATE_INTERNAL_LABEL(pub_label,
						PUB_DIE_LABEL_CLASS,
						DECL_UID(decl));
				ASM_OUTPUT_LABEL(asm_out_file, pub_label);
			}

			dwarf_output_function_instance(decl);
		}
		break;
	    }

	case TYPE_DECL:
	    {
		/* A null-named TYPE_DECL node represents a tagged _TYPE.
		   The decl's TREE_TYPE is this tagged type.  If we have
		   a concrete instance of such a beast, we need to output
		   a special form of the tagged type.  For example, the
		   DWARF standard says that we can't emit DIEs for the
		   struct members of "concrete instance" struct _TYPEs.

		   NOTE:  I believe we could suppress these DIEs altogether.
		   As far as I can tell, they are never referenced from other
		   DIEs, and they don't provide any information that isn't
		   already available in the abstract instance DIE.  Their
		   emission is carried over from the DWARF 1 implementation.

		   Except for these special tagged type instance DIEs, we
		   never emit concrete inlined or out-of-line instance DIEs
		   for _TYPEs.  There is only one DIE emitted for a _TYPE,
		   we don't distinguish between abstract and concrete _TYPE
		   DIEs.  The DW_AT_type attribute of any DIE will never
		   reference these tagged type instance DIEs.
		 */

		tree	origin = decl_ultimate_origin(decl);

		if (!DECL_NAME(decl) && origin)
		{
			dwarf_output_tagged_type_instantiation(decl);
			break;
		}

		/* Only emit DIEs for TYPE_DECLs that have a name.
		   Go ahead and emit the underlying _TYPE DIE regardless.
		   This is necessary to force out DIEs for some incomplete
		   struct tags, which would otherwise go un-emitted leading
		   to potential dangling reference problems.  For example,
		   this is where the DIE for "struct unknown" is emitted
		   in the following program:

			struct one { struct unknown *p; };
			struct two { struct one *q; };
			void main() { extern struct two *tp; }
		 */

		dwarf_output_type(TREE_TYPE(decl), scope,
			TREE_READONLY(decl), TREE_THIS_VOLATILE(decl));

		if (DECL_NAME(decl))
		{
			enum abbrev_code	abbrev;
			abbrev = origin ? DW_ABBREV_CONCRETE_TYPEDEF
					: DW_ABBREV_TYPEDEF;

			dwarf_output_common_decl_attributes(abbrev, decl,
						origin,
			/* low_pc => */		(char*) NULL,
			/* high_pc => */	(char*) NULL);
		}

		break;
	    }

	case LABEL_DECL:
	    {
		char	low_pc[MAX_ARTIFICIAL_LABEL_BYTES];
		rtx	insn = DECL_RTL(decl);
		tree	origin = decl_ultimate_origin(decl);
		enum abbrev_code	abbrev;

		if (debug_info_level != DINFO_LEVEL_NORMAL
		    && debug_info_level != DINFO_LEVEL_VERBOSE)
			break;

		/* Don't emit debug info for an unnamed label, unless this
		   label is a concrete instance and the abstract instance
		   has a name.
		 */
		if (!DECL_NAME(decl) || !IDENTIFIER_POINTER(DECL_NAME(decl)))
		{
			if (!origin || TREE_CODE(origin) != LABEL_DECL
			    || !DECL_NAME(origin)
			    || !IDENTIFIER_POINTER(DECL_NAME(origin)))
				break;
		}

		/* We currently can't trust optimizations to retain
		   user declared labels in the insn stream.  This leads
		   to unresolved DWARF references for the low_pc value
		   of a label DIE.  So, if optimization is enabled, assume
		   the label has been deleted.
		 */

		if (optimize || (insn && GET_CODE(insn) != CODE_LABEL))
			insn = NULL;

		if (DECL_ABSTRACT(decl))
			abbrev = DW_ABBREV_ABSTRACT_LABEL;
		else if (origin)
			abbrev = insn ? DW_ABBREV_CONCRETE_LABEL
				      : DW_ABBREV_CONCRETE_DELETED_LABEL;
		else
			abbrev = insn ? DW_ABBREV_LABEL
				      : DW_ABBREV_DELETED_LABEL;

		if (insn)
		{
			assert(!INSN_DELETED_P(insn));
			(void) sprintf(low_pc, INSN_LABEL_FMT,
					current_funcdef_number,
					(unsigned int) INSN_UID(insn));
		}

		dwarf_output_common_decl_attributes(abbrev, decl,
						origin,
						low_pc,
			/* high_pc => */	(char*) NULL);
		break;
	    }

	case VAR_DECL:

		/* Output DIEs needed to specify this object's type. */

		dwarf_output_type(TREE_TYPE(decl), scope,
					TREE_READONLY(decl),
					TREE_THIS_VOLATILE(decl));

		/* For a public variable, emit a "pubnames" label at the
		   the beginning of its DIE.  A reference to this label has
		   already been made in the .debug_pubnames section.
		 */

		if (var_should_have_a_pubnames_entry(decl))
		{
			(void) ASM_GENERATE_INTERNAL_LABEL(pub_label,
						PUB_DIE_LABEL_CLASS,
						DECL_UID(decl));
			ASM_OUTPUT_LABEL(asm_out_file, pub_label);
		}

		/* Now output the DIE to represent the data object itself.
		   This gets complicated because of the possibility that the
		   VAR_DECL really represents an inlined instance of a
		   formal parameter for an inline function.
		  */

		{
			tree	origin = decl_ultimate_origin(decl);

			if (origin != NULL && TREE_CODE(origin) == PARM_DECL)
				dwarf_output_formal_parameter_die(decl);
			else if (!DECL_CONTEXT(decl))
				dwarf_output_global_variable_die(decl);
			else
				dwarf_output_local_variable_die(decl);
		}
		break;

	case FIELD_DECL:
		/* Ignore the nameless fields that are used to skip bits.  */

		if (DECL_NAME(decl) == 0)
			break;

		dwarf_output_type(DECL_BIT_FIELD_TYPE(decl)
					   ? DECL_BIT_FIELD_TYPE(decl)
					   : TREE_TYPE(decl),
				  scope,
				  TREE_READONLY(decl),
				  TREE_THIS_VOLATILE(decl));

		dwarf_output_data_member_die(decl);

		break;

	case PARM_DECL:
		dwarf_output_type(TREE_TYPE(decl), scope,
			TREE_READONLY(decl), TREE_THIS_VOLATILE(decl));
		dwarf_output_formal_parameter_die(decl);
		break;

	default:
		abort();
	}
}

/* Output the DIE for a file scope declaration, and DIEs for its type.

   'set_finalizing' indicates this is the final chance to output
   debug information for the declaration.  The trees have as much
   information about the declaration as they will ever have.
 */

void
dwarfout_file_scope_decl(decl, set_finalizing)
tree	decl;
int	set_finalizing;
{
	if (TREE_CODE(decl) == ERROR_MARK)
		return;

	if (DECL_IGNORED_P(decl))
		return;

	finalizing = set_finalizing;

	switch (TREE_CODE(decl))
	{
	case CONST_DECL:
		/* The enumerators of an enum type get output when we output
		   the relevant enum type itself.  */
		return;

	case FUNCTION_DECL:

		/* Ignore this FUNCTION_DECL if it refers to a builtin
		   declaration of a builtin function.  Explicit programmer-
		   supplied declarations of these same functions should
		   NOT be ignored however.
		 */

		if (DECL_EXTERNAL(decl) && DECL_FUNCTION_CODE(decl))
			return;

		/* What we would really like to do here is to filter out all
		   mere file-scope declarations of file-scope functions which
		   are never referenced later within this translation unit
		   (and keep all of the ones that *are* referenced later on)
		   but we aren't clairvoyant, so we have no idea which functions
		   will be referenced in the future (i.e. later on within the
		   current translation unit), unless set_finalizing is true.
		   So here we just ignore all file-scope function declarations
		   which are not also definitions.  If the debugger needs to
		   know something about these functions, it will have to hunt
		   around and find the DWARF information associated with the
		   *definition* of the function.

		   Note that we can't just check `DECL_EXTERNAL' to find out
		   which FUNCTION_DECL nodes represent definitions and which
		   ones represent mere declarations.  We have to check
		   `DECL_INITIAL' instead.  That's because the C front-end
		   supports some weird semantics for "extern inline" function
		   definitions.  These can get inlined within the current
		   translation unit (and thus, we need to generate DWARF info
		   for their abstract instances so that the DWARF info for the
		   concrete inlined instances can have something to refer to)
		   but the compiler never generates any out-of-line instances
		   of such things (despite the fact that they *are*
		   definitions).  The important point is that the C front-end
		   marks these "extern inline" functions as DECL_EXTERNAL, but
		   we need to generate DWARF for them anyway.

		   Note that the C++ front-end also plays some similar games
		   for inline function definitions appearing within include
		   files which also contain `#pragma interface' pragmas.
		 */

		{
			/* If the function is only declared (not defined)
			   go ahead and emit a function declaration DIE if
			   the function is declared in the main source file
			   and we are finalizing.
			 */

			unsigned int	file_num;
			file_num = lookup_filename(DECL_SOURCE_FILE(decl));

			if (DECL_INITIAL(decl) == NULL_TREE
			    && (!finalizing || file_num != 1))
			{
				return;
			}
		}

		if (function_should_have_a_pubnames_entry(decl))
			dwarf_output_pubnames_entry(decl);

		if (!DECL_EXTERNAL(decl) && DECL_INITIAL(decl)
					 && !DECL_ABSTRACT(decl))
			dwarf_emit_text_arange = 1;

		break;

	case VAR_DECL:
		if (var_should_have_a_pubnames_entry(decl))
			dwarf_output_pubnames_entry(decl);

		/* If this decl's address should be mentioned in .debug_aranges
		   then force an arange to be emitted for its whole section.
		   It's okay to be conservative here and always emit the
		   aranges, they don't take up much space in the object file.
		 */
		if (!DECL_EXTERNAL(decl) && DECL_RTL(decl)
					 && GET_CODE(DECL_RTL(decl)) == MEM)
		{
			/* This test mirrors that in assemble_variable()
			   However, don't assume that
				   DECL_INITIAL(decl) == error_mark_node
			   implies there is no initializer.  This is
			   because finish_decl() sets DECL_INITIAL to
			   error_mark_node after emitting the initial value.
			 */
			if ((!flag_no_common || !TREE_PUBLIC(decl))
			    && DECL_COMMON(decl) && DECL_INITIAL(decl) == 0)
			{
				/* The variable will go into either .bss,
				   .comm, or a named .bss section.  Emit
				   the .bss arange if we know for sure
				   that it doesn't go into .comm.
				 */
				if (TARGET_STRICT_REF_DEF || !TREE_PUBLIC(decl))
					dwarf_emit_bss_arange = 1;
			}
			else if (TREE_READONLY(decl)
				 || DECL_IN_TEXT_SECTION(decl))
				dwarf_emit_text_arange = 1;
			else
				dwarf_emit_data_arange = 1;
		}

		break;

	case TYPE_DECL:

		/* Don't output DIEs for built-in types that come in as
		   TYPE_DECLs.  These types will have been output on an
		   as-needed basis when referenced.  Note that these types
		   are always complete (unlike some structs/unions).
		 */
		if (DECL_SOURCE_LINE(decl) == 0)
			return;

		break;

	default:
		/* Shouldn't see other kinds of file-scope declarations here */
		abort();
	}

	(void) fputc('\n', asm_out_file);
	debug_info_section();
	dwarf_output_decl(decl, NULL_TREE);

	/* NOTE:  The call above to `dwarf_output_decl' may have caused one
	   or more file-scope named types to be placed onto the
	   pending_types_list.  We have to get those types off of that list
	   at some point, and this is the perfect time to do it.  If we didn't
	   take them off now, they might still be on the list when cc1 finally
	   exits.  That might be OK if it weren't for the fact that when we put
	   types onto the pending_types_list, we set the TREE_ASM_WRITTEN flag
	   for these types, and that causes them never to be output unless
	   `dwarf_output_pending_types_for_scope' takes them off of the list
	   and un-sets their TREE_ASM_WRITTEN flags.
	 */

	dwarf_output_pending_types_for_scope(NULL_TREE);
	assert(pending_types == 0);

	if (TREE_CODE(decl) == FUNCTION_DECL && DECL_INITIAL(decl) != NULL)
		current_funcdef_number++;
}

/* Output a marker (i.e. a label) for the beginning of the generated code
   for a lexical block.	 */

void
dwarfout_begin_block(blocknum)
unsigned int	blocknum;
{
	char	block_label[MAX_ARTIFICIAL_LABEL_BYTES];

	text_section();
	(void) sprintf(block_label, BLOCK_BEGIN_LABEL_FMT, blocknum);
	ASM_OUTPUT_LABEL(asm_out_file, block_label);
}

/* Output a marker (i.e. a label) for the end of the generated code
   for a lexical block.	 */

void
dwarfout_end_block(blocknum)
unsigned int	blocknum;
{
	char	block_label[MAX_ARTIFICIAL_LABEL_BYTES];

	text_section();
	(void) sprintf(block_label, BLOCK_END_LABEL_FMT, blocknum);
	ASM_OUTPUT_LABEL(asm_out_file, block_label);
}

/* Output a marker (i.e. a label) at a point in the assembly code which
   corresponds to a given source level label.
 */

void
dwarfout_label(insn)
rtx	insn;
{
	char lab[MAX_ARTIFICIAL_LABEL_BYTES];

	text_section();
	(void) sprintf(lab, INSN_LABEL_FMT, current_funcdef_number,
				      (unsigned int) INSN_UID(insn));
	ASM_OUTPUT_LABEL(asm_out_file, lab);
}

/* Output a label at a point in the assembly code which corresponds
   to the boundary of the valid range for one or more location expressions.
 */

void
dwarfout_le_boundary(labelnum)
int labelnum;
{
	char lab[MAX_ARTIFICIAL_LABEL_BYTES];

	text_section();
	(void) ASM_GENERATE_INTERNAL_LABEL(lab,
				INSN_LE_BOUNDARY_LABEL_CLASS, labelnum);
	ASM_OUTPUT_LABEL(asm_out_file, lab);
}

/* Output a label in the generated code at the very beginning of a function. */

void
dwarfout_begin_function()
{
	char lab[MAX_ARTIFICIAL_LABEL_BYTES];

	func_first_le_boundary_number = next_le_boundary_number++;
	func_last_le_boundary_number = next_le_boundary_number++;

	text_section();
	(void) ASM_GENERATE_INTERNAL_LABEL(lab,
		INSN_LE_BOUNDARY_LABEL_CLASS, func_first_le_boundary_number);
	ASM_OUTPUT_LABEL(asm_out_file, lab);
}

/* Output a marker (i.e. a label) for the point in the generated code where
   the real body of the function ends (just before the epilogue code).  */

void
dwarfout_end_function()
{
	/* We don't need to do anything at this point for DWARF 2 */
}

/* Output a marker (i.e. a label) for the absolute end of the generated code
   for a function definition.  This gets called *after* the epilogue code
   has been generated.	*/

void
dwarfout_end_epilogue()
{
	char		high_pc[MAX_ARTIFICIAL_LABEL_BYTES];
	Dwarf_cie	*cie;
	int		i;
static	int		fde_label_counter = 0;

	(void) sprintf(high_pc, FUNC_END_LABEL_FMT, current_funcdef_number);
	ASM_OUTPUT_LABEL(asm_out_file, 	high_pc);

	/* Emit this function's FDE. */

	if (debug_info_level >= DINFO_LEVEL_NORMAL && (cie = dwarf_fde.cie))
	{
		char	cie_label[MAX_ARTIFICIAL_LABEL_BYTES];
		char	fde_begin_label[MAX_ARTIFICIAL_LABEL_BYTES];
		char	fde_end_label[MAX_ARTIFICIAL_LABEL_BYTES];
		char	code_label[MAX_ARTIFICIAL_LABEL_BYTES];
		int	align;
		unsigned long	opcode, reg_g14;

		(void) fputc('\n', asm_out_file);
		debug_frame_section();

		/* Special case for 80960 leaf functions.
		   Emit trivial FDEs for the call entry point,
		   and for the 'ret' instruction.
		 */

		fde_label_counter++;
		(void) ASM_GENERATE_INTERNAL_LABEL(fde_begin_label,
					DWARF_FDE_BEGIN_LABEL_CLASS,
					fde_label_counter);
		(void) ASM_GENERATE_INTERNAL_LABEL(fde_end_label,
					DWARF_FDE_END_LABEL_CLASS,
					fde_label_counter);

		if (dwarf_fde.call_cie)
		{
			if (dwarf_fde.leaf_return_label_num < 0)
				abort();

			ASM_OUTPUT_LABEL(asm_out_file, fde_begin_label);
			dwarf_output_delta4_minus4(fde_end_label,
					fde_begin_label, "FDE length - 4");

			(void) ASM_GENERATE_INTERNAL_LABEL(cie_label,
					DWARF_CIE_BEGIN_LABEL_CLASS,
					dwarf_fde.call_cie->label_num);
			dwarf_output_delta4(cie_label,
					DEBUG_FRAME_GLOBAL_BEGIN_LABEL,
					"CIE reference");
			(void) fprintf(asm_out_file, "\t.word\t_%s\n",
					dwarf_fde.name);
			(void) fprintf(asm_out_file, "\t.word\t%s.lf - _%s\n",
					dwarf_fde.name, dwarf_fde.name);

			align = exact_log2(
				dwarf_fde.call_cie->code_alignment_factor);
			if (align >= 1)
				(void) ASM_OUTPUT_ALIGN(asm_out_file, align);
			ASM_OUTPUT_LABEL(asm_out_file, fde_end_label);

			/* Reuse the previous FDE's end label as the next
			   FDE's begin label.  Create an FDE begin label
			   for the bal FDE, and use that as this next
			   FDE's end label.
			 */
			fde_label_counter++;
			(void) ASM_GENERATE_INTERNAL_LABEL(fde_begin_label,
						DWARF_FDE_BEGIN_LABEL_CLASS,
						fde_label_counter);

			dwarf_output_delta4_minus4(fde_begin_label,
					fde_end_label, "FDE length - 4");

			(void) ASM_GENERATE_INTERNAL_LABEL(cie_label,
					DWARF_CIE_BEGIN_LABEL_CLASS,
					dwarf_fde.call_cie->label_num);
			dwarf_output_delta4(cie_label,
					DEBUG_FRAME_GLOBAL_BEGIN_LABEL,
					"CIE reference");

			(void) ASM_GENERATE_INTERNAL_LABEL(code_label,
					"LR", dwarf_fde.leaf_return_label_num);

			dwarf_output_addr(code_label, "start address range");
			dwarf_output_delta4(high_pc,code_label,"range length");

			if (align >= 1)
				(void) ASM_OUTPUT_ALIGN(asm_out_file, align);

			/* The bal FDE needs a different end label. */
			(void) ASM_GENERATE_INTERNAL_LABEL(fde_end_label,
						DWARF_FDE_END_LABEL_CLASS,
						fde_label_counter);
		}

		/* Now output the meaty FDE */

		ASM_OUTPUT_LABEL(asm_out_file, fde_begin_label);
		dwarf_output_delta4_minus4(fde_end_label, fde_begin_label,
					"FDE length - 4");

		(void) ASM_GENERATE_INTERNAL_LABEL(cie_label,
					DWARF_CIE_BEGIN_LABEL_CLASS,
					dwarf_fde.cie->label_num);
		dwarf_output_delta4(cie_label, DEBUG_FRAME_GLOBAL_BEGIN_LABEL,
					"CIE reference");

		if (dwarf_fde.call_cie)
		{
			(void) fprintf(asm_out_file, "\t.word\t%s.lf\n",
					dwarf_fde.name);

			(void) fprintf(asm_out_file, "\t.word\t");
			assemble_name(asm_out_file, code_label);
			(void) fprintf(asm_out_file, " - %s.lf\n",
					dwarf_fde.name);
		}
		else
		{
			(void) fprintf(asm_out_file, "\t.word\t");
			assemble_name(asm_out_file, dwarf_fde.name);
			(void) fprintf(asm_out_file, "\n");
			dwarf_output_delta4(high_pc, dwarf_fde.name,
					"FDE address range");
		}

		/* Now put out the prologue and epilogue rules, if any */

		reg_g14 = dwarf_reg_info[14].cie_column;

		if (dwarf_fde.clear_g14_label_num > 0)
		{
			(void) ASM_GENERATE_INTERNAL_LABEL(code_label,
					DWARF_FDE_CODE_LABEL_CLASS,
					dwarf_fde.clear_g14_label_num);
			dwarf_output_data1(DW_CFA_set_loc, "DW_CFA_set_loc");
			dwarf_output_addr(code_label, NO_COMMENT);
			opcode = dwarf_fde.prologue_rules[reg_g14][0];

			dwarf_output_debug_frame_rule(cie,
					(unsigned long) reg_g14,
					opcode,
					dwarf_fde.prologue_rules[reg_g14][1]);
		}

		if (dwarf_fde.prologue_label_num > 0)
		{
			/* Most FDE rules that advance the address use a
			   factored delta.  Since we don't know the actual
			   delta, to use these rules we'd have to emit
			   something of the form
				.word	(label1 - label2)/factor
			   Some assemblers don't like this division.
			   Others require that both labels have already
			   been seen.  To avoid problems, we use the
			   DW_CFA_set_loc rule, which takes an unfactored
			   address operand.  Ideally, the assembler would
			   have a special directive for this functionality,
			   that could take advantage of the compression
			   offered by the DW_CFA_offset rule.
			 */

			int	epi_ctr;

			(void) ASM_GENERATE_INTERNAL_LABEL(code_label,
					DWARF_FDE_CODE_LABEL_CLASS,
					dwarf_fde.prologue_label_num);
			dwarf_output_data1(DW_CFA_set_loc, "DW_CFA_set_loc");
			dwarf_output_addr(code_label, NO_COMMENT);

			for (i = 0; i < DWARF_CIE_MAX_REG_COLUMNS; i++)
			{
				opcode = dwarf_fde.prologue_rules[i][0];

				if (opcode == DW_CFA_nop
				    || (i == reg_g14
				        && dwarf_fde.clear_g14_label_num > 0))
					continue;

				dwarf_output_debug_frame_rule(cie,
					(unsigned long) i, opcode,
					dwarf_fde.prologue_rules[i][1]);
			}

			for (epi_ctr = 0;
				epi_ctr < dwarf_fde.num_epilogues_emitted;
				epi_ctr++)
			{
				int	lnum = dwarf_fde.prologue_label_num
						+ 1 + 3 * epi_ctr;

				/* Record the register save info just
				   prior to the epilogue code.
				 */

				(void) ASM_GENERATE_INTERNAL_LABEL(code_label,
					DWARF_FDE_CODE_LABEL_CLASS, lnum);
				dwarf_output_data1(DW_CFA_set_loc,
						"DW_CFA_set_loc");
				dwarf_output_addr(code_label, NO_COMMENT);
				dwarf_output_debug_frame_rule(cie, 0,
						DW_CFA_remember_state, 0);

				/* Emit a DW_CFA_restore instruction for
				   each saved register, just after the
				   register restore code.
				 */
				lnum++;
				(void) ASM_GENERATE_INTERNAL_LABEL(code_label,
					DWARF_FDE_CODE_LABEL_CLASS, lnum);
				dwarf_output_data1(DW_CFA_set_loc,
						"DW_CFA_set_loc");
				dwarf_output_addr(code_label, NO_COMMENT);

				for (i = 0; i < DWARF_CIE_MAX_REG_COLUMNS; i++)
				{
					opcode = dwarf_fde.prologue_rules[i][0];

					if (opcode != DW_CFA_nop)
						dwarf_output_debug_frame_rule(
							cie,
							(unsigned long) i,
							DW_CFA_restore, 0);
				}

				/* After the epilogue's return instruction,
				   restore the register save info that was
				   in effect prior to the epilogue code.
				 */

				lnum++;
				(void) ASM_GENERATE_INTERNAL_LABEL(code_label,
					DWARF_FDE_CODE_LABEL_CLASS, lnum);
				dwarf_output_data1(DW_CFA_set_loc,
						"DW_CFA_set_loc");
				dwarf_output_addr(code_label, NO_COMMENT);
				dwarf_output_debug_frame_rule(cie, 0,
						DW_CFA_restore_state, 0);
			}
		}

		align = exact_log2(cie->code_alignment_factor);
		if (align >= 1)
			(void) ASM_OUTPUT_ALIGN(asm_out_file, align);

		ASM_OUTPUT_LABEL(asm_out_file, fde_end_label);

		/* Clear the FDE for the next function. */
		if (dwarf_fde.name)
			free((void*)dwarf_fde.name);
		(void) memset((void*) &dwarf_fde, 0, sizeof(dwarf_fde));
	}

	dwarf_next_line_note_is_first_in_function = 1;
}

/* Generate 'statement program' instructions that add a row to the
   consumer's machine-address-to-source-position mapping matrix.
   Note that we might need to emit multiple statement program instructions
   to define the state machine registers for a given machine address.
   We must be careful to emit these instructions in the proper order.
   The statement program "special" opcodes, the "standard" opcode
   DW_LNS_copy, and the "extended" opcode DW_LNE_end_sequence, are the only
   instructions that trigger adding a new row to the matrix.
   Thus, to describe the source position of a given address, we must use
   exactly one of the above opcodes, and that must be the last opcode we emit.
 */

void
dwarfout_line(filename, note)
char	*filename;
rtx	note;		/* A note insn representing a source position */
{
	unsigned int	file_number, new_line, new_column;
	int		line_incr, special_opcode, is_stmt_begin;
	char		current_label[MAX_ARTIFICIAL_LABEL_BYTES];
	char		previous_label[MAX_ARTIFICIAL_LABEL_BYTES];
	char		comment[64];

	/* Extract relevant info from the note insn. */


	file_number = lookup_filename(filename);
	new_line = NOTE_LINE_NUMBER(note);
	new_column = NOTE_GET_REAL_SRC_COL(note);

	/* If this is a left-most statement, and the state machine's column
	   number is 0, use 0 for the new column.  We save space this way
	   since we'll have to emit fewer DW_LNS_set_column instructions.
	 */
#if defined(NOTE_LEFTMOST_STMT_P)
	if (NOTE_LEFTMOST_STMT_P(note) && new_column != sminfo.col)
		new_column = 0;
#endif

#if defined(NOTE_STMT_BEGIN_P)
	is_stmt_begin = NOTE_STMT_BEGIN_P(note);
#else
	is_stmt_begin = 1;
#endif

	line_incr = new_line - sminfo.line;

	/* The DWARF spec says don't emit anything if the file, line number
	   and column number haven't changed.  However, doing so may result
	   in too few breakpoints being set.  Consider the loop

		for(...) {
			stmt;   # assume stmt is at source position N
		}

	   which gets unrolled 4 times as

		for(...) {
			note N
			stmt;
			note N
			stmt;
			note N
			stmt;
			note N
			stmt;
		}

	   If the debugger user sets a breakpoint at stmt, the debugger
	   should set four breakpoints, one at each instance of 'stmt'.
	   But this wouldn't happen if we suppress the last three notes
	   just because they identify the same source position.
	   So only suppress the note if it does not mark the beginning
	   of a statement (ie, a breakpoint).  dwarfout_simplify_notes()
	   has already taken care of the case where no insns appear
	   between successive notes denoting the same source position.
	 */

	if (file_number == sminfo.file && line_incr == 0
	    && new_column == sminfo.col
	    && !NOTE_STMT_BEGIN_P(note))
		return;

	(void) ASM_GENERATE_INTERNAL_LABEL(previous_label,LINE_CODE_LABEL_CLASS,
						sminfo.next_text_label_num-1);
	(void) ASM_GENERATE_INTERNAL_LABEL(current_label,LINE_CODE_LABEL_CLASS,
						sminfo.next_text_label_num);
	sminfo.next_text_label_num++;

	text_section();
	ASM_OUTPUT_LABEL(asm_out_file, current_label);

	/* Now generate .debug_line instructions to update the state machine */

	debug_line_section();

	/* Update the state machine's 'file' register, if needed. */

	if (file_number != sminfo.file)
	{
		dwarf_output_data1(DW_LNS_set_file, "set_file");
		dwarf_output_udata((unsigned long) file_number, filename);
		sminfo.file = file_number;
	}

	/* Update the state machine's 'column' register, if needed. */

	if (new_column != sminfo.col)
	{
		dwarf_output_data1(DW_LNS_set_column, "set_column");
		dwarf_output_udata((unsigned long) new_column, NO_COMMENT);
		sminfo.col = new_column;
	}

	/* Update the state machine's 'is_stmt' register, if needed. */

	if (is_stmt_begin != sminfo.is_stmt)
	{
		dwarf_output_data1(DW_LNS_negate_stmt, "negate_stmt");
		sminfo.is_stmt = is_stmt_begin;
	}

	/* Now we need to output instructions that update the consumer's
	   state machine's line number and address registers.
	   We know what the line number difference is.  The address register
	   increment is unknown, however.  It's the current LINE_CODE_LABEL
	   minus the previous LINE_CODE_LABEL.

	   Since we don't know the address increment, we don't know whether
	   we can use a "special" opcode.  The assembler *does* know this
	   information, however, and can decide on the fly whether to use a
	   special opcode, but only if the assembler supports directives that
	   let the compiler specify this behavior.

	   For now, let's assume a special opcode cannot be used.
	 */

	/* Advance the address register.

	   Assume that the unsigned 2-byte operand to DW_LNS_fixed_advance_pc
	   is large enough to hold the difference between our code labels.
	   Note that, since we don't know the address increment, we may
	   in fact be emitting an address increment of zero here.
	 */

	if (dwarf_next_line_note_is_first_in_function)
	{
		/* Use DW_LNE_set_address. */
		dwarf_output_udata(0, "extended opcode indicator");
		/* Assume the size of an address is TARGET_POINTER_BYTE_SIZE */
		dwarf_output_udata(1 + TARGET_POINTER_BYTE_SIZE,
							"instruction length");
		dwarf_output_data1(DW_LNE_set_address, "DW_LNE_set_address");
		dwarf_output_addr(current_label, NO_COMMENT);
		dwarf_next_line_note_is_first_in_function = 0;
	}
	else
	{
		dwarf_output_data1(DW_LNS_fixed_advance_pc,
						"DW_LNS_fixed_advance_pc");
		dwarf_output_delta2(current_label, previous_label,
						"address incr");
	}

	/* Advance the line register.  We need to emit a special opcode even
	   if the line increment is zero, because we need to force the consumer
	   to generate a new row in the matrix.
	 */

	/* Use DW_LNS_advance_line if line_incr won't fit in a special opcode.
	 */

	if (line_incr < sminfo.line_base ||
	    line_incr > (int) (sminfo.line_base + sminfo.line_range - 1))
	{
		dwarf_output_data1(DW_LNS_advance_line, "DW_LNS_advance_line");
		dwarf_output_sdata((long) line_incr, NO_COMMENT);

		/* We still need to output a special opcode (done below) that
		   has line and address increments of zero.  This is the
		   cheapest way to force the consumer to add a row to the
		   matrix that describes the new address.
		 */
		line_incr = 0;
	}

	special_opcode = line_incr - sminfo.line_base + sminfo.opcode_base;
	(void) sprintf(comment, "special opcode: line += %d", line_incr);
	assert(special_opcode >= sminfo.opcode_base && special_opcode <= 255);
	dwarf_output_data1(special_opcode, comment);

	sminfo.line = new_line;

	/* Leave final.c in the .text section. */
	text_section();
}

/* Determine if an insn is of a class of insns that may have an associated
   line number note.  This includes real insns, and block begin and block
   end notes.
 */

#define IS_NOTABLE_INSN_P(n) \
	(IS_ACTIVE_INSN_P(n) \
	 || (GET_CODE(n) == NOTE \
	     && (   NOTE_LINE_NUMBER(n) == NOTE_INSN_BLOCK_BEG \
	         || NOTE_LINE_NUMBER(n) == NOTE_INSN_BLOCK_END)))

#define DETACH_NOTE(n) \
	do { \
		if (PREV_INSN(n)) \
			NEXT_INSN(PREV_INSN(n)) = NEXT_INSN(n); \
		if (NEXT_INSN(n)) \
			PREV_INSN(NEXT_INSN(n)) = PREV_INSN(n); \
	} while (ALWAYS_ZERO)

/* Remove redundant line number notes within a group of un-interrupted notes.
   'last' is the last note in the group.  An un-interrupted group of line
   number notes is a list of insns containing no active insns.
 */

static rtx
dwarfout_simplify_note_group(last)
rtx	last;
{
	rtx	note, next;

	/* Examine each line number note in the group.  Notes further
	   down in the group are "stronger" in the sense that they are
	   more closely associated with the subsequent active insns.
	 */

	for (note = PREV_INSN(last); note; note = next)
	{
		next = PREV_INSN(note);

		if (IS_ACTIVE_INSN_P(note))
			return;

		if (IS_LINE_NUMBER_NOTE_P(note))
		{
			/* Notes that don't mark a statement boundary, and that
			   exist within a group of other line number notes,
			   are useless.
			 */
			if (!NOTE_STMT_BEGIN_P(note))
			{
				DETACH_NOTE(note);
			}
			else if (NOTES_REP_SAME_SOURCE_P(note, last))
			{
				/* We're going to delete 'note'.  But first,
				   transfer some of its info to 'last'.
				 */

				if (!NOTE_STMT_BEGIN_P(last))
				{
					/* These may be no-ops, that's okay */
					NOTE_SET_INSTANCE_NUMBER(last,
						NOTE_GET_INSTANCE_NUMBER(note));
					NOTE_SET_STMT_BEGIN(last);
				}

				DETACH_NOTE(note);
			}
		}
	}
}

void
dwarfout_simplify_notes(start)
rtx	start;	/* first insn in the insn stream. */
{
	rtx	note, first_note, next, prev_note;
	int	i, max_line;
	unsigned int	col;
	IMSTG_COL_TYPE	*min_col;

        /* As a special case, remove redundancy in the function's first line
           number note group.  We don't want to remove the function's first
	   note, since it may be the first insn in the insn stream, and because
	   other phases may depend on the first note staying around.
	   This special-casing will prevent dwarfout_simplify_note_group()
	   from removing the first note.
         */

	first_note = start;
	while (first_note && !IS_LINE_NUMBER_NOTE_P(first_note))
		first_note = NEXT_INSN(first_note);

        if (first_note)
        {
		for (note = NEXT_INSN(first_note); note; note = next)
		{
			next = NEXT_INSN(note);
			if (IS_ACTIVE_INSN_P(note))
				break;

			if (IS_LINE_NUMBER_NOTE_P(note)
			    && NOTES_REP_SAME_SOURCE_P(note, first_note))
				DETACH_NOTE(note);
		}
        }
	else
		return;

	/* Now walk the insns backwards, and each time we encounter a
	   line number note, remove redundant notes in its group.
	   This leads to an N-squared algorithm, but that's necessary
	   to do a good job.
	 */

	prev_note = 0;
	max_line = 0;

        for (note = get_last_insn(); note; note = PREV_INSN(note))
	{
		if (IS_LINE_NUMBER_NOTE_P(note))
		{
			if (NOTE_LINE_NUMBER(note) > max_line)
				max_line = NOTE_LINE_NUMBER(note);

			/* Remove line number notes prior to 'note' that are
			   redundant because they apply to the same active
			   insns as 'note', and because they have the same
			   information as 'note'.
			 */
			dwarfout_simplify_note_group(note);

			/* Remove the note from the previous iteration if
			   it is redundant because of 'note'.  This catches
			   the case

				note
				active insns for note
				prev_note
				active insns for prev_note

			   where 'note' and 'prev_note' indicate the same
			   instance of some source position, and prev_note
			   is not a breakpoint.  In essence, this allows
			   re-merging of a note that had been divided by
			   some optimization.
			 */
			if (prev_note
			    && NOTES_REP_SAME_SOURCE_P(note, prev_note)
			    && (!NOTE_STMT_BEGIN_P(prev_note)
				|| (NOTE_GET_INSTANCE_NUMBER(prev_note)
				    == NOTE_GET_INSTANCE_NUMBER(note))))
			{
				if (NOTE_STMT_BEGIN_P(prev_note)
				    != NOTE_STMT_BEGIN_P(note))
				{
				    NOTE_SET_STMT_BEGIN(note);
				}
				DETACH_NOTE(prev_note);
			}

			prev_note = note;
		}
	}

	/* Mark each line number note that represents the leftmost statement
	   on its source line.  This allows us to save space in the DWARF
	   debug output.
	   Also take this opportunity to map sinfo column numbers into
	   actual column numbers.
	 */

	/* if (write_symbols == DWARF_DEBUG) */
	{
		min_col = (IMSTG_COL_TYPE *) xmalloc(
					(max_line + 1)*sizeof(IMSTG_COL_TYPE));

		for (i = 0; i <= max_line; i++)
			min_col[i] = MAX_LIST_COL;

		for (note = start; note; note = NEXT_INSN(note))
		{
			if (IS_LINE_NUMBER_NOTE_P(note))
			{
				col = NOTE_COLUMN_NUMBER(note);
				if (col < min_col[NOTE_LINE_NUMBER(note)])
					min_col[NOTE_LINE_NUMBER(note)] = col;

				col = imstg_map_sinfo_col_to_real_col(
						NOTE_LISTING_START(note));
				NOTE_SET_REAL_SRC_COL(note, col);
			}
		}

		for (note = start; note; note = NEXT_INSN(note))
		{
			if (IS_LINE_NUMBER_NOTE_P(note))
			{
				col = NOTE_COLUMN_NUMBER(note);
				if (col == min_col[NOTE_LINE_NUMBER(note)])
					NOTE_SET_LEFTMOST_STMT(note);
			}
		}

		free((void*) min_col);
	}
}

void
dwarfout_disable_notes(deleted_insn)
rtx	deleted_insn;
{
	rtx	pred, succ, first_note;

	if (!deleted_insn || GET_CODE(deleted_insn) == NOTE)
		return;

	succ = NEXT_INSN(deleted_insn);
	/* For a JUMP_INSN, the following barrier will also be deleted */
	if (succ && GET_CODE(deleted_insn) == JUMP_INSN
		 && GET_CODE(succ) == BARRIER)
		succ = NEXT_INSN(succ);

	/* If there is any notable insn immediately following deleted_insn,
	   then do nothing.  The notes still apply to that active insn.
	 */

	for ( ; succ; succ = NEXT_INSN(succ))
	{
		if (IS_NOTABLE_INSN_P(succ))
			return;

		if (NOTE_LINE_NUMBER(succ) >= 0)
			break;
	}

	/* If there is any notable insn immediately preceding deleted_insn, then
	   do nothing.  Don't delete the function's first line number note.
	 */

	first_note = get_insns();
	while (first_note &&
	    (GET_CODE(first_note) != NOTE || NOTE_LINE_NUMBER(first_note) < 0))
		first_note = NEXT_INSN(first_note);

	pred = PREV_INSN(deleted_insn);
	for (; pred && pred != first_note; pred = PREV_INSN(pred))
	{
		if (IS_NOTABLE_INSN_P(pred))
			break;

		if (GET_CODE(pred) == NOTE && NOTE_LINE_NUMBER(pred) >= 0)
		{
			NOTE_LINE_NUMBER(pred) = NOTE_INSN_DELETED;
			NOTE_SOURCE_FILE(pred) = 0;
		}
	}
}

/* Return the line number note corresponding to insn, or NULL.
 */

rtx
dwarfout_effective_note(insn)
rtx	insn;
{
	while (insn && (GET_CODE(insn) != NOTE || NOTE_LINE_NUMBER(insn) < 0))
		insn = PREV_INSN(insn);

	return insn;
}

/* Determine if 'insn' is the last real insn corresponding to the
   given line number note.  Assume 'note' is the insn's effective note.
 */

int
dwarfout_last_insn_p(insn, note)
rtx	insn;
rtx	note;
{
	rtx	pred, succ;

	/* If there are any real insns before the next line number note,
	   then 'insn' is not this note's only remaining insn.
	 */
	for (succ = NEXT_INSN(insn); succ; succ = NEXT_INSN(succ))
	{
		if (GET_CODE(succ) != NOTE)
			return 0;

		if (NOTE_LINE_NUMBER(succ) >= 0)
			break;
	}

	/* If there are any real insns between 'note' and 'insn'
	   then 'insn' is not the note's only remaining insn.
	 */

	pred = PREV_INSN(insn);

	for (; pred && pred != note; pred = PREV_INSN(pred))
	{
		if (GET_CODE(pred) != NOTE)
			return 0;
	}

	return 1;
}

/* Called from check_newline in the parser when it see's a #line directive
   that enters a new include file.
 */

static int	start_new_source_file_called_already = 0;

void
dwarfout_start_new_source_file(filename, old_lineno)
char	*filename;
int	old_lineno;
{
	unsigned int	file_number;

	imstg_save_current_section();
	if (!pre_initialize_done)
		dwarf_preinitialize();

	start_new_source_file_called_already = 1;
		/* This let's dwarfout_init know that we've already generated
		   a define_file entry for the primary source file.
		 */

	file_number = lookup_filename(filename);

	if (old_lineno < 0)
	{
		warning("invalid line number (%d) in #line directive",
				old_lineno);
		old_lineno = 0;
	}

	(void) fputc('\n', asm_out_file);
	debug_macinfo_section();
	dwarf_output_data1(DW_MACINFO_start_file, "start file");
	dwarf_output_udata((unsigned long) old_lineno, "#include line number");
	dwarf_output_udata((unsigned long) file_number, "file number");
	imstg_resume_previous_section();
}

/* Called from check_newline in the parser when it see's a #line directive
   that exits an include file.
 */
void
dwarfout_resume_previous_source_file()
{
	imstg_save_current_section();
	if (!pre_initialize_done)
		dwarf_preinitialize();

	debug_macinfo_section();
	dwarf_output_data1(DW_MACINFO_end_file, "end file");
	imstg_resume_previous_section();
}

/* Called from check_newline().  The `buffer' parameter contains the raw
   (unparsed) tail part of the directive line, i.e. the part which is past
   the initial whitespace, #, whitespace, directive-name, whitespace part.

   We parse the line and emit diagnostics as appropriate.  The DWARF spec
   is very specific with regards to white space in the macro definition.
   We tolerate extra white space in 'buffer', but throw out this extraneous
   white space when writing the .debug_macinfo entry.
 */

void
dwarfout_define(line_num, buffer)
unsigned int	line_num;
unsigned char	*buffer;
	/* Use 'unsigned char' to avoid negative array subscripts */
{
static	char	*copy = NULL;
static	int	copy_length = 0;
static	unsigned char	open_paren = '(';
static	unsigned char	close_paren = ')';

	int	length;
	char	*cp;
	unsigned char	*bp;

	if (!pre_initialize_done)
		dwarf_preinitialize();

	/* Make a copy of buffer so we can compress white space. */

	if ((length = strlen((char*) buffer)) > copy_length - 1)
	{
		copy_length = length + 50;
		copy = (copy ? (char*) xrealloc(copy, copy_length)
			    : (char*) xmalloc(copy_length));
	}

	cp = copy;
	bp = buffer;

	/* First copy the macro name into 'copy' */

	if (is_idstart[*bp])
	{
		do {
			*cp++ = *bp++;
		} while (is_idchar[*bp]);
	}

	if (cp == copy)
	{
		error("missing or invalid macro name in #define directive");
		return;
	}

	/* If the macro takes arguments, copy the parameter list.
	   'copy' must have no white space between parameters.
	 */

	if (*bp == open_paren)
	{
		int	last_was_comma = 0;
		int	last_was_ellipsis = 0;

		*cp++ = *bp++;
		DWARF_SKIP_WHITE_SPACE(bp);

		while (*bp != close_paren)
		{
			/* At this point, we're expecting a parameter id */

			char	*start = cp;

			if (is_idstart[*bp])
			{
				do {
					*cp++ = *bp++;
				} while (is_idchar[*bp]);

				/* Handle the GNU '...' extension.  The last
				   parameter can be of the form "id..."
				 */

				 if (strncmp((char *)bp, "...", 3) == 0)
				 {
					bp += 3;
					*cp++ = '.'; *cp++ = '.'; *cp++ = '.';
					last_was_ellipsis = 1;
				 }
			}

			if (cp == start)
			{
				error("missing or invalid parameter name in #define directive");
				return;
			}

			/* Swallow the comma, if there. */
			DWARF_SKIP_WHITE_SPACE(bp);
			if (*bp == ',')
			{
				if (last_was_ellipsis)
				{
					error("comma after parm... in #define directive");
					return;
				}

				*cp++ = *bp++;
				last_was_comma = 1;
				DWARF_SKIP_WHITE_SPACE(bp);
			}
			else if (*bp != close_paren)
			{
				error("missing or invalid parameter separator in #define directive");
				return;
			}
			else
				last_was_comma = 0;
		}

		if (*bp != close_paren || last_was_comma)
		{
			error("unterminated parameter list in #define directive");
			return;
		}

		*cp++ = *bp++;	/* Copy the close_paren */
	}

	/* DWARF says one white space character must separate the
	   macro from its (possibly empty) definition.
	   Eat any white space in 'buffer' preceding the definition.
	 */

	*cp++ = ' ';
	DWARF_SKIP_WHITE_SPACE(bp);

	/* Allow anything in the macro body, except strip trailing white space.
	 */

	if (*bp)
	{
		unsigned char	*end = &buffer[length-1];

		while (end > bp && (*end == ' ' || *end == '\t'))
			end--;
		(void) strncpy(cp, (char*) bp, (end - bp + 1));
		cp += end - bp + 1;
	}

	*cp = '\0';

	imstg_save_current_section();
	(void) fputc('\n', asm_out_file);
	debug_macinfo_section();
	dwarf_output_data1(DW_MACINFO_define, "DW_MACINFO_define");
	dwarf_output_udata((unsigned long) line_num, "line number");
	/* We may want to break up 'copy' into smaller chunks, in case
	   the assembler has a limit on the length of a .ascii string.
	   We'll cross that bridge when we come to it.
	 */
	dwarf_output_string(copy, NO_COMMENT);
	imstg_resume_previous_section();
}

/* Called from check_newline().  The `buffer' parameter contains the raw
   (unparsed) tail part of the directive line, i.e. the part which is past
   the initial whitespace, #, whitespace, directive-name, whitespace part.
   This should be just the name of the macro being undefined, but might have
   white space or other garbage at the end.

   We assume 'buffer' is writable, so we can temporarily stick a '\0'
   after the macro name to nul-terminate it.
 */

void
dwarfout_undef(line_num, buffer)
unsigned int	line_num;
unsigned char	*buffer;
	/* Use 'unsigned char' to avoid negative array subscripts */
{
	unsigned char	save_end, *end = buffer;
	int	length;

	if (!pre_initialize_done)
		dwarf_preinitialize();

	/* Extract just the macro name from buffer */

	if (is_idstart[*end])
	{
		do {
			end++;
		} while (is_idchar[*end]);
	}

	if ((length = end - buffer) == 0)
	{
		error("missing or invalid macro name in #undef directive");
		return;
	}

	/* Allow trailing white space after the macro name */
	DWARF_SKIP_WHITE_SPACE(end);
	if (*end)
		warning("garbage at end of #undef directive ignored '%s'",end);

	imstg_save_current_section();
	(void) fputc('\n', asm_out_file);
	debug_macinfo_section();
	dwarf_output_data1(DW_MACINFO_undef, "DW_MACINFO_undef");
	dwarf_output_udata((unsigned long) line_num, "line number");

	save_end = buffer[length];
	buffer[length] = '\0';
	dwarf_output_string((char*)buffer, NO_COMMENT);
	buffer[length] = save_end;
	imstg_resume_previous_section();
}

/* Set up for DWARF output at the start of compilation.
   WARNING:  check_newline can make calls to dwarfout_start_new_source_file,
   dwarfout_resume_previous_source_file, dwarfout_define and dwarfout_undef
   before this initialization.  See dwarf_preinitialize() for more detail.
 */

void
dwarfout_init(asm_fd, primary_source_file_name)
FILE	*asm_fd;
char	*primary_source_file_name;
{
	assert(asm_fd == asm_out_file);

	/* Do preinitialization if not done already.  Do this first
	   so it always comes out first in the assembly file regardless
	   of who does the initialization.
	 */

	if (!pre_initialize_done)
		dwarf_preinitialize();
	(void) lookup_filename(primary_source_file_name);

	/* Generate a DW_MACINFO_start_file entry for the main source file,
	   if not done so already.
	 */
	if (!start_new_source_file_called_already)
		dwarfout_start_new_source_file(primary_source_file_name, 0);

	/* Output a starting label for various sections.  */

	(void) fputc('\n', asm_out_file);
	text_section();
	ASM_OUTPUT_LABEL(asm_out_file, TEXT_BEGIN_LABEL);
	{
		/* Define the first line number label in .text so that
		   dwarfout_line() will have a base address from which
		   to advance the program counter.
		 */

		char	line_label[MAX_ARTIFICIAL_LABEL_BYTES];

		(void) ASM_GENERATE_INTERNAL_LABEL(line_label,
				LINE_CODE_LABEL_CLASS,
				sminfo.next_text_label_num);
		sminfo.next_text_label_num++;
		ASM_OUTPUT_LABEL(asm_out_file, line_label);
	}

	(void) fputc('\n', asm_out_file);
	data_section();
	ASM_OUTPUT_LABEL(asm_out_file, DATA_BEGIN_LABEL);

	(void) fputc('\n', asm_out_file);
	(void) fprintf(asm_out_file, "\t.bss	");
	assemble_name(asm_out_file, BSS_BEGIN_LABEL);
	(void) fprintf(asm_out_file, ",0,1\n");

	(void) fputc('\n', asm_out_file);
	debug_abbrev_section();
	ASM_OUTPUT_LABEL(asm_out_file, DEBUG_ABBREV_BEGIN_LABEL);

	(void) fputc('\n', asm_out_file);
	debug_info_section();
	ASM_OUTPUT_LABEL(asm_out_file, DEBUG_INFO_BEGIN_LABEL);

	/* output the compile unit header */

	dwarf_output_delta4_minus4(DEBUG_INFO_END_LABEL, DEBUG_INFO_BEGIN_LABEL,
					".debug_info length - 4");
	dwarf_output_data2(DWARF_VERSION, "DWARF version");
	dwarf_output_delta4(DEBUG_ABBREV_BEGIN_LABEL,
				DEBUG_ABBREV_GLOBAL_BEGIN_LABEL, NO_COMMENT);
	dwarf_output_data1(TARGET_POINTER_BYTE_SIZE, "bytes in an address");
	dwarf_output_compile_unit_die(primary_source_file_name);

	/* output the .debug_pubnames prolog */

	(void) fputc('\n', asm_out_file);
	debug_pubnames_section();
	ASM_OUTPUT_LABEL(asm_out_file, DEBUG_PUBNAMES_BEGIN_LABEL);
	dwarf_output_delta4_minus4(DEBUG_PUBNAMES_END_LABEL,
		DEBUG_PUBNAMES_BEGIN_LABEL, ".debug_pubnames length - 4");
	dwarf_output_data2(DWARF_VERSION, "pubnames version");
	dwarf_output_delta4(DEBUG_INFO_BEGIN_LABEL,
		DEBUG_INFO_GLOBAL_BEGIN_LABEL, "offset to compile unit header");
	dwarf_output_delta4(DEBUG_INFO_END_LABEL, DEBUG_INFO_BEGIN_LABEL,
				"compile unit .debug_info length");

	/* output the .debug_aranges prolog */

	(void) fputc('\n', asm_out_file);
	debug_aranges_section();
	ASM_OUTPUT_LABEL(asm_out_file, DEBUG_ARANGES_BEGIN_LABEL);
	dwarf_output_delta4_minus4(DEBUG_ARANGES_END_LABEL,
		DEBUG_ARANGES_BEGIN_LABEL, ".debug_aranges length - 4");
	dwarf_output_data2(DWARF_VERSION, "aranges version");
	dwarf_output_delta4(DEBUG_INFO_BEGIN_LABEL,
		DEBUG_INFO_GLOBAL_BEGIN_LABEL, "offset to compile unit header");
	dwarf_output_data1(TARGET_POINTER_BYTE_SIZE, "bytes in an address");
	dwarf_output_data1(TARGET_SEGMENT_DESCR_BYTE_SIZE,
				"bytes in a segment descriptor");
	{
		int align = exact_log2(2*TARGET_POINTER_BYTE_SIZE);
		if (align >= 0)
			(void) ASM_OUTPUT_ALIGN(asm_out_file, align);
	}
}

/* Wrap up each .debug_* section, and emit the .debug_abbrev section. */

void
dwarfout_finish()
{
	int		i;
	Dwarf_cie	*ciep;

	/* Output an ending label for various sections.  */

	(void) fputc('\n', asm_out_file);
	text_section();
	ASM_OUTPUT_LABEL(asm_out_file, TEXT_END_LABEL);

	(void) fputc('\n', asm_out_file);
	data_section();
	ASM_OUTPUT_LABEL(asm_out_file, DATA_END_LABEL);

	{
		/* generate the DW_LNE_end_sequence extended opcode that
		   is required at the end of the .debug_line section.
		   But first bump the debugger's state machine's 'address'
		   register to 1-byte past the end of this compilation
		   unit's .text section.
		 */
		char	current_label[MAX_ARTIFICIAL_LABEL_BYTES];
		char	previous_label[MAX_ARTIFICIAL_LABEL_BYTES];

		(void) ASM_GENERATE_INTERNAL_LABEL(previous_label,
				LINE_CODE_LABEL_CLASS,
				sminfo.next_text_label_num-1);
		(void) ASM_GENERATE_INTERNAL_LABEL(current_label,
				LINE_CODE_LABEL_CLASS,
				sminfo.next_text_label_num);
		sminfo.next_text_label_num++;

		text_section();
		ASM_OUTPUT_LABEL(asm_out_file, current_label);

		(void) fputc('\n', asm_out_file);
		debug_line_section();

		/* Don't let this source position be interpreted as a
		   breakpoint location.
		 */

		if (sminfo.is_stmt)
		{
			dwarf_output_data1(DW_LNS_negate_stmt, "negate_stmt");
			sminfo.is_stmt = 0;
		}

		dwarf_output_data1(DW_LNS_fixed_advance_pc,
					"DW_LNS_fixed_advance_pc");
		dwarf_output_delta2(current_label, previous_label,
							"address incr");

		dwarf_output_udata(0, "extended opcode indicator");
		dwarf_output_udata(1, "instruction length");
		dwarf_output_data1(DW_LNE_end_sequence, "DW_LNE_end_sequence");

		dwarf_reset_smregs();  /* Record the effects of end_sequence */

		ASM_OUTPUT_LABEL(asm_out_file, DEBUG_LINE_END_LABEL);
	}

	/* Emit all CIEs used for .debug_frame.
	   Be careful not to emit an empty .debug_frame section,
	   DWARF disallows that.
	 */
	if ((ciep = dwarf_cie_list) != NULL)
	{
		(void) fputc('\n', asm_out_file);
		debug_frame_section();
		for (; ciep; ciep = ciep->next_cie)
			dwarf_output_cie(ciep);
	}

	(void) fputc('\n', asm_out_file);
	debug_macinfo_section();
	/* Emit a DW_MACINFO_end_file entry for the main source file. */
	dwarfout_resume_previous_source_file();
	dwarf_output_udata(0, "end of macinfo entries");

	(void) fputc('\n', asm_out_file);
	debug_info_section();
	dwarf_output_inline_attribute_values();
	/* Output a null DIE marking the end of the compile unit's children. */
	dwarf_output_null_die("end compilation unit children");
	ASM_OUTPUT_LABEL(asm_out_file, DEBUG_INFO_END_LABEL);

	(void) fputc('\n', asm_out_file);
	debug_pubnames_section();
	dwarf_output_data4(0, "end pubnames list");
	ASM_OUTPUT_LABEL(asm_out_file, DEBUG_PUBNAMES_END_LABEL);

	(void) fputc('\n', asm_out_file);
	debug_aranges_section();
#if defined(IMSTG_PRAGMA_SECTION_SEEN)
	/* Suppress range information if pragma section was used. */
	if (!IMSTG_PRAGMA_SECTION_SEEN && write_symbols >= DINFO_LEVEL_TERSE)
#else
	if (write_symbols >= DINFO_LEVEL_TERSE)
#endif
	{
		if (dwarf_emit_text_arange)
		{
			dwarf_output_addr(TEXT_BEGIN_LABEL, NO_COMMENT);
			dwarf_output_delta4(TEXT_END_LABEL, TEXT_BEGIN_LABEL,
						NO_COMMENT);
		}
		if (dwarf_emit_data_arange)
		{
			dwarf_output_addr(DATA_BEGIN_LABEL, NO_COMMENT);
			dwarf_output_delta4(DATA_END_LABEL, DATA_BEGIN_LABEL,
						NO_COMMENT);
		}
		if (dwarf_emit_bss_arange)
		{
			(void) fputc('\n', asm_out_file);
			(void) fprintf(asm_out_file, "\t.bss	");
			assemble_name(asm_out_file, BSS_END_LABEL);
			(void) fprintf(asm_out_file, ",0,1\n");

			dwarf_output_addr(BSS_BEGIN_LABEL, NO_COMMENT);
			dwarf_output_delta4(BSS_END_LABEL, BSS_BEGIN_LABEL,
						NO_COMMENT);
		}
	}
	dwarf_output_data4(0, "end aranges: address 0");
	dwarf_output_data4(0, "end aranges: length  0");
	ASM_OUTPUT_LABEL(asm_out_file, DEBUG_ARANGES_END_LABEL);

	/* Output all referenced .debug_abbrev abbreviations. */

	(void) fputc('\n', asm_out_file);
	debug_abbrev_section();
	for (i=0; abbrev_table[i].abbrev_id != LAST_AND_UNUSED_ABBREV_CODE; i++)
	{
		char		*form_char;
		int		attr_index;
		unsigned long	attr_id;
		Debug_Abbrev	*abbrev = &abbrev_table[i];

		if (!abbrev->is_referenced
		    || abbrev->abbrev_id == DW_ABBREV_NULL)
			continue;

		(void) fputc('\n', asm_out_file);
		dwarf_output_udata((unsigned long) abbrev->abbrev_id,
							abbrev->name);
		dwarf_output_udata(abbrev->die_tag,
					dwarf_tag_name(abbrev->die_tag));
		dwarf_output_data1(abbrev->has_children,
					abbrev->has_children == DW_CHILDREN_no
						? "no children"
						: "has children");

		attr_index = 0;
		form_char = abbrev->form_encodings;

		for (; attr_id = XABBREV_ATTR(abbrev,attr_index); attr_index++)
		{
			int			k;
			struct attr_form	*form = NULL;

			/* Get the info describing form_char */

			for (k = 0; form_mapping[k].encoded_name; k++)
			{
				if (*form_char == *form_mapping[k].encoded_name)
				{
					form = &form_mapping[k];
					break;
				}
			}
			assert(form);

			dwarf_output_udata(attr_id,
					/* dwarf_attribute_name() uses a
					   linear search, avoid calling it
					   if its result is not used.
					  */
					flag_verbose_asm
						? dwarf_attribute_name(attr_id)
						: NO_COMMENT);

			dwarf_output_udata(form->id, form->id_name);
			form_char++;
		}

		/* Mark the end of this abbreviation. */
		dwarf_output_udata(DW_ABBREV_NULL, NO_COMMENT);
		dwarf_output_udata(DW_ABBREV_NULL, NO_COMMENT);
	}

	/* Mark the end of all the .debug_abbrev entries. */
	(void) fputc('\n', asm_out_file);
	dwarf_output_udata(DW_ABBREV_NULL, "end of abbreviations");
}

/* Debugging Support */

static void
dwarf_dump_decl(fp, decl)
FILE	*fp;
tree	decl;
{
	char	*name = "--unknown--";

	if (TREE_CODE(decl) == VAR_DECL)
		(void) fprintf(fp, "VAR_DECL:  ");
	else if (TREE_CODE(decl) == PARM_DECL)
		(void) fprintf(fp, "PARM_DECL: ");
	else
		return;

	if (DECL_NAME(decl) && IDENTIFIER_POINTER(DECL_NAME(decl)))
		name = IDENTIFIER_POINTER(DECL_NAME(decl));

	(void) fprintf(fp, "\"%s\" (uid = %d) (first_alid = %d)\n",
			name, DECL_UID(decl), DECL_DWARF_FIRST_ALID(decl));
	if (TREE_CODE(decl) == PARM_DECL)
	{
		(void) fprintf(fp, "decl_incoming_rtl:  ");
		print_rtl(fp, DECL_INCOMING_RTL(decl));
		(void) fprintf(fp, "\n");
	}
	(void) fprintf(fp, "decl_rtl:  ");
	print_rtl(fp, DECL_RTL(decl));
	(void) fprintf(fp, "\n----\n");
}

static void
dwarf_dump_decls_for_block(fp, block)
FILE	*fp;
tree	block;
{
	tree	decl;

	if (!block)
		return;

	for (decl = BLOCK_VARS(block); decl; decl = TREE_CHAIN(decl))
		dwarf_dump_decl(fp, decl);

	for (decl = BLOCK_SUBBLOCKS(block); decl; decl = BLOCK_CHAIN(decl))
		dwarf_dump_decls_for_block(fp, decl);
}

#define DWARF_DISPLAY_SET(fp,S,w) \
	for ((w) = 0; (w) < dwarf_dfa_set_size_in_words; (w)++) \
		(void) fprintf((fp), " %.8x", (S)[(w)])

static void
dwarf_debugging_dump(banner)
char	*banner;
{
	int	i, b, r, w;
	FILE	*fp = dwarf_dump_file;
	tree	decl;

	if (!fp)
		return;

	(void) fprintf(fp, "\n;; %s\n", banner ? banner : "");

	/* Dump the avail_locations array. */

	if (avail_locations)
	{
		Dwarf_avail_location	*alidp;

		(void) fprintf(fp, "\n;; avail_locations[0..%d]\n\n",
							dwarf_max_alid);
		for (i = 0; i <= dwarf_max_alid; i++)
		{
			char	*name = "--unknown--";

			alidp = &avail_locations[i];

			if (!alidp->rtl)
				continue;

			if ((decl = alidp->decl) != NULL)
			{
			  if (DECL_NAME(decl)
			      && IDENTIFIER_POINTER(DECL_NAME(decl)))
				name = IDENTIFIER_POINTER(DECL_NAME(decl));
			}

			(void) fprintf(fp, "alid(%2d) kind(%d)",i,alidp->kind);
			if (alidp->same_as_id)
				(void) fprintf(fp, " same_as_id(%2d)",
					alidp->same_as_id);

			(void) fprintf(fp, "  \"%s\":  ", name);
			print_rtl(fp, alidp->rtl);
			(void) fprintf(fp, "\n");
			if (alidp->block_begin_note)
			  (void) fprintf(fp, "begin note uid %d\n",
					INSN_UID(alidp->block_begin_note));
			if (alidp->block_end_note)
			  (void) fprintf(fp, "end   note uid %d\n",
					INSN_UID(alidp->block_end_note));

			if (alidp->max_bound_index > 0)
			{
			  (void) fprintf(fp, "ranges: ");
			  for (r = 0; r <= alidp->max_bound_index; r+= 2)
			  {
				rtx low = alidp->bounds[r];
				rtx high = alidp->bounds[r+1];

				(void) fprintf(fp, " <%d, %d>",
				  low
				    ? ((low!=DWARF_FIRST_OR_LAST_BOUNDARY_NOTE)
						? INSN_UID(low) : -1)
				    : 0,
				  high
				    ? ((high!=DWARF_FIRST_OR_LAST_BOUNDARY_NOTE)
						? INSN_UID(high) : -1)
				    : 0);
			  }
			  (void) fprintf(fp, "\n");
			}
		}
	}

	/* Dump the sets indicating which alids are killed by a reg update */

	if (dwarf_alids_killed_by[0])
	{
		(void) fprintf(fp, "\n;; dwarf_alids_killed_by[reg]\n\n");
		for (r = 0; r < FIRST_PSEUDO_REGISTER; r++)
		{
			(void) fprintf(fp, "[%2d] ", r);
			DWARF_DISPLAY_SET(fp, dwarf_alids_killed_by[r], w);
			(void) fprintf(fp, "\n");
		}
	}

	/* Dump each decl's DWARF_DECL_LOCATIONS list. */

	(void) fprintf(fp, "\n;; dump of _DECLs\n\n");
	decl = DECL_ARGUMENTS(current_function_decl);
	for (; decl; decl = TREE_CHAIN(decl))
		dwarf_dump_decl(fp, decl);

	decl = DECL_INITIAL(current_function_decl);

	if (decl && TREE_CODE(decl) == BLOCK)
		dwarf_dump_decls_for_block(fp, BLOCK_SUBBLOCKS(decl));

	/* Dump the dataflow sets. */

	if (dwarf_in_sets)
	{
		(void) fprintf(fp, "\n;; DFA Sets (dwarf_dfa_set_size_in_words = %d)\n\n", dwarf_dfa_set_size_in_words);
		for (b=0; b < n_basic_blocks; b++)
		{
			(void) fprintf(fp, "\nBlock %d\nin   =", b);
			DWARF_DISPLAY_SET(fp, dwarf_in_sets[b], w);
			(void) fprintf(fp, "\ngen  =");
			DWARF_DISPLAY_SET(fp, dwarf_gen_sets[b], w);
			(void) fprintf(fp, "\nkill =");
			DWARF_DISPLAY_SET(fp, dwarf_kill_sets[b], w);
			(void) fprintf(fp, "\nout  =");
			DWARF_DISPLAY_SET(fp, dwarf_out_sets[b], w);
			(void) fprintf(fp, "\n");
		}
	}
}

#endif /* DWARF_DEBUGGING_INFO */
