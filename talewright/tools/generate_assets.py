#!/usr/bin/env python3
"""Generates Talewright's original default assets (D-011) into talewright/assets/generated.

Everything is made by code from fixed seeds: running it again gives the same pixels and samples. manifest.json records
each image's size and the SHA-256 of its RGBA pixels, and each sound's length, channels, loop settings and the SHA-256
of its 16-bit samples before encoding (not of the PNG or Ogg bytes, which depend on the encoder), plus the library
versions used; --check regenerates in memory and compares with the manifest.

Usage (from the repository root; needs numpy, Pillow and soundfile for the Ogg Vorbis encoding):
    python talewright/tools/generate_assets.py            # write the files and manifest.json
    python talewright/tools/generate_assets.py --check    # verify that the manifest still matches
Each Ogg file gets a <file>.import next to it with the Godot importer's loop settings, so a project that copies the
file imports it looping as intended. License of the output: CC0 1.0 (talewright/assets/LICENSES.md).
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path
from typing import Any

import numpy as np
import PIL

sys.path.insert(0, str(Path(__file__).resolve().parent))
from asset_gen import audio, battle, characters, interface, tiles  # noqa: E402
from asset_gen.common import TILE, pixel_hash, save_png, to_image  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "talewright" / "assets" / "generated"
IMAGE_GROUPS = [tiles.generate, characters.generate, battle.generate, interface.generate]


def pcm(sound: audio.Sound) -> Any:
    """The sound as 16-bit samples, the form it is hashed and encoded in."""
    return np.clip(np.rint(sound.samples * 32767.0), -32768, 32767).astype(np.int16)


def sound_entry(sound: audio.Sound) -> dict[str, Any]:
    samples = pcm(sound)
    return {
        "seconds": round(len(samples) / audio.RATE, 3),
        "channels": 1 if samples.ndim == 1 else int(samples.shape[1]),
        "loop": sound.loop,
        "pcm_sha256": hashlib.sha256(samples.tobytes()).hexdigest(),
    }


def import_settings(sound: audio.Sound) -> str:
    """The Ogg importer's options (modules/vorbis/resource_importer_ogg_vorbis.cpp:76-80); Godot fills in the rest."""
    return (
        '[remap]\n\nimporter="oggvorbisstr"\ntype="AudioStreamOggVorbis"\n\n[params]\n\n'
        f"loop={'true' if sound.loop else 'false'}\nloop_offset={sound.loop_offset}\nbpm={sound.bpm}\n"
        f"beat_count={sound.beat_count}\nbar_beats={sound.bar_beats}\n"
    )


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true", help="regenerate in memory and compare with manifest.json")
    ap.add_argument("--out", default=str(OUT), help="output folder (default talewright/assets/generated)")
    args = ap.parse_args()
    out = Path(args.out)
    images = {path: to_image(pixels) for group in IMAGE_GROUPS for path, pixels in group().items()}
    sounds = audio.generate()
    files: dict[str, Any] = {
        path: {"size": [image.width, image.height], "pixels_sha256": pixel_hash(image)}
        for path, image in images.items()
    }
    files.update({path: sound_entry(sound) for path, sound in sounds.items()})
    files = dict(sorted(files.items()))
    manifest_path = out / "manifest.json"
    if args.check:
        recorded = json.loads(manifest_path.read_text(encoding="utf-8"))
        differences = [
            path
            for path in sorted(set(files) | set(recorded["files"]))
            if files.get(path) != recorded["files"].get(path)
        ]
        for path in differences:
            print(f"[DIFF] {path}: generated {files.get(path)}, recorded {recorded['files'].get(path)}")
        print(f"{len(files) - len(differences)} of {len(files)} files match manifest.json")
        if differences:
            print(
                f"recorded with numpy {recorded['numpy']} / Pillow {recorded['pillow']}; now numpy {np.__version__} / Pillow {PIL.__version__}"
            )
        return 1 if differences else 0

    import soundfile  # Only needed to write the Ogg files.

    for path, image in images.items():
        save_png(image, out / path)
    for path, sound in sounds.items():
        target = out / path
        target.parent.mkdir(parents=True, exist_ok=True)
        samples = pcm(sound)
        channels = 1 if samples.ndim == 1 else samples.shape[1]
        # In blocks: libsndfile's Vorbis encoder overflows the stack on large single writes (seen on Windows).
        with soundfile.SoundFile(str(target), "w", audio.RATE, channels, format="OGG", subtype="VORBIS") as stream:
            for start in range(0, len(samples), 4096):
                stream.write(samples[start : start + 4096])
        with (out / (path + ".import")).open("w", encoding="utf-8", newline="\n") as handle:
            handle.write(import_settings(sound))
    manifest = {
        "generator": "talewright/tools/generate_assets.py",
        "license": "CC0-1.0",
        "tile_size": TILE,
        "sample_rate": audio.RATE,
        "numpy": np.__version__,
        "pillow": PIL.__version__,
        "files": files,
    }
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    with manifest_path.open("w", encoding="utf-8", newline="\n") as handle:  # LF on every platform.
        handle.write(json.dumps(manifest, indent=2) + "\n")
    print(f"wrote {len(images)} images and {len(sounds)} sounds to {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
