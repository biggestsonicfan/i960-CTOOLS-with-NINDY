/******************************************************************/
/* 		Copyright (c) 1989, Intel Corporation

   Intel hereby grants you permission to copy, modify, and 
   distribute this software and its documentation.  Intel grants
   this permission provided that the above copyright notice 
   appears in all copies and that both the copyright notice and
   this permission notice appear in supporting documentation.  In
   addition, Intel grants this permission provided that you
   prominently mark as not part of the original any modifications
   made to this software or documentation, and that the name of 
   Intel Corporation not be used in advertising or publicity 
   pertaining to distribution of the software or the documentation 
   without specific, written prior permission.  

   Intel Corporation does not warrant, guarantee or make any 
   representations regarding the use of, or the results of the use
   of, the software and documentation in terms of correctness, 
   accuracy, reliability, currentness, or otherwise; and you rely
   on the software, documentation and results solely at your own 
   risk.							  */
/******************************************************************/
#include "defines.h"
#include "globals.h"
#include "regs.h"



/************************************************/
/* readline					*/
/*                           			*/
/* Input a line from the user and leave it in	*/
/* the indicated buffer.  Recognize backspace   */
/* and DEL as character delete.  Don't let the	*/
/* buffer overflow.				*/
/************************************************/
readline( buf, bufsize, gdb_abort )
char *buf;		/* Input buffer					*/
int bufsize;		/* Input buffer size in bytes			*/
int gdb_abort;		/* If TRUE, return immediately if ^P (DLE -- start
			 *	of gdb command) received.
			 */
{
int count;		/* Number of characters in the input buffer	*/
unsigned char c;	/* Next input character				*/

	count = 0;
	buf[0] = '\0';

	for ( c=ci(); c != '\n' && c != '\r';  c=ci() ){

		switch (c) {

		case '\b':
		case DEL:
			if (count == 0) {
				co( BEL );
			} else {
				count--;
				prtf( "\b \b" );
			}
			break;

		case '\t':
			c = ' ';
			/* Fall through to next case */
		default:
			if ( (c==DLE) && gdb_abort ){
				/* start of gdb command.  Discard anything else
				 * we may have seen to this point --  this helps
				 * us get back in sync with host in case of
				 * error.
				 */
				buf[0] = DLE;
				gdb = TRUE;
				return;
			} else if ( count >= bufsize - 1 ){
				co( BEL );
			} else {
				co(c);
				buf[count++] = c;
			}
			break;
		}
		buf[count] = '\0';
	} 
}


/************************************************/
/* Read Data                       	 	*/
/*                           			*/
/* Read a line of input from the user, and	*/
/* convert it to a hex value.  Return TRUE on	*/
/* success;  print "unchanged" and return FALSE	*/
/* on failure.					*/
/************************************************/
int
read_data(data)
int *data;
{
char buf[16];
int retval;

	readline( buf, sizeof(buf), FALSE );
	retval = atoh(buf,data);
	prtf( "%s\n", retval ? "" : "unchanged" );
	return retval;
}


/************************************************/
/* get_words:	break individual words out of 	*/
/*		a command.			*/
/*						*/
/* Output:					*/
/*	words[0] is left pointing to first word	*/
/*	in command (command name), words[1]	*/
/*	points to 2nd word (first argument),	*/
/*	etc.  All words are '\0'-terminated.	*/
/*	Words are separated by whitespace or '#'*/
/*						*/
/* Return value:				*/
/*	Number of words in list.		*/
/*	Unused entries in 'words' are zeroed.	*/
/*						*/
/************************************************/
int
get_words( p, words, max )
    register char *p;	/* Pointer to space-separated operands; MUCKED BY US */
    char *words[];	/* Output arg -- see above	*/
    int max;		/* Max number of words to be accommodated.
			 *	'words' must contain 'max' entries.
			 */
{
	register int n;		/* Number of words found */


	for ( n = 0; n < max; n++ ){
		words[n] = NULL;
	}

	n = 0;

	while ( *p != '\0' ){

		/* Skip lead white space */
		while ( *p == ' ' || *p == '\t' || *p == '#' ){
			p++;
		}

		if ( *p == '\0' ){
			break;
		}

		if ( n++ < max ){
			words[n-1] = p;
		}

		/* Find end of word */
		while ( *p != ' ' && *p != '\t' && *p != '#' && *p != '\0' ){
			p++;
		}

		/* Terminate word */
		if ( *p != '\0' ){
			*p++ = '\0';
		}
	}
	return n;
}


/************************************************/
/* Get Register Number       			*/
/*                           			*/
/*	Returns:				*/
/*						*/
/*	- offset into 'register_set' array, if	*/
/*	  register is a local, global, or	*/
/*	  control register.			*/
/*						*/
/*	- NUM_REGS+(offset into fp_register_set)*/
/*	  if register is a floating pt reg, and	*/
/*	  floating point is installed		*/
/*						*/
/*	- ERROR if register name is unknown	*/
/*						*/
/************************************************/
struct tabentry { char *name; int num; };

static const struct tabentry
reg_tab[] = {
    {"g0", REG_G0},  {"g1", REG_G1},  {"g2", REG_G2},  {"g3", REG_G3},
    {"g4", REG_G4},  {"g5", REG_G5},  {"g6", REG_G6},  {"g7", REG_G7},
    {"g8", REG_G8},  {"g9", REG_G9},  {"g10",REG_G10}, {"g11",REG_G11},
    {"g12",REG_G12}, {"g13",REG_G13}, {"g14",REG_G14}, {"fp", REG_FP},
    {"pfp",REG_PFP}, {"sp", REG_SP},  {"rip",REG_RIP}, {"r3", REG_R3},
    {"r4", REG_R4},  {"r5", REG_R5},  {"r6", REG_R6},  {"r7", REG_R7},
    {"r8", REG_R8},  {"r9", REG_R9},  {"r10",REG_R10}, {"r11",REG_R11},
    {"r12",REG_R12}, {"r13",REG_R13}, {"r14",REG_R14}, {"r15",REG_R15},
    {"pc", REG_PC},  {"ac", REG_AC},  {"ip", REG_IP},  {"tc", REG_TC},
    {0,	0}
};

int
get_regnum(reg)
char *reg;	/* Name of register	*/
{
	const struct tabentry *tp;

	for (tp=reg_tab; tp->name != 0; tp++) {
		if (!strcmp(reg,tp->name)) {
			return (tp->num);
		}
	}

	return get_fp_regnum(reg);
}


/************************************************/
/* Get Register Name       			*/
/*                           			*/
/*	Returns:				*/
/*						*/
/*	- pointer to string containing name of	*/
/*	  register whose number is 'n', if	*/
/*	  register is a local, global, or	*/
/*	  control register.			*/
/*						*/
/*	- NULL if register name is unknown	*/
/*						*/
/************************************************/

char *
get_regname(n)
int n;		/* Number of register */
{
	const struct tabentry *tp;

	for (tp=reg_tab; tp->name != 0; tp++) {
		if ( tp->num == n ){
			return tp->name;
		}
	}
	return NULL;
}




badarg(s)
char *s;
{
	prtf( "Bad argument: '%s'\n", s );
}
