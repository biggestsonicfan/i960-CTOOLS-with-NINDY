/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1993, 1994, 1995 Intel Corporation
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

#ifndef STD_DEFS_H
#define STD_DEFS_H

#define SX_CPU (defined(__i960SA) || defined(__i960SA__) || (_80960SA == 1) || \
                defined(__i960SB) || defined(__i960SB__) || (_80960SB == 1))

#define KX_CPU (defined(__i960KA) || defined(__i960KA__) || (_80960KA == 1) || \
                defined(__i960KB) || defined(__i960KB__) || (_80960KB == 1))

#define CX_CPU (defined(__i960CA) || defined(__i960CA__) || (_80960CA == 1) || \
                defined(__i960CF) || defined(__i960CF__) || (_80960CF == 1))

#define JX_CPU (defined(__i960JA) || defined(__i960JA__) || (_80960JA == 1) || \
                defined(__i960JD) || defined(__i960JD__) || (_80960JD == 1) || \
                defined(__i960JF) || defined(__i960JF__) || (_80960JF == 1) || \
                defined(__i960JL) || defined(__i960JL__) || (_80960JL == 1) || \
                defined(__i960RP) || defined(__i960RP__) || (_80960RP == 1))

#define HX_CPU (defined(__i960HA) || defined(__i960HA__) || (_80960HA == 1) || \
                defined(__i960HD) || defined(__i960HD__) || (_80960HD == 1) || \
                defined(__i960HT) || defined(__i960HT__) || (_80960HT == 1))
/* Standard practice is CPU lists in alphbetic order. */
#define KXSX_CPU (KX_CPU || SX_CPU)
#define CXHX_CPU (CX_CPU || HX_CPU)
#define CXJX_CPU (CX_CPU || JX_CPU)
#define HXJX_CPU (HX_CPU || JX_CPU)
#define CXHXJX_CPU (CX_CPU || HX_CPU || JX_CPU)
/* ANY Which order you define CPU defs will be correct */
#define SXKX_CPU KXSX_CPU
#define HXCX_CPU CXHX_CPU
#define JXHX_CPU HXJX_CPU
#define JXCX_CPU CXJX_CPU
#define CXJXHX_CPU CXHXJX_CPU
#define HXCXJX_CPU CXHXJX_CPU
#define HXJXCX_CPU CXHXJX_CPU
#define JXCXHX_CPU CXHXJX_CPU
#define JXHXCX_CPU CXHXJX_CPU

#define BIG_ENDIAN_CODE defined(__i960_BIG_ENDIAN__)

#define MRI_CODE defined(_MCC960)

#endif
