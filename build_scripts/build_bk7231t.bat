@echo off
setlocal EnableDelayedExpansion
pushd "%~dp0\.."

:: ========================================================
:: OpenBeken BK7231T Windows build script
:: Uses Git Bash + bundled ARM GCC toolchain + Windows
:: packaging tools that are all already in the SDK.
:: ========================================================

set APP_NAME=OpenBK7231T_App
set APP_VERSION=dev_bk7231t
set SDK_DIR=sdk\OpenBK7231T
set OBK_VARIANT=2
set ACTION=build

:: Allow overriding version from command line
if not "%~1"=="" set APP_VERSION=%~1
if not "%~2"=="" set ACTION=%~2

echo ==============================================
echo Building OpenBeken for BK7231T on Windows
echo App Name:    %APP_NAME%
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
if not exist "%SDK_DIR%\platforms\bk7231t\bk7231t_os\build.sh" (
    echo [INFO] SDK submodule not found, checking out...
    git submodule update --init --recursive --depth=1 %SDK_DIR%
    if !errorlevel! neq 0 (
        echo [ERROR] Failed to checkout SDK submodule!
        exit /b 1
    )
)
echo [INFO] SDK submodule OK.

:: 2b. Check that Berry submodule is checked out
if not exist "libraries\berry\src\be_api.c" (
    echo [INFO] Berry submodule not found, checking out...
    git submodule update --init --depth=1 libraries\berry
    if !errorlevel! neq 0 (
        echo [ERROR] Failed to checkout Berry submodule!
        exit /b 1
    )
)
echo [INFO] Berry submodule OK.

:: 3. Check that Windows ARM GCC toolchain exists
if not exist "%SDK_DIR%\platforms\bk7231t\toolchain\windows\gcc-arm-none-eabi-4_9-2015q1\bin\arm-none-eabi-gcc.exe" (
    echo [ERROR] Windows ARM GCC toolchain not found in SDK!
    echo Expected at: %SDK_DIR%\platforms\bk7231t\toolchain\windows\gcc-arm-none-eabi-4_9-2015q1\bin\
    exit /b 1
)
echo [INFO] ARM GCC toolchain OK.

:: 4. Ensure our app symlink/junction exists in SDK apps folder
if not exist "%SDK_DIR%\apps\%APP_NAME%" (
    echo [INFO] Creating junction for app in SDK apps folder...
    mklink /J "%SDK_DIR%\apps\%APP_NAME%" "%CD%"
    if !errorlevel! neq 0 (
        echo [ERROR] Failed to create junction. Try running as Administrator.
        exit /b 1
    )
)
echo [INFO] App link OK.

:: 5. Ensure mbedtls is downloaded (needed by the top-level Makefile)
if not exist "output\mbedtls-2.28.5" (
    echo [INFO] Downloading mbedtls...
    "%GIT_BASH%" -c "mkdir -p output && cd output && curl -sL https://github.com/Mbed-TLS/mbedtls/archive/refs/tags/v2.28.5.tar.gz -o v2.28.5.tar.gz && tar -xf v2.28.5.tar.gz && rm -f v2.28.5.tar.gz && mv mbedtls-2.28.5/library/base64.c mbedtls-2.28.5/library/base64_mbedtls.c"
    if !errorlevel! neq 0 (
        echo [WARN] mbedtls download may have failed, build might still work.
    )
)

:: 6. Ensure output directory exists
if not exist "output\%APP_VERSION%" mkdir "output\%APP_VERSION%"


:: --- Berry prebuild ---
call build_scripts\berry_prebuild.bat

:: --- Build via Git Bash ---
:: The build.sh and application.mk already handle Windows vs Linux toolchain paths.
:: The trick is: application.mk uses `uname` to decide toolchain dir, and build.sh
:: uses `uname` to pick the Windows package tools.
:: Under Git Bash, uname returns MINGW64_NT-... which is NOT "Linux", so it picks
:: the Windows paths. We just need to provide `make` to Git Bash.

echo [INFO] Starting %ACTION% via Git Bash...
echo.

:: Run the inner build script, passing parameters as arguments.
:: This avoids the fragile inline echo approach that loses $variables.
"%GIT_BASH%" --login -i "%CD%\build_scripts\build_bk7231t_inner.sh" "%APP_NAME%" "%APP_VERSION%" "%SDK_DIR%" "%OBK_VARIANT%" "%ACTION%"
set BUILD_RESULT=%errorlevel%

if %BUILD_RESULT% neq 0 (
    echo.
    echo [ERROR] BK7231T build failed with exit code %BUILD_RESULT%!
    echo.
    echo [INFO] Checking for partial build artifacts...
    echo [INFO] Looking in: %SDK_DIR%\platforms\bk7231t\bk7231t_os\tools\generate\
    if exist "%SDK_DIR%\platforms\bk7231t\bk7231t_os\tools\generate" (
        dir /b "%SDK_DIR%\platforms\bk7231t\bk7231t_os\tools\generate\*.bin" 2>nul
        dir /b "%SDK_DIR%\platforms\bk7231t\bk7231t_os\tools\generate\*.rbl" 2>nul
    )
    echo.
    echo [INFO] Checking output dir: output\%APP_VERSION%\
    if exist "output\%APP_VERSION%" (
        dir /b "output\%APP_VERSION%\*.bin" 2>nul
        dir /b "output\%APP_VERSION%\*.rbl" 2>nul
        dir /b "output\%APP_VERSION%\*.axf" 2>nul
    ) else (
        echo [INFO] Output dir does not exist yet.
    )
    exit /b %BUILD_RESULT%
)

:: --- Copy output ---
echo.
echo [INFO] Checking output files...
set GEN_DIR=%SDK_DIR%\platforms\bk7231t\bk7231t_os\tools\generate

set COPIED_COUNT=0
if exist "%GEN_DIR%\%APP_NAME%_%APP_VERSION%.rbl" (
    copy /y "%GEN_DIR%\%APP_NAME%_%APP_VERSION%.rbl" "output\%APP_VERSION%\" >nul 2>nul
    echo [INFO] Copied: %APP_NAME%_%APP_VERSION%.rbl
    set /a COPIED_COUNT+=1
)
if exist "%GEN_DIR%\%APP_NAME%_UG_%APP_VERSION%.bin" (
    copy /y "%GEN_DIR%\%APP_NAME%_UG_%APP_VERSION%.bin" "output\%APP_VERSION%\" >nul 2>nul
    echo [INFO] Copied: %APP_NAME%_UG_%APP_VERSION%.bin
    set /a COPIED_COUNT+=1
)
if exist "%GEN_DIR%\%APP_NAME%_UA_%APP_VERSION%.bin" (
    copy /y "%GEN_DIR%\%APP_NAME%_UA_%APP_VERSION%.bin" "output\%APP_VERSION%\" >nul 2>nul
    echo [INFO] Copied: %APP_NAME%_UA_%APP_VERSION%.bin
    set /a COPIED_COUNT+=1
)
if exist "%GEN_DIR%\%APP_NAME%_QIO_%APP_VERSION%.bin" (
    copy /y "%GEN_DIR%\%APP_NAME%_QIO_%APP_VERSION%.bin" "output\%APP_VERSION%\" >nul 2>nul
    echo [INFO] Copied: %APP_NAME%_QIO_%APP_VERSION%.bin
    set /a COPIED_COUNT+=1
)

echo.
echo ==============================================
echo BUILD SUCCESSFUL!  (%COPIED_COUNT% files copied)
echo ==============================================
echo.
echo Output directory:
echo   %CD%\output\%APP_VERSION%\
echo.
echo Build files:
echo ----------------------------------------------
for %%F in (output\%APP_VERSION%\*.bin output\%APP_VERSION%\*.rbl) do (
    echo   %%~nxF  (%%~zF bytes)
)
echo ==============================================
exit /b 0

