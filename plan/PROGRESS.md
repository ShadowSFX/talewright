# Talewright Progress

<!-- status:begin -->
- **Updated:** 2026-10-13 15:18 -0600
- **Current task:** 2.1.1 Lineup, baseline, project structure, editor basics, 2.1.2 Map editor, map properties, tilesets, 2.1.3 Database A: actors, classes, skills, items, weapons/armors, 2.1.4 Database B: enemies, troops, states, animations, common events, 2.1.5 Database C: system 1/2, types, terms, traits, effects, 2.1.6 Events: system, pages, conditions, triggers, priority, move routes, all commands, 2.1.7 Messages, escape codes, menus, battle systems and formulas, save/load, audio, character generator, debug, 2.1.8 Other versions' notable features, 2.3 MMO feasibility and rAthena techniques
- **Last completed:** 1.3 Upstream test baseline
- **Next task:** 1.4 Export template builds (template_release, template_debug)
- **Done leaf tasks:** 8
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
| 2026-10-13 15:18 -0600 | 1.3 | done: Upstream test baseline — Baseline: 1380 passed / 0 failed / 3 skipped, 419763 assertions, 27.5 s; two identical runs; 1383 case names saved | pending |
