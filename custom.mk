TARGET_EXEC ?= win_main

BUILD_DIR ?= build
#SRC_DIRS ?= src/ src/bitmessage src/cJSON src/cmnds src/devicegroups src/driver src/hal/win32 src/httpclient src/httpserver src/jsmn src/libraries src/littlefs src/logging src/mqtt src/ota src/win32

SRC_DIRS ?= src/

EXCLUDED_FILES ?= src/httpserver/http_tcp_server.c src/ota/ota.c src/cmnds/cmd_tcp.c src/memory/memtest.c src/new_ping.c src/win_main_scriptOnly.c src/driver/drv_ir2.c src/driver/drv_ir.cpp 

SRCS := $(filter-out $(EXCLUDED_FILES), $(wildcard $(shell find $(SRC_DIRS) -not \( -path "src/hal/bl602" -prune \) -not \( -path "src/hal/xr809" -prune \) -not \( -path "src/hal/w800" -prune \) -not \( -path "src/hal/bk7231" -prune \) -name *.c | sort -k 1nr | cut -f2-)))


#OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_DIRS := $(filter-out src/hal/bl602 src/hal/xr809 src/hal/w800 src/hal/bk7231 src/memory, $(wildcard $(INC_DIRS)))
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

LDLIBS := -lpthread -lm -lnsl

CPPFLAGS ?= $(INC_FLAGS) -MMD -MP -std=gnu99 -DWINDOWS -DLINUX

CFLAGS ?= -std=gnu99 -W -Wall -Wextra -g

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	@echo "Linking: $@"
	$(CC) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	@echo "Compiling: $< -> $@"
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

MKDIR_P ?= mkdir -p