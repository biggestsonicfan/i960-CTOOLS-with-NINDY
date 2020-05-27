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
CFLAGS		= $(ARCH) $(OPT) $(FORMAT) $(EXTRA) $(IDIR) -DIEEE $(VERB)
CPP		= $(IC) $(ARCH) $(OPT) $(FORMAT) $(EXTRA) $(IDIR) -DIEEE -E
AS_OPT		=
ASFLAGS		= $(ARCH) $(AS_OPT)
ARFLAGS		= -r
VERB		=
TOOL		= libm

SNAPSHOT	=
LIB_DEBUG	=
LIB_INSTALL	=
INSTALL_ALL	= TRUE
GCC_OPT		= "-O4 $(LIB_DEBUG) -fsigned-char -mic3.0-compat -mstrict-align -mpic -w"
IC_OPT		= "-O2 $(LIB_DEBUG) -Wc,-fno-builtin,-mstrict-align, -Gpc -w2"

LIBM_GENERIC_OBJS	= ver960.o _class.o constant.o square.o\
	fabs.o fmod.o fp_exits.o _isnan.o _nan.o rem.o rmd.o round.o\
	const_dp.o copysign.o environ.o scale.o ldexp.o sinh.o cosh.o\
	tanh.o expf.o error.o _getac.o _setac.o\
	_fltprnt.o _fltscan.o difftime.o\
	afpcnvt.o atof.o ecvt.o fcvt.o gcvt.o\
	hypot.o hypot_ut.o logb.o matherr.o modf.o a_expm1.o

LIBM_LE_OBJS		= frexp.o

LIBM_BE_OBJS		= frexp_c.o

LIBM_A_OBJS		= a_log10.o a_asin.o a_acos.o a_ferr.o a_pow.o a_fpow.o\
	ipow.o ipowf.o a_tan.o a_tanf.o a_atan.o a_atanf.o a_log.o a_logf.o\
	a_exp.o a_expf.o sincos.o sincosf.o a_sqrt.o a_sqrtf.o a_sqrti.o\
	a_sqrtfi.o afpcnans.o afpcnand.o clssfsi.o clsdfsi.o clstfsi.o

LIBM_B_OBJS		= acoss.o acosd.o asins.o asind.o atan.o cos.o sin.o\
	tan.o ceilf.o ceil.o ceill.o floorf.o floor.o floorl.o exp2m1.o\
	exps.o expd.o log_sup.o logs.o logd.o loge.o pows.o powd.o sqrt.o

.PRECIOUS:	lmca.a lmka.a lmkb.a\
		lmcap.a lmkap.a lmkbp.a\
		lmcab.a lmcae.a

.SUFFIXES:
.SUFFIXES:      .o .as .c

.c.o:
	$(IC) -j1 $(CFLAGS) -c $<

.as.o:
	@rm -f temp.c temp.s
	@$(CP) $< temp.c
	$(CPP) temp.c > temp.s
	$(AS) -j1 $(ASFLAGS) -o $*.o temp.s
	@rm -f temp.c temp.s

all:	do_checks lib_elf_ca lib_elf_ka lib_elf_kb \
                  lib_elf_cap lib_elf_kap lib_elf_kbp \
	          lib_elf_cab lib_elf_cae

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
	rm -f		$(LIB_INSTALL)/libmca.a
	cp lmca.a	$(LIB_INSTALL)/libmca.a
	rm -f		$(LIB_INSTALL)/libmka.a
	cp lmka.a	$(LIB_INSTALL)/libmka.a
	rm -f		$(LIB_INSTALL)/libmkb.a
	cp lmkb.a	$(LIB_INSTALL)/libmkb.a
	rm -f		$(LIB_INSTALL)/libmca_p.a
	cp lmcap.a	$(LIB_INSTALL)/libmca_p.a
	rm -f		$(LIB_INSTALL)/libmka_p.a
	cp lmkap.a	$(LIB_INSTALL)/libmka_p.a
	rm -f		$(LIB_INSTALL)/libmkb_p.a
	cp lmkbp.a	$(LIB_INSTALL)/libmkb_p.a
	rm -f		$(LIB_INSTALL)/libmca_b.a
	cp lmcab.a	$(LIB_INSTALL)/libmca_b.a
	rm -f		$(LIB_INSTALL)/libmca_e.a
	cp lmcae.a	$(LIB_INSTALL)/libmca_e.a

objects:
	@rm -f *.o
	@if test -f $(LIB); then\
		$(EXTRACT) $(LIB);\
	fi

lib_elf_ca:
	@$(MAKE) LIB=lmca.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA=-mpid-safe ARCH=-ACA lmca.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpr ARCH=-ACA AR=arc960 lmca.a;\
	fi

lib_elf_cap:
	@$(MAKE) LIB=lmcap.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA=-mpid ARCH=-ACA lmcap.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpd ARCH=-ACA AR=arc960 lmcap.a;\
	fi

lib_elf_ka:
	@$(MAKE) LIB=lmka.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA=-mpid-safe ARCH=-AKA lmka.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpr ARCH=-AKA AR=arc960 lmka.a;\
	fi

lib_elf_kap:
	@$(MAKE) LIB=lmkap.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA=-mpid ARCH=-AKA lmkap.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpd ARCH=-AKA AR=arc960 lmkap.a;\
	fi

lib_elf_kb:
	@$(MAKE) LIB=lmkb.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA=-mpid-safe ARCH=-AKB lmkb.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpr ARCH=-AKB AR=arc960 lmkb.a;\
	fi

lib_elf_kbp:
	@$(MAKE) LIB=lmkbp.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA=-mpid ARCH=-AKB lmkbp.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpd ARCH=-AKB AR=arc960 lmkbp.a;\
	fi

lib_elf_cab:
	@$(MAKE) LIB=lmcab.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA="-mpid-safe -G" ARCH=-ACA AS_OPT=-G lmcab.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpr,be ARCH=-ACA AS_OPT=-G AR=arc960 lmcab.a;\
	fi

lib_elf_cae:
	@$(MAKE) LIB=lmcae.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA="-mpid -G" ARCH=-ACA AS_OPT=-G lmcae.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpd,be ARCH=-ACA AS_OPT=-G AR=arc960 lmcae.a;\
	fi

lmka.a \
lmkap.a \
lmca.a \
lmcap.a:	$(LIBM_GENERIC_OBJS) $(LIBM_LE_OBJS) $(LIBM_A_OBJS)
	$(AR) $(ARFLAGS) $@ $?
	rm -f *.o

lmcab.a \
lmcae.a:	$(LIBM_GENERIC_OBJS) $(LIBM_BE_OBJS) $(LIBM_A_OBJS)
	$(AR) $(ARFLAGS) $@ $?
	rm -f *.o

lmkb.a \
lmkbp.a:	$(LIBM_GENERIC_OBJS) $(LIBM_LE_OBJS) $(LIBM_B_OBJS)
	$(AR) $(ARFLAGS) $@ $?
	rm -f *.o

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

