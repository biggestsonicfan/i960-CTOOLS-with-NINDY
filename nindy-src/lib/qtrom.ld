
/*
	This file is a linker/loader directive file. 
	It assumes the system library and startup routines are
	part of the included files and does not automatically
	include any.  It does specify the checksum to be
	performed during startup.
*/
ram_ = 0x0801C000;

MEMORY
{
        rom: o=0x0,l=0x20000
        ram: o=0x0801C000,l=0x4000
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

HLL ()
FLOAT

/*
#*move $0
#*mkimage $0 image
#*split image 131072 64 131072 16 out
#*ihex out.00 qt0.hex
#*ihex out.01 qt1.hex
#*ihex out.02 qt2.hex
#*ihex out.03 qt3.hex
*/

