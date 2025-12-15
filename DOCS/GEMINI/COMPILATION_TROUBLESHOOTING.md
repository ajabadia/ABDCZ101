# COMPILACIÓN - PROBLEMAS CONOCIDOS Y SOLUCIONES

**Basado en:** DeepMindSynth (proyecto con muchos problemas de compilación)  
**Fecha:** 14 Diciembre 2025  
**Importancia:** 🔴 CRÍTICA

---

## ⚠️ ADVERTENCIA

La compilación en el proyecto anterior **dio mucha guerra**. Este documento captura todos los problemas conocidos y sus soluciones.

**NO SUBESTIMAR:** Estos problemas son reales y pueden aparecer.

---

## 🔴 PROBLEMAS CRÍTICOS CONOCIDOS

### 1. "Ghost Build" / Exit Code 1 Sin Output

**Síntoma:**
```
Build Failed.
Exit code: 1
(Sin más información)
```

**Causas:**
1. Output redirigido a archivo (`> log.txt`) silencia errores
2. Proceso cuelga sin terminar
3. Includes duplicados en código generado

**Soluciones:**
```powershell
# ✅ NUNCA hacer esto:
cmake --build . > build.log 2>&1

# ✅ SIEMPRE hacer esto:
cmake --build . --verbose

# O usar nuestros scripts que usan Tee-Object
.\build.ps1
```

**Acción preventiva:**
- ✅ Ya implementado en `build.ps1` con `Tee-Object`
- ✅ `build.bat` NO redirige output (excepto en `_nopause`)

---

### 2. Includes Duplicados

**Síntoma:**
```
error C2011: 'ClassName': 'class' type redefinition
```

**Causa:**
- Código generado por JUCE puede tener `#include` duplicados
- Especialmente en `PluginEditor.cpp` y `PluginProcessor.cpp`

**Solución:**
```cpp
// Revisar archivos generados
// Buscar duplicados de:
#include "PluginProcessor.h"
#include "PluginEditor.h"

// Eliminar duplicados manualmente
```

**Acción preventiva:**
- ✅ Usar `#pragma once` en todos los headers (ya implementado)
- 📋 Revisar archivos después de primera compilación

---

### 3. Falta `<cstring>` para `memcpy`

**Síntoma:**
```
error C3861: 'memcpy': identifier not found
```

**Causa:**
- Falta incluir `<cstring>` en archivos que usan `memcpy`, `strcpy`, etc.

**Solución:**
```cpp
// Agregar al inicio del archivo
#include <cstring>  // Para memcpy, strcpy, etc.
```

**Archivos propensos:**
- `SysExHandler.cpp` (si usamos SysEx)
- Cualquier archivo que copie buffers

**Acción preventiva:**
- ✅ Documentado en `08_CODING_STANDARDS.md`
- 📋 Agregar `<cstring>` preventivamente donde sea necesario

---

### 4. Conflictos de Namespace

**Síntoma:**
```
error C2872: 'DSP': ambiguous symbol
```

**Causa:**
- Namespace `DSP` colisiona con `juce::dsp`

**Solución:**
```cpp
// ❌ EVITAR
namespace DSP {
    class MyClass { };
}

// ✅ USAR
namespace CZ101 {
namespace DSP {
    class MyClass { };
}
}
```

**Acción preventiva:**
- ✅ Ya definido en `08_CODING_STANDARDS.md`
- ✅ Usar `CZ101::DSP`, `CZ101::Core`, etc.

---

### 5. Dependencias Circulares

**Síntoma:**
```
error LNK2019: unresolved external symbol
error C2027: use of undefined type
```

**Causa:**
- Headers que se incluyen mutuamente
- Implementación pesada en headers

**Solución:**
```cpp
// En .h - Usar forward declarations
class Voice;  // Forward declaration

class VoiceManager {
    std::vector<std::unique_ptr<Voice>> voices;
};

// En .cpp - Incluir header completo
#include "Voice.h"
```

**Acción preventiva:**
- ✅ Ya aplicado en headers existentes
- ✅ Documentado en `08_CODING_STANDARDS.md`

---

### 6. CMake No Encuentra JUCE

**Síntoma:**
```
CMake Error: Could not find JUCE
```

**Causa:**
- JUCE no está en la ruta especificada
- `CMakeLists.txt` tiene ruta incorrecta

**Solución:**
```cmake
# Verificar en CMakeLists.txt línea 11:
set(JUCE_DIR "C:/JUCE")  # ← Verificar esta ruta

# Verificar que existe:
# C:\JUCE\CMakeLists.txt
```

**Acción preventiva:**
- ✅ Scripts verifican JUCE antes de compilar
- ✅ Error claro si no se encuentra

---

### 7. Visual Studio No Encontrado

**Síntoma:**
```
CMake Error: Could not find Visual Studio
```

**Causa:**
- VS2022 no instalado
- CMake no encuentra el generador

**Solución:**
```cmd
# Opción 1: Ejecutar desde Developer Command Prompt
# Buscar en menú inicio: "Developer Command Prompt for VS 2022"

# Opción 2: Inicializar entorno manualmente
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"

# Opción 3: Usar nuestro build.bat que lo hace automáticamente
build.bat
```

**Acción preventiva:**
- ✅ `build.bat` inicializa entorno VS2022 automáticamente
- ✅ Busca en Community, Professional, Enterprise

---

### 8. Primera Compilación Muy Lenta

**Síntoma:**
- Compilación tarda 10-15 minutos
- Parece que se colgó

**Causa:**
- JUCE se compila por primera vez
- Es NORMAL

**Solución:**
- ⏳ **ESPERAR** - No cancelar
- Compilaciones subsecuentes serán mucho más rápidas (1-2 min)

**Acción preventiva:**
- ✅ Scripts muestran mensaje: "Esto puede tardar varios minutos..."

---

### 9. Plugin No Carga en DAW

**Síntoma:**
- Plugin compila OK
- DAW no lo detecta

**Causas posibles:**
1. Plugin no está en carpeta correcta
2. DAW no escaneó carpeta
3. Plugin tiene errores de inicialización

**Solución:**
```powershell
# 1. Verificar que existe
Test-Path "build\CZ101Emulator_artefacts\Release\VST3\CZ-101 Emulator.vst3"

# 2. Copiar a carpeta de plugins
Copy-Item "build\CZ101Emulator_artefacts\Release\VST3\CZ-101 Emulator.vst3" `
          "$env:USERPROFILE\AppData\Local\Programs\Common\VST3\"

# 3. Re-escanear plugins en DAW
```

**Acción preventiva:**
- ✅ CMakeLists.txt tiene `COPY_PLUGIN_AFTER_BUILD TRUE`
- 📋 Verificar después de primera compilación

---

### 10. Errores de Linking

**Síntoma:**
```
error LNK2001: unresolved external symbol
```

**Causas:**
1. Función declarada pero no implementada
2. Archivo `.cpp` no agregado a `CMakeLists.txt`
3. Dependencia circular

**Solución:**
```cmake
# Verificar que TODOS los .cpp están en CMakeLists.txt
target_sources(CZ101Emulator PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    # ← Agregar nuevos archivos aquí
)
```

**Acción preventiva:**
- 📋 Actualizar `CMakeLists.txt` cada vez que agregues un `.cpp`
- 📋 Verificar que cada función declarada está implementada

---

## 🛠️ ESTRATEGIA DE DEBUGGING

### Si la Compilación Falla:

**Paso 1: Capturar Output Completo**
```powershell
# Compilar con verbose
cmake --build build --config Release --verbose > full_log.txt 2>&1
```

**Paso 2: Buscar Primer Error**
```powershell
# Buscar "error" en log
Select-String -Path full_log.txt -Pattern "error" | Select-Object -First 10
```

**Paso 3: Identificar Tipo de Error**
- **C2011** → Includes duplicados
- **C3861** → Falta include (`<cstring>`, `<cmath>`)
- **C2872** → Conflicto de namespace
- **LNK2019** → Función no implementada o archivo no en CMake

**Paso 4: Aplicar Solución**
- Consultar este documento
- Consultar `07_LESSONS_FROM_DEEPMIND.md`

---

## 📋 CHECKLIST PRE-COMPILACIÓN

Antes de intentar compilar por primera vez:

- [x] JUCE instalado en `C:\JUCE\`
- [x] Visual Studio 2022 instalado
- [x] CMake disponible (en PATH o VS)
- [x] `CMakeLists.txt` apunta a JUCE correcto
- [x] Todos los headers tienen `#pragma once`
- [x] Namespaces son `CZ101::*`
- [x] Includes estándar presentes
- [ ] Tiempo disponible (10-15 min primera vez)
- [ ] Paciencia preparada 😅

---

## 🚨 SI TODO FALLA

### Plan B: Build Limpio

```powershell
# 1. Eliminar build completamente
Remove-Item -Recurse -Force build

# 2. Limpiar caché de CMake
Remove-Item -Recurse -Force CMakeCache.txt, CMakeFiles

# 3. Build desde cero
.\build_clean.bat
```

### Plan C: Verificar Instalaciones

```powershell
# Verificar Visual Studio
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
cl  # Debe mostrar versión del compilador

# Verificar CMake
cmake --version  # Debe ser >= 3.21

# Verificar JUCE
Test-Path "C:\JUCE\CMakeLists.txt"  # Debe ser True
```

### Plan D: Compilación Mínima

Si nada funciona, probar con proyecto JUCE mínimo:
1. Comentar todo el código en `PluginProcessor.cpp`
2. Dejar solo estructura básica
3. Compilar
4. Ir agregando código gradualmente

---

## 📝 LOG DE PROBLEMAS

### Formato para Documentar Nuevos Problemas

```markdown
### [Número]. [Nombre del Problema]

**Síntoma:**
[Mensaje de error exacto]

**Causa:**
[Por qué ocurre]

**Solución:**
[Cómo se resolvió]

**Acción preventiva:**
[Cómo evitarlo en el futuro]
```

---

## 🎯 EXPECTATIVAS REALISTAS

### Primera Compilación:
- ⏳ **Tiempo:** 10-15 minutos
- 🎲 **Probabilidad de éxito:** 60-70%
- 🐛 **Errores esperados:** 2-5

### Segunda Compilación (después de fixes):
- ⏳ **Tiempo:** 1-2 minutos
- 🎲 **Probabilidad de éxito:** 90%
- 🐛 **Errores esperados:** 0-1

### Compilaciones Subsecuentes:
- ⏳ **Tiempo:** 30 segundos - 1 minuto
- 🎲 **Probabilidad de éxito:** 95%+
- 🐛 **Errores esperados:** 0

---

## 💡 CONSEJOS FINALES

1. **No desesperar** - Es normal tener problemas la primera vez
2. **Leer errores completos** - No solo la primera línea
3. **Compilar con verbose** - Más información = más fácil debug
4. **Buscar en este documento** - Probablemente ya está documentado
5. **Documentar nuevos problemas** - Para la próxima vez

---

**Última actualización:** 14 Diciembre 2025  
**Basado en:** Experiencia real de DeepMindSynth  
**Importancia:** 🔴 CRÍTICA - Leer antes de compilar
