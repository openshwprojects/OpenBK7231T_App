@echo off
setlocal EnableDelayedExpansion
pushd "%~dp0\.."

:: Default settings
set APP_VERSION=dev_ln882h
set OBK_VARIANT=2
set ACTION=build

if not "%~1"=="" set APP_VERSION=%~1
if not "%~2"=="" set OBK_VARIANT=%~2
if not "%~3"=="" set ACTION=%~3

echo ==============================================
echo Building OpenBeken LN882H on Windows natively
echo App Version: %APP_VERSION%
echo Variant (2=tuyaMCU): %OBK_VARIANT%
echo Action: %ACTION%
echo ==============================================

:: 1. Check prerequisites

:: Git Bash
set GIT_BASH=
for %%P in (
    "C:\Program Files\Git\bin\bash.exe"
    "C:\Program Files (x86)\Git\bin\bash.exe"
) do (
    if exist "%%~P" (
        set "GIT_BASH=%%~P"
        goto :found_bash
    )
)
echo [ERROR] Git Bash not found! Please install Git for Windows.
exit /b 1

:found_bash
echo [INFO] Using Git Bash: %GIT_BASH%

:: Check CMake
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] 'cmake' not found in PATH!
    echo [INFO] Please install CMake from cmake.org or via your package manager.
    exit /b 1
)

:: Check Ninja
where ninja >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] 'ninja' not found in PATH!
    echo [INFO] LN882H build uses Ninja generator. Please install Ninja and add it to your PATH.
    exit /b 1
)
echo [INFO] CMake and Ninja found.

:: 2. Submodules
if not exist "sdk\OpenLN882H\CMakeLists.txt" (
    echo [INFO] OpenLN882H submodule not found, checking out...
    git submodule update --init --recursive sdk\OpenLN882H
    if !errorlevel! neq 0 (
        echo [ERROR] Failed to checkout OpenLN882H submodule!
        exit /b 1
    )
)
echo [INFO] LN882H submodule OK.

if not exist "libraries\berry\src\be_api.c" (
    echo [INFO] Berry submodule not found, checking out...
    git submodule update --init --depth=1 libraries\berry
)
echo [INFO] Berry submodule OK.

:: 3. Toolchain
set "CROSS_TOOLCHAIN_ROOT=%CD%\sdk\OpenBK7231N\platforms\BK7231N\toolchain\windows\gcc-arm-none-eabi-4_9-2015q1"
if not exist "%CROSS_TOOLCHAIN_ROOT%\bin\arm-none-eabi-gcc.exe" (
    echo [ERROR] ARM GCC not found at !CROSS_TOOLCHAIN_ROOT!
    echo [INFO] We reuse the BK7231N toolchain. Please ensure BK7231N SDK is updated.
    git submodule update --init --depth=1 sdk/OpenBK7231N
)
echo [INFO] Toolchain set to: !CROSS_TOOLCHAIN_ROOT!

:: Export APP_VERSION and OBK_VARIANT for Python and CMake
set APP_VERSION=%APP_VERSION%
set OBK_VARIANT=%OBK_VARIANT%

:: 4. Python3 workaround for subprocess "python3"
echo [INFO] Resolving Python dependency...
for /f "delims=" %%I in ('where python') do (
    set "WIN_PYTHON_PATH=%%~fI"
    goto found_win_python
)
:found_win_python
set PYTHON_TMP=%CD%\output\tmp_python
if not exist "%PYTHON_TMP%" mkdir "%PYTHON_TMP%"
if not exist "%PYTHON_TMP%\python3.exe" (
    copy /y "!WIN_PYTHON_PATH!" "%PYTHON_TMP%\python3.exe" >nul
)
set PATH=%PYTHON_TMP%;%PATH%

:: --- Berry prebuild ---
call build_scripts\berry_prebuild.bat

if "%ACTION%"=="clean" (
    pushd sdk\OpenLN882H
    python start_build.py clean
    popd
    echo [INFO] Clean complete!
    exit /b 0
)

:: 4. Pre-build script / App Symlink
if not exist "sdk\OpenLN882H\project\OpenBeken\app" (
    echo [INFO] Creating junction for app in LN882H OpenBeken project...
    mklink /J "%CD%\sdk\OpenLN882H\project\OpenBeken\app" "%CD%" >nul 2>nul
    if !errorlevel! neq 0 (
        echo [ERROR] Failed to create junction. Try running as Administrator.
        exit /b 1
    )
)

if exist "platforms\LN882H\pre_build.sh" (
    echo [INFO] Running LN882H pre-build bash script...
    "%GIT_BASH%" --login -c "cd '%CD:\=/%'; platforms/LN882H/pre_build.sh"
)

:: 5. Compile
echo [INFO] Building LN882H...
pushd sdk\OpenLN882H
python start_build.py %ACTION%
set BUILD_RESULT=!errorlevel!
popd

if !BUILD_RESULT! neq 0 (
    echo [ERROR] LN882H build failed with exit code !BUILD_RESULT!
    exit /b !BUILD_RESULT!
)

:: Ensure output directory exists
if not exist "output\%APP_VERSION%" mkdir "output\%APP_VERSION%"

set OUT_NAME=OpenLN882H_%APP_VERSION%

:: Package binaries (The python script writes to bin/flashimage.bin inside sdk/OpenLN882H/build)
echo [INFO] Copying binaries...

set BLD_DIR=sdk\OpenLN882H\build-OpenBeken-release\bin
set FACTORY_BIN=output\%APP_VERSION%\%OUT_NAME%.factory.bin

if exist "%BLD_DIR%\flashimage.bin" (
    copy /y "%BLD_DIR%\flashimage.bin" "!FACTORY_BIN!" >nul
    echo [INFO] Factory binary created at: !FACTORY_BIN!
) else (
    echo [WARNING] Flash image not found at %BLD_DIR%\flashimage.bin!
)

:: The OTA image in LN882H
if exist "%BLD_DIR%\OpenBeken.bin" (
    copy /y "%BLD_DIR%\OpenBeken.bin" "output\%APP_VERSION%\%OUT_NAME%_OTA.bin" >nul
)

echo ==============================================
echo BUILD SUCCESSFUL!
echo ==============================================
exit /b 0
