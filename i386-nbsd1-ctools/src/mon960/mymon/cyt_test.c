/* cyt_test.c - Test routines for C145/146 */

/*****************************************************************************
 *
 *	Diagnostic routines for C145/146 hardware - mostly based on
 *		existing code.
 *
 *****************************************************************************/

/*******************************************************************************
*
* MODIFICATION HISTORY:
*
* 19dec95       snc - added LED tests.  fixed change of priority on return
*		      from post tests, made MCON printing in Squall eeprom
*		      test indicate that MCON was a Cx MCON (not Jx or Hx)
*
*/

#include "this_hw.h"
#include "cyt_menu.h"
#include "c145_eep.h"
#include "cyt_intr.h"

extern void hex8out (unsigned char num);
extern void hex32out (unsigned long num);
extern  long hexIn (void);
extern  long decIn();
extern  void print (char *string);

#if CXHXJX_CPU
extern void disable_dcache();
extern void enable_dcache();
#endif

#define	TRUE 1
#define FALSE 0

int memTest (long startAddr, long endAddr, int useRMW);

#ifndef NULL
#define NULL ((void *)0)
#endif

/* EEPROM test #define's */
#define ON_BOARD_EEPROM_TEST_STRING	"CYCLONE MICROSYSTEMS\0"
#define ON_BOARD_EEPROM_TEST_SIZE	80

/* Can't use #defines from XXX_eeprom.h, menu() won't pass arg of 0 */
#define SQUALL			1

#define MOD_SPECIFIC_DATA_SIZE	10	/* Show 10 bytes of mod spec data */

/* Programming info for SQ01 ethernet module */
#define SQ01_MCON		0x00100002
#define SQ01_IRQ0_MODE		IRQ_0_FALLING_EDGE
#define SQ01_IRQ1_MODE		IRQ_1_FALLING_EDGE
#define SQ01_VER_0		'0'
#define SQ01_VER_1		'1'
#define SQ01_REV_LEVEL		0x00
#define CY_ENET_ADDR_BYTE_0	0x00
#define CY_ENET_ADDR_BYTE_1	0x80
#define CY_ENET_ADDR_BYTE_2	0x4d
#define ENET_ADDR_SIZE		6

/* Test Menu Action Routines - declared in this file */
static void memory_tests (void);
static void repeat_mem_test (void);
static void on_board_eeprom_test (void);
static void squall_eeprom_test (int);
static void squall_enet_test (int);
static void led_test (void);


/* Support routines in other files */
void uart_test (void);			/* uart_tst.c */
void cio_test (void);			/* cio_test.c */
void ether_test (volatile unsigned long * ca_addr, /* ether_te.c */
		 volatile unsigned long * port_addr,
		 int which_irq);
void plx_pci_test (void);		/* pci_test.c */

unsigned long get_mask();
void set_mask();
unsigned long change_priority();
void prtf();
char *sgets();

extern strcmp();

/* Test Menu Table */
static MENU_ITEM testMenu[] =
{
{"Memory Tests",			memory_tests,		NULL},
{"Repeating Memory Test",		repeat_mem_test,	NULL},
{"16C550 UART Serial Port Tests", 	uart_test, 		NULL},
{"85C36 CIO Timer Chip Tests", 		cio_test,		NULL},
{"On-board EEPROM Tests",		on_board_eeprom_test,	NULL},
{"Squall EEPROM Tests",			squall_eeprom_test, (void *) SQUALL},
{"Squall Ethernet Tests",		squall_enet_test,   (void *) SQUALL},
{"PLX-9060 PCI Chip Tests",		plx_pci_test,		NULL},
{"LED Tests",                           led_test,               NULL}
};

#define NUM_MENU_ITEMS	(sizeof (testMenu) / sizeof (testMenu[0]))
#define MENU_TITLE	"\n  C145/146 Tests"

void _post_test (void)
{
    static unsigned long old_prio;
#if CXHXJX_CPU
    static unsigned long old_mask;

    old_mask = get_mask();
#endif /* CXHXJX */

    init_intr();
    old_prio = (change_priority(0) & 0x001f0000) >> 16;
    menu (testMenu, NUM_MENU_ITEMS, MENU_TITLE, MENU_OPT_NONE);

#if CXHXJX_CPU
    (void) set_mask(old_mask);	/* Restore imask */
#endif /* CXHXJX */

    (void) change_priority(old_prio);	/* Restore old priority */
    print ("\n");
}


/************************************************/
/* Memory Tests					*/
/************************************************/
static void memory_tests (void)
{
    long	start_addr;
    long	mem_size;
    long	end_addr;


    print ("Base address of memory to test (in hex): ");
    start_addr = hexIn();
    prtf("\n");
    print ("Size of memory to test (in hex): ");
    mem_size = hexIn();
    prtf("\n");
    end_addr = start_addr + mem_size;

    print("Testing memory from $");
    hex32out(start_addr);
    print(" to $");
    hex32out(end_addr);
    print(".\n");

    memTest(start_addr, end_addr, FALSE);
    print("\n");

    print("\nRepeating test using read-modify-write cycles...\n");

    memTest(start_addr, end_addr, TRUE);
    print("\n");

    print ("\nMemory test done.\n");
    print ("Press return to continue.\n");
    (void) hexIn();
}


/************************************************/
/* Repeating Memory Tests		*/
/************************************************/
static void repeat_mem_test (void)
{
    unsigned long start_addr, mem_size, end_addr;

    print ("Base address of memory to test (in hex): ");
    start_addr = hexIn();
    prtf("\n");
    print ("Size of memory to test (in hex): ");
    mem_size = hexIn();
    prtf("\n");
    end_addr = start_addr + mem_size;

    print("Testing memory from $");
    hex32out(start_addr);
    print(" to $");
    hex32out(end_addr);
    print(".\n");

    while (memTest (start_addr, end_addr, FALSE))
	;

}


/************************************************/
/* On Board EEPROM Test				*/
/************************************************/
static void on_board_eeprom_test (void)
{
    char old_data[ON_BOARD_EEPROM_TEST_SIZE];
    char new_data[ON_BOARD_EEPROM_TEST_SIZE];
    int status;

    print ("Attempting to read from on board EEPROM, result: ");
    status = eeprom_read (ON_BOARD_EEPROM, 0, old_data,
			  ON_BOARD_EEPROM_TEST_SIZE);
    switch (status)
      {
      case OK:
	print ("OK\n");
	break;
      case EEPROM_ERROR:
	print ("EEPROM_ERROR\n");
	return;
	break;
      case EEPROM_NOT_RESPONDING:
	print ("EEPROM_NOT_RESPONDING\n");
	return;
	break;
      case EEPROM_TO_SMALL:
	print ("EEPROM_TO_SMALL\n");
	return;
	break;
      default:
	print ("UNKNOWN, $");
	hex8out(status);
	print ("\n");
	return;
	break;
      }

    print ("Writing test string: ");
    print (ON_BOARD_EEPROM_TEST_STRING);
    print (" Result: ");
    status = eeprom_write (ON_BOARD_EEPROM, 0, ON_BOARD_EEPROM_TEST_STRING,
			   sizeof(ON_BOARD_EEPROM_TEST_STRING));
    switch (status)
      {
      case OK:
	print ("OK\n");
	break;
      case EEPROM_ERROR:
	print ("EEPROM_ERROR\n");
	break;
      case EEPROM_NOT_RESPONDING:
	print ("EEPROM_NOT_RESPONDING\n");
	break;
      case EEPROM_TO_SMALL:
	print ("EEPROM_TO_SMALL\n");
	break;
      default:
	print ("UNKNOWN, $");
	hex8out(status);
	print ("\n");
	break;
      }

    print ("Reading test string, result: ");
    status = eeprom_read (ON_BOARD_EEPROM, 0, new_data,
			  sizeof(ON_BOARD_EEPROM_TEST_STRING));
    switch (status)
      {
      case OK:
	print ("OK\n");
	break;
      case EEPROM_ERROR:
	print ("EEPROM_ERROR\n");
	break;
      case EEPROM_NOT_RESPONDING:
	print ("EEPROM_NOT_RESPONDING\n");
	break;
      case EEPROM_TO_SMALL:
	print ("EEPROM_TO_SMALL\n");
	break;
      default:
	print ("UNKNOWN, $");
	hex8out(status);
	print ("\n");
	break;
      }

    print ("Read: ");
    print (new_data);

    if (strcmp (ON_BOARD_EEPROM_TEST_STRING, new_data))
	print (", BAD DATA!\n");
    else
	print (", data OK.\n");

    print ("Attempting to restore old on board EEPROM data, result: ");
    status = eeprom_write (ON_BOARD_EEPROM, 0, old_data,
			   ON_BOARD_EEPROM_TEST_SIZE);
    switch (status)
      {
      case OK:
	print ("OK\n");
	break;
      case EEPROM_ERROR:
	print ("EEPROM_ERROR\n");
	break;
      case EEPROM_NOT_RESPONDING:
	print ("EEPROM_NOT_RESPONDING\n");
	break;
      case EEPROM_TO_SMALL:
	print ("EEPROM_TO_SMALL\n");
	break;
      default:
	print ("UNKNOWN, $");
	hex8out(status);
	print ("\n");
	break;
      }

    print ("\nOn board EEPROM test done.\n");
    print ("Press return to continue.\n");
    (void) hexIn();
}


/************************************************/
/* Squall EEPROM Test				*/
/************************************************/
static void squall_eeprom_test (int which_squall)
{
    SQUALL_EEPROM_DATA squall_data;
    unsigned char mod_spec_data[MOD_SPECIFIC_DATA_SIZE];
    int which_eeprom;
    int i;
    int status;
    char answer[20];
    long tmp;
    long nbytes;

    switch (which_squall)
      {
      case SQUALL:
	which_eeprom = SQUALL_EEPROM;
	break;
      default:
	print ("UNKNOWN SQUALL MODULE.\n");
	return;
      }

    print ("Attempting to read squall eeprom data, result: ");
    status = eeprom_read (which_eeprom, GENERIC_DATA_OFFSET,
			  (void *) &squall_data, GENERIC_DATA_SIZE);
    if (status == OK)
	status = eeprom_read (which_eeprom, MODULE_SPECIFIC_DATA_OFFSET,
			      (void *) mod_spec_data, MOD_SPECIFIC_DATA_SIZE);
    switch (status)
      {
      case OK:
	print ("OK\n");
	break;
      case EEPROM_ERROR:
	print ("EEPROM_ERROR\n");
	return;
	break;
      case EEPROM_NOT_RESPONDING:
	print ("EEPROM_NOT_RESPONDING\n");
	print ("Check to be sure module is correctly installed.\n");
	return;
	break;
      case EEPROM_TO_SMALL:
	print ("EEPROM_TO_SMALL\n");
	return;
	break;
      default:
	print ("UNKNOWN, $");
	hex8out(status);
	print ("\n");
	return;
	break;
      }

    prtf ("Squall CX MCON:    0x%X\n", squall_data.mcon_byte_0 |
				   (squall_data.mcon_byte_1 << 8) |
				   (squall_data.mcon_byte_2 << 16) |
				   (squall_data.mcon_byte_3 << 24));
    prtf ("IRQ0 trigger mode: %s\n",
	    ((squall_data.intr_det_mode & IRQ_0_MASK) == IRQ_0_FALLING_EDGE) ?
	    "Edge Triggered" : "Low Level Triggered");
    prtf ("IRQ1 trigger mode: %s\n",
	    ((squall_data.intr_det_mode & IRQ_1_MASK) == IRQ_1_FALLING_EDGE) ?
	    "Edge Triggered" : "Low Level Triggered");
    prtf ("Squall ID:         %c%c\n", squall_data.version_byte_0,
	    squall_data.version_byte_1);
    prtf ("Revision level:    0x%B\n", squall_data.revision_level);

    prtf ("\nModule-specific Data\n");
    for (i = 0; i < MOD_SPECIFIC_DATA_SIZE; i++)
	prtf ("Byte #%B:          %s0x%B\n", i, ((i<10) ? " " : ""),
	    mod_spec_data[i]);

    prtf ("\nDo you wish to change this programming? ");
#if 0
    scanf ("%s", answer);
#endif
    sgets(answer);

    if ((answer[0] == 'y') || (answer[0] == 'Y'))
	{
	prtf ("\nSquall MCON (in hex): ");
#if 0
	scanf ("%lx", &tmp);
#endif
	tmp = hexIn();
	prtf("\n");
	squall_data.mcon_byte_0 = tmp & 0x0ff;
	squall_data.mcon_byte_1 = (tmp >> 8) & 0x0ff;
	squall_data.mcon_byte_2 = (tmp >> 16) & 0x0ff;
	squall_data.mcon_byte_3 = (tmp >> 24) & 0x0ff;

	do {
	    prtf ("IRQ0 trigger mode (1 = edge, 0 = level low): ");
#if 0
	    scanf ("%d", &tmp);
#endif
	    tmp = decIn();
	    prtf("\n");
	} while ((tmp != 0) && (tmp != 1));
	squall_data.intr_det_mode = ((tmp == 1) ? IRQ_0_FALLING_EDGE : 
				     IRQ_0_LOW_LEVEL);

	do {
	    prtf ("IRQ1 trigger mode (1 = edge, 0 = level low): ");
#if 0
	    scanf ("%d", &tmp);
#endif
	    tmp = decIn();
	    prtf("\n");
	} while ((tmp != 0) && (tmp != 1));
	squall_data.intr_det_mode |= ((tmp == 1) ? IRQ_1_FALLING_EDGE : 
				     IRQ_1_LOW_LEVEL);
	      
	prtf ("Squall ID (2 characters): ");
#if 0
	scanf ("%s", answer);
#endif
	sgets(answer);
	prtf("\n");
	squall_data.version_byte_0 = answer[0];
	squall_data.version_byte_1 = answer[1];

	prtf ("Revision level (in hex): ");
#if 0
	scanf ("%x", &tmp);
#endif
	tmp = hexIn();
	prtf("\n");
	squall_data.revision_level = tmp & 0x0ff;

	squall_data.reserved_0 = 0;
	squall_data.reserved_1 = 0;

	prtf ("Number of bytes of module specific data (max 10): ");
#if 0
	prtf ("Number of bytes of module specific data (max %d): ",
		MOD_SPECIFIC_DATA_SIZE);
	scanf ("%d", &nbytes);
#endif
	nbytes = decIn();
	prtf("\n");

	for (i = 0; i < nbytes; i++) {
	    prtf ("Byte #%B (in hex): ", i);
#if 0
	    scanf ("%x", &tmp);
#endif
	    tmp = hexIn();
	    prtf("\n");
	    mod_spec_data[i] = tmp & 0x0ff;
	}

    print ("\nAttempting to write squall eeprom data, result: ");
    status = eeprom_write (which_eeprom, GENERIC_DATA_OFFSET,
			  (void *) &squall_data, GENERIC_DATA_SIZE);
    if (status == OK)
	status = eeprom_write (which_eeprom, MODULE_SPECIFIC_DATA_OFFSET,
			      (void *) mod_spec_data, MOD_SPECIFIC_DATA_SIZE);
    switch (status)
      {
      case OK:
	print ("OK\n");
	break;
      case EEPROM_ERROR:
	print ("EEPROM_ERROR\n");
	return;
	break;
      case EEPROM_NOT_RESPONDING:
	print ("EEPROM_NOT_RESPONDING\n");
	print ("Check to be sure module is correctly installed.\n");
	return;
	break;
      case EEPROM_TO_SMALL:
	print ("EEPROM_TO_SMALL\n");
	return;
	break;
      default:
	print ("UNKNOWN, $");
	hex8out(status);
	print ("\n");
	return;
	break;
      }

      }

    print ("\nSquall EEPROM test done.\n");
    print ("Press return to continue.\n");
#if 0
    (void) gets(answer);	/* Strip off nl so hexIn waits for input */
    sgets (answer);
#endif
    (void) hexIn();
}

/************************************************/
/* Squall Ethernet Test				*/
/************************************************/
static void squall_enet_test (int which_squall)
{
    int which_eeprom;
    volatile unsigned long *ca_addr;
    volatile unsigned long *port_addr;
    int which_irq;
    long tmp;
    SQUALL_EEPROM_DATA squall_data;
    unsigned char mod_spec_data[MOD_SPECIFIC_DATA_SIZE];
    int status;

    switch (which_squall)
      {
      case SQUALL:
	which_eeprom	= SQUALL_EEPROM;
	ca_addr		= (volatile unsigned long *) (SQUALL_BASE_ADDR +
						      SQ_01_CA_OFFSET);
	port_addr	= (volatile unsigned long *) (SQUALL_BASE_ADDR +
						      SQ_01_PORT_OFFSET);
	which_irq	= IRQ_SQ_IRQ0;
	break;
      default:
	print ("UNKNOWN SQUALL MODULE.\n");
	return;
      }

    prtf ("Squall serial number: ");
#if 0
    scanf ("%d", &tmp);
#endif
    tmp = decIn();
    prtf("\n");
    
    /*  
       First read in the eeprom data, then change the address to reflect
       the serial number entered above. 
    */ 
    status = eeprom_read (which_eeprom,GENERIC_DATA_OFFSET,
       			  (void *)&squall_data,GENERIC_DATA_SIZE);
    if (status == OK)
	status = eeprom_read (which_eeprom, MODULE_SPECIFIC_DATA_OFFSET,
			      (void *) mod_spec_data, MOD_SPECIFIC_DATA_SIZE);
    switch (status)
      {
      case OK:
	print ("OK\n");
	break;
      case EEPROM_ERROR:
	print ("EEPROM_ERROR\n");
	return;
	break;
      case EEPROM_NOT_RESPONDING:
	print ("EEPROM_NOT_RESPONDING\n");
	print ("Check to be sure module is correctly installed.\n");
	return;
	break;
      case EEPROM_TO_SMALL:
	print ("EEPROM_TO_SMALL\n");
	return;
	break;
      default:
	print ("UNKNOWN, $");
	hex8out(status);
	print ("\n");
	return;
	break;
      }

    /*
       The enet address is stored in the module-specific data area.  
       The first three lines insert the manufacturer's 3-byte code, the next
       3 lines add the serial number of the module as the last 3 characters
       of the address.  The module-specific data is then written to eeprom.
    */ 
    mod_spec_data[0]		= CY_ENET_ADDR_BYTE_0 ;
    mod_spec_data[1] 		= CY_ENET_ADDR_BYTE_1 ;
    mod_spec_data[2]		= CY_ENET_ADDR_BYTE_2 ;  
    mod_spec_data[3]		= (tmp >> 16) & 0x0ff;
    mod_spec_data[4]		= (tmp >> 8) & 0x0ff;
    mod_spec_data[5]		= tmp & 0x0ff;
 
    print ("\nAttempting to write squall eeprom data, result: ");
    status = eeprom_write (which_eeprom, GENERIC_DATA_OFFSET,
			  (void *) &squall_data, GENERIC_DATA_SIZE);
    if (status == OK)
	status = eeprom_write (which_eeprom, MODULE_SPECIFIC_DATA_OFFSET,
			      (void *) mod_spec_data, ENET_ADDR_SIZE);
    switch (status)
      {
      case OK:
	print ("OK\n");
	break;
      case EEPROM_ERROR:
	print ("EEPROM_ERROR\n");
	return;
	break;
      case EEPROM_NOT_RESPONDING:
	print ("EEPROM_NOT_RESPONDING\n");
	print ("Check to be sure module is correctly installed.\n");
	return;
	break;
      case EEPROM_TO_SMALL:
	print ("EEPROM_TO_SMALL\n");
	return;
	break;
      default:
	print ("UNKNOWN, $");
	hex8out(status);
	print ("\n");
	return;
	break;
      }

#if CXHXJX_CPU
    /* Disable Data Cache for Ethernet Tests:
       If the memory area which is shared between the processor and the i82596
       Ethernet Controller gets cached, new data placed in the memory by the DMA
       Controller of the i82596 will not be read correctly (the processor will
       see the cached data) */
    disable_dcache();
#endif /*CXHXJX*/
 
    ether_test (ca_addr, port_addr, which_irq);
 
#if CXHXJX_CPU
    enable_dcache();
#endif /*CXHXJX*/

    print ("\nSquall ethernet test done.\n");
    print ("Press return to continue.\n");
    /*(void) gets(mod_spec_data);*/	/* Strip off nl so hexIn waits for input */
    hexIn();
}

static void delay (int msecs)
{
    int i;
    extern void eeprom_delay (int usec);
 
    /* call delay once for each msec */
    for (i=0; i<msecs; i++)
    {
        eeprom_delay(1000);
    }
}

/******************************************************************************
* led_test - Test the LEDs on the baseboard
*
* Note: these LEDs are connected to the CIO port C on Rev. C or later
*       baseboards.
*/
static void led_test (void)
{
    char led_val = 0;
    unsigned char cio_val;
    int i;
    extern int is_new_baseboard(void);
    extern void set_led_value(unsigned char);
    extern unsigned char get_led_value(void);
 
    /* Only test LEDs on a new baseboard */
    if (is_new_baseboard() == FALSE)
    {
        prtf("\nBoard Does Not Support LEDs\n\n");
        return;
    }
 
    /* no timer interrupts, delays are polled */
    CIO->ctrl = 0x00;           /* Master Interrupt Control register */
    cio_val = CIO->ctrl;        /* Read */
    CIO->ctrl = 0x00;           /* Master Interrupt Control register */
    CIO->ctrl = cio_val & 0x7f; /* clear Master Interrupt Enable */
 
    /* clear leds */
    set_led_value(led_val);
 
    /* walking ones test */
    led_val = 0x01;
    while (led_val < 0x10)
    {
        set_led_value(led_val); /* turn on led */
        delay(1000);            /* wait one second */
        led_val <<= 1;          /* next led */
    }
 
    /* an amusing display */
    for (i=0; i<4; i++)
    {
        led_val = 0;
        while (led_val < 0x10)
        {
            set_led_value(led_val);     /* turn on leds */
            delay(100);                 /* wait 1/10 second */
            led_val++;                  /* next value */
        }
    }
    CIO->ctrl = 0x00;           /* Master Interrupt Control register */
    CIO->ctrl = cio_val;        /* restore original value */
 
    return;
}
