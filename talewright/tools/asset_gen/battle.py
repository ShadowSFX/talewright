"""Enemy battlers (front view, transparent background) and battlebacks (a backdrop and a ground layer, 1280 x 720)."""

from __future__ import annotations

from typing import Any, Callable

import numpy as np

from .common import (
    Color,
    blank,
    coverage,
    grid,
    lit,
    normals,
    over,
    periodic_noise,
    ramp,
    rgb,
    rng,
    sdf_box,
    sdf_circle,
    sdf_ellipse,
    sdf_segment,
    shade,
)

OUTLINE: Color = (0.08, 0.06, 0.08)

# ---------------------------------------------------------------------------------------------------------------
# Enemies: drawn at their own size with the same lit-shape style as the map sprites, a soft shadow at the feet.


def part(canvas: Any, sdf: Any, color: Color, light: float = 0.45, texture: Any = None, outline: float = 1.6) -> None:
    nx, ny = normals(sdf)
    base = np.broadcast_to(np.array(color), (*sdf.shape, 3)).copy()
    if texture is not None:
        base = shade(base, (texture - 0.5) * 0.35)
    depth = np.clip(-sdf / 14.0, 0, 1)
    base = shade(base, lit(nx, ny) * light * (1 - depth * 0.6) + depth * 0.08)
    over(canvas, OUTLINE, coverage(sdf - outline) * 0.9)
    over(canvas, base, coverage(sdf))


def eyes(
    canvas: Any, xs: Any, ys: Any, points: list[tuple[float, float]], size: float, iris: Color, angry: bool = True
) -> None:
    for ex, ey in points:
        over(canvas, (1.0, 1.0, 0.95), coverage(sdf_ellipse(xs, ys, ex, ey, size, size * 1.15)))
        over(canvas, iris, coverage(sdf_circle(xs, ys, ex, ey + size * 0.15, size * 0.6)))
        over(canvas, (0.05, 0.03, 0.05), coverage(sdf_circle(xs, ys, ex, ey + size * 0.2, size * 0.3)))
        over(canvas, (1.0, 1.0, 1.0), coverage(sdf_circle(xs, ys, ex - size * 0.3, ey - size * 0.25, size * 0.2)))
        over(canvas, OUTLINE, coverage(np.abs(sdf_ellipse(xs, ys, ex, ey, size, size * 1.15)) - 0.7))
        if angry:
            inner = 1 if ex > np.mean([p[0] for p in points]) else -1
            brow = sdf_segment(
                xs, ys, ex - size * 1.1 * inner, ey - size * 1.5, ex + size * 0.9 * inner, ey - size * 0.9, size * 0.28
            )
            over(canvas, OUTLINE, coverage(brow))


def feet_shadow(canvas: Any, xs: Any, ys: Any, cx: float, cy: float, rx: float) -> None:
    over(canvas, (0.0, 0.0, 0.0), coverage(sdf_ellipse(xs, ys, cx, cy, rx, rx * 0.22) * 1.5) * 0.3)


def moss_blob() -> Any:
    w, h = 136, 104
    canvas = blank(w, h)
    xs, ys = grid(w, h)
    feet_shadow(canvas, xs, ys, 68, 94, 52)
    body = np.minimum(sdf_ellipse(xs, ys, 68, 64, 52, 32), sdf_ellipse(xs, ys, 68, 48, 34, 30))
    moss = np.kron(periodic_noise("moss", 48, 6, octaves=3), np.ones((3, 3)))[:h, :w]
    part(canvas, body, rgb("#6aa84f"), light=0.55, texture=moss)
    for cx, cy, r in ((44, 34, 7), (84, 28, 6), (98, 52, 5)):
        part(canvas, sdf_circle(xs, ys, cx, cy, r), rgb("#3f7d2e"), light=0.4, outline=1.0)
    over(canvas, (1.0, 1.0, 1.0), coverage(sdf_ellipse(xs, ys, 46, 44, 9, 5)) * 0.35)
    eyes(canvas, xs, ys, [(56, 58), (80, 58)], 7.5, rgb("#c0392b"))
    over(canvas, OUTLINE, coverage(sdf_segment(xs, ys, 60, 76, 76, 76, 1.6)))
    return canvas


def cave_bat() -> Any:
    w, h = 176, 116
    canvas = blank(w, h)
    xs, ys = grid(w, h)
    feet_shadow(canvas, xs, ys, 88, 108, 30)
    for side in (-1, 1):
        tip_x = 88 + side * 84
        wing = np.maximum(
            sdf_ellipse(xs, ys, 88 + side * 44, 42, 46, 30),
            -np.min(np.stack([sdf_circle(xs, ys, 88 + side * k, 76, 15) for k in (30, 56, 80)]), axis=0),
        )
        part(canvas, wing, rgb("#5b3f7a"), light=0.4)
        for k in (34, 58):
            over(canvas, OUTLINE, coverage(sdf_segment(xs, ys, 88 + side * 16, 36, 88 + side * k, 66, 1.0)) * 0.6)
        over(canvas, OUTLINE, coverage(sdf_segment(xs, ys, 88 + side * 16, 30, tip_x, 30, 1.0)) * 0.5)
    body = sdf_ellipse(xs, ys, 88, 54, 20, 26)
    part(canvas, body, rgb("#6e4e90"), light=0.5)
    for side in (-1, 1):
        ear = np.maximum(sdf_circle(xs, ys, 88 + side * 10, 30, 10), np.abs(xs - (88 + side * 12)) - (40 - ys) * 0.35)
        part(canvas, ear, rgb("#6e4e90"), light=0.4)
    eyes(canvas, xs, ys, [(81, 48), (95, 48)], 5.0, rgb("#f1c40f"))
    for side in (-1, 1):
        fang = np.maximum(np.abs(xs - (88 + side * 5)) - (70 - ys) * 0.35, 63 - ys)
        over(canvas, (1.0, 1.0, 1.0), coverage(fang))
    return canvas


def mushroom_imp() -> Any:
    w, h = 116, 136
    canvas = blank(w, h)
    xs, ys = grid(w, h)
    feet_shadow(canvas, xs, ys, 58, 128, 34)
    for side in (-1, 1):
        part(canvas, sdf_ellipse(xs, ys, 58 + side * 14, 124, 11, 6), rgb("#7a5a3a"), light=0.3)
    stem = sdf_box(xs, ys, 38, 64, 78, 122, 16)
    part(canvas, stem, rgb("#efe3c8"), light=0.45)
    for side in (-1, 1):
        part(canvas, sdf_segment(xs, ys, 58 + side * 20, 84, 58 + side * 34, 100, 4.5), rgb("#efe3c8"), light=0.35)
    cap = np.maximum(sdf_ellipse(xs, ys, 58, 58, 52, 40), ys - 72)
    part(canvas, cap, rgb("#c0392b"), light=0.5)
    for cx, cy, r in ((36, 40, 7), (62, 30, 8), (84, 46, 6), (50, 56, 5), (76, 62, 4)):
        over(canvas, (1.0, 0.97, 0.9), coverage(sdf_circle(xs, ys, cx, cy, r)))
    eyes(canvas, xs, ys, [(49, 88), (67, 88)], 5.5, rgb("#3a2a1a"), angry=False)
    over(canvas, OUTLINE, coverage(sdf_segment(xs, ys, 52, 104, 64, 104, 1.4)))
    return canvas


def stone_golem() -> Any:
    w, h = 168, 192
    canvas = blank(w, h)
    xs, ys = grid(w, h)
    feet_shadow(canvas, xs, ys, 84, 182, 60)
    rock = np.kron(periodic_noise("golem", 48, 5, octaves=3), np.ones((4, 4)))[:h, :w]
    stone = rgb("#8a857c")
    for side in (-1, 1):
        part(canvas, sdf_box(xs, ys, 84 + side * 22 - 15, 140, 84 + side * 22 + 15, 182, 8), stone, texture=rock)
        part(canvas, sdf_box(xs, ys, 84 + side * 62 - 18, 70, 84 + side * 62 + 18, 150, 12), stone, texture=rock)
    part(canvas, sdf_box(xs, ys, 30, 60, 138, 150, 22), stone, light=0.5, texture=rock)
    part(canvas, sdf_box(xs, ys, 52, 18, 116, 72, 16), stone, light=0.5, texture=rock)
    for x0, y0, x1, y1 in ((60, 96, 76, 120), (100, 84, 114, 104), (70, 130, 98, 138)):
        over(canvas, OUTLINE, coverage(sdf_segment(xs, ys, x0, y0, x1, y1, 1.2)) * 0.6)
    for ex in (72, 96):
        glow = sdf_box(xs, ys, ex - 7, 40, ex + 7, 48, 2)
        over(canvas, rgb("#7fe3ff"), coverage(glow * 0.25) * 0.4)
        over(canvas, rgb("#bff4ff"), coverage(glow))
    over(canvas, rgb("#7fe3ff"), coverage(sdf_circle(xs, ys, 84, 104, 10) * 0.3) * 0.5)
    over(canvas, rgb("#bff4ff"), coverage(sdf_circle(xs, ys, 84, 104, 6)))
    return canvas


def ember_wisp() -> Any:
    w, h = 120, 152
    canvas = blank(w, h)
    xs, ys = grid(w, h)
    feet_shadow(canvas, xs, ys, 60, 144, 26)
    flicker = np.kron(periodic_noise("wisp", 48, 4, octaves=3), np.ones((4, 4)))[:h, :w]
    for scale, color, alpha in ((1.0, rgb("#e2531f"), 0.95), (0.72, rgb("#f39c12"), 1.0), (0.42, rgb("#ffe08a"), 1.0)):
        # A round base with a flickering tongue rising from it.
        flame = np.minimum(
            sdf_circle(xs, ys, 60, 96, 38 * scale),
            np.maximum(np.abs(xs - 60 + (flicker - 0.5) * 12) - (ys - (96 - 96 * scale)) * 0.4, ys - 96),
        )
        glow = coverage(flame * 0.15) * 0.25
        over(canvas, color, glow)
        over(canvas, color, coverage(flame) * alpha)
    eyes(canvas, xs, ys, [(50, 96), (70, 96)], 6.0, rgb("#7a1f0a"))
    over(canvas, OUTLINE, coverage(sdf_ellipse(xs, ys, 60, 114, 6, 4)))
    return canvas


def thorn_wolf() -> Any:
    w, h = 176, 140
    canvas = blank(w, h)
    xs, ys = grid(w, h)
    feet_shadow(canvas, xs, ys, 88, 130, 64)
    fur = np.kron(periodic_noise("wolf", 48, 6, octaves=3), np.ones((4, 4)))[:h, :w]
    coat = rgb("#5d6b78")
    for side in (-1, 1):
        part(canvas, sdf_segment(xs, ys, 88 + side * 30, 96, 88 + side * 36, 126, 9), coat, texture=fur)
        part(canvas, sdf_ellipse(xs, ys, 88 + side * 38, 127, 12, 6), rgb("#3f4a55"))
    part(canvas, sdf_ellipse(xs, ys, 88, 92, 58, 34), coat, light=0.45, texture=fur)
    for k in range(-3, 4):
        spike = np.maximum(np.abs(xs - (88 + k * 14)) * 2.2 - (ys - (38 + abs(k) * 4)), ys - (70 + abs(k) * 2))
        part(canvas, spike, rgb("#3e7a3a"), light=0.4, outline=1.2)
    head = np.minimum(sdf_ellipse(xs, ys, 88, 66, 30, 26), sdf_ellipse(xs, ys, 88, 84, 18, 14))
    for side in (-1, 1):
        ear = np.maximum(np.abs(xs - (88 + side * 22)) - (ys - 26) * 0.42, ys - 56)
        part(canvas, ear, coat, light=0.45, texture=fur)
    part(canvas, head, coat, light=0.5, texture=fur)
    part(canvas, sdf_ellipse(xs, ys, 88, 88, 10, 7), rgb("#cfd6dc"), light=0.3, outline=1.0)
    over(canvas, OUTLINE, coverage(sdf_ellipse(xs, ys, 88, 82, 5, 3.5)))
    eyes(canvas, xs, ys, [(76, 64), (100, 64)], 5.5, rgb("#f1c40f"))
    return canvas


ENEMIES: dict[str, Callable[[], Any]] = {
    "moss_blob": moss_blob,
    "cave_bat": cave_bat,
    "mushroom_imp": mushroom_imp,
    "thorn_wolf": thorn_wolf,
    "stone_golem": stone_golem,
    "ember_wisp": ember_wisp,
}

# ---------------------------------------------------------------------------------------------------------------
# Battlebacks: a backdrop (sky, far scenery; opaque) and a ground layer (the floor in perspective; transparent above
# the horizon), so a scene can reuse either half.

BW, BH = 1280, 720
HORIZON = 330


def tiled(name: str, size: int, cells: int, xs: Any, ys: Any, octaves: int = 3) -> Any:
    """A periodic noise sampled at any coordinates (wrapping)."""
    noise = periodic_noise(name, size, cells, octaves=octaves)
    return noise[np.floor(ys).astype(int) % size, np.floor(xs).astype(int) % size]


def ground(name: str, stops: list[tuple[float, Color]], horizon: int = HORIZON, detail: float = 1.0) -> Any:
    """A floor in perspective: texture coordinates shrink toward the horizon; fades into the backdrop at it."""
    canvas = blank(BW, BH)
    xs, ys = grid(BW, BH)
    below = np.maximum(ys - horizon, 1.0)
    depth = 3600.0 / below  # Distance from the camera, in texture pixels.
    u = (xs - BW / 2) * depth / 600.0 + 4096
    v = depth
    texture = tiled(name, 256, 6, u * detail, v * detail) * 0.7 + tiled(name + ":fine", 128, 8, u * 3, v * 3) * 0.3
    color = ramp(texture, stops)
    color = shade(color, -np.clip(1 - (ys - horizon) / 260.0, 0, 1) * 0.25)  # Haze darkens the far floor a little.
    alpha = np.clip((ys - horizon) / 18.0, 0, 1)
    over(canvas, color, alpha)
    return canvas


def sky(top: Color, bottom: Color, clouds: float) -> Any:
    canvas = blank(BW, BH)
    xs, ys = grid(BW, BH)
    t = np.clip(ys / (HORIZON + 40), 0, 1)[..., None]
    color = np.array(top) * (1 - t) + np.array(bottom) * t
    if clouds > 0:
        noise = tiled("clouds", 256, 4, xs * 0.6, ys * 1.6, octaves=4)
        amount = np.clip((noise - (1 - clouds)) / 0.18, 0, 1) * np.clip(1 - ys / HORIZON, 0, 1)
        color = color * (1 - amount[..., None] * 0.85) + np.array((1.0, 1.0, 1.0)) * amount[..., None] * 0.85
    over(canvas, color, np.ones((BH, BW)))
    return canvas


def ridge(canvas: Any, name: str, base_y: float, height: float, color: Color, waviness: int) -> None:
    xs, ys = grid(BW, BH)
    gen = rng(name)
    line = np.zeros(BW)
    for k in range(1, waviness + 1):
        line += np.sin(2 * np.pi * k * np.arange(BW) / BW + gen.random() * 2 * np.pi) * gen.random() / k
    line = base_y - height * (0.5 + 0.5 * line / np.max(np.abs(line)))
    over(canvas, color, np.clip(ys - line[None, :] + 0.5, 0, 1))


def treeline(canvas: Any, name: str, base_y: float, size: float, color: Color, count: int) -> None:
    xs, ys = grid(BW, BH)
    gen = rng(name)
    sdf = np.full((BH, BW), 1000.0)
    for k in range(count):
        cx = (k + gen.random()) * BW / count
        height = size * (0.8 + gen.random() * 0.5)
        crown = sdf_ellipse(xs, ys, cx, base_y - height * 0.75, size * 0.42, height * 0.5)
        trunk = sdf_box(xs, ys, cx - size * 0.06, base_y - height * 0.4, cx + size * 0.06, base_y + 10)
        sdf = np.minimum(sdf, np.minimum(crown, trunk))
    sdf = np.minimum(sdf, base_y - ys)  # The ground strip below the trees.
    over(canvas, color, coverage(sdf))


def grassland() -> tuple[Any, Any]:
    back = sky(rgb("#6fa8dc"), rgb("#cfe6f5"), 0.38)
    ridge(back, "hills_far", HORIZON + 4, 70, rgb("#9cbcb0"), 4)
    ridge(back, "hills_near", HORIZON + 12, 46, rgb("#6f9a5f"), 6)
    floor = ground("meadow", [(0.0, rgb("#4f8a36")), (0.6, rgb("#6aa84f")), (1.0, rgb("#8cc06a"))])
    return back, floor


def forest() -> tuple[Any, Any]:
    back = sky(rgb("#3d6b5a"), rgb("#a9c9a4"), 0.0)
    treeline(back, "trees_far", HORIZON + 6, 150, rgb("#5f8a6a"), 14)
    treeline(back, "trees_mid", HORIZON + 14, 210, rgb("#3f6b45"), 10)
    treeline(back, "trees_near", HORIZON + 22, 290, rgb("#24452a"), 7)
    floor = ground("forest_floor", [(0.0, rgb("#3a4a24")), (0.55, rgb("#56662f")), (1.0, rgb("#7a7a3a"))])
    return back, floor


def cave() -> tuple[Any, Any]:
    back = blank(BW, BH)
    xs, ys = grid(BW, BH)
    rock = tiled("cave_wall", 256, 5, xs * 0.8, ys * 0.8, octaves=4)
    color = ramp(rock, [(0.0, rgb("#1d1a22")), (0.6, rgb("#38323f")), (1.0, rgb("#57505e"))])
    light_pool = np.clip(1 - np.hypot((xs - BW / 2) / 700, (ys - HORIZON) / 420), 0, 1)
    color = shade(color, light_pool * 0.35 - 0.2)
    over(back, color, np.ones((BH, BW)))
    gen = rng("stalactites")
    for k in range(18):
        cx = (k + gen.random()) * BW / 18
        length = 40 + gen.random() * 140
        tip = np.maximum(np.abs(xs - cx) - (length - ys) * 0.12, ys - length)
        over(back, rgb("#2a2530"), coverage(tip))
    for cx, cy in ((260, 230), (980, 200), (640, 120)):
        over(back, rgb("#7fe3ff"), coverage(sdf_circle(xs, ys, cx, cy, 30) * 0.08) * 0.25)
        over(back, rgb("#bff4ff"), coverage(sdf_circle(xs, ys, cx, cy, 5)))
    floor = ground("cave_floor", [(0.0, rgb("#2b2730")), (0.6, rgb("#4a4450")), (1.0, rgb("#6a6370"))])
    return back, floor


BATTLEBACKS: dict[str, Callable[[], tuple[Any, Any]]] = {"grassland": grassland, "forest": forest, "cave": cave}


def generate() -> dict[str, Any]:
    images: dict[str, Any] = {}
    for name, enemy in ENEMIES.items():
        images[f"enemies/{name}.png"] = enemy()
    for name, scene in BATTLEBACKS.items():
        back, floor = scene()
        images[f"battlebacks/{name}_back.png"] = back
        images[f"battlebacks/{name}_ground.png"] = floor
    return images
