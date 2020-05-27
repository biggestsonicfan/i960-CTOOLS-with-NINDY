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

/* ctype.c - define _ctype array.
 * Copyright (c) 1986 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#define HEXIDECIMAL    0x80
#define SPACE          0x40
#define CONTROL        0x20
#define PUNCTUATOR     0x10
#define WHITESPACE     0x08
#define DIGIT          0x04
#define LOWERCASE      0x02
#define UPPERCASE      0x01

const unsigned char _ctype[257] = {
    0,                              /* 0xFFFF  -1.     */
    CONTROL,                        /* 0x0      0.     */
    CONTROL,                        /* 0x1      1.     */
    CONTROL,                        /* 0x2      2.     */
    CONTROL,                        /* 0x3      3.     */
    CONTROL,                        /* 0x4      4.     */
    CONTROL,                        /* 0x5      5.     */
    CONTROL,                        /* 0x6      6.     */
    CONTROL,                        /* 0x7      7.     */
    CONTROL,                        /* 0x8      8.     */
    CONTROL|WHITESPACE,             /* 0x9      9.     */
    CONTROL|WHITESPACE,             /* 0xA     10.     */
    CONTROL|WHITESPACE,             /* 0xB     11.     */
    CONTROL|WHITESPACE,             /* 0xC     12.     */
    CONTROL|WHITESPACE,             /* 0xD     13.     */
    CONTROL,                        /* 0xE     14.     */
    CONTROL,                        /* 0xF     15.     */
    CONTROL,                        /* 0x10    16.     */
    CONTROL,                        /* 0x11    17.     */
    CONTROL,                        /* 0x12    18.     */
    CONTROL,                        /* 0x13    19.     */
    CONTROL,                        /* 0x14    20.     */
    CONTROL,                        /* 0x15    21.     */
    CONTROL,                        /* 0x16    22.     */
    CONTROL,                        /* 0x17    23.     */
    CONTROL,                        /* 0x18    24.     */
    CONTROL,                        /* 0x19    25.     */
    CONTROL,                        /* 0x1A    26.     */
    CONTROL,                        /* 0x1B    27.     */
    CONTROL,                        /* 0x1C    28.     */
    CONTROL,                        /* 0x1D    29.     */
    CONTROL,                        /* 0x1E    30.     */
    CONTROL,                        /* 0x1F    31.     */
    SPACE|WHITESPACE,               /* 0x20    32. ' ' */
    PUNCTUATOR,                     /* 0x21    33. '!' */
    PUNCTUATOR,                     /* 0x22    34. '"' */
    PUNCTUATOR,                     /* 0x23    35. '#' */
    PUNCTUATOR,                     /* 0x24    36. '$' */
    PUNCTUATOR,                     /* 0x25    37. '%' */
    PUNCTUATOR,                     /* 0x26    38. '&' */
    PUNCTUATOR,                     /* 0x27    39. ''' */
    PUNCTUATOR,                     /* 0x28    40. '(' */
    PUNCTUATOR,                     /* 0x29    41. ')' */
    PUNCTUATOR,                     /* 0x2A    42. '*' */
    PUNCTUATOR,                     /* 0x2B    43. '+' */
    PUNCTUATOR,                     /* 0x2C    44. ',' */
    PUNCTUATOR,                     /* 0x2D    45. '-' */
    PUNCTUATOR,                     /* 0x2E    46. '.' */
    PUNCTUATOR,                     /* 0x2F    47. '/' */
    HEXIDECIMAL|DIGIT,              /* 0x30    48. '0' */
    HEXIDECIMAL|DIGIT,              /* 0x31    49. '1' */
    HEXIDECIMAL|DIGIT,              /* 0x32    50. '2' */
    HEXIDECIMAL|DIGIT,              /* 0x33    51. '3' */
    HEXIDECIMAL|DIGIT,              /* 0x34    52. '4' */
    HEXIDECIMAL|DIGIT,              /* 0x35    53. '5' */
    HEXIDECIMAL|DIGIT,              /* 0x36    54. '6' */
    HEXIDECIMAL|DIGIT,              /* 0x37    55. '7' */
    HEXIDECIMAL|DIGIT,              /* 0x38    56. '8' */
    HEXIDECIMAL|DIGIT,              /* 0x39    57. '9' */
    PUNCTUATOR,                     /* 0x3A    58. ':' */
    PUNCTUATOR,                     /* 0x3B    59. ';' */
    PUNCTUATOR,                     /* 0x3C    60. '<' */
    PUNCTUATOR,                     /* 0x3D    61. '=' */
    PUNCTUATOR,                     /* 0x3E    62. '>' */
    PUNCTUATOR,                     /* 0x3F    63. '?' */
    PUNCTUATOR,                     /* 0x40    64. '@' */
    HEXIDECIMAL|UPPERCASE,          /* 0x41    65. 'A' */
    HEXIDECIMAL|UPPERCASE,          /* 0x42    66. 'B' */
    HEXIDECIMAL|UPPERCASE,          /* 0x43    67. 'C' */
    HEXIDECIMAL|UPPERCASE,          /* 0x44    68. 'D' */
    HEXIDECIMAL|UPPERCASE,          /* 0x45    69. 'E' */
    HEXIDECIMAL|UPPERCASE,          /* 0x46    70. 'F' */
    UPPERCASE,                      /* 0x47    71. 'G' */
    UPPERCASE,                      /* 0x48    72. 'H' */
    UPPERCASE,                      /* 0x49    73. 'I' */
    UPPERCASE,                      /* 0x4A    74. 'J' */
    UPPERCASE,                      /* 0x4B    75. 'K' */
    UPPERCASE,                      /* 0x4C    76. 'L' */
    UPPERCASE,                      /* 0x4D    77. 'M' */
    UPPERCASE,                      /* 0x4E    78. 'N' */
    UPPERCASE,                      /* 0x4F    79. 'O' */
    UPPERCASE,                      /* 0x50    80. 'P' */
    UPPERCASE,                      /* 0x51    81. 'Q' */
    UPPERCASE,                      /* 0x52    82. 'R' */
    UPPERCASE,                      /* 0x53    83. 'S' */
    UPPERCASE,                      /* 0x54    84. 'T' */
    UPPERCASE,                      /* 0x55    85. 'U' */
    UPPERCASE,                      /* 0x56    86. 'V' */
    UPPERCASE,                      /* 0x57    87. 'W' */
    UPPERCASE,                      /* 0x58    88. 'X' */
    UPPERCASE,                      /* 0x59    89. 'Y' */
    UPPERCASE,                      /* 0x5A    90. 'Z' */
    PUNCTUATOR,                     /* 0x5B    91. '[' */
    PUNCTUATOR,                     /* 0x5C    92. '\' */
    PUNCTUATOR,                     /* 0x5D    93. ']' */
    PUNCTUATOR,                     /* 0x5E    94. '^' */
    PUNCTUATOR,                     /* 0x5F    95. '_' */
    PUNCTUATOR,                     /* 0x60    96. '`' */
    HEXIDECIMAL|LOWERCASE,          /* 0x61    97. 'a' */
    HEXIDECIMAL|LOWERCASE,          /* 0x62    98. 'b' */
    HEXIDECIMAL|LOWERCASE,          /* 0x63    99. 'c' */
    HEXIDECIMAL|LOWERCASE,          /* 0x64   100. 'd' */
    HEXIDECIMAL|LOWERCASE,          /* 0x65   101. 'e' */
    HEXIDECIMAL|LOWERCASE,          /* 0x66   102. 'f' */
    LOWERCASE,                      /* 0x67   103. 'g' */
    LOWERCASE,                      /* 0x68   104. 'h' */
    LOWERCASE,                      /* 0x69   105. 'i' */
    LOWERCASE,                      /* 0x6A   106. 'j' */
    LOWERCASE,                      /* 0x6B   107. 'k' */
    LOWERCASE,                      /* 0x6C   108. 'l' */
    LOWERCASE,                      /* 0x6D   109. 'm' */
    LOWERCASE,                      /* 0x6E   110. 'n' */
    LOWERCASE,                      /* 0x6F   111. 'o' */
    LOWERCASE,                      /* 0x70   112. 'p' */
    LOWERCASE,                      /* 0x71   113. 'q' */
    LOWERCASE,                      /* 0x72   114. 'r' */
    LOWERCASE,                      /* 0x73   115. 's' */
    LOWERCASE,                      /* 0x74   116. 't' */
    LOWERCASE,                      /* 0x75   117. 'u' */
    LOWERCASE,                      /* 0x76   118. 'v' */
    LOWERCASE,                      /* 0x77   119. 'w' */
    LOWERCASE,                      /* 0x78   120. 'x' */
    LOWERCASE,                      /* 0x79   121. 'y' */
    LOWERCASE,                      /* 0x7A   122. 'z' */
    PUNCTUATOR,                     /* 0x7B   123. '{' */
    PUNCTUATOR,                     /* 0x7C   124. '|' */
    PUNCTUATOR,                     /* 0x7D   125. '}' */
    PUNCTUATOR,                     /* 0x7E   126. '~' */
    CONTROL,                        /* 0x7F   127.     */
};
