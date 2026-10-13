# Claude bridge

The Claude bridge is an editor-only part of Talewright (`modules/claude_bridge`) that lets Claude Code — or any
MCP client — inspect and drive the editor and the game it runs: scene trees, node properties, project data,
run/stop, screenshots of the editor and of the game, input injection and logs. Design and reasons:
`plan/DECISIONS.md` D-012.

## How it works
- When the editor opens a project, the bridge starts an **MCP server** (Model Context Protocol, Streamable HTTP
  transport, JSON responses) on **127.0.0.1** only, port **47700** (or the next free port up to 47709).
- Every request needs `Authorization: Bearer <token>`. The token is 64 hex characters (256 bits from the engine's
  CSPRNG), created once per user at `%APPDATA%\Godot\claude_bridge\token` (the editor config directory) and
  compared in constant time. Requests whose `Host` is not loopback (DNS rebinding) or whose `Origin` is a web page
  are refused with 403.
- The editor writes `<project>/.godot/claude_bridge/connection.json` with the URL, port, token file and editor pid.
- Everything runs on the editor's main thread without blocking: slow or stalled clients are isolated and timed
  out, long tool calls complete asynchronously, and while a client is active the editor does not throttle itself
  when unfocused.
- The running game is reached through Godot's debugger connection: a small game-side component registers only in
  games launched by the editor. **Nothing of the bridge exists in export templates** (the module only builds for
  `target=editor`; verified by `talewright/tools/check_binary_strings.py`).

## Settings (Editor Settings)
- `claude_bridge/enabled` (default on) — start the server when a project opens.
- `claude_bridge/port` (default 47700) — first port tried.

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
| `project_info` / `project_settings_get` | Project summary / settings by prefix |
| `project_list_files` / `project_read_file` | Files inside `res://` (traversal outside the project is refused) |
| `run_project` / `stop_project` / `run_status` | Run (main, current or a given scene, with extra args), stop, status |
| `get_logs` / `clear_logs` | Editor and game output/errors (script errors carry the `res://` file and line) |
| `game_screenshot` | Capture of the running game |
| `game_scene_tree` / `game_node_get` / `game_node_set` | Live game tree and properties |
| `game_input` | Inject actions, keys or mouse buttons (taps with hold frames, or press/release) |
| `game_wait` | Wait N game frames |
| `game_call` | Call a method on a node in the game |
| `game_logs` | The game's own log buffer (complete even when debugger output is rate-limited) |

Values that JSON cannot express use Godot literal strings, e.g. `"Vector2(10, 20)"`, `"Color(1, 0, 0, 1)"`.

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
