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

/************************************************/
/*  DMA						*/
/* 						*/
/* This initiates a DMA transfer		*/
/************************************************/
dma (target_addr, request_addr, bytes, channel)
unsigned int target_addr;
unsigned int request_addr;
unsigned int bytes;
unsigned int channel;
{
unsigned char t_byt1, t_byt2, t_byt3, t_byt4;
unsigned char r_byt1, r_byt2, r_byt3, r_byt4;
unsigned char b_byt1, b_byt2, b_byt3;

switch (channel) {
    case 0:	
	t_byt1 = target_addr & 0xff;
	t_byt2 = (target_addr & 0xff00) >> 8;
	t_byt3 = (target_addr & 0xff0000) >> 16;
	t_byt4 = (target_addr & 0xff000000) >> 24;
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (t_byt1, TA0_0);
	store_byte (t_byt2, TA0_0);
	store_byte (t_byt3, TA0_1);
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (t_byt4, TA0_2);

	b_byt1 = bytes & 0xff;
	b_byt2 = (bytes & 0xff00) >> 8;
	b_byt3 = (bytes & 0xff0000) >> 16;
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (b_byt1, BC0_0);
	store_byte (b_byt2, BC0_0);
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (b_byt3, BC0_1);

	r_byt1 = request_addr & 0xff;
	r_byt2 = (request_addr & 0xff00) >> 8;
	r_byt3 = (request_addr & 0xff0000) >> 16;
	r_byt4 = (request_addr & 0xff000000) >> 24;
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (r_byt1, RA0_0);
	store_byte (r_byt2, RA0_0);
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (r_byt3, RA0_1);
	store_byte (r_byt4, RA0_1);

	store_byte (0xff, CTCIR); /* clear TC interrupt request */
	store_byte (0x0, MSRR_0); /* unmask */
	store_byte (0x4, SRR_0);  /* assert request channel 0 */
	break;

    case 1:
	t_byt1 = target_addr & 0xff;
	t_byt2 = (target_addr & 0xff00) >> 8;
	t_byt3 = (target_addr & 0xff0000) >> 16;
	t_byt4 = (target_addr & 0xff000000) >> 24;
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (t_byt1, TA1_0);
	store_byte (t_byt2, TA1_0);
	store_byte (t_byt3, TA1_1);
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (t_byt4, TA1_2);

	b_byt1 = bytes & 0xff;
	b_byt2 = (bytes & 0xff00) >> 8;
	b_byt3 = (bytes & 0xff0000) >> 16;
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (b_byt2, BC1_0);
	store_byte (b_byt1, BC1_0);
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (b_byt3, BC1_1);

	r_byt1 = request_addr & 0xff;
	r_byt2 = (request_addr & 0xff00) >> 8;
	r_byt3 = (request_addr & 0xff0000) >> 16;
	r_byt4 = (request_addr & 0xff000000) >> 24;
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (r_byt2, RA1_0);
	store_byte (r_byt1, RA1_0);
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (r_byt4, RA1_1);
	store_byte (r_byt3, RA1_1);

	store_byte (0xff, CTCIR); /* clear TC interrupt request */
	store_byte (0x1, MSRR_0); /* unmask */
	store_byte (0x5, SRR_0); /* assert request channel 1 */
	break;

    case 2:
	t_byt1 = target_addr & 0xff;
	t_byt2 = (target_addr & 0xff00) >> 8;
	t_byt3 = (target_addr & 0xff0000) >> 16;
	t_byt4 = (target_addr & 0xff000000) >> 24;
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (t_byt1, TA2_0);
	store_byte (t_byt2, TA2_0);
	store_byte (t_byt3, TA2_1);
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (t_byt4, TA2_2);

	b_byt1 = bytes & 0xff;
	b_byt2 = (bytes & 0xff00) >> 8;
	b_byt3 = (bytes & 0xff0000) >> 16;
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (b_byt2, BC2_0);
	store_byte (b_byt1, BC2_0);
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (b_byt3, BC2_1);

	r_byt1 = request_addr & 0xff;
	r_byt2 = (request_addr & 0xff00) >> 8;
	r_byt3 = (request_addr & 0xff0000) >> 16;
	r_byt4 = (request_addr & 0xff000000) >> 24;
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (r_byt2, RA2_0);
	store_byte (r_byt1, RA2_0);
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (r_byt4, RA2_1);
	store_byte (r_byt3, RA2_1);

	store_byte (0xff, CTCIR); /* clear TC interrupt request */
	store_byte (0x2, MSRR_0); /* unmask */
	store_byte (0x6, SRR_0); /* assert request channel 2 */
	break;

    case 3:
	t_byt1 = target_addr & 0xff;
	t_byt2 = (target_addr & 0xff00) >> 8;
	t_byt3 = (target_addr & 0xff0000) >> 16;
	t_byt4 = (target_addr & 0xff000000) >> 24;
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (t_byt1, TA3_0);
	store_byte (t_byt2, TA3_0);
	store_byte (t_byt3, TA3_1);
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (t_byt4, TA3_2);

	b_byt1 = bytes & 0xff;
	b_byt2 = (bytes & 0xff00) >> 8;
	b_byt3 = (bytes & 0xff0000) >> 16;
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (b_byt2, BC3_0);
	store_byte (b_byt1, BC3_0);
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (b_byt3, BC3_1);

	r_byt1 = request_addr & 0xff;
	r_byt2 = (request_addr & 0xff00) >> 8;
	r_byt3 = (request_addr & 0xff0000) >> 16;
	r_byt4 = (request_addr & 0xff000000) >> 24;
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (r_byt2, RA3_0);
	store_byte (r_byt1, RA3_0);
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (r_byt4, RA3_1);
	store_byte (r_byt3, RA3_1);

	store_byte (0xff, CTCIR); /* clear TC interrupt request */
	store_byte (0x3, MSRR_0); /* unmask */
	store_byte (0x7, SRR_0); /* assert request channel 3 */
	break;

    case 5:
	t_byt1 = target_addr & 0xff;
	t_byt2 = (target_addr & 0xff00) >> 8;
	t_byt3 = (target_addr & 0xff0000) >> 16;
	t_byt4 = (target_addr & 0xff000000) >> 24;
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (t_byt1, TA5_0);
	store_byte (t_byt2, TA5_0);
	store_byte (t_byt3, TA5_1);
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (t_byt4, TA5_2);

	b_byt1 = bytes & 0xff;
	b_byt2 = (bytes & 0xff00) >> 8;
	b_byt3 = (bytes & 0xff0000) >> 16;
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (b_byt2, BC5_0);
	store_byte (b_byt1, BC5_0);
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (b_byt3, BC5_1);

	r_byt1 = request_addr & 0xff;
	r_byt2 = (request_addr & 0xff00) >> 8;
	r_byt3 = (request_addr & 0xff0000) >> 16;
	r_byt4 = (request_addr & 0xff000000) >> 24;
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (r_byt2, RA5_0);
	store_byte (r_byt1, RA5_0);
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (r_byt4, RA5_1);
	store_byte (r_byt3, RA5_1);

	store_byte (0xff, CTCIR); /* clear TC interrupt request */
	store_byte (0x1, MSRR_1); /* unmask */
	store_byte (0x5, SRR_1); /* assert request channel 5 */
	break;

    case 6:
	t_byt1 = target_addr & 0xff;
	t_byt2 = (target_addr & 0xff00) >> 8;
	t_byt3 = (target_addr & 0xff0000) >> 16;
	t_byt4 = (target_addr & 0xff000000) >> 24;
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (t_byt1, TA6_0);
	store_byte (t_byt2, TA6_0);
	store_byte (t_byt3, TA6_1);
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (t_byt4, TA6_2);

	b_byt1 = bytes & 0xff;
	b_byt2 = (bytes & 0xff00) >> 8;
	b_byt3 = (bytes & 0xff0000) >> 16;
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (b_byt2, BC6_0);
	store_byte (b_byt1, BC6_0);
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (b_byt3, BC6_1);

	r_byt1 = request_addr & 0xff;
	r_byt2 = (request_addr & 0xff00) >> 8;
	r_byt3 = (request_addr & 0xff0000) >> 16;
	r_byt4 = (request_addr & 0xff000000) >> 24;
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (r_byt2, RA6_0);
	store_byte (r_byt1, RA6_0);
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (r_byt4, RA6_1);
	store_byte (r_byt3, RA6_1);

	store_byte (0xff, CTCIR); /* clear TC interrupt request */
	store_byte (0x2, MSRR_1); /* unmask */
	store_byte (0x6, SRR_1); /* assert request channel 6 */
	break;

    case 7:
	t_byt1 = target_addr & 0xff;
	t_byt2 = (target_addr & 0xff00) >> 8;
	t_byt3 = (target_addr & 0xff0000) >> 16;
	t_byt4 = (target_addr & 0xff000000) >> 24;
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (t_byt1, TA7_0);
	store_byte (t_byt2, TA7_0);
	store_byte (t_byt3, TA7_1);
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (t_byt4, TA7_2);

	b_byt1 = bytes & 0xff;
	b_byt2 = (bytes & 0xff00) >> 8;
	b_byt3 = (bytes & 0xff0000) >> 16;
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (b_byt2, BC7_0);
	store_byte (b_byt1, BC7_0);
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (b_byt3, BC7_1);

	r_byt1 = request_addr & 0xff;
	r_byt2 = (request_addr & 0xff00) >> 8;
	r_byt3 = (request_addr & 0xff0000) >> 16;
	r_byt4 = (request_addr & 0xff000000) >> 24;
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (r_byt2, RA7_0);
	store_byte (r_byt1, RA7_0);
	store_byte (0xff, CBPFF);   	/* clear byte pointer ff */
	store_byte (r_byt4, RA7_1);
	store_byte (r_byt3, RA7_1);

	store_byte (0xff, CTCIR); /* clear TC interrupt request */
	store_byte (0x3, MSRR_1); /* unmask */
	store_byte (0x7, SRR_1);  /* assert request channel 7 */
	break;

    default:
	break;
    }
}

/************************************************/
/*  Initialize the DMA				*/
/* 						*/
/************************************************/
init_dma ()
{
	store_byte (0xf, CR1_0);
	store_byte (0xf, CR1_1); /* fixed priority, disable channels */

	store_byte (0xc, CR2_0); /* no DREQn sampling, async EOP#, */
	store_byte (0xc, CR2_1); /* channel 0 and 4 highest priority */

/* channel 0 */
	store_byte (0x50, BSR_0); /* 32 bit src & dst bus, ch 0 */
	store_byte (0x0, CHR_0); /* disable chaining, ch 0 */
	store_byte (0x0, SRR_0); /* remove request channel 0 */
	store_byte (0x84, MR1_0); /* block write transfer, targ incr */
				 /* auto-init disabled, ch 0 */
	store_byte (0x80, MR2_0); /* 2 cycle transfer, mem src & targ */
				 /* src & targ incr, ch 0 */

/* channel 1 */
	store_byte (0x51, BSR_0); /* 32 bit src & dst bus, ch 1 */
	store_byte (0x1, CHR_0); /* disable chaining, ch 1 */
	store_byte (0x1, SRR_0); /* remove request channel 1 */
	store_byte (0x85, MR1_0); /* block write transfer, targ incr */
				 /* auto-init disabled, ch 1 */
	store_byte (0x81, MR2_0); /* 2 cycle transfer, mem src & targ */
				 /* src & targ incr, ch 1 */

/* channel 2 */
	store_byte (0x52, BSR_0); /* 32 bit src & dst bus, ch 2 */
	store_byte (0x2, CHR_0); /* disable chaining, ch 2 */
	store_byte (0x2, SRR_0); /* remove request channel 2 */
	store_byte (0x86, MR1_0); /* block write transfer, targ incr */
				 /* auto-init disabled, ch 2 */
	store_byte (0x82, MR2_0); /* 2 cycle transfer, mem src & targ */
				 /* src & targ incr, ch 2 */

/* channel 3 */
	store_byte (0x3, CHR_0); /* disable chaining, ch 3 */
	store_byte (0x53, BSR_0); /* 32 bit src & dst bus, ch 3 */
	store_byte (0x3, SRR_0); /* remove request channel 3 */
	store_byte (0x87, MR1_0); /* block write transfer, targ incr */
				 /* auto-init disabled, ch 3 */
	store_byte (0x83, MR2_0); /* 2 cycle transfer, mem src & targ */
				 /* src & targ incr, ch 3 */

/* channel 5 */
	store_byte (0x51, BSR_1); /* 32 bit src & dst bus, ch 5 */
	store_byte (0x1, CHR_1); /* disable chaining, ch 5 */
	store_byte (0x1, SRR_1); /* remove request channel 5 */
	store_byte (0x85, MR1_1); /* block write transfer, targ incr */
				 /* auto-init disabled, ch 5 */
	store_byte (0x81, MR2_1); /* 2 cycle transfer, mem src & targ */
				 /* src & targ incr, ch 5 */

/* channel 6 */
	store_byte (0x52, BSR_1); /* 32 bit src & dst bus, ch 6 */
	store_byte (0x2, CHR_1); /* disable chaining, ch 6 */
	store_byte (0x2, SRR_1); /* remove request channel 6 */
	store_byte (0x86, MR1_1); /* block write transfer, targ incr */
				 /* auto-init disabled, ch 6 */
	store_byte (0x82, MR2_1); /* 2 cycle transfer, mem src & targ */
				 /* src & targ incr, ch 6 */

/* channel 7 */
	store_byte (0x53, BSR_1); /* 32 bit src & dst bus, ch 7 */
	store_byte (0x3, CHR_1); /* disable chaining, ch 7 */
	store_byte (0x3, SRR_1); /* remove request channel 7 */
	store_byte (0x87, MR1_1); /* block write transfer, targ incr */
				 /* auto-init disabled, ch 7 */
	store_byte (0x83, MR2_1); /* 2 cycle transfer, mem src & targ */
				 /* src & targ incr, ch 7 */

	store_byte (0x0, CR1_0);
	store_byte (0x0, CR1_1); /* fixed priority, enable channels */
}
