# RPG Maker inventory (Phase 1A) — entry point

**Baseline: RPG Maker MZ** — the most complete *released* standalone editor with complete official online
documentation (https://rpgmakerofficial.com/product/MZ_help-en/). Lineup and baseline reasoning:
`plan/research/planning_notes/00_lineup_baseline_trademark.md` (U2U announced 2026-06-15, unreleased).
Notable features of other versions: slice 09.

The inventory has **1023 sourced items** (1007 distinct features; 16 items describe the same feature in two slices and are cross-referenced as duplicates in `gap_map.md`), each `### RM-<AREA>-<NNN> — <name>` with **What**, **Behaviour**
and an official **Source** URL. Items are paraphrased from official pages (legal rule D-020); behaviour that the
official pages do not state is listed in each slice's final "Not verified" section, never guessed.

| Slice | File | Items | Areas | Not verified |
|---|---|---|---|---|
| Project, editor, aid tools, test play, plugins, deployment, localization, new features, asset/side-view standards | `rpgmaker/01_project_editor_tools.md` | 203 | PRJ 10, EDT 52, AID 21, TST 22, PLG 41, DEP 11, ASV 10, L10N 6, NEW 30 | 37 |
| Map editor, map properties, tilesets, asset standards | `rpgmaker/02_maps_tilesets_assets.md` | 119 | MAP 38, MPR 21, TIL 31, AST 29 | 34 |
| Database A: database UI, actors, classes, skills, items, weapons, armors | `rpgmaker/03_database_a.md` | 99 | DB 10, ACT 12, CLS 12, SKL 23, ITM 22, WPN 10, ARM 10 | 51 |
| Database B: enemies, troops/battle events, states, animations, common events | `rpgmaker/04_database_b.md` | 93 | ENM 24, TRP 22, STA 19, ANI 20, CEV 8 | 37 |
| Database C: system 1/2, types, terms, traits, effects | `rpgmaker/05_database_c.md` | 84 | SYS 32, TYP 6, TRM 6, TRT 26, EFF 14 | 42 |
| Events, event commands 1–8, message control characters, movement routes | `rpgmaker/06_events_commands_1.md` | 171 | EVT 48, CMD 53, MSG 19, MRT 51 | 30 |
| Event commands 9–16 | `rpgmaker/07_event_commands_2.md` | 52 | CMD2 52 | 37 |
| Runtime: menus, battle flow, formulas, save, audio, messages, input, generator, debug | `rpgmaker/08_runtime_systems.md` | 125 | MNU 20, BTL 26, FRM 21, SAV 7, AUD 15, MSGB 13, INP 9, GEN 7, DBG 7 | 34 |
| Other versions (2000, 2003, XP, VX, VX Ace, MV, Unite, WITH, U2U) | `rpgmaker/09_other_versions.md` | 77 | OTH 77 | 14 |

The request's minimum coverage list maps to areas as follows: project structure (PRJ); map editor, layers,
autotiles, passability/flags, regions, map properties, encounters (MAP, MPR, TIL, AST); every database tab
(DB, ACT, CLS, SKL, ITM, WPN, ARM, ENM, TRP, STA, ANI, TIL, CEV, SYS, TYP, TRM, TRT, EFF); event system,
pages, conditions, triggers, priority, movement routes, self switches, switches/variables and every event
command (EVT, CMD, CMD2, MRT); message system and escape codes (MSG, MSGB); menus (MNU); battle systems and
formulas (BTL, FRM); save/load (SAV); audio (AUD); character/face generator (GEN, AID); resource manager
(AID); test play, battle test, debug (TST, DBG); plugins and parameters (PLG); localization (L10N);
deployment (DEP).

## Gates (task 2.1.9)
- **TOC coverage:** `python plan/tools/toc_gate.py` — all 65 content pages of the crawled official TOC
  (`plan/research/mz_help_toc.md`) are cited by ≥1 item; 3 non-content pages exempt (license, trademarks,
  credits). Output: `plan/evidence/research/toc_gate.txt`.
- **Every item sourced:** 1023/1023 items carry an official URL; IDs unique (file 01's asset-standard items use
  `ASV` to avoid colliding with file 02's `AST`).
- **No verbatim copying:** `python plan/tools/verbatim_check.py --allow RM-PRJ-008,RM-PLG-025,RM-TRP-012,RM-TRM-003,RM-INP-001,RM-INP-003`
  compares every 12-word run with the downloaded official pages: 0 prose overlaps; the 6 allowed items share
  only lists of names (asset folder names, control-character codes, button labels, the 8 parameter names, key
  bindings), which the legal rule permits. Three prose overlaps found on the first run were rewritten.
  Output: `plan/evidence/research/verbatim_check.txt`.

## Known gaps in the official documentation (selected; full lists at the end of each slice)
Escape success formula; critical damage multiplier; order of damage-calculation steps; EXP curve formula;
the six damage-type names beyond Damage/Recover/Drain; state duration randomness; save file contents and
storage location; full Options screen list; message fast-forward keys; several dropdown option lists only
visible in screenshots. Per D-022 Talewright defines and documents its own behaviour for these instead of
guessing RPG Maker's.

## Gates added by the epic 2 review (task 2.5)
- **Field-label coverage:** `python plan/tools/label_gate.py` — the `<h3>/<h4>/<dt>` labels of every content page
  (1157) must appear in the inventory; 0 missing, 126 waived with reasons in `plan/research/label_gate_waivers.txt`
  (heading wording differences; every candidate field label was grep-verified). Output: `plan/evidence/research/label_gate.txt`.
- **Stricter verbatim check:** default lowered to 9-word runs; 0 prose overlaps; reviewed name-list items in
  `plan/research/verbatim_allowlist.txt`.
