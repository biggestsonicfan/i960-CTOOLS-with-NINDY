ic960 -c -R -O2 -AKB copyrght.c
ic960 -c -R -O2 -AKB -I..\include dma.c
ic960 -c -R -O2 -AKB -I..\include time_qt.c
ic960 -c -R -O2 -AKB -I..\include flashlib.c
ic960 -c -R -O2 -AKB -I..\include prof_qt.c
ic960 -c -R -O2 -AKB ver960.c
asm960 -o eat_time.o eat_time.s
asm960 -o prof_isr.o prof_isr.s
asm960 -o qtasms.o qtasms.s      
arc960 -sr libqt.a copyrght.o eat_time.o time_qt.o dma.o flashlib.o qtasms.o prof_qt.o prof_isr.o ver960.o
ic960 -c -R -O2 -AKA copyrght.c
ic960 -c -R -O2 -AKA -I..\include dma.c
ic960 -c -R -O2 -AKA -I..\include time_qt.c
ic960 -c -R -O2 -AKA -I..\include flashlib.c
ic960 -c -R -O2 -AKA -I..\include prof_qt.c
ic960 -c -R -O2 -AKA ver960.c
asm960 -o eat_time.o eat_time.s
asm960 -o prof_isr.o prof_isr.s
asm960 -o qtasms.o qtasms.s      
arc960 -sr libqtka.a copyrght.o eat_time.o time_qt.o dma.o flashlib.o qtasms.o prof_qt.o prof_isr.o ver960.o
cd ..
