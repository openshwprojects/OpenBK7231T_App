@echo off
setlocal EnableDelayedExpansion

:: ========================================================
:: OpenBeken Berry prebuild script
:: Must be called from the repository root directory
:: ========================================================

echo [INFO] Running Berry prebuild...

set PYTHON_CMD=
where python >nul 2>nul
if %errorlevel% equ 0 (
    set "PYTHON_CMD=python"
) else (
    where python3 >nul 2>nul
    if !errorlevel! equ 0 (
        set "PYTHON_CMD=python3"
    )
)

if "%PYTHON_CMD%"=="" (
    echo [WARNING] Neither 'python' nor 'python3' was found in PATH!
    echo [WARNING] Berry prebuild step will be skipped. Please install Python to generate Berry bindings.
    exit /b 0
)

echo [INFO] Using Python command: !PYTHON_CMD!

if not exist "libraries\berry\generate" (
    echo [INFO] Creating libraries\berry\generate directory...
    mkdir "libraries\berry\generate"
)

echo [INFO] Executing Berry C-Object-Compiler (coc)...
!PYTHON_CMD! libraries\berry\tools\coc\coc -o libraries\berry\generate libraries\berry\src src\berry\modules -c include\berry_conf.h

if !errorlevel! neq 0 (
    echo [ERROR] Berry prebuild failed with exit code !errorlevel!!
    exit /b !errorlevel!
)

echo [INFO] Berry prebuild completed successfully.
exit /b 0
