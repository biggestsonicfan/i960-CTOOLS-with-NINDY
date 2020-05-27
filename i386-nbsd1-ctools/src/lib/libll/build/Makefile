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

#
# Makefile for the runtime (a.k.a., low-level) library.
# Also makes crt start-up modules for mon960.
#
#	libll.a		- library, ELF format
#	libll_p.a	- library, ELF format, PID
#	libll_b.a	- library, ELF format, big-endian
#	libll_e.a	- library, ELF format, big-endian, PID
#	crt960.o	- startup module, ELF
#	crt960_p.o	- startup module, ELF, PID
#	crt960_b.o	- startup module, ELF, big-endian
#	crt960_e.o	- startup module, ELF, big-endian, PID
#

SHELL		= /bin/sh
IDIR		= -I../../../include
CP		= cp
RM		= rm -f
AR		= gar960
EXTRACT		= $(AR) -xo
AS		= gas960e
IC		= gcc960
FORMAT		= -Felf
EXTRA		= 
NOWARN		=
CFLAGS		= $(ARCH) $(OPT) $(NOWARN) $(FORMAT) $(EXTRA) $(IDIR) $(VERB)
CPP		= $(IC) $(ARCH) $(OPT) $(FORMAT) $(EXTRA) $(IDIR) -E
AS_OPT		=
ASFLAGS		= $(ARCH) $(AS_OPT)
ARFLAGS		= -r
VERB		=
BIG_ENDIAN	=
ARCH		=
TOOL		= libll

SNAPSHOT	=
LIB_DEBUG	=
LIB_INSTALL	=
INSTALL_ALL	= TRUE
GCC_OPT		= "-O4 $(LIB_DEBUG) -fsigned-char -mic3.0-compat -mstrict-align -mpic"
IC_OPT		= "-O2 $(LIB_DEBUG) -Wc,-mstrict-align -Gpc"

.PRECIOUS:	lll.a lllp.a lllb.a llle.a

.SUFFIXES:
.SUFFIXES:      .o .as .c

.c.o:
	$(IC) -j1 $(CFLAGS) -c $<

OBJS  = ver960.o _arg_ini.o _exit.o \
	access.o brk.o close.o creat.o isatty.o lseek.o open.o read.o \
	rename.o stat.o system.o time.o unlink.o write.o \
	ll_init.o _create.o _def_sig.o _map_len.o _semaph.o _stdopen.o \
	_tzset.o c_init.o c_term.o ioctl.o get_prcb.o ninobjs.o

all: do_checks crt_elf crt_elf_b \
               libll_elf libll_elf_p libll_elf_b libll_elf_e

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
	rm -f		$(LIB_INSTALL)/libll.a
	cp lll.a	$(LIB_INSTALL)/libll.a
	rm -f		$(LIB_INSTALL)/libll_p.a
	cp lllp.a	$(LIB_INSTALL)/libll_p.a
	rm -f		$(LIB_INSTALL)/libll_b.a
	cp lllb.a	$(LIB_INSTALL)/libll_b.a
	rm -f		$(LIB_INSTALL)/libll_e.a
	cp llle.a	$(LIB_INSTALL)/libll_e.a
	rm -f		$(LIB_INSTALL)/crt960.o
	cp crt960.o	$(LIB_INSTALL)/crt960.o
	rm -f		$(LIB_INSTALL)/crt960_p.o
	cp crt960_p.o	$(LIB_INSTALL)/crt960_p.o
	rm -f		$(LIB_INSTALL)/crt960_b.o
	cp crt960_b.o	$(LIB_INSTALL)/crt960_b.o
	rm -f		$(LIB_INSTALL)/crt960_e.o
	cp crt960_e.o	$(LIB_INSTALL)/crt960_e.o

objects:
	@rm -f $(OBJS)
	@if test -f $(LIB); then\
		$(EXTRACT) $(LIB);\
	fi


libll_elf:
	@$(MAKE) LIB=lll.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA=-mpid-safe NOWARN=-w ARCH=-AKA lll.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpr NOWARN=-w2 ARCH=-AKA AR=arc960 lll.a;\
	fi

libll_elf_p:
	@$(MAKE) LIB=lllp.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA=-mpid NOWARN=-w ARCH=-AKA lllp.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpd NOWARN=-w2 ARCH=-AKA AR=arc960 lllp.a;\
	fi


libll_elf_b:
	@$(MAKE) LIB=lllb.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA="-mpid-safe -G" NOWARN=-w ARCH=-ACA AS_OPT=-G lllb.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpr,be NOWARN=-w2 ARCH=-ACA AS_OPT=-G AR=arc960 lllb.a;\
	fi

libll_elf_e:
	@$(MAKE) LIB=llle.a objects
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(GCC_OPT) EXTRA="-mpid -G" NOWARN=-w ARCH=-ACA AS_OPT=-G llle.a;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) OPT=$(IC_OPT) EXTRA=-Gpd,be NOWARN=-w2 ARCH=-ACA AS_OPT=-G AR=arc960 llle.a;\
	fi

lll.a lllp.a lllb.a llle.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $?
	rm -f $(OBJS)

crt_elf:
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) ARCH=-AKA crt960.o crt960_p.o;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) ARCH=-AKA AR=arc960 crt960.o crt960_p.o;\
	fi

crt_elf_b:
	@if test $(IC) = gcc960; then\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) ARCH=-ACA BIG_ENDIAN=-G crt960_b.o crt960_e.o;\
	else\
		$(MAKE) IC=$(IC) SNAPSHOT=$(SNAPSHOT) LIB_DEBUG=$(LIB_DEBUG) VERB=$(VERB) ARCH=-ACA BIG_ENDIAN=-Gbe AR=arc960 crt960_b.o crt960_e.o;\
	fi

crt960.o crt960_b.o: crt960.s
	$(RM) tmp.c
	$(CP) crt960.s tmp.c
	$(IC) -E tmp.c > tmp.s
	$(IC) -j1 $(FORMAT) $(BIG_ENDIAN) $(ARCH) -c -o $@ tmp.s
	$(RM) tmp.c
	$(RM) tmp.s

crt960_p.o crt960_e.o: crt960_p.s
	$(RM) tmp.c
	$(CP) crt960_p.s tmp.c
	$(IC) -E tmp.c > tmp.s
	$(IC) -j1 $(FORMAT) $(BIG_ENDIAN) $(ARCH) -c -o $@ tmp.s
	$(RM) tmp.c
	$(RM) tmp.s

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

