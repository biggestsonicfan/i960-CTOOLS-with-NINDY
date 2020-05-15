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
