/* Get info from stack frames;
   convert between frames, blocks, functions and pc values.
   Copyright 1986, 1987, 1988, 1989, 1991 Free Software Foundation, Inc.

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

#include "defs.h"
#include "symtab.h"
#include "bfd.h"
#include "symfile.h"
#include "objfiles.h"
#include "frame.h"
#include "gdbcore.h"
#include "value.h"		/* for read_register */
#include "target.h"		/* for target_has_stack */
#include "inferior.h"		/* for read_pc */
#include "annotate.h"

/* Is ADDR inside the startup file?  Note that if your machine
   has a way to detect the bottom of the stack, there is no need
   to call this function from FRAME_CHAIN_VALID; the reason for
   doing so is that some machines have no way of detecting bottom
   of stack. 

   A PC of zero is always considered to be the bottom of the stack. */

int
inside_entry_file (addr)
     CORE_ADDR addr;
{
  if (addr == 0)
    return 1;
  if (symfile_objfile == 0)
    return 0;
#if CALL_DUMMY_LOCATION == AT_ENTRY_POINT
  /* Do not stop backtracing if the pc is in the call dummy
     at the entry point.  */
  if (PC_IN_CALL_DUMMY (addr, 0, 0))
    return 0;
#endif
  return (addr >= symfile_objfile -> ei.entry_file_lowpc &&
	  addr <  symfile_objfile -> ei.entry_file_highpc);
}

/* Test a specified PC value to see if it is in the range of addresses
   that correspond to the main() function.  See comments above for why
   we might want to do this.

   Typically called from FRAME_CHAIN_VALID.

   A PC of zero is always considered to be the bottom of the stack. */

int
inside_main_func (pc)
CORE_ADDR pc;
{
  if (pc == 0)
    return 1;
  if (symfile_objfile == 0)
    return 0;
  return (symfile_objfile -> ei.main_func_lowpc  <= pc &&
	  symfile_objfile -> ei.main_func_highpc > pc);
}

/* Test a specified PC value to see if it is in the range of addresses
   that correspond to the process entry point function.  See comments
   in objfiles.h for why we might want to do this.

   Typically called from FRAME_CHAIN_VALID.

   A PC of zero is always considered to be the bottom of the stack. */

int
inside_entry_func (pc)
CORE_ADDR pc;
{
  if (pc == 0)
    return 1;
  if (symfile_objfile == 0)
    return 0;
#if CALL_DUMMY_LOCATION == AT_ENTRY_POINT
  /* Do not stop backtracing if the pc is in the call dummy
     at the entry point.  */
  if (PC_IN_CALL_DUMMY (pc, 0, 0))
    return 0;
#endif
  return (symfile_objfile -> ei.entry_func_lowpc  <= pc &&
	  symfile_objfile -> ei.entry_func_highpc > pc);
}

/* Address of innermost stack frame (contents of FP register) */

static FRAME current_frame;

/*
 * Cache for frame addresses already read by gdb.  Valid only while
 * inferior is stopped.  Control variables for the frame cache should
 * be local to this module.
 */
struct obstack frame_cache_obstack;

/* Return the innermost (currently executing) stack frame.  */

FRAME
get_current_frame ()
{
  /* We assume its address is kept in a general register;
     param.h says which register.  */

  return current_frame;
}

void
set_current_frame (frame)
     FRAME frame;
{
  current_frame = frame;
}

FRAME
create_new_frame (addr, pc)
     FRAME_ADDR addr;
     CORE_ADDR pc;
{
  struct frame_info *fci;	/* Same type as FRAME */
  char *name;

  fci = (struct frame_info *)
    obstack_alloc (&frame_cache_obstack,
		   sizeof (struct frame_info));

  /* Arbitrary frame */
  fci->next = (struct frame_info *) 0;
  fci->prev = (struct frame_info *) 0;
  fci->frame = addr;
  fci->pc = pc;
#if defined(IMSTG)  /* i960 INLINED_BREAKPOINTS */
  fci->virtual_pc = pc;
  fci->inlin = pc_is_inline(pc);
#endif
  find_pc_partial_function (pc, &name, (CORE_ADDR *)NULL,(CORE_ADDR *)NULL);
  fci->signal_handler_caller = IN_SIGTRAMP (fci->pc, name);

#ifdef INIT_EXTRA_FRAME_INFO
  INIT_EXTRA_FRAME_INFO (0, fci);
#endif

  return fci;
}

/* Return the frame that called FRAME.
   If FRAME is the original frame (it has no caller), return 0.  */

FRAME
get_prev_frame (frame)
     FRAME frame;
{
  /* We're allowed to know that FRAME and "struct frame_info *" are
     the same */
  return get_prev_frame_info (frame);
}

/* Return the frame that FRAME calls (0 if FRAME is the innermost
   frame).  */

FRAME
get_next_frame (frame)
     FRAME frame;
{
  /* We're allowed to know that FRAME and "struct frame_info *" are
     the same */
  return frame->next;
}

/*
 * Flush the entire frame cache.
 */
void
flush_cached_frames ()
{
  /* Since we can't really be sure what the first object allocated was */
  obstack_free (&frame_cache_obstack, 0);
  obstack_init (&frame_cache_obstack);

  current_frame = (struct frame_info *) 0; /* Invalidate cache */
  annotate_frames_invalid ();
}

/* Flush the frame cache, and start a new one if necessary.  */
void
reinit_frame_cache ()
{
  flush_cached_frames ();
  if (target_has_stack)
    {
      set_current_frame (create_new_frame (read_fp (), read_pc ()));
      select_frame (get_current_frame (), 0);
    }
  else
    {
      set_current_frame (0);
      select_frame ((FRAME) 0, -1);
    }
}

/* Return a structure containing various interesting information
   about a specified stack frame.  */
/* How do I justify including this function?  Well, the FRAME
   identifier format has gone through several changes recently, and
   it's not completely inconceivable that it could happen again.  If
   it does, have this routine around will help */

struct frame_info *
get_frame_info (frame)
     FRAME frame;
{
  return frame;
}

/* If a machine allows frameless functions, it should define a macro
   FRAMELESS_FUNCTION_INVOCATION(FI, FRAMELESS) in param.h.  FI is the struct
   frame_info for the frame, and FRAMELESS should be set to nonzero
   if it represents a frameless function invocation.  */

/* Return nonzero if the function for this frame lacks a prologue.  Many
   machines can define FRAMELESS_FUNCTION_INVOCATION to just call this
   function.  */

int
frameless_look_for_prologue (frame)
     FRAME frame;
{
  CORE_ADDR func_start, after_prologue;
  func_start = (get_pc_function_start (frame->pc) +
		FUNCTION_START_OFFSET);
  if (func_start)
    {
      after_prologue = func_start;
#ifdef SKIP_PROLOGUE_FRAMELESS_P
      /* This is faster, since only care whether there *is* a prologue,
	 not how long it is.  */
      SKIP_PROLOGUE_FRAMELESS_P (after_prologue);
#else
      SKIP_PROLOGUE (after_prologue);
#endif
      return after_prologue == func_start;
    }
  else
    /* If we can't find the start of the function, we don't really
       know whether the function is frameless, but we should be able
       to get a reasonable (i.e. best we can do under the
       circumstances) backtrace by saying that it isn't.  */
    return 0;
}

/* Default a few macros that people seldom redefine.  */

#if !defined (INIT_FRAME_PC)
#define INIT_FRAME_PC(fromleaf, prev) \
  prev->pc = (fromleaf ? SAVED_PC_AFTER_CALL (prev->next) : \
	      prev->next ? FRAME_SAVED_PC (prev->next) : read_pc ());
#endif

#ifndef FRAME_CHAIN_COMBINE
#define	FRAME_CHAIN_COMBINE(chain, thisframe) (chain)
#endif

/* Return a structure containing various interesting information
   about the frame that called NEXT_FRAME.  Returns NULL
   if there is no such frame.  */

struct frame_info *
get_prev_frame_info (next_frame)
     FRAME next_frame;
{
  FRAME_ADDR address = 0;
  struct frame_info *prev;
  int fromleaf = 0;
  char *name;

  /* If the requested entry is in the cache, return it.
     Otherwise, figure out what the address should be for the entry
     we're about to add to the cache. */

  if (!next_frame)
    {
#if 0
      /* This screws value_of_variable, which just wants a nice clean
	 NULL return from block_innermost_frame if there are no frames.
	 I don't think I've ever seen this message happen otherwise.
	 And returning NULL here is a perfectly legitimate thing to do.  */
      if (!current_frame)
	{
	  error ("You haven't set up a process's stack to examine.");
	}
#endif

      return current_frame;
    }

  /* If we have the prev one, return it */
  if (next_frame->prev)
    return next_frame->prev;

  /* On some machines it is possible to call a function without
     setting up a stack frame for it.  On these machines, we
     define this macro to take two args; a frameinfo pointer
     identifying a frame and a variable to set or clear if it is
     or isn't leafless.  */
#ifdef FRAMELESS_FUNCTION_INVOCATION
  /* Still don't want to worry about this except on the innermost
     frame.  This macro will set FROMLEAF if NEXT_FRAME is a
     frameless function invocation.  */
  if (!(next_frame->next))
    {
      FRAMELESS_FUNCTION_INVOCATION (next_frame, fromleaf);
      if (fromleaf)
	address = next_frame->frame;
    }
#endif

  if (!fromleaf)
    {
      /* Two macros defined in tm.h specify the machine-dependent
	 actions to be performed here.
	 First, get the frame's chain-pointer.
	 If that is zero, the frame is the outermost frame or a leaf
	 called by the outermost frame.  This means that if start
	 calls main without a frame, we'll return 0 (which is fine
	 anyway).

	 Nope; there's a problem.  This also returns when the current
	 routine is a leaf of main.  This is unacceptable.  We move
	 this to after the ffi test; I'd rather have backtraces from
	 start go curfluy than have an abort called from main not show
	 main.  */
      address = FRAME_CHAIN (next_frame);
      if (!FRAME_CHAIN_VALID (address, next_frame))
	return 0;
      address = FRAME_CHAIN_COMBINE (address, next_frame);
    }
  if (address == 0)
    return 0;

  prev = (struct frame_info *)
    obstack_alloc (&frame_cache_obstack,
		   sizeof (struct frame_info));

  if (next_frame)
    next_frame->prev = prev;
  prev->next = next_frame;
  prev->prev = (struct frame_info *) 0;
  prev->frame = address;
  prev->signal_handler_caller = 0;
  prev->inlin = 0;

/* This change should not be needed, FIXME!  We should
   determine whether any targets *need* INIT_FRAME_PC to happen
   after INIT_EXTRA_FRAME_INFO and come up with a simple way to
   express what goes on here.

      INIT_EXTRA_FRAME_INFO is called from two places: create_new_frame
      		(where the PC is already set up) and here (where it isn't).
      INIT_FRAME_PC is only called from here, always after
      		INIT_EXTRA_FRAME_INFO.
   
   The catch is the MIPS, where INIT_EXTRA_FRAME_INFO requires the PC
   value (which hasn't been set yet).  Some other machines appear to
   require INIT_EXTRA_FRAME_INFO before they can do INIT_FRAME_PC.  Phoo.

   We shouldn't need INIT_FRAME_PC_FIRST to add more complication to
   an already overcomplicated part of GDB.   gnu@cygnus.com, 15Sep92.

   To answer the question, yes the sparc needs INIT_FRAME_PC after
   INIT_EXTRA_FRAME_INFO.  Suggested scheme:

   SETUP_INNERMOST_FRAME()
     Default version is just create_new_frame (read_fp ()),
     read_pc ()).  Machines with extra frame info would do that (or the
     local equivalent) and then set the extra fields.
   SETUP_ARBITRARY_FRAME(argc, argv)
     Only change here is that create_new_frame would no longer init extra
     frame info; SETUP_ARBITRARY_FRAME would have to do that.
   INIT_PREV_FRAME(fromleaf, prev)
     Replace INIT_EXTRA_FRAME_INFO and INIT_FRAME_PC.  This should
     also return a flag saying whether to keep the new frame, or
     whether to discard it, because on some machines (e.g.  mips) it
     is really awkward to have FRAME_CHAIN_VALID called *before*
     INIT_EXTRA_FRAME_INFO (there is no good way to get information
     deduced in FRAME_CHAIN_VALID into the extra fields of the new frame).
   std_frame_pc(fromleaf, prev)
     This is the default setting for INIT_PREV_FRAME.  It just does what
     the default INIT_FRAME_PC does.  Some machines will call it from
     INIT_PREV_FRAME (either at the beginning, the end, or in the middle).
     Some machines won't use it.
   kingdon@cygnus.com, 13Apr93, 31Jan94.  */

#ifdef INIT_FRAME_PC_FIRST
  INIT_FRAME_PC_FIRST (fromleaf, prev);
#endif

#ifdef INIT_EXTRA_FRAME_INFO
  INIT_EXTRA_FRAME_INFO(fromleaf, prev);
#endif

  /* This entry is in the frame queue now, which is good since
     FRAME_SAVED_PC may use that queue to figure out its value
     (see tm-sparc.h).  We want the pc saved in the inferior frame. */
  INIT_FRAME_PC(fromleaf, prev);
  
#if defined(IMSTG)  /* i960 INLINED_BREAKPOINTS */
  /* Notes on inlined functions: we're going to try to create the illusion
     that the function was actually called.  (with the flag <inlined> 
     printed.)

     To do this, we'll create a virtual frame to represent the inline.

     If current frame is really a virtual frame, then fudge caller's PC
     to point to just *beyond* the scope of the inline -- much like what
     happens to the caller's PC for a normal call.

     If caller's frame is really a virtual frame, then fudge the caller's
     frame address to be the same as the current frame address, since 
     there really isn't a frame for the caller. */
  prev->virtual_pc = prev->pc;
  if (next_frame->inlin)
  {
      /* Current frame is an inline.  Set up virtual PC for the caller.
	 This will point to the next instruction beyond the range of the 
	 inline, much like a real caller's PC is one instruction past the
	 call instruction. */
      struct block *b = 
	  block_for_pc(next_frame->virtual_pc);
      if (b && BLOCK_END(b) != prev->pc)
      {
	  prev->frame = next_frame->frame;
	  prev->pc = next_frame->pc;
	  prev->virtual_pc = BLOCK_END(b);
	  if (pc_is_inline(prev->virtual_pc))
	  {
	      /* Caller's frame is also an inline */
	      prev->inlin = 1;
	  }
      }
  }
  else if (pc_is_inline(prev->pc - 1))
  {
      /* Caller's frame is an inline */
      prev->inlin = 1;
  }
#endif

  /* If ->frame and ->pc are unchanged, we are in the process of getting
     ourselves into an infinite backtrace.  Some architectures check this
     in FRAME_CHAIN or thereabouts, but it seems like there is no reason
     this can't be an architecture-independent check.  */
  if (next_frame != NULL)
    {
	if (prev->frame == next_frame->frame
	    && prev->pc == next_frame->pc
#if defined(IMSTG)  /* i960 INLINED_BREAKPOINTS */
	    /* It's OK for caller and callee to have same frame and PC values 
	       PROVIDED the caller's virtual PC is different from its PC. */
	    && prev->pc == prev->virtual_pc
#endif
	    )
	{
	  next_frame->prev = NULL;
	  obstack_free (&frame_cache_obstack, prev);
	  return NULL;
	}
    }

  find_pc_partial_function (prev->pc, &name,
			    (CORE_ADDR *)NULL,(CORE_ADDR *)NULL);
  if (IN_SIGTRAMP (prev->pc, name))
    prev->signal_handler_caller = 1;

  return prev;
}

CORE_ADDR
get_frame_pc (frame)
     FRAME frame;
{
  struct frame_info *fi;
  fi = get_frame_info (frame);
  return fi->pc;
}

#if defined (FRAME_FIND_SAVED_REGS)
/* Find the addresses in which registers are saved in FRAME.  */

void
get_frame_saved_regs (frame_info_addr, saved_regs_addr)
     struct frame_info *frame_info_addr;
     struct frame_saved_regs *saved_regs_addr;
{
  FRAME_FIND_SAVED_REGS (frame_info_addr, *saved_regs_addr);
}
#endif

/* Return the innermost lexical block in execution
   in a specified stack frame.  The frame address is assumed valid.  */

struct block *
get_frame_block (frame)
     FRAME frame;
{
  struct frame_info *fi;
  CORE_ADDR pc;

  fi = get_frame_info (frame);

  pc = fi->pc;
  if (fi->next != 0 && fi->next->signal_handler_caller == 0)
    /* We are not in the innermost frame and we were not interrupted
       by a signal.  We need to subtract one to get the correct block,
       in case the call instruction was the last instruction of the block.
       If there are any machines on which the saved pc does not point to
       after the call insn, we probably want to make fi->pc point after
       the call insn anyway.  */
    --pc;
  return block_for_pc (pc);
}

struct block *
get_current_block ()
{
  return block_for_pc (read_pc ());
}

CORE_ADDR
get_pc_function_start (pc)
     CORE_ADDR pc;
{
  register struct block *bl;
  register struct symbol *symbol;
  register struct minimal_symbol *msymbol;
  CORE_ADDR fstart;

  if ((bl = block_for_pc (pc)) != NULL &&
      (symbol = block_function (bl)) != NULL)
    {
      bl = SYMBOL_BLOCK_VALUE (symbol);
      fstart = BLOCK_START (bl);
    }
  else if ((msymbol = lookup_minimal_symbol_by_pc (pc)) != NULL)
    {
      fstart = SYMBOL_VALUE_ADDRESS (msymbol);
    }
  else
    {
      fstart = 0;
    }
  return (fstart);
}

/* Return the symbol for the function executing in frame FRAME.  */

struct symbol *
get_frame_function (frame)
     FRAME frame;
{
  register struct block *bl = get_frame_block (frame);
  if (bl == 0)
    return 0;
  return block_function (bl);
}

/* Return the blockvector immediately containing the innermost lexical block
   containing the specified pc value, or 0 if there is none.
   PINDEX is a pointer to the index value of the block.  If PINDEX
   is NULL, we don't pass this information back to the caller.  */

struct blockvector *
blockvector_for_pc (pc, pindex)
     register CORE_ADDR pc;
     int *pindex;
{
  register struct block *b;
  register int bot, top, half;
  register struct symtab *s;
  struct blockvector *bl;

  /* First search all symtabs for one whose file contains our pc */
  s = find_pc_symtab (pc);
  if (s == 0)
    return 0;

  bl = BLOCKVECTOR (s);
  b = BLOCKVECTOR_BLOCK (bl, 0);

  /* Then search that symtab for the smallest block that wins.  */
  /* Use binary search to find the last block that starts before PC.  */

  bot = 0;
  top = BLOCKVECTOR_NBLOCKS (bl);

  while (top - bot > 1)
    {
      half = (top - bot + 1) >> 1;
      b = BLOCKVECTOR_BLOCK (bl, bot + half);
      if (BLOCK_START (b) <= pc)
	bot += half;
      else
	top = bot + half;
    }

  /* Now search backward for a block that ends after PC.  */

  while (bot >= 0)
    {
      b = BLOCKVECTOR_BLOCK (bl, bot);
      if (BLOCK_END (b) > pc)
	{
	  if (pindex)
	    *pindex = bot;
	  return bl;
	}
      bot--;
    }

  return 0;
}

/* Return the innermost lexical block containing the specified pc value,
   or 0 if there is none.  */

struct block *
block_for_pc (pc)
     register CORE_ADDR pc;
{
  register struct blockvector *bl;
  int index;

  bl = blockvector_for_pc (pc, &index);
  if (bl)
    return BLOCKVECTOR_BLOCK (bl, index);
  return 0;
}

/* Return the function containing pc value PC.
   Returns 0 if function is not known.  */

struct symbol *
find_pc_function (pc)
     CORE_ADDR pc;
{
  register struct block *b = block_for_pc (pc);
  if (b == 0)
    return 0;
  return block_function (b);
}

/* These variables are used to cache the most recent result
 * of find_pc_partial_function. */

static CORE_ADDR cache_pc_function_low = 0;
static CORE_ADDR cache_pc_function_high = 0;
static char *cache_pc_function_name = 0;

/* Clear cache, e.g. when symbol table is discarded. */

void
clear_pc_function_cache()
{
  cache_pc_function_low = 0;
  cache_pc_function_high = 0;
  cache_pc_function_name = (char *)0;
}

/* Finds the "function" (text symbol) that is smaller than PC but
   greatest of all of the potential text symbols.  Sets *NAME and/or
   *ADDRESS conditionally if that pointer is non-null.  If ENDADDR is
   non-null, then set *ENDADDR to be the end of the function
   (exclusive), but passing ENDADDR as non-null means that the
   function might cause symbols to be read.  This function either
   succeeds or fails (not halfway succeeds).  If it succeeds, it sets
   *NAME, *ADDRESS, and *ENDADDR to real information and returns 1.
   If it fails, it sets *NAME, *ADDRESS, and *ENDADDR to zero
   and returns 0.  */

int
find_pc_partial_function (pc, name, address, endaddr)
     CORE_ADDR pc;
     char **name;
     CORE_ADDR *address;
     CORE_ADDR *endaddr;
{
    struct symbol *func_sym;
    struct partial_symtab *pst;
    struct symbol *f;
    struct minimal_symbol *msymbol;
    struct partial_symbol *psb;
    struct obj_section *sec;

    if (pc >= cache_pc_function_low && pc < cache_pc_function_high)
	    goto return_cached_value;

    if (func_sym=find_pc_function(pc)) {
	struct block *func_block = SYMBOL_BLOCK_VALUE(func_sym);
		
	cache_pc_function_name = SYMBOL_NAME(func_sym);
	cache_pc_function_low  = BLOCK_START(func_block);
	cache_pc_function_high = BLOCK_END(func_block);
	goto return_cached_value;
    }

    /* If sigtramp is in the u area, it counts as a function (especially
       important for step_1).  */
#if defined SIGTRAMP_START
    if (IN_SIGTRAMP (pc, (char *)NULL)) {
	cache_pc_function_low = SIGTRAMP_START;
	cache_pc_function_high = SIGTRAMP_END;
	cache_pc_function_name = "<sigtramp>";
	goto return_cached_value;
    }
#endif

    msymbol = lookup_minimal_symbol_by_pc (pc);
    pst = find_pc_psymtab (pc);
    if (pst) {
	/* Need to read the symbols to get a good value for the end address.  */
	if (endaddr != NULL && !pst->readin) {
	    /* Need to get the terminal in case symbol-reading produces
	       output.  */
	    target_terminal_ours_for_output ();
	    PSYMTAB_TO_SYMTAB (pst);
	}

	if (pst->readin) {
	    /* Checking whether the msymbol has a larger value is for the
	       "pathological" case mentioned in print_frame_info.  */
	    f = find_pc_function (pc);
#if defined(IMSTG)
	    /* For i960 leafprocs, a minimal symbol shows up 4 - 12 bytes 
	       larger than the corresponding function symbol.  We'll take 
	       our chances on the pathological case just mentioned; the 
	       leafproc case is common and we want to report the function 
	       name, not the minimal symbol name. */
	    if (f != NULL)
#else
	    if (f != NULL
	      && (msymbol == NULL
		  || (BLOCK_START (SYMBOL_BLOCK_VALUE (f))
		      >= SYMBOL_VALUE_ADDRESS (msymbol))))
#endif
	    {
		cache_pc_function_low = BLOCK_START (SYMBOL_BLOCK_VALUE (f));
		cache_pc_function_high = BLOCK_END (SYMBOL_BLOCK_VALUE (f));
		cache_pc_function_name = SYMBOL_NAME (f);
		goto return_cached_value;
	    }
	}
	else
	{
	  /* Now that static symbols go in the minimal symbol table, perhaps
	     we could just ignore the partial symbols.  But at least for now
	     we use the partial or minimal symbol, whichever is larger.  */
	  psb = find_pc_psymbol (pst, pc);

	  if (psb
	      && (msymbol == NULL ||
		  (SYMBOL_VALUE_ADDRESS (psb)
		   >= SYMBOL_VALUE_ADDRESS (msymbol))))
	    {
	      /* This case isn't being cached currently. */
	      if (address)
		*address = SYMBOL_VALUE_ADDRESS (psb);
	      if (name)
		*name = SYMBOL_NAME (psb);
	      /* endaddr non-NULL can't happen here.  */
	      return 1;
	    }
	}
    }

  /* Not in the normal symbol tables, see if the pc is in a known section.
     If it's not, then give up.  This ensures that anything beyond the end
     of the text seg doesn't appear to be part of the last function in the
     text segment.  */

  sec = find_pc_section (pc);

  if (!sec)
    msymbol = NULL;

  /* Must be in the minimal symbol table.  */
  if (msymbol == NULL)
    {
      /* No available symbol.  */
      if (name != NULL)
	*name = 0;
      if (address != NULL)
	*address = 0;
      if (endaddr != NULL)
	*endaddr = 0;
      return 0;
    }

  /* See if we're in a transfer table for Sun shared libs.  */

  if (msymbol -> type == mst_text || msymbol -> type == mst_file_text)
    cache_pc_function_low = SYMBOL_VALUE_ADDRESS (msymbol);
  else
    /* It is a transfer table for Sun shared libraries.  */
    cache_pc_function_low = pc - FUNCTION_START_OFFSET;

  cache_pc_function_name = SYMBOL_NAME (msymbol);

  /* Use the lesser of the next minimal symbol, or the end of the section, as
     the end of the function.  */

  if (1) {
      int i;

      for (i=1;;i++) {
	  if (SYMBOL_NAME (msymbol + i) == NULL ||
	      SYMBOL_VALUE_ADDRESS(msymbol+i) > sec->endaddr) {
	      /* We got the start address from the last msymbol in the objfile.
		 So the end address is the end of the section.  */
	      cache_pc_function_high = sec->endaddr;
	      break;
	  }
	  else if (SYMBOL_VALUE_ADDRESS (msymbol + i) <= sec->endaddr
		   && SYMBOL_VALUE_ADDRESS (msymbol + i) > SYMBOL_VALUE_ADDRESS(msymbol)) {
	      cache_pc_function_high = SYMBOL_VALUE_ADDRESS (msymbol + i);
	      break;
	  }
      }
  }

 return_cached_value:
  if (address)
    *address = cache_pc_function_low;
  if (name)
    *name = cache_pc_function_name;
  if (endaddr)
    *endaddr = cache_pc_function_high;
  return 1;
}

#if defined(IMSTG)  /* i960 INLINED_BREAKPOINTS */

/* Used by virtual frame code to determine if a PC falls into the 
   range of instructions inlined by the compiler. */

int
pc_is_inline(pc)
    CORE_ADDR pc;
{
    struct symbol *s;
    if (s = find_pc_function(pc))
	return (SYMBOL_ABSTRACT_ORIGIN(s) != NULL);
    return 0;
}
#endif

/* Return the innermost stack frame executing inside of BLOCK,
   or NULL if there is no such frame.  If BLOCK is NULL, just return NULL.  */

FRAME
block_innermost_frame (block)
     struct block *block;
{
  struct frame_info *fi;
  register FRAME frame;
  register CORE_ADDR start;
  register CORE_ADDR end;

  if (block == NULL)
    return NULL;

  start = BLOCK_START (block);
  end = BLOCK_END (block);

  frame = 0;
  while (1)
    {
      frame = get_prev_frame (frame);
      if (frame == 0)
	return 0;
      fi = get_frame_info (frame);
      if (fi->pc >= start && fi->pc < end)
	return frame;
    }
}

/* Return the full FRAME which corresponds to the given FRAME_ADDR
   or NULL if no FRAME on the chain corresponds to FRAME_ADDR.  */

FRAME
find_frame_addr_in_frame_chain (frame_addr)
     FRAME_ADDR frame_addr;
{
  FRAME frame = NULL;

  if (frame_addr == (CORE_ADDR)0)
    return NULL;

  while (1)
    {
      frame = get_prev_frame (frame);
      if (frame == NULL)
	return NULL;

      if (FRAME_FP (frame) == frame_addr)
	return frame;
    }
}

#ifdef SIGCONTEXT_PC_OFFSET
/* Get saved user PC for sigtramp from sigcontext for BSD style sigtramp.  */

CORE_ADDR
sigtramp_saved_pc (frame)
     FRAME frame;
{
  CORE_ADDR sigcontext_addr;
  char buf[TARGET_PTR_BIT / TARGET_CHAR_BIT];
  int ptrbytes = TARGET_PTR_BIT / TARGET_CHAR_BIT;
  int sigcontext_offs = (2 * TARGET_INT_BIT) / TARGET_CHAR_BIT;

  /* Get sigcontext address, it is the third parameter on the stack.  */
  if (frame->next)
    sigcontext_addr = read_memory_integer (FRAME_ARGS_ADDRESS (frame->next)
					    + FRAME_ARGS_SKIP + sigcontext_offs,
					   ptrbytes);
  else
    sigcontext_addr = read_memory_integer (read_register (SP_REGNUM)
					    + sigcontext_offs,
					   ptrbytes);

  /* Don't cause a memory_error when accessing sigcontext in case the stack
     layout has changed or the stack is corrupt.  */
  target_read_memory (sigcontext_addr + SIGCONTEXT_PC_OFFSET, buf, ptrbytes);
  return extract_unsigned_integer (buf, ptrbytes);
}
#endif /* SIGCONTEXT_PC_OFFSET */

void
_initialize_blockframe ()
{
  obstack_init (&frame_cache_obstack);
}
