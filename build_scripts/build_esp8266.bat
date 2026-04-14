@echo off
setlocal EnableDelayedExpansion
pushd "%~dp0\.."

:: Default settings
set APP_VERSION=dev_esp8266
set OBK_VARIANT=1
set ESP_FSIZE=2MB
set ACTION=build
set BUILD_DIR=platforms\ESP8266\build
set BOOTLOADER_ADDR=0x0

:: Check for action argument
if not "%~1"=="" set ACTION=%~1

echo ==============================================
echo Building OpenBeken ESP8266 on Windows natively
echo Size: %ESP_FSIZE%
echo Variant: %OBK_VARIANT%
echo Bootloader Addr: %BOOTLOADER_ADDR%
echo Build Directory: %BUILD_DIR%
echo Action: %ACTION%
echo ==============================================

:: The ESP8266 RTOS SDK uses its own idf.py from sdk\OpenESP8266
:: We need to set IDF_PATH to this SDK, not the ESP-IDF one
set IDF_PATH=%CD%\sdk\OpenESP8266

if not exist "%IDF_PATH%\tools\idf.py" (
    echo [ERROR] ESP8266 RTOS SDK not found at %IDF_PATH%
    echo Please ensure sdk\OpenESP8266 is checked out:
    echo   git submodule update --init --recursive sdk/OpenESP8266
    exit /b 1
)

:: Check that Berry submodule is checked out
if not exist "libraries\berry\src\be_api.c" (
    echo [INFO] Berry submodule not found, checking out...
    git submodule update --init --depth=1 libraries\berry
    if !errorlevel! neq 0 (
        echo [ERROR] Failed to checkout Berry submodule!
        exit /b 1
    )
)
echo [INFO] Berry submodule OK.

:: Set environment variables for the build
set APP_VERSION=%APP_VERSION%
set OBK_VARIANT=%OBK_VARIANT%

:: Try to set up the ESP8266 toolchain environment
:: First check if xtensa-lx106-elf-gcc is already available
where xtensa-lx106-elf-gcc >nul 2>nul
if %errorlevel% equ 0 goto :toolchain_ready

echo [INFO] xtensa-lx106-elf toolchain not found in PATH.
echo [INFO] Attempting to install/export ESP8266 tools...

:: Run idf_tools.py to install and export
python "%IDF_PATH%\tools\idf_tools.py" install
if !errorlevel! neq 0 (
    echo [ERROR] Failed to install ESP8266 tools!
    exit /b 1
)

:: Get the export paths from idf_tools.py
for /f "usebackq delims=" %%i in (`python "%IDF_PATH%\tools\idf_tools.py" export --format key-value 2^>nul`) do (
    set "%%i"
)

:: Also try to add the tool paths via the standard mechanism
for /f "usebackq delims=" %%i in (`python "%IDF_PATH%\tools\idf_tools.py" export 2^>nul`) do (
    set "TOOL_EXPORTS=%%i"
)
if defined TOOL_EXPORTS (
    :: The export command outputs something like: export PATH="..."
    :: Parse and apply it
    for /f "tokens=2 delims==" %%p in ("!TOOL_EXPORTS!") do (
        set "EXTRACTED_PATH=%%~p"
    )
    if defined EXTRACTED_PATH (
        set "PATH=!EXTRACTED_PATH!;!PATH!"
    )
)

:: Check again
where xtensa-lx106-elf-gcc >nul 2>nul
if %errorlevel% equ 0 goto :toolchain_ready

:: Last resort: try to find it in the espressif tools directory
set ESPRESSIF_TOOLS=%USERPROFILE%\.espressif\tools
if exist "%ESPRESSIF_TOOLS%\xtensa-lx106-elf" (
    for /d %%d in ("%ESPRESSIF_TOOLS%\xtensa-lx106-elf\*") do (
        if exist "%%d\xtensa-lx106-elf\bin\xtensa-lx106-elf-gcc.exe" (
            set "PATH=%%d\xtensa-lx106-elf\bin;!PATH!"
            echo [INFO] Found toolchain at %%d\xtensa-lx106-elf\bin
            goto :toolchain_ready
        )
    )
)

echo [WARNING] xtensa-lx106-elf toolchain may not be available.
echo [WARNING] Build may fail. Install with: python sdk\OpenESP8266\tools\idf_tools.py install

:toolchain_ready
echo [INFO] Toolchain environment ready.

:: Activate the ESP8266 RTOS SDK Python virtual environment
:: The SDK's check_python_dependencies.py requires packages installed in the venv
set ESPRESSIF_VENV=
for /d %%v in ("%USERPROFILE%\.espressif\python_env\rtos3.4_py*") do (
    if exist "%%v\Scripts\activate.bat" set "ESPRESSIF_VENV=%%v"
)
if defined ESPRESSIF_VENV (
    echo [INFO] Activating Python venv: !ESPRESSIF_VENV!
    call "!ESPRESSIF_VENV!\Scripts\activate.bat"
) else (
    echo [INFO] No ESP8266 Python venv found, installing...
    python "%IDF_PATH%\tools\idf_tools.py" install-python-env
    if !errorlevel! neq 0 (
        echo [ERROR] Failed to install ESP8266 Python environment!
        echo [INFO] You can try manually: python sdk\OpenESP8266\tools\idf_tools.py install-python-env
        exit /b 1
    )
    :: Re-scan for the newly created venv
    for /d %%v in ("%USERPROFILE%\.espressif\python_env\rtos3.4_py*") do (
        if exist "%%v\Scripts\activate.bat" set "ESPRESSIF_VENV=%%v"
    )
    if defined ESPRESSIF_VENV (
        echo [INFO] Activating Python venv: !ESPRESSIF_VENV!
        call "!ESPRESSIF_VENV!\Scripts\activate.bat"
    ) else (
        echo [WARNING] Python venv was installed but could not be found. Using system Python.
    )
)

:: If just cleaning
if "%ACTION%"=="clean" (
    echo [INFO] Cleaning ESP8266 build...
    if exist %BUILD_DIR% cmake --build %BUILD_DIR% --target clean
    echo [INFO] Clean complete!
    exit /b 0
)

:: Set up partition table (2MB default)
if exist platforms\ESP8266\partitions.csv del platforms\ESP8266\partitions.csv
copy /y platforms\ESP8266\partitions-2mb.csv platforms\ESP8266\partitions.csv >nul

:: Remove stale sdkconfig to allow defaults to regenerate
if exist platforms\ESP8266\sdkconfig del platforms\ESP8266\sdkconfig


:: --- Berry prebuild ---
call build_scripts\berry_prebuild.bat

:: Configure CMake via the ESP8266 RTOS SDK
echo [INFO] Running CMake configuration for ESP8266...
cmake platforms\ESP8266 -B %BUILD_DIR% -G "Ninja"
if !errorlevel! neq 0 (
    echo [ERROR] CMake configuration failed!
    exit /b 1
)

:: Build using CMake/Ninja
echo [INFO] Building with CMake/Ninja...
cmake --build %BUILD_DIR% -j
if !errorlevel! neq 0 (
    echo [ERROR] Build failed!
    exit /b 1
)

:: Ensure output directory exists
echo [INFO] Creating output directory...
if not exist "output\%APP_VERSION%" mkdir output\%APP_VERSION%

set OUT_NAME=OpenESP8266

:: Package binaries using esptool
echo [INFO] Merging binaries with esptool...
set FACTORY_BIN=output\%APP_VERSION%\!OUT_NAME!_%APP_VERSION%.factory.bin
set IMG_BIN=output\%APP_VERSION%\!OUT_NAME!_%APP_VERSION%.img

python -m esptool --chip esp8266 merge_bin -o !FACTORY_BIN! --flash_mode dout --flash_size %ESP_FSIZE% %BOOTLOADER_ADDR% %BUILD_DIR%\bootloader\bootloader.bin 0x8000 %BUILD_DIR%\partition_table\partition-table.bin 0x10000 %BUILD_DIR%\OpenBeken.bin
if !errorlevel! neq 0 (
    :: Try with esptool.py directly from SDK
    echo [INFO] Retrying with SDK esptool...
    python "%IDF_PATH%\components\esptool_py\esptool\esptool.py" --chip esp8266 merge_bin -o !FACTORY_BIN! --flash_mode dout --flash_size %ESP_FSIZE% %BOOTLOADER_ADDR% %BUILD_DIR%\bootloader\bootloader.bin 0x8000 %BUILD_DIR%\partition_table\partition-table.bin 0x10000 %BUILD_DIR%\OpenBeken.bin
    if !errorlevel! neq 0 (
        echo [ERROR] esptool merge failed!
        exit /b 1
    )
)

copy /y %BUILD_DIR%\OpenBeken.bin !IMG_BIN! >nul

echo ==============================================
echo BUILD SUCCESSFUL!
echo Output files located in output\%APP_VERSION%
echo Factory Bin: !FACTORY_BIN!
echo OTA Bin:     !IMG_BIN!
echo ==============================================
exit /b 0

