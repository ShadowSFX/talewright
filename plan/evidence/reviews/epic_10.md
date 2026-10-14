# Review — Epic 10 (messages and UI kit: windows, lists, message text, message window, choices, screen stack)

Reviewers: two fresh-context subagents, 2026-10-14, read-only, on `git diff 86701dc67f..HEAD` (commits 10.1.1–10.3)
against tasks 10.1–10.3, D-048–D-054, spec 03 and the RM-MSG-001..019, RM-CMD-001..003, RM-MNU-016/017/019,
RM-INP-001/004 and RM-TST-015/016 rows. Part A: windows and lists (JRPGWindow, JRPGSelectableList, JRPGInputReader,
built-in skin, choice/number/gold windows): 21 findings, 1 high. Part B: message text, message window, screen stack,
bridge checks and decisions: 19 findings, 1 high. Both ran the filtered doctests (all passing); part A reproduced its
input findings with GDScript probes outside the repository.

Fixes: 10.4.1.1 (input and focus), 10.4.1.2 (drawing, theme and data), 10.4.1.3 (screen stack and checks). Each
finding below is marked with the task and fix as it lands.

## Part A — findings

| # | Sev | Finding | Fix |
|---|---|---|---|
| 1 | high | `JRPGInputReader::reset` cleared presses but not the repeat timers: a direction held when focus came in repeated on the next frame (D-050 says it is not a press for the new window) | 10.4.1.1: keys down at `reset()` are blocked until released (no press, no repeat); timers reset. Doctest "A key held when the focus comes in…" |
| 2 | med | Only the eight jrpg actions were accepted: Tab (`ui_focus_next`, also the default `jrpg_menu` key) and other `ui_*` focus keys moved the focus off the active window | 10.4.1.1: `handle_event` also recognizes the viewport's focus keys (`FOCUS_NAVIGATION`); focused windows accept them. Doctest "Focus keys stay with the focused window" |
| 3 | med | A closed window still passed hit tests and took the focus on a click (children too) | 10.4.1.1: `JRPGWindow::has_point` covers only the part that shows; closed: `MOUSE_BEHAVIOR_DISABLED` for the window and its children, focus released. Doctest "Clicks: a closed window takes none…" |
| 4 | med | One click on an inactive list both focused it and chose its item; an answered choice window could be refocused and replay the OK sound | 10.4.1.1: the click that gives a list the focus only moves the cursor; an answered choice window is `FOCUS_NONE` until `show_choices`. Same doctest, and "Right click anywhere cancels…" |
| 5 | med-low | `database` getter returned the project database, so the class default and saved scenes could hold it; every sound reloaded the database | pending (10.4.1.2) |
| 6 | med-low | Nine-patches drew `get_rid()` with the full size: AtlasTexture skin parts drew the wrong region | pending (10.4.1.2) |
| 7 | med-low | Nothing applied System data's window tone to windows (task 10.1.1, RM-MNU-016) | pending (10.4.1.2) |
| 8 | low | `set_max_polyphony(4)` had no effect: `set_stream` on every sound stops the playbacks | pending (10.4.1.2) |
| 9 | low | Icon-only choices counted as blank and were hidden; nothing handled "every choice blank" | pending (10.4.1.2) |
| 10 | low | `show_choices` cancel index not validated | pending (10.4.1.2) |
| 11 | low | Choice and number windows not clamped to the screen | pending (10.4.1.2) |
| 12 | low | Choice window's Dim background drew no dim band | pending (10.4.1.2) |
| 13 | low | Right click cancelled only over the focused list; no drag scrolling; no mouse on the number input | 10.4.1.1: right click anywhere cancels a focused list (`unhandled_input`). Drag/swipe and number input by mouse or touch moved to epic 28 (criterion added). Doctest "Right click anywhere cancels…" |
| 14 | low | Page actions always scrolled the list: no way to use them for actor switching (RM-TST-015) | 10.4.1.1: `page_mode` (`PAGE_SCROLL`, `PAGE_SIGNAL` with `page_requested(direction)`, `PAGE_NONE`). Same doctest |
| 15 | low | Repeats caught up one per frame after a long frame | 10.4.1.1: after a repeat the next one is an interval after now (`MAX(next, held) + interval`). Doctest "Repeats do not catch up after a long frame" |
| 16 | low | Scroll and pause marks overlapped the first and last rows | pending (10.4.1.2) |
| 17 | low | Coded item text had no ellipsis and dropped `right_text`; choice width ignored size codes | pending (10.4.1.2) |
| 18 | low | Number and gold windows skipped their own theme overrides; number input hard-coded the cursor margin; gold amount appeared only when fully open | pending (10.4.1.2) |
| 19 | low | `open()`/`close()` in the state already reached emitted at once (an `await` after the call hangs) | pending (10.4.1.2) |
| 20 | low | A list with a theme type variation fell back to the default 16 px font size | pending (10.4.1.2) |
| 21 | low | The sound player was a raw pointer exposed to scripts | pending (10.4.1.2) |

## Part B — findings

| # | Sev | Finding | Fix |
|---|---|---|---|
| 1 | high | `JRPGScreenStack.pop()` with a fade freed the screen and returned it (debug GDScript error "previously freed instance") | pending (10.4.1.3) |
| 2 | med | No re-entrancy guard: a second pop or push during a fade, the top screen still active while fading out | pending (10.4.1.3) |
| 3 | med | A covered screen's controls could still take the focus by click | pending (10.4.1.3) |
| 4 | med (latent) | Nothing stopped the player walking while a screen or message was active | 10.4.1.1: `JRPGSceneManager::_simulation_step` takes no new step while a control has the focus (D-044 amended). Doctest in `test_jrpg_scene_manager.h` |
| 5 | med | Message window always focusable (a click on a finished message took the focus from choices); Tab moved its focus away; click-anywhere needed the focus | 10.4.1.1: focusable only while a message is under way; focus keys accepted; clicks count while under way without the focus. Doctest "The message window: focus only while under way…" |
| 6 | med | Values inserted by `expand` were read again as codes (names with codes, self-expanding variables growing without bound) | pending (10.4.1.2) |
| 7 | low | Message height used the default padding, not the theme's | pending (10.4.1.2) |
| 8 | low | Layout and placement computed once: theme, font or screen size changes not followed | pending (10.4.1.2) |
| 9 | low | `show_message` on a closing window reverses the close without `closed` | pending (10.4.1.2) |
| 10 | low | Click-anywhere only where no stopping control is under the pointer; right click not handled | 10.4.1.1: left and right clicks both read the message while it is under way, on the window (`gui_input`) or anywhere unhandled; a control that takes the click keeps it (documented). Same doctest |
| 11 | low | Lines past the fourth and big fonts drawn outside the window | pending (10.4.1.2) |
| 12 | low | Reveal by raw elapsed time: jitter gives 0 then 2 letters; a hitch shows many letters at once | 10.4.1.1: a frame within 0.05 of whole 60ths counts that many letters; at most `MAX_LETTERS_PER_FRAME` = 4 a frame. Same doctest |
| 13 | low | `_activate` forced `PROCESS_MODE_INHERIT` and moved a focus the screen had chosen | pending (10.4.1.3) |
| 14 | low | Screens freed from outside stayed stacked | pending (10.4.1.3) |
| 15 | low | Docs: `\^` anywhere, PX/PY wording, D-052 vs RM-MSG-017/018, trailing `\!` merging with the end wait | pending (10.4.1.2) |
| 16 | low | Bridge checks: choice scene not mouse-proof, evidence rewritten each run, golden copy on an empty path | pending (10.4.1.3) |
| 17 | low | Screen stack signals not checked (SIGNAL_WATCH cannot watch script signals; error lines in the log) | pending (10.4.1.3) |
| 18 | low | Name box label ignored the message font | pending (10.4.1.2) |
| 19 | low | Reveal cut inside grapheme clusters | pending (10.4.1.2) |
