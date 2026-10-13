# Spec 01 — `modules/jrpg` layout and boundaries

Decisions: D-004 (where it lives), D-007 (events), D-010 (RNG), D-018/D-019 (movement, battle).

## Directory layout
```
modules/jrpg/
  config.py            can_build: always (runtime ships in templates); get_doc_classes lists public classes
  SCsub                core/*.cpp, runtime/*.cpp always; editor/*.cpp only when env.editor_build; tests via tests/*.h
  register_types.*     SCENE level: core + runtime classes; EDITOR level (#ifdef TOOLS_ENABLED): editor plugins
  core/                pure logic, NO include from scene/ or editor/ (enforced by tests/check_core_includes.py)
  runtime/             nodes and UI building blocks used by games (scene/ allowed)
  editor/              workspace, database editor, map canvas, event editor, importers, setup tool
  addon_template/      GDScript + .tscn managed addon sources, embedded into the binary (template_builders pattern)
  doc_classes/         class reference XML for every exposed class (generated with --doctool, then edited)
  tests/               doctest headers, tag [JRPG]
```

## Layering rules
- `core/` may include `core/` of the engine (Variant, Resource, RefCounted, math) and `modules/jrpg/core/` only.
  It never touches Nodes, SceneTree, rendering, input or editor code. This is what MMO zone servers reuse (D-015).
- **Assets in core are `uid://` strings.** Faces, character sheets, battlers, pictures, audio and a tileset's
  `TileSet` are referenced by their ResourceUID text (`uid://…`, falling back to a `res://` path for files without a
  uid). Core never holds `Texture2D` (`scene/resources/texture.h`), `AudioStream` (`servers/audio/audio_stream.h`) or
  `TileSet` (`scene/resources/2d/tile_set.h`); the runtime resolves the strings with `ResourceLoader`.
- **Everything the simulation needs is core data:** map layers, regions and events (`JRPGMapData`, spec 05), the
  baked tile-flag table (`JRPGTilesetData.flags`, spec 05) and runtime event state (`JRPGEventState`, spec 06). Scene
  nodes only present that data.
- `runtime/` may include `scene/` and `core/`; never `editor/`.
- `editor/` may include everything; compiled only into editor builds.
- Online authority (D-007): every runtime feature states whether it runs on the server, the client or both; the
  core decides, the runtime presents.

## Public class families (initial)
| Family | Location | Notes |
|---|---|---|
| `JRPGDatabase`, `JRPGTable`, `JRPGEntry` + typed entries (`JRPGActorData`, `JRPGClassData`, `JRPGSkillData`, `JRPGItemData`, `JRPGWeaponData`, `JRPGArmorData`, `JRPGEnemyData`, `JRPGTroopData`, `JRPGStateData`, `JRPGAnimationData`, `JRPGTilesetData`, `JRPGCommonEventData`, `JRPGSystemData`, `JRPGTypesData`, `JRPGTermsData`) | core | Resources (spec 02) |
| `JRPGRandom` | core | own PCG32 integer RNG (spec 04) |
| `JRPGFormula` | core | Expression wrapper with proxies (spec 04) |
| `JRPGGameState` (switches, variables, self switches, party, actors, inventory, gold, timers, system flags) | core | serializable (spec 06) |
| `JRPGBattler`, `JRPGActorState`, `JRPGEnemyState`, `JRPGBattleModel` | core | battle math (spec 04) |
| `JRPGEventCommand` + typed commands, `JRPGCommandList`, `JRPGInterpreter`, `JRPGPresentationPort` | core | spec 03 |
| `JRPGMapData`, `JRPGEventData`, `JRPGEventPage`, `JRPGMapList` | core | map and event data, spec 03/05 |
| `JRPGEventState`, `JRPGSimClock` | core | runtime event state (spec 06), fixed 60 Hz step counter (spec 03) |
| `JRPGMapCellCache`, `JRPGPathfinder`, `JRPGMover` | core | spec 05 |
| `JRPGMap`, `JRPGEvent` (internal, built from data), `JRPGEventAnchor`, `JRPGCharacterSprite`, `JRPGPlayer`, `JRPGSceneManager`, `JRPGLocalPort` | runtime | nodes presenting core data |
| `JRPGWindow`, `JRPGSelectableList`, `JRPGMessageView`, `JRPGGauge` | runtime | UI building blocks |
| `JRPGWorkspacePlugin`, `JRPGBootstrapPlugin`, `JRPGDatabaseEditor`, `JRPGMapCanvas`, `JRPGEventEditor`, `JRPGSetupTool`, importers | editor | editor-only |

## Naming and conventions
- Classes `JRPG*`; files `snake_case`; tests `TEST_CASE("[JRPG][Area] ...")`.
- Every public class registered with `GDREGISTER_CLASS` has class docs in `doc_classes/` (CI-style doctool check:
  `--doctool` must produce no diff).
- Project setting: only `jrpg/enabled` (bool, default false) is a `GLOBAL_DEF` (D-008); everything else lives in
  `JRPGSystemData`.
