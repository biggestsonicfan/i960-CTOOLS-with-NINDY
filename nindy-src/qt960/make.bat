ic960 -c -R -AKB -I..\include flash.c
ic960 -c -R -AKB -I..\include qt_data.c
ic960 -c -R -AKB -I..\include qt_hw.c
ic960 -c -R -AKB -I..\include qt_io.c
ic960 -c -R -AKB -I..\include test_cal.c
ic960 -c -R -AKB -I..\include test_int.c
ic960 -c -R -AKB -I..\include test_tim.c
ic960 -c -R -AKB -I..\include tests.c
asm960 -o copymem.o copymem.s
asm960 -o test_asm.o test_asm.s    
arc960 -sr qt.a test_cal.o qt_hw.o tests.o test_tim.o copymem.o test_int.o test_asm.o flash.o qt_io.o qt_data.o
cd ..
