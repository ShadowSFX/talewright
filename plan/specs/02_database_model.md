# Spec 02 — JRPG database model

Decision: D-005. Inventory: RM-DB-*, ACT, CLS, SKL, ITM, WPN, ARM, ENM, TRP, STA, ANI, TIL, CEV, SYS, TYP, TRM, TRT, EFF.

## Files
```
res://jrpg/database.tres            JRPGDatabase (manifest): schema_version + one ExtResource per table + system/types/terms
res://jrpg/db/actors.tres           JRPGTable (entry_class = JRPGActorData), entries as sub-resources
res://jrpg/db/classes.tres ... skills, items, weapons, armors, enemies, troops, states, animations, tilesets, common_events
res://jrpg/db/system.tres           JRPGSystemData (single resource)
res://jrpg/db/types.tres            JRPGTypesData (elements, skill/weapon/armor/equip types)
res://jrpg/db/terms.tres            JRPGTermsData
```
Text `.tres` keeps diffs readable; exports convert to binary automatically.

## Starter database (setup tool, task 6.2)
`JRPGProjectSetup` (Project ▸ Tools ▸ Set Up JRPG Project…) writes the files above when missing, with Talewright's
own starter content: actors (4 slots; 1 "Hero", class 1, starting with weapon 1 and armor 1), classes (4; 1
"Adventurer": baked EXP table, linear parameter curves, can equip swords, clothing, light armor and shields, attacks
with element 1, 95 % hit / 5 % evasion), skills (8; 1 "Attack" `a.atk * 3 - b.def * 2`, 2 "Guard" adding state 2),
items (8; 1 "Herb Tonic" restoring 300 HP), weapons (8; 1 "Short Sword", attack +10), armors (8; 1 "Leather Vest",
body, defense +5), enemies (8; 1 "Moss Blob" using skill 1, dropping item 1 at 1 in 4), troops (8; 1 with two Moss
Blobs), states (8; 1 "Knocked Out", 2 "Guarding" with the guard flag for one turn), animations (8), tilesets (4),
common events (8), System (game title from the project name, a new game id, starting party [1], no start map until
maps exist), default types and terms. Every reference resolves (`JRPGReferenceScanner.find_dangling` is empty).
Existing files are never overwritten; a table missing from an existing manifest is added. Theme and placeholder
assets join when the asset generator exists (task 7.1).

## IDs
- **ID = slot** (RPG Maker model): `entries[0]` is ID 1. IDs never move: no reordering, "delete" clears an entry to
  its defaults, "Change Maximum" grows/shrinks the array (shrinking warns when the reference scanner finds uses).
- ID 0 means "none" in every reference.
- Each entry sub-resource gets the explicit scene-unique id `<Kind>_<id>` (e.g. `Item_42`) through
  `Resource::set_scene_unique_id` before saving, so `.tres` diffs are stable (Godot would otherwise generate random
  ids, `core/io/resource.cpp:129-150`).
- References between entries store the integer ID. Saves store IDs, never resource paths.

## Entry base: `JRPGEntry` (Resource)
| Property | Type | Notes |
|---|---|---|
| `id` | int | read-only, = slot index + 1 (set by the table) |
| `name` | String | source-language text; translated through the stable key `<Kind>_<id>.name` (D-026) |
| `note` | String | free text; `<tag:value>` metadata parsed on demand (plugin parity) |
| `icon_index` | int | where applicable |

## Packed nested lists (values, not sub-resources)
Traits, effects, learnings, enemy actions, drops, troop members, parameter curves are stored as packed arrays of
small dictionaries or packed ints so tables stay compact:
- **Trait**: `{code:int, data_id:int, value:int}` (rates in basis points, 10 000 = 100 %; flat amounts as integers; spec 04) — codes enumerate RM-TRT-002..026 (element rate, debuff rate,
  state rate, state resist, param, xparam, sparam, attack element, attack state, attack speed, attack times, attack
  skill, add/seal skill type, add/seal skill, equip weapon/armor type, lock/seal equip, slot type, action times,
  special flag, collapse effect, party ability).
- **Effect**: `{code:int, data_id:int, value1:int, value2:int}` (percentages in basis points) — codes for RM-EFF-002..014 (recover HP/MP, gain
  TP, add/remove state, add/remove buff/debuff, special effect, grow, learn skill, common event).
- **Learning**: `{level:int, skill_id:int, note:String}`; **Drop**: `{kind:int, data_id:int, denominator:int}`;
  **Enemy action**: `{skill_id:int, condition_type:int, param1:int, param2:int, rating:int}` (HP/MP conditions in basis points);
  **Troop member**: `{enemy_id:int, x:int, y:int, hidden:bool}`.
- Parameter curves: `PackedInt32Array` of 8 params × (max_level+1) values.
Full field lists per table follow the inventory slices (03/04/05). The vertical slice (M1) implements the
**slice field set** below first (tasks 5.3, 6.5); the rest arrives with the parity epics.

## Slice field set (M1)
| Table | Fields |
|---|---|
| Actors | name, nickname, class, initial level, max level, face (`uid://`), character sheet (`uid://`), battler image, initial equipment, traits, note |
| Classes | name, EXP curve (base, growth, extra → baked table), parameter curves, learnings, traits |
| Skills | name, icon, description, skill type, MP cost, TP cost, scope, occasion, speed, success, repeats, TP gain, hit type, animation, use messages, damage (type, element, formula, variance, critical), effects |
| Items | name, icon, description, item type, price, consumable, scope, occasion, speed, success, repeats, TP gain, hit type, animation, damage, effects |
| Weapons / Armors | name, icon, description, weapon or armor type, equipment type, price, parameter bonuses, traits |
| Enemies | name, battler image, parameters, EXP, gold, drops, action patterns, traits |
| Troops | name, members (enemy, position, hidden), battle event pages |
| States | name, icon, restriction, priority, removal (battle end, restriction, turns min/max, damage chance, steps), messages, traits |
| System | game title, `game_id`, currency unit, initial party, **start position** (map, cell, direction; required, RM-SYS-007), title image, title/battle/victory/defeat music, UI sound set (cursor, ok, cancel, buzzer), window color, battle rules (spec 04) |
| Types | elements, skill types, weapon types, armor types, equipment types |
| Terms | basic statuses, parameters, menu and battle commands, battle messages |
| Tilesets | spec 05 (palette, baked flags, `tileset_uid`) |
| Common events | name, trigger, condition switch, commands |

## Versioning and migration
- `JRPGDatabase.schema_version` and `JRPGTable.schema_version` (int). The class default is -1 ("unstamped", never a
  real version) and any content change stamps the current version, so every saved manifest and table records its
  version (Godot's text saver skips values equal to the class default, `scene/resources/resource_format_text.cpp:1986-1990`).
  The loader runs the `JRPGSchema` migration steps from the saved version; renamed properties are handled with
  `_set()` fallbacks (old name → new name, `JRPGSchema::set_renamed`) on every persisted JRPG resource so old files load.
- Stored defaults are frozen (D-029): a value equal to its class default is never written, so changing a default
  would silently change every project that kept it. `test_jrpg_core_regressions.h` fingerprints the stored defaults
  of every persisted class; changing one needs a migration step that writes the old value into older data.
- Saves record the database schema version and a content hash of the tables used (spec 06).

## Editing rules
- The database editor edits one entry at a time through an embedded `EditorInspector` (never the whole table, to
  avoid huge inspectors; task 6.4.2). `JRPGInspectorPlugin` serves it and the main Inspector: integer properties that
  hold one ID (`JRPGReferenceScanner.get_id_target`, one table shared with the scanner) are searchable lists of the
  target's entries ("0003  Fire"; 0 None, -1 Normal Attack where the range allows it, missing IDs shown as such);
  the name is shown first. Engine `resource_*` properties are hidden from inspectors (the table owns paths and
  scene-unique ids). Custom `EditorProperty`s for traits/effects/curves come with the per-tab tasks (6.5.x).
- Entries added by Change Maximum get their `file::Kind_id` path at once (as the loader and saver would,
  scene/resources/resource_format_text.cpp:633,1942), so inspector edits of an unsaved new entry also go to the
  global history.
- List operations (task 6.4.1, `JRPGDatabaseEditing`, shared by the workspace's Database section and the `jrpg_db_*`
  bridge tools): Change Maximum, Clear, Copy, Paste. The clipboard holds a deep copy taken at copy time; paste only
  into the same table; each paste gets its own arrays and dictionaries (`JRPGEntry.assign_from`). Change Maximum and
  Clear list the references they would break (`JRPGReferenceScanner`) before confirming; the bridge refuses them
  unless `allow_broken_references` is set.
- Undo: database edits use the global undo history (D-005). Each action is created for `GLOBAL_HISTORY` with
  `force_fixed_history()`: entries added by Change Maximum have no path until saved, and Godot routes path-less
  resources to the open scene's history (editor/editor_undo_redo_manager.cpp:82-86).
- Saving: changes are tracked per file (table, System, Types, Terms) through the resources' `changed` signals and
  shown as "(*)" on the table. `JRPGBootstrapPlugin::save_external_data()` saves them with the editor's own saves
  (scene save, Ctrl+S without a scene, before running: editor/editor_node.cpp:2525,3495,2603); quitting or reloading
  asks first through `get_unsaved_status()` (editor/editor_node.cpp:3794). The Save button and `jrpg_db_save` save
  them too. Only changed files are written.
- External changes (files edited on disk while the editor runs) are reloaded through Godot's resource reload
  (editor/editor_node.cpp:1291-1323); reloaded files lose their unsaved mark; the database UI refreshes on the
  resources' `changed` signal.

## Reference scanner (task 5.6)
Finds every use of an entry ID: other tables (traits, effects, learnings, drops, actions, members, equipment),
events and common events (command parameters), maps (encounters), system data. Used for: delete/clear warnings,
"Change Maximum" warnings, a dangling-reference report, and the final sweep's database-integrity checks.
`JRPGReferenceScanner` (core) returns references as `{source, source_id, path, target, target_id}` (`path` such as
`effects[1].data_id`; targets are table names or the type lists `elements`, `skill_types`, `weapon_types`,
`armor_types`, `equip_types`; System data is source `system`, id 0). `find_dangling` marks targets beyond the table or
list as `missing` and entries/types without a name as `blank` (cleared). Which trait and effect codes carry IDs is
listed in `jrpg_reference_scanner.cpp` (`trait_target`, `effect_target`); actor initial equipment slot 1 is a
weapon, and slot 2 too with dual wield. Event command parameters (task 9.2.2), map encounters (task 12.3) and the
System start map (task 8.3) join the scanner when their data exists.

## Edge cases
- Dangling ID (points beyond the table or to a cleared entry) → runtime treats it as "none" and logs a warning once.
- Duplicate scene-unique ids after copy/paste → the table re-assigns `<Kind>_<id>` on save.
- Empty tables are valid; ID 1 may be cleared.
- Very large tables (2000+ entries, RM-OTH-048): up to `JRPGTable.MAX_ENTRIES` (100000); the entry list is an
  `ItemList`, which draws only the visible rows (scene/gui/item_list.cpp:1496-1530).
