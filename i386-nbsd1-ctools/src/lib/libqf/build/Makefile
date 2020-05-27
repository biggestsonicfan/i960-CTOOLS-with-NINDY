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
CP		= cp
AR		= gar960
EXTRACT		= $(AR) -xo
AS		= gas960e
IC		= gcc960
FORMAT		= -Felf
CFLAGS		= $(FORMAT) $(ARCH) $(OPT)
AS_OPT		=
ASFLAGS		= $(ARCH) $(AS_OPT)
ARFLAGS		= -r

LIB_INSTALL	=
INSTALL_ALL	= TRUE
LIB_DEBUG	=

LIBQF_OBJS	= prof_f.o

LIBQ_OBJS	= prof_nf.o

.PRECIOUS:	lqf.a lqfp.a lqfb.a lqfe.a \
		lq.a  lqp.a  lqb.a  lqe.a

.SUFFIXES:
.SUFFIXES:	.o .c

.c.o:
	$(IC) -j1 $(CFLAGS) -c $<

all:	do_checks lqf_elf lqfp_elf lqfb_elf lqfe_elf \
                  lq_elf  lqp_elf  lqb_elf  lqe_elf

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
	rm -f		$(LIB_INSTALL)/libq.a
	cp lq.a		$(LIB_INSTALL)/libq.a
	rm -f		$(LIB_INSTALL)/libq_p.a
	cp lqp.a	$(LIB_INSTALL)/libq_p.a
	rm -f		$(LIB_INSTALL)/libq_b.a
	cp lqb.a	$(LIB_INSTALL)/libq_b.a
	rm -f		$(LIB_INSTALL)/libq_e.a
	cp lqe.a	$(LIB_INSTALL)/libq_e.a
	rm -f		$(LIB_INSTALL)/libqf.a
	cp lqf.a	$(LIB_INSTALL)/libqf.a
	rm -f		$(LIB_INSTALL)/libqf_p.a
	cp lqfp.a	$(LIB_INSTALL)/libqf_p.a
	rm -f		$(LIB_INSTALL)/libqf_b.a
	cp lqfb.a	$(LIB_INSTALL)/libqf_b.a
	rm -f		$(LIB_INSTALL)/libqf_e.a
	cp lqfe.a	$(LIB_INSTALL)/libqf_e.a

objects:
	@rm -f *.o
	@if test -f $(LIB); then\
		$(EXTRACT) $(LIB);\
	fi

lqf_elf:
	@$(MAKE) LIB=lqf.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) LIB_DEBUG=$(LIB_DEBUG) OPT="-O4 -w -mstrict-align -mpic -mpid-safe" ARCH=-AKA lqf.a;\
	else\
		$(MAKE) IC=$(IC) LIB_DEBUG=$(LIB_DEBUG) OPT="-O2 -w2 -Wc,-mstrict-align -Gpc,pr" ARCH=-AKA AR=arc960 lqf.a;\
	fi

lqfp_elf:
	@$(MAKE) LIB=lqfp.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) LIB_DEBUG=$(LIB_DEBUG) OPT="-O4 -w -mstrict-align -mpic -mpid" ARCH=-AKA lqfp.a;\
	else\
		$(MAKE) IC=$(IC) LIB_DEBUG=$(LIB_DEBUG) OPT="-O2 -w2 -Wc,-mstrict-align -Gpc,pd" ARCH=-AKA AR=arc960 lqfp.a;\
	fi

lqfb_elf:
	@$(MAKE) LIB=lqfb.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) LIB_DEBUG=$(LIB_DEBUG) OPT="-O4 -w -mstrict-align -mpic -mpid-safe -G" ARCH=-ACA AS_OPT=-G lqfb.a;\
	else\
		$(MAKE) IC=$(IC) LIB_DEBUG=$(LIB_DEBUG) OPT="-O2 -w2 -Wc,-mstrict-align -Gpc,pr,be" ARCH=-ACA AS_OPT=-G AR=arc960 lqfb.a;\
	fi

lqfe_elf:
	@$(MAKE) LIB=lqfe.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) LIB_DEBUG=$(LIB_DEBUG) OPT="-O4 -w -mstrict-align -mpic -mpid -G" ARCH=-ACA AS_OPT=-G lqfe.a;\
	else\
		$(MAKE) IC=$(IC) LIB_DEBUG=$(LIB_DEBUG) OPT="-O2 -w2 -Wc,-mstrict-align -Gpc,pd,be" ARCH=-ACA AS_OPT=-G AR=arc960 lqfe.a;\
	fi

lq_elf:
	@$(MAKE) LIB=lq.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) LIB_DEBUG=$(LIB_DEBUG) OPT="-O4 -w -mstrict-align -mpic -mpid-safe" ARCH=-AKA lq.a;\
	else\
		$(MAKE) IC=$(IC) LIB_DEBUG=$(LIB_DEBUG) OPT="-O2 -w2 -Wc,-mstrict-align -Gpc,pr" ARCH=-AKA AR=arc960 lq.a;\
	fi

lqp_elf:
	@$(MAKE) LIB=lqp.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) LIB_DEBUG=$(LIB_DEBUG) OPT="-O4 -w -mstrict-align -mpic -mpid" ARCH=-AKA lqp.a;\
	else\
		$(MAKE) IC=$(IC) LIB_DEBUG=$(LIB_DEBUG) OPT="-O2 -w2 -Wc,-mstrict-align -Gpc,pd" ARCH=-AKA AR=arc960 lqp.a;\
	fi

lqb_elf:
	@$(MAKE) LIB=lqb.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) LIB_DEBUG=$(LIB_DEBUG) OPT="-O4 -w -mstrict-align -mpic -mpid-safe -G" ARCH=-ACA AS_OPT=-G lqb.a;\
	else\
		$(MAKE) IC=$(IC) LIB_DEBUG=$(LIB_DEBUG) OPT="-O2 -w2 -Wc,-mstrict-align -Gpc,pr,be" ARCH=-ACA AS_OPT=-G AR=arc960 lqb.a;\
	fi

lqe_elf:
	@$(MAKE) LIB=lqe.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) LIB_DEBUG=$(LIB_DEBUG) OPT="-O4 -w -mstrict-align -mpic -mpid -G" ARCH=-ACA AS_OPT=-G lqe.a;\
	else\
		$(MAKE) IC=$(IC) LIB_DEBUG=$(LIB_DEBUG) OPT="-O2 -w2 -Wc,-mstrict-align -Gpc,pd,be" ARCH=-ACA AS_OPT=-G AR=arc960 lqe.a;\
	fi

lqf.a lqfp.a lqfb.a lqfe.a:	$(LIBQF_OBJS)
	$(AR) $(ARFLAGS) $@ $?
	rm -f *.o

lq.a lqp.a lqb.a lqe.a:	$(LIBQ_OBJS)
	$(AR) $(ARFLAGS) $@ $?
	rm -f *.o

clean:
	rm -f *.a
	rm -f *.o
