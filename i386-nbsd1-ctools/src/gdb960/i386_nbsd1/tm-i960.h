/* Parameters for target machine Intel 960, for GDB, the GNU debugger.
   Copyright (C) 1990, 1991, 1993 Free Software Foundation, Inc.
   Contributed by Intel Corporation.
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

#ifndef TM_I960_H
#define TM_I960_H

/* Definitions to target GDB to an i960 debugged over a serial line.  */

#ifndef I80960
#define I80960
#endif

#include "target.h"

/* Target byte order (and target bitfield order) are set by command-line
   switch -G (specifies big-endian, default is little-endian) or figured out
   by inspecting mon960 after connecting. */
extern int target_byte_order;

/* Host byte order is figured out in main() */
extern int host_byte_order;

/* Two variables that (may) contain section offsets for PIC and PID.
   They will be ignored if < 0.  Defined in i960-tdep.c */
extern unsigned int picoffset;
extern unsigned int pidoffset;

/* Two flags set in main based on the command line.  Used in remote-hdi.c. */
extern int 	quiet;
extern int 	batch;

/* A flag to toggle whether to automatically reset the target when quitting. */
extern int	autoreset;

/* The starting entry point for execution on the target. */
extern unsigned long  i960_startp;

/* From i960-tdep.c: number of FP regs in target i960 architecture */
extern int num_fp_regs;	

/* From i960-tdep.c: number of special-function regs in target architecture */
extern int num_sf_regs;	

/* From i960-tdep.c: total number of regs in target architecture */
extern int num_regs;

/* From main.c: Additional command line options for handling the 
   remote-hdi.c interface. */
/* Send a BREAK to reset board first */
extern int target_initial_brk;	

/* Connect to PCI using default target */
extern int pci_dflt_conn;	

/* Flag to enable pci debug */
extern int pci_debug;

/* -pcic command line argument */
extern char *pci_config_arg;	

/* Connect to PCI using explicit vendor */
extern char *pci_vnd_arg[];	

/* Connect to PCI using explicit bus addr */
extern char *pci_bus_arg[];	

/* Name of serial port to talk to mon960 */
extern char *target_ttyname;	

/* Name of parallel port for downloading */
extern char *parallel_device;	

/* was -G seen on the command-line? */
extern int bigendian_switch;	

/* mon960 (default), */
extern char *target_type;	

/* End command-line globals */

/* The target structure that represents mon960 */
extern struct target_ops mon960_ops;

/* bytes to insert for sw bp */
extern unsigned char little_endian_break_insn[]; 

/* bytes to insert for sw bp */
extern unsigned char big_endian_break_insn[]; 

/* address that caused HW watchpoint */
extern CORE_ADDR mon960_data_bp_address;

/* fault types in string form for printing out unclaimed faults */
#define MAX_FAULT_TYPES	    16
#define MAX_FAULT_SUBTYPES   8
extern char *fault_name_table[MAX_FAULT_TYPES][MAX_FAULT_SUBTYPES];	

#define HOST_BIG_ENDIAN (HOST_BYTE_ORDER == BIG_ENDIAN)
#define HOST_LITTLE_ENDIAN (HOST_BYTE_ORDER == LITTLE_ENDIAN)

#define TARGET_BYTE_ORDER target_byte_order
#define TARGET_BIG_ENDIAN (target_byte_order == BIG_ENDIAN)
#define TARGET_LITTLE_ENDIAN (target_byte_order == LITTLE_ENDIAN)

#define SWAP_TARGET_AND_HOST(b, l) swap_target_and_host(b, l)

#define BITS_BIG_ENDIAN TARGET_BIG_ENDIAN

/* Because we support both big- and little-endian i960 targets,
   we need two different BREAKPOINT macros; default to little */
#define LITTLE_ENDIAN_BREAKPOINT {0x00, 0x3e, 0x00, 0x66}
#define BIG_ENDIAN_BREAKPOINT {0x66, 0x00, 0x3e, 0x00}
#define BREAKPOINT LITTLE_ENDIAN_BREAKPOINT

#define ADDR_BITS_REMOVE(addr) (addr)
#define ADDR_BITS_SET(addr) (addr)

#define	DEFAULT_PROMPT	"(gdb960) "

/* Hook for the SYMBOL_CLASS of a parameter when decoding DBX symbol
   information.  In the i960, parameters can be stored as locals or as
   args, depending on the type of the debug record.

   From empirical observation, gcc960 uses N_LSYM to indicate
   arguments passed in registers and then copied immediately
   to the frame, and N_PSYM to indicate arguments passed in a
   g14-relative argument block.  */

#define	DBX_PARM_SYMBOL_CLASS(type) (((int) type == (int) N_LSYM)? LOC_LOCAL_ARG: LOC_ARG)

#define IEEE_FLOAT

/* Hook for activities that need to be done whenever the symfile changes. */
#define target_clear_symtab_users mon960_clear_symtab_users

/* Can't use sizeof(long double) because some hosts (sun4) don't have it. */
#define SIZEOF_LONG_DOUBLE 16

/* Define this if the C compiler puts an underscore at the front
   of external names before giving them to the linker.  */

#define NAMES_HAVE_UNDERSCORE


/* Offset from address of function to start of its code.
   Zero on most machines.  */

#define FUNCTION_START_OFFSET 0

/* Advance ip across any function entry prologue instructions
   to reach some "real" code.  */

#define SKIP_PROLOGUE(ip)	{ ip = skip_prologue (ip); }

/* Immediately after a function call, return the saved ip.
   Can't always go through the frames for this because on some machines
   the new frame is not set up until the new function
   executes some instructions.  */

#define SETUP_ARBITRARY_FRAME(argc, argv) setup_arbitrary_frame (argc, argv)
extern struct frame_info *setup_arbitrary_frame PARAMS ((int, CORE_ADDR *));

#define SAVED_PC_AFTER_CALL(frame) (saved_pc_after_call (frame))

/* Stack grows upward */

#define INNER_THAN >

/* Nonzero if instruction at ip is a return instruction.  */

#define ABOUT_TO_RETURN(ip) (read_memory_integer(ip,4) == 0x0a000000)

/* Return 1 if P points to an invalid floating point value.
   LEN is the length in bytes.  */

#define INVALID_FLOAT(p, len) (0)

/* Say how long (ordinary) registers are.  This is a piece of bogosity
   used in push_word and a few other places; REGISTER_RAW_SIZE is the
  real way to know how big a register is.  */

#define REGISTER_SIZE 4

/* Number of machine registers.  NOTE that an i960 target may not have 
   the 4 floating-point registers.  So we make this a variable and set it
   in mon960_open.  */
#define NUM_REGS num_regs
#define TOTAL_POSSIBLE_REGS 72

/* Initializer for an array of names of registers.
   There should be TOTAL_POSSIBLE_REGS strings in this initializer.  */

#define REGISTER_NAMES { \
    /*  0 */ "pfp",  "sp",   "rip",  "r3",   "r4",   "r5",   "r6",   "r7", \
    /*  8 */ "r8",   "r9",   "r10",  "r11",  "r12",  "r13",  "r14",  "r15",\
    /* 16 */ "g0",   "g1",   "g2",   "g3",   "g4",   "g5",   "g6",   "g7", \
    /* 24 */ "g8",   "g9",   "g10",  "g11",  "g12",  "g13",  "g14",  "fp", \
    /* 32 */ "pc",   "ac",   "ip",   "tc",   "sf0",  "sf1",  "sf2",  "sf3", \
    /* 40 */ "sf4",  "sf5",  "sf6",  "sf7",  "sf8",  "sf9",  "sf10", "sf11", \
    /* 48 */ "sf12", "sf13", "sf14", "sf15", "sf16", "sf17", "sf18", "sf19", \
    /* 56 */ "sf20", "sf21", "sf22", "sf23", "sf24", "sf25", "sf26", "sf27", \
    /* 64 */ "sf28", "sf29", "sf30", "sf31", "fp0",  "fp1",  "fp2",  "fp3", \
}

/* Register numbers of various important registers (used to index
   into arrays of register names and register values).  */

#define R0_REGNUM   0	/* First local register		*/
#define SP_REGNUM   1	/* Contains address of top of stack */
#define RIP_REGNUM  2	/* Return instruction pointer (local r2) */
#define R15_REGNUM 15	/* Last local register		*/
#define G0_REGNUM  16	/* First global register	*/
#define G12_REGNUM 28   /* g12 - holds PID offset */
#define G13_REGNUM 29	/* g13 - holds struct return address */
#define G14_REGNUM 30	/* g14 - ptr to arg block / leafproc return address */
#define FP_REGNUM  31	/* Contains address of executing stack frame */
#define	PCW_REGNUM 32	/* process control word */
#define	ACW_REGNUM 33	/* arithmetic control word */
#define	IP_REGNUM  34	/* instruction pointer */
#define	TCW_REGNUM 35	/* trace control word */
#define SF0_REGNUM 36	/* First special-function register */

/* FP0_REGNUM has to be variable, not a constant, since gdb base code expects
   regs to be in a block with no gaps, and the number of SFRs varies */
#define FP0_REGNUM (SF0_REGNUM + num_sf_regs)

/* Some registers have more than one name */

#define PC_REGNUM  IP_REGNUM	/* GDB refers to ip as the Program Counter */
#define PFP_REGNUM R0_REGNUM	/* Previous frame pointer	*/

/* Total amount of space needed to store the largest possible machine
   register state, the array `registers'.  */
#define REGISTER_BYTES ((68 * 4) + (4 * 10))

/* Index within `registers' of the first byte of the space for register N.  */

#define REGISTER_BYTE(N) ( (N) < FP0_REGNUM ? \
				(4*(N)) : ((10*(N)) - (6*FP0_REGNUM)) )

#if 0

I turned this off because it complicates unwinding the frame significantly.

We DO NOT support register windows in gdb960.

Paul Reger Tue Dec 12 11:14:59 PST 1995

/* The i960 has register windows, sort of.  */

#define HAVE_REGISTER_WINDOWS

/* Is this register part of the register window system?  A yes answer
   implies that 1) The name of this register will not be the same in
   other frames, and 2) This register is automatically "saved" upon
   subroutine calls and thus there is no need to search more than one
   stack frame for it.
   
   On the i960, in fact, the name of this register in another frame is
   "mud" -- there is no overlap between the windows.  Each window is
   simply saved into the stack (true for our purposes, after having been
   flushed; normally they reside on-chip and are restored from on-chip
   without ever going to memory).  */

#define REGISTER_IN_WINDOW_P(regnum)	((regnum) <= R15_REGNUM)
#endif


/* Number of bytes of storage in the actual machine representation
   for register N.  On the i960, all regs are 4 bytes except for floating
   point, which are 10.  */

#define REGISTER_RAW_SIZE(N)		( (N) < FP0_REGNUM ? 4 : 10 )

/* Number of bytes of storage in the program's representation for register N. */

#define REGISTER_VIRTUAL_SIZE(N)	( (N) < FP0_REGNUM ? 4 : 8 )

/* Largest value REGISTER_RAW_SIZE can have.  */

#define MAX_REGISTER_RAW_SIZE 10

/* Largest value REGISTER_VIRTUAL_SIZE can have.  */

#define MAX_REGISTER_VIRTUAL_SIZE 8

/* Nonzero if register N requires conversion from raw format to virtual
   format.  */

#define REGISTER_CONVERTIBLE(N) ((N) >= FP0_REGNUM)

/* Convert data from raw format for register REGNUM
   to virtual format for register REGNUM.  */

#ifdef IMSTG
#define REGISTER_CONVERT_TO_VIRTUAL(REGNUM,TYPE,FROM,TO) \
{ if ((REGNUM) >= FP0_REGNUM)   {                        \
    extended_to_double ((FROM), ((double *)(TO)));       \
    SWAP_TARGET_AND_HOST(TO,sizeof(double));             \
  }                                                      \
  else                                                   \
    bcopy ((FROM), (TO), 4); }
#else
#define REGISTER_CONVERT_TO_VIRTUAL(REGNUM,TYPE,FROM,TO) \
{ if ((REGNUM) >= FP0_REGNUM)                            \
    extended_to_double ((FROM), ((double *)(TO)));       \
  else                                                   \
    bcopy ((FROM), (TO), 4); }
#endif

/* Convert data from virtual format for register REGNUM
   to raw format for register REGNUM.  */

#ifdef IMSTG
#define REGISTER_CONVERT_TO_RAW(TYPE,REGNUM,FROM,TO) \
{ if ((REGNUM) >= FP0_REGNUM)  {                     \
    SWAP_TARGET_AND_HOST(FROM,sizeof(double));       \
    double_to_extended (((double *)(FROM)), (TO));   \
  }                                                  \
  else                                               \
    bcopy ((FROM), (TO), 4); }
#else
#define REGISTER_CONVERT_TO_RAW(TYPE,REGNUM,FROM,TO) \
{ if ((REGNUM) >= FP0_REGNUM)                        \
    double_to_extended (((double *)(FROM)), (TO));   \
  else                                  \
    bcopy ((FROM), (TO), 4); }
#endif

/* Return the GDB type object for the "standard" data type
   of data in register N.  */

#define REGISTER_VIRTUAL_TYPE(N) ((N) < FP0_REGNUM ? \
					builtin_type_int : builtin_type_double)

/* Suppress code for retrieval of host inferior process registers */
#define FETCH_INFERIOR_REGISTERS

/* Amount ip must be decremented by after a breakpoint. */
#define DECR_PC_AFTER_BREAK 0

/* Macro for new HW breakpoint, watchpoint features added in gdb 4.13. */
#define TARGET_CAN_USE_HARDWARE_WATCHPOINT(type, count, other) \
    mon960_check_hardware_resources(type, count)

/* Macros for additional command-line options */

#ifdef DOS
#define PCI_OPTS_1 \
	{"pci", no_argument, 0, 1011},        /* pci download */ \
	{"pcib", required_argument, 0, 1012}, /* PCI download by bus addr */ \
	{"pcic", required_argument, 0, 1013}, /* configure PCI comm */ \
	{"pciv", required_argument, 0, 1014}, /* PCI download by vnd & dvc id */\
	{"pcid", no_argument, 0, 1015},       /* pci debug (undocumented) */
#else
#define PCI_OPTS_1
#endif

#define	ADDITIONAL_OPTIONS \
	{"G", no_argument, 0, 'G'}, /* big-endian switch */ \
	{"brk", no_argument, &target_initial_brk, 1}, \
	{"remote", required_argument, 0, 1004},  /* remote serial port */ \
	{"r", required_argument, 0, 1004}, /* remote serial port */ \
	{"target", required_argument, 0, 1006}, /* connect on startup */ \
	{"t", required_argument, 0, 1006}, /* connect on startup */ \
	{"picoffset", required_argument, 0, 1007}, /* for PI code */ \
	{"pc", required_argument, 0, 1007}, /* for PI code */ \
	{"pidoffset", required_argument, 0, 1008}, /* for PI data */ \
	{"pd", required_argument, 0, 1008}, /* for PI data */ \
	{"pixoffset", required_argument, 0, 1009}, /* PI code and data*/\
	{"px", required_argument, 0, 1009}, /* PI code and data*/ \
	{"download", required_argument, 0, 1010}, /* parallel DL device */ \
	{"D", required_argument, 0, 1010}, /* parallel DL device */ \
	{"parallel", required_argument, 0, 1010}, /* parallel DL device */ \
	PCI_OPTS_1


#ifdef DOS
#define PCI_OPTS_2 \
		case 1011:	/* -pci option: download via PCI bus	\
				 * using default target.		\
				 */					\
		    pci_dflt_conn = 1;					\
		    break;						\
		case 1012:	/* -pcib option: download via PCI bus	\
				 * using bus address.			\
				 */					\
		    pci_bus_arg[0] = optarg;				\
		    pci_bus_arg[1] = argv[optind];			\
		    if (pci_bus_arg[1])					\
		    {							\
			optind++;					\
			pci_bus_arg[2] = argv[optind];			\
			if (pci_bus_arg[2])				\
			    optind++;					\
		    }							\
		    break;						\
		case 1013:	/* -pcic option: configure PCI comm */	\
		    pci_config_arg = optarg;				\
		    break;						\
		case 1014:	/* -pciv option: download via PCI bus	\
				 * using bus vendor & device ID.	\
				 */					\
		    pci_vnd_arg[0] = optarg;				\
		    pci_vnd_arg[1] = argv[optind];			\
		    if (pci_vnd_arg[1])					\
			optind++;					\
		    break;						\
		case 1015:	/* -pcid option: enable PCI debug	\
				 * using default target.		\
				 */					\
		    pci_debug = 1;					\
		    break;
#else
#define PCI_OPTS_2
#endif

#define	ADDITIONAL_OPTION_CASES	\
 		case 'G':	/* big-endian switch */			\
		  target_byte_order = BIG_ENDIAN;			\
		  bigendian_switch = 1;					\
                  set_break_insn(big_endian_break_insn);                \
		  break;						\
		case 1004:	/* -r option:  remote auto-start */ \
		  target_ttyname = optarg;	\
		  break;			\
		case 1006:	/* -target option:  connect at startup */ \
		  target_type = optarg;                          	\
		  break;						\
 		case 1007:      /* -pc option: set picoffset */ 	\
		  picoffset = (unsigned long) strtol(optarg, NULL, 0);	\
		  break;						\
 		case 1008:      /* -pd option: set pidoffset */ 	\
		  pidoffset = (unsigned long) strtol(optarg, NULL, 0);	\
		  break;						\
 		case 1009:      /* -px option: set both */ 		\
		  picoffset = (unsigned long) strtol(optarg, NULL, 0);	\
		  pidoffset = picoffset;				\
		  break;						\
		case 1010:	/* -D option: use parallel download */	\
		  parallel_device = optarg;				\
		  break;						\
		PCI_OPTS_2


#ifdef DOS
#define	ADDITIONAL_OPTION_HELP \
	"\
  -brk               Send a break to an i960 target to reset it.\n\
  -G                 Informs gdb960 that the target has big-endian memory.\n\
  -r SERIAL          Open remote session to SERIAL port.\n\
  -parallel PARALLEL Download through PARALLEL port.\n\
  -pci               Download via PCI bus.  Selected target defaults to\n\
                        Cyclone/PLX baseboard.\n\
  -pcib BUS DEV FUNC Download via PCI bus.  Select target by specifying\n\
                        a PCI address.  Hex args required.\n\
  -pcic {io | mmap}  Configure PCI communication.\n\
                        io   -> exchange data via I/O space\n\
                        mmap -> exchange data via memory mapped access\n\
  -pciv VNDID DEVID  Download via PCI bus.  Select target by specifying\n\
                        a PCI vendor & device ID.  Hex args required.\n\
  -t TARGET_TYPE     Identifies target type: mon960 only, currently.\n\
  -pc OFFSET         Add the number OFFSET to text sections before loading.\n\
  -pd OFFSET         Add the number OFFSET to data sections before loading.\n\
  -px OFFSET         Use same OFFSET for both text and data sections.\n\
"
#else
#define	ADDITIONAL_OPTION_HELP \
	"\
  -brk               Send a break to an i960 target to reset it.\n\
  -G                 Informs gdb960 that the target has big-endian memory.\n\
  -parallel PARALLEL Download through PARALLEL port.\n\
  -r SERIAL          Open remote session to SERIAL port.\n\
  -t TARGET_TYPE     Identifies target type: mon960 only, currently.\n\
  -pc OFFSET         Add the number OFFSET to text sections before loading.\n\
  -pd OFFSET         Add the number OFFSET to data sections before loading.\n\
  -px OFFSET         Use same OFFSET for both text and data sections.\n\
"
#endif

/*
 * If target_type is set to a supported target (currently only mon960) AND
 * -r has been set to a value (indicating user desires a serial connection)
 * or the user has specified one of the pci options (indicating the user
 * desires a pci comm connection), then we connect to the target now.  Note
 * that target_type will never be null (defaults to "mon960")
 */
#define ADDITIONAL_OPTION_HANDLER \
if ( ! SET_TOP_LEVEL() ) { \
    if ( ! strcmp(target_type,"mon960") ) { \
	if (target_ttyname || pci_dflt_conn || pci_vnd_arg[0] || pci_bus_arg[0]) { \
	    char tmp_buff[192]; \
	    char scratch[64]; \
	    sprintf(tmp_buff, "target mon960 %s %s %s %s %s %s %s", \
		    target_ttyname ? target_ttyname : "", \
		    parallel_device ? "-parallel" : "", \
		    parallel_device ? parallel_device : "", \
		    pci_dflt_conn ? "-pci" : "", \
		    pci_debug ? "-pcid" : "", \
		    pci_config_arg ? "-pcic" : "", \
		    pci_config_arg ? pci_config_arg : ""); \
	    if (pci_vnd_arg[0]) { \
		sprintf(scratch, " -pciv %s %s", pci_vnd_arg[0], \
			pci_vnd_arg[1] ? pci_vnd_arg[1] : ""); \
		strcat(tmp_buff, scratch); \
	    } \
	    if (pci_bus_arg[0]) { \
		sprintf(scratch, " -pcib %s %s %s", pci_bus_arg[0], \
			pci_bus_arg[1] ? pci_bus_arg[1] : "", \
			pci_bus_arg[2] ? pci_bus_arg[2] : ""); \
		strcat(tmp_buff, scratch); \
	    } \
	    execute_command(tmp_buff, ! batch); \
	    /* if we have an exec file we further do a download */ \
	    if ( ! SET_TOP_LEVEL() && execarg && get_exec_file(0) ) \
		target_load (execarg, ! batch); \
	} \
    } \
    else { \
	fprintf(stderr,"Unknown target type %s\n",target_type); \
	fflush(stderr); \
	exit(1); \
    } \
}

/* Macros for understanding function return values */

/* Does the specified function use the "struct returning" convention
   or the "value returning" convention?  The "value returning" convention
   almost invariably returns the entire value in registers.  The
   "struct returning" convention often returns the entire value in
   memory, and passes a pointer (out of or into the function) saying
   where the value (is or should go).

   Since this sometimes depends on whether it was compiled with GCC,
   this is also an argument.  This is used in call_function to build a
   stack, and in value_being_returned to print return values.

   On i960, a structure is returned in registers g0-g3, if it will fit.
   If it's more than 16 bytes long, g13 pointed to it on entry.  */

#define USE_STRUCT_CONVENTION(gcc_p, type) (TYPE_LENGTH (type) > 16)

/* Extract from an array REGBUF containing the (raw) register state
   a function return value of type TYPE, and copy that, in virtual format,
   into VALBUF.  This is only called if USE_STRUCT_CONVENTION for this
   type is 0.
   We can not just use memcpy(VALBUF, REGBUF+REGISTER_BYTE(G0_REGNUM), TYPE_LENGTH (TYPE))
   because we loose when the type lengths are any of: 1,2,3,5,6,7,9,10,11,13,14, or 15
   And the code is big endian.
*/

#define EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF) extract_return_value(TYPE,REGBUF,VALBUF)

/* If USE_STRUCT_CONVENTION produces a 1, 
   extract from an array REGBUF containing the (raw) register state
   the address in which a function should return its structure value,
   as a CORE_ADDR (or an expression that can be used as one).

   Address of where to put structure was passed in in global
   register g13 on entry.  God knows what's in g13 now.  The
   (..., 0) below is to make it appear to return a value, though
   actually all it does is call recov_err().  */

/* Write into appropriate registers a function return value
   of type TYPE, given in virtual format, for "value returning" functions.
  
   For 'return' command:  not (yet) implemented for i960.  */

#define STORE_RETURN_VALUE(TYPE,VALBUF) \
    error ("Returning values from functions is not implemented in i960 gdb")

/* Store the address of the place in which to copy the structure the
   subroutine will return.  This is called from call_function. */

#define STORE_STRUCT_RETURN(ADDR, SP) \
    error ("Returning structures greater than 16 bytes as values from functions is not implemented in i960 gdb")

/* Describe the pointer in each stack frame to the previous stack frame
   (its caller).  */

/* FRAME_CHAIN takes a frame's nominal address
   and produces the frame's chain-pointer.

   FRAME_CHAIN_COMBINE takes the chain pointer and the frame's nominal address
   and produces the nominal address of the caller frame.

   However, if FRAME_CHAIN_VALID returns zero,
   it means the given frame is the outermost one and has no caller.
   In that case, FRAME_CHAIN_COMBINE is not used.  */

/* We cache information about saved registers in the frame structure,
   to save us from having to re-scan function prologues every time
   a register in a non-current frame is accessed.  */

#define EXTRA_FRAME_INFO 	\
	struct frame_saved_regs *fsr;	\
	CORE_ADDR arg_pointer;

/* Zero the frame_saved_regs pointer when the frame is initialized,
   so that FRAME_FIND_SAVED_REGS () will know to allocate and
   initialize a frame_saved_regs struct the first time it is called.
   Set the arg_pointer to -1, which is not valid; 0 and other values
   indicate real, cached values.  PS: first param not used, added
   for compatibility with gdb 4.10 sources. */

#define INIT_EXTRA_FRAME_INFO(fromleaf, fi) ((fi)->fsr = 0, (fi)->arg_pointer = -1)

/* On the i960, we get the chain pointer by reading the PFP saved
   on the stack and clearing the status bits.  */

#define FRAME_CHAIN(thisframe) \
  (read_memory_integer (FRAME_FP(thisframe), 4) & ~0xf)

#define FRAME_CHAIN_COMBINE(chain, thisframe) (chain)

/* FRAME_CHAIN_VALID returns zero if the given frame is the outermost one
   and has no caller.  In that case, FRAME_CHAIN_COMBINE is not used. */

#define	FRAME_CHAIN_VALID(chain, thisframe) \
	frame_chain_valid (chain, thisframe)

/* A macro that tells us whether the function invocation represented
   by FI does not have a frame on the stack associated with it.  If it
   does not, FRAMELESS is set to 1, else 0.  */

#define FRAMELESS_FUNCTION_INVOCATION(FI, FRAMELESS) \
  { (FRAMELESS) = (leafproc_return ((FI)->pc) != 0); }

/* Note that in the i960 architecture the return pointer is saved in the
   *caller's* stack frame.
  
   Make sure to zero low-order bits because of bug in 960CA A-step part
   (instruction addresses should always be word-aligned anyway).  */

#define FRAME_SAVED_PC(frame) \
			((read_memory_integer(FRAME_CHAIN(frame)+8,4)) & ~3)

/* On the i960, FRAME_ARGS_ADDRESS should return the value of
   g14 as passed into the frame, if known.  We need a function for this.
   We cache this value in the frame info if we've already looked it up.  */

#define FRAME_ARGS_ADDRESS(fi) 	\
  (((fi)->arg_pointer != -1)? (fi)->arg_pointer: frame_args_address (fi, 0))

/* This is the same except it should return 0 when
   it does not really know where the args are, rather than guessing.
   This value is not cached since it is only used infrequently.  */

#define	FRAME_ARGS_ADDRESS_CORRECT(fi)	(frame_args_address (fi, 1))

#define FRAME_LOCALS_ADDRESS(fi)	(fi)->frame

/* Set NUMARGS to the number of args passed to a frame.
   Can return -1, meaning no way to tell.  */

#define FRAME_NUM_ARGS(numargs, fi)	(numargs = -1)

/* Return number of bytes at start of arglist that are not really args.  */

#define FRAME_ARGS_SKIP 0

/* Produce the positions of the saved registers in a stack frame.  */

#define FRAME_FIND_SAVED_REGS(frame_info_addr, sr) \
	frame_find_saved_regs (frame_info_addr, &sr, (CORE_ADDR *) 0)

/* Things needed for making calls to functions in the inferior process */

/* Push an empty stack frame, to record the current ip, and forming a new
   dummy context for the function to be called. */

#define PUSH_DUMMY_FRAME	push_dummy_frame(nargs,args)

#define PUSH_ARGUMENTS(nargs, args, sp, struct_return, struct_addr) \
	push_dummy_arguments(nargs, args, sp, struct_return, struct_addr);

/*
 * We use default definition of PC_IN_CALL_DUMMY found in inferior.h
 * Too, we use default definition of CALL_DUMMY_LOCATION of ON_STACK.
 */

/* Discard from the stack the innermost frame, restoring all registers.
   In gdb960, this is only used in calling an arbitrary function.  */

#define POP_FRAME \
	pop_frame ()

/* This sequence of words is the instructions
  
        flushreg
  	callx 0x00000000
  	ret
 */

#define CALL_DUMMY { 0x66003e80, 0x86003000, 0x00000000, 0x66003e00 }

#define CALL_DUMMY_START_OFFSET      0  /* Start execution at beg of dummy */
#define CALL_DUMMY_BREAKPOINT_OFFSET 12 /* Where the fmark is. */

/* Insert the function address into a call sequence of the above form 
   stored at 'dummyname'.  Note that we ignore most of the arguments here,
   leaving them to other parts of call dummy support.
 */

#define FIX_CALL_DUMMY(dummyname, SIP_IG, fun, NARGS_IG, ARGS_IG, VT_IG, UG_VT) \
	if (1) {\
		store_unsigned_integer((((int *)dummyname)+2),4,fun);\
	}

/* Function declarations for functions used within macros.  DON'T make these
   into prototypes because the include nesting is tricky. */
extern CORE_ADDR  skip_prologue ();
extern CORE_ADDR saved_pc_after_call ();
extern void extract_return_value ();
extern CORE_ADDR leafproc_return ();
extern CORE_ADDR frame_args_address ();
extern void frame_find_saved_regs ();
extern void push_dummy_frame ();
extern void push_dummy_arguments ();

/* Function prototypes for i960-specific functions. */
int mon960_arch PARAMS ((void));
int mon960_connected PARAMS ((void));
void mon960_clear_symtab_users PARAMS ((void));
void init_gmu_cmd_cache PARAMS ((void));
void reprogram_gmu PARAMS ((void));
#endif /* TM_I960_H */
