copy crtnin.s crtnin.c
ic960 -E crtnin.c | asm960 -o crtnin.o -ACA_A 
ic960 -E -DFLASH crtnin.c | asm960 -o crtfnin.o -ACA_A 
ic960 -E -DPROFILE crtnin.c | asm960 -o crtpnin.o -ACA_A 
del crtnin.c
cd ..
