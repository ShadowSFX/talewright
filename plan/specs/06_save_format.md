# Spec 06 — Save format and game state serialization

Inventory: RM-SAV-001..007, RM-NEW-026/027 (autosave, game id), RM-CMD2-029 (save access). Decision: D-005, D-022.

## What is saved (`JRPGGameState.serialize()` → Dictionary)
`format: "talewright-save"`, `format_version: int`, `game_id: String` (`JRPGSystemData.game_id`, a random id
generated once by the setup tool and editable in the System tab, RM-NEW-027; not a project setting, D-008),
`db_schema_version`, `db_content_hash`, `playtime_steps` (60 per second, menus included), `map: {map_id, cell,
direction, vehicle, followers}`, `switches: PackedByteArray (bitset, bit id%8 of byte id/8, trailing zero bytes
dropped)`, `variables: Array (int/float/String, index = id)`, `self_switches: Dictionary "map:event:letter" → true`
(only switches that are on), `party: {members, gold, steps, items, weapons, armors}` (each kind `{id: count}` in
ascending id order, counts ≥ 1),
`actors: [{id, name, nickname, class_id, level, exp, hp, mp, tp, param_bonus[8], skills (ascending), states: [{id,
turns, steps}], buffs[8], buff_turns[8], equips: [{kind: "weapon"|"armor", id} or {} per slot]}]` in ascending
actor id, one entry per actor used so far (`name` and `nickname` only when an event changed them; otherwise they come
from the database through the current language, D-026). An actor keeps one EXP total: levels follow it within 1 and
the actor's maximum level, and loading never re-learns skills. Change Class without "keep level" keeps the EXP and
lets the level follow the new class's table (the help documents only the keep-level option, RM-CMD-034). Each slot
stores the item kind with its id, so losing dual wield never reads a weapon id as an armor id.
`JRPGGameState.attach_database()` recomputes parameters after loading and repairs references to entries deleted
since the save (actors, party members, inventory, and each actor's class, skills, states and equipment), with one
warning,
`system: {save_enabled, menu_enabled, encounters_enabled, formation_enabled, window_tone (only when set by an event),
music: {battle/victory/defeat → {stream, volume, pitch, pan}} (only overrides), timer: {working, steps}}` (vehicle
music joins with vehicles), `screen: {tint, pictures, weather}`, `audio: {bgm, bgs, saved bgm}`, `interpreters: [spec
03 serialized state for the map autorun/parallel processes and the current event, with list hashes]`,
`rng: {stream name → {state, inc}}` (spec 04 streams), `clock: simulation steps`, `events_state: {map_id,
map_revision, events: [JRPGEventState]}` for the current map (other maps start from their data when entered).
The save writer adds `saved_at` (UTC ISO-8601) to the file header; `serialize()` never reads the wall clock, so the
same state always gives the same Dictionary and bytes (keys in a fixed order). Sections are added by the tasks that
build their features; `deserialize()` ignores unknown keys, gives missing sections their defaults, and checks the
whole input before changing anything.

**Limits** (`JRPGGameState` constants): switch and variable IDs 1..100 000 (the database table bound); item counts
0..99 per item, weapon or armor (RM-CMD-019); gold 0..999 999 999 — the help gives no gold limit (RM-CMD-018 notes),
so this is Talewright's own: nine digits fit the menu's gold window and stay far from 32-bit overflow when rewards are
added; timer 0..99 min 59 s (RM-CMD-009).
`JRPGEventState` (core): `{event_id, cell, direction, pattern, route_index, route_wait, erased, transparent,
through}` (through: 0 from the page, 1 on, 2 off), every key required, in ascending `event_id`. Entering another map
(`JRPGGameState.set_events_map`) forgets the previous map's states. On load, states whose `event_id` no longer exists
are dropped (`prune_event_states`, one warning); if `map_revision` differs, events whose data changed are reset to
their data position (one warning) — done by the map loader once map data exists (task 8.6.1).
Header (`slot_info`): title, playtime, party faces/characters, map name — readable without loading the full state.

## Encoding and files
- `user://saves/<game_id>/slot_<n>.save` (`n` 0 = autosave, 1..N manual; N from System data).
- File = 8-byte magic `TWSAVE01` + `u32 header_len` + header (`var_to_bytes`, objects disabled; **never encrypted**,
  so slot lists read it cheaply) + `u8 flags` (bit 0 compressed, bit 1 encrypted) + `u32 raw_body_size` +
  `u32 stored_body_size` + 16-byte IV (zeros when not encrypted) + body + SHA-256 of everything before it. The whole
  file is built in memory and the hash is checked before anything is decoded.
- Body = `var_to_bytes` of the state (objects disabled), compressed with `Compression::compress(MODE_ZSTD)`. zstd has
  no dynamic decompression in Godot (only gzip/deflate/brotli do, `core/io/compression.cpp:294`), so the raw size is
  stored and capped at 64 MiB before allocating.
- Optional encryption (off by default; documented as obfuscation, not security): the compressed body is encrypted in
  memory with `CryptoCore::AESContext::encrypt_cfb` (AES-256-CFB, key = SHA-256 of the project's save key, random IV
  from `CryptoCore::RandomGenerator`; `core/crypto/crypto_core.h:42-52,102-118`) — the same primitives
  `FileAccessEncrypted` uses. Godot's file-level helpers do not fit: `FileAccess::open_encrypted_pass`
  (`core/io/file_access.h:245`) encrypts the header too and decrypts the whole file on open
  (`core/io/file_access_encrypted.cpp:94-110`), and `FileAccessEncrypted` writes only when closed
  (`core/io/file_access_encrypted.cpp:137-162`), so no hash could follow it.
- Replacing a slot is not atomic on Windows: `DirAccessWindows::rename` deletes an existing target and then moves
  (`drivers/windows/dir_access_windows.cpp:339-345`). Sequence: write and close `slot_n.save.tmp`; rename the current
  `slot_n.save` to `slot_n.save.bak` (replacing an older `.bak`); rename `.tmp` to `slot_n.save`. The loader tries
  `slot_n.save`, then a complete and valid `.tmp`, then `.bak`, so a crash at any point leaves a loadable slot.

## Loading and versioning
- Reject: wrong magic, hash mismatch, header/body decode errors (`decode_variant` with objects disallowed,
  `core/io/marshalls.h:222`) → slot shown as "corrupt" (with `.bak` restore offer); never crash.
- `format_version` older → run ordered migration steps; newer → refuse with a clear message.
- `db_content_hash` differs (database changed since the save) → load anyway; dangling IDs become "none" with a warning
  (spec 02), actors whose class no longer exists fall back to class 1.
- Saving mid-event: allowed when the interpreter is waiting at a safe point (message closed, no scene change in
  progress); the Save command itself is a safe point. Saving during a running parallel process stores its frames.

## Online (MMO) note
Character persistence on servers uses the same `JRPGGameState` sub-structures (actors, inventory, variables with
scopes) but different storage (spec 07); single-player saves are never trusted by servers.
