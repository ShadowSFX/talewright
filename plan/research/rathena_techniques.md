# rAthena techniques catalogue (task 2.3)

Reference: rAthena commit `d4b8e7b8f16061cc2496d377ac8f777ce72a4f39` (2026-10-06), read-only at
`C:\Users\Relic\Desktop\ro server\rathena` (the owner's Ragnarok Relic submodule). Paths below are relative to
that directory and prefixed `ra:`. **rAthena is GPL-3.0: Talewright reimplements techniques clean-room and
copies no code.** Full study with more citations: `plan/research/planning_notes/06_rathena_study.md`.

Each row: technique → why it matters → rAthena reference → Talewright mapping (task/epic).

## A. Process topology and handoff
| # | Technique | Why | rAthena | Talewright |
|---|---|---|---|---|
| A1 | Split login / char(+inter) / map servers, each a `Core` subclass with the same loop | Isolation, per-role scaling | `ra:src/common/core.cpp:333-422`; `ra:src/login/login.hpp:21`, `ra:src/char/char.hpp:22`, `ra:src/map/map.hpp:31` | `login`/`world`/`zone` roles (E15, E17) |
| A2 | Single-use handoff keys created at login, consumed by char, expiring in 30 s | No password reuse downstream; replay resistance | `ra:src/login/login.cpp:413-414`; `ra:src/login/loginchrif.cpp:72-124`; `ra:src/login/login.hpp:150` | 256-bit HMAC tickets bound to account/IP/expiry (E17) |
| A3 | Online registry with conflict detection; kicks the older session | Stops double-login duplication | `ra:src/char/char.cpp:121-160`; `ra:src/login/loginclif.cpp:84-108` | world online registry (E17) |
| A4 | Per-player auth state (login / logout / map change) with final-save ack before freeing | Correct under restarts and link drops | `ra:src/map/chrif.cpp:136-205,418-420,743-764` | per-player state machine (E17) |
| A5 | Staged player load; "enter world" ignored until all parts arrived | No half-loaded players | `ra:src/map/pc.cpp:2304,2434-2436,14800-14804`; `ra:src/map/clif.cpp:10756-10760` | staged load (E17/E18) |
| A6 | Auto-reconnect + full online-list resync both ways; replay pending saves | Self-healing cluster | `ra:src/map/chrif.cpp:507-565,1498-1508,1855-1879`; `ra:src/char/char_mapif.cpp:363-399` | world↔zone links (E15/E17) |
| A7 | Map servers register their map lists; char routes warps across map servers | Horizontal scaling by map | `ra:src/map/chrif.cpp:367-375,423-472`; `ra:src/char/char_mapif.cpp:198-274,609-678` | zone directory + zone handoff (E18) |

## B. Network layer
| # | Technique | Why | rAthena | Talewright |
|---|---|---|---|---|
| B1 | One loop: due timers, then socket poll with timeout = next timer | Deterministic, lock-free, low idle CPU | `ra:src/common/core.cpp:388-391`; `ra:src/common/timer.cpp:357-412`; `ra:src/common/socket.cpp:892-1037` | zone loop (E15) |
| B2 | Bounded per-session write queue (1 MB client cap), stall timeout, flush only dirty sessions | Slow/dead clients cannot exhaust memory/CPU | `ra:src/common/socket.cpp:257-263,822-890,1016-1024,1689-1761` | session queues (E14) |
| B3 | Per-IP connection-rate limit with allow/deny rules | Cheap pre-auth DDoS defence | `ra:src/common/socket.cpp:1112-1210`; `ra:conf/packet_athena.conf` | per-IP limits (E14/E40) |
| B4 | Table-driven packet dispatch with length checks and session-state gating | Malformed/out-of-phase packets never reach logic | `ra:src/common/packets.hpp:665-752`; `ra:src/map/clif.cpp:25728-25798` | generated dispatch tables + state gates (E14) |
| B5 | Login brute-force IP bans, registration flood limits | Account security | `ra:src/login/loginclif.cpp:222-223,520-531`; `ra:src/login/login.cpp:770-778` | E17/E40 |

## C. Timers and simulation
| # | Technique | Why | rAthena | Talewright |
|---|---|---|---|---|
| C1 | Timers carry entity ID + data and check ownership when fired; lazy deletion | Use-after-free safety without refcounts | `ra:src/map/unit.cpp:554-557`; `ra:src/map/status.cpp:14155`; `ra:src/common/timer.cpp:302-319` | timer heap API (E15) — with O(log n) removal (fixes weakness W14) |
| C2 | Late timers fire with the current tick; repeating timers re-arm from now | No catch-up storm after lag | `ra:src/common/timer.cpp:376-380,401-404` | E15 |
| C3 | 8×8 block grid per map, separate mob lists, collect-then-apply iteration with deferred free | Fast area queries; safe when callbacks remove objects | `ra:src/map/map.cpp:120,293-366,407-506,719-791` | AoI grid (E18) |
| C4 | On movement, only the strip entering/leaving view is scanned | Visibility cost scales with view edge | `ra:src/map/map.cpp:1077-1125`; `ra:src/map/unit.cpp:621-631` | AoI enter/leave (E18) |
| C5 | Server-decided movement: client sends a destination; server A* (no corner cutting), path caps, one timer per cell with speed (diagonal ×14/10) | Kills speed/teleport hacks | `ra:src/map/clif.cpp:11379-11425`; `ra:src/map/unit.cpp:180-247,542-632,841-916`; `ra:src/map/path.cpp:337-399` | D-018 movement (E8 offline, E18 online) |
| C6 | Cell bitflags including dynamic "touch NPC here" | O(1) touch triggers per step | `ra:src/map/map.hpp:788-810`; `ra:src/map/npc.cpp:4971-4998`; `ra:src/map/unit.cpp:683-698` | map cell cache flags (E8.2) |
| C7 | Two-speed mob AI (100 ms near players, 1 s elsewhere); spawn on first visitor; despawn after the map empties | CPU follows players | `ra:src/map/mob.cpp:2348-2476,7363-7364`; `ra:src/map/map.cpp:3004-3096` | ARPG AI (E20) — region-scheduled (fixes W12) |
| C8 | Stat recalculation by dirty bits along dependencies | Recompute only what changed | `ra:src/map/status.cpp:5837-5918` | battler stat cache (E5.4) |
| C9 | Skill cast/delay/cooldown as server timers; ground skills ticked as unit groups | Authoritative timing | `ra:src/map/unit.cpp:2469-2471,2702-2761`; `ra:src/map/skill.cpp:13915,12485-12491` | ARPG skills (E20) |
| C10 | Instances by cloning source map cells into a fresh slot with idle/lifetime timers | Cheap dungeon copies | `ra:src/map/map.cpp:2819-2893`; `ra:src/map/instance.cpp:405-470` | instances (E18) |

## D. Scripts / NPCs
| # | Technique | Why | rAthena | Talewright |
|---|---|---|---|---|
| D1 | Scripts compiled once; duplicates share compiled code | Memory and load time | `ra:src/map/script.hpp:230-265`; `ra:src/map/npc.cpp:4602-4604` | command trees shared by duplicate events (E9) |
| D2 | VM with instruction (65535) and jump (2048) budgets | Runaway scripts cannot hang the server | `ra:src/map/script.cpp:259,4420-4424,4472-4476` | interpreter budgets (E9.3) |
| D3 | Dialog commands suspend the script; menus/input re-run the same command when the answer arrives | Server-authoritative dialogs | `ra:src/map/script.cpp:4923-5031,5225-5292,6165-6189`; `ra:src/map/npc.cpp:2292-2378` | presentation port (E9.4, E19) |
| D4 | Dialog idle timeouts; invalid menu index kicks; disconnect frees script state | Robustness vs AFK/malicious clients | `ra:src/config/secure.hpp:22-48`; `ra:src/map/npc.cpp:1135-1205`; `ra:src/map/clif.cpp:13329-13334` | E19 |
| D5 | Variable scopes by prefix mapped to storage (temp char, char, account, global, NPC, instance) | Authors choose persistence explicitly | `ra:src/map/script.cpp:3248-3311` | variable scopes (E5.5, E19) |
| D6 | Event labels (OnInit, OnTimer, OnPCLogin…) and a small per-player event queue | Server-side hooks | `ra:src/map/npc.cpp:1211-1246,6004-6041`; `ra:src/map/script.cpp:257-300` | common events/hooks (E19) |

## E. Persistence and economy integrity
| # | Technique | Why | rAthena | Talewright |
|---|---|---|---|---|
| E1 | Save only what changed (cached last-saved copy; per-row item diff; dirty vars; zero deletes the row) | DB load drops by orders of magnitude | `ra:src/char/char.cpp:257-749`; `ra:src/map/intif.cpp:432-435`; `ra:src/char/inter.cpp:601-653` | dirty tracking (E16) |
| E2 | Autosave one player per tick (interval/(players+1)) + saves triggered by risky events | Smooth DB load; small loss window | `ra:src/map/pc.cpp:13002-13041`; `ra:src/map/map.hpp:1121-1133` | E16 |
| E3 | Unique item instance IDs `(owner << 32) | counter` | Dupes traceable | `ra:src/map/pc.cpp:15334-15336` | item UIDs (E16) |
| E4 | Two-phase trade (lock, confirm, re-validate vs snapshot, apply, save both); shop-version check on purchase | Classic dupe/race defences | `ra:src/map/trade.cpp:181-239,586-686`; `ra:src/map/vending.cpp:134` | E19/E38 |
| E5 | Item/zeny/GM/chat logs with bitmask config | Auditing | `ra:src/map/log.cpp:207-341` | audit logs (E16/E39) |
| E6 | Mapreg (global vars) batched writes, immediate inserts/deletes | Cheap global state | `ra:src/map/mapreg.cpp:62-112,230-264` | global variables (E16) |

## F. Content and ops
| # | Technique | Why | rAthena | Talewright |
|---|---|---|---|---|
| F1 | Versioned YAML data (header type+version), override imports, `Clear`, live reload re-pointing derived state | Safe content iteration and mod layering | `ra:src/common/database.cpp:37-200`; `ra:src/map/itemdb.cpp:4978-5000` | server data compiler + staged apply (E15/E39) |
| F2 | Config descriptor table `{name, pointer, default, min, max}` with import chains | Validated, overridable config | `ra:src/map/battle.cpp:8246-8252` | config tables (E15) |
| F3 | Permission groups with inheritance; per-group command logging | Auditable admin tooling | `ra:conf/groups.yml`; `ra:src/map/pc_groups.cpp:180-205` | GM groups (E39) |
| F4 | Ban/kick propagation across servers; broadcast via inter-server | Consistent admin actions | `ra:src/map/chrif.cpp:846,1086`; `ra:src/login/loginchrif.cpp:356-395` | E39 |
| F5 | Pooled allocation for hot object types | Long-uptime fragmentation control | `ra:src/common/ers.hpp:1-40` | object pools where measured hot (E42) |

## G. Weaknesses to avoid (Talewright fixes)
| # | Weakness | rAthena | Talewright fix |
|---|---|---|---|
| W1 | Synchronous SQL on the game thread | `ra:src/char/char.cpp:305-336`; `ra:src/map/log.cpp:242` | async DB worker (D-017) |
| W2 | MyISAM, non-transactional character saves | `ra:sql-files/main.sql:309,757`; `ra:src/char/char.cpp:525-527` | atomic versioned transactions |
| W3 | Raw C structs between processes, 64 KB ceiling | `ra:src/map/chrif.cpp:317-328,1941-1945` | schema-generated messages |
| W4 | Hand-maintained packet length tables | `ra:src/map/chrif.cpp:42-50`; `ra:src/char/inter.cpp:58-70` | IDL codegen |
| W5 | Hard caps (2 map-servers per char, 5 char-servers, FD_SETSIZE) | `ra:src/char/char.hpp:212`; `ra:src/login/login.hpp:62` | dynamic registries |
| W6 | 32-bit tokens, ID-only XOR "obfuscation", MD5 passwords | `ra:src/login/login.cpp:413-414`; `ra:src/map/clif.cpp:25717` | HMAC tickets, DTLS/TLS, Argon2id |
| W7 | Partial packet = disconnect; 3 packets/loop cap | `ra:src/map/clif.cpp:25679,25773-25780` | reassembly + token buckets |
| W8 | `exit()` on write-buffer misuse | `ra:src/common/socket.cpp:831-846` | per-session failure isolation |
| W9 | Global scratch state (bl_list, path heap) blocks parallelism | `ra:src/map/path.cpp:267,327-330` | per-instance scratch |
| W10 | Clear-then-load reloads; reload closes all dialogs | `ra:src/common/database.cpp:86-90`; `ra:src/map/atcommand.cpp:4399-4405` | build → validate → swap |
| W11 | Cache arrays sized from max ID | `ra:src/common/database.hpp:194-220` | hash maps / dense remap |
| W12 | Mob AI cost grows with overlapping players | `ra:src/map/mob.cpp:1854-1858,2348-2352` | region-scheduled AI |
| W13 | Saving inside a crash signal handler | `ra:src/map/map.cpp:5122-5139` | frequent incremental saves + WAL |
| W14 | O(n) `settick_timer`, lazy deletion | `ra:src/common/timer.cpp:302-353` | indexed heap with removal |
