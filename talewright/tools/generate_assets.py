#!/usr/bin/env python3
"""Generates Talewright's original default assets (D-011) into talewright/assets/generated.

Everything is made by code from fixed seeds: running it again gives the same pixels and samples. manifest.json records
each image's size and the SHA-256 of its RGBA pixels, and each sound's length, channels, import settings and the
SHA-256 of its 16-bit samples before encoding (not of the PNG or Ogg bytes, which depend on the encoder), plus the
library versions used. --check regenerates in memory and compares with the manifest, then compares the committed
files with it: each PNG's pixels, each Ogg file's length and channels, each <file>.import, and no stray files.

Usage (from the repository root; needs the libraries in talewright/tools/asset_gen/requirements.txt, at those versions
for identical pixels and samples):
    python talewright/tools/generate_assets.py            # write what differs and manifest.json
    python talewright/tools/generate_assets.py --check    # verify the manifest and the committed files (exit 2: a
                                                          # library is missing)
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

try:
    import numpy as np
    import PIL
    import soundfile
    from PIL import Image
except ImportError as missing:  # static_checks.py reports a skip for exit code 2.
    print(f"missing library: {missing.name} (pip install --user -r talewright/tools/asset_gen/requirements.txt)")
    sys.exit(2)

sys.path.insert(0, str(Path(__file__).resolve().parent))
from asset_gen import audio, battle, characters, interface, templates, tiles  # noqa: E402
from asset_gen.common import TILE, pixel_hash, save_png, to_image  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "talewright" / "assets" / "generated"
IMAGE_GROUPS = [tiles.generate, characters.generate, battle.generate, interface.generate, templates.generate]


def pcm(sound: audio.Sound) -> Any:
    """The sound as 16-bit samples, the form it is hashed and encoded in."""
    return np.clip(np.rint(sound.samples * 32767.0), -32768, 32767).astype(np.int16)


SAMPLE_KEYS = ("seconds", "channels", "pcm_sha256")  # What the Ogg file holds; the rest is in its .import.


def sound_entry(sound: audio.Sound) -> dict[str, Any]:
    samples = pcm(sound)
    return {
        "seconds": round(len(samples) / audio.RATE, 3),
        "channels": 1 if samples.ndim == 1 else int(samples.shape[1]),
        "loop": sound.loop,
        "loop_offset": sound.loop_offset,
        "bpm": sound.bpm,
        "beat_count": sound.beat_count,
        "bar_beats": sound.bar_beats,
        "pcm_sha256": hashlib.sha256(samples.tobytes()).hexdigest(),
    }


def import_settings(sound: audio.Sound) -> str:
    """The Ogg importer's options (modules/vorbis/resource_importer_ogg_vorbis.cpp:76-80); Godot fills in the rest."""
    return (
        '[remap]\n\nimporter="oggvorbisstr"\ntype="AudioStreamOggVorbis"\n\n[params]\n\n'
        f"loop={'true' if sound.loop else 'false'}\nloop_offset={sound.loop_offset}\nbpm={sound.bpm}\n"
        f"beat_count={sound.beat_count}\nbar_beats={sound.bar_beats}\n"
    )


def disk_differences(out: Path, files: dict[str, Any], sounds: dict[str, audio.Sound]) -> list[str]:
    """How the committed files differ from what the manifest records (pixels, Ogg length/channels, .import text)."""
    found: list[str] = []
    for path, entry in files.items():
        target = out / path
        if not target.is_file():
            found.append(f"{path}: missing")
        elif "pixels_sha256" in entry:
            with Image.open(target) as image:
                if pixel_hash(image) != entry["pixels_sha256"]:
                    found.append(f"{path}: its pixels differ from the manifest")
        else:
            info = soundfile.info(str(target))
            if (round(info.frames / info.samplerate, 3), info.channels) != (entry["seconds"], entry["channels"]):
                found.append(f"{path}: {info.frames / info.samplerate:.3f} s, {info.channels} channel(s) on disk")
            sidecar = out / (path + ".import")
            if not sidecar.is_file() or sidecar.read_text(encoding="utf-8") != import_settings(sounds[path]):
                found.append(f"{path}.import: missing or not the importer settings the manifest records")
    for target in sorted(out.rglob("*")):
        relative = target.relative_to(out).as_posix()
        if target.is_file() and relative != "manifest.json" and relative.removesuffix(".import") not in files:
            found.append(f"{relative}: not in the manifest")
    return found


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
        on_disk = disk_differences(out, files, sounds)
        for problem in on_disk:
            print(f"[DISK] {problem}")
        print(
            f"{len(files) - len(differences)} of {len(files)} files match manifest.json; "
            f"{len(on_disk)} difference(s) in the committed files"
        )
        if differences:
            print(
                f"recorded with numpy {recorded['numpy']} / Pillow {recorded['pillow']}; now numpy {np.__version__} / Pillow {PIL.__version__}"
            )
        return 1 if differences or on_disk else 0

    # Only what differs is written: an image whose pixels on disk differ, an Ogg file whose samples changed (Ogg
    # streams carry a random serial number, so rewriting the same samples would still change the file), and an
    # .import whose text differs.
    previous = json.loads(manifest_path.read_text(encoding="utf-8"))["files"] if manifest_path.exists() else {}

    for path, image in images.items():
        target = out / path
        if not target.is_file():
            save_png(image, target)
            continue
        with Image.open(target) as current:
            stale = pixel_hash(current) != files[path]["pixels_sha256"]
        if stale:
            save_png(image, target)
    for path, sound in sounds.items():
        target = out / path
        target.parent.mkdir(parents=True, exist_ok=True)
        old = previous.get(path, {})
        if not target.is_file() or any(old.get(key) != files[path][key] for key in SAMPLE_KEYS):
            samples = pcm(sound)
            channels = 1 if samples.ndim == 1 else samples.shape[1]
            # In blocks: libsndfile's Vorbis encoder overflows the stack on large single writes (seen on Windows).
            with soundfile.SoundFile(str(target), "w", audio.RATE, channels, format="OGG", subtype="VORBIS") as stream:
                for start in range(0, len(samples), 4096):
                    stream.write(samples[start : start + 4096])
        sidecar = out / (path + ".import")
        if not sidecar.is_file() or sidecar.read_text(encoding="utf-8") != import_settings(sound):
            with sidecar.open("w", encoding="utf-8", newline="\n") as handle:
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
