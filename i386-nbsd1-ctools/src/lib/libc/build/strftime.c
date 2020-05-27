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

/* strftime - places the characters into the array controlled by
 *            format string, returns the number of characters placed
 * Copyright (c) 1987 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

struct strf {
    char *a[7];       /* abbreviated weekday name */
    char *A[7];       /* full weekday name */
    char *b[12];      /* abbreviated month name */
    char *B[12];      /* full month name */
    char *dtfrmt;     /* date and time format using strftime() directives */
    char *dfrmt;      /* date format using strftime() directives */
    char *tfrmt;      /* time format using strftime() directives */
    char *dmark;      /* date mark */
    char *tmark;      /* time mark */
    char *p[2];       /* AM PM */
};

#define USA       	0
#define GERMANY   	1
#define JAPAN     	2
#define SPAIN     	3
#define UK        	4
#define FRANCE    	5
#define ITALY      	6

static void
init_country(int cntry, struct strf *ct_p)
{
  int i;

  switch(cntry)
  {
    case USA:
      i = 0;
      ct_p->a[i++] = "Sun";
      ct_p->a[i++] = "Mon";
      ct_p->a[i++] = "Tue";
      ct_p->a[i++] = "Wed";
      ct_p->a[i++] = "Thu";
      ct_p->a[i++] = "Fri";
      ct_p->a[i++] = "Sat";

      i = 0;
      ct_p->A[i++] = "Sunday";
      ct_p->A[i++] = "Monday";
      ct_p->A[i++] = "Tuesday";
      ct_p->A[i++] = "Wednesday";
      ct_p->A[i++] = "Thursday";
      ct_p->A[i++] = "Friday";
      ct_p->A[i++] = "Saturday";

      i = 0;
      ct_p->b[i++] = "Jan";
      ct_p->b[i++] = "Feb";
      ct_p->b[i++] = "Mar";
      ct_p->b[i++] = "Apr";
      ct_p->b[i++] = "May";
      ct_p->b[i++] = "Jun";
      ct_p->b[i++] = "Jul";
      ct_p->b[i++] = "Aug";
      ct_p->b[i++] = "Sep";
      ct_p->b[i++] = "Oct";
      ct_p->b[i++] = "Nov";
      ct_p->b[i++] = "Dec";

      i = 0;
      ct_p->B[i++] = "January";
      ct_p->B[i++] = "February";
      ct_p->B[i++] = "March";
      ct_p->B[i++] = "April";
      ct_p->B[i++] = "May";
      ct_p->B[i++] = "June";
      ct_p->B[i++] = "July";
      ct_p->B[i++] = "August";
      ct_p->B[i++] = "September";
      ct_p->B[i++] = "October";
      ct_p->B[i++] = "November";
      ct_p->B[i++] = "December";

      ct_p->dtfrmt = "%a %b %d %H:%M:%S %Y";
      ct_p->dfrmt  = "%a %b %d, %Y";
      ct_p->tfrmt  = "%H:%M:%S";
      ct_p->dmark  = "-";
      ct_p->tmark  = ":";

      i = 0;
      ct_p->p[i++] = "AM";
      ct_p->p[i++] = "PM";
      break;

    case GERMANY:
      i = 0;
      ct_p->a[i++] = "Son";
      ct_p->a[i++] = "Mon";
      ct_p->a[i++] = "Dien";
      ct_p->a[i++] = "Mit";
      ct_p->a[i++] = "Don";
      ct_p->a[i++] = "Fre";
      ct_p->a[i++] = "Sam";

      i = 0;
      ct_p->A[i++] = "Sonntag";
      ct_p->A[i++] = "Montag";
      ct_p->A[i++] = "Dienstag";
      ct_p->A[i++] = "Mittwoch";
      ct_p->A[i++] = "Donnerztag";
      ct_p->A[i++] = "Freitag";
      ct_p->A[i++] = "Samstag";

      i = 0;
      ct_p->b[i++] = "Jan";
      ct_p->b[i++] = "Feb";
      ct_p->b[i++] = "Mae";
      ct_p->b[i++] = "Apr";
      ct_p->b[i++] = "Mai";
      ct_p->b[i++] = "Jun";
      ct_p->b[i++] = "Jul";
      ct_p->b[i++] = "Aug";
      ct_p->b[i++] = "Sep";
      ct_p->b[i++] = "Okt";
      ct_p->b[i++] = "Nov";
      ct_p->b[i++] = "Dez";

      i = 0;
      ct_p->B[i++] = "Januar";
      ct_p->B[i++] = "Februar";
      ct_p->B[i++] = "Maerz";
      ct_p->B[i++] = "April";
      ct_p->B[i++] = "Mai";
      ct_p->B[i++] = "Juni";
      ct_p->B[i++] = "Juli";
      ct_p->B[i++] = "August";
      ct_p->B[i++] = "September";
      ct_p->B[i++] = "Oktober";
      ct_p->B[i++] = "November";
      ct_p->B[i++] = "Dezember";

      ct_p->dtfrmt = "%d %b %y  %H:%M:%S";
      ct_p->dfrmt  = "%d %b %y";
      ct_p->tfrmt  = "%H:%M:%S";
      ct_p->dmark  = "-";
      ct_p->tmark  = ":";

      i = 0;
      ct_p->p[i++] = "AM";
      ct_p->p[i++] = "PM";
      break;

    case JAPAN:
      i = 0;
      ct_p->a[i++] = "Sun";
      ct_p->a[i++] = "Mon";
      ct_p->a[i++] = "Tue";
      ct_p->a[i++] = "Wed";
      ct_p->a[i++] = "Thu";
      ct_p->a[i++] = "Fri";
      ct_p->a[i++] = "Sat";

      i = 0;
      ct_p->A[i++] = "Sunday";
      ct_p->A[i++] = "Monday";
      ct_p->A[i++] = "Tuesday";
      ct_p->A[i++] = "Wednesday";
      ct_p->A[i++] = "Thursday";
      ct_p->A[i++] = "Friday";
      ct_p->A[i++] = "Saturday";

      i = 0;
      ct_p->b[i++] = "Jan";
      ct_p->b[i++] = "Feb";
      ct_p->b[i++] = "Mar";
      ct_p->b[i++] = "Apr";
      ct_p->b[i++] = "May";
      ct_p->b[i++] = "Jun";
      ct_p->b[i++] = "Jul";
      ct_p->b[i++] = "Aug";
      ct_p->b[i++] = "Sep";
      ct_p->b[i++] = "Oct";
      ct_p->b[i++] = "Nov";
      ct_p->b[i++] = "Dec";

      i = 0;
      ct_p->B[i++] = "January";
      ct_p->B[i++] = "February";
      ct_p->B[i++] = "March";
      ct_p->B[i++] = "April";
      ct_p->B[i++] = "May";
      ct_p->B[i++] = "June";
      ct_p->B[i++] = "July";
      ct_p->B[i++] = "August";
      ct_p->B[i++] = "September";
      ct_p->B[i++] = "October";
      ct_p->B[i++] = "November";
      ct_p->B[i++] = "December";

      ct_p->dtfrmt = "%y %b %d  %H:%M:%S";
      ct_p->dfrmt  = "%y %b %d";
      ct_p->tfrmt  = "%H:%M:%S";
      ct_p->dmark  = "-";
      ct_p->tmark  = ":";

      i = 0;
      ct_p->p[i++] = "AM";
      ct_p->p[i++] = "PM";
      break;

    case SPAIN:
      i = 0;
      ct_p->a[i++] = "Lun";
      ct_p->a[i++] = "Mar";
      ct_p->a[i++] = "Mie";
      ct_p->a[i++] = "Jue";
      ct_p->a[i++] = "Vie";
      ct_p->a[i++] = "Sab";
      ct_p->a[i++] = "Dim";

      i = 0;
      ct_p->A[i++] = "Lunes";
      ct_p->A[i++] = "Martes";
      ct_p->A[i++] = "Miercoles";
      ct_p->A[i++] = "Jueves";
      ct_p->A[i++] = "Viernes";
      ct_p->A[i++] = "Sabado";
      ct_p->A[i++] = "Dimingo";

      i = 0;
      ct_p->b[i++] = "Ene";
      ct_p->b[i++] = "Feb";
      ct_p->b[i++] = "Mar";
      ct_p->b[i++] = "Apr";
      ct_p->b[i++] = "May";
      ct_p->b[i++] = "Jun";
      ct_p->b[i++] = "Jul";
      ct_p->b[i++] = "Ago";
      ct_p->b[i++] = "Sep";
      ct_p->b[i++] = "Oct";
      ct_p->b[i++] = "Nov";
      ct_p->b[i++] = "Dec";

      i = 0;
      ct_p->B[i++] = "Enero";
      ct_p->B[i++] = "Febrero";
      ct_p->B[i++] = "Marzo";
      ct_p->B[i++] = "April";
      ct_p->B[i++] = "Mayo";
      ct_p->B[i++] = "Juno";
      ct_p->B[i++] = "Julio";
      ct_p->B[i++] = "Agosto";
      ct_p->B[i++] = "Septembre";
      ct_p->B[i++] = "Octobre";
      ct_p->B[i++] = "Noviembre";
      ct_p->B[i++] = "Deciembre";

      ct_p->dtfrmt = "%b %d, %y  %H:%M:%S";
      ct_p->dfrmt  = "%b %d, %y";
      ct_p->tfrmt  = "%H:%M:%S";
      ct_p->dmark  = "-";
      ct_p->tmark  = ":";

      i = 0;
      ct_p->p[i++] = "AM";
      ct_p->p[i++] = "PM";
      break;

    case UK:
      i = 0;
      ct_p->a[i++] = "Sun";
      ct_p->a[i++] = "Mon";
      ct_p->a[i++] = "Tue";
      ct_p->a[i++] = "Wed";
      ct_p->a[i++] = "Thu";
      ct_p->a[i++] = "Fri";
      ct_p->a[i++] = "Sat";

      i = 0;
      ct_p->A[i++] = "Sunday";
      ct_p->A[i++] = "Monday";
      ct_p->A[i++] = "Tuesday";
      ct_p->A[i++] = "Wednesday";
      ct_p->A[i++] = "Thursday";
      ct_p->A[i++] = "Friday";
      ct_p->A[i++] = "Saturday";

      i = 0;
      ct_p->b[i++] = "Jan";
      ct_p->b[i++] = "Feb";
      ct_p->b[i++] = "Mar";
      ct_p->b[i++] = "Apr";
      ct_p->b[i++] = "May";
      ct_p->b[i++] = "Jun";
      ct_p->b[i++] = "Jul";
      ct_p->b[i++] = "Aug";
      ct_p->b[i++] = "Sep";
      ct_p->b[i++] = "Oct";
      ct_p->b[i++] = "Nov";
      ct_p->b[i++] = "Dec";

      i = 0;
      ct_p->B[i++] = "January";
      ct_p->B[i++] = "February";
      ct_p->B[i++] = "March";
      ct_p->B[i++] = "April";
      ct_p->B[i++] = "May";
      ct_p->B[i++] = "June";
      ct_p->B[i++] = "July";
      ct_p->B[i++] = "August";
      ct_p->B[i++] = "September";
      ct_p->B[i++] = "October";
      ct_p->B[i++] = "November";
      ct_p->B[i++] = "December";

      ct_p->dtfrmt = "%d %b %y  %H:%M:%S";
      ct_p->dfrmt  = "%d %b %y";
      ct_p->tfrmt  = "%H:%M:%S";
      ct_p->dmark  = "-";
      ct_p->tmark  = ":";

      i = 0;
      ct_p->p[i++] = "AM";
      ct_p->p[i++] = "PM";
      break;

    case FRANCE:
      i = 0;
      ct_p->a[i++] = "Dim";
      ct_p->a[i++] = "Lun";
      ct_p->a[i++] = "Mar";
      ct_p->a[i++] = "Mer";
      ct_p->a[i++] = "Jeu";
      ct_p->a[i++] = "Ven";
      ct_p->a[i++] = "Sam";

      i = 0;
      ct_p->A[i++] = "Dimanche";
      ct_p->A[i++] = "Lundi";
      ct_p->A[i++] = "Mardi";
      ct_p->A[i++] = "Mercredi";
      ct_p->A[i++] = "Jeudi";
      ct_p->A[i++] = "Vendredi";
      ct_p->A[i++] = "Samedi";

      i = 0;
      ct_p->b[i++] = "Jan";
      ct_p->b[i++] = "Fev";
      ct_p->b[i++] = "Mar";
      ct_p->b[i++] = "Avr";
      ct_p->b[i++] = "Mai";
      ct_p->b[i++] = "Jui";
      ct_p->b[i++] = "Jui";
      ct_p->b[i++] = "Auo";
      ct_p->b[i++] = "Sep";
      ct_p->b[i++] = "Oct";
      ct_p->b[i++] = "Nov";
      ct_p->b[i++] = "Dec";

      i = 0;
      ct_p->B[i++] = "Janvier";
      ct_p->B[i++] = "Fevrier";
      ct_p->B[i++] = "Mars";
      ct_p->B[i++] = "Avril";
      ct_p->B[i++] = "Mai";
      ct_p->B[i++] = "Juin";
      ct_p->B[i++] = "Juillet";
      ct_p->B[i++] = "Auot";
      ct_p->B[i++] = "Septembre";
      ct_p->B[i++] = "Octobre";
      ct_p->B[i++] = "Novembre";
      ct_p->B[i++] = "Decembre";

      ct_p->dtfrmt = "%d %b %y  %H:%M:%S";
      ct_p->dfrmt  = "%d %b %y";
      ct_p->tfrmt  = "%H:%M:%S";
      ct_p->dmark  = "-";
      ct_p->tmark  = ":";

      i = 0;
      ct_p->p[i++] = "AM";
      ct_p->p[i++] = "PM";
      break;

    case ITALY:
      i = 0;
      ct_p->a[i++] = "Dom";
      ct_p->a[i++] = "Lun";
      ct_p->a[i++] = "Mar";
      ct_p->a[i++] = "Mer";
      ct_p->a[i++] = "Gio";
      ct_p->a[i++] = "Ven";
      ct_p->a[i++] = "Sab";

      i = 0;
      ct_p->A[i++] = "Domenica";
      ct_p->A[i++] = "Lunedi";
      ct_p->A[i++] = "Martedi";
      ct_p->A[i++] = "Mercoledi";
      ct_p->A[i++] = "Giovedi";
      ct_p->A[i++] = "Venerdi";
      ct_p->A[i++] = "Sabato";

      i = 0;
      ct_p->b[i++] = "Gen";
      ct_p->b[i++] = "Feb";
      ct_p->b[i++] = "Mar";
      ct_p->b[i++] = "Apr";
      ct_p->b[i++] = "Mag";
      ct_p->b[i++] = "Gui";
      ct_p->b[i++] = "Lug";
      ct_p->b[i++] = "Ago";
      ct_p->b[i++] = "Set";
      ct_p->b[i++] = "Ott";
      ct_p->b[i++] = "Nov";
      ct_p->b[i++] = "Dic";

      i = 0;
      ct_p->B[i++] = "Gennaio";
      ct_p->B[i++] = "Febbraio";
      ct_p->B[i++] = "Marzo";
      ct_p->B[i++] = "Aprile";
      ct_p->B[i++] = "Maggio";
      ct_p->B[i++] = "Guigno";
      ct_p->B[i++] = "Luglio";
      ct_p->B[i++] = "Agosto";
      ct_p->B[i++] = "Settembre";
      ct_p->B[i++] = "Ottobre";
      ct_p->B[i++] = "Novembre";
      ct_p->B[i++] = "Dicembre";

      ct_p->dtfrmt = "%d %b %y  %H:%M:%S";
      ct_p->dfrmt  = "%d %b %y";
      ct_p->tfrmt  = "%H:%M:%S";
      ct_p->dmark  = "-";
      ct_p->tmark  = ":";

      i = 0;
      ct_p->p[i++] = "AM";
      ct_p->p[i++] = "PM";
      break;
  }
}

struct copy {
    char *s1;
    char *s2;
    size_t max;
    size_t count;
};

static int cat(x)			/* copy s2 at the end of s1 until max is zero */
struct copy *x;
{
    (x->s1) += strlen(x->s1);
    while (*(x->s2)) {
        if (!(x->max)) {
            *(x->s1) = 0;
            return 0;			/* return, max = 0 */
        }
        *(x->s1)++ = *(x->s2)++;
        x->count++;
        (x->max)--;
    }
    *(x->s1) = 0;
    return 1;				/* return, max != 0 */
}

static int check_cat(a, b, pad, x)
int a, b;
char *pad;
struct copy *x;
{
    char buffer[7];
    int temp;

    temp = strlen(itoa(a, buffer, 10));
    if (temp < b) {
        for( ;temp < b; temp++) { 
            x->s2 = pad;
            if (!(cat(x)))
                return 0;
        }
    }
    x->s2 = buffer; 
    if (!(cat(x)))
        return 0;
    return 1;
}

size_t
strftime(char *s,
         size_t max,
         const char *format,
         const struct tm *tmptr)
{
    div_t quorem;
    int temp, day;
    char buf[25];
    struct copy x;
    int ok;
    struct strf country;

    init_country(USA, &country);

    x.count = 0;
    *s = 0;
    x.s1 = s;
    if (max == 0)
        return 0;
    else
        x.max = max - 1;		/* -1 for NULL */

    while (*format) {
        if (x.max == 0) return 0;
        if (*format != '%') {		/* ordinary char - copy it */
            *(x.s1)++ = *format;
            *(x.s1) = 0;
            x.count++;
            (x.max)--;
        }
        else {
            format++;
            switch (*format) {
                case 'a':
                    x.s2 = country.a[tmptr->tm_wday];	/* abb. weekday name */
                    ok = cat(&x);
                    break;
                case 'A':
                    x.s2 = country.A[tmptr->tm_wday];	/* full weekday name */
                    ok = cat(&x);
                    break;
                case 'b':
                    x.s2 = country.b[tmptr->tm_mon];		/* abb. month name */
                    ok = cat(&x);
                    break;
                case 'B':
                    x.s2 = country.B[tmptr->tm_mon];		/* full month name */
                    ok = cat(&x);
                    break;
                case 'c':
                    if (!strftime(buf, sizeof(buf), country.dtfrmt, tmptr))
                        return 0;					/* date & time */
                    x.s2 = buf;
                    ok = cat(&x);
                    break;
                case 'd':
                    ok = check_cat(tmptr->tm_mday, 2, "0", &x);		/* day of month 01-31 */
                    break;
                case 'H':
                    ok = check_cat(tmptr->tm_hour, 2, "0", &x);		/* hour 00-23 */
                    break;
                case 'I':
                    if ((temp = tmptr->tm_hour) > 12)
                        temp -= 12;					/* hour 01-12 */
                    if (temp == 0)
                        temp = 12;
                    ok = check_cat(temp, 2, "0", &x);
                    break;
                case 'j':
                    ok = check_cat(tmptr->tm_yday + 1, 3, "0", &x);	/* day of year 001-366 */
                    break;
                case 'm':
                    ok = check_cat(tmptr->tm_mon + 1, 2, "0", &x);	/* month 01-12 */
                    break;
                case 'M':
                    ok = check_cat(tmptr->tm_min, 2, "0", &x);		/* minute 00-59 */
                    break;
                case 'p':
                    x.s2 = country.p[tmptr->tm_hour >= 12];	/* AM or PM */
                    ok = cat(&x);
                    break;
                case 'S':
                    ok = check_cat(tmptr->tm_sec, 2, "0", &x);		/* seconds 00-59 */
                    break;
                case 'U':
                    quorem = div(tmptr->tm_yday + 1, 7);		/* week of year 00-52 */
                    temp = quorem.quot;					/* Sunday as first day of week */
                    if (quorem.rem > (tmptr->tm_wday + 1))
                       temp++;
                    ok = check_cat(temp, 2, "0", &x);
                    break;
                case 'w':
                    ok = check_cat(tmptr->tm_wday, 1, "0", &x);		/* weekday 0-6 */ 
                    break;
                case 'W':
                    quorem = div(tmptr->tm_yday + 1, 7);		/* week of year 00-52 */   
                    temp = quorem.quot;					/* Monday as first day of week */
                    day = tmptr->tm_wday - 1;				/* Monday = 0, ... */
                    if (day < 0) day = 6;				/* Sunday = 6 */
                    if (quorem.rem > (day + 1)) temp++;
                    ok = check_cat(temp, 2, "0", &x);
                    break;
                case 'x':
                    if (!strftime(buf, sizeof(buf), country.dfrmt, tmptr))
                        return 0;					/* date */
                    x.s2 = buf;
                    ok = cat(&x);
                    break;
                case 'X':
                    if (!strftime(buf, sizeof(buf), country.tfrmt, tmptr))
                        return 0;					/* time */
                    x.s2 = buf;
                    ok = cat(&x);
                    break;
                case 'y':
                    ok = check_cat(tmptr->tm_year, 2, "0", &x);		/* year 00-99 */
                    break;
                case 'Y':
                    ok = check_cat(tmptr->tm_year + 1900, 4, "0", &x);	/* year with century */
                    break;
                case 'Z':
                    tzset();
  	  	    if (&tzname)
  	  	    {
                        x.s2 = tzname[daylight];
                        ok = cat(&x);
  	  	    }
                    break;
                case '%':
                    x.s2 = "%";						/* % */
                    ok = cat(&x);
                    break;
                default:
                    break;
            } /* end switch */
            if (!ok)
                return 0;
        } /* end else */
        format++;
    } /* end while */

    return x.count;
}
