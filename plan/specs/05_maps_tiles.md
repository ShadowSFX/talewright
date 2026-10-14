# Spec 05 — Maps, tilesets, autotiles, cell cache, movement

Decisions: D-006 (maps are scenes), D-018 (movement), D-011 (original assets). Inventory: RM-MAP-*, RM-MPR-*, RM-TIL-*,
RM-AST-*, RM-AID-012..015. Godot: `TileMapLayer` (`scene/2d/tile_map_layer.h:332`), `TileSet` custom data
(`scene/resources/2d/tile_set.h:500-507`), animated atlas tiles (`scene/resources/2d/tile_set.h:754-766`).

## Map data and map scenes
Each map is a scene `res://jrpg/maps/map_NNNN.tscn` whose root is a `JRPGMap` node (runtime) holding a
`JRPGMapData` resource (core) as a **built-in sub-resource** of the scene:
- Undo: edits of a built-in resource are routed to its scene's undo history
  (`editor/editor_undo_redo_manager.cpp:81-94`), so map edits keep per-map undo (D-006).
- Node-free access: tools and the server data compiler load the scene as a `PackedScene` and read the resource from
  `get_state()` / `SceneState::get_node_property_value` (`scene/resources/packed_scene.h:191-193,291`) without
  instancing nodes; the server data pack stores the extracted `JRPGMapData` (spec 07).
- `JRPGMapList` (core, `res://jrpg/map_list.tres`): the source of truth for map ids, names, tree order and scene
  paths. A project scan that finds two scenes claiming one `map_id` (a copied file) gives the newer one a fresh id
  and logs a warning.

### `JRPGMapData` (core Resource)
| Property | Type | Notes |
|---|---|---|
| `map_id` | int | stable, assigned by the map list |
| `revision` | int | +1 on every save that changes layers or events (saves compare it, spec 06) |
| `display_name`, `note` | String | |
| `tileset_id` | int | JRPGTilesetData |
| `size` | Vector2i | cells (1..256 per axis default, 512 max) |
| `layers` | Array[PackedInt32Array] | 4 tile layers (ground, ground detail, objects, upper), `size.x*size.y` each; cell value = palette index (0 empty) |
| `shadow` | PackedByteArray | 4 bits per cell (quarter shadows) |
| `regions` | PackedByteArray | region id 0..255 per cell |
| `scroll_type` | enum | none / loop vertical / loop horizontal / loop both |
| `bgm`, `bgs`, `battleback1/2`, `parallax` | dictionaries | autoplay + parameters |
| `encounters` | Array[Dictionary] | `{troop_id, weight, regions:PackedInt32Array}` + `encounter_steps` (see below) |
| `disable_dash`, `next_event_id` | | |
| `events` | Array[JRPGEventData] | spec 03 |

### Random encounters
`encounter_steps` is the documented *average* number of steps between encounters (RM-MPR-008; the distribution is
not documented). Talewright's rule: after each encounter (and on entering the map) the countdown is drawn uniformly
from `[ceil(n/2), n + floor(n/2)]` with the `encounter` stream, so the mean is n and no two encounters come
back-to-back unless n is 1. Each step on a cell that allows encounters decrements it; at 0 a troop is picked by
weight among the entries whose region list is empty or contains the cell's region. Bush tiles and other step
modifiers are later data fields (epic 22).

### `JRPGMap` (runtime node)
Builds **internal** TileMapLayer children (`INTERNAL_MODE_FRONT`, like `scene/2d/tile_map.cpp:272`) from
`map_data.layers` — split into below/above-characters by the per-tile overhead flag — and one internal `JRPGEvent`
node per event, so the data stays the single source of truth and users can add any other nodes as children (D-006),
including `JRPGEventAnchor` nodes that follow an event.

## Tilesets and tile ids
- `JRPGTilesetData` (database entry) references its Godot `TileSet` by `uid://` string (`tileset_uid`) and owns an
  ordered **tile palette**. Palette index 0 is "empty"; each other entry
  is `{kind: plain|autotile|wall, name, source_id, atlas_coords, alternative[, frames]}` (flags are baked into a
  separate table, below) where `alternative` is Godot's full
  alternative id including the transform bits (`TRANSFORM_FLIP_H/V/TRANSPOSE` = bits 12–14,
  `scene/resources/2d/tile_set.h:632-634`), so flipped/rotated tiles are ordinary palette entries.
- A map cell stores the **palette index** (int32, 0 = empty). For autotile and wall kinds the cell stores the kind's
  palette entry only; the resolved shape is not stored but recomputed by the resolver on load and around every edit
  (the map data stays the single source of truth; no shape byte competes with the atlas layout).
- This Talewright cell encoding replaces Godot's `tile_map_data` byte format in the stored map (D-006 amended in
  3.5.2): it is independent of atlas coordinates, so re-laying out a tileset atlas never rewrites maps.

## Tile flags (Talewright layout)
Authoring uses Godot's own TileSet custom data layers, so the stock TileSet editor can edit them, and the JRPG
tileset tab paints them as overlays:

| Custom data layer | Type | Meaning |
|---|---|---|
| `jrpg_pass` | int | 0 open, 1 blocked, 2 transparent (no effect on passability; the tile below decides) |
| `jrpg_no_entry` | int (4 bits) | sides through which the tile cannot be **entered**: N = 1, E = 2, S = 4, W = 8 |
| `jrpg_no_exit` | int (4 bits) | sides through which the tile cannot be **left** (same bit order) |
| `jrpg_overhead` | bool | drawn above characters |
| `jrpg_ladder`, `jrpg_bush`, `jrpg_counter`, `jrpg_damage` | bool | as in the inventory rows RM-TIL-009..012 |
| `jrpg_terrain_tag` | int 0–15 | free tag (the documented range 0–7, RM-TIL-013, is a subset) |

On save/import of a tileset the editor **bakes** these into `JRPGTilesetData.flags` (`PackedInt32Array`, one word per
palette index) so core code never reads a TileSet: bits 0–1 pass mode, 2–5 no-entry N/E/S/W, 6–9 no-exit N/E/S/W,
10 overhead, 11 ladder, 12 bush, 13 counter, 14 damage, 16–19 terrain tag; other bits reserved (0). The baked table
carries a hash of the TileSet's custom data so a stale bake is detected and rebuilt in the editor.
Implementation (7.3.1, `JRPGTileFlags`, editor): word 0 is the empty cell (0); an autotile or wall entry's flags are
written to all its shapes together and read from its first shape; the hash (`flags_hash`, SHA-256) covers each palette
index with its source id, atlas coordinates, alternative and word, so a changed flag or a changed palette both make
the bake stale. `JRPGDatabaseEditing::save_all` bakes every stale tileset again before saving, and TileSets the
tileset tab edited are listed as unsaved (`mark_unsaved`) and saved with the database.
Authoring (7.3.2): `JRPGDatabaseEditing.set_tile_flags / add_tile_sheet / create_tileset`, one undoable action each,
shared by the tileset form (`JRPGEditorPropertyTiles`: the palette as a grid with the chosen flag drawn over each
tile) and the bridge tools (7.3.3).
Documented behavior this reproduces: entry permission per side for one-way edges (RM-TIL-008) and overhead tiles
that never block (RM-TIL-007); the separate no-exit mask is a Talewright addition.

## Autotile template (Talewright format)
- **Source sheet = 47-tile blob sheet**, 8 columns × 6 rows of full tiles (48 cells; cell 47 is an optional preview
  icon). Each tile is drawn whole by the artist (or by the asset generator); there is no quarter-tile composition.
- **Shape index:** the 8-neighbour mask uses bits N = 1, NE = 2, E = 4, SE = 8, S = 16, SW = 32, W = 64, NW = 128
  (clockwise from north). A corner bit is kept only when both of its adjacent sides are set (the usual blob
  reduction), leaving 47 distinct masks. Shape index = position of the reduced mask in ascending numeric order
  (0 = isolated tile, 46 = fully surrounded); sheet cell = (index mod 8, index div 8). The full index-to-mask table
  and a generated example sheet live in `talewright/docs/autotile_template.md` (written with the importer, task 7.2).
- **Animated kinds:** frames are further 8 × 6 blocks to the right (frame f at column offset 8·f); the importer uses
  Godot atlas animation columns (`scene/resources/2d/tile_set.h:754-766`).
- **Wall kinds:** a 4 × 4 sheet indexed by the 4-neighbour mask (N = 1, E = 2, S = 4, W = 8; cell (mask mod 4,
  mask div 4)), for wall faces where diagonal neighbors don't matter.
- The importer (`JRPGAutotileImporter`, editor, task 7.2) adds the sheet to the TileSet as an atlas source, creates one
  palette entry per kind (one per tile for plain sheets), wires animation frames as the tiles' animation (columns =
  frames, separation = one block) and sets Godot terrain peering bits (match corners and sides for blobs, sides for
  walls) so the stock terrain painter also works. The shape tables live in core `JRPGAutotile`; the index table and
  guide sheets are in `talewright/docs/autotile_template.md` and `talewright/assets/generated/templates/`.

## Autotile resolver (core, deterministic)
For each cell with an autotile kind, build the 8-neighbour mask from neighbors with the **same kind on the same
layer**, reduce it, and look up the shape index. Neighbors outside the map:
- on an axis that loops (`scroll_type`), the wrapped cell is compared normally;
- otherwise the tileset's `edge_mode` decides: `extend` (default; out-of-map counts as the same kind, so terrain runs
  cleanly to the map edge) or `border` (counts as different, drawing an outline at the edge).
Re-resolve the 3 × 3 neighbourhood of every edited cell. Layers never influence each other.

## Character sheets (Talewright format, task 8.7)
- **One character per image.** Rows are facing directions in the order **up, right, down, left** (clockwise from
  north); an optional 8-direction sheet adds rows up-right, down-right, down-left, up-left. Columns are walk-cycle
  frames (default 4); column 0 is the standing pose, and walking loops through all columns in order.
- Frame size = image size divided by (columns, rows); the import settings hold `frames`, `directions` (4 or 8),
  `y_offset` (pixels to draw above the cell, default 0) and `object` (no bush transparency), so no filename prefixes
  are needed.
- Import produces a Godot `SpriteFrames` resource (one animation per direction), reused by `AnimatedSprite2D`-based
  character nodes. Sheets laid out differently (several characters per image, other row orders) are imported by
  setting `grid` and `row_order` explicitly in the import dock; Talewright does not ship or require any other
  engine's layout.
- Faces: one portrait per image by default; a `grid` import setting cuts face sets.

## Map cell cache (core, `JRPGMapCellCache`)
Compiled from map data + the baked tileset flags: per cell the effective pass mode, no-entry and no-exit masks,
`overhead`, `ladder`, `bush`, `counter`, `damage`, `terrain_tag`, `region`. The effective values come from the
top-most tile whose pass mode is not "transparent" (documented stacking: overhead tiles never decide, RM-TIL-007,
RM-MAP-020). A step from A to B in direction d is allowed when A's exit on side d and B's entry on the opposite side
are both open and B is not blocked. Used by movement, pathfinding (`AStarGrid2D`-compatible or own A*), events and MMO servers
(server data compiler). Rebuilt when map tiles or the tileset change.

## Movement (core `JRPGMover`, runtime animates)
Cell steps advanced by the fixed 60 Hz simulation step (spec 03). Speed levels follow the documented labeling
(higher is faster; the default shown is "x2 slower" at level 3, so neighboring levels differ by a factor of two,
RM-EVT-032); the base value is Talewright's:

| Speed | 1 | 2 | 3 | 4 (normal) | 5 | 6 |
|---|---|---|---|---|---|---|
| Steps (1/60 s) per cell | 160 | 80 | 40 | 20 | 10 | 5 |

Autonomous-movement frequency 1–5 (higher = shorter pause, RM-EVT-033) waits 240 / 120 / 60 / 30 / 0 steps between
moves, plus a random 0–15 steps from the `map_ai` stream so crowds do not move in lockstep. Dash = one speed level
up. Diagonal moves optional; through; vehicle passability (later); loops (wrap coordinates). Blocked moves: "skip if
cannot move" or wait (spec 03). Transfers to a missing map or out-of-bounds cell fail safely: stay, warn once (final
sweep). All numbers are fields of `JRPGSystemData`.

## Map editor (editor, `JRPGMapCanvas` + `MapEditSession`)
Modes: map (pencil, rectangle, ellipse, flood fill, shadow pen, eraser, eyedropper), region, event. Auto-layer
selection by tile kind (RM-MAP) or explicit layer. One brush stroke = one undo action with a compact before/after
patch of the touched layers/cells. The canvas renders the edited map scene through a SubViewport sharing its World2D.
Spike first (task 8.1): open → paint → undo → save → reopen → run, verified through the bridge.
