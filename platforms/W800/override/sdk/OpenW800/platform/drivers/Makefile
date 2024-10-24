TOP_DIR = ../..
sinclude $(TOP_DIR)/tools/w800/conf.mk

ifndef PDIR
GEN_LIBS = libdrivers$(LIB_EXT)
COMPONENTS_libdrivers = 7816/lib7816$(LIB_EXT) 	\
						adc/libadc$(LIB_EXT)	\
						cpu/libcpu$(LIB_EXT)	\
						dma/libdma$(LIB_EXT)	\
						efuse/libefuse$(LIB_EXT)	\
						flash/libflash$(LIB_EXT) \
						gpio/libgpio$(LIB_EXT) \
						hspi/libhspi$(LIB_EXT) \
						i2c/libi2c$(LIB_EXT) \
						i2s/libi2s$(LIB_EXT) \
						internalflash/libinternalflash$(LIB_EXT) \
						io/libio$(LIB_EXT) \
						irq/libirq$(LIB_EXT) \
						lcd/liblcd$(LIB_EXT) \
						pmu/libpmu$(LIB_EXT) \
						psram/libpsram$(LIB_EXT) \
						pwm/libpwm$(LIB_EXT) \
						rtc/librtc$(LIB_EXT) \
						sasc/libsasc$(LIB_EXT) \
						spi/libspi$(LIB_EXT) \
						timer/libtimer$(LIB_EXT) \
						uart/libuart$(LIB_EXT) \
						watchdog/libwatchdog$(LIB_EXT) \
						sdio_host/libsdio_host$(LIB_EXT) \
						touchsensor/libwmtouchsensor$(LIB_EXT)
endif

#DEFINES +=

sinclude $(TOP_DIR)/tools/w800/rules.mk
INCLUDES := $(INCLUDES) -I $(PDIR)include
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile