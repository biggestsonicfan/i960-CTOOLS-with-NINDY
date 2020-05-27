
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

#include <stdio.h>
/* following two included added 9/28/89 during update to common error
handling */
#include "err_msg.h"
#include "rom960.h"

#define GULP 16		/* number of bytes per record */
#define	SIXTY_FOUR_K	65536
#define BYTE_MAX	256
#define SHIFT_R_12	4096 /* divide by 4096 to shift right 12 */

extern char *get_file_name();
extern FILE *get_asc_file_w();
extern char *copy();
extern int interactive;

ihex(s)
char *s;
{
	char rdbuf[256], wrbuf[256];
	char *fname;
	FILE* fd;
	unsigned long addr, cnt, mode;
	/* These are needed to cosset DOS's 16-bit integers */
	unsigned shortaddr,shortcnt;
	FILE *fpOut;

	fname = copy(get_file_name(&s,"rom image to convert to hex:"));
	fpOut = get_asc_file_w(&s, "name of intel hex ouput file:");
	/* always ask for mode in interactive mode; look for it
	in configuration file only if we haven't hit the end of the
	current command line */
	mode = MODE32;
	skip_white(&s);
	if ((interactive) || (*s != '\0'))
		mode = get_mode(&s,"MODE16 or MODE32?");
	if ((mode != MODE16) && (mode != MODE32)){
		error_out(ROMNAME,ROM_BAD_IHEX_MODE,0);
		return(1);
	}

	if ((fd = fopen(fname, FOPEN_RDONLY)) == NULL) {
		error_out(ROMNAME,NOT_OPEN,0,fname);
		return(1);
	}

	/* emit initial extended address record, per below */
	addr = 0;
	rdbuf[0] = 0;
	rdbuf[1] = 0;
	if (mode == MODE32)
		binihex(rdbuf, wrbuf, 0, 2, 4);
	else 
		binihex(rdbuf, wrbuf, 0, 2, 2);
	fprintf(fpOut,"%s\n",wrbuf);

	while ((cnt=fread(rdbuf, 1, GULP, fd)) > 0) {
		shortaddr = (unsigned) (addr%0x10000);
		shortcnt = (unsigned) (cnt%0x10000);
		binihex(rdbuf, wrbuf, shortaddr , shortcnt, 0);	/* data record */
		fprintf(fpOut,"%s\n",wrbuf);
		addr += cnt;

		/*
		 * every 64k bytes, issue an 'extended address record'
		 * containing an (ignored) address, and two bytes of
		 * data containing bits 4-19 of the new segment base
		 * address for MODE16 and the top 16 bits of the linear
		 * address for MODE32.
		 *
		 * on jeff g's request - mcg - 11/12/86
		 */

		if ((addr%SIXTY_FOUR_K) == 0) {
			if (mode == MODE32) {
				rdbuf[0] = (addr/SIXTY_FOUR_K)/BYTE_MAX;
				rdbuf[1] = (addr/SIXTY_FOUR_K)%BYTE_MAX;
				binihex(rdbuf, wrbuf, 0, 2, 4);
			}
			else {
				/* in MODE16, we want to get only bits
				16-19 of a (supposedly) 20-bit address,
				the least significant three bits of which
				are assumed to be zero */
				rdbuf[0] = addr/SHIFT_R_12;
				rdbuf[1] = 0;
				binihex(rdbuf, wrbuf, 0, 2, 2);
			}
			fprintf(fpOut,"%s\n",wrbuf);
		}
	}
	fprintf(fpOut,":00000001FF\n");			/* end record */
	fclose (fpOut);
	(void) fclose(fd);
	return(0);
}
