# Target: UltraSPARC running NetBSD
TDEPFILES= sparc-tdep.o sparcnbsd-tdep.o nbsd-tdep.o corelow.o solib.o \
	solib-svr4.o
TM_FILE= tm-nbsd64.h
