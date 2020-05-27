#!/bin/sh

# Create the ver960.c file
# Invocation is mkver.sh [SNAPSHOT]

GCC960_VER=5.0
GCC960_VER_MAC=500
IC960_VER=5.0
IC960_VER_MAC=500
FSF_VER=2.6.3

if test "$SNAPSHOT" = ""; then
  SNAPSHOT=0;
fi

if test "$1" = "SNAPSHOT"; then
  SNAPSHOT=1
  shift;
fi

if test "0" = "$SNAPSHOT"; then
  DATE="`date`"
else
  DATE="                            "
fi

#
# Create ver960.c
#

rm -f ver960.c

echo "" >ver960.c
echo "char gnu960_ver[]=\"gcc960 Version $GCC960_VER.`cat i_minrev` $DATE\";">> ver960.c

echo "char cc1960_ver[]=\"cc1.960 Version $GCC960_VER.`cat i_minrev` GNU C $FSF_VER $DATE\";">> ver960.c
echo "char cc1plus960_ver[]=\"cc1plus.960 Version $GCC960_VER.`cat i_minrev` GNU C++ $FSF_VER $DATE\";">> ver960.c

echo "char cpp960_ver[]=\"cpp.960 Version $GCC960_VER.`cat i_minrev` GNU CPP $FSF_VER $DATE\";">> ver960.c

echo "char ic960_ver[]=\"ic960 Version $IC960_VER.`cat i_minrev` $DATE\";">> ver960.c

echo "char gcc960_ver_mac[] = \"$GCC960_VER_MAC`cat i_minrev`\";">> ver960.c
echo "char ic960_ver_mac[] = \"$IC960_VER_MAC`cat i_minrev`\";"  >> ver960.c
