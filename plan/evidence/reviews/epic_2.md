# Review — Epic 2 (Research)

Reviewer: fresh-context subagent, 2026-10-13. Confirmed: TOC gate 65/65, citations ok, verbatim ok; 13 official pages
compared field by field with no missing field/command/code; ~25 items spot-checked (1 misstatement).
Fixes landed in task 2.5.

| # | Sev | Finding | Fix |
|---|---|---|---|
| 1 | high | Gap map rows identical per area; wrong mappings (EVT editor rows, BTL-006..008, FRM-011/016/020, SAV-003) | Rules apply to all non-database areas; curated `OVERRIDES` for the cited rows; database-field areas keep their (accurate) uniform mapping |
| 2 | high | OTH/NEW rule noise (URLs, substrings like "window"/"web"/"save") | Rules match the item name only with word boundaries; 49 OTH/NEW rows hand-curated (7 explicitly N/A) |
| 3 | high | CMD/CMD2/AID misfires (Play Movie→audio, encounter→tile counter, location→transfer, save access→file I/O, dungeon generator→character generator) | Category prefixes stripped; rule order fixed; overrides for each cited row |
| 4 | medium | R/E/N and sizes (AUD reuse, DEP encryption reuse, INP-002, TIL-016/017, L sizes) | AUD→extend; DEP/AUD encryption→extend with `core/SCsub:190`; INP-002→pathfinding; autotile rows→NEW importer M with animated-tile ref; per-row sizes S/M + per-epic totals table |
| 5 | medium | Duplicate items across slices | `DUPLICATES` map (16): rendered as cross-references, excluded from counts (1007 distinct) |
| 6 | medium | RM-FRM-002 stated unverified names | Aligned with RM-SKL-017; SKL-017 guess removed |
| 7 | medium | Missing media gaps (Theora-only video, Ogg loop end/tags) | New section 6b in godot_current_state.md; gap rows AUD-011/012, CMD2-017 updated |
| 8 | medium | "Deterministic sub-resource ids" wrong | Corrected (random, clock-seeded; Talewright sets ids explicitly) with `core/io/resource.cpp:129-150` |
| 9 | medium | libgodot/GDExtension imprecision | One-instance check `platform/windows/libgodot_windows.cpp:42`; extension editor plugins `core/extension/gdextension.cpp:1108`; exposed flag `core/object/class_db.h:151` |
| 10 | low | Citation drift and thin sections | All listed citations corrected; headless/doctool, inline test baseline, hi-godot/godot-ai, D-012 link added |
| 11 | high | Missing MMO risks | Risk table with mitigation and owning epic (privacy, chat, payments, bots/RMT, DDoS, cost, backups, observability, patching, staffing, GDScript tick cost, SPOF) |
| 12 | high | Monster-capture patent/trade-dress risk | Legal section + D-023 (no capture template without counsel review), source NBC News 2024 |
| 13 | medium | Unconditional verdict; misread forum source; SPOF hidden | "Conditionally feasible" + 3 go/no-go spikes (D-024, epic 14 updated); forum source restated; world SPOF and zone-crash loss stated with mitigations |
| 14 | medium | rAthena citations unchecked; W2 line; W6 MD5 wrong; "clean-room" overstated | `check_citations.py` resolves `ra:` paths (92 verified); W2→752; W6→plain text default (`use_MD5_passwords: no`); D-015a "independent reimplementation" rules |
| 15 | low | 9-word echoes; label coverage unchecked; 03 heading format | 6 prose items reworded, boilerplate removed, 9-word default + allowlist file; new `label_gate.py` (0 missing, 126 waived with reasons); 99 headings normalized |
