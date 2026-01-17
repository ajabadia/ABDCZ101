$ErrorActionPreference = "Stop"
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.CMake.Project -property installationPath
$msbuild = Join-Path $vsPath "MSBuild\Current\Bin\MSBuild.exe"

if (-not (Test-Path $msbuild)) {
    $msbuild = Join-Path $vsPath "MSBuild\15.0\Bin\MSBuild.exe"
}

Write-Host "Building CZ101GoldenMaster.vcxproj directly..."
& $msbuild "build\CZ101GoldenMaster.vcxproj" /p:Configuration=Release /t:Rebuild /v:detailed | Out-File -Encoding utf8 build_log.txt
Write-Host "Build complete. Log saved to build_log.txt"
