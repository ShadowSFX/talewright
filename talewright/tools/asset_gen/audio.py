"""Synthesized sound effects and short loopable music (Ogg Vorbis), from fixed seeds.

Sound effects are mono; music is stereo and loops on a whole number of bars: every note ends inside the loop and the
echo is applied circularly, so the end flows into the start. The loop settings are written next to each file as the
Ogg importer's options (loop, loop_offset, bpm, beat_count, bar_beats; modules/vorbis/resource_importer_ogg_vorbis.cpp).
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Callable

import numpy as np

from .common import rng

RATE = 44100


@dataclass
class Sound:
    samples: Any  # float in -1..1; (n,) mono or (n, 2) stereo.
    loop: bool = False
    loop_offset: float = 0.0
    bpm: float = 0.0
    beat_count: int = 0
    bar_beats: int = 4


def seconds(duration: float) -> Any:
    return np.arange(int(round(duration * RATE))) / RATE


def envelope(n: int, attack: float, decay: float, sustain: float, release: float) -> Any:
    """ADSR over n samples: attack and decay in seconds, sustain level, release in seconds at the end."""
    t = np.arange(n) / RATE
    total = n / RATE
    env = np.where(t < attack, t / max(attack, 1e-6), 1.0)
    decaying = (t >= attack) & (t < attack + decay)
    env = np.where(decaying, 1.0 - (1.0 - sustain) * (t - attack) / max(decay, 1e-6), env)
    env = np.where(t >= attack + decay, sustain, env)
    release_start = max(total - release, attack)
    env = np.where(t > release_start, env * np.clip((total - t) / max(release, 1e-6), 0, 1), env)
    return env


def osc(freq: Any, duration: float, shape: str = "sine", duty: float = 0.5) -> Any:
    """An oscillator; freq may be a number or an array (a glide)."""
    t = seconds(duration)
    freq = np.broadcast_to(np.asarray(freq, dtype=np.float64), t.shape)
    phase = np.cumsum(freq) / RATE
    frac = phase % 1.0
    if shape == "sine":
        return np.sin(2 * np.pi * phase)
    if shape == "square":
        return np.where(frac < duty, 1.0, -1.0) * 0.6
    if shape == "triangle":
        return 2 * np.abs(2 * frac - 1) - 1
    if shape == "saw":
        return (2 * frac - 1) * 0.6
    raise ValueError(shape)


def noise(name: str, duration: float) -> Any:
    return rng(name).uniform(-1, 1, len(seconds(duration)))


def lowpass(signal: Any, cutoff: float) -> Any:
    """A one-pole low-pass filter."""
    a = np.exp(-2 * np.pi * cutoff / RATE)
    out = np.empty_like(signal)
    acc = 0.0
    for i, x in enumerate(signal):
        acc = (1 - a) * x + a * acc
        out[i] = acc
    return out


def midi(note: float) -> float:
    return float(440.0 * 2 ** ((note - 69) / 12))


def normalize(signal: Any, peak: float = 0.8) -> Any:
    top = float(np.max(np.abs(signal)))
    return signal * (peak / top) if top > 0 else signal


def glide(start: float, end: float, duration: float) -> Any:
    t = seconds(duration)
    return start * (end / start) ** (t / duration)


def concat(*parts: Any) -> Any:
    return np.concatenate(parts)


# ---------------------------------------------------------------------------------------------------------------
# Sound effects.


def blip(freq: float, duration: float, shape: str = "square", volume: float = 0.6) -> Any:
    n = len(seconds(duration))
    return osc(freq, duration, shape) * envelope(n, 0.002, duration * 0.4, 0.4, duration * 0.4) * volume


def cursor() -> Any:
    return blip(1320, 0.05, "square", 0.5)


def decision() -> Any:
    return concat(blip(880, 0.06), blip(1318.5, 0.11))


def cancel() -> Any:
    n = len(seconds(0.14))
    return osc(glide(900, 420, 0.14), 0.14, "square") * envelope(n, 0.002, 0.05, 0.5, 0.06) * 0.5


def buzzer() -> Any:
    n = len(seconds(0.25))
    return (osc(110, 0.25, "square", 0.3) + osc(117, 0.25, "square", 0.5)) * envelope(n, 0.003, 0.05, 0.7, 0.05) * 0.4


def equip() -> Any:
    n = len(seconds(0.35))
    ring = sum(osc(f, 0.35, "sine") * a for f, a in ((1760, 0.5), (2637, 0.3), (3520, 0.15)))
    return (
        ring * envelope(n, 0.001, 0.05, 0.3, 0.3) * 0.6
        + noise("equip", 0.35) * envelope(n, 0.001, 0.02, 0.0, 0.0) * 0.2
    )


def save() -> Any:
    notes = [72, 76, 79, 84]
    return concat(*[blip(midi(n), 0.09 if k < 3 else 0.3, "triangle", 0.7) for k, n in enumerate(notes)])


def load() -> Any:
    notes = [84, 79, 76, 72]
    return concat(*[blip(midi(n), 0.09 if k < 3 else 0.3, "triangle", 0.7) for k, n in enumerate(notes)])


def heal() -> Any:
    notes = [72, 76, 79, 84, 88, 91]
    sparkle = concat(*[blip(midi(n), 0.07, "sine", 0.5) for n in notes])
    tail = blip(midi(96), 0.4, "sine", 0.4)
    return concat(sparkle, tail)


def hit() -> Any:
    n = len(seconds(0.18))
    thump = osc(glide(160, 50, 0.18), 0.18, "sine") * envelope(n, 0.001, 0.08, 0.2, 0.08)
    crack = lowpass(noise("hit", 0.18), 3000) * envelope(n, 0.001, 0.03, 0.0, 0.0)
    return thump * 0.8 + crack * 1.2


def slash() -> Any:
    n = len(seconds(0.22))
    sweep = noise("slash", 0.22)
    sweep = sweep - lowpass(sweep, 1500)  # Bright: the high part of the noise.
    return sweep * envelope(n, 0.01, 0.08, 0.3, 0.12) * 1.4


def magic() -> Any:
    duration = 0.6
    n = len(seconds(duration))
    wobble = 1 + 0.02 * np.sin(2 * np.pi * 6 * seconds(duration))
    tone = sum(osc(midi(n_) * wobble, duration, "sine") * a for n_, a in ((79, 0.4), (86, 0.3), (91, 0.2)))
    return tone * envelope(n, 0.05, 0.2, 0.5, 0.3) * 0.8


def item_get() -> Any:
    melody = [(76, 0.08), (79, 0.08), (84, 0.08), (88, 0.32)]
    return concat(*[blip(midi(n), d, "square", 0.5) for n, d in melody])


def door() -> Any:
    n = len(seconds(0.3))
    knock = osc(glide(120, 70, 0.3), 0.3, "sine") * envelope(n, 0.002, 0.1, 0.1, 0.15)
    rattle = lowpass(noise("door", 0.3), 900) * envelope(n, 0.002, 0.05, 0.1, 0.15)
    return knock * 0.9 + rattle * 0.6


def step() -> Any:
    n = len(seconds(0.08))
    return lowpass(noise("step", 0.08), 1200) * envelope(n, 0.002, 0.03, 0.0, 0.0) * 1.5


def escape() -> Any:
    n = len(seconds(0.5))
    whoosh = noise("escape", 0.5)
    low = lowpass(whoosh, 600)
    return (whoosh - low) * envelope(n, 0.15, 0.1, 0.6, 0.2) * 0.9


def miss() -> Any:
    n = len(seconds(0.2))
    return osc(glide(600, 300, 0.2), 0.2, "triangle") * envelope(n, 0.005, 0.1, 0.3, 0.08) * 0.5


SFX: dict[str, Callable[[], Any]] = {
    "cursor": cursor,
    "decision": decision,
    "cancel": cancel,
    "buzzer": buzzer,
    "equip": equip,
    "save": save,
    "load": load,
    "heal": heal,
    "hit": hit,
    "slash": slash,
    "magic": magic,
    "item_get": item_get,
    "door": door,
    "step": step,
    "escape": escape,
    "miss": miss,
}

# ---------------------------------------------------------------------------------------------------------------
# Music.

MAJOR = [0, 2, 4, 5, 7, 9, 11]
MINOR = [0, 2, 3, 5, 7, 8, 10]


@dataclass(frozen=True)
class Track:
    bpm: float
    root: int  # MIDI note of the key's root (an octave below the melody).
    scale: tuple[int, ...]
    progression: tuple[int, ...]  # Scale degree (0-based) of each bar's chord.
    lead: str  # Oscillator shape of the melody.
    drums: str  # none, soft, march, fast
    density: float  # 0..1: how many eighth notes the melody plays.
    loop: bool = True


TRACKS: dict[str, Track] = {
    "title": Track(84, 57, tuple(MAJOR), (0, 5, 3, 4, 0, 5, 3, 4), "triangle", "none", 0.45),
    "field": Track(112, 60, tuple(MAJOR), (0, 4, 5, 3, 0, 4, 3, 4), "square", "soft", 0.6),
    "battle": Track(152, 57, tuple(MINOR), (0, 5, 6, 4, 0, 5, 3, 4), "saw", "fast", 0.75),
    "dungeon": Track(76, 50, tuple(MINOR), (0, 0, 5, 4, 0, 0, 3, 4), "sine", "march", 0.35),
    "victory": Track(132, 60, tuple(MAJOR), (0, 3, 4, 0), "square", "soft", 0.8, loop=False),
}


def chord_notes(track: Track, degree: int) -> list[int]:
    scale = list(track.scale)
    return [track.root + scale[(degree + k) % 7] + 12 * ((degree + k) // 7) for k in (0, 2, 4)]


def place(buffer: Any, start: int, signal: Any, gain: float, pan: float) -> None:
    """Adds signal into a stereo buffer at sample start, wrapping around the end (the loop)."""
    n = len(signal)
    idx = (start + np.arange(n)) % len(buffer)
    left, right = gain * np.sqrt((1 - pan) / 2), gain * np.sqrt((1 + pan) / 2)
    np.add.at(buffer[:, 0], idx, signal * left)
    np.add.at(buffer[:, 1], idx, signal * right)


def note(freq: float, duration: float, shape: str, attack: float = 0.01, release: float = 0.08) -> Any:
    n = len(seconds(duration))
    return osc(freq, duration, shape) * envelope(n, attack, duration * 0.3, 0.6, min(release, duration * 0.5))


def kick(name: str) -> Any:
    n = len(seconds(0.2))
    return osc(glide(140, 45, 0.2), 0.2, "sine") * envelope(n, 0.001, 0.08, 0.2, 0.1)


def snare(name: str) -> Any:
    n = len(seconds(0.16))
    return (lowpass(noise(name, 0.16), 5000) * 0.8 + osc(190, 0.16, "triangle") * 0.3) * envelope(
        n, 0.001, 0.05, 0.1, 0.08
    )


def hat(name: str) -> Any:
    n = len(seconds(0.05))
    hiss = noise(name, 0.05)
    return (hiss - lowpass(hiss, 6000)) * envelope(n, 0.001, 0.02, 0.0, 0.0)


def compose(name: str, track: Track) -> Sound:
    beat = 60.0 / track.bpm
    bar = 4 * beat
    bars = len(track.progression)
    total = int(round(bars * bar * RATE))
    tail = int(round(bar * RATE)) if not track.loop else 0
    buffer = np.zeros((total + tail, 2))
    gen = rng("music:" + name)
    scale = list(track.scale)
    melody_note = track.root + 12 + scale[2]
    for b, degree in enumerate(track.progression):
        bar_start = b * bar
        chord = chord_notes(track, degree)
        # Pad: the chord, held for the bar.
        for k, n_ in enumerate(chord):
            place(
                buffer,
                int(bar_start * RATE),
                note(midi(n_), bar * 0.95, "triangle", 0.05, 0.3) * 0.18,
                1.0,
                -0.3 + 0.3 * k,
            )
        # Bass: root on beats 1 and 3, fifth on 2 and 4.
        for k in range(4):
            bass = chord[0] - 12 if k % 2 == 0 else chord[0] - 12 + 7
            place(
                buffer,
                int((bar_start + k * beat) * RATE),
                note(midi(bass), beat * 0.9, "sine", 0.005, 0.05) * 0.5,
                1.0,
                0.0,
            )
        # Melody: eighth notes walking the scale, landing on chord tones on the beats.
        for k in range(8):
            if k % 2 == 1 and gen.random() > track.density:
                continue
            if k % 2 == 0:
                candidates = [c + 12 for c in chord] + [c + 24 for c in chord]
                melody_note = min(candidates, key=lambda c: abs(c - melody_note) + gen.random() * 3)
            else:
                melody_note += int(gen.choice([-2, -1, 1, 2]))
                pitch_class = (melody_note - track.root) % 12
                if pitch_class not in scale:
                    melody_note += 1
            melody_note = int(np.clip(melody_note, track.root + 12, track.root + 31))
            length = beat * 0.45 if k % 2 else beat * (0.9 if gen.random() < 0.6 else 0.45)
            place(
                buffer,
                int((bar_start + k * beat / 2) * RATE),
                note(midi(melody_note), length, track.lead, 0.01, 0.06) * 0.32,
                1.0,
                0.15,
            )
        # Drums.
        if track.drums != "none":
            for k in range(8):
                when = int((bar_start + k * beat / 2) * RATE)
                if k % 4 == 0 or (track.drums == "fast" and k == 5):
                    place(buffer, when, kick("kick"), 0.7, 0.0)
                if k % 4 == 2 and track.drums in ("soft", "fast", "march"):
                    place(buffer, when, snare(f"snare{b}{k}"), 0.35 if track.drums == "soft" else 0.5, 0.05)
                if track.drums in ("soft", "fast") or (track.drums == "march" and k % 2 == 0):
                    place(buffer, when, hat(f"hat{b}{k}"), 0.18, 0.4)
    # Echo, circular for loops (the tail of the last bar sounds at the start again).
    delay = int(beat * 0.75 * RATE)
    echo = np.zeros_like(buffer)
    source = buffer.copy()
    for repeat in range(1, 4):
        if track.loop:
            echo += np.roll(source, delay * repeat, axis=0) * (0.28**repeat)
        else:
            shifted = np.zeros_like(source)
            shifted[delay * repeat :] = source[: len(source) - delay * repeat]
            echo += shifted * (0.28**repeat)
    mix = normalize(buffer + echo[:, ::-1], 0.75)
    if not track.loop:
        mix[-int(0.3 * RATE) :] *= np.linspace(1, 0, int(0.3 * RATE))[:, None]
    return Sound(mix, loop=track.loop, bpm=track.bpm if track.loop else 0.0, beat_count=bars * 4 if track.loop else 0)


def generate() -> dict[str, Sound]:
    sounds: dict[str, Sound] = {}
    for name, make in SFX.items():
        sounds[f"audio/se/{name}.ogg"] = Sound(normalize(make(), 0.8))
    for name, track in TRACKS.items():
        folder = "me" if not track.loop else "bgm"
        sounds[f"audio/{folder}/{name}.ogg"] = compose(name, track)
    return sounds
