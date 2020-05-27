/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1993, 1994 Intel Corporation
 *
 * Intel hereby grants you permission to copy, modify, and distribute this
 * software and its documentation.  Intel grants this permission provided
 * that the above copyright notice appears in all copies and that both the
 * copyright notice and this permission notice appear in supporting
 * documentation.  In addition, Intel grants this permission provided that
 * you prominently mark as "not part of the original" any modifications
 * made to this software or documentation, and that the name of Intel
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software or the documentation without specific,
 * written prior permission.
 *
 * Intel Corporation provides this AS IS, WITHOUT ANY WARRANTY, EXPRESS OR
 * IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY
 * OR FITNESS FOR A PARTICULAR PURPOSE.  Intel makes no guarantee or
 * representations regarding the use of, or the results of the use of,
 * the software and documentation in terms of correctness, accuracy,
 * reliability, currentness, or otherwise; and you rely on the software,
 * documentation and results solely at your own risk.
 *
 * IN NO EVENT SHALL INTEL BE LIABLE FOR ANY LOSS OF USE, LOSS OF BUSINESS,
 * LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES
 * OF ANY KIND.  IN NO EVENT SHALL INTEL'S TOTAL LIABILITY EXCEED THE SUM
 * PAID TO INTEL FOR THE PRODUCT LICENSED HEREUNDER.
 *
 ******************************************************************************/
/*)ce*/

/********************************************************************
  Module Name: EVCAP_M0.C   MEMORY TESTS

  Functions:   u_char *byte_test(u_char *start, u_long size)
               u_short *short_test(u_short *start, u_long size)
               u_long *word_test(u_long *start, u_long size)

    This module contains the procedures necessary to test the
    memory array using STOB/LDOB, STOS/LDOS, and ST/LD instructions.

    The test writes a walking '1' pattern through each memory
    location according the the size of the data being accessed.
    For instance with the STOB/LDOB instructions, the walking '1'
    is sequenced through every eight bytes.  The entire array is
    written and then read and verified, creating a delay between
    the two events.

    Next, a checkerboard and checkerboard bar pattern is written
    to the array.  For each data type, the checkerboard value is
    written, verified, then checkerboard bar is immediately written
    and verified.

    Two macros are used extensively in this code, NUM_DTYPES, and
    SHIFT.  NUM_DTYPES takes the size of the array in bytes and
    determines how many testable units of the current data type
    are contained in that array.  For instance, a 1MB DRAM array
    has 256K 32-bit words.  SHIFT simply returns the shift count
    that the walking '1' must be shifted based on the current
    address being accessed.  If shorts (16-bit values) are being
    written/read and the address being accessed is E0000073H, shift
    will return 3.

    In the Word_Test a walking '1' and walking '0' pattern is written
    across the address lines and unique data is written to each of
    these locations.  Then the data is verified.


********************************************************************/

#include "epcxptst.h"

/*--------------------------------------------------------------------
  Function:  byte_test(u_char *start, u_long size)
  Action:    Verify the memory array using STOB/LDOB instructions.
  Passed:    Starting address of the array and its size.
  Returns:   Address of failing location, or zero if test passes.
--------------------------------------------------------------------*/
u_char *byte_test(start, size)
u_char *start;
u_long size;
{
  long i;

  /* write the walking 1's pattern to the memory array */
  for (i = 0; i < num_dtypes(size, u_char); i++)
    *(start + i) = 1 << shift(i, u_char);

  /* verify the walking 1's pattern just written */
  for (i = 0; i < num_dtypes(size, u_char); i++)
    if ((1 << shift(i, u_char)) != *(start + i))
      return(start + (sizeof(u_char) * i));

  /* write and verify checkerboard/checkerboard# to array */
  for (i = 0; i < num_dtypes(size, u_char); i++)
  {
    *(start + i) = 0x55;
    if (*(start + i) != 0x55)
      return(start + (sizeof(u_char) * i));
    *(start + i) = 0xAA;
    if (*(start + i) != 0xAA)
      return(start + (sizeof(u_char) * i));
  }

  /* if code progresses to this point the test passed - return 0 */
  return(0);
}

/*--------------------------------------------------------------------
  Function:  short_test(u_short *start, u_long size)
  Action:    Verify the memory array using STOS/LDOS instructions.
  Passed:    Starting address of the array and its size.
  Returns:   Address of failing location, or zero if test passes.
--------------------------------------------------------------------*/
u_short *short_test(start, size)
u_short *start;
u_long size;
{
  long i;

  /* write the walking 1's pattern to the memory array */
  for (i = 0; i < num_dtypes(size, u_short); i++)
    *(start + i) = 1 << shift(i, u_short);

  /* verify the walking 1's pattern just written */
  for (i = 0; i < num_dtypes(size, u_short); i++)
    if ((1 << shift(i, u_short)) != *(start + i))
      return(start + (sizeof(u_short) * i));

  /* write and verify checkerboard/checkerboard# to array */
  for (i = 0; i < num_dtypes(size, u_short); i++)
  {
    *(start + i) = 0x5555;
    if (*(start + i) != 0x5555)
      return(start + (sizeof(u_short) * i));
    *(start + i) = 0xAAAA;
    if (*(start + i) != 0xAAAA)
      return(start + (sizeof(u_short) * i));
  }

  /* if code progresses to this point the test passed - return 0 */
  return(0);
}

/*--------------------------------------------------------------------
  Function:  word_test(u_long *start, u_long size)
  Action:    Verify the memory array using ST/LD instructions.
  Passed:    Starting address of the array and its size.
  Returns:   Address of failing location, or zero if test passes.
--------------------------------------------------------------------*/
u_long *word_test(start, size)
u_long *start, size;
{
  long i, temp;

  /* initialize the array */
  for (i = 0; i < num_dtypes(size, u_long); i++)
    *(start + i) = 0xFFFFFFFF;
  *start = 0x11111111;
  temp = num_dtypes(size, u_long) - 1;
  *(start + temp) = 0x88888888;

  /* walk a '1' across the address lines and write a unique value */
  for (i = 0; i < DRAM_SCAN; i++)
  {
      *(start + (1ul << i)) = ((i + 5) << 24) | ((i + 5) << 16) | ((i + 5) << 8) | (i + 5);
      if (*start != 0x11111111)
	  {
      return(start + sizeof(u_long) * i);
	  }
  }

  for (i = 0; i < DRAM_SCAN; i++)
    if (*(start + (1 << i)) != (((i + 5) << 24) | ((i + 5) << 16) | ((i + 5) << 8) | (i + 5)))
	  {
      return(start + (sizeof(u_long) * i));
      }

  /* walk a '0' across the address lines and write a unique value */
  for (i = 0; i < DRAM_SCAN-1; i++)
  {
    *(start + (0x3FFFF ^ (1L << i))) = ((i + 5) << 24) | ((i + 5) << 16) | ((i + 5) << 8) | (i + 5);
    if (*(start + temp) != 0x88888888)
	  {
      return(start + (sizeof(u_long) * i));
	  }
  }

  for (i = 0; i < DRAM_SCAN-1; i++)
    if (*(start + (0x3FFFF ^ (1L << i))) != (((i + 5) << 24) | ((i + 5) << 16) | ((i + 5) << 8) | (i + 5)))
	  {
      return(start + (sizeof(u_long) * i));
	  }

  /* sequentially access each location and write address plus offset */
  for (i = 0; i < num_dtypes(size, u_long); i++)
    *(start + i) = (u_long) ((4 * i) + 15L);
  for (i = 0; i < num_dtypes(size, u_long); i++)
    if ((u_long) ((4 * i) + 15L) != *(start + i))
	  {
      return(start + (sizeof(u_long) * i));
	  }

  /* write the walking 1's pattern to the memory array */
  for (i = 0; i < num_dtypes(size, u_long); i++)
    *(start + i) = 1 << shift(i, u_long);

  /* verify the walking 1's pattern just written */
  for (i = 0; i < num_dtypes(size, u_long); i++)
    if ((1 << shift(i, u_long)) != *(start + i))
	  {
      return(start + (sizeof(u_long) * i));
	  }

  /* write and verify checkerboard/checkerboard# to array */
  for (i = 0; i < num_dtypes(size, u_long); i++)
  {
    *(start + i) = 0x55555555;
    if (*(start + i) != 0x55555555)
	  {
      return(start + (sizeof(u_long) * i));
	  }
    *(start + i) = 0xAAAAAAAA;
    if (*(start + i) != 0xAAAAAAAA)
	  {
      return(start + (sizeof(u_long) * i));
	  }
  }

  /* if code progresses to this point the test passed - return 0 */
  return(0);
}
