#!/bin/bash
set -e

# Arguments passed from Docker command
PLATFORM=$1

# Source (Host Mount) and Build (Volume) directories
SOURCE_DIR="/app/source"
BUILD_DIR="/app/build"

echo "Checking build environment..."

if [ -n "$IDF_PATH" ] && [ -f "$IDF_PATH/export.sh" ]; then
    echo "Sourcing ESP-IDF environment..."
    . "$IDF_PATH/export.sh"
    # Suppress IDF checks during export to avoid noise
fi

# Ensure Python packages are installed for BL602 SDK
echo "Ensuring Python packages for BL602 SDK..."
pip3 install --quiet fdt toml configobj pycryptodomex 2>/dev/null || true


# Smart Sync Strategy
if [ ! -d "$BUILD_DIR/sdk" ]; then
    echo "Initial setup: Full sync of source to build volume..."
    # First run: Sync everything including SDKs
    rsync -rltD --no-owner --no-group --info=progress2 "$SOURCE_DIR/" "$BUILD_DIR/" || true
else
    echo "Incremental build: Syncing application code (excluding SDKs)..."
    # Subsequent runs: Sync everything EXCEPT sdk/
    rsync -rltD --no-owner --no-group --info=progress2 --exclude 'sdk/' --exclude '.git/' "$SOURCE_DIR/" "$BUILD_DIR/" || true
fi

# Apply custom configuration
if [ -f "/app/custom_config.h" ]; then
    echo "Applying custom configuration..."
    cp "/app/custom_config.h" "$BUILD_DIR/src/obk_config.h"
fi

# Fix Windows line endings for critical scripts
echo "Fixing line endings for scripts..."
if [ -f "$BUILD_DIR/libraries/berry/tools/coc/coc" ]; then
    sed -i 's/\r$//' "$BUILD_DIR/libraries/berry/tools/coc/coc"
    chmod +x "$BUILD_DIR/libraries/berry/tools/coc/coc"
fi
find "$BUILD_DIR" -name "*.sh" -exec sed -i 's/\r$//' {} + || true

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
    sed -i 's|PRIV_REQUIRES .*|PRIV_REQUIRES mqtt lwip esp_wifi nvs_flash esp_pm app_update driver spi_flash)|g' "$TARGET_CMAKE"
    
    # 4. Remove WHOLE_ARCHIVE (it seems to be misinterpreted as a source file)
    sed -i 's|WHOLE_ARCHIVE||g' "$TARGET_CMAKE"
    sed -i '/idf_component_register/i message(STATUS "DEBUG: CMAKE_CURRENT_LIST_DIR=${CMAKE_CURRENT_LIST_DIR}")' "$TARGET_CMAKE"

    echo "Patched CMakeLists.txt:"
    cat "$TARGET_CMAKE"
else
    echo "Warning: $TARGET_CMAKE not found, skipping patch."
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
    sed -i 's|PRIV_REQUIRES mqtt lwip esp_wifi nvs_flash esp_pm app_update driver spi_flash)|PRIV_REQUIRES mqtt lwip esp_wifi nvs_flash esp_pm app_update driver spi_flash esp_adc esp_timer)|g' "$TARGET_CMAKE"
    
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

# Manual Patch: Ensure partitions.csv exists (Makefile logic fails for OBK_VARIANT=0)
if [ ! -f "$BUILD_DIR/platforms/ESP-IDF/partitions.csv" ]; then
    echo "partitions.csv missing, copying default partitions-2mb.csv..."
    cp "$BUILD_DIR/platforms/ESP-IDF/partitions-2mb.csv" "$BUILD_DIR/platforms/ESP-IDF/partitions.csv" || echo "Warning: Failed to copy partitions.csv"
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
