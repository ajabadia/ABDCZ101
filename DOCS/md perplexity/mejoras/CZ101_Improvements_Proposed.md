# 🔧 CZ101 v2.1 - Propuestas de Mejoras & Ajustes

**Audit Date:** December 15, 2025, 22:00 CET  
**Current Score:** 9.4/10  
**Status:** Production Ready (Con 3 ajustes menores)

---

## 📋 Problemas Identificados & Soluciones

### 1. ⚠️ MEDIA PRIORIDAD: Hardcode 44100.0 en updateXXXEnvelopeFromADSR()

**Localización:** `Voice.cpp` - Líneas en `updateDCWEnvelopeFromADSR()`, `updateDCAEnvelopeFromADSR()`, `updatePitchEnvelopeFromADSR()`

**Problema:**
```cpp
// ❌ ACTUAL (INCORRECTO)
DSPADSRtoStageConverter::convertADSR(
    dcwADSR.attackMs,
    dcwADSR.decayMs,
    dcwADSR.sustainLevel,
    dcwADSR.releaseMs,
    rates, levels, sus, end,
    44100.0  // ← HARDCODEADO - Incorrecto en 96kHz/192kHz
);
```

**Impacto:** 
- A 96kHz: Envelopes 2x más lentos (tiempos duplicados)
- A 192kHz: Envelopes 4x más lentos
- Attack/Release times completamente incorrectos en estudio profesional

**Solución Propuesta:**

Agregar miembro privado `sampleRate` a Voice y usarlo:

```cpp
// ✅ EN Voice.h (private section)
private:
    double sampleRate = 44100.0;  // ← Agregar esto

// ✅ EN Voice.cpp - updateDCWEnvelopeFromADSR()
void Voice::updateDCWEnvelopeFromADSR() noexcept
{
    std::array<float, 8> rates, levels;
    int sus, end;
    
    DSP::ADSRtoStageConverter::convertADSR(
        dcwADSR.attackMs,
        dcwADSR.decayMs,
        dcwADSR.sustainLevel,
        dcwADSR.releaseMs,
        rates, levels, sus, end,
        sampleRate  // ← USAR THIS->SAMPLERATE
    );
    
    for (int i = 0; i < 4; ++i) {
        dcwEnvelope.setStage(i, rates[i], levels[i]);
    }
    dcwEnvelope.setSustainPoint(sus);
    dcwEnvelope.setEndPoint(end);
}

// ✅ EN Voice.cpp - setSampleRate()
void Voice::setSampleRate(double sr) noexcept
{
    sampleRate = sr;  // ← ALMACENAR AQUÍ
    osc1.setSampleRate(sr);
    osc2.setSampleRate(sr);
    dcwEnvelope.setSampleRate(sr);
    dcaEnvelope.setSampleRate(sr);
    pitchEnvelope.setSampleRate(sr);
}
```

**Esfuerzo:** 5 minutos  
**Riesgo:** Muy bajo - solo cambiar parámetro  
**Prioridad:** ALTA - Fix before production

---

### 2. ⚠️ MEDIA PRIORIDAD: JSON no serializa 8-stage envelope data

**Localización:** `PresetManager.cpp` - Métodos `saveBankToFile()` y `loadBankFromFile()`

**Problema:**
```cpp
// ❌ ACTUAL - Solo guarda parámetros básicos
juceDynamicObject obj = new juceDynamicObject;
obj->setProperty("name", juceString(preset.name));
juceDynamicObject paramsObj = new juceDynamicObject;
for (const auto& [id, val] : preset.parameters) {
    paramsObj->setProperty(juceIdentifier(id), val);
}
obj->setProperty("params", paramsObj);
// ← Falta serializar dcwEnv, dcaEnv, pitchEnv
```

**Impacto:**
- Presets guardados con envelopes editados se pierden
- SysEx load funciona pero no persiste en banco
- Usuario customiza envelopes, guarda preset, recarga y ha desaparecido

**Solución Propuesta:**

```cpp
// ✅ EN PresetManager.cpp - saveBankToFile()
void PresetManager::saveBankToFile(const juceFile& file)
{
    juceArray<juceVar> bankArray;
    
    for (const auto& preset : presets) {
        juceDynamicObject obj = new juceDynamicObject;
        
        // Name & params (existing)
        obj->setProperty("name", juceString(preset.name));
        juceDynamicObject paramsObj = new juceDynamicObject;
        for (const auto& [id, val] : preset.parameters) {
            paramsObj->setProperty(juceIdentifier(id), val);
        }
        obj->setProperty("params", paramsObj);
        
        // ✅ AGREGAR: Serialize 8-stage envelopes
        // DCW Envelope
        juceDynamicObject dcwObj = new juceDynamicObject;
        {
            juceArray<juceVar> ratesArray;
            for (int i = 0; i < 8; ++i) {
                ratesArray.append(preset.dcwEnv.rates[i]);
            }
            juceArray<juceVar> levelsArray;
            for (int i = 0; i < 8; ++i) {
                levelsArray.append(preset.dcwEnv.levels[i]);
            }
            dcwObj->setProperty("rates", ratesArray);
            dcwObj->setProperty("levels", levelsArray);
            dcwObj->setProperty("sustainPoint", preset.dcwEnv.sustainPoint);
            dcwObj->setProperty("endPoint", preset.dcwEnv.endPoint);
        }
        obj->setProperty("dcwEnv", dcwObj);
        
        // DCA Envelope (same pattern)
        juceDynamicObject dcaObj = new juceDynamicObject;
        {
            juceArray<juceVar> ratesArray;
            for (int i = 0; i < 8; ++i) {
                ratesArray.append(preset.dcaEnv.rates[i]);
            }
            juceArray<juceVar> levelsArray;
            for (int i = 0; i < 8; ++i) {
                levelsArray.append(preset.dcaEnv.levels[i]);
            }
            dcaObj->setProperty("rates", ratesArray);
            dcaObj->setProperty("levels", levelsArray);
            dcaObj->setProperty("sustainPoint", preset.dcaEnv.sustainPoint);
            dcaObj->setProperty("endPoint", preset.dcaEnv.endPoint);
        }
        obj->setProperty("dcaEnv", dcaObj);
        
        // Pitch Envelope (same pattern)
        juceDynamicObject pitchObj = new juceDynamicObject;
        {
            juceArray<juceVar> ratesArray;
            for (int i = 0; i < 8; ++i) {
                ratesArray.append(preset.pitchEnv.rates[i]);
            }
            juceArray<juceVar> levelsArray;
            for (int i = 0; i < 8; ++i) {
                levelsArray.append(preset.pitchEnv.levels[i]);
            }
            pitchObj->setProperty("rates", ratesArray);
            pitchObj->setProperty("levels", levelsArray);
            pitchObj->setProperty("sustainPoint", preset.pitchEnv.sustainPoint);
            pitchObj->setProperty("endPoint", preset.pitchEnv.endPoint);
        }
        obj->setProperty("pitchEnv", pitchObj);
        
        bankArray.append(obj);
    }
    
    juceString jsonString = juceJSON::toString(bankArray);
    file.replaceWithText(jsonString);
}

// ✅ EN PresetManager.cpp - loadBankFromFile()
void PresetManager::loadBankFromFile(const juceFile& file)
{
    juceString jsonString = file.loadFileAsString();
    juceVar parsedJson = juceJSON::parse(jsonString);
    
    if (!parsedJson.isArray()) return;
    
    presets.clear();
    
    for (int i = 0; i < parsedJson.size(); ++i) {
        Preset p;
        juceVar obj = parsedJson[i];
        
        // Load name & params
        p.name = obj["name"].toString().toStdString();
        juceVar paramsObj = obj["params"];
        if (paramsObj.isObject()) {
            for (int j = 0; j < paramsObj.size(); ++j) {
                juceIdentifier key = paramsObj.getProperties().getName(j);
                p.parameters[key.toString().toStdString()] = 
                    (float)paramsObj[key];
            }
        }
        
        // ✅ AGREGAR: Load 8-stage envelopes
        if (obj.hasProperty("dcwEnv")) {
            juceVar dcwObj = obj["dcwEnv"];
            juceArray<juceVar> rates = dcwObj["rates"];
            juceArray<juceVar> levels = dcwObj["levels"];
            for (int j = 0; j < 8; ++j) {
                p.dcwEnv.rates[j] = (float)rates[j];
                p.dcwEnv.levels[j] = (float)levels[j];
            }
            p.dcwEnv.sustainPoint = (int)dcwObj["sustainPoint"];
            p.dcwEnv.endPoint = (int)dcwObj["endPoint"];
        }
        
        // Load DCA Envelope (same pattern)
        if (obj.hasProperty("dcaEnv")) {
            juceVar dcaObj = obj["dcaEnv"];
            juceArray<juceVar> rates = dcaObj["rates"];
            juceArray<juceVar> levels = dcaObj["levels"];
            for (int j = 0; j < 8; ++j) {
                p.dcaEnv.rates[j] = (float)rates[j];
                p.dcaEnv.levels[j] = (float)levels[j];
            }
            p.dcaEnv.sustainPoint = (int)dcaObj["sustainPoint"];
            p.dcaEnv.endPoint = (int)dcaObj["endPoint"];
        }
        
        // Load Pitch Envelope (same pattern)
        if (obj.hasProperty("pitchEnv")) {
            juceVar pitchObj = obj["pitchEnv"];
            juceArray<juceVar> rates = pitchObj["rates"];
            juceArray<juceVar> levels = pitchObj["levels"];
            for (int j = 0; j < 8; ++j) {
                p.pitchEnv.rates[j] = (float)rates[j];
                p.pitchEnv.levels[j] = (float)levels[j];
            }
            p.pitchEnv.sustainPoint = (int)pitchObj["sustainPoint"];
            p.pitchEnv.endPoint = (int)pitchObj["endPoint"];
        }
        
        presets.push_back(p);
    }
    
    if (!presets.empty()) {
        currentPreset = presets[0];
        applyPresetToProcessor();
    }
}
```

**Esfuerzo:** 30-45 minutos (boilerplate JSON)  
**Riesgo:** Bajo - versión anterior simplemente no guarda, nueva es additive  
**Prioridad:** ALTA - Fix before release

---

### 3. ℹ️ BAJA PRIORIDAD: Rate editing incompleto en EnvelopeEditor

**Localización:** `EnvelopeEditor.cpp` - `mouseDrag()` handler

**Problema:**
```cpp
// ⚠️ ACTUAL - Solo edita Level (Y), Rate editing mencionado pero no implementado
void EnvelopeEditor::mouseDrag(const juceMouseEvent& e)
{
    if (selectedStage < 0 || selectedStage >= 8) return;
    
    float y = juce::jlimit(e.position.y, 0.0f, (float)getHeight());
    levels[selectedStage] = 1.0f - (y / getHeight());  // ← Solo level
    
    // ⚠️ Rate editing "hard to map X to rate if X is also stage index"
    // Comentario sugiere que dejó pendiente
    
    sendUpdateToProcessor(selectedStage);
    repaint();
}
```

**Impacto:**
- Usuario solo puede editar niveles (vertical drag)
- No puede ajustar rates (velocidad de transición)
- Solo funciona 50% de la edición de envelopes

**Solución Propuesta:**

Agregar "shift+drag" para rate:

```cpp
// ✅ EN EnvelopeEditor.cpp
void EnvelopeEditor::mouseDrag(const juceMouseEvent& e)
{
    if (selectedStage < 0 || selectedStage >= 8) return;
    
    if (e.mods.isShiftDown()) {
        // ✅ SHIFT+DRAG = Rate (horizontal)
        // X range: 0 to stepWidth
        // Normalize to 0-1 for rate
        float stepWidth = getWidth() / 8.0f;
        float stageStartX = selectedStage * stepWidth;
        float xWithinStage = juce::jlimit(0.0f, stepWidth, 
                                          e.position.x - stageStartX);
        float newRate = xWithinStage / stepWidth;  // 0..1
        
        rates[selectedStage] = newRate;
        
    } else {
        // ✅ NORMAL DRAG = Level (vertical) - existing code
        float y = juce::jlimit(0.0f, (float)getHeight(), e.position.y);
        levels[selectedStage] = 1.0f - (y / getHeight());
    }
    
    sendUpdateToProcessor(selectedStage);
    repaint();
}

// ✅ AGREGAR: Paint hint text
void EnvelopeEditor::paint(juceGraphics& g)
{
    // ... existing paint code ...
    
    // Add help text at bottom
    g.setColour(juceColours::white.withAlpha(0.3f));
    g.setFont(10.0f);
    g.drawText("Drag: Level | Shift+Drag: Rate", 
               getLocalBounds().reduced(5).withHeight(15),
               juceJustification::centred, false);
}
```

**Esfuerzo:** 15-20 minutos  
**Riesgo:** Muy bajo - feature completo  
**Prioridad:** MEDIA - Nice to have, pero no crítico

---

### 4. ℹ️ BAJA PRIORIDAD: Precisión float en setDCWAttack()

**Localización:** `Voice.cpp` - `setDCWAttack()` y similares

**Problema:**
```cpp
// ⚠️ ACTUAL - Float * 1000.0f puede perder precisión en valores pequeños
void Voice::setDCWAttack(float seconds) noexcept
{
    dcwADSR.attackMs = std::clamp(seconds * 1000.0f, 0.5f, 8000.0f);
    //                                  ^^^^^^^^ 
    // Valores muy pequeños (0.0001s) pueden perder precisión
    updateDCWEnvelopeFromADSR();
}
```

**Impacto:**
- Mínimo en uso normal (usuarios especifican 0.01s - 2.0s)
- Posible issue solo si alguien pasa 0.00005s
- En la práctica: 0%

**Solución Propuesta:**

```cpp
// ✅ MEJORADO - Double precision intermedia
void Voice::setDCWAttack(float seconds) noexcept
{
    double ms = static_cast<double>(seconds) * 1000.0;  // ← Double
    dcwADSR.attackMs = static_cast<float>(
        std::clamp(ms, 0.5, 8000.0)  // ← Clamp en double
    );
    updateDCWEnvelopeFromADSR();
}
```

**Esfuerzo:** 5 minutos (cosmético)  
**Riesgo:** Muy bajo - cambio defensivo  
**Prioridad:** BAJA - Opcional

---

## 📊 Resumen de Mejoras

| Mejora | Prioridad | Esfuerzo | Riesgo | Estado |
|--------|-----------|----------|--------|--------|
| Usar this->sampleRate en updateXXX() | 🔴 ALTA | 5 min | Bajo | **FIX REQUIRED** |
| Serializar 8-stage envelope data | 🔴 ALTA | 40 min | Bajo | **FIX REQUIRED** |
| Completar rate editing UI | 🟡 MEDIA | 15 min | Muy bajo | Opcional |
| Precisión float en setters | 🟢 BAJA | 5 min | Muy bajo | Opcional |

---

## 🚀 Plan de Implementación

### Fase 1: Fixes Críticos (25 minutos)
1. ✅ Reemplazar 44100.0 con sampleRate (5 min)
2. ✅ Implementar JSON envelope serialization (20 min)
3. ✅ Testear en 96kHz y 192kHz

### Fase 2: Polish (20 minutos)
1. ✅ Agregar rate editing en EnvelopeEditor (15 min)
2. ✅ Precisión float improvements (5 min)

### Fase 3: Validation (30 minutos)
1. ✅ Recompilar sin errores
2. ✅ Testing multi-sample-rate
3. ✅ Preset save/load test
4. ✅ Envelope editing test

**Total: ~75 minutos**

---

## ✅ Post-Implementation Checklist

- [ ] Compilación sin errores C++17 -O3
- [ ] No warnings (todas categorías)
- [ ] ADSR timing correcto a 44.1k, 96k, 192k
- [ ] Presets guardan/cargan envelopes correctamente
- [ ] EnvelopeEditor permite editar levels Y rates
- [ ] No crashes con inputs extremos
- [ ] Normalización osciladores 100% funcional
- [ ] Memory stable (sin leaks)
- [ ] CPU usage < 10% @ 8 voces 192k

---

## 📝 Final Status After Fixes

**Before Fixes:** 9.4/10 (Production Ready with caveats)  
**After Fixes:** **9.8/10** (Production Ready - Full Release)

**Next Step:** Apply these fixes, recompile, test 1 hour → **SHIP IT** 🚀

