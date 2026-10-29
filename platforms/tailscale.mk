ifneq ($(TARGET_PLATFORM),bk7231n)
$(error tailscaleC currently supports the BK7231N SDK only)
endif
ifeq ($(CFG_USE_MQTT_TLS),1)
$(error Use one shared mbedTLS configuration before combining MQTT TLS and tailscaleC)
endif

TSC_DIR = $(OBK_DIR)/libraries/tailscalec
TSC_MBEDTLS = $(OBK_DIR)/output/mbedtls-2.28.5
CPPDEFINES += -DENABLE_DRIVER_TAILSCALE=1 -DTSC_PLATFORM_LWIP=1
CPPDEFINES += -DINCLUDE_uxTaskGetStackHighWaterMark=1
OSFLAGS += -DINCLUDE_uxTaskGetStackHighWaterMark=1
./beken378/os/FreeRTOSv9.0.0/FreeRTOS/Source/tasks.o: $(OBK_DIR)/platforms/tailscale.mk
CPPDEFINES += -DTSC_CONFIG_HEADER='"openbeken.h"'
CPPDEFINES += -DMBEDTLS_CONFIG_FILE='"tailscale/mbedtls_config.h"'
INCLUDES := -I$(TSC_MBEDTLS)/include -I$(TSC_DIR)/include -I$(TSC_DIR)/profiles \
    -I$(OBK_DIR)/src -I$(TSC_DIR)/platform -I$(TSC_DIR)/src/control \
    -I$(TSC_DIR)/src/wireguard -I$(TSC_DIR)/third_party/wireguard \
    -I$(TSC_DIR)/third_party/wireguard/crypto/refc $(INCLUDES)
TSC_SOURCES = $(TSC_DIR)/src/tsc.c $(TSC_DIR)/src/stun.c $(TSC_DIR)/src/client.c
TSC_SOURCES += $(TSC_DIR)/src/packet.c $(TSC_DIR)/src/policy.c
TSC_SOURCES += $(wildcard $(TSC_DIR)/src/control/*.c)
TSC_SOURCES += $(wildcard $(TSC_DIR)/src/wireguard/*.c)
TSC_SOURCES += $(TSC_DIR)/src/discovery/nacl_box.c $(TSC_DIR)/src/discovery/disco.c
TSC_SOURCES += $(TSC_DIR)/third_party/wireguard/crypto/refc/blake2s.c
TSC_SOURCES += $(TSC_DIR)/third_party/wireguard/crypto/refc/chacha20.c
TSC_SOURCES += $(TSC_DIR)/platform/socket.c $(TSC_DIR)/platform/mbedtls/crypto.c
TSC_SOURCES += $(TSC_DIR)/platform/mbedtls/stream.c
TSC_SOURCES += $(wildcard $(TSC_MBEDTLS)/library/*.c)

# The SDK flattens source basenames into Debug/obj; crypto.c and chacha20.c
# would overwrite each other. Keep every external object uniquely named in a
# separate archive, with ordinary compiler-generated header dependencies.
TSC_BUILD = $(OBK_DIR)/output/$(APP_VERSION)/tailscalec
tsc_object = $(TSC_BUILD)/$(subst /,_,$(patsubst %.c,%.o,$(patsubst $(OBK_DIR)/%,%,$(1))))
TSC_OBJECTS = $(foreach s,$(TSC_SOURCES),$(call tsc_object,$(s)))
TSC_ARCHIVE = $(TSC_BUILD)/libtailscalec.a
define tsc_compile
$(call tsc_object,$(1)): $(1) $(OBK_DIR)/platforms/tailscale.mk
	@mkdir -p $$(TSC_BUILD)
	$$(CC) $$(CFLAGS) -std=c11 -fstack-usage $$(INCLUDES) -MMD -MP -c $$< -o $$@
endef
$(foreach s,$(TSC_SOURCES),$(eval $(call tsc_compile,$(s))))
$(TSC_ARCHIVE): $(TSC_OBJECTS)
	$(AR) rcs $@ $^
test_target: $(TSC_ARCHIVE)
LIBFLAGS += $(TSC_ARCHIVE)
-include $(TSC_OBJECTS:.o=.d)
