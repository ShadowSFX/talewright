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
- **Options:** GitHub fork via `gh repo fork`; local clone with an `upstream` remote only; a new standalone repository.

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
  Talewright palette-index format — spec 05, amended in 3.5.2 instead of Godot's tile_map_data bytes, so atlas
  re-layouts never rewrite maps — shadow, region, properties, next event id) and builds internal TileMapLayers;
  events are data (`JRPGEventData` inside a core `JRPGMapData` built-in resource, amended in 3.5.3 so core and
  servers need no scene nodes and Scene-dock duplication cannot clone event ids; the runtime builds internal
  `JRPGEvent` nodes and users attach nodes with `JRPGEventAnchor`); any other Godot node can be added. The JRPG workspace has its own
  map canvas sharing the edited scene's World2D through a `MapEditSession` adapter; a brush stroke is one
  undo action with a compact patch; own autotile resolver over a baked atlas; a map cell cache compiler
  (passability/flags/regions) feeds movement, pathfinding and servers. Task 8.1 is a spike; if it fails,
  fall back to a 2D-editor plugin route (same data model).
- **Why:** per-map undo, save/dirty/tabs for free, F6 playtest, nothing Godot can do is lost.
- **Reversal:** fallback route above; data model unchanged.

## D-007 — Events, commands and the interpreter
- **Choice:** typed command Resources in a tree (branches own child lists), gated by a storage benchmark
  (task 9.2.1: 10k commands load < ~30 ms and re-save byte-identically, otherwise packed storage behind
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
- **Context:** RPG Maker events are pages of command lists with flow control, waits and dialog that must work in single-player and, for the MMO toolkit, on a server that talks to a client (inventory RM-EVT/CMD/CMD2; rAthena dialog model).
- **Options:** flat command list with indentation (RPG Maker style); typed command resources in a tree; a scripting language (GDScript per event); a bytecode VM compiled from a tree.

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
- **Options:** always-visible JRPG tab; tab shown only when a project setting is on; editor feature profile; separate editor binary.

## D-009 — Formulas
- **Context:** `Expression` parses `a.atk * 4 - b.def * 2` with inputs (`core/math/expression.h:253-254`), supports
  `[]` indexing (`core/math/expression.cpp:866-884`), resolves Godot utility functions before identifiers
  (`:485`), has no ternary/assignment, and is not sandboxed (script methods callable even with const_calls_only,
  `core/object/script_instance.cpp:57-58`).
- **Options:** Godot `Expression`; a custom formula parser; GDScript snippets per formula.
- **Choice:** evaluate with `Expression` (`const_calls_only = true`) against read-only proxy objects `a`, `b` and the
  variables Array `v` (so the documented `v[n]` works), plus helpers on the base instance (`iff`, `roll`). An
  **allow-list** of identifiers (inputs, proxy members, the deterministic utilities `abs/min/max/clamp/floor/ceil/
  round/sqrt/sign`, the helpers, `true/false`) is checked lexically before parsing; anything else is a parse error.
  Runtime failure ⇒ 0 + logged warning. Formulas are trusted project content. Details: spec 04.
  (Amended in 3.5.1: was a deny-list; an allow-list also excludes non-deterministic utilities such as `randi`.)
- **Why:** reuses a tested evaluator and keeps formulas data-driven and editable; the allow-list keeps results
  deterministic across platforms and away from engine calls without writing a parser up front.
- **Reversal:** small custom formula parser behind the same evaluator interface.

## D-010 — Determinism and RNG
- **Context:** `RandomPCG` integer ops are exact but float ops vary (`core/math/random_pcg.h:75-135`); Godot vendors
  the PCG32 minimal C implementation (`thirdparty/misc/pcg.h:12-14`, Apache-2.0); the engine already compiles with
  `-ffp-contract=off` / `/fp:strict` (`platform/*/detect.py`), so IEEE basic operations are reproducible but libm
  transcendental functions are not.
- **Options:** `RandomNumberGenerator`; own RNG over the vendored PCG32; float math everywhere.
- **Choice:** `JRPGRandom` in `jrpg/core` wrapping the vendored PCG32 with an integer-only API, one saved stream per
  purpose (battle, map AI, encounters, loot, misc); battle math in `int64` with rates in basis points; floats only
  inside formula evaluation (converted once) and for presentation; fixed 60 Hz simulation steps (spec 03/04).
- **Why:** golden-log tests and identical results on client and server; separate streams keep a battle's results
  independent of unrelated map activity.
- **Reversal:** swap the RNG implementation behind the jrpg/core RNG interface; golden logs would be regenerated.

## D-011 — Default assets
- **Choice:** all default assets generated by an in-repo deterministic script (original pixel art,
  synthesized SFX/BGM); licenses recorded in `talewright/assets/LICENSES.md`. CC0 third-party assets only
  if ever needed, with license recorded. No RTP, no RPG Maker text/code/data.
- **Reversal:** swap in licensed packs; record licenses.
- **Context:** Default assets are required for a usable template and samples; RPG Maker RTP assets are proprietary and must not be copied (legal rule).
- **Options:** generate originals with a deterministic script; bundle CC0 packs (e.g. Kenney, https://kenney.nl/support); draw by hand.
- **Why:** generated originals avoid all third-party license tracking, are reproducible and can be regenerated in other sizes/styles; CC0 stays possible later.

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
  bound explicitly to 127.0.0.1, persistent 256-bit Bearer token (constant-time compare) + fixed port
  (fallback to the next ports only with `claude_bridge/port_fallback`, default off — amended in 4.13 after
  the epic 4 review: a silent fallback would let a squatter on 47700 receive the token, or a second editor
  be driven by mistake), written to `.godot/claude_bridge/connection.json`; the token is per user (0600),
  not per editor, so a `claude mcp add` registration stays valid across editor restarts — with the
  no-fallback default, a second editor fails loudly instead of taking another port; client-supplied
  values never reach `str_to_var` unless they pass a lexical literal allow-list (no `Object(...)`/
  `Resource(...)`, which would construct objects and run code); Origin ⇒ 403, Host check, size/connection/
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
- **Context:** The request asks for one commit per task containing code and tracker updates, prefixed with the task id; the owner's global CLAUDE.md forbids Co-Authored-By trailers.
- **Options:** commit hash written in the same commit (impossible: a commit cannot contain its own hash); amend after commit (rewrites history); stamp in the next commit.
- **Why:** stamping in the next commit keeps history append-only and still records every hash.
- **Reversal:** none needed; hashes can always be recovered with `git log --grep '^<id>: '`.

## D-014 — Godot multiplayer stack is not the MMO stack
- **Context:** SceneMultiplayer encodes per peer × per synchronizer each frame, has no spatial AoI, drops
  states above MTU (`modules/multiplayer/scene_replication_interface.cpp:809,832`), and its protocol "may
  change without notice" (`modules/multiplayer/doc_classes/SceneMultiplayer.xml:10`); a Godot team reported mass disconnects at
  ~80–100 clients (https://forum.godotengine.org/t/our-experience-building-an-mmo-tech-demo-with-godot-4-4-1-net-c/134348).
- **Choice:** own schema-generated protocol over `ENetConnection`/`ENetPacketPeer`
  (`modules/enet/enet_connection.h:112-133`; 4095 peers/host, DTLS, zstd) and WebSocket+TLS for web
  clients; SceneMultiplayer remains available for ordinary Godot games.
- **Reversal:** a `MultiplayerAPIExtension` adapter over the same protocol.
- **Options:** SceneMultiplayer + MultiplayerSynchronizer; a `MultiplayerAPIExtension`; own protocol over ENet/WebSocket.
- **Why:** SceneMultiplayer has no spatial interest management, encodes per peer, and its wire protocol is internal; an own schema-generated protocol is versionable and can be shared with non-Godot tools.

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
- **Options:** single monolithic server process; rAthena-style login/char/map split; gateway + stateless workers; an external framework (Nakama, Colyseus).
- **Why:** the split is proven at scale by rAthena (owner's own server), isolates failures, scales zones horizontally and maps cleanly onto Talewright's editor (maps → zones).

## D-016 — MMO meta-services: own vs Nakama
- **Options:** own login/world; Nakama (Apache-2.0; OSS has no clustering, needs Go + Postgres/Cockroach);
  Colyseus (MIT, Node, beta Godot SDK); SpacetimeDB (BSL 1.1, single production instance).
- **Choice:** own services in the same stack; Nakama adapter is task 43 (`deferred`).
- **Why:** one language and toolchain, MIT-clean, seamless editor integration, no single-node ceiling.
- **Reversal:** implement task 43.
- **Context:** Login, characters, social systems and persistence must exist for any MMO; Nakama (Apache-2.0) offers many of them (research note 07).

## D-017 — Persistence backends and password hashing
- **Choice:** DB abstraction with **SQLite** (vendored, public domain) for development/single host and
  **PostgreSQL via libpq** (PostgreSQL License, system library, build flag) for production. MySQL/MariaDB only
  through an optional dynamically loaded LGPL connector (`deferred`); MySQL Connector/C++ (GPL) avoided.
  Valkey (BSD) if a cache/pubsub is ever needed. Passwords: Argon2id (vendored reference implementation,
  CC0/Apache-2.0). Async DB worker threads; atomic versioned per-character transactions.
- **Reversal:** add backends behind the abstraction.
- **Context:** Godot has no database driver (research note 05); the MMO servers need durable, transactional storage and safe password storage; licenses must stay compatible with an MIT fork.
- **Options:** SQLite; PostgreSQL (libpq); MySQL/MariaDB (GPL/LGPL connectors); embedded key-value stores; a document DB.
- **Why:** SQLite is public domain and zero-config for development; libpq is permissively licensed and PostgreSQL is transactional; Argon2id is the current recommended password hash.

## D-018 — Movement model
- **Choice:** discrete cell steps with per-cell speed timers in all modes. Online JRPG mode: local tile-step
  prediction + server reconciliation. ARPG mode: client sends a destination, server runs A* with path caps,
  clients interpolate along announced paths (rAthena `unit.cpp:180-247,841-870`, `path.cpp:337-399`).
- **Context:** JRPG movement is grid-based; online play needs a server-authoritative model; RPG Maker and rAthena both step characters cell by cell with speed-based timing.
- **Options:** physics-based free movement (CharacterBody2D); tween-based cell movement on the client only; cell steps with per-cell timers shared by client and server.
- **Why:** one movement model offline and online removes divergence; cell steps make passability, triggers and server validation exact and cheap.
- **Reversal:** a free-movement controller can be added as an alternative character controller.

## D-019 — Battle systems
- **Choice:** shared C++ battle core; systems: MZ turn-based, TPB (active/wait), and a real-time action
  (ARPG) system usable offline and online. Online turn-based battles are instanced on the zone server
  (client waits for results); ARPG combat is server-timed.
- **Context:** MZ offers turn-based and time-progression battles (inventory RM-BTL, RM-NEW-002); the owner also wants ARPG (Ragnarok-like) and PokeMMO-like online battles.
- **Options:** MZ systems only; MZ systems + real-time action system; separate engines per mode.
- **Why:** one battle core with pluggable systems keeps formulas, states and data shared across modes and online.
- **Reversal:** systems are selected per project in System settings; unused systems can be disabled.

## D-020 — Legal guardrails
- **Choice:** RPG Maker behavior reimplemented from public documentation only (paraphrased inventory,
  no verbatim text, no RTP, no file formats); the RPG Maker project importer is task 45 (`deferred`). No
  Gravity/Nintendo data or protocols. GPL/AGPL projects are design references only; FishNet code is not
  read (its license forbids competing implementations). Asset and data layouts are Talewright's own (47-tile
  blob autotile sheets, one-character sheets with clockwise rows, own tile-flag bits, own direction and speed
  encodings; spec 05); other layouts can be imported only through explicit import settings, never through a
  built-in reader of another engine's format (amended in 3.5.2 after the epic 3 review).
- **Context:** RPG Maker is proprietary; Pokémon/Ragnarok are third-party IP; GPL code cannot enter the MIT fork.
- **Options:** allow importing/copying RPG Maker data and assets; reimplement from documentation only.
- **Why:** avoids copyright, trademark and license contamination while still reaching functional parity.
- **Reversal:** only after legal review (e.g. the deferred importer, task 45).

## D-021 — Bridge epic starts before the research/decision epics finish
- **Context:** epic 4 (Claude bridge) was seeded with dependencies on epics 1 and 3; epic 3 waits for the RPG Maker
  inventory (epic 2), which runs in parallel research agents. The bridge design is already fixed (D-012) and does not
  depend on RPG Maker research.
- **Options:** keep the dependency (idle time while agents research); depend only on epic 1 (toolchain baseline).
- **Choice:** epic 4 depends only on epic 1. Epics 2/3 continue in parallel; epic 5 still depends on epics 3 and 4.
- **Why:** no idle time; the bridge is the verification tool for everything after it.
- **Reversal:** restore `"dependencies": ["1", "3"]` on task 4.

## D-022 — Behavior the official documentation does not specify
- **Context:** the official MZ help leaves some mechanics unstated (escape formula, critical multiplier, damage
  step order, EXP curve formula, save contents…; see `plan/research/rpgmaker_inventory.md`). The legal rule
  forbids learning them from RPG Maker's code, and the "never guess" rule forbids presenting guesses as parity.
- **Options:** copy community-reverse-engineered values; leave unimplemented; define our own documented behavior.
- **Choice:** Talewright defines its own behavior for each gap, documents it in `talewright/docs/` and in the
  relevant spec, makes it data-driven/overridable (system settings or formula fields), and marks the gap-map row
  "parity: functional, exact numbers not verified". Each own rule is chosen deliberately on stated design grounds
  and must not reproduce community-known engine internals (amended in 3.5.1 after the epic 3 review found spec 04
  mirroring them); battle rules and their reasons are listed in `talewright/docs/battle_defaults.md`.
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
- **Options:** keep the 'clean-room' label; split spec writers from implementers; rename and add rules.
- **Why:** accurate wording plus concrete rules is honest and enforceable without a second team.

## D-023 — No monster-capture template without legal review
- **Context:** Nintendo and The Pokémon Company sued Pocketpair (Palworld) for patent infringement on 2024-09-18
  (https://www.nbcnews.com/tech/nintendo-pokemon-company-palworld-pocketpair-lawsuit-rcna171817); status not verified.
- **Choice:** Talewright provides generic systems (actors, battles, events, items) with which users can build any game, but
  ships no monster-capture template/sample; such a template needs counsel review first. Original assets and UI only.
- **Reversal:** add the template after review.
- **Options:** ship a monster-capture sample; ship generic systems only; ship nothing creature-related.
- **Why:** generic systems let users build such games while Talewright itself avoids shipping a mechanic under active patent litigation.

## D-024 — MMO go/no-go spikes gate the MMO epics
- **Context:** epic 2 review: capacity is unmeasured; feasibility is conditional.
- **Choice:** epic 14 starts with three spikes (1000-peer raw ENet loopback; 1000-entity zone tick prototype with AoI;
  handoff chaos test). Failing spikes block dependent MMO epics with measured reasons and trigger a design revision
  (e.g. TCP/WebSocket transport behind the same protocol layer).
- **Reversal:** none needed; the spikes are cheap insurance.
- **Options:** build the MMO stack directly; prototype first with go/no-go gates.
- **Why:** cheap measurements before large investment; failures change the design early.

## D-025 — Online authority for event pages and variable scopes
- **Context:** single-player page conditions read one global state; online, switches, variables and self switches
  belong to someone, and evaluating every event for every player on every change would cost players × events
  (epic 3 review, finding 6). rAthena maps variable scopes to storage by prefix (rathena_techniques D5).
- **Options:** evaluate all pages on the server per player; make all event state world-global; scope each kind of
  data and split events into per-player and shared.
- **Choice:** every switch and variable declares a scope in System data — `character` (default), `account`,
  `world`, `instance` (one map instance, not persisted) or `temp` (per character, not persisted); self switches are
  per character unless the event is marked `shared`. Events whose conditions read only character-scoped data are
  per-player: clients evaluate their own active page, the zone evaluates a page only when a player triggers it and
  validates the trigger; such events may only use fixed movement. Events reading only world/instance data are
  shared: evaluated once per zone and replicated. The editor flags mixed events. Details: spec 03, spec 07.
- **Why:** quest-style NPCs need per-player state; crowds and doors need shared state; per-trigger evaluation keeps
  server cost proportional to player actions.
- **Reversal:** scopes are data; a project can mark everything `world` to get one shared state, at the cost of
  per-player quests.

## D-026 — Localization keys
- **Context:** database names and event text were plain strings, so source text could not be a stable translation
  key, servers would send rendered text, and saves stored resolved names (epic 3 review, finding 12). Godot provides
  `TranslationServer` and `EditorTranslationParserPlugin` (`editor/translations/editor_translation_parser.h:37`).
- **Options:** source text as key (Godot's default for UI); generated keys per text field; external string tables.
- **Choice:** stable keys. Database fields use `<Kind>_<id>.<field>` (stable because ID = slot, D-005). Every
  text-bearing event command gets a `text_key` when created — a short random id scoped by owner (`map3.ev7.k9f2c1`,
  `ce12.k…`, `troop4.p2.k…`) that survives text edits and command moves. The source-language text stays in the
  resource as the fallback; a Talewright `EditorTranslationParserPlugin` exports keys and source text to POT/CSV.
  Online, messages carry keys plus arguments for control codes and clients expand and translate them. Saves store
  only name overrides set by events.
- **Why:** translations survive edits; one content pack serves every language; servers stay language-neutral.
- **Reversal:** keys are data; a project could map them to source text at export time.

## D-027 — Fork files pass upstream's CI static checks
- **Context:** every push runs the GitHub "Static checks" job: prek with `.pre-commit-config.yaml` on the changed
  files (`.github/workflows/static_checks.yml`). It failed on every push so far: scripts with shebangs not marked
  executable, mypy generics, codespell (configured with `en-GB_to_en-US`, `pyproject.toml` `[tool.codespell]`),
  unowned fork paths in `validate-codeowners`; the `copyright-headers` hook also replaces any C++ header with Godot's.
- **Options:** disable the workflow in the fork; exclude fork paths from all hooks; make fork files comply and
  exempt them only where compliance would be wrong.
- **Choice:** comply: US spelling in all fork files, executable bit on scripts with shebangs, typed generics, and
  `codespell:ignore` markers where a flagged token is correct (labels such as `SAV`, the PostgreSQL URL, a quoted
  file name). Two upstream-file patches (ledger rows 1–2): fork modules are excluded from `copyright-headers` (their
  Talewright header is checked by `add_header.py --check`) and fork paths get CODEOWNERS entries. Before every push,
  run `python talewright/tools/static_checks.py` (same hooks on the files the push brings, plus fork checks) and
  review any file a hook rewrote: codespell's automatic fixes turned a Python variable into the keyword `and` and a C++ variable into `end`, and changed
  labels once and must never be committed unreviewed.
- **Why:** keeps the fork's CI green so real regressions are visible, with the smallest upstream diff.
- **Reversal:** revert ledger rows 1–2; the fork's own checks keep working locally.

## D-028 — Fork CI: Linux jobs that upstream sizes break
- **Context:** CI run 38489070426 (push of 5.2.1) failed two Linux jobs for size reasons, not code faults.
  "Editor w/ Mono": the godot-cpp test archives ~1100 objects with one `ar` command (`sh -c`), and the fork's
  workspace path `/home/runner/work/talewright/talewright/` (10 bytes longer per object than upstream's) made it
  131 524 bytes, over Linux's 131 072-byte single-argument limit. "Editor with doubles and GCC sanitizers": with the
  fork modules the unoptimized ASan+UBSan binary passes 2 GiB, and mold fails with an `.eh_frame` relocation out of
  range by 512 KiB. Upstream's own binary sits just under that limit.
- **Options:** rename the repository; disable the jobs; trim code; build godot-cpp from a shorter path and leave the
  fork modules out of the one oversized job.
- **Choice:** the last. The godot-cpp action moves its checkout to `$RUNNER_TEMP` (34-byte prefix, shorter than
  upstream's 40) before building; the GCC double-precision sanitizer job passes
  `module_claude_bridge_enabled=no module_jrpg_enabled=no module_mmo_enabled=no` (unknown module flags are ignored by
  SCons until the module exists). Ledger rows 4–5.
- **Why:** both jobs keep testing what they were built for (godot-cpp compatibility; double precision under GCC
  sanitizers), and the fork modules stay covered by the clang ASan/UBSan and ThreadSanitizer jobs, which passed with
  them in the same run.
- **Reversal:** revert ledger rows 4–5; if upstream later shrinks that build, re-enable the fork modules in it.
- **Follow-up (run 38493196969):** clang builds on every other platform then failed on
  `-Wtautological-constant-out-of-range-compare` (an enum argument checked against a bound outside the enum's range)
  that MSVC never reports. Enum arguments are now cast to `int` in `ERR_FAIL_INDEX`, and
  `talewright/tools/clang_check.py` compiles every fork translation unit (plus `tests/test_main.cpp` for the doctest
  headers) with clang's diagnostics and Godot's clang warning set through Visual Studio's `clang-tidy` and the
  `compile_commands.json` written by the `editor` build profile. `static_checks.py` runs it before every push.
  GCC-only warnings (`-Wlogical-op`, `-Wduplicated-branches`, ...) still need CI: no GCC toolchain is installed.
- **Follow-up 2 (run 38501753154):** every template job failed because templates also compile the doctests and a
  test header used `ClassDB` without including it (editor builds include it indirectly), and the Linux Mono job's
  "Style checks via clangd-tidy" step (`.github/actions/clangd-tidy/action.yml`: every changed C/C++ file analyzed on
  its own, default `--fail-on-severity hint`, so unused includes fail too) rejected test headers that did not include
  what they use. `clang_check.py` now also compiles `tests/test_main.cpp` without `TOOLS_ENABLED` (template view of
  the tests), and `static_checks.py` runs the same pip `clangd-tidy` on the changed C/C++ files with the local
  compile database (`PYTHONUTF8=1`; `// IWYU pragma: keep` where an include is needed only for a conversion).

## D-029 — Saved JRPG data always records its version; stored defaults are frozen
- **Context:** Godot's text saver skips any property equal to the class default
  (`scene/resources/resource_format_text.cpp:1986-1990`, defaults from a fresh instance, `core/object/class_db.cpp:2184-2198`).
  The epic 5 review (finding 1) showed that `schema_version`, defaulting to the current version, was never written, so
  the first real migration would misfire; finding 6 showed the same for starter values (types, terms, battle rules,
  every entry field): a build that changed a default would silently change every project that kept it.
- **Options:** always-store tricks (synthetic properties, null defaults); move starter content out of class defaults
  into a setup-time factory; keep the defaults and forbid changing them without a migration.
- **Choice:** `schema_version` class default is -1 ("unstamped", never a real version) and any content change stamps
  the current version, so every saved table and manifest records it (an explicit 0 stays a real older version for
  migrations). Stored defaults stay as they are and are frozen: `test_jrpg_core_regressions.h` fingerprints the
  stored defaults of every persisted JRPG class (entries, troop pages, system, types, terms, rules, audio cues,
  tables, manifest); changing one fails the test until a `JRPGSchema` migration step writes the old value into data
  saved by the previous version and the fingerprint is updated.
- **Why:** no project files exist yet, so the stamping convention costs nothing now; freezing defaults keeps the
  starter content convenient (inspector-created resources are usable) and makes accidental rebalancing impossible.
- **Reversal:** to change a default, add the migration step described above; to drop the freeze, delete the test.

## D-030 — Modules add Claude bridge tools through a registry
- **Context:** D-012 asks every editor feature for an authoring API plus a bridge tool, but the bridge's tools were
  registered only inside `modules/claude_bridge` (`claude_bridge_plugin.cpp` `_register_tools`). JRPG (and later MMO)
  tools must not make either module depend on the other: each must build alone (`module_*_enabled=no`).
- **Options:** a generic "call any bound method" tool (too broad: arbitrary engine calls from a client); the bridge
  depending on jrpg/mmo; a registry the bridge exposes and other modules feed when the bridge is built.
- **Choice:** `ClaudeBridgeRegistry::add_tool_provider(fn)` (`modules/claude_bridge/claude_bridge_registry.h`): a
  module registers a provider at EDITOR initialization inside `#ifdef MODULE_CLAUDE_BRIDGE_ENABLED` (from
  `modules/modules_enabled.gen.h`, the pattern `modules/openxr/register_types.cpp:47,93` uses for gltf) and removes
  it at uninitialization; the bridge plugin, created later by the editor, calls every provider after its own tools.
  Tools stay narrow (one per authoring operation) and validate their arguments like the bridge's own tools.
- **Why:** explicit, reviewable tool surface; modules stay optional; no new upstream patch.
- **Reversal:** drop the registry and register tools from the bridge with its own `#ifdef MODULE_JRPG_ENABLED`.

## D-031 — The bridge can operate editor controls like a user
- **Context:** editor UI tasks (database view, entry form, trait/effect dialogs; later the map and event editors) must
  be verified through the bridge, but its tools only read state, take screenshots and call authoring APIs. Dialogs,
  buttons, typing in the form and picking from lists could not be exercised (tasks 6.4.1 and 6.4.2 needed tool
  options such as `show_dialog` just to see a dialog, and typing in the form stayed unverified).
- **Options:** one-off "show this dialog" options per tool; OS-level mouse/keyboard injection into the editor window
  (`DisplayServer`/`Input` events: coordinates, focus and timing make it fragile); tools that find editor controls and
  act on them through their own signals and setters, the way a click or keystroke would.
- **Choice:** `editor_ui_find` (visible controls under the editor root or a given path, matched by text, tooltip,
  class or accessibility name; returns paths, classes, texts, states) and `editor_ui_act` (press a button, toggle,
  set a LineEdit/TextEdit/SpinBox value and submit, select an OptionButton/ItemList/Tree/TabBar item by text or
  index, activate a list item as a double-click would, confirm or cancel a dialog). Only visible, enabled controls
  inside the editor's own windows are reachable, so the tools can do no more than a user at the editor. Since 6.7.3
  (epic 6 review) "reachable" also excludes viewports that take no input (the edited scene) and everything behind
  an open modal dialog; presses go through the button's input handling and selections through each widget's click
  path, so the editor sees exactly what a click would give it.
- **Why:** robust (no coordinates), general (every future editor UI is testable), and narrow compared with the
  rejected "call any bound method" tool of D-030: it reaches what the user can already click, nothing else.
- **Reversal:** remove the two tools; UI checks fall back to screenshots and authoring-API calls.

## D-032 — Collapse effect values
- **Context:** the Collapse Effect trait (RM-TRT-025) picks how a defeated enemy disappears; the public help names
  only the Boss option.
- **Choice:** Talewright's own list, stored as the trait's data_id: 0 Normal (fades out), 1 Boss (long effect with
  the Boss Collapse sounds), 2 Instant (vanishes at once), 3 Stays (remains on screen). `JRPGTraits::CollapseEffect`.
- **Why:** covers the documented Boss case plus the common needs; the battle scene (epic 11) implements them.
- **Reversal:** renumbering needs a `JRPGSchema` migration of saved traits.

## D-033 — Enemy action condition values
- **Context:** an enemy action pattern stores its condition's values in `param1`/`param2` (spec 02). The public help
  describes Turn as A + B·X but does not say what B = 0 means (RM-ENM-013, not verified), nor the ranges of the
  Party Level and Switch values (RM-ENM-017/018).
- **Choice:** Turn: `param1` is the first turn A, `param2` the repeat B, and B = 0 means turn A only. HP and MP:
  `param1`..`param2` is the band in basis points of the maximum, inclusive. State: `param1` is the state ID. Party
  Level: `param1` is the level, met at that level or above (1..99). Switch: `param1` is the switch ID (met while
  on). Unused values are 0. The database form shows these labels and units (`JRPGRecordSpecs::Shape`, task 6.7.2).
- **Why:** a single meaning for the form, the bridge and the battle epic (12.x) to implement; B = 0 as "once" is the
  only reading that gives the value a use instead of an infinite loop of turn A.
- **Reversal:** a `JRPGSchema` migration of saved actions if the meaning changes; the form follows the shapes.

## D-034 — Tile size 48 pixels
- **Context:** the generated default tiles (task 7.1.1), the autotile templates and the character sheets need one
  size. The reference documents 48 × 48 pixel tiles (RM-AST rows in plan/research/rpgmaker/02_maps_tilesets_assets.md,
  "Tiles are 48×48 pixels"); Godot's TileSet takes any size (`scene/resources/2d/tile_set.h`, tile_size).
- **Options:** 16 or 32 px (retro), 48 px, a per-project size only.
- **Choice:** Talewright's default assets and templates use 48 × 48 px tiles; imported tilesets and sheets may use
  any size (the importer reads the tile size from the TileSet or the import settings).
- **Why:** assets made for the reference's standard import without scaling, 48 px reads well at 1280 × 720, and the
  generator can redraw everything at another size later.
- **Reversal:** change `TILE` in `talewright/tools/asset_gen/common.py` and regenerate; maps store palette indexes,
  not pixels.
