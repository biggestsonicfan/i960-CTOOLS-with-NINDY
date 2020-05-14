/* Copyright 1991 Intel Corporation.  All rights reserved.  */
/************************************************************/
/*                                                          */
/*       Intel Corporation Proprietary Information          */
/*                                                          */
/*  This software is supplied under the terms of a license  */
/*  agreement or nondisclosure agreement with Intel Corpo-  */
/*  ration and may not be copied or disclosed except in     */
/*  accordance with the terms of that agreement.            */
/*                                                          */
/************************************************************/
  
#ifndef _afpfaulth
#define _afpfaulth
/*
* afpfault.h - Accelerated Floating-Point library fault handling
* support.
*/

/*
*  The following union is for long double fault handling routines.  The
*  use of this construct allows AFP routines to pass two long double
*  values and an integer code value in the eight global registers normally
*  used for parameter passing.
*  To access the long double reference:   param.f2
*  To access the opcode reference:        param.f1.op
*/

union fild {
	struct { int w1, w2, w3, op; } f1;
	long double                    f2;
};


/*
* Fault handling routine prototypes.
* Note that the routines provided in the AFP library (libh*.a) are only stubs
* and do nothing.  These routines can be replaced in the library by user
* defined routines as long as the interface (as defined below) is used.
*/

float        AFP_Fault_Inexact_S( float result, int opcode);
double       AFP_Fault_Inexact_D( double result, int opcode);
long double  AFP_Fault_Inexact_T( long double result, int opcode);

float        AFP_Fault_Invalid_Operation_S( float src1, float src2, int opcode);
double       AFP_Fault_Invalid_Operation_D( double src1, double src2, int opcode);
long double  AFP_Fault_Invalid_Operation_T( long double src1, union fild src2);

float        AFP_Fault_Overflow_S( float result, int opcode);
double       AFP_Fault_Overflow_D( double result, int opcode);
long double  AFP_Fault_Overflow_T( long double result, int opcode);

float        AFP_Fault_Reserved_Encoding_S( float src1, float src2, int opcode);
double       AFP_Fault_Reserved_Encoding_D( double src1, double src2, int opcode);
long double  AFP_Fault_Reserved_Encoding_T( long double src1, union fild src2);

float        AFP_Fault_Underflow_S( float result, int opcode);
double       AFP_Fault_Underflow_D( double result, int opcode);
long double  AFP_Fault_Underflow_T( long double result, int opcode);

float        AFP_Fault_Zero_Divide_S( float src1, float src2, int opcode);
double       AFP_Fault_Zero_Divide_D( double src1, double src2, int opcode);
long double  AFP_Fault_Zero_Divide_T( long double src1, union fild src2);


/*
* Fault handler opcodes.
*
* Each opcode below corresponds to a particular type of operation as indicated
* in the comment at the end of each line.  These are provided to use in the 
* above routines to interpret the opcode argument.
*/

#define     AFP_FAULT_ADDSUB      1      /* addition/subtraction */
#define     AFP_FAULT_DIVIDE      2      /* division */
#define     AFP_FAULT_MULTIP      3      /* multiplication */
#define     AFP_FAULT_ITOF        4      /* convert Integer to Float */
#define     AFP_FAULT_UITOF       5      /* convert Unsigned Integer TO Float */
#define     AFP_FAULT_LDTOD       6      /* convert Long Double TO Double */
#define     AFP_FAULT_DTOLD       7      /* convert Double TO Long Double */
#define     AFP_FAULT_LDTOF       8      /* convert Long Double TO Float */
#define     AFP_FAULT_FTOLD       9      /* convert Float TO Long Double */
#define     AFP_FAULT_DTOF        10     /* convert Double TO Float */
#define     AFP_FAULT_FTOD        11     /* convert Float TO Double */
#define     AFP_FAULT_COMPARE     12     /* comparison */
#define     AFP_FAULT_SCALE       13     /* scale */
#define     AFP_FAULT_LOGB        14     /* logb */
#define     AFP_FAULT_REM         15     /* 960 remainder ala remr instruction */
#define     AFP_FAULT_RINT        16     /* round to nearest integer */
#define     AFP_FAULT_RMD         17     /* IEEE remainder */
#define     AFP_FAULT_ROUND       18     /* round using current rounding mode */
#define     AFP_FAULT_CEIL        19     /* ceil */
#define     AFP_FAULT_FLOOR       20     /* floor */

#endif

