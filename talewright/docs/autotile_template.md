# Autotile templates

Talewright's autotiles are whole tiles drawn for every shape a terrain can take, laid out on a fixed sheet (spec 05,
`plan/specs/05_maps_tiles.md`). The map stores only which kind each cell holds; the shape is worked out from the
neighbors when the map is drawn, so painting a terrain always gives the right edges and corners.

Draw over the guide sheets in `talewright/assets/generated/templates/` (`blob_template.png`, `wall_template.png`): each
48 × 48 cell shows the tile (middle square) and which neighbors are the same kind (filled squares). The generated
terrains in `talewright/assets/generated/autotiles/` are finished examples. Any tile size works; the sheet must be
exactly 8 × 6 (blob) or 4 × 4 (wall) tiles of the TileSet's tile size per animation frame.

## Blob autotiles (terrain, water, paths)
- A sheet of 8 × 6 tiles: shape *i* at column *i* mod 8, row *i* div 8. The 48th cell (column 7, row 5) is free for a
  preview of the terrain; it is not imported as a tile.
- Animated kinds put each frame as another 8 × 6 block to the right: a 3-frame sheet is 24 × 6 tiles.
- The shape comes from the 8 neighbors: N 1, NE 2, E 4, SE 8, S 16, SW 32, W 64, NW 128 (clockwise from north), each
  set when that neighbor is the same kind. A corner counts only when both of its sides are set, which leaves 47
  shapes; shape *i* is the *i*-th of them in ascending order of the mask.
- Neighbors outside the map count as the same kind (the terrain runs to the edge), or as different with the
  tileset's border edge mode; on maps that loop, the cell on the other side is used.

<!-- blob index table: kept equal to JRPGAutotile by modules/jrpg/tests/test_jrpg_autotile.h -->
| Index | Mask | Same-kind neighbors | Sheet cell (column, row) |
|---|---|---|---|
| 0 | 0 | none | (0, 0) |
| 1 | 1 | N | (1, 0) |
| 2 | 4 | E | (2, 0) |
| 3 | 5 | N E | (3, 0) |
| 4 | 7 | N NE E | (4, 0) |
| 5 | 16 | S | (5, 0) |
| 6 | 17 | N S | (6, 0) |
| 7 | 20 | E S | (7, 0) |
| 8 | 21 | N E S | (0, 1) |
| 9 | 23 | N NE E S | (1, 1) |
| 10 | 28 | E SE S | (2, 1) |
| 11 | 29 | N E SE S | (3, 1) |
| 12 | 31 | N NE E SE S | (4, 1) |
| 13 | 64 | W | (5, 1) |
| 14 | 65 | N W | (6, 1) |
| 15 | 68 | E W | (7, 1) |
| 16 | 69 | N E W | (0, 2) |
| 17 | 71 | N NE E W | (1, 2) |
| 18 | 80 | S W | (2, 2) |
| 19 | 81 | N S W | (3, 2) |
| 20 | 84 | E S W | (4, 2) |
| 21 | 85 | N E S W | (5, 2) |
| 22 | 87 | N NE E S W | (6, 2) |
| 23 | 92 | E SE S W | (7, 2) |
| 24 | 93 | N E SE S W | (0, 3) |
| 25 | 95 | N NE E SE S W | (1, 3) |
| 26 | 112 | S SW W | (2, 3) |
| 27 | 113 | N S SW W | (3, 3) |
| 28 | 116 | E S SW W | (4, 3) |
| 29 | 117 | N E S SW W | (5, 3) |
| 30 | 119 | N NE E S SW W | (6, 3) |
| 31 | 124 | E SE S SW W | (7, 3) |
| 32 | 125 | N E SE S SW W | (0, 4) |
| 33 | 127 | N NE E SE S SW W | (1, 4) |
| 34 | 193 | N W NW | (2, 4) |
| 35 | 197 | N E W NW | (3, 4) |
| 36 | 199 | N NE E W NW | (4, 4) |
| 37 | 209 | N S W NW | (5, 4) |
| 38 | 213 | N E S W NW | (6, 4) |
| 39 | 215 | N NE E S W NW | (7, 4) |
| 40 | 221 | N E SE S W NW | (0, 5) |
| 41 | 223 | N NE E SE S W NW | (1, 5) |
| 42 | 241 | N S SW W NW | (2, 5) |
| 43 | 245 | N E S SW W NW | (3, 5) |
| 44 | 247 | N NE E S SW W NW | (4, 5) |
| 45 | 253 | N E SE S SW W NW | (5, 5) |
| 46 | 255 | N NE E SE S SW W NW | (6, 5) |
<!-- end of blob index table -->

## Wall autotiles (wall faces, roofs)
- A sheet of 4 × 4 tiles, one per 4-neighbor mask N 1, E 2, S 4, W 8: shape *m* at column *m* mod 4, row *m* div 4.
  Corners do not matter for wall faces.

<!-- wall index table: kept equal to JRPGAutotile by modules/jrpg/tests/test_jrpg_autotile.h -->
| Mask | Same-kind neighbors | Sheet cell (column, row) |
|---|---|---|
| 0 | none | (0, 0) |
| 1 | N | (1, 0) |
| 2 | E | (2, 0) |
| 3 | N E | (3, 0) |
| 4 | S | (0, 1) |
| 5 | N S | (1, 1) |
| 6 | E S | (2, 1) |
| 7 | N E S | (3, 1) |
| 8 | W | (0, 2) |
| 9 | N W | (1, 2) |
| 10 | E W | (2, 2) |
| 11 | N E W | (3, 2) |
| 12 | S W | (0, 3) |
| 13 | N S W | (1, 3) |
| 14 | E S W | (2, 3) |
| 15 | N E S W | (3, 3) |
<!-- end of wall index table -->

## Importing
`JRPGAutotileImporter.import_sheet(tileset, texture, kind, frames, name, data)` (editor) adds a sheet to the project's
TileSet as an atlas source and lists it in a JRPG tileset's palette: one palette entry per autotile or wall kind
(`{kind, name, source_id, atlas_coords, alternative, frames}`, the shapes at the cells above from `atlas_coords`), one
per tile for a plain sheet. It also sets Godot's terrain peering bits (match corners and sides for blobs, sides for
walls), so Godot's own terrain painter works with the same sheets. Animation frames become the tile's animation
(columns = frames; Godot's animation separation is one block minus one tile, 7 tiles for blobs and 3 for walls, so
each frame starts one block further right), played at 3 frames per second.
