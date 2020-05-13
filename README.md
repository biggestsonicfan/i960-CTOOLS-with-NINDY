# i960-CTOOLS-with-NINDY
A repo in which i960 CTOOLS and NINDY are mirrored and a work in progress for binaries will be.


#CTOOLS 5.0 for i386 NetBSD

In order to run CTOOLS, you must either be on actual i386 hardware or, better yet, a virtual machine. It can be installed from the ISO located here: http://wiki.netbsd.org/ports/i386/

Once a machine is up and running, install git-base and mozilla-rootcerts in order to clone this repo. After the repo is cloned, install compat12 to be able to run the binaries. Chmod +x the executables and you should be good to run CTOOLS!

#NINDY

Officially, the documentation for CTOOLS 5.0 states: "The NINDY monitor is no longer supported. MON960 is the only supported target." However, this does not mean NINDY can not be compiled with CTOOLS 5.0.

So far, I have not successfully compiled a NINDY binary.

#GDB 5.3 and NINDY

GDB 5.3 is the latest version, that I am aware of, that will support a NINDY monitor. The "nindy-share" has a Makefile, but it is virtually uncompilable as it's coding was abandoned in favor or "remote-nindy.c", which in this release has been commented out entirely with "// OBSOLETE", however, it may still be compilable.


These three tools combine may allow access to the EV80960SX and QT960 Eval boards, but absolutely no guarantee or warranty is given or implied here.
