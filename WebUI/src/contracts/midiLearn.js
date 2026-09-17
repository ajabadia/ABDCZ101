// MIDI Learn manager for the WebUI — parity with the native plugin's
// MIDIProcessor (Source/MIDI/MIDIProcessor.cpp).
//
// Native semantics replicated here:
//   learnNextCC(paramId)  -> learn(paramId): the NEXT CC received is captured
//                            and mapped to the parameter. The CC that taught
//                            the mapping is consumed (not applied), exactly
//                            like handleControlChange()'s "isLearning" branch.
//   unmapCC(cc)           -> unmapCC(cc): removes one CC->param mapping.
//   getCCForParam(paramId)-> getCCForParam(paramId): first CC for a param, -1
//                            if none (native scans ccMapping in insertion order).
//   Mapped CCs override the engine's hardcoded default behavior: when a CC has
//   a mapping, the value (0..1) is routed to that parameter and the raw CC is
//   NOT forwarded to the engine (native: "Override default behavior if mapped").
//   Unmapped CCs pass through untouched so the default hardcoded CC behavior
//   (mod wheel, volume, drive, filter 71/74, ...) keeps working.
//
// Unlike the native in-memory map, this store is persistable to localStorage
// (opt-in via `persist`) so the standalone WebUI keeps mappings across reloads.

export function createMidiLearnStore({ persist = false, storageKey = 'cz101.midiLearn' } = {}) {
  let ccMapping = new Map(); // cc (int) -> paramId (string)
  let isLearning = false;
  let learningParamId = null;

  const save = () => {
    if (!persist) return;
    try {
      localStorage.setItem(storageKey, JSON.stringify([...ccMapping.entries()]));
    } catch (err) {
      // Storage unavailable (private mode / quota) — mappings stay in memory.
    }
  };

  const load = () => {
    if (!persist) return;
    try {
      const raw = localStorage.getItem(storageKey);
      if (raw) {
        const entries = JSON.parse(raw);
        if (Array.isArray(entries)) {
          ccMapping = new Map(entries.filter(([cc, id]) => Number.isInteger(cc) && typeof id === 'string'));
        }
      }
    } catch (err) {
      ccMapping = new Map();
    }
  };

  load();

  return {
    /** Begin learning: the next CC message will map to paramId. */
    learn(paramId) {
      if (!paramId) return false;
      isLearning = true;
      learningParamId = paramId;
      return true;
    },

    /** Cancel a pending learn without consuming the next CC. */
    cancelLearn() {
      isLearning = false;
      learningParamId = null;
    },

    get isLearning() { return isLearning; },

    /** ParamId waiting to be mapped (valid while isLearning). */
    get pendingParamId() { return learningParamId; },

    /** Remove the mapping for a specific CC number. Returns true if removed. */
    unmapCC(cc) {
      const had = ccMapping.delete(cc);
      if (had) save();
      return had;
    },

    /** First CC mapped to paramId, or -1 (native getCCForParam). */
    getCCForParam(paramId) {
      for (const [cc, id] of ccMapping) {
        if (id === paramId) return cc;
      }
      return -1;
    },

    /**
     * Process one raw Web MIDI message (array of bytes).
     *
     * Returns:
     *   { consumed: true, learned: { cc, paramId } }  — a learn was in progress;
     *     the CC is now mapped and was NOT forwarded (native isLearning branch).
     *   { consumed: true, mapped: { cc, paramId, value } } — the CC has a
     *     mapping; `onParam` was invoked with the normalized value and the CC
     *     was NOT forwarded (native mapped branch).
     *   { consumed: false } — not a CC, or unmapped CC: forward to the engine.
     *
     * `onParam` is only called for mapped CCs (native onMidiParamChange).
     */
    handleMidiMessage(message, onParam) {
      if (!message || message.length < 2) return { consumed: false };
      const status = message[0];
      const cc = message[1];
      if ((status & 0xF0) !== 0xB0) return { consumed: false }; // not a CC

      // 1. MIDI Learn logic
      if (isLearning) {
        ccMapping.set(cc, learningParamId);
        isLearning = false;
        const paramId = learningParamId;
        learningParamId = null;
        save();
        return { consumed: true, learned: { cc, paramId } };
      }

      // 2. Mapped parameter control (overrides default behavior)
      const paramId = ccMapping.get(cc);
      if (paramId !== undefined) {
        const value = message.length > 2 ? message[2] / 127.0 : 0.0;
        if (onParam) onParam(paramId, value);
        return { consumed: true, mapped: { cc, paramId, value } };
      }

      // 3. Default hardcoded behavior: forward to the engine
      return { consumed: false };
    },

    /** All mappings as [[cc, paramId], ...] sorted by CC number. */
    getMappings() {
      return [...ccMapping.entries()].sort((a, b) => a[0] - b[0]);
    },

    /** Remove every mapping. */
    clear() {
      ccMapping.clear();
      save();
    }
  };
}
