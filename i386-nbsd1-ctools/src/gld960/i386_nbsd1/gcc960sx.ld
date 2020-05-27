
/*
This file is used for the gcc960 hosted assembler.

It is for the mon960 monitor and on a tomcat board.
*/

/*sram_length       = 2*(1<<20);                     Total of 2MB of sram. */
/*mon_sram_needs    = 32*(1<<10);                    mon960 needs 32K */
/*application_sram  = sram_length - mon_sram_length; Resulting sram available to application. */
/*start_of_app_sram = 0x18008000; */
/*end_of_sram       = start_of_sram + application_sram; */

MEMORY
{
        isram : o=0x0, l=0x400		                   /* 1K */
        sram  : o=0x18008000, l=(2*(1<<20)) - (32*(1<<10)) /* sram available to application. */
}


SECTIONS
   {
      .text :
         {
		*(.text)
         } >sram

      .data 0xfffffff0 & (0xf+ADDR(.text)+SIZEOF(.text)+SIZEOF(.data)) :
         {
		*(.data)
         }

      .bss 0xfffffff0 & (0xf+ADDR(.data)+SIZEOF(.data)): 
         {
		[COMMON]
		*(.bss)
         }
   }

/* Arithmetic controls location when using floating point library. */
fpem_CA_AC = 0x100;


/* Bounds of heap: */
/* The heap may be placed in a separate memory region, if desired. */
_heap_size = (0x18008000+(2*(1<<20)) - (32*(1<<10))) - (_end + 64);
_heap_base = (_end + 0xf) & ~0xf;
_heap_end = _heap_base + _heap_size - 1;

/* _stackbase marks base of user stack */
/* stack is allocated following the heap. */
/* The stack may be based in a separate memory region, if desired. */
_stackbase = (_heap_end + 0x10) & ~0x0f;

STARTUP (crt960g.o)
HLL ()
SYSLIB (libtcg.a)
SYSLIB (libllg.a)
FLOAT
/*
#*move $0
*/
