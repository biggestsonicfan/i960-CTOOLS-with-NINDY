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
AR		= gar960
EXTRACT		= $(AR) -xo
AS		= gas960e
IC		= gcc960
FORMAT		= -Felf
EXTRA		=
CFLAGS		= -ACA $(OPT) $(FORMAT) $(EXTRA) $(IDIR) $(VERB)
CPP		= $(IC) -ACA $(OPT) $(FORMAT) $(EXTRA) $(IDIR) -E
AS_OPT		=
ASFLAGS		= -ACA $(AS_OPT)
ARFLAGS		= -r
VERB		=
TOOL		= libmstub

SNAPSHOT	=
LIB_DEBUG	=
LIB_INSTALL	=
INSTALL_ALL	= TRUE
GCC_OPT		= "-O4 $(LIB_DEBUG) -fsigned-char -mic3.0-compat -mstrict-align -mpic"
IC_OPT		= "-O2 $(LIB_DEBUG) -Wc,-mstrict-align -Gpc"

OBJS		= ver960.o pr_noflt.o sc_noflt.o qtc_nofl.o _setac.o _getac.o

.SUFFIXES:
.SUFFIXES:      .o .s .as .c

.c.o:
		$(IC) -j1 $(CFLAGS) -c $<

.s.o:
		$(AS) -j1 $(ASFLAGS) $<

.PRECIOUS:	lst.a lstb.a


all: do_checks lib_elf_ca lib_elf_cab

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
	rm -f		$(LIB_INSTALL)/libmstub.a
	cp lst.a	$(LIB_INSTALL)/libmstub.a
	rm -f		$(LIB_INSTALL)/libmstb.a
	cp lst.a	$(LIB_INSTALL)/libmstb.a
	rm -f		$(LIB_INSTALL)/libmstbp.a
	cp lst.a	$(LIB_INSTALL)/libmstbp.a
	rm -f		$(LIB_INSTALL)/libmstbb.a
	cp lstb.a	$(LIB_INSTALL)/libmstbb.a
	rm -f		$(LIB_INSTALL)/libmstbe.a
	cp lstb.a	$(LIB_INSTALL)/libmstbe.a

objects:
	@rm -f *.o
	@if test -f $(LIB); then\
		$(EXTRACT) $(LIB);\
	fi

lib_elf_ca:
	@$(MAKE) LIB=lst.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA="-mpid-safe" lst.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA="-Gpr" AR=arc960 lst.a;\
	fi

lib_elf_cab:
	@$(MAKE) LIB=lstb.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA="-mpid-safe -G" AS_OPT=-G lstb.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpr,be AS_OPT=-G AR=arc960 lstb.a;\
	fi

lst.a lstb.a:	$(OBJS)
	$(AR) $(ARFLAGS) $@ $?
	rm -f *.o

clean:
	rm -f *.o *.a

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

