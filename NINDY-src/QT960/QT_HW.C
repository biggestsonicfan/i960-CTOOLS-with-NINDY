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
#include "globals.h"
#include "qtcommon.h"

/****************************************/
/* 	Reset Board                     */
/****************************************/
reset()
{
	init_380_timers();
	store_byte (RESET_DATA, RESET_ADDR);
}

/****************************************/
/* 	Init 380 Timers                 */
/****************************************/
init_380_timers()
{

	store_byte (0x30, CWR1); /* MSB,LSB,16-bit binary,timer 0 */
	store_byte (0x02, CR0);  /* LSB initial count */
	store_byte (0x00, CR0);  /* MSB initial count */

	store_byte (0x70, CWR1); /* MSB,LSB,16-bit binary,timer 1 */
	store_byte (0x02, CR1);  /* LSB initial count */
	store_byte (0x00, CR1);  /* MSB initial count */

	store_byte (0xb0, CWR1); /* MSB,LSB,16-bit binary,timer 2 */
	store_byte (0x02, CR2);  /* LSB initial count */
	store_byte (0x00, CR2);  /* MSB initial count */

	store_byte (0x30, CWR2); /* LSB,MSB,16-bit binary,timer 3 */
	store_byte (0x02, CR3);  /* LSB initial count */
	store_byte (0x00, CR3);  /* MSB initial count */
	
	store_byte(0x00, CNTL1);   /* disable GATE */
}

/****************************************/
/* Clear all the 380 interrupt vectors	*/
/* 		           	        */
/* Set all interrupt vectors to prioity */
/* 8; effectivly shutting them off for  */
/* all higher priorities                */
/****************************************/
clear_380_intr_vectors()
{
  /* neutralize all vectors to interrupt level 8 */
  /* inside the 380 vector registers             */
	
  /* BANK A */
  store_byte (8, VR0);   /* interrupt vector 0 */
  store_byte (8, VR1);   /* interrupt vector 1 */
  store_byte (8, VR1_5); /* interrupt vector 1.5 */
  store_byte (8, VR3);   /* interrupt vector 3 */
  store_byte (8, VR4);   /* interrupt vector 4 */
  store_byte (8, VR7);   /* interrupt vector 7 */
  /* Bank B */
  store_byte (8, VR8);   /* interrupt vector 8 */
  store_byte (8, VR9);   /* interrupt vector 9 */
  store_byte (8, VR11);  /* interrupt vector 11 */
  store_byte (8, VR12);  /* interrupt vector 12 */
  store_byte (8, VR13);  /* interrupt vector 13 */
  store_byte (8, VR14);  /* interrupt vector 14 */
  store_byte (8, VR15);  /* interrupt vector 15 */
  /* Bank C */
  store_byte (8, VR16);  /* interrupt vector 16 */
  store_byte (8, VR17);  /* interrupt vector 17 */
  store_byte (8, VR18);  /* interrupt vector 18 */
  store_byte (8, VR19);  /* interrupt vector 19 */
  store_byte (8, VR20);  /* interrupt vector 20 */
  store_byte (8, VR21);  /* interrupt vector 21 */
  store_byte (8, VR22);  /* interrupt vector 22 */
  store_byte (8, VR23);  /* interrupt vector 23 */

}

/****************************************/
/* Disable interrupts in 380		*/
/* 		           	        */
/* Initalize the 82380's interrupts and */
/* possible interrupt sources to knowen */
/* and safe states                      */
/****************************************/
disable_ints()
{
unsigned char dummy;
unsigned int reg;
	
/* Reset all 380 interrupt vectors by setting to priority 8 */
        clear_380_intr_vectors();

/* Mask off all interrupts within the 380 VIA the Mask registers */
        store_byte (0xff, OCW1_A); /* Disable bank A */
        store_byte (0xff, OCW1_B); /* Disable bank B */
        store_byte (0xff, OCW1_C); /* Disable bank C */

/* Acknowlege all interrupts that possibly are pending */

        /* Bank C */
        store_byte (0x0c,OCW3_C);  /* enable pole register read           */
        dummy = load_byte(READ_C); /* read pole reg "acks any interrupts" */
        dummy = load_byte(MASK_C); /* read the mask register (not needed?)*/

        /* Bank B */
        store_byte (0x0c,OCW3_B);
        dummy = load_byte(READ_B);
        dummy = load_byte(MASK_B);

        /* Bank A */
        store_byte (0x0c,OCW3_A);
        dummy = load_byte(READ_A);
        dummy = load_byte(MASK_A);
/* set the 80960KX interrupt register for a exsternal 8259 */
reg = 0xff000808;
interrupt_register_write(&reg);

/*
 * Set Bank A for Master controller (82380) 
 * FOR BANK A                               
 *            EDGE TRIGERED                 
 *            EXSTERNAL CASCADE             
 *            ICW4 NEEDED                                
 */
	store_byte (0x11, ICW1_A );     /* ICW1 */
	store_byte (0x20, ICW2_A );    	/* ICW2 */
	store_byte (0x08, ICW3_A );    	/* ICW3 */ 
	store_byte (0x12, ICW4_A );	/* ICW4 */

/* clear off bank A level 1.5 interrupts created during programing */
       dummy = load_byte(ICW2R_A);

/* Set bank B in Cascaded(Slave) Specal fully Nested mode.     
 * FOR BANK B                               
 *            EDGE TRIGARED                 
 *            EXSTERNAL CASCADE             
 *            ICW4 NEEDED                   
 */
	store_byte (0x11, ICW1_B ); /* ICW1 */
	store_byte (0x20, ICW2_B ); /* ICW2 */
	store_byte (0x04, ICW3_B ); /* ICW3 */
	store_byte (0x02, ICW4_B ); /* ICW4 */

/* clear off bank B level 1.5 interrupts created during programing */
       dummy = load_byte(ICW2R_B);

/* Set bank C in Fully nested with no cascades.       
 * FOR BANK C                               
 *            EDGE TRIGARED                 
 *            NO EXSTERNAL CASCADE          
 *            NO ICW3 NEEDED                
 *            NO ICW4 NEEDED 
 */               
       store_byte (0x12, ICW1_C ); /* ICW1 */
       store_byte (0x20, ICW2_C ); /* ICW2 */

/* clear off bank C level 1.5 interrupts created during programing */
       dummy = load_byte(ICW2R_C);


/* Initalize the 82380's on chip timer/counters 
 * so they won't cause interrupts
 */
       init_380_timers();
}

/************************************************/
/* DRAM Initialization				*/
/*                           			*/
/************************************************/
init_dram()
{
}

/************************************************/
/* Check if Flash is Blank   			*/
/*                           			*/
/************************************************/
check_flash()
{
unsigned int *addr, a, md1, md2;
unsigned int *first_addr, *last_addr, *next_addr;
unsigned int data1, data2, size;

	addr = (unsigned int *)FLASH_ADDR;

	size = FLASH_ADDR;

	switch (fltype){
	case ERROR:
		return (ERROR);
	case 256:
	case 257:
		size += 0x20000;
		break;
	case 512:
		size += 0x40000;
		break;
	case 1024:
		size += 0x80000;
		break;
	default:
		size += 0x100000;
		break;
	}

	first_addr = last_addr = (unsigned int *)size;

	while (addr < (unsigned int *)size) {

		/* find first non_blank address */
		if (*addr != 0xffffffff) {
			first_addr = addr;
			break;
		}
		addr++;
	}
	if (first_addr != last_addr) {
		addr = (unsigned int *)(size - 4);
		while (addr > first_addr) {

			/* find last non_blank address */
			if (*addr != 0xffffffff) {
				last_addr = addr;
				break;
			}
			addr--;
		}
	}
	if (last_addr == first_addr) {	    /* Flash space is blank */
		prtf ("\n Flash space is blank");
	} else {
		size = size - FLASH_ADDR;
		prtf ("\n Flash is programmed between 0x%X and 0x%X",
							first_addr, last_addr );
		prtf ("\n Size is 0x%x", size );
	}
}

/************************************************/
/*	CHECK DOWNLOAD				*/
/************************************************/

check_download(nsecs)
int nsecs;
{
int i;
int have_flash_section;

	have_flash_section = FALSE;

	for (i=0; i<nsecs; i++) {

		/* Assume section's in RAM until proven otherwise */
		downtype[i] = RAM;

		if ((sectbuf[i].data_ptr != 0) 
		&&  (sectbuf[i].p_addr >= FLASH_ADDR)
		&&  (sectbuf[i].p_addr < (FLASH_ADDR + 0x100000))) {

			/* Not a no-load section, and within the maximum
			 * possible Flash boundaries. Now check the upper 
			 * bound of actually-installed Flash.
			 */

			if (flsize == 0) { 
				/* Flash not programmable */
				cancel_download();
				prtf ("\n Flash not present in programmable address space");
				return ERROR;
			}

			if (sectbuf[i].p_addr < (FLASH_ADDR + flsize)) {
				downtype[i] = FLASH;
				have_flash_section = TRUE;
			}
		}
	}

	if ( have_flash_section && !noerase) {
		return erase_flash();
	}
	return 0;
}
