/*******************************************************************************
 * 
 * Copyright (c) 1996 Intel Corporation
 * 
 * Intel hereby grants you permission to copy, modify, and distribute this
 * software and its documentation.  Intel grants this permission provided
 * that the above copyright notice appears in all copies and that both the
 * copyright notice and this permission notice appear in supporting
 * documentation.  In addition, Intel grants this permission provided that
 * you prominently mark as "not part of the original" any modifications
 * made to this software or documentation, and that the name of Intel
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software or the documentation without specific,
 * written prior permission.
 * 
 * Intel Corporation provides this AS IS, WITHOUT ANY WARRANTY, EXPRESS OR
 * IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY
 * OR FITNESS FOR A PARTICULAR PURPOSE.  Intel makes no guarantee or
 * representations regarding the use of, or the results of the use of,
 * the software and documentation in terms of correctness, accuracy,
 * reliability, currentness, or otherwise; and you rely on the software,
 * documentation and results solely at your own risk.
 *
 * IN NO EVENT SHALL INTEL BE LIABLE FOR ANY LOSS OF USE, LOSS OF BUSINESS,
 * LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES
 * OF ANY KIND.  IN NO EVENT SHALL INTEL'S TOTAL LIABILITY EXCEED THE SUM
 * PAID TO INTEL FOR THE PRODUCT LICENSED HEREUNDER.
 * 
 ******************************************************************************/

/*
 *	dispatch - used by compression assisted virtual execution (CAVE).
 *
 *	The dispatch is called by interceptors. Interceptors pass information
 *	about secondary functions as follows:
 *		
 *		r4 - Address of the first word of the compressed function
 *		-4(r4) - Original size of the secondary function
 *		-2(r4) - Compressed size of the secondary function, if zero
 *			 function is uncompressed, use memcpy to load.
 *
 */

	.file "dispatch.as"
	.text
	.link_pix


	.align	4
#if defined(__i960KA)||defined(__i960KB)||defined(__i960SA)||defined(__i960SB)
flush_cache_iac:
        .word   0x89000000, 0, 0, 0
#endif
	.globl	__cave_dispatch
__cave_dispatch:
	flushreg
	ld	16(pfp), r4	/* Address of the function was in r4.  After 
				   flushreg the value is in 16(pfp) */
	lda     0(ip), r3
	lda     ., r9
	subo    r9, r3, r3		/* Compute the bias */
	addo	r3, r4, r4		/* Adjust the function location */
	ld	8(pfp), r8		/* Get the current rip */
	mov	0, r10			/* Last activation frame seen */
	ld	(pfp), r7		/* Get the previous frame */
	mov	pfp, r11		/* Current activation frame */
search_the_stack:			/* Search the stack for the function */
	ld	8(r7), r9		/* Get the previous rip */
	cmpo	r8, r9			/* Compare current and previous rip */
	bne.t	skip			/* No active function in this frame */
	mov	r11, r10		/* Remember the activation candidate */
skip:
	mov	r7, r11			/* Remember the activation frame */
	ld	(r7), r7		/* Get the previous frame */
	cmpo	r7, 0			/* Last frame ? */
	bne.t	search_the_stack	/* Continue the search */	
	cmpo	0, r10			/* Is active frame found? */
	bne.f	invoke_func		/* Invoke function */

		/* Function not found, activate here */
	ld	-4(r4), r5		/* load original size */
	chkbit	0, r5			/* Set means function was compressed */
	clrbit	0, r5, r5		/* Clear the bit for true size value */
	lda	16(sp)[r5], sp		/* Allocate the activation buffer */
	movq	g0, r12			/* Save g0 - g3 */

	lda	80(fp), g0		/* Pass the activation buffer */
	mov	r4, g1			/* Pass the compressed start */
	mov	r5, g2			/* Pass the uncompressed size */
	movq	g4, r4			/* Save g4 - g7 */
	movq	g8, r8			/* Save g8 - g11 */
	stt	g12, 64(fp)		/* Save g12 - g14 */
	be.t	decompress		/* Function is compressed, decompress */
	call	_memcpy			/* Function is not compressed, copy */
	b	activated
decompress:
	ld	__decompression_table(r3), g3	/* Pointer the decode table */
	addo	r3, g3, g3		/* Adjust for pic */
	call	__decompress_buffer	/* Decompress the function */
activated:

#if defined(__i960KA)||defined(__i960KB)||defined(__i960SA)||defined(__i960SB)
        lda     0xff000010, g0
        lda     flush_cache_iac(r3), g1
        synmovq g0, g1			/* Invalidate the instruction cache */
#else
        ldconst 0x100, g1
        sysctl  g1, g0, g0		/* Invalidate the instruction cache */
#endif
	movq	r12, g0			/* Restore g0 - g3 */
	movq	r4, g4			/* Restore g4 - g7 */
	movq	r8, g8			/* Restore g8 - g11 */
	ldt	64(fp), g12		/* Restore g12 - g14 */
	mov	fp, r10			/* Activated on the current frame */

invoke_func:
	st	r3, 12(fp)		/* A pic function will restore r3 */
	callx	80(r10)			/* Invoke the secondary function */
	ret

