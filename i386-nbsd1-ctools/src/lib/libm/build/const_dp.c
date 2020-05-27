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

/*** qtc_constants_dp.c ***/

/*
** This file defines constants for the 
** double precision and complex intrinsic functions.
*/

/*
**  Force double-word alignment
*/
static const double dummy = 0.0;

#ifdef MSH_FIRST
const unsigned long int _Ldexp_threshold_1[2] = {0x40862E42L, 0xFEFA39EFL};  
        /*  7.0978271289338e+02 */
const unsigned long int _Ldexp_threshold_2[2] = {0xC086232BL, 0xDD7ABCD2L};  
        /* -7.0839641853226e-02 */
const unsigned long int _Ldcos_threshold_1[2] = {0x417921FBL, 0x2219586AL};
        /*  2.6353586131187833e+07 */
const unsigned long int _Ldsin_threshold_1[2] ={0x416921FBL, 0x2219586AL};
        /*  1.3176793065593719e+07 */
const unsigned long int _Linfinity[2]         = {0x7FF00000L, 0x00000000L};
const unsigned long int _Lxmax[2]             = {0x7FEFFFFFL, 0xFFFFFFFFL};
const unsigned long int _LPI_over_2[2]        = {0x3FF921FBL, 0x54442D18L};
        /* 1.5707963267949e+00 */
const unsigned long int _Lhalf[2]             = {0x3FE00000L, 0x00000000L};
        /* 0.5 = 1/2 */
const unsigned long int _Ldpone[2]            = {0x3FF00000L, 0x00000000L};
        /* 1.0 */
const unsigned long int _Ldptwo[2]            = {0x40000000L, 0x00000000L};  
        /* 2.0 */
const unsigned long int _Lpi[2]               = {0x400921FBL,0x54442D18L};
        /* 3.1415926535898e+00 */
const unsigned long int _Lthrpio4[2]          = {0x4002D97CL,0x7F3321D2L};
        /* 2.3561944901923e+00 */
const unsigned long int _Lpio4[2]             = {0x3FE921FBL,0x54442D18L};
        /* 7.8539816339748e-01 */
const unsigned long int _Lnegzero[2]          = {0x80000000L,0x00000000L};
        /* - 0.0 */
const unsigned long int _Lposzero[2]          = {0x00000000L,0x00000000L};
        /* + 0.0 */
#else
const unsigned long int _Ldexp_threshold_1[2] = {0xFEFA39EFL, 0x40862E42L};  
        /*  7.0978271289338e+02 */
const unsigned long int _Ldexp_threshold_2[2] = {0xDD7ABCD2L, 0xC086232BL};  
        /* -7.0839641853226e-02 */
const unsigned long int _Ldcos_threshold_1[2] = {0x2219586AL, 0x417921FBL};
        /*  2.6353586131187833e+07 */
const unsigned long int _Ldsin_threshold_1[2] ={0x2219586AL, 0x416921FBL};
        /*  1.3176793065593719e+07 */
const unsigned long int _Linfinity[2]         = {0x00000000L, 0x7FF00000L};
const unsigned long int _Lxmax[2]             = {0xFFFFFFFFL, 0x7FEFFFFFL};
const unsigned long int _LPI_over_2[2]        = {0x54442D18L, 0x3FF921FBL};
        /* 1.5707963267949e+00 */
const unsigned long int _Lhalf[2]             = {0x00000000L, 0x3FE00000L};
        /* 0.5 = 1/2 */
const unsigned long int _Ldpone[2]            = {0x00000000L, 0x3FF00000L};  
        /* 1.0 */
const unsigned long int _Ldptwo[2]            = {0x00000000L, 0x40000000L};  
        /* 2.0 */
const unsigned long int _Lpi[2]               = {0x54442D18L,0x400921FBL};
        /* 3.1415926535898e+00 */
const unsigned long int _Lthrpio4[2]          = {0x7F3321D2L,0x4002D97CL};
        /* 2.3561944901923e+00 */
const unsigned long int _Lpio4[2]             = {0x54442D18L,0x3FE921FBL};
        /* 7.8539816339748e-01 */
const unsigned long int _Lnegzero[2]          = {0x00000000L,0x80000000L};
        /* - 0.0 */
const unsigned long int _Lposzero[2]          = {0x00000000L,0x00000000L};
        /* + 0.0 */
#endif
