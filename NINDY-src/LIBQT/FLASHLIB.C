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
#include "qtcommon.h"

#define ERROR -1

/********************************************************/
/* INIT FLASH    				  	*/
/*               				  	*/
/* This routine initializes the variables for timing	*/
/* with any board configuration.  This is used to get   */
/* exact timing every time. The size of the flash chips */
/* is returned.  Or -1 if flash not programmable.       */
/********************************************************/
init_flash()
{
unsigned char *check;

	/* set up timing loop numbers */
	numloops6 = flash_overhead(TIMEOUT_6);	
	numloops100 = flash_overhead(TIMEOUT_100);	
	numloops10 = flash_overhead(TIMEOUT_10);	

	check = (unsigned char *)FLASH_ADDR;
	*check = 0x90;

	/* find out if devices are really Flash */
	if (*check != 0x89) {
		*check = 0xff;
		*check = 0xff;
		*check = 0x0;
		return (ERROR);
	}

	/* find out which devices are present */
	check = (unsigned char *)(FLASH_ADDR + 4);
	if ((*check == 0xB1) || (*check == 0xB2))  {
		*check = 0xff;
		*check = 0xff;
		*check = 0x0;
		return (256); 	/* 256 Kbit flash */
	}
	if (*check == 0xB9) {
		*check = 0xff;
		*check = 0xff;
		*check = 0x0;
		return (257); 	/* 256 Kbit flash new process */
	}
	if (*check == 0xB8) {
		*check = 0xff;
		*check = 0xff;
		*check = 0x0;
		return (512); 	/* 512 Kbit flash */
	}
	if (*check == 0xB4) {
		*check = 0xff;
		*check = 0xff;
		*check = 0x0;
		return (1024); 	/* 1 Mbit flash */
	}
	if (*check == 0xBD) {
		*check = 0xff;
		*check = 0xff;
		*check = 0x0;
		return (2048); 	/* 2 Mbit flash */
	}
}

/********************************************************/
/* FLASH OVERHEAD      				  	*/
/*               				  	*/
/* This routine returns the number of loops needed to 	*/
/* send to time() to get the number of uS desired.  The	*/
/* ticks are from the 82380 timer 0.			*/
/********************************************************/
flash_overhead(uS)
int uS;
{
unsigned int tot_time, LSB, MSB;
int testloops, loops, trueuS;
float board_ratio, tot_needed;

	/* estimate the number of loops needed 
	 * this is done because the overhead involved is 
	 * non-negligible for the lower numbers.  For numbers
	 * above about 1000 uS the board_ratio is a constant.
	 * The timer flips over making calculations more difficult
	 * above about 7000 uS.  This is why the 5000 number is 
	 * used for the estimation and timing, but the true number
	 * is used for the final calculation. */
	trueuS = uS;
	if (uS > 5000) {
		uS = 5000;
	}
	testloops = 4 * uS;

	/* init timer */
	store_byte (0x30, CWR1);   /* LSB,MSB,16-bit binary,timer 0 */
	store_byte (0xff, CR0);    /* LSB initial count */
	store_quick_byte (0xff, CR0);    /* MSB initial count */

	time(testloops);

	/* latch and read timer */
	store_quick_byte (0x00, CWR1);    /* latch timer */
	LSB = load_byte (CR0);
	MSB = load_byte (CR0);

	/* figure out total number of ticks */
	tot_time = (MSB << 8) + LSB;
	tot_time = 0xffff - tot_time;

	/* figure out number of ticks needed */
	tot_needed = trueuS / CRYSTALTIME;

	/* determine from the board ticks-per-loop how many loops
	   	are really needed */
	board_ratio = (float)tot_time / (float)testloops;
	loops = (float)(tot_needed / board_ratio);

	return (loops);
}

/********************************************************/
/* ERASE FLASH  	  	 			*/
/*               					*/
/* returns -1 if error writing occurs otherwise 0 	*/
/*               				  	*/
/* Flash Programming Algorithm 				*/
/* Copyright (c) 1988, Intel Corporation		*/
/* All Rights Reserved                 			*/
/********************************************************/
erase_flash()
{
unsigned int	start_addr, *erase_data, end_addr;
int	error, erase_cmd, verify_cmd,i;
int  	cumtime, etime, size;
int  	ERRNUM, type;

	start_addr = FLASH_ADDR;
	type = init_flash();

	if (type == ERROR)
		return (ERROR);

	/* set up number of errors permissible for flash type */
	if (type == 256)  {
		ERRNUM = 64;
		size = 0x20000;
	}
	else {
		ERRNUM = 1000;
		if (type == 257)
			size = 0x20000;
		if (type == 512)
			size = 0x40000;
		if (type == 1024)
			size = 0x80000;
		if (type == 2048)
			size = 0x100000;
	}

	end_addr = start_addr + size;

	etime = TIMEOUT_10000; 		/* 10 mS */
	bigloops = flash_overhead(etime);

	cumtime = 0;			/* cumulative erase time */
	error = 0;			/* no cumulative errors */
	erase_cmd = 0x20202020;		/* erase command */
	verify_cmd = 0xa0a0a0a0;	/* verify command */

	/* program device to all zeros */
	if (program_zero(start_addr, type) == ERROR) {
		return (ERROR);
	}

	erase_data = (unsigned int *)start_addr;
	do {
		*erase_data = erase_cmd;    /* set up erase */
		*erase_data = erase_cmd;    /* begin erase */
		time(bigloops);		    /* wait XX mS */
		*erase_data = verify_cmd;   /* end and verify erase */
		time(numloops6);	    /* wait 6 uS */

		/* check for devices being erased */
		while (*erase_data == 0xffffffff) {
			erase_data++;
			if (erase_data >= (unsigned int *)end_addr)
				break;
			*erase_data = verify_cmd; /* verify erase */
			time(numloops6);	  /* wait 6 uS */
		}

		/* if not erased */
		if (erase_data != (unsigned int *)end_addr) {
			erase_cmd = 0x00000000;	/* erase command */

			/* erase only the invalid banks */
			if ((*erase_data & 0x000000ff) != 0x000000ff) {
				erase_cmd |= 0x00000020;
			}
			if ((*erase_data & 0x0000ff00) != 0x0000ff00) {
				erase_cmd |= 0x00002000;
			}
			if ((*erase_data & 0x00ff0000) != 0x00ff0000) {
				erase_cmd |= 0x00200000;
			}
			if ((*erase_data & 0xff000000) != 0xff000000) {
				erase_cmd |= 0x20000000;
			}

			/* special instructions for 256K devices */
			if (type == 256) {
				/* adjust cumulative time */
				cumtime += etime;  

				/* adjust time for check */
				etime = cumtime / 8;
				bigloops = flash_overhead(etime);
			}

			/* check for max errors */
			if (++error == ERRNUM) {
				*erase_data = 0x00000000;
				return (ERROR);
			}
		}

	/* repeat until end of device reached */
	} while (erase_data < (unsigned int *)end_addr);

	erase_data = (unsigned int *)start_addr;
	*erase_data = 0xffffffff;	/* reset */
	*erase_data = 0xffffffff;	/* reset */
	*erase_data = 0x00000000;	/* set for reading */
	return (0);
}

/********************************************************/
/* PROGRAM ZEROS  	 		   	  	*/
/*               				  	*/
/* returns -1 if error writing occurs otherwise 0 	*/
/*               				  	*/
/* Flash Programming Algorithm 				*/
/* Copyright (c) 1988, Intel Corporation		*/
/* All Rights Reserved                 			*/
/********************************************************/
program_zero(start_addr, type)
unsigned int start_addr;
int type;
{
unsigned int 	*write_data;
int i, error, stop_cmd, write_cmd, size, standby;

	/* set size of device to program zeros */
	if (type == 256) {
		standby = numloops100;
		size = 0x20000;
	}
	else {
		standby = numloops10;
		if (type == 257)
			size = 0x20000;
		if (type == 512)
			size = 0x40000;
		if (type == 1024)
			size = 0x80000;
		if (type == 2048)
			size = 0x100000;
	}

	write_data = (unsigned int *)start_addr;
	write_cmd = 0x40404040;
	stop_cmd = 0xc0c0c0c0;
	error = 0;

/* begin writing */
	for (i=0; i<size; i+=4) {
		*write_data = write_cmd;    /* send write cmd */
		*write_data = 0x0;	    /* write data */
		time(standby);	 	    /* standby for program */
		*write_data = stop_cmd;	    /* send stop cmd */
		time(numloops6);	    /* wait 6 uS */
		if (*write_data != 0x0) {   /* verify data */
			if (++error == 25) {
				*write_data = 0xffffffff;
				*write_data = 0xffffffff;
				*write_data = 0x00000000;
				return (ERROR);
			}

			/* mask off valid device */
			if ((*write_data & 0x000000ff) == 0x0) {
				write_cmd &= 0xffffff00;
				stop_cmd &= 0xffffff00;
			}
			if ((*write_data & 0x0000ff00) == 0x0) {
				write_cmd &= 0xffff00ff;
				stop_cmd &= 0xffff00ff;
			}
			if ((*write_data & 0x00ff0000) == 0x0) {
				write_cmd &= 0xff00ffff;
				stop_cmd &= 0xff00ffff;
			}
			if ((*write_data & 0xff000000) == 0x0) {
				write_cmd &= 0x00ffffff;
				stop_cmd &= 0x00ffffff;
			}
			i-=4; /* need to go back and do again */
		}
		else {				/* if correct */
			write_data++; 		/* increment address */
			error = 0;
			write_cmd = 0x40404040;
			stop_cmd = 0xc0c0c0c0;
		}
	}

/* reset for reading */
	write_data-=4; 	/* make sure address is within device range */
	*write_data = 0xffffffff;	/* reset */
	*write_data = 0xffffffff;	/* reset */
	*write_data = 0x00000000;	/* set for reading */

	return (0);
}

/********************************************************/
/* PROGRAM FLASH				  	*/
/*               				  	*/
/* returns -1 if error writing occurs otherwise 0 	*/
/*               				  	*/
/* Flash Programming Algorithm 				*/
/* Copyright (c) 1988, Intel Corporation		*/
/* All Rights Reserved                 			*/
/********************************************************/
program_flash (start_addr, dataptr, bytes)
unsigned int start_addr;
unsigned int *dataptr;
int bytes;
{
unsigned int *write_data, data, mask;
unsigned int write_cmd, stop_cmd, first_addr;
int i, error, extra, done, leftover, standby, type;
union {
	unsigned int *dptr;
	unsigned int dint; 
} dataunion;

	type = init_flash();
	if (type == ERROR)
		return (ERROR);

	/* check device type for standby time */
	if (type == 256)
		standby = numloops100;
	else
		standby = numloops10;

	dataunion.dptr = dataptr;

/* need to make sure the address begins on a word boundry */
/* program bytes until it is there */

	/* find number of bytes done */
	done = start_addr % 4;

	/* find number of extra bytes */
	extra = 4 - done;
	if (done != 0) {
		write_cmd = 0x0;
		stop_cmd = 0x0;
		data = *dataptr;
		mask = 0x0;

		/* get starting address to write extra bytes */
		first_addr = start_addr - done;
		write_data = (unsigned int *)first_addr;

		/* fix start address for rest of bytes */
		start_addr = first_addr + 4;

		/* mask off already written bytes */
		for (i=0; i<extra; i++) {
			write_cmd |= (0x40 << (24-(i*8)));
			stop_cmd |= (0xc0 << (24-(i*8)));
		}
		for (i=0; i<done; i++) {
			data = (data << 8) | 0xff;
			mask = (mask << 8) | 0xff;
		}		

		/* the only way to coerce an int pointer into adding */
		/* only one to its pointer address in C */
		dataunion.dint += extra;
		dataptr = dataunion.dptr;

		bytes -= extra;
		error = 0;
		/* write the word */
		do {
			/* send write cmd */
			*write_data = write_cmd;

			/* write data */
			*write_data = data;

			/* standby for programming */
			time(standby);

			/* send stop cmd */
			*write_data = stop_cmd;

			/* wait 6 uS */
			time(numloops6);

		if ((*write_data | mask) != data) {  /* verify data */
			if (++error == 25) {
				*write_data = 0xffffffff;
				*write_data = 0xffffffff;
				*write_data = 0x00000000;
				return (ERROR);
			}
			/* mask off valid data */
			if ((*write_data & 0x0000ff00)
					==(data & 0x0000ff00)) {
				write_cmd &= 0xffff00ff;
				stop_cmd &= 0xffff00ff;
				data |= 0x0000ff00;
			}
			if ((*write_data & 0x00ff0000)
					==(data & 0x00ff0000)) {
				write_cmd &= 0xff00ffff;
				stop_cmd &= 0xff00ffff;
				data |= 0x00ff0000;
			}
			if ((*write_data & 0xff000000)
					==(data & 0xff000000)) {
				write_cmd &= 0x00ffffff;
				stop_cmd &= 0x00ffffff;
				data |= 0xff000000;
			}
		}
		else 
			error = 0;
		} while (error != 0);
	}

/* write all 4 byte words possible */
	/* find out how many leftover bytes must be written at end */
	leftover = bytes % 4;
	bytes -= leftover;

	write_data = (unsigned int *)start_addr;
	write_cmd = 0x40404040;
	stop_cmd = 0xc0c0c0c0;
	error = 0;

	for (i=0; i<bytes; i+=4) {
		*write_data = write_cmd;	/* send write cmd */
		*write_data = *dataptr;		/* write data */
		time(standby);	 	/* standby for program */
		*write_data = stop_cmd;		/* send stop cmd */
		time(numloops6);		/* wait 6 uS */

		if (*write_data != *dataptr) {  /* verify data */
			if (++error == 25) {
				*write_data = 0xffffffff;
				*write_data = 0xffffffff;
				*write_data = 0x00000000;
				return (ERROR);
			}

			/* mask off valid device */
			if ((*write_data & 0x000000ff)
					==(*dataptr & 0x000000ff)) {
				write_cmd &= 0xffffff00;
				stop_cmd &= 0xffffff00;
				*dataptr |= 0x000000ff;
			}
			if ((*write_data & 0x0000ff00)
					==(*dataptr & 0x0000ff00)) {
				write_cmd &= 0xffff00ff;
				stop_cmd &= 0xffff00ff;
				*dataptr |= 0x0000ff00;
			}
			if ((*write_data & 0x00ff0000)
					==(*dataptr & 0x00ff0000)) {
				write_cmd &= 0xff00ffff;
				stop_cmd &= 0xff00ffff;
				*dataptr |= 0x00ff0000;
			}
			if ((*write_data & 0xff000000)
					==(*dataptr & 0xff000000)) {
				write_cmd &= 0x00ffffff;
				stop_cmd &= 0x00ffffff;
				*dataptr |= 0xff000000;
			}
			i-=4; /* need to go back and do again */
		}
		else {				/* if correct */
			write_data++; 		/* increment address */
			dataptr++;		/* and data */
			error = 0;
			write_cmd = 0x40404040;
			stop_cmd = 0xc0c0c0c0;
		}
	}

/* write out leftover bytes */
	if (leftover) {
		write_cmd = 0x0;
		stop_cmd = 0x0;
		data = *dataptr;
		mask = 0x0;
		extra = 4 - leftover;

		/* mask off bytes not being written */
		for (i=0; i<leftover; i++) {
			write_cmd |= (0x40 << (i*8));
			stop_cmd |= (0xc0 << (i*8));
		}		
		for (i=0; i<extra; i++) {
			data |= (0xff << (24-(i*8)) );
			mask |= (0xff << (24-(i*8)) );
		}

		error = 0;
		/* write the word */
		do {
			/* send write cmd */
			*write_data = write_cmd;

			/* write data */
			*write_data = data;

			/* standby for programming */
			time(standby);

			/* send stop cmd */
			*write_data = stop_cmd;

			/* wait 6 uS */
			time(numloops6);

		if ((*write_data | mask) != data) {  /* verify data */
			if (++error == 25) {
				*write_data = 0xffffffff;
				*write_data = 0xffffffff;
				*write_data = 0x00000000;
				return (ERROR);
			}
			/* mask off valid device */
			if ((*write_data & 0x000000ff)
					==(data & 0x000000ff)) {
				write_cmd &= 0xffffff00;
				stop_cmd &= 0xffffff00;
				data |= 0x000000ff;
			}
			if ((*write_data & 0x0000ff00)
					==(data & 0x0000ff00)) {
				write_cmd &= 0xffff00ff;
				stop_cmd &= 0xffff00ff;
				data |= 0x0000ff00;
			}
			if ((*write_data & 0x00ff0000)
					==(data & 0x00ff0000)) {
				write_cmd &= 0xff00ffff;
				stop_cmd &= 0xff00ffff;
				data |= 0x00ff0000;
			}
		}
		else 
			error = 0;
		} while (error != 0);
	}

/* reset for reading */
	*write_data = 0xffffffff;
	*write_data = 0xffffffff;
	*write_data = 0x00000000;
	return (0);
}
