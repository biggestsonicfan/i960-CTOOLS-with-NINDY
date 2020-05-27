/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1993, 1994, 1995 Intel Corporation
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
/*)ce*/
/*
 * $Header: /ffs/p1/dev/src/hdilcomm/common/RCS/getopt.c,v 1.4 1995/08/31 09:56:32 cmorgan Exp $
 * Windows compatible device level driver; using Windows Device[IO]...
 */

 
/**************************************************************************
*
*  Name:
*    getopt
*
*  Description:
*    loads the global _com_config structure with the proper configuration
*    information on startup
*
*  NOTE:  This file is shared with a number of different projects so be 
*         careful when changing it.
*
************************************************************************m*/


#include <stdio.h>
#include <string.h>
#include <limits.h>

#ifdef __STDC__
#include <stdlib.h>
#else
extern long atol();
#endif

#if defined(WINDOWS)	
#include <windows.h>
#include <dos.h>
#else
#define WINAPI
#endif /* WINDOWS */
#include <assert.h>

#include "common.h"
#include "com.h"

#if defined(WINDOWS)
extern void com_get_ini();
#endif /* WINDOWS */



/* 
 * Handlers to verify user-supplied config settings and initialize 
 * global config struct.  There must be a handler for each option 
 * given to HI in the target_opt_list[].
 */
#if defined(__STDC__)
static int set_baud (const char *);
static int set_com (const char *);
#if defined(MSDOS)
static int set_freq (const char *);
static int set_irq (const char *);
static int set_iobase (const char *);
#endif /* MSDOS */
static int set_host (const char *);
static int set_target (const char *);
static int set_ack (const char *);
static int set_max_length (const char *);
static int set_max_retry (const char *);
#else
static int set_baud ();
static int set_com ();
#if defined(MSDOS)
static int set_freq ();
static int set_irq ();
static int set_iobase ();
#endif /* MSDOS */
static int set_host ();
static int set_target ();
static int set_ack ();
static int set_max_length ();
static int set_max_retry ();
#endif /* __STDC__ */


/*
 * An array of config option handlers;  The ORDER of this array 
 * must match the order of the target_opt_list defined next.
 */
static const opt_handler_t opt_handler[] =
{
	set_baud,
	set_com,
	set_com,     /*Yes, this SHOULD be here twice (for "port" synonym).   */
	set_com,     /*Yes, this SHOULD be here thrice (for "serial" synonym).*/
#if defined(MSDOS)
	set_freq,
	set_irq,
	set_iobase,
#endif /* MSDOS */
	set_host,
	set_target,
	set_ack,
	set_max_length,
	set_max_retry,
};


#define TBL_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define MAXOPT TBL_SIZE(opt_handler)

/* 
 * An array of allowable target-specific options.  The ORDER of this array
 * must match the order of the opt_handler array defined above.  FALSE in
 * the third field means no argument is required for the option, therefore
 * no memory will be allocated for the argument (fourth) field.
 */
static unsigned char com_opt_set [MAXOPT];

#if defined(MSDOS)
static unsigned char freq_opt_set, irq_opt_set, iobase_opt_set;
#endif

static const COM_INVOPT com_opt_list [] =
{ 
	{ "baud",	&com_opt_set[0],  TRUE, NULL },
	{ "com",	&com_opt_set[1],  TRUE, NULL },
	{ "port",	&com_opt_set[2],  TRUE, NULL },
	{ "serial",	&com_opt_set[3],  TRUE, NULL },
#if defined(MSDOS)
	{ "freq",	&freq_opt_set,    TRUE, NULL },
	{ "irq",	&irq_opt_set,     TRUE, NULL },
	{ "iobase",	&iobase_opt_set,  TRUE, NULL },
#endif /* MSDOS */
	{ "host_pkt_timeout", &com_opt_set[4],  TRUE, NULL },
	{ "target_pkt_timeout", &com_opt_set[5], TRUE, NULL },
	{ "ack_timeout",&com_opt_set[6],  TRUE, NULL },
	{ "max_pkt_len",&com_opt_set[7],  TRUE, NULL },
	{ "max_retries",&com_opt_set[8],  TRUE, NULL },
};



/*p**********************************************************************
*
*  NAME: checkNumber
*
*  DESIGNER: Boaz Kochman
*
*  RETURNS: 1 if all characters in a string are numeric
*           0 otherwise
*
*  DESCRIPTION:
*    Determines whether or not all characters in a string are numeric
*
**********************************************************************p*/ 

int
checkNumber(s)
const char *s;          /* string to be checked */
{
    const char *tmp;
  
    tmp = s;
    while (*tmp != '\0')
        if (((*tmp < '0') || (*tmp > '9')) && (*tmp != '\0'))
            return 0;
        else
            tmp++;
    return 1;
} /* checkNumber */



/*p**********************************************************************
*
*  NAME: checkHexNumber
*
*  DESIGNER: Boaz Kochman
*
*  RETURNS: 1 if all characters in a string are a hex digit
*           0 otherwise
*
*  DESCRIPTION:
*    Determines whether or not all characters in a string are hex digits
*
**********************************************************************p*/ 

int
checkHexNumber(s)
const char *s;          /* string to be checked */
{
    
    const char *tmp;
  
    tmp = s;
    while (*tmp != '\0')
        if (((*tmp >= '0') && (*tmp <= '9')) ||
                ((*tmp >= 'A') && (*tmp <= 'F')) ||
                ((*tmp >= 'a') && (*tmp <= 'f')))
            tmp++;
        else
            return 0;
    return 1;
} /* checkHexNumber */



/*l**********************************************************************
*
*  NAME: toUpper
*
*  DESIGNER: Tim Gardner
*
*  RETURNS: the Upper case of a character if its is alphabetic
*           the character unchanged otherwise
*
*  DESCRIPTION:
*    Converts a character to upper case
*
**********************************************************************l*/ 

static char
toUpper(c)
char c;         /* character to be converted */
{
	if ((c >= 'a') && (c <= 'z')) {
		c = (c-'a') + 'A';
	}
	return(c);
} /* toUpper */



/*l**********************************************************************
*
*  NAME: toHex
*
*  DESIGNER: Tim Gardner
*
*  RETURNS: the value of a hex digit
*
*  DESCRIPTION:
*    Converts a character to it hex value
*
**********************************************************************l*/ 

static int
toHex(c)
char c;         /* character to be converted */
{
	if ((c >= '0') && (c <= '9')) {
		return(c - '0');
	}
	else if ((c >= 'A') && (c <= 'F')) {
		return(c - 'A' + 10);
	}
	return(-1);
} /* toHex */ 


/*l**********************************************************************
*
*  NAME: strToUpper
*
*  DESIGNER: Tim Gardner
*
*  RETURNS: the address of the input string.
*
*  DESCRIPTION:
*	Forces a string to all upper case.
*
**********************************************************************l*/ 

char *
strToUpper(s)
char *s;         /* character to be converted */
{
	char *tmp = s;
	while (*s)
	{
		*s++ = toUpper(*s);
	}
	return(tmp);
}


/*p**********************************************************************
*
*  NAME: power
*
*  DESIGNER: Boaz Kochman
*
*  RETURNS: x to the power of y, where x and y are integers 
*
*  DESCRIPTION:
*    Calculates an integer raised to an integral power
*
**********************************************************************p*/ 

static long
power(x, y)
int x;          /* base */
int y;          /* exponent */
{
    short i;
    long val = 1;
    
    for (i = 1; i <= y; ++i)
        val = val * x;
    return val;
}



/*p**********************************************************************
*
*  NAME: get_hex
*
*  DESIGNER: Tim Gardner
*
*  RETURNS: 0 if an error occured
*           1 otherwise
*
*  DESCRIPTION:
*    Accepts the pointer to a long and a character string, returns the
*    value of the string as a hex number in the long pointer.
*
**********************************************************************p*/ 

int
get_hex(s, val)
const char *s;      /* string to be converted */
long *val;          /* location to put result in */
{
	char c;
	const char *s2;
	int m = 0;
	int tmp;

	*val = 0;
	s2 = s + strlen(s) - 1;
	while (s2 >= s)
        {
		c = toUpper(*s2);
		if ((tmp=toHex(c)) >= 0) {
			*val += tmp * (power(16, m));
			m++;
			s2--;
		}
		else
        {
			break;
		}
	}
	return(m ? 1 : 0);
}


/* 
 * Config "handlers" to do the following:
 * (1) verify the validity of a user option
 * (2) set the correct field within the global config struct. 
 *
 * They all return 0 if successful, a non-zero error code if not.
 */


static int set_baud (arg)
const char * arg;
{
    /* if not all chatacters in string are numerical report an error
       and set the baud to 0 (garbage) */
    if (checkNumber(arg))
    { 
        _com_config.baud = atol(arg);
        _com_config_set.baud = CMD_LINE;
    }
    else
    {
        _com_config.baud = 0;
        _com_config_set.baud = ERR_CMD_LINE;
    }
    return 0;
}


static int set_com (arg)
const char * arg;
{
#if defined(MSDOS)
    if (strlen(arg) == 4)
#else
    /* allow unix style device names */
    if (*arg == '/') /* a UNIX-style device name */
    {
        strcpy(_com_config.device, arg);
        _com_config_set.device = CMD_LINE;
    }

    /* string must be exactly four characters for comX comport */
    else if (strlen(arg) == 4)
#endif
    {
        /* allow the argument to be in lowercase or caps but put caps into
         * _com_config 
         */
        strToUpper(arg);
        if (!strncmp(arg, "COM", 3))
        {
            strcpy(_com_config.device, arg);
            _com_config_set.device = CMD_LINE;
        }
        else
        {
            *(_com_config.device) = '\0';
            _com_config_set.device = ERR_CMD_LINE;
        }
    }
    

    /* we have a parse error so fill _com_config with an empty string and
     * fill _com_config_set with an error flag 
     */
    else
    {
	*(_com_config.device) = '\0';
        _com_config_set.device = ERR_CMD_LINE;
    }
        
    return 0;
}



#if defined(MSDOS)
static int set_freq (arg)
const char * arg;
{

    /* make sure each character is a hex digit */
    if (checkNumber(arg))
    {
        _com_config.freq = atol(arg);
        _com_config_set.freq = CMD_LINE;
    }
    else
    {
        _com_config.freq = 0;
        _com_config_set.freq = ERR_CMD_LINE;
    }

    return 0;
}

static int set_irq (arg)
const char * arg;
{
    unsigned long tmpl;

    /* make sure each character is a decimal digit, AND it won't overflow */
    if (checkNumber(arg) && ((tmpl=atol(arg))<=UCHAR_MAX))
    {
        _com_config.irq = (unsigned char)tmpl;
        _com_config_set.irq = CMD_LINE;
    }
    
    /* if not, set the error flag */
    else
    {
        _com_config.irq = 0;
        _com_config_set.irq = ERR_CMD_LINE;
    }

    return 0;
}    


static int set_iobase (arg)
const char * arg;
{
    long iobase;

    /* check to make sure each character is a hex digit, if not report error */
    if (!checkHexNumber(arg))
    {
        _com_config.iobase = 0;
        _com_config_set.iobase = ERR_CMD_LINE;
    }

    /* convert the hex string to a long integer. get_hex puts the
     * result in iobase as a side-effect
     */
    else if (!get_hex(arg,&iobase))
    /* an error occured in get_hex */   
    {
        _com_config.iobase = 0;
        _com_config_set.iobase = ERR_CMD_LINE;
    }

    /* OK so far, check to make sure fits into an unsigned int space */
    else
    {
        if (iobase <= UINT_MAX)
        {
            _com_config.iobase = (unsigned int)iobase;
            _com_config_set.iobase = CMD_LINE;
        }
        
        /* error, too big */
        else
        {
            _com_config.iobase = 0;
            _com_config_set.iobase = ERR_CMD_LINE;
        }
    }
    
    return 0;
}
#endif /* MSDOS */



static int set_host (arg)
const char * arg;
{
    unsigned long tmpl;

    /* make sure each character is a decimal digit, AND no overflow */
    if (checkNumber(arg) && ((tmpl=atol(arg))<=USHRT_MAX))
    {
        _com_config.host_pkt_timo = (unsigned short)tmpl;
        _com_config_set.host_pkt_timo = CMD_LINE;
    }

    /* error, not all characters are decimal digits */
    else
    {
        _com_config.host_pkt_timo = 0;
        _com_config_set.host_pkt_timo = ERR_CMD_LINE;
    }

    return 0;
}

static int set_target (arg)
const char * arg;
{
    unsigned long tmpl;

    /* make sure each character is a decimal digit, AND no overflow */
    if (checkNumber(arg) && ((tmpl=atol(arg))<=USHRT_MAX))
    {
        _com_config.target_pkt_timo = (unsigned short)tmpl;
        _com_config_set.target_pkt_timo = CMD_LINE;
    }

    /* error, not all characters are a deciamal digit */
    else
    {
        _com_config.target_pkt_timo = 0;
        _com_config_set.target_pkt_timo = ERR_CMD_LINE;
    }

return 0;
}

static int set_ack (arg)
const char * arg;
{
    unsigned long tmpl;

    /* make sure each character is a decimal digit, AND no overflow */
    if (checkNumber(arg) && ((tmpl=atol(arg))<=USHRT_MAX))
    {
        _com_config.ack_timo = (unsigned short)tmpl;
        _com_config_set.ack_timo = CMD_LINE;
    }

    /* error, not all characters are decimal digits */
    else
    {
        _com_config.ack_timo = 0;
        _com_config_set.ack_timo = ERR_CMD_LINE;
    }
    
    return 0;
}

static int set_max_length (arg)
const char * arg;
{
    /* make sure each character is a deciamal digit */
    if (checkNumber(arg))
    {
        long len = atol(arg);
    
        if ((len > 1) && (len <= USHRT_MAX))
        {
            _com_config.max_len = (unsigned short)len;
            _com_config_set.max_len = CMD_LINE;
        }
        else
        {
            _com_config.max_len = 0;
            _com_config_set.max_len = ERR_CMD_LINE;
        }
    }
    else
    {
        _com_config.max_len = 0;
        _com_config_set.max_len = ERR_CMD_LINE;
    }

    return 0;
}

static int set_max_retry (arg)
const char * arg;
{
    unsigned long tmpl;

    /* make sure each character is a decimal digit, AND it won't overflow */
    if (checkNumber(arg) && ((tmpl=atol(arg))<=UCHAR_MAX))
    {
        _com_config.max_try = (unsigned char)tmpl;
        _com_config_set.max_try = CMD_LINE;
    }
    
    /* error, not all characters decimal digits OR overflow */
    else
    {
        _com_config.max_try = 0;
        _com_config_set.max_try = ERR_CMD_LINE;
    }
    
    return 0;
}





/*p**********************************************************************
*
*  NAME: com_get_opt_list
*
*  DESIGNER: Boaz Kochman
*
*  RETURNS: the number of command line options it handles
*
*  DESCRIPTION:
*    Returns command line option handlers 
*
**********************************************************************p*/ 

short WINAPI
com_get_opt_list(optPtr, optHandler)
const COM_INVOPT **optPtr;
const opt_handler_t **optHandler;
{
    
#if defined(WINDOWS)
com_get_ini();
#endif /* WINDOWS */
            
    *optPtr = com_opt_list;
    *optHandler = opt_handler;
    return(MAXOPT);
}
