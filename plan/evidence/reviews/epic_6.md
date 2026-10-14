# Review — Epic 6 (JRPG workspace and database editor)

Reviewer: fresh-context subagent (two read-only sub-reviews for the bridge UI tools and for addon/setup/workspace),
2026-10-13, read-only, on `git diff 803d0e2f7c..HEAD -- modules talewright/tools talewright/docs plan/specs`
(commits 6.1–6.6) against specs 01/02, D-005, D-008, D-012, D-029–D-032 and the RM-DB/TRT/EFF and database inventory
rows. Reported 2 high, 12 medium and 19 low / low-medium findings.

Confirmed fine: every authoring-API action is created for GLOBAL_HISTORY with `force_fixed_history`; table undo
arrays are copies; clear/paste restore the same objects in place; new entries get `file::Kind_id` paths; list
editors copy arrays and dictionaries before `emit_changed`; the `Range::set_min` guard holds in every custom editor;
trait/effect units and ranges match RM-TRT-002..026 and RM-EFF-002..014 and D-032; curve generation and parameter
limits; equipment slot filtering and dual wield; core files include no editor or scene headers; editor tests are
`TOOLS_ENABLED`-guarded; either module builds alone; "Save & Quit" cannot lose database edits.

Fixes land in tasks 6.7.1 (data integrity), 6.7.2 (database UI), 6.7.3 (bridge UI tools) and 6.7.4 (addon, setup,
hints, tests, docs).

| # | Sev | Finding | Fix |
|---|---|---|---|
| 1 | high | Edits inside nested sub-resources (System cues and battle rules, troop pages) were not tracked as unsaved; path-less ones routed their edits to the open scene's undo history | fixed (6.7.1): `JRPGDatabaseEditing::_adopt/_watch_nested` watch sub-resources up to 4 levels with their file as owner and give each a `file::id` path; the emitter is re-scanned on change; troop pages get ids before `changed` |
| 2 | high | `editor_ui_find`/`editor_ui_act` reached controls inside the edited scene (input-disabled SubViewport), breaking D-031 | fixed (6.7.3): viewports that take no input (the edited scene, editor/editor_node.cpp:8871) are skipped by find and refused by act; doctest + bridge check |
| 3 | med | `jrpg_db_set` could truncate type lists and set IDs that do not exist without warning | fixed (6.7.1): `jrpg_db_set` refuses shrinking a used type list (uses listed) unless `allow_broken_references`; reports `dangling_references` of the record, and for type lists the data pointing at removed types |
| 4 | med | `create_entry` could fill a blank slot that other data still points at | fixed (6.7.1): `create_entry` skips blank slots that `collect_references` finds referenced |
| 5 | med | Change Maximum: a value typed but not yet applied was ignored on confirm (deferred SpinBox apply) | fixed (6.7.2): the maximum field applies typed text as it is typed (warning follows); confirm applies pending text and re-opens to show the warning of an unseen cut |
| 6 | med | UI tools ignored open modal dialogs | fixed (6.7.3): a window with a visible modal child refuses everything outside that child's chain; open dialogs are still found from the root |
| 7 | med | `press` emitted `pressed` only, not the click path (`pressed()` virtual, button_down/up); MenuButton/OptionButton did not open | fixed (6.7.3): press delivers a ui_accept press and release through gui_input: pressed() runs (menus and option lists open), button_down/up, radio groups |
| 8 | med | `set_text` without a value typed `<null>`; `submit` replaced a TextEdit's text; numbers typed as "42.0" | fixed (6.7.3): set_text needs text (numbers typed as digits); submit without a value submits what is there; TextEdit refuses submit |
| 9 | med | `select` emitted signals a click would not (ItemList multi mode, multi Tree, re-selecting the current option) | fixed (6.7.3): select follows each widget's click: OptionButton through its popup (no signal for the current item), ItemList multi/toggle signals, Tree::set_selected |
| 10 | med | Disabled MenuButtons could be used; menus that fill on opening showed stale items | fixed (6.7.3): disabled MenuButtons refuse; about_to_popup is emitted before items are read (JRPGPicker lists its entries) |
| 11 | med | Setup changed the live database without the tracker (new tables unwatched, System saved while marked unsaved) | fixed (6.7.1): setup calls `forget_unsaved` for system.tres after saving the game id; the manifest's `changed` re-watches new tables |
| 12 | med | No tests for undo round trips, the jrpg bridge tools or the dialogs | fixed (6.7.4): undo/redo round trips of every database tool in a real editor (`talewright/tools/jrpg_bridge_checks.py`, 39 checks, evidence 6.7.4-jrpg-bridge-checks.txt); tool-level doctests through `ClaudeBridgeMCP` (`test_jrpg_bridge_tools.h`); trait dialog exact values; record list Edit/Remove; record dialog get_row (6.7.2) |
| 13 | med | Bridge edits of structured fields unchecked (array element shapes, typed `pages`) and nested fields (cues, battle rules, page conditions) not editable | fixed (6.7.1): `check_values/check_element` check list elements (trait/effect codes and keys, record fields and enum ranges, resource classes); `resolve_path` nested keys; bridge reads/writes sub-resources as dictionaries and reports the refusal reason |
| 14 | low-med | `editor_ui_find` revealed secret LineEdit text | fixed (6.7.3): secret LineEdits are shown and matched as bullets |
| 15 | low-med | `confirm`/`cancel` could log an engine error after acting; plain-dialog cancel differed from Esc | fixed (6.7.3): the target is described before acting and by ObjectID after ("closed" when it left); plain-dialog cancel goes through WM_CLOSE_REQUEST; the reply waits for deferred calls |
| 16 | low-med | Addon hashes: builder normalizes CRLF, runtime hashed raw bytes | fixed (6.7.4): runtime hashes of text files normalize CRLF to LF as the builder does; doctest |
| 17 | low | MCP hints contradicted the documented convention (save, install, copy) | fixed (6.7.4): `jrpg_db_save` and `jrpg_addon_install` destructive, `jrpg_db_copy` not read-only; documented convention lists them |
| 18 | low | `show_dialog` on `jrpg_db_clear` cleared at once when nothing used the entry, unlike its description | fixed (6.7.4): `show_dialog` described as pressing the section's button (Clear asks only when the entry is used, clears at once otherwise) |
| 19 | low | `param` rejected "3"; curve inputs not range-checked (int overflow); fractional ids truncated | fixed (6.7.1): `int_arg` accepts integers, integral floats and digit strings, refuses fractions; curve inputs clamped and generated in doubles |
| 20 | low | `jrpg_db_get/set` on a missing System/Types/Terms never completed | fixed (6.7.1): `database_record` fails with a setup hint when System/Types/Terms are missing |
| 21 | low | The view's Save left the editor's global history unsaved | fixed (6.7.1): `save_all` clears the saved files' edited flags and, when no other project file has edits, saves the global history and emits `history_changed` (the title's "(*)" clears; bridge check part 2) |
| 22 | low | Undo of an EXP edit baked an empty table; dialogs emitted without changes; unknown trait codes replaced on Apply | fixed (6.7.1): EXP setters re-bake only a baked table (undo exact; preview computes); list and curve dialogs skip no-op applies; unknown trait codes keep Apply disabled until a kind is chosen |
| 23 | low | List edits applied by the row index captured at popup | fixed (6.7.1): `_apply_row` re-finds the row opened in the dialog, or warns and skips |
| 24 | low | Troop page actor picker offered None (setter clamps to 1); equipment pickers stale after a dual-wield change | fixed (6.7.2): ID pickers take the range minimum (no None for a troop page's actor); equipment pickers list choices when opened and relabel on database changes (Dual Wield) |
| 25 | low | Enemy action conditions as raw numbers (HP/MP basis points, state ID without picker) | fixed (6.7.2): action values shaped by condition (`JRPGRecordSpecs::Shape`, D-033): turn A/B, HP/MP percent band, state picker, level, switch; one Condition column ("HP 0% to 50%") |
| 26 | low | Record dialog: out-of-range enums and untouched out-of-range ints changed on Apply | fixed (6.7.2): the record dialog keeps untouched fields exactly (out-of-range ints shown unclamped, unknown enums as "n (unknown)") |
| 27 | low | Large tables rebuilt the whole list and every picker on each change | fixed (6.7.2): `entry_changed` signal; the entry list updates changed rows in place; `JRPGPicker` lists entries only when opened |
| 28 | low | `JRPGEntry.assign_from` shared sub-resources despite its docs | fixed (6.7.1): `JRPGEntry::copy_value` deep-duplicates arrays, dictionaries and internal sub-resources |
| 29 | low | Setup and addon install errors hidden | fixed (6.7.4): install reports `failed`/`error`; setup refuses existing files of the wrong class or table kind and stops on an addon error; `jrpg_setup_project` names what was created before an error |
| 30 | low | Addon tool robustness (directory, pruning, zero-byte files, addon.cfg rewrites, builder paths) | fixed (6.7.4): `directory` must be a folder of res://addons; untouched files no longer shipped are removed; zero-byte files install; addon.cfg written only on change; builder uses `os.path.relpath`; doctests |
| 31 | low | UI tool gaps (hidden OK, toggle values, non-integral indexes, hidden tab bars, root path, cross-window search, non-editable ranges, duplicate tool names) | fixed (6.7.3): hidden OK refuses confirm; toggle values checked; whole-number indexes and limit; hidden tab bars refuse; root path refused; actions stay in one window; set_value only on number fields and sliders; duplicate tool names refused (ERR_ALREADY_EXISTS) |
| 32 | low | Missing setup idempotency and UI-tool dialog tests; unused include; static checks pass when tools are skipped | fixed (6.7.4): setup idempotency checked by jrpg_bridge_checks.py; UI-tools dialog/modal/MenuButton/TextEdit tests (6.7.3); the include is now used; static_checks.py fails on skipped checks unless --allow-skips |
| 33 | low | Stale workspace guide, untranslatable non-literal strings, "before closing" on reload, double refresh on section select | double refresh fixed (6.7.2: `select_section` refreshes once); guide rewritten for setup and the Database section, `TTRC` on the section names and the setup menu item, neutral quit/reload prompt (6.7.4) |
