cd kx
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../../i386-nbsd1-ctools/bin/ic960 -c -R -O2 -AKB -I../include kx_break.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../../i386-nbsd1-ctools/bin/asm960 -o kx.o kx.s
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../../i386-nbsd1-ctools/bin/asm960 -o kx_ftbl.o kx_ftbl.s
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../../i386-nbsd1-ctools/bin/asm960 -o kx_init.o kx_init.s
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../../i386-nbsd1-ctools/bin/arc960 -sr kx.a kx_break.o kx.o kx_ftbl.o
cd ../qt960
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../../i386-nbsd1-ctools/bin/ic960 -c -R -AKB -I../include flash.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../../i386-nbsd1-ctools/bin/ic960 -c -R -AKB -I../include qt_data.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../../i386-nbsd1-ctools/bin/ic960 -c -R -AKB -I../include qt_hw.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../../i386-nbsd1-ctools/bin/ic960 -c -R -AKB -I../include qt_io.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../../i386-nbsd1-ctools/bin/ic960 -c -R -AKB -I../include test_cal.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../../i386-nbsd1-ctools/bin/ic960 -c -R -AKB -I../include test_int.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../../i386-nbsd1-ctools/bin/ic960 -c -R -AKB -I../include test_tim.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../../i386-nbsd1-ctools/bin/ic960 -c -R -AKB -I../include tests.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../../i386-nbsd1-ctools/bin/asm960 -o copymem.o copymem.s
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../../i386-nbsd1-ctools/bin/asm960 -o test_asm.o test_asm.s    
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../../i386-nbsd1-ctools/bin/arc960 -sr qt.a test_cal.o qt_hw.o tests.o test_tim.o copymem.o test_int.o test_asm.o flash.o qt_io.o qt_data.o
cd ..
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/ic960 -c -R -O2 -Iinclude break.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/ic960 -c -R -O2 -Iinclude convert.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/ic960 -c -R -O2 -Iinclude dis.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/ic960 -c -R -O2 -Iinclude disp_mod.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/ic960 -c -R -O2 -Iinclude download.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/ic960 -c -R -O2 -Iinclude errno.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/ic960 -c -R -O2 -Iinclude fault.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/ic960 -c -R -O2 -Iinclude fp.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/ic960 -c -R -O2 -Iinclude gdb.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/ic960 -c -R -O2 -Iinclude int_rout.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/ic960 -c -R -O2 -Iinclude io.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/ic960 -c -R -O2 -Iinclude main.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/ic960 -c -R -O2 -Iinclude monitor.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/ic960 -c -R -O2 -Iinclude parse.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/ic960 -c -R -O2 -Iinclude trace.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/asm960 -o eat_time.o eat_time.s
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/asm960 -o float.o float.s
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/asm960 -o faultasm.o faultasm.s
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/asm960 -o i_table.o i_table.s
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/asm960 -o ldstore.o ldstore.s 
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/asm960 -o setjmp.o setjmp.s 
cp i_handle.c.test i_handle.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/ic960 -E i_handle.c > i_handle_test.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/asm960 -o i_handle.o i_handle_test.c
rm i_handle_test.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/lnk960 -r -o basefile.o main.o break.o convert.o dis.o disp_mod.o download.o eat_time.o errno.o fault.o faultasm.o gdb.o i_handle.o i_table.o int_rout.o io.o ldstore.o monitor.o parse.o setjmp.o trace.o
G960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/gcc960 -o qtrom lib/qtrom_test.ld kx/kx_init.o basefile.o fp.o float.o kx/kx.a qt960/qt.a -lckb -lll


I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../i386-nbsd1-ctools/bin/rom960 ./lib/qtrom qtrom
rm image
rm out.*
