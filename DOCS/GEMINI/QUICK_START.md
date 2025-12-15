# 🚀 QUICK START GUIDE - CZ-101 EMULATOR

**Tiempo de lectura:** 5 minutos  
**Objetivo:** Empezar desarrollo en 30 minutos

---

## 📖 PASO 1: LEER DOCUMENTACIÓN ESENCIAL (15 min)

### Orden de lectura:

1. **Este documento** (5 min) ← Estás aquí
2. **[EXECUTIVE_SUMMARY.md](EXECUTIVE_SUMMARY.md)** (10 min) ← Resumen completo

**Opcional pero recomendado:**
- [06_ADDITIONAL_NOTES.md](06_ADDITIONAL_NOTES.md) - Notas críticas

---

## ⚙️ PASO 2: INSTALAR HERRAMIENTAS (15 min)

### Requisitos Mínimos

```bash
# 1. JUCE Framework
# Descargar de: https://juce.com/get-juce
# Versión: 7.0.12 o superior

# 2. CMake
# Windows: winget install Kitware.CMake
# Versión: 3.21 o superior

# 3. Compilador C++17
# Windows: Visual Studio 2022 (Community Edition)
# macOS: Xcode 14+
# Linux: GCC 11+ o Clang 14+

# 4. GoogleTest (opcional, para tests)
# Se descarga automáticamente con CMake
```

### Verificar instalación

```bash
# Verificar CMake
cmake --version  # Debe ser >= 3.21

# Verificar compilador
# Windows (PowerShell)
cl  # Debe mostrar versión MSVC

# macOS/Linux
g++ --version  # Debe ser >= 11
clang++ --version  # Debe ser >= 14
```

---

## 🏗️ PASO 3: CREAR PROYECTO (10 min)

### A. Crear estructura de directorios

```bash
cd d:\desarrollos\ABDZ101

# Crear directorios principales
mkdir Source Tests Resources

# Crear subdirectorios de Source
mkdir Source\Core
mkdir Source\DSP
mkdir Source\DSP\Oscillators
mkdir Source\DSP\Envelopes
mkdir Source\DSP\Filters
mkdir Source\DSP\Effects
mkdir Source\DSP\Modulation
mkdir Source\MIDI
mkdir Source\State
mkdir Source\UI
mkdir Source\UI\Components
mkdir Source\UI\LookAndFeel
mkdir Source\Utils

# Crear subdirectorios de Tests
mkdir Tests\DSP
mkdir Tests\Core
mkdir Tests\MIDI
mkdir Tests\Integration

# Crear subdirectorios de Resources
mkdir Resources\Presets
mkdir Resources\Fonts
mkdir Resources\Images
```

### B. Crear CMakeLists.txt

Copiar el siguiente contenido a `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.21)

project(CZ101Emulator VERSION 1.0.0)

# C++17 mínimo
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Agregar JUCE (ajustar ruta según tu instalación)
add_subdirectory(JUCE)

# Crear plugin
juce_add_plugin(CZ101Emulator
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Cz01
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "CZ-101 Emulator"
)

# Source files (vacío por ahora)
target_sources(CZ101Emulator PRIVATE
    # Se agregarán en Milestone 0
)

# JUCE modules
target_link_libraries(CZ101Emulator PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
)

# Compile definitions
target_compile_definitions(CZ101Emulator PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)
```

### C. Configurar JUCE

```bash
# Opción 1: Clonar JUCE en el proyecto
git clone https://github.com/juce-framework/JUCE.git

# Opción 2: Usar JUCE instalado globalmente
# Modificar CMakeLists.txt para apuntar a tu instalación
```

---

## ✅ PASO 4: VERIFICAR SETUP (5 min)

```bash
# Crear directorio de build
mkdir build
cd build

# Configurar con CMake
cmake ..

# Si hay errores, verificar:
# - Ruta de JUCE correcta
# - Compilador instalado
# - CMake versión correcta
```

**Criterio de éxito:** CMake configura sin errores

---

## 🎯 PASO 5: SIGUIENTE ACCIÓN

**Ahora estás listo para:**

1. Abrir **[02_MILESTONES.md](02_MILESTONES.md)**
2. Ir a **MILESTONE 0: INFRAESTRUCTURA**
3. Seguir tareas del **Día 1**

---

## ⚠️ RECORDATORIOS CRÍTICOS

### 🔴 SIEMPRE Agregar en processBlock():

```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;  // ← CRÍTICO
    // ... resto del código
}
```

### 🟡 NUNCA Modificar Parámetros en Audio Thread:

```cpp
// ❌ INCORRECTO
void processBlock(...) {
    myParameter = newValue;
}

// ✅ CORRECTO
juce::AudioProcessorValueTreeState apvts;
void processBlock(...) {
    float value = apvts.getRawParameterValue("paramID")->load();
}
```

### 🟢 Mantener Archivos Pequeños:

- **Máximo:** 300 líneas por archivo
- **Ideal:** 150-200 líneas
- **Dividir** si excede

---

## 📚 DOCUMENTOS CLAVE

| Documento | Cuándo Leer |
|-----------|-------------|
| [EXECUTIVE_SUMMARY.md](EXECUTIVE_SUMMARY.md) | Ahora |
| [02_MILESTONES.md](02_MILESTONES.md) | Desarrollo diario |
| [06_ADDITIONAL_NOTES.md](06_ADDITIONAL_NOTES.md) | Cuando tengas dudas |
| [03_DSP_SPECS.md](03_DSP_SPECS.md) | Al implementar DSP |
| [04_UI_DESIGN.md](04_UI_DESIGN.md) | Al implementar UI |
| [05_TESTING.md](05_TESTING.md) | Al escribir tests |

---

## 🆘 TROUBLESHOOTING

### CMake no encuentra JUCE

```cmake
# Opción 1: Especificar ruta manualmente
set(JUCE_DIR "C:/JUCE")
add_subdirectory(${JUCE_DIR} JUCE)

# Opción 2: Usar FetchContent
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
)
FetchContent_MakeAvailable(JUCE)
```

### Compilador no encontrado

```bash
# Windows: Instalar Visual Studio 2022
# Descargar de: https://visualstudio.microsoft.com

# macOS: Instalar Xcode Command Line Tools
xcode-select --install

# Linux: Instalar GCC
sudo apt install build-essential  # Ubuntu/Debian
sudo dnf install gcc-c++          # Fedora
```

### Plugin no carga en DAW

1. Verificar que compiló sin errores
2. Verificar formato correcto (VST3/AU)
3. Verificar ruta de instalación del plugin
4. Reiniciar DAW

---

## ✅ CHECKLIST FINAL

Antes de continuar, verifica:

- [ ] He leído este Quick Start
- [ ] He leído EXECUTIVE_SUMMARY.md
- [ ] Tengo JUCE instalado
- [ ] Tengo CMake instalado
- [ ] Tengo compilador C++17
- [ ] He creado estructura de directorios
- [ ] He creado CMakeLists.txt
- [ ] CMake configura sin errores
- [ ] Sé qué hacer después (Milestone 0)

---

## 🎉 ¡LISTO PARA EMPEZAR!

**Siguiente paso:**

```bash
# Abrir Milestone 0
code DOCS/GEMINI/02_MILESTONES.md

# Buscar: "MILESTONE 0: INFRAESTRUCTURA"
# Seguir tareas del Día 1
```

**¡Buena suerte!** 🚀

---

**Última actualización:** 14 Diciembre 2025  
**Tiempo total de setup:** ~30 minutos  
**Próximo documento:** [02_MILESTONES.md](02_MILESTONES.md)
