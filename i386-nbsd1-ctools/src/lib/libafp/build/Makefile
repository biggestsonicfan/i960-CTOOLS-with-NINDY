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
EXTRA		=
CFLAGS		= $(ARCH) $(OPT) $(FORMAT) $(EXTRA) $(VERB)
CPP		= $(IC) $(ARCH) $(OPT) $(FORMAT) $(EXTRA) -E
AS_OPT		=
ASFLAGS		= $(ARCH) $(AS_OPT)
ARFLAGS		= -r
VERB		=
TOOL		= libafp

SNAPSHOT	=
LIB_DEBUG	=
LIB_INSTALL	=
INSTALL_ALL	= TRUE

GCC_OPT		= "-O4 $(LIB_DEBUG) -fsigned-char -mic3.0-compat -mstrict-align -mpic"
IC_OPT		= "-O2 $(LIB_DEBUG) -Wc,-mstrict-align -Gpc"

OBJS =  ver960.o addtf3.o afpnand.o afpnans.o afpnant.o afprrcd.o afprrcs.o \
	afprrct.o cmpdf2.o cmpsf2.o \
	cmptf2.o divtf3.o environ.o extdftf.o extsfdf.o extsftf.o \
	faultind.o faultins.o faultint.o faultiod.o faultios.o faultiot.o \
	faultovd.o faultovs.o faultovt.o faultred.o faultres.o faultret.o \
	faultund.o faultuns.o faultunt.o faultzdd.o faultzds.o faultzdt.o \
	fixdfsi.o fixsfsi.o fixtfsi.o fltsidf.o fltsisf.o fltsitf.o \
	logbdf2.o logbsf2.o logbtf2.o multf3.o opndf3.o opnsf3.o \
	remdf3.o remsf3.o remtf3.o rnddfsi.o rndsfsi.o rndtfsi.o rndudfsi.o \
	rndusfsi.o rndutfsi.o rounddf2.o roundsf2.o roundtf2.o scaledf.o \
	scalesf.o scaletf.o trndfsf.o trntfdf.o trntfsf.o trng960.o

.PRECIOUS:	lhka.a lhkap.a lhca.a lhcap.a lhjx.a lhjxp.a\
                lhcab.a lhjxb.a lhcae.a lhjxe.a

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

all:	do_checks \
        lib_elf_ca lib_elf_jx lib_elf_ka \
        lib_elf_ca_p lib_elf_jx_p lib_elf_ka_p \
	lib_elf_cab lib_elf_jxb \
        lib_elf_cae lib_elf_jxe

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
	rm -f		$(LIB_INSTALL)/libhca.a
	cp lhca.a	$(LIB_INSTALL)/libhca.a
	rm -f		$(LIB_INSTALL)/libhca_p.a
	cp lhcap.a	$(LIB_INSTALL)/libhca_p.a
	rm -f		$(LIB_INSTALL)/libhjx.a
	cp lhjx.a	$(LIB_INSTALL)/libhjx.a
	rm -f		$(LIB_INSTALL)/libhjx_p.a
	cp lhjxp.a	$(LIB_INSTALL)/libhjx_p.a
	rm -f		$(LIB_INSTALL)/libhka.a
	cp lhka.a	$(LIB_INSTALL)/libhka.a
	rm -f		$(LIB_INSTALL)/libhka_p.a
	cp lhkap.a	$(LIB_INSTALL)/libhka_p.a
	rm -f		$(LIB_INSTALL)/libhca_b.a
	cp lhcab.a	$(LIB_INSTALL)/libhca_b.a
	rm -f		$(LIB_INSTALL)/libhjx_b.a
	cp lhjxb.a	$(LIB_INSTALL)/libhjx_b.a
	rm -f		$(LIB_INSTALL)/libhca_e.a
	cp lhcae.a	$(LIB_INSTALL)/libhca_e.a
	rm -f		$(LIB_INSTALL)/libhjx_e.a
	cp lhjxe.a	$(LIB_INSTALL)/libhjx_e.a

objects:
	@rm -f *.o
	@if test -f $(LIB); then\
		$(EXTRACT) $(LIB);\
	fi

lib_elf_ca:
	@$(MAKE) LIB=lhca.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA=-mpid-safe ARCH=-ACA lhca.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpr ARCH=-ACA AR=arc960 lhca.a;\
	fi

lib_elf_ca_p:
	@$(MAKE) LIB=lhcap.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA=-mpid ARCH=-ACA lhcap.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpd ARCH=-ACA AR=arc960 lhcap.a;\
	fi

lib_elf_jx:
	@$(MAKE) LIB=lhjx.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA=-mpid-safe ARCH=-AJA lhjx.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpr ARCH=-AJA AR=arc960 lhjx.a;\
	fi

lib_elf_jx_p:
	@$(MAKE) LIB=lhjxp.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA=-mpid ARCH=-AJA lhjxp.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpd ARCH=-AJA AR=arc960 lhjxp.a;\
	fi

lib_elf_ka:
	@$(MAKE) LIB=lhka.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA=-mpid-safe ARCH=-AKA lhka.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpr ARCH=-AKA AR=arc960 lhka.a;\
	fi

lib_elf_ka_p:
	@$(MAKE) LIB=lhkap.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA=-mpid ARCH=-AKA lhkap.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpd ARCH=-AKA AR=arc960 lhkap.a;\
	fi

lib_elf_cab:
	@$(MAKE) LIB=lhcab.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA="-mpid-safe -G" ARCH=-ACA AS_OPT=-G lhcab.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpr,be ARCH=-ACA AS_OPT=-G AR=arc960 lhcab.a;\
	fi

lib_elf_jxb:
	@$(MAKE) LIB=lhjxb.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA="-mpid-safe -G" ARCH=-AJA AS_OPT=-G lhjxb.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpr,be ARCH=-AJA AS_OPT=-G AR=arc960 lhjxb.a;\
	fi

lib_elf_cae:
	@$(MAKE) LIB=lhcae.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA="-mpid -G" ARCH=-ACA AS_OPT=-G lhcae.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpd,be ARCH=-ACA AS_OPT=-G AR=arc960 lhcae.a;\
	fi

lib_elf_jxe:
	@$(MAKE) LIB=lhjxe.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA="-mpid -G" ARCH=-AJA AS_OPT=-G lhjxe.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpd,be ARCH=-AJA AS_OPT=-G AR=arc960 lhjxe.a;\
	fi

lhka.a lhca.a lhjx.a\
lhkap.a lhcap.a lhjxp.a\
lhcab.a lhjxb.a\
lhcae.a lhjxe.a:	$(OBJS)
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
