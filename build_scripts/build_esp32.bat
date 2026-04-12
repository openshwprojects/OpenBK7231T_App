@echo off
setlocal EnableDelayedExpansion
pushd "%~dp0\.."

:: Default settings
set APP_VERSION=dev_esp32
set OBK_VARIANT=2
set ESP_FSIZE=4MB
set TARGET=esp32
set ACTION=build

:: Check for target argument
if not "%~1"=="" set TARGET=%~1
if not "%~2"=="" set ACTION=%~2

if "%TARGET%"=="esp32" (
    set BUILD_DIR=platforms\ESP-IDF\build-32
    set BOOTLOADER_ADDR=0x1000
) else if "%TARGET%"=="esp32c3" (
    set BUILD_DIR=platforms\ESP-IDF\build-c3
    set BOOTLOADER_ADDR=0x0
) else if "%TARGET%"=="esp32c2" (
    set BUILD_DIR=platforms\ESP-IDF\build-c2
    set BOOTLOADER_ADDR=0x0
) else if "%TARGET%"=="esp32c5" (
    set BUILD_DIR=platforms\ESP-IDF\build-c5
    set BOOTLOADER_ADDR=0x2000
) else if "%TARGET%"=="esp32c6" (
    set BUILD_DIR=platforms\ESP-IDF\build-c6
    set BOOTLOADER_ADDR=0x0
) else if "%TARGET%"=="esp32c61" (
    set BUILD_DIR=platforms\ESP-IDF\build-c61
    set BOOTLOADER_ADDR=0x0
) else if "%TARGET%"=="esp32s2" (
    set BUILD_DIR=platforms\ESP-IDF\build-s2
    set BOOTLOADER_ADDR=0x1000
) else if "%TARGET%"=="esp32s3" (
    set BUILD_DIR=platforms\ESP-IDF\build-s3
    set BOOTLOADER_ADDR=0x0
) else (
    echo [ERROR] Unknown target: %TARGET%
    exit /b 1
)

echo ==============================================
echo Building OpenBeken ESP32 on Windows natively
echo Target: %TARGET%
echo Size: %ESP_FSIZE%
echo Variant (2=tuyaMCU/4MB): %OBK_VARIANT%
echo Bootloader Addr: %BOOTLOADER_ADDR%
echo Build Directory: %BUILD_DIR%
echo Action: %ACTION%
echo ==============================================

:: Check if ESP-IDF environment is active
where idf.py >nul 2>nul
if %errorlevel% equ 0 goto :idf_ready

echo [INFO] ESP-IDF 'idf.py' not found in PATH.
echo [INFO] Attempting to load from sdk\esp-idf...

if not exist "sdk\esp-idf\export.bat" goto :no_idf

:: Try export.bat first
call "sdk\esp-idf\export.bat"
where idf.py >nul 2>nul
if %errorlevel% equ 0 goto :idf_ready

:: export.bat didn't give us idf.py - try install.bat then re-export
echo [INFO] export.bat did not set up environment. Running install.bat...
if not exist "sdk\esp-idf\install.bat" goto :no_idf

call "sdk\esp-idf\install.bat"

echo [INFO] Retrying export.bat...
call "sdk\esp-idf\export.bat"
where idf.py >nul 2>nul
if %errorlevel% equ 0 goto :idf_ready

echo [ERROR] ESP-IDF environment still not available after install+export!
exit /b 1

:no_idf
echo [ERROR] ESP-IDF not found!
echo Please ensure sdk\esp-idf is checked out (git submodule update --init --recursive sdk/esp-idf)
exit /b 1

:idf_ready
echo [INFO] ESP-IDF environment is ready.

:: If just cleaning
if "%ACTION%"=="clean" (
    echo [INFO] Cleaning %TARGET%...
    if exist %BUILD_DIR% cmake --build %BUILD_DIR% --target clean
    echo [INFO] Clean complete!
    exit /b 0
)

:: Prepare partition tables based on OBK_VARIANT
if exist platforms\ESP-IDF\sdkconfig del platforms\ESP-IDF\sdkconfig
if exist platforms\ESP-IDF\partitions.csv del platforms\ESP-IDF\partitions.csv

:: Defaulting to 4MB for variant 2
copy /y platforms\ESP-IDF\partitions-4mb.csv platforms\ESP-IDF\partitions.csv >nul

:: Set environment variables for IDF cmake
set IDF_TARGET=%TARGET%


:: --- Berry prebuild ---
call build_scripts\berry_prebuild.bat

:: Configure CMake via ESP-IDF toolchain
echo [INFO] Running CMake configuration for %TARGET%...
cmake platforms\ESP-IDF -B %BUILD_DIR% -G "Ninja"
if !errorlevel! neq 0 (
    echo [ERROR] CMake configuration failed!
    exit /b 1
)

:: Build using CMake
echo [INFO] Building with CMake/Ninja...
cmake --build %BUILD_DIR% -j
if !errorlevel! neq 0 (
    echo [ERROR] Build failed!
    exit /b 1
)

:: Ensure output directory exists
echo [INFO] Creating output directory...
if not exist "output\%APP_VERSION%" mkdir output\%APP_VERSION%

:: Determine output names
if "%TARGET%"=="esp32" (
    set OUT_NAME=OpenESP32
) else if "%TARGET%"=="esp32c3" (
    set OUT_NAME=OpenESP32C3
) else if "%TARGET%"=="esp32c2" (
    set OUT_NAME=OpenESP32C2
) else if "%TARGET%"=="esp32c5" (
    set OUT_NAME=OpenESP32C5
) else if "%TARGET%"=="esp32c6" (
    set OUT_NAME=OpenESP32C6
) else if "%TARGET%"=="esp32c61" (
    set OUT_NAME=OpenESP32C61
) else if "%TARGET%"=="esp32s2" (
    set OUT_NAME=OpenESP32S2
) else if "%TARGET%"=="esp32s3" (
    set OUT_NAME=OpenESP32S3
) else (
    set OUT_NAME=Open%TARGET%
)

:: Package binaries using esptool
echo [INFO] Merging binaries with esptool...
set FACTORY_BIN=output\%APP_VERSION%\!OUT_NAME!_%APP_VERSION%.factory.bin
set IMG_BIN=output\%APP_VERSION%\!OUT_NAME!_%APP_VERSION%.img

python -m esptool -c %TARGET% merge_bin -o !FACTORY_BIN! --flash_mode dio --flash_size %ESP_FSIZE% %BOOTLOADER_ADDR% %BUILD_DIR%\bootloader\bootloader.bin 0x8000 %BUILD_DIR%\partition_table\partition-table.bin 0x10000 %BUILD_DIR%\OpenBeken.bin
if !errorlevel! neq 0 (
    echo [ERROR] esptool merge failed!
    exit /b 1
)

copy /y %BUILD_DIR%\OpenBeken.bin !IMG_BIN! >nul

echo ==============================================
echo BUILD SUCCESSFUL!
echo Output files located in output\%APP_VERSION%
echo Factory Bin: !FACTORY_BIN!
echo OTA Bin:     !IMG_BIN!
echo ==============================================
exit /b 0

