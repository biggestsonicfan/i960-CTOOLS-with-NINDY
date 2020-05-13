ic960 -c -R -O2 -ACA -I..\include evca_dat.c
ic960 -c -R -O2 -ACA -I..\include evca_io.c 
ic960 -c -R -O2 -ACA -I..\include evca_hw.c 
asm960 -ACA_A -o copymem.o copymem.s
asm960 -ACA_A -o evca_asm.o evca_asm.s
copy ev_boot.s ev_boot.c
ic960 -E -I..\include ev_boot.c | asm960 -ACA_A -o ev_boot.o
del ev_boot.c
copy ev_boot.o ..
arc960 -sr evca.a evca_io.o evca_hw.o copymem.o evca_dat.o evca_asm.o
cd ..
