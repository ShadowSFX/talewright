# Claude bridge

The Claude bridge is an editor-only part of Talewright (`modules/claude_bridge`) that lets Claude Code — or any
MCP client — inspect and drive the editor and the game it runs: scene trees, node properties, project data,
run/stop, screenshots of the editor and of the game, input injection and logs. Design and reasons:
`plan/DECISIONS.md` D-012.

## How it works
- When the editor opens a project, the bridge starts an **MCP server** (Model Context Protocol, Streamable HTTP
  transport, JSON responses) on **127.0.0.1** only, port **47700**. By default only that port is tried, so a
  client can never reach a different editor by accident; `claude_bridge/port_fallback` allows the next free port
  up to 47709. The bridge does not start in command-line runs of the editor (`--export-*`, `--script`, ...).
- Every request needs `Authorization: Bearer <token>`. The token is 64 hex characters (256 bits from the engine's
  CSPRNG), created once per user in the editor config directory (Windows `%APPDATA%\Godot\claude_bridge\token`,
  Linux `~/.config/godot/claude_bridge/token`, macOS `~/Library/Application Support/Godot/claude_bridge/token`),
  made owner-only (0600) where the OS supports it, and compared in constant time. Requests whose `Host` is not
  loopback (DNS rebinding) or whose `Origin` is a web page are refused with 403; 401/403/4xx framing errors close
  the connection. Bodies over 1 MiB, non-UTF-8 bodies and malformed headers are refused before any tool runs.
- The editor writes `<project>/.godot/claude_bridge/connection.json` with the URL, port, token file and editor pid
  (and removes it on exit only if it still holds its own pid). Clients must treat that file as untrusted (it lives
  in the project): `bridge.py` accepts only loopback URLs from it, always reads the token from the per-user path
  above (or `--token-file`) and never sends bridge traffic through HTTP proxies.
- Everything runs on the editor's main thread without blocking: slow or stalled clients are isolated and timed
  out, long tool calls complete asynchronously, and while a client is active the editor does not throttle itself
  when unfocused.
- The running game is reached through Godot's debugger connection: a small game-side component registers only in
  games launched by the editor. **Nothing of the bridge exists in export templates** (the module only builds for
  `target=editor`; verified by `talewright/tools/check_binary_strings.py`).

## Settings (Editor Settings)
- `claude_bridge/enabled` (default on) — start the server when a project opens.
- `claude_bridge/port` (default 47700) — the port to listen on.
- `claude_bridge/port_fallback` (default off) — if the port is busy, try the next 9 ports instead of failing.
Changing a `claude_bridge/*` setting restarts the server and closes the connections open at that moment (if
the change arrives from inside a tool call, the restart waits until that poll finishes).

## Connecting Claude Code
Register the server once (user scope; replace the port if `connection.json` shows another one):

```
claude mcp add --scope user --transport http talewright http://127.0.0.1:47700/mcp --header "Authorization: Bearer <token>"
```

where `<token>` is the content of the token file (PowerShell: `Get-Content $env:APPDATA\Godot\claude_bridge\token`).
New Claude Code sessions then see the tools below. Inside a session that started before the editor, use the CLI.

## Command-line client
`python talewright/tools/bridge.py` speaks the same protocol over HTTP:

```
python talewright/tools/bridge.py launch --project talewright/samples/bridge_probe        # start an editor and wait
python talewright/tools/bridge.py --project <dir> list                                    # tools
python talewright/tools/bridge.py --project <dir> call editor_screenshot max_width=1280   # image saved, path printed
python talewright/tools/bridge.py --project <dir> call game_input action=ui_accept
```

Images are saved under `talewright/build_logs/bridge/` by the CLI and under `<project>/.godot/claude_bridge/screenshots/`
by the editor. Exit codes: 0 ok, 1 the tool reported an error, 2 transport/protocol error.

## Tools
| Tool | What it does |
|---|---|
| `bridge_status` | Server/engine version, project, port, tool count |
| `editor_scene_tree` | Tree of the edited scene and list of open scenes |
| `editor_node_get` / `editor_node_set` | Read / write (undoable) node properties in the edited scene |
| `editor_open_scene` / `editor_save_scene` | Open / save scenes |
| `editor_select` | Select nodes and show them in the inspector |
| `editor_set_main_screen` | Switch 2D / 3D / Script / Game / plugin screens |
| `editor_undo_redo` | Undo or redo the last editor action |
| `editor_rescan` | Rescan project files (the editor otherwise rescans only on focus) |
| `editor_screenshot` | Window, main screen, 2D or 3D viewport capture |
| `editor_ui_find` / `editor_ui_act` | Find visible editor controls and dialogs (by text, tooltip, placeholder, list item, class) / use one like a user: press, toggle, type (`set_text`, `submit`), `set_value`, `select` or `activate` a list, tree, option, tab or menu item, `confirm` or `cancel` a dialog (D-031) |
| `project_info` / `project_settings_get` | Project summary / settings by prefix |
| `project_settings_set` | Change an existing project setting (or `reset` it to its default) and save `project.godot`; not undoable |
| `project_list_files` / `project_read_file` | Files inside `res://` (traversal outside the project is refused) |
| `run_project` / `stop_project` / `run_status` | Run (main, current or a given scene, with extra args), stop, status |
| `get_logs` / `clear_logs` | Editor and game output/errors (script errors carry the `res://` file and line) |
| `game_screenshot` | Capture of the running game |
| `game_scene_tree` / `game_node_get` / `game_node_set` | Live game tree and properties |
| `game_input` | Inject actions, keys or mouse buttons (taps with hold frames, or press/release) |
| `game_wait` | Wait N game frames |
| `game_call` | Call a method on a node in the game |
| `game_logs` | The game's own log buffer (complete even when debugger output is rate-limited) |
| `jrpg_setup_project` | Set up the JRPG project (same as Project > Tools > Set Up JRPG Project...): creates only what is missing and reports it; `show_report` also shows the report dialog |
| `jrpg_addon_status` / `jrpg_addon_install` | State of the managed addon (`res://addons/talewright`) / install or upgrade it, keeping files the user changed |
| `jrpg_workspace_state` / `jrpg_workspace_select` | The JRPG workspace (set up, open, current section and map mode, database tables) / show the Map or Database section |
| `jrpg_db_list` | Database tables (maximum, file, unsaved) or one table's entries as `{id, name}` (`from`, `count`) |
| `jrpg_db_get` / `jrpg_db_set` | Read an entry's (or System/Types/Terms) editable properties, with the table each ID property points into / set several properties as one undoable action (all-or-nothing; JSON values, Godot literal strings for engine types) |
| `jrpg_db_set_maximum` / `jrpg_db_clear` | Change a table's maximum / clear an entry, undoable; refused with the list of references that would break unless `allow_broken_references`; `show_dialog` opens the editor's confirmation dialog instead |
| `jrpg_db_copy` / `jrpg_db_paste` | Copy an entry to the database clipboard (shared with the Database section) / paste it over an entry of the same table, undoable |
| `jrpg_db_save` | Save the database files with unsaved changes |
| `jrpg_db_select` | Show the Database section on a table (or `system`, `types`, `terms`) and select an entry |

The `jrpg_*` tools come from the JRPG module through the bridge's tool registry (D-030): modules add tools without the
bridge depending on them, and each module still builds alone.

Notes on arguments and results:
- **Values** that JSON cannot express use Godot literal strings, e.g. `"Vector2(10, 20)"`, `"Color(1, 0, 0, 1)"`.
  Only math/packed-array constructors, numbers, `true/false/null` and `inf/nan` are accepted (no `Object(...)`,
  `Resource(...)` or other constructors). Resource-typed properties take a `res://` path of an existing resource of
  the property's class (or `null` to clear). `editor_node_*` only reach nodes inside the edited scene.
- **Paths**: `res://` only; `..`, backslashes and other drive/scheme prefixes are refused.
- **`run_project` args** are passed after `--`, so the game reads them with `OS.get_cmdline_user_args()` and they
  can never be taken as engine options.
- **Game tools** accept `instance` (0 = the first connected game, default) when several game instances run. A
  request whose game stops before replying fails with "The game stopped before replying.".
- **Annotations**: every tool declares `readOnlyHint`; tools that modify something also declare `destructiveHint`
  (false for undoable or transient actions such as `editor_node_set`, `run_project`, `game_input`; true for
  saving scenes, clearing logs, `game_node_set` and `game_call`).
- **Large values** are summarized: strings over 64K characters are truncated and packed arrays over 1024
  elements are reported by type and size.

## Troubleshooting
- *connection.json not found*: the editor is not running for that project or `claude_bridge/enabled` is off.
- *Screenshots fail*: headless editors and minimized windows cannot render; viewport captures need the viewport
  visible (`editor_set_main_screen`).
- *Game tools say the game is not connected*: call `run_project`, then poll `run_status` until `debugger_connected`.
- *Port changed*: another program holds 47700; read the port from `connection.json`.

## Tests
- Unit tests: `bin/godot.windows.editor.x86_64.console.exe --headless --test --test-case="*ClaudeBridge*"`.
- Robustness: `python talewright/tools/bridge_stress.py --project <dir>` against a running editor.
- Absence from templates: `python talewright/tools/check_binary_strings.py --absent bin/godot.windows.template_release.x86_64.exe`.
