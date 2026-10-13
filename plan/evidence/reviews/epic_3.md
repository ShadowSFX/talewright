# Review — Epic 3 (Decisions, specs, task tree)

Reviewer: fresh-context subagent, 2026-10-13, over `plan/DECISIONS.md`, `plan/specs/01..07` and the epic 5–13 task
tree. Confirmed fine: all 25 decisions had the five fields; `core_diff_check.py` passed; the two ENet candidates in the
ledger are accurate. 6 high, 6 medium and 5 low findings. Fixes landed in tasks 3.5.1–3.5.5 (commits stamped in
`plan/tasks.json`).

| # | Sev | Finding | Fix (task) |
|---|---|---|---|
| 1 | high | Spec 04's "Talewright-defined" battle numbers matched RPG Maker internals on every undocumented point (step order, ×3 critical, max of attack elements, two-roll hit, `agi + rand(agi/4)`); critical chance departed from the documented `cri × (1 − cev)` | Spec 04 rebuilt: documented behaviour kept with inventory citations (variance range, `cri × (1 − cev)`, buffs, TP start, TPB √agi, guard and hit/evasion rules); own rules for the gaps (formula → variance → element → critical → rate → guard, ×2 critical, average of attack elements, additive one-roll hit, agility ±10 % with a per-battle tie shuffle, −40 % guard, TP on damage), each with its reason in `talewright/docs/battle_defaults.md`; D-022 amended (3.5.1) |
| 2 | high | Spec 05's "original" autotile template was RPG Maker's 2×3 block; passage bits mirrored internal flags; no character sheet spec | 47-tile blob sheet (8×6, rank of the reduced clockwise mask) + 4×4 wall sheet; own flag layout (pass mode open/blocked/transparent, separate no-entry/no-exit masks) baked into the tileset data; one-character sheets with clockwise rows and import settings instead of filename prefixes; D-020 amended (3.5.2) |
| 3 | high | Core classes depended on `TileSet`, `Texture2D`, `AudioStream` and on event nodes in scenes | Assets in core are `uid://` strings; baked tile-flag table; `JRPGMapData` (built-in sub-resource of the map scene, read node-free through `SceneState`) with events as `JRPGEventData`, internal event nodes + `JRPGEventAnchor`; `JRPGEventState`; D-006 amended; tasks 5.3.3, 8.1, 9.1 updated (3.5.3) |
| 4 | high | Saves resumed at wrong commands after edits; duplicated events/maps duplicated ids | Per-list content hashes (Merkle-style) in saved frames with restart/drop rules and one warning; map revisions for event states; duplicate `event_id` reassigned on save; `JRPGMapList` reassigns duplicate map ids on scan; doctest criteria in 9.3.3 and 13.1.2 (3.5.3) |
| 5 | high | Order: 9.6/9.7.3 needed the message window of epic 10; 11.1.2 saved before 13.1; 13.3 exported a demo built later | Epic 10 now depends on 8 and epic 9 on 8 + 10; epic 11 on 9; save screens moved to 13.1.3 after the format; 13.3 split into 13.3.1 demo content via bridge authoring tools and 13.3.2 export (3.5.5) |
| 6 | high | Online authority for event pages undefined | D-025: scopes for every switch/variable kind, per-character self switches unless shared, per-player events evaluated by clients and validated per trigger (fixed movement only), shared events evaluated once per zone; editor flags mixed scopes; spec 03/07 updated (3.5.4) |
| 7 | medium | Expression facts wrong (`[]` exists; `v.get` cannot be bound; utility functions shadow helpers) | Spec 04: `v` passed as the variables Array (`v[n]`), `const_calls_only` (`expression.cpp:1451`), name resolution order (`:485`), explicit identifier allow-list with `roll()` instead of utility RNGs; D-009 amended (3.5.1) |
| 8 | medium | "Integer battle math" contradicted float traits and the RNG API; FP contraction risk | int64 + basis points throughout, integer trait/effect values in spec 02, `chance_bp`; FP contraction already off engine-wide (`platform/*/detect.py`) and libm functions excluded from formulas; D-010 amended (3.5.1) |
| 9 | medium | Spec 06 FileAccess facts wrong | Correct names and lines; header never encrypted; zstd raw size stored and capped (`compression.cpp:294`); body encrypted in memory with CryptoCore (FileAccessEncrypted decrypts whole files and writes only on close); `.tmp`/`.bak` sequence because Windows rename deletes the target first (`dir_access_windows.cpp:339-345`) (3.5.3) |
| 10 | medium | No fixed step; one RNG for everything | `JRPGSimClock` fixed 60 Hz steps (accumulator, max 4 steps per frame); named RNG streams (battle, map_ai, encounter, loot, misc) saved separately; zones use the same steps (3.5.1, 3.5.3, 3.5.4) |
| 11 | medium | Slice gaps: New Game/start position, encounter distribution, battle return, templates, audio, input actions, fixtures | Spec 02 slice field set incl. start position; spec 05 encounter rule (uniform ½n…1½n around the documented average); 12.2.2 battle-return and BGM criteria; 13.3.2 rebuilds `template_release` with `modules/jrpg`; map BGM (8.6.1), UI sounds (10.1); spec 01 `jrpg_*` input actions added by the setup tool (6.2); fixtures built in test code (5.6, 8.2); start position tool (8.8) and New Game criterion (13.2) (3.5.5) |
| 12 | medium | No localization key strategy | D-026: `<Kind>_<id>.<field>` for database text, random per-command `text_key`, keys + arguments over the network, saves store only name overrides; tasks 9.5, 10.2, 19, 35 linked (3.5.4) |
| 13 | low | Spec 05 internals (tile_map_data vs own encoding, 8-bit alternative, shape byte, edge rule, speed table) | Palette-index cells keep Godot's full alternative id (transform bits 12–14); no stored shape byte; `edge_mode` extend/border + loop wrap; speed and frequency tables in steps; D-006 amended (3.5.2) |
| 14 | low | Non-HP parameter minimum 1; TP rules missing | Minimum 0 for all but max HP; TP max, start (documented 0–24), gain on damage (own rule), regeneration (3.5.1) |
| 15 | low | `game_id` as project setting; deny- vs allow-list; benchmark task id; rAthena/RPG Maker constants in spec 07 | `game_id` in System data; allow-list everywhere; D-007 → 9.2.1; own direction order (clockwise from north), own path limits (24 cells / 64 steps) with reasons, step-based movement messages, own dialog timeouts (120/300 s) (3.5.1, 3.5.4) |
| 16 | low | `core_diff_check.py`: renames hidden, whitespace split, any backtick counted, not in a gate | `--no-renames -z`, NUL-separated untracked files, paths only from the ledger table's File(s) column, `--root` option; tested on a fixture repo (output below); added to the epic gate in `CLAUDE.md`, every epic gate's test strategy and 13.5 (3.5.5) |
| 17 | low | Vague criteria; oversized tasks | Criteria rewritten for 5.3.x, 5.6, 6.1, 6.2, 6.3, 6.5.x, 7, 7.2, 7.4, 8.2, 8.4.2, 10.1 (image comparison with stated tolerance via `img_compare.py`), 13.1, 13.3, 13.4 (definition of a pass); split 9.3 → 9.3.1–3, 9.6 → 9.6.1–2, 11.1 → 11.1.1–4, 12.1.1 → 12.1.1.1–3, 12.2.1 → 12.2.1.1–2, 13.1 → 13.1.1–3, 13.3 → 13.3.1–2 (3.5.2, 3.5.3, 3.5.5) |

## core_diff_check.py fixture test (3.5.5)
Fixture repo: base commit with `core/a.cpp`, `core/b c.cpp` and a ledger listing `core/b c.cpp` in the table and
mentioning `core/a.cpp` outside it; then `git mv core/a.cpp modules/jrpg/a.cpp`, edit `core/b c.cpp`, add
`core/new file.cpp`:

```
UNLISTED core/a.cpp
LISTED   core/b c.cpp
UNLISTED core/new file.cpp
core paths changed: 3; unlisted: 2
exit=1
```
The move into an owned directory is caught, a backtick outside the table does not count, and paths with spaces
survive. On the real tree: `core paths changed: 0; unlisted: 0`.
