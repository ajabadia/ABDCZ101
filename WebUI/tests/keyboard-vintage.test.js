import { describe, it, expect } from 'vitest';
import fs from 'fs';
import path from 'path';

/**
 * CZ-101 vintage: el keybed es el componente COMPARTIDO @abdsynths/midi-keyb.
 *
 * Contratos:
 *  1. El host MONTA el componente (import del paquete + CZ101_PRESET) en vez de
 *     renderizar su propio keybed.
 *  2. El rango sigue siendo el del host (C2..C6 = 36..84) con la velocity fija
 *     de siempre, y el preset aporta desgaste vintage + QWERTY + marfil.
 *  3. La franja `.keyboard` mantiene su altura DEFINIDA de 180px (contrato de
 *     layout: WebView2 colapsa las teclas sin altura definida).
 *  4. El aspecto envejecido (marfil + manchas deterministas) se aplica sobre las
 *     clases del componente, no sobre un keybed propio.
 */
describe('Vintage keyboard (aged ivory + deterministic wear)', () => {
  const appJs = fs.readFileSync(path.resolve(__dirname, '../src/app.js'), 'utf8');
  const css = fs.readFileSync(path.resolve(__dirname, '../src/styles/keyboard.css'), 'utf8');
  const html = fs.readFileSync(path.resolve(__dirname, '../index.html'), 'utf8');

  it('monta el keybed compartido del paquete (nada de keybed propio)', () => {
    expect(appJs).toContain("import { createKeyboard, CZ101_PRESET } from '@abdsynths/midi-keyb';");
    expect(appJs).toContain("import '@abdsynths/midi-keyb/keyboard.css';");
    expect(appJs).toContain('createKeyboard({');
    expect(appJs).toContain('...CZ101_PRESET,');
    expect(appJs).toContain("containerId: 'piano-keyboard',");
    // El rango y la velocity del host se conservan (36..84, MIDI 102 fijo).
    expect(appJs).toContain('startNote: 36,');
    expect(appJs).toContain('fixedVelocity: 102 / 127,');
    // El keybed propio ya no existe.
    expect(fs.existsSync(path.resolve(__dirname, '../src/ui/keyboard.js'))).toBe(false);
  });

  it('cablea la nota, el panico del chasis y el sostenido del componente', () => {
    // ALL OFF del chasis (navbar.js) en vez de un segundo boton en el keybed.
    expect(appJs).toContain("panicBtnId: 'btn-panic',");
    // El CC va al bridge JUCE o, en standalone, al motor WASM.
    expect(appJs).toContain('onSustainChange: (on) => sendMidiCc(64, on ? 127 : 0),');
    expect(appJs).toContain('onPanic: () => sendMidiCc(123, 0),');
    expect(appJs).toContain('audioEngine.sendMidiMessage(new Uint8Array([0xB0, controller, value]));');
    // Botones de octava del chasis: los manda el componente (ids + LEDs).
    expect(appJs).toContain("octUpId: 'octave-up-btn',");
    expect(appJs).toContain("ledDownId: 'octave-down-led',");
  });

  it('la franja de 180px envuelve al contenedor del componente', () => {
    expect(html).toContain('<div class="keyboard" style="flex-grow: 1;">');
    expect(html).toContain('<div id="piano-keyboard"></div>');
    expect(css).toMatch(/\.keyboard \{[^}]*height: 180px/);
  });

  it('las teclas usan marfil envejecido en vez de blanco plano', () => {
    expect(css).toMatch(/#piano-keyboard \.kbd-white-key \{[^}]*--kbd-white-base: #f6efdd/);
    expect(css).toContain('--kbd-white-bottom: #e6dabf;'); // yellowed base
  });

  it('el desgaste determinista se estiliza sobre las clases del componente', () => {
    for (const cls of ['kbd-stain-yellow', 'kbd-stain-scuff', 'kbd-stain-ding']) {
      expect(css).toContain(`#piano-keyboard .kbd-white-key.${cls}::after`);
    }
    expect(css).toContain('#piano-keyboard .kbd-black-key.kbd-stain-worn::after');
  });

  it('la patina global va encima de las teclas sin interceptar clicks', () => {
    expect(css).toMatch(/\.keyboard::after \{[^}]*z-index: 3/);
    expect(css).toMatch(/\.keyboard::after \{[^}]*pointer-events: none/);
    expect(css).toContain('repeating-linear-gradient(0deg, rgba(255, 255, 255, 0.015)');
  });
});
