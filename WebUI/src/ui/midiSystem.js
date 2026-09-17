// ─── MIDI subsystem (native MIDIActivityIndicator + Web MIDI + audio output
// device selector + wheels + MIDI Learn parity) ───
// Extracted from app.js so all MIDI I/O — the activity LED, the connection
// badge, the audio-output device chooser (native Audio Settings), the virtual
// pitch/mod wheels and the right-click MIDI Learn menu — lives in one module.
//
// Factory pattern (same as createEnvelopeEditor / createKeyboard): the engine
// is a live reference (created at audio init) and `isAudioStarted` flips at
// runtime, so both arrive as getters. `sendParameter`/`applyParameterToUI`
// are stable app functions, and the LCD lines are stable app refs.

import { MIDIActivityIndicator, MIDI_LED_INTERVAL_MS } from '../contracts/midiActivity.js';
import { buildMidiStatus } from '../contracts/midiStatus.js';
import { createMidiLearnStore } from '../contracts/midiLearn.js';
import { PARAM_MAP } from '../contracts/registry.gen.js';

/**
 * createMidiSystem
 * @param {object} deps
 * @param {() => object} deps.getAudioEngine - live engine (null until started)
 * @param {() => boolean} deps.getIsAudioStarted
 * @param {(id: string, norm: number) => void} deps.sendParameter
 * @param {(id: string, norm: number) => void} deps.applyParameterToUI
 * @param {object} deps.lcdLine1
 * @param {object} deps.lcdLine2
 */
export function createMidiSystem(deps) {
  const {
    getAudioEngine,
    getIsAudioStarted,
    sendParameter,
    applyParameterToUI,
    lcdLine1,
    lcdLine2
  } = deps;

  // --- MIDI Activity LED (native MIDIActivityIndicator parity) ---
  // The native editor owns a MIDIActivityIndicator (timer 30 ms) that lights on
  // triggerActivity() and fades brightness by FADE_SPEED (0.1) per tick. Here
  // the same state machine drives the header LED via a CSS variable.
  const midiActivityLed = document.getElementById('midi-activity-led');
  const midiActivity = new MIDIActivityIndicator({
    onRender: (brightness) => {
      if (midiActivityLed) {
        midiActivityLed.style.setProperty('--led-brightness', String(brightness));
      }
    }
  });
  setInterval(() => midiActivity.tick(), MIDI_LED_INTERVAL_MS);

  // Every path that moves MIDI (inbound via Web MIDI, outbound from the virtual
  // keyboard / wheels / panic) triggers the LED, mirroring where the native
  // processor would call triggerActivity().
  function triggerMidiActivity() {
    midiActivity.triggerActivity();
  }

  // Web MIDI API connection setup. The badge shows the connected inputs (count +
  // names) and the active receive channel (native parity: the editor shows the
  // MIDI activity LED and the MIDI Channel menu; here the badge is the readout).
  const midiStatusBadge = document.getElementById('midi-status-badge');
  const midiChannelSelect = document.getElementById('MIDI_CH');
  let midiDevices = []; // [{ name }] of connected inputs (kept for the badge)

  const activeMidiChannel = () => {
    const v = midiChannelSelect ? parseInt(midiChannelSelect.value) : 1;
    return Number.isFinite(v) ? v : 1;
  };

  function applyMidiStatus(state) {
    if (!midiStatusBadge) return;
    const payload = buildMidiStatus({
      state,
      devices: midiDevices,
      channel: activeMidiChannel()
    });
    midiStatusBadge.innerText = payload.text;
    midiStatusBadge.title = payload.tooltip;
    midiStatusBadge.style.color = payload.color;
    midiStatusBadge.style.background = payload.bg;
    midiStatusBadge.style.border = payload.border;
  }

  // Keep the badge in sync when the receive channel changes (slider + navbar
  // submenu both dispatch 'change' on the MIDI_CH select).
  if (midiChannelSelect) {
    midiChannelSelect.addEventListener('change', () => {
      applyMidiStatus(midiDevices.length > 0 ? 'online' : 'no-devices');
    });
  }

  // ─── Audio output device selector (web equivalent of the native Audio
  // Settings device chooser / requestAudioSettings) ───
  // The browser routes an AudioContext to the system default output; setSinkId()
  // (Chromium/Edge) lets the user pick a specific device. enumerateDevices()
  // only exposes labels after a user gesture + running audio, so we (re)scan
  // after AUDIO ONLINE and via the REFRESH button / devicechange events.
  const audioOutputSelect = document.getElementById('audio-output-device');
  const btnRefreshAudioDevices = document.getElementById('btn-refresh-audio-devices');
  const AUDIO_OUTPUT_STORAGE_KEY = 'cz101.audioOutputDevice';

  function populateAudioOutputDevices() {
    if (!audioOutputSelect) return;
    const previous = audioOutputSelect.value;
    const resetToDefault = () => {
      audioOutputSelect.innerHTML = '';
      const def = document.createElement('option');
      def.value = 'default';
      def.textContent = 'System default';
      audioOutputSelect.appendChild(def);
    };
    if (!navigator.mediaDevices || typeof navigator.mediaDevices.enumerateDevices !== 'function') {
      resetToDefault();
      return;
    }
    navigator.mediaDevices.enumerateDevices()
      .then(devices => {
        const outputs = devices.filter(d => d.kind === 'audiooutput');
        if (outputs.length === 0) { resetToDefault(); return; }
        audioOutputSelect.innerHTML = '';
        const def = document.createElement('option');
        def.value = 'default';
        def.textContent = 'System default';
        audioOutputSelect.appendChild(def);
        outputs.forEach(d => {
          const opt = document.createElement('option');
          opt.value = d.deviceId;
          opt.textContent = d.label || 'Audio output';
          audioOutputSelect.appendChild(opt);
        });
        if (previous && Array.from(audioOutputSelect.options).some(o => o.value === previous)) {
          audioOutputSelect.value = previous;
        }
      })
      .catch(() => resetToDefault());
  }

  function applyAudioOutputDevice(deviceId) {
    const engine = getAudioEngine();
    if (!engine || !engine.audioCtx) return;
    const ctx = engine.audioCtx;
    if (typeof ctx.setSinkId !== 'function') return;
    ctx.setSinkId(deviceId === 'default' ? '' : deviceId)
      .then(() => {
        if (deviceId === 'default') localStorage.removeItem(AUDIO_OUTPUT_STORAGE_KEY);
        else localStorage.setItem(AUDIO_OUTPUT_STORAGE_KEY, deviceId);
      })
      .catch(() => {
        // Device no longer available (unplugged): fall back to the default.
        if (audioOutputSelect) audioOutputSelect.value = 'default';
      });
  }

  if (audioOutputSelect) {
    audioOutputSelect.addEventListener('change', () => applyAudioOutputDevice(audioOutputSelect.value));
  }
  if (btnRefreshAudioDevices) {
    btnRefreshAudioDevices.addEventListener('click', populateAudioOutputDevices);
  }
  if (navigator.mediaDevices && typeof navigator.mediaDevices.addEventListener === 'function') {
    navigator.mediaDevices.addEventListener('devicechange', populateAudioOutputDevices);
  }

  let activeMidiAccess = null;

  function setupWebMIDI() {
    if (window.__JUCE__) {
      applyMidiStatus('online'); // Native handles MIDI
      return;
    }

    if (!navigator.requestMIDIAccess) {
      applyMidiStatus('unsupported');
      return;
    }

    navigator.requestMIDIAccess().then(access => {
      activeMidiAccess = access;

      const scanDevices = () => {
        const inputs = Array.from(access.inputs.values());
        midiDevices = inputs.map(input => ({ name: input.name || 'MIDI Input' }));
        applyMidiStatus(midiDevices.length > 0 ? 'online' : 'no-devices');

        inputs.forEach(input => {
          input.onmidimessage = (message) => {
            if (getAudioEngine() && getIsAudioStarted()) {
              routeMidi(message.data);
            }
          };
        });
      };

      access.onstatechange = () => {
        scanDevices();
      };

      scanDevices();
    }).catch(err => {
      console.warn('MIDI Access request rejected:', err);
      applyMidiStatus('blocked');
    });
  }

  // Setup MIDI after starting audio
  const btnInit = document.getElementById('btn-init-audio');
  if (btnInit) {
    btnInit.addEventListener('click', () => {
      setupWebMIDI();
    });
  }

  // Virtual Pitch Bend and Mod Wheel Event Listeners
  const pitchWheel = document.getElementById('virtual-pitch-bend');
  const modWheel = document.getElementById('virtual-mod-wheel');

  if (pitchWheel) {
    const sendPitchBend = (val) => {
      const shifted = val + 8192;
      const lsb = shifted & 0x7F;
      const msb = (shifted >> 7) & 0x7F;
      if (window.__JUCE__ && window.__JUCE__.backend && window.__JUCE__.backend.sendMidiMessage) {
        window.__JUCE__.backend.sendMidiMessage([0xE0, lsb, msb]);
        triggerMidiActivity();
        return;
      }
      const engine = getAudioEngine();
      if (engine) {
        engine.sendMidiMessage(new Uint8Array([0xE0, lsb, msb]));
        triggerMidiActivity();
      }
    };

    pitchWheel.addEventListener('input', (e) => {
      const val = parseInt(e.target.value);
      sendPitchBend(val);
      
      const spriteContainer = document.getElementById('pitch-wheel-sprite');
      if (spriteContainer) {
        const pct = (val + 8192) / 16383;
        const frame = Math.round(pct * 100);
        spriteContainer.style.backgroundPosition = `0px -${frame * 76}px`;
      }
    });

    const resetPitch = () => {
      pitchWheel.value = 0;
      sendPitchBend(0);
      const spriteContainer = document.getElementById('pitch-wheel-sprite');
      if (spriteContainer) spriteContainer.style.backgroundPosition = `0px -3800px`;
    };

    pitchWheel.addEventListener('mouseup', resetPitch);
    pitchWheel.addEventListener('touchend', resetPitch);
  }

  if (modWheel) {
    modWheel.addEventListener('input', (e) => {
      const val = parseInt(e.target.value);
      if (window.__JUCE__ && window.__JUCE__.backend && window.__JUCE__.backend.sendMidiMessage) {
        window.__JUCE__.backend.sendMidiMessage([0xB0, 0x01, val]);
        triggerMidiActivity();
      } else {
        const engine = getAudioEngine();
        if (engine) {
          engine.sendMidiMessage(new Uint8Array([0xB0, 0x01, val]));
          triggerMidiActivity();
        }
      }
      
      const spriteContainer = document.getElementById('mod-wheel-sprite');
      if (spriteContainer) {
        const pct = val / 127;
        const frame = Math.round(pct * 100);
        spriteContainer.style.backgroundPosition = `0px -${frame * 76}px`;
      }
    });
  }

  // --- MIDI Learn (parity with the native MIDIProcessor learnNextCC/unmapCC) ---
  // Right-click any control to map a MIDI CC to it (like the native ScaledSlider
  // context menu). While learning, the next CC received from a real MIDI input is
  // captured and mapped; the CC that taught the mapping is consumed and does not
  // trigger the default hardcoded CC behavior. Mapped CCs override the engine's
  // default behavior (the raw CC is not forwarded; the normalized value drives the
  // parameter directly, mirroring onMidiParamChange -> APVTS). Unmapped CCs pass
  // through untouched so the defaults (mod wheel, sustain, filter 71/74, ...)
  // keep working. Mappings persist in localStorage across reloads.
  const midiLearn = createMidiLearnStore({ persist: true });

  // Route one raw MIDI message (from the Web MIDI API). Returns true when the
  // message was consumed by MIDI Learn (learned CC or mapped CC), false when it
  // was forwarded to the WASM engine unchanged.
  function routeMidi(data) {
    if (!data || !data.length) return false;
    triggerMidiActivity(); // inbound MIDI -> LED

    // Read the CC number up front: the mapped callback below runs synchronously
    // inside handleMidiMessage, before `result` is assigned (temporal dead zone).
    const cc = (data[0] & 0xF0) === 0xB0 ? data[1] : -1;

    const result = midiLearn.handleMidiMessage(data, (paramId, normValue) => {
      // Mapped CC -> normalized value drives the parameter (native onMidiParamChange).
      sendParameter(paramId, normValue);
      applyParameterToUI(paramId, normValue);
      const spec = PARAM_MAP.get(paramId);
      if (lcdLine1 && lcdLine2) {
        lcdLine1.innerText = (spec ? spec.name : paramId).toUpperCase();
        lcdLine2.innerText = `CC ${cc} = ${Math.round(normValue * 127)}`;
      }
      updateMidiLearnBadge();
    });

    if (result.consumed) {
      if (result.learned) {
        const spec = PARAM_MAP.get(result.learned.paramId);
        if (lcdLine1 && lcdLine2) {
          lcdLine1.innerText = 'MIDI LEARNED';
          lcdLine2.innerText = `CC ${result.learned.cc} -> ${(spec ? spec.name : result.learned.paramId).toUpperCase()}`;
        }
        updateMidiLearnBadge();
      }
      return true;
    }

    const engine = getAudioEngine();
    if (engine) engine.sendMidiMessage(data);
    return false;
  }

  function updateMidiLearnBadge() {
    const badge = document.getElementById('midi-learn-badge');
    if (!badge) return;
    const mappings = midiLearn.getMappings();
    const learning = midiLearn.isLearning;
    const spec = learning ? PARAM_MAP.get(midiLearn.pendingParamId) : null;
    badge.style.display = (learning || mappings.length > 0) ? '' : 'none';
    badge.textContent = learning
      ? `LEARN: MOVE CC FOR ${(spec ? spec.name : 'PARAM').toUpperCase()}`
      : `MIDI CC MAPS: ${mappings.length}`;
    badge.classList.toggle('learning', learning);
  }

  // Right-click context menu on any control bound to a parameter (native
  // ScaledSlider menu: Learn / Unlearn).
  let midiLearnMenu = null;

  function hideMidiLearnMenu() {
    if (midiLearnMenu) midiLearnMenu.remove();
    midiLearnMenu = null;
  }

  function showMidiLearnMenu(paramId, x, y) {
    hideMidiLearnMenu();
    const spec = PARAM_MAP.get(paramId);
    const name = (spec ? spec.name : paramId).toUpperCase();
    const cc = midiLearn.getCCForParam(paramId);

    midiLearnMenu = document.createElement('div');
    midiLearnMenu.className = 'midi-learn-menu';
    midiLearnMenu.style.left = `${x}px`;
    midiLearnMenu.style.top = `${y}px`;

    const title = document.createElement('div');
    title.className = 'midi-learn-menu-title';
    title.textContent = name;
    midiLearnMenu.appendChild(title);

    if (cc !== -1) {
      const mapped = document.createElement('div');
      mapped.className = 'midi-learn-menu-mapped';
      mapped.textContent = `Mapped to CC ${cc}`;
      midiLearnMenu.appendChild(mapped);
    }

    const learnBtn = document.createElement('button');
    learnBtn.type = 'button';
    learnBtn.textContent = cc === -1 ? 'Learn MIDI CC' : 'Re-learn MIDI CC';
    learnBtn.addEventListener('click', () => {
      midiLearn.learn(paramId);
      if (lcdLine1 && lcdLine2) {
        lcdLine1.innerText = 'MIDI LEARN';
        lcdLine2.innerText = `MOVE A CC FOR ${name}`;
      }
      updateMidiLearnBadge();
      hideMidiLearnMenu();
    });
    midiLearnMenu.appendChild(learnBtn);

    if (cc !== -1) {
      const unmapBtn = document.createElement('button');
      unmapBtn.type = 'button';
      unmapBtn.textContent = `Unmap CC ${cc}`;
      unmapBtn.addEventListener('click', () => {
        midiLearn.unmapCC(cc);
        if (lcdLine1 && lcdLine2) {
          lcdLine1.innerText = 'MIDI UNMAPPED';
          lcdLine2.innerText = `CC ${cc} -> ${name} CLEARED`;
        }
        updateMidiLearnBadge();
        hideMidiLearnMenu();
      });
      midiLearnMenu.appendChild(unmapBtn);
    }

    const closeBtn = document.createElement('button');
    closeBtn.type = 'button';
    closeBtn.textContent = 'Cancel';
    closeBtn.addEventListener('click', hideMidiLearnMenu);
    midiLearnMenu.appendChild(closeBtn);

    document.body.appendChild(midiLearnMenu);
  }

  // Right-click on any control bound to a parameter opens the learn/unmap menu.
  document.addEventListener('contextmenu', (e) => {
    // Walk up from the target to find an element whose id is a bound parameter.
    let el = e.target;
    while (el && el !== document.documentElement) {
      if (el.id && PARAM_MAP.has(el.id)) {
        e.preventDefault();
        showMidiLearnMenu(el.id, e.clientX, e.clientY);
        return;
      }
      el = el.parentElement;
    }
  });

  document.addEventListener('click', (e) => {
    if (midiLearnMenu && !midiLearnMenu.contains(e.target)) hideMidiLearnMenu();
  });
  document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') hideMidiLearnMenu();
  });

  return {
    triggerMidiActivity,
    routeMidi,
    populateAudioOutputDevices,
    applyAudioOutputDevice,
    applyMidiStatus,
    setupWebMIDI,
    AUDIO_OUTPUT_STORAGE_KEY,
    midiLearn
  };
}
