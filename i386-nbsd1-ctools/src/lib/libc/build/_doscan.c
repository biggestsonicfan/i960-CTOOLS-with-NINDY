/*******************************************************************************
 * 
 * Copyright (c) 1993 Intel Corporation
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

/* _doscan - format data into memory (used by scanf et al)
 * Copyright (c) 1985,86,87,88 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>

#define _IIC_BIN 0		/* indices for integer conversion tables */
#define _IIC_OCT 1
#define _IIC_DEC 2
#define _IIC_HEX 3

int _Lfltscan(FILE *, unsigned, unsigned, va_list *, unsigned,
              int (*fp)(), int (*ufp)());

#if (__PIC)
#pragma optimize(read_char="notce") /* Needed for PIX960 */
#endif

static int
read_char(int *num_read, FILE *str, int (*fp)())
{
    ++*num_read;
    return (*fp)(str);
}

static void
unread_char(int c, int *num_read, FILE *stream)
{
    --*num_read;
    if (c != EOF) {
        if (&stream->_sem)
            _semaphore_wait(&stream->_sem);
        stream->_flag &= (~_IOEOF);
        stream->_cnt++;
        --stream->_ptr;		/* NOTE:  the character is not written
        					  back to the buffer */
        if (&stream->_sem)
            _semaphore_signal(&stream->_sem);
    }
}

static int
_str_scan(int c, const char *b)
{
    while (*b != 0) {
        if ((char)c == *b)
            return (1);
        ++b;
    }
    return (0);
}

int _Ldoscan(const char *format, va_list *args, FILE *stream, int (*fp)())
{
    static const char int_digits[4][23] = {
        { "01\0" },
        { "01234567\0" },
        { "0123456789\0" },
        { "0123456789abcdefABCDEF\0" },
    };

    int c, save_c;     /*save_c handles where you have ambiguity in hex input*/
    int dgt_index;
    long el;
    unsigned long eul;
    union {
        char *p;
        unsigned pp[2];
    } ptr;
    int num_conv = 0;			/* number of conversions made */
    int num_asgn = 0;			/* number of assignments done */
    int un_signed = 0;			/* if zero, use strtol;  otherwise
					     use strtoul.  This is a "hack"
					     to fix the problem with scanning
					     large unsigned integers
					     (those greater than 2**31) */
    unsigned shortflg, longflg, width;
    unsigned suppress, sign, base, invert;
    char buffer[350], *bptr;
    const char *dptr;
    unsigned longptrflg = 1;
    int num_read;

    num_read = 0;

    c = read_char(&num_read, stream, fp);
    if (c == EOF) goto wig_out;
    unread_char(c, &num_read, stream);

    while (*format) {
        shortflg = longflg = width = suppress = sign = 0;
        while (*format != '%') {
            if (*format == '\0') goto wig_out;
            if (isspace(*format)) {
                do {
                    c = read_char(&num_read, stream, fp);
                } while (isspace(c));
                unread_char(c, &num_read, stream);
                while (isspace(*format))
                    ++format;
            }
            else if (*format++ != (c = read_char(&num_read, stream, fp))) {
                unread_char(c, &num_read, stream);
                goto wig_out;
            }
        }

        ++format;
        if (*format == '*') {
            suppress = 1;
            ++format;
        }
        while (isdigit(*format))
            width = width * 10 + (*format++ - '0');
        if (*format == 'h') {
            shortflg = 1;
            longptrflg = 0;
            ++format;
        }
        else if (*format == 'L') {
            longptrflg = longflg = 2;	/* long double */
            ++format;
        }
        else if (*format == 'l') {
            longptrflg = longflg = 1;	/* regular double */
            ++format;
        }

        if (*format != '[' && *format != 'c' && *format != 'n' && 
            *format != '%') {
            do {
                c = read_char(&num_read, stream, fp);	/* dump whitespace */
            } while (isspace(c));
            if (c == EOF) goto wig_out;
        }

        switch (*format) {
            case 'B':
                longflg = 1;

            case 'b':
                base = 2;
                dgt_index = _IIC_BIN;
                goto integral;

            case 'D':
                longflg = 1;

            case 'd':
                base = 10;
                dgt_index = _IIC_DEC;
                goto integral;

            case 'i':
                base = 0;
                dgt_index = _IIC_DEC;
                goto integral;

            case 'O':
                longflg = 1;

            case 'o':
                base = 8;
                dgt_index = _IIC_OCT;
                un_signed = 1;
                goto integral;

            case 'U':
                longflg = 1;

            case 'u':
                base = 10;
                dgt_index = _IIC_DEC;
                un_signed = 1;
                goto integral;

            case 'X':
            case 'x':
                base = 16;
                dgt_index = _IIC_HEX;
                un_signed = 1;
                if (c == '0') {
						save_c = c;
						bptr = buffer;
						*bptr = c;
						if (toupper(c = read_char(&num_read, stream, fp)) == 'X') {
						  *bptr++ = c;
						  c = read_char(&num_read, stream, fp);
						}
						else {
						  unread_char(c, &num_read, stream);
						  c = save_c;
						}
					 }
                /* Note fall through... */

integral:
                dptr = int_digits[dgt_index];
                if (!width)
                    width = sizeof(buffer) - 1;
                bptr = buffer;
                if (width && (c == '+' || c == '-'))
                {
                    *bptr = c;
                    bptr++;
                    width--;
                    c = read_char(&num_read, stream, fp);
                }
                if (base == 0) {
                    if (c == '0') {
                        *bptr = c;
                        bptr++;
                        width--;
                        if (toupper(c = read_char(&num_read, stream, fp)) == 'X') {
                            dptr = int_digits[_IIC_HEX];
                            *bptr++ = c;
                            width--;
                            c = read_char(&num_read, stream, fp);
                        }
                        else
                            dptr = int_digits[_IIC_OCT];
                    }
                    else
                        dptr = int_digits[_IIC_DEC];
                }

                for (; width && _str_scan(c, dptr); --width) {
                    *bptr++ = c;
                    c = read_char(&num_read, stream, fp);
                }

                /* At this point, we either read the wrong type of character,
                 * or we've read too many of the right kind.  IN EITHER CASE,
                 * we should put it back where it belongs
                 */
                unread_char(c, &num_read, stream); /* no "if (width)" */
                *bptr = 0;
                if (bptr == buffer || !_str_scan(bptr[-1], dptr))
                    goto wig_out;	/* nothing converted */
                if (un_signed)
                    eul = strtoul(buffer, NULL, base);
                else
                    el = strtol(buffer, NULL, base);
                num_conv++;
int_assign:
                if (!suppress) {
                    if (longflg) {
                        long *lptr = va_arg(*args, long *);
                        if(un_signed)
                            *lptr = eul;
                        else
                            *lptr = el;
                    } else if (shortflg) {
                        short *sptr = va_arg(*args, short *);
                        if(un_signed)
                            *sptr = (unsigned short)eul;
                        else
                            *sptr = (short)el;
                    } else {
                        int *iptr = va_arg(*args, int *);
                        if(un_signed)
                            *iptr = (unsigned int)eul;
                        else
                            *iptr = (int)el;
                    }
                    num_asgn++;
                }
                break;

            case 's':
                if (!suppress)
                    bptr = va_arg(*args, char *);
                if (!width)
                    width = 0x7FFF;
                if (c == EOF)
                    goto wig_out;	/* don't bump num_conv if EOF in white space */
                while (width && !isspace(c) && c != EOF) {
                    if (!suppress) *bptr++ = c;
                    --width;
                    c = read_char(&num_read, stream, fp);
                }
                if (!suppress) {
                    *bptr = 0;
                    num_asgn++;
                }
                num_conv++;
                if (c == EOF) goto wig_out;
                unread_char(c, &num_read, stream);
                break;

            case 'c':
                if (!suppress)
                    bptr = va_arg(*args,char *);
                if (!width)
                    width = 1;
                c = read_char(&num_read, stream, fp);
                if (c == EOF) goto wig_out;
                while (width && c != EOF) {
                    if (!suppress) *bptr++ = c;
                    c = read_char(&num_read, stream, fp);
                    --width;
                }
                unread_char(c, &num_read, stream);
                if (!suppress)
                    num_asgn++;
                num_conv++;
                break;

            case 'E':
            case 'G':
            case 'e':
            case 'f':
            case 'g':
                unread_char(c, &num_read, stream);
                if (!width)
                    width = sizeof(buffer) - 1;
                if (!(c = _Lfltscan(stream, width, longflg, args, suppress,
                                    fp, ungetc)))
                    goto wig_out;
                num_read += c;
                if (!suppress)
                    num_asgn++;
                num_conv++;
                break;

            case 'p':
                if (longptrflg) {
                    if (!width)
                        width = sizeof(buffer) - 1;
                    bptr = buffer;
                    while (width && isxdigit(c)) {
                        *bptr++ = c;
                        --width;
                        c = read_char(&num_read, stream, fp);
                    }
                    *bptr = 0;
                    if (!buffer[0])
                        goto wig_out;
                    unread_char(c, &num_read, stream);
                    if (!buffer[0]) goto wig_out;
                    ptr.pp[0] = strtoul(buffer, NULL, 16);
                    if (!suppress) {
                        void **fptr = va_arg(*args, void **);

                        *fptr = ptr.p;
                        num_asgn++;
                    }
                }
                else {
                    unread_char(c, &num_read, stream);
                    width = 2*sizeof(int);
                    bptr = buffer;
                    *bptr++ = '0';
                    while (width && isxdigit(c = read_char(&num_read, stream, fp))) {
                        *bptr++ = c;
                        width--;
                    }
                    if (!buffer[0]) goto wig_out;
                    *bptr = 0;
                    if (!suppress) {
                        void **nptr = va_arg(*args, void **);

                        *nptr = (void *)strtoul(buffer, NULL, 16);
                        num_asgn++;
                    }
                }
                num_conv++;
                break;

            case 'n':
                el = num_read;
                goto int_assign;

            case '[':
                c = read_char(&num_read, stream, fp);
                if (c == EOF) goto wig_out;
                unread_char(c, &num_read, stream);
                bptr = buffer;
                if (*(++format) == '^') {
                    invert = 1;
                    *bptr++ = EOF;	/* add EOF to the list of terminators */
                    ++format;
                }
                else
                    invert = 0;
                if (*format == ']' && *(++format) != '\0')
                    *bptr++ = ']';
                while (*format!=']')
                    *bptr++ = *format++;
                *bptr = 0;
                if (!width)
                    width = 0x7FFF;
                if (!suppress)
                    bptr = va_arg(*args, char *);
                if (invert != _str_scan((c = read_char(&num_read, stream, fp)), buffer))
                    unread_char(c, &num_read, stream);
                else
                    goto wig_out;	/* no match character found */
                while (width && invert != _str_scan((c = read_char(&num_read, stream, fp)), buffer)) {
                    if (!suppress)
                        *bptr++ = c;
                    --width;
                }
                unread_char(c, &num_read, stream);
                *bptr = 0;
                if (!suppress)
                    num_asgn++;
                num_conv++;
                break;

            case '%':
                if (c != '%') goto wig_out;
                read_char(&num_read, stream, fp);
                c = read_char(&num_read, stream, fp);
                unread_char(c, &num_read, stream);
                break;

            default:
                goto wig_out;
        }
        ++format;
    }					/* end while */

wig_out:
    if (num_conv)
        return (num_asgn);
    return (c == EOF ? EOF : 0);
}
