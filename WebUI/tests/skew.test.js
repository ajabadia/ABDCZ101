import { describe, it, expect } from 'vitest';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';

import { rawToNormalized, normalizedToRaw, usesSkew, PARAM_MAP } from '../src/contracts/registry.gen.js';

const __dirname = dirname(fileURLToPath(import.meta.url));
// Las fuentes van en CRLF: los escaneos las normalizan a LF antes de anclar
// (mismo truco que block-badge).
const read = (relative) => readFileSync(join(__dirname, '..', relative), 'utf8').replace(/\r\n/g, '\n');

describe('JUCE Skew 0.3 Parity Tests', () => {
  it('should match skew 0.3 boundaries for MODERN_LPF_CUTOFF', () => {
    // Limits
    expect(rawToNormalized('MODERN_LPF_CUTOFF', 20)).toBeCloseTo(0.0, 5);
    expect(rawToNormalized('MODERN_LPF_CUTOFF', 20000)).toBeCloseTo(1.0, 5);
    
    expect(normalizedToRaw('MODERN_LPF_CUTOFF', 0.0)).toBeCloseTo(20, 5);
    expect(normalizedToRaw('MODERN_LPF_CUTOFF', 1.0)).toBeCloseTo(20000, 5);
  });

  it('should calculate the skew curve correctly at midpoint (norm = 0.5)', () => {
    // Midpoint: 20 + pow(0.5, 1/0.3) * (20000 - 20)
    // 0.5^(1/0.3) = 0.5^3.3333333 = 0.099212565
    // 20 + 0.099212565 * 19980 = 2002.267
    const expectedRaw = 20 + Math.pow(0.5, 1.0 / 0.3) * (20000 - 20); // ~2002.27
    const calculatedRaw = normalizedToRaw('MODERN_LPF_CUTOFF', 0.5);
    
    expect(calculatedRaw).toBeCloseTo(expectedRaw, 2);
    expect(rawToNormalized('MODERN_LPF_CUTOFF', calculatedRaw)).toBeCloseTo(0.5, 5);
  });

  it('should match skew 0.3 boundaries for MODERN_HPF_CUTOFF', () => {
    // MODERN_HPF_CUTOFF: min=20, max=10000, skew=0.3
    expect(rawToNormalized('MODERN_HPF_CUTOFF', 20)).toBeCloseTo(0.0, 5);
    expect(rawToNormalized('MODERN_HPF_CUTOFF', 10000)).toBeCloseTo(1.0, 5);

    const expectedMidRaw = 20 + Math.pow(0.5, 1.0 / 0.3) * (10000 - 20); // ~1010.14
    expect(normalizedToRaw('MODERN_HPF_CUTOFF', 0.5)).toBeCloseTo(expectedMidRaw, 2);
  });
});

// La regla de skew estaba escrita de tres formas y dos no coincidian para
// `skew: 1.0`: la identidad cuenta como SIN skew, asi que decide UN predicado
// (`usesSkew`, en el contrato generado) y nadie la reescribe a mano.
describe('la regla de skew del proyecto, escrita una sola vez', () => {
  it('usesSkew trata la identidad como "sin skew"', () => {
    expect(usesSkew({ skew: 0.3 })).toBe(true);
    expect(usesSkew({})).toBe(false);
    expect(usesSkew({ skew: undefined })).toBe(false);
    expect(usesSkew({ skew: null })).toBe(false);
    expect(usesSkew(null)).toBe(false);
    // 1.0 es identidad: contarlo como skew mandaria el control en normalizado
    // donde el motor espera el crudo.
    expect(usesSkew({ skew: 1.0 })).toBe(false);
  });

  it('el contrato solo tiene los dos cutoffs del filtro como parametros con skew', () => {
    // Ninguno con 1.0: por eso la discrepancia era latente y nadie la vio.
    const skewed = [...PARAM_MAP.values()].filter(usesSkew).map((spec) => spec.id);
    expect(skewed.sort()).toEqual(['MODERN_HPF_CUTOFF', 'MODERN_LPF_CUTOFF']);
    expect([...PARAM_MAP.values()].filter((spec) => spec.skew === 1.0)).toHaveLength(0);
  });

  it('las conversiones y el distintivo comparten el predicado en vez de repetirlo', () => {
    // Una copia de la regla en cada modulo es lo que la dejo divergir.
    for (const relative of ['src/app.js', 'src/ui/blockBadges.js']) {
      const source = read(relative);
      expect(source, relative).not.toMatch(/skew &&/);
      expect(source, relative).not.toMatch(/\.skew \?/);
    }
    expect(read('src/contracts/registry.gen.js')).toContain('export const usesSkew');
  });

  it('el default de fabrica entra SIEMPRE por el mapeo, con skew o sin el', () => {
    // El `default` de la APVTS llega en CRUDO. Mandarlo en crudo cuando hay skew
    // lo saturaba en applyParameterToUI: el paso alto (default 20 Hz) se
    // reiniciaba a 10 kHz en vez de a su minimo.
    expect(read('src/app.js')).toContain(
      'applyParameterToUI(p.id, p.default !== undefined ? rawToNormalized(p.id, p.default) : 0.0);'
    );
    for (const id of ['MODERN_LPF_CUTOFF', 'MODERN_HPF_CUTOFF']) {
      const spec = PARAM_MAP.get(id);
      const reset = rawToNormalized(id, spec.default);
      expect(Math.min(1, Math.max(0, reset)), id).toBeCloseTo(rawToNormalized(id, spec.default), 10);
    }
    expect(rawToNormalized('MODERN_HPF_CUTOFF', PARAM_MAP.get('MODERN_HPF_CUTOFF').default)).toBe(0.0);
  });
});
