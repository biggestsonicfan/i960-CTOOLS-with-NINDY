ic960 -c -R -O2 -ACA copyrght.c
ic960 -c -R -O2 -ACA noint_tm.c
ic960 -c -R -O2 -ACA profevca.c
ic960 -c -R -O2 -ACA time_ev.c
ic960 -c -R -O2 -ACA ver960.c
asm960 -o evcaasm.o -ACA_A evcaasm.s
asm960 -o prof_isr.o -ACA_A prof_isr.s
asm960 -o eat_time.o -ACA_A eat_time.s
arc960 -sr libevca.a copyrght.o evcaasm.o profevca.o prof_isr.o eat_time.o noint_tm.o time_ev.o ver960.o
cd ..
