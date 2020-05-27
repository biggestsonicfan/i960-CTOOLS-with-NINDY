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

#include "ui.h"
#include "i960.h"

extern int load_mem(ADDR, int mem_size, void *buf, int buf_size);
extern void prtf(); 
extern void perror(char *, int);

void reg(unsigned int, int, int);
void cobr(unsigned int, int, int);
void mem(unsigned int, int, int);
void ctrl(unsigned int, int, int);
void invalid(unsigned int);

static void mema(unsigned int, int, int, const char *, const char *);
static void memb(unsigned int, int, int, const char *, const char *);

/************************************************/
/*    LOOKUP TABLE FOR OPCODES        */
/* this table is in numeric order for binary search */
/* 7 or 8 probes should find the right opcode */
/************************************************/

/* Encodings for the operands of REG format instructions. The
 * third operand can be encoded one of three ways, according
 * to its use by the instruction.  This affects whether it is
 * a literal or a sfr when mode bit 3 is set.  */
#define SRC1    1
#define SRC2    2
#define SRC3    4
#define DST    8
#define SRC_DST    16

static const struct { char *name; short opcode; short operands; }
instr[] = {
    {NULL,        0,        0},        /* --------00-------- */
    {"b",         0x08,     1},
    {"call",      0x09,     1},
    {"ret",       0x0a,     0},
    {"bal",       0x0b,     1},
    {"bno",       0x10,     1},        /* --------10-------- */
    {"bg",        0x11,     1},
    {"be",        0x12,     1},
    {"bge",       0x13,     1},
    {"bl",        0x14,     1},
    {"bne",       0x15,     1},
    {"ble",       0x16,     1},
    {"bo",        0x17,     1},
    {"faultno",   0x18,     0},
    {"faultg",    0x19,     0},
    {"faulte",    0x1a,     0},
    {"faultge",   0x1b,     0},
    {"faultl",    0x1c,     0},
    {"faultne",   0x1d,     0},
    {"faultle",   0x1e,     0},
    {"faulto",    0x1f,     0},        /* ________1F________ */
    {"testno",    0x20,     1},        /* --------20-------- */
    {"testg",     0x21,     1},
    {"teste",     0x22,     1},
    {"testge",    0x23,     1},
    {"testl",     0x24,     1},
    {"testne",    0x25,     1},
    {"testle",    0x26,     1},
    {"testo",     0x27,     1},
    {"bbc",       0x30,     3},        /* --------30-------- */
    {"cmpobg",    0x31,     3},
    {"cmpobe",    0x32,     3},
    {"cmpobge",   0x33,     3},
    {"cmpobl",    0x34,     3},
    {"cmpobne",   0x35,     3},
    {"cmpoble",   0x36,     3},
    {"bbs",       0x37,     3},
    {"cmpibno",   0x38,     3},
    {"cmpibg",    0x39,     3},
    {"cmpibe",    0x3a,     3},
    {"cmpibge",   0x3b,     3},
    {"cmpibl",    0x3c,     3},
    {"cmpibne",   0x3d,     3},
    {"cmpible",   0x3e,     3},
    {"cmpibo",    0x3f,     3},        /* ________3F________ */
    {"ldob",      0x80,     2},        /* --------80-------- */
    {"stob",      0x82,     2},
    {"bx",        0x84,     1},
    {"balx",      0x85,     2},
    {"callx",     0x86,     1},
    {"ldos",      0x88,     2},
    {"stos",      0x8a,     2},
    {"lda",       0x8c,     2},
    {"ld",        0x90,     2},        /* --------90-------- */
    {"st",        0x92,     2},
    {"ldl",       0x98,     2},
    {"stl",       0x9a,     2},
    {"dcflusha",  0x9d,     1},
    {"ldt",       0xa0,     2},        /* --------A0-------- */
    {"stt",       0xa2,     2},
    {"dchint",    0xad,     1},
    {"ldq",       0xb0,     2},        /* --------B0-------- */
    {"stq",       0xb2,     2},
    {"dcinva",    0xbd,     1},
    {"ldib",      0xc0,     2},        /* --------C0-------- */
    {"stib",      0xc2,     2},
    {"ldis",      0xc8,     2},
    {"stis",      0xca,     2},
    {"notbit",    0x580,    SRC1|SRC2|DST},    /* --------58-------- */
    {"and",       0x581,    SRC1|SRC2|DST},
    {"andnot",    0x582,    SRC1|SRC2|DST},
    {"setbit",    0x583,    SRC1|SRC2|DST},
    {"notand",    0x584,    SRC1|SRC2|DST},
    {"xor",       0x586,    SRC1|SRC2|DST},
    {"or",        0x587,    SRC1|SRC2|DST},
    {"nor",       0x588,    SRC1|SRC2|DST},
    {"xnor",      0x589,    SRC1|SRC2|DST},
    {"not",       0x58a,    SRC1|DST},
    {"ornot",     0x58b,    SRC1|SRC2|DST},
    {"clrbit",    0x58c,    SRC1|SRC2|DST},
    {"notor",     0x58d,    SRC1|SRC2|DST},
    {"nand",      0x58e,    SRC1|SRC2|DST},
    {"alterbit",  0x58f,    SRC1|SRC2|DST},
    {"addo",      0x590,    SRC1|SRC2|DST},
    {"addi",      0x591,    SRC1|SRC2|DST},
    {"subo",      0x592,    SRC1|SRC2|DST},
    {"subi",      0x593,    SRC1|SRC2|DST},
    {"cmpob",     0x594,    SRC1|SRC2},
    {"cmpib",     0x595,    SRC1|SRC2},
    {"cmpos",     0x596,    SRC1|SRC2},
    {"cmpis",     0x597,    SRC1|SRC2},
    {"shro",      0x598,    SRC1|SRC2|DST},
    {"shrdi",     0x59a,    SRC1|SRC2|DST},
    {"shri",      0x59b,    SRC1|SRC2|DST},
    {"shlo",      0x59c,    SRC1|SRC2|DST},
    {"rotate",    0x59d,    SRC1|SRC2|DST},
    {"shli",      0x59e,    SRC1|SRC2|DST},
    {"cmpo",      0x5a0,    SRC1|SRC2},
    {"cmpi",      0x5a1,    SRC1|SRC2},
    {"concmpo",   0x5a2,    SRC1|SRC2},
    {"concmpi",   0x5a3,    SRC1|SRC2},
    {"cmpinco",   0x5a4,    SRC1|SRC2|DST},
    {"cmpinci",   0x5a5,    SRC1|SRC2|DST},
    {"cmpdeco",   0x5a6,    SRC1|SRC2|DST},
    {"cmpdeci",   0x5a7,    SRC1|SRC2|DST},
    {"scanbyte",  0x5ac,    SRC1|SRC2},
    {"bswap",     0x5ad,    SRC1|SRC_DST},
    {"chkbit",    0x5ae,    SRC1|SRC2},
    {"addc",      0x5b0,    SRC1|SRC2|DST},
    {"subc",      0x5b2,    SRC1|SRC2|DST},
    {"intdis",    0x5b4,    0},
    {"inten",     0x5b5,    0},
    {"mov",       0x5cc,    SRC1|DST},
    {"eshro",     0x5d8,    SRC1|SRC2|SRC_DST},
    {"movl",      0x5dc,    SRC1|DST},
    {"movt",      0x5ec,    SRC1|DST},
    {"movq",      0x5fc,    SRC1|DST},    /* ________5F________ */
    {"synmov",    0x600,    SRC1|SRC2},
    {"synmovl",   0x601,    SRC1|SRC2},
    {"synmovq",   0x602,    SRC1|SRC2},
    {"cmpstr",    0x603,    SRC1|SRC2|SRC3},
    {"movqstr",   0x604,    SRC1|SRC2|SRC3},
    {"movstr",    0x605,    SRC1|SRC2|SRC3},    
    {"atmod",     0x610,    SRC1|SRC2|SRC_DST}, 
    {"atadd",     0x612,    SRC1|SRC2|DST},
    {"inspacc",   0x613,    SRC1|DST},
    {"ldphy",     0x614,    SRC1|DST},
    {"synld",     0x615,    SRC1|DST},
    {"fill",      0x617,    SRC1|SRC2|SRC3},
    {"sdma",      0x630,    SRC1|SRC2|SRC3},
    {"udma",      0x631,    0},
    {"spanbit",   0x640,    SRC1|DST},
    {"scanbit",   0x641,    SRC1|DST},
    {"daddc",     0x642,    SRC1|SRC2|DST},
    {"dsubc",     0x643,    SRC1|SRC2|DST},
    {"dmovt",     0x644,    SRC1|DST},
    {"modac",     0x645,    SRC1|SRC2|DST},
    {"condrec",   0x646,    SRC1|DST},
    {"modify",    0x650,    SRC1|SRC2|SRC_DST},
    {"extract",   0x651,    SRC1|SRC2|SRC_DST},
    {"modtc",     0x654,    SRC1|SRC2|DST},
    {"modpc",     0x655,    SRC1|SRC2|SRC_DST},
    {"receive",   0x656,    SRC1|DST},
    {"intctl",    0x658,    SRC1|SRC_DST},
    {"sysctl",    0x659,    SRC1|SRC2|SRC3},
    {"icctl",     0x65b,    SRC1|SRC2|SRC_DST},
    {"dcctl",     0x65c,    SRC1|SRC2|SRC_DST},
    {"halt",      0x65d,    SRC1},
    {"calls",     0x660,    SRC1},        
    {"send",      0x662,    SRC1|SRC2|SRC3},
    {"sendserv",  0x663,    SRC1},
    {"resumprcs", 0x664,    SRC1},
    {"schedprcs", 0x665,    SRC1},
    {"saveprcs",  0x666,    0},
    {"condwait",  0x668,    SRC1},
    {"wait",      0x669,    SRC1},
    {"signal",    0x66a,    SRC3},
    {"mark",      0x66b,    0},
    {"fmark",     0x66c,    0},
    {"flushreg",  0x66d,    0},
    {"syncf",     0x66f,    0},        
    {"emul",      0x670,    SRC1|SRC2|DST},
    {"ediv",      0x671,    SRC1|SRC2|DST},
    {"ldtime",    0x673,    DST},
    {"cvtir",     0x674,    SRC1|DST},
    {"cvtilr",    0x675,    SRC1|DST},
    {"scalerl",   0x676,    SRC1|SRC2|DST},
    {"scaler",    0x677,    SRC1|SRC2|DST},
    {"atanr",     0x680,    SRC1|SRC2|DST},    
    {"logepr",    0x681,    SRC1|SRC2|DST},
    {"logr",      0x682,    SRC1|SRC2|DST},
    {"remr",      0x683,    SRC1|SRC2|DST},
    {"cmpor",     0x684,    SRC1|SRC2},
    {"cmpr",      0x685,    SRC1|SRC2},
    {"sqrtr",     0x688,    SRC1|DST},
    {"expr",      0x689,    SRC1|DST},
    {"logbnr",    0x68a,    SRC1|DST},
    {"roundr",    0x68b,    SRC1|DST},
    {"sinr",      0x68c,    SRC1|DST},
    {"cosr",      0x68d,    SRC1|DST},
    {"tanr",      0x68e,    SRC1|DST},
    {"classr",    0x68f,    SRC1},        
    {"atanrl",    0x690,    SRC1|SRC2|DST},    
    {"logeprl",   0x691,    SRC1|SRC2|DST},
    {"logrl",     0x692,    SRC1|SRC2|DST},
    {"remrl",     0x693,    SRC1|SRC2|DST},
    {"cmporl",    0x694,    SRC1|SRC2},
    {"cmprl",     0x695,    SRC1|SRC2},
    {"sqrtrl",    0x698,    SRC1|DST},
    {"exprl",     0x699,    SRC1|DST},
    {"logbnrl",   0x69a,    SRC1|DST},
    {"roundrl",   0x69b,    SRC1|DST},
    {"sinrl",     0x69c,    SRC1|DST},
    {"cosrl",     0x69d,    SRC1|DST},
    {"tanrl",     0x69e,    SRC1|DST},
    {"classrl",   0x69f,    SRC1},        
    {"cvtri",     0x6c0,    SRC1|DST},
    {"cvtril",    0x6c1,    SRC1|DST},
    {"cvtzri",    0x6c2,    SRC1|DST},
    {"cvtzril",   0x6c3,    SRC1|DST},
    {"movr",      0x6c9,    SRC1|DST},
    {"movrl",     0x6d9,    SRC1|DST},
    {"cpysre",    0x6e2,    SRC1|SRC2|DST},
    {"cpyrsre",   0x6e3,    SRC1|SRC2|DST},
    {"movre",     0x6e1,    SRC1|DST},    /* ________6F________ */
    {"mulo",      0x701,    SRC1|SRC2|DST},
    {"remo",      0x708,    SRC1|SRC2|DST},
    {"divo",      0x70b,    SRC1|SRC2|DST},    
    {"muli",      0x741,    SRC1|SRC2|DST},
    {"remi",      0x748,    SRC1|SRC2|DST},
    {"modi",      0x749,    SRC1|SRC2|DST},
    {"divi",      0x74b,    SRC1|SRC2|DST},    
    {"addono",    0x780,    SRC1|SRC2|SRC_DST},
    {"addig",     0x781,    SRC1|SRC2|SRC_DST},
    {"subono",    0x782,    SRC1|SRC2|SRC_DST},
    {"subig",     0x783,    SRC1|SRC2|SRC_DST},
    {"selno",     0x784,    SRC1|SRC2|SRC_DST},
    {"divr",      0x78b,    SRC1|SRC2|DST},
    {"mulr",      0x78c,    SRC1|SRC2|DST},
    {"subr",      0x78d,    SRC1|SRC2|DST},
    {"addr",      0x78f,    SRC1|SRC2|DST},
    {"addog",     0x790,    SRC1|SRC2|SRC_DST},
    {"addie",     0x791,    SRC1|SRC2|SRC_DST},
    {"subog",     0x792,    SRC1|SRC2|SRC_DST},
    {"subie",     0x793,    SRC1|SRC2|SRC_DST},
    {"selg",      0x794,    SRC1|SRC2|SRC_DST},
    {"divrl",     0x79b,    SRC1|SRC2|DST},
    {"mulrl",     0x79c,    SRC1|SRC2|DST},
    {"subrl",     0x79d,    SRC1|SRC2|DST},
    {"addrl",     0x79f,    SRC1|SRC2|DST},
    {"addoe",     0x7a0,    SRC1|SRC2|SRC_DST},
    {"addige",    0x7a1,    SRC1|SRC2|SRC_DST},
    {"suboe",     0x7a2,    SRC1|SRC2|SRC_DST},
    {"subige",    0x7a3,    SRC1|SRC2|SRC_DST},
    {"sele",      0x7a4,    SRC1|SRC2|SRC_DST},
    {"addoge",    0x7b0,    SRC1|SRC2|SRC_DST},
    {"addil",     0x7b1,    SRC1|SRC2|SRC_DST},
    {"suboge",    0x7b2,    SRC1|SRC2|SRC_DST},
    {"subil",     0x7b3,    SRC1|SRC2|SRC_DST},
    {"selge",     0x7b4,    SRC1|SRC2|SRC_DST},
    {"addol",     0x7c0,    SRC1|SRC2|SRC_DST},
    {"addine",    0x7c1,    SRC1|SRC2|SRC_DST},
    {"subol",     0x7c2,    SRC1|SRC2|SRC_DST},
    {"subine",    0x7c3,    SRC1|SRC2|SRC_DST},
    {"sell",      0x7c4,    SRC1|SRC2|SRC_DST},
    {"addone",    0x7d0,    SRC1|SRC2|SRC_DST},
    {"addile",    0x7d1,    SRC1|SRC2|SRC_DST},
    {"subone",    0x7d2,    SRC1|SRC2|SRC_DST},
    {"subile",    0x7d3,    SRC1|SRC2|SRC_DST},
    {"selne",     0x7d4,    SRC1|SRC2|SRC_DST},
    {"addoo",     0x7e0,    SRC1|SRC2|SRC_DST},
    {"addio",     0x7e1,    SRC1|SRC2|SRC_DST},
    {"suboo",     0x7e2,    SRC1|SRC2|SRC_DST},
    {"subio",     0x7e3,    SRC1|SRC2|SRC_DST},
    {"selle",     0x7e4,    SRC1|SRC2|SRC_DST},
    {"addino",    0x7f0,    SRC1|SRC2|SRC_DST},
    {"addono",    0x7f1,    SRC1|SRC2|SRC_DST},
    {"subino",    0x7f2,    SRC1|SRC2|SRC_DST},
    {"subono",    0x7f3,    SRC1|SRC2|SRC_DST},
    {"selo",      0x7f4,    SRC1|SRC2|SRC_DST},
};

static const char regs[][4] = {
    "pfp", "sp", "rip", "r3",  "r4",  "r5",  "r6",  "r7",
    "r8",  "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "g0",  "g1", "g2",  "g3",  "g4",  "g5",  "g6",  "g7",
    "g8",  "g9", "g10", "g11", "g12", "g13", "g14", "fp"
};

#if HX_CPU
static const char sfregs[][5] = { "ipnd", "imsk", "cctl", "intc", "gmuc" };
#else
static const char sfregs[][5] = { "ipnd", "imsk", "dmac" };
#endif
static const char fregs[][4] = { "fp0", "fp1", "fp2", "fp3", "0.0", "1.0" };
static const char tab[] = "           ";

static ADDR ip;
static unsigned int buf[2];

static void dis();
static void regop(int src, int fp, int mode, int special);

static int
find_opcode(int opcode)
{
/* use a binary search of the table to find the opcode */
/* return 0 if not found */

    int current, min = 0, max = sizeof(instr) / sizeof(instr[0]);

    current = max / 2;

	while (opcode != instr[current].opcode)
		{
		if (current == max && max == min + 1)
			return 0;

		if (instr[current].opcode < opcode)
		    {
            min = current;
		    current = (current + max + 1) / 2;
			}
		else
		    {
		    max = current;
		    current = (current + min) / 2;
		    }
        }
	return current;
}

/************************************************/
/* Disassemble A Word in Memory                */
/*                                       */
/************************************************/
void
dasm(dummy, nargs, addr, cnt )
int dummy;  /* ignored */
int nargs;    /* Number of following arguments that are valid (0,1, or 2) */
ADDR addr;    /* Optional address of instruction (defaults to ip)    */
int cnt;    /* Optional number of instructions (defaults to 1)    */
{
    /* Set defaults */
    if (nargs <= 1)
        cnt = 1;
    if ((nargs == 0) || (addr == NO_ADDR))
        addr = (ADDR)register_set[REG_IP];

    ip = addr;
    break_flag = FALSE;
    while (cnt-- && !break_flag)
    {
        if (load_mem(ip, WORD, &buf, 2*WORD) != OK)
        {
            perror((char *)NULL, 0);
            break;
        }
        prtf("%X : %X", ip, buf[0]);
        dis();
        prtf("\n");
        ip += WORD;
    }
}

/****************************************/
/* Disassemble Instructions        */
/****************************************/
static void
dis()
{
unsigned int instruction;     /* 32 bit machine instruction */
unsigned int opcode, opcode_index; 

    instruction = buf[0];
    opcode = instruction >> 24;

    /* check for valid opcode */
        if (opcode >= 0x58 && opcode <= 0x7f)
			{
			opcode = (opcode << 4) + ((instruction >> 7) & 0xf);
            if ((opcode_index = find_opcode(opcode)) == 0)
    			{
                prtf (tab);
                invalid(instruction);
				return;
                }
            prtf (tab);
            reg(instruction, opcode_index, opcode);
			return;
			}

        if ((opcode_index = find_opcode(opcode)) == 0)
			{
            prtf (tab);
            invalid(instruction);
			return;
            }

        if (opcode < 0x20)
			{
            prtf (tab);
            ctrl(instruction, opcode_index, opcode);
            }
        else if (opcode < 0x40)
			{
            prtf (tab);
            cobr(instruction, opcode_index, opcode);
            }
        else if (opcode >= 0x80)
			{
            mem(instruction, opcode_index, opcode);
            }
}

/****************************************/
/* CTRL format                */
/****************************************/
void
ctrl(instruction, op, opcode)
unsigned int instruction;
int op, opcode;
{
int displace;

    if ( (instruction & 0x1) && (instr[op].opcode == opcode) ){
        invalid(instruction);
    } else {
        prtf( "%s", instr[op].name );
         if (instruction & 0x3)
                        prtf (".f  ");
                else
                        prtf ("  ");

        if (opcode < 0x18 && opcode != 0x0a) {
            displace = (int)(instruction & 0xfffffc);
            if (displace & 0x800000) { /* sign bit is set */
                /* sign extend displacement */
                displace |= -1L << 24;
            }
            prtf ("0x%X", ip + displace);
        }
    }
}

/****************************************/
/* COBR format                */
/****************************************/
void
cobr(instruction, op, opcode)
unsigned int instruction;
int op, opcode;
{
int displace, src1, src2, m1, s2;

    prtf ("%s", instr[op].name);

     if (instruction & 0x2)
                prtf (".f  ");
        else
                prtf ("  ");

    src1 = (instruction >> 19) & 0x1f;
    src2 = (instruction >> 14) & 0x1f;
    m1 = (instruction >> 13) & 0x01;
    s2 = instruction & 0x01;

    if (opcode >= 0x30) {
        regop(src1, FALSE, m1, 0);
        prtf (", ");
        regop(src2, FALSE, 0, s2);
        prtf (", ");

        displace = (int)(instruction & 0x1ffc);
        if (displace & 0x1000) { /* negative displacement */
            /* sign extend displacement */
            displace = (~0x1fff) | displace;
        }
        prtf ( "0x%X", ip + displace );
    }
    else {
        regop(src1, FALSE, m1, s2);
    }
}

/****************************************/
/* MEM format                */
/****************************************/
void
mem(instruction, op, opcode)
unsigned int instruction;
int op, opcode;
{
const char *reg1, *reg2;

    reg1 = regs[ (instruction>>19) & 0x1f ];
    reg2 = regs[ (instruction>>14) & 0x1f ];

    if  ( instruction & 0x1000 ){
        memb(instruction, op, opcode, reg1, reg2);
    } else {             /* MEMB format */
        mema(instruction, op, opcode, reg1, reg2);
    }
}

/****************************************/
/* REG format                */
/****************************************/
void
reg(instruction, op, opcode)
unsigned int instruction;
int op, opcode;
{
int mode1, mode2, mode3, s1, s2, fp=FALSE;
int operands, comma_flag;
int src, dst, src2;

/* In fact, this module is architecture-independent (i.e., KX is not defined),
 * so this check is not done.  The instruction will be disassembled with a
 * reference to a CA sfr.  */
#if KXSX_CPU
    mode1 = (instruction >> 5) & 0x03;
    if (mode1 != 0)
    {
        invalid (instruction);
        return;
    }
#endif /* KXSX */

    prtf ("%s  ", instr[op].name );

    mode1 = (instruction >> 11) & 0x01;
    mode2 = (instruction >> 12) & 0x01;
    mode3 = (instruction >> 13) & 0x01;
    s1 = (instruction >> 5) & 0x01;
    s2 = (instruction >> 6) & 0x01;
    src = instruction & 0x1f;
    src2 = (instruction >> 14) & 0x1f;
    dst = (instruction >> 19) & 0x1f;

    if ((opcode >= 0x78b) || ((opcode < 0x6e9) && (opcode >= 0x674))) 
        fp = TRUE;
    else
        fp = FALSE;

    comma_flag = FALSE;
    operands = instr[op].operands;
    if (operands & SRC1)
    {
        regop(src, fp, mode1, s1);
        comma_flag = TRUE;
    }

    if (operands & SRC2)
    {
        if (comma_flag)
        prtf (", ");
        regop(src2, fp, mode2, s2);
        comma_flag = TRUE;
    }

    if (operands & (SRC3|DST|SRC_DST))
    {
        int special = 0;

        if (mode3 && !fp)
        {
        if (operands&DST)
        {
            mode3 = 0;
            special = 1;
        }
        else if (operands&SRC_DST)
            special = 1;     /* This will cause regop to print ??? */
        }

        if (comma_flag)
        prtf (", ");
        regop(dst, fp, mode3, special);
    }
}

/************************************************/
/* Output a single register operand         */
/*                                       */
/************************************************/
static void
regop(int reg, int fp, int mode, int special)
{
    if (!mode) {
        if (special)
            prtf(sfregs[reg]);
        else
            prtf(regs[reg]);
    } else if (fp) {
        if (reg == 16) reg = 4;
        if (reg == 22) reg = 5;
        prtf (fregs[reg]);
    } else {
        if (special)
            prtf("???");
        else
            prtf ("%h", reg);
    }
}

/************************************************/
/* Print out effective address            */
/*                                       */
/************************************************/
static void
ea(mode, reg2, reg3, word2, scale)
int mode;
const char *reg2, *reg3;
unsigned int word2;
int scale;
{
    switch (mode) {
        case 4:             /* (reg) */
            prtf ("(%s)",reg2);
            break;
        case 5:            /* displ+8(ip) */
            prtf ("0x%x(0x%x)", word2, ip + 4); /* client has previously 
                                                 * bumped ip by 4.
                                                 */
            break;
        case 7:            /* (reg)[index*scale] */
            if (scale == 1) {
                prtf ("(%s)[%s]",reg2,reg3);
            } else {
                prtf ("(%s)[%s*%b]",reg2,reg3,scale);
            }
            break;
        case 12:        /* displacement */
            prtf ("0x%X",word2);
            break;
        case 13:        /* displ(reg) */
            prtf ("0x%X(%s)",word2,reg2);
            break;
        case 14:        /* displ[index*scale] */
            if (scale == 1) {
                prtf ("0x%x[%s]",word2,reg3);
            } else {
                prtf ("0x%x[%s*%b]",word2,reg3,scale);
            }
            break;
        case 15:        /* displ(reg)[index*scale] */
            prtf ("0x%x(%s)[%s",word2,reg2,reg3);
            if (scale != 1) {
                prtf ("*%b",scale);
            }
            prtf("]");
            break;
        default:
            invalid (buf[0]);
            return;
    }
}

/************************************************/
/* MEMA instruction type                  */
/*                                       */
/************************************************/
static void
mema (instruction, op, opcode, reg1, reg2)
unsigned int instruction;
int op, opcode;
const char *reg1, *reg2;
{
int mode, offset;

    prtf (tab);
    prtf ("%s  ",instr[op].name);
    mode = instruction & 0x2000;
    offset = instruction & 0xfff;

    switch ( opcode & 0x0f ){
    case 0:
    case 5:
    case 8:
    case 12:
        /* load type */
        prtf ("0x%x",offset);
        if (mode != 0) {
            prtf ("(%s)",reg2);
        }
        prtf (", %s",reg1);
        break;

    case 2:
    case 10:
        /* store type */
        prtf ("%s, 0x%x", reg1, offset);
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
/* MEMB instruction type                  */
/*                                       */
/************************************************/
static void
memb (instruction, op, opcode, reg1, reg2)
unsigned int instruction;
int op, opcode;
const char *reg1, *reg2;
{
int scale, mode;
unsigned int word2 = 0;
static const int scaler[] = {1, 2, 4, 8, 16};

    mode = (instruction >> 10) & 0x0f;
    scale = (instruction >> 7) & 0x07;
    if ((scale > 4) || (instruction & 0x60) != 0 ) {
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
            word2 = buf[1];
            ip += WORD;
            prtf (" %X  %s  ", word2, instr[op].name);
            break;
        default:
            invalid (instruction);
            return;
    }


    if ( (opcode & 0xf) == 2 || (opcode & 0xf) == 10 ){       /* store type */
        prtf ("%s, ",reg1);
    }

    ea(mode, reg2, regs[instruction & 0x1f], word2, scaler[scale]);

    if ((opcode == 0x9d) || (opcode == 0xad) || (opcode == 0xbd))
        return;

    opcode &= 0xf;
    if ( opcode == 0 || opcode == 5 || opcode == 8 || opcode == 12 ){ 
	/* load type */
        prtf (", %s",reg1);
    }

}

/************************************************/
/* Invalid operation                        */
/*                                       */
/************************************************/
void
invalid (instruction)
unsigned int instruction;
{
    prtf (".word     0x%X",instruction);
}
