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

/* _Lfltprnt - floating point support for _Ldoprnt()
 * Copyright (c) 1987 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "afpcnvt.h"

typedef union {
  unsigned long lval[2];
  double d;
} dnan;

typedef union {
  unsigned long lval[3];
  long double t;
} tnan;

/* Is X a NAN or an Infinity. */
#define D_NAN_INF_P(X)  ((((dnan *)&(X))->lval[1] & 0x7FF00000) == 0x7FF00000)
#define T_NAN_INF_P(X)  ((((tnan *)&(X))->lval[2] & 0x7FFF) == 0x7FFF)

/* These two are only meaningful after having checked for exponent. */
#define D_INF_P(X)      ((((dnan *)&(X))->lval[0] == 0) && \
                         ((((dnan *)&(X))->lval[1] & 0xFFFFF) == 0))
#define T_INF_P(X)      ((((tnan *)&(X))->lval[0] == 0) && \
                         ((((tnan *)&(X))->lval[1] & 0x7FFFFFFF) == 0))

/* Is X negative. */
#define D_NEG_P(X)	(((((dnan *)&(X))->lval[1]) & 0x80000000) != 0)
#define T_NEG_P(X)      (((((tnan *)&(X))->lval[2]) & 0x8000) != 0)

/* Is X Quiet NAN */
#define D_QNAN_P(X)     (((((dnan *)&(X))->lval[1]) & 0x80000) != 0)
#define T_QNAN_P(X)     (((((tnan *)&(X))->lval[1]) & 0x40000000) != 0)

/* this macros return first 32 bits of fraction field */
#define D_NAN_F(X)      (((((dnan *)&(X))->lval[1] & 0xFFFFF) << 12) | \
                         (((dnan *)&(X))->lval[0] >> 20))
#define T_NAN_F(X)      (((((tnan *)&(X))->lval[1] & 0x7FFFFFFF) << 1) | \
                         (((tnan *)&(X))->lval[0] >> 31))

static int prnt_INF(int sign_flg,FILE *str, int (*fp)());
static int prnt_NAN(int QNaN_flg,int val,FILE *str, int (*fp)());

#define printchar(c, char_count, str, fp) (++*char_count, (*fp)(c, str)) 


static void
printpad(int width, int padchar, int *char_count, FILE *str, int (*fp)())
{
    while (width--)
        printchar(padchar, char_count, str, fp);
}


static int
printstring(char *cp, int *char_count, FILE *str, int (*fp)())
{
    register int i;

    for (i = 0; *cp; i++, cp++)
        printchar(*cp, char_count, str, fp);
    return i;
}


static void
prntstr(char *cptr, int width, int *char_count, FILE *str, int (*fp)())
{
int len;

    for (len = 0; cptr[len] != '\0'; len++);
     
    while (width && len)
    {
        width--;
        len--;
        printchar(*cptr, char_count, str, fp);
        ++cptr;
    }
    if (width > 0)
    {
       while(width--)
           printchar('0', char_count, str, fp);
    }
}


int _Lfltprnt( int        f_precision,   /* field precision value */
              va_list *  ap,            /* arg list */
              char       format,        /* control string */
              int        f_width,       /* field width */
              unsigned   alternate,     /* alternate form, yes or no */
              int        csign,         /* sign to be printed */
              unsigned   leftadj,       /* left/right justified in the field */
              unsigned   padchar,       /* pad character */
              FILE *     stream,        /* output stream */
              int        (*fp)(),       /* pointer to putc or _putch function */
              unsigned   longflg        /* double or long double */
            )


{
    int             log_10;             /* floor(log10(value))              */
    int             sig_rod_cnt;        /* Significant right-of-decimal cnt */
    int             dec_pt_ofs;         /* Char count offset for decimal pt */
    int             g_flag, e_flag;     /* Flags for g/e format             */
    int             padlen;             /* Count for pending pad chars      */
    int		    zero_pad;		/* zero pad after sig digs.         */
    unsigned        length;
    int             temp;
    char            bcd_buf[30];        /* Working area for value cnvsn     */
    char *          cp;                 /* Char pointer for string copying  */
    int             char_count;         /* Total number of chars issued     */
    long double     ldval;              /* Temp for incoming long double    */
    double          dval;               /* Temp for incoming double         */
    int             sig_digs;           /* significant digits of precision  */
    int             round_index;        /* Temp for rounding cnvted value   */
    int             g_bump;             /* Log10 rounding pre-correction    */
    int             e_length;           /* Number of digits in exponent     */

    char_count = 0;

    if  (longflg == 2)  {
        ldval = va_arg(*ap,long double);
        if  (T_NAN_INF_P(ldval))  {
            csign = T_NEG_P(ldval);
            if  (T_INF_P(ldval))
                char_count = prnt_INF(csign,stream,fp);
            else
                char_count = prnt_NAN(T_QNAN_P(ldval),T_NAN_F(ldval),stream,fp);
            return char_count;
        }  else  {
            log_10 = _AFP_tp2a(bcd_buf, ldval);
        }
    }  else  {
        dval = va_arg(*ap,double);
        if  (D_NAN_INF_P(dval))  {
            csign = D_NEG_P(dval);
            if  (D_INF_P(dval))
                char_count = prnt_INF(csign,stream,fp);
            else
                char_count = prnt_NAN(D_QNAN_P(dval),D_NAN_F(dval),stream,fp);
            return char_count;
        }  else  {
            log_10 = _AFP_dp2a(bcd_buf, dval);
        }
    }

    bcd_buf[2] = bcd_buf[1];            /* Overwrite decimal point for ease */


    /*  Leading sign if negative or if forced  - stm */

    if  (bcd_buf[0] == '-'  ||  csign == '+')  {
        csign = bcd_buf[0];
    }


    /* Set flags and adjust initial right-of-decimal digit count */

    e_flag = 0;
    g_flag = 0;

    sig_rod_cnt = f_precision;

    switch  (format)  {
        case 'f':
        case 'F':

                /* Change f format to e format when the value is too big */

                if  (log_10 >= AFP_MANT_DIGS)  {
                    format       = 'e';
                    e_flag       = 1;
                    sig_rod_cnt  = AFP_MANT_DIGS-1;
                }

                break;

        case 'e':
        case 'E':
                e_flag      = 1;
                sig_rod_cnt = f_precision;
                break;

        case 'g':
        case 'G':
                g_flag = 1;

                /* Force at least one significant digit  */

                if  (f_precision <= 0)  {
                    f_precision = 1;
                }

                g_bump = 0;

                if  (     f_precision < AFP_MANT_DIGS
                      &&  bcd_buf[2+f_precision] >= '5')  {
                    round_index = 0;
                    while  (     round_index < f_precision
                             &&  bcd_buf[2+round_index] == '9' )  {
                        round_index += 1;
                    }
                    if  (round_index == f_precision)  {
                        g_bump = 1;
                    }
                }

                /*  Check for number value forcing E format  */

                if  (    log_10+g_bump < -4
                      || log_10+g_bump >= f_precision
                      || log_10        >= AFP_MANT_DIGS )  {
                    e_flag = 1;
                }

                /*  Compute the number of right-of-decimal digits  */

                if  (e_flag)  {
                    sig_rod_cnt = f_precision - 1;
                }  else  {
                    sig_rod_cnt = f_precision - log_10 - 1;
                }
                break;
    }



    /*  Limit significant digits to maximum allowed or user specified  */

    zero_pad = 0;
    if  (e_flag)  {
        if  (sig_rod_cnt >= AFP_MANT_DIGS)  {
	    zero_pad    = sig_rod_cnt - (AFP_MANT_DIGS-1);
            sig_rod_cnt = AFP_MANT_DIGS-1;
        }
        sig_digs = sig_rod_cnt + 1;
 
    }  else  {
        if  ((log_10+sig_rod_cnt) >= AFP_MANT_DIGS)  {
	    zero_pad    = (log_10+sig_rod_cnt) - (AFP_MANT_DIGS-1);
            sig_rod_cnt = (AFP_MANT_DIGS-1) - log_10;
        }
        dec_pt_ofs = log_10 + 1;
        sig_digs   = dec_pt_ofs + sig_rod_cnt;
    }


    /*
     *  Round to the specified number of significant digits (as req'd)
     */

    if  (sig_digs < AFP_MANT_DIGS  &&  bcd_buf[2+sig_digs] >= '5')  {

        round_index = sig_digs-1;

        while  (round_index >= 0  &&  bcd_buf[2+round_index] == '9')  {
            bcd_buf[2+round_index]  = '0';
            round_index            -= 1;
        }

        if  (round_index != -1)  {
            bcd_buf[2+round_index] += 1;

        }  else  {
            log_10     += 1;
            bcd_buf[2]  = '1';
        
            if  (!e_flag)  {
                /*
                 *  Adjust the location of the decimal point offset and,
                 *  if necessary, reduce the number of R.O.D. digits
                 */
                if  ((dec_pt_ofs+sig_rod_cnt) > AFP_MANT_DIGS  ||  g_flag)  {
                    sig_rod_cnt -= 1;
                }  else  {
                    bcd_buf[2+sig_digs] = '0';      /* Add the extra digit */
                }
                dec_pt_ofs += 1;
            }  else  {
                /*
                 *  Increment the exponent field (handle positive, negative,
                 *  and zero cases separately).
                 */
                round_index = 1+1+AFP_MANT_DIGS+2+AFP_EXP_DIGS-1;
                if         (log_10 > 0)  {
                    while  (bcd_buf[round_index] == '9')  {
                        bcd_buf[round_index]  = '0';
                        round_index          -= 1;
                    }
                    bcd_buf[round_index] += 1;
                }  else if (log_10 < 0)  {
                    while  (bcd_buf[round_index] == '0')  {
                        bcd_buf[round_index]  = '9';
                        round_index          -= 1;
                    }
                    bcd_buf[round_index] -= 1;
                }  else  {  /*  -1 -> +0  */
                    bcd_buf[1+1+AFP_MANT_DIGS+1               ] = '+';
                    bcd_buf[1+1+AFP_MANT_DIGS+2+AFP_EXP_DIGS-1] = '0';
                }
            }
        }
    }


    /*
     *  Change g format to f/e/E
     */

    if  (g_flag)  {
        if  (e_flag)  {
            if  (format == 'G')  {
                format = 'E';
            }  else  {
                format = 'e';
            }
        }  else  {
            format = 'f';   /* Use f format since e/E hasn't been forced */
        }
    }


    cp = &bcd_buf[2];

    if  (format == 'f')  {
        if  (g_flag && (!alternate))  {    /* remove trailing zeros */
            if  ((int)(length = (dec_pt_ofs-1) + sig_rod_cnt) > 0)  {
                while  ( *(cp + length) == '0'  &&  sig_rod_cnt != 0 )  {
                    sig_rod_cnt -= 1;
                    length      -= 1;
                }
            }
        }

        if  ((int)(length = dec_pt_ofs) < 0)  {
            length  = 0;                    /* integral - 1 */
        }

	if (zero_pad > 0)
		length += zero_pad;     /* need to get proper # of digs -stm */

       	length += sig_rod_cnt;		/* integral + fraction */

        if  (csign)  {
            length += 1;                    /* sign */
        }

        if  (sig_rod_cnt != 0 || alternate)  {
            length += 1;                    /* decimal point */
        }

        if  ((padlen = f_width-length) < 0)  {
            padlen =  0;                    /* no padding necessary */
        }

        if  ((dec_pt_ofs <= 0) && padlen)  {
            padlen -= 1;                    /*  Provide for leading zero */
        }

        if  (csign && padchar == '0')  {
            printchar(csign, &char_count, stream, fp);  /* sign before 0 pad */
        }

        if  (!leftadj && padlen)  {
            printpad(padlen, padchar, &char_count, stream, fp);
            padlen = 0;
        }

        if  (csign && padchar == ' ')  {
            printchar(csign, &char_count, stream, fp);  /* print sign spaces */
        }


        /*  Generate/copy left of decimal digits  */

        if  (dec_pt_ofs <= 0)  {
            printchar('0', &char_count, stream, fp);
        }  else  {
            prntstr(cp, dec_pt_ofs, &char_count, stream, fp);
            cp += dec_pt_ofs;
        }


        /*  Decimal and right-of-decimal digits  */

        if  (alternate || sig_rod_cnt)  {
            printchar('.', &char_count, stream, fp);
        }

        if  (sig_rod_cnt)  {
            if  (dec_pt_ofs < 0) {
                if  ((temp = -dec_pt_ofs) > sig_rod_cnt)  {
                    temp = sig_rod_cnt;
                }
                printpad(temp, '0', &char_count, stream, fp);
                sig_rod_cnt -= temp;
            }
            prntstr(cp, sig_rod_cnt, &char_count, stream, fp);
        }

	/* If requested precision was greater than the number of significant 
	   digits we can represent, print trailing zeros to pad. -stm */

	if (zero_pad > 0) {
	    printpad(zero_pad, '0', &char_count, stream, fp);
	}


        if  (padlen)  {
            printpad(padlen, padchar, &char_count, stream, fp);
        }

    }  else  {                          /* format is 'e' or 'E' */

        if          (abs(log_10) > 999)  {
            e_length = 4;                       /*  xE-xxxx  or  xE+xxxx  */
        }  else if  (abs(log_10) >  99)  {
            e_length = 3;                       /*  xE-xxx   or  xE+xxx   */
        }  else  {
            e_length = 2;                       /*  xE-xx    or  xE+xx    */
        }

        length = 3 + e_length + sig_rod_cnt;    /* plus right-of-decimal digs */


	if (zero_pad > 0)
		length += zero_pad;		/* stm */

        if  (csign)  {
            length += 1;                        /* ... and sign */
        }

        if  (g_flag && (!alternate))  {
            while  ( *(cp + sig_rod_cnt) == '0'  &&  sig_rod_cnt != 0 )  {
                sig_rod_cnt -= 1;
                length      -= 1;
            }
        }

        if  (sig_rod_cnt != 0  ||  alternate)  {
            length += 1;                        /* ... and decimal point */
        }
  
        if  ((padlen = f_width-length) < 0)  {
            padlen = 0;            /* no padding necessary */
        }

        if  (csign && padchar == '0')  {        /* print sign before padding? */
            printchar(csign, &char_count, stream, fp);
        }

        if  (!leftadj && padlen)  {
            printpad(padlen, padchar, &char_count, stream, fp);
            padlen = 0;
        }

        if  (csign && padchar == ' ')  {        /* print sign after spaces? */
            printchar(csign, &char_count, stream, fp);
        }

        /*
         *  Copy the mantissa digits (w/ decimal point as required)
         */

        printchar(*cp, &char_count, stream, fp);
        cp += 1;
        if  (alternate || sig_rod_cnt)  {
            printchar('.', &char_count, stream, fp);
        } 
        prntstr(cp, sig_rod_cnt, &char_count, stream, fp);

	/* If requested precision was greater than the number of significant 
	   digits we can represent, print trailing zeros to pad. -stm */

	if (zero_pad > 0) {
	    printpad(zero_pad, '0', &char_count, stream, fp);
	}

        /*
         *  Copy the exponent field:  e/E, sign, then exponent digits
         */

        printchar(format, &char_count, stream, fp);
        printchar(bcd_buf[1+1+AFP_MANT_DIGS+1], &char_count, stream, fp);
        printstring(&bcd_buf[2+AFP_MANT_DIGS+2+AFP_EXP_DIGS-e_length],
                    &char_count, stream, fp);

        if  (padlen)  {
            printpad(padlen, padchar, &char_count, stream, fp);
        }
    }

    return  char_count;
}



static int
prnt_INF(int sign_flg,FILE *str, int (*fp)())
{
    int count=0;
    char strng[]="+INF";

    if  (sign_flg)
        strng[0]='-';
    printstring(strng,&count,str,fp);
    return count;
}

static int
prnt_NAN(int QNaN_flg,int val,FILE *str, int (*fp)())
{
    int count=0;
    static const char strng[]="SNaN";
    char strng2[sizeof(strng)];
    int i;

    for (i = 0; (strng2[i] = strng[i]) != '\0'; i++);

    if (QNaN_flg)
        strng2[0]='Q';
    printstring(strng2,&count,str,fp);
    return count;
}
