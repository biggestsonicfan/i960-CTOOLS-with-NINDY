ic960 -AKB -O2 -c hello.c 
lnk960 -v -Tqtnindy -AKB -o hello hello.o 
lnk960 -v -Tflash -AKB -o hello.fla hello.o 
rom960 flash hello.fla
ic960 -AKA -Tqtka -O2 -o hello.ka hello.c 
ic960 -ACA -Tevnindy -O2 -o hello.ev hello.c
