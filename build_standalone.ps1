$ErrorActionPreference = "Stop"
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.CMake.Project -property installationPath
$cmakeExe = Join-Path $vsPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

Write-Host "Re-Configuring..."
& $cmakeExe -B build -S . -DENABLE_SANITIZERS=OFF -A x64

Write-Host "Building CZ101Emulator (Release)..."
& $cmakeExe --build build --config Release --target CZ101Emulator
