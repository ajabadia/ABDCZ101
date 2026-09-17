// Patch randomizer for the WebUI — parity with the native
// CZ101::State::PresetRandomizer (Source/State/PresetRandomizer.h).
//
// The native generator returns RAW parameter values plus EnvelopeData. The
// WebUI keeps its own envelope editor state and uses the parameter registry
// (PARAM_MAP) for ranges/choices, so this module returns:
//   - `params`: paramId -> RAW value (same values/choices/ranges as the native
//     generator), ready to be pushed through rawToNormalized + applyParameterToUI
//   - `envelopes`: { dca, dcw, pitch } x { line1, line2 } in the same shape as
//     the WebUI envelope editor state (rates/levels 0..1, sustainPoint, endPoint)
// The random ranges and probabilities mirror PresetRandomizer.h exactly.

function randomizeEnvelope(isDCA, rng) {
  const rates = new Array(8).fill(0);
  const levels = new Array(8).fill(0);
  const numStages = 3 + Math.floor(rng() * 4); // 3-6 active stages

  for (let i = 0; i < 8; i++) {
    rates[i] = 0.3 + rng() * 0.7; // bias towards faster rates
    if (i === 0) rates[i] = 0.6 + rng() * 0.4; // attack fast usually
    levels[i] = rng();
  }

  let sustainPoint;
  let endPoint;
  if (isDCA) {
    levels[numStages] = 0.0; // endpoint must be silence
    endPoint = numStages;
    sustainPoint = numStages > 0 ? numStages - 1 : 0;
    if (rng() > 0.7) sustainPoint = -1; // randomly no sustain (pluck)
  } else {
    endPoint = numStages;
    sustainPoint = numStages > 0 ? numStages - 1 : 0;
  }

  return { rates, levels, sustainPoint, endPoint };
}

/**
 * Build a randomized patch. `rng` is a () => [0,1) function (injectable for
 * tests; defaults to Math.random). Returns { params, envelopes }.
 */
export function generateRandomPatch(rng = Math.random) {
  const params = {};

  // 1. Oscillators
  params.LINE_SELECT = Math.floor(rng() * 4);
  params.OSC1_WAVEFORM = Math.floor(rng() * 8);
  params.OSC1_WAVEFORM2 = rng() > 0.7 ? Math.floor(rng() * 8) : 0; // 0 = None
  params.OSC1_LEVEL = 0.8 + rng() * 0.2; // high level
  params.OSC2_WAVEFORM = Math.floor(rng() * 8);
  params.OSC2_WAVEFORM2 = rng() > 0.7 ? Math.floor(rng() * 8) : 0;
  params.OSC2_LEVEL = rng();
  params.OSC2_DETUNE = (rng() - 0.5) * 4.0; // +/- 2 semitones
  params.LINE_MODULATION = rng() > 0.8 ? 1 + Math.floor(rng() * 5) : 0; // 1..5: Ring1..Noise2
  params.MOD_SPECIAL = rng() > 0.9 ? 1 : 0;

  // Key Follow (per-line): 0-5, bias toward moderate values
  params.LINE1_KF_DCA = Math.floor(rng() * 6); // 0-5
  params.LINE1_KF_DCW = Math.floor(rng() * 6);
  params.LINE2_KF_DCA = Math.floor(rng() * 6);
  params.LINE2_KF_DCW = Math.floor(rng() * 6);

  // 2. Envelopes
  const dcaEnv = randomizeEnvelope(true, rng);
  const dcwEnv = randomizeEnvelope(false, rng);
  let pitchEnv;
  if (rng() > 0.8) {
    pitchEnv = randomizeEnvelope(false, rng);
  } else {
    // flat pitch
    const rates = new Array(8).fill(0.5);
    const levels = new Array(8).fill(0.5);
    pitchEnv = { rates, levels, sustainPoint: 0, endPoint: 1 };
  }

  // 3. LFO
  params.LFO_WAVE = Math.floor(rng() * 4); // 0-3
  params.LFO_RATE = 0.5 + rng() * 8.0; // 0.5 - 8.5 Hz
  params.LFO_DEPTH = rng() * 0.3; // subtle (×4 in handler → 0-1.2 semitones)
  params.LFO_DELAY = rng() * 0.5; // 0 - 0.5 s

  // 4. Effects
  params.CHORUS_MIX = rng() > 0.7 ? rng() * 0.5 : 0;
  params.DELAY_MIX = rng() > 0.8 ? rng() * 0.4 : 0;
  params.DELAY_TIME = 0.2 + rng() * 0.5;
  params.DELAY_FEEDBACK = 0.3;
  if (rng() > 0.8) { // 20% drive active
    params.DRIVE_AMOUNT = 0.2 + rng() * 0.6;
    params.DRIVE_COLOR = rng();
    params.DRIVE_MIX = 0.3 + rng() * 0.7;
  } else {
    params.DRIVE_AMOUNT = 0.0;
    params.DRIVE_COLOR = 0.5;
    params.DRIVE_MIX = 0.0;
  }

  // 5. Arpeggiator
  params.ARP_ENABLED = rng() > 0.8 ? 1 : 0;
  params.ARP_LATCH = rng() > 0.7 ? 1 : 0;
  params.ARP_RATE = Math.floor(rng() * 4); // 0-3
  params.ARP_BPM = 80 + rng() * 60; // 80-140
  params.ARP_GATE = 0.3 + rng() * 0.7;
  params.ARP_SWING = rng() > 0.7 ? rng() * 0.5 : 0;
  params.ARP_SWING_MODE = Math.floor(rng() * 3);
  params.ARP_PATTERN = Math.floor(rng() * 5);
  params.ARP_OCTAVE = 1 + Math.floor(rng() * 3); // 1-3

  return {
    params,
    envelopes: {
      dca: { line1: dcaEnv, line2: { ...dcaEnv, rates: [...dcaEnv.rates], levels: [...dcaEnv.levels] } },
      dcw: { line1: dcwEnv, line2: { ...dcwEnv, rates: [...dcwEnv.rates], levels: [...dcwEnv.levels] } },
      pitch: { line1: pitchEnv, line2: { ...pitchEnv, rates: [...pitchEnv.rates], levels: [...pitchEnv.levels] } }
    }
  };
}
