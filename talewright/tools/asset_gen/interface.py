"""The icon set (32 x 32 icons, 16 per row, indexed by icon_index) and the window skin (nine-slice frame, cursor,
arrows) used by the JRPG windows."""

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
    rgb,
    sdf_box,
    sdf_circle,
    sdf_ellipse,
    sdf_segment,
    shade,
)

ICON = 32
COLUMNS = 16
OUTLINE: Color = (0.08, 0.06, 0.08)
Painter = Callable[[Any, Any, Any], None]


def part(icon: Any, sdf: Any, color: Color, light: float = 0.45, outline: float = 1.0) -> None:
    nx, ny = normals(sdf)
    base = shade(np.broadcast_to(np.array(color), (*sdf.shape, 3)).copy(), lit(nx, ny) * light)
    over(icon, OUTLINE, coverage(sdf - outline) * 0.95)
    over(icon, base, coverage(sdf))


def shine(icon: Any, xs: Any, ys: Any, x: float, y: float, r: float = 1.6) -> None:
    over(icon, (1.0, 1.0, 1.0), coverage(sdf_circle(xs, ys, x, y, r)) * 0.85)


def poly(xs: Any, ys: Any, points: list[tuple[float, float]]) -> Any:
    """Distance field of a convex polygon given counter-clockwise or clockwise (approximate, from edge half-planes)."""
    sdf = np.full(xs.shape, -1000.0)
    n = len(points)
    cx = sum(p[0] for p in points) / n
    cy = sum(p[1] for p in points) / n
    for k in range(n):
        ax, ay = points[k]
        bx, by = points[(k + 1) % n]
        ex, ey = bx - ax, by - ay
        length = float(np.hypot(ex, ey))
        nx, ny = ey / length, -ex / length
        if (cx - ax) * nx + (cy - ay) * ny > 0:
            nx, ny = -nx, -ny
        sdf = np.maximum(sdf, (xs - ax) * nx + (ys - ay) * ny)
    return sdf


# Items.
def vial(color: Color) -> Painter:
    def paint(icon: Any, xs: Any, ys: Any) -> None:
        glass = np.minimum(sdf_circle(xs, ys, 16, 20, 8.5), sdf_box(xs, ys, 13, 6, 19, 14, 1))
        part(icon, glass, (0.85, 0.9, 0.95), light=0.3)
        over(icon, color, coverage(np.maximum(sdf_circle(xs, ys, 16, 20, 7.0), 15 - ys)))
        part(icon, sdf_box(xs, ys, 12, 3, 20, 7, 1.5), rgb("#8a5a32"), light=0.3)
        shine(icon, xs, ys, 12.5, 17.5)

    return paint


def herb(icon: Any, xs: Any, ys: Any) -> None:
    for angle, length in ((-0.6, 11), (0.0, 13), (0.6, 11)):
        tip_x, tip_y = 16 + np.sin(angle) * length, 27 - np.cos(angle) * length
        leaf = sdf_segment(xs, ys, 16, 27, tip_x, tip_y, 3.2)
        part(icon, leaf, rgb("#5aa83f"), light=0.4)
    over(icon, rgb("#2e6b2a"), coverage(sdf_segment(xs, ys, 16, 29, 16, 18, 0.8)))


def elixir(icon: Any, xs: Any, ys: Any) -> None:
    vial(rgb("#f2c94c"))(icon, xs, ys)
    over(icon, (1.0, 1.0, 1.0), coverage(sdf_circle(xs, ys, 21, 9, 1.4)))
    over(icon, (1.0, 1.0, 1.0), coverage(sdf_circle(xs, ys, 24, 14, 1.0)))


def scroll(icon: Any, xs: Any, ys: Any) -> None:
    part(icon, sdf_box(xs, ys, 7, 8, 25, 24, 1), rgb("#efe3c8"), light=0.25)
    for y in (8, 24):
        part(icon, sdf_ellipse(xs, ys, 16, y, 11, 2.6), rgb("#d8c9a0"), light=0.3)
    for line_y in (13.0, 16.5, 20.0):
        over(icon, rgb("#8a6a4a"), coverage(sdf_segment(xs, ys, 10, line_y, 22, line_y, 0.5)))
    part(icon, sdf_circle(xs, ys, 16, 26, 2.4), rgb("#c0392b"), light=0.3)


def key(icon: Any, xs: Any, ys: Any) -> None:
    ring = np.abs(sdf_circle(xs, ys, 10, 12, 5)) - 2.0
    shaft = sdf_segment(xs, ys, 14, 16, 25, 27, 1.8)
    teeth = np.minimum(sdf_box(xs, ys, 20, 22, 23, 26, 0.5), sdf_box(xs, ys, 23, 25, 26, 28, 0.5))
    part(icon, np.minimum(np.minimum(ring, shaft), teeth), rgb("#e2b43a"), light=0.5)


def gem(icon: Any, xs: Any, ys: Any) -> None:
    part(icon, poly(xs, ys, [(16, 4), (26, 13), (16, 28), (6, 13)]), rgb("#3fa0d8"), light=0.55)
    over(icon, (1.0, 1.0, 1.0), coverage(poly(xs, ys, [(16, 6), (21, 12), (16, 14), (11, 12)])) * 0.5)


def coins(icon: Any, xs: Any, ys: Any) -> None:
    part(icon, sdf_box(xs, ys, 8, 12, 24, 27, 6), rgb("#b5763a"), light=0.4)
    part(icon, sdf_box(xs, ys, 12, 7, 20, 13, 2), rgb("#8a5a32"), light=0.3)
    part(icon, sdf_circle(xs, ys, 16, 20, 4.5), rgb("#f2c94c"), light=0.5)


def bread(icon: Any, xs: Any, ys: Any) -> None:
    part(icon, sdf_ellipse(xs, ys, 16, 18, 12, 8), rgb("#c98a3f"), light=0.5)
    for x in (11, 16, 21):
        over(icon, rgb("#8a5a2a"), coverage(sdf_segment(xs, ys, x - 1.5, 14, x + 1.5, 20, 0.6)))


def tent(icon: Any, xs: Any, ys: Any) -> None:
    part(icon, poly(xs, ys, [(16, 5), (28, 26), (4, 26)]), rgb("#3f7d45"), light=0.45)
    over(icon, rgb("#1f3a22"), coverage(poly(xs, ys, [(16, 14), (20, 26), (12, 26)])))


def feather(icon: Any, xs: Any, ys: Any) -> None:
    vane = np.maximum(sdf_ellipse(xs, ys, 17, 15, 6.5, 12.5), -(sdf_circle(xs, ys, 26, 26, 6)))
    part(icon, vane, rgb("#f2f6fb"), light=0.4)
    over(icon, rgb("#9aa8b8"), coverage(sdf_segment(xs, ys, 8, 28, 22, 4, 0.7)))
    for k in range(4):
        over(
            icon,
            rgb("#b8c6d6"),
            coverage(sdf_segment(xs, ys, 13 + k * 2.5, 21 - k * 4, 18 + k * 2.5, 22 - k * 4, 0.45)),
        )


def bomb(icon: Any, xs: Any, ys: Any) -> None:
    part(icon, sdf_circle(xs, ys, 15, 19, 9), rgb("#3a3a42"), light=0.5)
    part(icon, sdf_segment(xs, ys, 20, 11, 24, 6, 1.2), rgb("#c99a62"), light=0.2)
    over(icon, rgb("#f39c12"), coverage(sdf_circle(xs, ys, 25, 5, 2.4)))
    shine(icon, xs, ys, 11, 15, 2.0)


def map_icon(icon: Any, xs: Any, ys: Any) -> None:
    part(icon, sdf_box(xs, ys, 5, 7, 27, 25, 1), rgb("#efe3c8"), light=0.25)
    over(icon, rgb("#6aa84f"), coverage(sdf_ellipse(xs, ys, 13, 15, 5, 4)))
    over(icon, rgb("#3f6db4"), coverage(sdf_ellipse(xs, ys, 21, 19, 4, 3)))
    over(
        icon,
        rgb("#c0392b"),
        coverage(sdf_segment(xs, ys, 17, 11, 21, 15, 0.9)) + coverage(sdf_segment(xs, ys, 21, 11, 17, 15, 0.9)),
    )


def book(icon: Any, xs: Any, ys: Any) -> None:
    part(icon, sdf_box(xs, ys, 7, 5, 25, 27, 1.5), rgb("#2f5f8e"), light=0.4)
    over(icon, rgb("#efe3c8"), coverage(sdf_box(xs, ys, 23, 7, 25, 25)))
    part(icon, sdf_box(xs, ys, 11, 10, 21, 14, 0.5), rgb("#e2c044"), light=0.2, outline=0.5)


# Weapons and armor.
def blade(icon: Any, xs: Any, ys: Any, length: float, width: float, metal: Color = (0.82, 0.85, 0.9)) -> None:
    part(icon, sdf_segment(xs, ys, 8, 24, 8 + length * 0.7, 24 - length * 0.7, width), metal, light=0.55)


def sword(icon: Any, xs: Any, ys: Any) -> None:
    blade(icon, xs, ys, 22, 2.0)
    part(icon, sdf_segment(xs, ys, 5, 20, 12, 27, 1.4), rgb("#e2c044"), light=0.4)
    part(icon, sdf_segment(xs, ys, 4, 28, 8, 24, 1.6), rgb("#6b4426"), light=0.3)


def dagger(icon: Any, xs: Any, ys: Any) -> None:
    blade(icon, xs, ys, 15, 1.8)
    part(icon, sdf_segment(xs, ys, 6, 21, 11, 26, 1.2), rgb("#8a8f96"), light=0.4)
    part(icon, sdf_segment(xs, ys, 4, 28, 8, 24, 1.5), rgb("#3a2a1a"), light=0.3)


def axe(icon: Any, xs: Any, ys: Any) -> None:
    part(icon, sdf_segment(xs, ys, 8, 27, 22, 6, 1.6), rgb("#8a5a32"), light=0.3)
    head = np.maximum(sdf_circle(xs, ys, 23, 12, 8), -(sdf_circle(xs, ys, 30, 12, 5)))
    part(icon, np.maximum(head, xs - 28), (0.8, 0.83, 0.88), light=0.55)


def spear(icon: Any, xs: Any, ys: Any) -> None:
    part(icon, sdf_segment(xs, ys, 5, 28, 21, 12, 1.4), rgb("#8a5a32"), light=0.3)
    part(icon, poly(xs, ys, [(28, 4), (24, 15), (18, 10)]), (0.82, 0.85, 0.9), light=0.55)


def bow(icon: Any, xs: Any, ys: Any) -> None:
    # The quarter of a ring above and right of its center, from (4, 6) round to (26, 28); the string joins the ends.
    arc = np.maximum(np.abs(sdf_circle(xs, ys, 4, 28, 22)) - 1.8, np.maximum(4 - xs, ys - 28))
    part(icon, arc, rgb("#8a5a32"), light=0.35)
    over(icon, (0.9, 0.9, 0.85), coverage(sdf_segment(xs, ys, 4.5, 6.5, 25.5, 27.5, 0.5)))


def staff(icon: Any, xs: Any, ys: Any) -> None:
    part(icon, sdf_segment(xs, ys, 6, 28, 21, 11, 1.5), rgb("#6b4426"), light=0.3)
    part(icon, sdf_circle(xs, ys, 23, 9, 5), rgb("#7a4ad8"), light=0.6)
    shine(icon, xs, ys, 21.5, 7.5, 1.4)


def mace(icon: Any, xs: Any, ys: Any) -> None:
    part(icon, sdf_segment(xs, ys, 6, 27, 18, 15, 1.5), rgb("#6b4426"), light=0.3)
    part(icon, sdf_circle(xs, ys, 21, 11, 6.5), (0.62, 0.65, 0.7), light=0.55)
    for dx, dy in ((0, -8), (8, 0), (-6, -6), (6, -6), (6, 6)):
        part(icon, sdf_circle(xs, ys, 21 + dx * 0.9, 11 + dy * 0.9, 1.6), (0.75, 0.78, 0.82), light=0.4, outline=0.6)


def claw(icon: Any, xs: Any, ys: Any) -> None:
    for k in range(3):
        part(icon, sdf_segment(xs, ys, 10 + k * 5, 24, 14 + k * 5, 6, 1.4), (0.82, 0.85, 0.9), light=0.5)
    part(icon, sdf_box(xs, ys, 7, 22, 25, 28, 2), rgb("#6b4426"), light=0.3)


def shield(icon: Any, xs: Any, ys: Any) -> None:
    body = np.maximum(sdf_box(xs, ys, 6, 4, 26, 30, 3), sdf_circle(xs, ys, 16, 4, 26))
    part(icon, body, rgb("#3f6fb5"), light=0.5)
    over(
        icon,
        rgb("#e2c044"),
        coverage(np.abs(np.maximum(sdf_box(xs, ys, 9, 7, 23, 27, 2), sdf_circle(xs, ys, 16, 6, 22))) - 0.8),
    )


def helmet(icon: Any, xs: Any, ys: Any) -> None:
    part(icon, np.maximum(sdf_circle(xs, ys, 16, 18, 11), ys - 23), (0.68, 0.71, 0.76), light=0.55)
    part(icon, sdf_box(xs, ys, 9, 17, 23, 21, 1), rgb("#4a4f55"), light=0.2)
    part(icon, sdf_segment(xs, ys, 16, 5, 16, 11, 1.4), rgb("#c0392b"), light=0.3)


def armor(icon: Any, xs: Any, ys: Any) -> None:
    body = np.minimum(
        sdf_box(xs, ys, 9, 8, 23, 28, 3), np.minimum(sdf_circle(xs, ys, 8, 10, 4.5), sdf_circle(xs, ys, 24, 10, 4.5))
    )
    part(icon, body, (0.68, 0.71, 0.76), light=0.5)
    over(icon, rgb("#4a4f55"), coverage(sdf_segment(xs, ys, 16, 10, 16, 26, 0.6)))


def robe(icon: Any, xs: Any, ys: Any) -> None:
    part(icon, poly(xs, ys, [(11, 5), (21, 5), (27, 28), (5, 28)]), rgb("#6a3fa0"), light=0.45)
    over(icon, rgb("#e2c044"), coverage(sdf_segment(xs, ys, 16, 6, 16, 27, 0.8)))


def boots(icon: Any, xs: Any, ys: Any) -> None:
    for dx in (-5, 5):
        boot = np.minimum(
            sdf_box(xs, ys, 12 + dx - 3, 7, 12 + dx + 3, 24, 1.5), sdf_box(xs, ys, 12 + dx - 3, 20, 12 + dx + 7, 26, 2)
        )
        part(icon, boot, rgb("#6b4426"), light=0.4)


def gloves(icon: Any, xs: Any, ys: Any) -> None:
    palm = sdf_box(xs, ys, 9, 13, 21, 27, 3)
    fingers = np.min(np.stack([sdf_segment(xs, ys, 11 + k * 3, 13, 11 + k * 3, 6, 1.6) for k in range(4)]), axis=0)
    thumb = sdf_segment(xs, ys, 21, 19, 25, 14, 1.8)
    part(icon, np.minimum(np.minimum(palm, fingers), thumb), rgb("#8a5a32"), light=0.4)


def ring(icon: Any, xs: Any, ys: Any) -> None:
    part(icon, np.abs(sdf_circle(xs, ys, 16, 19, 7.5)) - 1.8, rgb("#e2b43a"), light=0.55)
    part(icon, sdf_circle(xs, ys, 16, 10, 3.5), rgb("#c0392b"), light=0.55)


def amulet(icon: Any, xs: Any, ys: Any) -> None:
    over(icon, rgb("#e2b43a"), coverage(np.abs(sdf_ellipse(xs, ys, 16, 12, 9, 9)) - 0.7))
    part(icon, poly(xs, ys, [(16, 16), (22, 22), (16, 29), (10, 22)]), rgb("#3fa06a"), light=0.55)


# Elements.
def flame(icon: Any, xs: Any, ys: Any) -> None:
    outer = np.minimum(sdf_circle(xs, ys, 16, 20, 8.5), np.maximum(np.abs(xs - 16) - (ys - 3) * 0.48, ys - 20))
    part(icon, outer, rgb("#e2531f"), light=0.3)
    inner = np.minimum(sdf_circle(xs, ys, 16, 22, 4.5), np.maximum(np.abs(xs - 16) - (ys - 12) * 0.4, ys - 22))
    over(icon, rgb("#ffd25a"), coverage(inner))


def snowflake(icon: Any, xs: Any, ys: Any) -> None:
    arms = np.min(
        np.stack([
            sdf_segment(xs, ys, 16 - 11 * np.cos(a), 16 - 11 * np.sin(a), 16 + 11 * np.cos(a), 16 + 11 * np.sin(a), 1.3)
            for a in (0, np.pi / 3, 2 * np.pi / 3)
        ]),
        axis=0,
    )
    part(icon, arms, rgb("#bfe6ff"), light=0.3)
    part(icon, sdf_circle(xs, ys, 16, 16, 3), rgb("#e8f6ff"), light=0.3, outline=0.6)


def bolt(icon: Any, xs: Any, ys: Any) -> None:
    upper = poly(xs, ys, [(19, 3), (9, 18), (16, 18)])
    lower = poly(xs, ys, [(16, 14), (23, 14), (13, 29)])
    part(icon, np.minimum(upper, lower), rgb("#f2d23c"), light=0.4)


def droplet(icon: Any, xs: Any, ys: Any) -> None:
    drop = np.minimum(sdf_circle(xs, ys, 16, 20, 8), np.maximum(np.abs(xs - 16) - (ys - 4) * 0.5, ys - 20))
    part(icon, drop, rgb("#3f8fd8"), light=0.5)
    shine(icon, xs, ys, 12.5, 19, 1.8)


def rock(icon: Any, xs: Any, ys: Any) -> None:
    part(icon, poly(xs, ys, [(10, 8), (22, 6), (28, 18), (22, 27), (8, 26), (4, 16)]), rgb("#9a7a4a"), light=0.5)
    over(icon, rgb("#5a4a2a"), coverage(sdf_segment(xs, ys, 12, 14, 18, 20, 0.6)))


def swirl(icon: Any, xs: Any, ys: Any) -> None:
    for k, r in enumerate((10, 6.5, 3.5)):
        arc = np.maximum(np.abs(sdf_circle(xs, ys, 16 + k, 16, r)) - 1.4, -(ys - 16 + k * 2))
        part(icon, arc, rgb("#8fd8a8"), light=0.3, outline=0.6)


def sun(icon: Any, xs: Any, ys: Any) -> None:
    for k in range(8):
        a = k * np.pi / 4
        part(
            icon,
            sdf_segment(xs, ys, 16 + 8 * np.cos(a), 16 + 8 * np.sin(a), 16 + 13 * np.cos(a), 16 + 13 * np.sin(a), 1.4),
            rgb("#f2d23c"),
            light=0.2,
            outline=0.6,
        )
    part(icon, sdf_circle(xs, ys, 16, 16, 7), rgb("#fff1a8"), light=0.4)


def moon(icon: Any, xs: Any, ys: Any) -> None:
    crescent = np.maximum(sdf_circle(xs, ys, 15, 16, 11), -sdf_circle(xs, ys, 21, 12, 9))
    part(icon, crescent, rgb("#5a3f8a"), light=0.45)


def fist(icon: Any, xs: Any, ys: Any) -> None:
    hand = np.minimum(sdf_box(xs, ys, 8, 10, 24, 25, 4), sdf_box(xs, ys, 12, 23, 22, 29, 1.5))
    part(icon, hand, rgb("#e0a878"), light=0.45)
    for x in (12, 16, 20):
        over(icon, rgb("#8a5a3a"), coverage(sdf_segment(xs, ys, x, 11, x, 17, 0.5)))


# States and buffs.
def skull(icon: Any, xs: Any, ys: Any, color: Color = (0.92, 0.9, 0.85)) -> None:
    head = np.minimum(sdf_circle(xs, ys, 16, 14, 10), sdf_box(xs, ys, 11, 18, 21, 27, 2))
    part(icon, head, color, light=0.35)
    for ex in (12, 20):
        over(icon, OUTLINE, coverage(sdf_circle(xs, ys, ex, 15, 2.6)))
    for x in (13.5, 16, 18.5):
        over(icon, OUTLINE, coverage(sdf_segment(xs, ys, x, 23, x, 26, 0.5)))


def poison(icon: Any, xs: Any, ys: Any) -> None:
    drop = np.minimum(sdf_circle(xs, ys, 16, 20, 9), np.maximum(np.abs(xs - 16) - (ys - 3) * 0.55, ys - 20))
    part(icon, drop, rgb("#7a3fa0"), light=0.5)
    for ex in (13, 19):
        over(icon, (0.95, 0.95, 0.9), coverage(sdf_circle(xs, ys, ex, 20, 1.8)))


def sleep(icon: Any, xs: Any, ys: Any) -> None:
    for k, (x, y, s) in enumerate(((7, 22, 7), (16, 13, 5.5), (23, 6, 4))):
        z = np.minimum(
            np.minimum(sdf_segment(xs, ys, x, y, x + s, y, 1.0), sdf_segment(xs, ys, x + s, y, x, y + s, 1.0)),
            sdf_segment(xs, ys, x, y + s, x + s, y + s, 1.0),
        )
        part(icon, z, rgb("#7fb0f0"), light=0.2, outline=0.7)


def stun(icon: Any, xs: Any, ys: Any) -> None:
    for k in range(3):
        a = 2 * np.pi * k / 3
        cx, cy = 16 + 9 * np.cos(a), 16 + 6 * np.sin(a)
        star = np.min(
            np.stack([
                sdf_segment(xs, ys, cx, cy, cx + 4 * np.cos(b), cy + 4 * np.sin(b), 1.0)
                for b in np.arange(5) * 2 * np.pi / 5
            ]),
            axis=0,
        )
        part(icon, star, rgb("#f2d23c"), light=0.3, outline=0.6)


def silence(icon: Any, xs: Any, ys: Any) -> None:
    part(icon, sdf_ellipse(xs, ys, 16, 17, 11, 9), (0.95, 0.95, 0.95), light=0.3)
    over(icon, OUTLINE, coverage(sdf_segment(xs, ys, 11, 17, 21, 17, 1.0)))
    over(
        icon,
        rgb("#c0392b"),
        coverage(np.abs(sdf_circle(xs, ys, 16, 16, 13)) - 1.2) + coverage(sdf_segment(xs, ys, 7, 7, 25, 25, 1.2)),
    )


def question_hook(xs: Any, ys: Any) -> Any:
    """The curve of a question mark: a ring with its lower-left quarter removed, and the stem below it."""
    ring = np.abs(sdf_circle(xs, ys, 16, 12, 6)) - 1.8
    lower_left = np.maximum(12 - ys, xs - 16)  # Negative inside the quarter below and left of the center.
    return np.minimum(np.maximum(ring, -lower_left), sdf_segment(xs, ys, 16, 18, 16, 20.5, 1.8))


def confusion(icon: Any, xs: Any, ys: Any) -> None:
    part(icon, question_hook(xs, ys), rgb("#e07a3a"), light=0.3)
    part(icon, sdf_circle(xs, ys, 16, 26, 2.0), rgb("#e07a3a"), light=0.3)


def knocked_out(icon: Any, xs: Any, ys: Any) -> None:
    skull(icon, xs, ys)


def guard(icon: Any, xs: Any, ys: Any) -> None:
    body = np.maximum(sdf_box(xs, ys, 8, 6, 24, 28, 3), sdf_circle(xs, ys, 16, 6, 23))
    part(icon, body, rgb("#8a8f96"), light=0.5)
    over(icon, rgb("#3f6fb5"), coverage(sdf_segment(xs, ys, 16, 9, 16, 24, 1.6)))


def stat_arrow(up: bool, color: Color, symbol: Callable[[Any, Any], Any]) -> Painter:
    """A parameter's symbol on its color, with a green arrow up (buff) or a red arrow down (debuff)."""

    def paint(icon: Any, xs: Any, ys: Any) -> None:
        part(icon, sdf_box(xs, ys, 3, 3, 29, 29, 5), color, light=0.35)
        over(icon, (0.98, 0.98, 0.98), coverage(symbol(xs, ys)))
        if up:
            arrow = np.minimum(poly(xs, ys, [(24, 5), (30, 14), (18, 14)]), sdf_box(xs, ys, 22, 13, 26, 22, 0.5))
        else:
            arrow = np.minimum(poly(xs, ys, [(24, 27), (30, 18), (18, 18)]), sdf_box(xs, ys, 22, 10, 26, 19, 0.5))
        part(icon, arrow, (0.3, 0.85, 0.35) if up else (0.9, 0.3, 0.3), light=0.3, outline=0.7)

    return paint


def letter_a(xs: Any, ys: Any) -> Any:  # A sword: attack.
    return np.minimum(sdf_segment(xs, ys, 8, 24, 17, 9, 1.3), sdf_segment(xs, ys, 7, 19, 13, 23, 1.1))


def letter_d(xs: Any, ys: Any) -> Any:  # A shield: defense.
    return np.maximum(sdf_box(xs, ys, 7, 9, 17, 24, 2), sdf_circle(xs, ys, 12, 9, 15))


def letter_m(xs: Any, ys: Any) -> Any:  # A spark: magic.
    return np.minimum(sdf_segment(xs, ys, 12, 8, 12, 24, 1.2), sdf_segment(xs, ys, 5, 16, 19, 16, 1.2))


def letter_s(xs: Any, ys: Any) -> Any:  # A wing: agility.
    return np.maximum(sdf_ellipse(xs, ys, 12, 16, 7, 6), -sdf_ellipse(xs, ys, 14, 19, 6, 5))


def heart(icon: Any, xs: Any, ys: Any) -> None:
    shape = np.minimum(
        np.minimum(sdf_circle(xs, ys, 11.5, 12, 6), sdf_circle(xs, ys, 20.5, 12, 6)),
        poly(xs, ys, [(5.8, 14), (26.2, 14), (16, 27)]),
    )
    part(icon, shape, rgb("#d94a5a"), light=0.5)
    shine(icon, xs, ys, 10, 10, 1.6)


def regen(icon: Any, xs: Any, ys: Any) -> None:
    heart(icon, xs, ys)
    over(
        icon,
        (0.95, 1.0, 0.95),
        coverage(np.minimum(sdf_segment(xs, ys, 16, 12, 16, 22, 1.4), sdf_segment(xs, ys, 11, 17, 21, 17, 1.4))),
    )


def star(icon: Any, xs: Any, ys: Any) -> None:
    points = []
    for k in range(10):
        a = -np.pi / 2 + k * np.pi / 5
        r = 13 if k % 2 == 0 else 5.5
        points.append((16 + r * np.cos(a), 17 + r * np.sin(a)))
    sdf = np.full(xs.shape, 1000.0)
    for k in range(0, 10, 2):  # A star as five triangles around a pentagon.
        sdf = np.minimum(sdf, poly(xs, ys, [points[k], points[(k + 1) % 10], (16, 17), points[(k - 1) % 10]]))
    part(icon, sdf, rgb("#f2c94c"), light=0.4)


def coin(icon: Any, xs: Any, ys: Any) -> None:
    part(icon, sdf_circle(xs, ys, 16, 16, 11), rgb("#e2b43a"), light=0.55)
    over(icon, rgb("#a8781a"), coverage(np.abs(sdf_circle(xs, ys, 16, 16, 7.5)) - 0.8))


def sparkle(icon: Any, xs: Any, ys: Any) -> None:
    for cx, cy, s in ((16, 15, 11), (24, 25, 5)):
        diamond = poly(xs, ys, [(cx, cy - s), (cx + s * 0.3, cy), (cx, cy + s), (cx - s * 0.3, cy)])
        cross = poly(xs, ys, [(cx - s, cy), (cx, cy - s * 0.3), (cx + s, cy), (cx, cy + s * 0.3)])
        part(icon, np.minimum(diamond, cross), rgb("#bfe6ff"), light=0.3, outline=0.7)


def flag(icon: Any, xs: Any, ys: Any) -> None:
    part(icon, sdf_segment(xs, ys, 8, 4, 8, 29, 1.2), rgb("#6b4426"), light=0.3)
    part(icon, poly(xs, ys, [(9, 5), (26, 10), (9, 17)]), rgb("#c0392b"), light=0.4)


def question(icon: Any, xs: Any, ys: Any) -> None:
    part(icon, sdf_box(xs, ys, 4, 4, 28, 28, 6), rgb("#5a6a7a"), light=0.35)
    over(icon, (0.98, 0.98, 0.98), coverage(question_hook(xs, ys)))
    over(icon, (0.98, 0.98, 0.98), coverage(sdf_circle(xs, ys, 16, 24, 1.8)))


def nothing(icon: Any, xs: Any, ys: Any) -> None:
    pass


# The icon index table (documented in talewright/docs/assets.md); index = position.
ICONS: list[tuple[str, Painter]] = [
    ("none", nothing),
    ("potion", vial(rgb("#d94a4a"))),
    ("ether", vial(rgb("#3f7fd8"))),
    ("herb", herb),
    ("elixir", elixir),
    ("antidote", vial(rgb("#5aa83f"))),
    ("scroll", scroll),
    ("key", key),
    ("gem", gem),
    ("money_bag", coins),
    ("bread", bread),
    ("tent", tent),
    ("feather", feather),
    ("bomb", bomb),
    ("map", map_icon),
    ("book", book),
    ("sword", sword),
    ("dagger", dagger),
    ("axe", axe),
    ("spear", spear),
    ("bow", bow),
    ("staff", staff),
    ("mace", mace),
    ("claw", claw),
    ("shield", shield),
    ("helmet", helmet),
    ("armor", armor),
    ("robe", robe),
    ("boots", boots),
    ("gloves", gloves),
    ("ring", ring),
    ("amulet", amulet),
    ("physical", fist),
    ("fire", flame),
    ("ice", snowflake),
    ("thunder", bolt),
    ("water", droplet),
    ("earth", rock),
    ("wind", swirl),
    ("light", sun),
    ("dark", moon),
    ("poison", poison),
    ("sleep", sleep),
    ("stun", stun),
    ("silence", silence),
    ("confusion", confusion),
    ("knocked_out", knocked_out),
    ("guard", guard),
    ("attack_up", stat_arrow(True, rgb("#a03a2a"), letter_a)),
    ("defense_up", stat_arrow(True, rgb("#2f5f8e"), letter_d)),
    ("magic_up", stat_arrow(True, rgb("#6a3fa0"), letter_m)),
    ("agility_up", stat_arrow(True, rgb("#3f7d45"), letter_s)),
    ("attack_down", stat_arrow(False, rgb("#a03a2a"), letter_a)),
    ("defense_down", stat_arrow(False, rgb("#2f5f8e"), letter_d)),
    ("magic_down", stat_arrow(False, rgb("#6a3fa0"), letter_m)),
    ("agility_down", stat_arrow(False, rgb("#3f7d45"), letter_s)),
    ("regen", regen),
    ("star", star),
    ("coin", coin),
    ("heart", heart),
    ("sparkle", sparkle),
    ("flag", flag),
    ("question", question),
    ("none_63", nothing),
]


def icon_sheet() -> Any:
    rows = (len(ICONS) + COLUMNS - 1) // COLUMNS
    sheet = blank(COLUMNS * ICON, rows * ICON)
    xs, ys = grid(ICON, ICON)
    for index, (_, painter) in enumerate(ICONS):
        icon = blank(ICON, ICON)
        painter(icon, xs, ys)
        x, y = (index % COLUMNS) * ICON, (index // COLUMNS) * ICON
        sheet[y : y + ICON, x : x + ICON] = icon
    return sheet


# Window skin.
WINDOW = 96
WINDOW_MARGIN = 16


def window_frame() -> Any:
    """A nine-slice window: 16-pixel corners and edges, a translucent body that stretches."""
    frame = blank(WINDOW, WINDOW)
    xs, ys = grid(WINDOW, WINDOW)
    body = sdf_box(xs, ys, 2, 2, WINDOW - 2, WINDOW - 2, 8)
    t = (ys / WINDOW)[..., None]
    fill = np.array(rgb("#22305a")) * (1 - t) + np.array(rgb("#121a33")) * t
    over(frame, fill, coverage(body) * 0.88)
    over(frame, rgb("#0a0f1f"), coverage(np.abs(body) - 1.0))
    over(frame, rgb("#d9c38a"), coverage(np.abs(sdf_box(xs, ys, 4, 4, WINDOW - 4, WINDOW - 4, 6)) - 0.9))
    over(
        frame,
        rgb("#fff3c4"),
        coverage(np.abs(sdf_box(xs, ys, 4, 4, WINDOW - 4, WINDOW - 4, 6)) - 0.4) * np.clip((40 - ys) / 40, 0, 1) * 0.6,
    )
    for cx, cy in ((8, 8), (WINDOW - 8, 8), (8, WINDOW - 8), (WINDOW - 8, WINDOW - 8)):
        over(
            frame,
            rgb("#e8d49a"),
            coverage(poly(xs, ys, [(cx, cy - 3.5), (cx + 3.5, cy), (cx, cy + 3.5), (cx - 3.5, cy)])),
        )
    return frame


def window_cursor() -> Any:
    cursor = blank(48, 48)
    xs, ys = grid(48, 48)
    box = sdf_box(xs, ys, 1.5, 1.5, 46.5, 46.5, 6)
    over(cursor, rgb("#fff1b0"), coverage(box) * 0.22)
    over(cursor, rgb("#fff1b0"), coverage(np.abs(box) - 0.9) * 0.9)
    return cursor


def window_arrows() -> Any:
    """Four 16 x 16 marks: scroll up, scroll down, more text (pause), and a right arrow for submenus."""
    sheet = blank(64, 16)
    xs, ys = grid(16, 16)
    marks = [
        poly(xs, ys, [(8, 3), (14, 11), (2, 11)]),
        poly(xs, ys, [(8, 13), (14, 5), (2, 5)]),
        poly(xs, ys, [(8, 12), (13, 6), (3, 6)]),
        poly(xs, ys, [(13, 8), (5, 2), (5, 14)]),
    ]
    for k, mark in enumerate(marks):
        cell = blank(16, 16)
        over(cell, OUTLINE, coverage(mark - 1.0))
        over(cell, rgb("#f2e2a8"), coverage(mark))
        sheet[:, k * 16 : (k + 1) * 16] = cell
    return sheet


def generate() -> dict[str, Any]:
    return {
        "system/icons.png": icon_sheet(),
        "system/window.png": window_frame(),
        "system/window_cursor.png": window_cursor(),
        "system/window_arrows.png": window_arrows(),
    }
