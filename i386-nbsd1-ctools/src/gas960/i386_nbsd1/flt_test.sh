
#
# This file along with the following others are used to test .float
# .double and .extended in gas960:
# 
# flt_test.c
# faillines.c
# encoding.c
# cmphost.c
# 
# To execute these tests:
# 
# 1. Login to sys v r4 (or another host that supports asm960
# version 3.5).
# 
# 2. Obtain a copy of asm960 3.5 binary.
# 
# 3. Compile gas960c for this host.
# 
# 4. Obtain a copy of gdmp960 for this host, and place it on your PATH.
# 
# 5. Run this file:
# 
# nohup flt_test.sh > flt_test.out 2>&1 < /dev/null &
# 
# (per borne shell C shell will differ).
# 
# How to check results:
# 
# 1.  cd failures
# 
# 1.5 for i in 0*.s ; do
#      tail -1 $i | grep -v cmp
#     done
# 
#     Should be silent.
# 
# 1.75 ls -l f[0-3].s
# 
# 2.  If any of the files f[0-3].s exist, and are not size 0 this
# means the following:
# 
# If f0.s has something in it, that means that asm960 3.5 encoding of
# .extended differed from gas's.
# 
# If f1.s has something in it, that means that asm960 3.5 encoding of
# .float or .double disagreed with gas's.  BUT, the host's scanf() agreed
# with gas.
# 
# If f2.s has something in it, that means that asm960 3.5 encoding of
# .float or .double disagreed with gas's.  BUT, the host's scanf() agreed
# with asm.
# 
# If f3.s has something in it, that means that asm960 3.5 encoding of
# .float or .double disagreed with gas's.  AND, the host's scanf() disagreed
# with both gas and asm.
# 
# 
# How to interpret results:
# 
# 1.  f2.s MUST BE empty or there is likely a bug in gas.
# 2.  If any of the other files are non empty we should look at them a
#     little - run cmphost on them for example but do not spend too much
#     time.
#
# 
# Other tools:
#
# The following other tools are provided from your computing pleasure.
#
# The tool encoding gives you the host's encoding of a double argument.
# For example:
# 
# $ cc encoding.c -o encoding
# $ ./encoding 1.2345e67
# 
# Will output the bits of the host's encoding of the indicated floating
# point value.
# 
# 
# The tool cmphost allows you to score different hosts encoding vis-a-vis
# gas and asm's encoding.  An example use:
# 
# Goto sun4 host and:
# $ cc cmphost.c -o cmphost
# $ cp f1.s .
# $ emacs f1.s
# change lines of the form:
# x59: .double 0d-4.2426102463236077000e256
# into:
# d-4.2426102463236077000e256
# Similarly for .float
# $ ./cmphost f1.s data
# 
# Output is a tabular score of the different encodings of the host asm and
# gas.
# 
# Goto rs6000 host and:
# $ cc cmphost.c -o cmphost
# $ ./cmphost f1.s data
# 
# Here, the file data is used to accumulate 'votes'.  So many votes for
# gas and asm.  The first operation initialized data to the sun4's
# encoding.  Here you may see 'majorities' where so many hosts agree with
# gas or asm and 0 agree with gas or asm.  Clearly, if 5 hosts agree with
# asm and 0 agree with gas, then there is likely a bug in gas.
# 
#
#
# Let's execute the test:
#

rm -fr failures
mkdir failures
cc flt_test.c -o flt_test.svr4
cc faillines.c -o faillines
ASM960=./asm960.35 GAS960=./gas960c ./flt_test.svr4
