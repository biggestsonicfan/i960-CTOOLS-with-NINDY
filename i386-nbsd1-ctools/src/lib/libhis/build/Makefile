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
IDIR		= -I../../../include
CP		= cp
IC		= gcc960
FORMAT		= -Felf
EXTRA		=
ARCH		= -AKA
#
# The code in profile.c is architecture independent.  We call it
# CA here only to keep assembler happy about produciing -G code for it.
#
CFLAGS		= $(OPT) $(ARCH) $(FORMAT) $(EXTRA) $(IDIR) $(VERB)
VERB		=

LIB_DEBUG	=
LIB_INSTALL	=
INSTALL_ALL	= TRUE
GCC_OPT		= "-O4 -w $(LIB_DEBUG) -fsigned-char -mic3.0-compat -mstrict-align -mpic"
IC_OPT		= "-O2 -w2 $(LIB_DEBUG) -Wc,-mstrict-align -Gpc"

.SUFFIXES:
.SUFFIXES:	.o .c

.c.o:
	$(IC) -j1 -c $(CFLAGS) $<

LIBHIS_OBJ = profile.o

all: do_checks libhis_elf libhis_elf_p libhis_elf_b libhis_elf_e

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
	rm -f		$(LIB_INSTALL)/libhis.a
	cp lhis.a	$(LIB_INSTALL)/libhis.a
	rm -f		$(LIB_INSTALL)/libhis_p.a
	cp lhisp.a	$(LIB_INSTALL)/libhis_p.a
	rm -f		$(LIB_INSTALL)/libhis_b.a
	cp lhisb.a	$(LIB_INSTALL)/libhis_b.a
	rm -f		$(LIB_INSTALL)/libhis_e.a
	cp lhise.a	$(LIB_INSTALL)/libhis_e.a

cleanobjects:
	@rm -f *.o

libhis_elf:
	@$(MAKE) cleanobjects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA=-mpid-safe lhis.a;\
	else\
		$(MAKE) IC=$(IC) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpr lhis.a;\
	fi

libhis_elf_p:
	@$(MAKE) cleanobjects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA=-mpid lhisp.a;\
	else\
		$(MAKE) IC=$(IC) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpd lhisp.a;\
	fi

libhis_elf_b:
	@$(MAKE) cleanobjects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) ARCH=-ACA LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA="-mpid-safe -G" lhisb.a;\
	else\
		$(MAKE) IC=$(IC) ARCH=-ACA LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpr,be lhisb.a;\
	fi

libhis_elf_e:
	@$(MAKE) cleanobjects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) ARCH=-ACA LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA="-mpid -G" lhise.a;\
	else\
		$(MAKE) IC=$(IC) ARCH=-ACA LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpd,be lhise.a;\
	fi

lhis.a lhisp.a lhisb.a lhise.a: $(LIBHIS_OBJ)
	mv $(LIBHIS_OBJ) $@

clean:
	rm -f *.a
	rm -f *.o
