@echo off
setlocal EnableDelayedExpansion

:: ========================================================
:: OpenBeken Berry prebuild script
:: Can be called from any directory - resolves repo root
:: from the script's own location.
:: ========================================================

:: Resolve repo root (one level up from build_scripts)
set "REPO_ROOT=%~dp0.."
pushd "!REPO_ROOT!"
set "REPO_ROOT=%CD%"
popd

echo [INFO] Running Berry prebuild...
echo [INFO] Repo root: !REPO_ROOT!

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

if not exist "!REPO_ROOT!\libraries\berry\generate" (
    echo [INFO] Creating libraries\berry\generate directory...
    mkdir "!REPO_ROOT!\libraries\berry\generate"
)

echo [INFO] Executing Berry C-Object-Compiler (coc)...
!PYTHON_CMD! "!REPO_ROOT!\libraries\berry\tools\coc\coc" -o "!REPO_ROOT!\libraries\berry\generate" "!REPO_ROOT!\libraries\berry\src" "!REPO_ROOT!\src\berry\modules" -c "!REPO_ROOT!\include\berry_conf.h"

if !errorlevel! neq 0 (
    echo [ERROR] Berry prebuild failed with exit code !errorlevel!!
    exit /b !errorlevel!
)

echo [INFO] Berry prebuild completed successfully.
exit /b 0
