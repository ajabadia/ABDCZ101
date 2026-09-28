import { defineConfig, devices } from '@playwright/test';

/**
 * E2E de navegador del CZ-101, con DOS paginas a la vez: la de DESARROLLO (Vite
 * dev server sobre `WebUI/`, con los once parciales de la hoja servidos desde
 * `src/styles/`) y la del BUNDLE (`vite preview` sobre `WebUI/dist/`, con los
 * parciales copiados a `styles/` y el grafo de JS empaquetado en
 * `assets/index.js`).
 *
 * Por que dos: el distintivo VIVO del cajon de bloques se verificaba solo contra
 * la pagina de desarrollo, y ahi es donde mas puede mentir un bundle — la hoja
 * son once ficheros en orden canonico, el pegamento del motor se pide por
 * fetch, y los `<link>` los reescribe el empaquetador. Un caso que abre el mismo
 * cajon en las dos paginas y compara el dato (y la pintura de la cabecera) es
 * lo que convierte "funciona en dev" en "funciona en lo que se envia".
 *
 * `--mute-audio` no es cosmetico: sin el, Chromium crea un AudioContext cuyo
 * reloj no avanza y la pagina se queda esperando al motor.
 *
 *   npx playwright test                      (las dos paginas)
 *   node scripts/build_webui.js              (construye el bundle que se sirve)
 */

const DEV_PORT = Number(process.env.CZ101_DEV_PORT ?? 5236);
const DIST_PORT = Number(process.env.CZ101_DIST_PORT ?? 5239);

export default defineConfig({
  testDir: './e2e',
  // En serie: las dos paginas comparten un unico navegador y un unico servicio
  // de audio.
  fullyParallel: false,
  workers: 1,
  forbidOnly: !!process.env.CI,
  retries: process.env.CI ? 1 : 0,
  reporter: [['list']],
  timeout: 60_000,
  use: {
    trace: 'on-first-retry',
    screenshot: 'only-on-failure',
  },
  projects: [
    {
      name: 'chromium',
      use: {
        ...devices['Desktop Chrome'],
        launchOptions: {
          args: ['--autoplay-policy=no-user-gesture-required', '--mute-audio'],
        },
      },
    },
  ],
  webServer: [
    {
      // La pagina de DESARROLLO: Vite sirve los fuentes sueltos, que es como se
      // ha estado viendo la maquina hasta ahora.
      command: `npx vite --port ${DEV_PORT} --strictPort`,
      url: `http://localhost:${DEV_PORT}/`,
      reuseExistingServer: !process.env.CI,
      timeout: 120_000,
    },
    {
      // La del BUNDLE. `npm run bundle` antes: servir un `dist` viejo seria
      // medir otra cosa (el mismo error que motivó el `--mute-audio` del smoke
      // de NEURONiK, aqui con el mismo disfraz).
      command: `npm run bundle && npx vite preview --port ${DIST_PORT} --strictPort`,
      url: `http://localhost:${DIST_PORT}/`,
      reuseExistingServer: !process.env.CI,
      timeout: 180_000,
    },
  ],
});
