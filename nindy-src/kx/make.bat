ic960 -c -R -O2 -AKB -I..\include kx_break.c
asm960 -o kx.o kx.s
asm960 -o kx_ftbl.o kx_ftbl.s
asm960 -o kx_init.o kx_init.s 
arc960 -sr kx.a kx_break.o kx.o kx_ftbl.o
cd ..
