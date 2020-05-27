/*****************************************************************************
 * 		Copyright (c) 1992, Intel Corporation
 *
 * Intel hereby grants you permission to copy, modify, and 
 * distribute this software and its documentation.  Intel grants
 * this permission provided that the above copyright notice 
 * appears in all copies and that both the copyright notice and
 * this permission notice appear in supporting documentation.  In
 * addition, Intel grants this permission provided that you
 * prominently mark as not part of the original any modifications
 * made to this software or documentation, and that the name of 
 * Intel Corporation not be used in advertising or publicity 
 * pertaining to distribution of the software or the documentation 
 * without specific, written prior permission.  
 *
 * Intel Corporation does not warrant, guarantee or make any 
 * representations regarding the use of, or the results of the use
 * of, the software and documentation in terms of correctness, 
 * accuracy, reliability, currentness, or otherwise; and you rely
 * on the software, documentation and results solely at your own risk.
 *****************************************************************************/

/*  icoffdmp.h contains format strings, definitions, and tables for 
 *  printing of symbols in full formatted display. This file was added 
 *  for the namer tool. 
 *
 *  The format used depends on the display option. -d (decimal) for
 *  fmt???[0], -x  (hexadecimal) for fmt???[1] and -o (octal)
 *  for fmt???[2] and the type of options specified.
 */

/*
 * Types of aux displays
 */
#define A_ERROR		-1
#define A_CATCHALL	0
#define A_FILENAME	1
#define A_FUNCTION	2
#define A_SECTION	3
#define A_ARRAY		4
#define A_IDENT		5
#define A_TAG		6

/*
 * Flags for different display options
 */
#define F_HEXADECIMAL   0x00  /* all numbers displayed in hexadecimal */
#define F_DECIMAL       0x01     /* all numbers displayed in decimal */
#define F_OCTAL         0x02  /* all numbers displayed in octal */
#define F_EXTERNAL      0x04  /* external flag only */
#define F_UNDEFINED     0X08  /* undefined symbols only */
#define F_HEADER        0x10  /* don't display header */
#define F_FULL          0x20  /* display .text, .bss, and .data as well */
#define F_PREPEND       0x40  /* prepend filename to each display line */
#define F_TRUNCATE      0x80  /* truncate names to within column widths */

/* FORMAT STRINGS */

static char	*fmt_head[3] = {
"Name                  Value     Class        Type         Size   Line  Section\n\n",
"Name                  Value     Class        Type        Size   Line  Section\n\n",
"Name                    Value     Class      Type       Size    Line  Section\n\n"
};

static char	*fmt_offset[3] = {
			"%-20s|%10ld|%-6.6s",
			"%-20s|0x%.8lx|%-6.6s",
			"%-20s|0%.11lo|%-6.6s"
};

static char	*fmt_address[3] = {
			"%-20s|%10lu|%-6.6s",
			"%-20s|0x%.8lx|%-6.6s",
			"%-20s|0%.11lo|%-6.6s"
};


static char	*fmt_novalue[3] = {
			"%-20s|          |%-6.6s",
			"%-20s|          |%-6.6s",
			"%-20s|            |%-6.6s"
};

static char	*fmt_file[3] = {
		"%-20s|          | file |                  |      |     |\n",
		"%-20s|          | file |                 |      |     |\n",
		"%-20s|            | file |              |       |     |\n"
};

/*  print size information */
static char	*fmt_size[3] = {
			"|%6hd",
			"|0x%.4hx",
			"|0%.6ho"
};

/*  If there is no size information */
static char	*fmt_nosize[3] = {
			"|      ",
			"|      ",
			"|       "
};                     
/* function size information */
static char	*fmt_fsize[3] = {
			"|%6ld",
			"|0x%.4lx",
			"|0%.6lo"
};

/* for type information */

static char     *fmt_type[3] = {
                        "|%18s",
                        "|%17s",
                        "|%14s"
			};

static char     *fmt_etype[3] = {
                        "|%16s",
                        "|%13s",
                        "|%14s"
			};


/* STORAGE CLASS NAMES */

#define LAST_CLASS_ENTRY C_AUTOARG+1   /* largest storage class */

/*  ordinary C language storage classes (C_LASTENT is the largest) */
static  char    *s_class[LAST_CLASS_ENTRY] = {
                         	"",       /* C_NULL    */
                                "auto",       /* C_AUTO    */
                                "extern",     /* C_EXT     */
                                "static",     /* C_STAT    */
                                "reg",        /* C_REG     */
                                "extdef",     /* C_EXTDEF  */
                                "label",      /* C_LABEL   */
                                "ulabel",     /* C_ULABEL  */
                                "strmem",     /* C_MOS     */
                                "argm't",     /* C_ARG     */
                                "strtag",     /* C_STRTAG  */
                                "unmem",      /* C_MOU     */
                                "untag",      /* C_UNTAG   */
                                "typdef",     /* C_TPDEF   */
                                "ustat",      /* C_USTATIC */
                                "entag",      /* C_ENTAG   */
                                "enmem",      /* C_MOE     */
                                "regprm",     /* C_REGPARM */
                                "bitfld",     /* C_FIELD   */
                                "autoar",      /* C_AUTOARG */
			};


/*  special debugging symobol storage classes (have values beginning at 100) */
static  char    *DBGclass[] = {
                                "block",      /* C_BLOCK   */
                                "fcn",        /* C_FCN     */
                                "endstr",     /* C_EOS     */
                                "file",       /* C_FILE    */
                                "line",       /* C_LINE    */
                                "alias",      /* C_ALIAS   */
                                "hidden",     /* C_HIDDEN  */
                                /* 960 -specific classes follow */
                                "scall",      /* C_SCALL   */
                                "leafx",      /* C_LEAFEXT */
                                "optvar",     /* 109       */
                                "define",     /* 110       */
                                "pragma",     /* 111       */
                                "segment",    /* 112       */
                                "leafs",      /* C_LEAFSTAT */
                                "ss",         /* 114       */
                                "pa"          /* 115       */

				};


/*  SCLASS_STR(x)  chooses a string in sclass or DBGclass depending on size of x */
#define SCLASS_STR(x)       (x) < LAST_CLASS_ENTRY ? s_class[(x)] : DBGclass[(x) - 100]

/* TYPE LIST */

#define LAST_TYPE_ENTRY 17
#define MAX_TYPELEN 18*2   /* including parenthesis if needed        */
#define MAX_NAME    128    /* max name in the name column of display */

static char *TypeList[LAST_TYPE_ENTRY + 1] = {
	                       "",       /* T_NULL     */
			       "void",       /* T_VOID     */
			       "char",       /* T_CHAR     */
			       "short",      /* T_SHORT    */
			       "int",        /* T_INT      */
			       "long",       /* T_LONG     */
			       "float",      /* T_FLOAT    */
			       "double",     /* T_DOUBLE   */
			       "struct",     /* T_STRUCT   */
			       "union",      /* T_UNION    */
			       "enum",       /* T_ENUM     */
			       "enmem",      /* T_MOE      */
			       "Uchar",      /* T_UCHAR    */
			       "Ushort",     /* T_USHORT   */
			       "Uint",       /* T_UINT     */
			       "Ulong",      /* T_ULONG    */
			       "Ldoubl",     /* T_LNGDBL   */
			       "??????"      /* unknown    */
			};
			       
/* TYPE_STR(x) chooses a string in TypeList given a type */
#define TYPE_STR(x)  (x) < LAST_TYPE_ENTRY ? TypeList[(x)] : TypeList[LAST_TYPE_ENTRY + 1]

typedef enum number_base { decimal, hexadecimal, octal } base_type;





