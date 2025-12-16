@echo off
REM CZ-101 Emulator Build Script (Custom Target)

REM 0. Initialize VS2022
set "CMAKE_GEN=-G "Visual Studio 17 2022" -A x64"
if defined VSCMD_VER goto :EnvReady
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
    goto :EnvReady
)
:EnvReady

REM 1. Find CMake
set "CMAKE_PATH="
where cmake >nul 2>nul
if %errorlevel% equ 0 (
    set "CMAKE_PATH=cmake"
    goto :FoundCMake
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
    set "CMAKE_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
)
:FoundCMake

if not defined CMAKE_PATH (
    echo Error: Could not find CMake
    exit /b 1
)

echo Using CMake: %CMAKE_PATH%
echo Building Target: %1

"%CMAKE_PATH%" --build build --config Release --target %1
if %errorlevel% neq 0 ( exit /b 1 )
echo Build Successful.
