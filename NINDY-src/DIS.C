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
#include "regs.h"

/************************************************/
/*	LOOKUP TABLE FOR OPCODES		*/
/* This table is rather confusing at first 	*/
/* glance.  Given the embedded nature of NINDY, */
/* the table is designed to pack the op's in as */
/* tight as possible in a somewhat logical 	*/
/* order.  The opcodes are inserted into the 	*/
/* table in their respective places where 	*/
/* possible.  All of the two digit opcodes are  */
/* just a straight lookup.  The three digit 	*/
/* opcodes often must be inserted elsewhere into*/
/* the table where there is room.  Their new	*/
/* location is pointed to by the first two 	*/
/* digit's straight lookup.  It then turns into */
/* somewhat of a hash table to find the correct */
/* opcode from there.  Believe it or not it all	*/
/* works out fine.  It just takes some time for	*/
/* the programmer to get it all sorted out.	*/
/************************************************/
static const struct { short opcode; char *name; short numops; }
instr[] = {

	0,	NULL,		0,	/* --------00-------- */
	0x670,	"emul",		3,
	0x671,	"ediv",		3,
	0x673, 	"ldtime",	1,
	0x674,	"cvtir",	2,
	0x675,	"cvtilr",	2,
	0x676,	"scalerl",	3,
	0x677,	"scaler",	3,
	0x08,	"b",		1,
	0x09,	"call",		1,
	0x0a,	"ret",		0,
	0x0b,	"bal",		1,
	0x741,	"muli",		3,
	0x748,	"remi",		3,
	0x749,	"modi",		3,
	0x74b,	"divi",		3,	/* ________0F________ */
	0x10,	"bno",		1,	/* --------10-------- */
	0x11,	"bg",		1,
	0x12,	"be",		1,
	0x13,	"bge",		1,
	0x14,	"bl",		1,
	0x15,	"bne",		1,
	0x16,	"ble",		1,
	0x17,	"bo",		1,
	0x18,	"faultno",	0,
	0x19,	"faultg",	0,
	0x1a,	"faulte",	0,
	0x1b,	"faultge",	0,
	0x1c,	"faultl",	0,
	0x1d,	"faultne",	0,
	0x1e,	"faultle",	0,
	0x1f,	"faulto",	0,	/* ________1F________ */
	0x20,	"testno",	1,	/* --------20-------- */
	0x21,	"testg",	1,
	0x22,	"teste",	1,
	0x23,	"testge",	1,
	0x24,	"testl",	1,
	0x25,	"testne",	1,
	0x26,	"testle",	1,
	0x27,	"testo",	1,
	0x640,	"spanbit",	2,
	0x641,	"scanbit",	2,
	0x642,	"daddc",	3,
	0x643,	"dsubc",	3,
	0x644,	"dmovt",	2,
	0x645,	"modac",	3,
	0x646,	"condrec",	2,
	0,	NULL,		0,	/* ________2F________ */
	0x30,	"bbc",		3,	/* --------30-------- */
	0x31,	"cmpobg",	3,
	0x32,	"cmpobe",	3,
	0x33,	"cmpobge",	3,
	0x34,	"cmpobl",	3,
	0x35,	"cmpobne",	3,
	0x36,	"cmpoble",	3,
	0x37,	"bbs",		3,
	0x38,	"cmpibno",	3,
	0x39,	"cmpibg",	3,
	0x3a,	"cmpibe",	3,
	0x3b,	"cmpibge",	3,
	0x3c,	"cmpibl",	3,
	0x3d,	"cmpibne",	3,
	0x3e,	"cmpible",	3,
	0x3f,	"cmpibo",	3,	/* ________3F________ */
	0x690,	"atanrl",	3,	/* --------40-------- */
	0x691,	"logeprl",	3,
	0x692,	"logrl",	3,
	0x693,	"remrl",	3,
	0x694,	"cmporl",	2,
	0x695,	"cmprl",	2,
	0,	NULL,		0,
	0,	NULL,		0,
	0x698,	"sqrtrl",	2,
	0x699,	"exprl",	2,
	0x69a,	"logbnrl",	2,
	0x69b,	"roundrl",	2,
	0x69c,	"sinrl",	2,
	0x69d,	"cosrl",	2,
	0x69e,	"tanrl",	2,
	0x69f,	"classrl",	1,	/* ________4F________ */
	0x610,	"atmod",	3,	/* --------50-------- */
	0x612,	"atadd",	3,
	0x613,	"inspacc",	2,
	0x614,	"ldphy",	2,
	0x615,	"synld",	2,
	0x617,	"fill",		3,
	0,	NULL,		0,
	0,	NULL,		0,
	0,	(char*)0xd0,	0,
	0,	(char*)0xb3,	0,
	0,	(char*)0xa3,	0,
	0,	(char*)0xbd,	0,
	0x5cc,	"mov",		2,
	0x5dc,	"movl",		2,
	0x5ec,	"movt",		2,
	0x5fc,	"movq",		2,	/* ________5F________ */
	0,	(char*)0x7a,	0,	/* --------60-------- */
	0,	(char*)0x50,	0,
	0,	NULL,		0,
	0,	(char*)0x71,	0,
	0,	(char*)0x28,	0,
	0,	(char*)0xff,	0,
	0,	(char*)0xf0,	0,
	0,	(char*)0x01,	0,
	0,	(char*)0xe0,	0,
	0,	(char*)0x40,	0,
	0,	NULL,		0,
	0,	NULL,		0,
	0,	(char*)0xc3,	0,
	0x6d9,	"movrl",	2,
	0,	(char*)0xad,	0,
	0,	NULL,		0,	/* ________6F________ */
	0,	(char*)0x8d,	0,	/* --------70-------- */
	0x630,	"sdma",		3,
	0x631,	"udma",		0,
	0,	NULL,		0,
	0,	(char*)0x0c,	0,
	0,	NULL,		0,
	0,	NULL,		0,
	0,	NULL,		0,
	0, 	(char*)0x93,	0,
	0,	(char*)0xcb,	0,
	0x600,	"synmov",	2,
	0x601,	"synmovl",	2,
	0x602,	"synmovq",	2,
	0x603,	"cmpstr",	3,
	0x604,	"movqstr",	3,
	0x605,	"movstr",	3,	/* ________7F________ */
	0x80,	"ldob",		2,	/* --------80-------- */
	0,	NULL,		0,
	0x82,	"stob",		2,
	0,	NULL,		0,
	0x84,	"bx",		1,
	0x85,	"balx",		2,
	0x86,	"callx",	1,
	0,	NULL,		0,
	0x88,	"ldos",		2,
	0,	NULL,		0,
	0x8a,	"stos",		2,
	0,	NULL,		0,
	0x8c,	"lda",		2,
	0x701,	"mulo",		3,
	0x708,	"remo",		3,
	0x70b,	"divo",		3,	/* ________8F________ */
	0x90,	"ld",		2,	/* --------90-------- */
	0,	NULL,		0,
	0x92,	"st",		2,
	0x78b,	"divr",		3,
	0x78c,	"mulr",		3,
	0x78d,	"subr",		3,
	0x78f,	"addr",		3,
	0,	NULL,		0,
	0x98,	"ldl",		2,
	0,	NULL,		0,
	0x9a,	"stl",		2,
	0,	NULL,		0,
	0,	NULL,		0,
	0,	NULL,		0,
	0,	NULL,		0,
	0,	NULL,		0,	/* ________9F________ */
	0xa0,	"ldt",		2,	/* --------A0-------- */
	0,	NULL,		0,
	0xa2,	"stt",		2,
	0x5a0, 	"cmpo",		2,
	0x5a1, 	"cmpi",		2,
	0x5a2, 	"concmpo",	2,
	0x5a3, 	"concmpi",	2,
	0x5a4, 	"cmpinco",	3,
	0x5a5, 	"cmpinci",	3,
	0x5a6, 	"cmpdeco",	3,
	0x5a7, 	"cmpdeci",	3,
	0x5ac, 	"scanbyte",	2,
	0x5ae, 	"chkbit",	2,
	0x6e2, 	"cpysre",	3,
	0x6e3, 	"cpyrsre",	3,
	0x6e1, 	"movre",	2,	/* ________AF________ */
	0xb0,	"ldq",		2,	/* --------B0-------- */
	0,	NULL,		0,
	0xb2,	"stq",		2,
	0x590, 	"addo",		3,
	0x591, 	"addi",		3,
	0x592, 	"subo",		3,
	0x593, 	"subi",		3,
	0x598, 	"shro",		3,
	0x59a, 	"shrdi",	3,
	0x59b, 	"shri",		3,
	0x59c, 	"shlo",		3,
	0x59d, 	"rotate",	3,
	0x59e, 	"shli",		3,
	0x5b0, 	"addc",		3,
	0x5b2, 	"subc",		3,
	0, 	NULL,		0,	/* ________BF________ */
	0xc0,	"ldib",		2,	/* --------C0-------- */
	0,	NULL,		0,
	0xc2,	"stib",		2,
	0x6c0,	"cvtri",	2,
	0x6c1,	"cvtril",	2,
	0x6c2,	"cvtzri",	2,
	0x6c3,	"cvtzril",	2,
	0x6c9,	"movr",		2,
	0xc8,	"ldis",		2,
	0,	NULL,		0,
	0xca,	"stis",		2,
	0x79b,	"divrl",	3,
	0x79c,	"mulrl",	3,
	0x79d,	"subrl",	3,
	0,	NULL,		0,
	0x79f,	"addrl",	3,	/* ________CF________ */
	0x580,	"notbit",	3,	/* --------D0-------- */
	0x581,	"and",		3,
	0x582,	"andnot",	3,
	0x583,	"setbit",	3,
	0x584,	"notand",	3,
	0,	NULL,		0,
	0x586,	"xor",		3,
	0x587,	"or",		3,
	0x588,	"nor",		3,
	0x589,	"xnor",		3,
	0x58a,	"not",		2,
	0x58b,	"ornot",	3,
	0x58c,	"clrbit",	3,
	0x58d,	"notor",	3,
	0x58e,	"nand",		3,
	0x58f,	"alterbit",	3,	/* ________DF________ */
	0x680,	"atanr",	3,	/* --------E0-------- */
	0x681,	"logepr",	3,
	0x682,	"logr",		3,
	0x683,	"remr",		3,
	0x684,	"cmpor",	2,
	0x685,	"cmpr",		2,
	0,	NULL,		0,
	0,	NULL,		0,
	0x688,	"sqrtr",	2,
	0x689,	"expr",		2,
	0x68a,	"logbnr",	2,
	0x68b,	"roundr",	2,
	0x68c,	"sinr",		2,
	0x68d,	"cosr",		2,
	0x68e,	"tanr",		2,
	0x68f,	"classr",	1,	/* ________EF________ */
	0x660,	"calls",	1,	/* --------F0-------- */
	0,	NULL,		0,
	0x662,	"send",		3,
	0x663,	"sendserv",	1,
	0x664,	"resumprcs",	1,
	0x665,	"schedprcs",	1,
	0x666,	"saveprcs",	0,
	0,	NULL,		0,
	0x668,	"condwait",	1,
	0x669,	"wait",		1,
	0x66a,	"signal",	1,
	0x66b,	"mark",		0,
	0x66c,	"fmark",	0,
	0x66d,	"flushreg",	0,
	0,	NULL,		0,
	0x66f,	"syncf",	0,	/* ________FF________ */
	0x650,	"modify",	3,
	0x651,	"extract",	3,
	0x654,	"modtc",	3,
	0x655,	"modpc",	3,
	0x656,	"receive",	2,
	0x659,	"sysctl",	3,
};

static const char
regs[][4] = {
	"pfp", "sp", "rip", "r3",  "r4",  "r5",  "r6",  "r7",
	"r8",  "r9", "r10", "r11", "r12", "r13", "r14", "r15",
	"g0",  "g1", "g2",  "g3",  "g4",  "g5",  "g6",  "g7",
	"g8",  "g9", "g10", "g11", "g12", "g13", "g14", "fp"
};

static char *fregs[] = { "fp0", "fp1", "fp2", "fp3", "0.0", "1.0" };
static char tab[] = "           ";

static unsigned int *ip;


/************************************************/
/* Disassemble A Word in Memory        		*/
/*                           			*/
/************************************************/
dasm( dummy, nargs, addr, cnt )
int dummy;	/* Ignored */
int nargs;	/* Number of following arguments that are valid (0,1, or 2) */
int addr;	/* Optional address of instruction (defaults to ip)	*/
int cnt;	/* Optional number of instructions (defaults to 1)	*/
{
	/* Set defaults */
	if ( nargs <= 1 ){
		cnt = 1;
		if ( nargs == 0 ){
			addr = register_set[REG_IP];
		}
	}

	ip = (unsigned int *)addr;
	while ( cnt-- ) {
		prtf("\n%X : %X", ip, *ip);
		dis();
		ip++;
	}
}

/****************************************/
/* Disassemble Instructions		*/
/****************************************/
dis()
{
unsigned int instruction; 	/* 32 bit machine instruction */
unsigned int opcode; 	

	instruction = *ip;
	opcode = instruction >> 24;

	/* check for valid opcode */
	if (instr[opcode].opcode != 0) {
		if (opcode < 0x20) {
			prtf (tab);
			ctrl(instruction, opcode);
		}
		else if (opcode < 0x40) {
			prtf (tab);
			cobr(instruction, opcode);
		}
		else if (opcode > 0xCA) {
			prtf (tab);
			invalid(instruction);
		}
		else if (opcode >= 0x80) {
			mem(instruction, opcode);
		}
		else {
			prtf (tab);
			reg(instruction, opcode);
		}
	}

	/* still may be valid opcode, but of REG format */
	else {
		/* valid REG opcode */
		if ( instr[opcode].name != NULL ) {
			prtf (tab);
			reg(instruction, opcode);
		}
	
		/* invalid instruction, print as data word */ 
		else {
			prtf (tab);
			invalid (instruction);
		}
	}
}

/****************************************/
/* CTRL format				*/
/****************************************/
ctrl(instruction, op)
unsigned int instruction;
int op;
{
int displace;

	if ( (instruction & 0x1) && (instr[op].opcode == op) ){
		invalid(instruction);
	} else {
		prtf( "%s", instr[op].name );
 		if (instruction & 0x3)
                        prtf (".f  ");
                else
                        prtf ("  ");

		if ((op < 0x18) && (strcmp(instr[op].name, "ret"))) {
			displace = (int)(instruction & 0xfffffc);
			if (displace & 0x800000) { /* sign bit is set */
				/* sign extend displacement */
				displace = (-1 & ~0x00fffffc) | displace;
			}
			displace += (int)ip;
			prtf ("0x%X",displace);
		}
	}
}

/****************************************/
/* COBR format				*/
/****************************************/
cobr(instruction, op)
unsigned int instruction;
int op;
{
int displace, dst;

	if ( (instruction & 0x1) && (instr[op].opcode == op) ){
		invalid (instruction);
		return;
	}
	prtf ("%s", instr[op].name);

 	if (instruction & 0x3)
                prtf (".f  ");
        else
                prtf ("  ");

	dst = (instruction >> 19) & 0x1f;

	if (op >= 0x30) {
		if ( instruction & 0x2000 ) {
			prtf ("0x%b,  ",dst);
		} else {
			prtf ("%s,   ", regs[dst]);
		}
		
		prtf ("%s,  ", regs[(instruction>>14) & 0x1f] );

		displace = (int)(instruction & 0x1ffc);
		if (displace & 0x1000) { /* negative displacement */
			/* sign extend displacement */
			displace = (-1 & ~0x1ffc) | displace;
		}
		prtf ( "0x%X", displace + (int)ip );
	}
	else {
		prtf (regs[dst]);
	}
}

/****************************************/
/* MEM format				*/
/****************************************/
mem(instruction, op)
unsigned int instruction;
int op;
{
const char *reg1, *reg2;

	reg1 = regs[ (instruction>>19) & 0x1f ];
	reg2 = regs[ (instruction>>14) & 0x1f ];

	if  ( instruction & 0x1000 ){
		memb(instruction, op, reg1, reg2);
	} else { 			/* MEMB format */
		mema(instruction, op, reg1, reg2);
	}
}

/****************************************/
/* REG format				*/
/****************************************/
reg(instruction, op)
unsigned int instruction;
int op;
{
int index, mode1, mode2, mode3, fp;
int src, dst, src2, tmp, count, check, valid;

	check = (instruction >> 5) & 0x03;
	if (check != 0) {
		invalid (instruction);
		return;
	}

	if ((instr[op].opcode == 0x6d9) || ((instr[op].opcode < 0x5ff) 
	   && (instr[op].opcode > 0x5c0))) {
		index = op;
		op = (op << 4) + ((instruction >> 7) & 0x0f);
		if (instr[index].opcode != op) {
			invalid (instruction);
			return;
		}
	}
	else {
		index = (int) instr[op].name;
		op = (op << 4) + ((instruction >> 7) & 0x0f);
		count = 0;
		while (instr[index].opcode != op) {
			if (count++ > 15) {
				invalid (instruction);
				return;
			}
			index++;
		}
	}
	prtf ("%s  ", instr[index].name );

	mode1 = (instruction >> 11) & 0x01;
	mode2 = (instruction >> 12) & 0x01;
	mode3 = (instruction >> 13) & 0x01;
	src = instruction & 0x1f;
	src2 = (instruction >> 14) & 0x1f;
	dst = (instruction >> 19) & 0x1f;
	if ((op >= 0x78b) || ((op < 0x6e9) && (op >= 0x674))) 
		fp = TRUE;
	else
		fp = FALSE;

	switch ( instr[index].numops ){
		case 1:
			if (op == 0x673) {
				mode1 = mode3;
				src = dst;
			}
			regop (mode1, src, fp);
			break;
		case 2:
			if ( ((op >= 0x5a0) && (op <= 0x5ae))
			||   ((op >= 0x600) && (op <= 0x602))
			||   (op == 0x684) || (op == 0x685)
			||   (op == 0x694) || (op == 0x695) ) {
				dst = src2;
				mode3 = mode2;
			}
			regop(mode1,src,fp);
			prtf (",  ");
			regop(mode3,dst,fp);
			break;
		case 3:
			regop(mode1,src,fp);
			prtf (",  ");
			regop(mode2,src2,fp);
			prtf (",  ");
			regop(mode3,dst,fp);
			break;
	}
}

/************************************************/
/* Print out effective address			*/
/*                           			*/
/************************************************/
ea(mode, reg2, reg3, word2, scale)
int mode;
char *reg2, *reg3;
unsigned int word2;
int scale;
{
	switch (mode) {
		case 4:	 		/* (reg) */
			prtf ("(%s)",reg2);
			break;
		case 5:			/* displ+8(ip) */
			word2 += 8;
			prtf ("0x%x(0x%x)", word2, ip);
			break;
		case 7:			/* (reg)[index*scale] */
			if (scale == 1) {
				prtf ("(%s)[%s]",reg2,reg3);
			} else {
				prtf ("(%s)[%s*%b]",reg2,reg3,scale);
			}
			break;
		case 12:		/* displacement */
			prtf ("0x%X",word2);
			break;
		case 13:		/* displ(reg) */
			prtf ("0x%X(%s)",word2,reg2);
			break;
		case 14:		/* displ[index*scale] */
			if (scale == 1) {
				prtf ("0x%x[%s]",word2,reg3);
			} else {
				prtf ("0x%x[%s*%b]",word2,reg3,scale);
			}
			break;
		case 15:		/* displ(reg)[index*scale] */
			prtf ("0x%x(%s)[%s",word2,reg2,reg3);
			if (scale != 1) {
				prtf ("*%b",scale);
			}
			prtf("]");
			break;
		default:
			invalid (*ip);
			return;
	}
}

/************************************************/
/* MEMA instruction type      			*/
/*                           			*/
/************************************************/
mema (instruction, op, reg1, reg2)
unsigned int instruction;
int op;
char *reg1, *reg2;
{
int mode, offset;

	prtf (tab);
	if (instr[op].opcode != op) {
		invalid (instruction);
		return;
	}
	prtf ("%s  ",instr[op].name);
	mode = instruction & 0x2000;
	offset = instruction & 0xfff;

	switch ( op & 0x0f ){
	case 0:
	case 5:
	case 8:
	case 12:
		/* load type */
		prtf ("0x%x",offset);
		if (mode != 0) {
			prtf ("(%s)",reg2);
		}
		prtf (",   %s",reg1);
		break;

	case 2:
	case 10:
		/* store type */
		prtf ("%s,   0x%x",reg1,offset);
		if (mode != 0) {
			prtf ("(%s)",reg2);
		}
		break;

	case 4:
	case 6:
		/* bx/callx type */
		prtf ("0x%x",offset);
		if (mode != 0) {
			prtf ("(%s)",reg2);
		}
		break;
	}
}

/************************************************/
/* MEMB instruction type      			*/
/*                           			*/
/************************************************/
memb (instruction, op, reg1, reg2)
unsigned int instruction;
int op;
char *reg1, *reg2;
{
int scale, mode;
unsigned int word2;
unsigned int *oldip;
static const int scaler[] = {1, 2, 4, 8, 16};

	mode = (instruction >> 10) & 0x0f;
	scale = (instruction >> 7) & 0x07;
	if ((scale > 4) || (instruction & 0x60) != 0 ) {
		invalid (instruction);
		return;
	}
	if (instr[op].opcode != op) {
		prtf (tab);
		invalid (instruction);
		return;
	}

	switch (mode) {
		case 4:	
		case 7:
			prtf ("%s%s  ", tab, instr[op].name);
			break;
		case 5:
		case 12:
		case 13:
		case 14:
		case 15:
			oldip = ip++;
			word2 = *ip;
			prtf (" %X  %s  ",word2,instr[op].name);
			break;
		default:
			invalid (instruction);
			return;
	}


	op &= 0xf;

	if ( op == 2 || op == 10 ){				/* store type */
		prtf ("%s,   ",reg1);
	}

	ea(mode,reg2, regs[instruction & 0x1f], word2, scaler[scale], oldip);

	if ( op == 0 || op == 5 || op == 8 || op == 12 ){	/* load type */
		prtf (",   %s",reg1);
	}
	
}

/************************************************/
/* Output a single register operand 		*/
/*                           			*/
/************************************************/
static
regop(mode1,src,fp)
int mode1, src, fp;
{
	if(mode1 == 0) {
		prtf (regs[src]);
	} else if (fp) {
		if (src == 16) src = 4;
		if (src == 22) src = 5;
		prtf (fregs[src]);
	} else {
		prtf ("%h", src);
	}
}

/************************************************/
/* Invalid operation                		*/
/*                           			*/
/************************************************/
invalid (instruction)
int instruction;
{
	prtf (".word     0x%X",instruction);
}	
