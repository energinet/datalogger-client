# How to build the data logger client

## Prerequisites:
A linux machine with a gcc cross compiler for ARM
git to pull the source from source (optional, it can be downloaded as an archive)
gsoap 2.7.16 is needed on the build machine, the directory "patches" includes a patch needed when building with recent versions of gcc and glibc
The system's PATH variable must include a path to the cross compiler binaries named "arm-unknown-linux-gnu-*"

# Build:
The directory contains a file called "components" listing the components (modules) to be built and included in the resulting tarball. This file should be edited to reflect the wanted setup. At LIAB the components file is a symbolic link to different setups, and this approach can also be used if needed.

To build the data logger client, first check the Makefile for the variables: "ARMLIBDIR" and "ARMINCLUDEDIR" which should point to the cross compilation libs and includes respectively.
When set up correctly, the project should build by issuing a "make" command.

Several make targets exist, but "make tar" will produce a tarball called "contdaem.tar.gz" for download to the jffs2 share on the LIABSG computer.

Remember to add a contdaem.conf to the /etc/ folder of the LIABSG, an example configuration can be found in xmlfiles/contdaem_example.conf

# Getting help:
If in need of a cross compiler, or any other help, please contact info@liab.dk
