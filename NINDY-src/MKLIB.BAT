cd libnin
call make
copy libnin\libnin.a %I960BASE%\lib
cd libqt
call make
copy libqt\libqt*.a %I960BASE%\lib
cd libevca
call make
copy libevca\libevca.a %I960BASE%\lib
cd crt
call make
copy crt\crt*.o %I960BASE%\lib
