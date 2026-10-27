@echo off
setlocal EnableDelayedExpansion

set APP_VERSION=dev_rtl8710b
set APP_NAME=OpenBK7231T_App
set ACTION=all

if not "%~1"=="" set APP_VERSION=%~1
if not "%~2"=="" set OBK_VARIANT=%~2
if not "%~3"=="" set ACTION=%~3

echo ==============================================
echo Building OpenBeken RTL8710B on Windows natively
echo App Version: %APP_VERSION%
echo Action: %ACTION%
echo ==============================================

:: 1. Check for GNU make (requires MSYS2 or similar for Windows natively)
set "MSYS_MAKE="
for %%P in (
    "C:\msys64\usr\bin\make.exe"
    "C:\ProgramData\chocolatey\bin\make.exe"
    "W:\TOOLS\msys64\usr\bin\make.exe"
) do (
    if exist "%%~P" (
        set "MSYS_MAKE=%%~dpP"
        goto :found_make
    )
)
echo [ERROR] GNU make not found. Please install GNU make (e.g. MSYS2 or choco).
exit /b 1

:found_make
echo [INFO] GNU make found at %MSYS_MAKE%

:: 2. Check for Git Bash bins (to provide cp, rm, bash for make natively)
set "GIT_USR_BIN="
if exist "C:\Program Files\Git\usr\bin\sh.exe" set "GIT_USR_BIN=C:\Program Files\Git\usr\bin"
if exist "C:\Program Files\Git\bin\sh.exe" set "GIT_USR_BIN=C:\Program Files\Git\bin"
if not "%GIT_USR_BIN%"=="" (
    echo [INFO] Unix Utilities found at: %GIT_USR_BIN%
)

:: 3. Check for toolchain
set "TOOLCHAIN_ROOT=%CD%\sdk\OpenBK7231N\platforms\BK7231N\toolchain\windows\gcc-arm-none-eabi-4_9-2015q1"
if not exist "%TOOLCHAIN_ROOT%\bin\arm-none-eabi-gcc.exe" (
    echo [ERROR] GCC 4.9 toolchain not found at %TOOLCHAIN_ROOT%
    exit /b 1
)
echo [INFO] Toolchain set to: %TOOLCHAIN_ROOT%

:: 3. Submodule Sync Check (OpenRTL8710A_B and libraries/berry)
if not exist "sdk\OpenRTL8710A_B\project\obk_amebaz\GCC-RELEASE\Makefile" (
    echo [INFO] OpenRTL8710A_B submodule missing or empty. Initializing...
    git submodule update --init sdk/OpenRTL8710A_B
) else (
    echo [INFO] RTL8710B submodule OK.
)

if not exist "libraries\berry\src\be_api.c" (
    echo [INFO] Berry submodule missing or empty. Initializing...
    git submodule update --init libraries/berry
) else (
    echo [INFO] Berry submodule OK.
)
echo [INFO] Resolving Berry paths...
call build_scripts\berry_prebuild.bat
if !errorlevel! neq 0 (
    echo [ERROR] Berry prebuild failed.
    exit /b 1
)

:: 5. Compile RTL8710B
echo [INFO] Building RTL8710B using OS=CYGWIN to enable Native Windows .exe parsing...
echo [INFO] Entering compilation phase...

:: Inject Make, SysUtils, and Toolchain into PATH just before make (to avoid MSYS python conflicts)
set "PATH=%MSYS_MAKE%;%GIT_USR_BIN%;%TOOLCHAIN_ROOT%\bin;%PATH%"

pushd sdk\OpenRTL8710A_B\project\obk_amebaz\GCC-RELEASE
make OS=CYGWIN %ACTION%
set BUILD_RESULT=!errorlevel!
popd

if %BUILD_RESULT% neq 0 (
    echo [ERROR] Build failed with code %BUILD_RESULT%
    exit /b %BUILD_RESULT%
)

:: 6. Copy Binaries
echo [INFO] Copying binaries...
if not exist "output\%APP_VERSION%" mkdir "output\%APP_VERSION%"

set "BIN_DIR=sdk\OpenRTL8710A_B\project\obk_amebaz\GCC-RELEASE\application\Debug\bin"

if exist "%BIN_DIR%\image2_all_ota1.bin" (
    copy /y "%BIN_DIR%\image2_all_ota1.bin" "output\%APP_VERSION%\%APP_VERSION%_RTL8710B_OTA.bin" >nul
) else (
    echo [WARNING] OTA image not found at %BIN_DIR%\image2_all_ota1.bin
)

echo ==============================================
echo BUILD SUCCESSFUL
echo ==============================================
exit /b 0
