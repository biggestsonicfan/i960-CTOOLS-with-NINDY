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
	.globl	_user_intr1
	.globl	_user_intr2
	.globl	_user_intr3
	.globl	_user_intr4
	.globl	_user_intr5
	.globl	_user_intr6
	.globl	_user_intr7
	.globl	_user_intr8
	.globl	_user_intr9
	.globl	_user_intr10
	.globl	_user_intr11
	.globl	_user_intr12
	.globl	_user_intr13
	.globl	_user_intr14
	.globl	_user_intr15
	.globl	_user_intr16
	.globl	_user_intr17
	.globl	_user_intr18
	.globl	_user_intr19
	.globl	_user_intr20
	.globl	_user_intr21
	.globl	_user_intr22
	.globl	_user_intr23
	.globl	_user_intr24
	.globl	_user_intr25
	.globl	_user_intr26
	.globl	_user_intr27
	.globl	_user_intr28
	.globl	_user_intr29
	.globl	_user_intr30
	.globl	_user_intr31
	.globl	_user_NMI

_user_intr1:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	1, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr2:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	2, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr3:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	3, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr4:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	4, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr5:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	5, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr6:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	6, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr7:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	7, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr8:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	8, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr9:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	9, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr10:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	10, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr11:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	11, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr12:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	12, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr13:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	13, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr14:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	14, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr15:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	15, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr16:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	16, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr17:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	17, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr18:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	18, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr19:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	19, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr20:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	20, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr21:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	21, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr22:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	22, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr23:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	23, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr24:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	24, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr25:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	25, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr26:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	26, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr27:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	27, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr28:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	28, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr29:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	29, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr30:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	30, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_intr31:
#ifdef DEBUG
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	ldconst	31, g0
	call	_print_interrupt
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
#endif
	ret

_user_NMI:
/* 
	we allocate a spot for a "register holder" on the stack
 	and will use some assembler inlines to store data to that
	spot.  We will take advantage of the fact that this will be
	allocated by the compiler at the first spot on the stack
*/
	ldconst 64, r4
	addo sp, r4, sp

	stq g0, -64(sp)
	stq g4, -48(sp)
	stq g8, -32(sp)
	stt g12, -16(sp)

	call	_print_nmi
	
/* restore the registers before we return */

	ldq -64(sp), g0
	ldq -48(sp), g4
	ldq -32(sp), g8
	ldt -16(sp), g12
	ret
