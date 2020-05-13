/* Linker directive file for the EV80960CA (EVCA) board */

ram_ = 0xe00f8000;

MEMORY
{
	sram 	:  org = 0xB0000000, len = 0x10000   /* 64K */
        dram 	:  org = 0xE00f8000, len = 0x8000    /* 32K */
        eprom	:  org = 0xffff0000, len = 0xffff    /* 64k 27512 */
}

SECTIONS
{
	ibr_sec 0xffffff00:
  	{
        ev_boot.o		/* locates inital boot record */
        }

	.text :
    	{
	} >eprom
                
	.data :
    	{
      	} >dram 

     	.bss : 
     	{
	} >dram
} 

cs6 = -(rom_prcb + start_ip);	/* 6th powerup checksum word:  see ev_boot.s */

HLL ()
NOFLOAT

/*
#*rom 0xffff 8
#*move $0 .text 0x0
#*move $0
#*move $0 ibr_sec 0xff00
#*mkimage $0 image
#*ihex image evca.hex
*/
