/*
	This file is a linker/loader directive file. 
	It is used for burning a "NINDY" or other monitor
	into Flash EPROM space.
	use:

	lnk960 thisfile.ld monitorfiles.o -o name
	rom960 thisfile name

	and download with df from the monitor.
	Turn off board.
	Reset the jumpers on the front right corner to be
	jumpered J23-J26 and J22-J24.
	Upon powerup the board will be running out of the Flash
	space.
*/
ram_ = 0x0801c000;

MEMORY
{
        rom: o=0x0,l=0x20000
        ram: o=0x0801c000,l=0x4000
}

SECTIONS
{
	.text :
        {
        } >rom

	.data : 
	{
	} >ram

	.bss  : 
	{	
	} >ram
}

cs1 = -(system_address_table + prcb_ptr + start_ip);

HLL()
FLOAT

/*
#*move $0 .text 0x10000000
#*move $0
*/
