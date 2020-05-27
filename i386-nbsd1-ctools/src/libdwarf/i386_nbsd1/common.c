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

/* Miscellaneous support routines used by all other library functions.
   Functions here are not intended to be called directly by the debugger.  */

#include "libdwarf.h"
#include "internal.h"

static int host_byte_order = -1;

/* For _dw_malloc/_dw_free that is NOT associated with a dbg, 
   such as temporary storage, this memory table will keep memory
   allocation/deallocation consistent. */
static Dwarf_Memory_Table *global_memory_table;

/* 
 * _dw_fread -- like fread but with error handling.
 * 
 * ERROR: Returns DLV_NOCOUNT.
 */
Dwarf_Signed
_dw_fread(bufp, size, nitems, stream)
    Dwarf_Small *bufp;
    int size;
    int nitems;
    FILE *stream;
{
    RESET_ERR();
    if (fread(bufp, size, nitems, stream) != nitems ) 
    {
        LIBDWARF_ERR(DLE_BAD_FREAD);
	return(DLV_NOCOUNT); 
    }
}

/*
 * _dw_fseek -- like fseek but with error handling.
 * 
 * ERROR: Returns DLV_BADOFFSET.
 */ 
int
_dw_fseek(stream, offset, start)
    FILE *stream;
    Dwarf_Signed offset;
    int start;
{
    long pos = fseek(stream, offset, start);

    RESET_ERR();
    if (pos == -1) 
    {
        LIBDWARF_ERR(DLE_BAD_SEEK);
   	return(DLV_BADOFFSET);
    }
    return pos;
}

/*
 * _dw_ftell -- like ftell but with error handling.
 * 
 * ERROR: Returns DLV_NOCOUNT
 */ 
long
_dw_ftell(stream)
    FILE *stream;
{
    long pos = ftell(stream);

    RESET_ERR();
    if (pos == -1) 
    {
        LIBDWARF_ERR(DLE_BAD_SEEK);
   	return(DLV_NOCOUNT);
    }
    return pos;
}

/*
 * _dw_find_tabentry -- find an address in the memory table
 */
static 
char **
_dw_find_tabentry(memtab, buf)
    Dwarf_Memory_Table *memtab;
    char *buf;
{
    register int i = 0;
    register int count = memtab->count;
    register char **tab = memtab->table;

    for ( ; i < count; ++i, ++tab )
	if (*tab == buf)
	    return tab;
    return NULL;
}

/*
 * _dw_malloc -- like malloc but with 2 extras:
 *
 * 1. error handling
 * 2. keep track of the memory you allocate.  This makes it easy to free
 *    all memory for a given dbg at one time.  (see dwarf_finish.)
 * 
 * This is a crude, temporary system to implement dwarf_finish (FIXME.)
 *
 * The table only grows, never shrinks, as long as the dbg is alive.
 * ERROR: Returns DLV_BADADDR.
 */
char *
_dw_malloc(dbg, len)
    Dwarf_Debug dbg;
    Dwarf_Unsigned len;
{
    char *p = (char *) malloc(len);
    Dwarf_Memory_Table *memtab;

    RESET_ERR();

    if (p == NULL) 
	    LIBDWARF_ERR(DLE_MAF);

    if (dbg == NULL)
	    memtab = global_memory_table;
    else
	    memtab = dbg->memory_table;

    if (memtab == NULL) {
	/* Don't record memory for the table in the table!  Just make sure
	   to free it last in dwarf_finish. */
	memtab = (Dwarf_Memory_Table *)
		malloc(sizeof(struct dwarf_memory_table));
	if (memtab == NULL)
		LIBDWARF_ERR(DLE_MAF);
	_dw_clear((char *) memtab, sizeof(struct dwarf_memory_table));
	if (dbg)
		dbg->memory_table = memtab;
	else
		global_memory_table = memtab;
    }

    if (memtab->count + 1 > memtab->limit) {
	/* Don't record memory for the table in the table!  Just make sure
	   to free it last in dwarf_finish. */
	if (memtab->table) {
	    /* Double the current table size */
	    memtab->table = (char **) realloc(memtab->table,
					      sizeof(memtab->table[0]) * memtab->limit * 2);
	    if (memtab->table == NULL)
		    LIBDWARF_ERR(DLE_MAF);
	    _dw_clear(memtab->table + memtab->limit, 
		      memtab->limit * sizeof(memtab->table[0]));
	    memtab->limit *= 2;
	}
	else {
	    /* pick a number; we'll increase it later if necessary */
	    memtab->limit = 1024;
	    memtab->table = (char **) malloc(memtab->limit * sizeof(memtab->table[0]));
	    if (memtab->table == NULL)
		    LIBDWARF_ERR(DLE_MAF);
	}
    }
    memtab->table[memtab->count++] = p;
    return p;
}

/*
 * _dw_realloc -- like realloc but with 2 extras:
 *
 * 1. error handling
 * 2. keep track of the memory you allocate.  This makes it easy to free
 *    all memory for a given dbg at one time.  (see dwarf_finish.)
 * 
 * This is a crude, temporary system to implement dwarf_finish (FIXME.)
 *
 * A table entry is assumed to exist for the memory since we're reallocating.
 *
 * ERROR: Returns DLV_BADADDR.
 */
char *
_dw_realloc(dbg, oldbuf, len)
    Dwarf_Debug dbg;
    char *oldbuf;
    int len;
{
    char *newbuf = (char *) realloc(oldbuf, len);
    char **tabentry;

    RESET_ERR();

    if (newbuf == NULL) 
        LIBDWARF_ERR(DLE_MAF);
    
    tabentry = _dw_find_tabentry(dbg ? dbg->memory_table : global_memory_table,
			     oldbuf);
    if (tabentry)
	*tabentry = newbuf;
    return(newbuf);
}

/*
 * _dw_free -- like free but with 2 extras:
 *
 * 1. error handling
 * 2. keep track of the memory you free.  This means you won't attempt
 *    to free it a second time with either _dw_free or dwarf_finish. 
 * 
 * This is a crude, temporary system to implement dwarf_finish (FIXME.)
 */
void
_dw_free(dbg, buf)
    Dwarf_Debug dbg;
    char *buf;
{
    char **tabentry;
    if (buf)
    {
	tabentry = 
	    _dw_find_tabentry(dbg ? dbg->memory_table : global_memory_table,
			      buf);
	if (tabentry)
	    *tabentry = NULL;
	free(buf);
    }
}

/*
 * _dw_clear -- Fill memory with zeroes.
 *
 * ERROR: Returns DLV_BADADDR.
 */
char *
_dw_clear(c, len)
    char *c;
    int len;
{
    RESET_ERR();

    if (c == NULL) 
    {
        if ( LIBDWARF_ERR(DLE_IA) == DLS_ERROR )
            return(DLV_BADADDR); 
    }
    memset(c, 0, len);
    return(c);
}

/* 
 * _dw_check_version -- verifies the dwarf version
 * information vs. a known value
 */
_dw_check_version(version)
    Dwarf_Unsigned version;
{
    RESET_ERR();
    if (version != DWARF_VERSION) 
    {
	LIBDWARF_ERR((version == 1 ? DLE_D1 : DLE_NOT_D2));
	return(0); 
    }
    else
	return(1);
}

/* 
 * Run through an array of sections and find the one 
 * named 'name'.  Don't error out if you can't find it,
 * leave that for the caller to determine, some sections
 * don't have to be there, but the caller would have a 
 * better idea of what's expected.
 */ 
Dwarf_Section
_dw_find_section(dbg, name)
    Dwarf_Debug dbg;
    char *name;
{
    Dwarf_Section my_sections = dbg->dbg_sections;
    Dwarf_Unsigned i;

    RESET_ERR();
    for (i=0; i < dbg->dbg_num_dw_scns; i++) 
    {     
	if (my_sections)
            if (!strcmp(my_sections->name, name)) 
	        return(my_sections);
	    else 
                my_sections++;
	else
       	    if(LIBDWARF_ERR(DLE_MOF) == DLS_ERROR)
                return(DLV_BADADDR); 
    }
    return(NULL); /* if it wasn't found */
}

/* 
 * _dw_leb128_to_ulong -- Convert LEB128 number into unsigned long.
 * Read an unknown number of bytes from the Dwarf file, build up
 * an unsigned long (in host byte order) and return it.
 */
Dwarf_Unsigned
_dw_leb128_to_ulong(dbg)
    Dwarf_Debug dbg;
{
    Dwarf_Unsigned buf, result = 0;
    int more = 1;
    int shift = 0;
    
    do 
    {
    	buf = _dw_read_constant(dbg, 1, 0);
    	more = buf & 0x80;
	buf &= 0x7f;
	result |= buf << shift;
	shift += 7;
    } while ( more && shift < 32 );
    return result;
}

/* 
 * _dw_leb128_to_long - Convert LEB128 number into signed long.
 * Read an unknown number of bytes from the Dwarf file, build up
 * a signed long (in host byte order) and return it.
 */
Dwarf_Signed
_dw_leb128_to_long(dbg)
    Dwarf_Debug dbg;
{
    Dwarf_Unsigned buf;
    Dwarf_Signed result = 0;
    int more = 1;
    int shift = 0;

    do 
    {
    	buf = _dw_read_constant(dbg, 1, 0);
    	more = buf & 0x80;
    	buf &= 0x7f;
    	result |= buf << shift;
    	shift += 7;
    } while ( more && shift < 32 );
    if ( shift < 32 && buf & 0x40 ) 
    {
	/* sign extend */
    	result |= (-1 << shift);
    }
    return result;
}

/* 
 * _dw_swap_bytes -- swap within a short or a word, if the byte order of
 * the bits in the file is different from the byte order of the host.
 */
void
_dw_swap_bytes(buf, siz, section_byte_order)
    unsigned char *buf;
    Dwarf_Unsigned siz;
    Dwarf_Bool section_byte_order;
{
    Dwarf_Unsigned	tmp;
 
    RESET_ERR();
    if ( host_byte_order == -1 )
    {
	/* First time _dw_swap_bytes has been called */
	short test_byte_order = 0x1234;
	host_byte_order = (*((char *) &test_byte_order) == 0x12);  
    }
	
    if ( section_byte_order != host_byte_order ) 
    {
	switch ( siz ) 
	{
	case 1:
	    break;
	case 2:
	    tmp = *buf;
	    *buf = *(buf + 1);
	    *(buf + 1) = tmp;
	    break;
	case 4:
	    tmp = *buf;
	    *buf = *(buf + 3);
	    *(buf + 3) = tmp;
	    tmp = *(buf + 1);
	    *(buf + 1) = *(buf + 2);
	    *(buf + 2) = tmp;
	    break;
	default:
	    if(LIBDWARF_ERR(DLE_BAD_SWAP) == DLS_ERROR)
		return;
	    else
		break;
	}
    }
}

/* 
 * _dw_read_constant -- read a fixed-size constant from the Dwarf file.
 * Size is expected to be 1, 2, or 4 (only).  For 8-byte constants,
 * use _dw_read_large_constant.  For variable-length data, use 
 * _dw_leb128_to_ulong and _dw_leb128_to_long.
 *
 * Returns the value as an unsigned long.  The caller must cast the
 * return value if it is expected to be signed.
 */
Dwarf_Unsigned
_dw_read_constant(dbg, siz, section_byte_order)
    Dwarf_Debug dbg;
    Dwarf_Signed siz;  /* How many bytes to read */
    Dwarf_Bool section_byte_order; /* is info being read big-endian? */
{
    Dwarf_Small tmp[4]; /* Force alignment on word boundary */
 
    RESET_ERR();
    _dw_fread(tmp, siz, 1, dbg->stream);
    _dw_swap_bytes(tmp, siz, section_byte_order);
    switch(siz) 
    {
    case 1:
	return *((unsigned char *) tmp);
    case 2:
	return *((unsigned short *) tmp);
    case 4:
	return *((unsigned long *) tmp);
    default:
	if(LIBDWARF_ERR(DLE_UNSUPPORTED_FORM) == DLS_ERROR)
	    return(DLV_NOCOUNT); 
	break;
    }
}

/* 
 * _dw_read_large_constant -- read a fixed-size constant from the Dwarf file.
 * Size is expected to be 8 (only).  For variable-length data, use 
 * _dw_leb128_to_ulong and _dw_leb128_to_long.
 *
 * Returns the value as a pointer to a struct Dwarf_Bignum.
 */
Dwarf_Bignum
_dw_read_large_constant(dbg, siz, section_byte_order)
    Dwarf_Debug dbg;
    Dwarf_Signed siz;  /* How many bytes to read */
    Dwarf_Bool section_byte_order; /* is info being read big-endian? */
{
    Dwarf_Bignum result;
    if (siz != 8 && LIBDWARF_ERR(DLE_UNSUPPORTED_FORM) == DLS_ERROR)
	return(NULL);
    result = (Dwarf_Bignum) _dw_malloc(dbg, sizeof(struct Dwarf_Bignum));
    result->words = (Dwarf_Unsigned *) _dw_malloc(dbg, 8);
    result->numwords = 2;
    _dw_fread((Dwarf_Small *) &result->words[0], 4, 1, dbg->stream);
    _dw_swap_bytes((unsigned char *) &result->words[0], 4, section_byte_order);
    _dw_fread((Dwarf_Small *) &result->words[1], 4, 1, dbg->stream);
    _dw_swap_bytes((unsigned char *) &result->words[1], 4, section_byte_order);
    return result;
}

/* 
 * _dw_build_a_string -- read bytes from the Dwarf file until a 0 is read.
 * Copy the bytes into malloc'ed memory.  This is a little tricky because
 * we don't know in advance how long the string is, and we don't want to
 * read from the file twice.
 */
char *
_dw_build_a_string(dbg)
    Dwarf_Debug dbg;
{
    static int bigbufsiz = 1024;
    static char *bigbuf = NULL;
    char *c, *result;
    int count = 0;

    if (! bigbuf)
	bigbuf = _dw_malloc (NULL, bigbufsiz);

    for ( c = bigbuf, count = 0; 1; )
    {
	for ( ; count < bigbufsiz; ++c, ++count )
	{
	    *c = getc(dbg->stream);
	    if ( *c == 0 )
		break;
	}
	
	if ( count == bigbufsiz )
	{
	    /* The string was too long to fit into the temp buffer.
	       Reallocate the temp buffer and go on as if nothing
	       had happened. */
	    bigbuf = _dw_realloc(NULL, bigbuf, bigbufsiz * 2);
	    c = bigbuf + bigbufsiz;
	    bigbufsiz *= 2;
	}
	else
	    break;
    }

    result = _dw_malloc(dbg, count + 1);
    strcpy(result, bigbuf);
    return result;
}

