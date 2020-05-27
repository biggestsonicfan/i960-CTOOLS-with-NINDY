
/* 
   This program generates a header file called "i_copreg.h".

   This header file is #included from i960.h.  It sets up the compiler to
   support one or more sets of co-processor registers.  Unless you
   build the compiler with IMSTG_COPREGS defined, i_copreg.h will not be used.

   You change the initializer for 'reg_info' to change the co-processor
   support.  This support is the minimum needed by the compiler to manage
   co-processor registers; it is just barely enough to let the compiler
   know how to spill and reload registers for each co-processor.

   Each reg_info[] array entry generates one co-processor register class.

   Each class corresponds to a different co-processor.  The classes have
   constraint letters 'u','v', ... and are named COP_REG1, COP_REG2, ...

   The co-processors are accessed by the user via implementation-provided
   static inline functions which use a single asm, e.g,

   static __inline addu(l,r)
   {
     int _t;
     __asm ("cpdcp\t0,%2,%1,%0" : "=u"(_t):"u"(l),"u"(r));
     return _t;
   }

   The implementation should supply one of these for each co-processor
   operation (eg, add, sub, etc) for each co-processor (u, v, ...).
   Note that the encoding of the operation (first field) has been left
   empty (as have the encodings for the move operations in 'reg_info').
   You will need to fill these out with meaningful encodings for your
   particular setup.

   To make these defintions available to users, don't forget that the
   pre-include switch can be used to add an include file before ever
   .c file to be compiled.

   Generally, the user can use these functions without worrying about
   where the operands are allocated.  The compiler will generate the
   moves or loads required to satisfy the constraints, will allocate
   operands in the co-processor registers as appropriate, and will manage
   spills to memory or to 960 registers as needed.

   Co-processor registers are all word sized call-clobbered, and are
   suitable for modes QI,HI,SI, and SF.

   If call-preserved registers are desired, that's probably easy to add,
   by changing the generation of COP_CALL_USED_REGS.  If you want these
   babies in the calling sequence for register parameter passing, good luck;
   that's a bunch of work.

   If a co-processor is integer-only or floating point only, having the
   registers set up for QI/HI/SI/SF is still probably appropriate;  the
   semantics exposed to the user should reflect this, however.  For example,
   there would be an addf with float arguments for a floating point
   co-processor, but not for an integer co-processor.  Be careful here that
   users do their conversions to and from float via your provided
   co-processor functions and not implicitly on the 960 (unless that
   is what you want, of course).

   As far as the register allocator is concerned, the registers are
   numbered contiguously.  To change the number of registers in class 'r',
   you change the number of the first register in class 'r'+1.  Co-processor
   numbers start at 38. Only FIRST_CREG should have to change if you
   add more 960 registers.

   For each class, you supply strings to move 960 or co-processor registers to
   co-processor regs, and a string to move co-processor regs to 960 registers.
   These strings are referenced in the move patterns in i960.md.

   The sample support provided by the current version of the regs_info
   initializer sets up the compiler for 2 co-processors of 4 regs each,
   with dummy encodings of 0 for the operand encodings of the move
   instructions.
*/

typedef struct {
  int first;		/* First REGNO belonging to this class.  # of regs for
			   class R is reg_info[R+1].first-reg_info[R].first */

  int offset;		/* Add this to encoding after subtracting 'first' */

  char* to_960_reg;	/* Asm string to move to 960 "d" registers from R */
  char* fm_960_reg;	/* Asm string to move from 960 "d" registers to R */
  char* fm_cop_reg;	/* Asm string to move from R to R registers */
} cop_reg_class;

#define FIRST_CREG 38

cop_reg_class reg_info[]
={ { FIRST_CREG+0, 0,
     "cpd960	0,%1,0,%0	#putu",
     "cpdcp	0,%1,0,%0	#getu",
     "cpdcp	0,%1,0,%0	#cpyu",
   },
   { FIRST_CREG+4, 0,
     "cpd960	0,%1,0,%0	#putv",
     "cpdcp	0,%1,0,%0	#getv",
     "cpdcp	0,%1,0,%0	#cpyv",
   },
   { FIRST_CREG+8, 0, 0, 0 },	/* Required for sizing of last class */
 };

#include <stdio.h>
#include <assert.h>
#include <string.h>

main (argc, argv)
int argc;
char *argv[];
{
  int num_classes = sizeof(reg_info)/sizeof(reg_info[0]) - 1;
  int num_regs = reg_info[num_classes].first - reg_info[0].first;
  int reg_set_wds = 1 + (FIRST_CREG + num_regs) / 32;

  int clas, reg;
  char *miname, *t;

  FILE *reg_hdr;

  miname = argv[0];

  if (t = strrchr (miname, '\\'))
    miname = t+1;

  if (t = strrchr (miname, '/'))
    miname = t+1;

  if (argc != 1)
  { fprintf (stderr, "no arguments allowed for %s\n", argv[0]);
    exit (1);
  }

  reg_hdr = fopen ("i_copreg.h", "w");
  if (reg_hdr == 0)
  { fprintf (stderr, "cannot open i_copreg.h for write\n");
    exit (1);
  }

  fprintf (reg_hdr, "/* WARNING - this file was generated by '%s' - do not edit */\n", miname);

  fprintf (reg_hdr, "\n#define COP_FIRST_REGISTER %d", FIRST_CREG);
  fprintf (reg_hdr, "\n#define COP_LAST_REGISTER %d", (FIRST_CREG+num_regs)-1);
  fprintf (reg_hdr, "\n#define COP_NUM_REGISTERS %d", num_regs);
  fprintf (reg_hdr, "\n#define COP_NUM_CLASSES %d", num_classes);

  fprintf (reg_hdr, "\n#define COP_REGNO_MODE_OK_INIT \\\n");
  for (reg = 0; reg < num_regs; reg++)
    fprintf (reg_hdr, "S_MODES,");

  fprintf (reg_hdr, "\n#define COP_FIXED_REGISTERS \\\n");
  for (reg = 0; reg < num_regs; reg++)
    fprintf (reg_hdr, "0,");

  fprintf (reg_hdr, "\n#define COP_CALL_USED_REGISTERS \\\n");
  for (reg = 0; reg < num_regs; reg++)
    fprintf (reg_hdr, "1,");

  fprintf (reg_hdr, "\n#define COP_REG_ALLOC_ORDER \\\n");
  for (reg = 0; reg < num_regs; reg++)
    fprintf (reg_hdr, "%d,", reg+FIRST_CREG);

  fprintf (reg_hdr, "\n#define COP_REG_CLASS_NAMES ");
  for (clas = 0; clas < num_classes; clas++)
    fprintf (reg_hdr, "\"COP%d_REGS\",", clas);

  fprintf (reg_hdr, "\n#define COP_REG_CLASS_ENUM ");
  for (clas = 0; clas < num_classes; clas++)
    fprintf (reg_hdr, "COP%d_REGS,", clas);

  fprintf (reg_hdr, "\n#define COP_REG_CONTENTS ");
  for (clas = 0; clas < num_classes; clas++)
  { unsigned wds[32];  int i;

    assert (sizeof (wds) >= reg_set_wds * sizeof (unsigned));
    memset ((char*) wds, 0, reg_set_wds * sizeof (unsigned));

    for (reg = reg_info[clas].first; reg < reg_info[clas+1].first; reg++)
      wds[reg/32] |= (1 << (reg % 32));

    fprintf (reg_hdr, "{");
    for (i = 0; i < reg_set_wds; i++)
      fprintf (reg_hdr, "0x%08x,", wds[i]);
    fprintf (reg_hdr, "},");
  }

  fprintf (reg_hdr, "\n#define ALL_REG_CONTENTS {");
  { unsigned wds[32];  int i;
    for (reg = 0; reg < reg_info[num_classes].first; reg++)
      wds[reg/32] |= (1 << (reg % 32));
    for (i = 0; i < reg_set_wds; i++)
      fprintf (reg_hdr, "0x%08x,", wds[i]);
  }
  fprintf (reg_hdr, "}");

  fprintf (reg_hdr, "\n#define COP_REG_NAMES");
  for (clas = 0; clas < num_classes; clas++)
  { 
    fprintf (reg_hdr, " \\\n");
    for (reg = reg_info[clas].first; reg < reg_info[clas+1].first; reg++)
      fprintf (reg_hdr, "\"%d\",", (reg-reg_info[clas].first)+reg_info[clas].offset);
  }

  fprintf (reg_hdr, "\n#define COP_TO_960 {");
  for (clas = 0; clas < num_classes; clas++)
  { fprintf (reg_hdr, " \\\n");
    fprintf (reg_hdr, "\"%s\",", reg_info[clas].to_960_reg);
  }
  fprintf (reg_hdr, "}\n");

  fprintf (reg_hdr, "\n#define COP_FM_960 {");
  for (clas = 0; clas < num_classes; clas++)
  { fprintf (reg_hdr, " \\\n");
    fprintf (reg_hdr, "\"%s\",", reg_info[clas].fm_960_reg);
  }
  fprintf (reg_hdr, "}\n");

  fprintf (reg_hdr, "\n#define COP_FM_COP {");
  for (clas = 0; clas < num_classes; clas++)
  { fprintf (reg_hdr, " \\\n");
    fprintf (reg_hdr, "\"%s\",", reg_info[clas].fm_cop_reg);
  }
  fprintf (reg_hdr, "}\n");

  fclose (reg_hdr);

  return 0;
}
