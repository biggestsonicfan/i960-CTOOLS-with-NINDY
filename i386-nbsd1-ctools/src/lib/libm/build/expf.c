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

/*** qtc_dexpf.c ***/

/*
*******************************************************************************
**
** QTC INTRINSICS LIBRARY
**
**  DEXP TABLES
**
**   Power of 2 tables for double precision exponential.
**   Also used by double precision pow and double precision expm1.
**
**         dexpfh[j] + dexpfl[j] = 2**(j/32) for j = 0, 31
**
**   History:
**
**        Version 1.0 - 8 July 1988, J F Kauth and L A Westerman
**                1.1 - 21 July 1988, L A Westerman
**                1.2 - 26 October 1988, L A Westerman
**                1.3 -  7 December 1988, J F Kauth, zeroed last 6
**                       bits of dexpfh for compatibility with expm1
**
**   Copyright: 
**      (c) 1988
**      (c) 1989 Quantitative Technology Corporation.
**               8700 SW Creekside Place Suite D
**               Beaverton OR 97005 USA
**               503-626-3081
**      Intel usage with permission under contract #89B0090
** 
*******************************************************************************
*/

const unsigned long int _Ldexpfh[32][2] =  {
        0x00000000L, 0x3FF00000L,  /*  1.00000000000000000e+00 */
        0xD3158540L, 0x3FF059B0L,  /*  1.02189714865410510e+00 */
        0x6CF98900L, 0x3FF0B558L,  /*  1.04427378242741040e+00 */
        0xD0125B40L, 0x3FF11301L,  /*  1.06714040067681990e+00 */
        0x3C7D5140L, 0x3FF172B8L,  /*  1.09050773266524460e+00 */
        0x3168B980L, 0x3FF1D487L,  /*  1.11438674259588310e+00 */
        0x6E756200L, 0x3FF2387AL,  /*  1.13878863475667910e+00 */
        0xF51FDEC0L, 0x3FF29E9DL,  /*  1.16372485877757010e+00 */
        0x0A31B700L, 0x3FF306FEL,  /*  1.18920711500271640e+00 */
        0x373AA9C0L, 0x3FF371A7L,  /*  1.21524735998046650e+00 */
        0x4C123400L, 0x3FF3DEA6L,  /*  1.24185781207347650e+00 */
        0x60618900L, 0x3FF44E08L,  /*  1.26905095719172320e+00 */
        0xD5362A00L, 0x3FF4BFDAL,  /*  1.29683955465100100e+00 */
        0x569D4F80L, 0x3FF5342BL,  /*  1.32523664315974090e+00 */
        0xDD485400L, 0x3FF5AB07L,  /*  1.35425554693688350e+00 */
        0xB03A5580L, 0x3FF6247EL,  /*  1.38390988196383090e+00 */
        0x667F3BC0L, 0x3FF6A09EL,  /*  1.41421356237309230e+00 */
        0xE8EC5F40L, 0x3FF71F75L,  /*  1.44518080697703510e+00 */
        0x73EB0180L, 0x3FF7A114L,  /*  1.47682614593949780e+00 */
        0x994CCE00L, 0x3FF82589L,  /*  1.50916442759341860e+00 */
        0x422AA0C0L, 0x3FF8ACE5L,  /*  1.54221082540793470e+00 */
        0xB0CDC5C0L, 0x3FF93737L,  /*  1.57598084510787830e+00 */
        0x82A3F080L, 0x3FF9C491L,  /*  1.61049033194925070e+00 */
        0xB23E2540L, 0x3FFA5503L,  /*  1.64575547815395850e+00 */
        0x995AD380L, 0x3FFAE89FL,  /*  1.68179283050741900e+00 */
        0xF2FB5E40L, 0x3FFB7F76L,  /*  1.71861929812247640e+00 */
        0xDD855280L, 0x3FFC199BL,  /*  1.75625216037329320e+00 */
        0xDCEF9040L, 0x3FFCB720L,  /*  1.79470907500309810e+00 */
        0xDCFBA480L, 0x3FFD5818L,  /*  1.83400808640934090e+00 */
        0x337B9B40L, 0x3FFDFC97L,  /*  1.87416763411029310e+00 */
        0xA2A490C0L, 0x3FFEA4AFL,  /*  1.91520656139714160e+00 */
        0x5B6E4540L, 0x3FFF5076L   /*  1.95714412417540020e+00 */
    };
const unsigned long int _Ldexpfl[32][2] = {
        0x00000000L, 0x00000000L,  /*  0.00000000000000000e+00 */
        0xE2A475B4L, 0x3D0A1D73L,  /*  1.15974117063913630e-14 */
        0x7256E308L, 0x3CEEC531L,  /*  3.41618797093084940e-15 */
        0xBF1AED93L, 0x3CF0A4EBL,  /*  3.69575974405711660e-15 */
        0xBE462876L, 0x3D0D6E6FL,  /*  1.30701638697787210e-14 */
        0xDC0144C8L, 0x3D053C02L,  /*  9.42997619141977060e-15 */
        0xFD6D8E0BL, 0x3D0C3360L,  /*  1.25236260025620070e-14 */
        0xE8AFAD12L, 0x3D009612L,  /*  7.36576401089527360e-15 */
        0xD5A46306L, 0x3CF52DE8L,  /*  4.70275685574031320e-15 */
        0xAA05E8A9L, 0x3CE54E28L,  /*  2.36536434724852950e-15 */
        0x0911F09FL, 0x3D011ADAL,  /*  7.59609684336943400e-15 */
        0xB7A04EF8L, 0x3D068189L,  /*  9.99467515375775050e-15 */
        0xCBD7F621L, 0x3D038EA1L,  /*  8.68512209487110810e-15 */
        0x3C49D86AL, 0x3CBDF0A8L,  /*  4.15501897749673970e-16 */
        0x980A8C8FL, 0x3D04AC64L,  /*  9.18083828572431140e-15 */
        0xE81BF4B7L, 0x3CD2C7C3L,  /*  1.04251790803720870e-15 */
        0x5F626CDDL, 0x3CE92116L,  /*  2.78990693089087750e-15 */
        0xB8797785L, 0x3D09EE91L,  /*  1.15160818747516890e-14 */
        0x408FDB37L, 0x3CDB5F54L,  /*  1.51947228890629130e-15 */
        0x88AFAB35L, 0x3CF28ACFL,  /*  4.11720196080016620e-15 */
        0xC55A192DL, 0x3CFB5BA7L,  /*  6.07470268107282240e-15 */
        0x0E1F92A0L, 0x3D027A28L,  /*  8.20551346575487980e-15 */
        0x46B071F3L, 0x3CF01C7CL,  /*  3.57742087137029860e-15 */
        0x4491CAF8L, 0x3CFC8B42L,  /*  6.33803674368915890e-15 */
        0x9A68BB99L, 0x3D06AF43L,  /*  1.00739973218322240e-14 */
        0xC206AD4FL, 0x3CDBAA9EL,  /*  1.53579843029258800e-15 */
        0xCB12A092L, 0x3CFC2220L,  /*  6.24685034485536580e-15 */
        0xE5E8F4A5L, 0x3D048A81L,  /*  9.12205626035419390e-15 */
        0x16BAD9B8L, 0x3CDC9768L,  /*  1.58714330671767550e-15 */
        0xCAC39ED3L, 0x3CFEB968L,  /*  6.82215511854592900e-15 */
        0x73A18F5EL, 0x3CF9858FL,  /*  5.66696026748885370e-15 */
        0x2DD8A18BL, 0x3C99D3E1L   /*  8.96076779103666790e-17 */
    };
/*** end of qtc_dexpf.c ***/
