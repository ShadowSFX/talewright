# Talewright Decision Log

Each decision: **Context**, **Options**, **Choice**, **Why**, **Reversal** (how to undo it).
Decisions are never asked of the owner; they are recorded here and execution continues.
Newer entries may supersede older ones; superseded entries are marked, never deleted.
Godot citations are `path:line` at tag `4.7.2-stable` (commit ed1daf0bf0) unless stated.

---

## D-001 — Upstream base and branch model
- **Context:** The fork must be freely modifiable yet keep pulling upstream updates. Upstream latest
  stable is `4.7.2-stable` (2026-08-18); `master` is 4.8-dev (`version.py`).
- **Options:** (a) track `master`; (b) base on the latest stable tag and merge later stable tags;
  (c) rebase onto each release.
- **Choice:** (b). Working branch `talewright` created from tag `4.7.2-stable`; the fork's `master` is left
  as an untouched mirror; upstream updates arrive by `git merge <next-stable-tag>`.
- **Why:** stable internals while building a large additive layer; merges keep history intact (the rules
  forbid rewriting shared history and force-pushing); stable tags are tested.
- **Reversal:** create a new branch from `upstream/master`, merge `talewright` into it.

## D-002 — GitHub fork and push policy
- **Context:** `gh` is authenticated as ShadowSFX; the request says to use a GitHub fork in that case.
- **Choice:** fork `ShadowSFX/talewright` (fork of godotengine/godot, created 2026-10-13, default branch
  only). Local clone at `C:\Users\Relic\desktop\grpg-maker`. Remote `upstream` = godotengine/godot with its
  **push URL disabled** (`DISABLED--never-push-to-godotengine`). The `talewright` branch is pushed to
  `origin` at the end of each epic; never force-pushed.
- **Why:** off-machine backup of the work, and the fork can still fetch upstream.
- **Reversal:** `git remote set-url --push upstream <url>` (never needed); delete the remote branch on the fork.

## D-003 — Fork name and branding
- **Context:** Godot Foundation Trademark Policy (2025-07-21,
  https://godot.foundation/policies-and-procedures/trademark-policy): the Godot logo may not be used as the
  logo of a derivative; "Godot" may appear in a name only with a word that describes its purpose; no
  implied endorsement; commercial engine forks need a license. RPG Maker trademarks must never be used.
- **Options:** keep "Godot" branding; rename everything; new product name with minimal internal changes.
- **Choice:** product name **Talewright** ("a JRPG-focused fork of the Godot Engine"). Internal binary
  names, window titles and icons stay unchanged (minimal upstream diff) because the fork is personal and
  not distributed; `TALEWRIGHT.md` carries the recommended non-affiliation disclaimer. A branding patch
  (version.py name/short_name, icons, VERSIONINFO) is task 44, `deferred` until distribution.
  Note: the local folder name `grpg-maker` is the owner's choice and is not a published name.
- **Why:** complies with the policy for a personal fork while keeping the core diff at zero.
- **Reversal:** execute task 44.

## D-004 — Where the JRPG layer lives
- **Context:** must be additive, optional, rebase-friendly (prefer modules over core patches). Modules are
  self-contained (`modules/<m>/{config.py,SCsub,register_types.*}`, detection at `methods.py:302-309`) and
  can be disabled (`module_<m>_enabled`, `SConstruct:485`). Editor-only parts compile only when
  `env.editor_build` (`modules/gridmap/SCsub:11-12`).
- **Options:** (a) GDScript addon only; (b) GDExtension; (c) engine C++ module(s); (d) mix.
- **Choice:** (d) mix, all inside the repo:
  - `modules/jrpg/` C++: `core/` pure logic with **no Node/scene/UI dependency** (data model, game state,
    interpreter, battle math, movement/passability, save, integer RNG) shared by single-player client and
    MMO servers; `runtime/` client nodes and UI building blocks (window frame, selectable list, message
    renderer, gauges, sprite helpers); `editor/` workspace and editors (editor builds only).
  - Screens (title, menus, battle layout, shop, name input, options, save/load, later login/char select,
    chat and social UIs) as **GDScript + .tscn managed addon** `res://addons/talewright/`, embedded in the
    binary and copied by setup; each file version+hash stamped; an upgrade tool overwrites untouched files
    and lists user-modified ones.
  - `modules/mmo/` (D-016+), `modules/claude_bridge/` (D-012).
  - `talewright/` top-level directory for fork-only tools, samples, docs, licenses.
- **Why:** GDExtension cannot reach editor internals like `EditorNode`/`EditorMainScreen`
  (not ClassDB-registered, `editor/register_editor_types.cpp`; `core/extension/extension_api_dump.cpp:917-941`);
  modules are first-class, documented, testable with doctest and disable cleanly. GDScript screens give
  RPG Maker-style "patch anything" customization and fast UI iteration; C++ logic gives determinism, tests
  and server reuse.
- **Reversal:** build with `module_jrpg_enabled=no` (vanilla); or port screens to C++ nodes.

## D-005 — JRPG database model
- **Context:** needs editor integration, VCS-friendly files, stable IDs, versioning. Text resources keep
  existing sub-resource ids and only generate missing ones (`scene/resources/resource_format_text.cpp:1896-1935`);
  `Resource::set_scene_unique_id` accepts identifier characters (`core/io/resource.cpp:171-182`).
- **Options:** one file per entry; one `.tres` per table; custom JSON + ResourceFormatLoader.
- **Choice:** manifest `res://jrpg/database.tres` referencing one `.tres` per table (ExtResource); entries
  are sub-resources with deterministic ids (`Item_42`). **ID = slot** like RPG Maker (no reordering,
  "Change Maximum", clear instead of delete) so IDs are stable; references by int id; nested lists
  (traits, effects, learnings, actions, drops, members) stored as packed values, not sub-resources;
  `schema_version` per table + `_set()` migrations; the inspector edits only the selected entry; a
  reference scanner warns before truncating referenced entries.
- **Why:** reuse of Godot dependency tracking/UIDs/export conversion; minimal diffs; stable save files.
- **Reversal:** a converter to another format; the entry classes are format-independent.

## D-006 — Maps are scenes with a JRPGMap root
- **Context:** undo routing: node edits go to the per-scene history, standalone resources share one GLOBAL
  history (`editor/editor_undo_redo_manager.cpp:70-101`); the TileMap editor is not extensible
  (`editor/scene/2d/tiles/tile_map_layer_editor.cpp:4532`); `TileMap` builds internal layers with
  `INTERNAL_MODE_FRONT` (`scene/2d/tile_map.cpp:272`).
- **Options:** map as standalone Resource + own canvas; `.tscn` + stock 2D editor plugins; `.tscn` + own canvas.
- **Choice:** each map is a `.tscn` whose root `JRPGMap` node stores layer data (tile layers in
  tile_map_data format, shadow, region, properties, next event id) and builds internal TileMapLayers;
  events are `JRPGEvent` child nodes; any other Godot node can be added. The JRPG workspace has its own
  map canvas sharing the edited scene's World2D through a `MapEditSession` adapter; a brush stroke is one
  undo action with a compact patch; own autotile resolver over a baked atlas; a map cell cache compiler
  (passability/flags/regions) feeds movement, pathfinding and servers. Task 8.1 is a spike; if it fails,
  fall back to a 2D-editor plugin route (same data model).
- **Why:** per-map undo, save/dirty/tabs for free, F6 playtest, nothing Godot can do is lost.
- **Reversal:** fallback route above; data model unchanged.

## D-007 — Events, commands and the interpreter
- **Choice:** typed command Resources in a tree (branches own child lists), gated by a storage benchmark
  (task 3.3/5.x: 10k commands load < ~30 ms and re-save byte-identically, otherwise packed storage behind
  the same API). `JRPGInterpreter`: frame stack, waits, labels resolved to tree paths, instruction/jump
  budgets, serializable state (event/page/index-path) so saving mid-event works. Each command declares its
  **authority** (server/client/both). A **presentation port** abstracts messages, choices, number input,
  pictures, screen effects and audio: the local port renders in single-player; the network port sends
  requests to the client and suspends until reply (the rAthena `mes`/`select` model,
  `ro server/rathena/src/map/script.cpp:5225-5292`). Custom/plugin commands are script classes extending
  `JRPGEventCommand` (`_execute`) discovered via the global class list; `JRPGPluginRegistry` holds ordered
  hook scripts with `@export` parameters.
- **Why:** tree = structured editing and safe serialization; port + authority make the same events run
  offline and on MMO servers.
- **Reversal:** flat command list adapter; local-only port.

## D-008 — "RPG Maker mode" coexistence
- **Context:** `EditorNode::add_editor_plugin/remove_editor_plugin` are static (`editor/editor_node.h:778-779`);
  `EditorMainScreen::set_button_enabled` hides a main-screen button (`editor/editor_main_screen.cpp:106-112`);
  feature profiles are global, not per project (`editor/settings/editor_feature_profile.cpp:348`); the
  Project Manager has no template mechanism (`editor/project_manager/project_dialog.cpp:561-634`).
- **Choice:** a tiny always-on bootstrap plugin adds/removes the "JRPG" main-screen workspace at runtime
  from project setting `jrpg/enabled` (the only `GLOBAL_DEF`; everything else lives in the System
  resource). Project ▸ Tools ▸ "Set up JRPG Project" (idempotent) creates folders, database, input actions
  with gamepad bindings, managed addon, theme and original assets. No Project Manager patch. JRPG node
  classes appearing in the Create dialog of all projects is accepted as additive.
- **Why:** vanilla projects see no JRPG UI; zero core patches.
- **Reversal:** remove the bootstrap plugin; or add a Project Manager template (core patch).

## D-009 — Formulas
- **Context:** `Expression` parses `a.atk * 4 - b.def * 2` with inputs (`core/math/expression.h:253-256`),
  has no ternary/assignment, and is not sandboxed (script methods callable even with const_calls_only,
  `core/object/script_instance.cpp:57-58`).
- **Choice:** evaluate with `Expression` against whitelisted proxy objects (`a`, `b`, `v`) exposing read-only
  values, plus helper methods on the base instance (`iff(c,x,y)`, `rand`, clamp); a deny-list rejects
  dangerous utility calls at parse time; parse errors shown in the editor; runtime failure ⇒ 0 + logged
  warning. Formulas are trusted project content.
- **Reversal:** small custom formula parser behind the same evaluator interface.

## D-010 — Determinism and RNG
- **Context:** `RandomPCG` integer ops are exact but float ops vary (`core/math/random_pcg.h:75-135`).
- **Choice:** own PCG32 integer RNG in `jrpg/core`, integer battle math; float only for presentation.
- **Why:** golden-log tests and identical results on client and server.

## D-011 — Default assets
- **Choice:** all default assets generated by an in-repo deterministic script (original pixel art,
  synthesized SFX/BGM); licenses recorded in `talewright/assets/LICENSES.md`. CC0 third-party assets only
  if ever needed, with license recorded. No RTP, no RPG Maker text/code/data.
- **Reversal:** swap in licensed packs; record licenses.

## D-012 — Claude bridge design
- **Context:** the play process is the editor binary (`editor/run/editor_run.cpp:161`), so EDITOR-level
  init runs there (`main/main.cpp:3818-3825`) after `EngineDebugger::initialize` (`main/main.cpp:2300`);
  `can_build ⇒ env.editor_build` keeps a module out of templates (`modules/xatlas_unwrap/config.py:1-2`);
  no HTTP server exists in Godot; `TCPServer::listen` binds `*` by default (`core/io/tcp_server.h:44`);
  game output reaches the editor only as debugger messages (`editor/debugger/script_editor_debugger.cpp:561-593`);
  a game screenshot round trip exists (`scene/debugger/scene_debugger.cpp:529-567`); unfocused editor sleeps
  100 ms/frame (`editor/settings/editor_settings.cpp:554`). Prior art (Coding-Solo/godot-mcp,
  Godot-MCP-Native, beckett-godot-mcp, satelliteoflove/godot-mcp, GDAI, …) are addons/external processes.
- **Options:** reuse an existing addon; external stdio server + addon; native C++ module.
- **Choice:** native editor-only C++ module `modules/claude_bridge`: MCP Streamable HTTP with JSON
  responses (accepts the 2025-06-18 `initialize` flow and the 2026-07-28 stateless shape; GET/DELETE ⇒ 405),
  bound explicitly to 127.0.0.1, persistent 256-bit Bearer token (constant-time compare) + fixed port with
  fallback, written to `.godot/claude_bridge/connection.json`; Origin ⇒ 403, Host check, size/connection/
  timeout limits; non-blocking partial I/O polled on the main thread; pending requests with deadlines;
  re-entrancy "busy" guard; unfocused throttle disabled while a client is active
  (`EditorNode::set_unfocused_low_processor_usage_mode_enabled`, `editor/editor_node.cpp:7996`); editor
  screenshots after `Main::force_redraw`. Game side: debugger capture registered only when
  `!Engine::is_editor_hint() && EngineDebugger::is_active()`. `talewright/tools/bridge.py` is the in-session
  CLI (MCP servers cannot be hot-added to a running Claude Code session).
- **Why:** first-class, no external runtime, absent from exported games by construction.
- **Reversal:** disable via editor setting or `module_claude_bridge_enabled=no`.

## D-013 — Commit conventions
- **Choice:** one commit per task, message `"<task id>: <summary>"`, containing code + tracker updates.
  **No `Co-Authored-By` trailer** (owner's global CLAUDE.md overrides the harness default). Commit hashes are
  stamped into tasks.json by `tasks.py stamp` in the following commit (a commit cannot contain its own hash).

## D-014 — Godot multiplayer stack is not the MMO stack
- **Context:** SceneMultiplayer encodes per peer × per synchronizer each frame, has no spatial AoI, drops
  states above MTU (`modules/multiplayer/scene_replication_interface.cpp:809,832`), and its protocol "may
  change without notice" (`modules/multiplayer/doc_classes/SceneMultiplayer.xml:10`); a Godot team reported mass disconnects at
  ~80–100 clients (https://forum.godotengine.org/t/our-experience-building-an-mmo-tech-demo-with-godot-4-4-1-net-c/134348).
- **Choice:** own schema-generated protocol over `ENetConnection`/`ENetPacketPeer`
  (`modules/enet/enet_connection.h:112-133`; 4095 peers/host, DTLS, zstd) and WebSocket+TLS for web
  clients; SceneMultiplayer remains available for ordinary Godot games.
- **Reversal:** a `MultiplayerAPIExtension` adapter over the same protocol.

## D-015 — MMO topology (rAthena techniques, clean-room)
- **Context:** rAthena (GPL-3.0) splits login/char/map servers with one-time handoff keys, an online
  registry, per-player state machines and per-process scaling (study in
  `plan/research/rathena_techniques.md`). Copying GPL code would force the MIT fork to GPL.
- **Choice:** `login` (accounts, tickets), `world` (characters, persistence owner, social, online registry,
  zone directory), `zone` (simulation of many maps, channels, instances). All are headless processes built
  from this engine and sharing `jrpg/core`. Techniques reimplemented clean-room; no rAthena code copied.
  Weaknesses fixed: async DB, transactional saves, schema-generated messages, no hard server-count caps,
  256-bit HMAC tickets + transport encryption, proper reassembly + token buckets, session isolation,
  build-validate-swap reloads, hash maps not max-ID arrays, region-scheduled AI.
- **Reversal:** roles are modular; a gateway or external meta-service can be added.

## D-016 — MMO meta-services: own vs Nakama
- **Options:** own login/world; Nakama (Apache-2.0; OSS has no clustering, needs Go + Postgres/Cockroach);
  Colyseus (MIT, Node, beta Godot SDK); SpacetimeDB (BSL 1.1, single production instance).
- **Choice:** own services in the same stack; Nakama adapter is task 43 (`deferred`).
- **Why:** one language and toolchain, MIT-clean, seamless editor integration, no single-node ceiling.
- **Reversal:** implement task 43.

## D-017 — Persistence backends and password hashing
- **Choice:** DB abstraction with **SQLite** (vendored, public domain) for development/single host and
  **PostgreSQL via libpq** (PostgreSQL License, system library, build flag) for production. MySQL/MariaDB only
  through an optional dynamically loaded LGPL connector (`deferred`); MySQL Connector/C++ (GPL) avoided.
  Valkey (BSD) if a cache/pubsub is ever needed. Passwords: Argon2id (vendored reference implementation,
  CC0/Apache-2.0). Async DB worker threads; atomic versioned per-character transactions.
- **Reversal:** add backends behind the abstraction.

## D-018 — Movement model
- **Choice:** discrete cell steps with per-cell speed timers in all modes. Online JRPG mode: local tile-step
  prediction + server reconciliation. ARPG mode: client sends a destination, server runs A* with path caps,
  clients interpolate along announced paths (rAthena `unit.cpp:180-247,841-870`, `path.cpp:337-399`).

## D-019 — Battle systems
- **Choice:** shared C++ battle core; systems: MZ turn-based, TPB (active/wait), and a real-time action
  (ARPG) system usable offline and online. Online turn-based battles are instanced on the zone server
  (client waits for results); ARPG combat is server-timed.

## D-020 — Legal guardrails
- **Choice:** RPG Maker behaviour reimplemented from public documentation only (paraphrased inventory,
  no verbatim text, no RTP, no file formats); the RPG Maker project importer is task 45 (`deferred`). No
  Gravity/Nintendo data or protocols. GPL/AGPL projects are design references only; FishNet code is not
  read (its license forbids competing implementations).

## D-021 — Bridge epic starts before the research/decision epics finish
- **Context:** epic 4 (Claude bridge) was seeded with dependencies on epics 1 and 3; epic 3 waits for the RPG Maker
  inventory (epic 2), which runs in parallel research agents. The bridge design is already fixed (D-012) and does not
  depend on RPG Maker research.
- **Options:** keep the dependency (idle time while agents research); depend only on epic 1 (toolchain baseline).
- **Choice:** epic 4 depends only on epic 1. Epics 2/3 continue in parallel; epic 5 still depends on epics 3 and 4.
- **Why:** no idle time; the bridge is the verification tool for everything after it.
- **Reversal:** restore `"dependencies": ["1", "3"]` on task 4.

## D-022 — Behaviour the official documentation does not specify
- **Context:** the official MZ help leaves some mechanics unstated (escape formula, critical multiplier, damage
  step order, EXP curve formula, save contents…; see `plan/research/rpgmaker_inventory.md`). The legal rule
  forbids learning them from RPG Maker's code, and the "never guess" rule forbids presenting guesses as parity.
- **Options:** copy community-reverse-engineered values; leave unimplemented; define our own documented behaviour.
- **Choice:** Talewright defines its own behaviour for each gap, documents it in `talewright/docs/` and in the
  relevant spec, makes it data-driven/overridable (system settings or formula fields), and marks the gap-map row
  "parity: functional, exact numbers not verified".
- **Why:** functional parity without copying or guessing; users can tune values to taste.
- **Reversal:** change the documented defaults; the values are data, not code.

## D-015a — Amendment: "independent reimplementation", not "clean-room"
- **Context:** the epic 2 review noted that the same agents read the rAthena study and implement the MMO servers, so
  "clean-room" (separate spec writers and implementers) overstates the process.
- **Choice:** call it an *independent reimplementation*. Rules: implement from Talewright's own behaviour-level specs in
  `plan/specs/`; never transcribe rAthena code; never reuse rAthena packet ids/layouts, opcodes, inter-server message
  numbers, YAML schemas or script syntax/variable sigils; techniques (algorithms, state machines) are reimplemented in
  Talewright's own structure and naming.
- **Reversal:** a true clean-room process would require separate spec writers and implementers.

## D-023 — No monster-capture template without legal review
- **Context:** Nintendo and The Pokémon Company sued Pocketpair (Palworld) for patent infringement on 2024-09-18
  (https://www.nbcnews.com/tech/nintendo-pokemon-company-palworld-pocketpair-lawsuit-rcna171817); status not verified.
- **Choice:** Talewright provides generic systems (actors, battles, events, items) with which users can build any game, but
  ships no monster-capture template/sample; such a template needs counsel review first. Original assets and UI only.
- **Reversal:** add the template after review.

## D-024 — MMO go/no-go spikes gate the MMO epics
- **Context:** epic 2 review: capacity is unmeasured; feasibility is conditional.
- **Choice:** epic 14 starts with three spikes (1000-peer raw ENet loopback; 1000-entity zone tick prototype with AoI;
  handoff chaos test). Failing spikes block dependent MMO epics with measured reasons and trigger a design revision
  (e.g. TCP/WebSocket transport behind the same protocol layer).
- **Reversal:** none needed; the spikes are cheap insurance.
