
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
/* **************************************** */
/*      User Fault Table                    */
/* 	                                    */
/*  This NINDY table is built for the 960CA */
/* and uses different fault entries than    */
/* the Kx version                           */
/* 	                                    */
/* generic fault handler: entry 0 	    */
/* trace fault handler:	entry 1 	    */
/* **************************************** */

 	.globl	fault_table 
	.align	4
fault_table:
	.word	(3 << 2) | 2	
	.word	0x27f	# Override Fault (MC)
	.word	(4 << 2) | 2	
	.word	0x27f	# Trace Fault
	.word	(3 << 2) | 2     
	.word	0x27f	# Operation Fault
	.word	(3 << 2) | 2
	.word	0x27f	# Arithmetic Fault
	.word	(3 << 2) | 2
	.word	0x27f	# Floating Point Fault
	.word	(3 << 2) | 2
	.word	0x27f	# Constraint Fault
	.word	(3 << 2) | 2	
	.word	0x27f	# Virtual Memory Fault (MC)
	.word	(3 << 2) | 2
	.word	0x27f	# Protection Fault
	.word	(3 << 2) | 2
	.word	0x27f	# Machine Fault
	.word	(3 << 2) | 2
	.word	0x27f	# Structural Fault (MC) 
	.word	(3 << 2) | 2  
	.word	0x27f	# Type Fault
	.word	(3 << 2) | 2
	.word	0x27f	# Type 11 Reserved Fault Handler
	.word	(3 << 2) | 2
	.word	0x27f	# Process Fault (MC)
	.word	(3 << 2) | 2	
	.word	0x27f   # Descriptor Fault (MC)
	.word	(3 << 2) | 2	
	.word	0x27f	# Event Fault (MC)
	.word	(3 << 2) | 2	
	.word	0x27f	# Type 15 Reserved Fault Handler
	.word	(3 << 2) | 2	
	.word	0x27f	# Type 16 Reserved Fault Handler
	.word	(3 << 2) | 2	
	.word	0x27f	# Type 17 Reserved Fault Handler
	.word	(3 << 2) | 2	
	.word	0x27f	# Type 18 Reserved Fault Handler
	.word	(3 << 2) | 2	
	.word	0x27f	# Type 19 Reserved Fault Handler
	.word	(3 << 2) | 2	
	.word	0x27f	# Type 20 Reserved Fault Handler
	.word	(3 << 2) | 2	
	.word	0x27f	# Type 21 Reserved Fault Handler
	.word	(3 << 2) | 2	
	.word	0x27f	# Type 22 Reserved Fault Handler
	.word	(3 << 2) | 2	
	.word	0x27f	# Type 23 Reserved Fault Handler	
	.word	(3 << 2) | 2	
	.word	0x27f	# Type 24 Reserved Fault Handler	
	.word	(3 << 2) | 2	
	.word	0x27f	# Type 25 Reserved Fault Handler	
	.word	(3 << 2) | 2	
	.word	0x27f	# Type 26 Reserved Fault Handler	
	.word	(3 << 2) | 2	
	.word	0x27f	# Type 27 Reserved Fault Handler	
	.word	(3 << 2) | 2	
	.word	0x27f	# Type 28 Reserved Fault Handler	
	.word	(3 << 2) | 2	
	.word	0x27f	# Type 29 Reserved Fault Handler	
	.word	(3 << 2) | 2	
	.word	0x27f	# Type 30 Reserved Fault Handler	
	.word	(3 << 2) | 2	
	.word	0x27f	# Type 31 Reserved Fault Handler	
