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
/* Modify Memory or Register           		*/
/*                           			*/
/* If a register name is specified, display and	*/
/* modify its contents;  if a hex address is	*/
/* specified, display/modify contents of memory	*/
/* at that address.				*/
/************************************************/
modify( size, nargs, where, cnt )
int size;	/* BYTE or INTEGER */
int nargs;	/* Number of following arguments that are valid (1 or 2) */
char *where;	/* Register name or hex address (required)		*/
int cnt;	/* Number of items to display/mod (optional, default 1)	*/
{
char *addr;
unsigned int data;
int r;

	if ( (r=get_regnum(where)) != ERROR ){
		if ( nargs > 1 ){
			/* Never display more than one reg at a time */
			prtf("\n Too many arguments");
		} else {
			mod_register( where, r );
		}

	} else if ( !atoh(where,&addr) ){
		badarg(where);

	} else {
		if ( nargs < 2 ){
			cnt = 1;  /* Set default */
		}
		prtf("\n");
		while ( cnt-- ) {
			prtf( "%X : ", addr );
			if (size == BYTE){
				if ( read_data(&data) ){
					store_byte(data, addr);
				}
				addr++;
			} else {   /* INT */
				prtf( "%X : ", *(unsigned int*)addr );
				read_data(addr);
				addr += 4;
			}
		}
	}
}

/************************************************/
/* Display Memory               		*/
/*                           			*/
/* If a register name is specified and the size	*/
/* is INT, display register's contents;  if re-	*/
/* gister name is specified and size is *not*	*/
/* INT, display memory at address stored in re-	*/
/* gister; if a hex address is specified,	*/
/* display contents of memory at that address.	*/
/************************************************/

display( size, nargs, where, cnt )
int size;	/* BYTE, SHORT, INT, LONG, TRIPLE, or QUAD */
int nargs;	/* Number of the following args that are valid (1 or 2) */
char * where;	/* Register name or hex address (required)		*/
int cnt;	/* Number of items to display (optional, default 1	*/
{
unsigned char byte_data;
int i;
int r;
int n;
int addr;

	if ( nargs < 2 ){
		cnt = 1;	/* Set default */
	}

	r = get_regnum(where);
	if ( r != ERROR ){
		if ( size == INT ){
			/* Display contents of register and return --
			 * no count allowed.
			 */
			if ( nargs > 1 ){
				prtf( "Too many arguments\n" );
			} else {
				disp_register(where,r);
			}
			return;
		} else if ( r >= NUM_REGS ){
			/* FP register */
			badarg( where );
			return;
		} else {
			addr = register_set[r];
		}
	} else if ( !atoh(where,&addr) ){
		badarg( where );
		return;
	}


	while ( cnt-- ) {
		prtf( "\n%X : ", addr );
		n = 0;

		switch (size) {
		case BYTE:
			byte_data = load_byte((int)addr);
			prtf( "%B", byte_data );

			/* print in ASCII if a printable character */
			if ((byte_data > 0x20) && (byte_data < 0x7f)) {
				prtf ("    %c", byte_data);
			}
			addr++;
			break;
		case SHORT:
			prtf( "%H",*(unsigned short *)addr );
			addr += sizeof(unsigned short);
			break;
		case INT:
			long_data[0] = *(unsigned int*)addr,
			n = 1;
			break;
		case LONG:
			load_long(addr);
			n = 2;
			break;
		case TRIPLE:
			load_triple(addr);
			n = 3;
			break;
		case QUAD:
			load_quad(addr);
			n = 4;
			break;
		}

		for (i=0; i<n; i++) {
			prtf("%X ", long_data[i] );
			addr += sizeof(unsigned int);
		}
	}
}


/************************************************/
/* Modify Register                  	 	*/
/*                           			*/
/************************************************/
mod_register(reg, num)
char *reg;	/* ASCII name of register	*/
int num;	/* If < NUM_REGS, an index into the 'register_set' array.
		 * Otherwise, a floating point register, and 'num-NUM_REGS'
		 *	is index into the 'fp_register_set' array.
		 */
{
unsigned int data;
	disp_register(reg, num);
	prtf (" : ");
	if (num < NUM_REGS){
		read_data(&register_set[num]);
	} else {	/* floating point register */
		prtf ("not implemented");
	}
}

/************************************************/
/* Display Register                  	 	*/
/*                           			*/
/************************************************/
disp_register(reg, num)
char reg[];
int num;    /* If < NUM_REGS, an index into the 'register_set' array.
	     * Otherwise, a floating point register, and 'num-NUM_REGS'
	     *	is index into the 'fp_register_set' array.
	     */
{
	prtf ("\n%s : ",reg);
	if (num < NUM_REGS){
		prtf( "%X", register_set[num] );
	} else {	/* floating point register */
		disp_fp_register(num-NUM_REGS);
	}
}


/************************************************/
/* Display Registers                            */
/* 						*/
/* this routine will display the last stored    */
/* register values from the global register     */
/* storage area 				*/
/************************************************/

static const int
regtab[] = {
	REG_G0, REG_G1, REG_G2, REG_G3, ERROR,
	REG_G4, REG_G5, REG_G6, REG_G7, ERROR,
	REG_G8, REG_G9, REG_G10,REG_G11,ERROR,
	REG_G12,REG_G13,REG_G14,REG_FP, ERROR,
	ERROR,
	REG_PFP,REG_SP, REG_RIP,REG_R3, ERROR,
	REG_R4, REG_R5, REG_R6, REG_R7, ERROR,
	REG_R8, REG_R9, REG_R10,REG_R11,ERROR,
	REG_R12,REG_R13,REG_R14,REG_R15,ERROR,
	REG_PC, REG_AC, REG_TC, REG_IP, ERROR,
};
#define TABLEN (sizeof(regtab)/sizeof(regtab[0]))

void
display_regs()
{
int i;
char *p;
int r;

	for ( i = 0; i < TABLEN; i++ ){
		r = regtab[i];
		if ( r == ERROR ){
			prtf( "\n" );
		} else {
			p = get_regname(r);
			prtf( " %s%s: %X", p, strlen(p) == 2 ? " " : "",
							register_set[r] );
		}
	}

	display_fp_regs();
}


/************************************************/
/* Fill Memory 					*/
/*                           			*/
/************************************************/
fill( dummy1, dummy2, addr1, addr2, data )
int dummy1;	/* Ignored */
int dummy2;	/* Ignored */
int *addr1;	/* Starting address to be filled */
int *addr2;	/* Ending address to be filled	*/
int data;	/* Word of data to fill with	*/
{
	while (addr1 <= addr2) {
		*addr1++ = data;
	}
}
