# ⚡ CZ101 v2.1 - QUICK FIX GUIDE (5 minutos)

**Tienes 2 fixes críticos. Este documento te dice EXACTAMENTE qué cambiar.**

---

## FIX #1: Reemplazar 44100.0 con sampleRate (5 minutos)

### Cambio 1 de 3: Voice.cpp - updateDCWEnvelopeFromADSR()

**BUSCA:**
```cpp
void Voice::updateDCWEnvelopeFromADSR() noexcept
{
    std::array<float, 8> rates, levels;
    int sus, end;
    
    DSPADSRtoStageConverter::convertADSR(
        dcwADSR.attackMs,
        dcwADSR.decayMs,
        dcwADSR.sustainLevel,
        dcwADSR.releaseMs,
        rates, levels, sus, end,
        44100.0  // ← REEMPLAZA ESTA LÍNEA
    );
```

**REEMPLAZA CON:**
```cpp
void Voice::updateDCWEnvelopeFromADSR() noexcept
{
    std::array<float, 8> rates, levels;
    int sus, end;
    
    DSPADSRtoStageConverter::convertADSR(
        dcwADSR.attackMs,
        dcwADSR.decayMs,
        dcwADSR.sustainLevel,
        dcwADSR.releaseMs,
        rates, levels, sus, end,
        sampleRate  // ✅ CAMBIADO
    );
```

---

### Cambio 2 de 3: Voice.cpp - updateDCAEnvelopeFromADSR()

**BUSCA:**
```cpp
void Voice::updateDCAEnvelopeFromADSR() noexcept
{
    // ...
    DSPADSRtoStageConverter::convertADSR(
        dcaADSR.attackMs,
        dcaADSR.decayMs,
        dcaADSR.sustainLevel,
        dcaADSR.releaseMs,
        rates, levels, sus, end,
        44100.0  // ← REEMPLAZA
    );
```

**REEMPLAZA CON:**
```cpp
void Voice::updateDCAEnvelopeFromADSR() noexcept
{
    // ...
    DSPADSRtoStageConverter::convertADSR(
        dcaADSR.attackMs,
        dcaADSR.decayMs,
        dcaADSR.sustainLevel,
        dcaADSR.releaseMs,
        rates, levels, sus, end,
        sampleRate  // ✅ CAMBIADO
    );
```

---

### Cambio 3 de 3: Voice.cpp - updatePitchEnvelopeFromADSR()

**BUSCA:**
```cpp
void Voice::updatePitchEnvelopeFromADSR() noexcept
{
    // ...
    DSPADSRtoStageConverter::convertADSR(
        pitchADSR.attackMs,
        pitchADSR.decayMs,
        pitchADSR.sustainLevel,
        pitchADSR.releaseMs,
        rates, levels, sus, end,
        44100.0  // ← REEMPLAZA
    );
```

**REEMPLAZA CON:**
```cpp
void Voice::updatePitchEnvelopeFromADSR() noexcept
{
    // ...
    DSPADSRtoStageConverter::convertADSR(
        pitchADSR.attackMs,
        pitchADSR.decayMs,
        pitchADSR.sustainLevel,
        pitchADSR.releaseMs,
        rates, levels, sus, end,
        sampleRate  // ✅ CAMBIADO
    );
```

---

**✅ FIX #1 COMPLETO - Recompila y verifica sin errores**

---

## FIX #2: JSON Serialization (40 minutos)

### Cambio 1 de 2: PresetManager.cpp - saveBankToFile()

**BUSCA ESTA SECCIÓN:**
```cpp
void PresetManager::saveBankToFile(const jucFile& file)
{
    juceArray<juceVar> bankArray;
    
    for (const auto& preset : presets) {
        juceDynamicObject obj = new juceDynamicObject;
        
        // Name & params
        obj->setProperty("name", juceString(preset.name));
        juceDynamicObject paramsObj = new juceDynamicObject;
        for (const auto& [id, val] : preset.parameters) {
            paramsObj->setProperty(juceIdentifier(id), val);
        }
        obj->setProperty("params", paramsObj);
        
        // ← FALTA AQUÍ: Serializar envelopes
        
        bankArray.append(obj);
    }
    
    juceString jsonString = juceJSON::toString(bankArray);
    file.replaceWithText(jsonString);
}
```

**REEMPLAZA CON:**
```cpp
void PresetManager::saveBankToFile(const jucFile& file)
{
    juceArray<juceVar> bankArray;
    
    for (const auto& preset : presets) {
        juceDynamicObject obj = new juceDynamicObject;
        
        // Name & params
        obj->setProperty("name", juceString(preset.name));
        juceDynamicObject paramsObj = new juceDynamicObject;
        for (const auto& [id, val] : preset.parameters) {
            paramsObj->setProperty(juceIdentifier(id), val);
        }
        obj->setProperty("params", paramsObj);
        
        // ✅ AGREGAR: Serialize DCW Envelope
        juceDynamicObject dcwObj = new juceDynamicObject;
        {
            juceArray<juceVar> ratesArray;
            for (int i = 0; i < 8; ++i) ratesArray.append(preset.dcwEnv.rates[i]);
            juceArray<juceVar> levelsArray;
            for (int i = 0; i < 8; ++i) levelsArray.append(preset.dcwEnv.levels[i]);
            dcwObj->setProperty("rates", ratesArray);
            dcwObj->setProperty("levels", levelsArray);
            dcwObj->setProperty("sustainPoint", preset.dcwEnv.sustainPoint);
            dcwObj->setProperty("endPoint", preset.dcwEnv.endPoint);
        }
        obj->setProperty("dcwEnv", dcwObj);
        
        // ✅ AGREGAR: Serialize DCA Envelope
        juceDynamicObject dcaObj = new juceDynamicObject;
        {
            juceArray<juceVar> ratesArray;
            for (int i = 0; i < 8; ++i) ratesArray.append(preset.dcaEnv.rates[i]);
            juceArray<juceVar> levelsArray;
            for (int i = 0; i < 8; ++i) levelsArray.append(preset.dcaEnv.levels[i]);
            dcaObj->setProperty("rates", ratesArray);
            dcaObj->setProperty("levels", levelsArray);
            dcaObj->setProperty("sustainPoint", preset.dcaEnv.sustainPoint);
            dcaObj->setProperty("endPoint", preset.dcaEnv.endPoint);
        }
        obj->setProperty("dcaEnv", dcaObj);
        
        // ✅ AGREGAR: Serialize Pitch Envelope
        juceDynamicObject pitchObj = new juceDynamicObject;
        {
            juceArray<juceVar> ratesArray;
            for (int i = 0; i < 8; ++i) ratesArray.append(preset.pitchEnv.rates[i]);
            juceArray<juceVar> levelsArray;
            for (int i = 0; i < 8; ++i) levelsArray.append(preset.pitchEnv.levels[i]);
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
```

---

### Cambio 2 de 2: PresetManager.cpp - loadBankFromFile()

**BUSCA ESTA SECCIÓN:**
```cpp
void PresetManager::loadBankFromFile(const jucFile& file)
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
                p.parameters[key.toString().toStdString()] = (float)paramsObj[key];
            }
        }
        
        // ← FALTA AQUÍ: Cargar envelopes
        
        presets.push_back(p);
    }
}
```

**REEMPLAZA CON:**
```cpp
void PresetManager::loadBankFromFile(const jucFile& file)
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
                p.parameters[key.toString().toStdString()] = (float)paramsObj[key];
            }
        }
        
        // ✅ AGREGAR: Load DCW Envelope
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
        
        // ✅ AGREGAR: Load DCA Envelope
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
        
        // ✅ AGREGAR: Load Pitch Envelope
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

---

**✅ FIX #2 COMPLETO**

---

## 🧪 Verification Steps (después de ambos fixes)

```bash
# 1. Recompila
g++ -std=c++17 -O3 *.cpp -o cz101_test 2>&1 | grep -i error

# 2. Verifica compilación limpia
echo "Compilation status: $?"  # Debe ser 0

# 3. Test ADSR @ diferentes sample rates
./test_adsr_timing 44100
./test_adsr_timing 96000
./test_adsr_timing 192000
# Todos deben mostrar tiempos ADSR idénticos en segundos

# 4. Test Preset save/load
./test_preset_save
# Debe preservar envelope data
```

---

## ✅ CHECKLIST

- [ ] Cambio 1 de FIX #1: Voice::updateDCWEnvelopeFromADSR() 
- [ ] Cambio 2 de FIX #1: Voice::updateDCAEnvelopeFromADSR()
- [ ] Cambio 3 de FIX #1: Voice::updatePitchEnvelopeFromADSR()
- [ ] Cambio 1 de FIX #2: PresetManager::saveBankToFile()
- [ ] Cambio 2 de FIX #2: PresetManager::loadBankFromFile()
- [ ] Recompila sin errores
- [ ] ADSR timing correcto @ 44.1k, 96k, 192k
- [ ] Presets guardan/cargan correctamente

---

**¡FIX COMPLETADO! Ya puedes hacer DEPLOY. 🚀**

