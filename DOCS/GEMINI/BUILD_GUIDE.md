# BUILD SCRIPTS - GUÍA DE USO

**Versión:** 2.0  
**Fecha:** 14 Diciembre 2025  
**Basado en:** DeepMindSynth build scripts

---

## 📋 SCRIPTS DISPONIBLES

### 1. build.ps1 (PowerShell) - RECOMENDADO

**Características:**
- ✅ Output colorizado
- ✅ Detección automática de CMake
- ✅ Verificación de JUCE
- ✅ Manejo de errores mejorado
- ✅ Log de errores automático
- ✅ Opción de limpiar build

**Uso:**
```powershell
.\build.ps1
```

---

### 2. build.bat (Batch) - COMPATIBILIDAD

**Características:**
- ✅ Compatible con cmd.exe
- ✅ Inicializa entorno VS2022
- ✅ Detección automática de CMake
- ✅ Log de errores

**Uso:**
```cmd
build.bat
```

---

## 🚀 COMPILACIÓN PASO A PASO

### Opción A: Automática (Recomendado)

```powershell
# PowerShell
cd d:\desarrollos\ABDZ101
.\build.ps1

# O Batch
build.bat
```

### Opción B: Manual

```powershell
# 1. Crear directorio build
mkdir build
cd build

# 2. Configurar
cmake .. -G "Visual Studio 17 2022" -A x64

# 3. Compilar
cmake --build . --config Release --target CZ101Emulator_Standalone

# 4. Ejecutable estará en:
# build\CZ101Emulator_artefacts\Release\Standalone\CZ-101 Emulator.exe
```

---

## 🔍 DETECCIÓN AUTOMÁTICA DE CMAKE

Los scripts buscan CMake en este orden:

1. **PATH** (si está agregado)
2. **Visual Studio 2022 Community**
   - `C:\Program Files\Microsoft Visual Studio\2022\Community\...\cmake.exe`
3. **Visual Studio 2022 Professional**
   - `C:\Program Files\Microsoft Visual Studio\2022\Professional\...\cmake.exe`
4. **Visual Studio 2022 Enterprise**
   - `C:\Program Files\Microsoft Visual Studio\2022\Enterprise\...\cmake.exe`
5. **CMake Standalone**
   - `C:\Program Files\CMake\bin\cmake.exe`

---

## ⚙️ CONFIGURACIÓN

### Variables de Entorno (Opcional)

Si CMake no se encuentra automáticamente:

```powershell
# Agregar CMake al PATH
$env:Path += ";C:\Program Files\CMake\bin"

# O especificar JUCE_DIR (si no está en C:\JUCE)
$env:JUCE_DIR = "D:\MiJUCE"
```

---

## 📦 SALIDA DE LA COMPILACIÓN

### Archivos Generados

```
build/
└── CZ101Emulator_artefacts/
    └── Release/
        ├── Standalone/
        │   └── CZ-101 Emulator.exe    ← Aplicación standalone
        └── VST3/
            └── CZ-101 Emulator.vst3   ← Plugin VST3
```

### Copia Automática

El script copia automáticamente:
- `CZ-101 Emulator.exe` → `CZ101Emulator.exe` (raíz del proyecto)

---

## 🐛 TROUBLESHOOTING

### Error: CMake no encontrado

**Solución 1:** Ejecutar desde Developer Command Prompt
```cmd
# Buscar en menú inicio:
Developer Command Prompt for VS 2022
```

**Solución 2:** Instalar CMake standalone
- Descargar de: https://cmake.org/download/
- Agregar al PATH durante instalación

---

### Error: JUCE no encontrado

**Causa:** JUCE no está en `C:\JUCE\`

**Solución:**
1. Verificar que existe `C:\JUCE\CMakeLists.txt`
2. O editar `CMakeLists.txt` línea 11:
   ```cmake
   set(JUCE_DIR "C:/JUCE")  # Cambiar ruta aquí
   ```

---

### Error: Visual Studio no encontrado

**Causa:** VS2022 no instalado o ruta incorrecta

**Solución:**
1. Instalar Visual Studio 2022 (Community es gratis)
2. Incluir "Desktop development with C++"
3. O usar CMake con otro generador:
   ```powershell
   cmake .. -G "Ninja"  # Requiere Ninja instalado
   ```

---

### Error de compilación

**Revisar log:**
```powershell
# El script guarda errores en:
type compilation_error.log
```

**Compilar con verbose:**
```powershell
cmake --build build --config Release --verbose
```

---

## 🎯 TARGETS DISPONIBLES

### Compilar solo Standalone

```powershell
cmake --build build --config Release --target CZ101Emulator_Standalone
```

### Compilar solo VST3

```powershell
cmake --build build --config Release --target CZ101Emulator_VST3
```

### Compilar todo

```powershell
cmake --build build --config Release
```

---

## 🔧 CONFIGURACIONES

### Debug vs Release

**Release (por defecto):**
- Optimizado
- Sin símbolos de debug
- Más rápido

```powershell
cmake --build build --config Release
```

**Debug:**
- Con símbolos
- Sin optimizaciones
- Para debugging

```powershell
cmake --build build --config Debug
```

---

## 📝 NOTAS IMPORTANTES

### Primera Compilación

- Puede tardar 5-10 minutos
- CMake descarga dependencias
- JUCE se compila por primera vez

### Compilaciones Subsecuentes

- Mucho más rápidas (1-2 minutos)
- Solo recompila archivos modificados

### Limpiar Build

**PowerShell:**
```powershell
Remove-Item -Recurse -Force build
```

**Batch:**
```cmd
rmdir /s /q build
```

---

## ✅ VERIFICACIÓN POST-COMPILACIÓN

### 1. Verificar ejecutable

```powershell
# Debe existir
Test-Path "CZ101Emulator.exe"
```

### 2. Ejecutar standalone

```powershell
.\CZ101Emulator.exe
```

### 3. Verificar VST3

```powershell
# Debe existir
Test-Path "build\CZ101Emulator_artefacts\Release\VST3\CZ-101 Emulator.vst3"
```

### 4. Instalar VST3 (opcional)

```powershell
# Copiar a carpeta de plugins
Copy-Item "build\CZ101Emulator_artefacts\Release\VST3\CZ-101 Emulator.vst3" `
          "$env:USERPROFILE\AppData\Local\Programs\Common\VST3\"
```

---

## 🎉 EJEMPLO DE SALIDA EXITOSA

```
========================================
CZ-101 Emulator Build Script
========================================

1. Buscando CMake...
   ✅ CMake encontrado: C:\Program Files\CMake\bin\cmake.exe

2. Verificando JUCE...
   ✅ JUCE encontrado en C:\JUCE\

3. Verificando Git...
   ✅ git version 2.43.0

4. Preparando directorio de build...
   ✅ Directorio build creado

5. Configurando proyecto con CMake...
   ✅ Configuración completada

6. Compilando proyecto (Release)...
   ✅ Compilación exitosa!

7. Copiando ejecutable...
   ✅ Ejecutable copiado a: CZ101Emulator.exe

========================================
BUILD COMPLETADO EXITOSAMENTE
========================================

Archivos generados:
  - Standalone: CZ101Emulator.exe
  - VST3: build\CZ101Emulator_artefacts\Release\VST3\
```

---

**Última actualización:** 14 Diciembre 2025  
**Versión:** 2.0  
**Basado en:** DeepMindSynth build scripts
