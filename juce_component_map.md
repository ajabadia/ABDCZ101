# JUCE Component & Parameter Mapping

This document provides a complete and exact dump of all `juce::Component` subclasses used in the **ABD CZ-101** user interface editor, their parameter ID attachments (`juce::AudioProcessorValueTreeState::ParameterAttachment`), ranges, default values, data types, and callback structures.

---

## 1. Oscillator Section (`OscillatorSection`)
*Contiene la selección de formas de onda y niveles para las dos líneas DCO (Digital Controlled Oscillators) y controles compartidos.*

| Clase Componente | Control UI (ID C++) | ID Parámetro APVTS | Tipo de Dato | Rango / Opciones | Valor por Defecto | Retrollamada / Sincronización |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `juce::ComboBox` | `osc1WaveSelector` | `OSC1_WAVEFORM` | Choice | 1: Saw, 2: Square, 3: Pulse, 4: Dbl Sine, 5: SawPulse, 6: Reso 1, 7: Reso 2, 8: Reso 3 | `0` (Sawtooth) | `comboBoxChanged` / APVTS |
| `juce::ComboBox` | `osc1WaveSelector2` | `OSC1_WAVEFORM2` | Choice | 0: None, 1: Saw, 2: Square, 3: Pulse, 4: Dbl Sine, 5: SawPulse, 6: Reso 1, 7: Reso 2, 8: Reso 3 | `0` (None) | `comboBoxChanged` / APVTS |
| `Knob` (`juce::Slider`) | `osc1LevelKnob` | `OSC1_LEVEL` | Float | `0.0` a `1.0` | `1.0` | `sliderValueChanged` / APVTS |
| `juce::ComboBox` | `osc2WaveSelector` | `OSC2_WAVEFORM` | Choice | 1: Saw, 2: Square, 3: Pulse, 4: Dbl Sine, 5: SawPulse, 6: Reso 1, 7: Reso 2, 8: Reso 3 | `0` (Sawtooth) | `comboBoxChanged` / APVTS |
| `juce::ComboBox` | `osc2WaveSelector2` | `OSC2_WAVEFORM2` | Choice | 0: None, 1: Saw, 2: Square, 3: Pulse, 4: Dbl Sine, 5: SawPulse, 6: Reso 1, 7: Reso 2, 8: Reso 3 | `0` (None) | `comboBoxChanged` / APVTS |
| `Knob` (`juce::Slider`) | `osc2LevelKnob` | `OSC2_LEVEL` | Float | `0.0` a `1.0` | `0.0` | `sliderValueChanged` / APVTS |
| `Knob` (`juce::Slider`) | `osc2DetuneKnob` | `OSC2_DETUNE` | Float | `-12.0` a `12.0` (semitonos) | `0.0` | `sliderValueChanged` / APVTS |
| `Knob` (`juce::Slider`) | `lineMixKnob` | `LINE_MIX` | Float | `0.0` (L1) a `1.0` (L2) | `0.5` | `sliderValueChanged` / APVTS |
| `juce::TextButton` | `hardSyncButton` | `HARD_SYNC` | Boolean | `true` (On) / `false` (Off) | `false` | `buttonClicked` / APVTS |
| `juce::TextButton` | `ringModButton` | `RING_MOD` | Boolean | `true` (On) / `false` (Off) | `false` | `buttonClicked` / APVTS |

---

## 2. Filters & Effects Section (`EffectsSection`)
*Maneja los filtros y el procesador de efectos integrados (Drive, Chorus, Delay, Reverb).*

| Clase Componente | Control UI (ID C++) | ID Parámetro APVTS | Tipo de Dato | Rango / Opciones | Valor por Defecto | Retrollamada / Sincronización |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `Knob` (`juce::Slider`) | `cutoffKnob` | `MODERN_LPF_CUTOFF`| Float | `20.0 Hz` a `20000.0 Hz` | `20000.0 Hz` | `sliderValueChanged` / APVTS |
| `Knob` (`juce::Slider`) | `resonanceKnob` | `MODERN_LPF_RESO` | Float | `0.0` a `1.0` | `0.0` | `sliderValueChanged` / APVTS |
| `Knob` (`juce::Slider`) | `driveMixKnob` | `DRIVE_MIX` | Float | `0.0` a `1.0` | `0.0` | `sliderValueChanged` / APVTS |
| `Knob` (`juce::Slider`) | `chorusMixKnob` | `CHORUS_MIX` | Float | `0.0` a `1.0` | `0.3` | `sliderValueChanged` / APVTS |
| `Knob` (`juce::Slider`) | `delayMixKnob` | `DELAY_MIX` | Float | `0.0` a `1.0` | `0.0` | `sliderValueChanged` / APVTS |
| `Knob` (`juce::Slider`) | `reverbMixKnob` | `REVERB_MIX` | Float | `0.0` a `1.0` | `0.2` | `sliderValueChanged` / APVTS |

---

## 3. Envelope Parameters (`FilterLfoSection` & Envelopes)
*Controles para simplificar parámetros ADSR o etapas del LFO (Vibrato).*

| Clase Componente | Control UI (ID C++) | ID Parámetro APVTS | Tipo de Dato | Rango / Opciones | Valor por Defecto | Retrollamada / Sincronización |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `juce::ComboBox` | `lfoWaveSelector` | `LFO_WAVE` | Choice | Triangle, Saw Up, Saw Down, Square | `0` (Triangle) | `comboBoxChanged` / APVTS |
| `Knob` (`juce::Slider`) | `lfoRateKnob` | `LFO_RATE` | Float | `0.1 Hz` a `30.0 Hz` | `5.0 Hz` | `sliderValueChanged` / APVTS |
| `Knob` (`juce::Slider`) | `lfoDepthKnob` | `LFO_DEPTH` | Float | `0.0` a `1.0` | `0.0` | `sliderValueChanged` / APVTS |
| `Knob` (`juce::Slider`) | `lfoDelayKnob` | `LFO_DELAY` | Float | `0.0 s` a `2.0 s` | `0.0 s` | `sliderValueChanged` / APVTS |
| `juce::Slider` | Envolventes DCA | `DCA_ATTACK`, `DCA_DECAY`, `DCA_SUSTAIN`, `DCA_RELEASE` | Float | `0.0s - 10.0s` (Sustain `0.0 - 1.0`)| `0.0s` (Sustain `1.0`) | `sliderValueChanged` / APVTS |
| `juce::Slider` | Envolventes DCW | `DCW_ATTACK`, `DCW_DECAY`, `DCW_SUSTAIN`, `DCW_RELEASE` | Float | `0.0s - 10.0s` (Sustain `0.0 - 1.0`)| `0.0s` (Sustain `1.0`) | `sliderValueChanged` / APVTS |

---

## 4. Modulation Matrix Section (`ModulationMatrixSection`)
*Configuración de ruteo de modulación, fuentes físicas y seguimiento de teclado (Key Follow).*

| Clase Componente | Control UI (ID C++) | ID Parámetro APVTS | Tipo de Dato | Rango / Opciones | Valor por Defecto | Retrollamada / Sincronización |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `Knob` (`juce::Slider`) | `veloToDcwKnob` | `MOD_VELO_DCW` | Float | `0.0` a `1.0` | `0.0` | `sliderValueChanged` / APVTS |
| `Knob` (`juce::Slider`) | `veloToDcaKnob` | `MOD_VELO_DCA` | Float | `0.0` a `1.0` | `1.0` | `sliderValueChanged` / APVTS |
| `Knob` (`juce::Slider`) | `wheelToVibKnob` | `MOD_WHEEL_VIB` | Float | `0.0` a `1.0` | `0.0` | `sliderValueChanged` / APVTS |
| `juce::ComboBox` | `keyFollowDcoSelector` | `KEY_FOLLOW_DCO` | Choice | OFF, FIX, VAR | `2` (VAR) | `comboBoxChanged` / APVTS |
| `juce::ComboBox` | `keyFollowDcwSelector` | `KEY_FOLLOW_DCW` | Choice | OFF, FIX, VAR | `0` (OFF) | `comboBoxChanged` / APVTS |
| `juce::ComboBox` | `keyFollowDcaSelector` | `KEY_FOLLOW_DCA` | Choice | OFF, FIX, VAR | `0` (OFF) | `comboBoxChanged` / APVTS |

---

## 5. Arpeggiator Section (`ArpeggiatorSection`)
*Configuración del arpegiador integrado en hardware o modulación temporal.*

| Clase Componente | Control UI (ID C++) | ID Parámetro APVTS | Tipo de Dato | Rango / Opciones | Valor por Defecto | Retrollamada / Sincronización |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `juce::TextButton` | `arpOnOffButton` | `ARP_ENABLED` | Boolean | `true` (On) / `false` (Off) | `false` | `buttonClicked` / APVTS |
| `juce::TextButton` | `arpLatchButton` | `ARP_LATCH` | Boolean | `true` (On) / `false` (Off) | `false` | `buttonClicked` / APVTS |
| `juce::ComboBox` | `arpRateSelector` | `ARP_RATE` | Choice | 1/4, 1/8, 1/16, 1/32 | `2` (1/16) | `comboBoxChanged` / APVTS |
| `Knob` (`juce::Slider`) | `arpBpmKnob` | `ARP_BPM` | Float | `40.0` a `240.0` (BPM) | `120.0` | `sliderValueChanged` / APVTS |
| `Knob` (`juce::Slider`) | `arpGateKnob` | `ARP_GATE` | Float | `0.0` a `1.0` | `0.5` | `sliderValueChanged` / APVTS |
| `juce::ComboBox` | `arpPatternSelector`| `ARP_PATTERN` | Choice | Up, Down, Up/Down, Random, As Played | `0` (Up) | `comboBoxChanged` / APVTS |
| `juce::ComboBox` | `arpOctaveSelector` | `ARP_OCTAVE` | Choice/Int | 1, 2, 3, 4 | `1` (1 Octave) | `comboBoxChanged` / APVTS |

---

## 6. General Settings Section (`GeneralSection`)
*Ajustes globales y calibración del sintetizador.*

| Clase Componente | Control UI (ID C++) | ID Parámetro APVTS | Tipo de Dato | Rango / Opciones | Valor por Defecto | Retrollamada / Sincronización |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `juce::ComboBox` | `opModeSelector` | `OPERATION_MODE` | Choice | Classic 101, Classic 5000, Modern | `0` (Classic 101) | `comboBoxChanged` / APVTS |
| `juce::ComboBox` | `oversamplingSelector`| `OVERSAMPLING_QUALITY`| Choice| 1x (Eco), 2x (High), 4x (Ultra)| `0` (1x) | `comboBoxChanged` / APVTS |
| `Knob` (`juce::Slider`) | `volumeKnob` | `MASTER_VOLUME` | Float | `0.0` a `1.0` | `1.0` | `sliderValueChanged` / APVTS |
| `juce::ComboBox` | `midiChannelSelector`| `MIDI_CH` | Choice/Int | Channels 1 to 16 | `1` | `comboBoxChanged` / APVTS |
| `Knob` (`juce::Slider`) | `masterTuneKnob` | `MASTER_TUNE` | Float | `-50.0` a `50.0` (cents) | `0.0` | `sliderValueChanged` / APVTS |
| `Knob` (`juce::Slider`) | `benderRangeKnob` | `PITCH_BEND_RANGE`| Int | `0` a `12` (semitonos) | `2` | `sliderValueChanged` / APVTS |
| `juce::TextButton` | `protectSwitchButton`| `PROTECT_SWITCH` | Boolean | `true` (On) / `false` (Off) | `true` | `buttonClicked` / APVTS |
| `juce::TextButton` | `hardwareNoiseButton`| `HARDWARE_NOISE` | Boolean | `true` (On) / `false` (Off) | `true` | `buttonClicked` / APVTS |

---

## 7. Global Controls & Overlays (`AudioProcessorEditor` Header / Root)
*Menús de navegación globales, botones del LCD y paneles flotantes.*

| Clase Componente | Control UI (ID C++) | Conexión / Acción del Botón |
| :--- | :--- | :--- |
| `juce::TextButton` | `randomButton` | Gatilla `randomizePatch()` para barajar los parámetros de síntesis de forma aleatoria. |
| `juce::TextButton` | `panicButton` | Envía una señal "All Notes Off" al motor de voces para silenciarlo instantáneamente. |
| `juce::TextButton` | `compareButton` | Activa/Desactiva `setCompareMode()` para contrastar la edición actual con la guardada. |
| `juce::TextButton` | `cursorLeft` / `cursorRight` | Navegación horizontal en el menú del display LCD simulado. |
| `juce::TextButton` | `cursorUp` / `cursorDown` | Modificación de valores o navegación vertical en el LCD simulado. |
| `juce::TabbedComponent`| `envelopeTabs` | Pestañas visuales para alternar entre DCA, DCW y Pitch Envelopes (Líneas 1 y 2). |
| `juce::TabbedComponent`| `mainTabs` | Pestañas superiores para alternar entre paneles principales del sintetizador. |
| `juce::MidiKeyboardComponent`| `keyboardComponent`| Recibe pulsaciones de ratón / MIDI y desencadena `handleNoteOn`/`handleNoteOff` en el motor. |
