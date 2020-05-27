
#ifdef PROCEDURE_PLACEMENT
#include "sysdep.h"
#include "bfd.h"
#include "ldlang.h"


extern unsigned long ldfile_output_machine;

extern int is_really_i960_cf;


unsigned int
log2_bytes_in_cache_line ()
{
   if (ldfile_output_machine == bfd_mach_i960_ca)
      return 5;
   else
      return 6;
}


unsigned int
cache_line_size ()
{
   if (ldfile_output_machine == bfd_mach_i960_ca)
      return 32;
   else
      return 16;
}


/* note: this had better be a power of 2 */
unsigned int
number_of_cache_lines ()
{
   if (is_really_i960_cf)
      return 64;
   else if (ldfile_output_machine == bfd_mach_i960_ca)
      return 16;
   else
      return 32; /* SA, SB, KA, KB, ... what about JX/HA ? */
}


unsigned int
cache_line_alignment ()
{
   if (ldfile_output_machine == bfd_mach_i960_ca)
      return 5;
   else
      return 4;
}

unsigned int
cache_half_line (addr)
unsigned long addr;
{
   unsigned int half_line_num;

   half_line_num = (addr >> (log2_bytes_in_cache_line() - 1)) & 
			((number_of_cache_lines() * 2) - 1);

   return half_line_num;
}
#endif
