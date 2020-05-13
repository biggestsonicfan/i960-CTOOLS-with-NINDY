/******************************************************************/
/* 		Copyright (c) 1989, Intel Corporation

   Intel hereby grants you permission to copy, modify, and 
   distribute this software and its documentation.  Intel grants
   this permission provided that the above copyright notice 
   appears in all copies and that both the copyright notice and
   this permission notice appear in supporting documentation.  In
   addition, Intel grants this permission provided that you
   prominently mark as not part of the original any modifications
   made to this software or documentation, and that the name of 
   Intel Corporation not be used in advertising or publicity 
   pertaining to distribution of the software or the documentation 
   without specific, written prior permission.  

   Intel Corporation does not warrant, guarantee or make any 
   representations regarding the use of, or the results of the use
   of, the software and documentation in terms of correctness, 
   accuracy, reliability, currentness, or otherwise; and you rely
   on the software, documentation and results solely at your own 
   risk.							  */
/******************************************************************/
#include "defines.h"
#include "test_def.h"

/****************************************/
/*	SERIAL TEST A			*/
/* This test will initialize the 510 and*/
/* send all printible ASCII characters	*/
/* to the console for verification 	*/
/****************************************/
serial_test_a()
{
unsigned char i;

    	init_console(9600);

	/* print all printable ASCII characters */
    	for (i=0x20; i<0x5a; i++)  
        	co (i); 
    	co ('\r');             
    	for (i=0x5a; i<0x7f; i++)
        	co (i);
}

/************************************************/
/*	LED TEST A				*/
/* this test will verify that the LED indicators*/
/* are functioning correctly 			*/
/************************************************/
led_test_a()
{
volatile unsigned int *user_led01;
volatile unsigned int *user_led23;
int i;

    	user_led01 = (unsigned int *)USER_LED01;
    	user_led23 = (unsigned int *)USER_LED23;
    	*user_led01 =0;
    	*user_led23 =0;            /* turn off LEDs */

    	for(i=0; i<=300000; i++) {
       		*user_led01 = 3;     /* turn on all user LED's */
       		*user_led23 = 3;   
    	}

       	*user_led01 = 0;             /* turn off all user LED's */
       	*user_led23 = 0;         
}

/****************************************/
/*	EPROM TEST A			*/
/* this test will verify that the high  */
/* speed EPROM is working by computing  */
/* a checksum of the established EPROM  */
/****************************************/
eprom_test_a()
{
volatile unsigned char *data_ptr;	/* data ptr to be changed */
unsigned short total;              	/* computed value of contents */
unsigned int eprom_size;
unsigned int *size_ptr;          	/* ptr to size value of EPROM */
unsigned char value;
int i;

   	/* computes EPROM size for use later */
   	size_ptr = (unsigned int *) EPROM_SIZE;
   	eprom_size = (1 << (15 + *size_ptr));
  
   	total = 0;

   	/* sets starting address */
   	data_ptr = (unsigned char *) EPROM_START;  

        /* computes total EPROM values */
   	for (i=0; i<=eprom_size; i++) {
          	value = *data_ptr; 
          	total += (unsigned short) value;
          	*data_ptr++; 
	} 
}

/********************************************************/
/*	FLASH TEST A					*/
/* This test will assign a 0x2k buffer in SRAM which 	*/
/* will be filled by a coresponding amount of Flash data*/
/* The buffer will then be verified using single, double*/
/* triple, and quad word reads and comparing with the   */
/* original Flash data. This test will continue 	*/
/* throughout the entire SRAM by incrementing the SRAM  */
/* address by the buffer.               		*/
/********************************************************/
flash_test_a()
{
#define BUFFER 0x2000
unsigned int *flash_ptr;
unsigned int *sram_ptr;
unsigned int end_flash;
unsigned int flash_size;
volatile unsigned char *check;
unsigned int flash_addr = FLASH_ADDRESS;
int i;
unsigned int a,b,c,d;

	check = (unsigned char *)flash_addr;
	*check = 0x90;

	/* find out if devices are really Flash */
	if (*check != 0x89) 
		return;	

	/* find out what size devices are present */
	check = (unsigned char *)(flash_addr + 4);
	if ((*check == 0xB1) || (*check == 0xB2) || (*check == 0xB9))  {
		*check = 0xff;
		*check = 0xff;
		*check = 0x0;
		flash_size = 0x20000;   /* 256 Kbit Flash */
	}
	if (*check == 0xB8) {
		*check = 0xff;
		*check = 0xff;
		*check = 0x0;
		flash_size = 0x40000;   /* 512 Kbit Flash */
	}
	if (*check == 0xB4) {
		*check = 0xff;
		*check = 0xff;
		*check = 0x0;
		flash_size = 0x80000;   /* 1 Mbit Flash */
	}
	if (*check == 0xBD) {
		*check = 0xff;
		*check = 0xff;
		*check = 0x0;
		flash_size = 0x100000;   /* 2 Mbit Flash */
	}

   	flash_ptr = (unsigned int *) flash_addr;
   	sram_ptr = (unsigned int *) 0x08018000;
	end_flash = flash_addr + flash_size;

   	/* uses all of Flash */
   	while ((unsigned int)flash_ptr < end_flash) {

       		/* assigns Flash values to SRAM values */
       		while ((unsigned int)flash_ptr <= (flash_addr + BUFFER)) { 
              		*sram_ptr = *flash_ptr;
              		flash_ptr++;  /* increments both array spaces */
              		sram_ptr++; 
		}

/* SINGLE WORD PASS */
 		/* reset starting address */
      		flash_ptr = (unsigned int *)flash_addr;
      		sram_ptr = (unsigned int *)0x08018000;
	
      		for (i=0; i<=BUFFER; i+=4) {
             		/* verify for error */
             		if (*sram_ptr != *flash_ptr) {
             			while (TRUE)
					led_test_a();
			}
			sram_ptr++;
	     		flash_ptr++;
		}

/* DOUBLE WORD PASS */
		/* reset starting address */
      		sram_ptr = (unsigned int *)0x08018000;
      		flash_ptr = (unsigned int *)flash_addr; 
	
      		for (i=0; i<=(BUFFER-8); i+=8) {
	     		a = *sram_ptr++;
	     		b = *sram_ptr++;
    	     		check_2(flash_ptr, a, b);
	     		flash_ptr +=2;
        	}

/* TRIPLE WORD PASS */
		/* reset starting address */
		sram_ptr = (unsigned int *) 0x08018000;
      		flash_ptr = (unsigned int *) flash_addr; 
	
      		for (i=0; i<=(BUFFER-12); i+=12) {
	     		a = *sram_ptr++;
	     		b = *sram_ptr++;
	     		c = *sram_ptr++;
             		check_3(flash_ptr, a,b,c);

             		/* keeps word boundries even */
             		flash_ptr += 3;
        	}

/* QUAD WORD PASS */
		/* reset starting address */
      		sram_ptr = (unsigned int *) 0x08018000;
      		flash_ptr = (unsigned int *) flash_addr;
	
      		for (i=0; i<=(BUFFER-16); i+=16) {
	     		a = *sram_ptr++;
	     		b = *sram_ptr++;
	     		c = *sram_ptr++;
	     		d = *sram_ptr++;
             		check_4(flash_ptr, a,b,c,d);
     	     		flash_ptr += 4;
        	}
	
      		flash_addr += BUFFER;

		/* reset starting address */
      		sram_ptr = (unsigned int *) 0x08018000;
      		flash_ptr = (unsigned int *) flash_addr;
   	}
}

/***************************************************************/
/*	PROGRAMMABLE WAIT STATE GENERATOR TEST A	       */
/* This test will verify that the LEDs, and first and burst    */
/* wait-state control-status registers are properly            */
/* functioning by writting 0-3 to each with Wait State Register*/
/* 1, (18010071h) changed.              		       */
/***************************************************************/
pwsg_test_a()
{
int i;
unsigned char **j;
unsigned int *csr_ptr;

/* sets up array with address spaces */
unsigned char *csr_reg[] = {
      (unsigned char *) USER_LED01,
      (unsigned char *) USER_LED23,
      (unsigned char *) BURST_CYCLE,
      (unsigned char *) FIRST_CYCLE,
      (unsigned char *) 0
};

   	*((unsigned char*)OFF_WAIT_STATE) = 0x00; /* sets off chip regulator */

      	/* assigns CSR locations in array to "j" fills */
	/* up array with 0-3 then reads and verifies   */
   	for (i=2; i<=3; i++) {
      		j = csr_reg;
      		while ((int)* j != 0)
         		**j++ = (unsigned char) i;
      		j = csr_reg;

		/* verification for correct value */
      		while ((int)*j != 0)
      			if ((**j++ & 0x3) != i) 
				while (TRUE)
					led_test_a();
   	}
}
