#pragma once

#include "PresetManager.h"
#include <juce_core/juce_core.h>

namespace CZ101 {
namespace State {

class PresetRandomizer
{
public:
    static Preset generateRandomPreset(const std::string& name = "Random Patch")
    {
        Preset p(name);
        p.author = "Randomizer";
        
        juce::Random rng;
        
        // 1. Oscillators
        // LINE_SELECT: 0-3 (Choice Index)
        p.parameters["LINE_SELECT"] = (float)rng.nextInt(4); 
        
        // WAVEFORM: 0-7 (Choice Index)
        p.parameters["OSC1_WAVEFORM"] = (float)rng.nextInt(8);
        p.parameters["OSC1_WAVEFORM2"] = (rng.nextFloat() > 0.7f) ? (float)rng.nextInt(8) : 0.0f;
        p.parameters["OSC1_LEVEL"] = 0.8f + rng.nextFloat() * 0.2f; // High level
        
        p.parameters["OSC2_WAVEFORM"] = (float)rng.nextInt(8);
        p.parameters["OSC2_WAVEFORM2"] = (rng.nextFloat() > 0.7f) ? (float)rng.nextInt(8) : 0.0f;
        p.parameters["OSC2_LEVEL"] = rng.nextFloat(); 
        p.parameters["OSC2_DETUNE"] = (rng.nextFloat() - 0.5f) * 10.0f; // +/- 5 cents approx (Detune parameter is +/- 12 semitones? No, wait.)
        // OSC2_DETUNE in Parameters.cpp is -12.0 to 12.0 semitones.
        // We want subtle detune. 
        p.parameters["OSC2_DETUNE"] = (rng.nextFloat() - 0.5f) * 0.2f; // +/- 0.1 semitones
        
        p.parameters["RING_MOD"] = (rng.nextFloat() > 0.8f) ? 1.0f : 0.0f;
        // NOISE_MOD? Not in Parameters.cpp. Check HARDWARE_NOISE? That's global.
        
        // 2. Envelopes
        randomizeEnvelope(p.dcaEnv, rng, true); 
        randomizeEnvelope(p.dcwEnv, rng, false);
        // Randomize pitch? Maybe Keep it simple
        if (rng.nextFloat() > 0.8f) randomizeEnvelope(p.pitchEnv, rng, false);
        else {
             // Flat Pitch
             for (int i=0; i<8; ++i) { p.pitchEnv.rates[i]=0.5f; p.pitchEnv.levels[i]=0.5f; }
             p.pitchEnv.sustainPoint = 0; p.pitchEnv.endPoint = 1;
        }
        
        // Copy to Line 2 for consistency if select is 1+2
        p.dcaEnv2 = p.dcaEnv;
        p.dcwEnv2 = p.dcwEnv;
        p.pitchEnv2 = p.pitchEnv;
        
        // 3. LFO
        p.parameters["LFO_WAVE"] = (float)rng.nextInt(4); // 0-3
        p.parameters["LFO_RATE"] = 0.5f + rng.nextFloat() * 8.0f; // 0.5Hz to 8.5Hz
        p.parameters["LFO_DEPTH"] = rng.nextFloat() * 0.3f; // Subtle
        p.parameters["LFO_DELAY"] = rng.nextFloat() * 0.5f; // 0-0.5s
        
        // 4. Effects
        p.parameters["CHORUS_MIX"] = (rng.nextFloat() > 0.7f) ? rng.nextFloat() * 0.5f : 0.0f;
        p.parameters["DELAY_MIX"] = (rng.nextFloat() > 0.8f) ? rng.nextFloat() * 0.4f : 0.0f;
        p.parameters["DELAY_TIME"] = 0.2f + rng.nextFloat() * 0.5f;
        p.parameters["DELAY_FEEDBACK"] = 0.3f;

        // 5. Arpeggiator (Audit Fix)
        // Enable with low probability to not annoy user immediately, but randomize settings
        p.parameters["ARP_ENABLED"] = (rng.nextFloat() > 0.8f) ? 1.0f : 0.0f; 
        p.parameters["ARP_LATCH"] = (rng.nextFloat() > 0.7f) ? 1.0f : 0.0f;
        p.parameters["ARP_RATE"] = (float)rng.nextInt(4); // 0-3
        p.parameters["ARP_BPM"] = 80.0f + rng.nextFloat() * 60.0f; // 80-140 Practical range
        p.parameters["ARP_GATE"] = 0.3f + rng.nextFloat() * 0.7f; // Usable gate
        p.parameters["ARP_SWING"] = (rng.nextFloat() > 0.7f) ? rng.nextFloat() * 0.5f : 0.0f;
        p.parameters["ARP_SWING_MODE"] = (float)rng.nextInt(3);
        p.parameters["ARP_PATTERN"] = (float)rng.nextInt(5);
        p.parameters["ARP_OCTAVE"] = (float)(1 + rng.nextInt(3)); // 1-3 useful range
        
        return p;
    }
    
private:
    static void randomizeEnvelope(EnvelopeData& env, juce::Random& rng, bool isDCA)
    {
        // Randomize number of active stages (3 to 6 usually musical)
        int numStages = 3 + rng.nextInt(4); // 3-6
        
        for (int i=0; i<8; ++i) {
            env.rates[i] = 0.3f + rng.nextFloat() * 0.7f; // Bias towards faster rates
            if (i == 0) env.rates[i] = 0.6f + rng.nextFloat() * 0.4f; // Attack fast usually
            
            env.levels[i] = rng.nextFloat();
        }
        
        // DCA constraints
        if (isDCA) {
            env.levels[numStages] = 0.0f; // End point must be silence
            env.endPoint = numStages;
            if (numStages > 0) env.sustainPoint = numStages - 1;
            else env.sustainPoint = 0;
            
            // Randomly no sustain (pluck)
            if (rng.nextFloat() > 0.7f) env.sustainPoint = -1; 
        } else {
            // DCW/Pitch can end anywhere, usually 0 or 0.5
             env.endPoint = numStages;
             env.sustainPoint = (numStages > 0) ? numStages - 1 : 0;
        }
    }
};

}
}
