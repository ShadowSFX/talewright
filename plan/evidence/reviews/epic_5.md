# Review — Epic 5 (JRPG core)

Reviewer: fresh-context subagent, 2026-10-13, read-only, on the epic 5 diff (`git diff ebd8ac64d8~1..HEAD --
modules/jrpg plan/specs`: commits 5.1.1–5.6) against specs 01/02/04/06, decisions D-004–D-010, D-022, D-025, D-026,
D-028 and the database/formula inventory rows. Reported 1 high, 8 medium and 18 low findings (plus one doc and one
tooling item). Confirmed fine: core include boundary, PCG32 reference vector and unbiased bounded draws, `mul_bp`
rounding, game-state all-or-nothing deserialize and canonical form, actor refresh recursion termination, ObjectID
party link (no Ref cycle), ID = slot handling, formula sandbox (no bypass found), portability of enum checks and
range-for temporaries, spec 06 limits, editor bootstrap teardown, Talewright-own trait/effect numbering.
Fixes landed in task 5.7; regression doctests in `modules/jrpg/tests/test_jrpg_core_regressions.h` (10 cases).

| # | Sev | Finding | Fix |
|---|---|---|---|
| 1 | high | `schema_version` defaulted to the current version, and the text saver skips values equal to the class default, so no file recorded its version; the first migration would misfire | Class default -1 (`SCHEMA_UNSTAMPED`, bound); any content change (`set_entry_class`, `set_maximum`, `set_entries`, `clear_entry`; manifest setters) stamps the current version; explicit 0 stays a real older version; doctest checks a fresh save contains `schema_version = 1` (D-029, spec 02) |
| 2 | med | HP 0 knocked out an actor that resists state 1 | `_clamp_pools` knocks out only without `TRAIT_STATE_RESIST` 1; doctest |
| 3 | med | Formula result `int64_t(floor(d + 0.5))` on any finite double: UB out of range, and 0.49999999999999994 → 1 | Clamp to ±2^62, then `Math::round`; doctests for ±1e300, 0.49999999999999994, ±2.5 |
| 4 | med | EXP curve fell back to libm `pow` at run time (differs by an ULP across platforms) | `compute_total_exp` is integer fixed-point only (Q30 log2 by squaring + exp2 table); doctest: within 1 of the real formula for 3×4×2×98 cases, strictly increasing; spec 04 updated |
| 5 | med | `v[n]` past the end of the variables Array zeroed the whole formula | Highest literal `v[N]` recorded at `set_text`; a padded copy (zeros) is evaluated when the Array is shorter; doctest |
| 6 | med | Starter values (types, terms, rules, entry fields) are class defaults, never written; a changed default would silently change projects | Defaults frozen: fingerprint of every stored default of 20 persisted classes in a doctest; changing one requires a migration (D-029) |
| 7 | med | `JRPGSystemData()` instantiated `battle_rules` (ClassDB "instantiated default" warning, shared revert object) | Null by default with `PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT`; readers fall back (stat block keeps one fallback instance instead of allocating per refresh); tests updated |
| 8 | med | Scanner read dual wield only from actor traits | Also the class traits (`has_dual_wield`); doctest |
| 9 | med | Repair kept the level after the class fell back to class 1 | `_sync_level` (no skill learning) after repair; doctest checks level matches the EXP in class 1 |
| 10 | low | `mul_bp` and stat narrowing could overflow | `mul_bp` saturates at ±`INT64_MAX/10000` (unsigned magnitudes); x/sp-params clamped to int32 before narrowing; doctests |
| 11 | low | `range()` computed `high - low` in signed arithmetic | Unsigned span; doctest with `INT64_MIN..INT64_MAX` |
| 12 | low | Stream `deserialize` applied valid streams then returned false | Validate all, then apply; doc updated; doctest |
| 13 | low | Invalid formulas returned 0 silently; "one warning" untested | One warning per invalid formula (first evaluation); warning counts asserted with a counting error handler for formulas, `attach_database` and `prune_event_states` |
| 14 | low | `EquipKind` not range-checked | `ERR_FAIL_INDEX_V(int(p_kind), EQUIP_ARMOR + 1, false)` in `change_equip`/`force_equip`; doctest |
| 15 | low | Slot index checked against `equips`, not the slot types | Index checked against both; doctest |
| 16 | low | `setup`/`deserialize` could change an actor's id under its map key; `deserialize` traded equipment with the party | Id change refused (`ERR_ALREADY_IN_USE` / `ERR_INVALID_DATA`); `deserialize` refreshes without releasing equipment; doctests |
| 17 | low | `set_entries` accepted one object in two slots and entries of another class | Repeated objects copied, wrong-class entries replaced with a warning; doctest |
| 18 | low | Oversized drops/members/entries refused wholesale | Truncated to the limit with a warning; tests updated |
| 19 | low | Party members could lack an actor state | Game state keeps its database (`get_database`); `add_party_member`/`set_party_members` create states and refuse unknown actors; `attach_database` creates missing member states; doctest |
| 20 | low | `start_new_game` ignored a missing start position; `db_content_hash` never computed | `ERR_UNCONFIGURED` before any change; content hash assigned to task 13.1.2's criteria (save writer) |
| 21 | low | Battle-rule setters unvalidated (`max_hp_limit = 0` kills everyone) | Setters clamp to the inspector ranges; doctest |
| 22 | low | Every HP/MP/TP change ran a full refresh; release loop capped at 11 | Pools use `_clamp_pools` (no trait rebuild unless the knockout state changes); release loop bounded by slot count + 1 |
| 23 | low | Nested sub-resources got random scene-unique ids | `JRPGEntry::assign_sub_resource_ids` (troop pages `Troop_<id>_Page<n>`), system `BattleRules`, `Music_<slot>`, `Sound_<slot>`; doctest |
| 24 | low | Rename fallbacks only on entries | `JRPGSchema::set_renamed` used by `_set` of tables, manifest, system, types, terms, rules, audio cues, troop pages; doctest |
| 25 | low (legal) | Two default messages resembled another product's templates | Reworded: "%1 reached %2 %3." and "%1 used %2." |
| 26 | low | A test built an out-of-range enum (UBSan) | Removed |
| 27 | docs | Spec 04 "copy-on-write", missing `and/or/not`, stat cache wording | Spec 04 and class docs corrected |
| 28 | tooling | Core include check let `<platform/…>`, `<drivers/…>`, `<main/…>` through | Angle brackets in core: standard headers (no path) and the vendored PCG only |

Not verified by the reviewer and still open: Expression stack depth with deeply nested formulas; thread-safety of a
shared `JRPGFormula` on future servers (Expression is not reentrant) — recorded for the MMO zone work (epic 18).
Re-verification after the fixes: full suite 1475/1475 (441 562 assertions), clang check clean, doctool stable,
`smoke_samples.py` ALL PASS, `core_diff_check.py` 0 unlisted.
