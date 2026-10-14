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
| 5 | med-low | `database` getter returned the project database, so the class default and saved scenes could hold it; every sound reloaded the database | 10.4.1.2.1: `database` returns what was set (null: nothing stored); `get_effective_database()` uses the project's, loaded once. Doctest "By default the tone and the sounds come from the System data…" |
| 6 | med-low | Nine-patches drew `get_rid()` with the full size: AtlasTexture skin parts drew the wrong region | 10.4.1.2.1: `JRPGWindow::draw_nine_patch`/`get_nine_patch_region` take the texture's region (`get_rect_region`, `get_scaled_rid`) for the frame and the list and number cursors. Doctest "Skin parts may be regions of one image" |
| 7 | med-low | Nothing applied System data's window tone to windows (task 10.1.1, RM-MNU-016) | 10.4.1.2.1: `use_system_tone` (default on): the back takes the System data's window tone; `tone` when off; `get_tone_in_use()`. The game state's Change Window Color override joins with that command (epic 24 note). Doctest "By default the tone…" |
| 8 | low | `set_max_polyphony(4)` had no effect: `set_stream` on every sound stops the playbacks | 10.4.1.2.1: the stream is set only when it changes, so a sound played again overlaps itself. Doctest in "Sounds come from the System data's cues" (the first playback stays active) |
| 9 | low | Icon-only choices counted as blank and were hidden; nothing handled "every choice blank" | 10.4.1.2.2: `is_blank_choice` (an icon is something to choose); every choice blank: an error, nothing shown, `choice_made` with the cancel choice (or `CANCEL_BRANCH`) on the next idle frame. Doctest "Icon choices show; all blank ends as cancelled…" |
| 10 | low | `show_choices` cancel index not validated | 10.4.1.2.2: a cancel index that is no choice is an error and cancel is refused. Same doctest |
| 11 | low | Choice and number windows not clamped to the screen | 10.4.1.2.2: choice and number windows clamped to the screen (choices also no taller than it). Doctest "Widths follow size codes and icons; the windows stay on the screen" |
| 12 | low | Choice window's Dim background drew no dim band | 10.4.1.2.2: the dim band moved to `JRPGWindow` (`dim_visible`, theme item `dim_color`); messages, their name box and choices use it. Doctests in the choice and message window cases |
| 13 | low | Right click cancelled only over the focused list; no drag scrolling; no mouse on the number input | 10.4.1.1: right click anywhere cancels a focused list (`unhandled_input`). Drag/swipe and number input by mouse or touch moved to epic 28 (criterion added). Doctest "Right click anywhere cancels…" |
| 14 | low | Page actions always scrolled the list: no way to use them for actor switching (RM-TST-015) | 10.4.1.1: `page_mode` (`PAGE_SCROLL`, `PAGE_SIGNAL` with `page_requested(direction)`, `PAGE_NONE`). Same doctest |
| 15 | low | Repeats caught up one per frame after a long frame | 10.4.1.1: after a repeat the next one is an interval after now (`MAX(next, held) + interval`). Doctest "Repeats do not catch up after a long frame" |
| 16 | low | Scroll and pause marks overlapped the first and last rows | 10.4.1.2.1: marks centered `mark_offset` (new theme constant, 6) from the edge: in the padding, over the border, clear of the rows. Goldens window, list and message regenerated on purpose |
| 17 | low | Coded item text had no ellipsis and dropped `right_text`; choice width ignored size codes | 10.4.1.2.2: one layout walk (`_layout_coded_text`) draws and measures coded text: ellipsis at the end, room left for `right_text`; `get_text_width` gives choice windows their width with sizes and icons. Doctest "Widths follow size codes…" |
| 18 | low | Number and gold windows skipped their own theme overrides; number input hard-coded the cursor margin; gold amount appeared only when fully open | 10.4.1.2.2: `_get_shared_font`/`_font_size`/`_constant` take the window's own overrides first; the number cursor uses the list's `cursor_margin`; the gold text draws while opening (clipped to the band). Doctests in the number and gold cases |
| 19 | low | `open()`/`close()` in the state already reached emitted at once (an `await` after the call hangs) | 10.4.1.2.1: already in the state asked for, `opened`/`closed` are emitted on the next idle frame (deferred). Doctest "Opening and closing…" |
| 20 | low | A list with a theme type variation fell back to the default 16 px font size | 10.4.1.2.2: the default theme gives a theme type variation the list's font size too (project themes still come first). Doctest "A theme type variation keeps the list's font size" |
| 21 | low | The sound player was a raw pointer exposed to scripts | 10.4.1.2.1: the player is kept as an ObjectID; freed from outside, it is made again with the next sound. Doctest in "Sounds come from the System data's cues" |

## Part B — findings

| # | Sev | Finding | Fix |
|---|---|---|---|
| 1 | high | `JRPGScreenStack.pop()` with a fade freed the screen and returned it (debug GDScript error "previously freed instance") | 10.4.1.3: `pop()` returns null unless `keep` (then the screen, out of the tree, its state given back) and frees the screen after the fade. Doctest "Fades: the stack refuses changes until the end…" (the coroutine's `completed` value) |
| 2 | med | No re-entrancy guard: a second pop or push during a fade, the top screen still active while fading out | 10.4.1.3: the lock covers a faded operation from start to end (it was dropped between the halves, so a `screen_pushed` handler could re-enter); the top screen is suspended before the fade; clicks stop at the fade. Same doctest |
| 3 | med | A covered screen's controls could still take the focus by click | 10.4.1.3: covered screens get `focus_behavior_recursive` and `mouse_behavior_recursive` DISABLED, given back on top. Doctest "Covered screens take no clicks…" |
| 4 | med (latent) | Nothing stopped the player walking while a screen or message was active | 10.4.1.1: `JRPGSceneManager::_simulation_step` takes no new step while a control has the focus (D-044 amended). Doctest in `test_jrpg_scene_manager.h` |
| 5 | med | Message window always focusable (a click on a finished message took the focus from choices); Tab moved its focus away; click-anywhere needed the focus | 10.4.1.1: focusable only while a message is under way; focus keys accepted; clicks count while under way without the focus. Doctest "The message window: focus only while under way…" |
| 6 | med | Values inserted by `expand` were read again as codes (names with codes, self-expanding variables growing without bound) | 10.4.1.2.3: values put in by `expand` come out escaped (text, never codes): a name with codes shows them, a self-referencing variable is inserted once; new `escape()`. Doctest "Inserted values are text, never codes" (also `\N[0]`, `\P[0]`, decimals) |
| 7 | low | Message height used the default padding, not the theme's | 10.4.1.2.3: the height uses the theme's padding. Doctest "Text stays inside the window and follows theme and screen changes" |
| 8 | low | Layout and placement computed once: theme, font or screen size changes not followed | 10.4.1.2.3: the parsed operations are kept and laid out again on THEME_CHANGED and on `set_text_colors`; placement follows the viewport's `size_changed`; the reveal goes on where it was. Same doctest |
| 9 | low | `show_message` on a closing window reverses the close without `closed` | 10.4.1.2.1: documented on `open()`/`close()` (a closing window turns round, `closed` not emitted). Doctest "Opening and closing…" |
| 10 | low | Click-anywhere only where no stopping control is under the pointer; right click not handled | 10.4.1.1: left and right clicks both read the message while it is under way, on the window (`gui_input`) or anywhere unhandled; a control that takes the click keeps it (documented). Same doctest |
| 11 | low | Lines past the fourth and big fonts drawn outside the window | 10.4.1.2.3: text below the fourth line (by new lines or `\PY`) is left out with a warning when shown; text is drawn on its own canvas item clipped to the window, so big sizes stay inside. Same doctest |
| 12 | low | Reveal by raw elapsed time: jitter gives 0 then 2 letters; a hitch shows many letters at once | 10.4.1.1: a frame within 0.05 of whole 60ths counts that many letters; at most `MAX_LETTERS_PER_FRAME` = 4 a frame. Same doctest |
| 13 | low | `_activate` forced `PROCESS_MODE_INHERIT` and moved a focus the screen had chosen | 10.4.1.3: `_activate` gives back the process mode, focus and mouse behavior the screen had, keeps a focus the screen chose itself, and falls back to its next focusable control when the saved one is gone. Doctests in the first and third cases |
| 14 | low | Screens freed from outside stayed stacked | 10.4.1.3: a screen leaving the tree by itself is taken off (`screen_popped`) and the one under it becomes active; freed ones are also filtered. Doctest "…screens that free themselves" |
| 15 | low | Docs: `\^` anywhere, PX/PY wording, D-052 vs RM-MSG-017/018, trailing `\!` merging with the end wait | 10.4.1.2.3: docs and header comments: `\^` anywhere; `\PX`/`\PY` in pixels inside the padding (the help's "window corner" noted as a deviation in D-052); a trailing `\!` is the end's wait (one press) |
| 16 | low | Bridge checks: choice scene not mouse-proof, evidence rewritten each run, golden copy on an empty path | 10.4.1.3: the choice scene ignores the pointer (`mouse_filter = 2`); evidence is written only with `--write-evidence` (a plain run changes no repository file); no golden is written or compared from a missing screenshot |
| 17 | low | Screen stack signals not checked (SIGNAL_WATCH cannot watch script signals; error lines in the log) | 10.4.1.3: the script's signals are counted through `callable_mp_static` callables (no SignalWatcher errors in the log); every push, pop, replace and clear is checked |
| 18 | low | Name box label ignored the message font | 10.4.1.2.3: the name label takes the message font as well as its size. Same doctest |
| 19 | low | Reveal cut inside grapheme clusters | 10.4.1.2.3: pieces count grapheme clusters (`TextServer::string_get_character_breaks`), and the reveal cuts at cluster ends. Same doctest (an e with a combining accent is one letter with a break-iterator text server) |

## Close-out (10.4.2)

- Fixes: 10.4.1.1 input and focus (e03f8a993a), 10.4.1.2.1 window, skin and data (629d4c5407), 10.4.1.2.2 lists,
  choices, number and gold windows (df06a99e4b), 10.4.1.2.3 message text and message window (7b8a0c25ef), 10.4.1.3
  screen stack and checks (6b6fb91483). All 40 findings fixed; parts deferred on purpose: drag/swipe scrolling and
  mouse or touch on the number input (A13, epic 28 criterion), the game state's Change Window Color reaching windows
  (A7, epic 24 note).
- Goldens regenerated on purpose once (10.4.1.2.1: the scroll and pause marks moved into the padding), after a
  visual check of the window, list and message images.
- plain_2d/plain_3d smoke: ALL PASS with a rebuilt release template (`talewright/build_logs/smoke-20261014-133843.log`).
- core_diff_check: 5 core paths changed, 0 unlisted.
- Full test suite: 1620 cases, all pass; release template (`template-tests`): 1506 cases, all pass. One run of the
  full suite earlier in 10.4.1.1 crashed in the upstream `[Image] Saving and loading` case (SIGSEGV) and passed in
  every run since (not reproduced; watched).
- Static checks: ALL PASS. Bridge checks: 188/188, and a plain run leaves the repository untouched.
