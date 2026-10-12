@echo off
setlocal
echo ========================================
echo  ABDCZ101 WASM Build
echo ========================================
cd /d "%~dp0\.."

rem ------------------------------------------------------------------
rem  Emscripten environment detection
rem ------------------------------------------------------------------
if defined EMSDK_ROOT goto check_emsdk
if exist "d:\desarrollos\emsdk\upstream\emscripten\emcc.exe" set "EMSDK_ROOT=d:\desarrollos\emsdk"& goto check_emsdk
if exist "C:\emsdk\upstream\emscripten\emcc.exe" set "EMSDK_ROOT=C:\emsdk"& goto check_emsdk
if defined EMSDK set "EMSDK_ROOT=%EMSDK%"

:check_emsdk
set "EMSCRIPTEN=%EMSDK_ROOT%\upstream\emscripten"
if not exist "%EMSCRIPTEN%\emcc.exe" (
    echo ERROR: Emscripten not found at %EMSCRIPTEN%
    echo Set EMSDK_ROOT at the top of this file to your emsdk install path.
    exit /b 1
)

if exist "%EMSDK_ROOT%\emsdk_env.bat" (
    call "%EMSDK_ROOT%\emsdk_env.bat" >nul 2>&1
)
set "EMSDK=%EMSDK_ROOT%"
set "PATH=%EMSCRIPTEN%;%EMSDK_ROOT%;%PATH%"

rem ------------------------------------------------------------------
rem  JUCE modules detection
rem ------------------------------------------------------------------
set "JUCE_PATH=d:/desarrollos/juce"
set "JUCE_MODULES_DIR="
if exist "d:\desarrollos\juce\modules\juce_core" (
    set "JUCE_PATH=d:/desarrollos/juce"
    set "JUCE_MODULES_DIR=d:\desarrollos\juce\modules"
) else if exist "C:\JUCE\modules\juce_core" (
    set "JUCE_PATH=C:/JUCE"
    set "JUCE_MODULES_DIR=C:\JUCE\modules"
)

if exist "wasm\juce_shim\juce_core" (
    echo Cleaning juce_core shim...
    rmdir /s /q "wasm\juce_shim\juce_core"
)

if defined JUCE_MODULES_DIR (
    echo Copying juce_core module from %JUCE_MODULES_DIR%...
    powershell -NoProfile -Command "Copy-Item -Path '%JUCE_MODULES_DIR%\juce_core' -Destination 'wasm\juce_shim\juce_core' -Recurse -Force"
    echo Overriding ThreadPriorities for Emscripten...
    copy /y "wasm\juce_shim\native\juce_ThreadPriorities_native.h" "wasm\juce_shim\juce_core\native\juce_ThreadPriorities_native.h" >nul
) else (
    echo [WARNING] No se encontro juce_core en d:\desarrollos\juce ni en C:\JUCE.
)

if not exist "wasm\build" mkdir wasm\build
if exist "wasm\build\CMakeCache.txt" del /q "wasm\build\CMakeCache.txt"
if exist "wasm\build\CMakeFiles" rmdir /s /q "wasm\build\CMakeFiles"

echo Configuring CMake with Emscripten...
set "NINJA_PATH=%EMSCRIPTEN%\ninja.exe"
call emcmake cmake -S wasm -B wasm\build -DCMAKE_BUILD_TYPE=Release -DJUCE_PATH="%JUCE_PATH%" -DCMAKE_MAKE_PROGRAM="%NINJA_PATH%" -G Ninja
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake configuration failed!
    exit /b 1
)

echo Building WASM module...
cmake --build wasm\build
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed!
    exit /b 1
)

echo ========================================
echo  Build complete!
echo  Output: WebUI\wasm\cz101_dsp.js
echo          WebUI\wasm\cz101_dsp.wasm
echo ========================================
