// On-screen keyboard — the full CZ-101 range (49 keys, C2 to C6 = MIDI 36..84),
// parity with the real hardware instead of a short 2-octave fragment. Extracted
// from app.js to keep responsibilities separated.
//
// Factory pattern (same as createEnvelopeEditor / createCompareManager): the
// engine is a live reference (created at audio init), so deps receives a
// getter; the LCD lines and MIDI-activity callback are stable app refs.
export function createKeyboard(deps) {
  const getAudioEngine = deps.getAudioEngine;
  const lcdLine1 = deps.lcdLine1;
  const lcdLine2 = deps.lcdLine2;
  const triggerMidiActivity = deps.triggerMidiActivity;

  const keyboard = document.getElementById('keyboard');
  const notes = [
    { name: 'C', black: false, offset: 0 },
    { name: 'C#', black: true, offset: 1 },
    { name: 'D', black: false, offset: 2 },
    { name: 'D#', black: true, offset: 3 },
    { name: 'E', black: false, offset: 4 },
    { name: 'F', black: false, offset: 5 },
    { name: 'F#', black: true, offset: 6 },
    { name: 'G', black: false, offset: 7 },
    { name: 'G#', black: true, offset: 8 },
    { name: 'A', black: false, offset: 9 },
    { name: 'A#', black: true, offset: 10 },
    { name: 'B', black: false, offset: 11 }
  ];

  const startNote = 36; // C2
  const numOctaves = 4; // C2..C5 (48 keys) + final C6 = 49 keys, like the CZ-101

  // Vintage wear: deterministic pseudo-random marks per key. The same note always
  // shows the same wear across reloads (like a real, aged keyboard), so the stains
  // are stable and testable — no RNG at runtime.
  function keyWear(midiNote, isBlack) {
    const h = (midiNote * 2654435761) % 100; // integer hash, stable per note
    const classes = [];
    if (isBlack) {
      if (h % 7 === 0) classes.push('stain-worn'); // dulled black key
      return classes;
    }
    if (h % 5 === 0) classes.push('stain-yellow'); // hand oil / yellowing
    if (h % 9 === 0) classes.push('stain-scuff');  // dark scuff mark
    if (h % 13 === 0) classes.push('stain-ding');  // chipped edge ding
    return classes;
  }

  const activeKeys = new Set();
  
  // State for octave transposition
  let currentOctaveOffset = 0;
  // Map of physical baseNote to currently playing actual transposed MIDI note
  const playingNotes = new Map();

  const updateOctaveLEDs = () => {
    const upLed = document.getElementById('octave-up-led');
    const downLed = document.getElementById('octave-down-led');
    if (!upLed || !downLed) return;

    upLed.className = 'octave-led';
    downLed.className = 'octave-led';

    if (currentOctaveOffset === 1) upLed.classList.add('blink');
    else if (currentOctaveOffset === 2) upLed.classList.add('on');
    
    if (currentOctaveOffset === -1) downLed.classList.add('blink');
    else if (currentOctaveOffset === -2) downLed.classList.add('on');
  };
  
  // Initialize LEDs
  setTimeout(updateOctaveLEDs, 0);

  // QWERTY keyboard mapping (Ableton/JUCE standard layout)
  const keyMap = {
    'z': 36, 's': 37, 'x': 38, 'd': 39, 'c': 40, 'v': 41, 'g': 42, 'b': 43,
    'h': 44, 'n': 45, 'j': 46, 'm': 47, ',': 48, 'l': 49, '.': 50, ';': 51, '/': 52,
    'q': 48, '2': 49, 'w': 50, '3': 51, 'e': 52, 'r': 53, '5': 54, 't': 55,
    '6': 56, 'y': 57, '7': 58, 'u': 59, 'i': 60, '9': 61, 'o': 62, '0': 63,
    'p': 64, '[': 65, '=': 66, ']': 67
  };

  const triggerNoteOn = (baseNote, keyEl) => {
    if (activeKeys.has(baseNote)) return;
    activeKeys.add(baseNote);
    
    const actualNote = Math.max(0, Math.min(127, baseNote + (currentOctaveOffset * 12)));
    playingNotes.set(baseNote, actualNote);

    if (keyEl) {
      keyEl.classList.add('active');
      keyEl.classList.add('playing');
    }
    const backend = window.getJuceBackend ? window.getJuceBackend() : ((window.__JUCE__ && window.__JUCE__.backend) || (window.Juce && window.Juce.backend));
    if (window.logToCpp) window.logToCpp(`Key down: note=${actualNote} hasBackend=${!!backend}`);
    if (backend) {
      if (typeof backend.sendMidiMessage === 'function') backend.sendMidiMessage([0x90, actualNote, 102]);
      if (typeof backend.emitEvent === 'function') backend.emitEvent('sendMidiMessage', [0x90, actualNote, 102]);
      lcdLine1.innerText = `NOTE ON`;
      lcdLine2.innerText = `MIDI NOTE: ${actualNote}`;
      triggerMidiActivity();
      return;
    }
    const audioEngine = getAudioEngine();
    if (audioEngine) {
      audioEngine.noteOn(actualNote, 0.8);
      lcdLine1.innerText = `NOTE ON`;
      lcdLine2.innerText = `MIDI NOTE: ${actualNote}`;
      triggerMidiActivity();
    }
  };

  const triggerNoteOff = (baseNote, keyEl) => {
    if (!activeKeys.has(baseNote)) return;
    activeKeys.delete(baseNote);
    
    const actualNote = playingNotes.get(baseNote);
    if (actualNote === undefined) return;
    playingNotes.delete(baseNote);

    if (keyEl) {
      keyEl.classList.remove('active');
      keyEl.classList.remove('playing');
    }
    const backend = window.getJuceBackend ? window.getJuceBackend() : ((window.__JUCE__ && window.__JUCE__.backend) || (window.Juce && window.Juce.backend));
    if (backend) {
      if (typeof backend.sendMidiMessage === 'function') backend.sendMidiMessage([0x80, actualNote, 0]);
      if (typeof backend.emitEvent === 'function') backend.emitEvent('sendMidiMessage', [0x80, actualNote, 0]);
      triggerMidiActivity();
      return;
    }
    const audioEngine = getAudioEngine();
    if (audioEngine) {
      audioEngine.noteOff(actualNote);
      triggerMidiActivity();
    }
  };

  // Attach global QWERTY listeners
  window.addEventListener('keydown', (e) => {
    if (e.ctrlKey || e.metaKey || e.altKey || e.repeat) return;
    if (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA' || e.target.tagName === 'SELECT') return;
    
    // Use ArrowUp / ArrowDown for octaves on QWERTY
    if (e.key === 'ArrowUp') {
      currentOctaveOffset = Math.min(2, currentOctaveOffset + 1);
      updateOctaveLEDs();
      return;
    } else if (e.key === 'ArrowDown') {
      currentOctaveOffset = Math.max(-2, currentOctaveOffset - 1);
      updateOctaveLEDs();
      return;
    }

    const note = keyMap[e.key.toLowerCase()];
    if (note !== undefined) {
      const keyEl = document.querySelector(`[data-note="${note}"]`);
      triggerNoteOn(note, keyEl);
    }
  });

  window.addEventListener('keyup', (e) => {
    const note = keyMap[e.key.toLowerCase()];
    if (note !== undefined) {
      const keyEl = document.querySelector(`[data-note="${note}"]`);
      triggerNoteOff(note, keyEl);
    }
  });

  // Attach octave button listeners
  setTimeout(() => {
    const upBtn = document.getElementById('octave-up-btn');
    const downBtn = document.getElementById('octave-down-btn');
    if (upBtn) {
      upBtn.addEventListener('click', () => {
        currentOctaveOffset = Math.min(2, currentOctaveOffset + 1);
        updateOctaveLEDs();
      });
    }
    if (downBtn) {
      downBtn.addEventListener('click', () => {
        currentOctaveOffset = Math.max(-2, currentOctaveOffset - 1);
        updateOctaveLEDs();
      });
    }
  }, 0);

  for (let octave = 0; octave < numOctaves; octave++) {
    const octOffset = octave * 12;
    notes.forEach(n => {
      const key = document.createElement('div');
      const midiNote = startNote + octOffset + n.offset;
      key.className = `key ${n.black ? 'black' : 'white'}`;
      key.dataset.note = midiNote;
      key.classList.add(...keyWear(midiNote, n.black));

      const onNoteStart = (e) => {
        if (e && e.preventDefault) e.preventDefault();
        triggerNoteOn(midiNote, key);
      };
      const onNoteEnd = (e) => {
        if (e && e.preventDefault) e.preventDefault();
        triggerNoteOff(midiNote, key);
      };

      key.addEventListener('pointerdown', onNoteStart);
      key.addEventListener('mousedown', onNoteStart);
      key.addEventListener('pointerup', onNoteEnd);
      key.addEventListener('mouseup', onNoteEnd);
      key.addEventListener('mouseleave', onNoteEnd);

      keyboard.appendChild(key);
    });
  }
  
  // Top C (C6 = MIDI 84) closes the 49-key range.
  const topC = document.createElement('div');
  const topCNote = startNote + numOctaves * 12;
  topC.className = 'key white';
  topC.dataset.note = topCNote;
  topC.classList.add(...keyWear(topCNote));

  const onTopCStart = (e) => {
    if (e && e.preventDefault) e.preventDefault();
    triggerNoteOn(topCNote, topC);
  };
  const onTopCEnd = (e) => {
    if (e && e.preventDefault) e.preventDefault();
    triggerNoteOff(topCNote, topC);
  };

  topC.addEventListener('pointerdown', onTopCStart);
  topC.addEventListener('mousedown', onTopCStart);
  topC.addEventListener('pointerup', onTopCEnd);
  topC.addEventListener('mouseup', onTopCEnd);
  topC.addEventListener('mouseleave', onTopCEnd);
  
  keyboard.appendChild(topC);
}
