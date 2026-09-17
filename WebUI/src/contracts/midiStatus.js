// MIDI connection status badge — pure formatting for the header badge.
// The native C interface has no badge (only the MIDIActivityIndicator LED), so
// this is a WebUI enhancement: it shows the connected MIDI inputs (count +
// names) and the active receive channel, like a DAW's MIDI port readout.
//
// States:
//   - 'offline'     audio not initialized yet (default badge)
//   - 'unsupported' navigator.requestMIDIAccess missing
//   - 'blocked'     requestMIDIAccess rejected (permission)
//   - 'no-devices'  Web MIDI available but no input ports
//   - 'online'      N input(s) connected, listening on the active channel
//
// Colors mirror the current inline badge styles (green = connected, orange =
// no devices, red = errors) so the module stays the single source of truth.

export const MIDI_CHANNEL_MIN = 1;
export const MIDI_CHANNEL_MAX = 16;

export function clampChannel(channel) {
  const n = Number(channel);
  if (!Number.isFinite(n)) return MIDI_CHANNEL_MIN;
  return Math.min(MIDI_CHANNEL_MAX, Math.max(MIDI_CHANNEL_MIN, Math.round(n)));
}

/** Device names, deduped and truncated so a long port list stays readable. */
export function formatDeviceNames(devices, max = 2) {
  const names = (devices || [])
    .map(d => (d && d.name) || 'MIDI Input')
    .filter((v, i, arr) => arr.indexOf(v) === i);
  if (names.length <= max) return names.join(', ');
  const shown = names.slice(0, max).join(', ');
  return `${shown} +${names.length - max} more`;
}

/**
 * Build the badge payload for a given state.
 * @param {object} opts { state, devices, channel }
 * @returns {{ text: string, tooltip: string, color: string, bg: string, border: string }}
 */
export function buildMidiStatus({ state = 'offline', devices = [], channel = MIDI_CHANNEL_MIN } = {}) {
  const ch = clampChannel(channel);
  const names = formatDeviceNames(devices);
  const tooltip = `MIDI inputs: ${devices.length} · listening on Ch ${ch}${names ? ' · ' + names : ''}`;

  switch (state) {
    case 'unsupported':
      return {
        text: 'MIDI: UNSUPPORTED',
        tooltip: 'Web MIDI API is not available in this browser',
        color: '#ff5555',
        bg: 'rgba(255, 50, 50, 0.15)',
        border: '1px solid rgba(255, 50, 50, 0.3)'
      };
    case 'blocked':
      return {
        text: 'MIDI: BLOCKED',
        tooltip: 'MIDI access was denied — check the browser permission',
        color: '#ff5555',
        bg: 'rgba(255, 50, 50, 0.15)',
        border: '1px solid rgba(255, 50, 50, 0.3)'
      };
    case 'no-devices':
      return {
        text: `MIDI: NO DEVICES · CH ${ch}`,
        tooltip: `Listening on Ch ${ch} — connect a MIDI input to play`,
        color: '#ffa500',
        bg: 'rgba(255, 165, 0, 0.15)',
        border: '1px solid rgba(255, 165, 0, 0.3)'
      };
    case 'online':
      return {
        text: `MIDI: ${devices.length} DEVICE${devices.length > 1 ? 'S' : ''} · CH ${ch}`,
        tooltip,
        color: '#00ffcc',
        bg: 'rgba(0, 255, 204, 0.15)',
        border: '1px solid rgba(0, 255, 204, 0.3)'
      };
    case 'offline':
    default:
      return {
        text: 'MIDI: OFFLINE',
        tooltip: 'Initialize the audio engine to scan MIDI inputs',
        color: '#ff5555',
        bg: 'rgba(255, 50, 50, 0.15)',
        border: '1px solid rgba(255, 50, 50, 0.3)'
      };
  }
}
