"""Shared helpers of the asset generator: deterministic randomness, tileable noise, shapes, lighting, PNG output.

Everything is computed with numpy on float RGBA arrays (height, width, 4) in 0..1, so results depend only on the
seeds below and the library versions recorded in the manifest. Shapes are signed distance fields: negative inside,
in pixels, so edges get one pixel of anti-aliasing and outlines are a distance band.
"""

from __future__ import annotations

import hashlib
from pathlib import Path
from typing import Any, Callable

import numpy as np
from PIL import Image

TILE = 48  # Pixels per tile (D-034).

Color = tuple[float, float, float]


def rgb(hex_color: str) -> Color:
    """'#5a9a3c' as floats in 0..1."""
    value = int(hex_color.lstrip("#"), 16)
    return ((value >> 16 & 255) / 255.0, (value >> 8 & 255) / 255.0, (value & 255) / 255.0)


def rng(name: str) -> Any:
    """A numpy Generator seeded by a name, so every asset has its own stable stream."""
    seed = int.from_bytes(hashlib.sha256(name.encode("utf-8")).digest()[:8], "little")
    return np.random.default_rng(seed)


def grid(width: int, height: int) -> tuple[Any, Any]:
    """Pixel-center coordinates (x, y) as float arrays of shape (height, width)."""
    ys, xs = np.mgrid[0:height, 0:width].astype(np.float64)
    return xs + 0.5, ys + 0.5


def periodic_noise(name: str, size: int, cells: int, octaves: int = 3, persistence: float = 0.5) -> Any:
    """Tileable value noise in 0..1 of shape (size, size): it wraps at the edges, so tiles join without seams."""
    gen = rng(name)
    xs, ys = grid(size, size)
    total = np.zeros((size, size))
    amplitude = 1.0
    weight = 0.0
    for octave in range(octaves):
        n = cells * (2**octave)
        lattice = gen.random((n, n))
        u = xs * n / size
        v = ys * n / size
        i0 = np.floor(u).astype(int) % n
        j0 = np.floor(v).astype(int) % n
        i1 = (i0 + 1) % n
        j1 = (j0 + 1) % n
        fu = u - np.floor(u)
        fv = v - np.floor(v)
        su = fu * fu * (3 - 2 * fu)
        sv = fv * fv * (3 - 2 * fv)
        top = lattice[j0, i0] * (1 - su) + lattice[j0, i1] * su
        bottom = lattice[j1, i0] * (1 - su) + lattice[j1, i1] * su
        total += amplitude * (top * (1 - sv) + bottom * sv)
        weight += amplitude
        amplitude *= persistence
    return total / weight


def periodic_wave(name: str, length: int, amplitude: float, harmonics: int = 3) -> Callable[[Any], Any]:
    """A smooth 1D offset function with period `length` (pixels), used to roughen edges that must line up across
    tiles: the offset at the end of a tile equals the offset at the start of the next one."""
    gen = rng(name)
    phases = gen.random(harmonics) * 2 * np.pi
    weights = gen.random(harmonics) + 0.5
    weights = weights / weights.sum()

    def wave(t: Any) -> Any:
        out = np.zeros_like(t, dtype=np.float64)
        for k in range(harmonics):
            out += weights[k] * np.sin(2 * np.pi * (k + 1) * t / length + phases[k])
        return amplitude * out

    return wave


def ramp(values: Any, stops: list[tuple[float, Color]]) -> Any:
    """Maps values in 0..1 to colors along a gradient; returns an (h, w, 3) array."""
    positions = np.array([p for p, _ in stops])
    colors = np.array([c for _, c in stops])
    out = np.zeros((*values.shape, 3))
    for channel in range(3):
        out[..., channel] = np.interp(values, positions, colors[:, channel])
    return out


def coverage(sdf: Any) -> Any:
    """Anti-aliased coverage of a signed distance field (negative inside)."""
    return np.clip(0.5 - sdf, 0.0, 1.0)


def sdf_circle(xs: Any, ys: Any, cx: float, cy: float, r: float) -> Any:
    return np.hypot(xs - cx, ys - cy) - r


def sdf_ellipse(xs: Any, ys: Any, cx: float, cy: float, rx: float, ry: float) -> Any:
    """Approximate distance to an ellipse (exact enough for drawing at these sizes)."""
    k = np.hypot((xs - cx) / rx, (ys - cy) / ry)
    return (k - 1.0) * min(rx, ry)


def sdf_box(xs: Any, ys: Any, x0: float, y0: float, x1: float, y1: float, radius: float = 0.0) -> Any:
    """Rounded rectangle from (x0, y0) to (x1, y1)."""
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    hx, hy = (x1 - x0) / 2 - radius, (y1 - y0) / 2 - radius
    dx = np.abs(xs - cx) - hx
    dy = np.abs(ys - cy) - hy
    outside = np.hypot(np.maximum(dx, 0), np.maximum(dy, 0))
    inside = np.minimum(np.maximum(dx, dy), 0)
    return outside + inside - radius


def sdf_segment(xs: Any, ys: Any, ax: float, ay: float, bx: float, by: float, r: float) -> Any:
    """A capsule: the segment a-b thickened by r."""
    px, py = xs - ax, ys - ay
    vx, vy = bx - ax, by - ay
    h = np.clip((px * vx + py * vy) / (vx * vx + vy * vy), 0.0, 1.0)
    return np.hypot(px - vx * h, py - vy * h) - r


def normals(sdf: Any) -> tuple[Any, Any]:
    """Unit outward normals (nx, ny) of a distance field, from its gradient."""
    gy, gx = np.gradient(sdf)
    length = np.hypot(gx, gy) + 1e-9
    return gx / length, gy / length


LIGHT = (-0.6, -0.8)  # Toward the light: up and to the left.


def lit(nx: Any, ny: Any) -> Any:
    """-1..1: how much a surface facing (nx, ny) faces the light."""
    return nx * LIGHT[0] + ny * LIGHT[1]


def blank(width: int, height: int) -> Any:
    return np.zeros((height, width, 4))


def over(dst: Any, color: Any, alpha: Any) -> None:
    """Composites color (h, w, 3 or a single color) with per-pixel alpha onto dst in place ("over" operator)."""
    color = np.broadcast_to(np.asarray(color, dtype=np.float64), (*alpha.shape, 3))
    a = np.clip(alpha, 0.0, 1.0)[..., None]
    out_a = a + dst[..., 3:4] * (1 - a)
    safe = np.where(out_a > 0, out_a, 1.0)
    dst[..., :3] = (color * a + dst[..., :3] * dst[..., 3:4] * (1 - a)) / safe
    dst[..., 3:4] = out_a


def shade(color: Any, amount: Any) -> Any:
    """Lightens (amount > 0) or darkens (amount < 0) colors (h, w, 3)."""
    amount = np.asarray(amount, dtype=np.float64)[..., None]
    lighter = color + (1 - color) * amount
    darker = color * (1 + amount)
    return np.where(amount > 0, lighter, darker)


def to_image(pixels: Any) -> Any:
    """Float RGBA in 0..1 to an 8-bit PIL image (rounded, so identical floats give identical pixels)."""
    data = np.clip(np.rint(pixels * 255.0), 0, 255).astype(np.uint8)
    return Image.fromarray(data, "RGBA")


def pixel_hash(image: Any) -> str:
    """SHA-256 of the size and the RGBA pixel bytes (independent of PNG compression)."""
    digest = hashlib.sha256(f"{image.width}x{image.height}".encode())
    digest.update(image.convert("RGBA").tobytes())
    return digest.hexdigest()


def save_png(image: Any, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    image.save(path, format="PNG", optimize=False, compress_level=9)


# Autotile shapes (spec 05): the 8-neighbour mask, reduced so a corner counts only when both of its sides are set.
N, NE, E, SE, S, SW, W, NW = 1, 2, 4, 8, 16, 32, 64, 128


def reduce_mask(mask: int) -> int:
    for corner, a, b in ((NE, N, E), (SE, E, S), (SW, S, W), (NW, W, N)):
        if not (mask & a and mask & b):
            mask &= ~corner
    return mask


BLOB_MASKS = sorted({reduce_mask(m) for m in range(256)})  # Shape index = position in this list.
assert len(BLOB_MASKS) == 47
