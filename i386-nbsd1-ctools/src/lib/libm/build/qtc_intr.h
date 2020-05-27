/*******************************************************************************
 * 
 * Copyright (c) 1993 Intel Corporation
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

/*** qtc_intrinsics.h ***/

/*
** One of the compile line switches -DMSH_FIRST or -DLSH_FIRST should
** be specified when compiling this source.
*/

#define FUNCTION
#define TRUE 1
#define FALSE 0
#define ABS(x) ((x) > 0 ? (x) : -(x))

#define SIGN_MASK 0x80000000L
#define EXP_MASK  0x7FF00000L
#define FULL_MASK 0xFFFFFFFFL
#define FRAC_MASK 0x000FFFFFL
#define ABS_MASK  0x7FFFFFFFL

#ifdef MSH_FIRST
#ifdef HW_MSH_FIRST
#define MSQ(x) (*(((volatile unsigned short int *)&x)))
#else
#define MSQ(x) (*(((volatile unsigned short int *)&x)+1))
#endif
#define LSH(x) (*(((volatile unsigned long int *)&x)+1))
#define MSH(x) (*(volatile unsigned long int *)&x)
#else
#ifdef HW_MSH_FIRST
#define MSQ(x) (*(((volatile unsigned short int *)&x)+2))
#else
#define MSQ(x) (*(((volatile unsigned short int *)&x)+3))
#endif
#define MSH(x) (*(((volatile unsigned long int *)&x)+1))
#define LSH(x) (*(volatile unsigned long int *)&x)
#endif

#define DOUBLE(x) (*(double*)x)

/* Special case codes */

#define is_sNaN(x) (((MSH(x)&0x00080000L)==0)?1:0)
#define is_fsNaN(x) ((((x.i)&0x00400000L)==0)?1:0)
#define is_single_sNaN(x) (((x.i)&0x00400000L)==0)?1:0)
#define fcn_mask 0xffc0
#define error_mask 0x003f

#define INFINITY          0x0000
#define INVALID_OPERATION 0x0001
#define QNAN              0x0002
#define OUT_OF_RANGE      0x0004
#define NEGATIVE_NUMBER   0x0008
#define DIV_BY_ZERO       0x0010
#define ARGUMENT_ERROR    0x0020

#define DOUBLE_PRECISION   0x8000
#define COMPLEX_FLAG       0x4000

#define ATAN              0x0000
#define COS               0x1000
#define EXP               0x2000
#define LOG               0x3000
#define POW               0x0100
#define SIN               0x0200 
#define SQRT              0x0300
#define TAN               0x0400
#define ABSS              0x0500
#define SIGN              0x0600
#define DIM               0x0700
#define INT               0x0800
#define NINT              0x0900
#define SINH              0x0A00
#define LOG10             0x0B00
#define COSH              0x0C00
#define ASIN              0x0D00
#define MAX1              0x0E00
#define MIN1              0x0F00
#define ATAN2             0x1100
#define TANH              0x1200
#define MOD               0x1300
#define CONJ              0x1400
#define COT               0x1500
#define DIV               0x1600
#define ACOS              0x1700
#define HYPOT             0x1800

#define DATAN  ATAN  | DOUBLE_PRECISION
#define DCOS   COS   | DOUBLE_PRECISION
#define DEXP   EXP   | DOUBLE_PRECISION
#define DLOG   LOG   | DOUBLE_PRECISION
#define DPOW   POW   | DOUBLE_PRECISION
#define DSIN   SIN   | DOUBLE_PRECISION
#define DSQRT  SQRT  | DOUBLE_PRECISION
#define DTAN   TAN   | DOUBLE_PRECISION
#define DABS   ABSS  | DOUBLE_PRECISION
#define DSIGN  SIGN  | DOUBLE_PRECISION
#define DDIM   DIM   | DOUBLE_PRECISION
#define DINT   INT   | DOUBLE_PRECISION
#define DNINT  NINT  | DOUBLE_PRECISION
#define DSINH  SINH  | DOUBLE_PRECISION
#define DLOG10 LOG10 | DOUBLE_PRECISION
#define DCOSH  COSH  | DOUBLE_PRECISION
#define DASIN  ASIN  | DOUBLE_PRECISION
#define DACOS  ACOS  | DOUBLE_PRECISION
#define DMAX1  MAX1  | DOUBLE_PRECISION
#define DMIN1  MIN1  | DOUBLE_PRECISION
#define DATAN2 ATAN2 | DOUBLE_PRECISION
#define DTANH  TANH  | DOUBLE_PRECISION
#define DMOD   MOD   | DOUBLE_PRECISION
#define DCOT   COT   | DOUBLE_PRECISION
#define DHYPOT HYPOT | DOUBLE_PRECISION
#define CABS   ABSS  | COMPLEX_FLAG
#define CSQRT  SQRT  | COMPLEX_FLAG
#define CEXP   EXP   | COMPLEX_FLAG  
#define CLOG   LOG   | COMPLEX_FLAG  
#define CSIN   SIN   | COMPLEX_FLAG
#define CCOS   COS   | COMPLEX_FLAG
#define CPOW   POW   | COMPLEX_FLAG
#define CTAN   TAN   | COMPLEX_FLAG
#define CDIV   DIV   | COMPLEX_FLAG


#define DATAN_QNAN               DATAN | QNAN
#define DATAN_INVALID_OPERATION  DATAN | INVALID_OPERATION

#define FATAN_QNAN               ATAN  | QNAN
#define FATAN_INVALID_OPERATION  ATAN  | INVALID_OPERATION

#define DCOS_INFINITY            DCOS  | INFINITY
#define DCOS_QNAN                DCOS  | QNAN
#define DCOS_INVALID_OPERATION   DCOS  | INVALID_OPERATION
#define DCOS_OUT_OF_RANGE        DCOS  | OUT_OF_RANGE

#define FCOS_INFINITY            COS   | INFINITY
#define FCOS_QNAN                COS   | QNAN
#define FCOS_INVALID_OPERATION   COS   | INVALID_OPERATION
#define FCOS_OUT_OF_RANGE        COS   | OUT_OF_RANGE

#define DEXP_INFINITY            DEXP  | INFINITY
#define DEXP_QNAN                DEXP  | QNAN
#define DEXP_INVALID_OPERATION   DEXP  | INVALID_OPERATION
#define DEXP_OUT_OF_RANGE        DEXP  | OUT_OF_RANGE

#define FEXP_INFINITY            EXP   | INFINITY
#define FEXP_QNAN                EXP   | QNAN
#define FEXP_INVALID_OPERATION   EXP   | INVALID_OPERATION
#define FEXP_OUT_OF_RANGE        EXP   | OUT_OF_RANGE

#define DLOG_INFINITY            DLOG  | INFINITY
#define DLOG_QNAN                DLOG  | QNAN
#define DLOG_INVALID_OPERATION   DLOG  | INVALID_OPERATION
#define DLOG_OUT_OF_RANGE        DLOG  | OUT_OF_RANGE

#define FLOG_INFINITY            LOG   | INFINITY
#define FLOG_QNAN                LOG   | QNAN
#define FLOG_INVALID_OPERATION   LOG   | INVALID_OPERATION
#define FLOG_OUT_OF_RANGE        LOG   | OUT_OF_RANGE

#define DPOW_INFINITY            DPOW  | INFINITY
#define DPOW_QNAN                DPOW  | QNAN
#define DPOW_INVALID_OPERATION   DPOW  | INVALID_OPERATION
#define DPOW_OUT_OF_RANGE        DPOW  | OUT_OF_RANGE
#define DPOW_DIV_BY_ZERO         DPOW  | DIV_BY_ZERO

#define FPOW_INFINITY            POW   | INFINITY
#define FPOW_QNAN                POW   | QNAN
#define FPOW_INVALID_OPERATION   POW   | INVALID_OPERATION
#define FPOW_OUT_OF_RANGE        POW   | OUT_OF_RANGE
#define FPOW_DIV_BY_ZERO         POW   | DIV_BY_ZERO

#define DSIN_INFINITY            DSIN  | INFINITY
#define DSIN_QNAN                DSIN  | QNAN
#define DSIN_INVALID_OPERATION   DSIN  | INVALID_OPERATION
#define DSIN_OUT_OF_RANGE        DSIN  | OUT_OF_RANGE

#define FSIN_INFINITY            SIN   | INFINITY
#define FSIN_QNAN                SIN   | QNAN
#define FSIN_INVALID_OPERATION   SIN   | INVALID_OPERATION
#define FSIN_OUT_OF_RANGE        SIN   | OUT_OF_RANGE

#define DSQRT_INFINITY           DSQRT | INFINITY
#define DSQRT_QNAN               DSQRT | QNAN
#define DSQRT_INVALID_OPERATION  DSQRT | INVALID_OPERATION
#define DSQRT_OUT_OF_RANGE       DSQRT | OUT_OF_RANGE

#define FSQRT_INFINITY           SQRT  | INFINITY
#define FSQRT_QNAN               SQRT  | QNAN
#define FSQRT_INVALID_OPERATION  SQRT  | INVALID_OPERATION
#define FSQRT_OUT_OF_RANGE       SQRT  | OUT_OF_RANGE

#define DTAN_INFINITY            DTAN  | INFINITY
#define DTAN_QNAN                DTAN  | QNAN
#define DTAN_INVALID_OPERATION   DTAN  | INVALID_OPERATION
#define DTAN_OUT_OF_RANGE        DTAN  | OUT_OF_RANGE

#define FTAN_INFINITY            TAN   | INFINITY
#define FTAN_QNAN                TAN   | QNAN
#define FTAN_INVALID_OPERATION   TAN   | INVALID_OPERATION
#define FTAN_OUT_OF_RANGE        TAN   | OUT_OF_RANGE

#define DABS_QNAN                DABS  | QNAN
#define DABS_INVALID_OPERATION   DABS  | INVALID_OPERATION

#define DSIGN_QNAN               DSIGN | QNAN
#define DSIGN_INVALID_OPERATION  DSIGN | INVALID_OPERATION

#define DDIM_INVALID_OPERATION   DDIM  | INVALID_OPERATION
#define DDIM_QNAN                DDIM  | QNAN
#define DDIM_INFINITY            DDIM  | INFINITY

#define DINT_INVALID_OPERATION   DINT  | INVALID_OPERATION
#define DINT_QNAN                DINT  | QNAN
#define DINT_INFINITY            DINT  | INFINITY

#define DNINT_INVALID_OPERATION  DNINT | INVALID_OPERATION
#define DNINT_QNAN               DNINT | QNAN
#define DNINT_INFINITY           DNINT | INFINITY

#define DSINH_INFINITY           DSINH | INFINITY
#define DSINH_QNAN               DSINH | QNAN
#define DSINH_INVALID_OPERATION  DSINH | INVALID_OPERATION
#define DSINH_OUT_OF_RANGE       DSINH | OUT_OF_RANGE

#define DLOG10_INFINITY            DLOG10  | INFINITY
#define DLOG10_QNAN                DLOG10  | QNAN
#define DLOG10_INVALID_OPERATION   DLOG10  | INVALID_OPERATION
#define DLOG10_OUT_OF_RANGE        DLOG10  | OUT_OF_RANGE

#define DCOSH_INFINITY           DCOSH | INFINITY
#define DCOSH_QNAN               DCOSH | QNAN
#define DCOSH_INVALID_OPERATION  DCOSH | INVALID_OPERATION
#define DCOSH_OUT_OF_RANGE       DCOSH | OUT_OF_RANGE

#define DASIN_INFINITY           DASIN | INFINITY
#define DASIN_QNAN               DASIN | QNAN
#define DASIN_INVALID_OPERATION  DASIN | INVALID_OPERATION
#define DASIN_OUT_OF_RANGE       DASIN | OUT_OF_RANGE

#define DACOS_INFINITY           DACOS | INFINITY
#define DACOS_QNAN               DACOS | QNAN
#define DACOS_INVALID_OPERATION  DACOS | INVALID_OPERATION
#define DACOS_OUT_OF_RANGE       DACOS | OUT_OF_RANGE

#define CONJ_INVALID_OPERATION   CONJ  | INVALID_OPERATION

#define DMAX1_QNAN               DMAX1 | QNAN
#define DMAX1_INVALID_OPERATION  DMAX1 | INVALID_OPERATION
#define DMAX1_ARGUMENT_ERROR     DMAX1 | ARGUMENT_ERROR

#define DMIN1_QNAN               DMIN1 | QNAN
#define DMIN1_INVALID_OPERATION  DMIN1 | INVALID_OPERATION
#define DMIN1_ARGUMENT_ERROR     DMIN1 | ARGUMENT_ERROR

#define DATAN2_QNAN               DATAN2 | QNAN
#define DATAN2_INVALID_OPERATION  DATAN2 | INVALID_OPERATION
#define DATAN2_OUT_OF_RANGE       DATAN2 | OUT_OF_RANGE

#define CABS_INFINITY            CABS  | INFINITY
#define CABS_QNAN                CABS  | QNAN
#define CABS_INVALID_OPERATION   CABS  | INVALID_OPERATION
#define CABS_OUT_OF_RANGE        CABS  | OUT_OF_RANGE

#define CLOG_INFINITY            CLOG  | INFINITY
#define CLOG_QNAN                CLOG  | QNAN
#define CLOG_INVALID_OPERATION   CLOG  | INVALID_OPERATION
#define CLOG_OUT_OF_RANGE        CLOG  | OUT_OF_RANGE

#define CEXP_INFINITY            CEXP  | INFINITY
#define CEXP_QNAN                CEXP  | QNAN
#define CEXP_INVALID_OPERATION   CEXP  | INVALID_OPERATION
#define CEXP_OUT_OF_RANGE        CEXP  | OUT_OF_RANGE

#define CSIN_INFINITY            CSIN  | INFINITY
#define CSIN_QNAN                CSIN  | QNAN
#define CSIN_INVALID_OPERATION   CSIN  | INVALID_OPERATION
#define CSIN_OUT_OF_RANGE        CSIN  | OUT_OF_RANGE

#define CCOS_INFINITY            CCOS  | INFINITY
#define CCOS_QNAN                CCOS  | QNAN
#define CCOS_INVALID_OPERATION   CCOS  | INVALID_OPERATION
#define CCOS_OUT_OF_RANGE        CCOS  | OUT_OF_RANGE

#define CPOW_INFINITY            CPOW  | INFINITY
#define CPOW_QNAN                CPOW  | QNAN
#define CPOW_INVALID_OPERATION   CPOW  | INVALID_OPERATION
#define CPOW_OUT_OF_RANGE        CPOW  | OUT_OF_RANGE

#define CTAN_INFINITY            CTAN  | INFINITY
#define CTAN_QNAN                CTAN  | QNAN
#define CTAN_INVALID_OPERATION   CTAN  | INVALID_OPERATION
#define CTAN_OUT_OF_RANGE        CTAN  | OUT_OF_RANGE

#define CDIV_INFINITY            CDIV  | INFINITY
#define CDIV_QNAN                CDIV  | QNAN
#define CDIV_INVALID_OPERATION   CDIV  | INVALID_OPERATION
#define CDIV_OUT_OF_RANGE        CDIV  | OUT_OF_RANGE
#define CDIV_DIV_BY_ZERO         CDIV  | DIV_BY_ZERO

#define CSQRT_INFINITY           CSQRT | INFINITY
#define CSQRT_QNAN               CSQRT | QNAN
#define CSQRT_INVALID_OPERATION  CSQRT | INVALID_OPERATION
#define CSQRT_OUT_OF_RANGE       CSQRT | OUT_OF_RANGE

#define DTANH_QNAN               DTANH | QNAN
#define DTANH_INVALID_OPERATION  DTANH | INVALID_OPERATION

#define DMOD_QNAN                DMOD | QNAN
#define DMOD_DIV_BY_ZERO         DMOD | DIV_BY_ZERO
#define DMOD_INVALID_OPERATION   DMOD | INVALID_OPERATION

#define DCOT_INFINITY            DCOT  | INFINITY
#define DCOT_QNAN                DCOT  | QNAN
#define DCOT_INVALID_OPERATION   DCOT  | INVALID_OPERATION
#define DCOT_DIV_BY_ZERO         DCOT  | DIV_BY_ZERO
#define DCOT_OUT_OF_RANGE        DCOT  | OUT_OF_RANGE

#define DHYPOT_QNAN              DHYPOT | QNAN
#define DHYPOT_INVALID_OPERATION DHYPOT | INVALID_OPERATION
#define DHYPOT_OUT_OF_RANGE      DHYPOT | OUT_OF_RANGE
#define DHYPOT_INFINITY          DHYPOT | INFINITY
