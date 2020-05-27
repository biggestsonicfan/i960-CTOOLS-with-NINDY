/*******************************************************************************
 * 
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
 ******************************************************************************/

#include <stdio.h>
#include "cc_info.h"
#include <string.h>
#include <stdlib.h>
#include "assert.h"
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "decode.h"
#include "encode.h"

#include "i_toolib.h"

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <stdlib.h>

#if defined(DOS)
#include <io.h>
#include <process.h>
#if !defined(WIN95)
#include <stat.h>
#endif
#else
#include <sys/file.h>
extern char* mktemp();
#endif
#include <fcntl.h>

#ifndef R_OK
#define R_OK 4
#endif
#ifndef W_OK
#define W_OK 2
#endif
#ifndef X_OK
#define X_OK 1
#endif

#if defined(DOS)
#define GNUTMP_DFLT     "./"
#else
#define GNUTMP_DFLT     "/tmp"
#endif

extern int db_rw_buf_siz;
extern unsigned char* db_rw_buf;

				/* Compression disabled for Beta */
int db_do_src_compression = 1;
int db_compress_prof_rec = 0;

#define	HOST_DECODE
#include "decode.c"

#define NEW_ALLOC
#define SLIDE(type, ptr)	*(*(type**)&(ptr))++
#define SHORT_SWAP(x)	((x << 8) | (x >> 8))
#define LONG_SWAP(x)\
((x << 24) | (((x >> 8) & 0xff) << 16) | (((x >> 16) & 0xff) << 8) | (x >> 24))

static Encode_table* etable;
static Alphabet* alphabet;

static int 
is_big_endian_host()
{
	short i = 0x0100;
	Uchar* p = (Uchar*) &i;

	return *p;
}
	
static Uchar*
malloc_and_clr(len)
unsigned len;
{
	Uchar* buf = (Uchar*) malloc(len);
	assert(buf);
	return (Uchar*) memset(buf, 0, len);
}

unsigned
decompression_table_size(e)
Encode_table* e;
{
  return DECODE_TABLE_SIZE + e->short_alpha * sizeof(Ushort) + e->char_alpha;
}

void
decompression_table_pack(sec, etable, is_wrong_endian)
Uchar* sec;
Encode_table* etable;
int is_wrong_endian;
{
	int i;
	Ushort short_alpha = etable->short_alpha;
	Ushort char_alpha = etable->char_alpha;
	Ushort char_rows= etable->char_rows;
	if (is_wrong_endian)
	{
		SLIDE(Ushort, sec) = SHORT_SWAP(char_rows);
		SLIDE(Ushort, sec) = SHORT_SWAP(short_alpha);
	}
	else
	{
		SLIDE(Ushort, sec) = char_rows;
		SLIDE(Ushort, sec) = short_alpha;
	}
	for (i = 0; i < ROWS; i++)
	{
		Ushort num = etable->suffix_number[i];
		if (is_wrong_endian)
			SLIDE(Ushort, sec) = SHORT_SWAP(num);
		else
			SLIDE(Ushort, sec) = num;
	}
	for (i = 0; i < ROWS; i++)
		*sec++ = etable->suffix_length[i];

	for (i = 0; i < short_alpha; i++)
	{
		Ushort sym = etable->short_symbols[i];
		if (is_wrong_endian)
			SLIDE(Ushort, sec) = SHORT_SWAP(sym);
		else
			SLIDE(Ushort, sec) = sym;
	}
	for (i = 0; i < char_alpha; i++)
		*sec++ = etable->char_symbols[i];
}

static int 
cmpf_char(p1, p2)
Uchar* p1;
Uchar* p2;
{
        unsigned count1 = etable->char_codes[*p1];
        unsigned count2 = etable->char_codes[*p2];
	if (count1 < count2)
		return 1;
	if (count1 > count2)
		return -1;
	if (*p1 < *p2)
		return 1;
	return -1;
}

static int
cmpf_alpha(p1, p2)
Ushort* p1; 
Ushort* p2;
{
        unsigned count1 = alphabet->counts[*p1];
        unsigned count2 = alphabet->counts[*p2];
	if (count1 < count2)
		return 1;
	if (count1 > count2)
		return -1;
	if (*p1 < *p2)
		return 1;
	return -1;
}

void
compression_alphabet_free()
{
	free(alphabet->symbols);
	free(alphabet->counts);
	free(alphabet);
}

void
compression_table_free(etable)
Encode_table* etable;
{
	free(etable->char_codes);
	free(etable->short_codes);
	free(etable->char_symbols);
	free(etable->short_symbols);
	free(etable);
}

static void
add_symbol_to_alphabet(sym)
Ushort sym;
{
	Ushort alpha = alphabet->alpha;
	Ushort max_alpha = alphabet->max_alpha;
	unsigned i;

	if (alpha == max_alpha)
	{
		Ushort* temp;
		if (max_alpha)
			max_alpha <<= 1;
		else
			max_alpha = MIN_SHORT;
		temp = (Ushort*) malloc_and_clr(sizeof(Ushort) * max_alpha);
		for (i = 0; i < alpha; i++)
		{
			temp[i] = alphabet->symbols[i];
		}
		if (alpha)
			free(alphabet->symbols);
		alphabet->symbols = temp;
		alphabet->max_alpha = max_alpha;
	}
	alphabet->symbols[alphabet->alpha++] = sym;
}

static Ushort
flat_section_alphabet(sec, sec_sz, is_wrong_endian)
Uchar* sec;
int sec_sz;
int is_wrong_endian;
{
	Ushort* head = (Ushort*) sec;
	Ushort* stop = (Ushort*) (sec + sec_sz);
	Ushort* func;

	alphabet = (Alphabet*) malloc_and_clr(sizeof(Alphabet));
	alphabet->counts = (unsigned*)malloc_and_clr(sizeof(unsigned) * ALPHA);

	func = head;
	while (func < stop)
	{
		Ushort sym = *func++;
		if (is_wrong_endian)
			sym = SHORT_SWAP(sym);
		if (alphabet->counts[sym] == 0)
			add_symbol_to_alphabet(sym);
		alphabet->counts[sym]++;
	}
	qsort(alphabet->symbols, alphabet->alpha, sizeof(Ushort), cmpf_alpha);
	return alphabet->alpha;
}

static Ushort
framed_section_alphabet(sec, sec_sz, is_wrong_endian)
Uchar* sec;
int sec_sz;
int is_wrong_endian;
{
	Uchar* head = sec;
	Uchar* stop = sec + sec_sz;
	Uchar* psym;
	unsigned sz;

	alphabet = (Alphabet*) malloc_and_clr(sizeof(Alphabet));
	alphabet->counts = (unsigned*)malloc_and_clr(sizeof(unsigned) * ALPHA);

	for (sz = SLIDE(int, head); head < stop; sz = SLIDE(int, head))
	{
		if (is_wrong_endian)
			sz = LONG_SWAP(sz);
		psym = head;				/* Skip header slot */
		head = psym + sz;			/* The next header */
		while (psym < head)
		{
			Ushort sym = SLIDE(Ushort, psym);
			if (is_wrong_endian)
				sym = SHORT_SWAP(sym);
			if (alphabet->counts[sym] == 0)
				add_symbol_to_alphabet(sym);
			alphabet->counts[sym]++;
		}
	}
	qsort(alphabet->symbols, alphabet->alpha, sizeof(Ushort), cmpf_alpha);
	return alphabet->alpha;
}

static unsigned
min_power_of_two(a)
unsigned a;
{
	unsigned i;
	if (a <= MIN_SHORT)
		return 0;
	a >>= 1;
	for (i = 1; i < a; i <<= 1)
		;
	return i;
}

static unsigned
cave_encoding_size(etable)
Encode_table* etable;
{
	return etable->encoded_bits / 8 + decompression_table_size(etable);
}

static void
cave_section_encode(alpha)
Ushort alpha;
{
	unsigned i, row, char_rows, short_count = 0;
	unsigned number = 0; 

	etable = (Encode_table*) malloc_and_clr(sizeof(Encode_table));
	etable->encoded_bits = 0;
	etable->short_alpha = alpha;
	etable->short_symbols = (Ushort*) malloc_and_clr(alpha * sizeof(Ushort));
	etable->short_codes = (unsigned*)malloc_and_clr(sizeof(unsigned) * ALPHA);
	for (i = 0; i < alpha; i++)
	{
		Ushort short_sym = alphabet->symbols[i];
		etable->short_symbols[i] = short_sym;
		etable->short_codes[short_sym] = alphabet->counts[short_sym];
		short_count += etable->short_codes[short_sym];
	}
	if (alpha < alphabet->alpha)
	{
		unsigned char_alpha = 0; 
		unsigned char_count = 0;
		unsigned counts_per_row;
		etable->char_codes = 
			(unsigned*)malloc_and_clr(sizeof(unsigned) * MIN_CHAR);
		etable->char_symbols = 
			(Uchar*)malloc_and_clr(sizeof(Uchar) * MIN_CHAR);
		for (i = alpha; i < alphabet->alpha; i++)
		{
			Ushort short_sym = alphabet->symbols[i];
			Uchar* sym = (Uchar*) &short_sym;
			unsigned count = alphabet->counts[short_sym];

			char_count += count * 2;
			etable->short_codes[short_sym] = -1;
			if (etable->char_codes[sym[0]] == 0)
				etable->char_symbols[char_alpha++] = sym[0];
			etable->char_codes[sym[0]] += count;
			if (etable->char_codes[sym[1]] == 0)
				etable->char_symbols[char_alpha++] = sym[1];
			etable->char_codes[sym[1]] += count;
		}
		counts_per_row = (char_count + short_count) / ROWS + 1;
		char_rows = char_count / counts_per_row;
		char_rows = char_rows < 1 ? 1 : 
				char_rows > ROWS - 1 ? ROWS - 1 : char_rows;
		etable->char_rows = char_rows;
		qsort(etable->char_symbols, char_alpha, sizeof(Uchar), 
			cmpf_char);
		etable->char_alpha = char_alpha;
		for (row = 0, i = 0; row < char_rows; row++)
		{
			unsigned r, n, len = 0, count = 0;
			unsigned cpr = char_count / (char_rows - row);
			for (n = 0; i < char_alpha && 
				(count <= cpr || (n & (n + 1))); n++)
			{
				Uchar char_sym = etable->char_symbols[i++];
				count += etable->char_codes[char_sym];
				len = (n == 1 << len) ? len + 1 : len;
			}
			char_count -= count;
			etable->suffix_length[row] = len;
			for (r = row; r; r--)
			{
				if (etable->suffix_length[r] < 
					etable->suffix_length[r - 1])
				{
					len = etable->suffix_length[r - 1];
					etable->suffix_length[r - 1] = 
						etable->suffix_length[r];
					etable->suffix_length[r] = len;
				}
				else
					break;
			}
		}
		for (row = 0, i = 0; row < char_rows; row++)
		{
			unsigned n = 0, count = 0; 
			unsigned len = etable->suffix_length[row];
			while (i < char_alpha && n < 1 << len)
			{
				Uchar char_sym = etable->char_symbols[i++];
				count += etable->char_codes[char_sym];
				etable->char_codes[char_sym] = 
					(n++ << PREFIX_LENGTH) | row;
			}
			etable->encoded_bits += count * (PREFIX_LENGTH + len);
			number += n;
			etable->suffix_number[row] = number;
		}
	}
	char_rows = etable->char_rows;
	for (row = char_rows, i = 0; row < ROWS; row++)
	{
		unsigned r, n, len = 0, count = 0;
		unsigned cpr = short_count / (ROWS - row);
		for (n = 0; i < alpha && (count <= cpr || (n & (n + 1))); n++)
		{
			Ushort short_sym = etable->short_symbols[i++];
			count += etable->short_codes[short_sym];
			len = (n == 1 << len) ? len + 1 : len;
		}
		short_count -= count;
		etable->suffix_length[row] = len;
		for (r = row; r > char_rows; r--)
		{
			if (etable->suffix_length[r] < 
				etable->suffix_length[r - 1])
			{
				len = etable->suffix_length[r - 1];
				etable->suffix_length[r - 1] = 
					etable->suffix_length[r];
				etable->suffix_length[r] = len;
			}
			else
				break;
		}
	}
	for (row = char_rows, i = 0; row < ROWS; row++)
	{
		unsigned n = 0, count = 0, len = etable->suffix_length[row];
		while (i < alpha && n < 1 << len)
		{
			Ushort short_sym = etable->short_symbols[i++];
			count += etable->short_codes[short_sym];
			etable->short_codes[short_sym] = 
				(n++ << PREFIX_LENGTH) | row;
		}
		etable->encoded_bits += count * (PREFIX_LENGTH + len);
		number += n;
		etable->suffix_number[row] = number;
	}
}


Encode_table*
select_compression_encoding(sec, sec_sz, is_framed, is_wrong_endian, option_v)
Uchar* sec;
int sec_sz;
int is_framed;
int is_wrong_endian;
int option_v;
{
	Ushort alpha;
	if (is_framed)
		alpha = framed_section_alphabet(sec, sec_sz, is_wrong_endian);
	else
		alpha = flat_section_alphabet(sec, sec_sz, is_wrong_endian);

	cave_section_encode(alpha);
	if (option_v)
		printf("Original %d, code %d, table %d, total %d\n", 
		       sec_sz * 8, 
		       etable->encoded_bits, 
		       decompression_table_size(etable) * 8, 
		       etable->encoded_bits +
		       decompression_table_size(etable) * 8);
	for (alpha = min_power_of_two(alpha); alpha >= MIN_SHORT; alpha >>= 1)
	{
		Encode_table* etab = etable;	/* Save the previous table */
		cave_section_encode(alpha);
		if (option_v)
			printf("Original %d, code %d, table %d, total %d, short %d, char %d, rows %d\n", 
			       sec_sz * 8, 
			       etable->encoded_bits, 
			       decompression_table_size(etable) * 8,
			       etable->encoded_bits +
			       decompression_table_size(etable) * 8,
			       etable->short_alpha, 
			       etable->char_alpha, 
			       etable->char_rows);
		if (cave_encoding_size(etable) < cave_encoding_size(etab))
		{
			compression_table_free(etab);
		}
		else
		{
			compression_table_free(etable);
			etable = etab;		/* Restore the previous table */
		}
	}
	return etable;
}


static unsigned
pack_word(etable, code, word, len, rem, out_first, is_wrong_endian)
Encode_table* etable; 
unsigned code; 
unsigned word; 
int* len; 
int* rem; 
unsigned** out_first;
int is_wrong_endian;
{
	unsigned prefix = code & MASK(PREFIX_LENGTH);
	unsigned suffix = code >> PREFIX_LENGTH;
	*len = etable->suffix_length[prefix];
	code = (prefix << *len) | suffix;
	*len += PREFIX_LENGTH;
	if (*rem < *len)
	{
		if (*rem)
		{
			word <<= *rem;
			*len -= *rem;
			word |= (code >> *len);
			code &= MASK(*len);
		}
		if (is_wrong_endian)
			*((*out_first)++) = LONG_SWAP(word);
		else
			*((*out_first)++) = word;
		*rem = 32;
		word = 0;
	}
	word <<= *len;
	word |= code;
	*rem -= *len;
	return word;
}

int
compress_buffer(etable, out, in, in_sz, is_wrong_endian)
Encode_table* etable; 
Uchar* out;
Uchar* in; 
int in_sz; 
int is_wrong_endian;
{
	int len;
	int rem = 32;
	unsigned word = 0;
	Ushort* in_first = (Ushort*) in;
	Ushort* in_stop = (Ushort*)(in + in_sz);
	unsigned* out_first = (unsigned*) out;
	unsigned* out_stop = (unsigned*) (out + in_sz);

	if (!in_sz)
		return 0;
	while (in_first < in_stop)
	{
		unsigned code;
		Ushort short_sym = *in_first++;
		if (is_wrong_endian)
			short_sym = SHORT_SWAP(short_sym);
		code = etable->short_codes[short_sym];
		if (code == -1)
		{
			Uchar* sym = (Uchar*) &short_sym;
			Uchar sym0 = sym[0], sym1 = sym[1];
			
			if (is_wrong_endian)
			{
				sym0 = sym[1], sym1 = sym[0];
			}
			code = etable->char_codes[sym0];
			word = pack_word(etable, code, word, &len, &rem, 
				&out_first, is_wrong_endian);
			code = etable->char_codes[sym1];
			if (out_first == out_stop)
				return 0;
			word = pack_word(etable, code, word, &len, &rem, 
				&out_first, is_wrong_endian);
		}
		else
			word = pack_word(etable, code, word, &len, &rem, 
				&out_first, is_wrong_endian);
		if (out_first == out_stop)
			return 0;
	}
	word <<= rem;
	if (is_wrong_endian)
		*out_first++ = LONG_SWAP(word);
	else
		*out_first++ = word;
	return (out_first - (unsigned*)out) * 4;
}

static void
unpack_table(p, table, sz)
char* p;
Decode_table* table;
int sz;
{
  int i;
  char* t;

  memset (table, 0, sz);

  CI_U16_FM_BUF (p, table->char_rows);    p += 2;
  CI_U16_FM_BUF (p, table->short_alpha);  p += 2;

  for (i = 0; i < ROWS; i++)
  { unsigned short num;
    CI_U16_FM_BUF (p, num);  p += 2;
    table->suffix_number[i] = num;
  }

  for (i = 0; i < ROWS; i++)
    table->suffix_length[i] = *p++;

  /* This is sick.  The alphabet tables are packed onto the
     end of the table;  first comes the short alphabet then
     the char alphabet.  Alex should fix this */

  t = (char*) &table->suffix_length[ROWS];
  for (i = 0; i < table->short_alpha; i++)
  { unsigned short alpha;
    CI_U16_FM_BUF (p, alpha); p += 2;
    SLIDE(Ushort, t) = alpha;
  }

  /* Have to calculate size of char alphabet from overall size */
  i = sz - (t - (char*) table);
  assert (i >= 0);

  /* Copy in the char alphabet */
  if (i)
    memcpy (t, p, i);
}

int
src_compress (ptext)
char **ptext;
{
  Encode_table* etable;

  if (db_do_src_compression)
  {
    int new_size, old_size, tbl_size;
    char *new_text, *old_text;
  
    old_size = db_get32 (*ptext);
    assert (old_size >= 4);
  
    if (old_size & 3)
    { /* Compression currently requires that input be multiple of 4 bytes */

      int extra = ((old_size + 4) & ~3) - old_size;

      assert (extra > 0 && extra < 4);

      *ptext = db_realloc (*ptext, old_size+extra);

      /* Clear the extra bytes for consistency in the database, even tho
         the extra bytes will be ignored after decompression. */

      memset (*ptext+old_size, 0, extra);
      old_size += extra;
    }
  
    old_text = *ptext;
  
    etable = select_compression_encoding(old_text, old_size, 0, 
	is_big_endian_host(), 0);
  
    tbl_size = decompression_table_size (etable);
    assert (tbl_size);
  
    new_text = (char*) malloc_and_clr (12 + old_size + tbl_size);
    new_size = compress_buffer(etable, new_text+12, old_text,old_size,
	is_big_endian_host());
  
    /* Don't use compressed version if it would grow */
    if (new_size == 0 || (new_size + tbl_size >= old_size))
    { memcpy (new_text+12, old_text, old_size);
      new_size = old_size;
      tbl_size = 0;
    }
    else
    { assert (new_size < old_size);
      decompression_table_pack(12 + new_text + new_size, etable, 
	is_big_endian_host());
    }
  
    free (old_text);
    compression_table_free(etable);
    compression_alphabet_free();
  
    new_size += 12 + tbl_size;
    *ptext = new_text;
  
    CI_U32_TO_BUF (new_text, new_size);  new_text += 4;
    CI_U32_TO_BUF (new_text, old_size);  new_text += 4;
    CI_U32_TO_BUF (new_text, tbl_size);  new_text += 4;
  }
  
  return db_get32 (*ptext);
}

int
src_uncompress (ptext)
unsigned char **ptext;
{
  if (db_do_src_compression)
  {
    /* Uncompress the source record at *ptext to a new
       area, and free the old space */
  
    unsigned char *in = *ptext;
    unsigned char *out;

    int in_sz, out_sz, dt_sz;
  
    /* Read total current size, uncompressed size, and table size */
    CI_U32_FM_BUF (in, in_sz);  in += 4;
    CI_U32_FM_BUF (in, out_sz); in += 4;
    CI_U32_FM_BUF (in, dt_sz);  in += 4;
  
    /* Get offset of table */
    in_sz -= (12 + dt_sz);
  
    out = (unsigned char*) malloc_and_clr (out_sz);
  
    /* If we have a decompression table, the text was compressed */
    if (dt_sz)
    { Decode_table* dtable = (Decode_table*) malloc_and_clr (dt_sz);
      unpack_table (in + in_sz, dtable, dt_sz);
      _decompress_buffer (out, in, out_sz, dtable);
    }
    else
    { assert (out_sz == in_sz);
      memcpy (out, in, out_sz);
    }
  
    free (*ptext);
    *ptext = out;
  }

  /* This may return a value slightly (1..3) less than out_sz */
  return db_get32 (*ptext);
}

static int (*db_compress[CI_NUM_DB][CI_MAX_REC_TYP+1][CI_NUM_LISTS])();
static int (*db_uncompress[CI_NUM_DB][CI_MAX_REC_TYP+1][CI_NUM_LISTS])();

static void
db_init_compress()
{
  static int did_init = 0;

  /* Initialize the table which selects automatic compression/decompression
     at write/read time.

    The compression/decompression calls for records read/written by cc1.960
    are hard coded in cc1.960.  This table only controls what happens
    when db_list_codec is being used.  */

  if (!did_init)
  {
    if (db_compress_prof_rec)
    {
      db_uncompress[CI_PASS1_DB][CI_PROF_REC_TYP][CI_PROF_FNAME_LIST] = src_uncompress;
      db_uncompress[CI_PASS2_DB][CI_PROF_REC_TYP][CI_PROF_FNAME_LIST] = src_uncompress;
    }
    did_init = 1;
  }
}

void
db_list_codec (p, l, method)
st_node* p;
int l;
int (*method)();
{
  if (method)
  {
    int new_size, old_size, delta;
  
    assert (p->db_list[l]->next == 0);
  
    old_size = db_get32 (p->db_list[l]->text);
    new_size = method (&(p->db_list[l]->text));
  
    delta = new_size - old_size;
  
    p->db_list_size += delta;
    p->db_rec_size += delta;
    p->db_list[l]->size += delta;
  }
}

void
dbp_init_lists (this, p, f)
dbase* this;
st_node* p;
DB_FILE* f;
{
  /* Set up the records describing the lists which come at the
     end of this object.  We maintain this info so we can weave
     lists as we merge symbols;  finally, when we write out the
     database, we write out all of the info on the merged lists.
  */

  int i,fmt,skip;

  skip = this->rec_skip_fmt ? this->rec_skip_fmt(p) : 0;

  { int n = CI_REC_FIXED_SIZE(p->rec_typ);
    p->db_rec = (unsigned char *) db_malloc (n);
    dbf_read (f, p->db_rec, n);
  }

  p->db_list_size = 0;

  for ((i=0),(fmt=CI_REC_LIST_FMT(p->rec_typ)); i < CI_NUM_LISTS; i++)
    if (fmt & (1 << i))
    { /* This symbol has a list of type 'i'.  Set up the descriptor for list
         type 'i' for this symbol to allow weaving when symbols are merged. */

      unsigned char buf[4];
      int n;

      dbf_read (f, buf, 4);
      CI_U32_FM_BUF (buf, n);		/* Read length of list ... */
      assert (n >= 4);

      p->db_list[i] = (db_list_node *) db_malloc (sizeof (db_list_node));
      memset (p->db_list[i], 0, sizeof (db_list_node));

      p->db_list[i]->last = p->db_list[i];

      p->db_list[i]->size = n-4;
      p->db_list_size += n;

      if (skip & (1 << i))
      { p->db_list[i]->text = (unsigned char *) f;
        p->db_list[i]->tell = dbf_tell(f)-4;

        dbf_seek_cur (f, n-4);
      }
      else
      { 
        unsigned char *t = (unsigned char*) db_malloc (n);

        memcpy (t, buf, 4);

        if (n > 4)
          dbf_read (f, t+4, n-4);

        p->db_list[i]->text = t;

        assert (this->kind);
        db_list_codec (p, i, db_uncompress[this->kind][p->rec_typ][i]);
      }
    }
    else
      p->db_list[i] = 0;
}

void
dbp_read_ccinfo(this, input)
dbase *this;
DB_FILE* input;
{
  unsigned char hbuf[CI_HEAD_REC_SIZE], *strtab, *symtab, *stab_ptr;
  int i, num_stabs, data_tell;
  st_node *st_nodes;
  ci_head_rec head;

  db_init_compress();

  this->in = (named_fd*) input;

  dbf_read (input, hbuf, CI_HEAD_REC_SIZE);
  db_exam_head (hbuf, dbf_name(input), &head);
  this->time_stamp = head.time_stamp;

  this->kind = head.kind_ver;

  num_stabs = head.sym_size/CI_STAB_REC_SIZE;

  /* allocate the string table and symbol table nodes. */
  st_nodes = (st_node *)malloc_and_clr(num_stabs * sizeof(st_node));
  memset (st_nodes, 0, num_stabs * sizeof(st_node));
  strtab = (unsigned char *)db_malloc(head.str_size);
  symtab = (unsigned char *)db_malloc(num_stabs * CI_STAB_REC_SIZE);

  /* read in the data base records for this file.  We seek past the
     data section for now, because we don't want to read in all of
     it.  So, we record where it is, skip past it, and read the
     rest of the records.  */

  data_tell = dbf_tell(input);

  dbf_seek_set(input,head.db_size+data_tell);
  dbf_read(input,symtab,head.sym_size);
  dbf_read(input,strtab,head.str_size);

  stab_ptr = symtab;
  for (i = 0; i < num_stabs; i++)
  {
    int val;

    st_node *db_p = &st_nodes[i];

    CI_U32_FM_BUF(stab_ptr + CI_STAB_STROFF_OFF, val);
    db_p->name   = (char *)strtab + val;

    CI_U32_FM_BUF(stab_ptr + CI_STAB_DBOFF_OFF, val);
    db_p->db_rec_offset = val;

    CI_U32_FM_BUF(stab_ptr + CI_STAB_DBRSZ_OFF, db_p->db_rec_size);
    CI_U16_FM_BUF(stab_ptr + CI_STAB_HASH_OFF, val);
    CI_U8_FM_BUF (stab_ptr + CI_STAB_STATIC_OFF, db_p->is_static);
    CI_U8_FM_BUF (stab_ptr + CI_STAB_RECTYP_OFF, db_p->rec_typ);
    
    db_p->name_length = strlen(db_p->name);

    db_p->next = this->db_stab[val];
    this->db_stab[val] = db_p;

    dbf_seek_set (input, db_p->db_rec_offset + data_tell);

    if (db_p->db_rec_size > db_rw_buf_siz)
      db_rw_buf_siz = db_p->db_rec_size;

    dbp_init_lists (this, db_p, input);

    stab_ptr += CI_STAB_REC_SIZE;
  }

  /* Call application-specific scanner */
  if (this->post_read)
    this->post_read (this, input, st_nodes, num_stabs);

  /* don't need the symtab space any more, get rid of it */
  free (symtab);
}

void
dbp_pre_write (p)
dbase* p;
{
  /* Calculate size of the database.  This routine is called from the linker
     so that we can decide how big the cc_info section needs to be in elf.

     We also call it again from dbp_write_ccinfo just for a sanity check
     to make sure we have not changed the size of the database since we
     calculated the size. */

  long sym_sz = 0;
  long str_sz = 0;
  long db_sz  = 0;
  long tot_sz = 0;

  int i;

  db_init_compress();

  if (db_rw_buf == 0)
    db_rw_buf = (unsigned char*) db_malloc (db_rw_buf_siz + 1);

  if (p->pre_write)
  { assert (p->tot_sz == 0); 
    p->pre_write (p);
    p->pre_write = 0;
  }

  /* compute the section sizes. */
  for (i = 0; i < CI_HASH_SZ; i++)
  { st_node * list_p;
    for (list_p = p->db_stab[i]; list_p != 0; list_p = list_p->next)
    {
      if (list_p->db_rec_size != 0)
      { int auto_del = p->pre_write_del ? p->pre_write_del(list_p) : 0;
        int j;

        if (auto_del == -1)
          list_p->db_rec_size = 0;
        else
        { int lo = CI_REC_LIST_LO(list_p->rec_typ);
          int hi = CI_REC_LIST_HI(list_p->rec_typ);
          for (j=lo; j < hi; j++)
          { if (auto_del & (1 << j))
            { list_p->db_rec_size -= list_p->db_list[j]->size;
              list_p->db_list_size -= list_p->db_list[j]->size;
              list_p->db_list[j]->size = 0;
            }

            assert (p->kind);
            db_list_codec (list_p, j, db_compress[p->kind][list_p->rec_typ][j]);
          }

          assert (list_p->db_rec_size >= (hi-lo) * 4);
        }
      }

      if (list_p->db_rec_size != 0)
      { /* Allocate space in the file for the record, less deleted parts. */
        list_p->db_rec_offset = db_sz;
        db_sz  += list_p->db_rec_size;
        sym_sz += CI_STAB_REC_SIZE;
        str_sz += list_p->name_length+1;
      }
    }
  }

  tot_sz = db_sz + sym_sz + str_sz + CI_HEAD_REC_SIZE;

  if (p->tot_sz)
  { assert (p->sym_sz==sym_sz && p->str_sz==str_sz && p->db_sz==db_sz);
    assert (p->tot_sz==tot_sz);
  }
  else
    assert (p->sym_sz==0 && p->str_sz==0 && p->db_sz==0);

  p->sym_sz = sym_sz;
  p->str_sz = str_sz;
  p->db_sz  = db_sz;
  p->tot_sz = tot_sz;
}
