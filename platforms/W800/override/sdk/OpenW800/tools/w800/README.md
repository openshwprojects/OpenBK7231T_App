**wm_tool** is a command line tool that provides firmware packaging, firmware download, and log capture for the w600 chip. 

wm_tool runs on Windows, Linux and Mac OS X.

Build
-----
```
cd zlib-1.2.11
make clean;make
cd ..
gcc wm_tool.c -lpthread -o wm_tool -L. zlib-1.2.11/libz.a
```

Windows system needs to be compiled in MSYS environment.
Linux/Mac system needs to install the gcc compiler.