@echo off
REM CZ-101 Emulator Build Script (Test Target)

echo ========================================
echo CZ-101 Emulator Build Script (Test Target)
echo ========================================

REM 0. Inicializar entorno VS2022
echo.
echo 0. Inicializando entorno Visual Studio 2022...

set "CMAKE_GEN=-G "Visual Studio 17 2022" -A x64"

if defined VSCMD_VER goto :EnvReady

REM Check VS 18 first (Insiders/Community)
if exist "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" (
    echo Found VS 18 Community
    call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
    set "CMAKE_GEN="
    goto :EnvReady
)
if exist "C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" (
    echo Found VS 18 Insiders
    call "C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
    set "CMAKE_GEN="
    goto :EnvReady
)

REM Check VS 2022 (17)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
    goto :EnvReady
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
    goto :EnvReady
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
    goto :EnvReady
)

echo Warning: VS2022 no encontrado. Usando entorno actual...

:EnvReady

REM 1. Buscar CMake
echo.
echo 1. Buscando CMake...

set "CMAKE_PATH="

where cmake >nul 2>nul
if %errorlevel% equ 0 (
    set "CMAKE_PATH=cmake"
    goto :FoundCMake
)

if exist "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
    set "CMAKE_PATH=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    goto :FoundCMake
)
if exist "C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
    set "CMAKE_PATH=C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    goto :FoundCMake
)

if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
    set "CMAKE_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    goto :FoundCMake
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
    set "CMAKE_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    goto :FoundCMake
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
    set "CMAKE_PATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    goto :FoundCMake
)
if exist "C:\Program Files\CMake\bin\cmake.exe" (
    set "CMAKE_PATH=C:\Program Files\CMake\bin\cmake.exe"
    goto :FoundCMake
)

echo Error: CMake no encontrado
exit /b 1

:FoundCMake
echo    CMake encontrado: %CMAKE_PATH%

REM 6. Compilar proyecto (ONLY TEST TARGET)
echo.
echo 6. Compilando proyecto (Test)...
echo    (Esto puede tardar varios minutos...)

"%CMAKE_PATH%" --build build --config Release --target CZ101SysExTest
if %errorlevel% neq 0 (
    echo Error: Compilacion fallida
    exit /b 1
)

echo    Compilacion exitosa!
exit /b 0
