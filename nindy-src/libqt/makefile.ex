I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ic960 -c -R -O2 -AKB copyrght.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ic960 -c -R -O2 -AKB -I../include dma.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ic960 -c -R -O2 -AKB -I../include time_qt.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ic960 -c -R -O2 -AKB -I../include flashlib.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ic960 -c -R -O2 -AKB -I../include prof_qt.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ic960 -c -R -O2 -AKB ver960.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ asm960 -o eat_time.o eat_time.s
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ asm960 -o prof_isr.o prof_isr.s
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ asm960 -o qtasms.o qtasms.s      
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ arc960 -sr libqt.a copyrght.o eat_time.o time_qt.o dma.o flashlib.o qtasms.o prof_qt.o prof_isr.o ver960.o
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ic960 -c -R -O2 -AKA copyrght.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ic960 -c -R -O2 -AKA -I../include dma.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ic960 -c -R -O2 -AKA -I../include time_qt.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ic960 -c -R -O2 -AKA -I../include flashlib.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ic960 -c -R -O2 -AKA -I../include prof_qt.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ic960 -c -R -O2 -AKA ver960.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ asm960 -o eat_time.o eat_time.s
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ asm960 -o prof_isr.o prof_isr.s
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ asm960 -o qtasms.o qtasms.s      
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ arc960 -sr libqtka.a copyrght.o eat_time.o time_qt.o dma.o flashlib.o qtasms.o prof_qt.o prof_isr.o ver960.o
cd ..
