"""Character sheets and faces (spec 05, task 8.7 format).

A character sheet holds one character: rows are the facing directions up, right, down, left; columns are the walk
frames (4), column 0 the standing pose; frames are 48 x 48 pixels. A face is one 144 x 144 portrait per image.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any

import numpy as np

from .common import (
    TILE,
    Color,
    blank,
    coverage,
    grid,
    lit,
    normals,
    over,
    rgb,
    sdf_box,
    sdf_circle,
    sdf_ellipse,
    sdf_segment,
    shade,
)

OUTLINE: Color = (0.1, 0.07, 0.07)
FRAMES = 4
DIRECTIONS = ("up", "right", "down", "left")
STEP = (0, 1, 0, -1)  # Which foot is forward in each walk frame.
BOB = (0, -1, 0, -1)  # The body rises on the steps.


@dataclass(frozen=True)
class Look:
    skin: Color
    hair: Color
    hair_style: str  # short, long, spiky, bun
    outfit: Color
    trim: Color
    legs: Color  # Trousers, or the skirt's color when robe is set.
    shoes: Color
    eyes: Color
    robe: bool = False  # A long robe or dress instead of trousers.
    hat: str = ""  # mage, helmet, bandana
    hat_color: Color = (0.0, 0.0, 0.0)


LOOKS: dict[str, Look] = {
    "hero": Look(
        rgb("#f1c7a1"),
        rgb("#6b3f1f"),
        "spiky",
        rgb("#3f6fb5"),
        rgb("#e2c044"),
        rgb("#4a3a2a"),
        rgb("#3a2a1a"),
        rgb("#2f5a9a"),
    ),
    "mage": Look(
        rgb("#f3d1b0"),
        rgb("#9c93c4"),
        "long",
        rgb("#6a3fa0"),
        rgb("#e2c044"),
        rgb("#5a3290"),
        rgb("#3a2a4a"),
        rgb("#6a4aa0"),
        robe=True,
        hat="mage",
        hat_color=rgb("#4b2d7a"),
    ),
    "knight": Look(
        rgb("#e9b88f"),
        rgb("#2a2a2a"),
        "short",
        rgb("#9aa3ad"),
        rgb("#c0392b"),
        rgb("#6e7780"),
        rgb("#4a4f55"),
        rgb("#3a3a3a"),
        hat="helmet",
        hat_color=rgb("#aeb6bf"),
    ),
    "villager_man": Look(
        rgb("#d9a67a"),
        rgb("#3b2a1e"),
        "short",
        rgb("#5f8a3c"),
        rgb("#d8c9a0"),
        rgb("#6b4a2a"),
        rgb("#3a2a1a"),
        rgb("#3a2a1e"),
    ),
    "villager_woman": Look(
        rgb("#f1c7a1"),
        rgb("#b5532b"),
        "long",
        rgb("#c0587a"),
        rgb("#f4efe4"),
        rgb("#9a3f5f"),
        rgb("#4a2a2a"),
        rgb("#5a3a2a"),
        robe=True,
    ),
    "merchant": Look(
        rgb("#e0b08a"),
        rgb("#1f1f1f"),
        "bun",
        rgb("#b5763a"),
        rgb("#f2d27a"),
        rgb("#5a4a3a"),
        rgb("#3a2a1a"),
        rgb("#2a1f1a"),
        hat="bandana",
        hat_color=rgb("#d9622b"),
    ),
}


def part(canvas: Any, sdf: Any, color: Color, light: float = 0.35, outline: bool = True) -> None:
    """A lit shape with a dark one-pixel outline."""
    nx, ny = normals(sdf)
    base = shade(
        np.broadcast_to(np.array(color), (*sdf.shape, 3)).copy(), lit(nx, ny) * light * np.clip(1 + sdf / 4, 0, 1)
    )
    if outline:
        over(canvas, OUTLINE, coverage(sdf - 1.0) * 0.9)
    over(canvas, base, coverage(sdf))


NONE = 1000.0  # Distance of a shape that is not there.


def hair_shapes(xs: Any, ys: Any, look: Look, view: str, hx: float, hy: float, r: float) -> tuple[Any, Any]:
    """The hair around a head centered at (hx, hy) with radius r, as two distance fields: what hangs behind the head
    (long locks, a bun) and what covers it (the cap, the fringe, spikes). From behind, all of it covers the head."""
    cap = sdf_circle(xs, ys, hx, hy, r + 1.4)
    if view == "up":
        front = np.maximum(cap, ys - (hy + r * 0.55))
    elif view == "down":
        front = np.maximum(cap, ys - (hy - r * 0.15 + 1.5 * np.sin((xs - hx) * 0.9)))
    else:  # Side view facing right: over the top and the back of the head, the face left open.
        front = np.maximum(cap, np.minimum(ys - (hy - r * 0.15), xs - (hx - r * 0.1)))
    behind = np.full(xs.shape, NONE)
    if look.hair_style == "long":
        if view == "side":
            behind = sdf_box(xs, ys, hx - r - 1.5, hy - 2, hx + r * 0.1, hy + r + 4, 3.0)
        else:
            behind = sdf_box(xs, ys, hx - r - 1.5, hy - 2, hx + r + 1.5, hy + r + 4, 4.0)
    elif look.hair_style == "spiky":
        for dx, height in ((-6, 4.5), (-1.5, 6.0), (3.5, 5.0), (7.5, 3.5)):
            tip_x = hx + dx + (1.5 if view == "side" else 0)
            front = np.minimum(front, sdf_segment(xs, ys, hx + dx * 0.8, hy - r + 2, tip_x - 1.5, hy - r - height, 2.2))
    elif look.hair_style == "bun":
        bun_x = hx - (r * 0.8 if view == "side" else 0)
        behind = sdf_circle(xs, ys, bun_x, hy - r * 0.95, r * 0.42)
    if view == "up":
        return np.full(xs.shape, NONE), np.minimum(front, behind)
    return behind, front


def hat_shape(canvas: Any, xs: Any, ys: Any, look: Look, view: str, hx: float, hy: float, r: float) -> None:
    if look.hat == "mage":
        brim = sdf_ellipse(xs, ys, hx, hy - r * 0.55, r * 1.55, r * 0.38)
        tip_x = hx + (r * 0.55 if view == "side" else (r * 0.25 if view == "down" else -r * 0.25))
        cone = np.maximum(np.abs(xs - hx) * 1.0 - (ys - (hy - r * 1.9)) * 0.55 - 1.0, ys - (hy - r * 0.55))
        tip = sdf_segment(xs, ys, hx, hy - r * 1.3, tip_x, hy - r * 2.05, 2.2)
        part(canvas, brim, look.hat_color, light=0.3)
        part(canvas, np.minimum(cone, tip), look.hat_color, light=0.35)
        part(
            canvas,
            sdf_box(xs, ys, hx - r * 0.75, hy - r * 0.95, hx + r * 0.75, hy - r * 0.62, 1.0),
            look.trim,
            light=0.3,
            outline=False,
        )
    elif look.hat == "helmet":
        dome = np.maximum(sdf_circle(xs, ys, hx, hy - 0.5, r + 1.8), ys - (hy + (r * 0.3 if view != "up" else r * 0.7)))
        part(canvas, dome, look.hat_color, light=0.5)
        if view == "down":
            part(
                canvas,
                sdf_box(xs, ys, hx - r * 0.7, hy - r * 0.1, hx + r * 0.7, hy + r * 0.25, 1.0),
                rgb("#5a626b"),
                light=0.2,
            )
        elif view == "side":
            part(
                canvas,
                sdf_box(xs, ys, hx + r * 0.2, hy - r * 0.1, hx + r + 1.5, hy + r * 0.25, 1.0),
                rgb("#5a626b"),
                light=0.2,
            )
        part(canvas, sdf_segment(xs, ys, hx, hy - r - 2, hx, hy - r * 0.2, 1.4), look.trim, light=0.2, outline=False)
    elif look.hat == "bandana":
        band = np.maximum(sdf_circle(xs, ys, hx, hy, r + 1.6), np.abs(ys - (hy - r * 0.5)) - 2.6)
        part(canvas, band, look.hat_color, light=0.35)
        if view in ("up", "side"):
            knot_x = hx - (r + 1 if view == "side" else 0)
            part(
                canvas,
                sdf_segment(xs, ys, knot_x, hy - r * 0.45, knot_x - 3, hy - r * 0.45 + 6, 1.8),
                look.hat_color,
                light=0.3,
            )


def draw_frame(look: Look, view: str, frame: int) -> Any:
    """One 48 x 48 frame facing up, down or right ("side")."""
    canvas = blank(TILE, TILE)
    xs, ys = grid(TILE, TILE)
    step, bob = STEP[frame], BOB[frame]
    hx, hy, r = 24.0 + (1.0 if view == "side" else 0.0), 15.0 + bob, 11.0
    over(canvas, (0.0, 0.0, 0.0), coverage(sdf_ellipse(xs, ys, 24, 45.5, 10, 2.6) * 2) * 0.25)

    # Legs (or a robe) and feet.
    if view == "side":
        feet = [(24 - 3.5 * step, False), (24 + 3.5 * step, True)]
    else:
        lift = 1.5 * step
        feet = [(20.0, lift > 0), (28.0, lift < 0)]
    for foot_x, raised in feet:
        foot_y = 44.0 - (1.5 if raised and view != "side" else 0.0)
        if view == "side":
            leg_top_x = 24.0
        else:
            leg_top_x = foot_x
        if not look.robe:
            part(canvas, sdf_segment(xs, ys, leg_top_x, 37 + bob, foot_x, foot_y - 1, 2.6), look.legs, light=0.25)
        shoe_dx = 1.5 if view == "side" else 0.0
        part(canvas, sdf_ellipse(xs, ys, foot_x + shoe_dx, foot_y, 3.3, 2.0), look.shoes, light=0.25)

    # Body.
    half_width = 6.0 if view == "side" else 8.5
    torso = sdf_box(xs, ys, 24 - half_width, 25 + bob, 24 + half_width, 38 + bob, 3.5)
    if look.robe:
        skirt = np.maximum(
            np.abs(xs - 24) - (half_width + (ys - (30 + bob)) * 0.35), np.maximum(ys - (43.5), (30 + bob) - ys)
        )
        part(canvas, skirt, look.legs, light=0.3)
    part(canvas, torso, look.outfit, light=0.35)
    part(
        canvas,
        sdf_box(xs, ys, 24 - half_width + 0.5, 33.5 + bob, 24 + half_width - 0.5, 35.5 + bob, 0.5),
        look.trim,
        light=0.2,
        outline=False,
    )

    # Arms swing against the legs.
    swing = 2.0 * step
    if view == "side":
        part(canvas, sdf_segment(xs, ys, 24, 27.5 + bob, 24 + swing, 35.5 + bob, 2.4), look.outfit, light=0.3)
        part(canvas, sdf_circle(xs, ys, 24 + swing, 36.5 + bob, 2.2), look.skin, light=0.3)
    else:
        for sx, sign in ((24 - half_width - 0.5, 1), (24 + half_width + 0.5, -1)):
            hand_y = 35.5 + bob + swing * sign
            part(canvas, sdf_segment(xs, ys, sx, 27.5 + bob, sx - sign * 1.0, hand_y, 2.4), look.outfit, light=0.3)
            part(canvas, sdf_circle(xs, ys, sx - sign * 1.0, hand_y + 1.2, 2.2), look.skin, light=0.3)

    # Head, face, hair, hat.
    hair_behind, hair_front = hair_shapes(xs, ys, look, "side" if view == "side" else view, hx, hy, r)
    if look.hat != "helmet" and np.any(hair_behind < 1):
        part(canvas, hair_behind, look.hair, light=0.3)
    head = sdf_circle(xs, ys, hx, hy, r)
    part(canvas, head, look.skin, light=0.3)
    if view == "down":
        for ex in (hx - 4.5, hx + 4.5):
            over(canvas, look.eyes, coverage(sdf_ellipse(xs, ys, ex, hy + 2.5, 1.6, 2.4)))
            over(canvas, (1.0, 1.0, 1.0), coverage(sdf_circle(xs, ys, ex - 0.6, hy + 1.6, 0.7)))
        over(
            canvas,
            shade(np.array([look.skin]), np.array([-0.35]))[0],
            coverage(sdf_segment(xs, ys, hx - 1.5, hy + 7, hx + 1.5, hy + 7, 0.5)),
        )
        over(canvas, rgb("#f08a8a"), coverage(sdf_circle(xs, ys, hx - 7, hy + 5.5, 1.5)) * 0.35)
        over(canvas, rgb("#f08a8a"), coverage(sdf_circle(xs, ys, hx + 7, hy + 5.5, 1.5)) * 0.35)
    elif view == "side":
        over(canvas, look.eyes, coverage(sdf_ellipse(xs, ys, hx + 6.0, hy + 2.5, 1.3, 2.3)))
        over(canvas, (1.0, 1.0, 1.0), coverage(sdf_circle(xs, ys, hx + 5.6, hy + 1.6, 0.6)))
    if look.hat != "helmet":
        part(canvas, hair_front, look.hair, light=0.35)
    hat_shape(canvas, xs, ys, look, "side" if view == "side" else view, hx, hy, r)
    return canvas


def character_sheet(look: Look) -> Any:
    sheet = blank(FRAMES * TILE, len(DIRECTIONS) * TILE)
    for row, direction in enumerate(DIRECTIONS):
        for frame in range(FRAMES):
            if direction == "left":
                pixels = draw_frame(look, "side", frame)[:, ::-1]  # Facing left: the right-facing frame mirrored.
            else:
                pixels = draw_frame(look, "side" if direction == "right" else direction, frame)
            sheet[row * TILE : (row + 1) * TILE, frame * TILE : (frame + 1) * TILE] = pixels
    return sheet


FACE = 144


def face(look: Look) -> Any:
    """A front portrait: head and shoulders, larger eyes, the same colors and hair as the sprite."""
    canvas = blank(FACE, FACE)
    xs, ys = grid(FACE, FACE)
    hx, hy, r = 72.0, 64.0, 38.0
    shoulders = sdf_box(xs, ys, 22, 108, 122, 170, 26)
    part(canvas, shoulders, look.outfit, light=0.35)
    part(canvas, sdf_box(xs, ys, 22, 132, 122, 138, 2), look.trim, light=0.2, outline=False)
    part(canvas, sdf_box(xs, ys, 60, 92, 84, 114, 6), look.skin, light=0.25)
    hair_behind, hair_front = hair_shapes(xs, ys, look, "down", hx, hy, r)
    if look.hair_style == "long":
        hair_behind = sdf_box(xs, ys, hx - r - 7, hy - 10, hx + r + 7, hy + r + 30, 16)  # Down to the shoulders.
    if look.hat != "helmet" and np.any(hair_behind < 1):
        part(canvas, hair_behind, look.hair, light=0.3)
    part(canvas, sdf_circle(xs, ys, hx, hy, r), look.skin, light=0.3)
    for ex in (hx - 15, hx + 15):
        over(canvas, (1.0, 1.0, 1.0), coverage(sdf_ellipse(xs, ys, ex, hy + 6, 7.5, 9.0)))
        over(canvas, look.eyes, coverage(sdf_ellipse(xs, ys, ex + 0.5, hy + 7, 5.0, 7.0)))
        over(canvas, (0.05, 0.05, 0.08), coverage(sdf_ellipse(xs, ys, ex + 0.5, hy + 8, 2.6, 3.8)))
        over(canvas, (1.0, 1.0, 1.0), coverage(sdf_circle(xs, ys, ex - 1.8, hy + 3.5, 1.8)))
        over(
            canvas,
            OUTLINE,
            coverage(np.abs(sdf_ellipse(xs, ys, ex, hy + 6, 7.5, 9.0)) - 0.6) * np.clip((hy + 6 - ys) / 4, 0, 1),
        )
        over(
            canvas,
            shade(np.array([look.hair]), np.array([-0.3]))[0],
            coverage(sdf_segment(xs, ys, ex - 6, hy - 7, ex + 6, hy - 8.5, 1.5)),
        )
    over(
        canvas,
        shade(np.array([look.skin]), np.array([-0.25]))[0],
        coverage(sdf_segment(xs, ys, hx, hy + 15, hx + 1.5, hy + 19, 1.2)),
    )
    over(canvas, rgb("#9a4a4a"), coverage(sdf_segment(xs, ys, hx - 5, hy + 26, hx + 5, hy + 26, 1.4)))
    for cx in (hx - 24, hx + 24):
        over(canvas, rgb("#f08a8a"), coverage(sdf_circle(xs, ys, cx, hy + 18, 6)) * 0.3)
    if look.hat != "helmet":
        part(canvas, hair_front, look.hair, light=0.35)
    hat_shape(canvas, xs, ys, look, "down", hx, hy, r)
    return canvas


def generate() -> dict[str, Any]:
    images: dict[str, Any] = {}
    for name, look in LOOKS.items():
        images[f"characters/{name}.png"] = character_sheet(look)
        images[f"faces/{name}.png"] = face(look)
    return images
