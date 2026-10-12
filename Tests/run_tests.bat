@echo off
setlocal enabledelayedexpansion

:: ============================================================================
:: ABDCZ101 - Compilador y Ejecutor de Tests Unitarios Cz101ParamsTests
:: ============================================================================

echo.
echo ========================================================
echo   ABDCZ101 - Cz101 Wire-Domain Parameter Tests Runner
echo ========================================================
echo.

set "SCRIPT_DIR=%~dp0"
set "ROOT_DIR=%SCRIPT_DIR%.."
set "BUILD_DIR=%ROOT_DIR%\build"

:: 1. Intentar compilar si existe el proyecto CMake ya configurado
if exist "%BUILD_DIR%\CZ101Emulator.sln" (
    echo [INFO] Detectada solucion CMake en build\
    echo [INFO] Compilando target Cz101ParamsTests...
    cmake --build "%BUILD_DIR%" --config Release --target Cz101ParamsTests
    if !errorlevel! equ 0 (
        set "EXE_PATH=%BUILD_DIR%\Tests\Release\Cz101ParamsTests.exe"
        if not exist "!EXE_PATH!" set "EXE_PATH=%BUILD_DIR%\Release\Cz101ParamsTests.exe"
        if exist "!EXE_PATH!" (
            echo.
            echo [OK] Ejecutando tests...
            echo.
            "!EXE_PATH!"
            goto :done
        )
    )
)

:: 2. Si no hay CMake o fallo el target, compilar directamente con cl.exe
echo [INFO] Compilando standalone con cl.exe...

where cl.exe >nul 2>&1
if %errorlevel% neq 0 (
    :: Buscar vcvars64 / Visual Studio 2026 / BuildTools
    set "VCVARS="
    for %%P in (
        "C:\Program Files\Microsoft Visual Studio\18\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    ) do (
        if exist %%P if not defined VCVARS set "VCVARS=%%~P"
    )

    if defined VCVARS (
        echo [INFO] Cargando entorno MSVC desde: !VCVARS!
        call "!VCVARS!"
    ) else (
        echo [ERROR] cl.exe no encontrado en PATH y no se detecto vcvars64.bat.
        echo Por favor ejecuta este script desde el "Developer Command Prompt" de Visual Studio.
        pause
        exit /b 1
    )
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

echo [INFO] Compilando Cz101ParamsTests.cpp con C++20...
cl.exe /nologo /EHsc /std:c++20 /O2 ^
    /I"%ROOT_DIR%\..\ABDSharedCode" ^
    /I"%ROOT_DIR%\Source" ^
    "%SCRIPT_DIR%Cz101ParamsTests.cpp" ^
    /Fe:"%BUILD_DIR%\Cz101ParamsTests.exe" ^
    /Fo:"%BUILD_DIR%\Cz101ParamsTests.obj"

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Fallo la compilacion de Cz101ParamsTests.
    pause
    exit /b 1
)

echo.
echo [OK] Compilacion exitosa. Ejecutando tests:
echo ========================================================
echo.
"%BUILD_DIR%\Cz101ParamsTests.exe"

:done
echo.
echo ========================================================
echo   Ejecucion finalizada.
echo ========================================================
pause
