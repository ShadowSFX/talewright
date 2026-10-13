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
enum Direction : u8 { up = 0, right = 1, down = 2, left = 3, up_right = 4, down_right = 5, down_left = 6, up_left = 7 }
message MoveRequest @client {          # client → zone
  dest: vec2i
}
message EntityMoved @server area {     # zone → clients in area of interest
  entity: u32
  path: [vec2i] max 64
  start_step: u32
  steps_per_cell: u16
}
message DialogShow @server self {      # network presentation port (spec 03)
  request: u32
  face: str max 128                    # uid:// of the face image
  name_key: str max 128                # localization key (D-026)
  text_key: str max 128
  args: [variant_arg] max 16           # values for control codes, expanded by the client
}
message DialogReply @client { request: u32  choice: i16 }
```
Codegen (Python, run by SCons): C++ structs + `encode/decode` with bounds checks (max lengths, counts), a dispatch
table `{opcode → handler, allowed session states, token-bucket cost}`, and GDScript-facing wrappers for game UIs.
The handshake also carries the client's content hash (database + maps + translations); a mismatch means the client
must update, because localization keys and IDs must resolve to the same content on both sides.
Fuzz test: random bytes into every decoder must never crash or allocate beyond limits.

## Rate limiting and validation
Per-session token buckets per opcode class (movement, chat, actions, UI replies); per-IP connection rate limit;
messages rejected in the wrong session state; all values validated server-side (destination at most
`max_move_distance` cells away, default 24, and reachable by a path of at most 64 steps; skill cooldowns, inventory
quantities, choice indices). Talewright's numbers: 24 cells covers a click at the edge of a 1080p view of 48-pixel
cells (20 cells) with room for detours, and 64 steps lets a path go around a building without a second request.
Directions use Talewright's clockwise-from-north order, the same as the character sheet rows (spec 05).
Simulation time on zones is the same fixed 60 Hz step as single-player (spec 03); a zone may process several steps
per network tick, and movement messages carry step numbers, not milliseconds.

## Persistence model (epic 16)
Tables: `account`, `character`, `character_actor`, `inventory_item` (unique item instance id
`(world_id << 48) | counter`), `variable` (scope: character/account/world; instance and temp scopes are never
persisted, D-025), `self_switch` (per character; shared events keep theirs per zone instance in memory), `party`,
`guild`, `mail`, `market_listing`, `audit_log`, `migration_ledger`. Character save = one transaction with `version` check (optimistic concurrency);
dirty tracking sends only changed sections; staggered autosave + event-triggered saves (trade, shop, storage, mail).
