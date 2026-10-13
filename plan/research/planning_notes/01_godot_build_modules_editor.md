# Godot 4.7.2: build system, modules, editor plugin API, GDExtension, project manager, branding, tests

_Planning-phase research report (2026-10-13), produced by a read-only research subagent and kept verbatim as evidence. Godot citations are at tag 4.7.2-stable; they are re-verified against the clone in task 2.2/2.3._

---

# Godot 4.7.2-stable research report (commit ed1daf0, verified via `gh api` and raw.githubusercontent)

All paths are repo-relative at tag `4.7.2-stable`. Nothing was cloned or written.

## 1. Build system (Windows/MSVC)

**Version minimums**
- `EnsureSConsVersion(4, 4)` and `EnsurePythonVersion(3, 9)` are at `SConstruct:4-5`.
- Visual Studio 2022 needs SCons 4.8.0+ (`platform/windows/detect.py:187-188`). VS2026 ("14.5") needs SCons 4.10.1+ (`detect.py:190-191`).
- CI pins `scons==4.10.1` with Python `3.x` (`.github/actions/godot-deps/action.yml:7,13,30`).
- MSVC must be VS2019 16.11 or newer; VS2017 is rejected (`SConstruct:780-791`). The C++ standard is `/std:c++17` (`SConstruct:921`).
- README has no build steps. It links to the online docs (`README.md:42-45`).

**Windows CI command** (`.github/workflows/windows_builds.yml`)
- Global flags are `dev_mode=yes module_text_server_fb_enabled=yes debug_symbols=no` (`:8-11`).
- The MSVC editor job adds `target=editor windows_subsystem=console vsproj=yes vsproj_gen_only=no` (`:25-33`).
- The compilation step appends `d3d12=… accesskit=… angle=…` (`:115`).
- The `godot-build` action runs `scons platform=windows target=<t> <flags> "cache_path=…" redirect_build_objects=no` (`.github/actions/godot-build/action.yml:41`).
- The effective editor command is therefore:
  `scons platform=windows target=editor dev_mode=yes module_text_server_fb_enabled=yes debug_symbols=no windows_subsystem=console vsproj=yes vsproj_gen_only=no d3d12=yes accesskit=yes angle=yes`
- Dependencies are fetched by `misc/scripts/install_d3d12_sdk_windows.py`, `install_angle.py`, `install_winrt.py` and `install_accesskit.py` (`windows_builds.yml:77-105`). They default to `%LOCALAPPDATA%\Godot\build_deps` (`detect.py:207-209`).

**Tests in CI**
- There is no explicit `tests=yes`. `dev_mode` is an alias for `verbose=yes warnings=extra werror=yes tests=yes strict_checks=yes` (`SConstruct:237`, applied at `:668-673`). The plain option is `tests` (`:240`).
- With tests on, `tests/SCsub` is built (`SConstruct:1238-1239`) and `TESTS_ENABLED` is defined (`main/SCsub:17-18`).
- Windows runs `bin/godot.windows.editor.x86_64.exe --test --force-colors` without `--headless` (`windows_builds.yml:137-141`). Linux runs `--headless --test --force-colors` (`linux_builds.yml:237-241`).
- `--test` is intercepted in `Main::test_entrypoint` (`main/main.cpp:982-997`), which calls `test_main`. The doctest entry is `tests/test_main.cpp:75-147`: it strips `--test`, then calls `doctest::Context::applyCommandLine` (`:138`) and `run()` (`:146`).

**Dev build options**
- `target` accepts `editor|template_release|template_debug` (`SConstruct:162-166`). `editor` defines `TOOLS_ENABLED` (`:548-549`). `editor` and `template_debug` define `DEBUG_ENABLED` (`:535,551-554`).
- `dev_build` (`:168`) defines `DEV_ENABLED` (`:556-559`), sets optimize to `none` (`:538-539`), defaults `debug_symbols` to on (`:546`), and adds a `.dev` suffix to the binary name (`:1040-1041`).
- `debug_symbols` is at `:178` and `separate_debug_symbols` at `:179`.
- `compiledb` and `compiledb_gen_only` are at `:245-246`, wired at `:1195-1199` and `:1252-1256`.
- `vsproj` and `vsproj_name` are at `:260-261` and call `methods.generate_vs_project` (`:1245-1248`). `vsproj_gen_only` defaults to true and exits after generating (`methods.py:1501-1502`). `methods.py:1002` documents `scons vsproj=yes vsproj_gen_only=no` to generate and build.
- Other useful options: `ninja` (`:242`), `scu_build` (`:296`), `fast_unsafe` (`:241`), `production` (`:186,674-680`), `custom_modules` (`:280`), `build_profile` (`:279`).
- On Windows: `use_mingw`, `use_llvm`, `windows_subsystem` (default `gui`), `msvc_version`, `debug_crt` (`detect.py:224-233`).

**Template builds**
- CI matrix has `template_release` with MSVC (`windows_builds.yml:44-48`, output `godot.windows.template_release.x86_64.console.exe`) and with `use_mingw=yes` (`:50-56`).
- `template_debug` is only built on Linux (`linux_builds.yml:97-101`).
- For any non-editor target, the build action runs `rm -rf editor` first (`godot-build/action.yml:29-32`).

## 2. Module architecture

**Detection and options**
- A directory counts as a module only if it has `register_types.h`, `SCsub` and `config.py` (`methods.py:302-309`).
- Each module gets an option `module_<name>_enabled` (`SConstruct:485`). Its default is `config.is_enabled()` if that exists, otherwise True. If `modules_enabled_by_default=no`, every module defaults to False (`SConstruct:282,476-483`).
- Optional `config.get_opts(platform)` adds extra options (`:488-491`).
- Examples that return False from `is_enabled()`: `modules/text_server_fb/config.py:10-12` and `modules/mono/config.py:31-33`.

**Configuration flow**
- For each enabled module: `can_build(env, platform)`, then the dependency check, then `configure(env)`. Next `get_doc_classes()`/`get_doc_path()` fill `env.doc_class_path`, and `get_icons_path()` sets icons (default `icons/`) (`SConstruct:1112-1140`).
- Dependencies are declared with `env.module_add_dependencies` (`methods.py:324-334`). A module with unmet dependencies is disabled with a warning (`methods.py:337-361`). Example: `modules/gdscript/config.py:2`.
- Out-of-tree modules come from `custom_modules=path1,path2` (`SConstruct:446-469`).

**Generated code**
- `modules/SCsub` defines `GODOT_MODULE` (`:13`) and generates `modules_enabled.gen.h` with `MODULE_<NAME>_ENABLED` (`:19-21`, `modules/modules_builders.py:8-12`).
- It also generates `register_module_types.gen.cpp`, which calls `initialize_<name>_module(p_level)` for each module (`modules_builders.py:15-51`).
- Each module is built as `libmodule_<name>` (`modules/SCsub:33-42`).
- Init levels are `MODULE_INITIALIZATION_LEVEL_CORE/SERVERS/SCENE/EDITOR`, mapped to the GDExtension enum (`modules/register_module_types.h:35-40`).
- The EDITOR level runs after `register_editor_types()` inside `#ifdef TOOLS_ENABLED` (`main/main.cpp:3818-3826`).

**Example: gridmap**
- `config.py:1-14` has `can_build` returning `not disable_3d`, and lists doc classes `GridMap` and `GridMapEditorPlugin`.
- `SCsub:11-12` adds `editor/*.cpp` only when `env.editor_build`.
- `register_types.cpp:43-55`: `GridMap` is registered at SCENE level. At EDITOR level, inside `#ifdef TOOLS_ENABLED`, it does `GDREGISTER_VIRTUAL_CLASS(GridMapEditorPlugin); EditorPlugins::add_by_type<GridMapEditorPlugin>();`.

**Example: multiplayer**
- `register_types.cpp:58-62` calls `EditorPlugins::add_by_type<MultiplayerEditorPlugin>()` at EDITOR level.
- `SCsub:12-13` adds editor sources and `:17-18` adds `tests/*.cpp`.

**Example: gdscript**
- It uses `EditorNode::add_init_callback(_editor_init)` at SERVERS level (`modules/gdscript/register_types.cpp:157-158`).
- `_editor_init` registers an export plugin and a syntax highlighter (`:123-131`).
- At EDITOR level it calls `EditorPlugins::add_by_type<GDScriptLanguageServer>()` (`:162-167`).
- `EditorPlugins` is a static table of 128 create functions (`editor/plugins/editor_plugin.h:282-311`). It is instantiated in `editor/editor_node.cpp:9420-9421`.

**Module docs**
- The build does not fail on missing docs. `editor/SCsub:30-50` simply globs whatever `doc/classes/*.xml` and `<module>/<doc_path>/*.xml` files exist into compressed doc data.
- `--doctool` writes each class's XML to its module's `doc_classes` path if the class is listed in `get_doc_classes`, otherwise to `doc/classes` (`main/main.cpp:4220-4272`, `editor/doc/doc_tools.cpp:1687-1699`).
- Enforcement is in Linux CI. The "Check for class reference updates" step runs `--doctool` and fails on any `git diff` or any new untracked `.xml` file (`linux_builds.yml:249-255`).
- The pre-commit `make_rst.py --dry-run` (`.pre-commit-config.yaml:83-88`) exits 1 on XML errors or warnings (`doc/tools/make_rst.py:892-897`). `doc_status.py` also runs there (`:90-96`). Whether `doc_status.py` fails on incomplete docs is not verified.

## 3. Editor plugin API (`editor/plugins/editor_plugin.h` / `.cpp`)

**Virtuals**

| Method | Header | Impl |
|---|---|---|
| `get_plugin_name` | `:196` | `.cpp:339` |
| `get_plugin_icon` | `:197` | `.cpp:345` |
| `has_main_screen` | `:200` | `.cpp:359` |
| `make_visible` | `:201` | `.cpp:365` |
| `edit` | `:204` | `.cpp:369` |
| `handles` | `:205` | `.cpp:373` |

Their script-side GDVIRTUAL counterparts are at `:128-133`.

**Docks**
- There is a new **`EditorDock`** class (`editor/docks/editor_dock.h:40`, a `MarginContainer`). It has the `DockSlot` enum including `BOTTOM_L/BOTTOM_R` (`:51-65`), `DockLayout` (`:44-49`), and `set_default_slot` / `set_available_layouts`.
- It is ClassDB-registered (`editor/register_editor_types.cpp:160`) and documented in `doc/classes/EditorDock.xml`.
- `add_dock` / `remove_dock` are at `editor_plugin.h:167-168`, `.cpp:132-137`, and go through `EditorDockManager` (`editor/docks/editor_dock_manager.h:156-157`).
- These are deprecated and wrapped in `#ifndef DISABLE_DEPRECATED` (`.h:148-159`):
  - `add_control_to_dock` (`.cpp:93-106`) wraps the control in an `EditorDock`.
  - `add_control_to_bottom_panel` (`.cpp:88-91`) calls `EditorNode::get_bottom_panel()->add_item`. Its docs say to use `add_dock` with `DOCK_SLOT_BOTTOM` instead (`doc/classes/EditorPlugin.xml:427,448`).
- Gridmap shows the modern pattern: `GridMapEditor : public EditorDock` (`grid_map_editor_plugin.h:51`), `set_default_slot(DOCK_SLOT_BOTTOM)` (`.cpp:1495-1496`), and `EditorDockManager::get_singleton()->add_dock` (`.cpp:1969`). `ReplicationEditor : public EditorDock` follows the same pattern (`modules/multiplayer/editor/replication_editor.h:47`).

**Other registration methods**

| Method | Header | Impl |
|---|---|---|
| `add_tool_menu_item` | `.h:170` | `.cpp:240` |
| `add_export_plugin` | `.h:240` | `.cpp:463` |
| `add_inspector_plugin` | `.h:249` | `.cpp:493` |
| `add_debugger_plugin` | `.h:261` | `.cpp:592` |
| `add_import_plugin` | `.h:237` | — |
| `add_control_to_container` | `.h:164` | — |
| `add_context_menu_plugin` | `.h:267` | — |

**Main screen**
- `EditorMainScreen` (`editor/editor_main_screen.h:41`) has `EditorTable { EDITOR_2D, EDITOR_3D, EDITOR_SCRIPT, EDITOR_GAME, EDITOR_ASSETLIB }` (`:45-51`).
- Buttons are created in `add_main_plugin` (`editor_main_screen.cpp:262-290`). It uses the plugin name as text and looks up the shortcut `editor/editor_<name>`.
- `EditorNode::add_editor_plugin` routes any plugin with `has_main_screen()` there (`editor/editor_node.cpp:4365-4368`). The buttons sit in the title bar's `EditorMainScreenButtons` (`:9075-9080`).
- Built-in main-screen plugins:
  - 2D: `editor/scene/canvas_item_editor_plugin.h:645-646`
  - 3D: `editor/scene/3d/node_3d_editor_plugin.h:1065-1066`
  - Script: `editor/script/script_editor_plugin.h:485-486`
  - Game: `editor/run/game_view_plugin.h:315-316`
  - "Asset Store": `editor/asset_library/asset_library_editor_plugin.h:415-417`. It is no longer named "AssetLib".
- They are added in that order at `editor_node.cpp:9391-9406`.

## 4. GDExtension

**Where it lives**
- `core/extension/` contains `gdextension.{h,cpp}`, `gdextension_interface.cpp`, `gdextension_interface.json`, `extension_api_dump.cpp`, `gdextension_manager.*`, `godot_instance.*` and `libgodot.h`.
- The C header `gdextension_interface.gen.h` is generated from the JSON (`core/extension/SCsub:17-18`). Modules include it (`modules/register_module_types.h:33`).
- CLI dumps: `--dump-gdextension-interface[-json]`, `--dump-extension-api[-with-docs]` and `--validate-extension-api` (`main/main.cpp:723-727`).

**Trade-off: what GDExtension can see**
- `extension_api.json` only includes ClassDB classes where `is_class_exposed` is true, each tagged with an `api_type` of core/editor/… (`extension_api_dump.cpp:917-941`). `GDREGISTER_INTERNAL_CLASS` sets `exposed=false` (`core/object/class_db.h:273-280,580-583`).
- `EditorNode` and `EditorMainScreen` are never ClassDB-registered. `register_editor_types.cpp` only references `EditorNode::` static members (`:302,342`).
- Also C++-only: `EditorPlugins::add_by_type` and `EditorNode::add_init_callback` (`editor_node.h:785`).
- GDExtension instead gets `editor_add_plugin`, `editor_remove_plugin` and `editor_help_load_xml_*` (`gdextension_interface.cpp:1682-1705,1883-1886`), handled through `GDExtensionEditorPlugins` (`gdextension.h:185-227`, consumed at `editor_node.cpp:9424-9428`).
- The exposed surface is `EditorInterface` (e.g. `get_editor_main_screen`, `set_main_screen_editor`, `editor/editor_interface.h:122,127`) plus the classes registered at `register_editor_types.cpp:155-206`.

## 5. Project manager, templates, feature profiles

**New project creation** (`editor/project_manager/project_dialog.cpp:561-634`)
- It builds `initial_settings`: the renderer (`rendering/renderer/rendering_method`, plus a `.mobile` override for `gl_compatibility`), the features list, `application/config/name`, and `application/config/icon = res://icon.svg` (`:576-597`).
- It merges `EditorNode::get_initial_settings()`: stretch `canvas_items`/`expand`, Jolt Physics, and `d3d12` on Windows (`editor_node.cpp:8346-8352`).
- It writes the file with `ProjectSettings::save_custom(path/project.godot, …)` (`:604`).
- It writes `icon.svg` from the editor icon `DefaultProjectIcon` (`:611-616`, `editor/themes/editor_icons.cpp:219-227`), VCS metadata (`:618`) and `.editorconfig` (`:621-633`).
- Dialog modes are `NEW, IMPORT, INSTALL, RENAME, DUPLICATE` (`project_dialog.h:47-52`).

**Templates**
- There is no local project-template folder: no `project_templates` path exists in the tree, and `EditorPaths` only has `export_templates`, `text_editor_themes`, `script_templates` and `feature_profiles` (`editor/file_system/editor_paths.h:47-50`).
- The template-like route is installing a ZIP: from the Asset Store tab (`project_manager.cpp:1739-1742`, via `_install_project` and `MODE_INSTALL` at `:750-752`) or by dropping a `.zip` (`:1283-1287`). It extracts from the first folder containing `project.godot` (`project_dialog.cpp:646-720`).
- `editor/template_builders.py` handles script templates only.

**EditorFeatureProfile** (`editor/settings/editor_feature_profile.h:42-104`)
- Features you can disable: `3D, SCRIPT, ASSET_LIB, SCENE_TREE, FILESYSTEM_DOCK, IMPORT_DOCK, HISTORY_DOCK, GAME, SIGNALS_DOCK, GROUPS_DOCK`. There is no 2D flag.
- It can also disable classes, class editors and individual properties.
- Profiles are saved as JSON `<config>/feature_profiles/<name>.profile` (`.cpp:191-223,499`). The active one is stored globally in the EditorSettings key `_default_feature_profile` (`.cpp:348,920`), not per project.
- `EditorNode::_feature_profile_changed` toggles docks and main-screen buttons (`editor_node.cpp:7850-7881`).
- `CreateDialog`, the inspector, the FileSystem dock and the SceneTree dock filter by it (`create_dialog.cpp:174-177`, `editor_inspector.cpp:4114-4126`).
- It is also exposed to scripts (`register_editor_types.cpp:193`).

**JRPG mode via profiles**
- A profile can hide 3D, Script and the Asset Store and prune node types. It cannot add UI and cannot hide 2D.
- Adding JRPG-specific UI would need a module or plugin. Making it per project would need custom code, because the active profile is a global editor setting.

## 6. Version and branding

**`version.py`**
- `short_name="godot"`, `name="Godot Engine"`, 4/7/2, `status="stable"`, `module_config=""`, the website URL and `docs="4.7"` (`version.py:1-9`).
- It is read by `methods.get_version_info` (`methods.py:136-165`). The `BUILD_NAME` and `GODOT_VERSION_STATUS` environment variables override parts of it.
- It becomes `GODOT_VERSION_*` defines in `version_generated.gen.h` (`core/core_builders.py:22-32`, `core/SCsub:176-180`).
- The legacy `VERSION_*` aliases exist only without `DISABLE_DEPRECATED` (`core/version.h:88-108`).
- Module detection treats any directory whose `version.py` contains `short_name = "godot"` as an engine tree and skips it (`methods.py:262-270`).

**Data directory**
- `OS::get_godot_dir_name()` returns `GODOT_VERSION_SHORT_NAME` lowercased (`core/os/os.cpp:294-297`). Windows overrides it with `.capitalize()`, giving `%APPDATA%\Godot` (`platform/windows/os_windows.cpp:2466-2468`).
- `EditorPaths` uses it for the data, config and cache dirs, and switches to self-contained mode when a `_sc_` or `._sc_` file is present (`editor_paths.cpp:134-184`).
- Game user data lives under `<dir>/app_userdata/<name>` (`os.cpp:349-352`).
- The editor settings file is named `editor_settings-<maj>.<min>.tres` (`editor_settings.cpp:1369`).

**Window titles**
- Editor: `"<scene> - <project> - " + GODOT_VERSION_NAME` (`editor_node.cpp:368-380`).
- Project manager: `GODOT_VERSION_NAME + " - Project Manager"` (`project_manager.cpp:119`).

**Icons, splash and logos**
- `platform/windows/godot.ico` (resource at `godot_res.rc:7`) and `godot_console.ico` (`godot_res_wrap.rc:7`). The `VERSIONINFO` block hardcodes `CompanyName "Godot Engine"` and uses `GODOT_VERSION_NAME` (`godot_res.rc:10-27`).
- `main/app_icon.png` and `main/splash.png` are embedded via `main/SCsub:21-36` and used at `main/main.cpp:3980-3995`.
- `main/splash_editor.png` does not exist and `no_editor_splash` defaults to True, so `NO_EDITOR_SPLASH` is forced (`SConstruct:283,587-591`).
- There is no root `icon.svg` or `logo.svg`. Logos are in `misc/logo/{icon,logo}*.svg/png` and `editor/icons/{Godot,Logo,DefaultProjectIcon}.svg`.
- The binary base name is hardcoded as `#bin/godot` (`platform/windows/SCsub:119`).

## 7. Tests

**Layout**
- `tests/core/**`, `tests/scene/*` and `tests/servers/**` hold 168 `.cpp` files. There are no `.h` test files there any more.
- Each file declares `TEST_FORCE_LINK(name)` (example: `tests/scene/test_node.cpp:33`; macro at `tests/test_macros.h:53-56`).
- Files are collected by `tests/SCsub:11-22` into a generated `force_link.gen.h` (`tests/test_builders.py`).
- New tests are scaffolded with `python tests/create_test.py <ClassName> [subdir]`, which creates `test_<name>.cpp` (`tests/create_test.py:14-33`).

**Module tests**
- When tests are enabled, `modules/SCsub:44-55` globs `<module>/tests/*.h` into `modules/modules_tests.gen.h`. That file is included by `tests/test_main.cpp:73`.
- Module `tests/*.cpp` files are added by each module's own SCsub when `env["tests"]` is set (`modules/zip/SCsub:12-13`, `modules/jsonrpc/SCsub:10-11`, `modules/multiplayer/SCsub:17-18`).
- Modules with tests: csg, dds, gdscript, gltf, gridmap, jsonrpc, mbedtls, multiplayer, noise, regex, visual_shader, zip.

**Tags**
- There is no `[Modules]` tag convention in the files I sampled. Tags are class names such as `TEST_CASE("[RegEx] …")` (`modules/regex/tests/test_regex.h:40`).
- Special tags: `[SceneTree]` creates a mock DisplayServer, physics/navigation servers and a `SceneTree`. `[Editor]` additionally creates `EditorPaths` and `EditorSettings`. `[Audio]` sets up a dummy audio driver. These are handled at `tests/test_main.cpp:176-248`. Example: `modules/gridmap/tests/test_grid_map.h:42` uses `"[SceneTree][GridMap][MeshLibrary]"`.
- Custom commands use `REGISTER_TEST_COMMAND` (`test_macros.h:148`), e.g. `godot --test gdscript-parser` (`modules/gdscript/register_types.cpp:231-235`).

**Running a subset**
- `godot.windows.editor.x86_64.exe --test --test-case="*GridMap*"` (short form `-tc=`). Also available: `--test-suite=`, `--source-file=`, `--test-case-exclude=`, `--no-skip` and `--list-test-cases`.
- Doctest parses these at `thirdparty/doctest/doctest.h:6645-6656,6716,6750`. The unprefixed forms work because `DOCTEST_CONFIG_NO_UNPREFIXED_OPTIONS` is not defined (`:6520-6525`).
- `--test --help` is listed in the main help (`main/main.cpp:733`).

**Not verified:** whether `doc_status.py` exits non-zero on incomplete docs, and whether any `[Modules]` tag is used anywhere (only a subset of module tests was sampled).
