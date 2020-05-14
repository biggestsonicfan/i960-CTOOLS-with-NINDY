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

/******************************************************************
	Below is system initialization code and tables.
	The code builds the PRCB in memory, sets up the stack frame,
	the interrupt, fault, and system procedure tables, and
 	then vectors to a user defined routine.
******************************************************************/
	
/* ------ declare the below symbols public */

	.globl	system_address_table
	.globl	prcb_ptr
	.globl	_prcb_ram
	.globl	start_ip
	.globl	cs1
	.globl  comm_area
	
	.globl	_reinit_iac 	/* "reset" command		*/
	.globl	_nindy_stack 	/* used for nindy commands	*/
	.globl	_intr_stack	/* used for interrupts		*/

/* ------ define IAC address */

	.set	local_IAC, 0xff000010

/* ------ core initialization block (located at address 0)  */
/* ------   ( 8 words)					    */

	.text
start:

system_address_table:
	.word	system_address_table  /*  0 - SAT pointer 	*/
	.word	prcb_ptr	      /*  4 - PRCB pointer   */
	.word	0
	.word	start_ip	      /* 12 - Pointer to first IP       */
	.word	cs1		      /* 16 - calculated at link time   */
	.word	0		      /* 20 - cs1= -(SAT + PRCB + startIP) */
	.word	0
	.word	-1
	
	/*  NINDY config information */
	.word 	0
	.word	0
	.word	1
	.word	0
	
	.space 72

	.word sys_proc_table		/* 120 - initialization words */
	.word 0x304000fb

	.space 8

	.word system_address_table	/* 136 -			*/
	.word 0x00fc00fb		/* 140 - initialization words */
	
	.space 8

	.word sys_proc_table		/* 152 - initialization words */
	.word 0x304000fb
	
	.space 8

	.word fault_proc_table		/* 168 - initialization words */
	.word 0x304000fb
	

/* ------ initial PRCB  */

/* ------ This is our startup PRCB.  After initialization, */
/* ------ this will be copied to RAM 			   */
	.align 6
prcb_ptr:
	.word	0x0         	#   0 - reserved
	.word	0xc		#   4 - initialize to 0x0c 
	.word	0x0         	#   8 - reserved
	.word	0x0 	  	#  12 - reserved 
	.word	0x0	  	#  16 - reserved 
	.word	_intr_table	#  20 - interrupt table address
	.word	_intr_stack	#  24 - interrupt stack pointer
	.word	0x0		#  28 - reserved
	.word	0x000001ff	#  32 - pointer to offset zero
	.word	0x0000027f	#  36 - system procedure table pointer
	.word	fault_table	#  40 - fault table
	.word	0x0		#  44 - reserved
	.space	12		#  48 - reserved
	.word	0x0		#  60 - reserved
	.space	8		#  64 - reserved
	.word	0x0		#  72 - reserved
	.word	0x0		#  76 - reserved
	.space	48		#  80 - scratch space (resumption)
	.space	44		# 128 - scratch space ( error)


/* The system procedure table will only be used if the 	*/
/* user makes a supervisor procedure call     		*/
	.align 6
sys_proc_table:
	.word	0			# Reserved  
	.word	0			# Reserved
	.word	0			# Reserved
	.word	(_trap_stack + 0x01)	# Supervisor stack pointer      
	.word	0			# Preserved
	.word	0			# Preserved
	.word	0			# Preserved
	.word	0			# Preserved
	.word	0			# Preserved
	.word	0			# Preserved
	.word	0			# Preserved
	.word	0			# Preserved
	.word	(_console_io + 0x2)	# 0 - console I/O routines 
	.word	(_file_io + 0x2)	# 1 - remote host service request
	.word	(_lpt_io + 0x2)		# 2 - laser printer I/O routines
	.word	0			# 3 - reserved for CX compatibility
	.word	0 			# 4 - reserved for CX compatibility
	.word	0			# 5 - reserved for CX compatibility


/* Below is the fault table for calls to the fault handler.   */
/* This table is provided because the above table (supervisor */
/* table) will allow tracing of fault events, whereas this    */
/* table will not allow tracing of fault events 	      */

	.align 6
fault_proc_table:
	.word	0				# Reserved
	.word	0				# Reserved
	.word	0				# Reserved
	.word	_trap_stack 			# Supervisor stack pointer      
	.word	0				# Preserved
	.word	0				# Preserved
	.word	0				# Preserved
	.word	0				# Preserved
	.word	0				# Preserved
	.word	0				# Preserved
	.word	0				# Preserved
	.word	0				# Preserved
	.word	(_switch_stack_on_fault + 0x2)	# Fault Handler 
	.word	(_switch_stack_on_fault + 0x2)	# Trace Handler 


/*  --- Processor starts execution at this spot after reset. */
start_ip:

# --  Call board-specific routine to initialize DRAM if necessary.
# --  May clobber any/all registers.
# --  Compiler expects g14 = 0

		mov	0, g14
		call	_init_dram

_reinit_iac:	/* re-entry point for the "rs" command */

/* --  Copy the .data area into RAM.  It has been packed      */
/* --  in the EPROM after the code area, so call a routine to */
/* --  move it 						      */

		bal	_move_data_area
		mov	0, g14

/* --   copy the interrupt table to RAM */

		lda	1024, g0	# load length of int. table
		lda	0, g4		# initialize offset to 0
		lda	_intr_table, g1	# load source 
		lda	intr_ram, g2	# load address of new table
		bal	move_data	# branch to move routine

# --
# --   copy PRCB to RAM space, located at _prcb_ram
# --
		lda	176, g0		# load length of PRCB
		lda	0, g4		# initialize offset to 0
		lda	prcb_ptr, g1	# load source
		lda	_prcb_ram, g2	# load destination
		bal	move_data	# branch to move routine
# --
# --  fix up the PRCB to point to a new interrupt table
# --
		lda	intr_ram, g12	# load address
		st	g12,20(g2)	# store into PRCB

# 
# --  At this point, the PRCB, and interrupt table have 
# --  been moved to RAM.  It is time
# --  to issue a REINITIALIZE IAC, which will start us anew with
# --  our RAM based PRCB.
# --  
# --  The IAC message, found in the 4 words located at the
# --  reinitialize_iac label, contain pointers to the current
# --  System Address Table, the new RAM based PRCB, and to
# --  the Instruction Pointer labeled start_again_ip
#
		lda local_IAC, g5	
		lda reinitialize_iac, g6	
		synmovq	 g5, g6

# --
# --   Below is the software loop to move data
# --
move_data:	
		ldq	(g1)[g4*1], g8	  # load 4 words into g8
		stq	g8, (g2)[g4*1] 	  # store to RAM block
		addi	g4,16, g4	  # increment index	
		cmpibg	g0,g4, move_data  # loop until done
		bx	(g14)

fix_stack:	flushreg
		or	pfp, 7, pfp	# put interrupt return
					# code into pfp
#
# 	we have reserved area on the stack before the call to this
#	routine.  We need to build a phony interrupt record here
# 	to force the processor to pick it up on return.  Also, we
# 	will take advantage of the fact that the processor will
#	restore the pc and ac to it's registers
#

		ldconst	0x1f0002, g0
		st	g0, -16(fp)	# store contrived pc
		ldconst	0x3b001000, g0 	# set up arith. controls 
		st	g0, -12(fp)	# store contrived ac
		ret

# --  The processor will begin execution here after being
# --  reinitialized.  We will now set up the stacks and continue.
# --
start_again_ip:
		call	_disable_ints	  # disable board interrupts
#
#	Before call to main, we need to take the processor out
#	of the "interrupted" state.  In order to do this, we will
# 	execute a call statement, then "fix up" the stack frame
#	to cause an interrupt return to be executed.
#
		ldconst	64, g0		# bump up stack to make
		addo	sp, g0, sp	# room for simulated
					# interrupt frame

		call	fix_stack	# routine to turn off int state

		lda	_nindy_stack,fp # set up user stack space
		lda	-0x40(fp), pfp	# load pfp (just in case)
		lda	0x40(fp), sp	# set up current stack ptr

		mov	 0, g14		# g14 used by C compiler
					# for argument lists of
					# more than 12 arguments.
					# Initialize to 0.

/* -- 	initialize floating point registers, if any */
		callx	_init_fp

/* --   If you are using just the boot portion of NINDY, this
 * --	is the point where your main code is called. If any
 * -- 	I/O needs to be set up, you should do it here before
 * --	your call to main.  No opens have been done for STDIN
 * --	STDOUT or STDERR.
 */

		call	 _pre_main	# this would normally be 
					# "callx _main" for a 
					# standalone program

	.align	4
reinitialize_iac:	
	.word	0x93000000		# reinitialize IAC message
	.word	system_address_table 
	.word	_prcb_ram		# use newly copied PRCB
	.word	start_again_ip		# start here 


/* When NINDY runs on a PC board, communication with DOS takes place through
 * shared memory at the following absolute addresses:
 *
 *	0x0800	serial I/O pseudo-port
 *	0x1100	file I/O buffers
 *	0x1600	file I/O semaphore
 */
	.org	start+0x800
comm_area:
	.org	start+0x1100
buff_io:
	.org	start+0x1600
semaphore:
	.space	4

	.bss	intr_ram, 1028, 6
	.bss	_prcb_ram, 176, 6

# -- Stacks
#	The _trap_stack should never get used because we never take
#	the processor out of supervisor mode (and thus never transition
#	into it).  If application code will us out of supervisor mode, 
#	care must be taken to increase the size of the _trap_stack.
#
	.bss	_nindy_stack, 0x2000, 6	# NINDY's stack 
	.bss	_intr_stack,  0x0200, 6	# interrupt stack
	.bss	_trap_stack,  0x0100, 6	# fault (supervisor) stack
