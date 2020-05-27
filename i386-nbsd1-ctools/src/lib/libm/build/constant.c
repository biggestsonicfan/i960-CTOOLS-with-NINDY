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


/*
 * The following defines the actual constant values.
 */

/* rounded up */
const unsigned int	_Le_log2_e[4] = {0x5c17f0bc, 0xb8aa3b29, 0x3fff, 0};

/* rounded up */
const unsigned int	_e_loge_2[4] = {0xd1cf79ac, 0xb17217f7, 0x3ffe, 0};

/* rounded up */
const unsigned int	_Le_log10_2[4] = {0xfbcff799, 0x9a209a84, 0x3ffd, 0};

const unsigned int	_Le_posinf[4] = {0x00000000, 0x80000000, 0x7fff, 0};

const unsigned int	_Le_neginf[4] = {0x00000000, 0x80000000, 0xffff, 0};

const unsigned int	_d_posinf[2] = {0x00000000, 0x7ff00000};

const unsigned int	_Ld_neginf[4] = {0x00000000, 0xfff00000};

const unsigned int	_Ls_posinf[1] = {0x7f800000};

const unsigned int	_Ls_neginf[1] = {0xff800000};

const unsigned int	_Le_qnan[4] = {0x00000000, 0xc0000000, 0x7fff, 0};

const unsigned int	_Le_negqnan[4] = {0x00000000, 0xc0000000, 0xffff, 0};

const unsigned int	_Ld_qnan[2] = {0x00000000, 0x7ff80000};

const unsigned int	_Ld_negqnan[2] = {0x00000000, 0xfff80000};

const unsigned int	_Ls_qnan[1] = {0x7fc00000};

const unsigned int	_Ls_negqnan[1] = {0xffc00000};

const unsigned int	_Le_logep_hlim[4] = {0xf9de6484, 0xb504f333, 0x3fff, 0};

const unsigned int	_Ld_logep_hlim[2] = {0x667f3bcc, 0x3ff6a09e};

const unsigned int	_Ls_logep_hlim[1] = {0x3fb504f3};

const unsigned int	_Le_logep_llim[4] = {0xf9de6485, 0xb504f333, 0x3ffe, 0};

const unsigned int	_Ld_logep_llim[2] = {0x667f3bcd, 0x3fe6a09e};

const unsigned int	_Ls_logep_llim[1] = {0x3f3504f4};

/* rounded up */
const unsigned int	_Le_tanh_hlim[4] = {0x6fa53360, 0xb9c37117, 0x4003, 0};

const unsigned int	_Le_tanh_llim[4] = {0x0, 0x80000000, 0x3fdf, 0};

const unsigned int	_Ld_tanh_hlim[2] = {0x931f09ca,0x40330fc1};

const unsigned int	_Ld_tanh_llim[2] = {0x0, 0x3e500000};

const unsigned int	_Ls_tanh_hlim[1] = {0x41102cb3};

const unsigned int	_Ls_tanh_llim[1] = {0x39800000};
