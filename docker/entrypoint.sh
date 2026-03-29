#!/bin/bash
set -e

# Arguments passed from Docker command
PLATFORM=$1

# Source (Host Mount) and Build (Volume) directories
SOURCE_DIR="/app/source"
BUILD_DIR="/app/build"
IMPORTANT_DIR="$SOURCE_DIR/docker/important_files"
MBEDTLS_CACHE_DIR="/app/mbedtls-cache"
CSKY_W800_CACHE_DIR="/app/csky-w800-cache"
CSKY_TXW_CACHE_DIR="/app/csky-txw-cache"
PIP_CACHE_DIR_CANDIDATE="/app/pip-cache"

echo "Checking build environment..."

# Use dedicated pip cache volume when available.
if [ -w "$PIP_CACHE_DIR_CANDIDATE" ]; then
    export PIP_CACHE_DIR="$PIP_CACHE_DIR_CANDIDATE"
    mkdir -p "$PIP_CACHE_DIR"
fi

# Ensure Python packages are installed for BL602 SDK
echo "Ensuring Python packages for BL602 SDK..."
if [ -d "$IMPORTANT_DIR/python-wheels" ]; then
    pip3 install --quiet --no-index --find-links "$IMPORTANT_DIR/python-wheels" \
        fdt toml configobj pycryptodomex 2>/dev/null || \
    pip3 install --quiet fdt toml configobj pycryptodomex 2>/dev/null || true
else
    pip3 install --quiet fdt toml configobj pycryptodomex 2>/dev/null || true
fi


# Smart Sync Strategy
# CLEAN_BUILD=1 means we must refresh SDK content too, otherwise stale/corrupted
# SDK files in the persistent build volume can survive indefinitely.
if [ "$CLEAN_BUILD" = "1" ]; then
    echo "Clean build requested: full sync of source to build volume (including SDKs)..."
    rsync -rltD --no-owner --no-group --info=progress2 \
        --exclude 'docker/test_runs/' \
        "$SOURCE_DIR/" "$BUILD_DIR/" || true
elif [ ! -d "$BUILD_DIR/sdk" ]; then
    echo "Initial setup: Full sync of source to build volume..."
    # First run: Sync everything including SDKs
    rsync -rltD --no-owner --no-group --info=progress2 \
        --exclude 'docker/test_runs/' \
        "$SOURCE_DIR/" "$BUILD_DIR/" || true
else
    echo "Incremental build: Syncing application code (excluding SDKs)..."
    # Subsequent runs: Sync everything EXCEPT sdk/
    rsync -rltD --no-owner --no-group --info=progress2 \
        --exclude 'sdk/' \
        --exclude '.git/' \
        --exclude 'docker/test_runs/' \
        "$SOURCE_DIR/" "$BUILD_DIR/" || true
fi

# Seed mbedtls cache from local archive and expose it at output/mbedtls-2.28.5.
MBEDTLS_LOCAL_ARCHIVE="$IMPORTANT_DIR/mbedtls-2.28.5-gpl.tgz"
MBEDTLS_TARGET_DIR="$BUILD_DIR/output/mbedtls-2.28.5"
MBEDTLS_CACHE_EXTRACTED_DIR="$MBEDTLS_CACHE_DIR/mbedtls-2.28.5"
if [ -d "$MBEDTLS_CACHE_DIR" ]; then
    mkdir -p "$BUILD_DIR/output"
    if [ ! -d "$MBEDTLS_CACHE_EXTRACTED_DIR" ] && [ -f "$MBEDTLS_LOCAL_ARCHIVE" ]; then
        echo "Seeding mbedtls cache from local important_files archive..."
        TMP_MBEDTLS_EXTRACT="$MBEDTLS_CACHE_DIR/.extract.$$"
        rm -rf "$TMP_MBEDTLS_EXTRACT"
        mkdir -p "$TMP_MBEDTLS_EXTRACT"
        if tar -xzf "$MBEDTLS_LOCAL_ARCHIVE" -C "$TMP_MBEDTLS_EXTRACT" 2>/dev/null; then
            if [ -d "$TMP_MBEDTLS_EXTRACT/mbedtls-mbedtls-2.28.5" ]; then
                mv "$TMP_MBEDTLS_EXTRACT/mbedtls-mbedtls-2.28.5" "$MBEDTLS_CACHE_EXTRACTED_DIR"
                if [ -f "$MBEDTLS_CACHE_EXTRACTED_DIR/library/base64.c" ] && [ ! -f "$MBEDTLS_CACHE_EXTRACTED_DIR/library/base64_mbedtls.c" ]; then
                    mv "$MBEDTLS_CACHE_EXTRACTED_DIR/library/base64.c" "$MBEDTLS_CACHE_EXTRACTED_DIR/library/base64_mbedtls.c"
                fi
            fi
        fi
        rm -rf "$TMP_MBEDTLS_EXTRACT"
    fi
    if [ -d "$MBEDTLS_CACHE_EXTRACTED_DIR" ] && [ ! -e "$MBEDTLS_TARGET_DIR" ]; then
        ln -sfn "$MBEDTLS_CACHE_EXTRACTED_DIR" "$MBEDTLS_TARGET_DIR"
    fi
fi

# Seed W800 csky cache.
W800_CSKY_ARCHIVE="$IMPORTANT_DIR/csky-elf-noneabiv2-tools-x86_64-newlib-20250328.tar.gz"
W800_CSKY_TARGET_DIR="$BUILD_DIR/sdk/OpenW800/tools/w800/csky"
if [ -d "$CSKY_W800_CACHE_DIR" ]; then
    if [ ! -d "$CSKY_W800_CACHE_DIR/bin" ] && [ -f "$W800_CSKY_ARCHIVE" ]; then
        echo "Seeding W800 csky cache from local important_files archive..."
        tar -xzf "$W800_CSKY_ARCHIVE" -C "$CSKY_W800_CACHE_DIR" 2>/dev/null || true
    fi
    mkdir -p "$W800_CSKY_TARGET_DIR"
    if [ -d "$CSKY_W800_CACHE_DIR/bin" ] && [ ! -e "$W800_CSKY_TARGET_DIR/bin" ]; then
        ln -sfn "$CSKY_W800_CACHE_DIR/bin" "$W800_CSKY_TARGET_DIR/bin"
    fi
    if [ -d "$CSKY_W800_CACHE_DIR/x86_64-linux" ] && [ ! -e "$W800_CSKY_TARGET_DIR/x86_64-linux" ]; then
        ln -sfn "$CSKY_W800_CACHE_DIR/x86_64-linux" "$W800_CSKY_TARGET_DIR/x86_64-linux"
    fi
    if [ -d "$W800_CSKY_TARGET_DIR/bin" ] && [ ! -f "$W800_CSKY_TARGET_DIR/got_csky-elf-noneabiv2-tools-x86_64-newlib-20250328" ]; then
        touch "$W800_CSKY_TARGET_DIR/got_csky-elf-noneabiv2-tools-x86_64-newlib-20250328"
    fi
fi

# Seed TXW csky cache.
TXW_CSKY_ARCHIVE="$IMPORTANT_DIR/csky-elfabiv2-tools-x86_64-minilibc-20230301.tar.gz"
TXW_CSKY_TARGET_PARENT="$BUILD_DIR/sdk/OpenTXW81X/tools/gcc"
TXW_CSKY_CACHE_EXTRACTED_DIR="$CSKY_TXW_CACHE_DIR/csky-elfabiv2"
if [ -d "$CSKY_TXW_CACHE_DIR" ]; then
    if [ ! -d "$TXW_CSKY_CACHE_EXTRACTED_DIR/bin" ] && [ -f "$TXW_CSKY_ARCHIVE" ]; then
        echo "Seeding TXW csky cache from local important_files archive..."
        mkdir -p "$TXW_CSKY_CACHE_EXTRACTED_DIR"
        tar -xzf "$TXW_CSKY_ARCHIVE" -C "$TXW_CSKY_CACHE_EXTRACTED_DIR" 2>/dev/null || true
    fi
    mkdir -p "$TXW_CSKY_TARGET_PARENT"
    if [ -d "$TXW_CSKY_CACHE_EXTRACTED_DIR" ] && [ ! -e "$TXW_CSKY_TARGET_PARENT/csky-elfabiv2" ]; then
        ln -sfn "$TXW_CSKY_CACHE_EXTRACTED_DIR" "$TXW_CSKY_TARGET_PARENT/csky-elfabiv2"
    fi
    if [ -d "$TXW_CSKY_CACHE_EXTRACTED_DIR/bin" ] && [ ! -e "$TXW_CSKY_TARGET_PARENT/bin" ]; then
        ln -sfn "$TXW_CSKY_CACHE_EXTRACTED_DIR/bin" "$TXW_CSKY_TARGET_PARENT/bin"
    fi
    if [ -d "$TXW_CSKY_CACHE_EXTRACTED_DIR/x86_64-linux" ] && [ ! -e "$TXW_CSKY_TARGET_PARENT/x86_64-linux" ]; then
        ln -sfn "$TXW_CSKY_CACHE_EXTRACTED_DIR/x86_64-linux" "$TXW_CSKY_TARGET_PARENT/x86_64-linux"
    fi
fi

# Source build environment after sync so platform-specific SDK paths exist.
if [ "$PLATFORM" = "OpenESP8266" ] && [ -f "$BUILD_DIR/sdk/OpenESP8266/export.sh" ]; then
    echo "Sourcing OpenESP8266 environment..."
    # Force fresh CMake configure for OpenESP8266 to avoid stale toolchain cache.
    rm -rf "$BUILD_DIR/platforms/ESP8266/build"
    unset CMAKE_TOOLCHAIN_FILE
    export IDF_PATH="$BUILD_DIR/sdk/OpenESP8266"
    # Persist ESP8266 toolchain cache in dedicated volume; fallback to build volume if needed.
    if [ -w "/app/esp8266-cache" ]; then
        export IDF_TOOLS_PATH="/app/esp8266-cache"
    else
        export IDF_TOOLS_PATH="$BUILD_DIR/.espressif"
    fi
    mkdir -p "$IDF_TOOLS_PATH"
    if [ -d "$IMPORTANT_DIR/esp-idf-tools/dist" ] && [ ! -d "$IDF_TOOLS_PATH/dist" ]; then
        echo "Hydrating OpenESP8266 tools cache from local important_files..."
        mkdir -p "$IDF_TOOLS_PATH/dist"
        cp -f "$IMPORTANT_DIR/esp-idf-tools/dist/"* "$IDF_TOOLS_PATH/dist/" 2>/dev/null || true
    fi
    if [ -f "$IMPORTANT_DIR/esp-idf-tools/idf-env.json" ] && [ ! -f "$IDF_TOOLS_PATH/idf-env.json" ]; then
        cp -f "$IMPORTANT_DIR/esp-idf-tools/idf-env.json" "$IDF_TOOLS_PATH/idf-env.json" 2>/dev/null || true
    fi
    if [ ! -d "$IDF_TOOLS_PATH/tools/xtensa-lx106-elf" ] && [ -f "$IDF_PATH/install.sh" ]; then
        echo "Installing OpenESP8266 toolchain (first run)..."
        (cd "$IDF_PATH" && ./install.sh)
    fi
    . "$IDF_PATH/export.sh"
    if [ -f "$IDF_PATH/add_path.sh" ]; then
        . "$IDF_PATH/add_path.sh"
    fi
elif [[ "$PLATFORM" == OpenESP32* ]] && [ -n "$IDF_PATH" ] && [ -f "$IDF_PATH/export.sh" ]; then
    echo "Sourcing ESP-IDF environment..."
    # Prefer a dedicated volume cache, but fall back to build-volume cache if not writable.
    PREFERRED_IDF_TOOLS_PATH="/app/espressif-cache"
    FALLBACK_IDF_TOOLS_PATH="$BUILD_DIR/.espressif"
    mkdir -p "$PREFERRED_IDF_TOOLS_PATH" 2>/dev/null || true
    if [ -w "$PREFERRED_IDF_TOOLS_PATH" ]; then
        export IDF_TOOLS_PATH="$PREFERRED_IDF_TOOLS_PATH"
    else
        export IDF_TOOLS_PATH="$FALLBACK_IDF_TOOLS_PATH"
        mkdir -p "$IDF_TOOLS_PATH"
        echo "ESP-IDF cache volume not writable; using fallback cache: $IDF_TOOLS_PATH"
    fi

    IDF_TARGET=""
    case "$PLATFORM" in
        OpenESP32) IDF_TARGET="esp32" ;;
        OpenESP32C2) IDF_TARGET="esp32c2" ;;
        OpenESP32C3) IDF_TARGET="esp32c3" ;;
        OpenESP32C5) IDF_TARGET="esp32c5" ;;
        OpenESP32C6) IDF_TARGET="esp32c6" ;;
        OpenESP32C61) IDF_TARGET="esp32c61" ;;
        OpenESP32S2) IDF_TARGET="esp32s2" ;;
        OpenESP32S3) IDF_TARGET="esp32s3" ;;
    esac

    if [ -n "$IDF_TARGET" ] && [ -f "$IDF_PATH/install.sh" ]; then
        if [ -d "$IMPORTANT_DIR/esp-idf-tools/dist" ] && [ ! -d "$IDF_TOOLS_PATH/dist" ]; then
            echo "Hydrating ESP-IDF tools cache from local important_files..."
            mkdir -p "$IDF_TOOLS_PATH/dist"
            cp -f "$IMPORTANT_DIR/esp-idf-tools/dist/"* "$IDF_TOOLS_PATH/dist/" 2>/dev/null || true
        fi
        if [ -f "$IMPORTANT_DIR/esp-idf-tools/idf-env.json" ] && [ ! -f "$IDF_TOOLS_PATH/idf-env.json" ]; then
            cp -f "$IMPORTANT_DIR/esp-idf-tools/idf-env.json" "$IDF_TOOLS_PATH/idf-env.json" 2>/dev/null || true
        fi
        echo "Ensuring ESP-IDF tools for $IDF_TARGET (cache: $IDF_TOOLS_PATH)..."
        (cd "$IDF_PATH" && ./install.sh "$IDF_TARGET")
    fi

    . "$IDF_PATH/export.sh"

    # Keep runtime Python deps in the cached IDF venv to avoid regressions in mixed-platform runs.
    IDF_PIP="$(find "$IDF_TOOLS_PATH/python_env" -maxdepth 4 -type f -path '*/idf5*_py3*_env/bin/pip' | head -n 1)"
    if [ -n "$IDF_PIP" ]; then
        if [ -d "$IMPORTANT_DIR/python-wheels" ]; then
            "$IDF_PIP" install --quiet --no-index --find-links "$IMPORTANT_DIR/python-wheels" \
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
                configobj 2>/dev/null || \
            "$IDF_PIP" install --quiet \
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
                configobj 2>/dev/null || true
        else
            "$IDF_PIP" install --quiet \
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
                configobj 2>/dev/null || true
        fi
    fi
elif [ -n "$IDF_PATH" ] && [ -f "$IDF_PATH/export.sh" ]; then
    echo "Sourcing ESP-IDF environment..."
    . "$IDF_PATH/export.sh"
fi

# Seed Realtek toolchain archives from local important_files if provided.
if [ -d "/opt/rtk-toolchain" ]; then
    RTL_TOOLCHAIN_ARCHIVES=(
        "asdk-10.3.1-linux-newlib-build-4354-x86_64_with_small_reent.tar.bz2"
        "vsdk-10.3.1-linux-newlib-build-4353-x86_64_with_small_reent.tar.bz2"
    )
    for rtl_archive in "${RTL_TOOLCHAIN_ARCHIVES[@]}"; do
        rtl_local_archive="$IMPORTANT_DIR/$rtl_archive"
        rtl_cache_archive="/opt/rtk-toolchain/$rtl_archive"
        if [ -f "$rtl_local_archive" ] && [ ! -f "$rtl_cache_archive" ]; then
            echo "Seeding /opt/rtk-toolchain from local $rtl_archive..."
            cp -f "$rtl_local_archive" "$rtl_cache_archive" 2>/dev/null || true
        elif [ ! -f "$rtl_cache_archive" ]; then
            rtl_url=""
            case "$rtl_archive" in
                asdk-10.3.1-linux-newlib-build-4354-x86_64_with_small_reent.tar.bz2)
                    rtl_url="https://github.com/Ameba-AIoT/ameba-toolchain/releases/download/10.3.1_v5//asdk-10.3.1-linux-newlib-build-4354-x86_64_with_small_reent.tar.bz2"
                    ;;
                vsdk-10.3.1-linux-newlib-build-4353-x86_64_with_small_reent.tar.bz2)
                    rtl_url="https://github.com/Ameba-AIoT/ameba-toolchain/releases/download/10.3.1_v5//vsdk-10.3.1-linux-newlib-build-4353-x86_64_with_small_reent.tar.bz2"
                    ;;
            esac

            # First-run self-seeding: hydrate the shared volume from upstream once.
            if [ -n "$rtl_url" ]; then
                echo "Seeding /opt/rtk-toolchain from upstream $rtl_archive (first run)..."
                if command -v wget >/dev/null 2>&1; then
                    wget -O "$rtl_cache_archive" "$rtl_url" >/dev/null 2>&1 || rm -f "$rtl_cache_archive"
                elif command -v curl >/dev/null 2>&1; then
                    curl -L --fail -o "$rtl_cache_archive" "$rtl_url" >/dev/null 2>&1 || rm -f "$rtl_cache_archive"
                fi
            fi
        fi
    done
fi

# Ensure ARM GCC path is exported for XR/LN Makefile targets that consume
# ARM_NONE_EABI_GCC_PATH and otherwise fall back to invalid /arm-none-eabi-gcc.
# XR platforms in CI use ARM GNU Toolchain 8-2019-q3.
ARM_GCC_XR_CI_BIN="/opt/arm-none-eabi-8-2019-q3/bin"
ARM_GCC_LEGACY_CANDIDATES=(
    "$BUILD_DIR/sdk/OpenBK7231N/platforms/bk7231n/toolchain/gcc-arm-none-eabi-4_9-2015q1/bin/arm-none-eabi-gcc"
    "$BUILD_DIR/sdk/OpenBK7231T/platforms/bk7231t/toolchain/gcc-arm-none-eabi-4_9-2015q1/bin/arm-none-eabi-gcc"
)

if [ "$PLATFORM" = "OpenXR806" ] || [ "$PLATFORM" = "OpenXR809" ] || [ "$PLATFORM" = "OpenXR872" ]; then
    if [ -x "$ARM_GCC_XR_CI_BIN/arm-none-eabi-gcc" ]; then
        export ARM_NONE_EABI_GCC_PATH="$ARM_GCC_XR_CI_BIN"
    elif command -v arm-none-eabi-gcc >/dev/null 2>&1; then
        export ARM_NONE_EABI_GCC_PATH="$(dirname "$(command -v arm-none-eabi-gcc)")"
    fi
elif [ "$PLATFORM" = "OpenLN882H" ] || [ "$PLATFORM" = "OpenLN8825" ]; then
    # LN SDK requires newer GCC flags (e.g. -ffile-prefix-map) not supported by
    # BK legacy 4.9 toolchain. Prefer modern system/CI toolchains.
    if command -v arm-none-eabi-gcc >/dev/null 2>&1; then
        export ARM_NONE_EABI_GCC_PATH="$(dirname "$(command -v arm-none-eabi-gcc)")"
    elif [ -x "$ARM_GCC_XR_CI_BIN/arm-none-eabi-gcc" ]; then
        export ARM_NONE_EABI_GCC_PATH="$ARM_GCC_XR_CI_BIN"
    fi
else
    for arm_gcc in "${ARM_GCC_LEGACY_CANDIDATES[@]}"; do
        if [ -x "$arm_gcc" ]; then
            export ARM_NONE_EABI_GCC_PATH="$(dirname "$arm_gcc")"
            break
        fi
    done
    if [ -z "$ARM_NONE_EABI_GCC_PATH" ] && [ -x "$ARM_GCC_XR_CI_BIN/arm-none-eabi-gcc" ]; then
        export ARM_NONE_EABI_GCC_PATH="$ARM_GCC_XR_CI_BIN"
    elif [ -z "$ARM_NONE_EABI_GCC_PATH" ] && command -v arm-none-eabi-gcc >/dev/null 2>&1; then
        export ARM_NONE_EABI_GCC_PATH="$(dirname "$(command -v arm-none-eabi-gcc)")"
    fi
fi

if [ -n "$ARM_NONE_EABI_GCC_PATH" ]; then
    echo "Using ARM_NONE_EABI_GCC_PATH=$ARM_NONE_EABI_GCC_PATH"
else
    echo "Warning: arm-none-eabi-gcc not found in PATH"
fi

# Apply custom configuration
if [ -f "/app/custom_config.h" ]; then
    echo "Applying custom configuration..."
    cp "/app/custom_config.h" "$BUILD_DIR/src/obk_config.h"
fi

# BK beken_freertos platforms currently fail link when ROOMBA/BMP280 are enabled:
# the corresponding driver objects are not present in this SDK build path.
# Enforce disable for affected targets after config copy (also covers driver-mode-all).
if [ "$PLATFORM" = "OpenBK7231U" ] || [ "$PLATFORM" = "OpenBK7238" ] || [ "$PLATFORM" = "OpenBK7252" ] || [ "$PLATFORM" = "OpenBK7252N" ]; then
    CFG_FILE="$BUILD_DIR/src/obk_config.h"
    if [ -f "$CFG_FILE" ] && ! grep -q "OBK_DOCKER_DISABLE_BK_ROOMBA_BMP280" "$CFG_FILE"; then
        echo "Patching obk_config.h to disable unsupported ROOMBA/BMP280 on $PLATFORM..."
        {
            echo ""
            echo "// OBK_DOCKER_DISABLE_BK_ROOMBA_BMP280"
            echo "#if PLATFORM_BK7231U || PLATFORM_BK7238 || PLATFORM_BK7252 || PLATFORM_BK7252N"
            echo "#undef ENABLE_DRIVER_ROOMBA"
            echo "#undef ENABLE_DRIVER_BMP280"
            echo "#endif"
        } >> "$CFG_FILE"
    fi
fi

# Fix Windows line endings for critical scripts
echo "Fixing line endings for scripts..."
if [ -f "$BUILD_DIR/libraries/berry/tools/coc/coc" ]; then
    sed -i 's/\r$//' "$BUILD_DIR/libraries/berry/tools/coc/coc"
    chmod +x "$BUILD_DIR/libraries/berry/tools/coc/coc"
fi
find "$BUILD_DIR" -name "*.sh" -exec sed -i 's/\r$//' {} + || true
find "$BUILD_DIR" -name "*.py" -exec sed -i 's/\r$//' {} + || true

# Fix permissions if needed (simple approach for now)
# owner of files in volume might be issue if UID mismatch, but we are running as user
# Assuming rsync handles it reasonably well for our context.

# -----------------------------------------------------------------------------
# Runtime Patching (To keep repo clean)
# -----------------------------------------------------------------------------
echo "Applying runtime patches to build files..."

# 1. Patch Makefile: Add toolchain file to cmake invocations
# WE USE REGEX CAPTURE here to exact the target name (e.g. "esp32c3") from the command line itself.
# Match: IDF_TARGET="([a-zA-Z0-9_]+)" ... cmake platforms/ESP-IDF
# Replace with: IDF_TARGET="\1" ... cmake -DCMAKE_TOOLCHAIN_FILE=$(IDF_PATH)/tools/cmake/toolchain-\1.cmake platforms/ESP-IDF
sed -i -E 's|IDF_TARGET="([^"]+)"(.*)cmake platforms/ESP-IDF|IDF_TARGET="\1"\2cmake -DCMAKE_TOOLCHAIN_FILE=$(IDF_PATH)/tools/cmake/toolchain-\1.cmake platforms/ESP-IDF|g' "$BUILD_DIR/Makefile"

# 2. Patch CMakeLists.txt: Fix relative paths
TARGET_CMAKE="$BUILD_DIR/platforms/ESP-IDF/main/CMakeLists.txt"
if [ -f "$TARGET_CMAKE" ]; then
    # Replace "$ENV{IDF_PATH}/../../" with "${CMAKE_CURRENT_LIST_DIR}/../../"
    # Logic Fix:
    # 1. obk_main.cmake is in platforms/ (../../ relative to main)
    #    Original: $ENV{IDF_PATH}/../../platforms/obk_main.cmake (Assuming IDF_PATH=sdk/esp-idf -> ../..=root -> root/platforms/obk_main.cmake)
    #    Target: ${CMAKE_CURRENT_LIST_DIR}/../../obk_main.cmake
    sed -i 's|\$ENV{IDF_PATH}/../../platforms/obk_main.cmake|${CMAKE_CURRENT_LIST_DIR}/../../obk_main.cmake|g' "$TARGET_CMAKE"
    
    # 2. berry.cmake is in libraries/ (../../../ relative to main)
    #    Original: $ENV{IDF_PATH}/../../libraries/berry.cmake
    #    Target: ${CMAKE_CURRENT_LIST_DIR}/../../../libraries/berry.cmake
    sed -i 's|\$ENV{IDF_PATH}/../../libraries/berry.cmake|${CMAKE_CURRENT_LIST_DIR}/../../../libraries/berry.cmake|g' "$TARGET_CMAKE"
    
    # 3. Fix mqtt_patched.c path (../../../libraries/mqtt_patched.c)
    #    Original: ../../../libraries/mqtt_patched.c (appears in PROJ_ALL_SRC list)
    #    Target: ${CMAKE_CURRENT_LIST_DIR}/../../../libraries/mqtt_patched.c
    #    NOTE: We match the line with the specific file to avoid catching other ../../../ occurrences accidentally
    sed -i 's|\.\./\.\./\.\./libraries/mqtt_patched.c|${CMAKE_CURRENT_LIST_DIR}/../../../libraries/mqtt_patched.c|g' "$TARGET_CMAKE"
    
    # Replace "../../../" with "${CMAKE_CURRENT_LIST_DIR}/../../../" 
    # Be specific about the variable assignments to avoid breaking other things
    sed -i 's|set(OBK_SRCS "\.\./\.\./\.\./src/")|set(OBK_SRCS "${CMAKE_CURRENT_LIST_DIR}/../../../src/")|g' "$TARGET_CMAKE"
    # Also patch BERRY_SRCPATH to have the correct relative path:
    sed -i 's|set(BERRY_SRCPATH "\.\./\.\./\.\./libraries/berry/src")|set(BERRY_SRCPATH "${CMAKE_CURRENT_LIST_DIR}/../../../libraries/berry/src")|g' "$TARGET_CMAKE"

    # 3. Patch CMakeLists.txt: Fix Component Requirements (downgrade v5.x -> v4.4)
    # Remove v5.x specific drivers and use generic 'driver'
    sed -i 's|PRIV_REQUIRES .*|PRIV_REQUIRES mqtt lwip esp_wifi nvs_flash bt esp_pm app_update driver spi_flash)|g' "$TARGET_CMAKE"
    
    # 4. Remove WHOLE_ARCHIVE (it seems to be misinterpreted as a source file)
    # sed -i 's|WHOLE_ARCHIVE||g' "$TARGET_CMAKE"
    sed -i '/idf_component_register/i message(STATUS "DEBUG: CMAKE_CURRENT_LIST_DIR=${CMAKE_CURRENT_LIST_DIR}")' "$TARGET_CMAKE"

    echo "Patched CMakeLists.txt:"
    cat "$TARGET_CMAKE"
else
    echo "Warning: $TARGET_CMAKE not found, skipping patch."
fi

# 2b. Patch ESP8266 CMakeLists.txt: Fix IDF-relative include paths
TARGET_CMAKE_ESP8266="$BUILD_DIR/platforms/ESP8266/main/CMakeLists.txt"
if [ -f "$TARGET_CMAKE_ESP8266" ]; then
    # Keep paths rooted in the checked-out tree inside /app/build.
    sed -i 's|\$ENV{IDF_PATH}/../../platforms/obk_main.cmake|${CMAKE_CURRENT_LIST_DIR}/../../obk_main.cmake|g' "$TARGET_CMAKE_ESP8266"
    sed -i 's|\$ENV{IDF_PATH}/../../libraries/berry.cmake|${CMAKE_CURRENT_LIST_DIR}/../../../libraries/berry.cmake|g' "$TARGET_CMAKE_ESP8266"
fi

# 2c. Patch ESP8266 root CMakeLists.txt for IDF v5 toolchain behavior
TARGET_ROOT_CMAKE_ESP8266="$BUILD_DIR/platforms/ESP8266/CMakeLists.txt"
if [ -f "$TARGET_ROOT_CMAKE_ESP8266" ]; then
    sed -i 's|include_directories(\"\\$ENV{IDF_PATH}/../../libraries/berry/src\")|include_directories(\"${CMAKE_CURRENT_LIST_DIR}/../../libraries/berry/src\")|g' "$TARGET_ROOT_CMAKE_ESP8266"
    sed -i 's|include_directories(\"\\$ENV{IDF_PATH}/../../include\")|include_directories(\"${CMAKE_CURRENT_LIST_DIR}/../../include\")|g' "$TARGET_ROOT_CMAKE_ESP8266"
fi

# 3. Patch Makefile: Prevent deletion of partitions.csv
# The Makefile prebuild_ESPIDF rule deletes partitions.csv, which fails builds where OBK_VARIANT=0.
echo "Patching Makefile to preserve partitions.csv..."
sed -i 's|-rm platforms/ESP-IDF/partitions.csv|# -rm platforms/ESP-IDF/partitions.csv|g' "$BUILD_DIR/Makefile"

# 4. Patch CMakeLists.txt for IDF v5 Compatibility
# IDF v5 splits 'driver' component. We need explicit dependency on 'esp_adc'.
TARGET_CMAKE="$BUILD_DIR/platforms/ESP-IDF/main/CMakeLists.txt"
if [ -f "$TARGET_CMAKE" ]; then
    echo "Patching CMakeLists.txt for IDF v5 dependencies..."
    # Append esp_adc to component list
    sed -i 's|PRIV_REQUIRES mqtt lwip esp_wifi nvs_flash bt esp_pm app_update driver spi_flash)|PRIV_REQUIRES mqtt lwip esp_wifi nvs_flash bt esp_pm app_update driver spi_flash esp_adc esp_timer)|g' "$TARGET_CMAKE"
    
    # Also verify if WHOLE_ARCHIVE works or needs removal. V5 usually supports it.
    # Leaving it alone for now as previous log didn't complain about it.
fi

# 5. Patch source for Minor API Differences (v5.0 vs v5.1+)
# Fix ADC Attenuation Enum: ADC_ATTEN_DB_12 -> ADC_ATTEN_DB_11 (C3 specific mismatch)
ADC_FILE="$BUILD_DIR/src/hal/espidf/hal_adc_espidf.c"
if [ -f "$ADC_FILE" ]; then
    echo "Patching ADC attenuation enum..."
    # Use more robust regex (handle potential whitespace or context)
    sed -i 's/ADC_ATTEN_DB_12/ADC_ATTEN_DB_11/g' "$ADC_FILE"
fi

# Fix Reset Reason Enums (Missing in v5 C3 headers)
# The compiler errors if we define them as integers because they are not in the enum.
# The safest fix is to remove the case statements for these specific reasons.
HTTP_FNS="$BUILD_DIR/src/httpserver/http_fns.c"
if [ -f "$HTTP_FNS" ]; then
    echo "Patching http_fns.c to remove storage-dependent reset reasons..."
    # Delete lines containing the problematic cases (Use grep-like deletion)
    sed -i '/ESP_RST_USB/d' "$HTTP_FNS"
    sed -i '/ESP_RST_JTAG/d' "$HTTP_FNS"
    sed -i '/ESP_RST_EFUSE/d' "$HTTP_FNS"
    sed -i '/ESP_RST_PWR_GLITCH/d' "$HTTP_FNS"
    sed -i '/ESP_RST_CPU_LOCKUP/d' "$HTTP_FNS"
fi

# 6. Fix Include Directories (Berry Config & Others)
# CMakeLists.txt misses include/ and libraries/berry/default/
TARGET_CMAKE="$BUILD_DIR/platforms/ESP-IDF/main/CMakeLists.txt"
if [ -f "$TARGET_CMAKE" ]; then
    echo "Patching CMakeLists.txt to add include directories..."
    # Append PRIV_INCLUDE_DIRS to idf_component_register
    # It currently ends with "esp_timer)" or "spi_flash)" depending on patches.
    # We target the closing parenthesis of the idf_component_register call.
    # We replace ')' with ' PRIV_INCLUDE_DIRS ... )'
    # BUT we need to be careful not to match other closing parentheses.
    # The idf_component_register call is at the end of the file.
    
    sed -i 's|esp_timer)|esp_timer PRIV_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/../../../include ${CMAKE_CURRENT_LIST_DIR}/../../../libraries/berry/default ${CMAKE_CURRENT_LIST_DIR}/../../../libraries/berry/src ${CMAKE_CURRENT_LIST_DIR}/../../../libraries/berry/generate)|g' "$TARGET_CMAKE"
fi

# 7. Run Berry Prebuild (Generate Headers)
# The Berry library requires a pre-build step to generate be_const_strtab.h and others.
echo "Running Berry prebuild..."
if [ -d "$BUILD_DIR/libraries/berry" ]; then
    # We need to run this in the berry directory
    cd "$BUILD_DIR/libraries/berry"
    # The Makefile uses python3, ensuring it's available
    make prebuild || echo "Warning: Berry prebuild failed, but continuing..."
    cd "$BUILD_DIR"
else
    echo "Warning: libraries/berry not found!"
fi

# 8. Force recreate SDK app symlinks for Docker environment
# The Docker volume may cache stale symlinks from previous builds that point
# to incorrect paths. The Make rules only create symlinks if missing, so
# broken symlinks cause build failures. We force recreation here.
echo "Recreating SDK app symlinks for Docker..."

# BK7231T SDK symlink
if [ -d "$BUILD_DIR/sdk/OpenBK7231T/apps" ]; then
    rm -f "$BUILD_DIR/sdk/OpenBK7231T/apps/OpenBK7231T"
    ln -s "/app/build/" "$BUILD_DIR/sdk/OpenBK7231T/apps/OpenBK7231T"
    echo "  Created: sdk/OpenBK7231T/apps/OpenBK7231T -> /app/build/"
fi

# BK7231N SDK symlink
if [ -d "$BUILD_DIR/sdk/OpenBK7231N/apps" ]; then
    rm -f "$BUILD_DIR/sdk/OpenBK7231N/apps/OpenBK7231N"
    ln -s "/app/build/" "$BUILD_DIR/sdk/OpenBK7231N/apps/OpenBK7231N"
    echo "  Created: sdk/OpenBK7231N/apps/OpenBK7231N -> /app/build/"
fi

# BK beken_freertos_sdk app link
# On Windows checkouts, git symlinks can become plain text files (e.g. "../../src").
# The BK SDK expects app to be an actual symlink so app/../libraries resolves correctly.
if [ -d "$BUILD_DIR/sdk/beken_freertos_sdk" ]; then
    rm -f "$BUILD_DIR/sdk/beken_freertos_sdk/app"
    ln -s "../../src" "$BUILD_DIR/sdk/beken_freertos_sdk/app"
    echo "  Created: sdk/beken_freertos_sdk/app -> ../../src"
fi

# 9. Clean stale object files from libraries directories
# The BK SDK compiles .o files next to sources, then copies to Debug/obj/.
# Make caching can skip recompilation if .o exists, bypassing the copy.
# This ensures fresh compilation and proper copy on each build.
echo "Cleaning stale object files from libraries..."
find "$BUILD_DIR/libraries" -name "*.o" -delete 2>/dev/null || true

# 9b. Targeted cleanup for RTL8710A/B incremental object drift
# These SDKs compile objects next to sources and copy them into application/Debug/obj.
# In non-clean builds, stale source-side .o can skip recompilation, leaving Debug/obj
# incomplete and causing linker "No such file or directory" failures.
if [ "$PLATFORM" = "OpenRTL8710A" ] || [ "$PLATFORM" = "OpenRTL8710B" ]; then
    echo "Applying targeted object cleanup for $PLATFORM..."
    find "$BUILD_DIR/src" -name "*.o" -delete 2>/dev/null || true
    find "$BUILD_DIR/platforms/RTL8710A" -name "*.o" -delete 2>/dev/null || true
    find "$BUILD_DIR/platforms/RTL8710B" -name "*.o" -delete 2>/dev/null || true
    find "$BUILD_DIR/sdk/OpenRTL8710A_B" -name "*.o" -delete 2>/dev/null || true
    rm -rf "$BUILD_DIR/sdk/OpenRTL8710A_B/project/obk_ameba1/GCC-RELEASE/application/Debug/obj"
    rm -rf "$BUILD_DIR/sdk/OpenRTL8710A_B/project/obk_amebaz/GCC-RELEASE/application/Debug/obj"
fi

# 10. Patch Makefile symlink paths for Docker environment
# The native Makefile uses $(shell pwd) for symlinks, which doesn't work in Docker
# because the build happens in /app/build, not the source directory
echo "Patching Makefile symlink targets for Docker..."
sed -i 's|ln -s "$(shell pwd)/"|ln -s "/app/build/"|g' "$BUILD_DIR/Makefile"

# 11. Patch Realtek menuconfig invocation to bypass CRLF shebang issues
# Some SDK menuconfig.py scripts have CRLF endings and fail via /usr/bin/env.
# Explicit python3 invocation avoids dependency on script shebang format.
echo "Patching Makefile menuconfig.py invocations..."
sed -i 's|\./menuconfig.py|python3 ./menuconfig.py|g' "$BUILD_DIR/Makefile"

# 11b. TXW81X packaging mode (CI-like wine path or Linux fallback)
# OpenTXW81X build invokes Windows-only post-processing tools (BinScript.exe/makecode.exe).
# TXW_PACKAGER_MODE values:
#   auto     -> use wine if available, otherwise Linux fallback wrappers
#   wine     -> force CI-like path via wine wrappers
#   fallback -> force Linux fallback wrappers
if [ "$PLATFORM" = "OpenTXW81X" ]; then
    TXW_PROJECT_DIR="$BUILD_DIR/sdk/OpenTXW81X/project"
    TXW_PACKAGER_MODE="${TXW_PACKAGER_MODE:-auto}"
    case "$TXW_PACKAGER_MODE" in
        auto|wine|fallback) ;;
        *)
            echo "Invalid TXW_PACKAGER_MODE='$TXW_PACKAGER_MODE', defaulting to auto."
            TXW_PACKAGER_MODE="auto"
            ;;
    esac

    if [ -d "$TXW_PROJECT_DIR" ]; then
        cd "$TXW_PROJECT_DIR"

        if [ -f "BinScript.exe" ] && [ ! -f "BinScript.exe.win" ]; then
            mv -f "BinScript.exe" "BinScript.exe.win"
        fi
        if [ -f "makecode.exe" ] && [ ! -f "makecode.exe.win" ]; then
            mv -f "makecode.exe" "makecode.exe.win"
        fi

        TXW_HAS_WINE=0
        if command -v wine >/dev/null 2>&1; then
            TXW_HAS_WINE=1
        fi

        TXW_HAS_DISPLAY=0
        if [ -n "${DISPLAY:-}" ] || [ -n "${WAYLAND_DISPLAY:-}" ]; then
            TXW_HAS_DISPLAY=1
        fi

        TXW_EFFECTIVE_MODE="$TXW_PACKAGER_MODE"
        if [ "$TXW_EFFECTIVE_MODE" = "auto" ]; then
            if [ "$TXW_HAS_WINE" -eq 1 ] && [ "$TXW_HAS_DISPLAY" -eq 1 ]; then
                TXW_EFFECTIVE_MODE="wine"
            else
                TXW_EFFECTIVE_MODE="fallback"
                if [ "$TXW_HAS_WINE" -eq 1 ] && [ "$TXW_HAS_DISPLAY" -ne 1 ]; then
                    echo "TXW auto mode: wine detected but no display; using fallback wrappers."
                fi
            fi
        fi

        if [ "$TXW_EFFECTIVE_MODE" = "wine" ]; then
            if [ "$TXW_HAS_WINE" -ne 1 ]; then
                echo "ERROR: TXW_PACKAGER_MODE=wine but wine is not installed in the image."
                exit 2
            fi
            if [ ! -f "BinScript.exe.win" ] || [ ! -f "makecode.exe.win" ]; then
                echo "ERROR: TXW Windows packager binaries are missing in $TXW_PROJECT_DIR"
                exit 2
            fi

            echo "Configuring OpenTXW81X CI-like packaging wrappers (wine)..."

            cat > "BinScript.exe" <<'EOF'
#!/bin/sh
set -eu
wine "./BinScript.exe.win" "$@" || rc=$?
rc="${rc:-0}"
if [ "$rc" -eq 0 ]; then
    exit 0
fi
if [ -f "txw81x_fpv.bin" ]; then
    echo "BinScript.exe.win returned $rc but txw81x_fpv.bin exists; continuing." >&2
    exit 0
fi
exit "$rc"
EOF
            cat > "makecode.exe" <<'EOF'
#!/bin/sh
set -eu
wine "./makecode.exe.win" "$@" || rc=$?
rc="${rc:-0}"
if [ "$rc" -eq 0 ]; then
    exit 0
fi
if [ -f "APP.bin" ] && [ -f "APP_compress.bin" ]; then
    echo "makecode.exe.win returned $rc but APP artifacts exist; continuing." >&2
    exit 0
fi
exit "$rc"
EOF
        else
            echo "Applying Linux fallback wrappers for OpenTXW81X packaging tools..."

            cat > "BinScript.exe" <<'EOF'
#!/bin/sh
set -eu
# Linux fallback for Windows BinScript.exe
if [ -f "txw81x_fpv.bin" ]; then
    exit 0
fi
if [ -f "project.bin" ]; then
    cp -f "project.bin" "txw81x_fpv.bin"
fi
if [ -f "parameter.cfg" ] && [ ! -f "param.bin" ]; then
    cp -f "parameter.cfg" "param.bin"
fi
exit 0
EOF
            cat > "makecode.exe" <<'EOF'
#!/bin/sh
set -eu
# Linux fallback for Windows makecode.exe
src=""
for candidate in project.bin txw81x_fpv.bin program.bin; do
    if [ -f "$candidate" ]; then
        src="$candidate"
        break
    fi
done
if [ -z "$src" ]; then
    echo "TXW fallback packaging failed: no source binary found." >&2
    exit 1
fi
cp -f "$src" "APP.bin"
cp -f "$src" "APP_compress.bin"
cp -f "$src" "program.bin" 2>/dev/null || true
exit 0
EOF
        fi

        chmod +x "BinScript.exe" "makecode.exe"
        echo "OpenTXW81X packager mode: $TXW_EFFECTIVE_MODE (requested: $TXW_PACKAGER_MODE)"
        cd "$BUILD_DIR"
    fi
fi

echo "Patches applied."


cd "$BUILD_DIR"

echo "Starting build for platform: $PLATFORM"
# Run the specific build command for the platform
# Avoiding global make clean to allow incremental builds.
# However, if we switch platforms, we might need some cleanup?
# SDKs are usually separate, but 'OpenBK7231T_App' object files might conflict?
# For now, trusting the request to avoid make clean.

if [ -z "$APP_VERSION" ]; then
    APP_VERSION="dev_$(date +%Y%m%d_%H%M%S)"
fi

# Set Default OBK_VARIANT if not set (Defines partition table selection in Makefile)
# 0/Empty = No partition table copied -> Build Failure
# 1 = 2MB (Default)
if [ -z "$OBK_VARIANT" ] || [ "$OBK_VARIANT" = "0" ]; then
    echo "OBK_VARIANT not set or 0. Defaulting to 1 (2MB Partition Table)."
    export OBK_VARIANT=1
fi

if [ "$CLEAN_BUILD" = "1" ]; then
    echo "Clean build requested. Running make clean..."
    make clean APP_NAME="$PLATFORM" "$PLATFORM" || true
    # FORCE CLEANUP: Nuke CMake build directories to remove poisoned caches
    echo "Force removing CMake build directories..."
    rm -rf "$BUILD_DIR/platforms/ESP-IDF/build-"*
    
    # FORCE CLEANUP: Nuke object files in libraries/ and src/
    # If symlinks were missing during 'make clean', these could be left behind,
    # causing 'make' to skip rebuilding them, but failing to copy them to Debug/obj.
    echo "Force removing object files (.o) in libraries/ and src/..."
    find "$BUILD_DIR/libraries" -name "*.o" -delete || true
    find "$BUILD_DIR/src" -name "*.o" -delete || true
fi

# Manual Patch: Ensure correct partitions.csv is used
if [ "$OBK_VARIANT" = "2" ] || [ "$FLASH_SIZE" = "4MB" ]; then
    echo "Copying partitions-4mb.csv (OBK_VARIANT=$OBK_VARIANT, FLASH_SIZE=$FLASH_SIZE)..."
    cp -f "$BUILD_DIR/platforms/ESP-IDF/partitions-4mb.csv" "$BUILD_DIR/platforms/ESP-IDF/partitions.csv" || echo "Warning: Failed to copy partitions-4mb.csv"
else
    echo "Copying partitions-2mb.csv (OBK_VARIANT=$OBK_VARIANT, FLASH_SIZE=$FLASH_SIZE)..."
    cp -f "$BUILD_DIR/platforms/ESP-IDF/partitions-2mb.csv" "$BUILD_DIR/platforms/ESP-IDF/partitions.csv" || echo "Warning: Failed to copy partitions-2mb.csv"
fi

echo "Building version: $APP_VERSION"
# Pass ESP_FSIZE from FLASH_SIZE env var (default to 2MB if not set)
# esptool requires this to be set.
if [ -z "$FLASH_SIZE" ]; then
    export FLASH_SIZE="2MB"
fi
echo "Using Flash Size: $FLASH_SIZE"
make APP_VERSION="$APP_VERSION" APP_NAME="$PLATFORM" ESP_FSIZE="$FLASH_SIZE" "$PLATFORM"
# Copy specific platform output to the mounted output directory
# Output usually goes to ../output or similar relative to SDK?
# The original script says: Find outputs in: ../output/${app_ver}
# In our structure: $BUILD_DIR/output/$APP_VERSION

# We need to find where the build actually put the files.
# Standard OpenBeken makefile usually puts them in <RepoRoot>/output/<Version>/
if [ -d "$BUILD_DIR/output/$APP_VERSION" ]; then
    cp -r "$BUILD_DIR/output/$APP_VERSION/"* "/app/output/"
    echo "Build artifacts copied to output folder."
else
    echo "ERROR: Output directory not found at $BUILD_DIR/output/$APP_VERSION"
    ls -la "$BUILD_DIR/output"
    exit 1
fi
