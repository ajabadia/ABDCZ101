# CZ-101 Emulator Build Script
# Basado en DeepMindSynth build script con mejoras

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "CZ-101 Emulator Build Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# 1. Buscar CMake
Write-Host "`n1. Buscando CMake..." -ForegroundColor Yellow

$cmakePaths = @(
    # PATH primero
    (Get-Command cmake -ErrorAction SilentlyContinue).Source,
    # Visual Studio 2022
    "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
    # CMake standalone
    "C:\Program Files\CMake\bin\cmake.exe",
    "C:\Program Files (x86)\CMake\bin\cmake.exe"
)

$cmakeExe = $null
foreach ($path in $cmakePaths) {
    if ($path -and (Test-Path $path)) {
        $cmakeExe = $path
        Write-Host "   ✅ CMake encontrado: $path" -ForegroundColor Green
        break
    }
}

if (-not $cmakeExe) {
    Write-Host "   ❌ CMake no encontrado" -ForegroundColor Red
    Write-Host "   Instala CMake desde: https://cmake.org/download/" -ForegroundColor Yellow
    Write-Host "   O ejecuta desde 'Developer Command Prompt for VS 2022'" -ForegroundColor Yellow
    exit 1
}

# 2. Verificar JUCE
Write-Host "`n2. Verificando JUCE..." -ForegroundColor Yellow

if (-not (Test-Path "C:\JUCE\CMakeLists.txt")) {
    Write-Host "   ❌ JUCE no encontrado en C:\JUCE\" -ForegroundColor Red
    Write-Host "   Verifica la instalación de JUCE" -ForegroundColor Yellow
    exit 1
}
Write-Host "   ✅ JUCE encontrado en C:\JUCE\" -ForegroundColor Green

# 3. Verificar Git
Write-Host "`n3. Verificando Git..." -ForegroundColor Yellow

try {
    $gitVersion = git --version 2>&1
    Write-Host "   ✅ $gitVersion" -ForegroundColor Green
}
catch {
    Write-Host "   ⚠️  Git no encontrado (opcional)" -ForegroundColor Yellow
}

# 4. Limpiar build anterior (opcional)
Write-Host "`n4. Preparando directorio de build..." -ForegroundColor Yellow

if (Test-Path "build") {
    $response = Read-Host "   ¿Limpiar build anterior? (s/N)"
    if ($response -eq 's' -or $response -eq 'S') {
        Write-Host "   Limpiando build..." -ForegroundColor Yellow
        Remove-Item -Path "build" -Recurse -Force -ErrorAction SilentlyContinue
    }
}

if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
    Write-Host "   ✅ Directorio build creado" -ForegroundColor Green
}

# 5. Configurar proyecto
Write-Host "`n5. Configurando proyecto con CMake..." -ForegroundColor Yellow
Write-Host "   (Esto puede tardar la primera vez...)" -ForegroundColor Gray

Push-Location build
try {
    # Configurar con Visual Studio 2022
    $configArgs = @(
        "..",
        "-G", "Visual Studio 17 2022",
        "-A", "x64"
    )
    
    Write-Host "   Ejecutando: cmake $($configArgs -join ' ')" -ForegroundColor Gray
    
    & $cmakeExe $configArgs 2>&1 | Tee-Object -Variable configOutput | ForEach-Object {
        if ($_ -match "error|fatal") {
            Write-Host "   $_" -ForegroundColor Red
        }
        elseif ($_ -match "warning") {
            Write-Host "   $_" -ForegroundColor Yellow
        }
        else {
            Write-Host "   $_" -ForegroundColor Gray
        }
    }
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "`n   ❌ Error en configuración de CMake" -ForegroundColor Red
        Write-Host "   Revisa los errores arriba" -ForegroundColor Yellow
        exit 1
    }
    
    Write-Host "`n   ✅ Configuración completada" -ForegroundColor Green
    
    # 6. Compilar proyecto
    Write-Host "`n6. Compilando proyecto (Release)..." -ForegroundColor Yellow
    Write-Host "   (Esto puede tardar varios minutos...)" -ForegroundColor Gray
    
    $buildArgs = @(
        "--build", ".",
        "--config", "Release",
        "--target", "CZ101Emulator_Standalone"
    )
    
    Write-Host "   Ejecutando: cmake $($buildArgs -join ' ')" -ForegroundColor Gray
    
    & $cmakeExe $buildArgs 2>&1 | Tee-Object -Variable buildOutput | ForEach-Object {
        if ($_ -match "error|fatal|failed") {
            Write-Host "   $_" -ForegroundColor Red
        }
        elseif ($_ -match "warning") {
            Write-Host "   $_" -ForegroundColor Yellow
        }
        elseif ($_ -match "Building|Compiling|Linking") {
            Write-Host "   $_" -ForegroundColor Cyan
        }
        else {
            # Silenciar output verbose
        }
    }
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "`n   ❌ Error en compilación" -ForegroundColor Red
        Write-Host "   Revisa los errores arriba" -ForegroundColor Yellow
        
        # Guardar log de errores
        $buildOutput | Out-File -FilePath "..\compilation_error.log"
        Write-Host "   Log guardado en: compilation_error.log" -ForegroundColor Yellow
        exit 1
    }
    
    Write-Host "`n   ✅ Compilación exitosa!" -ForegroundColor Green
    
    # 7. Copiar ejecutable
    Write-Host "`n7. Copiando ejecutable..." -ForegroundColor Yellow
    
    $exePath = "CZ101Emulator_artefacts\Release\Standalone\CZ-101 Emulator.exe"
    $destPath = "..\CZ101Emulator.exe"
    
    if (Test-Path $exePath) {
        Copy-Item -Path $exePath -Destination $destPath -Force
        Write-Host "   ✅ Ejecutable copiado a: CZ101Emulator.exe" -ForegroundColor Green
    }
    else {
        Write-Host "   ⚠️  Ejecutable no encontrado en ruta esperada" -ForegroundColor Yellow
        Write-Host "   Busca en: build\CZ101Emulator_artefacts\" -ForegroundColor Gray
    }
    
    # 8. Mostrar plugins generados
    Write-Host "`n8. Plugins generados:" -ForegroundColor Yellow
    
    $vst3Path = "CZ101Emulator_artefacts\Release\VST3\CZ-101 Emulator.vst3"
    if (Test-Path $vst3Path) {
        Write-Host "   ✅ VST3: $vst3Path" -ForegroundColor Green
    }
    
    Write-Host "`n========================================" -ForegroundColor Cyan
    Write-Host "BUILD COMPLETADO EXITOSAMENTE" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Cyan
    
    Write-Host "`nArchivos generados:" -ForegroundColor Cyan
    Write-Host "  - Standalone: CZ101Emulator.exe" -ForegroundColor White
    Write-Host "  - VST3: build\CZ101Emulator_artefacts\Release\VST3\" -ForegroundColor White
    
}
catch {
    Write-Host "`n❌ Error inesperado: $_" -ForegroundColor Red
    exit 1
}
finally {
    Pop-Location
}

Write-Host "`nPresiona Enter para salir..." -ForegroundColor Gray
Read-Host
