#*******************************************************************************
#*
#* Copyright (c) 1993, 1994, 1995 Intel Corporation
#*
#* Intel hereby grants you permission to copy, modify, and distribute this
#* software and its documentation.  Intel grants this permission provided
#* that the above copyright notice appears in all copies and that both the
#* copyright notice and this permission notice appear in supporting
#* documentation.  In addition, Intel grants this permission provided that
#* you prominently mark as "not part of the original" any modifications
#* made to this software or documentation, and that the name of Intel
#* Corporation not be used in advertising or publicity pertaining to
#* distribution of the software or the documentation without specific,
#* written prior permission.
#*
#* Intel Corporation provides this AS IS, WITHOUT ANY WARRANTY, EXPRESS OR
#* IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY
#* OR FITNESS FOR A PARTICULAR PURPOSE.  Intel makes no guarantee or
#* representations regarding the use of, or the results of the use of,
#* the software and documentation in terms of correctness, accuracy,
#* reliability, currentness, or otherwise; and you rely on the software,
#* documentation and results solely at your own risk.
#*
#* IN NO EVENT SHALL INTEL BE LIABLE FOR ANY LOSS OF USE, LOSS OF BUSINESS,
#* LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES
#* OF ANY KIND.  IN NO EVENT SHALL INTEL'S TOTAL LIABILITY EXCEED THE SUM
#* PAID TO INTEL FOR THE PRODUCT LICENSED HEREUNDER.
#*
#******************************************************************************/
#
# $Header: /ffs/p1/dev/src/mon960/gorman/RCS/Makefile,v 1.115 1996/02/01 01:09:22 gorman Exp $
#
# This makefile is tested using unix make or Microsoft nmake.  Small changes 
# will probably work with OPUS make on DOS (see all #__OPUS__# comments). 
# NOTE: For DOS/nmake or DOS/OPUS makes copy makefile.ic or makefile.gnu to
#       makefile.

# If user doesn't specify a target help is executed.
help: FORCE
	@echo "For the first make use 'make make HOST=host TOOL=tool' to setup Makefile."
	@echo "  For HOST use 'unix' or 'dos'; for TOOL use 'intel','gnu', 'intel_elf' or 'gnu_elf'."
	@echo "Build monitors by specifing a target as in 'make cyjx'."

#-----------------------------------------------------------------------------
#	Makefile for building Mon960 monitor for various boards
#-----------------------------------------------------------------------------
# Defining the cpp symbol ALLOW_UNALIGNED causes the monitor to support
# unaligned references to memory.  For the CA, it also sets the Mask
# Unaligned Fault bit in the PRCB.  Commenting out this definition will
# cause the monitor to generate an error when the user requests an
# unaligned memory access.  For the CA, it will clear the Mask Unaligned
# Fault bit in the PRCB, causing the processor to fault if the application
# makes an unaligned reference.
#
ALLOW_UNALIGNED= -DALLOW_UNALIGNED

# The TIMER_HW symbol causes the cyclone monitor to use either
# the 85c36.c timer code on the cyclone board or the hj_timer.c timer
# code using the HX or JX on chip timers.
# Uncomment only one of the following TIMER_HW definitions.
#
## Uncomment the next two lines to use the HX or JX on chip timers
#TIMER_HW=$(TARGET)timr.o    # Use the HX and JX on chip timers
#HJ_TIMER=-DHJ_TIMER         # Used to compile ghist.c for the on chip timers
#
## Uncomment the next line to use the Cyclone board 85c36 timers
TIMER_HW=$(TARGET)8536.o     # Use the cyclone 85c36 board timers

#------------------------------------------------------------------------------
# VARIOUS OBJECT FILES FOR NO SUPPORT IN MONITOR
#------------------------------------------------------------------------------
NO_HI_OBJS= hi_stubs.o
NO_UI_OBJS= ui_stubs.o
NO_PARALLEL_OBJS= no_para.o
NO_PCI_OBJS= no_pci.o
NO_SERIAL_OBJS= no_serl.o
NO_FLASH_OBJS= no_flash.o
NO_TIMER_OBJS= no_timer.o
NO_LEDS_OBJS= no_leds.o
NO_POST_OBJS= no_post.o
NO_APLINK_OBJS= no_aplnk.o

HI_OBJS= hi_main.o hi_rt.o
UI_OBJS= ui_main.o parse.o ui_break.o convert.o dis.o disp_mod.o download.o \
	trace.o io.o ui_rt.o perror.o
PARALLEL_OBJS=$(TARGET)para.o
PCI_OBJS=$(TARGET)_pci.o
FLASH_OBJS=$(TARGET)flsh.o 
SERIAL_OBJS=serial.o
TIMER_OBJS=$(TARGET)time.o $(TARGET)btim.o $(TARGET)ghis.o
POST_OBJS=$(C145_POST_OBJS)    

#--------------- Host-specific definitions ----------------------------
# Host specific definitions -- the proper lines are uncommented by making
# the target "make".  See the bottom of this file.
#--------------- Defaults ---------------------------------------------
CP = cp
RM = rm -f
MV = mv -f
#__dos__#CP = copy
#__dos__#RM = del
#__dos__#MV = rename

#------------------------------------------------------------------------------
# For DOS, we have to overcome the 127-character limit for command lines.
# So, the CFLAGS settings are all written into a temporary file, called a
# "response file".  Then, on the compiler command line, we need only specify
# the name of the response file, and all of the CFLAGS settings will be read
# from the file.
# On hosts other than DOS, CFLAGS settings are specified right on the
# command line, as normal.
#------------------------------------------------------------------------------
COMM_DIR= ../../hdilcomm/common
HDI_IDIR= ../../hdil/common

ROM= rom960
BE_ASM_LD_OPT=-G  # used only for big-endian targets (eg, "epcxbe")

#__intel__#TOOLSET=itl
#__intel__#CC= ic960 -A$(ARCH) $(BIGEND_C)
#__intel__#CC_FP= ic960 -A$(FP_ARCH) $(BIGEND_C)
#__intel__#CC_OPTS= -Gpr -w0 -Fnoai -O2 -Wc,-mstrict-align,-fno-unroll-loops
#__intel__#ARCH_DMA= _DMA
#__intel__#AS= asm960 -A$(ARCH) $(BE_ASM_LD)
#__intel__#AS_FP= asm960 -A$(FP_ARCH) $(BE_ASM_LD)
#__intel__#AS_EMIT= -E
#__intel__#LD= lnk960 -A$(ARCH) -A$(FP_ARCH) -z $(BE_ASM_LD) # -z = no dates in objects
#__intel__#LIBCKA=-l libcka
#__intel__#COF= cof960 -lpz
#__intel__#BE_C_OPT=-Gbe  # used only for big-endian targets (eg, "epcxbe")
#__intel__##__dos__#DOS_LNK=@  # used only for DOS big-endian possible targets

#__gnu__#TOOLSET=gnu
#__gnu__#CC= gcc960 -A$(ARCH) $(BIGEND_C)
#__gnu__#CC_FP= gcc960 -A$(FP_ARCH) $(BIGEND_C)
#__gnu__#CC_OPTS= -mpid-safe -Fcoff -Wall -O4 -fno-inline-functions -mstrict-align -fno-unroll-loops -mic3.0-compat
#__gnu__#ARCH_DMA= _DMA
#__gnu__#AS= gas960c -A$(ARCH) $(BE_ASM_LD)
#__gnu__#AS_FP= gas960c -A$(FP_ARCH) $(BE_ASM_LD)
#__gnu__#AS_EMIT= -E
#__gnu__#LD= gld960 -A$(ARCH) -A$(FP_ARCH) -Fcoff -z $(BE_ASM_LD) # -z = no dates in objects
#__gnu__#LIBCKA=-l libcka
#__gnu__#COF= objcopy -lpz
#__gnu__#BE_C_OPT=-G  # used only for big-endian targets (eg, "epcxbe")

#__intel_elf__#TOOLSET=ite
#__intel_elf__#CC= ic960 -Felf -A$(ARCH) $(BIGEND_C)
#__intel_elf__#CC_FP= ic960 -Felf -A$(FP_ARCH) $(BIGEND_C)
#__intel_elf__#CC_OPTS= -Gpr -w0 -Fnoai -O2 -Wc,-mstrict-align,-fno-unroll-loops
#__intel_elf__#ARCH_DMA= _DMA
#__intel_elf__#AS= gas960e -A$(ARCH) $(BE_ASM_LD)
#__intel_elf__#AS_FP= gas960e -A$(FP_ARCH) $(BE_ASM_LD)
#__intel_elf__#AS_EMIT= -E
#__intel_elf__#LD= lnk960 -Felf -A$(ARCH) -A$(FP_ARCH) -z $(BE_ASM_LD) # -z = no dates in objects
#__intel_elf__#LIBCKA=-l libcka
#__intel_elf__#COF= cof960 -lpz
#__intel_elf__#BE_C_OPT=-Gbe  # used only for big-endian targets (eg, "epcxbe")
#__intel_elf__##__dos__#DOS_LNK=@  # used only for DOS big-endian possible targets

#__gnu_elf__#TOOLSET=gne
#__gnu_elf__#CC= gcc960 -A$(ARCH) $(BIGEND_C)
#__gnu_elf__#CC_FP= gcc960 -A$(FP_ARCH) $(BIGEND_C)
#__gnu_elf__#CC_OPTS= -mpid-safe -Felf -Wall -O4 -fno-inline-functions -mstrict-align -fno-unroll-loops -mic3.0-compat
#__gnu_elf__#ARCH_DMA= _DMA
#__gnu_elf__#AS= gas960e -A$(ARCH) $(BE_ASM_LD)
#__gnu_elf__#AS_FP= gas960e -A$(FP_ARCH) $(BE_ASM_LD)
#__gnu_elf__#AS_EMIT= -E
#__gnu_elf__#LD= gld960 -A$(ARCH) -A$(FP_ARCH) -Felf -z $(BE_ASM_LD) # -z = no dates in objects
#__gnu_elf__#LIBCKA=-l libcka
#__gnu_elf__#COF= objcopy -lpz
#__gnu_elf__#BE_C_OPT=-G  # used only for big-endian targets (eg, "epcxbe")

CCC_OPTS= -DTARGET -I. -I$(HDI_IDIR)
#__dos__#CC_OPTS_FILE=options.cl
#__dos__#CC_OPTIONS = @$(CC_OPTS_FILE)
#__unix__#CC_OPTIONS= $(CC_OPTS) $(CCC_OPTS)

# Essential under System V, harmless elsewhere
SHELL = /bin/sh

# Tells 'make' about .s suffix for files.
# This is only needed for some 'make's (e.g., HP Apollo 9000/400, and 
# some DOS 'make' utilities).  It shouldn't hurt others.
.SUFFIXES: .o .s .c

#------------------------------------------------------------------------------
# Special TARGETS to keep directory clean and set up correctly
#------------------------------------------------------------------------------
# Remove all files created during make TARGET.
clean: FORCE
	$(MAKE) small
#__dos__#	$(RM) *.fls
#__dos__#	$(RM) *.hex
#__dos__#	$(RM) *.ima
#__dos__#	$(RM) cy?x
#__dos__#	$(RM) cy?xbe
#__dos__#	$(RM) epcx
#__dos__#	$(RM) epcxbe
#__unix__#	$(RM) epcx epcxbe cy?x cy?xbe *.fls *.ima *.hex
	$(MAKE) rmtemps

# Remove only the object files so we can switch makes between targets.
small: FORCE
	$(RM) *.o
	$(RM) __*.???

# Each make TARGET calls make cmdopt to set up to build a new TARGET file. 
cmdopt: FORCE
#__dos__##__bat__#	:$(TARGET)$(BE)
	$(RM) $(TARGET)$(BE)
	$(RM) $(TARGET)$(BE).o
	$(RM) $(TARGET)$(BE).fls
	$(RM) $(TARGET)$(BE).hex
	$(RM) $(TARGET)$(BE).ima
	$(RM) this_hw.h
	$(CP) $(TARGET).h this_hw.h
#__dos__#	echo > tmp.c
#__dos__#	$(RM) $(CC_OPTS_FILE)
#__dos__#	echo $(CC_OPTS) > $(CC_OPTS_FILE)
#__dos__#	echo $(CCC_OPTS) >> $(CC_OPTS_FILE)
	$(MAKE) __$(TARGET)$(BE).$(TOOLSET)

# Each make TARGET calls make rmtemps to clean up after building a TARGET file. 
rmtemps: FORCE
#__dos__#	$(RM) tmp.?
#__dos__#	$(RM) out.??
#__dos__#	$(RM) *.iee
#__dos__#	$(RM) this_hw.h
#__dos__#	$(RM) lst_????.dos
#__dos__#	$(RM) $(CC_OPTS_FILE)
#__dos__##__bat__#	goto end
#__unix__#	$(RM) tmp.? this_hw.h out.?? *.iee

# Build all supported mon960 monitors.
all: FORCE
#__dos__##__bat__#	:clean
	$(MAKE) clean
#__dos__##__bat__#	goto end
	$(MAKE) cykx
	$(MAKE) cysx
	$(MAKE) cycx
	$(MAKE) epcx
	$(MAKE) cyhx
	$(MAKE) cyjx
	$(MAKE) cycxbe
	$(MAKE) epcxbe
	$(MAKE) cyhxbe
	$(MAKE) cyjxbe
#__dos__##__bat__#	:small
	$(MAKE) small

# Build mon960 DOS BAT file for all monitors in a $(TOOLSET).
# DO a make clean before 'make bat', to ensure bat file is complete.
#
#__dos__##__bat__#bat: FORCE
#__dos__##__bat__#	goto %1
#__dos__##__bat__#	$(MAKE) all
#__dos__##__bat__#	:end

#------------------------------------------------------------------------------
# Standard rules
#------------------------------------------------------------------------------
.c.o:
	$(CC) $(CC_OPTIONS) -c $*.c

.s.o:
	$(RM) tmp.?
	$(CP) $< tmp.c
	$(CC) $(CC_OPTIONS) $(AS_EMIT) tmp.c > tmp.s
	$(AS) -o $*.o tmp.s

#------------------------------------------------------------------------------
# Nested header file dependencies
#------------------------------------------------------------------------------
I960_H     = $(HDI_IDIR)/hdi_regs.h std_defs.h i960.h
RETARGET_H = $(I960_H) $(HDI_IDIR)/common.h retarget.h
MON960_H   = $(HDI_IDIR)/common.h $(I960_H) mon960.h

#------------------------------------------------------------------------------
# BASE OBJECT FILES
#	Object files that are neither board- nor architecture-specific
#------------------------------------------------------------------------------
# init.o must be first in link line for KX/SX to ensure boot code at addr 0  
# WARNING: autobaud must not be moved because if its moves off an instruction 
#          cache boundry autobaud may no longer connect.  Symptom on cyclone
#          board is dark green run light instead of light green after boot.
FIRST_OBJS= init.o main.o
BASE_OBJS= autobaud.o $(SERIAL_OBJS) monitor.o entry.o fault.o go_user.o mem.o \
    break.o $(TARGET)prcb.o $(TARGET)_hw.o runtime.o asm_supp.o \
    aplnksup.o fastdown.o version.o commcfg.o unimplmt.o

# Note: autobaud.o is in the BASE_OBJS list above.
COMM_OBJS= com.o packet.o crcarray.o # autobaud.o

#------------------------------------------------------------------------------
# i960 PROCESSOR ARCHITECTURE-SPECIFIC OBJECT FILES
#   note: the KX_OBJS apply to KA, KB, SA, and SB processors, while the
#   CA_OBJS apply to the CA and CF processors, JX for the JA... processors.
#------------------------------------------------------------------------------
CA_BASE_OBJS= $(BASE_OBJS) ca.o ca_break.o ca_step.o no_float.o
CA_HI_OBJS= $(HI_OBJS) $(COMM_OBJS) ca_cpu.o
CA_UI_OBJS= $(UI_OBJS) no_fp.o

HX_BASE_OBJS= $(BASE_OBJS) hx.o hx_break.o hx_step.o no_float.o
HX_HI_OBJS= $(HI_OBJS) $(COMM_OBJS) hx_cpu.o
HX_UI_OBJS= $(UI_OBJS) no_fp.o

JX_BASE_OBJS= $(BASE_OBJS) jx.o jx_break.o jx_step.o no_float.o
JX_HI_OBJS= $(HI_OBJS) $(COMM_OBJS) jx_cpu.o
JX_UI_OBJS= $(UI_OBJS) no_fp.o

KX_BASE_OBJS= $(BASE_OBJS) kx.o kx_break.o float.o
KX_HI_OBJS= $(HI_OBJS) $(COMM_OBJS) kx_cpu.o
KX_UI_OBJS= $(UI_OBJS) fp.o ui_float.o

#------------------------------------------------------------------------------
# Common C145 base board objects
#------------------------------------------------------------------------------
COMMON_C145_OBJS= $(TARGET)_552.o $(TARGET)_eep.o c145_asm.o \
	$(TARGET)base.o $(TARGET)ldsw.o $(PARALLEL_OBJS) $(PCI_OBJS) \
	$(TIMER_OBJS) $(TIMER_HW) $(NO_APLINK_OBJS)

C145_POST_OBJS= $(TARGET)test.o cyt_mem.o cyt_io.o cyt_asm.o $(TARGET)_cio.o \
	cyt_ethr.o $(TARGET)tpci.o $(TARGET)intr.o cyt_uart.o cyt_menu.o

#------------------------------------------------------------------------------
# Supported TARGETS
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# EPCX Evaluation Board (EP80960CX) (i960 CA/CF processor)
#------------------------------------------------------------------------------
EPCX_ROM= monepcx
EPCX_LD= -T./$(EPCX_ROM)
EPCX_BOARD_OBJS= epcxldsw.o epcxpara.o $(NO_FLASH_OBJS) epcx_552.o $(NO_PCI_OBJS) \
                 $(TARGET)time.o $(TARGET)btim.o $(TARGET)ghis.o 82c54.o 
EPCX_POST_OBJS= epcxpdrv.o epcxp_m0.o epcxp_m1.o epcxptim.o epcxperr.o
EPCX_OBJS= $(CA_BASE_OBJS) $(CA_HI_OBJS) $(CA_UI_OBJS) \
	$(EPCX_BOARD_OBJS) $(NO_POST_OBJS) $(NO_APLINK_OBJS)
# The following are merely for overcoming DOS's 127-char cmd length limit.
#__unix__#LIST_EPCX= $(EPCX_OBJS)
#__dos__#LIST_EPCX= lst_epcx.dos

epcxbe: FORCE
	$(MAKE) cmdopt TARGET=epcx BE=be
	$(MAKE) epcxreal BE_ASM_LD=$(BE_ASM_LD_OPT) TARGET=epcx \
			BIGEND_C=$(BE_C_OPT) BE=be ARCH=CA$(ARCH_DMA) FP_ARCH=CA$(ARCH_DMA)

epcx:	FORCE
	$(MAKE) cmdopt ARCH=CA$(ARCH_DMA) TARGET=epcx
	$(MAKE) ARCH=CA$(ARCH_DMA) FP_ARCH=CA$(ARCH_DMA) TARGET=epcx epcxreal 

epcxreal: $(LIST_EPCX) $(FIRST_OBJS) epcxcibr.o
	$(LD) -o epcx$(BE).o -r $(DOS_LNK)$(LIST_EPCX)
	$(LD) -o epcx$(BE) $(EPCX_LD) $(FIRST_OBJS) epcx$(BE).o epcxcibr.o
	$(COF) epcx$(BE)
	$(ROM) $(EPCX_ROM) epcx$(BE)
	$(MAKE) rmtemps

# HEADER-FILE DEPENDENCIES
epcx_hw.o: epcx.h $(I960_H) $(HDI_IDIR)/hdi_arch.h $(HDI_IDIR)/common.h

epcxpdrv.o: epcxptst.h $(HDI_IDIR)/common.h epcx.h $(RETARGET_H) 16552.h
epcxp_m0.o epcxp_m1.o epcxperr.o: epcxptst.h
epcxptim.o: epcxptst.h 82c54.h epcx.h

# The following are merely for overcoming DOS's 127-char cmd length limit.
# For other hosts, these are "no-ops".
lst_epcx.dos: $(EPCX_OBJS)
#__dos__#	del lst_epcx.dos
#	The following line is for Microsoft's NMAKE:
#__dos__#	!echo $** >> lst_epcx.dos
#__dos__##	For OPUS MAKEL, comment out the above line, and
#__dos__##	uncomment the 3 lines of the following 'foreach' loop:
#__dos__##__opus__#	!foreach name $(EPCX_OBJS)
#__dos__##__opus__#		echo $(name) >> lst_epcx.dos
#__dos__##__opus__#	!end

#------------------------------------------------------------------------------
# C145/146 Evaluation Board (i960 JX processor)
#------------------------------------------------------------------------------
CYHX_ROM= moncyhx
CYHX_LD= -T./$(CYHX_ROM)
CYHX_OBJS= $(HX_BASE_OBJS) $(HX_HI_OBJS) $(HX_UI_OBJS) $(FLASH_OBJS) \
	$(COMMON_C145_OBJS) $(POST_OBJS)
# The following are merely for overcoming DOS's 127-char cmd length limit.
#__unix__#LIST_CYHX= $(CYHX_OBJS)
#__dos__#LIST_CYHX= lst_cyhx.dos

cyhxbe: FORCE
	$(MAKE) cmdopt TARGET=cyhx BE=be
	$(MAKE) cyhxreal BE_ASM_LD=$(BE_ASM_LD_OPT) TARGET=cyhx \
			BIGEND_C=$(BE_C_OPT) BE=be ARCH=HA FP_ARCH=HA

cyhx:	FORCE
	$(MAKE) cmdopt TARGET=cyhx
	$(MAKE) ARCH=HA FP_ARCH=HA TARGET=cyhx cyhxreal 

cyhxreal: $(LIST_CYHX) $(FIRST_OBJS) cyhxhibr.o
	$(LD) -o cyhx$(BE).o -r $(DOS_LNK)$(LIST_CYHX)
	$(LD) -o cyhx$(BE) $(CYHX_LD) $(FIRST_OBJS) cyhx$(BE).o cyhxhibr.o 
	$(COF) cyhx$(BE)
	$(CP)  cyhx$(BE) cyhx$(BE).fls
	$(ROM) $(CYHX_ROM) cyhx$(BE)
	$(MAKE) rmtemps

# HEADER-FILE DEPENDENCIES
cyhx_hw.o: cyhx.h c145.h $(I960_H) cyt_intr.h c145_eep.h $(COMM_DIR)/cyc9060.h\
		$(HDI_IDIR)/hdi_arch.h $(HDI_IDIR)/common.h
	$(CC) $(CC_OPTIONS) -I$(COMM_DIR) -o $@  -c cyhx_hw.c

# The following are merely for overcoming DOS's 127-char cmd length limit.
# For other hosts, these are "no-ops".
lst_cyhx.dos: $(CYHX_OBJS)
#__dos__#	del lst_cyhx.dos
#	The following line is for Microsoft's NMAKE:
#__dos__#	!echo $** >> lst_cyhx.dos
#__dos__##	For OPUS MAKEL, comment out the above line, and
#__dos__##	uncomment the 3 lines of the following 'foreach' loop:
#__dos__##__opus__#	!foreach name $(CYHX_OBJS)
#__dos__##__opus__#		echo $(name) >> lst_cyhx.dos
#__dos__##__opus__#	!end

#------------------------------------------------------------------------------
# C145/146 Evaluation Board (i960 JX processor)
#------------------------------------------------------------------------------
CYJX_ROM= moncyjx
CYJX_LD= -T./$(CYJX_ROM)
CYJX_OBJS= $(JX_BASE_OBJS) $(JX_HI_OBJS) $(JX_UI_OBJS) $(FLASH_OBJS) \
	$(COMMON_C145_OBJS) $(POST_OBJS)
# The following are merely for overcoming DOS's 127-char cmd length limit.
#__unix__#LIST_CYJX= $(CYJX_OBJS)
#__dos__#LIST_CYJX= lst_cyjx.dos

cyjxbe: FORCE
	$(MAKE) cmdopt TARGET=cyjx BE=be
	$(MAKE) cyjxreal BE_ASM_LD=$(BE_ASM_LD_OPT) TARGET=cyjx \
			BIGEND_C=$(BE_C_OPT) BE=be ARCH=JA FP_ARCH=JA

cyjx:	FORCE
	$(MAKE) cmdopt TARGET=cyjx
	$(MAKE) ARCH=JA FP_ARCH=JA TARGET=cyjx cyjxreal 

cyjxreal: $(LIST_CYJX) $(FIRST_OBJS) cyjxjibr.o
	$(LD) -o cyjx$(BE).o -r $(DOS_LNK)$(LIST_CYJX)
	$(LD) -o cyjx$(BE) $(CYJX_LD) $(FIRST_OBJS) cyjx$(BE).o cyjxjibr.o 
	$(COF) cyjx$(BE)
	$(CP)  cyjx$(BE) cyjx$(BE).fls
	$(ROM) $(CYJX_ROM) cyjx$(BE)
	$(MAKE) rmtemps

# HEADER-FILE DEPENDENCIES
cyjx_hw.o: cyjx.h c145.h $(MON960_H) $(COMM_DIR)/cyc9060.h\
		$(HDI_IDIR)/hdi_arch.h
	$(CC) $(CC_OPTIONS) -I$(COMM_DIR) -o $@  -c cyjx_hw.c

# The following are merely for overcoming DOS's 127-char cmd length limit.
# For other hosts, these are "no-ops".
lst_cyjx.dos: $(CYJX_OBJS)
#__dos__#	del lst_cyjx.dos
#	The following line is for Microsoft's NMAKE:
#__dos__#	!echo $** >> lst_cyjx.dos
#__dos__##	For OPUS MAKEL, comment out the above line, and
#__dos__##	uncomment the 3 lines of the following 'foreach' loop:
#__dos__##__opus__#	!foreach name $(CYJX_OBJS)
#__dos__##__opus__#		echo $(name) >> lst_cyjx.dos
#__dos__##__opus__#	!end

       
#------------------------------------------------------------------------------
# C145/146 Evaluation Board (i960 CA/CF processor)
#------------------------------------------------------------------------------
CYCX_ROM= moncycx
CYCX_LD= -T./$(CYCX_ROM)
CYCX_OBJS= $(CA_BASE_OBJS) $(CA_HI_OBJS) $(CA_UI_OBJS) $(FLASH_OBJS) \
	$(COMMON_C145_OBJS) $(POST_OBJS)
# The following are merely for overcoming DOS's 127-char cmd length limit.
#__unix__#LIST_CYCX= $(CYCX_OBJS)
#__dos__#LIST_CYCX= lst_cycx.dos

cycxbe: FORCE
	$(MAKE) cmdopt TARGET=cycx BE=be
	$(MAKE) cycxreal BE_ASM_LD=$(BE_ASM_LD_OPT) TARGET=cycx \
			BIGEND_C=$(BE_C_OPT) BE=be ARCH=CA$(ARCH_DMA) FP_ARCH=CA$(ARCH_DMA)

cycx: FORCE
	$(MAKE) cmdopt TARGET=cycx
	$(MAKE) ARCH=CA$(ARCH_DMA) FP_ARCH=CA$(ARCH_DMA) TARGET=cycx cycxreal 

cycxreal: $(LIST_CYCX) $(FIRST_OBJS) cycxcibr.o
	$(LD) -o cycx$(BE).o -r $(DOS_LNK)$(LIST_CYCX)
	$(LD) -o cycx$(BE) $(CYCX_LD) $(FIRST_OBJS) cycx$(BE).o cycxcibr.o 
	$(COF) cycx$(BE)
	$(CP)  cycx$(BE) cycx$(BE).fls
	$(ROM) $(CYCX_ROM) cycx$(BE)
	$(MAKE) rmtemps

# HEADER-FILE DEPENDENCIES
cycx_hw.o: cycx.h $(I960_H) c145.h c145_eep.h $(COMM_DIR)/cyc9060.h\
		$(HDI_IDIR)/hdi_arch.h $(HDI_IDIR)/common.h
	$(CC) $(CC_OPTIONS) -I$(COMM_DIR) -o $@  -c cycx_hw.c

# The following are merely for overcoming DOS's 127-char cmd length limit.
# For other hosts, these are "no-ops".
lst_cycx.dos: $(CYCX_OBJS)
#__dos__#	del lst_cycx.dos
#	The following line is for Microsoft's NMAKE:
#__dos__#	!echo $** >> lst_cycx.dos
#__dos__##	For OPUS MAKEL, comment out the above line, and
#__dos__##	uncomment the 3 lines of the following 'foreach' loop:
#__dos__##__opus__#	!foreach name $(CYCX_OBJS)
#__dos__##__opus__#		echo $(name) >> lst_cycx.dos
#__dos__##__opus__#	!end

#------------------------------------------------------------------------------
# C145/146 Evaluation Board (i960 SX/KX processor)
#------------------------------------------------------------------------------
CYKX_ROM= mon$(TARGET)
CYKX_LD= -T./$(CYKX_ROM)
CYKX_BOARD_OBJS= no_flash.o
CYKX_OBJS=  $(KX_BASE_OBJS) $(KX_HI_OBJS) $(COMMON_C145_OBJS)  \
			$(CYKX_BOARD_OBJS) $(KX_UI_OBJS) $(POST_OBJS)
# The following are merely for overcoming DOS's 127-char cmd length limit.
#__unix__#LIST_CYKX= $(CYKX_OBJS)
#__dos__#LIST_CYKX= lst_cykx.dos

cykx: FORCE
	$(RM)   cykx_*.hex
	$(MAKE) cmdopt TARGET=cykx
	$(MAKE) ARCH=KA FP_ARCH=KB TARGET=cykx cykxreal 

cysx: FORCE
	$(RM)   cysx_*.hex
	$(MAKE) cmdopt TARGET=cysx
	$(MAKE) ARCH=SA FP_ARCH=SB TARGET=cysx cykxreal 

# init.o must be first in link line to ensure boot code at addr 0
cykxreal: $(LIST_CYKX) $(FIRST_OBJS)
	$(LD) -o $(TARGET).o -r $(LIST_CYKX) $(LIBCKA)
	$(LD) -o $(TARGET) $(CYKX_LD) $(FIRST_OBJS) $(TARGET).o
	$(COF) $(TARGET)
	$(ROM)  $(CYKX_ROM) $(TARGET)
	$(MAKE) rmtemps

# HEADER-FILE DEPENDENCIES
$(TARGET)_hw.o: $(TARGET).h $(MON960_H) $(RETARGET_H) c145.h c145_eep.h \
		$(HDI_IDIR)/hdi_arch.h

# The following are merely for overcoming DOS's 127-char cmd length limit.
# For other hosts, these are "no-ops".
lst_cykx.dos: $(CYKX_OBJS)
#__dos__#	del lst_cykx.dos
#	The following line is for Microsoft's NMAKE:
#__dos__#	!echo $** >> lst_cykx.dos
#__dos__##	For OPUS MAKEL, comment out the above line, and
#__dos__##	uncomment the 3 lines of the following 'foreach' loop:
#__dos__##__opus__#	!foreach name $(CYKX_OBJS)
#__dos__##__opus__#		echo $(name) >> lst_cykx.dos
#__dos__##__opus__#	!end


# -----------------------------------------------------------------------------
# COMMUNICATIONS OBJECT FILES
#	These object files are built from source which isn't in this directory
#------------------------------------------------------------------------------
#
com.o: $(COMM_DIR)/com.c $(HDI_IDIR)/common.h $(HDI_IDIR)/hdi_com.h \
		$(HDI_IDIR)/hdi_errs.h $(COMM_DIR)/com.h $(COMM_DIR)/dev.h
	$(CC) $(CC_OPTIONS) -I$(COMM_DIR) -c $(COMM_DIR)/com.c

packet.o: $(COMM_DIR)/packet.c $(HDI_IDIR)/common.h $(HDI_IDIR)/hdi_com.h \
		$(COMM_DIR)/com.h $(COMM_DIR)/dev.h $(HDI_IDIR)/hdi_errs.h
	$(CC) $(CC_OPTIONS) -I$(COMM_DIR) -c $(COMM_DIR)/packet.c

crcarray.o: $(COMM_DIR)/crcarray.c $(HDI_IDIR)/common.h
	$(CC) $(CC_OPTIONS) -I$(COMM_DIR) -c $(COMM_DIR)/crcarray.c

autobaud.o: $(COMM_DIR)/autobaud.c $(HDI_IDIR)/common.h $(COMM_DIR)/com.h \
		$(COMM_DIR)/dev.h $(HDI_IDIR)/hdi_errs.h
	$(CC) $(CC_OPTIONS) -I$(COMM_DIR) -c $(COMM_DIR)/autobaud.c


#------------------------------------------------------------------------------
# SPECIAL TARGETS
#	object modules generated with special rules
#------------------------------------------------------------------------------
#
# $(TARGET)xxxx.o modules use this_hw.h to change hardware between targets.
#     The targets board definitions file is copied to this_hw.h from one of
#     (epcx.h evsx.h cycx.h cyjx.h ... ).
#
$(TARGET)_510.o: 82510.c 82510.h $(TARGET).h $(RETARGET_H) $(HDI_IDIR)/common.h
	$(CC) $(CC_OPTIONS) -o $@ -c 82510.c

$(TARGET)_530.o: 8530.c 8530.h $(TARGET).h $(RETARGET_H) $(HDI_IDIR)/common.h
	$(CC) $(CC_OPTIONS) -o $@ -c 8530.c

$(TARGET)_552.o: 16552.c 16552.h $(TARGET).h c145.h $(RETARGET_H) $(HDI_IDIR)/common.h
	$(CC) $(CC_OPTIONS) -o $@ -c 16552.c

$(TARGET)8254.o: 82c54.c $(TARGET).h 82c54.h $(MON960_H) $(RETARGET_H)
	$(CC) $(CC_OPTIONS) -o $@ -c 82c54.c

$(TARGET)8536.o: 85c36.c $(TARGET).h c145.h $(MON960_H) $(RETARGET_H)
	$(CC) $(CC_OPTIONS) -o $@ -c 85c36.c

$(TARGET)cibr.o: ca_ibr.c $(TARGET).h $(HDI_IDIR)/common.h $(I960_H)
	$(CC) $(CC_OPTIONS) -o $@ -c ca_ibr.c

$(TARGET)jibr.o: jx_ibr.c $(TARGET).h $(HDI_IDIR)/common.h $(I960_H)
	$(CC) $(CC_OPTIONS) -o $@ -c jx_ibr.c
    
$(TARGET)hibr.o: hx_ibr.c $(TARGET).h $(HDI_IDIR)/common.h $(I960_H)
	$(CC) $(CC_OPTIONS) -o $@ -c hx_ibr.c

$(TARGET)flsh.o: flash.c $(TARGET).h $(MON960_H) c145.h $(RETARGET_H) \
                 $(HDI_IDIR)/hdi_errs.h
	$(CC) $(CC_OPTIONS) -o $@ -c flash.c

$(TARGET)ldsw.o: leds_sw.c $(TARGET).h c145.h $(MON960_H)
	$(CC) $(CC_OPTIONS) -o $@ -c leds_sw.c

$(TARGET)para.o: paradrvr.c $(TARGET).h c145.h $(HDI_IDIR)/hdi_errs.h \
                 $(RETARGET_H) $(HDI_IDIR)/dbg_mon.h
	$(CC) $(CC_OPTIONS) -o $@ -c paradrvr.c

$(TARGET)_pci.o: pcidrvr.c $(TARGET).h c145.h $(RETARGET_H) $(COMM_DIR)/dev.h \
                 $(COMM_DIR)/com.h $(HDI_IDIR)/dbg_mon.h $(HDI_IDIR)/hdi_com.h \
                 $(COMM_DIR)/cyc9060.h
	$(CC) $(CC_OPTIONS) -I$(COMM_DIR) -o $@ -c pcidrvr.c

$(TARGET)prcb.o: set_prcb.c $(TARGET).h $(MON960_H) $(RETARGET_H)
	$(CC) $(CC_OPTIONS) -o $@ -c set_prcb.c

$(TARGET)ghis.o: ghist.c $(TARGET).h $(MON960_H) $(RETARGET_H)
	$(CC) $(CC_OPTIONS) $(HJ_TIMER) -o $@ -c ghist.c

$(TARGET)btim.o: bentime.c $(TARGET).h $(MON960_H) $(RETARGET_H)
	$(CC) $(CC_OPTIONS) -o $@ -c bentime.c

$(TARGET)time.o: timer.c $(TARGET).h $(MON960_H)
	$(CC) $(CC_OPTIONS) -o $@ -c timer.c

$(TARGET)timr.o: hj_timer.c $(TARGET).h $(MON960_H) $(RETARGET_H)
	$(CC) $(CC_OPTIONS) -o $@ -c hj_timer.c

$(TARGET)_eep.o: c145_eep.c c145_eep.h c145.h $(I960_H) $(TARGET).h
	$(CC) $(CC_OPTIONS) -o $@ -c c145_eep.c

$(TARGET)test.o: cyt_test.c $(TARGET).h c145.h cyt_menu.h c145_eep.h cyt_intr.h 
	$(CC) $(CC_OPTIONS) -o $@  -c cyt_test.c

$(TARGET)intr.o: cyt_intr.c $(TARGET).h c145.h $(I960_H) $(HDI_IDIR)/common.h cyt_intr.h 
	$(CC) $(CC_OPTIONS) -o $@  -c cyt_intr.c

$(TARGET)_cio.o: cyt_cio.c $(TARGET).h c145.h cyt_intr.h 
	$(CC) $(CC_OPTIONS) -o $@  -c cyt_cio.c

$(TARGET)tpci.o: cyt_pci.c $(TARGET).h c145.h cyt_intr.h $(COMM_DIR)/cyc9060.h
	$(CC) $(CC_OPTIONS) -I$(COMM_DIR) -o $@  -c cyt_pci.c

$(TARGET)base.o: c145_hw.c $(TARGET).h c145.h retarget.h $(I960_H) c145_eep.h \
                 $(HDI_IDIR)/common.h $(COMM_DIR)/cyc9060.h
	$(CC) $(CC_OPTIONS) -I$(COMM_DIR) -o $@ -c c145_hw.c

fastdown.o: $(RETARGET_H) $(MON960_H) $(HDI_IDIR)/hdi_com.h \
		$(HDI_IDIR)/dbg_mon.h $(HDI_IDIR)/hdi_errs.h $(COMM_DIR)/cyc9060.h
	$(CC) $(CC_OPTIONS) -I$(COMM_DIR) -o $@ -c fastdown.c

fp.o: ui.h $(RETARGET_H) $(HDI_IDIR)/hdi_arch.h $(HDI_IDIR)/hdi_errs.h
	$(CC_FP) $(CC_OPTIONS) -o $@ -c fp.c

float.o: float.s $(HDI_IDIR)/hdi_arch.h
	$(RM) tmp.?
	$(CP) float.s tmp.c
	$(CC_FP) $(CC_OPTIONS) $(ALLOW_UNALIGNED) $(AS_EMIT) tmp.c > tmp.s
	$(AS_FP) -o float.o tmp.s
 
ui_float.o: ui_float.s
	$(RM) tmp.?
	$(CP) ui_float.s tmp.c
	$(CC_FP) $(CC_OPTIONS) $(ALLOW_UNALIGNED) $(AS_EMIT) tmp.c > tmp.s
	$(AS_FP) -o ui_float.o tmp.s

init.o: init.s $(HDI_IDIR)/hdi_arch.h
	$(RM) tmp.? 
	$(CP) init.s tmp.c
	$(CC) $(CC_OPTIONS) $(ALLOW_UNALIGNED) $(AS_EMIT) tmp.c > tmp.s
	$(AS) -o init.o tmp.s

mem.o: $(RETARGET_H) $(HDI_IDIR)/hdi_errs.h $(HDI_IDIR)/common.h
	$(CC) $(CC_OPTIONS) $(ALLOW_UNALIGNED) -c mem.c

serial.o: $(RETARGET_H) $(HDI_IDIR)/hdi_errs.h $(HDI_IDIR)/hdi_com.h \
          $(MON960_H) $(COMM_DIR)/com.h
	$(CC) $(CC_OPTIONS) -I$(COMM_DIR) -c serial.c

commcfg.o: $(RETARGET_H) $(HDI_IDIR)/hdi_com.h $(COMM_DIR)/dev.h $(MON960_H)
	$(CC) $(CC_OPTIONS) -I$(COMM_DIR) -c commcfg.c

version.o: FORCE


#------------------------------------------------------------------------------
# HEADER-FILE DEPENDENCIES for common files
#------------------------------------------------------------------------------
#
aplnksup.o: aplnksup.c $(MON960_H) $(HDI_IDIR)/hdi_errs.h
break.o: $(HDI_IDIR)/hdi_brk.h $(HDI_IDIR)/hdi_errs.h $(HDI_IDIR)/common.h $(I960_H)
ca_break.o: $(I960_H) $(HDI_IDIR)/hdi_brk.h $(HDI_IDIR)/hdi_stop.h \
	$(HDI_IDIR)/common.h
ca_cpu.o: $(I960_H) $(HDI_IDIR)/dbg_mon.h $(HDI_IDIR)/common.h $(HDI_IDIR)/hdi_errs.h
ca_step.o: $(HDI_IDIR)/common.h
convert.o: ui.h
cyt_menu.o: cyt_menu.c $(HDI_IDIR)/common.h cyt_menu.h 
cyt_ethr.o: cyt_ethr.c cyt_ethr.h cyt_intr.h 
cyt_uart.o: cyt_uart.c 16552.h cyt_intr.h 
dis.o: ui.h $(I960_H)
disp_mod.o: ui.h $(I960_H)
download.o: ui.h $(I960_H) $(HDI_IDIR)/hdi_errs.h
fault.o: $(I960_H) $(HDI_IDIR)/hdi_stop.h $(HDI_IDIR)/common.h
fp.o: ui.h $(RETARGET_H) $(HDI_IDIR)/hdi_arch.h $(HDI_IDIR)/hdi_errs.h
go_user.o: $(RETARGET_H) $(HDI_IDIR)/hdi_errs.h $(HDI_IDIR)/common.h \
	$(HDI_IDIR)/hdi_stop.h $(HDI_IDIR)/hdi_com.h
hi_main.o: $(HDI_IDIR)/dbg_mon.h $(RETARGET_H) $(HDI_IDIR)/hdi_arch.h \
	$(HDI_IDIR)/hdi_stop.h $(HDI_IDIR)/hdi_errs.h $(HDI_IDIR)/hdi_mcfg.h \
	$(HDI_IDIR)/hdi_com.h $(MON960_H)
hi_rt.o: $(HDI_IDIR)/dbg_mon.h $(HDI_IDIR)/common.h \
	$(HDI_IDIR)/hdi_errs.h $(HDI_IDIR)/hdi_com.h $(HDI_IDIR)/cop.h
hi_stubs.o: $(HDI_IDIR)/hdi_com.h $(COMM_DIR)/dev.h $(HDI_IDIR)/common.h
io.o: ui.h $(HDI_IDIR)/common.h $(I960_H)
hx_break.o: $(I960_H) $(HDI_IDIR)/hdi_brk.h $(HDI_IDIR)/hdi_stop.h \
	$(HDI_IDIR)/common.h
hx_cpu.o: $(I960_H) $(HDI_IDIR)/dbg_mon.h $(HDI_IDIR)/common.h $(HDI_IDIR)/hdi_errs.h
hx_step.o: $(HDI_IDIR)/common.h
jx_break.o: $(I960_H) $(HDI_IDIR)/hdi_brk.h $(HDI_IDIR)/hdi_stop.h \
	$(HDI_IDIR)/common.h
jx_cpu.o: $(I960_H) $(HDI_IDIR)/dbg_mon.h $(HDI_IDIR)/common.h $(HDI_IDIR)/hdi_errs.h
jx_step.o: $(MON960_H)
kx_break.o: $(I960_H) $(HDI_IDIR)/hdi_brk.h $(HDI_IDIR)/hdi_stop.h \
	$(HDI_IDIR)/common.h
kx_cpu.o: $(I960_H) $(HDI_IDIR)/dbg_mon.h $(HDI_IDIR)/common.h $(HDI_IDIR)/hdi_errs.h
main.o:	$(RETARGET_H) $(HDI_IDIR)/hdi_com.h $(MON960_H)
monitor.o: $(RETARGET_H) $(MON960_H) $(HDI_IDIR)/hdi_com.h
no_flash.o: $(HDI_IDIR)/hdi_errs.h $(I960_H) $(HDI_IDIR)/common.h
no_fp.o: ui.h
no_aplnk.o: $(HDI_IDIR)/hdi_errs.h $(HDI_IDIR)/hdi_com.h $(HDI_IDIR)/dbg_mon.h
no_ghist.o: $(MON960_H)
no_para.o: $(RETARGET_H)
no_pci.o:  $(RETARGET_H)
no_serl.o: $(RETARGET_H)
no_timer.o: $(RETARGET_H)
parse.o: ui.h $(HDI_IDIR)/common.h $(I960_H)
perror.o: $(HDI_IDIR)/common.h
unimplmt.o: $(RETARGET_H)
runtime.o: $(HDI_IDIR)/common.h
trace.o: ui.h $(I960_H)
ui_break.o: ui.h $(HDI_IDIR)/hdi_brk.h $(HDI_IDIR)/hdi_errs.h $(HDI_IDIR)/common.h
ui_main.o: ui.h $(RETARGET_H) $(HDI_IDIR)/hdi_stop.h $(HDI_IDIR)/common.h \
           $(HDI_IDIR)/hdi_brk.h $(HDI_IDIR)/hdi_errs.h $(HDI_IDIR)/hdi_arch.h
ui_rt.o: ui.h
ui_stubs.o: $(I960_H) $(HDI_IDIR)/hdi_errs.h $(HDI_IDIR)/common.h

#------------------------------------------------------------------------------
# DUMMY "MAKE" TARGET to force execution of specific targets.
#------------------------------------------------------------------------------
#
FORCE:

#------------------------------------------------------------------------------
# FLAG TARGETS
#------------------------------------------------------------------------------
# Targets to check for presence of local "flag files":  if it exists the flag
# file indicates that the objects in the working directory were built 
# for the specified toolset, processor and endianess ie flagkx.gnu, flagkx.itl.
#
# if it doesn't exist delete all object files and flag files as these
# objects may be incompatible with what we're trying to create.
#
__$(TARGET)$(BE).$(TOOLSET):
	$(MAKE) small
	echo "I exist" > __$(TARGET)$(BE).$(TOOLSET)

#------------------------------------------------------------------------------
# MAKEFILE MANIPULATION RULES
#	customizes makefile for particular toolsets / hosts
#------------------------------------------------------------------------------
# Target to uncomment tool-specific lines in this makefile, i.e. lines
# beginning in column 1 with the following string:  #__toolset__# .
# Original Makefile is backed up as 'Makefile.old'.
#
# Invoke with:  make make TOOL=yyy HOST=xxx [BAT=bat]
#
# HOST is used to handle dos 127 char line limit and other cmd line oddities.
# BAT is used to make a TOOL specific .bat file for DOS fro all targets.
#
make:
	-@grep -s "^#The next line was generated by 'make make'" Makefile; \
	if test $$? = 0 ; then	\
		echo "Makefile has already been processed with 'make make'";\
		exit 1; \
	elif test $(TOOL)x = x ; then \
		echo 'Specify "make make TOOL=??? HOST=???"'; \
		exit 1; \
	elif test $(HOST)x = x ; then \
		echo 'Specify "make make TOOL=??? HOST=???"'; \
		exit 1; \
	else \
		mv -f Makefile Makefile.old; \
		echo "#The next line was generated by 'make make'"> Makefile; \
		echo "TOOL=$(TOOL)  HOST=$(HOST)"				 >> Makefile; \
		echo						 >> Makefile; \
		sed "s/^#__$(TOOL)__#//" < Makefile.old		 >> Makefile; \
		mv -f Makefile Makefile.hst; \
		sed "s/^#__$(HOST)__#//" < Makefile.hst		 >> Makefile; \
		if test $(HOST) = dos; then \
#  uncomment next two lines to make dos .bat file for script makes \
		    mv -f Makefile Makefile.hst; \
		    sed "s/^#__$(BAT)__#//" < Makefile.hst		 >> Makefile; \
		    rm -f Makefile.hst; \
			mv -f Makefile.old makefile.unx; \
		    mv -f Makefile makefile; \
		else \
		    rm -f Makefile.hst; \
		fi \
	fi
