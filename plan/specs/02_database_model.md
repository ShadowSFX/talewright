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
| `name` | String | localizable (spec: L10N epic 35 extracts it) |
| `note` | String | free text; `<tag:value>` metadata parsed on demand (plugin parity) |
| `icon_index` | int | where applicable |

## Packed nested lists (values, not sub-resources)
Traits, effects, learnings, enemy actions, drops, troop members, parameter curves are stored as packed arrays of
small dictionaries or packed ints so tables stay compact:
- **Trait**: `{code:int, data_id:int, value:float}` — codes enumerate RM-TRT-002..026 (element rate, debuff rate,
  state rate, state resist, param, xparam, sparam, attack element, attack state, attack speed, attack times, attack
  skill, add/seal skill type, add/seal skill, equip weapon/armor type, lock/seal equip, slot type, action times,
  special flag, collapse effect, party ability).
- **Effect**: `{code:int, data_id:int, value1:float, value2:float}` — codes for RM-EFF-002..014 (recover HP/MP, gain
  TP, add/remove state, add/remove buff/debuff, special effect, grow, learn skill, common event).
- **Learning**: `{level:int, skill_id:int, note:String}`; **Drop**: `{kind:int, data_id:int, denominator:int}`;
  **Enemy action**: `{skill_id:int, condition_type:int, param1:float, param2:float, rating:int}`;
  **Troop member**: `{enemy_id:int, x:int, y:int, hidden:bool}`.
- Parameter curves: `PackedInt32Array` of 8 params × (max_level+1) values.
Exact field lists per table follow the inventory slices (03/04/05) and are implemented in task 5.3.

## Versioning and migration
- `JRPGDatabase.schema_version` and `JRPGTable.schema_version` (int). The loader runs `_migrate(from, to)` steps;
  renamed properties are handled with `_set()` fallbacks (old name → new name) so old files load.
- Saves record the database schema version and a content hash of the tables used (spec 06).

## Editing rules
- The database editor edits one entry at a time through `EditorInspector` (never the whole table, to avoid huge
  inspectors); custom `EditorProperty`s for traits/effects/curves/ID pickers.
- Undo: database edits use the global undo history (D-005, editor/editor_undo_redo_manager.cpp:81-101); the table
  resource is marked edited and saved with "Save All".
- External changes (files edited on disk while the editor runs) are reloaded through Godot's resource reload; the
  database UI refreshes on the resource `changed` signal.

## Reference scanner (task 5.6)
Finds every use of an entry ID: other tables (traits, effects, learnings, drops, actions, members, equipment),
events and common events (command parameters), maps (encounters), system data. Used for: delete/clear warnings,
"Change Maximum" warnings, a dangling-reference report, and the final sweep's database-integrity checks.

## Edge cases
- Dangling ID (points beyond the table or to a cleared entry) → runtime treats it as "none" and logs a warning once.
- Duplicate scene-unique ids after copy/paste → the table re-assigns `<Kind>_<id>` on save.
- Empty tables are valid; ID 1 may be cleared.
- Very large tables (2000+ entries, RM-OTH-048): no hard cap; editor list is virtualized.
