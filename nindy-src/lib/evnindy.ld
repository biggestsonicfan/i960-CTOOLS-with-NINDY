/*
	This file is a linker/loader directive file
	for applications running on the EV80960CA board.
	It directs the linker to use the NINDY system
	library and NINDY startup routines.
 */
MEMORY
{
        eprom : o=0xffff0000, l=0xffff	/* 64K */
        dram  : o=0xe0000000, l=0xf8000 /* 1M - (32K for NINDY) */
        sram  : o=0xb0000000, l=0x10000	/* 64K */
        caram : o=0x0, l=0x400		/* 1K */
}

fpem_CA_AC = 0x100;

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
	_bss_start=.;
	} >dram
}

STARTUP (crtnin.o)
HLL ()
SYSLIB (libnin.a, libevca.a)
FLOAT 
