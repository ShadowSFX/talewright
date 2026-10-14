#!/usr/bin/env python3
"""Compare an image with a golden image (task 8.4.2): the map captures of the bridge (jrpg_map_capture) and other
screenshots whose pixels are expected to stay the same.

Two images match when they have the same size and at most --max-fraction of their pixels (default 0.1 %) differ by
more than --tolerance (default 2 of 255) in any channel, alpha included. Goldens are made on purpose, never by a
failing comparison: regenerate one only when the rendering is meant to change, and say so in the commit.

Usage:
    python talewright/tools/img_compare.py ACTUAL.png GOLDEN.png [--tolerance 2] [--max-fraction 0.001]
                                           [--diff DIFF.png]
Exit code 0: match; 1: the images differ (the report says how); 2: an image cannot be read.
--diff writes an image of the differing pixels (red where they differ, the golden dimmed elsewhere).
Requires Pillow and numpy (talewright/tools/asset_gen/requirements.txt pins both).
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path


def compare(actual_path: Path, golden_path: Path, tolerance: int, max_fraction: float, diff_path: Path | None) -> int:
    try:
        import numpy
        from PIL import Image
    except ImportError as error:
        print(f"img_compare: needs Pillow and numpy ({error})")
        return 2
    try:
        actual = numpy.asarray(Image.open(actual_path).convert("RGBA"), dtype=numpy.int16)
        golden = numpy.asarray(Image.open(golden_path).convert("RGBA"), dtype=numpy.int16)
    except (OSError, ValueError) as error:
        print(f"img_compare: cannot read an image: {error}")
        return 2
    if actual.shape != golden.shape:
        print(
            f"DIFFERENT: size {actual.shape[1]} x {actual.shape[0]} against the golden's "
            f"{golden.shape[1]} x {golden.shape[0]}"
        )
        return 1
    largest = numpy.abs(actual - golden).max(axis=2)
    differing = int((largest > tolerance).sum())
    total = largest.size
    fraction = differing / total if total else 0.0
    if diff_path is not None:
        out = (golden // 3).astype(numpy.uint8)
        out[..., 3] = 255
        mask = largest > tolerance
        out[mask] = (255, 0, 0, 255)
        Image.fromarray(out, "RGBA").save(diff_path)
    verdict = "MATCH" if fraction <= max_fraction else "DIFFERENT"
    print(
        f"{verdict}: {differing} of {total} pixels differ by more than {tolerance}/255 "
        f"({fraction * 100:.4f} %, allowed {max_fraction * 100:.4f} %); largest channel difference "
        f"{int(largest.max()) if total else 0}"
    )
    return 0 if verdict == "MATCH" else 1


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("actual", type=Path)
    parser.add_argument("golden", type=Path)
    parser.add_argument("--tolerance", type=int, default=2, help="per-channel difference allowed, 0-255 (default 2)")
    parser.add_argument(
        "--max-fraction", type=float, default=0.001, help="share of pixels allowed to differ (default 0.001)"
    )
    parser.add_argument("--diff", type=Path, help="write an image of the differing pixels")
    args = parser.parse_args()
    if not 0 <= args.tolerance <= 255 or not 0 <= args.max_fraction <= 1:
        print("img_compare: --tolerance is 0-255 and --max-fraction 0-1")
        return 2
    return compare(args.actual, args.golden, args.tolerance, args.max_fraction, args.diff)


if __name__ == "__main__":
    sys.exit(main())
