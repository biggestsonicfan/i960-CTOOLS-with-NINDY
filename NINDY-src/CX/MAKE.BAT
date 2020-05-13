ic960 -c -R -O2 -ACA -I..\include cx_break.c
asm960 -o cx.o -ACA_A cx.s
asm960 -o cx_ftbl.o -ACA_A cx_ftbl.s
asm960 -o cx_init.o -ACA_A cx_init.s 
arc960 -sr cx.a cx_break.o cx.o cx_ftbl.o
cd ..
