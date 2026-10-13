# Godot — current state of the fork (task 2.2)

Repository: `ShadowSFX/talewright` (fork of godotengine/godot), branch `talewright`, base tag `4.7.2-stable`
(commit `ed1daf0bf0`, "Bump version to 4.7.2-stable"). Every `path:line` below was checked against this
clone with `python plan/tools/check_citations.py plan/research/godot_current_state.md`. Deeper verbatim
reports (with more citations) are in `plan/research/planning_notes/01..05`.

## 1. Version and branch
- `version.py:1-9`: `short_name="godot"`, `name="Godot Engine"`, major 4 / minor 7 / patch 2, status `stable`,
  `docs="4.7"`. Read by `methods.get_version_info` (`methods.py:136-165`) into `GODOT_VERSION_*` defines
  (`core/core_builders.py:22-32`).
- User data dir name comes from `GODOT_VERSION_SHORT_NAME` (`core/os/os.cpp:294-297`; Windows capitalizes it,
  `platform/windows/os_windows.cpp:2466-2468`), so the fork shares `%APPDATA%\Godot` with vanilla 4.7 (D-003).

## 2. Build system and commands (from the repo, not memory)
- SCons ≥ 4.4 / Python ≥ 3.9 (`SConstruct:4-5`); VS2022 needs SCons ≥ 4.8 (`platform/windows/detect.py:187-188`).
  CI pins `scons==4.10.1` (`.github/actions/godot-deps/action.yml:13,30`; Python `3.x` at `:7`). Installed here: SCons 4.10.1
  (`python -m SCons`), clang-format 21.1.7 (pre-commit pin, `.pre-commit-config.yaml:20-23`).
- Targets: `target=editor|template_release|template_debug` (`SConstruct:162-166`); `editor` defines
  `TOOLS_ENABLED` (`SConstruct:548-549`); `editor`/`template_debug` define `DEBUG_ENABLED` (`SConstruct:551-554`).
- `dev_mode=yes` ⇒ `verbose=yes warnings=extra werror=yes tests=yes strict_checks=yes` (option help `SConstruct:237`,
  applied at `SConstruct:668-673`).
- Windows CI editor build: `.github/workflows/windows_builds.yml:8-33` (flags `dev_mode=yes
  module_text_server_fb_enabled=yes debug_symbols=no`, editor job adds `windows_subsystem=console vsproj=yes`).
- **Local build (verified on this machine, task 1.1/1.2):** `python talewright/tools/build.py editor` =
  `python -m SCons --site-dir=talewright/tools/site_scons platform=windows d3d12=no target=editor dev_mode=yes -j32
  accesskit_sdk_path=<repo>/bin/build_deps/accesskit`. Findings:
  - SCons ranks VS instances Enterprise > Professional > Community > BuildTools and does not fall back when the
    preferred instance lacks a compiler; with an incomplete Community install on D: `msvc_exists()` was False and
    `platform/windows/detect.py:179` fell back to MinGW ("No valid compilers found", `detect.py:682`). Fixed without
    Godot changes by a vswhere shim registered from `talewright/tools/site_scons/site_init.py`.
  - D3D12 driver needs extra SDKs (`misc/scripts/install_d3d12_sdk_windows.py`) ⇒ `d3d12=no` (Vulkan + OpenGL kept).
  - AccessKit installed by `misc/scripts/install_accesskit.py` (v0.22.3); from Git Bash (`MSYSTEM` set) both the
    script and `platform/windows/detect.py:206-209` use `bin/build_deps`, so the wrapper pins the path.
  - Result (task 1.2, 2026-10-13): vanilla editor `dev_mode=yes` full build **5.8 min on 32 cores, 0 errors,
    0 warnings** (werror on), toolchain "Visual Studio 14.3 with Windows SDK 10.0.22621.0"; no-op rebuild 0.4 min.
    Outputs: `bin/godot.windows.editor.x86_64.exe` (178 MB) and the console wrapper
    `bin/godot.windows.editor.x86_64.console.exe`. Logs in `talewright/build_logs/` (git-ignored).
  - Export templates (task 1.4): `template_release` 3.7 min → `bin/godot.windows.template_release.x86_64.exe` (68.6 MB);
    `template_debug` 3.9 min → `bin/godot.windows.template_debug.x86_64.exe` (83.0 MB); 0 errors, 0 warnings.
  - Regression smoke (task 1.5): `python talewright/tools/smoke_samples.py` — plain_2d/plain_3d open in the editor,
    run with `-- --smoke`, and plain_2d exports with the custom release template and runs: ALL PASS (2026-10-13).
- Tests: built when `tests=yes` (`SConstruct:1238-1239`); `--test` handled by `Main::test_entrypoint`
  (`main/main.cpp:982-997`) → doctest (`tests/test_main.cpp:75-147`). Windows CI runs
  `bin/godot.windows.editor.x86_64.exe --test --force-colors` (`.github/workflows/windows_builds.yml:137-141`).
  Subsets: `--test --test-case="*Foo*"` / `--test-suite=` (doctest options, `thirdparty/doctest/doctest.h:6645-6656`).
- Class docs: CI fails on `--doctool` diffs (`.github/workflows/linux_builds.yml:249-255`); `--doctool` writes a
  class's XML to its module `doc_classes` when listed in `get_doc_classes` (`editor/doc/doc_tools.cpp:1687-1699`).

## 3. Modules
- A module = directory with `register_types.h`, `SCsub`, `config.py` (`methods.py:302-309`); option
  `module_<name>_enabled`, default from `config.is_enabled()` (`SConstruct:476-485`).
- `config.can_build(env, platform)` gate; editor-only pattern `return env.editor_build`
  (`modules/xatlas_unwrap/config.py:1-2`); doc classes via `get_doc_classes`/`get_doc_path` (`SConstruct:1112-1140`).
- Generated `register_module_types.gen.cpp` calls `initialize_<name>_module(level)`
  (`modules/modules_builders.py:15-51`); levels CORE/SERVERS/SCENE/EDITOR (`modules/register_module_types.h:35-40`);
  EDITOR level runs after `register_editor_types()` in TOOLS builds (`main/main.cpp:3818-3826`) — including the
  play process, which is the editor binary itself (`editor/run/editor_run.cpp:161`).
- Editor plugins from modules: `EditorPlugins::add_by_type<T>()` (`editor/plugins/editor_plugin.h:282-311`), e.g.
  `modules/gridmap/register_types.cpp:43-55`.
- Module tests: `<module>/tests/*.h` collected into `modules/modules_tests.gen.h` (`modules/SCsub:44-55`) and
  included by `tests/test_main.cpp:73`; tags `[SceneTree]`, `[Editor]`, `[Audio]` set up mocks
  (`tests/test_main.cpp:176-248`); example `modules/gridmap/tests/test_grid_map.h:42`.

## 4. Editor extension points
- `EditorPlugin` virtuals: `get_plugin_name`, `get_plugin_icon`, `has_main_screen`, `make_visible`, `edit`,
  `handles` (`editor/plugins/editor_plugin.h:196-205`); `add_dock` (`:167`), `add_tool_menu_item` (`:170`),
  `add_inspector_plugin` (`:249`), `add_debugger_plugin` (`:261`), `add_export_plugin` (`:240`).
- `EditorDock` (`editor/docks/editor_dock.h:40`), slots incl. bottom (`:51-65`); bottom panel API deprecated in
  favour of docks (`doc/classes/EditorPlugin.xml:427`).
- Main screens: `EditorMainScreen` (`editor/editor_main_screen.h:41`), `add_main_plugin`
  (`editor/editor_main_screen.cpp:262-290`), hide via `set_button_enabled` (`editor/editor_main_screen.cpp:106-112`);
  runtime plugin add/remove `EditorNode::add_editor_plugin/remove_editor_plugin` (`editor/editor_node.h:778-779`).
- Inspector: `EditorInspectorPlugin` (`editor/inspector/editor_inspector.h:339`), `EditorProperty`
  (`editor/inspector/editor_inspector.h:70`).
- Undo/redo: `EditorUndoRedoManager` (`editor/editor_undo_redo_manager.h:36`, `create_action` `:92`); history
  routing per scene vs global (`editor/editor_undo_redo_manager.cpp:70-101`).
- Feature profiles are global editor settings (`editor/settings/editor_feature_profile.cpp:348`).
- Project Manager has no template mechanism; new project writes `project.godot`, `icon.svg`, VCS metadata
  (`editor/project_manager/project_dialog.cpp:561-634`).

## 5. GDExtension
- `core/extension/` (interface JSON → generated C header, `core/extension/SCsub:17-18`); only exposed ClassDB
  classes are in `extension_api.json` (`core/extension/extension_api_dump.cpp:917-941`). Editor internals
  (`EditorNode`, `EditorMainScreen`) are not ClassDB-registered ⇒ engine modules can do more than GDExtension.
- libgodot API `core/extension/libgodot.h:48-71`; the one-instance-per-process limit is enforced by each platform,
  e.g. `platform/windows/libgodot_windows.cpp:42`.
- GDExtensions share the module initialization levels and can register editor plugins
  (`GDExtensionEditorPlugins::add_extension_class`, `core/extension/gdextension.cpp:1108`); `--dump-extension-api`
  dumps the exposed API. `EditorNode`/`EditorMainScreen` are GDCLASS objects but not exposed (ClassDB `exposed`
  flag, `core/object/class_db.h:151`), so engine modules can reach editor internals that extensions cannot.

## 6. TileMap / TileSet
- `TileMapLayer` (`scene/2d/tile_map_layer.h:332`): `set_cell` (`:557`), terrain painting
  `set_cells_terrain_connect/path` (`:580-581`), runtime tile data hooks (`scene/2d/tile_map_layer.h:591-594`); data property
  `tile_map_data` (`scene/2d/tile_map_layer.cpp:2272`).
- `TileSet` (`scene/resources/2d/tile_set.h:149`): terrain modes (`:240-244`), custom data layers (`:500-507`),
  physics/navigation/occlusion layers; `TileData` custom data (`:1028-1029`); animated atlas tiles on `TileSetAtlasSource`
  (`scene/resources/2d/tile_set.h:754-766`); scene tiles via `TileSetScenesCollectionSource` (`scene/resources/2d/tile_set.h:796`). Old `TileMap` node deprecated (`doc/classes/TileMap.xml:2`).
- No quarter-tile autotile composition and no region concept ⇒ Talewright importer/resolver and custom data.

## 6b. Media gaps relevant to RPG Maker parity
- Video: the only video module in the tree is `modules/theora` (Ogg Theora); WebM/MP4 movies (RM-AUD-012) need a
  conversion step or an extension.
- Ogg loops: `AudioStreamOggVorbis` has a loop offset (`modules/vorbis/audio_stream_ogg_vorbis.h:148`) but no loop
  end, and no importer reads `LOOPSTART`/`LOOPLENGTH` comments (no match for either string in `modules/vorbis` or
  `editor/import`) ⇒ Talewright audio importer step (RM-AUD-011, epic 29).
- Asset encryption: PCK encryption requires templates built with `SCRIPT_AES256_ENCRYPTION_KEY` (`core/SCsub:190`).
- Tile editor sub-plugins hard-coded (`editor/scene/2d/tiles/tile_map_layer_editor.cpp:4532-4533`).

## 7. Resources
- `Resource` (`core/io/resource.h:52`); `ResourceSaver::save` (`core/io/resource_saver.h:86`),
  `ResourceLoader::load` (`core/io/resource_loader.h:259`); text format v4 (`scene/resources/resource_format_text.h:46`);
  binary v6 (`core/io/resource_format_binary.cpp:99`); `ResourceUID` (`core/io/resource_uid.h:41`).
- Sub-resource ids: the text saver keeps existing ids and generates missing ones
  (`scene/resources/resource_format_text.cpp:1896-1935`); generated ids are **random**, from a generator seeded by
  the clock unless seeded explicitly (`core/io/resource.cpp:129-150`). Talewright therefore sets ids explicitly
  (`Resource::set_scene_unique_id`, e.g. `Item_42`) to keep database diffs stable (D-005).

## 8. Debugger, remote scene tree, logs
- Game ↔ editor protocol: `[name, thread_id, data]` arrays (`core/debugger/remote_debugger.cpp:105`), captures by
  prefix (`core/debugger/engine_debugger.h:118`), 8 MiB peer buffers (`core/debugger/remote_debugger_peer.cpp:84-85`).
- Remote tree: `scene:request_scene_tree` (`scene/debugger/scene_debugger.cpp:163-166`) → reply `scene:scene_tree`
  (`scene/debugger/scene_debugger.cpp:847`); editor handlers `editor/debugger/script_editor_debugger.cpp:1007,1018,1022`;
  object inspection `scene:inspect_objects` (`scene/debugger/scene_debugger.cpp:177-185`).
- Game screenshot round trip `scene:rq_screenshot` → `game_view:get_screenshot` (`scene/debugger/scene_debugger.cpp:529-567`).
- Game output reaches the editor as `output`/`error` messages handled by `ScriptEditorDebugger`
  (`editor/debugger/script_editor_debugger.cpp:561-593`), signals `output`/`debug_data` (`:2071`, `:2075`);
  `EditorDebuggerPlugin` (`editor/debugger/editor_debugger_plugin.h:72-94`).
- Editor log `EditorLog::add_message` (`editor/editor_log.h:182`); print/error handler APIs
  (`core/string/print_string.h:40-52`, `core/error/error_macros.h:61-71`).

## 9. Headless
- `--headless` ⇒ headless display + dummy audio (`main/main.cpp:1505-1508`); the headless display server offers only
  the dummy rendering driver, and `--doctool` implies headless (`main/main.cpp:1745-1748`); `dedicated_server` feature tag forces it
  (`main/main.cpp:2290-2293`). The dummy renderer returns no viewport texture
  (`servers/rendering/dummy/storage/texture_storage.h:229`) ⇒ screenshots need a real display.
- Main loop type resolution incl. custom `MainLoop` classes (`main/main.cpp:4360-4436`).

## 10. Existing tests
- 168 test `.cpp` files under `tests/` with `TEST_FORCE_LINK` (`tests/test_macros.h:53-56`); scaffolding
  `tests/create_test.py:14-33`. Baseline (task 1.3): 1380 passed, 0 failed, 3 skipped,
  419763 assertions (`plan/research/baseline_tests.md`).

## 11. AI agents / MCP servers for the Godot editor (prior art) and decision
Sources fetched 2026-10-13 (details and more projects in `plan/research/planning_notes/03_godot_debugger_bridge_mcp.md`):
- https://github.com/Coding-Solo/godot-mcp — Node/TS stdio; drives Godot CLI; no live tree/screenshots; MIT.
- https://github.com/ee0pdt/Godot-MCP — FastMCP ↔ WebSocket ↔ GDScript addon; MIT; stale (2025-03).
- https://github.com/yurineko73/Godot-MCP-Native — GDScript MCP server inside the editor (Streamable HTTP, optional
  Bearer token), game probe via EngineDebugger capture; MIT.
- https://github.com/beckettlab/beckett-godot-mcp — GDScript editor plugin, HTTP/JSON-RPC polled on main thread,
  localhost + token + Host/Origin checks; MIT Lite + paid Full.
- https://github.com/satelliteoflove/godot-mcp — Node stdio ↔ WebSocket ↔ addon ↔ game via debugger protocol; MIT.
- https://gdaimcp.com/docs/installation — commercial; editor + game screenshots.
- https://github.com/hi-godot/godot-ai — stdio attach → Python server → authenticated WebSocket to an editor plugin; MIT.
- MCP spec: https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http and
  2025-06-18; Claude Code HTTP servers with headers: https://code.claude.com/docs/en/mcp
- **Decision (D-012: Streamable HTTP with JSON responses, 127.0.0.1 only, bearer token, Origin/Host checks,
  editor-only module):** build natively as an editor-only C++ module; none of the prior art is an engine module, and
  native access to `EditorNode`, debugger sessions and editor redraw control is needed for robustness.
