@echo off
setlocal enabledelayedexpansion
echo --- OpenBK Build Tool EXE Creator ---
echo.

:: Change to the directory where the batch script is located
cd /d "%~dp0"
echo Working directory: %cd%

:: Check if source file exists
if not exist "GUI_build_tool.py" (
    echo [ERR] GUI_build_tool.py not found in current directory!
    pause
    exit /b 1
)

:: Ensure PyInstaller is installed
python -m PyInstaller --version >nul 2>&1
if !errorlevel! neq 0 (
    echo PyInstaller not found. Installing...
    python -m pip install pyinstaller
)

echo Building single-file EXE...
python -m PyInstaller --noconsole --onefile --name "OpenBK GUI Build Tool" GUI_build_tool.py

if !errorlevel! equ 0 (
    echo.
    echo [OK] Build successful!
    
    echo Moving EXE to current directory...
    if exist "dist\OpenBK GUI Build Tool.exe" (
        move /y "dist\OpenBK GUI Build Tool.exe" "OpenBK GUI Build Tool.exe"
    )
    
    echo cleaning up temporary build files...
    if exist "build" rmdir /s /q build
    if exist "dist" rmdir /s /q dist
    if exist "OpenBK GUI Build Tool.spec" del /f /q "OpenBK GUI Build Tool.spec"
    
    echo.
    echo Done! Your standalone EXE is ready in the current folder: %cd%\OpenBK GUI Build Tool.exe
) else (
    echo.
    echo [ERR] PyInstaller failed with exit code !errorlevel!.
    echo Please check the output above for errors.
)

pause
