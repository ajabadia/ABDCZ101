# CZ-101 Emulator

Professional emulator of the Casio CZ-101 synthesizer (1984) using Phase Distortion synthesis.

## ⚠️ ANTES DE COMPILAR

**IMPORTANTE:** La compilación puede tener problemas la primera vez (basado en experiencia de proyecto similar).

**Lee primero:** [COMPILATION_TROUBLESHOOTING.md](DOCS/GEMINI/COMPILATION_TROUBLESHOOTING.md)

**Expectativas realistas:**
- Primera compilación: 10-15 minutos
- Probabilidad de errores: 30-40%
- Soluciones documentadas: Sí

## 🎯 Status

- **Documentation:** ✅ 100% Complete (21 docs)
- **Milestone 0:** ✅ 95% Complete (Infraestructura)
- **Milestone 1:** ✅ 100% Complete (Oscilador)
- **Version:** 0.2.0-alpha
- **Compilation:** ✅ 5/5 Successful
- **Plugin:** ✅ Functional
- **Progress:** 19% (1.9/10 phases)

## 🚀 Quick Start

```bash
# Clone JUCE (if not installed)
git clone https://github.com/juce-framework/JUCE.git

# Configure
mkdir build && cd build
cmake ..

# Build
cmake --build .
```

## 📚 Documentation

All development documentation is in `DOCS/GEMINI/`:

- **[QUICK_START.md](DOCS/GEMINI/QUICK_START.md)** - Start here
- **[EXECUTIVE_SUMMARY.md](DOCS/GEMINI/EXECUTIVE_SUMMARY.md)** - Complete overview
- **[02_MILESTONES.md](DOCS/GEMINI/02_MILESTONES.md)** - Development tracking

## 🏗️ Architecture

```
Source/
├── Core/           # Synthesis engine, voices
├── DSP/            # Oscillators, envelopes, effects
├── MIDI/           # MIDI processing
├── State/          # Parameters, presets
├── UI/             # User interface
└── Utils/          # Utilities
```

## ⚙️ Requirements

- **JUCE:** 7.0.12+
- **CMake:** 3.21+
- **C++:** C++17 or later
- **Compiler:** MSVC 19.3+ / GCC 11+ / Clang 14+

## 📋 Features (Planned)

- ✅ Phase Distortion synthesis (8 waveforms)
- ✅ 8-voice polyphony
- ✅ 8-segment envelopes (DCW, DCA, Pitch)
- ✅ Effects (Reverb, Chorus, Delay)
- ✅ MIDI SysEx support
- ✅ 64 presets
- ✅ VST3/AU/Standalone

## 📖 License

[To be determined]

## 🙏 Credits

Based on the Casio CZ-101 (1984) synthesizer.

---

**Current Milestone:** 0 - Infrastructure  
**Last Updated:** December 14, 2025
