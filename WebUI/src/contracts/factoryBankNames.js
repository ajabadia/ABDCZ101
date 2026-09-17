// ─────────────────────────────────────────────────────────────────────────────
// factoryBankNames.js
//
// REAL names for the bundled factory banks. The CZ-101 sysex format stores no
// patch names (the engine labels everything "Imported Preset"), so without
// these tables the factory banks would only ever show generic "Patch NN".
//
// Sources:
//  - CZ-230S: official factory index ("CZ230Sindex.txt" from
//    github.com/alphacharlie/CZ-Presets, exported from a real CZ-230S). Each
//    .syx bank holds 16 patches in factory order (the 96-99 file holds 4).
//  - CZ Pack 1: the 20 individual .syx files are named after their patches;
//    we verified each one maps 1:1 onto a 264-byte chunk of CZPack1-All.syx,
//    so the order below is the bank order (Afterxylo … Yay Afterhit).
// ─────────────────────────────────────────────────────────────────────────────

// File name (as referenced by #select-cz230s-bank) -> patch names in order.
export const CZ230S_BANK_NAMES = {
  'CZ230S-00-15.syx': [
    'Brass Ens. 1', 'Brass Ens. 2', 'Brass Ens. 3', 'Symphonic Ens. 1',
    'Symphonic Ens. 2', 'Symphonic Ens. 3', 'String Ens. 1', 'String Ens. 2',
    'Synth Ens. 1', 'Synth Ens. 2', 'Light Harp', 'Mars Sound',
    'Southern Wing', 'Magical Wind', 'Funky Horn', 'Slap Horn'
  ],
  'CZ230S-16-31.syx': [
    'Sweet Strings', 'Light Attack', 'Synth Harp', 'Metallic Sound',
    'Jazz Organ 1', 'Jazz Organ 2', 'Pipe Organ 1', 'Pipe Organ 2',
    'Accordion', 'Female Chorus', 'Male Chorus', 'Space Voice 1',
    'Space Voice 2', 'Wah Voice', 'Trumpet', 'Flute'
  ],
  'CZ230S-31-47.syx': [
    'Whistle', 'Violin', 'Cello', 'Blues Harmonica', 'Shakuhachi', 'Koto',
    'Shamisen', 'Qanun', 'Synth Reed', 'Pearl Drop', 'Double Reed',
    'Meow Attack', 'Soft Attack', 'Fantasy 1', 'Fantasy 2', 'Plunk Extend'
  ],
  'CZ230S-48-63.syx': [
    'Slash Reed', 'Synth Sitar', 'Piano 1', 'Piano 2', 'Piano 3',
    'Honky-Tonk Piano', 'Elec. Piano', 'Harpsichord 1', 'Harpsichord 2',
    'Synth Clavi.', 'Metal Honky-Tonk', 'Double Attack', 'Bells', 'Carillon',
    'Synth Celesta', 'Synth Vib. 1'
  ],
  'CZ230S-64-79.syx': [
    'Synth Vib. 2', 'Synth Vib. 3', 'Bell-Lyra', 'Xylophone',
    'Soft Xylophone', 'Marimba', 'Aco. Guitar 1', 'Aco. Guitar 2',
    'Semiaco. Guitar', 'Feedback', 'Elec. Guitar 1', 'Elec. Guitar 2',
    'Elec. Bass 1', 'Elec. Bass 2', 'Slap Bass', 'Metallic Bass'
  ],
  'CZ230S-80-95.syx': [
    'Synth Drums 1', 'Synth Drums 2', 'Synth Drums 3', 'Synth Clapper',
    'Tambourine', 'Cowbell', 'Conga', 'Tabla', 'Afro-Percussion',
    'Steel Drum', 'Motorcycle', 'Jet Roar', 'Explosion', 'Typhoon Sound',
    'Cavernous Sound', 'Scratch Sound'
  ],
  'CZ230S-96-99.syx': ['Computer Game', 'Laser Gun', 'Miracle', 'Sweep']
};

// Order inside CZPack1-All.syx (verified byte-for-byte against the individual
// .syx files).
export const CZ_PACK1_NAMES = [
  'Afterxylo', 'Creature Bass', 'Digital Noise Organ', 'Drum Bass',
  'Drunk Trumpet', 'Fade Pad', 'Fairy Fly', 'Flick', 'Hollow Crystal',
  'Intense Feeling', 'Laser Puddles', 'Laser Sus Pad', 'Noisy Punch Bass',
  'Robo Growl', 'Saxobass', 'Snake Pad', 'Stiff Bass', 'Toad Bass',
  'Wist Pad', 'Yay Afterhit'
];

export const VIRTUALCZ_BANK_A = [
  '5th Bass', '80s Charmer Pad', 'Acid Pedal', 'Aftertouch Noise', 'Alarm', 'Alias Noiz', 'Aquatic Industries', 'Arpeggiate Me', 'AT Trumpets', 'Bassline House', 'Bdrrum Rroll', 'Brass Ens 2K', 'Carpenter Pedal', 'CZ Acid Lead', 'DDisco Bass', 'Detuned SynBrass', 'Dialtone Bass', 'Digi Zither', 'Double Squelch', 'Dying Hoover', 'Epic Strings', 'Faery Tails', 'Flyz', 'Formant Fun', 'Fuse Bass', 'Gameshow', 'Glocks and Toy Piano', 'Gnarly Bass', 'Golden Keys', 'Gremlin Bass', 'Harmonic Sweep', 'Inharmonic Fizz', 'Jungle Subbass', 'Kick Bass', 'Launch Site', 'Liquid Clav', 'Loopy Rez', 'Nails and Screws', 'Nineties Ambient', 'Octave Pulse', 'Perc Thing', 'PsyBass 1', 'PsyBass 2', 'PsyBass 3', 'Puck Pad', 'Raw Bass', 'Red Riding Pad', 'Reesey', 'Rip Noise', 'Robo Funk Bass', 'Rubber Bass', 'SAH Sawbass', 'Sea Ghosts', 'Singing Cats', 'Slop Bass', 'Slop Lead', 'Sooty and Sweep', 'Splash Bell', 'Sprung Spring', 'Squidge Lead', 'Upper Chugger', 'Wavescan Pad', 'Wide Organ', 'Wyrd Perc'
];

export const VIRTUALCZ_BANK_B = [
  '313 Stab', 'Alive', 'Ambient 110', 'AnaVeloBass', 'Bare Bones', 'Bass Vince', 'Carpet Pad', 'Cheesy Organ', 'Chese Extra', 'ChurchOrgan', 'Classic Organ', 'Computer', 'Crystals', 'CZ One', 'Detroit Pad', 'Distort Bass', 'Drive Velo', 'Elektronik', 'Emotional', 'Factory', 'FSOL Noizes', 'Funny', 'Goto100bpm', 'Hello PPG', 'Hold It', 'HoldDaChord', 'House Deeeep', 'House', 'Hypnotic', 'Industrial Prc', 'Industry', 'Japan', 'Jazz Chords', 'Jocko 110', 'JU60 Punchy', 'JU60 VeloBs', 'Karplus', 'Kerzwhile', 'LED Backlight', 'LegatoVeloBs', 'LL Cool', 'Magic', 'MagicalDream', 'Metal Atmo', 'MKS50 VeloBs', 'Mode Depeche', 'Model 200', 'MutatoMuzika', 'Paper Pad', 'Peaceful', 'PlayAt140bpm', 'Pluck', 'PseudoRandom', 'Pulsar', 'Pure Sine', 'PWM For You!', 'Radio Jingle', 'Radio', 'Raw Meat', 'Reso Organ', 'Retro Mania', 'Retroid', 'RetroPunchy', 'Rich CZ'
];

export const VIRTUALCZ_BANK_C = [
  'A-1 PolySynth C50', 'A-2 Pad C65', 'A-3 Resonant C65', 'A-4 Raw Square Velo', 'A-5 Saw Velo', 'A-6 Double Saw', 'A-8 CompressSine', 'B-1 Brass C40', 'B-2 Brass C30', 'B-3 Double Bass', 'B-4 Fat', 'B-5 OctaveSaw', 'B-6 Deeeep Bass', 'B-7 Riser C30', 'B-8 Poly Saw', 'C-1 Microwave', 'C-2 Morpher C60', 'C-3 Dirty Cash', 'C-4 Punchy Lows', 'C-5 SharpVelo Bass', 'C-6 Velo Killer BS', 'C-7 Bell Sound', 'C-8 Little Bell', 'D-1 Tubular Bell', 'D-2 Chinese Bell', 'D-3 Slowness C40', 'D-4 Thick Bass', 'D-5 Open Hat', 'D-6 LatelyBass', 'D-7 Velo SQR', 'D-8 Detuned Bass', 'E-1 KILLER Bass', 'E-2 Organ Ch60', 'E-3 Organ Ch50', 'E-4 Sweep C50', 'E-5 Scary C50', 'E-6 Moving C50', 'E-7 Phaser C50', 'E-8 Ring Mod C50', 'F-1 Perco Velo', 'F-2 Velo Glass', 'F-3 Feedback Gtr', 'F-4 Fifth', 'F-5 Moogy', 'F-6 Organ C70', 'F-7 OpMem Organ', 'F-8 CheeseOrgan', 'G-1 Kawai K3m C60', 'G-2 Kawai Patch v2', 'G-3 ST-01 Alligator', 'G-4 Ambient X', 'G-5 Ambient High', 'G-6 JetSet C50', 'G-7 Cosmosynth', 'G-8 Sweep', 'H-1 DX Seven', 'H-2 Elec Tom', 'H-3 OscSync Bass', 'H-4 Wow', 'H-5 1986', 'H-6 Eeeyeah', 'H-7 Dark String', 'H-8 Dark Pad'
];

export const VIRTUALCZ_BANK_D = [
  '1Un Bass', '303ness', '70ies Rocker', 'Alien 303', 'Aliens', 'Bassness', 'Basstix', 'Bee Organ', 'Bounce Map', 'Carnival', 'CasioTone', 'Chipzapper', 'Circiut Bent', 'Classic Dark', 'Dream Boat', 'DripBass', 'Ducky', 'Emox', 'Evolver', 'Flex Delay', 'Formantish', 'Frizzle', 'Glytches', 'GridKey', 'Grim Function', 'Happy Slide', 'Hot Rod', 'Industrial Pipe', 'Jabber Keys', 'Late Night Keys', 'Light Years', 'Little Toy Piano', 'Magikx', 'Make Noise', 'Melodinex', 'Midnight', 'Modern HarpCcord', 'MONOSnake', 'Moto Siren', 'Musical Snare', 'Nerve Organ', 'OctoBass', 'Out Tune World', 'oUTA Sight FX', 'Pulse Drum', 'RevDelay', 'Reverse Siren', 'ScaryMovie', 'Scrape', 'Shifty Keys', 'Shimerer', 'Skuaker', 'Smooth Space', 'Solar Harp', 'Star Data', 'StarLight', 'Static Storm', 'Triadical', 'TRON', 'WARRNING', 'Water World', 'Weirdo', 'Wind Star'
];

export const VIRTUALCZ_BANK_E = [
  'Robotic', 'Sequence Me', 'Sharp Angle', 'Sid', 'Soft Brass', 'Solid State', 'Solo Leader', 'Soundtrack', 'Space Echo', 'Stereo FM', 'Super Bass', 'Sweet', 'The Echo', 'The Film', 'The Pad', 'TheLastCZ', 'TheSpiritOf1986', 'Thick Brass', 'Thick Pad', 'Tokyo', 'Tsenk', 'Velo Arp', 'Velo Raptor', 'Velo Techno', 'Vortex CZ', 'Walkman Tune', 'Warp Bass', 'Wide Brass', 'Woody'
];

export const CZPATS_BANK_1 = [
  '1.octave.bass', '2.octave.bass', '747.landing', '8note.piano', '8note.poly', '96.tears', 'aaaho', 'acoustic.piano', 'art.of.noise.trumpet', 'background.swell', 'bell.organ', 'bird.call.1', 'bird.call.2', 'bowp.industrial', 'bowr.bass', 'brass.strings', 'brass1', 'bright.electric.piano', 'bushy', 'buzzing.fly', 'chain.gang', 'chiff.strings', 'choral', 'choral.ooh', 'chunky.blocks', 'clap.plus', 'coolstring', 'crazy', 'distorto.sweep', 'diva.harmonic', 'drill', 'drilling', 'dropped.bomb', 'drum.hit', 'drum.wash', 'echo.lead', 'electric.bubbles', 'electric.piano', 'fat.bass', 'fat.distorto.organ', 'fat.pluck', 'fat.sh', 'fat.synth.bass', 'fen', 'flute.light', 'fuzzy', 'fuzzy.bass', 'gliss.filter.sweep', 'grand.ethereal', 'guitar.distortion', 'guitar.with.distortion', 'hamburger.lady', 'harp', 'harpsichord.#1', 'harpsichord.#2', 'helicopter', 'herbie.scratch', 'ice.blue', 'index', 'jangly', 'juno106', 'kalimba', 'kick.1', 'kick.2'
];

export const CZPATS_BANK_2 = [
  'lazy.trumpet', 'lucky.man', 'marimba', 'marimba.2', 'massive.bass', 'mean.electric.guitar', 'mello.fat.bass', 'ministry.horns', 'minor.vox.chorus', 'moog.like', 'newhorn', 'night.porter', 'noise.sweep', 'oberheim.moan', 'oil.drum', 'oracle', 'organ.brass', 'pan.pipes.danger.sound', 'phil.in.a.box', 'piano', 'piano2', 'pipe.organ', 'pluck.wah', 'popcorn', 'psuedophone', 'psuedosilk', 'random.bass', 'rayz', 'reverb.flute', 'reverb.strings', 'reverbe.flute', 'reverse.perc', 'rhodes.piano', 'ring.modulo.1', 'ringmod.synth.bass', 'rising.resonance', 'sampled.snap', 'scratchy.trumpet', 'simmons.drum', 'simmons.sd7', 'slow.saw.sweep', 'snap.percussion', 'snare.1', 'snare.2', 'snare.double', 'solostring', 'sonic.bubbles', 'space', 'space.shots', 'spacious', 'star.trek.noise', 'steinway', 'swamp', 'swept.mut.ensemble', 'swept.reed', 'swerl', 'synth.bass.II', 'synth.organ', 'tb303', 'thompson.bass', 'thwack.drums', 'traffic.doppler', 'trevor.horns', 'wah.wow.yeh'
];

export const CZPATS_BANK_3 = [
  'whistling', 'wild.kingdom', 'woody.metal', 'yet.another.bass'
];

export const MAGAZINE_BANK = [
  '43 Hammond', 'Aaaaah', 'Acousticon', 'BigBrass', 'Blues Guitar', 'Brasso', 'Breathy Synth', 'Brufords Bong', 'Echo and the Bellman', 'ECM Noise 1', 'Fast Eddyfication', 'Happychord', 'Hollow', 'Its Krafty', 'Martha Going Oh and Ah', 'Mello Bell', 'Minimoog Bass', 'Nasty Sink', 'Piponette', 'Power Synth', 'Prophet Sync', 'Rebecca 2', 'Rebecca', 'Rich Reward', 'Rocko Pompo Mit Echo', 'Sad Smiles', 'Slightly Fretless Bass', 'Softo Americano', 'String Bass', 'Superbass', 'Thumb Bass', 'Timpani Drums', 'Ultrabass', 'Weird', 'Wetbrass'
];

export const RC20_BANK = [
  'PIANO 6', 'PIANO 7', 'PIANO 8', 'PIANO 9', 'ELEC.PIANO 4', 'ELEC.PIANO 5', 'ELEC.PIANO 6', 'ELEC.PIANO 7', 'HARPSICHORD 3', 'HARPSICHORD 4', 'FUNKY CLAVI.5', 'FUNKY CLAVI.6', 'TOY PIANO', 'HONKY-TONK 2', 'CELESTA 1', 'CELESTA 2', 'FOLK GUITAR', 'JAZZ GUITAR 2', 'ELEC.GUITAR 2', 'FUSION GUITAR', 'DIST.GUITAR', 'ELEC.BASS 3', 'SLAP BASS 4', 'SLAP BASS 5', 'WOOD BASS', 'BANJO 2', 'HARP 3', 'HARP 4', 'GLASS HARP', 'KOTO 2', 'KOTO 3', 'TAISHO KOTO', 'SHAMISEN', 'TSUZUMI', 'SITAR 2', 'JAW HARP', 'BONANG 1', 'BONANG 2', 'VIBRAPHONE 3', 'VIBRAPHONE 4', 'GLASS SOUND', 'MARIMBA 2', 'THUMB PIANO', 'AGOGO', 'CHIME 1', 'CHIME 2', 'BELLS 2', 'BELLS 3', 'GONG', 'SYN.PERCUSSION 1', 'SYN.PERCUSSION 2', 'SYN.PERCUSSION 3', 'SYN.PERCUSSION 4', 'SHORT SLAP', 'STEEL DRUM 2', 'MALLET', 'BASS DRUM', 'TAIKO 1', 'TAIKO 2', 'SYNTH.DRUMS 3', 'SYNTH.DRUMS 4', 'SYNTH.DRUMS 5', 'SCRATCH 1', 'SCRATCH 2'
];

export const RC30_BANK = [
  'PIPE ORGAN 4', 'PIPE ORGAN 5', 'PIPE ORGAN 6', 'ELEC.ORGAN 1', 'ELEC.ORGAN 2', 'ELEC.ORGAN 3', 'ELEC.ORGAN 4', 'ELEC.ORGAN 5', 'SYNTH.ENSEMBLE', 'SYNTH.BRASS', 'TWO OSCILLATORS', 'DREAM SOUND', 'MORNING HAZE', 'JUMP', 'POPCORN', 'MELLOW', 'SLOW SWELL TRPT.', 'SLAP BACK 4TH', 'LONG DELAY', 'SWEEP SOUND 4', 'SHADOW', 'NOON', 'SYNTH.BASS 4', 'SYNTH.BASS 5', 'DIST.LEAD', 'TRASH CAN LEAD', 'SYNTH.LEAD 9', 'SYNTH.LEAD 10', 'SYNTH.LEAD 11', 'SYNTH.LEAD 12', 'SYNTH.LEAD 13', 'SYNTH.LEAD 14', 'SPACE PITCH MOD.', 'FALLING ALIEN', 'DROP OF WATER', 'RIFLE', 'LASER GUN 2', 'FIRE ENGINE', 'AMBULANCE', 'CAR HORN', 'SPACE JET', 'TAKE OFF', 'HELICOPTER', 'FROZEN FREQUENCY', 'INDUSTRY', 'TANK', 'SF DOOR', 'TELEPHONE CALL', 'EXPLOSION 1', 'EXPLOSION 2', 'FIREWORKS 1', 'FIREWORKS 2', 'SEA GULL', 'OCEAN SOUND', 'TAP DANCE', 'INSECT', 'REVERSE', 'WEIRD SCIENCE', 'MUSIC BOX 2', 'OCT.TENSION', 'BIG WINDER', 'GALAXY TRAIN', 'SPACE LIFT', 'SAMBA WHISTLE'
];

export const BEW_BANK = [
  'apple-crisp-cz', 'basic-cz', 'Chapstick_Bass', 'deep-thoughts-cz', 'forgotten-cz', 'Funkwow_Bass', 'gingers_pad_cz', 'grounded-cz', 'melodiblock-cz', 'mono-lead-cz', 'mr-drummin-cz', 'naser-cz', 'round-bass-cz', 'Rubber_Bass', 'saw-pad-cz', 'soft-strings-cz', 'step-down-man-cz', 'systring-chapel-cz', 'twiggy-cz', 'wow-bass-cz'
];

/**
 * Names for a bundled factory bank, if we have a real list for it.
 * @param {string} source 'factory' or a .syx path (e.g. 'presets/czpack1/...')
 * @returns {string[] | null} real patch names (null if unknown)
 */
export const factoryBankNames = (source) => {
  if (!source || source === 'factory') return null;
  const file = source.split('/').pop();
  if (CZ230S_BANK_NAMES[file]) return CZ230S_BANK_NAMES[file];
  if (file === 'CZPack1-All.syx') return CZ_PACK1_NAMES;
  if (file === 'Bank_A_OL.syx') return VIRTUALCZ_BANK_A;
  if (file === 'Bank_B_DS.syx') return VIRTUALCZ_BANK_B;
  if (file === 'Bank_C_DS.syx') return VIRTUALCZ_BANK_C;
  if (file === 'Bank_D_AC.syx') return VIRTUALCZ_BANK_D;
  if (file === 'Bank_E_DS.syx') return VIRTUALCZ_BANK_E;
  if (file === 'Bank_CZPats_1.syx') return CZPATS_BANK_1;
  if (file === 'Bank_CZPats_2.syx') return CZPATS_BANK_2;
  if (file === 'Bank_CZPats_3.syx') return CZPATS_BANK_3;
  if (file === 'Bank_Magazine.syx') return MAGAZINE_BANK;
  if (file === 'Bank_RC20.syx') return RC20_BANK;
  if (file === 'Bank_RC30.syx') return RC30_BANK;
  if (file === 'Bank_BEW.syx') return BEW_BANK;
  return null;
};
