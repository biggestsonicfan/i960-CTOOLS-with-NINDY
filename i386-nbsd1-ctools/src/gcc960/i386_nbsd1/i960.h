/* Definitions of target machine for GNU compiler, for Intel 80960
   Copyright (C) 1992 Free Software Foundation, Inc.
   Contributed by Steven McGeady, Intel Corp.
   Additional Work by Glenn Colon-Bonet, Jonathan Shapiro, Andy Wilson
   Converted to GCC 2.0 by Jim Wilson and Michael Tiemann, Cygnus Support.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Note that some other tm.h files may include this one and then override
   many of the definitions that relate to assembler syntax.  */

#define abort() fancy_abort()

/* Turn on IEEE emulation */
#define IMSTG_REAL 1

/* Names to predefine in the preprocessor for this target machine.  */
#define CPP_PREDEFINES "-Di960 -Di80960 -DI960 -DI80960"

#define INCLUDE_DEFAULTS	{{0,0}} 
#define GCC_INCLUDE_DIR		""	/* just to inhibit messages */


/* Override conflicting target switch options.
   Doesn't actually detect if more than one -mARCH option is given, but
   does handle the case of two blatantly conflicting -mARCH options.  */
#define OVERRIDE_OPTIONS i960_override_options() /* in i960.c */

/* Enable changes to support builtin unaligned macros from unalign.h [SVW] */
#define IMSTG_UNALIGNED_MACROS 

/* Enable changes to support i960 big-endian target applications [SVW] */
/* Turned on by default */
#define IMSTG_BIG_ENDIAN

/* Don't enable anything by default.  The user is expected to supply a -mARCH
   option.  If none is given, then -mkb is added by CC1_SPEC.  */
#define TARGET_DEFAULT 0

/* Target machine storage layout.  */

/* Define this if most significant bit is lowest numbered
   in instructions that operate on numbered bit-fields.  */
#define BITS_BIG_ENDIAN 0

/* bytes_big_endian will be set to true based on a command line option.
 * It indicates that the target memory for data references is big-endian,
 * rather than the default little-endian orientation of the 80960CA.
 * It needs to be defined always so the insn_*.c will compile correctly
 * (even if not using IMSTG_BIG_ENDIAN).
 */
extern int bytes_big_endian;

/* Define this if most significant byte of a word is the lowest numbered.
   The i960 case be either big endian or little endian.  We only support
   little endian, which is the most common.  */

#ifdef IMSTG_BIG_ENDIAN
#define BYTES_BIG_ENDIAN 1 /* all uses will be under "if (bytes_big_endian)" control */
#else
#define BYTES_BIG_ENDIAN 0
#endif /* IMSTG_BIG_ENDIAN */

/* Define this if most significant word of a multiword number is lowest
   numbered.  */
#define WORDS_BIG_ENDIAN 0

/* Number of bits in an addressible storage unit.  */
#define BITS_PER_UNIT 8

/* Bitfields can cross word boundaries.  */
/*#define BITFIELD_NBYTES_LIMITED 1 */

/* Width in bits of a "word", which is the contents of a machine register.
   Note that this is not necessarily the width of data type `int';
   if using 16-bit ints on a 68000, this would still be 32.
   But on a machine with 16-bit registers, this would be 16.  */
#define BITS_PER_WORD 32

/* Width of a word, in units (bytes).  */
#define UNITS_PER_WORD 4

/* Width in bits of a pointer.  See also the macro `Pmode' defined below.  */
#define POINTER_SIZE 32

/* Width in bits of a double. */
#define DOUBLE_TYPE_SIZE	(TARGET_DOUBLE4 ? 32 : 64)

/* Width in bits of a long double.  */
/* This same definition must appear at the end of real.h */
#define	LONG_DOUBLE_TYPE_SIZE	(TARGET_LONGDOUBLE4 ? 32 : 128)

/* Allocation boundary (in *bits*) for storing pointers in memory.  */
#define POINTER_BOUNDARY 32

/* Allocation boundary (in *bits*) for storing arguments in argument list.  */
#define PARM_BOUNDARY 32

/* Boundary (in *bits*) on which stack pointer should be aligned.  */
#define STACK_BOUNDARY 128

/* Allocation boundary (in *bits*) for the code of a function.  */
extern unsigned i960_function_boundary;
#define FUNCTION_BOUNDARY (i960_function_boundary)

/* Alignment of field after `int : 0' in a structure.  */
#define EMPTY_FIELD_BOUNDARY 32

/* This makes zero-length anonymous fields lay the next field
   at a word boundary.  It also makes the whole struct have
   at least word alignment if there are any bitfields at all.  */
/* It also makes us pay attention to the fields's type, which we do
   not want.  The rules enforced by this macro aren't even close
   to the 960 ABI.  See the ROUND_FIELD macro.  */
/* #define PCC_BITFIELD_TYPE_MATTERS 1 */

/* Every structure's size must be a multiple of this.  */
#define STRUCTURE_SIZE_BOUNDARY 8

/* No data type wants to be aligned rounder than this.
   Extended precision floats gets 4-word alignment.  */
#define BIGGEST_ALIGNMENT 128

/* Define this if move instructions will actually fail to work
   when given unaligned data.
   80960 will work even with unaligned data, but it is slow.  */

/* Standard register usage.  */
/* Modes for condition codes.  */
#define C_MODES		\
  ((1 << (int) CCmode) | (1 << (int) CC_UNSmode) | (1<< (int) CC_CHKmode))

/* Modes for single-word (and smaller) quantities.  */
#define S_MODES						\
 (~C_MODES						\
  & ~ ((1 << (int) DImode) | (1 << (int) TImode)	\
       | (1 << (int) DFmode) | (1 << (int) TFmode)))

/* Modes for double-word (and smaller) quantities.  */
#define D_MODES					\
  (~C_MODES					\
   & ~ ((1 << (int) TImode) | (1 << (int) TFmode)))

/* Modes for quad-word quantities.  */
#define T_MODES (~C_MODES)

/* Modes for single-float quantities.  */
#define SF_MODES ((1 << (int) SFmode))

/* Modes for double-float quantities.  */
#define DF_MODES (SF_MODES | (1 << (int) DFmode) | (1 << (int) SCmode))

/* Modes for quad-float quantities.  */
#define TF_MODES (DF_MODES | (1 << (int) TFmode) | (1 << (int) DCmode))

#define REGISTER_MOVE_COST(X,Y) i960_reg_move_cost ((X),(Y))

#ifdef IMSTG_COPREGS
#include "i_copreg.h"

#define IS_COP_CLASS(C) ((C)>=COP0_REGS && (C)<COP0_REGS+COP_NUM_CLASSES)
#define IS_COP_REG(R) (( (R)>=COP_FIRST_REGISTER && (R)<=COP_LAST_REGISTER ))

#else

#define COP_NUM_REGISTERS 0	/* No co-processor regs */
#define COP_NUM_CLASSES   0	/* No co-processor classes */
#define ALL_REG_CONTENTS {0xffffffff,0x3f}

#define IS_COP_CLASS(C) 0
#define IS_COP_REG(R)   0

#define COP_REG_CLASS_ENUM	/* */
#define COP_REGNO_MODE_OK_INIT	/* */
#define COP_FIXED_REGISTERS	/* */
#define COP_CALL_USED_REGISTERS	/* */
#define COP_REG_ALLOC_ORDER	/* */
#define COP_REG_CLASS_NAMES	/* */
#define COP_REG_CONTENTS	/* */
#define COP_REG_NAMES		/* */

#endif

#define	GENERAL_REGS	(( LOCAL_OR_GLOBAL_REGS ))

#define REGNO_MODE_OK_INIT { \
  T_MODES, S_MODES, D_MODES, S_MODES, T_MODES, S_MODES, D_MODES, S_MODES, \
  T_MODES, S_MODES, D_MODES, S_MODES, T_MODES, S_MODES, D_MODES, S_MODES, \
  T_MODES, S_MODES, D_MODES, S_MODES, T_MODES, S_MODES, D_MODES, S_MODES, \
  T_MODES, S_MODES, D_MODES, S_MODES, T_MODES, S_MODES, D_MODES, S_MODES, \
  TF_MODES, TF_MODES, TF_MODES, TF_MODES, C_MODES, 0, \
  COP_REGNO_MODE_OK_INIT \
};

/* Number of actual hardware registers.
   The hardware registers are assigned numbers for the compiler
   from 0 to just below FIRST_PSEUDO_REGISTER.
   All registers that the compiler knows about must be given numbers,
   even those that are not normally considered general registers.

   Registers 0-15 are the global registers (g0-g15).
   Registers 16-31 are the local registers (r0-r15).
   Register 32-35 are the fp registers (fp0-fp3).
   Register 36 is the condition code register.
   Register 37 is unused.  */

#define FIRST_PSEUDO_REGISTER ((38+COP_NUM_REGISTERS))

/* 1 for registers that have pervasive standard uses and are not available
   for the register allocator.  On 80960, this includes the frame pointer
   (g15), the previous FP (r0), the stack pointer (r1), the return
   instruction pointer (r2), and the argument pointer (g14).  */
#define FIXED_REGISTERS  \
 {0, 0, 0, 0, 0, 0, 0, 0,	\
  0, 0, 0, 0, 0, 0, 1, 1,	\
  1, 1, 1, 0, 0, 0, 0, 0,	\
  0, 0, 0, 0, 0, 0, 0, 0,	\
  0, 0, 0, 0, 1, 1, \
  COP_FIXED_REGISTERS \
 }

/* 1 for registers not available across function calls.
   These must include the FIXED_REGISTERS and also any
   registers that can be used without being saved.
   The latter must include the registers where values are returned
   and the register where structure-value addresses are passed.
   Aside from that, you can include as many other registers as you like.  */

/* On the 80960, note that:
	g0..g3 are used for return values,
	g0..g7 may always be used for parameters,
	g8..g11 may be used for parameters, but are preserved if they aren't,
	g12 is always preserved, but otherwise unused,
	g13 is the struct return ptr if used, or temp, but may be trashed,
	g14 is the leaf return ptr or the arg block ptr otherwise zero,
		must be reset to zero before returning if it was used,
	g15 is the frame pointer,
	r0 is the previous FP,
	r1 is the stack pointer,
	r2 is the return instruction pointer,
	r3-r15 are always available,
	fp0..fp3 are never available.  */
#define CALL_USED_REGISTERS  \
 {1, 1, 1, 1, 1, 1, 1, 1,	\
  0, 0, 0, 0, 0, 1, 1, 1,	\
  1, 1, 1, 0, 0, 0, 0, 0,	\
  0, 0, 0, 0, 0, 0, 0, 0,	\
  1, 1, 1, 1, 1, 1, \
  COP_CALL_USED_REGISTERS \
 }

#define REG_SAVE_COST(REG) i960_reg_save_cost(REG)

/* If no fp unit, make all of the fp registers fixed so that they can't
   be used.  */
#define	CONDITIONAL_REGISTER_USAGE	\
  do { \
    if (!TARGET_NUMERICS) \
      fixed_regs[32] = fixed_regs[33] = fixed_regs[34] = fixed_regs[35] = 1;\
    if (pid_flag || TARGET_PID || TARGET_PID_SAFE) \
      global_regs[PID_BASE_REGNUM] =   /* g12 used for data base pointer */ \
      fixed_regs[PID_BASE_REGNUM] = \
      call_used_regs[PID_BASE_REGNUM] = 1; \
  } while (0);

/* Return number of consecutive hard regs needed starting at reg REGNO
   to hold something of mode MODE.
   This is ordinarily the length in words of a value of mode MODE
   but can be less for certain modes in special long registers.

   On 80960, ordinary registers hold 32 bits worth, but can be ganged
   together to hold double or extended precision floating point numbers,
   and the floating point registers hold any size floating point number */
#define HARD_REGNO_NREGS(REGNO, MODE)   \
  ((REGNO) < 32							\
   ? (((MODE) == VOIDmode)					\
      ? 1 : ((GET_MODE_SIZE (MODE) + UNITS_PER_WORD - 1) / UNITS_PER_WORD)) \
   : ((REGNO) < FIRST_PSEUDO_REGISTER) ? 1 : 0)

/* Value is 1 if hard register REGNO can hold a value of machine-mode MODE.
   On 80960, the cpu registers can hold any mode but the float registers
   can only hold SFmode, DFmode, or TFmode.  */
#define HARD_REGNO_MODE_OK(REGNO, MODE) \
  ((hard_regno_mode_ok[REGNO] & (1 << (int) (MODE))) != 0)

/* Value is 1 if it is a good idea to tie two pseudo registers
   when one has mode MODE1 and one has mode MODE2.
   If HARD_REGNO_MODE_OK could produce different values for MODE1 and MODE2,
   for any hard reg, then this must be 0 for correct output.  */

#define MODES_TIEABLE_P(MODE1, MODE2) \
  ((MODE1) == (MODE2) || GET_MODE_CLASS (MODE1) == GET_MODE_CLASS (MODE2))

/* Value is 1 if the given register is reserved for a specific purpose for this
 * function, else 0.  This check, and clearing of hard_regno_mode_ok[REGNO],
 * are used to treat a register as fixed on a per function basis.
 */
#define HARD_REGNO_FUNCTION_RESERVED_P(REGNO) \
  (i960_hard_regno_function_reserved_p(REGNO))

/* Specify the registers used for certain standard purposes.
   The values of these macros are register numbers.  */

/* 80960 pc isn't overloaded on a register that the compiler knows about.  */
/* #define PC_REGNUM  */

/* Register to use for pushing function arguments.  */
#define STACK_POINTER_REGNUM 17

/* First assigned argument starts way under stack top */
#define STACK_POINTER_OFFSET (-current_function_outgoing_args_size)
#define STACK_DYNAMIC_OFFSET(F) (-current_function_outgoing_args_size)

/* Base register for access to local variables of the function.  */
#define FRAME_POINTER_REGNUM 15

/* Value should be nonzero if functions must have frame pointers.
   Zero means the frame pointer need not be set up (and parms
   may be accessed via the stack pointer) in functions that seem suitable.
   This is computed in `reload', in reload1.c.  */
#define FRAME_POINTER_REQUIRED \
  (!leaf_function_p() || i960_interrupt_handler())

/* C statement to store the difference between the frame pointer
   and the stack pointer values immediately after the function prologue.  */

#define INITIAL_FRAME_POINTER_OFFSET(VAR) \
  do { (VAR) = -(compute_frame_size (get_frame_size ()) + 64); } while (0)

/* Base register for access to arguments of the function.  */
#define ARG_POINTER_REGNUM 14

/* Register in which static-chain is passed to a function.
   On i960, we use r3.  */
#define STATIC_CHAIN_REGNUM 19
#define TARGET_CAVE_PIC_REGNUM 19
 
/* Functions which return large structures get the address
   to place the wanted value at in g13.  */

#define STRUCT_VALUE_REGNUM 13 

#define CC_REGNUM 36

/* The order in which to allocate registers.  */

#define	REG_ALLOC_ORDER	\
{  4, 5, 6, 7, 0, 1, 2, 3, 13,	 /* g4, g5, g6, g7, g0, g1, g2, g3, g13  */ \
  20, 21, 22, 23, 24, 25, 26, 27,/* r4, r5, r6, r7, r8, r9, r10, r11  */    \
  28, 29, 30, 31, 19, 8, 9, 10,	 /* r12, r13, r14, r15, r3, g8, g9, g10  */ \
  11, 12,			 /* g11, g12  */			    \
  32, 33, 34, 35,		 /* fp0, fp1, fp2, fp3  */		    \
  /* We can't actually allocate these.  */				    \
  16, 17, 18, 14, 15, 36, 37,	 /* r0, r1, r2, g14, g15, cc */             \
  COP_REG_ALLOC_ORDER \
}

#define ISR_REG_ALLOC_ORDER \
{ 20, 21, 22, 23, 24, 25, 26, 27,/* r4, r5, r6, r7, r8, r9, r10, r11   */ \
  28, 29, 30, 31, 19,            /* r12, r13, r14, r15, r3 */             \
   4, 5, 6, 7, 0, 1, 2, 3, 13,   /* g4, g5, g6, g7 g0, g1, g2, g3, g13 */ \
   8,9,10,11,12,                 /* g8, g9, g10, g11, g12, */             \
   32,33,34,35,                  /* fp0, fp1, fp2, fp3 */                 \
   /* can't actually allocate these */                                    \
   16, 17, 18, 14, 15, 36, 37,   /* r0, r1, r2, g14, g15, cc */           \
   COP_REG_ALLOC_ORDER \
}

#define ORDER_REGS_FOR_LOCAL_ALLOC i960_pick_reg_alloc_order()

/* Define the classes of registers for register constraints in the
   machine description.  Also define ranges of constants.

   One of the classes must always be named ALL_REGS and include all hard regs.
   If there is more than one class, another class must be named NO_REGS
   and contain no registers.

   The name GENERAL_REGS must be the name of a class (or an alias for
   another name such as ALL_REGS).  This is the class of registers
   that is allowed by "g" or "r" in a register constraint.
   Also, registers outside this class are allocated only when
   instructions express preferences for them.

   The classes must be numbered in nondecreasing order; that is,
   a larger-numbered class must never be contained completely
   in a smaller-numbered class.

   For any two classes, it is very desirable that there be another
   class that represents their union.  */
   
/* The 80960 has four kinds of registers, global, local, floating point,
   and condition code.  The cc register is never allocated, so no class
   needs to be defined for it.  */

enum reg_class {
 NO_REGS,
 COP_REG_CLASS_ENUM
 FP_REGS,
 LOCAL_OR_GLOBAL_QREGS,
 FP_OR_QREGS,
 LOCAL_OR_GLOBAL_DREGS,
 FP_OR_DREGS,
 GLOBAL_REGS,
 LOCAL_REGS,
 LOCAL_OR_GLOBAL_REGS,
 ALL_REGS,
 LIM_REG_CLASSES
};

#define N_REG_CLASSES (int) LIM_REG_CLASSES

/* Give names of register classes as strings for dump file.   */

#define REG_CLASS_NAMES  { \
	"NO_REGS", \
	COP_REG_CLASS_NAMES \
	"FP_REGS", \
	"LOCAL_OR_GLOBAL_QREGS", \
        "FP_OR_QREGS", \
	"LOCAL_OR_GLOBAL_DREGS", \
        "FP_OR_DREGS", \
	"GLOBAL_REGS", \
	"LOCAL_REGS", \
	"LOCAL_OR_GLOBAL_REGS", \
	"ALL_REGS" }

/* Define which registers fit in which classes.
   This is an initializer for a vector of HARD_REG_SET
   of length N_REG_CLASSES.  */

#define REG_CLASS_CONTENTS { \
	{0x00000000, 0x00000000}, \
	COP_REG_CONTENTS \
	{0x00000000, 0x0000000f}, \
	{0xfff0ffff, 0x00000000}, \
	{0xfff0ffff, 0x0000000f}, \
	{0xfffcffff, 0x00000000}, \
	{0xfffcffff, 0x0000000f}, \
	{0x0000ffff, 0x00000020}, \
	{0xffff0000, 0x00000000}, \
	{0xffffffff, 0x00000000}, \
	ALL_REG_CONTENTS }

/* The same information, inverted:
   Return the class number of the smallest class containing
   reg number REGNO.  This could be a conditional expression
   or could index an array.  */

#define REGNO_REG_CLASS(R) i960_reg_class(R)

/* The class value for index registers, and the one for base regs.  */
#define INDEX_REG_CLASS LOCAL_OR_GLOBAL_REGS
#define BASE_REG_CLASS LOCAL_OR_GLOBAL_REGS

/* Get reg_class from a letter such as appears in the machine description.  */
/* 'f' is a floating point register (fp0..fp3) */
/* 'l' is a local register (r0-r15) */
/* 'b' is a global register (g0-g15) */
/* 'd' is any local or global register */
/* 'r' or 'g' are pre-defined to the class ALL_REGS */

#define REG_CLASS_FROM_LETTER(C) i960_reg_class_from_c(C)

/* Given an rtx X being reloaded into a reg required to be
   in class CLASS, return the class of reg to actually use.
   In general this is just CLASS; but on some machines
   in some cases it is preferable to use a more restrictive class.  */

/* on 960, can't load constant into floating-point reg except 0.0 or 1.0 */

#define PREFERRED_RELOAD_CLASS(X,CLASS) ((i960_pref_reload_class((X),(CLASS)) ))

/* The letters I, J, K, L and M in a register constraint string
   can be used to stand for particular ranges of immediate operands.
   This macro defines what the ranges are.
   C is the letter, and VALUE is a constant value.
   Return 1 if VALUE is in the range specified by C.

   For 80960:
	'I' is used for literal values 0..31
   	'J' means literal 0 only if this is a function where g14
            is guaranteed to contain the literal 0.
	'K' means 0..-31.
        'L' means ~value == 0..31 */

#define CONST_OK_FOR_LETTER_P(VALUE, C)  				\
  ((C) == 'I' ? (((unsigned) (VALUE)) <= 31)				\
   : (C) == 'J' ? (((VALUE) == 0) && i960_g14_contains_zero())		\
      : (C) == 'K' ? ((VALUE) > -32 && (VALUE) <= 0)			\
        : (C) == 'L' ? (~((unsigned)(VALUE)) <= 31)			\
	: 0)

/* Similar, but for floating constants, and defining letters G and H.
   Here VALUE is the CONST_DOUBLE rtx itself.
   For the 80960, G is 0.0 and H is 1.0.  */

#define CONST_DOUBLE_OK_FOR_LETTER_P(VALUE, C) \
( ((C)=='G' && fp_literal_zero((VALUE),VOIDmode)) || \
  ((C)=='H' && fp_literal_one((VALUE),VOIDmode)) )

/* Return the class of register needed to copy IN to a register of
   class CLASS for input and output */

#define SECONDARY_INPUT_RELOAD_CLASS(CLASS,MODE,IN) \
  secondary_reload_class (CLASS, MODE, IN)

#define SECONDARY_OUTPUT_RELOAD_CLASS(CLASS,MODE,IN) \
  secondary_reload_class (CLASS, MODE, IN)

/* Return the maximum number of consecutive registers
   needed to represent mode MODE in a register of class CLASS.  */
/* On 80960, this is the size of MODE in words,
   except in the FP regs, where a single reg is always enough.  */
#define CLASS_MAX_NREGS(CLASS, MODE)					\
  ((CLASS) == FP_REGS ? 1 : HARD_REGNO_NREGS (0, (MODE)))

/* Stack layout; function entry, exit and calling.  */

/* Define this if pushing a word on the stack
   makes the stack pointer a smaller address.  */
/* #define STACK_GROWS_DOWNWARD */

/* Define this if the nominal address of the stack frame
   is at the high-address end of the local variables;
   that is, each additional local variable allocated
   goes at a more negative offset in the frame.  */
/* #define FRAME_GROWS_DOWNWARD */

/* Offset within stack frame to start allocating local variables at.
   If FRAME_GROWS_DOWNWARD, this is the offset to the END of the
   first local allocated.  Otherwise, it is the offset to the BEGINNING
   of the first local allocated.

   The i960 has a 64 byte register save area, plus possibly some extra
   bytes allocated for varargs functions.
   This changes to 128 if the function is a #pragma interrupt routine. */
#define STARTING_FRAME_OFFSET \
   ((i960_pragma_interrupt(current_function_decl) != PRAGMA_DO) ? 64 : \
    (!TARGET_NUMERICS ? 128 : 192))

/* If we generate an insn to push BYTES bytes,
   this says how many the stack pointer really advances by.
   On 80960, don't define this because there are no push insns.  */
/* #define PUSH_ROUNDING(BYTES) BYTES */

/* Offset of first parameter from the argument pointer register value.  */
#define FIRST_PARM_OFFSET(FNDECL) 0

/* When a parameter is passed in a register, no stack space is
   allocated for it.  However, when args are passed in the
   stack, space is allocated for every register parameter.  */
#define MAYBE_REG_PARM_STACK_SPACE 48
#define FINAL_REG_PARM_STACK_SPACE(CONST_SIZE, VAR_SIZE)	\
  i960_final_reg_parm_stack_space (CONST_SIZE, VAR_SIZE);
#define REG_PARM_STACK_SPACE(DECL) i960_reg_parm_stack_space (DECL)
#define OUTGOING_REG_PARM_STACK_SPACE

/* Keep the stack pointer constant throughout the function.  */
#define ACCUMULATE_OUTGOING_ARGS

/* Value is 1 if returning from a function call automatically
   pops the arguments described by the number-of-args field in the call.
   FUNTYPE is the data type of the function (as a tree),
   or for a library call it is an identifier node for the subroutine name.  */

#define RETURN_POPS_ARGS(FUNTYPE, SIZE) 0

/* Define how to find the value returned by a library function
   assuming the value has mode MODE.  */

#define LIBCALL_VALUE(MODE) \
  set_rtx_mode_type (gen_rtx ((REG), (MODE), \
                              (TARGET_NUMERICS && (MODE) == TFmode && \
                               TARGET_IC_COMPAT2_0) ? 32 : 0))

/* 1 if N is a possible register number for a function value
   as seen by the caller.
   On 80960, returns are in g0..g3 */
#define FUNCTION_VALUE_REGNO_P(N) ((N) < 4 || \
                                   ((N) == 32 && \
                                    TARGET_NUMERICS && TARGET_IC_COMPAT2_0))

/* 1 if N is a possible register number for function argument passing.
   On 80960, parameters are passed in g0..g11 */

#define FUNCTION_ARG_REGNO_P(N) ((N) < 12)

/* Perform any needed actions needed for a function that is receiving a
   variable number of arguments. 

   CUM is as above.

   MODE and TYPE are the mode and type of the current parameter.

   PRETEND_SIZE is a variable that should be set to the amount of stack
   that must be pushed by the prolog to pretend that our caller pushed
   it.

   Normally, this macro will push all remaining incoming registers on the
   stack and set PRETEND_SIZE to the length of the registers pushed.  */

#define SETUP_INCOMING_VARARGS(CUM,MODE,TYPE,PRETEND_SIZE,NO_RTL) \
  i960_setup_incoming_varargs(&CUM,MODE,TYPE,&PRETEND_SIZE,NO_RTL)

/* Define a data type for recording info about an argument list
   during the scan of that argument list.  This data type should
   hold all necessary information about the function itself
   and about the args processed so far, enough to enable macros
   such as FUNCTION_ARG to determine where the next arg should go.

   On 80960, this is two integers, which count the number of register
   parameters and the number of stack parameters seen so far.  */

struct cum_args { int ca_nregparms; int ca_nstackparms; };

#define CUMULATIVE_ARGS struct cum_args

/* Define the number of registers that can hold parameters.
   This macro is used only in macro definitions below and/or i960.c.  */
#define NPARM_REGS 12

/* Define how to round to the next parameter boundary.
   This macro is used only in macro definitions below and/or i960.c.  */
#define ROUND(X, MULTIPLE_OF)	\
  ((((X) + (MULTIPLE_OF) - 1) / (MULTIPLE_OF)) * MULTIPLE_OF)

/* Initialize a variable CUM of type CUMULATIVE_ARGS
   for a call to a function whose data type is FNTYPE.
   For a library call, FNTYPE is 0.

   On 80960, the offset always starts at 0; the first parm reg is g0.  */

#define INIT_CUMULATIVE_ARGS(CUM,FNTYPE,LIBNAME)	\
  ((CUM).ca_nregparms = 0, (CUM).ca_nstackparms = 0)

/* Update the data in CUM to advance over an argument
   of mode MODE and data type TYPE.
   CUM should be advanced to align with the data type accessed and
   also the size of that data type in # of regs.
   (TYPE is null for libcalls where that information may not be available.)  */

#define FUNCTION_ARG_ADVANCE(CUM, MODE, TYPE, NAMED)	\
  i960_function_arg_advance(&CUM, MODE, TYPE, NAMED)

#define FUNCTION_ARG_PASS_BY_REFERENCE(ARGS,MODE,TYPE,NAMED) \
(( (TYPE) && (TYPE_SIZE(TYPE)==0 || TREE_CODE(TYPE_SIZE(TYPE))!=INTEGER_CST) ))

/* Indicate the alignment boundary for an argument of the specified mode and
   type.  */
#define FUNCTION_ARG_BOUNDARY(MODE, TYPE)				\
  ((TYPE) && TYPE_ALIGN (TYPE) > PARM_BOUNDARY ? TYPE_ALIGN (TYPE)	\
   : PARM_BOUNDARY)

#define MUST_PASS_IN_STACK(MODE,TYPE) 0

/* Determine where to put an argument to a function.
   Value is zero to push the argument on the stack,
   or a hard register in which to store the argument.

   MODE is the argument's machine mode.
   TYPE is the data type of the argument (as a tree).
    This is null for libcalls where that information may
    not be available.
   CUM is a variable of type CUMULATIVE_ARGS which gives info about
    the preceding args and about the function being called.
   NAMED is nonzero if this argument is a named parameter
    (otherwise it is an extra parameter matching an ellipsis).  */

#define FUNCTION_ARG(CUM, MODE, TYPE, NAMED)	\
  i960_function_arg(&CUM, MODE, TYPE, NAMED)

/* Define how to find the value returned by a function.
   VALTYPE is the data type of the value (as a tree).
   If the precise function being called is known, FUNC is its FUNCTION_DECL;
   otherwise, FUNC is 0.  */

#define FUNCTION_VALUE(TYPE, FUNC) i960_function_value (TYPE)

/* Force objects larger than 16 bytes to be returned in memory, since we
   only have 4 registers available for return values.  */

#define RETURN_IN_MEMORY(TYPE) (int_size_in_bytes (TYPE) > 16)

/* For an arg passed partly in registers and partly in memory,
   this is the number of registers used.
   This never happens on 80960.  */

#define FUNCTION_ARG_PARTIAL_NREGS(CUM, MODE, TYPE, NAMED) 0

/* Output the label for a function definition.
  This handles leaf functions and a few other things for the i960.  */

#define ASM_DECLARE_FUNCTION_NAME(FILE, NAME, DECL)	\
  i960_function_name_declare (FILE, NAME, DECL)

/* This macro generates the assembly code for function entry.
   FILE is a stdio stream to output the code to.
   SIZE is an int: how many units of temporary storage to allocate.
   Refer to the array `regs_ever_live' to determine which registers
   to save; `regs_ever_live[I]' is nonzero if register number I
   is ever used in the function.  This macro is responsible for
   knowing which registers should not be saved even if used.  */

#define FUNCTION_PROLOGUE(FILE, SIZE) i960_function_prologue ((FILE), (SIZE))

/* Output assembler code to FILE to increment profiler label # LABELNO
   for profiling a function entry.  */

#define FUNCTION_PROFILER(FILE, LABELNO)  \
  fprintf (FILE, "\tlda	LP%d,g0\n\tbal	mcount\n", (LABELNO))

/* EXIT_IGNORE_STACK should be nonzero if, when returning from a function,
   the stack pointer does not matter.  The value is tested only in
   functions that have frame pointers.
   No definition is equivalent to always zero.  */

#define	EXIT_IGNORE_STACK 1

/* This macro generates the assembly code for function exit,
   on machines that need it.  If FUNCTION_EPILOGUE is not defined
   then individual return instructions are generated for each
   return statement.  Args are same as for FUNCTION_PROLOGUE.

   The function epilogue should not depend on the current stack pointer!
   It should use the frame pointer only.  This is mandatory because
   of alloca; we also take advantage of it to omit stack adjustments
   before returning.  */

#define FUNCTION_EPILOGUE(FILE, SIZE) i960_function_epilogue (FILE, SIZE)

/* Addressing modes, and classification of registers for them.  */

/* #define HAVE_POST_INCREMENT */
/* #define HAVE_POST_DECREMENT */

/* #define HAVE_PRE_DECREMENT */
/* #define HAVE_PRE_INCREMENT */

/* Macros to check register numbers against specific register classes.  */

/* These assume that REGNO is a hard or pseudo reg number.
   They give nonzero only if REGNO is a hard reg of the suitable class
   or a pseudo reg currently allocated to a suitable hard reg.
   Since they use reg_renumber, they are safe only once reg_renumber
   has been allocated, which happens in local-alloc.c.  */

#define REGNO_OK_FOR_INDEX_P(REGNO) \
  ((REGNO) < 32 || (unsigned) reg_renumber[REGNO] < 32)
#define REGNO_OK_FOR_BASE_P(REGNO) \
  ((REGNO) < 32 || (unsigned) reg_renumber[REGNO] < 32)
#define REGNO_OK_FOR_FP_P(REGNO) \
  ((REGNO) < 36 || (unsigned) reg_renumber[REGNO] < 36)

/* Now macros that check whether X is a register and also,
   strictly, whether it is in a specified class.

   These macros are specific to the 960, and may be used only
   in code for printing assembler insns and in conditions for
   define_optimization.  */

/* 1 if X is an fp register.  */

#define FP_REG_P(X) (REGNO (X) >= 32 && REGNO (X) < 36)

/* Maximum number of registers that can appear in a valid memory address.  */
#define	MAX_REGS_PER_ADDRESS 2

#define CONSTANT_ADDRESS_P(X) CONSTANT_P(X)

/* LEGITIMATE_CONSTANT_P is nonzero if the constant value X
   is a legitimate general operand.
   It is given that X satisfies CONSTANT_P.

   For the i960 we choose all constants to be valid.  Any that aren't
   actually valid get fixed up in reload.
*/

#define LEGITIMATE_CONSTANT_P(X) (1)

/* The macros REG_OK_FOR..._P assume that the arg is a REG rtx
   and check its validity for a certain class.
   We have two alternate definitions for each of them.
   The usual definition accepts all pseudo regs; the other rejects
   them unless they have been allocated suitable hard regs.
   The symbol REG_OK_STRICT causes the latter definition to be used.

   Most source files want to accept pseudo regs in the hope that
   they will get allocated to the class that the insn wants them to be in.
   Source files for reload pass need to be strict.
   After reload, it makes no difference, since pseudo regs have
   been eliminated by then.  */

#ifndef REG_OK_STRICT

/* Nonzero if X is a hard reg that can be used as an index
   or if it is a pseudo reg.  */
#define REG_OK_FOR_INDEX_P(X) \
  (REGNO (X) < 32 || REGNO (X) >= FIRST_PSEUDO_REGISTER)
/* Nonzero if X is a hard reg that can be used as a base reg
   or if it is a pseudo reg.  */
#define REG_OK_FOR_BASE_P(X) \
  (REGNO (X) < 32 || REGNO (X) >= FIRST_PSEUDO_REGISTER)

#define REG_OK_FOR_INDEX_P_STRICT(X) REGNO_OK_FOR_INDEX_P (REGNO (X))
#define REG_OK_FOR_BASE_P_STRICT(X) REGNO_OK_FOR_BASE_P (REGNO (X))

#else

/* Nonzero if X is a hard reg that can be used as an index.  */
#define REG_OK_FOR_INDEX_P(X) REGNO_OK_FOR_INDEX_P (REGNO (X))
/* Nonzero if X is a hard reg that can be used as a base reg.  */
#define REG_OK_FOR_BASE_P(X) REGNO_OK_FOR_BASE_P (REGNO (X))

#endif

/* GO_IF_LEGITIMATE_ADDRESS recognizes an RTL expression
   that is a valid memory address for an instruction.
   The MODE argument is the machine mode for the MEM expression
   that wants to use this address.

	On 80960, legitimate addresses are:
		base				ld	(g0),r0
		disp	(12 or 32 bit)		ld	foo,r0
		base + index			ld	(g0)[g1*1],r0
		base + displ			ld	0xf00(g0),r0
		base + index*scale + displ	ld	0xf00(g0)[g1*4],r0
		index*scale + base		ld	(g0)[g1*4],r0
		index*scale + displ		ld	0xf00[g1*4],r0
		index*scale			ld	[g1*4],r0
		index + base + displ		ld	0xf00(g0)[g1*1],r0

	In each case, scale can be 1, 2, 4, 8, or 16.  */

/* Returns 1 if the scale factor of an index term is valid. */
#define SCALE_TERM_P(X)							\
  (GET_CODE (X) == CONST_INT						\
   && (INTVAL (X) == 1 || INTVAL (X) == 2 || INTVAL (X) == 4 		\
       || INTVAL(X) == 8 || INTVAL (X) == 16))


#ifdef REG_OK_STRICT
#define GO_IF_LEGITIMATE_ADDRESS(MODE, X, ADDR) \
  { if (legitimate_address_p (MODE, X, 1)) goto ADDR; }
#else
#define GO_IF_LEGITIMATE_ADDRESS(MODE, X, ADDR) \
  { if (legitimate_address_p (MODE, X, 0)) goto ADDR; }
#endif

/* Try machine-dependent ways of modifying an illegitimate address
   to be legitimate.  If we find one, return the new, valid address.
   This macro is used in only one place: `memory_address' in explow.c.

   OLDX is the address as it was before break_out_memory_refs was called.
   In some cases it is useful to look at this to decide what needs to be done.

   MODE and WIN are passed so that this macro can use
   GO_IF_LEGITIMATE_ADDRESS.

   It is always safe for this macro to do nothing.  It exists to recognize
   opportunities to optimize the output.  */

/* On 80960, convert non-cannonical addresses to canonical form.  */

#define LEGITIMIZE_ADDRESS(X, OLDX, MODE, WIN)	\
{ rtx orig_x = (X);				\
  (X) = legitimize_address (X, OLDX, MODE);	\
  if ((X) != orig_x && memory_address_p (MODE, X)) \
    goto WIN; }

/* Go to LABEL if ADDR (a legitimate address expression)
   has an effect that depends on the machine mode it is used for.
   On the 960 this is never true.  */

#define GO_IF_MODE_DEPENDENT_ADDRESS(ADDR,LABEL)

/* Specify the machine mode that this machine uses
   for the index in the tablejump instruction.  */
#define CASE_VECTOR_MODE SImode

/* Define this if the tablejump instruction expects the table
   to contain offsets from the address of the table.
   Do not define this if the table should contain absolute addresses.  */
/* #define CASE_VECTOR_PC_RELATIVE */

/* Specify the tree operation to be used to convert reals to integers.  */
#define IMPLICIT_FIX_EXPR FIX_ROUND_EXPR

/* This is the kind of divide that is easiest to do in the general case.  */
#define EASY_DIV_EXPR TRUNC_DIV_EXPR

/* Define this as 1 if `char' should by default be signed; else as 0.  */
#define DEFAULT_SIGNED_CHAR 0

/* Allow and ignore #sccs directives.  */
#define	SCCS_DIRECTIVE

/* Max number of bytes we can move from memory to memory
   in one reasonably fast instruction.  */
#define MOVE_MAX 16

/* Define if operations between registers always perform the operation
   on the full register even if a narrower mode is specified.  */
#define WORD_REGISTER_OPERATIONS 1

/* Define if loading in MODE, an integral mode narrower than BITS_PER_WORD
   will either zero-extend or sign-extend.  The value of this macro should
   be the code that says which one of the two operations is implicitly
   done, NIL if none.  */
#define LOAD_EXTEND_OP(MODE) ZERO_EXTEND

/* Nonzero if access to memory by bytes is no faster than for words.
   Defining this results in worse code on the i960.  */

#define SLOW_BYTE_ACCESS 0

/* Make this a funny value to keep gcc from trying to do its fancy,
   and usually (for the i960) inappropriate optimizations for when
   STORE_FLAG_VALUE is 1, or -1. */

#define STORE_FLAG_VALUE 16

/* Define if shifts truncate the shift count
   which implies one can omit a sign-extension or zero-extension
   of a shift count.  */
/* #define SHIFT_COUNT_TRUNCATED */

/* Value is 1 if truncating an integer of INPREC bits to OUTPREC bits
   is done just by pretending it is already truncated.  */
#define TRULY_NOOP_TRUNCATION(OUTPREC, INPREC) 1

/* Specify the machine mode that pointers have.
   After generation of rtl, the compiler makes no further distinction
   between pointers and any other objects of this machine mode.  */
#define Pmode SImode

/* Specify the widest mode that BLKmode objects can be promoted to */
#define	MAX_FIXED_MODE_SIZE GET_MODE_BITSIZE (TImode)

/* Add any extra modes needed to represent the condition code.

   Also, signed and unsigned comparisons are distinguished, as
   are operations which are compatible with chkbit insns.  */
#define EXTRA_CC_MODES CC_UNSmode, CC_CHKmode

/* Define the names for the modes specified above.  */
#define EXTRA_CC_NAMES "CC_UNS", "CC_CHK"

/* Given a comparison code (EQ, NE, etc.) and the first operand of a COMPARE,
   return the mode to be used for the comparison.  For floating-point, CCFPmode
   should be used.  CC_NOOVmode should be used when the first operand is a
   PLUS, MINUS, or NEG.  CCmode should be used when no special processing is
   needed.  */
#define SELECT_CC_MODE(OP,X,Y) select_cc_mode (OP,X,Y)

/* A function address in a call instruction is a byte address
   (for indexing purposes) so give the MEM rtx a byte's mode.  */
#define FUNCTION_MODE SImode

/* Define this if addresses of constant functions
   shouldn't be put through pseudo regs where they can be cse'd.
   Desirable on machines where ordinary constants are expensive
   but a CALL with constant address is cheap.  */
#define NO_FUNCTION_CSE

/* Use memcpy, etc. instead of bcopy.  */

#ifndef WIND_RIVER
#define	TARGET_MEM_FUNCTIONS	1
#endif

/* Compute the cost of computing a constant rtl expression RTX
   whose rtx-code is CODE.  The body of this macro is a portion
   of a switch statement.  If the code is computed here,
   return it with a return statement.  Otherwise, break from the switch.  */

/* Constants that can be (non-ldconst) insn operands are cost 0.  Constants
   that can be non-ldconst operands in rare cases are cost 1.  Other constants
   have higher costs.  */

extern int const_fixup_happened;
extern int cnst_prop_has_run;

#define CONST_COSTS(RTX, CODE, OUTER_CODE)				\
  case CONST_INT:							\
    if (!cnst_prop_has_run)						\
      return 0;								\
    if (INTVAL (RTX) >= 0 && INTVAL (RTX) < 32)				\
      return 0;								\
    else if ((OUTER_CODE==IOR || OUTER_CODE==DIV) &&			\
            power2_operand (RTX, VOIDmode))				\
      return 0;								\
    else if (OUTER_CODE==AND &&						\
             (inv_power2_operand(RTX, VOIDmode) ||			\
              power2_operand (RTX, VOIDmode)))				\
      return 0;								\
    else if (OUTER_CODE == PLUS &&					\
             INTVAL (RTX) >= -31 && INTVAL (RTX) < 4096)		\
      return 0;								\
  case CONST:								\
  case LABEL_REF:							\
  case SYMBOL_REF:							\
    if (!cnst_prop_has_run)						\
      return 0;								\
    if (OUTER_CODE == PLUS)						\
      return 4;								\
    if (TARGET_C_SERIES || TARGET_J_SERIES || TARGET_H_SERIES)		\
      return 6;								\
    if (TARGET_K_SERIES)						\
      return 8;								\
  case CONST_DOUBLE:							\
    if (!cnst_prop_has_run)						\
      return 0;								\
    if ((RTX) == CONST0_RTX (DFmode) || (RTX) == CONST1_RTX (DFmode))	\
      return 1;								\
    if ((RTX) == CONST0_RTX (SFmode) || (RTX) == CONST1_RTX (SFmode))	\
      return 1;								\
    if ((RTX) == CONST0_RTX (TFmode) || (RTX) == CONST1_RTX (TFmode))	\
      return 1;								\
    return 12;

#define RTX_COSTS(X,CODE,OUTER_CODE)	\
  case MULT:								\
    if (TARGET_K_SERIES)						\
      total = COSTS_N_INSNS(6);						\
    if (TARGET_C_SERIES || TARGET_J_SERIES || TARGET_H_SERIES)		\
      total = COSTS_N_INSNS(3);						\
    if (TARGET_NOMUL)							\
      total = COSTS_N_INSNS(15);					\
    break;								\
									\
  case DIV:								\
    if (GET_CODE(XEXP(X,1)) == CONST_INT &&				\
        exact_log2(INTVAL(XEXP(X,1))) >= 0)				\
      total = COSTS_N_INSNS(2);						\
    else								\
    {									\
      if (TARGET_K_SERIES)						\
        total = COSTS_N_INSNS(37);					\
      if (TARGET_C_SERIES || TARGET_J_SERIES || TARGET_H_SERIES)	\
        total = COSTS_N_INSNS(39);					\
    }									\
    break;								\
									\
  case UDIV:								\
    if (GET_CODE(XEXP(X,1)) == CONST_INT &&				\
        exact_log2(INTVAL(XEXP(X,1))) >= 0)				\
      total = COSTS_N_INSNS(1);						\
    else								\
    {									\
      if (TARGET_K_SERIES)						\
        total = COSTS_N_INSNS(37);					\
      if (TARGET_C_SERIES || TARGET_J_SERIES || TARGET_H_SERIES)	\
        total = COSTS_N_INSNS(34);					\
    }									\
    break;								\
									\
  case MEM:								\
    return (COSTS_N_INSNS(i960_mem_cost(X)));

#define BRANCH_COST (1)

/* The i960 offers addressing modes which are "as cheap as a register".
   See i960.c (or gcc.texinfo) for details.  */

#define ADDRESS_COST(RTX) (i960_address_cost (RTX, 0))


/* Control the assembler format that we output.  */

/* Output at beginning of assembler file.  */

#define ASM_FILE_START(file)	 i960_asm_file_start(file)

/* Output to assembler file text saying following lines
   may contain character constants, extra white space, comments, etc.  */

#define ASM_APP_ON ""

/* Output to assembler file text saying following lines
   no longer contain unusual constructs.  */

#define ASM_APP_OFF ""

/* Output before read-only data.  */

extern char *i960_text_section_asm_op();
#define TEXT_SECTION_ASM_OP i960_text_section_asm_op() 

#define EXTRA_SECTIONS	in_named_text, in_ctors, in_dtors

extern char *i960_init_section_asm_op();
extern char *i960_ctors_section_asm_op();
extern char *i960_dtors_section_asm_op();
#define INIT_SECTION_ASM_OP i960_init_section_asm_op()
#define CTORS_SECTION_ASM_OP i960_ctors_section_asm_op()
#define DTORS_SECTION_ASM_OP i960_dtors_section_asm_op()

#define EXTRA_SECTION_FUNCTIONS					\
								\
void								\
ctors_section() 						\
{								\
  if (in_section != in_ctors)					\
    {								\
      fprintf (asm_out_file, "%s\n", CTORS_SECTION_ASM_OP);	\
      in_section = in_ctors;					\
    }								\
}								\
								\
void								\
dtors_section() 						\
{								\
  if (in_section != in_dtors)					\
    {								\
      fprintf (asm_out_file, "%s\n", DTORS_SECTION_ASM_OP);	\
      in_section = in_dtors;					\
    }								\
}								\

#define ASM_OUTPUT_CONSTRUCTOR(FILE,NAME)	\
  do { ctors_section();				\
       fprintf(FILE, "\t.word\t_%s\n", NAME); } while (0)

#define ASM_OUTPUT_DESTRUCTOR(FILE,NAME)	\
  do { dtors_section();				\
       fprintf(FILE, "\t.word\t_%s\n", NAME); } while (0)

/* Output before writable data.  */

extern char *i960_data_section_asm_op();
#define DATA_SECTION_ASM_OP i960_data_section_asm_op()

#define ENCODE_SECTION_INFO(DECL) i960_encode_section_info(DECL)

/* How to refer to registers in assembler output.
   This sequence is indexed by compiler's hard-register-number (see above).  */

#define REGISTER_NAMES {						\
	"g0", "g1", "g2",  "g3",  "g4",  "g5",  "g6",  "g7",		\
	"g8", "g9", "g10", "g11", "g12", "g13", "g14", "fp",		\
	"pfp","sp", "rip", "r3",  "r4",  "r5",  "r6",  "r7",		\
	"r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",		\
	"fp0","fp1","fp2", "fp3", "cc", "fake",				\
	COP_REG_NAMES							\
}

/* How to renumber registers for dbx and gdb.
   In the 960 encoding, g0..g15 are registers 16..31.  */

#define DBX_REGISTER_NUMBER(REGNO)					\
  (((REGNO) < 16) ? (REGNO) + 16					\
   : (((REGNO) > 31) ? (REGNO) : (REGNO) - 16))

/* Don't emit dbx records longer than this.  This is an arbitrary value.  */
#define DBX_CONTIN_LENGTH 400

/* This is how to output a note to DBX telling it the line number
   to which the following sequence of instructions corresponds. */

#define ASM_OUTPUT_SOURCE_LINE(FILE, LINE)                      \
  { if (write_symbols == SDB_DEBUG) {                           \
        fprintf ((FILE), "\t.ln   %d\n",                        \
                (sdb_begin_function_line > -1) ?                \
                        (LINE) - sdb_begin_function_line : 1);  \
  } else if (write_symbols == DBX_DEBUG) {                      \
        fprintf((FILE),"\t.stabd  68,0,%d\n", (LINE));          \
  } else if (write_symbols == NO_DEBUG) {                       \
        fprintf((FILE), "\t\t# %d\n", (LINE));                  \
  } }

/* This is how to output the definition of a user-level label named NAME,
   such as the label on a static function or variable NAME.  */

#define ASM_OUTPUT_LABEL(FILE,NAME)	\
  do { assemble_name (FILE, NAME); fputs (":\n", FILE); } while (0)

/* This is how to output a command to make the user-level label named NAME
   defined for reference from other files.  */

#define ASM_GLOBALIZE_LABEL(FILE,NAME)		\
{ fputs ("\t.globl\t", FILE);			\
  assemble_name (FILE, NAME);			\
  fputs ("\n", FILE); }

/* This is how to output a reference to a user-level label named NAME.
   `assemble_name' uses this.  */

#define ASM_OUTPUT_LABELREF(FILE,NAME)	fprintf (FILE, "_%s", NAME)

/* This is how to output an internal numbered label where
   PREFIX is the class of label and NUM is the number within the class.  */

#define ASM_OUTPUT_INTERNAL_LABEL(FILE,PREFIX,NUM)	\
  (fprintf (FILE, "%s%d:\n", PREFIX, NUM))

/* This is how to store into the string LABEL
   the symbol_ref name of an internal numbered label where
   PREFIX is the class of label and NUM is the number within the class.
   This is suitable for output with `assemble_name'.  */

#define ASM_GENERATE_INTERNAL_LABEL(LABEL,PREFIX,NUM)	\
  sprintf (LABEL, "*%s%d", PREFIX, NUM)

/* This is how to output an assembler line defining a `long double' constant. */

#define ASM_OUTPUT_LONG_DOUBLE(FILE,VALUE)  i960_output_long_double(FILE,VALUE)

/* This is how to output an assembler line defining a `double' constant.  */

#define ASM_OUTPUT_DOUBLE(FILE,VALUE)  i960_output_double(FILE, VALUE)

/* This is how to output an assembler line defining a `float' constant.  */

#define ASM_OUTPUT_FLOAT(FILE,VALUE)  i960_output_float(FILE, VALUE)

/* This is how to output an assembler line defining an `int' constant.  */

#define ASM_OUTPUT_INT(FILE,VALUE)  \
( fprintf (FILE, "\t.word\t"),			\
  output_addr_const (FILE, (VALUE)),		\
  fprintf (FILE, "\n"))

/* Likewise for `char' and `short' constants.  */

#define ASM_OUTPUT_SHORT(FILE,VALUE)  \
( fprintf (FILE, "\t.short\t"),			\
  output_addr_const (FILE, (VALUE)),		\
  fprintf (FILE, "\n"))

#define ASM_OUTPUT_CHAR(FILE,VALUE)  \
( fprintf (FILE, "\t.byte\t"),			\
  output_addr_const (FILE, (VALUE)),		\
  fprintf (FILE, "\n"))

/* This is how to output an assembler line for a numeric constant byte.  */

#define ASM_OUTPUT_BYTE(FILE,VALUE)	\
  fprintf (FILE, "\t.byte\t0x%x\n", (VALUE))

#define ASM_OUTPUT_REG_PUSH(FILE,REGNO)  \
  fprintf (FILE, "\tst\t%s,(sp)\n\taddo\t4,sp,sp\n", reg_names[REGNO])

/* This is how to output an insn to pop a register from the stack.
   It need not be very fast code.  */

#define ASM_OUTPUT_REG_POP(FILE,REGNO)  \
  fprintf (FILE, "\tsubo\t4,sp,sp\n\tld\t(sp),%s\n", reg_names[REGNO])

/* This is how to output an element of a case-vector that is absolute.  */

#define ASM_OUTPUT_ADDR_VEC_ELT(FILE, VALUE)  \
  fprintf (FILE, "\t.word\tL%d\n", VALUE)

/* This is how to output an element of a case-vector that is relative.  */

#define ASM_OUTPUT_ADDR_DIFF_ELT(FILE, VALUE, REL)  \
  fprintf (FILE, "\t.word\tL%d-L%d\n", VALUE, REL)

/* This is how to output an assembler line that says to advance the
   location counter to a multiple of 2**LOG bytes.  */

#define ASM_OUTPUT_ALIGN(FILE,LOG)	\
  fprintf (FILE, "\t.align\t%d\n", (LOG))

#define ASM_OUTPUT_SKIP(FILE,SIZE)  \
  fprintf (FILE, "\t.space\t%d\n", (SIZE))

/* This says how to output an assembler line
   to define a global common symbol.  */

#define OBJECT_BYTES_BITALIGN(S) i960_object_bytes_bitalign(S)

/*
 * Declare an object name that will be initialized.  If this is
 * in SRAM then bind it specially, otherwise just ouput the label.
 */
#define ASM_DECLARE_OBJECT_NAME(FILE, NAME, DECL)	\
do {							\
  char *namexx = (NAME);					\
  int sram_addrxx = glob_sram_addr(namexx);		\
  if (sram_addrxx != 0)				\
  {							\
    int bytesxx = int_size_in_bytes (TREE_TYPE(DECL));	\
    fprintf (FILE, "\t.set\t_%s,%u\n", namexx, sram_addrxx);		\
    fprintf (FILE, "\t.globl\t___sram.%u\n", sram_addrxx);		\
    fprintf (FILE, "\t.globl\t___sram_length.%u\n", sram_addrxx);	\
    fprintf (FILE, "\t.set\t___sram_length.%u,%u\n",	\
             sram_addrxx, bytesxx);			\
    fprintf (FILE, "___sram.%u:\n", sram_addrxx);	\
    if (flag_elf) {					\
      fprintf (FILE, "\t.elf_type\t___sram.%u,object\n", sram_addrxx);	\
      fprintf (FILE, "\t.elf_size\t___sram.%u,%u\n", sram_addrxx, bytesxx); \
    }							\
  }							\
  else							\
  {							\
    ASM_OUTPUT_LABEL(FILE, namexx);			\
    if (flag_elf) {					\
      int  sizexx;					\
      fprintf (FILE, "\t.elf_type\t");			\
      assemble_name(FILE, namexx);			\
      fprintf (FILE, ",object\n");			\
      if (DECL_RTL(DECL) && GET_CODE(DECL_RTL(DECL)) == MEM	\
	  && GET_CODE(XEXP(DECL_RTL(DECL),0)) == SYMBOL_REF	\
	  && (sizexx = SYMREF_SIZE(XEXP((DECL_RTL(DECL)),0))) > 0)	\
      {							\
        fprintf (FILE, "\t.elf_size\t");		\
        assemble_name(FILE, namexx);			\
        fprintf (FILE, ",%d\n", sizexx);		\
      }							\
    }							\
  }							\
} while (0)

#define ASM_OUTPUT_COMMON(FILE,NAME,SIZE,ROUNDED)	\
  i960_asm_output_common(FILE,NAME,SIZE)

#define GLOBALIZE_STATICS (( (base_file_name[0]!='$'||base_file_name[1]!='\0') ))
#define ASM_OUTPUT_LOCAL(FILE,NAME,SIZE,ALIGN)	\
  i960_asm_output_local(FILE,NAME,SIZE)

/* Output text for an #ident directive.  */
#define	ASM_OUTPUT_IDENT(FILE, STR)  fprintf(FILE, "\t# %s\n", STR);

/* Align code to 8 byte boundary if TARGET_CODE_ALIGN is true.  */

#define	ASM_OUTPUT_ALIGN_CODE(FILE)		\
{ if (TARGET_CODE_ALIGN) fputs("\t.align\t3\n",FILE); }

/* Store in OUTPUT a string (made with alloca) containing
   an assembler-name for a local static variable named NAME.
   LABELNO is an integer which is different for each call.  */

#define ASM_FORMAT_PRIVATE_NAME(OUTPUT,NAME,LABELNO) \
do { \
  char* __n__ = (NAME); \
  char* __b__ = base_file_name; \
  char* __o__ = (char*) alloca(strlen(__n__)+strlen(__b__)+16); \
  int   __i__ = (LABELNO); \
  if ((GLOBALIZE_STATICS) || (LABELNO)) \
    sprintf(__o__, "%s.%s.%d", __n__, __b__, __i__); \
  else \
    sprintf(__o__, "%s", __n__); \
  (OUTPUT) = __o__; \
} while (0)

/* Define the parentheses used to group arithmetic operations
   in assembler code.  */

#define ASM_OPEN_PAREN "("
#define ASM_CLOSE_PAREN ")"

/* Define results of standard character escape sequences.  */
#define TARGET_BELL	007
#define TARGET_BS	010
#define TARGET_TAB	011
#define TARGET_NEWLINE	012
#define TARGET_VT	013
#define TARGET_FF	014
#define TARGET_CR	015

/* Output assembler code to FILE to initialize this source file's
   basic block profiling info, if that has not already been done.  */

#define FUNCTION_BLOCK_PROFILER(FILE, LABELNO) \
{ fprintf (FILE, "\tld	LPBX0,g12\n");			\
  fprintf (FILE, "\tcmpobne	0,g12,LPY%d\n",LABELNO);\
  fprintf (FILE, "\tlda	LPBX0,g12\n");			\
  fprintf (FILE, "\tcall	___bb_init_func\n");	\
  fprintf (FILE, "LPY%d:\n",LABELNO); }

/* Output assembler code to FILE to increment the entry-count for
   the BLOCKNO'th basic block in this source file.  */

#define BLOCK_PROFILER(FILE, BLOCKNO) \
{ int blockn = (BLOCKNO);				\
  fprintf (FILE, "\tld	LPBX2+%d,g12\n", 4 * blockn);	\
  fprintf (FILE, "\taddo	g12,1,g12\n");		\
  fprintf (FILE, "\tst	g12,LPBX2+%d\n", 4 * blockn); }

/* Print operand X (an rtx) in assembler syntax to file FILE.
   CODE is a letter or dot (`z' in `%z0') or 0 if no letter was specified.
   For `%' followed by punctuation, CODE is the punctuation and X is null.  */

#define PRINT_OPERAND(FILE, X, CODE)  \
  i960_print_operand (FILE, X, CODE);

/* Print a memory address as an operand to reference that memory location.  */

#define PRINT_OPERAND_ADDRESS(FILE, ADDR)	\
  i960_print_operand_addr (FILE, ADDR)

/* Output assembler code for a block containing the constant parts
   of a trampoline, leaving space for the variable parts.  */

/* On the i960, the trampoline contains three instructions:
     ldconst _function, r4
     ldconst static addr, r3
     jump (r4)  */

#define TRAMPOLINE_TEMPLATE(FILE)					\
{									\
  ASM_OUTPUT_INT (FILE, gen_rtx (CONST_INT, VOIDmode, 0x8C203000));	\
  ASM_OUTPUT_INT (FILE, gen_rtx (CONST_INT, VOIDmode, 0x00000000));	\
  ASM_OUTPUT_INT (FILE, gen_rtx (CONST_INT, VOIDmode, 0x8C183000));	\
  ASM_OUTPUT_INT (FILE, gen_rtx (CONST_INT, VOIDmode, 0x00000000));	\
  ASM_OUTPUT_INT (FILE, gen_rtx (CONST_INT, VOIDmode, 0x84212000));	\
}

/* Length in units of the trampoline for entering a nested function.  */

#define TRAMPOLINE_SIZE 20

/* Emit RTL insns to initialize the variable parts of a trampoline.
   FNADDR is an RTX for the address of the function's pure code.
   CXT is an RTX for the static chain value for the function.  */

#define INITIALIZE_TRAMPOLINE(TRAMP, FNADDR, CXT)			\
{									\
  emit_move_insn (gen_rtx (MEM, SImode, plus_constant (TRAMP, 4)),	\
		  FNADDR);						\
  emit_move_insn (gen_rtx (MEM, SImode, plus_constant (TRAMP, 12)),	\
		  CXT);							\
}

/* Promote char and short arguments to ints, when want compitibility with
   the iC960 compilers.  */

#define PROMOTE_PROTOTYPES	((TARGET_CLEAN_LINKAGE))
#define PROMOTE_RETURN		((TARGET_CLEAN_LINKAGE))

/* Instruction type definitions.  Used to alternate instructions types for
   better performance on the C series chips.  */

enum insn_types { I_TYPE_REG, I_TYPE_MEM, I_TYPE_CTRL };

/* Parse opcodes, and set the insn last insn type based on them.  */

#define ASM_OUTPUT_OPCODE(FILE, INSN)	i960_scan_opcode (INSN)

/* Table listing what rtl codes each predicate in i960.c will accept.  */

#define PREDICATE_CODES \
  {"move_operand", {CONST_INT, CONST_DOUBLE, CONST, SYMBOL_REF,		\
                          LABEL_REF, SUBREG, REG, MEM}},		\
  {"fpmove_src_operand", {CONST_INT, CONST_DOUBLE, CONST, SYMBOL_REF,	\
			  LABEL_REF, SUBREG, REG, MEM}},		\
  {"arith_operand", {SUBREG, REG, CONST_INT}},				\
  {"fp_arith_operand", {SUBREG, REG, CONST_DOUBLE}},			\
  {"signed_arith_operand", {SUBREG, REG, CONST_INT}},			\
  {"literal", {CONST_INT}},						\
  {"fp_literal_one", {CONST_DOUBLE}},					\
  {"fp_literal_double", {CONST_DOUBLE}},				\
  {"fp_literal", {CONST_DOUBLE}},					\
  {"signed_literal", {CONST_INT}},					\
  {"symbolic_memory_operand", {SUBREG, MEM}},				\
  {"eq_or_neq", {EQ, NE}},						\
  {"arith32_operand", {SUBREG, REG, LABEL_REF, SYMBOL_REF, CONST_INT,	\
		       CONST_DOUBLE, CONST}},				\
  {"power2_operand", {CONST_INT}},					\
  {"inv_power2_operand", {CONST_INT}},


#define IS_LEAF_PROC(T) (i960_is_leaf_proc(T))

#define HARD_REG_VAR_ADJ(RGNO, RGMASK) \
  do { \
    if ((RGNO) < 32) \
    { \
      switch ((RGNO) & 3) \
      { \
        case 3: \
          (RGNO) -= 1; \
          (RGMASK) <<= SI_BYTES; \
        case 2: \
          (RGNO) -= 1; \
          (RGMASK) <<= SI_BYTES; \
        case 1:  \
          (RGNO) -= 1; \
          (RGMASK) <<= SI_BYTES; \
        case 0: \
          break; \
      } \
      if ((RGMASK) > 0xFFFF) \
        abort(); \
    } \
  } while (0)

/* Note that the official interface to HANDLE_PRAGMA
   (a stream) does not work; because of the way the code
   surrounding the call to the macro is written, if we don't
   examine the current value of 'c' inside the macro, then if
   the input is only #pragma\n, we cannot do the right thing;
   the macro will look at the following line, but only
   the current line will be eaten after the call. */
#define HANDLE_PRAGMA(C,GETFUNC,UNGETFUNC,FILE) \
if (C == '\n')  \
  goto skipline; \
else \
  i960_process_pragma(GETFUNC,UNGETFUNC,FILE)

/* Include intel defs that are applicable to most (960, LION,
   sparc?) processors */

#include "i_extensions.h"

/* Put intel defs that are target dependent here. */

/*  All optimizations and state changes that need to go
    just before final should go in here if possible */
#define PRE_FINAL_OPT(FNDECL, INSNS) i960_pre_final_opt(FNDECL, INSNS)

#define INTEL_960

#define SETUP_INLINE_PSEUDO_FRAME(PFR, FRAME, INSN) \
  i960_setup_pseudo_frame(PFR, FRAME, INSN)

#define CLEANUPUP_INLINE_PSEUDO_FRAME(PFR, FRAME, INSN) \
  i960_cleanup_pseudo_frame(PFR, FRAME, INSN)

#define SETUP_HARD_PARM_REG(FN) \
  i960_setup_hard_parm_reg(FN)

#define OPTIMIZATION_OPTIONS(OPTIMIZE)  \
  switch (OPTIMIZE) {\
  default:\
\
  case 4:\
    flag_split_mem          = 1;\
    flag_marry_mem          = 1;\
    flag_coalesce           = 1;\
\
  case 3:\
    flag_shadow_mem         = (OPTIMIZE)-2;\
    flag_unroll_loops       = 1;\
    flag_constprop          = 1;\
    flag_inline_functions   = 1;\
    flag_coerce             = 1;\
\
  case 2:\
    if (!flag_shadow_mem)\
      flag_shadow_globals   = 1;\
    flag_copyprop           = 1;\
    flag_constreg           = 1;\
    flag_constcomp          = 1;\
    flag_omit_frame_pointer = 1;\
    flag_caller_saves       = 1;\
    flag_cond_xform         = 1;\
    target_flags |= (TARGET_FLAG_TAILCALL);\
    target_flags |= (TARGET_FLAG_LEAFPROC);\
\
  case 1:\
  case 0: ;\
  }

/* Define the set of single-letter and multi-letter gcc960 options
 * that take arguments and allow white space between the option and
 * the argument, and of those the options that take non-file
 * or non-directory name arguments.  This latter class of options
 * is needed for DOS in function get_response_file().
 */

#define SLOS_WITH_ARG		"DeILoUuTZ"

#define MLOS_WITH_ARG		{ \
				"bsize", \
				"clist", \
				"include", \
				"imacros", \
				"Tdata", \
				"Ttext", \
				"Tbss", \
				"defsym", \
				"idb", \
				"iprof", \
				"bname", \
				"input_tmp", \
				"bname_tmp", \
				NULL }

extern int i960_word_switch_takes_arg();

#define WORD_SWITCH_TAKES_ARG(STR) i960_word_switch_takes_arg((STR))

#define SWITCH_TAKES_ARG(CHAR) (strchr(SLOS_WITH_ARG, (CHAR)) != NULL)

#define	GNU_SPEC_SUFFIX	".gld"

/* tell cc1 where to find standard GNU defines - look in $G960INC first,
   then in $G960BASE/include, then in $INCDIR (ic960's include home), then
   in $I960BASE/include (ic960's default) */

#define	CPP_SPEC "%{@T'cpp'} %{!nostdinc:-nostdinc -I%[I:%[~/%[@I]:%[?I]]]}\
	-D__i960\
	%{AKA:-D__i960KA__ -D__i960_KA__ -D__i960KA}\
	%{AKB:-D__i960KB__ -D__i960_KB__ -D__i960KB}\
	%{ASA:-D__i960SA__ -D__i960_SA__ -D__i960SA}\
	%{ASB:-D__i960SB__ -D__i960_SB__ -D__i960SB}\
	%{ACA:-D__i960CA__ -D__i960_CA__ -D__i960CA}\
	%{ACF:-D__i960CF__ -D__i960_CF__ -D__i960CF}\
	%{AJA:-D__i960JA__ -D__i960_JA__ -D__i960JA}\
	%{AJD:-D__i960JD__ -D__i960_JD__ -D__i960JD}\
	%{AJF:-D__i960JF__ -D__i960_JF__ -D__i960JF}\
	%{AJL:-D__i960JL__ -D__i960_JL__ -D__i960JL}\
	%{ARP:-D__i960RP__ -D__i960_RP__ -D__i960RP}\
	%{AHA:-D__i960HA__ -D__i960_HA__ -D__i960HA}\
	%{AHD:-D__i960HD__ -D__i960_HD__ -D__i960HD}\
	%{AHT:-D__i960HT__ -D__i960_HT__ -D__i960HT}\
	%{ACA_DMA:-D__i960CA__ -D__i960_CA__ -D__i960CA}\
	%{ACF_DMA:-D__i960CF__ -D__i960_CF__ -D__i960CF}\
	%{!A*:-D__i960_KB__ -D__i960KB__ -D__i960KB}\
	%{G:-D__i960_BIG_ENDIAN__}\
	%{Felf:%{g:-g} %{g0:-g0} %{g1:-g1} %{g2:-g2} %{g3:-g3}}\
	%{mpic:-D__PIC}\
	%{mpid:-D__PID}"

#define	CC1_SPEC \
"%{@T'cc1'}\
 -fno-builtin\
 %{AKA:-mka -msoft-float}\
 %{ASA:-mka -msoft-float}\
 %{AKB:-mkb}\
 %{ASB:-mkb}\
 %{ACA:-mca -msoft-float -mstrict-align}\
 %{ACF:-mcf -msoft-float -mstrict-align}\
 %{ACA_DMA:-mca -msoft-float -mstrict-align}\
 %{ACF_DMA:-mcf -msoft-float -mstrict-align}\
 %{AJA:-mja -msoft-float -mstrict-align}\
 %{AJD:-mjd -msoft-float -mstrict-align}\
 %{AJF:-mjf -msoft-float -mstrict-align}\
 %{AJL:-mjl -msoft-float -mstrict-align}\
 %{ARP:-mrp -msoft-float -mstrict-align}\
 %{AHA:-mha -msoft-float}\
 %{AHD:-mhd -msoft-float}\
 %{AHT:-mht -msoft-float}\
 %{!A*:-mkb}\
 %{g:-g} %{g0:-g0} %{g1:-g1} %{g2:-g2} %{g3:-g3}\
 %{pid}\
 %{Fbout:%{Fcoff:%fspecify at most one -F option}}\
 %{Fbout:%{!Fcoff:%{Felf:%fspecify at most one -F option}}}\
 %{!Fbout:%{Fcoff:%{Felf:%fspecify at most one -F option}}}\
 %{Fbout:-Fbout} %{Fcoff:-Fcoff} %{Felf:-Felf}\
 %{Fbout:%{G:%fBig-endian code generation requires -Fcoff or -Felf}}\
 %{!F*:%{G:%fBig-endian code generation requires -Fcoff or -Felf}}\
 %{G:-G}"


#define	CC1PLUS_SPEC \
"%{@T'cc1plus'}\
 -fno-builtin\
 %{AKA:-mka -msoft-float}\
 %{ASA:-mka -msoft-float}\
 %{AKB:-mkb}\
 %{ASB:-mkb}\
 %{ACA:-mca -msoft-float -mstrict-align}\
 %{ACF:-mcf -msoft-float -mstrict-align}\
 %{ACA_DMA:-mca -msoft-float -mstrict-align}\
 %{ACF_DMA:-mcf -msoft-float -mstrict-align}\
 %{AJA:-mja -msoft-float -mstrict-align}\
 %{AJD:-mjd -msoft-float -mstrict-align}\
 %{AJF:-mjf -msoft-float -mstrict-align}\
 %{AJL:-mjl -msoft-float -mstrict-align}\
 %{ARP:-mrp -msoft-float -mstrict-align}\
 %{AHA:-mha -msoft-float -mstrict-align}\
 %{AHD:-mhd -msoft-float -mstrict-align}\
 %{AHT:-mht -msoft-float -mstrict-align}\
 %{!A*:-mkb}\
 %{g:-g} %{g0:-g0} %{g1:-g1} %{g2:-g2} %{g3:-g3}\
 %{pid}\
 %{!F*:%fspecify -Fcoff or -Felf for C++}\
 %{Fcoff:%{Felf:%fspecify at most one -F option}}\
 %{Fbout:%fspecify -Fcoff or -Felf for C++}\
 %{Fcoff:-Fcoff} %{Felf:-Felf}\
 %{Fbout:%{G:%fBig-endian code generation requires -Fcoff or -Felf}}\
 %{!F*:%{G:%fBig-endian code generation requires -Fcoff or -Felf}}\
 %{G:-G}"

#define	ENV_GNUBASE	"G960BASE"
#define	ENV_GNUBIN	"G960BIN"
#define	ENV_GNULIB	"G960LIB"
#define	ENV_GNUINC	"G960INC"
#define ENV_GNUTMP      "G960TMP"
#define ENV_GNUPDB	"G960PDB"

#define	ENV_GNUAS	"G960AS"
#define	ENV_GNUCPP	"G960CPP"
#define	ENV_GNUCC1	"G960CC1"
#define	ENV_GNUCC1PLUS	"G960CC1PLUS"
#define	ENV_GNULD	"G960LD"
#define	ENV_GNUCRT	"G960CRT"
#define	ENV_GNUCRT_G	"G960CRT_G"
#define	ENV_GNUCRT_M	"G960CRT_M"
#define	ENV_GNUCRT_B	"G960CRT_B"

#ifdef DOS
#define	GNUAS_DFLT	"%{!Fcoff:%{!Felf:gas960.exe}%{Felf:gas960e.exe}}%{Fcoff:gas960c.exe}"
#define GNULD_DFLT	"gld960.exe"
#define	GNUCC1_DFLT	"cc1.exe"
#define	GNUCC1PLUS_DFLT	"cc1plus.exe"
#define	GNUCPP_DFLT	"cpp.exe"
#else
#define	GNUAS_DFLT	"%{!Fcoff:%{!Felf:gas960}%{Felf:gas960e}}%{Fcoff:gas960c}"
#define GNULD_DFLT	"gld960"
#define	GNUCC1_DFLT	"cc1.960"
#define	GNUCC1PLUS_DFLT	"cc1plus.960"
#define	GNUCPP_DFLT	"cpp.960"
#endif

#define	ASM_SPEC "%{@T'as'} %{A*} %{!A*:-AKB}\
 %{Fbout:%{Fcoff:%fspecify at most one -F option}}\
 %{Fbout:%{!Fcoff:%{Felf:%fspecify at most one -F option}}}\
 %{!Fbout:%{Fcoff:%{Felf:%fspecify at most one -F option}}}\
 %{Fbout:%{G:%fBig-endian code generation requires -Fcoff or -Felf}}\
 %{!F*:%{G:%fBig-endian code generation requires -Fcoff or -Felf}}\
 %{G:-G}\
 %{z} %{j*}"

#define LIB_SPEC \
 "%{!nostdlib:-lqf -lc -lm %{@T'lib'}\
  %{AKA:-lh}\
  %{ASA:-lh}\
  %{AC*:-lh}\
  %{AJ*:-lh}\
  %{AH*:-lh}}"

#define	LINK_SPEC "%{defsym*} %{@T'ld'}\
		%{ACA_DMA:-ACA} %{ACF_DMA:-ACF}\
		%{!ACA_DMA:%{!ACF_DMA:%{A*} %{!A*:-AKB}}}\
		%{G:-G} %{F*}\
		%{Z*} %{e*} %{r} %{s} %{Ttext} %{Tdata} %{Tbss} %{u*}\
		%{X} %{x} %{y*} %{z} %{mpic:-pc} %{mpid:-pd}"

#define	ARCH_SPEC \
"%{A*:\
 %{!AKA:%{!AKB:\
 %{!ASA:%{!ASB:\
 %{!ACA:%{!ACF:%{!ACA_DMA:%{!ACF_DMA:\
 %{!AJA:%{!AJD:%{!AJF:%{!AJL:%{!ARP:\
 %{!AHA:%{!AHD:%{!AHT:\
 %finvalid -A architecture switch}}}}}}}}}}}}}}}}}"

#define STARTFILE_SPEC  \
    "%{!crt:%{@T'crt':%[~]/lib/%{mpid:%{G:crt960_e.o}%{!G:crt960_p.o}}%{!mpid:%{G:crt960_b.o}%{!G:crt960.o}}}}"

#define	SDB_STRUCT_TAG_MUNGING_P	(1)
#define SDB_ALLOW_FORWARD_REFERENCES	1
#define SDB_ALLOW_UNKNOWN_REFERENCES	1

/* Redefine this to print in hex like iC960.  */
#define PUT_SDB_TYPE(A) fprintf (asm_out_file, "\t.type\t0x%x;", A)

#define PUT_SDB_SIZE(SZ) \
  do { \
    if (SZ > 65535) \
    { \
      warning ("user data structure size exceeds 65535"); \
      warning ("    truncated to fit COFF debug information limitation."); \
    } \
    fprintf(asm_out_file, "\t.size\t%d%s", SZ & 0xFFFF, SDB_DELIM); \
  } while (0)

#define PUT_SDB_NEXT_DIM(N) \
  do { \
    if (N > 65535) \
    { \
      warning ("array dimension exceeds 65535\n"); \
      warning ("    truncated to fit COFF debug information limitation."); \
    } \
    fprintf(asm_out_file, "%d,", N & 0xFFFF); \
  } while (0)

#define PUT_SDB_LAST_DIM(N) \
  do { \
    if (N > 65535) \
    { \
      warning ("array dimension exceeds 65535\n"); \
      warning ("    truncated to fit COFF debug information limitation."); \
    } \
    fprintf(asm_out_file, "%d%s", N & 0xFFFF, SDB_DELIM); \
  } while (0)

/* 960 architecture with floating-point */
#define	TARGET_FLAG_NUMERICS	0x01
#define	TARGET_NUMERICS		((target_flags & TARGET_FLAG_NUMERICS)!=0)

/* Nonzero if we should generate code for the KA processor */
/* 	no FPU, no microcode instructions */
#define	TARGET_FLAG_K_SERIES	0x02
#define TARGET_K_SERIES		((target_flags & TARGET_FLAG_K_SERIES)!=0)

/* Nonzero if we should generate code for the CA processor */
/*	different optimization strategies */
#define	TARGET_FLAG_C_SERIES	0x04
#define	TARGET_C_SERIES		((target_flags & TARGET_FLAG_C_SERIES)!=0)

/* Nonzero if we should generate code for the CA processor */
/*	different optimization strategies */
#define	TARGET_FLAG_J_SERIES	0x08
#define	TARGET_J_SERIES		((target_flags & TARGET_FLAG_J_SERIES)!=0)

/* Nonzero if we should generate code for the CA processor */
/*	different optimization strategies */
#define	TARGET_FLAG_H_SERIES	0x10
#define	TARGET_H_SERIES		((target_flags & TARGET_FLAG_H_SERIES)!=0)

/* Nonzero if we should generate leaf-procedures when we find them */
/*	You may not want to do this because leaf-proc entries are */
/*	slower when not entered via BAL - this would be true when */
/*	a linker not supporting the optimization is used	*/
#define	TARGET_FLAG_LEAFPROC	0x20
#define	TARGET_LEAFPROC 	((target_flags & TARGET_FLAG_LEAFPROC)!=0)

/* Nonzero if we should performa tail-call optimizations when we find them */
/*	You may not want to do this because the detection of cases where */
/*	this is not legal is not totally complete		*/
#define	TARGET_FLAG_TAILCALL	0x40
#define	TARGET_TAILCALL		((target_flags & TARGET_FLAG_TAILCALL)!=0)

/* This is a two bit flag that takes certain values. depending on how     */
/* much complex addressing is allowable/desirable.                        */
/*	Complex addressing modes are probably not worthwhile on the K-series */
/*	But they definitely are on the C-series */
#define TARGET_FLAG_REG_OR_DIRECT_ONLY	0x000
#define TARGET_FLAG_REG_OFF_ADDR	0x80
#define TARGET_FLAG_INDEX_ADDR		0x100
#define TARGET_FLAG_ALL_ADDR		0x180
#define TARGET_FLAG_ADDR_MASK		0x180

#define TARGET_ADDR_LEGAL	(target_flags & TARGET_FLAG_ADDR_MASK)

#define	TARGET_FLAG_CODE_ALIGN		0x200
#define	TARGET_CODE_ALIGN	((target_flags  & TARGET_FLAG_CODE_ALIGN)!=0)

#define	TARGET_FLAG_BRANCH_PREDICT	0x400
#define	TARGET_BRANCH_PREDICT	((target_flags  & TARGET_FLAG_BRANCH_PREDICT)!=0)

#define	TARGET_FLAG_CLEAN_LINKAGE	0x800
#define	TARGET_CLEAN_LINKAGE	((target_flags & TARGET_FLAG_CLEAN_LINKAGE)!=0)

#define	TARGET_FLAG_IC_COMPAT3_0	0x1000 
#define	TARGET_IC_COMPAT3_0	((target_flags & TARGET_FLAG_IC_COMPAT3_0)!= 0)

#define	TARGET_FLAG_IC_COMPAT2_0	0x2000
#define	TARGET_IC_COMPAT2_0	((target_flags & TARGET_FLAG_IC_COMPAT2_0)!=0)

#define	TARGET_FLAG_STRICT_ALIGN	0x4000
#define	TARGET_STRICT_ALIGN	((target_flags & TARGET_FLAG_STRICT_ALIGN)!=0)

#define TARGET_FLAG_USE_CALLX		0x8000
#define TARGET_USE_CALLX	((target_flags & TARGET_FLAG_USE_CALLX)!=0)

#define	TARGET_FLAG_ASM_COMPAT		0x10000
#define	TARGET_ASM_COMPAT	((target_flags & TARGET_FLAG_ASM_COMPAT)!=0)

#define TARGET_FLAG_PIC			0x20000
#define TARGET_PIC		((target_flags & TARGET_FLAG_PIC)!= 0)

#define TARGET_FLAG_PID			0x40000
#define TARGET_PID		((target_flags & TARGET_FLAG_PID)!= 0)

#define TARGET_FLAG_PID_SAFE		0x80000
#define TARGET_PID_SAFE		((target_flags & TARGET_FLAG_PID_SAFE)!= 0)

/*
 * The next flag indicates that compare&branch instructions should be used
 * whenever possible.
 */
#define TARGET_FLAG_CMPBRANCH		0x100000
#define TARGET_FLAG_NOCMPBRANCH		0x200000
#define TARGET_FLAG_CMPBR_MASK		0x300000

#define TARGET_CMPBRANCH	\
	((target_flags & TARGET_FLAG_CMPBR_MASK) == TARGET_FLAG_CMPBRANCH)
#define TARGET_NOCMPBRANCH	\
	((target_flags & TARGET_FLAG_CMPBR_MASK) == TARGET_FLAG_NOCMPBRANCH)

#define TARGET_FLAG_STRICT_REF_DEF	0x400000
#define TARGET_STRICT_REF_DEF	((target_flags & TARGET_FLAG_STRICT_REF_DEF)!=0)

#define TARGET_FLAG_DOUBLE4		0x800000
#define TARGET_DOUBLE4		((target_flags & TARGET_FLAG_DOUBLE4) != 0)

#define TARGET_FLAG_LONGDOUBLE4		0x1000000
#define TARGET_LONGDOUBLE4	((target_flags & TARGET_FLAG_LONGDOUBLE4) != 0)

#define TARGET_FLAG_DCACHE		0x2000000
#define TARGET_DCACHE		((target_flags & TARGET_FLAG_DCACHE) != 0)

#define TARGET_FLAG_ABI			0x4000000
#define TARGET_ABI		((target_flags & TARGET_FLAG_ABI) != 0)

#define TARGET_FLAG_CAVE		0x8000000
#define TARGET_CAVE		((target_flags & TARGET_FLAG_CAVE) != 0)

#define TARGET_FLAG_NOMUL		0x10000000
#define TARGET_FLAG_CPMUL		0x20000000
#define TARGET_CPMUL		((target_flags & TARGET_FLAG_CPMUL) != 0)
#define TARGET_NOMUL		(((target_flags & TARGET_FLAG_NOMUL) != 0) && \
				 ((target_flags & TARGET_FLAG_CPMUL) == 0))

#define TARGET_SWITCHES  \
  { {"sa", (TARGET_FLAG_K_SERIES | TARGET_FLAG_ALL_ADDR)},\
    {"sb", (TARGET_FLAG_K_SERIES | TARGET_FLAG_NUMERICS |\
		TARGET_FLAG_ALL_ADDR)},\
    {"ka", (TARGET_FLAG_K_SERIES | TARGET_FLAG_ALL_ADDR)},\
    {"kb", (TARGET_FLAG_K_SERIES | TARGET_FLAG_NUMERICS |\
		TARGET_FLAG_ALL_ADDR)},\
    {"ca", (TARGET_FLAG_C_SERIES | TARGET_FLAG_BRANCH_PREDICT |\
		TARGET_FLAG_CODE_ALIGN | TARGET_FLAG_REG_OFF_ADDR)},\
    {"cf", (TARGET_FLAG_C_SERIES | TARGET_FLAG_BRANCH_PREDICT |\
		TARGET_FLAG_CODE_ALIGN | TARGET_FLAG_REG_OFF_ADDR |\
		TARGET_FLAG_DCACHE)},\
    {"ja", (TARGET_FLAG_J_SERIES | TARGET_FLAG_INDEX_ADDR |\
		TARGET_FLAG_DCACHE)},\
    {"jd", (TARGET_FLAG_J_SERIES | TARGET_FLAG_INDEX_ADDR |\
		TARGET_FLAG_DCACHE)},\
    {"jf", (TARGET_FLAG_J_SERIES | TARGET_FLAG_INDEX_ADDR |\
		TARGET_FLAG_DCACHE)},\
    {"jl", (TARGET_FLAG_J_SERIES | TARGET_FLAG_INDEX_ADDR |\
		TARGET_FLAG_NOMUL)},\
    {"rp", (TARGET_FLAG_J_SERIES | TARGET_FLAG_INDEX_ADDR |\
		TARGET_FLAG_DCACHE)},\
    {"ha", (TARGET_FLAG_H_SERIES | TARGET_FLAG_BRANCH_PREDICT |\
		TARGET_FLAG_CODE_ALIGN | TARGET_FLAG_REG_OFF_ADDR |\
		TARGET_FLAG_DCACHE)},\
    {"hd", (TARGET_FLAG_H_SERIES | TARGET_FLAG_BRANCH_PREDICT |\
		TARGET_FLAG_CODE_ALIGN | TARGET_FLAG_REG_OFF_ADDR |\
		TARGET_FLAG_DCACHE)},\
    {"ht", (TARGET_FLAG_H_SERIES | TARGET_FLAG_BRANCH_PREDICT |\
		TARGET_FLAG_CODE_ALIGN | TARGET_FLAG_REG_OFF_ADDR |\
		TARGET_FLAG_DCACHE)},\
    {"numerics", (TARGET_FLAG_NUMERICS)},		\
    {"soft-float", -(TARGET_FLAG_NUMERICS)},		\
    {"leaf-procedures", TARGET_FLAG_LEAFPROC},		\
    {"noleaf-procedures",-(TARGET_FLAG_LEAFPROC)},	\
    {"no-leaf-procedures",-(TARGET_FLAG_LEAFPROC)},	\
    {"tail-call",TARGET_FLAG_TAILCALL},			\
    {"notail-call",-(TARGET_FLAG_TAILCALL)},		\
    {"no-tail-call",-(TARGET_FLAG_TAILCALL)},		\
    {"complex-addr",TARGET_FLAG_ALL_ADDR},		\
    {"nocomplex-addr",-(TARGET_FLAG_ALL_ADDR)},		\
    {"no-complex-addr",-(TARGET_FLAG_ALL_ADDR)},	\
    {"code-align",TARGET_FLAG_CODE_ALIGN},		\
    {"nocode-align",-(TARGET_FLAG_CODE_ALIGN)},		\
    {"no-code-align",-(TARGET_FLAG_CODE_ALIGN)},	\
    {"clean-linkage", (TARGET_FLAG_CLEAN_LINKAGE)},	\
    {"noclean-linkage", -(TARGET_FLAG_CLEAN_LINKAGE)},	\
    {"no-clean-linkage", -(TARGET_FLAG_CLEAN_LINKAGE)},	\
    {"ic-compat", TARGET_FLAG_IC_COMPAT2_0},		\
    {"ic2.0-compat", TARGET_FLAG_IC_COMPAT2_0},		\
    {"ic3.0-compat", TARGET_FLAG_IC_COMPAT3_0},		\
    {"asm-compat",TARGET_FLAG_ASM_COMPAT},		\
    {"intel-asm",TARGET_FLAG_ASM_COMPAT},		\
    {"strict-align", TARGET_FLAG_STRICT_ALIGN},		\
    {"nostrict-align", -(TARGET_FLAG_STRICT_ALIGN)},	\
    {"no-strict-align", -(TARGET_FLAG_STRICT_ALIGN)},	\
    {"long-calls", TARGET_FLAG_USE_CALLX},		\
    {"cave", TARGET_FLAG_CAVE},                         \
    {"pic", TARGET_FLAG_PIC},				\
    {"pid", TARGET_FLAG_PID},				\
    {"pid-safe", TARGET_FLAG_PID_SAFE},			\
    {"cmpbr", TARGET_FLAG_CMPBRANCH},			\
    {"nocmpbr", TARGET_FLAG_NOCMPBRANCH},		\
    {"no-cmpbr", TARGET_FLAG_NOCMPBRANCH},		\
    {"strict-ref-def", TARGET_FLAG_STRICT_REF_DEF},	\
    {"double4", TARGET_FLAG_DOUBLE4},			\
    {"long-double4", TARGET_FLAG_LONGDOUBLE4},		\
    {"dcache", TARGET_FLAG_DCACHE},			\
    {"no-dcache", -(TARGET_FLAG_DCACHE)},		\
    {"nodcache", -(TARGET_FLAG_DCACHE)},		\
    {"abi", TARGET_FLAG_ABI},				\
    {"mul", -(TARGET_FLAG_NOMUL)},			\
    {"no-mul", TARGET_FLAG_NOMUL},			\
    {"cpmul", TARGET_FLAG_CPMUL},			\
    { "", TARGET_DEFAULT}}

/* -mwait=N gives the number of wait states to schedule for */
extern char * wait_states_string;

/* -mi960_align=n gives the initial alignment used for
 * the #pragma i960_align value.
 */
extern char * i960_align_option;

#define TARGET_OPTIONS \
  { {"wait=", &wait_states_string },			\
    {"i960_align=", &i960_align_option }}

/* Generate DBX debugging information.  */
#define DBX_DEBUGGING_INFO

/* Generate SDB style debugging information.  */
#define SDB_DEBUGGING_INFO

/* Generate DWARF style debugging information. */
#define DWARF_DEBUGGING_INFO

#if !defined(DWARF_VERSION)
/* Allow overriding DWARF_VERSION from cc command line. */
#define DWARF_VERSION 2
#endif

#define PREFERRED_DEBUGGING_TYPE  ((flag_coff) ? SDB_DEBUG :\
                                   (flag_elf) ?  DWARF_DEBUG : DBX_DEBUG)

#define DEFAULT_GDB_EXTENSIONS 0

#if DWARF_VERSION >= 2

#define DWARF_PRODUCER			"Intel 80960 GNU Compiler"
#define DWARF_CIE_AUGMENTER		DW_CFA_i960_ABI_augmentation
#define DWARF_CIE_MAX_REG_COLUMNS	36
#define DWARF_MIN_INSTRUCTION_BYTE_LENGTH 4
#define ASM_COMMENT_START		"#"

/* For each physical register, define the DWARF version 2 location expression
   opcodes and operands used to identify the register as a location or as a
   base register.  For each register, four values are given.
   The first value is the DWARF opcode that names the register as the
   location of some expression, and the second value is the operand
   to be used with that opcode.
   The third value is the DWARF opcode that names the register as containing
   the memory address of some expression, and the fourth value is the operand
   to be used with that opcode.
   The operands are only used with the DW_OP_regx and DW_OP_bregx opcodes.

   The fifth operand is the .debug_frame CIE column number that
   specifies the rules for restoring this register.  The value -1
   indicates the register is not represented in .debug_frame.
 */
#define DWARF_REGISTER_INFO	{ \
  { DW_OP_reg16,   0,  DW_OP_breg16,   0,  DW_CFA_i960_g0 },	/* g0  */ \
  { DW_OP_reg17,   0,  DW_OP_breg17,   0,  DW_CFA_i960_g1 },	/* g1  */ \
  { DW_OP_reg18,   0,  DW_OP_breg18,   0,  DW_CFA_i960_g2 },	/* g2  */ \
  { DW_OP_reg19,   0,  DW_OP_breg19,   0,  DW_CFA_i960_g3 },	/* g3  */ \
  { DW_OP_reg20,   0,  DW_OP_breg20,   0,  DW_CFA_i960_g4 },	/* g4  */ \
  { DW_OP_reg21,   0,  DW_OP_breg21,   0,  DW_CFA_i960_g5 },	/* g5  */ \
  { DW_OP_reg22,   0,  DW_OP_breg22,   0,  DW_CFA_i960_g6 },	/* g6  */ \
  { DW_OP_reg23,   0,  DW_OP_breg23,   0,  DW_CFA_i960_g7 },	/* g7  */ \
  { DW_OP_reg24,   0,  DW_OP_breg24,   0,  DW_CFA_i960_g8 },	/* g8  */ \
  { DW_OP_reg25,   0,  DW_OP_breg25,   0,  DW_CFA_i960_g9 },	/* g9  */ \
  { DW_OP_reg26,   0,  DW_OP_breg26,   0,  DW_CFA_i960_g10 },	/* g10 */ \
  { DW_OP_reg27,   0,  DW_OP_breg27,   0,  DW_CFA_i960_g11 },	/* g11 */ \
  { DW_OP_reg28,   0,  DW_OP_breg28,   0,  DW_CFA_i960_g12 },	/* g12 */ \
  { DW_OP_reg29,   0,  DW_OP_breg29,   0,  DW_CFA_i960_g13 },	/* g13 */ \
  { DW_OP_reg30,   0,  DW_OP_breg30,   0,  DW_CFA_i960_g14 },	/* g14 */ \
  { DW_OP_reg31,   0,  DW_OP_breg31,   0,  DW_CFA_i960_fp },	/* g15 */ \
  { DW_OP_reg0,    0,  DW_OP_breg0,    0,  DW_CFA_i960_pfp },	/* r0  */ \
  { DW_OP_reg1,    0,  DW_OP_breg1,    0,  DW_CFA_i960_sp },	/* r1  */ \
  { DW_OP_reg2,    0,  DW_OP_breg2,    0,  DW_CFA_i960_rip },	/* r2  */ \
  { DW_OP_reg3,    0,  DW_OP_breg3,    0,  DW_CFA_i960_r3 },	/* r3  */ \
  { DW_OP_reg4,    0,  DW_OP_breg4,    0,  DW_CFA_i960_r4 },	/* r4  */ \
  { DW_OP_reg5,    0,  DW_OP_breg5,    0,  DW_CFA_i960_r5 },	/* r5  */ \
  { DW_OP_reg6,    0,  DW_OP_breg6,    0,  DW_CFA_i960_r6 },	/* r6  */ \
  { DW_OP_reg7,    0,  DW_OP_breg7,    0,  DW_CFA_i960_r7 },	/* r7  */ \
  { DW_OP_reg8,    0,  DW_OP_breg8,    0,  DW_CFA_i960_r8 },	/* r8  */ \
  { DW_OP_reg9,    0,  DW_OP_breg9,    0,  DW_CFA_i960_r9 },	/* r9  */ \
  { DW_OP_reg10,   0,  DW_OP_breg10,   0,  DW_CFA_i960_r10 },	/* r10 */ \
  { DW_OP_reg11,   0,  DW_OP_breg11,   0,  DW_CFA_i960_r11 },	/* r11 */ \
  { DW_OP_reg12,   0,  DW_OP_breg12,   0,  DW_CFA_i960_r12 },	/* r12 */ \
  { DW_OP_reg13,   0,  DW_OP_breg13,   0,  DW_CFA_i960_r13 },	/* r13 */ \
  { DW_OP_reg14,   0,  DW_OP_breg14,   0,  DW_CFA_i960_r14 },	/* r14 */ \
  { DW_OP_reg15,   0,  DW_OP_breg15,   0,  DW_CFA_i960_r15 },	/* r15 */ \
  { DW_OP_regx,    0,  DW_OP_bregx,    0,  DW_CFA_i960_fp0 },	/* fp0 */ \
  { DW_OP_regx,    1,  DW_OP_bregx,    1,  DW_CFA_i960_fp1 },	/* fp1 */ \
  { DW_OP_regx,    2,  DW_OP_bregx,    2,  DW_CFA_i960_fp2 },	/* fp2 */ \
  { DW_OP_regx,    3,  DW_OP_bregx,    3,  DW_CFA_i960_fp3 },	/* fp3 */ \
  { DW_OP_regx,   20,  DW_OP_bregx,   20,  -1 },	/* CC, Not used */ \
  { DW_OP_regx,   21,  DW_OP_bregx,   21,  -1 }	/* Not Used */ \
}

#endif	/* DWARF_VERSION >= 2 */

/* Define some macros used to extract a listing line number and
   source column number from IMSTG-specific fields found in NOTE insns
   and _DECL nodes.

   COL_BITS denotes the number of bits we give (out of 32) to column position.
   MAX_LIST_COL is based on COL_BITS.  All column positions >= MAX_LIST_COL
   are represented as MAX_LIST_COL.
 */

#define COL_BITS 8
#define MAX_LIST_COL (((1 << COL_BITS)-1))

#define GET_LINE(P)  (( ((unsigned)(P)) >> COL_BITS ))
#define GET_COL(P)   (( (P) & MAX_LIST_COL ))

#define SET_COL(P,I) do {\
  (P) = ((P) & ~MAX_LIST_COL) | MIN((I),MAX_LIST_COL); \
} while (0)

#if COL_BITS <= 8
#define IMSTG_COL_TYPE unsigned char
#else
#if COL_BITS <= 16
#define IMSTG_COL_TYPE unsigned short
#else
#define IMSTG_COL_TYPE unsigned long
#endif
#endif

/* See i_rtl.h for definition of note instances.  Need to put this
   declaration here to avoid including rtl.h in several places.
 */
extern int imstg_note_instance_counter;

#define INTEGRATE_THRESHOLD(DECL) i960_integrate_threshold(DECL)

#define GLOBAL_INTEGRATE_THRESHOLD(DECL) i960_global_integrate_threshold(DECL)

#define STRICT_ALIGNMENT TARGET_STRICT_ALIGN

/* Specify alignment for string literals (which might be higher than the
   base type's minimnal alignment requirement.  This allows strings to be
   aligned on word boundaries, and optimizes calls to the str* and mem*
   library functions.  */

#define CONSTANT_ALIGNMENT(EXP, ALIGN) \
  (i960_object_bytes_bitalign (int_size_in_bytes (TREE_TYPE (EXP))) > (ALIGN) \
   ? i960_object_bytes_bitalign (int_size_in_bytes (TREE_TYPE (EXP)))	    \
   : (ALIGN))

/* This is alignment for objects.  If object is an array whose size will
   be determined by an initializer, int_size_in_bytes(T) may be -1, and
   we have to assume that the object will turn up big after we do
   the initializer.  Hence we need to set up the alignment as the maximum
   possible. */

#define DATA_ALIGNMENT(T, ALIGN) \
  (int_size_in_bytes(T)<=0 ? 128 : \
    (i960_object_bytes_bitalign (int_size_in_bytes (T)) > (ALIGN) \
     ? i960_object_bytes_bitalign (int_size_in_bytes (T))	    \
     : (ALIGN)))

/* Macros to determine size of aggregates (structures and unions
   in C).  Normally, these may be defined to simply return the maximum
   alignment and simple rounded-up size, but on some machines (like
   the i960), the total size of a structure is based on a non-trivial
   rounding method.  */

#define ROUND_TYPE_ALIGN(TYPE, COMPUTED, SPECIFIED) \
i960_round_type_align ((TYPE), MAX((COMPUTED),(SPECIFIED)))

#define ROUND_TYPE_SIZE(TYPE, SIZE, ALIGN) \
(tree)i960_round_type_size(TYPE, SIZE, ALIGN)

#define USE_PSEUDO_ARGPTR(FUNC) (( 1 ))

/* SIZE and ALIGN are known byte size and alignment.  This
   macro picks the highest alignment for X that it can,
   by examination of X.  If -mstrict-align is disabled, we may
   guess (based on SIZE and ALIGN) */

#define MEM_ALIGNMENT(X,SIZE,ALIGN) (( i960_mem_alignment((X),(SIZE),(ALIGN)) ))
#define REG_ALIGNMENT(X,SIZE,ALIGN) (( i960_reg_alignment((X),(SIZE),(ALIGN)) ))
#define RTX_ALIGNMENT(X,SIZE,ALIGN) (( i960_rtx_alignment((X),(SIZE),(ALIGN)) ))

#define STRUCT_SET_PRAGMA_ALIGN(T)  (i960_set_pragma_align(T))
#define STRUCT_DONE_PRAGMA_ALIGN(T) (i960_done_pragma_align(T))
#define MEMBER_SET_PRAGMA_PACK(T)   (i960_set_pragma_pack(T))

#define DEFAULT_SIGNED_BITFIELDS 0
#define REG_PARM_FAKE_STACK_SPACE 1
#define FIND_UNUSED_REGISTER(MODE,COULD_CROSS_CALL) \
  i960_find_unused_register((MODE),(COULD_CROSS_CALL))

#define SMALL_AGGR_RETURN_IN_REGS 1
#define DEFAULT_PCC_STRUCT_RETURN 0

#define SIZE_TYPE "unsigned int"
#define PTRDIFF_TYPE "int"

#define WCHAR_TYPE "char"
#define WCHAR_TYPE_SIZE 8

/*
 * Define this so that we consistently use gdb stabs for the compiler we build,
 * not some host systems idea of what stabs should be.
 */
#define NO_STAB_H 1

/* Return rtl expression for size of named arguments */
#define ARGSIZE_RTX (rtx) i960_argsize_rtx()

/*
 * extern declarations for all global data and functions referred to by
 * the i960.h file.
 */
#include "machmode.h"

extern int i960_need_compare;
extern enum machine_mode i960_cmp_mode;

/* These global variables are used to pass information between
   cc setter and cc user at insn emit time.  */
extern struct rtx_def *i960_compare_op0, *i960_compare_op1;

/* Holds the insn type of the last insn output to the assembly file.  */
extern enum insn_types i960_last_insn_type;

/* Holds target cpu and code generation flags. */
extern int target_flags;

extern unsigned int hard_regno_mode_ok[FIRST_PSEUDO_REGISTER];

extern enum reg_class i960_reg_class();
extern enum reg_class i960_reg_class_from_c();
extern enum reg_class secondary_reload_class();
extern enum reg_class i960_pref_reload_class();
extern struct rtx_def *i960_function_arg ();
extern struct rtx_def *i960_function_value ();
extern struct rtx_def *legitimize_address ();
extern enum machine_mode select_cc_mode();

/* Define the function that build the compare insn for scc and bcc.  */
extern struct rtx_def *gen_compare_reg ();

/* Define functions in i960.c and used in insn-output.c.  */
extern char *i960_output_ldconst ();
extern char *i960_output_call_insn ();
extern char *i960_output_ret_insn ();
extern char *i960_output_compare ();
extern char *i960_output_cmp_branch();
extern char *i960_output_test ();
extern char *i960_output_pred_insn();
extern char *i960_best_lda_sequence ();
extern char *i960_out_movl();
extern char *i960_out_movt();
extern char *i960_out_movq();
extern char *i960_out_ldl();
extern char *i960_out_ldt();
extern char *i960_out_ldq();
extern char *i960_out_stl();
extern char *i960_out_stt();
extern char *i960_out_stq();
extern int i960_shr_comp_cond_change();
extern int i960_change_cc_code();
extern int i960_change_cc_mode();

extern int i960_func_passes_on_stack;
#define NOTE_PASS_ON_STACK i960_func_passes_on_stack++

#define OUTPUT_SDB_ARG_PTR_DEF() i960_output_arg_ptr_def()

#define OUTPUT_DBX_ARG_PTR_DEF() i960_output_arg_ptr_def()

#define PID_BASE_REGNUM 12  /* g12 */

#define PID_CHG_DECL(decl) \
  do { \
    if ((pid_flag || TARGET_PID) && \
        TREE_CODE(decl) == VAR_DECL && !TREE_READONLY(decl)) \
      SYMREF_ETC(XEXP(DECL_RTL(decl),0)) |= SYMREF_PIDBIT; \
  } while (0);

/*
 * We make this definition because some compilers don't properly honor
 * casts to float.
 */
extern double dbl_to_flt();

#ifdef IMSTG_REAL
#define REAL_IS_NOT_DOUBLE
#define REAL_ARITHMETIC
/* Since we are using the emulator, we can have our choice of host
   float endianess - making it always be LE makes dumps consistent
   across hosts. */
#define HOST_FLOAT_WORDS_BIG_ENDIAN 0
#define FLOAT_WORDS_BIG_ENDIAN 0
#else
#define REAL_VALUE_TRUNCATE(mode, x) \
 (GET_MODE_BITSIZE (mode) == sizeof (float) * HOST_BITS_PER_CHAR        \
  ? dbl_to_flt(x) : (x))

extern double dnegate();
#define REAL_VALUE_NEGATE(X) (( dnegate(X) ))
#endif

#define PRAGMA_UNSET 0
#define PRAGMA_NO    1
#define PRAGMA_DO    2

#define PRAGMA_PURE(DECL)	(i960_pragma_pure(DECL) == PRAGMA_DO)

#define PRAGMA_INLINE(DECL)	(i960_pragma_inline(DECL) == PRAGMA_DO)

#define CHANGE_OPTIONS_FOR_FUNC(DECL)	i960_change_flags_for_function(DECL)

#define RESTORE_OPTIONS_AFTER_FUNC(DECL)  i960_restore_flags_after_function(DECL)

/* Set to true when we parse a "#pragma section" in the source.  It stays
   true from then on.  This flag tells DWARF debug output to suppress
   certain constructs that might fail to assemble due to pragma section.
 */
#define IMSTG_PRAGMA_SECTION_SEEN	i960_pragma_section_seen
extern int i960_pragma_section_seen;

#define MAX_CND_XFRM_INSNS	(TARGET_J_SERIES ? 2 :\
				 TARGET_H_SERIES ? 2 : 0)

#define LIST_POS get_list_pos()
#define LIST_PREV_POS get_list_prev_pos()

#define CHECK_PIC_PID(X)	i960_check_pic_pid(X)

#define ADJUST_LEGAL_ADDR(X)	i960_adjust_legal_addr(X)

extern char *i960_strip_name_encoding();
#define STRIP_NAME_ENCODING(T,S) ((T) = i960_strip_name_encoding ((T), (S)))

/* For now, until ifdefs are removed */
#define TMC_TS 1

/* This variables indicates how many registers flow allocated space for. */
extern int max_regno;
