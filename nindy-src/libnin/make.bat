ic960 -c -R -O2 -ACA branch.c
ic960 -c -R -O2 -ACA brk.c
ic960 -c -R -O2 -ACA copyrght.c
ic960 -c -R -O2 -ACA dto_stub.c
ic960 -c -R -O2 -ACA errno.c
ic960 -c -R -O2 -ACA fileio.c
ic960 -c -R -O2 -ACA gcc_stub.c
ic960 -c -R -O2 -ACA newlib.c
ic960 -c -R -O2 -ACA profile.c
ic960 -c -R -O2 -ACA ver960.c
asm960 -ACA -o br_asm.o br_asm.s
asm960 -ACA -o heapsize.o heapsize.s
asm960 -ACA -o libasm.o libasm.s
asm960 -ACA -o pr_start.o pr_start.s
asm960 -ACA -o pr_freq.o pr_freq.s
asm960 -ACA -o pr_end.o pr_end.s
asm960 -ACA -o pr_buck.o pr_buck.s
arc960 -sr libnin.a copyrght.o br_asm.o branch.o brk.o dto_stub.o errno.o fileio.o gcc_stub.o heapsize.o libasm.o newlib.o 
arc960 -sr libnin.a pr_start.o pr_freq.o pr_end.o pr_buck.o profile.o ver960.o
cd ..
