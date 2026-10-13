# Spec 03 — Events, commands, interpreter, presentation port, authority

Decision: D-007. Inventory: RM-EVT-*, RM-CMD-*, RM-CMD2-*, RM-MRT-*, RM-CEV-*, RM-TRP-* (battle events).
rAthena reference (independent reimplementation, D-015a): dialog suspension, budgets, timeouts
(`plan/research/rathena_techniques.md` D1–D6).

## Data
- `JRPGEvent` (runtime node, child of a `JRPGMap`): `event_id` (int, stable per map, from `JRPGMap.next_event_id`),
  `display_name`, `note`, `cell` (Vector2i), `pages: Array[JRPGEventPage]`.
- `JRPGEventPage` (Resource, core): `conditions` (switch1, switch2, variable≥value, self switch A–D, item, actor),
  `graphic` (character sheet + index/direction/pattern, or a tile), `movement` (type fixed/random/approach/custom,
  speed 1–6, frequency 1–5, `route: JRPGMoveRoute`), options (walking anim, stepping anim, direction fix, through),
  `priority` (below/same/above characters), `trigger` (action, player touch, event touch, autorun, parallel),
  `commands: JRPGCommandList`.
- `JRPGCommandList` (Resource): `Array[JRPGEventCommand]`.
- `JRPGEventCommand` (Resource, abstract): typed subclasses with `@export`-like properties; branch commands own
  child lists (`JRPGCmdConditionalBranch.then_list/else_list`, `JRPGCmdShowChoices.branches[]` + cancel branch,
  `JRPGCmdLoop.body`, `JRPGCmdBattleProcessing.win/escape/lose` lists). No flat indentation format.
- Common events (`JRPGCommonEventData`): name, trigger (none/autorun/parallel), condition switch, `commands`.
- Troop battle event pages: conditions (turn end, turn A+B·X, enemy HP ≤%, actor HP ≤%, switch), span
  (battle/turn/moment), `commands`.

## Command metadata (registry)
Each command class declares (static, queried by the editor and the server):
| Field | Meaning |
|---|---|
| `category` | one of the 16 MZ categories (Message, Game Progression, Flow Control, Party, Actor, Movement, Character, Picture, Timing, Screen, Audio & Video, Scene Control, System Settings, Map, Battle, Advanced) or `Plugin` |
| `display_name`, `summary(command)` | list rendering in the event editor |
| `authority` | `SERVER` (changes game state), `CLIENT` (presentation only), `BOTH` (state change + presentation) |
| `usable_in` | map events / battle events / common events |
| `waits` | whether it can suspend the interpreter |

Custom/plugin commands: any script class extending `JRPGEventCommand` (found through
`ScriptServer::get_global_class_list`, `core/object/script_language.h:98`) implementing `_execute(interpreter)`;
parameters are its exported properties. `JRPGPluginRegistry` lists enabled plugin scripts in order.

## Interpreter (`JRPGInterpreter`, core, RefCounted)
- Stack of frames `{list: JRPGCommandList, index: int, loop_marker}`; executing a branch pushes its child list.
- `step(budget)` executes commands until one waits, the list ends, or the budget is exhausted
  (default 10 000 commands and 1 000 jumps per frame, rAthena-style runaway protection, rathena_techniques D2).
- Waits: `WAIT_FRAMES(n)`, `WAIT_MESSAGE`, `WAIT_CHOICE`, `WAIT_NUMBER`, `WAIT_MOVE_ROUTE(event)`,
  `WAIT_ANIMATION`, `WAIT_SCREEN_EFFECT`, `WAIT_SCENE(battle/shop/name/menu)`, `WAIT_PORT_REPLY(request_id)`.
- Labels resolve to **tree paths** (list of child indices) at load; Jump to Label may only target labels in the
  same command list or an enclosing list (editor validates; runtime falls back to "label not found" warning).
- Kinds: map event (bound to `map_id,event_id,page_index`), common event, battle event, parallel/autorun
  processes (one interpreter each).
- **Serializable state**: `{kind, owner ids, frames:[{list_ref:path-from-root, index}], wait, wait_data,
  local temp variables}`. `list_ref` addresses the list by its owner (e.g. map 3 / event 7 / page 1) plus child path,
  never by object pointer, so saving mid-event works and survives reloading the map scene (spec 06).
- Determinism: all randomness through `JRPGRandom` of the game state.

## Presentation port (`JRPGPresentationPort`, core interface)
Requests the interpreter needs from a presenter, all asynchronous with a request id:
`show_message(face, name, text, background, position)`, `show_choices(items, default, cancel, ...)`,
`input_number(digits, variable)`, `select_item(item_type, variable)`, `show_scrolling_text(...)`,
`picture_*`, `screen_effect(kind, params)`, `play_audio(kind, params)`, `play_movie`, `show_animation`,
`show_balloon`, `open_scene(kind, params)` (battle/shop/name/menu/save/game over/title).
- **Local port** (`JRPGLocalPort`, runtime): renders through the UI kit, replies when the player confirms.
- **Network port** (MMO, epic 19): serializes requests to the client, suspends the server interpreter until the
  reply arrives (rAthena dialog model, rathena_techniques D3), with timeouts (60 s menus/next, 180 s input; D4),
  validation of replies (choice index in range, number within digits), and disconnect cleanup (interpreter
  aborted, pending state discarded; D4).
- `CLIENT`-authority commands on the server become presentation requests; `SERVER`-authority commands never run on
  online clients.

## Page selection and triggers (single source of truth in core)
- Highest-numbered page whose conditions all hold is active (RM-EVT rule); refresh when switches/variables/self
  switches/items/party change (dirty flag, refresh at most once per frame).
- Autorun blocks player movement and other autoruns; parallel processes run each frame; an autorun that never
  ends is detected (no wait for N frames + no state change) and reported once (final-sweep edge case).

## Storage benchmark (gate before committing to typed command resources; run in task 9.2)
Generate a command list with 10 000 mixed commands (nested branches); measure `.tres` save, load, and re-save
byte-identity. Pass: load < 30 ms, save < 100 ms, byte-identical re-save. Fail → keep the same API but store
commands as packed `{code, params}` arrays behind `JRPGCommandList` (D-007 reversal).

## Edge cases (final sweep list)
Several pages qualify → highest wins; event commands with missing targets (event erased, map not loaded, entry ID
dangling) → no-op + warning; move routes blocked → "skip if cannot move" or wait; interpreter budget exceeded →
yield and continue next frame, warn if it repeats; saving mid-event; changing maps while a parallel process runs →
processes bound to the old map stop.
