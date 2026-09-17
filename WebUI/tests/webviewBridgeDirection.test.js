/**
 * Guard de dirección del canal nativo → JS de JUCE WebView2 — ABDCZ101.
 *
 * Mismo guard que en ABDMS2000, con la lógica **compartida** de la suite
 * (`ABDSharedCode/WebView2Bridge/testing/webviewBridgeDirection.js`): en JUCE 8,
 * `window.__JUCE__.backend.emitEvent(...)` es el canal *JS → nativo*, así que
 * escribirlo desde C++ descarta el mensaje en silencio; el canal nativo → JS es
 * `WebBrowserComponent::emitEventIfBrowserIsVisible(eventId, obj)`.
 *
 * ABDCZ101 ya usaba la API correcta; esto evita que se pierda en un refactor.
 */
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { describe, it, expect } from 'vitest';
import {
  REQUIRED_CALL,
  checkBridgeDirection,
  formatFindings
} from '../../../ABDSharedCode/WebView2Bridge/testing/webviewBridgeDirection.js';

const here = path.dirname(fileURLToPath(import.meta.url));
const sourceRoot = path.resolve(here, '../../Source');

/** Ficheros del proyecto que emiten del C++ hacia el WebUI. */
const EMITTERS = ['PluginEditor.cpp'];

describe('canal nativo → JS del WebView2 — guard de dirección (JUCE 8)', () => {
  it('ningún fichero C++ usa backend.emitEvent ni le falta emisor con la API correcta', () => {
    const result = checkBridgeDirection({ sourceRoot, emitters: EMITTERS });

    expect(result.scanned).toBeGreaterThan(0);
    expect(formatFindings(result)).toMatch(/^OK/);
    expect(result.absentEmitters).toEqual([]);
    expect(result.missingEmitters).toEqual([]);
    expect(result.problems).toEqual([]);
    expect(result.ok).toBe(true);
  });

  it('el emisor del editor usa emitEventIfBrowserIsVisible', () => {
    const editor = fs.readFileSync(path.join(sourceRoot, EMITTERS[0]), 'utf8');

    expect(editor).toContain(REQUIRED_CALL);
    expect(editor).not.toContain('backend.emitEvent');
  });
});
