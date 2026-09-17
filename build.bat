@echo off
echo ============================================================
echo  1. Building Native Plugin (Standalone and VST3 - Release)
echo ============================================================
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\manage.ps1" -task build -config Release
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Native plugin build failed!
    exit /b 1
)
echo ============================================================
echo  2. Building WebAssembly Target (WASM + AudioWorklet)
echo ============================================================
call "%~dp0wasm\build_wasm.bat"
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: WebAssembly build failed!
    exit /b 1
)
echo ============================================================
echo  All targets built successfully!
echo ============================================================
