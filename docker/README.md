# OpenBeken Docker Build Guide

This folder contains the local Docker-based build system used to run OpenBeken builds with shared caches and repeatable test runs.

## Prerequisites

1. Docker Desktop running
2. Python 3 in `PATH`
3. Git in `PATH`

## Repository Setup (Required)

Fresh clone:

```powershell
git clone --recursive https://github.com/openshwprojects/OpenBK7231T_App.git
cd OpenBK7231T_App
```

If the repository was cloned without submodules:

```powershell
git submodule update --init --recursive
```

## Docker folder files

- `docker/build_tool.py`: interactive single-platform builder
- `docker/GUI_build_tool.py`: GUI wrapper for `build_tool.py`
- `docker/OpenBK_GUI_Build_Tool.exe`: packaged Windows GUI executable for the Docker build tool
- `docker/entrypoint.sh`: runtime setup, sync, patching, and cache hydration inside container
- `docker/Dockerfile`: build image definition
- `docker/gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2`: required ARM toolchain archive used during Docker image build

## Quick Start

From repository root:

```powershell
python docker/build_tool.py
```

Launch GUI:

```powershell
python docker/GUI_build_tool.py
```

## CLI Help and Options

Show CLI help:

```powershell
python docker/build_tool.py --help
```

Supported options for `docker/build_tool.py`:

- `--clean`: perform a clean build (`make clean`) before building
- `--no-cache`: rebuild the Docker image with layer cache disabled
- `--flash-size <size>`: set flash size (for example `2MB`, `4MB`, `8MB`)
- `--txw-packager <mode>`: TXW packaging mode (`auto`, `wine`, `fallback`) for TXW builds only
- `--src <path>`: repository root path when running from outside the repo root

Examples:

```powershell
# Standard interactive run
python docker/build_tool.py

# Clean build with explicit flash size
python docker/build_tool.py --clean --flash-size 4MB

# Force Docker image rebuild without layer cache
python docker/build_tool.py --no-cache

# Set TXW packager mode explicitly
python docker/build_tool.py --txw-packager fallback

# Run from another directory by pointing to repo root
python docker/build_tool.py --src "E:\\OBK"
```

## Cache Volumes

Default volumes used by current scripts:

- `openbk_build_data`
- `openbk_rtk_toolchain`
- `openbk_espressif_tools`
- `openbk_esp8266_tools`
- `openbk_mbedtls_cache`
- `openbk_csky_w800_cache`
- `openbk_csky_txw_cache`
- `openbk_pip_cache`

These are reused across runs for speed.

## Optional `important_files` Seeding

`docker/important_files` is optional. If present, `entrypoint.sh` uses it to seed caches on early runs.

Recognized inputs:

- `docker/important_files/python-wheels/` (offline Python wheels)
- `docker/important_files/mbedtls-2.28.5-gpl.tgz`
- `docker/important_files/csky-elf-noneabiv2-tools-x86_64-newlib-20250328.tar.gz`
- `docker/important_files/csky-elfabiv2-tools-x86_64-minilibc-20230301.tar.gz`
- `docker/important_files/esp-idf-tools/dist/*`
- `docker/important_files/esp-idf-tools/idf-env.json`
- `docker/important_files/asdk-10.3.1-linux-newlib-build-4354-x86_64_with_small_reent.tar.bz2`

If these files are not present, the build still proceeds using upstream download/install paths.

## Important Note About Fresh Clones

Current Docker image build expects this file to exist in `docker/`:

- `gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2`

`Dockerfile` uses a hard `COPY` for it. If missing, `docker build` fails immediately.

`Disclamer` - 100% of testing was done on Win11 PC, if able test on linux and Mac and report issues!
