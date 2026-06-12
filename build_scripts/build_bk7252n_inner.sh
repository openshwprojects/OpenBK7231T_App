#!/bin/bash
# Inner build script for BK7252N
# Arguments: APP_NAME APP_VERSION SDK_DIR OBK_VARIANT ACTION
set -e

APP_NAME="$1"
APP_VERSION="$2"
SDK_DIR="$3"
export OBK_VARIANT="$4"
ACTION="$5"

echo "[INFO] Inner build script started for BK7252N"

SCRIPT_DIR="$(pwd)"

# Export the borrowed ARM GCC toolchain to the environment for Beken FreeRTOS SDK Makefile to pick up
TOOLCHAIN_RAW="$(pwd)/sdk/OpenBK7231N/platforms/BK7231N/toolchain/windows/gcc-arm-none-eabi-4_9-2015q1/bin/"
export ARM_GCC_TOOLCHAIN="$(cygpath -u "$TOOLCHAIN_RAW")/"
export PATH="$ARM_GCC_TOOLCHAIN:$PATH"
echo "[INFO] Toolchain: $ARM_GCC_TOOLCHAIN"

# Set a localized TEMP dir to avoid PyInstaller extraction permission errors (AntiVirus/Defender locking)
# beken_packager.exe is a PyInstaller bundle that needs to extract VCRUNTIME140.dll
# Use native Windows Temp directory where DLL drops are historically trusted by Defender
# We must double-escape backslashes so MSYS Make doesn't swallow them when passing to subshells!
LOCAL_TMP="$(cygpath -w "$USERPROFILE/AppData/Local/Temp/obk_p_$$" | sed 's/\\/\\\\/g')"
echo "[INFO] Setting local TEMP dir: $LOCAL_TMP"
mkdir -p "$USERPROFILE/AppData/Local/Temp/obk_p_$$"
export TEMP="$LOCAL_TMP"
export TMP="$LOCAL_TMP"
export TMPDIR="$LOCAL_TMP"

# Navigate to the build system directory
cd "$SDK_DIR"

# First, check if make is available
if type make > /dev/null 2>&1; then
    echo "[INFO] GNU make is available."
else
    echo "[INFO] GNU make not found in Git Bash. Attempting to get it..."
    MAKE_FOUND=0
    for MAKE_PATH in /c/ProgramData/chocolatey/bin/make.exe /w/TOOLS/msys64/usr/bin/make.exe /c/msys64/usr/bin/make.exe; do
        if [ -f "$MAKE_PATH" ]; then
            export PATH="$(dirname "$MAKE_PATH"):$PATH"
            MAKE_FOUND=1
            echo "[INFO] Found make at $MAKE_PATH"
            break
        fi
    done
    if [ $MAKE_FOUND -eq 0 ]; then
        echo "[ERROR] GNU make not found!"
        exit 1
    fi
fi

if [ "$ACTION" = "clean" ]; then
    echo "[INFO] Running BK7252N clean from $(pwd)..."
    make clean -C ./
    exit 0
fi

echo "[INFO] Running BK7252N build from $(pwd)..."

# Strip GCC 4.9 unsupported Wno-error flags from application.mk
sed -i 's/-Wno-error=implicit-function-declaration -Wno-error=incompatible-pointer-types -Wno-error=return-mismatch -Wno-error=int-conversion -Wno-error=changes-meaning//g' application.mk
# Force encrypt.exe on Windows MSYS (which identifies as MINGW64, unlike MINGW32 tracked in application.mk)
sed -i 's/"\.\/tools\/crc_binary\/encrypt_n"/"\.\/tools\/crc_binary\/encrypt\.exe"/g' application.mk

# Replace encrypt.exe with the compatible one from OpenBK7231T SDK that supports multi-key arguments
cp "$(cygpath -u "$SCRIPT_DIR/sdk/OpenBK7231T/platforms/bk7231t/bk7231t_os/tools/generate/package_tool/windows/encrypt.exe")" ./tools/crc_binary/encrypt.exe

make TEMP="$LOCAL_TMP" TMP="$LOCAL_TMP" TMPDIR="$LOCAL_TMP" bk7252n -j8 USER_SW_VER="$APP_VERSION" OBK_DIR="../../src"
./tools/rtt_ota/rt_ota_packaging_tool_cli.exe -f ./out/bsp.bin -o ./out/app.rbl -p app -c gzip -s aes -k 0123456789ABCDEF0123456789ABCDEF -i 0123456789ABCDEF -v "$APP_VERSION"

echo "[INFO] Build complete!"
