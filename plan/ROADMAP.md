# Talewright Roadmap

_Generated from `plan/tasks.json` by `python plan/tools/tasks.py render`. Do not edit by hand._

Legend: `[x]` done, `[~]` in progress, `[ ]` pending, `[!]` blocked, `[-]` deferred. Priority P1 (highest) .. P4.

**Totals:** pending: 106, in_progress: 0, done: 132, blocked: 0, deferred: 3 (of 241)

## [x] 0 Bootstrap and task system

Fork, local clone, working branch, persistent tracker, resume protocol, persisted planning research.

- [x] **0.1** Fork, clone, upstream remote, branch talewright @ 4.7.2-stable (P1) _(commit: ed1daf0bf0)_
- [x] **0.2** Task system: plan/ files and tasks.py (P1) _(deps: 0.1; commit: e0717a6727)_
- [x] **0.3** CLAUDE.md resume protocol, NOTICE and fork README (P1) _(deps: 0.2; commit: 12253dd0a5)_
- [x] **0.4** Persist planning research reports (P1) _(deps: 0.2; commit: d611bf476c)_

## [x] 1 Toolchain baseline

Reproducible builds and a recorded vanilla baseline before any engine change.

- [x] **1.1** Build wrapper talewright/tools/build.py (P1) _(commit: 2c1f389019)_
- [x] **1.2** Vanilla editor dev_mode build (P1) _(deps: 1.1; commit: ed36cad7e0)_
- [x] **1.3** Upstream test baseline (P1) _(deps: 1.2; commit: 499d506b7c)_
- [x] **1.4** Export template builds (template_release, template_debug) (P2) _(deps: 1.2; commit: b96aeaeae0)_
- [x] **1.5** Plain general-purpose samples (P2) _(deps: 1.2, 1.4; commit: 51255adeae)_
- [x] **1.6** Epic 0+1 review fixes (P1) _(commit: a6fbf412ba)_
- [x] **1.7** Fork files pass upstream CI static checks (P1) _(commit: b1fb0940d7)_
- [x] **1.8** Fork CI: every Linux job green (P1) _(deps: 1.7; commit: 803d0e2f7c)_
- [x] **1.9** Fork CI: templates compile the tests; clangd-tidy clean (P1) _(deps: 1.8; commit: ee2c26277a)_

## [x] 2 Research (Phase 1)

Sourced RPG Maker inventory, Godot current state from the clone, MMO feasibility, gap map.

- [x] **2.1** RPG Maker inventory (P1)
  - [x] **2.1.1** Lineup, baseline, project structure, editor basics (P1) _(commit: 01307d3dd1)_
  - [x] **2.1.2** Map editor, map properties, tilesets (P1) _(commit: b8e9d81f47)_
  - [x] **2.1.3** Database A: actors, classes, skills, items, weapons/armors (P1) _(commit: c2cca57cf2)_
  - [x] **2.1.4** Database B: enemies, troops, states, animations, common events (P1) _(commit: 2a82d83e30)_
  - [x] **2.1.5** Database C: system 1/2, types, terms, traits, effects (P1) _(commit: 434f27dce2)_
  - [x] **2.1.6** Events: system, pages, conditions, triggers, priority, move routes, all commands (P1) _(commit: 0aec90fe28)_
  - [x] **2.1.7** Messages, escape codes, menus, battle systems and formulas, save/load, audio, character generator, debug (P1) _(commit: ca16b51e19)_
  - [x] **2.1.8** Other versions' notable features (P2) _(commit: 3f1bff4314)_
  - [x] **2.1.9** Inventory merge and TOC gate (P1) _(deps: 2.1.1, 2.1.2, 2.1.3, 2.1.4, 2.1.5, 2.1.6, 2.1.7, 2.1.8; commit: 8daa29608b)_
- [x] **2.2** Godot current state (from the clone) (P1) _(deps: 0.4; commit: 23d0dcdf80)_
- [x] **2.3** MMO feasibility and rAthena techniques (P1) _(deps: 0.4; commit: 0a33c6e7da)_
- [x] **2.4** Gap map (P1) _(deps: 2.1, 2.2, 2.3; commit: 89239e392b)_
- [x] **2.5** Epic review: research (P1) _(deps: 2.4; commit: 6eb6a76de9)_

## [x] 3 Architecture decisions and specs (Phases 2-3)

Decision log, upstream patch ledger, design specs, refined task tree.

- [x] **3.1** DECISIONS.md (P1) _(commit: 7d0964b4de)_
- [x] **3.2** UPSTREAM_PATCHES.md ledger (P2) _(commit: 81bb919732)_
- [x] **3.3** Specs (P1) _(deps: 3.1; commit: e5dbc43068)_
- [x] **3.4** Refine task tree (P1) _(deps: 3.3; commit: 1bd824ac35)_
- [x] **3.5** Epic review: decisions and specs (P1) _(deps: 3.4)_
  - [x] **3.5.1** Review fixes: battle math and formulas (spec 04) (P1) _(commit: 59b450e196)_
  - [x] **3.5.2** Review fixes: tiles, autotiles, passage bits, character sheets (spec 05) (P1) _(deps: 3.5.1; commit: 8eb312e55e)_
  - [x] **3.5.3** Review fixes: core layering, save robustness, determinism (specs 01, 03, 05, 06) (P1) _(deps: 3.5.2; commit: 67d53aa4ac)_
  - [x] **3.5.4** Review fixes: online page authority, localization keys, inconsistencies (P1) _(deps: 3.5.3; commit: d25927e933)_
  - [x] **3.5.5** Review fixes: task tree order, slice gaps, criteria, core_diff_check; review record (P1) _(deps: 3.5.4; commit: 2ab7e3196b)_

## [x] 4 Claude bridge (MCP)

Editor-only C++ module: MCP Streamable HTTP server on 127.0.0.1 with token; editor + game inspection and driving; CLI.

- [x] **4.1** Module skeleton modules/claude_bridge (P1) _(commit: 00c95dc78e)_
- [x] **4.2** HTTP/1.1 server component (P1) _(deps: 4.1; commit: 4eac8ec916)_
- [x] **4.3** Auth, Origin/Host checks, connection file (P1) _(deps: 4.2; commit: b4d7680cd4)_
- [x] **4.4** MCP JSON-RPC layer (P1) _(deps: 4.3; commit: 110b884601)_
- [x] **4.5** bridge.py CLI (P1) _(deps: 4.4; commit: 6f08aa1d78)_
- [x] **4.6** Editor tools: tree, nodes, project (P1) _(deps: 4.5; commit: 3c278a7942)_
- [x] **4.7** Logs and errors (P1) _(deps: 4.6; commit: 601c674fea)_
- [x] **4.8** Run/stop (P1) _(deps: 4.7; commit: a2a9c2529f)_
- [x] **4.9** Editor screenshots (P1) _(deps: 4.8; commit: 12773911ae)_
- [x] **4.10** Game-side capture (P1) _(deps: 4.9; commit: fea49bd11f)_
- [x] **4.11** Hardening (P1) _(deps: 4.10; commit: 9c9ec40d5a)_
- [x] **4.12** Docs, claude mcp add, template absence (P1) _(deps: 4.11; commit: 5302ccaedc)_
- [x] **4.13** Epic review: Claude bridge (P1) _(deps: 4.12; commit: c0673d2898)_

## [x] 5 JRPG module foundation and data core

modules/jrpg with pure-logic core (no Node dependency), database, game state, RNG, formulas.

- [x] **5.1** Module skeleton and dynamic workspace (P1)
  - [x] **5.1.1** Module skeleton (P1) _(commit: ebd8ac64d8)_
  - [x] **5.1.2** jrpg/enabled + dynamic workspace (P1) _(deps: 5.1.1; commit: 43a0aa1607)_
- [x] **5.2** Database core (P1) _(deps: 5.1)_
  - [x] **5.2.1** Entry/table/manifest classes (P1) _(commit: 43ba9d9de0)_
  - [x] **5.2.2** Schema versioning and byte-identical persistence (P1) _(deps: 5.2.1; commit: 3a16fc1f88)_
- [x] **5.3** Database tables (slice fields) (P1) _(deps: 5.2, 2.1)_
  - [x] **5.3.1** Actors, classes, types, terms, system (P1) _(commit: 2ec147be91)_
  - [x] **5.3.2** Skills, items, weapons, armors (P1) _(deps: 5.3.1; commit: 57cc51f10a)_
  - [x] **5.3.3** Enemies, troops, states, animations stub, tilesets data, common events (P1) _(deps: 5.3.2; commit: 390a77cfee)_
- [x] **5.4** RNG, parameters, traits, formulas (P1) _(deps: 5.3)_
  - [x] **5.4.1** JRPGRandom (PCG32) (P1) _(commit: 00d2e575a7)_
  - [x] **5.4.2** Traits, effects, parameters, stat cache, EXP curve (P1) _(deps: 5.4.1; commit: d05cbf6650)_
  - [x] **5.4.3** JRPGFormula (P1) _(deps: 5.4.2; commit: bebdd8b95a)_
- [x] **5.5** Game state core (P1) _(deps: 5.4)_
  - [x] **5.5.1** JRPGGameState core (P1) _(commit: f357cf1e53)_
  - [x] **5.5.2** Runtime actor state (P1) _(deps: 5.5.1; commit: 1295b2eb53)_
  - [x] **5.5.3** Event runtime state (P1) _(deps: 5.5.1; commit: 5ec93c1ded)_
  - [x] **5.5.4** Actor equipment, class change and reference repair (P1) _(deps: 5.5.2; commit: 63a440f97e)_
- [x] **5.6** Reference scanner (P2) _(deps: 5.5; commit: dbea2b98f0)_
- [x] **5.7** Epic review: JRPG core (P1) _(deps: 5.6; commit: 7cfb5653ce)_

## [x] 6 JRPG workspace and database editor

Setup tool, managed addon, main-screen workspace, database editor, authoring API.

- [x] **6.1** Managed addon embedding (P1) _(commit: 554066373a)_
- [x] **6.2** Set up JRPG Project tool (P1) _(deps: 6.1; commit: 895eec6a88)_
- [x] **6.3** Workspace shell (P1) _(deps: 6.2; commit: ac59a59975)_
- [x] **6.4** Database editor framework (P1) _(deps: 6.3)_
  - [x] **6.4.1** Database window, tabs and entry list (P1) _(commit: 8c57162b56)_
  - [x] **6.4.2** Entry form, undo, save, ID pickers (P1) _(deps: 6.4.1; commit: 25423f20df)_
- [x] **6.5** Database tab forms (slice) (P1) _(deps: 6.4)_
  - [x] **6.5.0** Bridge: editor UI interaction tools (P1) _(commit: 130bb48a21)_
  - [x] **6.5.1** Trait and effect editors (P1) _(deps: 6.5.0; commit: 8c9f5fcbe9)_
  - [x] **6.5.2** Actors, classes, skills, items forms (P1) _(deps: 6.5.1)_
    - [x] **6.5.2.1** Class EXP curve and learnings (P1) _(commit: 581a945334)_
    - [x] **6.5.2.2** Parameter curve editor (P1) _(deps: 6.5.2.1; commit: 04dbf10e7d)_
    - [x] **6.5.2.3** Actor initial equipment and slice field check (P1) _(deps: 6.5.2.2; commit: e5fe6b28ea)_
  - [x] **6.5.3** Weapons, armors, enemies, troops, states, system, types, terms forms (P1) _(deps: 6.5.2)_
    - [x] **6.5.3.1** Parameter grids, enemy drops and actions, troop members (P1) _(commit: 27dfc497c0)_
    - [x] **6.5.3.2** System party, type lists and slice field check (P1) _(deps: 6.5.3.1; commit: c98c01be4b)_
- [x] **6.6** Database authoring API + bridge tools (P1) _(deps: 6.5; commit: c788b268bc)_
- [x] **6.7** Epic review: workspace (P1) _(deps: 6.6)_
  - [x] **6.7.1** Review fixes: database data integrity (P1) _(commit: b7501e870c)_
  - [x] **6.7.2** Review fixes: database UI (P1) _(deps: 6.7.1; commit: f8e005287c)_
  - [x] **6.7.3** Review fixes: bridge UI tools (P1) _(commit: b40024f3dc)_
  - [x] **6.7.4** Review fixes: addon, setup, hints, tests, docs (P1) _(deps: 6.7.1, 6.7.3; commit: a72b7240a6)_
  - [x] **6.7.5** Epic 6 review close-out (P1) _(deps: 6.7.1, 6.7.2, 6.7.3, 6.7.4; commit: 8e16c19757)_

## [x] 7 Assets and tilesets

Original generated assets, autotile importer and resolver, tileset flags.

- [x] **7.1** Original asset generator (P1)
  - [x] **7.1.1** Generated tiles and autotile templates (P1) _(commit: 5ae44ffd2f)_
  - [x] **7.1.2** Generated characters, faces, enemies, battlebacks, icons, window skin (P1) _(deps: 7.1.1)_
    - [x] **7.1.2.1** Generated character sheets and faces (P1) _(commit: 70f3604677)_
    - [x] **7.1.2.2** Generated enemy battlers and battlebacks (P1) _(deps: 7.1.2.1; commit: 0dc1b46965)_
    - [x] **7.1.2.3** Generated icon set and window skin (P1) _(deps: 7.1.2.2; commit: df03e168d4)_
  - [x] **7.1.3** Synthesized SFX and BGM (P1) _(deps: 7.1.2; commit: 7eea0185c4)_
- [x] **7.2** Autotile template + importer (P1) _(deps: 7.1; commit: 76f6d02a50)_
- [x] **7.3** JRPGTileset + flags + tileset tab (P1) _(deps: 7.2)_
  - [x] **7.3.1** Tile flag layers, bake and staleness (P1) _(commit: a63c34ca05)_
  - [x] **7.3.2** Tileset tab with flag overlays (P1) _(deps: 7.3.1)_
    - [x] **7.3.2.1** Tileset authoring API (P1) _(deps: 7.3.1; commit: 5b15badb20)_
    - [x] **7.3.2.2** Tileset form with flag overlays (P1) _(deps: 7.3.2.1; commit: 4d026f244e)_
  - [x] **7.3.3** Tileset bridge tools (P1) _(deps: 7.3.2; commit: 42229a98dc)_
- [x] **7.4** Autotile resolver (P1) _(deps: 7.2; commit: 7b1706be56)_
- [x] **7.5** Import authoring tools (P2) _(deps: 7.3; commit: 975dc7eda4)_
- [x] **7.6** Epic review: assets (P1) _(deps: 7.5, 7.4)_
  - [x] **7.6.1** Review fixes: assets, autotile core, importer, resolver (P1) _(commit: a47cb524f1)_
  - [x] **7.6.2** Review fixes: tile flags, tileset form, bridge tools (P1) _(deps: 7.6.1; commit: 133cb6e076)_
  - [x] **7.6.3** Epic 7 close-out (P1) _(deps: 7.6.2; commit: c2fabd838a)_

## [ ] 8 Maps

JRPGMap scenes, map editor canvas, cell cache, runtime map and movement.

- [x] **8.1** JRPGMap node + spike (P1)
  - [x] **8.1.1** Map data and map list (P1) _(commit: cab2cece50)_
  - [x] **8.1.2** JRPGMap node (P1) _(deps: 8.1.1; commit: a807812582)_
  - [x] **8.1.3** Map scene spike (P1) _(deps: 8.1.2; commit: 2cf119671a)_
- [x] **8.2** Map cell cache compiler (P1) _(deps: 8.1; commit: 53d32934d3)_
- [x] **8.3** Map list and properties (P1) _(deps: 8.1)_
  - [x] **8.3.1** Map list authoring API and bridge tools (P1) _(commit: a80ed0a213)_
  - [x] **8.3.2** Start map checks and start position picker (P1) _(deps: 8.3.1; commit: da64948eea)_
  - [x] **8.3.3** Map tree and properties UI (P1) _(deps: 8.3.2; commit: 3d1e41f6ae)_
- [ ] **8.4** Map canvas tools (P1) _(deps: 8.3, 7.4)_
  - [x] **8.4.1** Map canvas view and MapEditSession (P1) _(commit: pending)_
  - [ ] **8.4.2** Drawing tools with undo patches (P1) _(deps: 8.4.1)_
  - [ ] **8.4.3** Shadow, region, event modes and auto-layer (P1) _(deps: 8.4.2)_
- [ ] **8.5** Tile palette (P1) _(deps: 8.4)_
- [ ] **8.6** Runtime map, cell-step movement, camera, transfer (P1) _(deps: 8.2)_
  - [ ] **8.6.1** Scene manager, map loading and transfer (P1)
  - [ ] **8.6.2** Player movement, passability, dash, camera (P1) _(deps: 8.6.1)_
- [ ] **8.7** Character sprites (P1) _(deps: 8.6)_
- [ ] **8.8** Map authoring tools (P2) _(deps: 8.5)_
- [ ] **8.9** Epic review: maps (P1) _(deps: 8.7, 8.8)_

## [ ] 9 Events and interpreter

Pages, conditions, triggers, command model with authority, interpreter with presentation port, slice commands, event editor.

- [ ] **9.1** JRPGEvent + pages + conditions + triggers (P1)
- [ ] **9.2** Command model + registry + authority + script commands (P1) _(deps: 9.1, 3.3)_
  - [ ] **9.2.1** Command storage benchmark (P1)
  - [ ] **9.2.2** Command model, registry, authority, script commands (P1) _(deps: 9.2.1)_
- [ ] **9.3** Interpreter (P1) _(deps: 9.2)_
  - [ ] **9.3.1** Interpreter frames, waits, labels, budgets (P1)
  - [ ] **9.3.2** Common events and map processes (P1) _(deps: 9.3.1)_
  - [ ] **9.3.3** Interpreter serialization and resume rules (P1) _(deps: 9.3.2)_
- [ ] **9.4** Presentation port + local port (P1) _(deps: 9.3)_
- [ ] **9.5** Slice commands (P1) _(deps: 9.4)_
  - [ ] **9.5.1** Message and flow commands (P1)
  - [ ] **9.5.2** State commands (P1) _(deps: 9.5.1)_
  - [ ] **9.5.3** Scene and movement commands (P1) _(deps: 9.5.2)_
- [ ] **9.6** Event characters runtime (P1) _(deps: 9.5, 8.7)_
  - [ ] **9.6.1** Autonomous movement (P1)
  - [ ] **9.6.2** Triggers and map event processes (P1) _(deps: 9.6.1)_
- [ ] **9.7** Event editor UI (P1) _(deps: 9.6)_
  - [ ] **9.7.1** Event properties, pages and conditions UI (P1)
  - [ ] **9.7.2** Command list, picker and forms (P1) _(deps: 9.7.1)_
  - [ ] **9.7.3** Quick events (P1) _(deps: 9.7.2)_
- [ ] **9.8** Event authoring tools (P2) _(deps: 9.7)_
- [ ] **9.9** Epic review: events (P1) _(deps: 9.8)_

## [ ] 10 Messages and UI kit

Window frame, selectable list, message system, screen stack. Built before epic 9 so the local presentation port and NPC dialogs have a message window (epic 3 review).

- [ ] **10.1** Window frame + selectable list (C++) (P1)
- [ ] **10.2** Message system (P1) _(deps: 10.1)_
  - [ ] **10.2.1** Control-code parser and text layout (P1)
  - [ ] **10.2.2** Message window, choices, number input, gold window (P1) _(deps: 10.2.1)_
- [ ] **10.3** Screen stack (GDScript managed addon) (P1) _(deps: 10.2)_
- [ ] **10.4** Epic review: messages/UI (P1) _(deps: 10.3)_

## [ ] 11 Menu (slice)

Main menu, item, skill, equip, status, save, game end.

- [ ] **11.1** Menu screens (P1)
  - [ ] **11.1.1** Main menu and item screen (P1)
  - [ ] **11.1.2** Skill screen (P1) _(deps: 11.1.1)_
  - [ ] **11.1.3** Equip screen (P1) _(deps: 11.1.2)_
  - [ ] **11.1.4** Status screen (P1) _(deps: 11.1.3)_
- [ ] **11.2** Epic review: menu (P1) _(deps: 11.1)_

## [ ] 12 Battle (slice)

Turn-based core, scene, encounters, troop events, battle test.

- [ ] **12.1** Battle core (P1)
  - [ ] **12.1.1** Battlers, actions and damage pipeline (P1)
    - [ ] **12.1.1.1** Battlers and action resolution (P1)
    - [ ] **12.1.1.2** Damage pipeline, hit, critical, elements (P1) _(deps: 12.1.1.1)_
    - [ ] **12.1.1.3** Basic states in battle (P1) _(deps: 12.1.1.2)_
  - [ ] **12.1.2** Turn order, enemy AI, victory/defeat/escape, rewards (P1) _(deps: 12.1.1)_
- [ ] **12.2** Battle scene (front view) (P1) _(deps: 12.1)_
  - [ ] **12.2.1** Battle layout, status and command windows, targeting (P1)
    - [ ] **12.2.1.1** Battle layout and status window (P1)
    - [ ] **12.2.1.2** Command windows and targeting (P1) _(deps: 12.2.1.1)_
  - [ ] **12.2.2** Battle log, popups, battleback and enemy sprites (P1) _(deps: 12.2.1)_
- [ ] **12.3** Encounters + troop events (P1) _(deps: 12.2)_
- [ ] **12.4** Battle test + test from here (P2) _(deps: 12.3)_
- [ ] **12.5** Epic review: battle (P1) _(deps: 12.4)_

## [ ] 13 Save/load, title and vertical slice milestone (M1)

Save format, title/game over, slice export, demo playthrough.

- [ ] **13.1** Save format + slots (P1)
  - [ ] **13.1.1** Save file format (P1)
  - [ ] **13.1.2** Game state save and continue (P1) _(deps: 13.1.1)_
  - [ ] **13.1.3** Save and load screens (P1) _(deps: 13.1.2)_
- [ ] **13.2** Title, game over, boot flow (P1) _(deps: 13.1)_
- [ ] **13.3** Demo content and slice export (P1) _(deps: 13.2)_
  - [ ] **13.3.1** jrpg_demo content via bridge authoring tools (P1)
  - [ ] **13.3.2** Slice export (P1) _(deps: 13.3.1)_
- [ ] **13.4** Scripted M1 playthrough (P1) _(deps: 13.3)_
- [ ] **13.5** Vanilla regression rerun (P1) _(deps: 13.4)_
- [ ] **13.6** Milestone review M1 (P1) _(deps: 13.5)_

## [ ] 14 MMO protocol and transport

Go/no-go spikes first (D-024): raw ENetConnection 1000-peer loopback test; 1000-entity zone tick prototype with AoI (p99 < 50 ms); handoff chaos test. Then: IDL + codegen, framing/reassembly, ENet (DTLS) and WebSocket (TLS) transports, per-opcode token buckets, session-state gating, fuzz tests. Subtasks are defined when the epic starts (rule: split before starting).


## [ ] 15 Server runtime

Role processes (login/world/zone), timer heap with entity-ID ownership, config descriptor tables, logging, graceful shutdown, server data compiler (map cache + DB snapshot + content hash), server build profile (mmo_server=yes). Subtasks are defined when the epic starts (rule: split before starting).


## [ ] 16 Persistence

DB abstraction, SQLite vendored, async worker, migrations ledger, atomic versioned character saves, dirty diffs, unique item IDs, audit logs, PostgreSQL backend flag. Subtasks are defined when the epic starts (rule: split before starting).


## [ ] 17 Accounts, login and handoff

Argon2id accounts, brute-force bans, HMAC tickets, online registry (kick older), per-player state machine with final-save ack, reconnect/resync, login/char-select/create screens. Subtasks are defined when the epic starts (rule: split before starting).


## [ ] 18 Zone simulation

AoI 8x8 block grid with strip updates + hysteresis, authoritative movement (tile prediction mode and click-to-move mode), replication + interpolation, chat, warps and zone handoff, channels, instances. Subtasks are defined when the epic starts (rule: split before starting).


## [ ] 19 Server-side events and shops

Interpreter on the zone server with network presentation port, NPC dialogs (timeouts, disconnect cleanup), server shops with two-phase transactions, variable scopes persistence. Subtasks are defined when the epic starts (rule: split before starting). Scopes and per-player/shared events per D-025; dialogs carry keys + args (D-026).


## [ ] 20 Online battles

Instanced turn-based battles (PokeMMO-style) and ARPG core (spawns, two-speed AI, skill timers, drops, exp sharing, damage logs). Subtasks are defined when the epic starts (rule: split before starting).


## [ ] 21 Editor and bridge integration, online slice (M2)

Online project mode, Server workspace (Run Local Cluster, N clients, logs, entity inspector, packet inspector, GM console, bots), bridge cluster tools; M2 online slice evidence. Subtasks are defined when the epic starts (rule: split before starting).


## [ ] 22 Database completion

Every field of every MZ database tab, full traits and effects, types, terms, system 1/2, animations tab. Each feature works offline and declares online authority. Subtasks from gap map rows when the epic starts.


## [ ] 23 Map editor completion

All tools, map loops, parallax, encounters/regions UI, quick events, copy/paste/shift, map search. Each feature works offline and declares online authority. Subtasks from gap map rows when the epic starts.


## [ ] 24 All event commands and move routes

Every command in the 16 MZ categories and every movement route command, offline and online authority. Each feature works offline and declares online authority. Subtasks from gap map rows when the epic starts.


## [ ] 25 Characters, vehicles, followers, balloons

Vehicles (boat/ship/airship), followers, balloon icons, character animations on map. Each feature works offline and declares online authority. Subtasks from gap map rows when the epic starts.


## [ ] 26 Pictures, screen effects, weather, timers

Show/move/rotate/tint/erase picture, fade/tint/flash/shake, weather, timer. Each feature works offline and declares online authority. Subtasks from gap map rows when the epic starts.


## [ ] 27 Battle completion and ARPG action system

TPB active/wait, side-view battlers and motions, enemy action patterns, full states, battle events; real-time action battle system for single-player. Each feature works offline and declares online authority. Subtasks from gap map rows when the epic starts.


## [ ] 28 Menus completion

Formation, options, shop, name input, item/key-item choice, status variants. Each feature works offline and declares online authority. Subtasks from gap map rows when the epic starts.


## [ ] 29 Audio system

BGM/BGS/ME/SE, fades, memorize/replay, buses, editor preview. Each feature works offline and declares online authority. Subtasks from gap map rows when the epic starts.


## [ ] 30 Animations

Particle/sprite-based animation resources with SE/flash timings and editor preview (replacing Effekseer). Each feature works offline and declares online authority. Subtasks from gap map rows when the epic starts.


## [ ] 31 Debug and test-play tools

Playtest options, in-game debug panel (switches/variables), event test, battle test variants. Each feature works offline and declares online authority. Subtasks from gap map rows when the epic starts.


## [ ] 32 Character generator

Part-based generator with original parts producing walk/face/battler sheets. Each feature works offline and declares online authority. Subtasks from gap map rows when the epic starts.


## [ ] 33 Resource manager and asset import pipelines

Browse/import/preview/delete assets; importers for characters, tilesets, animations, audio; bridge authoring tools. Each feature works offline and declares online authority. Subtasks from gap map rows when the epic starts.


## [ ] 34 Plugin manager and plugin commands

Addon-based plugins with ordered enable list, parameters, plugin commands. Each feature works offline and declares online authority. Subtasks from gap map rows when the epic starts.


## [ ] 35 Localization

String extraction from DB/messages (EditorTranslationParserPlugin), CSV/PO, runtime locale switch, CJK font fallback. Each feature works offline and declares online authority. Subtasks from gap map rows when the epic starts. Keys per D-026 (<Kind>_<id>.<field>, command text_key).


## [ ] 36 Deployment

Export presets with JRPG checks; Windows templates locally; other platforms via fork CI; PCK encryption. Each feature works offline and declares online authority. Subtasks from gap map rows when the epic starts.


## [ ] 37 Social systems

Party, guild, friends, mail (RODEX-like) with persistence and cross-zone sync.


## [ ] 38 Economy

Trade (two-phase), market/GTL, vending, storage, item audit trail.


## [ ] 39 GM and ops

Permission groups, GM commands with audit, admin HTTP API + game-op queue, staged content apply with verify/rollback, metrics, restart countdown.


## [ ] 40 Security hardening

DTLS/TLS everywhere, inter-server auth, DDoS/per-IP limits, login bans, protocol fuzzing in CI.


## [ ] 41 Production persistence, packaging, Linux builds

PostgreSQL backend, server packaging, service units, backups, Linux builds via fork CI or WSL.


## [ ] 42 Load and chaos testing

Bot swarm, latency/loss injection, kill -9 recovery, dupe attempts; capacity measured and recorded.


## [-] 43 Optional Nakama adapter

Backend adapter allowing Nakama for accounts/social.


## [-] 44 Branding patch for distribution

Replace window title/icons/binary names if the fork is ever distributed (trademark policy).


## [-] 45 RPG Maker project importer

One-way importer of user-owned RPG Maker projects.


## [ ] 46 Full parity and MMO maturity milestone (M3)

Offline and online playthroughs covering every implemented feature; evidence in plan/evidence/M3/.


## [ ] 47 Phase 5 final bug and edge-case sweep

Re-read every touched file; all checklist items of the request + MMO checks; findings table; not-verified list; blocked/deferred list.
