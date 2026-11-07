/**
 * Retro 8-bit Buzzer Audio Synthesizer
 * Emulates the ESP32 Cheap Yellow Display (CYD) piezo speaker on GPIO 26
 * using the Web Audio API with square and triangle waveforms.
 */

let audioCtx: AudioContext | null = null;
let soundEnabled = true;

export function toggleAudio(enabled?: boolean): boolean {
  if (enabled !== undefined) {
    soundEnabled = enabled;
  } else {
    soundEnabled = !soundEnabled;
  }
  return soundEnabled;
}

export function isAudioEnabled(): boolean {
  return soundEnabled;
}

function getAudioContext(): AudioContext | null {
  if (!soundEnabled) return null;
  if (!audioCtx) {
    const AudioContextClass = window.AudioContext || (window as any).webkitAudioContext;
    if (AudioContextClass) {
      audioCtx = new AudioContextClass();
    }
  }
  if (audioCtx && audioCtx.state === 'suspended') {
    audioCtx.resume();
  }
  return audioCtx;
}

/**
 * Play a raw tone (equivalent to ESP32 tone(GPIO_SPEAKER, freq, duration))
 */
export function playTone(freq: number, durationMs: number, type: OscillatorType = 'square', volume = 0.1) {
  const ctx = getAudioContext();
  if (!ctx) return;

  try {
    const osc = ctx.createOscillator();
    const gain = ctx.createGain();

    osc.type = type;
    osc.frequency.setValueAtTime(freq, ctx.currentTime);

    gain.gain.setValueAtTime(volume, ctx.currentTime);
    gain.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + durationMs / 1000);

    osc.connect(gain);
    gain.connect(ctx.destination);

    osc.start();
    osc.stop(ctx.currentTime + durationMs / 1000);
  } catch (e) {
    // Ignore audio autoplay restrictions
  }
}

export const cydAudio = {
  // Classic tactile CYD resistive touch click
  click: () => {
    playTone(980, 25, 'square', 0.08);
  },

  // Soft menu cancel or decline
  back: () => {
    playTone(440, 40, 'square', 0.08);
  },

  // Pet eating snack or meal
  munch: () => {
    playTone(330, 40, 'triangle', 0.12);
    setTimeout(() => playTone(494, 50, 'triangle', 0.12), 45);
    setTimeout(() => playTone(659, 60, 'triangle', 0.12), 95);
  },

  // Pet playing / joyful bounce
  play: () => {
    playTone(523, 40, 'square', 0.09);
    setTimeout(() => playTone(659, 40, 'square', 0.09), 50);
    setTimeout(() => playTone(784, 80, 'square', 0.09), 100);
  },

  // Bath / cleaning bubble pop
  clean: () => {
    playTone(700, 30, 'sine', 0.1);
    setTimeout(() => playTone(880, 30, 'sine', 0.1), 35);
    setTimeout(() => playTone(1050, 40, 'sine', 0.1), 70);
  },

  // Treat sent to remote friend's CYD
  treatSend: () => {
    playTone(523, 60, 'square', 0.1);
    setTimeout(() => playTone(659, 60, 'square', 0.1), 70);
    setTimeout(() => playTone(784, 60, 'square', 0.1), 140);
    setTimeout(() => playTone(1046, 120, 'square', 0.1), 210);
  },

  // Treat received from remote friend!
  treatReceive: () => {
    playTone(659, 80, 'triangle', 0.12);
    setTimeout(() => playTone(784, 80, 'triangle', 0.12), 90);
    setTimeout(() => playTone(1046, 160, 'triangle', 0.15), 180);
  },

  // Incoming friend chat notification
  chatPing: () => {
    playTone(880, 45, 'square', 0.09);
    setTimeout(() => playTone(1318, 90, 'square', 0.09), 60);
  },

  // Casting RPG Magic Spell
  magicCast: () => {
    const ctx = getAudioContext();
    if (!ctx) return;
    try {
      const osc = ctx.createOscillator();
      const gain = ctx.createGain();
      osc.type = 'sawtooth';
      osc.frequency.setValueAtTime(220, ctx.currentTime);
      osc.frequency.exponentialRampToValueAtTime(880, ctx.currentTime + 0.25);
      gain.gain.setValueAtTime(0.08, ctx.currentTime);
      gain.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.3);
      osc.connect(gain);
      gain.connect(ctx.destination);
      osc.start();
      osc.stop(ctx.currentTime + 0.3);
    } catch {
      playTone(600, 150, 'sawtooth', 0.08);
    }
  },

  // Egg hatching celebration
  eggHatch: () => {
    const notes = [440, 554, 659, 880, 1108];
    notes.forEach((freq, idx) => {
      setTimeout(() => playTone(freq, 90, 'triangle', 0.12), idx * 80);
    });
  },

  // RPG Level Up Fanfare!
  levelUp: () => {
    const notes = [523, 659, 784, 1046, 784, 1046, 1318];
    const delays = [0, 80, 160, 240, 320, 400, 520];
    notes.forEach((freq, idx) => {
      setTimeout(() => playTone(freq, idx === notes.length - 1 ? 250 : 70, 'square', 0.12), delays[idx]);
    });
  },

  // Trade offer received
  tradeBell: () => {
    playTone(587, 80, 'square', 0.1);
    setTimeout(() => playTone(880, 140, 'square', 0.1), 100);
  },

  // Low health warning
  warning: () => {
    playTone(260, 90, 'sawtooth', 0.1);
    setTimeout(() => playTone(220, 120, 'sawtooth', 0.1), 110);
  },
};
