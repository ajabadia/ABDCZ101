# CZ-101 Emulator

Professional emulator of the Casio CZ-101 synthesizer (1984) using Phase Distortion synthesis.

## 🎯 Status

- **Documentation:** ✅ 100% Complete (21 docs)
- **Milestone 0:** ✅ 95% Complete (Infrastructure)
- **Milestone 1:** ✅ 100% Complete (Oscillator)
- **Version:** 0.2.0-alpha
- **Compilation:** ✅ 5/5 Successful
- **Plugin:** ✅ Functional
- **Progress:** 19% (1.9/10 phases)

## 🚀 Quick Start

To build the project on Windows, use the unified management script:

```powershell
# 1. Clone the repository
git clone https://github.com/your-username/ABDZ101.git
cd ABDZ101

# 2. Build the project (Release)
.\scripts\manage.ps1 -task build
```

Other tasks available:
- `.\scripts\manage.ps1 -task clean` - Remove build artifacts
- `.\scripts\manage.ps1 -task test`  - Run unit tests
- `.\scripts\manage.ps1 -task gm`    - Run Golden Master validation


> **Note on First Compilation:** The initial build process, especially when CMake configures JUCE for the first time, can be slow (10-15 minutes). If you encounter any issues, please refer to our detailed [Compilation Troubleshooting Guide](DOCS/GEMINI/COMPILATION_TROUBLESHOOTING.md).

## 📚 Documentation

All development documentation is in the `DOCS/GEMINI/` directory:

- **[QUICK_START.md](DOCS/GEMINI/QUICK_START.md)** - Start here for a detailed setup.
- **[EXECUTIVE_SUMMARY.md](DOCS/GEMINI/EXECUTIVE_SUMMARY.md)** - Get a complete overview of the project.
- **[02_MILESTONES.md](DOCS/GEMINI/02_MILESTONES.md)** - Track our development progress.

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

## 📋 Features

### Implemented
- ✅ Core Phase Distortion synthesis engine (8 waveforms)
- ✅ 8-voice polyphony
- ✅ 8-segment envelopes (DCW, DCA, Pitch)
- ✅ VST3/AU/Standalone formats
- ✅ MIDI SysEx loading
- ✅ Basic preset management (64 slots)

### Planned
- ⏳ Effects Suite (Reverb, Chorus, Delay with more controls)
- ⏳ Advanced modulation matrix
- ⏳ Full preset bank compatibility
- ⏳ Micro-tuning support

## 📖 License

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for details.

## 🙏 Credits

Based on the groundbreaking Casio CZ-101 (1984) synthesizer.

---

**Current Milestone:** 0 - Infrastructure  
**Last Updated:** January 19, 2026
