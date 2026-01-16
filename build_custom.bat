@echo off
REM CZ-101 Emulator Build Script (Auto-Config & Build)

REM 0. Initialize VS2022
if defined VSCMD_VER goto :EnvReady
set "VS_CMD="
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" set "VS_CMD=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" set "VS_CMD=C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat" set "VS_CMD=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat"

if defined VS_CMD (
    call "%VS_CMD%" -arch=x64 -host_arch=x64
    goto :EnvReady
)
echo Warning: VS Dev Cmd not found. Assuming environment is already set.

:EnvReady

REM 1. Find CMake
set "CMAKE_PATH="
where cmake >nul 2>nul
if %errorlevel% equ 0 (
    set "CMAKE_PATH=cmake"
    goto :FoundCMake
)

REM Check VS 18 usage of CMake (From build.bat)
if exist "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
    set "CMAKE_PATH=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    goto :FoundCMake
)
if exist "C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
    set "CMAKE_PATH=C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    goto :FoundCMake
)

REM Check VS 2022
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" set "CMAKE_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" & goto :FoundCMake
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" set "CMAKE_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" & goto :FoundCMake
if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" set "CMAKE_PATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" & goto :FoundCMake

REM Check Standard
if exist "C:\Program Files\CMake\bin\cmake.exe" set "CMAKE_PATH=C:\Program Files\CMake\bin\cmake.exe" & goto :FoundCMake

:FoundCMake
if not defined CMAKE_PATH (
    echo Error: Could not find CMake. Please install it or add it to PATH.
    exit /b 1
)

echo Using CMake: "%CMAKE_PATH%"

REM 2. Check for Configuration
if not exist "build\CMakeCache.txt" (
    echo Configuration missing. Running CMake Configure...
    "%CMAKE_PATH%" -B build -G "Visual Studio 17 2022" -A x64
    if %errorlevel% neq 0 (
        echo Error: Configuration failed.
        exit /b 1
    )
)

REM 3. Build
set "TARGET_ARG="
if not "%1"=="" set "TARGET_ARG=--target %1"

echo Building...
"%CMAKE_PATH%" --build build --config Release %TARGET_ARG%
if %errorlevel% neq 0 ( exit /b 1 )
echo Build Successful.
if exist "build\CZ101Emulator_artefacts\Release\Standalone\CZ-101 Emulator.exe" (
    copy "build\CZ101Emulator_artefacts\Release\Standalone\CZ-101 Emulator.exe" "CZ-5000_Emulator.exe" /Y
    echo    Ejecutable copiado a: CZ-5000_Emulator.exe
)
