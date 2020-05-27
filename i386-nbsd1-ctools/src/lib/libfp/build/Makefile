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
IC		= gcc960
AS		= gas960e
AS_OPT		=
ARCH		= -AKA
ASFLAGS		= $(ARCH) $(AS_OPT)
ARFLAGS		= -r

LIB_INSTALL     =
INSTALL_ALL	= TRUE

OBJS	= dascbin.o dint.o dpfncs.o dpcnvt.o dpopns.o \
	  fascbin.o fint.o fpfncs.o fpcnvt.o fpopns.o \
	  fpem_ac.o

.SUFFIXES:
.SUFFIXES:	.o .s .c

.s.o:
		$(AS) -j1 $(ASFLAGS) $<

.c.o:
		$(IC) -j1 -c $(CFLAGS) $<

.PRECIOUS:	lf.a lfb.a

all: do_checks lib_elf lib_elf_b

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
	rm -f		$(LIB_INSTALL)/libfp.a
	cp lf.a		$(LIB_INSTALL)/libfp.a
	rm -f		$(LIB_INSTALL)/libfp_p.a
	cp lf.a		$(LIB_INSTALL)/libfp_p.a
	rm -f		$(LIB_INSTALL)/libfp_b.a
	cp lfb.a	$(LIB_INSTALL)/libfp_b.a
	rm -f		$(LIB_INSTALL)/libfp_e.a
	cp lfb.a	$(LIB_INSTALL)/libfp_e.a

objects:
	@rm -f *.o
	@if test -f $(LIB); then\
		$(EXTRACT) $(LIB);\
	fi


lib_elf:
	@$(MAKE) LIB=lf.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) lf.a;\
	else\
		$(MAKE) AR=arc960 lf.a;\
	fi

lib_elf_b:
	@$(MAKE) LIB=lfb.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) AS_OPT=-G ARCH=-ACA lfb.a;\
	else\
		$(MAKE) AS_OPT=-G ARCH=-ACA AR=arc960 lfb.a;\
	fi

lf.a lfb.a:	$(OBJS)
	$(AR) $(ARFLAGS) $@ $?
	rm -f *.o
clean:
	rm -f *.a
	rm -f *.o
