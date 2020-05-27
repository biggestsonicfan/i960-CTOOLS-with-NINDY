#!/bin/sh

if [ \( "$1" != "gas960c" \) -a \( "$1" != "gas960" \) ]; then
  echo "Usage: $0 (gas960 | gas960c) device [files]"
  exit 1
fi

if [ "$2" = "" ]; then
  echo "Usage: $0 gas960|gas960c device [files]"
  exit 1
fi

GAS=$1
DEVICE=$2

shift; shift

if [ "$1" = "" ]; then
  FILES=*.s
else
  FILES=$*
fi

cat >tmp$$ <<EOF
exec $GAS
load
quit
EOF

gdb960 -r $DEVICE -t mon960 -b 38400 -batch -x tmp$$

for i in $FILES; do
  echo $i
  t=`basename $i .s`

  cat >tmp$$ <<EOF
exec $GAS
run -z $i -o gdb/$t.o
quit
EOF

  gdb960 -r $DEVICE -t mon960 -b 38400 -batch -x tmp$$
  if [ -f default.pf ]; then
    mv default.pf gdb/$t.pf
  fi
done

rm -f tmp$$
