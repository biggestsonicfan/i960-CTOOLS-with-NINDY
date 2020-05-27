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
IDIR	=
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
TOOL		= libcy

SNAPSHOT	=
LIB_DEBUG	=
LIB_INSTALL	=
INSTALL_ALL	= TRUE
GCC_OPT		= "-O4 $(LIB_DEBUG) -fsigned-char -mic3.0-compat -mstrict-align -mpic"
IC_OPT		= "-O2 $(LIB_DEBUG) -Wc,-mstrict-align -Gpc"

OBJS	=

.SUFFIXES:
.SUFFIXES:      .o .as .c

.as.o:
	@rm -f temp.c temp.s
	@$(CP) $< temp.c
	@$(CPP) temp.c > temp.s
	$(AS) -j1 $(ASFLAGS) -o $*.o temp.s
	@rm -f temp.c temp.s

.c.o:
	$(IC) -j1 $(CFLAGS) -c $<


all: do_checks

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
	rm -f		$(LIB_INSTALL)/cycx.ld
	cp cycx.ld	$(LIB_INSTALL)/cycx.ld
	rm -f		$(LIB_INSTALL)/cycxp.ld
	cp cycx.ld	$(LIB_INSTALL)/cycxp.ld
	rm -f		$(LIB_INSTALL)/cycxb.ld
	cp cycx.ld	$(LIB_INSTALL)/cycxb.ld
	rm -f		$(LIB_INSTALL)/cycxfls.ld
	cp cycxfls.ld   $(LIB_INSTALL)/cycxfls.ld
	rm -f		$(LIB_INSTALL)/cycxpfls.ld
	cp cycxfls.ld	$(LIB_INSTALL)/cycxpfls.ld
	rm -f		$(LIB_INSTALL)/cycxbfls.ld
	cp cycxfls.ld	$(LIB_INSTALL)/cycxbfls.ld
	rm -f		$(LIB_INSTALL)/cyjx.ld
	cp cycx.ld	$(LIB_INSTALL)/cyjx.ld
	rm -f		$(LIB_INSTALL)/cyjxp.ld
	cp cycx.ld	$(LIB_INSTALL)/cyjxp.ld
	rm -f		$(LIB_INSTALL)/cyjxb.ld
	cp cycx.ld	$(LIB_INSTALL)/cyjxb.ld
	rm -f		$(LIB_INSTALL)/cyjxfls.ld
	cp cycxfls.ld   $(LIB_INSTALL)/cyjxfls.ld
	rm -f		$(LIB_INSTALL)/cyjxpfls.ld
	cp cycxfls.ld	$(LIB_INSTALL)/cyjxpfls.ld
	rm -f		$(LIB_INSTALL)/cyjxbfls.ld
	cp cycxfls.ld	$(LIB_INSTALL)/cyjxbfls.ld
	rm -f		$(LIB_INSTALL)/cyhx.ld
	cp cycx.ld	$(LIB_INSTALL)/cyhx.ld
	rm -f		$(LIB_INSTALL)/cyhxfls.ld
	cp cycxfls.ld   $(LIB_INSTALL)/cyhxfls.ld
	rm -f		$(LIB_INSTALL)/cykx.ld
	cp cykx.ld	$(LIB_INSTALL)/cykx.ld
	rm -f		$(LIB_INSTALL)/cykxp.ld
	cp cykx.ld	$(LIB_INSTALL)/cykxp.ld
	rm -f		$(LIB_INSTALL)/cysx.ld
	cp cykx.ld	$(LIB_INSTALL)/cysx.ld
	rm -f		$(LIB_INSTALL)/cysxp.ld
	cp cykx.ld	$(LIB_INSTALL)/cysxp.ld
	rm -f		$(LIB_INSTALL)/mcycx.gld
	cp mcycx.gld	$(LIB_INSTALL)/mcycx.gld
	rm -f		$(LIB_INSTALL)/mcycxfls.gld
	cp mcycxfls.gld	$(LIB_INSTALL)/mcycxfls.gld
	rm -f		$(LIB_INSTALL)/mcyjx.gld
	cp mcyjx.gld	$(LIB_INSTALL)/mcyjx.gld
	rm -f		$(LIB_INSTALL)/mcyjxfls.gld
	cp mcyjxfls.gld	$(LIB_INSTALL)/mcyjxfls.gld
	rm -f		$(LIB_INSTALL)/mcyhx.gld
	cp mcyhx.gld	$(LIB_INSTALL)/mcyhx.gld
	rm -f		$(LIB_INSTALL)/mcyhxfls.gld
	cp mcyhxfls.gld	$(LIB_INSTALL)/mcyhxfls.gld
	rm -f		$(LIB_INSTALL)/mcykx.gld
	cp mcykx.gld	$(LIB_INSTALL)/mcykx.gld
	rm -f		$(LIB_INSTALL)/mcysx.gld
	cp mcysx.gld	$(LIB_INSTALL)/mcysx.gld

objects:
	@rm -f *.o
	@if test -f $(LIB); then\
		$(EXTRACT) $(LIB);\
	fi

clean:

FORCE:
