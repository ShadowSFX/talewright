# Talewright Progress

<!-- status:begin -->
- **Updated:** 2026-10-13 16:02 -0600
- **Current task:** none
- **Last completed:** 3.2 UPSTREAM_PATCHES.md ledger
- **Next task:** 3.3 Specs
- **Done leaf tasks:** 30
<!-- status:end -->

## Log

| Time | Task | What | Commit |
|---|---|---|---|
| 2026-10-13 15:02 -0600 | 0.2 | started: Task system: plan/ files and tasks.py |  |
| 2026-10-13 15:02 -0600 | 0.1 | Fork ShadowSFX/talewright created via gh; cloned; upstream remote (push disabled); branch talewright @ 4.7.2-stable (ed1daf0bf0). Done before tracker existed. | |
| 2026-10-13 15:02 -0600 | 0.2 | done: Task system: plan/ files and tasks.py — plan/ tracker created: tasks.json (142 tasks), tasks.py CLI, ROADMAP/PROGRESS/DECISIONS/UPSTREAM_PATCHES | e0717a6727 |
| 2026-10-13 15:02 -0600 | 0.3 | started: CLAUDE.md resume protocol, NOTICE and fork README |  |
| 2026-10-13 15:02 -0600 | 0.3 | done: CLAUDE.md resume protocol, NOTICE and fork README — Root CLAUDE.md (resume protocol, loop, rules, commands) and TALEWRIGHT.md (overview + trademark disclaimer) | 12253dd0a5 |
| 2026-10-13 15:02 -0600 | 0.4 | started: Persist planning research reports |  |
| 2026-10-13 15:04 -0600 | 0.4 | done: Persist planning research reports — Planning research persisted: 7 subagent reports verbatim + lineup/baseline/trademark notes in plan/research/planning_notes; MZ help TOC crawler + crawled TOC (closed containers: 0) | d611bf476c |
| 2026-10-13 15:04 -0600 | 1.1 | started: Build wrapper talewright/tools/build.py |  |
| 2026-10-13 15:10 -0600 | 2.1.1 | started: Lineup, baseline, project structure, editor basics |  |
| 2026-10-13 15:10 -0600 | 2.1.2 | started: Map editor, map properties, tilesets |  |
| 2026-10-13 15:10 -0600 | 2.1.3 | started: Database A: actors, classes, skills, items, weapons/armors |  |
| 2026-10-13 15:10 -0600 | 2.1.4 | started: Database B: enemies, troops, states, animations, common events |  |
| 2026-10-13 15:10 -0600 | 2.1.5 | started: Database C: system 1/2, types, terms, traits, effects |  |
| 2026-10-13 15:10 -0600 | 2.1.6 | started: Events: system, pages, conditions, triggers, priority, move routes, all commands |  |
| 2026-10-13 15:10 -0600 | 2.1.7 | started: Messages, escape codes, menus, battle systems and formulas, save/load, audio, character generator, debug |  |
| 2026-10-13 15:10 -0600 | 2.1.8 | started: Other versions' notable features |  |
| 2026-10-13 15:10 -0600 | 2.2 | started: Godot current state (from the clone) |  |
| 2026-10-13 15:13 -0600 | 2.2 | done: Godot current state (from the clone) — godot_current_state.md written from the clone; 79 citations range-checked + 21 content anchors verified (check_citations.py) | 23d0dcdf80 |
| 2026-10-13 15:13 -0600 | 2.3 | started: MMO feasibility and rAthena techniques |  |
| 2026-10-13 15:14 -0600 | 1.1 | done: Build wrapper talewright/tools/build.py — Build wrapper: profiles, vswhere shim via --site-dir (no Godot changes), d3d12=no, accesskit path pin, lock-kill verified (stopped PID), error summary verified | 2c1f389019 |
| 2026-10-13 15:14 -0600 | 1.2 | started: Vanilla editor dev_mode build |  |
| 2026-10-13 15:15 -0600 | 1.2 | done: Vanilla editor dev_mode build — Vanilla editor dev_mode build: 5.8 min, 0 errors, 0 warnings; facts recorded in godot_current_state.md | ed36cad7e0 |
| 2026-10-13 15:15 -0600 | 1.3 | started: Upstream test baseline |  |
| 2026-10-13 15:18 -0600 | 1.3 | done: Upstream test baseline — Baseline: 1380 passed / 0 failed / 3 skipped, 419763 assertions, 27.5 s; two identical runs; 1383 case names saved | 499d506b7c |
| 2026-10-13 15:18 -0600 | 2.3 | done: MMO feasibility and rAthena techniques — mmo_feasibility.md (verdict, reuse table, NEW list, topology, modes, persistence, prior art, legal, capacity targets) + rathena_techniques.md (40 techniques + 14 weaknesses, rAthena d4b8e7b8 refs). Note: an earlier draft of rathena_techniques.md was swept into commit 1.2. | 0a33c6e7da |
| 2026-10-13 15:23 -0600 | 1.4 | done: Export template builds (template_release, template_debug) — template_release 3.7 min (68.6 MB), template_debug 3.9 min (83.0 MB), 0 errors/0 warnings | b96aeaeae0 |
| 2026-10-13 15:23 -0600 | 1.5 | started: Plain general-purpose samples |  |
| 2026-10-13 15:23 -0600 | 1.5 | done: Plain general-purpose samples — plain_2d (CharacterBody2D vs wall) + plain_3d (RigidBody3D on floor) samples with --smoke self-checks; smoke_samples.py: editor open/import, run, export with custom template + run exported exe: ALL PASS (closed containers: 1) | 51255adeae |
| 2026-10-13 15:24 -0600 | 4.1 | started: Module skeleton modules/claude_bridge |  |
| 2026-10-13 15:32 -0600 | 4.1 | done: Module skeleton modules/claude_bridge — claude_bridge module skeleton: editor-only config.py, internal EditorPlugin with settings (enabled, port), doctest; builds with and without the module (vanilla: 1380/1380 tests, smoke pass); build.py gains SCons cache + UTF-8-safe output; add_header.py | 00c95dc78e |
| 2026-10-13 15:33 -0600 | - | imported 3 new / 8 updated tasks (containers: 1 done->in_progress) | |
| 2026-10-13 15:36 -0600 | 1.6 | done: Epic 0+1 review fixes — All 14 findings of the epic 0+1 review fixed; record in plan/evidence/reviews/epic_0_1.md (containers: 1 in_progress->done) | a6fbf412ba |
| 2026-10-13 15:38 -0600 | 2.1.1 | done: Lineup, baseline, project structure, editor basics — Official-source inventory slice: 203 items (PRJ 10, EDT 52, AID 21, TST 22, PLG 41, DEP 11, AST 10, L10N 6, NEW 30); every item cites an official URL; paraphrase check run | 01307d3dd1 |
| 2026-10-13 15:38 -0600 | 2.1.2 | done: Map editor, map properties, tilesets — Official-source inventory slice: 119 items (MAP 38, MPR 21, TIL 31, AST 29); every item cites an official URL; paraphrase check run | b8e9d81f47 |
| 2026-10-13 15:38 -0600 | 2.1.3 | done: Database A: actors, classes, skills, items, weapons/armors — Official-source inventory slice: 99 items (DB 10, ACT 12, CLS 12, SKL 23, ITM 22, WPN 10, ARM 10); every item cites an official URL; paraphrase check run | c2cca57cf2 |
| 2026-10-13 15:38 -0600 | 2.1.4 | done: Database B: enemies, troops, states, animations, common events — Official-source inventory slice: 93 items (ENM 24, TRP 22, STA 19, ANI 20, CEV 8); every item cites an official URL; paraphrase check run | 2a82d83e30 |
| 2026-10-13 15:38 -0600 | 2.1.5 | done: Database C: system 1/2, types, terms, traits, effects — Official-source inventory slice: 84 items (SYS 32, TYP 6, TRM 6, TRT 26, EFF 14); every item cites an official URL; paraphrase check run | 434f27dce2 |
| 2026-10-13 15:38 -0600 | 2.1.6 | done: Events: system, pages, conditions, triggers, priority, move routes, all commands — Official-source inventory slice: 223 items (EVT 48, CMD 53, MSG 19, MRT 51, CMD2 52); every item cites an official URL; paraphrase check run | 0aec90fe28 |
| 2026-10-13 15:38 -0600 | 2.1.7 | done: Messages, escape codes, menus, battle systems and formulas, save/load, audio, character generator, debug — Official-source inventory slice: 125 items (MNU 20, BTL 26, FRM 21, SAV 7, AUD 15, MSGB 13, INP 9, GEN 7, DBG 7); every item cites an official URL; paraphrase check run | ca16b51e19 |
| 2026-10-13 15:38 -0600 | 2.1.8 | done: Other versions' notable features — Official-source inventory slice: 77 items across 2000/2003/XP/VX/VX Ace/MV/Unite/WITH/U2U; every item cites an official URL; paraphrase check run (containers: 2.1 in_progress->pending, 2 in_progress->pending) | 3f1bff4314 |
| 2026-10-13 15:38 -0600 | 2.1.9 | started: Inventory merge and TOC gate (containers: 2.1 pending->in_progress, 2 pending->in_progress) |  |
| 2026-10-13 15:39 -0600 | 2.1.9 | done: Inventory merge and TOC gate — Inventory gate: TOC coverage 65/65 pages, 1023 sourced unique items (file 01 AST->ASV), verbatim check 0 prose overlaps (3 rewritten, 6 name lists allowed); rpgmaker_inventory.md index; D-022 (containers: 2.1 in_progress->done, 2 in_progress->pending) | 8daa29608b |
| 2026-10-13 15:41 -0600 | 2.4 | started: Gap map (containers: 2 pending->in_progress) |  |
| 2026-10-13 15:42 -0600 | 2.4 | done: Gap map — gap_map.md generated: 1023 inventory rows (ID parity verified) + 16 MMO rows; 1873 Godot citations verified (anchors checked at generation); reuse 86 / extend 488 / NEW 465 (containers: 2 in_progress->pending) | 89239e392b |
| 2026-10-13 15:42 -0600 | 2.5 | started: Epic review: research (containers: 2 pending->in_progress) |  |
| 2026-10-13 15:42 -0600 | 4.2 | started: HTTP/1.1 server component (containers: 4 pending->in_progress) |  |
| 2026-10-13 15:45 -0600 | 4.2 | done: HTTP/1.1 server component — HTTP/1.1 component: incremental parser (411/413/431/501/505/400), non-blocking server with bounded buffers, deferred responses, keep-alive, idle/request/write timeouts, header-injection-safe responses; 6 new doctests incl. loopback; full suite 1387/1387 (containers: 4 in_progress->pending) | 4eac8ec916 |
| 2026-10-13 15:45 -0600 | 4.3 | started: Auth, Origin/Host checks, connection file (containers: 4 pending->in_progress) |  |
| 2026-10-13 15:48 -0600 | 4.3 | done: Auth, Origin/Host checks, connection file — Auth: persistent 256-bit token in editor config dir, Host/Origin/Bearer checks (constant-time), port fallback 47700-47709, .godot/claude_bridge/connection.json; e2e curl: 401/401/403/403/405/415/404, non-loopback refused; 3 new doctests (containers: 4 in_progress->pending) | b4d7680cd4 |
| 2026-10-13 15:51 -0600 | 4.4 | done: MCP JSON-RPC layer — MCP JSON-RPC dispatcher: initialize (version negotiation), ping, tools/list (schemas+annotations), tools/call (sync/async with deadlines, isError), notifications 202, parse/batch/invalid errors, integer id preservation, stateless calls without initialize; plugin pending-call processing; bridge_status tool; 3 new doctests (13 total) | 110b884601 |
| 2026-10-13 15:51 -0600 | 4.5 | started: bridge.py CLI (containers: 4 pending->in_progress) |  |
| 2026-10-13 15:52 -0600 | 4.5 | done: bridge.py CLI — bridge.py CLI: connection discovery, status/list/call/raw/launch/wait, images saved as PNG files, exit codes; verified against a live headless editor (containers: 4 in_progress->pending) | 6f08aa1d78 |
| 2026-10-13 16:01 -0600 | - | imported 0 new / 1 updated tasks | |
| 2026-10-13 16:01 -0600 | 2.5 | done: Epic review: research — Epic 2 review: all 15 findings fixed (gap map rules/overrides/duplicates, research corrections, media gaps, MMO conditional verdict + go/no-go spikes + risk table + legal, rAthena citation checking, label gate, 9-word verbatim); record in plan/evidence/reviews/epic_2.md (containers: 2 in_progress->done) | 6eb6a76de9 |
| 2026-10-13 16:01 -0600 | 3.1 | started: DECISIONS.md (containers: 3 pending->in_progress) |  |
| 2026-10-13 16:02 -0600 | 3.1 | done: DECISIONS.md — DECISIONS.md: 25 decisions (D-001..D-024 + D-015a), all with context/options/choice/why/reversal (containers: 3 in_progress->pending) | 7d0964b4de |
| 2026-10-13 16:02 -0600 | 3.2 | started: UPSTREAM_PATCHES.md ledger (containers: 3 pending->in_progress) |  |
| 2026-10-13 16:02 -0600 | 3.2 | done: UPSTREAM_PATCHES.md ledger — UPSTREAM_PATCHES.md ledger with format, candidates (ENet notes) and enforcement script core_diff_check.py; core diff currently empty (containers: 3 in_progress->pending) | pending |
