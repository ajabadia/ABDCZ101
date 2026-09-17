// Seeded example slots for the free modulation matrix on NEW presets in Modern
// mode. The fixed hardware routes were removed (each one is replicable with the
// free matrix), so these slots restore the authentic base behavior (velocity →
// amp) and demonstrate a key-track → pitch route. Slot numbers are 1-based,
// matching the param ids MOD_SLOT_<n>_SRC / _DEST / _DEPTH.
// Sources: 1=Velocity 2=ModWheel 3=Aftertouch 4=KeyTrack 5=LFO 6=EnvDCW
//          7=EnvDCA 8=EnvPitch 9=PitchBend 10=Noise 11=Authentic Key Track
// Dests:   1=DCW 2=DCA 3=Pitch 4=Vibrato 5=LFORate 6=Osc2Detune 7=Pan
export const MOD_MATRIX_EXAMPLE_SEEDS = [
  { slot: 1, src: 1, dest: 2, depth: 1.0 }, // Velocity → DCA (authentic base)
  { slot: 2, src: 4, dest: 3, depth: 0.3 }  // Key Track → Pitch (example route)
];

// True when every matrix slot source is "None" (0) — i.e. nothing has been
// configured yet, so the example seeds can be applied without clobbering a
// user-built routing.
export function isMatrixEmpty(srcValues) {
  if (!Array.isArray(srcValues) || srcValues.length === 0) return false;
  return srcValues.every(v => !v || parseInt(v, 10) === 0);
}
