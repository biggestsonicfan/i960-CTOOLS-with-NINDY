
/*(c**************************************************************************** *
 * Copyright (c) 1990, 1991, 1992, 1993 Intel Corporation
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
 ***************************************************************************c)*/

/*
 * routines to write memory image file
 */

#include "p7mem.h"
#if defined(MSDOS) || defined(MWC)
#include "procdef2.h"
#endif

#ifdef VMS
#include "vmsutils.h"
#else
#define EXIT_OK 0
#define EXIT_FAIL 1
#endif

#include "paths.h"
/* the includes and declaration below were added during update to common error
handling 9/29/89 -- robertat */
#include "sgs.h"
#include "err_msg.h"
#include <setjmp.h>
extern jmp_buf parse_err;

copy_mem_seg(lp,ifp,scn)
LDFILE *lp; /* COFF file */
BFILE *ifp;	/* image file */
SCNHDR *scn;
{
	unsigned long start = scn->s_paddr;
	unsigned long bytes_to_read, bytes_read;

#ifdef DEBUG
	printf("Starting write at %lx\n",start);
	printf("Image_current is %lx\n",image_current);
#endif
	if (image_current) {
#ifdef DEBUG
	printf("Before bfseek, ifp is %lx\n",(unsigned long)ifp);
#endif
		if (bfseek(ifp,start-image_current,1))  {
			/* the line below was commented out and the one
			following it added during update to common error
			handling 9/29/89 -- robertat
			perror("fseek for padding failed\n");
			*/
			error_out(ROMNAME,ROM_PAD_FSEEK,0,"image file",
				scn->s_name);
			longjmp(parse_err,1);
		}
#ifdef DEBUG
	printf("After bfseek, ifp is %lx\n",(unsigned long)ifp);
#endif
	}
	image_current = start;

#ifdef DEBUG
		printf("writing section starting at 0x%lx, len 0x%lx\n",
		    start,scn->s_size);
#endif

	/*
	 * Copy the section from COFF file to .mem file
	 */
	bytes_to_read = scn->s_size;

#if defined(MSDOS) && !defined(MWC)
	assert( min(BUFSIZE, bytes_to_read) < 65536l );
#endif
	while (bytes_read = FREAD(buf, 1, min(BUFSIZE, bytes_to_read), lp)) {
		bytes_to_read -= bytes_read;
		if (bfwrite (buf, 1, bytes_read, ifp) < bytes_read) {
			/* the line below was commented out and the one
			following added during update to common error
			handling 9/29/89 -- robertat 
			perror("fwrite failed to image file");
			*/
			error_out(ROMNAME,ROM_NO_IMAGE_WRITE,0);
			longjmp(parse_err,1);
		}
		image_current += bytes_read;
	}
#ifdef DEBUG
	printf("Exiting copy_mem_seg, image_current is %lx\n",image_current);
#endif
}
