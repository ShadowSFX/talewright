# Review — Epic 7 (assets: generated defaults, autotiles, tile flags, tileset form, resolver, import tools)

Reviewers: two fresh-context subagents, 2026-10-14, read-only, on `git diff 9e26221d20..HEAD` (commits 7.1.1–7.5)
against spec 05, D-005, D-006, D-011, D-012, D-031, D-034 and the RM-TIL/RM-AST inventory rows
(`plan/research/rpgmaker/02_maps_tilesets_assets.md`). Part A: asset generator, autotile core, importer, resolver
(tasks 7.1, 7.2, 7.4): 9 findings, none high. Part B: tile flags, tileset form, bridge tools (tasks 7.3, 7.5): 14
findings, none high.

## Part A (fixed in 7.6.1)

Confirmed fine by the reviewer: the mask math is consistent across the generator, `JRPGAutotile`, the importer's
peering bits and the resolver; the animation layout and the importer's creation order fit Godot's atlas rules
(`scene/resources/2d/tile_set.cpp:5055-5150,5203,5305`); every failed import changes nothing; the resolver's wrap,
seam and malformed-input handling, determinism and core boundary; the brute-force reference shares no code with the
resolver; generator determinism; asset sizes and the icon table; the `.ogg.import` sidecars; CC0 licensing and no RM
material.

| # | Sev | Finding | Fix |
|---|---|---|---|
| 1 | med | `generate_assets.py --check` compared the in-memory output with the manifest only: an edited PNG, a deleted `.import` or a changed import setting passed; regenerating trusted the manifest and never restored them; not run by any check | fixed: `--check` also compares the committed files (each PNG's pixels, each Ogg's length and channels, each `.import` text, stray files); sound entries record every import setting; regenerating rewrites what differs on disk (Ogg only when its samples change, so no serial-number churn) and recreates sidecars; `static_checks.py` runs it ("generated assets", skipped when a library is missing); verified by editing a PNG and deleting a sidecar (both reported, regenerating restored them byte for byte) |
| 2 | low-med | The resolver's flag bits do not line up with `JRPGTilesetData::EdgeMode` or spec 05's scroll-type order, with no helper: future map code could swap axes | fixed: `JRPGAutotileResolver.get_flags(loop_horizontal, loop_vertical, edge_mode)` (bound, documented); spec 05 states the scroll-type mapping (type & 2 horizontal, type & 1 vertical); doctest over the four scroll types |
| 3 | low | `import_sheet` accepted any integer kind and frames < 1 from scripts | fixed: unknown kinds and frames < 1 are refused with a reason; doctests |
| 4 | low | `get_blob_cell` / `get_wall_cell` had no range checks (47 gave the preview cell, 16 wrapped) | fixed: `ERR_FAIL_INDEX_V`, (-1, -1) out of range, documented; doctests |
| 5 | low | Animation speed hard-coded and undocumented | answered: kept at 3 frames per second for every animated sheet (Godot's atlas animation speed 3, frame duration 1 s), now documented in spec 05 and the template guide; a per-entry speed can come with the map editor's animation settings if needed |
| 6 | low | Bit masks bound as plain enums; static-only classes instantiable | fixed: `Neighbor`, `Side` and the resolver's `Flags` are bitfields (`BIND_BITFIELD_FLAG`, `BitField<>` parameters, `is_bitfield` in the docs); `JRPGAutotile`, `JRPGAutotileResolver`, `JRPGAutotileImporter` and `JRPGTileFlags` are registered abstract |
| 7 | low | Test gaps: peering bits of 2 shapes only, no animated wall sheet, script wrappers untested, `changed` checked one way, "two layers" trivially true | fixed: every blob (47) and wall (16) shape's peering bits against its mask; an animated wall sheet (separation (3, 0), frame region); `import_sheet_bind` and `update_around_bind`; `changed` lists exactly the cells whose shape changed (plus the edited cell). The two-layer case stays as the documentation of a per-layer API (layers cannot influence each other by construction) |
| 8 | low | D-034's reversal ("change `TILE` and regenerate") would draw broken art: the painters use fixed 48 px geometry | fixed: the reversal names what must scale by `TILE / 48` first |
| 9 | low | Doc nits: ambiguous "separation = one block", a citation off by a few lines, "generated" table comment, British spellings, no pinned library versions | fixed: separation explained (one block minus one tile, 7 and 3), `tile_set.h:218-235`, "kept equal" comments, US spellings in docs and comments, `talewright/tools/asset_gen/requirements.txt` pins numpy, Pillow and soundfile |

## Part B (fixed in 7.6.2)

Confirmed fine by the reviewer: the order of `add_tile_sheet`'s undo and redo (source removed first, terrain and
terrain set after; redo puts the terrain set, mode and terrain back before the same source, so its peering bits
survive, `scene/resources/2d/tile_set.cpp:5963-5976`, as `editor/scene/2d/tiles/tile_set_editor.cpp:276-288`); no
array shared with the history is mutated; `set_tile_flags` validates everything before writing; staleness compares
the words as well as the hash; the form's deferred refresh and its connections; keyboard handling; bridge path
validation and annotations; no engine file changed; every Godot citation spot-checked.

| # | Sev | Finding | Fix |
|---|---|---|---|
| 1 | med-high | Flags painted in Godot's TileSet editor left the bake stale and nothing rebuilt it: every save route required an unsaved database file | fixed: `JRPGDatabaseEditing.get_stale_tilesets` / `has_changes_to_save`; the bootstrap plugin's unsaved status (quit prompt, Save All, `editor/editor_node.cpp:2414-2421`) and `save_external_data` (scene save, run) count stale bakes; the Database section lists "tile flags of tileset N" and enables Save; the TileSets read are watched so painting refreshes the section; doctest paints the custom data directly and saves |
| 2 | med | Overlays and side clicks were off on the last column (`get_item_rect` expanded) and not mirrored right to left | fixed: `icon_rect` uses the unexpanded rectangle, as ItemList centers the icon (`scene/gui/item_list.cpp:346-351,1592`), and mirrors it in RTL (`:1616-1618`); doctests; screenshot `plan/evidence/jrpg/7.6.2-terrain-tags-last-column.png` (the pine tree's mark now centered) |
| 3 | med | `jrpg_db_set_image` was cut off at the bridge's 10 s default, not the documented 20 s | fixed: the tool's timeout is the import wait plus 5 s, so its own message reports a slow import |
| 4 | low-med | Skipping `scan_changes` during a scan could miss a file copied after the scan began | fixed: always asks for a scan (a running one queues another, `editor/file_system/editor_file_system.cpp:1710-1716`) and asks once more if a finished scan missed the file |
| 5 | low-med | `jrpg_tileset_set_flag` reported `undoable: true` for an edit that made no undo step | fixed: `set_tile_flags` reports whether it changed anything; the tool returns `changed` and `undoable: false` then; doctest and end-to-end check |
| 6 | med | The riskiest undo code was not really tested: the round trips compared palette and flags only; `jrpg_tileset_create` not round-tripped | fixed: `jrpg_tileset_flags` reports the TileSet's sources (with tile counts) and terrain sets (mode, terrains), and the round trips compare the whole result, so undo must remove the source, terrain and terrain set and redo restore them; `jrpg_tileset_create` round-tripped; `jrpg_bridge_checks.py` 74/74 |
| 7 | low | Two forms (workspace and Inspector dock) shared one static mode: buttons and overlays could disagree | fixed: each form keeps its own mode (a new form starts in the last one chosen); doctest with two forms |
| 8 | low | The Menu key edited a tile (ItemList reports it as a right click) | fixed: the form takes `ui_menu` itself and does nothing; doctest |
| 9 | low | `add_tile_sheet` (and `set_tile_flags`) did not force the global history | fixed: `force_fixed_history` on both |
| 10 | low | Flipped or rotated entries silently share flags with the base tile | answered and shown: Godot keeps tile data per alternative without transform bits (`scene/resources/2d/tile_set.cpp:5474`), so sharing is inherent; spec 05, the form's tooltip and the bridge (`transformed`) say so |
| 11 | low | Shapes painted differently were invisible in the form | fixed: a warning-colored dot and a tooltip mark mixed kinds (`is_mixed`); doctest |
| 12 | low | `create_tileset` could orphan a palette by re-pointing a tileset whose TileSet loads | fixed: refused unless `replace` (bridge argument); the form only offers Create TileSet when no TileSet loads; doctests (nothing written on refusal) |
| 13 | low | `jrpg_db_save` listed files before saving (missed a table saved only for its rebake); `add_sheet` reported clamped frames | fixed: `save_all` returns what it wrote and the tool reports that; frames outside 1-64 are refused, not clamped |
| 14 | low | A tileset whose `tileset_uid` was cleared kept its old bake | fixed: the next save clears the table (a TileSet that exists but fails to load keeps it, so a passing load error loses nothing); doctest |
