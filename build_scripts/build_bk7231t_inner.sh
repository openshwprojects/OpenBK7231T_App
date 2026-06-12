#!/bin/bash
# Inner build script called by build_bk7231t.bat
# Arguments: APP_NAME APP_VERSION SDK_DIR OBK_VARIANT
set -e

APP_NAME="$1"
APP_VERSION="$2"
SDK_DIR="$3"
export OBK_VARIANT="$4"

echo "[INFO] Inner build script started"
echo "[INFO] APP_NAME=$APP_NAME"
echo "[INFO] APP_VERSION=$APP_VERSION"
echo "[INFO] SDK_DIR=$SDK_DIR"
echo "[INFO] OBK_VARIANT=$OBK_VARIANT"

# First, check if make is available
if type make > /dev/null 2>&1; then
    echo "[INFO] GNU make is available."
else
    echo "[INFO] GNU make not found in Git Bash. Attempting to get it..."
    MAKE_FOUND=0
    for MAKE_PATH in /c/ProgramData/chocolatey/bin/make.exe /c/msys64/usr/bin/make.exe /w/TOOLS/msys64/usr/bin/make.exe; do
        if [ -f "$MAKE_PATH" ]; then
            export PATH="$(dirname "$MAKE_PATH"):$PATH"
            MAKE_FOUND=1
            echo "[INFO] Found make at $MAKE_PATH"
            break
        fi
    done
    if [ $MAKE_FOUND -eq 0 ]; then
        echo "[ERROR] GNU make not found!"
        echo "Please install make via one of:"
        echo "  choco install make"
        echo "  pacman -S make   (in MSYS2)"
        echo "  Or download from https://gnuwin32.sourceforge.net/packages/make.htm"
        exit 1
    fi
fi

echo "[INFO] make found: $(which make)"
echo "[INFO] make version: $(make --version | head -1)"

SCRIPT_DIR="$(pwd)"

# Set a localized TEMP dir to avoid PyInstaller extraction permission errors
# beken_packager.exe is a PyInstaller bundle that needs to extract VCRUNTIME140.dll
LOCAL_TMP="$(cygpath -m "$SCRIPT_DIR/output/tmp")"
echo "[INFO] Setting local TEMP dir: $LOCAL_TMP"
mkdir -p "$SCRIPT_DIR/output/tmp"
export TEMP="$LOCAL_TMP"
export TMP="$LOCAL_TMP"
export TMPDIR="$LOCAL_TMP"
echo "[INFO] TEMP=$TEMP"
echo "[INFO] TMP=$TMP"
echo "[INFO] TMPDIR=$TMPDIR"

ACTION="$5"

# Navigate to the build system directory
cd "$SDK_DIR/platforms/bk7231t/bk7231t_os"

if [ "$ACTION" = "clean" ]; then
    echo "[INFO] Running BK7231T clean from $(pwd)..."
    make APP_BIN_NAME="$APP_NAME" USER_SW_VER="$(echo $APP_VERSION | cut -d'-' -f1)" APP_VERSION="$APP_VERSION" clean -C ./
    echo "[INFO] Clean complete!"
    exit 0
fi

echo "[INFO] Running BK7231T build from $(pwd)..."
bash build.sh "$APP_NAME" "$APP_VERSION" bk7231t "OBK_VARIANT=$OBK_VARIANT TARGET_PLATFORM=bk7231t"

echo "[INFO] Build complete!"
