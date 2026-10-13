# Spec 06 — Save format and game state serialization

Inventory: RM-SAV-001..007, RM-NEW-026/027 (autosave, game id), RM-CMD2-029 (save access). Decision: D-005, D-022.

## What is saved (`JRPGGameState.serialize()` → Dictionary)
`format: "talewright-save"`, `format_version: int`, `game_id: String` (project setting, RM-NEW-027),
`db_schema_version`, `db_content_hash`, `saved_at` (UTC ISO-8601), `playtime_frames`, `map: {map_id, player cell,
direction, vehicle, followers}`, `switches: PackedByteArray (bitset)`, `variables: Array (int/float/String)`,
`self_switches: Dictionary "map:event:letter" → bool`, `party: {members, gold, steps, items/weapons/armors counts}`,
`actors: [{id, name, nickname, class_id, level, exp, hp, mp, tp, equips, skills, states with turns, buffs}]`,
`system: {save/menu/encounter/formation access, window tone, battle/victory/defeat/vehicle BGM overrides, timer}`,
`screen: {tint, pictures, weather}`, `audio: {bgm, bgs, saved bgm}`, `interpreters: [spec 03 serialized state for
the map autorun/parallel processes and the current event]`, `rng: {state, inc}`, `events_state: per map moved events
(positions, directions, erased flags) for the current map only (others reset like RPG Maker)`.
Header (`slot_info`): title, playtime, party faces/characters, map name — readable without loading the full state.

## Encoding and files
- `user://saves/<game_id>/slot_<n>.save` (`n` 0 = autosave, 1..N manual; N from System data).
- File = 8-byte magic `TWSAVE01` + `uint32 header_len` + header (`var_to_bytes`, objects disabled) + body
  (`var_to_bytes`, objects disabled, compressed with `FileAccess::COMPRESSION_ZSTD`) + SHA-256 of header+body.
- Writes go to `slot_n.save.tmp` then rename over the old file (atomic replace); the previous file is kept as
  `slot_n.save.bak` once.
- Optional encryption: `FileAccess::open_encrypted_with_pass` (`core/io/file_access.h:246`) when the project sets a
  save key (off by default; documented as obfuscation, not security).

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
