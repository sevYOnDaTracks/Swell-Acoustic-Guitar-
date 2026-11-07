#!/usr/bin/env python3
"""Builds the built-in guitar kits from three CC0 multisample banks.

  python3 -I Tools/prepare_kits.py <downloads>

<downloads> holds the extracted banks:
  steel/x/INSGuitarSteel_String*_Vel0?_*.wav      Meta Steel Guitar (CC0)
  nylon/x/INSGuitarNylon_String*_Vel0?*_*.wav     Meta Nylon Guitar (CC0)
  electric/x/*/samples/bridge/*.flac              FreePats Electric Guitar FSBS direct (CC0)

For each pitch the acoustic kits keep the string a guitarist would use in first position
(the highest open string at or below the note), so a melody keeps one coherent timbre.
Every sample becomes mono FLAC, 16 bits / 48 kHz, its tail cut once it falls 60 dB below
its peak (with a short fade), and is named <Kit>_<note>_<layer>[_rrN].flac, which
SampleLibrary reads. Output: assets/samples/<Kit>/ and assets/samples/models.json.
"""
import glob
import json
import os
import re
import subprocess
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, '..', 'assets', 'samples')
RATE = 48000
OPEN = {'LowE': 40, 'A': 45, 'D': 50, 'G': 55, 'B': 59, 'HighE': 64}
NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']


def midi(note):
    m = re.fullmatch(r'([A-G])(s|#)?(-?\d)', note)
    pc = 'C D EF G A B'.index(m.group(1)) + (1 if m.group(2) else 0)
    return (int(m.group(3)) + 1) * 12 + pc


def name(n):
    return f'{NAMES[n % 12]}{n // 12 - 1}'


def decode(path):
    raw = subprocess.run(['ffmpeg', '-v', 'error', '-i', path, '-ac', '1', '-ar', str(RATE), '-f', 'f32le', '-'],
                         capture_output=True, check=True).stdout
    return np.frombuffer(raw, dtype=np.float32)


def clipped(path):
    raw = subprocess.run(['ffmpeg', '-v', 'error', '-i', path, '-f', 'f32le', '-'], capture_output=True, check=True).stdout
    return bool((np.abs(np.frombuffer(raw, dtype=np.float32)) > 0.999).any())


def export(src, dst, max_seconds):
    """Mono, tail cut at -60 dB under the peak (or max_seconds), 40 ms fade, 16-bit FLAC."""
    x = decode(src)
    w = RATE // 50
    env = np.array([np.abs(x[i:i + w]).max() for i in range(0, len(x), w)])
    peak = env.max()
    above = np.nonzero(env > peak * 10 ** (-60 / 20))[0]
    end = min(len(x), (above[-1] + 1) * w if len(above) else len(x), int(max_seconds * RATE))
    seconds = end / RATE
    subprocess.run(['ffmpeg', '-v', 'error', '-y', '-i', src, '-ac', '1', '-ar', str(RATE), '-t', f'{seconds:.3f}',
                    '-af', f'afade=t=out:st={max(0.0, seconds - 0.04):.3f}:d=0.04', '-sample_fmt', 's16',
                    '-compression_level', '8', dst], check=True)


def acoustic(bank, kit, pattern, layers, max_seconds):
    files = {}
    for path in glob.glob(pattern):
        m = re.search(r'_String(LowE|A|D|G|B|HighE)_(Vel0\d\w*)_([A-G]s?-?\d)_\d+\.wav$', path)
        if m and m.group(2) in layers:
            files[(m.group(1), layers[m.group(2)], midi(m.group(3)))] = path
    out_dir = os.path.join(OUT, kit)
    os.makedirs(out_dir, exist_ok=True)
    notes = sorted({n for (_, _, n) in files})
    written, skipped = 0, []
    for n in notes:
        for layer in sorted(set(layers.values())):
            # First-position string: the highest open string at or below the note, then the next ones down.
            strings = sorted((s for s in OPEN if OPEN[s] <= n and (s, layer, n) in files), key=lambda s: -OPEN[s])
            chosen = next((s for s in strings if not clipped(files[(s, layer, n)])), None)
            if chosen is None:
                skipped.append(f'{name(n)} {layer}')
                continue
            export(files[(chosen, layer, n)], os.path.join(out_dir, f'{kit}_{name(n).replace("#", "s")}_{layer}.flac'), max_seconds)
            written += 1
    print(f'{bank}: {written} samples, {name(notes[0])} to {name(notes[-1])}', ('skipped: ' + ', '.join(skipped)) if skipped else '')


def electric(pattern, max_seconds):
    out_dir = os.path.join(OUT, 'Electric')
    os.makedirs(out_dir, exist_ok=True)
    written = 0
    for path in sorted(glob.glob(pattern)):
        m = re.fullmatch(r'([A-G]#?-?\d)_s\d(_soft)?_(\d+)\.flac', os.path.basename(path))
        if not m:
            continue
        layer = 'p' if m.group(2) else 'mf'
        export(path, os.path.join(out_dir, f'Electric_{m.group(1).replace("#", "s")}_{layer}_rr{int(m.group(3))}.flac'), max_seconds)
        written += 1
    print(f'electric: {written} samples')


def main():
    downloads = sys.argv[1]
    acoustic('steel', 'Steel', os.path.join(downloads, 'steel/x/INSGuitarSteel_*.wav'), {'Vel01': 'p', 'Vel02': 'mf', 'Vel03': 'f'}, 6.0)
    acoustic('nylon', 'Nylon', os.path.join(downloads, 'nylon/x/INSGuitarNylon_*.wav'),
             {'Vel01Finger': 'p', 'Vel02Pick': 'mf', 'Vel03Pick': 'f'}, 6.0)
    electric(os.path.join(downloads, 'electric/x/*/samples/bridge/*.flac'), 8.0)
    models = [
        {'type': 0, 'name': 'Folk (acier)', 'kit': 'Steel', 'source': 'Meta Steel Guitar, CC0'},
        {'type': 0, 'name': 'Classique (nylon)', 'kit': 'Nylon', 'source': 'Meta Nylon Guitar, CC0'},
        {'type': 1, 'name': 'Fender (direct)', 'kit': 'Electric', 'source': 'FreePats Electric Guitar FSBS direct, CC0'},
    ]
    with open(os.path.join(OUT, 'models.json'), 'w', encoding='utf-8') as f:
        json.dump(models, f, ensure_ascii=False, indent=1)


if __name__ == '__main__':
    main()
