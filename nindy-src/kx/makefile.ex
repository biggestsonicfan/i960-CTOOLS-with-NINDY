I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../../i386-nbsd1-ctools/bin/ic960 -c -R -O2 -AKB -I../include kx_break.c
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../../i386-nbsd1-ctools/bin/asm960 -o kx.o kx.s
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../../i386-nbsd1-ctools/bin/asm960 -o kx_ftbl.o kx_ftbl.s
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../../i386-nbsd1-ctools/bin/asm960 -o kx_init.o kx_init.s
I960BASE=~/i960-CTOOLS-with-NINDY/i386-nbsd1-ctools/ ../../i386-nbsd1-ctools/bin/arc960 -sr kx.a kx_break.o kx.o kx_ftbl.o