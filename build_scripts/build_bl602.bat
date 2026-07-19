@echo off
setlocal EnableDelayedExpansion
pushd "%~dp0\.."

:: ========================================================
:: OpenBeken BL602 Windows build script
:: Uses Git Bash + Built-in SDK RISC-V toolchain
:: ========================================================

set APP_NAME=OpenBL602_App
set APP_VERSION=dev_bl602
set SDK_DIR=sdk\OpenBL602
set OBK_VARIANT=2
set ACTION=build

:: Allow overriding version from command line
if not "%~1"=="" set APP_VERSION=%~1
if not "%~2"=="" set ACTION=%~2

echo ==============================================
echo Building OpenBeken for BL602 natively on Windows
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
    if exist "%%~P" (
        set "GIT_BASH=%%~P"
        goto :found_bash
    )
)
echo [ERROR] Git Bash not found! Please install Git for Windows.
exit /b 1

:found_bash
echo [INFO] Using Git Bash: %GIT_BASH%

:: 2. Check for GNU make
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
echo [ERROR] GNU make not found. Please install GNU make (e.g. via pacman or choco).
exit /b 1

:found_make
:: Add make to path for this session so bash natively sees it
set PATH=%MSYS_MAKE%;%PATH%
echo [INFO] make found at %MSYS_MAKE%

:: 3. Python prerequisites for packing BL602 images (they might already be in PATH)
python -c "import pkg_resources; pkg_resources.require(['fdt', 'toml', 'configobj', 'pycryptodomex', 'pycryptodome'])" >nul 2>nul
if !errorlevel! neq 0 (
    echo [INFO] Installing required Python dependencies...
    python -m pip install fdt toml configobj pycryptodomex pycryptodome
) else (
    echo [INFO] Python dependencies OK.
)
:: Also ensure Git Bash python has them, because the Makefile uses bash python!
"%GIT_BASH%" --login -c "python -m pip install fdt toml configobj pycryptodome" >nul 2>nul

:: 4. Check that SDK submodule is checked out
if not exist "%SDK_DIR%\customer_app\bl602_sharedApp\Makefile" (
    echo [INFO] SDK submodule not found, checking out...
    git submodule update --init --recursive --depth=1 %SDK_DIR%
    if !errorlevel! neq 0 (
        echo [ERROR] Failed to checkout SDK submodule!
        exit /b 1
    )
)
echo [INFO] SDK submodule OK.

:: 5. Create symlink for Git Bash (MINGW64) to use MSYS toolchain
if not exist "%SDK_DIR%\toolchain\riscv\MINGW64" (
    mklink /J "%SDK_DIR%\toolchain\riscv\MINGW64" "%SDK_DIR%\toolchain\riscv\MSYS" >nul 2>nul
)


:: --- Berry prebuild ---
call build_scripts\berry_prebuild.bat

:: Ensure output directory exists
if not exist "output\%APP_VERSION%" mkdir "output\%APP_VERSION%"

:: --- Build via Git Bash ---
echo [INFO] Starting %ACTION% via Git Bash...
echo.

:: Fix for GNU Assembler "Cannot create temporary file in C:\windows\"
set TEMP=%CD%\output\tmp
set TMP=%CD%\output\tmp
set TMPDIR=%CD%\output\tmp
if not exist "%TEMP%" mkdir "%TEMP%"

if "%ACTION%"=="clean" goto do_clean

:: Find native Windows python
for /f "delims=" %%I in ('where python') do (
    set "WIN_PYTHON_PATH=%%~fI"
    goto found_win_python
)
:found_win_python
set "WIN_PYTHON_PATH_CYG=!WIN_PYTHON_PATH:\=/!"

:: Create python3 wrapper to force MSYS to use native Windows Python
echo #!/bin/bash> "%TEMP%\python3"
echo "!WIN_PYTHON_PATH_CYG!" "$@">> "%TEMP%\python3"
"%GIT_BASH%" --login -c "chmod +x $(cygpath -u '%TEMP%\python3')"

:: Run make via Git bash, explicitly injecting TMP vars as make arguments to appease the GNU assembler
"%GIT_BASH%" --login -c "export LOCAL_TMP=$(cygpath -m '%USERPROFILE%\AppData\Local\Temp'); make APP_VERSION=%APP_VERSION% OBK_VARIANT=%OBK_VARIANT% TEMP=\"$LOCAL_TMP\" TMP=\"$LOCAL_TMP\" TMPDIR=\"$LOCAL_TMP\" OpenBL602"
set BUILD_RESULT=!errorlevel!
:: Ignore exit code if the root .bin was generated successfully (indicating only the Python OTA step failed in bash)
if exist "%SDK_DIR%\customer_app\bl602_sharedApp\build_out\bl602_sharedApp.bin" (
    echo [INFO] Firmware compiled successfully. Running native Python OTA generation...
    pushd "%SDK_DIR%\image_conf"
    python flash_build.py OpenBL602_App BL602
    popd

    echo [INFO] Copying output files...
    copy /y "%SDK_DIR%\customer_app\bl602_sharedApp\build_out\bl602_sharedApp.bin" "output\%APP_VERSION%\OpenBL602_%APP_VERSION%.bin" >nul
    copy /y "%SDK_DIR%\customer_app\bl602_sharedApp\build_out\ota\dts40M_pt2M_boot2release_ef4015\FW_OTA.bin" "output\%APP_VERSION%\OpenBL602_%APP_VERSION%_OTA.bin" >nul
    copy /y "%SDK_DIR%\customer_app\bl602_sharedApp\build_out\ota\dts40M_pt2M_boot2release_ef4015\FW_OTA.bin.xz" "output\%APP_VERSION%\OpenBL602_%APP_VERSION%_OTA.bin.xz" >nul 2>nul
    
    set BUILD_RESULT=0
)
goto after_build

:do_clean
"%GIT_BASH%" --login -c "make -C %SDK_DIR%/customer_app/bl602_sharedApp clean"
set BUILD_RESULT=!errorlevel!

:after_build

if !BUILD_RESULT! neq 0 (
    echo.
    echo [ERROR] BL602 build failed with exit code !BUILD_RESULT!!
    exit /b !BUILD_RESULT!
)

if "%ACTION%"=="clean" (
    echo.
    echo [INFO] Clean complete!
    exit /b 0
)

echo.
echo ==============================================
echo BUILD SUCCESSFUL!
echo ==============================================
echo.
echo Output directory:
echo   %CD%\output\%APP_VERSION%\
echo.
echo Build files:
echo ----------------------------------------------
for %%F in (output\%APP_VERSION%\OpenBL602_*) do (
    echo   %%~nxF  (%%~zF bytes)
)
echo ==============================================
exit /b 0
