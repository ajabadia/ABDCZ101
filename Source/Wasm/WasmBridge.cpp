// ─── WASM bridge: audio core ───
// wasm_init / wasm_process / wasm_note_on / wasm_note_off / wasm_midi_message.
// The rest of the old monolithic WasmBridge.cpp now lives in focused modules
// that share state through WasmState.h:
//   - WasmParams.cpp     parameter dispatch (wasm_set/get_param, updateWasmDSP)
//   - WasmPresets.cpp    preset CRUD (load/save/rename/delete/move)
//   - WasmBank.cpp       bank JSON + SysEx import/export
//   - WasmEnvelopes.cpp  envelope get/set accessors
//   - WasmMacros.cpp     Modern Brilliance/Tone/Space + matrix/arp gating

#include "WasmState.h"
#include "../DSP/Effects/EffectsChain.h"
#include "../Core/HardwareConstants.h"
#include <cmath>
#include <cstdint>

extern "C" {

EMSCRIPTEN_KEEPALIVE void wasm_init(double sampleRate) {
    double sr = sampleRate > 8000.0 ? sampleRate : 44100.0;
    gVoiceManager.setSampleRate(sr);

    initWasmPresets(); // Decode Casio CZ ROM into gPresets


    // Initialize default modulation matrix values for WASM to match C++ defaults
    gModulationMatrix.kfDco = 2; // VAR (full key tracking)
    gModulationMatrix.kfDcw = 0;
    gModulationMatrix.kfDca = 0;
    gModulationMatrix.keyTrackPitch = 1.0f;
    gModulationMatrix.keyTrackDcw = 0.0f; // Matches APVTS default (Parameters.cpp)
    gModulationMatrix.veloToDca = 1.0f;
    gVoiceManager.setModulationMatrix(gModulationMatrix);

    // Prepare the effects chain (same spec shape as PluginProcessor::prepareToPlay)
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = 512; // Worklet processes 128-sample blocks; 512 is a safe ceiling
    spec.numChannels = 2;
    gEffectsChain.prepare(spec);
    gEffectsChain.reset();

    // Wire the MIDI processor. CC-mapped parameters (mod wheel, volume, drive,
    // filter CC71/74, ...) are routed back through wasm_set_param so they drive
    // the DSP exactly like the native host does via the APVTS.
    getMidiProcessor().setParamChangeCallback([](const char* paramId, float value) {
        if (paramId != nullptr) wasm_set_param(paramId, value);
    });
    // Defaults from the APVTS (Parameters.cpp): channel 1, bend range 2 st.
    getMidiProcessor().setMidiChannel(1);
    getMidiProcessor().setPitchBendRange(2);

    // Seed snapshot defaults to match the APVTS (Parameters.cpp) so the first
    // block before any set_param already behaves like the native plugin.
    gWasmSnapshot.system.opMode = 0; // Classic 101
    applyTone();
    applyMacroSpace();
    applyBrilliance();
    applyArpState();
    // hardwareNoise / chorusOn: kept for snapshot parity with PluginProcessor,
    // though the WASM path drives hardware noise via the VoiceManager setter and
    // EffectsChain ignores chorusOn (chorus runs on its mix in every mode).
    gWasmSnapshot.system.hardwareNoise = true; // APVTS default (Parameters.cpp)
    gWasmSnapshot.effects.chorusOn = true;
    gWasmSnapshot.effects.chorusRate = 0.5f;
    gWasmSnapshot.effects.chorusDepth = 0.2f;
    gWasmSnapshot.effects.chorusMix = 0.3f;
    gWasmSnapshot.effects.delayTime = 0.5f;
    gWasmSnapshot.effects.delayFb = 0.3f;
    gWasmSnapshot.effects.delayMix = 0.0f;
    gWasmSnapshot.effects.reverbSize = 0.5f;
    gWasmSnapshot.effects.reverbMix = 0.2f;
    gWasmSnapshot.effects.driveAmount = 0.0f;
    gWasmSnapshot.effects.driveColor = 0.5f;
    gWasmSnapshot.effects.driveMix = 0.0f;
    gWasmSnapshot.effects.lpfCutoff = 20000.0f;
    gWasmSnapshot.effects.lpfReso = 0.0f;
    gWasmSnapshot.effects.hpfCutoff = 20.0f;
    gEffectsPrepared = true;

    updateWasmDSP();
    gInitialized = true;
}

EMSCRIPTEN_KEEPALIVE void wasm_process(float* outputL, float* outputR, int numSamples) {
    if (!gInitialized || numSamples <= 0 || outputL == nullptr || outputR == nullptr) return;
    gVoiceManager.renderNextBlock(outputL, outputR, numSamples);

    // Apply the effects chain (Drive -> Chorus -> Delay -> Reverb), mirroring
    // PluginProcessor::processBlock. The snapshot holds the effect parameters.
    // NOTE: wasm_process / wasm_set_param run single-threaded on the worklet's
    // audio rendering thread, so gWasmSnapshot needs no locking.
    if (gEffectsPrepared)
    {
        float* channelData[2] = { outputL, outputR };
        juce::AudioBuffer<float> buffer(channelData, 2, numSamples);
        gEffectsChain.process(buffer, gWasmSnapshot);
        
        static float currentMasterGain = 0.0f;
        float rawMasterVol = gWasmSnapshot.system.masterVol;
        float targetGain = (rawMasterVol / 10.0f) * CZ101::Core::HardwareConstants::MASTER_HEADROOM_GAIN;
        
        buffer.applyGainRamp(0, buffer.getNumSamples(), currentMasterGain, targetGain);
        buffer.applyGainRamp(1, buffer.getNumSamples(), currentMasterGain, targetGain);
        currentMasterGain = targetGain;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            float* channelDataPtr = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                 channelDataPtr[i] = std::clamp(channelDataPtr[i], -0.99f, 0.99f);
            }
        }
    }
}

EMSCRIPTEN_KEEPALIVE void wasm_note_on(int note, float velocity) {
    if (!gInitialized) return;
    gVoiceManager.noteOn(note + gKeyTranspose + gOctave * 12, std::clamp(velocity, 0.0f, 1.0f));
}

EMSCRIPTEN_KEEPALIVE void wasm_note_off(int note) {
    if (!gInitialized) return;
    gVoiceManager.noteOff(note + gKeyTranspose + gOctave * 12);
}

EMSCRIPTEN_KEEPALIVE void wasm_midi_message(const uint8_t* data, int length) {
    if (!gInitialized || !data || length <= 0) return;

    // Parse raw MIDI messages and route them through the MIDI processor
    // (channel filter via MIDI_CH, pitch bend range via PITCH_BEND_RANGE,
    // sustain pedal, CC->param mapping). Built with the per-message
    // juce::MidiMessage factories because this JUCE version does not expose
    // MidiMessage::createFromBytes.
    //
    // Input contract: complete, self-contained messages as delivered by the
    // Web MIDI API (no running status). A data byte without a preceding
    // status is dropped.
    int i = 0;
    while (i < length) {
        const uint8_t byte = data[i];

        // SysEx: skip the chunk (the standalone engine has no SysEx consumer;
        // MIDIProcessor routes SysEx to SysExManager which is null in WASM).
        if (byte == 0xF0) {
            while (i < length && data[i] != 0xF7) i += 1;
            i += 1; // skip the terminating F7 (or the last byte of the chunk)
            continue;
        }
        // System realtime / common messages (clock, start, stop, ...): ignore.
        if (byte >= 0xF8) { i += 1; continue; }

        // Data byte without a status: running status is not supported.
        if ((byte & 0x80) == 0) { i += 1; continue; }

        const int type = byte & 0xF0;
        const int channel = (byte & 0x0F) + 1;

        if (type == 0xC0 || type == 0xD0) { // program change / channel pressure: 1 data byte
            if (i + 1 >= length) break;
            const uint8_t d1 = data[i + 1];
            i += 2;
            if (type == 0xC0)
                getMidiProcessor().processMidiMessage(juce::MidiMessage::programChange(channel, d1));
            else
                getMidiProcessor().processMidiMessage(juce::MidiMessage::channelPressureChange(channel, d1));
            continue;
        }

        // Note/CC/aftertouch/pitch-bend: 2 data bytes.
        if (i + 2 >= length) break;
        const uint8_t d1 = data[i + 1];
        const uint8_t d2 = data[i + 2];
        i += 3;

        switch (type)
        {
            case 0x80:
                getMidiProcessor().processMidiMessage(juce::MidiMessage::noteOff(channel, d1, d2 / 127.0f));
                break;
            case 0x90:
                if (d2 == 0)
                    getMidiProcessor().processMidiMessage(juce::MidiMessage::noteOff(channel, d1));
                else
                    getMidiProcessor().processMidiMessage(juce::MidiMessage::noteOn(channel, d1, d2 / 127.0f));
                break;
            case 0xA0:
                getMidiProcessor().processMidiMessage(juce::MidiMessage::aftertouchChange(channel, d1, d2));
                break;
            case 0xB0:
                getMidiProcessor().processMidiMessage(juce::MidiMessage::controllerEvent(channel, d1, d2));
                break;
            case 0xE0: {
                const int bend = (int)d1 | ((int)d2 << 7);
                getMidiProcessor().processMidiMessage(juce::MidiMessage::pitchWheel(channel, bend));
                break;
            }
            default:
                break;
        }
    }
}

} // extern "C"
