/* Flash Programming Algorithm 				*/
/* Copyright Intel Corporation,  1988 			*/
/* All Rights Reserved                 			*/
/*
	This file is a linker/loader directive file.
	It directs the linker to use the NINDY system
	library and NINDY startup routines.
 */
MEMORY
{
        flash: o=0x10000000,l=0x20000
        ram: o=0x08000000,l=0x10000
}

SECTIONS
{
	.text :
        {
        } >flash

	.data :
	{
	_ram = .;
	} >ram

	.bss : 
	{
	_bss_start=.;
	} >ram
}

STARTUP (crtfnin.o)
HLL     ()
SYSLIB  (libnin.a, libqt.a)
FLOAT

/*
#*move $0
*/
