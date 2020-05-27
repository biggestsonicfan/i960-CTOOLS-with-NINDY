/* flonum_const.c - Useful Flonum constants
   Copyright (C) 1987, 1990, 1991 Free Software Foundation, Inc.

This file is part of GAS, the GNU Assembler.

GAS is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GAS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GAS; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* static const char rcsid[] = "$Id: fp_const.c,v 1.8 1994/08/11 19:19:55 peters Exp $"; */

#include "flonum.h"

const int table_size_of_flonum_powers_of_ten = 13;

static const LITTLENUM_TYPE zero[] = {     1 };

/*To the best of my knowledge, these constants are accurate.  At least     *
* they pass the tests I have.  (Paul Reger Tue Dec 22 22:03:16 PST 1992)   *
*                                                                          *
* Please see make_table.c and flt_test.c for how to regenerate             *
* these tables, and how to test their accuracy.                            *
\**************************************************************************/

#define X (LITTLENUM_TYPE *)

/* The following tables are automatically created by make_tables.c  DO
   NOT HAND MODIFY. */

/* entry 1 */
static const LITTLENUM_TYPE plus_1 [] = {
    10,
};
static const LITTLENUM_TYPE minus_1 [] = {
 39321, 39321, 39321, 39321, 39321, 39321, 39321, 39321, 39321, 39321,
 39321, 39321, 39321, 39321, 39321, 39321, 39321, 39321, 39321, 39321,
 39321, 39321, 39321, 39321, 39321, 39321, 39321, 39321, 39321,  6553,
};

/* entry 2 */
static const LITTLENUM_TYPE plus_2 [] = {
   100,
};
static const LITTLENUM_TYPE minus_2 [] = {
 10485, 36700, 62914, 23592, 49807, 10485, 36700, 62914, 23592, 49807,
 10485, 36700, 62914, 23592, 49807, 10485, 36700, 62914, 23592, 49807,
 10485, 36700, 62914, 23592, 49807, 10485, 36700, 62914, 23592,   655,
};

/* entry 3 */
static const LITTLENUM_TYPE plus_3 [] = {
 10000,
};
static const LITTLENUM_TYPE minus_3 [] = {
 36805, 30513,  8493, 62495, 22124, 44669, 25270, 55679, 31037, 43096,
 52533, 20027, 37329, 65116, 64067, 60397, 14784, 18979, 33659, 19503,
  2726,  9542,   629,  2202, 40475, 10590,  4299, 47815, 36280,     6,
};

/* entry 4 */
static const LITTLENUM_TYPE plus_4 [] = {
 57600,  1525,
};
static const LITTLENUM_TYPE minus_4 [] = {
  4122, 59045, 18752, 63328, 16966, 31125, 59183, 49383, 30289, 49443,
 56974, 22516, 49501, 54293, 19424, 60699,  6716, 24348, 22618, 23904,
 21327,  3919, 44703, 19149, 28803, 48959,  6259, 50273, 62237,    42,
};

/* entry 5 */
static const LITTLENUM_TYPE plus_5 [] = {
 28609, 34546,    35,
};
static const LITTLENUM_TYPE minus_5 [] = {
 42163, 46306, 60871, 23935, 23164, 28097, 55089, 20787,  5286,  5164,
 15994, 31741,   150, 22199, 45957, 17005, 26266, 10526, 16260, 55017,
 35680, 40443, 19789, 17356, 30195, 55905, 28426, 63010, 44197,  1844,
};

/* entry 6 */
static const LITTLENUM_TYPE plus_6 [] = {
 61313, 34220, 16731, 11629,  1262,
};
static const LITTLENUM_TYPE minus_6 [] = {
 21339, 17718, 41919, 35312, 24953,  4340, 10478, 40974,  8119, 43983,
 34917,  9349, 40322, 35349, 60259, 14561, 30926, 26518, 13110, 43018,
 54982, 48258, 24658, 15209, 63366, 11929, 20069, 43857, 60487,    51,
};

/* entry 7 */
static const LITTLENUM_TYPE plus_7 [] = {
  7937, 49002, 60772, 28216, 38893, 55975, 63988, 59711, 20227,    24,
};
static const LITTLENUM_TYPE minus_7 [] = {
  1119,  4618,  9014, 44872,  4542, 24299, 39427, 54886,  9954, 40521,
 53522, 28709,   788, 63218, 53171, 55608, 10421,  4321, 43699,  3472,
 65252, 17057, 13858, 29819, 14733, 21490, 40602, 31315, 65186,  2695,
};

/* entry 8 */
static const LITTLENUM_TYPE plus_8 [] = {
 15873, 11925, 39177,   991, 14589,  3861, 58415,  9076, 62956, 54223,
 56328, 50180, 45274, 48333, 32537, 42547,  9731, 59679,   590,
};
static const LITTLENUM_TYPE minus_8 [] = {
  9945, 62884, 43630, 63478, 30055, 21784, 12430, 57495,  9456, 58615,
  1273, 12232, 31186, 43385, 63804, 65060, 27578, 64807, 12543,   794,
 13907, 61297, 12013, 64360, 15961, 20566, 24178, 15922, 59427,   110,
};

/* entry 9 */
static const LITTLENUM_TYPE plus_9 [] = {
 20678, 27614, 28272, 53066, 55311, 54677, 29038,  9906, 26288, 44486,
 13860,  7445, 54106, 15426, 21518, 25599, 29632, 52309, 61207, 26105,
 10482, 21948, 51191, 32988, 60892, 62574, 61390, 24540, 21495,     5,
};
static const LITTLENUM_TYPE minus_9 [] = {
 63760, 20921, 43561, 11041,  8464, 58971, 63429,  6022, 63485,  5500,
 53464, 47545, 50068, 56988, 22819, 49708, 54493,  9920, 47667, 40409,
 35764, 10383, 54466, 32702, 17493, 32420, 34382, 22750, 20681, 12300,
};

/* entry 10 */
static const LITTLENUM_TYPE plus_10 [] = {
 63839, 36576, 45712, 44516, 37803, 29482,  4966, 30556, 37961, 23310,
 27070, 44972, 29507, 48257, 45209,  7494, 17831, 38728, 41577, 29443,
 36016,  7955, 35339, 35479, 36011, 14553, 49618,  5588, 25396,    28,
};
static const LITTLENUM_TYPE minus_10 [] = {
  6214, 48771, 23471, 30163, 31763, 38013, 57001, 11770, 18263, 36366,
 20742, 45086, 56969, 53231, 37856, 55814, 38057, 15692, 46761,  8713,
  6102, 20083,  8269, 11839, 11571, 50963, 15649, 11698, 40675,  2308,
};

/* entry 11 */
static const LITTLENUM_TYPE plus_11 [] = {
    91,  9054, 62707, 17993,  7821, 56838, 13992, 21321, 29637, 48426,
 42982, 38668, 49574, 28820, 18200, 18927, 53979, 16219, 37484,  2516,
 44642, 14665, 11587, 41926, 13556, 23956, 54320,  6661, 55766,   805,
};
static const LITTLENUM_TYPE minus_11 [] = {
 16662, 56882, 61983,  7804, 36555, 32060, 34502,  1000, 14356, 21681,
  6605, 34767, 51411, 59048, 53614, 39850, 30079,  6496,  6846, 26841,
 40778, 19578, 59899, 44085, 54016, 24259, 11232, 21229, 21313,    81,
};

/* entry 12 */
static const LITTLENUM_TYPE plus_12 [] = {
 10097, 37922, 58070,  7432, 10470, 63465, 23718, 62190, 47420,  7009,
 38443,  4587, 45596, 38472, 52129, 52779, 29012, 13559, 48688, 31678,
 41753, 58662, 10668, 36067, 29906, 56906, 21461, 46556, 59571,     9,
};
static const LITTLENUM_TYPE minus_12 [] = {
 33202, 45969, 58804, 56734, 16482, 26007, 44984, 49334, 31007, 32944,
 44517, 63329, 47131, 15291, 59465,  2264, 23218, 11829, 59771, 38798,
 31051, 28748, 23129, 40541, 41562, 35108, 50620, 59014, 51817,  6613,
};

/* entry 13 */
static const LITTLENUM_TYPE plus_13 [] = {
 18788, 16960,  6318, 45685, 55400, 46230, 35794, 25588,  7253, 55541,
 49716, 59760, 63592,  8191, 63765, 58530, 44667, 13294, 10001, 55586,
 47887, 18738,  9509, 40896, 42506, 52580,  4171,   325, 12329,    98,
};
static const LITTLENUM_TYPE minus_13 [] = {
 45309, 27592, 37144, 34637, 34328, 41671, 34620, 24135, 53401, 22112,
 21576, 45147, 39310, 44051, 48572,  3676, 46544, 59768, 33350,  2323,
 49524, 61568,  3903, 36487, 36356, 30903, 14975,  9035, 29715,   667,
};
const FLONUM_TYPE flonum_positive_powers_of_ten [] = {
{X zero,X zero,X zero,0,'+'},
{X plus_1,X plus_1 + 0,X plus_1+0, 0, '+'},
{X plus_2,X plus_2 + 0,X plus_2+0, 0, '+'},
{X plus_3,X plus_3 + 0,X plus_3+0, 0, '+'},
{X plus_4,X plus_4 + 1,X plus_4+1, 0, '+'},
{X plus_5,X plus_5 + 2,X plus_5+2, 1, '+'},
{X plus_6,X plus_6 + 4,X plus_6+4, 2, '+'},
{X plus_7,X plus_7 + 9,X plus_7+9, 4, '+'},
{X plus_8,X plus_8 + 18,X plus_8+18, 8, '+'},
{X plus_9,X plus_9 + 29,X plus_9+29, 24, '+'},
{X plus_10,X plus_10 + 29,X plus_10+29, 77, '+'},
{X plus_11,X plus_11 + 29,X plus_11+29, 183, '+'},
{X plus_12,X plus_12 + 29,X plus_12+29, 396, '+'},
{X plus_13,X plus_13 + 29,X plus_13+29, 821, '+'},
};
const FLONUM_TYPE flonum_negative_powers_of_ten [] = {
{X zero,X zero,X zero,0,'+'},
{X minus_1,X minus_1 + 29,X minus_1+29, -30, '+'},
{X minus_2,X minus_2 + 29,X minus_2+29, -30, '+'},
{X minus_3,X minus_3 + 29,X minus_3+29, -30, '+'},
{X minus_4,X minus_4 + 29,X minus_4+29, -31, '+'},
{X minus_5,X minus_5 + 29,X minus_5+29, -33, '+'},
{X minus_6,X minus_6 + 29,X minus_6+29, -36, '+'},
{X minus_7,X minus_7 + 29,X minus_7+29, -43, '+'},
{X minus_8,X minus_8 + 29,X minus_8+29, -56, '+'},
{X minus_9,X minus_9 + 29,X minus_9+29, -83, '+'},
{X minus_10,X minus_10 + 29,X minus_10+29, -136, '+'},
{X minus_11,X minus_11 + 29,X minus_11+29, -242, '+'},
{X minus_12,X minus_12 + 29,X minus_12+29, -455, '+'},
{X minus_13,X minus_13 + 29,X minus_13+29, -880, '+'},
};

