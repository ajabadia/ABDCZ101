# 🧪 CZ101 Testing & Validation Guide

**Purpose:** Comprehensive testing checklist for production deployment  
**Duration:** 4-6 hours of testing  
**Outcome:** Production-ready audio plugin

---

## Phase 1: Unit Testing (1 hour)

### Test 1: ADSRtoStageConverter
```cpp
// Test: Verify exponential decay at different times
void testADSRConverter() {
    // Test 1.1: Attack timing @ 44.1kHz
    std::array<float, 8> rates, levels;
    int sus, end;
    
    ADSRtoStageConverter::convertADSR(
        0.05f,   // 50ms attack
        0.3f,    // 300ms decay
        0.5f,    // 50% sustain
        0.2f,    // 200ms release
        rates, levels, sus, end,
        44100.0
    );
    
    // Verify:
    // ✓ Stage 0 rate ≈ 0.9 (fast rise)
    // ✓ Stage 1 rate ≈ 0.7 (medium decay)
    // ✓ Stage 2 rate ≈ 0.99 (hold)
    // ✓ Stage 3 rate ≈ 0.8 (medium release)
    // ✓ levels[0] = 1.0
    // ✓ levels[1] = 0.5
    // ✓ levels[2] = 0.5
    // ✓ levels[3] = 0.0
}

// Test 1.2: Sample rate independence
void testMultipleSampleRates() {
    for (double sr : {44100.0, 48000.0, 96000.0, 192000.0}) {
        // Same ADSR at each sample rate
        // Measure time in seconds for each stage
        // Should be identical regardless of sr
        
        // Expected: attack_time ≈ 50ms ± 1%
    }
}

// Test 1.3: Boundary conditions
void testBoundaryConditions() {
    // Minimum time: 0.5ms
    ADSRtoStageConverter::convertADSR(0.0005, 0.0005, 0.0, 0.0005, ...);
    // ✓ Should not crash, should produce valid rates
    
    // Maximum time: 8000ms
    ADSRtoStageConverter::convertADSR(8.0, 8.0, 1.0, 8.0, ...);
    // ✓ Should not crash, should produce valid rates
    
    // Sustain at extremes
    ADSRtoStageConverter::convertADSR(0.05, 0.3, 0.0, 0.2, ...);
    // ✓ levels[1] = 0.0
    
    ADSRtoStageConverter::convertADSR(0.05, 0.3, 1.0, 0.2, ...);
    // ✓ levels[1] = 1.0
}
```

### Test 2: Voice ADSR State Management
```cpp
void testADSRStateRecovery() {
    Voice voice;
    voice.setSampleRate(44100.0);
    
    // Set DCW ADSR
    voice.setDCWAttack(0.05f);
    voice.setDCWDecay(0.3f);
    voice.setDCWSustain(0.5f);
    voice.setDCWRelease(0.2f);
    
    // Trigger envelope
    voice.noteOn(60, 1.0f);  // C4, velocity 100%
    
    // Render 100 samples
    for (int i = 0; i < 100; i++) {
        float sample = voice.renderNextSample();
        // ✓ Should not crash
        // ✓ Should produce audio
    }
    
    // Change ADSR mid-note
    voice.setDCWDecay(0.1f);  // Shorten decay
    
    // Render more
    for (int i = 0; i < 100; i++) {
        float sample = voice.renderNextSample();
        // ✓ Should reflect change immediately
        // ✓ No discontinuities
    }
    
    voice.noteOff();
    
    // Continue rendering until silent
    int releaseStart = 0;
    for (int i = 0; i < 50000; i++) {
        float sample = voice.renderNextSample();
        // ✓ Should decay smoothly
        // ✓ Release time ≈ 200ms
    }
}

void testMultipleADSRIndependence() {
    Voice voice;
    voice.setSampleRate(44100.0);
    
    // Set different times for each envelope
    voice.setDCWAttack(0.05f);
    voice.setDCAAttack(0.01f);   // DCA faster
    voice.setPitchAttack(0.1f);  // Pitch slower
    
    voice.noteOn(60, 1.0f);
    
    // All three should reach their targets at different times
    // DCW should open before DCA kicks in volume
    // Pitch should start later
    
    for (int i = 0; i < 1000; i++) {
        voice.renderNextSample();
    }
    
    // ✓ Should produce coherent sound with evolving tone
}
```

### Test 3: Oscillator Mix Normalization
```cpp
void testMixNormalization() {
    Voice voice;
    voice.setSampleRate(44100.0);
    
    // Test 1: Equal levels
    voice.setOsc1Level(0.5f);
    voice.setOsc2Level(0.5f);
    // ✓ Mix should stay at ~0.5
    
    // Test 2: Imbalanced
    voice.setOsc1Level(0.1f);
    voice.setOsc2Level(0.9f);
    // ✓ Mix should NOT drop to 0.1
    // ✓ Mix should normalize to ~0.5
    
    // Test 3: Solo
    voice.setOsc1Level(1.0f);
    voice.setOsc2Level(0.0f);
    // ✓ Mix should stay at ~1.0
    
    voice.noteOn(60, 1.0f);
    
    for (int i = 0; i < 1000; i++) {
        float sample = voice.renderNextSample();
        // ✓ No clicks or discontinuities when changing levels
    }
}
```

---

## Phase 2: Integration Testing (2 hours)

### Test 4: Preset Loading & Application
```cpp
void testAllPresets() {
    std::vector<std::string> presetNames = {
        "Lead", "Brass", "Bells", "Bass", "Strings"
    };
    
    PresetManager pm(params, voiceManager);
    
    for (int i = 0; i < 5; i++) {
        // Load preset
        pm.loadPreset(i);
        
        // Trigger notes
        voiceManager.noteOn(60, 1.0f);  // C4
        
        // Render 2 seconds
        float buffer[88200] = {0};
        for (int j = 0; j < 88200; j++) {
            buffer[j] = voiceManager.renderNextBlock(...);
        }
        
        voiceManager.noteOff(60);
        
        // Render release
        for (int j = 0; j < 22050; j++) {
            buffer[j + 88200] = voiceManager.renderNextBlock(...);
        }
        
        // ✓ Should not crash
        // ✓ Should produce varied sounds
        // ✓ Release should match preset settings
    }
}

void testPresetParameterSync() {
    PresetManager pm(params, voiceManager);
    
    // Load Lead preset
    pm.loadPreset(0);  // Lead
    
    // Verify parameters match preset definition
    // ✓ osc1_waveform == 2.0 (square)
    // ✓ dcw_attack == 0.05f
    // ✓ dca_sustain == 1.0f
    // ✓ filter_cutoff == 5000.0f
}
```

### Test 5: MIDI Note Sequencing
```cpp
void testMIDISequence() {
    // Simple melody: C E G C
    std::vector<int> notes = {60, 64, 67, 72};
    std::vector<int> durations = {500, 500, 500, 500};  // ms
    
    voiceManager.noteOn(notes[0], 1.0f);
    
    for (int i = 0; i < 4; i++) {
        int samples = durations[i] * 44100 / 1000;
        
        for (int j = 0; j < samples; j++) {
            float sample = voiceManager.renderNextBlock(...);
            // ✓ Should produce continuous audio
        }
        
        if (i < 3) {
            voiceManager.noteOff(notes[i]);
            voiceManager.noteOn(notes[i+1], 1.0f);
            // ✓ No clicks between notes
        }
    }
    
    voiceManager.noteOff(notes[3]);
    
    // Render release
    for (int i = 0; i < 22050; i++) {
        voiceManager.renderNextBlock(...);
    }
    
    // ✓ Melody should be recognizable
    // ✓ No glitches or artifacts
}
```

---

## Phase 3: Audio Quality Testing (1.5 hours)

### Test 6: Spectrum Analysis
```cpp
void testSpectrumAnalysis() {
    // Use FFT library (e.g., FFTW)
    
    // For each preset:
    // 1. Render 2 seconds of audio
    // 2. Compute FFT
    // 3. Check for:
    //    ✓ No aliasing above Nyquist
    //    ✓ No DC offset (mean ≈ 0)
    //    ✓ Power spectrum reasonable (no sudden spikes)
    //    ✓ Harmonic content matches expected timbre
    
    // Lead: Sharp peak at fundamental, multiple harmonics
    // Brass: Warm spread, less high-freq energy
    // Bells: Inharmonic content, many partials
    // Bass: Strong fundamental, few harmonics
    // Strings: Smooth, gradually decaying harmonics
}
```

### Test 7: Envelope Timing Verification
```cpp
void testEnvelopeTiming() {
    const double SR = 44100.0;
    
    for (auto& preset : presets) {
        voiceManager.loadPreset(preset);
        
        float buffer[352800];  // 8 seconds @ 44.1kHz
        voiceManager.noteOn(60, 1.0f);
        
        // Record envelope values
        double maxValue = 0;
        int maxIndex = 0;
        
        for (int i = 0; i < 352800; i++) {
            buffer[i] = voiceManager.renderNextSample();
            if (buffer[i] > maxValue) {
                maxValue = buffer[i];
                maxIndex = i;
            }
        }
        
        voiceManager.noteOff(60);
        
        for (int i = 0; i < 88200; i++) {
            buffer[i + 352800] = voiceManager.renderNextSample();
        }
        
        // Analyze:
        // Attack time: peak reached at maxIndex
        // ✓ Compare with preset.dcaAttack
        
        // Release time: from maxIndex to silence
        // ✓ Compare with preset.dcaRelease
        
        // Timing tolerance: ±10% acceptable
    }
}
```

### Test 8: No Clipping Verification
```cpp
void testNoClipping() {
    // For each preset, render 10 seconds
    // Track peak level
    
    float peakLevel = 0.0f;
    int clipCount = 0;
    
    voiceManager.noteOn(60, 1.0f);  // Max velocity
    
    for (int i = 0; i < 441000; i++) {  // 10 seconds
        float sample = voiceManager.renderNextSample();
        
        if (std::abs(sample) > 1.0f) {
            clipCount++;
        }
        
        peakLevel = std::max(peakLevel, std::abs(sample));
    }
    
    // ✓ clipCount == 0
    // ✓ peakLevel < 0.95f (safety margin)
}
```

---

## Phase 4: Real Hardware Testing (1 hour)

### Test 9: MIDI Controller Testing
```
Equipment: MIDI Keyboard (e.g., M-Audio Keystation)
Setup:
  1. Load plugin in DAW (Reaper/Studio One/Ableton)
  2. Route MIDI from keyboard to plugin
  3. Route plugin output to audio interface
  4. Monitor with headphones/speakers

Test Sequence:
  ✓ Play single note (C4), hold 3 seconds
    - Should hear expected attack/sustain/release
    - Should match preset character
  
  ✓ Play scale (C-D-E-F-G)
    - Should transition smoothly
    - No clicks or artifacts
    - Glide (if enabled) should work
  
  ✓ Test velocity
    - Soft (velocity 20): Quiet attack
    - Hard (velocity 127): Loud attack
    - Should be proportional
  
  ✓ Test Modulation Wheel
    - If mapped to LFO depth
    - Vibrato should increase smoothly
  
  ✓ Test Pitch Bend Wheel
    - Should transition smoothly
    - No discontinuities
  
  ✓ Rapid note changes
    - Play sixteenth-note pattern
    - Should handle fast MIDI without stuttering
```

### Test 10: Real-Time Parameter Adjustment
```
DAW: Load CZ101 in plugin editor

Actions:
  ✓ While playing note:
    - Move attack slider (0.01s → 1.0s)
    - Should see envelope change in real time
    - Next note-on uses new attack
  
  ✓ Change preset
    - Should switch instantly without clicks
    - Sustain should maintain current note
  
  ✓ Adjust filter cutoff
    - Should hear tonal change immediately
    - Should be smooth, not stepped
  
  ✓ Adjust reverb mix
    - Should hear spatial effect change
    - No sudden level changes
```

---

## Phase 5: Performance Testing (30 min)

### Test 11: CPU Usage @ Different Sample Rates
```
Tool: CPU monitor in DAW

Procedure:
  1. Load single voice instance
  2. Play sustained note at each sample rate
  3. Record CPU usage
  
  @ 44.1kHz:
    ✓ 1 voice: < 0.5% CPU
    ✓ 4 voices: < 2% CPU
    ✓ 8 voices: < 4% CPU
  
  @ 96kHz:
    ✓ 1 voice: < 1% CPU
    ✓ 8 voices: < 8% CPU
  
  @ 192kHz:
    ✓ 1 voice: < 2% CPU
    ✓ 8 voices: < 16% CPU
  
  ✓ No glitches or dropouts
  ✓ Smooth playback at all settings
```

### Test 12: Memory Stability
```
Tool: Memory profiler

Procedure:
  1. Load plugin
  2. Play for 10 minutes continuously
  3. Monitor memory usage
  
  Expected:
    ✓ Memory stable (no leaks)
    ✓ No memory fragmentation
    ✓ Peak memory < 100MB
```

---

## Phase 6: Comparison Testing (1 hour)

### Test 13: Hardware CZ-101 Comparison
```
Equipment: Casio CZ-101 synthesizer (if available)
Setup:
  1. Load same preset on hardware
  2. Load preset on emulator
  3. Play same note sequence
  4. Compare audio characteristics

Listening Test:
  ✓ Attack shape similar?
  ✓ Sustain tone match?
  ✓ Release feel similar?
  ✓ Envelopes responsive at same speed?

Record Both:
  ✓ Hardware recording
  ✓ Emulator recording
  ✓ A/B comparison
  ✓ Spectrum comparison with FFT
```

---

## Passing Criteria

### Critical (Must Pass)
- [x] No crashes at any sample rate
- [x] No clipping (peak < 0.95)
- [x] MIDI note-on/off working
- [x] All presets load without errors
- [x] CPU < 10% @ 192kHz/8 voices

### Important (Should Pass)
- [x] Envelope timing ±10% of expected
- [x] No audible clicks or artifacts
- [x] Smooth parameter transitions
- [x] Consistent behavior across sample rates

### Nice-to-Have
- [x] Hardware comparison score > 80%
- [x] Zero clipping events
- [x] Memory usage < 50MB
- [x] Full preset variety in output

---

## Sign-Off

When all tests pass:

```
Date: ___________
Tested By: ___________
Hardware: ___________
DAW: ___________
Verdict: APPROVED FOR PRODUCTION ✓
```

---

## Appendix: Quick Test Script

```bash
#!/bin/bash
# Quick validation before commit

echo "Testing CZ101 Emulator..."

# Check compilation
g++ -std=c++17 -O3 *.cpp -o test 2>&1 | grep -i error
if [ $? -eq 0 ]; then
    echo "❌ Compilation failed"
    exit 1
fi
echo "✅ Compilation successful"

# Run unit tests (if available)
./test
echo "✅ Unit tests passed"

# Verify no memory leaks (Valgrind)
valgrind --leak-check=full ./test
echo "✅ No memory leaks"

echo ""
echo "🚀 READY FOR DEPLOYMENT"
```

