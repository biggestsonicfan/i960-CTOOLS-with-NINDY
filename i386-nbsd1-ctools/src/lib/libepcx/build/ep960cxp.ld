/*
 *   EP960CXP.LD
 *
 *   This is the linker directive file to link a position-independent
 *   application to run under mon960 on the EP960CX evaluation board.
 *
 *   Programs linked with this file are allocated to DRAM after the
 *   data area used by mon960.  When downloaded to the board, your
 *   execution vehicle (ICE, debugger, emulator, etc.) can load the
 *   the .text section, and the .data and .bss sections, at arbitrary
 *   but non-conflicting memory locations.
 *
 *   Note that .data and .bss must be relocated as a unit.  Your execution
 *   mechanism must initialize register g12 to the difference between the
 *   link-time and load-time address of the .data section.  This
 *   initialization must occur before any code that references data
 *   is executed.
 *
 *   malloc() uses the symbols _heap_base and _heap_end to manage the heap.
 *   _heap_base is located at the end of program memory (.text + .data +.bss).
 *   _heap_size is defined as 0x20000.  _heap_base and _heap_end are biased
 *   by the value in register g12 at runtime.
 *   The C runtime stack starts at _heap_end and grows up.
 *   There must be enough room after the program memory in DRAM for
 *   the program's heap and stack.
 */

MEMORY
   {
      dram  :  org = 0xC0008000, len = 0xff8000   /* 16M less monitor */
      isram :  org = 0x00000100, len = 0x300
   }


SECTIONS
   {
      .text :
         {
         } >dram

      .data :
         {
         } >dram

      .bss :
         {
         } >dram


/* For version 4.0 of the iC-960 compiler & libraries, the */
/* following method for defining fpem_CA_AC should be used. */
/*
        SFP_AC :
        {
           fpem_CA_AC = .;
        } >isram
*/
   }

/* Arithmetic controls location when using floating point library. */
/* Uncomment the following definition of fpem_CA_AC if not using   */
/* version 4.0 of the iC-960 compiler & libraries. */
fpem_CA_AC = 0x100;


/* Bounds of heap: */
/* The heap may be placed in a separate memory region, if desired. */
_heap_size = 0x20000;
_heap_base = (_end + 0xf) & ~0xf;
_heap_end = _heap_base + _heap_size - 1;

/* _stackbase marks base of user stack */
/* stack is allocated following the heap. */
/* The stack may be based in a separate memory region, if desired. */
_stackbase = (_heap_end + 0x10) & ~0x0f;

STARTUP (crt960_p.o)
HLL ()
SYSLIB (libmon_p.a)
SYSLIB (libll_p.a)
FLOAT
