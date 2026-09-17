@echo off
setlocal
echo ========================================
echo  ABDCZ101 WASM Build
echo ========================================
cd /d "%~dp0\.."

rem ------------------------------------------------------------------
rem  Emscripten environment - configured HERE, not via emsdk_env.bat.
rem  `call emsdk_env.bat >nul` silently fails to propagate the PATH
rem  (the redirection swallows the setx/PATH updates), which made the
rem  build fail with "emcmake is not recognized". We set the exact
rem  variables emscripten needs directly instead.
rem ------------------------------------------------------------------
set "EMSDK_ROOT=C:\emsdk"
set "EMSCRIPTEN=%EMSDK_ROOT%\upstream\emscripten"
set "EMSDK=%EMSDK_ROOT%"
set "EMSDK_NODE=%EMSDK_ROOT%\node\22.16.0_64bit\bin\node.exe"
set "EMSDK_PYTHON=%EMSDK_ROOT%\python\3.13.3_64bit\python.exe"
set "EM_CONFIG=%EMSDK_ROOT%\.emscripten"
set "PATH=%EMSCRIPTEN%;%EMSDK_ROOT%;%PATH%"

rem Guard: fail fast with a clear message if emsdk is not where expected.
if not exist "%EMSCRIPTEN%\emcc.exe" (
    echo ERROR: Emscripten not found at %EMSCRIPTEN%
    echo Set EMSDK_ROOT at the top of this file to your emsdk install path.
    exit /b 1
)

if exist "wasm\juce_shim\juce_core" (
    echo Cleaning juce_core shim...
    rmdir /s /q "wasm\juce_shim\juce_core"
)
echo Copying juce_core module...
powershell -NoProfile -Command "Copy-Item -Path 'C:\JUCE\modules\juce_core' -Destination 'wasm\juce_shim\juce_core' -Recurse -Force"
echo Overriding ThreadPriorities for Emscripten...
copy /y "wasm\juce_shim\native\juce_ThreadPriorities_native.h" "wasm\juce_shim\juce_core\native\juce_ThreadPriorities_native.h" >nul

if not exist "wasm\build" mkdir wasm\build
echo Configuring CMake with Emscripten...
call emcmake cmake -S wasm -B wasm\build -DCMAKE_BUILD_TYPE=Release -G Ninja
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
