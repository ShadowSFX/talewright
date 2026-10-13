# Architecture stress-test review (Plan agent)

_Planning-phase research report (2026-10-13), produced by a read-only research subagent and kept verbatim as evidence. Godot citations are at tag 4.7.2-stable; they are re-verified against the clone in task 2.2/2.3._

---

I checked the important claims against the 4.7.2-stable source; citations are `path:line` at that tag. **Short version:** the architecture holds up, but I would change five things. Maps should be scenes, not standalone Resources. Screens should be GDScript, with logic and UI building blocks staying in C++. Database IDs should follow table order. The bridge must stop the editor throttling itself while it is unfocused. Each feature should get its runtime before its editor.

---

## 1. Maps: Resource with its own canvas vs `.tscn`

**Recommendation: neither option as stated. Use a hybrid: maps saved as scenes and edited by your own canvas.**

- **File layout.** Each map is a `.tscn` whose root is a `JRPGMap` node. The root stores the layer data itself (4 tile layers in `tile_map_data` format, shadow layer, region layer, map properties, and a next-event-id counter).
- **Internal layers.** The root builds *internal* `TileMapLayer` children. Godot's own `TileMap` already does this (`scene/2d/tile_map.cpp:272`, `add_child(new_layer, false, INTERNAL_MODE_FRONT)`). Users can't break the layer structure from the Scene dock, and the layers aren't saved twice.
- **Events and user nodes.** Events are `JRPGEvent` child nodes with a stable int `event_id`; their pages are sub-resources. Anything else the user wants (lights, particles, Area2D) is an ordinary child node.
- **Canvas.** The JRPG main screen shows a SubViewport that shares the edited scene's world: `set_world_2d(EditorNode::get_scene_root()->get_world_2d())` (`scene/main/viewport.h:552`). The 2D editor sets its own transform on `scene_root` (`editor/scene/canvas_item_editor_plugin.cpp:4287`, container at `:5664-5668`). Each viewport has its own canvas transform, so the two views don't fight over pan/zoom.

**Why this wins:**
- **Undo/redo (the deciding reason).**
  - Edits to nodes go to the per-scene undo history (`editor/editor_undo_redo_manager.cpp:70-79`).
  - A standalone map `.tres` and its sub-resources fall through to `GLOBAL_HISTORY` (`:81-101`). That is one undo stack shared by every map and the database.
  - Ctrl+Z only picks the newest action among global, remote and current-scene histories (`:509-534`). Custom per-map histories created with `create_action_for_history` can't be reached by the normal shortcut or the History dock.
- **Saving, tabs and dirty markers for free.** Unsaved markers, scene tabs, Save, save-on-focus-loss and reload on external change all work as normal. Resources only get "Save All" through `is_edited` (`editor/editor_node.cpp:2360-2380`), with no per-map indicator.
- **Runtime loading.** A map is just `PackedScene.instantiate()`. You don't need a separate `JRPGMapView` builder kept in sync with the data, and F6 (run current scene) can playtest a map.
- **"Nothing Godot can do is lost."** This becomes literally true. Arbitrary nodes live in the same file, show in both canvases, and can be inherited or instanced. `overlay_scene` is not needed.
- **Diffs.** No difference between the options. `PackedByteArray` is written as base64 either way (`core/variant/variant_parser.cpp:2399-2412`). A git textconv tool can come later.
- **Effort.** You need a custom canvas in both cases, because the TileMap editor isn't extensible. The scene approach removes the save, undo, runtime-builder and overlay-preview code.

**Why not the stock 2D editor with `forward_canvas` plugins:**
- `CanvasItemEditor` only forwards input to the plugins that are editing the current selection (`canvas_item_editor_plugin.cpp:2772`).
- The "always forward input" option (`editor/plugins/editor_plugin.cpp:257-259`) is only honored by the 3D editor (`editor/scene/3d/node_3d_editor_plugin.cpp:2203`). So the JRPG tools would disappear whenever a non-JRPG node is selected.
- The 2D editor's own shortcuts would also collide with the tools.
- Keep this as the fallback. The fix would be a ~5-line patch that could go upstream: make `_gui_input_viewport` honor the always-forward option.

**Main risk:** tight coupling to the editor's open-scene lifecycle (clicking a map opens a scene tab; switching, closing, external reloads, the order of `edit()`/`make_visible()`). World sharing also has gaps: `CanvasLayer` children of a map don't show in the JRPG view.

**Mitigation:**
- Put all `EditorNode` access in one `MapEditSession` adapter that re-binds on `scene_changed`/`scene_closed`.
- Use "preview tab" behavior: auto-close the previous map tab if it's unmodified.
- Make the **first map task a spike**: open, paint, undo, save, reopen, run, all checked through the bridge. Fall back to the 2D-plugin route if it fails; the data model is the same.
- Record each brush stroke as one undo action that stores a compact before/after patch (`_apply_patch(PackedInt32Array)`), not thousands of `set_cell` calls.
- Cross-map tools (event search, finding switch usages) can read `PackedScene::get_state()` without instancing the map.

Also replace Godot terrains with **your own autotile resolver**:
- It applies the RPG Maker same-kind 8-neighbour rule, then picks the matching tile from the baked atlas.
- `set_cells_terrain_connect` is slow and rewrites neighboring cells in non-RPG-Maker ways when several terrains touch.
- Keep the terrain metadata so the stock editor still works.

## 2. All-C++ presentation vs GDScript screens

**Recommendation: split it.**
- **C++:** all game logic (data model, interpreter, game state, battle, movement/passability, save) plus generic UI building blocks:
  - window-skin frame with open/close animation
  - selectable list (cursor, paging, key repeat, mouse)
  - message renderer (control codes, typewriter speed and waits)
  - gauges, face and character sprite helpers
- **GDScript + .tscn:** the screens themselves (title, menus, battle layout, shop, name input, options, save/load).
  - They ship as a managed addon (`res://addons/talewright/`) that setup copies into the project.
  - The sources are embedded in the engine binary, using the same pattern as `modules/gdscript/editor/script_templates/SCsub` (`template_builders.make_templates`).

**Deciding reasons:**
1. **Customization.** RPG Maker users can override *any* method through JS plugins. GDVIRTUAL hooks can't cover that surface; editable GDScript screens can.
2. **Iteration speed for the agent.** The UI is the most visually iterative part. GDScript hot-reloads, while each C++ tweak costs an MSVC relink plus an editor restart.
3. **A smaller, stable C++ API.** Add-ons target the logic layer, which is the part that needs doctest coverage and determinism.

Export templates don't decide this: the C++ runtime needs custom templates either way.

**Mitigations:**
- **Version drift of the copied scripts:** stamp each managed file with a version and hash. An "Upgrade JRPG scripts" tool overwrites untouched files and lists the ones the user changed.
- **Script correctness:** a doctest compiles every embedded `.gd` file with the GDScript parser.
- **Behavior:** check it through bridge end-to-end runs.
- **Plugin parity:** add a `JRPGPluginRegistry` for ordered hook scripts, with plugin parameters exposed as `@export` vars.

## 3. Database format

**Recommendation: keep one `.tres` per table, with these changes.**

- **ID equals slot, kept in order. Drop reordering.**
  - `[sub_resource]` blocks are written in the order the saver discovers them. Reordering the array reshuffles the whole file, and the one-line `entries=[SubResource(...)…]` array changes on every add.
  - RPG Maker has the same model (ID = slot, "Change Maximum", clear instead of delete), so importing RPG Maker data maps 1:1. The reference scanner should warn before "Change Maximum" cuts entries that are still referenced.
- **Deterministic sub-resource IDs.**
  - The text saver keeps existing IDs and only generates IDs for resources that lack one (`scene/resources/resource_format_text.cpp:1896-1935`; random seed derived from the path at `:1760`).
  - Set readable IDs such as `Item_42` before saving; `set_scene_unique_id` accepts identifier characters (`core/io/resource.cpp:171-182`).
  - Clear the ID on pasted copies (made with `duplicate_deep`, `core/io/resource.h:155`).
- **Nested lists are values, not Resources.** Traits, effects, learnings, enemy actions, drops and troop members should be packed arrays or typed dictionaries. Making each one a sub-resource multiplies blocks and makes diffs noisy.
- **A manifest file** (`res://jrpg/database.tres`) refers to each table as an ExtResource. Godot's dependency fixer then survives moved files and UIDs work. Never reference `table.tres::sub_id` from outside the table.
- **Inspector:** edit only the selected entry in the inspector, never the table. Array paging defaults to 20 items (`editor/settings/editor_settings.cpp:575`). Hide `entries` with `PROPERTY_USAGE_NO_EDITOR`.
- **Saving and undo:** mark the table with `set_edited(true)` and "Save All" picks it up (`editor_node.cpp:2360-2380`). Database undo uses `GLOBAL_HISTORY`, which is fine here.
- **Edits made on disk while the editor is open:** `_resources_changed` calls `reload_from_file` (`editor_node.cpp:1291-1321`, connected at `:9568`). The database UI must refresh on that signal, and the agent should not write `.tres` files while the editor is running.
- **Migrations:** handle legacy property names with `_set()` overrides plus `schema_version`.
- **Load time is fine.** Exports convert text resources to binary (`editor/export/editor_export_platform.cpp:1483`).
- **Rejected alternatives:**
  - One file per entry: thousands of files and UIDs, and slow bulk operations.
  - A custom JSON format: you would re-implement dependencies, renaming, UIDs and export remaps. Do a one-way RPG Maker JSON *importer* instead.
- **Event commands (part D):** typed command Resources are fine, but gate them on a benchmark: 10k commands should load in under about 30 ms and re-save byte-identically. If that fails, keep the same API but store commands as packed data behind a single storage property.

## 4. Bridge

**Main thread vs worker thread: poll on the main thread.**
- Every useful tool touches editor or scene objects, and debugger messages are pumped on the main thread anyway.
- A worker thread would only speed up raw socket I/O; the time to dispatch a tool stays the same.
- Use a worker later only if you want instant "busy" replies while the main thread is stuck.

**Latency, and what to fix:**
- The editor keeps iterating when unfocused, but it sleeps 100,000 µs per frame (`editor_settings.cpp:554`, applied on focus-out at `editor_node.cpp:1064-1073`). It also sleeps when the window can't draw, e.g. minimized (`core/os/os.cpp:721-722`). The focused value is 6,900 µs (`editor_settings.cpp:548`).
- An agent driving from a terminal leaves the editor unfocused, so expect 0.3–0.5 s per call.
- **Fix:** while a client has been active recently, call `EditorNode::set_unfocused_low_processor_usage_mode_enabled(false)` (`editor_node.cpp:7996`; the Game view does the same at `editor/run/game_view_plugin.cpp:567/591`). Also set the sleep to the focused value right away, and restore both when idle.

**Other required pieces:**
- **Editor screenshots.** In low-processor mode the editor only draws when something changed (`main/main.cpp:5087-5095`). Call `Main::force_redraw()` (`main.cpp:5207`) and reply after `frame_post_draw`.
- **Deferred responses.** Keep a table of pending requests (connection, JSON-RPC id, deadline, cancel on disconnect). Complete them from signals or from game messages received through an **`EditorDebuggerPlugin`** (`editor/debugger/editor_debugger_plugin.h:72-94`, registered with `add_debugger_plugin` at `editor_plugin.h:261`). Don't hook into `ScriptEditorDebugger` internals.
- **Re-entrancy.** `ProgressDialog::_update_ui` calls `Main::iteration()` from inside long operations (`editor/gui/progress_dialog.cpp:148-153`), so your process callback can run in the middle of a save or import. Do socket I/O only while another request is executing, and answer "busy" during scans and imports.
- **Disk changes aren't picked up while unfocused.** File rescans happen only on focus-in (`editor_node.cpp:1049-1058`). Add a `rescan` tool that calls `EditorFileSystem::scan_changes`.
- **Large data.** Debugger packets are capped at 8 MiB (`core/debugger/remote_debugger_peer.cpp:69,84-85`). Send images as temp-file paths, as `scene_debugger.cpp:540-563` already does.
- **Game logs.** Game output reaching the editor is rate-limited (`network/limits/debugger/max_chars_per_second` = 32768, `main.cpp:2295`). Keep a game-side ring buffer via print/error handlers and fetch it on demand.
- **JSON numbers.** All numbers parse as `double` (`core/io/json.cpp:393`). Echo integer JSON-RPC ids as integers and cast integer arguments.
- **Security.** Check the Host header as well as Origin (DNS-rebinding defense).
- **Survive restarts.** Every build restarts the editor. Use a persistent token and a fixed port so the `claude mcp add` registration stays valid, and make the Python CLI the agent's primary interface.

**Is registering the debugger capture at EDITOR init level in the game process safe? Yes.**
- `EngineDebugger::initialize` runs in `Main::setup` (`main.cpp:2300`), before `register_editor_types` and EDITOR-level module init in setup2 (`main.cpp:3822-3825`). So `is_active()` (`core/debugger/engine_debugger.h:106`) is valid at that point.
- `captures` is a static map (`engine_debugger.h:99`), so registering doesn't need the debugger singleton.
- `SceneDebugger` does the same check at SCENE level (`scene/register_scene_types.cpp:1315`, `scene/debugger/scene_debugger.cpp:95-96`).

Caveats:
- `test_setup` also runs EDITOR init without a debugger (`main.cpp:836-841`), so the guard is required there too.
- The SceneTree doesn't exist yet at init time; look it up per message.
- Handlers can be called while the game is paused at a breakpoint, so "wait N frames" tools need a timeout.
- Unregister at EDITOR uninit (`main.cpp:5288`).

**Open question from part E is solved:** show or hide the JRPG workspace at runtime with the static `EditorNode::add_editor_plugin`/`remove_editor_plugin` (`editor/editor_node.h:778-779`; these handle main screens, `editor_node.cpp:4365-4374`). Call them from a small bootstrap plugin that listens to `ProjectSettings::settings_changed`.

## 5. Build order

**Changes to the order:**
- **Toolchain baseline first**, running in parallel with research. It must include template_debug and template_release builds plus a build wrapper.
- **Bridge first version is CLI-first.** Full MCP polish can come later.
- **Decisions phase adds:** the event-storage benchmark, the map-scene spike, the presentation split, localization (text keys and translation-parser hooks), the formula evaluator, and the save format.
- **Game state *and interpreter state* must be serializable from day one.** RPG Maker saves running interpreters, so interpreter positions should be stored as paths (event/page/index-path), never object pointers.
- **Placeholder assets before tilesets.**
- **Runtime before editor, per feature.**
- **Custom script commands** (script class extending `JRPGEventCommand`) and an **exported build** are both inside the vertical slice.
- **RPG Maker JSON importer** is the first epic after the slice, because it shows what's missing for parity.

**Vertical-slice tasks:**

| # | Task | Acceptance |
|---|---|---|
| 1 | Fork at tag, task ledger, ADR folder | `git describe` = 4.7.2-stable; ledger lists tasks |
| 2 | Build wrapper (kills exes holding the file lock, timing, error extraction) | Editor dev build and both templates build; no-op rebuild time logged |
| 3 | Baseline `--test` run | Pass/fail baseline saved; rerun diff empty |
| 4 | Vanilla smoke (2D/3D samples, export with custom template) | Exit 0; exported exe runs |
| 5 | `claude_bridge` module skeleton + HTTP parser | Module off = identical build; parser doctests (partial reads, 401/403/413) |
| 6 | JSON-RPC/MCP layer + Python CLI | `tw-bridge status` works; both protocol shapes pass doctests |
| 7 | Editor tools (logs, screenshot, open/run/stop, save, rescan, settings) | CLI screenshot PNG; game print shows up in logs |
| 8 | Game-side capture (screenshot, tree, get/set property, input for N frames, wait) | Injected ui_accept changes a sample game; timeouts return errors |
| 9 | Bridge hardening (re-entrancy, busy, throttle off, port in use) | 1000 calls during an import: no crash; median latency <50 ms unfocused |
| 10 | `jrpg` module skeleton, `jrpg/enabled`, dynamic workspace | Module off = test baseline identical; setting off = no tab; templates build |
| 11 | DB core types (tables, entries, compact traits, note tags, manifest) | Save → load → save is byte-identical; migration test passes |
| 12 | Game state + interpreter-state serialization | Round-trip equality doctest |
| 13 | Parameter/trait math + formula evaluator | Table of hand-computed RPG Maker cases passes |
| 14 | "Set up JRPG project" tool (folders, DB, input incl. gamepad, managed addon, settings) | Idempotent on an empty project; screenshot shows JRPG tab |
| 15 | Workspace shell (map tree, palette, canvas, playtest) | Layout screenshot; tab switching produces no errors |
| 16 | Database editor framework (list, form, ID picker, trait editor, undo) | Bridge: create/rename/undo/save item; minimal `.tres` diff |
| 17 | Placeholder asset generator | Deterministic file hashes; assets render |
| 18 | Tileset importer (bake floor autotiles, tile flags as custom data) | Pixel-compare all 48 floor shapes |
| 19 | Autotile resolver | Matches reference on random 64×64 maps |
| 20 | `JRPGMap` node (internal layers, events, properties) + spike | Open → paint → undo → save → reopen → run via bridge |
| 21 | Map list + transfer/scene manager | Transfer to map 2 is visible in screenshot |
| 22 | Passability + player movement + camera | Passability doctest matrix; bridge walk is blocked by wall |
| 23 | Character sprite (pattern, direction, priority, through) | Walking frames visible in screenshot sequence |
| 24 | Map canvas tools (pencil/rect/ellipse/fill/eraser, shadow, region, auto-layer) | Golden screenshot; undo restores tile hash |
| 25 | Tile palette (tab sets, stamps, eyedropper) | 2×2 stamp paints correctly |
| 26 | Events + page conditions/refresh | Page-selection doctest matrix |
| 27 | Command model, registry, script custom commands | Script subclass is discovered and executed |
| 28 | Interpreter (call stack, waits, common events, parallel/autorun, labels across tree) | Deterministic step-by-step doctests |
| 29 | Slice commands (~20) | One doctest per command |
| 30 | Event editor (pages, command tree, forms, quick events) | Quick treasure chest → playtest gains the item |
| 31 | Window frame + selectable list (C++) | Navigation doctest + golden screenshot |
| 32 | Message system (control codes, typewriter, face, choices, translation lookup) | Parser doctest; end-to-end message advance |
| 33 | Screen stack + transitions (GDScript) | Embedded-script compile doctest passes |
| 34 | Menu (items, status, save) | Using a potion raises HP (state query) |
| 35 | Battle model (seeded turn order, AI, rewards, level-up) | Golden battle log for fixed seed |
| 36 | Battle scene (front view) | Bridge-driven battle test is won |
| 37 | Encounters + minimal troop events | Seeded encounter triggers within N steps |
| 38 | Save/load slots | Save → quit → continue gives an identical state hash |
| 39 | Title, game over, boot flow | Fresh build boots to title |
| 40 | Battle test + "test from here" | Battle test opens the battle directly |
| 41 | Export of the slice | Exported exe boots with no errors; no editor code linked |
| 42 | Slice demo + scripted playthrough + perf budget + vanilla regression rerun | Passes 3 times in a row; baseline diff empty |

## 6. Top 10 risks and mitigations

1. **Scope and parity breadth** (~110 commands, plugin culture). Keep a parity table generated from the RPG Maker inventory, with status and a test per row. Define parity as feature-equivalent, not JS-API-equivalent.
2. **Windows build loop.** A running editor or game exe holds the file lock, so the link fails (LNK1104); MSVC links are long; `dev_mode` turns warnings into errors. Quit through the bridge, then taskkill by path. Use `scu_build` for full builds and run tests filtered by tag.
3. **Bridge lifecycle.** The editor restarts on every build, throttles when unfocused, doesn't rescan unfocused, can re-enter, and modal dialogs can block. Apply the §4 measures, add a `dismiss_dialog` tool, and keep the CLI primary.
4. **Vanilla regressions from a default-on module.** `GLOBAL_DEF` settings appear in every project; JRPG classes appear in the Create dialog; startup time; missing `doc_classes`. Define only `jrpg/enabled` and keep everything else in the System resource. Add automated checks: empty-project settings diff, startup-time budget, `--doctool` check, test-baseline diff, and a script that only allows core diffs from a fixed list.
5. **Fragile map-editor integration** (open-scene lifecycle, world sharing). Run the spike early, isolate it behind `MapEditSession`, and keep the 2D-plugin fallback.
6. **Schema churn across 40+ tasks.** Use `schema_version` plus `_set()` migrations, fixture projects regenerated by a tool, and freeze the schema at the slice milestone.
7. **Runtime semantics.**
   - Jump-to-Label can jump into branches, which is awkward in a command tree.
   - Saving mid-event needs path-addressed interpreter state.
   - Expression has no ternary operator and is not sandboxed.

   Mitigations: resolve labels across the tree into a path; serialize interpreter state as paths; use a whitelisted formula context or a small custom parser.
8. **Tile fidelity** (animated A1 water frames 0-1-2-1, A3/A4 walls, table tiles, shadows, star-tile priority). Bake the atlas, use your own resolver, and drive tests from tables.
9. **Verification blind spots.** `[Editor]` doctests don't create an `EditorNode`, and judging pixels is fuzzy. Assert on state queries through the bridge. Use fixed seeds, window size, editor scale and DPI. Use golden screenshots with tolerance.
10. **Legal/IP.** Never copy RTP assets, `rmmz_*.js` code, default database content or help text. Re-implement behavior clean-room from documentation, with original generated assets. The importer is only for user-owned projects.

### Critical Files for Implementation
These are upstream Godot files; the working directory is empty.
- `editor/editor_undo_redo_manager.cpp` (undo routing, decisive for map storage)
- `editor/editor_node.cpp` (focus/throttle handling, rescans, saving resources, dynamic plugins)
- `editor/scene/canvas_item_editor_plugin.cpp` (scene_root viewport, input forwarding limits)
- `scene/resources/resource_format_text.cpp` (sub-resource ID stability and ordering)
- `main/main.cpp` (debugger vs module init order, iteration and redraw)
