/* Target-machine dependent code for the Intel 960
   Copyright (C) 1991 Free Software Foundation, Inc.
   Contributed by Intel Corporation.
   examine_prologue and other parts contributed by Wind River Systems.

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Miscellaneous i80960-dependent routines.
   Most are called from macros defined in "tm-i960.h".  */

/* These next two go together. */
#define  HOST
#include "hdil.h"
#ifdef NUM_REGS
#   undef NUM_REGS   /* HDI version of this macro not used in this file. */
#endif

#include "defs.h"
#include <signal.h>
#include <string.h>  /* for strchr() */
#include "symtab.h"
#include "value.h"
#include "frame.h"
#include "floatformat.h"
#include "target.h"
#include "bfd.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdbcmd.h"
#ifdef DOS
#       include "gnudos.h" /* for NSIG */
#endif

/* Two variables that (may) contain section offsets for PIC and PID. */
unsigned int 	picoffset;
unsigned int 	pidoffset;

/* A flag to toggle whether to automatically reset the target when quitting. */
int		autoreset = 1;

/* A flag to toggle whether memory reads and writes are cached */
int 		caching_enabled = 1;

/* The runtime starting entry point */
unsigned long 	i960_startp;

/* Number of active registers for the current i960 target.  May be changed
   in mon960_open depending on which i960 architecture is connected. */
int 		num_regs = TOTAL_POSSIBLE_REGS;

/* Create command lists for commands that have sub-commands */
static struct cmd_list_element *profilelist = NULL;
static struct cmd_list_element *aplinklist = NULL;
static struct cmd_list_element *gmulist = NULL;

/* Some sub-commands have sub-commands */
static struct cmd_list_element *gmuprotlist = NULL;
static struct cmd_list_element *gmudetlist = NULL;

/* number of FP regs in target i960 architecture */
int num_fp_regs;	

/* number of special-function regs in target i960 architecture */
int num_sf_regs;	

/* Remember the most recent commands that defined any GMU registers.
   Also remember the current enable status of all regs. */
struct gmu_cmd_cache
{
    /* number of protection registers actually used */
    int num_pregs; 

    /* number of detection registers actually used */
    int num_dregs;
    
    /* most recent protection register definitions */
    char *preg_arg[HX_MAX_GMU_PROTECT_REGS];

    /* most recent detection register definitions */
    char *dreg_arg[HX_MAX_GMU_DETECT_REGS];
};

/* For convenience, store the GMU command cache in a file-scope global. */
static struct gmu_cmd_cache gmu_cmd_cache;

/* Fault types in string form for printing out unclaimed faults */
char *fault_name_table[MAX_FAULT_TYPES][MAX_FAULT_SUBTYPES];	

/* HDI_CALL calls an HDI function and prints an error message if the 
   return value is anything other than OK.  HDI_CALL can be used as 
   an expression wherever "a" could be used. */
#define HDI_CALL(a) ((a) == OK ? OK : \
    (fprintf_filtered(gdb_stderr, "HDIL error (%d), %s\n", hdi_cmd_stat,  hdi_get_message()), ERR))

/* Sub-command codes for profile_command_1 */
#define PROFILE_PUT	1
#define PROFILE_GET	2
#define PROFILE_CLEAR	3

/* For reading and writing target memory */
#define PROFILE_BUF_SIZE 65536

/* Sub-command codes for gmu_command */
#define GMU_PROTECT	HDI_GMU_PROTECT
#define GMU_DETECT	HDI_GMU_DETECT
#define GMU_DEFINE	0
#define GMU_ENABLE	1
#define GMU_DISABLE	2
#define GMU_DELETE	3

/* A macro to call error() if GMU hardware is not available. */
/* FIXME: for now, this is wired to HX architecture.  
   This should be generalized to use HDI_MON_CONFIG when available. */
#define GMU_SANITY() if (!mon960_connected()) \
	error("Not connected to a mon960 target."); \
    if (mon960_arch() != ARCH_HX) \
	error("No GMU in this architecture.")

/* Should addresses be printed with print commands? */
extern int addressprint;

/* Profile command and children */
static void profile_command PARAMS ((char *, int));
static void profile_put_command PARAMS ((char *, int));
static void profile_get_command PARAMS ((char *, int));
static void profile_clear_command PARAMS ((char *, int));
static void profile_command_1 PARAMS ((int, char *, int));

/* Aplink command and children */
static void aplink_command PARAMS ((char *, int));
static void aplink_enable_command PARAMS ((char *, int));
static void aplink_reset_command PARAMS ((char *, int));
static void aplink_switch_command PARAMS ((char *, int));
static void aplink_wait_command PARAMS ((char *, int));

/* GMU command, children, and grandchildren */
static void gmu_command PARAMS ((char *, int));
static void gmu_protect_command PARAMS ((char *, int));
static void gmu_detect_command PARAMS ((char *, int));
static void gmu_protect_define_command PARAMS ((char *, int));
static void gmu_protect_enable_command PARAMS ((char *, int));
static void gmu_protect_disable_command PARAMS ((char *, int));
static void gmu_detect_define_command PARAMS ((char *, int));
static void gmu_detect_enable_command PARAMS ((char *, int));
static void gmu_detect_disable_command PARAMS ((char *, int));
static void gmu_command_1 PARAMS ((int, int, char *, int, int));
static unsigned long gmu_decode_access PARAMS ((char *));
static void info_gmu PARAMS ((char *, int));
static void dump_gmu_reg_hdr PARAMS ((void));
static void dump_gmu_reg PARAMS ((int, int, HDI_GMU_REG *));
static void maintenance_info_gmu PARAMS ((char *, int));

/* Miscellaneous 80960-specific commands */
static void mcon_command PARAMS ((char *, int));
static void lmadr_command PARAMS ((char *, int));
static void lmmr_command PARAMS ((char *, int));
static void maintenance_backtrace PARAMS ((char *, int));
static void maintenance_raw_backtrace PARAMS ((char *, int));
static char *tokenize PARAMS ((char *, int *, char ***));
CORE_ADDR next_insn PARAMS ((CORE_ADDR, unsigned long *, unsigned long *));
struct frame_saved_regs *
get_dwarf_frame_register_information PARAMS ((struct frame_info *,
					      struct frame_info *,
					      struct partial_symtab *,
					      int *,
					      CORE_ADDR *));

/* gdb960 is always running on a non-960 host.  Check its characteristics.
   This routine must be called as part of gdb initialization.  */

static void
check_host()
{
	int i;

	static struct typestruct {
		int hostsize;		/* Size of type on host		*/
		int i960size;		/* Size of type on i960		*/
		char *typename;		/* Name of type, for error msg	*/
	} types[] = {
		{ sizeof(short),  2, "short" },
		{ sizeof(int),    4, "int" },
		{ sizeof(long),   4, "long" },
		{ sizeof(float),  4, "float" },
		{ sizeof(double), 8, "double" },
		{ sizeof(char *), 4, "pointer" },
	};
#define TYPELEN	(sizeof(types) / sizeof(struct typestruct))

	/* Make sure that host type sizes are same as i960
	 */
	for ( i = 0; i < TYPELEN; i++ ){
		if ( types[i].hostsize != types[i].i960size ){
			printf_unfiltered("sizeof(%s) != %d:  PROCEED AT YOUR OWN RISK!\n",
					types[i].typename, types[i].i960size );
		}

	}
}

/* Examine an i960 function prologue, recording the addresses at which
   registers are saved explicitly by the prologue code, and returning
   the address of the first instruction after the prologue (but not
   after the instruction at address LIMIT, as explained below).

   LIMIT places an upper bound on addresses of the instructions to be
   examined.  If the prologue code scan reaches LIMIT, the scan is
   aborted and LIMIT is returned.  This is used, when examining the
   prologue for the current frame, to keep examine_prologue () from
   claiming that a given register has been saved when in fact the
   instruction that saves it has not yet been executed.  LIMIT is used
   at other times to stop the scan when we hit code after the true
   function prologue (e.g. for the first source line) which might
   otherwise be mistaken for function prologue.

   The format of the function prologue matched by this routine is
   derived from examination of the source to gcc960 1.21, particularly
   the routine i960_function_prologue ().  A "regular expression" for
   the function prologue is given below:

   (lda LRn, g14
    mov g14, g[0-7]
    (mov 0, g14) | (lda 0, g14))?

   (mov[qtl]? g[0-15], r[4-15])*
   ((addo [1-31], sp, sp) | (lda n(sp), sp))?
   (st[qtl]? g[0-15], n(fp))*

   (cmpobne 0, g14, LFn
    mov sp, g14
    lda 0x30(sp), sp
    LFn: stq g0, (g14)
    stq g4, 0x10(g14)
    stq g8, 0x20(g14))?

   (st g14, n(fp))?
   (mov g13,r[4-15])?
*/

/* Macros for extracting fields from i960 instructions.  */

#define BITMASK(pos, width) (((0x1 << (width)) - 1) << (pos))
#define EXTRACT_FIELD(val, pos, width) ((val) >> (pos) & BITMASK (0, width))

#define REG_SRC1(insn)    EXTRACT_FIELD (insn, 0, 5)
#define REG_SRC2(insn)    EXTRACT_FIELD (insn, 14, 5)
#define REG_SRCDST(insn)  EXTRACT_FIELD (insn, 19, 5)
#define MEM_SRCDST(insn)  EXTRACT_FIELD (insn, 19, 5)
#define MEMA_OFFSET(insn) EXTRACT_FIELD (insn, 0, 12)

/* Fetch the instruction at ADDR, returning 0 if ADDR is beyond LIM or
   is not the address of a valid instruction, the address of the next
   instruction beyond ADDR otherwise.  *PWORD1 receives the first word
   of the instruction, and (for two-word instructions), *PWORD2 receives
   the second.  */

#define NEXT_PROLOGUE_INSN(addr, lim, pword1, pword2) \
  (((addr) < (lim)) ? next_insn (addr, pword1, pword2) : 0)

static CORE_ADDR
examine_prologue (ip, limit, frame_addr, fsr, frame_is_leafproc)
    register CORE_ADDR ip;
    register CORE_ADDR limit;
    FRAME_ADDR frame_addr;
    struct frame_saved_regs *fsr;
    int *frame_is_leafproc;
{
  register CORE_ADDR next_ip;
  register int src, dst;
  register unsigned int *pcode;
  unsigned long insn1, insn2;
  int size;
  CORE_ADDR save_addr;
  static unsigned int varargs_prologue_code [] =
    {
       0x3507a00c,	/* cmpobne 0x0, g14, LFn */
       0x5cf01601,	/* mov sp, g14		 */
       0x8c086030,	/* lda 0x30(sp), sp	 */
       0xb2879000,	/* LFn: stq  g0, (g14)   */
       0xb2a7a010,	/* stq g4, 0x10(g14)	 */
       0xb2c7a020	/* stq g8, 0x20(g14)	 */
    };

  /* Accept a leaf procedure prologue code fragment if present.
     Note that ip might point to either the leaf or non-leaf
     entry point; we look for the non-leaf entry point first:  */

  (*frame_is_leafproc) = 0;
  if ((next_ip = NEXT_PROLOGUE_INSN (ip, limit, &insn1, &insn2))
      && ((insn1 & 0xfffff000) == 0x8cf00000         /* lda LRx, g14 (MEMA) */
	  || (insn1 & 0xfffffc60) == 0x8cf03000))    /* lda LRx, g14 (MEMB) */
      {
	  (*frame_is_leafproc) = 1;
	  next_ip = NEXT_PROLOGUE_INSN (next_ip, limit, &insn1, &insn2);
      }

  /* Now look for the prologue code at a leaf entry point:  */

  if (next_ip
      && (insn1 & 0xff87ffff) == 0x5c80161e         /* mov g14, gx */
      && REG_SRCDST (insn1) <= G0_REGNUM + 7)
    {
	(*frame_is_leafproc) = 1;

	/* Check for the special case of an argument block as the 
	   first parameter.  In this case "mov g14, gx" is the entire
	   prologue (i.e. limit has been hit after one insn.) */
	if (next_ip == limit)
		return next_ip;

	/* OK, we must be within a leafproc prologue. */
	if ((next_ip = NEXT_PROLOGUE_INSN (next_ip, limit, &insn1, &insn2))
	    && (insn1 == 0x8cf00000                   /* lda 0, g14 */
		|| insn1 == 0x5cf01e00))              /* mov 0, g14 */
	    {
		ip = next_ip;
		next_ip = NEXT_PROLOGUE_INSN (ip, limit, &insn1, &insn2);
		(*frame_is_leafproc) = 0;
	}
    }

	/* If something that looks like the beginning of a leaf prologue
	   has been seen, but the remainder of the prologue is missing, bail.
     We don't know what we've got.  */

	if (*frame_is_leafproc)
    return (ip);
	  
  /* Accept zero or more instances of "mov[qtl]? gx, ry", where y >= 4.
     This may cause us to mistake the moving of a register
     parameter to a local register for the saving of a callee-saved
     register, but that can't be helped, since with the
     "-fcall-saved" flag, any register can be made callee-saved.  */

  while (next_ip
	 && (insn1 & 0xfc802fb0) == 0x5c000610
	 && (dst = REG_SRCDST (insn1)) >= (R0_REGNUM + 4))
    {
      src = REG_SRC1 (insn1);
      size = EXTRACT_FIELD (insn1, 24, 2) + 1;
      save_addr = frame_addr + ((dst - R0_REGNUM) * 4);
      while (size--)
	{
	    set_fsr_address(fsr,src,save_addr);
	    src++;
	    save_addr += 4;
	}
      ip = next_ip;
      next_ip = NEXT_PROLOGUE_INSN (ip, limit, &insn1, &insn2);
    }

  /* Accept an optional "addo n, sp, sp" or "lda n(sp), sp".  */

  if (next_ip &&
      ((insn1 & 0xffffffe0) == 0x59084800	/* addo n, sp, sp */
       || (insn1 & 0xfffff000) == 0x8c086000	/* lda n(sp), sp (MEMA) */
       || (insn1 & 0xfffffc60) == 0x8c087400))	/* lda n(sp), sp (MEMB) */
    {
      ip = next_ip;
      next_ip = NEXT_PROLOGUE_INSN (ip, limit, &insn1, &insn2);
    }

  /* Accept zero or more instances of "st[qtl]? gx, n(fp)".  
     This may cause us to mistake the copying of a register
     parameter to the frame for the saving of a callee-saved
     register, but that can't be helped, since with the
     "-fcall-saved" flag, any register can be made callee-saved.
     We can, however, refuse to accept a save of register g14,
     since that is matched explicitly below.  */

  while (next_ip &&
	 ((insn1 & 0xf787f000) == 0x9287e000      /* stl? gx, n(fp) (MEMA) */
	  || (insn1 & 0xf787fc60) == 0x9287f400   /* stl? gx, n(fp) (MEMB) */
	  || (insn1 & 0xef87f000) == 0xa287e000   /* st[tq] gx, n(fp) (MEMA) */
	  || (insn1 & 0xef87fc60) == 0xa287f400)  /* st[tq] gx, n(fp) (MEMB) */
	 && ((src = MEM_SRCDST (insn1)) != G14_REGNUM))
    {
      save_addr = frame_addr + ((insn1 & BITMASK (12, 1))
				? insn2 : MEMA_OFFSET (insn1));
      size = (insn1 & BITMASK (29, 1)) ? ((insn1 & BITMASK (28, 1)) ? 4 : 3)
	                               : ((insn1 & BITMASK (27, 1)) ? 2 : 1);
      while (size--)
	{
	    set_fsr_address(fsr,src,save_addr);
	    src++;
	    save_addr += 4;
	}
      ip = next_ip;
      next_ip = NEXT_PROLOGUE_INSN (ip, limit, &insn1, &insn2);
    }

  /* Accept the varargs prologue code if present.  */

  size = sizeof (varargs_prologue_code) / sizeof (int);
  pcode = varargs_prologue_code;
  while (size-- && next_ip && *pcode++ == insn1)
    {
      ip = next_ip;
      next_ip = NEXT_PROLOGUE_INSN (ip, limit, &insn1, &insn2);
    }

  /* Accept an optional "st g14, n(fp)".  */

  if (next_ip &&
      ((insn1 & 0xfffff000) == 0x92f7e000	 /* st g14, n(fp) (MEMA) */
       || (insn1 & 0xfffffc60) == 0x92f7f400))   /* st g14, n(fp) (MEMB) */
    {
	set_fsr_address(fsr,G14_REGNUM, frame_addr + ((insn1 & BITMASK (12, 1))
				            ? insn2 : MEMA_OFFSET (insn1)));
	ip = next_ip;
	next_ip = NEXT_PROLOGUE_INSN (ip, limit, &insn1, &insn2);
    }

  /* Accept zero or one instance of "mov g13, ry", where y >= 4.
     This is saving the address where a struct should be returned.  */

  if (next_ip
      && (insn1 & 0xff802fbf) == 0x5c00061d
      && (dst = REG_SRCDST (insn1)) >= (R0_REGNUM + 4))
    {
      save_addr = frame_addr + ((dst - R0_REGNUM) * 4);
      set_fsr_address(fsr,G0_REGNUM+13,save_addr);
      ip = next_ip;
#if 0  /* We'll need this once there is a subsequent instruction examined. */
      next_ip = NEXT_PROLOGUE_INSN (ip, limit, &insn1, &insn2);
#endif
    }

  return (ip);
}

/* Given an ip value corresponding to the start of a function,
   return the ip of the first instruction after the function 
   prologue.  */

CORE_ADDR
skip_prologue (ip)
     CORE_ADDR (ip);
{
  struct frame_saved_regs saved_regs_dummy;
  struct symtab_and_line sal;
  CORE_ADDR limit;
  int frame_is_leafproc_dummy;

  sal = find_pc_line (ip, 0);
  limit = (sal.end) ? sal.end : 0xffffffff;

  return (examine_prologue (ip, limit, (FRAME_ADDR) 0, &saved_regs_dummy, &frame_is_leafproc_dummy));
}

/*
 * Called as FRAME_CHAIN_VALID from get_prev_frame_info(). 
 * FRAME_CHAIN_VALID is defined in tm-i960.h.
 *
 * Attention maintainers: when adding a new i960 target, write a routine
 * someother_frame_chain_valid(chain,curframe);
 * and add a call to it below.
 */
int
frame_chain_valid (chain, curframe)
    unsigned int chain;
    FRAME curframe;
{
    extern struct target_ops mon960_ops;

    if (current_target == &mon960_ops)
	return mon960_frame_chain_valid(chain,curframe);
    else
    {
	error("No target set or current target does not have stack.");
	return(0);
    }
}

/* Put here the code to store, into a struct frame_saved_regs,
   the addresses of the saved registers of frame described by FRAME_INFO.
   This includes special registers such as pc and fp saved in special
   ways in the stack frame.  sp is even more special:
   the address we return for it IS the sp for the next frame.

   We cache the result of doing this in the frame_cache_obstack, since
   it is fairly expensive.  */

void
frame_find_saved_regs (fi, fsr, ra)
    struct frame_info *fi;
    struct frame_saved_regs *fsr;
    CORE_ADDR *ra;
{
  register CORE_ADDR next_addr;
  register CORE_ADDR *saved_regs;
  register int regnum;
  register struct frame_saved_regs *cache_fsr;
  extern struct obstack frame_cache_obstack;
  CORE_ADDR ip;
  struct symtab_and_line sal;
  CORE_ADDR limit;
  int frame_is_leafproc;

  if (ra)
	  *ra = 0;
  if (!fi->fsr) {
      struct frame_info *next_frame = get_next_frame(fi);
      struct partial_symtab *psym = find_pc_psymtab(fi->pc);

      if (psym && psym->objfile && psym->objfile->obfd && psym->objfile->obfd->xvec &&
	  psym->objfile->obfd->xvec->flavour == bfd_target_elf_flavour)
	      fi->fsr = get_dwarf_frame_register_information(fi,get_next_frame(fi),psym,&frame_is_leafproc,
							     ra);
  }
  if (!fi->fsr) {
      cache_fsr = (struct frame_saved_regs *)
	      obstack_alloc (&frame_cache_obstack,
				 sizeof (struct frame_saved_regs));
      memset (cache_fsr, '\0', sizeof (struct frame_saved_regs));
      fi->fsr = cache_fsr;

      /* Find the start and end of the function prologue.  If the PC
	 is in the function prologue, we only consider the part that
	 has executed already.  */
         
      ip = get_pc_function_start (fi->pc);
      sal = find_pc_line (ip, 0);
      limit = (sal.end && sal.end < fi->pc) ? sal.end: fi->pc;
      
      examine_prologue (ip, limit, fi->frame, cache_fsr, &frame_is_leafproc);

      /* Record the addresses at which the local registers are saved.
	 Strictly speaking, we should only do this for non-leaf procedures,
	 but no one will ever look at these values if it is a leaf procedure,
	 since local registers are always caller-saved.  */

      next_addr = read_memory_integer(fi->frame,4);
      for (regnum = R0_REGNUM; regnum <= R15_REGNUM; regnum++)
	  {
	      set_fsr_address(fi->fsr,regnum,next_addr);
	      next_addr += 4;
	  }
  }

  *fsr = *fi->fsr;
  set_fsr_address(fsr,IP_REGNUM,get_fsr_address(fsr,RIP_REGNUM));
  if (!fi->inlin) {
      if (!frame_is_leafproc) {
	  set_fsr_address(fsr,SP_REGNUM,fi->frame);
	  set_fsr_address(fsr,FP_REGNUM,fi->frame);
      }
  }
}

/* Return the address of the argument block for the frame
   described by FI.  Returns 0 if the address is unknown.  */

CORE_ADDR
frame_args_address (fi, must_be_correct)
     struct frame_info *fi;
{
  register FRAME frame;
  struct frame_saved_regs fsr;
  CORE_ADDR ap;

#ifdef IMSTG
  value_ptr val = NULL;
  struct symbol *sym = (struct symbol *)NULL;
  struct block *blk;
#endif

  frame = FRAME_INFO_ID (fi);
#ifdef IMSTG
  blk = get_frame_block(frame);
#else
  blk = get_current_block();
#endif

#ifdef IMSTG
  /* gcc960 2.2 and IC960 4.X put out a local symbol ".argptr" which specifies
   * the base address of non-register parameters. .argptr can be a register or
   * a memory location.
   *
   * To maintain compatibility with load files generated with previous versions
   * of gcc960, if .argptr can't be found we default to G14 - the old, fixed
   * location.
   */

  /* check for and use .argptr, if present */

  sym = lookup_symbol(".argptr", blk, VAR_NAMESPACE,
                      (int *)NULL, (struct symtab **)NULL);
  if (sym != (struct symbol *)NULL)
    val = read_var_value(sym, frame);

  if (val != NULL)
  {
     /* the contents field of value val is always in TARGET byte order,
      * so swap to host byte order.
      */
     ap = *(CORE_ADDR *)VALUE_CONTENTS(val);
     SWAP_TARGET_AND_HOST(&ap, sizeof(CORE_ADDR));
  }
  else
  { /* use G14 */
  /* If g14 was saved in the frame by the function prologue code, return
     the saved value.  If the frame is current and we are being sloppy,
     return the value of g14.  Otherwise, return zero.  */
#endif

  get_frame_saved_regs (fi, &fsr);
  if (get_fsr_address(&fsr,G14_REGNUM))
	  ap = read_memory_integer(get_fsr_address(&fsr,G14_REGNUM),4);
  else {
    if (must_be_correct)
      return 0;			/* Don't cache this result */
    if (get_next_frame (frame))
      ap = 0;
    else
      ap = read_register (G14_REGNUM);
    if (ap == 0)
      ap = fi->frame;
  }
#ifdef IMSTG
}
#endif
  fi->arg_pointer = ap;		/* Cache it for next time */
  return ap;
}

/* Return the address of the return struct for the frame
   described by FI.  Returns 0 if the address is unknown.  */

CORE_ADDR
frame_struct_result_address (fi)
     struct frame_info *fi;
{
  register FRAME frame;
  struct frame_saved_regs fsr;
  CORE_ADDR ap;

  /* If the frame is non-current, check to see if g14 was saved in the
     frame by the function prologue code; return the saved value if so,
     zero otherwise.  If the frame is current, return the value of g14.

     FIXME, shouldn't this use the saved value as long as we are past
     the function prologue, and only use the current value if we have
     no saved value and are at TOS?   -- gnu@cygnus.com */

  frame = FRAME_INFO_ID (fi);
  if (get_next_frame (frame)) {
    get_frame_saved_regs (fi, &fsr);
    if (get_fsr_address(&fsr,G13_REGNUM))
      ap = read_memory_integer (get_fsr_address(&fsr,G13_REGNUM),4);
    else
      ap = 0;
  } else {
    ap = read_register (G13_REGNUM);
  }
  return ap;
}

/* Return nonzero if the given PC lies within a text section. 
   Return zero if PC does not lie in a text section, or if there
   is not enough information to decide.  Don't rely on any particular 
   section names (i.e. ".text") to decide. */
static int
pc_is_code(pc)
    CORE_ADDR pc;
{
    struct obj_section *sec = find_pc_section(pc);
    if ( ! sec || ! sec->the_bfd_section )
	return 0;
    return (sec->the_bfd_section->flags & SEC_CODE);
}

/* Return address to which the currently executing leafproc will return,
   or 0 if ip is not in a leafproc (or if we can't tell if it is).
  
   Do this by finding the starting address of the routine in which ip lies.
   If the instruction there is "mov g14, gx" (or lda (g14), gx), this
   may be a leafproc and the return address would in register gx.  Well, this is
   true unless the return address points at a RET instruction in the current
   procedure, which indicates that we have a 'dual entry' routine that
   has been entered through the CALL entry point.

   This routine has been extended to look at the first THREE instructions
   for a mov g14, gx.  This is to accomodate Intel's gcc960 runtime library
   functions, which are frequently leafprocs and frequently hand-coded
   assembler.  Yes, this code is ugly, but fortunately it is called rarely,
   and the leafproc return is usually found in the first instruction anyway. */
   
CORE_ADDR
leafproc_return (ip)
     CORE_ADDR ip;	/* ip from currently executing function	*/
{
    register struct minimal_symbol *msymbol;
    char *p;
    int dst;
    unsigned long insn1, insn2, insn3;
    CORE_ADDR return_addr;

    if ((msymbol = lookup_minimal_symbol_by_pc (ip)) != NULL)
    {
	if (next_insn (SYMBOL_VALUE_ADDRESS (msymbol), &insn1, &insn2))
	{
	    if ((insn1 & 0xff87ffff) == 0x5c80161e /* mov g14, gx */
		|| (insn1 & 0xff87ffff) == 0x8c879000) /* lda (g14), gx */
	    {

		/* Get the return address.  If the "mov g14, gx" 
		   instruction hasn't been executed yet, read
		   the return address from g14; otherwise, read it
		   from the register into which g14 was moved.  */
		return_addr =
		    read_register (ip == SYMBOL_VALUE_ADDRESS (msymbol) ?
				   G14_REGNUM : REG_SRCDST (insn1));

		/* If return address is zero, then we are not a leafproc
		   so return with 0.  Else we may a leafproc, so test to see
		   whether the caller did a "bal" or a "call" to get here.
		   Careful: reading garbage addresses can be deadly on some 
		   hardware, so first make sure that the address is within
		   the range of a valid text section before disassembling it.
		   
		   If we entered this function via call or callx, then the
		   return address will be the address of a "ret" instruction 
		   within the same function. */

		if (return_addr != 0 &&
		    pc_is_code(return_addr) &&
		    next_insn (return_addr, &insn1, &insn2) &&
		    ((insn1 & 0xff000000) != 0xa000000 /* ret */
		     || lookup_minimal_symbol_by_pc (return_addr) != msymbol))
		{
		    if (next_insn(return_addr - 8, &insn1, &insn2) &&
			((insn2 & 0xff000000) == 0x0b000000 /* bal */
			 || (insn1 & 0xff000000) == 0x85000000)) /* balx */
		    {
			return (return_addr);
		    }
		}
	    }
	    else if ((insn2 & 0xff87ffff) == 0x5c80161e /* mov g14, gx */
		     || (insn2 & 0xff87ffff) == 0x8c879000) /* lda (g14), gx */
	    {
		/* Same thing for the second instruction */
		return_addr =
		    read_register (ip == SYMBOL_VALUE_ADDRESS (msymbol)
				   || ip == SYMBOL_VALUE_ADDRESS (msymbol)+4 ?
				   G14_REGNUM : REG_SRCDST (insn2));
		  
		if (return_addr != 0 &&
		    pc_is_code(return_addr) &&
		    next_insn (return_addr, &insn1, &insn2) &&
		    ((insn1 & 0xff000000) != 0xa000000 /* ret */
		     || lookup_minimal_symbol_by_pc (return_addr) != msymbol))
		{
		    if (next_insn(return_addr - 8, &insn1, &insn2) &&
			((insn2 & 0xff000000) == 0x0b000000 /* bal */
			 || (insn1 & 0xff000000) == 0x85000000)) /* balx */
		    {
			return (return_addr);
		    }
		}
	    }
	    else if (next_insn (SYMBOL_VALUE_ADDRESS (msymbol) + 8, 
				&insn1, &insn2))
	    {
		/* Same thing again for the third instruction */
		if ((insn1 & 0xff87ffff) == 0x5c80161e /* mov g14, gx */
		    || (insn1 & 0xff87ffff) == 0x8c879000) /* lda (g14), gx */
		{
		    return_addr =
			read_register (ip == SYMBOL_VALUE_ADDRESS (msymbol) ?
				       G14_REGNUM : REG_SRCDST (insn1));
		    if (return_addr != 0 &&
			pc_is_code(return_addr) &&
			next_insn (return_addr, &insn1, &insn2) &&
			((insn1 & 0xff000000) != 0xa000000 /* ret */
			 || lookup_minimal_symbol_by_pc (return_addr) != msymbol))
		    {
			if (next_insn(return_addr - 8, &insn1, &insn2) &&
			    ((insn2 & 0xff000000) == 0x0b000000 /* bal */
			     || (insn1 & 0xff000000) == 0x85000000)) /* balx */
			{
			    return (return_addr);
			}
		    }
		}
	    }
	}
    }
    return (0);
}

/* Immediately after a function call, return the saved pc.
   Can't go through the frames for this because on some machines
   the new frame is not set up until the new function executes
   some instructions. 
   On the i960, the frame *is* set up immediately after the call,
   unless the function is a leaf procedure.  */

CORE_ADDR
saved_pc_after_call (frame)
     FRAME frame;
{
    struct frame_saved_regs regs;
    CORE_ADDR return_address = 0;

    frame_find_saved_regs(frame,&regs,&return_address);
    if (return_address)
    {
	return return_address;
    }
    else {
	CORE_ADDR saved_pc;
	CORE_ADDR get_frame_pc ();

	saved_pc = leafproc_return (get_frame_pc (frame));
	if (!saved_pc)
		saved_pc = FRAME_SAVED_PC (frame);
	return (saved_pc);
    }
}

/* extract_return_value() */

void extract_return_value(t,RB,VB)
    struct type *t;
    char *RB;
    char *VB;
{
    if (TARGET_BIG_ENDIAN) {
	int s = 0;
	int r;

	if (s=(TYPE_LENGTH(t)/4)*4)
		memcpy(VB,RB+REGISTER_BYTE(G0_REGNUM),s);
	if (r=(TYPE_LENGTH(t) % 4))
		memcpy(VB,RB+REGISTER_BYTE(G0_REGNUM)+s+(4-r),r);
    }
    else
	    memcpy(VB,RB+REGISTER_BYTE(G0_REGNUM),TYPE_LENGTH(t));
}

/* push_dummy_arguments() is used to push arguments onto the stack. */

void push_dummy_arguments(nargs, args, sp, struct_return, struct_addr)
    int nargs;
    value_ptr *args;
    unsigned int sp;
    unsigned char struct_return;
    unsigned int struct_addr;
{
    int i;
    int regnum = G0_REGNUM;

    for (i=0;i < nargs;i++) {
	int len = TYPE_LENGTH(VALUE_TYPE(args[i]));

	if (len < 4) {
	    args[i] = value_arg_coerce(args[i]);
	    len = TYPE_LENGTH(VALUE_TYPE(args[i]));
	}
	if (len == 4) {
	    write_register_bytes(REGISTER_BYTE(regnum), VALUE_CONTENTS(args[i]), len);
	    regnum++;
	}
	else if (len == 8) { /* (double) Goes into exactly two registers. */
	    regnum = (regnum + 1) & (~1);
	    write_register_bytes(REGISTER_BYTE(regnum), VALUE_CONTENTS(args[i]), 4);
	    regnum++;
	    write_register_bytes(REGISTER_BYTE(regnum), VALUE_CONTENTS(args[i])+4,4);
	    regnum++;
	}
	else if (len == 16) { /* (long double) Goes into exactly four registers. */
	    regnum = (regnum + 3) & (~3);
	    write_register_bytes(REGISTER_BYTE(regnum), VALUE_CONTENTS(args[i]), 4);
	    regnum++;
	    write_register_bytes(REGISTER_BYTE(regnum), VALUE_CONTENTS(args[i])+4,4);
	    regnum++;
	    write_register_bytes(REGISTER_BYTE(regnum), VALUE_CONTENTS(args[i])+8,4);
	    regnum++;
	    write_register_bytes(REGISTER_BYTE(regnum), VALUE_CONTENTS(args[i])+12,4);
	    regnum++;
	}
	else
		error("push_dummy_arguments got unexpected length: %d\n",len);
    }
}

/* push_dummy_frame starts here. */

static void write_int(a,v)
    char *a;
    int v;
{
    unsigned long t;

    store_unsigned_integer(&t,4,v);
    write_memory(a,&t,4);
}

static int check_args(nargs, args)
    int nargs;
    value_ptr *args;
{
    int i,regnum = G0_REGNUM;

    for (i=0;i < nargs;i++) {
	enum type_code type_code_of_t;

 again:
	switch (type_code_of_t = TYPE_CODE(VALUE_TYPE(args[i]))) {
    case TYPE_CODE_ARRAY:
    case TYPE_CODE_FUNC:
	    args[i] = value_arg_coerce(args[i]);
	    goto again;
    case TYPE_CODE_PTR:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_INT:
    case TYPE_CODE_FLT:
    case TYPE_CODE_CHAR:
	    if (1) {
		int len = TYPE_LENGTH(VALUE_TYPE(args[i]));

		if (len <= 4) { /* Goes into exactly one register. */
		    regnum++;
		}
		else if (len == 8) { /* Goes into exactly two registers. */
		    regnum = (regnum + 1) & (~1);
		    regnum += 2;
		}
		else if (len == 16) { /* Goes into exactly four registers. */
		    regnum = (regnum + 3) & (~3);
		    regnum += 4;
		}
		else
			error("type length: %d not supported in gdb calling fuctions\n",len);
	    }
	    break;
    default:
	    error ("Don't know how to pass type code %d arguments to functions.\n",type_code_of_t);
	}
	if (regnum > G13_REGNUM)
		error ("Too many paratemters for function call on target.\n");

    }
    return 1;
}

void push_dummy_frame(nargs,args)
     int nargs;
     value_ptr *args;
{
    /* Before we make a dummy frame, let's make sure the user is not tryng to
       do something unsupported like pass a structure, or return a big structure,
       or use an arg block. */
    if (check_args(nargs,args)) {
	int i;
	unsigned long fp = read_fp(),sp = read_sp(),pfp;

	pfp = fp;

	/* Make a dummy frame. */

	/* First, we change the previous frame pointer. */
	write_register(R0_REGNUM,fp);  /* We set the pfp to the previous frame ptr. */
	fp = (sp + 0x3f) & ~0x3f; /* The dummy frame starts at the previous sp
					  - aligned for 6 bits. */
	write_fp(fp);

#if 0
	There is no need to save any local registers to the targets stack
		assume the dummy callx insn will do that.

			write_int(fp,pfp);        /* R0 is the previous frame pointer. */

	for (i=SP_REGNUM;i <= R15_REGNUM;i++) {
	    unsigned long reg = read_register(i);

	    write_int(fp+(4*i),reg);
	}
#endif

	sp = fp + 16*4;  /* Sp is just beyond the local registers. */

	/* Save off all global registers on the stack, and bump the sp. */
	if (1) {
	    int j;

	    for (j=0,i=G0_REGNUM;i <= G14_REGNUM;j++,i++) {
		unsigned long reg = read_register(i);

		write_int(sp+(4*j),reg);
	    }

	    sp += 15*4;  /* Sp is just beyond the global registers. */

	    write_sp(sp);
	}
    }
}

/* Discard from the stack the innermost frame,
   restoring all saved registers.  */

pop_frame ()
{
  register struct frame_info *current_fi, *prev_fi;
  register int i;
  CORE_ADDR save_addr;
  CORE_ADDR leaf_return_addr;
  struct frame_saved_regs fsr;
  char local_regs_buf[16 * 4];
  extern int stop_stack_dummy;

  current_fi = get_frame_info (get_current_frame ());

  /* First, undo what the hardware does when we return.
     If this is a non-leaf procedure, restore local registers from
     the save area in the calling frame.  Otherwise, load the return
     address obtained from leafproc_return () into the rip.  */

  leaf_return_addr = leafproc_return (current_fi->pc);
  if (!leaf_return_addr)
    {
      /* Non-leaf procedure.  Restore local registers, incl IP.  */
      prev_fi = get_frame_info (get_prev_frame (FRAME_INFO_ID (current_fi)));
      read_memory (prev_fi->frame, local_regs_buf, sizeof (local_regs_buf));
      write_register_bytes (REGISTER_BYTE (R0_REGNUM), local_regs_buf, 
		            sizeof (local_regs_buf));

      /* Restore frame pointer.  */
      write_register (FP_REGNUM, prev_fi->frame);

      /*FIXME.  In gdb960, this code is ONLY reachabe via return command and
       arbitrary function calls.  Further, we do not support the return command.
       Therefore, we do not need to have the following conditional. */

      if (stop_stack_dummy) {
	  write_register (PC_REGNUM, prev_fi->pc);
	  write_register (RIP_REGNUM, prev_fi->pc);
      }
  }
  else
    {
      /* Leaf procedure.  Just restore the return address into the IP.  */
      write_register (RIP_REGNUM, leaf_return_addr);
    }

  /* Now restore any global regs that the current function had saved. */
  if (!stop_stack_dummy) {
      get_frame_saved_regs (current_fi, &fsr);
      for (i = G0_REGNUM; i < G14_REGNUM; i++)
	  {
	      if (save_addr = get_fsr_address(&fsr,i))
		      write_register (i, read_memory_integer (save_addr, 4));
	  }
  }
  else {
      
      /* In push_dummy_frame() we saved g0-g14 at fp+64+(0*4) -> fp+64+(14*4).
	 Here, we restore them. */

      int j;

      for (j=0,i = G0_REGNUM; i < G14_REGNUM; j++,i++) {
	  write_register (i, read_memory_integer (current_fi->frame+64+(j*4), 4));	  
      }
  }
  /* Flush the frame cache, create a frame for the new innermost frame,
     and make it the current frame.  */

  flush_cached_frames ();
  set_current_frame (create_new_frame (read_register (FP_REGNUM), read_pc ()));
}

/* Given a 960 stop code (fault or trace), return the signal which
   corresponds.  */

enum target_signal
i960_fault_to_signal (fault)
    int fault;
{
  switch (fault)
    {
    case 0: return TARGET_SIGNAL_BUS; /* parallel fault */
    case 1: return TARGET_SIGNAL_UNKNOWN;
    case 2: return TARGET_SIGNAL_ILL; /* operation fault */
    case 3: return TARGET_SIGNAL_FPE; /* arithmetic fault */
    case 4: return TARGET_SIGNAL_FPE; /* floating point fault */

       /* constraint fault.  This appears not to distinguish between
	  a range constraint fault (which should be SIGFPE) and a privileged
	  fault (which should be SIGILL).  */
    case 5: return TARGET_SIGNAL_ILL;

    case 6: return TARGET_SIGNAL_SEGV; /* virtual memory fault */

       /* protection fault.  This is for an out-of-range argument to
	  "calls".  I guess it also could be SIGILL. */
    case 7: return TARGET_SIGNAL_SEGV;

    case 8: return TARGET_SIGNAL_BUS; /* machine fault */
    case 9: return TARGET_SIGNAL_BUS; /* structural fault */
    case 0xa: return TARGET_SIGNAL_ILL; /* type fault */
    case 0xb: return TARGET_SIGNAL_UNKNOWN; /* reserved fault */
    case 0xc: return TARGET_SIGNAL_BUS; /* process fault */
    case 0xd: return TARGET_SIGNAL_SEGV; /* descriptor fault */
    case 0xe: return TARGET_SIGNAL_BUS; /* event fault */
    case 0xf: return TARGET_SIGNAL_UNKNOWN; /* reserved fault */
    case 0x10: return TARGET_SIGNAL_TRAP; /* single-step trace */
    case 0x11: return TARGET_SIGNAL_TRAP; /* branch trace */
    case 0x12: return TARGET_SIGNAL_TRAP; /* call trace */
    case 0x13: return TARGET_SIGNAL_TRAP; /* return trace */
    case 0x14: return TARGET_SIGNAL_TRAP; /* pre-return trace */
    case 0x15: return TARGET_SIGNAL_TRAP; /* supervisor call trace */
    case 0x16: return TARGET_SIGNAL_TRAP; /* breakpoint trace */
    default: return TARGET_SIGNAL_UNKNOWN;
    }
}


/* 
 * Initialize the reg_names array.  This is done statically in infcmd.c,
 * but it must be adjusted each time you connect to mon960, because
 * the register set varies with the particular i960 flavor.  The base
 * code (e.g. registers_info) expects that the registers are all laid out
 * in a contiguous block, numbered [0, NUM_REGS-1].  Thus FP0_REGNUM
 * and the reg_names array must be variable, not constant.
 */

static char *myregnames[] = REGISTER_NAMES;

void
init_reg_names()
{
    register int i;

    /* Re-initialize the reg_names array in case we are just 
       re-connecting to a different target within the same gdb session */
    for ( i = 0; i < 32; ++i )
	reg_names[SF0_REGNUM + i] = myregnames[SF0_REGNUM + i];

    /* Initialize for the current i960 flavor by copying over the 
       SF regnames with FP regnames (if any). */
    for ( i = 0; i < num_fp_regs; ++i )
	reg_names[SF0_REGNUM + num_sf_regs + i] = 
	    reg_names[SF0_REGNUM + 32 + i];
}


/*
 * Support for "regs" command.
 * print_reg(n) prints the value of register #n in this format:
 *
 * nnnnnnnnnnnnnnn 0xhhhhhhhh
 *
 * (no CR/LF)
 */

static void
print_reg(regnum)
    unsigned int regnum;
{
    char raw_buffer[MAX_REGISTER_RAW_SIZE];
    
    printf_filtered("%-15s\t", reg_names[regnum]);
    
    /* Get the data in raw format, then convert also to virtual format.  */
    if (read_relative_register_raw_bytes (regnum, raw_buffer))
    {
	printf ("Invalid register contents\n");
    }
    else
    {
	print_scalar_formatted(raw_buffer, builtin_type_int, 'x', 'w', stdout);
    }
}


/* Intel extension to "info regs".  Print the integer regs, but print
   in two columns; uses less screen scroll than "info regs". */
static void
regs_command(expr, from_tty)
    char *expr;
    int from_tty;
{
    register int i, col2_start;

    if (!target_has_registers)
	error ("The program has no registers now.");
    
    for (i = 0, col2_start = (NUM_REGS - num_fp_regs + 1) / 2; 
	 i < col2_start; ++i)
    {
	print_reg(i);
	printf_filtered("\t");
	if (col2_start + i < NUM_REGS - num_fp_regs)
	    print_reg(col2_start + i);
	printf_filtered("\n");
    }
    gdb_flush(stdout);
}

#if defined(IMSTG)
/* Process PIC/PID command arguments.  If found, remove from the arg
   list so that gdb commands will work as always.  This can be called
   from either or both of exec_file_command and symbol_file_command. */
void
get_pix_options(argv)
char **argv;
{
    char **ap = argv;
    while ( *argv )
    {
	if ( **argv == '-' )
	{
	    /* option */
	    if (STREQ (*argv, "-picoffset") || STREQ (*argv, "-pc"))
	    {
		++argv;
		if ( *argv == NULL )
		    error("Expected PIC offset after %s", *(argv - 1));
		picoffset = (unsigned long) strtol(*argv, NULL, 0);
	    }
	    else if (STREQ (*argv, "-pidoffset") || STREQ (*argv, "-pd"))
	    {
		++argv;
		if ( *argv == NULL )
		    error("Expected PID offset after %s", *(argv - 1));
		pidoffset = (unsigned long) strtol(*argv, NULL, 0);
	    }
	    else if (STREQ (*argv, "-pixoffset") || STREQ (*argv, "-px"))
	    {
		++argv;
		if ( *argv == NULL )
		    error("Expected PIX offset after %s", *(argv - 1));
		picoffset = (unsigned long) strtol(*argv, NULL, 0);
		pidoffset = picoffset;
	    }
	    if ( picoffset || pidoffset )
	    {
		/* Got one; move remaining args forward; We know we have 
		   processed exactly 2 args; See liberty:argv.c for 
		   reassurance about how this argv works. */
		--argv;
		free(*argv);	
		free(*(argv + 1));
		do
		{
		    *argv = *(argv + 2);
		} 
		while ( *argv++ ); 
		argv = ap;
	    }		
	}
	else
	{
	    /* non-option; ignore */
	    ap = ++argv;
	}
    }
}    

/*
 * Profiling commands: these allow you to read and write 2-pass profiling 
 * statistics to and from a file, even when your target system doesn't 
 * have file i/o.  Just fetch/send the info along the serial line, and 
 * use the host's read and write.
 */
static void 
profile_command(args, from_tty)
	char *args;
	int from_tty;
{
	fputs_filtered ("Argument required (put, get, or clear).  Try `help profile'\n", stdout);
}

static void 
profile_put_command(filename, from_tty)
	char *filename;
	int from_tty;
{
	profile_command_1(PROFILE_PUT, filename, from_tty);
}

static void 
profile_get_command(filename, from_tty)
	char *filename;
	int from_tty;
{
	profile_command_1(PROFILE_GET, filename, from_tty);
}

static void 
profile_clear_command(filename, from_tty)
	char *filename;
	int from_tty;
{
	profile_command_1(PROFILE_CLEAR, filename, from_tty);
}

static
void profile_command_1(subcmd, filename, from_tty)
	int subcmd;
	char *filename;
	int from_tty;
{
	struct cleanup		*old_chain;
	struct minimal_symbol 	*msym = NULL;
	int 			data_len = 0;	/* from ___profile_data_length symbol */
	CORE_ADDR		data_start = 0; /* from ___profile_data_start symbol */
	char			*data_file = NULL;
	FILE			*data_fp = NULL;
	char 			*data_buf = NULL;
	int	 		file_data_len, bufnum, buf_size;

  	if ( current_target != &mon960_ops )
		error("Not connected to an i960 target.  Try `help target'.");

	if ( (msym = lookup_minimal_symbol("__profile_data_length", NULL)) == NULL )
		error("No profiling information available in this file.");
	data_len = SYMBOL_VALUE_ADDRESS(msym);

	if ( (msym = lookup_minimal_symbol("__profile_data_start", NULL)) == NULL )
		error("No profiling information available in this file.");
	data_start = (CORE_ADDR) SYMBOL_VALUE_ADDRESS(msym) + pidoffset;

	data_file = filename ? filename : "default.pf";

	data_buf = (char *) xmalloc(data_len < PROFILE_BUF_SIZE ? data_len : PROFILE_BUF_SIZE);
	old_chain = make_cleanup(free, data_buf);

	/* OK, ready to do something */
	switch ( subcmd )
	{
	case PROFILE_GET:
		if ( (data_fp = fopen(data_file, FOPEN_RB)) == NULL )
			perror_with_name(data_file);
		/* 
		 * The length of the profile data is the first 32-bit word
		 * in the file.  Profile files are little-endian by design.
		 * But the host might be big-endian.  Byteswap if needed. 
		 */
		if ( fread((char *) &file_data_len, 4, 1, data_fp) != 1 )
		{
			error("Error reading from profile file: %s.\nCheck that the file is not corrupted.", data_file);
		}
		if ( HOST_BIG_ENDIAN )
			byteswap_within_word((char *) &file_data_len);
		if ( file_data_len != data_len )
			error("Profile file's data size: %d does not match expected size: %d.", file_data_len, data_len);
		if ( from_tty ) 
		    if ( addressprint )
			printf_filtered("Copying %d bytes from %s into target memory at 0x%x.\n", data_len, data_file, data_start);
		    else
			printf_filtered("Copying %d bytes from %s into target memory.\n", data_len, data_file);

		/* 
		 * Read as much of the profile file as will fit into data_buf,
		 * copying into target memory.  Loop until entire file is read.
		 * A complication:  profile files are always written little-
		 * endian.  So byteswap each word within data_buf if target
		 * memory is big-endian.
		 */
		for ( bufnum = 0; data_len > 0; ++bufnum, data_len -= PROFILE_BUF_SIZE )
		{
			buf_size = data_len < PROFILE_BUF_SIZE ? data_len : PROFILE_BUF_SIZE;
			if ( fread(data_buf, buf_size, 1, data_fp) != 1 )
				error("Error reading profile file: %s.\nCheck that the file is not corrupted.\n", data_file);
			if ( TARGET_BIG_ENDIAN )
				byteswap_within_buffer(data_buf, buf_size);
			write_memory(data_start + (bufnum * PROFILE_BUF_SIZE), data_buf, buf_size);
		}
		fclose(data_fp);
		break;

	case PROFILE_PUT:
		if ( (data_fp = fopen(data_file, FOPEN_WB)) == NULL )
			perror_with_name(data_file);
		if ( data_len < 0 )
			error("Invalid profile data length: %d", data_len);
		if ( from_tty )
		    if ( addressprint )
			printf_filtered("Copying %d bytes from target memory at 0x%x into %s.\n", data_len, data_start, data_file);
		    else
			printf_filtered("Copying %d bytes from target memory into %s.\n", data_len, data_file);

		/* 
		 * Write the data length as the first 32-bit word in the file. 
		 * Profile files are little-endian by design.  But the host
		 * might be big-endian.  Byteswap if needed. 
		 */
		file_data_len = data_len;
		if ( HOST_BIG_ENDIAN )
			byteswap_within_word((char *) &file_data_len);
		if ( fwrite((char *) &file_data_len, 4, 1, data_fp) != 1 )
			error("Error writing profile file: %s.\n", data_file);

		/* 
		 * Write as much of the profile file as will fit into data_buf,
		 * copied from target memory.  Loop until entire file is written.
		 * A complication:  profile files are always written little-
		 * endian.  So byteswap each word within data_buf if target
		 * memory is big-endian.
		 */
		for ( bufnum = 0; data_len > 0; ++bufnum, data_len -= PROFILE_BUF_SIZE )
		{
			buf_size = data_len < PROFILE_BUF_SIZE ? data_len : PROFILE_BUF_SIZE;
			read_memory(data_start + (bufnum * PROFILE_BUF_SIZE), data_buf, buf_size);
			if ( TARGET_BIG_ENDIAN )
				byteswap_within_buffer(data_buf, buf_size);
			if ( fwrite(data_buf, buf_size, 1, data_fp) != 1 )
				error("Error writing profile file: %s.", data_file);
		}
		fclose(data_fp);
		break;
	case PROFILE_CLEAR:
		if ( filename )
			warning("File name argument ignored.");
		if ( from_tty )
		    if ( addressprint )
			printf_filtered("Clearing %d bytes in target memory starting at 0x%x.\n", data_len, data_start);
		    else
			printf_filtered("Clearing %d bytes in target memory.\n", data_len);

		/* 
		 * Zero out the profile data area in target memory.
		 */
		buf_size = data_len < PROFILE_BUF_SIZE ? data_len : PROFILE_BUF_SIZE;
		for ( --buf_size; buf_size >= 0; --buf_size )
			data_buf[buf_size] = 0;
		for ( bufnum = 0; data_len > 0; ++bufnum, data_len -= PROFILE_BUF_SIZE )
		{
			buf_size = data_len < PROFILE_BUF_SIZE ? data_len : PROFILE_BUF_SIZE;
			write_memory(data_start + (bufnum * PROFILE_BUF_SIZE), data_buf, buf_size);
		}

		break;
	default:
		break;
	}
	free(data_buf);
	discard_cleanups(old_chain);
} /* end profile_command_1 */

static void 
gmu_command (args, from_tty)
    char * args;
    int from_tty;
{
    fputs_filtered ("Argument required (protect or detect).  Try `help gmu'\n", stdout);
}

static void 
gmu_protect_command (args, from_tty)
    char * args;
    int from_tty;
{
    fputs_filtered ("Argument required (define, enable, or disable).  Try `help gmu protect'\n", stdout);
}

static void 
gmu_detect_command (args, from_tty)
    char * args;
    int from_tty;
{
    fputs_filtered ("Argument required (define, enable, or disable).  Try `help gmu detect'\n", stdout);
}

static void 
gmu_protect_define_command (args, from_tty)
    char * args;
    int from_tty;
{
    gmu_command_1(GMU_PROTECT, GMU_DEFINE, args, 1, from_tty);
}

static void 
gmu_protect_enable_command (args, from_tty)
    char * args;
    int from_tty;
{
    gmu_command_1(GMU_PROTECT, GMU_ENABLE, args, 1, from_tty);
}

static void 
gmu_protect_disable_command (args, from_tty)
    char * args;
    int from_tty;
{
    gmu_command_1(GMU_PROTECT, GMU_DISABLE, args, 1, from_tty);
}

static void 
gmu_detect_define_command (args, from_tty)
    char * args;
    int from_tty;
{
    gmu_command_1(GMU_DETECT, GMU_DEFINE, args, 1, from_tty);
}

static void 
gmu_detect_enable_command (args, from_tty)
    char * args;
    int from_tty;
{
    gmu_command_1(GMU_DETECT, GMU_ENABLE, args, 1, from_tty);
}

static void 
gmu_detect_disable_command (args, from_tty)
    char * args;
    int from_tty;
{
    gmu_command_1(GMU_DETECT, GMU_DISABLE, args, 1, from_tty);
}

static void
gmu_command_1 (subcmd, action, args, from_user, from_tty)
    int subcmd;	/* protect or detect; pass directly to HDI routines */
    int action; /* enable, disable, define */
    char *args; /* remaining cmd line args */
    int from_user; /* user (as opposed to reprogram_gmu) gave this command */
    int from_tty; /* interactive, not from a batch file */
{
    int i, argc = 0;
    char **argv;    /* Unlike main's argv, the first real arg is numbered 0 */
    char *myargs;
    struct cleanup *cleanup_chain = NULL;
    int regnum;
    int syntax = 0;
    HDI_GMU_REG gmureg;
    HDI_GMU_REGLIST gmureglist;

    GMU_SANITY();   /* calls error if no GMU available */

    myargs = tokenize(args, &argc, &argv);
    if (argc > 0)
    {
	cleanup_chain = make_cleanup(free, myargs);
	make_cleanup(free, argv);
    }
    switch (action)
    {
    case GMU_ENABLE:
	if (argc == 1)
	{
	    regnum = atoi(argv[0]);
	    HDI_CALL(hdi_update_gmu_reg(subcmd, regnum, 1));
	}
	else if (argc == 0)
	{
	    /* Enable all of this type of register */
	    int numregs;
	    HDI_CALL(hdi_get_gmu_regs(&gmureglist));
	    numregs = (subcmd == GMU_PROTECT ? 
		       gmureglist.num_pregs :
		       gmureglist.num_dregs);
	    for (i = 0; i < numregs; ++i)
	    {
		HDI_CALL(hdi_update_gmu_reg(subcmd, i, 1));
	    }
	}
	else 
	    syntax = 1;
	break;
    case GMU_DISABLE:
	if (argc == 1)
	{
	    regnum = atoi(argv[0]);
	    HDI_CALL(hdi_update_gmu_reg(subcmd, regnum, 0));
	}
	else if (argc == 0)
	{
	    /* Disable all of this type of register */
	    int numregs;
	    HDI_CALL(hdi_get_gmu_regs(&gmureglist));
	    numregs = (subcmd == GMU_PROTECT ? 
		       gmureglist.num_pregs :
		       gmureglist.num_dregs);
	    for (i = 0; i < numregs; ++i)
	    {
		HDI_CALL(hdi_update_gmu_reg(subcmd, i, 0));
	    }
	}
	else
	    syntax = 1;
	break;
    case GMU_DEFINE:
	if (argc == 4)
	{
	    HDI_GMU_REG gmureg;
	    unsigned long access = gmu_decode_access(argv[1]);
	    value_ptr val;

	    regnum = atoi(argv[0]);
	    if (access == -1)
		syntax = 1;
	    else
	    {
		unsigned long word1, word2;

		/* Parse and evaluate address 1 from command line */
		word1 = parse_and_eval_address(argv[2]);

		/* Parse and evaluate address 2 from command line */
		word2 = parse_and_eval_address(argv[3]);

		/* The meanings of the two words are backwards for the 
		   two register types (sigh) ... For detection registers,
		   the upper bound comes first, then the lower bound, but
		   the most natural command-line syntax is "lower upper".
		   Hence the switcheroo below. */
		switch (subcmd)
		{
		case GMU_PROTECT:
		    gmureg.loword = word1;
		    gmureg.hiword = word2;
		    break;
		case GMU_DETECT:
		    gmureg.loword = word2;
		    gmureg.hiword = word1;
		    break;
		default:
		    break;
		}

		gmureg.access = access;
		if (from_user)
		    gmureg.enabled = 1;
		else
		{
		    /* This was called by reprogram_gmu -- the register
		       may or may not still be enabled.  Query the actual
		       register to find out. */
		    HDI_GMU_REG tmpreg;
		    if (HDI_CALL(hdi_get_gmu_reg(subcmd, regnum, &tmpreg)) == OK)
			gmureg.enabled = tmpreg.enabled;
		    else
			return;
		}

		/* Program the register. */
		if (HDI_CALL(hdi_set_gmu_reg(subcmd, regnum, &gmureg)) == OK)
		{
		    /* This definition succeeded.  If the user is entering
		       this from the keyboard, then give some feedback and
		       enter the raw arg string into the gmu command cache. */
		    if (from_user)
		    {
			char **cache_args;
			switch (subcmd)
			{
			case GMU_PROTECT:
			    cache_args = &gmu_cmd_cache.preg_arg[regnum];
			    if (from_tty)
				printf_filtered("GMU protection register %d defined.\n", regnum);
			    break;
			case GMU_DETECT:
			    cache_args = &gmu_cmd_cache.dreg_arg[regnum];
			    if (from_tty)
				printf_filtered("GMU detection register %d defined.\n", regnum);
			    break;
			default:
			    break;
			}
			if (*cache_args)
			    free(*cache_args);
			*cache_args = (char *) xmalloc (strlen(args) + 1);
			strcpy(*cache_args, args);
		    }
		}
	    }
	}
	else 
	    syntax = 1;
	break;
    default:
	error("Internal error: unknown gmu subcommand: %d", subcmd);
	break;
    }
    if (syntax)
	error("Syntax error; try `help gmu %s %s'",
	      subcmd == GMU_PROTECT ? "protect" : "detect",
	      action == GMU_DEFINE ? "define" :
	      action == GMU_ENABLE ? "enable" : "disable");
    if (cleanup_chain)
	do_cleanups(cleanup_chain);
}

static unsigned long
gmu_decode_access(buf)
    char *buf;
{
    unsigned long access = 0;
    int bitoffset;
    char *c;

    for (c = strtok(buf, ",");
	 c;
	 c = strtok(NULL, ","))
    {
	if (tolower(*c) == 'u')
	    bitoffset = 0;
	else if (tolower(*c) == 's')
	    bitoffset = 4;
	else
	    return -1;
	for (++c; *c; ++c)
	{
	    switch (tolower(*c))
	    {
	    case 'r':
		access |= (1 << (bitoffset + 0));
		break;
	    case 'w':
		access |= (1 << (bitoffset + 1));
		break;
	    case 'x':
		access |= (1 << (bitoffset + 2));
		break;
	    case 'c':
		access |= (1 << (bitoffset + 3));
		break;
	    case 'n':
		/* Special access code meaning "none" */
		access &= (~ (1 << (bitoffset + 0)));
		access &= (~ (1 << (bitoffset + 1)));
		access &= (~ (1 << (bitoffset + 2)));
		access &= (~ (1 << (bitoffset + 3)));
		break;
	    default:
		return -1;
	    }
	}
    }
    return access;
}
    
/* Initialize the GMU command cache.  Called by init_i960_architecture, 
   only if the architecture has a GMU.  MAINTAINERS: if a new architecture
   comes out that has a GMU, but different numbers of protect and detect
   registers, generalize this routine by calling mon960_arch() first to
   get the architecture type.

   NOTE: can be called more than once per GDB session (i.e. if mon960
   is disconnected and then connected again with the target command.)
*/
void
init_gmu_cmd_cache()
{
    register int i;
    
    gmu_cmd_cache.num_pregs = 2;
    gmu_cmd_cache.num_dregs = 6;

    for (i = 0; i < gmu_cmd_cache.num_pregs; ++i)
    {
	if (gmu_cmd_cache.preg_arg[i])
	    free(gmu_cmd_cache.preg_arg[i]);
	gmu_cmd_cache.preg_arg[i] = NULL;
    }
    for (i = 0; i < gmu_cmd_cache.num_dregs; ++i)
    {
	if (gmu_cmd_cache.dreg_arg[i])
	    free(gmu_cmd_cache.dreg_arg[i]);
	gmu_cmd_cache.dreg_arg[i] = NULL;
    }
}    

/* Re-program the GMU using the contents of gmu_cmd_cache.
   Called from target_clear_symtab_users.  If we are here, we know that
   the i960 architecture supports the GMU so we can just dive in. */
void
reprogram_gmu()
{
    register int i;

    for (i = 0; i < gmu_cmd_cache.num_pregs; ++i)
	if (gmu_cmd_cache.preg_arg[i])
	    gmu_command_1(GMU_PROTECT, 
			  GMU_DEFINE, 
			  gmu_cmd_cache.preg_arg[i], 
			  0,
			  0);
    for (i = 0; i < gmu_cmd_cache.num_dregs; ++i)
	if (gmu_cmd_cache.dreg_arg[i])
	    gmu_command_1(GMU_DETECT,
			  GMU_DEFINE,
			  gmu_cmd_cache.dreg_arg[i],
			  0,
			  0);
}

/* Helper function for info_gmu */
static void 
dump_gmu_protect_hdr()
{
    printf("%-4s%-8s%-4s%-3s%-3s%-3s%-3s%-3s%-3s%-3s%-3s %-16s%-16s\n",
	   "Num",
	   "Type",
	   "Enb",
	   "SC", "SX", "SW", "SR", "UC", "UX", "UW", "UR",
	   "Address",
	   "Mask");
}

/* Helper function for info_gmu */
static void 
dump_gmu_detect_hdr()
{
    printf("%-4s%-8s%-4s%-3s%-3s%-3s%-3s%-3s%-3s%-3s%-3s %-16s%-16s\n",
	   "Num",
	   "Type",
	   "Enb",
	   "SC", "SX", "SW", "SR", "UC", "UX", "UW", "UR",
	   "Start Address",
	   "End Address");
}

/* Helper function for info_gmu */
static void 
dump_gmu_reg(num, type, reg)
    int num, type;
    HDI_GMU_REG *reg;
{
    unsigned long access = reg->access;
    printf_filtered(" %-3d%-9s%-3s%2d%3d%3d%3d%3d%3d%3d%3d  0x%-14x0x%-14x\n",
		    num,
		    type == GMU_PROTECT ? "protect" : 
		    type == GMU_DETECT ? "detect" : "unknown",
		    reg->enabled ? "y" : "n",
		    access & 0x80 ? 1 : 0,
		    access & 0x40 ? 1 : 0,
		    access & 0x20 ? 1 : 0,
		    access & 0x10 ? 1 : 0,
		    access & 0x8 ? 1 : 0,
		    access & 0x4 ? 1 : 0,
		    access & 0x2 ? 1 : 0,
		    access & 0x1 ? 1 : 0,
		    type == GMU_PROTECT ? reg->loword : reg->hiword,
		    type == GMU_PROTECT ? reg->hiword : reg->loword);
}

/* GMU dump command available to the user with "info gmu".
   Reads the GMU regs directly from the processor. */
static void
info_gmu(args, from_tty)
    char *args;
    int from_tty;
{
    HDI_GMU_REGLIST reglist;
    int i;

    GMU_SANITY();   /* calls error if no GMU available */

    if (HDI_CALL(hdi_get_gmu_regs(&reglist)) == OK)
    {
	dump_gmu_protect_hdr();
	for (i = 0; i < reglist.num_pregs; ++i)
	{
	    dump_gmu_reg(i, GMU_PROTECT, &reglist.preg[i]);
	}
	dump_gmu_detect_hdr();
	for (i = 0; i < reglist.num_dregs; ++i)
	{
	    dump_gmu_reg(i, GMU_DETECT, &reglist.dreg[i]);
	}
    }
}

/* Helper function for maintenance_info_gmu */
static void 
dump_gmu_cmd_cache_hdr()
{
    printf("%-4s%-9s%s\n",
	   "Num",
	   "Type",
	   "Definition");
}

/* Helper function for maintenance_info_gmu */
static void 
dump_gmu_cmd_cache_reg(num, type, str)
    int num, type;
    char *str;
{
    printf_filtered("%-4d%-9s%s\n",
		    num,
		    type == GMU_PROTECT ? "protect" : 
		    type == GMU_DETECT ? "detect" : "unknown",
		    str ? str : "");
}

/* GMU dump command available through "maint info gmu".
   This dumps the contents of gdb's internal gmu command cache. */
static void
maintenance_info_gmu(args, from_tty)
    char *args;
    int from_tty;
{
    int i;
    if (gmu_cmd_cache.num_pregs > 0 || gmu_cmd_cache.num_dregs > 0 )
    {
	dump_gmu_cmd_cache_hdr();
	for (i = 0; i < gmu_cmd_cache.num_pregs; ++i)
	{
	    dump_gmu_cmd_cache_reg(i, GMU_PROTECT, gmu_cmd_cache.preg_arg[i]);
	}
	for (i = 0; i < gmu_cmd_cache.num_dregs; ++i)
	{
	    dump_gmu_cmd_cache_reg(i, GMU_DETECT, gmu_cmd_cache.dreg_arg[i]);
	}
    }
    else
	error("No GMU command cache for this architecture");
}

static char *
tokenize(buf, argcp, argvp)
    char *buf;
    int *argcp;
    char ***argvp;
{
    char *mybuf;
    char *tok;
    int i;

    if (! buf || strlen(buf) == 0)
    {
	*argcp = 0;
	return NULL;
    }

    mybuf = xmalloc(strlen(buf) + 1);
    strcpy(mybuf, buf);
    for (i = 0, tok = strtok(mybuf, " \t"); 
	 tok; 
	 ++i, tok = strtok(NULL, " \t"))
	;

    *argcp = i;
    *argvp = (char **) xmalloc(sizeof(char *) * i);
    strcpy(mybuf, buf);
    for (i = 0, tok = strtok(mybuf, " \t"); 
	 tok; 
	 ++i, tok = strtok(NULL, " \t"))
	(*argvp)[i] = tok;
    return mybuf;
}


static void 
aplink_command (args, from_tty)
    char * args;
    int from_tty;
{
	fputs_filtered ("Argument required.  Try `help aplink'\n", stdout);
}


/* Common routine to implement mcon, lmadr, and lmmr commands. */
static void
common_mem_control_set (routine, args, from_tty)
    int (*routine)();
    char *args;
    int  from_tty;
{
    long reg, value;
    char *cp, save;

    if (! (args && (cp = strrchr(args, ' '))))
        error ("This command requires two hexadecimal arguments");
    else
    {
        save = *cp;
        *cp  = '\0';
        if (
               (hdi_convert_number(args, 
                                   &reg, 
                                   HDI_CVT_UNSIGNED, 
                                   16,
                                   NULL) != OK)
                                   ||
               (hdi_convert_number(cp + 1, 
                                   &value, 
                                   HDI_CVT_UNSIGNED, 
                                   16,
                                   NULL) != OK)
                                   ||
               (routine(reg, value) != OK)
           )
        {
            error((char *) hdi_get_message());
        }
        *cp = save;  /* Restore so that GDB repeat command feature works. */
    }
}

static void 
mcon_command (args, from_tty)
    char * args; 
    int from_tty;
{
    common_mem_control_set(hdi_set_mcon, args, from_tty);
}

static void 
lmadr_command (args, from_tty)
    char * args;
    int from_tty;
{
    common_mem_control_set(hdi_set_lmadr, args, from_tty);
}

static void 
lmmr_command (args, from_tty)
    char * args;
    int from_tty;
{
    common_mem_control_set(hdi_set_lmmr, args, from_tty);
}

static void 
aplink_enable_command (args, from_tty)
    char * args; 
    int from_tty;
{
    unsigned long bit, value;
    char     *cp, save;

    if (! (args && (cp = strrchr(args, ' '))))
        error ("This command requires two decimal arguments");
    else
    {
        save = *cp;
        *cp  = '\0';
        if (
               (hdi_convert_number(args, 
                                   (long *) &bit, 
                                   HDI_CVT_UNSIGNED, 
                                   10,
                                   NULL) != OK)
                                   ||
               (hdi_convert_number(cp + 1, 
                                   (long *) &value, 
                                   HDI_CVT_UNSIGNED, 
                                   10,
                                   NULL) != OK)
                                   ||
               (hdi_aplink_enable(bit, value) != OK)
           )
        {
            error((char *) hdi_get_message());
        }
        *cp = save;  /* Restore so that GDB repeat command feature works. */
    }
}

static void 
aplink_switch_command (args, from_tty)
    char * args; 
    int from_tty;
{
    unsigned long region, mode;
    char          *cp, save;

    if (! (args && (cp = strrchr(args, ' '))))
        error ("This command requires two arguments");
    else
    {
        save = *cp;
        *cp  = '\0';
        if (
               (hdi_convert_number(args, 
                                   (long *) &region, 
                                   HDI_CVT_UNSIGNED, 
                                   16,
                                   NULL) != OK)
                                   ||
               (hdi_convert_number(cp + 1, 
                                   (long *) &mode, 
                                   HDI_CVT_UNSIGNED, 
                                   10,
                                   NULL) != OK)
                                   ||
               (hdi_aplink_switch(region, mode) != OK)
           )
        {
            error((char *) hdi_get_message());
        }
        *cp = save;  /* Restore so that GDB repeat command feature works. */
    }
}

static void 
aplink_wait_reset_action(args, action)
char *args;
int action;
{
    int valid = TRUE;

    if (args)
    {
        while (isspace(*args))
            args++;
        if (*args)
        {
            valid = FALSE;
            error ("This command takes no arguments");
        }
    }
    if (valid && hdi_aplink_sync(action) != OK)
            error((char *) hdi_get_message());
}

static void 
aplink_reset_command (args, from_tty)
    char * args; 
    int from_tty;
{
    aplink_wait_reset_action(args, HDI_APLINK_RESET);
}

static void 
aplink_wait_command (args, from_tty)
    char * args; 
    int from_tty;
{
    aplink_wait_reset_action(args, HDI_APLINK_WAIT);
}

kill_inferior_fast()
{
	/* Stub that allows us to omit infptrace.c */
}

/* An individual frame on the Intel 80960 is defined by the fp.
   We therefore we need only one argument to resolve any frame.
*/

struct frame_info *
setup_arbitrary_frame (argc, argv)
     int argc;
     CORE_ADDR *argv;
{
    struct frame_info *frame;

    if (argc != 1)
	    error ("Intel 80960 frame specifications require one argument: fp");

    frame = create_new_frame (argv[0], 0);

    if (!frame)
	    fatal ("internal: create_new_frame returned invalid frame");
  
    if (get_next_frame(frame))
	    frame->pc = FRAME_SAVED_PC (get_next_frame(frame));
    else
	    frame->pc = read_register(IP_REGNUM);
    frame->virtual_pc = frame->pc;
    return frame;
}

/* Unembellished backtrace (To support maintenance command "backtrace")
   Read and print exactly what you find in GDB's frame chain. */

static void
maintenance_backtrace(args, from_tty)
     char *args;
     int from_tty;
{
    FRAME frame = get_current_frame();
    struct symbol *sym;
    int n = 123456;

    if (!target_has_stack)
	error ("No stack.");
    
    printf_filtered("Virtual Backtrace:\n%-12s%-12s%-12s%-12s%s\n", 
		    "fp", "pc", "virtual pc", "inlined", "Best guess fcn name");

    for ( args && (n = atoi(args)); frame && frame->frame && n > 0; --n )
    {
	sym = find_pc_function(frame->virtual_pc);
	printf_filtered("0x%-10x0x%-10x0x%-10x%-12c%s\n", 
			frame->frame, 
			frame->pc, 
			frame->virtual_pc,
			frame->inlin ? 'y' : 'n',
			sym ? SYMBOL_NAME(sym) : "<null>");
	frame = get_prev_frame(frame);
    }
}



/* 960-specific raw backtrace (To support maintenance command "rbacktrace")
   Only the first fp comes from GDB's FRAME structure.  All previous frames
   come directly from what's stored in the stack. */
static void
maintenance_raw_backtrace(args, from_tty)
    char *args;
    int from_tty;
{
    FRAME frame = get_current_frame();
    FRAME_ADDR fp = frame->frame;
    CORE_ADDR  pc = frame->pc;
    struct symbol *sym;
    int n = 123456;

    if (!target_has_stack)
	error ("No stack.");

    printf_filtered("Raw i960 Backtrace:\n%-15s%-15s%s\n", 
		    "fp", "pc", "Best guess fcn name");

    for ( args && (n = atoi(args)); n > 0; --n )
    {
	sym = find_pc_function(pc);
	printf_filtered("0x%-13x0x%-13x%s\n", fp, pc, 
			sym ? SYMBOL_NAME(sym) : "<null>");

	/* This is essentially prev_frame->frame = FRAME_CHAIN(frame) */
	fp = read_memory_integer(fp, 4) & ~0xf;
	if ( fp == 0 )
	    break;

	/* This is essentially prev_frame->pc = FRAME_SAVED_PC(frame) */
	pc = read_memory_integer(fp + 8, 4) & ~3;
    }
}

#endif /* IMSTG */

static void
generic_reset(args,from_tty)
    char *args;
    int from_tty;
{
    if ( current_target != &mon960_ops )
	error("Not connected to an i960 target.  Try `help target'.");
}

#ifdef signal
#undef signal
/* A debugging function just to catch signal calls on their way in.
   to use, set USER_CFLAGS=-Dsignal=sig_call on the make command line. 
   Then set a breakpoint in sig_call and observe who calls signal when. */
void
(*sig_call (sig, disp)) ()
    int sig;
    void (*disp)();
{
    void (*ret)() = (void (*) ()) signal (sig, disp);
    return ret;
}
#endif

#if defined(IMSTG)
/* In Cygnus's gdb distribution, these are in i960-pinsn.c */
/****************************************/
/* MEM format				*/
/****************************************/

struct tabent {
	char	*name;
	char	numops;
};

static int				/* returns instruction length: 4 or 8 */
mem( memaddr, word1, word2, noprint )
    unsigned long memaddr;
    unsigned long word1, word2;
    int noprint;		/* If TRUE, return instruction length, but
				   don't output any text.  */
{
	int i, j;
	int len;
	int mode;
	int offset;
	const char *reg1, *reg2, *reg3;

	/* This lookup table is too sparse to make it worth typing in, but not
	 * so large as to make a sparse array necessary.  We allocate the
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
#define MEM_MAX	0xcf
#define MEM_SIZ	((MEM_MAX-MEM_MIN+1) * sizeof(struct tabent))

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
		0xa0,	"ldt",	 2,
		0xa2,	"stt",	-2,
		0xb0,	"ldq",	 2,
		0xb2,	"stq",	-2,
		0xc0,	"ldib",	 2,
		0xc2,	"stib",	-2,
		0xc8,	"ldis",	 2,
		0xca,	"stis",	-2,
		0,	NULL,	0
	};

	if ( mem_tab == NULL ){
		mem_tab = (struct tabent *) xmalloc( MEM_SIZ );
		memset( mem_tab, '\0', MEM_SIZ );
		for ( i = 0; mem_init[i].opcode != 0; i++ ){
			j = mem_init[i].opcode - MEM_MIN;
			mem_tab[j].name = mem_init[i].name;
			mem_tab[j].numops = mem_init[i].numops;
		}
	}

	i = ((word1 >> 24) & 0xff) - MEM_MIN;
	mode = (word1 >> 10) & 0xf;

	if ( (mem_tab[i].name != NULL)		/* Valid instruction */
	&&   ((mode == 5) || (mode >=12)) ){	/* With 32-bit displacement */
		len = 8;
	} else {
		len = 4;
	}

	if ( noprint ){
		return len;
	}
	abort ();
}

/* Read the i960 instruction at 'memaddr' and return the address of 
   the next instruction after that, or 0 if 'memaddr' is not the
   address of a valid instruction.  The first word of the instruction
   is stored at 'pword1', and the second word, if any, is stored at
   'pword2'.  */

CORE_ADDR
next_insn (memaddr, pword1, pword2)
     CORE_ADDR memaddr;
     unsigned long *pword1, *pword2;
{
  int len;
  char buf[8];

  /* Read the two (potential) words of the instruction at once,
     to eliminate the overhead of two calls to read_memory ().
     FIXME: Loses if the first one is readable but the second is not
     (e.g. last word of the segment).  */

  read_memory (memaddr, buf, 8);
  *pword1 = extract_unsigned_integer (buf, 4);
  *pword2 = extract_unsigned_integer (buf + 4, 4);

  /* Divide instruction set into classes based on high 4 bits of opcode*/

  switch ((*pword1 >> 28) & 0xf)
    {
    case 0x0:
    case 0x1:	/* ctrl */

    case 0x2:
    case 0x3:	/* cobr */

    case 0x5:
    case 0x6:
    case 0x7:	/* reg */
      len = 4;
      break;

    case 0x8:
    case 0x9:
    case 0xa:
    case 0xb:
    case 0xc:
      len = mem (memaddr, *pword1, *pword2, 1);
      break;

    default:	/* invalid instruction */
      len = 0;
      break;
    }

  if (len)
    return memaddr + len;
  else
    return 0;
}
#endif /* IMSTG */

/*
  Initialize the fault name table for pretties when printing out
  fault types.  This is a sparse array, since not all subtypes are 
  used for each type, so it's better to initialize them one by one here.
  To save space, there are only 8 subtypes per type.  Subtypes that are 
  numbers (e.g. type 2) are simply indexes into the table.  Subtypes that 
  are bitfields (e.g. type 1) are shifted and masked to find the correct
  subtype index.

  All the unused strings are NULL by default.

  Note the table represents the union of all 80960 architecture flavors.
  Not all faults can occur on all flavors, but thankfully the designers
  did not overlap the type and subtype numbers.
*/
static void
init_fault_name_table()
{
    fault_name_table[1][1] = "Instruction Trace";
    fault_name_table[1][2] = "Branch Trace";
    fault_name_table[1][3] = "Call Trace";
    fault_name_table[1][4] = "Return Trace";
    fault_name_table[1][5] = "Prereturn Trace";
    fault_name_table[1][6] = "Supervisor Trace";
    fault_name_table[1][7] = "Breakpoint Trace";
    
    fault_name_table[2][1] = "Invalid Opcode";
    fault_name_table[2][2] = "Operation - Unimplemented";
    fault_name_table[2][3] = "Operation - Unaligned";
    fault_name_table[2][4] = "Invalid Operand";

    fault_name_table[3][1] = "Integer Overflow";
    fault_name_table[3][2] = "Arithmetic Zero-Divide";

    fault_name_table[4][0] = "Floating-Point Overflow";
    fault_name_table[4][1] = "Floating-Point Underflow";
    fault_name_table[4][2] = "Floating-Point Invalid-Operation";
    fault_name_table[4][3] = "Floating-Point Zero-Divide";
    fault_name_table[4][4] = "Floating-Point Inexact";
    fault_name_table[4][5] = "Floating-Point Reserved-Encoding";

    fault_name_table[5][1] = "Constraint Range";
    fault_name_table[5][2] = "Privileged";

    fault_name_table[7][1] = "Protection - Length";
    fault_name_table[7][5] = "Protection - Bad Access";

    fault_name_table[8][1] = "Machine - Bad Access";
    fault_name_table[8][2] = "Machine - Parity Error";

    fault_name_table[10][1] = "Type Mismatch";
}

/* Initialization stub */

void
_initialize_i960_tdep ()
{
    check_host ();
    init_fault_name_table();

    /* Add our own command class for help purposes.  Commands created in this
       file should go into this command class.  The commands themselves should
       be put onto the global "cmdlist" command list. (use add_com)

       Subcommands of gmu, aplink and profile should be put onto the "aplink",
       "gmu", and "profile" command lists, respectively. (use add_cmd) */
    add_com ("i960", class_i960, NO_FUNCTION, "80960-specific features.");

    /* Start the command parade.  Commands will appear in "help i960" list
       in reverse order from the order given here. */
    /* Generic form of reset that will appear if you do "help reset" 
       before you connect to a target.  Target-specific reset help 
       will take effect upon connecting (code is in mon960_open) */
    add_com ("reset", class_i960, generic_reset,
	     "Resets the remote target system.");

    add_com ("regs", class_i960, regs_command, 
	     "Like `info registers' but prints in a more compact format.");

    add_prefix_cmd ("profile", class_i960, profile_command,
		    "Transfer profiling data to and from target memory.\n\
The first argument is the operation you want to do: put, get, or clear.\n\
The second (optional) argument is the filename of the profile file\n\
(defaults to `default.pf').",
                   &profilelist, "profile ", 0, &cmdlist);

    add_cmd ("put", no_class, profile_put_command,
	     "Store profile data into a file.\n\
Reads data from target memory, then stores it into a file.\n\
Provide a file name argument, or no arguments defaults to `profile.pf'.",
 	     &profilelist);

    add_cmd ("get", no_class, profile_get_command,
	     "Read profile data from a file.\n\
Reads data from the profile file, then writes it into target memory.\n\
Provide a file name argument, or no arguments defaults to `profile.pf'.",
	     &profilelist);

    add_cmd ("clear", no_class, profile_clear_command,
	     "Reset profile data in target memory to zero.",
	     &profilelist);

    add_com ("lmmr", class_i960, lmmr_command,
	     "Change the processor's logical memory mask registers.\n\
Syntax:  lmmr REGNUM MASK_VALUE\n\
Valid for Jx and Hx processors.  REGNUM and MASK_VALUE are hex constants.\n");

    add_com ("lmadr", class_i960, lmadr_command,
	     "Change the processor's logical memory address registers.\n\
Syntax:  lmadr REGNUM START_ADDR\n\
Valid for Jx and Hx processors.  REGNUM and START_ADDR are hex constants.\n");

    add_com ("mcon", class_i960, mcon_command,
	     "Change the processor's memory control register.\n\
Syntax:  mcon REGION VALUE\n\
Valid for Cx, Jx, and Hx processors.  REGION and VALUE are hex constants.\n");

    add_prefix_cmd ("aplink", class_i960, aplink_command,
		    "Special commands to support the Aplink target extension.",
		    &aplinklist, "aplink ", 0, &cmdlist);

    add_cmd ("enable", no_class, aplink_enable_command,
	     "Change bits in the ApLink mode register.\n\
Syntax:  aplink enable BIT VALUE\n",
	     &aplinklist);

    add_cmd ("reset", no_class, aplink_reset_command,
	     "Execute ApLink reset command.\n\
Syntax:  aplink reset\n",
	     &aplinklist);

    add_cmd ("switch", no_class, aplink_switch_command,
	     "Switch ApLink modes.\n\
Syntax:  aplink switch REGION MODE\n\
REGION is a hex constant, MODE is a decimal constant.\n",
	     &aplinklist);

    add_cmd ("wait", no_class, aplink_wait_command,
	     "Execute ApLink wait command.\n\
Syntax:  aplink wait\n",
	     &aplinklist);

    add_prefix_cmd ("gmu", class_i960, gmu_command,
	            "Program the processor's Guarded Memory Unit.",
                    &gmulist, "gmu ", 0, &cmdlist);

    add_prefix_cmd ("protect", no_class, gmu_protect_command,
	            "Program the processor's GMU Protection registers.",
                    &gmuprotlist, "gmu protect ", 0, &gmulist);

    add_prefix_cmd ("detect", no_class, gmu_detect_command,
	            "Program the processor's GMU Detection registers.",
                    &gmudetlist, "gmu detect ", 0, &gmulist);

    add_cmd ("define", no_class, gmu_protect_define_command,
             "Define a new GMU Protection register.\n\
Syntax: gmu protect define REGNUM ACCESS ADDRESS MASK\n\
REGNUM is the register number you are defining\n\
ACCESS is a string of the form ModeType[,ModeType] where Mode is 'u' or 's',\n\
    and Type is one or more of 'r', 'w', 'x', 'c'\n\
ADDRESS is an expression, evaluating to the start of the protection range\n\
MASK is an expression, evaluating to a mask that defines the block size.",
             &gmuprotlist);

    add_cmd ("enable", no_class, gmu_protect_enable_command,
             "Enable one or all GMU Protection registers.\n\
Syntax: gmu protect enable [REGNUM]\n\
REGNUM is register number.  If REGNUM is omitted, enable all\n\
protection registers.", &gmuprotlist);

    add_cmd ("disable", no_class, gmu_protect_disable_command,
             "Disable one or all GMU Protection registers.\n\
Syntax: gmu protect disable [REGNUM]\n\
REGNUM is register number.  If REGNUM is omitted, disable all\n\
protection registers.", &gmuprotlist);

    add_cmd ("define", no_class, gmu_detect_define_command,
             "Define a new GMU Detection register.\n\
Syntax: gmu detect define REGNUM ACCESS STARTADDR ENDADDR\n\
REGNUM is the register number you are defining\n\
ACCESS is a string of the form ModeType[,ModeType] where Mode is 'u' or 's',\n\
    and Type is one or more of 'r', 'w', 'x', 'c'\n\
STARTADDR is an expression, evaluating to the start of the detection range\n\
ENDADDR is an expression, evaluating to one byte beyond the end of the range",
             &gmudetlist);

    add_cmd ("enable", no_class, gmu_detect_enable_command,
             "Enable one or all GMU Detection registers.\n\
Syntax: gmu detect enable [REGNUM]\n\
REGNUM is register number.  If REGNUM is omitted, enable all\n\
detection registers.", &gmudetlist);

    add_cmd ("disable", no_class, gmu_detect_disable_command,
             "Disable one or all GMU Detection registers.\n\
Syntax: gmu detect disable [REGNUM]\n\
REGNUM is register number.  If REGNUM is omitted, disable all\n\
detection registers.", &gmudetlist);

    add_info ("gmu", info_gmu, "Current state of the i960 GMU");

    add_cmd ("gmu", class_maintenance, maintenance_info_gmu,
	     "Like info gmu but print GDB's internal command cache",
	     &maintenanceinfolist);

    add_cmd ("backtrace", class_maintenance, maintenance_backtrace,
	     "Prints an unembellished virtual backtrace.",
	     &maintenancelist);

    add_cmd ("rbacktrace", class_maintenance, maintenance_raw_backtrace,
	     "Prints a raw i960-specific backtrace.",
	     &maintenancelist);

    /* Add a couple of set/show commands.  These come out before 
       the i960 commands, in reverse order from that given here. */
    add_show_from_set
	(add_set_cmd ("caching", class_i960, var_boolean, 
		      (char *)&caching_enabled,
		      "Set target memory caching.\n\
When on, target memory accesses are kept to a minimum.", &setlist),
	 &showlist);
    
    add_show_from_set
	(add_set_cmd ("autoreset", class_i960, var_boolean, (char *)&autoreset,
		      "Set resetting of the target automatically when quitting.",
		      &setlist),
	 &showlist);
}


