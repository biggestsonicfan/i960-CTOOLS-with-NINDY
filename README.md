# i960-CTOOLS-with-NINDY
A repo in which i960 CTOOLS and NINDY are mirrored and a work in progress for binaries will be.

# CTOOLS 5.0 for i386 NetBSD
The original source for these CTOOLS binaries came from a project hosted by Carnegie Mellon University. The original project information can be found here: https://www.cs.cmu.edu/afs/cs/project/cmcl/link.vcnectar/www/CreditNet.html

The original binaries, mirrored here in this repo, can be found here: https://www.cs.cmu.edu/afs/cs/project/cmcl/link.vcnectar/gnu960/i386_nbsd1/ with the documentation here: https://www.cs.cmu.edu/afs/cs/project/cmcl/link.vcnectar/gnu960/i386_nbsd1/html/tools/gnuhome.htm

In order to run CTOOLS, you must either be on actual i386 hardware, or better yet a virtual machine, with NetBSD. It can be installed from the ISO located here: http://wiki.netbsd.org/ports/i386/

Once a machine is up and running, install git-base and mozilla-rootcerts in order to clone this repo. After the repo is cloned, install compat12 to be able to run the binaries. After using chmod +x the executables in the "bin" directory and "cc1.960" and "cpp.960 in the "lib" directory you should be good to run CTOOLS! You may want to enable sshd and connect via SSH to the machine for ease of access.

## Included Source Code
The source code to CTOOLS 5.0 for i386 NetBSD has been included in this repository. It has been modified from it's original state to remove the duplicate "common" folders and "sun4" folders, as these files are (seemingly) unnecessary for compiling. To view source code in it's original form, I suggest you visit the original CMU link here: https://www.cs.cmu.edu/afs/cs/project/cmcl/link.vcnectar/gnu960/src/ And should that link ever die, you should visit user DrItanium's i960 project for a mirror: https://github.com/DrItanium/i960/tree/master/src/gnu960/src The reason for the modification of the source code is to keep the repository as small as possible for i386 machines to clone the repository without too much unnecessary bloat.

# CTOOLS 6.0 for Windows 95
These tools were graciously provided by Professor Douglas Lynn of the Oregon Institue of Technology. Professor Lynn teaches Advanced Computer Architecture, Assembly Language, and Embedded System Architecture, including a course specifically designed around Intel's i960. These CTOOLS have been distributed in the name of preservation in hopes to keep the idea of knowledge alive with thanks to Professor Lynn.

# NINDY 3.01
## Building from source:
Officially, the documentation for CTOOLS 5.0 states: "The NINDY monitor is no longer supported. MON960 is the only supported target." However, this does not mean NINDY can not be compiled with CTOOLS 5.0.

NINDY's compilation is coming along. See this repo's Issues for any problems which may occur during compilation. Some minor quirks needed to be made to successfully compile certain files, such as `i_handle.c`. Notes for these will be in the issues they caused for compilation.

The easiest way to compile NINDY will to be having this repo cloned onto your NetBSD machine into a home directory. While this NINDY source was targed for Windows 95/DOS, I have included "makefile.ex" as example commands to compile, in the same order as the Windows 95/DOS batch files, each step of NINDY.

## Grabbing the precompiled binary:
A binary has been provided by intel in the 'roms' directory. As stated in the NINDY Manual burn the EPROMs and place them in the slots accordingly: `qt0.hex` in slot U33; `qt1.hex` in slot U34; `qt2.hex` in slot U43; `qt3.hex` in lot U44. Be sure the jumpers in the lower right corner of the QT960 board are jumpered: J22-J23 in and J24-J25 in. The board will now boot from the new EPROMs upon powerup.

## Untouched Source:
For unmodified source code directly from the floppy disk, please see my other repository: https://github.com/cglmrfreeman/QT960-Schematics/tree/master/src/NINDY

# GDB and NINDY Monitor

In order to clone the most recent repository before NINDY and the i960 were declared "OBSOLETE", I cloned the following repo https://github.com/bminor/binutils-gdb and checked out commit 6c0c456d8792d7a40fe94658d0975dc4c6c5289b. However, I have embedded a submodule which revered to this correct commit in this repo with minor updates to reflect changes over the years that modern compilers need in order to compile gdb.

When configuring, make sure to configure something like this:
`CFLAGS=-std=gnu89 ./configure -target=i960-intel-nindy -prefix=/wherever/you/want/your/output`

Minor adjustments may be needed to compile.
