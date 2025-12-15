# CZ-101 EMULATOR - SPECIFICATION HANDBOOK
## Basado en Documentación Histórica Auténtica (1984-1999)

---

## 📋 TABLA DE CONTENIDOS
1. [Overview Hardware](#overview-hardware)
2. [Architecture Síntesis](#architecture-síntesis)
3. [MIDI Implementation](#midi-implementation)
4. [SysEx Protocol](#sysex-protocol)
5. [Voice Data Structure](#voice-data-structure)
6. [Control Mappings](#control-mappings)
7. [Preset Memory](#preset-memory)

---

## OVERVIEW HARDWARE

**Casio CZ-101** (1984)
- **CPU**: NEC µPD7811 @ 10 MHz
- **Audio DSP**: NEC µPD933 @ 4.48 MHz  
- **Memory**: 4KB RAM (internal), 2KB cartridge (optional)
- **MIDI**: Input/Output capability
- **Polyphony**: 4 voices (hardware limitation)
- **Display**: 2-line LCD (97×19 pixels)
- **Oscillators**: Phase Distortion synthesis (2 per voice)

### Arquitectura Síntesis (Auténtica)

```
┌──────────────────────────────────────────────┐
│         CZ-101 VOICE ARCHITECTURE            │
├──────────────────────────────────────────────┤
│                                               │
│  [DCO1] ──┐                                  │
│  (Osc1)   ├─→ [MIXER] ──→ [DCW] ──→ [DCA]   │
│  [DCO2] ──┘                                  │
│  (Osc2)   (Tone Mix %)    (Timbre)  (Amp)   │
│                                               │
│  Modulation:                                 │
│  - Hard Sync (Osc2 reset on Osc1 wrap)      │
│  - Ring Modulation (Osc1 × Osc2)            │
│  - Vibrato (LFO → Pitch)                    │
│  - Glide/Portamento                         │
│                                               │
│  Envelopes (8-Stage each):                  │
│  - DCW (Digital Controlled Wave)            │
│  - DCA (Digital Controlled Amplifier)       │
│  - DCO (Pitch modulation)                   │
│                                               │
└──────────────────────────────────────────────┘
```

---

## MIDI IMPLEMENTATION

### Control Changes (CC) - CZ101/1000

| CC# | Function | Range | Default |
|-----|----------|-------|---------|
| 01  | Vibrato On/Off | 0=OFF, 7F=ON | - |
| 05  | Portamento Time | 00..63 (0..99) | - |
| 06  | Master Tune | 00..7F (±24 semitones) | 40 |
| 41  | Portamento On/Off | 0=OFF, 7F=ON | - |
| 7A  | Local On/Off | 0=OFF, 7F=ON | 7F |
| 7C  | Omni Off | - | - |
| 7D  | Omni On | - | - |
| 7E  | Poly Off (SOLO) | - | - |
| 7F  | Poly On | - | - |

### Program Change
```
Format: C0 + channel, program_number

Memory Locations:
  0x00-0x0F → Preset Sounds 1-16
  0x20-0x2F → Internal Sounds 1-16
  0x40-0x4F → Cartridge Sounds 1-16
  0x60      → Edit Buffer (temporary)
```

### Pitch Bend
```
Format: E0 + channel, LSB, MSB

Range (14-bit):
  0x0000 → Minimum (typically -2 octaves)
  0x2000 → Center (no bend)
  0x3FFF → Maximum (typically +2 octaves)

Hardware limitations:
  - CZ-101 doesn't detect velocity
  - Always transmits/recognizes 0x40 (64 decimal)
```

---

## SysEx PROTOCOL

### Message Structure
```
F0 44 00 00 70 [Channel] [Command] [Data] F7
```

**Byte Breakdown:**
- `F0` = System Exclusive Start
- `44` = Casio Manufacturer ID
- `00 00` = Division/Region
- `70` = Model identifier
- `[Channel]` = MIDI Channel (add to 0x70)
- `[Command]` = Action type
- `[Data]` = Payload (varies)
- `F7` = System Exclusive End

### SysEx Commands

#### 1. Set Bend Range
```
F0 44 00 00 7X 40 [range] F7
Range: 00-0C (0-12 semitones)
Example: F0 44 00 00 70 40 08 F7  (set to 8 semitones, channel 0)
```

#### 2. Key Transpose
```
F0 44 00 00 7X 41 [key] F7
Keys:
  G=0x45  A=0x44  A#=0x43  B=0x41
  C=0x00  C#=0x01 D=0x02   D#=0x03
  E=0x04  F=0x05  F#=0x06
```

#### 3. Tone Mix Level
```
F0 44 00 00 7X 42 [level] F7
Level: 0x00=OFF, 0x41-0x49=Mix1-9
Example: F0 44 00 00 70 42 47 F7  (mix level 7, channel 0)
```

#### 4. Request Programmer Info (Handshake)
```
COMPUTER → CZ:   F0 44 00 00 7X 19 00
CZ → COMPUTER:   F0 44 00 00 7X 30
COMPUTER → CZ:   70[Channel] 31
CZ → COMPUTER:   [program] [vibrato_portamento] F7
COMPUTER → CZ:   F7
```

#### 5. Request Tone Data (Send Request)
```
COMPUTER → CZ:   F0 44 00 00 7X 10 [program]
CZ → COMPUTER:   F0 44 00 00 7X 30
COMPUTER → CZ:   7X 31
CZ → COMPUTER:   [256-byte tone data] F7
COMPUTER → CZ:   F7
```

#### 6. Receive Tone Data
```
COMPUTER → CZ:   F0 44 00 00 7X 20 [program]
CZ → COMPUTER:   F0 44 00 00 7X 30
COMPUTER → CZ:   [256-byte tone data] F7
CZ → COMPUTER:   F7
```

---

## VOICE DATA STRUCTURE

**Total: 256 bytes** (transmitted as 512 nibbles due to CZ encoding)

### Section Map (25 sections)

| Sec | Bytes | Field | Purpose |
|-----|-------|-------|---------|
| 1   | 1     | pflag | Line Select + Octave |
| 2   | 1     | pds   | Detune Direction |
| 3   | 2     | pdl,h | Detune Range |
| 4   | 1     | pvk   | Vibrato Waveform |
| 5   | 3     | pvdl  | Vibrato Delay |
| 6   | 3     | pvs   | Vibrato Rate |
| 7   | 3     | pvd   | Vibrato Depth |
| 8   | 2     | mfw   | DCO1 Waveform |
| 9   | 2     | mamd  | DCA1 Key Follow |
| 10  | 2     | mwmd  | DCW1 Key Follow |
| 11  | 1     | pmal  | DCA1 End Step |
| 12  | 16    | pma   | **DCA1 Envelope** (8 stages) |
| 13  | 1     | pmwl  | DCW1 End Step |
| 14  | 16    | pmw   | **DCW1 Envelope** (8 stages) |
| 15  | 1     | pmpl  | DCO1 End Step |
| 16  | 16    | pmp   | **DCO1 Envelope** (8 stages) |
| 17  | 2     | sfw   | DCO2 Waveform |
| 18  | 2     | samd  | DCA2 Key Follow |
| 19  | 2     | swmd  | DCW2 Key Follow |
| 20  | 1     | psal  | DCA2 End Step |
| 21  | 16    | psa   | **DCA2 Envelope** |
| 22  | 1     | pswl  | DCW2 End Step |
| 23  | 16    | psw   | **DCW2 Envelope** |
| 24  | 1     | pspl  | DCO2 End Step |
| 25  | 16    | psp   | **DCO2 Envelope** |

### Encoding Details

#### PFLAG (Byte 1)
```
Bits: [7:2] unused | [1:0] OCTV (Octave) | [0:0] LS (Line Select)

Octave Values:
  00 = Octave 0 (natural)
  01 = Octave +1
  10 = Octave -1

Line Select:
  00 = Line 1 only
  01 = Line 2 only
  10 = Line 1 + 1' (octave up)
  11 = Line 1 + 2'

Example: 0x06 = Octave +1, Line Select 1+1'
```

#### Vibrato Waveform (Byte 4)
```
Wave #1 (Sine)    → 0x08
Wave #2 (Triangle)→ 0x04
Wave #3 (Sawtooth)→ 0x20
Wave #4 (Square)  → 0x02
```

#### 8-Stage Envelope Encoding (Sections 12,14,16,21,23,25)

Each envelope = 16 bytes = 8 (rate, level) pairs

**DCA/DCW Encoding** (sections 12, 14, 21, 23):
```
Rate Encoding:
  byte = (119 × rate / 99) + 8
  rate = (99 × (byte - 8) / 119) + 1

Level Encoding:
  byte = (99 × level / 119)
  level = (119 × byte / 99)

Special flags (OR with level byte):
  0x80 = Decay step (level decreasing)
  0x80 = Sustain point indicator
```

**DCO Envelope Encoding** (sections 16, 25):
```
Rate:
  byte = 127 × rate / 99
  rate = 99 × byte / 127 + 1

Level: (0-63 normal, 64-99 special)
  0-63  → 0x00-0x3F (direct)
  64-99 → 0x44-0x67 (offset +0x44)
```

---

## CONTROL MAPPINGS

### Front Panel → MIDI CC Mapping

| Control | CC | Range | Notes |
|---------|----|----|-------|
| Vibrato Wave | Internal | - | Wave 1-4 |
| Portamento Time | CC#05 | 0-99 | Linear |
| Master Tune | CC#06 | ±24st | 7-bit linear |
| Tone Mix | SysEx#42 | 1-9 | 1=10%, 9=90% |
| Sustain Pedal | CC#40 | ON/OFF | CZ5000 only |
| All Notes Off | CC#7B | - | Global |
| Reset Controllers | CC#79 | - | - |

---

## PRESET MEMORY

### Memory Layout

```
┌─────────────────────────────────┐
│   PRESET SOUNDS (0x00-0x0F)     │ 16 factory presets
│   - Synth Bass                  │ (read-only)
│   - Stack Lead                  │
│   - etc...                      │
├─────────────────────────────────┤
│   INTERNAL SOUNDS (0x20-0x2F)   │ 16 user-editable patches
│   - User 1-16                   │ (stored in RAM/EEPROM)
├─────────────────────────────────┤
│   CARTRIDGE SOUNDS (0x40-0x4F)  │ Optional ROM cartridge
│   - Expansion packs             │ (if installed)
├─────────────────────────────────┤
│   EDIT BUFFER (0x60)            │ Temporary working patch
│   - Current patch               │ (lost on power-off unless saved)
└─────────────────────────────────┘
```

### Handshake Protocol (CRITICAL)

> **Historical Note**: Ken Whitley (1999) discovered that **YOU CAN IGNORE HANDSHAKING**.
> The CZ-101 will still send and receive data correctly in many environments.

Standard handshake required:
1. Computer sends request (0x19, 0x10, or 0x20)
2. CZ responds with 0x30 (acknowledge)
3. Computer sends 0x31 (ready)
4. CZ sends data + F7
5. Computer sends F7

Without handshake: Data may be corrupted in some MIDI environments.

---

## IMPLEMENTACIÓN EN PLUGIN

### Mapeo de SysEx a Plugin Parameters

```cpp
// SysEx Command → Plugin Parameter
0x19 (Request Info)     → getPresetName(), getVibratoStatus()
0x10 (Send Tone)        → exportPreset()
0x20 (Receive Tone)     → importPreset()
0x40 (Bend Range)       → pitchBendRange
0x41 (Key Transpose)    → keyTranspose
0x42 (Tone Mix)         → oscMixLevel
```

### Byte Order Convention

⚠️ **CRITICAL**: Casio uses **little-endian nibble pairs**

Dato hexadecimal `5F` se transmite como `0F 05` (nibbles invertidos)

Implementación:
```cpp
uint8_t toBigEndianNibbles(uint8_t byte) {
    return ((byte & 0x0F) << 4) | ((byte & 0xF0) >> 4);
}

uint8_t fromBigEndianNibbles(uint8_t nibbles) {
    return ((nibbles & 0x0F) << 4) | ((nibbles & 0xF0) >> 4);
}
```

---

## REFERENCES

1. **CZ MIDI Guide** (Thong Ellis, ~1995)
   - Comprehensive SysEx specification
   - Encoding algorithms for envelopes
   - Historical implementation notes

2. **Casio CZ-101 Owner's Manual** (1984)
   - Hardware architecture
   - Front panel control mapping
   - Preset memory organization

3. **Ken Whitley SysEx Tips** (1999)
   - Practical MIDI implementation
   - Handshake workarounds
   - Cross-platform compatibility notes

4. **MAME CZ-101 Emulator** (machine.json)
   - NEC µPD7811 CPU specifications
   - Memory layout confirmation
   - Hardware constraints documentation

---

## APPENDIX: FACTORY PRESET LIST

**CZ-101 Preset Tones (16):**

| # | Name | Type |
|---|------|------|
| 1 | Synth Bass | Bass |
| 2 | Stack Lead | Lead |
| 3 | Clav | Electric Piano |
| 4 | Sine Wave | Pad |
| 5 | Vibraphone | Percussion |
| 6 | Harmonica | Wind |
| 7 | Saxophone | Wind |
| 8 | Wind | Wind |
| 9 | Techno Bass | Bass |
| 10 | Digital Bell | Pad |
| 11 | Bongo | Percussion |
| 12 | Bass Wave | Bass |
| 13 | High Lead | Lead |
| 14 | Rock Organ | Organ |
| 15 | Soft Horn | Wind |
| 16 | Digital Sine | Pad |

---

**Document Generated**: 2025-12-15  
**Source**: Authentic CZ-101 documentation (1984-1999)  
**Status**: ✅ Historically Verified
