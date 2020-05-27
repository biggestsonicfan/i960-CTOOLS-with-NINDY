/*(cb*/
/**************************************************************************
 *
 *     Copyright (c) 1992 Intel Corporation.  All rights reserved.
 *
 *
 * Intel hereby grants you permission to copy, modify, and distribute this
 * software and its documentation.  Intel grants this permission provided
 * that the above copyright notice appears in all copies and that both the
 * copyright notice and this permission notice appear in supporting
 * documentation.  In addition, Intel grants this permission provided that
 * you prominently mark as not part of the original any modifications made
 * to this software or documentation, and that the name of Intel
 * Corporation not be used in advertising or publicity pertaining to the
 * software or the documentation without specific, written prior
 * permission.
 *
 * Intel provides this AS IS, WITHOUT ANY WARRANTY, INCLUDING THE WARRANTY
 * OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, and makes no
 * guarantee or representations regarding the use of, or the results of the
 * use of, the software and documentation in terms of correctness,
 * accuracy, reliability, currentness, or otherwise, and you rely on the
 * software, documentation, and results solely at your own risk.
 *
 **************************************************************************/
/*)ce*/

/* Public define's and function prototypes */
#define EEPROM_SIZE		1024	/* Maximum # bytes in X24C08 eeprom */

/* Values of which_eeprom parameter */
#define SQUALL_EEPROM		1
#define ON_BOARD_EEPROM		2

/* result codes for the functions below */
#define OK			0	/* Operation completed successfully */
#define EEPROM_ERROR		1	/* generic error */
#define EEPROM_NOT_RESPONDING	2	/* eeprom not resp/not installed */
#define EEPROM_TO_SMALL		3	/* req write/read past end of eeprom */

/* global functions declared in c145_eeprom.c */

int eeprom_read (int which_eeprom,	/* one of the define's above */
		 int eeprom_addr,	/* byte offset from start of eeprom */
		 unsigned char *p_data,		/* where to put data in memory */
		 int nbytes		/* number of bytes to read */
		 );

int eeprom_write (int which_eeprom,	/* one of the define's above */
		  int eeprom_addr,	/* byte offset from start of eeprom */
		  unsigned char *p_data,		/* where to get data from memory */
		  int nbytes		/* number of bytes to write */
		  );

int eeprom_installed (int which_eeprom);/* one of the define's above */

/* This structure defines the format of the data in the squall eeproms */
typedef struct {
	unsigned char mcon_byte_0;	/* Byte 0 (LSB) of squall's MCON */
	unsigned char mcon_byte_1;	/* Byte 1 of squall's MCON */
	unsigned char mcon_byte_2;	/* Byte 2 of squall's MCON */
	unsigned char mcon_byte_3;	/* Byte 3 (MSB) of squall's MCON */
	unsigned char intr_det_mode;	/* Int detect mode;see defines below */
	unsigned char reserved_0;	/* Reserved byte 0 */
	unsigned char reserved_1;	/* Reserved byte 1 */
	unsigned char version_byte_0;	/* Byte 0 of squall version in ASCII */
	unsigned char version_byte_1;	/* Byte 1 of squall version in ASCII */
	unsigned char revision_level;	/* Squall's revision level */
} SQUALL_EEPROM_DATA;

/* Values for intr_det_mode field */
#define IRQ_0_LOW_LEVEL		0
#define IRQ_0_FALLING_EDGE	1
#define IRQ_0_MASK		1
#define IRQ_1_LOW_LEVEL		0
#define IRQ_1_FALLING_EDGE	2
#define IRQ_1_MASK		2

/* Offsets into eeprom for generic squall data and module specific data */
#define GENERIC_DATA_OFFSET	0
#define GENERIC_DATA_SIZE	10	/* No sizeof (struct padding) */
#define MODULE_SPECIFIC_DATA_OFFSET	GENERIC_DATA_SIZE
