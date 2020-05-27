/* BFD library support routines for architectures. */

/* Copyright (C) 1990, 1991 Free Software Foundation, Inc.
 *
 * This file is part of BFD, the Binary File Diddler.
 *
 * BFD is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * BFD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with BFD; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include "sysdep.h"
#include "bfd.h"

#include "libbfd.h"   /* Get definition of BFD_ASSERT(). */

static char *prt_num_mach ();
static boolean scan_num_mach ();
static char *prt_960_mach ();
static boolean scan_960_mach ();

struct arch_print {
	enum bfd_architecture arch;
	char *astr;
	char *(*mach_print)();
	boolean (*mach_scan)();
} arch_print[] = {
	{bfd_arch_unknown,	"unknown",	prt_num_mach,	scan_num_mach},
	{bfd_arch_i960,		"i960",		prt_960_mach,	scan_960_mach},
	{bfd_arch_unknown,	(char *)0,	prt_num_mach,   scan_num_mach},
};

/* Return a printable string representing the architecture and machine
 * type.  The result is only good until the next call to
 * bfd_printable_arch_mach.
 */
char *
bfd_printable_arch_mach (arch, machine)
    enum bfd_architecture arch;
    unsigned long machine;
{
	struct arch_print *ap;

	for (ap = arch_print; ap->astr; ap++) {
		if (ap->arch == arch) {
			if (machine == 0)
				return ap->astr;
			return (*ap->mach_print)(ap, machine);
		}
	}
	return "UNKNOWN!";
}

static char *
prt_num_mach (ap, machine)
    struct arch_print *ap;
    unsigned long machine;
{
	static char result[20];

	sprintf(result, "%s:%ld", ap->astr, (long) machine);
	return result;
}


/* Scan a string and attempt to turn it into an archive and machine type
 * combination.
 */
boolean
DEFUN(bfd_scan_arch_mach,(string, archp, machinep),
CONST char *string AND
    enum bfd_architecture *archp AND
    unsigned long *machinep)
{
	struct arch_print *ap;
	int len;

	/* First look for an architecture, possibly followed by machtype. */
	for (ap = arch_print; ap->astr; ap++) {
		if (ap->astr[0] != string[0])
			continue;
		len = strlen (ap->astr);
		if (!strncmp (ap->astr, string, len)) {
			/* We found the architecture, now see about the
			 * machine type
			 */
			if (archp)
				*archp = ap->arch;
			if (string[len] != '\0') {
				if (ap->mach_scan (string+len, ap, archp, machinep, 1))
					return true;
			}
			if (machinep)
				*machinep = 0;
			return true;
		}
	}

	/* Couldn't find an architecture -- try for just a machine type */
	for (ap = arch_print; ap->astr; ap++) {
		if (ap->mach_scan (string, ap, archp, machinep, 0))
			return true;
	}

	return false;
}

static boolean
scan_num_mach (string, ap, archp, machinep, archspec)
    char *string;
    struct arch_print *ap;
    enum bfd_architecture *archp;
    unsigned long *machinep;
    int archspec;
{
	enum bfd_architecture arch;
	unsigned long machine;
	char achar;

	if (archspec) {

		/* Architecture already specified, now go for machine type.  */
		if (string[0] != ':')
			return false;
		/* Take any valid number that occupies the entire string */
		if (1 != sscanf (string+1, "%lu%c", &machine, &achar))
			return false;
		arch = ap->arch;

	} else {
		/* Couldn't identify an architecture prefix.
		 * Perhaps the entire thing is a machine type.
		 */
		if (1 != sscanf (string, "%lu%c", &machine, &achar))
			return false;
		switch (machine) {
		case 80960:
		case 960:		
			arch = bfd_arch_i960; 
			machine = 0; 
			break;
		default:		
			return false;
		}
	}

	if (archp)
		*archp = arch;
	if (machinep)
		*machinep = machine;
	return true;
}

/* Intel 960 machine variants.  */

static char *
prt_960_mach (ap, machine)
    struct arch_print *ap;
    unsigned long machine;
{
	static char result[20];
	char *str;

	switch (machine) {
	case bfd_mach_i960_core:	str = "core";	break;
	case bfd_mach_i960_core2:	str = "core2";	break;
	case bfd_mach_i960_kb_sb:	str = "kb";	break;
	case bfd_mach_i960_ca:		str = "ca";	break;
	case bfd_mach_i960_jx:		str = "jx";	break;
	case bfd_mach_i960_hx:		str = "hx";	break;
	case bfd_mach_i960_ka_sa:	str = "ka";	break;
	default:
		return prt_num_mach (ap, machine);
	}
	sprintf (result, "%s:%s", ap->astr, str);
	return result;
}

static boolean
scan_960_mach (string, ap, archp, machinep, archspec)
    char *string;
    struct arch_print *ap;
    enum bfd_architecture *archp;
    unsigned long *machinep;
    int archspec;
{
	unsigned long machine;

	if ( !archspec || (*string != ':') ){
		return false;
	}
	string++;

	if ( !strcmp(string,"core") ){
		machine = bfd_mach_i960_core;
	} else if ( !strcmp(string,"core2") ){
		machine = bfd_mach_i960_core2;
	} else if ( !strcmp(string,"kb") || !strcmp(string,"sb") ){
		machine = bfd_mach_i960_kb_sb;
	} else if ( !strcmp(string,"ka") || !strcmp(string,"sa") ){
		machine = bfd_mach_i960_ka_sa;
	} else if ( !strcmp(string,"ca") ){
		machine = bfd_mach_i960_ca;
	} else if ( !strcmp(string,"jx") ){
		machine = bfd_mach_i960_jx;
	} else if ( !strcmp(string,"hx") ){
		machine = bfd_mach_i960_hx;
	} else {
		return false;
	}

	if (archp){
		*archp = ap->arch;
	}
	if (machinep){
		*machinep = machine;
	}
	return true;
}

/* Determine whether two BFDs' architectures and machine types are
 * compatible.  Return merged architecture and machine type if nonnull
 * pointers.
 */
boolean
bfd_arch_compatible (abfd, bbfd, archp, machinep)
    bfd *abfd;
    bfd *bbfd;
    enum bfd_architecture *archp;
    unsigned long *machinep;
{
    enum bfd_architecture archa, archb;
    unsigned long macha, machb;
    int pick_a;

    archa = bfd_get_architecture (abfd);
    archb = bfd_get_architecture (bbfd);
    macha = bfd_get_machine (abfd)-1;
    machb = bfd_get_machine (bbfd)-1;

    if (archb == bfd_arch_unknown)
	    pick_a = 1;
    else if (archa == bfd_arch_unknown)
	    pick_a = 0;
    else if (archa != archb)
	    return false;			/* Not compatible */
    else {
	/* Architectures are the same.  Check machine types.  */
	if (macha == machb)		/* Same machine type */
		pick_a = 1;
	else switch (archa) {
	    /* If particular machine types of one architecture are
	     * not compatible with each other, this is the place to
	     * put those tests (returning false if incompatible).
	     */

    case bfd_arch_i960:
	{
 	    static char compat_matrix[7][7] = {
 		/*                   core, ka/sa, kb/sb, ca, core2, jx, hx */
 		/* core  (0) */   {     1,     1,     1,  1,     1,  1,  1  },
		/* ka/sa (1) */   {     1,     1,     1,  0,     0,  0,  0  },
 		/* kb/sb (2) */   {     1,     1,     1,  0,     0,  0,  0  },
 		/* ca    (3) */   {     1,     0,     0,  1,     0,  1,  1  },
 	        /* core2 (4) */   {     1,     0,     0,  0,     1,  1,  1  },
 	        /* jx    (5) */   {     1,     0,     0,  1,     1,  1,  1  },
 	        /* hx    (6) */   {     1,     0,     0,  1,     1,  1,  1  }  };

 	    /* i960 has three distinct subspecies which may mix:
 	     *	CORE  CA JX HX
 	     *  CORE2 JX HX
 	     *	CORE  KA KB

 	     * So, if either is a ca jx or hx then the other must be a be
 	     * core or ca jx or hx and similarly for KA, KB. */

 	    BFD_ASSERT(macha >= 0 && macha <= 6 && machb >= 0 && machb <= 6);

	    if (!compat_matrix[macha][machb])
		    return false;
	    pick_a = (macha > machb);
	}
	break;

 default:
	pick_a = 1;
    }
    }

    /* Set result based on our pick */
    if (!pick_a) {
	archa = archb;
	macha = machb;
    }
    if (archp)
	    *archp = archa;
    if (machinep)
	    *machinep = (macha+1);

    return true;
}
