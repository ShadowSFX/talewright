"""Guide sheets for artists: the blob and wall autotile templates (spec 05), one diagram per shape.

Each 48 x 48 cell shows a 3 x 3 neighborhood: the middle square is the tile, a filled neighbor is the same kind,
an empty one is not. Draw each shape over its diagram, then remove the guide.
"""

from __future__ import annotations

from typing import Any

import numpy as np

from .common import BLOB_MASKS, NE, NW, SE, SW, TILE, E, N, S, W, blank, coverage, grid, over, rgb, sdf_box

NEIGHBORS = {N: (0, -1), NE: (1, -1), E: (1, 0), SE: (1, 1), S: (0, 1), SW: (-1, 1), W: (-1, 0), NW: (-1, -1)}


def diagram(mask: int, sides_only: bool) -> Any:
    cell = blank(TILE, TILE)
    xs, ys = grid(TILE, TILE)
    over(cell, rgb("#e9e6df"), np.ones((TILE, TILE)))
    over(cell, rgb("#b9b4a9"), coverage(np.abs(sdf_box(xs, ys, 0.5, 0.5, TILE - 0.5, TILE - 0.5)) - 0.5))
    size, gap = 12.0, 2.0
    origin = (TILE - 3 * size - 2 * gap) / 2
    for (dx, dy), bit in [((0, 0), 0)] + [(offset, bit) for bit, offset in NEIGHBORS.items()]:
        if sides_only and bit in (NE, SE, SW, NW):
            continue
        x0 = origin + (dx + 1) * (size + gap)
        y0 = origin + (dy + 1) * (size + gap)
        square = sdf_box(xs, ys, x0, y0, x0 + size, y0 + size, 1.5)
        if bit == 0:
            over(cell, rgb("#3f6fb5"), coverage(square))
        elif mask & bit:
            over(cell, rgb("#7fa8dc"), coverage(square))
        else:
            over(cell, rgb("#b9b4a9"), coverage(np.abs(square) - 0.5))
    return cell


def blob_template() -> Any:
    sheet = blank(8 * TILE, 6 * TILE)
    for index, mask in enumerate(BLOB_MASKS):
        x, y = (index % 8) * TILE, (index // 8) * TILE
        sheet[y : y + TILE, x : x + TILE] = diagram(mask, sides_only=False)
    return sheet


def wall_template() -> Any:
    sheet = blank(4 * TILE, 4 * TILE)
    for mask4 in range(16):
        mask8 = (N if mask4 & 1 else 0) | (E if mask4 & 2 else 0) | (S if mask4 & 4 else 0) | (W if mask4 & 8 else 0)
        x, y = (mask4 % 4) * TILE, (mask4 // 4) * TILE
        sheet[y : y + TILE, x : x + TILE] = diagram(mask8, sides_only=True)
    return sheet


def generate() -> dict[str, Any]:
    return {"templates/blob_template.png": blob_template(), "templates/wall_template.png": wall_template()}
