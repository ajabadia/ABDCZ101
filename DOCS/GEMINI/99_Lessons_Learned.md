# DeepMindSynth Lessons Learned & Error Log

A record of obstacles encountered and solutions applied during development.

## 1. Build System & Compilation
- **Issue**: "Ghost Build" / Exit Code 1 with no output.
- **Cause**: Redirecting output (`> log.txt`) in `build_nopause.bat` sometimes silenced critical errors or failed to write if the process crashed early. Also, duplicate includes.
- **Solution**: 
  - Use `cmake --build ...` directly in the terminal to see potential environment errors.
  - Check for duplicate `#include` directives in generated code (e.g., `PluginEditor.cpp`).
  - Ensure `SysexInjector` has `<cstring>` for `memcpy`.
- **Lesson**: Don't trust "silent" build scripts. Always have a verbose fallback.

## 2. MIDI & SysEx
- **Issue**: Incoming SysEx data (e.g., Calibration) could corrupt the synth.
- **Solution**: Implemented a "Blacklist" for Cmd 12 (Calibration Dump).
- **Issue**: NRPN Data format confusion.
- **Lesson**: DeepMind uses "Packed MS Bit" format for SysEx. Reading raw bytes results in garbage. We must use an unpacker (8 bytes -> 7 bytes).

## 3. Audio & DSP
- **Issue**: Aliasing in naive DCOs.
- **Solution**: PolyBLEP is mandatory for Saw/Square waves at 48kHz.
- **Issue**: 44.1kHz vs 48kHz mismatch.
- **Lesson**: The hardware is native 48kHz. Running the VST at 44.1kHz without recalculating coefficients causes detuning and timing drift in delays.

## 4. Documentation Strategy
- **Issue**: Fragmented knowledge in chat.
- **Solution**: Created `Docs/TechSpecs/` with atomic, topic-based files (`01` to `10`) to serve as a Single Source of Truth.

## 5. Architecture & Namespaces (Historical)
- **Issue**: Conflict between `DeepMind` and `juce::dsp`.
- **Solution**: Renamed namespace to `DeepMindDSP`.
- **Lesson**: Use project-specific namespaces (e.g., `DeepMind::Module`) to avoid JUCE collisions.

## 6. DSP & Threading (Historical)
- **Issue**: Changing voice count at runtime caused crashes.
- **Solution**: Implemented `updatePolyphony()` with `synthesiser.suspendProcessing(true)`.
- **Lesson**: Audio graph changes must be thread-safe and ideally suspended during reconfiguration.

## 7. GUI Layout (Historical)
- **Issue**: Manual `setBounds` became unmanageable for 40+ sliders.
- **Solution**: Adopted "Strip" layout using `area.removeFromLeft(ratio)`.
- **Lesson**: Define "Sections" first, then layout relative controls. Stick to relative, not absolute pixels.

## 8. Build System (Historical)
- **Issue**: Linker errors with Circular Dependencies (`ModMatrix`).
- **Solution**: Strict `.h` vs `.cpp` separation and Forward Declarations.
- **Lesson**: Heavy implementations belong in `.cpp`. Headers should remain lightweight.

---

## CZ-101 EMULATOR LESSONS (14 Diciembre 2025)

### 9. std::clamp Requires <algorithm>

**Problem:**
```
error C2039: "clamp": no es un miembro de "std"
error C3861: 'clamp': no se encontró el identificador
```

**Cause:** Using `std::clamp()` without including `<algorithm>`

**Solution:**
```cpp
#include <algorithm>  // For std::clamp
```

**Affected Files:** Voice.cpp, ResonantFilter.cpp, Delay.cpp

**Lesson:** Although `std::clamp` is C++17, it requires `<algorithm>` explicitly, not included with `<cmath>`.

---

### 10. JUCE MIDI Types Need Explicit Include

**Problem:**
```
error C2653: 'juce': no es un nombre de clase o espacio de nombres
```

**Cause:** Using `juce::MidiMessage` and `juce::MidiBuffer` without JUCE header

**Solution:**
```cpp
#include <juce_audio_processors/juce_audio_processors.h>
```

**Affected Files:** MIDIProcessor.h

**Lesson:** JUCE types are not auto-included. Always include the specific module containing the classes you use.

---

### 11. Unused Variables Generate Warnings

**Problem:**
```
warning C4189: 'PI': variable initialized but not referenced
warning C4189: 'lowTime': variable initialized but not referenced
```

**Cause:** Declaring constexpr variables that aren't used in code

**Solution:**
- Remove if truly unused
- Or comment its purpose if it's documentation

**Affected Files:**
- WaveShaper.cpp (PI unused)
- WaveTable.cpp (lowTime implicit in else)

**Lesson:** Keep code clean. If a constant isn't used, remove it or document why it's there.

---

### 12. Modular Architecture Enables Reusability

**Observation:** 85-90% of DSP code is reusable across different synth projects

**Implementation:**
- Separate namespaces (CZ101::DSP, CZ101::Core, etc.)
- Low coupling between modules
- High cohesion within modules
- Header-only utilities when possible

**Benefit:** Can extract as SynthDSP library for future projects (MiniMoog, Juno-106, etc.)

**Lesson:** Design for reusability from day one. Modular architecture pays off long-term.

---

### 13. Incremental Compilation Strategy

**Strategy:** Implement 2-3 milestones, then compile

**Benefits:**
- Catch errors early
- Verify integration points
- Maintain momentum

**Lesson:** Don't implement entire project before first compilation. Incremental validation prevents cascading errors.

---

### 14. Documentation as You Go

**Strategy:** Document each milestone immediately after completion

**Benefits:**
- Fresh context
- Accurate details
- Lessons captured while relevant

**Created:**
- 30+ documents (~900 KB)
- Lessons learned log
- Reusable library guide
- Architecture documentation

**Lesson:** Documentation debt compounds. Document immediately while context is fresh.

---

### 15. Performance Monitoring from Start

**Implementation:** PerformanceMonitor utility from Milestone 9

**Benefits:**
- Early CPU usage visibility
- Identify bottlenecks early
- Optimize before it's critical

**Lesson:** Build performance monitoring into architecture, don't add it as afterthought.

---

### 16. JUCE getParameters() Returns Const Array

**Problem:**
```
error C2663: 'juce::Array::add': no se puede convertir el puntero 'this' 
de 'const juce::Array' a 'juce::Array &'
Se pierden calificadores en la conversión
```

**Cause:** Using `getParameters().add()` - the array is const

**Wrong:**
```cpp
auto& params = audioProcessor.getParameters();
params.add(myParameter);  // ERROR: params is const
```

**Correct:**
```cpp
audioProcessor.addParameter(myParameter);
```

**Affected Files:** Parameters.cpp

**Lesson:** JUCE AudioProcessor provides `addParameter()` method directly. Don't try to get the parameters array and modify it - it's const for a reason.

---

### 17. Namespace Qualification Required

**Problem:**
```
error C2653: 'State': no es un nombre de clase o espacio de nombres
error C2653: 'Core': no es un nombre de clase o espacio de nombres
```

**Cause:** Using partial namespace qualification when classes are inside nested namespaces

**Wrong:**
```cpp
// In file outside CZ101 namespace
State::Parameters parameters;  // ERROR: State not found
Core::VoiceManager voiceManager;  // ERROR: Core not found
```

**Correct:**
```cpp
// Use fully qualified names
CZ101::State::Parameters parameters;
CZ101::Core::VoiceManager voiceManager;
```

**Affected Files:** PluginProcessor.h

**Lesson:** When using classes from nested namespaces (`namespace CZ101 { namespace State { } }`), always use fully qualified names from outside the namespace, or add `using namespace CZ101;` at the top.

---

### 18. Method Declarations Must Match Implementation

**Problem:**
```
error C3861: 'createBrassPreset': no se encontró el identificador
error C2039: "createBrassPreset": no es un miembro de "PresetManager"
```

**Cause:** Methods implemented in .cpp but not declared in .h

**Wrong:**
```cpp
// PresetManager.h
class PresetManager {
    void createInitPreset();  // Only this declared
};

// PresetManager.cpp
void PresetManager::createBrassPreset() { }  // ERROR: not declared
```

**Correct:**
```cpp
// PresetManager.h
class PresetManager {
    void createInitPreset();
    void createBrassPreset();  // Declare all methods
};
```

**Affected Files:** PresetManager.h

**Lesson:** Every method implemented in .cpp must be declared in .h. C++ requires forward declarations.

---


### 19. UI Components Must Be Instantiated

**Problem:**
Standalone app ran but showed empty/placeholder UI, despite components (Knob, WaveformDisplay) being implemented.

**Cause:**
`PluginEditor` only had placeholder text. The custom components were created in separate files but never added/instantiated in the editor's constructor.

**Solution:**
Instantiate and `addAndMakeVisible()` all components in `PluginEditor.cpp`.

**Lesson:**
Implementing a component class doesn't make it appear. You must explicitly add it to the parent component hierarchy.

---

### 20. JUCE GUI Classes Need Specific Includes

**Problem:**
```
error C2039: "MidiKeyboardComponent": no es un miembro de "juce"
```

**Cause:**
`MidiKeyboardComponent` is part of `juce_audio_utils` module, which is not included by default in `juce_audio_processors`.

**Solution:**
```cpp
#include <juce_audio_utils/juce_audio_utils.h>
```

**Lesson:**
If a JUCE class isn't found, check which module it belongs to and ensure that module's header is included.

---

### 21. Build Scripts Must Handle New VS Versions

**Problem:**
`build.bat` failed to find Visual Studio, while `build_clean.bat` worked.

**Cause:**
`build.bat` only checked for specific paths (2022 Community/Pro/Enterprise). The user has VS 2026 (Internal v18), which resides in a different path.

**Solution:**
Updated `build.bat` to verify `Microsoft Visual Studio\18\Community` and `Insiders` paths.

**Lesson:**
Build scripts should be robust to environment variations. Hardcoding version paths leads to fragility when IDEs update.

---

**Last Updated:** 15 Diciembre 2025, 00:40  
**Total Lessons:** 21  
**Projects:** DeepMindSynth + CZ-101 Emulator
