.PHONY: all build clean

all: build

build:
	cd ../../../sdk/ameba-rtos/amebalite_gcc_project && ./build.py -a ../../../platforms/RTL8720E/flasher_stub -g flashloader

clean:
	rm RTL8720E_Stub.bin
