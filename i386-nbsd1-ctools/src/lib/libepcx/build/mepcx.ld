/*
 *   EPCX.LD              August, 1992
 *
 *   This is the linker directive file to link an application to run
 *   under mon960 on the EPCX board.
 *   Programs linked with this file are downloaded to DRAM after the
 *   data area used by mon960.
 *   malloc() uses the symbols _heap_base and _heap_end to manage the heap.
 *   _heap_base is located at the end of program memory (.text + .data +.bss).
 *   _heap_size is defined as 0x20000.
 *   The C runtime stack starts at _heap_end and grows up.
 *   There must be enough room after the program memory in the dram for
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
   }

/* Arithmetic controls location when using floating point library. */
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

STARTUP (crt960g.o)
HLL ()
SYSLIB (libepcxg.a)
SYSLIB (libllg.a)
FLOAT
