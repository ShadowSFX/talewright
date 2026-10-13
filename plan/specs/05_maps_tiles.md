# Spec 05 — Maps, tilesets, autotiles, cell cache, movement

Decisions: D-006 (maps are scenes), D-018 (movement), D-011 (original assets). Inventory: RM-MAP-*, RM-MPR-*, RM-TIL-*,
RM-AST-*, RM-AID-012..015. Godot: `TileMapLayer` (`scene/2d/tile_map_layer.h:332`), `TileSet` custom data
(`scene/resources/2d/tile_set.h:500-507`), animated atlas tiles (`scene/resources/2d/tile_set.h:754-766`).

## JRPGMap (runtime node, root of each map scene `res://jrpg/maps/map_NNNN.tscn`)
| Property | Type | Notes |
|---|---|---|
| `map_id` | int | stable, from the map list |
| `display_name`, `note` | String | |
| `tileset_id` | int | JRPGTilesetData |
| `size` | Vector2i | cells (1..256 per axis default, 512 max) |
| `layers` | Array[PackedInt32Array] | 4 tile layers (ground, ground detail, objects, upper), `size.x*size.y` each; cell value = tile id (0 empty) |
| `shadow` | PackedByteArray | 4 bits per cell (quarter shadows) |
| `regions` | PackedByteArray | region id 0..255 per cell |
| `scroll_type` | enum | none / loop vertical / loop horizontal / loop both |
| `bgm`, `bgs`, `battleback1/2`, `parallax` | dictionaries | autoplay + parameters |
| `encounters` | Array[Dictionary] | `{troop_id, weight, regions:PackedInt32Array}` + `encounter_steps` |
| `disable_dash`, `next_event_id` | | |
The node builds **internal** TileMapLayer children (`INTERNAL_MODE_FRONT`, like `scene/2d/tile_map.cpp:272`) from
`layers` — split into below/above-characters by the per-tile ☆ flag — so the stored data is the single source of
truth and users can add any other nodes as children (D-006). Events are `JRPGEvent` children.

## Tile ids
A tileset is a JRPGTilesetData referencing a Godot `TileSet`. Talewright tile id (int32) encodes
`source (8 bits) | atlas x (8) | atlas y (8) | alternative/autotile shape (8)` — for autotiles the shape byte is
recomputed by the resolver, so map data stores the autotile kind and shape together and re-resolves on edits.

## Tile flags (TileSet custom data layers, names fixed)
`jrpg_passage` (int bitmask: down/left/right/up passable = bits 0–3, ☆ above = bit 4), `jrpg_ladder`, `jrpg_bush`,
`jrpg_counter`, `jrpg_damage` (bool), `jrpg_terrain_tag` (0–7). Stored on the TileSet so the stock TileSet editor
can also edit them; the JRPG tileset tab paints them as overlays (O/X/☆/arrows) like the RPG Maker UI (RM-TIL).

## Autotile template (Talewright format, original — not an RPG Maker sheet layout)
- A source image of **2×3 cells** per autotile kind, each cell 2×2 quarter tiles: cell (0,0) = isolated piece,
  (1,0) = inner-corner set, (0,1)…(1,2) = outer frame (top-left, top-right, bottom-left, bottom-right corners plus
  edges). Documented with an ASCII diagram and a generated example in `talewright/docs/autotile_template.md`.
- The importer (`JRPGAutotileImporter`, editor) bakes all 47 blob shapes (8-neighbour) into an atlas row per kind,
  sets terrain peering bits for the stock editor, and supports animated kinds (N frames → atlas animation columns,
  `set_tile_animation_columns`) and wall kinds (top + side templates).

## Autotile resolver (core, deterministic)
For each cell with an autotile kind, compute the 8-neighbour mask counting neighbours of the **same kind on the
same layer** (map edges count as "same" when the map loops on that axis, otherwise as "same" to avoid hard edges —
configurable per tileset), reduce to the 47-shape index, write the shape byte. Re-resolve the 3×3 neighbourhood of
every edited cell. Mixed layers never influence each other.

## Map cell cache (core, `JRPGMapCellCache`)
Compiled from map data + tileset flags: per cell `passage4 (4 bits)`, `above`, `ladder`, `bush`, `counter`,
`damage`, `terrain_tag`, `region`. Passability of a cell for a direction = top-most non-☆ tile with a decisive flag
(RM-TIL passage rules). Used by movement, pathfinding (`AStarGrid2D`-compatible or own A*), events and MMO servers
(server data compiler). Rebuilt when map tiles or the tileset change.

## Movement (core `JRPGMover`, runtime animates)
Cell steps with per-cell timers; speed 1–6 maps to frames per cell (D-018); diagonal moves optional; through, dash,
vehicle passability (later), loops (wrap coordinates). Blocked moves: "skip if cannot move" or wait (spec 03).
Transfers to a missing map or out-of-bounds cell fail safely: stay, warn once (final sweep).

## Map editor (editor, `JRPGMapCanvas` + `MapEditSession`)
Modes: map (pencil, rectangle, ellipse, flood fill, shadow pen, eraser, eyedropper), region, event. Auto-layer
selection by tile kind (RM-MAP) or explicit layer. One brush stroke = one undo action with a compact before/after
patch of the touched layers/cells. The canvas renders the edited map scene through a SubViewport sharing its World2D.
Spike first (task 8.1): open → paint → undo → save → reopen → run, verified through the bridge.
