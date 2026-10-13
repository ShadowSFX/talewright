# Spec 07 — MMO protocol (IDL draft) and server topology

Decisions: D-014..D-017, D-015a, D-024. Research: `plan/research/mmo_feasibility.md`, `rathena_techniques.md`.
This is a draft refined when epic 14 starts (after the go/no-go spikes).

## Topology
| Role | Owns | Scales by |
|---|---|---|
| `login` | accounts (Argon2id), bans, brute-force limits, ticket issuing | stateless replicas behind one DB |
| `world` | characters, persistence (async DB worker), online registry, party/guild/friends/mail/market, zone directory, handoff | one per world (SPOF mitigated by fast restart + queued saves) |
| `zone` | simulation of assigned maps (+ channels, instances), AoI, movement, events (network port), battles, ARPG timers/AI | more processes, maps reassigned |
All roles are the same engine binary built with `mmo_server=yes` (headless profile) started with
`--tw-role=login|world|zone --tw-config=<file>`; they load the compiled server data pack (map cell caches, database
snapshot, content hash) produced by the editor.

## Session flow (state machine per character; rathena_techniques A2–A6)
`CONNECTED → AUTHENTICATED(login) → TICKET(world) → CHAR_SELECT → LOADING(zone: staged load) → IN_WORLD →
(MAP_CHANGE → LOADING …) → LOGOUT_SAVING → OFFLINE`. Tickets: 256-bit random, HMAC-signed with the cluster secret,
bound to account id + client IP + expiry (30 s), single use. World keeps the online registry; a second login kicks
the older session and waits for its final-save ack before admitting the new one.

## Wire format
- Frame: `u32 length (LE, ≤ 1 MiB)` + `u16 opcode` + payload (message fields in declaration order, little-endian;
  strings `u16 len + UTF-8`, arrays `u16 count + items`, optional fields with a presence byte).
- Transport: ENet channels (0 reliable ordered control, 1 reliable gameplay, 2 unreliable sequenced movement) or
  WebSocket binary frames (one message per frame). Handshake first message carries the **protocol hash** (SHA-256
  of the normalized IDL); mismatch → disconnect with "client out of date".
- Opcodes are generated from the IDL order within named ranges; they are Talewright's own and unrelated to any
  other game's protocol (D-015a).

## IDL (`modules/mmo/net/*.twproto`) — draft syntax
```
enum Direction : u8 { down = 2, left = 4, right = 6, up = 8 }
message MoveRequest @client {          # client → zone
  dest: vec2i
}
message EntityMoved @server area {     # zone → clients in area of interest
  entity: u32
  path: [vec2i] max 32
  start_tick: u32
  speed_ms: u16
}
message DialogShow @server self {      # network presentation port (spec 03)
  request: u32
  face: str max 64
  name: str max 64
  text: str max 4096
}
message DialogReply @client { request: u32  choice: i16 }
```
Codegen (Python, run by SCons): C++ structs + `encode/decode` with bounds checks (max lengths, counts), a dispatch
table `{opcode → handler, allowed session states, token-bucket cost}`, and GDScript-facing wrappers for game UIs.
Fuzz test: random bytes into every decoder must never crash or allocate beyond limits.

## Rate limiting and validation
Per-session token buckets per opcode class (movement, chat, actions, UI replies); per-IP connection rate limit;
messages rejected in the wrong session state; all values validated server-side (path length ≤ 17 cells and walkable,
skill cooldowns, inventory quantities, choice indices).

## Persistence model (epic 16)
Tables: `account`, `character`, `character_actor`, `inventory_item` (unique item instance id
`(world_id << 48) | counter`), `variable` (scope: character/account/global), `party`, `guild`, `mail`, `market_listing`,
`audit_log`, `migration_ledger`. Character save = one transaction with `version` check (optimistic concurrency);
dirty tracking sends only changed sections; staggered autosave + event-triggered saves (trade, shop, storage, mail).
