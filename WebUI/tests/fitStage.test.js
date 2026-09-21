/**
 * fitStage en CZ101: dos contratos.
 *
 * 1. COPIA GESTIONADA — WebUI/src/shared/fitStage.js es una copia VERBATIM de
 *    ABDSharedAssets/components/fitStage.js (CZ101 no tiene bundler: ESM nativo
 *    + juce_add_binary_data). Byte a byte contra el paquete del workspace;
 *    editar la copia a mano ROMPE la suite a propósito (node scripts/sync_shared.js).
 *
 * 2. INTEGRACIÓN — el mount escala y centra el lienzo de diseño (1409x768) y
 *    el detach deja de recibir resizes. Sin DOM: la suite corre en entorno
 *    `node` y mountFitStage solo escribe en `stage.style`, así que un stage
 *    falso { style: {} } es observador suficiente. El viewport falso tiene un
 *    fire() que despacha SOLO a los listeners registrados (simular un resize
 *    llamando a la fn capturada siempre repintaria: el detach es que el
 *    registro quede vacio, y eso es lo que se verifica).
 */

import { readFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { describe, expect, it } from 'vitest';

import { computeFit, mountFitStage } from '../src/shared/fitStage.js';

const here = dirname(fileURLToPath(import.meta.url));

describe('fitStage / copia gestionada', () =>
{
    it('es byte a byte la del paquete compartido', () =>
    {
        const suiteRoot = resolve(here, '..', '..', '..');
        const origin = join(suiteRoot, 'ABDSharedAssets', 'components', 'fitStage.js');
        const copy = join(here, '..', 'src', 'shared', 'fitStage.js');

        expect(readFileSync(copy, 'utf8')).toBe(readFileSync(origin, 'utf8'));
    });
});

describe('fitStage / integración CZ101', () =>
{
    const DESIGN = { width: 1409, height: 768 };

    /**
     * Viewport falso: innerWidth/innerHeight mutables y un fire() que ejecuta
     * solo a los listeners AUN registrados (como un window de verdad).
     */
    function fakeViewport ()
    {
        const listeners = new Map();

        return {
            listeners,
            innerWidth: 1409,
            innerHeight: 768,
            addEventListener: (name, fn) => listeners.set(name, fn),
            removeEventListener: (name) => listeners.delete(name),
            fire (name) { for (const fn of listeners.values()) fn(); },
        };
    }

    it('escala el lienzo para caber en el viewport y centra con offsets', () =>
    {
        const stage = { style: {} };
        const viewport = fakeViewport();

        mountFitStage(stage, { ...DESIGN, viewport });

        // Identidad = SIN transform (un scale(1) residual crea containing
        // block y re-anclaria overlays position:fixed; contrato del paquete).
        expect(stage.style.transform).toBe('');
        expect(stage.style.marginLeft).toBe('0px');
        expect(stage.style.marginTop).toBe('0px');
    });

    it('re-escala al resize y deja de hacerlo tras el detach', () =>
    {
        const stage = { style: {} };
        const viewport = fakeViewport();

        viewport.innerWidth = 704.5;   // mitad EXACTA del diseño: escala 0.5
        viewport.innerHeight = 384;

        const detach = mountFitStage(stage, { ...DESIGN, viewport });

        expect(Number.parseFloat(stage.style.transform.replace('scale(', ''))).toBeCloseTo(0.5, 5);

        viewport.innerWidth = 2818;
        viewport.innerHeight = 1536;
        viewport.fire('resize');

        expect(Number.parseFloat(stage.style.transform.replace('scale(', ''))).toBeCloseTo(2, 5);

        const before = stage.style.transform;

        detach();

        // Un resize tras el desuscribirse NO repinta: el registro esta vacio.
        viewport.innerWidth = 704.5;
        viewport.innerHeight = 384;
        viewport.fire('resize');

        expect(stage.style.transform).toBe(before);
    });

    it('computeFit acota con minScale/maxScale', () =>
    {
        const design = { designWidth: 1409, designHeight: 768 };

        expect(computeFit({ ...design, viewportWidth: 28180, viewportHeight: 15360, maxScale: 3 }).scale).toBe(3);
        expect(computeFit({ ...design, viewportWidth: 14, viewportHeight: 8, minScale: 0.01 }).scale).toBe(0.01);
    });
});
