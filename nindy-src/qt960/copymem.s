
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
	#	_move_data_area
	#
	# 	This routine is meant to initialize the data
	# area expected by the monitor with data.  When the monitor
	# is burned into eprom, the data area is put into rom
	# next to the code (starting at address _etext).  We will
	# need to move the data to it's proper home, (starting
	# at _edata).
	#

	.globl _move_data_area
_move_data_area:

	lda	ram_, r4	# get start address of ram
	lda	_edata, r5	# get end address of data
	lda	_etext, r6	# get end address of code

move_loop:
	cmpibg	r4, r5, targ	# see if done
	ld	(r6), r7	# load data word from ROM
	addo	r6, 4, r6	# increment pointer
	st	r7, (r4)	# store data to memory
	addo	r4, 4, r4	# increment destination
	b	move_loop

targ:
	bx	(g14)
