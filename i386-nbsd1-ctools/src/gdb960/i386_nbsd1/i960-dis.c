/* Disassemble i80960 instructions.
   Copyright (C) 1990, 1991 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "dis-asm.h"
#if defined(IMSTG)
#include "defs.h"
#else
/* these are declared in infcmd.c, extern'ed in defs.h */
static char *reg_names[] = {
/*  0 */	"pfp", "sp",  "rip", "r3",  "r4",  "r5",  "r6",  "r7", 
/*  8 */	"r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
/* 16 */	"g0",  "g1",  "g2",  "g3",  "g4",  "g5",  "g6",  "g7", 
/* 24 */	"g8",  "g9",  "g10", "g11", "g12", "g13", "g14", "fp", 
/* 32 */	"pc",  "ac",  "ip",  "tc",  "fp0", "fp1", "fp2", "fp3" 
};
#endif

static FILE *stream;		/* Output goes here */
static struct disassemble_info *info;

static void print_addr PARAMS ((int));
static void ctrl PARAMS ((unsigned long, unsigned long));
static void cobr PARAMS ((unsigned long, unsigned long));
static void reg PARAMS ((unsigned long));
static int mem PARAMS ((unsigned long, unsigned long, unsigned long, int));
static void ea PARAMS ((CORE_ADDR, int, const char *, const char *, 
			unsigned long, unsigned long));
static void dstop PARAMS ((int, int, int));
static void regop PARAMS ((int, int, int, int));
static void invalid PARAMS ((int));
static int pinsn PARAMS ((unsigned long, unsigned long, unsigned long));
static void put_abs PARAMS ((unsigned long, unsigned long));


/* Print the i960 instruction at address 'memaddr' in debugged memory,
   on INFO->STREAM.  Returns length of the instruction, in bytes. */
int
print_insn_i960 (memaddr, info_arg)
    bfd_vma memaddr;
    struct disassemble_info *info_arg;
{
  unsigned int word1, word2 = 0;
  bfd_byte buffer[8];
  int status;

  info = info_arg;
  stream = info->stream;

  /* Read word1.  Only read word2 if the instruction
     needs it, to prevent reading past the end of a section.  */

  status = (*info->read_memory_func) (memaddr, (bfd_byte *) buffer, 4, info);
  if (status != 0)
    {
      (*info->memory_error_func) (status, memaddr, info);
      return -1;
    }

#if defined(IMSTG)	/* Intel BIG_ENDIAN mods */
  word1 = TARGET_BIG_ENDIAN ? bfd_getb32(buffer) : bfd_getl32(buffer);
#else
  word1 = bfd_getl32 (buffer);
#endif

  /* Divide instruction set into classes based on high 4 bits of opcode.  */
  switch ( (word1 >> 28) & 0xf )
    {
    default:
      break;
    case 0x8:
    case 0x9:
    case 0xa:
    case 0xb:
    case 0xc:
    case 0xd:
      /* Read word2.  */
      status = (*info->read_memory_func)
	(memaddr + 4, (bfd_byte *) (buffer + 4), 4, info);
      if (status != 0)
	{
	  (*info->memory_error_func) (status, memaddr, info);
	  return -1;
	}
#if defined(IMSTG)	/* Intel BIG_ENDIAN mods */
      word2 = TARGET_BIG_ENDIAN ? bfd_getb32(buffer + 4) : bfd_getl32(buffer + 4);
#else
      word2 = bfd_getl32 (buffer + 4);
#endif
      break;
    }

  return pinsn( memaddr, word1, word2 );
}


#define IN_GDB

/*****************************************************************************
 *	All code below this point should be identical with that of
 *	the disassembler in gdmp960.

 A noble sentiment, but at least in cosmetic ways (info->fprintf_func), it
 just ain't so. -kingdon, 31 Mar 93
 *****************************************************************************/

struct tabent {
	char	*name;
	char	numops;
};

static int
pinsn( memaddr, word1, word2 )
    unsigned long memaddr;
    unsigned long word1, word2;
{
	int instr_len;

	instr_len = 4;
	put_abs( word1, word2 );

	/* Divide instruction set into classes based on high 4 bits of opcode*/
	switch ( (word1 >> 28) & 0xf ){
	case 0x0:
	case 0x1:
		ctrl( memaddr, word1 );
		break;
	case 0x2:
	case 0x3:
		cobr( memaddr, word1 );
		break;
	case 0x5:
	case 0x6:
	case 0x7:
		reg( word1 );
		break;
	case 0x8:
	case 0x9:
	case 0xa:
	case 0xb:
	case 0xc:
	case 0xd:
		instr_len = mem( memaddr, word1, word2, 0 );
		break;
	default:
		/* invalid instruction, print as data word */ 
		invalid( word1 );
		break;
	}
	return instr_len;
}

/****************************************/
/* CTRL format				*/
/****************************************/
static void
ctrl( memaddr, word1 )
    unsigned long memaddr;
    unsigned long word1;
{
	int i;
	static struct tabent ctrl_tab[] = {
		NULL,		0,	/* 0x00 */
		"syscall",	0,	/* 0x01 */	/* CA simulator only */
		NULL,		0,	/* 0x02 */
		NULL,		0,	/* 0x03 */
		NULL,		0,	/* 0x04 */
		NULL,		0,	/* 0x05 */
		NULL,		0,	/* 0x06 */
		NULL,		0,	/* 0x07 */
		"b",		1,	/* 0x08 */
		"call",		1,	/* 0x09 */
		"ret",		0,	/* 0x0a */
		"bal",		1,	/* 0x0b */
		NULL,		0,	/* 0x0c */
		NULL,		0,	/* 0x0d */
		NULL,		0,	/* 0x0e */
		NULL,		0,	/* 0x0f */
		"bno",		1,	/* 0x10 */
		"bg",		1,	/* 0x11 */
		"be",		1,	/* 0x12 */
		"bge",		1,	/* 0x13 */
		"bl",		1,	/* 0x14 */
		"bne",		1,	/* 0x15 */
		"ble",		1,	/* 0x16 */
		"bo",		1,	/* 0x17 */
		"faultno",	0,	/* 0x18 */
		"faultg",	0,	/* 0x19 */
		"faulte",	0,	/* 0x1a */
		"faultge",	0,	/* 0x1b */
		"faultl",	0,	/* 0x1c */
		"faultne",	0,	/* 0x1d */
		"faultle",	0,	/* 0x1e */
		"faulto",	0,	/* 0x1f */
	};

	i = (word1 >> 24) & 0xff;
	if ( (ctrl_tab[i].name == NULL) || ((word1 & 1) != 0) ){
		invalid( word1 );
		return;
	}

	(*info->fprintf_func) ( stream, ctrl_tab[i].name );
	if ( word1 & 2 ){		/* Predicts branch not taken */
		(*info->fprintf_func) ( stream, ".f" );
		word1 &= ~2;
	}

	if ( ctrl_tab[i].numops == 1 ){
		/* EXTRACT DISPLACEMENT AND CONVERT TO ADDRESS */
		word1 &= 0x00ffffff;
		if ( word1 & 0x00800000 ){		/* Sign bit is set */
			word1 |= (-1 & ~0xffffff);	/* Sign extend */
		}
		(*info->fprintf_func)( stream, "\t" );
		print_addr( word1 + memaddr );
	}
}

/****************************************/
/* COBR format				*/
/****************************************/
static void
cobr( memaddr, word1 )
    unsigned long memaddr;
    unsigned long word1;
{
	int src1;
	int src2;
	int i;

	static struct tabent cobr_tab[] = {
		"testno",	1,	/* 0x20 */
		"testg",	1,	/* 0x21 */
		"teste",	1,	/* 0x22 */
		"testge",	1,	/* 0x23 */
		"testl",	1,	/* 0x24 */
		"testne",	1,	/* 0x25 */
		"testle",	1,	/* 0x26 */
		"testo",	1,	/* 0x27 */
		NULL,		0,	/* 0x28 */
		NULL,		0,	/* 0x29 */
		NULL,		0,	/* 0x2a */
		NULL,		0,	/* 0x2b */
		NULL,		0,	/* 0x2c */
		NULL,		0,	/* 0x2d */
		NULL,		0,	/* 0x2e */
		NULL,		0,	/* 0x2f */
		"bbc",		3,	/* 0x30 */
		"cmpobg",	3,	/* 0x31 */
		"cmpobe",	3,	/* 0x32 */
		"cmpobge",	3,	/* 0x33 */
		"cmpobl",	3,	/* 0x34 */
		"cmpobne",	3,	/* 0x35 */
		"cmpoble",	3,	/* 0x36 */
		"bbs",		3,	/* 0x37 */
		"cmpibno",	3,	/* 0x38 */
		"cmpibg",	3,	/* 0x39 */
		"cmpibe",	3,	/* 0x3a */
		"cmpibge",	3,	/* 0x3b */
		"cmpibl",	3,	/* 0x3c */
		"cmpibne",	3,	/* 0x3d */
		"cmpible",	3,	/* 0x3e */
		"cmpibo",	3,	/* 0x3f */
	};

	i = ((word1 >> 24) & 0xff) - 0x20;
	if ( cobr_tab[i].name == NULL ){
		invalid( word1 );
		return;
	}

	(*info->fprintf_func) ( stream, cobr_tab[i].name );
	if ( word1 & 2 ){		/* Predicts branch not taken */
		(*info->fprintf_func) ( stream, ".f" );
		word1 &= ~2;
	}
	(*info->fprintf_func)( stream, "\t" );

	src1 = (word1 >> 19) & 0x1f;
	src2 = (word1 >> 14) & 0x1f;

	if ( word1 & 0x02000 ){		/* M1 is 1 */
		(*info->fprintf_func)( stream, "%d", src1 );
	} else {			/* M1 is 0 */
		(*info->fprintf_func)( stream, reg_names[src1] );
	}

	if ( cobr_tab[i].numops > 1 ){
		if ( word1 & 1 ){		/* S2 is 1 */
			(*info->fprintf_func)( stream, ",sf%d,", src2 );
		} else {			/* S1 is 0 */
			(*info->fprintf_func)( stream, ",%s,", reg_names[src2] );
		}

		/* Extract displacement and convert to address
		 */
		word1 &= 0x00001ffc;
		if ( word1 & 0x00001000 ){	/* Negative displacement */
			word1 |= (-1 & ~0x1fff);	/* Sign extend */
		}
		print_addr( memaddr + word1 );
	}
}

/****************************************/
/* MEM format				*/
/****************************************/
static int				/* returns instruction length: 4 or 8 */
mem( memaddr, word1, word2, noprint )
    unsigned long memaddr;
    unsigned long word1, word2;
    int noprint;		/* If TRUE, return instruction length, but
				 * don't output any text.
				 */
{
	int i, j;
	int len;
	int mode;
	int offset;
	const char *reg1, *reg2, *reg3;

	/* This lookup table is too sparse to make it worth typing in, but not
	 * so large as to make a sparse array necessary.  We allocate the table
	 * table at runtime, initialize all entries to empty, and copy the
	 * real ones in from an initialization table.
	 *
	 * NOTE: In this table, the meaning of 'numops' is:
	 *	 1: single operand
	 *	 2: 2 operands, load instruction
	 *	-2: 2 operands, store instruction
	 */
	static struct tabent *mem_tab = NULL;
/* Opcodes of 0x8X, 9X, aX, bX, and cX must be in the table.  */
#define MEM_MIN	0x80
#define MEM_MAX	0xd0
#define MEM_NEL (MEM_MAX-MEM_MIN+2)
#define MEM_SIZ	(MEM_NEL * sizeof(struct tabent))

	static struct { int opcode; char *name; char numops; } mem_init[] = {
		0x80,	"ldob",	 2,
		0x82,	"stob",	-2,
		0x84,	"bx",	 1,
		0x85,	"balx",	 2,
		0x86,	"callx", 1,
		0x88,	"ldos",	 2,
		0x8a,	"stos",	-2,
		0x8c,	"lda",	 2,
		0x90,	"ld",	 2,
		0x92,	"st",	-2,
		0x98,	"ldl",	 2,
		0x9a,	"stl",	-2,
		0x9d,   "dchint",1,	/* HX */
		0xa0,	"ldt",	 2,
		0xa2,	"stt",	-2,
		0xad,   "dcinva",1,	/* HX */
		0xb0,	"ldq",	 2,
		0xb2,	"stq",	-2,
		0xbd,   "dcflusha",1,	/* HX */
		0xc0,	"ldib",	 2,
		0xc2,	"stib",	-2,
		0xc8,	"ldis",	 2,
		0xca,	"stis",	-2,
		0xd0,	"icemark",2,	/* HX */
		0,	NULL,	0
	};

	if ( mem_tab == NULL ){
		mem_tab = (struct tabent *) xmalloc( MEM_SIZ );
		memset( (void *) mem_tab, 0, MEM_SIZ );
		for ( i = 0; mem_init[i].opcode != 0; i++ ){
			j = mem_init[i].opcode - MEM_MIN;
			mem_tab[j].name = mem_init[i].name;
			mem_tab[j].numops = mem_init[i].numops;
		}
	}

	i = ((word1 >> 24) & 0xff) - MEM_MIN;
	mode = (word1 >> 10) & 0xf;

	if ( i < MEM_NEL && mem_tab[i].name != NULL	/* Valid instruction */
	    &&  (mode == 5 || mode >=12) ){	/* With 32-bit displacement */
		len = 8;
	} else {
		len = 4;
	}

	if ( noprint ){
		return len;
	}

	if ( i >= MEM_NEL || mem_tab[i].name == NULL || mode == 6 ){
		invalid( word1 );
		return len;
	}

	(*info->fprintf_func)( stream, "%s\t", mem_tab[i].name );

	reg1 = reg_names[ (word1 >> 19) & 0x1f ];	/* MEMB only */
	reg2 = reg_names[ (word1 >> 14) & 0x1f ];
	reg3 = reg_names[ word1 & 0x1f ];		/* MEMB only */
	offset = word1 & 0xfff;				/* MEMA only  */

	switch ( mem_tab[i].numops ){

	case 2: /* LOAD INSTRUCTION */
		if ( mode & 4 ){			/* MEMB FORMAT */
			ea( memaddr, mode, reg2, reg3, word1, word2 );
			(*info->fprintf_func)( stream, ",%s", reg1 );
		} else {				/* MEMA FORMAT */
			(*info->fprintf_func)( stream, "0x%x", (unsigned) offset );
			if (mode & 8) {
				(*info->fprintf_func)( stream, "(%s)", reg2 );
			}
			(*info->fprintf_func)( stream, ",%s", reg1 );
		}
		break;

	case -2: /* STORE INSTRUCTION */
		if ( mode & 4 ){			/* MEMB FORMAT */
			(*info->fprintf_func)( stream, "%s,", reg1 );
			ea( memaddr, mode, reg2, reg3, word1, word2 );
		} else {				/* MEMA FORMAT */
			(*info->fprintf_func)( stream, "%s,0x%x", reg1, (unsigned) offset );
			if (mode & 8) {
				(*info->fprintf_func)( stream, "(%s)", reg2 );
			}
		}
		break;

	case 1: /* BX/CALLX INSTRUCTION */
		if ( mode & 4 ){			/* MEMB FORMAT */
			ea( memaddr, mode, reg2, reg3, word1, word2 );
		} else {				/* MEMA FORMAT */
			(*info->fprintf_func)( stream, "0x%x", (unsigned) offset );
			if (mode & 8) {
				(*info->fprintf_func)( stream, "(%s)", reg2 );
			}
		}
		break;
	}

	return len;
}

/****************************************/
/* REG format				*/
/****************************************/
static void
reg( word1 )
    unsigned long word1;
{
	int i, j;
	int opcode;
	int fp;
	int m1, m2, m3;
	int s1, s2;
	int src, src2, dst;
	int numops;
	char *mnemp;

	/* This lookup table is too sparse to make it worth typing in, but not
	 * so large as to make a sparse array necessary.  We allocate the
	 * table at runtime, initialize all entries to empty, and copy the
	 * real ones in from an initialization table.
	 *
	 * NOTE: In this table, the meaning of 'numops' is:
	 *	 1: single operand, which is NOT a destination.
	 *	-1: single operand, which IS a destination.
	 *	 2: 2 operands, the 2nd of which is NOT a destination.
	 *	-2: 2 operands, the 2nd of which IS a destination.
	 *	 3: 3 operands
	 *	-3: 3 operands, the 3rd of which is NOT a destination.
	 *
	 *	If an opcode mnemonic begins with "F", it is a floating-point
	 *	opcode (the "F" is not printed).
	 */

	static struct tabent *reg_tab = NULL;
	static struct { int opcode; char *name; char numops; } reg_init[] = {
#define REG_MIN	0x580
		0x580,	"notbit",	3,
		0x581,	"and",		3,
		0x582,	"andnot",	3,
		0x583,	"setbit",	3,
		0x584,	"notand",	3,
		0x586,	"xor",		3,
		0x587,	"or",		3,
		0x588,	"nor",		3,
		0x589,	"xnor",		3,
		0x58a,	"not",		-2,
		0x58b,	"ornot",	3,
		0x58c,	"clrbit",	3,
		0x58d,	"notor",	3,
		0x58e,	"nand",		3,
		0x58f,	"alterbit",	3,
		0x590, 	"addo",		3,
		0x591, 	"addi",		3,
		0x592, 	"subo",		3,
		0x593, 	"subi",		3,
		0x594,	"cmpob",	2,	/* JX */
		0x595,	"cmpib",	2,	/* JX */
		0x596,	"cmpos",	2,	/* JX */
		0x597,	"cmpis",	2,	/* JX */
		0x598, 	"shro",		3,
		0x59a, 	"shrdi",	3,
		0x59b, 	"shri",		3,
		0x59c, 	"shlo",		3,
		0x59d, 	"rotate",	3,
		0x59e, 	"shli",		3,
		0x5a0, 	"cmpo",		2,
		0x5a1, 	"cmpi",		2,
		0x5a2, 	"concmpo",	2,
		0x5a3, 	"concmpi",	2,
		0x5a4, 	"cmpinco",	3,
		0x5a5, 	"cmpinci",	3,
		0x5a6, 	"cmpdeco",	3,
		0x5a7, 	"cmpdeci",	3,
		0x5ac, 	"scanbyte",	2,
		0x5ad,	"bswap",	-2,	/* JX */
		0x5ae, 	"chkbit",	2,
		0x5b0, 	"addc",		3,
		0x5b2, 	"subc",		3,
		0x5b4,	"intdis",	0,	/* JX */
		0x5b5,	"inten",	0,	/* JX */
		0x5cc,	"mov",		-2,
		0x5d8,	"eshro",	3,
		0x5dc,	"movl",		-2,
		0x5ec,	"movt",		-2,
		0x5fc,	"movq",		-2,
		0x600,	"synmov",	2,
		0x601,	"synmovl",	2,
		0x602,	"synmovq",	2,
		0x610,	"atmod",	3,
		0x612,	"atadd",	3,
		0x615,	"synld",	-2,
		0x630,	"sdma",		3,
		0x631,	"udma",		0,
		0x640,	"spanbit",	-2,
		0x641,	"scanbit",	-2,
		0x642,	"daddc",	3,
		0x643,	"dsubc",	3,
		0x644,	"dmovt",	-2,
		0x645,	"modac",	3,
		0x650,	"modify",	3,
		0x651,	"extract",	3,
		0x654,	"modtc",	3,
		0x655,	"modpc",	3,
		0x658,	"intctl",	-2,      /* JX */
		0x659,	"sysctl",	3,
		0x65b,	"icctl",	3,	/* JX */
		0x65c,	"dcctl",	3,	/* JX */
		0x65d,	"halt",		1,	/* JX */
		0x660,	"calls",	1,
		0x66b,	"mark",		0,
		0x66c,	"fmark",	0,
		0x66d,	"flushreg",	0,
		0x66f,	"syncf",	0,
		0x670,	"emul",		3,
		0x671,	"ediv",		3,
		0x672,	"cvtadr",	-2,
		0x674,	"Fcvtir",	-2,
		0x675,	"Fcvtilr",	-2,
		0x676,	"Fscalerl",	3,
		0x677,	"Fscaler",	3,
		0x680,	"Fatanr",	3,
		0x681,	"Flogepr",	3,
		0x682,	"Flogr",	3,
		0x683,	"Fremr",	3,
		0x684,	"Fcmpor",	2,
		0x685,	"Fcmpr",	2,
		0x688,	"Fsqrtr",	-2,
		0x689,	"Fexpr",	-2,
		0x68a,	"Flogbnr",	-2,
		0x68b,	"Froundr",	-2,
		0x68c,	"Fsinr",	-2,
		0x68d,	"Fcosr",	-2,
		0x68e,	"Ftanr",	-2,
		0x68f,	"Fclassr",	1,
		0x690,	"Fatanrl",	3,
		0x691,	"Flogeprl",	3,
		0x692,	"Flogrl",	3,
		0x693,	"Fremrl",	3,
		0x694,	"Fcmporl",	2,
		0x695,	"Fcmprl",	2,
		0x698,	"Fsqrtrl",	-2,
		0x699,	"Fexprl",	-2,
		0x69a,	"Flogbnrl",	-2,
		0x69b,	"Froundrl",	-2,
		0x69c,	"Fsinrl",	-2,
		0x69d,	"Fcosrl",	-2,
		0x69e,	"Ftanrl",	-2,
		0x69f,	"Fclassrl",	1,
		0x6c0,	"Fcvtri",	-2,
		0x6c1,	"Fcvtril",	-2,
		0x6c2,	"Fcvtzri",	-2,
		0x6c3,	"Fcvtzril",	-2,
		0x6c9,	"Fmovr",	-2,
		0x6d9,	"Fmovrl",	-2,
		0x6e1, 	"Fmovre",	-2,
		0x6e2, 	"Fcpysre",	3,
		0x6e3, 	"Fcpyrsre",	3,
		0x701,	"mulo",		3,
		0x703,	"cpmulo",	3,	/* JL */
		0x708,	"remo",		3,
		0x70b,	"divo",		3,
		0x741,	"muli",		3,
		0x748,	"remi",		3,
		0x749,	"modi",		3,
		0x74b,	"divi",		3,
		0x780,	"addono",	3,	/* JX */
		0x781,	"addino",	3,	/* JX */
		0x782,	"subono",	3,	/* JX */
		0x783,	"subino",	3,	/* JX */
		0x784,	"selno",	3,	/* JX */
		0x78b,	"Fdivr",	3,
		0x78c,	"Fmulr",	3,
		0x78d,	"Fsubr",	3,
		0x78f,	"Faddr",	3,
		0x790,	"addog",	3,	/* JX */
		0x791,	"addig",	3,	/* JX */
		0x792,	"subog",	3,	/* JX */
		0x794,	"selg",		3,	/* JX */
		0x793,	"subig",	3,	/* JX */
		0x79b,	"Fdivrl",	3,
		0x79c,	"Fmulrl",	3,
		0x79d,	"Fsubrl",	3,
		0x79f,	"Faddrl",	3,
		0x7a0,	"addoe",	3,	/* JX */
		0x7a1,	"addie",	3,	/* JX */
		0x7a2,	"suboe",	3,	/* JX */
		0x7a3,	"subie",	3,	/* JX */
		0x7a4,	"sele",		3,	/* JX */
		0x7b0,	"addoge",	3,	/* JX */
		0x7b1,	"addige",	3,	/* JX */
		0x7b2,	"suboge",	3,	/* JX */
		0x7b3,	"subige",	3,	/* JX */
		0x7b4,	"selge",	3,	/* JX */
		0x7c0,	"addol",	3,	/* JX */
		0x7c1,	"addil",	3,	/* JX */
		0x7c2,	"subol",	3,	/* JX */
		0x7c3,	"subil",	3,	/* JX */
		0x7c4,	"sell",		3,	/* JX */
		0x7d0,	"addone",	3,	/* JX */
		0x7d1,	"addine",	3,	/* JX */
		0x7d2,	"subone",	3,	/* JX */
		0x7d3,	"subine",	3,	/* JX */
		0x7d4,	"selne",	3,	/* JX */
		0x7e0,	"addole",	3,	/* JX */
		0x7e1,	"addile",	3,	/* JX */
		0x7e2,	"subole",	3,	/* JX */
		0x7e3,	"subile",	3,	/* JX */
		0x7e4,	"selle",	3,	/* JX */
		0x7f0,	"addoo",	3,	/* JX */
		0x7f1,	"addio",	3,	/* JX */
		0x7f2,	"suboo",	3,	/* JX */
		0x7f3,	"subio",	3,	/* JX */
		0x7f4,	"selo",		3,	/* JX */

#define REG_MAX	0x7f4
#define REG_SIZ	((REG_MAX-REG_MIN+1) * sizeof(struct tabent))
		0,	NULL,	0
	};

	if ( reg_tab == NULL ){
		reg_tab = (struct tabent *) xmalloc( REG_SIZ );
		memset( (void *) reg_tab, 0, REG_SIZ );
		for ( i = 0; reg_init[i].opcode != 0; i++ ){
			j = reg_init[i].opcode - REG_MIN;
			reg_tab[j].name = reg_init[i].name;
			reg_tab[j].numops = reg_init[i].numops;
		}
	}

	opcode = ((word1 >> 20) & 0xff0) | ((word1 >> 7) & 0xf);
	i = opcode - REG_MIN;

	s1   = (word1 >> 5)  & 1;
	s2   = (word1 >> 6)  & 1;
	m1   = (word1 >> 11) & 1;
	m2   = (word1 >> 12) & 1;
	m3   = (word1 >> 13) & 1;
	src  =  word1        & 0x1f;
	src2 = (word1 >> 14) & 0x1f;
	dst  = (word1 >> 19) & 0x1f;

	/* coprocessor instructions need special support, check for them
	 * here, and do whats necessary. */
	if ((opcode >= 0x500 && opcode <= 0x57f) ||
	    (opcode >= 0x710 && opcode <= 0x73f) ||
	    (opcode >= 0x750 && opcode <= 0x77f))
	{
		if (m3)
			fprintf (stream, "cpdcp\t0x%x,", opcode);
		else
			fprintf (stream, "cpd960\t0x%x,", opcode);
		numops = -3;
		fp = 0;
	}
	else
	{
		if ( (opcode<REG_MIN) || (opcode>REG_MAX) ||
		     (reg_tab[i].name==NULL) )
		{
			invalid( word1 );
			return;
		}

		mnemp = reg_tab[i].name;
		if ( *mnemp == 'F' )
		{
			fp = 1;
			mnemp++;
		}
		else
		{
			fp = 0;
		}

		(*info->fprintf_func)( stream, mnemp );

		numops = reg_tab[i].numops;
		if (numops != 0)
			(*info->fprintf_func)( stream, "\t" );
	}


	if  ( numops != 0 )
	{
		switch ( numops )
		{
		case 1:
			regop( m1, s1, src, fp );
			break;
		case -1:
			dstop( m3, dst, fp );
			break;
		case 2:
			regop( m1, s1, src, fp );
			(*info->fprintf_func)( stream, "," );
			regop( m2, s2, src2, fp );
			break;
		case -2:
			regop( m1, s1, src, fp );
			(*info->fprintf_func)( stream, "," );
			dstop( m3, dst, fp );
			break;
		case 3:
			regop( m1, s1, src, fp );
			(*info->fprintf_func)( stream, "," );
			regop( m2, s2, src2, fp );
			(*info->fprintf_func)( stream, "," );
			dstop( m3, dst, fp );
			break;
		case -3:
			regop( m1, s1, src, fp );
			(*info->fprintf_func)( stream, "," );
			regop( m2, s2, src2, fp );
			(*info->fprintf_func)( stream, "," );
			regop( m3, 0,  dst, fp );
			break;
		}
	}
}


/*
 * Print out effective address for memb instructions.
 */
static void
ea( memaddr, mode, reg2, reg3, word1, word2 )
    CORE_ADDR memaddr;
    int mode;
    const char *reg2, *reg3;
    unsigned long word1;
    unsigned long word2;
{
	int scale;
	static int scale_tab[] = { 1, 2, 4, 8, 16 };

	scale = (word1 >> 7) & 0x07;
	if ( (scale > 4) || ((word1 >> 5) & 0x03 != 0) ){
		invalid( (int) word1 );
		return;
	}
	scale = scale_tab[scale];

	switch (mode) {
	case 4:	 					/* (reg) */
		(*info->fprintf_func)( stream, "(%s)", reg2 );
		break;
	case 5:						/* displ+8(ip) */
		(*info->fprintf_func)( stream, "0x%x(ip)", word2 );
		break;
	case 7:						/* (reg)[index*scale] */
		if (scale == 1) {
			(*info->fprintf_func)( stream, "(%s)[%s]", reg2, reg3 );
		} else {
			(*info->fprintf_func)( stream, "(%s)[%s*%d]",reg2,reg3,scale);
		}
		break;
	case 12:					/* displacement */
		print_addr( word2 );
		break;
	case 13:					/* displ(reg) */
		print_addr( word2 );
		(*info->fprintf_func)( stream, "(%s)", reg2 );
		break;
	case 14:					/* displ[index*scale] */
		print_addr( word2 );
		if (scale == 1) {
			(*info->fprintf_func)( stream, "[%s]", reg3 );
		} else {
			(*info->fprintf_func)( stream, "[%s*%d]", reg3, scale );
		}
		break;
	case 15:				/* displ(reg)[index*scale] */
		print_addr( word2 );
		if (scale == 1) {
			(*info->fprintf_func)( stream, "(%s)[%s]", reg2, reg3 );
		} else {
			(*info->fprintf_func)( stream, "(%s)[%s*%d]",reg2,reg3,scale );
		}
		break;
	default:
		invalid( (int) word1 );
		return;
	}
}


/************************************************/
/* Register Instruction Operand  		*/
/************************************************/
static void
regop( mode, spec, reg, fp )
    int mode, spec, reg, fp;
{
	if ( fp ){				/* FLOATING POINT INSTRUCTION */
		if ( mode == 1 ){			/* FP operand */
			switch ( reg ){
			case 0:  (*info->fprintf_func)( stream, "fp0" );
			  break;
			case 1:  (*info->fprintf_func)( stream, "fp1" );
			  break;
			case 2:  (*info->fprintf_func)( stream, "fp2" );
			  break;
			case 3:  (*info->fprintf_func)( stream, "fp3" );
			  break;
			case 16: (*info->fprintf_func)( stream, "0f0.0" );
			  break;
			case 22: (*info->fprintf_func)( stream, "0f1.0" );
			  break;
			default: (*info->fprintf_func)( stream, "?" );
			  break;
			}
		} else {				/* Non-FP register */
			(*info->fprintf_func)( stream, reg_names[reg] );
		}
	} else {				/* NOT FLOATING POINT */
		if ( mode == 1 ){			/* Literal */
			(*info->fprintf_func)( stream, "%d", reg );
		} else {				/* Register */
			if ( spec == 0 ){
				(*info->fprintf_func)( stream, reg_names[reg] );
			} else {
				(*info->fprintf_func)( stream, "sf%d", reg );
			}
		}
	}
}

/************************************************/
/* Register Instruction Destination Operand	*/
/************************************************/
static void
dstop( mode, reg, fp )
    int mode, reg, fp;
{
	/* 'dst' operand can't be a literal. On non-FP instructions,  register
	 * mode is assumed and "m3" acts as if were "s3";  on FP-instructions,
	 * sf registers are not allowed so m3 acts normally.
	 */
	 if ( fp ){
		regop( mode, 0, reg, fp );
	 } else {
		regop( 0, mode, reg, fp );
	 }
}


static void
invalid( word1 )
    int word1;
{
	(*info->fprintf_func)( stream, ".word\t0x%08x", (unsigned) word1 );
}	

static void
print_addr(a)
int a;
{
  (*info->print_address_func) ((bfd_vma) a, info);
}

static void
put_abs( word1, word2 )
    unsigned long word1, word2;
{
#ifdef IN_GDB
	return;
#else
	int len;

	switch ( (word1 >> 28) & 0xf ){
	case 0x8:
	case 0x9:
	case 0xa:
	case 0xb:
	case 0xc:
		/* MEM format instruction */
		len = mem( 0, word1, word2, 1 );
		break;
	default:
		len = 4;
		break;
	}

	if ( len == 8 ){
		(*info->fprintf_func)( stream, "%08x %08x\t", word1, word2 );
	} else {
		(*info->fprintf_func)( stream, "%08x         \t", word1 );
	}
;

#endif
}
