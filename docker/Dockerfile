FROM --platform=linux/amd64 ubuntu:20.04

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
                libc6-i386 make \
                build-essential cmake gcc-arm-none-eabi

RUN pip3 install pycryptodomex configobj toml fdt

RUN useradd -m -s /bin/bash -u "${UID}" ${USERNAME}

COPY build_target_platforms.sh /build_target_platforms.sh
RUN chmod 775 /build_target_platforms.sh

ENV OPENBK7231T_APP_NAME="OpenBK7231T_App"

RUN mkdir -p /${OpenBK7231T_App} && chown ${USERNAME} /${OpenBK7231T_App}

USER ${USERNAME}

CMD ["/build_target_platforms.sh"]
