# Makefile for zlib
# Copyright (C) 1995-2017 Jean-loup Gailly, Mark Adler
# For conditions of distribution and use, see copyright notice in zlib.h

# To compile and test, type:
#    ./configure; make test
# Normally configure builds both a static and a shared library.
# If you want to build just a static library, use: ./configure --static

# To use the asm code, type:
#    cp contrib/asm?86/match.S ./match.S
#    make LOC=-DASMV OBJA=match.o

# To install /usr/local/lib/libz.* and /usr/local/include/zlib.h, type:
#    make install
# To install in $HOME instead of /usr/local, use:
#    make install prefix=$HOME

CC=gcc

CFLAGS=-O3
#CFLAGS=-O -DMAX_WBITS=14 -DMAX_MEM_LEVEL=7
#CFLAGS=-g -DZLIB_DEBUG
#CFLAGS=-O3 -Wall -Wwrite-strings -Wpointer-arith -Wconversion \
#           -Wstrict-prototypes -Wmissing-prototypes

SFLAGS=-O3 -fPIC
LDFLAGS=
TEST_LDFLAGS=-L. libz.a
LDSHARED=gcc
CPP=gcc -E

STATICLIB=libz.a
SHAREDLIB=
SHAREDLIBV=
SHAREDLIBM=
LIBS=$(STATICLIB) $(SHAREDLIBV)

AR=ar
ARFLAGS=rc
RANLIB=ranlib
LDCONFIG=ldconfig
LDSHAREDLIBC=-lc
TAR=tar
SHELL=/bin/sh
EXE=.exe

prefix =/usr/local
exec_prefix =${prefix}
libdir =${exec_prefix}/lib
sharedlibdir =${libdir}
includedir =${prefix}/include
mandir =${prefix}/share/man
man3dir = ${mandir}/man3
pkgconfigdir = ${libdir}/pkgconfig
SRCDIR=
ZINC=
ZINCOUT=-I.

OBJZ = adler32.o crc32.o deflate.o trees.o zutil.o
OBJG = gzlib.o gzclose.o gzlib.o gzwrite.o
OBJC = $(OBJZ) $(OBJG)

OBJS = $(OBJC)


all: static

static: minigzip$(EXE)

libz.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)
	-@ ($(RANLIB) $@ || true) >/dev/null 2>&1

minigzip.o: $(SRCDIR)minigzip.c $(SRCDIR)zlib.h zconf.h
	$(CC) $(CFLAGS) $(ZINCOUT) -c -o $@ $(SRCDIR)minigzip.c
adler32.o: adler32.c
	$(CC) $(CFLAGS) $(ZINC) -c -o $@ adler32.c

crc32.o: crc32.c
	$(CC) $(CFLAGS) $(ZINC) -c -o $@ crc32.c

deflate.o: deflate.c
	$(CC) $(CFLAGS) $(ZINC) -c -o $@ deflate.c

trees.o: trees.c
	$(CC) $(CFLAGS) $(ZINC) -c -o $@ trees.c

zutil.o: zutil.c
	$(CC) $(CFLAGS) $(ZINC) -c -o $@ zutil.c

gzclose.o: gzclose.c
	$(CC) $(CFLAGS) $(ZINC) -c -o $@ gzclose.c

gzlib.o: gzlib.c
	$(CC) $(CFLAGS) $(ZINC) -c -o $@ gzlib.c

gzwrite.o: gzwrite.c
	$(CC) $(CFLAGS) $(ZINC) -c -o $@ gzwrite.c

minigzip$(EXE): minigzip.o $(STATICLIB)
	$(CC) $(CFLAGS) -o $@ minigzip.o $(TEST_LDFLAGS)
docs: zlib.3.pdf

zlib.3.pdf: zlib.3
	groff -mandoc -f H -T ps zlib.3 | ps2pdf - $@

zconf.h.cmakein: zconf.h.in
	-@ TEMPFILE=zconfh_$$; \
	echo "/#define ZCONF_H/ a\\\\\n#cmakedefine Z_PREFIX\\\\\n#cmakedefine Z_HAVE_UNISTD_H\n" >> $$TEMPFILE &&\
	sed -f $$TEMPFILE zconf.h.in > $@ &&\
	touch -r zconf.h.in $@ &&\
	rm $$TEMPFILE

zconf: zconf.h.in
	cp -p zconf.h.in zconf.h

mostlyclean: clean
clean:
	rm -f *.o *.lo *~ wm_tool$(EXE) libz.*
	rm -rf objs
	rm -f *.gcda *.gcno *.gcov
	rm -f contrib/infback9/*.gcda contrib/infback9/*.gcno contrib/infback9/*.gcov

maintainer-clean: distclean
distclean: clean zconf zconf.h.cmakein docs
	rm -f Makefile zlib.pc configure.log
	-@rm -f .DS_Store
	@if [ -f Makefile.in ]; then \
	printf 'all:\n\t-@echo "Please use ./configure first.  Thank you."\n' > Makefile ; \
	printf '\ndistclean:\n\tmake -f Makefile.in distclean\n' >> Makefile ; \
	touch -r Makefile.in Makefile ; fi
	@if [ ! -f zconf.h.in ]; then rm -f zconf.h zconf.h.cmakein ; fi
	@if [ ! -f zlib.3 ]; then rm -f zlib.3.pdf ; fi

tags:
	etags *.[ch]

adler32.o zutil.o: zutil.h zlib.h zconf.h
gzclose.o gzlib.o gzwrite.o: zlib.h gzguts.h zconf.h
minigzip.o: $(SRCDIR)zlib.h zconf.h
crc32.o: zutil.h zlib.h crc32.h zconf.h
deflate.o: deflate.h zutil.h zlib.h zconf.h
trees.o: deflate.h zutil.h zlib.h trees.h zconf.h
