/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1993, 1994 Intel Corporation
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

#include "ui.h"
#include "i960.h"

extern int load_mem(ADDR, int mem_size, void *buf, int buf_size);
extern int store_mem(ADDR, int mem_size, const void *data, int data_size, int verify);
extern int fill_mem(ADDR, int mem_size, const void *data, int data_size, int count);
extern prtf(), atoh(), perror(), get_regnum(), read_data(), badarg();
extern disp_fp_register(), display_fp_regs(), strlen(); 
extern int get_mask(), get_pending();

void mod_register(char *, int), disp_register(char *, int);

static void display_sfrs();

/************************************************/
/* Modify Memory or Register direct    		*/
/*                           			*/
/* If a register name is specified, */
/* modify its contents;  if a hex address is	*/
/* specified, modify contents of word	*/
/* at that address.				*/
/************************************************/
void
modify_d(size, nargs, where, value)
int size;	/* WORD */
int nargs;	/* Number of following arguments that are valid (1 or 2) */
char *where;	/* Register name or hex address (required)		*/
int value;  /* value to set */
{
	ADDR addr;
	int r;

	if ((r=get_regnum(where)) != ERR)
		{
		register_set[r] = value;
	    }
	else
		{
	    if (!atoh(where,&addr))
	    	badarg(where);
		else
	        if (store_mem(addr, size, &value, size, TRUE) != OK)
 		    	perror((char *)NULL, 0);
        }
}

/************************************************/
/* Modify Memory or Register           		*/
/*                           			*/
/* If a register name is specified, display and	*/
/* modify its contents;  if a hex address is	*/
/* specified, display/modify contents of memory	*/
/* at that address.				*/
/************************************************/
void
modify(size, nargs, where, cnt)
int size;	/* BYTE or WORD */
int nargs;	/* Number of following arguments that are valid (1 or 2) */
char *where;	/* Register name or hex address (required)		*/
int cnt;	/* Number of items to display/mod (optional, default 1)	*/
{
	ADDR addr;
	int r;

	if ((r=get_regnum(where)) != ERR)
		{
		if (nargs > 1)
			{
			/* Never display more than one reg at a time */
			prtf("Too many arguments\n");
		    }
		else
			{
			mod_register(where, r);
		    }
	    }
    else if (!atoh(where,&addr))
		{
		badarg(where);
	    }
	else
		{
		if (nargs < 2)
	    	{
			cnt = 1;  /* Set default */
	    	}
		break_flag = FALSE;
		while (cnt-- && !break_flag)
			{
			unsigned int data[4];
                        unsigned char *cp = (unsigned char *)data;
                        unsigned short *sp = (unsigned short *)data;

			prtf( "%X : ", addr );

			if (size != BYTE)
			    {
			    if (load_mem(addr, size, data, size) != OK)
			        {
				    perror((char *)NULL, 0);
				    break;
			        }
			    if (size == SHORT)
				    data[0] = *sp;
		        prtf("%X : ", data[0]);
			    }

			if (read_data(data))
			    {
			    if (size == BYTE)
				    *cp = data[0];
			    else
				    if (size == SHORT)
				        *sp = data[0];

			    if (store_mem(addr, size, data, size, FALSE) != OK)
			        {
			    	perror((char *)NULL, 0);
			    	break;
			        }
			    }

			addr += size;
		    }
	    }
}

/************************************************/
/* Display Memory               		*/
/*                           			*/
/* If a register name is specified and the size	*/
/* is WORD, display register's contents;  if 	*/
/* register name is specified and size is *not*	*/
/* WORD, display memory at address stored in	*/
/* register; if a hex address is specified,	*/
/* display contents of memory at that address.	*/
/************************************************/

void
display(size, nargs, where, cnt)
int size;	/* BYTE, SHORT, WORD, LONG, TRIPLE, or QUAD */
int nargs;	/* Number of the following args that are valid (1 or 2) */
char * where;	/* Register name or hex address (required)		*/
int cnt;	/* Number of items to display (optional, default 1	*/
{
unsigned char byte_data;
int i;
int r;
int addr;

	if (nargs < 2)
		cnt = 1;	/* Set default */
	

	r = get_regnum(where);
	if (r != ERR)
		{
		if (size == WORD)
			{
			/* Display contents of register and return --
			 * no count allowed.
			 */
			if (nargs > 1)
				{
				prtf("Too many arguments\n");
			    }
			else 
				{
				disp_register(where,r);
				prtf("\n");
		    	}
			return;
		    }
		else if (r >= NUM_REGS)
			{
			/* FP register */
			badarg(where);
			return;
		    }
		else 
			{
			addr = register_set[r];
		    }
	    }
	else if (!atoh(where,&addr))
		{
		badarg(where);
		return;
	    }

	break_flag = FALSE;
	while (cnt-- && !break_flag) 
		{
		unsigned int data[4];

		prtf( "%X : ", addr );

		if (load_mem(addr, size, data, size) != OK)
		    {
		    perror((char *)NULL, 0);
		    break;
		    }

		switch (size) 
			{
		    case BYTE:
		    	byte_data = *(unsigned char *)data;
		    	prtf( "%B", byte_data );

		    	/* print in ASCII if a printable character */
		    	if ((byte_data > 0x20) && (byte_data < 0x7f)) 
		    		{
		    		prtf ("    %c", byte_data);
		        	}
		    	break;
		    case SHORT:
		    	prtf( "%H", *(unsigned short *)data);
		    	break;
		    default:
		    	for (i = 0; i < size/sizeof(int); i++)
		    		prtf("%X ", data[i]);
		    	break;
		    }

		prtf("\n");
		addr += size;
	    }
}

/************************************************/
/* Display Char               		        */
/************************************************/

void
display_char(null_check, nargs, where, cnt )
int null_check;
int nargs;	/* Number of the following args that are valid (1 or 2) */
char * where;	/* Register name or hex address (required)		*/
int cnt;	/* Number of items to display (optional)                */
{
	int r;
	int addr;

	if (nargs < 2)
	    {
		cnt = 64;	/* Set default */
		null_check = TRUE;
	    }

	r = get_regnum(where);
	if (r != ERR)
		{
		if (r >= NUM_REGS)
			{
			/* FP register */
			badarg(where);
			return;
	    	}
		else 
			{
			addr = register_set[r];
	    	}
	    }
	else if (!atoh(where,&addr))
		{
		badarg(where);
		return;
	    }

	break_flag = FALSE;
	while (cnt > 0 && !break_flag) 
		{
		char data;
		int i = 0;

		prtf("%X : ", addr);

		while (cnt-- > 0 && i++ < 64)
	    	{
		    if (load_mem(addr, 1, &data, 1) != OK)
	    	    {
	    		perror((char *)NULL, 0);
	    		cnt = 0;
	    		break;
	    	    }
		    addr++;

		    if (null_check && data == '\0')
	    		cnt = 0;
		    else if (data >= 0x20 && data < 0x7f)
	    		prtf("%c", data);
		    else if (data == '\n')
	    		break;
		    else
	    		prtf(".");
	    	}
		prtf("\n");
	    }
}



/************************************************/
/* Modify Register                  	 	*/
/*                           			*/
/************************************************/
void
mod_register(reg, num)
char *reg;	/* ASCII name of register	*/
int num;	/* If < NUM_REGS, an index into the 'register_set' array.
		     * Otherwise, a floating point register, and 'num-NUM_REGS'
		     *	is index into the 'fp_register_set' array.
		     */
{
	break_flag = FALSE;	/* Used by read_data */
	disp_register(reg, num);
	prtf (" : ");
	if (num < NUM_REGS)
		read_data(&register_set[num]);
	else	/* floating point register */
		prtf ("not implemented");
}

/************************************************/
/* Display Register                  	 	*/
/*                           			*/
/************************************************/
void
disp_register(reg, num)
char reg[];
int num;    /* If < NUM_REGS, an index into the 'register_set' array.
	         * Otherwise, a floating point register, and 'num-NUM_REGS'
	         *	is index into the 'fp_register_set' array.
	         */
{
	prtf ("%s : ",reg);
	if (num < NUM_REGS)
		prtf("%X", register_set[num]);
	else /* floating point register */
		disp_fp_register(num-NUM_REGS);
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
	REG_G0, REG_G1, REG_G2, REG_G3, -1,
	REG_G4, REG_G5, REG_G6, REG_G7, -1,
	REG_G8, REG_G9, REG_G10,REG_G11,-1,
	REG_G12,REG_G13,REG_G14,REG_FP, -1,
	-1,
	REG_PFP,REG_SP, REG_RIP,REG_R3, -1,
	REG_R4, REG_R5, REG_R6, REG_R7, -1,
	REG_R8, REG_R9, REG_R10,REG_R11,-1,
	REG_R12,REG_R13,REG_R14,REG_R15,-1,
	-1,
	REG_PC, REG_AC, REG_TC,
};
#define TABLEN (sizeof(regtab)/sizeof(regtab[0]))

void
display_regs()
{
int i;
char *p;
int r;

	for (i = 0; i < TABLEN; i++)
		{
		r = regtab[i];
		if (r == -1)
			{
			prtf("\n");
	     	}
		else 
			{
			p = get_regname(r);
			prtf("%s%s: %X   ", p, (strlen(p)==2)?" ":"", register_set[r]);
		    }
	    }

	display_sfrs();
	display_fp_regs();
}


/************************************************/
/* Fill Memory 					*/
/*                           			*/
/************************************************/
void
fill(dummy1, dummy2, addr1, addr2, data)
int dummy1;	/* Ignored */
int dummy2;	/* Ignored */
ADDR addr1;	/* Starting address to be filled */
ADDR addr2;	/* Ending address to be filled	*/
int data;	/* Word of data to fill with	*/
{
	if (fill_mem(addr1, WORD, &data, sizeof(int),
	             (addr2 - addr1 + sizeof(int)) / sizeof(int)) != OK)
	    perror((char *)NULL, 0);
}

/************************************************/
/* Display Special Function Registers 		*/
/*						*/
/* this routine will display the last stored	*/
/* sfr values from the global register storage	*/
/* area						*/
/************************************************/
#if HX_CPU
void
display_sfrs()
{
	prtf("\nipnd: %X   imsk: %X", register_set[REG_IPND], register_set[REG_IMSK]);
	prtf("\ncctl: %X   intc: %X   gmuc: %X\n",
        register_set[REG_SF2], register_set[REG_SF3], register_set[REG_SF4]);
}
#endif /*HX*/

#if JX_CPU
void
display_sfrs()
{
	prtf("\nipnd: %X   imsk: %X\n", register_set[REG_IPND],
		register_set[REG_IMSK]);
}
#endif /*JX*/

#if CX_CPU
void
display_sfrs()
{
	prtf("\nipnd: %X   imsk: %X   dmac: %X\n", register_set[REG_SF0],
		register_set[REG_SF1], register_set[REG_SF2]);
}
#endif  /*CX*/

#if KXSX_CPU
extern unsigned int interrupt_register_read();
void
display_sfrs()
{
	/* The Kx does not define any special function registers. */
    prtf("ir : %X\n", interrupt_register_read());
}
#endif /*KXSX*/
