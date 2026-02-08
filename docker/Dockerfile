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
	flex bison gperf python3-venv \
	build-essential cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0 gcc-arm-none-eabi

# Install Python packages required by build scripts (BL602 SDK requirements from Makefile line 162)
RUN pip3 install fdt toml configobj pycryptodomex

RUN useradd -m -s /bin/bash -u "${UID}" ${USERNAME}

# Install ESP-IDF
# using v4.4.4 as a stable baseline for OpenBeken/Tasmota style projects
ENV IDF_PATH=/opt/esp-idf
ENV IDF_TOOLS_PATH=/opt/espressif

# Prepare directories with correct permissions
RUN mkdir -p $IDF_PATH $IDF_TOOLS_PATH && \
	chown -R ${USERNAME}:${USERNAME} $IDF_PATH $IDF_TOOLS_PATH

USER ${USERNAME}

RUN git clone -b v5.1.2 --recursive https://github.com/espressif/esp-idf.git $IDF_PATH
RUN $IDF_PATH/install.sh

# Switch back to root briefly for system setup
USER root

COPY entrypoint.sh /entrypoint.sh
RUN chmod 775 /entrypoint.sh && chown ${USERNAME}:${USERNAME} /entrypoint.sh

ENV OPENBK7231T_APP_NAME="OpenBK7231T_App"

RUN mkdir -p /app/build && chown ${USERNAME}:${USERNAME} /app/build

WORKDIR /app/build

USER ${USERNAME}

ENTRYPOINT ["/entrypoint.sh"]
