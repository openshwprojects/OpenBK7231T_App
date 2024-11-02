SiFive Freedom Bare Metal Toolchain (riscv64-unknown-elf)
--------

At SiFive we've been distributing binary release packages of the
tools that target our Freedom RISC-V platforms.  This repository
contains the scripts we use to build some of these tools.

This repo is part of Freedom Tools: https://github.com/sifive/freedom-tools

### Customizations added to original code by SiFive:
To be written!

### To build the tools:

    $ git clone git@github.com:sifive/freedom-toolchain-metal.git
    $ cd freedom-toolchain-metal
    $ make package

The final output is a set of tarballs in the "bin" folder that should be ready to use.
The output of a Ubuntu build includes a set of tarballs and zip files for Windows
which is build using the MinGW toolchain.

### Prerequisites

Several standard packages are needed to build the tools on the different supported platforms.


On Ubuntu, executing the following command should suffice:

    $ sudo apt-get install cmake autoconf automake autotools-dev curl libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf patchutils bc zlib1g-dev libexpat-dev libtool pkg-config mingw-w64 mingw-w64-tools texlive zip python-dev gettext libglib2.0-dev libpixman-1-dev swig ninja-build python3
    $ sudo pip3 install meson

On OS X, you can use [Homebrew](http://brew.sh) to install most of the dependencies and then you also need [MacTex](http://www.tug.org/mactex/):

    $ brew install cmake autoconf automake gawk gnu-sed gnu-tar texinfo libtool pkg-config wget xz swig python3 ninja meson

On Fedora/CentOS/RHEL OS, executing the following command should suffice - plus see below:

    $ sudo yum install cmake libmpc-devel mpfr-devel gmp-devel gawk bison flex texinfo patchutils gcc gcc-c++ zlib-devel expat-devel swig rh-python35 ninja-build
    $ sudo pip3 install meson

On CentOS/RHEL 7 and Fedora you can use yum install for the rest:

    $ sudo yum install autoconf automake libtool pkg-config

On CentOS/RHEL 6 you need to download and compile some tools manually to get the correct versions:

    $ wget http://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.gz
    $ tar xzvf autoconf-2.69.tar
    $ cd autoconf-2.69
    $ ./configure
    $ make
    $ make install

    $ wget http://ftp.gnu.org/gnu/automake/automake-1.15.tar.gz
    $ tar xzvf automake-1.15.tar.gz
    $ cd automake-1.15
    $ ./configure
    $ make
    $ make install

    $ wget http://ftp.gnu.org/gnu/libtool/libtool-2.4.6.tar.gz
    $ tar xzvf libtool-2.4.6.tar.gz
    $ cd libtool-2.4.6
    $ ./configure
    $ make
    $ make install

    $ wget https://pkgconfig.freedesktop.org/releases/pkg-config-0.29.2.tar.gz
    $ tar xzvf pkg-config-0.29.2.tar.gz
    $ cd pkg-config-0.29.2
    $ ./configure --with-internal-glib
    $ make
    $ make install

    $ wget https://ftp.gnu.org/gnu/texinfo/texinfo-6.4.tar.gz
    $ tar xzvf texinfo-6.4.tar.gz
    $ cd texinfo-6.4
    $ ./configure
    $ make
    $ make install
