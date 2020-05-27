#*******************************************************************************
# 
# Copyright (c) 1993 Intel Corporation
# 
# Intel hereby grants you permission to copy, modify, and distribute this
# software and its documentation.  Intel grants this permission provided
# that the above copyright notice appears in all copies and that both the
# copyright notice and this permission notice appear in supporting
# documentation.  In addition, Intel grants this permission provided that
# you prominently mark as "not part of the original" any modifications
# made to this software or documentation, and that the name of Intel
# Corporation not be used in advertising or publicity pertaining to
# distribution of the software or the documentation without specific,
# written prior permission.
# 
# Intel Corporation provides this AS IS, WITHOUT ANY WARRANTY, EXPRESS OR
# IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY
# OR FITNESS FOR A PARTICULAR PURPOSE.  Intel makes no guarantee or
# representations regarding the use of, or the results of the use of,
# the software and documentation in terms of correctness, accuracy,
# reliability, currentness, or otherwise; and you rely on the software,
# documentation and results solely at your own risk.
#
# IN NO EVENT SHALL INTEL BE LIABLE FOR ANY LOSS OF USE, LOSS OF BUSINESS,
# LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES
# OF ANY KIND.  IN NO EVENT SHALL INTEL'S TOTAL LIABILITY EXCEED THE SUM
# PAID TO INTEL FOR THE PRODUCT LICENSED HEREUNDER.
# 
#*******************************************************************************

SHELL		= /bin/sh
DIR		= ../../../include
IDIR		= -I$(DIR)
CP		= cp
AR		= gar960
EXTRACT		= $(AR) -xo
AS		= gas960e
IC		= gcc960
FORMAT		= -Felf
EXTRA		=
CFLAGS		= $(ARCH) $(OPT) $(FORMAT) $(EXTRA) $(IDIR) $(VERB)
CPP		= $(IC) $(ARCH) $(OPT) $(FORMAT) $(EXTRA) $(IDIR) -E
AS_OPT		=
ASFLAGS		= $(ARCH) $(AS_OPT)
ARFLAGS		= -r
VERB		=
TOOL		= libc

SNAPSHOT	=
LIB_DEBUG	=
LIB_INSTALL	=
INSTALL_ALL	= TRUE
GCC_OPT		= "-O4 $(LIB_DEBUG) -fsigned-char -mic3.0-compat -mstrict-align -mpic"
IC_OPT		= "-O2 $(LIB_DEBUG) -Wc,-mstrict-align -Gpc"

LIBC_GENERIC_OBJS = ver960.o _assert.o bcopy.o bcmp.o\
	bzero.o ctype.o isalnum.o isalpha.o\
	iscntrl.o isdigit.o isgraph.o\
	islower.o isprint.o ispunct.o\
	isspace.o isupper.o isxdigit.o\
	tolower.o toupper.o _tolower.o\
	_toupper.o isascii.o isodigit.o\
	_exit_in.o _stdio_i.o _thread_.o\
	init_c.o lfind.o lsearch.o\
	setjmp.o raise.o signal.o\
	_doprnt.o _doscan.o\
	_filbuf.o _flsbuf.o _getch.o\
	_putch.o clearerr.o fclose.o\
	feof.o ferror.o fflush.o\
	fgetc.o fgets.o fopen.o\
	fprintf.o fputc.o fputs.o\
	fread.o freopen.o fscanf.o\
	fseek.o ftell.o fwrite.o\
	getc.o getchar.o gets.o\
	perror.o printf.o putc.o\
	putchar.o puts.o remove.o\
	rewind.o scanf.o setbuf.o\
	setvbuf.o sprintf.o sscanf.o\
	tmpfile.o ungetc.o vfprintf.o\
	vprintf.o vsprintf.o fsetpos.o\
	fgetpos.o fcloseal.o fdopen.o\
	fgetchar.o fileno.o flushall.o\
	fputchar.o getw.o putw.o\
	rmtmp.o tmpnam.o abs.o\
	atexit.o atoi.o atol.o\
	bsearch.o div.o exit.o\
	getenv.o getopt.o labs.o\
	ldiv.o malloc.o new.o qsort.o\
	rand.o strtol.o strtoul.o\
	mblen.o mbstowcs.o mbtowc.o\
	wcstombs.o wctomb.o system.o\
	locale.o abort.o itoa.o\
	itoh.o ltoa.o ltoh.o\
	ltos.o ultoa.o utoa.o\
	strstr.o strtok.o strxfrm.o\
	strcoll.o strupr.o strerror.o\
	strdup.o strset.o strlwr.o\
	strnset.o strrev.o strspn.o\
	memicmp.o stricmp.o strnicmp.o\
	asctime.o clock.o ctime.o\
	gmtime.o localtim.o mktime.o\
	strftime.o tzset.o\
	pr_buck.o pr_end.o\
	pr_freq.o pr_start.o dispatch.o decode1.o

LIBC_KX_OBJS = $(LIBC_GENERIC_OBJS) strrchr.o strpbrk.o\
	memset.o strcspn.o memchr.o\
	memcmp.o memcpy.o strchr.o\
	strcmp.o strcpy.o strlen.o\
	strncat.o strncmp.o strncpy.o

LIBC_CA_OBJS = $(LIBC_GENERIC_OBJS) strpbrk.o\
	memset.o strcspn.o memch_ca.o\
	memcm_ca.o memcp_ca.o strch_ca.o\
	strcm_ca.o strcp_ca.o strle_ca.o\
	sncat_ca.o sncmp_ca.o sncpy_ca.o

LIBC_CAL_OBJS = strrchr.o

LIBC_CAB_OBJS = strchr_c.o

.PRECIOUS:	lcca.a lcka.a lckb.a\
		lccap.a lckap.a lckbp.a\
		lccab.a lccae.a

.SUFFIXES:
.SUFFIXES:	.o .as .c

.as.o:
	@rm -f temp.c temp.s
	@$(CP) $< temp.c
	@$(CPP) temp.c > temp.s
	$(AS) -j1 $(ASFLAGS) -o $*.o temp.s
	@rm -f temp.c temp.s

.c.o:
	$(IC) -j1 -c $(CFLAGS) $<

all:	do_checks libc_elf_ca  libc_elf_ka  libc_elf_kb \
                  libc_elf_cap libc_elf_kap libc_elf_kbp \
                  libc_elf_cab libc_elf_cae

do_checks:
	@if test $(IC) != gcc960 -a $(IC) != ic960; then\
		echo "IC must be ic960 or gcc960"; exit 1;\
	fi
	@if test "$(LIB_INSTALL)" = "." -o\
		"$(LIB_INSTALL)" = "./" -o\
		"$(LIB_INSTALL)" = "`pwd`"; then\
		echo "LIB_INSTALL can't be the generation directory"; exit 1;\
	fi

install: all
	@if test -z "$(LIB_INSTALL)"; then\
		if test -d $(G960BASE)/../lib_i960; then\
			$(MAKE) install_sub IC=$(IC) INSTALL_ALL=$(INSTALL_ALL) LIB_INSTALL=$(G960BASE)/../lib_i960;\
		else\
			$(MAKE) install_sub IC=$(IC) INSTALL_ALL=$(INSTALL_ALL) LIB_INSTALL=$(G960BASE)/lib;\
		fi;\
	else\
		$(MAKE) install_sub IC=$(IC) INSTALL_ALL=$(INSTALL_ALL) LIB_INSTALL=$(LIB_INSTALL);\
	fi

install_sub:
	@if test ! -d $(LIB_INSTALL); then mkdir $(LIB_INSTALL); fi
	rm -f		$(LIB_INSTALL)/libcca.a
	cp lcca.a	$(LIB_INSTALL)/libcca.a
	rm -f		$(LIB_INSTALL)/libcka.a
	cp lcka.a	$(LIB_INSTALL)/libcka.a
	rm -f		$(LIB_INSTALL)/libckb.a
	cp lckb.a	$(LIB_INSTALL)/libckb.a
	rm -f		$(LIB_INSTALL)/libcca_p.a
	cp lccap.a	$(LIB_INSTALL)/libcca_p.a
	rm -f		$(LIB_INSTALL)/libcka_p.a
	cp lckap.a	$(LIB_INSTALL)/libcka_p.a
	rm -f		$(LIB_INSTALL)/libckb_p.a
	cp lckbp.a	$(LIB_INSTALL)/libckb_p.a
	rm -f		$(LIB_INSTALL)/libcca_b.a
	cp lccab.a	$(LIB_INSTALL)/libcca_b.a
	rm -f		$(LIB_INSTALL)/libcca_e.a
	cp lccae.a	$(LIB_INSTALL)/libcca_e.a

objects:
	@rm -f *.o
	@if test -f $(LIB); then\
		$(EXTRACT) $(LIB);\
	fi

libc_elf_ca:
	@$(MAKE) LIB=lcca.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA=-mpid-safe ARCH=-ACA lcca.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpr ARCH=-ACA AR=arc960 lcca.a;\
	fi

libc_elf_cap:
	@$(MAKE) LIB=lccap.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA=-mpid ARCH=-ACA lccap.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpd ARCH=-ACA AR=arc960 lccap.a;\
	fi

libc_elf_ka:
	@$(MAKE) LIB=lcka.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA=-mpid-safe ARCH=-AKA lcka.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpr ARCH=-AKA AR=arc960 lcka.a;\
	fi

libc_elf_kap:
	@$(MAKE) LIB=lckap.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA=-mpid ARCH=-AKA lckap.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpd ARCH=-AKA AR=arc960 lckap.a;\
	fi

libc_elf_kb:
	@$(MAKE) LIB=lckb.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA=-mpid-safe ARCH=-AKB lckb.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpr ARCH=-AKB AR=arc960 lckb.a;\
	fi

libc_elf_kbp:
	@$(MAKE) LIB=lckbp.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA=-mpid ARCH=-AKB lckbp.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpd ARCH=-AKB AR=arc960 lckbp.a;\
	fi

libc_elf_cab:
	@$(MAKE) LIB=lccab.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA="-mpid-safe -G" ARCH=-ACA AS_OPT=-G lccab.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpr,be ARCH=-ACA AS_OPT=-G AR=arc960 lccab.a;\
	fi

libc_elf_cae:
	@$(MAKE) LIB=lccae.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA="-mpid -G" ARCH=-ACA AS_OPT=-G lccae.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpd,be ARCH=-ACA AS_OPT=-G AR=arc960 lccae.a;\
	fi

lcca.a lccap.a:	$(LIBC_CA_OBJS) $(LIBC_CAL_OBJS)
	$(AR) $(ARFLAGS) $@ $?
	rm -f *.o

lccab.a lccae.a:	$(LIBC_CA_OBJS) $(LIBC_CAB_OBJS)
	$(AR) $(ARFLAGS) $@ $?
	rm -f *.o

lcka.a lckap.a lckb.a lckbp.a:	$(LIBC_KX_OBJS)
	$(AR) $(ARFLAGS) $@ $?
	rm -f *.o

decode1.o: $(DIR)/decode.c $(DIR)/decode.h

clean:
	rm -f *.a
	rm -f *.o

ver960.o: FORCE
	rm -f ver960.c temp.c
	echo "static char library_version[]=" > ver960.c
	echo "	\"$(TOOL) `cat _version`.`cat i_minrev`\\" >> ver960.c
	if [ -z "$(SNAPSHOT)" ] ; then echo "+, `date`\\" >> ver960.c ; fi
	echo "\";"					>> ver960.c
	touch temp.c
	echo "static char compiler_version[]=\"`$(IC) -V -S temp.c 2>&1`\";" >> ver960.c 2>&1
	echo "static char assembler_version[]=\"`$(AS) -V temp.s 2>&1`\";" >> ver960.c 2>&1
	$(IC) $(CFLAGS) -c ver960.c
	rm temp.*

FORCE:

