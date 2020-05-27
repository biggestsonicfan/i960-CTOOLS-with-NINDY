#ifndef PROF_INCLUDED
#define PROF_INCLUDED

/*
 *
 *  The instrumented code writes to a table called 'profile_data'.
 *  The 'profile_data' table is set up specially by the linker. The
 *  'profile_data' table starts at the symbol 'profile_data_start' and
 *  its length in bytes is defined by 'profile_data_length'.
 *
 *  For each file that is compiled, the area that will be used out of
 *  'profile_data' for the file will start at the symbol
 *  'profile_data_start.<filename>'.
 *
 *  The symbol 'profile_data_length.<filename>' will be initialized by the
 *  compiler to the length in bytes of the piece of 'profile_data' that
 *  the file needs.
 *
 *  It will be the linkers responsibility to set aside enough space
 *  in 'profile_data' for all the .o files in the executable that will be
 *  collecting profile information.  The linker uses
 *  'profile_data_length.<filename>' to find out how much space is needed
 *  out of 'profile_data' for a .o file, it allocates the space and sets
 *  'profile_data_start.<filename>' to the beginning.
 *
 *  The linker also sets up the 'profile_file_offsets' table. This table is
 *  a piece of inititalized data that gives the starting offset into
 *  'profile_data' for each file.  This area starts at the symbol
 *  'profile_file_offsets_start' and its length in bytes is defined by 
 *  'profile_file_offsets_length'. Both of these symbols are set up by the
 *  linker.
 *
 *  'profile_data' is just an array of 32 bit unsigned integers.
 *
 *  'profile_file_offsets' is an array of 8 byte structures each of which
 *  consists of an index into a string table, and an index into the
 *  'profile_data' table.  The index into the string table is at byte position
 *  0, and the index into the data is at byte position 4 from the beginning
 *  of the structure.  The index into the string table is a byte offset
 *  that is relative to the base of the profile_file_offsets table.  The
 *  data index is a byte offset relative to the base of the data.  The end
 *  of the file offset table is indicated by a string table index of 0.
 *
 *  The format of the data file that is produced from running instrumented
 *  code is as follows:
 *
 *      --------------------------------------
 *      |      'profile_data_length'         |     4 bytes
 *      --------------------------------------
 *      |                                    |
 *      |                                    |
 *      |                                    |
 *      |                                    |     profile_data_length bytes
 *      |         'profile_data'             |
 *      |                                    |
 *      |                                    |
 *      |                                    |
 *      --------------------------------------
 *      |    'profile_file_offsets_length'   |     4 bytes
 *      --------------------------------------
 *      |                                    |
 *      |                                    |
 *      |                                    |
 *      |       'profile_file_offsets'       |
 *      |                                    |     profile_file_offsets_length
 *      |- - - - - - - - - - - - - - - - - - |     bytes
 *      |                                    |
 *      |             strings                |
 *      |                                    |
 *      --------------------------------------
 *
 *  The profile table format is not known by the compiler anymore.  Instead
 *  a simple array indexed by basic block number is provided in the data base.
 *  The entry in the array contains the count for the basic block indicated.
 *  The profile code uses this information for annotation.
 */

/* 
 * RTL fields used.
 *
 *    int INSN_PROF_DATA_INDEX(insn)
 *
 *    This gives the basic block number associated with the instruction.
 *    This allows indexing into the profile information in order to do
 *    annotation, and is also used by gcdm960 to determine counts for
 *    call-sites.
 */

extern int prof_n_bblocks;
extern int prof_n_counters;
extern int prof_func_block;
extern int prof_func_used_prof_info;

extern void imstg_prof_annotate();
extern void imstg_prof_annotate_inline();
extern void imstg_compute_prob_expect();
extern void imstg_prof_min();
extern void prof_instrument_begin();
extern void prof_instrument_end();

#endif
