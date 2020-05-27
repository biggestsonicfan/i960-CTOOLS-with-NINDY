
typedef union  {
	bfd_vma integer;
	char *name;
	int token;
	union etree_union *etree;
	asection *section;
	union  lang_statement_union **statement_ptr;
} YYSTYPE;
extern YYSTYPE yylval;
# define INT 257
# define NAME 258
# define PATHNAME 259
# define PLUSEQ 260
# define MINUSEQ 261
# define MULTEQ 262
# define DIVEQ 263
# define LSHIFTEQ 264
# define RSHIFTEQ 265
# define ANDEQ 266
# define OREQ 267
# define CKSUM 268
# define OROR 269
# define ANDAND 270
# define EQ 271
# define NE 272
# define LE 273
# define GE 274
# define LSHIFT 275
# define RSHIFT 276
# define UNARY 277
# define ALIGN_K 278
# define BLOCK 279
# define LONG 280
# define SHORT 281
# define BYTE 282
# define SECTIONS 283
# define SIZEOF_HEADERS 284
# define FORCE_COMMON_ALLOCATION 285
# define OUTPUT_ARCH 286
# define MEMORY 287
# define NOLOAD 288
# define DSECT 289
# define COPY 290
# define DEFINED 291
# define SEARCH_DIR 292
# define ENTRY 293
# define SIZEOF 294
# define ADDR 295
# define STARTUP 296
# define HLL 297
# define SYSLIB 298
# define FLOAT 299
# define NOFLOAT 300
# define CHECKSUM 301
# define GROUP 302
# define ORIGIN 303
# define FILL 304
# define LENGTH 305
# define CREATE_OBJECT_SYMBOLS 306
# define INPUT 307
# define TARGET 308
# define INCLUDE 309
# define OUTPUT 310
# define CMD_LINE_OPT 311
