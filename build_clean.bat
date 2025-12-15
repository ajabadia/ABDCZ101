@echo off
REM CZ-101 Emulator Build Script
REM Version con limpieza automática de build

echo ========================================
echo CZ-101 Emulator Build Script
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

REM Check VS 18 Insiders CMake (vcpkg location)
if exist "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\vcpkg\scripts\cmake\cmake.exe" (
    set "CMAKE_PATH=C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\vcpkg\scripts\cmake\cmake.exe"
    goto :FoundCMake
)

REM Check VS 18 Community CMake
if exist "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
    set "CMAKE_PATH=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    goto :FoundCMake
)

REM Check Standard VS2022 Paths
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
echo Instala CMake o ejecuta desde Developer Command Prompt for VS 2022
pause
exit /b 1

:FoundCMake
echo    CMake encontrado: %CMAKE_PATH%

REM 2. Verificar JUCE
echo.
echo 2. Verificando JUCE...

if not exist "C:\JUCE\CMakeLists.txt" (
    echo Error: JUCE no encontrado en C:\JUCE\
    pause
    exit /b 1
)
echo    JUCE encontrado en C:\JUCE\

REM 3. Limpiar build anterior
echo.
echo 3. Limpiando build anterior...

if exist build (
    echo    Eliminando directorio build...
    rmdir /s /q build
)

REM 4. Configurar proyecto
echo.
echo 4. Configurando proyecto con CMake...
echo    (Esto puede tardar la primera vez...)

"%CMAKE_PATH%" -B build %CMAKE_GEN%
if %errorlevel% neq 0 (
    echo Error: Configuracion fallida
    pause
    exit /b 1
)

echo    Configuracion completada

REM 5. Compilar proyecto
echo.
echo 5. Compilando proyecto (Release)...
echo    (Esto puede tardar varios minutos...)

"%CMAKE_PATH%" --build build --config Release --target CZ101Emulator_Standalone
if %errorlevel% neq 0 (
    echo Error: Compilacion fallida
    pause
    exit /b 1
)

echo    Compilacion exitosa!

REM 6. Copiar ejecutable
echo.
echo 6. Copiando ejecutable...

if exist "build\CZ101Emulator_artefacts\Release\Standalone\CZ-101 Emulator.exe" (
    copy "build\CZ101Emulator_artefacts\Release\Standalone\CZ-101 Emulator.exe" "CZ101Emulator.exe"
    if %errorlevel% neq 0 (
        echo Warning: No se pudo copiar ejecutable
    ) else (
        echo    Ejecutable copiado a: %CD%\CZ101Emulator.exe
    )
) else (
    echo Warning: Ejecutable no encontrado en ruta esperada
    echo Busca en: build\CZ101Emulator_artefacts\
)

echo.
echo ========================================
echo BUILD COMPLETADO EXITOSAMENTE
echo ========================================
echo.
echo Archivos generados:
echo   - Standalone: CZ101Emulator.exe
echo   - VST3: build\CZ101Emulator_artefacts\Release\VST3\
echo.

pause
