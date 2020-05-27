
/*(c**************************************************************************** *
 * Copyright (c) 1990, 1991, 1992, 1993 Intel Corporation
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
 ***************************************************************************c)*/

/* Macros for error.c */
#define END_ERR_TBL	99999
#define	ERR_RETURN	-1
#define	OK_RETURN	0

/* macros for lnk960 for error.c */
#define		IERR "internal error"
#define		WARN 0
#define		ERR 1
#define		FATAL 2

/* Macros for usage() */
#define ARC_IDENT	0
#define ASM_IDENT	1
#define COF_IDENT	2
#define DIS_IDENT	3
#define DMP_IDENT	4
#define LNK_IDENT	5
#define MPP_IDENT	6
#define NAM_IDENT	7
#define PIC_IDENT	8
#define ROM_IDENT	9
#define SIZ_IDENT	10
#define STR_IDENT	11
#define CVT_IDENT	12

/*  Structure for error table */
typedef struct {
	int	num;
	char	*msg;
}error_struct;

/* Macros for ERRORS */
#define	NO_SUB			0	/* For no sub number */

/* File errors and memory allocation errors */
#define	NOT_OPEN			50
#define	NO_FILES			51
#define	NO_FOUND			52
#define	NOT_COFF			53
#define	CRE_FILE			54
#define	TMP_FILE			55
#define	ER_FREAD			56
#define	EXTR_FIL			57
#define	PREM_EOF			58
#define	ALLO_MEM			60
#define	ALLO_ARR			61
#define	WRIT_ERR			62
#define	NOT_COF2			63
#define	NO_TMPNM			64
#define	NOATEXIT			65
#define FILE_SEEK_ERROR         	66
#define SECT_HAS_BAD_PADDR_FILE_OFFSET	67
#define FILE_WRITE_ERROR              	68

/* Command line Option errors */
#define NSP_ARGS		70
#define OPT_ARG			71
#define ILL_OPT			72
#define	AOPT_ARG		73
#define	BD_AOPT			74
#define	EMPY_OPT		74
#define	AMBG_OPT		75
#define	OLD_OPTN		76	/* VMS only */

/* Individual tool options */
#define F_D_OPTS		80	/* dis960 only */
#define EX_FOPT			81	/* dis960 only */
#define	ONE_NOPT		86	/* dmp960 only */
#define	NO_OPT			91	/* dmp960 only */

/* Macros for libld access and other coff format referencing */
#define	RD_ARCHD		301
#define	RD_ARSYM		302
#define	RD_FILHD		303
#define	NO_OFLHD		304
#define	RD_SECHD		305
#define	NO_SECHD		306
#define	RD_SECT			307
#define	SYMTB_HD		308
#define	RD_SYMTB		309
#define	NO_SYMTB		310
#define	RD_SYMNM		311
#define	SK_SYMTB		312
#define	NO_FUNC			313
#define	RD_STRTB		314
#define	RD_AUXEN		315
#define	RD_RELOC		316
#define	RD_LINEN		317
#define	RD_LINES		318
#define	NO_LINE			319
#define	NLINE_FN		320
#define	WR_FILHD		321
#define	RD_OFLHD		322
#define	WR_OFLHD		323
#define	NO_OHDSZ		324
#define	WR_SECHD		325
#define	WR_RELOC		326
#define	WR_LINES		327
#define	RD_SCHDF		328
#define	WR_SCHDF		329
#define	RD_RLOCF		330
#define	WR_RLOCF		331
#define	RD_LINEF		332
#define	WR_LINEF		333

/* Errors peculiar to VMS */
#ifdef VMS
#define SUBPROCQUOTA		400
#define WILDCARD		401
#endif


/* Macros for indiviual tools etc. */
/* DIS960 */
#define	N_LD_SEC		501
#define	N_SYMDIS		502
#define	RD_FUNNM		503
#define	STR_BADD		504
#define	NO_TEXT			505

/* DMP960 */
#define	RD_LIBSC		525

/* COF960 */
#define AR_N_BIN		550
#define BAD_WHNC		551
#define BAD_MAGC		552
#define FIL_CNVT		553
#define FIL_SHRT		554
#define UN_OHDSZ		555
#define NAM_POOL		556
#define AFIL_CVT		557
#define BAD_ARCH		558

/* NAM960 */
#define LST_SECN		600
#define NO_PROSY		601
#define NO_REOPN		602
#define NO_SYMBS		603
#define OX_EXCLU		604
#define VN_EXCLU		605
#define ALST_SEC		606
#define ANO_SYMB		607

/* ARC960 */
#define ACT_OONE		650
#define ACT_QUAL		651
#define ARC_MALF		652
#define ARC_ORDR		653
#define ARC_SEEK		654
#define BAD_HEAD		655
#define BAD_SOFF		656
#define BAD_STRT		657
#define CPY_HEAD		658
#define CPY_POOL		659
#define ER_NAMPO		660
#define GRW_STRT		661
#define IGN_POSN		662
#define LNG_NAME		663
#define MAK_SYMB		664
#define MNY_SYMB		665
#define NO_STRTB		666
#define NOT_ARCH		667
#define NOT_FOND		668
#define NOT_OBJT		669
#define NOT_POSN		670
#define BAD_FILE_FMT	671
#define PHAS_ERR		672
#define PRE_FIVE		673
#define SKP_STRN		674
#define BAD_ADDN		675

/* PIX960 */
#define ENDCOMM			700
#define G12USED			701
#define LABL_BD			702
#define BD_SWIT			703
#define BD_CALLX		704
#define BD_BX			705
#define BD_ASM			705
#define ENDLIST			706

/* SIZ960 */
#define NOT_RD_ARHD		730
#define NOT_RD_SECHD		731
#define MULT_MODES		775

/* STR960 */
#define NOT_SEEK_LINE_NO	732
#define NOT_RD_LINE_NO		733
#define NOLOC_NSYMTB_IND	734
#define NOT_WR_LINE_NO		735
#define NOT_RD_STR_TBL		736
#define NOT_RD_REL_SEC		737
#define NOT_SEEK_REL_SEC	738
#define NOT_RD_REL		739
#define NOT_FWR			741
#define NO_INDEX_FND		742
#define MALLOC_FAIL		743
#define NOT_CPY_OPTHD		744
#define NOT_CPY_SECHD		745
#define NOT_CPY_SECS		746
#define NOT_CPY_REL_INF		747
#define NOT_CPY_SYM_TBL		748
#define NOT_CPY_EXT_SYM		749
#define NOT_CPY_REL_ENT		751
#define NOT_CPY_LINE_NO		752
#define NO_SYM_TBL		753
#define NO_LOC_SYMS		754
#define REL_ENT_NOT_STR		755
#define NOT_REC_FILEHD		756
#define NOT_REC_STR_FILE	757
#define NOT_RD_TMP_FILE		758
#define ERR_SEEK_FILE		759
#define ERR_ARHD_RD		760
#define NOT_OPEN_TMP_RD		761
#define NOT_OPEN_AR_WR		762
#define V_TO_RESTORE		763
#define TO_RESTORE		764
#define NOT_CPY_AR_NAM		765
#define NOT_REC_FILE		766
#define NOT_RD_FILE_HD		767
#define NOT_OPEN_TMP		768
#define NOT_WR_STR		769
#define NOT_CPY_ARHD		770
#define NOT_WR_TMP		771
#define NOT_REC_ARHD		772
#define BAD_COMBO_LS		773
#define BAD_COMBO_XR		774

/* ASM960 - by file in which the error is found */
/*************** addr1.c ***************/
#define AS_UNDEFTAG		800
#define AS_DIMDEB		801
#define AS_UNBSYM		802
#define AS_ILLTAG               803   
#define AS_AUXARRDIM            804
/*************** addr2.c ***************/
#define AS_RELSYM		805
#define AS_RELSYMSZ		806
#define AS_DISPOVF		807
#define AS_ILLEXP		808
#define AS_COBRLAB		809
#define AS_COBRISBR		810
#define AS_COBRDISPOVF		811
/*************** code.c ***************/
#define AS_INIBSS		812
#define AS_NOSECTCONT           813
#define AS_DUPSECT              814
#define AS_MAXSECT              815
#define AS_SECTNAM              816
#define AS_SECTATTR             817
/*************** codeout.c ***************/
#define AS_ILLCODEGEN		818
#define AS_EROFIL		819
#define AS_TMPFIL		820
#define AS_INVACT		821
#define AS_NOSCOPEENDS		822
/*************** expand1.c ***************/
#define AS_TABOVF		830
#define	AS_LABTABMALLOC         831
#define	AS_LABTABRALLOC         832
#define	AS_SDITABMALLOC         833
#define	AS_SDITABRALLOC         834
/*************** gencode.c ***************/
#define AS_ARCHINSTR		840
#define AS_BRTNOOK		841
#define AS_INSFORUNK		842
#define AS_BADMEMTYP		843
#define AS_REGREQ		844
#define AS_NOSFRREG		845
#define AS_BADALGNDST		846
#define AS_REGASMEMOP		847
#define AS_ILLUFLTREG		848
#define AS_MEMISABS		849
#define AS_ABASEINVIR		850
#define AS_SYNTAX		851
#ifdef CA_ASTEP
#define AS_CALLXOVF		852
#endif
#define AS_NOIDXBYT		853
#define AS_VFMTNODISP		854
#define AS_NOLITERAL		855
#define AS_ARCHNOSFR		856
#define AS_UNALGNREG		857
#define AS_ILL_LIT		858
#define AS_ILLFLIT		859
#define AS_LITNOINT		860
#define AS_ILLLITVAL		861
#define AS_REGORLITREQ		862
#define AS_BADPROPREG		863
#define AS_BADCTRLTARG		864
#define AS_CTRLDISPOVF		865
#define AS_ILLSFRUSE		866
#define AS_INTERNALCOBR		867
#define AS_BADCOBRTARG		868
/*************** obj.c ***************/
#define AS_OPENTEMP		870
#define AS_RELSYMBAD		871
#define AS_UNKSYM		872
#define AS_SYMSTKOVF		873
#define AS_SCOPEENDS		874
#define AS_UNKLEAFPROC		875
/*************** pass0.c ***************/
#define AS_IGNARGS		880
#define AS_ILLPIXARG            892
#define AS_OUTOVWPROT           893
#define AS_LSTOVWPROT           894
#define AS_NOTIMPLEM            895
/*************** pass1.c ***************/
#define AS_INPUTFILE		881
#define AS_TXTTEMPFILE		882
#define AS_DATATEMPFILE		883
#define AS_TEMPLISTFILE		884
#define AS_WRITEERR		885
/*************** pass2.c ***************/
#define AS_OUTPUTFILE		886
#define AS_TMPSYMFILE		887
#define AS_TMPLNNOFILE		888
#define AS_TMPRELFILE		889 
#define AS_TMPGSYMFILE		890
#define AS_LISTINGFILE		891   /*  Next in sequence is 896  */
/*************** symbols.c ***************/
#define AS_SYMTABOVF		900
#define AS_HASHOVF		901
#define AS_DUPINSTRTAB		902
#define AS_STRTABRALLOC		903
#define AS_STRTABMALLOC		904
#define AS_SYMTABRALLOC		905
#define AS_SYMTABMALLOC		906
#define AS_SYMHASHRALLOC	907
#define AS_SYMHASHMALLOC	908
/*************** symlist.c ***************/
#define AS_ALLOC		909
/*************** parse.y ***************/
#define AS_MULTDEFLAB		920
#define AS_ILLNUMLAB		921
#define AS_ASMABORTED		922
#define AS_ILLSPCSZEXP		923
#define AS_ILLREPCNT		924
#define AS_ILLSIZE		925
#define AS_ILLFILLVAL		926
#define AS_BSSNOTABS		927
#define AS_BSSALGNNOABS		928
#define AS_ILLBSSSZ		929
#define AS_ILLBSSALGN		930
#define AS_MULDEFBSSLAB		931
#define AS_ILLEXPINSET		932
#define AS_SYMDEF		933
#define AS_MULDEFLAB		934
#define AS_UNDEFLAB		935
#define AS_ILLCOMMSZ		936
#define AS_ILLREDEFSYM		937
#define AS_ILLALGNEXP		938
#define AS_ILLORGEXP		939
#define AS_ILLORGEXPTYP		940
#define AS_ABSORIGIN		941
#define AS_DECVALDOT		942
#define AS_ORGGTLIMIT		943
#define AS_ONEFILE		944
#define AS_LNARGBAD		945
#define AS_UNALGNVADDR		946
#define AS_BADSCALEF		947
#define AS_ILLFLTEXP		948
#define AS_NOEXTENDED		949
#define AS_ILLFLTSZ		950
#define AS_ILLFIXPTEXP		951
#define AS_ILLBITFLDEXP		952
#define AS_RELBITFLDEXP		953
#define AS_BITFLDXBNDRY		954
#define AS_ILLEXPOP		955
#define AS_DIVBYZERO		956
#define AS_ILLEXPOPS		957
#define AS_BADINSTMNEU		958
#define AS_BADNUMLAB		959
#define AS_LABUNDEF		960
#define AS_DUPDECPT		961
#define AS_ILLCHARCONST		962
#define AS_FLABUNDEF		963
#define AS_BADSTRING		964
#define AS_ILLINCHAR		965
#define AS_BADFILLSZ		966
#define AS_LOADDPCONST		967
#define	AS_NOI960INC		968
#define	AS_MAXI960INC		969
#define	AS_NOOUTFILE		970
#define	AS_LISTSRC		971
#define	AS_LSTOVWSRC		972
#define	AS_OUTOVWSRC		973
#define	AS_NOMPP		974
#define	AS_MPPNOTFND		975
#define	AS_CMDTOOLNG		976
#define	AS_BADCLI		977
#define	AS_MPPFAIL		978
#define AS_TOOMANYERRS		979 
#define AS_LOADXPCONST		980
#define AS_NOI_NOINFILE		981
#define AS_LONGFILE		982
#define AS_SYSPROC_IDX		983
#define AS_TOOBIGSIZE		984
#define AS_TOOBIGDIM		985
#define AS_IGNOREI		986
#define AS_BADSECTTYP		987
#define AS_BADSECTATTR		988
#define AS_BADSECTNAME		989
/*************** unused.c ****************/
#define AS_ERRUNUSED            990
#define AS_UNUSEDSYMS           991
/*************** statistics **************/
#define AS_STAT_OPEN            992

/* ROM960 */
#define	ROM_NO_CMDS		1000
#define	ROM_SILLY_ARG		1001
#define	ROM_UNSUP_EVA		1002
#define	ROM_SILLY_CMD		1003
#define	ROM_WARN_SPLIT		1004
#define	ROM_WARN_PERMUTE	1005
#define	ROM_ADDR_BOUNDS		1006
#define	ROM_PATCH_WRITE		1007
#define	ROM_READ_IMAGE		1008
#define	ROM_TOO_MANY_SECTS	1010
#define	ROM_READ_TEXT		1011
#define	ROM_NO_CLOSE		1012
#define	ROM_NO_SECTS		1013
#define	ROM_PAD_FSEEK		1014
#define	ROM_NO_IMAGE_WRITE	1015
#define	ROM_UNABLE_2_SIZE	1016
#define	ROM_UNEXPECTED_SECTYPE	1017
#define	ROM_MISSING_SECTION	1018
#define	ROM_SECTION_ORDER	1019
#define	ROM_CKSUM_ADDRS		1020
#define	ROM_CKSUM_TARGET	1021
#define ROM_ROM_WIDTH		1022
#define ROM_IMAGE_SIZE		1023
#define ROM_SHORT_READ		1024
#define ROM_MORE_TO_GO		1025
#define ROM_BAD_ROMLEN		1026
#define ROM_BAD_ROMWIDTH	1027
#define ROM_BAD_ROMCOUNT	1028
#define ROM_BAD_MEMWIDTH	1029
#define ROM_BAD_MEMLEN		1030
#define ROM_MEMW_LT_ROMW	1031
#define ROM_BAD_WIDTH_RATIO	1032
#define ROM_GET_INT_FAILED	1033
#define ROM_BAD_IHEX_MODE	1034
#define ROM_CKSUM_STRETCH	1035
#define ROM_WARN_OVERLAP    	1036
#define ROM_WARN_WRAP_ADDR  	1037
#define ROM_NOT_OBJECT		1038
#define ROM_NO_HEX_RECORDS      1039

/* MPP960 */
/* m4.c */
#define  MAXTOKSIZ  		1050
#define  ASTKOVF    		1051
#define  QUOTEDEOF  		1052
#define  COMMENTEOF 		1053
#define  ARGLSTEOF  		1054
#define  MAXI960INC  		1055
/*m4macs.c */                     
#define  MAXCOMMENT 		1056
#define  MAXQUOTE   		1057
#define  BADMACNAM  		1058
#define  MACDEFSELF 		1059
#define  INVEXPR    		1060
#define  INCNEST    		1061
/* m4.h */
#define  PSHBKOVF		1062
#define  ARGTXTOVF		1063

/* LNK960 */
#define ADDR_EXCLUDES_OWNER	1300
#define ADD_XXX_TO_MLT_OUT	1301
#define ADV_BAD_FILL		1302
#define ADV_BAD_SECT_FLAG	1303
#define ADV_B_ILL_SECT_NAME	1304
#define ADV_E_ILL_SYM_NAME	1305
#define ADV_FLAG_NEEDS_NUM	1306
#define ADV_F_NUMFORMAT		1307
#define ADV_INCLUDE		1308
#define ADV_I_REGIONS		1309
#define ADV_I_TEXT		1310
#define ADV_L2_UNSUPP		1311
#define ADV_L_2MANY		1312
#define ADV_L_LONGPATH		1313
#define ADV_L_NODIR_PATH	1314
#define ADV_L_NOPATH		1315
#define ADV_MEM_BAD_ARG		1316
#define ADV_NFLG		1317
#define ADV_NO_TARGNAME		1318
#define ADV_O_BADARG		1319
#define ADV_O_BADNAME		1320
#define ADV_P_B_FLGS		1321
#define ADV_RES_SECT_NAME	1322
#define ADV_R_S_FLGS		1323
#define ADV_U_ILL_SYM_NAME	1324
#define ALIGN_BAD_IN_CONTEXT	1325
#define ALLOC_SLOTVEC		1326
#define ARC_STRTAB_SHORT	1327
#define AUDIT_ADDR_MATCH	1328
#define AUDIT_NO_NODES		1329
#define AUX_OFLO		1330
#define AUX_OUT_OF_SEQ		1331
#define AUX_TAB_ID		1332
#define AUX_TAB_NEG		1333
#define BAD_ALIGN		1334
#define BAD_ALIGN_SECT_IN_GRP	1335
#define BAD_ALLOC		1336
#define BAD_ARCSIZ		1337
#define BAD_BOND_ESC		1338
#define BAD_B_D_DFLT_ALLOC	1339
#define BAD_CONFIG_CMD		1340
#define BAD_MAGIC		1341
#define BAD_PHY_ARG		1342
#define BAD_RELOC_TYPE		1343
#define BOND_ADDR_EXCLUDES_OWNER	1344
#define BOND_NO_ALIGN		1345
#define BOND_NO_CONFIG		1346
#define BOND_OUT_OF_BOUNDS	1347
#define BOND_OVRLAP		1348
#define BOND_TOO_BIG		1349
#define CP_RELOC_FIL		1350
#define CP_SECT			1351
#define CP_SECT_REM		1352
#define DECR_DOT		1353
#define EXPR_TERM		1354
#define FIL_SECT_TOO_BIG	1355
#define GROUP_TOO_BIG		1356
#define IFILE_NEST		1357
#define ILL_ABS_IN_PHY		1358
#define ILL_ADDR_DOT		1359
#define ILL_EXPR_OP		1360
#define ILL_USED_DOT		1361
#define IO_ERR_OUT		1362
#define LIB_NO_SYMDIR		1363
#define LIB_NO_SYMTAB		1364
#define LIB_SYMDIR_TOO_BIG	1365
#define LINE_NUM_NON_RELOC	1366
#define LIST_JUMBLE		1367
#define MEM_IGNORE		1368
#define MEM_OVRLAP		1369
#define MERG_STRING_FIL		1370
#define MISS_SECT		1371
#define MLT_SYM			1372
#define MLT_SYM_SZ		1373
#define MULT_DEF		1374
#define NOTFOUND_CRT0		1375
#define NOTFOUND_LIB_FIL	1376
#define NOTFOUND_LIB_NAME	1377
#define NOTFOUND_TARGFILE	1378
#define NOT_FINISH_WRITE	1379
#define NOT_OPEN_IN		1380
#define NOT_OPEN_TARG		1381
#define NO_ALLOC_ATTR		1382
#define NO_ALLOC_OWN		1383
#define NO_ALLOC_SECT		1384
#define NO_DFLT_ALLOC		1385
#define NO_DFLT_ALLOC_SIZ	1386
#define NO_DOT_ENTRY		1387
#define NO_ENTRY		1388
#define NO_EXEC			1389
#define NO_OUTPUT		1391
#define NO_RELOC_ENTRY		1392
#define NO_REL_FIL		1393
#define NO_REL_LIB		1394
#define NO_SEEK_RELOC_SECT	1395
#define NO_SEEK_SECT		1396
#define NO_STRTAB		1397
#define NO_TARG			1398
#define NO_TV_FILL		1399
#define NO_TV_SYM		1400
#define NO_XXX_IN_FILE		1401
#define ODD_SECT		1402
#define ONAME_TOO_BIG		1403
#define ORPH_DSECT		1404
#define OVR_AUX			1405
#define OVR_SECT		1406
#define RD_1ST_WORD		1407
#define RD_AUXEN_FIL		1408
#define RD_LIB			1409
#define RD_I960LIB		1410
#define RD_LINENO_FIL		1411
#define RD_OPEN_TV		1412
#define RD_RELOC_FIL		1413
#define RD_RELOC_F_TO_F		1414
#define RD_REL_SECT_FIL		1415
#define RD_SECHD_ENT_FIL	1416
#define RD_SECHD_FIL		1417
#define RD_SECT_FIL		1418
#define RD_STRSIZ_FIL		1419
#define RD_STR_FIL		1420
#define RD_SYMDIR		1421
#define RD_SYMDIR_SYMNUM	1422
#define RD_SYMTAB_ENT_FIL	1423
#define RD_SYMTAB_FIL		1424
#define RD_SYMTEMP_FIL		1425
#define REDEF_ABS		1426
#define REDEF_SYM		1427
#define REGION_NC_MEM		1428
#define RELOC_OFLO		1429
#define RES_IDENT		1430
#define SECT_NOT_BUILT		1431
#define SECT_TOO_BIG		1432
#define SEEK_MEM_FIL		1433
#define SKIP_MEMSTR_FIL		1434
#define SKP_AUXEN		1435
#define SK_MEM			1436
#define SK_REWIND		1437
#define SK_SYMTAB_FIL		1438
#define SPLIT_SCNS		1439
#define STMT_IGNORE		1440
#define SYM_REF_ERRS		1441
#define SYM_TAB_ID		1442
#define SYM_TAB_NEG		1443
#define SYM_TAB_OFLO		1444
#define TINY_HDR		1445
#define TOO_HARD		1446
#define TOO_MANY_RELOCS		1480
#define TV_EXCESS_ENTRIES	1447
#define TV_ILL_SYM		1448
#define TV_MULT_SLOT_FUNC	1449
#define TV_NOT_BUILT		1450
#define TV_NO_ALIGN		1451
#define TV_RANGE_TOO_NARROW	1452
#define TV_SLOTS		1453
#define TV_SYM_OUT_RANGE	1454
#define TV_UNDEF_SYM		1455
#define UNATTR_DSECT		1456
#define UNDEF_SYM		1457
#define UNIMP_VAL_DEF		1458
#define WR_OPEN_TV		1459
#define WR_SECT			1460
#define WR_STRTB		1461
#define WR_SYMTEMP_FIL		1462
#define WR_SYM_STRTB		1463
#define LD_SYNTAX		1464
#define ARCH_MISMATCH		1465
#define INTRNL_ERROR		1466
#define MULT_FLOATS		1467
#define NO_HLL_NAME		1468
#define NO_STARTUP_NAME		1469
#define NO_SYSLIB_NAME		1470
#define MULT_STARTUPS		1471
#define ADV_P_BADARG		1472
#define PIX_MISMATCH		1473
#define OUTFILE_OVERWRITES	1474
#define ADV_EMBEDDED_TSWITCH	1475
#define BAD_SYSCALL_VALUE	1476
#define MULT_SYSCALL_INDEX	1477
#define MULT_LEAFPROC_DEFS	1478
#define UNDEF_SYSCALL		1479
#define NUM_AUXS_DIFF		1481
#define TOO_MANY_LINENUMS	1482
#define MEMORY_WRAP		1483
#define ADV_NEW_SECTION_LINES	1484
#define ADV_NEW_SECTION_RELOC	1485


/* CVT960 */

/* 695 Writer Errors (hp695_960.c) */

#define HPER_SECID		1600
#define HPER_DUPSEC		1601
#define HPER_ACODE		1602
#define HPER_BADCHAR		1603
#define HPER_UNKSEC		1604
#define HPER_UNK		1605
#define HPER_BADSTYPE		1606
#define HPER_BADHPTYPE		1607
#define HPER_UNKDBLK		1608
#define HPER_BADATT		1609

#define HPER_BADHITYPE		1610
#define HPER_RECOPMOD		1611
#define HPER_TYDEFUSE		1612
#define HPER_CLSDMOD		1613
#define HPER_FCLOSE		1614
#define HPER_NOTASMATT		1615
#define HPER_SYNCNTXT		1616

/* cvt960.c */
#define	UNEXPECTED_SECTYPE	1717
#define TOO_MANY_SCNS           1718
#define NO_FILE_SYMS		1719
#define SYMCONFLICT		1720
#define DATACONFLICT		1721
#define	INVTAGNDX		1722
#define	ARGBLKARG		1723
#define	ILLREGVAL		1724
#define PIX_NOTRANS		1725
#define FILESAME		1726
#define BADAUX			1727

/* and the rest of the cvt960 errors */
#define CVT_BAD_ASW		1728
#define CVT_MV_BACK		1729
#define CVT_NO_MEM		1730
#define CVT_E_PT_ERR		1731
#define CVT_BAD_LD_ITEM		1732
#define CVT_E_PT_IMBAL		1733
#define CVT_D_SEC_SEEK		1734
#define CVT_BAD_TRAILER		1735
#define CVT_AS_RD_ERR		1736
#define CVT_BAD_695_ARCH	1737
#define CVT_ADDR_MISSING	1738
#define CVT_E_PT_MISSING	1739
#define CVT_STRING_BAD		1740
#define CVT_OFFSET_MISSING	1741
#define CVT_NO_CMP_FILE		1742
#define CVT_NOT_COFF		1743
#define CVT_NOT_695		1744
#define CVT_UNSUP_695_SEC	1745
#define CVT_BAD_STRING		1746
#define CVT_NO_OPEN_695		1747
#define CVT_TRAILER_SEEK	1748
#define CVT_SECTION_SEEK	1749
#define CVT_DATA_SEEK		1750
#define CVT_CATASTROPHE		1751
#define CVT_ERRS_FOUND		1752
#define CVT_WARNS_FOUND		1753
#define CVT_WRITE_ERR		1754
#define CVT_HP_ABORT		1755
#define CVT_SYMENT_SEEK		1756
#define CVT_DATA_CLASH		1757
#define CVT_NO_SUCH_SEC		1758

/* ZZZ is the next error number available for use */
#define ZZZ		1759
