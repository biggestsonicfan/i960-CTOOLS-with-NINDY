/*
	This file is a linker/loader directive file.
	It directs the linker to use the NINDY system
	library and NINDY startup routines.
 */
MEMORY
{
        rom: o=0x0,l=0x20000
        ram: o=0x8020000,l=0x60000
}

SECTIONS
{
	.text :
        {
        } >ram

	.data :
	{
	} >ram

	.bss : 
	{
	_bss_start=.;
	} >ram
}

STARTUP (crtnin.o)
HLL     ()
SYSLIB  (libnin.a, libqt.a)
FLOAT
