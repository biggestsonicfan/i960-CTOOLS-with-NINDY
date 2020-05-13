cd kx
call make
cd qt960
call make
ic960 -c -R -O2 -Iinclude BREAK.C
ic960 -c -R -O2 -Iinclude CONVERT.C
ic960 -c -R -O2 -Iinclude DIS.C
ic960 -c -R -O2 -Iinclude DISP_MOD.C
ic960 -c -R -O2 -Iinclude DOWNLOAD.C
ic960 -c -R -O2 -Iinclude ERRNO.C
ic960 -c -R -O2 -Iinclude FAULT.C
ic960 -c -R -O2 -Iinclude FP.C
ic960 -c -R -O2 -Iinclude GDB.C
ic960 -c -R -O2 -Iinclude INT_ROUT.C
ic960 -c -R -O2 -Iinclude IO.C
ic960 -c -R -O2 -Iinclude MAIN.C
ic960 -c -R -O2 -Iinclude MONITOR.C
ic960 -c -R -O2 -Iinclude PARSE.C
ic960 -c -R -O2 -Iinclude TRACE.C
asm960 -o eat_time.o eat_time.s
asm960 -o float.o float.s
asm960 -o faultasm.o faultasm.s
asm960 -o i_table.o i_table.s
asm960 -o ldstore.o ldstore.s 
asm960 -o setjmp.o setjmp.s 
copy i_handle.s i_handle.c
ic960 -E i_handle.c | asm960 -o i_handle.o
del i_handle.c
lnk960 -rF -o basefile.o basefile
lnk960 -o flrom -v -Tflrom kx\kx_init.o basefile.o fp.o float.o kx\kx.a qt960\qt.a 
rom960 flrom flrom
