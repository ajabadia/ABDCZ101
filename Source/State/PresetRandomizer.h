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
        p.parameters["LINE_SELECT"] = (float)rng.nextInt(4) / 3.0f; // 0, 0.33, 0.66, 1
        p.parameters["OSC1_WAVEFORM"] = rng.nextFloat();
        p.parameters["OSC1_WAVEFORM2"] = (rng.nextFloat() > 0.7f) ? rng.nextFloat() : 0.0f; // 30% chance of 2nd wave
        p.parameters["OSC1_LEVEL"] = 1.0f; // Always full for line 1?
        
        p.parameters["OSC2_WAVEFORM"] = rng.nextFloat();
        p.parameters["OSC2_WAVEFORM2"] = (rng.nextFloat() > 0.7f) ? rng.nextFloat() : 0.0f;
        p.parameters["OSC2_DETUNE"] = 0.5f + (rng.nextFloat() - 0.5f) * 0.1f; // Slight detune (0.5 is center)

        p.parameters["RING_MOD"] = (rng.nextFloat() > 0.8f) ? 1.0f : 0.0f;
        p.parameters["NOISE_MOD"] = (rng.nextFloat() > 0.9f) ? 1.0f : 0.0f; // If exists

        // 2. Envelopes
        randomizeEnvelope(p.dcaEnv, rng, true); // DCA needs 0 at end
        randomizeEnvelope(p.dcwEnv, rng, false);
        randomizeEnvelope(p.pitchEnv, rng, false);
        
        randomizeEnvelope(p.dcaEnv2, rng, true);
        randomizeEnvelope(p.dcwEnv2, rng, false);
        randomizeEnvelope(p.pitchEnv2, rng, false);
        
        // 3. LFO
        p.parameters["LFO_WAVE"] = rng.nextFloat();
        p.parameters["LFO_RATE"] = rng.nextFloat();
        p.parameters["LFO_DEPTH"] = rng.nextFloat() * 0.5f; // Don't go too crazy
        p.parameters["LFO_DELAY"] = rng.nextFloat() * 0.5f;
        
        // 4. Effects
        p.parameters["CHORUS_MIX"] = (rng.nextFloat() > 0.5f) ? rng.nextFloat() * 0.5f : 0.0f;
        p.parameters["DELAY_MIX"] = (rng.nextFloat() > 0.7f) ? rng.nextFloat() * 0.4f : 0.0f;

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
