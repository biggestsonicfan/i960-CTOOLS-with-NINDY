/******************************************************************/
/* 		Copyright (c) 1989, Intel Corporation

   Intel hereby grants you permission to copy, modify, and 
   distribute this software and its documentation.  Intel grants
   this permission provided that the above copyright notice 
   appears in all copies and that both the copyright notice and
   this permission notice appear in supporting documentation.  In
   addition, Intel grants this permission provided that you
   prominently mark as not part of the original any modifications
   made to this software or documentation, and that the name of 
   Intel Corporation not be used in advertising or publicity 
   pertaining to distribution of the software or the documentation 
   without specific, written prior permission.  

   Intel Corporation does not warrant, guarantee or make any 
   representations regarding the use of, or the results of the use
   of, the software and documentation in terms of correctness, 
   accuracy, reliability, currentness, or otherwise; and you rely
   on the software, documentation and results solely at your own 
   risk.							  */
/******************************************************************/
####################################################################
#
#	Below is system initialization code and tables.
#	The code builds the PRCB in memory, sets up the stack frame,
#	the interrupt, control, fault, and system procedure tables, and
# 	then vectors to a user defined routine.
#
####################################################################
	
# ------ declare the below symbols public

	.globl	start_ip
	.globl	_prcb_ram
	.globl	sys_proc_table
	.globl	_reinit_sysctl	# "reset command"

	.globl	_nindy_stack 	# used for NINDY commands
	.globl	_intr_stack	# used for interrupts
        .globl  _control_table  # 960CX control registers

	.text
        .align  4
sys_proc_table:
	.word	0			# Reserved  
	.word	0			# Reserved
	.word	0			# Reserved
	.word	(_trap_stack )		# Supervisor stack pointer      
	.word	0			# Preserved
	.word	0			# Preserved
	.word	0			# Preserved
	.word	0			# Preserved
	.word	0			# Preserved
	.word	0			# Preserved
	.word	0			# Preserved
	.word	0			# Preserved
	.word	(_console_io + 0x2)		# 0 - console I/O routines 
	.word	(_file_io + 0x2)		# 1 - file I/O routines
	.word	(_lpt_io + 0x2)			# 2 - laser printer I/O routines
	.word	(_switch_stack_on_fault + 0x2)	# 3 - Fault Handler 
	.word	(_switch_stack_on_fault + 0x2)	# 4 - Trace Handler 
	.word	(_get_prcb + 0x2)		# 5 - Return PRCB 


start_ip:		# Processor starts execution here after reset.

# --  The A-step 960CA part has bug requiring that memory regions 0 and F
# --  have the same configuration specified in control table during hard
# --  reset.  If the regions are in fact different, we have to provide one
# --  prcb/control table at 0xffffff00 for the hard reset and substitute the
# --  real one now, before accessing any RAM, with a soft reset.  (If the
# --  regions really are the same, the same table may be used both times.)

		ldconst  0x300,r3
		lda  continue, r4
		lda  rom_prcb, r5
		sysctl  r3,  r4,  r5
continue:

# --  Call board-specific routine to initialize dynamic RAM if necessary.
# --  May clobber any/all registers.

		balx	init_dram,g0

# --  Compiler expects g14 = 0

		mov	0, g14

# --
# --  Copy the .data area into RAM.  It has been packed
# --  in the EPROM after the code area, so call a routine to
# --  move it

_reinit_sysctl:		# this is the NINDY "rs" command entry point

		bal	_move_data_area

# --
# --   copy the interrupt table to RAM
# --
		lda	1024, g0	# load length of int. table
		lda	_intr_table, g1	# load source 
		lda	intr_ram, g2	# load address of new table
		bal	move_data	# branch to move routine

# --
# --   copy the control table to RAM
# --
		lda	112, g0		# load length of int. table
		lda	_rom_control_table, g1		# load source 
		lda	_control_table, g2	# load adderss of new table
		bal	move_data	# branch to move routine

# --
# --  copy PRCB to RAM
# --
		lda	64, g0		# load length of PRCB
		lda	rom_prcb, g1	# load source
		lda	_prcb_ram, g2	# load destination
		bal	move_data	# branch to move routine
# --
# --  fix up the PRCB to point to RAM interrupt table
# --
		lda	intr_ram, g12	# load address
		st	g12,16(g2)	# store into PRCB

# --
# --  and change the PRCB to point to RAM control table
# --
		lda	_control_table, g12	# load address
		st	g12,4(g2)	# store into PRCB

# 
# --  At this point, the PRCB, and interrupt table have 
# --  been moved to RAM.  It is time to issue the 
# --  REINITIALIZE sysctl, which will start us anew with
# --  our RAM based PRCB.
# --  
#
		ldconst 0x300, r4
		ldconst start_again_ip, r5
		ldconst _prcb_ram, r6
		sysctl	r4, r5, r6
# -------- execution will now resume at start_again_ip ------- #

# --
# --   Below is the software loop to move data
# --
move_data:	
		ldconst	0, g4		# initialize offset to 0
moveloop:
		ldq	(g1)[g4*1], g8	# load 4 words into g8
		stq	g8, (g2)[g4*1] 	# store to RAM proc. block
		addi	g4,16, g4	# increment index	
		cmpibg	g0,g4,moveloop	# loop until done
		bx	(g14)

# --  The processor will begin execution here after being
# --  reinitialized.  We will now set up the stacks and continue.
start_again_ip:

#	Before call to main, we need to take the processor out
#	of the "interrupted" state.  In order to do this, we will
# 	execute a call statement, then "fix up" the stack frame
#	to cause an interrupt return to be executed.
#
#	The CX interrupt return requires an interrupt frame and
#	a preceding stack frame.  The latter must contain a "valid"
#	(non-0) pfp.  We'll allocate the latter as a dummy frame,
#	and point its pfp at itself.

		st	sp,(sp)		# dummy pfp -> dummy frame
		mov	sp,fp		# allocate dummy frame
		ldconst	64, g0		# .
		addo	sp, g0, sp	# .
		call	fix_stack	# go in as a normal call, come
					#   out as an interrupt return
		lda	_nindy_stack,fp # set up user stack space
		lda	-0x40(fp), pfp	# load pfp (just in case)
		lda	0x40(fp), sp	# set up current stack ptr


		mov	 0, g14		# g14 used by C compiler
					# for arguement lists past
					# 13 arguements.
					# Initialize to 0

		call	_disable_ints	# disable board interrupts

#
# --   call main code from here
#
# --   Note: This setup assumes a main module "main()" written in
# --   C.  Also, no opens are done for STDIN, STDOUT, or STDERR.
# --   If I/O is required, the devices would need to be opened
# --   before the call to main.

		callx	 _main
# ------- execution will never return to this point


#
# --- routine to turn off int state
#
fix_stack:
		flushreg
		or	pfp, 7, pfp	# put interrupt return
					# code into pfp
#
# 	we have reserved area on the stack before the call to this
#	routine.  We need to build a phony interrupt record here
# 	to force the processor to pick it up on return.  Also, we
# 	will take advantage of the fact that the processor will
#	restore the PC and AC to it's registers
#

		ldconst	0xd81f0002, g0  # the upper "d8" is required
					#   by the CX microcode for
					#   interrupt return
		st	g0, -16(fp)	# store contrived PC
		ldconst	0x3b001000, g0 	# set up arith. controls 
		st	g0, -12(fp)	# store contrived AC
		ret

# -- RAM area for copies of the PRCB & interrupt table after initial bootup
#
	.bss	_prcb_ram,       64, 6
	.bss	intr_ram,      1028, 6
	.bss	_control_table, 112, 6


# -- Stacks
#	The _trap_stack should never get used because we never take
#	the processor out of supervisor mode (and thus never transition
#	into it).  If application code will us out of supervisor mode, 
#	care must be taken to increase the size of the _trap_stack.
#
	.bss	_nindy_stack, 0x2000, 6	# NINDY's stack 
	.bss	_intr_stack,  0x0200, 6	# interrupt stack
	.bss	_trap_stack,  0x0100, 6	# fault (supervisor) stack
