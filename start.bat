@echo off
REM ============================================================
REM start.bat — Starts local development server for CZ-101 WebUI
REM ============================================================

echo ============================================================
echo  Starting Local Web Server on Port 8383
echo  Access URL: http://localhost:8383
echo ============================================================

npx sirv-cli WebUI --port 8383 --cors --single --dev
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Server stopped or failed to launch.
)
