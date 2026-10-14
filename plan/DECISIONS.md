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
  saved by the previous version and the fingerprint is updated. The map list and map data follow the same rules
  (epic 8 review A5: stamped on every content change, migrated or warned about when loaded, fingerprinted).
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
- **Reversal:** maps store palette indexes, not pixels, so only the assets change: change `TILE` in
  `talewright/tools/asset_gen/common.py` and scale the painters' fixed 48 px geometry (texture cell sizes in
  `tiles.py`, edge margins, furniture and character coordinates, template marks) by `TILE / 48` before regenerating;
  until then the generator draws for 48 px only (epic 7 review, part A finding 8).

## D-035 — GCC diagnostics before every push (through WSL)
- **Context:** the epic 7 push failed CI's GCC builds (Linux Mono editor and templates, Windows MinGW template): an
  enum and a uint8_t in one conditional (`jrpg_autotile_resolver.cpp`, a doctest header) is a `-Wextra` warning in
  GCC and an error under dev_mode's `-Werror`, while MSVC (local builds) and clang (clang_check.py, D-028) accept it.
  Run 38541244575.
- **Options:** rely on CI; install MinGW on Windows; compile the fork's units with GCC in WSL (Ubuntu with g++ is
  installed on this machine).
- **Choice:** `talewright/tools/gcc_check.py` runs `g++ -fsyntax-only` with Godot's GCC warnings
  (SConstruct:962-1027) in WSL on every fork translation unit and on tests/test_main.cpp, with editor defines and
  again with template defines (no TOOLS_ENABLED, editor-only code left out), and fails on any diagnostic in fork code
  or any compile error; `static_checks.py` runs it (about 20 s), and a missing WSL/g++ is a skip that fails without
  `--allow-skips`.
- **Why:** catches GCC-only diagnostics before the push, the same way clang_check.py does for clang.
- **Reversal:** remove the call from static_checks.py; CI still reports the failure after the push.

## D-036 — Map scene spike passed: D-006's route kept
- **Context:** D-006 made maps scenes whose root `JRPGMap` holds a built-in `JRPGMapData`, with task 8.1 as a spike and
  a 2D-editor-plugin route as the fallback.
- **Options:** keep the scene route; fall back to the 2D-editor plugin route (same data model).
- **Choice:** keep the scene route. Spike (task 8.1.3, `plan/evidence/jrpg/8.1.3-*`, through the bridge): a map scene
  was created and opened, three paints went to the map scene's own undo history (the map node is the action's
  context; the built-in data resource routes to the same history, `editor/editor_undo_redo_manager.cpp:63-97`), undo
  and redo restored the cells exactly, no database file was marked unsaved, the scene saved, its `JRPGMapData` was
  read back from the file's SceneState without instancing, and the scene ran in a new game process showing the same
  tiles (editor and game screenshots); `jrpg_bridge_checks.py` round-trips a paint (80/80). The internal TileMapLayers
  are not saved with the scene (no owner).
- **Why:** every requirement of D-006 held on the first try; the fallback would only lose per-map undo and scene tabs.
- **Reversal:** the 2D-editor plugin route of D-006, with the same `JRPGMapData`.

## D-037 — Cell rules the documentation leaves open
- **Context:** the map cell cache (task 8.2) needs a passage for every cell and a stacking rule for every flag. The
  reference documents that the later layer decides between passable and impassable tiles, that a ☆ tile never blocks
  and an impassable tile under it wins (RM-MAP-020, RM-TIL-007), and that the top-most nonzero terrain tag wins
  (RM-TIL-013). It says nothing about a cell no tile decides, nor how ladder, bush, counter and damage stack.
- **Options:** an undecided cell open or blocked; flags from the deciding tile only, the top-most tile, or any tile.
- **Choice:** a cell where no tile decides (nothing painted, or only transparent and overhead tiles) is blocked;
  overhead, ladder, bush, counter and damage hold when any tile of the cell has them.
- **Why:** an unpainted cell has nothing to stand on, so walking into the void is never a surprise; a bush or damage
  decoration on an upper layer works over any ground without editing the ground tile.
- **Reversal:** `JRPGMapCellCache::_compile_cell` (one place) and the fixture table of its doctest.

## D-038 — Map properties are edited in the Inspector dock
- **Context:** task 8.3.3 asked for a map properties dialog in the workspace's Maps panel. A map's properties live in
  its scene (`JRPGMap.map_data`, D-006), so editing them needs the scene open, and undo belongs to that scene's
  history.
- **Options:** a dialog holding its own EditorInspector or form; or Properties opens the map and shows its
  `JRPGMapData` in the editor's Inspector dock (`EditorInterface::inspect_object`).
- **Choice:** the Inspector dock. The map's tileset is an ID picker there (JRPGInspectorPlugin), and an editor undo
  hook (`JRPGMapEditing::add_inspector_undo`, `EditorPlugin::add_undo_redo_inspector_hook_callback`) makes undo of a
  smaller size bring the cut cells back, as `jrpg_map_set` does.
- **Why:** one place to edit map data, the editor's own inspector (resources, audio cues, undo per scene, favorites)
  with nothing duplicated; the dock stays visible beside the JRPG workspace.
- **Reversal:** wrap an EditorInspector in a dialog opened by `JRPGMapTree::_inspect_map`; the undo hook stays.

## D-039 — Placing events on the map canvas comes with the event editor
- **Context:** task 8.4.3 listed an Events mode that creates, moves and deletes events on the canvas grid, but the
  event data model (`JRPGEventData`, task 9.1) belongs to epic 9, which depends on epic 8.
- **Options:** a stopgap event entry format now; or place events where the model and the event editor are built.
- **Choice:** event placement moves to task 9.7 (event editor UI). Events mode exists from 8.4.3 and holds what needs
  no event data: Set Start Position Here (8.4.3.2).
- **Why:** no throwaway format to migrate; one place builds events and their canvas handling.
- **Reversal:** move the placement criteria back into an epic 8 task once a minimal event model is wanted earlier.

## D-040 — What the automatic layer does
- **Context:** the map canvas's Auto layer (task 8.4.3.2) picks the layer a tile is drawn on (RM-MAP-022). The
  reference says Auto picks the layer while drawing, a cell keeps at most two upper tiles, a third drops the oldest and
  the tile already on top changes nothing (RM-MAP-019); it does not say which numbered layer holds what. Talewright's
  tiles have kinds (autotile, wall, plain), not the reference's sheet slots.
- **Options:** keep explicit layers only; map kinds to layers; also guess base/decoration splits for autotiles.
- **Choice:** autotile and wall kinds replace the Ground tile; plain tiles stack on Objects and Upper (older below,
  the oldest drops out, the top tile again changes nothing); the eraser clears the cell's top-most tile. Ground
  Detail is reached by choosing it explicitly. Auto is the default layer choice.
- **Why:** the documented stacking rules, applied to the kinds the tileset knows; no flag a tileset would have to set.
- **Reversal:** `JRPGMapStroke::add_auto` (one function) and the layer picker's default.

## D-041 — How a game plays its maps (scene manager)
- **Context:** task 8.6.1 needs a runtime owner of the running game: the current map, transfers between maps
  (RM-CMD-037: fade through black, the default, or white; facing "Retain" by default) and map music (RM-MPR-009/010:
  autoplay BGM/BGS while the player is on the map; RM-AUD-010: the same track continues). The help is silent on a
  "no fade" option, other facings, fade timing, music on maps without autoplay, and saves whose map is gone.
- **Options:** switch the whole tree per map with Godot's `SceneTree::change_scene_to_file/packed`
  (`scene/main/scene_tree.h:427-429`), which frees the current scene and so needs autoloads for the game state and
  everything shown across maps; an autoload singleton; a node that instances map scenes under itself.
- **Choice:** `JRPGSceneManager` (runtime node, usually the game scene's root): holds the `JRPGGameState`, shows
  the current map as its first child (a `JRPGMap` scene instance found through the map list), owns internal BGM/BGS
  `AudioStreamPlayer`s and a fade `CanvasLayer`. `transfer(map_id, cell, direction = retain, fade = black)` checks
  the destination first (listed, loadable, a map, cell inside) and refuses with one warning, the player staying;
  with a fade the map changes at the darkest point (`FADE_STEPS` = 18 simulation steps each way, 0.3 s at 60 steps a
  second: counted in steps, so the map changes after the same steps at any frame rate; epic 8 review A7). The
  destination's scene is loaded when the transfer is asked for, synchronously, because the check needs its map data
  to refuse a bad destination before the fade starts (epic 8 review A13: a one-off load of a few hundred kilobytes for
  slice-size maps; a threaded preload, `ResourceLoader::load_threaded_request` during the fade-out with the check at
  the darkest point, is the reversal if large maps hitch). A new or loaded game creates its map from the scene before
  it replaces the running game, so a scene that fails leaves that game as it was (review A11). A transfer asked for
  while another is under way, its fade-in included, is refused (`ERR_BUSY`, one warning); the event interpreter's
  Transfer Player waits on `is_transferring()` / `transfer_finished` before going on (review A15, task 9.5.3).
  Talewright's own rules where the help is silent:
  `FADE_NONE` exists; the four facings besides Retain; a transfer within the current map keeps its instance (events
  keep their states) and still fades; a map without autoplay leaves the music playing; autoplay without a file
  stops it; the same track continues on transfers too; cue pan waits for the audio system (task 29:
  `AudioStreamPlayer` has no pan, `scene/audio/audio_stream_player.cpp:283-293`); `resume()` of a save whose map or
  cell is gone continues at the System start position with one warning. Autostart (a new game when the scene is
  ready) is deferred until after every `_ready()`, so a boot script can resume a save first.
- **Why:** the game state and global presentation (fade, music, later the player, windows) persist across maps
  without autoloads; the network port can drive the same `transfer()` online (D-007 authority: server).
- **Reversal:** the manager's API can sit on `change_scene_to_packed` instead; the rules above are each one place in
  `jrpg_scene_manager.cpp`.

## D-042 — How characters step between cells (JRPGMover)
- **Context:** task 8.6.2 needs the movement rules every character shares (player now, events and servers later):
  speed levels twice as fast each (RM-EVT-032, spec 05's table), dash one level up, passability from the cell cache.
  The help does not say when a moving character's position changes, whether a blocked step turns it, or how input
  during a step is handled.
- **Options:** keep a fractional position and collide continuously; or discrete cell steps whose logical cell
  changes at once and a drawn position that follows.
- **Choice:** core `JRPGMover`: the logical cell becomes the destination when a step starts (what events and other
  characters see); the drawn position leaves the previous cell evenly over `get_steps_per_cell(speed)` simulation
  steps (160 >> (speed - 1): 160, 80, 40, 20, 10, 5; dash = speed + 1, capped at 6); a step the cache does not allow
  only turns the character; a new direction waits until the step under way ends; `through` ignores blocked cells
  and sides but not the map's edges (looping axes wrap). The speed table is a core constant instead of System data
  fields (spec 05 said "all numbers are fields"): it is the documented 2x ratio with one base value, and servers
  and clients must agree on it.
- **Why:** discrete steps are deterministic on the 60 Hz clock, cheap for servers, and match the reference's
  tile-step feel; one shared class keeps offline and online movement identical.
- **Reversal:** `JRPGMover::get_steps_per_cell` (one function) could read System data instead.

## D-043 — The editor file system only with an editor
- **Context:** the full doctest run crashed intermittently in the workspace tests (8.6.1.2): the shell connected to
  `EditorFileSystem::get_singleton()`, which the constructor sets and the destructor never clears
  (`editor/file_system/editor_file_system.cpp:3806`, `3827-3833`); `modules/gdscript/tests/test_lsp.h` and
  `modules/gltf/tests/test_gltf_images.h` create and delete one, so later tests used freed memory. Seven other fork
  call sites (scan_changes, update_file, signal connections) carried the same risk.
- **Options:** patch upstream so the destructor clears the singleton (a core change for the ledger); or have fork
  code use the file system only while an `EditorNode` exists (it owns the file system and clears its own singleton,
  `editor/editor_node.cpp:9663`).
- **Choice:** `jrpg_editor_file_system()` (modules/jrpg/editor/jrpg_editor_file_system.h) returns the file system
  only with an EditorNode; the bridge's rescan tool checks EditorNode the same way; `static_checks.py` fails on a
  direct `EditorFileSystem::get_singleton()` in fork code.
- **Why:** no core patch; the editor behaves as before (an EditorNode always exists there), doctests never touch a
  dangling pointer.
- **Reversal:** if upstream clears the singleton in the destructor, the helper can return the singleton directly.

## D-044 — Player input, the game loop and the camera
- **Context:** task 8.6.2.2 puts the player on the map. The help does not say which of several held directions wins,
  whether the player moves during a transfer's fade, when a step counts, or where the camera sits on a map smaller
  than the screen.
- **Options:** fixed direction priority vs the direction pressed last; real-time movement per frame vs the simulation
  clock; Camera2D smoothing vs exact following.
- **Choice:** `JRPGSceneManager` runs the game loop on the game state's `JRPGSimClock` (60 steps a second whatever
  the frame rate, at most 4 per frame); each step advances the player's `JRPGMover`, then reads the direction
  actions: of those held, the one pressed last (tracked every step, so a direction pressed during a step wins when
  it ends); no new step while a transfer fades. A step counts (`JRPGGameState.increase_steps`) when the player
  arrives; the game state's position follows every step and turn. `JRPGPlayer` lives inside the current map (moved
  along on transfers) at `Z_CHARACTERS`, drawn at the cell's middle, with a `Camera2D` without smoothing, made the
  current camera on each map, whose limits are the map's edges, or one screen centered on the map on an axis where the
  map is smaller, or none on a looping axis (the looping display itself comes with epic 23). The map's area is taken
  through its transform and the screen through the camera's zoom (epic 8 review A12). Until sprites (8.7) it draws a
  marker showing its facing. The transfer fade also runs on the simulation steps (D-041).
- **Why:** deterministic movement (servers, replays), the reference's grid feel, small maps framed like the
  reference's centered small maps.
- **Amended (epic 10 review B4):** the player takes no new step while any control has the focus (a message, a list, a
  screen): the keys belong to the focused control then, as Godot's own `ui_*` navigation would also move between
  focusable controls. A HUD button that should not stop walking uses `FOCUS_NONE` or `FOCUS_CLICK`.
- **Reversal:** `JRPGSceneManager::_simulation_step` and `_update_camera_limits` hold these rules.

## D-045 — Character sheets: plain images by default, the importer for other layouts
- **Context:** spec 05 says character sheets import to `SpriteFrames` with their layout as import settings
  (task 8.7). Actors already reference their character as an image file (`*.png` path strings); the database
  forms, the bridge's `jrpg_db_set_image` and the starter assets treat those files as textures.
- **Options:** switch every character image to a SpriteFrames importer (changes the files' resource type under the
  forms and tools); cut sheets only at run time with a fixed layout (no other layouts); both.
- **Choice:** `JRPGCharacterSheet` (runtime) cuts `SpriteFrames` from a texture and a layout (frames, 4 or 8
  directions, grid and index, row order, y offset, object); a plain image gets the default layout (one character,
  4 x 4) when a character is drawn with it; the editor's `JRPGCharacterSheetImporter` ("Character Sheet" under
  Import As, priority 0.1 so the texture importer stays the default, `core/io/resource_importer.cpp:521-537`) saves
  the SpriteFrames of any other layout, which `load_frames` returns as they are.
- **Why:** the common case needs no import step and nothing that reads the images as textures changes; other
  layouts use Godot's own import settings, as the spec asks.
- **Reversal:** make the importer the default for a folder by tooling, or drop the texture path in `load_frames`.

## D-046 — How a character's walk is animated
- **Context:** spec 05 says column 0 is the standing pose and walking loops through all columns in order; it does
  not say how fast the frames change or where the feet go (task 8.7.2).
- **Options:** frames by time (a fixed rate while moving), or by distance walked.
- **Choice:** `JRPGCharacterSprite` advances one frame every half cell walked (`FRAMES_PER_CELL` = 2), starting on
  frame 1 when a walk begins, so a 4-frame sheet shows 1, 2, 3, 0 over two cells at any speed; standing shows
  frame 0. The distance walked comes from the mover (`JRPGMover.get_walk_distance()`: whole steps counted plus the
  step under way, reset when a simulation step passes without a step), not from positions on screen, so it is exact
  across looping edges and the same at any frame rate (epic 8 review A8). The frame's bottom edge sits on the cell's bottom edge, centered, raised by the sheet's y offset. The
  frames are set from the simulation, never played, so they match the steps exactly (and online, the server's).
- **Why:** stepping looks the same at every speed and frame rate; deterministic from the mover's position.
- **Reversal:** `FRAMES_PER_CELL` and `JRPGCharacterSprite::show_state`.

## D-047 — Autotiles drawn with Auto go over other ground (refines D-040)
- **Context:** building a map through the bridge (task 8.8) showed holes along the edges of the pond and the path:
  the generated water and dirt autotiles are overlay art whose edges are transparent, and D-040 put every autotile
  drawn with Auto on Ground, replacing the grass under it, so the edges showed the clear color.
- **Options:** make autotile art opaque (every autotile would need its own copy of each base terrain); draw a base
  under Ground; place autotiles drawn over other ground on Ground Detail.
- **Choice:** with Auto, an autotile or wall is the Ground tile where Ground is empty or of its kind (and then
  clears Ground Detail, so painting grass over a path removes the path), and goes on Ground Detail over other ground,
  which shows through its edges. The automatic fill compares the ground as it shows (Ground Detail over Ground). The
  eraser still clears the top-most tile. Explicit layers are unchanged.
- **Why:** terrain art with soft edges looks right without extra authoring; the base terrain stays a single fill.
- **Reversal:** `JRPGMapStroke::add_auto` and `read_auto_area`.

## D-048 — The doctests also run in a release template before each push
- **Context:** CI builds its Linux templates with `dev_mode=yes` (`.github/workflows/linux_builds.yml:12-14`, which
  turns tests on, `SConstruct:667-671`) and runs `--test` in them (`:243-247`). Those builds have no editor code, so
  a doctest that needs it (the 8.9.2 inherited-scene case used `PackedScene::GEN_EDIT_STATE_MAIN_INHERITED`, which
  refuses outside tool builds, `scene/resources/packed_scene.cpp:2509`) crashed the template's test run after the
  editor build had passed every test locally. `clang_check.py` compiles the tests without `TOOLS_ENABLED` but never
  runs them.
- **Options:** guard every such case with `TOOLS_ENABLED` and rely on review; write cases that work in both builds;
  run the doctests in a test-enabled release template locally.
- **Choice:** `talewright/tools/build.py template-tests` builds `bin/godot.windows.template_release.x86_64.tests*.exe`
  (`dev_mode=yes`, own objects through `extra_suffix`), and before each push its `--headless --test` run must pass,
  as the editor's does. Cases prefer data written the way the engine saves it (the inherited scene is now a text
  scene file) over editor-only construction; cases that need the editor stay inside `TOOLS_ENABLED`.
- **Why:** the failure is only visible in a build CI makes and this machine did not; one more incremental build
  before a push costs minutes, a red CI run costs a push and a fix commit.
- **Reversal:** drop the profile and the step; CI still catches these failures, after the push.

## D-049 — Game windows: the skin in Godot's theme, a toned back, opening through a band
- **Context:** task 10.1.1 needs the game's window (RM-MNU-016: windows draw from a skin image whose back takes the
  System data's window tone, -255..255 per channel). No step yet puts default assets into a project, the default
  theme is built after modules initialize (`main/main.cpp:834` vs `:3880`) and again with every RenderingServer
  (`scene/theme/theme_db.cpp:88-95`, per test case in doctests), and the help gives no opening time or padding.
- **Options:** the skin as files in the managed addon, found by path; textures made per window from data built into
  the runtime; the built-in skin added to Godot's default theme as theme items of a `JRPGWindow` type; a new skin
  resource type. For the tone: a shader on the back, or the back's pixels tinted on the CPU.
- **Choice:** `JRPGWindow` (runtime `Container`) reads theme items (icons `back`, `border`, `cursor`, `arrows`;
  constants `padding` 12, `frame_margin` 16), so project themes and overrides change the skin as for any control. The
  generated skin is embedded at build time (`modules/jrpg/SCsub`, `jrpg_builders.make_default_skin`) and added to the
  default theme the first time a window updates its theme items (`JRPGWindow::ensure_default_skin`, the theme's
  signals blocked meanwhile), so the textures live with the theme and its RenderingServer. The generator draws the
  window in two layers (`window_back.png`, `window_border.png`; `window.png` unchanged) and the tone is added to the
  back's pixels once per tone change (no shader: the same on every renderer, the headless one included). Opening and
  closing take `OPEN_FRAMES` = 8 frames at 60 a second (Talewright's choice); meanwhile the frame is squashed into
  the band it opens through and the window's children are clipped to that band (they show through it rather than
  appearing only when open); closed, nothing is drawn. Windows stop mouse clicks.
- **Why:** reuses Godot's theme system for customization and its renderer's clipping; no project files are needed for
  a window to draw (games, tests, goldens); the tone tints only the body, as the skin's two layers allow.
- **Amended (epic 10 review A5-A8, A16, A19, A21, B9; task 10.4.1.2.1):** by default (`use_system_tone`) a window
  takes the window tone of its database's System data, the window's own `tone` otherwise; the game state's Change
  Window Color override (`JRPGGameState::set_window_tone_override`) joins with that command (epic 24). `database`
  holds only what was set; the project's database is used through `get_effective_database()` (loaded once, never
  stored in scenes). Skin images may be AtlasTextures: nine-slices are drawn from the texture's region, as
  `StyleBoxTexture::draw` does (`scene/resources/style_box_texture.cpp:166-185`). The scroll and pause marks are
  centered `mark_offset` (a theme constant, 6) from the edge, in the padding over the border, so they never cover a
  row. `open()`/`close()` on a window already in that state emit on the next idle frame (a caller can still await);
  on a window going the other way they turn it round without the other signal. The interface sound player keeps its
  stream while the same sound repeats (so repeats overlap, up to four) and is found by ObjectID (scripts may free it).
- **Reversal:** contents hidden until open: give the window an internal content node and toggle it; skin in the
  addon: drop `ensure_default_skin` and point a project theme at the addon's images; timing: `OPEN_FRAMES`.

## D-050 — How a list reads the player (selectable list)
- **Context:** task 10.1.2 needs the list window every menu uses. The help gives the controls (RM-INP-001, RM-TST-015,
  RM-TST-016), the repeat timing (RM-INP-004: again 24 frames after the press, then every 6), mouse and touch use
  (RM-MNU-017) and the interface sounds (RM-MNU-019); it is silent on which window takes the keys, wrapping at the
  ends, what the page keys do in a list, and what a click does.
- **Options:** an "active" flag per list, or Godot's focus; Godot's `is_action_just_pressed` (it compares the engine's
  process frame, which doctests never advance, and a window that gets the focus in the frame of a press would take
  that press too), or the list's own edge detection; key repeat from the operating system's echo events (keyboard
  only, its own timing), or the list's own.
- **Choice:** `JRPGSelectableList` (runtime, a `JRPGWindow`) takes input while it has focus (`FOCUS_ALL`; its own
  actions are accepted so the viewport's `ui_*` navigation never moves the focus away). A press is new when the action
  was up the frame before or arrived as an event while the list had focus (taps shorter than a frame count); a key
  already down when the focus comes in is not new. Directions wrap round at the ends on a fresh press only (a held
  key stops at the end); held, they repeat after 24/60 s, then every 6/60 s. Up and down move by rows, left and right
  by items (with two columns or more). The page actions move the view and the cursor by a page of rows. OK on a
  disabled item buzzes (`item_refused`). The mouse: pointing at an item moves the cursor there, a click on the item
  under the cursor chooses it (on another item it moves the cursor first), the right button cancels, the wheel
  scrolls; clicking a list also gives it the focus (Godot's `FOCUS_ALL`). Sounds come from the System data of the
  list's database (the project's when none is set); the player is made with the first sound, as a player needs the
  audio server from its creation (`scene/audio/audio_stream_player_internal.cpp:365`).
- **Why:** focus is Godot's own notion of the active control; the list's own press detection behaves the same in
  games, doctests and bridge runs, and never lets one press act on two windows.
- **Amended (epic 10 review A1, A2, A4, A13, A14, A15; task 10.4.1.1):** a key down when the focus comes in is
  blocked (neither a press nor a repeat) until it is released. The viewport's focus keys (`ui_focus_next`,
  `ui_focus_prev`, `ui_up` and the other directions, `scene/main/viewport.cpp:2382-2424`) are accepted by a focused
  window, so they never move the focus away. After a long frame a held key repeats once and the next repeat is an
  interval after now (no burst of repeats). The click that gives a list the focus only moves the cursor (a mouse button
  down at focus-enter); choosing takes a second click. A right click anywhere cancels while the list has the focus
  (`unhandled_input`, so a control under the pointer that takes the click keeps it). The page actions follow
  `page_mode`: scroll (default), a `page_requested` signal (menus switch actors with them, RM-TST-015) or not taken.
  A window takes clicks only on the part that shows (`has_point`); closed, it and its children take none
  (`MOUSE_BEHAVIOR_DISABLED`) and it lets go of the focus. An answered choice window is `FOCUS_NONE` until the next
  choices. Dragging and touch scrolling (RM-MNU-017) are left to epic 28 (menus completion).
- **Reversal:** `JRPGSelectableList::_process_input` (presses and repeats), `_move_cursor` (wrapping), `gui_input`
  (mouse), `unhandled_input` (right click); `JRPGInputReader::reset`/`update` (blocked keys, repeats).

## D-051 — Message control codes: expanded in core, parsed into operations
- **Context:** task 10.2.1 needs the Show Text codes (RM-MSG-001..019). The help lists them and their effect but not
  how the text is read: case, nesting, codes it does not know, malformed arguments, how long `\.` and `\|` wait in
  frames. Online, servers send text keys and arguments and clients expand them (D-026), so the step that reads game
  data must work without a window.
- **Options:** one step that renders a window directly; BBCode for Godot's `RichTextLabel`; a core expander plus a
  parser to operations that a window lays out.
- **Choice:** `JRPGMessageText` (core, static): `expand(text, game state)` replaces `\V[n]` (repeatedly, so `\V`
  inside `\V` and `\N[\V[n]]` work, up to `MAX_EXPAND_PASSES`), then `\N[n]` (the actor's current name: an event's
  rename or its database name), `\P[n]` (1-based party order), `\G` (System data's currency unit); a variable never
  set is 0, a missing actor or slot is empty. `parse(text)` gives operations (text, new line, color, icon, size steps
  and size, gold window, waits of 15 and 60 frames, wait for input, instant on/off, no input wait, x and y position).
  Codes are read in any case. A code not in the list, or whose `[n]` is malformed (no digits, no closing bracket,
  more than 9 digits), is shown as written, so a typo stays visible instead of vanishing. `\\` is a backslash and is
  kept apart while `expand` works, so `\\V[1]` stays text. Laying the operations out (fonts, size steps, icons,
  positions, the letter-by-letter display) is the message window's (task 10.2.2).
- **Why:** servers and clients share the expansion; windows and tests read the same operations; RichTextLabel cannot
  place text at an absolute position (`\PX`, `\PY`) and its BBCode would need escaping of every message.
- **Amended (epic 10 review B6; task 10.4.1.2.3):** the values `expand` puts in (variables, names, the currency unit)
  are text, never codes: their backslashes come out escaped, so a name holding `\C[2]` shows it and a variable holding
  `\V[n]` is not expanded again (only a `\V` written in the text, as an argument, nests). The reference keeps
  inserted values out of its code pass in the same way. `escape()` does the same for any text.
- **Reversal:** the rules are in `jrpg_message_text.cpp` (`expand`, `parse`); the wait lengths are its constants.

## D-052 — The message window: letter by letter, skipping, layout
- **Context:** task 10.2.2.1 draws Show Text (RM-CMD-001: face on the left, a speaker name above, up to four lines,
  Window/other backgrounds, Bottom/Middle/Top). The help does not give the letter speed, what OK does while the text
  appears, the size of a size step, how `\PX`/`\PY` count, the text colors (they come from the reference's skin image,
  which Talewright neither has nor copies) or where the name goes when the message is at the top.
- **Options:** Godot's `RichTextLabel` with `visible_characters`, or the window's own layout with `draw_string`;
  letters per frame counted in frames or in time.
- **Choice:** `JRPGMessageWindow` (runtime `JRPGWindow`) lays the `JRPGMessageText` operations out itself (no
  wrapping: lines are the text's own; one line height for all lines, the theme's `line_height`; `\PX`/`\PY` from the
  text area's top left inside the padding, the face included) and draws the revealed characters with `draw_string`
  (each run shaped by Godot's text server). One character a frame at 60 frames a second, counted in time (the same at
  any frame rate); an icon counts as one character. `\.` and `\|` wait 15 and 60 frames: the next letter comes when
  the wait ends. OK or cancel while the text appears shows the rest at once up to the next `\!`, timed waits
  included; a click anywhere on the screen counts as OK (RM-INP-001). A size step is 6 pixels, sizes from 8 to 96.
  The text colors are Talewright's own 32-color palette (`text_colors`, settable per window). The name box goes above
  the message, below it when the message is at the top. Dim is a flat translucent band (`dim_color`). The icon set
  joins the built-in skin (`icon_set`, 32-pixel icons), so `\I[n]` draws without project files. The window's layout
  never moves its internal parts (the name box): `get_child(i, false)`, as internal children are counted by default
  (`scene/main/node.h:536-537`).
- **Why:** `RichTextLabel` cannot place text at an absolute position and its BBCode would need escaping; drawing runs
  ourselves keeps the reveal, waits and positions exact and testable.
- **Amended (epic 10 review B7, B8, B11, B15, B18, B19; task 10.4.1.2.3):** a letter of the reveal is a grapheme
  cluster (`TextServer::string_get_character_breaks`), never part of one. Text below the fourth line (by new lines or
  `\PY`) is left out, with a warning when the message is shown; the text is drawn on its own canvas item clipped to
  the window, so sizes larger than the line stay inside. The window's height uses the theme's padding; the layout is
  made again on theme changes and text color changes, and the placement on screen size changes, without restarting
  the reveal. The speaker's name uses the message font. `\PX`/`\PY` count from the top left inside the padding,
  while the help (RM-MSG-017/018) says the window's corner: Talewright's choice, so position 0 is the first place
  text can show. `\^` acts wherever it stands; a `\!` at the very end is the end's own wait (one press).
- **Amended (epic 10 review B5, B10, B12; task 10.4.1.1):** the message window can take the focus only while a
  message is under way (`FOCUS_ALL` from `show_message` to the end, `FOCUS_NONE` otherwise), so a finished message
  never steals clicks or the focus from choices beside it. While under way, a left or right click anywhere a control
  does not take it reads the message (OK and cancel both skip or continue). The reveal counts frames: a frame within
  0.05 of a whole number of 60ths counts exactly that many letters (no 0-then-2 jitter), and one frame reveals at most
  `MAX_LETTERS_PER_FRAME` = 4 (after a hitch the text catches up a few letters at a time).
- **Reversal:** `JRPGMessageWindow::_layout` (layout), `_advance` (reveal, waits), `_press` (skipping); the palette is
  `get_default_text_colors`.

## D-053 — Choices, number input and the gold window
- **Context:** task 10.2.2.2 needs Show Choices (RM-CMD-002: up to six choices, blank ones hidden, a default, cancel
  mapped to a choice or its own branch, a window position), Input Number (RM-CMD-003: 1 to 8 digits) and the gold
  window (RM-MSG-010). The help does not list every position or cancel option, nor how digits are changed, nor where
  the windows go relative to the message.
- **Choice:** `JRPGChoiceWindow` (a `JRPGSelectableList`) shows the non-blank choices, as wide as the widest, and
  reports the index each had among those given; cancel is `CANCEL_DISALLOWED`, `CANCEL_BRANCH` (reported as -1) or a
  choice index; left, middle or right; above a message in the lower part of the screen, below one at the top, at
  the bottom without one (the `message_window` path). Choice text keeps its color, icon and size codes: list items
  holding a backslash are drawn code by code. After a choice the window lets go of the focus, so a second press
  chooses nothing. `JRPGNumberInputWindow`: up and down change the digit under the cursor (9 round to 0), left and
  right move between digits (round at the ends), repeating while held; OK enters the number; no cancel; at the
  right beside the message. `JRPGGoldWindow`: the amount and the unit (system label color) at the top right. Text
  windows share the message window's font, size and line height (theme type `JRPGMessageWindow`); lists default to
  the same 24-pixel size, and a class extending the list gets that size for its own type in the default theme,
  because a theme with a default font size answers it for any type before a parent type's own
  (`scene/resources/theme.cpp:670-672`). The interface sounds and the database moved from the list to `JRPGWindow`;
  key repeats (RM-INP-004) moved to `JRPGInputReader` (shared by the list and the number input).
- **Why:** the windows reuse the list and the window; the interpreter's presentation port (task 9.4) decides when to
  show them (the reference shows choices as soon as the message text is out; the test scene asks for OK first).
- **Amended (epic 10 review A9-A12, A17, A18, A20; task 10.4.1.2.2):** a choice holding an icon is shown (blank
  means nothing but blanks and codes that show nothing); when every choice is blank nothing is shown, an error is
  printed and the choices end as cancel would end them (the cancel branch when cancel is refused), on the next idle
  frame, so an event never waits for a choice that cannot be made. A cancel index that is no choice is an error and
  refuses cancel. Choice and number windows are kept on the screen. The Dim background is `JRPGWindow`'s band
  (`dim_visible`, theme item `dim_color`), shared by messages, their name box and choices. Coded item text is laid
  out by one walk that both measures (choice width, sizes and icons included) and draws (an ellipsis at the end, room
  for the right text). Text windows read the message window's text items and the list's cursor margin unless their
  own theme overrides set them (Control skips a control's overrides when asked for another theme type). A theme type
  variation of a list gets the list's font size in the default theme, as subclasses do.
- **Reversal:** `JRPGChoiceWindow::_place` and `show_choices`, `JRPGNumberInputWindow::_process_input`.

## D-054 — The screen stack (managed addon)
- **Context:** task 10.3 needs the scene stack that menus, shops, save and battle screens run on (D-004: screens are
  GDScript and scenes in the managed addon). Godot has no screen stack; `SceneTree::change_scene_to_*` replaces the
  whole scene and would drop the running game (D-041).
- **Options:** change scenes; a C++ node; a GDScript `CanvasLayer` in the addon game scene.
- **Choice:** `JRPGScreenStack` (`addons/talewright/ui/screen_stack.gd`, a `CanvasLayer` named `Screens`, layer 10, in
  the addon's game scene, referenced by a relative path so the addon works in any folder,
  `scene/resources/resource_format_text.cpp:509-511`): `push`, `pop` (`keep` to take the screen back), `replace`,
  `clear`, `top`, `count`; the screens under the top one get `PROCESS_MODE_DISABLED` (their windows read no input)
  and give up the focus, which they get back when they are on top again; a new screen focuses its first control that
  takes the focus; optional `_on_screen_activated` / `_on_screen_suspended` hooks. Push, pop and replace fade through
  black (`fade_seconds`, 0.15 s each way) unless `fade` is false; callers await them. A doctest parses every embedded
  addon script with the GDScript parser and runs the stack from its embedded source.
- **Why:** screens are project-editable scripts and scenes (D-004) and the map, the game state and music keep running
  under them.
- **Reversal:** the script is one file; another stack can replace the `Screens` node.
