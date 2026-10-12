@echo off
setlocal

rem ============================================================================
rem          ABDCZ101 - Compilacion Release (Control de flujo sin if-parentesis)
rem ============================================================================

echo =======================================================
echo          ABDCZ101 - Compilacion Release
echo =======================================================

:paso_1
echo [1/4] Generando registros y contratos...
if not exist "scripts\registry_generator.js" goto paso_2
node scripts\registry_generator.js
if errorlevel 1 goto error_exit

:paso_2
echo [2/5] Empaquetando WebUI...
if not exist "scripts\build_webui.js" goto paso_3
node scripts\build_webui.js
if errorlevel 1 goto error_exit

:paso_3
echo [3/5] Configurando CMake...
cmake -B build -S .
if not errorlevel 1 goto paso_4

echo [WARNING] Fallo en CMake. Limpiando cache y reintentando...
if exist "build\CMakeCache.txt" del /q "build\CMakeCache.txt"
if exist "build\CMakeFiles" rmdir /s /q "build\CMakeFiles"
cmake -B build -S .
if errorlevel 1 goto error_exit

:paso_4
echo [4/5] Compilando Standalone, VST3 y Tests Release...
taskkill /f /im CZ-5000_Emulator.exe >nul 2>&1
cmake --build build --config Release --parallel
if errorlevel 1 goto error_exit

:paso_5
echo [5/5] Compilando modulo WebAssembly WASM...
if defined SKIP_WASM goto final_ok
if not exist "wasm\build_wasm.bat" goto final_ok
pushd wasm
call build_wasm.bat
popd
if errorlevel 1 goto error_exit

:final_ok
echo =======================================================
echo  [EXITO] Compilacion de ABDCZ101 completada con exito!
echo =======================================================
pause
exit /b 0

:error_exit
echo.
echo =======================================================
echo  [ERROR] La compilacion se detuvo con errores.
echo =======================================================
pause
exit /b 1