# CONFIGURACIÓN DEL SISTEMA - NOTAS

**Fecha:** 14 Diciembre 2025

---

## 🔧 RUTAS IMPORTANTES

### JUCE
- **Ubicación:** `C:\JUCE\`
- **Estado:** ✅ Instalado localmente
- **Nota:** CMakeLists.txt configurado para usar esta ruta

### CMake
- **Estado:** ⚠️ No está en PATH
- **Ubicación probable:** 
  - `C:\Program Files\CMake\bin\cmake.exe`
  - `C:\Program Files (x86)\CMake\bin\cmake.exe`
- **Solución:** Buscar y usar ruta completa

---

## 🚀 CÓMO COMPILAR

### Opción 1: Buscar CMake manualmente

```powershell
# Buscar CMake en el sistema
Get-ChildItem -Path "C:\Program Files" -Recurse -Filter "cmake.exe" -ErrorAction SilentlyContinue | Select-Object FullName

# O buscar en Program Files (x86)
Get-ChildItem -Path "C:\Program Files (x86)" -Recurse -Filter "cmake.exe" -ErrorAction SilentlyContinue | Select-Object FullName
```

### Opción 2: Usar ruta completa de CMake

```powershell
# Ejemplo (ajustar según tu instalación)
cd d:\desarrollos\ABDZ101
mkdir build
cd build

# Usar ruta completa
& "C:\Program Files\CMake\bin\cmake.exe" ..
& "C:\Program Files\CMake\bin\cmake.exe" --build .
```

### Opción 3: Agregar CMake al PATH (permanente)

```powershell
# Agregar CMake al PATH del usuario
$cmakePath = "C:\Program Files\CMake\bin"  # Ajustar según tu instalación
[Environment]::SetEnvironmentVariable("Path", $env:Path + ";$cmakePath", "User")

# Reiniciar PowerShell después
```

---

## 📝 PROBLEMAS CONOCIDOS

### Problema 1: CMake no encontrado
**Síntoma:** `cmake: command not found`  
**Solución:** Usar ruta completa o agregar al PATH

### Problema 2: JUCE no encontrado
**Síntoma:** `JUCE not found at C:/JUCE`  
**Solución:** Verificar que existe `C:\JUCE\CMakeLists.txt`

### Problema 3: Compilador no encontrado
**Síntoma:** `No CMAKE_CXX_COMPILER could be found`  
**Solución:** Instalar Visual Studio 2022 con C++ workload

---

## ✅ VERIFICACIÓN RÁPIDA

```powershell
# 1. Verificar JUCE
Test-Path "C:\JUCE\CMakeLists.txt"  # Debe devolver True

# 2. Buscar CMake
where.exe cmake  # Si está en PATH
# O buscar manualmente

# 3. Verificar Visual Studio
& "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
```

---

## 🎯 PRÓXIMO PASO

1. Buscar CMake en tu sistema
2. Anotar la ruta completa
3. Intentar compilar usando esa ruta

**Comando para buscar CMake:**
```powershell
Get-ChildItem -Path "C:\" -Recurse -Filter "cmake.exe" -ErrorAction SilentlyContinue | Select-Object FullName
```
