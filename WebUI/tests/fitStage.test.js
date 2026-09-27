/**
 * fitStage en CZ101: usa el paquete compartido @abdsynths/shared.
 *
 * El fitStage es una utilidad compartida (ABDSharedAssets/components/fitStage.js)
 * que se importa vía @abdsynths/shared/components. Los tests verifican que la
 * integración CZ101 funciona correctamente con el paquete compartido.
 */

import { describe, expect, it } from 'vitest';

import { computeFit, mountFitStage } from '@abdsynths/shared/components';

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

describe('fitStage / integración CZ101', () =>
{
    const DESIGN = { width: 1409, height: 768 };

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
