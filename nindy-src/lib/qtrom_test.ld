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

_heap_size = 0x2000;
_heap_base = (_end + 0xf) & ~0xf;
_heap_end = _heap_base + _heap_size - 1;

_stackbase = (_heap_end + 0x10) & ~0x0f;

HLL ()
FLOAT
