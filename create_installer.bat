@echo off
setlocal enabledelayedexpansion

echo ==========================================
echo Building code-ZEN Installer
echo ==========================================

:: 1. Compile/Build the latest Release binary
echo 1. Building code-ZEN in Release mode...
call build.bat
if %errorlevel% neq 0 (
    echo FATAL ERROR: code-ZEN build failed. Cannot package installer.
    exit /b %errorlevel%
)

:: 2. Find makensis.exe
echo 2. Locating NSIS compiler (makensis.exe)...
set "MAKENSIS_PATH="

:: Try local workspace NSIS first
if exist "%~dp0nsis-3.12\nsis-3.12\makensis.exe" (
    set "MAKENSIS_PATH=%~dp0nsis-3.12\nsis-3.12\makensis.exe"
    echo Found makensis in local workspace folder: !MAKENSIS_PATH!
)

:: Try PATH next if not found locally
if not defined MAKENSIS_PATH (
    where makensis >nul 2>&1
    if !errorlevel! equ 0 (
        set "MAKENSIS_PATH=makensis"
        echo Found makensis in system PATH.
    )
)

:: Try standard installation paths if not on PATH
if not defined MAKENSIS_PATH (
    if exist "C:\Program Files (x86)\NSIS\makensis.exe" (
        set "MAKENSIS_PATH=C:\Program Files (x86)\NSIS\makensis.exe"
        echo Found makensis at: !MAKENSIS_PATH!
    ) else if exist "C:\Program Files\NSIS\makensis.exe" (
        set "MAKENSIS_PATH=C:\Program Files\NSIS\makensis.exe"
        echo Found makensis at: !MAKENSIS_PATH!
    )
)

if not defined MAKENSIS_PATH (
    echo FATAL ERROR: makensis.exe not found. Please install NSIS or add it to PATH.
    exit /b 1
)

:: 3. Run NSIS compiler to package the installer
echo 3. Running NSIS compiler...
"!MAKENSIS_PATH!" installer.nsi
if %errorlevel% neq 0 (
    echo FATAL ERROR: NSIS compilation failed.
    exit /b %errorlevel%
)

echo ==========================================
echo INSTALLER BUILD SUCCESSFUL!
echo Output file: code-ZEN-Installer.exe
echo ==========================================
exit /b 0
