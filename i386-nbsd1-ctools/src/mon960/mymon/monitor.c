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

#define _MONITOR_DEFINES  /* remove redundent externals from core.h */
#include "mon960.h"
#include "retarget.h"
#include "hdi_com.h"

extern void set_step_string(int);
extern void hi_main(const void *);
extern void ui_main(const void *);
extern sdm_write(int, char *, int, int*);

void led_output(char, int, int, int, int);
int debug_msg(char *, int, int, int, int);

int cmd_stat;
UREG register_set;
FPREG fp_register_set[NUM_FP_REGS];

int break_vector = 0;
int break_flag = 0;
int default_regs = 1;    /* Register values from monitor, not application. */
int boot_g0 = -1;
char *step_string = NULL;
int host_connection = 0;
unsigned long baud_rate = 0;
int user_code = 0;
void (*restart_func)() = NULL;
int lost_stop_reason = FALSE;

int mon_priority = 31;    /* Priority monitor runs at */

void
init_monitor(int application)
{
    user_code = application;
    /*
     * Due to need to use timing loops to determine JD/JF identity, we
     * want to setup CPU version once at boot time.
     */
    set_step_string(boot_g0);
}

void
monitor(const void *stop_reason)
{
    static int first_time = TRUE;

    lost_stop_reason = FALSE;
    if (first_time)
    {
        comm_config(); /* Determine which comm port to connect to. */

        /* The following line allows for this situation:
         * If the monitor is linked into an application, the application
         * is running, and the host interrupts it to gain control, then
         * we'll get here with stop_reason non-NULL (since the application
         * really did just stop).  However, the host (HDIL) is currently
         * not able to handle the stop cause information coming up from the
         * target while HDIL is trying to do initialization.
         * Therefore, we will set stop_reason to NULL, so that the target
         * and host will do what each other expects (i.e., the target
         * won't be sending any such information during init).
         * Note that, normally, stop_reason is already NULL.
         */
        if (host_connection)
            lost_stop_reason = TRUE;

        first_time = FALSE;
    }

    if (host_connection)
        hi_main(stop_reason);
    else
        ui_main(stop_reason);
    /* NOTREACHED */
}

void
reset(int mode)
{
    if (!mode && restart_func)
        restart_func();
    board_reset();
}

void
set_mon_priority(unsigned int new_pri)
{
    unsigned int mon_pri;

    if (new_pri > 31)
        return;

    mon_priority = new_pri;

    if (user_code)
        return;

    /* If we defined the current register values, we can change them! */
    if (default_regs)
        {
        register_set[REG_PC] = (register_set[REG_PC] & ~0x001f0000)
                | (new_pri << 16);
        mon_pri = new_pri;
        }
    else
        {
        /* Don't set the priority less than the application value */
        unsigned int app_pri;
        app_pri = (register_set[REG_PC] & 0x001f0000) >> 16;
        mon_pri = (new_pri>app_pri ? new_pri : app_pri);
        }

    i960_modpc(0x001f0000, mon_pri << 16);
}

unsigned int
get_mon_priority()
{
    return mon_priority;
}

extern void *get_fp(void);
extern void *get_ret_ip(void);

void
fatal_fault()
{
    void *fp = get_fp();
    FAULT_RECORD *flt_rec = i960_get_fault_rec();

    fatal_error(
        0x1, flt_rec->type, (int)flt_rec->addr, (int)flt_rec->pc, (int)fp);
}

void
fatal_intr()
{
    void *fp = get_fp();
    int *ip = get_ret_ip();
    INTERRUPT_RECORD *intr_rec = i960_get_int_rec();

    fatal_error(
        0x2, (int)intr_rec->vector_number, (int)ip, (int)intr_rec->pc, (int)fp);
}

/*-------------------------------------------------------------
 * Function:    void fatal_error(char id, int (a,b,c,d))
 *
 * Passed:      id, args. FOR fatal_fault(id=1), or fatal_intr(id=2)
 *
 * Returns:     Never.
 *
 * Action:      Internal fatal error, a string and printf style are
 *              passed in.  They are only used if the debug port is
 *              enabled.  This routine is for debugging help.
 *              Before calling, fault indications can be written to
 *              the 8-segment LED.
 *-------------------------------------------------------------*/
void
fatal_error(char id, int a, int b, int c, int d)
{

    /* If the communications port is connected to a host, 
     * send the error message to the host.
     * If the communications port is connected to a terminal, send to terminal*/

     if (id == 1)
         debug_msg("Unhandeled Fault: Fault type=%X IP=%X PC=%X FP=%X ", a,b,c,d);
     else if (id == 2)
         debug_msg("Unhandeled Interrupt: Vector#=%X IP=%X PC=%X FP=%X ", a,b,c,d);
     else 
         debug_msg("Fatal Error: %s  Error ID=%B ", a,id,0,0);

     while(1)
         {
         blink(0xf); blink(0xf); blink(0xf);
         led_output(id, a,b,c,d);
         }
}


/************************************************/
/* Output Hex Number                     */
/*                                       */
/* output a 32 bit value in hex to the console  */
/* leading determines whether or not to print   */
/* leading zeros                */
/************************************************/
static int
out_hex(buffer, value, bits, leading)
unsigned char * buffer;
unsigned int value;
int bits;    /* Number of low-order bits in 'value' to be output */
int leading;    /* leading 0 flag */
{
    static const char tohex[] = "0123456789abcdef";
    unsigned char out;
    int outptr = 0;

    for (bits -= 4; bits >= 0; bits -= 4) {
        out = ((value >> bits) & 0xf); /* capture digit */
        if ( out == 0 ){
            if ( leading || (bits == 0) ){
                *buffer = '0'; outptr++; buffer++;
            }
        } else {
            *buffer = tohex[out]; outptr++; buffer++;
            leading = TRUE;
        }
    }
    return(outptr);
}


/************************************************/
/* Output Decimal Number                     */
/*                                       */
/* output a 32 bit value in decimal to the console  */
/* leading determines whether or not to print   */
/* leading zeros                */
/************************************************/
static int
out_dec(buffer, value, digits, leading)
unsigned char * buffer;
unsigned int value;
int digits;    /* Number of low-order bits in 'value' to be output */
int leading;    /* leading 0 flag */
{
    int outptr;

    buffer = buffer + digits - 1;
    for ( outptr = digits ; outptr > 0 ; outptr--)
		{
		*buffer = value % 10 + '0';
        if (value == 0 && leading == FALSE)
            *buffer = ' ';
		value = value / 10;
		buffer--;
		}

    return(digits);
}


/************************************************/
/* sprtf                        */
/*   (1) provides a simple-minded equivalent    */
/*       of the printf function.        */
/*   (2) translates '\n' to \r\n' if !host_connection.        */
/*                        */
/* In addition to the format string, up to 4    */
/* arguments can be passed.            */
/*                        */
/* Only the following specifications are    */
/* recognized in the format string:        */
/*                        */
/*    %B    output a single hex Byte, as 2    */
/*          hex digits with lead 0s.    */
/*    %b    output a single hex Byte, up to    */
/*           2 digits without lead 0s.    */
/*    %c    output a single character    */
/*    %d    output a int word: up to 4 decimal    */
/*          digits without lead 0s.    */
/*    %H    output a hex Half (short) word,    */
/*          as 4 hex digits with lead 0s.    */
/*    %h    output a hex Half (short) word,    */
/*          up to 4 digits w/o lead 0s.    */
/*    %i    output a int word: up to 10 decimal    */
/*          digits without lead 0s.    */
/*    %s    output a character string.    */
/*    %X    output a hex word, as 8 hex    */
/*          digits with lead 0s.        */
/*    %x    output a hex word: up to 8 hex    */
/*          digits without lead 0s.    */
/*    %%    output the character '%'.    */
/************************************************/
#define MAX_PRTF_ARGS 4

int
sprtf( buffer, max_size, fmt, arg0, arg1, arg2, arg3 )
unsigned char buffer[];
int max_size;
char *fmt;
int arg0;
int arg1;
int arg2;
int arg3;
{
unsigned int args[MAX_PRTF_ARGS];
int argc, outptr;
char *p;
char *q;

    args[0] = (unsigned int)arg0;
    args[1] = (unsigned int)arg1;
    args[2] = (unsigned int)arg2;
    args[3] = (unsigned int)arg3;
    argc = 0;
    outptr = 0;

    for ( p = fmt; *p; p++ ){
        if ( *p != '%' ){
	    if (!host_connection)
	      if ( *p == '\n' ){
		  buffer[outptr++] = '\r';
	      }
            buffer[outptr++] = *p;
            continue;
        }

        /* Everything past this point is processing a '%'
         * format specification.
         */

        p++;    /* p -> character after the '%'    */

        if ( *p == '\0' ){
            return ERR;
        }
        
        if ( *p == '%' ){
            buffer[outptr++] = *p;
            continue;
        }

        if ( argc >= MAX_PRTF_ARGS ){
            return ERR;
        }

        while (*p >= '0' && *p <= '9')
            p++;

        if (*p == 'l')
            p++;

        switch (*p){
        case 'B':
        case 'b':
            outptr += out_hex(&buffer[outptr], args[argc], 8, (*p=='B'));
            break;
        case 'c':
	    if (!host_connection)
	      if ( args[argc] == '\n' ){
		  buffer[outptr++] = '\r';
	      }
            buffer[outptr++] = args[argc];
            break;
        case 'd':
            outptr += out_dec(&buffer[outptr], args[argc], 4, FALSE);
            break;
        case 'H':
        case 'h':
            outptr += out_hex(&buffer[outptr], args[argc], 16, (*p=='H'));
            break;
        case 'i':
            outptr += out_dec(&buffer[outptr], args[argc], 10, FALSE);
            break;
        case 's':
            for ( q = (char*)args[argc]; *q; q++ ){
		if (!host_connection)
		  if ( *q == '\n' ){
		      buffer[outptr++] = '\r';
		  }
                buffer[outptr++] = *q;
            }
            break;
        case 'X':
        case 'x':
            outptr += out_hex(&buffer[outptr], args[argc], 32, (*p=='X'));
            break;
        default:
            return ERR;
        }
        argc++;
		if (outptr > max_size) 
			return ERR;
    }

    return(outptr);
}


/*  Send a message to either the UI terminal or  the HOST treminal
 *  interface.  This is a way to debug mon960 internals.
 *  The caller may provide a format or use the default format (NULL).
*/
int
debug_msg(char * fmt, int arg0, int arg1, int arg2, int arg3)
{
#define LINESIZE    80
    int dummy;
    unsigned char buffer[LINESIZE];
    int outptr = 0;    
    char * fmt_err_msg = "SPRTF format string error.\n"; /* 27 charaters long */

	if (fmt==NULL) 
		{
        if ((outptr = sprtf(buffer, LINESIZE,"ID=%B ARG1 = %X ARG2 = %X ARG3 = %X\n",arg0,arg1,arg2,arg3)) == ERR)
			{
			sdm_write(1,fmt_err_msg,27,&dummy);
            return ERR;
			}
        }
    else
		{
        if ((outptr = sprtf(buffer, LINESIZE,fmt,arg0,arg1,arg2,arg3)) == ERR)
			{
			sdm_write(1,fmt_err_msg,27,&dummy);
            return ERR;
			}
		}

    return(sdm_write(1, buffer, outptr, &dummy));
}

