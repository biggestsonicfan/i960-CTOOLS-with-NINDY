/*****************************************************************************
 * 
 * Copyright (c) 1995 Intel Corporation
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
 ****************************************************************************/

#include <stdio.h>
#include "libdwarf.h"
#include "internal.h"

/* global error number descriptor that the client can see */
Dwarf_Unsigned _lbdw_errno = DLE_NE;

/* global error handler; can be reset by client in dwarf_seterrhand */
Dwarf_Handler _lbdw_errhand = _dw_error_handler;

/* global error arg; can be reset by client in dwarf_seterrarg */
Dwarf_Addr _lbdw_errarg = NULL;

/* global temp variable used in LIBDWARF_ERR macro */
Dwarf_Signed _lbdw_errsev;

/* 
 * This is the 'official' error table, the fields specify the 
 * error number (defined in libdwarf.h), message, optional message, 
 * and severity.  Severity is defined as: 
 * 
 * DLS_FATAL 	-1
 * DLS_WARNING 	0
 * DLS_ERROR 	1
 *
 */
static struct Dwarf_Error error_list[] = 
{
/* Warning */
{ DLE_NE, "no error", DLS_WARNING },
{ DLE_UNK_AUG, "unknown augmentation found for CIE", DLS_WARNING },

/* Error */
{ DLE_VMM, "version of dwarf info newer than libdwarf", DLS_ERROR },
{ DLE_MAP, "memory map failure", DLS_ERROR },
{ DLE_ID, "invalid descriptor", DLS_ERROR },
{ DLE_IOF, "I/O failure", DLS_ERROR },
{ DLE_IA, "invalid argument", DLS_ERROR },
{ DLE_MDE, "mangled debugging entry", DLS_ERROR },
{ DLE_MLE, "mangled line number entry", DLS_ERROR },
{ DLE_FNO, "file descriptor doesn't refer to open file", DLS_ERROR },
{ DLE_FNR, "not a regular file", DLS_ERROR },
{ DLE_FWA, "file opened with wrong access", DLS_ERROR },
{ DLE_NOB, "not an object file", DLS_ERROR },
{ DLE_UNKN, "unknown error", DLS_ERROR },
{ DLE_D1, "dwarf1 information", DLS_ERROR },
{ DLE_IH, "invalid handle", DLS_ERROR },
{ DLE_NOT_D2, "not dwarf 2 information", DLS_ERROR },
{ DLE_NO_ABBR_ENTRY, "no entry in abbrev table", DLS_ERROR },
{ DLE_NO_TAG, "tag == 0 in abbrev table entry", DLS_ERROR },
{ DLE_BAD_CU_LEN, "length of CU contribution to .debug_info unexpected", DLS_ERROR },
{ DLE_BAD_INFO_LEN, "length of .debug_info unexpected", DLS_ERROR },
{ DLE_UNSUPPORTED_FORM, "this form is unsupported", DLS_ERROR },
{ DLE_NO_FORM, "form == 0 in abbrev table entry", DLS_ERROR },
{ DLE_BAD_LOC, "this location is unsupported", DLS_ERROR },
{ DLE_NOABBR, "No .debug_abbrev section", DLS_ERROR },
{ DLE_NOLOC, "No .debug_loc section", DLS_ERROR },
{ DLE_NOINFO, "No .debug_info section", DLS_ERROR },
{ DLE_NOPUB, "No .debug_pubnames section", DLS_ERROR },
{ DLE_NOARANG, "No .debug_aranges section", DLS_ERROR },
{ DLE_NOLINE, "No .debug_line section", DLS_ERROR },
{ DLE_NOFRAME, "No .debug_frame section", DLS_ERROR },
{ DLE_NOMAC, "No .debug_macinfo section", DLS_ERROR },
{ DLE_NOT_CU_DIE, "Expected a CU DIE", DLS_ERROR },
{ DLE_NI, "Function not yet implemented", DLS_ERROR },
{ DLE_BAD_CU_OFFSET, "Invalid CU offset", DLS_ERROR },
{ DLE_BAD_SECT_OFFSET, "Invalid section offset", DLS_ERROR },
{ DLE_NOT_ARRAY, "the given DIE does not represent an array", DLS_ERROR },
{ DLE_BAD_OPCODE, "Unrecognized extended opcode", DLS_ERROR },
{ DLE_BAD_LINENO, "negative line number", DLS_ERROR },
{ DLE_NO_FRAME, "can not lookup the FDE for the given ip", DLS_ERROR },
{ DLE_BAD_FRAME, "the .debug_frame section could not be parsed", DLS_ERROR },
{ DLE_HUGE_LEB128, "leb128 too large to represent", DLS_ERROR },
{ DLE_BAD_FREAD, "Read failed", DLS_ERROR },
{ DLE_BAD_SEEK, "Seek failed", DLS_ERROR },
{ DLE_BAD_SWAP, "Swap failed", DLS_ERROR },
{ DLE_BAD_ERRNUM, "No such error number", DLS_ERROR },
{ DLE_BAD_ERRSEV, "No such error severity", DLS_ERROR },

/* Fatal */
{ DLE_MAF, "memory allocation failure", DLS_FATAL },
{ DLE_MOF, "mangled file headers", DLS_FATAL },
{ DLE_END_ERR_TBL, "END TABLE", DLS_FATAL }
};


/* 
 * _dw_fill_err -- fill up the error descriptor with the 
 * appropriate information for the error handler.
 */
Dwarf_Error
_dw_fill_err(dw_error, file, line)
    Dwarf_Unsigned dw_error;
    char *file;
    int line;
{
    int i = 0;
    Dwarf_Error error_desc;

    error_desc = (Dwarf_Error) _dw_malloc(NULL, sizeof(struct Dwarf_Error));
 
    /* Find the error in the table or end of table */
    while(error_list[i].num != dw_error && error_list[i].num != DLE_END_ERR_TBL) 
	i++;
    
    /* Assign the Dwarf_Error structure */
    error_desc->num = error_list[i].num;
    error_desc->msg = error_list[i].msg;
    error_desc->severity = error_list[i].severity;
    error_desc->file = file;
    error_desc->line = line;

/* 
 * This is a global so that the client can test it and see if an error
 * occured or not.  The libdwarf functions can return conditions that 
 * may or may not be errors (many of them return NULL on error or if the
 * attribute does not exist, for example).  This is a way for the client
 * to quickly check if an error really occured. 
 */
    _lbdw_errno = error_desc->num; 
    return((Dwarf_Error)error_desc);
}

/* 
 * _dw_error_handler -- libdwarf's main error handler, 
 * for now it just spits out the error message and 
 * exit's via libdwarf_exit() without even cleaning up.
 * 
 * There are two possibilities for error handling.
 * 
 * 1) The client provides an error handler.  If the client
 *    uses it's own error handler, it must be sure that 
 *    all the possible errors are covered (as defined in
 *    libdwarf.h).  This function could be an example for 
 *    such an error handler.  Keep in mind that libdwarf
 *    will continue processing if it gets a return value
 *    of WARNING or ERROR from whatever error handler it 
 *    uses, but it will abort if it ever sees a FATAL.  The
 *    client will have to use discretion when assigning the
 *    severity of the error.  Libdwarf will have a default
 *    severity built-in, but these may be over-ridden.  This
 *    method also has the benefit of allowing the client to 
 *    control how and when error information is output.
 *    The severity of the errors are as follows: 
 *
 *        -1 FATAL
 *         0 WARNING 
 *         1 ERROR 
 *
 * 2) Use the built-in error handler.  This function is 
 *    libdwarf's built-in error handler.  This is the default
 *    if no error handler is passed into dwarf_init().  The
 *    client can also call dwarf_seterrhand() with NULL as the
 *    errhand parameter to default to this.  This is a fairly
 *    simple error handler, it will output a diagnostic to 
 *    stderr as it encounters the error and either continue
 *    processing, returning either the ERROR or WARNING 
 *    condition, or abort.  
 */ 
Dwarf_Signed	 
_dw_error_handler(error_desc)
    Dwarf_Error error_desc;
{
    int i = 0;
 
    if(error_desc) {
        switch (error_desc->severity) {
            case DLS_FATAL: fprintf(stderr,"LIBDWARF FATAL ");
                fprintf(stderr,"0x%04X -- %s ", error_desc->num,
                    error_desc->msg);
                fprintf(stderr, "\"%s\", line %d: program aborted by libdwarf\n",
                    error_desc->file, error_desc->line);
                fflush(stderr);
                abort();
                break;
            case DLS_WARNING: fprintf(stderr,"LIBDWARF WARNING ");
                fprintf(stderr,"0x%04X -- %s\n", error_desc->num, error_desc->msg);
                fflush(stderr);
                break;
            case DLS_ERROR: fprintf(stderr,"LIBDWARF ERROR ");
                fprintf(stderr,"0x%04X -- %s\n", error_desc->num, error_desc->msg);
                fflush(stderr);
                break;
	    default:
        	fprintf(stderr, "\"%s\", LIBDWARF FATAL ");
                fprintf(stderr, "\"%s\", line %d: error descriptor not set up\n",
                    error_desc->file, error_desc->line);
        	fflush(stderr);
        	abort();
		break;
    	}
    	return(error_desc->severity);
    }
    else {
       	fprintf(stderr, "\"%s\", LIBDWARF FATAL ");
        fprintf(stderr, "\"%s\", line %d: error descriptor not set up\n",
            error_desc->file, error_desc->line);
        fflush(stderr);
        abort();
    }
}

/* 
    _dw_update_err - change an entry in the error list
    Pass in a filled error descriptor.  The entry in the list will be
    replaced with your version.
*/

void
_dw_update_err(error_desc)
    Dwarf_Error error_desc;
{
    int i = 0;
    Dwarf_Unsigned dw_error = error_desc->num;

    /* Find the error in the table or end of table */
    while(error_list[i].num != dw_error && 
	  error_list[i].num != DLE_END_ERR_TBL) 
	i++;
    
    if (error_list[i].num == DLE_END_ERR_TBL)
	/* "Can't happen" */
	return;

    /* Patch the new error into the error list */
    error_list[i].msg = error_desc->msg;
    error_list[i].severity = error_desc->severity;
}
