/******************************************************************/
/* 		Copyright (c) 1989, Intel Corporation

   Intel hereby grants you permission to copy, modify, and 
   distribute this software and its documentation.  Intel grants
   this permission provided that the above copyright notice 
   appears in all copies and that both the copyright notice and
   this permission notice appear in supporting documentation.  In
   addition, Intel grants this permission provided that you
   prominently mark as not part of the original any modifications
   made to this software or documentation, and that the name of 
   Intel Corporation not be used in advertising or publicity 
   pertaining to distribution of the software or the documentation 
   without specific, written prior permission.  

   Intel Corporation does not warrant, guarantee or make any 
   representations regarding the use of, or the results of the use
   of, the software and documentation in terms of correctness, 
   accuracy, reliability, currentness, or otherwise; and you rely
   on the software, documentation and results solely at your own 
   risk.							  */
/******************************************************************/
/*     Initial Interrupt Table        */
 	.globl	_intr_table     
	.align	6
_intr_table: 
	.word	0		# Pending Priorities    0
	.fill	8,4,0		# Pending Interrupts   4 + (0->7)*4
	.word	_user_intr1;	# interrupt table entry   8
	.word	_user_intr1;	# interrupt table entry   9
	.word	_user_intr1;	# interrupt table entry  10
	.word	_user_intr1;	# interrupt table entry  11
	.word	_user_intr1;	# interrupt table entry  12
	.word	_user_intr1;	# interrupt table entry  13
	.word	_user_intr1;	# interrupt table entry  14
	.word	_user_intr1;	# interrupt table entry  15
	.word	_user_intr2;	# interrupt table entry  16
	.word	_user_intr2;	# interrupt table entry  17
	.word	_user_intr2;	# interrupt table entry  18
	.word	_user_intr2;	# interrupt table entry  19
	.word	_user_intr2;	# interrupt table entry  20
	.word	_user_intr2;	# interrupt table entry  21
	.word	_user_intr2;	# interrupt table entry  22
	.word	_user_intr2;	# interrupt table entry  23
	.word	_user_intr3;	# interrupt table entry  24
	.word	_user_intr3;	# interrupt table entry  25
	.word	_user_intr3;	# interrupt table entry  26
	.word	_user_intr3;	# interrupt table entry  27
	.word	_user_intr3;	# interrupt table entry  28
	.word	_user_intr3;	# interrupt table entry  29
	.word	_user_intr3;	# interrupt table entry  30
	.word	_user_intr3;	# interrupt table entry  31
	.word	_user_intr4;	# interrupt table entry  32
	.word	_user_intr4;	# interrupt table entry  33
	.word	_user_intr4;	# interrupt table entry  34
	.word	_user_intr4;	# interrupt table entry  35
	.word	_user_intr4;	# interrupt table entry  36
	.word	_user_intr4;	# interrupt table entry  37
	.word	_user_intr4;	# interrupt table entry  38
	.word	_user_intr4;	# interrupt table entry  39
	.word	_user_intr5;	# interrupt table entry  40
	.word	_user_intr5;	# interrupt table entry  41
	.word	_user_intr5;	# interrupt table entry  42
	.word	_user_intr5;	# interrupt table entry  43
	.word	_user_intr5;	# interrupt table entry  44
	.word	_user_intr5;	# interrupt table entry  45
	.word	_user_intr5;	# interrupt table entry  46
	.word	_user_intr5;	# interrupt table entry  47
	.word	_user_intr6;	# interrupt table entry  48
	.word	_user_intr6;	# interrupt table entry  49
	.word	_user_intr6;	# interrupt table entry  50
	.word	_user_intr6;	# interrupt table entry  51
	.word	_user_intr6;	# interrupt table entry  52
	.word	_user_intr6;	# interrupt table entry  53
	.word	_user_intr6;	# interrupt table entry  54
	.word	_user_intr6;	# interrupt table entry  55
	.word	_user_intr7;	# interrupt table entry  56
	.word	_user_intr7;	# interrupt table entry  57
	.word	_user_intr7;	# interrupt table entry  58
	.word	_user_intr7;	# interrupt table entry  59
	.word	_user_intr7;	# interrupt table entry  60
	.word	_user_intr7;	# interrupt table entry  61
	.word	_user_intr7;	# interrupt table entry  62
	.word	_user_intr7;	# interrupt table entry  63
	.word	_user_intr8;	# interrupt table entry  64
	.word	_user_intr8;	# interrupt table entry  65
	.word	_user_intr8;	# interrupt table entry  66
	.word	_user_intr8;	# interrupt table entry  67
	.word	_user_intr8;	# interrupt table entry  68
	.word	_user_intr8;	# interrupt table entry  69
	.word	_user_intr8;	# interrupt table entry  70
	.word	_user_intr8;	# interrupt table entry  71
	.word	_user_intr9;	# interrupt table entry  72
	.word	_user_intr9;	# interrupt table entry  73
	.word	_user_intr9;	# interrupt table entry  74
	.word	_user_intr9;	# interrupt table entry  75
	.word	_user_intr9;	# interrupt table entry  76
	.word	_user_intr9;	# interrupt table entry  77
	.word	_user_intr9;	# interrupt table entry  78
	.word	_user_intr9;	# interrupt table entry  79
	.word	_user_intr10;	# interrupt table entry  80
	.word	_user_intr10;	# interrupt table entry  81
	.word	_user_intr10;	# interrupt table entry  82
	.word	_user_intr10;	# interrupt table entry  83
	.word	_user_intr10;	# interrupt table entry  84
	.word	_user_intr10;	# interrupt table entry  85
	.word	_user_intr10;	# interrupt table entry  86
	.word	_user_intr10;	# interrupt table entry  87
	.word	_user_intr11;	# interrupt table entry  88
	.word	_user_intr11;	# interrupt table entry  89
	.word	_user_intr11;	# interrupt table entry  90
	.word	_user_intr11;	# interrupt table entry  91
	.word	_user_intr11;	# interrupt table entry  92
	.word	_user_intr11;	# interrupt table entry  93
	.word	_user_intr11;	# interrupt table entry  94
	.word	_user_intr11;	# interrupt table entry  95
	.word	_user_intr12;	# interrupt table entry  96
	.word	_user_intr12;	# interrupt table entry  97
	.word	_user_intr12;	# interrupt table entry  98
	.word	_user_intr12;	# interrupt table entry  99
	.word	_user_intr12;	# interrupt table entry 100
	.word	_user_intr12;	# interrupt table entry 101
	.word	_user_intr12;	# interrupt table entry 102
	.word	_user_intr12;	# interrupt table entry 103
	.word	_user_intr13;	# interrupt table entry 104
	.word	_user_intr13;	# interrupt table entry 105
	.word	_user_intr13;	# interrupt table entry 106
	.word	_user_intr13;	# interrupt table entry 107
	.word	_user_intr13;	# interrupt table entry 108
	.word	_user_intr13;	# interrupt table entry 109
	.word	_user_intr13;	# interrupt table entry 110
	.word	_user_intr13;	# interrupt table entry 111
	.word	_user_intr14;	# interrupt table entry 112
	.word	_user_intr14;	# interrupt table entry 113
	.word	_user_intr14;	# interrupt table entry 114
	.word	_user_intr14;	# interrupt table entry 115
	.word	_user_intr14;	# interrupt table entry 116
	.word	_user_intr14;	# interrupt table entry 117
	.word	_user_intr14;	# interrupt table entry 118
	.word	_user_intr14;	# interrupt table entry 119
	.word	_user_intr15;	# interrupt table entry 120
	.word	_user_intr15;	# interrupt table entry 121
	.word	_user_intr15;	# interrupt table entry 122
	.word	_user_intr15;	# interrupt table entry 123
	.word	_user_intr15;	# interrupt table entry 124
	.word	_user_intr15;	# interrupt table entry 125
	.word	_user_intr15;	# interrupt table entry 126
	.word	_user_intr15;	# interrupt table entry 127
	.word	_user_intr16;	# interrupt table entry 128
	.word	_user_intr16;	# interrupt table entry 129
	.word	_user_intr16;	# interrupt table entry 130
	.word	_user_intr16;	# interrupt table entry 131
	.word	_user_intr16;	# interrupt table entry 132
	.word	_user_intr16;	# interrupt table entry 133
	.word	_user_intr16;	# interrupt table entry 134
	.word	_user_intr16;	# interrupt table entry 135
	.word	_user_intr17;	# interrupt table entry 136
	.word	_user_intr17;	# interrupt table entry 137
	.word	_user_intr17;	# interrupt table entry 138
	.word	_user_intr17;	# interrupt table entry 139
	.word	_user_intr17;	# interrupt table entry 140
	.word	_user_intr17;	# interrupt table entry 141
	.word	_user_intr17;	# interrupt table entry 142
	.word	_user_intr17;	# interrupt table entry 143
	.word	_user_intr18;	# interrupt table entry 144
	.word	_user_intr18;	# interrupt table entry 145
	.word	_user_intr18;	# interrupt table entry 146
	.word	_user_intr18;	# interrupt table entry 147
	.word	_user_intr18;	# interrupt table entry 148
	.word	_user_intr18;	# interrupt table entry 149
	.word	_user_intr18;	# interrupt table entry 150
	.word	_user_intr18;	# interrupt table entry 151
	.word	_user_intr19;	# interrupt table entry 152
	.word	_user_intr19;	# interrupt table entry 153
	.word	_user_intr19;	# interrupt table entry 154
	.word	_user_intr19;	# interrupt table entry 155
	.word	_user_intr19;	# interrupt table entry 156
	.word	_user_intr19;	# interrupt table entry 157
	.word	_user_intr19;	# interrupt table entry 158
	.word	_user_intr19;	# interrupt table entry 159
	.word	_user_intr20;	# interrupt table entry 160
	.word	_user_intr20;	# interrupt table entry 161
	.word	_user_intr20;	# interrupt table entry 162
	.word	_user_intr20;	# interrupt table entry 163
	.word	_user_intr20;	# interrupt table entry 164
	.word	_user_intr20;	# interrupt table entry 165
	.word	_user_intr20;	# interrupt table entry 166
	.word	_user_intr20;	# interrupt table entry 167
	.word	_user_intr21;	# interrupt table entry 168
	.word	_user_intr21;	# interrupt table entry 169
	.word	_user_intr21;	# interrupt table entry 170
	.word	_user_intr21;	# interrupt table entry 171
	.word	_user_intr21;	# interrupt table entry 172
	.word	_user_intr21;	# interrupt table entry 173
	.word	_user_intr21;	# interrupt table entry 174
	.word	_user_intr21;	# interrupt table entry 175
	.word	_user_intr22;	# interrupt table entry 176
	.word	_user_intr22;	# interrupt table entry 177
	.word	_user_intr22;	# interrupt table entry 178
	.word	_user_intr22;	# interrupt table entry 179
	.word	_user_intr22;	# interrupt table entry 180
	.word	_user_intr22;	# interrupt table entry 181
	.word	_user_intr22;	# interrupt table entry 182
	.word	_user_intr22;	# interrupt table entry 183
	.word	_user_intr23;	# interrupt table entry 184
	.word	_user_intr23;	# interrupt table entry 185
	.word	_user_intr23;	# interrupt table entry 186
	.word	_user_intr23;	# interrupt table entry 187
	.word	_user_intr23;	# interrupt table entry 188
	.word	_user_intr23;	# interrupt table entry 189
	.word	_user_intr23;	# interrupt table entry 190
	.word	_user_intr23;	# interrupt table entry 191
	.word	_user_intr24;	# interrupt table entry 192
	.word	_user_intr24;	# interrupt table entry 193
	.word	_user_intr24;	# interrupt table entry 194
	.word	_user_intr24;	# interrupt table entry 195
	.word	_user_intr24;	# interrupt table entry 196
	.word	_user_intr24;	# interrupt table entry 197
	.word	_user_intr24;	# interrupt table entry 198
	.word	_user_intr24;	# interrupt table entry 199
	.word	_user_intr25;	# interrupt table entry 200
	.word	_user_intr25;	# interrupt table entry 201
	.word	_user_intr25;	# interrupt table entry 202
	.word	_user_intr25;	# interrupt table entry 203
	.word	_user_intr25;	# interrupt table entry 204
	.word	_user_intr25;	# interrupt table entry 205
	.word	_user_intr25;	# interrupt table entry 206
	.word	_user_intr25;	# interrupt table entry 207
	.word	_user_intr26;	# interrupt table entry 208
	.word	_user_intr26;	# interrupt table entry 209
	.word	_user_intr26;	# interrupt table entry 210
	.word	_user_intr26;	# interrupt table entry 211
	.word	_user_intr26;	# interrupt table entry 212
	.word	_user_intr26;	# interrupt table entry 213
	.word	_user_intr26;	# interrupt table entry 214
	.word	_user_intr26;	# interrupt table entry 215
	.word	_user_intr27;	# interrupt table entry 216
	.word	_user_intr27;	# interrupt table entry 217
	.word	_user_intr27;	# interrupt table entry 218
	.word	_user_intr27;	# interrupt table entry 219
	.word	_user_intr27;	# interrupt table entry 220
	.word	_user_intr27;	# interrupt table entry 221
	.word	_user_intr27;	# interrupt table entry 222
	.word	_user_intr27;	# interrupt table entry 223
	.word	_user_intr28;	# interrupt table entry 224
	.word	_user_intr28;	# interrupt table entry 225
	.word	_user_intr28;	# interrupt table entry 226
	.word	_user_intr28;	# interrupt table entry 227
	.word	_user_intr28;	# interrupt table entry 228
	.word	_user_intr28;	# interrupt table entry 229
	.word	_user_intr28;	# interrupt table entry 230
	.word	_user_intr28;	# interrupt table entry 231
	.word	_user_intr29;	# interrupt table entry 232
	.word	_user_intr29;	# interrupt table entry 233
	.word	_user_intr29;	# interrupt table entry 234
	.word	_user_intr29;	# interrupt table entry 235
	.word	_user_intr29;	# interrupt table entry 236
	.word	_user_intr29;	# interrupt table entry 237
	.word	_user_intr29;	# interrupt table entry 238
	.word	_user_intr29;	# interrupt table entry 239
	.word	_user_intr30;	# interrupt table entry 240
	.word	_user_intr30;	# interrupt table entry 241
	.word	_user_intr30;	# interrupt table entry 242
	.word	_user_intr30;	# interrupt table entry 243
	.word	_user_intr30;	# reserved
	.word	_user_intr30;	# reserved
	.word	_user_intr30;	# reserved
	.word	_user_intr30;	# reserved
	.word	_user_NMI;	# system error interrupt (MC)
				# Non-Maskable Interrupt (CA)
	.word	_user_intr31;	# reserved
	.word	_user_intr31;	# reserved
	.word	_user_intr31;	# reserved
	.word	_user_intr31;	# interrupt table entry 252
	.word	_user_intr31;	# interrupt table entry 253
	.word	_user_intr31;	# interrupt table entry 254
	.word	_user_intr31;	# interrupt table entry 255
