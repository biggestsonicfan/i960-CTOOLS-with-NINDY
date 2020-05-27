
#if !defined(__I_DWARF2_H__)
#define __I_DWARF2_H__ 1

#if defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2

#include "dwarf2.h"	/* From tool-wide include directory. */
#include "dw2i960.h"	/* From tool-wide include directory. */

/* For each physical register r, dwarf_reg_info defines the
   DWARF location expression opcodes and operands to be used
   to represent r as a register or base register.
   It also maps each physical register to its CIE register/column number.
 */

typedef struct {
	unsigned int	reg_opcode;	  /* DW_OP_reg{0-31} or DW_OP_regx   */
	unsigned int	reg_operand;      /* meaningful only for DW_OP_regx  */
	unsigned int	base_reg_opcode;  /* DW_OP_breg{0-31} or DW_OP_bregx */
	unsigned int	base_reg_operand; /* meaningful only for DW_OP_bregx */
	int		cie_column;
} Dwarf_reg_info;

extern Dwarf_reg_info dwarf_reg_info[FIRST_PSEUDO_REGISTER];

/*
 * -----------------------------------------------------------
 * Define data structures to support the .debug_frame section.
 * -----------------------------------------------------------
 */

/* Since CIE's are shared by multiple functions, keep track of the ones
   already in use.  Reuse them when possible.
   The CIEs are placed in a list by dwarfout_cie().  They are emitted to
   the assembly file in dwarfout_finish().
 */

typedef struct dwarf_cie {
	struct dwarf_cie	*next_cie;
		/* Maintains a linked list of all emitted CIEs (for reuse) */
	int			label_num;
		/* Number used in the label at the CIE's definition. */

	char	*augmentation;
		/* Never NULL, use "\0" to indicate no augmentation. */
	unsigned long	code_alignment_factor;	/* always >= 1 */
	unsigned long	data_alignment_factor;	/* always >= 1 */

	int		return_address_register;
		/* Rule column number to be used to find the return address. */

	int		cfa_rule;
	unsigned long	cfa_register;
	unsigned long	cfa_offset;
		/* The initial rule for the CFA will either be DW_CFA_undefined,
		   or DW_CFA_def_cfa_register(reg_column_num, offset).
		   Record the reg_column_num and offset here.  The rules
		   for the remaining registers are specified below.
		 */

	unsigned long	rules[DWARF_CIE_MAX_REG_COLUMNS][2];
		/* This array specifies the initial register rules for each
		   register (except for the CFA register).  For a given
		   register number RN, rules[RN][0] is an opcode and
		   rules[RN][1] is the operand needed by that opcode (if any).
		   The possible initial register rules are as follows:

			rules[RN][0] == DW_CFA_undefined, rules[RN][1] == N/A
			rules[RN][0] == DW_CFA_same_value, rules[RN][1] == N/A

			rules[RN][0] == DW_CFA_offset, rules[RN][1] == offset
				It's hard to see how this rule could be an
				initial rule, but we'll support it anyway.

			rules[RN][0] == DW_CFA_register, rules[RN][1] == RX
				RX is the register number (i.e., rule column
				number), in which RN is preserved.

			rules[RN][0] == DW_CFA_i960_pfp_offset,
			rules[RN][1] == offset
				Indicates that the register RN is saved in
				the previous frame, at the given offset.
				This is used for the 80960 local registers.
		 */
} Dwarf_cie;

extern Dwarf_cie* dwarfout_cie(/* Dwarf_cie* */);

/* Define a data structure that holds enough information to represent
   a function's FDE.  During code generation and prologue/epilogue generation,
   we record information here.  We emit the FDE when final_end_function()
   calls dwarfout_end_epilogue().
 */

typedef struct dwarf_fde {
	Dwarf_cie	*cie;	/* The cie associated with thie FDE. */
	Dwarf_cie	*call_cie;
		/* call_cie is used only for leaf functions.  For such
		   functions we emit two CIEs and three FDEs.  One CIE
		   is emitted for the call entry point, and one for
		   the bal entry point.  One FDE (using the call CIE)
		   is emitted for the instructions at the call entry
		   point that set g14 and fall through to the bal entry point.
		   Another FDE is emitted for the bal portion of the function.
		   Finally, an FDE is emitted for the 'ret' instruction
		   at the end of the function.  This FDE reuses the
		   call entry point CIE.
		   "call_cie" references the CIE to be associated with
		   the call FDE.
		 */

	int		leaf_return_label_num;
		/* Number N used in the label LRN: at the return
		   instruction of a leaf procedure.  (-1) indicates
		   the label was not emitted, perhaps because the
		   function is not a leaf procedure.
		 */
	int		leaf_return_reg;
		/* CIE column indicating the register that g14 was
		   copied into in a leaf function.
		 */

	char	*name;
		/* Name of the function as emitted in assembler,
		   without the leading '_'.  Space is xmalloc'd
		   in i960_function_name_declare, and freed
		   in dwarfout_end_epilogue.  This is a hack to ensure
		   we use the same name emitted by ASM_DECLARE_FUNCTION_NAME.
		 */

	int		prologue_label_num;
		/* Number used in the .text label emitted after the
		   prologue code.  0 indicates no label was emitted because
		   no registers needed to be saved in the prologue.
		 */
	int		clear_g14_label_num;
		/* Number used in the .text label emitted before "mov 0,g14"
		   in an interrupt function.  Normally, we specify the save
		   rules for all registers after the entire prologue.  But
		   in this case we need to specify g14's rule before clearing
		   it.  0 indicates no such label was emitted.
		 */

	unsigned long	prologue_rules[DWARF_CIE_MAX_REG_COLUMNS][2];
		/* This array specifies the rules to be emitted after
		   the prologue.  Not used if prologue_label_num == -1.
		   For a given register number RN, rules[RN][0] is an opcode
		   and rules[RN][1] is the operand needed by that opcode
		   (if any).  The possible opcodes are as follows:

		   DW_CFA_nop	Indicates that register RN was not preserved
				by any prologue code, so that no rules need
				to be emitted for it.

		   DW_CFA_offset  Indicates that register RN was saved
				at offset prologue_rule[RN][1] off of
				this function's CFA.

		   DW_CFA_register  Indicates that register RN was saved
				in register prologue_rule[RN][1].
		 */

	int		num_epilogues_emitted;
		/* Number of times that epilogue code was emitted to
		   restore registers before returning.  Must be 0 if
		   prologue_label_num is 0.  Each time epilogue code is
		   emitted, three .text labels are defined and used
		   as follows:

			in .text		in .debug_frame
			=================	==========================
			Label_N:		advance location to Label_N
						DW_CFA_remember_state

			code to
			restore registers

			Label_N+1:		advance location to Label_N+1
						DW_CFA_restore for each
							restored register

			return instruction

			Label_N+2:		advance location to Label_N+2
						DW_CFA_restore_state
			=================	==========================

		   The epilogue restoration rules are identical in each
		   epilogue instance.  The only difference between epilogue
		   instances is that the labels must be distinct.  To achieve
		   this, we use prologue_label_num as a base counter.  Each
		   epilogue instance will use labels containing counter+1,
		   counter+2 and counter+3.  The base counter will be bumped
		   by three after each epilogue emission.
		 */
} Dwarf_fde;

extern Dwarf_fde dwarf_fde;
	/* This structure must be filled in appropriately during code
	   generation and as the prologue and epilogue are created.
	   It is emitted as the FDE in dwarfout_end_epilogue.
	 */

#if !defined(DWARF_FDE_CODE_LABEL_CLASS)
#define DWARF_FDE_CODE_LABEL_CLASS	"L_DFDEC"
	/* This label class is used in ASM_GENERATE_INTERNAL_LABEL to
	   generate labels in .text near prologue and epilogue instructions.
	   The labels are referenced from FDEs in the .debug_frame section.
	 */
#endif

extern int	dwarf_fde_code_label_counter;

extern void dwarfout_le_boundary(/* labelnum */);
extern unsigned int	func_last_le_boundary_number;
	/* Number used in label at end of function, which marks the upper
	   bound of the valid range for some location expressions.
	   This is initialized in dwarfout_begin_function().
	   This needs to be made public because it must be accessed
	   by ASM_DECLARE_FUNCTION_NAME when it emits a CAVE stub function.
	 */

#if !defined(MAX_ARTIFICIAL_LABEL_BYTES)
#define MAX_ARTIFICIAL_LABEL_BYTES	32
#endif

#endif /* DWARF_DEBUGGING_INFO */

#endif /* __I_DWARF2_H__ */
