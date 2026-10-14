"""Ground, wall and object tiles, and autotile sheets in the Talewright format (spec 05).

Autotile sheets: 47-tile blob sheets (8 x 6 tiles, shape index i at column i % 8, row i // 8, the 48th cell a
preview), animated kinds with further 8 x 6 blocks to the right; wall sheets: 4 x 4 tiles indexed by the
4-neighbour mask (N 1, E 2, S 4, W 8). All textures repeat every tile, so neighboring tiles join without seams.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Callable

import numpy as np

from .common import (
    BLOB_MASKS,
    NE,
    NW,
    SE,
    SW,
    TILE,
    Color,
    E,
    N,
    S,
    W,
    blank,
    coverage,
    grid,
    lit,
    normals,
    over,
    periodic_noise,
    periodic_wave,
    ramp,
    rgb,
    rng,
    sdf_box,
    sdf_circle,
    sdf_ellipse,
    sdf_segment,
    shade,
)

# ---------------------------------------------------------------------------------------------------------------
# Tileable fill textures (TILE x TILE, opaque RGB in 0..1).

LIGHT_DIR = (-0.6, -0.8)  # Toward the light, as common.LIGHT.


def wrap_points(xs: Any, ys: Any, px: float, py: float) -> Any:
    """Distance from each pixel to (px, py) on a torus of the tile's size."""
    dx = np.abs(xs - px)
    dy = np.abs(ys - py)
    dx = np.minimum(dx, TILE - dx)
    dy = np.minimum(dy, TILE - dy)
    return np.hypot(dx, dy)


def stamp_wrapped(rgb_out: Any, x: int, y: int, color: Color, amount: float) -> None:
    rgb_out[y % TILE, x % TILE] = rgb_out[y % TILE, x % TILE] * (1 - amount) + np.array(color) * amount


def grass_texture(name: str = "grass", dark: bool = False) -> Any:
    noise = periodic_noise(name, TILE, 3)
    stops = [(0.0, rgb("#2f6b2a")), (0.5, rgb("#4f9338")), (1.0, rgb("#76b34c"))]
    if dark:
        stops = [(0.0, rgb("#234f22")), (0.5, rgb("#356d2c")), (1.0, rgb("#4f8a3a"))]
    out = ramp(noise, stops)
    gen = rng(name + ":blades")
    for _ in range(70):
        x, y = int(gen.integers(TILE)), int(gen.integers(TILE))
        light = gen.random() < 0.6
        color = rgb("#9fd06a") if light else rgb("#24521f")
        if dark:
            color = rgb("#6aa24c") if light else rgb("#183a17")
        for k in range(int(gen.integers(2, 5))):
            stamp_wrapped(out, x + (k // 2 if gen.random() < 0.3 else 0), y - k, color, 0.55 - 0.1 * k)
    return out


def dirt_texture(name: str = "dirt") -> Any:
    noise = periodic_noise(name, TILE, 3)
    out = ramp(noise, [(0.0, rgb("#6e4a2c")), (0.55, rgb("#8f6540")), (1.0, rgb("#b08458"))])
    gen = rng(name + ":pebbles")
    for _ in range(26):
        x, y = int(gen.integers(TILE)), int(gen.integers(TILE))
        color = rgb("#c9a678") if gen.random() < 0.5 else rgb("#5a3b22")
        stamp_wrapped(out, x, y, color, 0.7)
        stamp_wrapped(out, x + 1, y, color, 0.4)
    return out


def sand_texture(name: str = "sand") -> Any:
    noise = periodic_noise(name, TILE, 2, octaves=2)
    out = ramp(noise, [(0.0, rgb("#cfae72")), (1.0, rgb("#e8cf96"))])
    speckle = rng(name + ":speckle").random((TILE, TILE))
    out = shade(out, (speckle - 0.5) * 0.12)
    return out


def water_texture(frame: int, frames: int = 3, name: str = "water") -> Any:
    xs, ys = grid(TILE, TILE)
    noise = periodic_noise(name, TILE, 2, octaves=3)
    out = ramp(noise, [(0.0, rgb("#1f4f8f")), (0.6, rgb("#2f6db4")), (1.0, rgb("#4a8fd0"))])
    phase = 2 * np.pi * frame / frames
    wobble = periodic_noise(name + ":wobble", TILE, 2, octaves=2) * 2 * np.pi
    waves = np.sin(2 * np.pi * (2 * xs + ys) / TILE + phase + wobble)
    crest = np.clip((waves - 0.86) / 0.14, 0, 1) * 0.45
    out = out * (1 - crest[..., None]) + np.array(rgb("#bfe2fb")) * crest[..., None]
    return out


def cobble_texture(name: str = "cobble") -> Any:
    """Rounded stones on a jittered 4 x 4 grid, each drawn with its wrapped copies so the tile repeats seamlessly."""
    xs, ys = grid(TILE, TILE)
    gen = rng(name)
    fields = []
    tones = []
    for row in range(4):
        for col in range(4):
            cx = (col + 0.5) * 12 + (gen.random() - 0.5) * 5 + (row % 2) * 6
            cy = (row + 0.5) * 12 + (gen.random() - 0.5) * 4
            rx, ry = 5.6 + gen.random() * 1.6, 5.2 + gen.random() * 1.4
            copies = [
                sdf_ellipse(xs, ys, cx + dx, cy + dy, rx, ry) for dx in (-TILE, 0, TILE) for dy in (-TILE, 0, TILE)
            ]
            fields.append(np.min(np.stack(copies), axis=0))
            tones.append(gen.random())
    stack = np.stack(fields)
    sdf = np.min(stack, axis=0)
    nearest = np.argmin(stack, axis=0)
    base = ramp(np.array(tones)[nearest], [(0.0, rgb("#7a766f")), (1.0, rgb("#aaa59b"))])
    grain = periodic_noise(name + ":grain", TILE, 6, octaves=2)
    nx, ny = normals(sdf)
    bevel = np.clip(1 - (-sdf) / 3.5, 0, 1)  # The lighting acts near each stone's rim; its top stays flat.
    base = shade(base, lit(nx, ny) * 0.55 * bevel + (grain - 0.5) * 0.14)
    base = shade(base, -np.clip(1.2 + sdf, 0, 1) * 0.35)  # A darker rim against the mortar.
    out = np.broadcast_to(np.array(rgb("#4a4744")), (TILE, TILE, 3)).copy()
    stone = coverage(sdf)[..., None]
    return out * (1 - stone) + base * stone


def wood_floor_texture(name: str = "wood_floor") -> Any:
    xs, ys = grid(TILE, TILE)
    gen = rng(name)
    rows = (ys // 12).astype(int)  # Four planks per tile, each the tile's length, joints staggered.
    joints = gen.integers(0, TILE, size=4)
    tones = gen.random(4)
    noise = periodic_noise(name + ":grain", TILE, 3, octaves=2)
    streaks = 0.5 + 0.5 * np.sin(2 * np.pi * ys / 3.0 + noise * 5.0)
    base = ramp(tones[rows] * 0.55 + streaks * 0.3 + noise * 0.15, [(0.0, rgb("#7a4f2b")), (1.0, rgb("#b07b44"))])
    local_y = ys % 12
    base = shade(base, np.where(local_y < 1.5, 0.12, 0) + np.where(local_y > 10.5, -0.2, 0))
    seam = (local_y < 1) | (np.abs(((xs - joints[rows]) % TILE)) < 1)
    base[seam] = base[seam] * 0.55
    return base


def stone_tiles_texture() -> Any:
    xs, ys = grid(TILE, TILE)
    checker = ((xs // 24).astype(int) + (ys // 24).astype(int)) % 2
    noise = periodic_noise("stone_tiles", TILE, 3)
    base = ramp(noise * 0.5 + checker * 0.5, [(0.0, rgb("#8e8a84")), (1.0, rgb("#b9b5ac"))])
    seam = ((xs % 24) < 1) | ((ys % 24) < 1)
    base[seam] = base[seam] * 0.7
    return base


# ---------------------------------------------------------------------------------------------------------------
# Blob autotiles.


@dataclass(frozen=True)
class EdgeStyle:
    margin: float = 7.0  # Distance of an edge from the tile border.
    radius: float = 9.0  # Rounding of outer corners.
    roughness: float = 1.6  # Amplitude of the edge waves.
    band: float = 3.5  # Width of the lit or shaded rim.
    light: float = 0.35
    raised: bool = True  # Raised terrain catches the light on its upper-left rims; sunken terrain the opposite.
    outline: Color = (0.0, 0.0, 0.0)
    outline_alpha: float = 0.45


def blob_tile(mask: int, fill: Any, style: EdgeStyle, waves: dict[str, Callable[[Any], Any]]) -> Any:
    """One autotile shape: fill inside the region the mask describes, transparent outside (the layer below shows)."""
    xs, ys = grid(TILE, TILE)
    half = TILE / 2
    inside = np.full((TILE, TILE), 100.0)  # Signed distance, positive inside.
    for top in (True, False):
        for left in (True, False):
            quadrant = ((ys < half) if top else (ys >= half)) & ((xs < half) if left else (xs >= half))
            vertical = bool(mask & (N if top else S))
            horizontal = bool(mask & (W if left else E))
            diagonal = bool(mask & ((NW if left else NE) if top else (SW if left else SE)))
            ox = xs if left else TILE - xs  # Distance from this quadrant's outer side.
            oy = ys if top else TILE - ys
            m_side = style.margin + waves["left" if left else "right"](ys)
            m_end = style.margin + waves["top" if top else "bottom"](xs)
            if vertical and horizontal and diagonal:
                q = np.full((TILE, TILE), 100.0)
            elif vertical and horizontal:
                # Inner corner: a notch whose sides continue the edges of the two neighbors.
                dx, dy = ox - m_side, oy - m_end
                q = np.where((dx > 0) & (dy > 0), np.hypot(dx, dy), np.maximum(dx, dy))
            elif horizontal:
                q = oy - m_end
            elif vertical:
                q = ox - m_side
            else:
                cx, cy = m_side + style.radius, m_end + style.radius
                q = np.where(
                    (ox < cx) & (oy < cy),
                    style.radius - np.hypot(ox - cx, oy - cy),
                    np.minimum(ox - m_side, oy - m_end),
                )
            inside[quadrant] = q[quadrant]
    tile = blank(TILE, TILE)
    nx, ny = normals(-inside)
    facing = lit(nx, ny) * (1 if style.raised else -1)
    rim = np.clip(1 - inside / style.band, 0, 1)
    color = shade(fill, facing * style.light * rim)
    edge_line = np.clip(1.4 - inside, 0, 1) * style.outline_alpha
    color = color * (1 - edge_line[..., None]) + np.array(style.outline) * edge_line[..., None]
    over(tile, color, coverage(-inside))
    return tile


def blob_sheet(name: str, fills: list[Any], style: EdgeStyle) -> Any:
    """The 8 x 6 blob sheet of a terrain; one block per animation frame, side by side."""
    waves = {
        side: periodic_wave(f"{name}:{side}", TILE, style.roughness) for side in ("top", "bottom", "left", "right")
    }
    sheet = blank(8 * TILE * len(fills), 6 * TILE)
    for f, fill in enumerate(fills):
        for index, mask in enumerate(BLOB_MASKS):
            x, y = (f * 8 + index % 8) * TILE, (index // 8) * TILE
            sheet[y : y + TILE, x : x + TILE] = blob_tile(mask, fill, style, waves)
        # Cell 47: a preview of the terrain (its full tile with a light frame).
        preview = blob_tile(255, fill, style, waves)
        frame_x, frame_y = grid(TILE, TILE)
        border = np.minimum(np.minimum(frame_x, TILE - frame_x), np.minimum(frame_y, TILE - frame_y))
        over(preview, np.array((1.0, 1.0, 1.0)), np.clip(2.0 - border, 0, 1) * 0.5)
        x, y = (f * 8 + 7) * TILE, 5 * TILE
        sheet[y : y + TILE, x : x + TILE] = preview
    return sheet


def autotile_sheets() -> dict[str, Any]:
    raised = EdgeStyle(outline=rgb("#1d3a18"))
    path = EdgeStyle(margin=6.0, radius=8.0, roughness=1.8, light=0.25, raised=False, outline=rgb("#4a321c"))
    beach = EdgeStyle(margin=6.0, radius=10.0, roughness=2.0, light=0.2, raised=False, outline=rgb("#a88a52"))
    water = EdgeStyle(
        margin=8.0,
        radius=10.0,
        roughness=1.4,
        band=4.0,
        light=0.45,
        raised=False,
        outline=rgb("#e6f4ff"),
        outline_alpha=0.7,
    )
    return {
        "autotiles/grass.png": blob_sheet("grass", [grass_texture()], raised),
        "autotiles/dirt_path.png": blob_sheet("dirt_path", [dirt_texture()], path),
        "autotiles/sand.png": blob_sheet("sand", [sand_texture()], beach),
        "autotiles/water.png": blob_sheet("water", [water_texture(f) for f in range(3)], water),
    }


# ---------------------------------------------------------------------------------------------------------------
# Wall autotiles (4 x 4 sheets by the 4-neighbour mask).


def brick_texture(name: str, brick_w: int, brick_h: int, colors: tuple[Color, Color], mortar: Color) -> Any:
    xs, ys = grid(TILE, TILE)
    gen = rng(name)
    rows = (ys // brick_h).astype(int)
    shifted = (xs + (rows % 2) * (brick_w / 2)) % TILE
    cols = (shifted // brick_w).astype(int)
    per_row = TILE // brick_w
    tones = gen.random((TILE // brick_h) * per_row + per_row)
    tone = tones[rows * per_row + cols]
    grain = periodic_noise(name + ":grain", TILE, 4, octaves=2)
    base = ramp(tone * 0.7 + grain * 0.3, [(0.0, colors[0]), (1.0, colors[1])])
    ly, lx = ys % brick_h, shifted % brick_w
    base = shade(base, np.where(ly < 1.5, 0.15, 0) + np.where(ly > brick_h - 1.5, -0.15, 0))
    joint = (ly < 1) | (lx < 1)
    base[joint] = np.array(mortar)
    return base


def plank_texture(name: str) -> Any:
    xs, ys = grid(TILE, TILE)
    gen = rng(name)
    cols = (xs // 12).astype(int)
    tones = gen.random(4)
    grain = periodic_noise(name + ":grain", TILE, 2, octaves=3)
    streak = np.sin((ys / TILE) * 2 * np.pi * 3 + grain * 6) * 0.5 + 0.5
    base = ramp(tones[cols] * 0.5 + streak * 0.5, [(0.0, rgb("#6b4426")), (1.0, rgb("#9a6a3c"))])
    seam = (xs % 12) < 1
    base[seam] = base[seam] * 0.55
    return base


def roof_texture(name: str) -> Any:
    xs, ys = grid(TILE, TILE)
    rows = (ys // 8).astype(int)
    shifted = (xs + (rows % 2) * 6) % TILE
    lx, ly = shifted % 12, ys % 8
    scallop = np.hypot(lx - 6, ly - 0) / 7.0
    tone = periodic_noise(name, TILE, 3)
    base = ramp(tone, [(0.0, rgb("#8c2f22")), (1.0, rgb("#b8483a"))])
    base = shade(base, 0.18 - np.clip(scallop, 0, 1) * 0.4)
    edge = np.abs(scallop - 1.0) < 0.12
    base[edge] = base[edge] * 0.55
    return base


def wall_tile(mask: int, face: Any, cap: Color, cap_height: float) -> Any:
    xs, ys = grid(TILE, TILE)
    color = face.copy()
    if not mask & N:  # A top ledge where no wall continues above.
        ledge = ys < cap_height
        color[ledge] = np.array(cap)
        color = shade(
            color, np.where(ledge & (ys < 2), 0.25, 0) + np.where((ys >= cap_height) & (ys < cap_height + 2), -0.35, 0)
        )
    if not mask & S:  # A darker foot where the wall meets the ground.
        color = shade(color, np.where(ys > TILE - 4, -0.35, 0))
    for missing, side in ((W, xs), (E, TILE - xs)):
        if not mask & missing:
            color = shade(color, np.where(side < 3, -0.3, 0) + np.where((side >= 3) & (side < 5), 0.12, 0))
    tile = blank(TILE, TILE)
    over(tile, color, np.ones((TILE, TILE)))
    return tile


def wall_sheet(face: Any, cap: Color, cap_height: float = 7.0) -> Any:
    sheet = blank(4 * TILE, 4 * TILE)
    for mask in range(16):
        x, y = (mask % 4) * TILE, (mask // 4) * TILE
        sheet[y : y + TILE, x : x + TILE] = wall_tile(mask, face, cap, cap_height)
    return sheet


def wall_sheets() -> dict[str, Any]:
    stone = brick_texture("stone_wall", 16, 8, (rgb("#6f6b66"), rgb("#9d978d")), rgb("#3f3c39"))
    brick = brick_texture("red_brick", 12, 6, (rgb("#8a3e2c"), rgb("#b45a3f")), rgb("#d8c9b0"))
    return {
        "walls/stone_wall.png": wall_sheet(stone, rgb("#b5b0a6")),
        "walls/brick_wall.png": wall_sheet(brick, rgb("#c9b9a0")),
        "walls/wood_wall.png": wall_sheet(plank_texture("wood_wall"), rgb("#b88a55")),
        "walls/roof.png": wall_sheet(roof_texture("roof"), rgb("#d0644f"), cap_height=5.0),
    }


# ---------------------------------------------------------------------------------------------------------------
# Object tiles. Each painter draws into a tile-sized RGBA array (transparent background).

Painter = Callable[[Any], None]


def body(tile: Any, sdf: Any, base: Color, light: float = 0.4, outline: Color = (0.08, 0.07, 0.06)) -> None:
    """A shape with lighting from its own outline normals, a dark outline and anti-aliased edges."""
    nx, ny = normals(sdf)
    depth = np.clip(-sdf / 6.0, 0, 1)
    color = shade(np.broadcast_to(np.array(base), (*sdf.shape, 3)).copy(), lit(nx, ny) * light * (1 - depth * 0.5))
    over(tile, outline, coverage(sdf - 1.0) * 0.85)
    over(tile, color, coverage(sdf))


def ground_shadow(tile: Any, cx: float, cy: float, rx: float, ry: float) -> None:
    xs, ys = grid(TILE, TILE)
    over(tile, (0.0, 0.0, 0.0), coverage(sdf_ellipse(xs, ys, cx, cy, rx, ry) * 2) * 0.28)


def leafy(tile: Any, circles: list[tuple[float, float, float]], name: str, dark: Color, light: Color) -> None:
    xs, ys = grid(TILE, TILE)
    sdf = np.min(np.stack([sdf_circle(xs, ys, cx, cy, r) for cx, cy, r in circles]), axis=0)
    texture = periodic_noise(name, TILE, 6, octaves=2)
    base = ramp(texture, [(0.0, dark), (1.0, light)])
    nx, ny = normals(sdf)
    base = shade(base, lit(nx, ny) * 0.35 + np.clip(-sdf / 10, 0, 1) * 0.05)
    over(tile, (0.06, 0.12, 0.05), coverage(sdf - 1.0) * 0.9)
    over(tile, base, coverage(sdf))


def fill_tile(texture: Any) -> Painter:
    def paint(tile: Any) -> None:
        over(tile, texture, np.ones((TILE, TILE)))

    return paint


def flowers(petal: Color, name: str) -> Painter:
    def paint(tile: Any) -> None:
        xs, ys = grid(TILE, TILE)
        gen = rng(name)
        for _ in range(7):
            cx, cy = 8 + gen.random() * 32, 10 + gen.random() * 30
            over(tile, rgb("#2e6b2a"), coverage(sdf_segment(xs, ys, cx, cy, cx, cy + 5, 0.8)))
            for k in range(5):
                angle = 2 * np.pi * k / 5
                over(tile, petal, coverage(sdf_circle(xs, ys, cx + 2.2 * np.cos(angle), cy + 2.2 * np.sin(angle), 1.6)))
            over(tile, rgb("#f5d24a"), coverage(sdf_circle(xs, ys, cx, cy, 1.2)))

    return paint


def rocks(tile: Any) -> None:
    xs, ys = grid(TILE, TILE)
    for cx, cy, rx, ry in ((18, 28, 9, 7), (31, 32, 6, 5), (26, 20, 4, 3.5)):
        ground_shadow(tile, cx + 1, cy + ry - 1, rx, ry * 0.5)
        body(tile, sdf_ellipse(xs, ys, cx, cy, rx, ry), rgb("#8e8a84"), light=0.5)


def tuft(tile: Any) -> None:
    xs, ys = grid(TILE, TILE)
    for k in range(9):
        a = -0.9 + k * 0.22
        x0 = 24 + (k - 4) * 1.5
        over(
            tile,
            rgb("#3f8a33" if k % 2 else "#5aa83f"),
            coverage(sdf_segment(xs, ys, x0, 34, x0 + 9 * np.sin(a), 34 - 12 * np.cos(a), 1.1)),
        )


def mushrooms(tile: Any) -> None:
    xs, ys = grid(TILE, TILE)
    for cx, cy, s in ((19, 30, 1.0), (29, 33, 0.8)):
        over(tile, rgb("#e9dfc9"), coverage(sdf_box(xs, ys, cx - 2 * s, cy - 4 * s, cx + 2 * s, cy + 4 * s, 1.5)))
        body(tile, sdf_ellipse(xs, ys, cx, cy - 5 * s, 7 * s, 4.5 * s), rgb("#c0392b"), light=0.4)
        for dx, dy in ((-3, -1), (2, -2), (0, 1)):
            over(tile, (1.0, 1.0, 1.0), coverage(sdf_circle(xs, ys, cx + dx * s, cy - 5 * s + dy * s, 1.0 * s)))


def leaves(tile: Any) -> None:
    xs, ys = grid(TILE, TILE)
    gen = rng("leaves")
    for _ in range(9):
        cx, cy, a = 8 + gen.random() * 32, 8 + gen.random() * 32, gen.random() * np.pi
        color = rgb("#c26a2a") if gen.random() < 0.5 else rgb("#d9a43a")
        dx, dy = 3 * np.cos(a), 3 * np.sin(a)
        over(tile, color, coverage(sdf_segment(xs, ys, cx - dx, cy - dy, cx + dx, cy + dy, 1.5)))


def puddle(tile: Any) -> None:
    xs, ys = grid(TILE, TILE)
    sdf = sdf_ellipse(xs, ys, 24, 26, 15, 8)
    over(tile, rgb("#3a5f86"), coverage(sdf) * 0.75)
    over(tile, rgb("#bfe2fb"), coverage(sdf_segment(xs, ys, 17, 24, 25, 22, 0.8)) * 0.7)


def cracks(tile: Any) -> None:
    xs, ys = grid(TILE, TILE)
    path = [(10, 30), (17, 26), (22, 28), (29, 21), (36, 22), (40, 15)]
    for (ax, ay), (bx, by) in zip(path, path[1:]):
        over(tile, (0.12, 0.1, 0.08), coverage(sdf_segment(xs, ys, ax, ay, bx, by, 0.7)) * 0.7)
    over(tile, (0.12, 0.1, 0.08), coverage(sdf_segment(xs, ys, 22, 28, 24, 35, 0.6)) * 0.6)


def round_tree(top: bool) -> Painter:
    """A two-tile tree: the crown is the upper tile (drawn above characters), the trunk the lower one."""

    def paint(tile: Any) -> None:
        xs, ys = grid(TILE, TILE)
        offset = 0 if top else -TILE  # Coordinates in the two-tile picture.
        if not top:
            ground_shadow(tile, 25, 40, 16, 5)
            body(tile, sdf_box(xs, ys, 19, 0, 29, 40, 3), rgb("#6b4426"), light=0.3)
        circles = [(24, 30, 18), (14, 38, 11), (34, 38, 11), (24, 46, 12)]
        crown: list[tuple[float, float, float]] = [(cx, cy + offset, r) for cx, cy, r in circles]
        leafy(tile, crown, "round_tree", rgb("#2b6127"), rgb("#6fae45"))

    return paint


def pine_tree(top: bool) -> Painter:
    def paint(tile: Any) -> None:
        xs, ys = grid(TILE, TILE)
        offset = 0 if top else -TILE
        if not top:
            ground_shadow(tile, 25, 41, 14, 4.5)
            body(tile, sdf_box(xs, ys, 21, 18, 27, 41, 2), rgb("#5e3a20"), light=0.3)
        layers = []
        for k, (y, w) in enumerate(((10, 9), (24, 13), (38, 17), (52, 20), (64, 22))):
            yy = y + offset
            tri = np.maximum(np.abs(xs - 24) * (16.0 / w) - (ys - (yy - 14)), ys - (yy + 2))
            layers.append(tri)
        sdf = np.min(np.stack(layers), axis=0)
        texture = periodic_noise("pine", TILE, 6, octaves=2)
        base = ramp(texture, [(0.0, rgb("#1d4a2a")), (1.0, rgb("#3f7d45"))])
        nx, ny = normals(sdf)
        base = shade(base, lit(nx, ny) * 0.3)
        over(tile, (0.05, 0.1, 0.06), coverage(sdf - 1.0) * 0.9)
        over(tile, base, coverage(sdf))

    return paint


def lamp(top: bool) -> Painter:
    def paint(tile: Any) -> None:
        xs, ys = grid(TILE, TILE)
        if top:
            body(tile, sdf_box(xs, ys, 22.5, 30, 25.5, 48), rgb("#2f2f35"), light=0.2)
            glow = coverage(sdf_circle(xs, ys, 24, 24, 13) * 0.25)
            over(tile, rgb("#ffd88a"), glow * 0.35)
            body(tile, sdf_box(xs, ys, 18, 16, 30, 31, 2), rgb("#3a3a42"), light=0.25)
            over(tile, rgb("#ffe7a6"), coverage(sdf_box(xs, ys, 20.5, 18.5, 27.5, 28.5, 1)))
            body(tile, sdf_box(xs, ys, 16, 12, 32, 17, 2), rgb("#2b2b31"), light=0.3)
        else:
            ground_shadow(tile, 26, 42, 8, 3)
            body(tile, sdf_box(xs, ys, 22.5, 0, 25.5, 40), rgb("#2f2f35"), light=0.2)
            body(tile, sdf_box(xs, ys, 19, 37, 29, 42, 1.5), rgb("#3a3a42"), light=0.3)

    return paint


def signpost(tile: Any) -> None:
    xs, ys = grid(TILE, TILE)
    ground_shadow(tile, 25, 42, 9, 3)
    body(tile, sdf_box(xs, ys, 22, 20, 26, 42, 1), rgb("#6b4426"), light=0.3)
    body(tile, sdf_box(xs, ys, 10, 10, 38, 24, 2), rgb("#a8743f"), light=0.35)
    for y in (14.5, 18.5):
        over(tile, (0.25, 0.16, 0.08), coverage(sdf_segment(xs, ys, 15, y, 33, y, 0.6)) * 0.8)


def bush(tile: Any) -> None:
    ground_shadow(tile, 25, 38, 17, 5)
    leafy(tile, [(17, 28, 10), (31, 28, 10), (24, 22, 11), (24, 31, 11)], "bush", rgb("#285c25"), rgb("#62a242"))


def stump(tile: Any) -> None:
    xs, ys = grid(TILE, TILE)
    ground_shadow(tile, 25, 36, 14, 4)
    body(tile, sdf_box(xs, ys, 13, 22, 35, 36, 4), rgb("#6b4426"), light=0.3)
    body(tile, sdf_ellipse(xs, ys, 24, 22, 11, 5), rgb("#c99a62"), light=0.2)
    over(tile, rgb("#8a5f35"), coverage(np.abs(sdf_ellipse(xs, ys, 24, 22, 6, 2.6)) - 0.5) * 0.8)


def barrel(tile: Any) -> None:
    xs, ys = grid(TILE, TILE)
    ground_shadow(tile, 25, 41, 12, 4)
    body(tile, sdf_box(xs, ys, 13, 12, 35, 40, 6), rgb("#8a5a32"), light=0.35)
    for y in (17, 34):
        over(tile, rgb("#3a3a40"), coverage(sdf_box(xs, ys, 13, y - 1.5, 35, y + 1.5, 1)))
    body(tile, sdf_ellipse(xs, ys, 24, 12, 10, 3.5), rgb("#a7743f"), light=0.2)


def crate(tile: Any) -> None:
    xs, ys = grid(TILE, TILE)
    ground_shadow(tile, 26, 41, 15, 4)
    body(tile, sdf_box(xs, ys, 10, 12, 38, 40, 1.5), rgb("#a8743f"), light=0.3)
    for ax, ay, bx, by in ((12, 14, 36, 38), (12, 38, 36, 14)):
        over(tile, rgb("#7a4f2b"), coverage(sdf_segment(xs, ys, ax, ay, bx, by, 1.4)))
    over(tile, rgb("#6b4426"), coverage(np.abs(sdf_box(xs, ys, 12, 14, 36, 38)) - 1.0))


def chest(tile: Any) -> None:
    xs, ys = grid(TILE, TILE)
    ground_shadow(tile, 25, 40, 16, 4)
    body(tile, sdf_box(xs, ys, 8, 20, 40, 39, 2), rgb("#8a5a32"), light=0.3)
    body(tile, sdf_box(xs, ys, 8, 12, 40, 22, 5), rgb("#a26a3a"), light=0.4)
    for x in (12, 36):
        over(tile, rgb("#d4a63a"), coverage(sdf_box(xs, ys, x - 1.5, 12, x + 1.5, 39)))
    body(tile, sdf_box(xs, ys, 21, 19, 27, 27, 1), rgb("#e0b54a"), light=0.5)


def pot(tile: Any) -> None:
    xs, ys = grid(TILE, TILE)
    ground_shadow(tile, 25, 40, 11, 4)
    sdf = np.minimum(sdf_circle(xs, ys, 24, 29, 11), sdf_box(xs, ys, 19, 13, 29, 22, 2))
    body(tile, sdf, rgb("#b0663a"), light=0.45)
    body(tile, sdf_ellipse(xs, ys, 24, 14, 7, 2.5), rgb("#7a3f22"), light=0.1)


def fence(horizontal: bool) -> Painter:
    def paint(tile: Any) -> None:
        xs, ys = grid(TILE, TILE)
        wood = rgb("#9a6a3c")
        if horizontal:
            for y in (20, 31):
                body(tile, sdf_box(xs, ys, -2, y - 2, 50, y + 2, 1), wood, light=0.3)
            for x in (8, 40):
                body(tile, sdf_box(xs, ys, x - 3, 12, x + 3, 40, 1.5), wood, light=0.35)
        else:
            ground_shadow(tile, 25, 40, 7, 3)
            body(tile, sdf_box(xs, ys, 21, 10, 27, 40, 1.5), wood, light=0.35)
            body(tile, sdf_box(xs, ys, 22.5, -2, 25.5, 50, 1), wood, light=0.3)

    return paint


def steps(tile: Any) -> None:
    xs, ys = grid(TILE, TILE)
    for k in range(4):
        y0 = k * 12
        face = shade(
            np.broadcast_to(np.array(rgb("#9d978d")), (TILE, TILE, 3)).copy(), np.full((TILE, TILE), -0.08 * k)
        )
        over(tile, face, coverage(sdf_box(xs, ys, 0, y0, TILE, y0 + 12)))
        over(tile, rgb("#cfc9bd"), coverage(sdf_box(xs, ys, 0, y0, TILE, y0 + 2)))
        over(tile, rgb("#55514c"), coverage(sdf_box(xs, ys, 0, y0 + 10.5, TILE, y0 + 12)))


def door(tile: Any) -> None:
    xs, ys = grid(TILE, TILE)
    body(tile, sdf_box(xs, ys, 9, 4, 39, 48, 3), rgb("#5a3a20"), light=0.2)
    over(tile, plank_texture("door"), coverage(sdf_box(xs, ys, 12, 7, 36, 48, 2)))
    over(tile, rgb("#e0b54a"), coverage(sdf_circle(xs, ys, 31, 28, 1.8)))


def window(lit_inside: bool) -> Painter:
    def paint(tile: Any) -> None:
        xs, ys = grid(TILE, TILE)
        body(tile, sdf_box(xs, ys, 9, 9, 39, 37, 2), rgb("#6b4426"), light=0.3)
        glass = rgb("#ffd98a") if lit_inside else rgb("#5d86b0")
        over(tile, glass, coverage(sdf_box(xs, ys, 12, 12, 36, 34, 1)))
        if not lit_inside:
            over(tile, rgb("#cfe6fb"), coverage(sdf_segment(xs, ys, 15, 26, 21, 15, 1.0)) * 0.7)
        for frame in (sdf_box(xs, ys, 22.5, 12, 25.5, 34), sdf_box(xs, ys, 12, 21.5, 36, 24.5)):
            over(tile, rgb("#6b4426"), coverage(frame))
        body(tile, sdf_box(xs, ys, 7, 36, 41, 40, 1), rgb("#8a5a32"), light=0.3)

    return paint


def flower_box(tile: Any) -> None:
    flowers(rgb("#e05a6a"), "flower_box")(tile)
    xs, ys = grid(TILE, TILE)
    body(tile, sdf_box(xs, ys, 6, 34, 42, 44, 2), rgb("#8a5a32"), light=0.3)


def table(tile: Any) -> None:
    xs, ys = grid(TILE, TILE)
    ground_shadow(tile, 25, 40, 18, 4)
    for x in (11, 37):
        body(tile, sdf_box(xs, ys, x - 2, 22, x + 2, 41, 1), rgb("#6b4426"), light=0.2)
    body(tile, sdf_box(xs, ys, 5, 12, 43, 28, 3), rgb("#a8743f"), light=0.35)


def chair(tile: Any) -> None:
    xs, ys = grid(TILE, TILE)
    ground_shadow(tile, 25, 41, 10, 3)
    body(tile, sdf_box(xs, ys, 15, 8, 33, 26, 2), rgb("#8a5a32"), light=0.3)
    body(tile, sdf_box(xs, ys, 14, 24, 34, 32, 2), rgb("#a26a3a"), light=0.35)
    for x in (16, 32):
        body(tile, sdf_box(xs, ys, x - 1.5, 30, x + 1.5, 41, 1), rgb("#6b4426"), light=0.2)


def bookshelf(top: bool) -> Painter:
    def paint(tile: Any) -> None:
        xs, ys = grid(TILE, TILE)
        y0, y1 = (6.0, 48.0) if top else (0.0, 42.0)
        body(tile, sdf_box(xs, ys, 4, y0, 44, y1, 2 if top else 0), rgb("#6b4426"), light=0.25)
        gen = rng("bookshelf_top" if top else "bookshelf_bottom")
        shelves = (14.0, 30.0, 46.0) if top else (14.0, 30.0)
        for shelf in shelves:
            x = 7.0
            while x < 40:
                width = 2.5 + gen.random() * 2.5
                height = 9 + gen.random() * 3
                color = (rgb("#8e2f2f"), rgb("#2f5f8e"), rgb("#3f7d45"), rgb("#c99a3a"))[int(gen.integers(4))]
                over(tile, color, coverage(sdf_box(xs, ys, x, shelf - height, x + width, shelf, 0.5)))
                x += width + 0.8
            over(tile, rgb("#4f3019"), coverage(sdf_box(xs, ys, 4, shelf, 44, shelf + 2)))

    return paint


def bed(top: bool) -> Painter:
    def paint(tile: Any) -> None:
        xs, ys = grid(TILE, TILE)
        if top:
            body(tile, sdf_box(xs, ys, 6, 4, 42, 48), rgb("#6b4426"), light=0.25)
            body(tile, sdf_box(xs, ys, 9, 10, 39, 48), rgb("#ece6da"), light=0.2)
            body(tile, sdf_box(xs, ys, 12, 12, 36, 24, 4), rgb("#fbf8f1"), light=0.3)
        else:
            body(tile, sdf_box(xs, ys, 6, 0, 42, 40, 3), rgb("#6b4426"), light=0.25)
            body(tile, sdf_box(xs, ys, 8, 0, 40, 34, 3), rgb("#3f6aa0"), light=0.35)
            over(tile, rgb("#5f8ac0"), coverage(sdf_box(xs, ys, 8, 2, 40, 6)))

    return paint


def rug(tile: Any) -> None:
    xs, ys = grid(TILE, TILE)
    over(tile, rgb("#8e2f2f"), coverage(sdf_box(xs, ys, 4, 8, 44, 40, 2)))
    over(tile, rgb("#d9a43a"), coverage(np.abs(sdf_box(xs, ys, 8, 12, 40, 36, 1)) - 1.0))
    over(tile, rgb("#d9a43a"), coverage(sdf_ellipse(xs, ys, 24, 24, 6, 4)))


def outdoor_sheet() -> Any:
    rows: list[list[Painter]] = [
        [
            fill_tile(grass_texture()),
            fill_tile(grass_texture("grass_dark", dark=True)),
            fill_tile(dirt_texture()),
            fill_tile(sand_texture()),
            fill_tile(cobble_texture()),
            fill_tile(wood_floor_texture()),
            fill_tile(stone_tiles_texture()),
            fill_tile(water_texture(0)),
        ],
        [
            flowers(rgb("#d94a4a"), "flowers_red"),
            flowers(rgb("#f2c94c"), "flowers_yellow"),
            rocks,
            tuft,
            mushrooms,
            leaves,
            puddle,
            cracks,
        ],
        [round_tree(True), pine_tree(True), lamp(True), signpost, bush, stump, barrel, crate],
        [round_tree(False), pine_tree(False), lamp(False), chest, pot, fence(True), fence(False), steps],
    ]
    return paint_sheet(rows)


def building_sheet() -> Any:
    rows: list[list[Painter]] = [
        [door, window(False), window(True), flower_box, table, chair, rug, fill_tile(wood_floor_texture("parquet"))],
        [bookshelf(True), bed(True), fill_tile(stone_tiles_texture())] + [lambda tile: None] * 5,
        [bookshelf(False), bed(False)] + [lambda tile: None] * 6,
    ]
    return paint_sheet(rows)


def paint_sheet(rows: list[list[Painter]]) -> Any:
    sheet = blank(8 * TILE, len(rows) * TILE)
    for r, row in enumerate(rows):
        for c, painter in enumerate(row):
            tile = blank(TILE, TILE)
            painter(tile)
            sheet[r * TILE : (r + 1) * TILE, c * TILE : (c + 1) * TILE] = tile
    return sheet


def generate() -> dict[str, Any]:
    """Every tile image, by path relative to the generated asset folder."""
    images: dict[str, Any] = {}
    images.update(autotile_sheets())
    images.update(wall_sheets())
    images["tiles/outdoor.png"] = outdoor_sheet()
    images["tiles/buildings.png"] = building_sheet()
    return images
