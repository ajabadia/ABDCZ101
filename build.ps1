# CZ-5000 Emulator Build Script for Standalone Executable
# Combina la lógica de auto-detección del .bat con las capacidades de PowerShell.

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "CZ-5000 Emulator Build Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# --- 0. Limpiar Build Anterior --- 
$buildDir = "build"
Write-Host "`n0. Preparando para una compilación limpia..." -ForegroundColor Yellow
if (Test-Path $buildDir) {
    Write-Host "   Limpiando directorio de build anterior..." -ForegroundColor Gray
    Remove-Item -Path $buildDir -Recurse -Force
}
New-Item -ItemType Directory -Path $buildDir | Out-Null
Write-Host "   ✅ Directorio de build limpio y preparado." -ForegroundColor Green

# --- 1. Localizar CMake --- 
Write-Host "`n1. Buscando CMake..." -ForegroundColor Yellow

$cmakeExe = $null
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"

if (Test-Path $vswhere) {
    try {
        $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.CMake.Project -property installationPath
        $potentialCMake = Join-Path $vsPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
        if (Test-Path $potentialCMake) {
            $cmakeExe = $potentialCMake
            Write-Host "   ✅ CMake localizado en Visual Studio: $cmakeExe" -ForegroundColor Green
        }
    } catch {
        # vswhere puede fallar si no encuentra el componente
    }
}

if (-not $cmakeExe) {
    $cmakeExe = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmakeExe) {
        Write-Host "   ✅ CMake localizado en el PATH del sistema: $($cmakeExe.Source)" -ForegroundColor Green
    } else {
        Write-Host "   ❌ No se pudo encontrar CMake." -ForegroundColor Red
        Write-Host "   Asegúrate de que Visual Studio tenga instalado el componente 'Herramientas de CMake para C++' o que CMake esté en tu PATH." -ForegroundColor Yellow
        Read-Host "Presiona Enter para salir"
        exit 1
    }
}

# --- 2. Incrementar Versión de Build ---
Write-Host "`n2. Incrementando versión de build..." -ForegroundColor Yellow

$versionFile = "build_no.txt"
if (-not (Test-Path $versionFile)) { "0" | Out-File $versionFile }
[int]$buildNo = Get-Content $versionFile
$buildNo++
$buildNo | Out-File $versionFile

$headerPath = "Source\Core\BuildVersion.h"
$timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"

$headerContent = "#pragma once`n`n"
$headerContent += "#define CZ_BUILD_VERSION `"$buildNo`"`n"
$headerContent += "#define CZ_BUILD_TIMESTAMP `"$timestamp`"`n"

# Asegurarse de que el directorio Core exista
if (-not (Test-Path (Split-Path $headerPath))) {
    New-Item -ItemType Directory -Path (Split-Path $headerPath) | Out-Null
}

$headerContent | Out-File $headerPath -Encoding utf8
Write-Host "   ✅ Build #$buildNo. Fichero de versión generado en: $headerPath" -ForegroundColor Green

# --- 3. Configurar y Compilar ---
Push-Location $buildDir
try {
    Write-Host "`n3. Configurando proyecto con CMake..." -ForegroundColor Yellow
    & $cmakeExe -B . -A x64 ..
    if ($LASTEXITCODE -ne 0) { throw "La configuración de CMake ha fallado." }
    Write-Host "   ✅ Configuración completada." -ForegroundColor Green

    Write-Host "`n4. Compilando el ejecutable Standalone (Release)..." -ForegroundColor Yellow
    & $cmakeExe --build . --config Release --target CZ101Emulator_Standalone
    if ($LASTEXITCODE -ne 0) { throw "La compilación ha fallado." }
    Write-Host "   ✅ Compilación completada." -ForegroundColor Green
}
catch {
    Write-Host "`n   ❌ ERROR: $_" -ForegroundColor Red
    Pop-Location
    Read-Host "Presiona Enter para salir"
    exit 1
}
finally {
    # Pop-Location se ejecuta siempre, incluso si hay un error
}

# --- 4. Finalizar ---
Write-Host "`n5. Finalizando..." -ForegroundColor Yellow

# La ruta al artefacto puede variar ligeramente dependiendo de la versión de JUCE/CMake
$exeName = "CZ-101 Emulator.exe"
$exeSrcPath = "CZ101Emulator_artefacts\Release\Standalone\$exeName"

$finalExeName = "CZ-5000_Emulator.exe"
$destPath = "..\$finalExeName"

if (Test-Path $exeSrcPath) {
    Copy-Item -Path $exeSrcPath -Destination $destPath -Force
    Pop-Location
    Write-Host "`n========================================" -ForegroundColor Cyan
    Write-Host "  BUILD EXITOSA! (Build #$buildNo)" -ForegroundColor Green
    Write-Host "  Ejecutable: $finalExeName" -ForegroundColor White
    Write-Host "========================================`n" -ForegroundColor Cyan
    
    $response = Read-Host "¿Quieres ejecutarlo ahora? (S/N)"
    if ($response -eq 's' -or $response -eq 'S') {
        Start-Process $finalExeName
    }
} else {
    Pop-Location
    Write-Host "   ⚠️  ADVERTENCIA: No se encontró el .exe compilado en la ruta esperada." -ForegroundColor Yellow
    Write-Host "   Ruta buscada: `"build\$exeSrcPath`"" -ForegroundColor Gray
}

Read-Host "Presiona Enter para salir"
