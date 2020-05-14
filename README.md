# i960-CTOOLS-with-NINDY
A repo in which i960 CTOOLS and NINDY are mirrored and a work in progress for binaries will be.


# CTOOLS 5.0 for i386 NetBSD

In order to run CTOOLS, you must either be on actual i386 hardware or, better yet, a virtual machine. It can be installed from the ISO located here: http://wiki.netbsd.org/ports/i386/

Once a machine is up and running, install git-base and mozilla-rootcerts in order to clone this repo. After the repo is cloned, install compat12 to be able to run the binaries. Chmod +x the executables and you should be good to run CTOOLS!

# NINDY

Officially, the documentation for CTOOLS 5.0 states: "The NINDY monitor is no longer supported. MON960 is the only supported target." However, this does not mean NINDY can not be compiled with CTOOLS 5.0.

So far, I have not successfully compiled a NINDY binary.

# GDB and NINDY Monitor

In order to clone the most recent repository before NINDY and the i960 were declared "OBSOLETE", I cloned the following repo https://cvs.sonologic.net/gmc/gmpu-binutils-gdb.git and checkedout commit 6c0c456d8792d7a40fe94658d0975dc4c6c5289b. However, I have embedded a submodule which revered to this correct commit in this repo.

When configuring, make sure to target i960-intel-nindy.

Minor adjustments will be needed to compile.
