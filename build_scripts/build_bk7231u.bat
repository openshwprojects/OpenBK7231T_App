@echo off
setlocal EnableDelayedExpansion
pushd "%~dp0\.."

:: ========================================================
:: OpenBeken BK7231U Windows build script
:: Uses Git Bash + bundled ARM GCC toolchain from BK7231N
:: ========================================================

set APP_NAME=OpenBK7231U_App
set APP_VERSION=dev_bk7231u
set SDK_DIR=sdk\beken_freertos_sdk
set OBK_VARIANT=2
set ACTION=build

:: Allow overriding version from command line
if not "%~1"=="" set APP_VERSION=%~1
if not "%~2"=="" set ACTION=%~2

echo ==============================================
echo Building OpenBeken for BK7231U on Windows
echo App Version: %APP_VERSION%
echo Variant:     %OBK_VARIANT%
echo SDK Dir:     %SDK_DIR%
echo Action:      %ACTION%
echo ==============================================

:: --- Check prerequisites ---

:: 1. Git Bash
set GIT_BASH=
for %%P in (
    "C:\Program Files\Git\bin\bash.exe"
    "C:\Program Files (x86)\Git\bin\bash.exe"
) do (
    if exist %%P (
        set "GIT_BASH=%%~P"
        goto :found_bash
    )
)
echo [ERROR] Git Bash not found! Please install Git for Windows.
exit /b 1

:found_bash
echo [INFO] Using Git Bash: %GIT_BASH%

:: 2. Check that SDK submodule is checked out
if not exist "%SDK_DIR%\build.sh" (
    echo [INFO] SDK submodule not found, checking out...
    git submodule update --init --recursive --depth=1 %SDK_DIR%
    if !errorlevel! neq 0 (
        echo [ERROR] Failed to checkout SDK submodule!
        exit /b 1
    )
)
echo [INFO] SDK submodule OK.

:: 3. Check Windows ARM GCC toolchain in OpenBK7231N (borrowed)
if not exist "sdk\OpenBK7231N\platforms\BK7231N\toolchain\windows\gcc-arm-none-eabi-4_9-2015q1\bin\arm-none-eabi-gcc.exe" (
    echo [ERROR] Windows ARM GCC toolchain not found!
    echo Needed from: sdk\OpenBK7231N\platforms\...
    echo Run build_bk7231n.bat once to initialize it, or pull submodules.
    exit /b 1
)
echo [INFO] ARM GCC toolchain OK.

:: 4. Ensure our app 'app' junction exists in SDK folder
if exist "%SDK_DIR%\app\" (
    echo [INFO] App junction exists.
) else (
    echo [INFO] Creating junction for app in SDK folder...
    if exist "%SDK_DIR%\app" del /q /f "%SDK_DIR%\app"
    mklink /J "%SDK_DIR%\app" "%CD%\src"
    if !errorlevel! neq 0 (
        echo [ERROR] Failed to create junction. Try running as Administrator.
        exit /b 1
    )
)

:: 5. Ensure output directory exists
if not exist "output\%APP_VERSION%" mkdir "output\%APP_VERSION%"


:: --- Berry prebuild ---
call build_scripts\berry_prebuild.bat

echo [INFO] Starting %ACTION% via Git Bash...
echo.

"%GIT_BASH%" --login -i "%CD%\build_scripts\build_bk72xx_freertos_inner.sh" "%APP_NAME%" "%APP_VERSION%" "%SDK_DIR%" "%OBK_VARIANT%" "%ACTION%" "bk7231u"
set BUILD_RESULT=%errorlevel%

if %BUILD_RESULT% neq 0 (
    echo.
    echo [ERROR] BK7231U build failed with exit code %BUILD_RESULT%!
    exit /b %BUILD_RESULT%
)

:: --- Copy output ---
echo.
echo [INFO] Checking output files...
set GEN_DIR=%SDK_DIR%\out

set COPIED_COUNT=0
if exist "%GEN_DIR%\app.rbl" (
    copy /y "%GEN_DIR%\app.rbl" "output\%APP_VERSION%\%APP_NAME%_%APP_VERSION%.rbl" >nul 2>nul
    echo [INFO] Copied: %APP_NAME%_%APP_VERSION%.rbl
    set /a COPIED_COUNT+=1
)
if exist "%GEN_DIR%\bk7231u_QIO.bin" (
    copy /y "%GEN_DIR%\bk7231u_QIO.bin" "output\%APP_VERSION%\%APP_NAME%_QIO_%APP_VERSION%.bin" >nul 2>nul
    echo [INFO] Copied: %APP_NAME%_QIO_%APP_VERSION%.bin
    set /a COPIED_COUNT+=1
)
if exist "%GEN_DIR%\bk7231u_UA.bin" (
    copy /y "%GEN_DIR%\bk7231u_UA.bin" "output\%APP_VERSION%\%APP_NAME%_UA_%APP_VERSION%.bin" >nul 2>nul
    echo [INFO] Copied: %APP_NAME%_UA_%APP_VERSION%.bin
    set /a COPIED_COUNT+=1
)

echo.
echo ==============================================
echo BUILD SUCCESSFUL!  (%COPIED_COUNT% files copied)
echo ==============================================
echo.
echo Output directory:
echo   %CD%\output\%APP_VERSION%\
exit /b 0

