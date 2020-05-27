/*
 * Memtest.c - this file performs an address/address bar memory test.
 *
 * Written 8/6/90 by Greg Ames.
 * Modified 9/19/90 by Greg Ames.
 *  1.  Moved local, static functions to the front of the file,
 *	to eliminate the need to declare the functions' return
 *	types.
 *  2.  Changed the interface to memTest and removed the prompting
 *	for memory size.  Caller must now pass the start and end
 *	addresses for the test.  This was done to make the test
 *	more general so it could be used to test the VME bus.
 *  3.  Cleaned up comments a bit.
 *
 * Version: @(#)memtest.c	1.5 9/6/90
 */


extern int quadtest();
extern void asm_atadd();
extern void hex32out (unsigned long num);
extern void print (char *string);
extern unsigned long read_long();

#define FAILED          1
#define PASSED          0

/* Do walking one's test */
int
onesTest(
	 long testAddr			/* address to test */
	 )
{
	long	testData = 1;		/* Current pattern being used */
	long	dataRead;
	int	fail = 0;		/* Test hasn't failed yet */
	int	loopCount = 0;		/* To keep track of when to print CR */

	print("\n");

	while(testData && !fail) {	/* Loop until bit shifted out */
		*((long *) testAddr) = testData;	 /* Write test data */
		*((long *) (testAddr + 4)) = 0xFFFFFFFF; /* Drive d0-d31 hi */
		dataRead = *((long *) testAddr);	 /* Read back data */

		hex32out(dataRead);
		if (!(++loopCount % 8) && (loopCount != 32))
			print("\n");
		else
			print(" ");

		if (dataRead != testData) /* Verify data */
			return FAILED;	  /* Signal failure */
		else
			testData <<= 1;	  /* Shift data over one bit */
	}

	return PASSED;
}

/* Do long word address test */

int
LWAddr (
	long	start,			/* Starting address of test */
	long	end,			/* Ending address */
	long	*badAddr,		/* Failure address */
	int	useRMW			/* True to do read-modify-write test */
	)
{
	register long	currentAddr;	/* Current address being tested */
	char	fail = 0;		/* Test hasn't failed yet */

	if (useRMW)
	    for(currentAddr = start; currentAddr < end; currentAddr += 4)
		*((long *) currentAddr) = currentAddr - 1;
	else
	    for(currentAddr = start; currentAddr < end; currentAddr += 4)
		*((long *) currentAddr) = currentAddr;

	if (useRMW)
	    for(currentAddr = start; currentAddr < end; currentAddr += 4)
            asm_atadd(currentAddr, 1);
	for (currentAddr = start;
	     (currentAddr < end) && (!fail);
	     currentAddr += 4)
	{
	    if (*((long *) currentAddr) != currentAddr)
		fail = 1;
	}

	if (fail) {
	    *badAddr = currentAddr - 4;
	    return FAILED;
	} else
	    return PASSED;
}

/* Do inverse long word address test */

int
LWBar (long	start,			/* Starting address of test */
       long	end,			/* Ending address */
       long	*badAddr,		/* Failure address */
       int	useRMW)			/* True to do read-modify-write test */
{
	register long	currentAddr;	/* Current address being tested */
	int	fail = 0;		/* Test hasn't failed yet */

	if (useRMW)
	    for(currentAddr = start; currentAddr < end; currentAddr += 4)
		*((long *) currentAddr) = ~currentAddr - 1;
	else
	    for(currentAddr = start; currentAddr < end; currentAddr += 4)
		*((long *) currentAddr) = ~currentAddr;

	if (useRMW)
	    for(currentAddr = start; currentAddr < end; currentAddr += 4)
            asm_atadd(currentAddr, 1);
	for (currentAddr = start;
	     (currentAddr < end) && (!fail); 
	     currentAddr += 4)
	{
	    if (*((long *) currentAddr) != ~currentAddr)
		fail = 1;
	}
	if (fail) {
	    *badAddr = currentAddr - 4;
	    return FAILED;
	} else
	    return PASSED;
}

/* Do short address test */

int
ShortAddr (
	  long	start,				/* Starting address of test */
	  long	end,				/* Ending address */
	  long	*badAddr			/* Failure address */
	  )
{
	long	currentAddr;	/* Current address being tested */
	int	fail = 0;		/* Test hasn't failed yet */

	for(currentAddr = start; currentAddr < end; currentAddr += 2)
		*((short *) currentAddr) = (short) currentAddr;

	for(currentAddr = start; (currentAddr < end) && (!fail); currentAddr += 2)
		if (*((short *) currentAddr) != (short) currentAddr)
			fail = 1;

	if (fail) {
		*badAddr = currentAddr - 1;
		return FAILED;
	} else
		return PASSED;
}

/* Do inverse short address test */

int
ShortBar (
	 long	start,				/* Starting address of test */
	 long	end,				/* Ending address */
	 long	*badAddr			/* Failure address */
	 )
{
	long	currentAddr;		/* Current address being tested */
	int	fail = 0;		/* Test hasn't failed yet */

	for(currentAddr = start; currentAddr < end; currentAddr += 2)
		*((short *) currentAddr) = (short) ~currentAddr;

	for(currentAddr = start; (currentAddr < end) && (!fail); currentAddr += 2)
		if (*((short *) currentAddr) != (short) ~currentAddr)
			fail = 1;
	if (fail) {
		*badAddr = currentAddr - 1;
		return FAILED;
	} else
		return PASSED;
}

/* Do byte address test */

int
ByteAddr (
	  long	start,				/* Starting address of test */
	  long	end,				/* Ending address */
	  long	*badAddr			/* Failure address */
	  )
{
	long	currentAddr;	/* Current address being tested */
	int	fail = 0;		/* Test hasn't failed yet */

	for(currentAddr = start; currentAddr < end; currentAddr++)
		*((char *) currentAddr) = (char) currentAddr;

	for(currentAddr = start; (currentAddr < end) && (!fail); currentAddr++)
		if (*((char *) currentAddr) != (char) currentAddr)
			fail = 1;

	if (fail) {
		*badAddr = currentAddr - 1;
		return FAILED;
	} else
		return PASSED;
}

/* Do inverse byte address test */

int
ByteBar (
	 long	start,				/* Starting address of test */
	 long	end,				/* Ending address */
	 long	*badAddr			/* Failure address */
	 )
{
	long	currentAddr;		/* Current address being tested */
	int	fail = 0;		/* Test hasn't failed yet */

	for(currentAddr = start; currentAddr < end; currentAddr++)
		*((char *) currentAddr) = (char) ~currentAddr;

	for(currentAddr = start; (currentAddr < end) && (!fail); currentAddr++)
		if (*((char *) currentAddr) != (char) ~currentAddr)
			fail = 1;
	if (fail) {
		*badAddr = currentAddr - 1;
		return FAILED;
	} else
		return PASSED;
}

/*
 * This routine is called if one of the memory tests fails.  It dumps
 * the 8 32-bit words before and the 8 after the failure address
 *
 * Use read_long() because it could be an Sx trying to read longwords across
 * the PCI bus.
 */

void
dumpMem (
	 long	badAddr			/* Failure address */
	 )
{
	print("\n");			/* Print out first line of mem dump */
	hex32out(badAddr - 32);		/* Starting address */
 	print(": ");
	hex32out(read_long((long *) (badAddr - 32)));	/* First longword */
 	print(" ");
	hex32out(read_long((long *) (badAddr - 28)));
 	print(" ");
	hex32out(read_long((long *) (badAddr - 24)));
 	print(" ");
	hex32out(read_long((long *) (badAddr - 20)));

 	print("\n");
	hex32out(badAddr - 16);	
 	print(": ");
	hex32out(read_long((long *) (badAddr - 16)));
 	print(" ");
	hex32out(read_long((long *) (badAddr - 12)));
 	print(" ");
	hex32out(read_long((long *) (badAddr - 8)));
 	print(" ");
	hex32out(read_long((long *) (badAddr - 4)));

	print("\n");		/* Print out contents of fault addr */
	hex32out(badAddr);
 	print(": ");
	hex32out(read_long((long *) badAddr));


	print("\n");		/* Print out next line of mem dump */
	hex32out(badAddr + 4);		/* Starting address */
 	print(": ");
	hex32out(read_long((long *) (badAddr + 4)));	/* First longword */
 	print(" ");
	hex32out(read_long((long *) (badAddr + 8)));
 	print(" ");
	hex32out(read_long((long *) (badAddr + 12)));
 	print(" ");
	hex32out(read_long((long *) (badAddr + 16)));

 	print("\n");
	hex32out(badAddr + 20);	
 	print(": ");
	hex32out(read_long((long *) (badAddr + 20)));
 	print(" ");
	hex32out(read_long((long *) (badAddr + 24)));
 	print(" ");
	hex32out(read_long((long *) (badAddr + 28)));
 	print(" ");
	hex32out(read_long((long *) (badAddr + 32)));

}

/*
 * Returns 1 if passed, 0 if failed.
 */

int
memTest (
	 long	startAddr,		/* Start address of test */
	 long	endAddr,		/* End address + 1 */
	 int	useRMW			/* True to do read-modify-write test */
	 )
{
	long	badAddr;		/* Addr test failed at */
	static const long	quad_test_val[4] =
		{0x12345678, 0x87654321, 0x11112222, 0x33334444};
	int i;

	if (!useRMW)
	{
	    print("\n");
	    if (onesTest(startAddr) == FAILED)
	    {
		print("\nWalking 1's test: failed");
		return 0;
	    }
	    print("\nWalking 1's test: passed");

	    print("\n\nWriting quadword value: ");
	    for (i = 0; i < 4; i++) {
		hex32out(quad_test_val[i]);
		print(" ");
	    }
	    if (quadtest(startAddr, quad_test_val) == FAILED)
	    {
	        print("\nQuadword test failed\n");
	        dumpMem(startAddr);
		return 0;
	    }
	    print("\nQuadword test passed\n");
	}


	print("\nLong word address test: ");
	if (LWAddr(startAddr, endAddr, &badAddr, useRMW) == FAILED) {
		print("failed");
		dumpMem(badAddr);
		return 0;
	}
	print("passed");

	print("\nLong word address bar test: ");
	if (LWBar(startAddr, endAddr, &badAddr, useRMW) == FAILED) {
		print("failed");
		dumpMem(badAddr);
		return 0;
	}
	print("passed");

	if (!useRMW) {
		print("\nByte address test: ");
		if (ByteAddr(startAddr, endAddr, &badAddr) == FAILED) {
			print("failed");
			dumpMem(badAddr);
			return 0;
		}
		print("passed");

		print("\nByte address bar test: ");
		if (ByteBar(startAddr, endAddr, &badAddr) == FAILED) {
			print("failed");
			dumpMem(badAddr);
			return 0;
		}
		print("passed");
	}
	return 1;
}
