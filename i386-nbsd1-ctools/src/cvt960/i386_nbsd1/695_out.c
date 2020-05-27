

/*(c**************************************************************************** *
 * Copyright (c) 1990, 1991, 1992, 1993 Intel Corporation
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
 ***************************************************************************c)*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <setjmp.h>
#include "coff.h"	/* F_I960* codes */
#include "err_msg.h"	/* error codes */
#include "695_out.h"	/* external interface definitions for this module */

/* Local #defines *************************************************************/
#define TRUE 1
#define FALSE 0
/* def MODENV */

#define MODENV HPHOST_UNK

/* def MODENV */

#define BIND_IBYTE(f,b) ((f)->flags = FF_NONE,   (f)->ibyte = (b))

/* built-in types are small unsigned numbers */
#define IS_BITY(t)  (t < 256)

#define LAST_ECALL	37	/* largest internal error.c code used */

#define MAXDWRITE 127

#define NO_ERROR 0		/* non-negative-> absence of error ret flag */
#define ERRORF	-1		/* error return flag */

#define ERROR(x,i)	(print_error((x),(i)), longjmp(env, -1) )

#define ERRORHANDLER() {if (setjmp(env)<0) return ERRORF;}
				/* setjmp initialization */

#define SECID_RANGECHK(id)  if (((id)<1) || (0x7FFF<(id))){ \
   error_section=(id); ERROR(HPER_SECID,5);}

#define CUR_OFFSET()	ftell(odescr.stream)

#define HPPARTS		7	/* 7 parts have offset values
				 * which need be backpatched
				 */

#define NOT_AN_OFFSET	(long)-1 /* impossible file offset */

/* PT_* are offsets into odescr.partoff[] */
#define PT_ADX		0
#define PT_ENV		1
#define PT_SEC		2
#define PT_EXT		3
#define PT_DEBUG	4
#define PT_DATA		5
#define PT_TRAIL	6


/* the IB_* define values of the ib field of FIELD; see 695 Table 2-1 */
#define IB_ZERO		0
#define IB_SCNTFIRST	IB_ZERO
#define IB_SCNTLAST	0x7F
#define IB_NULL		0x80
#define IB_1NUM		0x81
#define IB_2NUM		0x82
#define IB_3NUM		0x83
#define IB_4NUM		0x84
/* 0x85..0x88 presently unsupported (>4 byte unsigned numbers) */
/* 0xA0..0xBD presently unsupported (function values) */
/* 0xBE and 0xBF presently supported only for the quirky requirements
 * of the ASG record within Trailer Part.
 */
#define IB_FOPAREN	0xBE	/* '(' */
#define IB_FCPAREN	0xBF	/* ')' */
#define IBV(c)		(0xC1+((c)-'A'))
#define IB_VA		IBV('A')
#define IB_VB		IBV('B')
#define IB_VD		IBV('D')
#define IB_VF		IBV('F')
#define IB_VG		IBV('G')
#define IB_VI		IBV('I')
#define IB_VL		IBV('L')
#define IB_VM		IBV('M')
#define IB_VN		IBV('N')
#define IB_VP		IBV('P')
#define IB_VR		IBV('R')
#define IB_VS		IBV('S')
#define IB_VW		IBV('W')
#define IB_VX		IBV('X')
/* 0xDE..0xDF presently unsupported */
#define IB_SHRTSTR	0xDE
#define IB_LNGSTR	0xDF
#define IB_MB		0xE0
#define IB_ME		0xE1
#define IB_AS		0xE2
#define IB_SB		0xE5
#define IB_ST		0xE6
#define IB_NI		0xE8
#define IB_AD		0xEC
#define IB_LD		0xED
#define IB_NN		0xF0
#define IB_AT		0xF1
#define IB_TY		0xF2
#define IB_RE		0xF7
#define IB_BB		0xF8
#define IB_BE		0xF9

/* HP695_* define 695 spec 4.0 */
#define HP695_VN	4
#define HP695_VR	0

/* NN_* ; predefined N variable indices */
#define NN_ADX		32
#define NN_ENV		33
#define NN_FIRSTDYN	34	/* first allocated to user */

/* NI_* ; predefined I variable indices */
#define NI_FIRSTDYN	32	/* normal indices begin at 32 */

#define DEFHANDLE_FIRST    0x100   /* first type handle to allocate */

#define OMC_NOFORCE	2	/* do not change the case of symbols */
#define OMT_ABSOLUTE	1	/* the only type we now use */

/* FF_* in FIELD.flags */
#define FF_NONE		0
#define FF_STR		(1<<0)	/* FIELD represents a string. */
#define FF_PIN		(1<<1)	/* FIELD's file-offset must be recorded */

/*
 * DBLK_* are the debug-part block type values
 */
#define DBLK_BOTTOM	0	/* dummy type for bottom of stack */
#define DBLK_MTDEF	1
#define DBLK_GTDEF	2
#define DBLK_MSCOPE	3
#define DBLK_GFUNC	4
#define DBLK_SLINE	5
#define DBLK_LFUNC	6
#define DBLK_ASMSC	10
#define DBLK_MODSEC	11



/* local typedefs *************************************************************/
typedef struct shead_tag {
   char *name;
   HPSECID id;
   int  type;
   unsigned long size;
   LOGADR960 baseaddr;
   LOGADR960 pc;
   struct shead_tag *next;
} SHEAD;

typedef struct field_tag {
   /* FIELD supports 695 initial bytes, numbers and ids.
    * These can be translated into 695-compliant byte streams, and/or the
    * length of such translations can be determined by simple algorithms.
    * (Note however that multi-byte numeric fields in this
    * struct are STILL in host-native byte order).
    * Any data which is not implicit in the contents of ibyte is stored
    * in the data field.
    *
    * Expressions are not yet supported; one might like to have an
    * expression as a postfix list of operands & operators.
    */

   unsigned char ibyte; /* encoded as defined in 695 Table 2-1 */
   unsigned char flags; /* FF_* */
   union {
      struct {
         unsigned short len;	/* len(id): 0..2**16-1 */
	 unsigned char *p;	/* ptr to string */
      } s;
      unsigned long u;		/* unsigned between 2 and 4 bytes long */
      long i;			/* signed 4-byte number */
   } data;
} FIELD;

typedef struct dblock_tag {
   unsigned short type;  /* DBLK_* */
   struct dblock_tag *next;
   union {
      LOGADR960 func_end; /* B4 or B6 */
      unsigned long msect_size; /* B11 */
      unsigned long n_index; /* B5 */
      char *srcmod_name; /* bottom */
   } data;
} DBLK;

/* Global Variables **********************************************************/

extern int amc_mri;
extern int column_zero;
extern int architecture;

/* Local Variables ************************************************************/


char nul = '\0';

static DBLK debug_base = {DBLK_BOTTOM}, *debug_stack = &debug_base;

static unsigned next_i /* = NI_FIRSTDYN */;

unsigned init_n;	/* = NN_FIRSTDYN */
unsigned next_n;

static unsigned next_handle /* = DEFHANDLE_FIRST */;

static FIELD 
   null_name, zero, one, null_field; /* commonly-used field constants */

/* error_* are global parameters of print_error */
static HPSECID error_section;
static int error_int;
static unsigned char error_uchar;

static unsigned char exestat;

static SHEAD *seclist;

static char *client_name;
static char *error_prefix;		/* function of module and client name */
static char module[] = "<695 Writer>";	/* constant */

static int last_reptd_error;	/* error.c-compatible code */
static int last_ierr;

static struct {
   FILE *stream;	/* = NULL; output file stream */
   char *name;

   long partptroff[HPPARTS];	/* offsets of 4-byte part pointers */
   long partoff[HPPARTS];	/* backpatch-values for above */

   long meptroff;		/* offset to pointer to ME byte */

   long exestatoff;		/* offset to exe status byte */
   long meoff;			/* offset to ME record */

} odescr = {NULL};
		/* stream must be null to permit hp_abort() before
		 * hp_close().
                 */

static long *pin_offsetvar;		/* points to place to store offset */

static jmp_buf env;		/* see ERROR* */

/* Architecture String */
static char arch[] = "80960????"; /* big enough for "80960KA", "80960CORE" */
static char *archsuff = &arch[5];

/* Local Function Declarations ************************************************/
static char *chkalloc();

static FIELD *write_nn();

static SHEAD *alloc_sec(), *find_sec();

static DBLK *pop_debug_stack();

static int putseg();

static void 
   print_error(), insert_sec(), free_seclist(),
   trantime(), mark_pin(), bind_string(),
   bind_unum(), write_record(), write_field(), write_backpatches(),
   write_at(),  write_as(), write_st(), 
   write_header_part(), write_adx_part(), write_env_part(), write_symbol(),
   module_init(), open_dblk(), close_dblk(), deftype(),
   putasmid();

static unsigned write_new_ni();

/* Local Function Definitions *************************************************/

/*
 * chkalloc: malloc with an error-print and exit on NULL result.
 */
static char *chkalloc(size)
   unsigned size;
{
   char *mallocr = (char *) malloc(size);
   if (mallocr == NULL) {
      ERROR(ALLO_MEM, 36); 
   }

   return mallocr;
}

static DBLK *pop_debug_stack()
{
   if (debug_stack->type != DBLK_BOTTOM) {
      DBLK *prevtop = debug_stack;

      debug_stack = prevtop->next;
      return prevtop;
   } else
      return NULL;
}

/*
 * Allocate a DBLK structure on the debug_stack with data taken from
 * parameter *blk.  Write the BB byte, the block type, and 0 to indicate
 * that we are not specifying the size of the block.
 *
 * In the case of DBLKs of type MTDEF, MSCOPE, ASMSC and SLINE, also
 * pick up the module name from the base of the debug stack and write
 * it. Other DBLK types have varying semantics for the Id which follows,
 * so leave it up to the caller to implement them.
 */
static void open_dblk(blk)
   DBLK *blk;
{

   switch (blk->type) {
      FIELD f_b, f_btype, f_modname;

   case DBLK_MTDEF:
   case DBLK_MSCOPE:
   case DBLK_GFUNC:
   case DBLK_LFUNC:
   case DBLK_SLINE:
   case DBLK_ASMSC:
   case DBLK_MODSEC:
   case DBLK_BOTTOM:
      BIND_IBYTE(&f_b, IB_BB);
      write_field(&f_b); /* Block Begin */
      bind_unum(&f_btype, blk->type);
      write_field(&f_btype); /* block type */
      write_field(&zero);  /* block size in bytes (0 = unknown) */

      switch (blk->type) {
      case DBLK_MTDEF:
      case DBLK_MSCOPE:
      case DBLK_ASMSC:

         /* These cases also require the module name tucked away
          * in the base of the stack.
          */
         bind_string(&f_modname, debug_base.data.srcmod_name);
         write_field(&f_modname);
      }
      break;

   default:
      /* unknown or inappropriate debug block type */
      error_int = blk->type;
      ERROR(HPER_UNKDBLK, 29);
      /*NOTREACHED*/
   }

   {
      DBLK *allocd_blk = (DBLK *)chkalloc(sizeof(DBLK));

      if (!allocd_blk)
         ERROR(ALLO_MEM, 32);

      /* Set the block's type, and possibly set additional fields as a
       * function of the type.
       */
      switch (allocd_blk->type = blk->type) {
      case DBLK_MODSEC:
         allocd_blk->data.msect_size = blk->data.msect_size;
         break;
      }
   
      /* Push the block on the stack */
      allocd_blk->next = debug_stack;
      debug_stack = allocd_blk;
   }
}

/*
 * Close the block whose information is provided in the parameter.
 * Some blocks require the insertion of some information in
 * the closing sequence.
 *
 * UPDATE: May want to move the block deallocation (free) out of the
 * callers and put it in here.
 */
static void close_dblk(blk)
   DBLK *blk;
{
   switch (blk->type) {
      FIELD f_be;

   case DBLK_MTDEF:
   case DBLK_MSCOPE:
   case DBLK_GFUNC:
   case DBLK_LFUNC:
   case DBLK_SLINE:
   case DBLK_ASMSC:
   case DBLK_MODSEC:
      BIND_IBYTE(&f_be, IB_BE);
      write_field(&f_be);

      switch (blk->type) {
         FIELD f_fend, f_mss;

      case DBLK_MODSEC: /* also include size in MAUs of module section */
         bind_unum(&f_mss, blk->data.msect_size);
         write_field(&f_mss);
         break;

      case DBLK_GFUNC:
      case DBLK_LFUNC: /* also include ending address of function */
         bind_unum(&f_fend, blk->data.func_end);
         write_field(&f_fend);
         break;

      default:
         break; /* Nothing more to be included */
      }
      break;

   case DBLK_BOTTOM:
   default:
      ERROR(HPER_UNKDBLK, 30);
      /*NOTREACHED*/
   }
}

static void print_error(xerr,ierr)
   int xerr,ierr;
{
   switch (xerr) {
   default:
      error_out(error_prefix, HPER_UNK, xerr);
      break;

   /* The following cases relate to the output file */
   case NOT_OPEN:
   case WRIT_ERR:
   case CRE_FILE:
      error_out(error_prefix, xerr, ierr, odescr.name);
      break;

   /* The following case relates to the input file, whose name
    * we do not have defined in this module.
    */
   case ER_FREAD:
      error_out(error_prefix, xerr, ierr, "the load file");
      break;

   case ALLO_MEM:

   case HPER_BADHITYPE:
   case HPER_RECOPMOD:
   case HPER_TYDEFUSE:
   case HPER_CLSDMOD:
   case HPER_NOTASMATT:
      error_out(error_prefix, xerr, ierr);
      break;

   /* Unsupported architecture */
   case BD_AOPT:
      error_out(error_prefix, xerr, ierr, arch);
      break;

   /* out of range section */
   case HPER_SECID:
   case HPER_UNKSEC:
   case HPER_DUPSEC:
      error_out(error_prefix, xerr, ierr, error_section);
      break;

   /* nonsensical architecture code */
   case HPER_ACODE:

   case HPER_UNKDBLK:
   case HPER_BADATT:
   case HPER_FCLOSE:
      error_out(error_prefix, xerr, ierr, error_int);
      break;

   case HPER_BADCHAR:
   case HPER_BADSTYPE:
   case HPER_BADHPTYPE:
      error_out(error_prefix, xerr, ierr, 0xFF&error_uchar);
      break;

   }

   last_reptd_error = xerr;	/* error.c-compatible code */
   last_ierr = ierr;
}

/*
 * Put a new section id in the list.  It is an error to duplicate
 * a section id which exists in the list.
 */
static void insert_sec(sec)
   SHEAD *sec;
{
   SHEAD *listtail = sec->next = seclist;

   seclist = sec;

   while ( listtail != NULL ) {
      if ( listtail->id == sec->id ) {
         error_section = sec->id;
         ERROR(HPER_DUPSEC, 14);
      }
      listtail = listtail->next;
   }
}

static void free_seclist()
{
   SHEAD *nextsec;

   while (seclist != NULL) {
      nextsec = seclist->next;
      free(seclist->name);
      free(seclist);
      seclist = nextsec;
   }
}

static SHEAD *alloc_sec(sec)
   SHEAD *sec;
{
   SHEAD *newsec = (SHEAD*)chkalloc(sizeof(SHEAD));

   char  *newname = strcpy((char *)chkalloc(strlen(sec->name)+1), sec->name);

   newsec->id = sec->id;
   newsec->type = sec->type;
   newsec->size = sec->size;
   newsec->baseaddr = sec->baseaddr;
   newsec->pc = sec->pc;
   newsec->next = NULL;
   newsec->name = newname;

   return newsec;
}

static SHEAD *find_sec(id)
   HPSECID id;
{
   SHEAD *cursec = seclist;
   SHEAD *prevsec = NULL;

   while (cursec != NULL) {
      if (cursec->id == id  ) {
         if (prevsec != NULL)
            prevsec->next = cursec->next;
         else
            seclist = cursec->next;

         cursec->next = seclist;
         return seclist = cursec;
      } else {
         prevsec = cursec;
         cursec = cursec->next;
      }
   }
   return NULL;
}

int const_time_flag;

/* translate long integer UNIX time into 6 short integers */
static void trantime(utime,arr)
   long utime;
   unsigned short *arr;
      /* arr[0..5] <= Year, Month, Day, Hour, Minute, Second */
{
    long const_time = 0x12345678;
    struct tm *t = localtime(const_time_flag ? &const_time : &utime);

    arr[0] = t->tm_year + 1900; /* normalize 1900-relative  to absolute */
    arr[1] = t->tm_mon + 1; /* normalize 0-11 range to 1-12 range */
    arr[2] = t->tm_mday;
    arr[3] = t->tm_hour;
    arr[4] = t->tm_min;
    arr[5] = t->tm_sec;
}

static void mark_pin(f,offsetvar)
   FIELD *f;
   long *offsetvar;
{
   f->flags |= FF_PIN;	/* When this field is written, the file offset
			 * will be recorded.
			 */

   pin_offsetvar = offsetvar;
}


/*
 * Bind the value of the "C" string 's' to the HP field 'f'.
 *
 * Note that non-printable chars in strings are not yet supported by the 
 * calling sequence: strlen is used, which will stop at NULs.
 */
static void bind_string(f,s)
   FIELD *f;
   char  *s;
{
   int i;
   int n = (s!=NULL? strlen(s): 0);

   if ( n < 128 )
      BIND_IBYTE(f, n);
   else if ( n < 256 )
      BIND_IBYTE(f, IB_SHRTSTR);
   else
      BIND_IBYTE(f, IB_LNGSTR);
  

   f->flags |= FF_STR;
   f->data.s.len = n;
   f->data.s.p = (unsigned char *)s;
   
}

static void bind_unum(f,n)
   FIELD *f;
   unsigned long n;
{
   f->flags = FF_NONE;
   if (n <= IB_SCNTLAST) {
      f->ibyte = n;
      return;
   } else if (n <= 0xFF) {
      f->ibyte = IB_1NUM;
   } else if (n <= 0xFFFF) {
      f->ibyte = IB_2NUM;
   } else if (n <= 0xFFFFFF) {
      f->ibyte = IB_3NUM;
   } else {
      f->ibyte = IB_4NUM;
   }
   f->data.u = n;
}

static void write_record(fl)
   FIELD **fl;
{
   if (fl) {
      while (*fl) {
         write_field(*fl++);
      }
   }
}

/*
 * Write the field f.  If this field is marked "pin", and there is
 * a designated place to store the offset, write the file offset
 * where we will write the field to that place.
 */
static void write_field(f)
   FIELD *f;
{
   unsigned long n;
   int i;
   unsigned char b = f->ibyte;
   unsigned char c;
   int pin = (f->flags&FF_PIN)  &&  pin_offsetvar;

   clearerr(odescr.stream);

   if (pin) 
      *pin_offsetvar = CUR_OFFSET();
      
   if (b <= IB_SCNTLAST) {
      putc(b, odescr.stream);
      if ((f->flags&FF_STR) && b) {
         /* The field is a small (1<b<128) string; write the str value */
         fwrite(f->data.s.p,b,1,odescr.stream);
      } else
         /* otherwise the field was a small (<128) number and we are done */;
   } else
      /* The field is either a single byte, or a more complicated
       * string (note that we do not support expressions yet).
       */
      switch(b) {

      case IB_NULL:

      case IB_FOPAREN:
      case IB_FCPAREN:

      case IB_VA:
      case IB_VB:
      case IB_VD:
      case IB_VF:
      case IB_VG:
      case IB_VI:
      case IB_VL:
      case IB_VM:
      case IB_VN:
      case IB_VP:
      case IB_VR:
      case IB_VS:
      case IB_VW:
      case IB_VX:

      case IB_MB:
      case IB_ME:
      case IB_AS:
      case IB_SB:
      case IB_ST:
      case IB_AD:
      case IB_NN:
      case IB_AT:
      case IB_LD:
      case IB_RE:
      case IB_NI:
      case IB_BB:
      case IB_BE:
      case IB_TY:

      /* single byte field */
         putc(b, odescr.stream);
         break;

      case IB_1NUM:
      case IB_2NUM:
      case IB_3NUM:
      case IB_4NUM:

         /* multi-byte number */
         n = f->data.u;
         putc(b, odescr.stream);
         for (i = (b&7)-1; i>=0; i--) {
            c = 0xFF&(n>>i*8);
            putc(c, odescr.stream);
         }
         break;

      case IB_LNGSTR:
      case IB_SHRTSTR:

          /* complicated string: 0<len<2**16-1 */
         putc(b, odescr.stream);
         i = f->data.s.len;
         if (b==IB_LNGSTR) {
            c = (i>>8)&0xFF;
            putc(c, odescr.stream);
         }
         c = i&0xFF;
         putc(c, odescr.stream);
         if (i)
            fwrite(f->data.s.p, i, 1, odescr.stream);
         break;

      default:

         /* initial byte value is unsupported */
         error_uchar = b;
         ERROR(HPER_BADCHAR,15);
      }
      
      if (ferror(odescr.stream))
	 error_out(error_prefix,CVT_WRITE_ERR,27);

}

static void write_backpatches()
{
   unsigned w_index;
   FIELD part_pointer, w_if;
   FIELD *record[3];

   record[0]= &w_if;
   BIND_IBYTE(record[1]= &part_pointer,IB_4NUM);
   record[2]=0;
   for (w_index = 0; w_index < HPPARTS+1; w_index++) {
      long seekptr, bpvalue;

      if (w_index < HPPARTS) {
         seekptr = odescr.partptroff[w_index];
         bpvalue = odescr.partoff[w_index];
      } else {
         seekptr = odescr.meptroff;
         bpvalue = odescr.meoff;
      }
      if ( bpvalue != NOT_AN_OFFSET ) {
         bind_unum(record[0],w_index);
         part_pointer.data.u = bpvalue;
         if (0> fseek( odescr.stream, seekptr, 0))
            ERROR(WRIT_ERR, 1);
         write_record(record);
      } 
   }

   /* our last action before closing the file is to mark the execution
    * of our client.
    */
   if (0> fseek(odescr.stream, odescr.exestatoff, 0)
       ||
       exestat != putc(exestat, odescr.stream) )
      ERROR(WRIT_ERR, 6);

}

static void write_at(var,ni,ti,ac,fl)
   char var; /* variable name */
   unsigned ni;  /* name index */
   unsigned ti;  /* type index */
   unsigned ac;  /* attribute code */
   FIELD **fl;	/*  optional field list */
{
   FIELD f;

	/* AT record byte */
   BIND_IBYTE(&f,IB_AT);
   write_field(&f);

	/* Variable, e.g. 'N' for ATN */
   BIND_IBYTE(&f,IBV(var));
   write_field(&f);

   bind_unum(&f,ni); /* name index */
   write_field(&f);

   bind_unum(&f,ti); /* type index */
   write_field(&f);

   bind_unum(&f,ac); /* attribute code, e.g. 50 for creation date/time */
   write_field(&f);

   /* Write the optional fields */
   write_record(fl);
}

/*
 * write_nn does the output for an NN record.  As a courtesy to callers,
 * a pointer to static storage containing the FIELD translation of the index
 * is returned.  This is overwritten on each call.
 */
static FIELD *write_nn(index,id)
   unsigned index;
   FIELD *id;
{
   static  FIELD f;

   BIND_IBYTE(&f, IB_NN);
   write_field(&f);

   bind_unum(&f,index);
   write_field(&f);

   write_field(id);

   return &f;
}

static void write_as(var, flist)
   char var;
   FIELD **flist;
{
   FIELD f;

   BIND_IBYTE(&f,IB_AS);
   write_field(&f);

   BIND_IBYTE(&f,IBV(var));
   write_field(&f);

   write_record(flist);
}


static void write_st(id,type,name)
   HPSECID id;
   char type;
   char *name;
{
   FIELD f;


   BIND_IBYTE(&f,IB_ST);
   write_field(&f);

   bind_unum(&f, id);
   write_field(&f);

   BIND_IBYTE(&f,IBV('A'));
   write_field(&f);
   BIND_IBYTE(&f,IBV('S'));
   write_field(&f);
   switch(type) {
   case HPABST_CODE:
   case	HPABST_ROMDATA:
   case	HPABST_RWDATA:
      BIND_IBYTE(&f,IBV(type));
      write_field(&f);
      break;
   default:
      error_uchar = type;
      ERROR(HPER_BADSTYPE,18);
   }
 
   bind_string(&f, name);
   write_field(&f);
}

/*
 * Write a new NI record, and return the index invented for it.
 *
 * Note that normal I (public) variable indices begin at index 32.
 */
static unsigned write_new_ni(id)
   char *id;
{

   unsigned new_i = next_i++;
   FIELD *flist[4];
   FIELD fni, findex, fid; 

   BIND_IBYTE(flist[0] = &fni, IB_NI);
   bind_unum(flist[1] = &findex, new_i);
   bind_string(flist[2] = &fid, id);
   flist[3] = 0;
   write_record(flist);

   return new_i;
}


static void write_header_part(mname,cpu)
   char *mname;
   int  cpu;
{
   FIELD mb_rectype, modulename, procid;
   FIELD ad_rectype, ad_bitspmau, ad_mauspaddr, ad_byteorder;
   FIELD part_pointer, w_if;
   FIELD *record[5];

   unsigned w_index;

   if(architecture != NULL) {
     cpu = architecture;
   }

   switch(cpu) {
   case F_I960CORE:
      strcpy(archsuff, "CORE");
      break;
   case F_I960KA:
      strcpy(archsuff, "KA");
      break;
   case F_I960KB:
      strcpy(archsuff, "KB");
      break;
   case F_I960CA:
      strcpy(archsuff, "CA");
      break;
   case F_I960JX:
      strcpy(archsuff, "JX");
      break;
   case F_I960HX:
      strcpy(archsuff, "HX");
      break;
   default:
      error_int = cpu;
      ERROR(HPER_ACODE,9);
   }

   /* MB Record */
   BIND_IBYTE(record[0]= &mb_rectype,IB_MB);
   bind_string(record[1]= &procid, arch);
   bind_string(record[2]= &modulename, mname);
   record[3] = 0;
   write_record(record);

   /* AD Record*/
   BIND_IBYTE(record[0]= &ad_rectype,IB_AD);
   bind_unum(record[1]= &ad_bitspmau,8);		/* 8 bits per mau */
   bind_unum(record[2]= &ad_mauspaddr,4);	/* 4 maus per address */
   BIND_IBYTE(record[3]= &ad_byteorder,IBV('L'));/* low addr has least byte */
   record[4] = 0;
   write_record(record);

   /* The file-pointer fields of ASW0 through ASW6 get 
    * written and pinned.
    */
   record[0]= &w_if;
   BIND_IBYTE(record[1]= &part_pointer,IB_4NUM);
   part_pointer.data.u = 0;		/* 4 zero-bytes to be backpatched */
   record[2]=0;
   for (w_index = 0; w_index < 8; w_index++) {
      bind_unum(record[0],w_index);
      if (w_index != 7 )
         mark_pin(record[0], &odescr.partptroff[w_index]);
      else
         mark_pin(record[0], &odescr.meptroff);
      write_as('W', record);
   }
   pin_offsetvar = NULL;
}

static void write_adx_part()
{
   FIELD *flist[3];
   FIELD x1, x2;

   odescr.partoff[PT_ADX] = CUR_OFFSET();

   write_nn(NN_ADX, &null_name);

   bind_unum(flist[0]= &x1,HP695_VN);
   bind_unum(flist[1]= &x2,HP695_VR);
   flist[2] = 0;
   write_at('N', NN_ADX, 0, HPATN_OMFVNO, flist);

   bind_unum(flist[0],OMT_ABSOLUTE);
   flist[1] = 0;
   write_at('N', NN_ADX, 0, HPATN_OMFTYP, flist);

   bind_unum(flist[0],OMC_NOFORCE);
   write_at('N', NN_ADX, 0, HPATN_OMFCASE, flist);
}

static void write_env_part(tool,v,time,env,coml,comm)
   int tool;
   HPTOOL_VERS *v;
   unsigned short *time;
   int env;
   char *coml;
   char *comm;
{
   FIELD x[6];
   FIELD id;
   FIELD *flist[7];
   int i;
   
   /* remember part offset */
   odescr.partoff[PT_ENV] = CUR_OFFSET();

   /* Write creation date and time*/
   write_nn(NN_ENV, &null_name);
   for (i=0; i < 6; i++) {
      bind_unum(flist[i]= &x[i], time[i]);
   }
   flist[6] = 0;
   write_at('N', NN_ENV, 0, HPATN_CREDTG, flist);

   /* write command line text if any */
   if (coml != NULL) {
      bind_string(flist[0]= &id, coml);
      flist[1] = 0;
      write_at('N', NN_ENV, 0, HPATN_COMLINE, flist);
   }

   /* Write a pessimistic prediction of the execution status. */
   /* note that flist[1] == 0 */
   bind_unum(flist[0] = &x[0], HPE_FATAL);
   mark_pin(flist[0], &odescr.exestatoff);
   write_at('N', NN_ENV, 0, HPATN_EXESTAT, flist);

   /* Host environment */
   bind_unum(flist[0]= &x[0],env);
   write_at('N', NN_ENV, 0, HPATN_HOSTENV, flist);

   /* Tool and version no */
   bind_unum(flist[0]= &x[0],tool);
   bind_unum(flist[1]= &x[1],v->version);
   bind_unum(flist[2]= &x[2],v->revision);
   if (v->level != '\0')
      bind_unum(flist[3]= &x[3],(unsigned)v->level);
   else
      flist[3] = 0;
   flist[4] = 0;
   write_at('N', NN_ENV, 0, HPATN_TOOLVNO, flist);

   bind_string(flist[0]= &id,comm);
   flist[1] = 0;
   write_at('N', NN_ENV, 0, HPATN_COMMENT, flist);
}

/*
 * (usually) Write the NN/ATN/ASN sequence for a symbol.
 * (for line number symbols) Write the ATN/ASN sequence.
 */
static void write_symbol(cur_n, sym)
   unsigned long cur_n;
   HPSYM *sym;
{
   
   FIELD f_name, f_n;
   FIELD *pf_n;

   /* Write the NN if the symbol isn't a line number */
   if  ( sym->attribute.atnid != HPATN_SRCCOORD ) {
      bind_string(&f_name, sym->name);
      pf_n = write_nn(cur_n, &f_name);
   } else
      bind_unum( pf_n = &f_n, cur_n);

   /* *pf_n is the field-representation of cur_n */

   /* Write the ATN */
   {
      FIELD *atnflist[6]; /* UPDATE: are 5 fields + terminator enough? */
      FIELD f_numelems, f_globflag, f_auto_offset, f_regid,
            f_line, f_col, f_offset, f_control, f_publoc, f_basesize;

      unsigned flisti = 0;
      HPTYPEHANDLE type_index;

      type_index = sym->type;

      /* Fill in the atnflist switched by the attribute id */
      switch ( sym->attribute.atnid ) {

      case HPATN_SRCCOORD:
         bind_unum(
            atnflist[flisti++]= &f_line,
            sym->attribute.att.srccoord[0]);

         /* UPDATE: 695 V4.0's advice is to represent omitted column info
          * by a null field (0x80?).  We do not have the expressive
          * power in att to determine whether srccoord[1] is given or omitted,
          * so we'll just always omit it now.  Additional problem--dumper
          * chokes on 0x80<, so use 0 for now.
          */
	 /* 8/5/92 Input from MRI/AMC indicates that a better value
	  * for the column entry is a 1.  SO changed to be 1.
	  */
         atnflist[flisti++]= column_zero ? &zero : &one;
         break;

      case HPATN_COMPGLOBAL: /* no addnl fields */
      case HPATN_COMPSTAT:   /* no addnl fields */
         break;

      case HPATN_ASMSTAT: /* ASMSTAT_ATTRS define addnl fields */
         bind_unum(
            atnflist[flisti++]= &f_numelems,
            sym->attribute.att.asmstat.numelems);
         bind_unum(
            atnflist[flisti++]= &f_globflag, 
            (sym->attribute.att.asmstat.globflag?1:0));
         break;

      /* _AUTO and _REGVAR are needed for debugger functionality
       * but not emulator functionality.
       */
      case HPATN_AUTO: /* auto_offset defines addnl field */
         bind_unum(
            atnflist[flisti++]= &f_auto_offset,
            sym->attribute.att.auto_offset);
         break;

      case HPATN_REGVAR: /* regid defines addnl field */
      case HPATN_LOCKREG:
         bind_unum(
            atnflist[flisti++]= &f_regid,
            sym->attribute.att.regid);
         break;

      case HPATN_BASVAR:
	 /* x1: Offset value */
	 bind_unum(atnflist[flisti++] = &f_offset,
		   sym->attribute.att.base.offset);
	 /* x2: Control number */
	 bind_unum(atnflist[flisti++] = &f_control,
		   sym->attribute.att.base.control);
	 /* x3: Pulic/Local Indicator */
	 bind_unum(atnflist[flisti++] = &f_publoc,
		   sym->attribute.att.base.global != 0);
	 /* x4: Memory space indicator; default 0x80 */
	 atnflist[flisti++] = &null_field; /* Omit memspace indicator */
	 /* x5: number of MAUs for BASE value */
	 bind_unum(atnflist[flisti++] = &f_basesize,
		   sizeof(LOGADR960));
	 break;
      /*
       * the following are never expected here.
       */
      default:
         error_int = sym->attribute.atnid;
         ERROR(HPER_BADATT, 27);
      }
      atnflist[flisti] = 0;
      write_at('N', cur_n, type_index, sym->attribute.atnid, atnflist);
   }

   /* Write the ASN if necessary */

   switch ( sym->attribute.atnid ) {
      FIELD f_asnval;
      FIELD *asnrec[3];

   case HPATN_COMPGLOBAL:
   case HPATN_COMPSTAT:  
   case HPATN_ASMSTAT:
   case HPATN_SRCCOORD:
   case HPATN_BASVAR:
      asnrec[0] = pf_n;
      bind_unum(asnrec[1]= &f_asnval, sym->asnval);
      asnrec[2] = 0;
      write_as('N', asnrec);
      break;

   case HPATN_AUTO:
   case HPATN_REGVAR:
   case HPATN_LOCKREG:
      /* these classes do not take an ASN */
      break;

   /* UPDATE: when attributes other than the above are supported,
    * their cases will be placed here; for the present they will
    * have been eliminated by the ERROR exit above.
    */
   }

}


static void module_init()

{
   static unsigned init_count = 0;

   int i;


   if ( init_count != 0 ) {
      /* must clean up from the last initialization */
      if ( client_name != NULL ) 
         free(client_name);
      free_seclist();
      if (odescr.name)
         free(odescr.name);
      if (error_prefix)
         free(error_prefix);
      while (debug_stack->type != DBLK_BOTTOM)
         free(pop_debug_stack());
      if (debug_base.data.srcmod_name)
         free(debug_base.data.srcmod_name);
   }

   debug_base.data.srcmod_name = NULL;

   next_i = NI_FIRSTDYN;
   /* next_n = NN_FIRSTDYN; *//* changed to make PUBLIC NN's */
				  /* unique from DEBUG NN's */
   next_n = init_n;

   next_handle = DEFHANDLE_FIRST;

   error_prefix = NULL;

   bind_string(&null_name,""); /* initialize the union to be z-len string */
   bind_unum(&zero,0);
   bind_unum(&one,1);
   null_field.ibyte = IB_NULL;

   exestat = HPE_SUCCESS;
   seclist = NULL;		/* no sections defined */
   client_name = NULL;		/* client name unknown */
   last_reptd_error = last_ierr = NOT_AN_ERROR;	/* initialize to no-error */

   /* output file description struct init */
   odescr.name = NULL;
   odescr.stream = NULL;
   odescr.meoff = odescr.meptroff = odescr.exestatoff = NOT_AN_OFFSET;
   for (i=0; i<HPPARTS; i++) {
      odescr.partptroff[i] = odescr.partoff[i] = NOT_AN_OFFSET;
   }

   pin_offsetvar = NULL;

   strcpy(archsuff, "??");
   
   ++init_count;
}

/*
 * Define a NN/TY pair in the B1 block of the debug part.
 * Note that there may not exist a B1 at the invocation; so we have
 * to check and possibly create one.
 */
static void deftype(def, handle)
   HPHITYPE *def;
   HPTYPEHANDLE handle;
{
   char *type_name;
   FIELD f;
   FIELD *pf_n;

   /* Check for a module typedef block */
   if (debug_stack->type != DBLK_MTDEF) {
      DBLK mtdef_blk;

      mtdef_blk.type = DBLK_MTDEF;
      open_dblk(&mtdef_blk);
   }
   /* We are in the context of a MTDEF block */

   type_name = def->name;
   if (type_name == NULL)
      type_name = &nul;  /* Normalize the NULL/pointer-to-NUL case; either way
                          * the type does not have a name.
                          */

   bind_string(&f, type_name);
   pf_n = write_nn(next_n++, &f);
   
   BIND_IBYTE(&f, IB_TY);
   write_field(&f);
   bind_unum(&f, handle);
   write_field(&f);
   BIND_IBYTE(&f, IBV('N'));
   write_field(&f);
   write_field(pf_n);
   
   /* Now to handle the variable fields ... */
   
   switch (def->selector) {
      
   case HPHITY_UNK:
      bind_unum(&f, HPHITY_UNK);
      write_field(&f);
      bind_unum(&f, def->descriptor.unk);
      write_field(&f);
      break;
      
   case HPHITY_CENUM:
   case HPHITY_STRUCT:
   case HPHITY_UNION:
      bind_unum(&f, def->selector /* HPHITY_CENUM, _STRUCT or _UNION */);
      write_field(&f);
      if (def->selector == HPHITY_CENUM ) {
	 /* null-name indicates that the size of the enum follows */
	 write_field(&null_name);
      }
      bind_unum(&f, def->descriptor.tag.size);
      write_field(&f);

      {
	 unsigned curindex;
	 TAGMEM *curmem;

	 for (curindex = 0;
	      curindex < def->descriptor.tag.membcount;
	      curindex++ ) {
            curmem = & def->descriptor.tag.member[curindex] ;
		 
            /* enumconst, or member name */
	    bind_string(&f, curmem->name);
	    write_field(&f);
		 
	    if (def->selector != HPHITY_CENUM ) {
	       /* HPHITY_STRUCT || HPHITY_UNION */
	       bind_unum(&f, curmem->type);
	       write_field(&f);
	    }
		 
	    /* enumconst, or MAU offset or bit offset */
	    bind_unum(&f, curmem->value);
	    write_field(&f);
	 }
      }
      break;
      
   case HPHITY_PTR:
      bind_unum(&f, HPHITY_PTR);
      write_field(&f);
      bind_unum(&f, def->descriptor.ptr);
      write_field(&f);
      break;
      
   case HPHITY_CARR:
      bind_unum(&f, HPHITY_CARR);
      write_field(&f);
      bind_unum(&f, def->descriptor.carr.membtype);
      write_field(&f);
      bind_unum(&f, def->descriptor.carr.high_bound);
      write_field(&f);
      break;
      
   case HPHITY_BITFLD:
      bind_unum(&f, HPHITY_BITFLD);
      write_field(&f);
      {
	 HPTYSTR_BITFLD *bitfld = &def->descriptor.bitfld;

	 bind_unum(&f, bitfld->is_signed != '\0');
	 write_field(&f);
	 bind_unum(&f, bitfld->size);
	 write_field(&f);
	 if (bitfld->has_basetype != '\0') {
	    bind_unum(&f, bitfld->basetype);
	    write_field(&f);
	 }
      }
      break;

   case HPHITY_PROCWI:
      bind_unum(&f, HPHITY_PROCWI);
      write_field(&f);
      {
	 HPTYSTR_PROCWI *procwi = &def->descriptor.procwi;
	 int argcnt = procwi->argcnt;
	 HPTYSTR_ARG *curarg;

	 bind_unum(&f, procwi->attribute);
	 write_field(&f);
	 write_field(&zero); /* UPDATE: We designated frame type 0.
			      * If/When other frame types are possible, this
			      * and the 695_out.h interface must change.
			      */
	 write_field(&zero); /* PUSH Mask = 0; No register mask available */
	 bind_unum(&f, procwi->rtype);
	 write_field(&f);
	 bind_unum(&f, argcnt);
	 write_field(&f);
	 for (curarg = procwi->arglist; argcnt > 0; argcnt--, curarg++) {
	    /* For each type in the arglist, output the type handle */
	    bind_unum(&f, *curarg);
	    write_field(&f);
	 }
	 write_field(&zero); /* procedure level: always 0 for "C" */
      }
      break;

   default:
      ERROR(HPER_BADHITYPE,26);
      /*NOTREACHED*/
   }
}

/*
 * putseg
 *
 * Put a load image segment into the image part.  
 *
 * Callers assure that segment->size > 0, to avoid unnecessary records
 * in the image part.
 */
static int putseg(rz,segment)
   int rz;		/* nonzero => do not read from segment->stream */
   HPIMAGE_SEG *segment;
{
   SHEAD *cur_sec;
   unsigned rem;
   unsigned short written;
   unsigned char buffer[MAXDWRITE];
   FIELD pc;
   FIELD ld_rectype, ld_count, sb_rectype, section_index;
   FIELD re_rectype, re_count;
   FIELD *record[4], *re_rec[3];

   ERRORHANDLER();

   SECID_RANGECHK(segment->id);

   if ((cur_sec = find_sec(segment->id)) == NULL) {
      error_section = segment->id;
      ERROR(HPER_UNKSEC,17);
   }

   if (odescr.partoff[PT_DATA] == NOT_AN_OFFSET) {
      /* this is the 1st 'put' of the load module, so record the part offset */
      odescr.partoff[PT_DATA] = CUR_OFFSET();
   }

   /* current section set by SB(index) */
   BIND_IBYTE(record[0]= &sb_rectype, IB_SB);
   bind_unum(record[1]= &section_index, cur_sec->id);
   record[2] = 0;
   write_record(record);

   /* ASP sets current section PC */
   record[0]=record[1];
   bind_unum(record[1]= &pc,cur_sec->pc);
   write_as('P', record);

   BIND_IBYTE(record[0]= &ld_rectype, IB_LD);
   record[1]= &ld_count; /* value bound below */
   record[2]=0;

   if (rz) {
	   /* Emit an RE repeat count and a load-zero byte to implement
       * a repeating 0 pattern.
       */

      /* RE count */
      BIND_IBYTE(re_rec[0]= &re_rectype,IB_RE);
      bind_unum(re_rec[1]= &re_count,segment->size);
      re_rec[2]=0;
      write_record(re_rec);

      /* LD 1 0 */
      bind_unum(record[1],1);
      record[2]= &zero;
      record[3]=0;
      write_record(record); 

   } else /* read from stream */{

      /* Emit a sequence of LDs, with <= MAXDWRITE byte data fields 
       * taken from the stream.
       */
		for ( rem = segment->size; rem; rem -= written ) {
			written = (rem>MAXDWRITE?MAXDWRITE:rem);
			bind_unum(record[1], written);
			write_record(record); /* LD, n1  */

			if ( written !=
			   fread(buffer, sizeof(unsigned char), 
                                 written, segment->stream)
                        )
				ERROR(ER_FREAD,11);
/*----------------------------------------------------------------------------*/
/* Now write the data.  This is the ONLY EXCEPTION to the rule that
 * all 695 data output goes through write_record or write_field.
 */
			else if ( written !=
			   fwrite(buffer, sizeof(unsigned char), written, 
                                  odescr.stream) 
                        )
				ERROR(WRIT_ERR,12);
/*----------------------------------------------------------------------------*/
			else
				/* the transfer was good */;
		}
   }

   cur_sec->pc += segment->size;
   /* no warnings, and fatals have already exited */
   return NO_ERROR;
}

/*
 * putasmid
 *
 * Emit the information which is part of the BB10 record, but not emitted
 * by open_dblk.  The rationale for splitting up this task is that open_dblk
 * both emits information and maintains the block stack which records
 * where we are in the debug part.  The information emitted by putasmid
 * need not be kept around, so putasmid is called after open_dblk.
 */
static void putasmid(tool, vers, time)
   int tool;
   char *vers;
   long int *time;
{
   FIELD f;

   write_field(&null_name); /* Zero length string ( no input relo obj file ) */
   bind_unum(&f, tool);
   write_field(&f); /* Tool type. */

   /* Emissions of all mandatory fields is complete */

   if (vers == NULL) {
      if (time != NULL) {
	 /* produce a null string to mark the version as omitted, and 
	  * delimit the time which follows.
	  */
	 write_field(&null_name);
      }
   } else {
      /* Version string pointer is non-null.  An empty string ("")
       * is ok to put in explicitly, if that is what vers points to.
       */
      bind_string(&f, vers);
      write_field(&f);
   } /* Version and revision in string format. */
     
   if (time != NULL) {
      /* Write a 6-field time stamp */
      unsigned short t695[6]; int i;
      trantime(*time, t695);
      for (i=0; i<6; i++) {
	 bind_unum(&f, (unsigned long)t695[i]);
	 write_field(&f);
      }
   } else {
      /* NULL time means no more information to write */
   }
}

/* External Data Definitions **************************************************/
char hp_version[] = "$File: 695_out.c $ $Version: 1.32 $";

/* External Function Definitions **********************************************/

/* If a hp_* call returns < 0, this call
 * returns the
 * error code from "asm960/lib/err_msg.h" which was
 * used by this module to print the associated error message.
 */
int hp_err()
{  	
   return last_reptd_error;
}

/*
 * hp_create
 *
 *	Purpose:
 *		Begins the definition of the HP load module and opens the
 *		disk file which will contain it.
 *
 *	Input:
 *	
 *		* Name of client tool to be used in printing 960-tool
 *		  style error messages using "error.c" during future
 *		  operations.
 *		* 960 CPU Architecture Variant (see filehdr.h, valid
 *		  values are F_I960KA, F_I960KB, F_I960CA).
 *		* Load Module Name.
 *		* 695 File Name.
 *		* Module Creation Time in COFF (UNIX) Format.
 *		* Command Line Image (optional).
 *		* Host environment (optional; defaults to this module's
 *		  host environment).
 *		* Comment string (optional).
 *
 *	Tasks:
 *		* Initialize the module.
 *		* Create the output file (or truncate a preexisting file).
 *		* Define the 695 header, AD extension, and Environmental part.
 *
 *	Output:
 *		* < 0 for errors.
 */

int hp_create(input)
   HP695_MODDESCR *input;
{
   unsigned short t695[6];	/* yr,mth,day,hr,min,sec*/

   ERRORHANDLER();
   module_init(); /* initialize module's global internal state */
   client_name = strcpy((char *)chkalloc(strlen(input->client)+1), input->client);
   error_prefix = (char *)chkalloc(strlen(client_name)+1+strlen(module)+1);
   strcat(strcpy(error_prefix,client_name), module);

   odescr.name = strcpy(
      (char*)chkalloc(strlen(input->outputname)+1),
      input->outputname
   );
   odescr.stream = fopen(odescr.name, "w+b");
   if ( odescr.stream == NULL ) {
      ERROR(CRE_FILE,13);
   } 

   write_header_part(input->lmodname,input->cpu);
   write_adx_part();
   trantime(input->timdat, t695);
   write_env_part(
      input->toolid,
      &input->version,
      t695,
      (input->hostenv==HPHOST_695WRTR?MODENV:input->hostenv),
      input->comline,
      input->comment
   );

   /* no error-only fatals can happen and those do not come here. */
   return NO_ERROR;
}


/*
 * hp_close
 *	Purpose:
 *		Concludes the definition of a load module and writes
 *		all information to disk.
 *
 *	Input:
 *		Entry Point of the COFF load module.
 *	
 *	Output:
 *		<0 for file errors.
 *
 *	Tasks:
 *		Write the 695 Trailer Part: ME and ASG records.
 *		Backpatch pointer-fields in the 695 header.
 *		
 */	
int hp_close(entry_point)
   LOGADR960 entry_point;
{
   static FIELD
      as_oparen = {IB_FOPAREN}, 
      as_num, 
      as_cparen = {IB_FCPAREN};
   static FIELD *as_tail[] = {
      /* the AS record could be supported by
       * write_as, if and when expressions are supported by the
       * FIELD type.
       */
      &as_oparen, &as_num, &as_cparen, 0
   };

   FIELD me;


   ERRORHANDLER();

   /* remember the file offset of the trailer part.*/
   odescr.partoff[PT_TRAIL] = CUR_OFFSET();

   /* Write the ASG record; its n1 field needs to be a
    * bracketed expression.
    */
   bind_unum(&as_num,entry_point);
   write_as('G', as_tail);

   /* write the ME record*/
   BIND_IBYTE(&me,IB_ME);
   mark_pin(&me,&odescr.meoff);
   write_field(&me);

   write_backpatches();

   fclose(odescr.stream);

   return NO_ERROR;
}

/*
 * hp_set_exestat
 *
 *	Purpose:
 *		Set the cumulative error level of the 695 file.  The Execution
 *		Status field of the 695 Environmental Part is a function
 *		of the cumulative call-history of this function.
 *
 *	Input:
 *		HPE_* error level from 695_out.h.
 *	
 *	Output: 
 *		None.
 *
 *	Tasks:
 *		If the error level input exceeds the current error status,
 *		the current error status is set to the input.
 *
 *	Note:
 *		If this function is not called before hp_close, the status
 *		written to the 695 output file is HPE_SUCCESS.
 */
void hp_set_exestat(level)
   int level; /* one of HPE_* from 695_out.h */
{
   switch (level) {
   case HPE_SUCCESS:
   case HPE_WARNING:
   case HPE_ERROR:
   case	HPE_FATAL:
      exestat = level;
      break;

   default:;
   }
}

/*
 * hp_creabsect
 *
 *	Purpose:
 *		Define the current absolute section.
 *
 *	Input:
 *		* HPABST_* value; defines absolute section type.
 *		* Section Name string.
 *		* Section Size.  
 *		* Section Base Address.
 *		* A short integer id which the caller will use when referring
 *		  to this section in the future (e.g. when defining
 *		  this section's data and symbols).  It is most straightforward
 *		  to use COFF section numbers for this purpose; but note
 *		  that the special values N_DEBUG, N_ABS and N_UNDEF
 *		  do not make sense in this context.
 *
 *	Output:
 *		<0 for errors; n.b. id out of range (1..0x7FFF).
 *
 *	Note:
 *		* 695 does not define alignment for an absolute section;
 *		  hence there is no input for same in this call.
 *		* A section size of 2**32 cannot be represented.
 */		
int hp_creabsect(name,id,type,size,baseaddr)
   char *name;
   HPSECID id;
   int  type;
   unsigned long size;
   LOGADR960 baseaddr;
{
   SHEAD new_sec;
   FIELD sec_index;
   FIELD sec_size;
   FIELD sec_base;
   FIELD * record[5]  ;


   ERRORHANDLER();

   SECID_RANGECHK(id);

   if (odescr.partoff[PT_SEC] == NOT_AN_OFFSET) {
      /* this is the first section of the part, so define the offset */
      odescr.partoff[PT_SEC] = CUR_OFFSET();
   }

   /* Remember data about the section and init its pc */
   new_sec.name = name;
   new_sec.id = id;
   new_sec.type = type;
   if(amc_mri == TRUE) {
     size++;
   }
   new_sec.size = size;
   new_sec.pc = new_sec.baseaddr = baseaddr;
   insert_sec(alloc_sec(&new_sec));

   /* write ST record: type, index, type code, name. */
   write_st(id,type,name);

   /* write ASS: sec index and section size */
   bind_unum(record[0]= &sec_index, (unsigned short)id);
   bind_unum(record[1]= &sec_size, size);
   record[2] = 0;
   write_as('S', record);

   /* write ASL: sec index and section base */
   bind_unum(record[1]= &sec_base, baseaddr);
   write_as('L', record);

   /* no warnings, and fatals have already exited */
   return NO_ERROR;
}

/*
 * hp_putabsdata
 *	
 *	Purpose:
 *		Put data in the current absolute section.
 *
 *	Entry Condition:
 *		The section with which this data is associated is defined
 *		by the section id.
 *
 *	Input:
 *		The "image segment":
 *			Section id.
 *			Size of the "hunk" of load-text to be inserted.
 *			A stream from which to read the hunk.
 *
 *	Output:
 *		<0 for errors; n.b. id out of range (1..0x7FFF).
 *
 *	Note:
 *		* Multiple calls to hp_putabsdata are permitted, data
 *		begins at the section's base address and is concatenated
 *		by calls with the same id.
 *
 *		* If the hunk of load text is known to be all zeros,
 *		(and possibly does not actually exist as a file image)
 *		hp_putabs0 is preferable.
 *
 *		* A function similar to this one which would take
 *	        load data from a memory buffer
 *		could easily be defined, but is not needed presently.
 *
 *		* There is presently no facility to skip portions of
 *		a section (e.g. by advancing the section's location counter).
 *		
 */

int hp_putabsdata(segment)
   HPIMAGE_SEG *segment;
{

   if (segment->size == 0)
      return 0;
   else
      return putseg(0,segment);

}

/*
 * hp_putabs0
 *	
 *	Purpose:
 *		Fill an absolute section with zero data.
 *
 *	Input:
 *		Size of the "hunk" of zero-load-text to be inserted.
 *
 *	Output:
 *		<0 for errors; n.b. id out of range (1..0x7FFF).
 *
 *	Note:
 *		* This call is a special-purpose version of hp_putabsdata.
 *		
 *		* This call is principally useful for defining the
 *		load-semantics of blocks of
 *		"C" language uninitialized variables.
 *		
 */
hp_putabs0(id,count)
   HPSECID id;
   unsigned long count;
{
   HPIMAGE_SEG seg0 ;

   if (count==0)
      return 0; /* no-operation */
   /* count > 0 */
   seg0.id = id;
   seg0.size = count;
   seg0.stream = NULL;
   return putseg(1, &seg0);
}

/*
 * hp_abort
 *
 *	Purpose:
 *		Called to terminate the definition of a load module and
 *		delete the output file.
 *
 *	Output:
 *		None.
 *
 *	Note:
 *		This module is reinitialized such that an
 *		hp_create-hp_close sequence can be attempted again
 *		if necessary.
 */
void hp_abort()
{
   if (odescr.stream) {
      fclose(odescr.stream);
#ifdef DEBUG
      error_out(error_prefix,CVT_HP_ABORT,28, odescr.name);
#else
      unlink(odescr.name);
#endif
      odescr.stream = NULL;
   }
}


/*
 * hp_openmod
 *
 *	Purpose:
 *		Called to begin processing the symbols associated with
 *		a source module
 *
 *	Input:
 *		Module Name string; e.g. "foo.c" for a "C" translation unit.
 *
 *	Output:
 *		<0 on error.
 *
 *	Tasks:
 *		Installs the module name for the duration of the blocks
 *		associated with this module (e.g. B1,B3,B5,B10 for a high
 *		level group).
 *
 *		Defines the low bound of type handles to be used in
 *		hp_puttype operations.
 *
 *		Resets the current N-index to NN_FIRSTDYN.
 *
 */
int hp_openmod(module_name, first_type_handle)
   char *module_name;
   HPTYPEHANDLE first_type_handle;
{
   static int fake_count = 0;/* number of fake modules which have been emitted*/
   char fake_name[4+1+3]; /* accommodates "fake000" ... "fake999" */
   char *used_module_name; /* either module_name or fake_name */
   ERRORHANDLER();


   /* KLUDGE
    *    This is a temporary fix to handle a particular customer's duplicate
    *    module name 'kludge'.  In the future, we need to make a general
    *    fix here which prevents duplicate module names from being emitted into
    *    695; some consumers (notably MRI's loader) cannot handle duplicate
    *    module names, although others can.
    */

   if (0==strcmp(module_name, "fake")) {
      sprintf(fake_name, "fake%03d", fake_count);
      used_module_name = fake_name;
      fake_count++;
   }  else {
      used_module_name = module_name;
   }
      
   /* end KLUDGE */
   
   
   if (debug_stack->type != DBLK_BOTTOM) {
      /* the stack of debug blocks is not clear; recursive openmod is error */
      ERROR(HPER_RECOPMOD,20);
   } else if (odescr.partoff[PT_DEBUG] == NOT_AN_OFFSET) {
      /* this is the first openmod; mark the beginning of the debug part */
      odescr.partoff[PT_DEBUG] = CUR_OFFSET();
   }

   debug_base.data.srcmod_name = (char *)chkalloc(strlen(used_module_name)+1);
   strcpy(debug_base.data.srcmod_name,used_module_name);

   /* If the first_type_handle is not a valid h/l handle but a bity,
    * the caller doesn't care about where hp_puttype is allocating
    * handles from; start from a default place.  Otherwise start from
    * where the caller tells us.
    */
   if ( IS_BITY(first_type_handle) ) 
      next_handle = DEFHANDLE_FIRST;
   else
      next_handle = first_type_handle;

   /* next_n = NN_FIRSTDYN; */ /* changed to make PUBLIC NN's */
			       /* unique from DEBUG NN's */
   next_n = init_n;

   return NO_ERROR;
}

/*
 * hp_putxsym
 *
 *	Purpose:
 *		Called to define a Public symbol; i.e. a bound external.
 *	Output:
 *		<0 for errors.
 *
 */
int hp_putxsym( sym )
   HPXSYM *sym;
{
   char xtype = sym->bity;
   unsigned iindex;
   FIELD fnumelems, fval, findex;
   FIELD *flist[3];

   ERRORHANDLER();
   if (odescr.partoff[PT_EXT] == NOT_AN_OFFSET) {
      /* this is the first external of the part, so define the offset */
      odescr.partoff[PT_EXT] = CUR_OFFSET();
   }

   if ( xtype != HPBITY_UNK ) {
      switch (xtype) {
      case HPBITY_B:
      case HPBITY_I:
      case HPBITY_M:
      case HPBITY_F:
      case HPBITY_D:
      case HPBITY_K:
      case HPBITY_J:
         break;
      default:
         error_uchar = xtype;
         ERROR(HPER_BADHPTYPE,18);
      }
      bind_unum(flist[0] = &fnumelems, sym->numelems);
      flist[1] = 0;
   }

   /* write NI */
   bind_unum(&findex, iindex = write_new_ni(sym->name));

   /* Write ATI for static assembler symbol */
   write_at('I', iindex, xtype, HPATN_ASMSTAT,((xtype!=HPBITY_UNK)?flist:NULL));

   /* write ASI */
   flist[0] = &findex;
   bind_unum(flist[1] = &fval, sym->value);
   flist[2] = 0;
   write_as('I', flist);
   
   return NO_ERROR;
}

/*
 * hp_openblock
 *	Purpose:
 *		Called to begin processing the symbolic information for
 *		a "C" function or block.  
 *
 *	Input:
 *		Name (null for "C" blocks, non-null for "C" functions).
 *		Global/Local function flag ("C" blocks should be local).
 *		Return type for functions.
 *		Beginning and ending code addresses for the code segment
 *		associated with the block.
 *
 *	Output:
 *		<0 on error.
 *	Notes:
 *		Due to the definition of "C" blocks, there may be more
 *		than one hp_openblock without an intervening hp_closeblock.
 *
 *	Tasks:
 *		If there is no currently open B-3, open one.
 *		Open a B-4 or B-6 to correspond to the function or block.
 *
 *	Bugs:
 *		We have to figure out how to map COFF onto the 'x' and/or 'X'
 *		types described in 695 Table A-1.
 *
 */
int hp_openblock(name, gflag, funcinfo, base)
   char *name;		/* name of function; null for blocks */
   int  gflag;		/* nonzero for global function */
   HPTYPEHANDLE funcinfo; /* 'x' type handle, or 0 if unspecified */
   LOGADR960 base;	/* base address of block */
{

   ERRORHANDLER();
   if (debug_stack->type == DBLK_MTDEF) {
      DBLK *cur = pop_debug_stack(); /* pop the B1 which has module types */

      close_dblk(cur);
      free(cur);  
   }
   /* There is not a B1 below on the debug stack */

   if (debug_stack->type == DBLK_BOTTOM) {
      /* There is no block open at all; implying there is no B3. */
      DBLK hl_blk;

      hl_blk.type = DBLK_MSCOPE;
      open_dblk(&hl_blk);

   }
   /* We are now in the context of a B3. */
   {
      DBLK blk;
      FIELD f_funcname, f_beginoff, f_typeindex;

      blk.type = (gflag?DBLK_GFUNC:DBLK_LFUNC);
      open_dblk(&blk);
      bind_string(&f_funcname,name);
      write_field(&f_funcname); /* name of function or null for blocks */
      write_field(&zero); /* <<==========
          * UPDATE: do not have any info about size
          * of local variable region, and its not clear
          * whether same is required (the feature is used by HP debuggers
          * to traverse stack frames; it would seem that the 960 stack
          * discipline provides this info in other ways).
          */
         

      bind_unum(&f_typeindex, funcinfo);
      write_field(&f_typeindex);  
      bind_unum(&f_beginoff, base);
      write_field(&f_beginoff);
   } 
   return NO_ERROR;

}


int hp_putcompid(tool, typing, vstring, utime)
   unsigned long tool; /* 695 tool code */
   unsigned long typing; /* != 0 means transparent type equivalence */
   char *vstring; /* string for version of tool; NULL if not provided */
   long int *utime; /* ; NULL if not provided */
{
   static MISCARGPTR argv[10] = { {FALSE}, {FALSE}, {FALSE}, 
				  {TRUE},
				  {FALSE}, {FALSE}, {FALSE}, {FALSE}, 
				  {FALSE}, {FALSE}};   

   int argc = 0;
   unsigned long four = 4;
   unsigned long  timel[6];

   {
      jmp_buf env; /* stack state area for ERRORHANDLER */

      ERRORHANDLER();      
      switch (debug_stack->type) {
      
	 case DBLK_MTDEF: {
	    /* We are by definition finished with the MTDEF block.
	     * Close it.
	     */
	 
	    DBLK *cur = pop_debug_stack();
	 
	    close_dblk(cur);
	    free(cur);
	 }
	 /* ... and fall through to build the MSCOPE */
      
	 case DBLK_BOTTOM: {
	    /* We must build the MSCOPE on top of the stack bottom */
	 
	    DBLK hl_blk;
	 
	    hl_blk.type = DBLK_MSCOPE;
	    open_dblk(&hl_blk);
	    break; 
	 }

	 default:
	 /* error; none of the reasonable contexts obtain. */
	 error_int = debug_stack->type;
	 ERROR(HPER_SYNCNTXT,37);
      }
   }   

   argv[argc++].MNUMPTR = &tool;
   argv[argc++].MNUMPTR = &typing;
   argv[argc++].MNUMPTR = &four; /* 4 bytes per pointer */

   if (vstring != NULL ) {
      argv[argc++].MSTRPTR = vstring;

      if (utime != NULL) {
	 unsigned short times[6];

	 trantime(*utime, times);

	 /* Assert: argc == 4 */
	 for ( ; argc < 10; argc++) {
	    argv[argc].MNUMPTR = &timel[argc-4];
	    timel[argc-4] = times[argc-4];
	 }
	 /* Assert: argc == 10 */
      }
      /* Assert: argc == 10  ||  argc == 4 */
   }
   /* Assert: argc == 3  ||  argc == 4  ||  argc == 10 */

   return hp_mark(HPATN_MMISC, HPMISC_COMPID, argc, argv );
}

/*
 * hp_putcsym
 *	Purpose:
 *		Called to define a symbol within the current function and/or
 *		block context (if any) and the current module.
 *		The symbol is in "C" source syntax.  Symbols defined with
 *		hp_putcsym are defined within a high level module block
 *		in the 695 Debug Part.
 *
 *	Input:
 *		The HPSYM structure, which defines a symbol's name, type
 *		and value.
 *
 *	Tasks:
 *
 *		Emit the NN, ATN & ASN record for the symbol into whatever
 *		innermost block is open.
 *		
 */
int hp_putcsym(symbol)
   HPSYM *symbol;
{
   ERRORHANDLER();

   /* If we are not in the context of a GFUNC, LFUNC or MSCOPE,
    * we must open an MSCOPE.  We must also pay attention to the case
    * where an MTDEF is currently open.
    */
   switch (debug_stack->type) {

   case DBLK_MTDEF: {
      /* We are by definition finished with the MTDEF block.  Close it. */
      
         DBLK *cur = pop_debug_stack();
       
         close_dblk(cur);
         free(cur);
      }
      /* ... and fall through to build the MSCOPE */

   case DBLK_BOTTOM: {
      /* We must build the MSCOPE on top of the stack bottom */
      
      DBLK hl_blk;

      hl_blk.type = DBLK_MSCOPE;
      open_dblk(&hl_blk);
      break; 
      }

   case DBLK_MSCOPE:
   case DBLK_GFUNC:
   case DBLK_LFUNC:
      /* We are in the appropriate innermost context for a symbol */
      break;

   default:
      /* error; none of the reasonable contexts obtain. */
      error_int = debug_stack->type;
      ERROR(HPER_SYNCNTXT,33);

   }
   /* We are in an appropriate context to emit the symbol (MSCOPE, GFUNC or
    * LFUNC).
    */

   write_symbol(next_n++, symbol);
   return NO_ERROR;
}

/*
 * hp_putasmid
 *
 * 	Purpose: 
 *		The asm id precedes module sections and asm language symbols
 *		It is emitted as part of the BB10 record.
 *		This call is optional; if it is missing from the sequence of
 *		hp calls, a BB10 will be emitted on demand with tool type
 *		"unknown" preceding the first module section.
 */
int hp_putasmid(tool, vers, time)
   int tool; /* HPTOOL_* from 695_out.h */
   char *vers; /* string representing version number */
   long int *time; /* unix 1970-relative time stamp. */
{
   DBLK asm_blk; /* scope information for a B10 */
   ERRORHANDLER();
   
   /* need to close extant blocks and open an asm block */
   
   while (debug_stack->type != DBLK_BOTTOM) {
      DBLK *cur = pop_debug_stack();
      
      close_dblk(cur);
      free(cur);
   }
   asm_blk.type = DBLK_ASMSC;
   open_dblk(&asm_blk);
   putasmid(tool, vers, time);
   return NO_ERROR;
}

/*
 * hp_putasym
 *	Purpose:
 *		Called to define an assembler-level symbol within the 
 *		current module.  The symbol is restricted to
 *		be a compiler global static, local static or assembler
 *		static.
 *
 *	Input:
 *		The symbol.
 *
 *	Output:
 *		return <0 error.
 *
 *	Tasks:
 *		If a B-10 is not open, close all currently open debug
 *		part blocks and open a B-10 for the current module.
 *
 *		Define the symbol in the current B-10.
 *
 */
int hp_putasym(symbol)
   HPSYM *symbol;
{
   char atnid = symbol->attribute.atnid;

   ERRORHANDLER();
   if (debug_base.data.srcmod_name == NULL)
      /* put on a non-open module */
      ERROR(HPER_CLSDMOD, 22);
   else if (   atnid != HPATN_COMPSTAT
            && atnid != HPATN_COMPGLOBAL
            && atnid != HPATN_ASMSTAT ) {
      /* attempt to put a non-assembler attributed symbol into asm block */
      ERROR(HPER_NOTASMATT, 23);
   } else if (debug_stack->type != DBLK_ASMSC ) {
      DBLK asm_blk;

      /* need to close extant blocks and open an asm block */

      while (debug_stack->type != DBLK_BOTTOM) {
         DBLK *cur = pop_debug_stack();
       
         close_dblk(cur);
         free(cur);
      }

      asm_blk.type = DBLK_ASMSC;
      open_dblk(&asm_blk);
      putasmid(HPTOOL_UNK, NULL, NULL);

   }
   /* We are in the context of an ASMSC block */

   write_symbol(next_n++, symbol);

   return NO_ERROR;
}

/*
 * hp_closeblock
 *	Purpose:
 *		Called to terminate the translation of symbolic information
 *		for the current function or block.
 *
 *	Input: End address of function or block.
 *
 *	Output:
 *		<0 on error.
 *	Tasks:
 *		Close the innermost open B-4 or B-6 block.
 */
int hp_closeblock(end)
   LOGADR960 end;
{
   short top_type = debug_stack->type;

   ERRORHANDLER();
   if (top_type !=  DBLK_GFUNC  &&  top_type != DBLK_LFUNC ) {
      /* The block at the top of the stack isn't a function */
      error_int = top_type;
      ERROR(HPER_FCLOSE,25);
   } else {
      /* pop the function block at the top of the stack */
      DBLK *fblk = pop_debug_stack();

      fblk->data.func_end = end;
      close_dblk(fblk); /* close the function block we found at top. */
      free(fblk);

   }

   return NO_ERROR;
}
/*
 * hp_openline
 *	Purpose:
 *		Intialize line number emission.
 *	Input:
 *		Source path name.
 *	Output:
 *		<0 on error.
 *	Tasks:
 *		If a B-5 block is not currently open, close any open B-6, 
 *		B-4 and B-3 blocks and open a B-5.  When a B-5 is
 *		first opened, an NN-index must be associated with
 *		the block (to be referenced by each ATN/ASN).
 *
 */
int hp_openline(source_path)
   char *source_path;
{
   DBLK sline_blk;      /* block info to push on block stack */
   HPSYM line_symbol;   /* symbol to write */
   FIELD f_source_path; /* field representation of source_path */

   ERRORHANDLER();

   if (debug_base.data.srcmod_name == NULL)
      ERROR(HPER_CLSDMOD, 31);

   while (debug_stack->type != DBLK_BOTTOM) {
      DBLK *cur = pop_debug_stack();
       
      close_dblk(cur);
      free(cur);
   }

   
   sline_blk.type = DBLK_SLINE;
   open_dblk(&sline_blk); /* open the SLINE block */
   bind_string(&f_source_path, source_path);
   write_field(&f_source_path); 
   write_nn(debug_stack->data.n_index = next_n++, &null_name);
   /* The first record in the SLINE block is the NN which defines
    * a single index for the use of ATN/ASN pairs in the block.
    */

   return NO_ERROR;
}


/*
 * hp_putline
 *	Purpose:
 *		Define the location of a particular line number in the
 *		current source path. 
 *	Note:
 *		Due to the sequence in which 695 definitions are
 *		processed, the line number must be source-module relative.
 *		This differs from COFF, where the line numbers are
 *		function-relative.
 *
 *	Input:
 *		The line number and corresponding code address.
 *
 *	Output:
 *		<0 on error.
 *
 *	Tasks:
 *
 *		Add the NN, ATN and ASN for the line number to the B-5.
 *
 */
int hp_putline(line,addr)
   unsigned line;
   LOGADR960 addr;
{
   DBLK sline_blk;
   HPSYM line_symbol;

   ERRORHANDLER();

   /* We are in the context of an SLINE block */
   line_symbol.name = NULL; /* line-symbols do not have names */

   /* line symbols don't have a sensible type; call it built-in-unknown */
   line_symbol.type = HPBITY_UNK;

   /* line symbols DO have a sensible attribute and value */
   line_symbol.attribute.atnid = HPATN_SRCCOORD;
   line_symbol.attribute.att.srccoord[0] = line;
   line_symbol.attribute.att.srccoord[1] = 0; /* column */
   line_symbol.asnval = addr;
   write_symbol(debug_stack->data.n_index, &line_symbol);
    
   return NO_ERROR;
}


/*
 * hp_putmodsect
 *
 *	Purpose:
 *		Define a "module section"; the entity which defines the
 *		piece of a section defined in hp_creabsect used by the 
 *		currently open module.urce module.
 * 
 *	Input:
 *		Section index, module-section base, module-section size.
 *
 *	Output:
 *		<0 on error
 *
 *	Tasks:
 *		If a B-10 is not open, close all currently open debug-part
 *		blocks and open a B-10.
 *
 *		Define the B-11 block.
 */
int hp_putmodsect(id, baseaddr, size, type)
   HPSECID id;
   LOGADR960 baseaddr;
   unsigned long size,type;
{
   ERRORHANDLER();

   if ( debug_stack->type != DBLK_ASMSC ) {
      /* There is not an asm block open */
      DBLK asm_blk;

      while (debug_stack->type != DBLK_BOTTOM) {
         /* close the currently open debug block */
         DBLK *cur = pop_debug_stack();
       
         close_dblk(cur);
         free(cur);
      }

      /* Open the asm block */
      asm_blk.type = DBLK_ASMSC;
      open_dblk(&asm_blk);
      /* ... and write additional fields to complete B10 block begin */
      write_field(&null_name); /* Zero length string for incr. linking */
      write_field(&zero);   /*  Unknown assembler */
   }
   /* there is an asm block on top of the stack */

   /* There is no need to remember the nesting of a module section
    * block, since there are no entities inside one.  However, we
    * need writing primitives; and there is no compelling efficiency
    * reason to avoid the open_dblk..close_dblk algorithm.
    */
   {
      DBLK ms_blk;
      DBLK *p_ms_blk = &ms_blk;
      FIELD f_sindex, f_baseaddr;
 
      ms_blk.type =  DBLK_MODSEC;
      ms_blk.data.msect_size = size;
      open_dblk(p_ms_blk);   /* record header, type, and size byte */
      write_field(&null_name); /* zero-length name */
      zero.ibyte = type;
      write_field(&zero);  /* UPDATE: 0-type means mixture; could do better */
      zero.ibyte = 0;
      bind_unum(&f_sindex, id);
      write_field(&f_sindex); /* section index of module-section */
      bind_unum(&f_baseaddr, baseaddr);
      write_field(&f_baseaddr); /* base address of module-section */
      close_dblk(p_ms_blk = pop_debug_stack()); /* BE and size in MAUs */
      free(p_ms_blk);
   }

   return NO_ERROR;
}

/* 
 * hp_closemod
 *
 *	Purpose:
 *		Terminates processing of the symbolic and line-number
 *		information corresponding to a source module.
 *
 *	Input:
 *		none.
 *
 *	Output:
 *		<0 on error.
 *
 *	Tasks:
 *		Closes all currently open debug-part blocks.
 *		After this call, another module can be defined in
 *		the debug part by calling hp_openmod..hp_closemod.
 */
int hp_closemod()
{

   ERRORHANDLER();

   if (debug_base.data.srcmod_name == NULL)
      ERROR(HPER_CLSDMOD, 21);

   while (debug_stack->type != DBLK_BOTTOM) {
      DBLK *cur = pop_debug_stack();
       
      close_dblk(cur);
      free(cur);
   }
   free(debug_base.data.srcmod_name);
   debug_base.data.srcmod_name = NULL;
   return NO_ERROR;
}

/*
 * hp_puttype
 *
 *	Input:
 *		Define a type which may be referenced by hp_putcsym.
 *
 * 	Output:
 *		Write HPTYPEHANDLE back to caller-determined location, 
 *		for use in constructing
 *		types derived from the type defined by this call. 
 *
 *	        Return <0 on error.
 *
 *	Note:
 *		Callers of this function who also define their own 
 *		handles for use with hp_patchtype
 *		should note that this function generates handles from
 *		small integer values (e.g. 0x100, 0x101, ...) and take care
 *		not to conflict with same.
 *
 *
 */
int hp_puttype(type, handle)
   HPHITYPE *type;		/* The type to be defined */
   HPTYPEHANDLE *handle;	/* Output: handle to the type defined */
{
   ERRORHANDLER();

   deftype(type, *handle = next_handle++ );
   return NO_ERROR;
}

/*
 * hp_patchtype
 *
 *	Purpose: Same routine as hp_puttype, but the caller determines
 *	the handle to be used.  This is useful for doing forward references
 *	to base types in derived type definitions. 
 *
 *	To avoid conflicting with hp_puttype, the caller should invent handles
 *	from large integers downward; the reverse of hp_puttype.
 */
int hp_patchtype(type, handle)
   HPHITYPE *type;		/* The type to be defined.*/
   HPTYPEHANDLE handle;		/* Input: the handle to use for the type */
{

   ERRORHANDLER();
   deftype(type, handle);
   return NO_ERROR;
}

/*
 * hp_mark
 *
 *	Purpose:
 *		Create a  miscellaneous record.  These can
 *		occur in the context of either the Debug Part or (in the
 *		case of VMISC's) the Public Part.
 *
 *	Input:  ATN code differentiating VMISC, PMISC or MMISC.
 *		Miscellaneous record identifying code.
 *		Count of arguments to be accounted for.
 *		Vector of argument pointers, some possibly null.
 *		Vector of flags designating which args are strings.
 *
 *	Output:
 *		Errors.
 *
 *	Tasks:
 *		"Miscellaneous records are composed of groups of NN, ASN and
 *		ATN records which together form a cluster or packet of
 *		information" -- 695 Page B - 2.  Emitting this information
 *		involves:
 *
 *			1. Emitting the NN which provides the index for
 *			the rest of the records in the cluster.
 *
 *			2. Emitting the ATN which defines the misc record.
 *
 *			3. Emitting ATNs and/or ASNs which define the
 *			parameters of the misc record.
 *
 */
int hp_mark(entity, misc_code, argc, argv)
   int entity; /* HPATN_VMISC, HPATN_MMISC or HPATN_PMISC */
   unsigned long misc_code; /* numerical code of misc record */
   int argc; /* count of possible arguments to misc record */
   MISCARGPTR *argv; /* vector of provided and missing arguments */
{
   unsigned ni = next_n++; /* Name index for the records in the cluster */
   FIELD *fl[4]; /* field list for ATN or ASN */
   int fli = 0;  /* field list index */
   FIELD f_argsfollow; /* count of args following initial ATN */
   FIELD f_misc_code; /* numerical code of misc record */
   FIELD f_ni; /* N variable index */
   FIELD f_param; /* misc record parameter */

   ERRORHANDLER();

   /* NN: {$F0}{index}{null_name} */
   f_ni =  * write_nn(ni, &null_name);

   /* Emit the misc-defining ATN */
   bind_unum(fl[fli++] = &f_misc_code, misc_code);
   if(argv->is_string) {
      /* a string in the first parameter can be expressed in the 1st ATN */
      argc--; /* adjust the parameter count */
   }
   bind_unum(fl[fli++] = &f_argsfollow, argc);
   if (argv->is_string) {
      /* Bind the string in the first parameter */

      if (argv->MSTRPTR != NULL)
	 bind_string(fl[fli++] = &f_param, argv->MSTRPTR); /* supplied field */
      else
         fl[fli++] = &null_field; /* omitted field */
      fl[fli] = NULL;
      argv++;
   } else
      fl[fli] = NULL;
   /* ATN: {$F1}{$CE}{index}{$00}{miscatn}{misc-code}{argsfollowing} */
   write_at('N', ni, 0 /*no type index*/, entity, fl);

   /* Emit ATNs and/or ASNs for strings and number parameters respectively */

   while (argc > 0) {
      
      if (argv->is_string) {
	 /* parameter is a string */
	 if (argv->MSTRPTR != NULL) {
	    /* parameter is supplied */
	    bind_string(fl[0] = &f_param, argv->MSTRPTR);
	 } else {
	    /* parameter is omitted */
	    fl[0] = &null_field;
	 }
	 fl[1] = NULL;
	 /* ATN: {$F1}{$CE}{index}{$00}{atnmiscstring}{string} */
	 write_at('N', ni, 0, HPATN_MSTRING, fl);
      } else {
	 /* parameter is a number */
	 fl[0] = &f_ni;
	 if (argv->MNUMPTR != NULL) {
	    /* parameter is supplied */
	    bind_unum(fl[1] = &f_param, *argv->MNUMPTR);
	 } else {
	    /* parameter is omitted */
	    fl[1] = &null_field;
	 }
	 fl[2] = NULL;
	 /* ASN: {$E2}{$CE}{index}{value} */
	 write_as('N', fl);
      } 

      argc--; argv++;
   }
   return NO_ERROR;   
}

 
