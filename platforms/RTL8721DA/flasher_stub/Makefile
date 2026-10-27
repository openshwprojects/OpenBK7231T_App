.PHONY: all build clean

all: build

build:
	cd ../../../sdk/ameba-rtos/amebadplus_gcc_project && ./build.py -a ../../../platforms/RTL8721DA/flasher_stub -g flashloader

clean:
	rm RTL8721DA_Stub.bin
