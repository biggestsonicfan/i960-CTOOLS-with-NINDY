cd cx
call make
cd evca
call make
ic960 -c -R -O2 -ACA -Iinclude BREAK.C
ic960 -c -R -O2 -ACA -Iinclude CONVERT.C
ic960 -c -R -O2 -ACA -Iinclude DIS.C
ic960 -c -R -O2 -ACA -Iinclude DISP_MOD.C
ic960 -c -R -O2 -ACA -Iinclude DOWNLOAD.C
ic960 -c -R -O2 -ACA -Iinclude ERRNO.C
ic960 -c -R -O2 -ACA -Iinclude FAULT.C
ic960 -c -R -O2 -ACA -Iinclude GDB.C
ic960 -c -R -O2 -ACA -Iinclude INT_ROUT.C
ic960 -c -R -O2 -ACA -Iinclude IO.C
ic960 -c -R -O2 -ACA -Iinclude MAIN.C
ic960 -c -R -O2 -ACA -Iinclude MONITOR.C
ic960 -c -R -O2 -ACA -Iinclude NO_FP.C
ic960 -c -R -O2 -ACA -Iinclude PARSE.C
ic960 -c -R -O2 -ACA -Iinclude TRACE.C
asm960 -ACA_A -o eat_time.o eat_time.s
asm960 -ACA_A -o i_table.o i_table.s
asm960 -ACA_A -o ldstore.o ldstore.s 
asm960 -ACA_A -o faultasm.o faultasm.s
asm960 -ACA_A -o setjmp.o setjmp.s
copy i_handle.s i_handle.c
ic960 -E i_handle.c | asm960 -ACA_A -o i_handle.o
del i_handle.c
lnk960 -rF basefile -o basefile.o
lnk960 -ACA -v -o evcarom -Tevrom cx\cx_init.o basefile.o no_fp.o cx\cx.a evca\evca.a 
rom960 evrom evcarom
del image
