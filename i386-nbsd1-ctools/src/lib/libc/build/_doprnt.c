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

/* _doprnt - Format data under control of a format string.
 * Copyright (c) 1985,86,87,88 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define printchar(c, char_count, str, fp) (++*char_count, (*fp)(c, str)) 

static unsigned char const dig[] = {"0123456789abcdef"};

int _Lfltprnt(int, va_list *, char, int, unsigned, int,
             unsigned, unsigned, FILE *, int (*fp)(), unsigned);


static void
_inttohex(unsigned i, char *buf)
{
#define MASK 0xF0000000
#define SHFT 28

    unsigned mask = MASK;
    int shft = SHFT;

    while (mask) {
        *buf++ = dig[(i & mask) >> shft];
        mask >>= 4;
        shft -= 4;
    }
}

static void
printpad(int width, int padchar, int *char_count, FILE *str, int (*fp)())
{
    while (width--)
        printchar(padchar, char_count, str, fp);
}

static int
printstring(char *cp, int *char_count, FILE *str, int (*fp)())
{
    int i;

    for (i = 0; *cp; i++, cp++)
        printchar(*cp, char_count, str, fp);
     return i;
}

static void
prntstr(char *cptr, int width, int *char_count, FILE *str, int (*fp)())
{
    while (width--)
    {
        printchar(*cptr, char_count, str, fp);
        ++cptr;
    }
}

int _Ldoprnt(const char *format, va_list *args, FILE *stream, int (*fp)())
{
    int base, temp;
    int exponent, neg_flag;
    unsigned leftadj, padchar, precflg;
    unsigned longflg, shortflg, length, alternate;
    int width, precisn;
    char *cp, prefix[5], _buf[34], *tcp;
    int csign;
    long tlong;
    int tint;
    unsigned long tulong;
    unsigned int tuint;
    unsigned longptrflg = 1;
    unsigned hexupper;
    int char_count;


    char_count = 0;
    memset(prefix, 0, 5);
    memset(_buf, 0, 34);

    while (*format) {

        if (*format != '%') {           /* print literal one at a time */
            printchar(*format, &char_count, stream, fp);
            format++;                   /* normal characters */
        
        } else {                        /* conversion spec */

            hexupper = 0;
            alternate = 0;
            leftadj = 0; 
            length = 0;
            csign = 0;
            neg_flag = 0;
            padchar = ' ';
	    shortflg = 0;

            for (;;) {
                switch (*++format) {

                case '-': leftadj = '-';                /* left justify */
                          padchar = ' ';
                          break;
                case '+': csign = '+';                  /* print sign */
                          break;
                case ' ': if (!csign) csign = ' ';      /* prepend a space */
                          break;
                case '#': alternate = '#';              /* alt form */
                          break;
                case '0': if (!leftadj)                 /* alt pad character */
                              padchar = *format;
                          break;
                default : goto out;
                }
            }

out:        if (*format == '*') {
                width = va_arg(*args, int);        /* width is an argument */
                if (width < 0) {
                    leftadj = '-';
                    padchar = ' ';
                    width = -width;
                }
                ++format;

            } else {                            /* set the field width */
                for (width = 0; isdigit(*format); )
                    width = width * 10 + (*format++ - '0');
            }
 
            if (precflg = (*format=='.')) {     /* precision? */
                ++format;
                if (*format == '*') {
                    precisn = va_arg(*args, int);  /* precisn is an argument */
                    if (precisn < 0)
                        precflg = 0;
                    ++format;

                } else {
                    for (precisn = 0; isdigit(*format); )
                        precisn = precisn * 10 + (*format++ - '0');
                }

            } else {
                precisn = 0;
            }

            if (*format == 'l') {               /* long */
                longflg = longptrflg = 1;
                ++format;

            } else if (*format == 'L') {        /* extra long */
                longflg = 2;
                longptrflg = 1;
                ++format;

            } else if (*format == 'h') {
                ++format;
		if ( (csign == '+') || (csign == '-') )
			csign = 0;		/* don't print sign for hex */
                longptrflg = longflg = 0;
		shortflg = 1;

            } else {
                longflg = 0;
            }

            cp = _buf;
            tcp = _buf + 33;
            switch (*format) {

            case 'e':
            case 'E':
            case 'f':
            case 'g':
            case 'G':
                if (!precflg)
                    precisn = 6;                /* default */

                char_count += _Lfltprnt(precisn, args, *format, width, alternate,
                                  csign, leftadj, padchar, stream, fp, longflg);
                break;

            case 'B':
                longflg = 1;

            case 'b':
                base = 2;
                csign = 0;
                goto nosign;

            case 'O':
                longflg = 1;
            case 'o':
                if (alternate) {
                    prefix[0] = '0';
                    prefix[1] = '\0';
                    length = 1;
                }
		csign = 0;
                if (longflg) {                  /* convert the number to long */
                    tulong = va_arg(*args, long);   /* get the argument */
                    do {
                        *--tcp = dig[tulong & 7L];
                    } while (tulong >>= 3);

                } else {
                    tuint = va_arg(*args, int); /* get the argument */
                    do {
                        *--tcp = dig[tuint & 7];
                    } while (tuint >>= 3);
                }
                goto print_it;

            case 'U':
                longflg = 1;

            case 'u':
                base = 10;
		csign = 0;
                goto nosign;

            case 'X':
                hexupper = 1;

            case 'x':
                if (alternate) {
                    prefix[0] = '0';
                    prefix[1] = *format;
                    prefix[2] = '\0';
                    length = 2;
                }

                if (longflg) {          /* convert the number to string */
                    tulong = va_arg(*args, long); /* get the argument */
                    do {
                        *--tcp = dig[tulong & 15L];
                    } while (tulong >>= 4);

                } else {
                    tuint = va_arg(*args, int); /* get the argument */
                    do {
                        *--tcp = dig[tuint & 15L];
                    } while (tuint >>= 4);
                }
		csign = 0;
                goto print_it;

            case 'D':
                longflg = 1;

            case 'd':
            case 'i':
                neg_flag = 1;
                base = 10;
                goto nosign;

            case 'p':
                if (longptrflg) {
                    if (width > 8 && leftadj) {
                        printpad(width - 8, ' ', &char_count, stream, fp);
                        width = 0;
                    }

                    _inttohex(va_arg(*args, int), &_buf[0]);
                    prntstr(&_buf[0], 8, &char_count, stream, fp);

                    if (width > 8)
                        printpad(width - 8, ' ', &char_count, stream, fp);
                    break;

                } else {
                    if (width > 4 && leftadj) {
                        printpad(width - 4, ' ', &char_count, stream, fp);
                        width = 0;
                    }

                    _inttohex(va_arg(*args, int), &_buf[0]);
                    prntstr(&_buf[0], 4, &char_count, stream, fp);

                    if (width > 4)
                         printpad(width - 4, ' ', &char_count, stream, fp);
                    break;
                }

nosign:         if (longflg) {
                    tlong = va_arg(*args, long); /* get the argument */
                    if (neg_flag && ((long)tlong) < 0) {
                        csign = '-'; /* negative number */
                        tlong = -(tlong);
                    }

                    do {
                        *--tcp = dig[(unsigned long)tlong % base];
                    } while (tlong = (unsigned long)tlong / base);

                } else {
                    tint = va_arg(*args, int); /* get the argument */
                    if (neg_flag && ((int)tint) < 0) {
                        csign = '-'; /* negative number */
                        tint = -(tint);
                    }
                    do {
                        *--tcp = dig[(unsigned int)tint % base];
                    } while (tint =  (unsigned int)tint / base);
                }
                
print_it:       length += (unsigned)strlen(tcp);
                
                /*
                 * Two special cases:
		 *	(1)  if precisn is zero and the value being formatted
		 *	     is zero, then no '0' character is output
		 *	(2)  a pad character of '0' is ignored under d, i, o,
		 *	     u, and x conversion if a precison was specified
                 */
                if(precflg)  {
                    if ((*tcp == '0') && (precisn == 0))
                        break;

			padchar = ' ';
		}
                
                if (precflg && length < precisn)
                    precisn -= length;
                else
                    precisn = 0;

                if (width > length + precisn) {
                    width -= length + precisn;
                    if (csign && (width > 0))
                        width--;
                } else {
                    width = 0;
                }

                if (!leftadj && padchar == ' ') {       /* right justify with no padding */
                    printpad(width, padchar,
                             &char_count, stream, fp);  /* first print spaces ... */
                    width = 0;                          /* right justify */
                }

                if (alternate) {                        /* alternate form */
                    cp = prefix;
                    printstring(cp, &char_count,
                                stream, fp);            /* print prefix */
                    length -= (unsigned)strlen(prefix);
                }

                if (csign)  {                           /* output the sign */
                    printchar(csign, &char_count, stream, fp);
                }

                if (!leftadj && padchar == '0') {       /* right justify with padding */
                    printpad(width, padchar,
                             &char_count, stream, fp);  /* ... then print padding */
                    width = 0;                          /* right justify */
                }

                if (precisn)
                    printpad(precisn, '0',
                             &char_count, stream, fp);  /* put zeros */

                cp = tcp;
                if (hexupper) {
                    strupr(cp);
                    prntstr(cp, length, &char_count, stream, fp);
                } else {
                    prntstr(cp, length, &char_count, stream, fp);
                }

                if (width)
                    printpad(width, ' ',
                             &char_count, stream, fp);  /* left justify */
                break;

            case 's':
                cp = va_arg(*args, char *);
                if (!width && !precflg) {
                    cp += printstring(cp, &char_count, stream, fp);
                } else {
                    length = (unsigned)strlen(cp);
                    if (precflg) {
                        if (length > precisn) length = precisn;
                    }

                    if (width) {
                        if (width > length) {
                            width -= length;
                            if (!leftadj) {
                                printpad(width, ' ',
                                         &char_count, stream, fp);
                                width = 0;
                            }
                        } else {
                            width = 0;
                        }
                    }
                    prntstr(cp, length, &char_count, stream, fp);
                    cp += length;
                    if (width)
                        printpad(width, ' ', &char_count, stream, fp);
                }
                break;

            case 'c':
                if (!leftadj && width)
                    printpad(width - 1, padchar, &char_count, stream, fp);

                temp = va_arg(*args, int);
                printchar(temp, &char_count, stream, fp);

                if (leftadj && width)
                    printpad(width - 1, padchar, &char_count, stream, fp);
                break;

            case 'n':
                if (longflg)
                    *(va_arg(*args, long *)) = (long)char_count;
                else if (shortflg)
                    *(va_arg(*args, short *)) = (short)char_count;
                else
                    *(va_arg(*args, int *)) = char_count;

                break;

            default:
                if (!leftadj && width) /* right justify */
                    printpad(width - 1, padchar, &char_count, stream, fp);

                printchar(*format, &char_count, stream, fp);

                if (leftadj && width) /* left justify */
                    printpad(width - 1, padchar, &char_count, stream, fp);

                break;

            }                           /* end switch */
            ++format;
        }                               /* end else */
    }                                   /* end while */

    return char_count;
}
