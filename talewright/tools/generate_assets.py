#!/usr/bin/env python3
"""Generates Talewright's original default assets (D-011) into talewright/assets/generated.

Everything is drawn by code from fixed seeds: running it again gives the same pixels. manifest.json records each
file's size and the SHA-256 of its RGBA pixels (not of the PNG bytes, which depend on the compressor), plus the numpy
and Pillow versions used; --check regenerates in memory and compares with the manifest.

Usage (from the repository root; needs numpy and Pillow):
    python talewright/tools/generate_assets.py            # write the PNGs and manifest.json
    python talewright/tools/generate_assets.py --check    # verify that the manifest still matches
License of the output: CC0 1.0 (talewright/assets/LICENSES.md).
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

import numpy as np
import PIL

sys.path.insert(0, str(Path(__file__).resolve().parent))
from asset_gen import tiles  # noqa: E402
from asset_gen.common import TILE, pixel_hash, save_png, to_image  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "talewright" / "assets" / "generated"
GROUPS = {"tiles": tiles.generate}


def build(groups: list[str]) -> dict[str, Any]:
    images: dict[str, Any] = {}
    for group in groups:
        for path, pixels in GROUPS[group]().items():
            images[path] = to_image(pixels)
    return images


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true", help="regenerate in memory and compare with manifest.json")
    ap.add_argument("--out", default=str(OUT), help="output folder (default talewright/assets/generated)")
    args = ap.parse_args()
    out = Path(args.out)
    images = build(list(GROUPS))
    files = {
        path: {"size": [image.width, image.height], "pixels_sha256": pixel_hash(image)}
        for path, image in sorted(images.items())
    }
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
        print(f"{len(files) - len(differences)} of {len(files)} images match manifest.json")
        if differences:
            print(
                f"recorded with numpy {recorded['numpy']} / Pillow {recorded['pillow']}; now numpy {np.__version__} / Pillow {PIL.__version__}"
            )
        return 1 if differences else 0
    for path, image in images.items():
        save_png(image, out / path)
    manifest = {
        "generator": "talewright/tools/generate_assets.py",
        "license": "CC0-1.0",
        "tile_size": TILE,
        "numpy": np.__version__,
        "pillow": PIL.__version__,
        "files": files,
    }
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    with manifest_path.open("w", encoding="utf-8", newline="\n") as handle:  # LF on every platform.
        handle.write(json.dumps(manifest, indent=2) + "\n")
    print(f"wrote {len(images)} images to {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
