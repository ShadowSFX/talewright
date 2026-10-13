# Godot 4.7.2: run/debugger/game view/logs/screenshots/networking/headless/EditorInterface + MCP prior art

_Planning-phase research report (2026-10-13), produced by a read-only research subagent and kept verbatim as evidence. Godot citations are at tag 4.7.2-stable; they are re-verified against the clone in task 2.2/2.3._

---

I couldn't clone, so everything below was read straight from tag `4.7.2-stable` (tree fetched with `truncated:false`, 15,212 entries). Godot claims cite `path:line` at that tag; web claims cite URLs.

Five findings matter most for the design:
- **Game output never reaches print handlers in the editor process.** The editor writes it straight into EditorLog, so you have to hook `ScriptEditorDebugger` signals instead (section 4).
- **A game screenshot round-trip already exists** (`scene:rq_screenshot` → `game_view:get_screenshot`), and you can reuse it without the Game view (section 2.3).
- **Editor-level module init also runs inside the game process.** The game is the same TOOLS_ENABLED binary (section 1).
- **Godot has no general HTTP server**, and `TCPServer::listen` binds to `*` (all interfaces) unless you pass an address (section 6).
- **The newest MCP spec (2026-07-28) changed Streamable HTTP substantially** (section 9).

---

## 1. Running the project

**Where the code lives.** `EditorRun` is in `editor/run/editor_run.{h,cpp}`. It is a plain class held as a member of `EditorRunBar` (`editor/run/editor_run_bar.h:80`).

**How the arguments are built** (`EditorRun::run`, editor_run.cpp:51-196), in order:
- Forwarded CLI args from `Main::get_forwardable_cli_arguments(CLI_SCOPE_PROJECT)` (54).
- `--path <res path with %20>` (58-62).
- `--remote-debug <EditorDebuggerNode::get_server_uri()>` (64-68).
- `--editor-pid <editor pid>` (70-71).
- Optional debug flags: `--debug-collisions`, `--debug-paths`, `--debug-navigation`, `--debug-avoidance`, `--debug-canvas-item-redraw`, `--debug-mute-audio` (73-102).
- When recording: `--write-movie <f> --fixed-fps N [--disable-vsync]` (104-112).
- `--position x,y` and `--maximized`/`--fullscreen` (114-124).
- `--breakpoints a,b` (126-140), `--skip-breakpoints` (142-144), `--ignore-error-breaks` (146-148).
- `--scene <path>` (150-153), then any user run args (155-159).

**What it launches.**
- The executable is `OS::get_singleton()->get_executable_path()`, i.e. the editor binary itself (editor_run.cpp:161).
- Each instance is started with `OS::create_instance(args, &pid)` (182). That is defined as `create_process(get_executable_path(), ...)` (`core/os/os.h:218`) and does not block.
- Before launch, `instance_starting_callback` can rewrite the arguments (167-169). The Game view uses this hook.
- Stopping calls `OS::kill(pid)` for every child (222-232).

**Programmatic start/stop** (`editor/editor_interface.h:194-201`):
- `play_main_scene()` (196), `play_current_scene()` (197), `play_custom_scene(path)` (198).
- `stop_playing_scene()` (199), `is_playing_scene()` (200), `get_playing_scene()` (201).
- Each forwards to `EditorRunBar` (editor_interface.cpp:794-816).

**What `EditorRunBar::_run_scene` does:**
- `ensure_main_scene(false)` can pop up a dialog if no main scene is set (editor_run_bar.cpp:342). That would stall an agent.
- Then `try_autosave()` (350) and `call_build()` (351).
- Then `EditorDebuggerNode::start(uri)` (363) and `editor_run.run(...)` (364).
- Every `play_*` call runs `stop_playing()` first (424, 440, 456).
- `stop_playing` → `editor_run.stop()` + `EditorDebuggerNode::stop()` (462-477).
- Run args are also accepted: `play_main_scene(bool, Vector<String> args)` (editor_run_bar.h:123-125), and `get_current_process()` (135).

**Is the game TOOLS_ENABLED?** Yes, it is the same binary.
- Consequence: editor-level module init runs unconditionally in any TOOLS build, including the game child process: `register_editor_types(); initialize_modules(MODULE_INITIALIZATION_LEVEL_EDITOR)` (main/main.cpp:3818-3825, inside `Main::setup2`, 3033).
- Module editor plugins are only instantiated when an `EditorNode` exists: `EditorPlugins::add_by_type<T>()` (`editor/plugins/editor_plugin.h:303`) is consumed at editor_node.cpp:9420-9421. Pattern: modules/gridmap/register_types.cpp:50-54.
- Any non-plugin init you add at the EDITOR level must check `Engine::is_editor_hint()` itself.

**How the game knows it was launched from the editor:**
- `is_editor_hint()` is false. It is only set when `--editor` is passed (main.cpp:2222-2223, 2355-2356), and `EditorRun` never adds it. The getter is at `core/config/engine.h:172`.
- `EngineDebugger::is_active()` is true. That means `singleton && script_debugger` (`core/debugger/engine_debugger.h:106`), which `--remote-debug` sets up via main.cpp:1923-1942 → `EngineDebugger::initialize` (main.cpp:2300, in `Main::setup`) → `RemoteDebugger` created and a `set_pid` message sent (engine_debugger.cpp:136-149).
- When embedded, `--wid` sets `Engine::set_embedded_in_editor(true)` (main.cpp:2052-2061). The getter is `is_embedded_in_editor()` (engine.h:225).

**Keeping it out of exported games.** Use `def can_build(env, platform): return env.editor_build`, as in modules/xatlas_unwrap/config.py:1-2 and modules/lightmapper_rd/config.py:1-2. `editor_build` is `target=="editor"` (SConstruct:533), and TOOLS_ENABLED is only added for editor builds (SConstruct:549). Editor builds also define DEBUG_ENABLED (SConstruct:535, 554).

## 2. Remote debugger protocol

### 2.1 Core
- **Captures:** `register_message_capture(name, Capture(void*, CaptureFunc))` (engine_debugger.h:118). The signature is `Error(*)(void*, const String&, const Array&, bool &r_captured)` (49). Registering a duplicate name errors (engine_debugger.cpp:56-57).
- **Sending:** `send_message(msg, Array)` is pure virtual (engine_debugger.h:137).
- **Script access:** GDScript can use the same API through the `EngineDebugger` singleton (`core/core_bind.h:645-671`).

**Wire format:**
- Each frame is a 4-byte length followed by `encode_variant(Array)` (`core/debugger/remote_debugger_peer.cpp:114-119`).
- The Array is `[name, thread_id, data]` in both directions: game→editor at remote_debugger.cpp:105, editor→game at script_editor_debugger.cpp:82.
- Buffers are 8 MiB; larger packets are dropped or disconnected (remote_debugger_peer.cpp:84-85, 116, 147-148).
- The game-side peer runs its own thread, polling no more often than every 6.9 ms (196-211).
- Supported URIs: `tcp://` and, on Unix only, `unix://` (engine_debugger.cpp:124-127; remote_debugger_peer.cpp:226-262).
- The websocket module adds `ws://` and `wss://` (modules/websocket/register_types.cpp:58, 73-74).

**Game-side dispatch** (remote_debugger.cpp:652-687):
- Messages are processed on the main thread through `EngineDebugger::iteration` → `poll_events(true)` (engine_debugger.cpp:107-120; main.cpp:5114-5115).
- `"prefix:msg"` goes to capture `prefix` with `msg`; messages without a prefix go to `"core"`.

**Output and error reporting:**
- `RemoteDebugger` installs a print handler and an error handler (remote_debugger.cpp:793-799).
- Prints are flushed as `"output"` = `[PackedStringArray strings, PackedInt32Array types]` with types LOG / ERROR / LOG_RICH (remote_debugger.cpp:206-240; remote_debugger.h:41-45).
- Errors are sent as `"error"` = an `OutputError` array: hr, min, sec, msec, file, func, line, error, descr, warning, then N×3 call-stack entries (core/debugger/debugger_marshalls.cpp:101-121).
- Rate limits: 32768 chars/s, 2048 queued messages, 400 errors/s, 400 warnings/s (main.cpp:2295-2298). Overflow produces `"[output overflow, print less text!]"` (remote_debugger.cpp:155-174).
- `evaluate` is only handled inside the break loop (remote_debugger.cpp:549-618), so it only works while the game is paused at a breakpoint.

**Default editor server.** It listens on `tcp://127.0.0.1:6007` (editor/settings/editor_settings.cpp:1142-1143; editor/debugger/editor_debugger_server.cpp:83-110) and tries the next ports if that one is busy (94-106).

### 2.2 Scene debugger (game side, `scene/debugger/scene_debugger.cpp`)
- It registers capture `"scene"` only in DEBUG_ENABLED builds (74) and only when the debugger is active (95-101). The full handler table is at 608-662.

**Scene tree:**
- `scene:request_scene_tree` (163-166) → `LiveEditor::_send_tree` → reply `"scene:scene_tree"` (837-847).
- The payload is a flat depth-first list with 6 fields per node: child_count, name, type (script global name, else script path, else class), instance id, scene_file_path, view_flags (`scene_debugger_object.cpp:278-338`).

**Inspecting objects:**
- `scene:inspect_objects [ids[], update_selection]` (177-185 → `_send_object_ids` 699-745) replies `"scene:inspect_objects"`. If `update_selection` is true, it replies `"remote_objects_selected"` instead and also changes the in-game selection (716-729, 743). Pass `false`.
- Each object serializes as `[id, class, props]`, and each prop as `[name, type, hint, hint_string, usage, value]` (scene_debugger_object.cpp:189-224).
- Resources that have a path are sent as the path string (200-205).
- Values larger than 1 MiB are replaced with `PROPERTY_HINT_OBJECT_TOO_BIG` (207-213; `p_max_size = 1<<20` in scene_debugger_object.h:57).
- Script members appear as `Members/...` and constants as `Constants/...` (141-186).
- The old single-object `scene:inspect_object` still exists unless DISABLE_DEPRECATED (187-205).

**Writing properties:** `scene:set_object_property [id, prop, value]` and `..._field` (287-299, 747-771).

### 2.3 Editor side
**ScriptEditorDebugger** (`editor/debugger/script_editor_debugger.cpp`):
- Message processing is capped at about 20 ms per frame (1200-1213).
- `_parse_message` first emits `debug_data(msg, data)` for every message (984), then dispatches through the table (1002-1036): `"output"` (1018), `"error"` (1022), `"scene:scene_tree"` (1007), `"scene:inspect_objects"` (1008).
- Only unknown messages that have a prefix go to plugins (989-997).
- `_msg_output` calls `EditorNode::get_log()->add_message` directly and emits `output(msg, level)` (561-593).
- `_msg_error` builds the error tree (630+). Apart from `debug_data`, it emits no dedicated signal.
- Public API: `send_message` (h:394), `request_remote_tree` (h:323, sends at cpp:267-269), `get_remote_tree()` (h:324), `request_remote_objects` (h:315), `update_remote_object` (h:316), `is_session_active` (h:342), `get_error_count` / `get_warning_count` (h:348-349).
- Signals are listed at 2055-2077: `output` (2071), `debug_data` (2075), `remote_tree_updated` (2068), `started` / `stopped` (2055-2056).

**EditorDebuggerNode** (`editor/debugger/editor_debugger_node.h`):
- Debugger access: `get_current_debugger` / `get_debugger(i)` (172-174).
- Plugins: `add_debugger_plugin` / `remove_debugger_plugin` (228-229).
- Server: `get_server_uri` (221), `start(uri)` / `stop` (224-225).
- `plugins_capture` (editor_debugger_node.cpp:946-960) requires a colon in the message name and calls every plugin whose `has_capture(prefix)` matches.
- When a new session tab is added, every plugin gets `create_session` → `setup_session(idx)` (editor_debugger_node.cpp:149-153; editor_debugger_plugin.cpp:151-158).
- The remote tree auto-refreshes on a timer (editor_debugger_node.cpp:378-384).

**EditorDebuggerPlugin and EditorDebuggerSession** (`editor/debugger/editor_debugger_plugin.h`):
- Plugin: `capture` / `has_capture` (86-87) and the virtuals `_capture`, `_has_capture`, `_setup_session` (92-94).
- Session: `add_session_tab` (58), `send_message` (60), `toggle_profiler` (61), `is_active` (64).

**Plugins never receive `"output"` or `"error"`.** Both are handled built-in, and neither has a colon. To capture game logs and errors, connect to `output` / `debug_data` on each `ScriptEditorDebugger`.

**The existing screenshot message.** It exists and lives in `editor/run/`, not `editor/plugins/`:
- Editor: `GameViewDebugger::add_screenshot_callback` sends `"scene:rq_screenshot" [id]` to every active session (editor/run/game_view_plugin.cpp:366-383).
- Game: `_msg_rq_screenshot` reads `root->get_texture()->get_image()`, converts to RGBA8, applies sRGB if HDR-2D, and saves a PNG to `OS::get_temp_path()` (scene_debugger.cpp:529-560). It replies `"game_view:get_screenshot" [id, w, h, path]` (562-567).
- Editor: the reply is matched by `capture` / `_msg_get_screenshot` (game_view_plugin.cpp:385-428; `has_capture("game_view")` at 426-428).
- Hook: `EditorRun::request_screenshot(Callable)` (editor_run.cpp:198-204; h:82) only works when the game is embedded and not floating (game_view_plugin.cpp:497-507).
- Use in the editor: `EditorNode::_screenshot` composites the game image over the editor capture and then deletes the temp PNG (editor_node.cpp:3999-4037).

**Reusing it from the bridge (my inference from the code above):**
- Send `scene:rq_screenshot` yourself through `ScriptEditorDebugger::send_message` or a session, and register your own plugin with `has_capture("game_view")`. That works even when the game is not embedded.
- Use ids well above GameView's counter, which starts at 0 (`scr_rq_id`, game_view_plugin.h:71). An unknown id is ignored (393-398), but a colliding id would fire GameView's callback.

**"screenshot" search.** Across `editor/debugger`, `core/debugger`, `scene/debugger`, `servers/debugger` and `editor/run`, the only matches are editor_run.{h,cpp}, game_view_plugin.{h,cpp} and scene_debugger.{h,cpp}. There is no other capture message.

## 3. Game view and embedding (4.7)

**Files.**
- `editor/run/game_view_plugin.{h,cpp}` defines `GameViewDebugger : EditorDebuggerPlugin` (h:45), `GameView` (h:119), `GameViewPluginBase` (h:289; main screen named "Game", h:315-316) and `GameViewPlugin` (h:332).
- Embedding is done by `editor/run/embedded_process.{h,cpp}`: `EmbeddedProcessBase` / `EmbeddedProcess`, with a 45 s embed timeout (h:101).
- macOS has its own variant in `platform/macos/editor/embedded_*`.

**How embedding works:**
- `DisplayServer::embed_process(window_id, pid, rect, visible, grab_focus)` (embedded_process.cpp:245, 304).
- Argument rewrite (`_update_arguments_for_instance`, game_view_plugin.cpp:1271-1340):
  - It removes `--position`, `--resolution`, `--screen`, `-f`/`-m`/`-t`, and `--display-driver` on macOS, Wayland and X11.
  - It adds `--wid <native handle>`, plus `--embedded` on macOS (1325) and the right display driver on Wayland/X11.
- Embedding needs `FEATURE_WINDOW_EMBEDDING` (game_view_plugin.cpp:860, 1160). Windows reports it as supported (platform/windows/display_server_windows.cpp:192).
- The mode comes from the editor setting `run/window_placement/game_embed_mode` (editor_settings.cpp:1100-1109; game_view_plugin.cpp:1162-1180).
- On ready, the plugin installs both `EditorRun` callbacks (game_view_plugin.cpp:1186-1187).

**Runtime controls (editor → game messages):**

| Control | Message | Game handler |
|---|---|---|
| Suspend / resume | `scene:suspend_changed [bool]` (game_view_plugin.cpp:184-190) | scene_debugger.cpp:212-218 |
| Next frame | `scene:next_frame` (195-198) | 220-223; unsuspends for one frame via `frame_post_draw` (773-780) |
| Speed | `scene:speed_changed [scale]` (209, 220) | 225-230 |
| Node picking | `scene:runtime_node_select_{setup,set_type,set_mode,set_visible,set_avoid_locked,set_prefer_group,reset_camera_2d/3d}` (128-144, 233-338) | — |
| Camera override | `scene:override_cameras`, `transform_camera_2d/3d` | 275-285, 487-522 |
| Mute | `scene:debug_mute_audio` | — |
| Window size / HDR | `scene:window_request_size`, `hdr_output_*` | — |
| Focus | `scene:report_window_focused` | — |

- Session setup sends `setup_embedded_shortcuts` and `setup_game_view` (game_view_plugin.cpp:150-155).
- Game → editor: `game_view:setup_complete`, `game_view:window_size`, `game_view:hdr_state`, `game_view:get_screenshot` (game_view_plugin.cpp:406-416); `request_embed_suspend_toggle` / `request_embed_next_frame` (scene_debugger.cpp:119-140); and selection results `remote_objects_selected` / `remote_nothing_selected` (scene/debugger/runtime_node_select.cpp:556-684).

## 4. Editor output and errors

**EditorLog** is in `editor/editor_log.h`, not `editor/gui/`. It derives from `EditorDock` (42).
- `MessageType` values: `MSG_TYPE_STD`, `ERROR`, `STD_RICH`, `WARNING`, `EDITOR` (46-52).
- `add_message(msg, type)` is public (182) and splits text on newlines (editor_log.cpp:282-294).
- Messages are kept in a private `Vector<LogMessage> messages` (h:129) with no public getter, so the bridge needs its own ring buffer.
- The line limit is `run/output/max_lines`, default 10000 (editor_settings.cpp:1122).

**Handler APIs:**
- Prints: `add_print_handler(PrintHandlerList*)` / `remove_print_handler` (`core/string/print_string.h:40-52`). The callback is `void(void*, const String&, bool p_error, bool p_rich)`.
- Errors: `add_error_handler(ErrorHandlerList*)` / `remove_error_handler` (`core/error/error_macros.h:61-71`). The callback receives func, file, line, error, message, editor_notify and type, where type is ERROR / WARNING / SCRIPT / SHADER (38-43).
- Both handlers are called under `_global_lock()` on whichever thread printed (print_string.cpp:99-106; error_macros.cpp:133-137). Handlers must be fast and thread-safe.
- Follow the editor's own pattern and defer to the main thread when not on it: `callable_mp_static(...).call_deferred` (editor_node.cpp:7912-7917), or `MessageQueue::get_main_singleton()->push_callable` (editor_log.cpp:66-67). Supporting APIs: `Thread::is_main_thread` (core/os/thread.h:130), `push_callable` (core/object/message_queue.h:123).
- Alternative: `OS::add_logger(Logger*)` (os.h:388). `Logger::log_error` also receives script backtraces (core/io/logger.h:88).

**Can a module register its own handlers in the editor process?** Yes. EditorNode and EditorLog already do exactly that (editor_node.cpp:9596-9598; editor_log.cpp:588-590). But these handlers only see the editor's own output.

**Game output is not routed through print handlers.** It arrives as the debugger `"output"` message and goes straight into `EditorLog::add_message` (script_editor_debugger.cpp:590). For game logs and errors, connect to `output` and `debug_data` on each `ScriptEditorDebugger` (signals at 2071 and 2075; hook new sessions through `setup_session`).

## 5. Screenshots

**Viewport capture.**
- `Viewport::get_texture()` (scene/main/viewport.h:573) → `ViewportTexture::get_image()` (h:90) → `RS::texture_2d_get(vp->texture_rid)` (viewport.cpp:197-202).
- On RD renderers this calls `RD::texture_get_data` (servers/rendering/renderer_rd/storage_rd/texture_storage.cpp:1891), which is commented as likely to force a flush (rendering_device.h:468), so it is synchronous.
- `texture_get_data_async(rid, layer, Callable)` exists (rendering_device.h:469). Wiring it to a viewport texture via the RD texture RID is not verified.
- Apply `linear_to_srgb` when HDR-2D is on (editor_node.cpp:4018-4023).

**Full editor window.**
- `EditorNode::_save_screenshot` captures the main screen control's viewport (editor_node.cpp:4039-4056). The root equivalent is `EditorNode::get_singleton()->get_tree()->get_root()->get_texture()->get_image()`.
- Floating windows and the separate game window are separate viewports or processes. The embedded-game composite path is editor_node.cpp:4008-4037.

**2D/3D editor viewports.**
- `get_editor_viewport_2d()` → `EditorNode::get_scene_root()` (editor_interface.cpp:422-424).
- `get_editor_viewport_3d(idx)` → `Node3DEditor::get_editor_viewport(idx)->get_viewport_node()` (426-429).
- Whether the 2D one includes gizmo and overlay drawing is not verified.

**OS-level screen capture.**
- `DisplayServer::screen_get_image(screen)` and `screen_get_image_rect(Rect2i)`, plus `screen_get_pixel` (servers/display/display_server.h:351-353).
- Windows supports `FEATURE_SCREEN_CAPTURE` (display_server_windows.cpp:190, implementation at 1545 and 1603).
- This is the fallback for a game running in its own non-embedded window. It captures whatever is visible on screen, including overlapping windows.

## 6. Networking primitives

**TCPServer** (`core/io/tcp_server.h`):
- `listen(uint16_t port, const IPAddress &bind = IPAddress("*"))` (44). The default bind is the wildcard (all interfaces), so pass `"127.0.0.1"` explicitly.
- `get_local_port()` (45), `take_connection()` (46).
- Inherited from SocketServer (core/io/socket_server.h): `is_listening()` and `is_connection_available()` (69-70), `stop()` (73), at most 8 pending connections (41).
- The listen socket is non-blocking (socket_server.cpp:46) and has reuse-address set (tcp_server.cpp:58).

**StreamPeerTCP** (`core/io/stream_peer_tcp.h:36-54`) now derives from `StreamPeerSocket`:
- `connect_to_host` (48), `get_connected_host` / `get_connected_port` (49-50), `set_no_delay` (53).
- From StreamPeerSocket (core/io/stream_peer_socket.h): `get_status()` (75) with NONE / CONNECTING / CONNECTED / ERROR (46-50), `poll()` (78), `wait()` (81), `get_available_bytes()` (74).
- `put_data` / `get_data` block: they call `write`/`read` with `p_block=true` (stream_peer_socket.cpp:199-211), which polls with an infinite timeout (110). `put_partial_data` / `get_partial_data` do not block (204-215). Use only the partial variants on the editor main thread.

**HTTP server.** There is no general HTTP server in core, scene or modules.
- `EditorHTTPServer` exists only for web export (`platform/web/export/editor_http_server.h:36-70`).
- Limits: GET only (cpp:86), one client at a time, 4096-byte request buffer (h:49, cpp:185), 1 s client timeout (cpp:147).
- It runs its own thread, polling every 6.9 ms under a mutex (cpp:37-45, 235), with optional TLS (155-173).
- Treat it as a pattern only. If you use a worker thread, editor API calls still have to be marshaled to the main thread.
- `core/io/http_client.h` and `scene/main/http_request.h` are clients only.

**JSON** (`core/io/json.h`):
- `stringify(var, indent, sort_keys=true, full_precision=false)` (90).
- `parse_string` (91); `parse()` with error line and message (87).
- `from_native(variant, full_objects)` / `to_native` (93-97). Use these to turn Variant property values into JSON-safe data.

**Crypto** (`core/crypto/crypto.h`):
- `Crypto::create()` (127), `generate_random_bytes(n)` (130), `hmac_digest` (139), `constant_time_compare` (143). Use the last for comparing the token.
- `Crypto::create` can return null; EditorHTTPServer checks for this (editor_http_server.cpp:217-220).
- Core-only alternatives: `CryptoCore::RandomGenerator::get_random_bytes` (core/crypto/crypto_core.h:42-52) and `b64_encode_str` (112).

**WebSocket** (`modules/websocket/websocket_peer.h`):
- `accept_stream(StreamPeer)` (84) works with a TCPServer for server use. Also `poll`, `get_ready_state`, `send_text`, `get_requested_url` (95-102).
- The server handshake parses request headers into a local map and does not expose them (modules/websocket/wsl_peer.cpp:170-183). Only the URL (with query string) and the subprotocol are visible (196-200), so an `Authorization` header cannot be checked.
- Claude Code does not use WebSocket for MCP, so this would only be useful for an internal hop.

## 7. Headless and CLI (`main/main.cpp`)

| Option | Help line | Handling |
|---|---|---|
| `--headless` | 626 | 1505-1508: display `"headless"`, audio `"Dummy"` (300-302) |
| `-e` / `--editor` | — | 1598-1600 |
| `--path` | 576 | 1768 |
| `--quit` | 572 | 1787-1791 (also waits for import in TOOLS builds) |
| `--quit-after N` | 573 | 1792-1799 |
| `--remote-debug <uri>` | 666 | 1923-1942 (needs DEBUG or TOOLS) |
| `--debug-server <uri>` | 566 (editor only) | 1605-1616; at start the editor listens there and keeps it open (4745-4747) |
| `-s` / `--script` | 695 | 4118 |
| `--write-movie` | 629 | 1961+ |
| `--fixed-fps` | — | 1953-1960 |
| `--wid` | 646 | 2052-2061 |
| `--embedded` | — | 1510-1516 (macOS only) |
| `--editor-pid` | — | 1943-1950 (not exposed) |
| `--import` | — | 1683-1687 |

Headless is also implied by `--doctool` (1745-1748) and by the `dedicated_server` feature (2290-2292).

**Can headless render a viewport to an image? No.**
- `DisplayServerHeadless` only offers the `"dummy"` rendering driver (servers/display/display_server_headless.h:44-48) and reports no features (59).
- The dummy renderer's `render_target_get_texture` returns `RID()` (servers/rendering/dummy/storage/texture_storage.h:229), and `texture_2d_get` only returns images that were uploaded (108-112).
- So viewport `get_image()` returns nothing in headless mode, and headless runs cannot produce screenshots.

## 8. EditorInterface for driving the editor (`editor/editor_interface.h`)

- **Scene access:** `get_edited_scene_root()` (184; returns `EditorNode::get_edited_scene`, cpp:724-726), `get_open_scenes()` / `get_open_scene_roots()` (180, 183).
- **Opening scenes:** `open_scene_from_path(path, set_inherited)` (174). It returns void and silently does nothing while a scene change is in progress (cpp:699-704). `reload_scene_from_path` (175).
- **Saving:** `save_scene()` returns `ERR_CANT_CREATE` if there is no root or the scene has no path (h:188, cpp:763-773). `save_scene_as` (189), `save_all_scenes` (191), `close_scene` (192).
- **Selection and editing:** `get_selection()` (106, cpp:98-100), `edit_node()` (172, cpp:691-693), `inspect_object` (169), `edit_resource` / `edit_script` (171, 173).
- **Project data and settings:** `get_resource_filesystem()` (103, cpp:86-88), `get_editor_settings()` (107, cpp:102-104).
- **UI:** `set_main_screen_editor(name)` (127, cpp:431-433), `get_base_control` / `get_editor_main_screen` (121-122), `get_editor_viewport_2d` / `3d` (124-125).
- **Other:** `get_editor_undo_redo` (109); play API (196-201); `set_movie_maker_enabled` (203).

## 9. Prior art and MCP and Claude Code

### Godot MCP projects
Star counts and last-commit dates are from the GitHub API as of 2026-10-13.

| Project | Transport and architecture | Run/stop, logs, tree, screenshots | License, last commit |
|---|---|---|---|
| [Coding-Solo/godot-mcp](https://github.com/Coding-Solo/godot-mcp) (~6.0k★) | Node/TS **stdio** (`StdioServerTransport`, src/index.ts:17). No editor addon: shells out to `godot --headless --script godot_operations.gd` and `spawn(godot, ['-d','--path',..])`, capturing stdout/stderr | run/stop/get_debug_output, scene/node creation. No live tree, no screenshots (0 "screenshot" hits in src/index.ts) | MIT, 2026-04-16 |
| [ee0pdt/Godot-MCP](https://github.com/ee0pdt/Godot-MCP) | FastMCP TS (stdio) ↔ **WebSocket :9080** ↔ GDScript addon (TCPServer + WebSocketPeer, addons/godot_mcp/websocket_server.gd:10-13, 41) | scene tree, node/script editing | MIT, 2025-03-19 (stale) |
| [bradypp/godot-mcp](https://github.com/bradypp/godot-mcp) | TS stdio, CLI-only (GodotExecutor/ProcessManager), read-only mode | run/stop/debug output | MIT, 2025-05-31 (stale) |
| GDAI MCP ([gdaimcp.com](https://gdaimcp.com/docs/installation), [repo](https://github.com/3ddelano/gdai-mcp-plugin-godot)) | Editor plugin + Python server via `uv` (stdio by default; optional Streamable HTTP `--transport http --port 9090` since v0.3.3) | debugger output, logs, script errors, editor + game screenshots | Commercial ("All rights reserved"); repo has no license; 2026-09-11 |
| [yurineko73/Godot-MCP-Native](https://github.com/yurineko73/Godot-MCP-Native) (~850★) | **MCP server written in GDScript inside the editor.** Streamable HTTP at `http://localhost:9080/mcp`, optional Bearer token; also a stdio mode (`-- --mcp-server`). Game probe via `EngineDebugger.register_message_capture("mcp")` | scene tree, run/stop, editor screenshot, logs, debugger sessions | MIT, 2026-08-03 |
| [beckettlab/beckett-godot-mcp](https://github.com/beckettlab/beckett-godot-mcp) | "Zero-sidecar" GDScript EditorPlugin: TCPServer HTTP/JSON-RPC polled on the main thread, SSE for list_changed, port 8770. Localhost only, Host/Origin checks, per-project token, writes `.mcp.json`. Separate game channel on :8771, token passed via env var (core/runtime_bridge.gd:5-17, 46-50) | play/stop, logs, game screenshot, remote tree | MIT (Lite) + paid Full; 2026-10-07; README says CI-verified on 4.7.2 |
| [satelliteoflove/godot-mcp](https://github.com/satelliteoflove/godot-mcp) | Node stdio ↔ WebSocket :6550 (127.0.0.1) ↔ editor addon ↔ game over **Godot's debugger protocol** (no extra game port) | run/stop, screenshots, input injection, editor error log | MIT, 2026-09-16 |
| [hi-godot/godot-ai](https://github.com/hi-godot/godot-ai) (~3.0k★) | stdio `attach` → Python server (authenticated HTTP :8000) → editor plugin (authenticated WebSocket :9500, loopback) | screenshots, editor state | MIT, 2026-10-03 |
| [youichi-uda/godot-mcp-pro](https://github.com/youichi-uda/godot-mcp-pro) | Node stdio ↔ WebSocket :6505 ↔ editor plugin; the server part is paid ($15) | editor and game screenshots | NOASSERTION, 2026-09-24 |
| [tugcantopaloglu/godot-mcp](https://github.com/tugcantopaloglu/godot-mcp) | Coding-Solo fork plus a game autoload TCP server on 127.0.0.1:9090 | game_screenshot (base64 PNG) | MIT, 2026-10-09 ("Tested with Godot 4.7") |
| [IvanMurzak/Godot-MCP](https://github.com/IvanMurzak/Godot-MCP) | C# editor tools plus cloud connection (ai-game.dev) | — | Apache-2.0, 2026-10-13 |

None of these is a compiled C++ engine module. All of them are GDScript or C# addons, external processes, or both.

Godot-MCP-Native's `mcp_debugger_bridge.gd:5-12` lists `"output"` as a plugin capture prefix. Based on script_editor_debugger.cpp:986-997 and editor_debugger_node.cpp:949-950, that branch can never be reached (my inference).

### MCP spec
- The latest revision is **2026-07-28** ([Streamable HTTP](https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http)).
- **What changed:** POST to a single endpoint; no sessions or `Mcp-Session-Id`; no GET stream (respond 405 to GET/DELETE); no `initialize` (protocol version goes in each request's `_meta`); list-change notifications via `subscriptions/listen`.
- **Required on every POST:** `MCP-Protocol-Version`, `Mcp-Method`, `Mcp-Name` headers. A mismatch gets 400 with error `-32020`.
- **Security:** invalid `Origin` MUST get **403**; SHOULD bind 127.0.0.1; SHOULD authenticate.
- **Response:** either `application/json` or `text/event-stream`.
- **Backward compatibility:** clients try the modern format first; on a 400 with an unrecognized body they fall back to `initialize` (same page).
- **The earlier 2025-06-18 / 2025-11-25 shape** ([transports](https://modelcontextprotocol.io/specification/2025-06-18/basic/transports)): `initialize` handshake, optional `Mcp-Session-Id`, optional GET SSE (405 allowed), notifications get 202, plus the same Origin, localhost and auth guidance.
- **stdio:** newline-delimited JSON-RPC.
- Which protocol version Claude Code currently negotiates: not verified.

### Claude Code ([code.claude.com/docs/en/mcp](https://code.claude.com/docs/en/mcp))
- Add the server: `claude mcp add --transport http <name> <url> --header "Authorization: Bearer <token>"` (`-t` / `-H` short forms).
- Scopes: `local` (default, stored in `~/.claude.json`), `project` (`.mcp.json`), `user`.
- `.mcp.json` entry: `{"type":"http","url":...,"headers":{...}}`. `${VAR}` and `${VAR:-default}` are expanded in `url` and `headers`. An entry with a `url` but no `type` is treated as stdio.
- SSE transport is deprecated.
- Image tool results are shown inline, but still count against `MAX_MCP_OUTPUT_TOKENS` (warning at 10k, default cap 25k). Downscale screenshots.
- A single JSON response body is capped at 16 MB.

Other sources used: [web search summary](https://glama.ai/mcp/servers/hi-godot/godot-ai), [hybridindie/godot-mcp](https://github.com/hybridindie/godot-mcp), [keeveeg/godot-mcp](https://mcpservers.org/servers/keeveeg/godot-mcp), [GDAI repo listing](https://mcp.so/servers/gdai-mcp-plugin-godot).
