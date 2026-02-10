ARG BUILDPLATFORM=linux/amd64
FROM --platform=$BUILDPLATFORM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive
ARG USERNAME=devuser
ARG UID=1000
ARG TZ="Etc/UTC"

RUN echo "$TZ" > /etc/timezone

RUN dpkg --add-architecture i386 \
	&& apt-get update \
	&& apt-get upgrade -y \
	&& apt-get install -y \
	wget git python3 python-is-python3 python3-pip \
	libc6-i386 make rsync \
	# ESP-IDF Dependencies
	flex bison gperf gawk python3-venv \
	build-essential cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0 gcc-arm-none-eabi wine

# Install Python packages required by build scripts (BL602 SDK requirements from Makefile line 162)
RUN pip3 install fdt toml configobj pycryptodomex

RUN useradd -m -s /bin/bash -u "${UID}" ${USERNAME}

# Install ARM GNU Toolchain used by GitHub Actions for XR platforms.
COPY gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2 /tmp/gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2
RUN mkdir -p /opt/toolchains && \
	tar -xjf /tmp/gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2 -C /opt/toolchains && \
	ln -sfn /opt/toolchains/gcc-arm-none-eabi-8-2019-q3-update /opt/arm-none-eabi-8-2019-q3 && \
	rm -f /tmp/gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2 && \
	chown -R ${USERNAME}:${USERNAME} /opt/toolchains /opt/arm-none-eabi-8-2019-q3

# Install ESP-IDF (align with GitHub Actions matrix for ESP32C5/C61)
ENV IDF_PATH=/opt/esp-idf
ENV IDF_TOOLS_PATH=/opt/espressif

# Prepare directories with correct permissions
RUN mkdir -p $IDF_PATH $IDF_TOOLS_PATH /opt/rtk-toolchain && \
	chown -R ${USERNAME}:${USERNAME} $IDF_PATH $IDF_TOOLS_PATH /opt/rtk-toolchain
RUN mkdir -p /app/espressif-cache /app/esp8266-cache /app/mbedtls-cache /app/csky-w800-cache /app/csky-txw-cache /app/pip-cache && \
	chown -R ${USERNAME}:${USERNAME} /app/espressif-cache /app/esp8266-cache /app/mbedtls-cache /app/csky-w800-cache /app/csky-txw-cache /app/pip-cache

USER ${USERNAME}

RUN git clone -b v5.5.2 --recursive https://github.com/espressif/esp-idf.git $IDF_PATH
RUN $IDF_PATH/install.sh
RUN if [ ! -f /opt/esp-idf/tools/cmake/toolchain-esp32c5.cmake ] && [ -f /opt/esp-idf/tools/cmake/toolchain-esp32c6.cmake ]; then \
	ln -s /opt/esp-idf/tools/cmake/toolchain-esp32c6.cmake /opt/esp-idf/tools/cmake/toolchain-esp32c5.cmake; \
	fi && \
	if [ ! -f /opt/esp-idf/tools/cmake/toolchain-esp32c61.cmake ] && [ -f /opt/esp-idf/tools/cmake/toolchain-esp32c6.cmake ]; then \
	ln -s /opt/esp-idf/tools/cmake/toolchain-esp32c6.cmake /opt/esp-idf/tools/cmake/toolchain-esp32c61.cmake; \
	fi
RUN LEGACY_XTENSA_DIR="/opt/espressif/tools/xtensa-esp32-elf/esp-12.2.0_20230208/xtensa-esp32-elf" && \
	MODERN_XTENSA_DIR="$(find /opt/espressif/tools/xtensa-esp-elf -mindepth 2 -maxdepth 2 -type d -name xtensa-esp-elf | head -n 1)" && \
	if [ -n "$MODERN_XTENSA_DIR" ]; then \
		mkdir -p "$(dirname "$LEGACY_XTENSA_DIR")" && \
		rm -f "$LEGACY_XTENSA_DIR" && \
		ln -s "$MODERN_XTENSA_DIR" "$LEGACY_XTENSA_DIR"; \
	fi
RUN IDF_PIP="$(find /opt/espressif/python_env -maxdepth 4 -type f -path '*/idf5*_py3*_env/bin/pip' | head -n 1)" && \
	test -n "$IDF_PIP" && \
	"$IDF_PIP" install \
	json5 \
	python-mbedtls \
	pyDes \
	pycryptodome \
	pycryptodomex \
	sslcrypto \
	littlefs-python \
	pyfatfs \
	fdt \
	toml \
	configobj

# Switch back to root briefly for system setup
USER root

COPY entrypoint.sh /entrypoint.sh
RUN chmod 775 /entrypoint.sh && chown ${USERNAME}:${USERNAME} /entrypoint.sh

ENV OPENBK7231T_APP_NAME="OpenBK7231T_App"

RUN mkdir -p /app/build && chown ${USERNAME}:${USERNAME} /app/build

WORKDIR /app/build

USER ${USERNAME}

ENTRYPOINT ["/entrypoint.sh"]

