
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
/* generic fault handler: entry 0 	    */
/* trace fault handler:	entry 1 	    */
/* **************************************** */

 	.globl	fault_table 
	.align	8
fault_table:
	.word	2	
	.word	0x2bf	# Override Fault (MC)
	.word	6     
	.word	0x2bf	# Trace Fault
	.word	2     
	.word	0x2bf	# Operation Fault
	.word	2
	.word	0x2bf	# Arithmetic Fault
	.word	2
	.word	0x2bf	# Floating Point Fault
	.word	2
	.word	0x2bf	# Constraint Fault
	.word	2	
	.word	0x2bf	# Virtual Memory Fault (MC)
	.word	2
	.word	0x2bf	# Protection Fault
	.word	2
	.word	0x2bf	# Machine Fault
	.word	2
	.word	0x2bf	# Structural Fault (MC) 
	.word	2  
	.word	0x2bf	# Type Fault
	.word	2
	.word	0x2bf	# Type 11 Reserved Fault Handler
	.word	2
	.word	0x2bf	# Process Fault (MC)
	.word	2	
	.word	0x2bf   # Descriptor Fault (MC)
	.word	2	
	.word	0x2bf	# Event Fault (MC)
	.word	2	
	.word	0x2bf	# Type 15 Reserved Fault Handler
	.word	2	
	.word	0x2bf	# Type 16 Reserved Fault Handler
	.word	2	
	.word	0x2bf	# Type 17 Reserved Fault Handler
	.word	2	
	.word	0x2bf	# Type 18 Reserved Fault Handler
	.word	2	
	.word	0x2bf	# Type 19 Reserved Fault Handler
	.word	2	
	.word	0x2bf	# Type 20 Reserved Fault Handler
	.word	2	
	.word	0x2bf	# Type 21 Reserved Fault Handler
	.word	2	
	.word	0x2bf	# Type 22 Reserved Fault Handler
	.word	2	
	.word	0x2bf	# Type 23 Reserved Fault Handler	
	.word	2	
	.word	0x2bf	# Type 24 Reserved Fault Handler	
	.word	2	
	.word	0x2bf	# Type 25 Reserved Fault Handler	
	.word	2	
	.word	0x2bf	# Type 26 Reserved Fault Handler	
	.word	2	
	.word	0x2bf	# Type 27 Reserved Fault Handler	
	.word	2	
	.word	0x2bf	# Type 28 Reserved Fault Handler	
	.word	2	
	.word	0x2bf	# Type 29 Reserved Fault Handler	
	.word	2	
	.word	0x2bf	# Type 30 Reserved Fault Handler	
	.word	2	
	.word	0x2bf	# Type 31 Reserved Fault Handler	
