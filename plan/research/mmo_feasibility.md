# MMO feasibility for Talewright (task 2.3)

Question (owner, 2026-10-13): can the engine have mature, robust tooling to ship JRPGs/ARPGs as MMOs
(PokeMMO-like or Ragnarok-Online-like), with backend servers as separate programs, so that games are built in
the editor and integrate seamlessly? Sources: `plan/research/planning_notes/05_godot_networking_server.md`
(Godot code, file:line at 4.7.2-stable), `06_rathena_study.md` (rAthena d4b8e7b8), `07_mmo_prior_art.md`
(web sources). Technique catalogue: `plan/research/rathena_techniques.md`.

## Verdict
**Conditionally feasible**, as a set of server programs built from this engine and sharing its JRPG logic core —
**not** by using Godot's high-level multiplayer API for the world simulation. Nothing is measured yet, so epic 14
starts with three **go/no-go spikes** whose results are recorded before any further MMO work:
1. Raw `ENetConnection` loopback test with 1000 peers (CPU, memory, RTT, packet loss on this machine); fallback if it
   fails: TCP/WebSocket transport behind the same protocol layer.
2. Zone-tick prototype: 1000 simulated entities with AoI updates at RO-like movement rates; p99 tick < 50 ms.
3. Handoff chaos test: kill/restart world and zone processes during login, map change and logout; no duplicated or
   lost characters/items.
A failed spike blocks the dependent MMO epics with the measured reason (tracker rule), and the design is revised.
The Nakama alternative recommended by research note 07 is answered by D-016 (own services; Nakama adapter deferred).

## Why not Godot's high-level multiplayer for the MMO world
- `SceneReplicationInterface` reads and encodes state per peer × per visible synchronizer every network frame,
  nothing shared between peers; the source calls it "a lazy implementation"
  (`modules/multiplayer/scene_replication_interface.cpp:809`) and drops single states above the MTU
  (`modules/multiplayer/scene_replication_interface.cpp:832`).
- No spatial interest management: only per-peer booleans and script visibility filters
  (`modules/multiplayer/multiplayer_synchronizer.cpp:194-208`).
- The wire protocol "isn't meant to be used by non-Godot servers. It may change without notice."
  (`modules/multiplayer/doc_classes/SceneMultiplayer.xml:10`).
- `server_relay` (default on) announces every peer to every peer on join (`modules/multiplayer/scene_multiplayer.cpp:368-386`).
- Community report: server CPU spikes and mass disconnects at ~80–100 clients using the high-level API with ENet on a
  2-vCPU VM; the team replaced the stack with a custom TCP layer and stability improved
  (https://forum.godotengine.org/t/our-experience-building-an-mmo-tech-demo-with-godot-4-4-1-net-c/134348; anecdotal).
  The report does not separate ENet itself from the high-level API on top of it; Talewright keeps ENet only if
  go/no-go spike 1 passes, and the protocol layer is transport-agnostic so TCP/WebSocket remain available.
It remains fully available for ordinary Godot multiplayer games (requirement: nothing Godot can do is lost).

## What Godot gives us (reuse)
| Need | Godot piece | Reference |
|---|---|---|
| UDP transport, reliable/unreliable channels, RTT stats | `ENetConnection` / `ENetPacketPeer` (≤4095 peers/host, 255 channels, zstd/deflate) | `modules/enet/enet_connection.h:112-133`; `modules/enet/enet_connection.cpp:308-309` |
| Transport encryption | ENet DTLS (`dtls_server_setup`), `StreamPeerTLS`, `TLSOptions` | `modules/enet/enet_connection.h:130-133`; `core/crypto/crypto.h:89-91` |
| Web clients | `WebSocketPeer.accept_stream` over `TCPServer`/TLS | `modules/websocket/websocket_peer.h:84` |
| Server process host | `--headless`, dedicated_server export (strips textures/meshes to placeholders), custom `MainLoop` or SceneTree | `main/main.cpp:1505-1508,2290-2293,4360-4436`; `editor/export/dedicated_server_export_plugin.cpp:122-127` |
| Threads | `Thread`, `Mutex`, `Semaphore`, `WorkerThreadPool` | `core/os/thread.h:64`; `core/object/worker_thread_pool.h:237-298` |
| Crypto | CSPRNG, HMAC, constant-time compare | `core/crypto/crypto.h:130-143` |
| Compression | zstd/deflate | `core/io/compression.h:47-58` |
| Safe decoding | `decode_variant` with objects disallowed by default | `core/io/marshalls.h:222` |
| Server builds | build profiles, `disable_3d`, module stripping | `SConstruct:264-282,645-664` |

## What must be built (NEW)
Area-of-interest grid; schema-generated protocol + codecs; session/rate limiting; login/world/zone roles and
inter-server links; tickets and online registry; persistence (Godot has **no** database driver anywhere in the
tree); server data compiler (map cell cache + database snapshot); server-side interpreter port; instanced battles;
ARPG combat timing; social/economy systems; GM/ops tooling; cluster runner and inspectors in the editor; bots.

## Topology (D-015, rAthena techniques clean-room)
- **login**: accounts (Argon2id), brute-force bans, issues single-use HMAC tickets bound to account/IP/expiry.
- **world** (rAthena char + inter): character list/create/delete, persistence owner (async DB worker), party,
  guild, friends, mail, market, online registry (kick older session), zone directory, zone handoff, reconnect +
  resync of online lists with zones.
- **zone** (rAthena map-server): simulates many maps (+ channels and instances) in one single-threaded tick loop
  (timer heap + network poll); AoI 8×8 block grid with strip-based enter/leave; server-authoritative movement;
  NPC/event interpreter with network presentation port; battles; ARPG timers and AI.
- Processes scale horizontally by map assignment; the world process owns persistence.
- Failure honesty: the **world process is a single point of failure** for logins and saves (mitigation: fast restart,
  zones keep simulating and queue saves until it returns, as rAthena replays pending saves on reconnect,
  `ra:src/map/chrif.cpp:507-565`; a standby world is a later option). A **zone crash loses the state changed since its
  last save** (mitigation: event-triggered saves for trades/shops/storage, frequent incremental autosave, an
  append-only economic transaction log written before acknowledging trades).
- Optional later: gateway proxy, Nakama adapter (task 43, deferred).

## Game modes
- **PokeMMO-like JRPG:** tile movement with local prediction + reconciliation; instanced turn-based battles
  executed on the zone server (client waits for results, no prediction for irreversible state — Gambetta,
  https://www.gabrielgambetta.com/client-side-prediction-server-reconciliation.html); trading and a global
  market with transactional persistence.
- **RO-like ARPG:** client sends a destination; server A* + path caps + per-cell speed timers; clients interpolate
  along announced paths (rAthena `ra:src/map/unit.cpp:180-247`); server-timed cast/delay/cooldowns; two-speed
  region-scheduled mob AI; drops/exp sharing from damage logs.

## Persistence and security (D-017, D-014)
SQLite (vendored, public domain, https://sqlite.org/copyright.html) for development/single host; PostgreSQL via
libpq (PostgreSQL License, https://www.postgresql.org/about/licence/) for production; MySQL/MariaDB only via an
optional dynamically loaded LGPL connector (deferred); MySQL Connector/C++ (GPL + FOSS exception) avoided.
Argon2id passwords; DTLS/TLS transport; per-IP connection limits; token buckets per opcode; state-gated dispatch;
atomic versioned character saves; unique item instance IDs; two-phase trades; audit logs.

## Prior art (license → use)
| Project | License | Use |
|---|---|---|
| rAthena / Hercules | GPL-3.0 | design reference only (no code) |
| TrinityCore / AzerothCore | GPL-2.0 | reference only |
| Ryzom Core | AGPL-3.0 | reference only |
| Nakama (+ nakama-godot) | Apache-2.0 | optional adapter (deferred); OSS edition has no clustering |
| Colyseus | MIT | reference; Node runtime, beta Godot SDK |
| SpacetimeDB | BSL 1.1 (single production instance until 2031-10-08) | not a dependency |
| godot-tiny-mmo | MIT | reference (Gateway/Master/World split, custom protocol, SQLite) |
| FishNet | custom license forbidding competing implementations | not read |
| OpenMMO (PokeMMO server clone) | archived at PokeMMO's request | avoid |
Sources and dates: `plan/research/planning_notes/07_mmo_prior_art.md`.

## Legal
No Nintendo/Gravity data, assets or protocols. **Patent risk for monster-capture mechanics:** Nintendo and The
Pokémon Company sued Pocketpair (Palworld) for patent infringement in the Tokyo District Court on 2024-09-18
(https://www.nbcnews.com/tech/nintendo-pokemon-company-palworld-pocketpair-lawsuit-rcna171817; the case's current
status is not verified). Talewright therefore ships **no monster-capture template**; any such template requires
counsel review before it is built or shipped, and game makers are responsible for clearing their own mechanics.
Trade dress (look and feel imitating a franchise) is avoided by shipping only original assets and UI. PokeMMO's only documented precedent for third-party content is
"bring your own legally owned ROM" (https://manpages.debian.org/unstable/pokemmo/pokemmo.6.en.html); Talewright
ships original assets only. GPL/AGPL code is never copied into the MIT fork. Not legal advice.

## Capacity targets (to be measured; **not verified** until bot-swarm results exist, task 42)
- ≥ 1000 concurrent bots per zone process at RO-like movement rates with p99 tick < 50 ms.
- World ≥ 10k online characters registered.
- Login burst ≥ 100 logins/s with Argon2id at production parameters (may need a worker pool).

## Risks (owner = implementing epic)
| Risk | Mitigation | Epic |
|---|---|---|
| Transport/scale unknowns | Go/no-go spikes 1–3 before further MMO work; bot swarms | 14, 42 |
| Privacy law (GDPR, COPPA; minors attracted to creature games; logs keep IPs and chat) | Data minimization, retention limits, export/delete tooling, age gate option, privacy notice template | 39 |
| Chat abuse | Filters, mute/report flow, GM review queue, rate limits | 37, 39 |
| Payments / loot-box regulation | No payment code shipped; any store integration is a separate, reviewed epic | — |
| Bots and real-money trading | Server authority, rate limits, anomaly logs on trades/market, GM tools | 38, 39, 40 |
| Volumetric DDoS | Per-IP limits do not stop floods: document upstream scrubbing/proxy hosting; login behind a gateway option | 40, 41 |
| Hosting cost | Measure per-zone resource use in spikes and bot swarms; document sizing | 42 |
| Backups, point-in-time recovery, migrations | Migration ledger, scheduled backups, restore drill in chaos tests | 16, 41 |
| Observability | Structured logs, metrics endpoint, per-process dashboards in the Server workspace | 21, 39 |
| Client patching and protocol versions | Protocol hash handshake; PCK patch delivery; launcher out of scope until requested | 14, 41 |
| Live-ops staffing | GM tooling and staged content apply reduce manual work; documented runbooks | 39 |
| GDScript cost per server tick | Simulation in C++ (`jrpg/core`, `mmo/server`); GDScript only for hooks, with budgets | 15, 18 |
| World single point of failure; zone crash data loss | See topology section: fast restart, queued saves, transaction log | 16, 17 |

Scope (staged epics 14–21 then 37–42); server Linux builds from a Windows host (fork CI or WSL; blocked if neither
is available); DB driver build on Windows (SQLite first; PostgreSQL via system libpq); correctness of handoff and
persistence under failure (chaos tests, dupe tests); legal (clean-room discipline).
