# Review — Epic 8 (maps: map data, map list, cell cache, scene manager, movement, sprites, map editor, bridge)

Reviewers: two fresh-context subagents, 2026-10-14, read-only, on `git diff c2fabd838a..HEAD` (commits 8.1–8.8)
against spec 05, D-006, D-007, D-029, D-034 and D-037–D-047, and the RM-MAP, RM-MPR, RM-TIL, RM-AUD-010 and
RM-CMD-037 inventory rows (`plan/research/rpgmaker/02_maps_tilesets_assets.md`). Part A: core and runtime (map data,
map list, map files, cell cache, mover, scene manager, player, character sheets and sprites): 15 findings, 1 high.
Part B: map editor and bridge (strokes, shapes, canvas, palette, Maps tree, workspace, map and Play From Start tools):
13 findings, 1 high.

**All 28 findings fixed or answered** (2026-10-14): part B and part A's editor-side findings (A3, A4, A6) in 8.9.1,
the rest of part A in 8.9.2. Evidence: doctests for each fix (full suite 1581 cases), the end-to-end checks
(`talewright/tools/jrpg_bridge_checks.py`, 167/167: rename, create, undo/redo of the map list; far stroke corners;
refused rects, shadow strokes and palette indexes; a start cell outside a shrunken map; Rescan Maps on a copied map
scene). Close-out (8.9.3): plain_2d/plain_3d smoke with a rebuilt release template, core_diff_check, full test suite
and static checks, pushed with CI watched (results below).

## Part A (A3, A4, A6 fixed in 8.9.1; the others in 8.9.2)

Confirmed fine by the reviewer: the mover's speed table, step length, destination cell, blocked turns and edges
(D-042); the cell cache's deciding tile, flag merging, sides and looping wrap (D-037); map data arrays and resizing;
map ids never reused; the scene manager's destination checks, same-map transfers, resume fallback and teardown order
(`core/object/object.h:718-725`, `scene/main/node.cpp:281-312`); map music (RM-AUD-010, D-041); the character sheet
importer's priority and registration (`editor/import/resource_importer.h:126`); character sheet validation and walk
frames on straight walks; no RM trademarks or bare engine name.

| # | Sev | Finding | Fix |
|---|---|---|---|
| 1 | high | The scene manager read the map's cell size before the map's deferred rebuild gave it its TileSet: on any non-48 px tileset the player, its sprite and the camera limits sat on the wrong grid | fixed: `_enter` builds the entered map at once (`rebuild()` after `add_child`; the queued rebuild is skipped through `_rebuild_if_queued`), so the cell size, the player and the limits use the map's TileSet; doctest on a 40 x 30 map of 32 px tiles (cell size, player position before and after a step, camera limits) |
| 2 | med | Looping maps loop only logically: drawn once, camera unlimited on the looping axis; no task covered the looping display | answered: epic 23 (map editor completion, "map loops") gains the acceptance criterion for the looping display (tiles, characters and parallax wrapped, camera across the edge) and becomes its own subtask when the epic is split; until then the camera is free on looping axes and the limitation is stated in `JRPGSceneManager`'s and `JRPGMapData.scroll_type`'s docs, D-044 and the code |
| 3 | med | Nothing raised the map revision on save, so the edited-map warning on resume never triggered | fixed (8.9.1): `JRPGMap` marks itself edited on `cells_changed` / `layout_changed` and raises the revision on `NOTIFICATION_EDITOR_PRE_SAVE`; doctest |
| 4 | med | `JRPGMapFiles::scan` was never called; `create_map` named a rescan that did not exist | fixed (8.9.1): `JRPGMapEditing::rescan_maps()` (maps folder plus listed scenes), the Maps panel's Rescan Maps, the `jrpg_map_rescan` tool, once when the panel first opens and automatically when New Map meets an unlisted scene; an unreadable list is warned about; bridge check with a copied map scene |
| 5 | med | D-029: the map list's version defaulted to the current one (never written); map data was stamped only by some setters (not by an unchanged size or paints); neither class fingerprinted; no newer-version check | fixed: both default to -1 (`SCHEMA_UNSTAMPED`, bound with `CURRENT_SCHEMA_VERSION`) and every content change stamps (map data: every setter, the paints, and `set_size` before its early return); `upgrade()` on both (migrate older, stamp unstamped, warn about newer, as `JRPGDatabase`), run as the list's maps load, when a `JRPGMap` takes its data and when tools read map data; both in the frozen-defaults table; D-029 extended; doctests (stamps, migration step, newer data kept) |
| 6 | med | Shapes generated every cell before clipping: far corners or points overflowed int32 and could hang the editor | fixed (8.9.1): `JRPGMapShapes` take bounds (work only inside them, line cut by Liang-Barsky, int64 math; unbounded shapes over 2^20 cells refused); callers pass the map; doctests |
| 7 | low-med | The transfer fade ran on wall-clock time before the frame's steps: steps before the map change depended on the frame rate | fixed: the fade is counted in simulation steps (`FADE_STEPS` = 18 each way, map change at the darkest step) inside `_simulation_step`; D-041 and D-044 updated; doctests run the fade by steps |
| 8 | low | Walk frames summed on-screen distances per render frame: wrong across a looping seam and frame-rate dependent at corners | fixed: `JRPGMover.get_walk_distance()` (whole steps counted plus the step under way, reset after a simulation step without a step); `JRPGCharacterSprite.show_state(direction, moving, walk_distance)` and static `get_walk_frame`; D-046 updated; sprite doctest walks a looping ring |
| 9 | low | `set_maps` cleared before reading (so `set_maps(get_maps())` emptied the list) and accepted unknown or cyclic parents | fixed: builds a new array; unknown parents and loops move to the top level and maps out of tree order are put back in it, each with a warning; doctests |
| 10 | low | Paints reported every given cell in `cells_changed`, the skipped ones included | fixed: only the cells written are reported, and nothing is emitted when none was; doctest |
| 11 | low | `read_map_data_from` ignored the root's type and inherited scenes; a scene failing in `_enter` did so after the new game had begun | fixed: the root must be a `JRPGMap` (or extend it), found through `get_base_scene_state()` for inherited scenes (`scene/resources/packed_scene.cpp:1484-1493,1888-1894`), whose data comes from the base unless replaced (`own_only` for the scan, which skips such scenes); new and loaded games create their map before replacing the running game; doctests (an inherited scene, a game that cannot begin leaves the running one) |
| 12 | low | Camera limits assumed the map at the origin, scale 1 and zoom 1; the player's camera was never made current | fixed: the map's area goes through its global transform and the screen through the camera's zoom (`scene/2d/camera_2d.cpp:110,166`); the player's camera is made current on each map; doctest with a moved map root and zoom 2 |
| 13 | low | Costs: synchronous scene loads per transfer; three Dictionary lookups per cell in `rebuild`; region paints redrew the shadows; the player redrew every frame | fixed or answered: (a) answered in D-041 (the check needs the map data before the fade; a threaded preload is the reversal if large maps hitch); (b) per-palette-index tiles cached once per build; (c) separate `CELLS_REGIONS` / `CELLS_SHADOWS` codes, shadows redrawn for shadow paints only; (d) the player redraws only while its marker shows |
| 14 | low | Authority (D-007) lines missing in four headers and the mover and player docs | fixed: the cell cache, character sprite, character sheet and map files headers and class docs, and the mover and player docs |
| 15 | low | A transfer during a fade-in is refused: an autorun chain of transfers would break | answered: D-041 keeps refusing (`ERR_BUSY`); the interpreter's Transfer Player waits on `is_transferring()` / `transfer_finished` before going on, recorded in task 9.5.3's notes |

Test gaps named by the reviewer, closed: a TileSet-backed 32 px map in the scene manager (A1, A12); looping walk frames
(A8); the revision raised on save and the scan reachable from the editor and the bridge (A3, A4).

## Part B (fixed in 8.9.1)

Confirmed fine by the reviewer: one stroke is one undo action (`editor/editor_undo_redo_manager.cpp:246-256`) in the
map scene's history (`:63-101`); the Auto layer rules (D-040, D-047, RM-MAP-019); stamp anchoring; the Maps tree's
drop and move indexes, cycle refusal and shortcuts (`editor/docks/scene_tree_dock.cpp:178-265`); scene and tab
handling (`editor/editor_node.cpp:4682`, `editor/editor_main_screen.cpp:239-251`); the inspector's size undo hook
(`editor/inspector/editor_inspector.cpp:5576-5591`); D-043; the UI conventions (icons, EDSCALE, Title Case, sentence
case messages, accessibility names, empty states); `call_args_from_json`.

| # | Sev | Finding | Fix |
|---|---|---|---|
| 1 | high | Rename and move undid by swapping in a snapshot of the whole list, while create and delete had no undo step: Ctrl+Z after a New Map dropped the new map from the list (or brought a deleted one back) | fixed: `JRPGDatabaseEditing::map_list_rename` / `map_list_move` per entry (old name, old parent and index), skipping maps gone since, applied through `commit_list_edit`; bridge checks rename, create, undo and redo keep the new map |
| 2 | med | Bridge shapes had no size limit and `jrpg_map_paint`'s rect could overflow | fixed: bounded `JRPGMapShapes` (part A 6), each rect number checked against the map before multiplying; bridge checks (far corners drawn at once, a huge rect refused) |
| 3 | med | Play From Start missed start cells outside a shrunken map or unreadable scenes, and reported success when the editor refused to run | fixed: `check_start_position` at click and tool time; the tool checks `is_playing_scene()` after starting; bridge check |
| 4 | med-low | A lost or unreadable map list left New Map failing for good ("rescan the maps first" did not exist) | fixed: Rescan Maps (part A 4), run automatically when New Map meets an unlisted scene; an unreadable list warns once; the message names a real action |
| 5 | low | `jrpg_map_stroke` took `tool: "shadow"` and filed tile strokes as Draw Shadows | fixed: refused, with a hint (shadows are the pencil on layer `shadows`); bridge check |
| 6 | low | The layer picker's tooltip predated D-047 | fixed: reworded to D-047 |
| 7 | low | Palette tiles sharing a sheet cell (alternatives, transforms) landed in one grid cell | fixed: extra entries are placed in rows under the sheet; doctest |
| 8 | low | In Regions mode every hovered-cell change redrew every region rect | fixed: the grid, regions, start position and edge draw on a `marks` layer redrawn only on view or data changes; hover and preview stay on the light overlay |
| 9 | low | `jrpg_db_set_image` with default frames on an imported character sheet misreported the layout; the reimport was not undoable | fixed: frames and directions left out keep the imported layout; the result reports `import_undoable: false` when it reimports |
| 10 | low | Tileset ids and palette indexes were never checked | fixed: `check_tileset_id` on map create and set; `check_palette_indexes` in paint, stroke and view; bridge checks |
| 11 | low | Fit or center before the canvas had a size cleared the pending fit and left the map off-centre | fixed: deferred until the canvas has a size (`has_pending_center`) |
| 12 | low | The game scene path was fixed to the default addon folder | fixed: `JRPGManagedAddon::find_game_scene()` looks in every managed addon folder |
| 13 | low | Test gaps: registered-tool list, stroke and view refusals, undo of a list edit after a create | fixed: the four map tools and `jrpg_map_rescan` in the registered list; bridge checks for the refusals and the finding 1 case |

## Close-out (8.9.3)

- plain_2d/plain_3d smoke: ALL PASS with a rebuilt release template (`talewright/build_logs/smoke-20261014-095633.log`).
- core_diff_check: 5 core paths changed, 0 unlisted.
- Full test suite: 1581 cases, all pass.
- Static checks: ALL PASS (prek hooks, headers, include boundary, clang, GCC, clangd-tidy).
- Bridge checks: 167/167.
- First push (2d73d97756): CI run 38593870675 failed in four template jobs (Windows MSVC and MinGW, Linux Mono
  release and debug). The 8.9.2 inherited-scene doctest built its scene with an editor-only edit state, and CI runs
  the doctests in its release templates too. Fixed: the doctest writes the inherited scene as a text scene file; the
  `template-tests` build profile now runs the doctests in a release template locally before each push (D-048):
  1471/1471 pass there, and the editor's map data tests pass.
